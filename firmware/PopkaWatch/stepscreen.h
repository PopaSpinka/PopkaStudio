// stepscreen.h — the Steps screen (PopkaStudio's own, picked from ten mockups), made of layers as the Weather screen:
// a screen with any of them pages sideways through two pages (the screen's pager, wx_page; the Page dots layer — one a
// page, with the worm). Each layer is drawn on both pages, with that page's numbers:
//   Number (FL_STNUM)    — today's steps / the average a day this week, big;
//   Caption (FL_STCAP)   — "of 10,000 today" / "a day this week";
//   Chart (FL_STCHART)   — a bar an hour from 5 A.M. (this hour lighter; hours ahead or without steps — flat) with
//                          6A · 12P · 6P / a bar a day for the last seven (the goal reached — the accent, today lighter,
//                          the others gray) with the goal dashed and the days' letters; its size (k), the accent
//                          (color — the Steps pill's ring too: the editor keeps them one), the other days (color2), the
//                          letters (color3); the bars grow as the screen opens;
//   Footer (FL_STFOOT)   — "3,580 to go" ("Goal reached") / "3 of 7 days at goal".
// Each has its place (moved by dx, dy) and look (the texts: the font picked in the editor — size, weight, width, color);
// the goal (p) — the same in every one (the editor keeps it so).
//
// The history comes from steps.h (the chip's pedometer): today an hour each and the last days' totals, kept in flash.
// The editor's page (and the command stepsdemo on the board) shows a made-up day and week.
// Only in a firmware with these layers (FN_STEPSCR — layers.h; it needs the pedometer: FN_STEPS); the editor has them.
// Included from face.h after settings.h (wx_page, wx_text, box_add; fc_h24, fc_steps).
#if defined(__EMSCRIPTEN__) || (defined(FN_STEPSCR) && FN_STEPSCR)
#define HAS_STEPSCR 1
#else
#define HAS_STEPSCR 0
#endif
#if HAS_STEPSCR && !HAS_STEPS
#error "the Steps screen needs the pedometer: FN_STEPS (studio.py sets it with the screen)"
#endif

#if HAS_STEPSCR
// ---------- The history (steps.h fills it) ----------
static uint16_t ss_hour[24];                                // today, an hour each
static uint32_t ss_day_n[7];                                // the last days' totals, by day number % 7
static int32_t ss_day_d[7] = {-1, -1, -1, -1, -1, -1, -1};  // the day each of them is (-1 — none)
static int32_t ss_today = -1;                               // today's day number (-1 — the clock is not set)
static bool ss_demo;                                        // the made-up day and week on show
static volatile int ss_cmd_demo;                            // stepsdemo: toggle

// Days since 1970-01-01 of a local date (the week's days by number — across months and years); its weekday — (d + 4) % 7
static int32_t ss_daynum(int y, int m, int d) {
  y -= m <= 2;
  const int32_t era = (y >= 0 ? y : y - 399) / 400, yoe = y - era * 400;
  const int32_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1, doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

// The mockup's day and week (the editor; stepsdemo): 6,420 today, a morning walk and lunch
static const uint16_t SS_DEMO_HOUR[24] = {0, 0, 0, 0, 0, 0, 120, 600, 1640, 710, 180, 260, 1380, 620, 210, 460, 240};
static const uint32_t SS_DEMO_WEEK[6] = {8912, 10450, 12301, 4210, 9870, 10920};  // six days back … yesterday
#define SS_DEMO_TODAY 6420
#define SS_DEMO_HOUR_NOW 16

// ---------- The places (on a page at rest, before a layer's dx, dy) ----------
#define SS_TRACK 0x2A2B2E     // the hours ahead, a day without data
#define SS_DASH 0x6E6E6E      // the goal line
#define SS_NUM_Y 108.0f       // the big number
#define SS_CAP_Y 152.0f       // the line under it
#define SS_FOOT_Y 392.0f      // what is left / days at the goal
#define SS_CX CX              // the chart: 270 x 150 round its middle, the letters under it
#define SS_CY 257.0f
#define SS_W 270.0f
#define SS_H 150.0f
#define SS_LABEL_DY 95.0f     // the hours' and the days' letters below the chart's middle
#define SS_FROM 5             // the first hour shown (5 A.M.)
#define SS_HOURS (24 - SS_FROM)
#define SS_DASHES 27          // the goal line: 5 px dashes every 10 px
// The chart's items: today — a bar an hour and three hour labels; the week — seven bars, seven days, the dashes
enum { SC_HBAR = 0, SC_HLAB = SS_HOURS, SC_WBAR = SC_HLAB + 3, SC_WLAB = SC_WBAR + 7, SC_DASH = SC_WLAB + 7,
       SC_N = SC_DASH + SS_DASHES };
static const char *const SS_WDAY[7] = {"S", "M", "T", "W", "T", "F", "S"};
static char ss_hlab[3][4];
static char ss_txt[6][24];  // the number, the caption, the footer — on each page (the same text wherever they are)
static float ss_mid_t16;
static uint32_t ss_enter_ms;
static HaloEase SS_EASE;

// What both pages show now: today's hours and count, the seven days (six back and today), the average, the goal
struct SsData {
  uint16_t hours[24];
  uint32_t days[7], today, avg, max_d;
  bool has[7];
  int now_h, wday, at_goal;
  uint16_t max_h;
};
static SsData ss_data(uint32_t goal) {
  SsData d{};
  if (ss_cmd_demo) ss_cmd_demo = 0, ss_demo = !ss_demo;
  d.now_h = SS_DEMO_HOUR_NOW, d.wday = 3;  // (the demo: Wednesday, 4 P.M.)
  if (ss_demo) {
    memcpy(d.hours, SS_DEMO_HOUR, sizeof(d.hours)), d.today = SS_DEMO_TODAY;
    for (int k = 0; k < 6; k++) d.days[k] = SS_DEMO_WEEK[k], d.has[k] = true;
  } else {
    memcpy(d.hours, ss_hour, sizeof(d.hours)), d.today = fc_steps;
    const time_t t = time(nullptr);
    struct tm lt;
    localtime_r(&t, &lt);
    d.now_h = halo_clock_valid() ? lt.tm_hour : 23, d.wday = lt.tm_wday;
    for (int k = 0; k < 6; k++) {
      const int32_t day = ss_today - (6 - k);
      d.has[k] = ss_today >= 0 && ss_day_d[day % 7] == day, d.days[k] = d.has[k] ? ss_day_n[day % 7] : 0;
    }
  }
  d.days[6] = d.today, d.has[6] = true;
  uint32_t sum = 0;
  int n = 0;
  d.max_d = goal;
  for (int k = 0; k < 7; k++)
    if (d.has[k]) sum += d.days[k], n++, d.at_goal += d.days[k] >= goal, d.max_d = max(d.max_d, d.days[k]);
  d.avg = n ? (sum + n / 2) / n : 0;
  d.max_h = 1000;
  for (int h = SS_FROM; h < 24; h++) d.max_h = max(d.max_h, d.hours[h]);
  return d;
}

static void ss_commas(char *o, uint32_t n) {  // 6420 → "6,420"
  char s[12];
  const int len = snprintf(s, sizeof(s), "%lu", (unsigned long)n);
  for (int i = 0; i < len; i++) {
    *o++ = s[i];
    if ((len - 1 - i) % 3 == 0 && i < len - 1) *o++ = ',';
  }
  *o = 0;
}
static uint32_t ss_lighter(uint32_t c) {  // this hour, today below the goal: the accent halfway to white
  const uint32_t r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
  return ((r + 255) / 2) << 16 | ((g + 255) / 2) << 8 | (b + 255) / 2;
}

static void ss_build() {
  ss_mid_t16 = halo_text_mid(gl_t16, "12P");
  halo_ease_make(SS_EASE, 0.05f, 0.7f, 0.1f, 1);  // emphasized decelerate
#ifdef __EMSCRIPTEN__
  ss_demo = true;
#endif
  halo_command("stepsdemo", [](const char *) { ss_cmd_demo = 1; });
}
static void ss_enter() {
  ss_enter_ms = halo_ms();
}

// A layer's items: a text — one a page; the chart — SC_N (> 4 KB: the heap puts it in PSRAM). wx_layer_free frees them
static void ss_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  R.g = new HaloItem[L.kind == FL_STCHART ? SC_N : 2]();
  if (L.font) R.mid = halo_text_mid(*L.font, L.kind == FL_STNUM ? "0123456789" : "of 10,000 today");
}

static void ss_text(HaloItem &it, const FaceLayer &L, float mid, const char *s, float x, float y, const WxPageView &P,
                    float a, const HaloView &v) {
  wx_text(it, *L.font, s, mid, 1, P.X(x), P.Y(y), P.s, L.color, a, &halo_black, L.spacing);
  halo_view_commit(it, v);
}
static void ss_bar(HaloItem &it, float cx, float bottom, float w, float h, float r, uint32_t color, float k,
                   const WxPageView &P, float a, const HaloView &v) {
  it.kind = HI_RRECT, it.x = P.X(cx), it.y = P.Y(bottom - h * k / 2), it.w = w, it.h = h, it.r = fminf(r, h / 2);
  it.scale = P.s * k, it.color = color, it.alpha = a, it.on_black = true;
  halo_view_commit(it, v);
}

// Layer i on both pages (a = 0 — hidden); its box and anchor for the editor — the page on show
static void ss_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  if (!R.g) return;
  const uint32_t goal = L.p > 0 ? (uint32_t)L.p : 10000;
  const SsData d = ss_data(goal);
  const WxPageView P[2] = {wx_page(S.pager.x, 0, v), wx_page(S.pager.x, 1, v)};
  float by = SS_CY;
  if (L.kind != FL_STCHART) {
    if (!L.font) a = 0;
    char *t0 = ss_txt[(L.kind - FL_STNUM) * 2], *t1 = ss_txt[(L.kind - FL_STNUM) * 2 + 1];
    char g[12];
    if (L.kind == FL_STNUM) {
      ss_commas(t0, d.today), ss_commas(t1, d.avg), by = SS_NUM_Y;
    } else if (L.kind == FL_STCAP) {
      ss_commas(g, goal), snprintf(t0, 24, "of %s today", g), snprintf(t1, 24, "a day this week"), by = SS_CAP_Y;
    } else {
      if (d.today >= goal) snprintf(t0, 24, "Goal reached");
      else ss_commas(g, goal - d.today), snprintf(t0, 24, "%s to go", g);
      snprintf(t1, 24, "%d of 7 days at goal", d.at_goal), by = SS_FOOT_Y;
    }
    for (int p = 0; p < 2; p++)
      if (L.font) ss_text(R.g[p], L, R.mid, p ? t1 : t0, CX + L.dx, by + L.dy, P[p], P[p].k * a, v);
    by += L.dy;
  } else {
    const float k = L.k > 0 ? L.k : 1, grow = halo_ease(SS_EASE, (halo_ms() - ss_enter_ms) / 600.0f);
    const float cx = SS_CX + L.dx, cy = SS_CY + L.dy, x0 = cx - SS_W * k / 2, bottom = cy + SS_H * k / 2;
    const uint32_t accent = L.color, now = ss_lighter(L.color);
    by = cy;
    // Today: a bar an hour, three hour labels
    const float hp = SS_W * k / SS_HOURS;
    for (int j = 0; j < SS_HOURS; j++) {
      const int h = SS_FROM + j;
      const float bh = h > d.now_h ? 3 : fmaxf(3, SS_H * d.hours[h] / d.max_h * grow);
      ss_bar(R.g[SC_HBAR + j], x0 + (j + 0.5f) * hp, bottom, hp / k * 0.62f, bh, 3,
             h == d.now_h ? now : h > d.now_h || !d.hours[h] ? SS_TRACK : accent, k, P[0], P[0].k * a, v);
    }
    static const int LAB_H[3] = {6, 12, 18};
    for (int j = 0; j < 3; j++) {
      const int h = LAB_H[j];
      if (fc_h24) snprintf(ss_hlab[j], sizeof(ss_hlab[j]), "%02d", h);
      else snprintf(ss_hlab[j], sizeof(ss_hlab[j]), "%d%c", h % 12 ? h % 12 : 12, h < 12 ? 'A' : 'P');
      wx_text(R.g[SC_HLAB + j], gl_t16, ss_hlab[j], ss_mid_t16, 1, P[0].X(x0 + (h - SS_FROM + 0.5f) * hp),
              P[0].Y(cy + SS_LABEL_DY * k), P[0].s * k, L.color3, P[0].k * a, &halo_black);
      halo_view_commit(R.g[SC_HLAB + j], v);
    }
    // The week: a bar a day, its letter, the goal dashed
    const float scale = d.max_d * 1.3f, wp = SS_W * k / 7, gy = bottom - SS_H * k * goal / scale;
    for (int j = 0; j < 7; j++) {
      const float bh = !d.has[j] ? 3 : fmaxf(3, SS_H * d.days[j] / scale * grow);
      ss_bar(R.g[SC_WBAR + j], x0 + (j + 0.5f) * wp, bottom, wp / k * 0.56f, bh, 8,
             !d.has[j] ? SS_TRACK : d.days[j] >= goal ? accent : j == 6 ? now : L.color2, k, P[1], P[1].k * a, v);
      wx_text(R.g[SC_WLAB + j], gl_t16, SS_WDAY[(d.wday + 1 + j) % 7], ss_mid_t16, 1, P[1].X(x0 + (j + 0.5f) * wp),
              P[1].Y(cy + SS_LABEL_DY * k), P[1].s * k, j == 6 ? GL_TEXT : L.color3, P[1].k * a, &halo_black);
      halo_view_commit(R.g[SC_WLAB + j], v);
    }
    for (int j = 0; j < SS_DASHES; j++) {  // (behind the bars: drawn first)
      HaloItem &e = R.g[SC_DASH + j];
      e.kind = HI_RRECT, e.x = P[1].X(x0 + (j * 10 + 2.5f) * k), e.y = P[1].Y(gy), e.w = 5, e.h = 1.5f, e.r = 0;
      e.scale = P[1].s * k, e.color = SS_DASH, e.alpha = P[1].k * a, e.on_black = true;
      halo_view_commit(e, v);
    }
    if (grow < 1) halo_subpixel = true;  // bars growing
  }
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  for (int j = 0, n = L.kind == FL_STCHART ? SC_N : 2; j < n; j++) box_add(R.box, R.g[j]);
  R.ax = CX + L.dx, R.ay = by;
}

static void ss_draw(const FaceScreen &S, int i) {
  const FaceRun &R = S.run[i];
  if (!R.g) return;
  if (S.layers[i].kind != FL_STCHART) return (void)(halo_item_draw(R.g[0]), halo_item_draw(R.g[1]));
  for (int j = SC_DASH; j < SC_N; j++) halo_item_draw(R.g[j]);
  for (int j = 0; j < SC_DASH; j++) halo_item_draw(R.g[j]);
}
#else
static void ss_build() {}
static void ss_enter() {}
#endif
