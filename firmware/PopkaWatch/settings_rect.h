// settings_rect.h — Settings on a rectangular screen (variant L3 of web/mockups/rect-apps.html), as on a tablet: the sections
// at the left, the setting at the right, everything on one screen. This board has one real setting, the backlight: the whole
// bar is a slider. The second row opens About when the project has it (the layer's link; none — no row).
// Included from settings.h in place of its round layout.

#if defined(HALO_WATCH)
// A rectangular watch, 410 × 502 (direction B of web/mockups/watch206-look.html): "Display" at the top left, under it
// "Brightness" and its percent, then a pill the whole width — "−" and "+" in circles at its ends, the bar between them
// (a tap on "−"/"+" — a step, held — a step every 60 ms after half a second; a drag or a tap along the bar — that
// step); under it a pill to About when the project links one, and the hint what BOOT does
#define DW_X 26.0f            // the words' left (and right) margin
#define DW_TITLE_Y 84.0f
#define DW_LABEL_Y 140.0f
#define DW_PILL_X 14.0f
#define DW_PILL_W (HALO_W - 28.0f)
#define DW_PILL_Y 220.0f      // its middle
#define DW_PILL_H 104.0f
#define DW_BTN 96.0f          // "−" and "+": a zone at each end
#define DW_BTN_R 36.0f
#define DW_BAR_X0 (DW_PILL_X + 98.0f)
#define DW_BAR_W (DW_PILL_W - 196.0f)
#define DW_BAR_H 16.0f
#define DW_KNOB_R 14.0f
#define DW_ROW_Y 296.0f       // About's pill: its top
#define DW_ROW_H 94.0f
#define DW_HINT_Y 420.0f
#define DW_HINT_Y0 318.0f     // (no About: right under the slider)
#define DW_BTN_BG 0x3A3E44
#define DW_TRACK 0x4A4F57
#define DW_FILL 0xA8C7FA
#define DW_PILL 0x26292D
#define DW_LABEL 0xC9CACF
#define DR_ICON_CHIP "\xEF\x8B\x9B"
extern const HaloFont gl_w_t28, gl_w_ic30;

static HaloItem dr_title, dr_label, dr_pct, dw_pill, dw_btn[2], dw_sign[3], dw_track, dw_fill, dw_knob, dw_row, dw_row_c,
    dw_row_ic, dw_row_t, dw_row_s, dr_hint;
static Press dr_press;
static char dr_pct_s[8];
static float dr_mid_title, dr_mid_label, dr_mid_hint, dw_mid_ic;

static const char *dp_hint_text() {  // what BOOT does (the editor's Buttons): 1 — screen off, 2 — lock, 3 — Always On
  switch (pw_boot_fn()) {
    case 1: return "BOOT turns the screen off";
    case 2: return "BOOT locks the screen";
    case 3: return "BOOT turns on Always On";
  }
  return "";
}

static void display_build() {
  dr_mid_title = halo_text_mid(gl_title, "Display"), dr_mid_label = halo_text_mid(gl_t24, "Brightness 0123456789%");
  dr_mid_hint = halo_text_mid(gl_t20, "BOOT turns the screen off"), dw_mid_ic = halo_text_mid(gl_w_ic30, DR_ICON_CHIP);
  halo_ease_make(EASE_SHARP, 0, 0, 0, 1);
}

static bool dr_has_about() {
  return dp_link && dp_link->tap > 0;
}

static void display_update(float dt) {
  HaloFinger &f = halo_finger;
  const float x0 = DW_PILL_X, x1 = DW_PILL_X + DW_PILL_W;
  const bool on_pill = fabsf(f.y0 - DW_PILL_Y) < DW_PILL_H / 2 + 8 && f.x0 > x0 - 8 && f.x0 < x1 + 8;
  const bool on_about = dr_has_about() && f.x0 > DW_PILL_X && f.x0 < x1 && f.y0 > DW_ROW_Y && f.y0 < DW_ROW_Y + DW_ROW_H;
  if (f.pressed) {
    dp_armed = f.owner ? 0 : on_pill ? (f.x0 < x0 + DW_BTN ? -1 : f.x0 > x1 - DW_BTN ? 1 : 2) : on_about ? 3 : 0;
    dp_hold_at = halo_ms() + 500;
  }
  if (dp_armed == 2 && f.down && (!f.dragging || f.horiz) && halo_finger_claim(&dp_armed))  // the bar: by position
    dp_set((int)lroundf(halo_clamp01((f.x - DW_BAR_X0) / DW_BAR_W) * DP_STEPS));
  if ((dp_armed == -1 || dp_armed == 1) && f.dragging) dp_armed = 0;  // moved — a swipe, not a press
  if ((dp_armed == -1 || dp_armed == 1) && f.tap) dp_set(dp_value + dp_armed);
  if ((dp_armed == -1 || dp_armed == 1) && f.down && (int32_t)(halo_ms() - dp_hold_at) >= 0)  // held: a repeat
    dp_set(dp_value + dp_armed), dp_hold_at = halo_ms() + 60;
  if (dp_armed == 3 && f.dragging) dp_armed = 0;
  if (dp_armed == 3 && f.tap) gl_req = dp_link, gl_req_depth = halo_nav_depth();
  press_update(dp_press[0], dp_armed == -1 && f.down, dt);
  press_update(dp_press[1], dp_armed == 1 && f.down, dt);
  press_update(dr_press, dp_armed == 3 && f.down, dt);
  if (!f.down) dp_armed = 0;
}

static void dr_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, const HaloView &v, const HaloItem *on, float a = 1) {
  rg_text(it, f, s, mid, align, x, y, c, a, on);
  it.on_black = on == &halo_black;
  halo_view_commit(it, v);
}

// A rounded bar or (w = h = 0) a circle of radius r
static void dw_shape(HaloItem &it, int kind, float x, float y, float w, float h, float r, uint32_t c, bool on_black, const HaloView &v,
                     float a = 1) {
  it.kind = kind, it.x = x, it.y = y, it.w = w, it.h = h, it.r = r, it.scale = 1, it.color = c, it.alpha = a;
  it.on_black = on_black, it.on = nullptr;
  halo_view_commit(it, v);
}

static void display_layout(const HaloView &v) {
  dr_text(dr_title, gl_title, "Display", dr_mid_title, 0, DW_X, DW_TITLE_Y, GL_TEXT, v, &halo_black);
  dr_text(dr_label, gl_t24, "Brightness", dr_mid_label, 0, DW_X, DW_LABEL_Y, DW_LABEL, v, &halo_black);
  snprintf(dr_pct_s, sizeof dr_pct_s, "%d%%", (int)lroundf(halo_clamp01(dp_shown / DP_STEPS) * 100));
  dr_text(dr_pct, gl_t24, dr_pct_s, dr_mid_label, 2, W - DW_X, DW_LABEL_Y, DW_LABEL, v, &halo_black);
  dw_shape(dw_pill, HI_RRECT, DW_PILL_X + DW_PILL_W / 2, DW_PILL_Y, DW_PILL_W, DW_PILL_H, DW_PILL_H / 2, DW_PILL, true, v);
  for (int i = 0; i < 2; i++) {  // "−" and "+" in their circles; pressed — the circle shrinks a little and lightens
    const float pr = halo_clamp01(dp_press[i].s.x), cx = i ? DW_PILL_X + DW_PILL_W - DW_BTN / 2 : DW_PILL_X + DW_BTN / 2;
    dw_shape(dw_btn[i], HI_CIRCLE, cx, DW_PILL_Y, 0, 0, DW_BTN_R * (1 - 0.08f * pr), pr > 0.01f ? DP_TRACK : DW_BTN_BG, false, v);
  }
  const float mx = DW_PILL_X + DW_BTN / 2, px = DW_PILL_X + DW_PILL_W - DW_BTN / 2;
  dw_shape(dw_sign[0], HI_RRECT, mx, DW_PILL_Y, 22, 4, 2, GL_TEXT, false, v);
  dw_shape(dw_sign[1], HI_RRECT, px, DW_PILL_Y, 22, 4, 2, GL_TEXT, false, v);
  dw_shape(dw_sign[2], HI_RRECT, px, DW_PILL_Y, 4, 22, 2, GL_TEXT, false, v);
  const float k = halo_clamp01(dp_shown / DP_STEPS), fw = fmaxf(DW_BAR_H, DW_BAR_W * k), kx = DW_BAR_X0 + DW_BAR_W * k;
  dw_shape(dw_track, HI_RRECT, DW_BAR_X0 + DW_BAR_W / 2, DW_PILL_Y, DW_BAR_W, DW_BAR_H, DW_BAR_H / 2, DW_TRACK, false, v);
  dw_shape(dw_fill, HI_RRECT, DW_BAR_X0 + fw / 2, DW_PILL_Y, fw, DW_BAR_H, DW_BAR_H / 2, DW_FILL, false, v);
  dw_shape(dw_knob, HI_CIRCLE, fmaxf(DW_BAR_X0 + DW_KNOB_R / 2, kx), DW_PILL_Y, 0, 0, DW_KNOB_R, DW_FILL, false, v);
  const float ra = dr_has_about() ? 1 : 0, p = halo_clamp01(dr_press.s.x), ry = DW_ROW_Y + DW_ROW_H / 2;
  dw_shape(dw_row, HI_RRECT, DW_PILL_X + DW_PILL_W / 2, ry, DW_PILL_W, DW_ROW_H, DW_ROW_H / 2 - 16 * p, DW_PILL, true, v, ra);
  dw_shape(dw_row_c, HI_CIRCLE, DW_PILL_X + 48, ry, 0, 0, 30, 0xC9CDD2, false, v, ra);
  gl_text_mid(dw_row_ic, gl_w_ic30, DR_ICON_CHIP, dw_mid_ic, DW_PILL_X + 48, ry, 1, 0x1B1B1F, ra, &dw_row_c);
  halo_view_commit(dw_row_ic, v);
  gl_text_base(dw_row_t, gl_w_t28, "About", DW_PILL_X + 94, DW_ROW_Y + 44, 1, GL_TEXT, ra, &dw_row);
  gl_text_base(dw_row_s, gl_t20, "this board, live", DW_PILL_X + 94, DW_ROW_Y + 72, 1, GL_DIM, ra, &dw_row);
  halo_view_commit(dw_row_t, v), halo_view_commit(dw_row_s, v);
  const char *hint = dp_hint_text();
  dr_text(dr_hint, gl_t20, hint, dr_mid_hint, 1, CX, dr_has_about() ? DW_HINT_Y : DW_HINT_Y0, GL_DIM, v, &halo_black, *hint ? 1 : 0);
}

static void display_draw() {
  halo_item_draw(dr_title), halo_item_draw(dr_label), halo_item_draw(dr_pct);
  halo_item_draw(dw_pill);
  for (auto &b : dw_btn) halo_item_draw(b);
  for (auto &d : dw_sign) halo_item_draw(d);
  halo_item_draw(dw_track), halo_item_draw(dw_fill), halo_item_draw(dw_knob);
  halo_item_draw(dw_row), halo_item_draw(dw_row_c), halo_item_draw(dw_row_ic), halo_item_draw(dw_row_t), halo_item_draw(dw_row_s);
  halo_item_draw(dr_hint);
}
#elif defined(HALO_SQUARE)
// The square, 480 × 480 (variant 5C of web/mockups/square-screens.html): the two rows side by side at the top, the percent
// and "Brightness" in the middle, then the round board's control — big "−" and "+" at the ends of a pill and the eight
// steps between them as segments (settings.h: DP_STEPS 8 here). A tap on "−"/"+" — a step (held — a step every 60 ms
// after half a second), a drag or a tap along the segments — that step; the hint what BOOT does under it.
#define DQ_ROW_X 16.0f
#define DQ_ROW_W 218.0f
#define DQ_ROW1_X 246.0f
#define DQ_ROW_Y 52.0f
#define DQ_ROW_H 64.0f
#define DQ_ROW_R 18.0f
#define DQ_PCT_Y 176.0f
#define DQ_LABEL_Y 216.0f
#define DQ_PILL_X 16.0f
#define DQ_PILL_W 448.0f
#define DQ_PILL_Y 298.0f      // its middle
#define DQ_PILL_H 96.0f
#define DQ_BTN 96.0f          // "−" and "+": a zone at each end
#define DQ_SEG_X 124.0f       // the first segment's middle
#define DQ_SEG_STEP 32.5f
#define DQ_SEG_W 24.0f
#define DQ_SEG_H 20.0f
#define DQ_HINT_Y 386.0f
#define DR_FILL 0x8AD6F0
#define DR_CARD 0x2D3236
#define DQ_OFF 0x4A5056       // a step not reached
#define DR_ICON_SUN "\xEF\x86\x85"
#define DR_ICON_CHIP "\xEF\x8B\x9B"
extern const HaloFont gl_c_t14;

static HaloItem dr_title, dr_row[2], dr_row_ic[2], dr_row_t[2], dr_label, dr_pct, dr_hint, dq_pill, dq_circle[2], dq_sign[3];
static HaloItem dq_seg[DP_STEPS];
static Press dr_press;
static char dr_pct_s[8];
static float dr_mid_title, dr_mid_row, dr_mid_label, dr_mid_pct, dr_mid_hint, dr_mid_ic;

static const char *dp_hint_text() {  // what BOOT does (the editor's Buttons): 1 — screen off, 2 — lock, 3 — Always On
  switch (pw_boot_fn()) {
    case 1: return "BOOT turns the screen off";
    case 2: return "BOOT locks the screen";
    case 3: return "BOOT turns on Always On";
  }
  return "";
}

static void display_build() {
  dr_mid_title = halo_text_mid(gl_r_title, "Settings"), dr_mid_row = halo_text_mid(gl_r_t32, "Display");
  dr_mid_label = halo_text_mid(gl_c_t14, "Brightness"), dr_mid_pct = halo_text_mid(gl_r_n72, "0123456789");
  dr_mid_hint = halo_text_mid(gl_c_t14, "BOOT turns the screen off"), dr_mid_ic = halo_text_mid(gl_r_ic36, DR_ICON_SUN);
  halo_ease_make(EASE_SHARP, 0, 0, 0, 1);
}

static bool dr_has_about() {
  return dp_link && dp_link->tap > 0;
}

static void display_update(float dt) {
  HaloFinger &f = halo_finger;
  const float x0 = DQ_PILL_X, x1 = DQ_PILL_X + DQ_PILL_W, bx = x0 + DQ_BTN, bw = DQ_PILL_W - 2 * DQ_BTN;
  const bool on_pill = fabsf(f.y0 - DQ_PILL_Y) < DQ_PILL_H / 2 + 8 && f.x0 > x0 - 8 && f.x0 < x1 + 8;
  const bool on_about = dr_has_about() && f.x0 > DQ_ROW1_X && f.x0 < DQ_ROW1_X + DQ_ROW_W && f.y0 > DQ_ROW_Y && f.y0 < DQ_ROW_Y + DQ_ROW_H;
  if (f.pressed) {
    dp_armed = f.owner ? 0 : on_pill ? (f.x0 < bx ? -1 : f.x0 > bx + bw ? 1 : 2) : on_about ? 3 : 0;
    dp_hold_at = halo_ms() + 500;
  }
  if (dp_armed == 2 && f.down && (!f.dragging || f.horiz) && halo_finger_claim(&dp_armed))  // the segments: by position
    dp_set(constrain((int)floorf((f.x - (DQ_SEG_X - DQ_SEG_STEP / 2)) / DQ_SEG_STEP) + 1, 0, DP_STEPS));
  if ((dp_armed == -1 || dp_armed == 1) && f.dragging) dp_armed = 0;  // moved — a swipe, not a press
  if ((dp_armed == -1 || dp_armed == 1) && f.tap) dp_set(dp_value + dp_armed);
  if ((dp_armed == -1 || dp_armed == 1) && f.down && (int32_t)(halo_ms() - dp_hold_at) >= 0)  // held: a repeat
    dp_set(dp_value + dp_armed), dp_hold_at = halo_ms() + 60;
  if (dp_armed == 3 && f.dragging) dp_armed = 0;
  if (dp_armed == 3 && f.tap) gl_req = dp_link, gl_req_depth = halo_nav_depth();
  press_update(dp_press[0], dp_armed == -1 && f.down, dt);
  press_update(dp_press[1], dp_armed == 1 && f.down, dt);
  press_update(dr_press, dp_armed == 3 && f.down, dt);
  if (!f.down) dp_armed = 0;
}

static void dr_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, const HaloView &v, const HaloItem *on, float a = 1) {
  rg_text(it, f, s, mid, align, x, y, c, a, on);
  it.on_black = on == &halo_black;
  halo_view_commit(it, v);
}

static void dq_bar(HaloItem &it, float x, float y, float w, float h, const HaloView &v) {  // a stroke of "−" / "+"
  it.kind = HI_RRECT, it.x = x, it.y = y, it.w = w, it.h = h, it.r = fminf(w, h) / 2, it.scale = 1, it.color = GL_TEXT;
  it.alpha = 1, it.on_black = false, it.on = nullptr;
  halo_view_commit(it, v);
}

static void display_layout(const HaloView &v) {
  dr_text(dr_title, gl_r_title, "Settings", dr_mid_title, 0, RG_PAD, RG_TITLE_Y, GL_TEXT, v, &halo_black);
  const float p = halo_clamp01(dr_press.s.x);
  for (int i = 0; i < 2; i++) {
    const float a = i && !dr_has_about() ? 0 : 1, cy = DQ_ROW_Y + DQ_ROW_H / 2, rx = i ? DQ_ROW1_X : DQ_ROW_X;
    HaloItem &b = dr_row[i];
    b.kind = HI_RRECT, b.x = rx + DQ_ROW_W / 2, b.y = cy, b.w = DQ_ROW_W, b.h = DQ_ROW_H, b.r = DQ_ROW_R * (i ? 1 - 0.3f * p : 1), b.scale = 1;
    b.color = i ? DR_CARD : 0x254552, b.alpha = a, b.on_black = true, b.on = nullptr;
    halo_view_commit(b, v);
    gl_text_mid(dr_row_ic[i], gl_r_ic36, i ? DR_ICON_CHIP : DR_ICON_SUN, dr_mid_ic, rx + 34, cy, 1, i ? 0xC9CDD2 : DR_FILL, a, &b);
    halo_view_commit(dr_row_ic[i], v);
    dr_text(dr_row_t[i], gl_r_t32, i ? "About" : "Display", dr_mid_row, 0, rx + 62, cy, GL_TEXT, v, &b, a);
  }
  snprintf(dr_pct_s, sizeof dr_pct_s, "%d%%", (int)lroundf(halo_clamp01(dp_shown / DP_STEPS) * 100));
  dr_text(dr_pct, gl_r_n72, dr_pct_s, dr_mid_pct, 1, CX, DQ_PCT_Y, GL_TEXT, v, &halo_black);
  dr_text(dr_label, gl_c_t14, "Brightness", dr_mid_label, 1, CX, DQ_LABEL_Y, GL_DIM, v, &halo_black);
  dq_pill.kind = HI_RRECT, dq_pill.x = DQ_PILL_X + DQ_PILL_W / 2, dq_pill.y = DQ_PILL_Y, dq_pill.w = DQ_PILL_W, dq_pill.h = DQ_PILL_H;
  dq_pill.r = DQ_PILL_H / 2, dq_pill.scale = 1, dq_pill.color = DR_CARD, dq_pill.alpha = 1, dq_pill.on_black = true, dq_pill.on = nullptr;
  halo_view_commit(dq_pill, v);
  for (int i = 0; i < 2; i++) {  // a pressed "−" / "+": a circle grows under it
    const float pr = halo_clamp01(dp_press[i].s.x), cx = i ? DQ_PILL_X + DQ_PILL_W - DQ_BTN / 2 : DQ_PILL_X + DQ_BTN / 2;
    HaloItem &c = dq_circle[i];
    c.kind = HI_CIRCLE, c.x = cx, c.y = DQ_PILL_Y, c.r = 36 * pr, c.scale = 1, c.color = DQ_OFF, c.on_black = false, c.on = nullptr;
    c.alpha = pr > 0.01f ? 1 : 0;
    halo_view_commit(c, v);
  }
  const float mx = DQ_PILL_X + DQ_BTN / 2, px = DQ_PILL_X + DQ_PILL_W - DQ_BTN / 2;
  dq_bar(dq_sign[0], mx, DQ_PILL_Y, 18, 4, v), dq_bar(dq_sign[1], px, DQ_PILL_Y, 18, 4, v), dq_bar(dq_sign[2], px, DQ_PILL_Y, 4, 18, v);
  for (int i = 0; i < DP_STEPS; i++) {  // a segment a step: lit up to the brightness (the last one partly while it glides)
    HaloItem &d = dq_seg[i];
    const float on = halo_clamp01(dp_shown - i);
    d.kind = HI_RRECT, d.x = DQ_SEG_X + i * DQ_SEG_STEP, d.y = DQ_PILL_Y, d.w = DQ_SEG_W, d.h = DQ_SEG_H, d.r = DQ_SEG_H / 2;
    d.scale = 1, d.alpha = 1, d.on_black = false, d.on = nullptr;
    d.color = on > 0.5f ? DR_FILL : DQ_OFF;
    halo_view_commit(d, v);
  }
  const char *hint = dp_hint_text();
  dr_text(dr_hint, gl_c_t14, hint, dr_mid_hint, 1, CX, DQ_HINT_Y, GL_DIM, v, &halo_black, *hint ? 1 : 0);
}

static void display_draw() {
  halo_item_draw(dr_title);
  for (int i = 0; i < 2; i++) halo_item_draw(dr_row[i]), halo_item_draw(dr_row_ic[i]), halo_item_draw(dr_row_t[i]);
  halo_item_draw(dr_pct), halo_item_draw(dr_label);
  halo_item_draw(dq_pill);
  for (auto &c : dq_circle) halo_item_draw(c);
  for (auto &d : dq_sign) halo_item_draw(d);
  for (auto &d : dq_seg) halo_item_draw(d);
  halo_item_draw(dr_hint);
}
#else  // (the other rectangles)

#ifdef HALO_COMPACT  // a small rectangle: wide — the rows at the left, the setting at the right; tall — the rows side by
                     // side at the top, the setting under them
#define DR_WIDE (HALO_W >= HALO_H)
#define DR_ROW_X 12.0f
#define DR_ROW_W (DR_WIDE ? 164.0f : (HALO_W - 34) / 2.0f)
#define DR_ROW1_X (DR_WIDE ? DR_ROW_X : DR_ROW_X + DR_ROW_W + 10)
#define DR_ROW_H 60.0f
#define DR_ROW0_Y 58.0f
#define DR_ROW1_Y (DR_WIDE ? 126.0f : DR_ROW0_Y)
#define DR_X (DR_WIDE ? 190.0f : 12.0f)
#define DR_BAR_W (HALO_W - DR_X - 12)
#define DR_BAR_H 70.0f
#define DR_LABEL_Y (DR_WIDE ? 88.0f : 166.0f)
#define DR_BAR_Y (DR_LABEL_Y + 78)
#define DR_HINT_Y (DR_BAR_Y + 66)
#elif defined(HALO_P4_TALL)  // the 7″ board held tall (rect-tall.html 5A): the two rows side by side at the top, Brightness,
                             // its percent and the bar the whole width under them
#define DR_ROW_X 32.0f
#define DR_ROW_W 262.0f
#define DR_ROW1_X 306.0f
#define DR_ROW_H 100.0f
#define DR_ROW0_Y 96.0f
#define DR_ROW1_Y DR_ROW0_Y
#define DR_X 32.0f
#define DR_BAR_W 536.0f
#define DR_BAR_H 110.0f
#define DR_LABEL_Y 300.0f
#define DR_BAR_Y 435.0f
#define DR_HINT_Y 548.0f
#else
#define DR_ROW_X 24.0f
#define DR_ROW_W 330.0f
#define DR_ROW_H 100.0f
#define DR_ROW0_Y 96.0f
#define DR_ROW1_Y 208.0f
#define DR_X 400.0f
#define DR_BAR_W 572.0f
#define DR_BAR_H 110.0f
#define DR_BAR_Y 275.0f
#define DR_LABEL_Y 130.0f
#define DR_HINT_Y 386.0f
#define DR_ROW1_X DR_ROW_X    // the second row: under the first
#endif
#define DR_FILL 0x8AD6F0
#define DR_ON_FILL 0x14262E
#define DR_CARD 0x2D3236
#define DR_ICON_SUN "\xEF\x86\x85"
#define DR_ICON_CHIP "\xEF\x8B\x9B"

static HaloItem dr_title, dr_row[2], dr_row_ic[2], dr_row_t[2], dr_label, dr_pct, dr_bar, dr_fill, dr_sun, dr_hint;
static Press dr_press;
static char dr_pct_s[8];
static float dr_mid_title, dr_mid_row, dr_mid_label, dr_mid_pct, dr_mid_hint, dr_mid_ic;

static const char *dp_hint_text() {  // what BOOT does (the editor's Buttons): 1 — screen off, 2 — lock, 3 — Always On
  switch (pw_boot_fn()) {
    case 1: return "BOOT turns the screen off";
    case 2: return "BOOT locks the screen";
    case 3: return "BOOT turns on Always On";
  }
  return "";
}

static void display_build() {
  dr_mid_title = halo_text_mid(gl_r_title, "Settings"), dr_mid_row = halo_text_mid(gl_r_t32, "Display");
  dr_mid_label = halo_text_mid(gl_r_t44, "Brightness"), dr_mid_pct = halo_text_mid(gl_r_n72, "0123456789");
  dr_mid_hint = halo_text_mid(gl_t24, "BOOT turns the screen off"), dr_mid_ic = halo_text_mid(gl_r_ic36, DR_ICON_SUN);
  halo_ease_make(EASE_SHARP, 0, 0, 0, 1);
}

static bool dr_has_about() {
  return dp_link && dp_link->tap > 0;
}

static void display_update(float dt) {
  HaloFinger &f = halo_finger;
  const bool on_bar = fabsf(f.y0 - DR_BAR_Y) < DR_BAR_H / 2 + 10 && f.x0 > DR_X - 10 && f.x0 < DR_X + DR_BAR_W + 10;
  const bool on_about = dr_has_about() && f.x0 > DR_ROW1_X && f.x0 < DR_ROW1_X + DR_ROW_W && f.y0 > DR_ROW1_Y && f.y0 < DR_ROW1_Y + DR_ROW_H;
  if (f.pressed) dp_armed = f.owner ? 0 : on_bar ? 2 : on_about ? 3 : 0;
  if (dp_armed == 2 && f.down && (!f.dragging || f.horiz) && halo_finger_claim(&dp_armed))
    dp_set((int)lroundf(halo_clamp01((f.x - DR_X) / DR_BAR_W) * DP_STEPS));
  if (dp_armed == 3 && f.dragging) dp_armed = 0;
  if (dp_armed == 3 && f.tap) gl_req = dp_link, gl_req_depth = halo_nav_depth();
  press_update(dr_press, dp_armed == 3 && f.down, dt);
  if (!f.down) dp_armed = 0;
}

static void dr_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, const HaloView &v, const HaloItem *on, float a = 1) {
  rg_text(it, f, s, mid, align, x, y, c, a, on);
  it.on_black = on == &halo_black;
  halo_view_commit(it, v);
}

static void display_layout(const HaloView &v) {
  dr_text(dr_title, gl_r_title, "Settings", dr_mid_title, 0, RG_PAD, RG_TITLE_Y, GL_TEXT, v, &halo_black);
  const float p = halo_clamp01(dr_press.s.x);
  for (int i = 0; i < 2; i++) {
    const float y = i ? DR_ROW1_Y : DR_ROW0_Y, a = i && !dr_has_about() ? 0 : 1, cy = y + DR_ROW_H / 2, rx = i ? DR_ROW1_X : DR_ROW_X;
    HaloItem &b = dr_row[i];
    b.kind = HI_RRECT, b.x = rx + DR_ROW_W / 2, b.y = cy, b.w = DR_ROW_W, b.h = DR_ROW_H, b.r = RG_RADIUS + (i ? (RG_RADIUS_DOWN - RG_RADIUS) * p : 0), b.scale = 1;
    b.color = i ? DR_CARD : 0x254552, b.alpha = a, b.on_black = true, b.on = nullptr;
    halo_view_commit(b, v);
    gl_text_mid(dr_row_ic[i], gl_r_ic36, i ? DR_ICON_CHIP : DR_ICON_SUN, dr_mid_ic, rx + RK(48), cy, 1, i ? 0xC9CDD2 : DR_FILL, a, &b);
    halo_view_commit(dr_row_ic[i], v);
    dr_text(dr_row_t[i], gl_r_t32, i ? "About" : "Display", dr_mid_row, 0, rx + RK(92), cy, GL_TEXT, v, &b, a);
  }
  dr_text(dr_label, gl_r_t44, "Brightness", dr_mid_label, 0, DR_X, DR_LABEL_Y, GL_TEXT, v, &halo_black);
  snprintf(dr_pct_s, sizeof dr_pct_s, "%d%%", (int)lroundf(halo_clamp01(dp_shown / DP_STEPS) * 100));
  dr_text(dr_pct, gl_r_n72, dr_pct_s, dr_mid_pct, 2, DR_X + DR_BAR_W, DR_LABEL_Y, GL_TEXT, v, &halo_black);
  // The bar: the filled part ends in a round cap and never gets narrower than the cap
  const float f = fmaxf(DR_BAR_H, DR_BAR_W * halo_clamp01(dp_shown / DP_STEPS));
  dr_bar.kind = HI_RRECT, dr_bar.x = DR_X + DR_BAR_W / 2, dr_bar.y = DR_BAR_Y, dr_bar.w = DR_BAR_W, dr_bar.h = DR_BAR_H, dr_bar.r = DR_BAR_H / 2;
  dr_bar.scale = 1, dr_bar.color = DR_CARD, dr_bar.alpha = 1, dr_bar.on_black = true, dr_bar.on = nullptr;
  dr_fill.kind = HI_RRECT, dr_fill.x = DR_X + f / 2, dr_fill.y = DR_BAR_Y, dr_fill.w = f, dr_fill.h = DR_BAR_H, dr_fill.r = DR_BAR_H / 2;
  dr_fill.scale = 1, dr_fill.color = DR_FILL, dr_fill.alpha = 1, dr_fill.on_black = false, dr_fill.on = nullptr;
  halo_view_commit(dr_bar, v), halo_view_commit(dr_fill, v);
  gl_text_mid(dr_sun, gl_r_ic36, DR_ICON_SUN, dr_mid_ic, DR_X + DR_BAR_H / 2, DR_BAR_Y, 1.35f, DR_ON_FILL, 1, &dr_fill);
  halo_view_commit(dr_sun, v);
  const char *hint = dp_hint_text();
  dr_text(dr_hint, gl_t24, hint, dr_mid_hint, 0, DR_X, DR_HINT_Y, GL_DIM, v, &halo_black, *hint ? 1 : 0);
}

static void display_draw() {
  halo_item_draw(dr_title);
  for (int i = 0; i < 2; i++) halo_item_draw(dr_row[i]), halo_item_draw(dr_row_ic[i]), halo_item_draw(dr_row_t[i]);
  halo_item_draw(dr_label), halo_item_draw(dr_pct);
  halo_item_draw(dr_bar), halo_item_draw(dr_fill), halo_item_draw(dr_sun);
  halo_item_draw(dr_hint);
}
#endif  // HALO_SQUARE
