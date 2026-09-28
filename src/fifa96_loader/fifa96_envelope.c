#include "fifa96_loader/fifa96_envelope.h"
#include "fifa96_loader/fifa96_file.h"
#include <string.h>
fifa96_err_t fifa96_envelope_parse_hdr(const uint8_t *b, size_t n, fifa96_envelope_hdr_t *h) {
  if (!b || !h || n < 10) return FIFA96_ERR_TRUNCATED;
  if (b[0] != 0x10 || b[1] != 0xfb) return FIFA96_ERR_BAD_MAGIC;
  h->magic = fifa96_read_u16le(b);
  h->word_a = fifa96_read_u16le(b + 2);
  h->word_b = fifa96_read_u16le(b + 4);
  memcpy(h->tag, b + 6, 4);
  h->tail_off = 10;
  h->tail_len = n - 10;
  return FIFA96_OK;
}
