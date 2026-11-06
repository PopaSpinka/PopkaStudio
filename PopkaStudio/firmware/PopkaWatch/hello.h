// hello.h — who this board is, for the editor (studio.py: BoardWatch, LiveLink; docs/board-detect.md, step 1):
//   hello  → POPKA 1 board=amoled175 fw=1.1.1 project=project-1 rev=57 loaded=2026-11-06T12:03 live=1
// The board's model, PopkaStudio's version that made this firmware, the project it was made from and that project's
// revision, when it was loaded, and that it has Live. The editor asks once when the board appears (no reset: the port
// is opened with DTR=1, RTS=0) — an answer: ours; "unknown command: hello" — ours but older; silence — other firmware.
// Plugged into USB the board never truly sleeps (Halo "pretends", docs/halo.md), so it always answers.
#pragma once

// The values come from config.h (studio.py writes them at a Load); the folder's own default firmware has none
#ifndef POPKA_BOARD
#define POPKA_BOARD "amoled175"
#endif
#ifndef POPKA_VERSION
#define POPKA_VERSION "dev"
#endif
#ifndef POPKA_PROJECT
#define POPKA_PROJECT "-"
#endif
#ifndef POPKA_REV
#define POPKA_REV 0
#endif
#ifndef POPKA_LOADED
#define POPKA_LOADED "-"
#endif

static void hello_build() {
  halo_command("hello", [](const char *) {
    Serial.printf("POPKA 1 board=%s fw=%s project=%s rev=%d loaded=%s live=1\n", POPKA_BOARD, POPKA_VERSION,
                  POPKA_PROJECT, (int)POPKA_REV, POPKA_LOADED);
  });
}
