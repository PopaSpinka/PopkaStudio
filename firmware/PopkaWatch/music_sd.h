// music_sd.h — the Music player's tracks from a microSD card, on a board that has one (HALO_SD_* in its board.h: the
// Waveshare 1.75 without the C). Included from music.h; the player (player.h) does not know where its tracks come from.
//
// The card: FAT32 (or FAT16), MP3 (MPEG-1/2/2.5 Layer III, CBR or VBR) and WAV (16-bit PCM, mono or stereo) files in
// /music — or, without that folder, in the card's root; up to MS_MAX of them, by name. Read once at start (the card is
// not watched: a card put in later is seen after a restart). A card with tracks is the player's; without one — the
// flash's tracks, as before.
//
//   music_begin → ms_scan: each file's title and artist (MP3: the ID3v2 tag's TIT2/TPE1; else the file's name, "microSD"),
//   its length (MP3: the Xing/Info or VBRI frame count, else the size over the bitrate; WAV: the data's size)
//   → ms_dec (core 0, stack in PSRAM): the file → the Helix MP3 decoder (Arduino-ESP32 3.3.11's mp3dec.h, as the
//   Radio's) or the WAV's samples → mono → 22 050 Hz (the speaker's: a 2-tap average and linear steps, radio_net.h's
//   way) → a PCM ring in PSRAM → the speaker's fill (mu_fill → ms_fill, in the audio task).
//
// The player talks to it as to the flash's music: mu_cmd (start that track), mu_play, mu_pos, mu_ended.
// The fonts have Latin letters only: Cyrillic in a title is written in Latin letters (ms_latin), the rest dropped.
// Command: sd — the card and its tracks.
#pragma push_macro("DP")
#pragma push_macro("W")
#pragma push_macro("H")
#undef DP
#undef W
#undef H
#include <SPI.h>
#include <SD.h>
#include <freertos/idf_additions.h>
#include "mp3dec.h"
#pragma pop_macro("H")
#pragma pop_macro("W")
#pragma pop_macro("DP")

#define MS_MAX MU_MAX        // tracks on the card
#define MS_PATH 128          // a file's path, UTF-8
#define MS_PCM 16384         // decoded samples (PSRAM): 0.74 s; a power of two
#define MS_INB 4096          // the MP3 decoder's input window
#define MS_SPI_HZ 20000000

enum { MS_NONE, MS_MP3, MS_WAV };
struct MsFile {
  char path[MS_PATH];
  uint8_t kind, ch;
  uint32_t at, end;  // where the sound starts and ends in the file
  uint32_t rate;     // WAV's rate (MP3's: from its frames)
};
static MsFile *ms_files;  // MS_MAX (PSRAM)
static bool mu_sd;        // the player's tracks are the card's
static int mu_flash_n;    // the flash's tracks (the music command: studio.py writes them when there are none)
static SPIClass *ms_spi;
static bool ms_card;
static uint64_t ms_card_mb;

// The fill ↔ the decoder
static int16_t *ms_pcm;
static volatile uint32_t ms_pw, ms_pr;
static volatile int ms_want = -1;     // the track the decoder should play (set by the fill with ms_gen)
static volatile uint32_t ms_gen;      // +1 — a new track
static volatile bool ms_drop;         // the decoder: the ring's old samples are stale (the fill empties it)
static volatile bool ms_starting;     // a new track asked for, its first samples not there yet
static volatile bool ms_eof;          // the decoder came to the file's end
static uint32_t ms_st_frames, ms_st_errs, ms_st_us;
static inline uint32_t ms_pused() { return __atomic_load_n(&ms_pw, __ATOMIC_ACQUIRE) - ms_pr; }

// ---------- Names: UTF-8 → what the fonts have (Latin letters, "é"): Cyrillic written in Latin letters ----------
static void ms_latin(char *out, int cap, const char *in) {
  static const char *const CYR[64] = {
      "A", "B", "V", "G", "D", "E", "Zh", "Z", "I", "Y", "K", "L", "M", "N", "O", "P",  // А…П
      "R", "S", "T", "U", "F", "Kh", "Ts", "Ch", "Sh", "Shch", "", "Y", "", "E", "Yu", "Ya",  // Р…Я
      "a", "b", "v", "g", "d", "e", "zh", "z", "i", "y", "k", "l", "m", "n", "o", "p",  // а…п
      "r", "s", "t", "u", "f", "kh", "ts", "ch", "sh", "shch", "", "y", "", "e", "yu", "ya"};  // р…я
  int o = 0;
  auto put = [&](const char *s, int n) {
    if (o + n < cap) memcpy(out + o, s, n), o += n;
  };
  const uint8_t *p = (const uint8_t *)in;
  while (*p) {
    uint32_t c = *p;
    int len = 1;
    if (c >= 0xF0 && p[1] && p[2] && p[3]) c = (c & 7) << 18 | (p[1] & 63) << 12 | (p[2] & 63) << 6 | (p[3] & 63), len = 4;
    else if (c >= 0xE0 && p[1] && p[2]) c = (c & 15) << 12 | (p[1] & 63) << 6 | (p[2] & 63), len = 3;
    else if (c >= 0xC0 && p[1]) c = (c & 31) << 6 | (p[1] & 63), len = 2;
    p += len;
    if (c >= 0x20 && c < 0x7F) {
      const char ch = (char)c;
      put(&ch, 1);
    } else if (c >= 0x410 && c <= 0x44F) {
      put(CYR[c - 0x410], strlen(CYR[c - 0x410]));
    } else if (c == 0x401) {
      put("Yo", 2);
    } else if (c == 0x451) {
      put("yo", 2);
    } else if (c == 0xE9) {
      put("\xC3\xA9", 2);  // é is in the fonts
    } else if (c == 0x2013 || c == 0x2014) {
      put("-", 1);
    } else if (c == 0x2018 || c == 0x2019) {
      put("'", 1);
    } else if (c == 0x201C || c == 0x201D || c == 0xAB || c == 0xBB) {
      put("\"", 1);
    } else if (c == 0x2026) {
      put("...", 3);
    } else if (c == 0xA0) {
      put(" ", 1);
    }
  }
  out[o] = 0;
}

// ---------- The card's files ----------
static bool ms_ext(const char *name, const char *ext) {
  const size_t n = strlen(name), e = strlen(ext);
  return n > e && !strcasecmp(name + n - e, ext);
}

static uint32_t ms_be32(const uint8_t *b) { return (uint32_t)b[0] << 24 | b[1] << 16 | b[2] << 8 | b[3]; }
static uint32_t ms_le32(const uint8_t *b) { return b[0] | b[1] << 8 | b[2] << 16 | (uint32_t)b[3] << 24; }
static uint32_t ms_syncsafe(const uint8_t *b) { return (b[0] & 127) << 21 | (b[1] & 127) << 14 | (b[2] & 127) << 7 | (b[3] & 127); }

// An ID3v2 text frame (its encoding byte first) → UTF-8
static void ms_id3_text(char *out, int cap, const uint8_t *b, int n) {
  if (n < 1) return;
  const int enc = b[0];
  b++, n--;
  int o = 0;
  auto put = [&](uint32_t c) {
    char u[4];
    int k = 0;
    if (c < 0x80) u[k++] = c;
    else if (c < 0x800) u[k++] = 0xC0 | c >> 6, u[k++] = 0x80 | (c & 63);
    else u[k++] = 0xE0 | c >> 12, u[k++] = 0x80 | (c >> 6 & 63), u[k++] = 0x80 | (c & 63);
    if (o + k < cap) memcpy(out + o, u, k), o += k;
  };
  if (enc == 1 || enc == 2) {  // UTF-16: with its BOM (1), big-endian (2)
    bool be = enc == 2;
    if (enc == 1 && n >= 2) be = b[0] == 0xFE && b[1] == 0xFF, b += 2, n -= 2;
    for (int i = 0; i + 1 < n; i += 2) {
      const uint32_t c = be ? b[i] << 8 | b[i + 1] : b[i + 1] << 8 | b[i];
      if (!c) break;
      if (c < 0xD800 || c > 0xDFFF) put(c);
    }
  } else {
    for (int i = 0; i < n && b[i]; i++) {
      if (enc == 3 || b[i] < 0x80) {  // UTF-8 as it is
        if (o + 1 < cap) out[o++] = b[i];
      } else {
        put(b[i]);  // ISO-8859-1
      }
    }
  }
  out[o] = 0;
}

// An MP3's sound: after its ID3v2 tag (its title and artist read from it); where it starts, how many samples at 22 050 Hz
static bool ms_mp3_probe(File &f, MsFile &m, MuTrack &t, uint8_t *buf, int cap) {
  const uint32_t size = f.size();
  uint32_t at = 0;
  char title[128] = "", artist[128] = "";
  if (f.read(buf, 10) == 10 && !memcmp(buf, "ID3", 3) && buf[3] >= 3 && buf[3] <= 4) {
    const int ver = buf[3];
    const uint32_t tag = ms_syncsafe(buf + 6);
    at = 10 + tag + (buf[5] & 0x10 ? 10 : 0);
    const int n = f.read(buf, min((uint32_t)cap, tag));
    for (int i = 0; i + 10 <= n;) {
      if (!buf[i]) break;  // the padding
      const uint32_t fs = ver == 4 ? ms_syncsafe(buf + i + 4) : ms_be32(buf + i + 4);
      if (fs == 0 || i + 10 + fs > (uint32_t)n) break;
      if (!memcmp(buf + i, "TIT2", 4)) ms_id3_text(title, sizeof(title), buf + i + 10, fs);
      if (!memcmp(buf + i, "TPE1", 4)) ms_id3_text(artist, sizeof(artist), buf + i + 10, fs);
      i += 10 + fs;
    }
  }
  if (title[0]) ms_latin(t.title, sizeof(t.title), title);
  if (artist[0]) ms_latin(t.artist, sizeof(t.artist), artist);
  // The first frame: its header — the rate, the bitrate; its Xing/Info or VBRI — the frame count of a VBR file
  if (!f.seek(at)) return false;
  const int n = f.read(buf, min(cap, 4096));
  static const uint16_t BR1[16] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
  static const uint16_t BR2[16] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
  static const uint32_t SR[3] = {44100, 48000, 32000};
  for (int i = 0; i + 40 < n; i++) {
    if (buf[i] != 0xFF || (buf[i + 1] & 0xE0) != 0xE0) continue;
    const int ver = buf[i + 1] >> 3 & 3, layer = buf[i + 1] >> 1 & 3, bri = buf[i + 2] >> 4, sri = buf[i + 2] >> 2 & 3;
    if (ver == 1 || layer != 1 || bri == 0 || bri == 15 || sri == 3) continue;  // Layer III only
    const bool v1 = ver == 3, mono = (buf[i + 3] >> 6) == 3;
    const uint32_t rate = SR[sri] >> (v1 ? 0 : ver == 2 ? 1 : 2), kbps = (v1 ? BR1 : BR2)[bri];
    const uint32_t spf = v1 ? 1152 : 576;
    const int side = v1 ? (mono ? 17 : 32) : (mono ? 9 : 17);
    uint32_t frames = 0;
    const uint8_t *x = buf + i + 4 + side;
    if (x + 12 <= buf + n && (!memcmp(x, "Xing", 4) || !memcmp(x, "Info", 4)) && (x[7] & 1)) frames = ms_be32(x + 8);
    const uint8_t *v = buf + i + 36;
    if (!frames && v + 18 <= buf + n && !memcmp(v, "VBRI", 4)) frames = ms_be32(v + 14);
    const double secs = frames ? (double)frames * spf / rate : (double)(size - at - i) * 8 / (kbps * 1000.0);
    m.kind = MS_MP3, m.at = at + i, m.end = size, m.rate = rate;
    t.samples = (uint32_t)(secs * mu_rate);
    return true;
  }
  return false;
}

// A WAV's sound: its "fmt " (16-bit PCM, 1 or 2 channels) and "data" chunks
static bool ms_wav_probe(File &f, MsFile &m, MuTrack &t) {
  uint8_t h[24];
  if (f.read(h, 12) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return false;
  int ch = 0, bits = 0, fmt = 0;
  uint32_t rate = 0;
  while (f.read(h, 8) == 8) {
    const uint32_t len = ms_le32(h + 4), pos = f.position();
    if (!memcmp(h, "fmt ", 4) && len >= 16 && f.read(h + 8, 16) == 16) {
      fmt = h[8] | h[9] << 8, ch = h[10] | h[11] << 8, rate = ms_le32(h + 12), bits = h[22] | h[23] << 8;
    } else if (!memcmp(h, "data", 4)) {
      if ((fmt != 1 && fmt != 0xFFFE) || bits != 16 || ch < 1 || ch > 2 || rate < 8000) return false;
      m.kind = MS_WAV, m.ch = ch, m.rate = rate, m.at = pos, m.end = min((uint32_t)f.size(), pos + len);
      t.samples = (uint32_t)((uint64_t)(m.end - m.at) / (2 * ch) * mu_rate / rate);
      return true;
    }
    if (!f.seek(pos + len + (len & 1))) break;
  }
  return false;
}

// The card: mounted, its tracks listed into mu_tracks (by name) — true: it has some
static bool ms_scan() {
  ms_spi = new SPIClass(HSPI);  // SPI3: SPI2 is the display's
  ms_spi->begin(HALO_SD_SCK, HALO_SD_MISO, HALO_SD_MOSI, HALO_SD_CS);
  if (!SD.begin(HALO_SD_CS, *ms_spi, MS_SPI_HZ, "/sd", 2)) return LOG("sd: no card\n"), false;
  ms_card = true, ms_card_mb = SD.cardSize() >> 20;
  const char *dir = SD.exists("/music") ? "/music" : "/";
  File d = SD.open(dir);
  if (!d || !d.isDirectory()) return LOG("sd: %s is not a folder\n", dir), false;
  ms_files = (MsFile *)heap_caps_calloc(MS_MAX, sizeof(MsFile), MALLOC_CAP_SPIRAM);
  uint8_t *buf = (uint8_t *)heap_caps_malloc(16384, MALLOC_CAP_SPIRAM);
  if (!ms_files || !buf) return free(buf), LOG("sd: no PSRAM\n"), false;
  // The names first (by name, as a file manager shows them), then each file's tags and length
  char(*names)[MS_PATH] = (char(*)[MS_PATH])heap_caps_malloc(MS_MAX * MS_PATH, MALLOC_CAP_SPIRAM);
  if (!names) return free(buf), LOG("sd: no PSRAM\n"), false;
  int n = 0;
  for (File e = d.openNextFile(); e && n < MS_MAX; e = d.openNextFile()) {
    const char *name = e.name();
    if (!e.isDirectory() && name[0] != '.' && (ms_ext(name, ".mp3") || ms_ext(name, ".wav")))
      snprintf(names[n++], MS_PATH, "%s%s%s", dir, strcmp(dir, "/") ? "/" : "", name);
    e.close();
  }
  d.close();
  qsort(names, n, MS_PATH, [](const void *a, const void *b) { return strcasecmp((const char *)a, (const char *)b); });
  int k = 0;
  for (int i = 0; i < n; i++) {
    File f = SD.open(names[i]);
    if (!f) continue;
    MsFile &m = ms_files[k];
    MuTrack t = {};  // (into mu_tracks only if it plays: until then the flash's tracks stay)
    strlcpy(m.path, names[i], MS_PATH);
    char base[MS_PATH];  // the title by default: the file's name without its folder and extension
    strlcpy(base, strrchr(names[i], '/') + 1, sizeof(base));
    if (char *dot = strrchr(base, '.')) *dot = 0;
    ms_latin(t.title, sizeof(t.title), base);
    strlcpy(t.artist, "microSD", sizeof(t.artist));
    const bool ok = ms_ext(names[i], ".mp3") ? ms_mp3_probe(f, m, t, buf, 16384) : ms_wav_probe(f, m, t);
    f.close();
    if (!ok) {
      LOG("sd: %s — not a track this player plays\n", names[i]);
      continue;
    }
    if (!t.title[0]) strlcpy(t.title, "Track", sizeof(t.title));
    mu_tracks[k++] = t;
  }
  free(names), free(buf);
  LOG("sd: card %u MB, %d tracks in %s\n", (unsigned)ms_card_mb, k, dir);
  if (!k) return false;
  mu_n = k, mu_rate = 22050;
  return true;
}

// ---------- The decoder's task: the file → mono → 22 050 Hz → the PCM ring ----------
static int16_t ms_src[1153];  // [0] — the last sample of the piece before
static int ms_prev;
static uint32_t ms_rpos;
static void ms_out(const int16_t *pcm, int ns, int ch, int sr) {  // (radio_net.h's rn_out)
  const bool lp = sr > 22050 * 3 / 2;  // going down 2× or so: a 2-tap average first
  for (int j = 0; j < ns; j++) {
    const int x = ch == 2 ? (pcm[2 * j] + pcm[2 * j + 1]) >> 1 : pcm[j];
    ms_src[1 + j] = lp ? (x + ms_prev) >> 1 : x;
    ms_prev = x;
  }
  const uint32_t step = ((uint32_t)sr << 16) / 22050;
  uint32_t w = ms_pw;
  while ((ms_rpos >> 16) < (uint32_t)ns) {
    const int i = ms_rpos >> 16, f = (ms_rpos & 0xFFFF) >> 1;
    ms_pcm[w++ & (MS_PCM - 1)] = ms_src[i] + (((ms_src[i + 1] - ms_src[i]) * f) >> 15);
    ms_rpos += step;
  }
  __atomic_store_n(&ms_pw, w, __ATOMIC_RELEASE);
  ms_rpos -= (uint32_t)ns << 16;
  ms_src[0] = ms_src[ns];
}

static void ms_dec_loop(void *) {
  static int16_t pcm[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
  static uint8_t *inb;
  if (!inb) inb = (uint8_t *)heap_caps_malloc(MS_INB, MALLOC_CAP_SPIRAM);
  HMP3Decoder h = nullptr;
  File f;
  const MsFile *m = nullptr;
  uint32_t gen = ms_gen, left_in_file = 0;
  uint8_t *in = inb;
  int left = 0;
  for (;;) {
    if (gen != ms_gen) {  // a new track: the old file closed, the ring emptied (by the fill), the new one opened
      gen = ms_gen;
      if (f) f.close();
      const int t = ms_want;
      m = t >= 0 && t < mu_n ? &ms_files[t] : nullptr;
      if (m && (f = SD.open(m->path)) && f.seek(m->at)) left_in_file = m->end - m->at;
      else m = nullptr, LOG("sd: can't open track %d\n", t);
      in = inb, left = 0, ms_rpos = 0, ms_prev = 0, ms_src[0] = 0, ms_eof = !m;
      ms_drop = true;
      while (ms_drop && gen == ms_gen) vTaskDelay(1);  // (the fill drops the stale samples, then the new ones come)
      ms_starting = false;
      continue;
    }
    if (!m || ms_eof || MS_PCM - ms_pused() < 2400) {  // nothing to play, or the ring is full enough
      vTaskDelay(pdMS_TO_TICKS(m && !ms_eof ? 8 : 20));
      continue;
    }
    if (m->kind == MS_WAV) {
      const int fr = min((uint32_t)1152, left_in_file / (2 * m->ch));
      const int got = fr > 0 ? f.read((uint8_t *)pcm, fr * 2 * m->ch) : 0;
      if (got < 2 * m->ch) {
        ms_eof = true;
        continue;
      }
      left_in_file -= got;
      ms_out(pcm, got / (2 * m->ch), m->ch, m->rate);
      continue;
    }
    // MP3: the input window topped up from the file, a frame decoded
    if (!h && !(h = MP3InitDecoder())) {
      LOG("sd: no memory for the MP3 decoder\n");
      ms_eof = true;
      continue;
    }
    if (in != inb && left) memmove(inb, in, left);
    in = inb;
    if (left < MS_INB && left_in_file) {
      const int got = f.read(inb + left, min((uint32_t)(MS_INB - left), left_in_file));
      if (got > 0) left += got, left_in_file -= got;
      else left_in_file = 0;
    }
    if (left < 4) {
      ms_eof = true;
      continue;
    }
    const int off = MP3FindSyncWord(in, left);
    if (off < 0) {
      left = left_in_file ? 0 : left;  // (no frame in the window: the next window; at the end — done)
      if (!left_in_file) ms_eof = true;
      continue;
    }
    in += off, left -= off;
    const int before = left;
    const int64_t t0 = esp_timer_get_time();
    const int err = MP3Decode(h, &in, &left, pcm, 0);
    if (err == ERR_MP3_INDATA_UNDERFLOW || err == ERR_MP3_MAINDATA_UNDERFLOW) {
      if (!left_in_file && left == before) ms_eof = true;  // the last bytes are not a whole frame
      continue;
    }
    if (err) {
      ms_st_errs++, in++, left--;
      continue;
    }
    ms_st_us += esp_timer_get_time() - t0, ms_st_frames++;
    MP3FrameInfo fi;
    MP3GetLastFrameInfo(h, &fi);
    if (fi.nChans >= 1 && fi.nChans <= 2 && fi.samprate >= 8000) ms_out(pcm, fi.outputSamps / fi.nChans, fi.nChans, fi.samprate);
  }
}

// The speaker's task (mu_fill): the player's commands, then what the decoder made
static int ms_fill(int16_t *out, int n) {
  const int cmd = mu_cmd;
  if (cmd >= 0 && cmd < mu_n) {
    mu_cmd = -1;
    ms_starting = true, ms_want = cmd, mu_cur = cmd, mu_pos = 0, mu_ended = false;
    __atomic_add_fetch(&ms_gen, 1, __ATOMIC_RELEASE);
  }
  if (ms_drop) ms_pr = ms_pw, ms_drop = false;
  if (!mu_play || ms_starting || mu_ended) return 0;
  const int k = min((uint32_t)n, ms_pused());
  for (int i = 0; i < k; i++) out[i] = ms_pcm[(ms_pr + i) & (MS_PCM - 1)];
  __atomic_store_n(&ms_pr, ms_pr + k, __ATOMIC_RELEASE);
  if (!k && ms_eof) mu_ended = true;  // the file is done and its last samples played
  mu_pos += k;
  return k;
}

static void ms_status() {
  LOG("sd: %s, %u MB; %d tracks%s; decoded %u frames (%.2f ms each), errors %u\n", ms_card ? "card" : "no card",
      (unsigned)ms_card_mb, mu_sd ? mu_n : 0, mu_sd ? "" : " (the player plays the flash's)", (unsigned)ms_st_frames,
      ms_st_frames ? ms_st_us / 1000.0f / ms_st_frames : 0, (unsigned)ms_st_errs);
  for (int i = 0; mu_sd && i < mu_n; i++)
    LOG("  %2d  %s  %s — %s, %u s\n", i + 1, ms_files[i].kind == MS_MP3 ? "mp3" : "wav", mu_tracks[i].title,
        mu_tracks[i].artist, (unsigned)(mu_tracks[i].samples / mu_rate));
}

// music_begin, after the flash's tracks are read: the card's take their place when it has some
static void ms_begin() {
  halo_command("sd", [](const char *) { ms_status(); });
  ms_pcm = (int16_t *)heap_caps_malloc(MS_PCM * 2, MALLOC_CAP_SPIRAM);  // (first: once the card's tracks are listed, they are the player's)
  if (!ms_pcm) return LOG("sd: no PSRAM for the ring\n");
  if (!ms_scan()) return;
  mu_sd = true;
  // On core 0 (with the speaker and Wi-Fi), its stack in PSRAM: internal memory is for Wi-Fi
  xTaskCreatePinnedToCoreWithCaps(ms_dec_loop, "music_sd", 8192, nullptr, 4, nullptr, 0, MALLOC_CAP_SPIRAM);
}
