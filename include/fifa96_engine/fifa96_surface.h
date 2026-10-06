#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_engine/fifa96_platform.h"

#define FIFA96_SURFACE_MAX_W 320
#define FIFA96_SURFACE_MAX_H 240

struct fifa96_surface {
  int width, height;
  uint8_t indexed[FIFA96_SURFACE_MAX_W * FIFA96_SURFACE_MAX_H];
  uint8_t palette[768];       /* 8-bit RGB */
  uint8_t planes[4][FIFA96_SURFACE_MAX_W * FIFA96_SURFACE_MAX_H / 4];
};

struct fifa96_surface *fifa96_surface_create(int w, int h);
void fifa96_surface_destroy(struct fifa96_surface *s);
void fifa96_surface_clear(struct fifa96_surface *s, uint8_t index);
void fifa96_surface_set_palette6(struct fifa96_surface *s, const uint8_t rgb6[768]);
void fifa96_surface_set_palette8(struct fifa96_surface *s, const uint8_t rgb8[768]);
void fifa96_surface_plane(struct fifa96_surface *s, fifa96_platform_frame *out);
uint64_t fifa96_surface_hash(const struct fifa96_surface *s); /* FNV-1a over indexed + palette */
