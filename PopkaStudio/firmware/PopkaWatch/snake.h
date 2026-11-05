// snake.h — the snake that plays itself, shared by the Games (games.h): a board of G × G cells inside the round
// screen, walls, food, and the autopilot from the mockups (web/mockups/snake.html):
//   - the shortest way to the food (BFS; a cell of the body counts as free once the tail has left it), taken only if,
//     after eating, the head can still reach the tail — so the snake never shuts itself in;
//   - else it buys time: of the moves that keep the tail in reach, the one with the most room;
//   - no move at all — it "dies" and starts again (an event), so the lives are endless;
//   - grown to cap — it molts: the tail past 7 cells is shed (an event: the renderer turns it into sparks).
// A step every tick s; between steps the renderer slides each segment from its last cell (sn_cell). Everything a
// renderer shows comes from here; it reads the events (eat / molt / die) each frame and clears them.
// Food that moves (Comet: a star on an orbit): the game's before_step puts the food's cell for this step and, aim on,
// a target — where to meet it (the food is eaten when the head enters its cell, as always).
// The board is cols × rows cells: square (G × G inside the circle) or polar (sn_init_polar: angles round × rings out,
// the angle wrapping round — Comet: the snake goes round orbits and along rays).
// One Snake per game, in PSRAM (~10 KB: the board and the search's scratch). Only in a firmware with a game.

#define SN_MAXC 900  // cells: up to 30 × 30
#define SN_MAXL 72   // body cells at most (cap ≤ this)
enum { SN_EAT, SN_MOLT, SN_DIE };
struct SnakeEvt {
  uint8_t t;
  int16_t cell;  // eat: the food's cell; molt, die: the first shed cell
};
struct Snake {
  uint8_t G = 21, cap = 44;  // G — the square board's side (= cols)
  uint8_t cols = 21, rows = 21;  // the board: columns (angles), rows (rings)
  bool wrap;                 // columns wrap round (polar)
  float tick = 0.11f, inner = 212, cs = 1;  // s a step; the board's radius on screen and a cell, px
  uint8_t valid[SN_MAXC], obst[SN_MAXC];
  int16_t body[SN_MAXL], prev[SN_MAXL];  // cells (y·cols + x), head first; prev — before the last step
  int n, pn;
  int16_t food;
  uint32_t food_n;           // foods placed so far (a game sees a new one by it)
  int16_t target;            // aim: the path goes here, not to the food's cell
  bool aim;
  void (*before_step)(Snake &);  // the game's: before each step (moving food)
  float time, acc, last_eat = -9;
  int score, best;
  uint32_t rnd = 1;
  SnakeEvt ev[16];
  int ev_n;
  int16_t shed[SN_MAXL];  // the cells of the last molt or death
  int shed_n;
  // the search's scratch
  int16_t occ[SN_MAXC], dist[SN_MAXC], from[SN_MAXC], q[SN_MAXC], path[SN_MAXC], vb[SN_MAXL + SN_MAXC];
};

static uint32_t sn_rand(Snake &S) {  // xorshift32
  uint32_t x = S.rnd;
  x ^= x << 13, x ^= x >> 17, x ^= x << 5;
  return S.rnd = x;
}
static inline float sn_randf(Snake &S) { return (sn_rand(S) >> 8) * (1.0f / 16777216); }
static inline int sn_wx(const Snake &S, int x) { return S.wrap ? (x + S.cols) % S.cols : x; }
static inline bool sn_free(const Snake &S, int x, int y) {
  x = sn_wx(S, x);
  if (x < 0 || y < 0 || x >= S.cols || y >= S.rows) return false;
  const int i = y * S.cols + x;
  return S.valid[i] && !S.obst[i];
}
static void sn_event(Snake &S, uint8_t t, int16_t cell) {
  if (S.ev_n < 16) S.ev[S.ev_n++] = {t, cell};
}

// Breadth-first from start: to goal (the path into S.path, its length — 0: none), or, goal < 0, how many cells it
// reaches. body[0..bn) — the snake as it is: its cell i is free after bn − i steps
static int sn_bfs(Snake &S, int16_t start, int16_t goal, const int16_t *body, int bn) {
  const int N = S.cols * S.rows, NC = S.cols;
  memset(S.occ, 0, N * 2);
  for (int i = 0; i < bn; i++) S.occ[body[i]] = bn - i;
  for (int i = 0; i < N; i++) S.dist[i] = -1;
  int h = 0, t = 0;
  S.q[t++] = start, S.dist[start] = 0;
  static const int8_t DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  while (h < t) {
    const int c = S.q[h++], cx = c % NC, cy = c / NC, d = S.dist[c];
    for (int k = 0; k < 4; k++) {
      const int nx = sn_wx(S, cx + DX[k]), ny = cy + DY[k];
      if (!sn_free(S, nx, ny)) continue;
      const int ni = ny * NC + nx;
      if (S.dist[ni] >= 0 || S.occ[ni] > d + 1) continue;
      S.dist[ni] = d + 1, S.from[ni] = c, S.q[t++] = ni;
      if (ni == goal) {
        const int len = d + 1;
        for (int k2 = ni, j = len - 1; j >= 0; k2 = S.from[k2], j--) S.path[j] = k2;
        return len;
      }
    }
  }
  return goal < 0 ? t : 0;
}

static int16_t sn_choose(Snake &S) {
  const int16_t head = S.body[0];
  const int len = sn_bfs(S, head, S.aim ? S.target : S.food, S.body, S.n);
  if (len > 0) {  // the food — only if, after eating it, the tail is still in reach
    const int16_t first = S.path[0];
    int m = 0;
    for (int j = len - 1; j >= 0 && m < S.n + 1; j--) S.vb[m++] = S.path[j];
    for (int i = 0; i < S.n && m < S.n + 1; i++) S.vb[m++] = S.body[i];
    if (sn_bfs(S, S.vb[0], S.vb[m - 1], S.vb, m) > 0) return first;
  }
  // Buy time: keep the tail in reach, with the most room (a little chance between equals)
  int16_t best = -1;
  float bs = -1;
  const int NC = S.cols, hx = head % NC, hy = head / NC;
  static const int8_t DX[4] = {1, 0, -1, 0}, DY[4] = {0, 1, 0, -1};
  for (int k = 0; k < 4; k++) {
    const int nx = sn_wx(S, hx + DX[k]), ny = hy + DY[k];
    if (!sn_free(S, nx, ny)) continue;
    const int16_t c = ny * NC + nx;
    bool on_body = false;
    for (int i = 0; i < S.n - 1; i++) on_body |= S.body[i] == c;  // (the tail's cell is free in a step)
    if (on_body) continue;
    S.vb[0] = c;
    for (int i = 0; i < S.n - 1; i++) S.vb[i + 1] = S.body[i];
    const bool tail = sn_bfs(S, c, S.vb[S.n - 1], S.vb, S.n) > 0;
    const int room = sn_bfs(S, c, -1, S.vb, S.n);
    const float s = (tail ? 1e5f : 0) + room * 10 + sn_randf(S);
    if (s > bs) bs = s, best = c;
  }
  return best;
}

static void sn_spawn(Snake &S) {
  for (int k = 0; k < 400; k++) {
    const int x = S.wrap ? sn_rand(S) % S.cols : 2 + sn_rand(S) % (S.cols - 8), y = sn_rand(S) % S.rows;
    bool ok = true;
    for (int i = 0; i < 5; i++) ok &= sn_free(S, x + i, y);
    if (!ok) continue;
    S.n = 5;
    for (int i = 0; i < 5; i++) S.body[i] = y * S.cols + sn_wx(S, x + i);
    memcpy(S.prev, S.body, sizeof S.body), S.pn = S.n;
    return;
  }
}
static void sn_place_food(Snake &S) {
  const int NC = S.cols, hx = S.body[0] % NC, hy = S.body[0] / NC;
  for (int k = 0; k < 600; k++) {
    const int x = sn_rand(S) % NC, y = sn_rand(S) % S.rows, c = y * NC + x;
    const int dx = S.wrap ? min(abs(x - hx), NC - abs(x - hx)) : abs(x - hx);
    if (!sn_free(S, x, y) || dx + abs(y - hy) < 4) continue;
    bool on = false;
    for (int i = 0; i < S.n; i++) on |= S.body[i] == c;
    if (!on) return void((S.food = c, S.food_n++));
  }
}

// The board: G cells across inside a circle of radius inner (px); a cell is on it if its middle is within G/2 − edge
// cells of the centre; walls — cells; seed — the game's own
static void sn_init(Snake &S, int G, float tick, int cap, const int16_t *walls, int wn, uint32_t seed, float inner = 212,
                    float edge = 0.6f) {
  S.G = S.cols = S.rows = G, S.wrap = false;
  S.tick = tick, S.cap = min(cap, SN_MAXL - 1), S.inner = inner, S.cs = 2 * inner / G, S.rnd = seed | 1;
  for (int y = 0; y < G; y++)
    for (int x = 0; x < G; x++) {
      const float dx = x + 0.5f - G / 2.0f, dy = y + 0.5f - G / 2.0f, r = G / 2.0f - edge;
      S.valid[y * G + x] = dx * dx + dy * dy <= r * r, S.obst[y * G + x] = 0;
    }
  for (int i = 0; i + 1 < wn; i += 2)
    if (walls[i] >= 0 && walls[i + 1] >= 0 && walls[i] < G && walls[i + 1] < G) S.obst[walls[i + 1] * G + walls[i]] = 1;
  S.time = S.acc = 0, S.last_eat = -9, S.score = S.best = 0, S.ev_n = 0, S.shed_n = 0;
  sn_spawn(S), sn_place_food(S);
}

// A polar board: A angles round (wrapping) × R rings, all free (the renderer maps them to orbits)
static void sn_init_polar(Snake &S, int A, int R, float tick, int cap, uint32_t seed) {
  S.G = S.cols = A, S.rows = R, S.wrap = true;
  S.tick = tick, S.cap = min(cap, SN_MAXL - 1), S.rnd = seed | 1;
  for (int i = 0; i < A * R; i++) S.valid[i] = 1, S.obst[i] = 0;
  S.time = S.acc = 0, S.last_eat = -9, S.score = S.best = 0, S.ev_n = 0, S.shed_n = 0;
  sn_spawn(S), sn_place_food(S);
}

static void sn_step(Snake &S) {
  if (S.before_step) S.before_step(S);
  memcpy(S.prev, S.body, S.n * 2), S.pn = S.n;
  const int16_t c = sn_choose(S);
  if (c < 0) {  // shut in: it starts again (the old body — sparks)
    S.shed_n = S.n, memcpy(S.shed, S.body, S.n * 2);
    sn_event(S, SN_DIE, S.body[0]);
    S.score = 0, sn_spawn(S), sn_place_food(S);
    return;
  }
  memmove(S.body + 1, S.body, S.n * 2), S.body[0] = c;
  if (c == S.food) {
    S.n++, S.score++, S.best = max(S.best, S.score), S.last_eat = S.time;
    sn_event(S, SN_EAT, c);
    if (S.n >= S.cap) {  // molt: the tail past 7 cells is shed
      S.shed_n = S.n - 7, memcpy(S.shed, S.body + 7, S.shed_n * 2), S.n = 7;
      sn_event(S, SN_MOLT, S.shed[0]);
    }
    sn_place_food(S);
  }
}
static void sn_update(Snake &S, float dt) {
  S.time += dt, S.acc += dt;
  if (S.acc > S.tick * 4) S.acc = S.tick * 4;  // (after a pause: no burst of steps)
  while (S.acc >= S.tick) S.acc -= S.tick, sn_step(S);
}
static inline float sn_frac(const Snake &S) { return S.acc / S.tick; }
// Segment i between its last cell and this one, in cells (fractional)
static inline void sn_cell(const Snake &S, int i, float f, float &fx, float &fy) {
  const int a = S.prev[min(i, S.pn - 1)], b = S.body[i], NC = S.cols;
  int dx = b % NC - a % NC;
  if (S.wrap && abs(dx) > NC / 2) dx -= dx > 0 ? NC : -NC;  // (round the wrap: the short way)
  fx = a % NC + dx * f, fy = a / NC + (b / NC - a / NC) * f;
}
static inline float sn_px(const Snake &S, float fx) { return CX - S.inner + (fx + 0.5f) * S.cs; }
