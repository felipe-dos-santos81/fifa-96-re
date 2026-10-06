#include <stdlib.h>
#include <string.h>
#include "fifa96_engine/fifa96_surface.h"

struct fifa96_surface *fifa96_surface_create(int w, int h) {
  if (w <= 0 || h <= 0) return NULL;
  if (w > FIFA96_SURFACE_MAX_W || h > FIFA96_SURFACE_MAX_H) return NULL;
  if (w % 4 != 0) return NULL;  /* one plane per x & 3, four pixels per plane byte column */
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
  for (int p = 0; p < 4; p++) {
    uint8_t *dst = s->planes[p];
    for (int y = 0; y < s->height; y++) {
      const uint8_t *src = s->indexed + (size_t)y * (size_t)s->width;
      uint8_t *row = dst + (size_t)y * (size_t)stride;
      /* planar-chunky Mode-X: pixel (x, y) -> plane x & 3, byte y * 80 + (x >> 2) */
      for (int x = p; x < s->width; x += 4) row[x >> 2] = src[x];
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
