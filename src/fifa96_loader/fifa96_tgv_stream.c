#include "fifa96_loader/fifa96_tgv_stream.h"

#define TGV_TAG_KVGT 0x5447566Bu

int fifa96_tgv_walk_next(fifa96_tgv_walk *walk, const uint8_t **chunk, size_t *chunk_len, int *is_frame) {
  if (!walk || !chunk || !chunk_len || !is_frame) return -(int)FIFA96_ERR_TRUNCATED;
  if (walk->pos >= walk->size) return 0;
  if (walk->size - walk->pos < 8) return -(int)FIFA96_ERR_TRUNCATED;
  const uint8_t *p = walk->data + walk->pos;
  uint32_t tag = (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
                 ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
  uint32_t len = (uint32_t)p[4] | ((uint32_t)p[5] << 8) |
                 ((uint32_t)p[6] << 16) | ((uint32_t)p[7] << 24);
  if (len < 8 || (size_t)len > walk->size - walk->pos) return -(int)FIFA96_ERR_TRUNCATED;
  *chunk = p;
  *chunk_len = len;
  *is_frame = tag == TGV_TAG_KVGT;
  walk->pos += len;
  return 1;
}
