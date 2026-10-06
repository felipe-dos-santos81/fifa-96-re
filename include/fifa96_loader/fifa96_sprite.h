#pragma once
#include <stddef.h>
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_sprite_bank {
  const uint8_t *data;
  uint32_t total_size;
  uint32_t count;
  uint8_t tag[4];
} fifa96_sprite_bank;

typedef struct fifa96_sprite_entry {
  uint8_t name[4];
  uint32_t offset;
} fifa96_sprite_entry;

typedef struct fifa96_sprite_frame {
  uint8_t tag;
  uint32_t second_offset;
  uint16_t width;
  uint16_t height;
  uint16_t pivot_x;
  uint16_t pivot_y;
  uint32_t word12;
  const uint8_t *pixels;
  uint32_t pixel_len;
} fifa96_sprite_frame;

typedef struct fifa96_sprite_chunk {
  const uint8_t *data;
  uint32_t length;
  uint8_t type;
} fifa96_sprite_chunk;

fifa96_err_t fifa96_sprite_bank_parse(const uint8_t *data, size_t len, fifa96_sprite_bank *out);
fifa96_err_t fifa96_sprite_bank_entry(const fifa96_sprite_bank *bank, uint32_t index,
                                      fifa96_sprite_entry *out);
fifa96_err_t fifa96_sprite_frame_parse(const fifa96_sprite_bank *bank, uint32_t offset,
                                       fifa96_sprite_frame *out);
fifa96_err_t fifa96_sprite_chunk_parse(const fifa96_sprite_bank *bank, uint32_t offset,
                                       fifa96_sprite_chunk *out);
fifa96_err_t fifa96_sprite_chunk_palette(const fifa96_sprite_chunk *chunk,
                                         const uint8_t **rgb6, uint16_t *count);
fifa96_err_t fifa96_sprite_palette_to_rgb(const uint8_t *rgb6, uint16_t count, uint8_t *rgb8);
int32_t fifa96_sprite_stride(uint32_t bank_index, uint32_t count);
fifa96_err_t fifa96_sprite_columns(int32_t *cols, uint32_t count, int32_t col, int32_t col_step,
                                   int32_t row, int32_t row_step, uint32_t width);
fifa96_err_t fifa96_sprite_span(uint8_t *dst, const uint8_t *src, size_t src_len,
                                const int32_t *cols, uint32_t count, const uint8_t *remap);
