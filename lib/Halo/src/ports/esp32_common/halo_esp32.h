// What the ESP32 boards share: the log, commands and the trace (halo_core.cpp).
#pragma once

// ---------- Ядро (halo_core.cpp) ----------
void hc_begin(const char *name);
void hc_trace_setup();
void hc_loop();
void hc_send_image(const uint16_t *px, int w, int h);  // для shot: SHOT w h / пиксели / SHOT END сумма
void hc_stage(int s);   // шаг запуска (0 core, 1 display bus, 2 panel, 3 dma, 4 touch, 5 tasks, 6 first frame, 7 running)
bool hc_safe_mode();    // упали два раза подряд — запуск без экрана
void hc_boot_done();    // запуск кончился: снять сторожевой таймер с setup()
