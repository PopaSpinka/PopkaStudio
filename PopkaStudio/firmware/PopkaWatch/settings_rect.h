// settings_rect.h — Settings on a rectangular screen (variant L3 of web/mockups/rect-apps.html), as on a tablet: the sections
// at the left, the setting at the right, everything on one screen. This board has one real setting, the backlight: the whole
// bar is a slider. The second row opens About when the project has it (the layer's link; none — no row).
// Included from settings.h in place of its round layout.

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
  const bool on_about = dr_has_about() && f.x0 > DR_ROW_X && f.x0 < DR_ROW_X + DR_ROW_W && f.y0 > DR_ROW1_Y && f.y0 < DR_ROW1_Y + DR_ROW_H;
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
  dr_text(dr_title, gl_r_title, "Settings", dr_mid_title, 0, 32, 45, GL_TEXT, v, &halo_black);
  const float p = halo_clamp01(dr_press.s.x);
  for (int i = 0; i < 2; i++) {
    const float y = i ? DR_ROW1_Y : DR_ROW0_Y, a = i && !dr_has_about() ? 0 : 1, cy = y + DR_ROW_H / 2;
    HaloItem &b = dr_row[i];
    b.kind = HI_RRECT, b.x = DR_ROW_X + DR_ROW_W / 2, b.y = cy, b.w = DR_ROW_W, b.h = DR_ROW_H, b.r = 26 + (i ? (18 - 26) * p : 0), b.scale = 1;
    b.color = i ? DR_CARD : 0x254552, b.alpha = a, b.on_black = true, b.on = nullptr;
    halo_view_commit(b, v);
    gl_text_mid(dr_row_ic[i], gl_r_ic36, i ? DR_ICON_CHIP : DR_ICON_SUN, dr_mid_ic, DR_ROW_X + 48, cy, 1, i ? 0xC9CDD2 : DR_FILL, a, &b);
    halo_view_commit(dr_row_ic[i], v);
    dr_text(dr_row_t[i], gl_r_t32, i ? "About" : "Display", dr_mid_row, 0, DR_ROW_X + 92, cy, GL_TEXT, v, &b, a);
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
