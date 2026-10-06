#include "fifa96_loader/fifa96_iso9660.h"
#include <string.h>

#define ISO_SEC FIFA96_ISO9660_SECTOR_SIZE

static uint16_t iso_u16le(const uint8_t *p) {
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t iso_u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int iso_upper(int c) {
  return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

static int iso_name_eq(const char *a, const char *b) {
  while (*a && *b) {
    if (iso_upper((unsigned char)*a) != iso_upper((unsigned char)*b)) return 0;
    a++;
    b++;
  }
  return *a == '\0' && *b == '\0';
}

struct iso_dirent {
  char name[FIFA96_ISO9660_NAME_MAX + 1];
  uint32_t lba;
  uint32_t size;
  uint8_t flags;
};

typedef fifa96_err_t (*iso_dirent_fn)(void *ctx, const struct iso_dirent *e);

static fifa96_err_t iso_dir_iterate(const struct fifa96_iso9660 *iso, uint32_t dir_lba,
                                    uint32_t dir_size, iso_dirent_fn fn, void *ctx) {
  uint64_t base = (uint64_t)dir_lba * (uint64_t)ISO_SEC;
  uint64_t pos = 0;
  while (pos < (uint64_t)dir_size) {
    uint8_t len = 0;
    fifa96_err_t e = iso->read(iso->ctx, base + pos, &len, 1);
    if (e != FIFA96_OK) return e;
    if (len == 0) {
      uint64_t next = (pos / ISO_SEC + 1) * ISO_SEC;
      if (next >= (uint64_t)dir_size) break;
      pos = next;
      continue;
    }
    if (len < 33 || pos + len > (uint64_t)dir_size) return FIFA96_ERR_TRUNCATED;
    uint8_t rec[FIFA96_ISO9660_NAME_MAX + 1];
    e = iso->read(iso->ctx, base + pos, rec, len);
    if (e != FIFA96_OK) return e;
    uint8_t namelen = rec[32];
    if ((size_t)namelen > (size_t)len - 33u) return FIFA96_ERR_TRUNCATED;
    if (!(namelen == 1 && (rec[33] == 0 || rec[33] == 1))) {
      struct iso_dirent de;
      size_t k = 0;
      for (size_t i = 0; i < namelen && rec[33 + i] != ';'; i++) {
        if (k >= FIFA96_ISO9660_NAME_MAX) return FIFA96_ERR_TRUNCATED;
        de.name[k++] = (char)rec[33 + i];
      }
      if (k == 0) return FIFA96_ERR_TRUNCATED;
      de.name[k] = '\0';
      de.lba = iso_u32le(rec + 2);
      de.size = iso_u32le(rec + 10);
      de.flags = rec[25];
      uint64_t off = (uint64_t)de.lba * (uint64_t)ISO_SEC;
      if (off > iso->image_len || (uint64_t)de.size > iso->image_len - off)
        return FIFA96_ERR_TRUNCATED;
      e = fn(ctx, &de);
      if (e != FIFA96_OK) return e;
    }
    pos += len;
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_iso9660_open(fifa96_iso9660_read_fn read, void *ctx, uint64_t image_len,
                                 struct fifa96_iso9660 *iso) {
  if (!read || !iso) return FIFA96_ERR_INVALID;
  if (image_len < (uint64_t)(FIFA96_ISO9660_PVD_SECTOR + 1u) * (uint64_t)ISO_SEC)
    return FIFA96_ERR_TRUNCATED;
  uint8_t pvd[ISO_SEC];
  fifa96_err_t e = read(ctx, (uint64_t)FIFA96_ISO9660_PVD_SECTOR * (uint64_t)ISO_SEC, pvd, ISO_SEC);
  if (e != FIFA96_OK) return e;
  if (pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5) != 0 || pvd[6] != 1)
    return FIFA96_ERR_BAD_MAGIC;
  if (iso_u16le(pvd + 128) != ISO_SEC) return FIFA96_ERR_UNSUPPORTED;
  uint8_t root_len = pvd[156];
  if (root_len < 34) return FIFA96_ERR_TRUNCATED;
  uint32_t lba = iso_u32le(pvd + 158);
  uint32_t size = iso_u32le(pvd + 166);
  if (!(pvd[156 + 25] & 0x02u)) return FIFA96_ERR_TRUNCATED;
  uint64_t off = (uint64_t)lba * (uint64_t)ISO_SEC;
  if (off > image_len || (uint64_t)size > image_len - off) return FIFA96_ERR_TRUNCATED;
  iso->read = read;
  iso->ctx = ctx;
  iso->image_len = image_len;
  iso->root_lba = lba;
  iso->root_size = size;
  return FIFA96_OK;
}

fifa96_err_t fifa96_iso9660_read_extent(const struct fifa96_iso9660 *iso, uint32_t lba,
                                        uint32_t size, uint8_t *dst, size_t dst_len) {
  if (!iso || !iso->read || (!dst && size != 0)) return FIFA96_ERR_INVALID;
  uint64_t off = (uint64_t)lba * (uint64_t)ISO_SEC;
  if (off > iso->image_len || (uint64_t)size > iso->image_len - off) return FIFA96_ERR_TRUNCATED;
  if ((size_t)size > dst_len) return FIFA96_ERR_TRUNCATED;
  return iso->read(iso->ctx, off, dst, size);
}

struct iso_find_ctx {
  const char *want;
  int found;
  struct iso_dirent hit;
};

static fifa96_err_t iso_find_visit(void *ctx, const struct iso_dirent *e) {
  struct iso_find_ctx *f = (struct iso_find_ctx *)ctx;
  if (!f->found && iso_name_eq(e->name, f->want)) {
    f->found = 1;
    f->hit = *e;
  }
  return FIFA96_OK;
}

static fifa96_err_t iso_find_in(const struct fifa96_iso9660 *iso, uint32_t lba, uint32_t size,
                                const char *rest, uint32_t *out_lba, uint32_t *out_size,
                                unsigned depth) {
  if (depth >= FIFA96_ISO9660_DEPTH_MAX) return FIFA96_ERR_UNSUPPORTED;
  const char *slash = strchr(rest, '/');
  size_t clen = slash ? (size_t)(slash - rest) : strlen(rest);
  if (clen == 0 || clen > FIFA96_ISO9660_NAME_MAX) return FIFA96_ERR_NOT_FOUND;
  char want[FIFA96_ISO9660_NAME_MAX + 1];
  size_t k = 0;
  for (size_t i = 0; i < clen && rest[i] != ';'; i++) want[k++] = rest[i];
  if (k == 0) return FIFA96_ERR_NOT_FOUND;
  want[k] = '\0';
  struct iso_find_ctx f;
  memset(&f, 0, sizeof f);
  f.want = want;
  fifa96_err_t e = iso_dir_iterate(iso, lba, size, iso_find_visit, &f);
  if (e != FIFA96_OK) return e;
  if (!f.found) return FIFA96_ERR_NOT_FOUND;
  if (!slash) {
    if (f.hit.flags & 0x02u) return FIFA96_ERR_NOT_FOUND;
    *out_lba = f.hit.lba;
    *out_size = f.hit.size;
    return FIFA96_OK;
  }
  if (!(f.hit.flags & 0x02u)) return FIFA96_ERR_NOT_FOUND;
  const char *next = slash + 1;
  if (*next == '\0') return FIFA96_ERR_NOT_FOUND;
  return iso_find_in(iso, f.hit.lba, f.hit.size, next, out_lba, out_size, depth + 1);
}

fifa96_err_t fifa96_iso9660_find(const struct fifa96_iso9660 *iso, const char *path,
                                 uint32_t *lba, uint32_t *size) {
  if (!iso || !iso->read || !path || !lba || !size) return FIFA96_ERR_INVALID;
  while (*path == '/') path++;
  if (*path == '\0') return FIFA96_ERR_NOT_FOUND;
  return iso_find_in(iso, iso->root_lba, iso->root_size, path, lba, size, 0);
}

struct iso_walk_ctx {
  const struct fifa96_iso9660 *iso;
  fifa96_iso9660_visit_fn visit;
  void *ctx;
  char path[FIFA96_ISO9660_PATH_MAX];
  size_t used;
  unsigned depth;
};

static fifa96_err_t iso_walk_visit(void *ctx, const struct iso_dirent *e) {
  struct iso_walk_ctx *w = (struct iso_walk_ctx *)ctx;
  size_t nlen = strlen(e->name);
  if (w->used + nlen + 2u > sizeof w->path) return FIFA96_ERR_TRUNCATED;
  size_t save = w->used;
  w->path[w->used++] = '/';
  memcpy(w->path + w->used, e->name, nlen);
  w->used += nlen;
  w->path[w->used] = '\0';
  fifa96_err_t r;
  if (e->flags & 0x02u) {
    if (w->depth >= FIFA96_ISO9660_DEPTH_MAX) {
      r = FIFA96_ERR_UNSUPPORTED;
    } else {
      w->depth++;
      r = iso_dir_iterate(w->iso, e->lba, e->size, iso_walk_visit, w);
      w->depth--;
    }
  } else {
    r = w->visit(w->ctx, w->path, e->lba, e->size);
  }
  w->path[save] = '\0';
  w->used = save;
  return r;
}

fifa96_err_t fifa96_iso9660_walk(const struct fifa96_iso9660 *iso, fifa96_iso9660_visit_fn visit,
                                 void *ctx) {
  if (!iso || !iso->read || !visit) return FIFA96_ERR_INVALID;
  struct iso_walk_ctx w;
  memset(&w, 0, sizeof w);
  w.iso = iso;
  w.visit = visit;
  w.ctx = ctx;
  return iso_dir_iterate(iso, iso->root_lba, iso->root_size, iso_walk_visit, &w);
}
