// The ESP32-S3-Touch-LCD-4B display for Halo: a 4″ 480 × 480 ST7701 panel on the S3's 16-bit RGB bus (LCD_CAM, ESP-IDF's
// esp_lcd "rgb" driver). The measurements this is built on: experiments/LCD4BBringup, boards/lcd4b/hardware.md.
//
// The panel keeps no picture of its own in this mode: 60.3 times a second the S3 sends it the whole frame. So:
//   * the picture is a frame buffer of ours in PSRAM (450 KB). The LCD does not read it directly: PSRAM is one bus for the
//     LCD's reads, our writes and the processor's cache, and writing while the LCD's DMA reads it starves the scan — the
//     whole screen blinks. The LCD reads two bounce buffers of 16 rows in internal memory (the driver's no_fb mode), and a
//     GDMA pair of ours, memory to memory, fills each from the frame buffer the moment the LCD is done with it (the
//     processor only starts the copy: ~0.3 ms of the 0.52 the LCD takes to empty the other one);
//   * a band the engine drew goes into the frame buffer by a second GDMA pair, a descriptor a row, made ahead (ESP-IDF's
//     esp_async_memcpy builds a new list for every call, ~90 µs). Spans are multiples of HD_SPAN pixels: the DMA writes
//     PSRAM in blocks of HD_BURST bytes;
//   * one buffer, no tearing: a "pass" reads the frame buffer top to bottom, 16 rows at a time, and a band is written
//     once the pass going on has read it — the next pass shows the frame whole. The pass's first 32 rows are read just
//     before the LCD's blank (it reads ahead), the rest one 16-row step before the LCD shows them; hd_read() follows the
//     fills themselves, not a clock. "TE" for the engine is a pass's start;
//   * the panel is set up over a 3-wire SPI that the TCA9554 expander bit-bangs (CS, SDA, SCL on its pins 0-2, reset on 7).
//     The expander is also where the touch chip's reset and the speaker's amplifier are — and the same I2C bus carries
//     everything, so it starts here;
//   * brightness: GPIO4's PWM pulls the backlight converter's feedback up (schematic: R37, R29 into FB) — full duty is dark.
#include <sdkconfig.h>
#if defined(HALO_BOARD_LCD4B)  // this board's port: other boards build their own
#include "halo_lcd4b.h"

#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <esp_cache.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_private/gdma.h>
#include <esp_timer.h>
#include <hal/dma_types.h>
#include <soc/gdma_struct.h>

#define MAX_PX (HALO_ROWS * HALO_W)
#define MAX_BUFS 4
#define BB_ROWS 16  // a bounce buffer: 16 rows, 15 KB; the LCD empties one in 16 × 32.5 µs = 520 µs
#define CHUNKS (HALO_H / BB_ROWS)
#ifndef HD_LCD_PRIO
#define HD_LCD_PRIO 9  // the LCD's DMA channel (0-9); the fills 5, the band writes 0
#endif
#ifndef HD_GAP
#define HD_GAP 32768  // internal memory kept between the bounce buffers and the band buffers (below)
#endif
// The panel's timing (Espressif's esp_lcd_st7701: ST7701_480_480_PANEL_60HZ_RGB_TIMING): 520 × 510 clocks a frame, 60.3 Hz
#define PCLK_HZ 16000000
#define FB_BYTES (HALO_W * HALO_H * 2)
#define ACTIVE_LEAD_US 634  // from VSYNC's interrupt to the first row on the panel (VSYNC 10 rows + back porch 10, measured)

// ---------- The I2C bus and the expander ----------
static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t xdev;
static uint8_t x_out = (1 << HX_LCD_CS) | (1 << HX_LCD_RST) | (1 << HX_TP_RST), x_cfg = 0xFF;
static SemaphoreHandle_t x_mx;

i2c_master_bus_t *hx_bus() {
  return bus;
}

i2c_master_dev_t *hx_device(uint8_t addr) {
  if (!bus || i2c_master_probe(bus, addr, 50) != ESP_OK) return nullptr;
  i2c_device_config_t dc = {};
  dc.dev_addr_length = I2C_ADDR_BIT_LEN_7, dc.device_address = addr, dc.scl_speed_hz = 400000;
  i2c_master_dev_handle_t h = nullptr;
  return i2c_master_bus_add_device(bus, &dc, &h) == ESP_OK ? h : nullptr;
}

static bool xw(uint8_t reg, uint8_t v) {
  const uint8_t w[2] = {reg, v};
  return xdev && i2c_master_transmit(xdev, w, 2, 20) == ESP_OK;
}

void hx_set(int pin, bool high) {
  xSemaphoreTake(x_mx, portMAX_DELAY);
  x_out = high ? x_out | 1 << pin : x_out & ~(1 << pin);
  xw(1, x_out);
  xSemaphoreGive(x_mx);
}

void hx_dir(int pin, bool output) {
  xSemaphoreTake(x_mx, portMAX_DELAY);
  x_cfg = output ? x_cfg & ~(1 << pin) : x_cfg | 1 << pin;
  xw(3, x_cfg);
  xSemaphoreGive(x_mx);
}

int hx_inputs() {
  uint8_t r = 0, v = 0;
  return xdev && i2c_master_transmit_receive(xdev, &r, 1, &v, 1, 20) == ESP_OK ? v : -1;
}

// ---------- The panel's 3-wire SPI through the expander: 9 bits a word (D/C, then 8 bits MSB first), on SCL's rise ----------
static void spi9(bool data, uint8_t v) {
  const uint16_t w = (data ? 0x100 : 0) | v;
  for (int i = 8; i >= 0; i--) {
    x_out &= ~(1 << HX_LCD_SCL);
    x_out = (w >> i & 1) ? x_out | 1 << HX_LCD_SDA : x_out & ~(1 << HX_LCD_SDA);
    xw(1, x_out);
    x_out |= 1 << HX_LCD_SCL;
    xw(1, x_out);
  }
}

static void lcd_cmd(uint8_t c, std::initializer_list<uint8_t> d = {}, int wait_ms = 0) {
  x_out &= ~(1 << HX_LCD_CS);
  xw(1, x_out);
  spi9(false, c);
  for (uint8_t v : d) spi9(true, v);
  x_out = (x_out | 1 << HX_LCD_CS) & ~(1 << HX_LCD_SCL);
  xw(1, x_out);
  if (wait_ms) delay(wait_ms);
}

// Waveshare's BSP v2.0.0 (waveshare/esp32_s3_touch_lcd_4b) sequence, with the commands its ST7701 driver sends first
static void st7701_init() {
  lcd_cmd(0xFF, {0x77, 0x01, 0x00, 0x00, 0x00});
  lcd_cmd(0x36, {0x00});
  lcd_cmd(0x3A, {0x66});
  lcd_cmd(0x11, {}, 120);  // sleep out
  lcd_cmd(0xFF, {0x77, 0x01, 0x00, 0x00, 0x10});
  lcd_cmd(0xC0, {0x3B, 0x00});
  lcd_cmd(0xC1, {0x0D, 0x02});
  lcd_cmd(0xC2, {0x21, 0x08});
  lcd_cmd(0xCD, {0x08});
  lcd_cmd(0xB0, {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18});
  lcd_cmd(0xB1, {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18});
  lcd_cmd(0xFF, {0x77, 0x01, 0x00, 0x00, 0x11});
  lcd_cmd(0xB0, {0x60});
  lcd_cmd(0xB1, {0x30});
  lcd_cmd(0xB2, {0x87});
  lcd_cmd(0xB3, {0x80});
  lcd_cmd(0xB5, {0x49});
  lcd_cmd(0xB7, {0x85});
  lcd_cmd(0xB8, {0x21});
  lcd_cmd(0xC1, {0x78});
  lcd_cmd(0xC2, {0x78}, 20);
  lcd_cmd(0xE0, {0x00, 0x1B, 0x02});
  lcd_cmd(0xE1, {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44});
  lcd_cmd(0xE2, {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00});
  lcd_cmd(0xE3, {0x00, 0x00, 0x11, 0x11});
  lcd_cmd(0xE4, {0x44, 0x44});
  lcd_cmd(0xE5, {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0});
  lcd_cmd(0xE6, {0x00, 0x00, 0x11, 0x11});
  lcd_cmd(0xE7, {0x44, 0x44});
  lcd_cmd(0xE8, {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0});
  lcd_cmd(0xEB, {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40});
  lcd_cmd(0xEC, {0x3C, 0x00});
  lcd_cmd(0xED, {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA});
  lcd_cmd(0xFF, {0x77, 0x01, 0x00, 0x00, 0x00});
  lcd_cmd(0x36, {0x00});
  lcd_cmd(0x3A, {0x66});
  lcd_cmd(0x21, {}, 120);  // inversion on: this panel's
  lcd_cmd(0x29);           // display on
}

// ---------- Copies: a GDMA pair memory-to-memory, its descriptors made once ----------
struct M2M {
  gdma_channel_handle_t tx, rx;
  dma_descriptor_t *td, *rd;
  volatile bool busy;
  volatile uint32_t start_t, max_us, n, sum_us, bytes;
};
static M2M bm, wm;  // bm: bounce buffer fills (each has a deadline); wm: bands into the frame buffer

static inline void IRAM_ATTR desc_set(dma_descriptor_t *d, void *buf, int len, bool last, bool rx) {
  d->dw0.size = len;
  d->dw0.length = rx ? 0 : len;
  d->dw0.err_eof = 0;
  d->dw0.suc_eof = !rx && last;
  d->dw0.owner = DMA_DESCRIPTOR_BUFFER_OWNER_DMA;
  d->buffer = buf;
  d->next = last ? nullptr : d + 1;
}

static void IRAM_ATTR m2m_go(M2M &m) {
  m.busy = true;
  m.start_t = (uint32_t)esp_timer_get_time();
  gdma_start(m.rx, (intptr_t)m.rd);
  gdma_start(m.tx, (intptr_t)m.td);
}

static inline void IRAM_ATTR m2m_done(M2M &m) {
  const uint32_t us = (uint32_t)esp_timer_get_time() - m.start_t;
  if (us > m.max_us) m.max_us = us;
  m.sum_us = m.sum_us + us;
  m.n = m.n + 1;
  m.busy = false;
}

static bool m2m_init(M2M &m, int pieces, int prio, gdma_event_callback_t eof) {
  gdma_channel_alloc_config_t tc = {};
  tc.direction = GDMA_CHANNEL_DIRECTION_TX;
  tc.flags.reserve_sibling = 1;
  if (gdma_new_ahb_channel(&tc, &m.tx) != ESP_OK) return false;
  gdma_channel_alloc_config_t rc = {};
  rc.direction = GDMA_CHANNEL_DIRECTION_RX;
  rc.sibling_chan = m.tx;
  if (gdma_new_ahb_channel(&rc, &m.rx) != ESP_OK) return false;
  gdma_trigger_t trig = {};
  trig.periph = GDMA_TRIG_PERIPH_M2M;
  trig.bus_id = SOC_GDMA_BUS_ANY;
  uint32_t mask = 0;
  gdma_get_free_m2m_trig_id_mask(m.tx, &mask);
  trig.instance_id = __builtin_ctz(mask);
  if (gdma_connect(m.rx, trig) != ESP_OK || gdma_connect(m.tx, trig) != ESP_OK) return false;
  gdma_strategy_config_t st = {};
  st.owner_check = false, st.auto_update_desc = false, st.eof_till_data_popped = true;
  gdma_apply_strategy(m.tx, &st);
  gdma_apply_strategy(m.rx, &st);
  gdma_transfer_config_t tr = {};
  tr.max_data_burst_size = HD_BURST, tr.access_ext_mem = true;
  gdma_config_transfer(m.tx, &tr);
  gdma_config_transfer(m.rx, &tr);
  gdma_set_priority(m.tx, prio);
  gdma_set_priority(m.rx, prio);
  gdma_rx_event_callbacks_t cbs = {};
  cbs.on_recv_eof = eof;
  gdma_register_rx_event_callbacks(m.rx, &cbs, &m);
  m.td = (dma_descriptor_t *)heap_caps_calloc(pieces, sizeof(dma_descriptor_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  m.rd = (dma_descriptor_t *)heap_caps_calloc(pieces, sizeof(dma_descriptor_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  return m.td && m.rd;
}

// ---------- Passes: the bounce fills read the frame buffer ----------
static esp_lcd_panel_handle_t panel;
static uint16_t *fb;
static DRAM_ATTR volatile uint32_t pass_n, pass_t, pass_period, read_rows, fill_rows, fill_pass, fills, late_fills;
static DRAM_ATTR volatile uint32_t te_blank = 1480, vs_count;
static DRAM_ATTR uint32_t read_at[CHUNKS];  // µs from a pass's start to its fill of chunk k being done (the last pass's)
static DRAM_ATTR volatile TaskHandle_t te_waiter;
static void *volatile pend_buf;  // a fill asked for while the last one still runs: started from its interrupt
static volatile int pend_pos;
static portMUX_TYPE te_mux = portMUX_INITIALIZER_UNLOCKED;  // the pass's number and its time — as a pair

static void IRAM_ATTR fill_start(void *buf, int pos_px) {
  const int k = 4, part = HALO_W * BB_ROWS * 2 / k;  // 3840 bytes: four rows, a multiple of HD_BURST
  const uint8_t *src = (const uint8_t *)fb + pos_px * 2;
  for (int i = 0; i < k; i++) {
    desc_set(&bm.td[i], (void *)(src + i * part), part, i == k - 1, false);
    desc_set(&bm.rd[i], (uint8_t *)buf + i * part, part, i == k - 1, true);
  }
  fill_rows = pos_px / HALO_W + BB_ROWS;
  fill_pass = pass_n;
  bm.bytes = bm.bytes + HALO_W * BB_ROWS * 2;
  m2m_go(bm);
}

static bool IRAM_ATTR bm_eof(gdma_channel_handle_t, gdma_event_data_t *, void *) {
  m2m_done(bm);
  if (fill_pass == pass_n) {  // (a late fill of the last pass's end does not count for this one)
    read_rows = fill_rows;
    read_at[fill_rows / BB_ROWS - 1] = (uint32_t)esp_timer_get_time() - pass_t;
  }
  if (pend_buf) {
    void *b = pend_buf;
    pend_buf = nullptr;
    fill_start(b, pend_pos);
  }
  return false;
}

// The last late fills, for the lcd command: which 16 rows, how long the fill before it had run, since the last call
struct Late {
  uint16_t chunk, busy_us, gap_us, pend;
};
static DRAM_ATTR Late lates[8];
static DRAM_ATTR volatile uint32_t empty_t, vs_isr_t, gap_max, gap_early, from_vsync, isr_core;
static volatile bool no_writes;
static void *volatile bb_seen[2];  // the driver's bounce buffers (they come to the fill callback)  // (debug: lcd nowrite — bands are dropped, not written)

static bool IRAM_ATTR on_bounce_empty(esp_lcd_panel_handle_t, void *buf, int pos_px, int, void *) {
  fills = fills + 1;
  const uint32_t t_in = (uint32_t)esp_timer_get_time(), gap = t_in - empty_t;
  empty_t = t_in;
  // How regular the calls are: 520 µs apart inside a frame (1.5 ms before the third chunk: the LCD's blank). A late
  // interrupt shows as a long gap and then a short one; a call from the driver's restart at VSYNC — right after it
  const int ck = pos_px / (HALO_W * BB_ROWS);
  isr_core = xPortGetCoreID();
  if (bb_seen[0] != buf && !bb_seen[1]) bb_seen[bb_seen[0] ? 1 : 0] = buf;
  if (t_in - vs_isr_t < 30) from_vsync = from_vsync + 1;
  else if (ck != 2) {
    if (gap > gap_max) gap_max = gap;
    if (gap < 400) gap_early = gap_early + 1;
  }
  BaseType_t woken = pdFALSE;
  if (pos_px == 0) {  // a new pass over the frame buffer: "TE"
    const uint32_t now = (uint32_t)esp_timer_get_time();
    portENTER_CRITICAL_ISR(&te_mux);
    if (pass_t && now - pass_t < 40000) pass_period = now - pass_t;
    pass_t = now;
    pass_n = pass_n + 1;
    read_rows = 0;
    portEXIT_CRITICAL_ISR(&te_mux);
    if (halo_trace_on) halo_trace_put(TR_TE, now, 0, 0);
    TaskHandle_t w = te_waiter;
    if (w) vTaskNotifyGiveFromISR(w, &woken);
  }
  // The fill started last time (the buffer the LCD is reading now) must be done: if not, the LCD shows old rows there.
  // This one waits for it in the queue — never in the interrupt
  if (bm.busy) {
    Late &l = lates[late_fills % 8];
    l.chunk = pos_px / (HALO_W * BB_ROWS), l.busy_us = t_in - bm.start_t, l.gap_us = gap, l.pend = pend_buf != nullptr;
    late_fills = late_fills + 1;
    pend_pos = pos_px, pend_buf = buf;
  } else {
    fill_start(buf, pos_px);
  }
  return woken == pdTRUE;
}

// Where the LCD's own DMA is at VSYNC (debug): the descriptor it reads. Done with the frame — always the same one
static DRAM_ATTR int lcd_ch = -1;
static DRAM_ATTR uint32_t vs_dscr[4], vs_dscr_n[4], vs_dscr_other;

static bool IRAM_ATTR on_vsync(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *) {
  vs_isr_t = (uint32_t)esp_timer_get_time();
  if (lcd_ch >= 0) {
    const uint32_t d = GDMA.channel[lcd_ch].out.state.dscr_addr;
    int i = 0;
    while (i < 4 && vs_dscr_n[i] && vs_dscr[i] != d) i++;
    if (i < 4) vs_dscr[i] = d, vs_dscr_n[i] = vs_dscr_n[i] + 1;
    else vs_dscr_other = vs_dscr_other + 1;
  }
  const uint32_t d = (uint32_t)esp_timer_get_time() - pass_t;
  if (d < 5000) te_blank = d + ACTIVE_LEAD_US;  // the pass's start → the LCD's first row
  vs_count = vs_count + 1;
  if (halo_trace_on) halo_trace_put(TR_SCAN, (uint32_t)esp_timer_get_time(), 0, 0);
  return false;
}

bool hd_wait_te(uint32_t count, uint32_t timeout_ms) {
  te_waiter = xTaskGetCurrentTaskHandle();
  ulTaskNotifyTake(pdTRUE, 0);  // old notifications do not count — the counter does
  bool ok = true;
  while (ok && (int32_t)(pass_n - count) < 0) ok = ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(timeout_ms)) > 0;
  te_waiter = nullptr;
  return ok;
}

void hd_te_snapshot(uint32_t *count, uint32_t *rise) {
  portENTER_CRITICAL(&te_mux);
  *count = pass_n, *rise = pass_t;
  portEXIT_CRITICAL(&te_mux);
}

uint32_t hd_te_count() {
  return pass_n;
}
uint32_t hd_te_period() {
  return (uint32_t)esp_timer_get_time() - pass_t < 100000 ? pass_period : 0;
}
uint32_t hd_te_rise() {
  return pass_t;
}
uint32_t hd_te_blank() {
  return te_blank;
}

bool hd_read(uint32_t pass, int rows) {
  return pass_n != pass || read_rows >= (uint32_t)rows;
}

uint32_t hd_read_eta(int rows) {
  const int k = constrain((rows + BB_ROWS - 1) / BB_ROWS - 1, 0, CHUNKS - 1);
  const uint32_t at = read_at[k] ? read_at[k] : 2100 + k * 520;
  return pass_t + at;
}

const uint16_t *hd_fb() {
  return fb;
}

// ---------- Bands into the frame buffer: a queue, the next starts from the last one's interrupt ----------
struct WJob {
  uint16_t *tile;
  int16_t y, x0, w, rows;
};
#define WQ 8
static WJob wq[WQ];
static volatile int wq_head, wq_tail;
static portMUX_TYPE wq_mux = portMUX_INITIALIZER_UNLOCKED;
static uint16_t *bufs[MAX_BUFS];
static int nbufs, next_buf;
static SemaphoreHandle_t free_sem;

static void IRAM_ATTR wm_start(const WJob &j) {
  for (int r = 0; r < j.rows; r++) {
    desc_set(&wm.td[r], j.tile + r * j.w, j.w * 2, r == j.rows - 1, false);
    desc_set(&wm.rd[r], fb + (j.y + r) * HALO_W + j.x0, j.w * 2, r == j.rows - 1, true);
  }
  wm.bytes = wm.bytes + j.w * j.rows * 2;
  m2m_go(wm);
}

static bool IRAM_ATTR wm_eof(gdma_channel_handle_t, gdma_event_data_t *, void *) {
  m2m_done(wm);
  BaseType_t woken = pdFALSE;
  portENTER_CRITICAL_ISR(&wq_mux);
  wq_tail = wq_tail + 1;
  if (wq_head != wq_tail) wm_start(wq[wq_tail % WQ]);
  portEXIT_CRITICAL_ISR(&wq_mux);
  xSemaphoreGiveFromISR(free_sem, &woken);
  return woken == pdTRUE;
}

static void take_free() {
  xSemaphoreTake(free_sem, portMAX_DELAY);
}

uint16_t *hd_buffer() {
  if (uxSemaphoreGetCount(free_sem) == 0) {
    const uint32_t t0 = trace_now();
    take_free();
    trace_span(TR_BLIT_WAIT, t0);
  } else {
    xSemaphoreTake(free_sem, 0);
  }
  return bufs[next_buf];
}

void hd_buffer2(uint16_t **a, uint16_t **b) {
  *a = hd_buffer();
  const uint32_t t0 = trace_now();
  take_free();
  if (trace_now() - t0 >= 5) trace_span(TR_BLIT_WAIT, t0);
  *b = bufs[(next_buf + 1) % nbufs];
}

void hd_send(int x0, int y, int w, int rows) {
  if (no_writes) {
    next_buf = (next_buf + 1) % nbufs;
    xSemaphoreGive(free_sem);
    return;
  }
  const WJob j = {bufs[next_buf], (int16_t)y, (int16_t)x0, (int16_t)w, (int16_t)rows};
  next_buf = (next_buf + 1) % nbufs;
  portENTER_CRITICAL(&wq_mux);
  wq[wq_head % WQ] = j;  // (never full: a job holds one of nbufs < WQ buffers)
  const bool idle = wq_head == wq_tail;
  wq_head = wq_head + 1;
  if (idle) wm_start(wq[wq_tail % WQ]);
  portEXIT_CRITICAL(&wq_mux);
}

void hd_wait_idle() {
  for (int i = 0; i < nbufs; i++) take_free();
  for (int i = 0; i < nbufs; i++) xSemaphoreGive(free_sem);
}

// The light: 10 bits of PWM, the eye's curve (a square). Full duty is dark here, so the duty is the dark part
static uint8_t bl_level = 180;
static void bl_write(uint8_t level) {
  const uint32_t light = level ? max(24, (int)level * level * 1023 / 65025) : 0;
  ledcWrite(HALO_PIN_BL, 1023 - light);
}

void hd_set_brightness(uint8_t level) {
  bl_level = level;
  bl_write(level);
}

void hd_set_power(bool on) {  // the panel keeps getting the picture; only the light goes
  bl_write(on ? bl_level : 0);
}

int hd_buffers() {
  return nbufs;
}

void hd_state(int *f, int *queued, int *free_bufs) {
  const int fr = free_sem ? (int)uxSemaphoreGetCount(free_sem) : -1;
  *f = fr >= 0 && fr < nbufs ? 0 : -1, *queued = fr >= 0 ? nbufs - fr : 0, *free_bufs = fr;
}

void hd_regs(char *out, int n) {
  snprintf(out, n, "RGB 16 MHz, passes %lu, fills %lu (late %lu, max %lu us), writes %lu (max %lu us)", (unsigned long)pass_n,
           (unsigned long)fills, (unsigned long)late_fills, (unsigned long)bm.max_us, (unsigned long)wm.n,
           (unsigned long)wm.max_us);
}

// The RGB stream (no frame buffer in the driver, two bounce buffers that we fill) and the copy engines. Their interrupts go
// to the core that sets them up: a task there does it
static volatile int stream_ok = -1;
static int stream_start() {
  if (!m2m_init(bm, 4, 5, bm_eof) || !m2m_init(wm, HALO_ROWS, 0, wm_eof)) {
    LOG("display: no GDMA channels for the copies\n");
    return 0;
  }
  esp_lcd_rgb_panel_config_t c = {};
  c.clk_src = LCD_CLK_SRC_DEFAULT;
  c.timings.pclk_hz = PCLK_HZ;
  c.timings.h_res = HALO_W, c.timings.v_res = HALO_H;
  c.timings.hsync_pulse_width = 10, c.timings.hsync_back_porch = 10, c.timings.hsync_front_porch = 20;
  c.timings.vsync_pulse_width = 10, c.timings.vsync_back_porch = 10, c.timings.vsync_front_porch = 10;
  c.data_width = 16;
  c.bits_per_pixel = 16;
  c.num_fbs = 0;
  c.bounce_buffer_size_px = HALO_W * BB_ROWS;
  c.dma_burst_size = 64;
  c.hsync_gpio_num = HALO_PIN_HSYNC, c.vsync_gpio_num = HALO_PIN_VSYNC, c.de_gpio_num = HALO_PIN_DE;
  c.pclk_gpio_num = HALO_PIN_PCLK, c.disp_gpio_num = -1;
  const int pins[16] = HALO_LCD_DATA_PINS;
  for (int i = 0; i < 16; i++) c.data_gpio_nums[i] = pins[i];
  c.flags.no_fb = 1;
  if (esp_lcd_new_rgb_panel(&c, &panel) != ESP_OK) {
    LOG("display: the RGB bus did not start\n");
    return 0;
  }
  for (int ch = 0; ch < 5; ch++)
    if (GDMA.channel[ch].out.peri_sel.sel == 5) lcd_ch = ch;  // (5: LCD_CAM)
  // The LCD's channel goes first among the DMA channels: it has no time to wait. At priority 0 it lost to the fills and the
  // band writes while both cores blended whole bands — in 40 % of frames it had not read the last rows by VSYNC
  if (lcd_ch >= 0) GDMA.channel[lcd_ch].out.pri.tx_pri = HD_LCD_PRIO;
  esp_lcd_rgb_panel_event_callbacks_t cbs = {};
  cbs.on_vsync = on_vsync;
  cbs.on_bounce_empty = on_bounce_empty;
  esp_lcd_rgb_panel_register_event_callbacks(panel, &cbs, nullptr);
  esp_lcd_panel_reset(panel);
  esp_lcd_panel_init(panel);

  return 1;
}

static void stream_task(void *) {
  stream_ok = stream_start();
  vTaskDelete(nullptr);
}

// ---------- Start ----------
bool hd_begin(uint32_t, int buffers, uint8_t brightness) {
  // The light stays off until the frame buffer is black and the panel is set up
  ledcAttach(HALO_PIN_BL, 20000, 10);
  ledcWrite(HALO_PIN_BL, 1023);
  bl_level = brightness;

  hc_stage(1);
  x_mx = xSemaphoreCreateMutex();
  i2c_master_bus_config_t bc = {};
  bc.i2c_port = I2C_NUM_0;
  bc.sda_io_num = (gpio_num_t)HALO_PIN_SDA;
  bc.scl_io_num = (gpio_num_t)HALO_PIN_SCL;
  bc.clk_source = I2C_CLK_SRC_DEFAULT;
  bc.glitch_ignore_cnt = 7;
  bc.flags.enable_internal_pullup = 1;
  if (i2c_new_master_bus(&bc, &bus) != ESP_OK) {
    LOG("display: the I2C bus did not start\n");
    return false;
  }
  xdev = hx_device(0x20);
  if (!xdev) {
    LOG("display: no TCA9554 expander at 0x20\n");
    return false;
  }
  xw(1, x_out);  // CS high, resets high, SCL and SDA low
  x_cfg = (1 << HX_PWR_KEY) | (1 << HX_TP_INT);
  xw(3, x_cfg);

  // Our frame buffer, the copy engines, the band buffers
  fb = (uint16_t *)heap_caps_aligned_calloc(64, 1, FB_BYTES, MALLOC_CAP_SPIRAM);
  if (!fb) {
    LOG("display: no PSRAM for the frame buffer\n");
    return false;
  }
  esp_cache_msync(fb, FB_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
  // The RGB stream and the copies — their interrupts on core HD_ISR_CORE
  hc_stage(3);
  xTaskCreatePinnedToCore(stream_task, "halo_lcd", 4096, nullptr, 20, nullptr, HD_ISR_CORE);
  while (stream_ok < 0) delay(1);
  if (!stream_ok) return false;

  // The band buffers — away from the bounce buffers: internal memory is in blocks, and in a block the processors go
  // before DMA. Two cores blending a band right next to a bounce buffer held the LCD's DMA up to 0.6 ms (a late fill)
  void *gap = heap_caps_malloc(HD_GAP, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  nbufs = constrain(buffers, 2, MAX_BUFS);
  for (int i = 0; i < nbufs; i++) {
    bufs[i] = (uint16_t *)heap_caps_aligned_alloc(64, MAX_PX * 2, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!bufs[i]) {
      LOG("display: no DMA memory\n");
      return false;
    }
  }
  free(gap);
  free_sem = xSemaphoreCreateCounting(nbufs, nbufs);

  // The panel: reset, then its setup through the expander (~0.9 s: every clock edge is an I2C write)
  hc_stage(2);
  const uint32_t t0 = millis();
  hx_set(HX_LCD_RST, false);
  delay(10);
  hx_set(HX_LCD_RST, true);
  delay(120);
  st7701_init();
  hd_set_brightness(brightness);

  // lcd — the copies since the last time: how fast the fills and the writes went, how many fills were late
  halo_command("lcd", [](const char *a) {
    if (!strcmp(a, "nowrite")) return (void)(no_writes = !no_writes);
    if (!strcmp(a, "mem")) {
      LOG("lcd: bounce %p %p, band buffers %p %p %p %p, descriptors %p %p\n", bb_seen[0], bb_seen[1], bufs[0], bufs[1],
          bufs[2], nbufs > 3 ? bufs[3] : nullptr, bm.td, wm.td);
      return;
    }
    static uint32_t t0, bn0, bs0, bb0, wn0, ws0, wb0, late0;
    const uint32_t now = millis(), dt = max((uint32_t)1, now - t0);
    const uint32_t bn = bm.n - bn0, wn = wm.n - wn0, bs = bm.sum_us - bs0, ws = wm.sum_us - ws0;
    const uint32_t bb = bm.bytes - bb0, wb = wm.bytes - wb0;
    LOG("lcd: %lu ms: fills %lu, %.1f MB/s while running (max %lu us, late %lu); writes %lu, %.1f MB/s while running, "
        "%.1f MB/s overall (max %lu us, busy %.0f%%); pass %lu us, blank %lu us, rows read at +%lu +%lu +%lu ... +%lu us\n",
        (unsigned long)dt, (unsigned long)bn, bs ? bb / (float)bs : 0, (unsigned long)bm.max_us,
        (unsigned long)(late_fills - late0), (unsigned long)wn, ws ? wb / (float)ws : 0, wb / (dt * 1000.0f),
        (unsigned long)wm.max_us, ws / (dt * 10.0f), (unsigned long)pass_period, (unsigned long)te_blank,
        (unsigned long)read_at[0], (unsigned long)read_at[1], (unsigned long)read_at[2], (unsigned long)read_at[CHUNKS - 1]);
    LOG("lcd: fill calls (core %lu): longest gap %lu us, %lu too soon after the last one, %lu from the restart at VSYNC\n",
        (unsigned long)isr_core, (unsigned long)gap_max, (unsigned long)gap_early, (unsigned long)from_vsync);
    gap_max = gap_early = from_vsync = 0;
    LOG("lcd: the LCD's DMA at VSYNC (channel %d): %05lX x%lu, %05lX x%lu, %05lX x%lu, %05lX x%lu, other %lu\n", lcd_ch,
        (unsigned long)vs_dscr[0], (unsigned long)vs_dscr_n[0], (unsigned long)vs_dscr[1], (unsigned long)vs_dscr_n[1],
        (unsigned long)vs_dscr[2], (unsigned long)vs_dscr_n[2], (unsigned long)vs_dscr[3], (unsigned long)vs_dscr_n[3],
        (unsigned long)vs_dscr_other);
    memset(vs_dscr_n, 0, sizeof(vs_dscr_n)), vs_dscr_other = 0;
    for (int i = 0; i < 8 && i < (int)(late_fills - late0); i++) {
      const Late &l = lates[(late_fills - 1 - i) % 8];
      LOG("lcd: late fill: rows %d.., the fill before it running %u us, %u us since the last call%s\n", l.chunk * BB_ROWS,
          l.busy_us, l.gap_us, l.pend ? ", one already waiting" : "");
    }
    t0 = now, bn0 = bm.n, bs0 = bm.sum_us, bb0 = bm.bytes, wn0 = wm.n, ws0 = wm.sum_us, wb0 = wm.bytes, late0 = late_fills;
    bm.max_us = wm.max_us = 0;
  });
  LOG("display: ST7701 480 x 480 ok (set up in %lu ms), RGB 16 MHz from bounce buffers of %d rows, %d band buffers\n",
      (unsigned long)(millis() - t0), BB_ROWS, nbufs);
  return true;
}

#endif  // HALO_BOARD_LCD4B
