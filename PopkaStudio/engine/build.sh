#!/bin/bash
# The page's engine: firmware/PopkaWatch built for the browser (WebAssembly). The sketch and lib/Halo's drawing half
# go in unchanged; halo_web.cpp stands in for the hardware half (the panel, the touch chip, the buttons), shim/ for
# Arduino and ESP-IDF; studio_api.h — the editor's calls into the sketch.
#
#   PopkaStudio/engine/build.sh      → PopkaStudio/web/build/PopkaWatch.js + .wasm  (studio.py runs it when needed)
#
# Needs Emscripten (em++): brew install emscripten. The Wi-Fi password never gets in: shim/HaloClock.h stands in for
# the board's (the computer's clock), and no shared Wi-Fi file is on the include path.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ENGINE="$ROOT/PopkaStudio/engine"
SKETCH="$ROOT/PopkaStudio/firmware/PopkaWatch"
# Which board: none — the round one (web/build, as it always was); p4lcd7 — its own engine in web/build/p4lcd7
BOARD="${1:-amoled175}"
OUT="$ROOT/PopkaStudio/web/build"
TMP="$ROOT/build/PopkaStudio/engine"
EXTRA=()
if [ "$BOARD" = p4lcd7 ]; then
  OUT="$OUT/p4lcd7" TMP="$TMP-p4lcd7" EXTRA=(-DHALO_WEB_BOARD_P4)
fi
PY="${PY:-python3}"
command -v em++ >/dev/null || { echo "em++ not found: brew install emscripten" >&2; exit 1; }

HALO="$ROOT/lib/Halo/src"
mkdir -p "$OUT" "$TMP/out"
# The .ino as a C++ file (as the Arduino build does: Arduino.h first), then the editor's calls
WRAP="$TMP/PopkaWatch.sketch.cpp"
# The sketch's time() is the computer's clock moved by the zone picked in the editor (studio_clock): localtime here is
# always the computer's zone, so the page shows the board's wall clock
printf '#include <Arduino.h>\n#include <sys/time.h>\nstatic int st_time_shift;\nstatic inline time_t web_time(time_t *p) { time_t t = (time)(nullptr) + st_time_shift; if (p) *p = t; return t; }\n#define time(p) web_time(p)\n#include "%s"\n#include "%s"\n' "$SKETCH/PopkaWatch.ino" "$ENGINE/studio_api.h" > "$WRAP"
SRCS=("$ENGINE/halo_web.cpp" "$HALO/halo_draw.cpp" "$HALO/halo_ui.cpp" "$HALO/halo_list.cpp" "$HALO/halo_nav.cpp"
      "$HALO/halo_roll.cpp" "$SKETCH/src/halo_assets.cpp" "$WRAP")
if [ "$BOARD" = p4lcd7 ]; then  # a rectangle's own fonts (gallery_rect.h), beside the template's
  mkdir -p "$TMP/rect/assets"
  "$PY" "$ROOT/PopkaStudio/studio.py" --rect-conf "$TMP/rect/assets/halo.conf"
  "$PY" "$ROOT/tools/halo_assets.py" "$TMP/rect" >/dev/null
  SRCS+=("$TMP/rect/src/halo_assets.cpp")
else  # the round board's own fonts (shade_round.h): only its firmwares with the layer have them, its page — always
  mkdir -p "$TMP/round/assets"
  "$PY" "$ROOT/PopkaStudio/studio.py" --round-conf "$TMP/round/assets/halo.conf"
  "$PY" "$ROOT/tools/halo_assets.py" "$TMP/round" >/dev/null
  SRCS+=("$TMP/round/src/halo_assets.cpp")
fi
# As on the board: Halo and the sketch are built "O3, fast-math" (their #pragma GCC optimize, which clang ignores)
FLAGS=(-O3 -ffast-math -std=gnu++17 -I"$ENGINE/shim" -I"$HALO" -I"$ROOT/lib/ArduinoJson/src"
       -I"$SKETCH" -Wno-unknown-pragmas -Wno-ignored-pragmas -Wno-ignored-attributes -Wno-unused-function
       -Wno-unused-variable -Wno-deprecated-declarations)
EXPORTS='["_main","_web_frame","_web_touch","_web_command","_web_rgba","_web_dirty","_web_clock_ms","_studio_font","_studio_icons",'
EXPORTS+='"_studio_set","_studio_pin","_studio_boxes","_studio_radii","_studio_places","_studio_bases","_studio_open","_studio_stub",'
EXPORTS+='"_studio_main","_studio_scroll","_studio_tap","_studio_fn","_studio_clock","_studio_page","_studio_pages","_studio_city","_studio_weather","_web_finger_y","_web_nav_pull","_malloc","_free"]'

echo "building PopkaWatch for the browser…"
em++ "${FLAGS[@]}" ${EXTRA[@]+"${EXTRA[@]}"} "${SRCS[@]}" \
  -sMODULARIZE=1 -sEXPORT_ES6=1 -sEXPORT_NAME=createHalo -sENVIRONMENT=web,node \
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=16777216 -sSTACK_SIZE=1048576 \
  -sEXPORTED_FUNCTIONS="$EXPORTS" \
  -sEXPORTED_RUNTIME_METHODS='["HEAPU8","HEAPF32","stringToNewUTF8","UTF8ToString"]' \
  -o "$TMP/out/PopkaWatch.js"
# Into the page's folder only when whole, by renaming (6 Nov 2026): a page opening while this builds — another
# studio.py starting, the Mac app's window — took a half-written engine and broke ("Aborted()", no tile pictures)
mv -f "$TMP/out/PopkaWatch.wasm" "$OUT/PopkaWatch.wasm"
mv -f "$TMP/out/PopkaWatch.js" "$OUT/PopkaWatch.js"
ls -l "$OUT/PopkaWatch.js" "$OUT/PopkaWatch.wasm" | awk '{printf "%s  %d KB\n", $NF, $5/1024}'
