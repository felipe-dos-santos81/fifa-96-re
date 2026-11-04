#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

#define FIFA96_RENDER_CAM_BOUND 0x1770
#define FIFA96_RENDER_CAM_RESET (-0xFA0)
#define FIFA96_RENDER_CAM_Y_MAX 0x3E80
#define FIFA96_RENDER_HIDDEN_Y (-10000)

typedef struct fifa96_render_pos {
  int32_t x;
  int32_t y;
  int32_t z;
} fifa96_render_pos;

typedef struct fifa96_render_entity {
  fifa96_render_pos pos;
  int32_t heading;
  uint8_t anim_id;
  uint8_t frame;
  uint8_t hidden;
} fifa96_render_entity;

typedef struct fifa96_render_slot {
  fifa96_render_pos pos;
  int32_t angle;
  uint8_t anim_id;
  uint8_t frame;
  uint8_t hidden;
} fifa96_render_slot;

fifa96_err_t fifa96_render_camera_stage(const fifa96_render_pos *src, fifa96_render_pos *cam);
fifa96_err_t fifa96_render_slot_stage(const fifa96_render_entity *e, fifa96_render_slot *slot);

typedef struct fifa96_render_bank {
  const uint8_t *base;
  const int32_t *offsets;
  uint32_t count;
  int32_t step;
} fifa96_render_bank;

const uint8_t *fifa96_render_bank_at(const fifa96_render_bank *bank, int32_t index);

typedef struct fifa96_render_frame {
  const uint8_t *sprite;
  const uint8_t *overlay;
  int32_t offset;
  int32_t overlay_offset;
  uint8_t mirrored;
} fifa96_render_frame;

fifa96_err_t fifa96_render_resolve(uint8_t anim_id, int32_t frame_index, int32_t direction,
                                   const uint8_t *frames, uint8_t bank_index,
                                   const fifa96_render_bank *banks, uint32_t bank_count,
                                   const fifa96_render_bank *fixed60,
                                   const fifa96_render_bank *fixed61, const uint8_t *mirror,
                                   fifa96_render_frame *out);

typedef struct fifa96_render_clip {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
} fifa96_render_clip;

fifa96_err_t fifa96_render_place(int32_t sprite_w, int32_t sprite_h, int32_t pivot_x,
                                 int32_t pivot_y, int32_t dest_w, int32_t dest_h, int32_t x,
                                 int32_t y, int32_t *out_x, int32_t *out_y);

typedef struct fifa96_render_cover {
  int32_t dst_x;
  int32_t dst_y;
  int32_t dst_w;
  int32_t dst_h;
  int32_t src_x;
  int32_t src_y;
  int32_t src_dx;
  int32_t src_dy;
  uint8_t mirrored_x;
  uint8_t mirrored_y;
} fifa96_render_cover;

fifa96_err_t fifa96_render_cover_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                                      int32_t sprite_w, int32_t sprite_h,
                                      const fifa96_render_clip *clip, fifa96_render_cover *out,
                                      uint8_t *visible);
