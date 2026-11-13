// halo_web.cpp — the board half of Halo, for the browser (PopkaStudio engine).
//
// On the board Halo is two halves: hardware (halo_core, halo_display, halo_frame, halo_touch, halo_power, halo_audio:
// the CO5300 panel over QSPI, the TE signal, the touch chip, sleep, sound) and drawing (halo_draw, halo_ui, halo_list,
// halo_nav, halo_roll). The drawing half is compiled for the browser unchanged; this file replaces the hardware half.
//
// A frame is made the way the board makes it: update(dt) → for every band of 16 rows with damage: black background of
// the damaged columns (within the round screen), then draw() → the band goes into a 466×466 copy of the panel's own
// memory. Nothing else changes in that memory, exactly as on the panel. The page calls web_frame() on its display
// refresh, at most 60 times a second; dt is a multiple of 1/60 s, as on the board.
//
// Kept from the board: animation time (pause, step, slow), the finger from samples with timestamps (the page sends
// mouse or touch moves; while the button is held still, the sample repeats — like the touch chip, which reports 100
// times a second), commands (the page's command line — the same as the serial port's), the log (the console).
// Not here: two cores (one is enough: a band is drawn whole), DMA, the frame-pacing choice ahead of / behind the scan,
// sleep, sound, the motion sensor, I2C — calls answer "nothing here".
#include "halo_internal.h"
#include "halo_spans.h"

#include <emscripten.h>

WebSerial Serial;
__thread HaloBand hb;
uint32_t hf_finger_t;
volatile bool halo_quiet = false;
volatile bool halo_trace_on = false;

static HaloConfig cfg;
static HaloUpdate m_update;
static HaloDraw m_draw;
static bool m_started;

// ---------- The page's clock ----------
static double page_us;  // performance.now() of the latest call from the page, µs

uint32_t millis() {
  return (uint32_t)(page_us / 1000);
}
uint32_t micros() {
  return (uint32_t)page_us;
}
int64_t esp_timer_get_time() {
  return (int64_t)page_us;
}
uint32_t esp_random() {  // xorshift32: the same "random" on every start — a preview that does not change by itself
  static uint32_t s = 0x9E3779B9u;
  s ^= s << 13, s ^= s >> 17, s ^= s << 5;
  return s;
}

// ---------- Damage: per band — up to HALO_SPANS spans of columns (halo_spans.h, as halo_frame.cpp) ----------
static HaloSpans dmg[HALO_BANDS];  // n = 0 — the band is clean
static int16_t chord_x0[HALO_BANDS], chord_x1[HALO_BANDS];  // the round screen within the band (even — odd)

#ifdef HALO_FAST_VEIL  // (a board with its own loop for this: here the same formula, so the page shows what the board shows)
// (each channel on its own, as the board's halo_p4_simd.cpp since 9 Nov 2026: R and B in one multiply let R's low
// product bits into B's top ones — a blue cast under every veil over color)
void halo_fast_veil(uint16_t *p, int n, uint16_t color, int A) {
  const uint32_t kinv = 256 - A;
  const uint32_t veil = (((color & 0xF800u) * (uint32_t)A >> 8) & 0xF800u) | (((color & 0x07E0u) * (uint32_t)A >> 8) & 0x07E0u) |
                        ((color & 0x001Fu) * (uint32_t)A >> 8);
  for (int i = 0; i < n; i++) {
    const uint32_t c = p[i];
    const uint32_t r = ((c & 0xF800u) * kinv >> 8) & 0xF800u, g = ((c & 0x07E0u) * kinv >> 8) & 0x07E0u, b = (c & 0x001Fu) * kinv >> 8;
    p[i] = (uint16_t)(r + g + b + veil);
  }
}
#endif

static void chords() {
#ifndef HALO_ROUND  // a rectangle: every band is the whole width
  for (int b = 0; b < HALO_BANDS; b++) chord_x0[b] = 0, chord_x1[b] = HALO_W - 1, dmg[b].n = 0;
  return;
#endif
  const float c = (HALO_W - 1) / 2.0f, R = HALO_W / 2.0f;
  for (int b = 0; b < HALO_BANDS; b++) {
    const int y = b * HALO_ROWS, rows = min(HALO_ROWS, HALO_H - y);
    const float dy = (y <= c && y + rows >= c) ? 0 : fminf(fabsf(y - c), fabsf(y + rows - c));
    const float half = halo_sqrt(fmaxf(0, R * R - dy * dy)) + 2;
    chord_x0[b] = max(0, (int)(c - half)) & ~1;
    chord_x1[b] = min(HALO_W - 1, (int)(c + half)) | 1;
    dmg[b].n = 0;
  }
}

void halo_damage(int x0, int y0, int x1, int y1) {
  x0 = max(x0, 0), y0 = max(y0, 0), x1 = min(x1, HALO_W - 1), y1 = min(y1, HALO_H - 1);
  if (x0 > x1 || y0 > y1) return;
  for (int b = y0 / HALO_ROWS; b <= y1 / HALO_ROWS; b++) halo_spans_add(dmg[b], x0, x1);
}

void halo_damage_f(float x0, float y0, float x1, float y1) {
  halo_damage(halo_floor(x0) - 1, halo_floor(y0) - 1, halo_floor(x1) + 2, halo_floor(y1) + 2);
}

void halo_damage_all() {
  halo_damage(0, 0, HALO_W - 1, HALO_H - 1);
}

void halo_heavy() {}  // frame pacing against the display scan — the browser has its own

// ---------- Animation time ----------
static uint64_t m_clock = 1000000;  // as on the board: starts a little after 0
static bool vt_paused;
static int vt_steps;
static float slowmo = 1;

uint32_t halo_us() {
  return (uint32_t)m_clock;
}
uint32_t halo_ms() {
  return (uint32_t)(m_clock / 1000);
}

// ---------- The panel ----------
static uint16_t panel[HALO_W * HALO_H];       // the panel's memory, in the display's byte order (as the board sends it)
static uint8_t rgba[HALO_W * HALO_H * 4];     // the same for the page's canvas
static uint16_t band_buf[HALO_ROWS * HALO_W];
static int last_bands, last_px;
static int dirty[4];  // the last frame's changed area: x0, y0, x1, y1 (x1 < x0 — none): the page copies only that
static int brightness = 180, shown_level = -1;
static bool disp_on = true;

// 565 → 888 as tools/screenshot.py does it, so the page and a board screenshot can be compared pixel for pixel; lv5,
// lv6 — the same at the panel's brightness now (at the start's brightness — x5, x6 exactly)
static uint8_t x5[32], x6[64], lv5[32], lv6[64];

// The panel's brightness on the page (Always On dims it, the screen off is black): the panel's level is about the light
// it gives, the page's pixels are sRGB — level b against the start's shows as (b / start)^(1/2.2) of each value. A new
// level — the whole picture again from the panel's memory; true then
static bool shade() {
  const int level = disp_on ? brightness : 0;
  if (level == shown_level) return false;
  shown_level = level;
  const float f = fminf(1, powf(level / (float)max(1, (int)cfg.brightness), 1 / 2.2f));
  for (int i = 0; i < 32; i++) lv5[i] = (uint8_t)lroundf(x5[i] * f);
  for (int i = 0; i < 64; i++) lv6[i] = (uint8_t)lroundf(x6[i] * f);
  for (int i = 0; i < HALO_W * HALO_H; i++) {
    const uint16_t v = HALO_PX(panel[i]);
    rgba[i * 4] = lv5[v >> 11], rgba[i * 4 + 1] = lv6[(v >> 5) & 63], rgba[i * 4 + 2] = lv5[v & 31];
  }
  return true;
}

static void band_out(int y, int rows, int x0, int x1) {
  for (int r = 0; r < rows; r++) {
    const uint16_t *src = band_buf + r * HALO_W;
    uint16_t *dst = panel + (y + r) * HALO_W;
    uint8_t *px = rgba + ((y + r) * HALO_W + x0) * 4;
    for (int x = x0; x <= x1; x++, px += 4) {
      const uint16_t c = src[x];
      dst[x] = c;
      const uint16_t v = HALO_PX(c);
      px[0] = lv5[v >> 11], px[1] = lv6[(v >> 5) & 63], px[2] = lv5[v & 31], px[3] = 255;
    }
  }
}

static void render() {
  int bands = shade() ? 1 : 0, pixels = 0;
  if (bands) dirty[0] = dirty[1] = 0, dirty[2] = HALO_W - 1, dirty[3] = HALO_H - 1;  // (dimmed: all of it)
  else dirty[0] = dirty[1] = 1 << 20, dirty[2] = dirty[3] = -1;
  for (int b = 0; b < HALO_BANDS; b++) {
    HaloSpans d = dmg[b];
    dmg[b].n = 0;
    halo_spans_sort(d);
    for (int sp = 0; sp < d.n; sp++) {
      const int x0 = max(d.x0[sp] & ~1, (int)chord_x0[b]), x1 = min(d.x1[sp] | 1, (int)chord_x1[b]);
      if (x0 > x1) continue;
      const int y = b * HALO_ROWS, rows = min(HALO_ROWS, HALO_H - y);
      hb = {band_buf, y, rows, x0, x1, 0, HALO_W};
      for (int r = 0; r < rows; r++) memset(band_buf + r * HALO_W + x0, 0, (x1 - x0 + 1) * 2);
      m_draw();
      band_out(y, rows, x0, x1);
      bands++, pixels += (x1 - x0 + 1) * rows;
      dirty[0] = min(dirty[0], x0), dirty[1] = min(dirty[1], y), dirty[2] = max(dirty[2], x1), dirty[3] = max(dirty[3], y + rows - 1);
    }
  }
  last_bands = bands, last_px = pixels;
}

// ---------- The finger: samples from the page, newest first ----------
// Each with the time the mouse was there (the event's own time: the browser hands moves over once a frame, several
// together and a little late). As the touch chip, a report every ~10 ms: a mouse can report far more often (1000 Hz), so
// a move closer than TS_GAP to the report before last only moves the newest report. While the button is held still,
// the report repeats (web_frame) — such repeats at the top are dropped when a move turns out to be older than them
#define TS_MAX 16
#define TS_GAP 8000
static HaloTouchSample ts[TS_MAX];
static int ts_n, ts_reps;
static bool ts_down;

int halo_touch_samples(HaloTouchSample *out, int max_n) {
  const int n = min(ts_n, max_n);
  memcpy(out, ts, n * sizeof(HaloTouchSample));
  return n;
}

static void ts_push(float x, float y, uint32_t t) {
  while (ts_reps && ts_n > 1 && (int32_t)(t - ts[0].t_us) <= 0) {
    memmove(ts, ts + 1, (TS_MAX - 1) * sizeof(HaloTouchSample));
    ts_n--, ts_reps--;
  }
  if (ts_n && (int32_t)(t - ts[0].t_us) <= 0) t = ts[0].t_us + 1;  // never back in time: speed comes from these
  const HaloTouchSample s = {(int16_t)constrain((int)lroundf(x), 0, HALO_W - 1),
                             (int16_t)constrain((int)lroundf(y), 0, HALO_H - 1), t};
  if (ts_n >= 2 && (int32_t)(t - ts[1].t_us) < TS_GAP) return (void)(ts[0] = s);
  memmove(ts + 1, ts, (TS_MAX - 1) * sizeof(HaloTouchSample));
  ts[0] = s;
  if (ts_n < TS_MAX) ts_n++;
}

// ---------- Commands and the log ----------
struct Command {
  const char *name;
  void (*fn)(const char *args);
};
static Command commands[64];
static int command_count;

void halo_command(const char *name, void (*fn)(const char *args)) {
  for (int i = 0; i < command_count; i++)
    if (!strcmp(commands[i].name, name)) return (void)(commands[i].fn = fn);
  if (command_count < (int)(sizeof(commands) / sizeof(commands[0]))) commands[command_count++] = {name, fn};
}

void halo_logf(const char *fmt, ...) {
  char line[512];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof(line), fmt, ap);
  va_end(ap);
  fputs(line, stdout);
  if (!strchr(line, '\n')) fflush(stdout);
}

void halo_serial_binary(bool) {}
void halo_trace_put(uint8_t, uint32_t, uint16_t, uint16_t) {}
void halo_trace_name(uint8_t, const char *, char) {}
void halo_trace_pump() {}

// ---------- The rest of the board: buttons, power, sensors, sound — not in the browser ----------
static bool boot_cmd, boot_long_cmd;
static uint8_t pwr_cmd;
static uint32_t motion_count;
// Sleep as the board's on USB: the frames go on; a touch (it presses nothing until let go), BOOT (not a press), motion
// or the timer (animation time) wake it — halo_woke in the next update
static bool slept, woke_touch, woke_motion;
static uint64_t wake_at;
static uint8_t woke;

void halo_brightness(uint8_t level) {
  brightness = level;
}
void halo_display_on(bool on) {
  disp_on = on;
}
bool halo_display_is_on() {
  return disp_on;
}
bool halo_boot_click() {
  return boot_cmd ? (boot_cmd = false, true) : false;
}
bool halo_boot_long() {
  return boot_long_cmd ? (boot_long_cmd = false, true) : false;
}
uint8_t halo_pwr_event() {
  const uint8_t e = pwr_cmd;
  pwr_cmd = 0;
  return e;
}
HaloBattery halo_battery() {
  return {true, true, true, false, 100, 4140, 5040, 4980, 384};  // on USB, full — as the board on the desk
}
HaloI2c halo_i2c(uint8_t) {
  return nullptr;
}
bool halo_i2c_write(HaloI2c, uint8_t, uint8_t) {
  return false;
}
bool halo_i2c_read(HaloI2c, uint8_t, uint8_t *, int) {
  return false;
}
bool halo_audio_begin(HaloAudioFill, int) {
  return false;
}
static int audio_volume = 60;
void halo_audio_volume(int v) {
  audio_volume = v;
}
int halo_audio_get_volume() {
  return audio_volume;
}
int halo_mic_begin(int) {
  return 0;
}
int halo_mic_read(int16_t *, int) {
  return 0;
}
float halo_mic_db() {
  return -90;
}
void halo_mic_db2(float &mic1, float &mic2) {
  mic1 = mic2 = -90;
}
void halo_mic_end() {}
void halo_sleep(uint32_t ms) {
  slept = true, woke_touch = woke_motion = false, wake_at = ms ? m_clock + ms * 1000ull : 0;
}
uint8_t halo_woke() {
  return woke;
}
void halo_wake() {
  slept = false;
}
bool halo_asleep() {
  return slept;
}
static void wake_check() {  // before each update
  woke = 0;
  if (!slept) return;
  if (woke_touch) woke = HALO_WAKE_TOUCH, hu_swallow_touch();
  else if (boot_cmd) woke = HALO_WAKE_BOOT, boot_cmd = false;  // the press that woke it is not a press
  else if (woke_motion) woke = HALO_WAKE_MOTION;
  else if (wake_at && m_clock >= wake_at) woke = HALO_WAKE_TIMER;
  if (woke) slept = false;
}
void halo_sleep_on_usb(bool) {}
void halo_stay_awake(bool) {}
bool halo_motion_begin(int) {
  return true;
}
// Steps: a number for the editor's preview (the command steps N sets it)
static uint32_t web_steps = 6233;
bool halo_steps_begin() {
  halo_command("steps", [](const char *a) { if (*a) web_steps = strtoul(a, nullptr, 10); });
  return true;
}
uint32_t halo_steps() {
  return web_steps;
}
uint32_t halo_motion_count() {
  return motion_count;
}
bool halo_imu_begin() {
  return false;
}
bool halo_imu_read(HaloImu &) {
  return false;
}
void halo_imu_end() {}

// ---------- Start ----------
bool halo_begin(HaloUpdate update, HaloDraw draw, const HaloConfig &c) {
  cfg = c;
  m_update = update;
  m_draw = draw;
  brightness = cfg.brightness;
  for (int i = 0; i < 32; i++) x5[i] = i * 255 / 31;
  for (int i = 0; i < 64; i++) x6[i] = i * 255 / 63;
  for (int i = 0; i < HALO_W * HALO_H; i++) rgba[i * 4 + 3] = 255;  // black panel
  chords();
  hu_begin();
  halo_command("pause", [](const char *) { vt_steps = 0, vt_paused = true; });
  halo_command("resume", [](const char *) { vt_steps = 0, vt_paused = false; });
  halo_command("step", [](const char *a) { vt_steps += max(1, atoi(a)), vt_paused = true; });
  halo_command("slow", [](const char *a) { slowmo = 1 / constrain((float)atof(a), 1.0f, 100.0f); });
  halo_command("bright", [](const char *a) { halo_brightness(atoi(a)); });
  halo_command("boot", [](const char *a) { (strstr(a, "long") ? boot_long_cmd : boot_cmd) = true; });
  halo_command("pwr", [](const char *a) { pwr_cmd = strstr(a, "long") ? HALO_PWR_LONG : HALO_PWR_CLICK; });
  halo_command("shake", [](const char *) { motion_count++, woke_motion = true; });
  halo_command("redraw", [](const char *) { halo_damage_all(); });
  halo_command("help", [](const char *) {
    for (int i = 0; i < command_count; i++) printf("%s%s", commands[i].name, i + 1 < command_count ? " " : "\n");
  });
  halo_damage_all();
  m_started = true;
  LOG("halo: started in the browser (%s)\n", cfg.name);
  return true;
}

void halo_loop() {}  // commands come from the page (web_command), the log goes straight out

// The editor's map shows one screen of the sketch at rest: its own update and draw instead of the sketch's
// (studio_api.h: studio_pin)
void web_loop(HaloUpdate update, HaloDraw draw) {
  m_update = update, m_draw = draw;
  halo_damage_all();
}

// ---------- What the page calls ----------
extern void setup();
extern void loop();

int main() {
  setup();
  return 0;
}

extern "C" {

// One display refresh: ticks — how many 1/60 s passed since the last frame (1 normally, more if the page lagged).
// Returns how many bands were drawn (0 — nothing changed: the page need not repaint).
EMSCRIPTEN_KEEPALIVE int web_frame(double now_ms, int ticks) {
  page_us = now_ms * 1000;
  if (!m_started) return 0;
  ticks = constrain(ticks, 1, 4);
  // While the button is held still the touch chip keeps reporting the same place, every 10 ms: speed falls to 0, no
  // fling. A repeat is stamped 10 ms after the report it repeats, once that is 40 ms past: a move comes a frame late or
  // more (a busy page), with its own earlier time — it must not land behind a repeat (the finger would seem to stop)
  for (int i = 0; i < 3 && ts_down && ts_n && (int32_t)((uint32_t)page_us - ts[0].t_us) > 50000; i++)
    ts_push(ts[0].x, ts[0].y, ts[0].t_us + 10000), ts_reps++;
  // The finger as it was a little before the frame (as the board: between two reports of the touch chip)
  hf_finger_t = (uint32_t)page_us - 8000;
  // As the board's frame task: the clock goes by whole display periods; paused — only by steps of exactly 1/60 s
  const bool frame_new = !vt_paused || vt_steps > 0;
  if (vt_paused && frame_new) vt_steps--;
  const float dt = vt_paused ? 1.0f / 60 : ticks / 60.0f;
  if (frame_new) {
    m_clock += (uint32_t)(dt * 1e6f + 0.5f);
    wake_check();
    halo_subpixel = false;  // every frame anew: whoever needs it sets it in update (halo_frame.cpp)
    m_update(vt_paused ? dt : dt * slowmo);
  }
  render();
  loop();
  return last_bands;
}

// The finger: down (pressed or dragging) or up; x, y — in screen pixels 0..465; t_ms — when it was there (the event's
// own time: up to a frame before the call)
EMSCRIPTEN_KEEPALIVE void web_touch(int down, float x, float y, double t_ms) {
  page_us = fmax(page_us, t_ms * 1000);  // the page's clock only goes forward
  if (down) {
    if (!ts_down && slept) woke_touch = true;
    ts_push(x, y, (uint32_t)(t_ms * 1000));
    ts_down = true, ts_reps = 0;
  } else {
    ts_n = ts_reps = 0, ts_down = false;
  }
}

// A line as typed into the board's serial port: "name args"
EMSCRIPTEN_KEEPALIVE int web_command(const char *line) {
  char name[32];
  int n = 0;
  while (*line == ' ') line++;
  while (line[n] && line[n] != ' ' && n < (int)sizeof(name) - 1) name[n] = line[n], n++;
  name[n] = 0;
  const char *args = line + n;
  while (*args == ' ') args++;
  for (int i = 0; i < command_count; i++)
    if (!strcmp(commands[i].name, name)) {
      commands[i].fn(args);
      return 1;
    }
  printf("unknown command: %s (help — the list)\n", name);
  return 0;
}

EMSCRIPTEN_KEEPALIVE uint8_t *web_rgba() {
  return rgba;
}
EMSCRIPTEN_KEEPALIVE int *web_dirty() {
  return dirty;
}
EMSCRIPTEN_KEEPALIVE uint16_t *web_panel() {
  return panel;
}
EMSCRIPTEN_KEEPALIVE int web_last_px() {
  return last_px;
}
EMSCRIPTEN_KEEPALIVE int web_brightness() {
  return brightness;
}
EMSCRIPTEN_KEEPALIVE int web_display_on() {
  return disp_on;
}
EMSCRIPTEN_KEEPALIVE uint32_t web_clock_ms() {
  return halo_ms();
}
// For checking gestures: where the finger is as the frames take it (-1 — up), and how far the swipe back is pulled
EMSCRIPTEN_KEEPALIVE float web_finger_y() {
  return halo_finger.down ? halo_finger.y : -1;
}
EMSCRIPTEN_KEEPALIVE float web_nav_pull() {
  return halo_nav_pull();
}
}
