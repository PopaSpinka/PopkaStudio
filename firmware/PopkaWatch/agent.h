// agent.h — the voice agent's moods (Layers → Agent): PopkaStudio's own, from the pages "Лица голосового агента" and
// "Эмоции агента" (25 Nov 2026: the user took the dot face and the dot orb, all twelve moods of each). One layer kind,
// FL_AGENT, round on every board — on a rectangle, a circle in the middle:
//   Dot face (AG_FACE)  a board of 29 × 29 dots inside a circle, as Dot Snake (game_dots.h): eyes, brows, a mouth and
//                       small signs (z, ?, hearts, a tear, a check mark) lit dot by dot. Every dot keeps its heat and fades
//                       as the snake's trail (the signs drawn "crisp" — without it). Five colors: the layer's, and red,
//                       blue, pink, green for anger, tears, love and "done".
//   Dot orb (AG_ORB)    340 dots on a sphere (a Fibonacci lattice) tilted 20°, as Thinking Orbs: the far ones dim and
//                       small; a mood is how the dots light up, grow and move (rings, a beam, spikes, a check mark).
// The layer (FaceLayer): dx, dy — its middle from the screen's; k — its size (1 — the round screen; a rectangle's shorter
// side × 0.92); color — the dots; p2 — the look; p3 — the mood (0..11; 12 — all of them in turn, AG_CYCLE_S each).
// On the board a tap on it (a layer without a link) shows the next mood; the command "agent" sets every agent's mood:
//   agent next | agent all | agent <0..11> | agent <name> (a name of the layer's look: idle, happy, listen, ...)
// Only what changed is redrawn (Halo: a frame costs what changed): each dot's look is kept and compared — a face's
// dot: its color, size and brightness; an orb's dot: its place to half a pixel, size, brightness and color. A new view
// (a transition, a move, another size or color) — the whole circle. What it keeps (the heat, the dots, the masks)
// lives in PSRAM, an AgRun a layer. In Always On it stands still (fc_calm).
// Only the looks a project has are in its firmware (FN_AGENT in config.h: bit 0 — the face, bit 1 — the orb); the
// editor's page has both. Included from face.h after sclock.h.

#define AG_FACE 0
#define AG_ORB 1
#ifdef __EMSCRIPTEN__
#define AG_ON(n) 1
#else
#define AG_ON(n) ((FN_AGENT >> (n)) & 1)
#endif
#define AG_MOODS 12
#define AG_CYCLE_S 6.0f
#define AG_WRAP 600.0f  // s: a mood's time goes round (sinf of a big angle is slow)
#define AG_G 29         // the face's board: dots across
#define AG_N (AG_G * AG_G)
#define AG_NC 5         // colors: the layer's, red, blue, pink, green
#define AG_ORB_N 340
#define AG_EXTRA 64     // the orb's dots besides the sphere: the beam, confetti, the z's
#define AG_DOTS (AG_ORB_N + AG_EXTRA)
#define AG_DISC0 3      // the orb's dot masks: AG_DISC0 .. AG_DISC0 + AG_DISCS - 1 px across
#define AG_DISCS 22
#define AG_BH HALO_ROWS  // the orb's dots are sorted into rows of the screen as high as a band (a band draws only its own)
#define AG_BK ((HALO_H + AG_BH - 1) / AG_BH)
#ifdef HALO_ROUND
#define AG_D 466.0f     // k = 1: the screen
#else
#define AG_D ((HALO_W < HALO_H ? HALO_W : HALO_H) * 0.92f)
#endif

static const uint32_t AG_COL[AG_NC] = {0xFFA630, 0xFF4D4D, 0x6FA8FF, 0xFF6FB1, 0x5FE08F};  // (0 — the layer's color)
static const char *const AG_NAMES[2][AG_MOODS] = {
    {"idle", "happy", "laugh", "angry", "sad", "surprised", "love", "confused", "wink", "bored", "scared", "done"},
    {"idle", "listen", "think", "search", "speak", "happy", "angry", "sad", "surprised", "done", "error", "sleep"}};

struct AgDot {  // an orb's dot as drawn: its middle × 2 (half a pixel), mask, brightness 0..15, color
  int16_t x2, y2;
  uint8_t m, a, c, pad;
};
struct AgRun {
  uint8_t look, mood;
  bool cycle;
  float t, ct;          // the mood's own time; the time in it while all go in turn
  uint32_t ms, seq;     // the last layout; the last command seen
  HaloView v;           // this frame's view (its draw uses it)
  bool over;            // something under it: blended (not "on black")
  float cx, cy, D;      // its middle and size, px (before the view)
  uint32_t col0;        // the layer's color
  uint32_t vh;          // the view, place, size, color of the last frame (another — the whole circle again)
  float box[4];         // the circle last frame, view applied (x1 < x0 — none)
  // the face
  float *tg, *ht, *cr;  // AG_NC × AG_N: this frame's, the heat (fading), crisp (no trail)
  uint8_t *big, *look_now, *drawn;
  uint8_t used, inked, hot;  // colors lit this frame, lit last frame (theirs to clear), still warm (a bit a color)
  HaloMask dot[2];
  // the orb
  AgDot *now, *was;
  float *z;
  uint16_t *ord;  // back to front
  // the dots by rows of the screen, back to front: each one's top-left on screen (the view applied), its first row
  float *bx, *by;
  uint8_t *b_first, *b_m;      // (255 — not drawn); the mask it is drawn with — of its size on screen, at scale 1
  uint16_t *b_ofs, *b_idx;     // row r: b_idx[b_ofs[r] .. b_ofs[r + 1])
  int n, n_was;
  HaloMask disc[AG_DISCS];
};

static uint8_t ag_valid[AG_N];
static float *ag_pts, *ag_rnd;  // the orb's dots on the unit sphere (x, y, z), a random number each
static volatile uint32_t ag_cmd_seq;
static volatile int ag_cmd[2] = {-1, -1};  // the command's mood for each look (-1 — not for it; 12 — all; 13 — next)

static inline float ag_c01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
static inline int ag_round(float v) { return halo_floor(v + 0.5f); }
static inline int ag_ceil(float v) { const int i = halo_floor(v); return i + (v > i); }
static inline float ag_lerp(float a, float b, float t) { return a + (b - a) * t; }
static inline float ag_mod(float t, float p) { return t - p * halo_floor(t * (1 / p)); }
// sinf, cosf of an angle brought into 0..2π first: past ~200 rad newlib's sinf takes its slow path (thousands of cycles)
static inline float ag_sin(float x) { return sinf(ag_mod(x, 2 * HALO_PI)); }
static inline float ag_cos(float x) { return cosf(ag_mod(x, 2 * HALO_PI)); }
static inline float ag_sq(float x) { return x * x; }
static inline float ag_p8(float x) { x *= x, x *= x; return x * x; }
static inline float ag_bump(float t, float a, float b) { return t <= a || t >= b ? 0 : sinf((t - a) / (b - a) * HALO_PI); }
static inline float ag_outback(float t) { const float u = t - 1; return 1 + 2.9f * u * u * u + 1.9f * u * u; }
static inline float ag_inout(float t) { return t < 0.5f ? 2 * t * t : 1 - 2 * (1 - t) * (1 - t); }
static inline float ag_cubic(float t) { const float u = 1 - t; return 1 - u * u * u; }
static inline float ag_rsqrt(float x) {  // 1/√x (halo_sqrt's trick)
  union {
    float f;
    uint32_t i;
  } u = {x};
  u.i = 0x5f3759df - (u.i >> 1);
  float y = u.f;
  y = y * (1.5f - 0.5f * x * y * y);
  return y * (1.5f - 0.5f * x * y * y);
}
static inline float ag_hash(uint32_t n) {
  n ^= n >> 16, n *= 0x7feb352du, n ^= n >> 15, n *= 0x846ca68bu, n ^= n >> 16;
  return (n >> 8) * (1.0f / 16777216);
}
static float ag_voice(float t) {  // a made-up voice: syllables in phrases, 0..1
  const float ph = ag_c01(0.4f + 1.6f * ag_sin(t * 0.85f + 0.7f * ag_sin(t * 0.33f)));
  return ph * (0.25f + 0.75f * powf(fabsf(ag_sin(t * 6.1f)), 0.7f) * (0.55f + 0.45f * ag_sin(t * 2.3f + 1)));
}
static float ag_speak(float t) {
  return ag_c01(fabsf(ag_sin(t * 7.3f)) * (0.6f + 0.45f * ag_sin(t * 2.1f + 0.5f)) + 0.05f) * (ag_sin(t * 0.9f + 1) > -0.8f ? 1 : 0.1f);
}

static void ag_init() {
  static bool done;
  if (done) return;
  done = true;
  for (int y = 0; y < AG_G; y++)
    for (int x = 0; x < AG_G; x++) {
      const float dx = x + 0.5f - AG_G / 2.0f, dy = y + 0.5f - AG_G / 2.0f, r = AG_G / 2.0f - 0.35f;
      ag_valid[y * AG_G + x] = dx * dx + dy * dy <= r * r;
    }
  ag_pts = (float *)heap_caps_calloc(AG_ORB_N * 4, sizeof(float), MALLOC_CAP_SPIRAM);
  if (!ag_pts) return;
  ag_rnd = ag_pts + AG_ORB_N * 3;
  for (int i = 0; i < AG_ORB_N; i++) {
    const float y = 1 - 2 * (i + 0.5f) / AG_ORB_N, r = sqrtf(1 - y * y), ph = ag_mod(i * 2.399963f, 2 * HALO_PI);
    ag_pts[3 * i] = cosf(ph) * r, ag_pts[3 * i + 1] = y, ag_pts[3 * i + 2] = sinf(ph) * r;
    ag_rnd[i] = ag_hash(i * 7 + 3);
  }
}

// A rounded square (r = size / 2: a disc), 4 bits a pixel, 4 × 4 samples a pixel (games.h gm_make_rrect)
static void ag_mask(HaloMask &m, float size, float r) {
  const int MW = (int)size + 3, bpr = (MW + 1) / 2;
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
      first = first < x ? first : x, last = last > x ? last : x;
    }
    ext[2 * y] = first > last ? 1 : first, ext[2 * y + 1] = first > last ? 0 : last;
  }
  m = {(int16_t)MW, (int16_t)MW, 0, 0, 0, bits, ext};
}
static void ag_mask_free(HaloMask &m) {
  free((void *)m.bits);
  m = {};
}

// ================= The face: shapes lit on the board (soft edges: a shape lights a dot by how much it covers it) =================
static AgRun *ag_r;         // the face being laid out
static float *ag_into;      // where shapes go now: ag_r->tg, or ag_r->cr (crisp)

static void ag_put(float fx, float fy, float v, int col = 0, bool big = false) {
  const int x = ag_round(fx), y = ag_round(fy);
  if (x < 0 || y < 0 || x >= AG_G || y >= AG_G || v <= 0) return;
  const int i = y * AG_G + x;
  if (!ag_valid[i]) return;
  float &a = ag_into[col * AG_N + i];
  if (v > a) a = v > 1 ? 1 : v;
  ag_r->used |= 1 << col;
  if (big) ag_r->big[i] = 1;
}
#define AG_CELLS(X0, Y0, X1, Y1)                                                                                 \
  for (int y = halo_floor(Y0) < 0 ? 0 : halo_floor(Y0), yb = ag_ceil(Y1) > AG_G - 1 ? AG_G - 1 : ag_ceil(Y1); y <= yb; y++) \
    for (int x = halo_floor(X0) < 0 ? 0 : halo_floor(X0), xb = ag_ceil(X1) > AG_G - 1 ? AG_G - 1 : ag_ceil(X1); x <= xb; x++)

static void ag_ellipse(float cx, float cy, float rx, float ry, float a = 1, int col = 0) {
  if (ry < 0.35f) ry = 0.35f;
  const float m = fminf(rx, ry), irx = 1 / rx, iry = 1 / ry;
  AG_CELLS(cx - rx - 1, cy - ry - 1, cx + rx + 1, cy + ry + 1) {
    const float u = (x - cx) * irx, w = (y - cy) * iry;
    ag_put(x, y, a * ag_c01((1 - halo_sqrt(u * u + w * w)) * m + 0.5f), col);
  }
}
static void ag_ring_e(float cx, float cy, float rx, float ry, float a = 1, int col = 0, float w = 0.9f) {
  const float irx = 1 / rx, iry = 1 / ry, m = fminf(rx, ry);
  AG_CELLS(cx - rx - 1.5f, cy - ry - 1.5f, cx + rx + 1.5f, cy + ry + 1.5f) {
    const float dx = x - cx, dy = y - cy, d = halo_sqrt(dx * dx + dy * dy), u = dx * irx, q = dy * iry, n2 = u * u + q * q;
    const float dist = n2 > 0 ? fabsf(d - d * ag_rsqrt(n2)) : m;
    ag_put(x, y, a * ag_c01(w / 2 + 0.5f - dist), col);
  }
}
// A ring of radius r (its middle line), w thick; half ≥ 0 — only the arc mid ± half (radians, y down)
static void ag_ring(float cx, float cy, float r, float w, float a = 1, int col = 0, float mid = 0, float half = -1) {
  const float e = r + w + 1, mx = cosf(mid), my = sinf(mid), ch = cosf(half), sh = fmaxf(0.2f, sinf(half));
  AG_CELLS(cx - e, cy - e, cx + e, cy + e) {
    const float dx = x - cx, dy = y - cy, d2 = dx * dx + dy * dy, d = halo_sqrt(d2);
    float v = ag_c01(w / 2 + 0.5f - fabsf(d - r));
    if (v <= 0) continue;
    if (half >= 0 && d2 > 0) {
      const float c = (dx * mx + dy * my) * ag_rsqrt(d2);
      if (c < ch) v *= ag_c01(0.5f - (ch - c) / sh * d);  // (the angle past the arc's end ≈ (cos half − cos) / sin half)
    }
    ag_put(x, y, a * v, col);
  }
}
static void ag_line(float x0, float y0, float x1, float y1, float w = 1, float a = 1, int col = 0) {
  const float vx = x1 - x0, vy = y1 - y0, L = vx * vx + vy * vy, iL = L > 0 ? 1 / L : 0;
  AG_CELLS(fminf(x0, x1) - w - 1, fminf(y0, y1) - w - 1, fmaxf(x0, x1) + w + 1, fmaxf(y0, y1) + w + 1) {
    const float t = ag_c01(((x - x0) * vx + (y - y0) * vy) * iL), ex = x - x0 - vx * t, ey = y - y0 - vy * t;
    ag_put(x, y, a * ag_c01(w / 2 + 0.5f - halo_sqrt(ex * ex + ey * ey)), col);
  }
}
static void ag_cut(float cx, float cy, float r, float k, int col = 0) {  // a dark spot (a pupil) in what is lit
  AG_CELLS(cx - r - 1, cy - r - 1, cx + r + 1, cy + r + 1) {
    const float dx = x - cx, dy = y - cy, v = ag_c01(r + 0.5f - halo_sqrt(dx * dx + dy * dy));
    if (v > 0) ag_r->tg[col * AG_N + y * AG_G + x] *= 1 - k * v;
  }
}
static void ag_bits(const char *const *rows, int nr, float fx, float fy, float a = 1, int col = 0) {
  const int x = ag_round(fx), y = ag_round(fy);
  for (int r = 0; r < nr; r++)
    for (int c = 0; rows[r][c]; c++)
      if (rows[r][c] == '1') ag_put(x + c, y + r, a, col);
}
#define AG_BITS(rows, ...) ag_bits(rows, sizeof(rows) / sizeof(rows[0]), __VA_ARGS__)
static void ag_crisp_bits(const char *const *rows, int nr, float x, float y, float a = 1, int col = 0) {
  ag_into = ag_r->cr, ag_bits(rows, nr, x, y, a, col), ag_into = ag_r->tg;
}
#define AG_CRISP(rows, ...) ag_crisp_bits(rows, sizeof(rows) / sizeof(rows[0]), __VA_ARGS__)

#define AG_EL 9.0f
#define AG_ER 19.0f
static void ag_eye(float cx, float cy, float open, float lx = 0, float ly = 0, float r = 3, int col = 0, float pr = 0.85f,
                   float pk = -1) {
  ag_ellipse(cx, cy, r, fmaxf(0.45f, r * open), 1, col);
  const float k = pk >= 0 ? pk : ag_c01((open - 0.6f) * 5);
  if (k > 0) ag_cut(cx + lx * r * 0.47f, cy + ly * r * 0.47f * fminf(1, open), pr, 0.92f * k, col);
}
static void ag_sparkle(float cx, float cy, float L, float a = 1, int col = 0) {
  ag_put(cx, cy, a, col);
  for (int k = 1, n = ag_ceil(L); k <= n; k++) {
    const float v = a * ag_c01(L - k + 1) * (1 - 0.25f * (k - 1));
    ag_put(cx + k, cy, v, col), ag_put(cx - k, cy, v, col), ag_put(cx, cy + k, v, col), ag_put(cx, cy - k, v, col);
  }
}
static const char *const AG_HAPPY[] = {"01110", "10001", "10001"};
static void ag_happy_eye(float cx, float cy, float a = 1, int col = 0) { AG_BITS(AG_HAPPY, cx - 2, cy - 1, a, col); }
static const char *const AG_HEART_B[] = {"0110110", "1111111", "1111111", "0111110", "0011100", "0001000"};
static const char *const AG_HEART_M[] = {"01010", "11111", "11111", "01110", "00100"};
static const char *const AG_HEART_S[] = {"101", "111", "010"};
static const char *const AG_QMARK[] = {"01110", "10001", "00001", "00110", "00100", "00000", "00100"};
static const char *const AG_DROP[] = {"010", "111", "111"};
static const char *const AG_ZB[] = {"1111", "0001", "0010", "0100", "1111"};
static const char *const AG_ZS[] = {"111", "001", "010", "111"};

// The face's moods: what is lit at time t (s since the mood began)
static void ag_face_mood(int mood, float t) {
  switch (mood) {
    default:
    case 0: {  // idle: blinks every 3.6 s, glances about; the mouth a dim line
      static const float LOOK[7][2] = {{0, 0}, {0.7f, -0.2f}, {0.7f, -0.2f}, {0, 0}, {-0.7f, 0.15f}, {0, -0.6f}, {0, 0}};
      const float open = 1 - ag_bump(ag_mod(t, 3.6f), 3.3f, 3.52f);
      const float *l = LOOK[halo_floor(t * (1 / 1.1f)) % 7];
      ag_eye(AG_EL, 13, open, l[0], l[1]), ag_eye(AG_ER, 13, open, l[0], l[1]);
      ag_line(12.5f, 21, 15.5f, 21, 1, 0.5f);
      break;
    }
    case 1: {  // happy: arched eyes hop, a wide smile, sparkles at the sides
      const float oy = -ag_round(fabsf(ag_sin(t * 4.2f)) * 1.3f);
      ag_happy_eye(AG_EL, 12 + oy), ag_happy_eye(AG_ER, 12 + oy);
      ag_ring(14, 15 + oy, 4, 0.9f, 1, 0, HALO_PI / 2, 0.33f * HALO_PI);
      static const float SP[4][3] = {{4, 8, 0}, {24, 7, 0.4f}, {4, 20, 0.75f}, {24, 21, 0.2f}};
      for (const auto &s : SP) {
        const float b = ag_bump(ag_mod(t * 0.9f + s[2], 1), 0, 0.35f);
        if (b > 0) ag_sparkle(s[0], s[1], 1.6f * b, b);
      }
      break;
    }
    case 2: {  // laugh: > <, a "D" mouth, the face shakes in bursts, tears fly
      const bool burst = ag_mod(t, 1.9f) < 1.35f;
      const float st = ag_sin(t * 17), oy = burst ? ag_round(st * 0.75f) : 0, op = burst ? fabsf(st) : 0.3f;
      ag_line(AG_EL - 2, 11 + oy, AG_EL + 1.5f, 13 + oy), ag_line(AG_EL + 1.5f, 13 + oy, AG_EL - 2, 15 + oy);
      ag_line(AG_ER + 2, 11 + oy, AG_ER - 1.5f, 13 + oy), ag_line(AG_ER - 1.5f, 13 + oy, AG_ER + 2, 15 + oy);
      const float my = 17 + oy, r = 3.2f + 0.8f * op;
      ag_line(14 - r, my, 14 + r, my), ag_ring(14, my, r, 0.9f, 1, 0, HALO_PI / 2, 0.5f * HALO_PI);
      ag_ellipse(14, my + r * 0.62f, r * 0.5f, r * 0.28f, 0.8f, 3);
      if (burst) {
        const float p = ag_mod(t * 2.2f, 1);
        for (int s = -1; s <= 1; s += 2) ag_put(14 + s * (8.5f + p * 4), 11 + oy - p * 3 + p * p * 6, 1 - p, 2);
      }
      break;
    }
    case 3: {  // angry: red, brows down to the middle, clenched teeth, it shakes, steam
      const float ox = ag_round(ag_sin(t * 37) * ag_sin(t * 23) * 1.3f);
      ag_eye(AG_EL + ox, 14.5f, 0.78f, 0.3f, 0, 3, 1, 0.85f, 1), ag_eye(AG_ER + ox, 14.5f, 0.78f, -0.3f, 0, 3, 1, 0.85f, 1);
      ag_line(AG_EL - 3 + ox, 9.8f, AG_EL + 2.6f + ox, 12, 1.4f, 1, 1), ag_line(AG_ER + 3 + ox, 9.8f, AG_ER - 2.6f + ox, 12, 1.4f, 1, 1);
      for (int x = 10; x <= 18; x++) ag_put(x + ox, 20, 1, 1), ag_put(x + ox, 22, 1, 1);
      for (int x = 10; x <= 18; x += 2) ag_put(x + ox, 21, 1, 1);
      for (int s = -1; s <= 1; s += 2)
        for (int k = 0; k < 2; k++) {
          const float p = ag_mod(t * 1.5f + k * 0.5f, 1), x = 14 + s * (10 + p * 2.5f), y = 7.5f - p * 5, a = 0.75f * (1 - p);
          ag_put(x, y, a, 1), ag_put(x + s, y, a * 0.7f, 1), ag_put(x, y - 1, a * 0.7f, 1);
        }
      break;
    }
    case 4: {  // sad: eyes down, brows up in the middle, a frown, a blue tear with a tail
      ag_eye(AG_EL, 14, 0.72f, 0, 0.85f, 3, 0, 0.85f, 1), ag_eye(AG_ER, 14, 0.72f, 0, 0.85f, 3, 0, 0.85f, 1);
      ag_line(AG_EL - 3, 11, AG_EL + 2, 9.2f, 1, 0.8f), ag_line(AG_ER + 3, 11, AG_ER - 2, 9.2f, 1, 0.8f);
      ag_ring(14, 24.5f, 3.6f, 0.9f, 0.8f, 0, -HALO_PI / 2, 0.3f * HALO_PI);
      const float p = ag_mod(t, 2.8f) * (1 / 2.8f);
      if (p < 0.85f) {
        const float q = ag_c01((p - 0.15f) / 0.7f);
        ag_put(AG_EL - 1.5f, 16.5f + 10 * q * q, p < 0.15f ? p / 0.15f : 1, 2, true);
      }
      break;
    }
    case 5: {  // surprised: calm, then the eyes pop open, brows fly up, an "O", sparkles
      const float tt = ag_mod(t, 2.8f);
      const bool calm = tt < 0.7f;
      const float pop = (calm ? 0 : ag_outback(ag_c01((tt - 0.7f) / 0.28f))) * (1 - ag_c01((tt - 2.35f) / 0.4f)), r = 2.4f + pop;
      ag_eye(AG_EL, 13.5f, 1, 0, 0, r, 0, 0.55f, 1), ag_eye(AG_ER, 13.5f, 1, 0, 0, r, 0, 0.55f, 1);
      const float by = 10 - 2.2f * pop;
      ag_ring(AG_EL, by + 2.6f, 2.6f, 0.8f, 1, 0, -HALO_PI / 2, 0.28f * HALO_PI), ag_ring(AG_ER, by + 2.6f, 2.6f, 0.8f, 1, 0, -HALO_PI / 2, 0.28f * HALO_PI);
      ag_ring_e(14, 21.5f, 1.1f + 0.6f * pop, 1.2f + 1.1f * pop, 1);
      const float s = ag_bump(tt, 0.7f, 1.4f);
      if (s > 0) ag_sparkle(3, 9, 2 * s, s), ag_sparkle(25, 9, 2 * s, s);
      break;
    }
    case 6: {  // in love: heart eyes beat, small hearts float up at the sides
      const float bt = ag_mod(t, 1.1f), s = ag_bump(bt, 0, 0.2f) + 0.75f * ag_bump(bt, 0.27f, 0.45f);
      if (s > 0.45f) AG_BITS(AG_HEART_B, AG_EL - 3, 10, 1, 3), AG_BITS(AG_HEART_B, AG_ER - 3, 10, 1, 3);
      else AG_BITS(AG_HEART_M, AG_EL - 2, 11, 1, 3), AG_BITS(AG_HEART_M, AG_ER - 2, 11, 1, 3);
      ag_ring(14, 17, 3.2f, 0.9f, 1, 0, HALO_PI / 2, 0.3f * HALO_PI);
      for (int k = 0; k < 4; k++) {
        const float p = ag_mod(t * 0.42f + k * 0.25f, 1), side = k % 2 ? 1 : -1;
        AG_CRISP(AG_HEART_S, 14 + side * (10.5f + ag_sin(p * 7 + k) * 1.1f) - 1, 24 - p * 18, ag_sin(HALO_PI * p) * 0.85f, 3);
      }
      break;
    }
    case 7: {  // confused: the head aslant, one eye squints, a brow up, a wavy mouth, a "?" bobs
      static const float LOOK[3][2] = {{0.5f, -0.6f}, {0.5f, -0.6f}, {-0.3f, -0.5f}};
      const float *l = LOOK[halo_floor(t * (1 / 1.3f)) % 3];
      ag_eye(AG_EL, 12.5f, 1, l[0], l[1]), ag_eye(AG_ER, 15, 0.55f, l[0], l[1], 3, 0, 0.85f, 1);
      ag_ring(AG_EL, 10, 2.6f, 0.8f, 1, 0, -HALO_PI / 2, 0.28f * HALO_PI);
      ag_line(AG_ER - 2.5f, 11.4f, AG_ER + 2.5f, 10.8f, 1, 0.9f);
      for (int x = -3; x <= 3; x++) ag_put(14 + x, 21.5f + ag_round(ag_sin(x * 1.4f + t * 2) * 0.6f), 0.85f);
      AG_CRISP(AG_QMARK, 18, 1.6f + ag_round(ag_sin(t * 3) * 0.8f), 1);
      break;
    }
    case 8: {  // wink: the right eye winks, a sparkle by it, a smirk
      const float w = ag_bump(ag_mod(t, 2.6f), 0.8f, 1.5f), open = 1 - ag_c01(w * 1.6f);
      ag_eye(AG_EL, 13, 1, 0.35f, -0.1f);
      if (open < 0.35f) ag_happy_eye(AG_ER, 13.5f);
      else ag_eye(AG_ER, 13, open, 0.35f, -0.1f);
      ag_line(11, 20, 15.5f, 20), ag_line(15.5f, 20, 17.5f, 18.3f);
      if (w > 0.2f) ag_sparkle(25, 8, 1.8f * w, w);
      break;
    }
    case 9: {  // bored: heavy lids, eyes aside, now and then a long yawn
      const float yawn = ag_bump(ag_mod(t, 5), 2.4f, 4.2f), ry = 1.7f - 1.1f * yawn;
      for (const float ex : {AG_EL, AG_ER}) {
        ag_ellipse(ex, 14.6f, 3, ry, 1);
        if (ry > 1.1f) ag_cut(ex + 1.2f, 15.2f, 0.85f, 0.92f);
        ag_line(ex - 3, 13.2f, ex + 3, 13.2f);
      }
      if (yawn < 0.12f) ag_line(12, 21, 16, 21, 1, 0.8f);
      else ag_ring_e(14, 21, 1.5f + 0.7f * yawn, 0.6f + 2.3f * yawn, 1);
      break;
    }
    case 10: {  // scared: huge eyes with tiny pupils tremble, a zigzag mouth, a blue drop runs down
      const float ox = ag_round(ag_sin(t * 41) * 0.7f), jx = ag_sin(t * 29) * 0.35f, jy = ag_cos(t * 23) * 0.3f;
      ag_eye(AG_EL + ox, 13, 1, jx, jy, 3.3f, 0, 0.55f, 1), ag_eye(AG_ER + ox, 13, 1, jx, jy, 3.3f, 0, 0.55f, 1);
      ag_line(AG_EL - 3 + ox, 8.8f, AG_EL + 2 + ox, 7.6f, 1, 0.9f), ag_line(AG_ER + 3 + ox, 8.8f, AG_ER - 2 + ox, 7.6f, 1, 0.9f);
      const int ph = halo_floor(t * 12) & 1;
      for (int x = 0; x <= 8; x++) ag_put(10 + x + ox, 21 - ((x + ph) & 1), 1);
      const float p = ag_mod(t, 1.8f) * (1 / 1.8f);
      AG_CRISP(AG_DROP, 22, 3 + p * 4, 1 - p * p, 2);
      break;
    }
    case 11: {  // done: the snake draws a green check mark, sparkles round it, happy eyes
      const float tt = ag_mod(t, 2.8f), fade = 1 - ag_c01((tt - 2.3f) / 0.35f);
      ag_happy_eye(AG_EL, 9, fade), ag_happy_eye(AG_ER, 9, fade);
      const float ax = 8.5f, ay = 18, bx = 12, by = 21.5f, cx = 20, cy = 13.5f;
      const float L1 = 4.9497f, L2 = 11.3137f, q = ag_c01((tt - 0.15f) / 0.55f) * (L1 + L2);
      if (q > 0) {
        const float s1 = fminf(q, L1) / L1;
        ag_line(ax, ay, ax + (bx - ax) * s1, ay + (by - ay) * s1, 1.5f, fade, 4);
        if (q > L1) {
          const float s2 = (q - L1) / L2;
          ag_line(bx, by, bx + (cx - bx) * s2, by + (cy - by) * s2, 1.5f, fade, 4);
        }
      }
      const float b = ag_bump(tt, 0.7f, 1.3f);
      if (b > 0) ag_sparkle(23, 10, 1.6f * b, b, 4), ag_sparkle(5, 15, 1.6f * b, b, 4), ag_sparkle(23, 18, 1.6f * b, b, 4);
      break;
    }
  }
}

// ================= The orb: 340 dots on a sphere (in the 466-px picture the mockup drew; placed by the layer) =================
struct AgS {  // the sphere this frame: radius, middle, squash, turn, tilt, color, brightness, and what a mood keeps
  float R = 150, cx = 233, cy = 233, sx = 1, sy = 1, yaw = 0, tilt = 0.35f, a = 1;
  int col = 0;
  float L = 0, bx = 0, bz = 0, e = 0, ph = 0, flash = 0, m = 0;
  int hop = 0, tb = 0;
  bool g = false;
};
struct AgP {  // a dot this frame: on the sphere (turned: x, y, z; as made: mx, my, mz) and what the mood does to it
  float x, y, z, mx, my, mz, r, h, a, mix, tx, ty, ox, oy;
  int col;
};
static void ag_orb_prep(int mood, float t, AgS &S) {
  S.yaw = t * 0.4f;
  switch (mood) {
    default:
    case 0: S.yaw = t * 0.35f, S.R = 150 * (1 + 0.02f * ag_sin(t * 1.6f)), S.sy = 1 - 0.88f * ag_bump(ag_mod(t, 4.2f), 3.7f, 3.98f); break;
    case 1: S.yaw = t * 0.5f, S.L = ag_voice(t), S.R = 150 * (1 + 0.05f * S.L); break;
    case 2: S.yaw = t * 0.9f, S.bx = ag_sin(t * 2.6f), S.bz = ag_cos(t * 2.6f); break;  // (the meridian's direction)
    case 3: S.yaw = t * 0.3f, S.bx = ag_sin(t * 2.2f), S.bz = ag_cos(t * 2.2f); break;
    case 4: S.yaw = t * 0.6f, S.e = ag_speak(t), S.R = 150 * (1 + 0.06f * S.e); break;
    case 5: {
      const float ph = ag_mod(t * 1.5f, 1), hgt = 4 * ph * (1 - ph), k = ag_c01(1 - fminf(ph, 1 - ph) / 0.09f);
      S.R = 126, S.sx = 1 + 0.14f * k, S.sy = 1 - 0.17f * k, S.cy = 270 - 76 * hgt + 126 * 0.17f * k, S.yaw = t * 1.3f;
      S.ph = ph, S.hop = halo_floor(t * 1.5f);
      break;
    }
    case 6: S.col = 1, S.yaw = t * 2.1f, S.cx = 233 + ag_sin(t * 53) * 4, S.cy = 233 + ag_cos(t * 47) * 3, S.R = 142 * (1 + 0.03f * ag_sin(t * 12)); break;
    case 7: S.col = 2, S.yaw = t * 0.15f, S.sy = 0.9f, S.cy = 244, S.a = 0.78f; break;
    case 8: {
      const float tt = ag_mod(t, 2.6f);
      const bool calm = tt < 0.8f;
      const float pop = (calm ? 0 : ag_outback(ag_c01((tt - 0.8f) / 0.25f))) * (1 - ag_c01((tt - 2.05f) / 0.5f));
      S.R = 150 * (0.82f + 0.2f * pop), S.yaw = t * 0.4f;
      if (!calm) {
        const float u = tt - 0.8f, k = expf(-u * 4) * ag_sin(u * 24);
        S.sx = 1 + 0.1f * k, S.sy = 1 - 0.1f * k, S.flash = 1 - ag_c01(u / 0.45f);
      }
      break;
    }
    case 9: {
      const float tt = ag_mod(t, 3.2f);
      S.m = ag_inout(ag_c01((tt - 0.4f) / 0.5f)) * (1 - ag_inout(ag_c01((tt - 2.3f) / 0.5f))), S.yaw = t * 0.5f;
      break;
    }
    case 10: S.col = 1, S.yaw = t * 0.5f, S.g = ag_mod(t, 1.7f) < 0.55f, S.tb = halo_floor(t * (1 / 0.09f)); break;
    case 11: S.yaw = t * 0.2f, S.sy = 0.11f + 0.05f * ag_sin(t * 1.4f), S.a = 0.65f, S.cy = 248; break;
  }
}
static void ag_orb_dot(int mood, int i, AgP &P, float t, const AgS &S) {
  const float rnd = ag_rnd[i];
  switch (mood) {
    case 1: {  // listen: rings from the middle, as loud as the voice
      const float w = ag_c01(ag_sin((1 - P.z) * 12 - t * 9)), h = S.L * w * w * w * (P.z > 0 ? 1 : 0.3f);
      P.h = h, P.r = 1 + 0.05f * h;
      break;
    }
    case 2: {  // think: sparkles and a bright meridian
      const float w = ag_c01(ag_sin(t * (1.6f + rnd * 2.6f) + rnd * 40)), w2 = w * w, w16 = ag_p8(w2);
      // the meridian at angle t·2.6 round the axis: how far a dot is from it (its angle ≈ √(2(1 − cos)) near it)
      const float r2 = P.x * P.x + P.z * P.z, c = r2 > 0 ? (P.x * S.bx + P.z * S.bz) * ag_rsqrt(r2) : 0;
      const float d = c > 0 ? halo_sqrt(2 * (1 - c)) : 9;
      P.h = fmaxf(w16 * w2 * w2 * w2, 0.55f * ag_c01(1 - d / 0.3f));  // (w^22)
      break;
    }
    case 3: {  // search: the beam
      const float b = ag_c01((P.x * S.bx + P.z * S.bz + P.y * 0.1f - 0.78f) / 0.22f);
      P.h = b * halo_sqrt(b);
      break;
    }
    case 4: P.h = S.e * ag_sq(ag_c01(ag_sin(P.y * 8 + t * 9))) * 0.9f; break;  // speak: waves down it
    case 5: {  // happy: colored sparkles
      const float w = ag_c01(ag_sin(t * (2 + rnd * 3) + rnd * 30)), w2 = w * w;
      P.h = ag_p8(w) * w2 * w2 * w2 * 0.9f;  // (w^14)
      static const uint8_t C[4] = {3, 4, 2, 0};
      if (P.h > 0.5f) P.col = C[i & 3];
      break;
    }
    case 6: {  // angry: spikes
      const float s = ag_sin(P.mx * 5.1f + t * 6.3f) + ag_sin(P.my * 4.7f - t * 5.1f) + ag_sin(P.mz * 5.9f + t * 7.7f);
      const float sp = ag_c01((s - 1.1f) / 1.6f);
      P.r = 1 + 0.3f * sp * sp, P.h = sp * sp * 0.8f;
      break;
    }
    case 7:  // sad: the bottom drips
      if (P.y < -0.84f) {
        const float q = ag_mod(t * 0.32f + rnd, 1);
        if (q > 0.55f) {
          const float u = (q - 0.55f) / 0.45f;
          P.oy = u * u * 150, P.a = 1 - u, P.h = 0.3f;
        }
      }
      break;
    case 8: P.h = S.flash * 0.8f; break;  // surprised: the flash
    case 9:  // done: the dots fly into a check mark
      if (S.m > 0) {
        static const float A[3][2] = {{-72, 2}, {-24, 50}, {80, -56}};
        const float L1 = 67.882f, L2 = 145.79f;
        float s = (float)i / AG_ORB_N * (L1 + L2), L = L1;
        const float *a = A[0], *b = A[1];
        if (s > L1) s -= L1, a = A[1], b = A[2], L = L2;
        const float ux = (b[0] - a[0]) / L, uy = (b[1] - a[1]) / L, off = (rnd - 0.5f) * 16;
        P.tx = S.cx + a[0] + ux * s - uy * off, P.ty = S.cy + a[1] + uy * s + ux * off, P.mix = S.m;
        if (S.m > 0.5f) P.col = 4;
      }
      break;
    case 10:  // error: slices jump aside
      if (S.g) {
        const int sl = halo_floor((P.y + 1) * 5);
        const float hs = ag_hash(sl * 13 + S.tb * 7);
        if (hs > 0.55f || hs < 0.3f) P.ox = (hs - 0.5f) * 70;
        P.a = 0.55f + 0.45f * ag_hash(S.tb * 3 + sl + 101);
      }
      break;
    default: break;
  }
}
// What a mood adds besides the sphere: add(x, y, radius, brightness, color), in the mockup's px
template <typename F> static void ag_orb_extra(int mood, float t, const AgS &S, F add) {
  if (mood == 3) {  // search: the beam's ray, when it looks aside
    const float side = ag_c01(fabsf(S.bx) * 1.4f - 0.2f) * (S.bz > -0.3f ? 1 : 0.35f);
    if (side > 0)
      for (int k = 1; k <= 7; k++) {
        const float d = 1.06f + k * 0.11f;
        add(S.cx + S.bx * S.R * d, S.cy - 0.1f * S.R * d, 3.4f - k * 0.25f, side * (1 - k / 8.0f), 0);
      }
  } else if (mood == 5) {  // happy: confetti from the landing
    const float age = S.ph / 1.5f;
    if (age <= 0.55f) {
      static const uint8_t C[4] = {3, 4, 2, 0};
      for (int j = 0; j < 12; j++) {
        const float r1 = ag_hash(S.hop * 31 + j), r2 = ag_hash(S.hop * 17 + j * 3 + 5), ang = -HALO_PI / 2 + (r1 - 0.5f) * 2.6f;
        const float v = 170 + r2 * 140;
        add(233 + ag_cos(ang) * v * age, 396 + ag_sin(ang) * v * age + 380 * age * age, 3.4f, 1 - age / 0.55f, C[j & 3]);
      }
    }
  } else if (mood == 11) {  // sleep: z's float up
    const float per = 1.6f, life = 3.4f;
    const int k1 = halo_floor(t / per);
    for (int k = k1; k > k1 - 3; k--) {
      const float age = t - k * per;
      if (age < 0 || age > life) continue;
      const float u = age / life, a = ag_sin(HALO_PI * u) * 0.9f, x0 = 300 + u * 50, y0 = 205 - u * 120;
      const char *const *g = k & 1 ? AG_ZB : AG_ZS;
      const int nr = k & 1 ? 5 : 4;
      for (int r = 0; r < nr; r++)
        for (int c = 0; g[r][c]; c++)
          if (g[r][c] == '1') add(x0 + c * 9, y0 + r * 9, 3.3f, a, 0);
    }
  }
}

// ================= The layer =================
static uint32_t ag_color(const AgRun &A, int c) { return c ? AG_COL[c] : A.col0; }

static void ag_layer_free(FaceRun &R) {
  AgRun *A = R.ag;
  if (!A) return;
  free(A->tg), free(A->big), free(A->now), free(A->z), free(A->bx), free(A->b_first), free(A->b_ofs);
  for (HaloMask &m : A->dot) ag_mask_free(m);
  for (HaloMask &m : A->disc) ag_mask_free(m);
  free(A);
  R.ag = nullptr;
}

static void ag_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  ag_init();
  AgRun *A = (AgRun *)heap_caps_calloc(1, sizeof(AgRun), MALLOC_CAP_SPIRAM);
  if (!A) return;
  A->look = L.p2 > 0.5f ? AG_ORB : AG_FACE;
  const int m = (int)lroundf(L.p3);
  A->cycle = m >= AG_MOODS, A->mood = m >= 0 && m < AG_MOODS ? m : 0;
  A->seq = ag_cmd_seq, A->box[0] = A->box[1] = 0, A->box[2] = A->box[3] = -1;
  if (A->look == AG_FACE) {
    if (!AG_ON(AG_FACE)) return free(A);
    A->tg = (float *)heap_caps_calloc(AG_NC * AG_N * 3, sizeof(float), MALLOC_CAP_SPIRAM);
    A->big = (uint8_t *)heap_caps_calloc(AG_N * 3, 1, MALLOC_CAP_SPIRAM);
    if (!A->tg || !A->big) return free(A->tg), free(A->big), free(A);
    A->ht = A->tg + AG_NC * AG_N, A->cr = A->ht + AG_NC * AG_N, A->look_now = A->big + AG_N, A->drawn = A->look_now + AG_N;
    const float pitch = AG_D * L.k / AG_G;
    ag_mask(A->dot[0], pitch * 0.62f, pitch * 0.62f * 0.3f), ag_mask(A->dot[1], pitch * 0.74f, pitch * 0.74f * 0.3f);
  } else {
    if (!AG_ON(AG_ORB) || !ag_pts) return free(A);
    A->now = (AgDot *)heap_caps_calloc(AG_DOTS * 2, sizeof(AgDot), MALLOC_CAP_SPIRAM);
    A->z = (float *)heap_caps_calloc(AG_DOTS * 3, sizeof(float), MALLOC_CAP_SPIRAM);  // (z, then the order)
    A->bx = (float *)heap_caps_calloc(AG_DOTS * 2, sizeof(float), MALLOC_CAP_SPIRAM);
    A->b_first = (uint8_t *)heap_caps_calloc(AG_DOTS * 2, 1, MALLOC_CAP_SPIRAM);
    A->b_ofs = (uint16_t *)heap_caps_calloc(AG_BK + 1 + AG_DOTS * 4, sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (!A->now || !A->z || !A->bx || !A->b_first || !A->b_ofs)
      return free(A->now), free(A->z), free(A->bx), free(A->b_first), free(A->b_ofs), free(A);
    A->was = A->now + AG_DOTS, A->ord = (uint16_t *)(A->z + AG_DOTS);
    A->by = A->bx + AG_DOTS, A->b_idx = A->b_ofs + AG_BK + 1, A->b_m = A->b_first + AG_DOTS;
    for (int k = 0; k < AG_DOTS; k++) A->ord[k] = k;
    for (int k = 0; k < AG_DISCS; k++) ag_mask(A->disc[k], AG_DISC0 + k, (AG_DISC0 + k) / 2.0f);
  }
  R.ag = A;
}

// The command's mood for this layer (applied in its layout); a tap: the next one
static void ag_next(AgRun &A) {
  A.mood = (A.mood + 1) % AG_MOODS, A.cycle = false, A.t = 0;
}
static void ag_command(const char *a) {
  while (*a == ' ') a++;
  for (int look = 0; look < 2; look++) {
    int m = -1;
    if (!strcmp(a, "next")) m = 13;
    else if (!strcmp(a, "all")) m = 12;
    else if (*a >= '0' && *a <= '9') m = atoi(a) % AG_MOODS;
    else
      for (int k = 0; k < AG_MOODS; k++)
        if (!strcmp(a, AG_NAMES[look][k])) m = k;
    ag_cmd[look] = m;
  }
  ag_cmd_seq++;
}

static inline float ag_vx(const AgRun &A, float x) { return halo_view_x(A.v, x); }
static inline float ag_vy(const AgRun &A, float y) { return halo_view_y(A.v, y); }
static void ag_damage(const AgRun &A, float x0, float y0, float x1, float y1) {
  halo_damage_f(ag_vx(A, x0), ag_vy(A, y0), ag_vx(A, x1), ag_vy(A, y1));
}

static void ag_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  AgRun *A = R.ag;
  if (!A) return;
  // Time: from the clock (a screen may be laid out twice in a frame); still in Always On
  const uint32_t now = halo_ms();
  float dt = A->ms ? (now - A->ms) / 1000.0f : 0;
  dt = dt < 0 ? 0 : dt > 0.05f ? 0.05f : dt;
  A->ms = now;
  if (A->seq != ag_cmd_seq) {
    A->seq = ag_cmd_seq;
    const int m = ag_cmd[A->look];
    if (m == 13) ag_next(*A);
    else if (m == 12) A->cycle = true, A->ct = 0;
    else if (m >= 0) A->mood = m, A->cycle = false, A->t = 0;
  }
  if (!fc_calm) {
    A->t += dt;
    if (A->t > AG_WRAP) A->t -= AG_WRAP;
    if (A->cycle && (A->ct += dt) >= AG_CYCLE_S) A->ct = 0, A->mood = (A->mood + 1) % AG_MOODS, A->t = 0;
  }
  A->v = v, A->v.a *= a;
  if (A->v.a <= 0 && A->box[2] < A->box[0]) {  // (hidden, and nothing of it on screen: nothing to work out)
    R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
    A->vh = 0;
    return;
  }
  A->over = face_over_others(S, i);
  A->D = AG_D * L.k, A->cx = CX + L.dx, A->cy = CY + L.dy, A->col0 = L.color;
  R.ax = A->cx, R.ay = A->cy;
  const float h = A->D / 2;
  const float box[4] = {ag_vx(*A, A->cx - h), ag_vy(*A, A->cy - h), ag_vx(*A, A->cx + h), ag_vy(*A, A->cy + h)};
  memcpy(R.box, box, sizeof box);
  // A new view, place, size or color: the whole of it again — where it was and where it is (ag_whole, at the end)
  uint32_t vh = 2166136261u;
  for (const float f : {A->v.x, A->v.y, A->v.s, A->v.a, A->cx, A->cy, A->D})
    vh = (vh ^ (uint32_t)(int32_t)(f * 64)) * 16777619u;
  vh = (vh ^ A->col0 ^ (A->over ? 0x80000000u : 0)) * 16777619u;
  const bool whole = vh != A->vh;
  A->vh = vh;
  auto ag_whole = [&]() {
    if (A->v.a <= 0) R.box[2] = R.box[0] - 1;
    if (whole) {
      if (A->box[2] >= A->box[0]) halo_damage_f(A->box[0], A->box[1], A->box[2], A->box[3]);
      if (R.box[2] >= R.box[0]) halo_damage_f(R.box[0], R.box[1], R.box[2], R.box[3]);
    }
    memcpy(A->box, R.box, sizeof R.box);
  };

  if (A->look == AG_FACE) {
    // Only the colors in play (a mood lights one or two of the five): the board's numbers live in PSRAM, and going over
    // all five every frame cost ~2.5 ms of the update on the board
    ag_r = A, ag_into = A->tg;
    for (int col = 0; col < AG_NC; col++)
      if (A->inked >> col & 1) memset(A->tg + col * AG_N, 0, AG_N * sizeof(float)), memset(A->cr + col * AG_N, 0, AG_N * sizeof(float));
    memset(A->big, 0, AG_N);
    A->used = 0;
    ag_face_mood(A->mood, A->t);
    const float k = expf(-dt * 4.5f);
    uint8_t warm = 0;
    for (int col = 0; col < AG_NC; col++) {
      if (!((A->used | A->hot) >> col & 1)) continue;
      float *ht = A->ht + col * AG_N, *tg = A->tg + col * AG_N, top = 0;
      for (int j = 0; j < AG_N; j++) {
        const float f = ht[j] * k, v = tg[j] > f ? tg[j] : f;
        ht[j] = v, top = v > top ? v : top;
      }
      if (top >= 0.5f / 15) warm |= 1 << col;
      else if (top > 0) memset(ht, 0, AG_N * sizeof(float));  // (gone: no dot shows it any more)
    }
    A->hot = warm, A->inked = A->used;
    const uint8_t live = A->used | warm;
    const float pitch = A->D / AG_G, x0 = A->cx - h, y0 = A->cy - h, r = pitch * 0.44f;
    for (int c = 0; c < AG_N; c++) {
      if (!ag_valid[c]) continue;
      float best = 0;
      int bk = 0;
      for (int col = 0; col < AG_NC; col++) {
        if (!(live >> col & 1)) continue;
        const float f = fmaxf(A->ht[col * AG_N + c], A->cr[col * AG_N + c]);
        if (f > best) best = f, bk = col;
      }
      const int lv = ag_round(best * 15);
      const uint8_t look = lv <= 1 ? 1 : (uint8_t)(bk << 5 | (A->big[c] ? 16 : 0) | lv);
      A->look_now[c] = look;
      if (!whole && look != A->drawn[c]) {
        const float x = x0 + (c % AG_G + 0.5f) * pitch, y = y0 + (c / AG_G + 0.5f) * pitch;
        ag_damage(*A, x - r, y - r, x + r, y + r);
      }
      A->drawn[c] = look;
    }
    return ag_whole();
  }

  // The orb
  AgS Sp;
  ag_orb_prep(A->mood, A->t, Sp);
  const float u = A->D / 466, cyw = cosf(ag_mod(Sp.yaw, 2 * HALO_PI)), syw = sinf(ag_mod(Sp.yaw, 2 * HALO_PI));
  const float ct = cosf(Sp.tilt), st = sinf(Sp.tilt);
  int n = 0;
  auto put = [&](float X, float Y, float sz, float al, int col, float z) {
    if (n >= AG_DOTS) return;
    const float px = A->cx + (X - 233) * u, py = A->cy + (Y - 233) * u;
    int m = ag_round(2 * sz * u) - AG_DISC0;
    m = m < 0 ? 0 : m >= AG_DISCS ? AG_DISCS - 1 : m;
    const int lv = ag_round(ag_c01(al) * 15);
    A->now[n] = {(int16_t)ag_round(px * 2), (int16_t)ag_round(py * 2), (uint8_t)m, (uint8_t)lv, (uint8_t)col, 0};
    A->z[n] = z, n++;
  };
  AgP P;
  for (int k = 0; k < AG_ORB_N; k++) {
    const float mx = ag_pts[3 * k], my = ag_pts[3 * k + 1], mz = ag_pts[3 * k + 2], x1 = mx * cyw + mz * syw, z1 = -mx * syw + mz * cyw;
    P = {x1, my * ct - z1 * st, my * st + z1 * ct, mx, my, mz, 1, 0, 1, 0, 0, 0, 0, 0, Sp.col};
    ag_orb_dot(A->mood, k, P, A->t, Sp);
    const float f = (P.z + 1) / 2;
    float X = Sp.cx + P.x * Sp.R * P.r * Sp.sx + P.ox, Y = Sp.cy - P.y * Sp.R * P.r * Sp.sy + P.oy;
    float sz = Sp.R * 0.03f * (0.5f + 0.62f * f) * (1 + 0.7f * P.h), al = (0.16f + 0.84f * f * halo_sqrt(halo_sqrt(f))) * P.a * Sp.a + 0.6f * P.h, z = P.z;  // (f^1.25 for the mockup's f^1.3)
    // Into the check mark: every other dot fades out on the way and the rest shrink a little — 340 dots on one short line
    // lay over each other (the frame reached 15.2 ms on the board; 59 Hz has 16.9)
    if (P.mix > 0)
      X = ag_lerp(X, P.tx, P.mix), Y = ag_lerp(Y, P.ty, P.mix), sz = ag_lerp(sz, 3.4f, P.mix),
      al = ag_lerp(al, k & 1 ? 0 : 1, P.mix), z = ag_lerp(z, 1, P.mix);
    put(X, Y, sz, al, P.col, z);
  }
  ag_orb_extra(A->mood, A->t, Sp, [&](float X, float Y, float sz, float al, int col) { put(X, Y, sz, al, col, 2); });
  for (int k = n; k < AG_DOTS; k++) A->now[k] = {0, 0, 0, 0, 0, 0}, A->z[k] = -9;
  // Back to front: the order of the last frame, put right (nearly sorted already — insertion sort)
  uint16_t *o = A->ord;
  for (int k = 1; k < AG_DOTS; k++) {
    const uint16_t q = o[k];
    const float zq = A->z[q];
    int j = k - 1;
    while (j >= 0 && A->z[o[j]] > zq) o[j + 1] = o[j], j--;
    o[j + 1] = q;
  }
  // What changed: its old place and its new one
  const int top = n > A->n_was ? n : A->n_was;
  for (int k = 0; k < top; k++) {
    const AgDot &d = A->now[k], &w = A->was[k];
    if (!whole && memcmp(&d, &w, sizeof d)) {
      if (w.a) {
        const float r = (AG_DISC0 + w.m) / 2.0f + 1;
        ag_damage(*A, w.x2 * 0.5f - r, w.y2 * 0.5f - r, w.x2 * 0.5f + r, w.y2 * 0.5f + r);
      }
      if (d.a) {
        const float r = (AG_DISC0 + d.m) / 2.0f + 1;
        ag_damage(*A, d.x2 * 0.5f - r, d.y2 * 0.5f - r, d.x2 * 0.5f + r, d.y2 * 0.5f + r);
      }
    }
    A->was[k] = d;
  }
  A->n = n, A->n_was = n;
  // Into rows of the screen, back to front: a band draws only its rows' dots (going over all 404 in each band cost
  // ~4 ms a frame on the board). In a move (a transition) the dimmest — the far side — are left out: unseen while it
  // moves, and the move redraws the whole screen every frame
  {
    const float s = A->v.s;
    const bool rest = s == 1 && A->v.a >= 0.999f;
    uint16_t pos[AG_BK + 1];
    memset(pos, 0, sizeof pos);
    for (int pass = 0; pass < 2; pass++) {
      if (pass) {
        int sum = 0;
        for (int b = 0; b < AG_BK; b++) A->b_ofs[b] = sum, sum += pos[b], pos[b] = A->b_ofs[b];
        A->b_ofs[AG_BK] = sum;
      }
      for (int k = 0; k < AG_DOTS; k++) {
        const int j = A->ord[k];
        const AgDot &d = A->now[j];
        if (!pass) A->b_first[j] = 255;
        if (!d.a || (!rest && d.a <= 3) || A->v.a <= 0) continue;
        // (scaled — another mask of that size, drawn at scale 1: a mask drawn scaled samples every pixel, slow)
        int mi = d.m;
        if (s != 1) mi = ag_round((AG_DISC0 + d.m) * s) - AG_DISC0, mi = mi < 0 ? 0 : mi >= AG_DISCS ? AG_DISCS - 1 : mi;
        const float hw = A->disc[mi].w / 2.0f, py = ag_vy(*A, d.y2 * 0.5f);
        if (py + hw < 0 || py - hw >= HALO_H) continue;
        const int b0 = py - hw < 0 ? 0 : (int)((py - hw) * (1.0f / AG_BH));
        const int b1 = (int)((py + hw) * (1.0f / AG_BH)) >= AG_BK ? AG_BK - 1 : (int)((py + hw) * (1.0f / AG_BH));
        if (!pass) {
          A->bx[j] = ag_vx(*A, d.x2 * 0.5f) - hw, A->by[j] = py - hw, A->b_first[j] = b0, A->b_m[j] = mi;
          for (int b = b0; b <= b1; b++) pos[b]++;
        } else
          for (int b = b0; b <= b1; b++) A->b_idx[pos[b]++] = j;
      }
    }
  }
  // (an orb's dots may leave the circle: the confetti, the drops — the box grows to hold them)
  for (int k = 0; k < n; k++) {
    const AgDot &d = A->now[k];
    if (!d.a) continue;
    const float r = ((AG_DISC0 + d.m) / 2.0f + 1) * A->v.s, X = ag_vx(*A, d.x2 * 0.5f), Y = ag_vy(*A, d.y2 * 0.5f);
    R.box[0] = fminf(R.box[0], X - r), R.box[1] = fminf(R.box[1], Y - r), R.box[2] = fmaxf(R.box[2], X + r), R.box[3] = fmaxf(R.box[3], Y + r);
  }
  ag_whole();
}

static void ag_draw(const FaceScreen &S, int i) {
  const AgRun *A = S.run[i].ag;
  if (!A || A->v.a <= 0) return;
  const float s = A->v.s, va = A->v.a;
  if (A->look == AG_FACE) {
    const float pitch = A->D / AG_G, x0 = A->cx - A->D / 2, y0 = A->cy - A->D / 2;
    const bool rest = s == 1 && va >= 0.999f;
    for (int cy = 0; cy < AG_G; cy++) {
      const float py = ag_vy(*A, y0 + (cy + 0.5f) * pitch), half = pitch * 0.44f * s;
      if (py + half < hb.y || py - half >= hb.y + hb.rows) continue;
      for (int cx = 0; cx < AG_G; cx++) {
        const int c = cy * AG_G + cx;
        if (!ag_valid[c]) continue;
        const uint8_t L = A->look_now[c];
        const HaloMask &m = A->dot[L >> 4 & 1];
        const float px = ag_vx(*A, x0 + (cx + 0.5f) * pitch), hw = m.w * s / 2;
        if (px + hw < hb.x0 || px - hw > hb.x1 || !m.bits) continue;
        const float br = (L & 15) / 15.0f;
        if (A->over) {
          halo_mask(m, px - hw, py - hw, ag_color(*A, L >> 5), br * va, s);
          continue;
        }
        const uint32_t col = halo_dim(ag_color(*A, L >> 5), (int)(halo_clamp01(br * va) * 256));
        if (!rest && (L & 15) <= 1) {  // (in a transition: an unlit dot is too dim to show its corners)
          const float q = pitch * 0.31f * s;
          halo_rect((int)(px - q), (int)(py - q), (int)(px + q), (int)(py + q), col);
          continue;
        }
        halo_mask(m, px - hw, py - hw, col, 1, s, 0x000000);
      }
    }
    return;
  }
  // The band's rows of dots (agent's rows are AG_BH high: a band of one row keeps the order back to front; a taller
  // band — several rows — draws its dots row by row, each dot once: at the band's first row it reaches)
  const int r0 = hb.y < 0 ? 0 : hb.y / AG_BH, r1 = (hb.y + hb.rows - 1) / AG_BH;
  for (int b = r0; b <= r1 && b < AG_BK; b++)
    for (int e = A->b_ofs[b]; e < A->b_ofs[b + 1]; e++) {
      const int j = A->b_idx[e];
      if (b != (A->b_first[j] > r0 ? A->b_first[j] : r0)) continue;
      const AgDot &d = A->now[j];
      const HaloMask &m = A->disc[A->b_m[j]];
      const float x = A->bx[j], y = A->by[j];
      if (!m.bits || !halo_hits(x, y, x + m.w, y + m.w)) continue;
      halo_mask(m, x, y, ag_color(*A, d.c), d.a / 15.0f * va);
    }
}

// A tap on an agent of this screen (the top one under the finger, without a link): its next mood
static bool ag_tap(const FaceScreen &S) {
  const HaloFinger &f = halo_finger;
  if (!f.tap || f.owner) return false;
  for (int i = S.n - 1; i >= 0; i--) {
    const FaceLayer &L = S.layers[i];
    AgRun *A = S.run[i].ag;
    if (L.kind != FL_AGENT || L.hidden || L.tap > 0 || !A) continue;
    const float dx = f.x - A->cx, dy = f.y - A->cy, r = A->D / 2;
    if (dx * dx + dy * dy <= r * r) return ag_next(*A), true;
  }
  return false;
}
