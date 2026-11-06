// device.h — About (FL_DEVCARD, FL_DEVICE) and Power (FL_CHARGE): PopkaStudio's own screens about the board itself,
// picked from the mockups (web/mockups/hardware.html). About — variant 2, "cards with live charts": a gallery whose
// cards (FL_DEVCARD, a gallery item as a card is) show a value and a small live picture each — the processor and the
// temperature a line, the memory two bars, the battery a ring, the sound a spectrum, the motion a level's bubble; a
// tap opens the card's own screen (FL_DEVICE: the title, a big number, a chart or bars or the level, a line under it,
// two arcs at the edge with their values along the arc). Power (FL_CHARGE) — variant 10's third page: the charge along
// the bezel, what is going on, the percent, the voltage and a forecast, the voltages of USB and of the system, the
// power chip's temperature. About's battery card opens it.
//
// What it reads: the cores' load (each core's idle task's run time against the time gone, as lib/Halo's trace) and the
// free memory (internal: now and the lowest ever; PSRAM) every 250 ms, the chip's temperature every
// 2 s — from loop(), kept for the charts (128 samples: 32 s; the temperature — 4 min); the power chip — halo_battery()
// (the charge, the battery's, USB's and the system's voltage, its own temperature); the microphones (halo_mic_*: a
// spectrum of 16 bands by Goertzel over 512 samples, each microphone's loudness) and the motion sensor (halo_imu_*: the
// tilt, g, its temperature) — only while a card or a screen that shows them is on screen (the microphones are switched
// in a task — I2C, as the Recorder's; the motion sensor from the frame — ~3 ms once).
// The charts are masks (4 bits a pixel) baked anew when a sample comes, the line with its glow and the area under it
// in one mask, painted in the card's color; two buffers in turn — the item sees the new address and redraws.
// The editor has no sensors: made-up values that move as the board's would (the same every time).
// Only in a firmware with these layers (FN_ABOUT, FN_CHARGE — layers.h); the editor's page has all.
// Included from face.h after settings.h (gallery.h's helpers, the fonts gl_*).

#if defined(__EMSCRIPTEN__) || (defined(FN_ABOUT) && FN_ABOUT)
#define HAS_ABOUT 1
#else
#define HAS_ABOUT 0
#endif
#if defined(__EMSCRIPTEN__) || (defined(FN_CHARGE) && FN_CHARGE)
#define HAS_CHARGE 1
#else
#define HAS_CHARGE 0
#endif

enum : uint8_t { DV_CPU, DV_MEM, DV_BAT, DV_TEMP, DV_SND, DV_MOT, DV_N };  // what a card or a screen shows (p2)
#define DV_TEXT 0xFFF8EE
#define DV_DIM 0x9E9E9E
#define DV_RED 0xFF6B6B
#define DV_GREEN 0x6DD58C
#define DV_LIL 0xD0BCFF    // the spectrum's quiet end
#define DV_PSRAM 0xFFB3AB  // PSRAM: the mockups' coral
#define DV_ITEMS 48

static inline int dv_what(const FaceLayer &L) {
  return constrain((int)L.p2, 0, DV_N - 1);
}
static uint32_t dv_mix(uint32_t a, uint32_t b, float t) {  // color a toward b by t
  uint32_t out = 0;
  for (int s = 0; s < 24; s += 8) {
    const float ca = (a >> s) & 255, cb = (b >> s) & 255;
    out |= (uint32_t)lroundf(ca + (cb - ca) * t) << s;
  }
  return out;
}
// Text by the middle of its letters at (x, y); align 0 — x is the left edge, 1 — the middle, 2 — the right edge;
// on — what it lies on (&halo_black — the black screen)
static void dv_text(HaloItem &it, const HaloFont &f, const char *s, float mid, float x, float y, float sc,
                    uint32_t color, float alpha, int align, const HaloItem *on) {
  it.kind = HI_TEXT, it.font = &f, it.text = s, it.align = align, it.color = color, it.on = on;
  it.on_black = on == &halo_black, it.x = x, it.y = y + (f.line / 2.0f - mid) * sc, it.scale = sc;
  it.alpha = alpha, it.spacing = 0;
}

// ---------- A chart: a mask baked anew when a sample comes ----------
#define DV_HN 128     // samples kept: the load and the memory every 250 ms (32 s), the temperature every 2 s (4 min)
#define DV_COLS 52    // a chart is baked this many columns a frame (into the other buffer): no frame pays for all of it
struct DvChart {
  HaloMask m[2];
  int cur = -1;           // the mask shown (-1 — none yet)
  uint32_t gen = ~0u;     // the samples it shows
  int8_t what = -1;
  float last_y = 0, last_y2 = 0;  // the newest sample's place (the dot at the line's end)
  // The bake in progress (x ≥ 0): the samples' places (y — 0 at the top), how many, room for how many across
  float ya[DV_HN], yb[DV_HN];
  int n = 0, slots = 0, w = 0, h = 0, x = -1;
  bool two = false;
  uint32_t next_gen = 0;
  int8_t next_what = -1;
};
// Start baking into the buffer not shown: n values (newest last), the second line (yb, may be null) — dim
static void dv_bake_start(DvChart &c, int w, int h, const float *ya, const float *yb, int n, int slots) {
  const int stride = (w + 1) / 2, next = c.cur < 0 ? 0 : 1 - c.cur;
  HaloMask &m = c.m[next];
  if (!m.bits) {
#ifdef __EMSCRIPTEN__
    m.bits = (const uint8_t *)malloc(stride * h), m.ext = (const uint8_t *)malloc(2 * h);
#else
    m.bits = (const uint8_t *)heap_caps_malloc(stride * h, MALLOC_CAP_SPIRAM);
    m.ext = (const uint8_t *)heap_caps_malloc(2 * h, MALLOC_CAP_SPIRAM);
#endif
    if (!m.bits || !m.ext) return;
    m.w = w, m.h = h, m.ox = m.oy = 0, m.adv = w;
  }
  memset((void *)m.bits, 0, stride * h);
  uint8_t *ext = (uint8_t *)m.ext;
  for (int y = 0; y < h; y++) ext[2 * y] = 1, ext[2 * y + 1] = 0;
  memcpy(c.ya, ya, n * sizeof(float));
  if (yb) memcpy(c.yb, yb, n * sizeof(float));
  c.two = yb != nullptr, c.n = n, c.slots = slots, c.w = w, c.h = h, c.x = 0;
}
// The next columns (all of them: cols ≤ 0). The line is ~2.6 px thick with soft edges, its glow fades over 7 px, the
// area under it fades from 16 % to the bottom; the dim line — 45 %, no glow. Done — it is the one shown
static void dv_bake_step(DvChart &c, int cols) {
  if (c.x < 0) return;
  const int next = c.cur < 0 ? 0 : 1 - c.cur, w = c.w, h = c.h, n = c.n, stride = (w + 1) / 2;
  uint8_t *bits = (uint8_t *)c.m[next].bits, *ext = (uint8_t *)c.m[next].ext;
  const float step = c.slots > 1 ? (w - 1) / (float)(c.slots - 1) : 1, inv_step = 1 / step;
  const float NONE = -1e9f;  // (no samples there: not NAN — the build is fast-math, NAN checks are folded away)
  auto at = [&](const float *v, float x) {  // the line's y at column x
    const float back = (w - 1 - x) * inv_step;  // samples from the newest
    if (back < 0) return v[n - 1];
    if (back > n - 1 + 0.001f) return NONE;
    const int i = (int)back;
    const float f = back - i, a = v[n - 1 - i], b = i + 1 < n ? v[n - 2 - i] : a;
    return a + (b - a) * f;
  };
  const int x1 = cols > 0 ? min(w, c.x + cols) : w;
  for (int x = c.x; x < x1; x++) {
    const float yc = at(c.ya, x);
    if (yc == NONE) continue;
    float l = at(c.ya, x - 0.5f), r = at(c.ya, x + 0.5f);
    if (l == NONE) l = yc;
    if (r == NONE) r = yc;
    const float lo = fminf(yc, fminf(l, r)), hi = fmaxf(yc, fmaxf(l, r));
    const float area = 0.16f / fmaxf(1, h - yc);  // (per pixel below the line: 0.16 · (1 − (p − yc) / (h − yc)))
    float lo2 = 0, hi2 = -1;
    if (c.two) {
      const float c2 = at(c.yb, x);
      float l2 = at(c.yb, x - 0.5f), r2 = at(c.yb, x + 0.5f);
      if (l2 == NONE) l2 = c2;
      if (r2 == NONE) r2 = c2;
      lo2 = fminf(c2, fminf(l2, r2)), hi2 = fmaxf(c2, fmaxf(l2, r2));
    }
    for (int y = 0; y < h; y++) {
      const float p = y + 0.5f, d = fmaxf(0, fmaxf(lo - p, p - hi));
      float a = 0;
      if (d < 7) {
        const float g = 1 - d * (1 / 7.0f);
        a = fmaxf(halo_clamp01(1.8f - d), 0.3f * g * g);
      }
      if (p > yc) a = fmaxf(a, 0.16f - (p - yc) * area);
      if (hi2 >= lo2) a = fmaxf(a, 0.45f * halo_clamp01(1.6f - fmaxf(0, fmaxf(lo2 - p, p - hi2))));
      const int v4 = (int)(a * 15 + 0.5f);
      if (v4 <= 0) continue;
      bits[y * stride + x / 2] |= (x & 1) ? v4 : v4 << 4;
      if (ext[2 * y] > ext[2 * y + 1]) ext[2 * y] = min(x, 255);
      ext[2 * y + 1] = min(x, 254);
    }
  }
  c.x = x1;
  if (c.x < w) return;
  c.x = -1, c.cur = next, c.gen = c.next_gen, c.what = c.next_what;
  c.last_y = c.ya[n - 1], c.last_y2 = c.two ? c.yb[n - 1] : 0;
}
static void dv_chart_free(DvChart &c) {
  for (auto &m : c.m) free((void *)m.bits), free((void *)m.ext), m = {};
  c.cur = -1, c.x = -1;
}

// What a card or a screen has while running (one per layer, PSRAM): its items, its chart, its words along the arc
struct DvRun {
  HaloItem it[DV_ITEMS];
  DvChart chart;
  HaloMask lab[2];
  float lab_x[2], lab_y[2];
  char lab_s[2][32], lab_on[2][32];
  char big[16], sub[48], info[48], val[16], state[24];
  uint32_t enter_ms, text_at, lab_at[2];
  uint8_t tone;  // Power: the state's color — 0 dim, 1 green, 2 red
  uint8_t used;  // items laid out last frame (the rest stay hidden: not committed again)
};
// Commit items [0, n) (and hide the ones the last frame had beyond n); box — where they were drawn (or null)
static void dv_commit(DvRun &d, int n, const HaloView &v, float *box) {
  const int m = max(n, (int)d.used);
  for (int j = 0; j < m; j++) {
    if (j >= n) d.it[j].alpha = 0;
    halo_view_commit(d.it[j], v);
    if (box && j < n) box_add(box, d.it[j]);
  }
  d.used = (uint8_t)n;
}
static void dv_run_make(FaceRun &R) {
#ifdef __EMSCRIPTEN__
  R.dv = new DvRun();
#else
  void *p = heap_caps_calloc(1, sizeof(DvRun), MALLOC_CAP_SPIRAM);
  R.dv = p ? new (p) DvRun() : new DvRun();
#endif
  R.g = R.dv->it;
}
static void dv_run_free(FaceRun &R) {
  if (!R.dv) return;
  dv_chart_free(R.dv->chart);
  for (auto &m : R.dv->lab) free((void *)m.bits), free((void *)m.ext);
  R.dv->~DvRun();
#ifdef __EMSCRIPTEN__
  operator delete(R.dv);
#else
  free(R.dv);  // (calloc or new: both from the same heap)
#endif
  R.dv = nullptr, R.g = nullptr;
}

static float dv_mid_t20, dv_mid_t24, dv_mid_n34, dv_mid_n80, dv_mid_c16;
static HaloEase DV_EASE;
// "45 min" (by 5), "1 h 30 min" (by 10), "8 h": a forecast, rounded down
static void dv_about(char *out, int n, float s) {
  const int m = (int)(s / 60);
  if (m < 60) snprintf(out, n, "%d min", max(5, m / 5 * 5));
  else if (m < 120) m % 60 >= 10 ? snprintf(out, n, "1 h %d min", m % 60 / 10 * 10) : snprintf(out, n, "1 h");
  else snprintf(out, n, "%d h", m / 60);
}

#ifdef __EMSCRIPTEN__
// The editor: made-up values, smooth noise of time (the same every time)
static float dv_hash(float x) {
  const float s = sinf(x * 127.1f + 311.7f) * 43758.5453f;
  return s - floorf(s);
}
static float dv_noise(float t) {
  const float i = floorf(t), f = t - i, u = f * f * (3 - 2 * f);
  return dv_hash(i) * (1 - u) + dv_hash(i + 1) * u;
}
static float dv_voice(float t) {  // 0..1: words of a few syllables, pauses between
  const float L = 1.9f, w = floorf(t / L), tt = t - w * L, len = 0.7f + dv_hash(w * 3.1f) * 0.8f;
  if (dv_hash(w * 7.7f) < 0.22f || tt > len) return 0.04f + 0.05f * dv_noise(t * 17);
  const float n = 2 + floorf(dv_hash(w * 1.3f) * 4), ph = tt / len * n, s = floorf(ph);
  const float e = powf(sinf(HALO_PI * (ph - s)), 0.6f) * (0.45f + 0.55f * dv_hash(w * 5 + s));
  return fminf(1, e * (0.85f + 0.15f * dv_noise(t * 40)) + 0.04f);
}
#endif

// ---------- The board's numbers ----------
#if HAS_ABOUT
struct DvHist {
  float v[DV_HN];
  uint32_t n;  // samples so far (the newest: (n − 1) % DV_HN)
};
// The load of each core, the free internal memory, the chip's temperature — in PSRAM (2 KB: internal RAM is tight)
static DvHist *dv_hist;
#define dv_load0 dv_hist[0]
#define dv_load1 dv_hist[1]
#define dv_free dv_hist[2]
#define dv_temp dv_hist[3]
static float dv_heap_total = 330, dv_heap_low, dv_ps_free, dv_ps_total = 8192;  // KB
static int dv_tasks, dv_mhz = 240;
static volatile uint32_t dv_gen, dv_tgen;  // a new sample came (the charts are baked again): the load and the memory;
                                           // the temperature
static void dv_push(DvHist &h, float v) {
  h.v[h.n % DV_HN] = v;
  h.n++;
}
static float dv_last(const DvHist &h, int back = 0) {
  if (!h.n) return 0;
  return h.v[(h.n - 1 - min((uint32_t)back, h.n - 1)) % DV_HN];
}
// The last n samples (oldest first) into out; how many there were
static int dv_series(const DvHist &h, float *out, int n) {
  const int k = (int)min((uint32_t)n, min(h.n, (uint32_t)DV_HN));
  for (int i = 0; i < k; i++) out[i] = dv_last(h, k - 1 - i);
  return k;
}

#ifdef __EMSCRIPTEN__
static void dv_sample_at(float t, bool temp) {
  dv_push(dv_load0, fminf(100, 9 + 9 * dv_noise(t * 1.1f + 3) + 60 * fmaxf(0, dv_noise(t * 0.31f + 40) - 0.68f)));
  dv_push(dv_load1, fminf(100, 24 + 22 * dv_noise(t * 0.8f + 7) + 6 * sinf(t * 2.1f)));
  dv_push(dv_free, 97 + 15 * dv_noise(t * 0.5f + 21));
  dv_heap_total = 330, dv_heap_low = 61;
  dv_ps_free = 7230 + 70 * dv_noise(t * 0.3f + 33), dv_ps_total = 8192, dv_tasks = 9, dv_mhz = 240;
  if (temp) dv_push(dv_temp, 45.4f + 1.3f * dv_noise(t * 0.07f + 3) + 0.3f * dv_noise(t * 0.9f + 1) +
                                 dv_last(dv_load1) * 0.012f);
}
#endif

// Every 250 ms (the board: from loop(); the editor: from the frame)
static void dv_sample() {
  static uint32_t at, t_at;
#ifdef __EMSCRIPTEN__
  const uint32_t now = halo_ms();
  if (!dv_load0.n) {  // (the charts full from the start: the last 32 s and 4 min made up)
    for (int k = DV_HN - 1; k >= 1; k--) dv_sample_at(now / 1000.0f - k * 2.0f, true);  // (the temperature's 4 min)
    for (int k = DV_HN - 1; k >= 1; k--) dv_sample_at(now / 1000.0f - k * 0.25f, false);
  }
  if (now - at < 250 && at) return;
  const bool temp = now - t_at >= 2000 || !t_at;
  at = now | 1;
  if (temp) t_at = now | 1, dv_tgen++;
  dv_sample_at(now / 1000.0f, temp);
#else
  static uint32_t idle_prev[2], us_prev;
  const uint32_t now = millis();
  if (now - at < 250 && at) return;
  at = now | 1;
  const uint32_t us = (uint32_t)esp_timer_get_time();
  float load[2] = {0, 0};
  for (int c = 0; c < 2; c++) {
    const uint32_t idle = ulTaskGetIdleRunTimeCounterForCore(c);
    if (us_prev) load[c] = 100 * halo_clamp01(1 - (float)(idle - idle_prev[c]) / (float)(us - us_prev));
    idle_prev[c] = idle;
  }
  const bool first = !us_prev;
  us_prev = us;
  if (first) return;
  dv_push(dv_load0, load[0]), dv_push(dv_load1, load[1]);
  dv_push(dv_free, heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024.0f);
  dv_heap_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024.0f;
  dv_heap_low = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) / 1024.0f;
  dv_ps_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024.0f;
  dv_ps_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024.0f;
  if (now - t_at >= 2000 || !t_at) {
    t_at = now | 1;
    dv_push(dv_temp, temperatureRead());
    dv_tasks = uxTaskGetNumberOfTasks(), dv_mhz = getCpuFrequencyMhz();
    dv_tgen++;
  }
#endif
  dv_gen++;
}

// ---------- The microphones and the motion sensor: only while something shows them ----------
#define DV_NB 16
#define DV_BLOCK 512
static float dv_band[DV_NB], dv_peak[DV_NB], dv_hold[DV_NB];  // 0..1; a peak holds 0.25 s, then falls
static float dv_gcoef[DV_NB];
static float dv_db = -90, dv_db1 = -90, dv_db2 = -90;
static volatile bool dv_mic_on, dv_sens_busy, dv_imu_want, dv_imu_on;  // (dv_mic_want — settings.h: the Recorder's)
static int dv_rate = 22050;
static uint32_t dv_mic_seen, dv_imu_seen;  // when last on screen (0 — never: not "600 ms ago" at start)
static float dv_ax, dv_ay, dv_az = 1, dv_imu_t;
static bool dv_imu_t_ok;  // (the sensor's temperature read at least once)
static void dv_coefs(int rate) {  // bands from 120 Hz to 7 kHz, evenly on a log scale
  for (int b = 0; b < DV_NB; b++)
    dv_gcoef[b] = 2 * cosf(2 * HALO_PI * 120 * powf(7000 / 120.0f, b / (DV_NB - 1.0f)) / rate);
}
// A spectrum frame (~23 ms): each band's level, −78..−30 dBFS → 0..1 (a voice a metre away fills about half); up
// at once, down 1.7 a second
static void dv_spectrum(const float *lv, float dt) {
  for (int b = 0; b < DV_NB; b++) {
    const float v = halo_clamp01(lv[b]);
    dv_band[b] = fmaxf(v, dv_band[b] - 1.7f * dt);
    if (dv_band[b] >= dv_peak[b]) dv_peak[b] = dv_band[b], dv_hold[b] = 0.25f;
    else if ((dv_hold[b] -= dt) < 0) dv_peak[b] = fmaxf(dv_band[b], dv_peak[b] - 1.0f * dt);
  }
}
#ifndef __EMSCRIPTEN__
static int16_t *dv_mbuf;  // (PSRAM)
static int dv_mn;
static void dv_mic_block() {
  float lv[DV_NB];
  const float ref = 20 * log10f(DV_BLOCK / 2.0f * 32768);  // a full-scale sine's magnitude
  for (int b = 0; b < DV_NB; b++) {
    const float k = dv_gcoef[b];
    float s1 = 0, s2 = 0;
    for (int i = 0; i < DV_BLOCK; i++) {
      const float s0 = dv_mbuf[i] + k * s1 - s2;
      s2 = s1, s1 = s0;
    }
    const float p = s1 * s1 + s2 * s2 - k * s1 * s2;
    lv[b] = (10 * log10f(p + 1) - ref + 78) * (1 / 48.0f);
  }
  dv_spectrum(lv, DV_BLOCK / (float)dv_rate);
}
// The microphones and the motion sensor on or off as dv_mic_want and dv_imu_want say — in a task (I2C: ~4–8 ms for
// the microphones, ~3–20 ms for the motion sensor — not in a frame), one at a time. The Recorder's are the same
// microphones: whoever still wants them keeps them on
static void dv_sens_sync() {
  if (dv_sens_busy || (dv_mic_want == dv_mic_on && dv_imu_want == dv_imu_on)) return;
  dv_sens_busy = true;
  xTaskCreatePinnedToCore(
      [](void *) {
        while (dv_mic_want != dv_mic_on || dv_imu_want != dv_imu_on) {
          if (dv_mic_want && !dv_mic_on) {
            const int r = halo_mic_begin(22050);  // (the music's rate: the bus is one, the speaker plays at it)
            if (r > 0) dv_rate = r, dv_coefs(r);
            dv_mic_on = true;
          } else if (!dv_mic_want && dv_mic_on) {
#if HAS_RECORDER
            if (!rc_mic_want)
#endif
              halo_mic_end();
            dv_mic_on = false;
          }
          if (dv_imu_want && !dv_imu_on) dv_imu_on = halo_imu_begin() || true;  // (not there: no data, no retries)
          else if (!dv_imu_want && dv_imu_on) dv_imu_on = false, halo_imu_end();
        }
        dv_sens_busy = false;
        vTaskDelete(nullptr);
      },
      "dv_sens", 3072, nullptr, 2, nullptr, 0);
}
#endif

static void dv_sensors(float dt) {
  // Always On (from the start of the move into it, power.h: fc_calm): the sensors off — the board sleeps there (the
  // microphones would keep it up), and the motion sensor must be back to waking it when it is picked up
  const uint32_t now = halo_ms();
  dv_mic_want = !fc_calm && dv_mic_seen && now - dv_mic_seen < 600;
#ifdef __EMSCRIPTEN__
  const float t = now / 1000.0f, e = dv_voice(t);
  float lv[DV_NB];
  const float L = 1.9f, w = floorf(t / L), s = floorf(fmodf(t, L) * 4);
  const float fa = 0.08f + 0.15f * dv_hash(w * 2.3f + s), fb = 0.36f + 0.25f * dv_hash(w * 4.1f + s + 0.5f);
  for (int b = 0; b < DV_NB; b++) {
    const float x = (b + 0.5f) / DV_NB;
    const float g = expf(-(x - fa) * (x - fa) / 0.006f) + 0.7f * expf(-(x - fb) * (x - fb) / 0.01f) +
                    0.25f * expf(-(x - 0.8f) * (x - 0.8f) / 0.01f);
    lv[b] = e * (0.3f + 1.15f * g) * (0.8f + 0.4f * dv_noise(t * 25 + b * 3.7f)) + 0.03f * (1 - x);
  }
  dv_spectrum(lv, dt);
  dv_db = -71 + 56 * e, dv_db1 = dv_db - 1.5f - dv_noise(t * 3), dv_db2 = dv_db - 2.5f - dv_noise(t * 3 + 9);
  dv_ax = 0.17f * sinf(t * 0.53f) + 0.05f * sinf(t * 1.9f + 2), dv_ay = 0.14f * sinf(t * 0.41f + 1.2f) + 0.04f * sinf(t * 1.3f);
  dv_az = sqrtf(fmaxf(0, 1 - dv_ax * dv_ax - dv_ay * dv_ay)) + 0.004f * dv_noise(t * 7);
  dv_imu_t = 31.3f + 0.6f * dv_noise(t * 0.04f + 19), dv_imu_t_ok = true;
#else
  dv_imu_want = !fc_calm && dv_imu_seen && now - dv_imu_seen < 600;
  dv_sens_sync();
  if (dv_mic_on) {
    int got;
    while ((got = halo_mic_read(dv_mbuf + dv_mn, DV_BLOCK - dv_mn)) > 0)
      if ((dv_mn += got) == DV_BLOCK) dv_mic_block(), dv_mn = 0;
    float a, b;
    halo_mic_db2(a, b);
    const float k = 1 - expf(-dt / 0.06f);  // (smoothed: the blocks are 12–16 ms)
    dv_db += (halo_mic_db() - dv_db) * k, dv_db1 += (a - dv_db1) * k, dv_db2 += (b - dv_db2) * k;
  } else {
    dv_mn = 0, dv_db = dv_db1 = dv_db2 = -90;
    float zero[DV_NB] = {};
    dv_spectrum(zero, dt);
  }
  HaloImu m;
  if (dv_imu_on && halo_imu_read(m)) {
    const float k = 1 - expf(-dt / 0.08f);
    dv_ax += (m.ax - dv_ax) * k, dv_ay += (m.ay - dv_ay) * k, dv_az += (m.az - dv_az) * k;
    dv_imu_t = dv_imu_t_ok ? dv_imu_t + (m.temp - dv_imu_t) * k : m.temp, dv_imu_t_ok = true;
  }
#endif
}
static void dv_seen(int what) {  // a card or a screen of this is on screen now: its sensors on
  if (what == DV_SND) dv_mic_seen = halo_ms() | 1;
  if (what == DV_MOT) dv_imu_seen = halo_ms() | 1;
}
#endif  // HAS_ABOUT

// ---------- The power chip: a forecast by the pace since USB came or went (Power) ----------
#if HAS_CHARGE
static int ch_ref_pct = -1;
static uint32_t ch_ref_s;
static bool ch_ref_usb;
static HaloBattery ch_battery() {
#ifdef __EMSCRIPTEN__
  return {true, true, true, true, 89, 4118, 5050, 4970, 384};  // (the editor: charging — the mockup's state)
#else
  return halo_battery();
#endif
}
static void ch_poll() {  // every 5 s from loop(): where the percent was when USB came or went
  static uint32_t at;
  if (millis() - at < 5000 && at) return;
  at = millis() | 1;
  const HaloBattery b = ch_battery();
  if (!b.ok || !b.present || b.percent < 0) return void(ch_ref_pct = -1);
  if (ch_ref_pct < 0 || b.usb != ch_ref_usb) ch_ref_pct = b.percent, ch_ref_s = millis() / 1000, ch_ref_usb = b.usb;
}
// "full in about 40 min", "about 8 h left"; nothing known yet — ""
static void ch_forecast(const HaloBattery &b, char *out, int n) {
  *out = 0;
#ifdef __EMSCRIPTEN__
  if (b.charging) snprintf(out, n, "full in about 40 min");
#else
  if (ch_ref_pct < 0 || b.usb != ch_ref_usb) return;
  const uint32_t s = millis() / 1000 - ch_ref_s;
  const int moved = b.usb ? b.percent - ch_ref_pct : ch_ref_pct - b.percent;
  if (moved < 2 || s < 300) return;
  char a[24];
  if (b.usb && b.charging) dv_about(a, sizeof(a), (100 - b.percent) * s / (float)moved), snprintf(out, n, "full in about %s", a);
  else if (!b.usb) dv_about(a, sizeof(a), b.percent * s / (float)moved), snprintf(out, n, "about %s left", a);
#endif
}
#endif

// ---------- About's card (a gallery item: gallery.h lays it out as a card, by its place in the list) ----------
#if HAS_ABOUT
// Its value: "29%", "106 KB", "89%", "47.0°", "−69 dB", "1.00 g"
static void dc_value(int what, char *s, int n) {
  switch (what) {
    case DV_CPU: snprintf(s, n, "%d%%", (int)lroundf((dv_last(dv_load0) + dv_last(dv_load1)) / 2)); break;
    case DV_MEM: snprintf(s, n, "%d KB", (int)lroundf(dv_last(dv_free))); break;
    case DV_BAT: {
#ifdef __EMSCRIPTEN__
      snprintf(s, n, "89%%");
#else
      const HaloBattery b = halo_battery();
      b.ok && b.present && b.percent >= 0 ? snprintf(s, n, "%d%%", b.percent) : snprintf(s, n, "--");
#endif
      break;
    }
    case DV_TEMP: snprintf(s, n, "%.1f\xC2\xB0", dv_last(dv_temp)); break;
    case DV_SND: snprintf(s, n, "\xE2\x88\x92%d dB", (int)lroundf(fminf(90, -dv_db))); break;
    default: snprintf(s, n, "%.2f g", sqrtf(dv_ax * dv_ax + dv_ay * dv_ay + dv_az * dv_az)); break;
  }
}
// A chart's values as places in a box h tall (0 — the top): fixed range lo..hi, or (lo ≥ hi) around the values,
// at least `span` tall
static void dv_places(float *v, int n, float h, float lo, float hi, float span, float pad) {
  if (lo >= hi) {
    lo = 1e9f, hi = -1e9f;
    for (int i = 0; i < n; i++) lo = fminf(lo, v[i]), hi = fmaxf(hi, v[i]);
    if (hi - lo < span) lo = (lo + hi - span) / 2, hi = lo + span;
    const float m = (hi - lo) * 0.15f;
    lo -= m, hi += m;
  }
  for (int i = 0; i < n; i++) v[i] = pad + (h - 2 * pad) * (1 - halo_clamp01((v[i] - lo) / (hi - lo)));
}
// A chart of this what into d's chart (w × h), when a new sample came; detail — the screen's (the processor: both
// cores, 30 s), else the card's (the average, 20 s)
static void dv_chart(DvRun &d, int what, int w, int h, bool detail) {
  const uint32_t gen = what == DV_TEMP ? dv_tgen : dv_gen;
  if (d.chart.x >= 0) return dv_bake_step(d.chart, DV_COLS);  // (a bake in progress: its next columns)
  if (d.chart.cur >= 0 && d.chart.gen == gen && d.chart.what == what) return;
  float a[DV_HN], b[DV_HN];
  int n = 0, slots = 0;
  const float pad = 4;
  if (what == DV_CPU) {
    slots = detail ? 120 : 80;
    n = dv_series(dv_load1, a, slots);
    dv_series(dv_load0, b, slots);
    if (!detail)
      for (int i = 0; i < n; i++) a[i] = (a[i] + b[i]) / 2;
    dv_places(a, n, h, 0, detail ? 100 : 0, 30, pad);
    if (detail) dv_places(b, n, h, 0, 100, 0, pad);
  } else if (what == DV_MEM) {
    slots = 120, n = dv_series(dv_free, a, slots);
    dv_places(a, n, h, 0, 0, 16, pad);
  } else {
    slots = detail ? 120 : 60, n = dv_series(dv_temp, a, slots);
    dv_places(a, n, h, 0, 0, 2, pad);
  }
  if (n < 1) return;
  dv_bake_start(d.chart, w, h, a, what == DV_CPU && detail ? b : nullptr, n, slots);
  d.chart.next_gen = gen, d.chart.next_what = what;
  dv_bake_step(d.chart, DV_COLS);
}

static void dc_layout(const FaceScreen &S, int i, float a, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  R.box[0] = R.box[1] = 0, R.box[2] = R.box[3] = -1;
  DvRun *d = R.dv;
  if (!d) return;
  HaloItem *it = d->it;
  const GalRun *g = S.gal;
  const int k = g ? gal_item_of(*g, i) : -1, what = dv_what(L);
  int n = 0;
  if (k >= 0) {
    const HaloListFx &f = g->list.fx[k];
    const bool vis = halo_list_item_visible(g->list, k);
    const float s = f.s, cy = f.top + GL_ITEM_H * s / 2, ac = vis ? f.a_c * a : 0, p = halo_clamp01(g->press[k].s.x);
    if (vis && a > 0 && v.a > 0.01f) dv_seen(what);
    auto X = [&](float dx) { return CX + dx * s; };
    auto Y = [&](float dy) { return cy + dy * s; };
    HaloItem &bg = it[n++];
    bg.kind = HI_RRECT, bg.x = CX, bg.y = cy, bg.w = GL_ITEM_W, bg.h = GL_ITEM_H, bg.scale = s, bg.r = 52 + (36 - 52) * p;
    bg.color = L.color2, bg.alpha = vis ? f.a_bg * a : 0, bg.on_black = true, bg.on = nullptr;
    HaloItem &ic = it[n++];
    ic.kind = HI_CIRCLE, ic.x = X(-159), ic.y = Y(-21), ic.r = 18, ic.scale = s, ic.color = dv_mix(L.color2, L.color, 0.2f);
    ic.alpha = ac, ic.on_black = true, ic.on = nullptr;
    dv_text(it[n++], gl_ic24, gl_str(L.icon), gl_mid_icon, X(-159), Y(-21), 0.7f * s, L.color, ac, 1, &ic);
    dv_text(it[n++], gl_t20, gl_str(L.text), dv_mid_t20, X(-131), Y(-21), s, DV_DIM, ac, 0, &bg);
    if (!d->text_at || (!fc_calm && halo_ms() - d->text_at >= 500)) d->text_at = halo_ms() | 1, dc_value(what, d->val, sizeof(d->val));
    dv_text(it[n++], gl_n34, d->val, dv_mid_n34, X(-173), Y(19), s, DV_TEXT, ac, 0, &bg);
    // The picture on the right: 142 × 38 from (30, −6) of the card's middle
    const float x0 = 30, x1 = 172, y0 = -6, y1 = 32;
    if (what == DV_CPU || what == DV_TEMP) {
      if (!fc_calm || d->chart.cur < 0) dv_chart(*d, what, (int)(x1 - x0), (int)(y1 - y0), false);
      HaloItem &m = it[n++];
      m.kind = HI_MASK, m.mask = d->chart.cur >= 0 ? &d->chart.m[d->chart.cur] : nullptr, m.color = L.color;
      m.x = X((x0 + x1) / 2), m.y = Y((y0 + y1) / 2), m.scale = s, m.alpha = m.mask ? ac : 0, m.on = &bg, m.on_black = false;
      HaloItem &e = it[n++];
      e.kind = HI_CIRCLE, e.x = X(x1), e.y = Y(y0 + d->chart.last_y), e.r = 4, e.scale = s, e.color = L.color;
      e.alpha = m.mask ? ac : 0, e.on_black = true, e.on = nullptr;
    } else if (what == DV_MEM) {
      static const char *const LAB[2] = {"RAM", "PSRAM"};
      const float used[2] = {1 - dv_last(dv_free) / fmaxf(1, dv_heap_total), 1 - dv_ps_free / fmaxf(1, dv_ps_total)};
      for (int j = 0; j < 2; j++) {
        const float y = j ? 24 : 2, bx = x0 + 66, bw = x1 - bx, fw = fmaxf(7, bw * halo_clamp01(used[j]));
        dv_text(it[n++], gl_c16, LAB[j], dv_mid_c16, X(x0), Y(y), s, DV_DIM, ac, 0, &bg);
        HaloItem &t = it[n++];
        t.kind = HI_RRECT, t.x = X(bx + bw / 2), t.y = Y(y), t.w = bw, t.h = 7, t.r = 3.5f, t.scale = s;
        t.color = dv_mix(L.color2, DV_TEXT, 0.1f), t.alpha = ac, t.on_black = true, t.on = nullptr;
        HaloItem &u = it[n++];
        u.kind = HI_RRECT, u.x = X(bx + fw / 2), u.y = Y(y), u.w = fw, u.h = 7, u.r = 3.5f, u.scale = s;
        u.color = j ? DV_PSRAM : L.color, u.alpha = ac, u.on_black = true, u.on = nullptr;
      }
    } else if (what == DV_BAT) {
#ifdef __EMSCRIPTEN__
      const HaloBattery b = {true, true, true, true, 89, 4118, 5050, 4970, 384};
#else
      const HaloBattery b = halo_battery();
#endif
      const bool has = b.ok && b.present && b.percent >= 0;
      HaloItem &r = it[n++];
      r.kind = HI_RING, r.x = X(x1 - 26), r.y = Y(7), r.r = 22, r.w = 7, r.v = has ? b.percent / 100.0f : 0;
      r.scale = s, r.color = L.color, r.color2 = dv_mix(L.color2, L.color, 0.2f), r.alpha = ac, r.on_black = true, r.on = nullptr;
      dv_text(it[n++], gl_ic24, "\xEF\x83\xA7", gl_mid_icon, X(x1 - 26), Y(7), 0.55f * s, L.color, b.charging ? ac : 0, 1, &bg);
    } else if (what == DV_SND) {
      const float pitch = (x1 - x0) / DV_NB;
      for (int j = 0; j < DV_NB; j++) {
        const float h = 4 + 34 * dv_band[j];
        HaloItem &q = it[n++];
        q.kind = HI_RRECT, q.x = X(x0 + pitch * (j + 0.5f)), q.y = Y(y1 - h / 2), q.w = 5, q.h = h, q.r = 2.5f, q.scale = s;
        q.color = dv_mix(DV_LIL, L.color, dv_band[j]), q.alpha = ac, q.on_black = true, q.on = nullptr;
      }
    } else {  // the motion: a level — a ring, a cross, the bubble going to the high side
      const float cx = x1 - 26, cyy = 7, k2 = 60;
      HaloItem &r = it[n++];
      r.kind = HI_RING, r.x = X(cx), r.y = Y(cyy), r.r = 22, r.w = 2, r.v = 1, r.scale = s;
      r.color = dv_mix(L.color2, DV_TEXT, 0.16f), r.color2 = 0, r.alpha = ac, r.on_black = true, r.on = nullptr;
      for (int j = 0; j < 2; j++) {
        HaloItem &c = it[n++];
        c.kind = HI_RRECT, c.x = X(cx), c.y = Y(cyy), c.w = j ? 1.5f : 44, c.h = j ? 44 : 1.5f, c.r = 0.75f, c.scale = s;
        c.color = dv_mix(L.color2, DV_TEXT, 0.08f), c.alpha = ac, c.on_black = true, c.on = nullptr;
      }
      float bx = -dv_ay * k2, by = -dv_ax * k2;
      const float m = sqrtf(bx * bx + by * by);
      if (m > 15) bx *= 15 / m, by *= 15 / m;
      HaloItem &b = it[n++];
      b.kind = HI_CIRCLE, b.x = X(cx + bx), b.y = Y(cyy + by), b.r = 7, b.scale = s, b.color = L.color, b.alpha = ac;
      b.on_black = true, b.on = nullptr;
    }
    R.ax = CX, R.ay = cy;
  }
  dv_commit(*d, n, v, R.box);
}
static void dc_draw(const FaceScreen &S, int i) {
  const DvRun *d = S.run[i].dv;
  if (!d) return;
  for (int j = 0; j < d->used; j++) halo_item_draw(d->it[j]);
}
#else
static void dc_layout(const FaceScreen &S, int i, float, const HaloView &) {
  S.run[i].box[0] = S.run[i].box[1] = 0, S.run[i].box[2] = S.run[i].box[3] = -1;
}
static void dc_draw(const FaceScreen &, int) {}
#endif

// ---------- A card's own screen (FL_DEVICE) and Power (FL_CHARGE) ----------
#define DV_TITLE_Y 92.0f
#define DV_BIG_Y 160.0f
#define DV_SUB_Y 212.0f
#define DV_INFO_Y 346.0f
#define DV_CH_X0 130.0f  // the chart, the bars, the level: x 130..336, y 238..318
#define DV_CH_X1 336.0f
#define DV_CH_Y0 238.0f
#define DV_CH_Y1 318.0f
#define DV_ARC_R 212.0f
#define DV_ARC_W 12.0f
#define DV_LAB_R 212.0f
#define CH_RING_R 224.0f
#define CH_RING_W 7.0f
#define CH_SEGS 8  // the level in 45° pieces: as it grows, only the piece with its end is drawn again
static const float DV_DEG = HALO_PI / 180;

// The words along the arc at the bottom: baked again when they change, one a frame (~3 ms each). Only with About or
// Power: their font gl_c16 is in the firmware only then (a project without them didn't compile — 6 Nov 2026)
static bool dv_baked_one;
#if HAS_ABOUT || HAS_CHARGE
static void dv_label(DvRun &d, int j, const char *s, float at_deg) {
  if (!strcmp(d.lab_on[j], s) || dv_baked_one || (d.lab_on[j][0] && halo_ms() - d.lab_at[j] < 1000)) return;
  dv_baked_one = true, d.lab_at[j] = halo_ms();
  strncpy(d.lab_on[j], s, sizeof(d.lab_on[j]) - 1);
  if (!halo_text_arc(d.lab[j], gl_c16, s, dv_mid_c16, DV_LAB_R, at_deg * DV_DEG, true, 2, &d.lab_x[j], &d.lab_y[j]))
    free((void *)d.lab[j].bits), free((void *)d.lab[j].ext), d.lab[j] = {};
}
static void dv_words(HaloItem &it, const HaloMask &m, float x, float y) {
  it.kind = HI_MASK, it.mask = &m, it.color = DV_DIM, it.on_black = true, it.on = &halo_black, it.scale = 1;
  it.x = x + m.w / 2.0f, it.y = y + m.h / 2.0f, it.alpha = m.bits ? 1 : 0;
}
#endif
static HaloItem &dv_arc(HaloItem &it, float a0, float a1, uint32_t color) {  // degrees
  it.kind = HI_ARC, it.x = it.y = CX, it.r = DV_ARC_R, it.w = DV_ARC_W, it.scale = 1, it.color = color;
  it.a0 = a0 * DV_DEG, it.a1 = fmaxf(a0, a1) * DV_DEG, it.alpha = a1 - a0 > 0.2f ? 1 : 0, it.on_black = true, it.on = nullptr;
  return it;
}

#if HAS_ABOUT
static const char *const DV_TITLE[DV_N] = {"Processor", "Memory", "Battery", "Temperature", "Sound", "Motion"};
// The screen's words, twice a second: the big number, the line under it, the line under the picture, the two values
// along the arc; and the arcs' shares (0..1)
static void dv_texts(DvRun &d, int what, float &fl, float &fr) {
  char l0[32], l1[32];
  const float t = dv_last(dv_temp);
  switch (what) {
    case DV_CPU: {
      const float a = dv_last(dv_load0), b = dv_last(dv_load1);
      fl = a / 100, fr = b / 100;
      snprintf(d.big, sizeof(d.big), "%d%%", (int)lroundf((a + b) / 2));
      snprintf(d.sub, sizeof(d.sub), "2 cores \xC2\xB7 %d MHz", dv_mhz);
      snprintf(d.info, sizeof(d.info), "%.1f \xC2\xB0" "C \xC2\xB7 %d tasks", t, dv_tasks);
      snprintf(l0, sizeof(l0), "CORE 0 \xC2\xB7 %d%%", (int)lroundf(a)), snprintf(l1, sizeof(l1), "CORE 1 \xC2\xB7 %d%%", (int)lroundf(b));
      break;
    }
    case DV_MEM: {
      const float fr_ = dv_last(dv_free);
      fl = 1 - fr_ / fmaxf(1, dv_heap_total), fr = 1 - dv_ps_free / fmaxf(1, dv_ps_total);
      snprintf(d.big, sizeof(d.big), "%d", (int)lroundf(fr_));
      snprintf(d.sub, sizeof(d.sub), "KB free \xC2\xB7 lowest %d KB", (int)lroundf(dv_heap_low));
      snprintf(d.info, sizeof(d.info), "PSRAM %.1f of %d MB free", dv_ps_free / 1024, (int)lroundf(dv_ps_total / 1024));
      snprintf(l0, sizeof(l0), "RAM \xC2\xB7 %d%%", (int)lroundf(fl * 100)), snprintf(l1, sizeof(l1), "PSRAM \xC2\xB7 %d%%", (int)lroundf(fr * 100));
      break;
    }
    case DV_TEMP: {
#ifdef __EMSCRIPTEN__
      const float pmu = 38.4f;
#else
      const float pmu = halo_battery().temp10 / 10.0f;
#endif
      fl = halo_clamp01((t - 20) / 60), fr = halo_clamp01((pmu - 20) / 60);
      snprintf(d.big, sizeof(d.big), "%.1f\xC2\xB0", t);
      snprintf(d.sub, sizeof(d.sub), "ESP32-S3 chip");
      snprintf(d.info, sizeof(d.info), "Power chip %.1f \xC2\xB0" "C", pmu);
      snprintf(l0, sizeof(l0), "CHIP \xC2\xB7 %d\xC2\xB0", (int)lroundf(t)), snprintf(l1, sizeof(l1), "POWER \xC2\xB7 %d\xC2\xB0", (int)lroundf(pmu));
      break;
    }
    case DV_BAT: {  // (About's battery card opens Power; this — a screen set to the battery by hand)
#ifdef __EMSCRIPTEN__
      const HaloBattery b = {true, true, true, true, 89, 4118, 5050, 4970, 384};
#else
      const HaloBattery b = halo_battery();
#endif
      fl = b.present && b.percent >= 0 ? b.percent / 100.0f : 0, fr = halo_clamp01((b.temp10 / 10.0f - 20) / 60);
      b.present && b.percent >= 0 ? snprintf(d.big, sizeof(d.big), "%d%%", b.percent) : snprintf(d.big, sizeof(d.big), "--");
      snprintf(d.sub, sizeof(d.sub), "%.2f V Â· %s", b.mv / 1000.0f, b.charging ? "charging" : b.usb ? "on USB" : "on battery");
      snprintf(d.info, sizeof(d.info), "System %.2f V", b.vsys_mv / 1000.0f);
      snprintf(l0, sizeof(l0), "BATTERY Â· %d%%", (int)lroundf(fl * 100));
      snprintf(l1, sizeof(l1), "POWER CHIP Â· %dÂ°", (int)lroundf(b.temp10 / 10.0f));
      break;
    }
    case DV_SND: {
      fl = halo_clamp01((dv_db1 + 80) / 70), fr = halo_clamp01((dv_db2 + 80) / 70);
      snprintf(d.big, sizeof(d.big), "\xE2\x88\x92%d", (int)lroundf(fminf(90, -dv_db)));
      snprintf(d.sub, sizeof(d.sub), "dB \xC2\xB7 2 microphones");
      snprintf(d.info, sizeof(d.info), "Speaker %d%%", halo_audio_get_volume());
      snprintf(l0, sizeof(l0), "MIC 1 \xC2\xB7 \xE2\x88\x92%d", (int)lroundf(fminf(90, -dv_db1)));
      snprintf(l1, sizeof(l1), "MIC 2 \xC2\xB7 \xE2\x88\x92%d", (int)lroundf(fminf(90, -dv_db2)));
      break;
    }
    default: {  // the motion: the tilt from the axes (+X — the top of the screen, +Y — left)
      const float tx = asinf(fmaxf(-1, fminf(1, dv_ax))) / DV_DEG, ty = asinf(fmaxf(-1, fminf(1, dv_ay))) / DV_DEG;
      fl = halo_clamp01(fabsf(tx) / 45), fr = halo_clamp01(fabsf(ty) / 45);
#if HAS_STEPS
      snprintf(d.big, sizeof(d.big), "%s", fc_steps_s);
      snprintf(d.sub, sizeof(d.sub), "steps today");
#else
      snprintf(d.big, sizeof(d.big), "%.2f", sqrtf(dv_ax * dv_ax + dv_ay * dv_ay + dv_az * dv_az));
      snprintf(d.sub, sizeof(d.sub), "g \xC2\xB7 QMI8658A");
#endif
      if (dv_imu_t_ok) snprintf(d.info, sizeof(d.info), "Motion sensor %.1f \xC2\xB0" "C", dv_imu_t);
      else snprintf(d.info, sizeof(d.info), "Motion sensor");
      snprintf(l0, sizeof(l0), "TILT X \xC2\xB7 %d\xC2\xB0", (int)lroundf(tx)), snprintf(l1, sizeof(l1), "TILT Y \xC2\xB7 %d\xC2\xB0", (int)lroundf(ty));
      break;
    }
  }
  strncpy(d.lab_s[0], l0, sizeof(d.lab_s[0]) - 1), strncpy(d.lab_s[1], l1, sizeof(d.lab_s[1]) - 1);
}

static void dv_detail_layout(const FaceLayer &L, DvRun &d, const HaloView &v) {
  HaloItem *it = d.it;
  const int what = dv_what(L);
  const uint32_t acc = L.color, acc2 = what == DV_MEM ? DV_PSRAM : acc;
  if (v.a > 0.01f) dv_seen(what);
  static float fl[DV_N], fr[DV_N];
  if (!d.text_at || (!fc_calm && halo_ms() - d.text_at >= 500)) d.text_at = halo_ms() | 1, dv_texts(d, what, fl[what], fr[what]);
  const float k = halo_ease(DV_EASE, (halo_ms() - d.enter_ms) / 700.0f);  // the arcs grow as it opens
  int n = 0;
  dv_text(it[n++], gl_t24, DV_TITLE[what], dv_mid_t24, CX, DV_TITLE_Y, 1, DV_TEXT, 1, 1, &halo_black);
  dv_text(it[n++], gl_n80, d.big, dv_mid_n80, CX, DV_BIG_Y, 1, DV_TEXT, 1, 1, &halo_black);
  dv_text(it[n++], gl_t20, d.sub, dv_mid_t20, CX, DV_SUB_Y, 1, DV_DIM, 1, 1, &halo_black);
  dv_text(it[n++], gl_t20, d.info, dv_mid_t20, CX, DV_INFO_Y, 1, DV_TEXT, 1, 1, &halo_black);
  const uint32_t track = dv_mix(0, acc, 0.16f), track2 = dv_mix(0, acc2, 0.16f);
  dv_arc(it[n++], 232, 328, track), dv_arc(it[n++], 232, 232 + 96 * fl[what] * k, acc);
  dv_arc(it[n++], 32, 128, track2), dv_arc(it[n++], 128 - 96 * fr[what] * k, 128, acc2);
  dv_label(d, 0, d.lab_s[0], 206), dv_label(d, 1, d.lab_s[1], 154);
  dv_words(it[n++], d.lab[0], d.lab_x[0], d.lab_y[0]);
  dv_words(it[n++], d.lab[1], d.lab_x[1], d.lab_y[1]);
  const float w = DV_CH_X1 - DV_CH_X0, h = DV_CH_Y1 - DV_CH_Y0, cx = (DV_CH_X0 + DV_CH_X1) / 2, cy = (DV_CH_Y0 + DV_CH_Y1) / 2;
  if (what == DV_CPU || what == DV_MEM || what == DV_TEMP) {
    if (!fc_calm || d.chart.cur < 0) dv_chart(d, what, (int)w, (int)h, true);
    for (const float y : {DV_CH_Y0, cy, DV_CH_Y1}) {  // the grid
      HaloItem &g = it[n++];
      g.kind = HI_RRECT, g.x = cx, g.y = y, g.w = w, g.h = 1.5f, g.r = 0.75f, g.scale = 1, g.color = 0x202226;
      g.alpha = 1, g.on_black = true, g.on = nullptr;
    }
    HaloItem &m = it[n++];
    m.kind = HI_MASK, m.mask = d.chart.cur >= 0 ? &d.chart.m[d.chart.cur] : nullptr, m.color = acc, m.x = cx, m.y = cy;
    m.scale = 1, m.alpha = m.mask ? 1 : 0, m.on = &halo_black, m.on_black = true;
    for (int j = 0; j < (what == DV_CPU ? 2 : 1); j++) {
      HaloItem &e = it[n++];
      e.kind = HI_CIRCLE, e.x = DV_CH_X1, e.y = DV_CH_Y0 + (j ? d.chart.last_y2 : d.chart.last_y), e.r = j ? 3.5f : 4.5f;
      e.scale = 1, e.color = acc, e.alpha = m.mask ? (j ? 0.6f : 1) : 0, e.on_black = true, e.on = nullptr;
    }
  } else if (what == DV_SND) {  // the spectrum: a bar a band, its peak a dot above
    const float pitch = w / DV_NB;
    for (int j = 0; j < DV_NB; j++) {
      const float bh = 6 + (h - 14) * dv_band[j], x = DV_CH_X0 + pitch * (j + 0.5f);
      HaloItem &q = it[n++];
      q.kind = HI_RRECT, q.x = x, q.y = DV_CH_Y1 - bh / 2, q.w = 7, q.h = bh, q.r = 3.5f, q.scale = 1;
      q.color = dv_mix(DV_LIL, acc, dv_band[j]), q.alpha = 1, q.on_black = true, q.on = nullptr;
      HaloItem &pk = it[n++];
      pk.kind = HI_CIRCLE, pk.x = x, pk.y = DV_CH_Y1 - (6 + (h - 14) * dv_peak[j]) - 6, pk.r = 2.5f, pk.scale = 1;
      pk.color = DV_TEXT, pk.alpha = dv_peak[j] > 0.02f ? 0.75f : 0, pk.on_black = true, pk.on = nullptr;
    }
  } else if (what == DV_MOT) {  // the motion: a level — rings, a cross, the bubble to the high side
    const float R0 = 40, kk = 64;
    for (int j = 0; j < 2; j++) {
      HaloItem &r = it[n++];
      r.kind = HI_RING, r.x = CX, r.y = cy, r.r = j ? 12 : R0, r.w = j ? 1.5f : 2, r.v = 1, r.scale = 1;
      r.color = j ? 0x2A2C31 : 0x3A3C42, r.color2 = 0, r.alpha = 1, r.on_black = true, r.on = nullptr;
    }
    for (int j = 0; j < 2; j++) {
      HaloItem &c = it[n++];
      c.kind = HI_RRECT, c.x = CX, c.y = cy, c.w = j ? 1.5f : 2 * R0, c.h = j ? 2 * R0 : 1.5f, c.r = 0.75f, c.scale = 1;
      c.color = 0x2A2C31, c.alpha = 1, c.on_black = true, c.on = nullptr;
    }
    float bx = -dv_ay * kk, by = -dv_ax * kk;
    const float m = sqrtf(bx * bx + by * by);
    if (m > R0 - 12) bx *= (R0 - 12) / m, by *= (R0 - 12) / m;
    HaloItem &b = it[n++];
    b.kind = HI_CIRCLE, b.x = CX + bx, b.y = cy + by, b.r = 10, b.scale = 1, b.color = acc, b.alpha = 1;
    b.on_black = true, b.on = nullptr;
    HaloItem &hl = it[n++];
    hl.kind = HI_CIRCLE, hl.x = CX + bx - 3, hl.y = cy + by - 3.5f, hl.r = 3.5f, hl.scale = 1;
    hl.color = dv_mix(acc, DV_TEXT, 0.6f), hl.alpha = 1, hl.on_black = false, hl.on = nullptr;
  }
  dv_commit(d, n, v, nullptr);
}
#endif

#if HAS_CHARGE
static void ch_layout(DvRun &d, const HaloView &v) {
  HaloItem *it = d.it;
  const HaloBattery b = ch_battery();
  const bool has = b.ok && b.present && b.percent >= 0, low = has && !b.usb && b.percent <= 20;
  const uint32_t lvl = low ? DV_RED : DV_GREEN;
  if (!d.text_at || (!fc_calm && halo_ms() - d.text_at >= 250)) {
    d.text_at = halo_ms() | 1;
    uint32_t sc = DV_DIM;
    const char *st = "On battery";
    if (!b.ok) st = "Reading...";
    else if (!b.present) st = "No battery";
    else if (b.usb && b.charging) st = "Charging", sc = DV_GREEN;
    else if (b.usb) st = b.percent >= 99 ? "Fully charged" : "On USB", sc = b.percent >= 99 ? DV_GREEN : DV_DIM;
    else if (low) st = "Low battery", sc = DV_RED;
    snprintf(d.state, sizeof(d.state), "%s", st);
    d.tone = sc == DV_GREEN ? 1 : sc == DV_RED ? 2 : 0;
    char f[40];
    ch_forecast(b, f, sizeof(f));
    if (!has) snprintf(d.info, sizeof(d.info), b.ok ? "Powered from USB" : "");
    else if (*f) snprintf(d.info, sizeof(d.info), "%.2f V \xC2\xB7 %s", b.mv / 1000.0f, f);
    else snprintf(d.info, sizeof(d.info), "%.2f V", b.mv / 1000.0f);
    b.vbus_mv ? snprintf(d.lab_s[0], sizeof(d.lab_s[0]), "%.2f V", b.vbus_mv / 1000.0f) : snprintf(d.lab_s[0], sizeof(d.lab_s[0]), "\xE2\x80\x94");
    snprintf(d.lab_s[1], sizeof(d.lab_s[1]), "%.2f V", b.vsys_mv / 1000.0f);
    snprintf(d.sub, sizeof(d.sub), "%d\xC2\xB0", (int)lroundf(b.temp10 / 10.0f));
  }
  // The percent counts up as the screen opens, the line along the bezel grows along it (in pieces)
  const float k = halo_ease(DV_EASE, (halo_ms() - d.enter_ms) / 700.0f), shown = (has ? b.percent : 0) * k;
  int n = 0;
  HaloItem &tr = it[n++];
  tr.kind = HI_RING, tr.x = tr.y = CX, tr.r = CH_RING_R, tr.w = CH_RING_W, tr.scale = 1, tr.v = 1;
  tr.color = dv_mix(0, lvl, 0.15f), tr.color2 = 0, tr.alpha = 1, tr.on_black = true, tr.on = nullptr;
  const float end = 2 * HALO_PI * shown / 100, seg = 2 * HALO_PI / CH_SEGS;
  for (int i = 0; i < CH_SEGS; i++) {
    HaloItem &f = it[n++];
    f.kind = HI_ARC, f.x = f.y = CX, f.r = CH_RING_R, f.w = CH_RING_W, f.scale = 1, f.color = lvl, f.on_black = true;
    f.on = nullptr, f.a0 = i * seg, f.a1 = fmaxf(i * seg, fminf(end, (i + 1) * seg)), f.alpha = end > i * seg + 0.001f ? 1 : 0;
  }
  // What is going on (charging — with a bolt), the percent, the voltage and the forecast
  const uint32_t sc = d.tone == 1 ? DV_GREEN : d.tone == 2 ? DV_RED : DV_DIM;
  const bool bolt = b.usb && b.charging;
  const float tw = halo_text_width(gl_t20, d.state), x0 = CX - (tw + (bolt ? 26 : 0)) / 2;
  dv_text(it[n++], gl_ic24, "\xEF\x83\xA7", gl_mid_icon, x0 + 8, 112, 0.7f, sc, bolt ? 1 : 0, 1, &halo_black);
  dv_text(it[n++], gl_t20, d.state, dv_mid_t20, x0 + (bolt ? 26 : 0), 112, 1, sc, 1, 0, &halo_black);
  if (has) snprintf(d.big, sizeof(d.big), "%d%%", (int)lroundf(shown));
  else snprintf(d.big, sizeof(d.big), "--");
  dv_text(it[n++], gl_n80, d.big, dv_mid_n80, CX, 180, 1, has ? DV_TEXT : DV_DIM, 1, 1, &halo_black);
  dv_text(it[n++], gl_t20, d.info, dv_mid_t20, CX, 238, 1, DV_DIM, 1, 1, &halo_black);
  // USB, the system, the power chip
  static const char *const COL[3] = {"USB", "SYSTEM", "POWER CHIP"};
  const char *val[3] = {d.lab_s[0], d.lab_s[1], d.sub};
  for (int j = 0; j < 3; j++) {
    const float x = CX + (j - 1) * 98;
    dv_text(it[n++], gl_c16, COL[j], dv_mid_c16, x, 290, 1, DV_DIM, 1, 1, &halo_black);
    dv_text(it[n++], gl_t20, val[j], dv_mid_t20, x, 318, 1, DV_TEXT, 1, 1, &halo_black);
  }
  dv_text(it[n++], gl_c16, "AXP2101 \xC2\xB7 LI-ION 3.7 V", dv_mid_c16, CX, 366, 1, DV_DIM, 1, 1, &halo_black);
  dv_commit(d, n, v, nullptr);
}
#endif

// ---------- In face.h's screens ----------
static void dv_layout(const FaceScreen &S, int i, const HaloView &v) {
  const FaceLayer &L = S.layers[i];
  FaceRun &R = S.run[i];
  R.box[0] = R.box[1] = 7, R.box[2] = R.box[3] = W - 7;  // (the editor: the whole screen)
  R.ax = R.ay = CX;
  if (!R.dv) return;
#if HAS_CHARGE
  if (L.kind == FL_CHARGE) return ch_layout(*R.dv, v);
#endif
#if HAS_ABOUT
  if (L.kind == FL_DEVICE) return dv_detail_layout(L, *R.dv, v);
#endif
  (void)L, (void)v;
}
static void dv_draw(const FaceScreen &S, int i) {
  const DvRun *d = S.run[i].dv;
  if (!d) return;
  for (int j = 0; j < d->used; j++) halo_item_draw(d->it[j]);
}
static void dv_enter(const FaceScreen &S) {  // opened: the arcs and the percent grow from 0
  for (int i = 0; i < S.n; i++)
    if ((S.layers[i].kind == FL_DEVICE || S.layers[i].kind == FL_CHARGE) && S.run[i].dv)
      S.run[i].dv->enter_ms = halo_ms(), S.run[i].dv->text_at = 0;
}

static void dev_build() {
  dv_mid_t20 = halo_text_mid(gl_t20, "Processor");
  dv_mid_t24 = halo_text_mid(gl_t24, "Processor");
#if HAS_ABOUT || HAS_CHARGE
  dv_mid_n34 = halo_text_mid(gl_n34, "0123456789");
  dv_mid_n80 = halo_text_mid(gl_n80, "0123456789");
  dv_mid_c16 = halo_text_mid(gl_c16, "CORE");
  halo_ease_make(DV_EASE, 0.05f, 0.7f, 0.1f, 1);  // emphasized decelerate (as Battery's)
#endif
#if HAS_ABOUT
  dv_coefs(dv_rate);
#ifdef __EMSCRIPTEN__
  dv_hist = (DvHist *)calloc(4, sizeof(DvHist));
#else
  dv_hist = (DvHist *)heap_caps_calloc(4, sizeof(DvHist), MALLOC_CAP_SPIRAM);
  if (!dv_hist) dv_hist = (DvHist *)calloc(4, sizeof(DvHist));
  dv_mbuf = (int16_t *)heap_caps_malloc(DV_BLOCK * 2, MALLOC_CAP_SPIRAM);
  if (!dv_mbuf) dv_mbuf = (int16_t *)malloc(DV_BLOCK * 2);
#endif
#endif
}
static void dev_tick(float dt) {  // every frame, whatever is on screen
  dv_baked_one = false;
#if HAS_ABOUT
#ifdef __EMSCRIPTEN__
  dv_sample();
#endif
  dv_sensors(dt);
#endif
  (void)dt;
}
static void dev_poll() {  // from loop()
#if HAS_ABOUT && !defined(__EMSCRIPTEN__)
  dv_sample();
#endif
#if HAS_CHARGE
  ch_poll();
#endif
}
