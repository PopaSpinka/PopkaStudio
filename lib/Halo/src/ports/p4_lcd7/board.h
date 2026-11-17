// Waveshare ESP32-P4-WIFI6-Touch-LCD-7B: the numbers every part of Halo needs to know about this board.
// Our chip is revision 1.3: 360 MHz (boards/p4lcd7/hardware.md).
#pragma once

// The panel's own frame: 1024 × 600, whichever way up the board is held (halo_p4_display.cpp, the frame buffers)
#define HALO_FB_W 1024
#define HALO_FB_H 600
// Tall (-DHALO_P4_TALL, a project's "orientation": "portrait"): the board held upright, the interface 600 × 1024. The panel
// can't turn its picture, so the frame engine turns each band on its way to the frame (halo_p4_frame.cpp: rotate_band) and
// the touch turns the finger back (halo_p4_touch.cpp: halo_p4_to_screen)
#ifdef HALO_P4_TALL
#define HALO_W 600
#define HALO_H 1024
#else
#define HALO_W 1024
#define HALO_H 600
#endif
#define HALO_ROWS 16
// Wi-Fi goes through the ESP32-C6: its driver takes one big block of internal memory when it starts (144 KB in all) and, once
// the memory is in use, can't get that block a second time: the second start ends in an assert. So after the first start
// it stays up, and a sketch only disconnects (HaloClock.h)
#define HALO_WIFI_KEEP 1
#define HALO_BANDS ((HALO_H + HALO_ROWS - 1) / HALO_ROWS)
#define HALO_HOT IRAM_ATTR __attribute__((noinline))
#define HALO_PX(c) ((uint16_t)(c))  // a pixel in memory: the frame buffer is read by the display controller as it is
#ifdef F_CPU
#define HALO_CPU_MHZ (F_CPU / 1000000)  // (a chip of revision 3 runs at 400)
#else
#define HALO_CPU_MHZ 360
#endif
#define HALO_FAST_VEIL 1  // a veil over a row is this board's own loop (halo_p4_simd.cpp)
// (without the model RISC-V reaches an extern __thread through the GOT, which this linker script does not have)
#ifdef __EMSCRIPTEN__
#define HALO_TLS __thread
#else
#define HALO_TLS __thread __attribute__((tls_model("local-exec")))
#endif
