// Arduino.h — a stand-in for the browser build of Halo (PopkaStudio engine). Only what lib/Halo and the sketches on it
// use: integer types, Serial, time, a little of FreeRTOS and ESP-IDF. The real board code is compiled unchanged on top
// of it; where the board has hardware (tasks, flash, interrupts) there is a no-op or a plain C equivalent.
#pragma once

#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __cplusplus
#include <algorithm>
#include <cmath>
#include <new>
using std::max;
using std::min;
#endif

// ---------- Attributes the board needs and the browser does not ----------
#define IRAM_ATTR
#define DRAM_ATTR
#define EXT_RAM_BSS_ATTR
#define PROGMEM
#define F(s) (s)
#define pgm_read_byte(p) (*(const uint8_t *)(p))
#define pgm_read_word(p) (*(const uint16_t *)(p))
#define pgm_read_dword(p) (*(const uint32_t *)(p))

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define radians(deg) ((deg) * 0.017453292519943295)
#define degrees(rad) ((rad) * 57.29577951308232)
#define sq(x) ((x) * (x))

// ---------- Time: the page's clock (halo_web.cpp) ----------
uint32_t millis();
uint32_t micros();
static inline void delay(uint32_t) {}
static inline void delayMicroseconds(uint32_t) {}
static inline void yield() {}

// ---------- Pins: nothing is wired in the browser ----------
#define INPUT 0x01
#define OUTPUT 0x03
#define INPUT_PULLUP 0x05
#define LOW 0
#define HIGH 1
static inline void pinMode(int, int) {}
static inline int digitalRead(int) { return HIGH; }
static inline void digitalWrite(int, int) {}

// ---------- Serial: the browser console (and the page's log) ----------
#ifdef __cplusplus
struct WebSerial {
  void begin(unsigned long) {}
  operator bool() const { return true; }
  int available() { return 0; }
  int read() { return -1; }
  void flush() { fflush(stdout); }
  size_t write(const uint8_t *p, size_t n) { return fwrite(p, 1, n, stdout); }
  size_t write(uint8_t c) { return fputc(c, stdout) == EOF ? 0 : 1; }
  size_t print(const char *s) { return fputs(s, stdout) >= 0 ? strlen(s) : 0; }
  size_t print(int v) { return printf("%d", v); }
  size_t print(unsigned v) { return printf("%u", v); }
  size_t print(long v) { return printf("%ld", v); }
  size_t print(unsigned long v) { return printf("%lu", v); }
  size_t print(double v, int digits = 2) { return printf("%.*f", digits, v); }
  size_t println() { return print("\n"); }
  template <class T> size_t println(T v) { return print(v) + println(); }
  size_t printf(const char *fmt, ...) __attribute__((format(printf, 2, 3))) {
    va_list ap;
    va_start(ap, fmt);
    const int n = vprintf(fmt, ap);
    va_end(ap);
    return n < 0 ? 0 : n;
  }
};
extern WebSerial Serial;
#endif

// ---------- FreeRTOS: one thread in the browser — tasks are not started, locks do nothing ----------
typedef int BaseType_t;
typedef unsigned UBaseType_t;
typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef void *SemaphoreHandle_t;
typedef void (*TaskFunction_t)(void *);
typedef int portMUX_TYPE;
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdFAIL 0
#define portMAX_DELAY 0xFFFFFFFFu
#define configTICK_RATE_HZ 1000
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(m) ((void)(m))
#define portEXIT_CRITICAL(m) ((void)(m))
#define portENTER_CRITICAL_ISR(m) ((void)(m))
#define portEXIT_CRITICAL_ISR(m) ((void)(m))
static inline BaseType_t xPortGetCoreID() { return 1; }
static inline BaseType_t xTaskCreatePinnedToCore(TaskFunction_t, const char *, uint32_t, void *, UBaseType_t,
                                                 TaskHandle_t *h, BaseType_t) {
  if (h) *h = nullptr;
  return pdFAIL;
}
static inline BaseType_t xTaskCreate(TaskFunction_t, const char *, uint32_t, void *, UBaseType_t, TaskHandle_t *h) {
  if (h) *h = nullptr;
  return pdFAIL;
}
static inline void vTaskDelete(TaskHandle_t) {}
static inline void vTaskDelay(TickType_t) {}
static inline void xTaskNotifyGive(TaskHandle_t) {}
static inline uint32_t ulTaskNotifyTake(BaseType_t, TickType_t) { return 0; }

// ---------- ESP-IDF: memory, clock, random ----------
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define MALLOC_CAP_INTERNAL (1 << 11)
#define MALLOC_CAP_SPIRAM (1 << 10)
#define MALLOC_CAP_8BIT (1 << 2)
#define MALLOC_CAP_32BIT (1 << 1)
#define MALLOC_CAP_DMA (1 << 3)
#define MALLOC_CAP_DEFAULT (1 << 12)
static inline void *heap_caps_malloc(size_t n, uint32_t) { return malloc(n); }
static inline void *heap_caps_calloc(size_t n, size_t size, uint32_t) { return calloc(n, size); }
static inline void *heap_caps_realloc(void *p, size_t n, uint32_t) { return realloc(p, n); }
static inline void heap_caps_free(void *p) { free(p); }
// What the board would have free with Halo running (docs/halo.md): the screens that print it need some number
static inline size_t heap_caps_get_free_size(uint32_t caps) {
  return caps & MALLOC_CAP_SPIRAM ? 7600u * 1024 : 180u * 1024;
}
static inline size_t heap_caps_get_largest_free_block(uint32_t caps) { return heap_caps_get_free_size(caps); }
static inline uint32_t esp_cpu_get_cycle_count() { return micros() * 240; }
uint32_t esp_random();  // halo_web.cpp: the same sequence on every start
#include "esp_timer.h"
