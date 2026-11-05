// game_iso.h — Voxel Snake (mockup 9): pseudo-3D — a round board seen from above at an angle (isometric, 2:1),
// checkered tiles on a dark rim, the snake of cubes lit from the left (the top lightest, the right side darkest),
// eyes on the head; the food — a golden cube that bobs and spins (its sides lit as they turn), with a shadow on the
// board; the walls — stone towers two cubes high. Eaten — sparks that fall. Drawn with own code: convex polygons
// filled row by row in the band (the slopes worked out once per face), back to front (painter's order). Vertices
// snapped to whole pixels: the 2:1 edges stay clean steps, no blending. Redrawn: each cube that moved, the food, the
// sparks. Bigger since 5 Nov (a tile 40 px wide, the board nearly across the screen), no score (the user's word).
// Included from games.h.

#define IS_G 13
#define IS_K 20.0f    // half a tile's width, px
#define IS_H 24.0f    // a cube's height, px
#define IS_Y0 130.0f  // where cell (0, 0) is: the board's middle at y 250
#define IS_SC (IS_K / 15)  // (the mockup's sizes were for 15)
struct IsItem {
  float x, y, z, h, s;  // cells; the floor; height (cubes); half its side (cells)
  uint32_t c[3];        // top, left side, right side
  float spin;           // ≥ 0 — the food: turning round its axis; −2 — the head (eyes after it)
  float depth;
};
static Snake *is_s;
static IsItem *is_item;
static GmBox *is_box;  // a box per cube of the snake
static GmBox is_food_box;
static float is_t;
static const int16_t IS_WALLS[] = {3, 3, 9, 4, 4, 9, 9, 9};

static inline float is_px(float x, float y) { return CX + (x - y) * IS_K; }
static inline float is_py(float x, float y, float z) { return IS_Y0 + (x + y) * IS_K * 0.5f - z * IS_H; }

static void iso_build() {
  is_s = (Snake *)heap_caps_calloc(1, sizeof(Snake), MALLOC_CAP_SPIRAM);
  is_item = (IsItem *)heap_caps_calloc(SN_MAXL + 8, sizeof(IsItem), MALLOC_CAP_SPIRAM);
  is_box = (GmBox *)heap_caps_calloc(SN_MAXL, sizeof(GmBox), MALLOC_CAP_SPIRAM);
  if (!is_s || !is_item || !is_box) return void(is_s = nullptr);
  sn_init(*is_s, IS_G, 0.16f, 30, IS_WALLS, sizeof IS_WALLS / 2, 37);
}

static void iso_tick(float dt) {
  if (!is_s) return;
  Snake &S = *is_s;
  sn_update(S, dt), is_t += dt;
  static const uint32_t SPARK[2] = {0xB8FF6A, 0xFFFFFF};
  for (int e = 0; e < S.ev_n; e++) {
    const SnakeEvt &E = S.ev[e];
    const int n = E.t == SN_EAT ? 1 : S.shed_n;
    for (int i = 0; i < n; i += E.t == SN_EAT ? 1 : 2) {
      const int c = E.t == SN_EAT ? E.cell : S.shed[i];
      gm_burst(is_px(c % IS_G, c / IS_G), is_py(c % IS_G, c / IS_G, 0.5f), E.t == SN_EAT ? 20 : 5, 100, 0.9f, SPARK, 2, 2.2f, 200);
    }
  }
  S.ev_n = 0;
  gm_parts_tick(dt, false);
}

// ---------- Faces: convex polygons, row by row (screen px, after the view) ----------
static void is_poly(const float *x, const float *y, int n, uint32_t rgb) {  // (n ≤ 32)
  float y0 = y[0], y1 = y[0], x0 = x[0], x1 = x[0];
  for (int i = 1; i < n; i++) y0 = fminf(y0, y[i]), y1 = fmaxf(y1, y[i]), x0 = fminf(x0, x[i]), x1 = fmaxf(x1, x[i]);
  if (!halo_hits(x0, y0, x1, y1)) return;
  const int r0 = max(-halo_floor(-(y0 - 0.5f)), hb.y), r1 = min(halo_floor(y1 - 0.5f), hb.y + hb.rows - 1);
  if (r0 > r1) return;
  float k[32];  // each edge's dx/dy (once; up to 32 vertices)
  for (int i = 0; i < n; i++) {
    const float dy = y[(i + 1) % n] - y[i];
    k[i] = dy != 0 ? (x[(i + 1) % n] - x[i]) / dy : 0;
  }
  const uint16_t c = halo_565(rgb);
  for (int r = r0; r <= r1; r++) {
    const float yc = r + 0.5f;
    float xl = 1e9f, xr = -1e9f;
    for (int i = 0; i < n; i++) {
      const float ay = y[i], by = y[(i + 1) % n];
      if ((ay <= yc && by > yc) || (by <= yc && ay > yc)) {
        const float xi = x[i] + (yc - ay) * k[i];
        xl = fminf(xl, xi), xr = fmaxf(xr, xi);
      }
    }
    if (xl > xr) continue;
    const int a = max(-halo_floor(-(xl - 0.5f)), hb.x0), b = min(halo_floor(xr - 0.5f), hb.x1);
    if (a <= b) halo_span(halo_row(r), a, b, c);
  }
}
// The cubes' faces this frame, back to front, on screen (after the view) — worked out once in the layout; the draw
// (per band, both cores) only fills them. eyes — the head's eyes go here
struct IsFace {
  float x[4], y[4];
  uint32_t rgb;
  bool eyes;
};
#define IS_FACES ((SN_MAXL + 8) * 5)
static IsFace *is_face;
static int is_nface;
static float is_eye[4][3];  // x, y, r of the eyes' whites and pupils
// A quad of board points (cells, z) through the view; (sx, sy) — the snap to whole pixels
static void is_quad(const float (*p)[3], float sx, float sy, uint32_t rgb) {
  if (!is_face || is_nface >= IS_FACES) return;
  IsFace &f = is_face[is_nface++];
  for (int i = 0; i < 4; i++) f.x[i] = gm_x(is_px(p[i][0], p[i][1]) + sx), f.y[i] = gm_y(is_py(p[i][0], p[i][1], p[i][2]) + sy);
  f.rgb = gm_col(rgb), f.eyes = false;
}
static void is_cube(const IsItem &it) {
  const float bx = is_px(it.x, it.y), by = is_py(it.x, it.y, it.z);
  const float sx = roundf(bx) - bx, sy = roundf(by) - by, s = it.s, z0 = it.z, z1 = it.z + it.h;
  if (it.spin < 0) {  // (a plain cube; −2 — the head)
    const float L[4][3] = {{it.x - s, it.y + s, z0}, {it.x + s, it.y + s, z0}, {it.x + s, it.y + s, z1}, {it.x - s, it.y + s, z1}};
    const float R[4][3] = {{it.x + s, it.y - s, z0}, {it.x + s, it.y + s, z0}, {it.x + s, it.y + s, z1}, {it.x + s, it.y - s, z1}};
    const float T[4][3] = {{it.x - s, it.y - s, z1}, {it.x + s, it.y - s, z1}, {it.x + s, it.y + s, z1}, {it.x - s, it.y + s, z1}};
    is_quad(L, sx, sy, it.c[1]), is_quad(R, sx, sy, it.c[2]), is_quad(T, sx, sy, it.c[0]);
    return;
  }
  // Turning: its corners round the middle; the sides facing the viewer (+x +y), lit by how they face
  float cx[4], cy[4];
  for (int k = 0; k < 4; k++) {
    const float a = it.spin + HALO_PI / 4 + k * HALO_PI / 2;
    cx[k] = it.x + s * 1.414f * cosf(a), cy[k] = it.y + s * 1.414f * sinf(a);
  }
  for (int k = 0; k < 4; k++) {
    const int j = (k + 1) % 4;
    const float nx = cx[k] + cx[j] - 2 * it.x, ny = cy[k] + cy[j] - 2 * it.y;
    if (nx + ny <= 0) continue;
    const float lit = 0.55f + 0.45f * halo_clamp01((nx * -0.2f + ny * 1.0f) / (fabsf(nx) + fabsf(ny) + 1e-3f) + 0.3f);
    const float F[4][3] = {{cx[k], cy[k], z0}, {cx[j], cy[j], z0}, {cx[j], cy[j], z1}, {cx[k], cy[k], z1}};
    is_quad(F, sx, sy, halo_dim(it.c[1], (int)(lit * 256)));
  }
  const float T[4][3] = {{cx[0], cy[0], z1}, {cx[1], cy[1], z1}, {cx[2], cy[2], z1}, {cx[3], cy[3], z1}};
  is_quad(T, sx, sy, it.c[0]);
}
static void is_box_of(const IsItem &it, float &x0, float &y0, float &x1, float &y1) {
  const float r = it.s * 1.42f;
  x0 = is_px(it.x - r, it.y + r) - 2, x1 = is_px(it.x + r, it.y - r) + 2;
  y0 = is_py(it.x - r, it.y - r, it.z + it.h) - 2, y1 = is_py(it.x + r, it.y + r, it.z) + 2;
}

static int is_n;
static float (*is_tile)[8];  // the board's tiles this frame: 4 corners on screen (after the view)
static int is_ntile;
static float is_rim[2][24];
static void iso_layout() {
  if (!is_s) return;
  const Snake &S = *is_s;
  const float f = sn_frac(S);
  int n = 0;
  for (int y = 0; y < IS_G; y++)
    for (int x = 0; x < IS_G; x++)
      if (S.obst[y * IS_G + x]) is_item[n++] = {(float)x, (float)y, 0, 1.8f, 0.45f, {0x8B8F99, 0x6A6E78, 0x4C4F57}, -1, 0};
  for (int i = 0; i < S.n; i++) {
    IsItem &it = is_item[n++];
    sn_cell(S, i, f, it.x, it.y);
    const float k = (float)i / S.n;
    it.z = 0, it.h = 0.8f, it.s = 0.41f - 0.1f * k, it.spin = -1;
    if (i) it.c[0] = gm_rgb_mix(0xB8FF6A, 0x4CAF50, k), it.c[1] = gm_rgb_mix(0x7FD34A, 0x2E7D32, k), it.c[2] = gm_rgb_mix(0x5AA832, 0x1B5E20, k);
    else it.c[0] = 0xD4FF8A, it.c[1] = 0x9BE35A, it.c[2] = 0x6CBF3A, it.spin = -2;
    float x0, y0, x1, y1;
    is_box_of(it, x0, y0, x1, y1);
    gm_mark(is_box[i], x0, y0, x1, y1, gm_mix(gm_mix((int32_t)(it.x * 64), (int32_t)(it.y * 64)), (int32_t)(k * 256)));
  }
  for (int i = S.n; i < SN_MAXL; i++)
    if (is_box[i].h) gm_mark(is_box[i], 1, 1, 0, 0, 0);
  IsItem &fd = is_item[n++];
  fd = {(float)(S.food % IS_G), (float)(S.food / IS_G), 0.6f + 0.25f * sinf(is_t * 3), 0.45f, 0.24f, {0xFFE066, 0xFF9F1C, 0xE07A00}, fmodf(is_t * 1.6f, 2 * HALO_PI), 0};
  float x0, y0, x1, y1;
  is_box_of(fd, x0, y0, x1, y1);
  const float shx = is_px(fd.x, fd.y), shy = is_py(fd.x, fd.y, 0);
  gm_mark(is_food_box, fminf(x0, shx - 14), y0, fmaxf(x1, shx + 14), fmaxf(y1, shy + 8), gm_mix(S.food, (int32_t)(is_t * 60)));
  for (int i = 0; i < n; i++) is_item[i].depth = is_item[i].x + is_item[i].y + is_item[i].z * 0.1f;
  for (int i = 1; i < n; i++) {  // back to front (insertion sort: nearly sorted from the last frame)
    IsItem t = is_item[i];
    int j = i - 1;
    while (j >= 0 && is_item[j].depth > t.depth) is_item[j + 1] = is_item[j], j--;
    is_item[j + 1] = t;
  }
  is_n = n;
  // The board through this frame's view, once (the draw runs per band, on both cores)
  if (!is_tile) is_tile = (float(*)[8])heap_caps_malloc(IS_G * IS_G * sizeof(float) * 8, MALLOC_CAP_SPIRAM);
  is_ntile = 0;
  for (int y = 0; is_tile && y < IS_G; y++)
    for (int x = 0; x < IS_G; x++)
      if (S.valid[y * IS_G + x]) {
        float *t = is_tile[is_ntile++];
        const float cx[4] = {x - 0.5f, x + 0.5f, x + 0.5f, x - 0.5f}, cy[4] = {y - 0.5f, y - 0.5f, y + 0.5f, y + 0.5f};
        for (int k = 0; k < 4; k++) t[k] = gm_x(is_px(cx[k], cy[k])), t[4 + k] = gm_y(is_py(cx[k], cy[k], 0));
      }
  for (int k = 0; k < 24; k++)  // the rim: an ellipse a little below the board
    is_rim[0][k] = gm_x(CX + 152 * IS_SC * cosf(k * HALO_PI / 12)), is_rim[1][k] = gm_y(IS_Y0 + 6 * IS_K + 16 + 76 * IS_SC * sinf(k * HALO_PI / 12));
  // The cubes' faces, back to front; the eyes right after the head's (what is in front of it covers them too)
  if (!is_face) is_face = (IsFace *)heap_caps_calloc(IS_FACES, sizeof(IsFace), MALLOC_CAP_SPIRAM);
  is_nface = 0;
  for (int i = 0; i < n; i++) {
    is_cube(is_item[i]);
    if (is_item[i].spin != -2 || !is_face || is_nface >= IS_FACES) continue;
    is_face[is_nface++].eyes = true;
    float hx, hy, bx, by;
    const float f = sn_frac(S);
    sn_cell(S, 0, f, hx, hy), sn_cell(S, 1, f, bx, by);
    const float px = is_px(hx, hy), py = is_py(hx, hy, 0.8f), dx = (hx - bx) - (hy - by), dy = ((hx - bx) + (hy - by)) * 0.5f;
    for (int s = 0; s < 2; s++) {
      const float side = (s ? 5 : -5) * IS_SC;
      is_eye[s][0] = gm_x(px + side + dx * 3 * IS_SC), is_eye[s][1] = gm_y(py + dy * 2 * IS_SC), is_eye[s][2] = 2.6f * IS_SC * gm_v.s;
      is_eye[2 + s][0] = gm_x(px + side + dx * 4 * IS_SC), is_eye[2 + s][1] = gm_y(py + dy * 3 * IS_SC), is_eye[2 + s][2] = 1.3f * IS_SC * gm_v.s;
    }
  }
  gm_parts_layout();
}

static void iso_draw() {
  if (!is_s) return;
  const Snake &S = *is_s;
  // The sky: a gradient, row by row
  for (int r = hb.y; r < hb.y + hb.rows; r++) {
    const float t = halo_clamp01((r - gm_y(0)) / (466 * gm_v.s));
    halo_span(halo_row(r), hb.x0, hb.x1, halo_565(gm_col(gm_rgb_mix(0x0C1A2A, 0x05080D, t))));
  }
  // The board: its rim (an ellipse), the tiles
  is_poly(is_rim[0], is_rim[1], 24, gm_col(0x1B2A1F));
  const uint32_t tile[2] = {gm_col(0x35533B), gm_col(0x2F4A35)};
  for (int i = 0, k = 0; i < is_ntile; i++) {
    const float *t = is_tile[i];
    while (!S.valid[k]) k++;
    is_poly(t, t + 4, 4, tile[(k % IS_G + k / IS_G) & 1]), k++;
  }
  // The food's shadow, then everything back to front
  const float fx = S.food % IS_G, fy = S.food / IS_G;
  const float sw = 9 * IS_SC, sh = 4 * IS_SC;
  halo_rrect(gm_x(is_px(fx, fy) - sw), gm_y(is_py(fx, fy, 0) - sh), gm_x(is_px(fx, fy) + sw), gm_y(is_py(fx, fy, 0) + sh), sh * gm_v.s,
             gm_col(0x000000), 0.35f);
  for (int i = 0; i < is_nface; i++) {
    const IsFace &fc = is_face[i];
    if (!fc.eyes) {
      is_poly(fc.x, fc.y, 4, fc.rgb);
      continue;
    }
    for (int e = 0; e < 4; e++) halo_circle(is_eye[e][0], is_eye[e][1], is_eye[e][2], gm_col(e < 2 ? 0xFFFFFF : 0x111111));
  }
  gm_parts_draw(false);
}
