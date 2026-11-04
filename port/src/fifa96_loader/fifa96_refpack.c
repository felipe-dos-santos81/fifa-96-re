#include "fifa96_loader/fifa96_refpack.h"

static uint32_t refpack_be24(const uint8_t *p) {
  return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
}

int fifa96_refpack_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  if (!src || !dst || !out_len) return -(int)FIFA96_ERR_TRUNCATED;
  *out_len = 0;
  if (src_len < 5) return -(int)FIFA96_ERR_TRUNCATED;
  size_t cur = 2;
  // FU-22 §3/§8: raw 0x11 form skips 3 extra header bytes; no committed vector.
  if (src[0] & 1u) {
    if (src_len < 8) return -(int)FIFA96_ERR_TRUNCATED;
    cur += 3;
  }
  size_t declared = refpack_be24(src + cur);
  cur += 3;
  if (dst_cap < declared) return -(int)FIFA96_ERR_TRUNCATED;
  // FU-22 §6: forward byte copy is the exact behaviour of MOVSB.REP with the
  // platform ABI's DF = 0, including overlapping (distance 0) replication.
  // FU-22 §9: the original returns the declared length but writes past it by up
  // to a command's net output; writes at/past `declared` are clamped here and
  // `op` tracks the original's logical cursor so distances stay comparable.
  size_t op = 0;
  for (;;) {
    if (cur >= src_len) return -(int)FIFA96_ERR_TRUNCATED;  // no 0xFC..0xFF stop
    uint8_t c = src[cur];
    if (c >= 0xFCu) {
      // End marker: c&3 final literals at ctrl+1 (FU-22 §5).
      size_t lit = c & 3u;
      if (cur + 1 + lit > src_len) return -(int)FIFA96_ERR_TRUNCATED;
      for (size_t i = 0; i < lit; i++) {
        if (op < declared) dst[op] = src[cur + 1 + i];
        op++;
      }
      break;
    }
    size_t lit = 0, match = 0, dist = 0, base = 0, consumed = 0;
    if (c < 0x80u) {
      // Class A 0x00-0x7F (FU-22 §5).
      lit = c & 3u;
      consumed = 2 + lit;
      if (cur + consumed > src_len) return -(int)FIFA96_ERR_TRUNCATED;
      match = ((size_t)(c & 0x1Cu) >> 2) + 3;
      dist = ((size_t)((c & 0xE0u) >> 5) << 8) | src[cur + 1];
      base = cur + 2;
    } else if (c < 0xC0u) {
      // Class B 0x80-0xBF (FU-22 §5).
      if (cur + 3 > src_len) return -(int)FIFA96_ERR_TRUNCATED;
      uint8_t b1 = src[cur + 1];
      lit = b1 >> 6;
      consumed = 3 + lit;
      if (cur + consumed > src_len) return -(int)FIFA96_ERR_TRUNCATED;
      match = (size_t)(c & 0x3Fu) + 4;
      dist = ((size_t)(b1 & 0x3Fu) << 8) | src[cur + 2];
      base = cur + 3;
    } else if (c < 0xE0u) {
      // Class C 0xC0-0xDF: FU-22 §10 open leg — disassembly-derived formula with
      // zero vector coverage (record-10 has 0 class C commands). The cited
      // field ops give length (((c>>2)&3)<<8 | b3) + 5 (max 1028; §8's "773"
      // range annotation omits b3).
      lit = c & 3u;
      consumed = 4 + lit;
      if (cur + consumed > src_len) return -(int)FIFA96_ERR_TRUNCATED;
      uint8_t b1 = src[cur + 1], b2 = src[cur + 2], b3 = src[cur + 3];
      dist = ((size_t)(c & 0x10u) << 12) | ((size_t)b1 << 8) | b2;
      match = ((((size_t)c & 0x0Cu) << 6) | b3) + 5;
      base = cur + 4;
    } else {
      // Class D 0xE0-0xFB: (c&0x1F)+1 literal dwords at ctrl+1 (FU-22 §5).
      lit = 4u * ((c & 0x1Fu) + 1u);
      consumed = 1 + lit;
      if (cur + consumed > src_len) return -(int)FIFA96_ERR_TRUNCATED;
      base = cur + 1;
    }
    for (size_t i = 0; i < lit; i++) {
      if (op < declared) dst[op] = src[base + i];
      op++;
    }
    if (match) {
      // Back-reference source is op-1-dist; a source before the output start is
      // malformed (the original would read pre-buffer memory).
      if (dist >= op) return -(int)FIFA96_ERR_TRUNCATED;
      for (size_t i = 0; i < match; i++) {
        if (op < declared) dst[op] = dst[op - 1 - dist];
        op++;
      }
    }
    cur += consumed;
  }
  *out_len = declared;
  return FIFA96_OK;
}
