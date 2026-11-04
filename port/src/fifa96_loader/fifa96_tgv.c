#include "fifa96_loader/fifa96_tgv.h"
#include "fifa96_loader/fifa96_file.h"
fifa96_err_t fifa96_tgv_parse_hdr(const uint8_t *b, size_t n, fifa96_tgv_hdr_t *h) {
  if (!b || !h || n < 8) return FIFA96_ERR_TRUNCATED;
  if (!(b[0] == 0x6b && b[1] == 0x56 && b[2] == 0x47 && b[3] == 0x54)) return FIFA96_ERR_BAD_MAGIC;
  h->magic[0] = 'k'; h->magic[1] = 'V'; h->magic[2] = 'G'; h->magic[3] = 'T';
  h->v0 = fifa96_read_u32le(b + 4);
  return FIFA96_OK;
}
