// wxscreen_rect.h — the Weather screen on a rectangular screen (variant W4 of web/mockups/rect-screens.html). The layers
// are the round watch's (wxscreen.h), with the same pager and the same numbers; only their places and two of their
// looks are this board's: the top clock is a straight line in the corner, the high/low layer also says the sky in words,
// the hourly layer is a smooth line of the temperature for the next eight hours with captions under it, the page dots
// stand in a row. A chart is a mask baked when its numbers change (a new hour, new weather), one page per tick.
// Included from wxscreen.h in place of its layout when HALO_ROUND is not defined.

#define RW_CITY_X 40.0f
#define RW_CITY_Y 66.0f
#define RW_ICON_X 100.0f
#define RW_ICON_Y 180.0f
#define RW_TEMP_X 190.0f
#define RW_TEMP_Y 172.0f
#define RW_TEXT_R (W - 40.0f)   // the sky in words and the high/low: their right end
#define RW_WORD_Y 162.0f
#define RW_HILO_Y 206.0f
#define RW_FEELS_Y 250.0f
#define RW_TIME_R (W - 32.0f)   // the time in the corner: its right end
#define RW_TIME_Y 43.0f
#define RW_CHART_X 32
#define RW_CHART_Y 300
#define RW_CHART_W 960
#define RW_CHART_H 240
#define RW_LINE_X0 18.0f        // the line inside the chart's box: the first hour
#define RW_LINE_X1 942.0f       // and the last
#define RW_LINE_TOP 50.0f       // the warmest hour
#define RW_LINE_BOTTOM 210.0f   // and the coldest
#define RW_LINE_HALF 2.5f       // half the line's thickness
#define RW_MIN_RANGE 6.0f       // degrees: a flat day stays a flat line
#define RW_LABEL_X 50.0f
#define RW_LABEL_STEP 215.0f
#define RW_LABEL_Y 553.0f
#define RW_DOTS_Y 584.0f

// The next RW_POINTS hours of a page; 0 — no forecast
static uint32_t rw_chart_numbers(int page, float *t) {
  const WxCity &c = wx_city[page];
  if (!c.t0) return 0;
  int k = 0;
  if (time(nullptr) > (time_t)c.t0) k = (int)((time(nullptr) - (time_t)c.t0) / 3600);
  uint32_t sig = 2166136261u;
  for (int i = 0; i < RW_POINTS; i++) {
    const int16_t v = c.ht[k + i < WX_AHEAD ? k + i : WX_AHEAD - 1];
    if (v == WX_NONE) return 0;
    t[i] = v;
    sig = (sig ^ (uint16_t)v) * 16777619u;
  }
  return sig | 1;
}

// Catmull-Rom through the points p[0..n-1] at u (0..n-1), and its slope
static float rw_spline(const float *p, int n, float u, float *slope) {
  int i = (int)u;
  if (i > n - 2) i = n - 2;
  const float f = u - i, a = p[i > 0 ? i - 1 : 0], b = p[i], c = p[i + 1], d = p[i + 2 < n ? i + 2 : n - 1];
  const float c1 = (c - a) / 2, c2 = a - 2.5f * b + 2 * c - d / 2, c3 = (d - a) / 2 + 1.5f * (b - c);
  *slope = c1 + f * (2 * c2 + 3 * f * c3);
  return b + f * (c1 + f * (c2 + f * c3));
}

static bool rw_chart_bake(HaloMask &m, const float *t) {
  const int w = RW_CHART_W, h = RW_CHART_H, stride = (w + 1) / 2;
  if (!m.bits) {
#ifdef __EMSCRIPTEN__
    m.bits = (const uint8_t *)malloc(stride * h), m.ext = (const uint8_t *)malloc(2 * h);
#else
    m.bits = (const uint8_t *)heap_caps_malloc(stride * h, MALLOC_CAP_SPIRAM);
    m.ext = (const uint8_t *)heap_caps_malloc(2 * h, MALLOC_CAP_SPIRAM);
#endif
    if (!m.bits || !m.ext) {
      free((void *)m.bits), free((void *)m.ext), m = {};
      return false;
    }
    m.w = w, m.h = h, m.ox = m.oy = 0, m.adv = w;
  }
  uint8_t *bits = (uint8_t *)m.bits, *ext = (uint8_t *)m.ext;
  memset(bits, 0, stride * h);
  for (int y = 0; y < h; y++) ext[2 * y] = 1, ext[2 * y + 1] = 0;
  float lo = t[0], hi = t[0];
  for (int i = 1; i < RW_POINTS; i++) lo = fminf(lo, t[i]), hi = fmaxf(hi, t[i]);
  if (hi - lo < RW_MIN_RANGE) {
    const float mid = (lo + hi) / 2;
    lo = mid - RW_MIN_RANGE / 2, hi = mid + RW_MIN_RANGE / 2;
  }
  const float to_y = (RW_LINE_BOTTOM - RW_LINE_TOP) / (hi - lo), span = RW_LINE_X1 - RW_LINE_X0;
  for (int x = (int)RW_LINE_X0; x <= (int)RW_LINE_X1; x++) {
    float dv;
    const float v = rw_spline(t, RW_POINTS, (x - RW_LINE_X0) * (RW_POINTS - 1) / span, &dv);
    const float yc = RW_LINE_BOTTOM - (v - lo) * to_y, slope = dv * to_y * (RW_POINTS - 1) / span;
    const float half = RW_LINE_HALF * sqrtf(1 + slope * slope);  // (as thick across the line on a slope too)
    const int y0 = max(0, (int)(yc - half - 1)), y1 = min(h - 1, (int)(yc + half + 1));
    for (int y = y0; y <= y1; y++) {
      const int a = (int)(halo_clamp01(half + 0.5f - fabsf(y + 0.5f - yc)) * 15 + 0.5f);
      if (a <= 0) continue;
      bits[y * stride + x / 2] |= (x & 1) ? a : a << 4;
      if (ext[2 * y] > ext[2 * y + 1]) ext[2 * y] = min(x, 255);
      ext[2 * y + 1] = min(x, 255);  // (255 — to the end of the row: this mask is wider than a byte)
    }
  }
  return true;
}

// Four times a second: at most one chart whose numbers changed (a new hour, new weather)
static void rw_chart_tick() {
  static uint32_t at;
  if (halo_ms() - at < 250) return;
  at = halo_ms();
  float t[RW_POINTS];
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++) {
      FaceRun &R = face_scr[k].run[i];
      if (face_scr[k].layers[i].kind != FL_HOURLY || !R.pg) continue;
      for (int p = 0; p < wx_pages; p++) {
        WxRun &w = R.pg[p];
        const uint32_t sig = rw_chart_numbers(p, t);
        if (sig == w.chart_sig) continue;
        if (!sig || rw_chart_bake(w.chart, t)) w.chart_sig = sig;
        return;
      }
    }
}

// Text by the middle of its letters; align: 0 — x is its left end, 2 — its right end
static void rw_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, float sc,
                    uint32_t color, float alpha, const HaloItem *on, int sp) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = align, it.color = color, it.on = on, it.on_black = true;
  it.x = x, it.y = y + (f.line / 2.0f - mid) * sc, it.scale = sc, it.alpha = alpha, it.spacing = sp;
}

static void wx_layout_page(const FaceScreen &S, int i, int page, const WxPageView &P, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (!R.pg) return;
  WxRun &w = R.pg[page];
  const float s = P.s, k = P.k * a;
  switch (L.kind) {
    case FL_CITY:
      rw_text(w.it[0], *L.font, wx_city[page].name, R.mid, 0, P.X(RW_CITY_X + L.dx), P.Y(RW_CITY_Y + L.dy), s, L.color, k,
              &halo_black, L.spacing);
      halo_view_commit(w.it[0], v);
      break;
    case FL_WXTEMP: {
      HaloRoll &r = *w.roll;
      halo_roll_layout(r, RW_TEMP_X + L.dx, RW_TEMP_Y + L.dy, L.color, k, &halo_black, P.pv);
      break;
    }
    case FL_WXICON: {
      if (!L.icons) return;
      image_at(w.it[0], *L.icons[wx_day(page).kind], P.X(RW_ICON_X + L.dx), P.Y(RW_ICON_Y + L.dy), s, k);
      halo_view_commit(w.it[0], v);
      break;
    }
    case FL_WXHILO:
      rw_text(w.it[0], *L.font, wx_word_s[page], R.mid, 2, P.X(RW_TEXT_R + L.dx), P.Y(RW_WORD_Y + L.dy), s, L.color, k,
              &halo_black, L.spacing);
      rw_text(w.it[1], *L.font, wx_hl_s[page], R.mid, 2, P.X(RW_TEXT_R + L.dx), P.Y(RW_HILO_Y + L.dy), s, L.color2, k,
              &halo_black, L.spacing);
      halo_view_commit(w.it[0], v), halo_view_commit(w.it[1], v);
      break;
    case FL_FEELS:
      rw_text(w.it[0], *L.font, wx_feels_s[page], R.mid, 2, P.X(RW_TEXT_R + L.dx), P.Y(RW_FEELS_Y + L.dy), s, L.color, k,
              &halo_black, L.spacing);
      halo_view_commit(w.it[0], v);
      break;
    case FL_HOURLY: {
      HaloItem &c = w.it[0];
      c.kind = HI_MASK, c.mask = &w.chart, c.color = L.color, c.on_black = true, c.on = &halo_black;
      c.x = P.X(RW_CHART_X + RW_CHART_W / 2.0f + L.dx), c.y = P.Y(RW_CHART_Y + RW_CHART_H / 2.0f + L.dy), c.scale = s;
      c.alpha = w.chart.bits && w.chart_sig ? k : 0;
      c.v = (w.chart_sig & 255) / 256.0f;  // (not used by a mask: new numbers in the same memory must redraw it)
      halo_view_commit(c, v);
      for (int j = 0; j < RW_LABELS; j++) {
        rw_text(w.it[1 + j], *L.font, rw_label[j], R.mid, 0, P.X(RW_LABEL_X + j * RW_LABEL_STEP + L.dx), P.Y(RW_LABEL_Y + L.dy),
                s, L.color2, k, &halo_black, L.spacing);
        halo_view_commit(w.it[1 + j], v);
      }
      break;
    }
  }
}

// The layers that do not page: the time in the corner, the page dots in a row
static void wx_layout_static(const FaceScreen &S, int i, float a, const HaloView &v0) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  HaloView v = v0;
  v.x += L.dx * v0.s, v.y += L.dy * v0.s;
  if (L.kind == FL_ARC) {
    HaloItem &c = R.pill;
    if (!L.font) {
      c.alpha = 0, halo_view_commit(c, v);
      return;
    }
    // On a list it goes up and fades as the page scrolls (gallery_rect.h: tt_k)
    const float k = S.gal && S.gal->list.n ? S.gal->tt_k : 1, y = RW_TIME_Y - 24 * DP * (1 - k);
    rw_text(c, *L.font, wx_time, R.mid, 2, RW_TIME_R, y, 1, L.color, k > 0.02f ? k * a : 0, &halo_black, L.spacing);
    R.ax = RW_TIME_R + L.dx, R.ay = RW_TIME_Y + L.dy;
    halo_view_commit(c, v);
    return;
  }
  if (L.kind != FL_DOTS || !R.pg) return;
  HaloItem *it = R.pg[0].it;
  const float pos = S.pager.x, P = halo_nav_pull(), lift = 26 * (1 - expf(-P / 140)), stretch = 30 * (1 - expf(-P / 120));
  const int n = screen_pages(S);
  if (n < 2) a = 0;
  const float step = 20, x0 = CX - step * (n - 1) / 2, y = RW_DOTS_Y - lift;
  for (int d = 0; d < WX_MAX; d++) {
    HaloItem &q = it[d];
    q.kind = HI_CIRCLE, q.x = x0 + d * step, q.y = y, q.r = 6, q.scale = 1, q.color = L.color2;
    q.alpha = d < n ? 0.3f * a : 0, q.on_black = true;
    halo_view_commit(q, v);
  }
  const float pc = n > 1 ? halo_clamp01(pos / (n - 1)) * (n - 1) : 0;
  const int i0 = (int)pc;
  const float fr = pc - i0, head = fminf(1, fr * 2), tail = fmaxf(0, fr * 2 - 1);
  const float xa = x0 + (i0 + tail) * step, xb = x0 + (i0 + head) * step;
  const bool pulled = stretch > 0.3f;
  HaloItem &worm = it[WX_MAX], &pull = it[WX_MAX + 1];
  worm.kind = HI_RRECT, worm.x = (xa + xb) / 2, worm.y = y, worm.w = 12 + xb - xa, worm.h = 12, worm.r = 6, worm.scale = 1;
  worm.color = L.color, worm.alpha = pulled ? 0 : a, worm.on_black = true;
  halo_view_commit(worm, v);
  pull.kind = HI_RRECT, pull.x = x0 + lroundf(pc) * step, pull.y = y - stretch / 2, pull.w = 12, pull.h = 12 + stretch;
  pull.r = 6, pull.scale = 1, pull.color = L.color, pull.alpha = pulled ? a : 0, pull.on_black = true;
  halo_view_commit(pull, v);
  R.ax = CX + L.dx, R.ay = RW_DOTS_Y + L.dy;
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
      for (int j = 0; j <= RW_LABELS; j++) halo_item_draw(w.it[j]);
      break;
    default:
      halo_item_draw(w.it[0]);
  }
}

// Where a paged layer is (its anchor, page `page`) and what it covers: for the editor's frame and for overlaps
static void wx_place(const FaceScreen &S, int i, int page, float *box, float &ax, float &ay) {
  const FaceLayer &L = S.layers[i];
  const FaceRun &R = S.run[i];
  const WxRun &w = R.pg[page];
  switch (L.kind) {
    case FL_CITY: ax = RW_CITY_X, ay = RW_CITY_Y; break;
    case FL_WXTEMP: ax = RW_TEMP_X, ay = RW_TEMP_Y; break;
    case FL_WXICON: ax = RW_ICON_X, ay = RW_ICON_Y; break;
    case FL_WXHILO: ax = RW_TEXT_R, ay = (RW_WORD_Y + RW_HILO_Y) / 2; break;
    case FL_FEELS: ax = RW_TEXT_R, ay = RW_FEELS_Y; break;
    default: ax = RW_CHART_X + RW_CHART_W / 2.0f, ay = RW_CHART_Y + RW_CHART_H / 2.0f;
  }
  ax += L.dx, ay += L.dy;
  box[0] = box[1] = 0, box[2] = box[3] = -1;
  if (L.kind == FL_WXTEMP) {
    for (int c = 0; c < w.roll->n; c++) box_add(box, w.roll->c[c].in), box_add(box, w.roll->c[c].out);
  } else {
    for (const HaloItem &it : w.it) box_add(box, it);
  }
}
