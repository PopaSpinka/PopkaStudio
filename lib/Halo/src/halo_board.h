// Which board this build is for. Everything board-specific lives in ports/<board>/ (ports/README.md); this file only
// picks the one being built. The browser engine (studio/engine) previews the round watch for now.
//
// A chip family with one board is known by the chip. A second board on the same chip is named by the build:
// -DHALO_BOARD_<NAME> (its passport's "defines" — studio.py puts them on the compiler's line, engine/build.sh too).
#pragma once

#if defined(HALO_BOARD_WT32SC01P)  // WT32-SC01 Plus: an ESP32-S3 too, so the chip alone does not tell
#include "ports/s3_wt32sc01p/board.h"
#elif defined(HALO_BOARD_LCD4B)  // Waveshare ESP32-S3-Touch-LCD-4B: an ESP32-S3 as well
#include "ports/s3_lcd4b/board.h"
#elif defined(HALO_BOARD_AMOLED175G)  // Waveshare ESP32-S3-Touch-AMOLED-1.75 / -G: the round 1.75C's port, its own pins
#include "ports/s3_amoled175/board.h"
#elif defined(HALO_BOARD_AMOLED206)  // Waveshare ESP32-S3-Touch-AMOLED-2.06: the 1.75C's chips, 410 × 502, its own pins
#include "ports/s3_amoled175/board.h"
#elif defined(HALO_WEB_BOARD_P4) || CONFIG_IDF_TARGET_ESP32P4  // (HALO_WEB_BOARD_P4: the browser engine of this board)
#include "ports/p4_lcd7/board.h"
#elif defined(__EMSCRIPTEN__) || CONFIG_IDF_TARGET_ESP32S3
#include "ports/s3_amoled175/board.h"
#else
#error "Halo: no port for this board yet (lib/Halo/src/ports/README.md)"
#endif
