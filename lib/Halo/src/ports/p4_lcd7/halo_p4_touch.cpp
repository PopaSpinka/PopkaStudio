// The P4 board's touch: GT911 on the shared I2C bus (GPIO7/8, also the sound chips).
//
// The chip's INT line does not reach the processor (boards/p4lcd7/hardware.md), so reports are polled. They come exactly every
// 10 ms while a finger is down (measured: 98.7 a second, 10-11 ms apart, none lost), which is what the polling is built
// around: after a report there is nothing to ask for 8 ms, then the task asks without pausing until the next one appears.
// Each question is 175 us on the bus and the task sleeps through it, so a report's time is known to about 0.2 ms; the time
// matters more than the position here, since the finger code interpolates between reports by these times.
//
// Status at 0x814E: bit 7 — a new report, bits 0-3 — how many fingers; 8 bytes a finger from 0x814F (track id, x, y, size);
// writing 0 to 0x814E takes the report. Both axes of the touch layer are mirrored against the panel.
// The chip's address is 0x5D or 0x14: which one depends on a line we do not drive, and it can change after a reset.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>

#define PIN_SDA GPIO_NUM_7
#define PIN_SCL GPIO_NUM_8
#define REPORT_AFTER_US 8000  // nothing to ask for this long after a report
#define RELEASE_US 100000     // no reports for this long: the finger is gone even without a "no fingers" report

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t tp;
static uint8_t tp_addr;

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

static bool gt_read(uint16_t reg, uint8_t *out, size_t n) {
  const uint8_t r[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
  return i2c_master_transmit_receive(tp, r, 2, out, n, 20) == ESP_OK;
}

static bool gt_take() {  // the report is read: the chip may write the next one
  const uint8_t w[3] = {0x81, 0x4E, 0};
  return i2c_master_transmit(tp, w, 3, 20) == ESP_OK;
}

static int16_t down_x, down_y, last_x, last_y;
static uint16_t reports;

static void release(const char *why) {
  portENTER_CRITICAL(&hist_mux);
  hist_n = 0;
  portEXIT_CRITICAL(&hist_mux);
  if (reports) LOG("touch up %d,%d: moved %+d,%+d, %u reports (%s)\n", last_x, last_y, last_x - down_x, last_y - down_y, reports, why);
  reports = 0;
}

static void touch_task(void *) {
  int track = -1;      // the finger we follow: with two on the glass their order in the report changes
  uint32_t last = 0;   // when the latest report came
  uint32_t asked = 0;  // questions in a row without an answer
  for (;;) {
    if (hist_n) {
      const int32_t left = (int32_t)(REPORT_AFTER_US - (trace_now() - last));
      if (left >= 1000) vTaskDelay(left / 1000);
      if (asked >= 200) vTaskDelay(1);  // (40 ms of asking: a finger that stopped reporting, stay polite on the bus)
    } else {
      vTaskDelay(3);
    }
    uint8_t st = 0;
    const uint32_t t = trace_now();
    if (!gt_read(0x814E, &st, 1)) {
      vTaskDelay(5);
      continue;
    }
    if (!(st & 0x80)) {
      asked++;
      if (hist_n && t - last > RELEASE_US) track = -1, release("timeout");
      continue;
    }
    asked = 0;
    const int n = st & 0x0F;
    if (n < 1 || n > 5) {
      gt_take();
      track = -1;
      release("lift");
      continue;
    }
    uint8_t p[40];
    const bool ok = gt_read(0x814F, p, n * 8);
    gt_take();
    if (!ok) continue;
    int i = -1;
    if (track < 0) i = 0, track = p[0];
    else
      for (int k = 0; k < n; k++)
        if (p[k * 8] == track) i = k;
    if (i < 0) {  // the finger we followed is gone and another one stays: a new touch, not a jump to where that one is
      track = -1;
      release("finger changed");
      continue;
    }
    const int x = HALO_W - 1 - (p[i * 8 + 1] | (p[i * 8 + 2] << 8)), y = HALO_H - 1 - (p[i * 8 + 3] | (p[i * 8 + 4] << 8));
    const HaloTouchSample s = {(int16_t)constrain(x, 0, HALO_W - 1), (int16_t)constrain(y, 0, HALO_H - 1), t};
    portENTER_CRITICAL(&hist_mux);
    hist_head = (hist_head + 1) % HIST;
    hist[hist_head] = s;
    if (hist_n < HIST) hist_n++;
    portEXIT_CRITICAL(&hist_mux);
    last = t;
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
  bc.sda_io_num = PIN_SDA;
  bc.scl_io_num = PIN_SCL;
  bc.clk_source = I2C_CLK_SRC_DEFAULT;
  bc.glitch_ignore_cnt = 7;
  bc.flags.enable_internal_pullup = 1;
  if (i2c_new_master_bus(&bc, &bus) != ESP_OK) return false;
  for (uint8_t a : {(uint8_t)0x5D, (uint8_t)0x14}) {
    if (i2c_master_probe(bus, a, 50) != ESP_OK) continue;
    i2c_device_config_t dc = {};
    dc.dev_addr_length = I2C_ADDR_BIT_LEN_7, dc.device_address = a, dc.scl_speed_hz = 400000;
    if (i2c_master_bus_add_device(bus, &dc, &tp) == ESP_OK) tp_addr = a;
    break;
  }
  if (!tp) {
    LOG("touch: GT911 NOT FOUND\n");
    return false;
  }
  xTaskCreatePinnedToCore(touch_task, "halo_touch", 4096, nullptr, 12, nullptr, 0);  // (LOG takes most of it)
  LOG("touch: GT911 at 0x%02X\n", tp_addr);
  return true;
}

#endif  // CONFIG_IDF_TARGET_ESP32P4
