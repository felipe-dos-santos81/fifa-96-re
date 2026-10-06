#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_action_vec3 {
  int32_t x;
  int32_t y;
  int32_t z;
} fifa96_action_vec3;

fifa96_err_t fifa96_action_move_target(int32_t pos_x, int32_t pos_z, int8_t dir_x, int8_t dir_z,
                                       fifa96_action_vec3 *out);

typedef struct fifa96_action_move_state {
  int32_t timer89;
  uint16_t timer81;
  uint16_t delta;
  uint8_t phase;
  uint8_t active;
  uint8_t has_slot;
  int8_t dir_x;
  int8_t dir_z;
} fifa96_action_move_state;

typedef struct fifa96_action_move_out {
  uint8_t move;
  uint8_t install;
  uint8_t code;
} fifa96_action_move_out;

fifa96_err_t fifa96_action_move_step(fifa96_action_move_state *state, fifa96_action_move_out *out);

fifa96_err_t fifa96_action_kick_angle(int32_t x, int32_t z, int32_t *angle);

typedef struct fifa96_action_kick_row {
  uint16_t lo;
  uint16_t hi;
  uint16_t traj_add;
} fifa96_action_kick_row;

typedef struct fifa96_action_kick_ball {
  int16_t x;
  int16_t z;
  int16_t traj;
  int16_t comp_z;
} fifa96_action_kick_ball;

fifa96_err_t fifa96_action_kick_apply(fifa96_action_kick_ball *ball, const fifa96_action_kick_row *row,
                                      uint8_t mode, uint8_t event_code, uint8_t user_extend);

typedef struct fifa96_action_locomotion {
  int32_t pos_x;
  int32_t pos_z;
  int32_t target_x;
  int32_t target_z;
  int16_t delta_x;
  int16_t delta_z;
  int16_t distance;
  int16_t facing;
  int16_t desired_facing;
  int16_t speed;
  int16_t vel_x;
  int16_t vel_z;
  int16_t face_x;
  int16_t face_z;
  int32_t move_attr;
  int32_t stride_rate;
  uint8_t heading;
  uint8_t has_slot;
  uint8_t direct_face;
  uint8_t body_timer;
  uint8_t stride;
  uint8_t reserved;
  uint16_t delta;
} fifa96_action_locomotion;

fifa96_err_t fifa96_action_locomotion_step(fifa96_action_locomotion *state,
                                           const uint8_t *heading_table,
                                           const int16_t *stride_table);

fifa96_err_t fifa96_action_locomotion_restart_target(int32_t controlled_x,
                                                     int32_t *target_x, int32_t *target_z);

fifa96_err_t fifa96_action_locomotion_hold(uint8_t action_code,
                                           const fifa96_action_vec3 *pos,
                                           fifa96_action_vec3 *out, uint8_t *held);

fifa96_err_t fifa96_action_locomotion_clamp_placement(int32_t *target_z, int32_t pos_z,
                                                      int16_t bound_lo, int16_t bound_hi,
                                                      int32_t opponent_z, uint8_t side,
                                                      uint8_t settings_latch);

fifa96_err_t fifa96_action_locomotion_camera_lead(int32_t cam_x, int32_t cam_y, int32_t cam_z,
                                                  int16_t cam_vel_x, int16_t cam_vel_z,
                                                  fifa96_action_vec3 *out);
