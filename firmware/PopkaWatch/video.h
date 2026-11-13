// video.h — the Video app (FL_VIDEO), for a rectangular screen: two screens, variants B4 and P1 of web/mockups/rect-video.html.
// The tiles: three in a row, the title and the length on the picture, the list scrolls. A tap — the tile grows into the
// player (the small picture gives way to the clip's first frame on the way, so the video goes on from what is seen).
// The player: the video, the time line and one button: a tap — stop / play; held or moved up and down — the volume, a ring
// round the button. A tap elsewhere shows or hides them (hidden by themselves after 3 s); a swipe from the left edge or down
// — back to the tiles.
//
// The transitions are the interface's ordinary 60 Hz frames. While the video plays the frame engine decodes it into the
// frame buffer (halo_under) and only what lies over the picture goes through the usual drawing: nothing at all once the
// controls hide. The hardware half: video_hw.h. Included from settings.h.

extern const HaloFont gl_r_title, gl_r_t28, gl_r_t32, gl_r_v24, gl_r_n21, gl_r_n24, gl_r_n26, gl_r_n40, gl_r_ic64;
#include "video_hw.h"

#define VT_X0 32          // the tiles
#define VT_Y0 92
#define VT_DX 326
#define VT_DY 248
#define VT_W 308
#define VT_H 232
#define VT_R 24.0f
#define VT_TITLE_Y 45.0f  // the middle of the letters
#define VT_CLOCK_Y 42.0f
#define VT_SLOTS 12       // text items: no more tiles than that are seen at once
#define VP_TITLE_Y 43.0f
#define VP_BTN_X 512.0f
#define VP_BTN_Y 300.0f
#define VP_BTN_R 72.0f
#define VP_RING_R 92.0f
#define VP_RING_W 10.0f
#define VP_NUM_Y 434.0f
#define VP_TIME_Y 533.0f
#define VP_LINE_X 40.0f
#define VP_LINE_W 944.0f
#define VP_LINE_Y 559.0f
#define VP_SCRIM_Y 430    // the dark under the time line, from here down to .72
#define VP_TOP_H 76       // and under the title
#define VP_HIDE_S 3.0f
#define VP_HOME_ZONE 70.0f  // up from this close to the bottom — to the watch face (lib/Halo: OVER_ZONE of halo_nav.cpp)
#define VP_PLATE 0x141619  // the button's plate
#define VS_MARKS 3         // a stream's tiles with the green frame and the badge (they come first: S3, web/mockups/rect-video-stream.html)
#define VS_GREEN 0x8FD3A6
#define VP_TEXT 0xFFF8EE
#define VP_DIM 0xD6D6DA
#define VP_ICON_PLAY "\xEF\x81\x8B"
#define VP_ICON_PAUSE "\xEF\x81\x8C"
#define VP_ICON_VOL "\xEF\x80\xA8"

enum { VM_LIB, VM_OPEN, VM_PLAY, VM_HOLD, VM_CLOSE };  // (VM_HOLD: asked the frame task for the last picture)
static int vd_mode = VM_LIB, vd_sel = -1, vd_armed = -1, vd_pop;
static HaloSpring vd_open_s, vd_ctl, vd_pp, vd_vr, vd_press;
static HaloSpringK K_VD_OPEN, K_VD_FAST;
static HaloList vd_list;
static int vd_list_n = -1;
static float vd_scroll, vd_hide_in, vd_vol = 0.62f, vd_vol_from, vd_vol_y0, vd_seek_at = -1;
static float vd_mid_title, vd_mid_t, vd_mid_n24, vd_mid_n26, vd_mid_n21, vd_mid_n40, vd_mid_ic, vd_mid_v, vd_mid_e;
static bool vd_on_top, vd_vol_on, vd_btn, vd_line, vd_free;
static bool vd_away;  // the player on its way into Always On or dark, or there (vd_away_check)
static bool vd_line_wait;  // a finger on the time line by the bottom edge: up or sideways not known yet
static uint32_t vd_paused_at;
static int vd_tr_n, vd_tr_long;  // the transition's frames, for the log
static float vd_tr_max;
static uint32_t vd_tr_mask;
static uint32_t vd_lib_h, vd_ctl_h;
static bool vd_ctl_vis;
static HaloView vd_v;
static char vd_clock[8], vd_t0[10], vd_t1[10], vd_num[6];
static int vd_clock_min = -1;
struct VdUi {
  HaloItem title, clock, empty, empty2;
  HaloItem t[VT_SLOTS], d[VT_SLOTS];
  HaloItem sglow[VS_MARKS], sring[VS_MARKS], spill[VS_MARKS], sdot[VS_MARKS], sw1[VS_MARKS], sw2[VS_MARKS], swd[VS_MARKS], stxt[VS_MARKS];
  HaloItem name, cur, len, track, fill, knob, plate, ic[3], ring, arc, num;
};
static VdUi *vu;

static inline uint16_t vd_mix(uint16_t d, uint16_t s, int a) {  // a 0..32
  const uint32_t x = (d | (uint32_t)d << 16) & 0x07E0F81Fu, y = (s | (uint32_t)s << 16) & 0x07E0F81Fu;
  const uint32_t z = (x + ((y - x) * a >> 5)) & 0x07E0F81Fu;
  return (uint16_t)(z | z >> 16);
}

// n pixels of a row, nearest, from the source position u (16.16) on, step du. Near 1:1 (a picture a few per cent smaller
// or bigger: Always On, the start of a transition) as runs of consecutive source pixels, each a plain copy: a pixel at a
// time it was twice as slow — every one a load of its own out of PSRAM (24 ms a frame of the whole screen)
static inline void HALO_HOT vd_row_copy(uint16_t *d, const uint16_t *s, int n, int32_t u, int32_t du) {
  if (du == 65536) return (void)memcpy(d, s + (u >> 16), n * 2);
  if (du > 65536 && du < 72000) {  // (one source pixel skipped every run)
    const int32_t over = du - 65536;
    while (n > 0) {
      const int run = min(n, (int)((65536 - (u & 0xFFFF) + over - 1) / over));
      memcpy(d, s + (u >> 16), run * 2);
      d += run, n -= run, u += run * du;
    }
    return;
  }
  if (du < 65536 && du > 59000) {  // (one doubled)
    const int32_t under = 65536 - du;
    while (n > 0) {
      const int run = min(n, (int)((u & 0xFFFF) / under) + 1);
      memcpy(d, s + (u >> 16), run * 2);
      d += run, n -= run, u += run * du;
    }
    return;
  }
  for (int x = 0; x < n; x++, u += du) d[x] = s[u >> 16];
}

// A picture into its part of the band: the window (sx, sy, sw, sh) of src onto the rectangle with rounded corners, nearest
// pixel. a — how much of it over what is there (0..256). Pixel for pixel — a plain copy. coarse — every source row for two
// rows of the screen (the second comes from the cache): half the reading of PSRAM, for a picture that fades away
static void HALO_HOT vd_blit(const uint16_t *src, int stride, int rows, float sx, float sy, float sw, float sh, float x0, float y0, float x1, float y1, float r, int a, bool on_black = false, bool coarse = false) {
  if (a <= 2 || x1 <= x0 || y1 <= y0 || !halo_hits(x0, y0, x1, y1)) return;
  const int ya = max(hb.y, halo_floor(y0)), yb = min(hb.y + hb.rows - 1, halo_floor(y1 - 0.001f));
  const float ky = sh / (y1 - y0), kx = sw / (x1 - x0);
  const int32_t du = (int32_t)(kx * 65536 + 0.5f);
  const int a32 = (a + 4) >> 3;
  for (int y = ya; y <= yb; y++) {
    const float yc = y + 0.5f, dd = fmaxf(y0 + r - yc, yc - (y1 - r));
    float ins = 0;
    if (dd > 0 && r > 0) {
      if (dd >= r) continue;
      ins = r - halo_sqrt(r * r - dd * dd);
    }
    const float xa = x0 + ins, xb = x1 - ins;
    const int ia = -halo_floor(-xa), ib = halo_floor(xb) - 1;
    uint16_t *row = halo_row(y);
    const int line = constrain((int)(sy + ((coarse ? y & ~1 : y) + 0.5f - y0) * ky), 0, rows - 1);
    const uint16_t *s = src + line * stride;
    const int p0 = max(ia, hb.x0), p1 = min(ib, hb.x1);
    if (p0 <= p1) {
      int32_t u = (int32_t)((sx + (p0 + 0.5f - x0) * kx) * 65536);
      const int n = p1 - p0 + 1;
      if (a32 >= 32) {
        vd_row_copy(row + p0, s, n, u, du);
      } else if (on_black) {  // (fading on black needs no look at what is underneath: half the work)
#ifdef HALO_FAST_VEIL  // the picture as it is, then black over it, 8 pixels at a time: 13 cycles a pixel -> ~5
        vd_row_copy(row + p0, s, n, u, du);
        halo_fast_veil(row + p0, n, 0, 256 - a);
#else
        for (int x = p0; x <= p1; x++, u += du) {
          const uint32_t c = s[u >> 16], z = (((c | c << 16) & 0x07E0F81Fu) * a32 >> 5) & 0x07E0F81Fu;
          row[x] = (uint16_t)(z | z >> 16);
        }
#endif
      } else {
        for (int x = p0; x <= p1; x++, u += du) row[x] = vd_mix(row[x], s[u >> 16], a32);
      }
    }
    if (ins > 0) {  // the soft edge of the corners
      const int la = ia - 1, rb = ib + 1;
      if (la >= hb.x0 && la <= hb.x1) {
        const int u = constrain((int)(sx + (la + 0.5f - x0) * kx), 0, stride - 1);
        row[la] = vd_mix(row[la], s[u], (int)((ia - xa) * a32));
      }
      if (rb >= hb.x0 && rb <= hb.x1) {
        const int u = constrain((int)(sx + (rb + 0.5f - x0) * kx), 0, stride - 1);
        row[rb] = vd_mix(row[rb], s[u], (int)((xb - rb) * a32));
      }
    }
  }
}

// Two pictures on one rectangle with rounded corners, b over a at level k (0..256), in one pass: the small picture
// giving way to the clip's frame. Drawn as two layers, every pixel was written, read back and mixed (27 cycles; here 15)
struct VdPic {
  const uint16_t *px;
  int stride, rows;
  float sx, sy, sw, sh;  // the window of it that goes onto the rectangle
};
static void HALO_HOT vd_cross(const VdPic &A, const VdPic &B, float x0, float y0, float x1, float y1, float r, int k) {
  if (x1 <= x0 || y1 <= y0 || !halo_hits(x0, y0, x1, y1)) return;
  const int ya = max(hb.y, halo_floor(y0)), yb = min(hb.y + hb.rows - 1, halo_floor(y1 - 0.001f));
  const float kax = A.sw / (x1 - x0), kay = A.sh / (y1 - y0), kbx = B.sw / (x1 - x0), kby = B.sh / (y1 - y0);
  const int32_t dua = (int32_t)(kax * 65536 + 0.5f), dub = (int32_t)(kbx * 65536 + 0.5f);
  const uint32_t kb = (constrain(k, 0, 256) + 4) >> 3, ka = 32 - kb;
  for (int y = ya; y <= yb; y++) {
    const float yc = y + 0.5f, dd = fmaxf(y0 + r - yc, yc - (y1 - r));
    float ins = 0;
    if (dd > 0 && r > 0) {
      if (dd >= r) continue;
      ins = r - halo_sqrt(r * r - dd * dd);
    }
    const float xa = x0 + ins, xb = x1 - ins;
    const int ia = -halo_floor(-xa), ib = halo_floor(xb) - 1;
    uint16_t *row = halo_row(y);
    const uint16_t *sa = A.px + constrain((int)(A.sy + (yc - y0) * kay), 0, A.rows - 1) * A.stride;
    const uint16_t *sb = B.px + constrain((int)(B.sy + (yc - y0) * kby), 0, B.rows - 1) * B.stride;
    auto both = [&](uint32_t c, uint32_t d) {  // (the two in one multiply each: the channels apart, as in vd_mix)
      const uint32_t z = ((((c | c << 16) & 0x07E0F81Fu) * ka + ((d | d << 16) & 0x07E0F81Fu) * kb) >> 5) & 0x07E0F81Fu;
      return (uint16_t)(z | z >> 16);
    };
    auto at = [&](int x) {
      const float f = x + 0.5f - x0;
      return both(sa[constrain((int)(A.sx + f * kax), 0, A.stride - 1)], sb[constrain((int)(B.sx + f * kbx), 0, B.stride - 1)]);
    };
    const int p0 = max(ia, hb.x0), p1 = min(ib, hb.x1);
    if (p0 <= p1) {
      int32_t ua = (int32_t)((A.sx + (p0 + 0.5f - x0) * kax) * 65536), ub = (int32_t)((B.sx + (p0 + 0.5f - x0) * kbx) * 65536);
      for (int x = p0; x <= p1; x++, ua += dua, ub += dub) row[x] = both(sa[ua >> 16], sb[ub >> 16]);
    }
    if (ins > 0) {  // the soft edge of the corners
      const int la = ia - 1, rb = ib + 1;
      if (la >= hb.x0 && la <= hb.x1) row[la] = vd_mix(row[la], at(la), (int)((ia - xa) * 32));
      if (rb >= hb.x0 && rb <= hb.x1) row[rb] = vd_mix(row[rb], at(rb), (int)((xb - rb) * 32));
    }
  }
}

static void vd_ctl_repaint();
static void video_build() {
  vu = new (heap_caps_malloc(sizeof(VdUi), MALLOC_CAP_SPIRAM)) VdUi();
  K_VD_OPEN = halo_spring(0.36f, 1.0f), K_VD_FAST = halo_spring(0.16f, 1.0f);
  vd_mid_title = halo_text_mid(gl_r_title, "Videos"), vd_mid_t = halo_text_mid(gl_r_t28, "Video");
  vd_mid_n24 = halo_text_mid(gl_r_n24, "0123456789"), vd_mid_n26 = halo_text_mid(gl_r_n26, "0123456789");
  vd_mid_n21 = halo_text_mid(gl_r_n21, "0123456789"), vd_mid_n40 = halo_text_mid(gl_r_n40, "0123456789");
  vd_mid_ic = halo_text_mid(gl_r_ic64, VP_ICON_VOL), vd_mid_v = halo_text_mid(gl_r_v24, "Video"), vd_mid_e = halo_text_mid(gl_r_t32, "No videos");
  vh_commands();
  vh_on_picture = vd_ctl_repaint;
#ifdef __EMSCRIPTEN__
  vh_begin();  // (the editor's page: its made-up clips at once, for the screen's picture)
#endif
}

static void video_enter() {
  vh_begin();
  vd_armed = -1;
  if (vd_mode == VM_LIB) vd_sel = -1;
  halo_damage_all();
}

// ---------- Open and close ----------
static int vd_ready_i = -1;
static void vd_prepare(int i) {
  vh_open(i), vd_ready_i = i;
}
static void vd_unprepare() {
  if (vd_ready_i >= 0 && vd_mode == VM_LIB) vh_close(), vd_ready_i = -1;
}
static void vd_open_clip(int i) {
  if (i < 0 || i >= vh_n) return;
#if HAS_MUSIC
  if (pl_playing) pl_set_playing(false);  // (one speaker)
#endif
  vd_sel = i, vd_mode = VM_OPEN, vd_open_s.target = 1, vd_vr.target = 0, vd_vol_on = false;
  halo_spring_set(vd_ctl, 0), halo_spring_set(vd_pp, 1);
  vh_volume(vd_vol);
  if (vd_ready_i != i) vh_open(i);
  vd_ready_i = -1;
}
static void vd_close_clip() {
  if (vd_mode != VM_PLAY) return;
  vd_tr_n = vd_tr_long = 0, vd_tr_max = 0, vd_tr_mask = 0;
  vd_mode = VM_HOLD;  // (the controls stay as they are till the close starts, and go with it: nothing over the picture
  vh_stop();          // changes, so the stop needs no new picture under them)
}
static void vd_show_ctl() {
  vd_ctl.target = 1, vd_hide_in = VP_HIDE_S;
}

// Every frame, whatever is on screen
static void video_tick(float dt) {
  halo_spring_run(vd_open_s, K_VD_OPEN, dt);
  halo_spring_run(vd_ctl, K_VD_FAST, dt);
  halo_spring_run(vd_vr, K_VD_FAST, dt);
  halo_spring_run(vd_pp, K_VD_FAST, dt);
  halo_spring_run(vd_press, K_VD_FAST, dt);
  vh_tick(dt);
  if (vd_mode == VM_OPEN || vd_mode == VM_CLOSE) { if (dt > 0.024f && vd_tr_n < 32) vd_tr_mask |= 1u << vd_tr_n; vd_tr_n++, vd_tr_long += dt > 0.024f, vd_tr_max = fmaxf(vd_tr_max, dt); }
  if (vd_mode != VM_LIB && !vd_on_top) {  // the screen went away under the player
    LOG("video: the screen went away\n");
    vh_abort();
    vd_mode = VM_LIB, vd_sel = -1;
    halo_spring_set(vd_open_s, 0), halo_spring_set(vd_ctl, 0);
    halo_damage_all();
  }
  vd_on_top = false;
  if (vd_mode == VM_OPEN && vd_open_s.x > 0.995f && vh_still_ok && vh_ready()) {
    halo_spring_set(vd_open_s, 1);
    LOG("video: opened in %d frames, %d of them long (longest %.1f ms, which %08lx)\n", vd_tr_n, vd_tr_long, vd_tr_max * 1000, (unsigned long)vd_tr_mask);
    vd_tr_n = vd_tr_long = 0, vd_tr_max = 0, vd_tr_mask = 0;
    vd_mode = VM_PLAY;
    vd_paused_at = 0;
    vh_start();
    vd_show_ctl();
  }
  // (the screen below still holds the video's pictures: the first frames of the close are drawn whole, after that only
  // what moves)
  if (vd_mode == VM_HOLD && vh_stopped()) vd_mode = VM_CLOSE, vd_open_s.target = 0, halo_spring_set(vd_ctl, 0), halo_spring_set(vd_vr, 0), halo_damage_all();
  if (vd_mode == VM_CLOSE && vd_open_s.x < 0.004f) {
    halo_spring_set(vd_open_s, 0);
    LOG("video: closed in %d frames, %d of them long (longest %.1f ms, which %08lx)\n", vd_tr_n, vd_tr_long, vd_tr_max * 1000, (unsigned long)vd_tr_mask);
    vd_tr_n = vd_tr_long = 0, vd_tr_max = 0, vd_tr_mask = 0;
    vh_close();
    vd_mode = VM_LIB, vd_sel = -1;
    halo_damage_all();
  }
  if (vd_mode == VM_PLAY) {
    vd_pp.target = vh_playing() || (!vh_ended() && !vd_paused_at) ? 1 : 0;
    if (!halo_display_is_on() && vh_playing()) vh_set_playing(false), vd_paused_at = halo_ms() | 1;
    if (vh_playing() && !vd_vol_on && !vd_line) {
      vd_hide_in -= dt;
      if (vd_hide_in <= 0) vd_ctl.target = 0;
    }
    if (vh_ended() && !vd_paused_at) vd_paused_at = halo_ms() | 1, vd_ctl.target = 1;
    // (left paused: not forever; in Always On it waits. Signed: vd_paused_at is the time | 1 — 1 ms ahead on an even
    // millisecond, and set in this same tick at the clip's end the unsigned difference was 4e9: every other clip closed
    // by itself the moment it ended)
    if (vd_paused_at && !vd_away && (int32_t)(halo_ms() - vd_paused_at) > 120000) vd_close_clip();
  }
}

// Open or going there: not idle (power.h keeps Always On away)
static bool video_busy() {
  return vd_mode != VM_LIB;
}

// ---------- The finger ----------
static void vd_lib_update(float dt) {
  HaloFinger &f = halo_finger;
  const int n = vh_n;
  if (n != vd_list_n) {
    const int rows = (n + 2) / 3;
    const float page = VT_Y0 + rows * VT_DY + 12;
    const float heights[1] = {page};
    halo_list_init(vd_list, heights, 1, 0, HALO_H, false);
    vd_list_n = n;
  }
  halo_list_update(vd_list, dt);
  vd_scroll = vd_list.scroll;
  if (f.pressed) {
    vd_armed = -1;
    const int sc = (int)lroundf(vd_scroll);
    for (int i = 0; i < n && vd_armed < 0; i++) {
      const int x = VT_X0 + i % 3 * VT_DX, y = VT_Y0 + i / 3 * VT_DY + sc;
      if (f.x0 >= x && f.x0 <= x + VT_W && f.y0 >= y && f.y0 <= y + VT_H) vd_armed = i;
    }
    vd_pop = vd_armed;
    if (vd_armed >= 0) vd_prepare(vd_armed);  // (ready before the finger is up: the transition has nothing else to wait for)
  }
  if (f.dragging && vd_armed >= 0) vd_armed = -1, vd_unprepare();
  vd_press.target = vd_armed >= 0 && f.down ? 1 : 0;
  if (f.tap && vd_armed >= 0) vd_open_clip(vd_armed);
  else if (!f.down && vd_armed >= 0) vd_unprepare();
  if (!f.down) vd_armed = -1;
}

static void vd_player_update(float) {
  HaloFinger &f = halo_finger;
  const bool seen = vd_ctl.x > 0.4f;
  if (f.pressed) {
    // The gestures of the interface under the video are not for now — but for the one up from the bottom edge (to the
    // watch face, as from any app: lib/Halo's home gesture) and the time line there: a finger down near the bottom is
    // not taken yet. Up — the face's; sideways — the time line's (vd_line_wait). Near the left edge too — the player's
    // own way back
    const bool edge = f.y0 > HALO_H - VP_HOME_ZONE;
    if (!edge || f.x0 < 70) halo_finger_claim(&vd_mode);
    const float dx = f.x0 - VP_BTN_X, dy = f.y0 - VP_BTN_Y;
    vd_btn = seen && dx * dx + dy * dy < (VP_BTN_R + 16) * (VP_BTN_R + 16);
    vd_line = seen && !vd_btn && fabsf(f.y0 - VP_LINE_Y) < 34 && f.x0 > VP_LINE_X - 18 && f.x0 < VP_LINE_X + VP_LINE_W + 18;
    vd_line_wait = vd_line && edge && f.x0 >= 70;
    vd_free = !vd_btn && !vd_line;
    vd_vol_on = false;
    if (seen) vd_hide_in = VP_HIDE_S;
  }
  if (vd_line_wait) {  // a finger on the time line by the bottom edge: which way it goes decides whose it is
    if (f.dragging && f.horiz && halo_finger_claim(&vd_mode)) vd_line_wait = false;  // sideways — the time line
    else if (f.dragging || (!f.down && !f.tap)) vd_line = vd_line_wait = false;  // up — the face's (lib/Halo took it)
    else if (!f.down) vd_line_wait = false;  // a tap on the line: the place under the finger
    else return;
  }
  if (vd_btn && f.down) {  // held, or moved up and down: the volume
    if (!vd_vol_on && (halo_ms() - f.t0 > 320 || fabsf(f.y - f.y0) > 14)) {
      vd_vol_on = true, vd_vol_from = vd_vol, vd_vol_y0 = f.y, vd_vr.target = 1;
    }
    if (vd_vol_on) {
      vd_vol = halo_clamp01(vd_vol_from + (vd_vol_y0 - f.y) / 260);
      vh_volume(vd_vol);
      vd_hide_in = VP_HIDE_S;
    }
  }
  if (vd_btn && !f.down) {
    if (!vd_vol_on && f.tap) {
      const bool on = !vh_playing() && !(vd_pp.target > 0.5f && !vh_ended());
      vd_paused_at = on ? 0 : halo_ms() | 1;
      vh_set_playing(on);
      vd_show_ctl();
    }
    vd_btn = false, vd_vol_on = false, vd_vr.target = 0;
    vd_hide_in = VP_HIDE_S;
  }
  if (vd_line) {
    if (f.down) {
      vd_seek_at = halo_clamp01((f.x - VP_LINE_X) / VP_LINE_W);
      vh_seek(vd_seek_at, false);
      vd_hide_in = VP_HIDE_S;
    } else {
      if (vd_seek_at < 0) vd_seek_at = halo_clamp01((f.x - VP_LINE_X) / VP_LINE_W);  // (a tap that waited: never seeking)
      vh_seek(vd_seek_at, true);
      vd_paused_at = vd_paused_at && !vh_ended() ? vd_paused_at : 0;
      vd_seek_at = -1, vd_line = false;
    }
  }
  if (vd_free) {
    if (f.tap) {
      if (vd_ctl.target > 0.5f) vd_ctl.target = 0;
      else vd_show_ctl();
    }
    const float mx = f.x - f.x0, my = f.y - f.y0;
    if (f.down && ((f.x0 < 70 && mx > 80 && fabsf(my) < mx) || (my > 110 && fabsf(mx) < my))) vd_close_clip(), vd_free = false;
    if (!f.down) vd_free = false;
  }
}

static void video_update(float dt) {
  if (vd_mode == VM_LIB) vd_lib_update(dt);
  else if (vd_mode == VM_PLAY) vd_player_update(dt);
  else if (halo_finger.pressed) halo_finger_claim(&vd_mode);
}

// ---------- Where everything stands ----------
static void vd_text(HaloItem &it, const HaloFont &f, const char *s, float mid, int align, float x, float y, uint32_t c, float a, const HaloView &v, bool black = false) {
  rg_text(it, f, s, mid, align, x, y, c, a, black ? &halo_black : nullptr);
  it.on_black = black;
  halo_view_commit(it, v);
}

static void vd_shape(HaloItem &it, uint8_t kind, float x, float y, float w, float h, float r, uint32_t c, float a, const HaloView &v, bool black = false) {
  it.kind = kind, it.x = x, it.y = y, it.w = w, it.h = h, it.r = r, it.color = c, it.alpha = a, it.scale = 1, it.on_black = black, it.on = nullptr;
  halo_view_commit(it, v);
}

// A tile from a stream (S3): a green frame with a glow round it, and over its picture the badge — a green dot, Wi-Fi, Stream
static void vd_stream_marks(int s, int i, int x0, int y0, float a, const HaloView &v) {
  const bool on = i >= 0 && a > 0;
  const float k = on ? a : 0, cx = x0 + VT_W / 2.0f, cy = y0 + VT_H / 2.0f, bx = x0 + 14, by = y0 + 30;
  vd_shape(vu->sglow[s], HI_RRECT, cx, cy, VT_W + 18, VT_H + 18, VT_R + 9, VS_GREEN, 0.2f * k, v, true);
  vd_shape(vu->sring[s], HI_RRECT, cx, cy, VT_W + 6, VT_H + 6, VT_R + 3, VS_GREEN, k, v, true);
  vd_shape(vu->spill[s], HI_RRECT, bx + 69, by, 138, 32, 16, 0x000000, 0.62f * k, v);
  vd_shape(vu->sdot[s], HI_CIRCLE, bx + 15, by, 0, 0, 5, VS_GREEN, k, v);
  for (HaloItem *w : {&vu->sw1[s], &vu->sw2[s]}) {
    w->a0 = -0.8f, w->a1 = 0.8f;
    vd_shape(*w, HI_ARC, bx + 38, by + 6, 2.2f, 0, w == &vu->sw1[s] ? 11 : 6.5f, VP_TEXT, k, v);
  }
  vd_shape(vu->swd[s], HI_CIRCLE, bx + 38, by + 6, 0, 0, 1.8f, VP_TEXT, k, v);
  vd_text(vu->stxt[s], gl_r_v24, "Stream", vd_mid_v, 0, bx + 56, by, VP_TEXT, k, v);
}

static inline float vd_lib_alpha() {
  return 1 - halo_clamp01(halo_clamp01(vd_open_s.x) / 0.2f);  // (gone while the tile is small: the two together are the most expensive frames)
}

// The last few pixels of the way are not worth a scaled frame: the whole picture, pixel for pixel
static inline float vd_snap(float x) {
  return x >= 0.985f ? 1 : halo_clamp01(x);
}

// Where the tile growing into the screen (or shrinking into its place) is at p: x0, y0, x1, y1; false — not drawn
static bool vd_grow_rect(float p, float *r) {
  if (p <= 0.001f || vd_sel < 0) return false;
  const int sc = (int)lroundf(vd_scroll), tx = VT_X0 + vd_sel % 3 * VT_DX, ty = VT_Y0 + vd_sel / 3 * VT_DY + sc;
  r[0] = tx + (0 - tx) * p, r[1] = ty + (VH_TOP - ty) * p, r[2] = tx + VT_W + (W - tx - VT_W) * p, r[3] = ty + VT_H + (VH_TOP + VH_H - ty - VT_H) * p;
  return true;
}
static float vd_grow_r[4];
static bool vd_grow_was;
static int vd_lib_la = -1, vd_lib_pr = -1, vd_lib_pop = -2;

static void vd_lib_layout(const HaloView &v) {
  const float a = vd_lib_alpha() * v.a;
  const int n = vh_n, sc = (int)lroundf(vd_scroll);
  vd_text(vu->title, gl_r_title, "Videos", vd_mid_title, 0, VT_X0, VT_TITLE_Y + sc, VP_TEXT, a, v, true);
  time_t now = time(nullptr);
  struct tm t;
  localtime_r(&now, &t);
  // (by the hour and the minute: the minute alone kept a night's 01:01 at 09:01)
  if (t.tm_hour * 60 + t.tm_min != vd_clock_min) vd_clock_min = t.tm_hour * 60 + t.tm_min, snprintf(vd_clock, sizeof vd_clock, "%02d:%02d", fc_shown_hour(t.tm_hour), t.tm_min);
  vd_text(vu->clock, gl_r_n26, vd_clock, vd_mid_n26, 2, W - 32, VT_CLOCK_Y + sc, 0x9A9A9F, a, v, true);
  const bool none = !n && !vh_scanning && vd_list_n >= 0;
  vd_text(vu->empty, gl_r_t32, "No videos", vd_mid_e, 1, CX, CY - 22, VP_TEXT, none ? a : 0, v, true);
  vd_text(vu->empty2, gl_r_v24, "Add them in PopkaStudio", vd_mid_v, 1, CX, CY + 26, 0x9A9A9F, none ? a : 0, v, true);
  bool used[VT_SLOTS] = {};
  for (int i = 0; i < n; i++) {
    const int y0 = VT_Y0 + i / 3 * VT_DY + sc, x0 = VT_X0 + i % 3 * VT_DX;
    if (y0 + VT_H <= 0 || y0 >= HALO_H) continue;
    const int s = i % VT_SLOTS;
    used[s] = true;
    const float ta = i == vd_sel ? 0 : a, py = y0 + VT_H - 25;
    const VhFile &F = vh_files[i];
    vd_text(vu->t[s], gl_r_v24, F.title, vd_mid_v, 0, x0 + 14, py, VP_TEXT, ta, v);
    vd_text(vu->d[s], gl_r_n21, F.dur, vd_mid_n21, 2, x0 + VT_W - 14, py + 1, VP_DIM, ta, v);
  }
  for (int s = 0; s < VT_SLOTS; s++)
    if (!used[s]) {
      vu->t[s].alpha = vu->d[s].alpha = 0;
      halo_view_commit(vu->t[s], v), halo_view_commit(vu->d[s], v);
    }
  for (int i = 0; i < VS_MARKS; i++) {
    const int y0 = VT_Y0 + i / 3 * VT_DY + sc, x0 = VT_X0 + i % 3 * VT_DX;
    const bool shown = i < n && vh_files[i].net && i != vd_sel && y0 + VT_H > -20 && y0 < HALO_H + 20;
    vd_stream_marks(i, shown ? i : -1, x0, y0, shown ? a : 0, v);
  }
  // The whole screen moved (scrolled, the view, the list) — all of it again. Otherwise only what moves: the tiles
  // fading (or one pressed) and the tile growing or shrinking, the black around them stays as it is. The whole screen
  // every frame of the close was what made its end cost two refreshes a frame (boards/p4lcd7/p4-video.md)
  uint32_t h = 2166136261u;
  for (float q : {(float)sc, v.x, v.y, v.s * 1024, v.a * 256, (float)n, (float)vd_sel}) h = (h ^ (uint32_t)(int32_t)q) * 16777619u;
  const int la = (int)(vd_lib_alpha() * 256), pr = (int)(vd_press.x * 256);
  float g[4] = {};
  const bool grow = vd_grow_rect(vd_snap(vd_open_s.x), g);
  if (h != vd_lib_h) {
    halo_damage_all();
  } else {
    if (la != vd_lib_la || pr != vd_lib_pr || vd_pop != vd_lib_pop) {
      const int rows = (n + 2) / 3;
      const float y0 = VT_Y0 + sc, y1 = y0 + (rows - 1) * VT_DY + VT_H;
      halo_damage_f(halo_view_x(v, VT_X0), halo_view_y(v, y0), halo_view_x(v, VT_X0 + 2 * VT_DX + VT_W), halo_view_y(v, y1));
    }
    if (grow != vd_grow_was || memcmp(g, vd_grow_r, sizeof g)) {
      if (vd_grow_was) halo_damage_f(vd_grow_r[0], vd_grow_r[1], vd_grow_r[2], vd_grow_r[3]);
      if (grow) halo_damage_f(g[0], g[1], g[2], g[3]);
    }
  }
  vd_lib_h = h, vd_lib_la = la, vd_lib_pr = pr, vd_lib_pop = vd_pop, vd_grow_was = grow;
  memcpy(vd_grow_r, g, sizeof g);
}

// Everything over the video: the dark under the title and under the time line, the button with its ring and number
static void vd_ctl_mark() {
  halo_damage(0, 0, W - 1, VP_TOP_H);
  halo_damage(0, VP_SCRIM_Y, W - 1, HALO_H - 1);
  halo_damage((int)(VP_BTN_X - VP_RING_R - 12), (int)(VP_BTN_Y - VP_RING_R - 12), (int)(VP_BTN_X + VP_RING_R + 12), (int)VP_NUM_Y + 26);
}
// A whole new picture under them (halo_under, a new video frame or the same one again): each frame of that kind paints
// over it only what is marked, so all of them are marked — not only the item that changed (its letters alone over the
// picture, without the dark and the button's plate: they blinked when the controls came up, and the plate was gone after
// a tap on pause)
static void vd_ctl_repaint() {
  if (vd_ctl_vis) vd_ctl_mark();
}

static void vd_ctl_layout(const HaloView &v) {
  const float k = halo_clamp01(vd_ctl.x), ka = k * v.a, vr = halo_clamp01(vd_vr.x);
  const bool on = vd_mode == VM_PLAY || vd_mode == VM_HOLD;
  if (on && vd_sel >= 0) {
    const float len = vh_length(), now = vd_seek_at >= 0 ? vd_seek_at * len : vh_position(), f = len > 0 ? halo_clamp01(now / len) : 0, fx = VP_LINE_W * f;
    const int e = (int)now, l = (int)(len + 0.5f);
    snprintf(vd_t0, sizeof vd_t0, "%d:%02d", e / 60, e % 60), snprintf(vd_t1, sizeof vd_t1, "%d:%02d", l / 60, l % 60);
    const int pct = (int)lroundf(vd_vol * 100);
    snprintf(vd_num, sizeof vd_num, "%d", pct);
    const char *title = vh_files[vd_sel].title;
#if VH_UNDER && FN_WIFI
    if (vh_files[vd_sel].net) {  // a clip from a computer (the streaming bench): how the stream goes, beside its name
      static char line[128];
      char st[80], next[128];
      vn_status(st, sizeof st);
      snprintf(next, sizeof next, "%s    %s", title, st);
      if (strcmp(next, line)) strlcpy(line, next, sizeof line), vd_ctl_mark();
      title = line;
    }
#endif
    vd_text(vu->name, gl_r_t28, title, vd_mid_t, 0, 40, VP_TITLE_Y, VP_TEXT, ka, v);
    vd_text(vu->cur, gl_r_n24, vd_t0, vd_mid_n24, 0, VP_LINE_X, VP_TIME_Y, VP_TEXT, ka, v);
    vd_text(vu->len, gl_r_n24, vd_t1, vd_mid_n24, 2, VP_LINE_X + VP_LINE_W, VP_TIME_Y, VP_DIM, ka, v);
    HaloItem &tr = vu->track, &fi = vu->fill, &kn = vu->knob, &pl = vu->plate;
    tr.kind = HI_RRECT, tr.x = VP_LINE_X + VP_LINE_W / 2, tr.y = VP_LINE_Y, tr.w = VP_LINE_W, tr.h = 6, tr.r = 3, tr.scale = 1, tr.color = 0xFFFFFF, tr.alpha = 0.26f * ka, tr.on_black = false, tr.on = nullptr;
    const float fw = fmaxf(6, fx);
    fi.kind = HI_RRECT, fi.x = VP_LINE_X + fw / 2, fi.y = VP_LINE_Y, fi.w = fw, fi.h = 6, fi.r = 3, fi.scale = 1, fi.color = VP_TEXT, fi.alpha = ka, fi.on_black = false, fi.on = nullptr;
    kn.kind = HI_CIRCLE, kn.x = VP_LINE_X + fx, kn.y = VP_LINE_Y, kn.r = vd_seek_at >= 0 ? 14 : 11, kn.scale = 1, kn.color = VP_TEXT, kn.alpha = ka, kn.on_black = false, kn.on = nullptr;
    pl.kind = HI_CIRCLE, pl.x = VP_BTN_X, pl.y = VP_BTN_Y, pl.r = VP_BTN_R, pl.scale = 1, pl.color = VP_PLATE, pl.alpha = 0.6f * ka, pl.on_black = false, pl.on = nullptr;
    halo_view_commit(tr, v), halo_view_commit(fi, v), halo_view_commit(kn, v), halo_view_commit(pl, v);
    const float pp = halo_clamp01(vd_pp.x);
    const float A[3] = {(1 - vr) * pp, (1 - vr) * (1 - pp), vr};
    static const char *const G[3] = {VP_ICON_PAUSE, VP_ICON_PLAY, VP_ICON_VOL};
    for (int i = 0; i < 3; i++) gl_text_mid(vu->ic[i], gl_r_ic64, G[i], vd_mid_ic, VP_BTN_X + (i == 1 ? 4 : 0), VP_BTN_Y, 1, VP_TEXT, ka * A[i], nullptr), vu->ic[i].on_black = false, halo_view_commit(vu->ic[i], v);
    HaloItem &ri = vu->ring, &ar = vu->arc;
    ri.kind = HI_RING, ri.x = VP_BTN_X, ri.y = VP_BTN_Y, ri.r = VP_RING_R, ri.w = VP_RING_W, ri.v = 1, ri.scale = 1, ri.color = 0xFFFFFF, ri.color2 = 0xFFFFFF, ri.alpha = 0.22f * ka * vr, ri.on_black = false, ri.on = nullptr;
    ar.kind = HI_ARC, ar.x = VP_BTN_X, ar.y = VP_BTN_Y, ar.r = VP_RING_R, ar.w = VP_RING_W, ar.a0 = 0, ar.a1 = 2 * HALO_PI * vd_vol, ar.scale = 1, ar.color = VP_TEXT, ar.alpha = (vd_vol > 0.004f ? ka * vr : 0), ar.on_black = false, ar.on = nullptr;
    halo_view_commit(ri, v), halo_view_commit(ar, v);
    vd_text(vu->num, gl_r_n40, vd_num, vd_mid_n40, 1, VP_BTN_X, VP_NUM_Y, VP_TEXT, ka * vr, v);
  } else {
    for (HaloItem *it : {&vu->name, &vu->cur, &vu->len, &vu->track, &vu->fill, &vu->knob, &vu->plate, &vu->ic[0], &vu->ic[1], &vu->ic[2], &vu->ring, &vu->arc, &vu->num}) it->alpha = 0, halo_view_commit(*it, v);
  }
  // Anything of theirs changed: a frame is drawn, and with it the picture under them is decoded again (halo_under, and
  // vd_ctl_repaint marks all of them over it). Hidden just now: the picture alone, once more
  uint32_t h = 2166136261u;
  for (float q : {k * 256, vr * 256, halo_clamp01(vd_pp.x) * 256, vd_vol * 1000, vd_seek_at * 4000, vh_position() * 10, (float)vd_sel, v.a * 256})
    h = (h ^ (uint32_t)(int32_t)q) * 16777619u;
  const bool vis = on && ka > 0.004f;  // (closing they are not drawn at all: marking them made the close redraw the whole screen)
  if (vis && h != vd_ctl_h) vd_ctl_mark();
  else if (!vis && vd_ctl_vis) halo_damage(0, 0, W - 1, HALO_H - 1);
  vd_ctl_h = h, vd_ctl_vis = vis;
}

// The player on its way into Always On or dark (its view faded or smaller): the video stops, and its frame is drawn like
// any picture, through the view (vh_freeze). Before, the frame buffer kept the whole frame at full size and brightness,
// and the watch face's digits went over it in their own rectangles only. Seen as it is again — the same frame from the
// decoder, paused, the controls up
static void vd_away_check(const HaloView &v) {
  if (vd_mode != VM_PLAY) return (void)(vd_away = false);
  const bool plain = v.a > 0.995f && fabsf(v.s - 1) < 0.002f && fabsf(v.x) < 0.5f && fabsf(v.y) < 0.5f;
  if (!plain) {
    if (!vd_away) LOG("video: away (view %.2f %.2f %.4f %.3f)\n", v.x, v.y, v.s, v.a);
    if (vh_playing()) vh_set_playing(false), vd_paused_at = halo_ms() | 1;
    if (!vd_away) vd_show_ctl();
    vh_freeze(true);
    vd_away = true;
  } else if (vd_away) {
    vh_freeze(false);
    vd_away = false, vd_paused_at = halo_ms() | 1;  // (the two minutes till it closes by itself: from now)
  }
}

static void video_layout(const HaloView &v) {
  // (still in lib/Halo's stack: it lays out every screen there each frame — on the way to the face as well, where the
  // screen's update is not called while the finger leads; it was taken for the screen gone, and the player closed at
  // the first frame of the swipe up)
  vd_on_top = true;
  vd_away_check(v);
  vd_v = v;
  vd_lib_layout(v);
  vd_ctl_layout(v);
}

// ---------- Drawing ----------
static void vd_lib_draw() {
  const float a = vd_lib_alpha();
  if (a <= 0.004f) return;
  const HaloView &v = vd_v;
  const int ia = (int)(a * v.a * 256), n = vh_n, sc = (int)lroundf(vd_scroll);
  for (int s = 0; s < VS_MARKS; s++) halo_item_draw(vu->sglow[s]), halo_item_draw(vu->sring[s]);  // (under the tiles)
  for (int i = 0; i < n; i++) {
    const int x = VT_X0 + i % 3 * VT_DX, y = VT_Y0 + i / 3 * VT_DY + sc;
    if (i == vd_sel || y + VT_H <= 0 || y >= HALO_H || !vh_files[i].thumb) continue;
    const float sq = i == vd_pop ? (1 - 0.03f * halo_clamp01(vd_press.x)) : 1, hw = VT_W * sq / 2, hh = VT_H * sq / 2, cx = x + VT_W / 2.0f, cy = y + VT_H / 2.0f;
    vd_blit(vh_files[i].thumb, VH_TW, VH_TH, (VH_TW - VT_W) / 2, (VH_TH - VT_H) / 2, VT_W, VT_H, halo_view_x(v, cx - hw), halo_view_y(v, cy - hh), halo_view_x(v, cx + hw), halo_view_y(v, cy + hh), VT_R * sq * v.s, ia, !v.over);
  }
  for (int s = 0; s < VS_MARKS; s++)
    for (HaloItem *it : {&vu->spill[s], &vu->sdot[s], &vu->sw1[s], &vu->sw2[s], &vu->swd[s], &vu->stxt[s]}) halo_item_draw(*it);
  halo_item_draw(vu->title), halo_item_draw(vu->clock), halo_item_draw(vu->empty), halo_item_draw(vu->empty2);
  for (int s = 0; s < VT_SLOTS; s++) halo_item_draw(vu->t[s]), halo_item_draw(vu->d[s]);
}

// The tile growing into the screen, or the screen shrinking into the tile: the small picture early, the clip's frame over it
static void vd_grow_draw() {
  const float p = vd_snap(vd_open_s.x);
  float g[4];
  if (!vh_files || !vd_grow_rect(p, g)) return;
  const float x0 = g[0], y0 = g[1], x1 = g[2], y1 = g[3], r = VT_R * (1 - p);
  const float q = halo_clamp01(p / 0.2f);
  const VhFile &F = vh_files[vd_sel];
  const VdPic thumb = {F.thumb, VH_TW, VH_TH, (VH_TW - VT_W) / 2, (VH_TH - VT_H) / 2, VT_W, VT_H};
  if (F.thumb && !vh_still_ok) return vd_blit(thumb.px, VH_TW, VH_TH, thumb.sx, thumb.sy, VT_W, VT_H, x0, y0, x1, y1, r, 256);
  if (!vh_still_ok) return;
  if (p >= 1) return vd_blit(vh_still, VH_W, VH_H, 0, 0, VH_W, VH_H, 0, VH_TOP, VH_W, VH_TOP + VH_H, 0, F.thumb ? (int)(q * 256) : 256);
  const float sw = 382.5f + (512 - 382.5f) * p;  // (the half-size picture; the tile shows the middle of the frame)
  const VdPic half = {vh_half, VH_W / 2, VH_H / 2, (VH_W / 2 - sw) / 2, 0, sw, VH_H / 2};
  if (F.thumb && q < 1) return vd_cross(thumb, half, x0, y0, x1, y1, r, (int)(q * 256));
  vd_blit(half.px, VH_W / 2, VH_H / 2, half.sx, 0, sw, VH_H / 2, x0, y0, x1, y1, r, 256);
}

// The clip's frame as an ordinary picture, through the player's view: the player is not seen as it is (on its way into
// Always On or dark — faded, smaller), and the frame buffer can't hold the video then (vh_freeze)
static void vd_still_draw() {
  if (!vh_still_ok) return;
  const HaloView &v = vd_v;
  // (coarse on the board: every row of the frame for two of the screen — the whole frame read out of PSRAM was 20 ms a
  // frame. Out of the half-size copy it was slower still: a pixel at a time, twice each, no runs to copy)
  vd_blit(vh_still, VH_W, VH_H, 0, 0, VH_W, VH_H, halo_view_x(v, 0), halo_view_y(v, VH_TOP), halo_view_x(v, VH_W), halo_view_y(v, VH_TOP + VH_H), 0,
          (int)(v.a * 256 + 0.5f), !v.over, VH_UNDER);
}

static void vd_scrim_draw(float k) {
  const HaloView &v = vd_v;
  const int xa = max(hb.x0, (int)halo_view_x(v, 0)), xb = min(hb.x1, (int)halo_view_x(v, W) - 1);
  if (xa > xb) return;
  for (int y = max(hb.y, 0); y < hb.y + hb.rows; y++) {
    const float py = HALO_H / 2.0f + (y - HALO_H / 2.0f - v.y) / v.s;  // (the row in the player's own place: the view makes it smaller)
    int A = 0;
    if (py < VP_TOP_H) A = (int)(115 * k * fminf(VP_TOP_H - py, VP_TOP_H) / VP_TOP_H);
    else if (py >= VP_SCRIM_Y) A = (int)(184 * k * fminf(py - VP_SCRIM_Y, HALO_H - VP_SCRIM_Y) / (HALO_H - VP_SCRIM_Y));
    if (A > 0) halo_fast_veil(halo_row(y) + xa, xb - xa + 1, 0, A);
  }
}

static void video_draw() {
  if (!vu) return;
  const bool over = vd_mode == VM_PLAY || vd_mode == VM_HOLD;
  if (!over) vd_lib_draw(), vd_grow_draw();
  else if (!VH_UNDER || vh_frozen) vd_still_draw();
  if (!over) return;
  const float k = halo_clamp01(vd_ctl.x) * vd_v.a;
  if (k <= 0.004f) return;
  vd_scrim_draw(k);
  halo_item_draw(vu->plate);
  for (HaloItem &it : vu->ic) halo_item_draw(it);
  halo_item_draw(vu->ring), halo_item_draw(vu->arc), halo_item_draw(vu->num);
  halo_item_draw(vu->name), halo_item_draw(vu->cur), halo_item_draw(vu->len);
  halo_item_draw(vu->track), halo_item_draw(vu->fill), halo_item_draw(vu->knob);
}
