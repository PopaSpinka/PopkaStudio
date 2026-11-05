// esp_timer.h — the board's microsecond clock; in the browser it is the page's clock (halo_web.cpp).
#pragma once
#include <stdint.h>
int64_t esp_timer_get_time();
