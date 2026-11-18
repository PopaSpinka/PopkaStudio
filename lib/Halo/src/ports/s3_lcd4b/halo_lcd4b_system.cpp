// The ESP32-S3-Touch-LCD-4B: sleep, and what is not done yet. A part of the contract without a driver answers "no", the way
// the browser port does (studio/engine/halo_web.cpp), so interfaces simply do not offer it.
//
// Sleep: the processor stays up. The panel has no memory of its own — the LCD has to keep sending it the picture — and the
// touch chip's interrupt reaches only the expander. What the interface calls sleep is the backlight (power.h dims it or
// turns it off); frames go on, and the wake-ups (BOOT, a touch, the timer) are watched once a frame — the path the round
// watch takes on USB power.
//
// Not done yet (the board has them): the QMI8658 motion sensor, the PCF85063 clock. Sound: halo_lcd4b_audio.cpp.
#include <sdkconfig.h>
#if defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_lcd4b.h"

#include <driver/gpio.h>
#include <esp_timer.h>

static volatile bool sleep_req, wake_req;
static volatile uint32_t sleep_req_ms;
static bool asleep, boot_armed, touch_armed;
static uint32_t sleep_ms;
static int64_t asleep_at_us;
static uint8_t woke;

void halo_sleep(uint32_t ms) {
  sleep_req_ms = ms, sleep_req = true;
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
void halo_sleep_on_usb(bool) {}
void halo_stay_awake(bool) {}

// Once a frame, after it is on the screen
void hp_frame_end() {
  woke = 0;
  if (wake_req) wake_req = sleep_req = asleep = false;  // the interface woke up by itself: no sleep, asked or going on
  if (sleep_req) {
    sleep_req = false;
    if (!asleep) asleep_at_us = esp_timer_get_time(), boot_armed = touch_armed = false;
    sleep_ms = sleep_req_ms, asleep = true;
  }
  if (!asleep) return;
  const int64_t gone_ms = (esp_timer_get_time() - asleep_at_us) / 1000;
  // BOOT and a touch count once released: the press or tap that put the board to sleep is not a wake-up
  HaloTouchSample s;
  const bool boot_up = gpio_get_level((gpio_num_t)HALO_PIN_BOOT), finger = halo_touch_samples(&s, 1) > 0;
  if (boot_up) boot_armed = true;
  if (!finger) touch_armed = true;
  uint8_t w = 0;
  if (sleep_ms && gone_ms >= sleep_ms) w = HALO_WAKE_TIMER;
  else if (boot_armed && !boot_up) w = HALO_WAKE_BOOT;
  else if (touch_armed && finger) w = HALO_WAKE_TOUCH;
  if (!w) return;
  asleep = false, woke = w;
  if (w == HALO_WAKE_TOUCH) hu_swallow_touch();
  if (w == HALO_WAKE_BOOT) hf_swallow_boot();
}

// ---------- Motion: not yet ----------
bool halo_motion_begin(int) {
  return false;
}
bool halo_steps_begin() {
  return false;
}
uint32_t halo_steps() {
  return 0;
}
uint32_t halo_motion_count() {
  return 0;
}
bool halo_imu_begin() {
  return false;
}
bool halo_imu_read(HaloImu &) {
  return false;
}
void halo_imu_end() {}

#endif  // HALO_BOARD_LCD4B
