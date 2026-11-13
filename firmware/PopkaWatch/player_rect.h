// player_rect.h — the Music screen on a rectangular screen (variant M3 of web/mockups/rect-apps.html): at the left a cover
// square of the track's own colour, drawn by this code; at the right the title, the artist, a time line that can be dragged
// and one row of buttons round the watch's big wave button. The sound, the tracks and every animation are player.h's; only
// where the parts stand and the cover are this board's. Included from player.h in place of its round layout.

#define PR_COVER_X 48.0f
#define PR_COVER_Y 90.0f
#define PR_COVER_S 420.0f
#define PR_COVER_R 36.0f
#define PR_TEXT_X 510.0f
#define PR_TITLE_Y 137.0f   // the middle of the letters
#define PR_ARTIST_Y 196.0f
#define PR_LINE_X 510.0f
#define PR_LINE_W 466.0f
#define PR_LINE_Y 266.0f
#define PR_TIME_Y 303.0f
#define PR_BTN_Y 430.0f
#define PR_VOL_X 530.0f
#define PR_PREV_X 616.0f
#define PR_BIG_X 748.0f
#define PR_NEXT_X 880.0f
#define PR_LIKE_X 966.0f
#define PR_SIDE_W 84.0f
#define PR_SIDE_H 100.0f
#define PR_SMALL_R 36.0f
#define PR_BIG_K 1.06f      // the watch's button at this board's size
#define PR_CARD 0x2D3236    // the small buttons, the track of the line
#define PR_ACC 0xD9C2F0     // the line, the knob
#define PR_ICON_VOL "\xEF\x80\xA8"
#define PR_ICON_HEART "\xEF\x80\x84"

static const uint32_t PR_COVERS[8][2] = {  // a track's cover: the light corner, the dark corner
    {0x6F5AA8, 0x272447}, {0x4C7FA8, 0x1C3247}, {0xA8635A, 0x47261F}, {0x5AA883, 0x1F4735},
    {0xA89A5A, 0x47401F}, {0xA85A8F, 0x471F3B}, {0x5A9FA8, 0x1F4147}, {0x7B7F8C, 0x2A2C33}};
static uint16_t pr_pal[4][256];  // the gradient, four palettes a quarter of a 16-bit step apart (a 2 x 2 dither hides the steps)
static uint32_t pr_pal_sig, pr_c0, pr_c1;
static HaloMask pr_note;
static HaloItem pr_note_it, pr_track, pr_fill, pr_knob, pr_t0, pr_t1;
static PlDmg pr_dmg_cover;
static char pr_s0[8], pr_s1[8];
static float pr_mid_n, pr_mid_ic;
static float pr_seek = -1;  // the finger's place on the time line, 0..1 (-1 — not dragged)
static bool pr_line;
static struct {
  float x0, y0, x1, y1, r;
} PRC;

// The cover's note: drawn once at start into a mask (the icon of the mockup, a 24-px design at 7 x)
static void pr_build() {
  pr_mid_n = halo_text_mid(gl_r_n24, "0123456789"), pr_mid_ic = halo_text_mid(gl_r_ic36, PR_ICON_VOL);
  const int D = 170, bpr = (D + 1) / 2;
  uint8_t *bits = (uint8_t *)heap_caps_calloc(D * bpr + D * 2, 1, MALLOC_CAP_SPIRAM);
  if (!bits) return;
  uint8_t *ext = bits + D * bpr;
  const float k = D / 24.0f, hw = 1;
  auto seg = [](float px, float py, float ax, float ay, float bx, float by) {
    const float ex = bx - ax, ey = by - ay, t = halo_clamp01(((px - ax) * ex + (py - ay) * ey) / (ex * ex + ey * ey));
    const float qx = px - ax - t * ex, qy = py - ay - t * ey;
    return halo_sqrt(qx * qx + qy * qy);
  };
  for (int y = 0; y < D; y++) {
    int first = D, last = -1;
    for (int x = 0; x < D; x++) {
      const float px = (x + 0.5f) / k, py = (y + 0.5f) / k;
      float d = fminf(fminf(seg(px, py, 9, 18, 9, 6), seg(px, py, 9, 6, 19, 4)), seg(px, py, 19, 4, 19, 16)) - hw;
      const float dx1 = px - 6.5f, dy1 = py - 18, dx2 = px - 16.5f, dy2 = py - 16;
      d = fminf(d, fabsf(halo_sqrt(dx1 * dx1 + dy1 * dy1) - 2.5f) - hw);
      d = fminf(d, fabsf(halo_sqrt(dx2 * dx2 + dy2 * dy2) - 2.5f) - hw);
      const int v = (int)(halo_clamp01(0.5f - d * k) * 15 + 0.5f);
      if (!v) continue;
      bits[y * bpr + x / 2] |= x & 1 ? v : v << 4;
      first = min(first, x), last = max(last, x);
    }
    ext[2 * y] = first > last ? 1 : first, ext[2 * y + 1] = first > last ? 0 : last;
  }
  pr_note = {(int16_t)D, (int16_t)D, 0, 0, 0, bits, ext};
}

static void player_enter() {
  pl_armed = pl_small_armed = -1, pr_line = false, pr_seek = -1;
  vol_enter();
}

static void player_update(float dt) {
  HaloFinger &f = halo_finger;
  pl_active = true;
  if (vol_update(dt)) return;
  if (f.pressed && !f.owner) {
    pl_armed = pl_small_armed = -1, pr_line = false;
    const float dx = f.x0 - PR_BIG_X, dy = f.y0 - PR_BTN_Y;
    const bool row = fabsf(dy) < 58;
    if (dx * dx + dy * dy < 90 * 90) pl_armed = 1;
    else if (row && fabsf(f.x0 - PR_PREV_X) < 50) pl_armed = 0;
    else if (row && fabsf(f.x0 - PR_NEXT_X) < 50) pl_armed = 2;
    else if (row && fabsf(f.x0 - PR_VOL_X) < 44) pl_small_armed = 0;
    else if (row && fabsf(f.x0 - PR_LIKE_X) < 44) pl_small_armed = 1;
    pr_line = mu_n && fabsf(f.y0 - PR_LINE_Y) < 26 && f.x0 > PR_LINE_X - 14 && f.x0 < PR_LINE_X + PR_LINE_W + 14;
  }
  if (pr_line && f.down && (!f.dragging || f.horiz) && halo_finger_claim(&pr_line)) pr_seek = halo_clamp01((f.x - PR_LINE_X) / PR_LINE_W);
  if (f.dragging) pl_armed = pl_small_armed = -1;
  if (f.tap && pl_armed >= 0) {
    if (pl_armed == 1) pl_set_playing(!pl_playing);
    else pl_change_track(pl_armed == 2 ? 1 : -1), pl_skip_t[pl_armed == 2] = 0;
  }
  if (f.tap && pl_small_armed == 0) vol_open();
  if (f.tap && pl_small_armed == 1) pl_liked = !pl_liked;
  if (pl_armed >= 0) pl_last = pl_armed;
  if (!f.down) {
    if (pr_seek >= 0 && mu_n) {  // the finger left the line: play from there
      if (mu_cur != pl_cur) music_start_audio(), mu_cmd = pl_cur;
      mu_seek = (int32_t)(pr_seek * mu_tracks[pl_cur].samples);
    }
    pl_armed = pl_small_armed = -1, pr_line = false, pr_seek = -1;
  }
}

static void pr_palette(uint32_t c0, uint32_t c1, float a) {
  const uint32_t sig = (c0 * 31 + c1) * 16777619u ^ (uint32_t)(a * 256);
  if (sig == pr_pal_sig) return;
  pr_pal_sig = sig, pr_c0 = halo_dim(c0, (int)(a * 256)), pr_c1 = halo_dim(c1, (int)(a * 256));
  for (int i = 0; i < 256; i++) {
    const uint32_t c = pl_mix_rgb(pr_c0, pr_c1, i / 255.0f);
    const int r = c >> 16 & 255, g = c >> 8 & 255, b = c & 255;
    for (int k = 0; k < 4; k++) {  // (the offset: 0, 2/4, 3/4, 1/4 of a step)
      static const uint8_t OFF[4] = {0, 2, 3, 1};
      const int dr = OFF[k] * 2, dg = OFF[k];
      pr_pal[k][i] = halo_565((uint32_t)min(r + dr, 255) << 16 | (uint32_t)min(g + dg, 255) << 8 | (uint32_t)min(b + dr, 255));
    }
  }
}

static void pr_text_item(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, float a,
                         const HaloView &v, const HaloItem *on) {
  rg_text(it, f, s, mid, align, x, y, c, a, on);
  it.on_black = on == &halo_black;
  halo_view_commit(it, v);
}

static void player_layout(const HaloView &v) {
  pl_v = v;
  const float f_pl = vol_layout(v), pa = f_pl;
  const float s = v.s * PR_BIG_K, ka = pa * v.a;
  // The cover: the colours flow from the old track's into the new one's with the titles
  const int a_t = mu_n ? pl_prev % 8 : 0, b_t = mu_n ? pl_cur % 8 : 0;
  const float p = pl_change_t >= 0.3f ? 1 : halo_ease(EASE_PL_EMPH, pl_change_t / 0.3f);
  const uint32_t c0 = pl_mix_rgb(PR_COVERS[a_t][0], PR_COVERS[b_t][0], p), c1 = pl_mix_rgb(PR_COVERS[a_t][1], PR_COVERS[b_t][1], p);
  pr_palette(c0, c1, ka);
  PRC.x0 = halo_view_x(v, PR_COVER_X), PRC.y0 = halo_view_y(v, PR_COVER_Y), PRC.r = PR_COVER_R * v.s;
  PRC.x1 = halo_view_x(v, PR_COVER_X + PR_COVER_S), PRC.y1 = halo_view_y(v, PR_COVER_Y + PR_COVER_S);
  uint32_t hc = 2166136261u;
  for (float q : {PRC.x0, PRC.y0, PRC.x1, PRC.y1, ka, (float)(c0 >> 8), (float)(c1 >> 8), (float)(c0 & 255)}) hc = pl_mix(hc, q);
  pl_damage(pr_dmg_cover, ka > 0.004f ? hc | 1 : 1, PRC.x0, PRC.y0, PRC.x1, PRC.y1);
  HaloItem &n = pr_note_it;
  n.kind = HI_MASK, n.mask = pr_note.bits ? &pr_note : nullptr, n.x = PR_COVER_X + PR_COVER_S / 2, n.y = PR_COVER_Y + PR_COVER_S / 2, n.scale = 1;
  n.color = pl_mix_rgb(c0, 0xFFFFFF, 0.72f), n.alpha = pr_note.bits ? pa : 0, n.on_black = false, n.on = nullptr;
  halo_view_commit(n, v);
  // The time line
  const float len = mu_n ? pl_len[pl_cur] : 0, frac = pr_seek >= 0 ? pr_seek : len > 0 ? halo_clamp01(pl_elapsed / len) : 0;
  const float fx = PR_LINE_W * frac, kr = 11 + 3 * (pr_seek >= 0);
  pr_track.kind = HI_RRECT, pr_track.x = PR_LINE_X + PR_LINE_W / 2, pr_track.y = PR_LINE_Y, pr_track.w = PR_LINE_W, pr_track.h = 8, pr_track.r = 4;
  pr_track.scale = 1, pr_track.color = 0x3A3F44, pr_track.alpha = pa, pr_track.on_black = true, pr_track.on = nullptr;
  pr_fill.kind = HI_RRECT, pr_fill.x = PR_LINE_X + fmaxf(8, fx) / 2, pr_fill.y = PR_LINE_Y, pr_fill.w = fmaxf(8, fx), pr_fill.h = 8, pr_fill.r = 4;
  pr_fill.scale = 1, pr_fill.color = PR_ACC, pr_fill.alpha = pa, pr_fill.on_black = true, pr_fill.on = nullptr;
  pr_knob.kind = HI_CIRCLE, pr_knob.x = PR_LINE_X + fx, pr_knob.y = PR_LINE_Y, pr_knob.r = kr, pr_knob.scale = 1;
  pr_knob.color = PR_ACC, pr_knob.alpha = pa, pr_knob.on_black = true, pr_knob.on = nullptr;
  halo_view_commit(pr_track, v), halo_view_commit(pr_fill, v), halo_view_commit(pr_knob, v);
  const int e = (int)(pr_seek >= 0 ? pr_seek * len : pl_elapsed), l = (int)len;
  snprintf(pr_s0, sizeof pr_s0, "%d:%02d", e / 60, e % 60), snprintf(pr_s1, sizeof pr_s1, "%d:%02d", l / 60, l % 60);
  pr_text_item(pr_t0, gl_r_n24, pr_s0, pr_mid_n, 0, PR_LINE_X, PR_TIME_Y, GL_DIM, mu_n ? pa : 0, v, &halo_black);
  pr_text_item(pr_t1, gl_r_n24, pr_s1, pr_mid_n, 2, PR_LINE_X + PR_LINE_W, PR_TIME_Y, GL_DIM, mu_n ? pa : 0, v, &halo_black);
  // Previous and next: capsules; pressed they grow a little
  for (int i = 0; i < 2; i++) {
    const float cx = i ? PR_NEXT_X : PR_PREV_X, g = halo_clamp01(pl_grp[i ? 2 : 0].s.x);
    HaloItem &b = pl_side[i];
    b.kind = HI_RRECT, b.x = cx, b.y = PR_BTN_Y, b.w = PR_SIDE_W + 8 * g, b.h = PR_SIDE_H + 8 * g, b.r = 42 + 4 * g, b.scale = 1;
    b.color = PR_CARD, b.alpha = pa, b.on_black = true, b.on = nullptr;
    halo_view_commit(b, v);
    HaloItem &ic = pl_side_ic[i];
    const float t = pl_skip_t[i];
    ic.kind = HI_MASK, ic.mask = &pl_m_skip[i][t >= 0.5f ? 30 : min(29, (int)(t * 60))];
    ic.x = cx, ic.y = PR_BTN_Y, ic.scale = 1.1f, ic.color = PL_ON_SURFACE, ic.alpha = pa, ic.on_black = false, ic.on = nullptr;
    halo_view_commit(ic, v);
  }
  // The big button: the watch's wave and ring
  const bool mid = pl_last == 1;
  pl_ring_shape(BK.kb, false, mid ? 152 : 104, 128, s);
  pl_ring_shape(BK.kr, true, mid ? 176 : 128, 152, s);
  BK.cx = halo_view_x(v, PR_BIG_X), BK.cy = halo_view_y(v, PR_BTN_Y);
  const ShapeK &r = BK.kr, &k = BK.kb;
  const bool pr = r.m > 0.001f;
  BK.rk.hw = 3.2f * v.s;
  BK.r_in = (pr ? fminf(r.a - r.b, fminf(r.pw, r.ph)) : r.a - r.b) - BK.rk.hw - 1;
  BK.r_out = (pr ? fmaxf(r.a + r.b, fmaxf(r.pw, r.ph)) : r.a + r.b) + BK.rk.hw + 1;
  BK.b_in = (pr ? fminf(k.a - k.b, fminf(k.pw, k.ph)) : k.a - k.b) - 1;
  BK.b_out = (pr ? fmaxf(k.a + k.b, fmaxf(k.pw, k.ph)) : k.a + k.b) + 1;
  const float prog = frac * 2 * HALO_PI, gap = 4.05f * HALO_PI / 180;
  const float ends[4] = {gap, prog - gap, prog + gap, 2 * HALO_PI - gap};
  BK.rk.fill = sector_make(ends[0], ends[1]), BK.rk.track = sector_make(ends[2], ends[3]);
  for (int i = 0; i < 4; i++) {
    const float ux = sinf(ends[i]), uy = -cosf(ends[i]);
    float rr = 76 * s;
    for (int it = 0; it < 3; it++) rr -= shape_dist(BK.kr, ux * rr, uy * rr);
    BK.rk.cap[i][0] = BK.cx + ux * rr, BK.rk.cap[i][1] = BK.cy + uy * rr;
  }
  BK.rk.c_fill = halo_dim(PR_ACC, (int)(ka * 256)), BK.rk.c_track = halo_dim(0x3A3F44, (int)(ka * 256));
  BK.c_btn = halo_dim(PR_ACC, (int)(ka * 256));
  uint32_t h = 2166136261u;
  for (float q : {BK.cx, BK.cy, s, r.a, r.b, r.m, r.pw, k.pw, prog * 20, pl_rot * 400, ka}) h = pl_mix(h, q);
  pl_damage(pl_dmg_big, ka > 0.004f ? h | 1 : 1, BK.cx - BK.r_out - 2, BK.cy - BK.r_out - 2, BK.cx + BK.r_out + 2, BK.cy + BK.r_out + 2);
  HaloItem &bi = pl_big_ic;
  bi.kind = HI_MASK, bi.mask = &pl_m_pp[(int)lroundf(halo_clamp01(pl_pp.x) * 10)], bi.x = PR_BIG_X, bi.y = PR_BTN_Y, bi.scale = 1;
  bi.color = PL_ON_PRIMARY, bi.alpha = pa, bi.on_black = false, bi.on = nullptr;
  halo_view_commit(bi, v);
  // The volume and the heart
  for (int i = 0; i < 2; i++) {
    const float q = halo_clamp01(pl_small[i].s.x), cx = i ? PR_LIKE_X : PR_VOL_X;
    HaloItem &b = pl_sm[i];
    b.kind = HI_RRECT, b.x = cx, b.y = PR_BTN_Y, b.w = b.h = 2 * PR_SMALL_R, b.r = PR_SMALL_R + (18 - PR_SMALL_R) * q, b.scale = 1;
    b.color = PR_CARD, b.alpha = pa, b.on_black = true, b.on = nullptr;
    halo_view_commit(b, v);
    const float lm = halo_clamp01(pl_like_m.x), bounce = i ? 1 + 0.15f * fmaxf(0, pl_like_m.x - 1) * 4 : 1;
    gl_text_mid(pl_sm_ic[i], gl_r_ic36, i ? PR_ICON_HEART : PR_ICON_VOL, pr_mid_ic, cx, PR_BTN_Y, 0.85f * bounce,
                i ? pl_mix_rgb(PL_ON_SURFACE, PL_LIKE, lm) : PL_ON_SURFACE, pa, bounce == 1 ? &b : nullptr);
    halo_view_commit(pl_sm_ic[i], v);
  }
  // The title and the artist
  uint32_t ht = 2166136261u;
  for (float q : {(float)pl_cur, (float)pl_prev, pl_change_t < 0.45f ? pl_change_t : 9.0f, pl_marquee_x, ka, v.x, v.y, v.s}) ht = pl_mix(ht, q);
  pl_damage(pl_dmg_text, ka > 0.004f ? ht | 1 : 1, halo_view_x(v, PR_TEXT_X - 6), halo_view_y(v, PR_TITLE_Y - 42),
            halo_view_x(v, PR_TEXT_X + PL_TITLE_W + 6), halo_view_y(v, PR_ARTIST_Y + 26));
  if (pl_marquee_x > 0 || pl_change_t < 0.45f || pl_playing) halo_subpixel = true;
}

// The cover, row by row: the rounded square filled with the gradient straight from the palette; only the rounded edge blends
static void HALO_HOT pr_cover_draw() {
  const float x0 = PRC.x0, y0 = PRC.y0, x1 = PRC.x1, y1 = PRC.y1, r = PRC.r;
  if (!halo_hits(x0, y0, x1, y1)) return;
  const float S = x1 - x0, inv = 256 / (S * 1.393f);
  const int32_t dv = (int32_t)(0.574f * inv * 65536);
  const int ya = max(hb.y, halo_floor(y0)), yb = min(hb.y + hb.rows - 1, halo_floor(y1 - 0.001f));
  for (int y = ya; y <= yb; y++) {
    const float yc = y + 0.5f, dd = fmaxf(y0 + r - yc, yc - (y1 - r));
    float ins = 0;
    if (dd > 0) {
      if (dd >= r) continue;
      ins = r - halo_sqrt(r * r - dd * dd);
    }
    const float xa = x0 + ins, xb = x1 - ins;
    const int ia = -halo_floor(-xa), ib = halo_floor(xb) - 1;
    uint16_t *row = halo_row(y);
    const int a = max(ia, hb.x0), b = min(ib, hb.x1);
    const float base = (y + 0.5f - y0) * 0.819f * inv;
    if (a <= b) {
      int32_t v = (int32_t)((((a + 0.5f - x0) * 0.574f) * inv + base) * 65536);
      for (int x = a; x <= b; x++, v += dv) {
        const int i = v >> 16;
        row[x] = pr_pal[(x & 1) | (y & 1) << 1][i > 255 ? 255 : i < 0 ? 0 : i];
      }
    }
    if (ins > 0) {  // the soft edge of the corners
      const float t = halo_clamp01(base / 256 + 0.0f);
      const uint32_t c = pl_mix_rgb(pr_c0, pr_c1, t);
      const int la = ia - 1, rb = ib + 1;
      if (la >= hb.x0 && la <= hb.x1) pl_blend(row + la, c, (int)((ia - xa) * 256));
      if (rb >= hb.x0 && rb <= hb.x1) pl_blend(row + rb, c, (int)((xb - rb) * 256));
    }
  }
}

// New track: the old lines slide out and fade, the new ones slide in; a title too long for its place is a marquee
static void pr_texts(float alpha) {
  const HaloView &v = pl_v;
  const float xa = halo_view_x(v, PR_TEXT_X), xb = halo_view_x(v, PR_TEXT_X + PL_TITLE_W);
  if (!mu_n) {
    halo_text(gl_r_t56, "No music", xa, halo_view_y(v, PR_TITLE_Y) - pl_mid_t * v.s, halo_dim(PL_ON_SURFACE, (int)(alpha * 256)), 1, 0, v.s, 0, 0);
    return;
  }
  for (int line = 0; line < 2; line++) {
    const float y = line ? PR_ARTIST_Y : PR_TITLE_Y, mid = line ? pl_mid_a : pl_mid_t, d_out = line ? 0.03f : 0, d_in = line ? 0.09f : 0.06f;
    const float sh = 0.125f * PL_TITLE_W * pl_change_dir;
    const HaloFont &f = line ? PL_F_ARTIST : PL_F_TITLE;
    for (int which = 0; which < 2; which++) {
      if (!which && pl_change_t >= 0.3f + d_out) continue;
      const int t = which ? pl_cur : pl_prev;
      const float p = halo_ease(EASE_PL_EMPH, (pl_change_t - (which ? d_in : d_out)) / 0.3f);
      const float a = (which ? p : 1 - p) * alpha, off = which ? -sh * (1 - p) : sh * p;
      if (a <= 0.01f) continue;
      const char *s = line ? mu_tracks[t].artist : mu_tracks[t].title;
      const float tw = line ? pl_aw[t] : pl_tw[t], top = halo_view_y(v, y) - mid * v.s;
      const uint32_t c = halo_dim(line ? GL_DIM : PL_ON_SURFACE, (int)(a * 256));
      const float x0 = halo_view_x(v, PR_TEXT_X - (which && !line ? pl_marquee_x : 0) + off);
      pl_text(f, s, x0, top, v.s, c, xa, xb);
      if (!line && tw > PL_TITLE_W) {
        pl_text(f, s, x0 + (tw + 96 * DP) * v.s, top, v.s, c, xa, xb);
        const int y0 = (int)top, y1 = (int)(top + f.line * v.s);
        for (int e = 0; e < 16; e++) pl_fade_col((int)xb - 1 - e, e / 16.0f, y0, y1);
        if (pl_marquee_x > 0)
          for (int e = 0; e < 16; e++) pl_fade_col((int)xa + e, e / 16.0f, y0, y1);
      }
    }
  }
}

static void player_draw() {
  const float vs = halo_clamp01(vol_screen.x), pa = halo_clamp01(1 - vs / 0.4f) * pl_v.a;
  if (pa > 0.004f) {
    pr_cover_draw();
    halo_item_draw(pr_note_it);
    pr_texts(pa);
    halo_item_draw(pr_track), halo_item_draw(pr_fill), halo_item_draw(pr_knob);
    halo_item_draw(pr_t0), halo_item_draw(pr_t1);
    for (int i = 0; i < 2; i++) halo_item_draw(pl_side[i]), halo_item_draw(pl_side_ic[i]);
    shape_draw(BK.cx, BK.cy, BK.kr, BK.r_in, BK.r_out, 0, &BK.rk);
    shape_draw(BK.cx, BK.cy, BK.kb, BK.b_in, BK.b_out, BK.c_btn, nullptr);
    halo_item_draw(pl_big_ic);
    for (int i = 0; i < 2; i++) halo_item_draw(pl_sm[i]), halo_item_draw(pl_sm_ic[i]);
  }
  vol_draw();
}
