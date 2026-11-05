// volume.h — the speaker's volume and its screen, shared by the Music player and the Recorder (one speaker, one
// volume): HaloWatch's player's, one to one (moved here from player.h). The volume screen: "+" at the top, "−" at the
// bottom, the speaker in the middle; a tap elsewhere or 3 s idle — closed; it grows from 0.92 while the screen under it
// fades through. The level at the left edge (LevelIndicator: r 223, 142.8°…217.2° from 3 o'clock) shows with it and
// for 2.5 s after a change. 0..10, 6 at start (not saved).
// The owner screen: vol_enter() when it opens, vol_update(dt) first in its update (true — the volume screen took the
// finger), f = vol_layout(v) — how much of its own screen is seen, vol_draw() last; vol_open() — its volume button.
// vol_tick(dt) — once a frame (settings_tick). Included from settings.h before player.h and recorder.h.

#define VOL_PRIMARY 0xE9DDFF     // "+" and "−"
#define VOL_ON_SURFACE 0xF6EDFF  // the speaker
#define VOL_FILL 0xBAC3FF        // the level: the volume
#define VOL_TRACK 0x332E3C       // the level: the rest
#define VOL_BTN 0x4D3D76         // "+/−"
#define VOL_PLUS "\xEF\x81\xA7"     // Font Awesome (gl_ic24): plus, minus, volume-high
#define VOL_MINUS "\xEF\x81\xA8"
#define VOL_SPEAKER "\xEF\x80\xA8"

static int vol_level = 6;
static float vol_lvl = 0.6f, vol_from = 0.6f, vol_t = 9, vol_hide_in, vol_idle_in, vol_mid_i;
static HaloSpring vol_show, vol_screen;
static HaloSpringK K_EFFECT_FAST, K_EFFECT_SLOW;
static HaloEase EASE_VOL_SHARP;
static Press vol_btn[2];
static int vol_armed = -1;
static bool vol_active, vol_built;
static HaloItem vol_arc[2], vol_b[2], vol_b_ic[2], vol_ic;

// Volume 0..10 → the codec: 0 — silence, 1..10 — −40 … +6 dB in even steps for the ear (the codec's register is
// 0.5 dB a step, 0 dB = 0xBF; its top, +32 dB, only distorts music)
static int vol_codec(int vol) {
  if (vol <= 0) return 0;
  const float db = -40 + (vol - 1) * 46.0f / 9, reg = 0xBF + 2 * db;
  return (int)lroundf((reg + 1) * 100 / 256);
}
static void vol_set(int v) {
  vol_level = constrain(v, 0, 10);
  halo_audio_volume(vol_codec(vol_level));
  vol_from = vol_lvl, vol_t = 0;
  vol_show.target = 1, vol_hide_in = 2.5f;
}
static void vol_build() {
  if (vol_built) return;
  vol_built = true;
  K_EFFECT_FAST = halo_spring(halo_stiff(1400), 1);
  K_EFFECT_SLOW = halo_spring(halo_stiff(260), 1);
  halo_ease_make(EASE_VOL_SHARP, 0, 0, 0, 1);
  vol_mid_i = halo_text_mid(gl_ic24, VOL_SPEAKER);
  halo_audio_volume(vol_codec(vol_level));
  vol_lvl = vol_from = vol_level / 10.0f;
}
static bool vol_screen_up() { return vol_screen.target > 0.5f; }
static void vol_open() { vol_screen.target = 1, vol_idle_in = 3, vol_show.target = 0; }
static void vol_enter() { vol_armed = -1; }

static void vol_tick(float dt) {
  const HaloFinger &f = halo_finger;
  for (int i = 0; i < 2; i++) press_update(vol_btn[i], f.down && vol_active && vol_screen_up() && vol_armed == i, dt);
  vol_active = false;  // vol_update sets it again while an owner screen is up
  halo_spring_run(vol_screen, K_EFFECT_FAST, dt);
  vol_t = fminf(vol_t + dt, 9);  // the level: 500 ms along (0, 0, 0, 1); the arc hides after 2.5 s
  vol_lvl = vol_from + (vol_level / 10.0f - vol_from) * halo_ease(EASE_VOL_SHARP, vol_t / 0.5f);
  vol_hide_in = fmaxf(vol_hide_in - dt, -1);
  if (vol_hide_in < 0) vol_show.target = 0;
  halo_spring_run(vol_show, vol_show.target > 0.5f ? K_EFFECT_FAST : K_EFFECT_SLOW, dt);
}

static bool vol_update(float dt) {
  HaloFinger &f = halo_finger;
  vol_active = true;
  if (!vol_screen_up()) return false;
  if (f.pressed) vol_armed = fabsf(f.x0 - CX) < 66 && fabsf(f.y0 - 118) < 54 ? 0 : fabsf(f.x0 - CX) < 66 && fabsf(f.y0 - 348) < 54 ? 1 : -1;
  if (f.dragging) vol_armed = -1;
  if (f.tap) {
    if (vol_armed >= 0) vol_set(vol_level + (vol_armed ? -1 : 1)), vol_idle_in = 3;
    else vol_screen.target = 0;
  }
  if ((vol_idle_in -= dt) < 0) vol_screen.target = 0;
  if (!f.down) vol_armed = -1;
  return true;
}

static float vol_layout(const HaloView &v) {
  const float vs = halo_clamp01(vol_screen.x);
  const float f_own = halo_clamp01(1 - vs / 0.4f), f_vol = halo_clamp01((vs - 0.4f) / 0.6f);  // fade through
  // The arc at the left edge (LevelIndicator), with the volume screen too
  const float va = fmaxf(vol_show.x, f_vol), a0 = (142.8f + 90) * HALO_PI / 180, A = 74.4f * HALO_PI / 180;
  const float g = 2.25f * HALO_PI / 180, a1 = a0 + A * halo_clamp01(vol_lvl);
  for (int i = 0; i < 2; i++) {
    HaloItem &it = vol_arc[i];
    const float from = i ? a1 + g : a0 + g, to = i ? a0 + A - g : a1 - g;
    it.kind = HI_ARC, it.x = CX, it.y = CX, it.r = 223, it.w = 12, it.scale = 1, it.a0 = from, it.a1 = to;
    it.color = i ? VOL_TRACK : VOL_FILL, it.alpha = va > 0.01f && to - from > 0.01f ? va : 0, it.on_black = true;
    halo_view_commit(it, v);
  }
  // The volume screen: "+" at the top, "−" at the bottom, the icon in the middle; grows from 0.92 (fade through)
  const float sc = 0.92f + 0.08f * f_vol;
  for (int i = 0; i < 2; i++) {
    const float p = halo_clamp01(vol_btn[i].s.x), my = CX + ((i ? 348.0f : 118.0f) - CX) * sc;
    HaloItem &b = vol_b[i];
    b.kind = HI_RRECT, b.x = CX, b.y = my, b.w = 120 * sc, b.h = 96 * sc, b.r = (48 + (16 - 48) * p) * sc, b.scale = 1;
    b.color = VOL_BTN, b.alpha = f_vol, b.on_black = true;
    halo_view_commit(b, v);
    gl_text_mid(vol_b_ic[i], gl_ic24, i ? VOL_MINUS : VOL_PLUS, vol_mid_i, CX, my, sc, VOL_PRIMARY, f_vol, sc == 1 ? &b : nullptr);
    halo_view_commit(vol_b_ic[i], v);
  }
  gl_text_mid(vol_ic, gl_ic24, VOL_SPEAKER, vol_mid_i, CX, CX, 1.6f * sc, VOL_ON_SURFACE, f_vol, nullptr);
  halo_view_commit(vol_ic, v);
  return f_own;
}
static void vol_draw() {
  for (auto &it : vol_arc) halo_item_draw(it);
  for (int i = 0; i < 2; i++) halo_item_draw(vol_b[i]), halo_item_draw(vol_b_ic[i]);
  halo_item_draw(vol_ic);
}
