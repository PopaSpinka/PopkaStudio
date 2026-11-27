// recorder_rect.h — the Recorder on a rectangular screen (variant R4 of web/mockups/rect-apps.html): the timer takes the
// middle of the screen, the wave is a thin strip of dots under it (63 columns, 5 rows, 15 px apart, unlit dots stay faintly
// on), three buttons below. The microphones, the recording and the columns are recorder.h's; here — only where the parts
// stand and how the dots are painted. Included from recorder.h in place of its round layout.
// The 7″ board held tall (HALO_P4_TALL, 600 x 1024: variant 9A of web/mockups/rect-tall.html) — the same stack: the timer
// in the middle, the band of dots under it 35 columns wide and 7 rows tall (recorder.h's 63 columns and 3 rows from the
// middle, shown here: while recording — the newest 35, one a slice, scrolling as they do; else the whole recording
// gathered into 35; the rows stretched to 4 from the middle), the three buttons at thumb height.

extern const HaloFont gl_r_n190, gl_r_t28, gl_r_ic36, gl_r_title;

#if defined(HALO_WATCH)  // (the rectangular watch: no title — the timer is at the top left, the time in the middle)
#define RR_TITLE_X 26.0f
#define RR_TITLE_Y 84.0f
#define RR_TITLE_S ""
#elif defined(HALO_SQUARE)  // (the square's titles: as its other screens')
#define RR_TITLE_X 16.0f
#define RR_TITLE_Y 30.0f
#else
#define RR_TITLE_X 32.0f
#define RR_TITLE_Y 45.0f
#endif
#ifndef RR_TITLE_S
#define RR_TITLE_S "Recorder"
#endif
#if defined(HALO_P4_TALL) || defined(HALO_SQUARE) || defined(HALO_WATCH)
#if defined(HALO_WATCH)  // the rectangular watch (variant 2B of web/mockups/watch206-media.html): the square's, by its size
#define RR_TIME_X 26.0f
#define RR_TIME_Y 98.0f
#define RR_STATE_R (HALO_W - 26.0f)
#define RR_STATE_Y 98.0f
#define RR_WAVE_Y 256.0f
#define RR_CARD_Y 256.0f
#define RR_CARD_W (HALO_W - 28.0f)
#define RR_CARD_H 220.0f
#define RR_BTN_Y 434.0f
#define RR_SIDE_W 110.0f
#define RR_SIDE_H 76.0f
#define RR_BIG_R 48.0f
#define RR_N 24
#define RR_ROWS 7
#define RR_TIME_FONT gl_q_n64
static const float RR_BX[3] = {72, CX, HALO_W - 72};
extern const HaloFont gl_q_n64;  // (studio.py: WATCH_APP_FONTS)
#elif defined(HALO_SQUARE)  // the square (variant 2B of web/mockups/square-media.html), as on the round board: the timer smaller at
                    // the top left with what is going on at the right, the dots filling a dark card in the middle (28
                    // columns, 13 rows), the three buttons under it
#define RR_TIME_X 16.0f     // the timer: from its left end
#define RR_TIME_Y 92.0f
#define RR_STATE_R 464.0f   // what is going on: its right end
#define RR_STATE_Y 92.0f
#define RR_WAVE_Y 242.0f
#define RR_CARD_Y 242.0f    // the card under the dots: its middle, its size
#define RR_CARD_W 448.0f
#define RR_CARD_H 220.0f
#define RR_BTN_Y 410.0f
#define RR_SIDE_W 132.0f
#define RR_SIDE_H 72.0f
#define RR_BIG_R 44.0f
#define RR_N 28
#define RR_ROWS 7
#define RR_TIME_FONT gl_q_n64
static const float RR_BX[3] = {82, 240, 398};
extern const HaloFont gl_q_n64;  // (studio.py: SQUARE_APP_FONTS)
#else
#define RR_TIME_Y 330.0f
#define RR_STATE_Y 470.0f
#define RR_WAVE_Y 601.0f
#define RR_BTN_Y 860.0f
#define RR_SIDE_W 168.0f
#define RR_SIDE_H 96.0f
#define RR_BIG_R 56.0f
#define RR_N 35             // the columns shown
#define RR_ROWS 4           // rows from the middle, it included
static const float RR_BX[3] = {116, 300, 484};
#endif
#define RR_X0 (CX - (RR_N - 1) * RC_P / 2)
#define RR_XR (RR_X0 + (RR_N - 1) * RC_P)
#define RR_DB rr_db
#define RR_DP rr_dp
static float rr_db[RR_N + 1], rr_dp[RR_N + 1];  // what the columns show (as rc_db, rc_dp: [0] — the one leaving on the left)
static uint32_t rr_ms;
// The columns shown, from recorder.h's: recording — its newest 35 as they are; else the whole recording gathered into
// 35 (the loudness — the mean, the peak — the highest), eased there as rc_columns eases its own (~70 ms); the rows
// stretched from 3 to 4 from the middle
static void rr_columns() {
  const uint32_t now = halo_ms();
  const float k = rc_state == RC_REC ? 1 : fminf(1, (now - rr_ms) * (14 / 1000.0f));
  rr_ms = now;
  auto rows = [](float r) { return r < 1 ? r : 1 + (r - 1) * ((RR_ROWS - 1) / (float)(RC_ROWS - 1)); };
  for (int d = 0; d <= RR_N; d++) {
    float b = 0, p = 0;
    if (rc_state == RC_REC) {
      b = rc_db[RC_N - RR_N + d], p = rc_dp[RC_N - RR_N + d];
    } else if (d) {
      const int a = (d - 1) * RC_N / RR_N, e = max(a + 1, d * RC_N / RR_N);
      for (int i = a; i < e; i++) b += rc_db[i + 1], p = fmaxf(p, rc_dp[i + 1]);
      b /= e - a;
    }
    rr_db[d] += (rows(b) - rr_db[d]) * k, rr_dp[d] += (rows(p) - rr_dp[d]) * k;
  }
}
#else
#define RR_TIME_Y 214.0f
#define RR_STATE_Y 345.0f
#define RR_WAVE_Y 426.0f
#define RR_BTN_Y 530.0f
#define RR_SIDE_W 200.0f
#define RR_SIDE_H 96.0f
#define RR_BIG_R 54.0f
#define RR_N RC_N
#define RR_ROWS RC_ROWS
#define RR_X0 RC_X0
#define RR_XR RC_XR
#define RR_DB rc_db
#define RR_DP rc_dp
static const float RR_BX[3] = {312, 512, 712};
#endif
#define RR_DOT 9            // a dot's square, px
#define RR_LIT 0xFFAB91     // the loudness
#define RR_PEAK 0x7A5249    // up to the peak
#define RR_FAINT 0x24282B   // unlit
#define RR_GREY 0x6B6B70    // not played yet
#define RR_CARD 0x2D3236
#define RR_ICON_VOL "\xEF\x80\xA8"
#define RR_ICON_PLAY "\xEF\x81\x8B"
#define RR_ICON_PAUSE "\xEF\x81\x8C"
#define RR_C_N 4

static uint16_t rr_pal[RR_C_N][17];    // colour (lit, peak, faint, not played) by brightness
static uint8_t rr_cov[RR_DOT][RR_DOT];  // the round dot, 0..16 in each pixel
static float rr_mid_time, rr_mid_state, rr_time_w, rr_state_w;
static const char *rr_state_s = "Ready";

#ifndef RR_TIME_FONT
#define RR_TIME_FONT gl_r_n190
#endif
#ifdef RR_CARD_W
static HaloItem rr_card;  // (the square: the card under the dots)
#define RR_DOT_BG RR_CARD  // (the dots' soft edges: on the card)
#else
#define RR_DOT_BG 0        // (on black)
#endif

static void rr_build() {
  rc_mid_title = halo_text_mid(gl_r_title, "Recorder"), rr_mid_time = halo_text_mid(RR_TIME_FONT, "0"), rc_mid_ic = halo_text_mid(gl_r_ic36, RR_ICON_VOL);
  rr_mid_state = halo_text_mid(gl_r_t28, "Recording");
  static const uint32_t COL[RR_C_N] = {RR_LIT, RR_PEAK, RR_FAINT, RR_GREY};
  for (int c = 0; c < RR_C_N; c++)
    for (int a = 0; a <= 16; a++) rr_pal[c][a] = halo_565(rc_mix(RR_DOT_BG, COL[c], a / 16.0f));
  const float R = RR_DOT / 2.0f;
  for (int y = 0; y < RR_DOT; y++)
    for (int x = 0; x < RR_DOT; x++) {
      int in = 0;
      for (int sy = 0; sy < 4; sy++)
        for (int sx = 0; sx < 4; sx++) {
          const float dx = x + (sx + 0.5f) / 4 - R, dy = y + (sy + 0.5f) / 4 - R;
          in += dx * dx + dy * dy <= R * R;
        }
      rr_cov[y][x] = (uint8_t)in;
    }
}

static void recorder_update(float dt) {
  HaloFinger &f = halo_finger;
  rc_seen = halo_ms(), rc_active = true, rc_mic_want = true;
  if (vol_update(dt)) return void(rc_armed = -1);
  if (f.pressed && !f.owner) {
    rc_armed = -1;
    for (int i = 0; i < 3; i++) {
      const float dx = fabsf(f.x0 - RR_BX[i]), dy = fabsf(f.y0 - RR_BTN_Y);
      const bool in = i == 1 ? dx * dx + dy * dy < (RR_BIG_R + 10) * (RR_BIG_R + 10) : dx < RR_SIDE_W / 2 + 6 && dy < RR_SIDE_H / 2 + 8;
      if (in && (i != 1 || rc_has())) rc_armed = i;
    }
  }
  if (f.dragging) rc_armed = -1;
  if (f.tap && rc_armed >= 0) rc_action(rc_armed, (halo_ms() - f.t0) / 1000.0f);
  if (!f.down) rc_armed = -1;
}

static void rr_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, const HaloView &v) {
  rg_text(it, f, s, mid, align, x, y, c, 1, &halo_black);
  it.on_black = true;
  halo_view_commit(it, v);
}

static void recorder_layout(const HaloView &view) {
  rc_fa = vol_layout(view);
  HaloView v = view;
  v.a *= rc_fa;
  rc_v = v;
  rr_text(rc_title, gl_r_title, RR_TITLE_S, rc_mid_title, 0, RR_TITLE_X, RR_TITLE_Y, RC_TEXT, v);
  const float m = halo_clamp01(rc_recm.x), has = halo_clamp01(rc_hasm.x), pp = halo_clamp01(rc_ppm.x);
#ifdef RR_TIME_X
  rr_text(rc_time, RR_TIME_FONT, rc_time_s, rr_mid_time, 0, RR_TIME_X, RR_TIME_Y, rc_mix(0x9E9E9E, RC_TEXT, fmaxf(has, m)), v);
#else
  rr_text(rc_time, RR_TIME_FONT, rc_time_s, rr_mid_time, 1, CX, RR_TIME_Y, rc_mix(0x9E9E9E, RC_TEXT, fmaxf(has, m)), v);
#endif
  // What is going on: a red dot and the word, the two centred together
  const char *state = rc_state == RC_REC ? "Recording" : rc_state == RC_PLAY ? "Playing" : rc_len ? "Recorded" : "Ready";
  if (state != rr_state_s || !rr_state_w) rr_state_s = state, rr_state_w = halo_text_width(gl_r_t28, state);
#ifdef RR_STATE_R
  const float dot = 22 * m, x0 = RR_STATE_R - rr_state_w - dot;  // (by its right end)
#else
  const float dot = 22 * m, x0 = CX - (rr_state_w + dot) / 2;
#endif
  rr_text(rc_card, gl_r_t28, rr_state_s, rr_mid_state, 0, x0 + dot, RR_STATE_Y, 0x9A9A9F, v);
  const float pulse = rc_state == RC_REC ? 0.82f + 0.18f * cosf(rc_clock * 2 * HALO_PI * 0.8f) : 1;
  for (HaloItem *it : {&rc_glow, &rc_dot}) {
    const bool g = it == &rc_glow;
    it->kind = HI_CIRCLE, it->x = x0 + 5, it->y = RR_STATE_Y, it->r = g ? 14 : 9, it->scale = 1;
    it->color = RC_RED, it->alpha = m * pulse * (g ? 0.22f : 1), it->on_black = true, it->on = nullptr;
    halo_view_commit(*it, v);
  }
#ifdef RR_CARD_W
  rr_card.kind = HI_RRECT, rr_card.x = CX, rr_card.y = RR_CARD_Y, rr_card.w = RR_CARD_W, rr_card.h = RR_CARD_H, rr_card.r = 22;
  rr_card.scale = 1, rr_card.color = RR_CARD, rr_card.alpha = 1, rr_card.on_black = true, rr_card.on = nullptr;
  halo_view_commit(rr_card, v);
#endif
  // The columns (drawn in rr_wave_draw)
  const float played = rc_len ? (float)rc_pos / rc_len : 0;
  const bool prog = (rc_state == RC_PLAY || rc_state == RC_DONE) && rc_pos > 0;
#if defined(HALO_P4_TALL) || defined(HALO_SQUARE) || defined(HALO_WATCH)
  rr_columns();
#endif
  uint32_t h = 2166136261u;
  for (int c = 0; c <= RR_N; c++) {
    RcCol &q = rc_cols[c];
    const float x = RR_X0 + (c - 1) * RC_P + rc_xoff;
    const float fade = halo_clamp01((x - (RR_X0 - RC_P)) / RC_P) * halo_clamp01((RR_XR + RC_P - x) / RC_P);
    q.x = (int16_t)lroundf(x) - RR_DOT / 2, q.fade = (uint8_t)lroundf(fade * 16);
    q.nb = RR_DB[c] < 0.5f ? 0 : (uint8_t)min(RR_ROWS, (int)(RR_DB[c] + 0.5f));
    q.np = q.nb ? (uint8_t)max((int)q.nb, min(RR_ROWS, (int)(RR_DP[c] + 0.5f))) : 0;
    q.grey = prog && (c - 0.5f) / RR_N > played;
    h = (h ^ (uint32_t)(q.x << 20 | q.nb << 16 | q.np << 8 | q.grey << 5 | q.fade)) * 16777619u;
  }
  for (float qv : {v.x, v.y, v.s, v.a}) h = (h ^ (uint32_t)(int32_t)(qv * 512)) * 16777619u;
  const float ry = (RR_ROWS - 1) * RC_P + RR_DOT;
  const float r[4] = {halo_view_x(v, RR_X0 - RC_P - RR_DOT), halo_view_y(v, RR_WAVE_Y - ry), halo_view_x(v, RR_XR + RC_P + RR_DOT), halo_view_y(v, RR_WAVE_Y + ry)};
  if (h != rc_dmg_h || memcmp(r, rc_dmg_r, sizeof r)) {
    if (rc_dmg_h) halo_damage_f(rc_dmg_r[0], rc_dmg_r[1], rc_dmg_r[2], rc_dmg_r[3]);
    halo_damage_f(r[0], r[1], r[2], r[3]);
    rc_dmg_h = h, memcpy(rc_dmg_r, r, sizeof r);
  }
  // The buttons: record / stop, play / pause, the volume
  float sc[3];
  for (int i = 0; i < 3; i++) {
    sc[i] = 1 - 0.06f * halo_clamp01(rc_press[i].s.x);
    HaloItem &b = rc_btn[i];
    if (i == 1) {
      b.kind = HI_CIRCLE, b.x = RR_BX[1], b.y = RR_BTN_Y, b.r = RR_BIG_R * sc[1], b.scale = 1, b.color = rc_mix(RC_GREY, RR_LIT, has);
    } else {
      b.kind = HI_RRECT, b.x = RR_BX[i], b.y = RR_BTN_Y, b.w = RR_SIDE_W * sc[i], b.h = RR_SIDE_H * sc[i], b.r = RR_SIDE_H / 2 * sc[i], b.scale = 1;
      b.color = RR_CARD;
    }
    b.alpha = 1, b.on_black = true, b.on = nullptr;
    halo_view_commit(b, v);
  }
  HaloItem &ri = rc_rec_ic;  // the red dot turns into a red square
  const float side = (40 + (36 - 40) * m) * sc[0];
  ri.kind = HI_RRECT, ri.x = RR_BX[0], ri.y = RR_BTN_Y, ri.w = ri.h = side, ri.r = (20 + (8 - 20) * m) * sc[0], ri.scale = 1;
  ri.color = RC_RED, ri.alpha = 1, ri.on_black = false, ri.on = nullptr;
  halo_view_commit(ri, v);
  const uint32_t ic = rc_mix(RC_OFF, 0x2A1D1A, has);
  gl_text_mid(rc_play_ic, gl_r_ic36, RR_ICON_PLAY, rc_mid_ic, RR_BX[1] + 3, RR_BTN_Y, 1.25f * sc[1], ic, 1 - pp, &rc_btn[1]);
  gl_text_mid(rc_pause_ic, gl_r_ic36, RR_ICON_PAUSE, rc_mid_ic, RR_BX[1], RR_BTN_Y, 1.25f * sc[1], ic, pp, &rc_btn[1]);
  gl_text_mid(rc_vol_ic, gl_r_ic36, RR_ICON_VOL, rc_mid_ic, RR_BX[2], RR_BTN_Y, 1.1f * sc[2], RC_TEXT, 1, &rc_btn[2]);
  for (HaloItem *it : {&rc_play_ic, &rc_pause_ic, &rc_vol_ic}) halo_view_commit(*it, v);
}

// One dot, straight into this task's part of the band: on black, every pixel from the palette
static inline void rr_dot(int x, int y, const uint16_t *pal, int a) {
  const int y0 = max(y, hb.y), y1 = min(y + RR_DOT - 1, hb.y + hb.rows - 1), xa = max(x, hb.x0), xb = min(x + RR_DOT - 1, hb.x1);
  for (int yy = y0; yy <= y1; yy++) {
    uint16_t *row = halo_row(yy);
    const uint8_t *cov = rr_cov[yy - y];
    for (int xx = xa; xx <= xb; xx++)
      if (const int k = cov[xx - x]) row[xx] = pal[(a * k + 8) >> 4];
  }
}

static void rr_wave_draw() {
  const HaloView &v = rc_v;
  if (v.a <= 0.004f) return;
#if defined(HALO_SQUARE) || defined(HALO_WATCH)  // (a transition here only moves and fades the screen: the dots stay dots from palettes dimmed to it,
                    // on whole pixels — 377 dots drawn one by one as rounded squares were 25 ms a frame on the S3)
  const bool fast = v.s == 1;
  static uint16_t pal_dim[RR_C_N][17];
  static float pal_a = -1;
  if (fast && v.a < 0.999f && v.a != pal_a) {
    static const uint32_t COL[RR_C_N] = {RR_LIT, RR_PEAK, RR_FAINT, RR_GREY};
    const int ka = (int)(v.a * 256);
    for (int c = 0; c < RR_C_N; c++)
      for (int k = 0; k <= 16; k++) pal_dim[c][k] = halo_565(rc_mix(halo_dim(RR_DOT_BG, ka), halo_dim(COL[c], ka), k / 16.0f));
    pal_a = v.a;
  }
  uint16_t(*const pal)[17] = v.a < 0.999f ? pal_dim : rr_pal;
  const int vx = (int)lroundf(v.x), vy = (int)lroundf(v.y);
#else
  const bool fast = v.s == 1 && v.a >= 0.999f && v.x == (int)v.x && v.y == (int)v.y;
  uint16_t(*const pal)[17] = rr_pal;
  const int vx = (int)v.x, vy = (int)v.y;
#endif
  const float ry = (RR_ROWS - 1) * RC_P + RR_DOT;
  if (!halo_hits(halo_view_x(v, RR_X0 - RC_P - RR_DOT), halo_view_y(v, RR_WAVE_Y - ry), halo_view_x(v, RR_XR + RC_P + RR_DOT), halo_view_y(v, RR_WAVE_Y + ry))) return;
  for (int c = 0; c <= RR_N; c++) {
    const RcCol &q = rc_cols[c];
    if (!q.fade) continue;
    for (int j = 0; j < RR_ROWS; j++) {
      const bool lit = q.nb && j < q.nb, peak = !lit && j < q.np;
      const int col = lit ? (q.grey ? 3 : 0) : peak ? 1 : 2, a = (16 * q.fade + 8) >> 4;
      if (!a) continue;
      for (int sgn = j ? -1 : 1; sgn <= 1; sgn += 2) {
        const int y = (int)RR_WAVE_Y + sgn * j * RC_P - RR_DOT / 2;
        if (fast) {
          const int x = q.x + vx, yy = y + vy;
          if (yy + RR_DOT <= hb.y || yy >= hb.y + hb.rows || x + RR_DOT <= hb.x0 || x > hb.x1) continue;
          rr_dot(x, yy, pal[col], a);
        } else {  // (a transition: scaled, dimmed)
          static const uint32_t COL[RR_C_N] = {RR_LIT, RR_PEAK, RR_FAINT, RR_GREY};
          const float x0 = halo_view_x(v, q.x), y0 = halo_view_y(v, y), x1 = halo_view_x(v, q.x + RR_DOT), y1 = halo_view_y(v, y + RR_DOT);
          if (halo_hits(x0, y0, x1, y1)) halo_rrect(x0, y0, x1, y1, (x1 - x0) / 2, halo_dim(COL[col], (int)(v.a * a * 16)), 1);
        }
      }
    }
  }
}

static void recorder_draw() {
  if (rc_fa > 0.004f) {
    halo_item_draw(rc_title), halo_item_draw(rc_time);
    halo_item_draw(rc_glow), halo_item_draw(rc_dot), halo_item_draw(rc_card);
#ifdef RR_CARD_W
    halo_item_draw(rr_card);
#endif
    rr_wave_draw();
    for (const auto &b : rc_btn) halo_item_draw(b);
    halo_item_draw(rc_rec_ic), halo_item_draw(rc_play_ic), halo_item_draw(rc_pause_ic), halo_item_draw(rc_vol_ic);
  }
  vol_draw();
}
