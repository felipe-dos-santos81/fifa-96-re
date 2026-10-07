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

/* FU-138 §3: row 02 stage-0 wait (native `0x7E0B0..0x7E0F0`). `lane` is the
 * native dword `[rec+0x69]>>16` (FU-75 lane word); `ready` fires at timer `0x78`
 * when `lane > 0x40`, else at timer `0xA`; the `lane > 0x40` ready path is the
 * one the native pairs with `FUN_0007DAB4` (`reset`). */
fifa96_err_t fifa96_action_locomotion_restart_wait(int16_t lane, int32_t timer89,
                                                   uint8_t *ready, uint8_t *reset);

/* FU-138 §3: row 03 placement counter (native `0x7E2E0..0x7E2F4`):
 * `counter = min((move_attr >> 22) + 1, limit)` over the unsigned 16-bit
 * words the native writes to `rec+0x7B`/`rec+0x79`. */
fifa96_err_t fifa96_action_locomotion_placement_counter(int32_t move_attr, uint16_t limit,
                                                        uint16_t *counter);

/* FU-138 §3: row 03 phase-1 side clamp (native `0x7E3CA..0x7E417`): side 0 caps
 * the target z at `-0x20`, side 1 floors it at `+0x20`. */
fifa96_err_t fifa96_action_locomotion_phase1_clamp(uint8_t side, int32_t *target_z);

typedef struct fifa96_action_possession {
  int32_t carrier;
  uint8_t index;
  uint8_t rotation;
  int8_t dir_x;
  int8_t dir_z;
  uint8_t counter_c;
  int8_t release_timer;
  uint8_t counter_e;
  uint8_t counter_f;
} fifa96_action_possession;

fifa96_err_t fifa96_action_possession_reset(fifa96_action_possession *state);
fifa96_err_t fifa96_action_possession_claim(fifa96_action_possession *state, int32_t actor,
                                            int *claimed);
fifa96_err_t fifa96_action_possession_timer(int32_t *timer, uint16_t delta);

typedef struct fifa96_action_dribble_dir {
  int8_t dir_x;
  int8_t dir_z;
  uint16_t speed;
  uint8_t resolved;
} fifa96_action_dribble_dir;

fifa96_err_t fifa96_action_possession_dribble_dir(uint8_t type8, int32_t distance, uint8_t has_slot,
                                                  int8_t slot_x, int8_t slot_z,
                                                  const int8_t *type_x, const int8_t *type_z,
                                                  fifa96_action_dribble_dir *out);

typedef struct fifa96_action_receive {
  int32_t timer89;
  int16_t offset_word;
  uint8_t stage;
  uint8_t active;
  uint8_t event_flag;
  uint8_t is_team_target;
} fifa96_action_receive;

typedef struct fifa96_action_receive_out {
  uint8_t reset;
  uint8_t advance;
  uint8_t handoff;
  uint8_t stage;
} fifa96_action_receive_out;

fifa96_err_t fifa96_action_receive_step(fifa96_action_receive *state,
                                        fifa96_action_receive_out *out);

typedef struct fifa96_action_tackle {
  int32_t pos_x;
  int32_t pos_z;
  int32_t camera_x;
  int32_t target_x;
  int32_t target_z;
  int32_t vector_x;
  int32_t vector_z;
  int32_t timer89;
  int16_t target_height;
  int16_t cam_f8;
  int16_t close_word;
  int16_t opp_close;
  int16_t opp_bound;
  int16_t own_bound;
  int16_t cam_f2;
  int16_t cam_fa;
  int16_t cam_100;
  int16_t cam_fe;
  uint16_t facing;
  uint16_t delta;
  uint8_t phase;
  uint8_t is_tracked;
  uint8_t active;
  uint8_t side;
  uint8_t stage;
  uint8_t lob;
  uint8_t has_slot;
  uint8_t slot_button_40;
  uint8_t flag99;
  uint8_t field5d;
  uint8_t is_own;
} fifa96_action_tackle;

typedef struct fifa96_action_tackle_out {
  uint8_t reset;
  uint8_t install_0e;
  uint8_t install_0f;
  uint8_t stage;
} fifa96_action_tackle_out;

fifa96_err_t fifa96_action_tackle_attempt(const fifa96_action_tackle *state, int *install_0e);
fifa96_err_t fifa96_action_tackle_step(fifa96_action_tackle *state, fifa96_action_tackle_out *out);

typedef struct fifa96_action_duel {
  int32_t timer89;
  int32_t pos_x;
  int32_t pos_z;
  int16_t distance;
  int16_t delta_x;
  int16_t delta_z;
  uint8_t stage;
  uint8_t animation;
  uint8_t has_slot;
  uint8_t stride;
} fifa96_action_duel;

typedef struct fifa96_action_duel_out {
  uint8_t wait;
  uint8_t handoff;
  uint8_t reset;
  uint8_t stage;
} fifa96_action_duel_out;

fifa96_err_t fifa96_action_duel_step(fifa96_action_duel *state, uint16_t delta, uint8_t input_byte,
                                     fifa96_action_duel_out *out);
fifa96_err_t fifa96_action_duel_split(int16_t own_metric, int16_t opp_metric,
                                      uint8_t opp_is_duel_type, uint8_t *own_code,
                                      uint8_t *opp_code);

typedef struct fifa96_action_stage {
  uint8_t phase;
  uint8_t stage;
  uint8_t active;
  uint8_t occupied;
  int32_t timer89;
  uint16_t delta;
} fifa96_action_stage;

typedef struct fifa96_action_stage_out {
  uint8_t allowed;
  uint8_t reset;
  uint8_t advance;
  uint8_t stage;
} fifa96_action_stage_out;

fifa96_err_t fifa96_action_stage_enter(fifa96_action_stage *state, const uint8_t *gates,
                                       uint8_t gate_count, fifa96_action_stage_out *out);
fifa96_err_t fifa96_action_stage_tick(fifa96_action_stage *state);
fifa96_err_t fifa96_action_stage_advance(fifa96_action_stage *state, fifa96_action_stage_out *out);
fifa96_err_t fifa96_action_stage_finish(fifa96_action_stage *state, fifa96_action_stage_out *out);
fifa96_err_t fifa96_action_stage_marker(int32_t marker, uint8_t *set_leader, uint8_t *hold);

/* FU-138 §3: shared timed-arm gate (native row 01 `0x7DC80`, row 0E `0x8276A`,
 * row 12 `0x83E26`): an arm advances once its accumulated `+0x89` timer reaches
 * the arm's threshold. */
fifa96_err_t fifa96_action_stage_wait(int32_t timer89, int32_t threshold, uint8_t *ready);
fifa96_err_t fifa96_action_phase_select(uint8_t phase, const uint32_t *table, uint32_t count,
                                        uint32_t *entry);

typedef struct fifa96_action_phase_record {
  uint32_t handler;
  uint8_t active;
  uint8_t reserved[3];
} fifa96_action_phase_record;

fifa96_err_t fifa96_action_phase_install(fifa96_action_phase_record *records, uint32_t count,
                                         uint8_t phase, const uint32_t *table,
                                         uint32_t table_count);
fifa96_err_t fifa96_action_phase_drive(uint8_t active, uint8_t *drive);
fifa96_err_t fifa96_action_phase_cell(int8_t x, int8_t z, uint8_t side, fifa96_action_vec3 *out);
fifa96_err_t fifa96_action_phase_slot(int16_t x, int16_t z, uint8_t side, fifa96_action_vec3 *out);
fifa96_err_t fifa96_action_phase_ball_entry(int32_t ball_z, uint8_t side, uint32_t *index);
fifa96_err_t fifa96_action_phase_ball_line(const int16_t *entries, int32_t ball_z, uint8_t side,
                                           fifa96_action_vec3 *out);
fifa96_err_t fifa96_action_phase_line_timer(int32_t *timer89, uint16_t delta, uint8_t *ready);
fifa96_err_t fifa96_action_phase_restart_line(uint8_t axis, int32_t offset, int16_t lateral,
                                              fifa96_action_vec3 *out);

#define FIFA96_ACTION_SEQUENCE_SCATTER_POINTS 5u

typedef struct fifa96_action_sequence_lane_out {
  int32_t z;
  int32_t x;
  int32_t threshold;
} fifa96_action_sequence_lane_out;

typedef struct fifa96_action_sequence_duel {
  uint8_t reset;
  uint8_t event_id;
} fifa96_action_sequence_duel;

typedef struct fifa96_action_sequence_press {
  uint8_t fire;
  uint8_t reset;
} fifa96_action_sequence_press;

fifa96_err_t fifa96_action_sequence_select(uint8_t stage, const uint32_t *arms, uint32_t count,
                                           uint32_t *arm);
fifa96_err_t fifa96_action_sequence_event(uint8_t anim_byte, uint8_t event_id, uint8_t *post);

/* FU-138 §3: row 01 marker target (native `0x7DBDC..0x7DC39`). When the marker
 * (`rec+0x8F >> 24`) is >= 2 the position triple is copied to the output;
 * otherwise the caller-supplied pre-reset x (`lead_x`, the native `rec+0x4D`
 * the unported camera-reset call leaves) selects the ±0x30 kickoff target with
 * y and z cleared. */
fifa96_err_t fifa96_action_sequence_marker_target(uint8_t marker,
                                                  const fifa96_action_vec3 *pos,
                                                  int32_t lead_x,
                                                  fifa96_action_vec3 *out);
fifa96_err_t fifa96_action_sequence_marker(uint8_t marker, uint8_t want, uint8_t *match);
fifa96_err_t fifa96_action_sequence_rng_event(uint32_t rng, uint8_t even_id, uint8_t odd_id,
                                              uint8_t *event_id);
fifa96_err_t fifa96_action_sequence_countdown(uint32_t rng, uint16_t *countdown);
fifa96_err_t fifa96_action_sequence_anim_byte(const uint8_t *table, uint8_t index,
                                              uint16_t *value);
fifa96_err_t fifa96_action_sequence_lane(uint8_t subtype, uint8_t side, int32_t boost,
                                         fifa96_action_sequence_lane_out *out);
fifa96_err_t fifa96_action_sequence_scatter_celebration(const fifa96_action_vec3 *base,
                                                        int32_t dir_x, int32_t dir_z,
                                                        const uint32_t *rng,
                                                        fifa96_action_vec3 *points);
fifa96_err_t fifa96_action_sequence_scatter_stats(const fifa96_action_vec3 *base, int32_t dir_x,
                                                  int32_t dir_z, const uint32_t *rng,
                                                  fifa96_action_vec3 *points);
fifa96_err_t fifa96_action_sequence_duel_event(int16_t delta_angle, int16_t aim, int16_t facing,
                                               int16_t atan_delta,
                                               fifa96_action_sequence_duel *out);
fifa96_err_t fifa96_action_sequence_press_event(int16_t height, int32_t timer89,
                                                fifa96_action_sequence_press *out);
fifa96_err_t fifa96_action_sequence_event_ids(const uint8_t *t344, const uint8_t *t346,
                                              const uint8_t *t349, const uint8_t *t34c,
                                              const uint32_t *rng, uint8_t *ids);

/* FU-138 §3: row 0D stage-0 velocity seed (native `0x825B2..0x82613`). The
 * caller-supplied `type_x`/`type_z` (the sign-extended `[0x10F334]`/`[0x10F33C]`
 * entries the native reads at `rec+0x8B>>24`) are scaled by 3 when both are
 * nonzero else by 4 into `rec+0x73`/`rec+0x75`; `speed` is their entity
 * distance (`FUN_0008DC68`). */
fifa96_err_t fifa96_action_sequence_velocity_scale(int16_t type_x, int16_t type_z,
                                                   int16_t *vel_x, int16_t *vel_z,
                                                   int16_t *speed);
