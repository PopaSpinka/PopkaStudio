// Sleep and wake-up for Halo: light sleep of the ESP32 — memory and program stay, the board goes on where it
// stopped, the picture is back in ~20–35 ms — and the motion sensor that can wake it. docs/halo.md, section 12.
// The same sensor gives accelerometer and gyroscope data while a screen asks for them (halo_imu_*, section 13).
//
// The sketch asks (halo_sleep) inside update; the frame engine calls hp_frame_end() once that frame is on the
// display, and the board sleeps right there. Wake-ups: a touch (CST9217 INT, GPIO11, low), BOOT (GPIO0, low),
// motion (QMI8658 INT1, GPIO21 — the level opposite to the current one; after halo_motion_begin), a timer. The
// next update sees halo_woke().
// On USB power, or while someone holds halo_stay_awake (Wi-Fi), the board only pretends: a really sleeping board
// leaves the USB port silent and cannot be flashed. The same wake-ups are then watched once a frame.
// Time during sleep is counted by the RTC slow clock. The default one, the slow RC oscillator (~140 kHz), ran 0.54 %
// fast in light sleep (half a minute per hour asleep); the fast RC oscillator / 256 (~70 kHz) — 0.03 %. It is
// switched over before the first sleep, kept running during sleep and measured against the crystal every 5 minutes.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32S3  // this board's port: other boards build their own
#include "halo_s3.h"

#include <driver/gpio.h>
#include <esp_private/esp_clk.h>
#include <esp_sleep.h>
#include <soc/rtc.h>

// ---------- Motion sensor: wake-on-motion on INT1 (register sequence as SensorLib configWakeOnMotion) ----------
// With steps (halo_steps_begin) — instead of wake-on-motion (it gives no data, so no pedometer either: datasheet
// 6.1) the accelerometer runs at 62.5 Hz with the chip's pedometer (datasheet 11) alone, its interrupt sent to INT2
// (not wired): the steps are counted silently and motion does not wake the board. Its Any-Motion engine (9.1) was
// meant for the wake-up, but with the pedometer on its flag stays set and INT1 flips at the data rate, ~140 edges a
// second, whatever its threshold (2026-11-04, on the board) — the board never slept
static HaloI2c imu;
static bool imu_on, imu_steps;
static uint32_t steps_total, steps_chip, steps_read_ms;
static bool imu_live;  // halo_imu_begin: accel + gyro data instead of wake-on-motion (INT1 off, its interrupt too)
static int imu_mg;     // the wake-on-motion threshold halo_motion_begin set: halo_imu_end goes back to it
static int64_t imu_live_us;
static volatile uint32_t imu_edges;
// The board wakes on motion only after lying still: pressing BOOT or putting it down in a hand moves it too. A burst
// starts with an edge after at least SETTLE_US of quiet; motion wakes only if a burst started after the sleep did
#define SETTLE_US 1500000
static volatile int64_t imu_last_us = -SETTLE_US, imu_burst_us = -SETTLE_US;

static void IRAM_ATTR imu_isr(void *) {
  const int64_t t = esp_timer_get_time();
  if (t - imu_last_us >= SETTLE_US) imu_burst_us = t;
  imu_last_us = t, imu_edges++;
}

static void imu_edge_now() {  // the INT1 flip that woke the board: its interrupt was off in sleep
  portDISABLE_INTERRUPTS();
  imu_isr(nullptr);
  portENABLE_INTERRUPTS();
}

static uint8_t imu_reg(uint8_t r) {
  uint8_t v = 0;
  halo_i2c_read(imu, r, &v, 1);
  return v;
}

static bool imu_wait(uint8_t r, uint8_t mask, uint8_t want, int tries = 100, int step_ms = 5) {
  for (int i = 0; i < tries; i++) {
    if ((imu_reg(r) & mask) == want) return true;
    delay(step_ms);
  }
  return false;
}

// A CTRL9 command with its settings in CAL1..CAL4 (0x08 — wake-on-motion, 0x0D — pedometer, 0x0E — motion): done —
// bit 7 of STATUSINT, then acknowledge. INT1 is off meanwhile (CTRL1), so the handshake does not flip it
static bool imu_cmd(uint8_t cmd, int tries, int step_ms) {
  halo_i2c_write(imu, 0x0A, cmd);
  const bool done = imu_wait(0x2D, 0x80, 0x80, tries, step_ms);
  halo_i2c_write(imu, 0x0A, 0x00);
  imu_wait(0x2D, 0x80, 0x00, tries, step_ms);
  return done;
}
static bool imu_wom_cmd(int tries, int step_ms) {
  return imu_cmd(0x08, tries, step_ms);
}
static bool imu_cal(const uint8_t (&cal)[8], uint8_t cmd, int tries, int step_ms) {  // CAL1_L … CAL4_H, then cmd
  for (int i = 0; i < 8; i++) halo_i2c_write(imu, 0x0B + i, cal[i]);
  return imu_cmd(cmd, tries, step_ms);
}

// Wake-on-motion: after a reset (halo_motion_begin) or from the accel + gyro data (halo_imu_end)
static bool imu_wom(int mg, int tries, int step_ms) {
  halo_i2c_write(imu, 0x02, 0x40);         // CTRL1: address auto-increment, INT pins off
  halo_i2c_write(imu, 0x08, 0x00);         // CTRL7: sensors off
  halo_i2c_write(imu, 0x06, 0x00);         // CTRL5: no filters (as after a reset)
  halo_i2c_write(imu, 0x03, 0x2C);         // CTRL2: ±8 g, low-power 128 Hz
  halo_i2c_write(imu, 0x0B, (uint8_t)mg);  // CAL1_L: threshold, 1 mg per step
  halo_i2c_write(imu, 0x0C, 0x80 | 0x20);  // CAL1_H: INT1 starting high, skip 32 samples
  const bool done = imu_wom_cmd(tries, step_ms);
  halo_i2c_write(imu, 0x08, 0x01);         // CTRL7: accelerometer on
  halo_i2c_write(imu, 0x02, 0x40 | 0x08);  // CTRL1: INT1 output on
  return done;
}

// Steps: out of wake-on-motion (threshold 0), then — the sensors off — the pedometer (the datasheet's example for
// 50 Hz with its windows at 62.5 Hz: a 1 s window, 200 mg peak to peak, 100 mg peak, a step 0.4…4 s, 10 steps in a
// row to start counting — those 10 are counted too; the count is updated every step); the accelerometer on, then the
// pedometer, its interrupt to INT2 (not wired), INT1 off. The chip's count starts from 0 again: what it had is in
// steps_total already
static bool imu_activity(int, int tries, int step_ms) {
  halo_i2c_write(imu, 0x02, 0x40);  // CTRL1: address auto-increment, INT pins off
  halo_i2c_write(imu, 0x08, 0x00);  // CTRL7: sensors off
  halo_i2c_write(imu, 0x09, 0x80);  // CTRL8: CTRL9 handshake by STATUSINT.bit7, no engines yet
  halo_i2c_write(imu, 0x0B, 0x00);  // CAL1_L = 0: out of wake-on-motion
  bool done = imu_wom_cmd(tries, step_ms);
  halo_i2c_write(imu, 0x06, 0x00);  // CTRL5: no filters
  halo_i2c_write(imu, 0x03, 0x27);  // CTRL2: ±8 g, 62.5 Hz
  const uint8_t ped1[8] = {62, 0, 0xCC, 0x00, 0x66, 0x00, 0x02, 0x01};  // window, peak to peak, peak; 1st set
  const uint8_t ped2[8] = {250, 0, 25, 10, 0, 1, 0x02, 0x02};             // longest, shortest, entry, -, every; 2nd
  done &= imu_cal(ped1, 0x0D, tries, step_ms) && imu_cal(ped2, 0x0D, tries, step_ms);
  halo_i2c_write(imu, 0x08, 0x01);         // CTRL7: accelerometer on
  halo_i2c_write(imu, 0x09, 0x80 | 0x10);  // CTRL8: the pedometer, its interrupt to INT2 (not wired)
  steps_chip = 0, steps_read_ms = millis();
  return done;
}

// The motion sensor at rest (no data asked for): wake-on-motion, or with steps — the pedometer and Any-Motion (by a
// pointer halo_steps_begin sets: a firmware that never counts steps has none of this code)
static bool (*imu_steps_setup)(int mg, int tries, int step_ms);
static bool imu_rest(int tries, int step_ms) {
  return imu_steps_setup ? imu_steps_setup(imu_mg, tries, step_ms) : imu_wom(imu_mg, tries, step_ms);
}

static void steps_poll() {  // the chip's count into steps_total (it starts from 0 when set up again)
  uint8_t b[3];
  if (!halo_i2c_read(imu, 0x5A, b, 3)) return;
  const uint32_t c = b[0] | b[1] << 8 | (uint32_t)b[2] << 16;
  steps_total += c >= steps_chip ? c - steps_chip : c;
  steps_chip = c, steps_read_ms = millis();
}

bool halo_steps_begin() {
  if (imu_steps) return true;
  if (!imu_mg) return LOG("steps: halo_motion_begin first\n"), false;
  imu_steps = true, imu_steps_setup = imu_activity;
  if (imu_live) return true;  // halo_imu_end comes back to them
  if (imu_on) gpio_intr_disable((gpio_num_t)HALO_PIN_IMU_INT1);
  const bool done = imu_activity(imu_mg, 100, 5);
  imu_last_us = esp_timer_get_time();
  if (imu_on) gpio_intr_enable((gpio_num_t)HALO_PIN_IMU_INT1);
  LOG("steps: the chip's pedometer (62.5 Hz; motion does not wake the board now)%s\n", done ? "" : ", settings NOT confirmed");
  halo_command("imureg", [](const char *a) {  // (debug) imureg R — read, imureg R V — write; hex
    unsigned r = 0, v = 0;
    const int n = sscanf(a, "%x %x", &r, &v);
    if (n == 2) halo_i2c_write(imu, (uint8_t)r, (uint8_t)v);
    if (n >= 1) LOG("imureg 0x%02x = 0x%02x\n", r, imu_reg((uint8_t)r));
  });
  halo_command("steps", [](const char *) {
    LOG("steps: %lu (the chip's count %lu), INT1 edges %lu, INT1 now %d\n", (unsigned long)halo_steps(),
        (unsigned long)steps_chip, (unsigned long)imu_edges, gpio_get_level((gpio_num_t)HALO_PIN_IMU_INT1));
  });
  return done;
}

uint32_t halo_steps() {
  if (imu_steps && imu && !imu_live && millis() - steps_read_ms >= 500) steps_poll();
  return steps_total;
}

bool halo_motion_begin(int mg) {
  if (imu_live) halo_imu_end();
  if (!imu) imu = halo_i2c(0x6B);
  const uint8_t who = imu ? imu_reg(0x00) : 0;
  if (who != 0x05) return LOG("motion: QMI8658 not found (who am i 0x%02x)\n", who), false;
  halo_i2c_write(imu, 0x60, 0xB0);  // reset; done when 0x4D reads 0x80
  if (!imu_wait(0x4D, 0xFF, 0x80)) return LOG("motion: QMI8658 reset failed\n"), false;
  imu_mg = constrain(mg, 1, 255);
  const bool done = imu_rest(100, 5);
  if (!imu_on) {
    gpio_config_t io = {};
    io.pin_bit_mask = 1ull << HALO_PIN_IMU_INT1;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.intr_type = GPIO_INTR_ANYEDGE;
    gpio_config(&io);
    gpio_isr_handler_add((gpio_num_t)HALO_PIN_IMU_INT1, imu_isr, nullptr);
    imu_on = true;
  }
  LOG("motion: wake-on-motion %d mg (QMI8658 INT1, GPIO21)%s\n", mg, done ? "" : ", settings NOT confirmed");
  return done;
}

uint32_t halo_motion_count() {
  return imu_edges;
}

// ---------- Motion data: accelerometer + gyroscope for a screen that needs them ----------
// Out of wake-on-motion (datasheet 12.6): sensors off, threshold 0 and the same CTRL9 command; then the configuration
// AmoledImu used on this board (±8 g; ±512 °/s, gyro low-pass 13.37 % of ODR; accel without a filter), at 224.2 Hz
// (both sensors on: the gyroscope's rate). INT1 stays off (high-Z, pulled up), its interrupt too: no edges are
// counted and motion does not wake the board. Fast waits: begin/end run from a screen's update
bool halo_imu_begin() {
  if (imu_live) return true;
  if (!imu) imu = halo_i2c(0x6B);
  const uint8_t who = imu ? imu_reg(0x00) : 0;
  if (who != 0x05) return LOG("imu: QMI8658 not found (who am i 0x%02x)\n", who), false;
  if (imu_on) gpio_intr_disable((gpio_num_t)HALO_PIN_IMU_INT1);
  if (imu_steps) steps_poll();      // the steps so far (the pedometer stops while the data run)
  halo_i2c_write(imu, 0x08, 0x00);  // CTRL7: sensors off
  halo_i2c_write(imu, 0x02, 0x40);  // CTRL1: address auto-increment, little-endian, INT pins off
  halo_i2c_write(imu, 0x09, 0x00);  // CTRL8: no motion engines (tap, pedometer — a previous firmware may have left them)
  halo_i2c_write(imu, 0x0B, 0x00);  // CAL1_L = 0: wake-on-motion off (also when it was not on: harmless)
  const bool done = imu_wom_cmd(20, 1);
  halo_i2c_write(imu, 0x03, 0x25);  // CTRL2: ±8 g, 250 Hz (224.2 with the gyroscope)
  halo_i2c_write(imu, 0x04, 0x55);  // CTRL3: ±512 °/s, 224.2 Hz
  halo_i2c_write(imu, 0x06, 0x70);  // CTRL5: gyro low-pass on, mode 3 (13.37 % of ODR ≈ 30 Hz); accel — none
  halo_i2c_write(imu, 0x08, 0x03);  // CTRL7: accelerometer + gyroscope
  imu_live = true, imu_live_us = esp_timer_get_time();
  LOG("imu: accel + gyro on (224 Hz)%s\n", done ? "" : ", wake-on-motion exit NOT confirmed");
  return true;
}

bool halo_imu_read(HaloImu &out) {
  // The gyroscope turns on in 150 ms + 3 samples (datasheet, table 8): until then its registers are not data
  if (!imu_live || esp_timer_get_time() - imu_live_us < 170000) return false;
  uint8_t b[14];
  if (!halo_i2c_read(imu, 0x33, b, sizeof(b))) return false;  // TEMP_L, TEMP_H, AX_L … GZ_H, little-endian
  out.temp = (int8_t)b[1] + b[0] * (1.0f / 256);                // TEMP_H — целые °C, TEMP_L — доли
  int16_t v[6];
  for (int i = 0; i < 6; i++) v[i] = (int16_t)(b[2 + 2 * i] | b[3 + 2 * i] << 8);
  out.ax = v[0] * (1.0f / 4096), out.ay = v[1] * (1.0f / 4096), out.az = v[2] * (1.0f / 4096);  // 8 g / 32768
  out.gx = v[3] * (1.0f / 64), out.gy = v[4] * (1.0f / 64), out.gz = v[5] * (1.0f / 64);  // 512 °/s / 32768
  return true;
}

void halo_imu_end() {
  if (!imu_live) return;
  imu_live = false;
  if (!imu_mg) {  // halo_motion_begin was not called: just sensors off
    halo_i2c_write(imu, 0x08, 0x00);
    LOG("imu: off\n");
    return;
  }
  const bool done = imu_rest(20, 1);
  // INT1 starts high again, as the pull-up kept it: no edge. The board was in a hand until now — as if it moved
  // (motion wakes only after 1.5 s of quiet)
  imu_last_us = esp_timer_get_time();
  if (imu_on) gpio_intr_enable((gpio_num_t)HALO_PIN_IMU_INT1);
  LOG("imu: back to %s %d mg%s\n", imu_steps ? "the pedometer" : "wake-on-motion", imu_mg,
      done ? "" : ", settings NOT confirmed");
}

// ---------- Sleep ----------
static volatile bool req, wake_req;
static volatile uint32_t req_ms;
static volatile int holds;
static volatile bool on_usb_ok;
static bool asleep, boot_armed, touch_armed, told_usb;
static uint32_t sleep_ms;
static int64_t asleep_at_us;
static uint8_t woke;
static bool clock_ready;
static uint32_t cal_at_ms;

void halo_sleep(uint32_t ms) {
  req_ms = ms, req = true;
}

uint8_t halo_woke() {
  return woke;
}

void halo_wake() {
  wake_req = true;
}

bool halo_asleep() {
  return asleep;
}

void halo_sleep_on_usb(bool on) {
  on_usb_ok = on;
}

void halo_stay_awake(bool hold) {
  __atomic_add_fetch(&holds, hold ? 1 : -1, __ATOMIC_RELAXED);
}

static const char *wake_name(uint8_t w) {
  return w == HALO_WAKE_TOUCH ? "touch" : w == HALO_WAKE_BOOT ? "BOOT" : w == HALO_WAKE_MOTION ? "motion" : "timer";
}

// The slow clock keeps time while asleep: the fast RC oscillator / 256 instead of the slow RC one, re-measured
// against the 40 MHz crystal every 5 minutes (1024 cycles — 15 ms)
static void clock_prepare() {
  if (!clock_ready) {
    rtc_clk_8m_enable(true, true);
    rtc_clk_slow_src_set(SOC_RTC_SLOW_CLK_SRC_RC_FAST_D256);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RC_FAST, ESP_PD_OPTION_ON);
    clock_ready = true, cal_at_ms = 0;
  }
  if (!cal_at_ms || millis() - cal_at_ms > 300000) {
    esp_clk_slowclk_cal_set(rtc_clk_cal(RTC_CAL_RTC_MUX, 1024));
    cal_at_ms = millis() | 1;
  }
}

static bool finger_down() {
  HaloTouchSample s;
  return halo_touch_samples(&s, 1) > 0;
}

static bool motion_new() {  // the board was lying still, and then it moved — after it was put to sleep
  return imu_on && !imu_live && imu_burst_us > asleep_at_us;
}

// One light sleep; motion — whether INT1 may wake it. The log line is the caller's: some wake-ups are internal
static uint8_t sleep_now(uint32_t ms, bool motion) {
  clock_prepare();
  ht_bus_lock();  // the touch task is not in the middle of an I2C transfer
  const gpio_num_t tp = (gpio_num_t)HALO_PIN_TP_INT, boot = (gpio_num_t)HALO_PIN_BOOT,
                   mo = (gpio_num_t)HALO_PIN_IMU_INT1;
  // The pins' own interrupts are off for the sleep. Their wake-up is by level, and so would be their interrupt: a
  // level that stays (INT1 after one motion event, when the board is then held still) fired it again and again —
  // core 1 frozen, the interrupt watchdog rebooted the board, and the level setting, which survives the reboot,
  // crashed the next start too: safe mode, a black screen (experiments/SleepLab: storm 0). The wake-up itself does
  // not need the interrupt
  gpio_intr_disable(tp);
  if (motion) gpio_intr_disable(mo);
  gpio_wakeup_enable(tp, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable(boot, GPIO_INTR_LOW_LEVEL);
  const int mo_was = motion ? gpio_get_level(mo) : 0;
  if (motion) gpio_wakeup_enable(mo, mo_was ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL);
  esp_sleep_enable_gpio_wakeup();
  if (ms) esp_sleep_enable_timer_wakeup(ms * 1000ull);
  const uint32_t e0 = imu_edges;
  esp_light_sleep_start();
  // Who woke us — at once (a touch pulse is short; INT1 may be back where it was, its interrupt has counted it)
  const int lb = gpio_get_level(boot), lm = motion ? gpio_get_level(mo) : mo_was;
  const esp_sleep_wakeup_cause_t c = esp_sleep_get_wakeup_cause();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  // gpio_wakeup_disable also switches the pin interrupt off: give the touch and the sensor back their edges
  gpio_wakeup_disable(tp), gpio_set_intr_type(tp, GPIO_INTR_NEGEDGE), gpio_intr_enable(tp);
  gpio_wakeup_disable(boot);
  if (motion) gpio_wakeup_disable(mo), gpio_set_intr_type(mo, GPIO_INTR_ANYEDGE), gpio_intr_enable(mo);
  ht_bus_unlock();
  uint8_t w = c == ESP_SLEEP_WAKEUP_TIMER                      ? HALO_WAKE_TIMER
              : !lb                                            ? HALO_WAKE_BOOT
              : motion && (lm != mo_was || imu_edges != e0) ? HALO_WAKE_MOTION
                                                               : HALO_WAKE_TOUCH;
  if (w == HALO_WAKE_MOTION && imu_edges == e0) imu_edge_now();
  if (w != HALO_WAKE_TIMER) ht_kick();  // the report whose pulse woke the board: its interrupt was off
  // A finger moves the board too: give the touch task a moment to read a report — a finger there means a touch
  if (w == HALO_WAKE_MOTION) {
    vTaskDelay(pdMS_TO_TICKS(4));
    if (finger_down()) w = HALO_WAKE_TOUCH;
  }
  return w;
}

// Asleep for real until a wake-up the sketch should see. Motion that is not new (the board still moving since it
// was put to sleep) — 300 ms deaf to motion, then listen again: ~3 short wake-ups a second while it is carried
static uint8_t sleep_real() {
  static int64_t deaf_until;
  int quiet = 0;
  for (;;) {
    const int64_t now = esp_timer_get_time(), gone_ms = (now - asleep_at_us) / 1000;
    if (sleep_ms && gone_ms >= sleep_ms) return HALO_WAKE_TIMER;
    uint32_t ms = sleep_ms ? (uint32_t)(sleep_ms - gone_ms) : 0;
    const bool deaf = now < deaf_until;
    if (deaf) {
      const uint32_t d = (uint32_t)((deaf_until - now) / 1000) + 1;
      ms = ms ? min(ms, d) : d;
    }
    const uint8_t w = sleep_now(ms, imu_on && !imu_live && !deaf);  // accel + gyro data: INT1 is off
    if (w == HALO_WAKE_TIMER && !(sleep_ms && (esp_timer_get_time() - asleep_at_us) / 1000 >= sleep_ms)) {
      quiet++;
      continue;  // the end of a deaf spell, not the sketch's timer
    }
    if (w == HALO_WAKE_MOTION && !motion_new()) {
      deaf_until = esp_timer_get_time() + 300000, quiet++;
      continue;
    }
    if (quiet) LOG("sleep: %d wake-ups on motion that was not new (still moving since sleep)\n", quiet);
    return w;
  }
}

void hp_frame_end() {
  woke = 0;
  if (wake_req) wake_req = req = asleep = false;  // the sketch woke up by itself: no sleep, asked or going on
  if (req) {
    req = false;
    if (!asleep) asleep_at_us = esp_timer_get_time(), boot_armed = touch_armed = false;
    sleep_ms = req_ms, asleep = true;
  }
  if (!asleep) return;
  const int64_t gone_ms = (esp_timer_get_time() - asleep_at_us) / 1000;
  // BOOT and a touch count once released: the press or tap that put the board to sleep is not a wake-up
  const bool boot_up = gpio_get_level((gpio_num_t)HALO_PIN_BOOT), finger = finger_down();
  if (boot_up) boot_armed = true;
  if (!finger) touch_armed = true;
  uint8_t w = 0;
  if (sleep_ms && gone_ms >= sleep_ms) w = HALO_WAKE_TIMER;
  else if (boot_armed && !boot_up) w = HALO_WAKE_BOOT;
  else if (touch_armed && finger) w = HALO_WAKE_TOUCH;
  else if (motion_new()) w = HALO_WAKE_MOTION;
  if (w) {
    LOG("sleep: %.1f s up (USB or held), woke by %s\n", gone_ms / 1000.0f, wake_name(w));
  } else {
    const HaloBattery b = halo_battery();
    const bool usb = !b.ok || b.usb;  // no data yet — as on USB: stay up
    // Really asleep: nobody holds the board up, it is not on USB (or allowed), BOOT and the finger are up
    if (!holds && (!usb || on_usb_ok) && boot_up && !finger) {
      w = sleep_real();
      LOG("sleep: %.1f s, woke by %s\n", (esp_timer_get_time() - asleep_at_us) / 1e6f, wake_name(w));
    } else if (usb && !on_usb_ok && !told_usb) {
      told_usb = true;
      LOG("sleep: on USB power — staying up, waking the same way (halo_sleep_on_usb to really sleep)\n");
    }
  }
  if (!w) return;
  asleep = false, woke = w;
  if (w == HALO_WAKE_TOUCH) hu_swallow_touch();
  if (w == HALO_WAKE_BOOT) hf_swallow_boot();
}

#endif  // CONFIG_IDF_TARGET_ESP32S3
