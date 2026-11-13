// HaloClock.h — the browser build: the time is already right (the computer's clock), so no Wi-Fi and no password.
// Found before lib/Halo/src/HaloClock.h because engine/shim comes first in the include path.
#pragma once
#include <Halo.h>
#include <stdlib.h>
#include <time.h>

static inline bool halo_clock_valid() {
  return true;
}

static inline void halo_clock_online(void (*)(), uint32_t) {}  // (the page gets the weather from studio.py)

static inline void halo_clock_begin(const char *tz, const char * = nullptr, const char * = nullptr) {
  setenv("TZ", tz ? tz : "UTC0", 1);
  tzset();
  LOG("clock: the computer's time\n");
}
