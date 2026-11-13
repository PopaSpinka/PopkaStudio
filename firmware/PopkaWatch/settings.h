// settings.h — HaloWatch's Settings sections as PopkaStudio layers, one to one (projects/HaloWatch: display.h,
// battery.h): Display (FL_DISPLAY) — the screen's brightness, WearKit's slider, kept in flash; Battery (FL_BATTERY) —
// the level from the power chip, what is going on, how long it lasts (redrawn for the round screen: a line along the
// bezel, words along the arc). Each fills its screen; the editor puts it on a screen of its own behind a gallery card
// (Gallery → Display / Battery).
//
// Display: as on HaloWatch, without its lock — PopkaStudio's lock is a button action (power.h, Lock). The slider sets
// the awake brightness (pw_awake_level: power.h leads it to Always On and dark along its spring); its hint says what
// BOOT does (the editor's Buttons). Battery: halo_battery() every 2 s; when USB came or went and the percent then, a
// slot per 30 min of history — in NVS ("battery"/"hist", "plug"), written from loop() (settings_poll); the forecast —
// by the pace since then. The editor's page shows a made-up state (battery_demo; on the board — the command batdemo).
// Only in a firmware with that layer (FN_DISPLAY, FN_BATTERY — layers.h); the editor's page has both.
// Included from face.h after gallery.h and wxscreen.h (gl_* fonts and helpers, Press).
#include <Preferences.h>

#if defined(__EMSCRIPTEN__) || (defined(FN_DISPLAY) && FN_DISPLAY)
#define HAS_DISPLAY 1
#else
#define HAS_DISPLAY 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_BATTERY) && FN_BATTERY)
#define HAS_BATTERY 1
#else
#define HAS_BATTERY 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_MUSIC) && FN_MUSIC)  // HaloWatch's Music: the player (player.h, music.h)
#define HAS_MUSIC 1
#else
#define HAS_MUSIC 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_RECORDER) && FN_RECORDER)  // the Recorder (recorder.h)
#define HAS_RECORDER 1
#else
#define HAS_RECORDER 0
#endif

// The Radio (radio.h): the round board's — in a firmware with it and in that board's page
#if defined(HALO_ROUND) && (defined(__EMSCRIPTEN__) || (defined(FN_RADIO) && FN_RADIO))
#define HAS_RADIO 1
#else
#define HAS_RADIO 0
#endif

// The Video app (video.h): only a rectangular board has it — in a firmware with it and in that board's page
#if !defined(HALO_ROUND) && (defined(__EMSCRIPTEN__) || (defined(FN_VIDEO) && FN_VIDEO))
#define HAS_VIDEO 1
#else
#define HAS_VIDEO 0
#endif

#define ST_CARD_HI 0x2E3034  // HaloWatch's CARD_HI (surfaceContainerHigh)
// About and Power (device.h, included after this): what runs every frame, from loop(), at start; About wants the
// microphones (the Recorder's too: the one who still wants them keeps them on)
static void dev_build();
static void dev_tick(float dt);
static void dev_poll();
static volatile bool dv_mic_want;
static int pw_awake_level = 180;  // the panel awake (power.h): Display's slider, else HaloWatch's default (step 7)
static int pw_boot_fn();          // what BOOT does (power.h): Display's hint
#ifndef HALO_ROUND
static const FaceLayer *dp_link;  // the Display layer laid out now: where its row About goes (settings_rect.h)
#endif

// ---------- Display ----------
#if HAS_DISPLAY
#ifdef HALO_ROUND
#define DP_STEPS 8
#define DP_DEFAULT 7  // = 180, the brightness before this screen existed
// Panel brightness (command 0x51) for each step: roughly even steps for the eye (it sees brightness about as a
// logarithm). The lowest must stay visible
static const uint8_t DP_LEVEL[DP_STEPS + 1] = {24, 34, 48, 66, 90, 118, 150, 180, 255};
#else
#define DP_STEPS 20    // a backlight: 5 % a step (settings_rect.h)
#define DP_DEFAULT 14  // 70 %
// The level for the backlight's PWM: even for the eye, the lowest still visible
static const uint8_t DP_LEVEL[DP_STEPS + 1] = {10, 11, 13, 15, 18, 22, 27, 33, 40, 48, 58, 69, 81, 95, 110, 127, 146, 167, 190, 221, 255};
extern const HaloFont gl_r_n72, gl_r_t44, gl_r_t32, gl_r_ic36;
#endif
#define DP_W 410.0f  // the pill: 205 dp x 52 dp
#define DP_H 104.0f
#define DP_BTN 96.0f  // "−" and "+" zones
#define DP_HEAD_Y 96.0f
#define DP_LABEL_Y 166.0f
#define DP_SL_Y 240.0f
#define DP_HINT_Y 334.0f
#define DP_PRIMARY 0xA8C7FA  // Wear OS 6 dark scheme
#define DP_ON_PRIMARY 0x0B2A5C
#define DP_OUTLINE 0x8E9099
#define DP_TRACK 0x45474D    // the empty bar; a pressed "−"/"+"

static volatile int dp_value = DP_DEFAULT;  // 0..DP_STEPS; settings_poll saves it (from loop())
static float dp_shown = DP_DEFAULT, dp_from = DP_DEFAULT, dp_t = 9;  // in steps; the 150 ms catch-up
static HaloEase EASE_SHARP;
static Press dp_press[2];  // "−", "+"
static int dp_armed;       // the finger came down on: −1 — "−", 1 — "+", 2 — the bar, 0 — nothing
static uint32_t dp_hold_at;
static float dp_mid_head, dp_mid_label, dp_mid_hint, dp_mid_icon;
static HaloItem dp_head, dp_label, dp_hint, dp_pill, dp_track, dp_fill, dp_dot[DP_STEPS - 1];
static HaloItem dp_circle[2], dp_icon[2];
static Preferences dp_prefs;
static int dp_saved = -1, dp_seen = -1;  // the brightness in flash; the last one seen and when it changed
static uint32_t dp_changed_at;

static uint8_t dp_level(float steps) {  // between the table's steps — linearly
  const float s = halo_clamp01(steps / DP_STEPS) * DP_STEPS;
  const int i = min((int)s, DP_STEPS - 1);
  return (uint8_t)lroundf(DP_LEVEL[i] + (DP_LEVEL[i + 1] - DP_LEVEL[i]) * (s - i));
}

#ifdef HALO_ROUND
static const char *dp_hint_text() {  // what BOOT does (the editor's Buttons): 1 — screen off, 2 — lock, 3 — Always On
  switch (pw_boot_fn()) {
    case 1: return "Press BOOT to turn the screen off";
    case 2: return "Press BOOT to lock";
    case 3: return "Press BOOT for Always On";
  }
  return "";
}

static void display_build() {
  dp_mid_head = halo_text_mid(gl_title, "Display");
  dp_mid_label = halo_text_mid(gl_t20, "Brightness");
  dp_mid_hint = halo_text_mid(gl_t20, "Press BOOT to turn the screen off");
  dp_mid_icon = halo_text_mid(gl_ic32, "");
  halo_ease_make(EASE_SHARP, 0, 0, 0, 1);
}
#endif

static void dp_set(int v) {
  v = constrain(v, 0, DP_STEPS);
  if (v == dp_value) return;
  dp_from = dp_shown, dp_t = 0, dp_value = v;
}

static void display_enter() {
  dp_armed = 0;
  for (auto &p : dp_press) halo_spring_set(p.s, 0), p.going_down = false;
}

#ifdef HALO_ROUND
static void display_update(float dt) {
  HaloFinger &f = halo_finger;
  const float x0 = CX - DP_W / 2, x1 = CX + DP_W / 2, bx = x0 + DP_BTN, bw = DP_W - 2 * DP_BTN;
  const bool on_pill = fabsf(f.y0 - DP_SL_Y) < DP_H / 2 && f.x0 > x0 && f.x0 < x1;
  if (f.pressed) {
    dp_armed = !on_pill || f.owner ? 0 : f.x0 < bx ? -1 : f.x0 > bx + bw ? 1 : 2;
    dp_hold_at = halo_ms() + 500;
  }
  if (dp_armed == 2 && f.down && (!f.dragging || f.horiz) && halo_finger_claim(&dp_armed))  // the bar: by position
    dp_set((int)lroundf(halo_clamp01((f.x - bx) / bw) * DP_STEPS));
  if ((dp_armed == -1 || dp_armed == 1) && f.dragging) dp_armed = 0;  // moved — a swipe, not a press
  if ((dp_armed == -1 || dp_armed == 1) && f.tap) dp_set(dp_value + dp_armed);
  if ((dp_armed == -1 || dp_armed == 1) && f.down && (int32_t)(halo_ms() - dp_hold_at) >= 0)  // held: a repeat
    dp_set(dp_value + dp_armed), dp_hold_at = halo_ms() + 60;
  press_update(dp_press[0], dp_armed == -1 && f.down, dt);
  press_update(dp_press[1], dp_armed == 1 && f.down, dt);
  if (!f.down) dp_armed = 0;
}

static void display_layout(const HaloView &v) {
  gl_text_mid(dp_head, gl_title, "Display", dp_mid_head, CX, DP_HEAD_Y, 1, GL_TEXT, 1, &halo_black);
  gl_text_mid(dp_label, gl_t20, "Brightness", dp_mid_label, CX, DP_LABEL_Y, 1, GL_DIM, 1, &halo_black);
  const char *hint = dp_hint_text();
  gl_text_mid(dp_hint, gl_t20, hint, dp_mid_hint, CX, DP_HINT_Y, 1, GL_DIM, *hint ? 1 : 0, &halo_black);
  dp_head.on_black = dp_label.on_black = dp_hint.on_black = true;
  halo_view_commit(dp_head, v), halo_view_commit(dp_label, v), halo_view_commit(dp_hint, v);
  // The pill and the pressed buttons' circles under the icons
  dp_pill.kind = HI_RRECT, dp_pill.x = CX, dp_pill.y = DP_SL_Y, dp_pill.w = DP_W, dp_pill.h = DP_H, dp_pill.r = DP_H / 2;
  dp_pill.scale = 1, dp_pill.alpha = 1, dp_pill.color = ST_CARD_HI, dp_pill.on_black = true;
  halo_view_commit(dp_pill, v);
  for (int i = 0; i < 2; i++) {
    const float p = halo_clamp01(dp_press[i].s.x), cx = i ? CX + DP_W / 2 - DP_BTN / 2 : CX - DP_W / 2 + DP_BTN / 2;
    HaloItem &c = dp_circle[i];
    c.kind = HI_CIRCLE, c.x = cx, c.y = DP_SL_Y, c.r = 40 * p, c.scale = 1, c.color = DP_TRACK, c.on_black = false;
    c.alpha = p > 0.01f ? 1 : 0;
    halo_view_commit(c, v);
    gl_text_mid(dp_icon[i], gl_ic32, i ? "" : "", dp_mid_icon, cx, DP_SL_Y, 1, GL_TEXT, 1, nullptr);
    halo_view_commit(dp_icon[i], v);
  }
  // The bar: the empty part thin (4 dp), the filled one thick (12 dp), round ends; dots between the steps
  const float bx = CX - DP_W / 2 + DP_BTN, bw = DP_W - 2 * DP_BTN, r = halo_clamp01(dp_shown / DP_STEPS), filled = bw * r;
  dp_track.kind = HI_RRECT, dp_track.x = bx + bw / 2, dp_track.y = DP_SL_Y, dp_track.w = bw, dp_track.h = 8, dp_track.r = 4;
  dp_track.scale = 1, dp_track.alpha = 1, dp_track.color = DP_TRACK, dp_track.on_black = false;
  dp_fill.kind = HI_RRECT, dp_fill.x = bx + filled / 2, dp_fill.y = DP_SL_Y, dp_fill.w = filled, dp_fill.h = 24, dp_fill.r = 12;
  dp_fill.scale = 1, dp_fill.alpha = filled > 1 ? 1 : 0, dp_fill.color = DP_PRIMARY, dp_fill.on_black = false;
  halo_view_commit(dp_track, v), halo_view_commit(dp_fill, v);
  for (int i = 1; i < DP_STEPS; i++) {  // at the filled part's round end — no dot: the end itself marks the step
    HaloItem &d = dp_dot[i - 1];
    d.kind = HI_CIRCLE, d.x = bx + i * (bw + 8) / DP_STEPS - 4, d.y = DP_SL_Y, d.r = 4, d.scale = 1;
    d.alpha = halo_clamp01((fabsf(d.x - (bx + filled)) - 6) / 6);
    d.color = d.x < bx + filled ? DP_ON_PRIMARY : DP_OUTLINE, d.on_black = false;
    halo_view_commit(d, v);
  }
}

static void display_draw() {
  halo_item_draw(dp_head), halo_item_draw(dp_label), halo_item_draw(dp_hint);
  halo_item_draw(dp_pill);
  for (int i = 0; i < 2; i++) halo_item_draw(dp_circle[i]), halo_item_draw(dp_icon[i]);
  halo_item_draw(dp_track), halo_item_draw(dp_fill);
  for (auto &d : dp_dot) halo_item_draw(d);
}
#else
#include "settings_rect.h"  // this screen's own places: Settings, variant L3
#endif

// Every frame: the 150 ms catch-up — the brightness glides along it (power.h sends it to the panel)
static void display_tick(float dt) {
  dp_t = fminf(dp_t + dt, 9);
  dp_shown = dp_from + (dp_value - dp_from) * halo_ease(EASE_SHARP, dp_t / 0.15f);
  pw_awake_level = dp_level(dp_shown);
}
#endif

// ---------- Battery ----------
// A round screen (the editor's mockup: variant 7's middle in variant 9's frame): the level as a thin line along the
// bezel; along the arc at the top — what is going on and the time, at the bottom — for how long; in the middle — the
// percent, how long it lasts (or till full) and what it cost since the plug.
#if HAS_BATTERY
#define BH_N 48       // slots: 24 h
#define BH_SLOT 1800  // s per slot
struct BattHistory {   // (as HaloWatch's battery.h: the same NVS key, either firmware reads it)
  uint32_t last;       // the newest slot: unix time / BH_SLOT (0 — none yet)
  uint32_t unplug;     // when USB was unplugged, unix time (0 — on USB or unknown)
  uint8_t unplug_pct;  // the percent then
  uint8_t v[BH_N];     // a ring by slot % BH_N: bits 0..6 — the percent, bit 7 — on USB at some point; 0xFF — no data
};
struct BattPlug {      // ("battery"/"plug": PopkaStudio's own)
  uint32_t at;         // when USB was plugged in, unix time (0 — on battery or unknown)
  uint8_t pct;         // the percent then
};
static BattHistory bh;
static BattPlug bp;
static Preferences bh_prefs;
static bool bh_known, bh_was_usb;

// Made-up states (the command batdemo [battery|charging|low|full|off]; the editor's page — battery): the chip's
// reading, since when on battery or USB, the percent then
struct BattDemo {
  const char *name;
  HaloBattery b;
  uint32_t ago;
  uint8_t from;
};
static const BattDemo BT_DEMO[] = {
    {"battery", {true, true, false, false, 72, 3980}, 3 * 3600 + 20 * 60, 100},
    {"charging", {true, true, true, true, 45, 4050}, 40 * 60, 15},
    {"low", {true, true, false, false, 14, 3620}, 9 * 3600 + 50 * 60, 100},
    {"full", {true, true, true, false, 100, 4180}, 2 * 3600 + 5 * 60, 40},
};
#define BT_DEMO_N (int)(sizeof(BT_DEMO) / sizeof(BT_DEMO[0]))
static int bt_demo = -1;          // the made-up state on show; -1 — the real battery
static volatile int bt_ask = -2;  // batdemo's: -1 — off, 0.. — a state, -2 — nothing asked

static void battery_read() {
  if (bh_prefs.getBytes("hist", &bh, sizeof(bh)) != sizeof(bh)) memset(&bh, 0, sizeof(bh)), memset(bh.v, 0xFF, BH_N);
  if (bh_prefs.getBytes("plug", &bp, sizeof(bp)) != sizeof(bp)) memset(&bp, 0, sizeof(bp));
}

static void battery_demo(int k) {
  if (k < 0) {
    if (bt_demo >= 0) battery_read();
    bt_demo = -1;
    return;
  }
  const BattDemo &d = BT_DEMO[k];
  const uint32_t at = time(nullptr) - d.ago;
  bh.unplug = d.b.usb ? 0 : at, bh.unplug_pct = d.from;
  bp.at = d.b.usb ? at : 0, bp.pct = d.from;
  bt_demo = k;
}

static HaloBattery bt_battery() {  // the chip's, or the made-up state's
  return bt_demo >= 0 ? BT_DEMO[bt_demo].b : halo_battery();
}

// From loop(): every 5 s — this slot's value, when USB came or went; a new slot or another power source — into flash
static void battery_poll() {
  if (bt_ask != -2) battery_demo(bt_ask), bt_ask = -2;
  static uint32_t at;
  if (millis() - at < 5000) return;
  at = millis();
  const HaloBattery b = halo_battery();
  if (!b.ok || !b.present || b.percent < 0 || !halo_clock_valid() || bt_demo >= 0) return;
  const uint32_t now = time(nullptr), slot = now / BH_SLOT;
  bool save = false, psave = false;
  if (slot != bh.last) {
    if (!bh.last || slot < bh.last) memset(bh.v, 0xFF, BH_N);  // the first one, or the clock went back
    else
      for (uint32_t s = bh.last + 1; s < slot && s <= bh.last + BH_N; s++) bh.v[s % BH_N] = 0xFF;  // missed (off)
    bh.last = slot, save = true;
  }
  const uint8_t old = bh.v[slot % BH_N];
  const bool usb = b.usb || (old != 0xFF && (old & 0x80));
  bh.v[slot % BH_N] = (uint8_t)b.percent | (usb ? 0x80 : 0);
  if (bh_known && bh_was_usb && !b.usb) bh.unplug = now, bh.unplug_pct = b.percent, save = true;  // unplugged now
  if (bh_known && !bh_was_usb && b.usb) bp.at = now, bp.pct = b.percent, psave = true;            // plugged in now
  if (b.usb && bh.unplug) bh.unplug = 0, save = true;
  if (!b.usb && bp.at) bp.at = 0, psave = true;
  // Started without knowing when it happened: the history's last slot of the other kind
  if (!bh_known && ((!b.usb && !bh.unplug) || (b.usb && !bp.at)))
    for (uint32_t k = 1; k < BH_N && k <= slot; k++) {
      const uint8_t v = bh.v[(slot - k) % BH_N], nx = bh.v[(slot - k + 1) % BH_N];
      if (v == 0xFF || !(v & 0x80) == !b.usb) continue;
      if (b.usb) bp.at = (slot - k + 1) * BH_SLOT, bp.pct = v & 0x7F, psave = true;  // (the percent as USB came)
      else bh.unplug = (slot - k + 1) * BH_SLOT, bh.unplug_pct = nx != 0xFF ? nx & 0x7F : b.percent, save = true;
      break;
    }
  bh_known = true, bh_was_usb = b.usb;
  if (save) bh_prefs.putBytes("hist", &bh, sizeof(bh));
  if (psave) bh_prefs.putBytes("plug", &bp, sizeof(bp));
}

#define BT_GREEN 0x6DD58C    // the level, charging, full
#define BT_RED 0xFF6B6B      // 20 % and lower
#define BT_PRIMARY 0xA8C7FA  // how long it lasts (Wear OS 6 dark scheme)
#define BT_TRACK 0x2A2B2E    // the rest of the line
#define BT_RING_R 225.0f     // the line along the bezel: its middle, thickness
#define BT_RING_W 6.0f
#define BT_SEGS 8            // the level in 45° pieces: as it grows, only the piece with its end is drawn again
#define BT_WORDS_R 196.0f    // the words along the arc: the middle of their letters
#define BT_PCT_Y 200.0f      // the middle: the percent, the two lines (one line — all 14 px lower)
#define BT_LINE1_Y 262.0f
#define BT_LINE2_Y 292.0f
static uint32_t bt_enter_ms;
static HaloEase EASE_EMPH;
static char bt_pct[8], bt_line1[40], bt_line2[40], bt_top[40], bt_bot[40], bt_top_on[40], bt_bot_on[40];
static uint32_t bt_c1 = GL_DIM;  // the first line's color
static float bt_mid_caps, bt_mid_t24, bt_mid_t20, bt_mid_pct;
static HaloMask bt_top_m, bt_bot_m;  // the words along the arc, baked when they change
static float bt_top_x, bt_top_y, bt_bot_x, bt_bot_y;
static HaloItem bt_track, bt_fill[BT_SEGS], bt_top_i, bt_bot_i, bt_pct_t, bt_line1_t, bt_line2_t;

static void battery_build() {
  bt_mid_caps = halo_text_mid(gl_t20, "ON BATTERY");
  bt_mid_t24 = halo_text_mid(gl_t24, "About 8 h left");
  bt_mid_t20 = halo_text_mid(gl_t20, "On battery");
  bt_mid_pct = halo_text_mid(gl_n92, "0123456789");
  halo_ease_make(EASE_EMPH, 0.05f, 0.7f, 0.1f, 1);  // emphasized decelerate
}

static void battery_enter() {
  bt_enter_ms = halo_ms();
}

// "3 h 20 min", "40 min", "less than a minute"
static void bt_duration(char *out, int n, uint32_t s) {
  const uint32_t m = s / 60;
  if (m < 1) snprintf(out, n, "less than a minute");
  else if (m < 60) snprintf(out, n, "%lu min", (unsigned long)m);
  else snprintf(out, n, "%lu h %lu min", (unsigned long)(m / 60), (unsigned long)(m % 60));
}
// A forecast, rounded down (a promise kept): "45 min" (by 5), "1 h 30 min" (by 10), "8 h", "2 days"
static void bt_about(char *out, int n, float s) {
  const int m = (int)(s / 60);
  if (m < 60) snprintf(out, n, "%d min", max(5, m / 5 * 5));
  else if (m < 120) m % 60 >= 10 ? snprintf(out, n, "1 h %d min", m % 60 / 10 * 10) : snprintf(out, n, "1 h");
  else if (m < 48 * 60) snprintf(out, n, "%d h", m / 60);
  else snprintf(out, n, "%d days", m / (24 * 60));
}
static void bt_upper(char *s) {
  for (; *s; s++) *s = toupper((unsigned char)*s);
}

// Text by the middle of its letters at y; align 0 — x is the left edge, 1 — the middle
static void bt_text(HaloItem &it, const HaloFont &f, const char *s, float mid, float x, float y, uint32_t color,
                    int align, const HaloView &v, float alpha = 1) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = align, it.color = color, it.on_black = true;
  it.on = &halo_black, it.x = x, it.y = y + (f.line / 2.0f - mid), it.scale = 1, it.alpha = alpha, it.spacing = 0;
  halo_view_commit(it, v);
}
// The words along the arc, baked again when they change (bt_tick: ~3 ms each — not in the frame the screen opens)
static void bt_bake(HaloMask &m, char *on, const char *s, bool bottom, float &x, float &y) {
  strcpy(on, s);
  if (!*s || !halo_text_arc(m, gl_t20, s, bt_mid_caps, BT_WORDS_R, bottom ? HALO_PI : 0, bottom, 2, &x, &y))
    free((void *)m.bits), free((void *)m.ext), m = {};
}
static void bt_words(HaloItem &it, const HaloMask &m, float x, float y, const HaloView &v) {
  it.kind = HI_MASK, it.mask = &m, it.color = GL_DIM, it.on_black = true, it.on = &halo_black, it.scale = 1;
  it.x = x + m.w / 2.0f, it.y = y + m.h / 2.0f, it.alpha = m.bits ? 1 : 0;
  halo_view_commit(it, v);
}

// What is going on (the top), for how long (the bottom), the forecast and what it cost (the middle)
static void bt_texts() {
  const HaloBattery b = bt_battery();
  const bool has = b.ok && b.present && b.percent >= 0, clock = bt_demo >= 0 || halo_clock_valid();
  const int pct = has ? b.percent : 0;
  const uint32_t now = time(nullptr);
  const bool low = has && !b.usb && pct <= 20;
  char t[24] = "", d[32] = "", a[24];
  uint32_t c1 = GL_DIM;
  const char *state = "BATTERY";
  bt_line1[0] = bt_line2[0] = 0;
  if (!b.ok) snprintf(bt_line1, sizeof(bt_line1), "Reading...");
  else if (!b.present) state = "NO BATTERY", snprintf(bt_line1, sizeof(bt_line1), "Powered from USB");
  else if (b.usb) {
    state = b.charging ? "CHARGING" : "ON USB";
    const uint32_t s = bp.at && clock && now > bp.at ? now - bp.at : 0;
    const int added = pct - bp.pct;
    if (bp.at && clock) {
      if (s < 60) snprintf(d, sizeof(d), "just plugged in");
      else bt_duration(t, sizeof(t), s), snprintf(d, sizeof(d), "plugged in %s", t);
    }
    if (!b.charging) {
      c1 = pct >= 99 ? BT_GREEN : GL_DIM;
      snprintf(bt_line1, sizeof(bt_line1), pct >= 99 ? "Fully charged" : "Not charging");
    } else if (!bp.at || !clock) {
      c1 = BT_GREEN, snprintf(bt_line1, sizeof(bt_line1), "%.2f V", b.mv / 1000.0f);
    } else {
      c1 = BT_GREEN;
      if (added >= 2 && s >= 300) bt_about(a, sizeof(a), (100 - pct) * s / (float)added), snprintf(bt_line1, sizeof(bt_line1), "Full in about %s", a);
      else snprintf(bt_line1, sizeof(bt_line1), "Estimating...");
      snprintf(bt_line2, sizeof(bt_line2), "%d%% added since plugged in", max(0, added));
    }
  } else {
    state = low ? "LOW BATTERY" : "ON BATTERY";
    if (bh.unplug && clock) {
      const uint32_t s = now > bh.unplug ? now - bh.unplug : 0;
      const int used = bh.unplug_pct - pct;
      if (s < 60) snprintf(d, sizeof(d), "just unplugged");
      else bt_duration(t, sizeof(t), s), snprintf(d, sizeof(d), "%s unplugged", t);
      c1 = low ? BT_RED : BT_PRIMARY;
      if (used >= 2 && s >= 600) bt_about(a, sizeof(a), pct * s / (float)used), snprintf(bt_line1, sizeof(bt_line1), "About %s left", a);
      else snprintf(bt_line1, sizeof(bt_line1), "Estimating...");
      snprintf(bt_line2, sizeof(bt_line2), "%d%% used since unplugged", max(0, used));
    } else {
      snprintf(bt_line1, sizeof(bt_line1), "%.2f V", b.mv / 1000.0f);
    }
  }
  if (clock) {
    time_t tn = now;
    struct tm lt;
    localtime_r(&tn, &lt);
    if (fc_h24) snprintf(t, sizeof(t), "%02d:%02d", lt.tm_hour, lt.tm_min);
    else snprintf(t, sizeof(t), "%d:%02d", lt.tm_hour % 12 ? lt.tm_hour % 12 : 12, lt.tm_min);
    snprintf(bt_top, sizeof(bt_top), "%s · %s", state, t);
  } else {
    snprintf(bt_top, sizeof(bt_top), "%s", state);
  }
  if (!*d && has) snprintf(d, sizeof(d), "%.2f V", b.mv / 1000.0f);  // (nothing known about the plug: the voltage)
  snprintf(bt_bot, sizeof(bt_bot), "%s", d), bt_upper(bt_bot);
  bt_c1 = c1;
}

// Every frame, whatever is on screen (settings_tick): the texts 4 times a second; words that changed — baked, one a
// frame (the minute, the state: the screen opens with them ready)
static void battery_tick() {
  static uint32_t at;
  if (halo_ms() - at >= 250 || !at) at = halo_ms() | 1, bt_texts();
  if (strcmp(bt_top_on, bt_top)) bt_bake(bt_top_m, bt_top_on, bt_top, false, bt_top_x, bt_top_y);
  else if (strcmp(bt_bot_on, bt_bot)) bt_bake(bt_bot_m, bt_bot_on, bt_bot, true, bt_bot_x, bt_bot_y);
}

static void battery_layout(const HaloView &v) {
  const HaloBattery b = bt_battery();
  const bool has = b.ok && b.present && b.percent >= 0;
  const int pct = has ? b.percent : 0;
  const uint32_t lvl = has && !b.usb && pct <= 20 ? BT_RED : BT_GREEN;
  // The percent counts up as the screen opens, the line along the bezel grows along (in pieces: one ring would be the
  // whole screen drawn again every frame — its box is the screen)
  const float k = halo_ease(EASE_EMPH, (halo_ms() - bt_enter_ms) / 700.0f), shown = pct * k;
  bt_track.kind = HI_RING, bt_track.x = bt_track.y = CX, bt_track.r = BT_RING_R, bt_track.w = BT_RING_W;
  bt_track.scale = 1, bt_track.v = 1, bt_track.color = BT_TRACK, bt_track.color2 = 0, bt_track.alpha = 1;
  bt_track.on_black = true;
  halo_view_commit(bt_track, v);
  const float end = 2 * HALO_PI * shown / 100, seg = 2 * HALO_PI / BT_SEGS;
  for (int i = 0; i < BT_SEGS; i++) {
    HaloItem &f = bt_fill[i];
    f.kind = HI_ARC, f.x = f.y = CX, f.r = BT_RING_R, f.w = BT_RING_W, f.scale = 1, f.color = lvl, f.on_black = true;
    f.a0 = i * seg, f.a1 = fminf(end, (i + 1) * seg), f.alpha = end > i * seg + 0.001f ? 1 : 0;
    if (f.a1 < f.a0) f.a1 = f.a0;
    halo_view_commit(f, v);
  }
  if (has) snprintf(bt_pct, sizeof(bt_pct), "%d%%", (int)lroundf(shown));
  else snprintf(bt_pct, sizeof(bt_pct), "--");
  bt_words(bt_top_i, bt_top_m, bt_top_x, bt_top_y, v);
  bt_words(bt_bot_i, bt_bot_m, bt_bot_x, bt_bot_y, v);
  const float dy = bt_line2[0] ? 0 : 14;  // one line: the middle a bit lower
  bt_text(bt_pct_t, gl_n92, bt_pct, bt_mid_pct, CX, BT_PCT_Y + dy, has ? GL_TEXT : GL_DIM, 1, v);
  bt_text(bt_line1_t, gl_t24, bt_line1, bt_mid_t24, CX, BT_LINE1_Y + dy, bt_c1, 1, v);
  bt_text(bt_line2_t, gl_t20, bt_line2, bt_mid_t20, CX, BT_LINE2_Y, GL_DIM, 1, v, bt_line2[0] ? 1 : 0);
}

static void battery_draw() {
  halo_item_draw(bt_track);
  for (auto &it : bt_fill) halo_item_draw(it);
  halo_item_draw(bt_top_i), halo_item_draw(bt_bot_i);
  halo_item_draw(bt_pct_t), halo_item_draw(bt_line1_t), halo_item_draw(bt_line2_t);
}
#endif

// ---------- Music: HaloWatch's player and its music, one to one; the Recorder ----------
#if HAS_MUSIC || HAS_RECORDER || HAS_VIDEO || HAS_RADIO
#include "sound.h"   // one speaker for both: the recorder's playback first, else the music
#endif
#if HAS_MUSIC || HAS_RECORDER || HAS_SHADE || HAS_RADIO
#include "volume.h"  // … and one volume: the player's volume screen, for both (and the quick settings panel's slider)
#endif
#if HAS_RADIO
static void rd_other_sound();  // (radio.h: the Music or the Recorder starts — the radio stops)
#endif
#if HAS_MUSIC
#include "music.h"   // the tracks in the flash's "ffat" partition, the IMA ADPCM decoder, the sound through Halo
#include "player.h"  // the Music screen: WearKit's player on Halo
#endif
#if HAS_RECORDER
#include "recorder.h"  // the Recorder: the microphones, a dotted wave, play it back, forget it
#endif
#if HAS_RADIO
#include "radio.h"  // the Radio: the tuner, Browse (the catalog's folders), the stream (radio_net.h)
#endif
#include "games.h"  // the Games: self-playing snakes (only the ones added)
#if HAS_VIDEO
#include "video.h"  // the Video app: tiles of the card's clips and a player
#endif

// ---------- Both: the layers in face.h's screens, flash, the frame ----------
static bool st_has(const FaceScreen &S, uint8_t kind) {
  for (int i = 0; i < S.n; i++)
    if (S.layers[i].kind == kind && !S.layers[i].hidden) return true;
  return false;
}

// Before halo_begin: the saved brightness (the panel starts at it), the battery's history
static void settings_load() {
#if HAS_VIDEO && !defined(__EMSCRIPTEN__) && FN_WIFI
  hf_tiles(2);  // (Wi-Fi, the card and the JPEG decoder leave the speaker's DMA buffers no room otherwise: measured, boards/p4lcd7/p4-video.md)
#endif
#if HAS_DISPLAY
  dp_prefs.begin("popkawatch", false);
  dp_value = dp_saved = dp_seen = constrain((int)dp_prefs.getUChar("bright", DP_DEFAULT), 0, DP_STEPS);
  dp_shown = dp_from = dp_value, pw_awake_level = DP_LEVEL[dp_value];
#endif
#if HAS_BATTERY
  bh_prefs.begin("battery", false);
  battery_read();
#ifdef __EMSCRIPTEN__
  battery_demo(0);  // (the editor's page: on battery for a while — not the desk's "on USB, full")
#endif
  halo_command("batdemo", [](const char *a) {  // batdemo [battery|charging|low|full|off]; alone — on and off
    while (*a == ' ') a++;
    int k = !*a ? (bt_demo >= 0 ? -1 : 0) : -1;
    for (int i = 0; i < BT_DEMO_N; i++)
      if (!strncmp(a, BT_DEMO[i].name, strlen(BT_DEMO[i].name))) k = i;
    bt_ask = k;
  });
#endif
}

static void settings_build() {  // the fonts' middles (face_build)
  dev_build();
#if HAS_DISPLAY
  display_build();
#endif
#if HAS_MUSIC
  music_begin();  // the tracks — from the flash's partition (tools/halo_music.py): before the player measures them
  player_build();
#endif
#if HAS_RECORDER
  recorder_build();
#endif
#if HAS_RADIO
  radio_build();
#endif
#if HAS_GAMES
  games_build();
#endif
#if HAS_VIDEO
  video_build();
#endif
#if HAS_BATTERY
  battery_build();
#endif
}

// From loop() (a flash write stalls the other core for a moment — not in the middle of a frame): the brightness 0.8 s
// after the last change (not on every step of a drag); the battery's slot
static void settings_poll() {
  dev_poll();
#if HAS_DISPLAY
  const int v = dp_value;
  if (v != dp_seen) dp_seen = v, dp_changed_at = millis();
  if (v != dp_saved && millis() - dp_changed_at > 800) {
    dp_prefs.putUChar("bright", v), dp_saved = v;
    LOG("brightness %d/%d (%d) saved\n", v, DP_STEPS, DP_LEVEL[v]);
  }
#endif
#if HAS_BATTERY
  battery_poll();
#endif
#if HAS_RADIO
  radio_poll();  // the favourites (hearts) into flash
#endif
}

static void settings_tick(float dt) {  // every frame, whatever is on screen
  dev_tick(dt);
#if HAS_DISPLAY
  display_tick(dt);
#endif
#if HAS_BATTERY
  battery_tick();
#endif
#if HAS_MUSIC || HAS_RECORDER || HAS_SHADE || HAS_RADIO
  vol_tick(dt);  // the volume screen and the level at the edge (volume.h)
#endif
#if HAS_MUSIC
  player_tick(dt);  // the music and the player's animations — it plays with the screen closed too
#endif
#if HAS_RECORDER
  recorder_tick(dt);  // the microphones, the wave, the timer; the screen gone — the recording stops
#endif
#if HAS_RADIO
  radio_tick(dt);  // the scale settling, the song's line — it plays with the screen closed too
#endif
#if HAS_GAMES
  games_tick(dt);  // the game on screen plays
#endif
#if HAS_VIDEO
  video_tick(dt);  // the player's clock and animations
#endif
}
// Recording or playing back, a game on screen: not idle (power.h keeps Always On away)
static bool settings_busy() {
  bool busy = false;
#if HAS_RECORDER
  busy |= rc_busy();
#endif
#if HAS_GAMES
  busy |= games_busy();
#endif
#if HAS_VIDEO
  busy |= video_busy();
#endif
  return busy;
}

// A screen with a section: opened, on top and still, laid out, drawn (face.h)
static void settings_enter(const FaceScreen &S) {
#if HAS_DISPLAY
  if (st_has(S, FL_DISPLAY)) display_enter();
#endif
#if HAS_BATTERY
  if (st_has(S, FL_BATTERY)) battery_enter();
#endif
#if HAS_MUSIC
  if (st_has(S, FL_MUSIC)) player_enter();
#endif
#if HAS_RECORDER
  if (st_has(S, FL_RECORDER)) recorder_enter();
#endif
#if HAS_RADIO
  if (st_has(S, FL_RADIO)) radio_enter();
  if (st_has(S, FL_RBROWSE)) rb_enter();
  if (st_has(S, FL_RFOLDER)) rs_enter();
  if (st_has(S, FL_RVOLUME)) rv_enter();
#endif
#if HAS_VIDEO
  if (st_has(S, FL_VIDEO)) video_enter();
#endif
}
static void settings_update(const FaceScreen &S, float dt) {
#if HAS_DISPLAY
  if (st_has(S, FL_DISPLAY)) display_update(dt);
#endif
#if HAS_MUSIC
  if (st_has(S, FL_MUSIC)) player_update(dt);
#endif
#if HAS_RECORDER
  if (st_has(S, FL_RECORDER)) recorder_update(dt);
#endif
#if HAS_RADIO
  if (st_has(S, FL_RADIO)) radio_update(S, dt);
  if (st_has(S, FL_RBROWSE)) rb_update_on(S, dt);
  if (st_has(S, FL_RFOLDER)) rs_update(dt);
  if (st_has(S, FL_RVOLUME)) rv_update(dt);
#endif
#if HAS_VIDEO
  if (st_has(S, FL_VIDEO)) video_update(dt);
#endif
}
static void settings_layout(FaceRun &R, uint8_t kind, const HaloView &v) {
#if HAS_DISPLAY
  if (kind == FL_DISPLAY) display_layout(v);
#endif
#if HAS_BATTERY
  if (kind == FL_BATTERY) battery_layout(v);
#endif
#if HAS_MUSIC
  if (kind == FL_MUSIC) player_layout(v);
#endif
#if HAS_RECORDER
  if (kind == FL_RECORDER) recorder_layout(v);
#endif
#if HAS_RADIO
  if (kind == FL_RADIO) radio_layout(v);
  if (kind == FL_RBROWSE) rb_layout_on(v);
  if (kind == FL_RFOLDER) rs_layout_on(v);
  if (kind == FL_RVOLUME) rv_layout(v);
#endif
#if HAS_VIDEO
  if (kind == FL_VIDEO) video_layout(v);
#endif
#if HAS_GAMES
  if (fl_game(kind)) games_layout(kind, v);
#endif
#ifdef HALO_ROUND
  R.box[0] = R.box[1] = 7, R.box[2] = R.box[3] = W - 7;  // (the editor: the whole screen)
  R.ax = R.ay = CX;
#else
  R.box[0] = R.box[1] = 7, R.box[2] = W - 7, R.box[3] = HALO_H - 7, R.ax = CX, R.ay = CY;
#endif
}
static void settings_draw(uint8_t kind) {
#if HAS_DISPLAY
  if (kind == FL_DISPLAY) display_draw();
#endif
#if HAS_BATTERY
  if (kind == FL_BATTERY) battery_draw();
#endif
#if HAS_MUSIC
  if (kind == FL_MUSIC) player_draw();
#endif
#if HAS_RECORDER
  if (kind == FL_RECORDER) recorder_draw();
#endif
#if HAS_RADIO
  if (kind == FL_RADIO) radio_draw();  // (its parts draw themselves, the volume screen goes over them: face.h)
  if (kind == FL_RBROWSE) rb_draw();
  if (kind == FL_RFOLDER) rs_draw();
  if (kind == FL_RVOLUME) vol_draw();
#endif
#if HAS_VIDEO
  if (kind == FL_VIDEO) video_draw();
#endif
#if HAS_GAMES
  if (fl_game(kind)) games_draw(kind);
#endif
}
