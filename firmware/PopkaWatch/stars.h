// stars.h — a starry background (PopkaStudio's layer Stars, face.h: FL_STARS), projects/Orbit's: stars on black all
// over the circle, most of them dim, each twinkling at its own pace, and now and then a meteor flying through.
// Orbit draws the whole screen every frame; Halo redraws only what changed — here a star is a tiny circle whose
// brightness (in 1/16 steps) changes at most st_rate times a second, so the screen is redrawn that often (a meteor —
// every frame while it flies, 0.7 s). What it costs — docs/devlog.md.
// The layer (FaceLayer): p — how many stars (up to ST_MAX), k — their brightness (1 — Orbit's), p2 — twinkling (1 —
// Orbit's pace, 0 — still), p3 — meteors a minute (0 — none); color — the stars'. The same stars every start (a seed
// per layer): the editor shows the board's sky. In Always On and on the way there — still, no meteors (fc_calm).
// Only in a firmware with a Stars layer (FN_STARS — the editor writes it into layers.h); the editor's page has it
// always. Command: stars rate N (updates a second, to measure; 20 by default).
// Included from face.h after wxscreen.h.
#if HAS_STARS
#include <utility>
#define ST_MAX 200
#ifdef HALO_ROUND
#define ST_DOTS 12     // a meteor: its head and tail
#else
#define ST_DOTS 40     // (a longer tail on a wider screen, its dots touching)
#endif
#define ST_SKY_R 231   // the stars fill the screen's circle (233) and stay inside it: a star is 1.3 px
#ifdef HALO_ROUND
#define ST_TAIL 0.13f  // s: the way behind the meteor its tail covers
#else
#define ST_TAIL 0.2f
#endif
struct StarRun {
  int n;
  float x[ST_MAX], y[ST_MAX], bright[ST_MAX], phase[ST_MAX], speed[ST_MAX];
  uint8_t no[ST_MAX];  // the star's number before sorting (its size on a rectangle goes by it)
  float mx, my, mvx, mvy, mage = 9, mlife = 0.7f, next;  // the meteor: where it started, its speed, its age
  uint32_t tick, seed;
};
static volatile int st_rate = 20;

static float st_rand(uint32_t &s) {  // 0..1, the same sequence every start
  s = s * 1664525u + 1013904223u;
  return (s >> 8) * (1.0f / 16777216);
}

static void stars_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  StarRun *st = R.stars = new StarRun();
  st->n = constrain((int)L.p, 0, ST_MAX);
  st->seed = 0x5EED0000u + i * 977u;
  for (int k = 0; k < st->n; k++) {  // evenly over the circle, most of them dim (Orbit's)
#ifdef HALO_ROUND
    const float r = ST_SKY_R * halo_sqrt(st_rand(st->seed)), a = st_rand(st->seed) * 2 * HALO_PI, b = st_rand(st->seed);
    st->x[k] = CX + r * cosf(a), st->y[k] = CX + r * sinf(a), st->bright[k] = 0.3f + 0.7f * b * b;
#else
    const float px = st_rand(st->seed), py = st_rand(st->seed), b = st_rand(st->seed);  // (over the whole rectangle)
    st->x[k] = 3 + px * (W - 6), st->y[k] = 3 + py * (HALO_H - 6), st->bright[k] = 0.3f + 0.7f * b * b;
#endif
    st->phase[k] = st_rand(st->seed) * 2 * HALO_PI, st->speed[k] = 0.8f + 2.2f * st_rand(st->seed);
    st->no[k] = (uint8_t)k;
  }
  // Top to bottom (the same stars, in another order): a band draws only the stars near its rows (stars_draw) — a view
  // (a transition's shift and scale) keeps the order
  for (int k = 1; k < st->n; k++)
    for (int j = k; j > 0 && st->y[j] < st->y[j - 1]; j--) {
      std::swap(st->x[j], st->x[j - 1]), std::swap(st->y[j], st->y[j - 1]), std::swap(st->bright[j], st->bright[j - 1]);
      std::swap(st->phase[j], st->phase[j - 1]), std::swap(st->speed[j], st->speed[j - 1]);
      std::swap(st->no[j], st->no[j - 1]);
    }
  st->tick = halo_ms(), st->next = halo_ms() / 1000.0f + 4;
  R.g = new HaloItem[st->n + ST_DOTS]();
  R.ax = CX, R.ay = CY;
}

static void stars_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  StarRun *st = R.stars;
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  if (!st || !R.g) return;
  // Twinkling: the phases go on st_rate times a second (still in Always On and on the way there: fc_calm — from where
  // they stopped, without a jump); a brightness only in 1/16 steps
  const uint32_t ms = halo_ms();
  if (fc_calm) st->tick = ms;
  if (!fc_calm && ms - st->tick >= 1000u / (uint32_t)max(1, (int)st_rate)) {
    const float dt = (ms - st->tick) / 1000.0f;
    st->tick = ms;
    for (int k = 0; k < st->n; k++) {
      st->phase[k] += st->speed[k] * L.p2 * dt;
      if (st->phase[k] > 2 * HALO_PI) st->phase[k] -= 2 * HALO_PI;
    }
  }
  for (int k = 0; k < st->n; k++) {
    HaloItem &it = R.g[k];
    const float b = halo_clamp01(L.k * st->bright[k] * (0.55f + 0.45f * sinf(st->phase[k])));
#ifdef HALO_ROUND
    it.kind = HI_CIRCLE, it.x = st->x[k], it.y = st->y[k], it.r = 1.3f, it.scale = 1, it.color = L.color;
#else
    const int no = st->no[k];
    it.kind = HI_CIRCLE, it.x = st->x[k], it.y = st->y[k], it.r = no % 14 == 0 ? 2.3f : no % 3 == 0 ? 1.6f : 1.15f, it.scale = 1, it.color = L.color;
#endif
    it.alpha = (int)(b * 16 + 0.5f) * 0.0625f * a, it.on_black = true;
    halo_view_commit(it, v);
  }
  // A meteor now and then (Orbit's: from somewhere inside, mostly down and left, 650 px/s, 0.7 s; its tail — the way
  // of 0.13 s behind it, fading). A dot past the screen's edge is not drawn, and once all of it is past — it is over (a
  // straight way out of a circle never comes back): no frames for what cannot be seen
  const float now = ms / 1000.0f;
  if (fc_calm || L.p3 <= 0) {
    st->mage = 9;
  } else {
    if (now >= st->next) {
#ifdef HALO_ROUND
      const float r = 40 + 120 * st_rand(st->seed), an = st_rand(st->seed) * 2 * HALO_PI;
      const float dir = (200 + 90 * st_rand(st->seed)) * HALO_PI / 180;
      st->mx = CX + r * cosf(an), st->my = CX + r * sinf(an) - 60, st->mvx = 650 * cosf(dir), st->mvy = -650 * sinf(dir);
#else
      const float dir = (200 + 60 * st_rand(st->seed)) * HALO_PI / 180;  // (from the upper right, across the screen)
      st->mx = W * (0.35f + 0.6f * st_rand(st->seed)), st->my = HALO_H * 0.5f * st_rand(st->seed), st->mvx = 650 * cosf(dir), st->mvy = -650 * sinf(dir);
#endif
      st->mage = 0, st->next = now + 60 / L.p3 * (0.5f + st_rand(st->seed));
    }
    static uint32_t last;
    st->mage += st->mage < 9 ? (ms - last) / 1000.0f : 0;
    last = ms;
  }
  const float ma = st->mage < st->mlife ? sinf(st->mage / st->mlife * HALO_PI) : 0;
  const float hx = st->mx + st->mvx * st->mage, hy = st->my + st->mvy * st->mage, len = fminf(1, st->mage * 4) * ST_TAIL;
#ifdef HALO_ROUND
  const float edge2 = (CX - 2.0f) * (CX - 2.0f);
#endif
  bool seen = false;
  for (int d = 0; d < ST_DOTS; d++) {
    HaloItem &it = R.g[st->n + d];
    const float f = d / (float)ST_DOTS;
    it.kind = HI_CIRCLE, it.x = hx - st->mvx * len * f, it.y = hy - st->mvy * len * f, it.r = 1.6f, it.scale = 1;
    it.color = L.color, it.on_black = true;
#ifdef HALO_ROUND
    const float dx = it.x - CX, dy = it.y - CX;
    const bool in = dx * dx + dy * dy < edge2;
#else
    const bool in = it.x >= 2 && it.x < W - 2 && it.y >= 2 && it.y < HALO_H - 2;
#endif
    seen |= in;
    it.alpha = ma > 0 && in ? halo_clamp01(ma * (1 - f) * (1 - f) * 1.4f * L.k) * a : 0;
    halo_view_commit(it, v);
  }
  if (!seen && st->mage < st->mlife) st->mage = 9;
  // Where it is: the whole screen (the editor selects it there, under the other layers, with a round frame)
#ifdef HALO_ROUND
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = 2 * CX;
  R.ax = CX, R.ay = CX;
#else
  R.box[0] = R.box[1] = 0, R.box[2] = W, R.box[3] = HALO_H;
  R.ax = CX, R.ay = CY;
#endif
}

static void stars_layer_free(FaceRun &R) {
  delete R.stars;
  R.stars = nullptr;
}

static void stars_draw(const FaceScreen &S, int i) {
  const FaceRun &R = S.run[i];
  if (!R.stars || !R.g) return;
  // Only the stars near this band's rows (sorted by y): 138 checks a band were ~0.7 ms of a core's frame in a transition,
  // when the whole sky is drawn
  const int n = R.stars->n;
  const float m = 3 * fmaxf(1, R.g[0].scale) + 2, top = hb.y - m, bottom = hb.y + hb.rows + m;
  int lo = 0, hi = n;
  while (lo < hi) {
    const int mid = (lo + hi) / 2;
    if (R.g[mid].y < top) lo = mid + 1;
    else hi = mid;
  }
  for (int k = lo; k < n && R.g[k].y <= bottom; k++) halo_item_draw(R.g[k]);
  for (int k = n; k < n + ST_DOTS; k++) halo_item_draw(R.g[k]);  // the meteor
}

static void stars_commands() {
  halo_command("stars", [](const char *a) {
    if (!strncmp(a, "rate", 4)) st_rate = constrain(atoi(a + 4), 1, 60), LOG("stars: %d updates a second\n", st_rate);
  });
}
#endif
