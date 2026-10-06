#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_surface.h"

struct fifa96_surface *fifa96_surface_create(int w, int h) {
  if (w <= 0 || h <= 0) return NULL;
  if (w > FIFA96_SURFACE_MAX_W || h > FIFA96_SURFACE_MAX_H) return NULL;
  if (w % 8 != 0) return NULL;  /* one byte packs eight pixels per plane row */
  struct fifa96_surface *s = calloc(1, sizeof *s);
  if (!s) return NULL;
  s->width = w;
  s->height = h;
  return s;
}

void fifa96_surface_destroy(struct fifa96_surface *s) { free(s); }

void fifa96_surface_clear(struct fifa96_surface *s, uint8_t index) {
  if (!s) return;
  memset(s->indexed, index, (size_t)s->width * (size_t)s->height);
}

void fifa96_surface_set_palette6(struct fifa96_surface *s, const uint8_t rgb6[768]) {
  if (!s || !rgb6) return;
  for (size_t i = 0; i < 768u; i++) {
    s->palette[i] = (uint8_t)((rgb6[i] << 2) | (rgb6[i] >> 4));
  }
}

void fifa96_surface_set_palette8(struct fifa96_surface *s, const uint8_t rgb8[768]) {
  if (!s || !rgb8) return;
  memcpy(s->palette, rgb8, 768u);
}

void fifa96_surface_plane(struct fifa96_surface *s, fifa96_platform_frame *out) {
  if (!s || !out) return;
  const int stride = s->width / 4;      /* Mode-X plane stride, 80 for 320 px */
  const int row_bytes = s->width / 8;   /* packed 1bpp row, 40 for 320 px */
  for (int p = 0; p < 4; p++) {
    uint8_t *dst = s->planes[p];
    memset(dst, 0, (size_t)stride * (size_t)s->height);
    for (int y = 0; y < s->height; y++) {
      const uint8_t *src = s->indexed + (size_t)y * (size_t)s->width;
      uint8_t *row = dst + (size_t)y * (size_t)stride;
      for (int x = 0; x < s->width; x++) {
        uint8_t bit = (uint8_t)(((src[x] >> p) & 1u) << (7 - (x & 7)));
        row[x >> 3] |= bit;
        /* The 80-byte hardware stride holds the 40-byte packed row twice so the
           whole ABI plane (stride x height) is defined every frame. */
        row[row_bytes + (x >> 3)] |= bit;
      }
    }
    out->planes[p] = s->planes[p];
  }
  out->stride = (size_t)stride;
  memcpy((void *)out->palette, s->palette, sizeof s->palette);
  out->flags = 0;
}

static uint64_t fifa96_fnv1a(uint64_t h, const uint8_t *p, size_t n) {
  for (size_t i = 0; i < n; i++) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}

uint64_t fifa96_surface_hash(const struct fifa96_surface *s) {
  if (!s) return 0;
  uint64_t h = fifa96_fnv1a(14695981039346656037ull, s->indexed, sizeof s->indexed);
  return fifa96_fnv1a(h, s->palette, sizeof s->palette);
}
