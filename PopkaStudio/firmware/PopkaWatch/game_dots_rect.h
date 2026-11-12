// game_dots_rect.h — Dot Snake on a rectangular screen (variant N2 of web/mockups/rect-apps.html): the whole screen is a
// matrix of big dots, 28 x 16, and a second, smaller game of its own lives in the Games gallery's card (14 px apart, as the
// mockup K1). The rules and the look are the watch's (game_dots.h): lit dots, a bigger head, a fading trail, a blinking red
// food with a cross round it; every dot's look is kept, so only the dots that changed are redrawn. Included from games.h.

struct DotGame {
  Snake *s;
  float *heat;                  // each cell: lit (1) -> unlit (0)
  uint8_t *drawn, *look;        // each cell's look: colour << 5 | big << 4 | brightness 0..15
  HaloMask dot[2];              // a dot and the bigger one (the head, the food)
  int cols, rows;
  float pitch, ox, oy, w, h;    // from the top-left of its area
};
static DotGame dt_full, dt_mini;
static float dt_t;
static const uint32_t DT_COL[3] = {0xFFF8EE, 0x9AA0A6, 0xFF4D4D};

static void dot_make(DotGame &g, int cols, int rows, float pitch, float w, float h, float tick, int cap, uint32_t seed, int mw0, int mw1) {
  g.s = (Snake *)heap_caps_calloc(1, sizeof(Snake), MALLOC_CAP_SPIRAM);
  g.heat = (float *)heap_caps_calloc(cols * rows, sizeof(float), MALLOC_CAP_SPIRAM);
  g.drawn = (uint8_t *)heap_caps_calloc(cols * rows, 2, MALLOC_CAP_SPIRAM);
  if (!g.s || !g.heat || !g.drawn) {
    g.s = nullptr;
    return;
  }
  g.look = g.drawn + cols * rows;
  g.cols = cols, g.rows = rows, g.pitch = pitch, g.w = w, g.h = h;
  g.ox = (w - (cols - 1) * pitch) / 2, g.oy = (h - (rows - 1) * pitch) / 2;
  sn_init_rect(*g.s, cols, rows, tick, cap, seed);
  gm_make_rrect(g.dot[0], mw0, pitch * 0.62f, pitch * 0.62f * 0.3f), gm_make_rrect(g.dot[1], mw1, pitch * 0.74f, pitch * 0.74f * 0.3f);
}

static void dots_build() {
  dot_make(dt_full, 28, 16, 36, HALO_W, HALO_H, 0.11f, 48, 11, 26, 30);
  dot_make(dt_mini, 22, 25, 14, 309, 350, 0.1f, 30, 7, 12, 14);
}

static void dot_tick(DotGame &g, float dt) {
  if (!g.s) return;
  Snake &S = *g.s;
  sn_update(S, dt), S.ev_n = 0;
  const float k = expf(-dt * 4);
  const int N = g.cols * g.rows;
  for (int i = 0; i < N; i++) g.heat[i] *= k;
  for (int i = 0; i < S.n; i++) g.heat[S.body[i]] = fmaxf(g.heat[S.body[i]], 1 - 0.5f * i / S.n);
}

static void dots_tick(float dt) {
  dot_tick(dt_full, dt), dot_tick(dt_mini, dt);
  dt_t += dt;
}

// What each dot looks like now; rest — only the ones that changed are redrawn. (x0, y0) — its area on the page
static void dot_layout(DotGame &g, const HaloView &v, float x0, float y0, bool rest) {
  if (!g.s) return;
  const Snake &S = *g.s;
  const bool blink = sinf(dt_t * 9) > -0.3f;
  const int fx = S.food % g.cols, fy = S.food / g.cols, N = g.cols * g.rows;
  for (int c = 0; c < N; c++) {
    int col = 0, a = 1, big = 0;
    const int h = (int)(g.heat[c] * 15 + 0.5f);
    if (h > a) a = h;
    const int d = abs(c % g.cols - fx) + abs(c / g.cols - fy);
    if (d == 0) col = 2, a = blink ? 15 : 6, big = 1;
    else if (d == 1 && blink && a < 6) col = 2, a = 6;
    if (c == S.body[0]) big = 1;
    g.look[c] = col << 5 | big << 4 | a;
    if (rest && g.look[c] != g.drawn[c]) {
      const float x = x0 + g.ox + (c % g.cols) * g.pitch, y = y0 + g.oy + (c / g.cols) * g.pitch, r = g.pitch * 0.44f;
      halo_damage_f(halo_view_x(v, x - r), halo_view_y(v, y - r), halo_view_x(v, x + r), halo_view_y(v, y + r));
    }
    g.drawn[c] = g.look[c];
  }
}

static void dot_draw(const DotGame &g, const HaloView &v, float x0, float y0) {
  if (!g.s) return;
  const float s = v.s;
  const bool rest = s == 1 && v.a >= 0.999f;
  for (int cy = 0; cy < g.rows; cy++) {
    const float py = halo_view_y(v, y0 + g.oy + cy * g.pitch), half = g.pitch * 0.44f * s;
    if (py + half < hb.y || py - half >= hb.y + hb.rows) continue;
    for (int cx = 0; cx < g.cols; cx++) {
      const uint8_t L = g.look[cy * g.cols + cx];
      const HaloMask &m = g.dot[L >> 4 & 1];
      const float px = halo_view_x(v, x0 + g.ox + cx * g.pitch), hw = m.w * s / 2;
      if (px + hw < hb.x0 || px - hw > hb.x1 || !m.bits) continue;
      const uint32_t c = halo_dim(DT_COL[L >> 5], (int)(halo_clamp01((L & 15) / 15.0f * v.a) * 256));
      if (!rest && (L & 15) <= 1) {  // (in a transition: an unlit dot is too dim to show its corners)
        const float q = g.pitch * 0.31f * s;
        halo_rect((int)(px - q), (int)(py - q), (int)(px + q), (int)(py + q), c);
        continue;
      }
      halo_mask(m, px - hw, py - hw, c, 1, s, 0x000000);
    }
  }
}

static void dots_layout() {
  dot_layout(dt_full, gm_v, 0, 0, gm_rest());
}

static void dots_draw() {
  dot_draw(dt_full, gm_v, 0, 0);
}

static void dots_mini_layout(const HaloView &v, float x, float y) {
  dot_layout(dt_mini, v, x, y, v.s == 1 && v.a >= 0.999f);
}

static void dots_mini_draw(const HaloView &v, float x, float y) {
  dot_draw(dt_mini, v, x, y);
}
