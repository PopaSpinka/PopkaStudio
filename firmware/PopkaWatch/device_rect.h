// device_rect.h — About on a rectangular screen (variant A3 of web/mockups/rect-apps.html): at the left one thing large (its
// number, two lines, a chart or the spectrum), at the right the list of four; tap a row and the left card shows it. All on
// one screen, so there are no separate screens per thing as on the watch. This board has no battery and no motion sensor.
// The numbers come from device.h. Included from it in place of its round screen.

extern const HaloFont gl_r_n110, gl_r_n36, gl_r_t32, gl_r_ic36, gl_r_title;

#define DA_CARD_X 32.0f
#define DA_CARD_Y 96.0f
#define DA_CARD_W 620.0f
#define DA_CARD_H 470.0f
#define DA_ROW_X 672.0f
#define DA_ROW_W 320.0f
#define DA_ROW_H 110.0f
#define DA_ROW_STEP 120.0f
#define DA_CH_W 560
#define DA_CH_H 220
#define DA_CARD 0x2D3236
#define DA_ROWS 4

static const uint8_t DA_WHAT[DA_ROWS] = {DV_CPU, DV_MEM, DV_TEMP, DV_SND};
static const uint32_t DA_COL[DA_ROWS] = {0x8AD6F0, 0xC9A7E8, 0xFFAB91, 0x8FDCAE};
static const char *const DA_NAME[DA_ROWS] = {"Processor", "Memory", "Temperature", "Sound"};
static const char *const DA_ICON[DA_ROWS] = {"\xEF\x8B\x9B", "\xEF\x94\xB8", "\xEF\x8B\x89", "\xEF\x84\xB0"};
static int da_sel = -1;      // the row shown large (-1 — the layer's own, from the editor)
static int da_armed = -1;
static HaloSpring da_m[DA_ROWS];
static Press da_press[DA_ROWS];
static HaloSpringK K_DA;
static bool da_ready;
static float da_mid_title, da_mid_big, da_mid_t32, da_mid_t24, da_mid_val, da_mid_ic;
static const DvRun *da_run;

static int da_row_of(int what) {
  for (int i = 0; i < DA_ROWS; i++)
    if (DA_WHAT[i] == what) return i;
  return 0;
}

static void da_build() {
  if (da_ready) return;
  da_ready = true;
  K_DA = halo_spring(HALO_M3X_FAST);
  da_mid_title = halo_text_mid(gl_r_title, "About"), da_mid_big = halo_text_mid(gl_r_n110, "0123456789");
  da_mid_t32 = halo_text_mid(gl_r_t32, "Processor"), da_mid_t24 = halo_text_mid(gl_t24, "Core 0");
  da_mid_val = halo_text_mid(gl_r_n36, "0123456789"), da_mid_ic = halo_text_mid(gl_r_ic36, DA_ICON[0]);
}

// Every frame while the screen is on top (face.h): the rows; the taps
static void dv_rect_update(const FaceScreen &S, float dt) {
  HaloFinger &f = halo_finger;
  bool has = false;
  for (int i = 0; i < S.n; i++) has |= S.layers[i].kind == FL_DEVICE && !S.layers[i].hidden;
  if (!has) return;
  if (f.pressed) {
    da_armed = -1;
    for (int i = 0; i < DA_ROWS; i++)
      if (!f.owner && f.x0 >= DA_ROW_X && f.x0 <= DA_ROW_X + DA_ROW_W && f.y0 >= DA_CARD_Y + i * DA_ROW_STEP && f.y0 <= DA_CARD_Y + i * DA_ROW_STEP + DA_ROW_H) da_armed = i;
  }
  if (f.dragging) da_armed = -1;
  if (f.tap && da_armed >= 0) da_sel = da_armed;
  for (int i = 0; i < DA_ROWS; i++) press_update(da_press[i], da_armed == i && f.down, dt);
  if (!f.down) da_armed = -1;
}

static void da_big(DvRun &d, int what, char *l0, char *l1) {  // the number and its two lines
  const float t = dv_last(dv_temp);
  switch (what) {
    case DV_CPU: {
      const float a = dv_last(dv_load0), b = dv_last(dv_load1);
      snprintf(d.big, sizeof d.big, "%d%%", (int)lroundf((a + b) / 2));
      snprintf(l0, 32, "Core 0  %d%%", (int)lroundf(a)), snprintf(l1, 32, "Core 1  %d%%", (int)lroundf(b));
      break;
    }
    case DV_MEM:
      snprintf(d.big, sizeof d.big, "%d", (int)lroundf(dv_last(dv_free)));
      snprintf(l0, 32, "KB free"), snprintf(l1, 32, "PSRAM %d MB free", (int)lroundf(dv_ps_free / 1024));
      break;
    case DV_TEMP:
      snprintf(d.big, sizeof d.big, "%.1f\xC2\xB0", t);
      snprintf(l0, 32, "ESP32-P4 chip"), snprintf(l1, 32, "%d MHz", dv_mhz);
      break;
    default:
      snprintf(d.big, sizeof d.big, "\xE2\x88\x92%d", (int)lroundf(fminf(90, -dv_db)));
      snprintf(l0, 32, "dB"), snprintf(l1, 32, "2 microphones");
  }
}

static void dv_detail_layout(const FaceLayer &L, DvRun &d, const HaloView &v) {
  da_build();
  da_run = &d;
  if (da_sel < 0) da_sel = da_row_of(dv_what(L));
  HaloItem *it = d.it;
  const int what = DA_WHAT[da_sel];
  const uint32_t acc = DA_COL[da_sel];
  const float dt = 1 / 60.0f;
  for (int i = 0; i < DA_ROWS; i++) da_m[i].target = i == da_sel, halo_spring_run(da_m[i], K_DA, dt);
  if (v.a > 0.01f) dv_seen(DV_SND);  // (the list shows the sound's level too: the microphones are on while this is open)
  if (!d.text_at || (!fc_calm && halo_ms() - d.text_at >= 500)) {
    d.text_at = halo_ms() | 1;
    da_big(d, what, d.lab_s[0], d.lab_s[1]);
    for (int i = 0; i < DA_ROWS; i++) dc_value(DA_WHAT[i], d.row[i], sizeof d.row[i]);
  }
  int n = 0;
  auto text = [&](const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, const HaloItem *on, float sc = 1) {
    HaloItem &t = it[n++];
    rg_text(t, f, s, mid, align, x, y, c, 1, on);
    t.on_black = on == &halo_black, t.scale = sc;
  };
  auto bar = [&](float cx, float cy, float w, float h, float r, uint32_t c, bool black, float a = 1) -> HaloItem & {
    HaloItem &q = it[n++];
    q.kind = HI_RRECT, q.x = cx, q.y = cy, q.w = w, q.h = h, q.r = r, q.scale = 1, q.color = c, q.alpha = a, q.on_black = black, q.on = nullptr;
    return q;
  };
  text(gl_r_title, "About", da_mid_title, 0, 32, 45, DV_TEXT, &halo_black);
  // The big card
  HaloItem &card = bar(DA_CARD_X + DA_CARD_W / 2, DA_CARD_Y + DA_CARD_H / 2, DA_CARD_W, DA_CARD_H, 26, DA_CARD, true);
  text(gl_r_t32, DA_NAME[da_sel], da_mid_t32, 0, DA_CARD_X + 30, DA_CARD_Y + 44, DV_TEXT, &card);
  text(gl_r_n110, d.big, da_mid_big, 0, DA_CARD_X + 30, DA_CARD_Y + 130, DV_TEXT, &card);
  const float bw = halo_text_width(gl_r_n110, d.big), lx = DA_CARD_X + fmaxf(300, 30 + bw + 36);
  text(gl_t24, d.lab_s[0], da_mid_t24, 0, lx, DA_CARD_Y + 108, DV_DIM, &card);
  text(gl_t24, d.lab_s[1], da_mid_t24, 0, lx, DA_CARD_Y + 148, DV_DIM, &card);
  const float cx0 = DA_CARD_X + 30, cy0 = DA_CARD_Y + 220, cw = DA_CH_W, ch = DA_CH_H;
  if (what == DV_SND) {  // the spectrum: a bar a band, its peak a dot above
    const float pitch = cw / DV_NB;
    for (int j = 0; j < DV_NB; j++) {
      const float bh = 10 + (ch - 24) * dv_band[j], x = cx0 + pitch * (j + 0.5f);
      bar(x, cy0 + ch - bh / 2, 20, bh, 10, dv_mix(0x4A5A52, acc, dv_band[j]), false);
      HaloItem &pk = it[n++];
      pk.kind = HI_CIRCLE, pk.x = x, pk.y = cy0 + ch - (10 + (ch - 24) * dv_peak[j]) - 10, pk.r = 4, pk.scale = 1;
      pk.color = DV_TEXT, pk.alpha = dv_peak[j] > 0.02f ? 0.75f : 0, pk.on_black = false, pk.on = nullptr;
    }
  } else {
    if (!fc_calm || d.chart.cur < 0) dv_chart(d, what, DA_CH_W, DA_CH_H, true);
    HaloItem &m = it[n++];
    m.kind = HI_MASK, m.mask = d.chart.cur >= 0 ? &d.chart.m[d.chart.cur] : nullptr, m.color = acc, m.x = cx0 + cw / 2, m.y = cy0 + ch / 2;
    m.scale = 1, m.alpha = m.mask ? 1 : 0, m.on = &card, m.on_black = false;
    for (int j = 0; j < (what == DV_CPU ? 2 : 1); j++) {
      HaloItem &e = it[n++];
      e.kind = HI_CIRCLE, e.x = cx0 + cw, e.y = cy0 + (j ? d.chart.last_y2 : d.chart.last_y), e.r = j ? 4.5f : 6, e.scale = 1;
      e.color = acc, e.alpha = m.mask ? (j ? 0.6f : 1) : 0, e.on_black = false, e.on = nullptr;
    }
  }
  // The list
  for (int i = 0; i < DA_ROWS; i++) {
    const float y = DA_CARD_Y + i * DA_ROW_STEP, cy = y + DA_ROW_H / 2, p = halo_clamp01(da_press[i].s.x), m = halo_clamp01(da_m[i].x);
    HaloItem &b = bar(DA_ROW_X + DA_ROW_W / 2, cy, DA_ROW_W, DA_ROW_H, 26 + (18 - 26) * p, dv_mix(DA_CARD, DA_COL[i], 0.22f * m), true);
    text(gl_r_ic36, DA_ICON[i], da_mid_ic, 1, DA_ROW_X + 42, cy, DA_COL[i], &b);
    text(gl_t24, DA_NAME[i], da_mid_t24, 0, DA_ROW_X + 76, y + 34, DV_DIM, &b);
    text(gl_r_n36, d.row[i], da_mid_val, 0, DA_ROW_X + 76, y + 74, DV_TEXT, &b);
  }
  dv_commit(d, n, v, nullptr);
}
