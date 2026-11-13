// The P4 board's display: EK79007 1024x600 over two-lane MIPI DSI, video mode. Measured on this board (boards/p4lcd7/hardware.md):
// refresh 16562.5 us with +-1.5 us of jitter, a buffer swap costs 0.25 ms, no late frames.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"

#include <driver/gpio.h>
#include <esp_cache.h>
#include <esp_ldo_regulator.h>
#include <esp_lcd_mipi_dsi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <hal/mipi_dsi_brg_ll.h>
#include <hal/mipi_dsi_host_ll.h>

static esp_lcd_dsi_bus_handle_t bus;
static esp_lcd_panel_io_handle_t io;
static esp_lcd_panel_handle_t panel;
static uint16_t *fb[2];
static int back = 1;
static bool bl_ready;
static uint8_t bl_level;
static int pixel_bytes = 2;

static volatile uint32_t te_count, te_rise, te_period;
static volatile uint32_t wait_for;
static TaskHandle_t volatile waiter;
static portMUX_TYPE te_mux = portMUX_INITIALIZER_UNLOCKED;

// Every frame boundary: the panel has finished a pass, a buffer swapped in earlier is now the one on screen
static bool IRAM_ATTR on_boundary(esp_lcd_panel_handle_t, esp_lcd_dpi_panel_event_data_t *, void *) {
  const uint32_t now = trace_now();
  BaseType_t woken = pdFALSE;
  portENTER_CRITICAL_ISR(&te_mux);
  if (te_rise) {
    const uint32_t p = now - te_rise;
    if (p > 10000 && p < 30000) te_period = te_period ? (te_period * 15 + p + 8) / 16 : p;
  }
  te_rise = now;
  te_count = te_count + 1;
  const bool wake = waiter && (int32_t)(te_count - wait_for) >= 0;
  TaskHandle_t t = waiter;
  if (wake) waiter = nullptr;
  portEXIT_CRITICAL_ISR(&te_mux);
  if (wake) vTaskNotifyGiveFromISR(t, &woken);
  trace_mark(TR_TE);
  return woken == pdTRUE;
}

bool hd_wait_te(uint32_t count, uint32_t timeout_ms) {
  ulTaskNotifyTake(pdTRUE, 0);  // (a leftover from an earlier wait)
  portENTER_CRITICAL(&te_mux);
  if ((int32_t)(te_count - count) >= 0) {
    portEXIT_CRITICAL(&te_mux);
    return true;
  }
  wait_for = count, waiter = xTaskGetCurrentTaskHandle();
  portEXIT_CRITICAL(&te_mux);
  const bool got = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms)) != 0;
  waiter = nullptr;
  return got;
}

void hd_te_snapshot(uint32_t *count, uint32_t *rise) {
  portENTER_CRITICAL(&te_mux);
  *count = te_count, *rise = te_rise;
  portEXIT_CRITICAL(&te_mux);
}

uint32_t hd_te_count() {
  return te_count;
}

uint32_t hd_te_period() {
  return te_period;
}

uint16_t *hd_back() {
  return fb[back];
}

uint16_t *hd_front() {
  return fb[1 - back];
}

void hd_flip() {
  esp_lcd_panel_draw_bitmap(panel, 0, 0, HALO_W, HALO_H, fb[back]);  // (a whole buffer of its own: a swap, not a copy)
  back = 1 - back;
}

static void backlight(uint32_t duty) {
  if (!bl_ready) {
    ledcAttach(HALO_PIN_BACKLIGHT, 5000, 10);
    ledcOutputInvert(HALO_PIN_BACKLIGHT, true);  // active low
    bl_ready = true;
  }
  ledcWrite(HALO_PIN_BACKLIGHT, duty);
}

void hd_set_brightness(uint8_t level) {
  bl_level = level;
  backlight((uint32_t)level * 1023 / 255);
}

void hd_set_power(bool on) {  // the backlight is what shows the picture; the panel keeps running, frames go on
  backlight(on ? (uint32_t)bl_level * 1023 / 255 : 0);
}

int hd_buffers() {
  return 2;
}

void hd_state(int *fly, int *queued, int *free_bufs) {
  *fly = -1, *queued = 0, *free_bufs = 1;
}

void hd_regs(char *out, int n) {
  snprintf(out, n, "dsi: boundaries %lu, period %lu us", (unsigned long)te_count, (unsigned long)te_period);
}

// One line lasts 1354 clocks of 52 MHz = 26.04 us. The sync (1) and the back porch (23) as in hd_begin; only the blank rows
// after the picture change. Written right after a boundary: the frame in progress finishes as it began. The frame task
// writes it at its next start, right after the boundary it waits for (hd_rate_apply): it waited here for that boundary
// itself — up to a whole refresh in the middle of a frame, and the frame after it came a refresh late (boards/p4lcd7/p4-video.md)
static volatile float rate_req = -1;
void hd_set_rate(float hz) {
  rate_req = hz;
}
void hd_rate_apply() {
  const float hz = rate_req;
  if (hz < 0 || (int32_t)(trace_now() - te_rise) > 4000) return;  // (not right after a boundary: at the next one)
  rate_req = -1;
  const int lines = hz > 0 ? (int)(1e6f / hz / (1354 / 52.0f) + 0.5f) : 600 + 1 + 23 + 12, vfp = max(12, lines - 600 - 1 - 23);
  dsi_host_dev_t *h = MIPI_DSI_LL_GET_HOST(0);
  dsi_brg_dev_t *b = MIPI_DSI_LL_GET_BRG(0);
  mipi_dsi_host_ll_dpi_set_vertical_timing(h, 1, 23, 600, vfp);
  mipi_dsi_brg_ll_set_vertical_timing(b, 1, 23, 600, vfp);
  mipi_dsi_brg_ll_update_dpi_config(b);
}

void hd_use_rgb888() {
  pixel_bytes = 3;
}

bool hd_begin(uint8_t brightness) {
  hc_stage(1);
  esp_ldo_channel_handle_t ldo = nullptr;
  esp_ldo_channel_config_t ldo_cfg = {};
  ldo_cfg.chan_id = 3;  // powers the D-PHY
  ldo_cfg.voltage_mv = 2500;
  if (esp_ldo_acquire_channel(&ldo_cfg, &ldo) != ESP_OK) return false;

  esp_lcd_dsi_bus_config_t bus_cfg = {};
  bus_cfg.bus_id = 0;
  bus_cfg.num_data_lanes = 2;
  bus_cfg.lane_bit_rate_mbps = 1000;
  if (esp_lcd_new_dsi_bus(&bus_cfg, &bus) != ESP_OK) return false;

  esp_lcd_dbi_io_config_t io_cfg = {};
  io_cfg.lcd_cmd_bits = 8;
  io_cfg.lcd_param_bits = 8;
  if (esp_lcd_new_panel_io_dbi(bus, &io_cfg, &io) != ESP_OK) return false;

  esp_lcd_dpi_panel_config_t dpi = {};
  dpi.dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT;
  dpi.dpi_clock_freq_mhz = 52;
  dpi.pixel_format = pixel_bytes == 3 ? LCD_COLOR_PIXEL_FORMAT_RGB888 : LCD_COLOR_PIXEL_FORMAT_RGB565;
  dpi.num_fbs = 2;
  dpi.video_timing.h_size = HALO_W;
  dpi.video_timing.v_size = HALO_H;
  dpi.video_timing.hsync_pulse_width = 10;
  dpi.video_timing.hsync_back_porch = 160;
  dpi.video_timing.hsync_front_porch = 160;
  dpi.video_timing.vsync_pulse_width = 1;
  dpi.video_timing.vsync_back_porch = 23;
  dpi.video_timing.vsync_front_porch = 12;
  if (esp_lcd_new_panel_dpi(bus, &dpi, &panel) != ESP_OK) return false;

  hc_stage(2);
  gpio_config_t rst = {};
  rst.pin_bit_mask = 1ULL << HALO_PIN_LCD_RST;
  rst.mode = GPIO_MODE_OUTPUT;
  gpio_config(&rst);
  gpio_set_level((gpio_num_t)HALO_PIN_LCD_RST, 1);
  delay(5);
  gpio_set_level((gpio_num_t)HALO_PIN_LCD_RST, 0);
  delay(10);
  gpio_set_level((gpio_num_t)HALO_PIN_LCD_RST, 1);
  delay(120);
  static const uint8_t init[][2] = {{0xB2, 0x10}, {0x80, 0x8B}, {0x81, 0x78}, {0x82, 0x84},
                                    {0x83, 0x88}, {0x84, 0xA8}, {0x85, 0xE3}, {0x86, 0x88}};  // EK79007, as in the board's BSP
  for (auto &c : init)
    if (esp_lcd_panel_io_tx_param(io, c[0], &c[1], 1) != ESP_OK) return false;
  if (esp_lcd_panel_io_tx_param(io, 0x11, nullptr, 0) != ESP_OK) return false;  // sleep out
  delay(120);

  hc_stage(3);
  esp_lcd_dpi_panel_event_callbacks_t cbs = {};
  cbs.on_frame_buf_complete = on_boundary;
  if (esp_lcd_dpi_panel_register_event_callbacks(panel, &cbs, nullptr) != ESP_OK) return false;
  if (esp_lcd_panel_init(panel) != ESP_OK) return false;
  if (esp_lcd_dpi_panel_get_frame_buffer(panel, 2, (void **)&fb[0], (void **)&fb[1]) != ESP_OK) return false;
  for (auto f : fb) {
    memset(f, 0, HALO_W * HALO_H * pixel_bytes);
    esp_cache_msync(f, HALO_W * HALO_H * pixel_bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  }
  hd_set_brightness(brightness);
  return true;
}

#endif  // CONFIG_IDF_TARGET_ESP32P4
