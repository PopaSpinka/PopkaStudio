// The clock chip of the ESP32-S3-Touch-AMOLED-1.75 / -G and of the 2.06: PCF85063 (I2C 0x51, powered with the RTC rail of the AXP2101,
// so it keeps time on the battery while the board is off). The round 1.75C has no such chip: its time is lost with the
// power (boards/amoled175/hardware.md). Only these boards build this file.
//
// At the start the system clock takes the chip's time (UTC), when the chip's oscillator never stopped since it was set.
// Afterwards the chip follows the system clock: whenever that jumps — the computer's "time" after a Load, Wi-Fi's NTP —
// the chip is written in the middle of the next second (STOP, the seven registers, STOP off: its divider starts the
// second from there). The command rtc prints both.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32S3 && (defined(HALO_BOARD_AMOLED175G) || defined(HALO_BOARD_AMOLED206))
#include "halo_s3.h"

#include <sys/time.h>

#define RTC_VALID 1735689600  // 2025-01-01: an earlier system time is "not set" (the firmware's "time" takes no less)

static HaloI2c rtc;
static int64_t base_us;     // the system clock and the monotonic one, read together: a jump is a change between them
static int64_t base_sys_us;
static bool write_due;
static uint32_t tick_ms;

static uint8_t bcd(uint8_t v) { return (v >> 4) * 10 + (v & 15); }
static uint8_t tobcd(int v) { return (uint8_t)((v / 10) << 4 | v % 10); }

// Days since 1970-01-01 of a civil date and back (H. Hinnant's algorithms): no timegm in newlib
static int64_t days_from(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400, yoe = y - era * 400;
  const int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return (int64_t)era * 146097 + doe - 719468;
}
static void civil_from(int64_t z, int *y, int *m, int *d) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const int doe = (int)(z - era * 146097), yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int doy = doe - (365 * yoe + yoe / 4 - yoe / 100), mp = (5 * doy + 2) / 153;
  *d = doy - (153 * mp + 2) / 5 + 1, *m = mp + (mp < 10 ? 3 : -9), *y = (int)(yoe + era * 400) + (*m <= 2);
}

static int64_t sys_us() {
  timeval tv;
  gettimeofday(&tv, nullptr);
  return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static void rebase() {
  base_us = esp_timer_get_time(), base_sys_us = sys_us();
}

// The chip's time, UTC seconds; 0 — no answer, or its oscillator stopped since it was set (OS, bit 7 of Seconds)
static time_t rtc_read(bool *stopped = nullptr) {
  uint8_t r[7];
  if (!rtc || !halo_i2c_read(rtc, 0x04, r, 7)) return 0;
  if (stopped) *stopped = r[0] & 0x80;
  if (r[0] & 0x80) return 0;
  const int64_t days = days_from(2000 + bcd(r[6]), bcd(r[5] & 0x1F), bcd(r[3] & 0x3F));
  return (time_t)(days * 86400 + bcd(r[2] & 0x3F) * 3600 + bcd(r[1] & 0x7F) * 60 + bcd(r[0] & 0x7F));
}

static bool rtc_write(time_t t) {
  const int64_t days = t / 86400;
  const int s = t % 86400;
  int y, m, d;
  civil_from(days, &y, &m, &d);
  if (y < 2000 || y > 2099) return false;
  bool ok = halo_i2c_write(rtc, 0x00, 0x20);  // Control_1: STOP — the divider waits
  const uint8_t v[7] = {tobcd(s % 60), tobcd(s / 60 % 60), tobcd(s / 3600), tobcd(d), (uint8_t)((days + 4) % 7),
                        tobcd(m), tobcd(y % 100)};  // (writing Seconds clears OS; 1970-01-01 was a Thursday: 4)
  for (int i = 0; i < 7; i++) ok &= halo_i2c_write(rtc, 0x04 + i, v[i]);
  ok &= halo_i2c_write(rtc, 0x00, 0x00);  // Control_1: running, 24 hours, its capacitor as Waveshare's (7 pF)
  return ok;
}

static void rtc_print() {
  bool stopped = false;
  const time_t c = rtc_read(&stopped), s = time(nullptr);
  if (!rtc) return (void)LOG("rtc: PCF85063 not found\n");
  if (!c) return (void)LOG("rtc: %s; system clock %s\n", stopped ? "the chip's time is not set (it stopped since)" : "no answer",
                           s >= RTC_VALID ? "set" : "not set");
  LOG("rtc: chip %lld, system %lld (%+lld s)\n", (long long)c, (long long)s, (long long)(s - c));
}

void hr_begin() {
  rtc = halo_i2c(0x51);
  bool stopped = false;
  const time_t c = rtc_read(&stopped);
  if (c >= RTC_VALID && time(nullptr) < RTC_VALID) {
    const timeval tv = {c, 0};
    settimeofday(&tv, nullptr);
    LOG("rtc: the time from the clock chip\n");
  } else {
    LOG("rtc: PCF85063 %s\n", !rtc ? "NOT FOUND" : stopped ? "has no time yet (it is set with the first time the board gets)" : "ok");
  }
  rebase();
  halo_command("rtc", [](const char *) { rtc_print(); });
}

void hr_tick() {
  if (!rtc) return;
  // Once a second: has the system clock jumped (> 0.5 s off the monotonic one since the last look)?
  if (millis() - tick_ms >= 1000) {
    tick_ms = millis();
    const int64_t now = esp_timer_get_time(), off = sys_us() - (base_sys_us + (now - base_us));
    if (off > 500000 || off < -500000) write_due = true;
    rebase();
  }
  // Written 0.492 s into a second: its first second after STOP is let go lasts 0.508 s (datasheet 8.2.2), so it then
  // counts on with the system clock's whole seconds (written at a whole second, it ran half a second ahead — on the
  // board). halo_loop comes every ~5 ms
  if (write_due && time(nullptr) >= RTC_VALID) {
    const int64_t s = sys_us(), us = s % 1000000;
    if (us >= 492000 && us < 522000) {
      write_due = false;
      const bool ok = rtc_write((time_t)(s / 1000000));
      rebase();
      LOG("rtc: %s\n", ok ? "the clock chip follows the new time" : "writing the clock chip FAILED");
    }
  }
}

#endif  // CONFIG_IDF_TARGET_ESP32S3 && (HALO_BOARD_AMOLED175G || HALO_BOARD_AMOLED206)
