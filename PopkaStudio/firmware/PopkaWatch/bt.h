// bt.h — Bluetooth (the editor: Functions → Bluetooth). On — the watch shows itself over Bluetooth Low Energy under
// its name, PopkaWatch: a phone sees it (in an app such as nRF Connect or LightBlue; iOS's own Bluetooth settings list
// only devices it has a use for). Off — no Bluetooth code in the firmware at all, and its memory is free for the rest
// (the Arduino core gives it back when nothing uses Bluetooth).
// It starts a moment after the first frames (bt_tick, from loop): the panel's buffers are in internal memory by then.
#pragma once

// One at a time with Wi-Fi: Bluetooth keeps ~71 KB of internal memory (NimBLE, the core keeps it internal), Wi-Fi needs
// ~65 KB to start, and next to the panel's buffers there are 103 KB free — both on crashed at the first frame (the
// editor and studio.py turn the other one off)
#if FN_BT && FN_WIFI
#error "Bluetooth and Wi-Fi don't fit in internal memory together: turn one of them off"
#endif

#if FN_BT && !defined(__EMSCRIPTEN__)
#include <BLEDevice.h>

static uint32_t bt_at;  // when to start (ms); 0 — started

static void bt_begin() {
  bt_at = millis() + 1500;
}

static void bt_tick() {
  if (!bt_at || (int32_t)(millis() - bt_at) < 0) return;
  bt_at = 0;
  const unsigned before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024;
  BLEDevice::init("PopkaWatch");
  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->setAppearance(0x00C0);  // a watch (Bluetooth's appearance list: 0x00C0 — generic watch)
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();
  LOG("bt: on, PopkaWatch, free internal %u -> %u KB (largest %u KB)\n", before,
      (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
      (unsigned)(heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024));
}
#else
static inline void bt_begin() {}
static inline void bt_tick() {}
#endif
