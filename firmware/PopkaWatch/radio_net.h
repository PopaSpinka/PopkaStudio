// radio_net.h — the Radio's sound from the internet (radio.h): on the board only, with Wi-Fi (FN_WIFI). What
// experiments/RadioLab measured, made to last:
//
//   Wi-Fi → rn_net (core 0, stack in PSRAM): HTTP/1.0 with Icy-MetaData: 1, redirects followed → a ring of 128 KB in
//   PSRAM (8 s of 128 kbps); the song's title (StreamTitle) cut out of the stream every icy-metaint bytes
//   → rn_dec (core 0, stack in PSRAM): the Helix MP3 decoder (in Arduino-ESP32 3.3.11 itself: mp3dec.h) → mono →
//   22 050 Hz (the speaker's, the Music's: a 2-tap average and linear steps) → a PCM ring in PSRAM
//   → the speaker's fill (sound.h: snd_radio, before the music).
//
// ~17 % of core 0 for 128 kbps; internal memory: ~3 KB for the decoder, Wi-Fi's own. HTTPS costs ~47 KB of internal
// memory, so an https address is first tried as http (most stream servers serve both); HTTPS only with RN_TLS_MIN free
// (no certificate check: a radio stream carries nothing secret). Wi-Fi: the radio holds it (hclk_hold — the clock's
// task leaves it on) from the first play or Browse fetch until RN_IDLE_OFF_MS after the last; a router that refuses a
// quick rejoin (208, "come back later") gets a fresh begin. A dropped stream reconnects while the ring plays on.
//
// The catalog (Browse): Radio Browser's JSON over plain HTTP (all.api.radio-browser.info: free, no key, public
// domain), up to RD_RES stations of a folder, MP3 only, the most listened first — into rn_res (PSRAM).
// Commands: radio — how it goes; radio URL — play that address (a test).
// (the sketch's one-letter macros — DP of gallery.h, W — are names inside mbedTLS's headers: hidden meanwhile)
#pragma push_macro("DP")
#pragma push_macro("W")
#pragma push_macro("H")
#undef DP
#undef W
#undef H
#include <NetworkClientSecure.h>
#include <lwip/netdb.h>
#include <freertos/idf_additions.h>
#include <HaloHttp.h>
#include "mp3dec.h"
#pragma pop_macro("H")
#pragma pop_macro("W")
#pragma pop_macro("DP")

#define RN_RATE 22050           // the speaker's rate: every stream is resampled to it
#define RN_RING (128 * 1024)    // the compressed stream (PSRAM): 8 s of 128 kbps; a power of two
#define RN_START (32 * 1024)    // play (and go on after a gap) with 2 s of 128 kbps in the ring
#define RN_PCM 16384            // decoded samples (PSRAM): 0.74 s; a power of two
#define RN_INB 4096             // the decoder's input window
#define RN_TLS_MIN (100 * 1024) // HTTPS only with this much internal memory free (it takes ~47 KB, RadioLab)
#define RN_IDLE_OFF_MS 30000    // nothing played or fetched for this long — Wi-Fi off
#define RN_HOST "all.api.radio-browser.info"

enum { RN_OFF, RN_JOINING, RN_CONNECTING, RN_STREAMING, RN_NOWIFI, RN_FAILED };
static volatile int rn_net = RN_OFF;   // the network's side
static volatile bool rn_playing;       // sound comes out (the ring had RN_START once and is not dry)
static volatile bool rn_on;            // should play (the play button)
static char rn_url[RD_URL];            // the station asked for (rn_mux)
static volatile uint32_t rn_gen;       // +1 — a new station (or play again after a pause)
static char rn_title[RD_TITLE];        // the song now (rn_mux); rn_title_gen +1 when it changes
static volatile uint32_t rn_title_gen;
static volatile int rn_kbps, rn_sr;
static volatile float rn_level;        // the sound's peak now, 0..1
static portMUX_TYPE rn_mux = portMUX_INITIALIZER_UNLOCKED;
// Browse: a fetch of a folder (path and query of the catalog's search) → rn_res
static char rn_fetch_path[300];
static volatile bool rn_fetch_want;
static volatile int rn_fetch_st;       // 0 — nothing, 1 — loading, 2 — done, 3 — failed
static volatile uint32_t rn_fetch_gen; // +1 a fetch done (or failed)
static RdSt *rn_res;                   // RD_RES (PSRAM)
static volatile int rn_res_n;

static uint8_t *rn_ring, *rn_inb;
static int16_t *rn_pcm;
static volatile uint32_t rn_w, rn_r, rn_pw, rn_pr;  // free-running counters: used = w − r
static volatile bool rn_flush, rn_pcm_drop;
static volatile uint32_t rn_st_in, rn_st_frames, rn_st_us, rn_st_gaps, rn_st_errs, rn_st_reconnects;
static TaskHandle_t rn_net_task, rn_dec_task;
static char rn_click_uuid[40];         // a station of the catalog started: its click (the screens set it)
static NetworkClient rn_plain;
static NetworkClientSecure *rn_tls;
static Client *rn_c;
static bool rn_wifi_held;
static int rn_metaint;

static inline uint32_t rn_used() { return rn_w - rn_r; }
static inline uint32_t rn_pused() { return rn_pw - rn_pr; }
static inline uint32_t rn_free_int() { return heap_caps_get_free_size(MALLOC_CAP_INTERNAL); }
static void rn_wake() {
  if (rn_net_task) xTaskNotifyGive(rn_net_task);
}

// ---------- What the screens call (the frame task) ----------
static void rn_play(const char *url) {
  portENTER_CRITICAL(&rn_mux);
  strlcpy(rn_url, url, sizeof(rn_url));
  rn_gen++;
  rn_title[0] = 0, rn_title_gen++;
  portEXIT_CRITICAL(&rn_mux);
  rn_on = true;
  rn_wake();
}
static void rn_pause() {
  rn_on = false;
  rn_wake();
}
static void rn_fetch(const char *path) {
  strlcpy(rn_fetch_path, path, sizeof(rn_fetch_path));
  rn_fetch_st = 1, rn_fetch_want = true;
  rn_wake();
}
static void rn_title_copy(char *out, size_t n) {
  portENTER_CRITICAL(&rn_mux);
  strlcpy(out, rn_title, n);
  portEXIT_CRITICAL(&rn_mux);
}

// ---------- Wi-Fi: held while the radio needs it ----------
static bool rn_wifi_join() {
  if (!rn_wifi_held) rn_wifi_held = true, hclk_hold = true, halo_stay_awake(true);
  if (WiFi.status() == WL_CONNECTED) return true;
  rn_net = RN_JOINING;
  for (int attempt = 0; attempt < 3 && (rn_on || rn_fetch_want); attempt++) {
    if (attempt) WiFi.disconnect(), vTaskDelay(pdMS_TO_TICKS(500));  // (a refused rejoin needs a fresh begin)
    WiFi.mode(WIFI_STA);
    WiFi.begin(hclk_ssid, hclk_pass);
    for (int i = 0; i < 40 && (rn_on || rn_fetch_want); i++) {  // 10 s
      if (WiFi.status() == WL_CONNECTED) return LOG("radio: wifi joined, %d dBm\n", (int)WiFi.RSSI()), true;
      vTaskDelay(pdMS_TO_TICKS(250));
    }
  }
  return WiFi.status() == WL_CONNECTED;
}
static void rn_wifi_release() {
  rn_wifi_held = false, hclk_hold = false;
  if (!hclk_busy) WiFi.disconnect(true), WiFi.mode(WIFI_OFF);  // (the clock syncing: it turns Wi-Fi off itself)
  halo_stay_awake(false);
  LOG("radio: wifi off, free internal %u KB\n", (unsigned)(rn_free_int() / 1024));
}

// ---------- One connection: the address, redirects, the headers ----------
struct RnUrl {
  bool tls;
  char host[96];
  int port;
  char path[RD_URL];
};
static bool rn_parse(const char *u, RnUrl &o) {
  o.tls = !strncmp(u, "https://", 8);
  if (!o.tls && strncmp(u, "http://", 7)) return false;
  const char *h = u + (o.tls ? 8 : 7), *p = strpbrk(h, "/?");
  const size_t hl = p ? (size_t)(p - h) : strlen(h);
  if (!hl || hl >= sizeof(o.host)) return false;
  memcpy(o.host, h, hl), o.host[hl] = 0;
  o.port = o.tls ? 443 : 80;
  char *colon = strchr(o.host, ':');
  if (colon) *colon = 0, o.port = atoi(colon + 1);
  o.path[0] = 0;
  if (p && *p == '?') strcpy(o.path, "/");
  strlcat(o.path, p ? p : "/", sizeof(o.path));
  return true;
}
static bool rn_gone(uint32_t gen) {  // the station changed or paused: stop what is going on
  return !rn_on || rn_gen != gen;
}
static int rn_line(char *s, int max, uint32_t ms, uint32_t gen) {  // a header line without \r\n; −1 — none in time
  int n = 0;
  const uint32_t t0 = millis();
  while (millis() - t0 < ms && !rn_gone(gen)) {
    const int ch = rn_c->read();
    if (ch < 0) {
      if (!rn_c->connected()) break;
      vTaskDelay(2);
      continue;
    }
    if (ch == '\n') {
      if (n && s[n - 1] == '\r') n--;
      s[n] = 0;
      return n;
    }
    if (n < max - 1) s[n++] = ch;
  }
  s[n] = 0;
  return -1;
}
static bool rn_exact(uint8_t *b, int n, uint32_t ms, uint32_t gen) {
  const uint32_t t0 = millis();
  while (n > 0 && millis() - t0 < ms && !rn_gone(gen)) {
    const int k = rn_c->read(b, n);
    if (k > 0) b += k, n -= k;
    else if (!rn_c->connected()) return false;
    else vTaskDelay(3);
  }
  return n == 0;
}
static void rn_close() {
  if (rn_c) rn_c->stop();
  rn_c = nullptr;
}
// 1 — streaming; 0 — failed; −1 — https and no memory for it
static int rn_open(const char *url, int depth, uint32_t gen) {
  RnUrl u;
  if (!rn_parse(url, u)) return LOG("radio: not an http(s) address\n"), 0;
  struct addrinfo hints = {}, *res = nullptr;
  hints.ai_family = AF_INET, hints.ai_socktype = SOCK_STREAM;
  if (lwip_getaddrinfo(u.host, nullptr, &hints, &res) != 0 || !res) return LOG("radio: %s not found\n", u.host), 0;
  const IPAddress ip(((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr);
  lwip_freeaddrinfo(res);
  if (u.tls) {
    if (rn_free_int() < RN_TLS_MIN) return LOG("radio: https needs memory (%u KB free)\n", (unsigned)(rn_free_int() / 1024)), -1;
    if (!rn_tls) rn_tls = new NetworkClientSecure;
    rn_tls->setInsecure();
    rn_tls->setHandshakeTimeout(10);
    if (!rn_tls->connect(ip, u.port, u.host, nullptr, nullptr, nullptr)) return LOG("radio: https to %s failed\n", u.host), 0;
    rn_c = rn_tls;
  } else {
    if (!rn_plain.connect(ip, u.port, 5000)) return LOG("radio: can't connect to %s\n", u.host), 0;
    rn_c = &rn_plain;
  }
  char req[RD_URL + 200];
  const int n = snprintf(req, sizeof(req),
                         "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: PopkaStudio/" RD_AGENT "\r\nIcy-MetaData: 1\r\n"
                         "Accept: */*\r\nConnection: close\r\n\r\n", u.path, u.host);
  rn_c->write((const uint8_t *)req, n);
  char line[RD_URL + 16], loc[RD_URL] = "", type[48] = "";
  if (rn_line(line, sizeof(line), 8000, gen) < 0) return LOG("radio: no answer from %s\n", u.host), rn_close(), 0;
  const char *sp = strchr(line, ' ');
  const int code = sp ? atoi(sp + 1) : 0;  // "HTTP/1.1 200 OK" or "ICY 200 OK"
  int metaint = 0, br = 0;
  while (rn_line(line, sizeof(line), 5000, gen) > 0) {
    if (!strncasecmp(line, "location:", 9)) strlcpy(loc, line + 9 + strspn(line + 9, " "), sizeof(loc));
    else if (!strncasecmp(line, "icy-metaint:", 12)) metaint = atoi(line + 12);
    else if (!strncasecmp(line, "icy-br:", 7)) br = atoi(line + 7);
    else if (!strncasecmp(line, "content-type:", 13)) strlcpy(type, line + 13 + strspn(line + 13, " "), sizeof(type));
  }
  if (code >= 300 && code < 400 && *loc && depth < 4) {
    rn_close();
    if (!strncmp(loc, "/", 1)) {  // (a path on the same server)
      char abs[RD_URL];
      snprintf(abs, sizeof(abs), "%s://%s:%d%s", u.tls ? "https" : "http", u.host, u.port, loc);
      strlcpy(loc, abs, sizeof(loc));
    }
    return rn_open(loc, depth + 1, gen);
  }
  if (code != 200) return LOG("radio: answer %d from %s\n", code, u.host), rn_close(), 0;
  if (*type && !strstr(type, "audio") && !strstr(type, "mpeg") && !strstr(type, "octet"))
    return LOG("radio: %s is not sound\n", type), rn_close(), 0;
  rn_metaint = metaint, rn_kbps = br;
  LOG("radio: %s %s, %s, %d kbps, title every %d bytes\n", u.tls ? "https" : "http", u.host, type, br, metaint);
  return 1;
}
// An https address: the same one over http first (most stream servers serve both), https only if that fails
static int rn_open_any(const char *url, uint32_t gen) {
  if (!strncmp(url, "https://", 8)) {
    char http[RD_URL];
    snprintf(http, sizeof(http), "http://%s", url + 8);
    char *colon = strchr(http + 7, ':'), *slash = strchr(http + 7, '/');
    if (colon && (!slash || colon < slash) && !strncmp(colon, ":443", 4)) memmove(colon, colon + 4, strlen(colon + 4) + 1);
    if (rn_open(http, 0, gen) == 1) return 1;
    if (rn_gone(gen)) return 0;
  }
  return rn_open(url, 0, gen);
}

// ---------- The ring, the title ----------
static void rn_put(const uint8_t *b, int n) {
  const uint32_t w = rn_w, at = w & (RN_RING - 1), a = min((uint32_t)n, (uint32_t)RN_RING - at);
  memcpy(rn_ring + at, b, a);
  memcpy(rn_ring, b + a, n - a);
  __atomic_store_n(&rn_w, w + n, __ATOMIC_RELEASE);
}
// ICY titles come in UTF-8 or Latin-1: not valid UTF-8 — read as Latin-1
static void rn_to_utf8(const char *s, int n, char *out, int cap) {
  bool utf8 = true;
  for (int i = 0; i < n && utf8;) {
    const uint8_t c = s[i];
    const int k = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 0;
    if (!k || i + k > n) utf8 = false;
    for (int j = 1; utf8 && j < k; j++) utf8 = ((uint8_t)s[i + j] >> 6) == 2;
    i += k ? k : 1;
  }
  int o = 0;
  for (int i = 0; i < n && o < cap - 3; i++) {
    const uint8_t c = s[i];
    if (utf8 || c < 0x80) out[o++] = c;
    else out[o++] = 0xC0 | c >> 6, out[o++] = 0x80 | (c & 0x3F);
  }
  out[o] = 0;
}
static void rn_meta(const char *m) {  // StreamTitle='Artist - Title';StreamUrl='…';
  const char *t = strstr(m, "StreamTitle='");
  if (!t) return;
  t += 13;
  const char *e = strstr(t, "';");
  char s[RD_TITLE];
  rn_to_utf8(t, e ? e - t : strlen(t), s, sizeof(s));
  portENTER_CRITICAL(&rn_mux);
  const bool changed = strcmp(s, rn_title) != 0;
  if (changed) strlcpy(rn_title, s, sizeof(rn_title)), rn_title_gen++;
  portEXIT_CRITICAL(&rn_mux);
  if (changed) LOG("radio: now «%s»\n", s);
}
static bool rn_stream(uint32_t gen) {  // false — the stream ended (reconnect)
  static uint8_t buf[2048], meta[4081];
  int metaleft = rn_metaint;
  uint32_t last = millis();
  while (!rn_gone(gen)) {
    if (rn_fetch_want) return true;  // (a Browse fetch first: the ring plays meanwhile)
    if (RN_RING - rn_used() < sizeof(buf)) { vTaskDelay(pdMS_TO_TICKS(20)), last = millis(); continue; }
    int want = sizeof(buf);
    if (rn_metaint) want = min(want, metaleft);
    const int n = rn_c->read(buf, want);
    if (n <= 0) {
      if (!rn_c->connected() || millis() - last > 8000) return false;  // closed or silent for 8 s
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }
    last = millis();
    rn_put(buf, n);
    rn_st_in += n;
    if (rn_metaint && (metaleft -= n) == 0) {
      uint8_t len = 0;
      if (!rn_exact(&len, 1, 5000, gen)) return false;
      if (len && !rn_exact(meta, len * 16, 5000, gen)) return false;
      meta[len * 16] = 0;
      if (len) rn_meta((const char *)meta);
      metaleft = rn_metaint;
    }
  }
  return true;
}

// ---------- Browse: a folder of the catalog ----------
static void rn_do_fetch() {
  JsonDocument filter, doc(HaloPsram::instance());
  JsonObject f = filter.add<JsonObject>();
  f["name"] = f["url_resolved"] = f["bitrate"] = f["countrycode"] = f["tags"] = f["stationuuid"] = f["hls"] = true;
  halo_http_agent = "PopkaStudio/" RD_AGENT;
  const uint32_t t0 = millis();
  const bool ok = halo_http_get_json(RN_HOST, rn_fetch_path, doc, &filter);
  int n = 0;
  if (ok)
    for (JsonObjectConst s : doc.as<JsonArrayConst>()) {
      if (n >= RD_RES) break;
      const char *url = s["url_resolved"] | "";
      if ((s["hls"] | 0) || strncmp(url, "http", 4) || strlen(url) >= RD_URL) continue;
      rd_station_make(rn_res[n], s["name"] | "", url, s["stationuuid"] | "", s["countrycode"] | "", s["tags"] | "",
                      s["bitrate"] | 0, 0);
      n++;
    }
  rn_res_n = n, rn_fetch_st = ok ? 2 : 3, rn_fetch_gen++;
  LOG("radio: %s in %u ms — %d stations\n", ok ? "the catalog answered" : "the catalog failed", (unsigned)(millis() - t0), n);
}
static void rn_click(const char *uuid) {  // the catalog's etiquette: a click when a station starts (best effort)
  if (!*uuid) return;
  char path[80];
  snprintf(path, sizeof(path), "/json/url/%s", uuid);
  JsonDocument filter, doc(HaloPsram::instance());
  filter["ok"] = true;
  halo_http_get_json(RN_HOST, path, doc, &filter);
}

// ---------- The network's task ----------
static void rn_net_loop(void *) {
  uint32_t idle_since = millis(), played = 0, backoff = 0;
  for (;;) {
    if (rn_fetch_want) {
      if (rn_wifi_join()) rn_do_fetch();
      else rn_res_n = 0, rn_fetch_st = 3, rn_fetch_gen++;
      rn_fetch_want = false, idle_since = millis();
      continue;
    }
    if (!rn_on) {
      rn_close();
      rn_net = RN_OFF;
      if (rn_wifi_held && millis() - idle_since > RN_IDLE_OFF_MS) rn_wifi_release();
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));
      continue;
    }
    idle_since = millis();
    char url[RD_URL];
    portENTER_CRITICAL(&rn_mux);
    const uint32_t gen = rn_gen;
    strlcpy(url, rn_url, sizeof(url));
    portEXIT_CRITICAL(&rn_mux);
    if (gen != played) {  // a new station: what is in the rings is the old one
      rn_close();
      rn_flush = true;
      for (int i = 0; i < 40 && rn_flush; i++) vTaskDelay(pdMS_TO_TICKS(5));
      played = gen, backoff = 0, rn_kbps = 0, rn_sr = 0;
    }
    if (!rn_wifi_join()) {
      rn_net = RN_NOWIFI;
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(3000));
      continue;
    }
    if (!rn_c) {
      rn_net = RN_CONNECTING;
      const int r = rn_open_any(url, gen);
      if (r != 1) {
        if (rn_gone(gen)) continue;
        rn_net = RN_FAILED;
        backoff = min(backoff ? backoff * 2 : (uint32_t)2000, (uint32_t)16000);
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(backoff));
        continue;
      }
      backoff = 0;
      if (*rn_click_uuid) {  // (a station from the catalog started: its click)
        char uuid[40];
        strlcpy(uuid, rn_click_uuid, sizeof(uuid)), rn_click_uuid[0] = 0;
        rn_click(uuid);
      }
    }
    rn_net = RN_STREAMING;
    if (!rn_stream(gen)) {
      rn_close();
      rn_st_reconnects++;
      LOG("radio: the stream stopped — again\n");
      vTaskDelay(pdMS_TO_TICKS(300));
    }
  }
}

// ---------- The decoder's task: MP3 frames → mono → 22 050 Hz → the PCM ring ----------
static int16_t rn_src[1153];  // [0] — the last sample of the frame before
static int rn_prev;
static uint32_t rn_pos;
static void rn_out(const int16_t *pcm, int ns, int ch, int sr) {
  const bool lp = sr > RN_RATE * 3 / 2;  // going down 2× or so: a 2-tap average first (a zero at the new Nyquist)
  for (int j = 0; j < ns; j++) {
    const int x = ch == 2 ? (pcm[2 * j] + pcm[2 * j + 1]) >> 1 : pcm[j];
    rn_src[1 + j] = lp ? (x + rn_prev) >> 1 : x;
    rn_prev = x;
  }
  const uint32_t step = ((uint32_t)sr << 16) / RN_RATE;
  uint32_t w = rn_pw;
  while ((rn_pos >> 16) < (uint32_t)ns) {
    const int i = rn_pos >> 16, f = (rn_pos & 0xFFFF) >> 1;
    rn_pcm[w++ & (RN_PCM - 1)] = rn_src[i] + (((rn_src[i + 1] - rn_src[i]) * f) >> 15);
    rn_pos += step;
  }
  __atomic_store_n(&rn_pw, w, __ATOMIC_RELEASE);
  rn_pos -= (uint32_t)ns << 16;
  rn_src[0] = rn_src[ns];
}
// ---------- The equalizer: the levels of RD_BANDS bands of what the speaker plays ----------
// Goertzel on the last samples played, every RN_VIS_HOP of them: band k (70 Hz … 9 kHz, log-spaced) is measured over
// its last N_k samples (a power of two, 32 … 512: the window ≈ the band's width — long for the bass, short for the
// highs), Hann-windowed (one table, read with a stride). Its level in dB to full scale, +3 dB an octave (music is
// quieter in the highs), against the loudest band of the last seconds → 0..1 in rn_band (the screen smooths it). In
// the speaker's task: ~4 200 multiply-adds 86 times a second
#define RN_VIS 512   // a power of two
#define RN_VIS_HOP 256
static volatile float rn_band[RD_BANDS];
static float *rn_vis, *rn_hann, rn_gc[RD_BANDS], rn_gdb[RD_BANDS];  // (the two windows: PSRAM)
static uint8_t rn_gsh[RD_BANDS];  // N_k = RN_VIS >> rn_gsh[k]
static int8_t rn_bdb[RD_BANDS];   // the bands now, dB (the radio command prints them)
static uint32_t rn_vis_w;         // the window is a ring: the newest sample at rn_vis_w − 1
static void rn_vis_init() {       // (rn_begin: once)
  rn_vis = (float *)heap_caps_calloc(RN_VIS * 2, sizeof(float), MALLOC_CAP_SPIRAM), rn_hann = rn_vis + RN_VIS;
  if (!rn_vis) return;
  for (int i = 0; i < RN_VIS; i++) rn_hann[i] = 0.5f - 0.5f * cosf(2 * HALO_PI * (i + 0.5f) / RN_VIS);
  for (int k = 0; k < RD_BANDS; k++) {
    const float f = 70 * powf(9000 / 70.0f, k / (RD_BANDS - 1.0f));
    int sh = 0;  // the shortest window still ≥ ~0.7 of rate / the band's width (≈ 0.247 f)
    while (sh < 4 && (float)(RN_VIS >> (sh + 1)) * f * 0.247f >= RN_RATE * 0.7f) sh++;
    rn_gsh[k] = sh;
    rn_gc[k] = 2 * cosf(2 * HALO_PI * f / RN_RATE);
    // power → dB to full scale: a sine of amplitude A gives (A·N·0.5 / 2)² (the window's gain 0.5); +3 dB an octave
    const float full = (RN_VIS >> sh) * 0.25f * 32768;
    rn_gdb[k] = -20 * log10f(full) + 3 * log2f(f / 1000);
  }
}
static inline float rn_log2(float x) {  // ≈ log2 (x > 0), ±0.01: the exponent and a parabola for the mantissa
  union {
    float f;
    uint32_t i;
  } u = {x};
  const float m = (u.i & 0x7FFFFF) * (1.0f / 8388608);
  return (int)((u.i >> 23) & 255) - 127 + m * (1.3466f - 0.3466f * m);
}
static float rn_vis_ref = -30;  // the loudest band of the last seconds, dB: a full ray (stations differ in loudness)
static void rn_vis_run() {
  float d[RD_BANDS], mx = -120;
  for (int k = 0; k < RD_BANDS; k++) {
    const int sh = rn_gsh[k], n = RN_VIS >> sh;
    const uint32_t o = rn_vis_w - n;
    const float c = rn_gc[k];
    float s1 = 0, s2 = 0;
    for (int i = 0; i < n; i++) {
      const float s = rn_vis[(o + i) & (RN_VIS - 1)] * rn_hann[i << sh] + c * s1 - s2;
      s2 = s1, s1 = s;
    }
    const float pw = s1 * s1 + s2 * s2 - c * s1 * s2;
    d[k] = pw > 1e-3f ? 3.0103f * rn_log2(pw) + rn_gdb[k] : -120;  // 10·log10 = 3.0103·log2
    rn_bdb[k] = d[k] < -127 ? -127 : (int8_t)d[k];
    if (d[k] > mx) mx = d[k];
  }
  // The reference: up at once to a louder band, down 2.6 dB a second; not below −45 dB (a quiet passage stays dim)
  rn_vis_ref = mx > rn_vis_ref ? mx : rn_vis_ref - 0.03f;
  if (rn_vis_ref < -45) rn_vis_ref = -45;
  for (int k = 0; k < RD_BANDS; k++) {  // 30 dB under the reference — nothing; squared: the loud ones stand out
    float x = (d[k] - rn_vis_ref + 30) * (1.0f / 30);
    x = x < 0 ? 0 : x > 1 ? 1 : x;
    rn_band[k] = x * x;
  }
}
static int rn_fill(int16_t *out, int n) {  // the speaker's task (sound.h)
  if (rn_pcm_drop) rn_pr = rn_pw, rn_pcm_drop = false;
  if (!rn_on || !rn_playing) {
    for (int b = 0; b < RD_BANDS; b++) rn_band[b] = 0;
    return rn_level = 0, 0;
  }
  const int k = min((uint32_t)n, rn_pused());
  int peak = 0;
  for (int i = 0; i < k; i++) {
    const int16_t v = rn_pcm[(rn_pr + i) & (RN_PCM - 1)];
    out[i] = v;
    if (abs((int)v) > peak) peak = abs((int)v);
  }
  for (int i = 0; i < k && rn_vis; i++) {
    rn_vis[rn_vis_w++ & (RN_VIS - 1)] = out[i];
    if (!(rn_vis_w & (RN_VIS_HOP - 1))) rn_vis_run();
  }
  __atomic_store_n(&rn_pr, rn_pr + k, __ATOMIC_RELEASE);
  rn_level = peak * (1.0f / 32768);
  return k;
}
static void rn_dec_loop(void *) {
  static int16_t pcm[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
  HMP3Decoder h = nullptr;
  uint8_t *in = rn_inb;
  int left = 0;
  for (;;) {
    if (rn_flush) {
      __atomic_store_n(&rn_r, rn_w, __ATOMIC_RELEASE);
      left = 0, in = rn_inb, rn_playing = false, rn_pcm_drop = true;
      rn_pos = 0, rn_prev = 0, rn_src[0] = 0;
      rn_flush = false;
    }
    if (!rn_on) {
      rn_playing = false;
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    if (!h) h = MP3InitDecoder();  // (3 KB internal, 20 KB PSRAM — kept from then on)
    if (!rn_playing) {
      if (rn_used() < RN_START) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }
      snd_begin(RN_RATE);  // (the speaker, once: sound.h)
      rn_playing = true;
    }
    if (RN_PCM - rn_pused() < 1400) { vTaskDelay(pdMS_TO_TICKS(8)); continue; }  // full enough
    if (in != rn_inb && left) memmove(rn_inb, in, left);
    in = rn_inb;
    const uint32_t take = min(rn_used(), (uint32_t)(RN_INB - left));
    for (uint32_t i = 0; i < take; i++) rn_inb[left + i] = rn_ring[(rn_r + i) & (RN_RING - 1)];
    __atomic_store_n(&rn_r, rn_r + take, __ATOMIC_RELEASE);
    left += take;
    if (left < 1000) {
      if (rn_pused() < 600) rn_playing = false, rn_st_gaps++, LOG("radio: the buffer ran dry\n");  // wait for 2 s again
      else vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    const int off = MP3FindSyncWord(in, left);
    if (off < 0) { left = 0; continue; }
    in += off, left -= off;
    const int64_t t0 = esp_timer_get_time();
    const int err = MP3Decode(h, &in, &left, pcm, 0);
    if (err == ERR_MP3_INDATA_UNDERFLOW || err == ERR_MP3_MAINDATA_UNDERFLOW) continue;  // more bytes / the reservoir
    if (err) { rn_st_errs++, in++, left--; continue; }
    rn_st_us += esp_timer_get_time() - t0, rn_st_frames++;
    MP3FrameInfo fi;
    MP3GetLastFrameInfo(h, &fi);
    if (fi.samprate != rn_sr) rn_sr = fi.samprate, LOG("radio: MP3 %d Hz, %d ch, %d kbps\n", fi.samprate, fi.nChans, fi.bitrate / 1000);
    if (!rn_kbps) rn_kbps = fi.bitrate / 1000;
    if (fi.nChans >= 1 && fi.nChans <= 2 && fi.samprate >= 8000) rn_out(pcm, fi.outputSamps / fi.nChans, fi.nChans, fi.samprate);
  }
}

static void rn_status() {
  static uint32_t in0, fr0, us0, ms0;
  static const char *const N[] = {"off", "joining", "connecting", "streaming", "no wifi", "failed"};
  const uint32_t ms = millis(), dt = max((uint32_t)1, ms - ms0), fr = rn_st_frames - fr0, us = rn_st_us - us0;
  char t[RD_TITLE];
  rn_title_copy(t, sizeof(t));
  LOG("radio: %s%s, in %u kbps, buffer %u KB, decode %.2f ms a frame, gaps %u, errors %u, reconnects %u, internal %u KB; %s\n",
      N[rn_net], rn_playing ? " playing" : "", (unsigned)((rn_st_in - in0) * 8 / dt), (unsigned)(rn_used() / 1024),
      fr ? us / 1000.0f / fr : 0, (unsigned)rn_st_gaps, (unsigned)rn_st_errs, (unsigned)rn_st_reconnects,
      (unsigned)(rn_free_int() / 1024), t);
  in0 = rn_st_in, fr0 = rn_st_frames, us0 = rn_st_us, ms0 = ms;
  char b[RD_BANDS * 5 + 1], *o = b;
  for (int k = 0; k < RD_BANDS; k++) o += sprintf(o, " %d", rn_bdb[k]);
  float top = 0;
  for (int k = 0; k < RD_BANDS; k++) top = fmaxf(top, rn_band[k]);
  LOG("radio: bands, dB (low → high):%s; full ray at %.1f dB, the longest now %.2f\n", b, rn_vis_ref, top);
}

static void rn_begin() {
  rn_ring = (uint8_t *)heap_caps_malloc(RN_RING, MALLOC_CAP_SPIRAM);
  rn_inb = (uint8_t *)heap_caps_malloc(RN_INB, MALLOC_CAP_SPIRAM);
  rn_pcm = (int16_t *)heap_caps_malloc(RN_PCM * 2, MALLOC_CAP_SPIRAM);
  rn_res = (RdSt *)heap_caps_calloc(RD_RES, sizeof(RdSt), MALLOC_CAP_SPIRAM);
  if (!rn_ring || !rn_inb || !rn_pcm || !rn_res) return LOG("radio: no PSRAM for the rings\n");
  rn_vis_init();
  snd_radio = rn_fill;
  // Both on core 0 (with Wi-Fi and the speaker), their stacks in PSRAM: internal memory is for Wi-Fi
  xTaskCreatePinnedToCoreWithCaps(rn_net_loop, "radio_net", 8192, nullptr, 3, &rn_net_task, 0, MALLOC_CAP_SPIRAM);
  xTaskCreatePinnedToCoreWithCaps(rn_dec_loop, "radio_dec", 8192, nullptr, 4, &rn_dec_task, 0, MALLOC_CAP_SPIRAM);
  halo_command("radio", [](const char *a) {
    while (*a == ' ') a++;
    if (!strncmp(a, "http", 4)) return rn_play(a);
    if (!strcmp(a, "stop")) return rn_pause();
    rn_status();
  });
}
