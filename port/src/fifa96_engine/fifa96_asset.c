#include "fifa96_engine/fifa96_asset.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int asset_upper(int c) {
  return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

static int asset_path_eq(const char *a, const char *b) {
  while (*a && *b) {
    if (asset_upper((unsigned char)*a) != asset_upper((unsigned char)*b)) return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

static fifa96_err_t asset_visit(void *ctx, const char *path, uint32_t lba, uint32_t size) {
  struct fifa96_asset_table *t = (struct fifa96_asset_table *)ctx;
  size_t n = strlen(path);
  if (n == 0 || n >= FIFA96_ISO9660_PATH_MAX) return FIFA96_ERR_TRUNCATED;
  if (t->count == t->cap) {
    size_t cap = t->cap ? t->cap * 2u : 16u;
    if (cap <= t->cap || cap > SIZE_MAX / sizeof *t->entries) return FIFA96_ERR_IO;
    struct fifa96_asset_entry *grown = realloc(t->entries, cap * sizeof *grown);
    if (!grown) return FIFA96_ERR_IO;
    t->entries = grown;
    t->cap = cap;
  }
  struct fifa96_asset_entry *e = &t->entries[t->count];
  memcpy(e->path, path, n + 1);
  e->lba = lba;
  e->size = size;
  t->count++;
  return FIFA96_OK;
}

fifa96_err_t fifa96_asset_mount_iso(fifa96_iso9660_read_fn read, void *ctx, uint64_t len,
                                    struct fifa96_asset_table **out) {
  if (!out) return FIFA96_ERR_INVALID;
  *out = NULL;
  if (!read) return FIFA96_ERR_INVALID;
  struct fifa96_asset_table *t = calloc(1, sizeof *t);
  if (!t) return FIFA96_ERR_IO;
  fifa96_err_t e = fifa96_iso9660_open(read, ctx, len, &t->iso);
  if (e == FIFA96_OK) e = fifa96_iso9660_walk(&t->iso, asset_visit, t);
  if (e != FIFA96_OK) {
    free(t->entries);
    free(t);
    return e;
  }
  *out = t;
  return FIFA96_OK;
}

fifa96_err_t fifa96_asset_lookup(const struct fifa96_asset_table *t, const char *path,
                                 uint32_t *lba, uint32_t *size) {
  if (!t || !path || !lba || !size) return FIFA96_ERR_INVALID;
  while (*path == '/') path++;
  if (*path == '\0') return FIFA96_ERR_NOT_FOUND;
  for (size_t i = 0; i < t->count; i++) {
    const char *stored = t->entries[i].path;
    while (*stored == '/') stored++;
    if (asset_path_eq(stored, path)) {
      *lba = t->entries[i].lba;
      *size = t->entries[i].size;
      return FIFA96_OK;
    }
  }
  return FIFA96_ERR_NOT_FOUND;
}

fifa96_err_t fifa96_asset_read(const struct fifa96_asset_table *t, const char *path,
                               uint8_t **out, size_t *len) {
  if (!t || !path || !out || !len) return FIFA96_ERR_INVALID;
  *out = NULL;
  *len = 0;
  uint32_t lba = 0, size = 0;
  fifa96_err_t e = fifa96_asset_lookup(t, path, &lba, &size);
  if (e != FIFA96_OK) return e;
  size_t n = size ? (size_t)size : 1u;
  uint8_t *buf = (uint8_t *)malloc(n);
  if (!buf) return FIFA96_ERR_IO;
  e = fifa96_iso9660_read_extent(&t->iso, lba, size, buf, n);
  if (e != FIFA96_OK) {
    free(buf);
    return e;
  }
  *out = buf;
  *len = (size_t)size;
  return FIFA96_OK;
}

void fifa96_asset_free(uint8_t *p) { free(p); }

struct asset_file {
  FILE *fp;
};

static fifa96_err_t asset_file_read(void *ctx, uint64_t off, uint8_t *dst, size_t len) {
  struct asset_file *f = (struct asset_file *)ctx;
  if (!f || !f->fp) return FIFA96_ERR_INVALID;
  if (off > (uint64_t)LONG_MAX) return FIFA96_ERR_TRUNCATED;
  if (fseek(f->fp, (long)off, SEEK_SET) != 0) return FIFA96_ERR_IO;
  if (len != 0 && fread(dst, 1, len, f->fp) != len) return FIFA96_ERR_SHORT_READ;
  return FIFA96_OK;
}

fifa96_err_t fifa96_asset_mount_file(const char *path, struct fifa96_asset_table **out) {
  if (!out) return FIFA96_ERR_INVALID;
  *out = NULL;
  if (!path) return FIFA96_ERR_INVALID;
  FILE *fp = fopen(path, "rb");
  if (!fp) return FIFA96_ERR_NOT_FOUND;
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return FIFA96_ERR_IO;
  }
  long end = ftell(fp);
  if (end < 0) {
    fclose(fp);
    return FIFA96_ERR_IO;
  }
  struct asset_file *f = (struct asset_file *)malloc(sizeof *f);
  if (!f) {
    fclose(fp);
    return FIFA96_ERR_IO;
  }
  f->fp = fp;
  fifa96_err_t e = fifa96_asset_mount_iso(asset_file_read, f, (uint64_t)end, out);
  if (e != FIFA96_OK) {
    fclose(fp);
    free(f);
  }
  return e;
}

void fifa96_asset_unmount(struct fifa96_asset_table *t) {
  if (!t) return;
  if (t->iso.read == asset_file_read) {
    struct asset_file *f = (struct asset_file *)t->iso.ctx;
    if (f) {
      fclose(f->fp);
      free(f);
    }
  }
  free(t->entries);
  free(t);
}
