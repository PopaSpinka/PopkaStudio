// The WT32-SC01 Plus touch: FT6336U on I2C (0x38, GPIO6/5), its INT line on GPIO7.
//
// While a finger is down the chip lowers INT on each new report (trigger mode, register 0xA4 = 1; about 60–100 a
// second). The interrupt keeps the time and wakes the reading task (core 0). Should the line stay silent (the chip left
// in its "low while touched" mode, or a board without the line), the task asks by itself: every 12 ms with a finger
// down, every 50 ms without — so a touch is never lost, only timed a little worse.
//
// A report — 5 bytes from 0x02: how many fingers; then the first one: [event << 6 | x >> 8] [x] [id << 4 | y >> 8] [y].
// The chip's axes are the tall panel's (x 0..319, y 0..479); the wide screen turns them the way MADCTL turns the picture.
// The chip's reset line is the panel's: halo_wt32_display.cpp has pulsed it already.
#include <sdkconfig.h>
#if defined(HALO_BOARD_WT32SC01P)  // this board's port: other boards build their own
#include "halo_wt32.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t tp;
static TaskHandle_t task;
static DRAM_ATTR volatile uint32_t irq_us;
static volatile uint8_t turn_req = 0xFF;
// How the chip's axes lie on the screen: bit 0 — swap x and y, bit 1 — mirror x, bit 2 — mirror y (after the swap).
// The touch command changes it on a live board (bring-up)
#ifdef HALO_LANDSCAPE
static uint8_t turn = 1 | 4;
#else
static uint8_t turn = 0;
#endif

#define HIST 8
static HaloTouchSample hist[HIST];
static int hist_n, hist_head;
static portMUX_TYPE hist_mux = portMUX_INITIALIZER_UNLOCKED;

int halo_touch_samples(HaloTouchSample *out, int max) {
  portENTER_CRITICAL(&hist_mux);
  const int n = min(max, hist_n);
  for (int i = 0; i < n; i++) out[i] = hist[(hist_head - i + HIST) % HIST];
  portEXIT_CRITICAL(&hist_mux);
  return n;
}

static void IRAM_ATTR tp_isr(void *) {
  irq_us = (uint32_t)esp_timer_get_time();
  if (halo_trace_on) halo_trace_put(TR_TOUCH, irq_us, 0, 0);
  BaseType_t woken = pdFALSE;
  if (task) vTaskNotifyGiveFromISR(task, &woken);
  if (woken) portYIELD_FROM_ISR();
}

static int16_t down_x, down_y, last_x, last_y;
static uint16_t reports;

static void release(const char *why) {
  portENTER_CRITICAL(&hist_mux);
  hist_n = 0;
  portEXIT_CRITICAL(&hist_mux);
  if (reports) LOG("touch up %d,%d: moved %+d,%+d, %u reports (%s)\n", last_x, last_y, last_x - down_x, last_y - down_y,
                   reports, why);
  reports = 0;
}

static void touch_task(void *) {
  int16_t px = -1, py = -1;  // the last report's raw point: the same one again is not a new report
  for (;;) {
    const bool told = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(hist_n ? 12 : 50)) > 0;
    const uint32_t t = told ? irq_us : (uint32_t)esp_timer_get_time();
    if (turn_req != 0xFF) turn = turn_req, turn_req = 0xFF;
    uint8_t reg = 0x02, b[5] = {};
    if (i2c_master_transmit_receive(tp, &reg, 1, b, sizeof(b), 20) != ESP_OK) continue;
    const int n = b[0] & 0x0F;
    if (!n || n > 2 || (b[1] >> 6) == 1) {  // no fingers, or the finger's event is "lifted"
      if (hist_n) release("lift");
      px = py = -1;
      continue;
    }
    const int16_t rx = (b[1] & 0x0F) << 8 | b[2], ry = (b[3] & 0x0F) << 8 | b[4];
    if (!told && rx == px && ry == py) continue;  // asked by ourselves and nothing moved: the chip has no new report
    px = rx, py = ry;
    int x = rx, y = ry, w = 320, h = 480;
    if (turn & 1) x = ry, y = rx, w = 480, h = 320;
    if (turn & 2) x = w - 1 - x;
    if (turn & 4) y = h - 1 - y;
    const HaloTouchSample s = {(int16_t)constrain(x, 0, HALO_W - 1), (int16_t)constrain(y, 0, HALO_H - 1), t};
    portENTER_CRITICAL(&hist_mux);
    hist_head = (hist_head + 1) % HIST;
    hist[hist_head] = s;
    if (hist_n < HIST) hist_n++;
    portEXIT_CRITICAL(&hist_mux);
    trace_mark(TR_TOUCH_READ, s.x, s.y);
    if (!reports++) down_x = s.x, down_y = s.y, LOG("touch down %d,%d\n", s.x, s.y);
    last_x = s.x, last_y = s.y;
  }
}

HaloI2c halo_i2c(uint8_t addr) {
  if (!bus || i2c_master_probe(bus, addr, 50) != ESP_OK) return nullptr;
  i2c_device_config_t dc = {};
  dc.dev_addr_length = I2C_ADDR_BIT_LEN_7, dc.device_address = addr, dc.scl_speed_hz = 400000;
  i2c_master_dev_handle_t h = nullptr;
  return i2c_master_bus_add_device(bus, &dc, &h) == ESP_OK ? h : nullptr;
}

bool halo_i2c_write(HaloI2c d, uint8_t reg, uint8_t val) {  // (the bus driver takes turns between tasks by itself)
  const uint8_t w[2] = {reg, val};
  return d && i2c_master_transmit(d, w, 2, 20) == ESP_OK;
}

bool halo_i2c_read(HaloI2c d, uint8_t reg, uint8_t *buf, int n) {
  return d && i2c_master_transmit_receive(d, &reg, 1, buf, n, 20) == ESP_OK;
}

bool ht_begin() {
  i2c_master_bus_config_t bc = {};
  bc.i2c_port = I2C_NUM_0;
  bc.sda_io_num = (gpio_num_t)HALO_PIN_SDA;
  bc.scl_io_num = (gpio_num_t)HALO_PIN_SCL;
  bc.clk_source = I2C_CLK_SRC_DEFAULT;
  bc.glitch_ignore_cnt = 7;
  bc.flags.enable_internal_pullup = 1;
  if (i2c_new_master_bus(&bc, &bus) != ESP_OK) return false;
  bool found = false;
  for (int i = 0; i < 10 && !found; i++) {  // (the chip restarted with the panel: it answers ~300 ms after the reset)
    found = i2c_master_probe(bus, 0x38, 50) == ESP_OK;
    if (!found) delay(50);
  }
  i2c_device_config_t dc = {};
  dc.dev_addr_length = I2C_ADDR_BIT_LEN_7, dc.device_address = 0x38, dc.scl_speed_hz = 400000;
  if (!found || i2c_master_bus_add_device(bus, &dc, &tp) != ESP_OK) {
    LOG("touch: FT6336 NOT FOUND\n");
    return false;
  }
  const uint8_t trig[2] = {0xA4, 0x01};  // INT: a pulse for each report
  i2c_master_transmit(tp, trig, 2, 20);
  uint8_t reg = 0xA8, vendor = 0;
  i2c_master_transmit_receive(tp, &reg, 1, &vendor, 1, 20);
  halo_command("touch", [](const char *a) {  // touch turn N — how the chip's axes lie on the screen (bits: swap, mirror x, mirror y)
    int v = 0;
    if (sscanf(a, "turn %d", &v) == 1) turn_req = v & 7;
    else Serial.printf("touch turn N (now %d): 1 — swap x and y, 2 — mirror x, 4 — mirror y\n", turn);
  });

  xTaskCreatePinnedToCore(touch_task, "halo_touch", 4096, nullptr, 12, &task, 0);
  gpio_config_t io = {};
  io.pin_bit_mask = 1ull << HALO_PIN_TP_INT;
  io.mode = GPIO_MODE_INPUT;
  io.pull_up_en = GPIO_PULLUP_ENABLE;
  io.intr_type = GPIO_INTR_NEGEDGE;
  gpio_config(&io);
  gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  gpio_isr_handler_add((gpio_num_t)HALO_PIN_TP_INT, tp_isr, nullptr);
  LOG("touch: FT6336 ok (vendor 0x%02X)\n", vendor);
  return true;
}

#endif  // HALO_BOARD_WT32SC01P
