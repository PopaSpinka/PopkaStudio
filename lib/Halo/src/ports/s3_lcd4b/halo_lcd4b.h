// The ESP32-S3-Touch-LCD-4B board's own declarations: its pins and what its files give each other. Other boards never see
// this file. The board and what was measured on it: boards/lcd4b/hardware.md.
#pragma once

#include "halo_internal.h"
#include "../esp32_common/halo_esp32.h"

// The board's pins (Waveshare's schematic, page 1)
#define HALO_PIN_SDA 47  // one I2C bus for all: the expander, touch, power chip, RTC, motion sensor, both sound chips
#define HALO_PIN_SCL 48
#define HALO_PIN_BL 4  // the backlight: its PWM pulls the boost converter's feedback up — full duty is dark
#define HALO_PIN_BOOT 0
// The RGB bus: DE, VSYNC, HSYNC, PCLK and 16 data lines (B0-B4, G0-G5, R0-R4)
#define HALO_PIN_DE 17
#define HALO_PIN_VSYNC 3
#define HALO_PIN_HSYNC 46
#define HALO_PIN_PCLK 9
#define HALO_LCD_DATA_PINS {40, 41, 42, 2, 1, 21, 8, 18, 45, 38, 39, 10, 11, 12, 13, 14}
// Sound: I2S to the ES8311 codec (speaker) and the ES7210 (microphones), the speaker's amplifier on the expander
#define HALO_PIN_MCLK 5
#define HALO_PIN_BCLK 16
#define HALO_PIN_LRCK 7
#define HALO_PIN_DOUT 6
#define HALO_PIN_DIN 15

// The TCA9554 expander (I2C 0x20) and what its pins do here
#define HX_LCD_CS 0
#define HX_LCD_SDA 1
#define HX_LCD_SCL 2
#define HX_AMP 3      // the speaker's amplifier: high — on
#define HX_PWR_KEY 4  // high while the PWR button is down (a BSS138 inverts the power chip's PWRON line)
#define HX_TP_RST 5
#define HX_TP_INT 6
#define HX_LCD_RST 7

// ---------- The I2C bus and the expander (halo_lcd4b_display.cpp: the panel needs them first) ----------
struct i2c_master_bus_t;
struct i2c_master_dev_t;
i2c_master_bus_t *hx_bus();                  // the bus (nullptr — did not start)
i2c_master_dev_t *hx_device(uint8_t addr);   // one more device on it (400 kHz), nullptr — no answer
void hx_set(int pin, bool high);             // an output of the expander
void hx_dir(int pin, bool output);
int hx_inputs();                             // the expander's input register (-1 — no answer)

// ---------- The display (halo_lcd4b_display.cpp) ----------
bool hd_begin(uint32_t hz, int buffers, uint8_t brightness);
uint16_t *hd_buffer();                         // a free DMA buffer (waits while all are queued)
void hd_buffer2(uint16_t **a, uint16_t **b);    // two at once — the next one and the one after (two hd_send in order)
// The DMA writes PSRAM in blocks of HD_BURST bytes: a span's start and width are multiples of HD_SPAN pixels
#ifndef HD_ISR_CORE
#define HD_ISR_CORE 0  // the core that takes the display's interrupts (bounce fills: each has a 0.3 ms deadline)
#endif
#define HD_BURST 64
#define HD_SPAN (HD_BURST / 2)
void hd_send(int x0, int y, int w, int rows);  // the buffer hd_buffer gave — into the frame buffer (queued, does not wait);
                                               // x0 and w — multiples of HD_SPAN
void hd_wait_idle();                           // everything is in the frame buffer
// "TE" here is the start of a pass over the frame buffer (the fill of its first rows): the next pass shows what is written
// into a band after this one has read it
bool hd_wait_te(uint32_t count, uint32_t timeout_ms);  // wait until the pass counter reaches count
void hd_te_snapshot(uint32_t *count, uint32_t *rise);   // the latest pass's number and its time, as a pair
uint32_t hd_te_count();
uint32_t hd_te_period();
uint32_t hd_te_rise();
uint32_t hd_te_blank();                      // from a pass's start to the LCD showing its first row
// A band of a frame begun at pass `pass` may go into the frame buffer: that pass has read its rows [0, rows) — or it is
// over (the next one, which shows the frame, has not come to them yet: writing now puts them in the same picture)
bool hd_read(uint32_t pass, int rows);
uint32_t hd_read_eta(int rows);              // when it will have (µs, the board's clock; by the last passes)
const uint16_t *hd_fb();                     // the frame buffer (what is on screen; DMA writes it — drop the cache first)
void hd_set_brightness(uint8_t level);       // between frames (the engine calls it)
void hd_set_power(bool on);                  // the light on / off — between frames too
int hd_buffers();
void hd_state(int *fly, int *queued, int *free_bufs);  // the output queue — for sys
void hd_regs(char *out, int n);              // the fills and writes in a line — for sys

// ---------- Sleep and buttons (halo_lcd4b_system.cpp, halo_lcd4b_frame.cpp) ----------
void hp_frame_end();     // the engine, once the frame is on the screen: asked to sleep — "pretend" and watch the wake-ups
void hf_swallow_boot();  // the BOOT press that woke the board is not a press (halo_lcd4b_frame.cpp)

// ---------- Touch and the power chip (halo_lcd4b_touch.cpp) ----------
bool ht_begin();
