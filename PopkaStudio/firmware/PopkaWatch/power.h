// power.h — PopkaStudio's Functions: Always On when idle, and what the BOOT and PWR buttons do. Always On and sleep
// are projects/HaloWatch's power.h one to one (lib/Halo: halo_nav_ambient, halo_sleep; docs/halo.md, section 12); the
// lock is new: on HaloWatch it dimmed the screen and turned the touch off where it was, here it is Always On itself.
//
// Always On (FN_AOD): FN_AOD_IDLE s without touches and motion — the interface glides into Always On in one move, one
// spring (pw_amb: 0 — awake, 1 — Always On). Along it the panel dims from the awake level (pw_awake_level: Display's
// slider, settings.h; else PW_AWAKE) down to FN_AOD_BRIGHT (evenly for
// the eye) and the whole interface shrinks by 5 % around the screen center; on the face the face itself goes there,
// from any other screen it is fade through (halo_nav_ambient). Then the board sleeps and wakes once a minute for one
// frame: new digits, set without rolling, and the face a few pixels elsewhere (AMOLED burn-in: 8 places on a 7 px
// circle). A touch, a button or picking the board up — the same move back, quicker; the waking touch presses nothing.
// Off — the screen stays on. The face's animations (the stars, the pixel art: fc_calm) stop the moment the move into
// Always On starts and go on the moment the move back starts — the same both ways. Back to watch face (FN_AOD_HOME):
// once there (only the face is seen), everything over the face is closed (halo_nav_home_now) — the watch wakes on the
// face, the face itself coming out of Always On; off — on the screen where it was.
//
// The buttons (FN_BOOT, FN_PWR): FN_SCREEN_OFF — the same move to fully dark, the panel off; a touch, a button or
// picking it up — back. FN_AMBIENT — into Always On at once, as when idle (also with Always On off): the same button
// again, a touch or picking it up — back. FN_LOCK — into Always On with the touch off (a touch only wiggles the lock at the top); a long
// press of that same button — back, unlocked. Locked and asleep, a wake-up (a touch, motion, the minute) keeps the
// lock: the board stays up PW_LISTEN_S listening for the long press, then sleeps again. PWR can't wake the board (its
// chip's interrupt is not wired: docs/hardware.md) — on battery its long press is read at the next wake-up (pick the
// watch up). On USB the board does not really sleep (Halo) — the same screens, the frames just go on.
//
// Waking up (FN_WAKE_TAP, FN_WAKE_RAISE, FN_WAKE_BTN — the editor's Always On → Wake up): what brings the screen back
// from Always On or dark — a touch, picking the watch up (the motion sensor; off — the sensor is not even started,
// unless the Steps layer needs it — then the pedometer has it and motion never wakes the board), a button (BOOT or PWR:
// in Always On and dark any press only wakes, its action is for an awake watch). A way that is off is not a way: the
// board, woken by it, goes straight back to sleep; motion with Raise off is not activity either. At least one is on
// (the editor and studio.py see to it). Locked — only the long press of the lock's button, as before. The command
// wake — always.
//
// The editor writes the settings into layers.h; its page changes them on the fly (engine/studio_api.h: studio_fn).
// Commands: aod on|off, aod N (the panel in Always On, of 255), aod home on|off (Back to watch face), idle N (s), sleep
// (Always On now), wake (as a touch);
// lib/Halo's boot / boot long, pwr / pwr long — the buttons.
// Included from PopkaWatch.ino after face.h.
#include <sys/time.h>

#ifndef FN_AOD  // layers.h has them when the editor wrote it
#define FN_AOD 1
#define FN_AOD_IDLE 15
#define FN_AOD_BRIGHT 24
#define FN_BOOT 1
#define FN_PWR 2
#endif
#ifndef FN_AOD_HOME
#define FN_AOD_HOME 0  // out of Always On on the face (1) or where it was (0)
#endif
#ifndef FN_WAKE_TAP
#define FN_WAKE_TAP 1
#define FN_WAKE_RAISE 1
#define FN_WAKE_BTN 1
#endif
enum { FN_NOTHING, FN_SCREEN_OFF, FN_LOCK, FN_AMBIENT };  // what a button does
#define PW_AWAKE 180        // the panel awake: HaloWatch's default brightness
#define PW_AOD_SCALE 0.95f  // Always On and dark: everything 5 % smaller
#define PW_LISTEN_S 3       // locked: awake this long after a wake-up, for the button's long press
#define PW_LOCK_Y 15.0f     // the lock's middle: where the Weather screen and the gallery show the time at the top

enum { PW_ACTIVE, PW_FADING, PW_AOD, PW_DARK };
static int pw_mode = PW_ACTIVE;
static bool pw_to_aod;        // where the fade goes: Always On or dark
static bool pw_aod_way;       // the way the spring left zero: to Always On (the brightness's curve, the face at the end)
static HaloSpring pw_amb;     // 0 — awake, 1 — Always On or dark
static HaloSpringK K_PW_OUT, K_PW_IN, K_PW_LOCK;
static int64_t pw_active_us;  // the last touch or motion
static int64_t pw_listen_us;  // locked: stay up until then
static uint32_t pw_motion;
static int pw_sent = -1;      // the brightness last sent to the panel
static HaloView pw_view;      // the whole interface's view on the way to Always On
static bool pw_locked;
static int pw_lock_btn;       // the button that locked: 1 — BOOT, 2 — PWR (its long press unlocks)
static uint32_t pw_lock_ms;   // locked then: a long press of that same press does not unlock
static HaloSpring pw_lock;    // 0 — unlocked, 1 — locked: the lock at the top fades and grows in
static uint32_t pw_shake_at;  // touched while locked then: the lock wiggles
static float pw_mid_lock;
static HaloItem pw_lock_ic;
static volatile bool pw_cmd_sleep, pw_cmd_wake;
static volatile int pw_aod_on = FN_AOD, pw_idle_s = FN_AOD_IDLE, pw_aod_level = FN_AOD_BRIGHT, pw_aod_home = FN_AOD_HOME;
static volatile int pw_boot = FN_BOOT, pw_pwr = FN_PWR;
static int pw_boot_fn() {  // (settings.h: Display's hint)
  return pw_boot;
}
static volatile int pw_wake_tap = FN_WAKE_TAP, pw_wake_raise = FN_WAKE_RAISE, pw_wake_btn = FN_WAKE_BTN;
#define PW_WAKE_CMD 5  // woken by the command wake (the editor entering Try): always

// Does this wake-up (HALO_WAKE_*) bring the screen back — is its way on
static bool pw_wakes(uint8_t w) {
  return w == HALO_WAKE_TOUCH ? pw_wake_tap : w == HALO_WAKE_MOTION ? pw_wake_raise : w == HALO_WAKE_BOOT ? pw_wake_btn : true;
}

static uint32_t pw_ms_to_minute() {  // until the next minute, +30 ms to be sure it has turned
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return 60000 - (uint32_t)((tv.tv_sec % 60) * 1000 + tv.tv_usec / 1000) + 30;
}

// The panel's brightness: awake — PW_AWAKE; to Always On — down to its level, every frame the same share less (the eye
// sees brightness about as a logarithm); to dark — to zero along a square (the fade's end is the slow part)
static int pw_level(float awake) {
  const float k = pw_amb.x;
  if (k <= 0 || awake < 1) return (int)lroundf(awake);
  if (!pw_aod_way) return (int)lroundf(awake * (1 - k) * (1 - k));
  const float lo = fminf(awake, (float)pw_aod_level);
  return (int)lroundf(awake * powf(lo / awake, k));
}

static void pw_fade(bool to_aod) {
  if (pw_amb.x == 0) pw_aod_way = to_aod;
  pw_mode = PW_FADING, pw_to_aod = to_aod, pw_amb.target = 1;
}

static void pw_wake(uint8_t w) {  // the screen where it was: back along the same spring
  static const char *const NAMES[] = {"button", "touch", "BOOT", "motion", "timer", "command"};
  LOG("power: woke by %s\n", NAMES[w < 6 ? w : 0]);
  pw_mode = PW_ACTIVE, pw_amb.target = 0, pw_active_us = esp_timer_get_time();
  halo_wake();  // woken by a button the chip reads or a command — lib/Halo would still take the next touch for a wake-up
  face_still(false);
  if (!halo_display_is_on()) halo_display_on(true);  // dark: the panel comes on at zero brightness
}

static void pw_lock_set(bool on, int btn) {
  pw_locked = on, pw_lock_btn = btn, pw_lock.target = on, pw_lock_ms = halo_ms();
  halo_touch_on(!on);
  LOG("%s\n", on ? "locked" : "unlocked");
}

// A button pressed (1 — BOOT, 2 — PWR; held — its long press) with what it does
static void pw_button(int btn, int fn, bool held, int64_t now) {
  if (pw_locked) {  // only the long press of the button that locked (a new press, not the one that locked): back
    if (held && btn == pw_lock_btn && halo_ms() - pw_lock_ms > HALO_BOOT_LONG_MS + 300) pw_lock_set(false, 0), pw_wake(0);
    else if (!held) pw_shake_at = halo_ms() | 1, pw_listen_us = now + PW_LISTEN_S * 1000000ll;
    return;
  }
  if (held) return;
  if (pw_mode == PW_AOD || pw_mode == PW_DARK) {  // asleep: a press only wakes — if buttons do (Wake up)
    if (pw_wake_btn) pw_wake(0);
    return;
  }
  if (fn == FN_LOCK) {  // into Always On, locked — from anywhere (from dark too: the panel on, straight to its level)
    pw_lock_set(true, btn);
    if (!halo_display_is_on()) halo_display_on(true);
    if (pw_mode != PW_AOD) pw_aod_way = true, pw_fade(true);
  } else if (fn == FN_SCREEN_OFF) {
    if (pw_mode == PW_ACTIVE || (pw_mode == PW_FADING && pw_to_aod)) pw_fade(false);  // to dark (from anywhere on the way)
    else pw_wake(0);  // dark, Always On, or fading to dark — back
  } else if (fn == FN_AMBIENT) {
    if (pw_mode == PW_ACTIVE && (pw_amb.x == 0 || pw_aod_way)) pw_fade(true);  // into Always On, as when idle
    else pw_wake(0);  // Always On, dark, or on the way there — back
  }
}

// Every frame, first (before the face: Always On wants its digits without rolling)
static void power_tick(float dt) {
  const HaloFinger &f = halo_finger;
  const int64_t now = esp_timer_get_time();
  const bool moved = pw_wake_raise && halo_motion_count() != pw_motion;  // (Raise off: motion is nothing)
  if (f.down || moved || settings_busy()) pw_active_us = now;  // touched, moved, recording or playing — not idle
  pw_motion = halo_motion_count();
  uint8_t w = halo_woke();
  if (pw_cmd_wake) {  // as a touch: asleep — wake, fading out — back, awake — idle from now
    pw_cmd_wake = false, pw_active_us = now;
    if (!w && (pw_mode == PW_AOD || pw_mode == PW_DARK)) w = PW_WAKE_CMD;
    else if (pw_mode == PW_FADING && !pw_locked) pw_mode = PW_ACTIVE, pw_amb.target = 0;
  }
  const bool boot = halo_boot_click(), boot_held = halo_boot_long();
  const uint8_t pwr = halo_pwr_event();
  if (halo_finger_blocked() && pw_locked) pw_shake_at = halo_ms() | 1, pw_listen_us = now + PW_LISTEN_S * 1000000ll;
  if (boot || boot_held) pw_button(1, pw_boot, boot_held, now);
  if (pwr) pw_button(2, pw_pwr, pwr & HALO_PWR_LONG, now);
  switch (pw_mode) {
    case PW_ACTIVE:
      if (!pw_locked && (pw_cmd_sleep || (pw_aod_on && pw_idle_s && now - pw_active_us > pw_idle_s * 1000000ll)) &&
          (pw_amb.x == 0 || pw_aod_way))
        pw_fade(true);
      break;
    case PW_FADING:
      // Touched or picked up while fading to Always On — the screen comes back (the touch is off when locked; a button
      // pressed in a hand moves the board too — the dark fade is not cancelled by that)
      if (!pw_locked && ((f.down && pw_wake_tap) || (pw_to_aod && moved))) {
        pw_mode = PW_ACTIVE, pw_amb.target = 0, pw_active_us = now;
      } else if (pw_amb.x > 0.995f || (!pw_aod_way && pw_level(pw_awake_level) == 0)) {  // there: the rest is not seen
        halo_spring_set(pw_amb, 1);
        if (pw_to_aod) {
          pw_mode = PW_AOD;
          face_still(true);
          if (pw_aod_home) halo_nav_home_now();  // Back to watch face: only the face is seen now — close the rest
        } else {
          pw_mode = PW_DARK;
          halo_display_on(false), LOG("screen off\n");
          halo_sleep();
        }
      }
      break;
    case PW_AOD:
      if (w && w != HALO_WAKE_TIMER && !pw_locked && pw_wakes(w)) {
        pw_wake(w);
      } else {
        if (w == HALO_WAKE_TIMER) fc_check = true;  // a new minute: face_tick sets the digits
        if (w && pw_locked) pw_listen_us = now + PW_LISTEN_S * 1000000ll;  // locked: up a while, for the button
        if (!halo_asleep() && now >= pw_listen_us) halo_sleep(pw_ms_to_minute());  // until the next minute
      }
      break;
    case PW_DARK:
      if (w && pw_wakes(w)) pw_wake(w);
      else if (w && !halo_asleep()) halo_sleep();  // (a way that is off: back to sleep)
      break;
  }
  pw_cmd_sleep = false;  // sent while already asleep — it must not put the board back to sleep right after it wakes
  fc_calm = pw_mode == PW_AOD || (pw_mode == PW_FADING && pw_to_aod);  // the animations: still from the move's start
  halo_spring_run(pw_amb, pw_amb.target > 0.5f ? K_PW_IN : K_PW_OUT, dt);
  halo_spring_run(pw_lock, K_PW_LOCK, dt);
  const int level = pw_level(pw_awake_level);
  if (level != pw_sent) halo_brightness(level), pw_sent = level;
}

// After face_tick (its minute is fresh), before halo_nav_update: the whole interface's view along the spring — 5 %
// smaller, and in Always On a few pixels elsewhere every minute (where — kept while the spring moves: a minute that
// turns on the way would make the face jump)
static void power_view() {
  static const int8_t SX[8] = {0, 5, 7, 5, 0, -5, -7, -5}, SY[8] = {-7, -5, 0, 5, 7, 5, 0, -5};
  static int spot;
  if (fc_min >= 0 && (pw_mode == PW_AOD || pw_amb.x == 0)) spot = fc_min & 7;
  const float k = pw_amb.x, m = pw_aod_way ? k : 0;
  pw_view = {SX[spot] * m, SY[spot] * m, 1 + (PW_AOD_SCALE - 1) * k, 1, false};
  halo_nav_ambient(k, pw_view, pw_aod_way);
}

// The lock over everything (laid out after halo_nav_update, drawn after halo_nav_draw), along with it into Always On:
// grows from 0.7 and fades in over the second half of the lock spring; touched while locked — a short wiggle (10 Hz,
// 0.45 s, dying away) — HaloWatch's lock icon (display.h), the gallery's icon font at 18 px
static void power_lock_layout() {
  const float a = halo_clamp01(pw_lock.x * 2 - 1), t = pw_shake_at ? (halo_ms() - pw_shake_at) / 1000.0f : 9;
  const float shake = t < 0.45f ? 5 * sinf(t * 2 * HALO_PI * 10) * (1 - t / 0.45f) : 0, s = 0.75f * (0.7f + 0.3f * a);
  gl_text_mid(pw_lock_ic, gl_ic24, "", pw_mid_lock, CX + shake, PW_LOCK_Y, s, 0xC6C6C6, a > 0.01f ? a : 0,
              &halo_black);
  pw_lock_ic.on_black = true;
  if (shake != 0) halo_subpixel = true;
  halo_view_commit(pw_lock_ic, pw_view);
}

static void power_lock_draw() {
  halo_item_draw(pw_lock_ic);
}

static void power_build() {  // after halo_begin: the motion sensor is on its I2C bus
  K_PW_IN = halo_spring(0.8f, 1);   // to Always On — calm: ~0.5 s most of the way
  K_PW_OUT = halo_spring(0.5f, 1);  // awake — quicker: ~0.3 s
  K_PW_LOCK = halo_spring(0.4f, 1);
  pw_mid_lock = halo_text_mid(gl_ic24, "");
#if FN_WAKE_RAISE || FN_STEPS || defined(__EMSCRIPTEN__)
  halo_motion_begin(150);  // (Raise off and no Steps — the motion sensor stays off)
#endif
  pw_active_us = esp_timer_get_time();
  halo_command("aod", [](const char *a) {
    if (isdigit((unsigned char)*a)) pw_aod_level = constrain(atoi(a), 1, 255), LOG("aod: brightness %d\n", pw_aod_level);
    else if (!strncmp(a, "home", 4)) pw_aod_home = !strstr(a, "off"), LOG("aod: back to face %s\n", pw_aod_home ? "on" : "off");
    else pw_aod_on = strcmp(a, "off") != 0, LOG("aod: %s\n", pw_aod_on ? "on" : "off");
  });
  halo_command("idle", [](const char *a) { pw_idle_s = max(0, atoi(a)), LOG("idle: %d s\n", pw_idle_s); });
  halo_command("sleep", [](const char *) { pw_cmd_sleep = true; });
  halo_command("wake", [](const char *) { pw_cmd_wake = true; });
}
