// The S3 board's own declarations: its pins and what its files give each other. Other boards never see this file.
#pragma once

#include "halo_internal.h"
#include "../esp32_common/halo_esp32.h"

// Пины платы (схема Waveshare ESP32-S3-Touch-AMOLED-1.75C)
#define HALO_PIN_SD0 4
#define HALO_PIN_SD1 5
#define HALO_PIN_SD2 6
#define HALO_PIN_SD3 7
#define HALO_PIN_SCLK 38
#define HALO_PIN_CS 12
#define HALO_PIN_RST 1
#define HALO_PIN_TE 13
#define HALO_PIN_SDA 15
#define HALO_PIN_SCL 14
#define HALO_PIN_TP_INT 11
#define HALO_PIN_TP_RST 2
#define HALO_PIN_BOOT 0
#define HALO_PIN_IMU_INT1 21  // QMI8658 INT1 (сеть QMI_INT1 на схеме; INT2 не подведён)
// Звук: I2S к кодеку ES8311 (динамик) и АЦП ES7210 (микрофоны) — одна шина, и усилитель динамика
#define HALO_PIN_MCLK 16
#define HALO_PIN_BCLK 9
#define HALO_PIN_LRCK 45
#define HALO_PIN_DOUT 8
#define HALO_PIN_DIN 10  // ← ES7210 SDOUT1 (микрофоны 1 и 2 — левый и правый)
#define HALO_PIN_PA 46

// ---------- Дисплей (halo_display.cpp) ----------
bool hd_begin(uint32_t hz, int buffers, uint8_t brightness);
uint16_t *hd_buffer();                       // свободный DMA-буфер (ждёт, если все в очереди)
void hd_buffer2(uint16_t **a, uint16_t **b);  // два сразу — следующий и за ним (уходят двумя hd_send по порядку)
void hd_send(int x0, int y, int w, int rows);  // буфер, выданный hd_buffer, — в очередь (не ждёт)
void hd_wait_idle();                         // всё ушло на дисплей
bool hd_wait_te(uint32_t count, uint32_t timeout_ms);  // ждать, пока счётчик TE дойдёт до count (дошёл — сразу)
void hd_te_snapshot(uint32_t *count, uint32_t *rise);   // номер последнего TE и его время — согласованной парой
uint32_t hd_te_count();
uint32_t hd_te_period();
uint32_t hd_te_rise();
uint32_t hd_te_blank();                    // пауза TE: с её концом начинается проход по строкам
void hd_set_brightness(uint8_t level);       // между кадрами (зовёт движок)
void hd_set_power(bool on);                  // панель видна / погашена — тоже между кадрами
int hd_buffers();
void hd_state(int *fly, int *queued, int *free_bufs);  // очередь вывода — для sys (кто чего ждёт)
void hd_regs(char *out, int n);  // регистры канала DMA и SPI2 — для sys (разбор зависаний)

// Шина I2C тача (halo_touch.cpp): ещё одно устройство на ней (400 кГц), nullptr — не ответило или шины нет
struct i2c_master_dev_t;
i2c_master_dev_t *ht_i2c_device(uint8_t addr);
// Шина занята: задача тача держит её на время своих обменов, сон — на время сна (не уснуть посреди обмена)
void ht_bus_lock();
void ht_bus_unlock();
void ht_kick();  // прочитать отчёт тача сейчас (его прерывание могло быть выключено — сон)

// ---------- Сон и кнопки (halo_power.cpp, halo_frame.cpp) ----------
void hp_frame_end();  // движок, когда кадр уже на экране: уснуть, если просили; «притворяясь» — ждать пробуждения
void hf_swallow_boot();   // нажатие BOOT, которое разбудило: не нажатие (halo_frame.cpp)

// ---------- Тач (halo_touch.cpp) ----------
bool ht_begin();
