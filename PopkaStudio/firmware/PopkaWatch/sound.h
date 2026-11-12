// sound.h — one speaker for the Music player and the Recorder. Halo has one fill per I2S bus (halo_audio_begin keeps
// the first), so both go through snd_fill: the recorder's playback first, else the music. The first one that needs the
// speaker starts it at the music's rate (22 050 Hz) in a task of its own (a few ms of I2C — not in a frame); the
// microphones share that bus and record at its rate. Included from settings.h before music.h and recorder.h.

static int (*snd_voice)(int16_t *, int);  // the recorder's playback (0 — nothing to play)
static int (*snd_music)(int16_t *, int);  // the music
static int (*snd_video)(int16_t *, int);  // the Video app's sound (video_hw.h): it goes first
static int snd_rate;                      // what the speaker runs at, once started
static volatile bool snd_started;

#ifndef __EMSCRIPTEN__
static int snd_fill(int16_t *buf, int n) {  // the audio task (core 0)
  if (snd_video) return snd_video(buf, n);
  const int k = snd_voice ? snd_voice(buf, n) : 0;
  if (k > 0) return k;
  return snd_music ? snd_music(buf, n) : 0;
}
#endif

// From the frame (or a task): the speaker on, once
static void snd_begin(int rate) {
#ifndef __EMSCRIPTEN__
  if (snd_started) return;
  snd_started = true;
  snd_rate = rate;
  static int r;
  r = rate;
  xTaskCreatePinnedToCore(
      [](void *) {
        halo_audio_begin(snd_fill, r);
        vTaskDelete(nullptr);
      },
      "audio_init", 3072, nullptr, 2, nullptr, 0);
#else
  (void)rate;
#endif
}
