// shade_round.h — the quick settings panel of the round board's home (PopkaStudio: App → Shade, "Pills"): variant
// 23 of web/mockups/round-quick3.html, picked by the user (11 Nov 2026), made after Google's own Wear OS 6 quick
// settings (Material 3 Expressive on the Pixel Watch 3 and 4: Google's launch GIF and 9to5Google's screenshots, 2025).
//
// A layer of the home (FL_SHADE), nothing while closed. A drag down anywhere on the home pulls it out: the panel is a
// black disc the size of the screen sliding down from above under the finger (its lower edge is an arc; the face under
// it dims by 35 %). Let go past a third of the way or flicked faster than 700 px/s — it goes on to the end, else back:
// the rest by time (260 ms open, 220 ms back, scaled by what is left) along Material's standard curve. Up — closes.
//
// On it, as on Wear OS 6: Wear's brightness slider at the top hugging the round edge (a pill: "−", eight segments, "+";
// a drag along the segments, a tap on "−"/"+"), two rows of three big pill buttons — Wi-Fi, Mute, Always On / Screen
// off, Lock, Volume — and a small status chip at the bottom (the battery's percent, Wi-Fi). A button on is filled with
// the accent and its shape glides from a pill to a rounded square. Volume opens HaloWatch's volume screen (volume.h, the
// player's own: "+" at the top, "−" at the bottom, the level along the left edge; a tap elsewhere or 3 s idle — back).
// The colours: the layer's color is the accent (Wear's theme); the buttons' container — the accent deep in black, as in
// Wear's dark scheme; the icons on it — white touched by the accent.
//
// What works — as on the rectangle (shade_rect.h): Brightness — Display's (settings.h: its 8 steps, saved; without
// Display the panel keeps its own under the same flash key); Volume and Mute — the speaker's (volume.h, brought in with
// the panel); Wi-Fi — the project's (a tap: the time and the weather now; without Wi-Fi in the firmware — off); Always On
// — on/off now (not saved); Lock and Screen off — the Buttons' actions (power.h); the battery — the power chip's (read
// from loop() every 5 s).
//
// The editor: a shade layer on any screen but the home is drawn open and still (the tile pictures); the command shade
// open|close shows it on the home (the map, while the layer is selected), shade alone — open or close; shade wifi on
// NAME | off — the project's Wi-Fi (the page sends it).
// Included from face.h on the round board in a firmware with the layer (FN_SHADE), and in that board's page.

#define SH_TRAVEL 466.0f  // the finger's way = the disc's: the whole screen
#define SH_MAX 40         // items in the pool
#define SH_BW 122.0f      // a button: Google's grid — 3 × 122 + 2 × 8, rows of 102
#define SH_BH 102.0f
#define SH_SEGS 8         // the slider's segments (Display's steps)
// What a button is (also a hit's "what"); "−", "+" and the slider itself
enum : uint8_t { SI_WIFI, SI_MUTE, SI_AOD, SI_OFF, SI_LOCK, SI_VOL, SI_N, SI_BMINUS = 50, SI_BPLUS, SL_BRIGHT = 100 };
// Font Awesome (gl_shr_ic30): wifi, volume-xmark, clock, power-off, lock, volume-high; minus, plus. The chip
// (gl_shr_ic20): battery-full, wifi
static const char *const SI_ICON[SI_N] = {"\xEF\x87\xAB", "\xEF\x9A\xA9", "\xEF\x80\x97", "\xEF\x80\x91", "\xEF\x80\xA3",
                                          "\xEF\x80\xA8"};
static const char SH_IC_MINUS[] = "\xEF\x81\xA8", SH_IC_PLUS[] = "\xEF\x81\xA7", SH_IC_BAT[] = "\xEF\x89\x80",
                  SH_IC_WIFI[] = "\xEF\x87\xAB";
extern const HaloFont gl_shr_ic30, gl_shr_ic20, gl_shr_n18;

#define SH_VOL 1  // the speaker's volume (volume.h: settings.h brings it in with the panel)
// Wi-Fi: on the board — the firmware's own (FN_WIFI); in the editor's page — the project's, sent by the page
#ifdef __EMSCRIPTEN__
static bool sh_wifi;
#else
static const bool sh_wifi = FN_WIFI;
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
static bool sh_slider;            // the finger is on the slider
static uint8_t sh_owner;          // (halo_finger_claim)
static bool sh_vol_live, sh_vol_was;  // HaloWatch's volume screen laid out this frame / the last
static float sh_cover_y = -1;     // the rows above it are all under the disc (it is opaque): the face isn't drawn there
static HaloEase sh_ease;
static HaloSpringK K_SH_MORPH;
static HaloSpring sh_on[SI_N];    // a button's shape: 0 — off (a pill), 1 — on (a rounded square in the accent)
static float sh_mid_ic[SI_N + 2], sh_mid_chip[2], sh_mid_n18;
static volatile int sh_cmd = -1;  // shade open|close (1, 0), shade alone (2)
static volatile int sh_bat = -1;  // the battery's percent (−1 — not known)
static char sh_pct[8];
struct ShHit {
  float x0, y0, x1, y1;
  uint8_t what;
};
static ShHit sh_hits[12];
static int sh_hit_n;

// Brightness: Display's steps when the firmware has Display, else the panel's own (the same table and flash key)
#if HAS_DISPLAY
static int sh_bright() { return dp_value; }
static void sh_bright_set(int v) { dp_set(v); }
static void sh_bright_tick(float) {}
static void sh_bright_load() {}
static void sh_bright_save() {}
#else
static const uint8_t SH_LEVEL[SH_SEGS + 1] = {24, 34, 48, 66, 90, 118, 150, 180, 255};  // Display's (settings.h)
static volatile int sh_bv = 7;  // = 180, HaloWatch's
static float sh_bshown = 7, sh_bfrom = 7, sh_bt = 9;
static Preferences sh_prefs;
static int sh_bsaved = -1;
static uint32_t sh_bchanged;
static int sh_bright() { return sh_bv; }
static void sh_bright_set(int v) {
  v = constrain(v, 0, SH_SEGS);
  if (v == sh_bv) return;
  sh_bfrom = sh_bshown, sh_bt = 0, sh_bv = v, sh_bchanged = halo_ms();
}
static void sh_bright_tick(float dt) {  // 150 ms to the new step, as Display's
  sh_bt = fminf(sh_bt + dt, 9);
  const float k = halo_clamp01(sh_bt / 0.15f);
  sh_bshown = sh_bfrom + (sh_bv - sh_bfrom) * (1 - (1 - k) * (1 - k));
  const float s = halo_clamp01(sh_bshown / SH_SEGS) * SH_SEGS;
  const int i = min((int)s, SH_SEGS - 1);
  pw_awake_level = (int)lroundf(SH_LEVEL[i] + (SH_LEVEL[i + 1] - SH_LEVEL[i]) * (s - i));
}
static void sh_bright_load() {  // before halo_begin: the panel starts at it
  sh_prefs.begin("popkawatch", false);
  sh_bv = sh_bsaved = constrain((int)sh_prefs.getUChar("bright", 7), 0, SH_SEGS);
  sh_bshown = sh_bfrom = sh_bv;
  pw_awake_level = SH_LEVEL[sh_bv];
}
static void sh_bright_save() {  // into flash a second after it settles
  if (sh_bv != sh_bsaved && halo_ms() - sh_bchanged > 1000) sh_bsaved = sh_bv, sh_prefs.putUChar("bright", (uint8_t)sh_bv);
}
#endif
static void shade_poll() {  // loop(): the panel's own brightness into flash; the battery every 5 s (the I2C bus — here)
  sh_bright_save();
  static uint32_t last;
  if (sh_bat >= 0 && halo_ms() - last < 5000) return;
  last = halo_ms();
  const HaloBattery b = halo_battery();
  sh_bat = b.ok && b.present ? b.percent : -1;
}

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
static uint32_t sh_on_accent(uint32_t c) {  // the icon on the accent: the accent's own hue, deep
  const int r = c >> 16 & 255, g = c >> 8 & 255, b = c & 255, m = min(r, min(g, b)) * 7 / 10;
  return (uint32_t)((r - m) * 45 / 100) << 16 | (uint32_t)((g - m) * 45 / 100) << 8 | (uint32_t)((b - m) * 45 / 100);
}

// ---------- The pool: items through a move (the disc's) and a fade, then the screen's view ----------
static HaloView sh_v;
static float sh_ty, sh_ta = 1;
static HaloItem *sh_take() {
  if (!sh_it || sh_n >= SH_MAX) return nullptr;
  HaloItem *it = &sh_it[sh_n++];
  it->on = nullptr, it->on_black = false, it->scale = 1, it->align = 1, it->spacing = 0, it->text = nullptr;
  return it;
}
static void sh_commit(HaloItem *it) {
  it->y += sh_ty, it->alpha *= sh_ta;
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
// Text: x by align (0 — left edge, 1 — middle), y — the middle of its letters (mid: the font's)
static void sh_text(const HaloFont &f, float mid, const char *s, float x, float y, uint32_t c, int align = 1, float a = 1) {
  HaloItem *it = sh_take();
  if (!it) return;
  it->kind = HI_TEXT, it->font = &f, it->text = s, it->align = align, it->color = c, it->alpha = s && *s ? a : 0;
  it->x = x, it->y = y + f.line / 2.0f - mid;
  sh_commit(it);
}
static void sh_hit(float x0, float y0, float x1, float y1, uint8_t what, bool live) {
  if (live && sh_hit_n < (int)(sizeof(sh_hits) / sizeof(sh_hits[0]))) sh_hits[sh_hit_n++] = {x0, y0 + sh_ty, x1, y1 + sh_ty, what};
}

// ---------- The parts ----------
static bool sh_muted() {
  return vol_level == 0;
}
static bool sh_state(uint8_t what) {  // a button on
  switch (what) {
    case SI_WIFI: return sh_wifi;
    case SI_AOD: return shade_aod_is_on();
    case SI_MUTE: return sh_muted();
  }
  return false;
}
// A button: a pill off; on — the accent, a rounded square (the shape glides on its spring)
static void sh_button(uint8_t what, float x, float y, uint32_t acc, uint32_t on, uint32_t box, uint32_t ink, bool live) {
  const float s = sh_on[what].x, k = halo_clamp01(s);
  sh_rrect(x, y, SH_BW, SH_BH, SH_BH / 2 + (SH_BH * 0.27f - SH_BH / 2) * s, sh_mix(box, acc, k));
  const int ic = what == SI_VOL && sh_muted() ? SI_MUTE : what;  // (Volume while muted: the crossed speaker)
  sh_text(gl_shr_ic30, sh_mid_ic[ic], SI_ICON[ic], x + SH_BW / 2, y + SH_BH / 2, sh_mix(ink, on, k));
  sh_hit(x, y, x + SH_BW, y + SH_BH, what, live);
}
// Wear OS 6's slider: a pill, "−" and "+" at its ends, the segments between them (lit — the steps up to the value)
static void sh_slider_put(uint32_t acc, uint32_t box, uint32_t ink, bool live) {
  const float x = 73, y = 52, w = 320, h = 78, cy = y + h / 2;
  sh_rrect(x, y, w, h, h / 2, box);
  sh_text(gl_shr_ic30, sh_mid_ic[SI_N], SH_IC_MINUS, x + 36, cy, ink);
  sh_text(gl_shr_ic30, sh_mid_ic[SI_N + 1], SH_IC_PLUS, x + w - 36, cy, ink);
  const float sx = x + 70, sw = w - 140, gap = 4, segw = (sw - gap * (SH_SEGS - 1)) / SH_SEGS;
  const int lit = sh_bright();
  const uint32_t off = sh_mix(box, 0xFFFFFF, 0.13f);
  for (int i = 0; i < SH_SEGS; i++) sh_rrect(sx + i * (segw + gap), cy - 7, segw, 14, 7, i < lit ? acc : off);
  sh_hit(x, y, sx, y + h, SI_BMINUS, live);
  sh_hit(sx + sw, y, x + w, y + h, SI_BPLUS, live);
  sh_hit(sx - 4, y - 8, sx + sw + 4, y + h + 8, SL_BRIGHT, live);
}
// The status chip: the battery's percent and Wi-Fi
static void sh_chip(uint32_t box, uint32_t ink) {
  const bool bat = sh_bat >= 0;
  if (bat) snprintf(sh_pct, sizeof(sh_pct), "%d%%", (int)sh_bat);
  const float cy = 387, bw = bat ? halo_text_width(gl_shr_ic20, SH_IC_BAT) : 0, tw = bat ? halo_text_width(gl_shr_n18, sh_pct) : 0;
  const float ww = sh_wifi ? halo_text_width(gl_shr_ic20, SH_IC_WIFI) : 0;
  const float all = (bat ? bw + 7 + tw : 0) + (bat && sh_wifi ? 10 : 0) + ww;
  sh_rrect(158, cy - 23, 150, 46, 23, box, all > 0 ? 1 : 0);
  float x = CX - all / 2;
  if (bat) {
    sh_text(gl_shr_ic20, sh_mid_chip[0], SH_IC_BAT, x, cy, ink, 0), x += bw + 7;
    sh_text(gl_shr_n18, sh_mid_n18, sh_pct, x, cy, ink, 0), x += tw + 10;
  }
  if (sh_wifi) sh_text(gl_shr_ic20, sh_mid_chip[1], SH_IC_WIFI, x, cy, ink, 0);
}

// ---------- The layer ----------
static int shade_index(const FaceScreen &S) {
  for (int i = 0; i < S.n; i++)
    if (S.layers[i].kind == FL_SHADE && !S.layers[i].hidden) return i;
  return -1;
}
// How far the panel of screen s is out: the home — the live one; any other screen — open (the editor's tile pictures)
static float shade_p(int s) {
  return s == 0 ? halo_clamp01(sh_px / SH_TRAVEL) : 1;
}
// The view the rest of screen s is laid out through: dimmed under the disc
static HaloView shade_under(int s, const FaceScreen &S, const HaloView &v) {
  if (shade_index(S) < 0) return v;
  const float p = shade_p(s);
  if (p <= 0) return v;
  HaloView u = v;
  u.a *= 1 - 0.35f * p;
  return u;
}
static float shade_top_fade(int, const FaceScreen &, int) {
  return 1;
}

// Its items, at the end of screen s's layout
static void shade_layout(int s, const FaceScreen &S, int i, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  const bool live = s == 0;
  const float p = shade_p(s);
  sh_n = 0, sh_v = v, sh_ty = 0, sh_ta = 1;
  if (live) sh_hit_n = 0;
  sh_vol_live = false, sh_cover_y = -1;
  if (p > 0 && v.a > 0) {
#ifdef __EMSCRIPTEN__
    if (sh_bat < 0) sh_bat = halo_battery().percent;  // (the page: the desk board's, full)
#endif
    const uint32_t acc = L.color, on = sh_on_accent(acc), box = sh_mix(0x0D0D0D, acc, 0.21f), ink = sh_mix(0xFFFFFF, acc, 0.08f);
    sh_ty = -(1 - p) * SH_TRAVEL;
    sh_circle(CX, CY, CX + 1, 0x000000);  // the disc: its lower edge is an arc while it comes
    // Where it covers the round screen whole: a row is under it while its chord of the disc holds the screen's chord —
    // with both circles on the same middle line that is every row above (screen + disc)/2 + (rd² − r²) / 2a, a — how
    // far the disc's middle is above the screen's (the slide redraws the whole screen each frame: the face under the
    // disc was half of it)
    if (v.a >= 0.999f && fabsf(v.x) < 0.01f) {
      const float dy = halo_view_y(v, CY + sh_ty), rd = (CX + 1) * v.s, a = CY - dy;
      if (a > 0.5f) sh_cover_y = (CY + dy) / 2 + (rd * rd - CX * CX) / (2 * a);
      else if (a > -0.5f && rd >= CX + 0.5f) sh_cover_y = HALO_H;
    }
    // HaloWatch's volume screen over the panel: the panel's own parts fade through as it grows (volume.h)
    if (live) {
      HaloView vv = v;
      vv.y += sh_ty;
      sh_ta = vol_layout(vv), sh_vol_live = sh_vol_was = true;
    }
    sh_slider_put(acc, box, ink, live);
    static const uint8_t GRID[6] = {SI_WIFI, SI_MUTE, SI_AOD, SI_OFF, SI_LOCK, SI_VOL};
    for (int k = 0; k < 6; k++) sh_button(GRID[k], 42 + (k % 3) * 130, 140 + (k / 3) * 110, acc, on, box, ink, live);
    sh_chip(box, ink);
  } else if (live && sh_vol_was) {  // closed: the volume screen's items away once (their place repainted)
    HaloView hidden = v;
    hidden.a = 0;
    vol_layout(hidden), sh_vol_was = false;
  }
  for (int k = sh_n; k < sh_n_was; k++) sh_it[k].alpha = 0, halo_item_commit(sh_it[k]);
  sh_n_was = sh_n;
  FaceRun &R = S.run[i];
  R.box[0] = 0, R.box[1] = 0, R.box[2] = p > 0 ? HALO_W : -1, R.box[3] = p > 0 ? HALO_H : -1;
  R.ax = CX, R.ay = CY;
}

// face.h, before the face's layers: this band without the rows under the disc (2 px for its soft edge); false — none left
static bool shade_cut_under() {
  if (sh_cover_y < 0) return true;
  const int y = (int)sh_cover_y - 2;
  if (y <= hb.y) return true;
  if (y >= hb.y + hb.rows) return false;
  const int k = y - hb.y;
  hb.buf += k * hb.w, hb.y += k, hb.rows -= k;
  return true;
}

static void shade_draw() {
  for (int k = 0; k < sh_n; k++) halo_item_draw(sh_it[k]);
  if (sh_vol_live) vol_draw();
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
  K_SH_MORPH = halo_spring(0.3f, 0.8f);    // (the shape: a little past and back, as Wear OS 6's morph)
  for (int i = 0; i < SI_N; i++) sh_mid_ic[i] = halo_text_mid(gl_shr_ic30, SI_ICON[i]);
  sh_mid_ic[SI_N] = halo_text_mid(gl_shr_ic30, SH_IC_MINUS), sh_mid_ic[SI_N + 1] = halo_text_mid(gl_shr_ic30, SH_IC_PLUS);
  sh_mid_chip[0] = halo_text_mid(gl_shr_ic20, SH_IC_BAT), sh_mid_chip[1] = halo_text_mid(gl_shr_ic20, SH_IC_WIFI);
  sh_mid_n18 = halo_text_mid(gl_shr_n18, "0");
  for (int i = 0; i < SI_N; i++) halo_spring_set(sh_on[i], sh_state(i) ? 1 : 0);
  sh_bright_load();
  vol_build();  // (the speaker's volume: Mute and the volume screen)
  halo_command("shade", [](const char *a) {  // shade open | close; alone — the other way; shade wifi on NAME | off (the page)
#ifdef __EMSCRIPTEN__
    if (!strncmp(a, "wifi", 4)) {
      a += 4;
      while (*a == ' ') a++;
      sh_wifi = !strncmp(a, "on", 2);
      return;
    }
#endif
    if (!strncmp(a, "look", 4)) return;  // (one look on the round board)
    sh_cmd = !strncmp(a, "open", 4) ? 1 : !strncmp(a, "close", 5) ? 0 : 2;
  });
}

// ---------- Every frame ----------
static void sh_tween(float to, float ms) {
  const float left = fabsf(to - sh_px) / SH_TRAVEL;
  sh_tw_from = sh_px, sh_tw_to = to, sh_tw_t = 0, sh_tw_dur = ms / 1000 * fmaxf(0.45f, left);
  if (to < sh_px) vol_screen.target = 0;  // (closing: the volume screen goes with it)
}
// The tween, the shapes, the brightness — also in the editor's still copies (no finger there)
static void shade_anim(float dt) {
  const int i = shade_index(face_scr[0]);
  if (sh_cmd >= 0) {
    const bool open = sh_cmd == 2 ? sh_px < 1 : sh_cmd == 1;
    if (open && sh_px == 0 && i >= 0) halo_heavy();
    if (i >= 0) sh_tween(open ? SH_TRAVEL : 0, open ? 260 : 220);
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

static void sh_act(uint8_t what) {
  switch (what) {
    case SI_AOD: shade_power(what); break;
    case SI_WIFI: if (sh_wifi) shade_power(what); break;  // (no Wi-Fi in the firmware: the button is off, nothing to do)
    case SI_LOCK: case SI_OFF:
      sh_px = 0, sh_tw_to = -1;
      shade_power(what);
      break;
    case SI_MUTE: {
      static int before = 6;
      if (vol_level > 0) before = vol_level, vol_set(0);
      else vol_set(max(1, before));
      break;
    }
    case SI_VOL: vol_enter(), vol_open(); break;
    case SI_BMINUS: sh_bright_set(sh_bright() - 1); break;
    case SI_BPLUS: sh_bright_set(sh_bright() + 1); break;
  }
}

// The live panel: the finger, before the navigation takes it (a swipe up on the face is the gallery's). awake — the
// screen is up, not going into Always On, not locked
static void shade_tick(float dt, bool awake) {
  const int i = shade_index(face_scr[0]);
  HaloFinger &f = halo_finger;
  const bool here = i >= 0 && awake && halo_nav_on_face() && !halo_nav_over();
  if (!here) {
    if (i < 0 || !awake) {
      if (sh_px > 0) halo_damage_all();
      sh_px = 0, sh_tw_to = -1, vol_screen.target = 0;
    }
    sh_drag = false, sh_slider = false;
    return shade_anim(dt);
  }
  const float T = SH_TRAVEL;
  const bool open = sh_px >= T - 0.5f;
  if (f.pressed && sh_px > 0.5f) halo_finger_claim(&sh_owner);  // out: every touch is the panel's
  if (open && vol_screen_up()) {  // HaloWatch's volume screen over the panel: its "+"/"−", a tap elsewhere — back
    vol_update(dt);
    sh_drag = false, sh_slider = false;
    return shade_anim(dt);
  }
  if (f.pressed && open && f.owner == &sh_owner) {
    sh_slider = false;
    for (int k = 0; k < sh_hit_n; k++) {
      const ShHit &h = sh_hits[k];
      if (h.what == SL_BRIGHT && f.x0 >= h.x0 && f.x0 <= h.x1 && f.y0 >= h.y0 && f.y0 <= h.y1) sh_slider = true;
    }
  }
  if (f.drag_start && !f.horiz && !sh_slider && !sh_drag) {
    const bool mine = f.owner == &sh_owner || (sh_px == 0 && f.y > f.y0 && halo_finger_claim(&sh_owner));
    if (mine) {
      if (sh_px == 0) halo_heavy();  // (the disc leaves zero: the whole screen moves from now on)
      sh_drag = true, sh_px0 = sh_px, sh_y0 = f.y, sh_tw_to = -1;
    }
  }
  if (sh_slider) {  // along the segments: the step under the finger (Display's)
    if (f.dragging || f.tap)
      for (int k = 0; k < sh_hit_n; k++) {
        const ShHit &h = sh_hits[k];
        if (h.what != SL_BRIGHT) continue;
        const float u = halo_clamp01((f.x - h.x0 - 4) / fmaxf(1, h.x1 - h.x0 - 8));
        sh_bright_set((int)ceilf(u * SH_SEGS - 0.15f));  // (the segment under the finger lights up)
      }
    if (!f.down) sh_slider = false;
  } else if (sh_drag) {
    if (f.down && f.owner == &sh_owner) {
      sh_px = halo_clamp01((sh_px0 + f.y - sh_y0) / T) * T;
    } else {  // let go: a flick faster than 700 px/s — where flicked, else past a third of the way from where it started
      sh_drag = false;
      const bool from_open = sh_px0 > T / 2;
      const bool go = f.vy > 700 || (f.vy > -700 && (from_open ? sh_px > T * 2 / 3 : sh_px > T / 3));
      sh_tween(go ? T : 0, go ? 260 : 220);
    }
  } else if (f.released && f.owner == &sh_owner && f.tap && open) {
    for (int k = 0; k < sh_hit_n; k++) {
      const ShHit &h = sh_hits[k];
      if (h.what < SL_BRIGHT && f.x >= h.x0 && f.x <= h.x1 && f.y >= h.y0 && f.y <= h.y1) {
        sh_act(h.what);
        break;
      }
    }
  }
  shade_anim(dt);
}
