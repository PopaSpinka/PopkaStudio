// DMA2D copy of a rectangle into a frame buffer. The core has this in its library (esp_lcd) but ships no header for it: the
// declarations below are ESP-IDF v5.5.5's components/esp_lcd/priv_include/esp_async_fbcpy.h. The core is pinned at 3.3.11
// (CLAUDE.md, rule 13); when it is updated, compare with that file.
//
// One transaction per handle at a time, there is no queue inside: the next call only after the callback of the previous one.
#pragma once

#include <esp_err.h>
#include <hal/color_types.h>

extern "C" {
typedef struct esp_async_fbcpy_context_t *esp_async_fbcpy_handle_t;
typedef struct {
} esp_async_fbcpy_config_t;
typedef struct {
  const void *src_buffer;
  void *dst_buffer;
  size_t src_buffer_size_x, src_buffer_size_y;  // in pixels
  size_t dst_buffer_size_x, dst_buffer_size_y;
  size_t src_offset_x, src_offset_y;
  size_t dst_offset_x, dst_offset_y;
  size_t copy_size_x, copy_size_y;
  color_space_pixel_format_t pixel_format_unique_id;
} esp_async_fbcpy_trans_desc_t;
typedef struct {
} esp_async_fbcpy_event_data_t;
typedef bool (*esp_async_fbcpy_event_callback_t)(esp_async_fbcpy_handle_t mcp, esp_async_fbcpy_event_data_t *event_data, void *cb_args);
esp_err_t esp_async_fbcpy_install(const esp_async_fbcpy_config_t *config, esp_async_fbcpy_handle_t *mcp);
esp_err_t esp_async_fbcpy(esp_async_fbcpy_handle_t mcp, esp_async_fbcpy_trans_desc_t *transaction,
                          esp_async_fbcpy_event_callback_t memcpy_done_cb, void *cb_args);
}
