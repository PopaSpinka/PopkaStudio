// pixelart.h — the pixel art layers (face.h: fl_pixels): a GIF of big pixels drawn cell by cell by lib/Halo (HaloPixels,
// HI_PIXELS), not played as pictures. tools/halo_pixels.py makes the cells from a GIF: Popka, the parrot (FL_PARROT,
// parrot.h from assets/parrot.gif, drawn by assets/make_parrot.py) — 8 frames of 48 × 40 cells, 12 colors of the parrot,
// 2 of the notes and twinkles around it. The GIF's night is "nothing" — the screen's black (or the layers under it). A
// frame lasts as long as in the GIF; between two frames nothing is drawn, at a frame only the cells that changed.
// The layer (FaceLayer): dx, dy — where the picture's middle is, from the screen's (it turns about it); k — its size (1 —
// PX_CELL px a cell); p — the turn, degrees clockwise; p2 — what is around it: 0 — off, 1 — around it (as the GIF has
// it), 2 — all over the screen (repeated every way: HaloItem.tile); p3 — a trail's length, times the GIF's (its wave
// repeated further left: HaloItem.trail; a picture without a trail ignores it). Only what is on the screen is drawn and
// checked for changes: the copies beyond the edge cost nothing.
// In Always On — its first frame, still.
// Only in a firmware with such a layer (FN_PARROT — the editor writes it into layers.h); the editor's page has them
// always. Included from face.h after stars.h.
#if HAS_PIXELS
#if HAS_PARROT
#include "parrot.h"
#endif
#define PX_CELL 7.0f  // px a cell at size 1

static const HaloPixels &pixel_art(uint8_t kind) {  // the layer's picture
  (void)kind;
#if HAS_PARROT
  return parrot;
#else
#endif
}

static void pixel_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  HaloItem &it = R.pill;
  const HaloPixels &P = pixel_art(L.kind);
  it.kind = HI_PIXELS, it.pixels = &P;
  it.x = CX + L.dx, it.y = CX + L.dy, it.scale = PX_CELL * L.k, it.a0 = L.p * (HALO_PI / 180), it.alpha = a;
  it.hide = L.p2 > 0.5f ? 0 : P.apart, it.tile = L.p2 > 1.5f, it.on_black = true;
  it.trail = L.p3 > 1 ? (int16_t)lroundf((L.p3 - 1) * P.tn) : 0;
  it.frame = fc_still ? 0 : (halo_ms() / P.ms) % P.frames;
  R.ax = it.x, R.ay = it.y;
  halo_view_commit(it, v);
  // Where it is: the picture and its trail (as long as it is), turned (what is around it is not) — the editor frames that
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  if (!it.shown) return;
  const float c = cosf(it.a0) * it.scale, n = sinf(it.a0) * it.scale;
  R.box[0] = R.box[1] = 1e9f, R.box[2] = R.box[3] = -1e9f;
  for (int k = 0; k < 4; k++) {
    const float u = (k & 1 ? P.bx1 : P.bx0 - it.trail) - P.ax, w = (k & 2 ? P.by1 : P.by0) - P.ay;
    const float X = it.x + u * c - w * n, Y = it.y + u * n + w * c;
    R.box[0] = fminf(R.box[0], X), R.box[1] = fminf(R.box[1], Y), R.box[2] = fmaxf(R.box[2], X), R.box[3] = fmaxf(R.box[3], Y);
  }
}

static void pixel_draw(const FaceScreen &S, int i) {
  halo_item_draw(S.run[i].pill);
}
#endif
