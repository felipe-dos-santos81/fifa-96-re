#include "fifa96_loader/fifa96_fvgt.h"

#include <stdlib.h>
#include <string.h>

// FU-33 §1/§2: vgt_dispatch routes tag 'fVGT' (LE32 0x54475666) to
// vgt_decode_f; the four header counts are the high halves of dword loads,
// i.e. plain LE16 fields.
#define FVGT_TAG 0x54475666u
#define FVGT_HEADER 0x14u

static uint32_t fvgt_le16(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t fvgt_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// FU-33 §10 leg 5: the unpackers load 32-bit dwords at the tail and can read
// up to 3 bytes past their region; only mask-extraneous bits are affected, so
// zero-filling past the chunk is equivalent and keeps every read in bounds.
static uint32_t fvgt_le32_bounded(const uint8_t *base, size_t avail, size_t off) {
  uint32_t v = 0;
  for (size_t i = 0; i < 4u; i++) {
    if (off + i < avail) v |= (uint32_t)base[off + i] << (8u * i);
  }
  return v;
}

static uint32_t fvgt_ror32(uint32_t v, unsigned r) {
  r &= 31u;
  return r ? ((v >> r) | (v << (32u - r))) : v;
}

// FU-33 §3 (unpack_signed_fields @ 0xADD60): descending bit cursor with a
// descending store, so dst[i] is the field at bit offset i*bits, LSB-first.
// The caller uses count = 2*width and bits = 10.
static void fvgt_unpack_signed_fields(int32_t *dst, const uint8_t *src,
                                      size_t avail, size_t count, unsigned bits) {
  uint32_t mask = ((uint32_t)1u << bits) - 1u;
  uint32_t sign = (uint32_t)1u << (bits - 1u);
  for (size_t i = count; i > 0u; i--) {
    size_t f = i - 1u;
    size_t bit = f * (size_t)bits;
    uint32_t v = (fvgt_le32_bounded(src, avail, bit >> 3) >> (unsigned)(bit & 7u)) & mask;
    if (v & sign) v |= ~mask;  // FU-33 §3 sign extension
    dst[f] = (int32_t)v;
  }
}

// FU-33 §5 (expand_palette_block @ 0xADDF0): each 8-byte record is a 4-colour
// palette (src[0..3]) plus 16 two-bit indices in LE32(src+4), MSB pair first.
static void fvgt_expand_palette(const uint8_t *src, uint8_t *dst, size_t count) {
  for (size_t r = 0; r < count; r++) {
    uint32_t idx = fvgt_le32(src + 4);
    for (unsigned k = 0; k < 16u; k++) {
      dst[k] = src[(idx >> (30u - 2u * k)) & 3u];
    }
    src += 8;
    dst += 16;
  }
}

// FU-33 §6 (unpack_block_indices @ 0xBA8F0): LSB-first `bits`-wide block ids.
// A field at bit offset B is ROR(LE32(src + B/8), B&7) & mask; the x86 mask
// is (1 << (bits & 31)) - 1 because SHL takes the shift count modulo 32. For
// fields whose start and width stay inside one dword the rotation matches the
// plain stream; a field crossing a dword boundary would wrap the dword's low
// bits (no committed vector, FU-33 §10 leg 5).
static void fvgt_unpack_block_indices(uint32_t *dst, const uint8_t *src,
                                      size_t avail, size_t count, unsigned bits) {
  uint32_t mask = ((uint32_t)1u << (bits & 31u)) - 1u;
  for (size_t i = 0; i < count; i++) {
    size_t bit = i * (size_t)bits;
    uint32_t v = fvgt_ror32(fvgt_le32_bounded(src, avail, bit >> 3),
                            (unsigned)(bit & 7u)) & mask;
    dst[i] = v;
  }
}

// FU-33 §7 (composite_4x4_blocks @ 0xBA994). Delta ids (< width) copy 4x4
// pixels from `pre` at dst_cursor + expanded[id] (a byte-column and row
// displacement, FU-33 §4); block ids copy four rows of four pixels from the
// raw/palette buffer. The original has no bounds checks (FU-33 §10 leg 2), so
// validate every reference before writing: the destination then stays
// untouched for a malformed chunk.
static int fvgt_composite(const uint32_t *indices, uint32_t width,
                          const int32_t *expanded, const uint8_t *blocks,
                          size_t block_count, const uint8_t *pre, uint8_t *dst,
                          size_t pitch, size_t bands, size_t cols,
                          size_t canvas_size) {
  for (size_t i = 0; i < bands * cols; i++) {
    size_t off = (i / cols) * 4u * pitch + (i % cols) * 4u;
    uint32_t id = indices[i];
    if (id < width) {
      int64_t src_off = (int64_t)off + expanded[id];
      if (src_off < 0 ||
          (uint64_t)src_off + (uint64_t)3u * pitch + 4u > (uint64_t)canvas_size) {
        return -(int)FIFA96_ERR_TRUNCATED;
      }
    } else if ((size_t)(id - width) >= block_count) {
      return -(int)FIFA96_ERR_TRUNCATED;
    }
  }
  for (size_t band = 0; band < bands; band++) {
    for (size_t col = 0; col < cols; col++) {
      size_t off = band * 4u * pitch + col * 4u;
      uint32_t id = *indices++;
      uint8_t *d = dst + off;
      if (id < width) {
        const uint8_t *s = pre + (size_t)((int64_t)off + expanded[id]);
        // FU-33 §7: source rows run at the canvas pitch.
        for (unsigned r = 0; r < 4u; r++) memcpy(d + r * pitch, s + r * pitch, 4);
      } else {
        const uint8_t *s = blocks + (size_t)(id - width) * 16u;
        // FU-33 §7: a 16-byte block holds four consecutive 4-pixel rows.
        for (unsigned r = 0; r < 4u; r++) memcpy(d + r * pitch, s + r * 4u, 4);
      }
    }
  }
  return 0;
}

int fifa96_fvgt_decode(const uint8_t *chunk, size_t chunk_len,
                       uint32_t canvas_w, uint32_t canvas_h,
                       const uint8_t *pre, uint8_t *dst, size_t dst_cap,
                       size_t *out_len) {
  if (!chunk || !pre || !dst || !out_len) return -(int)FIFA96_ERR_TRUNCATED;
  *out_len = 0;
  if (chunk_len < FVGT_HEADER) return -(int)FIFA96_ERR_TRUNCATED;
  if (fvgt_le32(chunk) != FVGT_TAG) return -(int)FIFA96_ERR_BAD_MAGIC;

  uint32_t width = fvgt_le16(chunk + 8);    // ctx[4], delta id threshold
  uint32_t raw = fvgt_le16(chunk + 10);     // ctx[5], raw 16-byte blocks
  uint32_t pal = fvgt_le16(chunk + 12);     // ctx[6], 8-byte palette records
  uint32_t bits = fvgt_le16(chunk + 14);    // ctx[7], row-stream entry width

  // Geometry is the keyframe's ctx[0]/ctx[1] (FU-33 §4/§10 leg 1). The
  // compositor runs four rows / four columns per iteration, so a dimension
  // that is not a multiple of 4 leaves a canvas tail the original never
  // rewrites (0xBA9C5/0xBA9D0 counters); the port refuses instead of
  // returning a partly undefined canvas. >2^31-1 bytes exceeds the
  // original's 32-bit pointer arithmetic.
  if (canvas_w == 0u || canvas_h == 0u) return -(int)FIFA96_ERR_TRUNCATED;
  if ((canvas_w & 3u) != 0u || (canvas_h & 3u) != 0u) return -(int)FIFA96_ERR_UNSUPPORTED;
  uint64_t canvas_size64 = (uint64_t)canvas_w * (uint64_t)canvas_h;
  if (canvas_size64 > (uint64_t)INT32_MAX) return -(int)FIFA96_ERR_UNSUPPORTED;
  size_t canvas_size = (size_t)canvas_size64;
  if (dst_cap < canvas_size) return -(int)FIFA96_ERR_TRUNCATED;

  // 0xAE1BC TEST EDX,EDX / JZ skips unpack_block_indices when the row-stream
  // size is zero (bits == 0), leaving ctx[0xc] holding the previous kVGT
  // keyframe's ids -- runtime state the chunk cannot supply (FU-33 §10 leg 1).
  if (bits == 0u) return -(int)FIFA96_ERR_UNSUPPORTED;

  // FU-33 §2 chunk-size equation (width*20 for the 2*width 10-bit fields).
  size_t block_count = (size_t)raw + (size_t)pal;
  size_t row_count = canvas_size / 16u;
  uint64_t table_bytes64 = ((uint64_t)width * 20u + 31u) & ~(uint64_t)31u;
  uint64_t row_bytes64 = ((uint64_t)bits * row_count + 31u) & ~(uint64_t)31u;
  uint64_t need64 = (uint64_t)FVGT_HEADER + (table_bytes64 >> 3) +
                    (uint64_t)raw * 16u + (uint64_t)pal * 8u + (row_bytes64 >> 3);
  if (need64 > (uint64_t)chunk_len) return -(int)FIFA96_ERR_TRUNCATED;
  size_t table_bytes = (size_t)(table_bytes64 >> 3);

  int rc = 0;
  int32_t *fields = NULL;
  int32_t *expanded = NULL;
  uint32_t *indices = NULL;
  uint8_t *blocks = NULL;
  if (width != 0u) {
    fields = (int32_t *)malloc((size_t)width * 2u * sizeof(int32_t));
    expanded = (int32_t *)malloc((size_t)width * sizeof(int32_t));
    if (!fields || !expanded) { rc = -(int)FIFA96_ERR_IO; goto done; }
  }
  if (block_count != 0u) {
    blocks = (uint8_t *)malloc(block_count * 16u);
    if (!blocks) { rc = -(int)FIFA96_ERR_IO; goto done; }
  }
  indices = (uint32_t *)malloc(row_count * sizeof(uint32_t));
  if (!indices) { rc = -(int)FIFA96_ERR_IO; goto done; }

  const uint8_t *p = chunk + FVGT_HEADER;
  if (width != 0u) {
    fvgt_unpack_signed_fields(fields, p, chunk_len - FVGT_HEADER,
                              (size_t)width * 2u, 10u);
  }
  p += table_bytes;
  if (raw != 0u) memcpy(blocks, p, (size_t)raw * 16u);
  p += (size_t)raw * 16u;
  if (pal != 0u) fvgt_expand_palette(p, blocks + (size_t)raw * 16u, pal);
  p += (size_t)pal * 8u;
  fvgt_unpack_block_indices(indices, p, chunk_len - (size_t)(p - chunk),
                            row_count, bits);

  if (width != 0u) {
    for (size_t j = 0; j < width; j++) {
      int32_t a = fields[2u * j];
      int32_t b = fields[2u * j + 1u];
      // FU-33 §4/§10 leg 2: rowtab[height+b] is only defined for
      // b in [-height, height); with it, the value is b*pitch.
      if (b < -(int32_t)canvas_h || b >= (int32_t)canvas_h) {
        rc = -(int)FIFA96_ERR_TRUNCATED;
        goto done;
      }
      // a + b*pitch in 32-bit arithmetic, matching the original's wrapping.
      expanded[j] = (int32_t)((uint32_t)a + (uint32_t)b * canvas_w);
    }
  }

  rc = fvgt_composite(indices, width, expanded, blocks, block_count, pre, dst,
                      canvas_w, canvas_h / 4u, canvas_w / 4u, canvas_size);
  if (rc == 0) *out_len = canvas_size;

done:
  free(fields);
  free(expanded);
  free(indices);
  free(blocks);
  return rc;
}
