// Звук для Halo: динамик через кодек ES8311 (I2C 0x18 — на шине тача) и усилитель (GPIO46), два микрофона через
// АЦП ES7210 (I2C 0x40).
//
// I2S (драйвер ESP-IDF i2s_std): ESP32 ведёт шину — MCLK = 256·fs на GPIO16, BCLK 9, LRCK 45, данные 8 (→ ES8311) и
// 10 (← ES7210); 16 бит, стерео (в динамик — в оба канала одно и то же: он один). Кодеки — ведомые на одних тактах,
// поэтому приём — пара к передаче на том же порту: i2s_new_channel сразу с обоими (полный дуплекс). Приём
// инициализируется, когда понадобился микрофон: поздний канал пары драйвер делает ведомым на BCLK/LRCK передачи
// (общие внутри I2S0). Но MCLK после этого идёт от делителя приёма (i2s_hal_set_rx_clock, частота та же), поэтому канал
// приёма не удаляется никогда: с ним остановился бы MCLK, а с ним и динамик. Регистры ES8311 — из драйвера Espressif
// (es8311.c, Apache-2.0; он же в lib/AmoledAudio): сброс, тактирование для MCLK = 256·fs (одинаковое для
// 16 000…48 000 Гц), формат 16 бит, питание ЦАП и выхода, громкость — регистр 0x32. ES7210 — из es7210.cpp там же.
//
// Звук даёт скетч: задача звука (ядро 0) зовёт fill(buf, n) — n отсчётов моно; вернул меньше n — дальше тишина.
// Пока тишина дольше 150 мс — усилитель выключен (динамик не шипит), задача спит по 10 мс, а DMA сам шлёт нули. Буфер
// DMA — 6 × 256 кадров (~70 мс при 22 050 Гц): запись во флеш (настройки) на миг останавливает задачи из флеша, звук
// за это время доигрывает из буфера. Микрофон: та же задача на каждом круге забирает из DMA приёма всё, что пришло
// (не ждёт), в кольцо в PSRAM, halo_mic_read — из кольца. DMA приёма — тоже 6 × 256 отсчётов (моно, 3 КБ): задача
// может опоздать на ~80 мс при 16 000 Гц, дальше пропадает самое старое.
#include "halo_internal.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/i2s_std.h>
#include <esp_heap_caps.h>

#define FRAMES 256
#define MIC_RING 16384  // отсчётов (32 КБ PSRAM): ~1 с при 16 000 Гц; степень двойки
#define MIC_GAIN 10     // усиление ES7210 шагами по 3 дБ: 10 — 30 дБ (как в AmoledAudio; тишина ≈ −61 дБ)
static i2s_chan_handle_t tx, rx;  // tx ≠ nullptr — шина запущена целиком
static i2c_master_dev_handle_t es, adc;
static HaloAudioFill fill_cb;
static int rate;
static volatile int volume = 60;
static volatile bool vol_dirty;
// Микрофон: кольцо пишет задача звука (ring_head), читает halo_mic_read (ring_tail)
static volatile bool rx_ready;  // канал приёма инициализирован (DMA выделен)
static volatile bool mic_want;  // halo_mic_begin / halo_mic_end
static bool mic_run;            // задача звука: приём включён (включает и выключает только она — она же читает)
static int16_t *ring;
static volatile uint32_t ring_head;
static uint32_t ring_tail, mic_lost;
static volatile float mic_level = -90;

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

static uint8_t vol_reg(int v) {  // 0..100 → регистр 0x32 (как es8311_voice_volume_set: 0 — тишина, 100 — +32 дБ)
  return v <= 0 ? 0 : (uint8_t)(v * 256 / 100 - 1);
}

// Всё, что пришло с микрофонов, — в кольцо (прямо из DMA, не ждёт); громкость — по последнему куску
// Приём, хоть и настроен на моно, приносит оба слота кадра LRCK — левый, правый, левый… (замер 2026-11-05: 42 970
// отсчётов в секунду на шине 22 050, и когда шину запускал микрофон, и когда динамик; видимо, в паре каналов слоты
// приёма — как у передачи, стерео). Это и есть оба микрофона: ES7210 кладёт АЦП 1 (MIC1) в левый слот SDOUT1, АЦП 2
// (MIC2) — в правый. Берём их среднее: голос у обоих один, а собственный шум у каждого свой — вместе на ~3 дБ тише
// (замер: каждый −50 дБ в комнате, их разница (L−R)/2 — −66…−72 дБ: два разных микрофона, слышат одно и то же).
// Пара может разорваться между чтениями: левый отсчёт ждёт правого в mic_carry
static bool mic_half;  // левый отсчёт пары уже прочитан и ждёт в mic_carry
static int16_t mic_carry;
static void mic_pull() {
  for (;;) {
    const uint32_t h = ring_head, at = h & (MIC_RING - 1);
    const int room = min(FRAMES, (int)(MIC_RING - at));  // до конца кольца
    size_t got = 0;
    i2s_channel_read(rx, ring + at, room * 2, &got, 0);
    const int raw = got / 2;
    if (raw <= 0) return;
    int n = 0, i = 0;
    auto pair = [&](int l, int r) {  // (на месте: пишем в n ≤ i, уже прочитанное)
      ring[at + n++] = (int16_t)((l + r) >> 1);
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
    __atomic_store_n(&ring_head, h + n, __ATOMIC_RELEASE);
    if (raw < room) return;
  }
}

static void audio_task(void *) {
  static int16_t mono[FRAMES], st[FRAMES * 2];
  bool pa = false;
  int silent = 1 << 30;  // отсчётов тишины подряд
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
      if (pa && silent > rate * 15 / 100) gpio_set_level((gpio_num_t)HALO_PIN_PA, 0), pa = false;
      if (!pa) {  // тихо: DMA сам шлёт нули (auto_clear), задаче — спать
        vTaskDelay(pdMS_TO_TICKS(10));
        continue;
      }
    } else {
      silent = 0;
      if (!pa) gpio_set_level((gpio_num_t)HALO_PIN_PA, 1), pa = true;
    }
    for (int i = 0; i < FRAMES; i++) st[2 * i] = st[2 * i + 1] = i < n ? mono[i] : 0;
    size_t w = 0;
    i2s_channel_write(tx, st, sizeof(st), &w, pdMS_TO_TICKS(200));
  }
}

// Канал приёма пары: моно (левый слот — MIC1), BCLK/LRCK — входы с тех же выводов (драйвер делает его ведомым на
// тактах передачи), вывод MCLK не трогаем. С этого момента MCLK — от делителя приёма
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

// with_rx — шину запускает микрофон: приём — сразу после передачи, до настройки кодеков (как в AmoledAudio), чтобы
// смена источника MCLK пришлась на время, когда кодеки ещё не настроены
static bool audio_start(HaloAudioFill fill, int sample_rate, bool with_rx) {
  if (tx) {  // шина уже идёт (её мог запустить микрофон — без fill): частота остаётся прежней
    if (!fill_cb) fill_cb = fill;
    if (sample_rate != rate) LOG("audio: already running at %d Hz, not %d\n", rate, sample_rate);
    return true;
  }
  es = ht_i2c_device(0x18);
  if (!es) {
    LOG("audio: ES8311 NOT FOUND\n");
    return false;
  }
  gpio_set_direction((gpio_num_t)HALO_PIN_PA, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)HALO_PIN_PA, 0);
  // I2S: сначала такты (кодеку нужен MCLK, чтобы принять настройки). Пара каналов занимает порт: второй запуск
  // одновременно из другой задачи здесь получит отказ, а не испортит первый
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
  // ES8311: сброс и включение, MCLK с вывода, все такты; делители для MCLK = 256·fs (pre_div 1, ×1, adc/dac div 1,
  // одинарная скорость, LRCK = MCLK/256, BCLK = MCLK/4, OSR 0x10)
  es_w(0x00, 0x1F), es_w(0x00, 0x00), es_w(0x00, 0x80);
  es_w(0x01, 0x3F);
  es_w(0x06, es_r(0x06) & ~0x20);                // BCLK не инвертирован
  es_w(0x02, es_r(0x02) & 0x07);                 // pre_div 1, pre_multi ×1
  es_w(0x03, 0x10), es_w(0x04, 0x10), es_w(0x05, 0x00);
  es_w(0x06, (es_r(0x06) & 0xE0) | 0x03);        // BCLK = MCLK / 4
  es_w(0x07, es_r(0x07) & 0xC0), es_w(0x08, 0xFF);  // LRCK = MCLK / 256
  es_w(0x00, es_r(0x00) & 0xBF);                 // ведомый
  es_w(0x09, 0x0C), es_w(0x0A, 0x0C);            // 16 бит на входе и выходе
  es_w(0x0D, 0x01), es_w(0x0E, 0x02), es_w(0x12, 0x00), es_w(0x13, 0x10);  // питание: аналог, ЦАП, выход
  es_w(0x1C, 0x6A), es_w(0x37, 0x08);            // без эквалайзеров
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

// ---------- Микрофоны ----------
// ES7210 — регистры как у es7210_adc_init + config_i2s + set_gain_all + start в lib/AmoledAudio (проверено на плате,
// 16 000 Гц): ведомый; делители для MCLK = 256·fs (adc_div 1, удвоитель, DLL в обход, OSR 32, LRCK = MCLK / 256 —
// одни и те же в таблице драйвера для 16, 44,1 и 48 кГц); 16 бит I2S, АЦП 1–2 → SDOUT1; микрофоны 1–4 (на плате
// два: MIC1 и MIC2 — левый и правый канал)
static void mic_codec_on() {
  adc_w(0x00, 0xFF), adc_w(0x00, 0x41);                      // сброс
  adc_w(0x01, 0x1F);                                          // такты выключены, пока настраиваем
  adc_w(0x09, 0x30), adc_w(0x0A, 0x30);                       // время начального состояния и включения
  adc_w(0x40, 0xC3), adc_w(0x41, 0x70), adc_w(0x42, 0x70);    // аналог 3,3 В; смещение микрофонов 2,87 В
  adc_w(0x07, 0x20), adc_w(0x02, 0xC1), adc_w(0x04, 0x01), adc_w(0x05, 0x00);  // OSR, делитель АЦП, LRCK
  adc_w(0x4B, 0x00), adc_w(0x4C, 0x00), adc_w(0x01, 0x00);    // питание смещения, АЦП и усилителей 1–4; такты
  for (int i = 0; i < 4; i++) adc_w(0x43 + i, (adc_r(0x43 + i) & 0xE0) | 0x10 | MIC_GAIN);  // усилитель вкл.
  adc_w(0x11, (adc_r(0x11) & 0x1C) | 0x60), adc_w(0x12, 0x00);  // 16 бит, I2S; АЦП 1–2 → SDOUT1
  adc_w(0x06, 0x00);                                          // из выключенного состояния
  for (int i = 0; i < 4; i++) adc_w(0x47 + i, 0x00);          // микрофоны 1–4
}

static void mic_codec_off() {  // как es7210_stop
  for (int i = 0; i < 4; i++) adc_w(0x47 + i, 0xFF);
  adc_w(0x4B, 0xFF), adc_w(0x4C, 0xFF), adc_w(0x01, 0x7F), adc_w(0x06, 0x07);
}

int halo_mic_begin(int sample_rate) {
  if (mic_want) return rate;
  if (!adc) adc = ht_i2c_device(0x40);
  if (!adc) return LOG("mic: ES7210 NOT FOUND\n"), 0;
  if (!ring) ring = (int16_t *)heap_caps_malloc(MIC_RING * 2, MALLOC_CAP_SPIRAM);
  if (!ring) return LOG("mic: no PSRAM for the ring\n"), 0;
  if (!tx && !audio_start(nullptr, sample_rate, true)) return 0;
  // Шина шла и раньше (динамик): приём — сейчас; MCLK переходит на делитель приёма той же частоты на ходу
  if (!rx_ready && !rx_init(rx)) return 0;
  mic_codec_on();
  ring_tail = __atomic_load_n(&ring_head, __ATOMIC_ACQUIRE), mic_lost = 0;
  mic_level = -90;
  mic_want = true;  // приём включит задача звука на следующем круге (≤ 10–16 мс)
  halo_stay_awake(true);  // в настоящем сне I2S стоит
  LOG("mic: ES7210 ok, %d Hz, gain %d dB\n", rate, MIC_GAIN * 3);
  return rate;
}

int halo_mic_read(int16_t *mono, int n) {
  if (!mic_want || n <= 0) return 0;
  const uint32_t h = __atomic_load_n(&ring_head, __ATOMIC_ACQUIRE);
  // Отстали почти на круг: старое уже перезаписывается — пропустить его (запас в четверть кольца: задача звука
  // может разом дописать весь DMA приёма, пока мы копируем)
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

void halo_mic_end() {
  if (!mic_want) return;
  mic_want = false;  // приём выключит задача звука на следующем круге
  mic_codec_off();
  halo_stay_awake(false);
  if (mic_lost) LOG("mic: %u samples skipped (read too late)\n", (unsigned)mic_lost);
  LOG("mic: off\n");
}
