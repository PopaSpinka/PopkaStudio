// wxscreen.h — the Weather screen of projects/HaloWatch (weather.h there), one to one, made of layers: the clock along
// the top edge, the city, the current weather icon, the temperature, today's high and low, the pill with the next three
// hours and the page dots. On HaloWatch they are one screen; here each is a layer of its own (a kind of FaceLayer) —
// moved by (dx, dy) from its place on HaloWatch, with its own size and colors, and any of them on any screen.
//
// A screen with any of the city, the icon, the temperature, the high/low or the hourly pill pages sideways through the
// cities picked in the editor (weather.h: wx_pages, one to six), exactly as HaloWatch's Weather does: the leaving page
// shrinks to 0.55 toward its edge and dims to 0.5, past 10 % or a flick faster than 400 px/s — to the neighbour, a
// 2000/1 spring; page 0 is the first city (the face's pill shows it too). One city — no dots, nothing to page.
// The icon and the high/low sit around the temperature's digits as on HaloWatch — by what is really drawn (the row
// rules there), their own (dx, dy) on top; the temperature is the first one on the screen.
// Positions and sizes — HaloWatch's constants, copied as they are (the screen is 1.2x the mockup: WX_SCALE).
// Included from face.h after the layer structures and gallery.h.

#define WX_WHITE 0xF9F9F9
#define WX_SCALE 1.2f
static inline float wsc(float v) {
  return CX + (v - CX) * WX_SCALE;
}
#define WX_LIFT 12.0f
#define WX_CITY_X wsc(234.6f)
#define WX_CITY_Y (wsc(115.4f) - WX_LIFT)
#define WX_ROW_GAP (27.0f * WX_SCALE)
#define WX_ICON_NEAR 0.7f
#define WX_NOW_Y (wsc(195.95f) - WX_LIFT)
#define WX_PILL_X wsc(232.7f)
#define WX_PILL_Y (wsc(314.45f) - WX_LIFT)
#define WX_PILL_W (293.2f * WX_SCALE)
#define WX_PILL_H (136.9f * WX_SCALE)
#define WX_HICON_Y (wsc(299.85f) - WX_LIFT)
#define WX_HTIME_Y (wsc(346.1f) - WX_LIFT)
static const float WX_COL[WX_HOURS] = {147.8f, 233.3f, 317.65f};
#define WX_FEELS_Y 420.0f  // what it feels like: between the hourly pill (its bottom 402) and the page dots (444)
#define TT_R 219.0f  // the clock along the top edge: radius of the middle of its letters (HaloWatch.ino)
#define DOTS_R 217.0f

static char wx_now_s[WX_MAX][8], wx_hi_s[WX_MAX][8], wx_lo_s[WX_MAX][8], wx_feels_s[WX_MAX][20];
static char wx_hour[WX_HOURS][8];  // "7AM" ... — the same on every page
static char wx_time[8] = "--:--";  // the clock along the top edge: "12:59" (12 hours, no leading zero)
static HaloSpringK K_PAGER;

// Strings of all pages; hours — this one and the next two: "7AM"
static void wx_fill(bool animate = false) {
  for (int p = 0; p < wx_pages; p++) {
    const WxDay d = wx_day(p);
    wx_deg(wx_now_s[p], sizeof(wx_now_s[p]), d.now);
    wx_num(wx_hi_s[p], sizeof(wx_hi_s[p]), d.hi);
    wx_num(wx_lo_s[p], sizeof(wx_lo_s[p]), d.lo);
    if (d.feels == WX_NONE) snprintf(wx_feels_s[p], sizeof(wx_feels_s[p]), "Feels like --°");
    else snprintf(wx_feels_s[p], sizeof(wx_feels_s[p]), "Feels like %d°", d.feels);
  }
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++) {
      FaceRun &R = face_scr[k].run[i];
      if (face_scr[k].layers[i].kind == FL_WXTEMP && R.pg)
        for (int p = 0; p < wx_pages; p++) halo_roll_set(*R.pg[p].roll, wx_now_s[p], animate);
    }
  int h = 12;
  if (halo_clock_valid()) {
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    h = t.tm_hour;
  }
  for (int i = 0; i < WX_HOURS; i++) {
    const int hh = (h + i) % 24;
    if (fc_h24) snprintf(wx_hour[i], sizeof(wx_hour[i]), "%02d:00", hh);
    else snprintf(wx_hour[i], sizeof(wx_hour[i]), "%d%s", hh % 12 ? hh % 12 : 12, hh < 12 ? "AM" : "PM");
  }
}

// ---------- HaloWatch's helpers, as they are ----------
static void wx_digit_rows(const HaloFont &f, float &top, float &bottom) {
  top = 1e9f, bottom = -1e9f;
  for (uint32_t d : {(uint32_t)'1', (uint32_t)'4'}) {
    const HaloMask *g = halo_glyph(f, d);
    if (!g) continue;
    for (int r = 0; r < g->h; r++)
      if (g->ext[2 * r] <= g->ext[2 * r + 1]) top = fminf(top, g->oy + r), bottom = fmaxf(bottom, g->oy + r + 1);
  }
}

static void wx_image_ink(const HaloImage &im, float &x0, float &x1, float &y0, float &y1) {
  x0 = y0 = 1e9f, x1 = y1 = -1e9f;
  for (int r = 0; r < im.h; r++)
    if (im.ext[2 * r] <= im.ext[2 * r + 1])
      x0 = fminf(x0, im.ext[2 * r]), x1 = fmaxf(x1, im.ext[2 * r + 1] + 1), y0 = fminf(y0, r), y1 = fmaxf(y1, r + 1);
}

static void image_at(HaloItem &it, const HaloImage &im, float x, float y, float s, float a) {
  it.kind = HI_IMAGE, it.image = &im, it.x = x, it.y = y, it.scale = s, it.alpha = a, it.on_black = false;
}

// (sp — letter spacing, as Halo draws it: after every letter but the last; HaloWatch has none)
static bool wx_ink(const HaloFont &f, const char *s, float &ink0, float &ink1, float &pen, int sp = 0) {
  pen = 0, ink0 = 1e9f, ink1 = -1e9f;
  while (*s) {
    const HaloMask *g = halo_glyph(f, halo_utf8_next(s));
    if (!g) continue;
    ink0 = fminf(ink0, pen + g->ox), ink1 = fmaxf(ink1, pen + g->ox + g->w), pen += g->adv + sp;
  }
  if (ink0 <= ink1) pen -= sp;
  return ink0 <= ink1;
}
static float wx_ink_w(const HaloFont &f, const char *s, int sp = 0) {
  float i0, i1, pen;
  return wx_ink(f, s, i0, i1, pen, sp) ? i1 - i0 : 0;
}
static float wx_ink_x(const HaloFont &f, const char *s, int align, float x, float sc, int sp = 0) {
  float ink0, ink1, pen;
  if (!wx_ink(f, s, ink0, ink1, pen, sp)) return x;
  return align == 2 ? x + (pen - ink1) * sc : x + (pen / 2 - (ink0 + ink1) / 2) * sc;
}

static void wx_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, float sc,
                    uint32_t color, float alpha, const HaloItem *on, int sp = 0) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = align, it.color = color, it.on = on, it.on_black = true;
  it.x = wx_ink_x(f, s, align, x, sc, sp), it.y = y + (f.line / 2.0f - mid) * sc, it.scale = sc, it.alpha = alpha;
  it.spacing = sp;
}

// ---------- Layers ----------
// Made anew (at start; in the editor — after a change of its font, spacing or size): what it needs on each page
static void wx_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (!L.font && L.kind != FL_WXICON && L.kind != FL_DOTS) return;
  if (fl_paged(L.kind) || L.kind == FL_DOTS) R.pg = new WxRun[WX_MAX]();  // (room for every page the editor may give)
  switch (L.kind) {
    case FL_ARC:
      R.mid = halo_text_mid(*L.font, "0123456789");
      halo_text_arc(R.arc, *L.font, wx_time, R.mid, L.p, 0, false, L.spacing, &R.tx, &R.ty);
      break;
    case FL_CITY:
      R.mid = halo_text_mid(*L.font, "San Francisco");
      break;
    case FL_WXTEMP:
      wx_digit_rows(*L.font, R.top, R.bottom);
      for (int p = 0; p < WX_MAX; p++) {
        HaloRoll &r = *(R.pg[p].roll = new HaloRoll());
        r.tabular = true;
        halo_roll_init(r, *L.font, "0123456789", 0, L.spacing);
        halo_roll_set(r, wx_now_s[p], false);
      }
      break;
    case FL_WXHILO:
      R.mid = halo_text_mid(*L.font, "0123456789");
      wx_digit_rows(*L.font, R.top, R.bottom);
      break;
    case FL_HOURLY:
      R.mid = halo_text_mid(*L.font, "7AM");
      break;
    case FL_FEELS:
      R.mid = halo_text_mid(*L.font, "Feels like 0123456789");
      break;
  }
}

static void wx_layer_free(FaceRun &R) {
  if (R.pg) {
    for (int p = 0; p < WX_MAX; p++) delete R.pg[p].roll;
    delete[] R.pg;
  }
  free((void *)R.arc.bits), free((void *)R.arc.ext);
  delete[] R.g;
}

// Every frame: the clock along the top edge (a new minute — baked along the arc again), the temperatures' rolls
static int wx_clock_h = -1, wx_clock_m = -1;  // the minute baked along the arc (-1: bake again)
static void wx_tick(float dt) {
  static uint32_t checked;
  if (halo_ms() - checked >= 250 && halo_clock_valid()) {
    checked = halo_ms();
    const time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    if (t.tm_min != wx_clock_m || t.tm_hour != wx_clock_h) {
      if (t.tm_hour != wx_clock_h && wx_clock_h >= 0) wx_fill(true);  // a new hour: the hourly pill moves on
      wx_clock_h = t.tm_hour, wx_clock_m = t.tm_min;
      if (fc_h24) snprintf(wx_time, sizeof(wx_time), "%02d:%02d", wx_clock_h, wx_clock_m);
      else snprintf(wx_time, sizeof(wx_time), "%d:%02d", wx_clock_h % 12 ? wx_clock_h % 12 : 12, wx_clock_m);  // 12 hours, as in the US
      for (int k = 0; k < face_scr_n; k++)
        for (int i = 0; i < face_scr[k].n; i++) {
          const FaceLayer &L = face_scr[k].layers[i];
          FaceRun &R = face_scr[k].run[i];
          if (L.kind == FL_ARC && L.font) halo_text_arc(R.arc, *L.font, wx_time, R.mid, L.p, 0, false, L.spacing, &R.tx, &R.ty);
        }
    }
  }
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++)
      if (face_scr[k].layers[i].kind == FL_WXTEMP && face_scr[k].run[i].pg)
        for (int p = 0; p < wx_pages; p++) halo_roll_update(*face_scr[k].run[i].pg[p].roll, dt);
}

// The row "icon — temperature — high/low" on a page: by the screen's first temperature (HaloWatch's rules: the
// temperature's digits centered by their cells, two lines along the top and the bottom of its digits)
struct WxRow {
  float now_line, anchor, top, bottom, ink_l, ink_r, degree;
};
static WxRow wx_row(const FaceScreen &S, int page) {
  for (int i = 0; i < S.n; i++) {
    const FaceLayer &T = S.layers[i];
    const FaceRun &RT = S.run[i];
    if (T.kind != FL_WXTEMP || !RT.pg || !RT.pg[page].roll) continue;
    const HaloRoll &r = *RT.pg[page].roll;
    int nd = 0;
    while (wx_now_s[page][nd] >= '0' && wx_now_s[page][nd] <= '9') nd++;
    WxRow w;
    w.now_line = halo_floor(WX_NOW_Y + T.dy - r.mid + 0.5f);
    w.anchor = CX + T.dx - nd * r.cell / 2, w.top = w.now_line + RT.top, w.bottom = w.now_line + RT.bottom;
    float t0 = 0, t1 = 0, d0 = 0, d1 = 0;
    halo_roll_ink(r, t0, t1);
    halo_roll_ink(r, d0, d1, 0, nd);
    w.ink_l = w.anchor + d0, w.ink_r = w.anchor + t1, w.degree = t1 - d1;
    return w;
  }
  // No temperature on the screen: where HaloWatch's "19°" would be
  return {WX_NOW_Y, CX - 54, WX_NOW_Y - 34, WX_NOW_Y + 34, CX - 50, CX + 64, 22};
}

// A page of the pager: its place and size (as on HaloWatch: the leaving page shrinks toward its edge and dims)
struct WxPageView {
  float cx, s, k;
  HaloView pv;
  float X(float x) const { return cx + (x - CX) * s; }
  float Y(float y) const { return CX + (y - CX) * s; }
};
static WxPageView wx_page(float pg, int i, const HaloView &v) {
  const float off = i - pg, f = halo_clamp01(fabsf(off)), s = 1 - 0.45f * f;
  const float k = fabsf(off) < 0.999f ? 1 - 0.5f * f : 0;
  const float pivot = off < 0 ? 0 : W, cx = pivot + (CX + off * W - pivot) * s;
  return {cx, s, k, {v.x + (cx - CX) * v.s, v.y, s * v.s, v.a, v.over}};
}

// A paged layer on one page (a — 0: hidden)
static void wx_layout_page(const FaceScreen &S, int i, int page, const WxPageView &P, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (!R.pg) return;
  WxRun &w = R.pg[page];
  const float s = P.s, k = P.k * a;
  const WxDay d = wx_day(page);
  switch (L.kind) {
    case FL_CITY:
      wx_text(w.it[0], *L.font, wx_city[page].name, R.mid, 1, P.X(WX_CITY_X + L.dx), P.Y(WX_CITY_Y + L.dy), s, L.color, k,
              &halo_black, L.spacing);
      halo_view_commit(w.it[0], v);
      break;
    case FL_FEELS:
      wx_text(w.it[0], *L.font, wx_feels_s[page], R.mid, 1, P.X(CX + L.dx), P.Y(WX_FEELS_Y + L.dy), s, L.color, k,
              &halo_black, L.spacing);
      halo_view_commit(w.it[0], v);
      break;
    case FL_WXTEMP: {  // the digits centered by their cells, the degree sign hangs after them; on a whole row
      HaloRoll &r = *w.roll;
      int nd = 0;
      while (wx_now_s[page][nd] >= '0' && wx_now_s[page][nd] <= '9') nd++;
      const float now_line = halo_floor(WX_NOW_Y + L.dy - r.mid + 0.5f), anchor = CX + L.dx - nd * r.cell / 2;
      halo_roll_layout(r, anchor, now_line + r.mid, L.color, k, &halo_black, P.pv);
      break;
    }
    case FL_WXICON: {
      if (!L.icons) return;
      const WxRow row = wx_row(S, page);
      const HaloImage &im = *L.icons[d.kind];
      float ix0, ix1, iy0, iy1;
      wx_image_ink(im, ix0, ix1, iy0, iy1);
      const float icon_x = row.ink_l - (WX_ROW_GAP + row.degree) * WX_ICON_NEAR - ix1 + im.w / 2.0f;
      const float icon_y = row.top - iy0 + im.h / 2.0f;
      image_at(w.it[0], im, P.X(icon_x + L.dx), P.Y(icon_y + L.dy), s, k);
      halo_view_commit(w.it[0], v);
      break;
    }
    case FL_WXHILO: {
      const WxRow row = wx_row(S, page);
      const float hl_r = row.ink_r + WX_ROW_GAP +
                         fmaxf(wx_ink_w(*L.font, wx_hi_s[page], L.spacing), wx_ink_w(*L.font, wx_lo_s[page], L.spacing));
      wx_text(w.it[0], *L.font, wx_hi_s[page], R.mid, 2, P.X(hl_r + L.dx), P.Y(row.top - R.top + R.mid + L.dy), s, L.color,
              k, &halo_black, L.spacing);
      wx_text(w.it[1], *L.font, wx_lo_s[page], R.mid, 2, P.X(hl_r + L.dx), P.Y(row.bottom - R.bottom + R.mid + L.dy), s,
              L.color2, k, &halo_black, L.spacing);
      halo_view_commit(w.it[0], v), halo_view_commit(w.it[1], v);
      break;
    }
    case FL_HOURLY: {
      // The pill with the next three hours: icon and time
      const float q = L.k, px = WX_PILL_X + L.dx, py = WX_PILL_Y + L.dy;
      HaloItem &pl = w.it[0];
      pl.kind = HI_RRECT, pl.x = P.X(px), pl.y = P.Y(py), pl.w = WX_PILL_W * q, pl.h = WX_PILL_H * q;
      pl.r = WX_PILL_H * q / 2, pl.scale = s, pl.color = L.color, pl.alpha = k, pl.on_black = true;
      halo_view_commit(pl, v);
      for (int j = 0; j < WX_HOURS; j++) {
        const float cx = px + (wsc(WX_COL[j]) - WX_PILL_X) * q;
        if (L.icons) image_at(w.it[1 + j], *L.icons[d.hour_kind[j]], P.X(cx), P.Y(py + (WX_HICON_Y - WX_PILL_Y) * q), s, k);
        wx_text(w.it[4 + j], *L.font, wx_hour[j], R.mid, 1, P.X(cx), P.Y(py + (WX_HTIME_Y - WX_PILL_Y) * q), s, L.color2,
                k, &pl, L.spacing);
        halo_view_commit(w.it[1 + j], v), halo_view_commit(w.it[4 + j], v);
      }
      break;
    }
  }
}

// Layers of the screen that do not page: the clock along the top edge, the page dots. HaloWatch's expressions as they
// are; the layer's (dx, dy) goes through the view (the same numbers: with dx = dy = 0 the view is the screen's own —
// written into the expressions instead, -ffast-math may round them another way)
static void wx_layout_static(const FaceScreen &S, int i, float a, const HaloView &v0) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  HaloView v = v0;
  v.x += L.dx * v0.s, v.y += L.dy * v0.s;
  if (L.kind == FL_ARC && S.gal && S.gal->list.n) {  // on a list: HaloWatch's gallery's (gallery.h)
    gal_arc(*S.gal, R, L.color, a, v);
    R.ax = R.tx + R.arc.w / 2.0f + L.dx, R.ay = R.ty + R.arc.h / 2.0f + L.dy;  // where it stands at rest
    return;
  }
  if (L.kind == FL_ARC) {
    HaloItem &c = R.pill;  // (the arc clock uses the run's first item)
    c.kind = HI_MASK, c.mask = &R.arc, c.color = L.color, c.on_black = true, c.on = &halo_black;
    c.x = R.tx + R.arc.w / 2.0f, c.y = R.ty + R.arc.h / 2.0f;
    c.scale = 1, c.alpha = R.arc.bits ? a : 0;
    R.ax = c.x + L.dx, R.ay = c.y + L.dy;
    halo_view_commit(c, v);
    return;
  }
  if (L.kind != FL_DOTS || !R.pg) return;
  // Page dots along the bottom arc (12 px every 20 px on r = 217, the current one — a worm); while the swipe back is
  // pulled from the bottom, they lift a little and the current one stretches up after the finger
  HaloItem *it = R.pg[0].it;
  const float p = S.pager.x;
  const float P = halo_nav_pull(), lift = 26 * (1 - expf(-P / 140)), stretch = 30 * (1 - expf(-P / 120));
  const int n = screen_pages(S);  // (a city a page; the Steps screen — two)
  if (n < 2) a = 0;  // one city: no dots
  const float R_ = DOTS_R, step = 20 / R_, a0 = HALO_PI + step * (n - 1) / 2;
  for (int i = 0; i < WX_MAX; i++) {
    const float a_ = a0 - i * step;
    HaloItem &d = it[i];
    d.kind = HI_CIRCLE, d.x = CX + R_ * sinf(a_), d.y = CX - R_ * cosf(a_) - lift, d.r = 6, d.scale = 1, d.color = L.color2;
    d.alpha = i < n ? 0.3f * a : 0, d.on_black = true;
    halo_view_commit(d, v);
  }
  const float pc = n > 1 ? halo_clamp01(p / (n - 1)) * (n - 1) : 0;
  const int i0 = (int)pc;
  const float fr = pc - i0, head = fminf(1, fr * 2), tail = fmaxf(0, fr * 2 - 1);
  const float aa = a0 - (i0 + tail) * step, ab = a0 - (i0 + head) * step;
  const bool pulled = stretch > 0.3f;
  HaloItem &worm = it[WX_MAX], &pull = it[WX_MAX + 1];
  worm.kind = HI_ARC, worm.x = CX, worm.y = CX - lift, worm.r = R_, worm.w = 12, worm.scale = 1;
  worm.a0 = fminf(aa, ab), worm.a1 = fmaxf(aa, ab) + 0.0001f, worm.color = L.color;
  worm.alpha = pulled ? 0 : a, worm.on_black = true;
  halo_view_commit(worm, v);
  // Pulled: the current dot becomes a capsule growing upward from its place
  const float ac = a0 - lroundf(pc) * step, dx = CX + R_ * sinf(ac), dy = CX - R_ * cosf(ac) - lift;
  pull.kind = HI_RRECT, pull.x = dx, pull.y = dy - stretch / 2, pull.w = 12, pull.h = 12 + stretch;
  pull.r = 6, pull.scale = 1, pull.color = L.color, pull.alpha = pulled ? a : 0, pull.on_black = true;
  halo_view_commit(pull, v);
  R.ax = CX + L.dx, R.ay = CX + R_ + L.dy;
}

static void wx_draw_page(const FaceScreen &S, int i, int page) {
  const FaceRun &R = S.run[i];
  if (!R.pg) return;
  const WxRun &w = R.pg[page];
  switch (S.layers[i].kind) {
    case FL_WXTEMP:
      halo_roll_draw(*w.roll);
      break;
    case FL_WXHILO:
      halo_item_draw(w.it[0]), halo_item_draw(w.it[1]);
      break;
    case FL_HOURLY:
      halo_item_draw(w.it[0]);
      for (int j = 0; j < WX_HOURS; j++) halo_item_draw(w.it[1 + j]), halo_item_draw(w.it[4 + j]);
      break;
    default:
      halo_item_draw(w.it[0]);
  }
}

static void wx_draw_static(const FaceScreen &S, int i) {
  const FaceRun &R = S.run[i];
  if (S.layers[i].kind == FL_ARC) return halo_item_draw(R.pill);
  if (S.layers[i].kind == FL_DOTS && R.pg)
    for (int d = 0; d < WX_MAX + 2; d++) halo_item_draw(R.pg[0].it[d]);
}

// Where a paged layer is (its anchor, page `page`) and what it covers: for the editor's frame and for overlaps
static void wx_place(const FaceScreen &S, int i, int page, float *box, float &ax, float &ay) {
  const FaceLayer &L = S.layers[i];
  const FaceRun &R = S.run[i];
  const WxRun &w = R.pg[page];
  const WxRow row = wx_row(S, page);
  switch (L.kind) {
    case FL_CITY: ax = WX_CITY_X + L.dx, ay = WX_CITY_Y + L.dy; break;
    case FL_FEELS: ax = CX + L.dx, ay = WX_FEELS_Y + L.dy; break;
    case FL_WXTEMP: ax = CX + L.dx, ay = WX_NOW_Y + L.dy; break;
    case FL_HOURLY: ax = WX_PILL_X + L.dx, ay = WX_PILL_Y + L.dy; break;
    default:  // the icon: its center; the high/low: the high's right end, the middle of the row (NaN — not drawn yet)
      ax = w.it[0].shown ? w.it[0].x : NAN, ay = w.it[0].shown ? w.it[0].y : NAN;
  }
  if (L.kind == FL_WXHILO && w.it[0].shown) ay = (row.top + row.bottom) / 2 + L.dy;
  box[0] = box[1] = 0, box[2] = box[3] = -1;
  if (L.kind == FL_WXTEMP) {
    for (int c = 0; c < w.roll->n; c++) box_add(box, w.roll->c[c].in), box_add(box, w.roll->c[c].out);
  } else {
    for (const HaloItem &it : w.it) box_add(box, it);
  }
}

// The pager of a screen with paged layers — only while the screen is open and still (as HaloWatch's weather_update):
// sideways, not from the left edge; past 10 % or a flick faster than 400 px/s — to the neighbour
static void wx_pager_update(FaceScreen &S, float dt, int pages) {  // (pages: the cities; the Steps screen — 2)
  HaloFinger &f = halo_finger;
  if (f.drag_start && f.horiz && !f.owner && f.x0 > 30 * 2.0f && halo_finger_claim(&S.pager)) S.g0 = S.pager.x, S.drag = true;
  if (S.drag && f.owner == &S.pager) {
    if (f.down) {
      const float raw = S.g0 - (f.x - f.x0) / W;
      S.pager.x = raw < 0 ? halo_rubber(raw * W) / W
                : raw > pages - 1 ? pages - 1 + halo_rubber((raw - pages + 1) * W) / W : raw;
      S.pager.v = -f.vx / W;
    } else {
      const int p = (int)lroundf(S.g0);
      const float fr = S.pager.x - p;
      int to = p;
      if (fr > 0.1f || f.vx < -400) to = p + 1;
      if (fr < -0.1f || f.vx > 400) to = p - 1;
      S.pager.target = constrain(to, 0, pages - 1);
      S.drag = false;
    }
  }
  if (!S.drag) halo_spring_run(S.pager, K_PAGER, dt);
}
