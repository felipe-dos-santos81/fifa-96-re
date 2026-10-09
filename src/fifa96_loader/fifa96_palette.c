#include "fifa96_loader/fifa96_palette.h"

#include <string.h>

/* First-hand image tables: 0x10727C (11 bytes) and 0x107287 (7 bytes), the
 * rank maps the FUN_00048DC0 kit path adds the local bases to. */
static const uint8_t palette_band_wide[11] = {0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2};
static const uint8_t palette_band_narrow[7] = {0, 0, 0, 0, 1, 1, 1};

fifa96_err_t fifa96_palette_pool_partition(uint8_t *base, uint32_t size,
                                           fifa96_palette_pool *out) {
  if (!base || !out || size < 0x3000u) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  out->base = base;
  out->size = size;
  /* FUN_00046F80 0x46F84: 23 x 0x100 at 0x14BF60. */
  for (uint32_t i = 0; i < 23u; i++) out->slots23[i] = base + i * 0x100u;
  /* 0x46F99: 7 x 0x100 at 0x14BF34, base+0x1700. */
  for (uint32_t i = 0; i < 7u; i++) out->slots7[i] = base + 0x1700u + i * 0x100u;
  /* 0x46FAE: 8 x 0x100 at 0x14BB00, base+0x2400. */
  for (uint32_t i = 0; i < 8u; i++) out->slots8[i] = base + 0x2400u + i * 0x100u;
  /* 0x46FC1/0x47045: the shared table (0x14BB20[0..255] and 0x14BFBC). */
  out->shared = base + 0x2600u;
  /* 0x46FC8..0x47018: the nine fixed blocks in native assignment order. */
  out->fixed[0] = base + 0x2700u;   /* 0x14BF50 */
  out->fixed[1] = base + 0x2800u;   /* 0x14BF20 */
  out->fixed[2] = base + 0x2900u;   /* 0x14BF54 */
  out->fixed[3] = base + 0x2A00u;   /* 0x14BF2C */
  out->fixed[4] = base + 0x2B00u;   /* 0x14BF28 */
  out->fixed[5] = base + 0x2C00u;   /* 0x14BF30 */
  out->fixed[6] = base + 0x2D00u;   /* 0x14BF5C */
  out->fixed[7] = base + 0x2E00u;   /* 0x14BF58 */
  out->fixed[8] = base + 0x2F00u;   /* 0x14BF24 */
  return FIFA96_OK;
}

fifa96_err_t fifa96_palette_translate_kit(const uint8_t *src, uint8_t *dst,
                                          uint32_t entity) {
  uint32_t narrow_lo, wide_lo, narrow_base, wide_base;
  if (!src || !dst) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (entity == 0u) {
    narrow_lo = 0x94u;   /* [0x94,0x9B) */
    narrow_base = 0xA1u;
    wide_lo = 0x9Bu;     /* [0x9B,0xA6) */
    wide_base = 0xA3u;
  } else if (entity == 0xBu) {
    narrow_lo = 0x82u;   /* [0x82,0x89) */
    narrow_base = 0x9Cu;
    wide_lo = 0x89u;     /* [0x89,0x94) */
    wide_base = 0x9Eu;
  } else {
    return (fifa96_err_t)-FIFA96_ERR_INVALID;
  }
  memcpy(dst, src, 256u);
  for (uint32_t i = 0; i < 256u; i++) {
    uint32_t value = src[i];
    if (value >= narrow_lo && value < narrow_lo + 7u)
      dst[i] = (uint8_t)(palette_band_narrow[value - narrow_lo] + narrow_base);
    else if (value >= wide_lo && value < wide_lo + 11u)
      dst[i] = (uint8_t)(palette_band_wide[value - wide_lo] + wide_base);
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_palette_translate_slot(uint8_t *dst, const uint8_t *slot) {
  if (!dst || !slot) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  memcpy(dst, slot, 256u);
  return FIFA96_OK;
}
