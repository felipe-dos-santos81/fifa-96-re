#include "fifa96_loader/fifa96_pog.h"
fifa96_err_t fifa96_pog_parse_hdr(const uint8_t *b, size_t n, fifa96_pog_hdr_t *h) {
  if (!b || !h || n < 8) return FIFA96_ERR_TRUNCATED;
  if (b[0] != 0x10 || b[1] != 0xfb) return FIFA96_ERR_BAD_MAGIC;
  h->magic[0] = b[0]; h->magic[1] = b[1];
  h->w1 = (uint16_t)(b[2] | (b[3] << 8));
  h->w2 = (uint32_t)(b[4] | (b[5] << 8) | (b[6] << 16) | (b[7] << 24));
  return FIFA96_OK;
}
