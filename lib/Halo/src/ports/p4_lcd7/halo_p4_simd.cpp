// The P4's SIMD extension (PIE: 128-bit registers), for the loop every full-screen effect is made of: a veil over a row.
// Measured in a tile, per pixel: the plain loop 13 cycles over black and 23 over color; here 8 pixels go through at once.
//
// Per pixel: dst = dst * (256 - A) / 256 + color * A / 256, each of R, G and B in its own 16-bit lane multiply. (R and B
// once went through one multiply as (R<<11 | B) * k: R's low product bits landed on B's top bits — a blue cast wherever a
// coloured veil or a veil over a picture was drawn; black over black, all there was then, never showed it. 9 Nov 2026)
// With 256 levels of A there are no visible steps, so this needs no dither pattern, unlike the 32 levels of the S3's loop.
//
// PIE registers are saved per task by the system, but not available in interrupts: only tasks may call this.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"

#pragma GCC optimize("O3", "fast-math")

static inline uint16_t veil_px(uint32_t c, uint32_t kinv, uint32_t veil) {
  const uint32_t r = ((c & 0xF800u) * kinv >> 8) & 0xF800u, g = ((c & 0x07E0u) * kinv >> 8) & 0x07E0u, b = (c & 0x001Fu) * kinv >> 8;
  return (uint16_t)(r + g + b + veil);
}

void HALO_HOT halo_fast_veil(uint16_t *p, int n, uint16_t color, int A) {
  const uint32_t kinv = 256 - A;
  // The veil's own part, each channel rounded down on its own: with the picture's part it never passes 31 / 63 / 31
  const uint32_t veil = (((color & 0xF800u) * (uint32_t)A >> 8) & 0xF800u) | (((color & 0x07E0u) * (uint32_t)A >> 8) & 0x07E0u) |
                        ((color & 0x001Fu) * (uint32_t)A >> 8);
  while (n > 0 && ((uintptr_t)p & 15)) *p = veil_px(*p, kinv, veil), p++, n--;  // up to a 16-byte boundary
  const int blocks = n >> 3;
  if (blocks) {
    // (not "static": the factors change with every call. 16-byte alignment is not needed for broadcast loads)
    uint16_t K[5] = {0xF800, 0x07E0, (uint16_t)kinv, (uint16_t)veil, 0x001F};
    // PIE takes addresses only in the short register set (a0-a5, s0-s1)
    register uint16_t *q __asm__("a0") = p;
    register const uint16_t *k __asm__("a1") = K;
    register int left __asm__("a2") = blocks;
    register int tmp __asm__("a3") = 8;
    __asm__ volatile(
        "esp.movx.w.sar a3\n"
        "esp.vldbc.16.ip q4, a1, 2\n"  // mask R
        "esp.vldbc.16.ip q5, a1, 2\n"  // mask G
        "esp.vldbc.16.ip q6, a1, 2\n"  // 256 - A
        "esp.vldbc.16.ip q7, a1, 2\n"  // the veil
        "esp.vldbc.16.ip q3, a1, 2\n"  // mask B
        "1:\n"
        "esp.vld.128.ip q0, a0, 0\n"
        "esp.andq q1, q0, q4\n"
        "esp.vmul.u16 q1, q1, q6\n"
        "esp.andq q1, q1, q4\n"
        "esp.andq q2, q0, q5\n"
        "esp.vmul.u16 q2, q2, q6\n"
        "esp.andq q2, q2, q5\n"
        "esp.vadd.u16 q1, q1, q2\n"
        "esp.andq q2, q0, q3\n"
        "esp.vmul.u16 q2, q2, q6\n"
        "esp.vadd.u16 q1, q1, q2\n"
        "esp.vadd.u16 q0, q1, q7\n"
        "esp.vst.128.ip q0, a0, 16\n"
        "addi a2, a2, -1\n"
        "bnez a2, 1b\n"
        : "+r"(q), "+r"(left), "+r"(k), "+r"(tmp)
        :
        : "memory");
    p += blocks * 8, n -= blocks * 8;
  }
  while (n-- > 0) *p = veil_px(*p, kinv, veil), p++;
}

#endif  // CONFIG_IDF_TARGET_ESP32P4
