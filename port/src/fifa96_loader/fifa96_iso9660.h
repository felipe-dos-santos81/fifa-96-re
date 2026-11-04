#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_ISO9660_SECTOR_SIZE 2048u
#define FIFA96_ISO9660_PVD_SECTOR 16u
#define FIFA96_ISO9660_PATH_MAX 1024u
#define FIFA96_ISO9660_NAME_MAX 255u
#define FIFA96_ISO9660_DEPTH_MAX 16u

typedef fifa96_err_t (*fifa96_iso9660_read_fn)(void *ctx, uint64_t off, uint8_t *dst, size_t len);
typedef fifa96_err_t (*fifa96_iso9660_visit_fn)(void *ctx, const char *path, uint32_t lba,
                                                uint32_t size);

struct fifa96_iso9660 {
  fifa96_iso9660_read_fn read;
  void *ctx;
  uint64_t image_len;
  uint32_t root_lba;
  uint32_t root_size;
};

fifa96_err_t fifa96_iso9660_open(fifa96_iso9660_read_fn read, void *ctx, uint64_t image_len,
                                 struct fifa96_iso9660 *iso);
fifa96_err_t fifa96_iso9660_read_extent(const struct fifa96_iso9660 *iso, uint32_t lba,
                                        uint32_t size, uint8_t *dst, size_t dst_len);
fifa96_err_t fifa96_iso9660_find(const struct fifa96_iso9660 *iso, const char *path,
                                 uint32_t *lba, uint32_t *size);
fifa96_err_t fifa96_iso9660_walk(const struct fifa96_iso9660 *iso, fifa96_iso9660_visit_fn visit,
                                 void *ctx);
