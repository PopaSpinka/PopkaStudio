// game_dots.h — Dot Snake (mockup 4): the whole screen is a matrix of dots, like the Recorder's wave — 29 across,
// the outer ones touching the screen's edge. The snake — lit dots, the head a little bigger; behind it a trail that
// fades (each cell's heat, 1/4 s); the food blinks red with a red cross round it. No walls, no score (the user's word).
// Every cell's look (colour, brightness in 16 steps, size) is kept: only cells whose look changed are redrawn —
// the snake's ends, the trail as it fades, the food. Included from games.h.

#define DT_G 29
static Snake *dt_s;
static float *dt_heat;     // each cell: lit (1) → unlit (0)
static uint8_t *dt_drawn;  // each cell's look last frame: colour << 5 | big << 4 | brightness (0..15)
static uint8_t *dt_look;
static HaloMask dt_dot[2];  // a dot and the bigger one (the head, the food)
static float dt_t;
static const uint32_t DT_COL[3] = {0xFFF8EE, 0x9AA0A6, 0xFF4D4D};  // the snake, (walls), the food

static void dots_build() {
  dt_s = (Snake *)heap_caps_calloc(1, sizeof(Snake), MALLOC_CAP_SPIRAM);
  dt_heat = (float *)heap_caps_calloc(SN_MAXC, sizeof(float), MALLOC_CAP_SPIRAM);
  dt_drawn = (uint8_t *)heap_caps_calloc(SN_MAXC, 2, MALLOC_CAP_SPIRAM), dt_look = dt_drawn + SN_MAXC;
  if (!dt_s || !dt_heat || !dt_drawn) return;
  // 233 px — the screen's radius; edge 0.35 cell: the outermost dots' outer edges just touch it
  sn_init(*dt_s, DT_G, 0.085f, 48, nullptr, 0, 11, 233, 0.35f);
  const float cs = dt_s->cs;
  gm_make_rrect(dt_dot[0], 12, cs * 0.62f, cs * 0.62f * 0.3f), gm_make_rrect(dt_dot[1], 14, cs * 0.74f, cs * 0.74f * 0.3f);
}

static void dots_tick(float dt) {
  if (!dt_s) return;
  Snake &S = *dt_s;
  sn_update(S, dt), S.ev_n = 0;  // (no sparks here: the trail is the effect)
  dt_t += dt;
  const float k = expf(-dt * 4);
  for (int i = 0; i < DT_G * DT_G; i++) dt_heat[i] *= k;
  for (int i = 0; i < S.n; i++) dt_heat[S.body[i]] = fmaxf(dt_heat[S.body[i]], 1 - 0.5f * i / S.n);
}

static inline float dt_cx(int c) { return sn_px(*dt_s, c % DT_G); }
static inline float dt_cy(int c) { return sn_px(*dt_s, c / DT_G); }

static void dots_layout() {
  if (!dt_s) return;
  const Snake &S = *dt_s;
  const bool blink = sinf(dt_t * 9) > -0.3f, rest = gm_rest();
  const int fx = S.food % DT_G, fy = S.food / DT_G;
  for (int c = 0; c < DT_G * DT_G; c++) {
    if (!S.valid[c]) continue;
    int col = 0, a = 1, big = 0;  // unlit: 1/16
    if (S.obst[c]) col = 1, a = (int)(6.5f + 1.3f * sinf(dt_t * 2 + c % DT_G));
    const int h = (int)(dt_heat[c] * 15 + 0.5f);
    if (h > a) col = 0, a = h;
    const int d = abs(c % DT_G - fx) + abs(c / DT_G - fy);
    if (d == 0) col = 2, a = blink ? 15 : 6, big = 1;
    else if (d == 1 && blink && a < 6) col = 2, a = 6;
    if (c == S.body[0]) big = 1;
    dt_look[c] = col << 5 | big << 4 | a;
    if (rest && dt_look[c] != dt_drawn[c]) {  // its dot changed: just its square
      const float x = dt_cx(c), y = dt_cy(c), r = S.cs * 0.4f;
      halo_damage_f(x - r, y - r, x + r, y + r);
    }
    dt_drawn[c] = dt_look[c];
  }
}

static void dots_draw() {
  if (!dt_s) return;
  const Snake &S = *dt_s;
  const float s = gm_v.s;
  // Only the rows of cells this band touches
  const float top = CX - S.inner;
  const bool rest = gm_rest();
  for (int cy = 0; cy < DT_G; cy++) {
    const float py = gm_y(top + (cy + 0.5f) * S.cs), half = S.cs * 0.4f * s;
    if (py + half < hb.y || py - half >= hb.y + hb.rows) continue;
    for (int cx = 0; cx < DT_G; cx++) {
      const int c = cy * DT_G + cx;
      if (!S.valid[c]) continue;
      const uint8_t L = dt_look[c];
      const HaloMask &m = dt_dot[L >> 4 & 1];
      const float px = gm_x(top + (cx + 0.5f) * S.cs), hw = m.w * s / 2;
      if (px + hw < hb.x0 || px - hw > hb.x1 || !m.bits) continue;
      if (!rest && (L & 15) <= 1) {  // (in a transition: an unlit dot — a plain square, it is too dim to show edges)
        const float q = S.cs * 0.31f * s;
        halo_rect((int)(px - q), (int)(py - q), (int)(px + q), (int)(py + q), gm_col(DT_COL[L >> 5], 1 / 15.0f));
        continue;
      }
      halo_mask(m, px - hw, py - hw, gm_col(DT_COL[L >> 5], (L & 15) / 15.0f), 1, s, 0x000000);  // (on black: no blending)
    }
  }
}
