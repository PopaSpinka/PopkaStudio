// The WT32-SC01 Plus: what does not exist here. A part of the contract without hardware answers "no", the way the
// browser port does (studio/engine/halo_web.cpp), so interfaces simply do not offer it.
//
// Not on this board at all: a battery and its power chip (USB or 5 V only), the PWR and BOOT buttons (GPIO0 is the
// panel's command line), a motion sensor, microphones.
// Not done yet: sound — the board has an I2S amplifier without a codec; halo_audio_begin answers "no" for now.
//
// Sleep: the processor stays up — the board is always on external power, and a sleeping one could not be loaded over
// USB. What the interface calls sleep is the backlight (power.h dims it or turns it off); frames go on, and the wake-ups
// (a touch, the timer) are watched once a frame — the path the round watch takes on USB power.
#include <sdkconfig.h>
#if defined(HALO_BOARD_WT32SC01P)  // this board's port: other boards build their own
#include "halo_wt32.h"

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
static bool asleep, touch_armed;
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
    if (!asleep) asleep_at_us = esp_timer_get_time(), touch_armed = false;
    sleep_ms = sleep_req_ms, asleep = true;
  }
  if (!asleep) return;
  const int64_t gone_ms = (esp_timer_get_time() - asleep_at_us) / 1000;
  // A touch counts once released: the tap that put the board to sleep is not a wake-up
  HaloTouchSample s;
  const bool finger = halo_touch_samples(&s, 1) > 0;
  if (!finger) touch_armed = true;
  uint8_t w = 0;
  if (sleep_ms && gone_ms >= sleep_ms) w = HALO_WAKE_TIMER;
  else if (touch_armed && finger) w = HALO_WAKE_TOUCH;
  if (!w) return;
  asleep = false, woke = w;
  if (w == HALO_WAKE_TOUCH) hu_swallow_touch();
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

// ---------- Sound: not yet ----------
static int volume = 60;

bool halo_audio_begin(HaloAudioFill, int) {
  return false;
}
void halo_audio_volume(int v) {
  volume = constrain(v, 0, 100);
}
int halo_audio_get_volume() {
  return volume;
}
int halo_mic_begin(int) {
  return 0;
}
int halo_mic_read(int16_t *, int) {
  return 0;
}
float halo_mic_db() {
  return -90;
}
void halo_mic_db2(float &mic1, float &mic2) {
  mic1 = mic2 = -90;
}
void halo_mic_end() {}

#endif  // HALO_BOARD_WT32SC01P
