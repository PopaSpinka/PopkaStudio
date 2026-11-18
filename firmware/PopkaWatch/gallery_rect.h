// gallery_rect.h — the gallery on a rectangular screen (variant G6 of web/mockups/rect-screens.html). The layers are the
// round watch's (gallery.h: the title, the cards, the scroll indicator); only where they stand and how they scroll is
// this board's. The cards flow over a grid instead of one column: a first card that is live (the weather: FaceLayer::p2)
// takes the left half, the next two stack on the right, the rest go three in a row. Without a live card first — three in
// a row from the top. The whole page is one lib/Halo list (one item as tall as the page): the same finger, inertia and
// rubber as on the watch, nothing shrinks at the edges — a rectangle has no circle to follow. The 7″ board held tall
// (HALO_P4_TALL) has one column instead: the live card, then every card the whole width (rect-tall.html 2A).
// Included from gallery.h in place of its list layout when HALO_ROUND is not defined.

extern const HaloFont gl_r_title, gl_r_t28, gl_r_t32, gl_r_ic28, gl_r_n64;  // assets/rect_fonts.conf
// (games.h: a game alive in its card)
static void games_mini_layout(uint8_t kind, const HaloView &page, float x, float y, float w, float h, float a);
static void games_mini_draw(uint8_t kind);

#if defined(HALO_SQUARE)  // the square, 480 × 480 (variant 2D of web/mockups/square-screens.html): a launcher — three
                          // tiles in a row, a big icon in its circle over the name, no second line; the live weather is
                          // the first tile (its sky and temperature). The games keep the small rectangle's three cards
#define RG_PAD 16.0f
#define RG_GAP_X 11.5f      // between the tiles
#define RG_TOP 52.0f
#define RG_TILE_H 132.0f
#define RG_ITEM_H RG_TILE_H
#define RG_BIG_H RG_TILE_H
#define RG_STACK_GAP 8.0f
#define RG_ROW_GAP 8.0f
#define RG_END 16.0f
#define RG_RADIUS 22.0f
#define RG_RADIUS_DOWN 15.0f
#define RG_TITLE_Y 30.0f
#define RG_ICON_R 30.0f
#define RG_ICON_Y 56.0f     // a tile: the icon's middle from its top
#define RG_NAME_Y 115.0f    // the name's middle
#define RG_SKY_Y 46.0f      // the weather tile: the sky's middle
#define RG_DEG_Y 107.0f     // and the temperature's
#define RG_GAME_COLS 3
#define RG_GAME_GAP 8.0f
#define RG_GAME_W ((HALO_W - 2 * RG_PAD - (RG_GAME_COLS - 1) * RG_GAME_GAP) / RG_GAME_COLS)
#define RG_GAME_H 246.0f
#define RG_GAME_STEP (RG_GAME_W + RG_GAME_GAP)
#define RG_SCENE_H (RG_GAME_H - 84.0f)
#define RG_FONT_SUB gl_c_t14
extern const HaloFont gl_c_t14, gl_q_ic30, gl_q_ic48, gl_q_n28;  // (studio.py: SQUARE_FONTS)
#elif defined(HALO_COMPACT)  // a small rectangle: two cards in a row when it is wide, one when it is tall
#define RG_WIDE (HALO_W >= HALO_H)  // (a square — the 4″ 480 × 480 board — is laid out as wide: those layouts are drawn 480 wide)
#define RG_COLS (RG_WIDE ? 2 : 1)
#define RG_PAD 12.0f
#define RG_GAP_X 10.0f
#define RG_TOP 58.0f
#define RG_ITEM_H 72.0f
#define RG_STACK_GAP 6.0f
#define RG_BIG_H (2 * RG_ITEM_H + RG_STACK_GAP)
#define RG_ROW_GAP 10.0f
#define RG_END 14.0f
#define RG_RADIUS 18.0f
#define RG_RADIUS_DOWN 12.0f
#define RG_TITLE_Y 30.0f
#define RG_ICON_R 19.0f
#define RG_GAME_COLS (RG_WIDE ? 3 : 2)
#define RG_GAME_GAP 8.0f
#define RG_GAME_W ((HALO_W - 2 * RG_PAD - (RG_GAME_COLS - 1) * RG_GAME_GAP) / RG_GAME_COLS)
#define RG_GAME_H (RG_WIDE ? 246.0f : 236.0f)
#define RG_GAME_STEP (RG_GAME_W + RG_GAME_GAP)
#define RG_SCENE_H (RG_GAME_H - 84.0f)
#define RG_FONT_SUB gl_c_t14     // the line under a card's title (the watch's 20 px is its title's size here)
extern const HaloFont gl_c_t14;
#elif defined(HALO_P4_TALL)  // the 7″ board held tall, 600 × 1024 (web/mockups/rect-tall.html 2A, 3A): one column — the live
                             // card across the top, every card the whole width under it; the games stacked, each alive in
                             // a wide band at its card's top
#define RG_PAD 32.0f
#define RG_GAP_X 24.0f
#define RG_TOP 96.0f
#define RG_BIG_H 236.0f
#define RG_ITEM_H 112.0f
#define RG_STACK_GAP 12.0f    // between the cards of the column
#define RG_ROW_GAP 18.0f      // under the live card; between the games
#define RG_END 24.0f
#define RG_RADIUS 26.0f
#define RG_RADIUS_DOWN 18.0f
#define RG_TITLE_Y 45.0f
#define RG_ICON_R 28.0f
#define RG_GAME_COLS 1
#define RG_GAME_W 536.0f      // a game's card (variant 3A): the page's width
#define RG_GAME_H 286.0f
#define RG_GAME_STEP 560.0f
#define RG_SCENE_H 176.0f     // the band the game shows in
#define RG_GAME_NAME_UP 64.0f  // a game's name and its line: their baselines up from the card's bottom
#define RG_GAME_SUB_UP 28.0f
#define RG_LIVE_T_Y 127.0f    // the live card: the middle of its temperature, the baseline of its line, from its top
#define RG_LIVE_SUB_Y 196.0f
#define RG_FONT_SUB gl_t20
#else
#define RG_PAD 32.0f        // the page's margin
#define RG_GAP_X 24.0f      // between columns
#define RG_TOP 96.0f        // the first row, under the title
#define RG_BIG_H 236.0f     // the live card
#define RG_ITEM_H 112.0f    // a card
#define RG_STACK_GAP 12.0f  // the two cards beside the live one
#define RG_ROW_GAP 18.0f    // between rows
#define RG_END 24.0f        // under the last row
#define RG_RADIUS 26.0f
#define RG_RADIUS_DOWN 18.0f  // pressed: the shape squeezes, as the watch's cards
#define RG_TITLE_Y 45.0f      // the middle of the title's letters
#define RG_ICON_R 28.0f       // the icon's circle
#define RG_GAME_W 309.0f      // a game's card (variant K1)
#define RG_GAME_H 470.0f
#define RG_GAME_STEP 325.0f
#define RG_SCENE_H 350.0f     // the part of it the game shows in
#define RG_GAME_COLS 3
#define RG_FONT_SUB gl_t20       // the line under a card's title
#endif
#ifndef RG_GAME_NAME_UP
#define RG_GAME_NAME_UP RK(68)
#define RG_GAME_SUB_UP RK(32)
#define RG_LIVE_T_Y RK(137)
#define RG_LIVE_SUB_Y RK(194)
#endif

static const char *const RW_WORD[4] = {"Partly cloudy", "Sunny", "Cloudy", "Rain"};  // by WX_PARTLY ...
// The live card's glyph (gl_r_ic28 has the cards' icons): by WX_PARTLY ...
static const char *const RG_WX_GLYPH[4] = {"", "", "", ""};

static inline bool rg_live(const FaceLayer &L) {
  return L.kind == FL_CARD && L.p2 > 0.5f;
}

// What a part has beyond the watch's (gal_layer_build): the title's middle in this board's font, the icons' middle (kept
// in R.top), a live card's temperature and its line
static void rg_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (L.kind == FL_TITLE) R.mid = halo_text_mid(gl_r_title, gl_str(L.text));
  if (L.kind != FL_CARD) return;
#if defined(HALO_SQUARE)  // (a tile: the icon's middle, the name's; the live one — the sky's, its temperature by its middle)
  R.top = rg_live(L) ? halo_text_mid(gl_q_ic48, "\uf185") : halo_text_mid(gl_q_ic30, "\uf0f3");
  R.mid = halo_text_mid(gl_r_t28, gl_str(L.text));
  if (!rg_live(L)) return;
  R.txt = (char *)calloc(48, 1);
  halo_roll_init(R.roll, gl_q_n28, "0123456789", 1, 0);
#else
  R.top = halo_text_mid(gl_r_ic28, "\uf0f3");
  if (!rg_live(L)) return;
  R.txt = (char *)calloc(48, 1);
  halo_roll_init(R.roll, gl_r_n64, "0123456789", 0, 0);
#endif
  halo_roll_set(R.roll, fc_temp, false);
}

// (the cards are not items of the list here: the page is one item)
static int gal_item_of(const GalRun &, int) {
  return -1;
}

// The game a card opens (its screen has a game's layer): 0 — none
static int rg_game_of(const FaceLayer &L) {
  if (L.kind != FL_CARD || L.tap <= 0 || L.tap >= face_scr_n) return 0;
  const FaceScreen &T = face_scr[L.tap];
  for (int j = 0; j < T.n; j++)
    if (T.layers[j].kind >= FL_SNKDOTS && T.layers[j].kind <= FL_SNKISO && !T.layers[j].hidden) return T.layers[j].kind;
  return 0;
}

// Card number c in the flow
static void rg_place(int c, bool big, bool games, RgCell &o) {
  if (games) {  // tall cards, the game alive in them, three in a row (a small tall screen: two; the 7″ held tall: one)
    o.x = RG_PAD + (c % RG_GAME_COLS) * RG_GAME_STEP, o.y = RG_TOP + (c / RG_GAME_COLS) * (RG_GAME_H + RG_ROW_GAP), o.w = RG_GAME_W, o.h = RG_GAME_H;
    return;
  }
#if defined(HALO_SQUARE)
  {  // three tiles in a row, the live one as the first of them
    const float wn = (W - 2 * RG_PAD - 2 * RG_GAP_X) / 3;
    (void)big;
    o.x = RG_PAD + (c % 3) * (wn + RG_GAP_X), o.y = RG_TOP + (c / 3) * (RG_TILE_H + RG_ROW_GAP), o.w = wn, o.h = RG_TILE_H;
    return;
  }
#elif defined(HALO_COMPACT)
  {  // RG_COLS in a row; a live first card: half the width with two beside it (wide) or the whole width (tall)
    const int cols = RG_COLS;
    const float wn = (W - 2 * RG_PAD - (cols - 1) * RG_GAP_X) / cols;
    if (big && c == 0) {
      o.x = RG_PAD, o.y = RG_TOP, o.w = wn, o.h = RG_BIG_H;
    } else if (big && cols > 1 && c <= 2) {
      o.x = RG_PAD + wn + RG_GAP_X, o.y = RG_TOP + (c - 1) * (RG_ITEM_H + RG_STACK_GAP), o.w = wn, o.h = RG_ITEM_H;
    } else {
      const int k = big ? c - (cols > 1 ? 3 : 1) : c, r = k / cols, col = k % cols;
      const float y0 = big ? RG_TOP + RG_BIG_H + RG_ROW_GAP : RG_TOP;
      o.x = RG_PAD + col * (wn + RG_GAP_X), o.y = y0 + r * (RG_ITEM_H + RG_ROW_GAP), o.w = wn, o.h = RG_ITEM_H;
    }
    return;
  }
#elif defined(HALO_P4_TALL)
  {  // one column: a live first card across the top, the rest the whole width under it
    const float w1 = W - 2 * RG_PAD;
    if (big && c == 0) {
      o.x = RG_PAD, o.y = RG_TOP, o.w = w1, o.h = RG_BIG_H;
    } else {
      const int k = big ? c - 1 : c;
      const float y0 = big ? RG_TOP + RG_BIG_H + RG_ROW_GAP : RG_TOP;
      o.x = RG_PAD, o.y = y0 + k * (RG_ITEM_H + RG_STACK_GAP), o.w = w1, o.h = RG_ITEM_H;
    }
    return;
  }
#endif
  const float w2 =(W - 2 * RG_PAD - RG_GAP_X) / 2, w3 = (W - 2 * RG_PAD - 2 * RG_GAP_X) / 3;
  if (big && c == 0) {
    o.x = RG_PAD, o.y = RG_TOP, o.w = w2, o.h = RG_BIG_H;
  } else if (big && c <= 2) {
    o.x = RG_PAD + w2 + RG_GAP_X, o.y = RG_TOP + (c - 1) * (RG_ITEM_H + RG_STACK_GAP), o.w = w2, o.h = RG_ITEM_H;
  } else {
    const int k = big ? c - 3 : c, r = k / 3, col = k % 3;
    const float y0 = big ? RG_TOP + RG_BIG_H + RG_ROW_GAP : RG_TOP;
    o.x = RG_PAD + col * (w3 + RG_GAP_X), o.y = y0 + r * (RG_ITEM_H + RG_ROW_GAP), o.w = w3, o.h = RG_ITEM_H;
  }
}

// The grid of a screen from its layers (at start; in the editor — after every change): kept as it is (where it was
// scrolled) when the same cards stand in the same places
static void gal_rebuild(FaceScreen &S) {
  RgCell cell[HALO_LIST_MAX];
  int n = 0;
  bool parts = false, big = false, games = false;
  float bottom = RG_TOP;
  for (int i = 0; i < S.n; i++) games |= !S.layers[i].hidden && rg_game_of(S.layers[i]);
  for (int i = 0; i < S.n; i++) {
    const FaceLayer &L = S.layers[i];
    if (L.hidden || !fl_gallery(L.kind)) continue;
    parts = true;
    if (L.kind != FL_CARD || n >= HALO_LIST_MAX) continue;
    if (!n) big = rg_live(L);
    cell[n].layer = i, cell[n].game = rg_game_of(L);
    rg_place(n, big, games, cell[n]);
    bottom = fmaxf(bottom, cell[n].y + cell[n].h);
    n++;
  }
  if (!parts) {
    delete S.gal;
    S.gal = nullptr;
    return;
  }
  const float page = bottom + RG_END;
  GalRun *g = S.gal;
  if (g && g->cells == n && g->list.total == page && !memcmp(g->cell, cell, n * sizeof(RgCell))) return;
  if (!g) g = S.gal = new GalRun();
  g->~GalRun();  // afresh in its own place: no copy on the stack (the frame task's is small — Live, live.h)
  new (g) GalRun();
  g->edge = -1, g->eb_mode = EB_NONE, g->cells = n;
  memcpy(g->cell, cell, n * sizeof(RgCell));
  const float heights[1] = {page};
  halo_list_init(g->list, heights, 1, 0, HALO_H, false);
}

// Still and on top: the page, the presses, a tap (depth — where the screen is in lib/Halo's stack; -1 — over the face:
// a tap there opens nothing)
static void gal_update(FaceScreen &S, float dt, int depth) {
  GalRun *g = S.gal;
  if (!g) return;
  HaloFinger &f = halo_finger;
  HaloList &page = g->list;
  halo_list_update(page, dt);
  if (f.pressed) {
    g->armed = -1;
    for (int c = 0; c < g->cells && g->armed < 0; c++) {
      const RgCell &q = g->cell[c];
      const float y = q.y + page.scroll;
      if (f.x0 >= q.x && f.x0 <= q.x + q.w && f.y0 >= y && f.y0 <= y + q.h) g->armed = c;
    }
  }
  if (f.dragging) g->armed = -1;
  if (f.tap && g->armed >= 0 && depth >= 0) gl_req = &S.layers[g->cell[g->armed].layer], gl_req_depth = depth;
  for (int c = 0; c < g->cells; c++) press_update(g->press[c], c == g->armed && f.down, dt);
  if (!f.down) g->armed = -1;
  // The time at the top goes up and fades over the first 36 dp of scrolling, as the watch's
  g->tt_k = 1 - halo_clamp01((page.top - page.scroll) / (36 * DP));
  g->sc_pos.target = halo_list_progress(page);
  halo_spring_run(g->sc_pos, K_M3_DEFAULT, dt);
  if (page.moving || f.owner == &page) g->sc_idle_ms = halo_ms();
  g->sc_vis.target = halo_ms() - g->sc_idle_ms < 2000;
  halo_spring_run(g->sc_vis, K_SC_VIS, dt);
}

// Text by the middle of its letters; align: 0 — x is its left end, 2 — its right end
static void rg_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t color,
                    float alpha, const HaloItem *on, int sp = 0) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = align, it.color = color, it.on = on, it.on_black = false;
  it.x = x, it.y = y + (f.line / 2.0f - mid), it.scale = 1, it.alpha = alpha, it.spacing = sp;
}

static void rg_card(const FaceScreen &S, int i, const GalRun &g, int c, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  HaloItem *it = R.g;
  const RgCell &q = g.cell[c];
  const float x0 = q.x, y0 = q.y + g.list.scroll, h = q.h, p = halo_clamp01(g.press[c].s.x);
  const float ac = y0 < HALO_H && y0 + h > 0 ? a : 0;
  const bool live = rg_live(L) && c == 0;
  HaloItem &bg = it[0];
  bg.kind = HI_RRECT, bg.x = x0 + q.w / 2, bg.y = y0 + h / 2, bg.w = q.w, bg.h = h, bg.scale = 1;
  bg.r = RG_RADIUS + (RG_RADIUS_DOWN - RG_RADIUS) * p, bg.color = L.color2, bg.alpha = ac, bg.on_black = true;
  const float ix = x0 + RK(24) + RG_ICON_R, iy = live ? y0 + RK(26) + RG_ICON_R : y0 + h / 2;
  HaloItem &dc = it[1];
  dc.kind = HI_CIRCLE, dc.x = ix, dc.y = iy, dc.r = RG_ICON_R, dc.scale = 1, dc.color = L.color, dc.alpha = ac;
  dc.on_black = true;
  if (q.game) {  // the game alive at the top, its name under it
    dc.alpha = 0, it[2].kind = HI_TEXT, it[2].alpha = 0;
    gl_text_base(it[3], gl_r_t32, gl_str(L.text), x0 + RK(24), y0 + h - RG_GAME_NAME_UP, 1, GL_TEXT, ac, &bg);
    gl_text_base(it[4], RG_FONT_SUB, gl_str(L.sub), x0 + RK(24), y0 + h - RG_GAME_SUB_UP, 1, GL_DIM, ac, &bg);
    for (int j = 0; j < 5; j++) halo_view_commit(it[j], v);
    R.ax = bg.x, R.ay = bg.y;
    games_mini_layout((uint8_t)q.game, v, x0, y0, q.w, RG_SCENE_H, ac);
    return;
  }
  const WxDay d = wx_day(0);
#if defined(HALO_SQUARE)
  {  // a launcher's tile: the icon in its circle over the name; the live one — the sky (no circle) over the temperature
    const float cx = x0 + q.w / 2;
    dc.x = cx, dc.y = y0 + RG_ICON_Y;
    it[4].alpha = 0;
    if (live) {
      dc.alpha = 0, it[3].alpha = 0;
      gl_text_mid(it[2], gl_q_ic48, RG_WX_GLYPH[d.kind & 3], R.top, cx, y0 + RG_SKY_Y, 1, L.color, ac, &bg);
      halo_roll_layout(R.roll, cx, y0 + RG_DEG_Y, GL_TEXT, ac, &bg, v);
    } else {
      gl_text_mid(it[2], gl_q_ic30, gl_str(L.icon), R.top, cx, dc.y, 1, 0x1B1B1F, ac, &dc);
      gl_text_mid(it[3], gl_r_t28, gl_str(L.text), R.mid, cx, y0 + RG_NAME_Y, 1, GL_TEXT, ac, &bg);
    }
    for (int j = 0; j < 5; j++) halo_view_commit(it[j], v);
    R.ax = bg.x, R.ay = bg.y;
    return;
  }
#endif
  gl_text_mid(it[2], gl_r_ic28, live ? RG_WX_GLYPH[d.kind & 3] : gl_str(L.icon), R.top, ix, iy, 1, 0x1B1B1F, ac, &dc);
  if (live) {
    int n = snprintf(R.txt, 48, "%s, ", wx_city[0].name);
    snprintf(R.txt + n, 48 - n, "%s", RW_WORD[d.kind & 3]);
    R.txt[n] = tolower((unsigned char)R.txt[n]);
    gl_text_base(it[3], RG_FONT_SUB, R.txt, x0 + RK(24), y0 + RG_LIVE_SUB_Y, 1, GL_DIM, ac, &bg);
    it[4].alpha = 0;
    halo_roll_layout(R.roll, x0 + RK(24), y0 + RG_LIVE_T_Y, GL_TEXT, ac, &bg, v);
  } else {
    const float tx = x0 + RK(24) + 2 * RG_ICON_R + RK(18);
    gl_text_base(it[3], gl_r_t28, gl_str(L.text), tx, y0 + RK(52), 1, GL_TEXT, ac, &bg);
    gl_text_base(it[4], RG_FONT_SUB, gl_str(L.sub), tx, y0 + RK(84), 1, GL_DIM, ac, &bg);
  }
  for (int j = 0; j < 5; j++) halo_view_commit(it[j], v);
  R.ax = bg.x, R.ay = bg.y;
}

// A part of the gallery through the view v (a = 0 — hidden)
static void gal_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  HaloItem *it = R.g;
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  if (!it) return;
  const GalRun *g = S.gal;
  int c = -1;
  if (g)
    for (int k = 0; k < g->cells; k++)
      if (g->cell[k].layer == i) c = k;
  if (!g || (L.kind == FL_CARD && c < 0) || L.kind == FL_EDGE) {
    for (int j = 0; j < 5; j++) it[j].alpha = 0, halo_view_commit(it[j], v);
    return;
  }
  switch (L.kind) {
    case FL_TITLE:
      rg_text(it[0], gl_r_title, gl_str(L.text), R.mid, 0, RG_PAD, g->list.scroll + RG_TITLE_Y, L.color, a, &halo_black);
      it[0].on_black = true;
      halo_view_commit(it[0], v);
      R.ax = RG_PAD, R.ay = g->list.scroll + RG_TITLE_Y;
      break;
    case FL_CARD:
      rg_card(S, i, *g, c, a, v);
      break;
    case FL_SCROLL: {  // at the right edge, while the page moves; nothing to scroll — none
      const float track = HALO_H - RG_TOP - RG_END, size = track * constrain(halo_list_visible(g->list), 0.2f, 1.0f);
      const float y0 = RG_TOP + (track - size) * halo_clamp01(g->sc_pos.x), x = W - RK(11.0f);
      const float al = g->list.total > HALO_H && g->sc_vis.x > 0.01f ? g->sc_vis.x * a : 0;
      for (int j = 0; j < 2; j++) {
        HaloItem &s = it[j];
        s.kind = HI_RRECT, s.x = x, s.y = j ? y0 + size / 2 : RG_TOP + track / 2, s.w = RK(6), s.h = j ? size : track, s.r = RK(3);
        s.scale = 1, s.color = j ? L.color : L.color2, s.alpha = al, s.on_black = !j;
        halo_view_commit(s, v);
      }
      it[2].alpha = 0, halo_view_commit(it[2], v);
      R.ax = x, R.ay = RG_TOP + track / 2;
      break;
    }
  }
  for (int j = 0; j < 5; j++) box_add(R.box, it[j]);
}

// A game's card: over its grey, the game alive with the card's top corners rounded off
static void rg_game_draw(const FaceScreen &S, int i) {
  const GalRun *g = S.gal;
  if (!g) return;
  for (int c = 0; c < g->cells; c++) {
    if (g->cell[c].layer != i || !g->cell[c].game) continue;
    games_mini_draw((uint8_t)g->cell[c].game);
    const HaloItem &bg = S.run[i].g[0];
    if (bg.alpha <= 0.004f) return;
    const float x0 = bg.x - bg.w / 2 * bg.scale, x1 = bg.x + bg.w / 2 * bg.scale, y0 = bg.y - bg.h / 2 * bg.scale, r = RG_RADIUS * bg.scale;
    for (int y = max(hb.y, halo_floor(y0)); y <= min(hb.y + hb.rows - 1, halo_floor(y0 + r)); y++) {
      const float d = y0 + r - (y + 0.5f);
      if (d <= 0) break;
      const float in = r - halo_sqrt(fmaxf(0, r * r - d * d));
      uint16_t *row = halo_row(y);
      for (int k = 0; k < 2; k++) {
        const float a = k ? x1 - in : x0, b = k ? x1 : x0 + in;
        for (int x = max(hb.x0, halo_floor(a)); x <= min(hb.x1, halo_floor(b)); x++) {
          const float cov = halo_clamp01(fminf(b, (float)x + 1) - fmaxf(a, (float)x));
          if (cov >= 0.99f) row[x] = 0;
          else if (cov > 0) halo_blend(row + x, 0, 0, 0, (int)(cov * 256));
        }
      }
    }
    return;
  }
}
