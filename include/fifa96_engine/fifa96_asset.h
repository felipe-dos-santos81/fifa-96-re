#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_iso9660.h"

struct fifa96_asset_entry { char path[FIFA96_ISO9660_PATH_MAX]; uint32_t lba, size; };
struct fifa96_asset_table {
  struct fifa96_iso9660 iso;
  struct fifa96_asset_entry *entries;
  size_t count, cap;
};

fifa96_err_t fifa96_asset_mount_iso(fifa96_iso9660_read_fn read, void *ctx, uint64_t len,
                                    struct fifa96_asset_table **out);
fifa96_err_t fifa96_asset_mount_file(const char *path, struct fifa96_asset_table **out);
fifa96_err_t fifa96_asset_lookup(const struct fifa96_asset_table *t, const char *path,
                                 uint32_t *lba, uint32_t *size);
fifa96_err_t fifa96_asset_read(const struct fifa96_asset_table *t, const char *path,
                               uint8_t **out, size_t *len);
void fifa96_asset_free(uint8_t *p);
void fifa96_asset_unmount(struct fifa96_asset_table *t);
