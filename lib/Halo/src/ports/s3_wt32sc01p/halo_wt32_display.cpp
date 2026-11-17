// The WT32-SC01 Plus display for Halo: an ST7796 on the 8-bit parallel bus (8080) of the ESP32-S3's LCD_CAM.
//
// The panel has its own memory, as the round watch's does: a frame is the bands that changed, each drawn into a small
// DMA buffer in internal memory and written into its window. No frame buffer on our side (2 MB of PSRAM stays free).
//
// The bus is ESP-IDF's esp_lcd "i80" driver: it owns LCD_CAM and its DMA channel.
//   * pixels — esp_lcd_panel_io_tx_color: queued, does not wait; "sent" comes as a callback from the interrupt and frees
//     the band's buffer (a counting semaphore, as on the round watch);
//   * a window — CASET (0x2A), PASET (0x2B), then the pixels after RAMWR (0x2C). The window runs to the bottom of the
//     screen: the next band right under this one with the same columns goes after RAMWRC (0x3C, "write on") with no
//     window commands. A command waits until the queued pixels are out (the driver's rule) — a frame of whole-width
//     bands never waits, a frame of narrow spans waits for one band at most (~0.5 ms).
//
// Which way up: MADCTL. The tall screen is the panel's own; the wide one (HALO_LANDSCAPE) swaps rows and columns in the
// panel, so drawing and windows are the same code. Colours: BGR order and inversion on — this board's panel.
//
// TE (GPIO48): the line rises in the pause between the panel's passes over its memory. No signal (the panel is silent,
// the line is not wired on some batch) — a 60 Hz timer stands in for it: frames keep their step, only not tied to the
// pass.
//
// Brightness: the backlight's PWM (GPIO45).
#include <sdkconfig.h>
#if defined(HALO_BOARD_WT32SC01P)  // this board's port: other boards build their own
#include "halo_wt32.h"

#include <driver/gpio.h>
#include <esp_lcd_io_i80.h>
#include <esp_lcd_panel_io.h>

#define MAX_PX (HALO_ROWS * HALO_W)
#define MAX_BUFS 4

// MADCTL: MY 0x80, MX 0x40, MV 0x20 (rows and columns swapped), BGR 0x08
#ifndef HALO_WT32_MADCTL
#ifdef HALO_LANDSCAPE
#define HALO_WT32_MADCTL 0x28
#else
#define HALO_WT32_MADCTL 0x48
#endif
#endif

static esp_lcd_i80_bus_handle_t i80;
static esp_lcd_panel_io_handle_t io;
static uint16_t *bufs[MAX_BUFS];
static int nbufs, next_buf;
static SemaphoreHandle_t free_sem;
static int last_x = -1, last_w, last_y;  // the last band's window
static volatile int madctl_req = -1, invert_req = -1;
static uint8_t madctl = HALO_WT32_MADCTL;

static bool IRAM_ATTR on_sent(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t *, void *) {
  BaseType_t woken = pdFALSE;
  xSemaphoreGiveFromISR(free_sem, &woken);
  return woken == pdTRUE;
}

static void take_free() {
  xSemaphoreTake(free_sem, portMAX_DELAY);
}

uint16_t *hd_buffer() {
  if (uxSemaphoreGetCount(free_sem) == 0) {
    const uint32_t t0 = trace_now();
    take_free();
    trace_span(TR_BLIT_WAIT, t0);
  } else {
    xSemaphoreTake(free_sem, 0);
  }
  return bufs[next_buf];
}

void hd_buffer2(uint16_t **a, uint16_t **b) {
  *a = hd_buffer();
  const uint32_t t0 = trace_now();
  take_free();
  if (trace_now() - t0 >= 5) trace_span(TR_BLIT_WAIT, t0);
  *b = bufs[(next_buf + 1) % nbufs];
}

void hd_send(int x0, int y, int w, int rows) {
  const bool cont = x0 == last_x && w == last_w && y == last_y;
  last_x = x0, last_w = w, last_y = y + rows;
  if (!cont) {
    const int x1 = x0 + w - 1, y1 = HALO_H - 1;  // the window — to the bottom of the screen
    const uint8_t ca[4] = {(uint8_t)(x0 >> 8), (uint8_t)x0, (uint8_t)(x1 >> 8), (uint8_t)x1};
    const uint8_t pa[4] = {(uint8_t)(y >> 8), (uint8_t)y, (uint8_t)(y1 >> 8), (uint8_t)y1};
    esp_lcd_panel_io_tx_param(io, 0x2A, ca, 4);
    esp_lcd_panel_io_tx_param(io, 0x2B, pa, 4);
  }
  esp_lcd_panel_io_tx_color(io, cont ? 0x3C : 0x2C, bufs[next_buf], w * rows * 2);
  next_buf = (next_buf + 1) % nbufs;
}

void hd_wait_idle() {
  for (int i = 0; i < nbufs; i++) take_free();
  for (int i = 0; i < nbufs; i++) xSemaphoreGive(free_sem);
  last_x = -1;  // the next frame's first band — always with its window
  // The bus is free here: what the lcd command asked for (bring-up: which way the panel is turned, its colours)
  if (madctl_req >= 0) {
    madctl = madctl_req, madctl_req = -1;
    esp_lcd_panel_io_tx_param(io, 0x36, &madctl, 1);
  }
  if (invert_req >= 0) esp_lcd_panel_io_tx_param(io, invert_req ? 0x21 : 0x20, nullptr, 0), invert_req = -1;
}

// The backlight: 10 bits of PWM; the eye's curve (a square) — the low steps differ as much as the high ones
static uint8_t bl_level = 180;
void hd_set_brightness(uint8_t level) {
  bl_level = level;
  ledcWrite(HALO_PIN_LCD_BL, level ? max(1, (int)level * level * 1023 / 65025) : 0);
}

void hd_set_power(bool on) {  // the picture stays in the panel's memory; only the light goes
  ledcWrite(HALO_PIN_LCD_BL, on ? max(1, (int)bl_level * bl_level * 1023 / 65025) : 0);
}

int hd_buffers() {
  return nbufs;
}

void hd_state(int *f, int *queued, int *free_bufs) {
  const int fr = free_sem ? (int)uxSemaphoreGetCount(free_sem) : -1;
  *f = fr >= 0 && fr < nbufs ? 0 : -1, *queued = fr >= 0 ? nbufs - fr : 0, *free_bufs = fr;
}

// ---------- TE ----------
static DRAM_ATTR volatile uint32_t te_rise, te_period, te_count, te_blank = 500;
static DRAM_ATTR volatile TaskHandle_t te_waiter;
static portMUX_TYPE te_mux = portMUX_INITIALIZER_UNLOCKED;  // the signal's number and its time — as a pair
static DRAM_ATTR volatile uint32_t te_real;                 // edges of the real line
static esp_timer_handle_t te_timer;                         // stands in when the line is silent

static void IRAM_ATTR te_isr(void *) {
  const uint32_t now = (uint32_t)esp_timer_get_time();
  te_real = te_real + 1;
  if ((REG_READ(GPIO_IN1_REG) >> (HALO_PIN_TE - 32)) & 1) {  // the pause between passes begins
    if (halo_trace_on) halo_trace_put(TR_TE, now, 0, 0);
    portENTER_CRITICAL_ISR(&te_mux);
    if (te_rise && now - te_rise < 25000) te_period = now - te_rise;
    te_rise = now;
    te_count = te_count + 1;
    portEXIT_CRITICAL_ISR(&te_mux);
    TaskHandle_t w = te_waiter;
    if (w) {
      BaseType_t woken = pdFALSE;
      vTaskNotifyGiveFromISR(w, &woken);
      if (woken) portYIELD_FROM_ISR();
    }
  } else {  // the pass over the rows begins
    if (now - te_rise < 4000) te_blank = now - te_rise;
    if (halo_trace_on) halo_trace_put(TR_SCAN, now, 0, 0);
  }
}

static void te_stand_in(void *) {  // the timer's task, 60 times a second
  const uint32_t now = (uint32_t)esp_timer_get_time();
  portENTER_CRITICAL(&te_mux);
  if (te_rise && now - te_rise < 25000) te_period = now - te_rise;
  te_rise = now;
  te_count = te_count + 1;
  portEXIT_CRITICAL(&te_mux);
  TaskHandle_t w = te_waiter;
  if (w) xTaskNotifyGive(w);
}

bool hd_wait_te(uint32_t count, uint32_t timeout_ms) {
  te_waiter = xTaskGetCurrentTaskHandle();
  ulTaskNotifyTake(pdTRUE, 0);  // old notifications do not count — the counter does
  bool ok = true;
  while (ok && (int32_t)(te_count - count) < 0) ok = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms)) > 0;
  te_waiter = nullptr;
  return ok;
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
  return (uint32_t)esp_timer_get_time() - te_rise < 100000 ? te_period : 0;
}
uint32_t hd_te_rise() {
  return te_rise;
}
uint32_t hd_te_blank() {
  return te_blank;
}

void hd_regs(char *out, int n) {
  snprintf(out, n, "i80 %d MHz, madctl %02x, TE %s (%lu edges)", HALO_WT32_PCLK_HZ / 1000000, madctl,
           te_timer ? "timer" : "panel", (unsigned long)te_real);
}

// ---------- Start ----------
static void cmd(uint8_t c, std::initializer_list<uint8_t> data = {}) {
  esp_lcd_panel_io_tx_param(io, c, data.size() ? data.begin() : nullptr, data.size());
}

bool hd_begin(uint32_t, int buffers, uint8_t brightness) {
  // The light stays off until the panel's memory is black
  ledcAttach(HALO_PIN_LCD_BL, 5000, 10);
  ledcWrite(HALO_PIN_LCD_BL, 0);
  bl_level = brightness;

  hc_stage(1);
  esp_lcd_i80_bus_config_t bc = {};
  bc.dc_gpio_num = HALO_PIN_LCD_DC;
  bc.wr_gpio_num = HALO_PIN_LCD_WR;
  bc.clk_src = LCD_CLK_SRC_DEFAULT;
  const int pins[8] = {HALO_PIN_LCD_D0, HALO_PIN_LCD_D1, HALO_PIN_LCD_D2, HALO_PIN_LCD_D3,
                       HALO_PIN_LCD_D4, HALO_PIN_LCD_D5, HALO_PIN_LCD_D6, HALO_PIN_LCD_D7};
  for (int i = 0; i < 8; i++) bc.data_gpio_nums[i] = pins[i];
  bc.bus_width = 8;
  bc.max_transfer_bytes = MAX_PX * 2;
  bc.dma_burst_size = 64;
  if (esp_lcd_new_i80_bus(&bc, &i80) != ESP_OK) {
    LOG("display: the parallel bus did not start\n");
    return false;
  }
  esp_lcd_panel_io_i80_config_t ic = {};
  ic.cs_gpio_num = -1;  // CS is tied low on the board
  ic.pclk_hz = HALO_WT32_PCLK_HZ;
  ic.trans_queue_depth = MAX_BUFS + 2;
  ic.on_color_trans_done = on_sent;
  ic.lcd_cmd_bits = 8;
  ic.lcd_param_bits = 8;
  ic.dc_levels.dc_idle_level = 0;
  ic.dc_levels.dc_cmd_level = 0;
  ic.dc_levels.dc_dummy_level = 0;
  ic.dc_levels.dc_data_level = 1;
  if (esp_lcd_new_panel_io_i80(i80, &ic, &io) != ESP_OK) {
    LOG("display: the panel's port did not start\n");
    return false;
  }

  hc_stage(2);
  // Reset (the line is the touch chip's reset too: it restarts here, before ht_begin)
  gpio_set_direction((gpio_num_t)HALO_PIN_LCD_RST, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)HALO_PIN_LCD_RST, 1);
  delay(10);
  gpio_set_level((gpio_num_t)HALO_PIN_LCD_RST, 0);
  delay(20);
  gpio_set_level((gpio_num_t)HALO_PIN_LCD_RST, 1);
  delay(120);
  cmd(0x11);  // Sleep Out
  delay(120);
  cmd(0xF0, {0xC3});  // the maker's commands: open
  cmd(0xF0, {0x96});
  cmd(0x36, {madctl});
  cmd(0x3A, {0x55});  // 16 bits a pixel
  cmd(0xB4, {0x01});  // inversion by dots
  cmd(0xB6, {0x80, 0x02, 0x3B});
  cmd(0xE8, {0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33});
  cmd(0xC1, {0x06});
  cmd(0xC2, {0xA7});
  cmd(0xC5, {0x18});
  cmd(0xE0, {0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F, 0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B});
  cmd(0xE1, {0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B, 0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B});
  cmd(0xF0, {0x3C});  // the maker's commands: closed
  cmd(0xF0, {0x69});
  cmd(0x21);          // inversion on: this board's panel (IPS)
  cmd(0x35, {0x00});  // TE: the line rises in the pause between passes

  hc_stage(3);
  nbufs = constrain(buffers, 2, MAX_BUFS);
  for (int i = 0; i < nbufs; i++) {
    bufs[i] = (uint16_t *)heap_caps_aligned_alloc(64, MAX_PX * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!bufs[i]) {
      LOG("display: no DMA memory\n");
      return false;
    }
    memset(bufs[i], 0, MAX_PX * 2);
  }
  free_sem = xSemaphoreCreateCounting(nbufs, nbufs);
  for (int y = 0; y < HALO_H; y += HALO_ROWS) {  // black, band by band — the way frames go
    uint16_t *b = hd_buffer();
    memset(b, 0, MAX_PX * 2);
    hd_send(0, y, HALO_W, min(HALO_ROWS, HALO_H - y));
  }
  hd_wait_idle();
  cmd(0x29);  // Display On — black already
  hd_set_brightness(brightness);

  gpio_config_t te = {};
  te.pin_bit_mask = 1ull << HALO_PIN_TE;
  te.mode = GPIO_MODE_INPUT;
  te.intr_type = GPIO_INTR_ANYEDGE;
  gpio_config(&te);
  gpio_install_isr_service(ESP_INTR_FLAG_IRAM);  // already there — no harm
  gpio_isr_handler_add((gpio_num_t)HALO_PIN_TE, te_isr, nullptr);
  delay(60);  // three passes: is the line alive?
  if (te_real < 2) {
    gpio_isr_handler_remove((gpio_num_t)HALO_PIN_TE);
    const esp_timer_create_args_t ta = {te_stand_in, nullptr, ESP_TIMER_TASK, "halo_te", true};
    if (esp_timer_create(&ta, &te_timer) == ESP_OK) esp_timer_start_periodic(te_timer, 16667);
  }
  halo_command("lcd", [](const char *a) {  // lcd madctl HEX | lcd inv 0|1 — which way the panel is turned, its colours
    unsigned v = 0;
    if (sscanf(a, "madctl %x", &v) == 1) madctl_req = v & 0xFF;
    else if (sscanf(a, "inv %u", &v) == 1) invert_req = v ? 1 : 0;
    else return (void)Serial.println("lcd madctl HEX | lcd inv 0|1");
    halo_damage_all();
  });
  LOG("display: ST7796 ok, %d x %d, bus %d MHz, %d DMA buffers, TE from the %s\n", HALO_W, HALO_H,
      HALO_WT32_PCLK_HZ / 1000000, nbufs, te_timer ? "timer (the line is silent)" : "panel");
  return true;
}

#endif  // HALO_BOARD_WT32SC01P
