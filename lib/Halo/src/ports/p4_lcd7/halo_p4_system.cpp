// The P4 board: what is not done yet or does not exist here. A part of the contract without hardware answers "no", the way
// the browser port does (PopkaStudio/engine/halo_web.cpp), so interfaces simply do not offer it.
//
// Not on this board at all: the PWR button and fuel gauge (no AXP2101), the motion sensor. Sound and microphones:
// halo_p4_audio.cpp.
//
// Sleep: the processor stays up. The touch chip's interrupt is not wired, so a sleeping board could not feel a finger, and
// the panel has no memory of its own: the DSI controller has to keep sending it the picture. What the interface calls sleep
// is the backlight (power.h dims it or turns it off); frames go on, and the wake-ups are watched once a frame — the path
// the S3 port takes on USB power.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"

#include <driver/gpio.h>
#include <esp_timer.h>

uint8_t halo_pwr_event() {
  return 0;
}

HaloBattery halo_battery() {
  HaloBattery b = {};
  b.percent = -1;
  return b;
}

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

#endif  // CONFIG_IDF_TARGET_ESP32P4
