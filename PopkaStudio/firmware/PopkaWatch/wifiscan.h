// wifiscan.h — a firmware without the Wi-Fi function can still tell which networks it sees, so the editor's Wi-Fi
// window (web/js/wifi.js) can list them before the first Load with Wi-Fi: the command "wifi scan" — Wi-Fi only for the
// scan (2–4 s, in its own task on core 0), then off and its memory given back; the same lines as HaloClock's: "wifi:
// net <dBm> <0 — open, 1 — a password> <name>", "wifi: scan done <how many>". It never joins a network. This is the one
// exception to PopkaStudio's "only what is added is in the firmware" (6 Nov 2026): without it the editor couldn't list
// the networks for a board that has no Wi-Fi yet — macOS hides the networks' names from programs (location access).
// With Bluetooth on there is no room for Wi-Fi (bt.h): the scan finds nothing. Included from PopkaWatch.ino when
// FN_WIFI is 0; the editor's page has no radio — nothing there.
#pragma once
#ifndef __EMSCRIPTEN__
#include <WiFi.h>

static TaskHandle_t ws_task;

static void ws_scan(void *) {
  halo_stay_awake(true);
  LOG("wifi: status off\n");
  int n = -1;
  if (WiFi.mode(WIFI_STA)) {
    n = WiFi.scanNetworks();
    for (int i = 0; i < n; i++)
      if (WiFi.SSID(i).length())  // (hidden networks have no name)
        LOG("wifi: net %d %d %s\n", (int)WiFi.RSSI(i), WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? 0 : 1, WiFi.SSID(i).c_str());
    WiFi.scanDelete();
  }
#ifndef HALO_WIFI_KEEP
  WiFi.mode(WIFI_OFF);  // (its memory back: the screen's buffers need it)
#endif
  halo_stay_awake(false);
  LOG("wifi: scan done %d\n", n < 0 ? 0 : n);
  ws_task = nullptr;
  vTaskDelete(nullptr);
}

static void wifiscan_build() {
  halo_command("wifi", [](const char *a) {
    if (strncmp(a, "scan", 4)) return (void)LOG("wifi: status off\n");
#if defined(FN_BT) && FN_BT
    LOG("wifi: status off\nwifi: scan done 0\n");  // (Bluetooth holds the memory Wi-Fi would need)
#else
    if (!ws_task) xTaskCreatePinnedToCore(ws_scan, "wifiscan", 6144, nullptr, 1, &ws_task, 0);
#endif
  });
}
#else
static void wifiscan_build() {}
#endif
