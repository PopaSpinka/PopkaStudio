// recorder.h — the Recorder (FL_RECORDER): PopkaStudio's own screen, picked from ten mockups (web/mockups/recorder.html,
// variant 9, "a card and forget"): the title and the timer, a dotted voice wave in a dark card, three buttons as the
// Music player's — record / stop, the big play / pause, the volume (the player's volume screen, volume.h; the trash
// was dropped: Record again replaces the recording). One recording, no list. Both microphones (halo_mic_*: Halo
// averages MIC1 and MIC2) record at the speaker's rate (one I2S bus, sound.h) into PSRAM, 60 s at most.
// The sound: a high-pass at ~80 Hz (hum, DC, knocks), the first 80 ms skipped and 10 ms faded in (the click of the
// finger leaving Record), the end cut back to the moment the finger touched Stop (its click) and faded out. Playback
// through the shared fill: louder so the voice (the loudest tenth of the slices above the noise) is at −20 dBFS — up
// to +24 dB, the peaks (but the loudest 0.5 % of the slices — clicks) not past −1 dBFS, anything louder bent by a soft
// limiter above −6 dBFS — and the pauses (slices at the room's noise) 10 dB quieter, eased.
//
// The wave: a slice of 75 ms → its loudness (RMS) as bright dots and its peak as dim ones above them, mirrored round the
// middle row (13 rows, 37 columns, 9 px apart). While recording the slices scroll left at 120 px/s — 2 px a frame, the
// newest one growing at the right edge; recorded — the whole recording in the 37 columns (they settle from the scroll
// and grow into it), while playing the part not played yet is grey. Drawn by own code straight into the band with
// ready colours on the card — no blending; its place is redrawn only when something in it changed (rc_damage).
// The microphones are on while the screen is open (switched in a task: ~8 ms of I2C, not in a frame); the screen gone
// (no update for 1 s) — the recording stops, the microphones go off. Recording or playing keeps Always On away
// (settings_busy → power.h). The editor has no microphones: a made-up voice, no sound.
// Included from settings.h after player.h (sound.h first).

#define RC_RED 0xFF5A4F
#define RC_RED_C 0x4A1714  // the record button at rest
#define RC_LIL 0xE8DDFF    // play: the player's lilac
#define RC_LIL_ON 0x2A1A4E
#define RC_GREY 0x2E3034
#define RC_OFF 0x6B6B70    // disabled; the wave not played yet
#define RC_SURF 0x1B1C1F   // the card
#define RC_TEXT 0xFFF8EE
#define RC_DIM 0x9E9E9E
#define RC_N 37     // columns
#define RC_P 9      // their pitch (and the rows')
#define RC_ROWS 7   // rows from the middle, it included
#define RC_X0 (CX - (RC_N - 1) * RC_P / 2)
#define RC_XR (RC_X0 + (RC_N - 1) * RC_P)
#define RC_Y 222.0f
#define RC_CARD_W 374.0f
#define RC_CARD_H 140.0f
#define RC_SLICE 0.075f  // s a column: 9 px / 75 ms = 120 px/s, 2 px a frame
#define RC_MAX_S 60
#define RC_MAX_SLICES ((int)(RC_MAX_S / RC_SLICE) + 4)
#define RC_RATE 22050      // the bus (sound.h): the music's rate
#define RC_DB_HI -14.0f    // the wave's top: this loud — all 7 rows (or 24 dB over the room's noise, if that is louder)
#define RC_LAG 0.1f        // the wave runs this late: the microphones give samples in blocks — the slices are there
#define RC_TITLE_Y 62.0f
#define RC_TIME_Y 112.0f
#define RC_BTN_Y 374.0f
static const float RC_BX[3] = {115, 233, 351}, RC_BR[3] = {42, 54, 42};
static const char RC_PLAY_S[] = "\xEF\x81\x8B", RC_PAUSE_S[] = "\xEF\x81\x8C", RC_VOL_S[] = "\xEF\x80\xA8";
#define RC_HP 0.9777f    // the high-pass: 1 / (1 + 2π · 80 Hz / 22 050)
#define RC_FADE 220      // samples: 10 ms in, 10 ms out
#define RC_SKIP 0.08f    // s skipped at the start (the finger leaving Record)
#define RC_VOICE_DB -20.0f  // the playback: the voice this loud
#define RC_GATE 0.32f    // … and the pauses this much (−10 dB)
#define RC_KNEE 16384.0f  // the soft limiter: above −6 dBFS the sound bends smoothly toward full scale

enum { RC_IDLE, RC_REC, RC_DONE, RC_PLAY };
static int rc_state;
static int16_t *rc_buf;       // the recording (PSRAM)
static uint8_t (*rc_lv)[4];   // per slice: loudness, peak (0..255 on the wave's scale), RMS and peak, dBFS + 90 (PSRAM)
static int rc_rate = RC_RATE, rc_cap, rc_spp = (int)(RC_RATE * RC_SLICE);  // samples a second, room, a slice
static int rc_n, rc_k;        // samples recorded, slices done
static int64_t rc_sq;         // the slice in progress: the sum of squares, the peak, samples
static int rc_pk, rc_cnt, rc_skip;
static float rc_hx, rc_hy;  // the high-pass: the last input and output
static float rc_t;            // s recorded (smooth: the frames' time, held near the samples)
static float rc_floor = -60;  // the room's noise, dB: follows quiet slices down at once, louder ones up slowly (~5 s)
static uint8_t rc_cur[2];     // the slice in progress, as rc_lv
static volatile int rc_len, rc_pos;  // the recording, samples; played
static volatile bool rc_playing;
static float rc_gainf = 1, rc_gate = 1;  // the playback: its gain; the pauses' (eased)
static volatile bool rc_mic_want, rc_mic_on, rc_mic_busy, rc_mic_ok;
static uint32_t rc_seen;
static bool rc_active;
static float rc_ob[RC_N], rc_op[RC_N];          // the whole recording in the columns (rows; 0 — empty)
static float rc_db[RC_N + 1], rc_dp[RC_N + 1];  // shown; [0] — the column leaving on the left
static float rc_xoff;                           // the columns' shift right, px (scrolling; after a stop — to 0)
static float rc_clock;                          // the dot's pulse
static float rc_fa = 1;                         // how much of the screen is seen (the volume screen fades it through)
static Press rc_press[3];
static int rc_armed = -1;
static HaloSpring rc_recm, rc_hasm, rc_ppm;  // recording; a recording to play; "pause" shown
static HaloSpringK K_RC;
static char rc_time_s[8];
static float rc_mid_title, rc_mid_time, rc_mid_ic, rc_time_w;
static HaloView rc_v;
static uint8_t RC_GA[RC_ROWS][RC_ROWS];  // a ghost dot's brightness (0..16): [span − 1][k]
static const uint8_t RC_BA[RC_ROWS] = {16, 15, 15, 14, 13, 13, 12};  // a bright dot's, by the row from the middle
static uint16_t rc_pal[2][17], rc_palc[2][17];  // white and grey on the card by brightness: the dot, its corners

static uint32_t rc_mix(uint32_t a, uint32_t b, float t) {
  t = halo_clamp01(t);
  const int k = (int)(t * 256);
  auto ch = [&](int s) { return (uint32_t)(((a >> s & 255) * (256 - k) + (b >> s & 255) * k) >> 8) << s; };
  return ch(16) | ch(8) | ch(0);
}
static bool rc_has() { return rc_len > 0 && rc_state != RC_REC; }
static float rc_dbfs(float amp) { return amp < 1 ? -90 : 20 * log10f(amp * (1 / 32768.0f)); }

// ---------- Sound ----------
#ifndef __EMSCRIPTEN__
static int rc_fill(int16_t *out, int n) {  // the audio task (sound.h): the recording, the voice louder, the pauses not
  if (!rc_playing) return 0;
  const int pos = rc_pos, k = min(n, (int)rc_len - pos);
  if (k <= 0) return rc_playing = false, 0;
  const int16_t *src = rc_buf + pos;
  for (int i = 0; i < k; i++) {
    const int sl = (pos + i) / rc_spp;
    const float tg = sl < rc_k && !rc_lv[sl][0] ? RC_GATE : 1;  // a slice at the room's noise — quieter
    rc_gate += (tg - rc_gate) * (tg < rc_gate ? 0.001f : 0.01f);  // down in ~45 ms, up in ~5 ms
    float v = src[i] * rc_gainf * rc_gate;
    const float a = fabsf(v);
    if (a > RC_KNEE) {  // (rare: a click louder than the voice)
      const float e = a - RC_KNEE;
      v = copysignf(RC_KNEE + e / (1 + e * (1 / (32767 - RC_KNEE))), v);
    }
    out[i] = (int16_t)v;
  }
  rc_pos = pos + k;
  return k;
}
static int16_t rc_junk[128];
static int64_t rc_idle_sq;
static int rc_idle_n;
static void rc_drain() {  // not recording: keep the microphones' ring empty — and learn the room's noise meanwhile
  int got;
  while ((got = halo_mic_read(rc_junk, 128)) > 0) {
    for (int i = 0; i < got; i++) rc_idle_sq += rc_junk[i] * rc_junk[i];
    if ((rc_idle_n += got) >= rc_spp) {
      const float db = rc_dbfs(sqrtf((float)rc_idle_sq / rc_idle_n));
      rc_floor += (db - rc_floor) * (db < rc_floor ? 0.5f : 0.015f);
      rc_idle_sq = 0, rc_idle_n = 0;
    }
    if (got < 128) break;
  }
}
#endif

// The microphones on or off as rc_mic_want says — in a task (I2C), one at a time
static void rc_mic_sync() {
#ifndef __EMSCRIPTEN__
  if (rc_mic_busy || rc_mic_want == rc_mic_on) return;
  rc_mic_busy = true;
  xTaskCreatePinnedToCore(
      [](void *) {
        while (rc_mic_want != rc_mic_on) {
          if (rc_mic_want) {
            const int r = halo_mic_begin(RC_RATE);
            if (r > 0) rc_rate = r, snd_begin(r);  // (the speaker on the same bus: the playback)
            rc_mic_ok = r > 0, rc_mic_on = true;
          } else halo_mic_end(), rc_mic_on = false;
        }
        rc_mic_busy = false;
        vTaskDelete(nullptr);
      },
      "rc_mic", 3072, nullptr, 2, nullptr, 0);
#endif
}

// ---------- Slices ----------
static uint8_t rc_level(float amp) {  // an amplitude (0..32768) → 0..255: the room's noise — 0, then up to the top
  const float lo = rc_floor + 6, hi = fmaxf(lo + 24, RC_DB_HI);
  return (uint8_t)(halo_clamp01((rc_dbfs(amp) - lo) / (hi - lo)) * 255);
}
#ifdef __EMSCRIPTEN__
static float rc_voice(float t) {  // the editor: a made-up voice — syllables, words, pauses (0..1)
  const float syl = 0.35f + 0.65f * fabsf(sinf(t * 2 * HALO_PI * 1.9f)) * (0.7f + 0.3f * sinf(t * 2 * HALO_PI * 5.3f));
  const float word = 0.6f + 0.4f * sinf(t * 2 * HALO_PI * 0.41f + 1.3f);
  const float gate = halo_clamp01((sinf(t * 2 * HALO_PI * 0.23f) + 0.7f) * 3);
  return halo_clamp01(0.05f + syl * word * gate * 0.95f + 0.04f * sinf(t * 71));
}
static void rc_voice_slice(float t0, float t1, uint8_t *out) {
  float a = 0, m = 0;
  for (int i = 0; i < 6; i++) {
    const float e = rc_voice(t0 + (t1 - t0) * (i + 0.5f) / 6);
    a += e, m = fmaxf(m, e);
  }
  out[0] = (uint8_t)(halo_clamp01(a / 6 * 1.1f) * 255), out[1] = (uint8_t)(halo_clamp01(m * 1.15f + 0.05f) * 255);
  if (out != rc_cur) out[2] = (uint8_t)(30 + a / 6 * 50), out[3] = (uint8_t)(40 + m * 50);
}
#endif
static void rc_slice_end() {
  if (rc_k < RC_MAX_SLICES) {
#ifdef __EMSCRIPTEN__
    rc_voice_slice(rc_k * RC_SLICE, fminf(rc_t, (rc_k + 1) * RC_SLICE), rc_lv[rc_k]);
#else
    if (!rc_cnt) return;
    const float rms = sqrtf((float)rc_sq / rc_cnt);
    const float db = rc_dbfs(rms);
    rc_lv[rc_k][0] = rc_level(rms), rc_lv[rc_k][1] = rc_level(rc_pk);
    rc_lv[rc_k][2] = (uint8_t)lroundf(db + 90), rc_lv[rc_k][3] = (uint8_t)lroundf(rc_dbfs(rc_pk) + 90);
    rc_floor += (db - rc_floor) * (db < rc_floor ? 0.5f : 0.015f);
#endif
    rc_k++;
  }
  rc_sq = 0, rc_pk = rc_cnt = 0;
}
#ifndef __EMSCRIPTEN__
static void rc_take(int16_t *s, int n, int at) {  // new samples (at — the first one's place): cleaned, into the slice
  for (int i = 0; i < n; i++) {
    const float x = s[i];
    rc_hy = RC_HP * (rc_hy + x - rc_hx), rc_hx = x;
    const float y = at + i < RC_FADE ? rc_hy * (at + i) * (1.0f / RC_FADE) : rc_hy;
    const int v = y > 32767 ? 32767 : y < -32768 ? -32768 : (int)y, a = v < 0 ? -v : v;
    s[i] = v;
    rc_sq += v * v;
    if (a > rc_pk) rc_pk = a;
    if (++rc_cnt >= rc_spp) rc_slice_end();
  }
}
#endif
static void rc_overview() {  // the whole recording in the columns: their loudness — the mean, their peak — the highest
  for (int s = 0; s < RC_N; s++) {
    if (!rc_k) {
      rc_ob[s] = rc_op[s] = 0;
      continue;
    }
    int a = min(s * rc_k / RC_N, rc_k - 1), b = min(max((s + 1) * rc_k / RC_N, a + 1), rc_k);
    int sum = 0, mx = 0;
    for (int i = a; i < b; i++) sum += rc_lv[i][0], mx = max(mx, (int)rc_lv[i][1]);
    rc_ob[s] = 1 + sum / (float)(b - a) * (RC_ROWS - 1) / 255, rc_op[s] = 1 + mx * (RC_ROWS - 1) / 255.0f;
  }
}

// ---------- What the buttons do ----------
static void rc_music_pause() {
#if HAS_MUSIC
  if (pl_playing) pl_set_playing(false);
#endif
}
static void rc_record() {
#ifndef __EMSCRIPTEN__
  if (rc_mic_on && !rc_mic_ok) return (void)LOG("recorder: no microphones\n");
  if (!rc_buf) rc_buf = (int16_t *)heap_caps_malloc(RC_MAX_S * RC_RATE * 2, MALLOC_CAP_SPIRAM);
  if (!rc_buf || !rc_lv) return (void)LOG("recorder: no memory\n");
  if (rc_mic_on) rc_drain();  // (from now on)
  rc_skip = (int)(RC_SKIP * rc_rate);
#endif
  rc_playing = false, rc_music_pause();
  rc_cap = RC_MAX_S * min(rc_rate, RC_RATE), rc_spp = (int)(rc_rate * RC_SLICE);
  rc_n = rc_k = rc_cnt = rc_pk = 0, rc_sq = 0, rc_t = 0, rc_len = rc_pos = 0, rc_hx = rc_hy = 0;
  rc_cur[0] = rc_cur[1] = 0;
  rc_state = RC_REC;
}
// trim — s cut off the end (the click of the finger on Stop: from its touch on)
static void rc_stop(float trim = 0) {
  if (trim <= 0 && (rc_cnt || rc_t > rc_k * RC_SLICE + 0.02f)) rc_slice_end();  // the slice in progress — the last one
#ifdef __EMSCRIPTEN__
  rc_t = fmaxf(0, rc_t - trim), rc_n = (int)(rc_t * rc_rate);
#else
  rc_n = max(0, rc_n - (int)(trim * rc_rate));
  for (int i = 0; i < min(RC_FADE, rc_n); i++) rc_buf[rc_n - 1 - i] = rc_buf[rc_n - 1 - i] * i / RC_FADE;  // fade out
#endif
  rc_k = min(rc_k, rc_n / max(1, rc_spp));
  rc_len = rc_n, rc_pos = 0, rc_state = rc_len ? RC_DONE : RC_IDLE;
  rc_overview();
  // The playback's gain: the voice — the loudest tenth of the slices above the room's noise (all, if none is)
  int hist[91] = {0}, peaks[91] = {0}, voiced = 0, peak = 0, top = 0;
  for (int i = 0; i < rc_k; i++) {
    top = max(top, (int)rc_lv[i][3]), peaks[min(90, (int)rc_lv[i][3])]++;
    if (rc_lv[i][0]) hist[min(90, (int)rc_lv[i][2])]++, voiced++;
  }
  for (int d = 90, seen = 0, skip = max(1, rc_k / 200); d >= 0; d--)  // the peak, but the loudest 0.5 % (clicks)
    if ((seen += peaks[d]) > skip) {
      peak = d;
      break;
    }
  if (!voiced)
    for (int i = 0; i < rc_k; i++) hist[min(90, (int)rc_lv[i][2])]++, voiced++;
  int voice = 0;
  for (int d = 90, seen = 0; d >= 0 && voiced; d--)
    if ((seen += hist[d]) * 10 >= voiced) {
      voice = d;
      break;
    }
  const float g = fminf(fminf(24, fmaxf(0, RC_VOICE_DB - (voice - 90))), fmaxf(0, -1 - (peak - 90)));
  rc_gainf = powf(10, g / 20);
  LOG("recorder: %.1f s, %d slices, the voice %d dB, the peak %d dB (the loudest %d), the room %.0f dB, playback +%.0f dB\n",
      (float)rc_len / rc_rate, rc_k, voice - 90, peak - 90, top - 90, rc_floor, g);
}
static void rc_play() {
  if (!rc_has()) return;
  rc_music_pause();
  if (rc_pos >= rc_len) rc_pos = 0;
#ifndef __EMSCRIPTEN__
  snd_begin(rc_rate);
#endif
  rc_gate = 1, rc_playing = true, rc_state = RC_PLAY;
}
static void rc_action(int i, float held) {  // held — s since the finger touched the button
  if (i == 0) rc_state == RC_REC ? rc_stop(held + 0.04f) : rc_record();  // (Record again — a new one in its place)
  else if (i == 1) rc_state == RC_PLAY ? (void)(rc_playing = false, rc_state = RC_DONE) : rc_play();
  else vol_open();
}
static bool rc_busy() { return rc_state == RC_REC || rc_state == RC_PLAY; }

// ---------- The frame ----------
static void recorder_build() {
  K_RC = halo_spring(HALO_M3X_FAST);
  rc_lv = (uint8_t(*)[4])heap_caps_malloc(RC_MAX_SLICES * 4, MALLOC_CAP_SPIRAM);
  vol_build();  // the speaker's volume (volume.h)
  rc_mid_title = halo_text_mid(gl_t24, "Recorder"), rc_mid_time = halo_text_mid(gl_n44, "0");
  rc_mid_ic = halo_text_mid(gl_rc32, RC_PLAY_S);
  for (int span = 1; span <= RC_ROWS; span++)
    for (int k = 0; k < RC_ROWS; k++) RC_GA[span - 1][k] = (uint8_t)lroundf(5.4f * (1 - 0.55f * k / span));
  for (int c = 0; c < 2; c++)
    for (int a = 0; a <= 16; a++) {
      const uint32_t col = c ? RC_OFF : RC_TEXT;
      rc_pal[c][a] = halo_565(rc_mix(RC_SURF, col, a / 16.0f)), rc_palc[c][a] = halo_565(rc_mix(RC_SURF, col, a * 0.45f / 16));
    }
#ifndef __EMSCRIPTEN__
  snd_voice = rc_fill;
#endif
  strcpy(rc_time_s, "0:00");
}

static float rc_rows(int lv) { return 1 + lv * (RC_ROWS - 1) / 255.0f; }
static void rc_columns(float dt) {  // what the columns show this frame (rc_db, rc_dp, rc_xoff)
  if (rc_state == RC_REC) {  // the last 37 slices, scrolling: the one in progress comes in at the right edge
    const float kf = fmaxf(0, rc_t - RC_LAG) / RC_SLICE;
    const int K = (int)kf;
    rc_xoff = (1 - (kf - K)) * RC_P;
    for (int c = 0; c <= RC_N; c++) {
      const int i = K - (RC_N - c);
      float b = 0, p = 0;
      if (i >= 0 && i < rc_k) b = rc_rows(rc_lv[i][0]), p = rc_rows(rc_lv[i][1]);
      else if (i == rc_k && rc_t > 0) b = rc_rows(rc_cur[0]), p = rc_rows(rc_cur[1]);
      rc_db[c] = b, rc_dp[c] = p;
    }
    return;
  }
  const float k = fminf(1, dt * 14);  // ~70 ms: they settle and grow into the whole recording (or into the line)
  rc_xoff -= rc_xoff * k;
  if (rc_xoff < 0.05f) rc_xoff = 0;
  for (int s = 0; s < RC_N; s++) {
    const float tb = rc_len ? rc_ob[s] : 0, tp = rc_len ? rc_op[s] : 0;
    rc_db[s + 1] += (tb - rc_db[s + 1]) * k, rc_dp[s + 1] += (tp - rc_dp[s + 1]) * k;
  }
}

static void recorder_tick(float dt) {  // every frame, whatever is on screen
  const HaloFinger &f = halo_finger;
  if (rc_mic_want && halo_ms() - rc_seen > 1000) {  // the screen is gone: the recording stops, the microphones off
    if (rc_state == RC_REC) rc_stop();
    rc_mic_want = false;
  }
  rc_mic_sync();
#ifndef __EMSCRIPTEN__
  if (rc_mic_on && rc_mic_ok) {
    if (rc_state == RC_REC) {
      int got;
      while (rc_skip > 0 && (got = halo_mic_read(rc_junk, min(128, rc_skip))) > 0) rc_skip -= got;
      while (!rc_skip && rc_n < rc_cap && (got = halo_mic_read(rc_buf + rc_n, min(512, rc_cap - rc_n))) > 0)
        rc_take(rc_buf + rc_n, got, rc_n), rc_n += got;
      if (rc_cnt) rc_cur[0] = rc_level(sqrtf((float)rc_sq / rc_cnt)), rc_cur[1] = rc_level(rc_pk);
    } else rc_drain();
  }
  if (rc_state == RC_REC) {
    rc_t = fminf(rc_t + dt, (float)rc_n / rc_rate + 0.25f);
    if (rc_n >= rc_cap) rc_stop();
  }
  if (rc_state == RC_PLAY && !rc_playing) rc_state = RC_DONE, rc_pos = rc_pos >= rc_len ? 0 : (int)rc_pos;
#else
  if (rc_state == RC_REC) {  // the made-up voice, slice by slice
    rc_t = fminf(rc_t + dt, (float)RC_MAX_S);
    while ((rc_k + 1) * RC_SLICE <= rc_t && rc_k < RC_MAX_SLICES) rc_slice_end();
    rc_voice_slice(rc_k * RC_SLICE, fmaxf(rc_t, rc_k * RC_SLICE + 0.01f), rc_cur);
    rc_n = (int)(rc_t * rc_rate);
    if (rc_t >= RC_MAX_S) rc_stop();
  }
  if (rc_state == RC_PLAY && (rc_pos += (int)(dt * rc_rate)) >= rc_len) rc_playing = false, rc_state = RC_DONE, rc_pos = 0;
#endif
#if HAS_MUSIC
  if (pl_playing && rc_state == RC_PLAY) rc_playing = false, rc_state = RC_DONE;  // the music took the speaker
#endif
  rc_recm.target = rc_state == RC_REC, rc_hasm.target = rc_has(), rc_ppm.target = rc_state == RC_PLAY;
  halo_spring_run(rc_recm, K_RC, dt), halo_spring_run(rc_hasm, K_RC, dt), halo_spring_run(rc_ppm, K_RC, dt);
  for (int i = 0; i < 3; i++) press_update(rc_press[i], f.down && rc_active && rc_armed == i, dt);
  rc_active = false;  // recorder_update sets it again while the screen is up
  rc_columns(dt);
  rc_clock += dt;
  const float sec = rc_state == RC_REC ? rc_t : rc_state == RC_PLAY ? (float)rc_pos / rc_rate
                    : rc_state == RC_DONE ? (float)(rc_pos > 0 ? rc_pos : rc_len) / rc_rate : 0;
  const int s = (int)sec;
  snprintf(rc_time_s, sizeof rc_time_s, "%d:%02d", s / 60, s % 60);
}

static void recorder_enter() {
  rc_armed = -1, rc_mic_want = true, rc_seen = halo_ms();  // (the microphones warm before Record is tapped)
  vol_enter();
}
static void recorder_update(float dt) {
  HaloFinger &f = halo_finger;
  rc_seen = halo_ms(), rc_active = true, rc_mic_want = true;
  if (vol_update(dt)) return void(rc_armed = -1);  // the volume screen: "+", "−"; a tap elsewhere or 3 s idle — closed
  if (f.pressed && !f.owner) {
    rc_armed = -1;
    for (int i = 0; i < 3; i++) {
      const float dx = f.x0 - RC_BX[i], dy = f.y0 - RC_BTN_Y, r = RC_BR[i] + 10;
      if (dx * dx + dy * dy < r * r && (i != 1 || rc_has())) rc_armed = i;
    }
  }
  if (f.dragging) rc_armed = -1;
  if (f.tap && rc_armed >= 0) rc_action(rc_armed, (halo_ms() - f.t0) / 1000.0f);
  if (!f.down) rc_armed = -1;
}

// ---------- Layout and drawing ----------
struct RcCol {
  int16_t x;                    // the dot's left edge
  uint8_t nb, np, grey, fade;   // bright rows, up to the peak (0 — empty: the dim middle dot), not played, 0..16
};
static RcCol rc_cols[RC_N + 1];
static HaloItem rc_title, rc_time, rc_dot, rc_glow, rc_card, rc_btn[3], rc_rec_ic, rc_play_ic, rc_pause_ic, rc_vol_ic;
static uint32_t rc_dmg_h;
static float rc_dmg_r[4];
static char rc_time_was[8];

static void recorder_layout(const HaloView &view) {
  rc_fa = vol_layout(view);  // the volume screen over it (volume.h): this screen fades through
  HaloView v = view;
  v.a *= rc_fa;
  rc_v = v;
  gl_text_mid(rc_title, gl_t24, "Recorder", rc_mid_title, CX, RC_TITLE_Y, 1, RC_TEXT, 1, &halo_black);
  halo_view_commit(rc_title, v);
  // The timer; recording — a red dot before it (pulsing), the two centred together
  if (strcmp(rc_time_was, rc_time_s)) rc_time_w = halo_text_width(gl_n44, rc_time_s), strcpy(rc_time_was, rc_time_s);
  const float m = halo_clamp01(rc_recm.x), has = halo_clamp01(rc_hasm.x), tx = CX + 11 * m;
  const uint32_t tc = rc_mix(rc_mix(RC_DIM, RC_TEXT, fmaxf(has, rc_len > 0 ? 1.0f : m)), RC_RED, m);
  gl_text_mid(rc_time, gl_n44, rc_time_s, rc_mid_time, tx, RC_TIME_Y, 1, tc, 1, &halo_black);
  halo_view_commit(rc_time, v);
  const float pulse = rc_state == RC_REC ? 0.82f + 0.18f * cosf(rc_clock * 2 * HALO_PI * 0.8f) : 1;
  for (HaloItem *it : {&rc_glow, &rc_dot}) {
    const bool g = it == &rc_glow;
    it->kind = HI_CIRCLE, it->x = tx - rc_time_w / 2 - 16, it->y = RC_TIME_Y, it->r = g ? 11 : 6, it->scale = 1;
    it->color = RC_RED, it->alpha = m * pulse * (g ? 0.22f : 1), it->on_black = true, it->on = nullptr;
    halo_view_commit(*it, v);
  }
  // The card and the wave's columns (drawn in recorder_draw)
  rc_card.kind = HI_RRECT, rc_card.x = CX, rc_card.y = RC_Y, rc_card.w = RC_CARD_W, rc_card.h = RC_CARD_H, rc_card.r = 36;
  rc_card.scale = 1, rc_card.color = RC_SURF, rc_card.alpha = 1, rc_card.on_black = true, rc_card.on = nullptr;
  halo_view_commit(rc_card, v);
  const float played = rc_len ? (float)rc_pos / rc_len : 0;
  const bool prog = (rc_state == RC_PLAY || rc_state == RC_DONE) && rc_pos > 0;
  uint32_t h = 2166136261u;
  for (int c = 0; c <= RC_N; c++) {
    RcCol &q = rc_cols[c];
    const float x = RC_X0 + (c - 1) * RC_P + rc_xoff;
    const float fade = halo_clamp01((x - (RC_X0 - RC_P)) / RC_P) * halo_clamp01((RC_XR + RC_P - x) / RC_P);
    q.x = (int16_t)lroundf(x) - 2, q.fade = (uint8_t)lroundf(fade * 16);
    q.nb = rc_db[c] < 0.5f ? 0 : (uint8_t)min(RC_ROWS, (int)(rc_db[c] + 0.5f));
    q.np = q.nb ? (uint8_t)max((int)q.nb, min(RC_ROWS, (int)(rc_dp[c] + 0.5f))) : 0;
    q.grey = prog && (c - 0.5f) / RC_N > played;
    h = (h ^ (uint32_t)(q.x << 20 | q.nb << 16 | q.np << 8 | q.grey << 5 | q.fade)) * 16777619u;
  }
  for (float qv : {v.x, v.y, v.s, v.a}) h = (h ^ (uint32_t)(int32_t)(qv * 512)) * 16777619u;
  const float ry = (RC_ROWS - 1) * RC_P + 4;
  const float r[4] = {halo_view_x(v, RC_X0 - RC_P - 4), halo_view_y(v, RC_Y - ry), halo_view_x(v, RC_XR + RC_P + 4),
                      halo_view_y(v, RC_Y + ry)};
  if (h != rc_dmg_h || memcmp(r, rc_dmg_r, sizeof r)) {
    if (rc_dmg_h) halo_damage_f(rc_dmg_r[0], rc_dmg_r[1], rc_dmg_r[2], rc_dmg_r[3]);
    halo_damage_f(r[0], r[1], r[2], r[3]);
    rc_dmg_h = h, memcpy(rc_dmg_r, r, sizeof r);
  }
  // The buttons: record / stop, play / pause, the volume (pressed — 6 % smaller)
  const uint32_t bc[3] = {rc_mix(RC_RED_C, RC_RED, m), rc_mix(RC_GREY, RC_LIL, has), RC_GREY};
  float sc[3];
  for (int i = 0; i < 3; i++) {
    sc[i] = 1 - 0.06f * halo_clamp01(rc_press[i].s.x);
    HaloItem &b = rc_btn[i];
    b.kind = HI_CIRCLE, b.x = RC_BX[i], b.y = RC_BTN_Y, b.r = RC_BR[i] * sc[i], b.scale = 1, b.color = bc[i], b.alpha = 1;
    b.on_black = true, b.on = nullptr;
    halo_view_commit(b, v);
  }
  HaloItem &ri = rc_rec_ic;  // the red dot turns into the white square
  const float side = (28 + (26 - 28) * m) * sc[0];
  ri.kind = HI_RRECT, ri.x = RC_BX[0], ri.y = RC_BTN_Y, ri.w = ri.h = side, ri.r = (14 + (6 - 14) * m) * sc[0];
  ri.scale = 1, ri.color = rc_mix(RC_RED, RC_TEXT, m), ri.alpha = 1, ri.on_black = false, ri.on = nullptr;
  halo_view_commit(ri, v);
  const uint32_t ic = rc_mix(RC_OFF, RC_LIL_ON, has);
  const float pp = halo_clamp01(rc_ppm.x);
  gl_text_mid(rc_play_ic, gl_rc32, RC_PLAY_S, rc_mid_ic, RC_BX[1] + 3, RC_BTN_Y, sc[1], ic, 1 - pp, &rc_btn[1]);
  gl_text_mid(rc_pause_ic, gl_rc32, RC_PAUSE_S, rc_mid_ic, RC_BX[1], RC_BTN_Y, sc[1], ic, pp, &rc_btn[1]);
  gl_text_mid(rc_vol_ic, gl_rc32, RC_VOL_S, rc_mid_ic, RC_BX[2], RC_BTN_Y, sc[2], RC_TEXT, 1, &rc_btn[2]);
  for (HaloItem *it : {&rc_play_ic, &rc_pause_ic, &rc_vol_ic}) halo_view_commit(*it, v);
}

// One dot, 5 × 5 with soft corners, straight into this task's part of the band (the card is under it, solid)
static void rc_dot5(int x, int y, uint16_t c, uint16_t e) {
  const int y0 = max(y, hb.y), y1 = min(y + 4, hb.y + hb.rows - 1), xa = max(x, hb.x0), xb = min(x + 4, hb.x1);
  for (int yy = y0; yy <= y1; yy++) {
    uint16_t *row = halo_row(yy);
    const bool edge = yy == y || yy == y + 4;
    for (int xx = xa; xx <= xb; xx++) row[xx] = edge && (xx == x || xx == x + 4) ? e : c;
  }
}
static void rc_wave_draw() {
  const HaloView &v = rc_v;
  if (v.a <= 0.004f) return;  // (hidden: dimmed to black it would paint black over the screen under it)
  const bool fast = v.s == 1 && v.a >= 0.999f && v.x == (int)v.x && v.y == (int)v.y;
  const float ry = (RC_ROWS - 1) * RC_P + 3;
  if (!halo_hits(halo_view_x(v, RC_X0 - RC_P - 3), halo_view_y(v, RC_Y - ry), halo_view_x(v, RC_XR + RC_P + 3),
                 halo_view_y(v, RC_Y + ry)))
    return;
  for (int c = 0; c <= RC_N; c++) {
    const RcCol &q = rc_cols[c];
    if (!q.fade) continue;
    for (int j = 0; j < RC_ROWS; j++) {
      int a = !q.nb ? (j ? 0 : 4) : j < q.nb ? RC_BA[j] : j < q.np ? RC_GA[q.np - q.nb - 1][j - q.nb] : 0;
      if (!a) break;
      if (!(a = (a * q.fade + 8) >> 4)) continue;
      for (int sgn = j ? -1 : 1; sgn <= 1; sgn += 2) {
        const int y = (int)RC_Y + sgn * j * RC_P - 2;
        if (fast) {
          const int x = q.x + (int)v.x, yy = y + (int)v.y;
          if (yy + 4 < hb.y || yy >= hb.y + hb.rows || x + 4 < hb.x0 || x > hb.x1) continue;
          rc_dot5(x, yy, rc_pal[q.grey][a], rc_palc[q.grey][a]);
        } else {  // (a transition: scaled, dimmed)
          const float x0 = halo_view_x(v, q.x - 0.5f), y0 = halo_view_y(v, y - 0.5f);
          const float x1 = halo_view_x(v, q.x + 4.5f), y1 = halo_view_y(v, y + 4.5f);
          if (!halo_hits(x0, y0, x1, y1)) continue;
          const uint32_t col = rc_mix(RC_SURF, q.grey ? RC_OFF : RC_TEXT, a / 16.0f);
          halo_rrect(x0, y0, x1, y1, 1.5f * v.s, halo_dim(col, (int)(v.a * 256)), 1);
        }
      }
    }
  }
}

static void recorder_draw() {
  if (rc_fa > 0.004f) {
    halo_item_draw(rc_title), halo_item_draw(rc_glow), halo_item_draw(rc_dot), halo_item_draw(rc_time);
    halo_item_draw(rc_card);
    rc_wave_draw();
    for (const auto &b : rc_btn) halo_item_draw(b);
    halo_item_draw(rc_rec_ic), halo_item_draw(rc_play_ic), halo_item_draw(rc_pause_ic), halo_item_draw(rc_vol_ic);
  }
  vol_draw();
}
