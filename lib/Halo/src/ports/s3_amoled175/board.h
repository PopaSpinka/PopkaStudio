// Waveshare ESP32-S3-Touch-AMOLED-1.75C: the numbers every part of Halo needs to know about this board.
#pragma once

#define HALO_W 466
#define HALO_H 466
#define HALO_ROWS 16                                // строк в пачке
#define HALO_BANDS ((HALO_H + HALO_ROWS - 1) / HALO_ROWS)
#define HALO_HOT IRAM_ATTR __attribute__((noinline))  // горячий код рисования — во внутренней памяти
#define HALO_PX(c) __builtin_bswap16(c)            // a pixel in memory: the panel takes the high byte first
#define HALO_CPU_MHZ 240
#define HALO_TLS __thread
#define HALO_ROUND 1  // the screen is a circle: nothing outside it is drawn
