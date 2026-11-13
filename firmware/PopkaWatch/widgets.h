// widgets.h — the watch face's widgets taken from the twenty face mockups (PopkaStudio's own; the editor's Face folder
// has them as ready pieces):
//   Curved text (FL_CURVE)     — one line along the edge: the weekday, "WED · 4 NOV", "4 NOVEMBER", "18° CLOUDY",
//                                "6,420 STEPS", "WED 4 NOV · 6,420 STEPS" or the battery — "89% BATTERY", charging —
//                                "CHARGING 15%" (p2); its middle at angle p3 (degrees from 12 o'clock; the bottom half
//                                reads left to right), the letters' middle on radius p;
//   Steps ring (FL_STEPRING)     — today's steps to the goal (p) along the bezel: radius p2, thickness 6·k, the accent
//                                (color — the Steps pill's ring too), the track (color2);
//   Battery ring (FL_BATRING)    — the same ring, the battery's charge (the power chip's percent): radius p;
//   Week (FL_WEEKARC)             — the days' letters along an arc (radius p; its middle at angle p3, degrees from 12
//                                o'clock; the letters upright, on the bottom half left to right, as curved text's),
//                                today in a circle (color3), the days gone — color, the days ahead — color2;
//   Info line (FL_INFOLINE)        — the weather and the steps in a line: "18° cloudy · 6,420 steps", "18° · 6,420" or
//                                with icons — the weather's and the shoe (color3) (p2);
//   Forecast (FL_FORECAST)     — the next five hours along an arc (radius p of the icons; its middle at angle p3, as
//                                the week's): the hour (color2), the sky, the temperature (color);
//   Date frame (FL_DATEFRAME)  — the weekday above, "4 NOVEMBER" below, a line under and over the gap (p) for the
//                                clock (the lines — color2);
//   Dot clock (FL_DOTCLOCK)    — the time in dots 5 x 7 as a station's board: lit (color), dark (color2), the colon
//                                (color3); its size k.
// Each is moved by dx, dy; texts in the font picked in the editor. Curved text is baked into a mask when it changes
// (wg_tick: one a frame, as Battery's words), the rest are plain items.
// Included from face.h after stepscreen.h (wx_day, wx_deg, wx_city; fc_time, fc_h24, fc_steps — HAS_STEPS).

static const char *const WEEKDAY_FULL[7] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY",
                                            "SATURDAY"};
static const char *const MONTH_FULL[12] = {"JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST",
                                           "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
static const char *const WX_WORD[4] = {"partly cloudy", "sunny", "cloudy", "rain"};  // by WX_PARTLY ...
enum : uint8_t { CV_WEEKDAY, CV_SHORTDATE, CV_DAYMONTH, CV_WEATHER, CV_STEPS, CV_DATESTEPS, CV_BATTERY };
enum : uint8_t { IF_WORDS, IF_SHORT, IF_ICONS };
#ifndef HALO_ROUND
#define CV_CITY 7                 // (a rectangle's: the Four sides face has the city where the watch has the steps)
#define WR_SIDE_X 56.0f           // a line along the left or the right side: its letters' middle from the edge
#define WR_SIDE_RAD 40000.0f      // baked as an arc so wide it is straight (halo_text_arc turns the letters)
static bool wr_side(float angle) {  // the Place of a line: -100 — along the left side, 100 — along the right
  const float a = fabsf(angle);
  return a > 95 && a < 115;
}
#endif
#define WG_DOTS (4 * 35 + 7)  // the dot clock: four digits and the colon
#define WG_ICON 0xF54B        // the shoe (Font Awesome shoe-prints, the Steps pill's)
#define WG_ROWS 10            // a layer's texts (R.txt: rows of 24; its items point into them — each layer its own)

static uint32_t wg_steps() {
#if HAS_STEPS
  return fc_steps;
#else
  return 0;
#endif
}
// The battery as the face shows it: the power chip's (lib/Halo reads it in the background), or the Battery screen's
// made-up state when there is one (the editor's page; on the board — the command batdemo)
static HaloBattery wg_battery() {
#if HAS_BATTERY
  return bt_battery();
#else
  return halo_battery();
#endif
}
static void wg_commas(char *o, uint32_t n) {  // 6420 → "6,420"
  char s[12];
  const int len = snprintf(s, sizeof(s), "%lu", (unsigned long)n);
  for (int i = 0; i < len; i++) {
    *o++ = s[i];
    if ((len - 1 - i) % 3 == 0 && i < len - 1) *o++ = ',';
  }
  *o = 0;
}
static bool wg_now(struct tm &t) {  // the local time the face shows (with the demo's shift)
  const time_t now = time(nullptr) + fc_offset;
  localtime_r(&now, &t);
  return halo_clock_valid();
}
static void wg_upper(char *s) {
  for (; *s; s++) *s = toupper((unsigned char)*s);
}

// ---------- Curved text ----------
static void wg_curve_text(char *o, int n, int what) {
  struct tm t;
  const bool ok = wg_now(t);
  char st[16], deg[8];
  wg_commas(st, wg_steps());
  const WxDay d = wx_day(0);
  wx_deg(deg, sizeof(deg), d.now);
  switch (what) {
    case CV_WEEKDAY: snprintf(o, n, "%s", ok ? WEEKDAY_FULL[t.tm_wday] : "---"); break;
    case CV_SHORTDATE: ok ? snprintf(o, n, "%s · %d %s", WEEKDAY[t.tm_wday], t.tm_mday, MONTH[t.tm_mon]) : snprintf(o, n, "---"); break;
#ifdef HALO_ROUND
    case CV_DAYMONTH: ok ? snprintf(o, n, "%d %s", t.tm_mday, MONTH_FULL[t.tm_mon]) : snprintf(o, n, "---"); break;
#else
    case CV_DAYMONTH: ok ? snprintf(o, n, "%s %d", MONTH_FULL[t.tm_mon], t.tm_mday) : snprintf(o, n, "---"); break;
    case CV_CITY: snprintf(o, n, "%s", wx_city[0].name), wg_upper(o); break;
#endif
    case CV_WEATHER: snprintf(o, n, "%s %s", deg, WX_WORD[d.kind & 3]), wg_upper(o); break;
    case CV_STEPS: snprintf(o, n, "%s STEPS", st); break;
    case CV_BATTERY: {
      const HaloBattery b = wg_battery();
      if (!b.ok || !b.present || b.percent < 0) snprintf(o, n, b.ok ? "NO BATTERY" : "BATTERY");
      else if (b.charging) snprintf(o, n, "CHARGING %d%%", b.percent);
      else snprintf(o, n, "%d%% BATTERY", b.percent);
      break;
    }
    default:
      if (ok) snprintf(o, n, "%s %d %s · %s STEPS", WEEKDAY[t.tm_wday], t.tm_mday, MONTH[t.tm_mon], st);
      else snprintf(o, n, "%s STEPS", st);
  }
}
static void wg_curve_bake(const FaceLayer &L, FaceRun &R, const char *s) {
  strlcpy(R.txt, s, 40);
#ifndef HALO_ROUND
  if (!wr_side(L.p3) || !L.font) return;  // (a rectangle draws it straight: wg_layout; along a side — turned, baked)
  {
    float tx, ty;
    const float an = (L.p3 < 0 ? -90 : 90) * (HALO_PI / 180);
    if (halo_text_arc(R.arc, *L.font, s, R.mid, WR_SIDE_RAD, an, false, L.spacing, &tx, &ty)) {
      const float C = HALO_W / 2.0f;                                // (the arc's middle)
      R.tx = tx - (L.p3 < 0 ? C - WR_SIDE_RAD : C + WR_SIDE_RAD), R.ty = ty - C;  // (from the letters' axis and its middle)
    } else {
      free((void *)R.arc.bits), free((void *)R.arc.ext), R.arc = {};
    }
    return;
  }
#endif
  const float a = L.p3 * (HALO_PI / 180);
  const bool bottom = fabsf(L.p3) > 120;  // (the lower part reads left to right, upright)
  if (!L.font || !halo_text_arc(R.arc, *L.font, s, R.mid, L.p, a, bottom, L.spacing, &R.tx, &R.ty))
    free((void *)R.arc.bits), free((void *)R.arc.ext), R.arc = {};
}

#ifndef HALO_ROUND
static int wgr_items(uint8_t kind);
static bool wgr_fc_tick(const FaceLayer &L, FaceRun &R);
#endif

// ---------- Built once a change (face_layer_build) ----------
static void wg_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  switch (L.kind) {
    case FL_CURVE: {
      R.txt = (char *)calloc(WG_ROWS, 24);
      if (!L.font) return;
      R.mid = halo_text_mid(*L.font, "WEDNESDAY 0123456789");
      char s[40];
      wg_curve_text(s, sizeof(s), (int)L.p2);
      wg_curve_bake(L, R, s);
      break;
    }
#ifdef HALO_ROUND
    case FL_WEEKARC: R.g = new HaloItem[8](); break;
    case FL_INFOLINE: R.g = new HaloItem[4](); break;
    case FL_FORECAST: R.g = new HaloItem[15](); break;
    case FL_DATEFRAME: R.g = new HaloItem[4](); break;
    case FL_DOTCLOCK: R.g = new HaloItem[WG_DOTS](); break;
#else
    case FL_WEEKARC:
    case FL_INFOLINE:
    case FL_FORECAST:
    case FL_DATEFRAME:
    case FL_DOTCLOCK: R.g = new HaloItem[wgr_items(L.kind)](); break;
#endif
  }
#ifndef HALO_ROUND
  if (L.font2 && (L.kind == FL_WEEKARC || L.kind == FL_FORECAST)) R.top = halo_text_mid(*L.font2, "0123456789");
#endif
  if (L.font && L.kind != FL_CURVE) R.mid = halo_text_mid(*L.font, L.kind == FL_FORECAST ? "0123456789" : "MTWFS 0123456789");
  if (!R.txt) R.txt = (char *)calloc(WG_ROWS, 24);
}

// Every frame (face_tick): curved text whose words changed (a minute, a day, the steps, the weather) — baked again, one
// layer a frame (each ~3 ms: not all in one frame)
static void wg_tick() {
  static uint32_t at;
  if (halo_ms() - at < 250) return;
  at = halo_ms();
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++) {
      const FaceLayer &L = face_scr[k].layers[i];
      FaceRun &R = face_scr[k].run[i];
#ifndef HALO_ROUND
      if (L.kind == FL_FORECAST && wgr_fc_tick(L, R)) return;
#endif
      if (L.kind != FL_CURVE || !R.txt || !L.font) continue;
      char s[40];
      wg_curve_text(s, sizeof(s), (int)L.p2);
      if (strcmp(s, R.txt)) return wg_curve_bake(L, R, s);
    }
}

// ---------- Text helpers ----------
static void wg_text(HaloItem &it, const HaloFont &f, const char *s, float mid, float x, float y, uint32_t color, float a,
                    int sp, const HaloView &v, int align = 1) {
  wx_text(it, f, s, mid, align, x, y, 1, color, a, &halo_black, sp);
  halo_view_commit(it, v);
}

// Five 7-row digits and the colon, a row a byte (bit 4 — the left column)
static const uint8_t WG_GLYPH[11][7] = {
    {14, 17, 17, 17, 17, 17, 14}, {4, 12, 4, 4, 4, 4, 14},   {14, 17, 1, 2, 4, 8, 31},   {31, 2, 4, 2, 1, 17, 14},
    {2, 6, 10, 18, 31, 2, 2},     {31, 16, 30, 1, 1, 17, 14}, {6, 8, 16, 30, 17, 17, 14}, {31, 1, 2, 4, 8, 8, 8},
    {14, 17, 17, 14, 17, 17, 14}, {14, 17, 17, 15, 1, 2, 12}, {0, 0, 1, 0, 1, 0, 0}};

#ifndef HALO_ROUND
#include "widgets_rect.h"
#endif

// ---------- Each frame ----------
static void wg_layout(const FaceScreen &S, int i, float a, const HaloView &v0) {
#ifndef HALO_ROUND
  if (wgr_layout(S, i, a, v0)) return;
#endif
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  HaloView v = v0;
  v.x += L.dx * v0.s, v.y += L.dy * v0.s;  // (its place moved as the arc clock's)
  float ax = CX, ay = CX;
  char(*wg_s)[24] = (char(*)[24])R.txt;  // (the layer's own texts)
  if (!wg_s) return;
  // (curved text and the rings need neither the time nor the weather: localtime_r is ~70 µs — five curved texts a frame
  // were ~0.4 ms of every frame's layout)
  const bool when = L.kind != FL_CURVE && L.kind != FL_STEPRING && L.kind != FL_BATRING;
  const WxDay d = when ? wx_day(0) : WxDay{};
  struct tm t = {};
  const bool ok = when && wg_now(t);
  switch (L.kind) {
#ifndef HALO_ROUND
    case FL_CURVE: {  // straight (D1, S2): the angle only picks its place on the top or the bottom line
      if (!L.font) break;
      if (wr_side(L.p3)) {  // along the left or the right side: turned, reads up on the left and down on the right
        const float sx = L.p3 < 0 ? WR_SIDE_X : W - WR_SIDE_X;
        HaloItem &m = R.pill;
        m.kind = HI_MASK, m.mask = &R.arc, m.color = L.color, m.on_black = true, m.on = &halo_black;
        m.x = sx + R.tx + R.arc.w / 2.0f, m.y = CY + R.ty + R.arc.h / 2.0f, m.scale = 1, m.alpha = R.arc.bits ? a : 0;
        halo_view_commit(m, v);
        ax = sx, ay = CY;
        break;
      }
      const float an = L.p3, abs_a = fabsf(an);
      const int align = abs_a < 15 || abs_a > 165 ? 1 : an < 0 ? 0 : 2;
      const float x = align == 1 ? CX : align == 0 ? RH_MARGIN : W - RH_MARGIN;
      const float y = abs_a > 120 ? HALO_H - RH_LINE_Y : RH_LINE_Y;
      HaloItem &c = R.pill;
      c.kind = HI_TEXT, c.font = L.font, c.text = R.txt, c.align = align, c.color = L.color, c.on = &halo_black, c.on_black = true;
      c.x = x, c.y = y + (L.font->line / 2.0f - R.mid), c.scale = 1, c.alpha = a, c.spacing = L.spacing;
      halo_view_commit(c, v);
      ax = x, ay = y;
      break;
    }
#else
    case FL_CURVE: {
      HaloItem &c = R.pill;
      c.kind = HI_MASK, c.mask = &R.arc, c.color = L.color, c.on_black = true, c.on = &halo_black;
      c.x = R.tx + R.arc.w / 2.0f, c.y = R.ty + R.arc.h / 2.0f, c.scale = 1, c.alpha = R.arc.bits ? a : 0;
      halo_view_commit(c, v);
      ax = c.x, ay = c.y;
      break;
    }
#endif
    case FL_STEPRING:
    case FL_BATRING: {  // (the same ring: the steps to the goal, or the battery's percent)
      HaloItem &c = R.pill;
      const bool bat = L.kind == FL_BATRING;
      const float goal = L.p > 0 ? L.p : 10000;
      const HaloBattery b = bat ? wg_battery() : HaloBattery{};
      c.kind = HI_RING, c.x = c.y = CX, c.r = bat ? (L.p > 0 ? L.p : 224) : L.p2 > 0 ? L.p2 : 224, c.w = 6 * L.k, c.scale = 1;
      c.v = bat ? (b.ok && b.percent > 0 ? fminf(1, b.percent / 100.0f) : 0) : fminf(1, wg_steps() / goal);
      c.color = L.color, c.color2 = L.color2, c.alpha = a, c.on_black = true;
      halo_view_commit(c, v);
      ay = CX - c.r;
      break;
    }
    case FL_WEEKARC: {
      if (!R.g || !L.font) break;
      static const char *const LET[7] = {"M", "T", "W", "T", "F", "S", "S"};
      const int today = ok ? (t.tm_wday + 6) % 7 : -1;
      const float r = L.p > 0 ? L.p : 190, cr = L.font->size * 0.95f, dir = fabsf(L.p3) > 120 ? -1 : 1;
      for (int j = 0; j < 7; j++) {
        const float an = (dir * (-54 + j * 18) + L.p3) * (HALO_PI / 180), x = CX + r * sinf(an), y = CX - r * cosf(an);
        if (j == today) {
          HaloItem &c = R.g[7];
          c.kind = HI_CIRCLE, c.x = x, c.y = y, c.r = cr, c.scale = 1, c.color = L.color3, c.alpha = a, c.on_black = true;
          halo_view_commit(c, v);
          const uint32_t dark = ((L.color3 >> 16 & 255) / 5) << 16 | ((L.color3 >> 8 & 255) / 5) << 8 | (L.color3 & 255) / 5;
          wx_text(R.g[j], *L.font, LET[j], R.mid, 1, x, y, 1, dark, a, &R.g[7], L.spacing);
          halo_view_commit(R.g[j], v);
        } else {
          wg_text(R.g[j], *L.font, LET[j], R.mid, x, y, j < today ? L.color : L.color2, a, L.spacing, v);
        }
      }
      if (today < 0) R.g[7].alpha = 0, halo_view_commit(R.g[7], v);
      ax = CX + r * sinf(L.p3 * (HALO_PI / 180)), ay = CX - r * cosf(L.p3 * (HALO_PI / 180));
      break;
    }
    case FL_INFOLINE: {
      if (!R.g || !L.font) break;
      const int fmt = (int)L.p2;
      char st[16], deg[8];
      wg_commas(st, wg_steps());
      wx_deg(deg, sizeof(deg), d.now);
      const float y = 372;
      ay = y;
      if (fmt != IF_ICONS) {
        if (fmt == IF_WORDS) snprintf(wg_s[0], 48, "%s %s · %s steps", deg, WX_WORD[d.kind & 3], st);  // (two rows)
        else snprintf(wg_s[0], 24, "%s · %s", deg, st);
        wg_text(R.g[0], *L.font, wg_s[0], R.mid, CX, y, L.color, a, L.spacing, v);
        for (int j = 1; j < 4; j++) R.g[j].alpha = 0, halo_view_commit(R.g[j], v);
        break;
      }
      // [icon] 18°   [shoe] 6,420 — as one group in the middle
      strlcpy(wg_s[2], deg, 24), strlcpy(wg_s[3], st, 24);
      static char shoe[5];
      if (!*shoe) {
        char *o = shoe;
        const uint32_t c = WG_ICON;
        *o++ = 0xF0 | (c >> 18), *o++ = 0x80 | ((c >> 12) & 63), *o++ = 0x80 | ((c >> 6) & 63), *o++ = 0x80 | (c & 63), *o = 0;
      }
      const HaloImage *im = L.icons ? L.icons[d.kind & 3] : nullptr;
      const float iw = im ? im->w : 0, tw = halo_text_width(*L.font, wg_s[2], L.spacing);
      const float sw = L.font2 ? halo_text_width(*L.font2, shoe, 0) : 0, nw = halo_text_width(*L.font, wg_s[3], L.spacing);
      const float gap = L.font->size * 0.4f, wide = L.font->size;
      float x = CX - (iw + gap + tw + wide + sw + gap + nw) / 2;
      if (im) image_at(R.g[0], *im, x + iw / 2, y, 1, a), halo_view_commit(R.g[0], v);
      x += iw + gap;
      wg_text(R.g[1], *L.font, wg_s[2], R.mid, x, y, L.color, a, L.spacing, v, 0);
      x += tw + wide;
      if (L.font2) {
        const float m = halo_text_mid(*L.font2, shoe);
        wx_text(R.g[2], *L.font2, shoe, m, 0, x, y, 1, L.color3, a, &halo_black, 0);
        halo_view_commit(R.g[2], v);
      }
      x += sw + gap;
      wg_text(R.g[3], *L.font, wg_s[3], R.mid, x, y, L.color, a, L.spacing, v, 0);
      break;
    }
    case FL_FORECAST: {
      if (!R.g || !L.font) break;
      const WxCity &c = wx_city[0];
      int k0 = 1;  // the next hour
      if (c.t0 && time(nullptr) > (time_t)c.t0) k0 += (int)((time(nullptr) - (time_t)c.t0) / 3600);
      const float r = L.p > 0 ? L.p : 168, dir = fabsf(L.p3) > 120 ? -1 : 1;  // (the bottom half: left to right)
      for (int j = 0; j < 5; j++) {
        const int k = min(k0 + j, WX_AHEAD - 1);
        const int h = ok ? (t.tm_hour + 1 + j) % 24 : 0;
        const float an = (dir * (-48 + j * 24) + L.p3) * (HALO_PI / 180), sn = sinf(an), cs = cosf(an);
        if (!ok) snprintf(wg_s[j], 24, "--");
        else if (fc_h24) snprintf(wg_s[j], 24, "%02d", h);
        else if (!h) snprintf(wg_s[j], 24, "12A");
        else if (h <= 12) snprintf(wg_s[j], 24, "%d", h);
        else snprintf(wg_s[j], 24, "%dP", h - 12);
        wg_text(R.g[j * 3], *L.font, wg_s[j], R.mid, CX + (r + 38) * sn, CX - (r + 38) * cs, L.color2, a, L.spacing, v);
        const HaloImage *im = L.icons ? L.icons[(c.t0 ? c.hk[k] : d.kind) & 3] : nullptr;
        if (im) image_at(R.g[j * 3 + 1], *im, CX + r * sn, CX - r * cs, 1, a), halo_view_commit(R.g[j * 3 + 1], v);
        wx_deg(wg_s[5 + j], 24, c.t0 && c.ht[k] != WX_NONE ? c.ht[k] : d.now);
        wg_text(R.g[j * 3 + 2], *L.font, wg_s[5 + j], R.mid, CX + (r - 40) * sn, CX - (r - 40) * cs, L.color, a, L.spacing, v);
      }
      ax = CX + r * sinf(L.p3 * (HALO_PI / 180)), ay = CX - r * cosf(L.p3 * (HALO_PI / 180));
      break;
    }
    case FL_DATEFRAME: {
      if (!R.g || !L.font) break;
      const float half = (L.p > 0 ? L.p : 160) / 2, cy = 226;
      snprintf(wg_s[0], 24, "%s", ok ? WEEKDAY_FULL[t.tm_wday] : "---");
      if (ok) snprintf(wg_s[1], 24, "%d %s", t.tm_mday, MONTH_FULL[t.tm_mon]);
      else snprintf(wg_s[1], 24, "---");
      wg_text(R.g[0], *L.font, wg_s[0], R.mid, CX, cy - half - 26, L.color, a, L.spacing, v);
      wg_text(R.g[3], *L.font, wg_s[1], R.mid, CX, cy + half + 28, L.color, a, L.spacing, v);
      for (int j = 1; j <= 2; j++) {
        HaloItem &l = R.g[j];
        l.kind = HI_RRECT, l.x = CX, l.y = cy + (j == 1 ? -half : half), l.w = 160, l.h = 1.5f, l.r = 0, l.scale = 1;
        l.color = L.color2, l.alpha = a, l.on_black = true;
        halo_view_commit(l, v);
      }
      ay = cy;
      break;
    }
    case FL_DOTCLOCK: {
      if (!R.g) break;
      const float pitch = 13 * L.k, dot = 5 * L.k, cy = 189;
      const char *s = fc_time;  // "10:08"
      float x = CX - (25 * pitch) / 2 + pitch / 2;
      int n = 0;
      for (int ci = 0; ci < 5 && s[ci]; ci++) {
        const bool colon = s[ci] == ':';
        const int g = colon ? 10 : s[ci] >= '0' && s[ci] <= '9' ? s[ci] - '0' : -1, w = colon ? 1 : 5;
        for (int r = 0; r < 7; r++)
          for (int c = 0; c < w && n < WG_DOTS; c++, n++) {
            const bool on = g >= 0 && (WG_GLYPH[g][r] >> (w - 1 - c) & 1);
            HaloItem &it = R.g[n];
            it.kind = HI_CIRCLE, it.x = x + c * pitch, it.y = cy + (r - 3) * pitch, it.r = dot, it.scale = 1;
            it.color = on ? (colon ? L.color3 : L.color) : L.color2, it.alpha = colon && !on ? 0 : a, it.on_black = true;
            halo_view_commit(it, v);
          }
        x += (w + 1) * pitch;
      }
      for (; n < WG_DOTS; n++) R.g[n].alpha = 0, halo_view_commit(R.g[n], v);
      ay = cy;
      break;
    }
  }
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  if (L.kind == FL_CURVE || L.kind == FL_STEPRING || L.kind == FL_BATRING) box_add(R.box, R.pill);
  if (R.g) {
    const int n = L.kind == FL_WEEKARC ? 8 : L.kind == FL_FORECAST ? 15 : L.kind == FL_DOTCLOCK ? WG_DOTS : 4;
    for (int j = 0; j < n; j++) box_add(R.box, R.g[j]);
  }
  R.ax = ax + L.dx, R.ay = ay + L.dy;
}

static void wg_draw(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  const FaceRun &R = S.run[i];
  if (L.kind == FL_CURVE || L.kind == FL_STEPRING || L.kind == FL_BATRING) return halo_item_draw(R.pill);
#ifndef HALO_ROUND
  if (wgr_draw(L, R)) return;
#endif
  if (!R.g) return;
  if (L.kind == FL_WEEKARC) halo_item_draw(R.g[7]);  // (the circle under today's letter)
  const int n = L.kind == FL_WEEKARC ? 7 : L.kind == FL_FORECAST ? 15 : L.kind == FL_DOTCLOCK ? WG_DOTS : 4;
  for (int j = 0; j < n; j++) halo_item_draw(R.g[j]);
}
