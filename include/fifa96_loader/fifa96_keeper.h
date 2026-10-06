#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

typedef struct fifa96_dispatch_record {
  uint8_t skip_9a;
} fifa96_dispatch_record;

typedef struct fifa96_dispatch_team {
  const fifa96_dispatch_record *records;
  uint32_t count;
} fifa96_dispatch_team;

typedef struct fifa96_dispatch_iter {
  const fifa96_dispatch_team *teams;
  uint32_t team_count;
  uint32_t team_index;
  uint32_t record_index;
} fifa96_dispatch_iter;

typedef struct fifa96_keeper_state {
  uint16_t timer;
  uint8_t phase;
  uint8_t controlled;
  uint8_t own_type_5;
  uint8_t opponent_controlled;
  uint8_t opponent_type_5;
} fifa96_keeper_state;

int fifa96_dispatch_begin(fifa96_dispatch_iter *iter, const fifa96_dispatch_team *teams,
                          uint32_t team_count);
int fifa96_dispatch_next(fifa96_dispatch_iter *iter, uint32_t *team_index,
                         uint32_t *record_index, int *is_keeper);
int fifa96_keeper_select_action(const fifa96_keeper_state *state, uint8_t type_gate,
                                uint8_t current_action, uint8_t *next_action);

typedef struct fifa96_keeper_point {
  int32_t x;
  int32_t y;
  int32_t z;
} fifa96_keeper_point;

typedef struct fifa96_keeper_vec {
  int16_t distance;
  int16_t dx;
  int16_t dz;
} fifa96_keeper_vec;

typedef struct fifa96_keeper_dive {
  fifa96_keeper_point target;
  uint8_t install_code;
  uint8_t invoke;
} fifa96_keeper_dive;

typedef struct fifa96_keeper_arm_out {
  uint8_t stage;
  uint8_t event_code;
  uint8_t install_code;
  uint8_t invoke;
  uint8_t copy_pos;
  uint8_t run_handler;
  uint8_t reset;
  uint8_t flag_9e;
} fifa96_keeper_arm_out;

typedef struct fifa96_keeper_rep_a_out {
  uint8_t install_code;
  uint8_t reset;
  uint8_t stage_advance;
  uint8_t flag_9e;
} fifa96_keeper_rep_a_out;

fifa96_err_t fifa96_keeper_distance(int16_t x, int16_t z, int16_t *distance);
fifa96_err_t fifa96_keeper_vec_from_delta(const fifa96_keeper_point *from,
                                          const fifa96_keeper_point *to,
                                          fifa96_keeper_vec *out);
fifa96_err_t fifa96_keeper_clear_vector(uint32_t rng_x, uint32_t rng_z, int8_t range_attr,
                                        uint8_t side, fifa96_keeper_vec *out);
fifa96_err_t fifa96_keeper_claim_place(const fifa96_keeper_point *pos, int8_t offset_x,
                                       int8_t offset_z, uint8_t stage, uint8_t has_ball,
                                       uint8_t has_slot, fifa96_keeper_point *place,
                                       uint8_t *helper_request, uint8_t *claimed);
int fifa96_keeper_hold_track(int16_t deepest_x, int16_t candidate_x);
fifa96_err_t fifa96_keeper_hold_guard(const fifa96_keeper_point *cam, uint8_t side,
                                      fifa96_keeper_point *out);
fifa96_err_t fifa96_keeper_hold_intercept(const fifa96_keeper_point *cam, int32_t distance,
                                          int16_t carrier_vx, int16_t carrier_vz,
                                          fifa96_keeper_point *out);
fifa96_err_t fifa96_keeper_guard_clamp(fifa96_keeper_point *p, uint8_t side);
fifa96_err_t fifa96_keeper_dive_target(const fifa96_keeper_point *pos, uint32_t rng_a,
                                       uint32_t divisor, uint32_t rng_b, int16_t trajectory_z,
                                       uint32_t rng_c, uint32_t rng_d,
                                       fifa96_keeper_dive *out);
fifa96_err_t fifa96_keeper_arm_step(uint8_t stage, uint8_t event_flag, uint8_t ball_actor,
                                    uint8_t ball_flag, uint8_t has_slot, uint8_t slot_pressed,
                                    uint8_t type8, uint8_t phase_latch,
                                    fifa96_keeper_arm_out *out);
fifa96_err_t fifa96_keeper_reposition_a_gate(uint8_t has_tracked_teammate,
                                             fifa96_keeper_rep_a_out *out);
fifa96_err_t fifa96_keeper_reposition_b_finish(uint8_t is_controlled, uint8_t human_side,
                                               uint8_t *install_code, uint8_t *notify_code);
fifa96_err_t fifa96_keeper_lunge_track(const fifa96_keeper_vec *delta, int16_t angle,
                                       fifa96_keeper_point *pos, uint8_t *on_target,
                                       int16_t *steer_x, int16_t *steer_z, uint8_t *event_code);
