#include "fifa96_loader/fifa96_huff.h"

#include <string.h>

typedef struct {
  const uint8_t *p;
  size_t nbits;
  size_t pos;
} huff_bits;

static int hb_peek(const huff_bits *b, unsigned n, uint32_t *out) {
  if (n > 32 || b->pos + n > b->nbits) return -1;
  uint32_t v = 0;
  for (unsigned i = 0; i < n; i++) {
    size_t bit = b->pos + i;
    v = (v << 1) | ((uint32_t)(b->p[bit >> 3] >> (7 - (bit & 7))) & 1u);
  }
  *out = v;
  return 0;
}

static int hb_read(huff_bits *b, unsigned n, uint32_t *out) {
  if (hb_peek(b, n, out)) return -1;
  b->pos += n;
  return 0;
}

// FU-24 §4: `0^j || binary(v)`, v >= 4, bitlen(v) = j+3, 2j+3 bits total.
static int hb_value(huff_bits *b, uint32_t *out) {
  uint32_t bit;
  if (hb_read(b, 1, &bit)) return -1;
  if (bit) {  // 1xx: v = 4..7
    uint32_t x;
    if (hb_read(b, 2, &x)) return -1;
    *out = 4 + x;
    return 0;
  }
  unsigned j = 1;
  for (;;) {
    if (j > 24) return -1;  // malformed: no plausible value this long
    if (hb_read(b, 1, &bit)) return -1;
    if (bit) break;
    j++;
  }
  uint32_t x;
  if (hb_read(b, j + 2, &x)) return -1;
  *out = (1u << (j + 2)) | x;
  return 0;
}

static uint32_t huff_be24(const uint8_t *p) {
  return ((uint32_t)p[0] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[2];
}

int fifa96_huff_decode(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_cap, size_t *out_len) {
  if (!src || !dst || !out_len) return -(int)FIFA96_ERR_TRUNCATED;
  *out_len = 0;
  if (src_len < 6) return -(int)FIFA96_ERR_TRUNCATED;

  // FU-24 §3: bit0 of stream[0] selects the 4/8-byte header.
  size_t bitstart;
  uint32_t declared;
  uint8_t special;
  if (src[0] & 1u) {
    if (src_len < 9) return -(int)FIFA96_ERR_TRUNCATED;
    declared = huff_be24(src + 5);
    special = src[8];
    bitstart = 9;
  } else {
    declared = huff_be24(src + 2);
    special = src[5];
    bitstart = 6;
  }
  if (dst_cap < declared) return -(int)FIFA96_ERR_TRUNCATED;
  uint8_t selector = src[0] & 0xFEu;

  huff_bits br = {src, src_len * 8, bitstart * 8};

  // FU-24 §5(a): code-length counts.
  uint32_t fc[18] = {0}, bc[18] = {0}, cum[18] = {0};
  uint32_t p = 0, n = 0;
  int maxlen = 0;
  for (int l = 1;; l++) {
    if (l > 16) return -(int)FIFA96_ERR_TRUNCATED;
    p *= 2;
    bc[l] = p - n;
    uint32_t v;
    if (hb_value(&br, &v)) return -(int)FIFA96_ERR_TRUNCATED;
    uint32_t cnt = v - 4;
    fc[l] = cnt;
    p += cnt;
    n += cnt;
    uint32_t cuml = cnt ? ((p << (16 - l)) & 0xFFFFu) : 0u;
    cum[l] = cuml;
    if (cnt != 0 && cuml == 0) {
      maxlen = l;
      break;
    }
  }
  cum[maxlen] = 0xFFFFFFFFu;  // sentinel: long-code search always terminates
  uint32_t total = n;
  if (total == 0 || total > 256) return -(int)FIFA96_ERR_TRUNCATED;

  // FU-24 §5(b): symbol order (1-based counts of unused symbols, circular).
  uint8_t used[256], order[256];
  memset(used, 0, sizeof used);
  int cur = 0xFF;
  for (uint32_t i = 0; i < total; i++) {
    uint32_t v;
    if (hb_value(&br, &v)) return -(int)FIFA96_ERR_TRUNCATED;
    uint32_t adv = v - 3;
    if (adv == 0 || adv > 256) return -(int)FIFA96_ERR_TRUNCATED;
    unsigned guard = 0;
    while (adv) {
      cur = (cur + 1) & 0xFF;
      if (!used[cur]) adv--;
      if (++guard > 256) return -(int)FIFA96_ERR_TRUNCATED;
    }
    used[cur] = 1;
    order[i] = (uint8_t)cur;
  }

  // FU-24 §5(c): 8-bit prefix tables.
  uint8_t len53[256], sym43[256];
  memset(len53, 0x40, sizeof len53);
  int special_len = -1;
  size_t pos = 0, oi = 0;
  for (int l = 1; l <= 8 && l <= maxlen; l++) {
    for (uint32_t k = 0; k < fc[l]; k++) {
      if (oi >= total) return -(int)FIFA96_ERR_TRUNCATED;
      uint8_t sym = order[oi++];
      size_t cnt = (size_t)1u << (8 - l);
      if (pos + cnt > 256) return -(int)FIFA96_ERR_TRUNCATED;
      for (size_t j = 0; j < cnt; j++) {
        len53[pos] = (sym == special) ? 0x60 : (uint8_t)l;
        sym43[pos] = sym;
        pos++;
      }
      if (sym == special && special_len < 0) special_len = l;
    }
  }

  // FU-24 §6/§7: decode loop. `last` mirrors the original's [op-1] read for
  // the repeat-last-byte run; logical writes past `declared` are dropped.
  size_t op = 0;
  uint8_t last = 0;
  for (;;) {
    uint32_t top8;
    if (hb_peek(&br, 8, &top8)) return -(int)FIFA96_ERR_TRUNCATED;
    uint8_t marker = len53[top8];
    uint8_t sym;
    if (marker == 0x40) {
      // Long code: find L with (top16) < cum[L] (FU-24 §6).
      if (maxlen < 9) return -(int)FIFA96_ERR_TRUNCATED;  // unfilled prefix
      uint32_t top16;
      if (hb_peek(&br, 16, &top16)) return -(int)FIFA96_ERR_TRUNCATED;
      int l = 0;
      for (int k = 9; k <= maxlen; k++) {
        if (top16 < cum[k]) {
          l = k;
          break;
        }
      }
      if (l == 0) return -(int)FIFA96_ERR_TRUNCATED;
      uint32_t code;
      if (hb_read(&br, (unsigned)l, &code)) return -(int)FIFA96_ERR_TRUNCATED;
      if (code < bc[l]) return -(int)FIFA96_ERR_TRUNCATED;
      uint32_t idx = code - bc[l];
      if (idx >= total) return -(int)FIFA96_ERR_TRUNCATED;
      sym = order[idx];
    } else if (marker == 0x60) {
      if (special_len < 1) return -(int)FIFA96_ERR_TRUNCATED;
      uint32_t dummy;
      if (hb_read(&br, (unsigned)special_len, &dummy)) return -(int)FIFA96_ERR_TRUNCATED;
      sym = special;
    } else {
      sym = sym43[top8];
      uint32_t dummy;
      if (hb_read(&br, marker, &dummy)) return -(int)FIFA96_ERR_TRUNCATED;
    }

    if (sym != special) {
      if (op < declared) dst[op] = sym;
      op++;
      last = sym;
      continue;
    }
    // Special: FU-24 §7 (run / escape / end).
    uint32_t v;
    if (hb_value(&br, &v)) return -(int)FIFA96_ERR_TRUNCATED;
    uint32_t r = v - 4;
    if (r != 0) {
      if (op == 0) return -(int)FIFA96_ERR_TRUNCATED;  // original reads [op-1]
      for (uint32_t i = 0; i < r; i++) {
        if (op < declared) dst[op] = last;
        op++;
      }
      continue;
    }
    uint32_t flag;
    if (hb_read(&br, 1, &flag)) return -(int)FIFA96_ERR_TRUNCATED;
    if (flag) break;  // end marker
    uint32_t lit;
    if (hb_read(&br, 8, &lit)) return -(int)FIFA96_ERR_TRUNCATED;
    if (op < declared) dst[op] = (uint8_t)lit;
    op++;
    last = (uint8_t)lit;
  }
  if (op < declared) return -(int)FIFA96_ERR_TRUNCATED;

  // FU-24 §8: 0x32FB/0x34FB byte prefix-sum transforms over the declared range.
  if (selector == 0x32u) {
    uint8_t acc = 0;
    for (uint32_t i = 0; i < declared; i++) {
      acc = (uint8_t)(acc + dst[i]);
      dst[i] = acc;
    }
  } else if (selector == 0x34u) {
    uint8_t first = 0, second = 0;
    for (uint32_t i = 0; i < declared; i++) {
      first = (uint8_t)(first + dst[i]);
      second = (uint8_t)(second + first);
      dst[i] = second;
    }
  }

  *out_len = declared;
  return FIFA96_OK;
}
