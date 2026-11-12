// games.h — the Games: self-playing snakes, each a screen of its own behind a card of the Games gallery (the editor's
// Games folder). Three looks picked from ten mockups (web/mockups/snake.html): Dot Snake (4, the LED matrix), Comet
// (7, space: a black hole, asteroids, stars), Voxel Snake (9, isometric cubes); Light Trail (5) was made and taken
// out at the user's word. One game logic for all — snake.h; each look draws itself with its own code (game_*.h).
// A game runs only while its screen is seen (laid out in the last 0.3 s: during the transitions too); away — it waits.
// Each draws only what changed: it keeps what it drew last (cells, boxes) and marks the places that differ
// (halo_damage) — at rest a frame redraws the snake's ends, the food, the sparks, not the screen.
// No gestures of their own: back — as everywhere (a swipe up from the bottom edge, or from the left edge). While a
// game is on screen the watch does not go to Always On (games_busy → settings_busy → power.h).
// Only the games added in the project (FN_SNKDOTS, FN_SNKSPACE, FN_SNKISO — layers.h); the editor's page has all.
// Included from settings.h (its hooks call these).

#if defined(__EMSCRIPTEN__) || (defined(FN_SNKDOTS) && FN_SNKDOTS)
#define HAS_SNKDOTS 1
#else
#define HAS_SNKDOTS 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_SNKSPACE) && FN_SNKSPACE)
#define HAS_SNKSPACE 1
#else
#define HAS_SNKSPACE 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_SNKISO) && FN_SNKISO)
#define HAS_SNKISO 1
#else
#define HAS_SNKISO 0
#endif
#define HAS_GAMES (HAS_SNKDOTS || HAS_SNKSPACE || HAS_SNKISO)

#if HAS_GAMES
#include "snake.h"

// ---------- What every game uses ----------
#ifdef HALO_ROUND
#define GM_TLS
#else
#define GM_TLS HALO_TLS  // (a rectangle draws several games in a frame, each core its own part)
#endif
static GM_TLS HaloView gm_v;  // this frame's view of the screen being laid out / drawn
#ifndef HALO_ROUND
static GM_TLS float gm_rest_s = 1;  // the view's scale when the game stands still
static GM_TLS int gm_k, gm_slot;    // the game in hand; 0 — its own screen, 1 — its card in the gallery
#endif
static float gm_dt = 1 / 60.0f;
static inline float gm_x(float x) { return halo_view_x(gm_v, x); }
static inline float gm_y(float y) { return halo_view_y(gm_v, y); }
#ifdef HALO_ROUND
static inline bool gm_rest() { return gm_v.s == 1 && gm_v.a >= 0.999f; }
#else
static inline bool gm_rest() { return gm_v.s == gm_rest_s && gm_v.a >= 0.999f; }
#endif
static inline uint32_t gm_mix(uint32_t h, int32_t v) { return (h ^ (uint32_t)v) * 16777619u; }
static uint32_t gm_rgb_mix(uint32_t a, uint32_t b, float t) {
  t = halo_clamp01(t);
  const int k = (int)(t * 256);
  auto ch = [&](int s) { return (uint32_t)(((a >> s & 255) * (256 - k) + (b >> s & 255) * k) >> 8) << s; };
  return ch(16) | ch(8) | ch(0);
}
// The view's alpha on a colour (the screens are on black: dimming is the fade)
static inline uint32_t gm_col(uint32_t rgb, float a = 1) { return halo_dim(rgb, (int)(halo_clamp01(a * gm_v.a) * 256)); }
static inline void gm_damage(float x0, float y0, float x1, float y1) { halo_damage_f(gm_x(x0), gm_y(y0), gm_x(x1), gm_y(y1)); }

// A part drawn last frame in a box with a state: changed (state or place) — both places are redrawn
struct GmBox {
  float r[4];
  uint32_t h;
};
static void gm_mark(GmBox &b, float x0, float y0, float x1, float y1, uint32_t h) {
  h = gm_mix(gm_mix(gm_mix(h, (int32_t)(gm_v.x * 64)), (int32_t)(gm_v.y * 64)), (int32_t)(gm_v.s * 1024 + gm_v.a * 64));
  if (h == b.h && x0 == b.r[0] && y0 == b.r[1] && x1 == b.r[2] && y1 == b.r[3]) return;
  if (b.h) halo_damage_f(b.r[0], b.r[1], b.r[2], b.r[3]);
  const float nr[4] = {gm_x(x0), gm_y(y0), gm_x(x1), gm_y(y1)};
  if (x1 >= x0) halo_damage_f(nr[0], nr[1], nr[2], nr[3]);
  b.h = h | 1, memcpy(b.r, nr, sizeof nr);
  if (x1 < x0) b.h = 0;  // (nothing there now)
}

// Sparks: one pool for the game on screen (PSRAM); in screen px of the game (before the view)
struct GmPart {
  float x, y, vx, vy, life, max, size, g, drag;
  uint32_t color;
};
#define GM_PARTS 160
#ifdef HALO_ROUND
static GmPart *gm_parts;
static int gm_np;
static GmBox gm_parts_box;
#else
static GmPart *gm_pool[3];  // one pool a game: they run side by side in the gallery's cards
static int gm_pool_n[3];
static GmBox gm_pool_box[3];
#define gm_parts gm_pool[gm_k]
#define gm_np gm_pool_n[gm_k]
#define gm_parts_box gm_pool_box[gm_k]
#endif
static void gm_burst(float x, float y, int n, float v, float life, const uint32_t *colors, int nc, float size = 2.4f,
                     float g = 0, float drag = 2.5f) {
  if (!gm_parts) return;
  static uint32_t r = 7;
  auto rnd = [&]() { r ^= r << 13, r ^= r >> 17, r ^= r << 5; return (r >> 8) * (1.0f / 16777216); };
  for (int i = 0; i < n && gm_np < GM_PARTS; i++) {
    const float a = rnd() * 2 * HALO_PI, s = v * (0.3f + rnd() * 0.9f);
    gm_parts[gm_np++] = {x, y, cosf(a) * s, sinf(a) * s, life * (0.5f + rnd() * 0.7f), life, size * (0.6f + rnd() * 0.8f),
                         g, drag, colors[(int)(rnd() * nc) % nc]};
  }
}
static void gm_parts_tick(float dt, bool bounce) {
  const float k = expf(-dt);  // (drag: k^drag ≈ e^(−drag·dt))
  int m = 0;
  for (int i = 0; i < gm_np; i++) {
    GmPart p = gm_parts[i];
    const float kd = powf(k, p.drag);
    p.vx *= kd, p.vy = p.vy * kd + p.g * dt, p.x += p.vx * dt, p.y += p.vy * dt, p.life -= dt;
    if (bounce) {
      const float dx = p.x - CX, dy = p.y - CX, d2 = dx * dx + dy * dy;
      if (d2 > 228 * 228) {
        const float d = halo_sqrt(d2), nx = dx / d, ny = dy / d, dot = p.vx * nx + p.vy * ny;
        p.vx -= 1.7f * dot * nx, p.vy -= 1.7f * dot * ny, p.x = CX + nx * 227, p.y = CX + ny * 227;
      }
    }
    if (p.life > 0) gm_parts[m++] = p;
  }
  gm_np = m;
}
static void gm_parts_layout() {  // their box: redrawn while any is alive (and once more when the last goes)
  float x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9;
  uint32_t h = 2166136261u;
  for (int i = 0; i < gm_np; i++) {
    const GmPart &p = gm_parts[i];
    x0 = fminf(x0, p.x - p.size - 1), y0 = fminf(y0, p.y - p.size - 1), x1 = fmaxf(x1, p.x + p.size + 1), y1 = fmaxf(y1, p.y + p.size + 1);
    h = gm_mix(h, (int32_t)(p.x * 4) ^ (int32_t)(p.y * 4) << 12 ^ (int32_t)(p.life * 30) << 22);
  }
  gm_mark(gm_parts_box, x0, y0, x1, y1, h);
}
static void gm_parts_draw(bool square) {
  for (int i = 0; i < gm_np; i++) {
    const GmPart &p = gm_parts[i];
    const float a = halo_clamp01(p.life / p.max), s = p.size * gm_v.s, x = gm_x(p.x), y = gm_y(p.y);
    if (!halo_hits(x - s, y - s, x + s, y + s)) continue;
    if (square) halo_rect((int)(x - s), (int)(y - s), (int)(x + s), (int)(y + s), gm_col(p.color, a));
    else halo_circle(x, y, s, gm_col(p.color, a));
  }
}
// A rounded square as a 4-bit mask (MW × MW, the square size × size in its middle, corners r; 4 × 4 samples a pixel) —
// drawn with halo_mask on black: ready edge colours, no blending
static void gm_make_rrect(HaloMask &m, int MW, float size, float r) {
  const int bpr = (MW + 1) / 2;
  uint8_t *bits = (uint8_t *)heap_caps_calloc(MW * bpr + MW * 2, 1, MALLOC_CAP_SPIRAM);
  if (!bits) return;
  uint8_t *ext = bits + MW * bpr;
  const float c = MW / 2.0f, h = size / 2 - r;
  for (int y = 0; y < MW; y++) {
    int first = MW, last = -1;
    for (int x = 0; x < MW; x++) {
      int in = 0;
      for (int sy = 0; sy < 4; sy++)
        for (int sx = 0; sx < 4; sx++) {
          const float px = fabsf(x + (sx + 0.5f) / 4 - c), py = fabsf(y + (sy + 0.5f) / 4 - c);
          const float dx = fmaxf(0, px - h), dy = fmaxf(0, py - h);
          in += px <= size / 2 && py <= size / 2 && dx * dx + dy * dy <= r * r;
        }
      const int v = (in * 15 + 8) / 16;
      if (!v) continue;
      bits[y * bpr + x / 2] |= x & 1 ? v : v << 4;
      first = min(first, x), last = max(last, x);
    }
    ext[2 * y] = first > last ? 1 : first, ext[2 * y + 1] = first > last ? 0 : last;
  }
  m = {(int16_t)MW, (int16_t)MW, 0, 0, 0, bits, ext};
}

// The game shown in this frame (games_tick runs only that one), and the view it was laid out with (for its draw)
static uint32_t gm_seen[FL_SNKISO - FL_SNKDOTS + 1];
#ifdef HALO_ROUND
static HaloView gm_views[FL_SNKISO - FL_SNKDOTS + 1];
#else
static HaloView gm_views[FL_SNKISO - FL_SNKDOTS + 1][2];  // its screen, its card
static uint32_t gm_full[FL_SNKISO - FL_SNKDOTS + 1];     // (the screen itself seen: only that keeps Always On away)
#endif

#ifndef HALO_ROUND
// Where the worlds stand on this screen (their middle — at this y — and their scale): full size, and in a card of the gallery
#define GM_SPACE_CY 512.0f  // Comet: the hole, at (CX, CX) as on the watch
#define GM_SPACE_S 1.4f
#define GM_SPACE_MINI 0.7f
#define GM_ISO_CY 250.0f    // Voxel Snake: the board's middle on the watch
#define GM_ISO_S 1.5f
#define GM_ISO_Y 310.0f     // … and on this screen
#define GM_ISO_MINI 0.78f
#endif

#if HAS_SNKDOTS
#ifdef HALO_ROUND
#include "game_dots.h"
#else
#include "game_dots_rect.h"
#endif
#endif
#if HAS_SNKSPACE
#include "game_space.h"
#endif
#if HAS_SNKISO
#include "game_iso.h"
#endif

// ---------- Hooks (settings.h) ----------
static inline bool fl_game(uint8_t k) { return k >= FL_SNKDOTS && k <= FL_SNKISO; }
// A game on screen (laid out in the last 0.3 s): the watch is busy — no Always On while it plays (settings_busy)
static bool games_busy() {
  const uint32_t now = halo_ms();
#ifdef HALO_ROUND
  for (uint32_t t : gm_seen)
#else
  for (uint32_t t : gm_full)
#endif
    if (t && now - t < 300) return true;
  return false;
}
static void games_build() {
#ifdef HALO_ROUND
  gm_parts = (GmPart *)heap_caps_malloc(GM_PARTS * sizeof(GmPart), MALLOC_CAP_SPIRAM);
#else
  for (gm_k = 0; gm_k < 3; gm_k++) gm_parts = (GmPart *)heap_caps_malloc(GM_PARTS * sizeof(GmPart), MALLOC_CAP_SPIRAM);
  gm_k = 0;
#endif
#if HAS_SNKDOTS
  dots_build();
#endif
#if HAS_SNKSPACE
  space_build();
#endif
#if HAS_SNKISO
  iso_build();
#endif
}
static void games_tick(float dt) {  // every frame: the game on screen moves
  gm_dt = dt;
  const uint32_t now = halo_ms();
  auto on = [&](int k) { return gm_seen[k] && now - gm_seen[k] < 300; };
  (void)on;
#if HAS_SNKDOTS
  if (on(0)) dots_tick(dt);
#endif
#if HAS_SNKSPACE
#ifndef HALO_ROUND
  gm_k = FL_SNKSPACE - FL_SNKDOTS;
#endif
  if (on(FL_SNKSPACE - FL_SNKDOTS)) space_tick(dt);
#endif
#if HAS_SNKISO
#ifndef HALO_ROUND
  gm_k = FL_SNKISO - FL_SNKDOTS;
#endif
  if (on(FL_SNKISO - FL_SNKDOTS)) iso_tick(dt);
#endif
}
// The screen moving (a transition, the swipe to the face) or hidden: its whole square is redrawn — old place and new
// (the games mark only what changes at rest: their dots, stars, board would stay on the panel where they were).
// Hidden (alpha 0 — the nav lays out a screen that is gone): nothing is drawn, nothing laid out — dimming to black
// there would paint black over the screen under it (the face after a swipe up), and it would stay until redrawn
static void gm_view_damage(const HaloView &a, const HaloView &b) {
  for (const HaloView *v : {&a, &b})
    if (v->a > 0.004f) halo_damage_f(halo_view_x(*v, 0) - 2, halo_view_y(*v, 0) - 2, halo_view_x(*v, HALO_W) + 2, halo_view_y(*v, HALO_H) + 2);
}
#ifdef HALO_ROUND
static void games_layout(uint8_t kind, const HaloView &v) {
  const int k = kind - FL_SNKDOTS;
  const HaloView was = gm_views[k];
  if (v.x != was.x || v.y != was.y || v.s != was.s || v.a != was.a) gm_view_damage(was, v);
  gm_v = gm_views[k] = v;
  if (v.a <= 0.004f) return;
  gm_seen[k] = halo_ms();
#if HAS_SNKDOTS
  if (kind == FL_SNKDOTS) dots_layout();
#endif
#if HAS_SNKSPACE
  if (kind == FL_SNKSPACE) space_layout();
#endif
#if HAS_SNKISO
  if (kind == FL_SNKISO) iso_layout();
#endif
}
static void games_draw(uint8_t kind) {
  gm_v = gm_views[kind - FL_SNKDOTS];
  if (gm_v.a <= 0.004f) return;  // (hidden: see games_layout)
#if HAS_SNKDOTS
  if (kind == FL_SNKDOTS) dots_draw();
#endif
#if HAS_SNKSPACE
  if (kind == FL_SNKSPACE) space_draw();
#endif
#if HAS_SNKISO
  if (kind == FL_SNKISO) iso_draw();
#endif
}
#else
// A rectangle. Comet and Voxel Snake keep the watch's numbers: a base view places their world on this screen (the view's
// scale and shift are all that changes, so the look is the watch's), Dot Snake has a grid of its own (game_dots_rect.h).
static HaloView gm_base(uint8_t kind) {
  HaloView b;
  if (kind == FL_SNKSPACE) b.s = GM_SPACE_S, b.y = HALO_H / 2.0f - HALO_H / 2.0f - (GM_SPACE_CY - HALO_H / 2.0f) * GM_SPACE_S;
  if (kind == FL_SNKISO) b.s = GM_ISO_S, b.y = GM_ISO_Y - HALO_H / 2.0f - (GM_ISO_CY - HALO_H / 2.0f) * GM_ISO_S;
  return b;
}
static HaloView gm_compose(const HaloView &v, const HaloView &b) {  // b first, then v
  HaloView c = v;
  c.s = v.s * b.s, c.x = v.x + b.x * v.s, c.y = v.y + b.y * v.s;
  return c;
}
static void gm_select(int k, int slot, const HaloView &v, float rest_s) {
  gm_k = k, gm_slot = slot, gm_v = v, gm_rest_s = rest_s;
}
static void games_layout(uint8_t kind, const HaloView &v) {
  const int k = kind - FL_SNKDOTS;
  const HaloView was = gm_views[k][0];
  if (v.x != was.x || v.y != was.y || v.s != was.s || v.a != was.a) gm_view_damage(was, v);
  gm_views[k][0] = v;
  if (v.a <= 0.004f) return;
  gm_seen[k] = gm_full[k] = halo_ms();
  const HaloView b = gm_base(kind);
  gm_select(k, 0, gm_compose(v, b), b.s);
#if HAS_SNKDOTS
  if (kind == FL_SNKDOTS) dots_layout();
#endif
#if HAS_SNKSPACE
  if (kind == FL_SNKSPACE) space_layout();
#endif
#if HAS_SNKISO
  if (kind == FL_SNKISO) iso_layout();
#endif
}
static void games_draw(uint8_t kind) {
  const int k = kind - FL_SNKDOTS;
  if (gm_views[k][0].a <= 0.004f) return;
  const HaloView b = gm_base(kind);
  gm_select(k, 0, gm_compose(gm_views[k][0], b), b.s);
#if HAS_SNKDOTS
  if (kind == FL_SNKDOTS) dots_draw();
#endif
#if HAS_SNKSPACE
  if (kind == FL_SNKSPACE) space_draw();
#endif
#if HAS_SNKISO
  if (kind == FL_SNKISO) iso_draw();
#endif
}

// ---------- A game alive in its card (variant K1): the gallery's page view, the part of the card it shows ----------
static struct {
  float x0, y0, x1, y1, sx, sy;
} gm_mini_at[FL_SNKISO - FL_SNKDOTS + 1];
static GmBox gm_mini_box[FL_SNKISO - FL_SNKDOTS + 1];
static void games_mini_layout(uint8_t kind, const HaloView &page, float x, float y, float w, float h, float a) {
  const int k = kind - FL_SNKDOTS;
  HaloView v = page;
  v.a *= a;
  gm_select(k, 1, page, -1);  // (gm_mark goes through the page's view; -1 — this is never "at rest" for gm_rest)
  gm_mark(gm_mini_box[k], x, y, a > 0.004f ? x + w : x - 1, y + h, 1);
  gm_mini_at[k] = {halo_view_x(page, x), halo_view_y(page, y), halo_view_x(page, x + w), halo_view_y(page, y + h), x + w / 2, y + h / 2};
  if (a <= 0.004f) return gm_views[k][1].a = 0, void();
  gm_seen[k] = halo_ms();
  if (kind == FL_SNKDOTS) {
#if HAS_SNKDOTS
    gm_views[k][1] = v, dots_mini_layout(v, x, y);
#endif
    return;
  }
  // The world's middle in the middle of the part shown, at the scale that fits; the whole part is redrawn: it all moves
  const float cy = kind == FL_SNKSPACE ? GM_SPACE_CY : GM_ISO_CY, S = kind == FL_SNKSPACE ? GM_SPACE_MINI : GM_ISO_MINI;
  HaloView m = v;
  m.s = v.s * S;
  m.x = halo_view_x(page, x + w / 2) - HALO_W / 2.0f;
  m.y = halo_view_y(page, y + h / 2) - HALO_H / 2.0f - (cy - HALO_H / 2.0f) * m.s;
  gm_views[k][1] = m;
  gm_select(k, 1, m, -1);
  halo_damage_f(gm_mini_at[k].x0, gm_mini_at[k].y0, gm_mini_at[k].x1, gm_mini_at[k].y1);
#if HAS_SNKSPACE
  if (kind == FL_SNKSPACE) space_layout();
#endif
#if HAS_SNKISO
  if (kind == FL_SNKISO) iso_layout();
#endif
}
static void games_mini_draw(uint8_t kind) {
  const int k = kind - FL_SNKDOTS;
  if (gm_views[k][1].a <= 0.004f) return;
  const auto &q = gm_mini_at[k];
  if (!halo_hits(q.x0, q.y0, q.x1, q.y1)) return;
  const HaloBand band = hb;
  const int ax = max(band.x0, halo_floor(q.x0 + 0.5f)), bx = min(band.x1, halo_floor(q.x1 - 0.5f));
  const int ay = max(band.y, halo_floor(q.y0 + 0.5f)), by = min(band.y + band.rows - 1, halo_floor(q.y1 - 0.5f));
  if (ax > bx || ay > by) return;
  hb.buf = band.buf + (ay - band.y) * band.w, hb.y = ay, hb.rows = by - ay + 1, hb.x0 = ax, hb.x1 = bx;
  for (int r = ay; r <= by; r++) halo_span(halo_row(r), ax, bx, 0);  // the scene's black (the card under it is grey)
  gm_select(k, 1, gm_views[k][1], -1);
#if HAS_SNKDOTS
  if (kind == FL_SNKDOTS) dots_mini_draw(gm_views[k][1], gm_mini_at[k].sx - dt_mini.w / 2, gm_mini_at[k].sy - dt_mini.h / 2);
#endif
#if HAS_SNKSPACE
  if (kind == FL_SNKSPACE) space_draw();
#endif
#if HAS_SNKISO
  if (kind == FL_SNKISO) iso_draw();
#endif
  hb = band;
}
#endif
#endif

#if !HAS_GAMES && !defined(HALO_ROUND)  // (the gallery calls them whatever the project has)
static void games_mini_layout(uint8_t, const HaloView &, float, float, float, float, float) {}
static void games_mini_draw(uint8_t) {}
#endif
