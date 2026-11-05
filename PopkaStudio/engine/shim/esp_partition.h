// esp_partition.h — flash partitions. The browser has none yet: a sketch that looks one up finds nothing (HaloWatch:
// the player has no tracks) and carries on.
#pragma once
#include <stddef.h>
#include <stdint.h>
typedef enum { ESP_PARTITION_TYPE_APP = 0, ESP_PARTITION_TYPE_DATA = 1 } esp_partition_type_t;
typedef enum { ESP_PARTITION_SUBTYPE_DATA_FAT = 0x81, ESP_PARTITION_SUBTYPE_ANY = 0xff } esp_partition_subtype_t;
typedef enum { ESP_PARTITION_MMAP_DATA = 0, ESP_PARTITION_MMAP_INST } esp_partition_mmap_memory_t;
typedef uint32_t esp_partition_mmap_handle_t;
typedef struct {
  uint32_t address, size;
  char label[17];
} esp_partition_t;
static inline const esp_partition_t *esp_partition_find_first(esp_partition_type_t, esp_partition_subtype_t,
                                                              const char *) {
  return nullptr;
}
static inline int esp_partition_read(const esp_partition_t *, size_t, void *, size_t) { return -1; }
static inline int esp_partition_mmap(const esp_partition_t *, size_t, size_t, esp_partition_mmap_memory_t,
                                     const void **, esp_partition_mmap_handle_t *) {
  return -1;
}
static inline void esp_partition_munmap(esp_partition_mmap_handle_t) {}
