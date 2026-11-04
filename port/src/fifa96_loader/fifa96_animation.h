#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_ANIM_ROW_COUNT 0x6Fu
#define FIFA96_ANIM_ROW_SIZE 9u
#define FIFA96_ANIM_FRAME_SIZE 5u
#define FIFA96_ANIM_NEXT_REPEAT 0x6Fu

typedef struct fifa96_anim_row {
  uint8_t index;
  uint8_t anim_id;
  uint8_t last_frame;
  uint8_t flags;
  uint8_t next_id;
  uint32_t frame_table;
  uint8_t sprite_bank;
} fifa96_anim_row;

fifa96_err_t fifa96_animation_row_lookup(const uint8_t *rows, uint32_t row_count, int32_t anim_id,
                                         fifa96_anim_row *out);

typedef struct fifa96_anim_flags {
  uint8_t terminal;
  uint8_t bit1;
  uint8_t bit2;
  int32_t lean;
} fifa96_anim_flags;

fifa96_err_t fifa96_animation_flags(uint8_t row_flags, fifa96_anim_flags *out);

typedef struct fifa96_anim_frame {
  uint8_t index;
  uint16_t duration;
  uint16_t aux;
  uint8_t sprite;
} fifa96_anim_frame;

fifa96_err_t fifa96_animation_frame(const uint8_t *frames, int32_t frame_index, uint8_t last_frame,
                                    fifa96_anim_frame *out);

typedef struct fifa96_anim_advance {
  uint16_t timer;
  uint16_t delta;
  uint8_t frame_index;
  int8_t turn;
} fifa96_anim_advance;

typedef struct fifa96_anim_advance_out {
  uint8_t advanced;
  uint8_t frame_index;
  uint16_t timer;
} fifa96_anim_advance_out;

fifa96_err_t fifa96_animation_advance(fifa96_anim_advance *state, uint16_t duration,
                                      uint8_t last_frame, fifa96_anim_advance_out *out);
fifa96_err_t fifa96_animation_turn(int32_t heading, int32_t facing, int8_t *turn);
fifa96_err_t fifa96_animation_successor(uint8_t next_id, uint8_t current_id, uint8_t *anim_id,
                                        uint8_t *face);
