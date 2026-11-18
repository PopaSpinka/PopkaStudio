// The ESP32-S3-Touch-LCD-4B's touch and power chip: GT911 (I2C 0x5D) and AXP2101 (0x34) on the board's one I2C bus.
//
// Touch. The chip's INT and reset lines go to the TCA9554 expander, not to the processor, so reports are polled — the P4
// board's way (ports/p4_lcd7/halo_p4_touch.cpp): they come every 10 ms while a finger is down; after a report there is
// nothing to ask for 8 ms, then the task asks without pausing until the next one appears. The reset picks the address:
// INT held low while RST rises — 0x5D. Status at 0x814E: bit 7 — a new report, bits 0-3 — how many fingers; 8 bytes a
// finger from 0x814F (track id, x, y, size); writing 0 to 0x814E takes the report. The axes are the panel's own.
//
// The PWR button is read by the power chip, as on the round watch (ports/s3_amoled175/halo_touch.cpp): its interrupt flags
// every 100 ms (register 0x49: bit 3 — a short press, after release; bit 2 — a long one, ~1.5 s, while held), cleared by
// writing the same bits. Every 2 s — the battery (same registers and fuel gauge as there).
#include <sdkconfig.h>
#if defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_lcd4b.h"

#include <driver/i2c_master.h>

#define REPORT_AFTER_US 8000  // nothing to ask for this long after a report
#define RELEASE_US 100000     // no reports for this long: the finger is gone even without a "no fingers" report

static i2c_master_dev_handle_t tp, pmu;

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

// ---------- The power chip: the PWR button and the battery ----------
static volatile uint8_t pwr_ev;  // HALO_PWR_* since the last halo_pwr_event()
static uint32_t pmu_at, batt_at;
static HaloBattery batt;
static portMUX_TYPE batt_mux = portMUX_INITIALIZER_UNLOCKED;

static uint8_t pmu_reg(uint8_t r) {
  uint8_t v = 0;
  i2c_master_transmit_receive(pmu, &r, 1, &v, 1, 20);
  return v;
}
static uint16_t pmu_adc(uint8_t hi) {  // 6 bits from hi, 8 from the next one
  uint8_t b[2] = {0, 0};
  i2c_master_transmit_receive(pmu, &hi, 1, b, 2, 20);
  return (uint16_t)((b[0] & 0x3F) << 8 | b[1]);
}
static void pmu_set_bits(uint8_t r, uint8_t bits) {
  const uint8_t w[2] = {r, (uint8_t)(pmu_reg(r) | bits)};
  i2c_master_transmit(pmu, w, 2, 20);
}

static void battery_poll() {
  if (!pmu || (batt.ok && millis() - batt_at < 2000)) return;
  batt_at = millis();
  const uint8_t s1 = pmu_reg(0x00), s2 = pmu_reg(0x01);
  HaloBattery b;
  b.ok = true, b.present = s1 & 0x08, b.usb = (s1 & 0x20) && !(s2 & 0x08), b.charging = (s2 >> 5) == 1;
  b.percent = b.present ? (int8_t)min(100, (int)pmu_reg(0xA4)) : -1;
  b.mv = b.present ? (uint16_t)((pmu_reg(0x34) & 0x1F) << 8 | pmu_reg(0x35)) : 0;
  b.vbus_mv = b.usb ? pmu_adc(0x38) : 0;
  b.vsys_mv = pmu_adc(0x3A);
  b.temp10 = (int16_t)(220 + (7274 - (int)pmu_adc(0x3C)) / 2);  // (22 + (7274 − x) / 20) · 10
  portENTER_CRITICAL(&batt_mux);
  batt = b;
  portEXIT_CRITICAL(&batt_mux);
}

HaloBattery halo_battery() {
  portENTER_CRITICAL(&batt_mux);
  const HaloBattery b = batt;
  portEXIT_CRITICAL(&batt_mux);
  return b;
}

static void pmu_poll() {
  if (!pmu || millis() - pmu_at < 100) return;
  pmu_at = millis();
  uint8_t reg = 0x49, st = 0;
  if (i2c_master_transmit_receive(pmu, &reg, 1, &st, 1, 20) != ESP_OK || !(st &= 0x0C)) return;
  const uint8_t clr[2] = {0x49, st};
  i2c_master_transmit(pmu, clr, 2, 20);
  const uint8_t ev = (st & 0x08 ? HALO_PWR_CLICK : 0) | (st & 0x04 ? HALO_PWR_LONG : 0);
  __atomic_fetch_or(&pwr_ev, ev, __ATOMIC_RELAXED);
  LOG("pwr:%s%s\n", ev & HALO_PWR_CLICK ? " click" : "", ev & HALO_PWR_LONG ? " long" : "");
}

uint8_t halo_pwr_event() {
  return __atomic_exchange_n(&pwr_ev, 0, __ATOMIC_RELAXED);
}

// ---------- Touch ----------
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
    pmu_poll();
    battery_poll();
    if (!tp) {
      vTaskDelay(50);
      continue;
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
    const int x = p[i * 8 + 1] | (p[i * 8 + 2] << 8), y = p[i * 8 + 3] | (p[i * 8 + 4] << 8);
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

// ---------- I2C for the sketch (the same bus: the driver takes turns between tasks by itself) ----------
HaloI2c halo_i2c(uint8_t addr) {
  return hx_device(addr);
}

bool halo_i2c_write(HaloI2c d, uint8_t reg, uint8_t val) {
  const uint8_t w[2] = {reg, val};
  return d && i2c_master_transmit(d, w, 2, 20) == ESP_OK;
}

bool halo_i2c_read(HaloI2c d, uint8_t reg, uint8_t *buf, int n) {
  return d && i2c_master_transmit_receive(d, &reg, 1, buf, n, 20) == ESP_OK;
}

bool ht_begin() {
  if (!hx_bus()) return false;
  // The reset through the expander: INT low while RST rises — address 0x5D; then INT is the chip's output
  hx_dir(HX_TP_INT, true);
  hx_set(HX_TP_INT, false);
  hx_set(HX_TP_RST, false);
  delay(10);
  hx_set(HX_TP_RST, true);
  delay(60);
  hx_dir(HX_TP_INT, false);
  delay(20);
  tp = hx_device(0x5D);
  if (!tp) tp = hx_device(0x14);
  // The power chip: short and long PWR press interrupts (0x41, bits 3 and 2), old flags cleared
  pmu = hx_device(0x34);
  if (pmu) {
    uint8_t reg = 0x41, en = 0;
    i2c_master_transmit_receive(pmu, &reg, 1, &en, 1, 20);
    const uint8_t w[2] = {0x41, (uint8_t)(en | 0x0C)}, c[2] = {0x49, 0x0F};
    i2c_master_transmit(pmu, w, 2, 20);
    i2c_master_transmit(pmu, c, 2, 20);
    pmu_set_bits(0x30, 0x1D), pmu_set_bits(0x68, 0x01), pmu_set_bits(0x18, 0x08);  // ADC: battery, USB, system,
                                                                                    // temperature; battery detect, fuel gauge
  }
  halo_command("battery", [](const char *) {
    const HaloBattery b = halo_battery();
    LOG("battery: %s, %d%%, %u mV, usb %s (%u mV), %s; system %u mV, power chip %.1f C\n", b.present ? "present" : "none",
        b.percent, b.mv, b.usb ? "yes" : "no", b.vbus_mv, b.charging ? "charging" : "not charging", b.vsys_mv,
        b.temp10 / 10.0f);
  });
  halo_command("pwr", [](const char *a) {  // pwr / pwr long — the same as the PWR button
    __atomic_fetch_or(&pwr_ev, strstr(a, "long") ? HALO_PWR_LONG : HALO_PWR_CLICK, __ATOMIC_RELAXED);
  });
  xTaskCreatePinnedToCore(touch_task, "halo_touch", 4096, nullptr, 12, nullptr, 0);  // (LOG takes most of it)
  LOG("touch: GT911 %s, PWR button (AXP2101) %s\n", tp ? "ok" : "NOT FOUND", pmu ? "ok" : "NOT FOUND");
  return tp != nullptr;
}

#endif  // HALO_BOARD_LCD4B
