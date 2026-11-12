// The P4 board's own declarations: its pins and what its files give each other. Other boards never see this file.
#pragma once

#include "halo_internal.h"
#include "../esp32_common/halo_esp32.h"

#define HALO_PIN_LCD_RST 33
#define HALO_PIN_BACKLIGHT 32
#define HALO_PIN_BOOT 35

// ---------- Display (halo_p4_display.cpp) ----------
// The panel has no memory of its own: the DSI controller keeps reading a frame buffer in PSRAM, 60.4 times a second.
// There are two buffers. Draw in the back one, hd_flip() — it goes on screen at the next frame boundary, and the other one
// becomes the back buffer after that boundary. "te" names are the same as on the S3: the frame boundary plays that role.
bool hd_begin(uint8_t brightness);
void hd_use_rgb888();  // before hd_begin: 3 bytes a pixel (experiments/P4Video; the frame engine itself is RGB565)
uint16_t *hd_back();
uint16_t *hd_front();
void hd_flip();
bool hd_wait_te(uint32_t count, uint32_t timeout_ms);  // wait until the boundary counter reaches count
void hd_te_snapshot(uint32_t *count, uint32_t *rise);  // the latest boundary and its time, as a consistent pair
uint32_t hd_te_count();
uint32_t hd_te_period();
void hd_set_brightness(uint8_t level);
void hd_set_power(bool on);
// The refresh at run time: the frame is made longer with blank rows at its end, a line and the picture stay the same. 0 — the
// panel's own 60.4 Hz. Does not wait: the frame task writes it right after the next boundary (hd_rate_apply), so the frame
// after that one has it (experiments/P4Video, docs/p4-video.md)
void hd_set_rate(float hz);
void hd_rate_apply();  // the frame task, right after a boundary
bool ht_begin();  // the touch chip (halo_p4_touch.cpp)
void hp_frame_end();     // the frame is on the screen: go on sleeping or wake up (halo_p4_system.cpp)
void hf_swallow_boot();  // the BOOT press that woke the board is not a press (halo_p4_frame.cpp)
// ---------- A picture under the interface (halo_p4_frame.cpp): the video player ----------
// Called in every frame after update. It either fills the whole back buffer and returns true — then the frame goes on screen
// even with nothing else to draw, and what the interface drew this frame is painted over the picture, not over black —
// or returns false and the frame is an ordinary one. changed — the interface asks to redraw something this frame: a picture
// must be there under it, so the same video frame is decoded again. Called before this frame's changes are taken: having
// filled the buffer, it marks (halo_damage) everything of its own that lies over the picture — what is not marked is not
// painted over the new picture and is missing from the frame. nullptr — no picture
typedef bool (*HaloUnder)(uint16_t *back, bool changed);
void halo_under(HaloUnder fn);
void hf_tiles(int n);  // before halo_begin: how many tiles of internal memory the frame takes (3, or 2 to leave room for the sound with Wi-Fi)
// For sys (halo_core.cpp)
int hd_buffers();
void hd_state(int *fly, int *queued, int *free_bufs);
void hd_regs(char *out, int n);
