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

/* FU-139 §3.5: the FUN_0007AE70 event-row resolver (native 0x7AE79..0x7B018)
 * selects one 10-byte event row from one of four native tables; the returned
 * `table` id names the native base (HEIGHT 0x1102FE, CARRY 0x11016E, ACTIVE
 * 0x110196, IDLE 0x11024A) and `index` is the row index (row = table + 10 *
 * index). `sector_table` is the caller-supplied native bitmask table
 * 0x1104CA; `code` is the native mode/event byte. `found == 0` reproduces the
 * native NULL return (`d < 0x38` while the actor height dword is nonzero). */
typedef enum fifa96_action_kick_event_table {
  FIFA96_ACTION_KICK_EVENT_TABLE_HEIGHT = 0,
  FIFA96_ACTION_KICK_EVENT_TABLE_CARRY = 1,
  FIFA96_ACTION_KICK_EVENT_TABLE_ACTIVE = 2,
  FIFA96_ACTION_KICK_EVENT_TABLE_IDLE = 3,
} fifa96_action_kick_event_table;

typedef struct fifa96_action_kick_event {
  uint8_t code;         /* native EDX mode/event byte */
  uint8_t subtype;      /* actor[+0x8B] >> 24 */
  uint8_t sector_byte;  /* actor[+0x8E] low byte, when x == z == 0 */
  uint8_t active;       /* actor[+0x8D] != 0 */
  uint8_t has_slot;     /* actor[+0x20] != 0 */
  uint8_t slot_counter; /* slot[+0x23] */
  uint8_t phase;        /* [0x157A4A] >> 24 */
  int16_t x;            /* native EBX word */
  int16_t z;            /* native ECX word */
  int16_t ball_height;  /* word [0x157750] */
  int32_t height;       /* dword actor[+0x5D] (zero test and low word band) */
} fifa96_action_kick_event;

typedef struct fifa96_action_kick_event_out {
  uint8_t found;
  uint8_t table;
  uint32_t index;
} fifa96_action_kick_event_out;

fifa96_err_t fifa96_action_kick_event_row(const fifa96_action_kick_event *event,
                                          const uint8_t *sector_table,
                                          fifa96_action_kick_event_out *out);

/* FU-139 §3.4: the FUN_0007B9C4 negative-mode range band (native
 * 0x7BBE4..0x7BC15): a non-negative mode is returned unchanged; a negative
 * one becomes 0x20 when x < 0x5A0, 0x30 when x < 0x780, else 0x10. */
fifa96_err_t fifa96_action_kick_range_band(int8_t mode, int16_t ball_x,
                                           uint8_t *band);

/* FU-139 §3.6: the FUN_0007AE70 tail append selector (native
 * 0x7B01A..0x7B09F, jump table flat 0x7AE38). `direct == 1` names the native
 * FUN_00092820 path, `direct == 0` the FUN_000928F0 ring path; `code` is the
 * native selector argument. The actor action byte takes precedence over the
 * row[0] fallback. */
typedef struct fifa96_action_kick_append {
  uint8_t direct;
  uint8_t code;
} fifa96_action_kick_append;

fifa96_err_t fifa96_action_kick_event_append(uint8_t actor_action, uint8_t row0,
                                             fifa96_action_kick_append *out);

/* FU-139 §3.7: row 07 stage-0 target (native 0x8154C..0x815B5). When the
 * record has a control slot and the slot word is 0x60 or 0x8000, x/z are the
 * camera words plus the caller-supplied sign-extended type-offset table
 * entries shifted left 4 (`resolved = 1`; the native arm leaves y untouched);
 * otherwise the whole camera triple is copied (`resolved = 0`). */
fifa96_err_t fifa96_action_kick_stage_target(uint8_t has_slot, uint16_t slot_word,
                                             uint8_t type8,
                                             const fifa96_action_vec3 *camera,
                                             const int8_t *offset_x,
                                             const int8_t *offset_z,
                                             fifa96_action_vec3 *out,
                                             uint8_t *resolved);

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
 * (`rec+0x8F >> 24`) is >= 2 the position triple is copied to the output
 * (including y); otherwise the caller-supplied pre-reset x (`lead_x`, the
 * native `rec+0x4D` the unported camera-reset call leaves) selects the ±0x30
 * kickoff x with z cleared — the native branch writes `+0x4D` and `+0x55`
 * only, so `out->y` is left untouched. */
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
