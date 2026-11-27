// device_rect.h — About on a rectangular screen (variant A3 of web/mockups/rect-apps.html): at the left one thing large (its
// number, two lines, a chart or the spectrum), at the right the list of four; tap a row and the left card shows it. All on
// one screen, so there are no separate screens per thing as on the watch. This board has no battery and no motion sensor.
// The numbers come from device.h. Included from it in place of its round screen.
// The square, 480 × 480 (variant 6C of web/mockups/square-screens.html): no big card — five rows, each its thing's
// number and a small live picture: the processor (both cores' average), the memory and the temperature as charts, the
// sound as its spectrum (the microphones are on while the screen is), the battery as a bar. Nothing to tap.

#if defined(HALO_WATCH)
// A rectangular watch, 410 × 502 (variant 1C of web/mockups/watch206-screens.html): six tiles two by two — the
// processor, the memory, the temperature, the battery, the sound, the motion: each its icon in a circle, its number, its
// name; all on one screen. A tap on a tile — that thing large: its number, two lines and its chart (the sound — its
// spectrum, the battery — its bar, the motion — a bubble level); another tap — the tiles again. The numbers: device.h
extern const HaloFont gl_r_n110, gl_w_n30, gl_w_ic30;
#define DZ_X 26.0f
#define DZ_PAD 14.0f
#define DZ_TILE_W ((HALO_W - 2 * DZ_PAD - 10) / 2)
#define DZ_TILE_H 118.0f
#define DZ_Y0 108.0f
#define DZ_STEP_Y (DZ_TILE_H + 8)
#define DZ_TILE_R 30.0f
#define DZ_CARD 0x26292D
#define DZ_CH_W 340
#define DZ_CH_H 150
static const uint8_t DZ_WHAT[6] = {DV_CPU, DV_MEM, DV_TEMP, DV_BAT, DV_SND, DV_MOT};
static const uint32_t DZ_COL[6] = {0x8AD6F0, 0xC9A7E8, 0xFFAB91, 0x6DD58C, 0xF6B5EA, 0xFFC979};
static const char *const DZ_NAME[6] = {"Processor", "Memory", "Temperature", "Battery", "Sound", "Motion"};
static const char *const DZ_ICON[6] = {"\xEF\x8B\x9B", "\xEF\x94\xB8", "\xEF\x8B\x89", "\xEF\x89\x80", "\xEF\x84\xB0", "\xEF\x88\x9E"};
static int dz_sel = -1, dz_armed = -1;  // the tile shown large (-1 — the tiles); the finger came down on (6 — the large one)
static Press dz_press[6];
static bool dz_ready;
static float dz_mid_title, dz_mid_val, dz_mid_name, dz_mid_ic, dz_mid_big;

static void dz_tile(int i, float &x, float &y) {
  x = DZ_PAD + (i % 2) * (DZ_TILE_W + 10), y = DZ_Y0 + (i / 2) * DZ_STEP_Y;
}

// Every frame while the screen is on top (face.h): a tap on a tile opens it large, a tap on the large one — the tiles
static void dv_rect_update(const FaceScreen &S, float dt) {
  HaloFinger &f = halo_finger;
  bool has = false;
  for (int i = 0; i < S.n; i++) has |= S.layers[i].kind == FL_DEVICE && !S.layers[i].hidden;
  if (!has) return;
  if (f.pressed) {
    dz_armed = -1;
    if (!f.owner && dz_sel >= 0) dz_armed = 6;
    for (int i = 0; i < 6 && !f.owner && dz_sel < 0; i++) {
      float x, y;
      dz_tile(i, x, y);
      if (f.x0 >= x && f.x0 <= x + DZ_TILE_W && f.y0 >= y && f.y0 <= y + DZ_TILE_H) dz_armed = i;
    }
  }
  if (f.dragging) dz_armed = -1;
  if (f.tap && dz_armed >= 0) dz_sel = dz_armed == 6 ? -1 : dz_armed;
  for (int i = 0; i < 6; i++) press_update(dz_press[i], dz_armed == i && f.down, dt);
  if (!f.down) dz_armed = -1;
}

static void dv_detail_layout(const FaceLayer &, DvRun &d, const HaloView &v) {
  if (!dz_ready) {
    dz_ready = true;
    dz_mid_title = halo_text_mid(gl_title, "About"), dz_mid_val = halo_text_mid(gl_w_n30, "0123456789");
    dz_mid_name = halo_text_mid(gl_t20, "Processor"), dz_mid_ic = halo_text_mid(gl_w_ic30, DZ_ICON[0]);
    dz_mid_big = halo_text_mid(gl_r_n110, "0123456789");
  }
  if (v.a > 0.01f) dv_seen(DV_SND), dv_seen(DV_MOT);  // (live: the microphones and the motion sensor while this is open)
#ifdef __EMSCRIPTEN__
  const HaloBattery hb = {true, true, true, true, 89, 4118, 5050, 4970, 384};
#else
  const HaloBattery hb = halo_battery();
#endif
  if (!d.text_at || (!fc_calm && halo_ms() - d.text_at >= 500)) {
    d.text_at = halo_ms() | 1;
    for (int i = 0; i < 6; i++) dc_value(DZ_WHAT[i], d.row[i], sizeof d.row[i]);
    if (dz_sel >= 0) {  // the large one: its number and its two lines
      const int what = DZ_WHAT[dz_sel];
      if (what == DV_BAT) {
        hb.ok && hb.present && hb.percent >= 0 ? snprintf(d.big, sizeof d.big, "%d%%", hb.percent) : snprintf(d.big, sizeof d.big, "--");
        snprintf(d.lab_s[0], 32, "%.2f V", hb.mv / 1000.0f);
        snprintf(d.lab_s[1], 32, "%s", !hb.present ? "no battery" : hb.charging ? "charging" : hb.usb ? "on USB" : "on battery");
      } else if (what == DV_MOT) {
        snprintf(d.big, sizeof d.big, "%.2f", sqrtf(dv_ax * dv_ax + dv_ay * dv_ay + dv_az * dv_az));
        snprintf(d.lab_s[0], 32, "g  ·  x %.2f  y %.2f", dv_ax, dv_ay), snprintf(d.lab_s[1], 32, "z %.2f", dv_az);
      } else {
        char l0[32], l1[32];
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
            snprintf(l0, 32, "ESP32-S3 chip"), snprintf(l1, 32, "%d MHz", dv_mhz);
            break;
          default:
            snprintf(d.big, sizeof d.big, "\xE2\x88\x92%d", (int)lroundf(fminf(90, -dv_db)));
            snprintf(l0, 32, "dB"), snprintf(l1, 32, "2 microphones");
        }
        memcpy(d.lab_s[0], l0, 32), memcpy(d.lab_s[1], l1, 32);
      }
    }
  }
  HaloItem *it = d.it;
  int n = 0;
  auto text = [&](const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, const HaloItem *on) {
    HaloItem &t = it[n++];
    rg_text(t, f, s, mid, align, x, y, c, 1, on);
    t.on_black = on == &halo_black;
  };
  auto shape = [&](int kind, float cx, float cy, float w, float h, float r, uint32_t c, bool black) -> HaloItem & {
    HaloItem &q = it[n++];
    q.kind = kind, q.x = cx, q.y = cy, q.w = w, q.h = h, q.r = r, q.scale = 1, q.color = c, q.alpha = 1, q.on_black = black, q.on = nullptr;
    return q;
  };
  if (dz_sel < 0) {  // the tiles
    text(gl_title, "About", dz_mid_title, 0, DZ_X, RG_TITLE_Y, DV_TEXT, &halo_black);
    for (int i = 0; i < 6; i++) {
      float x, y;
      dz_tile(i, x, y);
      const float p = halo_clamp01(dz_press[i].s.x);
      HaloItem &b = shape(HI_RRECT, x + DZ_TILE_W / 2, y + DZ_TILE_H / 2, DZ_TILE_W, DZ_TILE_H, DZ_TILE_R - 12 * p, DZ_CARD, true);
      HaloItem &c = shape(HI_CIRCLE, x + 38, y + 38, 0, 0, 24, DZ_COL[i], false);
      text(gl_w_ic30, DZ_ICON[i], dz_mid_ic, 1, x + 38, y + 38, 0x1B1B1F, &c);
      text(gl_w_n30, d.row[i], dz_mid_val, 0, x + 18, y + 78, DV_TEXT, &b);
      text(gl_t20, DZ_NAME[i], dz_mid_name, 0, x + 18, y + 104, DV_DIM, &b);
    }
    return dv_commit(d, n, v, nullptr);
  }
  // One of them large
  const int what = DZ_WHAT[dz_sel];
  const uint32_t acc = DZ_COL[dz_sel];
  text(gl_title, DZ_NAME[dz_sel], dz_mid_title, 0, DZ_X, RG_TITLE_Y, acc, &halo_black);
  HaloItem &card = shape(HI_RRECT, HALO_W / 2.0f, 300, HALO_W - 2 * DZ_PAD, 376, 40, DZ_CARD, true);
  text(gl_r_n110, d.big, dz_mid_big, 0, DZ_X + 8, 172, DV_TEXT, &card);
  text(gl_t20, d.lab_s[0], dz_mid_name, 0, DZ_X + 10, 230, DV_DIM, &card);
  text(gl_t20, d.lab_s[1], dz_mid_name, 0, DZ_X + 10, 258, DV_DIM, &card);
  const float cx0 = (HALO_W - DZ_CH_W) / 2.0f, cy0 = 300, cw = DZ_CH_W, ch = DZ_CH_H;
  if (what == DV_SND) {  // the spectrum: a bar a band
    const float pitch = cw / DV_NB;
    for (int j = 0; j < DV_NB; j++) {
      const float bh = 8 + (ch - 16) * dv_band[j];
      shape(HI_RRECT, cx0 + pitch * (j + 0.5f), cy0 + ch - bh / 2, 12, bh, 6, dv_mix(0x4A3A48, acc, dv_band[j]), false);
    }
  } else if (what == DV_BAT) {  // the charge: a bar
    const float f = hb.ok && hb.present && hb.percent >= 0 ? hb.percent / 100.0f : 0, w = fmaxf(28, cw * f);
    shape(HI_RRECT, cx0 + cw / 2, cy0 + ch / 2, cw, 28, 14, 0x3A3F44, false);
    shape(HI_RRECT, cx0 + w / 2, cy0 + ch / 2, w, 28, 14, acc, false).alpha = f > 0 ? 1 : 0;
  } else if (what == DV_MOT) {  // a bubble level: the bubble goes where the watch tilts
    const float R0 = 64, mx = HALO_W / 2.0f, my = cy0 + ch / 2;
    shape(HI_CIRCLE, mx, my, 0, 0, R0, 0x1E2024, false);
    float bx = -dv_ay * 160, by = -dv_ax * 160;
    const float m = sqrtf(bx * bx + by * by);
    if (m > R0 - 14) bx *= (R0 - 14) / m, by *= (R0 - 14) / m;
    shape(HI_CIRCLE, mx + bx, my + by, 0, 0, 12, acc, false);
  } else {  // a chart and the dot at its newest end
    if (!fc_calm || d.chart.cur < 0) dv_chart(d, what, DZ_CH_W, DZ_CH_H, true);
    HaloItem &m = it[n++];
    m.kind = HI_MASK, m.mask = d.chart.cur >= 0 ? &d.chart.m[d.chart.cur] : nullptr, m.color = acc, m.x = cx0 + cw / 2, m.y = cy0 + ch / 2;
    m.scale = 1, m.alpha = m.mask ? 1 : 0, m.on = &card, m.on_black = false;
    for (int j = 0; j < (what == DV_CPU ? 2 : 1); j++) {
      HaloItem &e = shape(HI_CIRCLE, cx0 + cw, cy0 + (j ? d.chart.last_y2 : d.chart.last_y), 0, 0, j ? 4.5f : 6, acc, false);
      e.alpha = m.mask ? (j ? 0.6f : 1) : 0;
    }
  }
  dv_commit(d, n, v, nullptr);
}
#elif defined(HALO_SQUARE)
extern const HaloFont gl_r_title, gl_r_n36, gl_c_t14, gl_q_ic22;
#define DQ_X 16.0f
#define DQ_W 448.0f
#define DQ_Y0 52.0f
#define DQ_STEP 82.0f
#define DQ_H 74.0f
#define DQ_R 18.0f
#define DQ_IC_X 48.0f      // the icon's circle: its middle, its radius
#define DQ_IC_R 20.0f
#define DQ_TX 80.0f        // the name and the value: their left end, their middles from the row's top
#define DQ_NAME_Y 24.0f
#define DQ_VAL_Y 52.0f
#define DQ_CH_X 276.0f     // the picture at the right
#define DQ_CH_W 168
#define DQ_CH_H 46
#define DQ_CH_Y 14.0f
#define DQ_CARD 0x2D3236
static const uint8_t DQ_WHAT[5] = {DV_CPU, DV_MEM, DV_TEMP, DV_SND, DV_BAT};
static const uint32_t DQ_COL[5] = {0x8AD6F0, 0xC9A7E8, 0xFFAB91, 0x8FDCAE, 0x6DD58C};
static const char *const DQ_NAME[5] = {"Processor", "Memory", "Temperature", "Sound", "Battery"};
static const char *const DQ_ICON[5] = {"\xEF\x8B\x9B", "\xEF\x94\xB8", "\xEF\x8B\x89", "\xEF\x84\xB0", "\xEF\x89\x80"};
static bool dq_ready;
static float dq_mid_title, dq_mid_name, dq_mid_val, dq_mid_ic;

static void dv_rect_update(const FaceScreen &, float) {}

// A small chart of this what into c (the card's kind: the processor — both cores' average)
static void dq_chart(DvChart &c, int what) {
  const uint32_t gen = what == DV_TEMP ? dv_tgen : dv_gen;
  if (c.x >= 0) return dv_bake_step(c, DV_COLS);
  if (c.cur >= 0 && c.gen == gen && c.what == what) return;
  float a[DV_HN], b[DV_HN];
  int n = 0, slots = 0;
  if (what == DV_CPU) {
    slots = 80, n = dv_series(dv_load1, a, slots), dv_series(dv_load0, b, slots);
    for (int i = 0; i < n; i++) a[i] = (a[i] + b[i]) / 2;
    dv_places(a, n, DQ_CH_H, 0, 0, 30, 4);
  } else if (what == DV_MEM) {
    slots = 120, n = dv_series(dv_free, a, slots);
    dv_places(a, n, DQ_CH_H, 0, 0, 16, 4);
  } else {
    slots = 60, n = dv_series(dv_temp, a, slots);
    dv_places(a, n, DQ_CH_H, 0, 0, 2, 4);
  }
  if (n < 1) return;
  dv_bake_start(c, DQ_CH_W, DQ_CH_H, a, nullptr, n, slots);
  c.next_gen = gen, c.next_what = what;
  dv_bake_step(c, DV_COLS);
}

static void dv_detail_layout(const FaceLayer &, DvRun &d, const HaloView &v) {
  if (!dq_ready) {
    dq_ready = true;
    dq_mid_title = halo_text_mid(gl_r_title, "About"), dq_mid_name = halo_text_mid(gl_c_t14, "Processor");
    dq_mid_val = halo_text_mid(gl_r_n36, "0123456789"), dq_mid_ic = halo_text_mid(gl_q_ic22, DQ_ICON[0]);
  }
  if (v.a > 0.01f) dv_seen(DV_SND);  // (the sound's row: the microphones are on while this is open)
  if (!d.text_at || (!fc_calm && halo_ms() - d.text_at >= 500)) {
    d.text_at = halo_ms() | 1;
    for (int i = 0; i < 4; i++) dc_value(DQ_WHAT[i], d.row[i], sizeof d.row[i]);
    dc_value(DV_BAT, d.val, sizeof d.val);
  }
  HaloItem *it = d.it;
  int n = 0;
  auto text = [&](const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, const HaloItem *on) {
    HaloItem &t = it[n++];
    rg_text(t, f, s, mid, align, x, y, c, 1, on);
    t.on_black = on == &halo_black;
  };
  auto bar = [&](float cx, float cy, float w, float h, float r, uint32_t c, bool black) -> HaloItem & {
    HaloItem &q = it[n++];
    q.kind = HI_RRECT, q.x = cx, q.y = cy, q.w = w, q.h = h, q.r = r, q.scale = 1, q.color = c, q.alpha = 1, q.on_black = black, q.on = nullptr;
    return q;
  };
  text(gl_r_title, "About", dq_mid_title, 0, RG_PAD, RG_TITLE_Y, DV_TEXT, &halo_black);
  for (int i = 0; i < 5; i++) {
    const float y = DQ_Y0 + i * DQ_STEP, cy = y + DQ_H / 2;
    const uint32_t col = DQ_COL[i], tint = dv_mix(DQ_CARD, col, 0.25f);
    HaloItem &b = bar(DQ_X + DQ_W / 2, cy, DQ_W, DQ_H, DQ_R, DQ_CARD, true);
    HaloItem &c = it[n++];
    c.kind = HI_CIRCLE, c.x = DQ_IC_X, c.y = cy, c.r = DQ_IC_R, c.scale = 1, c.color = tint, c.alpha = 1, c.on_black = false, c.on = nullptr;
    text(gl_q_ic22, DQ_ICON[i], dq_mid_ic, 1, DQ_IC_X, cy, col, &c);
    text(gl_c_t14, DQ_NAME[i], dq_mid_name, 0, DQ_TX, y + DQ_NAME_Y, DV_DIM, &b);
    text(gl_r_n36, i < 4 ? d.row[i] : d.val, dq_mid_val, 0, DQ_TX, y + DQ_VAL_Y, DV_TEXT, &b);
    const float x0 = DQ_CH_X, y0 = y + DQ_CH_Y;
    const int what = DQ_WHAT[i];
    if (what == DV_SND) {  // the spectrum: a bar a band, from the bottom
      const float pitch = DQ_CH_W / (float)DV_NB;
      for (int j = 0; j < DV_NB; j++) {
        const float h = 4 + (DQ_CH_H - 4) * dv_band[j];
        bar(x0 + pitch * (j + 0.5f), y0 + DQ_CH_H - h / 2, 6, h, 3, dv_mix(0x4A5A52, col, dv_band[j]), false);
      }
    } else if (what == DV_BAT) {  // the charge: a bar
#ifdef __EMSCRIPTEN__
      const HaloBattery hb = {true, true, true, true, 89, 4118, 5050, 4970, 384};
#else
      const HaloBattery hb = halo_battery();
#endif
      const float f = hb.ok && hb.present && hb.percent >= 0 ? hb.percent / 100.0f : 0, w = fmaxf(14, DQ_CH_W * f);
      bar(x0 + DQ_CH_W / 2, cy, DQ_CH_W, 14, 7, 0x3A3F44, false);
      bar(x0 + w / 2, cy, w, 14, 7, col, false).alpha = f > 0 ? 1 : 0;
    } else {  // a small chart and the dot at its newest end
      DvChart &ch = d.rows[i];
      if (!fc_calm || ch.cur < 0) dq_chart(ch, what);
      HaloItem &m = it[n++];
      m.kind = HI_MASK, m.mask = ch.cur >= 0 ? &ch.m[ch.cur] : nullptr, m.color = col, m.x = x0 + DQ_CH_W / 2.0f, m.y = y0 + DQ_CH_H / 2.0f;
      m.scale = 1, m.alpha = m.mask ? 1 : 0, m.on = &b, m.on_black = false;
      HaloItem &e = it[n++];
      e.kind = HI_CIRCLE, e.x = x0 + DQ_CH_W, e.y = y0 + ch.last_y, e.r = 5, e.scale = 1, e.color = col;
      e.alpha = m.mask ? 1 : 0, e.on_black = false, e.on = nullptr;
    }
  }
  dv_commit(d, n, v, nullptr);
}
#else  // (the other rectangles)

extern const HaloFont gl_r_n110, gl_r_n36, gl_r_t32, gl_r_ic36, gl_r_title;

#ifdef HALO_COMPACT  // a small rectangle: wide — the list at the right; tall — the list under the card. No microphones
                     // here: three rows
#define DA_WIDE (HALO_W >= HALO_H)
#define DA_CARD_X 12.0f
#define DA_CARD_Y 58.0f
#define DA_CARD_W (DA_WIDE ? 298.0f : HALO_W - 24.0f)
#define DA_CARD_H (DA_WIDE ? 250.0f : 200.0f)
#define DA_ROW_X (DA_WIDE ? 320.0f : 12.0f)
#define DA_ROW_Y0 (DA_WIDE ? DA_CARD_Y : DA_CARD_Y + DA_CARD_H + 10)
#define DA_ROW_W (DA_WIDE ? 148.0f : HALO_W - 24.0f)
#define DA_ROW_H (DA_WIDE ? 76.0f : 64.0f)
#define DA_ROW_STEP (DA_WIDE ? 84.0f : 70.0f)
#define DA_CH_W (DA_WIDE ? 256 : 254)
#define DA_CH_H (DA_WIDE ? 108 : 70)
#define DA_CH_Y (DA_WIDE ? 128.0f : 120.0f)   // the chart's top from the card's
#define DA_LAB_X 150.0f                       // the two lines beside the number: not nearer than this
#define DA_ROW_NAME_Y (DA_WIDE ? 26.0f : 21.0f)  // a row: its name and its value from its top
#define DA_ROW_VAL_Y (DA_WIDE ? 52.0f : 43.0f)
#define DA_ROW_IN 46.0f                       // and from its left edge
#define DA_FONT_LAB gl_c_t14
#define DA_CHIP "ESP32-S3 chip"
#define DA_CARD 0x2D3236
#define DA_ROWS 3
extern const HaloFont gl_c_t14;
#elif defined(HALO_P4_TALL)  // the 7″ board held tall (rect-tall.html 6A): the big card across the top, the four rows the
                             // whole width under it — a row's name at the left, its value at the right
#define DA_CARD_X 32.0f
#define DA_CARD_Y 96.0f
#define DA_CARD_W 536.0f
#define DA_CARD_H 430.0f
#define DA_ROW_X 32.0f
#define DA_ROW_W 536.0f
#define DA_ROW_H 96.0f
#define DA_ROW_STEP 108.0f
#define DA_CH_W 476
#define DA_CH_H 170
#define DA_CARD 0x2D3236
#define DA_ROWS 4
#define DA_ROW_Y0 544.0f
#define DA_CH_Y 230.0f
#define DA_LAB_X 300.0f
#define DA_ROW_NAME_Y 48.0f     // (the name and the value on one line, in the row's middle)
#define DA_ROW_VAL_Y 48.0f
#define DA_ROW_IN 80.0f
#define DA_ROW_VAL_R 30.0f      // the value: its right end this far from the row's
#define DA_FONT_LAB gl_t24
#define DA_CHIP "ESP32-P4 chip"
#else
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
#define DA_ROW_Y0 DA_CARD_Y     // the list: from the card's top
#define DA_CH_Y 220.0f          // the chart's top from the card's
#define DA_LAB_X 300.0f         // the two lines beside the number: not nearer than this
#define DA_ROW_NAME_Y 34.0f     // a row: its name and its value from its top
#define DA_ROW_VAL_Y 74.0f
#define DA_ROW_IN 76.0f         // and from its left edge
#define DA_FONT_LAB gl_t24      // the two lines, the rows' names
#define DA_CHIP "ESP32-P4 chip"
#endif

static const uint8_t DA_WHAT[4] = {DV_CPU, DV_MEM, DV_TEMP, DV_SND};
static const uint32_t DA_COL[4] = {0x8AD6F0, 0xC9A7E8, 0xFFAB91, 0x8FDCAE};
static const char *const DA_NAME[4] = {"Processor", "Memory", "Temperature", "Sound"};
static const char *const DA_ICON[4] = {"\xEF\x8B\x9B", "\xEF\x94\xB8", "\xEF\x8B\x89", "\xEF\x84\xB0"};
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
  da_mid_t32 = halo_text_mid(gl_r_t32, "Processor"), da_mid_t24 = halo_text_mid(DA_FONT_LAB, "Core 0");
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
      if (!f.owner && f.x0 >= DA_ROW_X && f.x0 <= DA_ROW_X + DA_ROW_W && f.y0 >= DA_ROW_Y0 + i * DA_ROW_STEP && f.y0 <= DA_ROW_Y0 + i * DA_ROW_STEP + DA_ROW_H) da_armed = i;
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
      snprintf(l0, 32, DA_CHIP), snprintf(l1, 32, "%d MHz", dv_mhz);
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
#ifndef HALO_COMPACT
  if (v.a > 0.01f) dv_seen(DV_SND);  // (the list shows the sound's level too: the microphones are on while this is open)
#endif
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
  text(gl_r_title, "About", da_mid_title, 0, RG_PAD, RG_TITLE_Y, DV_TEXT, &halo_black);
  // The big card
  HaloItem &card = bar(DA_CARD_X + DA_CARD_W / 2, DA_CARD_Y + DA_CARD_H / 2, DA_CARD_W, DA_CARD_H, RG_RADIUS, DA_CARD, true);
  text(gl_r_t32, DA_NAME[da_sel], da_mid_t32, 0, DA_CARD_X + RK(30), DA_CARD_Y + RK(44), DV_TEXT, &card);
  text(gl_r_n110, d.big, da_mid_big, 0, DA_CARD_X + RK(30), DA_CARD_Y + RK(130), DV_TEXT, &card);
  const float bw = halo_text_width(gl_r_n110, d.big), lx = DA_CARD_X + fmaxf(DA_LAB_X, RK(30) + bw + RK(36));
  text(DA_FONT_LAB, d.lab_s[0], da_mid_t24, 0, lx, DA_CARD_Y + RK(108), DV_DIM, &card);
  text(DA_FONT_LAB, d.lab_s[1], da_mid_t24, 0, lx, DA_CARD_Y + RK(148), DV_DIM, &card);
  const float cx0 = DA_CARD_X + RK(30), cy0 = DA_CARD_Y + DA_CH_Y, cw = DA_CH_W, ch = DA_CH_H;
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
    const float y = DA_ROW_Y0 + i * DA_ROW_STEP, cy = y + DA_ROW_H / 2, p = halo_clamp01(da_press[i].s.x), m = halo_clamp01(da_m[i].x);
    HaloItem &b = bar(DA_ROW_X + DA_ROW_W / 2, cy, DA_ROW_W, DA_ROW_H, RG_RADIUS + (RG_RADIUS_DOWN - RG_RADIUS) * p, dv_mix(DA_CARD, DA_COL[i], 0.22f * m), true);
    text(gl_r_ic36, DA_ICON[i], da_mid_ic, 1, DA_ROW_X + RK(42) - (DA_ROW_IN < 76 ? 6 : 0), cy, DA_COL[i], &b);
    text(DA_FONT_LAB, DA_NAME[i], da_mid_t24, 0, DA_ROW_X + DA_ROW_IN, y + DA_ROW_NAME_Y, DV_DIM, &b);
#ifdef DA_ROW_VAL_R
    text(gl_r_n36, d.row[i], da_mid_val, 2, DA_ROW_X + DA_ROW_W - DA_ROW_VAL_R, y + DA_ROW_VAL_Y, DV_TEXT, &b);
#else
    text(gl_r_n36, d.row[i], da_mid_val, 0, DA_ROW_X + DA_ROW_IN, y + DA_ROW_VAL_Y, DV_TEXT, &b);
#endif
  }
  dv_commit(d, n, v, nullptr);
}
#endif  // HALO_SQUARE
