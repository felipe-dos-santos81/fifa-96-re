#include "fifa96_loader/fifa96_eacs.h"

static uint32_t eacs_u32le(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int fifa96_eacs_parse(const uint8_t *src, size_t src_len, struct fifa96_eacs_info *info) {
  if (!src || !info) return -(int)FIFA96_ERR_TRUNCATED;
  if (src_len < 0x20) return -(int)FIFA96_ERR_TRUNCATED;
  // The original's chunk length is a u32; larger buffers are outside its model.
  if (src_len > (size_t)UINT32_MAX) return -(int)FIFA96_ERR_UNSUPPORTED;
  if (!(src[0] == 'E' && src[1] == 'A' && src[2] == 'C' && src[3] == 'S'))
    return -(int)FIFA96_ERR_BAD_MAGIC;
  int8_t voice = (int8_t)src[0x0B];
  // FU-35 §2: the 1SNh path rejects voice < 0 or > 15 (bank headers store -1
  // and take the voice as an argument in the separate bank loader, §6.5).
  if (voice < 0 || voice > 15) return -(int)FIFA96_ERR_TRUNCATED;
  uint32_t block_size = (uint32_t)src[0x08] * (uint32_t)src[0x09];
  if (block_size == 0) return -(int)FIFA96_ERR_TRUNCATED;  // original would DIV by zero
  // Video chunks store 0 and the parser force-writes payload+0x20; bank .spc
  // headers store the explicit offset (FU-35 §2).
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
  if (out.f8 == 2u && out.f9 == 2u) {
    out.format = FIFA96_EACS_FMT_PCM16_STEREO;
  } else if (out.f8 == 1u && out.f9 == 2u) {
    out.format = FIFA96_EACS_FMT_PCM8_STEREO;
  } else if (out.f8 == 2u && out.f9 == 1u) {
    out.format = FIFA96_EACS_FMT_PCM16_MONO;
  } else {
    out.format = FIFA96_EACS_FMT_UNKNOWN;
  }
  *info = out;
  return FIFA96_OK;
}
