// Sound for Halo on the ESP32-S3-Touch-LCD-4B: the round watch's driver (ports/s3_amoled175/halo_audio.cpp — the long
// story is there), since the chips are the same: the speaker through the ES8311 codec (I2C 0x18), two microphones through
// the ES7210 (0x40), one I2S bus with the ESP32 as master. What differs here: the pins (MCLK 5, BCLK 16, LRCK 7, DOUT 6,
// DIN 15), the speaker's amplifier is pin 3 of the TCA9554 expander (an I2C write, not a GPIO), and the chips sit on the
// board's one I2C bus (halo_lcd4b_display.cpp starts it).
#include <sdkconfig.h>
#if defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_lcd4b.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/i2s_std.h>
#include <esp_heap_caps.h>

#define FRAMES 256
#define MIC_RING 16384  // samples (32 KB of PSRAM): ~1 s at 16 000 Hz; a power of two
#define MIC_GAIN 10     // ES7210 gain in 3 dB steps: 10 — 30 dB (as AmoledAudio; silence ≈ −61 dB)
static i2s_chan_handle_t tx, rx;  // tx ≠ nullptr — the bus is fully up
static i2c_master_dev_handle_t es, adc;
static HaloAudioFill fill_cb;
static int rate;
static volatile int volume = 60;
static volatile bool vol_dirty;
// Microphones: the sound task writes the ring (ring_head), halo_mic_read reads it (ring_tail)
static volatile bool rx_ready;  // the receive channel is set up (its DMA allocated)
static volatile bool mic_want;  // halo_mic_begin / halo_mic_end
static bool mic_run;            // the sound task: receiving is on (only it switches it, and it is the reader)
static int16_t *ring;
static volatile uint32_t ring_head;
static uint32_t ring_tail, mic_lost;
static volatile float mic_level = -90, mic_level1 = -90, mic_level2 = -90;  // both together, MIC1, MIC2

static void es_w(uint8_t r, uint8_t v) {
  const uint8_t b[2] = {r, v};
  i2c_master_transmit(es, b, 2, 20);
}

static uint8_t es_r(uint8_t r) {
  uint8_t v = 0;
  i2c_master_transmit_receive(es, &r, 1, &v, 1, 20);
  return v;
}

static void adc_w(uint8_t r, uint8_t v) {
  const uint8_t b[2] = {r, v};
  i2c_master_transmit(adc, b, 2, 20);
}

static uint8_t adc_r(uint8_t r) {
  uint8_t v = 0;
  i2c_master_transmit_receive(adc, &r, 1, &v, 1, 20);
  return v;
}

static uint8_t vol_reg(int v) {  // 0..100 → register 0x32 (as es8311_voice_volume_set: 0 — silence, 100 — +32 dB)
  return v <= 0 ? 0 : (uint8_t)(v * 256 / 100 - 1);
}

// Whatever came from the microphones goes into the ring (straight from DMA, no waiting); the level — by the last piece.
// Though set to mono, receiving brings both slots of an LRCK frame — left, right, left… (the round watch, 2026-11-05: in a
// channel pair the receive slots follow the transmit ones, stereo). Those are the two microphones: the ES7210 puts ADC 1
// (MIC1) in SDOUT1's left slot, ADC 2 (MIC2) in the right one. Their mean is taken: the voice is the same in both, each
// one's own noise is not — together ~3 dB quieter. A pair can be split between reads: the left sample waits in mic_carry
static bool mic_half;  // the pair's left sample is read and waits in mic_carry
static int16_t mic_carry;
static void mic_pull() {
  for (;;) {
    const uint32_t h = ring_head, at = h & (MIC_RING - 1);
    const int room = min(FRAMES, (int)(MIC_RING - at));  // up to the ring's end
    size_t got = 0;
    i2s_channel_read(rx, ring + at, room * 2, &got, 0);
    const int raw = got / 2;
    if (raw <= 0) return;
    int n = 0, i = 0;
    float s1 = 0, s2 = 0;
    auto pair = [&](int l, int r) {  // (in place: writes go to n ≤ i, already read)
      ring[at + n++] = (int16_t)((l + r) >> 1);
      s1 += (float)l * l, s2 += (float)r * r;
    };
    if (mic_half) mic_half = false, pair(mic_carry, ring[at]), i = 1;
    for (; i + 1 < raw; i += 2) pair(ring[at + i], ring[at + i + 1]);
    if (i < raw) mic_carry = ring[at + i], mic_half = true;
    float s = 0;
    for (int k = 0; k < n; k++) {
      const float v = ring[at + k];
      s += v * v;
    }
    mic_level = s > 0 && n ? fmaxf(-90, 10 * log10f(s / n) - 90.309f) : -90;  // 90,309 = 20·lg 32768
    mic_level1 = s1 > 0 && n ? fmaxf(-90, 10 * log10f(s1 / n) - 90.309f) : -90;
    mic_level2 = s2 > 0 && n ? fmaxf(-90, 10 * log10f(s2 / n) - 90.309f) : -90;
    __atomic_store_n(&ring_head, h + n, __ATOMIC_RELEASE);
    if (raw < room) return;
  }
}

static void audio_task(void *) {
  static int16_t mono[FRAMES], st[FRAMES * 2];
  bool pa = false;
  int silent = 1 << 30;  // samples of silence in a row
  for (;;) {
    if (vol_dirty) vol_dirty = false, es_w(0x32, vol_reg(volume));
    if (rx_ready && mic_want != mic_run) {
      mic_run = mic_want;
      if (mic_run) mic_half = false, i2s_channel_enable(rx);  // (the first sample — the left slot)
      else i2s_channel_disable(rx);
    }
    if (mic_run) mic_pull();
    const int n = fill_cb ? fill_cb(mono, FRAMES) : 0;
    if (n <= 0) {
      silent += FRAMES;
      if (pa && silent > rate * 15 / 100) hx_set(HX_AMP, false), pa = false;
      if (!pa) {  // quiet: DMA sends zeros by itself (auto_clear), the task sleeps
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }
    } else {
      silent = 0;
      if (!pa) hx_set(HX_AMP, true), pa = true;
    }
    for (int i = 0; i < FRAMES; i++) st[2 * i] = st[2 * i + 1] = i < n ? mono[i] : 0;
    size_t w = 0;
    i2s_channel_write(tx, st, sizeof(st), &w, pdMS_TO_TICKS(200));
  }
}

// The pair's receive channel: mono (left slot — MIC1), BCLK/LRCK — inputs on the same pins (the driver makes it a slave to
// the transmit clocks), the MCLK pin untouched. From now on MCLK comes from the receive divider
static bool rx_init(i2s_chan_handle_t r) {
  i2s_std_config_t sc = {};
  sc.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)rate);
  sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  sc.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
  sc.gpio_cfg.mclk = I2S_GPIO_UNUSED, sc.gpio_cfg.bclk = (gpio_num_t)HALO_PIN_BCLK;
  sc.gpio_cfg.ws = (gpio_num_t)HALO_PIN_LRCK, sc.gpio_cfg.dout = I2S_GPIO_UNUSED;
  sc.gpio_cfg.din = (gpio_num_t)HALO_PIN_DIN;
  if (i2s_channel_init_std_mode(r, &sc) != ESP_OK) return LOG("mic: I2S RX FAILED\n"), false;
  rx_ready = true;
  return true;
}

// with_rx — the microphone starts the bus: receiving right after transmitting, before the codecs are set up (as
// AmoledAudio), so that MCLK changes its source while the codecs are not set up yet
static bool audio_start(HaloAudioFill fill, int sample_rate, bool with_rx) {
  if (tx) {  // the bus is up already (the microphone may have started it, with no fill): the rate stays
    if (!fill_cb) fill_cb = fill;
    if (sample_rate != rate) LOG("audio: already running at %d Hz, not %d\n", rate, sample_rate);
    return true;
  }
  es = hx_device(0x18);
  if (!es) {
    LOG("audio: ES8311 NOT FOUND\n");
    return false;
  }
  hx_set(HX_AMP, false);
  // I2S first: the codec needs MCLK to take its settings. The channel pair holds the port: a second start at the same
  // time from another task is refused here instead of spoiling the first
  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  cc.dma_desc_num = 6, cc.dma_frame_num = FRAMES, cc.auto_clear = true;
  i2s_chan_handle_t t = nullptr, r = nullptr;
  if (i2s_new_channel(&cc, &t, &r) != ESP_OK) return LOG("audio: I2S FAILED\n"), false;
  rate = sample_rate, fill_cb = fill;
  i2s_std_config_t sc = {};
  sc.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)rate);
  sc.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  sc.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  sc.gpio_cfg.mclk = (gpio_num_t)HALO_PIN_MCLK, sc.gpio_cfg.bclk = (gpio_num_t)HALO_PIN_BCLK;
  sc.gpio_cfg.ws = (gpio_num_t)HALO_PIN_LRCK, sc.gpio_cfg.dout = (gpio_num_t)HALO_PIN_DOUT;
  sc.gpio_cfg.din = I2S_GPIO_UNUSED;
  i2s_channel_init_std_mode(t, &sc);
  if (with_rx) rx_init(r);
  i2s_channel_enable(t);
  // ES8311: reset and power on, MCLK from the pin, all clocks; dividers for MCLK = 256·fs (pre_div 1, ×1, adc/dac div 1,
  // single speed, LRCK = MCLK/256, BCLK = MCLK/4, OSR 0x10)
  es_w(0x00, 0x1F), es_w(0x00, 0x00), es_w(0x00, 0x80);
  es_w(0x01, 0x3F);
  es_w(0x06, es_r(0x06) & ~0x20);                // BCLK not inverted
  es_w(0x02, es_r(0x02) & 0x07);                 // pre_div 1, pre_multi ×1
  es_w(0x03, 0x10), es_w(0x04, 0x10), es_w(0x05, 0x00);
  es_w(0x06, (es_r(0x06) & 0xE0) | 0x03);        // BCLK = MCLK / 4
  es_w(0x07, es_r(0x07) & 0xC0), es_w(0x08, 0xFF);  // LRCK = MCLK / 256
  es_w(0x00, es_r(0x00) & 0xBF);                 // slave
  es_w(0x09, 0x0C), es_w(0x0A, 0x0C);            // 16 bits in and out
  es_w(0x0D, 0x01), es_w(0x0E, 0x02), es_w(0x12, 0x00), es_w(0x13, 0x10);  // power: analog, DAC, output
  es_w(0x1C, 0x6A), es_w(0x37, 0x08);            // no equalizers
  es_w(0x32, vol_reg(volume));
  rx = r, tx = t;
  xTaskCreatePinnedToCore(audio_task, "halo_audio", 3072, nullptr, 5, nullptr, 0);
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

// ---------- Microphones ----------
// ES7210 — the registers of es7210_adc_init + config_i2s + set_gain_all + start (the round watch): slave; dividers for
// MCLK = 256·fs (adc_div 1, doubler, DLL bypassed, OSR 32, LRCK = MCLK / 256 — the same for 16, 44.1 and 48 kHz); 16-bit
// I2S, ADC 1–2 → SDOUT1; microphones 1–4
static void mic_codec_on() {
  adc_w(0x00, 0xFF), adc_w(0x00, 0x41);                      // reset
  adc_w(0x01, 0x1F);                                          // clocks off while setting up
  adc_w(0x09, 0x30), adc_w(0x0A, 0x30);                       // initial state and power-up time
  adc_w(0x40, 0xC3), adc_w(0x41, 0x70), adc_w(0x42, 0x70);    // analog 3.3 V; microphone bias 2.87 V
  adc_w(0x07, 0x20), adc_w(0x02, 0xC1), adc_w(0x04, 0x01), adc_w(0x05, 0x00);  // OSR, ADC divider, LRCK
  adc_w(0x4B, 0x00), adc_w(0x4C, 0x00), adc_w(0x01, 0x00);    // power: bias, ADCs and amplifiers 1–4; clocks
  for (int i = 0; i < 4; i++) adc_w(0x43 + i, (adc_r(0x43 + i) & 0xE0) | 0x10 | MIC_GAIN);  // amplifier on
  adc_w(0x11, (adc_r(0x11) & 0x1C) | 0x60), adc_w(0x12, 0x00);  // 16 bits, I2S; ADC 1–2 → SDOUT1
  adc_w(0x06, 0x00);                                          // out of the off state
  for (int i = 0; i < 4; i++) adc_w(0x47 + i, 0x00);          // microphones 1–4
}

static void mic_codec_off() {  // as es7210_stop
  for (int i = 0; i < 4; i++) adc_w(0x47 + i, 0xFF);
  adc_w(0x4B, 0xFF), adc_w(0x4C, 0xFF), adc_w(0x01, 0x7F), adc_w(0x06, 0x07);
}

int halo_mic_begin(int sample_rate) {
  if (mic_want) return rate;
  if (!adc) adc = hx_device(0x40);
  if (!adc) return LOG("mic: ES7210 NOT FOUND\n"), 0;
  if (!ring) ring = (int16_t *)heap_caps_malloc(MIC_RING * 2, MALLOC_CAP_SPIRAM);
  if (!ring) return LOG("mic: no PSRAM for the ring\n"), 0;
  if (!tx && !audio_start(nullptr, sample_rate, true)) return 0;
  // The bus was up before (the speaker): receiving starts now; MCLK moves to the receive divider of the same rate on the fly
  if (!rx_ready && !rx_init(rx)) return 0;
  mic_codec_on();
  ring_tail = __atomic_load_n(&ring_head, __ATOMIC_ACQUIRE), mic_lost = 0;
  mic_level = mic_level1 = mic_level2 = -90;
  mic_want = true;  // the sound task turns receiving on in its next round (≤ 10–16 ms)
  halo_stay_awake(true);  // in real sleep I2S stops
  LOG("mic: ES7210 ok, %d Hz, gain %d dB\n", rate, MIC_GAIN * 3);
  return rate;
}

int halo_mic_read(int16_t *mono, int n) {
  if (!mic_want || n <= 0) return 0;
  const uint32_t h = __atomic_load_n(&ring_head, __ATOMIC_ACQUIRE);
  // Almost a whole ring behind: the oldest is being overwritten — skip it (a quarter of the ring spare: the sound task can
  // add all of the receive DMA at once while we copy)
  if (h - ring_tail > MIC_RING - MIC_RING / 4) {
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
  mic_want = false;  // the sound task turns receiving off in its next round
  mic_codec_off();
  halo_stay_awake(false);
  if (mic_lost) LOG("mic: %u samples skipped (read too late)\n", (unsigned)mic_lost);
  LOG("mic: off\n");
}

#endif  // HALO_BOARD_LCD4B
