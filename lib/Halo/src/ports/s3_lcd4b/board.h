// Waveshare ESP32-S3-Touch-LCD-4B: ESP32-S3 (8 MB PSRAM), a 4″ 480 × 480 IPS panel (ST7701) on the 16-bit RGB bus. The numbers
// every part of Halo needs to know about this board.
#pragma once

#define HALO_W 480
#define HALO_H 480
#define HALO_ROWS 16                                // rows in a band
#define HALO_BANDS ((HALO_H + HALO_ROWS - 1) / HALO_ROWS)
#define HALO_HOT IRAM_ATTR __attribute__((noinline))  // hot drawing code — in internal memory
#define HALO_PX(c) ((uint16_t)(c))                  // a pixel in memory: the LCD peripheral reads the frame buffer as it is
#define HALO_CPU_MHZ 240
#define HALO_TLS __thread
#define HALO_COMPACT 1  // a small rectangle: screens are laid out from HALO_W and HALO_H, not for the 7″ board
#define HALO_SQUARE 1   // and a square: its own layouts where the wide ones (480 × 320) would leave the lower third empty
                        // (web/mockups/square-screens.html: 1A, 2D, 4A, 5C, 6C, 7C)
