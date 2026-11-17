// WT32-SC01 Plus (Wireless-Tag): ESP32-S3, a 3.5″ 320 × 480 LCD (ST7796, 8-bit parallel bus), 2 MB of PSRAM. The numbers
// every part of Halo needs to know about this board.
//
// The screen is used either way up: tall (320 × 480, the panel's own) or wide (480 × 320, -DHALO_LANDSCAPE — the panel
// turns the picture itself, MADCTL). A project picks one; each is its own build.
#pragma once

#ifdef HALO_LANDSCAPE
#define HALO_W 480
#define HALO_H 320
#else
#define HALO_W 320
#define HALO_H 480
#endif
#define HALO_ROWS 16                                // rows in a band
#define HALO_BANDS ((HALO_H + HALO_ROWS - 1) / HALO_ROWS)
#define HALO_HOT IRAM_ATTR __attribute__((noinline))  // hot drawing code — in internal memory
#define HALO_PX(c) __builtin_bswap16(c)             // a pixel in memory: the panel takes the high byte first
#define HALO_CPU_MHZ 240
#define HALO_TLS __thread
#define HALO_COMPACT 1  // a small rectangle: screens are laid out from HALO_W and HALO_H, not for the 7″ board
