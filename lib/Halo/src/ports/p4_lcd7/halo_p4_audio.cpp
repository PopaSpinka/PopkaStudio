// The P4 board's sound: a speaker through the ES8311 codec and the NS4150B amplifier (GPIO53, high is on), two microphones
// through the ES7210 ADC. Both codecs sit on the I2C bus shared with the touch chip (0x18, 0x40; the bus driver takes turns).
// The chips are the round board's, so the register sequences are the ones in ports/s3_amoled175/halo_audio.cpp; only the
// pins, the bus and the task's stack differ, and this file stays this board's own (docs/multi-board.md).
//
// I2S: the ESP32 drives the bus — MCLK = 256 fs on GPIO13, BCLK 12, LRCK 10, data out 9, data in 11; 16 bits, stereo. Receive
// is the pair of transmit on one port (full duplex): it is set up the first time a microphone is needed and never deleted,
// because after that MCLK comes from its divider. Silence longer than 150 ms switches the amplifier off.
// The microphones: the audio task pulls what came into a ring in PSRAM; the receive channel delivers both slots of a frame,
// MIC1 and MIC2 in turn, and their average is what is kept.
#include <sdkconfig.h>
#if CONFIG_IDF_TARGET_ESP32P4
#include "halo_p4.h"

#include <driver/gpio.h>
#include <driver/i2s_std.h>
#include <esp_heap_caps.h>

#define PIN_MCLK GPIO_NUM_13
#define PIN_BCLK GPIO_NUM_12
#define PIN_LRCK GPIO_NUM_10
#define PIN_DOUT GPIO_NUM_9
#define PIN_DIN GPIO_NUM_11
#define PIN_PA GPIO_NUM_53
#define FRAMES 256
#define MIC_RING 16384  // samples (32 KB of PSRAM): about a second at 16 kHz; a power of two
#define MIC_GAIN 10     // the ES7210's gain in 3 dB steps: 30 dB

static i2s_chan_handle_t tx, rx;
static HaloI2c es, adc;
static HaloAudioFill fill_cb;
static int rate;
static volatile int volume = 60;
static volatile bool vol_dirty;
static volatile bool rx_ready, mic_want;
static bool mic_run, mic_half;  // (the audio task's own)
static int16_t mic_carry;
static int16_t *ring;
static volatile uint32_t ring_head;
static uint32_t ring_tail, mic_lost;
static volatile float mic_level = -90, mic_level1 = -90, mic_level2 = -90;

static void es_w(uint8_t r, uint8_t v) {
  halo_i2c_write(es, r, v);
}

static uint8_t es_r(uint8_t r) {
  uint8_t v = 0;
  halo_i2c_read(es, r, &v, 1);
  return v;
}

static void adc_w(uint8_t r, uint8_t v) {
  halo_i2c_write(adc, r, v);
}

static uint8_t adc_r(uint8_t r) {
  uint8_t v = 0;
  halo_i2c_read(adc, r, &v, 1);
  return v;
}

static uint8_t vol_reg(int v) {  // 0..100 -> register 0x32: 0 is silence, 100 is +32 dB
  return v <= 0 ? 0 : (uint8_t)(v * 256 / 100 - 1);
}

static float db_of(float sum, int n) {  // 90.309 = 20 lg 32768
  return sum > 0 && n ? fmaxf(-90, 10 * log10f(sum / n) - 90.309f) : -90;
}

static void mic_pull() {
  for (;;) {
    const uint32_t h = ring_head, at = h & (MIC_RING - 1);
    const int room = min(FRAMES, (int)(MIC_RING - at));
    size_t got = 0;
    i2s_channel_read(rx, ring + at, room * 2, &got, 0);
    const int raw = got / 2;
    if (raw <= 0) return;
    int n = 0, i = 0;
    float s1 = 0, s2 = 0;
    auto pair = [&](int l, int r) {  // (in place: n never passes i)
      ring[at + n++] = (int16_t)((l + r) >> 1);
      s1 += (float)l * l, s2 += (float)r * r;
    };
    if (mic_half) mic_half = false, pair(mic_carry, ring[at]), i = 1;
    for (; i + 1 < raw; i += 2) pair(ring[at + i], ring[at + i + 1]);
    if (i < raw) mic_carry = ring[at + i], mic_half = true;
    float s = 0;
    for (int k = 0; k < n; k++) s += (float)ring[at + k] * ring[at + k];
    mic_level = db_of(s, n), mic_level1 = db_of(s1, n), mic_level2 = db_of(s2, n);
    __atomic_store_n(&ring_head, h + n, __ATOMIC_RELEASE);
    if (raw < room) return;
  }
}

static void audio_task(void *) {
  static int16_t mono[FRAMES], st[FRAMES * 2];
  bool pa = false;
  int silent = 1 << 30;
  for (;;) {
    if (vol_dirty) vol_dirty = false, es_w(0x32, vol_reg(volume));
    if (rx_ready && mic_want != mic_run) {
      mic_run = mic_want;
      if (mic_run) mic_half = false, i2s_channel_enable(rx);
      else i2s_channel_disable(rx);
    }
    if (mic_run) mic_pull();
    const int n = fill_cb ? fill_cb(mono, FRAMES) : 0;
    if (n <= 0) {
      silent += FRAMES;
      if (pa && silent > rate * 15 / 100) gpio_set_level(PIN_PA, 0), pa = false;
      if (!pa) {  // quiet: the DMA keeps sending zeros by itself
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }
    } else {
      silent = 0;
      if (!pa) gpio_set_level(PIN_PA, 1), pa = true;
    }
    for (int i = 0; i < FRAMES; i++) st[2 * i] = st[2 * i + 1] = i < n ? mono[i] : 0;
    size_t w = 0;
    i2s_channel_write(tx, st, sizeof(st), &w, pdMS_TO_TICKS(200));
  }
}

static bool rx_init(i2s_chan_handle_t r) {
  i2s_std_config_t sc = {};
  sc.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)rate);
  sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  sc.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
  sc.gpio_cfg.mclk = I2S_GPIO_UNUSED, sc.gpio_cfg.bclk = PIN_BCLK;
  sc.gpio_cfg.ws = PIN_LRCK, sc.gpio_cfg.dout = I2S_GPIO_UNUSED, sc.gpio_cfg.din = PIN_DIN;
  if (i2s_channel_init_std_mode(r, &sc) != ESP_OK) return LOG("mic: I2S RX FAILED\n"), false;
  rx_ready = true;
  return true;
}

static bool audio_start(HaloAudioFill fill, int sample_rate, bool with_rx) {
  if (tx) {  // already running (a microphone may have started it without a fill): the rate stays
    if (!fill_cb) fill_cb = fill;
    if (sample_rate != rate) LOG("audio: already running at %d Hz, not %d\n", rate, sample_rate);
    return true;
  }
  es = halo_i2c(0x18);
  if (!es) return LOG("audio: ES8311 NOT FOUND\n"), false;
  // (the whole setup, not just the direction: after reset this pad's mux is not on the GPIO function)
  const gpio_config_t pa_cfg = {.pin_bit_mask = 1ULL << PIN_PA, .mode = GPIO_MODE_OUTPUT};
  gpio_config(&pa_cfg);
  gpio_set_level(PIN_PA, 0);
  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  cc.dma_desc_num = 6, cc.dma_frame_num = FRAMES, cc.auto_clear = true;
  i2s_chan_handle_t t = nullptr, r = nullptr;
  if (i2s_new_channel(&cc, &t, &r) != ESP_OK) return LOG("audio: I2S FAILED\n"), false;
  rate = sample_rate, fill_cb = fill;
  i2s_std_config_t sc = {};
  sc.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)rate);
  sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  sc.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  sc.gpio_cfg.mclk = PIN_MCLK, sc.gpio_cfg.bclk = PIN_BCLK, sc.gpio_cfg.ws = PIN_LRCK;
  sc.gpio_cfg.dout = PIN_DOUT, sc.gpio_cfg.din = I2S_GPIO_UNUSED;
  i2s_channel_init_std_mode(t, &sc);
  if (with_rx) rx_init(r);
  i2s_channel_enable(t);
  // ES8311 (registers: Espressif's es8311.c, Apache-2.0): reset, MCLK from its pin, all clocks; dividers for MCLK = 256 fs
  es_w(0x00, 0x1F), es_w(0x00, 0x00), es_w(0x00, 0x80);
  es_w(0x01, 0x3F);
  es_w(0x06, es_r(0x06) & ~0x20);
  es_w(0x02, es_r(0x02) & 0x07);
  es_w(0x03, 0x10), es_w(0x04, 0x10), es_w(0x05, 0x00);
  es_w(0x06, (es_r(0x06) & 0xE0) | 0x03);
  es_w(0x07, es_r(0x07) & 0xC0), es_w(0x08, 0xFF);
  es_w(0x00, es_r(0x00) & 0xBF);                                            // slave
  es_w(0x09, 0x0C), es_w(0x0A, 0x0C);                                       // 16 bits in and out
  es_w(0x0D, 0x01), es_w(0x0E, 0x02), es_w(0x12, 0x00), es_w(0x13, 0x10);  // power: analog, DAC, output
  es_w(0x1C, 0x6A), es_w(0x37, 0x08);
  es_w(0x32, vol_reg(volume));
  rx = r, tx = t;
  xTaskCreatePinnedToCore(audio_task, "halo_audio", 4096, nullptr, 5, nullptr, 0);
  LOG("audio: ES8311 ok, %d Hz\n", rate);
  return true;
}

bool halo_audio_begin(HaloAudioFill fill, int sample_rate) {
  return audio_start(fill, sample_rate, false);
}

void halo_audio_volume(int v) {
  volume = constrain(v, 0, 100), vol_dirty = true;
}

int halo_audio_get_volume() {
  return volume;
}

static void mic_codec_on() {  // ES7210, slave, MCLK = 256 fs, 16-bit I2S, ADC 1-2 on SDOUT1
  adc_w(0x00, 0xFF), adc_w(0x00, 0x41);
  adc_w(0x01, 0x1F);
  adc_w(0x09, 0x30), adc_w(0x0A, 0x30);
  adc_w(0x40, 0xC3), adc_w(0x41, 0x70), adc_w(0x42, 0x70);
  adc_w(0x07, 0x20), adc_w(0x02, 0xC1), adc_w(0x04, 0x01), adc_w(0x05, 0x00);
  adc_w(0x4B, 0x00), adc_w(0x4C, 0x00), adc_w(0x01, 0x00);
  for (int i = 0; i < 4; i++) adc_w(0x43 + i, (adc_r(0x43 + i) & 0xE0) | 0x10 | MIC_GAIN);
  adc_w(0x11, (adc_r(0x11) & 0x1C) | 0x60), adc_w(0x12, 0x00);
  adc_w(0x06, 0x00);
  for (int i = 0; i < 4; i++) adc_w(0x47 + i, 0x00);
}

static void mic_codec_off() {
  for (int i = 0; i < 4; i++) adc_w(0x47 + i, 0xFF);
  adc_w(0x4B, 0xFF), adc_w(0x4C, 0xFF), adc_w(0x01, 0x7F), adc_w(0x06, 0x07);
}

int halo_mic_begin(int sample_rate) {
  if (mic_want) return rate;
  if (!adc) adc = halo_i2c(0x40);
  if (!adc) return LOG("mic: ES7210 NOT FOUND\n"), 0;
  if (!ring) ring = (int16_t *)heap_caps_malloc(MIC_RING * 2, MALLOC_CAP_SPIRAM);
  if (!ring) return LOG("mic: no PSRAM for the ring\n"), 0;
  if (!tx && !audio_start(nullptr, sample_rate, true)) return 0;
  if (!rx_ready && !rx_init(rx)) return 0;
  mic_codec_on();
  ring_tail = __atomic_load_n(&ring_head, __ATOMIC_ACQUIRE), mic_lost = 0;
  mic_level = mic_level1 = mic_level2 = -90;
  mic_want = true;
  LOG("mic: ES7210 ok, %d Hz, gain %d dB\n", rate, MIC_GAIN * 3);
  return rate;
}

int halo_mic_read(int16_t *mono, int n) {
  if (!mic_want || n <= 0) return 0;
  const uint32_t h = __atomic_load_n(&ring_head, __ATOMIC_ACQUIRE);
  if (h - ring_tail > MIC_RING - MIC_RING / 4) {  // nearly a lap behind: the oldest is being overwritten
    const uint32_t t = h - (MIC_RING - MIC_RING / 4);
    mic_lost += t - ring_tail, ring_tail = t;
  }
  const int k = (int)min((uint32_t)n, h - ring_tail);
  for (int i = 0; i < k; i++) mono[i] = ring[(ring_tail + i) & (MIC_RING - 1)];
  ring_tail += k;
  return k;
}

float halo_mic_db() {
  return mic_want ? mic_level : -90;
}

void halo_mic_db2(float &mic1, float &mic2) {
  mic1 = mic_want ? mic_level1 : -90, mic2 = mic_want ? mic_level2 : -90;
}

void halo_mic_end() {
  if (!mic_want) return;
  mic_want = false;
  mic_codec_off();
  if (mic_lost) LOG("mic: %u samples skipped (read too late)\n", (unsigned)mic_lost);
  LOG("mic: off\n");
}

#endif  // CONFIG_IDF_TARGET_ESP32P4
