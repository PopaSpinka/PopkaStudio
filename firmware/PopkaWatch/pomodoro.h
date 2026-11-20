// pomodoro.h — Pomodoro (FL_POMODORO): focus intervals and breaks, every board's (a rectangle: in its shorter side's square) (projects/pomodoro: the design —
// Pomodoro-Timer.webp, the rings' colours from original-main-display.webp; the user's choices, 19 Nov 2026).
//
// The cycle: Focus (25 min) → a short break (5 min); after every PM_AFTER (4) focus intervals done — a long break
// (15 min) instead, and the count starts again. An interval that runs out: the alert — three beeps (Sound on) and the next one —
// started at once (Auto-start on) or waiting for START. Skip — the next one now, a skipped focus interval is not
// counted. Reset — the current interval from its beginning. Nothing of it survives a restart (the user's word).
// The time is counted to the interval's end (esp_timer: it goes on in light sleep), not tick by tick: the board can
// sleep meanwhile — power.h sets the wake-up for the end (pm_wake_ms) and brings the screen back (pm_due).
//
// The screen: the ring at the edge — what is left of the interval, from 12 o'clock clockwise, in the mode's colour
// (Focus red-orange, Short break blue, Long break violet), brighter toward its head, a glowing dot on it; the mode's
// icon (Font Awesome's eye, mug-hot, umbrella-beach), the time (MM:SS), Skip (⏭) right of it, the count (a tick an
// interval of the cycle), the mode's name; START / PAUSE in a pill, Reset (↺) left of it, Settings (gear) right.
// Settings: DURATION — Focus, Short break, Long break (min), Long break after (sessions), each with its −/+ screen;
// NOTIFICATIONS — Sound, Auto-start. Saved in flash (settings_poll); the editor's values (FN_PM_*, layers.h) are the
// starting ones — a new Load with other ones puts them back.
// Command: pomo — the state; pomo fast — seconds instead of minutes (a test, not saved); pomo start; pomo end — the
// interval runs out now.
// Included from settings.h (after sound.h).

#ifndef FN_PM_FOCUS  // layers.h has them when the editor wrote it
#define FN_PM_FOCUS 25
#define FN_PM_SHORT 5
#define FN_PM_LONG 15
#define FN_PM_AFTER 4
#define FN_PM_SOUND 1
#define FN_PM_AUTO 0
#endif

// Its fonts: a font and the size it stands for on the round screen (466 px) — every board draws it at that × pm_k,
// whatever size its font was made at (a small rectangle's are the rectangle's × 0.7: studio.py compact_fonts).
// The round board's: studio.py ROUND_APP_FONTS; a rectangle's: RECT_APP_FONTS (at the 7″ board's sizes). In a
// firmware with it; the page's engine builds them apart (engine/build.sh)
struct PmFont {
  const HaloFont &f;
  float px;
};
#ifdef HALO_ROUND
extern const HaloFont gl_pm_n84, gl_pm_n40, gl_pm_ic30;
#define PMF_TIME (PmFont{gl_pm_n84, 84})
#define PMF_VAL (PmFont{gl_pm_n40, 40})
#define PMF_IC (PmFont{gl_pm_ic30, 30})
#define PMF_T20 (PmFont{gl_t20, 20})
#define PMF_T24 (PmFont{gl_t24, 24})
#else
extern const HaloFont gl_r_pm_n110, gl_r_pm_n52, gl_r_pm_ic40, gl_r_pm_t26, gl_r_pm_t32;
#define PMF_TIME (PmFont{gl_r_pm_n110, 84})
#define PMF_VAL (PmFont{gl_r_pm_n52, 40})
#define PMF_IC (PmFont{gl_r_pm_ic40, 30})
#define PMF_T20 (PmFont{gl_r_pm_t26, 20})
#define PMF_T24 (PmFont{gl_r_pm_t32, 24})
#endif

#define PM_TEXT 0xFFF8EE
#define PM_DIM 0x9E9E9E
#define PM_GREY 0x2E3034     // the buttons
#define PM_TRACK 0x232428    // the ring under the arc
#define PM_SURF 0x1B1C1F     // the tabs' track
#define PM_OFF 0x3A3B40      // a tick not done yet, a switch off
static const uint32_t PM_COL[3] = {0xF04A2A, 0x3D8BF2, 0x9466FF};   // the arc's head end: Focus, Short, Long
static const uint32_t PM_HEAD[3] = {0xFF9C4D, 0x86C0FF, 0xC7A6FF};  // the dot on it
static const char *const PM_NAME[3] = {"FOCUS", "SHORT BREAK", "LONG BREAK"};
static const char PM_IC_MODE[3][4] = {"\xEF\x81\xAE", "\xEF\x9E\xB6", "\xEF\x97\x8A"};  // eye, mug-hot, umbrella-beach
static const char PM_IC_SKIP[] = "\xEF\x81\x91", PM_IC_RESET[] = "\xEF\x83\xA2", PM_IC_GEAR[] = "\xEF\x80\x93";
static const char PM_IC_X[] = "\xEF\x80\x8D", PM_IC_NEXT[] = "\xEF\x81\x94", PM_IC_BACK[] = "\xEF\x81\x93";
static const char PM_IC_MINUS[] = "\xEF\x81\xA8", PM_IC_PLUS[] = "\xEF\x81\xA7";

// Places: on the round screen (466 × 466, its middle PMC); another screen — the same, in the square of its shorter
// side, in its middle, at pm_k (pomodoro_layout moves the items there, pm_hit the finger back)
#define PMC 233.0f
#define PM_RING_R 214.0f
#define PM_RING_W 12.0f
#define PM_SEGS 36           // the arc in pieces: each a shade brighter toward the head
#define PM_ICON_Y 120.0f
#define PM_TIME_Y 196.0f
#define PM_SKIP_X (PMC + 166.0f)
#define PM_SKIP_R 24.0f
#define PM_TICK_Y 258.0f
#define PM_NAME_Y 292.0f
#define PM_BTN_Y 360.0f
#define PM_PILL_W 150.0f
#define PM_PILL_H 56.0f
#define PM_SIDE_DX 112.0f    // Reset and Settings from the middle
#define PM_SIDE_R 27.0f
#define PM_TICKS_MAX 8

enum { PM_FOCUS, PM_SHORT, PM_LONG };
enum { PG_MAIN, PG_SET, PG_EDIT };
enum { PB_SKIP, PB_PILL, PB_RESET, PB_GEAR, PB_CLOSE, PB_TAB0, PB_TAB1, PB_ROW0, PB_ROW1, PB_ROW2, PB_ROW3,
       PB_BACK, PB_MINUS, PB_PLUS, PB_N };

// ---------- The timer ----------
static int pm_min[3] = {FN_PM_FOCUS, FN_PM_SHORT, FN_PM_LONG}, pm_after = FN_PM_AFTER;
static bool pm_sound = FN_PM_SOUND, pm_auto = FN_PM_AUTO;
static bool pm_fast;           // pomo fast: seconds, not minutes (a test)
static int pm_mode = PM_FOCUS, pm_done;  // the mode; focus intervals done in this cycle
static bool pm_running;
static int64_t pm_end_us;      // running: when the interval ends
static int64_t pm_left_ms;     // not running: what is left of it
static bool pm_dirty;          // the settings changed: settings_poll saves them
static uint32_t pm_changed_ms;
static volatile bool pm_ding_want;  // the alert: the audio task plays it
static uint8_t pm_ding_kind;        // 0 — a break begins (lower beeps), 1 — focus begins (higher)

static int64_t pm_dur_ms(int mode) { return (int64_t)pm_min[mode] * (pm_fast ? 1000 : 60000); }
static int64_t pm_now_us() { return esp_timer_get_time(); }
static int64_t pm_left() {  // ms left of the interval
  return pm_running ? max((int64_t)0, (pm_end_us - pm_now_us() + 999) / 1000) : pm_left_ms;
}
static void pm_start() {
  if (pm_running) return;
  if (pm_left_ms <= 0) pm_left_ms = pm_dur_ms(pm_mode);
  pm_end_us = pm_now_us() + pm_left_ms * 1000, pm_running = true;
}
static void pm_pause() {
  if (!pm_running) return;
  pm_left_ms = pm_left(), pm_running = false;
}
static void pm_reset() { pm_running = false, pm_left_ms = pm_dur_ms(pm_mode); }

// The next interval: after one that ran out (counted) or Skip (not). It waits for START unless Auto-start
static void pm_next(bool counted) {
  const int was = pm_mode;
  if (was == PM_FOCUS) {
    if (counted) pm_done++;
    if (pm_done >= pm_after) pm_mode = PM_LONG, pm_done = 0;
    else pm_mode = PM_SHORT;
  } else {
    pm_mode = PM_FOCUS;
  }
  pm_running = false, pm_left_ms = pm_dur_ms(pm_mode);
  if (pm_auto) pm_start();
  LOG("pomo: %s → %s%s, %d/%d done\n", PM_NAME[was], PM_NAME[pm_mode], counted ? "" : " (skipped)", pm_done, pm_after);
}
static void pm_ring_out() {  // the interval ran out
  if (pm_sound) pm_ding_kind = pm_mode == PM_FOCUS ? 0 : 1, pm_ding_want = true;
  pm_next(true);
}
static bool pm_due() { return pm_running && pm_now_us() >= pm_end_us; }  // (power.h: before the frame's tick)
static uint32_t pm_wake_ms() {  // power.h: wake the board then (0 — no need)
  if (!pm_running) return 0;
  const int64_t ms = (pm_end_us - pm_now_us()) / 1000 + 20;
  return ms < 1 ? 1 : ms > 0x7FFFFFFF ? 0x7FFFFFFF : (uint32_t)ms;
}

// ---------- The alert: three clear beeps, made on the fly (the speaker's task, sound.h) ----------
// 150 ms of silence first (sent as samples: the speaker's amplifier comes on with them and is awake for the first
// beep), then three beeps of 130 ms, 170 ms apart; lower when a break begins, higher when focus does. Soft 6 ms edges
// (no clicks), a sine with a little of its third harmonic (louder on a small speaker)
#ifndef __EMSCRIPTEN__
#define PM_BEEPS 3
static int pm_tone_n, pm_tone_i;   // samples of the whole alert, done
static float pm_tone_ph;
static int pm_ding_fill(int16_t *out, int n) {
  const int rate = snd_rate ? snd_rate : 22050, lead = rate * 15 / 100, on = rate * 13 / 100, every = rate * 30 / 100;
  const int edge = rate * 6 / 1000;
  if (pm_ding_want) pm_ding_want = false, pm_tone_n = lead + every * (PM_BEEPS - 1) + on, pm_tone_i = 0, pm_tone_ph = 0;
  if (pm_tone_i >= pm_tone_n) return 0;
  const float f = pm_ding_kind ? 1318.5f : 880.0f;  // E6 — focus begins; A5 — a break begins
  int k = 0;
  for (; k < n && pm_tone_i < pm_tone_n; k++, pm_tone_i++) {
    const int t = pm_tone_i - lead, in = t >= 0 ? t % every : -1;
    float env = 0;
    if (t >= 0 && in < on) env = fminf(1, fminf(in, on - in) / (float)edge);
    pm_tone_ph += 2 * HALO_PI * f / rate;
    if (pm_tone_ph > 2 * HALO_PI) pm_tone_ph -= 2 * HALO_PI;
    out[k] = (int16_t)(env * 14000 * (sinf(pm_tone_ph) + 0.2f * sinf(3 * pm_tone_ph)));
  }
  return k;
}
static bool pm_dinging() { return pm_ding_want || pm_tone_i < pm_tone_n; }
#else
static bool pm_dinging() { return false; }
#endif

// ---------- Flash: the settings ----------
static Preferences pm_prefs;
static uint32_t pm_defaults_stamp() {  // the editor's values: another Load with other ones — they win again
  return (uint32_t)FN_PM_FOCUS | FN_PM_SHORT << 7 | FN_PM_LONG << 13 | FN_PM_AFTER << 20 | FN_PM_SOUND << 24 | FN_PM_AUTO << 25;
}
static void pm_load() {
  pm_prefs.begin("pomodoro", false);
  if (pm_prefs.getUInt("def", 0) != pm_defaults_stamp()) {
    pm_prefs.putUInt("def", pm_defaults_stamp());
    pm_prefs.putUChar("f", FN_PM_FOCUS), pm_prefs.putUChar("s", FN_PM_SHORT), pm_prefs.putUChar("l", FN_PM_LONG);
    pm_prefs.putUChar("a", FN_PM_AFTER), pm_prefs.putUChar("snd", FN_PM_SOUND), pm_prefs.putUChar("auto", FN_PM_AUTO);
  }
  pm_min[PM_FOCUS] = constrain((int)pm_prefs.getUChar("f", FN_PM_FOCUS), 1, 90);
  pm_min[PM_SHORT] = constrain((int)pm_prefs.getUChar("s", FN_PM_SHORT), 1, 30);
  pm_min[PM_LONG] = constrain((int)pm_prefs.getUChar("l", FN_PM_LONG), 1, 60);
  pm_after = constrain((int)pm_prefs.getUChar("a", FN_PM_AFTER), 2, PM_TICKS_MAX);
  pm_sound = pm_prefs.getUChar("snd", FN_PM_SOUND), pm_auto = pm_prefs.getUChar("auto", FN_PM_AUTO);
}
static void pm_save_poll() {  // settings_poll (loop): 0.8 s after the last change
  if (!pm_dirty || millis() - pm_changed_ms < 800) return;
  pm_dirty = false;
  pm_prefs.putUChar("f", pm_min[PM_FOCUS]), pm_prefs.putUChar("s", pm_min[PM_SHORT]), pm_prefs.putUChar("l", pm_min[PM_LONG]);
  pm_prefs.putUChar("a", pm_after), pm_prefs.putUChar("snd", pm_sound), pm_prefs.putUChar("auto", pm_auto);
  LOG("pomo: settings saved (%d/%d/%d min, long after %d, sound %d, auto %d)\n", pm_min[0], pm_min[1], pm_min[2], pm_after,
      pm_sound, pm_auto);
}
static void pm_changed() { pm_dirty = true, pm_changed_ms = millis(); }

// ---------- The screen's state ----------
struct PmRow {  // DURATION's rows: a value, its range, its unit
  const char *name, *unit;
  int lo, hi;
};
static const PmRow PM_ROWS[4] = {{"Focus Session", "min", 1, 90}, {"Short break", "min", 1, 30},
                                 {"Long break", "min", 1, 60}, {"Long break after", "Sess.", 2, PM_TICKS_MAX}};
static int *pm_row_val(int r) { return r < 3 ? &pm_min[r] : &pm_after; }

static int pm_page = PG_MAIN, pm_tab, pm_edit_row;
static HaloSpring pm_page_in;  // the page just opened: fades and grows in
static HaloSpring pm_runm, pm_modem[3], pm_sw[2];  // running (the arc bright); the mode's colour; the switches
static HaloSpringK K_PM;
static Press pm_press[PB_N];
static int pm_armed = -1;
static bool pm_active;
static uint32_t pm_seen;
static uint32_t pm_hold_ms;   // −/+ held: repeats
static float pm_mid_ic, pm_mid_time, pm_mid_t20, pm_mid_t24, pm_mid_n40;
static char pm_time_s[8], pm_val_s[4][4], pm_edit_s[4];
static HaloView pm_v;
static float pm_k = 1, pm_ox, pm_oy;  // the round screen's places → this screen's: × pm_k, + (pm_ox, pm_oy)

static void pm_set_value(int r, int v) {
  const int nv = constrain(v, PM_ROWS[r].lo, PM_ROWS[r].hi);
  if (nv == *pm_row_val(r)) return;
  const bool fresh = r < 3 && r == pm_mode && !pm_running && pm_left_ms == pm_dur_ms(pm_mode);  // (not started yet)
  *pm_row_val(r) = nv;
  if (r == 3) pm_done = min(pm_done, pm_after - 1);
  if (fresh) pm_left_ms = pm_dur_ms(pm_mode);  // the new length shows at once
  pm_changed();
}
static void pm_open(int page) {
  pm_page = page, pm_armed = -1;
  halo_spring_set(pm_page_in, 0), pm_page_in.target = 1;
}

static void pm_status() {
  const int64_t s = pm_left() / 1000;
  LOG("pomo: %s %s, %lld:%02lld left, %d/%d done; %d/%d/%d %s, long after %d, sound %s, auto-start %s\n", PM_NAME[pm_mode],
      pm_running ? "running" : "waiting", s / 60, s % 60, pm_done, pm_after, pm_min[0], pm_min[1], pm_min[2],
      pm_fast ? "s (fast)" : "min", pm_after, pm_sound ? "on" : "off", pm_auto ? "on" : "off");
}

static void pomodoro_build() {
  K_PM = halo_spring(HALO_M3X_FAST);
  // The ring 19 px in from the shorter side's edge, as on the round screen (233 − 214); the rest with it
  pm_k = (fminf(HALO_W, HALO_H) / 2.0f - (PMC - PM_RING_R)) / PM_RING_R;
  pm_ox = CX - PMC * pm_k, pm_oy = CY - PMC * pm_k;
  pm_load();
  pm_left_ms = pm_dur_ms(pm_mode);
  pm_mid_ic = halo_text_mid(PMF_IC.f, PM_IC_MODE[0]), pm_mid_time = halo_text_mid(PMF_TIME.f, "0");
  pm_mid_t20 = halo_text_mid(PMF_T20.f, "FOCUS"), pm_mid_t24 = halo_text_mid(PMF_T24.f, "START");
  pm_mid_n40 = halo_text_mid(PMF_VAL.f, "0");
  halo_spring_set(pm_page_in, 1);
  for (int m = 0; m < 3; m++) halo_spring_set(pm_modem[m], m == pm_mode);
  halo_spring_set(pm_sw[0], pm_sound), halo_spring_set(pm_sw[1], pm_auto);
#ifndef __EMSCRIPTEN__
  snd_alert = pm_ding_fill;
#endif
  halo_command("pomo", [](const char *a) {
    while (*a == ' ') a++;
    if (!strcmp(a, "fast")) pm_fast = !pm_fast, pm_reset(), LOG("pomo: %s\n", pm_fast ? "seconds (a test)" : "minutes");
    else if (!strcmp(a, "end") && pm_running) pm_end_us = pm_now_us();
    else if (!strcmp(a, "start")) pm_start();
    pm_status();
  });
}

// Every frame, whatever is on screen: the interval running out, the springs
static void pomodoro_tick(float dt) {
  const HaloFinger &f = halo_finger;
  if (pm_due()) {
#ifndef __EMSCRIPTEN__
    if (pm_sound) snd_begin(22050);  // (the speaker, once: sound.h)
#endif
    pm_ring_out();
  }
  if (pm_page != PG_MAIN && halo_ms() - pm_seen > 1000) pm_page = PG_MAIN;  // the screen gone: back to the timer
  pm_runm.target = pm_running;
  halo_spring_run(pm_runm, K_PM, dt), halo_spring_run(pm_page_in, K_PM, dt);
  for (int m = 0; m < 3; m++) pm_modem[m].target = m == pm_mode, halo_spring_run(pm_modem[m], K_PM, dt);
  pm_sw[0].target = pm_sound, pm_sw[1].target = pm_auto;
  for (auto &s : pm_sw) halo_spring_run(s, K_PM, dt);
  for (int i = 0; i < PB_N; i++) press_update(pm_press[i], f.down && pm_active && pm_armed == i, dt);
  pm_active = false;  // pomodoro_update sets it again while the screen is up
  const int64_t s = (pm_left() + 999) / 1000;
  snprintf(pm_time_s, sizeof pm_time_s, "%02d:%02d", (int)min((int64_t)99, s / 60), (int)(s % 60));
  for (int r = 0; r < 4; r++) snprintf(pm_val_s[r], sizeof pm_val_s[r], r < 3 ? "%02d" : "%d", *pm_row_val(r));
  snprintf(pm_edit_s, sizeof pm_edit_s, pm_edit_row < 3 ? "%02d" : "%d", *pm_row_val(pm_edit_row));
}
static bool pm_busy() { return pm_dinging(); }  // (the alert keeps the screen up; a running timer may sleep)

static void pomodoro_enter() {
  pm_armed = -1, pm_seen = halo_ms();
  if (pm_page != PG_MAIN) pm_page = PG_MAIN;
}

// ---------- The finger ----------
// Where each button is on its page (x, y, the radius it takes a touch in)
static bool pm_btn_place(int b, float &x, float &y, float &r) {
  r = 40;
  switch (b) {
    case PB_SKIP: x = PM_SKIP_X, y = PM_TIME_Y; return pm_page == PG_MAIN;
    case PB_PILL: x = PMC, y = PM_BTN_Y, r = 50; return pm_page == PG_MAIN;
    case PB_RESET: x = PMC - PM_SIDE_DX, y = PM_BTN_Y; return pm_page == PG_MAIN;
    case PB_GEAR: x = PMC + PM_SIDE_DX, y = PM_BTN_Y; return pm_page == PG_MAIN;
    case PB_CLOSE: x = PMC + 118, y = 92; return pm_page == PG_SET;
    case PB_TAB0: case PB_TAB1: x = PMC + (b == PB_TAB0 ? -89 : 89), y = 150, r = 46; return pm_page == PG_SET;
    case PB_ROW0: case PB_ROW1: case PB_ROW2: case PB_ROW3: {
      const int i = b - PB_ROW0;
      x = PMC, y = 214 + i * 52, r = 26;
      return pm_page == PG_SET && (pm_tab == 0 || i < 2);
    }
    case PB_BACK: x = PMC - 118, y = 92; return pm_page == PG_EDIT;
    case PB_MINUS: x = PMC - 132, y = 238, r = 44; return pm_page == PG_EDIT;
    case PB_PLUS: x = PMC + 132, y = 238, r = 44; return pm_page == PG_EDIT;
  }
  return false;
}
static bool pm_hit(int b, float fx, float fy) {
  float x, y, r;
  fx = (fx - pm_ox) / pm_k, fy = (fy - pm_oy) / pm_k;  // (this screen → the round screen's places)
  if (!pm_btn_place(b, x, y, r)) return false;
  if (b >= PB_ROW0 && b <= PB_ROW3) return fabsf(fy - y) < r && fabsf(fx - x) < 175;  // a whole row
  return (fx - x) * (fx - x) + (fy - y) * (fy - y) < r * r;
}
static void pm_action(int b) {
  switch (b) {
    case PB_SKIP: pm_next(false); break;
    case PB_PILL: pm_running ? pm_pause() : pm_start(); break;
    case PB_RESET: pm_reset(); break;
    case PB_GEAR: pm_tab = 0, pm_open(PG_SET); break;
    case PB_CLOSE: pm_open(PG_MAIN); break;
    case PB_TAB0: pm_tab = 0; break;
    case PB_TAB1: pm_tab = 1; break;
    case PB_ROW0: case PB_ROW1: case PB_ROW2: case PB_ROW3: {
      const int i = b - PB_ROW0;
      if (pm_tab == 0) pm_edit_row = i, pm_open(PG_EDIT);
      else (i ? pm_auto : pm_sound) ^= 1, pm_changed();
      break;
    }
    case PB_BACK: pm_open(PG_SET); break;
    case PB_MINUS: case PB_PLUS: pm_set_value(pm_edit_row, *pm_row_val(pm_edit_row) + (b == PB_PLUS ? 1 : -1)); break;
  }
}
static void pomodoro_update(float dt) {
  (void)dt;
  HaloFinger &f = halo_finger;
  pm_seen = halo_ms(), pm_active = true;
  if (f.pressed && !f.owner) {
    pm_armed = -1, pm_hold_ms = 0;
    for (int b = 0; b < PB_N && pm_armed < 0; b++)
      if (pm_hit(b, f.x0, f.y0)) pm_armed = b;
  }
  if (f.dragging) pm_armed = -1;
  // −/+ held: once at once, then on — faster after a while (the value's screen only)
  if (f.down && (pm_armed == PB_MINUS || pm_armed == PB_PLUS)) {
    const uint32_t held = halo_ms() - f.t0;
    if (!pm_hold_ms) pm_action(pm_armed), pm_hold_ms = halo_ms() + 450;
    else if (halo_ms() >= pm_hold_ms) pm_action(pm_armed), pm_hold_ms = halo_ms() + (held > 2000 ? 60 : 130);
  } else if (f.tap && pm_armed >= 0 && pm_armed != PB_MINUS && pm_armed != PB_PLUS) {  // (−/+ changed on the touch)
    pm_action(pm_armed);
  }
  if (!f.down) pm_armed = -1;
}

// ---------- Layout and drawing: one pool of items, the page's (the rest hidden) ----------
#define PM_IT 72
static HaloItem pm_it[PM_IT];
static int pm_n;

static uint32_t pm_mix(uint32_t a, uint32_t b, float t) {
  t = halo_clamp01(t);
  const int k = (int)(t * 256);
  auto ch = [&](int s) { return (uint32_t)(((a >> s & 255) * (256 - k) + (b >> s & 255) * k) >> 8) << s; };
  return ch(16) | ch(8) | ch(0);
}
static uint32_t pm_mode_col(const uint32_t (&c)[3]) {  // the mode's colour, cross-faded on a change
  uint32_t out = c[0];
  float w = halo_clamp01(pm_modem[0].x);
  for (int m = 1; m < 3; m++) {
    const float k = halo_clamp01(pm_modem[m].x);
    if (k > 0) out = pm_mix(out, c[m], k / fmaxf(0.001f, w + k)), w += k;
  }
  return out;
}
static HaloItem &pm_new() { return pm_it[pm_n < PM_IT ? pm_n++ : PM_IT - 1]; }
static float pm_sc(int b) { return 1 - 0.07f * halo_clamp01(pm_press[b].s.x); }  // pressed — 7 % smaller
static HaloItem &pm_circle(float x, float y, float r, uint32_t c, float a = 1) {
  HaloItem &it = pm_new();
  it.kind = HI_CIRCLE, it.x = x, it.y = y, it.r = r, it.scale = 1, it.color = c, it.alpha = a, it.on_black = true, it.on = nullptr;
  return it;
}
static HaloItem &pm_rrect(float x, float y, float w, float h, float r, uint32_t c, float a = 1) {
  HaloItem &it = pm_new();
  it.kind = HI_RRECT, it.x = x, it.y = y, it.w = w, it.h = h, it.r = r, it.scale = 1, it.color = c, it.alpha = a;
  it.on_black = true, it.on = nullptr;
  return it;
}
static HaloItem &pm_text(const PmFont &F, const char *s, float mid, float x, float y, uint32_t c, float a = 1,
                         int align = 1, const HaloItem *on = &halo_black, float sc = 1, int spacing = 0) {
  HaloItem &it = pm_new();
  gl_text_mid(it, F.f, s, mid, x, y, sc * F.px / F.f.size, c, a, on);  // (at its size on the round screen)
  it.align = align, it.spacing = spacing;
  return it;
}
static void pm_icon_btn(int b, float x, float y, float r, const char *ic, float a, uint32_t bg = PM_GREY) {
  const float sc = pm_sc(b);
  HaloItem &c = pm_circle(x, y, r * sc, bg, a);
  pm_text(PMF_IC, ic, pm_mid_ic, x, y, PM_TEXT, a, 1, &c, 0.72f * sc);
}

static void pm_layout_main(float a) {
  const uint32_t col = pm_mode_col(PM_COL), head = pm_mode_col(PM_HEAD);
  const float frac = (float)pm_left() / fmaxf(1.0f, (float)pm_dur_ms(pm_mode));
  const bool fresh = !pm_running && pm_left_ms >= pm_dur_ms(pm_mode);  // not started: the track only (the design)
  const float run = halo_clamp01(pm_runm.x), bright = 0.55f + 0.45f * run;  // paused — dimmer
  // The ring: the track, the arc in pieces (darker toward its tail at 12 o'clock), the dot at its head
  HaloItem &tr = pm_new();
  tr.kind = HI_RING, tr.x = tr.y = PMC, tr.r = PM_RING_R, tr.w = PM_RING_W, tr.v = 1, tr.scale = 1;
  tr.color = PM_TRACK, tr.color2 = 0, tr.alpha = a, tr.on_black = true, tr.on = nullptr;
  const float end = fresh ? 0 : 2 * HALO_PI * halo_clamp01(frac), seg = 2 * HALO_PI / PM_SEGS;
  for (int i = 0; i < PM_SEGS; i++) {
    HaloItem &s = pm_new();
    const float a0 = i * seg, a1 = fminf(end, (i + 1) * seg), t = end > 0 ? (a0 + a1) / 2 / end : 0;
    s.kind = HI_ARC, s.x = s.y = PMC, s.r = PM_RING_R, s.w = PM_RING_W, s.scale = 1, s.a0 = a0, s.a1 = fmaxf(a0, a1);
    s.color = pm_mix(pm_mix(PM_TRACK, col, 0.35f + 0.65f * t), head, t * t * 0.6f);
    s.alpha = a1 > a0 + 0.0005f ? a * bright : 0, s.on_black = true, s.on = nullptr;
  }
  const float hx = PMC + sinf(end) * PM_RING_R, hy = PMC - cosf(end) * PM_RING_R, ha = end > 0.01f ? a * bright : 0;
  pm_circle(hx, hy, PM_RING_W * 1.25f, head, ha * 0.25f * run);  // the glow (running)
  pm_circle(hx, hy, PM_RING_W * 0.62f, pm_mix(head, PM_TEXT, 0.35f), ha);
  // The middle: the mode's icon, the time, Skip, the ticks, the mode's name
  for (int m = 0; m < 3; m++) {
    const float k = halo_clamp01(pm_modem[m].x);
    pm_text(PMF_IC, PM_IC_MODE[m], pm_mid_ic, PMC, PM_ICON_Y, pm_mix(PM_TEXT, PM_HEAD[m], 0.25f), a * k);
  }
  pm_text(PMF_TIME, pm_time_s, pm_mid_time, PMC, PM_TIME_Y, PM_TEXT, a);
  pm_icon_btn(PB_SKIP, PM_SKIP_X, PM_TIME_Y, PM_SKIP_R, PM_IC_SKIP, a);
  const int ticks = pm_after, shown = pm_mode == PM_LONG ? ticks : pm_done;
  for (int i = 0; i < ticks; i++) {
    const float x = PMC + (i - (ticks - 1) / 2.0f) * 14;
    pm_rrect(x, PM_TICK_Y, 5, 16, 2.5f, i < shown ? col : PM_OFF, a);
  }
  pm_text(PMF_T20, PM_NAME[pm_mode], pm_mid_t20, PMC, PM_NAME_Y, PM_DIM, a, 1, &halo_black, 1, 2);
  // The row: Reset, START / PAUSE, Settings
  const float ps = pm_sc(PB_PILL);
  HaloItem &pill = pm_rrect(PMC, PM_BTN_Y, PM_PILL_W * ps, PM_PILL_H * ps, PM_PILL_H / 2 * ps, PM_GREY, a);
  pm_text(PMF_T24, pm_running ? "PAUSE" : "START", pm_mid_t24, PMC, PM_BTN_Y, PM_TEXT, a, 1, &pill, ps, 2);
  pm_icon_btn(PB_RESET, PMC - PM_SIDE_DX, PM_BTN_Y, PM_SIDE_R, PM_IC_RESET, a, 0x000000);
  pm_icon_btn(PB_GEAR, PMC + PM_SIDE_DX, PM_BTN_Y, PM_SIDE_R, PM_IC_GEAR, a, 0x000000);
  // (the side buttons: outlined, as in the design — a ring around the black)
  for (float x : {PMC - PM_SIDE_DX, PMC + PM_SIDE_DX}) {
    HaloItem &r = pm_new();
    r.kind = HI_RING, r.x = x, r.y = PM_BTN_Y, r.r = PM_SIDE_R - 1, r.w = 2, r.v = 1, r.scale = 1;
    r.color = PM_GREY, r.color2 = 0, r.alpha = a, r.on_black = true, r.on = nullptr;
  }
}

static void pm_layout_settings(float a) {
  pm_text(PMF_T24, "Settings", pm_mid_t24, PMC, 92, PM_TEXT, a);
  pm_icon_btn(PB_CLOSE, PMC + 118, 92, 24, PM_IC_X, a);
  // The tabs: a track, the picked one lighter
  HaloItem &trk = pm_rrect(PMC, 150, 364, 52, 26, PM_SURF, a);
  (void)trk;
  HaloItem &sel = pm_rrect(PMC + (pm_tab ? 89 : -89), 150, 176, 44, 22, PM_GREY, a);
  pm_text(PMF_T20, "DURATION", pm_mid_t20, PMC - 89, 150, pm_tab ? PM_DIM : PM_TEXT, a, 1, pm_tab ? &trk : &sel);
  pm_text(PMF_T20, "NOTIFICATIONS", pm_mid_t20, PMC + 89, 150, pm_tab ? PM_TEXT : PM_DIM, a, 1, pm_tab ? &sel : &trk);
  const int rows = pm_tab ? 2 : 4;
  for (int i = 0; i < rows; i++) {
    const float y = 214 + i * 52, sc = pm_sc(PB_ROW0 + i), ra = a * (1 - 0.35f * halo_clamp01(pm_press[PB_ROW0 + i].s.x));
    if (pm_tab == 0) {
      pm_text(PMF_T20, PM_ROWS[i].name, pm_mid_t20, PMC - 172, y, PM_TEXT, ra, 0);  // (20 px: the user's word)
      pm_text(PMF_VAL, pm_val_s[i], pm_mid_n40, PMC + 94, y, PM_TEXT, ra, 2);
      pm_text(PMF_T20, PM_ROWS[i].unit, pm_mid_t20, PMC + 100, y + 4, PM_DIM, ra, 0);
      pm_text(PMF_IC, PM_IC_NEXT, pm_mid_ic, PMC + 166, y, PM_DIM, ra, 1, &halo_black, 0.55f);
    } else {  // a switch: the track goes the mode's red when on, the knob slides
      static const char *const N[2] = {"Sound", "Auto-start"};
      const float k = halo_clamp01(pm_sw[i].x);
      pm_text(PMF_T20, N[i], pm_mid_t20, PMC - 172, y, PM_TEXT, ra, 0);
      pm_rrect(PMC + 128, y, 56, 32, 16, pm_mix(PM_OFF, PM_COL[0], k), a);
      pm_circle(PMC + 128 - 12 + 24 * k, y, 12 * sc, PM_TEXT, a);
    }
  }
}

static void pm_layout_edit(float a) {
  const PmRow &r = PM_ROWS[pm_edit_row];
  pm_icon_btn(PB_BACK, PMC - 118, 92, 24, PM_IC_BACK, a);
  pm_text(PMF_T24, r.name, pm_mid_t24, PMC, 156, PM_TEXT, a);
  pm_text(PMF_TIME, pm_edit_s, pm_mid_time, PMC, 238, PM_TEXT, a);
  for (int b : {PB_MINUS, PB_PLUS}) {
    const float x = PMC + (b == PB_PLUS ? 132 : -132), sc = pm_sc(b);
    const int v = *pm_row_val(pm_edit_row);
    const bool can = b == PB_PLUS ? v < r.hi : v > r.lo;
    HaloItem &bx = pm_rrect(x, 238, 56 * sc, 56 * sc, 16 * sc, PM_GREY, a);
    pm_text(PMF_IC, b == PB_PLUS ? PM_IC_PLUS : PM_IC_MINUS, pm_mid_ic, x, 238, can ? PM_TEXT : PM_OFF, a, 1, &bx, 0.8f * sc);
  }
  pm_text(PMF_T24, r.unit, pm_mid_t24, PMC, 312, PM_DIM, a);
}

static void pomodoro_layout(const HaloView &view) {
  pm_v = view;
  pm_n = 0;
  const float in = halo_clamp01(pm_page_in.x);
  HaloView v = view;  // a page just opened: from 96 % (the view scales from the screen's middle) and transparent
  v.a *= in, v.s *= 0.96f + 0.04f * in;
  if (pm_page == PG_MAIN) pm_layout_main(1);
  else if (pm_page == PG_SET) pm_layout_settings(1);
  else pm_layout_edit(1);
  const int used = pm_n;
  for (int i = used; i < PM_IT; i++) {  // the rest: hidden, set afresh (the view changes an item's own fields)
    HaloItem &it = pm_it[i];
    it.kind = HI_CIRCLE, it.x = it.y = PMC, it.r = 0, it.scale = 1, it.alpha = 0, it.on_black = true, it.on = nullptr;
  }
  for (int i = 0; i < used; i++) {  // the round screen's places → this screen's (round: as they are)
    HaloItem &it = pm_it[i];
    it.x = pm_ox + it.x * pm_k, it.y = pm_oy + it.y * pm_k, it.scale *= pm_k;
  }
  for (int i = 0; i < PM_IT; i++) halo_view_commit(pm_it[i], v);
  pm_n = used;
}

static void pomodoro_draw() {
  for (int i = 0; i < pm_n; i++) halo_item_draw(pm_it[i]);
}
