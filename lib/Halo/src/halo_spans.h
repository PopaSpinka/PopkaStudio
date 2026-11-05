// halo_spans.h — what changed in a band (16 rows of the screen): up to HALO_SPANS spans of columns, drawn and sent to
// the panel each on its own. Changes far apart (sparkles all over the screen, a clock at one side and a pill at the
// other) — each its own span, so the pixels between them are neither drawn nor sent; closer than HALO_SPAN_GAP — one
// span (a window of its own costs more than that many pixels); one more than fits — joined to the span it grows the
// least. Before, a band had one span from its leftmost change to its rightmost: three small stars in a band — the
// whole band. Shared by halo_frame.cpp and the editor's engine (PopkaStudio/engine/halo_web.cpp), so both draw the
// same parts.
#pragma once
#include <stdint.h>

#ifndef HALO_SPANS  // (a build may set it: -DHALO_SPANS=1 — one span a band, as before, to compare)
#define HALO_SPANS 4
#endif
#define HALO_SPAN_GAP 48

struct HaloSpans {
  int16_t x0[HALO_SPANS], x1[HALO_SPANS];
  uint8_t n;  // 0 — the band is clean
};

static inline void halo_spans_drop(HaloSpans &d, int i) {
  d.n--;
  d.x0[i] = d.x0[d.n], d.x1[i] = d.x1[d.n];
}

// Columns x0..x1 changed: into the spans it touches (within the gap), else a span of its own, else into the nearest
static inline void halo_spans_add(HaloSpans &d, int x0, int x1) {
  for (;;) {
    bool joined = false;
    for (int i = 0; i < d.n; i++) {
      if (x0 > d.x1[i] + HALO_SPAN_GAP || x1 < d.x0[i] - HALO_SPAN_GAP) continue;
      if (d.x0[i] < x0) x0 = d.x0[i];
      if (d.x1[i] > x1) x1 = d.x1[i];
      halo_spans_drop(d, i);
      joined = true;
      break;
    }
    if (joined) continue;  // (grown: it may touch another one now)
    if (d.n < HALO_SPANS) {
      d.x0[d.n] = x0, d.x1[d.n] = x1, d.n++;
      return;
    }
    int best = 0, grow = 1 << 30;
    for (int i = 0; i < d.n; i++) {
      const int a = x0 < d.x0[i] ? x0 : d.x0[i], b = x1 > d.x1[i] ? x1 : d.x1[i], g = (b - a) - (d.x1[i] - d.x0[i]);
      if (g < grow) grow = g, best = i;
    }
    if (d.x0[best] < x0) x0 = d.x0[best];
    if (d.x1[best] > x1) x1 = d.x1[best];
    halo_spans_drop(d, best);
  }
}

// Left to right (a frame draws and sends them in order)
static inline void halo_spans_sort(HaloSpans &d) {
  for (int i = 1; i < d.n; i++)
    for (int j = i; j > 0 && d.x0[j] < d.x0[j - 1]; j--) {
      const int16_t a = d.x0[j], b = d.x1[j];
      d.x0[j] = d.x0[j - 1], d.x1[j] = d.x1[j - 1], d.x0[j - 1] = a, d.x1[j - 1] = b;
    }
}
