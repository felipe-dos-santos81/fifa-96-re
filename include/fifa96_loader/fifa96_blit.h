#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_MODEX_WIDTH 320u
#define FIFA96_MODEX_PLANES 4u
#define FIFA96_MODEX_PLANE_STRIDE 80u
#define FIFA96_MODEX_PAGE_ROWS 240u
#define FIFA96_MODEX_PAGE_PLANE_OFFSET 0x4B00u
#define FIFA96_MODEX_PAGE_LINEAR_OFFSET 0x12C00u

typedef struct {
  uint8_t *planes[FIFA96_MODEX_PLANES];
  size_t plane_cap;
} fifa96_modex_image;

typedef struct {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
} fifa96_blit_clip;

uint32_t fifa96_blit_page_rows(uint32_t page);
uint32_t fifa96_blit_page_start(uint32_t page);
int fifa96_blit_modex(fifa96_modex_image *dst, int32_t x, int32_t y,
                      const uint8_t *src, size_t src_len, uint32_t width, uint32_t height,
                      const fifa96_blit_clip *clip);
