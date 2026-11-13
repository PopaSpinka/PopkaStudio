// gallery.h — the gallery of projects/HaloWatch (HaloWatch.ino there: menu_*), one to one, made of layers: the title
// at the top of the list, the cards, the button at the bottom edge ("More") and the scroll indicator; the clock along
// the top edge (wxscreen.h) scrolls away on it, as on HaloWatch.
//
// On HaloWatch the gallery is one screen with five cards. Here each part is a layer of its own (a kind of FaceLayer),
// and a screen with any title or card is a list: its titles and cards, in the order of its layers, are the items of
// HaloWatch's list (halo_list: a title 64 px tall, a card 104 px; a title first — the list's header, as "Gallery"),
// with everything HaloWatch does with it: the cards shrink and fade at the edges of the circle, inertia and rubber at
// the ends, a press squeezes a card's corners, the clock along the top edge scrolls away, the scroll indicator shows
// while the list moves (and fades 2 s later), the button at the bottom edge grows as the list reaches its end. A tap on
// a card or on the button opens its screen (face.h: lib/Halo's Android 16 transition; back — a swipe from the left
// edge); one without a link opens a screen with its title in the middle, as HaloWatch's Alarm and More do.
//
// The text is in HaloWatch's fonts (t_title, t24, t20, ic24 there; gl_* here — assets/halo.conf has them always): any
// Latin text; the icons — Font Awesome, the ones gl_ic24 has. The places and sizes are HaloWatch's: a part of the
// gallery does not move (dx, dy are not used) — the list places it.
// Included from face.h after the layer structures, before wxscreen.h (its clock along the top edge uses gal_arc).

#define DP 2.0f  // 1 dp of Wear OS ≈ 2 px of this screen
#define GL_TEXT 0xFFF8EE  // warm white
#define GL_DIM 0x9E9E9E   // the line under a card's title
#define GL_HEAD_H 64.0f   // a title
#define GL_ITEM_H 104.0f  // a card with a line under its title: 52 dp
#define GL_ITEM_W 410.0f
// The button at the bottom edge (EdgeButton): grows from below as the list reaches its end — its height follows the
// scroll, a jump of more than 16 dp without scrolling goes by a 400/1 spring; under 30 dp its background fades, under
// 38 dp its text (EdgeButton.kt). The list's last card ends at 333 at the end of the list, 20 px above the button
#define EB_FULL 107.0f  // full height
#define EB_R 227.0f     // the bottom follows the screen's circle 3 dp in
#define EB_W 300.0f
// How it stands by the number of cards (5 Nov 2026, the user's wish): one card or none — no button at all (the list
// ends as without it, nothing to tap); two — full height from the start, still (the list doesn't scroll); three and
// more — HaloWatch's: it grows as the list reaches its end
enum : uint8_t { EB_NONE, EB_STILL, EB_GROWS };

static HaloSpringK K_M3_DEFAULT, K_PRESS, K_RELEASE, K_SC_VIS, K_EB;
static float gl_mid_icon;  // all icons by one middle (the bell's, as on WearKit)

struct Press {  // a press of a card (as Wear OS buttons: RoundButton.kt)
  HaloSpring s;
  bool going_down;
};
// 0 — up, 1 — pressed. Pressing — a very fast spring (fast x 9), letting go — a slow one with a bounce. Even a short
// tap plays the press to at least 0.75 before letting go: a press is always seen
static void press_update(Press &p, bool held, float dt, const HaloSpringK *k_down = nullptr) {
  if (held) p.going_down = true;
  else if (p.going_down && p.s.x >= 0.75f) p.going_down = false;
  p.s.target = p.going_down ? 1 : 0;
  halo_spring_run(p.s, p.going_down ? (k_down ? *k_down : K_PRESS) : K_RELEASE, dt);
}

// A screen's list and what goes with it (HaloWatch's menu, tt_k, sc_*, eb_*)
#ifndef HALO_ROUND
struct RgCell {  // gallery_rect.h: a card's place on the page
  int layer;
  float x, y, w, h;
  int game;       // the game this card opens and shows alive (its FL_SNK*; 0 — an ordinary card)
};
#endif
struct GalRun {
  HaloList list;
  int8_t item[HALO_LIST_MAX];  // the layer of each item
  Press press[HALO_LIST_MAX];
  int armed = -1;              // the card pressed now (its item; -1 — none)
  int edge = -1;               // the button at the bottom edge a tap opens (the topmost; -1 — none)
  bool eb_armed = false;
  float tt_k = 1;              // the clock along the top edge: 1 — in place, 0 — scrolled away (ScrollAway)
  HaloSpring sc_pos, sc_vis;   // the scroll indicator: where (catches up in 500 ms), shown (fades 2 s after the list stops)
  uint32_t sc_idle_ms = 0;
#ifndef HALO_ROUND
  RgCell cell[HALO_LIST_MAX];  // the cards on the page; press[] — by the same number
  int cells = 0;
#endif
  HaloSpring eb_h;             // the button's height now, px
  uint8_t eb_mode = EB_GROWS;  // by the number of cards: EB_NONE, EB_STILL, EB_GROWS
  Press eb_press;
};

// About's live card (device.h): made, laid out and drawn there — by its place in this list
struct FaceRun;
static void dv_run_make(FaceRun &R);
static void dc_layout(const FaceScreen &S, int i, float a, const HaloView &v);
static void dc_draw(const FaceScreen &S, int i);

// A tap opened something: this layer's link, from the screen at this depth of lib/Halo's stack (face.h opens it in
// the next frame, as HaloWatch does: open_req)
static const FaceLayer *gl_req;
static int gl_req_depth;

static const char *gl_str(const char *s) {
  return s ? s : "";
}

// Text by the middle of its letters (mid — halo_text_mid) at (x, y), as WearKit places text; on — what it lies on
static void gl_text_mid(HaloItem &it, const HaloFont &f, const char *s, float mid, float x, float y, float sc,
                        uint32_t color, float alpha, const HaloItem *on) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = 1, it.color = color, it.on_black = false, it.on = on;
  it.x = x, it.y = y + (f.line / 2.0f - mid) * sc, it.scale = sc, it.alpha = alpha;
}
// Text from the left edge x, its baseline at y
static void gl_text_base(HaloItem &it, const HaloFont &f, const char *s, float x, float y, float sc, uint32_t color,
                         float alpha, const HaloItem *on) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = 0, it.color = color, it.on_black = false, it.on = on;
  it.x = x, it.y = y + (f.line / 2.0f - f.ascent) * sc, it.scale = sc, it.alpha = alpha;
}

#ifndef HALO_ROUND
#include "gallery_rect.h"  // this screen's own places: gal_rebuild, gal_update, gal_layout
#endif

// A part of the gallery from scratch (at start; in the editor — after a change of its text)
static void gal_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (L.kind == FL_DEVCARD) return dv_run_make(R);
  R.g = new HaloItem[5]();
  if (L.kind == FL_TITLE) R.mid = halo_text_mid(gl_title, gl_str(L.text));
  if (L.kind == FL_EDGE) R.mid = halo_text_mid(gl_t24, gl_str(L.text));
#ifndef HALO_ROUND
  rg_layer_build(S, i);
#endif
}

#ifdef HALO_ROUND
// The list of a screen from its layers (at start; in the editor — after every change): made anew when other layers
// make it, kept as it is (where it was scrolled) when the same ones do. A screen with no part of the gallery has none
static void gal_rebuild(FaceScreen &S) {
  int8_t item[HALO_LIST_MAX];
  float heights[HALO_LIST_MAX];
  int n = 0, edge = -1, cards = 0;
  bool parts = false;
  for (int i = 0; i < S.n; i++) {
    const FaceLayer &L = S.layers[i];
    if (L.hidden || !fl_gallery(L.kind)) continue;
    parts = true;
    if (L.kind == FL_EDGE) edge = i;
    if (L.kind == FL_CARD || L.kind == FL_DEVCARD) cards++;
    if ((L.kind == FL_TITLE || L.kind == FL_CARD || L.kind == FL_DEVCARD) && n < HALO_LIST_MAX)
      item[n] = i, heights[n++] = L.kind == FL_TITLE ? GL_HEAD_H : GL_ITEM_H;
  }
  if (!parts) {
    delete S.gal;
    S.gal = nullptr;
    return;
  }
  // The bottom of the last card at the end of the list: with the button — the list's own gap (8 px) above it, as
  // between the cards (4 Nov, the user's wish; HaloWatch has 20 px: 333); without it — HaloWatch's 333
  const uint8_t mode = edge < 0 || cards <= 1 ? EB_NONE : cards == 2 ? EB_STILL : EB_GROWS;
  if (mode == EB_NONE) edge = -1;  // (no button: nothing to tap)
  const float end = edge >= 0 ? CX + EB_R - EB_FULL - 8 : 333;
  GalRun *g = S.gal;
  bool same = g && g->eb_mode == mode && g->list.n == n && !memcmp(g->item, item, n) && (!n || g->list.bottom == end);
  for (int k = 0; same && k < n; k++) same = g->list.h[k] == heights[k];
  if (!g) g = S.gal = new GalRun();
  g->edge = edge;
  if (same) return;
  g->~GalRun();  // afresh in its own place: no copy on the stack (the frame task's is small — Live, live.h)
  new (g) GalRun();
  g->edge = edge, g->eb_mode = mode;
  memcpy(g->item, item, n);
  if (n) halo_list_init(g->list, heights, n, 64, end, S.layers[item[0]].kind == FL_TITLE);
  // Still (two cards): the list stands where it ends — 8 px above the button — and doesn't scroll. From HaloWatch's
  // top (64) a title and two cards end 7 px into the button, and only a scroll to the end set the gap right
  if (n && mode == EB_STILL && halo_list_min(g->list) < g->list.top)
    g->list.top = halo_list_min(g->list), halo_list_reset(g->list);
  // still — full height (and nothing scrolls: the same); none — 0; growing — from the list's place (0 at its top)
  if (mode == EB_STILL || (mode == EB_GROWS && !n)) halo_spring_set(g->eb_h, EB_FULL);
}
#endif

// The button's height for the list as it is now (its mode: gal_rebuild)
static float gal_eb_height(const GalRun &g) {
  if (g.eb_mode == EB_NONE) return 0;
  if (g.eb_mode == EB_STILL || !g.list.n) return EB_FULL;
  return halo_clamp01(1 - (g.list.scroll - halo_list_min(g.list)) / EB_FULL) * EB_FULL;
}

static void gal_build() {
  K_M3_DEFAULT = halo_spring(HALO_M3_DEFAULT);
  K_PRESS = halo_spring(0.222f / 3, 0.70f);  // pressing: 3 times faster than expressive fast (stiffness x 9)
  K_RELEASE = halo_spring(HALO_M3X_SLOW);    // letting go — slowly, with a light bounce
  K_SC_VIS = halo_spring(halo_stiff(400), 1);
  K_EB = halo_spring(halo_stiff(400), 1);
  gl_mid_icon = halo_text_mid(gl_ic24, "");
  for (int k = 0; k < face_scr_n; k++) gal_rebuild(face_scr[k]);
}

// Opened (the gallery: arrived at the face — next time from the top, as at start)
static void gal_enter(FaceScreen &S) {
  GalRun *g = S.gal;
  if (!g) return;
  if (g->list.n) halo_list_reset(g->list);
  g->armed = -1, g->tt_k = 1;
  halo_spring_set(g->sc_pos, 0), halo_spring_set(g->sc_vis, 0);
  for (auto &p : g->press) halo_spring_set(p.s, 0), p.going_down = false;
  halo_spring_set(g->eb_h, g->eb_mode == EB_GROWS ? (g->list.n ? 0 : EB_FULL) : gal_eb_height(*g));  // (growing: from 0)
  halo_spring_set(g->eb_press.s, 0);
  g->eb_press.going_down = g->eb_armed = false;
}

#ifdef HALO_ROUND
// Still and on top: the list, the presses, a tap (depth — where the screen is in lib/Halo's stack; -1 — over the face:
// a tap there opens nothing)
static void gal_update(FaceScreen &S, float dt, int depth) {
  GalRun *g = S.gal;
  if (!g) return;
  HaloFinger &f = halo_finger;
  HaloList &menu = g->list;
  const bool list = menu.n > 0;
  if (list) {
    halo_list_update(menu, dt);
    // The button: full height at the end of the list, lower — by how far it is from the end (in the rubber past the
    // end — full height)
    g->eb_h.target = gal_eb_height(*g);
    const bool scrolling = f.owner == &menu || menu.flinging || menu.back_on;
    if (g->eb_mode != EB_GROWS || scrolling || fabsf(g->eb_h.target - g->eb_h.x) <= 16 * DP) g->eb_h.x = g->eb_h.target, g->eb_h.v = 0;
    halo_spring_run(g->eb_h, K_EB, dt);
  }
  // A press on a card (or on the button — it is over the list): the press plays; let go — open
  if (f.pressed) {
    const int i = list ? halo_list_hit(menu, f.y0) : -1;
    g->armed = i >= 0 && (S.layers[g->item[i]].kind == FL_CARD || S.layers[g->item[i]].kind == FL_DEVCARD) ? i : -1;
    if (g->armed >= 0 && fabsf(f.x0 - CX) > GL_ITEM_W / 2 * menu.fx[i].s) g->armed = -1;
    g->eb_armed = g->edge >= 0 && g->eb_h.x > 60 && f.y0 > CX + EB_R - g->eb_h.x && fabsf(f.x0 - CX) < EB_W / 2;
    if (g->eb_armed) g->armed = -1;
  }
  if (f.dragging) g->armed = -1, g->eb_armed = false;
  if (f.tap && g->armed >= 0 && depth >= 0) gl_req = &S.layers[g->item[g->armed]], gl_req_depth = depth;
  if (f.tap && g->eb_armed && depth >= 0) gl_req = &S.layers[g->edge], gl_req_depth = depth;
  for (int i = 0; i < menu.n; i++) press_update(g->press[i], i == g->armed && f.down, dt);
  press_update(g->eb_press, g->eb_armed && f.down, dt);
  if (!f.down) g->armed = -1, g->eb_armed = false;
  // The clock along the top edge goes up and shrinks over the first 36 dp of scrolling (ScrollAway: 24 dp, to 0.5)
  if (list) g->tt_k = 1 - halo_clamp01((menu.top - menu.scroll) / (36 * DP)), g->sc_pos.target = halo_list_progress(menu);
  halo_spring_run(g->sc_pos, K_M3_DEFAULT, dt);
  if (menu.moving || f.owner == &menu) g->sc_idle_ms = halo_ms();
  g->sc_vis.target = halo_ms() - g->sc_idle_ms < 2000;
  halo_spring_run(g->sc_vis, K_SC_VIS, dt);
}

static int gal_item_of(const GalRun &g, int layer) {
  for (int k = 0; k < g.list.n; k++)
    if (g.item[k] == layer) return k;
  return -1;
}

// A part of the gallery through the view v (a = 0 — hidden). HaloWatch's expressions as they are
static void gal_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (L.kind == FL_DEVCARD) return dc_layout(S, i, a, v);
  HaloItem *it = R.g;
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  if (!it) return;
  const GalRun *g = S.gal;
  const int k = g ? gal_item_of(*g, i) : -1;
  if (!g || ((L.kind == FL_TITLE || L.kind == FL_CARD) && k < 0)) {
    for (int j = 0; j < 5; j++) it[j].alpha = 0, halo_view_commit(it[j], v);
    return;
  }
  switch (L.kind) {
    case FL_TITLE: {  // (a header does not shrink: its scale is 1)
      const HaloListFx &h = g->list.fx[k];
      gl_text_mid(it[0], gl_title, gl_str(L.text), R.mid, CX, h.top + GL_HEAD_H * h.s / 2, h.s, L.color, h.a_bg * a,
                  &halo_black);
      it[0].on_black = true;
      halo_view_commit(it[0], v);
      R.ax = CX, R.ay = h.top + GL_HEAD_H * h.s / 2;
      break;
    }
    case FL_CARD: {
      const HaloListFx &f = g->list.fx[k];
      const bool vis = halo_list_item_visible(g->list, k);
      const float s = f.s, h = GL_ITEM_H * s, w = GL_ITEM_W * s, x0 = CX - w / 2, y0 = f.top;
      const float p = halo_clamp01(g->press[k].s.x), ac = vis ? f.a_c * a : 0, ix = x0 + 56 * s, iy = y0 + h / 2;
      // The card: pressed — less rounded (the shape "squeezes": 26 dp -> 18 dp)
      HaloItem &bg = it[0];
      bg.kind = HI_RRECT, bg.x = CX, bg.y = y0 + h / 2, bg.w = GL_ITEM_W, bg.h = GL_ITEM_H, bg.scale = s;
      bg.r = 52 + (36 - 52) * p, bg.color = L.color2, bg.alpha = vis ? f.a_bg * a : 0, bg.on_black = true;
      // The icon's circle — opaque over the card, the icon and the text — with the content's transparency
      HaloItem &dc = it[1];
      dc.kind = HI_CIRCLE, dc.x = ix, dc.y = iy, dc.r = 32, dc.scale = s, dc.color = L.color, dc.alpha = ac;
      dc.on_black = true;
      gl_text_mid(it[2], gl_ic24, gl_str(L.icon), gl_mid_icon, ix, iy, s, 0x1B1B1F, ac, &dc);
      gl_text_base(it[3], gl_t24, gl_str(L.text), x0 + 104 * s, y0 + 46 * s, s, GL_TEXT, ac, &bg);
      gl_text_base(it[4], gl_t20, gl_str(L.sub), x0 + 104 * s, y0 + 80 * s, s, GL_DIM, ac, &bg);
      for (int j = 0; j < 5; j++) halo_view_commit(it[j], v);
      R.ax = CX, R.ay = y0 + h / 2;
      break;
    }
    case FL_EDGE: {  // the background fades under 30 dp of height, the text under 38 dp; a press squeezes the top corners
      const float eh = g->eb_h.x, ep = i == g->edge ? halo_clamp01(g->eb_press.s.x) : 0, e_top = CX + EB_R - eh;
      const float e_bg = halo_clamp01((eh - 4 * DP) / (26 * DP)), e_txt = halo_clamp01((eh - 30 * DP) / (8 * DP));
      HaloItem &e = it[0];
      e.kind = HI_EDGE, e.x = CX, e.y = CX, e.r = EB_R, e.h = eh, e.w = EB_W, e.v = 48 + (32 - 48) * ep;
      e.scale = 1, e.color = L.color, e.alpha = eh > 1 ? e_bg * a : 0, e.on_black = true;
      gl_text_mid(it[1], gl_t24, gl_str(L.text), R.mid, CX, e_top + fminf(eh, EB_FULL) * 0.4f, 1, GL_TEXT,
                  eh > 1 ? e_txt * a : 0, &e);
      halo_view_commit(it[0], v), halo_view_commit(it[1], v);
      R.ax = CX, R.ay = CX + EB_R - EB_FULL / 2;
      break;
    }
    case FL_SCROLL: {
      // On the right (ScrollIndicator): a track of 50 dp along the arc, the thumb 0.3..0.7 of it, gaps of 3 dp, 6 dp
      // thick, round ends
      const float r = 233 - 2 * DP - 3 * DP, len = 50 * DP / r, gap = (6 + 3) * DP / r;
      const float a0 = HALO_PI / 2 - len / 2, size = len * constrain(halo_list_visible(g->list), 0.3f, 0.7f);
      const float t0 = a0 + (len - size) * halo_clamp01(g->sc_pos.x), t1 = t0 + size;
      const float al = g->sc_vis.x > 0.01f ? g->sc_vis.x : 0;
      const float from[3] = {a0, t1 + gap, t0}, to[3] = {t0 - gap, a0 + len, t1};  // track before, track after, thumb
      for (int j = 0; j < 3; j++) {
        HaloItem &s = it[j];
        s.kind = HI_ARC, s.x = CX, s.y = CX, s.r = r, s.w = 6 * DP, s.scale = 1, s.a0 = from[j], s.a1 = to[j];
        s.color = j == 2 ? L.color : L.color2, s.on_black = true, s.alpha = from[j] < to[j] ? al * a : 0;
        halo_view_commit(s, v);
      }
      R.ax = CX + r, R.ay = CX;
      break;
    }
  }
  for (int j = 0; j < 5; j++) box_add(R.box, it[j]);
}
#endif

static void gal_draw(const FaceScreen &S, int i) {
  if (S.layers[i].kind == FL_DEVCARD) return dc_draw(S, i);
  const HaloItem *it = S.run[i].g;
  if (!it) return;
  const int n = S.layers[i].kind == FL_CARD ? 5 : S.layers[i].kind == FL_SCROLL ? 3 : S.layers[i].kind == FL_EDGE ? 2 : 1;
  for (int j = 0; j < n; j++) halo_item_draw(it[j]);
#ifndef HALO_ROUND
  if (S.run[i].txt) halo_roll_draw(S.run[i].roll);  // (the live card: its temperature)
  if (S.layers[i].kind == FL_CARD) rg_game_draw(S, i);  // (a game's card: the game, alive)
#endif
}

// The clock along the top edge on a screen with a list (wxscreen.h: wx_layout_static): HaloWatch's gallery's — it goes
// up 24 dp as the list scrolls, shrinks and fades to 0.5, and is gone (scaled from its top middle)
static void gal_arc(const GalRun &g, FaceRun &R, uint32_t color, float a, const HaloView &v) {
  HaloItem &c = R.pill;
  const float tt_k = g.tt_k, k = 0.5f + 0.5f * tt_k, top = R.ty - 24 * DP * (1 - tt_k), left = CX - (CX - R.tx) * k;
  c.kind = HI_MASK, c.mask = &R.arc, c.color = color, c.on_black = true, c.on = &halo_black;
  c.x = left + R.arc.w * k / 2, c.y = top + R.arc.h * k / 2, c.scale = k;
  c.alpha = tt_k > 0.02f && R.arc.bits ? k * a : 0;
  halo_view_commit(c, v);
}
