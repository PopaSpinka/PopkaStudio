// weather.h — the weather the watch shows. The cities are picked in the editor (the Weather screen's pages, one each;
// the first — the watch face's pill too); wxdata.h, written by studio.py, has them with the weather the editor had at
// Flash. Then, with Wi-Fi on, the board asks Open-Meteo itself every 30 minutes (wxfetch.h) — the same numbers the
// editor shows. HaloWatch's four fictional days stay for the commands demo and wx N (everything rolls on cue).

#define WX_MAX 6     // cities at most (the editor: state.js WX_MAX)
#define WX_HOURS 3   // the hourly pill: this hour and the next two
#define WX_AHEAD 24  // hours of forecast kept: the pill picks the ones from this hour on
#define WX_NONE INT16_MIN  // no number yet: "--"

enum : uint8_t { WX_PARTLY, WX_SUN, WX_CLOUD, WX_RAIN };
// What the screens show of a city now
struct WxDay {
  int16_t now, hi, lo;
  uint8_t kind;
  uint8_t hour_kind[WX_HOURS];  // this hour and the next two
  int16_t feels;                // what it feels like
};
// A city and its weather as Open-Meteo gave it: now, feels like, today's high and low (in the city's own day), the sky,
// and the sky of the next WX_AHEAD hours from t0
struct WxCity {
  char name[24];
  float lat, lon;
  int16_t now, feels, hi, lo;
  uint8_t kind;
  uint32_t t0;  // the first forecast hour (unix time; 0 — no forecast)
  uint8_t hk[WX_AHEAD];
  int16_t ht[WX_AHEAD];  // the temperature of each of those hours (WX_NONE — not known; the face's Forecast widget)
};

// HaloWatch's fictional days (demo, wx N)
static const WxDay WX_DEMO[] = {
    {19, 21, 12, WX_PARTLY, {WX_PARTLY, WX_SUN, WX_PARTLY}, 18},
    {23, 25, 14, WX_SUN, {WX_SUN, WX_SUN, WX_PARTLY}, 25},
    {16, 17, 11, WX_CLOUD, {WX_CLOUD, WX_PARTLY, WX_CLOUD}, 14},
    {12, 13, 9, WX_RAIN, {WX_RAIN, WX_CLOUD, WX_RAIN}, 9},
};
#define WX_DEMO_N (int)(sizeof(WX_DEMO) / sizeof(WX_DEMO[0]))

// Open-Meteo's sky (WMO code) as one of the four icons
static inline uint8_t wx_kind_of(int wmo) {
  if (wmo <= 1) return WX_SUN;
  if (wmo == 2) return WX_PARTLY;
  if (wmo == 3 || wmo == 45 || wmo == 48 || (wmo >= 71 && wmo <= 77) || wmo == 85 || wmo == 86) return WX_CLOUD;  // (snow too)
  return WX_RAIN;  // drizzle, rain, showers, thunder
}

#include "wxdata.h"  // WXD_N, WXD_F and wx_city[] — the editor's cities and their weather at Flash

static int wx_pages = WXD_N;  // cities = pages
static int wx_demo = -1;      // a fictional day on show (demo, wx N); -1 — the real weather

// What page p shows now (a demo: page 0 the demo day, the others the next ones)
static WxDay wx_day(int page) {
  page = page < 0 ? 0 : page >= wx_pages ? wx_pages - 1 : page;
  if (wx_demo >= 0) return WX_DEMO[(wx_demo + page) % WX_DEMO_N];
  const WxCity &c = wx_city[page];
  WxDay d = {c.now, c.hi, c.lo, c.kind, {c.kind, c.kind, c.kind}, c.feels};
  if (!c.t0) return d;
  int k = 0;
  if (time(nullptr) > (time_t)c.t0) k = (int)((time(nullptr) - (time_t)c.t0) / 3600);
  for (int j = 0; j < WX_HOURS; j++) d.hour_kind[j] = c.hk[k + j < WX_AHEAD ? k + j : WX_AHEAD - 1];
  return d;
}

// "21°", "--°" (nothing yet); a number alone: "21", "--"
static inline void wx_deg(char *s, size_t n, int16_t v) {
  if (v == WX_NONE) snprintf(s, n, "--°");
  else snprintf(s, n, "%d°", v);
}
static inline void wx_num(char *s, size_t n, int16_t v) {
  if (v == WX_NONE) snprintf(s, n, "--");
  else snprintf(s, n, "%d", v);
}
