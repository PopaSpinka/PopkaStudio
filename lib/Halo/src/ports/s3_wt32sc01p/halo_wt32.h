// The WT32-SC01 Plus board's own declarations: its pins and what its files give each other. Other boards never see
// this file.
#pragma once

#include "halo_internal.h"
#include "../esp32_common/halo_esp32.h"

// The board's pins (Wireless-Tag's WT32-SC01 Plus datasheet)
// LCD: ST7796, 8-bit parallel (8080). CS is tied low on the board, RD high. RST is shared with the touch chip
#define HALO_PIN_LCD_BL 45
#define HALO_PIN_LCD_RST 4
#define HALO_PIN_LCD_DC 0   // (also the BOOT strap: there is no BOOT button to read while the screen works)
#define HALO_PIN_LCD_WR 47
#define HALO_PIN_TE 48
#define HALO_PIN_LCD_D0 9
#define HALO_PIN_LCD_D1 46
#define HALO_PIN_LCD_D2 3
#define HALO_PIN_LCD_D3 8
#define HALO_PIN_LCD_D4 18
#define HALO_PIN_LCD_D5 17
#define HALO_PIN_LCD_D6 16
#define HALO_PIN_LCD_D7 15
// Touch: FT6336U, I2C 0x38
#define HALO_PIN_SDA 6
#define HALO_PIN_SCL 5
#define HALO_PIN_TP_INT 7
// Sound: an I2S amplifier (no codec, no microphone)
#define HALO_PIN_BCLK 36
#define HALO_PIN_LRCK 35
#define HALO_PIN_DOUT 37
// microSD (SPI)
#define HALO_PIN_SD_CS 41
#define HALO_PIN_SD_MOSI 40
#define HALO_PIN_SD_CLK 39
#define HALO_PIN_SD_MISO 38

// The bus clock: one byte a tick, two bytes a pixel. The ST7796's write cycle is 66 ns by the datasheet (15 MHz)
#ifndef HALO_WT32_PCLK_HZ
#define HALO_WT32_PCLK_HZ 20000000
#endif

// ---------- The display (halo_wt32_display.cpp) ----------
bool hd_begin(uint32_t hz, int buffers, uint8_t brightness);
uint16_t *hd_buffer();                       // a free DMA buffer (waits while all are queued)
void hd_buffer2(uint16_t **a, uint16_t **b);  // two at once — the next one and the one after (two hd_send in order)
void hd_send(int x0, int y, int w, int rows);  // the buffer hd_buffer gave — into the queue (does not wait)
void hd_wait_idle();                         // everything is on the display
bool hd_wait_te(uint32_t count, uint32_t timeout_ms);  // wait until the TE counter reaches count
void hd_te_snapshot(uint32_t *count, uint32_t *rise);   // the last TE's number and its time, as a pair
uint32_t hd_te_count();
uint32_t hd_te_period();
uint32_t hd_te_rise();
uint32_t hd_te_blank();                    // TE's pause: the pass over the rows starts when it ends
void hd_set_brightness(uint8_t level);       // between frames (the engine calls it)
void hd_set_power(bool on);                  // the panel is seen / dark — between frames too
int hd_buffers();
void hd_state(int *fly, int *queued, int *free_bufs);  // the output queue — for sys
void hd_regs(char *out, int n);              // the bus in a line — for sys

// ---------- Sleep (halo_wt32_system.cpp) ----------
void hp_frame_end();  // the engine, once the frame is on the screen: asked to sleep — "pretend" and watch the wake-ups

// ---------- Touch (halo_wt32_touch.cpp) ----------
bool ht_begin();
