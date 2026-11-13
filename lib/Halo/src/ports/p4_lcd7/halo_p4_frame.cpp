// The P4 board's frame engine. It looks nothing like the S3's, because the hardware is different.
//
// On the S3 the panel has its own memory and the bus is the bottleneck: pixels are drawn in internal memory and raced against
// the panel's scan. Here the panel has no memory: the controller keeps reading a frame buffer, and there are two of them, so
// nothing has to race anything:
//
//   frame boundary -> update(dt) for the frame shown at the NEXT boundary -> draw what changed into the back buffer -> swap
//
// Every part follows from a measurement (boards/p4lcd7/hardware.md):
//   * two buffers swapped at the boundary: no tearing, and the swap costs 0.25 ms;
//   * every band is drawn in a tile of internal memory and DMA2D puts it into the frame. PSRAM is the bottleneck here, and
//     a band drawn right in the frame is written there two or three times (black background, then what lies on it), each
//     time by the processor. From a tile a pixel goes to PSRAM once, and the processor is already drawing the next band;
//   * only what changed — as on the S3. With two buffers a frame repairs what it changed AND what the frame before it
//     changed, since that one is not in this buffer yet;
//   * both cores take bands from one queue: no splitting of a band and no balancing to do.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"
#include "halo_spans.h"
#include "halo_p4_fbcpy.h"
#include <driver/gpio.h>
#include <esp_cache.h>
#include <esp_timer.h>
#include <freertos/queue.h>

#pragma GCC optimize("O3", "fast-math")

HALO_TLS HaloBand hb;
uint32_t hf_finger_t;

static HaloConfig cfg;
static HaloUpdate m_update;
static HaloDraw m_draw;

// ---------- What changed ----------
static HaloSpans dmg[HALO_BANDS];
static HaloSpans last[HALO_BANDS];  // what the previous frame changed: the back buffer does not have it yet
static portMUX_TYPE dmg_mux = portMUX_INITIALIZER_UNLOCKED;

void halo_damage(int x0, int y0, int x1, int y1) {
  x0 = max(x0, 0), y0 = max(y0, 0), x1 = min(x1, HALO_W - 1), y1 = min(y1, HALO_H - 1);
  if (x0 > x1 || y0 > y1) return;
  portENTER_CRITICAL(&dmg_mux);
  for (int b = y0 / HALO_ROWS; b <= y1 / HALO_ROWS; b++) halo_spans_add(dmg[b], x0, x1);
  portEXIT_CRITICAL(&dmg_mux);
}

void halo_damage_f(float x0, float y0, float x1, float y1) {
  halo_damage(halo_floor(x0) - 1, halo_floor(y0) - 1, halo_floor(x1) + 2, halo_floor(y1) + 2);
}

void halo_damage_all() {
  halo_damage(0, 0, HALO_W - 1, HALO_H - 1);
}

// A list that starts moving redraws most of the screen at once: the next frames start at once too, not late for a fresher
// finger — their cost is not in frame_peak yet, and they would miss the boundary (a fling and a drag each lost a frame)
static volatile uint8_t heavy_frames;
void halo_heavy() {
  heavy_frames = 3;
}

// ---------- Animation time ----------
static uint64_t m_clock;
static volatile bool vt_paused, vt_fixed;
static volatile int vt_steps;
static volatile float slowmo = 1;

uint32_t halo_us() {
  return (uint32_t)m_clock;
}
uint32_t halo_ms() {
  return (uint32_t)(m_clock / 1000);
}

// ---------- Drawing: both cores take jobs from one queue ----------
struct Job {
  int16_t y, rows, x0, x1;
};
static Job jobs[HALO_BANDS * HALO_SPANS];
static int job_n;
static volatile int job_next;
static uint16_t *job_fb;
static volatile HaloUnder m_under;
static bool job_under;  // this frame: the back buffer holds a whole new picture, the interface goes over it

void halo_under(HaloUnder fn) {
  m_under = fn;
}
static SemaphoreHandle_t help_go, help_done;

// A tile holds a band of the full width. Two are being drawn, the third waits for the copy: with fewer the drawing stops and
// waits for DMA. A fourth takes the internal memory the ESP32-C6's Wi-Fi driver needs (boards/p4lcd7/hardware.md) and adds nothing.
// (Two tiles' memory cut into four of half the width made a frame of the whole screen 0.5-1 ms slower, not faster: what
// such a frame waits for is PSRAM, not the copies — 10 Nov 2026, boards/p4lcd7/p4-video.md)
#define TILES 3
static int tiles_n = TILES;
void hf_tiles(int n) {  // before halo_begin
  tiles_n = constrain(n, 2, TILES);
}
#define TILE_BYTES (HALO_W * HALO_ROWS * 2)
struct Send {
  uint16_t *tile;
  int16_t y, rows, x0, w;
};
static QueueHandle_t free_tiles, to_send;
static esp_async_fbcpy_handle_t fbcpy;
static SemaphoreHandle_t copy_done, frame_sent;
static volatile int copies_left;

static bool IRAM_ATTR on_copied(esp_async_fbcpy_handle_t, esp_async_fbcpy_event_data_t *, void *) {
  BaseType_t woken = pdFALSE;
  xSemaphoreGiveFromISR(copy_done, &woken);
  return woken == pdTRUE;
}

// One copy at a time, in the order the bands were drawn
static void send_task(void *) {
  Send s;
  for (;;) {
    xQueueReceive(to_send, &s, portMAX_DELAY);
    esp_async_fbcpy_trans_desc_t t = {};
    t.src_buffer = s.tile, t.dst_buffer = job_fb;
    t.src_buffer_size_x = t.copy_size_x = s.w, t.src_buffer_size_y = t.copy_size_y = s.rows;
    t.dst_buffer_size_x = HALO_W, t.dst_buffer_size_y = HALO_H;
    t.dst_offset_x = s.x0, t.dst_offset_y = s.y;
    t.pixel_format_unique_id.color_type_id = COLOR_TYPE_ID(COLOR_SPACE_RGB, COLOR_PIXEL_RGB565);
    const uint32_t t0 = trace_now();
    if (esp_async_fbcpy(fbcpy, &t, on_copied, nullptr) == ESP_OK) xSemaphoreTake(copy_done, pdMS_TO_TICKS(100));
    trace_span(TR_DMA, t0, s.y / HALO_ROWS);
    xQueueSend(free_tiles, &s.tile, portMAX_DELAY);
    if (__atomic_sub_fetch(&copies_left, 1, __ATOMIC_ACQ_REL) == 0) xSemaphoreGive(frame_sent);
  }
}

static bool tiles_begin() {
  free_tiles = xQueueCreate(TILES, sizeof(uint16_t *));
  to_send = xQueueCreate(TILES, sizeof(Send));
  copy_done = xSemaphoreCreateBinary();
  frame_sent = xSemaphoreCreateBinary();
  for (int i = 0; i < tiles_n; i++) {
    uint16_t *t = (uint16_t *)heap_caps_aligned_alloc(64, TILE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    if (!t) return false;
    xQueueSend(free_tiles, &t, 0);
  }
  esp_async_fbcpy_config_t c = {};
  return esp_async_fbcpy_install(&c, &fbcpy) == ESP_OK;
}

static void clear_part() {
  const uint32_t c0 = hp_on ? esp_cpu_get_cycle_count() : 0;
  for (int r = 0; r < hb.rows; r++) memset(hb.buf + r * hb.w + (hb.x0 - hb.bx), 0, (hb.x1 - hb.x0 + 1) * 2);
  if (hp_on) hp_cyc[xPortGetCoreID()][HP_CLEAR] += esp_cpu_get_cycle_count() - c0;
}

// A video frame is under this band: its pixels into the tile in place of black. The decoder wrote them behind the cache,
// and the cache still holds what the processor last saw there (zeros from the start): those rows are invalidated first —
// on whole cache lines, or the call is refused (the line before and after belong to the frame buffer too: never dirty)
static void under_part() {
  const uintptr_t a = (uintptr_t)(job_fb + hb.y * HALO_W) & ~(uintptr_t)127, b = ((uintptr_t)(job_fb + (hb.y + hb.rows) * HALO_W) + 127) & ~(uintptr_t)127;
  esp_cache_msync((void *)a, b - a, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
  for (int r = 0; r < hb.rows; r++) memcpy(hb.buf + r * hb.w + (hb.x0 - hb.bx), job_fb + (hb.y + r) * HALO_W + hb.x0, (hb.x1 - hb.x0 + 1) * 2);
}

static void run_jobs() {
  for (;;) {
    const int i = __atomic_fetch_add(&job_next, 1, __ATOMIC_RELAXED);
    if (i >= job_n) return;
    const Job &j = jobs[i];
    const uint32_t t0 = trace_now();
    uint16_t *tile;
    xQueueReceive(free_tiles, &tile, portMAX_DELAY);
    const int w = j.x1 - j.x0 + 1;
    hb = {tile, j.y, j.rows, j.x0, j.x1, j.x0, w};
    if (job_under) under_part();
    else clear_part();
    m_draw();
    esp_cache_msync(tile, w * j.rows * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);  // (DMA reads memory)
    const Send s = {tile, j.y, j.rows, j.x0, (int16_t)w};
    xQueueSend(to_send, &s, portMAX_DELAY);
    trace_span(TR_BAND, t0, j.y / HALO_ROWS);
  }
}

static void helper_task(void *) {
  for (;;) {
    xSemaphoreTake(help_go, portMAX_DELAY);
    run_jobs();
    xSemaphoreGive(help_done);
  }
}

// ---------- Waiting for a moment ----------
static SemaphoreHandle_t wait_sem;
static esp_timer_handle_t wait_timer;

static void wait_wake(void *) {
  xSemaphoreGive(wait_sem);
}

static void wait_until(uint32_t t) {  // long waits sleep, the last few microseconds spin
  int32_t left = (int32_t)(t - trace_now());
  if (left > 2500) vTaskDelay(pdMS_TO_TICKS(left / 1000 - 1)), left = (int32_t)(t - trace_now());
  if (left > 150) {
    xSemaphoreTake(wait_sem, 0);
    if (esp_timer_start_once(wait_timer, left - 60) == ESP_OK) xSemaphoreTake(wait_sem, pdMS_TO_TICKS(5));
  }
  while ((int32_t)(t - trace_now()) > 0) {
  }
}

// ---------- A finger on the glass ----------
// Where the finger was this long before the frame starts: touch reports come every 10 ms, so at this distance there is
// always a report on both sides and the position is interpolated, never guessed ahead
#define FINGER_LAG_US 11000
// While a finger is down, a frame starts as late as it safely can and takes a fresher finger: a frame costs ~3 ms and has
// 16.5, and without this the finger would wait out the difference. Cost is the recent peak, it falls slowly.
static volatile bool late_start = true;
static float frame_peak = 9000;

// ---------- Frames ----------
struct Stats {
  uint32_t ticks, drawn, jobs, px, sum_us, max_us, missed, update_us, update_max, flip_us, flip_max, lead_us;
};
static Stats stats, report;
static uint32_t hp_report[2][HP_PARTS];
static volatile bool report_ready;
static volatile int bright_req = -1, power_req = -1;
static volatile bool disp_on = true, boot_cmd, boot_long_cmd;
static uint16_t *volatile shot_buf;
static volatile bool shot_req, shot_ready;

// ---------- film: what the screen showed, boundary by boundary ----------
// film N [STEP] — the next N boundaries, each a picture STEP times smaller (default 4) in PSRAM, with what made it: the
// boundary's number, a new frame or the old one again, a video picture under it, how many bands. film get — all of them as
// one tall picture (SHOT) and a line per frame (FILM: also how long the frame task took for it, from the update to the
// swap). For a moment too quick for the eye: a blink, a frame without a part, a frame that took two refreshes
static uint16_t *film_buf;
static uint32_t *film_meta;
static volatile int film_n, film_i, film_step = 4;

static void film_take(uint32_t te, bool drawn, int jobs, uint32_t us) {
  const int s = film_step, w = HALO_W / s, h = HALO_H / s;
  uint16_t *dst = film_buf + (size_t)film_i * w * h;
  const uint16_t *src = hd_front();
  for (int y = 0; y < h; y++) {
    const uint16_t *row = src + (size_t)(y * s + s / 2) * HALO_W;
    const uintptr_t a = (uintptr_t)row & ~(uintptr_t)127, b = ((uintptr_t)(row + HALO_W) + 127) & ~(uintptr_t)127;
    esp_cache_msync((void *)a, b - a, ESP_CACHE_MSYNC_FLAG_DIR_M2C);  // (DMA wrote it behind the cache)
    for (int x = 0; x < w; x++) dst[y * w + x] = row[x * s + s / 2];
  }
  uint32_t *m = film_meta + film_i * 3;
  m[0] = te, m[1] = (drawn ? 1u : 0u) | (job_under ? 2u : 0u) | (uint32_t)min(jobs, 65535) << 8, m[2] = us;
  film_i++;
}

static void film_get();
static void cmd_film(const char *a) {
  if (!strncmp(a, "get", 3)) return film_get();
  int n = 120, s = 4;
  sscanf(a, "%d %d", &n, &s);
  film_n = 0;
  delay(40);  // (a frame being taken now finishes first)
  s = constrain(s, 1, 8), n = constrain(n, 1, 600);
  heap_caps_free(film_buf), heap_caps_free(film_meta);
  film_buf = (uint16_t *)heap_caps_malloc((size_t)n * (HALO_W / s) * (HALO_H / s) * 2, MALLOC_CAP_SPIRAM);
  film_meta = (uint32_t *)heap_caps_malloc(n * 12, MALLOC_CAP_SPIRAM);
  if (!film_buf || !film_meta) return (void)Serial.println("FILM no memory");
  film_step = s, film_i = 0, film_n = n;
  Serial.printf("FILM START %d %d\n", n, s);
}

static void film_get() {
  const uint32_t t0 = millis();
  while (film_n && film_i < film_n && millis() - t0 < 20000) delay(5);
  if (!film_n || !film_buf) return (void)Serial.println("FILM none");
  const int n = film_i, w = HALO_W / film_step, h = HALO_H / film_step;
  for (int i = 0; i < n; i++) {
    const uint32_t *m = film_meta + i * 3;
    Serial.printf("FILM %d %lu %lu %lu\n", i, (unsigned long)m[0], (unsigned long)m[1], (unsigned long)m[2]);
  }
  hc_send_image(film_buf, w, h * n);
  film_n = 0;
  heap_caps_free(film_buf), heap_caps_free(film_meta), film_buf = nullptr, film_meta = nullptr;
}

static void frame_task(void *) {
  static HaloSpans fd[HALO_BANDS], all[HALO_BANDS];
  halo_damage_all();
  uint32_t report_at = millis() + cfg.report_ms;
  uint32_t target = hd_te_count() + 1, tick_te = 0, show_prev = trace_now();
  for (;;) {
    hd_wait_te(target, 50);
    uint32_t te, te_t;
    hd_te_snapshot(&te, &te_t);
    if (!stats.ticks && !report.ticks) hc_stage(6);
    stats.ticks += te != tick_te;
    tick_te = te;
    if (bright_req >= 0) hd_set_brightness(bright_req), bright_req = -1;
    if (power_req >= 0) hd_set_power(power_req), power_req = -1;
    hd_rate_apply();

    // This frame goes on screen at the next boundary: that is the moment its animation time stands for
    const uint32_t period = hd_te_period() ? hd_te_period() : 16563;
    uint32_t nominal = te_t;
    if (heavy_frames) {
      heavy_frames--;
    } else if (late_start && halo_finger.down) {
      // Not below 6 ms of cost: an idle screen costs 2 ms, and the first frame of a drag — 7 (it was late exactly there)
      const uint32_t budget = (uint32_t)(fmaxf(frame_peak, 6000) * 1.25f) + 2000;
      if (budget + 500 < period) nominal = te_t + period - budget, wait_until(nominal);
    }
    const uint32_t p0 = trace_now(), show_t = te_t + period;
    trace_mark(TR_CLOCK, (uint16_t)constrain((int32_t)(show_t - p0), 0, 65535));
    float dt = constrain((int32_t)(show_t - show_prev) / 1e6f, 0.001f, 0.1f);
    show_prev = show_t;
    const bool frame_new = !vt_paused || vt_steps > 0;
    if (vt_paused && frame_new) vt_steps--, trace_mark(TR_STEP, vt_steps);
    if (vt_paused || vt_fixed) dt = 1.0f / 60;
    hf_finger_t = nominal - FINGER_LAG_US;
    if (frame_new) {
      m_clock += (uint32_t)(dt * 1e6f + 0.5f);
      halo_subpixel = false;
      m_update(vt_paused || vt_fixed ? dt : dt * slowmo);
      const uint32_t u = trace_now() - p0;
      trace_span(TR_PREPARE, p0, 0, p0 + u);
      stats.update_us += u, stats.update_max = max(stats.update_max, u);
    }

    // A whole new picture under the interface (halo_under) comes first: what lies over it is drawn again only where it is
    // marked, so whoever made the picture marks all of its own over it — now, before the changes are taken (a part left
    // unmarked would be missing from this frame: a blink)
    const uint32_t t0 = trace_now();
    job_under = false;
    if (const HaloUnder under = m_under) {
      bool changed = false;
      portENTER_CRITICAL(&dmg_mux);
      for (const auto &d : dmg) changed |= d.n != 0;
      portEXIT_CRITICAL(&dmg_mux);
      job_under = under(hd_back(), changed);
      if (job_under) for (auto &d : last) d.n = 0;  // (a whole new picture: nothing of the frame before it is left in this buffer)
    }
    // What to draw: this frame's changes and the previous frame's
    portENTER_CRITICAL(&dmg_mux);
    memcpy(fd, dmg, sizeof(fd));
    for (auto &d : dmg) d.n = 0;
    portEXIT_CRITICAL(&dmg_mux);
    job_n = 0;
    int px = 0;
    for (int b = 0; b < HALO_BANDS; b++) {
      all[b] = fd[b];
      for (int i = 0; i < last[b].n; i++) halo_spans_add(all[b], last[b].x0[i], last[b].x1[i]);
      halo_spans_sort(all[b]);
      const int y = b * HALO_ROWS, rows = min(HALO_ROWS, HALO_H - y);
      for (int i = 0; i < all[b].n; i++) {
        const int x0 = all[b].x0[i] & ~1, x1 = min(all[b].x1[i] | 1, HALO_W - 1);
        jobs[job_n++] = {(int16_t)y, (int16_t)rows, (int16_t)x0, (int16_t)x1};
        px += (x1 - x0 + 1) * rows;
      }
    }
    // What the next frame repairs in the other buffer. After a picture frame — nothing: the next frame with anything to draw
    // brings a whole picture again (an ordinary frame repairing it would paint those parts over black)
    if (job_under) for (auto &d : last) d.n = 0;
    else memcpy(last, fd, sizeof(last));

    if (job_n || job_under) {
      job_fb = hd_back();
      if (job_n) {
        job_next = 0;
        copies_left = job_n;
        // A couple of bands are not worth waking the other core for
        const bool help = cfg.two_cores && job_n >= 4;
        if (help) xSemaphoreGive(help_go);
        run_jobs();
        if (help) {
          const uint32_t w0 = trace_now();
          xSemaphoreTake(help_done, portMAX_DELAY);
          trace_span(TR_WAIT_HALF, w0);
        }
        xSemaphoreTake(frame_sent, pdMS_TO_TICKS(100));  // every band is in the frame
      }
      const uint32_t f0 = trace_now();
      hd_flip();
      const uint32_t f1 = trace_now();
      trace_span(TR_FINISH, f0, 0, f1);
      // The swap happens at the next boundary, and only then is the other buffer free. The counter is read after the swap
      // was asked for: a boundary during that call cannot be taken for the one that makes the swap
      const uint32_t after = hd_te_count();
      target = after + 1;
      const uint32_t missed = after != te ? after - te : 0;
      const uint32_t frame_us = f1 - t0;
      // Falls only with no finger down: a start that drifts during a drag moves the finger's moment, and the steps go uneven
      frame_peak = fmaxf(halo_finger.down ? frame_peak : frame_peak * 0.97f, (float)(f1 - p0));
      stats.lead_us += show_t - p0;
      trace_span(TR_FRAME, t0, min(missed, (uint32_t)255), f1);
      stats.drawn++, stats.jobs += job_n, stats.px += px, stats.sum_us += frame_us, stats.missed += missed;
      stats.max_us = max(stats.max_us, frame_us);
      stats.flip_us += f1 - f0, stats.flip_max = max(stats.flip_max, f1 - f0);
    } else {
      target = te + 1;
    }

    if (shot_req) {
      esp_cache_msync(hd_front(), HALO_W * HALO_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);  // (DMA wrote it behind the cache)
      memcpy(shot_buf, hd_front(), HALO_W * HALO_H * 2);
      shot_req = false, shot_ready = true;
    }
    if (film_n && film_i < film_n) film_take(te, job_n || job_under, job_n, trace_now() - p0);
    if (cfg.report_ms && (int32_t)(millis() - report_at) >= 0) {
      report_at = (int32_t)(millis() - report_at) >= cfg.report_ms ? millis() + cfg.report_ms : report_at + cfg.report_ms;
      if (!report_ready) {
        report = stats, report_ready = true;
        memcpy(hp_report, hp_cyc, sizeof(hp_report));
      }
      stats = {};
      memset(hp_cyc, 0, sizeof(hp_cyc));
    }
    hp_frame_end();
    halo_trace_pump();
  }
}

// ---------- Commands ----------
static void cmd_shot(const char *) {
  if (!shot_buf) shot_buf = (uint16_t *)heap_caps_malloc(HALO_W * HALO_H * 2, MALLOC_CAP_SPIRAM);
  if (!shot_buf) return;
  shot_ready = false, shot_req = true;
  const uint32_t t0 = millis();
  while (!shot_ready && millis() - t0 < 1000) delay(2);
  if (shot_ready) hc_send_image(shot_buf, HALO_W, HALO_H);
}

// bench fill — one plate over a band in a tile, one core: nanoseconds a pixel for the loops every full-screen effect is made of
static void bench_fill() {
  uint16_t *t = (uint16_t *)heap_caps_aligned_alloc(64, TILE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  if (!t) return;
  const HaloBand saved = hb;
  const int px = HALO_W * HALO_ROWS;
  struct {
    const char *name;
    uint16_t bg;
    uint32_t color;
    float alpha;
    float r;
  } v[] = {{"black veil on black", 0, 0, 0.5f, 1},      {"black veil on color", 0x2222, 0, 0.5f, 1},
           {"color veil on color", 0x2222, 0x305070, 0.5f, 1}, {"opaque plate", 0, 0x305070, 1, 1},
           {"black veil on black, r0", 0, 0, 0.5f, 0}};
  for (auto &c : v) {
    uint32_t best = UINT32_MAX;
    for (int k = 0; k < 8; k++) {
      for (int i = 0; i < px; i++) t[i] = c.bg;
      hb = {t, 0, HALO_ROWS, 0, HALO_W - 1, 0, HALO_W};
      const uint32_t c0 = esp_cpu_get_cycle_count();
      halo_rrect(0, 0, HALO_W, HALO_H, c.r, c.color, c.alpha);
      best = min(best, esp_cpu_get_cycle_count() - c0);
    }
    LOG("bench fill %-26s %5.1f ns/px, %.1f cycles/px\n", c.name, best * 1000.0f / HALO_CPU_MHZ / px, (float)best / px);
  }
  // The SIMD loop against the plain formula: random sizes, offsets, colors, levels
  uint32_t seed = 0x9E3779B9u, bad = 0;
  auto rnd = [&]() { return seed ^= seed << 13, seed ^= seed >> 17, seed ^= seed << 5, seed; };
  static uint16_t ref[256];
  for (int k = 0; k < 2000; k++) {
    const int n = rnd() % 200 + 1, off = rnd() % 32;
    const uint16_t c = (uint16_t)rnd();
    const int A = rnd() % 257;
    for (int i = 0; i < n; i++) ref[i] = t[off + i] = (uint16_t)rnd();
    halo_fast_veil(t + off, n, c, A);
    for (int i = 0; i < n; i++) {  // (the plain formula, a channel at a time, exactly rounded down)
      const int r = ((ref[i] >> 11) * (256 - A) >> 8) + ((c >> 11) * A >> 8), g = ((ref[i] >> 5 & 63) * (256 - A) >> 8) + ((c >> 5 & 63) * A >> 8);
      const int b = ((ref[i] & 31) * (256 - A) >> 8) + ((c & 31) * A >> 8);
      if (t[off + i] != (uint16_t)(r << 11 | g << 5 | b)) bad++;
    }
  }
  LOG("bench fill: SIMD against the formula: %lu pixels differ\n", (unsigned long)bad);
  hb = saved;
  heap_caps_free(t);
}

static void cmd_bench(const char *a) {  // the whole frame on one core, into a spare buffer: what drawing alone costs
  if (!strcmp(a, "fill")) return bench_fill();
  static uint16_t *tmp = (uint16_t *)heap_caps_malloc(HALO_W * HALO_H * 2, MALLOC_CAP_SPIRAM);
  if (!tmp) return;
  vt_paused = true;
  delay(40);
  const uint32_t t0 = trace_now();
  for (int k = 0; k < 3; k++)
    for (int y = 0; y < HALO_H; y += HALO_ROWS) {
      hb = {tmp + y * HALO_W, y, min(HALO_ROWS, HALO_H - y), 0, HALO_W - 1, 0, HALO_W};
      clear_part();
      m_draw();
    }
  LOG("bench: full frame on one core %.2f ms\n", (trace_now() - t0) / 3000.0f);
  vt_paused = false;
}

void halo_brightness(uint8_t level) {
  bright_req = level;
}

void halo_display_on(bool on) {
  disp_on = on, power_req = on;
}

bool halo_display_is_on() {
  return disp_on;
}

// The BOOT button: pressed two frames in a row after two released (contact bounce is shorter than a frame)
static bool boot_down, boot_long_done = true, boot_swallow;
static uint32_t boot_down_ms;

void hf_swallow_boot() {
  boot_swallow = true;
}

bool halo_boot_click() {
  static uint8_t hist = 0xFF;
  hist = hist << 1 | (gpio_get_level((gpio_num_t)HALO_PIN_BOOT) ? 1 : 0);
  if (boot_cmd) return boot_cmd = false, true;
  if (boot_swallow) boot_swallow = false, boot_down = true, boot_down_ms = millis(), boot_long_done = false;
  if (!boot_down && (hist & 3) == 0) return boot_down_ms = millis(), boot_long_done = false, boot_down = true;
  if (boot_down && (hist & 3) == 3) boot_down = false, boot_long_done = true;
  return false;
}

bool halo_boot_long() {
  if (boot_long_cmd) return boot_long_cmd = false, true;
  if (boot_long_done || !boot_down || millis() - boot_down_ms < HALO_BOOT_LONG_MS) return false;
  return boot_long_done = true;
}

bool halo_begin(HaloUpdate update, HaloDraw draw, const HaloConfig &c) {
  cfg = c;
  m_update = update;
  m_draw = draw;
  hc_begin(cfg.name);
  if (hc_safe_mode()) return false;
  if (!hd_begin(cfg.brightness)) return hc_boot_done(), false;
  hc_stage(4);
  if (cfg.touch) ht_begin();
  pinMode(HALO_PIN_BOOT, INPUT_PULLUP);
  hc_stage(5);
  hc_boot_done();
  hu_begin();
  if (!tiles_begin()) {
    LOG("halo: no memory for tiles\n");
    return false;
  }
  xTaskCreatePinnedToCore(send_task, "halo_send", 3072, nullptr, 11, nullptr, 0);
  help_go = xSemaphoreCreateBinary();
  help_done = xSemaphoreCreateBinary();
  wait_sem = xSemaphoreCreateBinary();
  const esp_timer_create_args_t wa = {wait_wake, nullptr, ESP_TIMER_TASK, "halo_wait", false};
  esp_timer_create(&wa, &wait_timer);
  halo_trace_name(TR_FRAME, "frame", 'X');
  halo_trace_name(TR_PREPARE, "prepare", 'X');
  halo_trace_name(TR_BAND, "band", 'X');
  halo_trace_name(TR_WAIT_HALF, "wait other core", 'X');
  halo_trace_name(TR_FINISH, "swap", 'X');
  halo_trace_name(TR_STEP, "step", 'i');
  halo_trace_name(TR_CLOCK, "clock", 'i');
  halo_command("shot", cmd_shot);
  halo_command("bench", cmd_bench);
  halo_command("film", cmd_film);
  halo_command("pause", [](const char *) { vt_steps = 0, vt_paused = true; });
  halo_command("resume", [](const char *) { vt_steps = 0, vt_paused = false; });
  halo_command("step", [](const char *a) { vt_steps += max(1, atoi(a)), vt_paused = true; });
  halo_command("clock", [](const char *a) { vt_fixed = !strcmp(a, "fixed"); });
  halo_command("slow", [](const char *a) { slowmo = 1 / constrain(atof(a), 1.0f, 100.0f); });
  halo_command("bright", [](const char *a) { halo_brightness(atoi(a)); });
  halo_command("boot", [](const char *a) { (strstr(a, "long") ? boot_long_cmd : boot_cmd) = true; });
  halo_command("redraw", [](const char *) { halo_damage_all(); });
  halo_command("prof", [](const char *) { hp_on = !hp_on; });
  halo_command("late", [](const char *a) { late_start = strcmp(a, "off") != 0; });
  if (cfg.two_cores) xTaskCreatePinnedToCore(helper_task, "halo_helper", 4096, nullptr, 10, nullptr, 0);
  xTaskCreatePinnedToCore(frame_task, "halo_frames", 8192, nullptr, 5, nullptr, 1);
  LOG("halo: started, free internal %u KB, psram %u KB\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024,
      heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024);
  return true;
}

void halo_loop() {
  hc_loop();
  if (report_ready) {
    const Stats &s = report;
    const float sec = cfg.report_ms / 1000.0f;
    if (s.drawn)
      LOG("%s: %.0f fps drawn of %.0f, frame %.2f ms (max %.2f), %.1f jobs %.0f%% of screen, update %.2f ms (max %.2f), "
          "swap %.2f ms (max %.2f), start to show %.1f ms, missed %lu\n", cfg.name, s.drawn / sec, s.ticks / sec, s.sum_us / 1000.0f / s.drawn,
          s.max_us / 1000.0f, (float)s.jobs / s.drawn, 100.0f * s.px / s.drawn / (HALO_W * HALO_H),
          s.update_us / 1000.0f / max((uint32_t)1, s.ticks), s.update_max / 1000.0f, s.flip_us / 1000.0f / s.drawn,
          s.flip_max / 1000.0f, s.lead_us / 1000.0f / s.drawn, (unsigned long)s.missed);
    else
      LOG("%s: idle (%.0f ticks/s), update %.2f ms\n", cfg.name, s.ticks / sec, s.update_us / 1000.0f / max((uint32_t)1, s.ticks));
    if (hp_on && s.drawn) {
      static const char *const N[HP_PARTS] = {"text", "mask", "image", "rrect", "circle", "ring", "arc", "edge", "pixels", "clear", "-"};
      char line[280];
      for (int half = 0; half < 2; half++) {
        int n = snprintf(line, sizeof(line), "%s prof ms/frame (core1/core0):", cfg.name);
        for (int i = half ? 6 : 0; i < (half ? HP_PARTS - 1 : 6); i++)
          n += snprintf(line + n, sizeof(line) - n, " %s %.2f/%.2f", N[i], hp_report[1][i] / (HALO_CPU_MHZ * 1000.0f) / s.drawn,
                        hp_report[0][i] / (HALO_CPU_MHZ * 1000.0f) / s.drawn);
        LOG("%s\n", line);
      }
    }
    report_ready = false;
  }
  delay(5);
}

#endif  // CONFIG_IDF_TARGET_ESP32P4
