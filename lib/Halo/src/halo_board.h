// Which board this build is for. Everything board-specific lives in ports/<board>/ (ports/README.md); this file only
// picks the one being built. The browser engine (studio/engine) previews the round watch for now.
#pragma once

#if defined(HALO_WEB_BOARD_P4) || CONFIG_IDF_TARGET_ESP32P4  // (HALO_WEB_BOARD_P4: the browser engine of this board)
#include "ports/p4_lcd7/board.h"
#elif defined(__EMSCRIPTEN__) || CONFIG_IDF_TARGET_ESP32S3
#include "ports/s3_amoled175/board.h"
#else
#error "Halo: no port for this board yet (lib/Halo/src/ports/README.md)"
#endif
