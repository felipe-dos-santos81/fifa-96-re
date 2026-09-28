#include "fifa96_loader/fifa96_pog.h"
#include "fifa96_loader/fifa96_file.h"
fifa96_err_t fifa96_pog_parse_hdr(const uint8_t *b, size_t n, fifa96_pog_hdr_t *h) {
  if (!b || !h || n < 8) return FIFA96_ERR_TRUNCATED;
  if (b[0] != 0x10 || b[1] != 0xfb) return FIFA96_ERR_BAD_MAGIC;
  h->magic[0] = b[0]; h->magic[1] = b[1];
  h->w1 = fifa96_read_u16le(b + 2);
  h->w2 = fifa96_read_u32le(b + 4);
  return FIFA96_OK;
}
