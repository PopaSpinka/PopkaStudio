// wxwidgets.h — seven weather widgets for the face, the variants the user kept of web/mockups/weather-widgets.html
// (14 Nov 2026), a layer kind each (Layers → Face), the round board's:
//   Digits     (FL_WXDIGITS) the temperature huge, the sky moving inside its digits — sun rays, clouds, rain, snow;
//              under it the sky's word (color) and the day's high, low, feels like (color2); p2 1 — the digits alone;
//   Curve      (FL_WXCURVE)  the next 12 hours' temperature as a smooth line colored by the degrees, the sky's icons
//              every 3 hours over it, a light running along it; now, the warmest and the coldest marked;
//   Barometer  (FL_WXBARO)   an old barometer: the scale with STORMY · RAIN · CHANGE · FAIR · VERY DRY, the needle
//              (color) — the pressure now, the brass hand (color3) — in 3 hours; under the hub the degrees and hPa,
//              rising or falling;
//   Wear       (FL_WXWEAR)   what to put on — a light jacket, sunglasses, a hoodie, an umbrella or a warm hat (icons: the
//              wear set, studio.py) — the advice in words in its color, the degrees and feels like (color);
//   Buddy      (FL_WXBUDDY)  a little character dressed for the weather — a sun in sunglasses, a happy cloud, a sleepy
//              one, a sad rain cloud, a cloud in a woolly hat — bobbing and blinking, the degrees in a speech bubble
//              (color — the bubble, color2 — its digits; its size k);
//   Pixels     (FL_WXPIXEL)  the sky as in an old game: a pixel sun, clouds, rain and snow falling cell by cell, grass
//              (snow in winter); the time (p2 1; color3) and the degrees in pixel digits; its size k;
//   Week       (FL_WXWEEK)   the next five days in a straight row (as the Curve's hours; the user's word, 15 Nov 2026 —
//              it stood along an arc): the day, the sky, a bar from the low to the high on one scale (colored by the
//              degrees); today in color3.
// The sky has a fifth kind here — snow (weather.h keeps it as a flag: the other screens have four icons). What moves is
// drawn only where it changes; in Always On nothing moves. Only the widgets a project has are in its firmware (FN_WXW,
// config.h: a bit a widget, from FL_WXDIGITS); the editor's page has them all.
// Included from face.h after widgets.h.

#define WX_SNOW 4
// A widget's bit in FN_WXW (studio.py WXW: from wxdigits). Numbers, not FL_*: #if can't see an enum — FL_WXWEAR there
// is 0, and a project with one widget had all of them cut out but the first (15 Nov 2026)
#define WW_DIGITS 0
#define WW_CURVE 1
#define WW_BARO 2
#define WW_WEAR 3
#define WW_BUDDY 4
#define WW_PIXEL 5
#define WW_WEEK 6
static_assert(FL_WXDIGITS + WW_CURVE == FL_WXCURVE && FL_WXDIGITS + WW_BARO == FL_WXBARO && FL_WXDIGITS + WW_WEAR == FL_WXWEAR &&
                  FL_WXDIGITS + WW_BUDDY == FL_WXBUDDY && FL_WXDIGITS + WW_PIXEL == FL_WXPIXEL && FL_WXDIGITS + WW_WEEK == FL_WXWEEK,
              "WW_* — the widgets' order in face.h");
#ifdef __EMSCRIPTEN__
#define WW_ON(n) 1
#else
#define WW_ON(n) ((FN_WXW >> (n)) & 1)
#endif
#define WW_ITEMS 72
#define WW_CAPS 56
#define WW_SHAPES 64
#define WW_TXT 24
#define WW_TAU (2 * HALO_PI)
#define WW_INK 0x2A2320  // the buddy's eyes and mouth

static const char *const WW_WORD[5] = {"PARTLY CLOUDY", "SUNNY", "CLOUDY", "RAIN", "SNOW"};  // by WX_PARTLY … WX_SNOW
static bool face_over_others(const FaceScreen &S, int i);  // (face.h)

struct WwCap {  // a line with round ends, half as thick as w0 at its start and w1 at its end: drawn here, smoothed
  float x0, y0, x1, y1, w0, w1, a;
  uint32_t c;
  float o[5];  // where it was drawn: x0, y0, x1, y1, its widest half (o[4] < 0 — nowhere)
  uint32_t hash;
};
struct WwArea {  // a part drawn here: where it was, what it showed (a change — the old place and the new one)
  float b[4] = {0, 0, -1, -1};
  uint32_t hash = 0;
};
enum : uint8_t { WS_CIRCLE, WS_LINE, WS_RRECT, WS_WEDGE };
struct WwShape {  // a piece of the sky inside the digits (screen space)
  uint8_t kind;
  float p[6];  // a circle: x, y, r; a line: x0, y0, x1, y1, its half width; a box: x0, y0, x1, y1, its corners' r;
               // a wedge: its apex x, y, the cos and the sin of its first edge, the cos of the second, its sin
  uint32_t c;
  float a;
};
struct WwRun {
  HaloItem it[WW_ITEMS];
  WwCap cap[WW_CAPS];
  WwArea area[1];
  WwShape sh[WW_SHAPES];
  int nsh, over;  // the sky's shapes; the items from over on are drawn over what draw paints itself
  HaloPaint rowp[2][HALO_ROWS];  // the digits: a band's rows' colors, per core
  char txt[WW_TXT][40];
  float mid, mid2;  // the middle of font's and font2's letters
  bool vis, ob;
  float s, inv, a, vx, vy, dt;  // the view (its scale, 1 / it, its shift), the layer's alpha through it; the frame's step
  uint32_t ms, last_ms;         // the widgets' clock (stands in Always On), the last frame's
  int sky;
  float d[8];
  // Digits: the temperature's glyphs and where (screen); the sky's colors at the top and the bottom; its frame
  const HaloMask *gl[6];
  float gx[6], gy[6];
  int ng, gxi[6], gyi[6];  // (at scale 1: the glyphs' pixel columns and rows — the quick way, a row's run of a glyph)
  bool fast;
  uint32_t col[2], step;
  // Curve: each column's y (from the anchor) and color
  float *col_y;
  uint32_t *col_c;
  int col_n;
  uint32_t col_sig;
  // Barometer: the words along the scale, baked (their top-left from the dial's middle); the needle
  HaloMask word[5];
  float wx[5], wy[5];
  HaloSpring needle;
  HaloSpringK nk;
  bool needle_set;
  // Week: the bars (screen): x, top, bottom; their colors at the top and the bottom; their half width
  float bar[7][3], bw;
  uint32_t bcol[7][2];
  int nbar;
#if defined(HALO_P4_TALL) || defined(HALO_WATCH)
  bool vert, hbar;  // the 7″ board held tall: the curve upright (12.2B), the week in rows — its bars across (12.7A; the
                    // watch's 11B)
#endif
  // Pixels: the scene, the time, the degrees — two frames each (the one shown and the next); their size in cells
  HaloPixels px[3];
  int pxw[3], pxh[3];
  uint8_t *cells[3];
  uint32_t px_sig[3];
  uint16_t pframe[3];
  uint32_t pal[16], pal_t[16], pal_d[16];
  // Buddy: pops up when the sky changes
  HaloSpring pop;
  HaloSpringK pk;
};

// ---------- Small helpers ----------
static uint32_t ww_mix(uint32_t a, uint32_t b, float t) {  // color a toward b by t
  t = halo_clamp01(t);
  uint32_t out = 0;
  for (int s = 0; s < 24; s += 8) {
    const float ca = (a >> s) & 255, cb = (b >> s) & 255;
    out |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << s;
  }
  return out;
}
static inline uint32_t ww_hash(uint32_t h, float v) {
  h ^= (uint32_t)(int32_t)lroundf(v * 16);
  return h * 16777619u;
}
static inline uint32_t ww_hashu(uint32_t h, uint32_t v) {
  h ^= v;
  return h * 16777619u;
}
static inline float ww_rnd(uint32_t i) {  // 0..1, the same for the same i (no sinf of a big angle: slow on the S3)
  i = (i + 0x9E3779B9u) * 2654435761u;
  i ^= i >> 15, i *= 2246822519u, i ^= i >> 13;
  return (i & 0xFFFF) * (1.0f / 65536);
}
static inline float ww_wrap(float v, float m) {
  v = fmodf(v, m);
  return v < 0 ? v + m : v;
}
static inline float ww_rad(float d) {
  return d * (HALO_PI / 180);
}
static inline void ww_565(uint32_t c, int &r5, int &g6, int &b5) {
  r5 = (c >> 19) & 31, g6 = (c >> 10) & 63, b5 = (c >> 3) & 31;
}
static void ww_box_add(float *box, float x0, float y0, float x1, float y1) {
  if (x1 < x0) return;
  if (box[2] < box[0]) {
    box[0] = x0, box[1] = y0, box[2] = x1, box[3] = y1;
    return;
  }
  box[0] = fminf(box[0], x0), box[1] = fminf(box[1], y0), box[2] = fmaxf(box[2], x1), box[3] = fmaxf(box[3], y1);
}
// The degrees' color: cold blue, cool green, warm yellow, hot red (the mockup's; in °C)
static uint32_t ww_tcol(float T) {
  if (WXD_F) T = (T - 32) * (5 / 9.0f);
  static const float tt[6] = {-15, 0, 10, 18, 25, 35};
  static const uint32_t cc[6] = {0x6FA8FF, 0x9CCBFF, 0x8FE0C0, 0xFFD27A, 0xFF9F5A, 0xFF6060};
  if (T <= tt[0]) return cc[0];
  for (int i = 1; i < 6; i++)
    if (T <= tt[i]) return ww_mix(cc[i - 1], cc[i], (T - tt[i - 1]) / (tt[i] - tt[i - 1]));
  return cc[5];
}
static void ww_hour_name(char *o, int n, int h) {  // an hour as the clock shows it: 13 or 1PM
  h = ((h % 24) + 24) % 24;
  if (fc_h24) snprintf(o, n, "%02d", h);
  else snprintf(o, n, "%d%s", h % 12 ? h % 12 : 12, h < 12 ? "AM" : "PM");
}

// ---------- The weather the widgets show: the first city's ----------
struct WwSky {
  int sky, k0;  // the sky now (WX_PARTLY … WX_SNOW), the forecast's hour that is now
  int16_t now, feels, hi, lo;
  bool real;  // not a demo day (wx N)
  const WxCity *c;
};
static WwSky ww_sky() {
  WwSky w{};
  const WxDay d = wx_day(0);
  w.c = &wx_city[0], w.real = wx_demo < 0;
  w.now = d.now, w.feels = d.feels, w.hi = d.hi, w.lo = d.lo;
  w.sky = d.kind & 3;
  if (w.real && w.c->snow && w.sky == WX_CLOUD) w.sky = WX_SNOW;
  if (w.real && w.c->t0 && time(nullptr) > (time_t)w.c->t0)
    w.k0 = min(WX_AHEAD - 1, (int)((time(nullptr) - (time_t)w.c->t0) / 3600));
  return w;
}
static int ww_hour_sky(const WwSky &w, int j) {  // the sky j hours from now
  if (!w.real || !w.c->t0) return w.sky;
  const int k = min(w.k0 + j, WX_AHEAD - 1), kind = w.c->hk[k] & 3;
  return (w.c->hsnow >> k & 1) && kind == WX_CLOUD ? WX_SNOW : kind;
}
static float ww_hour_t(const WwSky &w, int j) {  // the degrees j hours from now
  if (w.real && w.c->t0) {
    const int k = min(w.k0 + j, WX_AHEAD - 1);
    if (w.c->ht[k] != WX_NONE) return w.c->ht[k];
  }
  return w.now == WX_NONE ? 0 : w.now;
}

// ---------- Items (through the view) ----------
static void ww_circle(WwRun &c, int i, float x, float y, float r, uint32_t col, float a, const HaloView &v, bool ob) {
  HaloItem &it = c.it[i];
  it.kind = HI_CIRCLE, it.x = x, it.y = y, it.r = r, it.scale = 1, it.color = col, it.alpha = a, it.on_black = ob;
  it.on = nullptr;
  halo_view_commit(it, v);
}
static void ww_rrect(WwRun &c, int i, float x, float y, float w, float h, float r, uint32_t col, float a,
                     const HaloView &v, bool ob) {
  HaloItem &it = c.it[i];
  it.kind = HI_RRECT, it.x = x, it.y = y, it.w = w, it.h = h, it.r = r, it.scale = 1, it.color = col, it.alpha = a;
  it.on_black = ob, it.on = nullptr;
  halo_view_commit(it, v);
}
static void ww_arc(WwRun &c, int i, float x, float y, float r, float w, float a0, float a1, uint32_t col, float a,
                   const HaloView &v, bool ob) {
  HaloItem &it = c.it[i];
  it.kind = HI_ARC, it.x = x, it.y = y, it.r = r, it.w = w, it.a0 = a0, it.a1 = a1, it.scale = 1, it.color = col;
  it.alpha = a, it.on_black = ob, it.on = nullptr;
  halo_view_commit(it, v);
}
static void ww_ring(WwRun &c, int i, float x, float y, float r, float w, uint32_t col, float a, const HaloView &v) {
  HaloItem &it = c.it[i];
  it.kind = HI_RING, it.x = x, it.y = y, it.r = r, it.w = w, it.v = 1, it.color = col, it.color2 = 0, it.scale = 1;
  it.alpha = a, it.on_black = false, it.on = nullptr;
  halo_view_commit(it, v);
}
static void ww_image(WwRun &c, int i, const HaloImage *im, float x, float y, float a, const HaloView &v) {
  HaloItem &it = c.it[i];
  it.kind = HI_IMAGE, it.image = im, it.x = x, it.y = y, it.scale = 1, it.alpha = im ? a : 0, it.on_black = false;
  it.on = nullptr;
  halo_view_commit(it, v);
}
static void ww_mask(WwRun &c, int i, const HaloMask *m, float x, float y, uint32_t col, float a, const HaloView &v) {
  HaloItem &it = c.it[i];
  it.kind = HI_MASK, it.mask = m, it.x = x, it.y = y, it.scale = 1, it.color = col, it.alpha = m ? a : 0;
  it.on_black = c.ob, it.on = c.ob ? &halo_black : nullptr;
  halo_view_commit(it, v);
}
// Text: align 0 — from x, 1 — centered, 2 — up to x (by its ink); the middle of its letters at y; scale sc
static void ww_text(WwRun &c, int i, const HaloFont *f, float mid, const char *s, float x, float y, int align,
                    uint32_t col, float a, int sp, const HaloView &v, bool ob, float sc = 1) {
  HaloItem &it = c.it[i];
  if (!f) {
    it.alpha = 0;
    halo_view_commit(it, v);
    return;
  }
  wx_text(it, *f, s, mid, align, x, y, sc, col, a, ob ? &halo_black : nullptr, sp);
  it.on_black = ob;
  halo_view_commit(it, v);
}
static void ww_pixels(WwRun &c, int i, const HaloPixels *p, int frame, float x, float y, float cell, float a,
                      const HaloView &v) {
  HaloItem &it = c.it[i];
  it.kind = HI_PIXELS, it.pixels = p, it.frame = frame, it.x = x, it.y = y, it.scale = cell, it.a0 = 0, it.alpha = a;
  it.hide = 0, it.trail = 0, it.tile = false, it.on_black = c.ob, it.on = nullptr;
  halo_view_commit(it, v);
}
static void ww_hide(WwRun &c, int from, const HaloView &v) {
  for (int i = from; i < WW_ITEMS; i++)
    if (c.it[i].shown) c.it[i].alpha = 0, halo_view_commit(c.it[i], v);
}

// ---------- Lines drawn here (the needle, the ticks, rays, drops) ----------
static void ww_cap_damage(const float *g) {  // a line's place, in pieces (a long one: not its whole square)
  const float dx = g[2] - g[0], dy = g[3] - g[1], hw = g[4] + 2;
  const int n = 1 + (int)(halo_sqrt(dx * dx + dy * dy) * (1 / 40.0f));
  for (int k = 0; k < n; k++) {
    const float ax = g[0] + dx * k / n, ay = g[1] + dy * k / n, bx = g[0] + dx * (k + 1) / n, by = g[1] + dy * (k + 1) / n;
    halo_damage_f(fminf(ax, bx) - hw, fminf(ay, by) - hw, fmaxf(ax, bx) + hw, fmaxf(ay, by) + hw);
  }
}
static void ww_cap_commit(WwCap &k) {
  const bool vis = k.a > 0.004f;
  uint32_t h = 2166136261u;
  h = ww_hash(h, k.x0), h = ww_hash(h, k.y0), h = ww_hash(h, k.x1), h = ww_hash(h, k.y1), h = ww_hash(h, k.w0);
  h = ww_hash(h, k.w1), h = ww_hash(h, k.a * 64), h = ww_hashu(h, k.c);
  const bool had = k.o[4] >= 0;
  if (vis && had && h == k.hash) return;
  if (had) ww_cap_damage(k.o);
  k.o[4] = -1;
  if (!vis) return;
  k.o[0] = k.x0, k.o[1] = k.y0, k.o[2] = k.x1, k.o[3] = k.y1, k.o[4] = fmaxf(k.w0, k.w1), k.hash = h;
  ww_cap_damage(k.o);
}
static void ww_cap(WwRun &c, int i, float x0, float y0, float x1, float y1, float w0, float w1, uint32_t col, float a,
                   const HaloView &v) {
  WwCap &k = c.cap[i];
  k.x0 = halo_view_x(v, x0), k.y0 = halo_view_y(v, y0), k.x1 = halo_view_x(v, x1), k.y1 = halo_view_y(v, y1);
  k.w0 = w0 * v.s, k.w1 = w1 * v.s, k.c = col, k.a = a * v.a;
  ww_cap_commit(k);
}
static void ww_hide_caps(WwRun &c, int from) {
  for (int i = from; i < WW_CAPS; i++)
    if (c.cap[i].o[4] >= 0) c.cap[i].a = 0, ww_cap_commit(c.cap[i]);
}
static void ww_cap_draw(const WwCap &k) {
  if (k.a <= 0.004f) return;
  const float hw = fmaxf(k.w0, k.w1), x0 = k.x0, y0 = k.y0, x1 = k.x1, y1 = k.y1;
  const float bx0 = fminf(x0, x1) - hw - 1, bx1 = fmaxf(x0, x1) + hw + 1, by0 = fminf(y0, y1) - hw - 1,
              by1 = fmaxf(y0, y1) + hw + 1;
  if (!halo_hits(bx0, by0, bx1, by1)) return;
  int r5, g6, b5;
  ww_565(k.c, r5, g6, b5);
  const float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy, inv = len2 > 1e-6f ? 1 / len2 : 0;
  const float len = len2 > 1e-6f ? halo_sqrt(len2) : 0, dw = k.w1 - k.w0;
  const int A = (int)(k.a * 256 + 0.5f);
  const int ya = max(hb.y, halo_floor(by0)), yb = min(hb.y + hb.rows - 1, halo_floor(by1));
  for (int y = ya; y <= yb; y++) {
    const float py = y + 0.5f;
    int xa = max(hb.x0, halo_floor(bx0)), xb = min(hb.x1, halo_floor(bx1));
    if (fabsf(dy) > 0.01f * len) {  // within hw of the line: an interval of this row
      const float xl = x0 + (py - y0) * dx / dy, half = (hw + 1.5f) * len / fabsf(dy);
      xa = max(xa, halo_floor(xl - half)), xb = min(xb, halo_floor(xl + half));
    }
    uint16_t *row = halo_row(y);
    for (int x = xa; x <= xb; x++) {
      const float px = x + 0.5f, t = halo_clamp01(((px - x0) * dx + (py - y0) * dy) * inv);
      const float qx = x0 + t * dx - px, qy = y0 + t * dy - py, d2 = qx * qx + qy * qy, r = k.w0 + dw * t;
      if (d2 >= (r + 0.5f) * (r + 0.5f)) continue;
      const float cov = halo_clamp01(r + 0.5f - halo_sqrt(d2));
      if (cov > 0) halo_blend(row + x, r5, g6, b5, (int)(cov * A));
    }
  }
}
static void ww_area(WwArea &A, bool vis, float x0, float y0, float x1, float y1, uint32_t hash) {
  const bool had = A.b[2] >= A.b[0];
  if (vis && had && A.hash == hash && A.b[0] == x0 && A.b[1] == y0 && A.b[2] == x1 && A.b[3] == y1) return;
  if (had) halo_damage_f(A.b[0], A.b[1], A.b[2], A.b[3]);
  if (!vis) {
    A.b[0] = A.b[1] = 0, A.b[2] = A.b[3] = -1, A.hash = 0;
    return;
  }
  A.b[0] = x0, A.b[1] = y0, A.b[2] = x1, A.b[3] = y1, A.hash = hash;
  halo_damage_f(x0, y0, x1, y1);
}

// ---------- Built once a change (face_layer_build) ----------
static void ww_free(FaceRun &R) {
  WwRun *c = R.ww;
  if (!c) return;
  free(c->col_y), free(c->col_c);
  for (uint8_t *p : c->cells) free(p);
  for (HaloMask &m : c->word) free((void *)m.bits), free((void *)m.ext);
  c->~WwRun();
  free(c);
  R.ww = nullptr;
}
static void *ww_alloc(size_t n) {
#ifdef __EMSCRIPTEN__
  return calloc(1, n);
#else
  void *p = heap_caps_calloc(1, n, MALLOC_CAP_SPIRAM);
  return p ? p : calloc(1, n);
#endif
}
#ifdef HALO_ROUND
#define WW_RECT false
static const int WW_PXW[3] = {44, 25, 22}, WW_PXH[3] = {16, 7, 7};  // the pixel scene, the time, the degrees (cells)
#else
#define WW_RECT true  // the 7-inch board (web/mockups/weather-widgets-rect.html): each widget in its layouts (p3)
#ifdef HALO_P4_TALL  // the 7″ board held tall, 600 x 1024 (web/mockups/rect-tall.html 12): each widget's two layouts
                     // under / over or across / upright — its anchors and inner places under #ifdef HALO_P4_TALL below
static const int WW_PXW[3] = {50, 25, 22}, WW_PXH[3] = {46, 7, 7};  // (room for the biggest scene: its band)
#else
#ifdef HALO_COMPACT  // the WT32-SC01 Plus held wide, 480 x 320 (web/mockups/wt32-widgets.html): the same layouts re-laid
                     // for it — its anchors and inner places under #ifdef HALO_COMPACT below (Tall has no widgets)
#ifdef HALO_WATCH  // the watch, 410 x 502 (web/mockups/watch206-widgets.html): a layout a widget, under #ifdef HALO_WATCH
static const int WW_PXW[3] = {46, 25, 22}, WW_PXH[3] = {28, 7, 7};  // (room for its scene: 46 x 28 cells)
#elif defined(HALO_SQUARE)  // the square, 480 x 480 (web/mockups/square-widgets.html): a layout a widget, under #ifdef HALO_SQUARE
static const int WW_PXW[3] = {60, 25, 22}, WW_PXH[3] = {30, 7, 7};  // (room for its scene: 54 x 30 cells)
#else
static const int WW_PXW[3] = {60, 25, 22}, WW_PXH[3] = {20, 7, 7};  // (room for the widest scene: its band)
#endif
#else
static const int WW_PXW[3] = {86, 25, 22}, WW_PXH[3] = {23, 7, 7};  // (room for the widest scene: its band)
#endif
#endif
#endif
static const char *const WW_WORDS[5] = {"STORMY", "RAIN", "CHANGE", "FAIR", "VERY DRY"};  // the barometer's
static const int16_t WW_AT[5] = {965, 985, 1000, 1015, 1035};                           // … and where (hPa)
static inline float ww_baro(float p) {  // hPa → the barometer's angle (radians from 12 o'clock): 960 … 1040 over 250°
  return ww_rad(-125 + (fminf(1040, fmaxf(960, p)) - 960) * (250 / 80.0f));
}
static void ww_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  void *p = ww_alloc(sizeof(WwRun));
  if (!p) return;
  WwRun &c = *(R.ww = new (p) WwRun());
  for (WwCap &k : c.cap) k.o[4] = -1, k.a = 0;
  c.sky = -1, c.over = WW_ITEMS;
  const bool letters = L.kind == FL_WXBARO || L.kind == FL_WXWEAR || L.kind == FL_WXPIXEL;
  c.mid = L.font ? halo_text_mid(*L.font, letters ? "ABCDEFGHIJKLMNOPRSTUVWY" : "0123456789") : 0;
  c.mid2 = L.font2 ? halo_text_mid(*L.font2, L.kind == FL_WXBUDDY ? "z" : L.kind == FL_WXBARO ? "0123456789" : "ABCDEFGHIJKLMNOPRSTUVWY0123456789") : 0;
  switch (L.kind) {
    case FL_WXCURVE:
      c.col_n = WW_RECT ? 761 : 275;  // (a column a pixel: the widest line — the 7-inch band's)
      c.col_y = (float *)ww_alloc(c.col_n * sizeof(float)), c.col_c = (uint32_t *)ww_alloc(c.col_n * sizeof(uint32_t));
      break;
    case FL_WXBARO:  // the words along the scale, baked once (at 211 px from its middle)
      for (int j = 0; L.font && j < 5; j++) {
        float tx, ty;
#ifdef HALO_P4_TALL  // (the tall dial is 206 px: its words at 224)
        if (halo_text_arc(c.word[j], *L.font, WW_WORDS[j], c.mid, 224, ww_baro(WW_AT[j]), false, 2, &tx, &ty))
#else
#ifdef HALO_COMPACT  // (the WT32 wide's dial is 90 px: its words at 101.5)
        if (halo_text_arc(c.word[j], *L.font, WW_WORDS[j], c.mid, 101.5f, ww_baro(WW_AT[j]), false, 1, &tx, &ty))
#else
        if (halo_text_arc(c.word[j], *L.font, WW_WORDS[j], c.mid, 211, ww_baro(WW_AT[j]), false, 2, &tx, &ty))
#endif
#endif
          c.wx[j] = tx - HALO_W / 2.0f, c.wy[j] = ty - HALO_W / 2.0f;  // (its circle's middle: (W/2, W/2) — on a rectangle too)
      }
      c.nk = halo_spring(0.7f, 0.45f);  // the needle swings a little past and settles
      break;
    case FL_WXBUDDY:
      c.pk = halo_spring(0.45f, 0.5f);
      halo_spring_set(c.pop, 1);
      break;
    case FL_WXPIXEL: {
      static const uint32_t PAL[16] = {0,        0xFFD23F, 0xFF9F1C, 0xFFFFFF, 0xB8C0CC, 0x9AA3B0, 0x6B7380, 0x4FA3FF,
                                       0x4CAF50, 0x388E3C, 0x7A5230, 0x5E3F24, 0xF2F6FF, 0xC9D6EA, 0x9AAAC4, 0x7D8DA8};
      memcpy(c.pal, PAL, sizeof(PAL));
      for (int j = 0; j < 3; j++) {
        const int w = WW_PXW[j], h = WW_PXH[j];
        c.pxw[j] = w, c.pxh[j] = h;
        c.cells[j] = (uint8_t *)ww_alloc(2 * h * ((w + 1) / 2));
        HaloPixels &q = c.px[j];
        q.w = w, q.h = h, q.frames = 2, q.ms = 250, q.ax = w / 2.0f, q.ay = 0;
        q.palette = j == 0 ? c.pal : j == 1 ? c.pal_t : c.pal_d, q.cells = c.cells[j];
        q.bx0 = 0, q.by0 = 0, q.bx1 = w, q.by1 = h;
        c.px_sig[j] = 0xFFFFFFFFu;
      }
      break;
    }
  }
}

// ---------- Digits: the sky inside the temperature ----------
static void ww_add(WwRun &c, uint8_t kind, float p0, float p1, float p2, float p3, float p4, uint32_t col, float a,
                   float p5 = 0) {
  if (c.nsh >= WW_SHAPES) return;
  WwShape &q = c.sh[c.nsh++];
  q.kind = kind, q.p[0] = p0, q.p[1] = p1, q.p[2] = p2, q.p[3] = p3, q.p[4] = p4, q.p[5] = p5, q.c = col, q.a = a;
}
static void ww_cloud(WwRun &c, float x, float y, float k, uint32_t col) {  // the mockup's cloud: three bumps on a base
  ww_add(c, WS_CIRCLE, x - 11 * k, y + 5 * k, 9 * k, 0, 0, col, 1);
  ww_add(c, WS_CIRCLE, x + 1 * k, y - 2 * k, 12 * k, 0, 0, col, 1);
  ww_add(c, WS_CIRCLE, x + 12 * k, y + 6 * k, 8 * k, 0, 0, col, 1);
  ww_add(c, WS_RRECT, x - 20 * k, y + 5 * k, x + 20 * k, y + 14 * k, 4.5f * k, col, 1);
}
// The sky at t seconds over the box x, y, w, h (screen; s — the view's scale)
static void ww_sky_shapes(WwRun &c, int sky, float t, float x, float y, float w, float h, float s) {
  static const uint32_t TOP[5] = {0x3D7FDB, 0xFFD45E, 0xA3AAB5, 0x2D5A96, 0x3E5579};
  static const uint32_t BOT[5] = {0x9CC9F5, 0xFF7A3D, 0x5D636D, 0x6FA3E6, 0x8098BC};
  c.col[0] = TOP[sky], c.col[1] = BOT[sky], c.nsh = 0;
  const float span = w + 140 * s;
  auto drift = [&](float sp, float ph) { return x - 70 * s + ww_wrap((t * sp + ph) * s, span); };
  if (sky == WX_SUN) {
    const float sx = x + w * 0.8f, sy = y + h * 0.08f, rot = ww_wrap(t * 0.12f, WW_TAU);
    for (int i = 0; i < 12; i++) {
      const float an = rot + i * (WW_TAU / 12);
      ww_add(c, WS_WEDGE, sx, sy, cosf(an - 0.09f), sinf(an - 0.09f), cosf(an + 0.09f), 0xFFFFFF, 0.16f, sinf(an + 0.09f));
    }
    ww_add(c, WS_CIRCLE, sx, sy, 58 * s, 0, 0, 0xFFF3B0, 0.32f);
    ww_add(c, WS_CIRCLE, sx, sy, 30 * s, 0, 0, 0xFFF6CC, 1);
  } else if (sky == WX_PARTLY) {
    const float sx = x + w * 0.82f, sy = y + h * 0.1f;
    ww_add(c, WS_CIRCLE, sx, sy, 60 * s, 0, 0, 0xFFF3B0, 0.25f);
    ww_add(c, WS_CIRCLE, sx, sy, 30 * s, 0, 0, 0xFFE27A, 1);
    ww_cloud(c, drift(14, 0), y + h * 0.45f, 3.4f * s, 0xFFFFFF);
    ww_cloud(c, drift(9, 190), y + h * 0.8f, 2.4f * s, 0xEEF3FA);
  } else if (sky == WX_CLOUD) {
    ww_cloud(c, drift(7, 60), y + h * 0.3f, 3.6f * s, 0x9FA6B1);
    ww_cloud(c, drift(16, 200), y + h * 0.55f, 2.8f * s, 0xC3C8D0);
    ww_cloud(c, drift(11, 330), y + h * 0.8f, 3.2f * s, 0xE3E6EB);
  } else if (sky == WX_RAIN) {
    for (int i = 0; i < 44; i++) {
      const float sp = (260 + ww_rnd(i + 9) * 140) * s, fy = ww_wrap(t * sp + ww_rnd(i + 3) * h, h + 40 * s) - 20 * s;
      const float fx = ww_wrap(ww_rnd(i) * (w + 80 * s) - fy * 0.3f, w + 80 * s) - 40 * s;
      ww_add(c, WS_LINE, x + fx, y + fy, x + fx - 4.8f * s, y + fy + 16 * s, 1.1f * s, 0xBFD6FF, 0.75f);
    }
  } else {
    for (int i = 0; i < 52; i++) {
      const float r = (1.6f + ww_rnd(i + 5) * 2.2f) * 1.2f * s, sp = (22 + ww_rnd(i + 7) * 26) * s;
      const float fy = ww_wrap(t * sp + ww_rnd(i + 1) * h, h + 20 * s) - 10 * s;
      const float fx = ww_rnd(i) * w + sinf(ww_wrap(t * 1.3f + i, WW_TAU)) * 6 * s;
      ww_add(c, WS_CIRCLE, x + fx, y + fy, r, 0, 0, 0xFFFFFF, 0.55f + 0.45f * ww_rnd(i + 11));
    }
  }
}
static int ww_nib(const WwRun &c, int x, int y) {  // the digits' mask at a pixel (0..15)
  const float px = x + 0.5f, py = y + 0.5f;
  for (int j = 0; j < c.ng; j++) {
    const HaloMask &m = *c.gl[j];
    const int i = halo_floor((px - c.gx[j]) * c.inv), r = halo_floor((py - c.gy[j]) * c.inv);
    if (i < 0 || r < 0 || i >= m.w || r >= m.h) continue;
    const uint8_t b = m.bits[r * ((m.w + 1) >> 1) + (i >> 1)];
    return (i & 1) ? (b & 15) : (b >> 4);
  }
  return 0;
}
// A row's run from xl to xr (its edges' fractions — their coverage), inside the digits only. At scale 1 — glyph by
// glyph along the run, its row's nibbles one after another (a pixel solid in the digits and in the shape — written,
// not blended); in a transition (scaled) — a pixel at a time through ww_nib
static void ww_span(const WwRun &c, int y, float xl, float xr, uint16_t px, int r5, int g6, int b5, int A, bool solid) {
  const int xa = max(max(hb.x0, halo_floor(xl)), halo_floor(c.d[0])), xb = min(min(hb.x1, halo_floor(xr)), halo_floor(c.d[2]));
  if (xa > xb) return;
  uint16_t *row = halo_row(y);
  if (!c.fast) {
    for (int x = xa; x <= xb; x++) {
      const float cov = fminf(1, fminf(x + 1 - xl, xr - x));
      if (cov <= 0) continue;
      const int m = ww_nib(c, x, y);
      if (m) halo_blend(row + x, r5, g6, b5, (int)(m * 17 * A * cov) >> 8);
    }
    return;
  }
  int done = xa - 1;  // (glyphs left to right: where two overlap, the pixel is drawn once)
  for (int j = 0; j < c.ng; j++) {
    const HaloMask &m = *c.gl[j];
    const int r = y - c.gyi[j];
    if (r < 0 || r >= m.h || m.ext[2 * r] > m.ext[2 * r + 1]) continue;
    const int e1 = m.ext[2 * r + 1] == 255 ? m.w - 1 : m.ext[2 * r + 1];
    const int a = max(done + 1, c.gxi[j] + m.ext[2 * r]), b = min(xb, c.gxi[j] + e1);
    if (a > b) continue;
    done = b;
    const uint8_t *bits = m.bits + r * ((m.w + 1) >> 1);
    for (int x = a; x <= b; x++) {
      const int i = x - c.gxi[j], v = (i & 1) ? (bits[i >> 1] & 15) : (bits[i >> 1] >> 4);
      if (!v) continue;
      const float cov = x > xl && x + 1 < xr ? 1 : fminf(1, fminf(x + 1 - xl, xr - x));
      if (cov <= 0) continue;
      if (solid && v == 15 && cov >= 1) row[x] = px;
      else halo_blend(row + x, r5, g6, b5, (int)(v * 17 * A * cov) >> 8);
    }
  }
}
static void ww_shape_draw(const WwRun &c, const WwShape &q) {
  int r5, g6, b5;
  ww_565(q.c, r5, g6, b5);
  const int A = (int)(q.a * c.a * 256 + 0.5f);
  const uint16_t px = halo_565(q.c);
  const bool solid = A >= 256;
  const int y0 = max(hb.y, halo_floor(c.d[1])), y1 = min(hb.y + hb.rows - 1, halo_floor(c.d[3]));
  switch (q.kind) {
    case WS_CIRCLE: {
      const float cx = q.p[0], cy = q.p[1], r = q.p[2];
      const int ya = max(y0, halo_floor(cy - r)), yb = min(y1, halo_floor(cy + r));
      for (int y = ya; y <= yb; y++) {
        const float dy = y + 0.5f - cy, e = r * r - dy * dy;
        if (e <= 0) continue;
        const float hw = halo_sqrt(e);
        ww_span(c, y, cx - hw, cx + hw, px, r5, g6, b5, A, solid);
      }
      break;
    }
    case WS_RRECT: {
      const float X0 = q.p[0], Y0 = q.p[1], X1 = q.p[2], Y1 = q.p[3], r = q.p[4];
      const int ya = max(y0, halo_floor(Y0)), yb = min(y1, halo_floor(Y1));
      for (int y = ya; y <= yb; y++) {
        const float py = y + 0.5f, dy = py < Y0 + r ? Y0 + r - py : py > Y1 - r ? py - (Y1 - r) : 0;
        if (dy >= r && r > 0) continue;
        const float in = r - halo_sqrt(r * r - dy * dy);
        ww_span(c, y, X0 + in, X1 - in, px, r5, g6, b5, A, solid);
      }
      break;
    }
    case WS_LINE: {
      const float x0 = q.p[0], ly0 = q.p[1], x1 = q.p[2], ly1 = q.p[3], hw = q.p[4], sl = (x1 - x0) / (ly1 - ly0);
      const int ya = max(y0, halo_floor(ly0 - hw)), yb = min(y1, halo_floor(ly1 + hw));
      for (int y = ya; y <= yb; y++) {
        const float py = y + 0.5f, yy = fminf(ly1, fmaxf(ly0, py)), d = fabsf(py - yy);
        if (d >= hw) continue;
        const float xc = x0 + (yy - ly0) * sl, h = (d > 0 ? halo_sqrt(hw * hw - d * d) : hw) * 1.1f;
        ww_span(c, y, xc - h, xc + h, px, r5, g6, b5, A, solid);
      }
      break;
    }
    case WS_WEDGE: {  // the rows between its two edges (the one that misses the row: open to that side)
      const float sx = q.p[0], sy = q.p[1], k1 = q.p[3] != 0 ? q.p[2] / q.p[3] : 0, k2 = q.p[5] != 0 ? q.p[4] / q.p[5] : 0;
      for (int y = y0; y <= y1; y++) {
        const float dy = y + 0.5f - sy;
        const bool r1 = q.p[3] * dy > 0, r2 = q.p[5] * dy > 0;
        if (!r1 && !r2) continue;
        const float e1 = r1 ? sx + dy * k1 : 0, e2 = r2 ? sx + dy * k2 : 0;
        float lo = -1e9f, hi = 1e9f;
        if (r1 && r2) lo = fminf(e1, e2), hi = fmaxf(e1, e2);
        else if (q.p[2] + q.p[4] > 0) lo = r1 ? e1 : e2;
        else hi = r1 ? e1 : e2;
        ww_span(c, y, lo, hi, px, r5, g6, b5, A, solid);
      }
      break;
    }
  }
}
static void ww_draw_digits(WwRun &c) {
  if (c.a <= 0.004f || !c.ng || !halo_hits(c.d[0] - 1, c.d[1] - 1, c.d[2] + 1, c.d[3] + 1)) return;
  HaloPaint *rows = c.rowp[xPortGetCoreID() & 1];
  const float k = 1 / fmaxf(1, c.d[3] - c.d[1]);
  for (int y = hb.y; y < hb.y + hb.rows; y++) halo_paint(rows[y - hb.y], ww_mix(c.col[0], c.col[1], (y + 0.5f - c.d[1]) * k), c.a);
  for (int j = 0; j < c.ng; j++) halo_mask_rows(*c.gl[j], c.gx[j], c.gy[j], c.s, rows, hb.y, hb.rows);
  if (!c.fast) return;  // (in a transition — the digits' colors only: the sky comes back once it stands; ~3x the work else)
  for (int j = 0; j < c.nsh; j++) ww_shape_draw(c, c.sh[j]);
}

// ---------- Curve: the line and the light under it ----------
static void ww_draw_curve(const WwRun &c) {
  const WwArea &A = c.area[0];
  if (c.a <= 0.004f || !c.col_y || A.b[2] < A.b[0] || !halo_hits(A.b[0], A.b[1], A.b[2], A.b[3])) return;
  const float s = c.s, ax = c.d[0], ay = c.d[1], hw0 = c.d[5] * s, fill_y = CY + c.vy + (ay + c.d[3] - CY) * s;
  const int ncol = (int)c.d[6];
  const int Ac = (int)(c.a * 256 + 0.5f);
  const int xa = max(hb.x0, halo_floor(A.b[0])), xb = min(hb.x1, halo_floor(A.b[2]));
  for (int x = xa; x <= xb; x++) {
    const float lx = CX + (x + 0.5f - CX - c.vx) * c.inv;
    const int i = halo_floor(lx - (ax - c.d[2]) + 0.5f);
    if (i < 0 || i >= ncol) continue;
    const float ys = CY + c.vy + (ay + c.col_y[i] - CY) * s;
    const float sl = (c.col_y[min(ncol - 1, i + 1)] - c.col_y[max(0, i - 1)]) * 0.5f;
    const float hh = hw0 * halo_sqrt(1 + sl * sl);
    int r5, g6, b5;
    ww_565(c.col_c[i], r5, g6, b5);
    const float fh = fill_y - ys;  // the light under the line: from 22 % down to nothing
    if (fh > 1) {
      const float k = 0.22f / fh;
      const int ya = max(hb.y, halo_floor(ys)), yb = min(hb.y + hb.rows - 1, halo_floor(fill_y));
      for (int y = ya; y <= yb; y++) {
        const float al = (fill_y - (y + 0.5f)) * k;
        if (al > 0) halo_blend(halo_row(y) + x, r5, g6, b5, (int)(al * Ac));
      }
    }
    const int ya = max(hb.y, halo_floor(ys - hh - 1)), yb = min(hb.y + hb.rows - 1, halo_floor(ys + hh + 1));
    for (int y = ya; y <= yb; y++) {
      const float cov = halo_clamp01(hh + 0.5f - fabsf(y + 0.5f - ys));
      if (cov > 0) halo_blend(halo_row(y) + x, r5, g6, b5, (int)(cov * Ac));
    }
  }
}
#ifdef HALO_P4_TALL
// The curve upright (the 7″ board held tall, 12.2B): a row a pixel down the hours — each row's x (from the anchor) and
// color in col_y, col_c; the light left of the line, from 22 % at it to nothing at c.d[3]
static void ww_draw_curve_v(const WwRun &c) {
  const WwArea &A = c.area[0];
  if (c.a <= 0.004f || !c.col_y || A.b[2] < A.b[0] || !halo_hits(A.b[0], A.b[1], A.b[2], A.b[3])) return;
  const float s = c.s, ax = c.d[0], ay = c.d[1], hw0 = c.d[5] * s, fill_x = CX + c.vx + (ax + c.d[3] - CX) * s;
  const int nrow = (int)c.d[6];
  const int Ac = (int)(c.a * 256 + 0.5f);
  const int ya = max(hb.y, halo_floor(A.b[1])), yb = min(hb.y + hb.rows - 1, halo_floor(A.b[3]));
  for (int y = ya; y <= yb; y++) {
    const float ly = CY + (y + 0.5f - CY - c.vy) * c.inv;
    const int i = halo_floor(ly - (ay - c.d[2]) + 0.5f);
    if (i < 0 || i >= nrow) continue;
    const float xs = CX + c.vx + (ax + c.col_y[i] - CX) * s;
    const float sl = (c.col_y[min(nrow - 1, i + 1)] - c.col_y[max(0, i - 1)]) * 0.5f;
    const float hh = hw0 * halo_sqrt(1 + sl * sl);
    int r5, g6, b5;
    ww_565(c.col_c[i], r5, g6, b5);
    uint16_t *row = halo_row(y);
    const float fw = xs - fill_x;
    if (fw > 1) {
      const float k = 0.22f / fw;
      const int xa = max(hb.x0, halo_floor(fill_x)), xb = min(hb.x1, halo_floor(xs));
      for (int x = xa; x <= xb; x++) {
        const float al = (x + 0.5f - fill_x) * k;
        if (al > 0) halo_blend(row + x, r5, g6, b5, (int)(al * Ac));
      }
    }
    const int xa = max(hb.x0, halo_floor(xs - hh - 1)), xb = min(hb.x1, halo_floor(xs + hh + 1));
    for (int x = xa; x <= xb; x++) {
      const float cov = halo_clamp01(hh + 0.5f - fabsf(x + 0.5f - xs));
      if (cov > 0) halo_blend(row + x, r5, g6, b5, (int)(cov * Ac));
    }
  }
}
#endif

// ---------- Week: the bars ----------
static void ww_draw_bars(const WwRun &c) {
  const WwArea &A = c.area[0];
  if (c.a <= 0.004f || A.b[2] < A.b[0] || !halo_hits(A.b[0], A.b[1], A.b[2], A.b[3])) return;
  const float hw = c.bw;
  const int Ac = (int)(c.a * 256 + 0.5f);
  for (int j = 0; j < c.nbar; j++) {
    const float x = c.bar[j][0], top = c.bar[j][1], bot = c.bar[j][2];
    if (!halo_hits(x - hw - 1, top - hw - 1, x + hw + 1, bot + hw + 1)) continue;
    const float k = 1 / fmaxf(1, bot - top);
    const int ya = max(hb.y, halo_floor(top - hw)), yb = min(hb.y + hb.rows - 1, halo_floor(bot + hw));
    for (int y = ya; y <= yb; y++) {
      const float py = y + 0.5f, dy = py < top ? top - py : py > bot ? py - bot : 0;
      if (dy >= hw + 0.5f) continue;
      const float half = dy > 0 ? halo_sqrt(fmaxf(0, hw * hw - dy * dy)) : hw, xl = x - half, xr = x + half;
      int r5, g6, b5;
      ww_565(ww_mix(c.bcol[j][0], c.bcol[j][1], (py - top) * k), r5, g6, b5);
      uint16_t *row = halo_row(y);
      const int xa = max(hb.x0, halo_floor(xl)), xb = min(hb.x1, halo_floor(xr));
      for (int px = xa; px <= xb; px++) {
        const float cov = fminf(1, fminf(px + 1 - xl, xr - px));
        if (cov > 0) halo_blend(row + px, r5, g6, b5, (int)(cov * Ac));
      }
    }
  }
}
#if defined(HALO_P4_TALL) || defined(HALO_WATCH)
// The week in rows (the 7″ board held tall, 12.7A; the watch, 11B): each bar across — bar[j]: its y, left end, right end; colored from
// the low (left, bcol[j][0]) to the high (right, bcol[j][1]); half as thick as c.bw, round ends
static void ww_draw_hbars(const WwRun &c) {
  const WwArea &A = c.area[0];
  if (c.a <= 0.004f || A.b[2] < A.b[0] || !halo_hits(A.b[0], A.b[1], A.b[2], A.b[3])) return;
  const float hw = c.bw;
  const int Ac = (int)(c.a * 256 + 0.5f);
  for (int j = 0; j < c.nbar; j++) {
    const float y = c.bar[j][0], l = c.bar[j][1], r = c.bar[j][2];
    if (!halo_hits(l - hw - 1, y - hw - 1, r + hw + 1, y + hw + 1)) continue;
    const float k = 1 / fmaxf(1, r - l);
    const int ya = max(hb.y, halo_floor(y - hw)), yb = min(hb.y + hb.rows - 1, halo_floor(y + hw));
    for (int py = ya; py <= yb; py++) {
      const float dy = fabsf(py + 0.5f - y);
      if (dy >= hw + 0.5f) continue;
      const float e = halo_sqrt(fmaxf(0, hw * hw - dy * dy)), xl = l - e, xr = r + e, vc = halo_clamp01(hw + 0.5f - dy);
      uint16_t *row = halo_row(py);
      const int xa = max(hb.x0, halo_floor(xl)), xb = min(hb.x1, halo_floor(xr));
      for (int px = xa; px <= xb; px++) {
        const float cov = fminf(1, fminf(px + 1 - xl, xr - px)) * vc;
        if (cov <= 0) continue;
        int r5, g6, b5;
        ww_565(ww_mix(c.bcol[j][0], c.bcol[j][1], (px + 0.5f - l) * k), r5, g6, b5);
        halo_blend(row + px, r5, g6, b5, (int)(cov * Ac));
      }
    }
  }
}
#endif

// ---------- Pixels: the scene, cell by cell ----------
static const char *const WW_CLOUD_PX[7] = {"......WWW.....", ".....WWWWW....", "..WW.WWWWWWW..", ".WWWWWWWWWWWW.",
                                           "WWWWWWWWWWWWWW", "WWWWWWWWWWWWWW", ".SSSSSSSSSSSS."};
static uint8_t *ww_px_frame(WwRun &c, int j) {  // the frame not shown, cleared
  const int bytes = c.pxh[j] * ((c.pxw[j] + 1) / 2);
  uint8_t *f = c.cells[j] + (1 - c.pframe[j]) * bytes;
  memset(f, 0, bytes);
  return f;
}
static void ww_px_put(const WwRun &c, uint8_t *f, int j, int x, int y, int col) {
  if (x < 0 || y < 0 || x >= c.pxw[j] || y >= c.pxh[j]) return;
  uint8_t &b = f[y * ((c.pxw[j] + 1) / 2) + (x >> 1)];
  b = (x & 1) ? (b & 0xF0) | col : (b & 0x0F) | (col << 4);
}
static void ww_px_scene(WwRun &c, int sky, uint32_t fr) {
  uint8_t *f = ww_px_frame(c, 0);
  const int o = 11;  // (the sky's 22 columns in the middle of the scene's 44)
  auto put = [&](int x, int y, int col) { ww_px_put(c, f, 0, x + o, y, col); };
  auto sun = [&](int sx, int sy) {
    for (int dy = -4; dy <= 4; dy++)
      for (int dx = -4; dx <= 4; dx++) {
        const int d = dx * dx + dy * dy;
        if (d <= 10) put(sx + dx, sy + dy, 1);
        else if (d <= 17) put(sx + dx, sy + dy, 2);
      }
    static const int8_t A[8][2] = {{0, -6}, {0, -7}, {0, 6}, {0, 7}, {-6, 0}, {-7, 0}, {6, 0}, {7, 0}};
    static const int8_t B[8][2] = {{4, 4}, {5, 5}, {-4, 4}, {-5, 5}, {4, -4}, {5, -5}, {-4, -4}, {-5, -5}};
    for (int k = 0; k < 8; k++) put(sx + ((fr & 1) ? A : B)[k][0], sy + ((fr & 1) ? A : B)[k][1], 1);
  };
  auto cloud = [&](int ox, int oy, int w, int sh) {
    for (int r = 0; r < 7; r++)
      for (int k = 0; WW_CLOUD_PX[r][k]; k++)
        if (WW_CLOUD_PX[r][k] != '.') put(ox + k, oy + r, WW_CLOUD_PX[r][k] == 'W' ? w : sh);
  };
  if (sky == WX_SUN) sun(11, 6);
  if (sky == WX_PARTLY) sun(14, 4), cloud(2 + (fr / 3) % 4, 5, 3, 4);
  if (sky == WX_CLOUD) cloud(7 - (fr / 4) % 3, 1, 5, 6), cloud(1 + (fr / 5) % 3, 5, 3, 4);
  if (sky == WX_RAIN || sky == WX_SNOW) {
    const bool rain = sky == WX_RAIN;
    cloud(4, 0, rain ? 5 : 3, rain ? 6 : 4);
    for (int i = 0; i < 7; i++) {
      const int x = 5 + ((i * 5 + 1) % 12);
      if (rain) {
        const int y = 7 + (fr + i * 3) % 6;
        put(x, y, 7), put(x, y + 1, 7);
      } else {
        put(x + (int)((fr / 2 + i) % 3) - 1, 7 + ((fr + i * 3) % 8) / 2, 3);
      }
    }
  }
  const bool snow = sky == WX_SNOW;
  for (int x = 0; x < c.pxw[0]; x++) {  // the ground: grass (puddles in the rain; snow in winter) over two rows of earth
    const int i = x - o + 2;
    ww_px_put(c, f, 0, x, 13, ((i + (i >> 2)) % 3 + 3) % 3 ? (snow ? 12 : 8) : (snow ? 13 : 9));
    if (sky == WX_RAIN && ((i % 7) + 7) % 7 == 3) ww_px_put(c, f, 0, x, 13, 7), ww_px_put(c, f, 0, x + 1, 13, 7);
    ww_px_put(c, f, 0, x, 14, snow ? 14 : 10), ww_px_put(c, f, 0, x, 15, snow ? 15 : 11);
  }
  c.pframe[0] = 1 - c.pframe[0];
}
#if WW_RECT
// The 7-inch board's wide scene (weather-widgets-rect.html, pxWide): the sky's things across it — the sun at three
// quarters, clouds drifting at their own pace, three rain or snow clouds — grass (snow) along its bottom three rows
static void ww_px_wide(WwRun &c, int sky, uint32_t fr, int cols, int rows) {
  uint8_t *f = ww_px_frame(c, 0);
  auto put = [&](int x, int y, int col) { ww_px_put(c, f, 0, x, y, col); };
  auto wc = [&](float x) { const int m = cols + 16; return ((int)x % m + m) % m - 15; };
  auto sun = [&](int sx, int sy) {
    for (int dy = -4; dy <= 4; dy++)
      for (int dx = -4; dx <= 4; dx++) {
        const int d = dx * dx + dy * dy;
        if (d <= 10) put(sx + dx, sy + dy, 1);
        else if (d <= 17) put(sx + dx, sy + dy, 2);
      }
    static const int8_t A[8][2] = {{0, -6}, {0, -7}, {0, 6}, {0, 7}, {-6, 0}, {-7, 0}, {6, 0}, {7, 0}};
    static const int8_t B[8][2] = {{4, 4}, {5, 5}, {-4, 4}, {-5, 5}, {4, -4}, {5, -5}, {-4, -4}, {-5, -5}};
    for (int k = 0; k < 8; k++) put(sx + ((fr & 1) ? A : B)[k][0], sy + ((fr & 1) ? A : B)[k][1], 1);
  };
  auto cloud = [&](int ox, int oy, int w, int sh) {
    for (int r = 0; r < 7; r++)
      for (int k = 0; WW_CLOUD_PX[r][k]; k++)
        if (WW_CLOUD_PX[r][k] != '.') put(ox + k, oy + r, WW_CLOUD_PX[r][k] == 'W' ? w : sh);
  };
  const int g = rows - 3, top = g - 1;
  if (sky == WX_SUN) sun(cols * 74 / 100, top * 45 / 100);
#ifdef HALO_P4_TALL  // (the tall sky, rect-tall.html 12.6: the sun higher, the clouds wider apart, a gray one low down)
  if (sky == WX_PARTLY) {
    sun(cols * 71 / 100, top * 32 / 100);
    cloud(wc(15 + cols * 0.36f + fr / 3.0f), top * 58 / 100, 3, 4), cloud(wc(15 + cols * 0.04f + fr / 5.0f), top / 10, 3, 4);
    if (rows > 30) cloud(wc(15 + cols * 0.1f + fr / 4.0f), top * 80 / 100, 5, 6);
  }
#else
  if (sky == WX_PARTLY) {
    sun(cols * 74 / 100, top * 40 / 100);
    cloud(wc(cols * 0.5f + fr / 3.0f), top * 55 / 100, 3, 4), cloud(wc(cols * 0.12f + fr / 5.0f), 2, 3, 4);
  }
#endif
  if (sky == WX_CLOUD)
    cloud(wc(cols * 0.2f + fr / 4.0f), 2, 5, 6), cloud(wc(cols * 0.6f + fr / 3.0f), top * 50 / 100, 3, 4),
        cloud(wc(cols * 0.85f + fr / 6.0f), 1, 3, 4);
  if (sky == WX_RAIN || sky == WX_SNOW) {
    const bool rain = sky == WX_RAIN;
    static const uint8_t AT[3] = {10, 42, 74};  // (per cent of the width)
    for (int ci = 0; ci < 3; ci++) {
      const int ox = cols * AT[ci] / 100, span = max(1, top - 8);
      cloud(ox, 1 + (ci & 1), rain ? 5 : 3, rain ? 6 : 4);
      for (int i = 0; i < 6; i++) {
        const int x = ox + 1 + ((i * 5 + 1 + ci) % 12), m = (int)((fr + i * 3 + ci * 2) % span);
        if (rain) put(x, 8 + (ci & 1) + m, 7), put(x, 9 + (ci & 1) + m, 7);
        else put(x + (int)((fr / 2 + i) % 3) - 1, 8 + (ci & 1) + m / 2, 3);
      }
    }
  }
  const bool snow = sky == WX_SNOW;
  for (int i = 0; i < cols; i++) {
    put(i, g, (i + (i >> 2)) % 3 ? (snow ? 12 : 8) : (snow ? 13 : 9));
    if (sky == WX_RAIN && i % 9 == 4) put(i, g, 7), put(i + 1, g, 7);
    put(i, g + 1, snow ? 14 : 10), put(i, g + 2, snow ? 15 : 11);
  }
  c.pframe[0] = 1 - c.pframe[0];
}
#endif
// Pixel digits (5 x 7; '-' and '°' 3 wide; the colon 1): WG_GLYPH of widgets.h
static void ww_px_text(WwRun &c, int j, const char *s) {
  static const uint8_t MINUS[7] = {0, 0, 0, 7, 0, 0, 0}, DEG[7] = {2, 5, 2, 0, 0, 0, 0};
  const uint8_t *g[8];
  int gw[8], n = 0, cols = -1;
  for (const char *p = s; *p && n < 8;) {
    const uint32_t ch = halo_utf8_next(p);
    if (ch >= '0' && ch <= '9') g[n] = WG_GLYPH[ch - '0'], gw[n] = 5;
    else if (ch == ':') g[n] = WG_GLYPH[10], gw[n] = 1;
    else if (ch == '-') g[n] = MINUS, gw[n] = 3;
    else if (ch == 0xB0) g[n] = DEG, gw[n] = 3;
    else continue;
    cols += gw[n++] + 1;
  }
  uint8_t *f = ww_px_frame(c, j);
  int x = (c.pxw[j] - cols) / 2;
  for (int k = 0; k < n; k++) {
    for (int r = 0; r < 7; r++)
      for (int q = 0; q < gw[k]; q++)
        if (g[k][r] >> (gw[k] - 1 - q) & 1) ww_px_put(c, f, j, x + q, r, 1);
    x += gw[k] + 1;
  }
  c.pframe[j] = 1 - c.pframe[j];
}

// ---------- Wear: what goes with what (by the sky and the degrees) ----------
enum : uint8_t { WG_JACKET, WG_GLASSES, WG_HOODIE, WG_UMBRELLA, WG_HAT };  // the wear set's icons, in this order
static const char *const WW_SAY[5] = {"LIGHT JACKET", "SUNGLASSES ON", "GRAB A HOODIE", "TAKE AN UMBRELLA", "HAT AND GLOVES"};
static const uint32_t WW_SAY_C[5] = {0xFFB36B, 0xFFC94D, 0xAEB8C8, 0x8AB8FF, 0xFF8A80};
static int ww_garment(const WwSky &w) {
  float tc = w.now == WX_NONE ? 15 : w.now;
  if (WXD_F) tc = (tc - 32) * (5 / 9.0f);
  if (w.sky == WX_RAIN) return WG_UMBRELLA;
  if (w.sky == WX_SNOW || tc <= 4) return WG_HAT;
  if ((w.sky == WX_SUN || w.sky == WX_PARTLY) && tc >= 21) return WG_GLASSES;
  return tc <= 16 ? WG_HOODIE : WG_JACKET;
}

// ---------- Buddy: drawn of circles, arcs and lines (B — a point of the buddy, k — its size) ----------
struct WwB {
  float x, y, k;
  float X(float u) const { return x + u * k; }
  float Y(float u) const { return y + u * k; }
};
static void ww_face(WwRun &c, int &n, int &nc, const WwB &b, float fy, int mood, float t, float a, const HaloView &v) {
  // mood: 0 — sunglasses, 1 — happy, 2 — sleepy, 3 — sad
  const float k = b.k, ey = fy - 4, ex = 15;
  const bool blink = ww_wrap(t, 3.6f) < 0.13f;
  for (int sx = -1; sx <= 1; sx += 2) {
    const float x = sx * ex;
    if (mood == 0) {
      ww_rrect(c, n++, b.X(x), b.Y(ey), 26 * k, 18 * k, 8 * k, 0x141414, a, v, false);
      ww_cap(c, nc++, b.X(x - 7), b.Y(ey - 4), b.X(x - 1), b.Y(ey - 6), 1.25f * k, 1.25f * k, 0xFFFFFF, a * 0.7f, v);
    } else if (mood == 1) {  // ∩: an arc of a 6.5 px circle 4.5 px under the eye
      ww_arc(c, n++, b.X(x), b.Y(ey + 4.5f), 6.5f * k, 3.4f * k, -1.176f, 1.176f, WW_INK, a, v, false);
    } else if (mood == 2) {
      ww_rrect(c, n++, b.X(x), b.Y(ey + 0.5f), 12 * k, 3.4f * k, 1.7f * k, WW_INK, a, v, false);
    } else {
      if (blink) ww_rrect(c, n++, b.X(x), b.Y(ey + 2), 8 * k, 3 * k, 1.5f * k, WW_INK, a, v, false);
      else ww_circle(c, n++, b.X(x), b.Y(ey + 2), 4.3f * k, WW_INK, a, v, false);
      ww_cap(c, nc++, b.X(x - 7 * sx), b.Y(ey - 12), b.X(x + 6 * sx), b.Y(ey - 9), 1.5f * k, 1.5f * k, WW_INK, a, v);
    }
  }
  if (mood == 0) ww_rrect(c, n++, b.X(0), b.Y(ey - 3), 6 * k, 3 * k, 1.5f * k, 0x141414, a, v, false);
  if (mood <= 1) ww_arc(c, n++, b.X(0), b.Y(fy + 3.25f), 11.25f * k, 3.4f * k, HALO_PI - 0.927f, HALO_PI + 0.927f, WW_INK, a, v, false);
  else if (mood == 2) ww_circle(c, n++, b.X(0), b.Y(fy + 13), (4 + sinf(ww_wrap(t * 1.4f, WW_TAU)) * 1.5f) * k, WW_INK, a, v, false);
  else ww_arc(c, n++, b.X(0), b.Y(fy + 23.4f), 10.9f * k, 3.2f * k, -0.823f, 0.823f, WW_INK, a, v, false);
  if (mood != 3)
    for (int sx = -1; sx <= 1; sx += 2) ww_circle(c, n++, b.X(sx * 24), b.Y(fy + 8), 5.5f * k, 0xFF8A6B, a * 0.45f, v, false);
}
static void ww_cloud_body(WwRun &c, int &n, const WwB &b, uint32_t col, float a, const HaloView &v) {
  const float k = b.k;
  ww_circle(c, n++, b.X(-38), b.Y(8), 30 * k, col, a, v, false);
  ww_circle(c, n++, b.X(0), b.Y(-14), 40 * k, col, a, v, false);
  ww_circle(c, n++, b.X(38), b.Y(6), 30 * k, col, a, v, false);
  ww_rrect(c, n++, b.X(0), b.Y(23), 132 * k, 34 * k, 17 * k, col, a, v, false);
}
#ifdef HALO_P4_TALL
// The tall curve's line over it, "18° · PARTLY CLOUDY" centered on cx (rect-tall.html 12.2): the degrees in the curve's
// own font at 24 px, a dot, the sky's word in its small one (whose letters have no "°" or "·")
static void ww_deg_word(WwRun &c, int &n, const FaceLayer &L, const WwSky &w, float cx, float cy, float a, const HaloView &v) {
  if (!L.font || !L.font2) return;
  wx_deg(c.txt[19], 40, w.now);
  const float sd = 24.0f / L.font->size, w0 = halo_text_width(*L.font, c.txt[19]) * sd, w1 = halo_text_width(*L.font2, WW_WORD[w.sky], 3);
  const float x0 = cx - (w0 + 26 + w1) / 2;
  ww_text(c, n++, L.font, c.mid, c.txt[19], x0, cy, 0, L.color2, a, 0, v, c.ob, sd);
  ww_circle(c, n++, x0 + w0 + 13, cy, 2.2f, L.color2, a, v, c.ob);
  ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], x0 + w0 + 26, cy, 0, L.color2, a, 3, v, c.ob);
}
#endif

// ---------- Each frame ----------
// Where a widget stands before it is moved (its anchor) — on the 7-inch board in its layout (p3: 0 — the first)
static void ww_base(uint8_t kind, int look, float &x, float &y) {
#if WW_RECT
#ifdef HALO_P4_TALL
  // (rect-tall.html 12, the 7″ board held tall — p3 0, 1 by the layouts' slots: digits 12.1A (the clock over them),
  // 12.1B (they first); curve 12.2A (across), 12.2B (upright: its middle); barometer 12.3A (the dial), 12.3B (the
  // ruler); wear 12.4A (the garment's point), 12.4B; buddy 12.5A (its middle), 12.5B (peeking); pixels 12.6A, 12.6B (the
  // scene's top); week 12.7B (seven columns), 12.7A (seven rows) — the clock moves where the mockup has it: the moves)
  static const float XY[7][2][2] = {{{300, 560}, {300, 392}}, {{315, 680}, {370, 684}}, {{300, 700}, {300, 620}},
                                    {{300, 556}, {146, 652}}, {{300, 660}, {300, 1000}}, {{300, 384}, {300, 472}},
                                    {{300, 700}, {300, 660}}};
#else
#ifdef HALO_WATCH
  // (watch206-widgets.html, the watch: one layout a widget — digits 5A, curve 6A, barometer 7B (the ruler), wear 8A,
  // buddy 9A, pixels 10A, week 11B (five rows), all under the clock but the pixels; the clock moves where the mockup
  // has it: the passport's moves)
  static const float XY[7][2][2] = {{{205, 300}, {205, 300}}, {{205, 310}, {205, 310}}, {{205, 304}, {205, 304}},
                                    {{205, 292}, {205, 292}}, {{195, 330}, {195, 330}}, {{205, 250}, {205, 250}},
                                    {{205, 228}, {205, 228}}};
#elif defined(HALO_SQUARE)
  // (square-widgets.html, the square: one layout a widget — digits 1A, curve 2A, barometer 3B, wear 4A, buddy 5A,
  // pixels 6A, week 7A, all under the clock but the pixels; the clock moves where the mockup has it: the passport's moves)
  static const float XY[7][2][2] = {{{240, 300}, {240, 300}}, {{248, 330}, {248, 330}}, {{240, 316}, {240, 316}},
                                    {{240, 298}, {240, 298}}, {{232, 344}, {232, 344}}, {{240, 210}, {240, 210}},
                                    {{240, 300}, {240, 300}}};
#elif defined(HALO_COMPACT)
  // (wt32-widgets.html, the WT32 wide: digits 1A, 1B; curve 2A, 2B; barometer 3A, 3B; wear 4A, 4B; buddy 5A, 5B; pixels
  // 6A, 6B; week 7A, 7B — the digits' middle, the curve's and the week's, the dial's, the garment's point, the buddy's,
  // the pixel scene's top; the clock moves where the mockup has it: the passport's moves)
  static const float XY[7][2][2] = {{{354, 148}, {146, 236}}, {{256, 200}, {371, 180}}, {{348, 194}, {240, 216}},
                                    {{356, 138}, {96, 238}}, {{334, 180}, {268, 268}}, {{240, 140}, {240, 168}},
                                    {{240, 200}, {362, 160}}};
#else
  // (weather-widgets-rect.html: digits 1A; curve 2A, 2B; barometer 3A, 3B; wear 4A, 4B; buddy 5A, 5B; pixels 6A, 6B;
  // week 7A, 7B — the clock moves where the mockup has it: the passport's moves)
  static const float XY[7][2][2] = {{{772, 278}, {772, 278}}, {{520, 410}, {777, 300}}, {{762, 340}, {512, 470}},
                                    {{752, 262}, {318, 438}}, {{771, 324}, {568, 520}}, {{512, 300}, {512, 460}},
                                    {{512, 455}, {778, 339}}};
#endif
#endif
  const int k = constrain(kind - FL_WXDIGITS, 0, 6), j = look ? 1 : 0;
  x = XY[k][j][0], y = XY[k][j][1];
#else
  (void)look;
  x = CX;
  y = kind == FL_WXDIGITS ? 262 : kind == FL_WXBARO ? CY : kind == FL_WXPIXEL ? 260 : 300;
#endif
}
static void ww_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  float ax, ay;
#ifdef HALO_WATCH  // (the watch has one layout a widget, whatever p3 says: the code's look 0 — but the ruler and the rows, 1)
  const int look = L.kind == FL_WXBARO || L.kind == FL_WXWEEK ? 1 : 0;
#elif defined(HALO_SQUARE)  // (the square has one layout a widget, whatever p3 says: the code's look 0 — but the ruler, look 1)
  const int look = L.kind == FL_WXBARO ? 1 : 0;
#else
  const int look = WW_RECT ? (int)L.p3 : 0;  // (the 7-inch board's layouts; the watch has one)
#endif
  ww_base(L.kind, look, ax, ay);
  ax += L.dx, ay += L.dy;
  R.ax = ax, R.ay = ay;
  WwRun *cp = R.ww;
  if (!cp) return;
  WwRun &c = *cp;
  const uint32_t now = halo_ms();
  c.dt = c.last_ms ? fminf(0.05f, (now - c.last_ms) * 0.001f) : 0;
  c.last_ms = now;
  if (!fc_calm) c.ms = now;  // (Always On: nothing moves)
  if (a * v.a <= 0.004f) {   // not seen: its parts go away once, nothing else
    if (c.vis) {
      c.a = 0;
      ww_hide(c, 0, v), ww_hide_caps(c, 0);
      for (WwArea &q : c.area) ww_area(q, false, 0, 0, -1, -1, 0);
      c.vis = false;
    }
    return;
  }
  c.vis = true;
  c.ob = !face_over_others(S, i);
  c.s = v.s, c.inv = 1 / v.s, c.a = a * v.a, c.vx = v.x, c.vy = v.y;
  c.over = WW_ITEMS;
  const WwSky w = ww_sky();
  const float t = (c.ms % 600000u) * 0.001f;  // (seconds, within 10 minutes: sinf of a big angle is slow)
  const float k = L.k > 0 ? L.k : 1;
  int n = 0, nc = 0;
  bool area = false;
  switch (L.kind) {
#if WW_ON(WW_DIGITS)
    case FL_WXDIGITS: {
      if (!L.font) break;
      wx_deg(c.txt[0], 40, w.now);
      float pen = 0, i0 = 1e9f, i1 = -1e9f, gp[6];
      const HaloMask *g[6];
      int ng = 0;
      for (const char *p = c.txt[0]; *p && ng < 6;) {
        const HaloMask *m = halo_glyph(*L.font, halo_utf8_next(p));
        if (!m) continue;
        if (m->w) i0 = fminf(i0, pen + m->ox), i1 = fmaxf(i1, pen + m->ox + m->w);
        g[ng] = m, gp[ng++] = pen;
        pen += m->adv + L.spacing;
      }
      const float left = ax - (i0 <= i1 ? (i0 + i1) * 0.5f : pen * 0.5f), top = ay - c.mid;
      float b[4] = {1e9f, 1e9f, -1e9f, -1e9f};
      c.ng = 0;
      for (int j = 0; j < ng; j++) {
        if (!g[j]->w) continue;
        const float x = left + gp[j] + g[j]->ox, y = top + g[j]->oy;
        c.gl[c.ng] = g[j], c.gx[c.ng] = halo_view_x(v, x), c.gy[c.ng] = halo_view_y(v, y);
        c.gxi[c.ng] = halo_floor(c.gx[c.ng] + 0.5f), c.gyi[c.ng] = halo_floor(c.gy[c.ng] + 0.5f), c.ng++;  // (as halo_mask_rows)
        b[0] = fminf(b[0], x), b[1] = fminf(b[1], y), b[2] = fmaxf(b[2], x + g[j]->w), b[3] = fmaxf(b[3], y + g[j]->h);
      }
      if (!c.ng) break;
      c.fast = fabsf(v.s - 1) < 1e-4f;
      const float X0 = halo_view_x(v, b[0]), Y0 = halo_view_y(v, b[1]), X1 = halo_view_x(v, b[2]), Y1 = halo_view_y(v, b[3]);
      static const uint8_t FPS[5] = {12, 8, 12, 30, 20};  // how often each sky moves a step
      const uint32_t step = (uint32_t)((uint64_t)c.ms * FPS[w.sky] / 1000);
      if (step != c.step || w.sky != c.sky || X0 != c.d[0] || Y0 != c.d[1] || X1 != c.d[2] || Y1 != c.d[3]) {
        c.step = step, c.sky = w.sky, c.d[0] = X0, c.d[1] = Y0, c.d[2] = X1, c.d[3] = Y1;
        ww_sky_shapes(c, w.sky, (step % (FPS[w.sky] * 600u)) / (float)FPS[w.sky], X0, Y0, X1 - X0, Y1 - Y0, v.s);
      }
      uint32_t h = ww_hashu(ww_hashu(2166136261u, step), w.sky);
      h = ww_hash(h, X0), h = ww_hash(h, Y0), h = ww_hash(h, X1), h = ww_hash(h, Y1), h = ww_hash(h, c.a * 64);
      for (const char *p = c.txt[0]; *p; p++) h = ww_hashu(h, (uint8_t)*p);
      ww_area(c.area[0], true, X0 - 1, Y0 - 1, X1 + 1, Y1 + 1, h), area = true;
      if (L.p2 < 0.5f) {  // the sky's word, the day's high, low, feels like
        char hi[8], lo[8], fe[8];
        wx_deg(hi, sizeof(hi), w.hi), wx_deg(lo, sizeof(lo), w.lo), wx_deg(fe, sizeof(fe), w.feels);
        snprintf(c.txt[1], 40, "H %s · L %s · FEELS %s", hi, lo, fe);
#ifdef HALO_P4_TALL  // (tall: the words under the digits — 12.1A at 290 px, 12.1B at 330)
        ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], ax, ay + (look ? 218 : 212), 1, L.color, a, 4, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ax, ay + (look ? 264 : 258), 1, L.color2, a, 0, v, c.ob);
#else
#ifdef HALO_WATCH  // (the watch's 5A: under the digits, which are under the clock)
        ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], ax, ay + 94, 1, L.color, a, 2, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ax, ay + 122, 1, L.color2, a, 0, v, c.ob);
#elif defined(HALO_SQUARE)  // (the square's 1A: under the digits, which are under the clock)
        ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], ax, ay + 98, 1, L.color, a, 2, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ax, ay + 124, 1, L.color2, a, 0, v, c.ob);
#elif defined(HALO_COMPACT)  // (the WT32 wide: 1A — under the digits; 1B — beside them, from past their right edge)
        const float lx = look ? fmaxf(ax + 86, b[2] + 16) : ax;
        ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], lx, ay + (look ? -14 : 77), look ? 0 : 1, L.color, a, 2, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], lx, ay + (look ? 12 : 102), look ? 0 : 1, L.color2, a, 0, v, c.ob);
#else
        ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], ax, ay + (WW_RECT ? 174 : 124), 1, L.color, a, 3, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ax, ay + (WW_RECT ? 218 : 156), 1, L.color2, a, 0, v, c.ob);
#endif
#endif
      }
      break;
    }
#endif
#if WW_ON(WW_CURVE)
    case FL_WXCURVE: {
      if (!c.col_y || !c.col_c) break;
      // Its geometry from the anchor: half the width, the line's bottom and height, the icons' row, the hours', how far
      // the light under the line goes, every how many hours, the sky's word, the line's half thickness, the dots' size —
      // the watch's; the 7-inch board's band under the clock (2A) and beside it (2B: the word under the clock)
#ifdef HALO_P4_TALL
      // (the 7″ board held tall: 12.2A across; 12.2B upright — down the hours, so its numbers are turned: half the
      // height, the cold end's x and minus the width (the line's x = G[1] - (T - min) · G[2] / span), the icons' x, the
      // hours', the light's end x, every 2 hours, the line over it, the line's half thickness, the dots' size)
      static const float GEO[3][11] = {{137, 44, 78, -70, 76, 58, 3, 0, 108, 2.25f, 1},
                                       {219, 40, 160, -194, 102, 54, 3, -15, -340, 3, 1.35f},
                                       {264, -108, -216, -212, -330, -138, 2, -70, -362, 3, 1.35f}};
#else
#ifdef HALO_WATCH  // (the watch's 6A: the 12 hours 330 px wide under the clock — watch206-widgets.html)
      static const float GEO[3][11] = {{137, 44, 78, -70, 76, 58, 3, 0, 108, 2.25f, 1},
                                       {165, 24, 54, -74, 66, 32, 3, 0, 110, 1.7f, 0.8f},
                                       {165, 24, 54, -74, 66, 32, 3, 0, 110, 1.7f, 0.8f}};
#elif defined(HALO_SQUARE)  // (the square's 2A: the 12 hours 396 px wide under the clock — square-widgets.html)
      static const float GEO[3][11] = {{137, 44, 78, -70, 76, 58, 3, 0, 108, 2.25f, 1},
                                       {198, 22, 52, -76, 68, 30, 2, 0, 102, 1.7f, 0.8f},
                                       {198, 22, 52, -76, 68, 30, 2, 0, 102, 1.7f, 0.8f}};
#elif defined(HALO_COMPACT)  // (the WT32 wide's 2A and 2B: wt32-widgets.html)
      static const float GEO[3][11] = {{137, 44, 78, -70, 76, 58, 3, 0, 108, 2.25f, 1},
                                       {194, 22, 40, -54, 62, 28, 2, 0, 89, 1.6f, 0.75f},
                                       {79, 22, 48, -68, 60, 28, 3, -249, 40, 1.5f, 0.7f}};
#else
      static const float GEO[3][11] = {{137, 44, 78, -70, 76, 58, 3, 0, 108, 2.25f, 1},
                                       {380, 38, 76, -88, 90, 52, 2, 0, 130, 3, 1.4f},
                                       {189, 40, 78, -90, 92, 54, 3, -515, 92, 3, 1.35f}};
#endif
#endif
      const float *G = GEO[WW_RECT ? (look ? 2 : 1) : 0];
      const float hw = G[0], q = G[10];
      const int ncol = min(c.col_n, (int)(2 * hw) + 1), every = (int)G[6];
      float T[13], mn = 1e9f, mx = -1e9f;
      for (int j = 0; j < 13; j++) T[j] = ww_hour_t(w, j), mn = fminf(mn, T[j]), mx = fmaxf(mx, T[j]);
      mn -= 1, mx += 1;
      const float yk = G[2] / (mx - mn);
      auto Y = [&](float u) { return G[1] - (u - mn) * yk; };  // (from the anchor)
      uint32_t sig = ww_hash(2166136261u, hw);
      for (float u : T) sig = ww_hash(sig, u);
      if (sig != c.col_sig) {  // the line again: Catmull-Rom through the hours, a column a pixel
        c.col_sig = sig, c.d[4] = 1e9f;
        const float per = 12.0f / (ncol - 1);
        for (int x = 0; x < ncol; x++) {
          const float u = x * per;
          const int sg = min(11, (int)u);
          const float f = u - sg, p0 = Y(T[max(0, sg - 1)]), p1 = Y(T[sg]), p2 = Y(T[sg + 1]), p3 = Y(T[min(12, sg + 2)]);
          c.col_y[x] = 0.5f * (2 * p1 + (p2 - p0) * f + (2 * p0 - 5 * p1 + 4 * p2 - p3) * f * f + (3 * p1 - p0 - 3 * p2 + p3) * f * f * f);
          c.col_c[x] = ww_tcol(T[sg] + (T[sg + 1] - T[sg]) * f);
          c.d[4] = fminf(c.d[4], c.col_y[x]);
        }
      }
      c.d[0] = ax, c.d[1] = ay, c.d[2] = hw, c.d[3] = G[5], c.d[5] = G[9], c.d[6] = ncol;
#ifdef HALO_P4_TALL
      c.vert = look;
      if (look) {  // (12.2B: upright — an hour a step down, the sky and the hour at the left, the degrees right of the line)
        const float Y0 = ay - hw, st = 2 * hw / 12, fs = L.font ? L.font->size : 18, hs = 0.67f;  // (the hours: 16 px)
        uint32_t h = ww_hash(ww_hash(ww_hash(sig, ax), ay), c.a * 64);
        h = ww_hash(ww_hash(ww_hash(h, v.x), v.y), v.s * 64);
        ww_area(c.area[0], true, halo_view_x(v, ax + G[5] - 2), halo_view_y(v, Y0 - 8), halo_view_x(v, ax + G[1] - G[2] + 8),
                halo_view_y(v, Y0 + 2 * hw + 8), h);
        area = true;
        struct tm tm;
        const bool ok = wg_now(tm);
        const float gl = G[3] + 26 * k, gr = G[5];  // (the guides: from right of the icons to the light's end)
        for (int j = 0; j <= 12; j += every) ww_rrect(c, n++, ax + (gl + gr) / 2, Y0 + j * st, gr - gl, 1.2f * q, 0.6f * q, L.color2, a * 0.35f, v, c.ob);
        c.over = n;
        for (int j = 0, m = 0; j <= 12; j += every, m++) {
          const float y = Y0 + j * st;
          ww_image(c, n++, L.icons ? L.icons[ww_hour_sky(w, j)] : nullptr, ax + G[3], y, a, v);
          if (!j) snprintf(c.txt[m], 40, "NOW");
          else if (ok) ww_hour_name(c.txt[m], 40, tm.tm_hour + j);
          else snprintf(c.txt[m], 40, "--");
          ww_text(c, n++, L.font2, c.mid2, c.txt[m], ax + G[4], y, 0, j ? L.color2 : L.color, a, 1, v, c.ob, hs);
        }
        int imax = 1, imin = 1;
        for (int j = 1; j <= 12; j++) {
          if (T[j] > T[imax]) imax = j;
          if (T[j] <= T[imin]) imin = j;
        }
        if (T[imax] > T[0]) {
          const float x = ax + Y(T[imax]), y = Y0 + imax * st;
          wx_deg(c.txt[16], 40, (int16_t)lroundf(T[imax]));
          ww_circle(c, n++, x, y, 4 * q, L.color, a, v, false);
          ww_text(c, n++, L.font, c.mid, c.txt[16], x + fs * 0.8f, y, 0, L.color, a, 0, v, c.ob);
        }
        if (T[imin] < T[0]) {
          const float x = ax + Y(T[imin]), y = Y0 + imin * st;
          wx_deg(c.txt[17], 40, (int16_t)lroundf(T[imin]));
          ww_circle(c, n++, x, y, 4 * q, L.color2, a, v, false);
          ww_text(c, n++, L.font, c.mid, c.txt[17], x + fs * 0.8f, y, 0, L.color2, a, 0, v, c.ob);
        }
        if (!fc_calm) {  // the light running down the line (5 s)
          const float u = (c.ms % 5000) * (12 / 5000.0f), gy = Y0 + u * st;
          const float gx = ax + c.col_y[min(ncol - 1, (int)(u * st))], gop = fminf(1, fminf(u * 2, (12 - u) * 2));
          ww_circle(c, n++, gx, gy, 9 * q, 0xFFFFFF, a * 0.18f * gop, v, false);
          ww_circle(c, n++, gx, gy, 3.5f * q, 0xFFFFFF, a * 0.9f * gop, v, false);
        }
        const float x0 = ax + c.col_y[0];
        ww_circle(c, n++, x0, Y0, 10 * q, 0x000000, a, v, false);
        ww_circle(c, n++, x0, Y0, 7 * q, L.color, a, v, false);
        wx_deg(c.txt[18], 40, (int16_t)lroundf(T[0]));
        ww_text(c, n++, L.font, c.mid, c.txt[18], x0 + fs * 0.9f, Y0, 0, L.color, a, 0, v, c.ob);
        ww_deg_word(c, n, L, w, ax + G[7], ay + G[8], a, v);
        break;
      }
#endif
      const float X0 = ax - hw, st = 2 * hw / 12, fs = L.font ? L.font->size : 18;
      uint32_t h = ww_hash(ww_hash(ww_hash(sig, ax), ay), c.a * 64);
      h = ww_hash(ww_hash(ww_hash(h, v.x), v.y), v.s * 64);
      ww_area(c.area[0], true, halo_view_x(v, X0 - 4), halo_view_y(v, ay + c.d[4] - 5), halo_view_x(v, X0 + 2 * hw + 4),
              halo_view_y(v, ay + G[5] + 2), h);
      area = true;
      struct tm tm;
      const bool ok = wg_now(tm);
#ifdef HALO_P4_TALL
      const float gt = G[3] + 20 * k, gb = G[5] - 2;  // (the guides: from under the icons — as big as k — to the light's end)
#else
#ifdef HALO_COMPACT
      const float gt = G[3] + 15 * k, gb = G[5] - 2;  // (the guides: from under the icons — as big as k — to the light's end)
#else
      const float gt = G[3] + 24 * q, gb = G[5] - 2;  // (the guides: from under the icons to the light's end)
#endif
#endif
      for (int j = 0; j <= 12; j += every) ww_rrect(c, n++, X0 + j * st, ay + (gt + gb) / 2, 1.2f * q, gb - gt, 0.6f * q, L.color2, a * 0.35f, v, c.ob);
      c.over = n;
      for (int j = 0, m = 0; j <= 12; j += every, m++) {
        const float x = X0 + j * st;
        ww_image(c, n++, L.icons ? L.icons[ww_hour_sky(w, j)] : nullptr, x, ay + G[3], a, v);
        if (!j) snprintf(c.txt[m], 40, "NOW");
        else if (ok) ww_hour_name(c.txt[m], 40, tm.tm_hour + j);
        else snprintf(c.txt[m], 40, "--");
#ifdef HALO_P4_TALL  // (tall: the small font is the line over the curve's, 24 px — the hours at 16)
        ww_text(c, n++, L.font2, c.mid2, c.txt[m], x, ay + G[4], 1, j ? L.color2 : L.color, a, 1, v, c.ob, 0.67f);
#else
        ww_text(c, n++, L.font2, c.mid2, c.txt[m], x, ay + G[4], 1, j ? L.color2 : L.color, a, 1, v, c.ob);
#endif
      }
      int imax = 1, imin = 1;
      for (int j = 1; j <= 12; j++) {
        if (T[j] > T[imax]) imax = j;
        if (T[j] <= T[imin]) imin = j;
      }
      if (T[imax] > T[0]) {
        const float x = X0 + imax * st, y = ay + Y(T[imax]);
        wx_deg(c.txt[16], 40, (int16_t)lroundf(T[imax]));
        ww_circle(c, n++, x, y, 4 * q, L.color, a, v, false);
        ww_text(c, n++, L.font, c.mid, c.txt[16], x, y - fs * 1.05f, 1, L.color, a, 0, v, c.ob);
      }
      if (T[imin] < T[0]) {
        const float x = X0 + imin * st, y = ay + Y(T[imin]);
        wx_deg(c.txt[17], 40, (int16_t)lroundf(T[imin]));
        ww_circle(c, n++, x, y, 4 * q, L.color2, a, v, false);
        ww_text(c, n++, L.font, c.mid, c.txt[17], x, y + fs * 1.05f, 1, L.color2, a, 0, v, c.ob);
      }
      if (!fc_calm) {  // the light running along the line (5 s)
        const float u = (c.ms % 5000) * (12 / 5000.0f), gx = X0 + u * st;
        const float gy = ay + c.col_y[min(ncol - 1, (int)(u * st))], gop = fminf(1, fminf(u * 2, (12 - u) * 2));
        ww_circle(c, n++, gx, gy, 9 * q, 0xFFFFFF, a * 0.18f * gop, v, false);
        ww_circle(c, n++, gx, gy, 3.5f * q, 0xFFFFFF, a * 0.9f * gop, v, false);
      }
      const float y0 = ay + c.col_y[0];
      ww_circle(c, n++, X0, y0, 10 * q, 0x000000, a, v, false);
      ww_circle(c, n++, X0, y0, 7 * q, L.color, a, v, false);
      wx_deg(c.txt[18], 40, (int16_t)lroundf(T[0]));
      ww_text(c, n++, L.font, c.mid, c.txt[18], X0 - fs * 0.78f, y0, 2, L.color, a, 0, v, c.ob);
#ifdef HALO_P4_TALL
      ww_deg_word(c, n, L, w, ax + G[7], ay + G[8], a, v);
#else
#ifdef HALO_COMPACT
      ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], ax + G[7], ay + G[8], 1, L.color2, a, 2, v, c.ob);
#else
      ww_text(c, n++, L.font2, c.mid2, WW_WORD[w.sky], ax + G[7], ay + G[8], 1, L.color2, a, 3, v, c.ob);
#endif
#endif
      break;
    }
#endif
#if WW_ON(WW_BARO)
    case FL_WXBARO: {
      static const int16_t DEMO_P[4] = {1013, 1024, 1004, 988};  // a demo day's (wx N), by its sky
      static float tick[41][2];  // the ticks' directions (sin, cos), once
      if (!tick[0][1]) for (int j = 0; j < 41; j++) tick[j][0] = sinf(ww_baro(960 + j * 2)), tick[j][1] = cosf(ww_baro(960 + j * 2));
      int P = w.real ? w.c->pres : DEMO_P[w.sky & 3];
      const int k3 = min(w.k0 + 3, WX_AHEAD - 1);
      int P3 = w.real && w.c->t0 && w.c->hp[k3] ? 900 + w.c->hp[k3] : P;
      const bool known = P != WX_NONE;
      if (!known) P = P3 = 1013;
      if (!c.needle_set) halo_spring_set(c.needle, P), c.needle_set = true;  // (the needle's spring: in hPa)
      c.needle.target = P;
      halo_spring_run(c.needle, c.nk, c.dt);
      int best = 0;
      for (int j = 1; j < 5; j++)
        if (abs(WW_AT[j] - P) < abs(WW_AT[best] - P)) best = j;
#ifdef HALO_P4_TALL
      // The 7″ board held tall (rect-tall.html 12.3): A — the old dial, 206 px, under the clock (the degrees, hPa and the
      // trend between); B — the ruler 480 px wide under the clock, 6 px a hPa (the degrees, hPa and the trend under it)
      const bool ruler = look;
      if (!ruler) {
        ww_arc(c, n++, ax, ay, 206, 1.5f, ww_rad(-125), ww_rad(125), ww_mix(L.color2, 0, 0.4f), a, v, c.ob);
        for (int j = 0; j < 5; j++) {
          const HaloMask &m = c.word[j];
          ww_mask(c, n++, m.bits ? &m : nullptr, ax + c.wx[j] + m.w * 0.5f, ay + c.wy[j] + m.h * 0.5f,
                  j == best && known ? 0xFFF8EE : L.color2, a, v);
        }
        for (int j = 0; j < 41; j++) {  // every 2 hPa, longer every 10
          const bool major = j % 5 == 0;
          const float sn = tick[j][0], cs = tick[j][1], r1 = major ? 189.2f : 197.6f, hw = major ? 1.37f : 0.79f;
          ww_cap(c, nc++, ax + 206 * sn, ay - 206 * cs, ax + r1 * sn, ay - r1 * cs, hw, hw, major ? 0xFFF8EE : L.color2, a, v);
        }
        const float b3 = ww_baro(P3), bs = sinf(b3), bc = cosf(b3), an = ww_baro(c.needle.x), sn = sinf(an), cs = cosf(an);
        ww_cap(c, nc++, ax, ay, ax + 154.5f * bs, ay - 154.5f * bc, 1.58f, 1.58f, L.color3, known ? a : 0, v);  // the brass hand
        ww_ring(c, n++, ax + 160.8f * bs, ay - 160.8f * bc, 6.3f, 2.6f, L.color3, known ? a : 0, v);
        ww_cap(c, nc++, ax - 30.5f * sn, ay + 30.5f * cs, ax + 179.7f * sn, ay - 179.7f * cs, 5.3f, 1.2f, L.color, a * (known ? 1 : 0.35f), v);
        c.over = n;
        ww_circle(c, n++, ax, ay, 12.6f, 0x1C1D20, a, v, false);
        ww_ring(c, n++, ax, ay, 11, 3.2f, L.color, a, v);
        ww_circle(c, n++, ax, ay, 3.7f, 0xFFF8EE, a, v, false);
      } else {  // 960 … 1040 hPa over 480 px; the words over it, the numbers under it; now — a coral bar, in 3 hours — brass
        auto X = [&](float p) { return ax - 240 + (fminf(1040, fmaxf(960, p)) - 960) * 6; };
        ww_cap(c, nc++, X(960), ay, X(1040), ay, 1, 1, ww_mix(L.color2, 0, 0.4f), a, v);
        for (int p = 960, j = 0; p <= 1040; p += 2, j++) {
          const bool major = j % 5 == 0;
          const float hw = major ? 1.3f : 0.75f;
          ww_cap(c, nc++, X(p), ay, X(p), ay + (major ? 22 : 11), hw, hw, major ? 0xFFF8EE : L.color2, a, v);
        }
        for (int j = 0; j < 5; j++) {
          snprintf(c.txt[2 + j], 40, "%d", 960 + j * 20);
          ww_text(c, n++, L.font2, c.mid2, c.txt[2 + j], X(960 + j * 20), ay + 46, 1, L.color2, a, 0, v, c.ob, 0.57f);
          ww_text(c, n++, L.font, c.mid, WW_WORDS[j], X(WW_AT[j]), ay - 54, 1, j == best && known ? 0xFFF8EE : L.color2, a, 2, v, c.ob);
        }
        const float x3 = X(P3), xn = X(c.needle.x), ka = known ? a : 0;
        ww_cap(c, nc++, x3 - 9, ay - 24, x3 + 9, ay - 24, 1.25f, 1.25f, L.color3, ka, v);
        ww_cap(c, nc++, x3 + 9, ay - 24, x3, ay - 8, 1.25f, 1.25f, L.color3, ka, v);
        ww_cap(c, nc++, x3, ay - 8, x3 - 9, ay - 24, 1.25f, 1.25f, L.color3, ka, v);
        c.over = n;
        ww_rrect(c, n++, xn, ay - 2, 6, 56, 3, L.color, a * (known ? 1 : 0.35f), v, false);
        ww_circle(c, n++, xn, ay - 30, 9, L.color, a * (known ? 1 : 0.35f), v, false);
      }
      wx_deg(c.txt[0], 40, w.now);
      if (known) snprintf(c.txt[1], 40, "%d hPa", P);
      else snprintf(c.txt[1], 40, "-- hPa");
      const float w0 = L.font2 ? halo_text_width(*L.font2, c.txt[0]) : 0, w1 = L.font2 ? halo_text_width(*L.font2, c.txt[1]) : 0;
      const float iy = ruler ? ay + 152 : ay - 378, x0 = ax - (w0 + 12 + w1) / 2;  // (both: centered under the clock)
      ww_text(c, n++, L.font2, c.mid2, c.txt[0], x0, iy, 0, 0xFFF8EE, a, 0, v, c.ob);
      ww_text(c, n++, L.font2, c.mid2, c.txt[1], x0 + w0 + 12, iy, 0, L.color2, a, 0, v, c.ob);
      const int dp = P3 - P, tr = !known ? 0 : dp >= 2 ? 1 : dp <= -2 ? -1 : 0;
      static const char *const TREND[3] = {"FALLING", "STEADY", "RISING"};
      const uint32_t tc = tr > 0 ? 0xA8C7FA : tr < 0 ? 0xFF8A80 : L.color2;
      const float ty = iy + 48;
      const float tw = L.font ? halo_text_width(*L.font, TREND[tr + 1], 2) : 0, tx = ax - (tw + 18) / 2;
      if (tr) {
        const float d = tr < 0 ? 1 : -1;  // (down — falling)
        ww_cap(c, nc++, tx + 4, ty - 7 * d, tx + 4, ty + 7 * d, 1.3f, 1.3f, tc, a, v);
        ww_cap(c, nc++, tx - 2, ty + d, tx + 4, ty + 7 * d, 1.3f, 1.3f, tc, a, v);
        ww_cap(c, nc++, tx + 10, ty + d, tx + 4, ty + 7 * d, 1.3f, 1.3f, tc, a, v);
      } else {
        ww_cap(c, nc++, tx - 2, ty, tx + 10, ty, 1.3f, 1.3f, tc, a, v);
      }
      ww_text(c, n++, L.font, c.mid, TREND[tr + 1], tx + 18, ty, 0, tc, a, 2, v, c.ob);
#else
#ifdef HALO_COMPACT
      // The WT32 wide (wt32-widgets.html): 3A — the old dial, 90 px, beside the clock (the degrees, hPa and the trend
      // under the clock); 3B — a ruler 400 px wide under the clock (the degrees, hPa and the trend under it)
      const bool ruler = look;
      if (!ruler) {
        ww_arc(c, n++, ax, ay, 90, 1.2f, ww_rad(-125), ww_rad(125), ww_mix(L.color2, 0, 0.4f), a, v, c.ob);
        for (int j = 0; j < 5; j++) {
          const HaloMask &m = c.word[j];
          ww_mask(c, n++, m.bits ? &m : nullptr, ax + c.wx[j] + m.w * 0.5f, ay + c.wy[j] + m.h * 0.5f,
                  j == best && known ? 0xFFF8EE : L.color2, a, v);
        }
        for (int j = 0; j < 41; j++) {  // every 2 hPa, longer every 10
          const bool major = j % 5 == 0;
          const float sn = tick[j][0], cs = tick[j][1], r1 = major ? 82.6f : 86.3f, hw = major ? 0.8f : 0.5f;
          ww_cap(c, nc++, ax + 90 * sn, ay - 90 * cs, ax + r1 * sn, ay - r1 * cs, hw, hw, major ? 0xFFF8EE : L.color2, a, v);
        }
        const float b3 = ww_baro(P3), bs = sinf(b3), bc = cosf(b3), an = ww_baro(c.needle.x), sn = sinf(an), cs = cosf(an);
        ww_cap(c, nc++, ax, ay, ax + 67.5f * bs, ay - 67.5f * bc, 0.8f, 0.8f, L.color3, known ? a : 0, v);  // the brass hand
        ww_ring(c, n++, ax + 70.2f * bs, ay - 70.2f * bc, 3.6f, 1.6f, L.color3, known ? a : 0, v);
        ww_cap(c, nc++, ax - 13.3f * sn, ay + 13.3f * cs, ax + 78.5f * sn, ay - 78.5f * cs, 3, 0.7f, L.color, a * (known ? 1 : 0.35f), v);
        c.over = n;
        ww_circle(c, n++, ax, ay, 8, 0x1C1D20, a, v, false);
        ww_ring(c, n++, ax, ay, 7, 2, L.color, a, v);
        ww_circle(c, n++, ax, ay, 2.2f, 0xFFF8EE, a, v, false);
      } else {  // 960 … 1040 hPa over 400 px; the words over it, the numbers under it; now — a coral bar, in 3 hours — brass
#ifdef HALO_WATCH
        const float pp = 4;  // (the watch's 7B: 320 px, 4 px a hPa)
#else
        const float pp = 5;
#endif
        auto X = [&](float p) { return ax - 40 * pp + (fminf(1040, fmaxf(960, p)) - 960) * pp; };
        ww_cap(c, nc++, X(960), ay, X(1040), ay, 0.7f, 0.7f, ww_mix(L.color2, 0, 0.4f), a, v);
        for (int p = 960, j = 0; p <= 1040; p += 2, j++) {
          const bool major = j % 5 == 0;
          const float hw = major ? 0.8f : 0.5f;
          ww_cap(c, nc++, X(p), ay, X(p), ay + (major ? 12 : 6), hw, hw, major ? 0xFFF8EE : L.color2, a, v);
        }
        for (int j = 0; j < 5; j++) {
          snprintf(c.txt[2 + j], 40, "%d", 960 + j * 20);
          ww_text(c, n++, L.font2, c.mid2, c.txt[2 + j], X(960 + j * 20), ay + 25, 1, L.color2, a, 0, v, c.ob, 0.63f);
          ww_text(c, n++, L.font, c.mid, WW_WORDS[j], X(WW_AT[j]), ay - 34, 1, j == best && known ? 0xFFF8EE : L.color2, a, 2, v, c.ob);
        }
        const float x3 = X(P3), xn = X(c.needle.x), ka = known ? a : 0;
        ww_cap(c, nc++, x3 - 5.5f, ay - 15, x3 + 5.5f, ay - 15, 0.8f, 0.8f, L.color3, ka, v);
        ww_cap(c, nc++, x3 + 5.5f, ay - 15, x3, ay - 6, 0.8f, 0.8f, L.color3, ka, v);
        ww_cap(c, nc++, x3, ay - 6, x3 - 5.5f, ay - 15, 0.8f, 0.8f, L.color3, ka, v);
        c.over = n;
        ww_rrect(c, n++, xn, ay - 2, 4, 32, 2, L.color, a * (known ? 1 : 0.35f), v, false);
        ww_circle(c, n++, xn, ay - 18, 5.5f, L.color, a * (known ? 1 : 0.35f), v, false);
      }
      wx_deg(c.txt[0], 40, w.now);
      if (known) snprintf(c.txt[1], 40, "%d hPa", P);
      else snprintf(c.txt[1], 40, "-- hPa");
      const float w0 = L.font2 ? halo_text_width(*L.font2, c.txt[0]) : 0, w1 = L.font2 ? halo_text_width(*L.font2, c.txt[1]) : 0;
#ifdef HALO_WATCH  // (the watch's 7B: the degrees and hPa, then the trend, in two lines in the middle under the ruler)
      const float RY = 76, IX = 0, TX = 0, TY = 108;
#elif defined(HALO_SQUARE)  // (the square's 3B: the degrees, hPa and the trend lower under the ruler)
      const float RY = 72, IX = -44, TX = 64, TY = 72;
#else
      const float RY = 56, IX = -44, TX = 64, TY = 56;
#endif
      const float ix = ruler ? ax + IX : ax - 226, iy = ruler ? ay + RY : ay + 16, x0 = ix - (w0 + 8 + w1) / 2;
      ww_text(c, n++, L.font2, c.mid2, c.txt[0], x0, iy, 0, 0xFFF8EE, a, 0, v, c.ob);
      ww_text(c, n++, L.font2, c.mid2, c.txt[1], x0 + w0 + 8, iy, 0, L.color2, a, 0, v, c.ob);
      const int dp = P3 - P, tr = !known ? 0 : dp >= 2 ? 1 : dp <= -2 ? -1 : 0;
      static const char *const TREND[3] = {"FALLING", "STEADY", "RISING"};
      const uint32_t tc = tr > 0 ? 0xA8C7FA : tr < 0 ? 0xFF8A80 : L.color2;
      const float tcx = ruler ? ax + TX : ax - 226, ty = ruler ? ay + TY : ay + 46;
      const float tw = L.font ? halo_text_width(*L.font, TREND[tr + 1], 2) : 0, tx = tcx - (tw + 18) / 2, xa = tx + 5;
      if (tr) {  // (the arrow 12 px long, its head 5 px each side)
        const float d = tr < 0 ? 1 : -1;  // (down — falling)
        ww_cap(c, nc++, xa, ty - 6 * d, xa, ty + 6 * d, 1.1f, 1.1f, tc, a, v);
        ww_cap(c, nc++, xa - 5, ty + 1.7f * d, xa, ty + 6.7f * d, 1.1f, 1.1f, tc, a, v);
        ww_cap(c, nc++, xa + 5, ty + 1.7f * d, xa, ty + 6.7f * d, 1.1f, 1.1f, tc, a, v);
      } else {
        ww_cap(c, nc++, xa - 5, ty, xa + 5, ty, 1.1f, 1.1f, tc, a, v);
      }
      ww_text(c, n++, L.font, c.mid, TREND[tr + 1], tx + 18, ty, 0, tc, a, 2, v, c.ob);
#else
      const bool ruler = WW_RECT && look;  // (the 7-inch board's 3B: a straight ruler along the bottom)
      if (!ruler) {
        ww_arc(c, n++, ax, ay, 196, 1.5f, ww_rad(-125), ww_rad(125), ww_mix(L.color2, 0, 0.4f), a, v, c.ob);
        for (int j = 0; j < 5; j++) {
          const HaloMask &m = c.word[j];
          ww_mask(c, n++, m.bits ? &m : nullptr, ax + c.wx[j] + m.w * 0.5f, ay + c.wy[j] + m.h * 0.5f,
                  j == best && known ? 0xFFF8EE : L.color2, a, v);
        }
        for (int j = 0; j < 41; j++) {  // every 2 hPa, longer every 10
          const bool major = j % 5 == 0;
          const float sn = tick[j][0], cs = tick[j][1], r1 = major ? 180 : 188, hw = major ? 1.3f : 0.75f;
          ww_cap(c, nc++, ax + 196 * sn, ay - 196 * cs, ax + r1 * sn, ay - r1 * cs, hw, hw, major ? 0xFFF8EE : L.color2, a, v);
        }
        const float b3 = ww_baro(P3), bs = sinf(b3), bc = cosf(b3), an = ww_baro(c.needle.x), sn = sinf(an), cs = cosf(an);
        ww_cap(c, nc++, ax, ay, ax + 144 * bs, ay - 144 * bc, 1.5f, 1.5f, L.color3, known ? a : 0, v);  // the brass hand
        ww_ring(c, n++, ax + 150 * bs, ay - 150 * bc, 6, 2.5f, L.color3, known ? a : 0, v);
        ww_cap(c, nc++, ax - 29 * sn, ay + 29 * cs, ax + 171 * sn, ay - 171 * cs, 5, 1.2f, L.color, a * (known ? 1 : 0.35f), v);
        c.over = n;
        ww_circle(c, n++, ax, ay, 12, 0x1C1D20, a, v, false);
        ww_ring(c, n++, ax, ay, 10.5f, 3, L.color, a, v);
        ww_circle(c, n++, ax, ay, 3.5f, 0xFFF8EE, a, v, false);
      } else {  // 960 … 1040 hPa over 800 px; the words over it, the numbers under it; now — a coral bar, in 3 hours — brass
        auto X = [&](float p) { return ax - 400 + (fminf(1040, fmaxf(960, p)) - 960) * 10; };
        ww_cap(c, nc++, X(960), ay, X(1040), ay, 1, 1, ww_mix(L.color2, 0, 0.4f), a, v);
        for (int p = 960, j = 0; p <= 1040; p += 2, j++) {
          const bool major = j % 5 == 0;
          const float hw = major ? 1.3f : 0.75f;
          ww_cap(c, nc++, X(p), ay, X(p), ay + (major ? 22 : 11), hw, hw, major ? 0xFFF8EE : L.color2, a, v);
        }
        for (int j = 0; j < 5; j++) {
          snprintf(c.txt[2 + j], 40, "%d", 960 + j * 20);
          ww_text(c, n++, L.font2, c.mid2, c.txt[2 + j], X(960 + j * 20), ay + 46, 1, L.color2, a, 0, v, c.ob, 0.62f);
          ww_text(c, n++, L.font, c.mid, WW_WORDS[j], X(WW_AT[j]), ay - 50, 1, j == best && known ? 0xFFF8EE : L.color2, a, 3, v, c.ob);
        }
        const float x3 = X(P3), xn = X(c.needle.x), ka = known ? a : 0;
        ww_cap(c, nc++, x3 - 9, ay - 24, x3 + 9, ay - 24, 1.25f, 1.25f, L.color3, ka, v);
        ww_cap(c, nc++, x3 + 9, ay - 24, x3, ay - 8, 1.25f, 1.25f, L.color3, ka, v);
        ww_cap(c, nc++, x3, ay - 8, x3 - 9, ay - 24, 1.25f, 1.25f, L.color3, ka, v);
        c.over = n;
        ww_rrect(c, n++, xn, ay - 2, 6, 56, 3, L.color, a * (known ? 1 : 0.35f), v, false);
        ww_circle(c, n++, xn, ay - 30, 9, L.color, a * (known ? 1 : 0.35f), v, false);
      }
      // the degrees and the pressure under the hub; where it goes in 3 hours
      wx_deg(c.txt[0], 40, w.now);
      if (known) snprintf(c.txt[1], 40, "%d hPa", P);
      else snprintf(c.txt[1], 40, "-- hPa");
      const float w0 = L.font2 ? halo_text_width(*L.font2, c.txt[0]) : 0, w1 = L.font2 ? halo_text_width(*L.font2, c.txt[1]) : 0;
      // (the watch: under the hub; the 7-inch board: under the clock (3A), along the bottom beside the trend (3B))
      const float ix = !WW_RECT ? ax : ruler ? ax - 72 : ax - 500, iy = !WW_RECT ? ay + 131 : ruler ? ay + 92 : ay + 38;
      const float x0 = ix - (w0 + 8 + w1) / 2;
      ww_text(c, n++, L.font2, c.mid2, c.txt[0], x0, iy, 0, 0xFFF8EE, a, 0, v, c.ob);
      ww_text(c, n++, L.font2, c.mid2, c.txt[1], x0 + w0 + 8, iy, 0, L.color2, a, 0, v, c.ob);
      const int dp = P3 - P, tr = !known ? 0 : dp >= 2 ? 1 : dp <= -2 ? -1 : 0;
      static const char *const TREND[3] = {"FALLING", "STEADY", "RISING"};
      const uint32_t tc = tr > 0 ? 0xA8C7FA : tr < 0 ? 0xFF8A80 : L.color2;
      const float tcx = !WW_RECT ? ax : ruler ? ax + 98 : ax - 500, ty = !WW_RECT ? ay + 165 : ruler ? ay + 92 : ay + 88;
      const float tw = L.font ? halo_text_width(*L.font, TREND[tr + 1], 2) : 0, tx = tcx - (tw + 18) / 2;
      if (tr) {
        const float d = tr < 0 ? 1 : -1;  // (down — falling)
        ww_cap(c, nc++, tx + 4, ty - 7 * d, tx + 4, ty + 7 * d, 1.3f, 1.3f, tc, a, v);
        ww_cap(c, nc++, tx - 2, ty + d, tx + 4, ty + 7 * d, 1.3f, 1.3f, tc, a, v);
        ww_cap(c, nc++, tx + 10, ty + d, tx + 4, ty + 7 * d, 1.3f, 1.3f, tc, a, v);
      } else {
        ww_cap(c, nc++, tx - 2, ty, tx + 10, ty, 1.3f, 1.3f, tc, a, v);
      }
      ww_text(c, n++, L.font, c.mid, TREND[tr + 1], tx + 18, ty, 0, tc, a, 2, v, c.ob);
#endif
#endif
      break;
    }
#endif
#if WW_ON(WW_WEAR)
    case FL_WXWEAR: {
      const int g = ww_garment(w);
      // (the 7-inch board: bigger beside the clock (4A), smaller in a row under it (4B) — studio.py makes its icons so)
      const float k = (L.k > 0 ? L.k : 1) * (WW_RECT ? (look ? 0.95f : 1.4f) : 1);
      const float ix = ax, iy = WW_RECT ? ay : ay - 28;  // the icons' own point (the canvas: 12 px under its middle — studio.py)
      const float bob = fc_calm ? 0 : roundf(sinf(ww_wrap(t * 1.6f, WW_TAU)) * 2.5f * k);
#ifdef HALO_P4_TALL  // (as on the WT32: the picture by its ink's middle — the garment's point where the mockup has it)
      static const float MID[5] = {3, -28.5f, -13, -3.5f, -26.5f};
      ww_image(c, n++, L.icons ? L.icons[g] : nullptr, ix, iy + MID[g] * k + bob, a, v);
#else
#ifdef HALO_COMPACT  // (a picture is cut to its ink: its middle from the garment's point, as gen_icons draws each — the
                     // point where the mockup has it)
      static const float MID[5] = {3, -28.5f, -13, -3.5f, -26.5f};
      ww_image(c, n++, L.icons ? L.icons[g] : nullptr, ix, iy + MID[g] * k + bob, a, v);
#else
      ww_image(c, n++, L.icons ? L.icons[g] : nullptr, ix, iy - 12 * k + bob, a, v);
#endif
#endif
      if (g == WG_UMBRELLA && !fc_calm) {  // the drops stop on the canopy, beside it they fall on
        for (int j = 0; j < 14; j++) {
          const float dx = (ww_rnd(j + 21) - 0.5f) * 200 * k, e = dx / (64 * k);
          const float top = fabsf(e) < 1 ? iy + bob - halo_sqrt(1 - e * e) * 56 * k : iy + 66 * k, from = iy - 96 * k;
          const float fy = from + ww_wrap(t * (170 + ww_rnd(j) * 60) * k + ww_rnd(j + 2) * 200, top - from);
          const float op = halo_clamp01(fminf((top - fy) / (30 * k), (fy - from) / (20 * k)));
          ww_cap(c, nc++, ix + dx, fy, ix + dx - 2 * k, fy + 9 * k, 1.2f * k, 1.2f * k, 0x8AB8FF, a * op, v);
        }
      }
      if (g == WG_HAT && !fc_calm) {
        for (int j = 0; j < 18; j++) {
          const float r = (1.6f + ww_rnd(j + 5) * 2.2f) * k, fy = ww_wrap(t * (22 + ww_rnd(j + 7) * 26) + ww_rnd(j + 1) * 150, 160);
          const float fx = ww_rnd(j) * 220 + sinf(ww_wrap(t * 1.3f + j, WW_TAU)) * 6;
          ww_circle(c, n++, ix + (fx - 110) * k, iy + (fy - 100) * k, r, 0xFFFFFF, a * (0.55f + 0.45f * ww_rnd(j + 11)), v, false);
        }
      }
      if (g == WG_GLASSES && !fc_calm) {  // a glint twinkles on a lens
        const float sp = fmaxf(0, sinf(ww_wrap(t * 2.4f, WW_TAU))), x = ix + 50 * k, y = iy - 16 * k + bob, l = 8 * k * sp;
        ww_cap(c, nc++, x - l, y, x + l, y, 1.2f * k, 1.2f * k, 0xFFFFFF, a * sp, v);
        ww_cap(c, nc++, x, y - l, x, y + l, 1.2f * k, 1.2f * k, 0xFFFFFF, a * sp, v);
      }
      char fe[8];
      wx_deg(c.txt[0], 40, w.now), wx_deg(fe, sizeof(fe), w.feels);
      snprintf(c.txt[1], 40, "%s · FEELS %s", c.txt[0], fe);
#ifdef HALO_P4_TALL  // (tall: 12.4B — the words beside the picture, 12.4A — under it, the picture 1.9 times its size; the
                     // advice smaller when it is longer than the room to the edge: TAKE AN UMBRELLA beside the picture)
      const float sw = L.font ? halo_text_width(*L.font, WW_SAY[g], L.spacing) : 0, sx = look ? ix + 102 * k : ix;
      const float ss = sw > 0 ? fminf(1, (look ? HALO_W - 24 - sx : HALO_W - 48) / sw) : 1;
      if (look) {
        ww_text(c, n++, L.font, c.mid, WW_SAY[g], sx, iy - 17 * k, 0, WW_SAY_C[g], a, L.spacing, v, c.ob, ss);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], sx, iy + 23.2f * k, 0, L.color, a, 0, v, c.ob);
      } else {
        ww_text(c, n++, L.font, c.mid, WW_SAY[g], ix, iy + 113.7f * k, 1, WW_SAY_C[g], a, L.spacing, v, c.ob, ss);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ix, iy + 142 * k, 1, L.color, a, 0, v, c.ob);
      }
#else
#ifdef HALO_SQUARE  // (the square's 4A: the garment big under the clock, the words under it)
      ww_text(c, n++, L.font, c.mid, WW_SAY[g], ix, iy + 122.5f * k, 1, WW_SAY_C[g], a, L.spacing, v, c.ob);
      ww_text(c, n++, L.font2, c.mid2, c.txt[1], ix, iy + 160 * k, 1, L.color, a, 0, v, c.ob);
#elif defined(HALO_COMPACT)  // (the WT32 wide: 4B — the words beside the picture, 4A — under it; wt32-widgets.html)
      if (look) {
        ww_text(c, n++, L.font, c.mid, WW_SAY[g], ix + 121 * k, iy - 24 * k, 0, WW_SAY_C[g], a, L.spacing, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ix + 121 * k, iy + 38 * k, 0, L.color, a, 0, v, c.ob);
      } else {
        ww_text(c, n++, L.font, c.mid, WW_SAY[g], ix, iy + 116 * k, 1, WW_SAY_C[g], a, L.spacing, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ix, iy + 152 * k, 1, L.color, a, 0, v, c.ob);
      }
#else
      if (WW_RECT && look) {  // (4B: the words beside the picture)
        ww_text(c, n++, L.font, c.mid, WW_SAY[g], ix + 118 * k, iy - 21 * k, 0, WW_SAY_C[g], a, L.spacing, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ix + 118 * k, iy + 38 * k, 0, L.color, a, 0, v, c.ob);
      } else {
        ww_text(c, n++, L.font, c.mid, WW_SAY[g], ix, WW_RECT ? iy + 133 * k : ay + 70, 1, WW_SAY_C[g], a, L.spacing, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[1], ix, WW_RECT ? iy + 166 * k : ay + 104, 1, L.color, a, 0, v, c.ob);
      }
#endif
#endif
      break;
    }
#endif
#if WW_ON(WW_BUDDY)
    case FL_WXBUDDY: {
#ifdef HALO_P4_TALL
      const float k = (L.k > 0 ? L.k : 1) * (look ? 3.0f : 2.1f);  // (tall: under the clock — 12.5A, peeking — 12.5B)
#else
#ifdef HALO_WATCH
      const float k = L.k > 0 ? L.k : 1;  // (the watch: under the clock — 9A)
#elif defined(HALO_SQUARE)
      const float k = (L.k > 0 ? L.k : 1) * 1.1f;  // (the square: under the clock — 5A)
#elif defined(HALO_COMPACT)
      const float k = (L.k > 0 ? L.k : 1) * (look ? 1.0f : 0.84f);  // (the WT32 wide: beside — 5A, peeking — 5B)
#else
      const float k = (L.k > 0 ? L.k : 1) * (WW_RECT ? (look ? 2.0f : 1.45f) : 1);  // (7-inch: beside — 5A, peeking — 5B)
#endif
#endif
      if (w.sky != c.sky) {
        if (c.sky >= 0) c.pop.x = 0.6f, c.pop.v = 0;  // (a new sky: pops up)
        c.sky = w.sky;
      }
      c.pop.target = 1;
      halo_spring_run(c.pop, c.pk, c.dt);
      // (its motion 30 times a second, not 60: the rays' ring is ~7 % of the screen — 2.7 ms a frame on the board)
      const float t = (float)((c.ms % 600000u) / 33) * 0.033f;
      const float bob = fc_calm ? 0 : roundf(sinf(ww_wrap(t * 2.2f, WW_TAU)) * 4 * k);
#ifdef HALO_P4_TALL
      const WwB b = {ax, ay + bob, k};  // (tall: the anchor is the buddy's middle; its bubble is its own size, apart)
#else
      const WwB b = {ax - 28 * k, ay + 18 * k + bob, k};
#endif
      const float ps = c.pop.x;
      HaloView vb = v;  // the buddy's own view: grown by ps round its middle
      vb.s = v.s * ps, vb.x = v.x + (b.x - CX) * (1 - ps) * v.s, vb.y = v.y + (b.y - CY) * (1 - ps) * v.s;
      if (w.sky == WX_SUN) {
        const float rot = ww_wrap(t * 0.3f, WW_TAU);
        for (int j = 0; j < 12; j++) {
          const float an = rot + j * (WW_TAU / 12), cs = cosf(an), sn = sinf(an);
          ww_cap(c, nc++, b.X(cs * 54), b.Y(sn * 54), b.X(cs * 68), b.Y(sn * 68), 4 * k, 4 * k, 0xFFC94D, a, vb);
        }
        ww_circle(c, n++, b.x, b.y, 46 * k, 0xFFC94D, a, vb, false);
        ww_face(c, n, nc, b, 2, 0, t, a, vb);
      } else if (w.sky == WX_PARTLY) {
        const float rot = ww_wrap(t * 0.25f, WW_TAU);
        ww_circle(c, n++, b.X(40), b.Y(-34), 24 * k, 0xFFC94D, a, vb, false);
        for (int j = 0; j < 8; j++) {
          const float an = rot + j * (WW_TAU / 8), cs = cosf(an), sn = sinf(an);
          ww_cap(c, nc++, b.X(40 + cs * 34.8f), b.Y(-34 + sn * 34.8f), b.X(40 + cs * 44.4f), b.Y(-34 + sn * 44.4f), 3.8f * k, 3.8f * k, 0xFFC94D, a, vb);
        }
        c.over = n;
        ww_cloud_body(c, n, b, 0xF4F5F7, a, vb);
        ww_face(c, n, nc, b, 10, 1, t, a, vb);
      } else if (w.sky == WX_CLOUD) {
        ww_cloud_body(c, n, b, 0xBFC4CC, a, vb);
        ww_face(c, n, nc, b, 10, 2, t, a, vb);
        for (int j = 0; j < 3 && !fc_calm; j++) {  // z z z
          const float ph = ww_wrap(t / 2.4f + j / 3.0f, 1);
          ww_text(c, n++, L.font2, c.mid2, "z", b.X(-56 - ph * 24 - j * 4), b.Y(-50 - ph * 50), 1, 0x9E9E9E,
                  a * sinf(ph * HALO_PI), 0, vb, false, (0.7f + ph * 0.5f) * k);
        }
      } else if (w.sky == WX_RAIN) {
        for (int j = 0; j < 7 && !fc_calm; j++) {
          const float ph = ww_wrap(t * 1.4f + ww_rnd(j), 1);
          ww_circle(c, n++, b.X(-54 + j * 18), b.Y(46 + ph * 56), 3 * k, 0x8AB8FF, a * (1 - ph), vb, false);
        }
        ww_cloud_body(c, n, b, 0x8B94A3, a, vb);
        ww_face(c, n, nc, b, 10, 3, t, a, vb);
      } else {
        for (int j = 0; j < 16 && !fc_calm; j++) {
          const float r = (1.6f + ww_rnd(j + 5) * 2.2f), fy = ww_wrap(t * (22 + ww_rnd(j + 7) * 26) + ww_rnd(j + 1) * 150, 160);
          const float fx = ww_rnd(j) * 196 + sinf(ww_wrap(t * 1.3f + j, WW_TAU)) * 6;
          ww_circle(c, n++, b.X(fx - 104), b.Y(fy - 70), r * k, 0xFFFFFF, a * (0.55f + 0.45f * ww_rnd(j + 11)), vb, false);
        }
        ww_cloud_body(c, n, b, 0xEEF3FA, a, vb);
        ww_face(c, n, nc, b, 10, 1, t, a, vb);
        ww_rrect(c, n++, b.X(1), b.Y(-62), 70 * k, 52 * k, 26 * k, 0xFF6B6B, a, vb, false);  // the woolly hat
        ww_rrect(c, n++, b.X(0), b.Y(-38), 80 * k, 16 * k, 8 * k, 0xFFF8EE, a, vb, false);
        ww_circle(c, n++, b.X(6), b.Y(-92), 10 * k, 0xFFF8EE, a, vb, false);
      }
      // the speech bubble with the degrees
#ifdef HALO_P4_TALL  // (tall: up and to the right of the buddy, a size of its own — 12.5A 1.25, 12.5B 1.3)
      const float qk = (L.k > 0 ? L.k : 1) * (look ? 1.3f : 1.25f);
      const float qx = ax + 182, qy = ay - (look ? 324 : 230) - (fc_calm ? 0 : roundf(sinf(ww_wrap(t * 2.2f, WW_TAU)) * 2 * qk));
      if (c.over == WW_ITEMS) c.over = n;
      ww_rrect(c, n++, qx, qy, 94 * qk, 58 * qk, 29 * qk, L.color, a, v, c.ob);
      ww_circle(c, n++, qx - 40 * qk, qy + 36 * qk, 6 * qk, L.color, a, v, c.ob);
      ww_circle(c, n++, qx - 52 * qk, qy + 47 * qk, 3.5f * qk, L.color, a, v, c.ob);
#else
      const float qx = ax + 106 * k, qy = ay - 53 * k - roundf(bob * 0.5f);
      if (c.over == WW_ITEMS) c.over = n;
      ww_rrect(c, n++, qx, qy, 94 * k, 58 * k, 29 * k, L.color, a, v, c.ob);
      ww_circle(c, n++, qx - 40 * k, qy + 36 * k, 6 * k, L.color, a, v, c.ob);
      ww_circle(c, n++, qx - 52 * k, qy + 47 * k, 3.5f * k, L.color, a, v, c.ob);
#endif
      wx_deg(c.txt[0], 40, w.now);
      ww_text(c, n++, L.font, c.mid, c.txt[0], qx, qy, 1, L.color2, a, L.spacing, v, false);
      break;
    }
#endif
#if WW_ON(WW_PIXEL)
    case FL_WXPIXEL: {
      if (!c.cells[0] || !c.cells[1] || !c.cells[2]) break;
      static const uint32_t ACC[5] = {0xFFDB85, 0xFFC94D, 0xFFF8EE, 0x8AB8FF, 0xEAF2FF};  // the degrees' color by the sky
      if (c.pal_t[1] != L.color3 || c.pal_d[1] != ACC[w.sky])  // (a new color: the same cells — drawn again)
        for (int j = 0; j < 3; j++)
          if (c.it[j].shown) halo_damage_f(c.it[j].b[0], c.it[j].b[1], c.it[j].b[2], c.it[j].b[3]);
      c.pal_t[1] = L.color3, c.pal_d[1] = ACC[w.sky];
      const uint32_t fr = c.ms / 250;  // four frames a second
      const uint32_t sig = ww_hashu(ww_hashu(2166136261u, fr), w.sky);
      wx_deg(c.txt[0], 40, w.now);
      uint32_t ts = 2166136261u;
      for (const char *p = c.txt[0]; *p; p++) ts = ww_hashu(ts, (uint8_t)*p);
      if (ts != c.px_sig[2]) c.px_sig[2] = ts, ww_px_text(c, 2, c.txt[0]);
#ifdef HALO_P4_TALL
      // The 7″ board held tall (rect-tall.html 12.6): A — 38 x 40 cells of 16 px from the anchor down, the pixel time
      // (p2; under the date line) and the degrees over it; B — 50 x 46 cells of 12 px, the degrees under the clock
      const bool whole = !look;
      const int cols = whole ? 38 : 50, rows = whole ? 40 : 46;
      if (c.pxw[0] != cols || c.pxh[0] != rows) {
        HaloPixels &q = c.px[0];
        c.pxw[0] = q.w = q.bx1 = cols, c.pxh[0] = q.h = q.by1 = rows, q.ax = cols / 2.0f;
        c.px_sig[0] = ~sig;
      }
      if (sig != c.px_sig[0]) c.px_sig[0] = sig, ww_px_wide(c, w.sky, fr, cols, rows);
      ww_pixels(c, n++, &c.px[0], c.pframe[0], ax, ay, (whole ? 16 : 12) * k, a, v);
      if (whole && L.p2 >= 0.5f) {
        uint32_t cs = 2166136261u;
        for (const char *p = fc_time; *p; p++) cs = ww_hashu(cs, (uint8_t)*p);
        if (cs != c.px_sig[1]) c.px_sig[1] = cs, ww_px_text(c, 1, fc_time);
        ww_pixels(c, n++, &c.px[1], c.pframe[1], ax, ay - 304 * k, 14 * k, a, v);
      }
      ww_pixels(c, n++, &c.px[2], c.pframe[2], ax, ay - (whole ? 174 : 154) * k, (whole ? 12 : 9) * k, a, v);
      ww_text(c, n++, L.font, c.mid, WW_WORD[w.sky], ax, ay - (whole ? 62 : 66) * k, 1, L.color, a, L.spacing, v, c.ob);
#else
#ifdef HALO_COMPACT
      // The WT32 wide (wt32-widgets.html): 6A — 54 x 20 cells of 9 px from the anchor down, the pixel time (p2) and the
      // degrees over it; 6B — 60 x 19 cells of 8 px along the bottom, the degrees beside the clock
      const bool whole = !look;
#ifdef HALO_WATCH  // (the watch's 10A: 46 x 28 cells of 9 px, from under the degrees to the bottom; the time and the
                   // degrees over it in the middle, one under the other)
      const int cols = 46, rows = 28;
      const float TY = 176, WY = 28, TX = 0, DX = 0, DY = 88, PT = 10, PD = 6;
#elif defined(HALO_SQUARE)  // (the square's 6A: 54 x 30 cells of 9 px, from under the time to the bottom)
      const int cols = 54, rows = 30;
      const float TY = 112, WY = 45, TX = -116, DX = 132, DY = TY, PT = 8, PD = 7;
#else
      const int cols = whole ? 54 : 60, rows = whole ? 20 : 19;
      const float TY = whole ? 82 : 98, WY = whole ? 15 : 40, TX = -116, DX = whole ? 132 : 160, DY = TY, PT = 8, PD = whole ? 7 : 6;
#endif
      if (c.pxw[0] != cols || c.pxh[0] != rows) {
        HaloPixels &q = c.px[0];
        c.pxw[0] = q.w = q.bx1 = cols, c.pxh[0] = q.h = q.by1 = rows, q.ax = cols / 2.0f;
        c.px_sig[0] = ~sig;
      }
      if (sig != c.px_sig[0]) c.px_sig[0] = sig, ww_px_wide(c, w.sky, fr, cols, rows);
      ww_pixels(c, n++, &c.px[0], c.pframe[0], ax, ay, (whole ? 9 : 8) * k, a, v);
      if (whole && L.p2 >= 0.5f) {
        uint32_t cs = 2166136261u;
        for (const char *p = fc_time; *p; p++) cs = ww_hashu(cs, (uint8_t)*p);
        if (cs != c.px_sig[1]) c.px_sig[1] = cs, ww_px_text(c, 1, fc_time);
        ww_pixels(c, n++, &c.px[1], c.pframe[1], ax + TX * k, ay - TY * k, PT * k, a, v);  // (under the date line)
      }
      const float tx = ax + DX * k;
      ww_pixels(c, n++, &c.px[2], c.pframe[2], tx, ay - DY * k, PD * k, a, v);
      ww_text(c, n++, L.font, c.mid, WW_WORD[w.sky], tx, ay - WY * k, 1, L.color, a, L.spacing, v, c.ob);
#elif WW_RECT
      // 6A: the whole screen — 64 x 22 cells of 16 px, the pixel time (p2) top left, the degrees top right; 6B: 86 x 23
      // cells of 12 px along the bottom, the degrees beside the clock (the passport moves it to the left)
      const bool whole = !look;
      const int cols = whole ? 64 : 86, rows = whole ? 22 : 23;
      if (c.pxw[0] != cols || c.pxh[0] != rows) {
        HaloPixels &q = c.px[0];
        c.pxw[0] = q.w = q.bx1 = cols, c.pxh[0] = q.h = q.by1 = rows, q.ax = cols / 2.0f;
        c.px_sig[0] = ~sig;
      }
      if (sig != c.px_sig[0]) c.px_sig[0] = sig, ww_px_wide(c, w.sky, fr, cols, rows);
      ww_pixels(c, n++, &c.px[0], c.pframe[0], ax, ay - (whole ? 52 : 136) * k, (whole ? 16 : 12) * k, a, v);
      if (whole && L.p2 >= 0.5f) {
        uint32_t cs = 2166136261u;
        for (const char *p = fc_time; *p; p++) cs = ww_hashu(cs, (uint8_t)*p);
        if (cs != c.px_sig[1]) c.px_sig[1] = cs, ww_px_text(c, 1, fc_time);
        ww_pixels(c, n++, &c.px[1], c.pframe[1], ax - 250 * k, ay - 204 * k, 12 * k, a, v);  // (under the date line)
      }
      const float tx = ax + (whole ? 318 : 328) * k;
      ww_pixels(c, n++, &c.px[2], c.pframe[2], tx, ay - (whole ? 196 : 332) * k, (whole ? 10 : 9) * k, a, v);
      ww_text(c, n++, L.font, c.mid, WW_WORD[w.sky], tx, ay - (whole ? 102 : 240) * k, 1, L.color, a, L.spacing, v, c.ob);
#else
      if (sig != c.px_sig[0]) c.px_sig[0] = sig, ww_px_scene(c, w.sky, fr);
      ww_pixels(c, n++, &c.px[0], c.pframe[0], ax, ay - 82 * k, 9 * k, a, v);
      if (L.p2 >= 0.5f) {
        uint32_t cs = 2166136261u;
        for (const char *p = fc_time; *p; p++) cs = ww_hashu(cs, (uint8_t)*p);
        if (cs != c.px_sig[1]) c.px_sig[1] = cs, ww_px_text(c, 1, fc_time);
        ww_pixels(c, n++, &c.px[1], c.pframe[1], ax, ay - 178 * k, 8 * k, a, v);
      }
      ww_pixels(c, n++, &c.px[2], c.pframe[2], ax, ay + 86 * k, 6 * k, a, v);
      ww_text(c, n++, L.font, c.mid, WW_WORD[w.sky], ax, ay + 150 * k, 1, L.color, a, 3, v, c.ob);
#endif
#endif
      break;
    }
#endif
#if WW_ON(WW_WEEK)
    case FL_WXWEEK: {
      struct tm tm;
      const bool ok = wg_now(tm);
      // Its geometry from the anchor: the columns' step, the days' row, the icons', the bars' bottom and height, the
      // numbers' gap from a bar, a bar's half width — the watch's five; the 7-inch board's seven (7A) and five (7B)
#ifdef HALO_P4_TALL  // (the 7″ board held tall: seven columns — 12.7B; seven rows — 12.7A, drawn apart: its rows' step,
                     // their first's y, the bars' half thickness)
      static const float GEO[3][7] = {{64, -90, -56, 72, 76, 19, 6}, {80, -280, -226, 80, 200, 26.4f, 8.25f},
                                      {80, -240, 0, 0, 0, 0, 6.5f}};
#else
#ifdef HALO_WATCH  // (the watch's 11B: five rows under the clock, 52 px apart from the anchor — watch206-widgets.html)
      static const float GEO[3][7] = {{64, -90, -56, 72, 76, 19, 6}, {54, -78, -48, 84, 84, 18, 6}, {52, 0, 0, 0, 0, 0, 4.5f}};
#elif defined(HALO_SQUARE)  // (the square's 7A: seven tall columns under the clock — square-widgets.html)
      static const float GEO[3][7] = {{64, -90, -56, 72, 76, 19, 6}, {64, -78, -48, 84, 84, 18, 6}, {64, -78, -48, 84, 84, 18, 6}};
#elif defined(HALO_COMPACT)  // (the WT32 wide's 7A and 7B: wt32-widgets.html)
      static const float GEO[3][7] = {{64, -90, -56, 72, 76, 19, 6}, {64, -50, -24, 61, 48, 17, 5.5f},
                                      {44, -74, -48, 80, 84, 16, 5}};
#else
      static const float GEO[3][7] = {{64, -90, -56, 72, 76, 19, 6}, {140, -155, -111, 45, 90, 31, 10},
                                      {94, -189, -143, 71, 142, 29, 9}};
#endif
#endif
      const float *G = GEO[WW_RECT ? (look ? 2 : 1) : 0];
#ifdef HALO_P4_TALL
      const int nd = 7;
#else
      const int nd = WW_RECT && !look ? 7 : 5;
#endif
      float hi[7], lo[7], mn = 1e9f, mx = -1e9f;
      int sk[7];
      for (int j = 0; j < nd; j++) {
        const bool has = w.real && w.c->dhi[j] != WX_NONE && w.c->dlo[j] != WX_NONE;
        hi[j] = has ? w.c->dhi[j] : (w.hi == WX_NONE ? 0 : w.hi), lo[j] = has ? w.c->dlo[j] : (w.lo == WX_NONE ? 0 : w.lo);
        sk[j] = !has ? w.sky : (w.c->dsnow >> j & 1) && (w.c->dk[j] & 3) == WX_CLOUD ? WX_SNOW : w.c->dk[j] & 3;
        mn = fminf(mn, lo[j]), mx = fmaxf(mx, hi[j]);
      }
      const float yk = G[4] / fmaxf(1, mx - mn);
      float bb[4] = {1e9f, 1e9f, -1e9f, -1e9f};
      uint32_t h = 2166136261u;
      c.over = 0;  // (the bars only — no dark track under them: the user's word, 15 Nov 2026)
#if defined(HALO_P4_TALL) || defined(HALO_WATCH)
      // (rows: the bars' width and their left end, the day's x, the sky's, where the low ends, where the high starts)
#ifdef HALO_WATCH
      static const float ROW[6] = {132, -21, -175, -93, -33, 123};
#else
      static const float ROW[6] = {194, -42, -252, -140, -60, 170};
#endif
      c.hbar = look;
      if (look) {  // (12.7A: a row a day — the day, the sky, the low, a bar from the low to the high on a dark track, the high)
        const float xk = ROW[0] / fmaxf(1, mx - mn);  // (the bars between 42 px left of the anchor and 152 right)
        for (int j = 0; j < nd; j++)
          ww_rrect(c, n++, ax + ROW[1] + ROW[0] / 2, ay + G[1] + j * G[0], ROW[0] + 2 * G[6], 2 * G[6], G[6], 0x232428, a, v, c.ob);
        c.over = n;  // (the tracks under the bars)
        for (int j = 0; j < nd; j++) {
          const float y = ay + G[1] + j * G[0], xl = ax + ROW[1] + (lo[j] - mn) * xk, xr = ax + ROW[1] + (hi[j] - mn) * xk;
          snprintf(c.txt[j], 40, "%s", ok ? WEEKDAY[(tm.tm_wday + j) % 7] : "---");
          ww_text(c, n++, L.font2, c.mid2, c.txt[j], ax + ROW[2], y, 0, j ? L.color2 : L.color3, a, 2, v, c.ob);
          ww_image(c, n++, L.icons ? L.icons[sk[j]] : nullptr, ax + ROW[3], y, a, v);
          c.bar[j][0] = halo_view_y(v, y), c.bar[j][1] = halo_view_x(v, xl), c.bar[j][2] = halo_view_x(v, xr);
          c.bcol[j][0] = ww_tcol(lo[j]), c.bcol[j][1] = ww_tcol(hi[j]);
          h = ww_hash(ww_hash(ww_hash(h, c.bar[j][0]), c.bar[j][1]), c.bar[j][2]), h = ww_hashu(ww_hashu(h, c.bcol[j][0]), c.bcol[j][1]);
          const float e = (G[6] + 2) * v.s;
          ww_box_add(bb, c.bar[j][1] - e, c.bar[j][0] - e, c.bar[j][2] + e, c.bar[j][0] + e);
          wx_deg(c.txt[7 + j], 40, (int16_t)lroundf(hi[j])), wx_deg(c.txt[14 + j], 40, (int16_t)lroundf(lo[j]));
          ww_text(c, n++, L.font, c.mid, c.txt[14 + j], ax + ROW[4], y, 2, L.color2, a, 0, v, c.ob);
          ww_text(c, n++, L.font, c.mid, c.txt[7 + j], ax + ROW[5], y, 0, j ? L.color : L.color3, a, 0, v, c.ob);
        }
      } else
#endif
      for (int j = 0; j < nd; j++) {
        const float x = ax + (j - (nd - 1) * 0.5f) * G[0], yh = ay + G[3] - (hi[j] - mn) * yk, yl = ay + G[3] - (lo[j] - mn) * yk;
        snprintf(c.txt[j], 40, "%s", ok ? WEEKDAY[(tm.tm_wday + j) % 7] : "---");
        ww_text(c, n++, L.font2, c.mid2, c.txt[j], x, ay + G[1], 1, j ? L.color2 : L.color3, a, 2, v, c.ob);
        ww_image(c, n++, L.icons ? L.icons[sk[j]] : nullptr, x, ay + G[2], a, v);
        c.bar[j][0] = halo_view_x(v, x), c.bar[j][1] = halo_view_y(v, yh), c.bar[j][2] = halo_view_y(v, yl);
        c.bcol[j][0] = ww_tcol(hi[j]), c.bcol[j][1] = ww_tcol(lo[j]);
        h = ww_hash(ww_hash(ww_hash(h, c.bar[j][0]), c.bar[j][1]), c.bar[j][2]), h = ww_hashu(ww_hashu(h, c.bcol[j][0]), c.bcol[j][1]);
        const float e = (G[6] + 2) * v.s;
        ww_box_add(bb, c.bar[j][0] - e, c.bar[j][1] - e, c.bar[j][0] + e, c.bar[j][2] + e);
        wx_deg(c.txt[7 + j], 40, (int16_t)lroundf(hi[j])), wx_deg(c.txt[14 + j], 40, (int16_t)lroundf(lo[j]));
        ww_text(c, n++, L.font, c.mid, c.txt[7 + j], x, yh - G[5], 1, j ? L.color : L.color3, a, 0, v, c.ob);
        ww_text(c, n++, L.font, c.mid, c.txt[14 + j], x, yl + G[5], 1, L.color2, a, 0, v, c.ob);
      }
      c.bw = G[6] * v.s, c.nbar = nd;
      ww_area(c.area[0], true, bb[0], bb[1], bb[2], bb[3], ww_hash(h, c.a * 64)), area = true;
#ifdef HALO_P4_TALL
      if (L.font && L.font2) {  // (tall, both: the weather now under the clock — "NOW 18° · PARTLY CLOUDY", at y 322)
        wx_deg(c.txt[21], 40, w.now);
        snprintf(c.txt[22], 40, "· %s", WW_WORD[w.sky]);
        const float w0 = halo_text_width(*L.font2, "NOW", 3), w1 = halo_text_width(*L.font, c.txt[21]);
        const float w2 = halo_text_width(*L.font2, c.txt[22], 3), x0 = ax - (w0 + w1 + w2 + 20) / 2, y0 = ay + (look ? -338 : -378);
        ww_text(c, n++, L.font2, c.mid2, "NOW", x0, y0, 0, L.color2, a, 3, v, c.ob);
        ww_text(c, n++, L.font, c.mid, c.txt[21], x0 + w0 + 10, y0, 0, L.color, a, 0, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[22], x0 + w0 + w1 + 20, y0, 0, L.color2, a, 3, v, c.ob);
      }
#else
#ifdef HALO_WATCH
      // (the watch's 11B: the rows alone — the weather now is in the status line)
#elif defined(HALO_COMPACT)
      if (look && L.font && L.font2) {  // (the WT32 wide's 7B: the weather now under the clock)
        wx_deg(c.txt[21], 40, w.now);
        snprintf(c.txt[22], 40, "· %s", WW_WORD[w.sky]);
        const float w0 = halo_text_width(*L.font2, "NOW", 1), w1 = halo_text_width(*L.font, c.txt[21]);
        const float w2 = halo_text_width(*L.font2, c.txt[22], 1), x0 = ax - 240 - (w0 + w1 + w2 + 10) / 2, y0 = ay + 56;
        ww_text(c, n++, L.font2, c.mid2, "NOW", x0, y0, 0, L.color2, a, 1, v, c.ob);
        ww_text(c, n++, L.font, c.mid, c.txt[21], x0 + w0 + 5, y0, 0, L.color, a, 0, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[22], x0 + w0 + w1 + 10, y0, 0, L.color2, a, 1, v, c.ob);
      }
#else
      if (WW_RECT && look && L.font && L.font2) {  // (7B: the weather now under the clock — "NOW 18° · PARTLY CLOUDY")
        wx_deg(c.txt[21], 40, w.now);
        snprintf(c.txt[22], 40, "· %s", WW_WORD[w.sky]);
        const float w0 = halo_text_width(*L.font2, "NOW", 2), w1 = halo_text_width(*L.font, c.txt[21]);
        const float w2 = halo_text_width(*L.font2, c.txt[22], 2), x0 = ax - 516 - (w0 + w1 + w2 + 20) / 2, y0 = ay + 53;
        ww_text(c, n++, L.font2, c.mid2, "NOW", x0, y0, 0, L.color2, a, 2, v, c.ob);
        ww_text(c, n++, L.font, c.mid, c.txt[21], x0 + w0 + 10, y0, 0, L.color, a, 0, v, c.ob);
        ww_text(c, n++, L.font2, c.mid2, c.txt[22], x0 + w0 + w1 + 20, y0, 0, L.color2, a, 2, v, c.ob);
      }
#endif
#endif
      break;
    }
#endif
    default:
      break;
  }
  if (!area) ww_area(c.area[0], false, 0, 0, -1, -1, 0);
  ww_hide(c, n, v), ww_hide_caps(c, nc);
  for (int j = 0; j < n; j++) box_add(R.box, c.it[j]);
  for (int j = 0; j < nc; j++) {
    const WwCap &q = c.cap[j];
    if (q.o[4] >= 0) ww_box_add(R.box, fminf(q.x0, q.x1) - q.o[4], fminf(q.y0, q.y1) - q.o[4], fmaxf(q.x0, q.x1) + q.o[4], fmaxf(q.y0, q.y1) + q.o[4]);
  }
  if (area) ww_box_add(R.box, c.area[0].b[0], c.area[0].b[1], c.area[0].b[2], c.area[0].b[3]);
}

static void ww_draw(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  WwRun *cp = S.run[i].ww;
  if (!cp || !cp->vis) return;
  WwRun &c = *cp;
  for (int j = 0; j < c.over; j++) halo_item_draw(c.it[j]);
  if (L.kind == FL_WXDIGITS) ww_draw_digits(c);
#ifdef HALO_P4_TALL
  else if (L.kind == FL_WXCURVE && c.vert) ww_draw_curve_v(c);
#endif
#if defined(HALO_P4_TALL) || defined(HALO_WATCH)
  else if (L.kind == FL_WXWEEK && c.hbar) ww_draw_hbars(c);
#endif
  else if (L.kind == FL_WXCURVE) ww_draw_curve(c);
  else if (L.kind == FL_WXWEEK) ww_draw_bars(c);
  for (const WwCap &k : c.cap) ww_cap_draw(k);
  for (int j = c.over; j < WW_ITEMS; j++) halo_item_draw(c.it[j]);
}
