// video_hw.h — what the Video app (video.h) needs from the hardware: the microSD card, the JPEG decoder, the sound, the
// refresh of the screen. How all of it works and every number: docs/p4-video.md (experiments/P4Video is where it was tried).
//
// A clip is one file, video/NAME.p4v on the card (PopkaStudio/p4v.py makes it): a header, a table of frames, then a record a
// frame — its JPEG and its sound. One task reads ahead into a ring in PSRAM; the frame task picks the frame for the next
// boundary and has the hardware decode it straight into the frame buffer. The sound is the clock.
//
// In the editor's page (__EMSCRIPTEN__) there is no card and no decoder: six made-up clips with coloured pictures, and a
// "playing" that is only a clock, so that the two screens can be seen and tried.
#pragma once

#define VH_MAX 48     // clips on the screen of tiles
#define VH_TW 320     // a clip's small picture: a whole number of JPEG blocks, the tile shows the middle of it
#define VH_TH 240
#define VH_W 1024     // the picture: a whole number of JPEG blocks, 12 black rows above and below
#define VH_H 576
#define VH_TOP 12

struct VhFile {
  char name[64];       // the file in video/
  char title[44];      // what the tile says
  char dur[10];        // "9:56"
  uint32_t seconds;
  uint16_t *thumb;     // VH_TW x VH_TH
  uint8_t net;         // from a computer over Wi-Fi (video_net.h), not from the card
  uint32_t bytes;      // (a network clip: the file's size)
};
static VhFile *vh_files;
static volatile int vh_n;        // the clips ready so far (they appear one by one)
static volatile bool vh_scanning;
static uint16_t *vh_still;       // VH_W x VH_H: the picture the tile grows into and shrinks from
static uint16_t *vh_half;        // the same, half the size: moving, it is drawn from this (a quarter of the bytes out of PSRAM)
static volatile bool vh_still_ok;
static void (*vh_on_picture)();  // a whole new picture under the interface: the player marks all of its own over it (halo_under)

// The words under the tile's title have to be read on any picture: the lower part is darkened once, here
static void vh_bake(uint16_t *t) {
  const int y0 = 4 + 232 * 42 / 100, y1 = 236;
  for (int y = y0; y < VH_TH; y++) {
    const int a = y >= y1 ? 26 : (y - y0) * 26 / (y1 - y0), k = 32 - a;  // (to 0.82 of black, in 32 steps)
    uint16_t *p = t + y * VH_TW;
    for (int x = 0; x < VH_TW; x++) {
      const uint32_t c = p[x], s = (c | c << 16) & 0x07E0F81Fu, z = (s * k >> 5) & 0x07E0F81Fu;
      p[x] = (uint16_t)(z | z >> 16);
    }
  }
}

static void vh_make_half() {
  for (int y = 0; y < VH_H / 2; y++) {
    const uint16_t *a = vh_still + y * 2 * VH_W, *b = a + VH_W;
    uint16_t *to = vh_half + y * VH_W / 2;
    for (int x = 0; x < VH_W / 2; x++, a += 2, b += 2) {  // (the four pixels' sum, each channel on its own)
      const uint32_t p0 = (a[0] | (uint32_t)a[0] << 16) & 0x07E0F81Fu, p1 = (a[1] | (uint32_t)a[1] << 16) & 0x07E0F81Fu;
      const uint32_t p2 = (b[0] | (uint32_t)b[0] << 16) & 0x07E0F81Fu, p3 = (b[1] | (uint32_t)b[1] << 16) & 0x07E0F81Fu;
      const uint32_t z = ((p0 >> 2 & 0x01F83E07u) + (p1 >> 2 & 0x01F83E07u) + (p2 >> 2 & 0x01F83E07u) + (p3 >> 2 & 0x01F83E07u));
      to[x] = (uint16_t)((z & 0x07E0F81Fu) | (z & 0x07E0F81Fu) >> 16);
    }
  }
}

static void vh_duration(VhFile &F) {
  snprintf(F.dur, sizeof F.dur, "%lu:%02lu", (unsigned long)F.seconds / 60, (unsigned long)F.seconds % 60);
}

// The title, cut to fit beside the duration
static void vh_fit_title(VhFile &F) {
  extern const HaloFont gl_r_v24, gl_r_n21;
  const int room = 308 - 28 - 12 - halo_text_width(gl_r_n21, F.dur);
  if (halo_text_width(gl_r_v24, F.title) <= room) return;
  int n = strlen(F.title);
  while (n > 1) {
    F.title[--n] = 0;
    char probe[48];
    snprintf(probe, sizeof probe, "%s...", F.title);
    if (halo_text_width(gl_r_v24, probe) <= room) return (void)strlcpy(F.title, probe, sizeof F.title);
  }
}

#ifdef __EMSCRIPTEN__
// ---------- The editor's page: no hardware ----------
#define VH_UNDER 0
static float vh_t, vh_len = 62;
static bool vh_play, vh_opened;
static int vh_sel;

static void vh_paint(uint16_t *p, int w, int h, int i) {
  static const uint32_t C[6][2] = {{0x6F5AA8, 0x272447}, {0x4C7FA8, 0x1C3247}, {0xA8635A, 0x47261F},
                                   {0x5AA883, 0x1F4735}, {0xA89A5A, 0x47401F}, {0xA85A8F, 0x471F3B}};
  const uint32_t a = C[i % 6][0], b = C[i % 6][1];
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      const float t = (x / (float)w * 0.574f + y / (float)h * 0.819f) / 1.393f;
      const int r = (a >> 16 & 255) + ((int)(b >> 16 & 255) - (int)(a >> 16 & 255)) * t;
      const int g = (a >> 8 & 255) + ((int)(b >> 8 & 255) - (int)(a >> 8 & 255)) * t;
      const int l = (a & 255) + ((int)(b & 255) - (int)(a & 255)) * t;
      p[y * w + x] = (r & 0xF8) << 8 | (g & 0xFC) << 3 | l >> 3;
    }
}

static void vh_begin() {
  if (vh_files) return;
  static const char *const N[6] = {"Morning Walk", "Sea Waves", "City Lights", "Mountains", "Forest Trail", "Night Sky"};
  static const uint32_t S[6] = {596, 198, 65, 42, 147, 251};
  vh_files = (VhFile *)calloc(VH_MAX, sizeof(VhFile));
  vh_still = (uint16_t *)malloc(VH_W * VH_H * 2);
  vh_half = (uint16_t *)malloc(VH_W * VH_H / 2);
  for (int i = 0; i < 6; i++) {
    VhFile &F = vh_files[i];
    strlcpy(F.title, N[i], sizeof F.title), F.seconds = S[i], vh_duration(F);
    F.thumb = (uint16_t *)malloc(VH_TW * VH_TH * 2);
    vh_paint(F.thumb, VH_TW, VH_TH, i), vh_bake(F.thumb);
    vh_fit_title(F);
  }
  vh_n = 6;
}
static void vh_open(int i) { vh_sel = i, vh_t = 0, vh_len = vh_files[i].seconds, vh_paint(vh_still, VH_W, VH_H, i), vh_make_half(), vh_still_ok = true, vh_opened = true; }
static bool vh_ready() { return vh_opened; }
static void vh_start() { vh_play = true; }
static void vh_stop() { vh_play = false; }
static bool vh_stopped() { return true; }
static void vh_close() { vh_opened = false, vh_still_ok = false; }
static void vh_abort() { vh_close(), vh_play = false; }
static bool vh_playing() { return vh_play; }
static void vh_set_playing(bool on) { vh_play = on; }
static bool vh_ended() { return vh_t >= vh_len; }
static float vh_length() { return vh_len; }
static float vh_position() { return vh_t; }
static void vh_seek(float frac, bool) { vh_t = frac * vh_len; }
static void vh_volume(float) {}
static void vh_tick(float dt) {
  if (vh_play && vh_t < vh_len) vh_t = fminf(vh_t + dt, vh_len);
}
static void vh_commands() {}
static bool vh_frozen;  // (here the player always draws vh_still itself)
static void vh_freeze(bool) {}

#else
// ---------- The board ----------
#define VH_UNDER 1
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <driver/jpeg_decode.h>
#include <driver/sdmmc_host.h>
#include <esp_cache.h>
#include <esp_heap_caps.h>
#include <esp_rom_crc.h>
#include <esp_vfs_fat.h>
#include <ff.h>
#include <sd_pwr_ctrl_by_on_chip_ldo.h>
#include <sdmmc_cmd.h>
#include "ports/p4_lcd7/halo_p4.h"

#define VH_RING 24             // frames read ahead (about a second)
#define VH_START_FRAMES 12     // frames in the ring before the picture and the sound start after a seek
#define VH_AUDIO_N (1 << 19)   // samples of sound read ahead (16 s at 32 kHz; the sound runs ahead of the picture only by the ring)
#define VH_POSTER_MAX (256 * 1024)
#define VH_DIR "/sdcard/video"
#define VH_PUT_CHUNK 8192      // a file over USB: chunks this big, each answered OK (PopkaStudio/studio.py: PUT_CHUNK)
#define VH_AUDIO_QUEUE 1400    // samples the I2S queue holds: what fill has handed over is heard this much later

static jpeg_decoder_handle_t vh_jpeg;
static sdmmc_card_t *vh_card;
static volatile bool vh_card_ok;

// The clip that is open
struct VhSdRun {
  uint32_t lba, sectors;
};
static struct {
  uint32_t num, den, count, a_rate;
  uint32_t *index;       // count + 1 offsets of the records in the file
  uint32_t *jpeg;        // the JPEG's size in each record
} vh_c;
static VhSdRun *vh_runs;
static uint32_t vh_run_n;

// The ring: the reader fills, the frame task empties (the slot in ring_tail is the frame on screen: it may be decoded again)
static uint8_t *ring_mem;
static uint32_t ring_slot;
static volatile uint32_t ring_head, ring_tail;
static uint32_t ring_size[VH_RING], ring_frame[VH_RING];
// The frame on screen, kept when a seek empties the ring: the picture under the interface can always be decoded again (a
// copy of the screen would bring what was drawn over it along, and the dark under the time line would double)
static uint8_t *vh_keep;
static uint32_t vh_keep_n, vh_keep_cap;

// The reader's orders: one at a time, it answers by making done equal gen. RD_HALF — vh_half from vh_still, for the close
enum { RD_NONE, RD_SCAN, RD_OPEN, RD_CLOSE, RD_HALF };
static volatile int vh_half_job;  // the close's vh_half from vh_still: 0 — not asked, 1 — the reader is making it, 2 — made
static volatile int rd_cmd, rd_arg;
static volatile uint32_t rd_gen, rd_done, rd_frame;
static volatile bool rd_more;   // read on after that frame (false — only it: a finger on the time line)
static volatile bool rd_open;
static uint32_t rd_next, rd_left;

// The sound
static int16_t *a_ring;
static volatile uint64_t a_write, a_read;
static volatile bool a_on;
static uint32_t a_phase, a_step = 65536;  // (16.16: the clip's samples a sample of the speaker)

// What the frame task is doing
static volatile bool vh_hook;           // the picture is ours
static volatile bool vh_stop_req, vh_stop_done;
// The player not seen as it is (on its way into Always On or dark): the frame on screen goes into vh_still, the picture
// is let go, and the player draws it like any other, through its view (vh_freeze)
static volatile int vh_freeze_req;  // 1 — asked, 2 — the frame is in vh_still, let go in the next frame
static volatile bool vh_frozen;
static volatile bool vh_paused, vh_end;
static volatile int vh_cur = -1;        // the frame on screen
static volatile bool vh_cur_ok;         // ... and it is still in the ring (a seek empties it)
static volatile bool vh_new;            // the next frame is a new one
static volatile int vh_want = -1;
static volatile bool vh_seeking;        // a go was ordered and its first frames have not come yet
static bool vh_go_final;                // ... and it reads on from there (else one frame: a finger on the time line)
static bool vh_seek_final, vh_scrub;    // the place asked for is the last one; a finger is on the time line
static float vh_seek_pending = -1;
static uint32_t vh_a_from;              // the frame the sound in the ring starts with (a go empties it, the count starts at 0)
static float vh_hz, vh_rate_req = -1;
static float vh_fpb = 0.5f;             // frames a boundary
static float vh_period_us = 20000;
static uint32_t vh_base_te;
static int vh_base_frame;
static double vh_err;
static uint32_t vh_open_ms;
static uint64_t vh_a_seen;
static uint32_t vh_a_ms;
// vstat
static volatile bool vh_stat_on;
static uint32_t vs_shown, vs_sum, vs_max, vs_at, vs_shift, vs_wait;
static double vs_off;

static int vh_fill(int16_t *b, int n) {  // the audio task (core 0)
  if (!a_on) return 0;
  const uint64_t r = a_read, have = a_write - r;
  if (a_step == 65536) {
    const int m = (int)min((uint64_t)n, have);
    for (int i = 0; i < m; i++) b[i] = a_ring[(r + i) & (VH_AUDIO_N - 1)];
    a_read = r + m;
    return m;
  }
  uint32_t ph = a_phase;  // (another rate on the speaker: the music's 22 050 Hz, when the project has Music or the Recorder)
  int i = 0;
  for (; i < n; i++) {
    const uint32_t k = ph >> 16;
    if (k + 1 >= have) break;
    const int s0 = a_ring[(r + k) & (VH_AUDIO_N - 1)], s1 = a_ring[(r + k + 1) & (VH_AUDIO_N - 1)];
    b[i] = (int16_t)(s0 + ((s1 - s0) * (int32_t)(ph >> 1 & 0x7FFF) >> 15));
    ph += a_step;
  }
  a_read = r + (ph >> 16);
  a_phase = ph & 0xFFFF;
  return i;
}

// The card, slot 0 (its own pins 39-44, powered by the chip's LDO 4), 4 bits at 40 MHz. The C6's Wi-Fi is on slot 1 of the
// same controller: whichever comes second finds it already started, so the init here is done once and only here
static bool vh_mount() {
  sd_pwr_ctrl_ldo_config_t ldo = {};
  ldo.ldo_chan_id = 4;
  sd_pwr_ctrl_handle_t power = nullptr;
  if (sd_pwr_ctrl_new_on_chip_ldo(&ldo, &power) != ESP_OK) return false;
  const esp_err_t he = sdmmc_host_init();
  if (he != ESP_OK && he != ESP_ERR_INVALID_STATE) return false;
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SDMMC_HOST_SLOT_0;
  host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
  host.pwr_ctrl_handle = power;
  host.init = []() -> esp_err_t { return ESP_OK; };
  host.deinit = []() -> esp_err_t { return ESP_OK; };
  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = 4;
  esp_vfs_fat_mount_config_t mc = {};
  mc.max_files = 4;
  mc.allocation_unit_size = 64 * 1024;
  const esp_err_t e = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot, &mc, &vh_card);
  if (e != ESP_OK) LOG("video: no card (%s)\n", esp_err_to_name(e));
  return e == ESP_OK;
}

static bool vh_decode(const uint8_t *src, uint32_t n, uint16_t *to, uint32_t bytes) {
  static jpeg_decode_cfg_t cfg = {};
  cfg.output_format = JPEG_DECODE_OUT_FORMAT_RGB565;
  cfg.rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR;  // little-endian RGB565, what the panel's buffer holds
  cfg.conv_std = JPEG_YUV_RGB_CONV_STD_BT601;
  uint32_t out = 0;
  const esp_err_t e = jpeg_decoder_process(vh_jpeg, &cfg, src, n, (uint8_t *)to, bytes, &out);
  if (e != ESP_OK) LOG("video: jpeg: %s\n", esp_err_to_name(e));
  return e == ESP_OK;
}


static volatile bool vh_rescan;  // the clips again when nothing is open (a stream came or went: video_net.h)
#if FN_WIFI
#include "video_net.h"  // clips straight from a computer over Wi-Fi (the streaming bench)
#endif

// Every clip's picture: the computer's first (a stream: the tile with the green frame, video.h), then the card's by name.
// They appear on the screen as they come. A slot keeps its picture's memory from one scan to the next
static void vh_scan() {
  vh_scanning = true;
  vh_n = 0;
  delay(40);  // (a frame in progress is done with the old list)
  static uint8_t *poster;
  if (!poster) poster = (uint8_t *)heap_caps_aligned_alloc(128, VH_POSTER_MAX, MALLOC_CAP_SPIRAM);
  static char (*names)[sizeof(VhFile::name)];
  if (!names) names = (char (*)[sizeof(VhFile::name)])heap_caps_malloc(VH_MAX * sizeof(VhFile::name), MALLOC_CAP_SPIRAM);
  int found = 0;
  if (DIR *d = names ? opendir(VH_DIR) : nullptr) {
    while (dirent *e = readdir(d)) {
      const size_t len = strlen(e->d_name);
      if (found >= VH_MAX || len < 5 || len >= sizeof(VhFile::name) || strcasecmp(e->d_name + len - 4, ".p4v") || e->d_name[0] == '.') continue;
      strlcpy(names[found++], e->d_name, sizeof(VhFile::name));
    }
    closedir(d);
  }
  qsort(names, found, sizeof(VhFile::name), [](const void *x, const void *y) { return strcasecmp((const char *)x, (const char *)y); });
  int n = 0;
#if FN_WIFI
  vn_scan(n);
#endif
  const int net = n;
  for (int i = 0; i < found && n < VH_MAX; i++) {
    char path[96];
    uint32_t h[16];
    VhFile F = vh_files[n];  // (the slot, with its picture's memory)
    strlcpy(F.name, names[i], sizeof F.name);
    snprintf(path, sizeof path, VH_DIR "/%s", F.name);
    const int fd = open(path, O_RDONLY);
    if (fd < 0) continue;
    bool ok = read(fd, h, 64) == 64 && !memcmp(h, "P4V2", 4) && !h[1] && h[4] && h[6];
    if (ok && F.thumb == nullptr) F.thumb = (uint16_t *)heap_caps_aligned_alloc(128, VH_TW * VH_TH * 2, MALLOC_CAP_SPIRAM);
    if (ok && F.thumb && h[9] && h[10] && h[10] <= VH_POSTER_MAX) {
      const uint32_t size = h[10];
      if (lseek(fd, h[9], SEEK_SET) == (off_t)h[9] && read(fd, poster, size) == (ssize_t)size) {
        esp_cache_msync(poster, (size + 127) & ~127, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        if (vh_decode(poster, size, F.thumb, VH_TW * VH_TH * 2)) {
          esp_cache_msync(F.thumb, VH_TW * VH_TH * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
          vh_bake(F.thumb);
          esp_cache_msync(F.thumb, VH_TW * VH_TH * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        }
      }
    } else if (ok && F.thumb) {
      memset(F.thumb, 0x22, VH_TW * VH_TH * 2);  // (a clip without a picture)
    }
    close(fd);
    if (!ok || !F.thumb) {
      vh_files[n].thumb = F.thumb;  // (the memory stays with the slot)
      continue;
    }
    strlcpy(F.title, F.name, sizeof F.title);
    if (char *dot = strrchr(F.title, '.')) *dot = 0;
    for (char *c = F.title; *c; c++)
      if (*c == '_') *c = ' ';
    F.seconds = (uint32_t)((uint64_t)h[6] * h[5] / h[4]), F.net = 0, F.bytes = 0;
    vh_duration(F);
    vh_fit_title(F);
    vh_files[n] = F;
    __sync_synchronize();
    vh_n = ++n;
  }
  LOG("video: %d clips on the card\n", n - net);
  vh_scanning = false;
}

// Where the file lies on the card: runs of consecutive sectors, by walking FAT32 ourselves. Through the file system a read
// costs a card command per cluster; a run is one command
static bool vh_map(const char *name) {
  char path[96];
  snprintf(path, sizeof path, "0:/video/%s", name);
  FIL fil;
  if (f_open(&fil, path, FA_READ) != FR_OK) return LOG("video: f_open failed\n"), false;
  FATFS *fs = fil.obj.fs;
  bool ok = fs->fs_type == FS_FAT32;
  if (!ok) LOG("video: the card is not FAT32\n");
  static uint8_t *fat;
  if (!fat) fat = (uint8_t *)heap_caps_aligned_alloc(128, 512, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!vh_runs) vh_runs = (VhSdRun *)heap_caps_malloc(8192 * sizeof(VhSdRun), MALLOC_CAP_SPIRAM);
  uint32_t n = 0, cached = UINT32_MAX, cl = fil.obj.sclust, left = (f_size(&fil) + 511) / 512;
  while (ok && left && cl >= 2 && cl < fs->n_fatent) {
    const uint32_t lba = fs->database + (cl - 2) * fs->csize, take = min((uint32_t)fs->csize, left);
    if (n && vh_runs[n - 1].lba + vh_runs[n - 1].sectors == lba) vh_runs[n - 1].sectors += take;
    else if (n < 8192) vh_runs[n++] = {lba, take};
    else ok = false;
    left -= take;
    const uint32_t sec = fs->fatbase + cl / 128;
    if (sec != cached && sdmmc_read_sectors(vh_card, fat, sec, 1) != ESP_OK) ok = false;
    cached = sec;
    cl = (fat[cl % 128 * 4] | fat[cl % 128 * 4 + 1] << 8 | fat[cl % 128 * 4 + 2] << 16 | (uint32_t)fat[cl % 128 * 4 + 3] << 24) & 0x0FFFFFFF;
  }
  f_close(&fil);
  vh_run_n = n;
  return ok && !left;
}

static bool vh_read(uint32_t at, uint32_t count, uint8_t *to) {
  uint32_t r = 0;
  while (r < vh_run_n && at >= vh_runs[r].sectors) at -= vh_runs[r++].sectors;
  while (count && r < vh_run_n) {
    const uint32_t n = min(count, vh_runs[r].sectors - at);
    if (sdmmc_read_sectors(vh_card, to, vh_runs[r].lba + at, n) != ESP_OK) return false;
    to += n * 512, count -= n, at = 0, r++;
  }
  return !count;
}

// The open clip's records, from the card or from the network's cache (video_net.h): n bytes at the file's byte at
static bool vh_is_net;
static bool vh_fetch(uint32_t at, uint32_t n, uint8_t *to) {
#if FN_WIFI
  if (vh_is_net) return vn_read(at, n, to);
#endif
  return vh_card_ok && vh_read(at / 512, (n + 511) / 512, to);
}

static uint32_t vh_samples(uint32_t i) {  // the sound of frame i: the running total decides, or it would drift
  const uint64_t a = (uint64_t)vh_c.a_rate * vh_c.den;
  return (uint32_t)((a * (i + 1)) / vh_c.num - (a * i) / vh_c.num);
}

// The header and the tables into PSRAM, a ring big enough for the largest frame; then frames from 0 on
static bool vh_load(int i) {
  rd_open = false;
  vh_is_net = false;
#if FN_WIFI
  vn_stop();
#endif
  uint32_t h[16];
  uint32_t *t = nullptr;
  if (vh_c.index) heap_caps_free(vh_c.index), vh_c.index = nullptr;
  if (vh_files[i].net) {  // (from a computer: the header and the tables by HTTP; its frames come through the cache)
#if FN_WIFI
    if (!vn_begin() || !vn_head(vh_files[i], h, &t)) return false;
    vh_is_net = true;
#else
    return false;
#endif
  } else {
    if (!vh_card_ok) return false;
    char path[96];
    snprintf(path, sizeof path, VH_DIR "/%s", vh_files[i].name);
    const int f = open(path, O_RDONLY);
    if (f < 0) return LOG("video: no file %s\n", path), false;
    if (read(f, h, 64) != 64 || memcmp(h, "P4V2", 4) || h[1]) return close(f), false;
    t = (uint32_t *)heap_caps_malloc(4 * (2 * h[6] + 1), MALLOC_CAP_SPIRAM);
    if (!t || read(f, t, 4 * (2 * h[6] + 1)) != (ssize_t)(4 * (2 * h[6] + 1))) return LOG("video: no table\n"), close(f), heap_caps_free(t), false;
    close(f);
    if (!vh_map(vh_files[i].name)) return LOG("video: cannot map the file\n"), heap_caps_free(t), false;
  }
  vh_c.num = h[4], vh_c.den = h[5], vh_c.count = h[6], vh_c.a_rate = h[8];
  vh_c.index = t, vh_c.jpeg = t + vh_c.count + 1;
  uint32_t most = 0;
  for (uint32_t k = 0; k < vh_c.count; k++) most = max(most, t[k + 1] - t[k]);
  const uint32_t slot = (most + 511) & ~511;
  if (slot > ring_slot) {
    if (ring_mem) heap_caps_free(ring_mem);
    ring_mem = (uint8_t *)heap_caps_aligned_alloc(128, slot * VH_RING, MALLOC_CAP_SPIRAM);
    ring_slot = ring_mem ? slot : 0;
    if (!ring_mem) return LOG("video: no memory for the ring\n"), false;
  }
  if (slot > vh_keep_cap) {
    heap_caps_free(vh_keep);
    vh_keep = (uint8_t *)heap_caps_aligned_alloc(128, slot, MALLOC_CAP_SPIRAM);
    vh_keep_cap = vh_keep ? slot : 0;
  }
  vh_keep_n = 0;
#if FN_WIFI
  if (vh_is_net) vn_go(vh_c.index[0]);
#endif
  rd_open = true;
  return true;
}

static void vh_reader(void *) {
  uint32_t still_try = 0, still_fails = 0;
  vh_card_ok = vh_mount();
  if (vh_card_ok) mkdir(VH_DIR, 0775);
  vh_scan();  // (no card — only a computer's clips, if one is set: video_net.h)
  for (;;) {
    const int cmd = rd_cmd;
    if (cmd != RD_NONE) {
      if (cmd == RD_SCAN) vh_scan();
      if (cmd == RD_OPEN) {
        vh_still_ok = false;
        if (vh_load(rd_arg)) {  // the first frame right away: it is the picture the tile grows into
          ring_head = ring_tail = 0, a_read = a_write = 0, a_phase = 0, rd_next = 0, rd_left = UINT32_MAX;
          rd_done = rd_gen;
          const uint32_t n = vh_c.index[1] - vh_c.index[0];
          if (vh_fetch(vh_c.index[0], n, ring_mem) && vh_decode(ring_mem, vh_c.jpeg[0], vh_still, VH_W * VH_H * 2)) {
            esp_cache_msync(vh_still, VH_W * VH_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
            vh_make_half();
            vh_still_ok = true;
          }
        }
      }
      if (cmd == RD_CLOSE) {
        rd_open = false;
#if FN_WIFI
        vn_stop();
#endif
      }
      if (cmd == RD_HALF) {  // (~30 ms of reading PSRAM: on this core, not in the frame task)
        esp_cache_msync(vh_still, VH_W * VH_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        vh_make_half();
        esp_cache_msync(vh_half, VH_W * VH_H / 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
        vh_half_job = 2;
      }
      rd_cmd = RD_NONE;
    }
    if (rd_done != rd_gen) {  // a new place in the clip: everything read ahead is forgotten
      const uint32_t g = rd_gen;
      __sync_synchronize();
      ring_head = ring_tail = 0, a_read = a_write = 0, a_phase = 0;
      rd_next = rd_frame, rd_left = rd_more ? UINT32_MAX : 1;
#if FN_WIFI
      if (vh_is_net && rd_open) vn_go(vh_c.index[rd_frame]);
#endif
      rd_done = g;
    }
    if (vh_rescan && rd_cmd == RD_NONE && !rd_open) vh_rescan = false, vh_scan();
    if (rd_open && !vh_still_ok && ring_head != ring_tail && millis() - still_try > 150) {
      // (the first frame's picture did not come out — the decoder had no internal memory for a moment while the network
      // was filling it: again, from the ring)
      still_try = millis();
      const uint32_t k = ring_tail % VH_RING;
      if (vh_decode(ring_mem + k * ring_slot, ring_size[k], vh_still, VH_W * VH_H * 2)) {
        esp_cache_msync(vh_still, VH_W * VH_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        vh_make_half();
        vh_still_ok = true;
      } else if (++still_fails % 20 == 1) LOG("video: the first picture failed, %lu times\n", (unsigned long)still_fails);
    }
    if (!rd_open || !rd_left || rd_next >= vh_c.count || ring_head - ring_tail >= VH_RING || (vh_c.a_rate && a_write - a_read > VH_AUDIO_N - 8192)) {
      delay(1);
      continue;
    }
    const uint32_t i = rd_next, k = ring_head % VH_RING, n = vh_c.index[i + 1] - vh_c.index[i];
    uint8_t *to = ring_mem + k * ring_slot;
    if (!vh_fetch(vh_c.index[i], n, to)) {
      if (rd_done != rd_gen || rd_cmd != RD_NONE) continue;  // (an order came while it waited for the network)
      LOG("video: read failed at frame %lu\n", (unsigned long)i);
      delay(100);
      continue;
    }
    if (rd_done != rd_gen || cmd != rd_cmd) continue;  // (an order came while it was reading: this frame may be stale)
    const uint32_t jpeg = vh_c.jpeg[i];
    if (vh_c.a_rate) {
      const int16_t *pcm = (const int16_t *)(to + ((jpeg + 511) & ~511));
      const uint32_t m = vh_samples(i);
      uint64_t w = a_write;
      for (uint32_t j = 0; j < m; j++) a_ring[(w + j) & (VH_AUDIO_N - 1)] = pcm[j];
      a_write = w + m;
    }
    rd_next = i + 1, rd_left--;
    ring_size[k] = jpeg, ring_frame[k] = i;
    __sync_synchronize();
    ring_head = ring_head + 1;
  }
}

static void vh_go(int frame, bool more) {
  a_on = false;
  if (vh_cur_ok) {  // (the reader empties the ring only after rd_gen moves: the slot is still whole)
    const int k = ring_tail % VH_RING;
    vh_keep_n = vh_keep && ring_size[k] <= vh_keep_cap ? ring_size[k] : 0;
    if (vh_keep_n) {  // (the decoder reads it by DMA: out of the processor's cache)
      memcpy(vh_keep, ring_mem + k * ring_slot, vh_keep_n);
      esp_cache_msync(vh_keep, (vh_keep_n + 127) & ~127, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    }
  }
  vh_cur_ok = false;
  rd_frame = frame, rd_more = more, vh_a_from = frame;
  __sync_synchronize();
  rd_gen = rd_gen + 1;
}

static void vh_begin() {
  static bool begun;
  if (begun) return;
  begun = true;
  vh_files = (VhFile *)heap_caps_calloc(VH_MAX, sizeof(VhFile), MALLOC_CAP_SPIRAM);
  vh_still = (uint16_t *)heap_caps_aligned_alloc(128, VH_W * VH_H * 2, MALLOC_CAP_SPIRAM);
  vh_half = (uint16_t *)heap_caps_aligned_alloc(128, VH_W * VH_H / 2, MALLOC_CAP_SPIRAM);
  a_ring = (int16_t *)heap_caps_malloc(VH_AUDIO_N * 2, MALLOC_CAP_SPIRAM);
  jpeg_decode_engine_cfg_t jc = {};
  jc.timeout_ms = 200;
  if (jpeg_new_decoder_engine(&jc, &vh_jpeg) != ESP_OK) LOG("video: no JPEG engine\n");
#if FN_WIFI  // (its stack in PSRAM: internal memory is a Wi-Fi stream's — video_net.h. It never calls Wi-Fi nor writes the flash)
  xTaskCreatePinnedToCoreWithCaps(vh_reader, "video_card", 8192, nullptr, 4, nullptr, 0, MALLOC_CAP_SPIRAM);
#else
  xTaskCreatePinnedToCore(vh_reader, "video_card", 8192, nullptr, 4, nullptr, 0);
#endif
}

static void vh_open(int i) {
  vh_begin();
  vh_end = false, vh_paused = false, vh_cur = -1, vh_cur_ok = false, vh_want = -1, vh_seeking = false, vh_seek_pending = -1;
  vh_scrub = false, vh_go_final = true, vh_a_from = 0;
  vh_freeze_req = vh_frozen = false, vh_half_job = 0;
  vh_open_ms = millis();
#if HAS_MUSIC || HAS_RECORDER
  snd_begin(22050);
#else
  snd_begin(32000);
#endif
  snd_video = vh_fill;
  rd_arg = i, rd_cmd = RD_OPEN;
}

// All frames read ahead (or as many as the clip has)
static bool vh_ready() {
  if (rd_cmd != RD_NONE || !rd_open) return false;
  if (rd_done != rd_gen) return false;
#if FN_WIFI
  if (vh_is_net) return (ring_head - ring_tail >= min((uint32_t)VH_RING, vh_c.count) && vn_ready()) || millis() - vh_open_ms > 15000;
#endif
  return ring_head - ring_tail >= min((uint32_t)VH_RING, vh_c.count) || millis() - vh_open_ms > 2500;
}

static void vh_clock_from(int frame) {
  vh_base_te = hd_te_count() + 1, vh_base_frame = frame, vh_err = 0;
}

static float vh_hz_for(float fps) {  // the refresh that shows every frame the same number of times
  for (int m : {2, 1, 3, 4})
    if (fps * m >= 47.5f && fps * m <= 60.5f) return fps * m;
  return 0;
}

// The tile has become the screen: from now on the picture is the decoder's. The sound and the clock start with the first frame
static void vh_start() {
  const float fps = (float)vh_c.num / vh_c.den;
  vh_hz = vh_hz_for(fps);
  vh_period_us = vh_hz > 0 ? 1e6f / vh_hz : 16557;
  vh_fpb = fps * vh_period_us / 1e6f;
  vh_rate_req = vh_hz;
  a_step = vh_c.a_rate ? (uint32_t)(((uint64_t)vh_c.a_rate << 16) / (snd_rate ? snd_rate : vh_c.a_rate)) : 65536;
  vh_cur = -1, vh_want = 0, vh_new = true;
  vh_stop_done = false, vh_stop_req = false;
  halo_under(nullptr);
  vh_hook = true;
  extern bool vh_under(uint16_t *, bool);
  halo_under(vh_under);
  vh_clock_from(0);
  a_on = vh_c.a_rate != 0;
  vs_at = millis() + 2000, vs_shown = vs_sum = vs_max = vs_shift = vs_wait = 0, vs_off = 0;
  LOG("video: %.2f fps, the screen at %.2f Hz, %lu frames\n", fps, vh_hz, (unsigned long)vh_c.count);
}

// Back to the tiles: the frame on screen becomes the picture that shrinks. The frame task does it between two frames
static int vh_stop_wait;
static void vh_stop() {
  a_on = false;
  vh_freeze_req = false, vh_stop_wait = 0;
  if (vh_frozen) {  // (the frame is in vh_still already, the picture let go)
    vh_make_half();
    vh_frozen = false, vh_stop_done = true;
    return;
  }
  vh_stop_done = false, vh_stop_req = true;
}
static bool vh_stopped() { return vh_stop_done; }
static void vh_close() { rd_cmd = RD_CLOSE, snd_video = nullptr, vh_freeze_req = vh_frozen = false; }
static void vh_abort() {  // the screen went away under the player
  a_on = false, vh_hook = false, vh_stop_req = false;
  hd_set_rate(0);
  vh_close();
}

// on — the player is not seen as it is: the next frame puts the frame on screen into vh_still, lets the picture go and
// goes back to 60 Hz (the way into Always On is an animation); vh_frozen then. off — the decoder's picture again, the
// same frame at the clip's refresh. Only while it plays (vh_start .. vh_stop) and is paused
static void vh_freeze(bool on) {
  if (on) {
    if (vh_hook && !vh_stop_req && !vh_freeze_req) vh_freeze_req = 1;
    return;
  }
  vh_freeze_req = 0;
  if (!vh_frozen) return;
  vh_frozen = false, vh_hook = true, vh_rate_req = vh_hz;
  halo_damage_all();
}

// The frame on screen, decoded again into vh_still (nothing decoded yet — the first frame from the open is there already)
static void vh_cur_still() {
  const int k = vh_cur_ok ? (int)(ring_tail % VH_RING) : -1;
  const uint8_t *src = k >= 0 ? ring_mem + k * ring_slot : vh_keep_n ? vh_keep : nullptr;
  if (src && vh_decode(src, k >= 0 ? ring_size[k] : vh_keep_n, vh_still, VH_W * VH_H * 2)) esp_cache_msync(vh_still, VH_W * VH_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
}

static uint32_t vu_calls, vu_true, vu_fail, vu_copy;
bool vh_under(uint16_t *back, bool changed) {
  vu_calls++;
  if (!vh_hook) return false;
  // vh_freeze, in two frames: the frame on screen into vh_still, and this frame is still a picture (the view has only
  // just left 1: not seen); in the next one the picture is let go — ordinary frames from then on, the player draws the
  // frame itself. (In one frame: the decode and the whole screen drawn from vh_still, 23 ms)
  if (vh_freeze_req == 2) {
    hd_set_rate(0);
    vh_hook = false, vh_frozen = true, vh_freeze_req = 0;
    halo_damage_all();
    return false;
  }
  if (vh_freeze_req && !vh_stop_req) {  // (the frame on screen stays: a newer one asked for this frame is not shown)
    vh_cur_still(), vh_freeze_req = 2, changed = true;
    if (vh_cur >= 0) vh_new = false, vh_want = vh_cur;
  }
  // The close: in a frame with nothing over the picture changing, the frame on screen goes into vh_still here (6 ms) and
  // its half size is made by the card's task on the other core (31 ms of reading PSRAM: here that frame came two
  // refreshes late and the close started with a jump). The picture stays ours meanwhile and the screen keeps it; made —
  // the picture is let go, and the close starts in the next frame
  if (vh_stop_req && vh_half_job == 2) {
    esp_cache_msync(vh_half, VH_W * VH_H / 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    hd_set_rate(0);
    vh_hook = false, vh_stop_req = false, vh_stop_done = true, vh_half_job = 0;
    return false;
  }
  if (vh_stop_req && !vh_half_job && !changed && rd_cmd == RD_NONE) {
    vh_cur_still();
    vh_half_job = 1, rd_cmd = RD_HALF;
    return false;
  }
  if (vh_rate_req >= 0) hd_set_rate(vh_rate_req), vh_rate_req = -1;
  uint16_t *pic = back + VH_TOP * HALO_W;
  // (the close waits for a frame with nothing over the picture changing — the time line moved in the last frame it
  // played — and only after three such frames takes the frame along with a whole new picture: two decodes in one frame)
  const bool stop = vh_stop_req && !vh_half_job && ++vh_stop_wait > 3;
  if (vh_new && vh_cur_ok && vh_want == vh_cur) vh_new = false;
  if (!vh_new && !changed && !stop) return false;
  // Which frame this picture is
  int slot = -1;
  if (vh_new && !vh_seeking) {
    for (uint32_t t = ring_tail; t != ring_head; t++)
      if ((int)ring_frame[t % VH_RING] == vh_want) slot = t % VH_RING, ring_tail = t;
    if (slot >= 0) vh_cur = vh_want, vh_cur_ok = true;
  }
  if (slot < 0 && vh_cur_ok) slot = ring_tail % VH_RING;
  const uint8_t *src = slot >= 0 ? ring_mem + slot * ring_slot : vh_keep_n ? vh_keep : nullptr;
  const uint32_t src_n = slot >= 0 ? ring_size[slot] : vh_keep_n;
  if (!src) {  // nothing decoded yet (the first frame is not in the ring): the picture on screen once more — nothing is over it yet
    if (!changed && !stop) return false;
    esp_cache_msync(hd_front(), HALO_W * HALO_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    memcpy(back, hd_front(), HALO_W * HALO_H * 2);
    vu_copy++;
    esp_cache_msync(back, HALO_W * HALO_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  } else {
    if (slot < 0 && !changed && !stop) return false;  // (the frame asked for is not here yet: nothing to show anew)
    const uint32_t t0 = trace_now();
    if (!vh_decode(src, src_n, pic, VH_W * VH_H * 2)) return vu_fail++, false;
    vu_true++;
    if (slot >= 0) {
      const uint32_t us = trace_now() - t0;
      vs_sum += us, vs_max = max(vs_max, us);
      if (vh_new) vs_shown++, vh_new = false;
    }
  }
  if (stop) {
    if (src && vh_decode(src, src_n, vh_still, VH_W * VH_H * 2)) esp_cache_msync(vh_still, VH_W * VH_H * 2, ESP_CACHE_MSYNC_FLAG_DIR_M2C), vh_make_half();
    hd_set_rate(0);
    vh_hook = false, vh_stop_req = false, vh_stop_done = true;
    halo_damage_all();
  } else if (vh_on_picture) {
    vh_on_picture();
  }
  return true;
}

static bool vh_playing() { return vh_hook && !vh_paused && !vh_end && !vh_seeking; }
static bool vh_ended() { return vh_end; }
static float vh_length() { return vh_c.count && vh_c.num ? (float)vh_c.count * vh_c.den / vh_c.num : 0; }
static float vh_position() { return vh_want >= 0 && vh_c.num ? (float)vh_want * vh_c.den / vh_c.num : 0; }  // (the frame about to be shown: time and picture change together)

static void vh_set_playing(bool on) {
  if (!vh_hook) return;
  if (on && vh_end) {  // from the beginning (the frame asked for — 0: it stayed the last one, and the clip was at its end at once)
    vh_end = false, vh_paused = false, vh_seeking = true, vh_go_final = true, vh_want = 0;
    vh_go(0, true);
    return;
  }
  if (on == !vh_paused) return;
  vh_paused = !on;
  if (on) vh_clock_from(vh_cur + 1);
  a_on = on && vh_c.a_rate;
}

// A finger on the time line: while it is there — single frames, the newest place once the last one is on screen, and the
// clip does not run; let go — from there on
static void vh_seek(float frac, bool final) {
  if (!vh_hook || !vh_c.count) return;
  vh_seek_pending = frac, vh_seek_final = final, vh_scrub = !final;
}

static void vh_seek_run() {
  if (vh_seek_pending < 0 || rd_done != rd_gen) return;
  // (the frame asked for last has not come yet, or has and is not on screen yet: the next place would empty the ring
  // under it — so the picture never changed while the finger moved)
  if (vh_seeking && !vh_go_final && ring_head == ring_tail) return;
  if (!vh_go_final && vh_new) return;
  const int n = constrain((int)(vh_seek_pending * vh_c.count), 0, (int)vh_c.count - 1);
  const bool final = vh_seek_final;
  vh_seek_pending = -1;
  if (!final && n == vh_want) return;  // (the same place)
  vh_end = false;
  vh_seeking = true, vh_want = n, vh_go_final = final;
  vh_go(n, final);
}

static void vh_volume(float v) {
  if (v <= 0.001f) return halo_audio_volume(0);
  const float db = -40 + v * 46, reg = 0xBF + 2 * db;
  halo_audio_volume((int)lroundf((reg + 1) * 100 / 256));
}

// Every frame: which frame the next boundary shows. The picture keeps to the screen's boundaries, so the cadence is even
// (every second one at 50 Hz); the sound is the clock, and only when they part by more than a boundary the picture steps
static void vh_tick(float) {
  if (!vh_hook || vh_stop_req) return;
  if (vs_at && vh_stat_on && (int32_t)(millis() - vs_at) >= 0) {
    if (vs_shown) LOG("video: %.1f fps, decode %.2f ms (max %.2f), picture against sound %.1f ms, steps %lu, waits %lu, ring %lu, frame %d\n", vs_shown / 2.0f, vs_sum / 1000.0f / vs_shown,
                      vs_max / 1000.0f, vs_off / 1000 / vs_shown, (unsigned long)vs_shift, (unsigned long)vs_wait, (unsigned long)(ring_head - ring_tail), vh_cur);
    vs_at = millis() + 2000, vs_shown = vs_sum = vs_max = vs_shift = vs_wait = 0, vs_off = 0;
  }
  // (a finger's frame that has come is shown first: vh_seek_run waits for it)
  if (rd_done == rd_gen && vh_seeking && !vh_go_final && ring_head != ring_tail) vh_seeking = false, vh_new = true;
  vh_seek_run();
  if (rd_done != rd_gen) return;
  if (vh_seeking) {  // the frames of the new place are coming
    if (vh_go_final) {
      const uint32_t need = min((uint32_t)VH_START_FRAMES, vh_c.count - vh_want);
      if (ring_head - ring_tail < need) return void(vs_wait++);
#if FN_WIFI
      if (vh_is_net && !vn_ready()) return void(vs_wait++);  // (from the network: VN_START ahead, as at the start)
#endif
      vh_seeking = false, vh_cur_ok = false, vh_new = true;
      if (!vh_paused) {
        a_step = vh_c.a_rate ? (uint32_t)(((uint64_t)vh_c.a_rate << 16) / (snd_rate ? snd_rate : vh_c.a_rate)) : 65536;
        a_on = vh_c.a_rate != 0;
        vh_clock_from(vh_want);
      }
    }
    return;
  }
  if (vh_paused || vh_end || vh_scrub) return;
#if FN_WIFI
  // The network did not keep up (the next frame is not here): the picture and the sound stop, and go on together once
  // the ring is full and VN_START is ahead again — a wait, not frames skipped and the sound running off
  static bool stall;
  if (!vh_is_net) stall = false;
  else {
    const uint32_t in = ring_head - ring_tail;
    if (!stall && in <= 1 && rd_next < vh_c.count) stall = true, a_on = false, vn_stalls++, LOG("video: waiting for the network\n");
    if (stall) {
      if ((in < VH_RING && rd_next < vh_c.count) || !vn_ready()) return;
      stall = false, vh_clock_from(vh_cur + 1), a_on = vh_c.a_rate != 0;
      LOG("video: the network caught up\n");
    }
  }
#endif
  const uint32_t te = hd_te_count();
  const float pos = (int32_t)(te + 1 - vh_base_te) * vh_fpb;  // (signed: a clock moved ahead of the boundary is not 4e9 frames on)
  const int want = vh_base_frame + (int)(pos + 0.001f);
  if (want >= (int)vh_c.count) {
    vh_end = true, a_on = false;
    return;
  }
  if (vh_c.a_rate) {  // the sound against the picture, at the moment this frame is seen
    const uint64_t r = a_read;
    if (r != vh_a_seen) vh_a_seen = r, vh_a_ms = millis();
    // (the ring's sound starts where the last go took it: after a seek the count began at 0 against a picture seconds on,
    // the clock was stepped by all of it and the clip was at its end at once)
    const double heard = r > VH_AUDIO_QUEUE ? (double)vh_a_from * vh_c.den * 1e6 / vh_c.num + (r - VH_AUDIO_QUEUE) * 1e6 / vh_c.a_rate : 0;
    const double shown = (vh_base_frame + pos) * vh_c.den * 1e6 / vh_c.num - vh_period_us;
    const double err = heard - shown;
    vs_off += err;
    vh_err += (err - vh_err) * 0.05;
    // (a speaker that has not started, or has stalled, is no clock: the picture goes on by itself)
    if (r > VH_AUDIO_QUEUE && millis() - vh_a_ms < 300 && fabs(vh_err) > 1.5 * vh_period_us) {
      const int step = (int)lround(vh_err / vh_period_us);
      vh_base_te -= step, vh_err = 0, vs_shift++;
    }
  }
  if (want > vh_cur) vh_want = want, vh_new = true;  // (never back: those frames are gone; ahead of the sound it simply waits)
}

// ---------- What PopkaStudio asks of the card (over USB) ----------
static bool vh_wait_scan() {
  vh_begin();
  for (int i = 0; i < 2000 && (!vh_files || vh_scanning || !vh_jpeg || (!vh_card_ok && i < 300)); i++) delay(5);
  delay(50);
  for (int i = 0; i < 1500 && vh_scanning; i++) delay(5);
  return vh_card_ok;
}

static void vh_put(const char *a) {
  char name[64];
  unsigned long size = 0;
  if (sscanf(a, "%63s %lu", name, &size) != 2 || !vh_wait_scan()) return (void)Serial.println("PUT FAIL no card");
  if (rd_open) return (void)Serial.println("PUT FAIL busy");
  char path[96];
  snprintf(path, sizeof path, VH_DIR "/%s", name);
  mkdir(VH_DIR, 0775);
  FILE *f = fopen(path, "wb");
  static uint8_t *buf = (uint8_t *)heap_caps_aligned_alloc(128, VH_PUT_CHUNK, MALLOC_CAP_SPIRAM);
  if (!f || !buf) return (void)Serial.println("PUT FAIL open");
  Serial.setTimeout(4000);
  // The USB port's receive queue must hold a whole chunk, or what does not fit is lost. 16 KB: with Wi-Fi the largest
  // free block of internal memory is ~30 KB (a 32 KB queue was refused and the transfer timed out)
  if (!Serial.setRxBufferSize(2 * VH_PUT_CHUNK)) return fclose(f), unlink(path), (void)Serial.println("PUT FAIL memory");
  Serial.println("PUT READY");
  uint32_t crc = 0, t0 = millis();
  for (unsigned long left = size; left;) {
    const size_t n = min(left, (unsigned long)VH_PUT_CHUNK);
    if (Serial.readBytes(buf, n) != n) {
      fclose(f), unlink(path);
      return (void)Serial.println("PUT FAIL timeout");
    }
    crc = esp_rom_crc32_le(crc, buf, n);
    if (fwrite(buf, 1, n, f) != n) {
      fclose(f), unlink(path);
      return (void)Serial.println("PUT FAIL write");
    }
    left -= n;
    Serial.println("OK");
  }
  fclose(f);
  Serial.setRxBufferSize(2048);
  Serial.printf("PUT DONE %08lx %.2f MB/s\n", (unsigned long)crc, size / 1048576.0 / ((millis() - t0) / 1000.0));
  rd_cmd = RD_SCAN;
}

static void vh_commands() {
#if FN_WIFI
  vn_commands();
#endif
  halo_command("vlist", [](const char *) {
    if (!vh_wait_scan()) return (void)Serial.println("VLIST nocard");
    uint64_t all = 0, free_b = 0;
    esp_vfs_fat_info("/sdcard", &all, &free_b);
    Serial.printf("VLIST %d %u %u\n", (int)vh_n, (unsigned)(all >> 20), (unsigned)(free_b >> 20));
    for (int i = 0; i < vh_n; i++) {
      char path[96];
      struct stat st = {};
      snprintf(path, sizeof path, VH_DIR "/%s", vh_files[i].name);
      stat(path, &st);
      Serial.printf("V %lu %lu %s\n", (unsigned long)st.st_size, (unsigned long)vh_files[i].seconds, vh_files[i].name);
    }
    Serial.println("VEND");
  });
  halo_command("vput", vh_put);
  halo_command("vdel", [](const char *a) {
    char path[96];
    snprintf(path, sizeof path, VH_DIR "/%s", a);
    if (!vh_wait_scan() || rd_open || strchr(a, '/') || unlink(path)) return (void)Serial.println("VDEL FAIL");
    Serial.println("VDEL OK");
    rd_cmd = RD_SCAN;
  });
  halo_command("vscan", [](const char *) { vh_begin(), rd_cmd = RD_SCAN; });
  halo_command("vdbg", [](const char *) {
    LOG("video: hook %d cur %d ok %d want %d new %d paused %d end %d seeking %d ring %lu..%lu calls %lu true %lu fail %lu copy %lu\n", vh_hook, vh_cur, vh_cur_ok, vh_want, vh_new, vh_paused, vh_end, vh_seeking,
        (unsigned long)ring_tail, (unsigned long)ring_head, (unsigned long)vu_calls, (unsigned long)vu_true, (unsigned long)vu_fail, (unsigned long)vu_copy);
  });
  halo_command("vstat", [](const char *) { vh_stat_on = !vh_stat_on, LOG("video: stats %s\n", vh_stat_on ? "on" : "off"); });
}
#endif  // __EMSCRIPTEN__
