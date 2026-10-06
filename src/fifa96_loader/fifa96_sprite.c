#include "fifa96_loader/fifa96_sprite.h"
#include <string.h>
#include "fifa96_loader/fifa96_file.h"

fifa96_err_t fifa96_sprite_bank_parse(const uint8_t *data, size_t len, fifa96_sprite_bank *out) {
  if (!data || !out) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (len < 16) return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  if (memcmp(data, "SHPI", 4) != 0) return (fifa96_err_t)-FIFA96_ERR_BAD_MAGIC;
  uint32_t total = fifa96_read_u32le(data + 4);
  uint32_t count = fifa96_read_u32le(data + 8);
  if (total < 16 || total > len) return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  if ((uint64_t)16 + (uint64_t)count * 8 > total) return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  out->data = data;
  out->total_size = total;
  out->count = count;
  memcpy(out->tag, data + 12, 4);
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_bank_entry(const fifa96_sprite_bank *bank, uint32_t index,
                                      fifa96_sprite_entry *out) {
  if (!bank || !bank->data || !out) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (index >= bank->count) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  const uint8_t *p = bank->data + 16 + (size_t)index * 8;
  memcpy(out->name, p, 4);
  out->offset = fifa96_read_u32le(p + 4);
  if (out->offset > bank->total_size || bank->total_size - out->offset < 16)
    return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_frame_parse(const fifa96_sprite_bank *bank, uint32_t offset,
                                       fifa96_sprite_frame *out) {
  if (!bank || !bank->data || !out) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (offset > bank->total_size || bank->total_size - offset < 16)
    return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  const uint8_t *p = bank->data + offset;
  uint32_t second = (uint32_t)p[1] | ((uint32_t)p[2] << 8) | ((uint32_t)p[3] << 16);
  uint32_t w = fifa96_read_u16le(p + 4);
  uint32_t h = fifa96_read_u16le(p + 6);
  uint64_t pixel_len = (uint64_t)w * (uint64_t)h;
  if (pixel_len > (uint64_t)bank->total_size - offset - 16)
    return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  if (second != 0 && (second < 16 + pixel_len || second > bank->total_size))
    return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  out->tag = p[0];
  out->second_offset = second;
  out->width = (uint16_t)w;
  out->height = (uint16_t)h;
  out->pivot_x = fifa96_read_u16le(p + 8);
  out->pivot_y = fifa96_read_u16le(p + 10);
  out->word12 = fifa96_read_u32le(p + 12);
  out->pixels = p + 16;
  out->pixel_len = (uint32_t)pixel_len;
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_chunk_parse(const fifa96_sprite_bank *bank, uint32_t offset,
                                       fifa96_sprite_chunk *out) {
  if (!bank || !bank->data || !out) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  fifa96_sprite_frame frame;
  fifa96_err_t r = fifa96_sprite_frame_parse(bank, offset, &frame);
  if (r != FIFA96_OK) return r;
  if (frame.second_offset == 0) return (fifa96_err_t)-FIFA96_ERR_NOT_FOUND;
  uint32_t chunk = offset + frame.second_offset;
  if (chunk < offset || chunk >= bank->total_size) return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  out->data = bank->data + chunk;
  out->length = bank->total_size - chunk;
  out->type = out->data[0];
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_chunk_palette(const fifa96_sprite_chunk *chunk, const uint8_t **rgb6,
                                         uint16_t *count) {
  if (!chunk || !rgb6 || !count) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  if (chunk->type != 0x22) return (fifa96_err_t)-FIFA96_ERR_BAD_MAGIC;
  if (chunk->length < 16) return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  uint32_t n = fifa96_read_u16le(chunk->data + 4);
  if ((uint64_t)16 + (uint64_t)n * 3 > chunk->length)
    return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
  *rgb6 = chunk->data + 16;
  *count = (uint16_t)n;
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_palette_to_rgb(const uint8_t *rgb6, uint16_t count, uint8_t *rgb8) {
  if (count != 0 && (!rgb6 || !rgb8)) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  for (uint32_t i = 0; i < count; i++)
    for (uint32_t c = 0; c < 3; c++)
      rgb8[i * 3 + c] = (uint8_t)(((uint32_t)rgb6[i * 3 + c] * 0xFFu) / 0x3Fu);
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_palette_kit_remap(uint8_t *rgb6, uint16_t count) {
  if (!rgb6 || count < 166) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  static const uint8_t dst[10] = {132, 135, 140, 143, 146, 150, 153, 158, 161, 164};
  static const uint8_t src[10] = {156, 157, 158, 159, 160, 161, 162, 163, 164, 165};
  for (int i = 0; i < 10; i++)
    for (int c = 0; c < 3; c++)
      rgb6[dst[i] * 3 + c] = rgb6[src[i] * 3 + c];
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_palette_to_bgra(const uint8_t *rgb6, uint16_t count, uint8_t *bgra8) {
  if (count != 0 && (!rgb6 || !bgra8)) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  for (uint32_t i = 0; i < count; i++) {
    bgra8[i * 4] = (uint8_t)(((uint32_t)rgb6[i * 3 + 2] * 0xFFu) / 0x3Fu);
    bgra8[i * 4 + 1] = (uint8_t)(((uint32_t)rgb6[i * 3 + 1] * 0xFFu) / 0x3Fu);
    bgra8[i * 4 + 2] = (uint8_t)(((uint32_t)rgb6[i * 3] * 0xFFu) / 0x3Fu);
    bgra8[i * 4 + 3] = (i == 0xFF) ? 0 : 0xFF;
  }
  return FIFA96_OK;
}

int32_t fifa96_sprite_stride(uint32_t bank_index, uint32_t count) {
  int32_t divisor = 5;
  switch (bank_index) {
    case 0x18:
    case 0x19:
    case 0x25:
    case 0x26:
    case 0x28:
    case 0x29:
    case 0x2C:
    case 0x2D:
      divisor = 4;
      break;
    case 0x22:
    case 0x37:
    case 0x3A:
    case 0x3E:
    case 0x41:
    case 0x45:
    case 0x4E:
    case 0x56:
    case 0x5A:
      divisor = 3;
      break;
    case 0x23:
    case 0x38:
    case 0x3B:
    case 0x3F:
    case 0x42:
    case 0x46:
    case 0x4F:
    case 0x57:
    case 0x5B:
      divisor = 2;
      break;
    case 0x2E:
      divisor = 8;
      break;
    case 0x27:
    case 0x2A:
      return 8;
    case 0x2F:
      return 5;
  }
  return (int32_t)count / divisor;
}

fifa96_err_t fifa96_sprite_columns(int32_t *cols, uint32_t count, int32_t col, int32_t col_step,
                                   int32_t row, int32_t row_step, uint32_t width) {
  if (!cols && count != 0) return (fifa96_err_t)-FIFA96_ERR_INVALID;
  uint32_t pairs = (count + 1) / 2;
  for (uint32_t k = 0; k < pairs; k++) {
    int32_t next_row = (int32_t)((uint32_t)row + (uint32_t)row_step);
    int32_t next_col = (int32_t)((uint32_t)col + (uint32_t)col_step);
    uint32_t v0 = (uint32_t)(row >> 16) * width + (uint32_t)(col >> 16);
    uint32_t v1 = (uint32_t)(next_row >> 16) * width + (uint32_t)(next_col >> 16);
    cols[(size_t)k * 2] = (int32_t)v0;
    cols[(size_t)k * 2 + 1] = (int32_t)v1;
    row = (int32_t)((uint32_t)next_row + (uint32_t)row_step);
    col = (int32_t)((uint32_t)next_col + (uint32_t)col_step);
  }
  return FIFA96_OK;
}

fifa96_err_t fifa96_sprite_span(uint8_t *dst, const uint8_t *src, size_t src_len,
                                const int32_t *cols, uint32_t count, const uint8_t *remap) {
  if (count != 0 && (!dst || !src || !cols || !remap))
    return (fifa96_err_t)-FIFA96_ERR_INVALID;
  for (uint32_t i = 0; i < count; i++) {
    int32_t c = cols[i];
    if (c < 0 || (size_t)c >= src_len) return (fifa96_err_t)-FIFA96_ERR_TRUNCATED;
    uint8_t v = remap[src[c]];
    if (v != 0xFF) dst[i] = v;
  }
  return FIFA96_OK;
}
