#include "fifa96_loader/fifa96_eacs.h"

static uint32_t eacs_u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* The decoders' accumulator clamp (0xC4D05/C4D0D): keep the 32-bit lane in
 * signed 16-bit range; unlike the output converter both bounds are used. */
static int32_t eacs_clamp16(int32_t v) {
  if (v > 0x7fff) return 0x7fff;
  if (v < -0x8000) return -0x8000;
  return v;
}

/* ADAPT row update clamp (0xC4D26/C4D2D): byte offset into DELTA, 0..0x1600. */
static uint32_t eacs_clamp_row(int32_t v) {
  if (v > 0x1600) return 0x1600u;
  if (v < 0) return 0u;
  return (uint32_t)v;
}

int fifa96_eacs_parse(const uint8_t *src, size_t src_len, struct fifa96_eacs_info *info) {
  if (!src || !info) return -(int)FIFA96_ERR_TRUNCATED;
  if (src_len < 0x20) return -(int)FIFA96_ERR_TRUNCATED;
  // The original's chunk length is a u32; larger buffers are outside its model.
  if (src_len > (size_t)UINT32_MAX) return -(int)FIFA96_ERR_UNSUPPORTED;
  if (!(src[0] == 'E' && src[1] == 'A' && src[2] == 'C' && src[3] == 'S'))
    return -(int)FIFA96_ERR_BAD_MAGIC;
  int8_t voice = (int8_t)src[0x0B];
  // FU-35 §2: the 1SNh video path requires 0..15. FU-41 §2: bank entries
  // store -1, the game's bank path never validates the byte and uses its sign
  // as the signed/unsigned flag selector; the runtime voice is supplied
  // separately at arm time (header +0x1C). So -1 is accepted as the bank-form
  // marker, any other out-of-range value is still rejected.
  if (voice < -1 || voice > 15) return -(int)FIFA96_ERR_TRUNCATED;
  uint32_t block_size = (uint32_t)src[0x08] * (uint32_t)src[0x09];
  if (block_size == 0) return -(int)FIFA96_ERR_TRUNCATED;  // original would DIV by zero
  // Video chunks store 0 and the parser force-writes payload+0x20; bank .spc
  // headers store the literal 0x20 that the game relocates to header+0x20
  // just before arming (FU-41 §2). Both resolve to data_off 0x20 here; a
  // nonzero offset is honored after bounds-checking.
  uint32_t data_ptr = eacs_u32le(src + 0x18);
  uint32_t data_off = data_ptr ? data_ptr : 0x20u;
  if (data_off < 0x20u || (size_t)data_off > src_len) return -(int)FIFA96_ERR_TRUNCATED;

  struct fifa96_eacs_info out;
  out.rate = eacs_u32le(src + 0x04);
  out.f8 = src[0x08];
  out.f9 = src[0x09];
  out.f10 = src[0x0A];
  out.voice = voice;
  out.count = eacs_u32le(src + 0x0C);
  out.loop_start = (int32_t)eacs_u32le(src + 0x10);
  out.loop_len = eacs_u32le(src + 0x14);
  out.data_ptr = data_ptr;
  out.volume = src[0x1D];
  out.data_off = data_off;
  out.data_len = (uint32_t)(src_len - data_off);
  out.block_size = block_size;
  out.blocks = out.data_len / block_size;
  out.samples = (uint64_t)out.blocks * (out.f10 == 2u ? 4u : 1u);
  out.delta_units = 0;
  if (out.f10 == 2u) {
    /* FU-39 §1/§3: f8=2,f9=2 selects the stereo nibble decoder (byte = L/R
     * frame); f8=2,f9=1 the mono one (nibble per sample). */
    if (out.f8 == 2u && out.f9 == 2u) {
      out.format = FIFA96_EACS_FMT_DELTA_STEREO;
    } else if (out.f8 == 2u && out.f9 == 1u) {
      out.format = FIFA96_EACS_FMT_DELTA_MONO;
    } else {
      out.format = FIFA96_EACS_FMT_UNKNOWN;
    }
  } else if (out.f8 == 2u && out.f9 == 2u) {
    out.format = FIFA96_EACS_FMT_PCM16_STEREO;
  } else if (out.f8 == 1u && out.f9 == 2u) {
    out.format = FIFA96_EACS_FMT_PCM8_STEREO;
  } else if (out.f8 == 2u && out.f9 == 1u) {
    out.format = FIFA96_EACS_FMT_PCM16_MONO;
  } else {
    out.format = FIFA96_EACS_FMT_UNKNOWN;
  }
  if (out.format == FIFA96_EACS_FMT_DELTA_STEREO) {
    /* FU-39 §2.1: the signed/video producer 0xB84FE parses the 20-byte block
     * header at data_off; its count is one decoder unit (one packed byte)
     * per stereo frame. */
    if (out.data_len < 0x14u) return -(int)FIFA96_ERR_TRUNCATED;
    uint32_t count = eacs_u32le(src + data_off);
    if (count > out.data_len - 0x14u) return -(int)FIFA96_ERR_TRUNCATED;
    out.delta_units = count;
  } else if (out.format == FIFA96_EACS_FMT_DELTA_MONO) {
    /* FU-39 §2.2: the unsigned/bank producer 0xB8610 skips the block header
     * and takes the declared +0x0C count (nibbles, two per byte at data_off;
     * FU-35 §6.1 measured 2 per byte, with trailing slack of 2-6 nibbles,
     * FU-39 §6). The bank loader's in-memory data mapping stays FU-39 §7
     * leg 3 and is not invented here. */
    if ((uint64_t)out.count > (uint64_t)out.data_len * 2u)
      return -(int)FIFA96_ERR_TRUNCATED;
    out.delta_units = out.count;
  }
  *info = out;
  return FIFA96_OK;
}

int fifa96_eacs_delta_header(const uint8_t *src, size_t src_len,
                             struct fifa96_eacs_delta *st, uint32_t *count) {
  if (!src || !st || !count) return -(int)FIFA96_ERR_TRUNCATED;
  if (src_len < 0x14u) return -(int)FIFA96_ERR_TRUNCATED;
  struct fifa96_eacs_delta out;
  /* FU-39 §2.1: ch+0x30 = (u32[4] << 6) | (u32[8] << 22), so the row code is
   * stored as a row index and becomes a DELTA byte offset (row << 6). The
   * static port clamps malformed codes to the last row (the decoder only
   * clamps after ADAPT). */
  out.l_row = ((uint32_t)eacs_u32le(src + 4) << 6) & 0xffffu;
  out.r_row = ((uint32_t)eacs_u32le(src + 8) << 6) & 0xffffu;
  if (out.l_row > 0x1600u) out.l_row = 0x1600u;
  if (out.r_row > 0x1600u) out.r_row = 0x1600u;
  out.l_acc = (int16_t)(uint16_t)eacs_u32le(src + 0xc);
  out.r_acc = (int16_t)(uint16_t)eacs_u32le(src + 0x10);
  *st = out;
  *count = eacs_u32le(src);
  return FIFA96_OK;
}

int fifa96_eacs_delta_unit(struct fifa96_eacs_delta *st, uint8_t f9,
                           const uint8_t *data, uint32_t unit,
                           int16_t *out_l, int16_t *out_r) {
  if (!st || !data) return -(int)FIFA96_ERR_TRUNCATED;
  if (f9 != 2u && f9 != 1u) return -(int)FIFA96_ERR_UNSUPPORTED;
  const int32_t *delta = fifa96_eacs_delta_table();
  const int32_t *adapt = fifa96_eacs_adapt_table();
  int16_t l, r;
  if (f9 == 2u) {
    /* 0xC4D6C: high nibble updates L and writes word[dest], low nibble R. */
    uint8_t byte = data[unit];
    uint32_t n = byte >> 4;
    uint32_t row = st->l_row > 0x1600u ? 0x1600u : st->l_row;
    st->l_acc = eacs_clamp16(st->l_acc + delta[row / 4u + n]);
    st->l_row = eacs_clamp_row((int32_t)row + adapt[n & 7u]);
    l = (int16_t)st->l_acc;
    n = byte & 0xfu;
    row = st->r_row > 0x1600u ? 0x1600u : st->r_row;
    st->r_acc = eacs_clamp16(st->r_acc + delta[row / 4u + n]);
    st->r_row = eacs_clamp_row((int32_t)row + adapt[n & 7u]);
    r = (int16_t)st->r_acc;
  } else {
    /* 0xC4CC4: even cursor takes the high nibble, odd the low one and the
     * source byte advances; one sample on the single (L) lane, duplicated by
     * the mono reader 0xB8AE1. The bank arm skips the block header entirely
     * (FU-39 §2.2); nonzero R state is malformed there and ignored. */
    uint8_t byte = data[unit >> 1];
    uint32_t n = (unit & 1u) ? (byte & 0xfu) : (byte >> 4);
    uint32_t row = st->l_row > 0x1600u ? 0x1600u : st->l_row;
    st->l_acc = eacs_clamp16(st->l_acc + delta[row / 4u + n]);
    st->l_row = eacs_clamp_row((int32_t)row + adapt[n & 7u]);
    l = r = (int16_t)st->l_acc;
  }
  if (out_l) *out_l = l;
  if (out_r) *out_r = r;
  return FIFA96_OK;
}

int fifa96_eacs_delta_block(const uint8_t *src, size_t src_len, uint8_t f9,
                            struct fifa96_eacs_delta *st,
                            int16_t *out, size_t out_frames, uint32_t *units) {
  if (!src || !st) return -(int)FIFA96_ERR_TRUNCATED;
  if (f9 != 2u && f9 != 1u) return -(int)FIFA96_ERR_UNSUPPORTED;
  if (src_len < 0x14u) return -(int)FIFA96_ERR_TRUNCATED;
  struct fifa96_eacs_delta next;
  uint32_t count = 0;
  int rc = fifa96_eacs_delta_header(src, src_len, &next, &count);
  if (rc != FIFA96_OK) return rc;
  size_t avail = src_len - 0x14u;
  size_t need = f9 == 2u ? (size_t)count : (size_t)(count >> 1) + (count & 1u);
  if (need > avail) return -(int)FIFA96_ERR_TRUNCATED;
  if (out && out_frames < count) return -(int)FIFA96_ERR_TRUNCATED;
  for (uint32_t i = 0; i < count; i++) {
    int16_t l = 0, r = 0;
    rc = fifa96_eacs_delta_unit(&next, f9, src + 0x14, i, &l, &r);
    if (rc != FIFA96_OK) return rc;
    if (out) {
      out[2u * i] = l;
      out[2u * i + 1u] = r;
    }
  }
  *st = next;
  if (units) *units = count;
  return FIFA96_OK;
}
