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
  client.printf("GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Halo\r\nConnection: close\r\n\r\n", path, host);
  String status = client.readStringUntil('\n');  // "HTTP/1.1 200 OK"
  if (status.indexOf(" 200 ") < 0) {
    LOG("http: %s from %s\n", status.c_str(), host);
    client.stop();
    return false;
  }
  client.find("\r\n\r\n");  // the headers
  const DeserializationError err = filter ? deserializeJson(doc, client, DeserializationOption::Filter(*filter))
                                          : deserializeJson(doc, client);
  client.stop();
  if (err) LOG("json: %s from %s\n", err.c_str(), host);
  return !err;
}
