// widgets_rect.h — the face's widgets on a rectangular screen (web/mockups/rect-widgets.html; variants E2, I1 with its two
// other formats, F4, H1, T3). One catalog on every board: the layers and their numbers are the round watch's (widgets.h),
// only their places and looks are this board's. Each is moved by dx, dy as on the watch.
//   Week        (E2) — seven columns edge to edge, Sunday first: the letter, the day's number; today in a capsule (color3);
//   Info line   (I1) — the weather in a line under the clock: words, short or with icons (this board counts no steps);
//   Forecast    (F4) — the next five hours: a smooth line of the temperature (color3), a dot, the sky and the degrees over
//                      each hour, the hours under it. The line is a mask baked when the numbers change (one a tick);
//   Date frame  (H1) — the weekday over a line, "NOVEMBER 12" under a line, the clock between them (the gap — p);
//   Dot clock   (T3) — lit dots only, 5 x 7, and each dot fades as a digit changes.
// Included from widgets.h when HALO_ROUND is not defined.

#define WR_WEEK_X0 104.0f
#define WR_WEEK_STEP 136.0f
#define WR_WEEK_LET_Y 433.0f
#define WR_WEEK_NUM_Y 500.0f
#define WR_WEEK_CAP_Y 481.0f
#define WR_WEEK_CAP_W 120.0f
#define WR_WEEK_CAP_H 166.0f
#define WR_INFO_Y 483.0f
#define WR_FC_N 5
#define WR_FC_X0 132.0f         // the first hour
#define WR_FC_STEP 190.0f
#define WR_FC_BOX_X 112.0f      // the line's mask
#define WR_FC_BOX_Y 396.0f
#define WR_FC_BOX_W 800
#define WR_FC_BOX_H 120
#define WR_FC_TOP 26.0f         // in the mask: the warmest hour's y
#define WR_FC_BOTTOM 94.0f      // and the coldest's
#define WR_FC_RANGE 3.0f        // degrees: a flat day stays a flat line
#define WR_FC_HOUR_Y 554.0f
#define WR_FRAME_CY 300.0f
#define WR_FRAME_W 524.0f
#define WR_FRAME_TEXT 41.0f     // the words' middle from their line
#define WR_DOT_CY 300.0f
#define WR_DOT_FADE 17          // of 255, a tick: a dot lights or dims in a quarter of a second
#define WR_ICON_UP "\xEF\x83\x98"    // Font Awesome: caret-up, caret-down
#define WR_ICON_DOWN "\xEF\x83\x97"

static int wgr_items(uint8_t kind) {
  switch (kind) {
    case FL_WEEKARC: return 15;          // letters, numbers, the capsule
    case FL_INFOLINE: return 6;          // the icon, the degrees, up and its number, down and its number
    case FL_FORECAST: return 21;         // hours, icons, degrees, dots, the line
    case FL_DATEFRAME: return 4;
    default: return WG_DOTS;
  }
}

// ---------- Forecast: the numbers and its line ----------
static uint32_t wgr_fc_numbers(float *t, int &k0) {  // the next five hours' degrees; 0 — no forecast
  const WxCity &c = wx_city[0];
  k0 = 1;
  if (!c.t0) return 0;
  if (time(nullptr) > (time_t)c.t0) k0 += (int)((time(nullptr) - (time_t)c.t0) / 3600);
  uint32_t sig = 2166136261u;
  for (int j = 0; j < WR_FC_N; j++) {
    const int16_t v = c.ht[min(k0 + j, WX_AHEAD - 1)];
    if (v == WX_NONE) return 0;
    t[j] = v;
    sig = (sig ^ (uint16_t)v) * 16777619u;
  }
  return sig | 1;
}

static void wgr_fc_range(const float *t, float &lo, float &to_y) {
  lo = t[0];
  float hi = t[0];
  for (int j = 1; j < WR_FC_N; j++) lo = fminf(lo, t[j]), hi = fmaxf(hi, t[j]);
  if (hi - lo < WR_FC_RANGE) {
    const float mid = (lo + hi) / 2;
    lo = mid - WR_FC_RANGE / 2, hi = mid + WR_FC_RANGE / 2;
  }
  to_y = (WR_FC_BOTTOM - WR_FC_TOP) / (hi - lo);
}

static bool wgr_fc_bake(HaloMask &m, const float *t) {
  const int w = WR_FC_BOX_W, h = WR_FC_BOX_H, stride = (w + 1) / 2;
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
  float lo, to_y;
  wgr_fc_range(t, lo, to_y);
  const float x0 = WR_FC_X0 - WR_FC_BOX_X, span = WR_FC_STEP * (WR_FC_N - 1), half = 2.5f;
  for (int x = (int)x0; x <= (int)(x0 + span); x++) {
    float dv;
    const float v = rw_spline(t, WR_FC_N, (x - x0) * (WR_FC_N - 1) / span, &dv);
    const float yc = WR_FC_BOTTOM - (v - lo) * to_y, slope = dv * to_y * (WR_FC_N - 1) / span;
    const float hw = half * sqrtf(1 + slope * slope);  // (as thick across the line on a slope too)
    const int y0 = max(0, (int)(yc - hw - 1)), y1 = min(h - 1, (int)(yc + hw + 1));
    for (int y = y0; y <= y1; y++) {
      const int a = (int)(halo_clamp01(hw + 0.5f - fabsf(y + 0.5f - yc)) * 15 + 0.5f);
      if (a <= 0) continue;
      bits[y * stride + x / 2] |= (x & 1) ? a : a << 4;
      if (ext[2 * y] > ext[2 * y + 1]) ext[2 * y] = min(x, 255);
      ext[2 * y + 1] = min(x, 255);  // (255 — to the end of the row: this mask is wider than a byte)
    }
  }
  return true;
}

// Four times a second (wg_tick): true — this layer's line was baked again, enough for now
static bool wgr_fc_tick(const FaceLayer &L, FaceRun &R) {
  float t[WR_FC_N];
  int k0;
  const uint32_t sig = wgr_fc_numbers(t, k0);
  if (sig == R.sig || !R.g) return false;
  if (sig && !wgr_fc_bake(R.arc, t)) return false;
  R.sig = sig;
  return true;
}

// ---------- Each frame ----------
static uint32_t wgr_mix(uint32_t c, uint32_t to, float k) {
  const auto ch = [&](int s) { return (uint32_t)((c >> s & 255) + (((int)(to >> s & 255) - (int)(c >> s & 255)) * k)) << s; };
  return ch(16) | ch(8) | ch(0);
}

// The widgets of this board; false — not one of them (widgets.h goes on)
static bool wgr_layout(const FaceScreen &S, int i, float a, const HaloView &v0) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (L.kind != FL_WEEKARC && L.kind != FL_INFOLINE && L.kind != FL_FORECAST && L.kind != FL_DATEFRAME && L.kind != FL_DOTCLOCK) return false;
  HaloView v = v0;
  v.x += L.dx * v0.s, v.y += L.dy * v0.s;  // (its place moved, as on the watch)
  char(*wg_s)[24] = (char(*)[24])R.txt;
  if (!wg_s || !R.g) return true;
  const WxDay d = wx_day(0);
  struct tm t;
  const bool ok = wg_now(t);
  float ax = CX, ay = CY;
  switch (L.kind) {
    case FL_WEEKARC: {
      if (!L.font) break;
      static const char *const LET[7] = {"S", "M", "T", "W", "T", "F", "S"};
      const int today = ok ? t.tm_wday : -1;
      if (ok && (uint8_t)wg_s[7][0] != t.tm_mday) {  // (a new day: the seven numbers)
        wg_s[7][0] = t.tm_mday;
        for (int j = 0; j < 7; j++) {
          struct tm c = t;
          c.tm_mday += j - t.tm_wday;
          mktime(&c);
          snprintf(wg_s[j], 24, "%d", c.tm_mday);
        }
      }
      HaloItem &cap = R.g[14];
      cap.kind = HI_RRECT, cap.x = WR_WEEK_X0 + (today < 0 ? 0 : today) * WR_WEEK_STEP, cap.y = WR_WEEK_CAP_Y, cap.w = WR_WEEK_CAP_W, cap.h = WR_WEEK_CAP_H;
      cap.r = 34, cap.scale = 1, cap.color = L.color3, cap.alpha = today < 0 ? 0 : a, cap.on_black = true, cap.on = nullptr;
      halo_view_commit(cap, v);
      const uint32_t dark = ((L.color3 >> 16 & 255) / 5) << 16 | ((L.color3 >> 8 & 255) / 5) << 8 | (L.color3 & 255) / 5;
      for (int j = 0; j < 7; j++) {
        const float x = WR_WEEK_X0 + j * WR_WEEK_STEP;
        const bool now = j == today;
        const uint32_t c = j < today ? L.color : L.color2, n = now ? dark : wgr_mix(c, 0xFFFFFF, 0.3f);
        const HaloItem *on = now ? &cap : &halo_black;
        wx_text(R.g[j], *L.font, LET[j], R.mid, 1, x, WR_WEEK_LET_Y, 1, now ? dark : c, a, on, L.spacing);
        halo_view_commit(R.g[j], v);
        if (L.font2) {
          wx_text(R.g[7 + j], *L.font2, wg_s[j], R.top, 1, x, WR_WEEK_NUM_Y, 1, n, ok ? a : 0, on, 0);
          halo_view_commit(R.g[7 + j], v);
        }
      }
      ay = WR_WEEK_CAP_Y;
      break;
    }
    case FL_INFOLINE: {
      if (!L.font) break;
      const int fmt = (int)L.p2;
      char now_s[8], hi_s[8], lo_s[8], fl_s[8];
      wx_deg(now_s, sizeof(now_s), d.now), wx_deg(hi_s, sizeof(hi_s), d.hi), wx_deg(lo_s, sizeof(lo_s), d.lo), wx_deg(fl_s, sizeof(fl_s), d.feels);
      ay = WR_INFO_Y;
      if (fmt != IF_ICONS) {
        if (fmt == IF_WORDS) snprintf(wg_s[0], 48, "%s %s · feels like %s", now_s, WX_WORD[d.kind & 3], fl_s);  // (two rows)
        else snprintf(wg_s[0], 48, "%s · %s / %s", now_s, hi_s, lo_s);
        wg_text(R.g[1], *L.font, wg_s[0], R.mid, CX, WR_INFO_Y, L.color, a, L.spacing, v);
        for (int j = 0; j < 6; j++)
          if (j != 1) R.g[j].alpha = 0, halo_view_commit(R.g[j], v);
        break;
      }
      // [icon] 12°   ^ 15°   v 7° — as one group in the middle
      strlcpy(wg_s[2], now_s, 24), strlcpy(wg_s[3], hi_s, 24), strlcpy(wg_s[4], lo_s, 24);
      const HaloImage *im = L.icons ? L.icons[d.kind & 3] : nullptr;
      const float iw = im ? im->w : 0, gap = L.font->size * 0.3f, wide = L.font->size * 0.8f;
      const float tw = halo_text_width(*L.font, wg_s[2], L.spacing), hw = halo_text_width(*L.font, wg_s[3], L.spacing), lw = halo_text_width(*L.font, wg_s[4], L.spacing);
      const float cw = L.font2 ? halo_text_width(*L.font2, WR_ICON_UP, 0) : 0;
      float x = CX - (iw + gap + tw + wide + cw + gap + hw + wide + cw + gap + lw) / 2;
      if (im) image_at(R.g[0], *im, x + iw / 2, WR_INFO_Y, 1, a), halo_view_commit(R.g[0], v);
      x += iw + gap;
      wg_text(R.g[1], *L.font, wg_s[2], R.mid, x, WR_INFO_Y, L.color, a, L.spacing, v, 0);
      x += tw + wide;
      const float cm = L.font2 ? halo_text_mid(*L.font2, WR_ICON_UP) : 0;
      if (L.font2) wx_text(R.g[2], *L.font2, WR_ICON_UP, cm, 0, x, WR_INFO_Y, 1, L.color3, a, &halo_black, 0), halo_view_commit(R.g[2], v);
      x += cw + gap;
      wg_text(R.g[3], *L.font, wg_s[3], R.mid, x, WR_INFO_Y, L.color, a, L.spacing, v, 0);
      x += hw + wide;
      if (L.font2) wx_text(R.g[4], *L.font2, WR_ICON_DOWN, cm, 0, x, WR_INFO_Y, 1, 0xA8C7FA, a, &halo_black, 0), halo_view_commit(R.g[4], v);
      x += cw + gap;
      wg_text(R.g[5], *L.font, wg_s[4], R.mid, x, WR_INFO_Y, L.color, a, L.spacing, v, 0);
      break;
    }
    case FL_FORECAST: {
      if (!L.font) break;
      const WxCity &c = wx_city[0];
      float deg[WR_FC_N], lo = 0, to_y = 0;
      int k0;
      const bool known = wgr_fc_numbers(deg, k0) != 0;
      if (known) wgr_fc_range(deg, lo, to_y);
      for (int j = 0; j < WR_FC_N; j++) {
        const int k = min(k0 + j, WX_AHEAD - 1), h = ok ? (t.tm_hour + 1 + j) % 24 : 0;
        const float x = WR_FC_X0 + j * WR_FC_STEP;
        const float y = WR_FC_BOX_Y + (known ? WR_FC_BOTTOM - (deg[j] - lo) * to_y : (WR_FC_TOP + WR_FC_BOTTOM) / 2);
        if (!ok) snprintf(wg_s[j], 24, "--");
        else if (fc_h24) snprintf(wg_s[j], 24, "%02d", h);
        else snprintf(wg_s[j], 24, "%d %cM", h % 12 ? h % 12 : 12, h < 12 ? 'A' : 'P');
        if (L.font2) wx_text(R.g[j], *L.font2, wg_s[j], R.top, 1, x, WR_FC_HOUR_Y, 1, L.color2, a, &halo_black, L.spacing), halo_view_commit(R.g[j], v);
        const HaloImage *im = L.icons ? L.icons[(c.t0 ? c.hk[k] : d.kind) & 3] : nullptr;
        if (im) image_at(R.g[5 + j], *im, x - 32, y - 38, 1, a), halo_view_commit(R.g[5 + j], v);
        wx_deg(wg_s[5 + j], 24, known ? (int16_t)deg[j] : d.now);
        wg_text(R.g[10 + j], *L.font, wg_s[5 + j], R.mid, x + 2, y - 37, L.color, a, L.spacing, v, 0);
        HaloItem &dot = R.g[15 + j];
        dot.kind = HI_CIRCLE, dot.x = x, dot.y = y, dot.r = 9, dot.scale = 1, dot.color = L.color3, dot.alpha = known ? a : 0, dot.on_black = true, dot.on = nullptr;
        halo_view_commit(dot, v);
      }
      HaloItem &m = R.g[20];
      m.kind = HI_MASK, m.mask = &R.arc, m.color = L.color3, m.on_black = true, m.on = &halo_black;
      m.x = WR_FC_BOX_X + WR_FC_BOX_W / 2.0f, m.y = WR_FC_BOX_Y + WR_FC_BOX_H / 2.0f, m.scale = 1, m.alpha = known && R.arc.bits && R.sig ? a : 0;
      m.v = (R.sig & 255) / 256.0f;  // (not used by a mask: new numbers in the same memory must redraw it)
      halo_view_commit(m, v);
      ay = WR_FC_BOX_Y + WR_FC_BOX_H / 2.0f;
      break;
    }
    case FL_DATEFRAME: {
      if (!L.font) break;
      const float half = (L.p > 0 ? L.p : 285) / 2;
      snprintf(wg_s[0], 24, "%s", ok ? WEEKDAY_FULL[t.tm_wday] : "---");
      if (ok) snprintf(wg_s[1], 24, "%s %d", MONTH_FULL[t.tm_mon], t.tm_mday);
      else snprintf(wg_s[1], 24, "---");
      wg_text(R.g[0], *L.font, wg_s[0], R.mid, CX, WR_FRAME_CY - half - WR_FRAME_TEXT, L.color, a, L.spacing, v);
      wg_text(R.g[3], *L.font, wg_s[1], R.mid, CX, WR_FRAME_CY + half + WR_FRAME_TEXT, L.color, a, L.spacing, v);
      for (int j = 1; j <= 2; j++) {
        HaloItem &l = R.g[j];
        l.kind = HI_RRECT, l.x = CX, l.y = WR_FRAME_CY + (j == 1 ? -half : half), l.w = WR_FRAME_W, l.h = 3, l.r = 1.5f, l.scale = 1;
        l.color = L.color2, l.alpha = a, l.on_black = true, l.on = nullptr;
        halo_view_commit(l, v);
      }
      ay = WR_FRAME_CY;
      break;
    }
    case FL_DOTCLOCK: {
      const float pitch = 13 * L.k, dot = 5 * L.k;
      const char *s = fc_time;  // "10:24"
      int cols = -1;  // (a time of four characters — "9:05" — stays in the middle too)
      for (int ci = 0; ci < 5 && s[ci]; ci++) cols += s[ci] == ':' ? 2 : 6;
      float x = CX - (cols - 1) * pitch / 2;
      uint8_t *fade = (uint8_t *)R.txt;   // (each dot's brightness, 0..255: R.txt is 240 bytes)
      const bool snap = R.top == 0 || fc_calm;  // (at the start, and in Always On: no fades)
      R.top = 1;
      int n = 0;
      for (int ci = 0; ci < 5 && s[ci]; ci++) {
        const bool colon = s[ci] == ':';
        const int g = colon ? 10 : s[ci] >= '0' && s[ci] <= '9' ? s[ci] - '0' : -1, w = colon ? 1 : 5;
        for (int r = 0; r < 7; r++)
          for (int c = 0; c < w && n < WG_DOTS; c++, n++) {
            const bool on = g >= 0 && (WG_GLYPH[g][r] >> (w - 1 - c) & 1);
            int f = fade[n];
            if (snap) f = on ? 255 : 0;
            else f = on ? min(255, f + WR_DOT_FADE) : max(0, f - WR_DOT_FADE);
            fade[n] = f;
            HaloItem &it = R.g[n];
            it.kind = HI_CIRCLE, it.x = x + c * pitch, it.y = WR_DOT_CY + (r - 3) * pitch, it.r = dot, it.scale = 1;
            it.color = colon ? L.color3 : L.color, it.alpha = f / 255.0f * a, it.on_black = true, it.on = nullptr;
            halo_view_commit(it, v);
          }
        x += (w + 1) * pitch;
      }
      for (; n < WG_DOTS; n++) fade[n] = 0, R.g[n].alpha = 0, halo_view_commit(R.g[n], v);
      ay = WR_DOT_CY;
      break;
    }
  }
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  for (int j = 0, n = wgr_items(L.kind); j < n; j++) box_add(R.box, R.g[j]);
  R.ax = ax + L.dx, R.ay = ay + L.dy;
  return true;
}

// The widgets of this board; false — not one of them
static bool wgr_draw(const FaceLayer &L, const FaceRun &R) {
  if (L.kind != FL_WEEKARC && L.kind != FL_INFOLINE && L.kind != FL_FORECAST && L.kind != FL_DATEFRAME && L.kind != FL_DOTCLOCK) return false;
  if (!R.g) return true;
  if (L.kind == FL_WEEKARC) halo_item_draw(R.g[14]);  // (the capsule under today's letter and number)
  if (L.kind == FL_FORECAST) halo_item_draw(R.g[20]);  // (the line under its dots)
  const int n = L.kind == FL_WEEKARC ? 14 : L.kind == FL_FORECAST ? 20 : wgr_items(L.kind);
  for (int j = 0; j < n; j++) halo_item_draw(R.g[j]);
  return true;
}
