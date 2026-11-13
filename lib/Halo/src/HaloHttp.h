// HaloHttp — a JSON answer over plain HTTP for sketches on Halo (with HaloClock: Wi-Fi). The same way as
// net_http_get_json in AmoledNet: the address by lwip_getaddrinfo (thread-safe, unlike hostByName in core 3.3.11),
// HTTP/1.0 (the whole body, no chunks), not HTTPClient (it crashes the board in core 3.3.11); the JSON's memory in PSRAM
// (internal memory is tight next to the panel's buffers and Wi-Fi). Call it while Wi-Fi is up — from HaloClock's job:
//   halo_clock_online(fetch, 30 * 60);  // before halo_clock_begin
//   static void fetch() { JsonDocument doc(HaloPsram::instance()); if (halo_http_get_json(host, path, doc)) … }
#pragma once

#include <ArduinoJson.h>
#include <Halo.h>
#include <WiFi.h>
#include <lwip/netdb.h>

// Who asks (some services want a speaking name: the Radio — "PopkaStudio/2.0" for Radio Browser)
static const char *halo_http_agent = "Halo";

// ArduinoJson's memory in PSRAM
struct HaloPsram : ArduinoJson::Allocator {
  void *allocate(size_t n) override { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM); }
  void deallocate(void *p) override { heap_caps_free(p); }
  void *reallocate(void *p, size_t n) override { return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM); }
  static HaloPsram *instance() {
    static HaloPsram a;
    return &a;
  }
};

// GET http://host/path into doc (filter — keep only these fields); false — no answer (the log says why)
static bool halo_http_get_json(const char *host, const char *path, JsonDocument &doc, const JsonDocument *filter = nullptr) {
  struct addrinfo hints = {}, *res = nullptr;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if (lwip_getaddrinfo(host, nullptr, &hints, &res) != 0 || !res) {
    LOG("http: %s not found\n", host);
    return false;
  }
  IPAddress ip(((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr);
  lwip_freeaddrinfo(res);
  NetworkClient client;
  if (!client.connect(ip, 80, 5000)) {
    LOG("http: can't connect to %s\n", host);
    return false;
  }
  client.setTimeout(10000);
  client.printf("GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\nConnection: close\r\n\r\n", path, host, halo_http_agent);
  String status = client.readStringUntil('\n');  // "HTTP/1.1 200 OK"
  if (status.indexOf(" 200 ") < 0) {
    LOG("http: %s from %s\n", status.c_str(), host);
    client.stop();
    return false;
  }
  client.find("\r\n\r\n");  // the headers
  // The body into PSRAM first, in big reads: ArduinoJson reading the socket a letter at a time took seconds for a
  // few dozen KB (the Radio's catalog: 38 KB). HTTP/1.0 — the body ends where the connection closes
  size_t cap = 16384, n = 0;
  char *body = (char *)heap_caps_malloc(cap, MALLOC_CAP_SPIRAM);
  uint32_t last = millis();
  while (body && millis() - last < 10000) {
    if (n + 4096 > cap) {
      if (cap >= 512 * 1024) break;  // (more than any answer here: the rest is cut)
      char *more = (char *)heap_caps_realloc(body, cap * 2, MALLOC_CAP_SPIRAM);
      if (!more) break;
      body = more, cap *= 2;
    }
    const int k = client.read((uint8_t *)body + n, cap - n);
    if (k > 0) n += k, last = millis();
    else if (!client.connected() && !client.available()) break;
    else delay(5);
  }
  client.stop();
  if (!body) return LOG("http: no memory for the answer from %s\n", host), false;
  const DeserializationError err = filter ? deserializeJson(doc, body, n, DeserializationOption::Filter(*filter))
                                          : deserializeJson(doc, body, n);
  heap_caps_free(body);
  if (err) LOG("json: %s from %s\n", err.c_str(), host);
  return !err;
}
