// wxfetch.h — fresh weather for the board (only with Wi-Fi on, FN_WIFI): every 30 minutes HaloClock wakes Wi-Fi, syncs
// the time and calls wx_fetch — one request to Open-Meteo for all the cities (plain HTTP: no TLS memory; free, no key),
// the same fields studio.py asks for the editor; the numbers reach the screens in the next frame (wx_poll, in update)
// and roll there. Without Wi-Fi the weather stays as it was at Flash. The command ntp — now.
#pragma once

#if FN_WIFI && !defined(__EMSCRIPTEN__)
#include <HaloHttp.h>

static WxCity wx_got[WX_MAX];  // the fetch's (the clock task), taken over by wx_poll
static volatile bool wx_fresh;
static portMUX_TYPE wx_mux = portMUX_INITIALIZER_UNLOCKED;

static inline int16_t wx_round(JsonVariantConst v) {
  return v.is<float>() ? (int16_t)lroundf(v.as<float>()) : (int16_t)WX_NONE;
}

static void wx_fetch() {
  static char path[700];
  const int n = wx_pages;
  int k = snprintf(path, sizeof(path), "/v1/forecast?latitude=");
  for (int i = 0; i < n; i++) k += snprintf(path + k, sizeof(path) - k, "%s%.4f", i ? "," : "", wx_city[i].lat);
  k += snprintf(path + k, sizeof(path) - k, "&longitude=");
  for (int i = 0; i < n; i++) k += snprintf(path + k, sizeof(path) - k, "%s%.4f", i ? "," : "", wx_city[i].lon);
  snprintf(path + k, sizeof(path) - k,
           "&current=temperature_2m,apparent_temperature,weather_code&hourly=weather_code,temperature_2m"
           "&daily=temperature_2m_max,temperature_2m_min&timezone=auto&forecast_days=2&forecast_hours=%d"
           "&timeformat=unixtime%s",
           WX_AHEAD, WXD_F ? "&temperature_unit=fahrenheit" : "");
  // Only what the screens use (one city — an object, several — an array of them)
  JsonDocument filter, doc(HaloPsram::instance());
  JsonObject f = n > 1 ? filter.add<JsonObject>() : filter.to<JsonObject>();
  f["current"]["temperature_2m"] = f["current"]["apparent_temperature"] = f["current"]["weather_code"] = true;
  f["hourly"]["time"] = f["hourly"]["weather_code"] = f["hourly"]["temperature_2m"] = true;
  f["daily"]["temperature_2m_max"] = f["daily"]["temperature_2m_min"] = true;
  if (!halo_http_get_json("api.open-meteo.com", path, doc, &filter)) return;
  for (int i = 0; i < n; i++) {
    const JsonVariantConst e = n > 1 ? doc[i].as<JsonVariantConst>() : doc.as<JsonVariantConst>();
    WxCity &w = wx_got[i];
    w = wx_city[i];
    w.now = wx_round(e["current"]["temperature_2m"]), w.feels = wx_round(e["current"]["apparent_temperature"]);
    w.hi = wx_round(e["daily"]["temperature_2m_max"][0]), w.lo = wx_round(e["daily"]["temperature_2m_min"][0]);
    w.kind = wx_kind_of(e["current"]["weather_code"] | 3);
    w.t0 = e["hourly"]["time"][0] | 0u;
    for (int j = 0; j < WX_AHEAD; j++) {
      w.hk[j] = wx_kind_of(e["hourly"]["weather_code"][j] | 3);
      const JsonVariantConst ht = e["hourly"]["temperature_2m"][j];
      w.ht[j] = ht.isNull() ? WX_NONE : wx_round(ht);
    }
  }
  portENTER_CRITICAL(&wx_mux);
  wx_fresh = true;
  portEXIT_CRITICAL(&wx_mux);
  LOG("weather: %d cities, %s %d (feels %d), %d..%d\n", n, wx_got[0].name, wx_got[0].now, wx_got[0].feels, wx_got[0].lo,
      wx_got[0].hi);
}

// New weather → the screens (in update, the frame task: the layers are laid out there)
static void wx_poll() {
  if (!wx_fresh) return;
  portENTER_CRITICAL(&wx_mux);
  memcpy(wx_city, wx_got, sizeof(WxCity) * wx_pages);
  wx_fresh = false;
  portEXIT_CRITICAL(&wx_mux);
  if (wx_demo < 0) face_weather_show(true);
}

static void wx_begin() {
  halo_clock_online(wx_fetch, 30 * 60);
}
#else
static inline void wx_poll() {}
static inline void wx_begin() {}
#endif
