#include "fifa96_loader/fifa96_qfs.h"
#include "fifa96_loader/fifa96_file.h"
fifa96_err_t fifa96_qfs_parse_hdr(const uint8_t *b, size_t n, fifa96_qfs_hdr_t *h) {
  if (!b || !h || n < 12) return FIFA96_ERR_TRUNCATED;
  if (b[0] != 0x10 || b[1] != 0xfb) return FIFA96_ERR_BAD_MAGIC;
  h->magic[0] = b[0]; h->magic[1] = b[1];
  h->dec_len = fifa96_read_u32le(b + 2);
  h->tag[0] = (char)b[6]; h->tag[1] = (char)b[7];
  h->tag[2] = (char)b[8]; h->tag[3] = (char)b[9];
  return FIFA96_OK;
}
