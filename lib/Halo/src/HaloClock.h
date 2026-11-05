// HaloClock — точное время из интернета для скетчей на Halo: Wi-Fi включается, узнаёт время по NTP и выключается;
// через 6 часов — снова (сети нет — через 10 минут). Как net_time_only() в AmoledNet, но без LVGL и AmoledBoard.
//
// Отдельный заголовок, а не часть Halo.h: Wi-Fi попадает в прошивку только тем скетчам, что его подключили
// (~0,4 МБ прошивки и ~65 КБ внутренней памяти на время подключения — у Halo её с запасом). Сеть и пароль — общие
// (заголовок с WIFI_SSID и WIFI_PASSWORD, если он есть), или свои (PopkaStudio передаёт сеть, выбранную в редакторе). Всё — в своей задаче
// FreeRTOS на ядре 0: кадры не ждут сети.
//   #include <HaloClock.h>
//   setup():  halo_begin(update, draw, cfg);  halo_clock_begin("EST5EDT,M3.2.0,M11.1.0");  // или (tz, ssid, пароль)
//   update(): if (halo_clock_valid()) { time_t now = time(nullptr); struct tm t; localtime_r(&now, &t); … }
// Работа «пока есть сеть» (погода, …): halo_clock_online(fn, раз в N с) до halo_clock_begin — тогда Wi-Fi включается
// раз в N с (а не в 6 часов), сверяет время и вызывает fn, пока подключён (HaloHttp.h — запрос JSON).
// Команды: ntp — сверить сейчас (и сделать работу); wifi — как прошла последняя сверка; wifi scan — какие сети видит плата (по строке
// «wifi: net <уровень дБм> <0 — открытая, 1 — с паролем> <имя>», в конце «wifi: scan done <сколько>»; ими пользуется
// PopkaStudio: studio.py, список сетей в меню Functions).
#pragma once

#include <Halo.h>
#include <WiFi.h>
#include <esp_sntp.h>
#if __has_include("wifi_secrets.h")  // the shared network (if there is one); else — only the one passed to begin
#include "wifi_secrets.h"
#endif
#ifndef WIFI_SSID
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#endif

static volatile bool hclk_synced, hclk_scan_req;
// Последняя сверка: 0 — ещё не было, 1 — время есть, 2 — к сети не подключилась, 3 — подключилась, но время не пришло
static volatile uint8_t hclk_state;
static char hclk_tz[48] = "UTC0";
static const char *hclk_ssid = WIFI_SSID, *hclk_pass = WIFI_PASSWORD;
static TaskHandle_t hclk_handle;
static void (*hclk_online)();         // работа, пока Wi-Fi подключён (halo_clock_online)
static uint32_t hclk_every = 6 * 3600;  // как часто включать Wi-Fi, когда всё прошло (с)

static inline bool halo_clock_valid() {
  return time(nullptr) > 1735689600;  // 2025-01-01: раньше — время ещё не получено
}

static void hclk_on_sync(struct timeval *) {
  hclk_synced = true;
}

static void hclk_status() {
  static const char *const name[4] = {"wait", "synced", "nonet", "noinet"};
  LOG("wifi: status %s %s\n", name[hclk_state & 3], hclk_ssid);
}

// Сети вокруг (только 2,4 ГГц — другие плата не видит): Wi-Fi на время поиска, 2–4 с
static void hclk_scan() {
  halo_stay_awake(true);
  WiFi.mode(WIFI_STA);
  const int n = WiFi.scanNetworks();
  hclk_status();
  for (int i = 0; i < n; i++)
    if (WiFi.SSID(i).length())  // (скрытые сети — без имени)
      LOG("wifi: net %d %d %s\n", (int)WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? 0 : 1, WiFi.SSID(i).c_str());
  WiFi.scanDelete();
  WiFi.mode(WIFI_OFF);
  halo_stay_awake(false);
  LOG("wifi: scan done %d\n", n < 0 ? 0 : n);
}

// Подключиться и узнать время (не дольше минуты; просьба поискать сети прерывает попытку — она повторится)
static void hclk_sync() {
  hclk_synced = false;
  halo_stay_awake(true);  // пока Wi-Fi включён, плата не засыпает по-настоящему (halo_sleep): связь оборвалась бы
  LOG("clock: wifi on\n");
  WiFi.mode(WIFI_STA);
  WiFi.begin(hclk_ssid, hclk_pass);
  sntp_set_time_sync_notification_cb(hclk_on_sync);
  configTzTime(hclk_tz, "pool.ntp.org", "time.google.com");  // SNTP сам дождётся сети
  const uint32_t t0 = millis();
  bool joined = false;
  while (!hclk_synced && !hclk_scan_req && millis() - t0 < 60000) {
    vTaskDelay(pdMS_TO_TICKS(250));
    joined |= WiFi.status() == WL_CONNECTED;
  }
  const bool ok = hclk_synced;
  if (hclk_online && !hclk_scan_req && WiFi.status() == WL_CONNECTED) hclk_online();  // (погода и т. п.)
  // Попытки подключения занимают ядро 0 — выключаем сразу, как только время есть (или сети нет минуту)
  esp_sntp_stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  halo_stay_awake(false);
  if (ok || !hclk_scan_req) hclk_state = ok ? 1 : joined ? 3 : 2;
  LOG("clock: %s, wifi off, free internal %u KB\n", ok ? "time synced" : hclk_scan_req ? "stopped for a scan" : joined ? "no time" : "no network",
      (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
}

static void hclk_task(void *) {
  uint32_t wait_s = 0;  // 0 — сверить сразу
  for (;;) {
    // Ждём до следующей сверки (ntp — сверить сейчас, wifi scan — поискать сети). Не pdMS_TO_TICKS(6 ч): там мс × 1000
    // в 32 битах — переполнение, и ждали бы 2 минуты
    if (wait_s) ulTaskNotifyTake(pdTRUE, (TickType_t)wait_s * configTICK_RATE_HZ);
    if (hclk_scan_req) {
      hclk_scan();
      hclk_scan_req = false;
      if (wait_s) continue;  // (поиск — не повод сверять время)
    }
    if (!*hclk_ssid) {  // (сети нет: только поиск)
      hclk_state = 2, wait_s = 24 * 3600;
      continue;
    }
    hclk_sync();
    wait_s = hclk_scan_req ? 1 : hclk_state == 1 ? hclk_every : 600;  // (прервана поиском — сверить сразу после него)
  }
}

// Работа на время Wi-Fi и как часто (с): до halo_clock_begin
static inline void halo_clock_online(void (*fn)(), uint32_t every_s) {
  hclk_online = fn, hclk_every = every_s;
}

static inline void halo_clock_begin(const char *tz, const char *ssid = WIFI_SSID, const char *pass = WIFI_PASSWORD) {
  strlcpy(hclk_tz, tz ? tz : "UTC0", sizeof(hclk_tz));
  setenv("TZ", hclk_tz, 1);
  tzset();
  hclk_ssid = ssid ? ssid : "", hclk_pass = pass ? pass : "";
  if (!strlen(hclk_ssid)) LOG("clock: wifi not configured (no network given)\n");
  xTaskCreatePinnedToCore(hclk_task, "clock", hclk_online ? 8192 : 6144, nullptr, 1, &hclk_handle, 0);  // (запрос — глубже)
  halo_command("ntp", [](const char *) { xTaskNotifyGive(hclk_handle); });  // сверить время сейчас
  halo_command("wifi", [](const char *a) {
    if (strncmp(a, "scan", 4)) return hclk_status();
    hclk_scan_req = true;
    xTaskNotifyGive(hclk_handle);
  });
}
