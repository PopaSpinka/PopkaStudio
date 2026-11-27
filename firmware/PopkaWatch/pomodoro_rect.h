// pomodoro_rect.h — Pomodoro on a rectangle (the user's choice of web/mockups/pomodoro-rect.html, 20 Nov 2026: variant
// B and settings S2). The ring at the round screen's edge becomes a frame at the rectangle's edge: what is left runs
// along it from the top's middle clockwise, brighter toward its head, a glowing dot on it; inside — as on the round
// board: the mode's icon, the time, the count's ticks, the mode's name, Reset · START / PAUSE · Skip at the bottom, the
// gear in the top right corner. When an interval runs out the frame flashes three times in the next mode's colour (on a
// board without a speaker — the WT32 — that is the alert). Settings: one screen — every duration's row with its own − and
// +, the switches under them (the 7″ board and the WT32 held wide: Duration at the left, Notifications at the right).
// The timer, its flash and the finger are pomodoro.h's; here — only where the parts stand. Five screens: the 7″ board
// Wide (1024 x 600) and Tall (HALO_P4_TALL, 600 x 1024), the 4″ square (HALO_SQUARE), the WT32 Tall (320 x 480) and Wide
// (HALO_LANDSCAPE, 480 x 320). Every target is at least 9 mm (60 px on these screens), the text at rest at scale 1.
// Included from pomodoro.h in place of the round board's layout.

// Fonts: the 7″ board's (studio.py RECT_APP_FONTS), a small rectangle's — its own (COMPACT_APP_FONTS: the 7″ ones
// × 0.7 would not fit), the square's time (SQUARE_APP_FONTS); the words — the rectangle's own (RECT_FONTS, COMPACT_FONTS)
extern const HaloFont gl_r_title, gl_r_t28, gl_r_t32;
#ifdef HALO_COMPACT
extern const HaloFont gl_c_t14, gl_c_pm_ic24, gl_c_pm_ic22, gl_c_pm_n24;
#if defined(HALO_SQUARE) || defined(HALO_WATCH)  // (the watch: the square's time and rows — WATCH_APP_FONTS)
extern const HaloFont gl_q_pm_n104;
#define PMR_F_TIME gl_q_pm_n104
#define PMR_F_ROW gl_r_t28
#else
extern const HaloFont gl_c_pm_n84;
#define PMR_F_TIME gl_c_pm_n84
#define PMR_F_ROW gl_c_t14
#endif
#define PMR_F_MODE gl_c_pm_ic24
#define PMR_F_IC gl_c_pm_ic22
#define PMR_F_VAL gl_c_pm_n24
#define PMR_F_NAME gl_c_t14
#define PMR_F_SMALL gl_c_t14
#else
extern const HaloFont gl_r_pm_n190, gl_r_pm_ic44, gl_r_pm_ic30, gl_r_pm_n40, gl_r_pm_t18;
#define PMR_F_TIME gl_r_pm_n190
#define PMR_F_MODE gl_r_pm_ic44
#define PMR_F_IC gl_r_pm_ic30
#define PMR_F_VAL gl_r_pm_n40
#define PMR_F_NAME gl_r_t28
#define PMR_F_ROW gl_r_t28
#define PMR_F_SMALL gl_r_pm_t18
#endif
#define PMR_F_PILL gl_r_t32
#define PMR_F_TITLE gl_r_title

// ---------- Places (the mockup's numbers) ----------
// The main screen: the frame (its line's width, how far in its middle line is, its corners' radius), the column in the
// middle (the icon, the time, the ticks, the name — their y), the row of buttons (y, the pill, the side buttons' radius,
// from the middle to them), the gear (its radius, where it is)
// Settings: the margin, the title's line, where the rows begin, a row's height, the Duration and Notifications columns'
// widths (side by side: PMS_TWO), a −/+ button's size and how far from the value, the switch
#if defined(HALO_SQUARE)  // 480 x 480
#define PMR_FW 9.0f
#define PMR_INS 9.5f
#define PMR_RAD 20.0f
#define PMR_ICON_Y 108.0f
#define PMR_TIME_Y 214.0f
#define PMR_TICK_Y 296.0f
#define PMR_TICK_H 14.0f
#define PMR_TICK_GAP 11.0f
#define PMR_NAME_Y 326.0f
#define PMR_BTN_Y 404.0f
#define PMR_PILL_W 156.0f
#define PMR_PILL_H 70.0f
#define PMR_BR 34.0f
#define PMR_GR 26.0f
#define PMS_PAD 16.0f
#define PMS_HY 32.0f
#define PMS_TOP 68.0f
#define PMS_RH 66.0f
#define PMS_TWO 0
#define PMS_BS 54.0f
#define PMS_SP 66.0f
#elif defined(HALO_WATCH)  // the 2.06 watch, 410 x 502: the square's sizes; the frame along the glass — its corners as
                          // round as the glass's (~64 px: the line's middle 9.5 px in), the gear on the corner's diagonal
#define PMR_FW 9.0f
#define PMR_INS 9.5f
#define PMR_RAD 54.5f
#define PMR_ICON_Y 120.0f
#define PMR_TIME_Y 226.0f
#define PMR_TICK_Y 308.0f
#define PMR_TICK_H 14.0f
#define PMR_TICK_GAP 11.0f
#define PMR_NAME_Y 338.0f
#define PMR_BTN_Y 416.0f
#define PMR_PILL_W 156.0f
#define PMR_PILL_H 70.0f
#define PMR_BR 34.0f
#define PMR_GR 26.0f
#define PMR_GEAR_X (HALO_W - PMR_INS - PMR_RAD + 10.0f)
#define PMR_GEAR_Y (PMR_INS + PMR_RAD - 10.0f)
#define PMS_PAD 24.0f
#define PMS_HY 50.0f
#define PMS_TOP 82.0f
#define PMS_RH 63.0f
#define PMS_TWO 0
#define PMS_BS 54.0f
#define PMS_SP 62.0f
#elif defined(HALO_COMPACT) && HALO_W > HALO_H  // the WT32 held wide, 480 x 320
#define PMR_FW 9.0f
#define PMR_INS 9.5f
#define PMR_RAD 20.0f
#define PMR_ICON_Y 70.0f
#define PMR_TIME_Y 140.0f
#define PMR_TICK_Y 200.0f
#define PMR_TICK_H 14.0f
#define PMR_TICK_GAP 11.0f
#define PMR_NAME_Y 226.0f
#define PMR_BTN_Y 272.0f
#define PMR_PILL_W 132.0f
#define PMR_PILL_H 56.0f
#define PMR_BR 30.0f
#define PMR_GR 24.0f
#define PMS_PAD 12.0f
#define PMS_HY 28.0f
#define PMS_TOP 70.0f
#define PMS_RH 62.0f
#define PMS_TWO 1
#define PMS_BS 50.0f
#define PMS_SP 58.0f
#elif defined(HALO_COMPACT)  // the WT32 held tall, 320 x 480
#define PMR_FW 9.0f
#define PMR_INS 9.5f
#define PMR_RAD 20.0f
#define PMR_ICON_Y 116.0f
#define PMR_TIME_Y 210.0f
#define PMR_TICK_Y 268.0f
#define PMR_TICK_H 14.0f
#define PMR_TICK_GAP 11.0f
#define PMR_NAME_Y 294.0f
#define PMR_BTN_Y 402.0f
#define PMR_PILL_W 120.0f
#define PMR_PILL_H 60.0f
#define PMR_BR 30.0f
#define PMR_GR 24.0f
#define PMS_PAD 12.0f
#define PMS_HY 28.0f
#define PMS_TOP 64.0f
#define PMS_RH 64.0f
#define PMS_TWO 0
#define PMS_BS 54.0f
#define PMS_SP 58.0f
#elif HALO_W > HALO_H  // the 7″ board, Wide: 1024 x 600
#define PMR_BIG 1
#define PMR_ICON_Y 110.0f
#define PMR_TIME_Y 268.0f
#define PMR_TICK_Y 388.0f
#define PMR_NAME_Y 430.0f
#define PMR_BTN_Y 515.0f
#define PMS_TWO 1
#else  // the 7″ board, Tall: 600 x 1024
#define PMR_BIG 1
#define PMR_ICON_Y 250.0f
#define PMR_TIME_Y 440.0f
#define PMR_TICK_Y 560.0f
#define PMR_NAME_Y 604.0f
#define PMR_BTN_Y 870.0f
#define PMS_TWO 0
#endif
#ifdef PMR_BIG  // (the 7″ board's either way)
#define PMR_FW 14.0f
#define PMR_INS 15.0f
#define PMR_RAD 36.0f
#define PMR_TICK_H 24.0f
#define PMR_TICK_GAP 20.0f
#define PMR_PILL_W 220.0f
#define PMR_PILL_H 92.0f
#define PMR_BR 40.0f
#define PMR_GR 30.0f
#define PMS_PAD 32.0f
#define PMS_HY 45.0f
#define PMS_TOP 105.0f
#define PMS_RH 96.0f
#define PMS_BS 64.0f
#define PMS_SP 92.0f
#define PMR_IN 14.0f    // the gear and the frame: the room between them
#define PMS_GAP 40.0f   // between the two columns
#define PMS_ROW_R 18.0f
#define PMS_TX 22.0f    // a row's words from its left end, its switch from its right end
#define PMS_STEP_R 18.0f
#define PMS_VAL_DY -8.0f
#define PMS_UNIT_DY 27.0f
#define PMS_SW_W 76.0f
#define PMS_SW_H 42.0f
#define PMR_SPACING 3   // START / PAUSE: letter-spaced
#else
#define PMR_IN 8.0f
#define PMS_GAP 12.0f
#define PMS_ROW_R 12.0f
#define PMS_TX 10.0f
#define PMS_STEP_R 6.0f
#define PMS_VAL_DY -7.0f
#define PMS_UNIT_DY 15.0f
#define PMS_SW_W 50.0f
#define PMS_SW_H 28.0f
#define PMR_SPACING 2
#endif
#define PMR_CX (HALO_W / 2.0f)
#define PMR_GAP (PMR_PILL_W / 2 + PMR_BR + (PMR_IN > 10 ? 26.0f : 14.0f))  // the pill's middle → a side button's
#ifndef PMR_GEAR_X  // (in the frame's corner, clear of it)
#define PMR_GEAR_X (HALO_W - PMR_INS - PMR_FW / 2 - PMR_GR - PMR_IN)
#define PMR_GEAR_Y (PMR_INS + PMR_FW / 2 + PMR_GR + PMR_IN)
#endif
#if PMS_TWO  // side by side: Duration takes 62 % of the width
#define PMS_COL_D ((HALO_W - 2 * PMS_PAD - PMS_GAP) * 0.62f)
#define PMS_COL_N (HALO_W - 2 * PMS_PAD - PMS_GAP - PMS_COL_D)
#else
#define PMS_COL_D (HALO_W - 2 * PMS_PAD)
#define PMS_COL_N PMS_COL_D
#endif
#define PMR_SEGS 40  // what is left, in pieces along the frame (each a shade brighter toward the head)

// A row's place: the four durations, then the switches (Sound, Auto-start; without a speaker — Auto-start only)
static void pms_row_place(int k, float &x, float &y) {
#if PMS_TWO
  x = PMS_PAD + (k < 4 ? 0 : PMS_COL_D + PMS_GAP), y = PMS_TOP + (k < 4 ? k : k - 4) * PMS_RH + PMS_RH / 2;
#else
  x = PMS_PAD, y = PMS_TOP + k * PMS_RH + PMS_RH / 2;
#endif
}
static float pms_step_x(float x) { return x + PMS_COL_D - PMS_STEP_R - PMS_BS / 2 - PMS_SP; }  // the value's middle (− left, + right)
static const char *const PMS_NAME[4] = {"Focus", "Short break", "Long break",
#ifdef HALO_COMPACT
#ifdef HALO_SQUARE
                                       "Long break after"};
#else
                                       "Long after"};  // (the WT32: the whole name would meet the −)
#endif
#else
                                       "Long break after"};
#endif
static const char *const PMS_UNIT[4] = {"min", "min", "min", "sess."};
#if PM_SPEAKER
#define PMS_SWITCHES 2
static const char *const PMS_SW_NAME[2] = {"Sound", "Auto-start"};
#else
#define PMS_SWITCHES 1
static const char *const PMS_SW_NAME[1] = {"Auto-start"};
#endif

// Where each button is on its page (x, y, the radius it takes a touch in; hw > 0 — a box hw either side of x instead)
static bool pm_btn_place(int b, float &x, float &y, float &r, float &hw) {
  r = PMR_BR + 8, hw = 0;
  switch (b) {
    case PB_RESET: x = PMR_CX - PMR_GAP, y = PMR_BTN_Y; return pm_page == PG_MAIN;
    case PB_SKIP: x = PMR_CX + PMR_GAP, y = PMR_BTN_Y; return pm_page == PG_MAIN;
    case PB_PILL: x = PMR_CX, y = PMR_BTN_Y, r = PMR_PILL_H / 2 + 8, hw = PMR_PILL_W / 2 + 8; return pm_page == PG_MAIN;
    case PB_GEAR: x = PMR_GEAR_X, y = PMR_GEAR_Y, r = PMR_GR + 12; return pm_page == PG_MAIN;
    case PB_CLOSE: x = HALO_W - PMS_PAD - PMR_GR, y = PMS_HY, r = PMR_GR + 12; return pm_page == PG_SET;
    case PB_RM0: case PB_RM1: case PB_RM2: case PB_RM3: case PB_RP0: case PB_RP1: case PB_RP2: case PB_RP3: {
      const bool plus = b >= PB_RP0;
      float rx;
      pms_row_place(b - (plus ? PB_RP0 : PB_RM0), rx, y);
      x = pms_step_x(rx) + (plus ? PMS_SP : -PMS_SP), r = hw = PMS_BS / 2 + 4;
      return pm_page == PG_SET;
    }
    case PB_SND: case PB_AUTO: {
      const int k = PMS_SWITCHES == 2 ? b - PB_SND : 0;
      if (PMS_SWITCHES == 1 && b == PB_SND) return false;
      float rx;
      pms_row_place(4 + k, rx, y);
      x = rx + PMS_COL_N / 2, hw = PMS_COL_N / 2, r = PMS_RH / 2 - 3;
      return pm_page == PG_SET;
    }
  }
  return false;
}

// ---------- Drawing ----------
static void pmr_frame_build();
static float pmr_mid_time, pmr_mid_mode, pmr_mid_ic, pmr_mid_val, pmr_mid_name, pmr_mid_pill, pmr_mid_title, pmr_mid_row, pmr_mid_small;
static void pm_fonts_build() {
  pmr_mid_time = halo_text_mid(PMR_F_TIME, "0"), pmr_mid_mode = halo_text_mid(PMR_F_MODE, PM_IC_MODE[0]);
  pmr_mid_ic = halo_text_mid(PMR_F_IC, PM_IC_GEAR), pmr_mid_val = halo_text_mid(PMR_F_VAL, "0");
  pmr_mid_name = halo_text_mid(PMR_F_NAME, "FOCUS"), pmr_mid_pill = halo_text_mid(PMR_F_PILL, "START");
  pmr_mid_title = halo_text_mid(PMR_F_TITLE, "Settings"), pmr_mid_row = halo_text_mid(PMR_F_ROW, "Focus");
  pmr_mid_small = halo_text_mid(PMR_F_SMALL, "min");
  pmr_frame_build();
}
static HaloItem &pmr_text(const HaloFont &f, const char *s, float mid, float x, float y, uint32_t c, float a = 1, int align = 1,
                         const HaloItem *on = &halo_black, float sc = 1, int spacing = 0) {
  HaloItem &it = pm_new();
  gl_text_mid(it, f, s, mid, x, y, sc, c, a, on);
  it.align = align, it.spacing = spacing;
  return it;
}
static void pmr_ring(float x, float y, float r, float w, uint32_t c, float a) {  // a thin ring: an outlined button
  HaloItem &it = pm_new();
  it.kind = HI_RING, it.x = x, it.y = y, it.r = r, it.w = w, it.v = 1, it.scale = 1;
  it.color = c, it.color2 = 0, it.alpha = a, it.on_black = true, it.on = nullptr;
}
static void pmr_icon_btn(int b, float x, float y, float r, const char *ic, float a, bool outlined) {
  const float sc = pm_sc(b);
  HaloItem &c = pm_circle(x, y, r * sc, outlined ? 0x000000 : PM_GREY, a);
  if (outlined) pmr_ring(x, y, r * sc - 1, PMR_FW > 10 ? 3 : 2, PM_GREY, a);
  pmr_text(PMR_F_IC, ic, pmr_mid_ic, x, y, PM_TEXT, a, 1, &c, sc);
}

// The frame: its middle line, from the top's middle clockwise — 9 pieces: lines and quarter circles
struct PmrSeg {
  float len, x0, y0, x1, y1;  // a line from (x0, y0) to (x1, y1); a corner: its middle (x0, y0), angle from x1
  bool arc;
};
static PmrSeg pmr_seg[9];
static float pmr_len;
static void pmr_frame_build() {
  const float x0 = PMR_INS, y0 = PMR_INS, x1 = HALO_W - PMR_INS, y1 = HALO_H - PMR_INS, R = PMR_RAD, q = HALO_PI * R / 2;
  const PmrSeg s[9] = {
      {x1 - R - PMR_CX, PMR_CX, y0, x1 - R, y0, false}, {q, x1 - R, y0 + R, 0, 0, true},
      {y1 - y0 - 2 * R, x1, y0 + R, x1, y1 - R, false}, {q, x1 - R, y1 - R, HALO_PI / 2, 0, true},
      {x1 - x0 - 2 * R, x1 - R, y1, x0 + R, y1, false}, {q, x0 + R, y1 - R, HALO_PI, 0, true},
      {y1 - y0 - 2 * R, x0, y1 - R, x0, y0 + R, false}, {q, x0 + R, y0 + R, HALO_PI * 1.5f, 0, true},
      {PMR_CX - x0 - R, x0 + R, y0, PMR_CX, y0, false}};
  pmr_len = 0;
  for (int i = 0; i < 9; i++) pmr_seg[i] = s[i], pmr_len += s[i].len;
}
static void pmr_point(float d, float &x, float &y) {  // the point d along the frame
  for (int i = 0; i < 9; i++) {
    const PmrSeg &g = pmr_seg[i];
    if (d <= g.len || i == 8) {
      const float t = g.len > 0 ? halo_clamp01(d / g.len) : 0;
      if (g.arc) {
        const float an = g.x1 + d / PMR_RAD;
        x = g.x0 + sinf(an) * PMR_RAD, y = g.y0 - cosf(an) * PMR_RAD;
      } else {
        x = g.x0 + (g.x1 - g.x0) * t, y = g.y0 + (g.y1 - g.y0) * t;
      }
      return;
    }
    d -= g.len;
  }
}
// A part of the frame from d0 to d1 (within one piece of it), in one colour
static void pmr_frame_part(int i, float u0, float u1, uint32_t c, float a) {
  const PmrSeg &g = pmr_seg[i];
  HaloItem &it = pm_new();
  it.scale = 1, it.color = c, it.alpha = a, it.on_black = true, it.on = nullptr;
  if (g.arc) {
    // (each part reaches 1 px into the next: their soft edges side by side left a dark seam)
    it.kind = HI_ARC, it.x = g.x0, it.y = g.y0, it.r = PMR_RAD, it.w = PMR_FW, it.a0 = g.x1 + u0 / PMR_RAD;
    it.a1 = g.x1 + fminf(g.len, u1 + 1) / PMR_RAD;
  } else {
    const float t0 = u0 / g.len, t1 = fminf(g.len, u1 + 1) / g.len;
    const float ax = g.x0 + (g.x1 - g.x0) * t0, ay = g.y0 + (g.y1 - g.y0) * t0;
    const float bx = g.x0 + (g.x1 - g.x0) * t1, by = g.y0 + (g.y1 - g.y0) * t1;
    it.kind = HI_RRECT, it.x = (ax + bx) / 2, it.y = (ay + by) / 2, it.r = 0;
    it.w = g.y0 == g.y1 ? fabsf(bx - ax) : PMR_FW, it.h = g.y0 == g.y1 ? PMR_FW : fabsf(by - ay);
  }
}

static void pm_layout_main(float a) {
  const uint32_t col = pm_mode_col(PM_COL), head = pm_mode_col(PM_HEAD);
  const float frac = (float)pm_left() / fmaxf(1.0f, (float)pm_dur_ms(pm_mode));
  const bool fresh = !pm_running && pm_left_ms >= pm_dur_ms(pm_mode);  // not started: the track only (as on the round)
  const float run = halo_clamp01(pm_runm.x), bright = 0.55f + 0.45f * run;  // paused — dimmer
  // The interval just ran out: the track flashes three times in the new mode's colour (0.6 s each)
  const float since = (halo_ms() - pm_flash_ms) / 1000.0f;
  const float flash = pm_flash_ms && since < 1.8f ? 0.5f - 0.5f * cosf(since * (2 * HALO_PI / 0.6f)) : 0;
  const uint32_t track = pm_mix(PM_TRACK, PM_COL[pm_mode], 0.85f * flash);
  for (int i = 0; i < 9; i++) pmr_frame_part(i, 0, pmr_seg[i].len, track, a);
  // What is left: from the top's middle clockwise, in pieces (split where the frame turns)
  const float end = fresh ? 0 : pmr_len * halo_clamp01(frac), piece = pmr_len / PMR_SEGS;
  for (int p = 0; p < PMR_SEGS; p++) {
    const float d0 = p * piece, d1 = fminf(end, (p + 1) * piece);
    if (d1 <= d0 + 0.05f) break;
    const float t = (d0 + d1) / 2 / end;
    const uint32_t c = pm_mix(pm_mix(PM_TRACK, col, 0.35f + 0.65f * t), head, t * t * 0.6f);
    float s0 = 0;
    for (int i = 0; i < 9; i++) {
      const float s1 = s0 + pmr_seg[i].len, u0 = fmaxf(d0, s0), u1 = fminf(d1, s1);
      if (u1 > u0 + 0.05f) pmr_frame_part(i, u0 - s0, u1 - s0, c, a * bright);
      s0 = s1;
    }
  }
  float hx, hy;
  pmr_point(end, hx, hy);
  const float ha = end > 0.5f ? a * bright : 0;
  pm_circle(hx, hy, PMR_FW * 1.25f, head, ha * 0.25f * run);  // the glow (running)
  pm_circle(hx, hy, PMR_FW * 0.62f, pm_mix(head, PM_TEXT, 0.35f), ha);
  // The middle: the mode's icon, the time, the ticks, the mode's name
  for (int m = 0; m < 3; m++) {
    const float k = halo_clamp01(pm_modem[m].x);
    pmr_text(PMR_F_MODE, PM_IC_MODE[m], pmr_mid_mode, PMR_CX, PMR_ICON_Y, pm_mix(PM_TEXT, PM_HEAD[m], 0.25f), a * k);
  }
  pmr_text(PMR_F_TIME, pm_time_s, pmr_mid_time, PMR_CX, PMR_TIME_Y, PM_TEXT, a);
  const int ticks = pm_after, shown = pm_mode == PM_LONG ? ticks : pm_done;
  for (int i = 0; i < ticks; i++) {
    const float x = PMR_CX + (i - (ticks - 1) / 2.0f) * PMR_TICK_GAP, w = PMR_TICK_H * 0.31f;
    pm_rrect(x, PMR_TICK_Y, w, PMR_TICK_H, w / 2, i < shown ? col : PM_OFF, a);
  }
  pmr_text(PMR_F_NAME, PM_NAME[pm_mode], pmr_mid_name, PMR_CX, PMR_NAME_Y, PM_DIM, a, 1, &halo_black, 1, 2);
  // The row: Reset, START / PAUSE, Skip; the gear in the corner
  const float ps = pm_sc(PB_PILL);
  HaloItem &pill = pm_rrect(PMR_CX, PMR_BTN_Y, PMR_PILL_W * ps, PMR_PILL_H * ps, PMR_PILL_H / 2 * ps, PM_GREY, a);
  pmr_text(PMR_F_PILL, pm_running ? "PAUSE" : "START", pmr_mid_pill, PMR_CX, PMR_BTN_Y, PM_TEXT, a, 1, &pill, ps, PMR_SPACING);
  pmr_icon_btn(PB_RESET, PMR_CX - PMR_GAP, PMR_BTN_Y, PMR_BR, PM_IC_RESET, a, true);
  pmr_icon_btn(PB_SKIP, PMR_CX + PMR_GAP, PMR_BTN_Y, PMR_BR, PM_IC_SKIP, a, false);
  pmr_icon_btn(PB_GEAR, PMR_GEAR_X, PMR_GEAR_Y, PMR_GR, PM_IC_GEAR, a, true);
}

static void pm_layout_settings(float a) {
  pmr_text(PMR_F_TITLE, "Settings", pmr_mid_title, PMS_PAD, PMS_HY, PM_TEXT, a, 0);
  pmr_icon_btn(PB_CLOSE, HALO_W - PMS_PAD - PMR_GR, PMS_HY, PMR_GR, PM_IC_X, a, false);
#if PMS_TWO
  pmr_text(PMR_F_SMALL, "DURATION", pmr_mid_small, PMS_PAD + 4, PMS_TOP - PMS_ROW_R / 2 - 6, PM_DIM, a, 0, &halo_black, 1, 2);
  pmr_text(PMR_F_SMALL, "NOTIFICATIONS", pmr_mid_small, PMS_PAD + PMS_COL_D + PMS_GAP + 4, PMS_TOP - PMS_ROW_R / 2 - 6, PM_DIM, a, 0,
          &halo_black, 1, 2);
#endif
  for (int i = 0; i < 4; i++) {  // the durations: the name, − value +
    float x, y;
    pms_row_place(i, x, y);
    HaloItem &bg = pm_rrect(x + PMS_COL_D / 2, y, PMS_COL_D, PMS_RH - 6, PMS_ROW_R, PM_SURF, a);
    pmr_text(PMR_F_ROW, PMS_NAME[i], pmr_mid_row, x + PMS_TX, y, PM_TEXT, a, 0, &bg);
    const float sx = pms_step_x(x);
    const int v = *pm_row_val(i);
    for (int k = 0; k < 2; k++) {
      const int b = k ? PB_RP0 + i : PB_RM0 + i;
      const float bx = sx + (k ? PMS_SP : -PMS_SP), sc = pm_sc(b);
      const bool can = k ? v < PM_ROWS[i].hi : v > PM_ROWS[i].lo;
      HaloItem &bb = pm_rrect(bx, y, PMS_BS * sc, PMS_BS * sc, PMS_BS * 0.3f * sc, PM_GREY, a);
      pmr_text(PMR_F_IC, k ? PM_IC_PLUS : PM_IC_MINUS, pmr_mid_ic, bx, y, can ? PM_TEXT : PM_OFF, a, 1, &bb, sc);
    }
    pmr_text(PMR_F_VAL, pm_val_s[i], pmr_mid_val, sx, y + PMS_VAL_DY, PM_TEXT, a, 1, &bg);
    pmr_text(PMR_F_SMALL, PMS_UNIT[i], pmr_mid_small, sx, y + PMS_UNIT_DY, PM_DIM, a, 1, &bg);
  }
  for (int k = 0; k < PMS_SWITCHES; k++) {  // the switches: the track goes the Focus red when on, the knob slides
    float x, y;
    pms_row_place(4 + k, x, y);
    const int b = PMS_SWITCHES == 2 ? PB_SND + k : PB_AUTO, s = b == PB_SND ? 0 : 1;
    const float on = halo_clamp01(pm_sw[s].x), ra = a * (1 - 0.35f * halo_clamp01(pm_press[b].s.x));
    HaloItem &bg = pm_rrect(x + PMS_COL_N / 2, y, PMS_COL_N, PMS_RH - 6, PMS_ROW_R, PM_SURF, a);
    pmr_text(PMR_F_ROW, PMS_SW_NAME[k], pmr_mid_row, x + PMS_TX, y, PM_TEXT, ra, 0, &bg);
    const float tx = x + PMS_COL_N - PMS_TX - PMS_SW_W / 2;
    pm_rrect(tx, y, PMS_SW_W, PMS_SW_H, PMS_SW_H / 2, pm_mix(PM_OFF, PM_COL[0], on), a);
    pm_circle(tx - PMS_SW_W / 2 + PMS_SW_H / 2 + (PMS_SW_W - PMS_SW_H) * on, y, PMS_SW_H / 2 - 4, PM_TEXT, a);
  }
}
