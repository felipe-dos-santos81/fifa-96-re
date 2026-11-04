#include "fifa96_loader/fifa96_record.h"

#include <string.h>

#include "fifa96_loader/fifa96_huff.h"
#include "fifa96_loader/fifa96_refpack.h"
#include "fifa96_loader/fifa96_tree.h"

// FU-27: literal-copy arm (0x9E825 / memmove_bytes 0xCD390): BE24 length at
// src+2, payload at src+5, source-first argument order, return = length.
static int record_literal_copy(const uint8_t *src, size_t src_len, uint8_t *dst,
                               size_t dst_cap, size_t *out_len) {
  if (src_len < 5) return -(int)FIFA96_ERR_TRUNCATED;
  size_t len = ((size_t)src[2] << 16) | ((size_t)src[3] << 8) | src[4];
  if (src_len < 5 + len) return -(int)FIFA96_ERR_TRUNCATED;
  if (dst_cap < len) return -(int)FIFA96_ERR_TRUNCATED;
  memmove(dst, src + 5, len);
  *out_len = len;
  return FIFA96_OK;
}

int fifa96_record_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  if (!src || !dst || !out_len) return -(int)FIFA96_ERR_TRUNCATED;
  *out_len = 0;
  if (src_len < 2) return -(int)FIFA96_ERR_TRUNCATED;
  if (src[1] != 0xFB) return -(int)FIFA96_ERR_BAD_MAGIC;
  uint8_t selector = src[0] & 0xFEu;
  switch (selector) {
    case 0x10:  // refpack (raw 0x10/0x11)
      return fifa96_refpack_decode(src, src_len, dst, dst_cap, out_len);
    case 0x30:  // huff (raw 0x30..0x35)
    case 0x32:
    case 0x34:
      return fifa96_huff_decode(src, src_len, dst, dst_cap, out_len);
    case 0x46:  // tree (raw 0x46/0x47)
      return fifa96_tree_decode(src, src_len, dst, dst_cap, out_len);
    case 0x6A:  // literal copy (raw 0x6A/0x6B) and its twin 0x6E/0x6F
    case 0x6E:
      return record_literal_copy(src, src_len, dst, dst_cap, out_len);
    case 0x16:  // lz_16fb: not ported
    case 0x60:  // delta_prefix (0x60/0x62/0x66/0x72): not ported
    case 0x62:
    case 0x66:
    case 0x72:
    case 0x7A:  // rle_row: not ported
      return -(int)FIFA96_ERR_UNSUPPORTED;
    default:  // 0x9E840 error arm (strict = 1)
      return -(int)FIFA96_ERR_BAD_MAGIC;
  }
}
