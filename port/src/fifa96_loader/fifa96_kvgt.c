#include "fifa96_loader/fifa96_kvgt.h"

#include <string.h>

#include "fifa96_loader/fifa96_record.h"

#define KVGT_TAG 0x5447566Bu

// The disassembly reads `MOV EAX,[p+6]; SHR EAX,16`: the high half of a
// little-endian dword is its second u16 read little-endian, i.e. LE16(p+8).
static uint32_t kvgt_le16(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

int fifa96_kvgt_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len,
                       struct fifa96_kvgt_info *info) {
  if (!src || !dst || !out_len) return -(int)FIFA96_ERR_TRUNCATED;
  *out_len = 0;
  if (src_len < 0x14) return -(int)FIFA96_ERR_TRUNCATED;
  // vgt_dispatch reads the tag as a little-endian dword (*param_2 == 0x5447566B).
  uint32_t tag = (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
                 ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
  if (tag != KVGT_TAG) return -(int)FIFA96_ERR_BAD_MAGIC;

  // 0xAE22B..0xAE2A4: LE16 fields at +8/+10/+12/+14 (dword>>16 of +6/+8/+10/+12).
  uint32_t width = kvgt_le16(src + 8);
  uint32_t height = kvgt_le16(src + 10);
  uint32_t count = kvgt_le16(src + 12);
  uint32_t palette_count = kvgt_le16(src + 14);
  // The original's palette loop (0xAE2B3..0xAE2CE) has no bound; ctx+0x44 holds
  // 0x300 bytes (256 triples), so a larger count is malformed here.
  if (palette_count > 256) return -(int)FIFA96_ERR_TRUNCATED;

  size_t rec_off = 0x14 + (size_t)palette_count * 3;
  if (src_len < rec_off) return -(int)FIFA96_ERR_TRUNCATED;

  if (info) {
    info->width = width;
    info->height = height;
    info->count = count;
    info->palette_count = palette_count;
    if (palette_count) memcpy(info->palette, src + 0x14, (size_t)palette_count * 3);
    if (palette_count < 256) memset(info->palette + palette_count * 3, 0, (256 - palette_count) * 3);
  }

  // 0xAE45A..0xAE474: decode_record_strict(stream = frame + 0x14 + count*3, dst).
  return fifa96_record_decode(src + rec_off, src_len - rec_off, dst, dst_cap, out_len);
}
