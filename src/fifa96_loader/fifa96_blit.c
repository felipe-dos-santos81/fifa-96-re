#include "fifa96_loader/fifa96_blit.h"

uint32_t fifa96_blit_page_rows(uint32_t page) {
  return page * FIFA96_MODEX_PAGE_ROWS;
}

uint32_t fifa96_blit_page_start(uint32_t page) {
  return page * FIFA96_MODEX_PAGE_PLANE_OFFSET;
}

int fifa96_blit_modex(fifa96_modex_image *dst, int32_t x, int32_t y,
                      const uint8_t *src, size_t src_len, uint32_t width, uint32_t height,
                      const fifa96_blit_clip *clip) {
  if (!dst || !src || !clip) return -(int)FIFA96_ERR_TRUNCATED;
  for (uint32_t p = 0; p < FIFA96_MODEX_PLANES; p++)
    if (!dst->planes[p]) return -(int)FIFA96_ERR_TRUNCATED;
  if (width == 0 || height == 0) return FIFA96_OK;
  if ((uint64_t)width * height > src_len) return -(int)FIFA96_ERR_TRUNCATED;

  int64_t sx = 0, sy = 0;
  int64_t w = width, h = height;
  int64_t dx = x, dy = y;
  if (clip->top > dy) {
    sy = clip->top - dy;
    h -= sy;
    dy = clip->top;
  }
  if (dy + h > clip->bottom) h = clip->bottom - dy;
  if (clip->left > dx) {
    sx = clip->left - dx;
    w -= sx;
    dx = clip->left;
  }
  if (dx + w > clip->right) w = clip->right - dx;
  if (w <= 0 || h <= 0) return FIFA96_OK;
  if (dx < 0 || dy < 0) return -(int)FIFA96_ERR_TRUNCATED;
  if (sx + w > (int64_t)width || sy + h > (int64_t)height) return -(int)FIFA96_ERR_TRUNCATED;

  uint64_t last = 0;
  int any = 0;
  for (int64_t p = 0; p < FIFA96_MODEX_PLANES; p++) {
    int64_t count = w - p;
    if (count < 4) continue;
    uint64_t off = (uint64_t)(dy + h - 1) * FIFA96_MODEX_PLANE_STRIDE +
                   (uint64_t)((dx + p) >> 2) + (uint64_t)(count / 4 - 1);
    if (!any || off > last) last = off;
    any = 1;
  }
  if (any && last >= dst->plane_cap) return -(int)FIFA96_ERR_TRUNCATED;

  for (int64_t p = 0; p < FIFA96_MODEX_PLANES; p++) {
    int64_t count = w - p;
    if (count < 4) continue;
    int64_t bytes = count / 4;
    uint64_t plane = (uint64_t)((dx + p) & 3);
    uint64_t col = (uint64_t)((dx + p) >> 2);
    for (int64_t row = 0; row < h; row++) {
      const uint8_t *s = src + (uint64_t)(sy + row) * width + (uint64_t)sx + (uint64_t)p;
      uint64_t base = (uint64_t)(dy + row) * FIFA96_MODEX_PLANE_STRIDE + col;
      for (int64_t k = 0; k < bytes; k++)
        dst->planes[plane][base + (uint64_t)k] = s[(uint64_t)k * 4];
    }
  }
  return FIFA96_OK;
}
