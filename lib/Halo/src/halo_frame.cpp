// Движок кадров Halo: по сигналу TE рисует и отправляет только пачки строк с изменениями.
//
//   задача кадров (ядро 1)                         помощник (ядро 0)
//     ждёт TE
//     изменения прошлого update → пачки с изменениями
//     для каждой: буфер (ждёт на семафоре) ───→     правая часть (если пачка широкая): чёрный фон + draw
//                 левая часть: чёрный фон + draw
//                 ждёт помощника ←───              готово
//                 в очередь DMA (не ждёт)
//     ждёт, пока всё уйдёт
//     update(dt) — следующий кадр; изменений нет — следующий кадр не рисуется вовсе
//
// В пачке перерисовывается и уходит не весь круг, а только столбцы с изменениями (объединение по пачке).
//
// Без разрыва (дисплей сам 60 раз в секунду проходит свою память сверху вниз, ~34 мкс на строку, проход начинается
// с концом паузы TE). Строки одного кадра должны попасть в один и тот же проход, есть два способа:
//   * впереди прохода — лёгкий кадр: пишем сразу после TE, каждая пачка успевает раньше, чем до неё дойдёт проход
//     (у верхней строки — 0,6 мс, у нижней — 16 мс). Задержка от расчёта до экрана — меньше;
//   * вслед за проходом — тяжёлый кадр: пачку отправляем, только когда текущий проход её миновал, а следующий
//     покажет кадр целиком. У каждой пачки — почти весь период 16,6 мс, но кадр на экране на проход позже.
// Какой выбрать — по оценке: рисование пачек (по прошлым кадрам) и очередь на шине против срока каждой «впереди».
//
// Ровный шаг движения (как Choreographer в Android: время кадра — от сигнала дисплея, а не от того, когда считали):
//   * время анимации — момент, когда кадр покажет экран: перед update решаем, с какого TE кадр начнётся и пойдёт ли
//     вслед за проходом, и берём начало этого прохода. Раньше брали время самого update — оно гуляет с ценой кадра
//     (лёгкий кадр кончился через 2 мс после TE, тяжёлый — через 10), и шаг движения на экране гулял так же;
//   * способ не меняется посреди движения: переход «впереди → вслед» показывает прошлый кадр дважды, обратный
//     (два кадра в один проход) — один не показывает вовсе. Раз пошли вслед — так и идём, пока движение не встанет
//     (0,1 с без изменений) или кадры долго лёгкие с запасом; тогда обратно «впереди», а чтобы два кадра не попали в
//     один проход, новый ждёт следующего TE (проход тот же, что был бы вслед, — ничего не теряется).
// Замер HaloWatch (лента, бросок): было 10 смен способа на 80 кадров — 6 кадров дважды, 6 ни разу, шаг движения
// гулял до 14 мс (tools/trace.py: «кадры на экране»).
#include "halo_internal.h"
#include "halo_spans.h"
#include <esp_timer.h>

#pragma GCC optimize("O3", "fast-math")

__thread HaloBand hb;
uint32_t hf_finger_t;

static HaloConfig cfg;
static HaloUpdate m_update;
static HaloDraw m_draw;

// ---------- Что изменилось: по пачке — до HALO_SPANS отрезков столбцов (halo_spans.h) ----------
static HaloSpans dmg[HALO_BANDS];  // n = 0 — пачка чистая
static portMUX_TYPE dmg_mux = portMUX_INITIALIZER_UNLOCKED;
static int16_t chord_x0[HALO_BANDS], chord_x1[HALO_BANDS];  // круг экрана в пачке (чётный — нечётный)

static void chords() {
  const float c = (HALO_W - 1) / 2.0f, R = HALO_W / 2.0f;
  for (int b = 0; b < HALO_BANDS; b++) {
    const int y = b * HALO_ROWS, rows = min(HALO_ROWS, HALO_H - y);
    const float dy = (y <= c && y + rows >= c) ? 0 : fminf(fabsf(y - c), fabsf(y + rows - c));
    const float half = halo_sqrt(fmaxf(0, R * R - dy * dy)) + 2;
    chord_x0[b] = max(0, (int)(c - half)) & ~1;
    chord_x1[b] = min(HALO_W - 1, (int)(c + half)) | 1;
    dmg[b].n = 0;
  }
}

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

// ---------- Время анимаций ----------
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

// ---------- Запись кадров (rec): как в AmoledMotion, но кадр — изменения поверх прошлого ----------
// Записи (чётной длины): 'F' 0 номер(2) TE(4) начало_мкс(4) часы_мкс(4) 0(4); 'S' 0 номер(2) y(2) x0(2)
// пикселей(2) слов(2) слова…; 'E' 0 номер(2) кадр_мкс(4) пропущено_TE(2) 0(2). Сжатие: слово с битом 0x8000 —
// столько чёрных пикселей, иначе — столько пикселей следом. На Mac кадр собирается поверх прошлого
static uint8_t *rec_pool[2];
static uint32_t rec_cap, rec_len[2];
static volatile int rec_left;
static volatile bool rec_half, rec_cur, rec_over, rec_first;
static uint16_t rec_no;

static void rec_put(int pool, char kind, const uint32_t *v, int n32, uint16_t extra) {
  uint8_t *h = rec_pool[pool] + rec_len[pool];
  h[0] = kind, h[1] = 0;
  memcpy(h + 2, &rec_no, 2);
  memcpy(h + 4, v, n32 * 4);
  memcpy(h + 4 + n32 * 4, &extra, 2);
  h[6 + n32 * 4] = h[7 + n32 * 4] = 0;
  rec_len[pool] += 8 + n32 * 4;
}

static void __attribute__((noinline, noclone)) rec_band(int pool) {  // во флеше: нужна только при записи
  const int step = rec_half ? 2 : 1;
  const int n = (hb.x1 - hb.x0) / step + 1;
  if (n <= 0) return;
  for (int r = 0; r < hb.rows; r++) {
    const int y = hb.y + r;
    if (rec_half && (y & 1)) continue;
    if (rec_len[pool] + 12 + n * 3 + 8 > rec_cap) {
      rec_over = true;
      return;
    }
    uint16_t *h = (uint16_t *)(rec_pool[pool] + rec_len[pool]);
    uint16_t *o = h + 6;
    const uint16_t *src = hb.buf + r * hb.w + (hb.x0 - hb.bx);
    for (int i = 0; i < n;) {
      int j = i;
      if (!src[i * step]) {
        while (j < n && !src[j * step] && j - i < 0x7FFF) j++;
        *o++ = 0x8000 | (j - i);
      } else {
        uint16_t *cnt = o++;
        while (j < n && src[j * step] && j - i < 0x7FFF) *o++ = src[j * step], j++;
        *cnt = j - i;
      }
      i = j;
    }
    const int words = o - (h + 6);
    h[0] = 'S', h[1] = rec_no, h[2] = y, h[3] = hb.x0, h[4] = n, h[5] = words;
    rec_len[pool] += 12 + words * 2;
  }
}

static void cmd_rec(const char *a) {
  if (!strncmp(a, "dump", 4)) {
    rec_left = 0;
    delay(40);
    if (!rec_pool[0] || !rec_pool[1]) rec_len[0] = rec_len[1] = 0;
    uint32_t sum = 0;
    for (int p = 0; p < 2; p++)
      for (uint32_t i = 0; i < rec_len[p]; i++) sum += rec_pool[p][i];
    halo_serial_binary(true);
    // 6-е поле 1: кадры — изменения поверх прошлого (record.py собирает их по очереди)
    Serial.printf("REC %lu %lu %d %u %d 1\n", (unsigned long)rec_len[0], (unsigned long)rec_len[1], rec_half ? 2 : 1,
                  (unsigned)rec_no, HALO_W);
    for (int p = 0; p < 2; p++)
      for (uint32_t off = 0; off < rec_len[p];) off += Serial.write(rec_pool[p] + off, min<uint32_t>(rec_len[p] - off, 4096));
    Serial.printf("\nREC END %lu\n", (unsigned long)sum);
    halo_serial_binary(false);
    return;
  }
  if (!strncmp(a, "free", 4)) {
    rec_left = 0;
    delay(40);
    for (auto &p : rec_pool) free(p), p = nullptr;
    rec_cap = rec_len[0] = rec_len[1] = rec_no = 0;
    return;
  }
  int n = 0;
  float mb = 0;
  char mode[8] = "half";
  if (sscanf(a, "%d %7s %f", &n, mode, &mb) < 1 || n <= 0) {
    Serial.println("rec N [half|full] [MB] | rec dump | rec free");
    return;
  }
  rec_left = 0;
  delay(40);
  const uint32_t free_ps = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) + rec_cap * 2;
  uint32_t total = mb > 0 ? (uint32_t)(mb * 1048576) : 4u << 20;
  total = min<uint32_t>(total, free_ps > (3u << 19) ? free_ps - (3u << 19) : 0) & ~15u;
  if (total / 2 != rec_cap) {
    for (auto &p : rec_pool) free(p), p = nullptr;
    rec_cap = total / 2;
    for (auto &p : rec_pool) p = (uint8_t *)heap_caps_malloc(rec_cap, MALLOC_CAP_SPIRAM);
    if (!rec_pool[0] || !rec_pool[1]) {
      for (auto &p : rec_pool) free(p), p = nullptr;
      rec_cap = 0;
      LOG("rec: no memory\n");
      return;
    }
  }
  rec_len[0] = rec_len[1] = 0;
  rec_no = 0;
  rec_over = false;
  rec_half = strcmp(mode, "full") != 0;
  rec_first = true;
  rec_left = n;
  halo_damage_all();  // первый кадр записи — целиком, дальше — изменения
  LOG("rec: %d frames, %s, %lu KB\n", n, rec_half ? "half" : "full", (unsigned long)(rec_cap * 2 / 1024));
}

// ---------- Помощник на ядре 0 ----------
static SemaphoreHandle_t half_go, half_done;
static volatile int job_y, job_rows, job_x0, job_x1, job_bx, job_w;
static uint16_t *volatile job_buf;
static volatile uint32_t job_us;

static void clear_part() {  // чёрный фон своей части пачки
  const uint32_t c0 = hp_on ? esp_cpu_get_cycle_count() : 0;
  for (int r = 0; r < hb.rows; r++) memset(hb.buf + r * hb.w + (hb.x0 - hb.bx), 0, (hb.x1 - hb.x0 + 1) * 2);
  if (hp_on) hp_cyc[xPortGetCoreID()][HP_CLEAR] += esp_cpu_get_cycle_count() - c0;
}

static void helper_task(void *) {
  for (;;) {
    xSemaphoreTake(half_go, portMAX_DELAY);
    const uint32_t t0 = trace_now();
    hb = {job_buf, job_y, job_rows, job_x0, job_x1, job_bx, job_w};
    clear_part();
    m_draw();
    if (rec_cur) rec_band(0);
    job_us = trace_now() - t0;
    trace_span(TR_BAND, t0, job_y / HALO_ROWS, t0 + job_us);
    xSemaphoreGive(half_done);
  }
}

// ---------- Скриншот и замер: весь кадр одним ядром в память ----------
static uint16_t *volatile shot_buf;
static volatile bool shot_req, shot_ready, bench_ring_req;

static void render_full(uint16_t *dst, bool swap) {  // dst — 466×466 в PSRAM
  // Пачка — во внутренней памяти, как у настоящего кадра: иначе замер bench врёт (запись в PSRAM медленная)
  static uint16_t *tmp = (uint16_t *)heap_caps_malloc(HALO_ROWS * HALO_W * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!tmp) return;
  const HaloBand saved = hb;
  for (int y = 0; y < HALO_H; y += HALO_ROWS) {
    hb = {tmp, y, min(HALO_ROWS, HALO_H - y), 0, HALO_W - 1, 0, HALO_W};
    clear_part();
    m_draw();
    if (!dst) continue;
    for (int i = 0; i < hb.rows * HALO_W; i++) dst[y * HALO_W + i] = swap ? __builtin_bswap16(tmp[i]) : tmp[i];
  }
  hb = saved;
}

// ---------- Задача кадров ----------
struct Stats {
  uint32_t ticks, drawn, bands, px, sum_us, max_us, missed, update_us, update_max, chased, late;
};
static Stats stats, report;
static uint32_t hp_report[2][HP_PARTS];
static volatile bool report_ready;
static volatile int bright_req = -1, power_req = -1;
static volatile bool disp_on = true, boot_cmd, boot_long_cmd;
// halo_heavy: первый кадр, который что-то нарисует, — вслед за проходом. Ждёт такого кадра до 4 тиков: в первом кадре
// движения на экране может ещё ничего не измениться (экран погоды поверх циферблата начинает с прозрачности 0)
static volatile uint8_t heavy_req;
static int16_t split[HALO_BANDS];
// Когда граница пачки делилась в последний раз (номер кадра): старая граница — от другой картинки (экран стоял, потом
// сменился целиком: первый кадр перехода Settings — правое ядро 18 мс, левое 6), начинать с середины
static uint32_t split_seq[HALO_BANDS], frame_seq;
// Сколько рисовалась каждая пачка в прошлых кадрах, мкс: середина с кольцами и цифрами в разы дороже краёв — по
// среднему выбор ошибался. Только рисование: ожидание буфера (очередь на шине) оценка считает сама — вслед за
// проходом его нет, и по такой цене кадр «впереди» казался легче, чем есть
static float band_cost[HALO_BANDS];
static float us_per_px;  // передача пикселя по шине: QSPI — 4 бита за такт, 2 байта на пиксель

// Успеет ли кадр впереди прохода: каждая пачка должна уйти на шину раньше, чем до неё дойдёт проход. Пачка ждёт
// свободный буфер (рисование обгоняет шину не больше чем на столько пачек, сколько буферов), рисуется (цена по
// прошлым кадрам × k) и встаёт в очередь на шину. start — сколько мкс прошло от TE до начала кадра.
// behind — период: кадр вслед за проходом этого TE (он уже идёт): пачка уходит, когда проход её миновал (следующая
// рисуется после этого — так идёт цикл кадра), и должна уйти раньше, чем до неё дойдёт следующий проход
// Отрезок пачки — свой буфер и своё окно на шине; цена пачки делится между её отрезками по ширине
static bool fits_ahead(const HaloSpans *fd, float start, float blank, float row_us, float k, float behind = 0) {
  const int nb = hd_buffers();
  float draw = start + 100, bus = draw, ends[4] = {};  // ends — когда освободится буфер каждой из последних пачек
  int n = 0;
  for (int b = 0; b < HALO_BANDS; b++) {
    int xs0[HALO_SPANS], xs1[HALO_SPANS], m = 0, tw = 0;
    for (int i = 0; i < fd[b].n; i++) {
      const int x0 = max(fd[b].x0[i] & ~1, (int)chord_x0[b]), x1 = min(fd[b].x1[i] | 1, (int)chord_x1[b]);
      if (x0 <= x1) xs0[m] = x0, xs1[m] = x1, m++, tw += x1 - x0 + 1;
    }
    const int rows = min(HALO_ROWS, HALO_H - b * HALO_ROWS);
    for (int i = 0; i < m; i++) {
      const int w = xs1[i] - xs0[i] + 1;
      if (n >= nb) draw = fmaxf(draw, ends[n % nb]);
      draw += band_cost[b] * k * w / tw + 20;
      float go = fmaxf(draw, bus);
      if (behind > 0) go = draw = fmaxf(go, blank + (b * HALO_ROWS + rows) * row_us + 40);
      if (go + 250 > behind + blank + b * HALO_ROWS * row_us) return false;
      bus = go + w * rows * us_per_px + 10;
      ends[n++ % nb] = bus;
    }
  }
  return true;
}

// Ждать до момента t (мкс): долго — спим, остаток — крутимся
// Короткое ожидание (вслед за проходом — до каждой пачки, ~0,5 мс) будит таймер: ядро в это время свободно (раньше
// оно крутилось — искорки по экрану держали кадры вслед за проходом, и ядро 1 было занято на 28 %, рисуя 8 %).
// Последние 60 мкс — крутимся, точно как раньше; таймер опоздал — пачка уйдёт чуть позже: не страшно, следующий проход
// до неё ещё далеко (раньше — нельзя: разрыв)
static TaskHandle_t wait_task;
static esp_timer_handle_t wait_timer;
static void wait_wake(void *) {
  xTaskNotifyGive(wait_task);
}

static void wait_until(uint32_t t) {
  int32_t left = (int32_t)(t - trace_now());
  if (left > 2500) vTaskDelay(pdMS_TO_TICKS(left / 1000 - 1)), left = (int32_t)(t - trace_now());
  if (left > 150 && wait_timer) {
    ulTaskNotifyTake(pdTRUE, 0);  // (старый сигнал — прочь)
    if (esp_timer_start_once(wait_timer, left - 60) == ESP_OK) ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5));
  }
  while ((int32_t)(t - trace_now()) > 0) {
  }
}

static void bench_ring();

static void frame_task(void *) {
  wait_task = xTaskGetCurrentTaskHandle();
  const esp_timer_create_args_t wa = {wait_wake, nullptr, ESP_TIMER_TASK, "halo_wait", false};
  if (esp_timer_create(&wa, &wait_timer) != ESP_OK) wait_timer = nullptr;
  for (auto &c : band_cost) c = 300;
  us_per_px = 4e6f / cfg.bus_hz;
  chords();
  halo_damage_all();
  halo_subpixel = false;
  m_update(1.0f / 60);
  uint32_t report_at = millis() + cfg.report_ms;
  bool frame_new = true;
  uint32_t frame_clock = 0;
  // План следующего кадра (решается перед update): с какого TE начать, идти ли вслед за проходом, какой проход его
  // покажет и когда. Проход s — тот, что начинается после TE номер s
  uint32_t plan_te = hd_te_count() + 1, plan_show = plan_te, show_t = trace_now(), last_show = 0;
  bool plan_chase = false, sticky = false;
  int roomy = 0;  // сколько кадров подряд, идущих вслед, легко успели бы впереди
  int still = 0;  // сколько тиков подряд ничего не менялось
  uint32_t tick_te = 0;
  float lead_avg = 16000;  // от расчёта кадра до его показа, мкс, в среднем
  for (;;) {
    // Ждём свой TE; если он уже пришёл, пока рисовался прошлый кадр (тяжёлый, вслед за проходом), — начинаем сразу
    hd_wait_te(plan_te, 50);
    const uint32_t te = hd_te_count();
    if (!stats.ticks && !report.ticks) hc_stage(6);  // первый кадр
    stats.ticks += te != tick_te;  // кадр вслед за уже идущим проходом — второй в том же периоде TE
    tick_te = te;
    if (bright_req >= 0) hd_set_brightness(bright_req), bright_req = -1;
    if (power_req >= 0) hd_set_power(power_req), power_req = -1;

    HaloSpans fd[HALO_BANDS];
    portENTER_CRITICAL(&dmg_mux);
    memcpy(fd, dmg, sizeof(fd));
    for (int b = 0; b < HALO_BANDS; b++) dmg[b].n = 0;
    portEXIT_CRITICAL(&dmg_mux);
    for (auto &d : fd) halo_spans_sort(d);

    const uint32_t t0 = trace_now();
    int bands = 0, px = 0;
    // Впереди прохода или вслед за ним: как решено в плане, но если кадр впереди не успеет — вслед (иначе разрыв)
    const uint32_t te_t = hd_te_rise(), blank = hd_te_blank(), period = hd_te_period() ? hd_te_period() : 16650;
    const float row_us = (float)(period - blank) / HALO_H;
    const uint32_t scan = te_t + blank;  // начало прохода после этого TE
    const bool chase = plan_chase || heavy_req || !fits_ahead(fd, t0 - te_t, blank, row_us, 1.15f);
    const bool light = chase && fits_ahead(fd, 0, blank, row_us, 1.5f);  // успел бы впереди с запасом
    rec_cur = rec_left > 0 && (frame_new || rec_first) && !rec_over && rec_len[1] + 64 < rec_cap;
    frame_seq++;
    int carry = -1;  // граница последней поделённой пачки этого кадра (после поправки по её замеру)
    for (int b = 0; b < HALO_BANDS; b++) {
      // Отрезки пачки внутри круга; её цена — все они
      int xs0[HALO_SPANS], xs1[HALO_SPANS], m = 0;
      for (int sp = 0; sp < fd[b].n; sp++) {
        const int x0 = max(fd[b].x0[sp] & ~1, (int)chord_x0[b]), x1 = min(fd[b].x1[sp] | 1, (int)chord_x1[b]);
        if (x0 <= x1) xs0[m] = x0, xs1[m] = x1, m++;
      }
      if (!m) continue;
      if (!bands && rec_cur) {
        const uint32_t v[3] = {te, t0, frame_clock};
        rec_put(1, 'F', v, 3, 0);
        rec_first = false;
      }
      const int y = b * HALO_ROWS, rows = min(HALO_ROWS, HALO_H - y);
      uint32_t band_us = 0;
      for (int sp = 0; sp < m; sp++) {
        const int x0 = xs0[sp], x1 = xs1[sp], w = x1 - x0 + 1;
        const uint32_t c0 = trace_now();
        // Два узких отрезка подряд — по одному на ядро: свой буфер каждому, на шину — по порядку (до 2026-11-04 отрезок
        // в пачке был один, узкие появились с отрезками — и всё рисовало одно ядро: искорки по экрану — 4 % и 29 %)
        const bool pair = cfg.two_cores && w < 160 && sp + 1 < m && xs1[sp + 1] - xs0[sp + 1] + 1 < 160 && hd_buffers() >= 2;
        if (pair) {
          const int x2 = xs0[sp + 1], w2 = xs1[sp + 1] - x2 + 1;
          uint16_t *buf, *buf2;
          hd_buffer2(&buf, &buf2);
          job_buf = buf2, job_y = y, job_rows = rows, job_x0 = x2, job_x1 = xs1[sp + 1], job_bx = x2, job_w = w2;
          xSemaphoreGive(half_go);
          hb = {buf, y, rows, x0, x1, x0, w};
          clear_part();
          m_draw();
          if (rec_cur) rec_band(1);
          const uint32_t c1 = trace_now();
          trace_span(TR_BAND, c0, b, c1);
          xSemaphoreTake(half_done, portMAX_DELAY);
          const uint32_t c2 = trace_now();
          if (c2 - c1 >= 5) trace_span(TR_WAIT_HALF, c1, b, c2);
          band_us += c2 - c0;
          if (chase) wait_until(scan + (uint32_t)((y + rows) * row_us) + 40);  // текущий проход миновал пачку
          hd_send(x0, y, w, rows);
          hd_send(x2, y, w2, rows);
          bands += 2, px += (w + w2) * rows;
          sp++;
          continue;
        }
        uint16_t *buf = hd_buffer();
        if (cfg.two_cores && w >= 160) {  // широкий отрезок — пополам; граница — туда, где работы меньше
          // Своя граница из прошлых кадров — если свежая (делилась в одном из 3 прошлых). Нет — граница пачки выше в
          // этом же кадре: картинка по высоте похожа. С середины первый кадр открытия WearLab №67 (новый экран справа,
          // всё полупрозрачное) — правое ядро 26 мс, левое 8: кадр 28–30 мс и изредка разрыв
          const bool fresh = frame_seq - split_seq[b] <= 3;
          int sx = fresh && split[b] > x0 + 32 && split[b] < x1 - 32 ? split[b]
                   : carry >= 0                                       ? constrain(carry, x0 + 32, x1 - 32) & ~1
                                                                      : ((x0 + x1) / 2) & ~1;
          job_buf = buf, job_y = y, job_rows = rows, job_x0 = sx, job_x1 = x1, job_bx = x0, job_w = w;
          xSemaphoreGive(half_go);
          hb = {buf, y, rows, x0, sx - 1, x0, w};
          clear_part();
          m_draw();
          if (rec_cur) rec_band(1);
          const uint32_t c1 = trace_now();
          trace_span(TR_BAND, c0, b, c1);
          xSemaphoreTake(half_done, portMAX_DELAY);
          const uint32_t c2 = trace_now();
          if (c2 - c1 >= 5) trace_span(TR_WAIT_HALF, c1, b, c2);
          const int left = c1 - c0, right = job_us, tot = left + right;
          const int step = tot > 0 ? constrain((int)((left - right) * (float)w / (2.0f * tot)), -64, 64) : 0;
          split[b] = constrain(sx - step, x0 + 32, x1 - 32) & ~1, split_seq[b] = frame_seq, carry = split[b];
        } else {
          hb = {buf, y, rows, x0, x1, x0, w};
          clear_part();
          m_draw();
          if (rec_cur) rec_band(1);
          trace_span(TR_BAND, c0, b);
        }
        band_us += trace_now() - c0;
        if (chase) wait_until(scan + (uint32_t)((y + rows) * row_us) + 40);  // текущий проход миновал пачку
        hd_send(x0, y, w, rows);
        bands++, px += w * rows;
      }
      band_cost[b] = band_cost[b] * 0.5f + 0.5f * (float)band_us;
    }
    uint32_t frame_us = 0, missed = 0;
    if (heavy_req) heavy_req = bands ? 0 : heavy_req - 1;
    if (bands) {
      const uint32_t f0 = trace_now();
      hd_wait_idle();
      trace_span(TR_FINISH, f0);
      frame_us = trace_now() - t0;
      missed = (int32_t)(te - plan_te) > 0 ? te - plan_te : 0;  // кадр начался позже своего TE
      trace_span(TR_FRAME, t0, min(missed, (uint32_t)255) | (chase ? 256 : 0), t0 + frame_us);
      stats.drawn++, stats.bands += bands, stats.px += px, stats.sum_us += frame_us, stats.missed += missed;
      stats.chased += chase;
      stats.max_us = max(stats.max_us, frame_us);
      // Показан не тем проходом, по которому считалось его время: впереди не успел (прошлый кадр — дважды) или начат
      // поздно
      last_show = te + chase;
      stats.late += last_show != plan_show;
      // Вслед за проходом — до конца движения; обратно впереди — только после полсекунды лёгких кадров подряд
      roomy = light ? roomy + 1 : 0;
      sticky = chase && roomy < 30;
      still = 0;
    } else if (++still >= 6) {
      // Ничего не движется 0,1 с — сменить способ можно незаметно. Пустой тик-другой — ещё не конец движения: лента
      // встала, а время сверху начинает возвращаться через 1–3 тика (первые шаги — меньше пикселя), пружина у края
      // доходит рывками по пикселю. Раньше способ сбрасывался на первом же пустом тике: следующий кадр планировался
      // «впереди», а рисовался всё равно вслед (верх экрана впереди не успеть) — показан на проход позже своего
      // времени, а следующий за ним шагнул на два кадра (WearLab, №7: «позже на проход» 3 на бросок)
      sticky = false, roomy = 0;
    }
    if (rec_cur && bands) {
      if (!rec_over) {
        const uint32_t v[1] = {frame_us};
        rec_put(1, 'E', v, 1, missed);
        rec_no++;
      }
      if (rec_over || --rec_left <= 0) {
        if (rec_over) LOG("rec: memory is full after %u frames\n", (unsigned)rec_no);
        rec_left = 0;
      }
    }
    rec_cur = false;
    hp_frame_end();  // скетч попросил уснуть (halo_sleep) — здесь: кадр уже на экране, шина свободна

    // План следующего кадра: начать с ближайшего TE (уже пришёл — сразу), способ — как сейчас. Два кадра в один проход
    // не ставим: после «вслед» кадр «впереди» ждёт ещё один TE
    const uint32_t p0 = trace_now();
    uint32_t te_now, te_now_t;
    hd_te_snapshot(&te_now, &te_now_t);
    plan_chase = sticky;
    plan_te = te_now != te ? te_now : te_now + 1;
    if ((int32_t)(plan_te + plan_chase - last_show) <= 0) plan_te = last_show + 1 - plan_chase;
    plan_show = plan_te + plan_chase;
    // Время анимации — начало прохода, который покажет кадр. Нет сигнала TE (панель молчит) — часы платы, как раньше
    const uint32_t te_per = hd_te_period(), show_prev = show_t;
    show_t = te_per ? te_now_t + (plan_show - te_now) * te_per + hd_te_blank() : p0;
    trace_mark(TR_CLOCK, (uint16_t)constrain((int32_t)(show_t - p0), 0, 65535), plan_chase);
    // Палец — за (среднее «расчёт → показ» + 8 мс) до показа: шаг ровно как у кадров, и почти всегда между двумя
    // отчётами тача (они раз в ~10 мс, не в такт экрану). Среднее, а не текущее: иначе шаг гулял бы с ценой кадра
    lead_avg += (constrain((int32_t)(show_t - p0), 0, 50000) - lead_avg) * 0.05f;
    hf_finger_t = show_t - (uint32_t)(lead_avg + 8000);
    // Виртуальное время — в паузе только по шагам (ровно 1/60 с)
    float dt = constrain((int32_t)(show_t - show_prev) / 1e6f, 0.001f, 0.1f);
    frame_new = !vt_paused || vt_steps > 0;
    if (vt_paused && frame_new) vt_steps--, trace_mark(TR_STEP, vt_steps);
    if (vt_paused || vt_fixed) dt = 1.0f / 60;
    if (frame_new) {
      m_clock += (uint32_t)(dt * 1e6f + 0.5f);
      frame_clock = (uint32_t)m_clock;
      halo_subpixel = false;  // каждый кадр заново: кому нужно — ставит в update
      m_update(vt_paused || vt_fixed ? dt : dt * slowmo);
      const uint32_t u = trace_now() - p0;
      trace_span(TR_PREPARE, p0, 0, p0 + u);
      stats.update_us += u, stats.update_max = max(stats.update_max, u);
    }
    // Кадр, задуманный впереди следующего прохода, впереди не успеет (подсказка halo_heavy или оценка по ценам пачек):
    // начать его сразу — вслед за проходом, который идёт сейчас. Покажет его тот же проход, что и по плану: время
    // анимации верное, прошлый кадр не висит дважды. Раньше он ждал TE и шёл вслед уже за следующим проходом — на
    // проход позже. Так было в начале каждого тяжёлого движения, перед которым что-то менялось впереди (нажатие на
    // карточку ленты — потом лента поехала: «показан дважды 1» на бросок), а следующий кадр шагал на два
    if (!plan_chase && plan_te == te_now + 1 && te_per) {
      HaloSpans dd[HALO_BANDS];
      portENTER_CRITICAL(&dmg_mux);
      memcpy(dd, dmg, sizeof(dd));
      portEXIT_CRITICAL(&dmg_mux);
      bool any = false;
      for (int b = 0; b < HALO_BANDS; b++) any |= dd[b].n > 0;
      const uint32_t bl = hd_te_blank();
      const float rus = (float)(te_per - bl) / HALO_H;
      uint32_t te_c, te_c_t;
      hd_te_snapshot(&te_c, &te_c_t);
      // Цены пачек могут быть от прошлой картинки (подсказку для того и зовут) — оценка вслед с запасом 1,5
      if (any && te_c == te_now && (heavy_req || !fits_ahead(dd, 30, bl, rus, 1.15f)) &&
          fits_ahead(dd, (float)(trace_now() - te_c_t) + 50, bl, rus, 1.5f, (float)te_per))
        plan_te = te_now, plan_chase = true;
    }
    if (shot_req) {
      render_full(shot_buf, true);
      shot_req = false, shot_ready = true;
    }
    if (bench_ring_req) bench_ring(), bench_ring_req = false;
    if (cfg.report_ms && (int32_t)(millis() - report_at) >= 0) {
      // После сна часы прыгают вперёд: пропущенные строки не догонять (было бы по строке в кадр)
      report_at = (int32_t)(millis() - report_at) >= cfg.report_ms ? millis() + cfg.report_ms : report_at + cfg.report_ms;
      if (!report_ready) {
        report = stats, report_ready = true;
        memcpy(hp_report, hp_cyc, sizeof(hp_report));
      }
      stats = {};
      memset(hp_cyc, 0, sizeof(hp_cyc));
    }
    halo_trace_pump();
  }
}

// ---------- Команды ----------
static void cmd_shot(const char *) {
  if (!shot_buf) shot_buf = (uint16_t *)heap_caps_malloc(HALO_W * HALO_H * 2, MALLOC_CAP_SPIRAM);
  if (!shot_buf) return;
  shot_ready = false, shot_req = true;
  const uint32_t t0 = millis();
  while (!shot_ready && millis() - t0 < 1000) delay(2);
  if (shot_ready) hc_send_image(shot_buf, HALO_W, HALO_H);
}

// bench ring — одно кольцо (r 100, толщина 20) во внутреннюю пачку в разных вариантах: тактов на пиксель кольца
static void bench_ring() {
  static uint16_t *tmp = (uint16_t *)heap_caps_malloc(HALO_ROWS * HALO_W * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  const HaloBand saved = hb;
  const float area = 2 * HALO_PI * 100 * 20;
  struct V {
    const char *name;
    float frac;
    uint32_t track;
  } vs[] = {{"full", 1, 0}, {"arc 0.7", 0.7f, 0}, {"arc 0.7 + track", 0.7f, 0x303030}, {"arc 0.3 + track", 0.3f, 0x303030}};
  for (auto &v : vs) {
    uint32_t cyc = 0;
    for (int y = 0; y < HALO_H; y += HALO_ROWS) {
      hb = {tmp, y, min(HALO_ROWS, HALO_H - y), 0, HALO_W - 1, 0, HALO_W};
      const uint32_t c0 = esp_cpu_get_cycle_count();
      halo_ring(233, 233, 100, 20, v.frac, 0xFF3060, v.track);
      cyc += esp_cpu_get_cycle_count() - c0;
    }
    LOG("bench ring %s: %.2f ms, %.0f cycles/px\n", v.name, cyc / 240000.0f, cyc / area);
  }
  struct R {
    const char *name;
    float x0, y0, x1, y1, r, a;
  } rs[] = {{"panel 466x466 r120", 0, -100, 466, 366, 120, 1}, {"row 300x62 r31", 83, 200, 383, 262, 31, 1},
            {"row 300x62 r31 alpha 0.5", 83, 200, 383, 262, 31, 0.5f}};
  for (auto &v : rs) {
    uint32_t cyc = 0;
    for (int y = 0; y < HALO_H; y += HALO_ROWS) {
      hb = {tmp, y, min(HALO_ROWS, HALO_H - y), 0, HALO_W - 1, 0, HALO_W};
      const uint32_t c0 = esp_cpu_get_cycle_count();
      halo_rrect(v.x0, v.y0, v.x1, v.y1, v.r, 0x202024, v.a);
      cyc += esp_cpu_get_cycle_count() - c0;
    }
    const float area = (v.x1 - v.x0) * (fminf(v.y1, 466) - fmaxf(v.y0, 0));
    LOG("bench rrect %s: %.2f ms, %.1f cycles/px\n", v.name, cyc / 240000.0f, cyc / area);
  }
  hb = saved;
}

static volatile bool bench_req;
static void cmd_bench(const char *a) {  // весь кадр одним ядром, без вывода — сколько стоит рисование
  if (!strcmp(a, "ring")) {  // в задаче кадров, между кадрами: её никто не перебивает
    bench_ring_req = true;
    return;
  }
  static uint16_t *dummy;
  vt_paused = true;  // состояние не меняется, пока меряем
  delay(40);
  const uint32_t t0 = trace_now();
  for (int k = 0; k < 5; k++) render_full(dummy, false);
  LOG("bench: full frame on one core %.2f ms\n", (trace_now() - t0) / 5000.0f);
  vt_paused = false;
}

void halo_heavy() {
  heavy_req = 4;
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

// Кнопка BOOT — GPIO0 (нажата — 0): нажатие засчитывается, когда она нажата два кадра подряд после двух отпущенных
// (дребезг контактов — короче кадра). Команда boot — то же нажатие из порта. Держат дольше HALO_BOOT_LONG_MS — ещё и
// долгое (halo_boot_long), и у нажатия, которым её разбудили, тоже
static bool boot_swallow;  // BOOT разбудила плату (halo_power.cpp): это нажатие уже «засчитано»
static bool boot_down, boot_long_done = true;
static uint32_t boot_down_ms;

void hf_swallow_boot() {
  boot_swallow = true;
}

bool halo_boot_click() {
  static uint8_t hist = 0xFF;
  hist = hist << 1 | (gpio_get_level(GPIO_NUM_0) ? 1 : 0);
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
  if (!hd_begin(cfg.bus_hz, cfg.buffers, cfg.brightness)) return hc_boot_done(), false;
  hc_stage(4);
  if (cfg.touch) ht_begin();
  pinMode(0, INPUT_PULLUP);  // кнопка BOOT
  hc_stage(5);
  hc_boot_done();
  hu_begin();
  half_go = xSemaphoreCreateBinary();
  half_done = xSemaphoreCreateBinary();
  halo_trace_name(TR_FRAME, "frame", 'X');
  halo_trace_name(TR_PREPARE, "prepare", 'X');
  halo_trace_name(TR_BAND, "band", 'X');
  halo_trace_name(TR_WAIT_HALF, "wait right half", 'X');
  halo_trace_name(TR_FINISH, "last band out", 'X');
  halo_trace_name(TR_REC, "rec", 'X');
  halo_trace_name(TR_STEP, "step", 'i');
  halo_trace_name(TR_CLOCK, "clock", 'i');  // перед update: через сколько мкс экран покажет кадр; 1 — вслед за проходом
  halo_command("shot", cmd_shot);
  halo_command("bench", cmd_bench);
  halo_command("rec", cmd_rec);
  halo_command("pause", [](const char *) { vt_steps = 0, vt_paused = true; });
  halo_command("resume", [](const char *) { vt_steps = 0, vt_paused = false; });
  halo_command("step", [](const char *a) { vt_steps += max(1, atoi(a)), vt_paused = true; });
  halo_command("clock", [](const char *a) { vt_fixed = !strcmp(a, "fixed"); });
  halo_command("slow", [](const char *a) { slowmo = 1 / constrain(atof(a), 1.0f, 100.0f); });
  halo_command("bright", [](const char *a) { halo_brightness(atoi(a)); });
  halo_command("boot", [](const char *a) { (strstr(a, "long") ? boot_long_cmd : boot_cmd) = true; });
  halo_command("redraw", [](const char *) { halo_damage_all(); });
  halo_command("prof", [](const char *) { hp_on = !hp_on; });
  if (cfg.two_cores) xTaskCreatePinnedToCore(helper_task, "halo_helper", 4096, nullptr, 10, nullptr, 0);
  xTaskCreatePinnedToCore(frame_task, "halo_frames", 6144, nullptr, 5, nullptr, 1);
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
      LOG("%s: %.0f fps drawn of %.0f (%lu%% behind the scan), frame %.2f ms (max %.2f), %.1f bands %.0f%% of "
          "screen, update %.2f ms (max %.2f), missed %lu, late %lu\n", cfg.name, s.drawn / sec, s.ticks / sec,
          (unsigned long)(100 * s.chased / s.drawn), s.sum_us / 1000.0f / s.drawn, s.max_us / 1000.0f,
          (float)s.bands / s.drawn, 100.0f * s.px / s.drawn / (HALO_W * HALO_H),
          s.update_us / 1000.0f / max((uint32_t)1, s.ticks), s.update_max / 1000.0f, (unsigned long)s.missed,
          (unsigned long)s.late);
    else
      LOG("%s: idle (%.0f ticks/s), update %.2f ms\n", cfg.name, s.ticks / sec, s.update_us / 1000.0f / max((uint32_t)1, s.ticks));
    if (hp_on && s.drawn) {  // мс на нарисованный кадр: ядро 1 / ядро 0
      static const char *const N[HP_PARTS] = {"text", "mask", "image", "rrect", "circle", "ring", "arc", "edge", "pixels", "clear", "-"};
      char line[280];
      for (int half = 0; half < 2; half++) {  // (двумя строками: длинную LOG обрезал — после «pixels» было не видно)
        int n = snprintf(line, sizeof(line), "%s prof ms/frame (core1/core0):", cfg.name);
        for (int i = half ? 6 : 0; i < (half ? HP_PARTS - 1 : 6); i++)
          n += snprintf(line + n, sizeof(line) - n, " %s %.2f/%.2f", N[i], hp_report[1][i] / 240000.0f / s.drawn,
                        hp_report[0][i] / 240000.0f / s.drawn);
        LOG("%s\n", line);
      }
    }
    report_ready = false;
  }
  delay(5);
}
