// shade_rect.h — the quick settings panel of a rectangular board's home screen (PopkaStudio: App → Shade). A layer of
// the home (FL_SHADE): nothing while closed; a drag down anywhere on the home pulls it out, it follows the finger, let
// go past a third of the way or flicked faster than 700 px/s — it goes on to the end, else back: the rest of the way
// by time (260 ms open, 220 ms back, scaled by what is left) along Material's standard curve — no spring, no bounce
// (the rect board's rule: docs/multi-board.md, "Переходы"). Up — closes; a tap beside the panel — closes too.
//
// Four looks — variants 1, 3, 7 and 8 of web/mockups/rect-quick.html, picked by the user (10 Nov 2026); p2 of the layer:
//   0 Full    — Android 16 QPR1's shade: the whole screen, a big clock, two thick sliders, tiles; a dark veil over the
//               face, its edge follows the finger, the panel's content slides in under it
//   1 Dual    — Android 16's "Separate" shade: a pull on the left half — the "Now" card (clock, music, weather), on the
//               right half — the settings card; the card slides down over the darkened face
//   2 Corner  — ChromeOS / Android desktop mode: a small card grows from the top right corner
//   3 Strip   — one row: brightness, volume and round buttons over the face (it stays where it is); it comes as the
//               gallery comes on this board (halo_nav.cpp, the vertical shared axis): the status line under it fades out
//               over the first 35 % of the way, then the strip comes 40 px from above and fades in; the finger leads
//               280 px for the whole way, the rest 380 ms × what is left; back — the same, mirrored
// The accent — the layer's color. Material 3 Expressive's parts: a tile off is a pill, on — a rounded square in the
// accent (the shape glides); a slider is a thick track, a gap, a thin handle, its icon inside, a stop dot at the end.
//
// What works — the same on the board as in the editor: Brightness — Display's (settings.h: its steps, saved; without
// Display the panel keeps its own, under the same flash key); Volume and Mute — the speaker's (volume.h, brought in with
// the panel); Wi-Fi — the project's: with Wi-Fi in the firmware its network, a tap — sync the time and the weather now,
// without it — Off; Always On — on/off now (not saved: the editor's setting comes back with a restart); Lock and Screen
// off — the Buttons' actions (power.h); the app shortcuts and Music's player — only for the apps this project has (they
// open their screen over the face, as the weather pill does).
//
// No blur (the user's word, 10 Nov 2026): under the panel a dark veil (one translucent plate — the board's SIMD veil),
// the face dims under it.
//
// The editor: a shade layer on any screen but the home is drawn open and still (the tile pictures); the command shade
// open|close shows it on the home (the map, while the layer is selected), shade alone — open or close; shade look N —
// another look now (a check on the board without a Load).
// Included from face.h on a rectangle in a firmware with the layer (FN_SHADE), and in that board's page.

#define SH_TEXT 0xFFF8EE
#define SH_GRAY 0x9A9A9F
#define SH_DATE 0xC9CAD0
#define SH_TILE 0x282C30  // (colors on the 565 levels: R, B ×8, G ×4 — no dither, no tint)
#define SH_MAX 120  // items in the pool
#define SH_STEPS 20 // brightness steps (Display's on a rectangle)
enum : uint8_t { SH_FULL, SH_DUAL, SH_CORNER, SH_STRIP };
static const float SH_TRAVEL[4] = {600, 380, 300, 280};  // the finger's way, px: the whole way open (Strip: the gallery's)
// What a tile or a button is (also the hit's "what"); sliders — SL_*
enum : uint8_t { SI_WIFI, SI_AOD, SI_MUTE, SI_LOCK, SI_OFF, SI_MUSIC, SI_REC, SI_VIDEO, SI_DISPLAY, SI_GEAR, SI_PLAY,
                 SI_N, SL_BRIGHT = 100, SL_VOL };
static const char *const SI_LABEL[SI_N] = {"Wi-Fi", "Always On", "Mute", "Lock", "Screen off", "Music", "Recorder", "Video",
                                           "Display", "Settings", ""};
// Font Awesome (gl_sh_ic26): wifi, clock, volume-xmark, lock, power-off, music, microphone, film, sun, gear, play
static const char *const SI_ICON[SI_N] = {"\xEF\x87\xAB", "\xEF\x80\x97", "\xEF\x9A\xA9", "\xEF\x80\xA3", "\xEF\x80\x91",
                                          "\xEF\x80\x81", "\xEF\x84\xB0", "\xEF\x80\x88", "\xEF\x86\x85", "\xEF\x80\x93",
                                          "\xEF\x81\x8B"};
static const char SH_IC_VOL[] = "\xEF\x80\xA8", SH_IC_SUN[] = "\xEF\x86\x85", SH_IC_PAUSE[] = "\xEF\x81\x8C",
                  SH_IC_NOTE[] = "\xEF\x80\x81";  // (arrays: one address each — sh_ic_index knows them by it)
static const char *const SH_WX_IC[4] = {"\xEF\x9B\x84", "\xEF\x86\x85", "\xEF\x83\x82", "\xEF\x9C\xBD"};  // WX_PARTLY ...
static const char *const SH_DAY[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *const SH_MON[12] = {"January", "February", "March", "April", "May", "June", "July", "August",
                                       "September", "October", "November", "December"};
static const char *const SH_DY[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *const SH_MO[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
extern const HaloFont gl_sh_n64, gl_sh_n40, gl_sh_n32, gl_sh_t22, gl_sh_t20, gl_sh_t18, gl_sh_t16, gl_sh_ic26, gl_sh_ic44;

#define SH_VOL 1  // the speaker's volume (volume.h: settings.h brings it in with the panel, as with Music or the Recorder)
// Wi-Fi: on the board — the firmware's own (FN_WIFI) and its network; in the editor's page — the project's, sent by the
// page (the command shade wifi on NAME | off), so the page shows the tile as this project's board will
#ifdef __EMSCRIPTEN__
static bool sh_wifi;
static char sh_ssid[33];
#else
static const bool sh_wifi = FN_WIFI;
static char sh_ssid[33];
#endif

// power.h's: Always On on/off, the Lock and Screen off actions, Wi-Fi's "now" (defined there)
static bool shade_aod_is_on();
static void shade_power(int what);

static HaloItem *sh_it;           // the pool (PSRAM): laid out bottom to top every frame
static int sh_n, sh_n_was;
static float sh_px;               // the live panel: how far it is out (finger travel, px)
static float sh_px0, sh_y0;       // the drag: where it started
static bool sh_drag;
static float sh_tw_from, sh_tw_to = -1, sh_tw_t, sh_tw_dur;  // the tween after the finger (to < 0 — none)
static int8_t sh_side = 1;        // Dual: 0 — the left card, 1 — the right one
static int sh_slider = -1;        // the slider the finger holds (SL_*), -1 — none
static float sh_slv;              // its value under the finger (0..1)
static uint8_t sh_owner;          // (halo_finger_claim)
static uint8_t sh_look;           // the live layer's look
static float sh_close_in;         // an app opened from the panel: close it when the face is gone
static HaloEase sh_ease;
static HaloSpringK K_SH_MORPH;
static HaloSpring sh_on[SI_N];    // a tile's shape: 0 — off (a pill), 1 — on
static float sh_mid_n64, sh_mid_n40, sh_mid_n32, sh_mid_t22, sh_mid_t20, sh_mid_t18, sh_mid_t16;
static float sh_mid_ic[SI_N + 8];
static volatile int sh_cmd = -1;  // shade open|close (1, 0), shade alone (2)
static char sh_date_l[32], sh_date_s[20], sh_temp[8], sh_hl[24];
static int sh_yday = -1;
struct ShHit {
  float x0, y0, x1, y1;
  uint8_t what;
};
static ShHit sh_hits[24];
static int sh_hit_n;
static float sh_card[4];  // the live panel's own area (a tap outside closes it)
static float sh_clip = HALO_H;  // the panel's own items: only above this row (Full: the veil's edge)

// Brightness: Display's steps when the firmware has Display, else the panel's own (the same table and flash key)
#if HAS_DISPLAY
static int sh_bright() { return dp_value; }
static void sh_bright_set(int v) { dp_set(v); }
static void sh_bright_tick(float) {}
static void sh_bright_load() {}
static void shade_poll() {}
#else
static const uint8_t SH_LEVEL[SH_STEPS + 1] = {10, 11, 13, 15, 18, 22, 27, 33, 40, 48, 58, 69, 81, 95, 110, 127, 146, 167, 190, 221, 255};
static volatile int sh_bv = 14;  // 70 %
static float sh_bshown = 14, sh_bfrom = 14, sh_bt = 9;
static Preferences sh_prefs;
static int sh_bsaved = -1;
static uint32_t sh_bchanged;
static int sh_bright() { return sh_bv; }
static void sh_bright_set(int v) {
  v = constrain(v, 0, SH_STEPS);
  if (v == sh_bv) return;
  sh_bfrom = sh_bshown, sh_bt = 0, sh_bv = v, sh_bchanged = halo_ms();
}
static void sh_bright_tick(float dt) {  // 150 ms to the new step, as Display's
  sh_bt = fminf(sh_bt + dt, 9);
  const float k = halo_clamp01(sh_bt / 0.15f);
  sh_bshown = sh_bfrom + (sh_bv - sh_bfrom) * (1 - (1 - k) * (1 - k));
  const float s = halo_clamp01(sh_bshown / SH_STEPS) * SH_STEPS;
  const int i = min((int)s, SH_STEPS - 1);
  pw_awake_level = (int)lroundf(SH_LEVEL[i] + (SH_LEVEL[i + 1] - SH_LEVEL[i]) * (s - i));
}
static void sh_bright_load() {  // before halo_begin: the panel starts at it
  sh_prefs.begin("popkawatch", false);
  sh_bv = sh_bsaved = constrain((int)sh_prefs.getUChar("bright", 14), 0, SH_STEPS);
  sh_bshown = sh_bfrom = sh_bv;
  pw_awake_level = SH_LEVEL[sh_bv];
}
static void shade_poll() {  // loop(): into flash a second after it settles
  if (sh_bv != sh_bsaved && halo_ms() - sh_bchanged > 1000) sh_bsaved = sh_bv, sh_prefs.putUChar("bright", (uint8_t)sh_bv);
}
#endif

// ---------- Colors ----------
static uint32_t sh_mix(uint32_t a, uint32_t b, float t) {
  t = halo_clamp01(t);
  uint32_t out = 0;
  for (int s = 0; s < 24; s += 8) {
    const float ca = (a >> s) & 255, cb = (b >> s) & 255;
    out |= (uint32_t)lroundf(ca + (cb - ca) * t) << s;
  }
  return out;
}
static uint32_t sh_on_accent(uint32_t c) {  // the text on the accent: the accent's own hue, deep
  const int r = c >> 16 & 255, g = c >> 8 & 255, b = c & 255, m = min(r, min(g, b)) * 7 / 10;
  return (uint32_t)((r - m) * 45 / 100) << 16 | (uint32_t)((g - m) * 45 / 100) << 8 | (uint32_t)((b - m) * 45 / 100);
}

// ---------- The pool: put items through a transform (the group's move, scale and fade) and the screen's view ----------
static HaloView sh_v;  // the screen's view
static float sh_tx, sh_ty, sh_ts = 1, sh_ox, sh_oy, sh_ta = 1;  // the group: moved, scaled from (ox, oy), its alpha
static void sh_group(float tx, float ty, float s = 1, float ox = 0, float oy = 0, float a = 1) {
  sh_tx = tx, sh_ty = ty, sh_ts = s, sh_ox = ox, sh_oy = oy, sh_ta = a;
}
static HaloItem *sh_take() {
  if (!sh_it || sh_n >= SH_MAX) return nullptr;
  HaloItem *it = &sh_it[sh_n++];
  it->on = nullptr, it->on_black = false, it->scale = 1, it->align = 1, it->spacing = 0, it->text = nullptr;
  return it;
}
static void sh_commit(HaloItem *it) {
  it->x = sh_ox + (it->x - sh_ox) * sh_ts + sh_tx, it->y = sh_oy + (it->y - sh_oy) * sh_ts + sh_ty;
  it->scale *= sh_ts, it->alpha *= sh_ta;
  if (sh_ta < 0.999f) halo_subpixel = true;
  halo_view_commit(*it, sh_v);
}
static void sh_rrect(float x, float y, float w, float h, float r, uint32_t c, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_RRECT, it->x = x + w / 2, it->y = y + h / 2, it->w = w, it->h = h, it->r = fminf(r, fminf(w, h) / 2);
  it->color = c, it->alpha = w > 0.5f && h > 0.5f ? a : 0;
  sh_commit(it);
}
static void sh_circle(float cx, float cy, float r, uint32_t c, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_CIRCLE, it->x = cx, it->y = cy, it->r = r, it->color = c, it->alpha = a;
  sh_commit(it);
}
// Text: x by align (0 — left edge, 1 — middle, 2 — right edge), y — the middle of its letters (mid: the font's)
static void sh_text(const HaloFont &f, float mid, const char *s, float x, float y, uint32_t c, int align = 0, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_TEXT, it->font = &f, it->text = s, it->align = align, it->color = c, it->alpha = s && *s ? a : 0;
  it->x = x, it->y = y + f.line / 2.0f - mid;
  sh_commit(it);
}
static int sh_ic_index(const char *g) {  // a glyph's place in sh_mid_ic (-1 — not measured)
  for (int i = 0; i < SI_N; i++)
    if (g == SI_ICON[i]) return i;
  if (g == SH_IC_VOL) return SI_N;
  if (g == SH_IC_PAUSE) return SI_N + 1;
  if (g == SH_IC_SUN) return SI_N + 2;
  if (g == SH_IC_NOTE) return SI_N + 3;
  for (int k = 0; k < 4; k++)
    if (g == SH_WX_IC[k]) return SI_N + 4 + k;
  return -1;
}
// The sun (brightness, a sunny sky): a disc and eight dots round it — Font Awesome's sun reads as a gear at this size
static void sh_sun(float cx, float cy, float k, uint32_t c, float a = 1) {
  sh_circle(cx, cy, 5.6f * k, c, a);
  static const float DX[8] = {0, 0.7071f, 1, 0.7071f, 0, -0.7071f, -1, -0.7071f}, DY[8] = {-1, -0.7071f, 0, 0.7071f, 1, 0.7071f, 0, -0.7071f};
  for (int i = 0; i < 8; i++) sh_circle(cx + DX[i] * 10.5f * k, cy + DY[i] * 10.5f * k, 1.9f * k, c, a);
}
static void sh_icon(const char *g, float cx, float cy, uint32_t c, float a = 1, const HaloFont &f = gl_sh_ic26) {
  if (g == SH_IC_SUN || g == SH_WX_IC[1] || g == SI_ICON[SI_DISPLAY]) return sh_sun(cx, cy, &f == &gl_sh_ic44 ? 1.6f : 1, c, a);
  const int k = &f == &gl_sh_ic26 ? sh_ic_index(g) : -1;
  const float mid = k >= 0 ? sh_mid_ic[k] : halo_text_mid(f, g);
  sh_text(f, mid, g, cx, cy, c, 1, a);
}
static void sh_hit(float x0, float y0, float x1, float y1, uint8_t what, bool live) {
  if (live && sh_hit_n < (int)(sizeof(sh_hits) / sizeof(sh_hits[0]))) {
    // (the hit in screen coordinates: through the group's transform)
    const float ax = sh_ox + (x0 - sh_ox) * sh_ts + sh_tx, ay = sh_oy + (y0 - sh_oy) * sh_ts + sh_ty;
    const float bx = sh_ox + (x1 - sh_ox) * sh_ts + sh_tx, by = sh_oy + (y1 - sh_oy) * sh_ts + sh_ty;
    sh_hits[sh_hit_n++] = {ax, ay, bx, by, what};
  }
}

// ---------- The parts ----------
// A Material 3 Expressive slider: track (inactive) — gap — handle — gap — the active part from the left; its icon inside
static void sh_slider_put(uint8_t id, float x, float y, float w, float h, float v, const char *icon, uint32_t acc,
                          uint32_t on_acc, uint32_t trk, bool live) {
  const float hw = 6, g = 5, R = fminf(h / 2, roundf(h * 0.32f)), r = 4;
  const float hx = x + halo_clamp01(v) * (w - hw), aw = fmaxf(0, hx - g - x), ix = hx + hw + g, iw = fmaxf(0, x + w - ix);
  // the inactive track: round outer corners, small inner ones (two plates)
  sh_rrect(ix, y, iw, h, R, trk);
  sh_rrect(ix, y, fminf(iw, R), h, r, trk);
  sh_rrect(x, y, aw, h, R, acc);
  sh_rrect(x + aw - fminf(aw, R), y, fminf(aw, R), h, r, acc);
  sh_rrect(hx, y - 7, hw, h + 14, 3, acc);
  sh_circle(x + w - 13, y + h / 2, 3, acc, ix < x + w - 30 ? 1 : 0);
  const float s = 26, pad = (h - s) / 2;
  const bool fits = aw >= pad * 2 + s;
  sh_icon(icon, (fits ? x : ix) + pad + s / 2, y + h / 2, fits ? on_acc : acc);
  sh_hit(x, y - 10, x + w, y + h + 10, id, live);
}
static float sh_value(uint8_t id) {
  if (sh_slider == id) return sh_slv;
  if (id == SL_BRIGHT) return sh_bright() / (float)SH_STEPS;
#if SH_VOL
  return vol_level / 10.0f;
#else
  return 0;
#endif
}
static bool sh_state(uint8_t what) {  // a tile on
  switch (what) {
    case SI_WIFI: return sh_wifi;
    case SI_AOD: return shade_aod_is_on();
#if SH_VOL
    case SI_MUTE: return vol_level == 0;
#endif
  }
  return false;
}
static const char *sh_sub(uint8_t what) {  // the line under a tile's label
  switch (what) {
    case SI_WIFI:
#if FN_WIFI && !defined(__EMSCRIPTEN__)
      if (!*sh_ssid) strlcpy(sh_ssid, *hclk_ssid ? hclk_ssid : "On", sizeof(sh_ssid));
#endif
      return sh_wifi ? (*sh_ssid ? sh_ssid : "On") : "Off";
    case SI_AOD: return shade_aod_is_on() ? "On" : "Off";
    case SI_MUTE: return sh_state(SI_MUTE) ? "Muted" : "Sound on";
    case SI_MUSIC: case SI_REC: case SI_VIDEO: case SI_DISPLAY: return "Open";
  }
  return "";
}
// A tile. kind: 0 — wide (icon in a circle at the left, the label, the line), 1 — a pod (the same, smaller), 2 — the
// icon alone, 3 — a round button
static void sh_tile(uint8_t what, int kind, float x, float y, float w, float h, uint32_t acc, uint32_t on_acc, bool live) {
  const float k = sh_on[what].x;
  const uint32_t bg = sh_mix(SH_TILE, acc, k), fg = sh_mix(SH_TEXT, on_acc, k);
  const float r1 = kind == 3 ? h * 0.3f : kind == 0 ? 24 : 22;
  sh_rrect(x, y, w, h, h / 2 + (r1 - h / 2) * k, bg);
  if (kind >= 2) {
    sh_icon(SI_ICON[what], x + w / 2, y + h / 2, fg);
  } else {
    const float c = kind == 0 ? 52 : 44, m = kind == 0 ? 20 : 13, cx = x + m + c / 2, cy = y + h / 2;
    sh_circle(cx, cy, c / 2, sh_mix(0x383C40, sh_mix(acc, 0x000000, 0.09f), k));
    sh_icon(SI_ICON[what], cx, cy, fg);
    const char *sub = sh_sub(what);
    const float tx = x + m + c + 14;
    const HaloFont &f1 = kind == 0 ? gl_sh_t22 : gl_sh_t20, &f2 = kind == 0 ? gl_sh_t18 : gl_sh_t16;
    const float m1 = kind == 0 ? sh_mid_t22 : sh_mid_t20, m2 = kind == 0 ? sh_mid_t18 : sh_mid_t16;
    sh_text(f1, m1, SI_LABEL[what], tx, *sub ? cy - (kind == 0 ? 11 : 10) : cy, fg);
    sh_text(f2, m2, sub, tx, cy + (kind == 0 ? 15 : 13), sh_mix(fg, bg, 0.3f));
  }
  sh_hit(x, y, x + w, y + h, what, live);
}

// The screen an app's tile opens (its section's layer on it), -1 — none
static int sh_app_screen(uint8_t what) {
  const uint8_t kind = what == SI_MUSIC ? FL_MUSIC : what == SI_REC ? FL_RECORDER : what == SI_VIDEO ? FL_VIDEO : FL_DISPLAY;
  for (int s = 1; s < face_scr_n; s++)
    for (int i = 0; i < face_scr[s].n; i++)
      if (face_scr[s].layers[i].kind == kind && !face_scr[s].layers[i].hidden) return s;
  return -1;
}
// What the panel offers, in order: the switches and actions (wide tiles), the apps (small ones). all — every one (the
// editor's tile pictures: no apps there)
static int sh_switches(uint8_t *out, bool all) {
  int n = 0;
  out[n++] = SI_WIFI, out[n++] = SI_AOD, out[n++] = SI_MUTE;
  (void)all;
  out[n++] = SI_LOCK, out[n++] = SI_OFF;
  return n;
}
static int sh_apps(uint8_t *out, bool all) {
  int n = 0;
  for (uint8_t w : {SI_MUSIC, SI_REC, SI_VIDEO, SI_DISPLAY})
    if (sh_app_screen(w) >= 0) out[n++] = w;  // (only the apps this project has: the board has nothing else)
  (void)all;
  return n;
}

// ---------- The four looks (laid out at p: 0 — closed, 1 — open) ----------
static float sh_seg(float e, float a, float b) {
  return halo_clamp01((e - a) / (b - a));
}
static void sh_date_strings() {
  const time_t now = time(nullptr) + fc_offset;
  struct tm t;
  localtime_r(&now, &t);
  if (t.tm_yday == sh_yday) return;
  sh_yday = t.tm_yday;
  snprintf(sh_date_l, sizeof(sh_date_l), "%s, %s %d", SH_DAY[t.tm_wday], SH_MON[t.tm_mon], t.tm_mday);
  snprintf(sh_date_s, sizeof(sh_date_s), "%s, %s %d", SH_DY[t.tm_wday], SH_MO[t.tm_mon], t.tm_mday);
}
// The weather in a row: its icon, the temperature, the sky in words and the high/low under them
static void sh_weather_row(float x, float cy, bool big) {
  const WxDay d = wx_day(0);
  wx_deg(sh_temp, sizeof(sh_temp), d.now);
  char hi[8], lo[8];
  wx_deg(hi, sizeof(hi), d.hi), wx_deg(lo, sizeof(lo), d.lo);
  snprintf(sh_hl, sizeof(sh_hl), "H %s \xC2\xB7 L %s", hi, lo);
  sh_icon(SH_WX_IC[d.kind & 3], x + 22, cy, (d.kind & 3) == 1 ? 0xFFC94D : 0xE8E8EC, 1, gl_sh_ic44);
  sh_text(gl_sh_n40, sh_mid_n40, sh_temp, x + 56, cy, SH_TEXT);
  const float tx = x + 56 + halo_text_width(gl_sh_n40, sh_temp) + 16;
  sh_text(gl_sh_t18, sh_mid_t18, RW_WORD[d.kind & 3], tx, cy - 11, SH_GRAY);
  sh_text(gl_sh_t18, sh_mid_t18, sh_hl, tx, cy + 13, SH_GRAY);
  (void)big;
}
#if HAS_MUSIC
static float sh_music_prog() {
  if (!mu_n || mu_cur != pl_cur || !mu_tracks[pl_cur].samples) return 0;
  return halo_clamp01((float)mu_pos / mu_tracks[pl_cur].samples);
}
#endif
static void sh_status(float right, float cy) {  // the weather and Wi-Fi at the top right
  const WxDay d = wx_day(0);
  wx_deg(sh_temp, sizeof(sh_temp), d.now);
  float x = right;
  if (sh_wifi) sh_icon(SI_ICON[SI_WIFI], x - 15, cy, SH_TEXT), x -= 30 + 22;
  sh_text(gl_sh_n32, sh_mid_n32, sh_temp, x, cy, SH_TEXT, 2);
  x -= halo_text_width(gl_sh_n32, sh_temp) + 10;
  sh_icon(SH_WX_IC[d.kind & 3], x - 16, cy, (d.kind & 3) == 1 ? 0xFFC94D : 0xE8E8EC);
}

static void sh_full(float p, uint32_t acc, uint32_t on_acc, uint32_t trk, bool live, bool all) {
  const float ty = -(1 - p) * 60, a = sh_seg(p, 0.08f, 0.6f);
  sh_group(0, ty, 1, 0, 0, a);
  sh_text(gl_sh_n64, sh_mid_n64, fc_time, 48, 62, SH_TEXT);
  sh_text(gl_sh_t22, sh_mid_t22, sh_date_l, 52, 117, SH_DATE);
  sh_status(976, 62);
  const bool vol = true;
  sh_slider_put(SL_BRIGHT, 48, 150, vol ? 454 : 928, 64, sh_value(SL_BRIGHT), SH_IC_SUN, acc, on_acc, trk, live);
  if (vol) sh_slider_put(SL_VOL, 522, 150, 454, 64, sh_value(SL_VOL), SH_IC_VOL, acc, on_acc, trk, live);
  uint8_t sw[6], ap[4];
  const int ns = sh_switches(sw, all), na = sh_apps(ap, all);
  // four wide tiles in the first row; in the second the rest of them, then the apps' small ones, as they come
  float x = 48;
  for (int i = 0; i < ns; i++) {
    if (i == 4) x = 48;
    sh_tile(sw[i], 0, x, i < 4 ? 242 : 350, 223, 96, acc, on_acc, live), x += 235;
  }
  if (ns <= 4) x = 48;
  for (int i = 0; i < na && x + 104 <= 900; i++) sh_tile(ap[i], 2, x, 350, 104, 96, acc, on_acc, live), x += 116;
  if (sh_app_screen(SI_DISPLAY) >= 0) sh_tile(SI_GEAR, 3, 916, 496, 60, 60, acc, on_acc, live);
  sh_rrect(486, 578, 52, 5, 2.5f, 0x5D6067);
  sh_card[0] = 0, sh_card[1] = 0, sh_card[2] = HALO_W, sh_card[3] = HALO_H;
}

static void sh_dual(float p, uint32_t acc, uint32_t on_acc, uint32_t trk, bool live, bool all) {
  const uint32_t card = 0x202428;
#if HAS_MUSIC
  const bool music = mu_n > 0 && sh_app_screen(SI_MUSIC) >= 0;  // (the Music app in this project: the board has its player)
#else
  const bool music = false;
#endif
  const float LH = music ? 352 : 252, RH = 528;
  if (sh_side == 0) {
    sh_group(0, -(1 - p) * (LH + 40));
    sh_rrect(16, 16, 440, LH, 32, card);
    sh_text(gl_sh_n64, sh_mid_n64, fc_time, 44, 72, SH_TEXT);
    sh_text(gl_sh_t22, sh_mid_t22, sh_date_l, 48, 125, SH_GRAY);
#if HAS_MUSIC
    if (music) {
      const MuTrack &t = mu_tracks[pl_cur];
      sh_rrect(40, 158, 96, 96, 18, 0x5F4A96);
      sh_icon(SH_IC_NOTE, 88, 206, 0xE9DCF7, 1, gl_sh_ic44);
      sh_text(gl_sh_t22, sh_mid_t22, t.title, 154, 181, SH_TEXT);
      sh_text(gl_sh_t18, sh_mid_t18, t.artist, 156, 211, SH_GRAY);
      sh_rrect(156, 235, 196, 6, 3, 0x45464C);
      sh_rrect(156, 235, 196 * sh_music_prog(), 6, 3, 0xE9DCF7);
      sh_circle(403, 205, 29, acc);
      sh_icon(pl_playing ? SH_IC_PAUSE : SI_ICON[SI_PLAY], 403, 205, on_acc);
      sh_hit(374, 176, 432, 234, SI_PLAY, live);
    }
#endif
    sh_weather_row(40, music ? 314 : 192, true);
    sh_card[0] = 16, sh_card[1] = 16, sh_card[2] = 456, sh_card[3] = 16 + LH;
  } else {
    sh_group(0, -(1 - p) * (RH + 40));
    sh_rrect(568, 16, 440, RH, 32, card);
    sh_text(gl_sh_n32, sh_mid_n32, fc_time, 596, 57, SH_TEXT);
    sh_text(gl_sh_t20, sh_mid_t20, sh_date_s, 596 + halo_text_width(gl_sh_n32, fc_time) + 14, 57, SH_GRAY);
    if (sh_app_screen(SI_DISPLAY) >= 0) sh_tile(SI_GEAR, 3, 936, 32, 52, 52, acc, on_acc, live);
    const bool vol = true;
    sh_slider_put(SL_BRIGHT, 592, 104, 392, 56, sh_value(SL_BRIGHT), SH_IC_SUN, acc, on_acc, trk, live);
    if (vol) sh_slider_put(SL_VOL, 592, 172, 392, 56, sh_value(SL_VOL), SH_IC_VOL, acc, on_acc, trk, live);
    uint8_t it[10];
    int n = sh_switches(it, all);
    n += sh_apps(it + n, all);
    for (int i = 0; i < min(n, 6); i++) sh_tile(it[i], 1, 592 + (i & 1) * 204, 248 + (i >> 1) * 96, 188, 84, acc, on_acc, live);
    sh_card[0] = 568, sh_card[1] = 16, sh_card[2] = 1008, sh_card[3] = 16 + RH;
  }
}

static void sh_corner(float p, uint32_t acc, uint32_t on_acc, uint32_t trk, bool live, bool all) {
#if HAS_MUSIC
  const bool music = mu_n > 0 && sh_app_screen(SI_MUSIC) >= 0;  // (the Music app in this project: the board has its player)
#else
  const bool music = false;
#endif
  const float H = music ? 496 : 410;
  sh_group(0, -(1 - p) * 24, 0.86f + 0.14f * p, 1010, 14, halo_clamp01(p * 1.7f));
  sh_rrect(590, 14, 420, H, 30, 0x202428);
  sh_text(gl_sh_n32, sh_mid_n32, fc_time, 614, 55, SH_TEXT);
  sh_text(gl_sh_t18, sh_mid_t18, sh_date_s, 614 + halo_text_width(gl_sh_n32, fc_time) + 12, 55, SH_GRAY);
  const bool gear = sh_app_screen(SI_DISPLAY) >= 0;
  sh_tile(SI_LOCK, 3, gear ? 838 : 894, 32, 48, 48, acc, on_acc, live);
  sh_tile(SI_OFF, 3, gear ? 894 : 950, 32, 48, 48, acc, on_acc, live);
  if (gear) sh_tile(SI_GEAR, 3, 950, 32, 48, 48, acc, on_acc, live);
  uint8_t it[10];
  int n = 0;
  it[n++] = SI_WIFI, it[n++] = SI_AOD, it[n++] = SI_MUTE;
  n += sh_apps(it + n, all);
  for (int i = 0; i < min(n, 4); i++) sh_tile(it[i], 1, 614 + (i & 1) * 192, 100 + (i >> 1) * 84, 180, 72, acc, on_acc, live);
  sh_slider_put(SL_VOL, 614, 280, 372, 52, sh_value(SL_VOL), SH_IC_VOL, acc, on_acc, trk, live);
  sh_slider_put(SL_BRIGHT, 614, 348, 372, 52, sh_value(SL_BRIGHT), SH_IC_SUN, acc, on_acc, trk, live);
#if HAS_MUSIC
  if (music) {
    const MuTrack &t = mu_tracks[pl_cur];
    sh_rrect(614, 418, 372, 76, 22, 0x282C30);
    sh_rrect(626, 430, 52, 52, 12, 0x5F4A96);
    sh_icon(SH_IC_NOTE, 652, 456, 0xE9DCF7);
    sh_text(gl_sh_t18, sh_mid_t18, t.title, 692, 446, SH_TEXT);
    sh_text(gl_sh_t16, sh_mid_t16, t.artist, 693, 469, SH_GRAY);
    sh_circle(946, 456, 26, acc);
    sh_icon(pl_playing ? SH_IC_PAUSE : SI_ICON[SI_PLAY], 946, 456, on_acc);
    sh_hit(920, 430, 972, 482, SI_PLAY, live);
  }
#endif
  sh_card[0] = 590, sh_card[1] = 14, sh_card[2] = 1010, sh_card[3] = 14 + H;
}

static void sh_strip(float p, uint32_t acc, uint32_t on_acc, uint32_t trk, bool live, bool all) {
  // As the gallery's shared axis: what stands under it fades out over the first 35 % of the way (shade_top_fade), then
  // the strip comes 40 px and fades in — never two half-seen things on top of each other
  const float b = sh_seg(p, 0.35f, 1);
  sh_group(0, -(1 - b) * 40, 1, 0, 0, b);
  sh_rrect(0, -28, HALO_W, 164, 28, 0x181C20);
  const bool vol = true;
  sh_slider_put(SL_BRIGHT, 28, 36, vol ? 280 : 576, 64, sh_value(SL_BRIGHT), SH_IC_SUN, acc, on_acc, trk, live);
  if (vol) sh_slider_put(SL_VOL, 324, 36, 280, 64, sh_value(SL_VOL), SH_IC_VOL, acc, on_acc, trk, live);
  uint8_t it[6];
  int n = 0;
  it[n++] = SI_WIFI, it[n++] = SI_AOD, it[n++] = SI_LOCK;
  if (vol) it[n++] = SI_MUTE;
  it[n++] = SI_OFF;
  for (int i = 0; i < n; i++) sh_tile(it[i], 3, 632 + i * 74, 36, 64, 64, acc, on_acc, live);
  sh_rrect(486, 121, 52, 5, 2.5f, 0x5D6067);
  sh_card[0] = 0, sh_card[1] = 0, sh_card[2] = HALO_W, sh_card[3] = 136;
}

// ---------- The layer ----------
static int shade_index(const FaceScreen &S) {
  for (int i = 0; i < S.n; i++)
    if (S.layers[i].kind == FL_SHADE && !S.layers[i].hidden) return i;
  return -1;
}
static uint8_t sh_look_of(const FaceLayer &L) {
  return (uint8_t)constrain((int)lroundf(L.p2), 0, 3);
}
// How far the panel of screen s is out: the home — the live one; any other screen — open (the editor's tile pictures)
static float shade_p(int s, uint8_t look) {
  return s == 0 ? halo_clamp01(sh_px / SH_TRAVEL[look]) : 1;
}
static bool sh_veils(uint8_t look) {  // a veil over the whole face, its edge following the finger
  return look == SH_FULL || look == SH_DUAL;
}

// The view the rest of screen s is laid out through: dimmed (and pushed down by the strip)
static HaloView shade_under(int s, const FaceScreen &S, const HaloView &v) {
  const int i = shade_index(S);
  if (i < 0) return v;
  const uint8_t look = sh_look_of(S.layers[i]);
  const float p = shade_p(s, look);
  if (p <= 0) return v;
  HaloView u = v;
  static const float DIM[4] = {0.5f, 0.35f, 0.3f, 0};  // (the strip: the face as it is)
  u.a *= 1 - DIM[look] * p;
  return u;
}

// The strip: how much is left of a layer of the face that the strip covers — those go over the first 35 % of the way,
// as the face goes before the gallery comes; 1 — no strip, or a layer not under it. Which are under it: on the home —
// taken when it leaves zero (sh_mask_take: where each was drawn then, backgrounds aside); in the editor's still pictures
// — the status line by its place
static uint32_t sh_fade_mask;
#define SH_STRIP_H 136.0f
static void sh_mask_take() {
  sh_fade_mask = 0;
  const FaceScreen &S = face_scr[0];
  for (int i = 0; i < S.n && i < 32; i++) {
    const FaceLayer &L = S.layers[i];
    const float *b = S.run[i].box;
    if (L.kind == FL_SHADE || L.kind == FL_STARS || fl_pixels(L.kind) || b[2] < b[0]) continue;
    if (b[1] < SH_STRIP_H + 4 && b[3] - b[1] < HALO_H - 40) sh_fade_mask |= 1u << i;  // (a whole-screen one — a background: stays)
  }
}
static float shade_top_fade(int s, const FaceScreen &S, int i) {
  const int k = shade_index(S);
  if (k < 0 || sh_look_of(S.layers[k]) != SH_STRIP) return 1;
  const FaceLayer &L = S.layers[i];
  const bool under = s == 0 ? i < 32 && (sh_fade_mask >> i & 1)
                            : (L.kind == FL_DATE || L.kind == FL_WEATHER || L.kind == FL_CLOCK || L.kind == FL_STEPS) &&
                                  face_base_y(L) + L.dy <= SH_STRIP_H;
  return under ? 1 - sh_seg(shade_p(s, SH_STRIP), 0, 0.35f) : 1;
}

// Its items, at the end of screen s's layout
static void shade_layout(int s, const FaceScreen &S, int i, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  const uint8_t look = sh_look_of(L);
  const bool live = s == 0, all = s != 0;
  const float p = shade_p(s, look);
  sh_n = 0, sh_v = v, sh_group(0, 0);
  if (live) sh_hit_n = 0, sh_look = look;
  sh_clip = HALO_H;
  if (p > 0 && v.a > 0) {
    sh_date_strings();
    const uint32_t acc = L.color, on_acc = sh_on_accent(acc), trk = sh_mix(0x202428, acc, 0.18f);
    if (sh_veils(look)) {
      const float e = p * HALO_H;
      if (look == SH_FULL) sh_clip = e;
      sh_group(0, 0);
      sh_rrect(0, -28, HALO_W, e + 28, 28, 0x000000, look == SH_FULL ? 0.88f : 0.45f);  // (Full: the face's own date and clock
      // must not show through the panel's header)
    }
    if (look == SH_FULL) sh_full(p, acc, on_acc, trk, live, all);
    else if (look == SH_DUAL) sh_dual(p, acc, on_acc, trk, live, all);
    else if (look == SH_CORNER) sh_corner(p, acc, on_acc, trk, live, all);
    else sh_strip(p, acc, on_acc, trk, live, all);
  }
  for (int k = sh_n; k < sh_n_was; k++) sh_it[k].alpha = 0, halo_item_commit(sh_it[k]);
  sh_n_was = sh_n;
  FaceRun &R = S.run[i];
  R.box[0] = 0, R.box[1] = 0, R.box[2] = p > 0 ? HALO_W : -1, R.box[3] = p > 0 ? HALO_H : -1;
  R.ax = CX, R.ay = CY;
}

static bool shade_cut_under() {  // (face.h: the rows a panel covers whole — a rectangle's panels are not opaque discs)
  return true;
}

static void shade_draw() {
  const HaloBand band = hb;
  if (sh_clip < hb.y + hb.rows) {  // (Full: the panel's content only above the sheet's edge)
    hb.rows = max(0, (int)sh_clip - hb.y);
    if (hb.rows <= 0) return void(hb = band);
  }
  for (int k = 0; k < sh_n; k++) halo_item_draw(sh_it[k]);
  hb = band;
}

static void shade_layer_build() {  // (face_build: once)
  static bool built;
  if (built) return;
  built = true;
#ifdef __EMSCRIPTEN__
  sh_it = new HaloItem[SH_MAX]();
#else
  sh_it = (HaloItem *)heap_caps_calloc(SH_MAX, sizeof(HaloItem), MALLOC_CAP_SPIRAM);
  if (!sh_it) sh_it = (HaloItem *)calloc(SH_MAX, sizeof(HaloItem));
  for (int i = 0; i < SH_MAX; i++) new (sh_it + i) HaloItem();
#endif
  halo_ease_make(sh_ease, 0.2f, 0, 0, 1);  // Material standard
  K_SH_MORPH = halo_spring(0.3f, 1);
  sh_mid_n64 = halo_text_mid(gl_sh_n64, "0"), sh_mid_n40 = halo_text_mid(gl_sh_n40, "0"), sh_mid_n32 = halo_text_mid(gl_sh_n32, "0");
  sh_mid_t22 = halo_text_mid(gl_sh_t22, "H"), sh_mid_t20 = halo_text_mid(gl_sh_t20, "H"), sh_mid_t18 = halo_text_mid(gl_sh_t18, "H");
  sh_mid_t16 = halo_text_mid(gl_sh_t16, "H");
  for (int i = 0; i < SI_N; i++) sh_mid_ic[i] = halo_text_mid(gl_sh_ic26, SI_ICON[i]);
  sh_mid_ic[SI_N] = halo_text_mid(gl_sh_ic26, SH_IC_VOL), sh_mid_ic[SI_N + 1] = halo_text_mid(gl_sh_ic26, SH_IC_PAUSE);
  sh_mid_ic[SI_N + 2] = halo_text_mid(gl_sh_ic26, SH_IC_SUN), sh_mid_ic[SI_N + 3] = halo_text_mid(gl_sh_ic26, SH_IC_NOTE);
  for (int k = 0; k < 4; k++) sh_mid_ic[SI_N + 4 + k] = halo_text_mid(gl_sh_ic26, SH_WX_IC[k]);
  for (int i = 0; i < SI_N; i++) halo_spring_set(sh_on[i], sh_state(i) ? 1 : 0);
  sh_bright_load();
  vol_build();  // (the speaker's volume: the slider and Mute)
  halo_command("shade", [](const char *a) {  // shade open | close; alone — the other way; shade wifi on NAME | off (the page)
#ifdef __EMSCRIPTEN__
    if (!strncmp(a, "wifi", 4)) {
      a += 4;
      while (*a == ' ') a++;
      sh_wifi = !strncmp(a, "on", 2);
      a += sh_wifi ? 2 : 0;
      while (*a == ' ') a++;
      strlcpy(sh_ssid, sh_wifi ? a : "", sizeof(sh_ssid));
      return;
    }
#endif
    if (!strncmp(a, "look", 4)) {  // shade look N — another look now (0..3; checks without a Load)
      const int k = shade_index(face_scr[0]);
      if (k >= 0) face_scr[0].layers[k].p2 = (float)constrain(atoi(a + 4), 0, 3), halo_damage_all();
      return;
    }
    sh_cmd = !strncmp(a, "open", 4) ? 1 : !strncmp(a, "close", 5) ? 0 : 2;
  });
}

// ---------- Every frame ----------
static void sh_tween(float to, float ms) {
  const float T = SH_TRAVEL[sh_look], left = fabsf(to - sh_px) / T;
  sh_tw_from = sh_px, sh_tw_to = to, sh_tw_t = 0;
  // (the strip as the gallery: 380 ms the whole way both ways, the rest — its share of it; halo_nav.cpp SH_S)
  sh_tw_dur = sh_look == SH_STRIP ? 0.38f * fmaxf(0.1f, left) : ms / 1000 * fmaxf(0.45f, left);
}
static void sh_start() {  // the panel leaves zero: the whole screen moves from now on (the strip: what it covers)
  if (sh_look == SH_STRIP) sh_mask_take();
  halo_heavy();
}
// The tween, the shapes, the brightness — also in the editor's still copies (no finger there)
static void shade_anim(float dt) {
  static uint8_t was = 255;
  const int i = shade_index(face_scr[0]);
  if (i >= 0) sh_look = sh_look_of(face_scr[0].layers[i]);
  if (was != 255 && was != sh_look && sh_px > 0) {  // another look while out (the editor): as far out on its own way
    sh_px = sh_px / SH_TRAVEL[was] * SH_TRAVEL[sh_look];
    if (sh_tw_to > 0) sh_tw_to = SH_TRAVEL[sh_look], sh_tw_from = sh_px;
    if (sh_look == SH_STRIP) sh_mask_take();
    halo_damage_all();
  }
  was = sh_look;
  if (sh_cmd >= 0) {
    const bool open = sh_cmd == 2 ? sh_px < 1 : sh_cmd == 1;
    if (open && sh_px == 0 && i >= 0) sh_start();
    if (i >= 0) sh_tween(open ? SH_TRAVEL[sh_look] : 0, open ? 260 : 220);
    sh_cmd = -1;
  }
  if (i < 0) sh_px = 0, sh_tw_to = -1;
  if (sh_tw_to >= 0 && !sh_drag) {
    sh_tw_t += dt;
    const float k = sh_tw_dur > 0 ? halo_clamp01(sh_tw_t / sh_tw_dur) : 1;
    sh_px = sh_tw_from + (sh_tw_to - sh_tw_from) * halo_ease(sh_ease, k);
    if (k >= 1) sh_px = sh_tw_to, sh_tw_to = -1;
  }
  for (int w = 0; w < SI_N; w++) {
    sh_on[w].target = sh_state(w) ? 1 : 0;
    halo_spring_run(sh_on[w], K_SH_MORPH, dt);
  }
  sh_bright_tick(dt);
}

static void sh_act(uint8_t what, const ShHit &h) {
  switch (what) {
    case SI_AOD: shade_power(what); break;
    case SI_WIFI: if (sh_wifi) shade_power(what); break;  // (no Wi-Fi in the firmware: the tile says Off, nothing to do)
    case SI_LOCK: case SI_OFF:
      sh_px = 0, sh_tw_to = -1;
      shade_power(what);
      break;
#if SH_VOL
    case SI_MUTE: {
      static int before = 6;
      if (vol_level > 0) before = vol_level, vol_set(0);
      else vol_set(max(1, before));
      break;
    }
#endif
#if HAS_MUSIC
    case SI_PLAY: pl_set_playing(!pl_playing); break;
#endif
    case SI_MUSIC: case SI_REC: case SI_VIDEO: case SI_DISPLAY: case SI_GEAR: {
      const int s = sh_app_screen(what == SI_GEAR ? SI_DISPLAY : what);
      if (s < 0) break;
      face_over = s;
      halo_nav_face_open(OVER, HALO_FX_FADE, (h.x0 + h.x1) / 2, (h.y0 + h.y1) / 2, h.x1 - h.x0, h.y1 - h.y0);
      sh_close_in = 0.5f;  // (the panel fades away with the face; closed for when the face comes back)
      break;
    }
  }
}

// The live panel: the finger, before the navigation takes it (a swipe up on the face is the gallery's). awake — the
// screen is up, not going into Always On, not locked
static void shade_tick(float dt, bool awake) {
  const int i = shade_index(face_scr[0]);
  HaloFinger &f = halo_finger;
  if (sh_close_in > 0 && (sh_close_in -= dt) <= 0) sh_px = 0, sh_tw_to = -1;
  const bool here = i >= 0 && awake && halo_nav_on_face() && !halo_nav_over();
  if (!here) {
    if (i < 0 || !awake) {
      if (sh_px > 0) halo_damage_all();
      sh_px = 0, sh_tw_to = -1;
    }
    sh_drag = false, sh_slider = -1;
    return shade_anim(dt);
  }
  sh_look = sh_look_of(face_scr[0].layers[i]);
  const float T = SH_TRAVEL[sh_look];
  if (f.pressed && sh_px > 0.5f && halo_finger_claim(&sh_owner)) {  // out: every touch is the panel's
    sh_slider = -1;
    if (sh_px >= T - 0.5f)
      for (int k = 0; k < sh_hit_n; k++) {
        const ShHit &h = sh_hits[k];
        if (h.what >= SL_BRIGHT && f.x0 >= h.x0 && f.x0 <= h.x1 && f.y0 >= h.y0 && f.y0 <= h.y1) sh_slider = h.what;
      }
  }
  if (f.drag_start && !f.horiz && sh_slider < 0 && !sh_drag) {
    const bool mine = f.owner == &sh_owner || (sh_px == 0 && f.y > f.y0 && halo_finger_claim(&sh_owner));
    if (mine) {
      if (sh_px == 0) sh_side = f.x0 < CX ? 0 : 1, sh_start();
      sh_drag = true, sh_px0 = sh_px, sh_y0 = f.y, sh_tw_to = -1;
    }
  }
  if (sh_slider >= 0) {
    for (int k = 0; k < sh_hit_n; k++) {
      const ShHit &h = sh_hits[k];
      if (h.what != sh_slider) continue;
      sh_slv = halo_clamp01((f.x - h.x0) / fmaxf(1, h.x1 - h.x0));
      if (sh_slider == SL_BRIGHT) sh_bright_set((int)lroundf(sh_slv * SH_STEPS));
#if SH_VOL
      else if ((int)lroundf(sh_slv * 10) != vol_level) vol_set((int)lroundf(sh_slv * 10));
#endif
    }
    if (!f.down) sh_slider = -1;
  } else if (sh_drag) {
    if (f.down && f.owner == &sh_owner) {
      sh_px = halo_clamp01((sh_px0 + f.y - sh_y0) / T) * T;
    } else {  // let go: a flick faster than 700 px/s — where flicked, else past a third of the way from where it started
      sh_drag = false;
      const bool from_open = sh_px0 > T / 2;
      const bool open = f.vy > 700 || (f.vy > -700 && (from_open ? sh_px > T * 2 / 3 : sh_px > T / 3));
      sh_tween(open ? T : 0, open ? 260 : 220);
    }
  } else if (f.released && f.owner == &sh_owner && f.tap && sh_px >= T - 0.5f) {
    const ShHit *hit = nullptr;
    for (int k = 0; k < sh_hit_n; k++) {
      const ShHit &h = sh_hits[k];
      if (h.what < SL_BRIGHT && f.x >= h.x0 && f.x <= h.x1 && f.y >= h.y0 && f.y <= h.y1) hit = &h;
    }
    if (hit) sh_act(hit->what, *hit);
    else if (f.x < sh_card[0] || f.x > sh_card[2] || f.y < sh_card[1] || f.y > sh_card[3]) sh_tween(0, 220);  // beside it
  }
  shade_anim(dt);
}
