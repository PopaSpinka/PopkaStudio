// recorder_rect.h — the Recorder on a rectangular screen (variant R4 of web/mockups/rect-apps.html): the timer takes the
// middle of the screen, the wave is a thin strip of dots under it (63 columns, 5 rows, 15 px apart, unlit dots stay faintly
// on), three buttons below. The microphones, the recording and the columns are recorder.h's; here — only where the parts
// stand and how the dots are painted. Included from recorder.h in place of its round layout.

extern const HaloFont gl_r_n190, gl_r_t28, gl_r_ic36, gl_r_title;

#define RR_TITLE_X 32.0f
#define RR_TITLE_Y 45.0f
#define RR_TIME_Y 214.0f
#define RR_STATE_Y 345.0f
#define RR_WAVE_Y 426.0f
#define RR_BTN_Y 530.0f
#define RR_SIDE_W 200.0f
#define RR_SIDE_H 96.0f
#define RR_BIG_R 54.0f
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

static const float RR_BX[3] = {312, 512, 712};
static uint16_t rr_pal[RR_C_N][17];    // colour (lit, peak, faint, not played) by brightness
static uint8_t rr_cov[RR_DOT][RR_DOT];  // the round dot, 0..16 in each pixel
static float rr_mid_time, rr_mid_state, rr_time_w, rr_state_w;
static const char *rr_state_s = "Ready";

static void rr_build() {
  rc_mid_title = halo_text_mid(gl_r_title, "Recorder"), rr_mid_time = halo_text_mid(gl_r_n190, "0"), rc_mid_ic = halo_text_mid(gl_r_ic36, RR_ICON_VOL);
  rr_mid_state = halo_text_mid(gl_r_t28, "Recording");
  static const uint32_t COL[RR_C_N] = {RR_LIT, RR_PEAK, RR_FAINT, RR_GREY};
  for (int c = 0; c < RR_C_N; c++)
    for (int a = 0; a <= 16; a++) rr_pal[c][a] = halo_565(rc_mix(0, COL[c], a / 16.0f));
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
  rr_text(rc_title, gl_r_title, "Recorder", rc_mid_title, 0, RR_TITLE_X, RR_TITLE_Y, RC_TEXT, v);
  const float m = halo_clamp01(rc_recm.x), has = halo_clamp01(rc_hasm.x), pp = halo_clamp01(rc_ppm.x);
  rr_text(rc_time, gl_r_n190, rc_time_s, rr_mid_time, 1, CX, RR_TIME_Y, rc_mix(0x9E9E9E, RC_TEXT, fmaxf(has, m)), v);
  // What is going on: a red dot and the word, the two centred together
  const char *state = rc_state == RC_REC ? "Recording" : rc_state == RC_PLAY ? "Playing" : rc_len ? "Recorded" : "Ready";
  if (state != rr_state_s || !rr_state_w) rr_state_s = state, rr_state_w = halo_text_width(gl_r_t28, state);
  const float dot = 22 * m, x0 = CX - (rr_state_w + dot) / 2;
  rr_text(rc_card, gl_r_t28, rr_state_s, rr_mid_state, 0, x0 + dot, RR_STATE_Y, 0x9A9A9F, v);
  const float pulse = rc_state == RC_REC ? 0.82f + 0.18f * cosf(rc_clock * 2 * HALO_PI * 0.8f) : 1;
  for (HaloItem *it : {&rc_glow, &rc_dot}) {
    const bool g = it == &rc_glow;
    it->kind = HI_CIRCLE, it->x = x0 + 5, it->y = RR_STATE_Y, it->r = g ? 14 : 9, it->scale = 1;
    it->color = RC_RED, it->alpha = m * pulse * (g ? 0.22f : 1), it->on_black = true, it->on = nullptr;
    halo_view_commit(*it, v);
  }
  // The columns (drawn in rr_wave_draw)
  const float played = rc_len ? (float)rc_pos / rc_len : 0;
  const bool prog = (rc_state == RC_PLAY || rc_state == RC_DONE) && rc_pos > 0;
  uint32_t h = 2166136261u;
  for (int c = 0; c <= RC_N; c++) {
    RcCol &q = rc_cols[c];
    const float x = RC_X0 + (c - 1) * RC_P + rc_xoff;
    const float fade = halo_clamp01((x - (RC_X0 - RC_P)) / RC_P) * halo_clamp01((RC_XR + RC_P - x) / RC_P);
    q.x = (int16_t)lroundf(x) - RR_DOT / 2, q.fade = (uint8_t)lroundf(fade * 16);
    q.nb = rc_db[c] < 0.5f ? 0 : (uint8_t)min(RC_ROWS, (int)(rc_db[c] + 0.5f));
    q.np = q.nb ? (uint8_t)max((int)q.nb, min(RC_ROWS, (int)(rc_dp[c] + 0.5f))) : 0;
    q.grey = prog && (c - 0.5f) / RC_N > played;
    h = (h ^ (uint32_t)(q.x << 20 | q.nb << 16 | q.np << 8 | q.grey << 5 | q.fade)) * 16777619u;
  }
  for (float qv : {v.x, v.y, v.s, v.a}) h = (h ^ (uint32_t)(int32_t)(qv * 512)) * 16777619u;
  const float ry = (RC_ROWS - 1) * RC_P + RR_DOT;
  const float r[4] = {halo_view_x(v, RC_X0 - RC_P - RR_DOT), halo_view_y(v, RR_WAVE_Y - ry), halo_view_x(v, RC_XR + RC_P + RR_DOT), halo_view_y(v, RR_WAVE_Y + ry)};
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
  const bool fast = v.s == 1 && v.a >= 0.999f && v.x == (int)v.x && v.y == (int)v.y;
  const float ry = (RC_ROWS - 1) * RC_P + RR_DOT;
  if (!halo_hits(halo_view_x(v, RC_X0 - RC_P - RR_DOT), halo_view_y(v, RR_WAVE_Y - ry), halo_view_x(v, RC_XR + RC_P + RR_DOT), halo_view_y(v, RR_WAVE_Y + ry))) return;
  for (int c = 0; c <= RC_N; c++) {
    const RcCol &q = rc_cols[c];
    if (!q.fade) continue;
    for (int j = 0; j < RC_ROWS; j++) {
      const bool lit = q.nb && j < q.nb, peak = !lit && j < q.np;
      const int col = lit ? (q.grey ? 3 : 0) : peak ? 1 : 2, a = (16 * q.fade + 8) >> 4;
      if (!a) continue;
      for (int sgn = j ? -1 : 1; sgn <= 1; sgn += 2) {
        const int y = (int)RR_WAVE_Y + sgn * j * RC_P - RR_DOT / 2;
        if (fast) {
          const int x = q.x + (int)v.x, yy = y + (int)v.y;
          if (yy + RR_DOT <= hb.y || yy >= hb.y + hb.rows || x + RR_DOT <= hb.x0 || x > hb.x1) continue;
          rr_dot(x, yy, rr_pal[col], a);
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
    rr_wave_draw();
    for (const auto &b : rc_btn) halo_item_draw(b);
    halo_item_draw(rc_rec_ic), halo_item_draw(rc_play_ic), halo_item_draw(rc_pause_ic), halo_item_draw(rc_vol_ic);
  }
  vol_draw();
}
