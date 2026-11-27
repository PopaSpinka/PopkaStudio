// Waveshare ESP32-S3-Touch-AMOLED-1.75C: the numbers every part of Halo needs to know about this board.
// The same port runs the 1.75 (-B, -G) and the 2.06 (-DHALO_BOARD_AMOLED206: a 410 × 502 rounded rectangle, the same
// CO5300 panel chip): their differences below.
#pragma once

#if defined(HALO_BOARD_AMOLED206)
#define HALO_W 410
#define HALO_H 502
#else
#define HALO_W 466
#define HALO_H 466
#endif
#define HALO_ROWS 16                                // строк в пачке
#define HALO_BANDS ((HALO_H + HALO_ROWS - 1) / HALO_ROWS)
#define HALO_HOT IRAM_ATTR __attribute__((noinline))  // горячий код рисования — во внутренней памяти
#define HALO_PX(c) __builtin_bswap16(c)            // a pixel in memory: the panel takes the high byte first
#define HALO_CPU_MHZ 240
#define HALO_TLS __thread
#if defined(HALO_BOARD_AMOLED206)
#define HALO_COMPACT 1  // a small rectangle: screens are laid out from HALO_W and HALO_H (the WT32's tall layouts) …
#define HALO_WATCH 1    // … but a watch: its own layouts of some screens — pills the whole width, the round watch's sizes
                        // (*_rect.h, branches #if defined(HALO_WATCH); web/mockups/watch206-look.html)
#else
#define HALO_ROUND 1  // the screen is a circle: nothing outside it is drawn
#endif

#if defined(HALO_BOARD_AMOLED175G)
// The 1.75 (-B, -G) has a microSD slot, wired for SPI (its own bus, SPI3: SPI2 is the display's). The firmware's
// Music player reads its tracks from it (firmware/PopkaWatch/music_sd.h); a board without these has no card
#define HALO_SD_MOSI 1
#define HALO_SD_SCK 2
#define HALO_SD_MISO 3
#define HALO_SD_CS 41
#elif defined(HALO_BOARD_AMOLED206)
#define HALO_SD_MOSI 1  // the 2.06's microSD slot: the same wiring, its own chip select
#define HALO_SD_SCK 2
#define HALO_SD_MISO 3
#define HALO_SD_CS 17
#endif
