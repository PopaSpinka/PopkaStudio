// game_space.h — Comet (mockup 7): the head is a comet — a glow with a white core and a tail of sparks; the body —
// a chain of glowing orbs that fade toward the tail; in the middle a black hole: a disc of hot blobs turning round it
// (the back half behind it, the front half before it), a photon ring, dust spiralling in; the stars of the sky orbit
// the hole the way its disc turns — the nearer, the faster (ω ∝ r^−1.6).
// The board is polar (5 Nov, the user's idea): 8 orbits round the hole (52…215 px out) × 40 steps round — the snake
// goes round the orbits and along the rays between them; faint dotted lines show the orbits it runs on (dots made
// at start, kept by the band they fall in: a band draws only its own — solid rings cost a frame its 60). The food — a
// twinkling star of a colour of its own each time (as in the photos of nebulae), its sparks of that colour when eaten;
// it appears on an orbit and sails on with the sky at that orbit's speed, and the snake meets it: before each step it
// looks where the star will be k steps on and heads for the first such cell it can reach by then (sp_before; eaten
// when the head enters the cell the star is in). On the outer orbits a step is long (up to 32 px): orbs in between,
// along the arc (a bright core only — no glow: they would double the glows to draw), keep the body whole. No walls, no score (the user's word). The glows are one radial mask (4-bit, made
// at start) drawn at any size and colour. Redrawn: the stars whose pixel changed, the hole (it turns), each piece of
// the body (they slide), the food, the sparks. Included from games.h.

#define SP_A 40      // steps round
#define SP_R 8       // orbits
#define SP_R0 52.0f  // the first orbit's inner edge, px
#define SP_DR 20.4f  // an orbit's width, px
#ifdef HALO_ROUND
#define SP_STARS 170
#define SP_SKY 236           // the stars: out to here
#define SP_H(v) (v)          // the hole's size
#define SP_S(v) (v)          // its disc's blobs
#define SP_BAND_REST gm_rest()
#else
#define SP_STARS 340         // (the sky goes on past the orbits, to the screen's sides)
#define SP_SKY 430
#define SP_H(v) ((v) * 1.6f)
#define SP_S(v) ((v) * 1.05f)
#define SP_BAND_REST false   // (the dots are bucketed by row of the watch's world, not of this screen)
#endif
#define SP_GLOW 48
struct SpStar {
  float r, a, w;   // round the hole: radius, angle, rad/s
  uint8_t l;       // brightness 1..3
  int16_t dx, dy;  // where it was drawn
};
static Snake *sp_s;
static SpStar *sp_star;
static GmBox *sp_orb;  // a box per piece of the body (an orb and the ones between it and the next)
static HaloMask sp_glow;
static float sp_t;
static GmBox sp_hole_box, sp_food_box;
#ifndef HALO_ROUND
// Stars and orbit dots are found by row of the world (16 px a bucket), for any view: a draw of one band touches only its own
#define SP_GB 64
static int16_t sp_star_at[SP_GB + 2], sp_star_of[SP_STARS];  // bucket -> first in sp_star_of
#endif
// The orbits' dots: screen px, sorted by row; sp_band[b] — the first dot of band b (16 rows each)
#define SP_ODOTS (SP_R * 96)
static int16_t (*sp_odot)[2];
static int16_t sp_band[HALO_BANDS + 1];
#ifndef HALO_ROUND
static int16_t sp_gband[SP_GB + 2];  // the same dots by rows of the world
#endif
// The food's colours: as the stars in the photos of nebulae
static const uint32_t SP_FOOD[7] = {0xFF6BD6, 0x7AA2FF, 0xFFB347, 0xB388FF, 0x64FFDA, 0xFFE28A, 0xFF6B6B};
static uint32_t sp_food_col = 0xFFE28A;
static uint32_t sp_food_rnd = 5, sp_col_n;
static float sp_fr = 100, sp_fa, sp_fw;  // the food star's orbit: radius, angle, rad/s
static uint32_t sp_food_n;              // the food (S.food_n) the orbit is for
static inline float sp_omega(float r) { return 0.55f * powf(55 / r, 1.6f); }  // the sky's: nearer — faster
static inline float sp_food_x() { return CX + sp_fr * cosf(sp_fa); }
static inline float sp_food_y() { return CX + sp_fr * sinf(sp_fa); }
// The board's cells on screen: a (steps round, fractional) and r (orbits out) → px; clockwise, as the sky turns
static inline void sp_xy(float a, float r, float &x, float &y) {
  const float t = (a + 0.5f) * (2 * HALO_PI / SP_A), R = SP_R0 + (r + 0.5f) * SP_DR;
  x = CX + R * cosf(t), y = CX + R * sinf(t);
}
static inline void sp_cxy(int c, float &x, float &y) { sp_xy(c % SP_A, c / SP_A, x, y); }
static int16_t sp_cell_at(float x, float y) {  // the cell a point is in (−1: off the orbits)
  const float dx = x - CX, dy = y - CX;
  const int r = halo_floor((sqrtf(dx * dx + dy * dy) - SP_R0) / SP_DR);
  if (r < 0 || r >= SP_R) return -1;
  float t = atan2f(dy, dx);
  if (t < 0) t += 2 * HALO_PI;
  return r * SP_A + (int)(t * (SP_A / (2 * HALO_PI))) % SP_A;
}
// A food just placed: on its orbit (the middle of its ring), at its cell's angle
static void sp_food_orbit(Snake &S) {
  const int a = S.food % SP_A, r = S.food / SP_A;
  sp_fr = SP_R0 + (r + 0.5f) * SP_DR, sp_fa = (a + 0.5f) * (2 * HALO_PI / SP_A), sp_fw = sp_omega(sp_fr);
  sp_food_n = S.food_n;
}
// Before each step: the star's cell now — the food; where to meet it — the target
static void sp_before(Snake &S) {
  if (S.food_n != sp_food_n) sp_food_orbit(S);
  const int16_t now = sp_cell_at(sp_food_x(), sp_food_y());
  if (now >= 0) S.food = now;
  sn_bfs(S, S.body[0], -1, S.body, S.n);  // (S.dist: steps to every cell it can reach)
  S.aim = false;
  for (int k = 1; k <= 48; k++) {
    const float a = sp_fa + sp_fw * k * S.tick;
    const int16_t c = sp_cell_at(CX + sp_fr * cosf(a), CX + sp_fr * sinf(a));
    if (c >= 0 && S.dist[c] > 0 && S.dist[c] <= k) {
      S.target = c, S.aim = true;
      break;
    }
  }
}

static void sp_make_glow() {  // a radial glow: (1 − r/R)² in 16 steps
  const int D = SP_GLOW, bpr = (D + 1) / 2;
  uint8_t *bits = (uint8_t *)heap_caps_calloc(D * bpr + D * 2, 1, MALLOC_CAP_SPIRAM);
  if (!bits) return;
  uint8_t *ext = bits + D * bpr;
  const float R = D / 2.0f;
  for (int y = 0; y < D; y++) {
    int first = D, last = -1;
    for (int x = 0; x < D; x++) {
      const float dx = x + 0.5f - R, dy = y + 0.5f - R, d = sqrtf(dx * dx + dy * dy) / R;
      if (d >= 1) continue;
      const int v = (int)(15 * (1 - d) * (1 - d) + 0.5f);
      if (!v) continue;
      bits[y * bpr + x / 2] |= x & 1 ? v : v << 4;
      first = min(first, x), last = max(last, x);
    }
    ext[2 * y] = first > last ? 1 : first, ext[2 * y + 1] = first > last ? 0 : last;
  }
  sp_glow = {(int16_t)D, (int16_t)D, 0, 0, 0, bits, ext};
}

static void space_build() {
  sp_s = (Snake *)heap_caps_calloc(1, sizeof(Snake), MALLOC_CAP_SPIRAM);
  sp_star = (SpStar *)heap_caps_calloc(SP_STARS, sizeof(SpStar), MALLOC_CAP_SPIRAM);
  sp_orb = (GmBox *)heap_caps_calloc(SN_MAXL, sizeof(GmBox), MALLOC_CAP_SPIRAM);
  if (!sp_s || !sp_star || !sp_orb) return void(sp_s = nullptr);
  sn_init_polar(*sp_s, SP_A, SP_R, 0.11f, 40, 23);
  sp_s->before_step = sp_before;
  uint32_t r = 31;
  auto rnd = [&]() { r ^= r << 13, r ^= r >> 17, r ^= r << 5; return (r >> 8) * (1.0f / 16777216); };
  for (int i = 0; i < SP_STARS; i++) {  // even over the area from the disc out (r 46…236); ω ∝ r^−1.6
    const float rr = sqrtf(46 * 46 + rnd() * (SP_SKY * SP_SKY - 46 * 46));
    sp_star[i] = {rr, rnd() * 2 * HALO_PI, sp_omega(rr), (uint8_t)(1 + (int)(rnd() * 3)), -9, -9};
  }
  sp_make_glow();
  // The orbits as dots: 96 round each (more out: farther apart), sorted by row and bucketed by band
  sp_odot = (int16_t(*)[2])heap_caps_malloc(SP_ODOTS * 4, MALLOC_CAP_SPIRAM);
  if (sp_odot) {
    int m = 0;
    for (int o = 0; o < SP_R; o++)
      for (int k = 0; k < 96; k++) {
        const float R = SP_R0 + (o + 0.5f) * SP_DR, t = (k + 0.5f) * (2 * HALO_PI / 96);
        sp_odot[m][0] = (int16_t)lroundf(CX + R * cosf(t)), sp_odot[m][1] = (int16_t)lroundf(CX + R * sinf(t)), m++;
      }
    for (int i = 1; i < m; i++) {  // (by row: insertion sort once)
      int16_t d[2] = {sp_odot[i][0], sp_odot[i][1]};
      int j = i - 1;
      while (j >= 0 && sp_odot[j][1] > d[1]) sp_odot[j + 1][0] = sp_odot[j][0], sp_odot[j + 1][1] = sp_odot[j][1], j--;
      sp_odot[j + 1][0] = d[0], sp_odot[j + 1][1] = d[1];
    }
    for (int b = 0, i = 0; b <= HALO_BANDS; b++) {
      while (i < m && sp_odot[i][1] < b * HALO_ROWS) i++;
      sp_band[b] = i;
    }
#ifndef HALO_ROUND
    for (int b = 0, i = 0; b <= SP_GB + 1; b++) {
      while (i < m && sp_odot[i][1] < b * 16) i++;
      sp_gband[b] = i;
    }
#endif
  }
}

static void space_tick(float dt) {
  if (!sp_s) return;
  Snake &S = *sp_s;
  sn_update(S, dt), sp_t += dt;
  for (int i = 0; i < SP_STARS; i++) {  // the stars orbit the hole the way its disc turns (clockwise on screen)
    SpStar &s = sp_star[i];
    if ((s.a += s.w * dt) > 2 * HALO_PI) s.a -= 2 * HALO_PI;
  }
  if ((sp_fa += sp_fw * dt) > 2 * HALO_PI) sp_fa -= 2 * HALO_PI;  // the food star too, with them
  static const uint32_t SPARK[3] = {0xFFFFFF, 0xFFE28A, 0x8ECBFF}, TAIL[2] = {0x8ECBFF, 0xFFFFFF};
  for (int e = 0; e < S.ev_n; e++) {
    const SnakeEvt &E = S.ev[e];
    const uint32_t eaten[3] = {sp_food_col, gm_rgb_mix(sp_food_col, 0xFFFFFF, 0.5f), 0xFFFFFF};
    if (E.t == SN_EAT) gm_burst(sp_food_x(), sp_food_y(), 34, 150, 1, eaten, 3);  // (where the eaten star is)
    else
      for (int i = 0; i < S.shed_n; i += 2) {
        float x, y;
        sp_cxy(S.shed[i], x, y), gm_burst(x, y, 4, 110, 1, SPARK, 3);
      }
  }
  S.ev_n = 0;
  if (S.food_n != sp_food_n) sp_food_orbit(S);  // a new star: its orbit at once (not at the next step)
  if (sp_col_n != S.food_n) {  // … and a colour of its own (not the last one's)
    uint32_t c = sp_food_col;
    while (c == sp_food_col) sp_food_rnd ^= sp_food_rnd << 13, sp_food_rnd ^= sp_food_rnd >> 17, sp_food_rnd ^= sp_food_rnd << 5, c = SP_FOOD[sp_food_rnd % 7];
    sp_food_col = c, sp_col_n = S.food_n;
  }
  // The comet's sparks: two a frame, thrown back from the head
  float ax, ay, bx, by, hx, hy, px, py;
  const float f = sn_frac(S);
  sn_cell(S, 1, f, bx, by), sn_cell(S, 0, f, ax, ay);
  sp_xy(ax, ay, hx, hy), sp_xy(bx, by, px, py);
  const float dx = hx - px, dy = hy - py, l = fmaxf(1, fabsf(dx) + fabsf(dy));
  for (int k = 0; k < 2; k++) gm_burst(hx, hy, 1, 25, 0.7f, TAIL, 2, 1.4f, 0, 1.5f);
  for (int k = 0; k < 2 && gm_np >= 2; k++) gm_parts[gm_np - 1 - k].vx -= dx / l * 40, gm_parts[gm_np - 1 - k].vy -= dy / l * 40;
  gm_parts_tick(dt, false);
}

// The body this frame: orbs at the cells (interpolated) and, between two orbs far apart, more along the arc (in cells:
// round the orbit, not across); glow — the mask's scale, core — the bright dot's radius, a — brightness
struct SpOrb {
  float x, y, glow, core, a;
};
#define SP_ORBS (SN_MAXL * 4)
static SpOrb *sp_orbs;
static int16_t sp_piece[SN_MAXL + 1];  // orb i's first entry in sp_orbs
static int sp_norbs;
static void sp_body(const Snake &S) {
  if (!sp_orbs) sp_orbs = (SpOrb *)heap_caps_malloc(SP_ORBS * sizeof(SpOrb), MALLOC_CAP_SPIRAM);
  if (!sp_orbs) return;
  const float f = sn_frac(S);
  int m = 0;
  float ca, cr, na, nr;
  sn_cell(S, 0, f, ca, cr);
  for (int i = 0; i < S.n; i++) {
    sp_piece[i] = m;
    const float k = (float)i / S.n;
    float x, y;
    sp_xy(ca, cr, x, y);
    if (m < SP_ORBS) sp_orbs[m++] = i ? SpOrb{x, y, 0.55f - 0.3f * k, 3.2f - 1.6f * k, 0.85f - 0.5f * k} : SpOrb{x, y, 1, 5, 1};
    if (i + 1 >= S.n) break;
    sn_cell(S, i + 1, f, na, nr);
    float da = na - ca;
    if (da > SP_A / 2) da -= SP_A;
    if (da < -SP_A / 2) da += SP_A;
    float nx, ny;
    sp_xy(na, nr, nx, ny);
    const int sub = (int)((fabsf(nx - x) + fabsf(ny - y)) * 0.75f / 13);  // (one more about every 13 px)
    for (int j = 1; j <= sub && m < SP_ORBS; j++) {  // (in between: a little dimmer)
      const float t = (float)j / (sub + 1), kk = (i + t) / S.n;
      float sx, sy;
      sp_xy(ca + da * t, cr + (nr - cr) * t, sx, sy);
      sp_orbs[m++] = {sx, sy, 0, 2.6f - 1.3f * kk, (0.85f - 0.5f * kk) * 0.8f};  // (glow 0: a core only)
    }
    ca = na, cr = nr;
  }
  sp_piece[S.n] = m, sp_norbs = m;
}

static void space_layout() {
  if (!sp_s) return;
  const Snake &S = *sp_s;
  const bool rest = gm_rest();
  for (int i = 0; i < SP_STARS; i++) {  // a star is redrawn only when its pixel changes
    SpStar &s = sp_star[i];
    const int ix = (int)(CX + s.r * cosf(s.a)), iy = (int)(CX + s.r * sinf(s.a));
    if (ix == s.dx && iy == s.dy) continue;
    if (rest) gm_damage(s.dx - 1, s.dy - 1, s.dx + 2, s.dy + 2), gm_damage(ix - 1, iy - 1, ix + 2, iy + 2);
    s.dx = ix, s.dy = iy;
  }
#ifndef HALO_ROUND
  {  // the stars by rows of the world, for the draw (a counting sort)
    int16_t count[SP_GB + 1] = {0};
    auto bucket = [](int y) { return constrain(y / 16, 0, SP_GB); };
    for (int i = 0; i < SP_STARS; i++) count[bucket(sp_star[i].dy)]++;
    int16_t at = 0;
    for (int b = 0; b <= SP_GB; b++) sp_star_at[b] = at, at += count[b];
    sp_star_at[SP_GB + 1] = at;
    int16_t next[SP_GB + 1];
    memcpy(next, sp_star_at, sizeof next);
    for (int i = 0; i < SP_STARS; i++) sp_star_of[next[bucket(sp_star[i].dy)]++] = i;
  }
#endif
  gm_mark(sp_hole_box, CX - SP_H(64), CX - SP_H(36), CX + SP_H(64), CX + SP_H(36), (uint32_t)(sp_t * 60) + 1);  // (it turns: every frame)
  const float fx = sp_food_x(), fy = sp_food_y();
  gm_mark(sp_food_box, fx - 18, fy - 18, fx + 18, fy + 18, gm_mix(gm_mix((int32_t)(fx * 4) << 12 ^ (int32_t)(fy * 4), (int32_t)(sp_t * 60)), sp_food_col));
  sp_body(S);
  for (int i = 0; i < SN_MAXL; i++) {  // a piece: orb i and the ones toward orb i + 1
    if (i >= S.n || !sp_orbs) {
      if (sp_orb[i].h) gm_mark(sp_orb[i], 1, 1, 0, 0, 0);
      continue;
    }
    float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    uint32_t h = 2166136261u;
    for (int j = sp_piece[i]; j < sp_piece[i + 1]; j++) {
      const SpOrb &o = sp_orbs[j];
      const float r = fmaxf(SP_GLOW * o.glow / 2, o.core) + 1;
      x0 = fminf(x0, o.x - r), y0 = fminf(y0, o.y - r), x1 = fmaxf(x1, o.x + r), y1 = fmaxf(y1, o.y + r);
      h = gm_mix(gm_mix(h, (int32_t)(o.x * 4) << 12 ^ (int32_t)(o.y * 4)), (int32_t)(o.a * 64));
    }
    gm_mark(sp_orb[i], x0, y0, x1, y1, h);
  }
  gm_parts_layout();
}

static void sp_glow_at(float x, float y, float scale, uint32_t rgb, float a) {  // (the mask, SP_GLOW × scale across)
  const float s = scale * gm_v.s, half = SP_GLOW * s / 2, X = gm_x(x), Y = gm_y(y);
  if (sp_glow.bits && halo_hits(X - half, Y - half, X + half, Y + half)) halo_mask(sp_glow, X - half, Y - half, gm_col(rgb, a), 1, s);
}
static void sp_disc(bool front) {  // the hole's disc: hot blobs on three ellipses, the back half or the front one
  static const float DR[3] = {22, 31, 40}, DW[3] = {1.3f, 0.95f, 0.7f}, DS[3] = {3.2f, 3.8f, 4.4f}, DA[3] = {0.8f, 0.6f, 0.45f};
#ifdef HALO_ROUND
  static const int DN[3] = {10, 14, 18};
#else
  static const int DN[3] = {22, 30, 40};  // (more and smaller: a bigger disc stays smooth)
#endif
  static const uint32_t DC[3] = {0xFFF0C8, 0xFFB347, 0xFF5A1F};
  for (int k = 2; k >= 0; k--)
    for (int i = 0; i < DN[k]; i++) {
      const float a = i * 2 * HALO_PI / DN[k] + sp_t * DW[k] + k, s = sinf(a);
      if ((s >= 0) != front) continue;
      halo_circle(gm_x(CX + SP_H(DR[k]) * cosf(a)), gm_y(CX + 0.38f * SP_H(DR[k]) * s), SP_S(DS[k]) * gm_v.s, gm_col(DC[k], DA[k]));
    }
}
static void space_draw() {
  if (!sp_s) return;
  static const uint32_t STAR[4] = {0, 0x5A5A5A, 0x9A9A9A, 0xE0E0E0};
#ifdef HALO_ROUND
  if (sp_odot) {  // the orbits the snake runs on: faint dots — at rest only this band's, else all through the view
    const uint32_t oc = gm_col(0x26355C);
    const bool rest = SP_BAND_REST;
    const int b0 = rest ? max(0, hb.y / HALO_ROWS) : 0, b1 = rest ? min(HALO_BANDS, (hb.y + hb.rows - 1) / HALO_ROWS + 1) : HALO_BANDS;
    for (int i = sp_band[b0]; i < sp_band[b1]; i++) {
      const int x = (int)gm_x(sp_odot[i][0]), y = (int)gm_y(sp_odot[i][1]);
      if (y >= hb.y && y < hb.y + hb.rows && x >= hb.x0 && x <= hb.x1) halo_rect(x, y, x, y, oc);
    }
  }
  for (int i = 0; i < SP_STARS; i++) {
    const SpStar &s = sp_star[i];
    const float x = gm_x(s.dx), y = gm_y(s.dy), w = (s.l == 3 ? 2 : 1) * gm_v.s;
    if (halo_hits(x, y, x + w, y + w)) halo_rect((int)x, (int)y, (int)(x + w - 0.01f), (int)(y + w - 0.01f), gm_col(STAR[s.l]));
  }
#else
  // Only the rows of the world this band shows (the view maps rows one to one: a shift and a scale)
  const float wy0 = gm_y(0), wk = 1 / (gm_v.s * 16);
  const int g0 = constrain((int)((hb.y - wy0) * wk) - 1, 0, SP_GB + 1), g1 = constrain((int)((hb.y + hb.rows - wy0) * wk) + 2, 0, SP_GB + 1);
  if (sp_odot) {
    const uint32_t oc = gm_col(0x26355C);
    for (int i = sp_gband[g0]; i < sp_gband[g1]; i++) {
      const int x = (int)gm_x(sp_odot[i][0]), y = (int)gm_y(sp_odot[i][1]);
      if (y >= hb.y && y < hb.y + hb.rows && x >= hb.x0 && x <= hb.x1) halo_rect(x, y, x, y, oc);
    }
  }
  for (int k = sp_star_at[g0]; k < sp_star_at[g1]; k++) {
    const SpStar &s = sp_star[sp_star_of[k]];
    const float x = gm_x(s.dx), y = gm_y(s.dy), w = (s.l == 3 ? 2 : 1) * gm_v.s;
    if (halo_hits(x, y, x + w, y + w)) halo_rect((int)x, (int)y, (int)(x + w - 0.01f), (int)(y + w - 0.01f), gm_col(STAR[s.l]));
  }
#endif
  // The black hole
  if (halo_hits(gm_x(CX - SP_H(64)), gm_y(CX - SP_H(36)), gm_x(CX + SP_H(64)), gm_y(CX + SP_H(36)))) {
    sp_disc(false);
    halo_circle(gm_x(CX), gm_y(CX), SP_H(13) * gm_v.s, gm_col(0x000000));
    halo_ring(gm_x(CX), gm_y(CX), SP_H(14.5f) * gm_v.s, SP_H(1.5f) * gm_v.s, 1, gm_col(0xFFDCAA));
    sp_disc(true);
    for (int k = 0; k < 14; k++) {
      const float ph = fmodf(sp_t * 0.4f + k / 14.0f, 1), a = k * 2.4f + ph * 9, r = SP_H(60) * (1 - ph);
      const float x = gm_x(CX + cosf(a) * r), y = gm_y(CX + sinf(a) * r * 0.5f);
      halo_rect((int)x, (int)y, (int)x + 1, (int)y + 1, gm_col(0xFFC88C, ph));
    }
  }
  // The food: a twinkling star of its colour — a wide glow, a bright one, rays and a white core
  const float fx = sp_food_x(), fy = sp_food_y(), tw = 0.6f + 0.4f * sinf(sp_t * 7);
  const uint32_t fc = sp_food_col, light = gm_rgb_mix(fc, 0xFFFFFF, 0.25f);
  sp_glow_at(fx, fy, 0.8f, fc, 0.7f + 0.25f * tw), sp_glow_at(fx, fy, 0.34f, gm_rgb_mix(fc, 0xFFFFFF, 0.45f), 0.95f);
  const float L = 13 * tw, l2 = 6 * tw;
  halo_rrect(gm_x(fx - L), gm_y(fy - 1), gm_x(fx + L), gm_y(fy + 1), 1 * gm_v.s, gm_col(light));
  halo_rrect(gm_x(fx - 1), gm_y(fy - L), gm_x(fx + 1), gm_y(fy + L), 1 * gm_v.s, gm_col(light));
  for (int k = -1; k <= 1; k += 2)  // the short diagonal rays: little dots along them
    for (int j = 1; j <= 3; j++) {
      const float d = l2 * j / 3 * 0.7f;
      halo_circle(gm_x(fx + k * d), gm_y(fy + d), (1.3f - 0.3f * j) * gm_v.s, gm_col(light, 0.8f));
      halo_circle(gm_x(fx + k * d), gm_y(fy - d), (1.3f - 0.3f * j) * gm_v.s, gm_col(light, 0.8f));
    }
  halo_circle(gm_x(fx), gm_y(fy), 2.4f * gm_v.s, gm_col(0xFFFFFF));
  // The comet: orbs from the tail, the head last
  for (int j = sp_norbs - 1; j >= 0 && sp_orbs; j--) {
    const SpOrb &o = sp_orbs[j];
    if (j) {
      if (o.glow > 0) sp_glow_at(o.x, o.y, o.glow, 0x8ECBFF, o.a);
      halo_circle(gm_x(o.x), gm_y(o.y), o.core * gm_v.s, gm_col(0xCFE6FF, o.a + 0.05f));
    } else {
      sp_glow_at(o.x, o.y, 1, 0xA0D2FF, 0.9f);
      halo_circle(gm_x(o.x), gm_y(o.y), 5 * gm_v.s, gm_col(0xFFFFFF));
    }
  }
  gm_parts_draw(false);
}
