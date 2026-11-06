// stars.h — a starry background (PopkaStudio's layer Stars, face.h: FL_STARS), projects/Orbit's: stars on black all
// over the circle, most of them dim, each twinkling at its own pace, and now and then a meteor flying through.
// Orbit draws the whole screen every frame; Halo redraws only what changed — here a star is a tiny circle whose
// brightness (in 1/16 steps) changes at most st_rate times a second, so the screen is redrawn that often (a meteor —
// every frame while it flies, 0.7 s). What it costs — PopkaStudio/README.md.
// The layer (FaceLayer): p — how many stars (up to ST_MAX), k — their brightness (1 — Orbit's), p2 — twinkling (1 —
// Orbit's pace, 0 — still), p3 — meteors a minute (0 — none); color — the stars'. The same stars every start (a seed
// per layer): the editor shows the board's sky. In Always On and on the way there — still, no meteors (fc_calm).
// Only in a firmware with a Stars layer (FN_STARS — the editor writes it into layers.h); the editor's page has it
// always. Command: stars rate N (updates a second, to measure; 20 by default).
// Included from face.h after wxscreen.h.
#if HAS_STARS
#define ST_MAX 200
#define ST_DOTS 12     // a meteor: its head and tail
#define ST_SKY_R 231   // the stars fill the screen's circle (233) and stay inside it: a star is 1.3 px
struct StarRun {
  int n;
  float x[ST_MAX], y[ST_MAX], bright[ST_MAX], phase[ST_MAX], speed[ST_MAX];
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
    const float r = ST_SKY_R * halo_sqrt(st_rand(st->seed)), a = st_rand(st->seed) * 2 * HALO_PI, b = st_rand(st->seed);
    st->x[k] = CX + r * cosf(a), st->y[k] = CX + r * sinf(a), st->bright[k] = 0.3f + 0.7f * b * b;
    st->phase[k] = st_rand(st->seed) * 2 * HALO_PI, st->speed[k] = 0.8f + 2.2f * st_rand(st->seed);
  }
  st->tick = halo_ms(), st->next = halo_ms() / 1000.0f + 4;
  R.g = new HaloItem[st->n + ST_DOTS]();
  R.ax = CX, R.ay = CX;
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
    it.kind = HI_CIRCLE, it.x = st->x[k], it.y = st->y[k], it.r = 1.3f, it.scale = 1, it.color = L.color;
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
      const float r = 40 + 120 * st_rand(st->seed), an = st_rand(st->seed) * 2 * HALO_PI;
      const float dir = (200 + 90 * st_rand(st->seed)) * HALO_PI / 180;
      st->mx = CX + r * cosf(an), st->my = CX + r * sinf(an) - 60, st->mvx = 650 * cosf(dir), st->mvy = -650 * sinf(dir);
      st->mage = 0, st->next = now + 60 / L.p3 * (0.5f + st_rand(st->seed));
    }
    static uint32_t last;
    st->mage += st->mage < 9 ? (ms - last) / 1000.0f : 0;
    last = ms;
  }
  const float ma = st->mage < st->mlife ? sinf(st->mage / st->mlife * HALO_PI) : 0;
  const float hx = st->mx + st->mvx * st->mage, hy = st->my + st->mvy * st->mage, len = fminf(1, st->mage * 4) * 0.13f;
  const float edge2 = (CX - 2.0f) * (CX - 2.0f);
  bool seen = false;
  for (int d = 0; d < ST_DOTS; d++) {
    HaloItem &it = R.g[st->n + d];
    const float f = d / (float)ST_DOTS;
    it.kind = HI_CIRCLE, it.x = hx - st->mvx * len * f, it.y = hy - st->mvy * len * f, it.r = 1.6f, it.scale = 1;
    it.color = L.color, it.on_black = true;
    const float dx = it.x - CX, dy = it.y - CX;
    const bool in = dx * dx + dy * dy < edge2;
    seen |= in;
    it.alpha = ma > 0 && in ? halo_clamp01(ma * (1 - f) * (1 - f) * 1.4f * L.k) * a : 0;
    halo_view_commit(it, v);
  }
  if (!seen && st->mage < st->mlife) st->mage = 9;
  // Where it is: the whole screen (the editor selects it there, under the other layers, with a round frame)
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = 2 * CX;
  R.ax = CX, R.ay = CX;
}

static void stars_layer_free(FaceRun &R) {
  delete R.stars;
  R.stars = nullptr;
}

static void stars_draw(const FaceScreen &S, int i) {
  const FaceRun &R = S.run[i];
  if (!R.stars || !R.g) return;
  for (int k = 0; k < R.stars->n + ST_DOTS; k++) halo_item_draw(R.g[k]);
}

static void stars_commands() {
  halo_command("stars", [](const char *a) {
    if (!strncmp(a, "rate", 4)) st_rate = constrain(atoi(a + 4), 1, 60), LOG("stars: %d updates a second\n", st_rate);
  });
}
#endif
