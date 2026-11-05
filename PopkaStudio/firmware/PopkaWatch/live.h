// live.h — Live (the editor: the box in Edit): the looks of the layers this firmware has, from the editor over USB, at
// once — no Load. studio.py (LiveLink) sends a line per layer that changed, its last word — the sum of the bytes
// before it (hex, of 256: a line garbled on the way is dropped; the last state comes again when the editing stops):
//   live S I K dx dy k color color2 spacing p color3 p2 p3 hidden SUM — screen S, its layer I of kind K (colors in
//                                                                     hex; the numbers as layers.h has them: FaceLayer)
//   live S I K - SUM                                                — hide it (gone from the project or hidden there)
//   live ?   — what this firmware has: "LIVE 1 <shape> <map>" (layers.h: LIVE_SHAPE, LIVE_MAP) — studio.py sends each
//              layer to its place by it and tells what needs a Load (a new layer, another font, a text, a link)
// Only numbers change here: where a layer is, its size, colors, angle, radius, hidden or not. The line comes in loop()
// (Halo's commands); the frame task takes it (live_tick, first in update(): no layer changes in the middle of a frame)
// and wakes the screen. Nothing is kept: a restart shows the firmware's own layers again (Load keeps them).
// Included from PopkaWatch.ino after power.h (pw_cmd_wake).
#pragma once
#ifndef LIVE_MAP  // (a layers.h made before Live)
#define LIVE_SHAPE "0"
#define LIVE_MAP ""
#endif

struct LiveSet {
  uint8_t s, i;
  bool hide;  // "-": only hide it
  float dx, dy, k, p, p2, p3;
  uint32_t c, c2, c3;
  int8_t sp;
  bool hidden;
};
#define LIVE_Q 24  // layers waiting for the next frame (a layer sent again before it — one place, the newest)
static LiveSet live_q[LIVE_Q], live_take[LIVE_Q];
static int live_n;
static portMUX_TYPE live_mux = portMUX_INITIALIZER_UNLOCKED;

static void live_cmd(const char *a) {
  while (*a == ' ') a++;
  if (*a == '?') {
    Serial.printf("LIVE 1 %s %s\n", LIVE_SHAPE, LIVE_MAP);
    return;
  }
  const char *sum_at = strrchr(a, ' ');
  if (!sum_at) return;
  unsigned sum = 0;
  for (const char *c = a; c < sum_at; c++) sum += (uint8_t)*c;
  if ((sum & 255) != strtoul(sum_at + 1, nullptr, 16)) return;  // (garbled)
  char *e;
  const long s = strtol(a, &e, 10), i = strtol(e, &e, 10), kind = strtol(e, &e, 10);
  if (s < 0 || s >= face_scr_n || i < 0 || i >= face_scr[s].n || face_scr[s].layers[i].kind != kind) return;
  LiveSet q{};
  q.s = s, q.i = i;
  while (*e == ' ') e++;
  if (*e == '-') {
    q.hide = true;
  } else {
    q.dx = strtof(e, &e), q.dy = strtof(e, &e), q.k = strtof(e, &e);
    q.c = strtoul(e, &e, 16), q.c2 = strtoul(e, &e, 16), q.sp = (int8_t)strtol(e, &e, 10);
    q.p = strtof(e, &e), q.c3 = strtoul(e, &e, 16), q.p2 = strtof(e, &e), q.p3 = strtof(e, &e);
    char *end;
    const long h = strtol(e, &end, 10);
    if (end == e || !(q.k > 0.05f && q.k < 10) || fabsf(q.dx) > 1000 || fabsf(q.dy) > 1000) return;  // (cut short)
    q.hidden = h != 0;
  }
  portENTER_CRITICAL(&live_mux);
  int j = 0;
  while (j < live_n && !(live_q[j].s == q.s && live_q[j].i == q.i)) j++;
  if (j == live_n && live_n < LIVE_Q) live_n++;
  if (j < live_n) live_q[j] = q;  // (full: dropped — studio.py sends the last state again when the editing stops)
  portEXIT_CRITICAL(&live_mux);
}

// First in update(): the layers sent since the last frame. Built again only when the change needs it (a size, a
// radius, an angle of curved text: face_layer_rebuild); a move or a color — the next layout draws only what changed
static void live_tick() {
  if (!live_n) return;
  portENTER_CRITICAL(&live_mux);
  const int n = live_n;
  memcpy(live_take, live_q, n * sizeof(LiveSet));
  live_n = 0;
  portEXIT_CRITICAL(&live_mux);
  bool all = false;
  for (int j = 0; j < n; j++) {
    const LiveSet &q = live_take[j];
    FaceScreen &S = face_scr[q.s];
    FaceLayer &L = S.layers[q.i];
    const FaceLayer was = L;
    if (q.hide) {
      L.hidden = true;
    } else {
      L.dx = q.dx, L.dy = q.dy, L.k = q.k, L.color = q.c, L.color2 = q.c2, L.spacing = q.sp, L.p = q.p;
      L.color3 = q.c3, L.p2 = q.p2, L.p3 = q.p3, L.hidden = q.hidden;
    }
    if (face_layer_rebuild(was, L)) face_layer_build(S, q.i), all = true;
    if (was.hidden != L.hidden) all = true;
    if (fl_gallery(L.kind)) gal_rebuild(S), all = true;  // (the list: its cards' heights, a card hidden)
  }
  if (all) halo_damage_all();
  pw_cmd_wake = true;  // (asleep or dim: awake to show it; awake — idle from now)
}

static void live_build() {
  halo_command("live", live_cmd);
}
