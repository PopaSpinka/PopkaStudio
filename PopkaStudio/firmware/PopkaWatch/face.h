// face.h — the watch face of projects/HaloWatch (face.h there), one to one, made of layers for PopkaStudio, and the
// screens it opens.
//
// HaloWatch has one date, one clock and one weather pill, each at its place. Here these three kinds are layers
// (layers.h, written by PopkaStudio from the editor): any number of each, drawn bottom to top in the order of the
// list, each moved by (dx, dy) from its place on HaloWatch, with its own font size, weight and width (the fonts are
// made at that size by tools/halo_assets.py, as HaloWatch's are — not stretched), letter spacing and colors; the pill
// with its own size k (its font and icons made at that size, its measurements times k). Everything else is
// HaloWatch's code as it is: the same constants and springs, letters that roll (HaloRoll), the pill following the
// temperature's width, the weather icon popping in. With the default layers.h (HaloWatch's three layers at their
// places) the screen is HaloWatch's face pixel for pixel. The demo (everything changes at once) is only the command
// demo, as on HaloWatch now: a tap used to play it — that was for testing.
//
// Screens: screen 0 is the watch face — the home; the others are made of the same layers and open right over the
// face, as HaloWatch opens its Weather screen from the pill: a layer of the face with a link (FaceLayer::tap) opens its
// screen with HaloWatch's transition (halo_nav_face_open: Fade, from the layer's place and size), back — a swipe up
// from the bottom edge, the same motion mirrored (built into lib/Halo's navigation). A tap on the face anywhere else
// does nothing.
//
// Layers that overlap: text over another layer is blended with what is under it (on HaloWatch nothing overlaps, so
// text is drawn the fast way, as if on black — over the pill that would leave dark edges).
//
// The Weather screen's parts (the clock along the top edge, the city, the icon, the temperature, the high/low, the
// hourly pill, the page dots) are layers too, from HaloWatch's weather.h one to one: wxscreen.h. So are the gallery's
// (the title, the cards, the button at the bottom edge, the scroll indicator): gallery.h.
//
// The gallery: the screen MAIN_SCREEN (layers.h; -1 — none) is lib/Halo's main screen, as HaloWatch's gallery: a swipe
// up on the face — Fade to it, a swipe down from its top edge — back (halo_nav_face). A tap on a card opens its screen
// on it with the Android 16 transition (halo_nav_open; a stack of up to 4), back — a swipe from the left edge; from
// any of them — a gesture to the face (HaloScreen::home: a screen that pages — up from the bottom edge, others — both
// ways from the very edges), as on HaloWatch.
//
// Commands: demo (everything changes at once, 10 s later the clock rolls back to the real time), wx N (weather 0..3),
// tick [N] (the clock N minutes ahead), day (a day ahead); back (lib/Halo).
// Included from PopkaWatch.ino after weather.h.

// ---------- HaloWatch's face, as it is (projects/HaloWatch/face.h) ----------
#define FACE_DATE 0xC2C3CB  // the date: light gray with a cool tint
#define FACE_WHITE 0xF8F7F9
#define FACE_PILL 0x2D2F2F
#define FACE_SCALE 1.2f  // the face is 1.2x the mockup (keep in sync with halo.conf and FACE_SCALE in gen_icons.py)
// Mockup coordinates -> screen, scaled around the screen center; sizes and offsets — just times FACE_SCALE
static inline float fsc(float v) {
  return CX + (v - CX) * FACE_SCALE;
}
#define FACE_LIFT -3.5f
// Mockup measurements, board px
#define FACE_DATE_Y (fsc(116.0f) - FACE_LIFT)   // middle of the date's capitals
#define FACE_TIME_Y (fsc(219.2f) - FACE_LIFT)   // middle of the clock digits
#define PILL_Y (fsc(336.1f) - FACE_LIFT)        // pill center
#define PILL_H (75.6f * FACE_SCALE)
#define PILL_W (186.1f * FACE_SCALE)          // pill width with "19°"
#define PILL_ICON (53.5f * FACE_SCALE)        // icon center from the pill's left edge
#define PILL_ICON_Y (fsc(337.0f) - FACE_LIFT)
#define PILL_INK (95.2f * FACE_SCALE)         // left ink edge of "19°" from the pill's left edge
#define PILL_TEMP_Y (fsc(338.0f) - FACE_LIFT)   // middle of the temperature digits
#define FACE_ICON_RISE (10 * FACE_SCALE)      // a changing icon moves up by this much
// The Steps pill (not on HaloWatch: PopkaStudio's): as tall as the weather pill; its left end — a ring to the goal round
// the shoe, then the number
#define ST_RING_R (PILL_H / 2 - 11)  // the ring's middle line
#define ST_RING_W 6.0f               // its thickness
#define ST_GAP 6.0f                  // the ring's cell to the number
#define ST_PAD 30.0f                 // the number to the pill's right end
#define ST_ICON "\uf54b"             // Font Awesome's shoe-prints

static const char *const WEEKDAY[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
static const char *const MONTH[12] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

// ---------- Layers and screens ----------
enum : uint8_t {
  FL_DATE, FL_CLOCK, FL_WEATHER,  // the watch face: the date, the clock, the weather pill
  FL_ARC, FL_CITY, FL_WXICON, FL_WXTEMP, FL_WXHILO, FL_HOURLY, FL_DOTS,  // the Weather screen (wxscreen.h)
  FL_TITLE, FL_CARD, FL_EDGE, FL_SCROLL,  // the gallery (gallery.h)
  FL_STEPS,                               // the watch face too: steps (steps.h)
  FL_STARS,                               // a starry background (stars.h)
  FL_FEELS = 17,                          // the Weather screen too: what it feels like (wxscreen.h)
  FL_DISPLAY, FL_BATTERY,                 // HaloWatch's Settings sections, a screen each (settings.h)
  FL_STNUM, FL_STCAP, FL_STFOOT, FL_STCHART,  // the Steps screen's: two pages, today and the week (stepscreen.h)
  FL_CURVE, FL_STEPRING, FL_WEEKARC, FL_INFOLINE, FL_FORECAST, FL_DATEFRAME, FL_DOTCLOCK,  // the face's widgets (widgets.h)
  FL_MUSIC,                               // HaloWatch's Music: the player, a screen of its own (settings.h, player.h)
  FL_RECORDER,                            // the Recorder: record, play back, forget (settings.h, recorder.h)
  FL_SNKDOTS, FL_SNKSPACE, FL_SNKISO,  // the Games: self-playing snakes (games.h)
  FL_BATRING,                          // the face's widgets too: the battery's charge along the bezel (widgets.h)
  FL_PARROT                            // pixel art: Popka, the parrot (pixelart.h)
};
static inline bool fl_pixels(uint8_t k) {  // pixel art drawn cell by cell (pixelart.h)
  return k == FL_PARROT
      ;
}
static inline bool fl_widget(uint8_t k) {  // a widget from the face mockups (and the battery ring, as Steps ring)
  return (k >= FL_CURVE && k <= FL_DOTCLOCK) || k == FL_BATRING;
}
static inline bool fl_wx(uint8_t k) {  // a part of the Weather screen
  return (k >= FL_ARC && k <= FL_DOTS) || k == FL_FEELS;
}
static inline bool fl_gallery(uint8_t k) {  // a part of the gallery
  return k >= FL_TITLE && k <= FL_SCROLL;
}
static inline bool fl_steps(uint8_t k) {  // a part of the Steps screen
  return k >= FL_STNUM && k <= FL_STCHART;
}
static inline bool fl_paged(uint8_t k) {  // drawn on every page of the screen's pager (one a city)
  return (k >= FL_CITY && k <= FL_HOURLY) || k == FL_FEELS;
}
struct FaceLayer {
  uint8_t kind;
  float dx, dy;                   // moved from its place on HaloWatch, px
  float k;                        // the weather pill, the hourly pill: size, 1 — as on HaloWatch
  uint32_t color;                 // the text; the pills: the pill; the dots: the current one
  uint32_t color2;                // the pills: their text; the high/low: the low; the dots: the others
  int8_t spacing;                 // letter spacing, px
  const HaloFont *font;           // the text (the pills: their text)
  const HaloImage *const *icons;  // the weather icons: by WX_PARTLY ...
  int8_t tap;                     // a tap on it opens this screen (-1 — no link; a card — a screen with its title)
  uint8_t fx;                     // on the watch face: with this transition (HALO_FX_*; HaloWatch's pill: Fade)
  float p;                        // the clock along the top edge: the radius of the middle of its letters
  bool hidden;                    // in the editor only (the board gets just the visible layers)
  const char *text, *sub, *icon;  // the gallery: the title (the card's, the button's), the line under it, the icon
  uint32_t color3;                // steps: the ring and the shoe (the goal — p)
  const HaloFont *font2;          // steps: the shoe
  float p2, p3;                   // stars: twinkling, meteors a minute (how many — p, brightness — k); pixel art: what
                                  // is around it (p2), its trail (p3; the turn — p, the size — k); steps: p2 1 — no pill
};
// A layer of the Weather screen on one page of its pager (the dots: on the first one)
struct WxRun {
  HaloRoll *roll;  // the temperature
  HaloItem it[WX_MAX + 2];  // the city, the icon: [0]; the high/low: [0], [1]; the hourly pill: [0], icons [1..3], times
                           // [4..6]; the page dots: one a city, the worm, the pulled one
};
// What a layer has while running: its letters (the weather: the temperature's) and the pill
struct FaceRun {
  HaloRoll roll;
  HaloItem pill, icon_in, icon_out;  // (steps: the pill, the shoe, the ring)
  float pen, pad_r;   // the weather: the temperature's pen from the pill's left edge, and the pill after the text
  float box[4];       // where it was drawn in the last frame (x0, y0, x1, y1; x1 < x0 — nowhere)
  float ax, ay;       // where it is (its anchor) in the last frame — the editor shows it and snaps by it
  float mid, top, bottom;  // the Weather screen: the middle of the letters; the rows of the digits
  WxRun *pg;               // the Weather screen: one per page
  HaloMask arc;            // the clock along the top edge: the time baked along the arc, where it goes
  float tx, ty;
  HaloItem *g;             // the gallery's parts: the card, its icon's circle, the icon, the title, the line under it
                           // (stars: the stars and the meteor)
  struct StarRun *stars;   // stars: where each one is, how it twinkles (stars.h)
  char *txt;               // the widgets: their texts (widgets.h)
};
struct GalRun;  // gallery.h
struct FaceScreen {
  FaceLayer *layers;
  FaceRun *run;
  int n;
  const char *name;  // shown in the middle while it has no layers — as HaloWatch's stubs (a card's screen not made yet)
  HaloSpring pager;  // the page (fractional while paging) — a screen with the Weather screen's parts
  float g0;
  bool drag;
  GalRun *gal;       // the list — a screen with the gallery's parts
  HaloItem name_it;  // (the name, while it is empty)
  float name_mid;
  bool name_ok;
};
// What the layers have while running — a FaceRun is ~4.8 KB (its roll's letters are items) — lives in PSRAM: in internal
// RAM a project of ~40 layers left Wi-Fi nothing (3 KB free, the lowest 0, with Wi-Fi on). Static state can't be put
// there by an attribute in this core, so it is allocated before setup() (PSRAM is ready by the global constructors) and
// layers.h names it by a reference (as WearLab's lab_psa)
#include <new>
#ifndef __EMSCRIPTEN__
#include <esp_heap_caps.h>
#endif
template <int N> static FaceRun (&face_runs())[N] {
#ifdef __EMSCRIPTEN__
  FaceRun *p = new FaceRun[N]();
#else
  FaceRun *p = (FaceRun *)heap_caps_calloc(N, sizeof(FaceRun), MALLOC_CAP_SPIRAM);
  if (!p) p = (FaceRun *)calloc(N, sizeof(FaceRun));
  for (int i = 0; i < N; i++) new (p + i) FaceRun();
#endif
  return *(FaceRun(*)[N])p;
}
#include "layers.h"  // screens[SCREEN_N]: 0 — the watch face, then the screens it opens
// Steps — only in a firmware with a Steps layer (FN_STEPS, written by the editor), always in the editor's page
#if defined(__EMSCRIPTEN__) || (defined(FN_STEPS) && FN_STEPS)
#define HAS_STEPS 1
#else
#define HAS_STEPS 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_STARS) && FN_STARS)  // the same for Stars
#define HAS_STARS 1
#else
#define HAS_STARS 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_PARROT) && FN_PARROT)  // and for the pixel art: Popka, the parrot
#define HAS_PARROT 1
#else
#define HAS_PARROT 0
#endif
#ifndef HAS_PIXELS
#define HAS_PIXELS HAS_PARROT
#endif

static FaceScreen *face_scr = screens;  // (the editor's page swaps in its own: engine/studio_api.h)
static int face_scr_n = SCREEN_N;
static int face_over = -1;  // the screen open over the face now
#define NAV_LEVELS 5        // the gallery and up to 4 screens over it (lib/Halo: NAV_MAX)
static int nav_scr[NAV_LEVELS] = {MAIN_SCREEN, -1, -1, -1, -1};  // the screen on each level (0 — the gallery); -1 — a stub

// HaloWatch's place of each kind: the date and the clock are centered on the screen by their middle (the clock — by
// its colon), the pill — by its center
static float face_base_x(const FaceLayer &) {
  return CX;
}
static float face_base_y(const FaceLayer &L) {
  return L.kind == FL_DATE ? FACE_DATE_Y : L.kind == FL_CLOCK ? FACE_TIME_Y : PILL_Y;  // (steps — where the pill is)
}

static HaloSpring fc_icon_t;  // icon change: 0 -> 1
static HaloSpringK K_ICON;
static int fc_kind = -1, fc_kind_old = -1;  // the weather icon now and the one leaving (-1 — none)
static char fc_temp[8], fc_time[8] = "--:--", fc_date[20];
static int fc_min = -1, fc_hour = -1, fc_yday = -1;
static bool fc_h24 = CLOCK_24H;  // the clock: 24 hours ("13:05") or 12 ("01:05", as HaloWatch) — the editor's Time
static inline int fc_shown_hour(int h) {
  return fc_h24 ? h : h % 12 ? h % 12 : 12;
}
static int32_t fc_offset;       // shift of the clock, s (the demo, tick, day)
static uint32_t fc_demo_until;  // the demo's clock goes back to real time then
static bool fc_check;
static bool fc_still;  // Always On (power.h): the clock and the date change without rolling — asleep there is no time
// The layers' animations (the stars, the pixel art) stand still: from the moment the watch starts going into Always On
// until the moment it starts coming back — in step with the move's start both ways (power.h sets it every frame)
static bool fc_calm;
static volatile int fc_cmd_wx = -1;
static volatile bool fc_cmd_demo;
static volatile int32_t fc_cmd_shift;

static float pill_width(const FaceRun &R) {
  return R.pen + halo_roll_width(R.roll) + R.pad_r;
}
#if HAS_STEPS
static uint32_t fc_steps;             // today's (steps.h)
static char fc_steps_s[16] = "0";     // as shown: 6,233
static float steps_width(const FaceLayer &L, const FaceRun &R) {
  return (PILL_H + ST_GAP + ST_PAD) * L.k + halo_roll_width(R.roll);
}
static uint32_t st_mix(uint32_t a, uint32_t b, float t) {  // color a toward b by t (the ring's track: the pill's, a hint of
  uint32_t out = 0;                                         // the ring's)
  for (int s = 0; s < 24; s += 8) {
    const float ca = (a >> s) & 255, cb = (b >> s) & 255;
    out |= (uint32_t)lroundf(ca + (cb - ca) * t) << s;
  }
  return out;
}
#endif

static void box_add(float *box, const HaloItem &it) {
  if (!it.shown) return;
  if (box[2] < box[0]) return (void)memcpy(box, it.b, sizeof(it.b));
  box[0] = fminf(box[0], it.b[0]), box[1] = fminf(box[1], it.b[1]);
  box[2] = fmaxf(box[2], it.b[2]), box[3] = fmaxf(box[3], it.b[3]);
}

static int screen_pages(const FaceScreen &S);  // (how many pages it swipes through: the dots)
#include "gallery.h"   // the gallery's layers
#include "wxscreen.h"  // the Weather screen's layers
#include "settings.h"  // Display and Battery (HaloWatch's Settings sections)
#include "stepscreen.h"  // the Steps screen
#include "widgets.h"     // the face's widgets
#include "stars.h"     // the starry background
#include "pixelart.h"  // pixel art

// Every roll of this kind on every screen gets the text s
static void face_set_all(uint8_t kind, const char *s, bool animate) {
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++)
      if (face_scr[k].layers[i].kind == kind) halo_roll_set(face_scr[k].run[i].roll, s, animate);
}

// The weather on every pill (the first city's now) and on every page — after new weather came (wxfetch.h, the editor),
// or a demo day; what changed rolls
static void face_weather_show(bool animate = true) {
  const WxDay d = wx_day(0);
  if (d.kind != fc_kind && animate) {
    fc_kind_old = fc_kind;
    halo_spring_set(fc_icon_t, 0);
    fc_icon_t.target = 1;
  }
  fc_kind = d.kind;
  wx_deg(fc_temp, sizeof(fc_temp), d.now);
  face_set_all(FL_WEATHER, fc_temp, animate);
  wx_fill(animate);  // page 0 of the Weather screen: the same weather (its temperature rolls too)
}
// A fictional day of HaloWatch's (wx N, demo); -1 — the real weather again
static void face_weather(int i, bool animate = true) {
  wx_demo = i < 0 ? -1 : (i % WX_DEMO_N + WX_DEMO_N) % WX_DEMO_N;
  face_weather_show(animate);
}

// Demo (the command demo): the clock jumps 20..320 days and some minutes ahead, picked so that every digit of the time, the weekday, the
// month and the day's last digit change — the whole face rolls at once; and the next weather. 10 s later — real time
static void face_demo() {
  face_weather(wx_demo + 1);
  if (!halo_clock_valid()) return;
  const time_t real = time(nullptr), shown = real + fc_offset;
  struct tm a, b;
  localtime_r(&shown, &a);
  const int ah = fc_shown_hour(a.tm_hour);
  for (int k = 0; k < 100; k++) {
    const int32_t off = (int32_t)(20 + esp_random() % 300) * 86400 + (int32_t)(esp_random() % 1440) * 60;
    const time_t t = real + off;
    localtime_r(&t, &b);
    const int bh = fc_shown_hour(b.tm_hour);
    if (bh / 10 != ah / 10 && bh % 10 != ah % 10 && b.tm_min / 10 != a.tm_min / 10 && b.tm_min % 10 != a.tm_min % 10 &&
        b.tm_wday != a.tm_wday && b.tm_mon != a.tm_mon && b.tm_mday % 10 != a.tm_mday % 10) {
      fc_offset = off;
      break;
    }
  }
  fc_demo_until = halo_ms() + 10000, fc_check = true;
}

// A layer from scratch (at start; in the editor — after a change of its font, spacing or size): the letters it shows
// now, in place, without rolling
// HaloWatch's sections and the games: one of a kind, a screen of its own (settings.h, games.h)
static inline bool fl_section(uint8_t k) {
  return k == FL_DISPLAY || k == FL_BATTERY || k == FL_MUSIC || k == FL_RECORDER || (k >= FL_SNKDOTS && k <= FL_SNKISO);
}
static void face_layer_build(const FaceScreen &S, int i) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  wx_layer_free(R);
#if HAS_STARS
  stars_layer_free(R);
#endif
  free(R.txt);
  // Afresh in its own place: no copy on the stack (it is ~4.8 KB; Live rebuilds a layer in the frame task — 6 KB)
  R.~FaceRun();
  new (&R) FaceRun();
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  R.ax = R.ay = NAN;  // not drawn yet
  if (fl_gallery(L.kind)) return gal_layer_build(S, i);
  if (fl_wx(L.kind)) return wx_layer_build(S, i);
  if (fl_section(L.kind)) return;  // (settings.h: one of a kind)
#if HAS_STEPSCR
  if (fl_steps(L.kind)) return ss_layer_build(S, i);
#endif
  if (fl_widget(L.kind)) return wg_layer_build(S, i);
#if HAS_STARS
  if (L.kind == FL_STARS) return stars_layer_build(S, i);
#endif
  if (!L.font) return;
#if HAS_STEPS
  if (L.kind == FL_STEPS) {  // the number rolls inside the pill, as the temperature; the shoe by its middle
    R.roll.rise = 15 * FACE_SCALE * L.k;
    halo_roll_init(R.roll, *L.font, "0123456789", 0, L.spacing);
    halo_roll_set(R.roll, fc_steps_s, false);
    R.mid = L.font2 ? halo_text_mid(*L.font2, ST_ICON) : 0;
    return;
  }
#endif
  if (L.kind == FL_CLOCK) {
    R.roll.tabular = true;  // a cell per digit: nothing shifts when digits change, the colon is right at the center
    halo_roll_init(R.roll, *L.font, "0123456789", 1, L.spacing);
    halo_roll_set(R.roll, fc_time, false);
  } else if (L.kind == FL_DATE) {
    halo_roll_init(R.roll, *L.font, "HOT0123", 1, L.spacing);
    if (*fc_date) halo_roll_set(R.roll, fc_date, false);
  } else {
    R.roll.rise = 15 * FACE_SCALE * L.k;  // the temperature rolls inside the pill (its letters are drawn with the pill's colors)
    halo_roll_init(R.roll, *L.font, "0123456789", 0, L.spacing);
    halo_roll_set(R.roll, fc_temp, false);
    // The pill is laid out by the pen of the text (the mockup gives the ink edge of "1")
    const HaloMask *one = halo_glyph(*L.font, '1');
    R.pen = PILL_INK * L.k - (one ? one->ox : 0);
    R.pad_r = PILL_W * L.k - R.pen - halo_text_width(*L.font, "19°", L.spacing);
  }
}

// A layer whose numbers changed in place (the editor's page: engine/studio_api.h; the board: live.h) — built again when
// the change needs it: another kind or font, its size k, the spacing, p (a radius, the stars' count), a curved line's
// angle; moving it, its colors and the rest are laid out by the next frame as they are
static bool face_layer_rebuild(const FaceLayer &was, const FaceLayer &L) {
  return was.kind != L.kind || was.font != L.font || was.font2 != L.font2 || was.spacing != L.spacing ||
         was.k != L.k || was.p != L.p || (L.kind == FL_CURVE && was.p3 != L.p3);
}

static void face_build() {
  K_ICON = halo_spring(0.5f, 0.7f);  // the icon pops with a visible overshoot
  K_PAGER = halo_spring(HALO_M3_PAGER);
  face_weather(-1, false);
  halo_spring_set(fc_icon_t, 1);
  settings_build();
  ss_build();
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++) face_layer_build(face_scr[k], i);
  gal_build();
#if HAS_STARS
  stars_commands();
#endif
  halo_command("wx", [](const char *a) { fc_cmd_wx = *a ? atoi(a) : -2; });  // wx N — a demo day; wx — the real weather
  halo_command("demo", [](const char *) { fc_cmd_demo = true; });
  halo_command("tick", [](const char *a) { fc_cmd_shift = 60 * (*a ? atoi(a) : 1); });
  halo_command("day", [](const char *) { fc_cmd_shift = 86400; });
}

// Every frame, whatever is on screen: the clock and the date, the rolls and the icon spring
static void face_tick(float dt) {
  static uint32_t checked;
  if (fc_cmd_wx != -1) face_weather(fc_cmd_wx < 0 ? -1 : fc_cmd_wx), fc_cmd_wx = -1;
  if (fc_cmd_demo) face_demo(), fc_cmd_demo = false;
  bool now_check = halo_ms() - checked >= 250 || fc_check;
  fc_check = false;
  if (fc_cmd_shift) fc_offset += fc_cmd_shift, fc_cmd_shift = 0, now_check = true;
  if (fc_demo_until && (int32_t)(halo_ms() - fc_demo_until) > 0) fc_offset = 0, fc_demo_until = 0, now_check = true;
  if (now_check && halo_clock_valid()) {
    checked = halo_ms();
    const time_t now = time(nullptr) + fc_offset;
    struct tm t;
    localtime_r(&now, &t);
    if (t.tm_min != fc_min || t.tm_hour != fc_hour) {
      fc_min = t.tm_min, fc_hour = t.tm_hour;
      snprintf(fc_time, sizeof(fc_time), "%02d:%02d", fc_shown_hour(fc_hour), fc_min);
      face_set_all(FL_CLOCK, fc_time, !fc_still);
    }
    if (t.tm_yday != fc_yday) {  // "THU, 29 OCT" — the mockup's format
      fc_yday = t.tm_yday;
      snprintf(fc_date, sizeof(fc_date), "%s, %d %s", WEEKDAY[t.tm_wday], t.tm_mday, MONTH[t.tm_mon]);
      face_set_all(FL_DATE, fc_date, !fc_still);
    }
  }
  for (int k = 0; k < face_scr_n; k++)
    for (int i = 0; i < face_scr[k].n; i++) halo_roll_update(face_scr[k].run[i].roll, dt);
  wx_tick(dt);
  wg_tick();
  halo_spring_run(fc_icon_t, K_ICON, dt);
  if (fc_icon_t.x == 1 && fc_icon_t.v == 0) fc_kind_old = -1;
}

// Always On (power.h): on — the clock and the date as they are now, at once (a roll still playing would freeze in
// sleep half-way) and from then on without rolling; off — rolling again (HaloWatch's face_still)
static void face_still(bool on) {
  fc_still = on;
  if (on) fc_min = fc_hour = fc_yday = -1, fc_check = true;
}

// Was anything drawn in the last frame under layer i (by the layers below it)?
static bool face_over_others(const FaceScreen &S, int i) {
  const float *a = S.run[i].box;
  if (a[2] < a[0]) return false;
  for (int j = 0; j < i; j++) {
    const float *b = S.run[j].box;
    if (b[2] >= b[0] && a[0] < b[2] && b[0] < a[2] && a[1] < b[3] && b[1] < a[3]) return true;
  }
  return false;
}

// Screen s through the view v (a = 0 — hidden): every layer, bottom to top; the Weather screen's parts — on every page
// of the screen's pager
static void screen_layout(int s, const HaloView &v) {
  if (s < 0 || s >= face_scr_n) return;
  FaceScreen &S = face_scr[s];
  if (!S.n) {  // empty (not the face): its name in the middle, as the stub of a card without a link
    if (s == 0 || !S.name) return;
    if (!S.name_ok) S.name_mid = halo_text_mid(gl_title, gl_str(S.name)), S.name_ok = true;
    gl_text_mid(S.name_it, gl_title, gl_str(S.name), S.name_mid, CX, CX, 1, GL_TEXT, 1, &halo_black);
    S.name_it.on_black = true;
    halo_view_commit(S.name_it, v);
    return;
  }
  const float t = fc_icon_t.x;
  WxPageView pages[WX_MAX];
  for (int p = 0; p < wx_pages; p++) pages[p] = wx_page(S.pager.x, p, v);
  const int shown = wx_pages > 1 ? (int)lroundf(halo_clamp01(S.pager.x / (wx_pages - 1)) * (wx_pages - 1)) : 0;
  if (S.gal && S.gal->list.moving && v.a > 0 && v.s >= 0.999f) halo_subpixel = true;  // the list moves: letters by a
                                                                                       // fraction of a pixel, or they shake
  for (int i = 0; i < S.n; i++) {
    const FaceLayer &L = S.layers[i];
    FaceRun &R = S.run[i];
    if (fl_gallery(L.kind)) {
      gal_layout(S, i, L.hidden ? 0 : 1, v);
      continue;
    }
    if (fl_section(L.kind)) {
      if (!L.hidden) settings_layout(R, L.kind, v);
      continue;
    }
    if (fl_steps(L.kind)) {
#if HAS_STEPSCR
      ss_layout(S, i, L.hidden ? 0 : 1, v);
#endif
      continue;
    }
    if (fl_widget(L.kind)) {
      wg_layout(S, i, L.hidden ? 0 : 1, v);
      continue;
    }
#if HAS_STARS
    if (L.kind == FL_STARS) {
      stars_layout(S, i, L.hidden ? 0 : 1, v);
      continue;
    }
#endif
    if (fl_pixels(L.kind)) {
#if HAS_PIXELS
      pixel_layout(S, i, L.hidden ? 0 : 1, v);
#endif
      continue;
    }
    if (fl_wx(L.kind)) {
      const float a = L.hidden || (!L.font && L.kind != FL_WXICON && L.kind != FL_DOTS) ? 0 : 1;
      if (fl_paged(L.kind)) {
        for (int p = 0; p < wx_pages; p++) wx_layout_page(S, i, p, pages[p], a, v);
        if (R.pg) wx_place(S, i, shown, R.box, R.ax, R.ay);
      } else {
        wx_layout_static(S, i, a, v);
        R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
        box_add(R.box, R.pill);
        if (R.pg) for (const HaloItem &it : R.pg[0].it) box_add(R.box, it);
      }
      continue;
    }
    const float x = face_base_x(L) + L.dx, y = face_base_y(L) + L.dy, a = L.hidden || !L.font ? 0 : 1;
    R.ax = x, R.ay = y;
    // On HaloWatch nothing overlaps: text is drawn as if on black. Over another layer — blended with it
    const bool over = face_over_others(S, i);
#if HAS_STEPS
    if (L.kind == FL_STEPS) {
      // The pill as wide as its number right now; at its left end the ring to the goal (L.p steps) round the shoe.
      // Without the pill (p2): the ring and the number alone, in the same places — on black (or blended over a layer)
      const bool bare = L.p2 > 0.5f;
      const float k = L.k, w = steps_width(L, R), left = x - w / 2, rx = left + PILL_H * k / 2;
      HaloItem &p = R.pill, &ring = R.icon_out;
      const HaloItem *on = bare ? (over ? nullptr : &halo_black) : &R.pill;
      p.kind = HI_RRECT, p.x = x, p.y = y, p.w = w, p.h = PILL_H * k, p.r = PILL_H * k / 2;
      p.color = L.color, p.on_black = !over, p.scale = 1, p.alpha = bare ? 0 : a;
      halo_view_commit(p, v);
      ring.kind = HI_RING, ring.x = rx, ring.y = y, ring.r = ST_RING_R * k, ring.w = ST_RING_W * k, ring.scale = 1;
      ring.v = halo_clamp01(fc_steps / fmaxf(1, L.p)), ring.color = L.color3;
      ring.color2 = st_mix(L.color3, bare ? 0x000000 : L.color, 0.72f);  // the track: the ring's color toward what is under
      ring.alpha = a, ring.on_black = false, ring.on = nullptr;
      halo_view_commit(ring, v);
      if (L.font2) gl_text_mid(R.icon_in, *L.font2, ST_ICON, R.mid, rx, y, 1, L.color3, a, on);
      else R.icon_in.alpha = 0;
      halo_view_commit(R.icon_in, v);
      halo_roll_layout(R.roll, left + (PILL_H + ST_GAP) * k, y + (PILL_TEMP_Y - PILL_Y) * k, L.color2, a, on, v);
    } else
#endif
    if (L.kind != FL_WEATHER) {
      halo_roll_layout(R.roll, x, y, L.color, a, over ? nullptr : &halo_black, v);
    } else {
      // The pill: as wide as its text right now (it follows the temperature's width spring)
      const float k = L.k, w = pill_width(R), left = x - w / 2;
      HaloItem &p = R.pill;
      p.kind = HI_RRECT, p.x = x, p.y = y, p.w = w, p.h = PILL_H * k, p.r = PILL_H * k / 2;
      p.color = L.color, p.on_black = !over, p.scale = 1, p.alpha = a;
      halo_view_commit(p, v);
      // The icon: the new one grows from 0.5 with an overshoot and rises a little, the old one shrinks, rises and fades
      // out quickly (gone by a third of the way: two half-transparent icons on top of each other look muddy)
      const float ix = left + PILL_ICON * k, iy = y + (PILL_ICON_Y - PILL_Y) * k;
      HaloItem &in = R.icon_in, &out = R.icon_out;
      in.kind = out.kind = HI_IMAGE, in.on_black = out.on_black = false;
      in.image = L.icons && fc_kind >= 0 ? L.icons[fc_kind] : nullptr;
      in.x = ix, in.y = iy + (1 - t) * FACE_ICON_RISE * k, in.scale = 0.5f + 0.5f * t;
      in.alpha = in.image ? a * halo_clamp01((t - 0.12f) * 1.8f) : 0;
      out.image = L.icons && fc_kind_old >= 0 ? L.icons[fc_kind_old] : nullptr;
      out.x = ix, out.y = iy - t * FACE_ICON_RISE * k, out.scale = 1 - 0.5f * t;
      out.alpha = out.image ? a * halo_clamp01(1 - t * 3) : 0;
      halo_view_commit(out, v), halo_view_commit(in, v);
      halo_roll_layout(R.roll, left + R.pen, y + (PILL_TEMP_Y - PILL_Y) * k, L.color2, a, &R.pill, v);
    }
    // Where the layer is now (for overlaps; the editor draws its frame by it)
    R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
    box_add(R.box, R.pill);
    for (int c = 0; c < R.roll.n; c++) box_add(R.box, R.roll.c[c].in), box_add(R.box, R.roll.c[c].out);
  }
}

// Bottom to top; the Weather screen's paged parts — page by page, as HaloWatch draws its pages (at the place of the
// first of them in the list)
static void screen_draw(int s) {
  if (s < 0 || s >= face_scr_n) return;
  const FaceScreen &S = face_scr[s];
  if (!S.n) {
    if (s > 0 && S.name) halo_item_draw(S.name_it);
    return;
  }
  bool paged = false;
  for (int i = 0; i < S.n; i++) {
    const FaceLayer &L = S.layers[i];
    const FaceRun &R = S.run[i];
    if (L.kind == FL_EDGE) continue;  // (the button at the bottom edge — over the list, after everything)
    if (L.kind == FL_STARS) {
#if HAS_STARS
      stars_draw(S, i);
#endif
    } else if (fl_pixels(L.kind)) {
#if HAS_PIXELS
      pixel_draw(S, i);
#endif
    } else if (fl_gallery(L.kind)) {
      gal_draw(S, i);
    } else if (fl_section(L.kind)) {
      if (!L.hidden) settings_draw(L.kind);
    } else if (fl_steps(L.kind)) {
#if HAS_STEPSCR
      ss_draw(S, i);
#endif
    } else if (fl_widget(L.kind)) {
      wg_draw(S, i);
    } else if (fl_paged(L.kind)) {
      if (paged) continue;
      paged = true;
      for (int p = 0; p < wx_pages; p++)
        for (int j = i; j < S.n; j++)
          if (fl_paged(S.layers[j].kind)) wx_draw_page(S, j, p);
    } else if (fl_wx(L.kind)) {
      wx_draw_static(S, i);
#if HAS_STEPS
    } else if (L.kind == FL_STEPS) {
      halo_item_draw(R.pill), halo_item_draw(R.icon_out), halo_item_draw(R.icon_in);
      halo_roll_draw(R.roll);
#endif
    } else if (L.kind != FL_WEATHER) {
      halo_roll_draw(R.roll);
    } else {
      halo_item_draw(R.pill), halo_item_draw(R.icon_out), halo_item_draw(R.icon_in);
      halo_roll_draw(R.roll);
    }
  }
  for (int i = 0; i < S.n; i++)
    if (S.layers[i].kind == FL_EDGE) gal_draw(S, i);
}

static bool screen_paged(const FaceScreen &S) {
  for (int i = 0; i < S.n; i++)
    if (fl_paged(S.layers[i].kind)) return true;
  return false;
}
static bool screen_steps(const FaceScreen &S) {  // a screen with the Steps screen's parts
  for (int i = 0; i < S.n; i++)
    if (fl_steps(S.layers[i].kind)) return true;
  return false;
}
// How many pages a screen swipes through sideways: the Weather screen's — a city each; the Steps screen's — two
static int screen_pages(const FaceScreen &S) {
  return screen_paged(S) ? wx_pages : screen_steps(S) ? 2 : 1;
}

// ---------- The face and the screen over it (lib/Halo's navigation, as on HaloWatch) ----------
static void face_layout(const HaloView &v) {
  screen_layout(0, v);
}
static void face_draw() {
  screen_draw(0);
}
// Opened: the values in place (no rolling), the first page — as HaloWatch's weather_enter; the list from its top — as
// menu_enter
static void screen_enter(int s) {
  if (s < 0 || s >= face_scr_n) return;
  FaceScreen &S = face_scr[s];
  if (screen_paged(S)) wx_fill();
  halo_spring_set(S.pager, 0);
  S.drag = false;
  gal_enter(S);
  settings_enter(S);
  if (screen_steps(S)) ss_enter();
}
// Still and on top: paging, if it has pages; the list, if it has one (depth — in lib/Halo's stack, -1 — over the face)
static void screen_update(int s, float dt, int depth) {
  if (s < 0 || s >= face_scr_n) return;
  FaceScreen &S = face_scr[s];
  if (screen_paged(S) || screen_pages(S) > 1) wx_pager_update(S, dt, screen_pages(S));
  gal_update(S, dt, depth);
  settings_update(S, dt);
}

// The screen open right over the face (a layer of the face with a link)
static void over_layout(const HaloView &v) {
  screen_layout(face_over, v);
}
static void over_draw() {
  screen_draw(face_over);
}
static void over_enter() {
  screen_enter(face_over);
}
static void over_update(float dt) {
  screen_update(face_over, dt, -1);
}
static const HaloScreen OVER = {over_enter, over_update, over_layout, over_draw};

// Where a layer is for a tap and for the transition: the pill — as HaloWatch has it (its center, its width now, its
// height); text — the box it was drawn in
static bool layer_rect(const FaceScreen &S, int i, float &x, float &y, float &w, float &h) {
  const FaceLayer &L = S.layers[i];
  const FaceRun &R = S.run[i];
  if (L.kind == FL_WEATHER) {
    x = face_base_x(L) + L.dx, y = face_base_y(L) + L.dy, w = pill_width(R), h = PILL_H * L.k;
    return true;
  }
#if HAS_STEPS
  if (L.kind == FL_STEPS) {
    x = face_base_x(L) + L.dx, y = face_base_y(L) + L.dy, w = steps_width(L, R), h = PILL_H * L.k;
    return true;
  }
#endif
  const float *b = R.box;
  if (b[2] < b[0]) return false;
  x = (b[0] + b[2]) / 2, y = (b[1] + b[3]) / 2, w = b[2] - b[0], h = b[3] - b[1];
  return true;
}

// Only while the face is on screen and still: a layer with a link opens its screen over the face (the top one under the
// finger); a tap anywhere else does nothing
static void face_update(float) {
  const HaloFinger &f = halo_finger;
  if (!f.tap || f.owner) return;
  const FaceScreen &S = face_scr[0];
  for (int i = S.n - 1; i >= 0; i--) {
    const FaceLayer &L = S.layers[i];
    float x, y, w, h;
    if (L.hidden || L.tap <= 0 || L.tap >= face_scr_n || L.tap == nav_scr[0] || !layer_rect(S, i, x, y, w, h)) continue;
    if (fabsf(f.x - x) < w / 2 + 10 && fabsf(f.y - y) < h / 2 + 10) {
      face_over = L.tap;
      halo_nav_face_open(OVER, L.fx, x, y, w, h);
      return;
    }
  }
}

static const HaloScreen FACE = {nullptr, face_update, face_layout, face_draw};

// ---------- The gallery and the screens opened on it (lib/Halo's main screen and its stack, as on HaloWatch) ----------
// Each level of the stack shows one of the screens — or a stub (a card without a link): its title in the middle, as
// HaloWatch's Alarm and More
static const char *nav_stub[NAV_LEVELS];
static float stub_mid[NAV_LEVELS];
static HaloItem stub_it[NAV_LEVELS];

static void level_enter(int d) {
  if (nav_scr[d] >= 0) return screen_enter(nav_scr[d]);
  stub_mid[d] = halo_text_mid(gl_title, gl_str(nav_stub[d]));
}
static void level_update(int d, float dt) {
  if (nav_scr[d] >= 0) screen_update(nav_scr[d], dt, d);
}
static void level_layout(int d, const HaloView &v) {
  if (nav_scr[d] >= 0) return screen_layout(nav_scr[d], v);
  gl_text_mid(stub_it[d], gl_title, gl_str(nav_stub[d]), stub_mid[d], CX, CX, 1, GL_TEXT, 1, &halo_black);
  stub_it[d].on_black = true;
  halo_view_commit(stub_it[d], v);
}
static void level_draw(int d) {
  if (nav_scr[d] >= 0) screen_draw(nav_scr[d]);
  else halo_item_draw(stub_it[d]);
}
template <int D> static void lv_enter() { level_enter(D); }
template <int D> static void lv_update(float dt) { level_update(D, dt); }
template <int D> static void lv_layout(const HaloView &v) { level_layout(D, v); }
template <int D> static void lv_draw() { level_draw(D); }
static HaloScreen level(int d, uint8_t home) {
  static const HaloScreen L[NAV_LEVELS] = {
      {lv_enter<0>, lv_update<0>, lv_layout<0>, lv_draw<0>}, {lv_enter<1>, lv_update<1>, lv_layout<1>, lv_draw<1>},
      {lv_enter<2>, lv_update<2>, lv_layout<2>, lv_draw<2>}, {lv_enter<3>, lv_update<3>, lv_layout<3>, lv_draw<3>},
      {lv_enter<4>, lv_update<4>, lv_layout<4>, lv_draw<4>}};
  HaloScreen s = L[d];
  s.home = home;
  return s;
}

// The gallery — lib/Halo's main screen (none — the face is alone: a swipe up on it leads nowhere)
static void gallery_begin() {
  if (nav_scr[0] > 0 && nav_scr[0] < face_scr_n) halo_nav_begin(level(0, 0));  // to the face — down from the top edge
}

// A tap on a card or on the button (gal_update, the frame before): open its screen over the one it is on — as HaloWatch
// opens a card's screen (open_req: from the gallery only once it is in place). A screen already in the stack is not
// opened twice; the face and the gallery are not opened by a card
static void gallery_tick() {
  const FaceLayer *L = gl_req;
  gl_req = nullptr;
  if (!L) return;
  const int d = halo_nav_depth();
  if (gl_req_depth == 0 ? !halo_nav_at_home() : d != gl_req_depth) return;
  if (d + 1 >= NAV_LEVELS) return;
  const int to = L->tap > 0 && L->tap < face_scr_n ? L->tap : -1;
  for (int k = 0; k <= d; k++)
    if (to >= 0 && nav_scr[k] == to) return;
  nav_scr[d + 1] = to, nav_stub[d + 1] = L->text;
  // The gesture to the face from it: one that pages — up from the bottom edge (as Weather), others — both (as Settings)
  halo_nav_open(level(d + 1, to >= 0 && (screen_paged(face_scr[to]) || screen_pages(face_scr[to]) > 1) ? HALO_HOME_UP
                                                                                                    : HALO_HOME_BOTH));
}
