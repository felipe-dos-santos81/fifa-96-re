#include "fifa96_loader/fifa96_animation.h"

fifa96_err_t fifa96_animation_row_lookup(const uint8_t *rows, uint32_t row_count, int32_t anim_id,
                                         fifa96_anim_row *out) {
  int16_t id;
  uint32_t index;
  if (!rows || !out) return -FIFA96_ERR_INVALID;
  id = (int16_t)anim_id;
  if (id < 0 || id >= (int16_t)FIFA96_ANIM_ROW_COUNT) id = 0;
  index = (uint32_t)id;
  if (index >= row_count) return -FIFA96_ERR_INVALID;
  out->index = (uint8_t)index;
  out->anim_id = rows[index * FIFA96_ANIM_ROW_SIZE + 0u];
  out->last_frame = rows[index * FIFA96_ANIM_ROW_SIZE + 1u];
  out->flags = rows[index * FIFA96_ANIM_ROW_SIZE + 2u];
  out->next_id = rows[index * FIFA96_ANIM_ROW_SIZE + 3u];
  out->frame_table = (uint32_t)rows[index * FIFA96_ANIM_ROW_SIZE + 4u] |
                     ((uint32_t)rows[index * FIFA96_ANIM_ROW_SIZE + 5u] << 8) |
                     ((uint32_t)rows[index * FIFA96_ANIM_ROW_SIZE + 6u] << 16) |
                     ((uint32_t)rows[index * FIFA96_ANIM_ROW_SIZE + 7u] << 24);
  out->sprite_bank = rows[index * FIFA96_ANIM_ROW_SIZE + 8u];
  return FIFA96_OK;
}

fifa96_err_t fifa96_animation_flags(uint8_t row_flags, fifa96_anim_flags *out) {
  if (!out) return -FIFA96_ERR_INVALID;
  out->terminal = row_flags & 0x01u;
  out->bit1 = row_flags & 0x02u;
  out->bit2 = row_flags & 0x04u;
  if (row_flags & 0x10u) out->lean = 2;
  else if (row_flags & 0x20u) out->lean = -2;
  else out->lean = 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_animation_frame(const uint8_t *frames, int32_t frame_index, uint8_t last_frame,
                                    fifa96_anim_frame *out) {
  int32_t index = frame_index;
  uint32_t offset;
  if (!frames || !out) return -FIFA96_ERR_INVALID;
  if (index < 0) index = (int32_t)last_frame;
  else if (index > (int32_t)last_frame) index = 0;
  offset = (uint32_t)index * FIFA96_ANIM_FRAME_SIZE;
  out->index = (uint8_t)index;
  out->duration = (uint16_t)(frames[offset + 0u] | ((uint16_t)frames[offset + 1u] << 8));
  out->aux = (uint16_t)(frames[offset + 2u] | ((uint16_t)frames[offset + 3u] << 8));
  out->sprite = frames[offset + 4u];
  return FIFA96_OK;
}

fifa96_err_t fifa96_animation_advance(fifa96_anim_advance *state, uint16_t duration,
                                      uint8_t last_frame, fifa96_anim_advance_out *out) {
  uint16_t timer;
  if (!state || !out) return -FIFA96_ERR_INVALID;
  timer = state->timer;
  out->advanced = 0;
  out->frame_index = state->frame_index;
  if (timer >= duration) {
    int32_t next = (int32_t)(int8_t)state->turn + (int32_t)(int8_t)state->frame_index;
    if (next < 0) next = (int32_t)last_frame;
    else if (next > (int32_t)last_frame) next = 0;
    state->frame_index = (uint8_t)next;
    out->frame_index = (uint8_t)next;
    out->advanced = 1;
    timer = 0;
  }
  timer = (uint16_t)(timer + (uint16_t)(state->delta << 4));
  state->timer = timer;
  out->timer = timer;
  return FIFA96_OK;
}

fifa96_err_t fifa96_animation_turn(int32_t heading, int32_t facing, int8_t *turn) {
  int32_t delta;
  if (!turn) return -FIFA96_ERR_INVALID;
  delta = (heading - facing) & 0x3FF;
  if (delta > 0x200) delta = 0x400 - delta;
  *turn = (delta < 0x100) ? 1 : -1;
  return FIFA96_OK;
}

fifa96_err_t fifa96_animation_successor(uint8_t next_id, uint8_t current_id, uint8_t *anim_id,
                                        uint8_t *face) {
  if (!anim_id || !face) return -FIFA96_ERR_INVALID;
  *face = 0;
  if (next_id == FIFA96_ANIM_NEXT_REPEAT) {
    *anim_id = current_id;
  } else if (next_id < 0x70u) {
    *anim_id = next_id;
  } else {
    *anim_id = (uint8_t)(next_id - 0x70u);
    *face = 1;
  }
  return FIFA96_OK;
}
