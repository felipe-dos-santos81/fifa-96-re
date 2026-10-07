#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"

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

/* FU-139 §8 (Task 10): the row-05 carrier machine `0x7F194..0x7F665`
 * (first-hand this slice). The bounded record-visible parts:
 *   - `0x7F19F`: `ran_set` — the unconditional `byte[+0x9E] = 1` latch (the
 *     engine `ran` field) runs before the phase check on every call;
 *   - `0x7F1A6`: phase != 2 -> `reset` (native FUN_0007DAB4) and return;
 *   - `0x7F1BF..0x7F205`: `claim` the possession block for `actor`;
 *   - `0x7F20B..0x7F217`: `team_target` (native team+0x7B2 = rec, +0x7B6 = 0);
 *   - `0x7F221..0x7F23A`: the capped/additive `timer89` (FU-78 helper);
 *   - `0x7F240`: word+0x81 != 0 returns;
 *   - `0x7F24E..0x7F258`: `target_camera` (the camera triple copy to
 *     +0x4D/+0x51/+0x55);
 *   - stage dispatch on +0x92 (`0x7F259..0x7F26C`):
 *     stage 0 (`0x7F274..0x7F57B`): `lane > 0x40` -> `clear_control` and
 *     return; else `set_control`; `close_word > bound_word`,
 *     `release_timer > 0` or an airborne record wait; the type arm
 *     (`ball_height > 0x38`) takes the caller's `type_dir_*[type8]` with speed
 *     0x60 and the slot arm the caller's `slot_dir_*` with speed 0x30; both
 *     then run the `0x7F386` dir-byte writes and the unported
 *     `0x7F3A1..0x7F57B` target algebra (`tail`, OL-63). The team-gate path
 *     raises the `slot_merge` request (native FUN_0007876C at `0x7F356`): its
 *     continuation is the merge result (a slot -> the slot arm/tail, no slot ->
 *     the fallback path), so `tail` is not claimed here. The remaining path
 *     raises `fallback` (native `FUN_0007F7E0`, unported, OL-63) and only
 *     continues into the dir-byte writes/tail when `[rec+0x8E]>>24 == 5`
 *     (`0x7F374..0x7F380`), so `tail` mirrors that gate;
 *     stage 1 (`0x7F57C..0x7F5E8`): `sink` (native FUN_00092820(rec,0x26),
 *     OL-27), `camera_zero` (0x1577BE/C0/C2), the 0x6E598 animation id
 *     (active -> 6, inactive -> 0x30), timer 0 and the latch advance;
 *     stage 2 (`0x7F5E9..0x7F626`): no slot waits; else `snap` (native
 *     `FUN_00079B1C`: target = pos, lane/velocity words zeroed) and the
 *     0x79C50 face from `slot_dir_*`; a zero slot word waits, else the latch
 *     loops to 0;
 *     stage 3 (`0x7F627..0x7F65C`): `+0x44` gates; latch 0 and the
 *     `handoff` request (native install code 4 on the staged ball actor
 *     `[0x158730]` + FUN_00079B58 receiver timer on `[0x158734]`) only for the
 *     team target; stages > 3 return unchanged.
 * All other native side effects (the record target writes of the stage-0 tail,
 * the 0x92820/0x71C94/0x79CCC/0x6DA64/0x7F7E0 call bodies, the camera/record
 * globals) are unported requests or no-ops with the numbered legs above; the
 * row is therefore NOT wired (binding gate, plan Global Constraints). NULL
 * `state`/`carrier`/`out` -> -FIFA96_ERR_INVALID; NULL `type_dir_*` is invalid
 * only when the type arm is taken. */
typedef struct fifa96_action_carrier {
  int32_t actor;          /* native EBP: the record pointer identity */
  int32_t timer89;        /* +0x89 (in/out: capped/additive, stage-1 clear) */
  int32_t lane;           /* sign-extended word[+0x6B] (native [+0x69]>>16) */
  int32_t ball_height;    /* dword [0x157750] (dribble type-arm gate) */
  int16_t timer81;        /* word +0x81 */
  int16_t close_word;     /* word +0x6B */
  int16_t bound_word;     /* word +0x77 */
  uint16_t delta;         /* word [0x157A64] */
  uint8_t phase;          /* [0x157A4A] >> 24 */
  uint8_t stage92;        /* +0x92 (in/out: the stage latch) */
  uint8_t active;         /* +0x8D */
  uint8_t airborne;       /* dword +0x5D != 0 */
  uint8_t has_slot;       /* +0x20 != 0 */
  uint8_t slot_live;      /* stage 2: word[slot+6] != 0 */
  uint8_t type8;          /* +0x8B >> 24 */
  uint8_t event_flag44;   /* +0x44 */
  uint8_t is_team_target; /* rec == [[rec]+0x7B2] */
  uint8_t facing;         /* +0x8E low byte (in/out: the stage-2 face) */
  uint8_t team_slot_pool; /* byte [team+0x828] */
  uint8_t team_chosen;    /* [team+0x7BF] != 0 */
  uint8_t team_search_gate; /* byte [team+0x829] */
  int8_t slot_dir_x;      /* slot[+0x1D]>>24 (stage-2 face dx) */
  int8_t slot_dir_z;      /* slot[+0x1E]>>24 (stage-2 face dz) */
} fifa96_action_carrier;

typedef struct fifa96_action_carrier_out {
  uint8_t ran_set;        /* native +0x9E = 1 latch (0x7F19F; engine `ran`) */
  uint8_t reset;          /* native FUN_0007DAB4 */
  uint8_t claim;          /* possession block claim (FU-78) */
  uint8_t team_target;    /* native team+0x7B2 = rec, +0x7B6 = 0 */
  uint8_t set_control;    /* native [0x157A83] = rec */
  uint8_t clear_control;  /* native [0x157A83] = 0 */
  uint8_t target_camera;  /* native rec+0x4D/+0x51/+0x55 = camera triple */
  uint8_t dirs;           /* native 0x5872A/B dir bytes live */
  uint8_t dir_x;          /* native [0x5872A] */
  uint8_t dir_z;          /* native [0x5872B] */
  uint8_t fallback;       /* native FUN_0007F7E0 (unported, OL-63) */
  uint8_t tail;           /* native stage-0 tail 0x7F3A1..0x7F57B (unported, OL-63) */
  uint8_t slot_merge;     /* native FUN_0007876C request (0x7F356) */
  uint8_t sink;           /* native FUN_00092820 request (OL-27) */
  uint8_t camera_zero;    /* native 0x1577BE/C0/C2 zero (stage 1) */
  uint8_t stage;          /* resulting native +0x92 */
  uint8_t snap;           /* native FUN_00079B1C snap (target = pos) */
  uint8_t face;           /* stage-2 result (native +0x8E low byte); 0 on the
                           * stages/returns that never run the face arm */
  uint8_t anim;           /* resolved 0x6E598 id (stage 1) */
  uint8_t handoff;        /* native stage-3 ball-actor install 4 + receiver timer */
} fifa96_action_carrier_out;

fifa96_err_t fifa96_action_carrier_arm(fifa96_action_possession *state,
                                       fifa96_action_carrier *carrier,
                                       const int8_t *type_dir_x,
                                       const int8_t *type_dir_z,
                                       fifa96_action_carrier_out *out);

typedef struct fifa96_action_receive {
  int32_t timer89;
  int16_t offset_word;   /* native word[+0x6B] lane */
  uint8_t stage;
  uint8_t active;
  uint8_t event_flag;    /* native +0x44 (staged zero; OL-68) */
  uint8_t is_team_target;
  /* FU-142 OL-32 / M2 Task 12: the row-21 head and resolution arm. */
  uint8_t phase;         /* native [0x157A4A]>>24 */
  uint8_t tracked;       /* native rec == [0x157A83] (the controlled actor) */
  uint8_t type8;         /* native +0x8B>>24 (0x6E598 selector argument) */
  int32_t pos_x;         /* native +0x59 (0x8DE8C target) */
  int32_t pos_z;         /* native +0x61 */
} fifa96_action_receive;

typedef struct fifa96_action_receive_out {
  uint8_t reset;
  uint8_t advance;
  uint8_t handoff;
  uint8_t stage;
  uint8_t ran;           /* native byte[+0x9E] = 1 (stage 0 only) */
  int32_t nearest;       /* native 0x8DE8C result index or -1 */
  uint8_t anim;          /* native 0x6E598 id 0x4A on the resolution arm */
} fifa96_action_receive_out;

/* FU-142 OL-32 / M2 Task 12: the full row-21 machine `0x85214..0x8539B`.
 * The phase/tracked head resets (`reset = 1`); the stage-0 lane <= 0x40 arm
 * runs the `0x8DE8C` nearest over the caller's team candidates (the native's
 * `[team+0x7A6]` record-11 alias holds the team base), the (dead) angle pair
 * and the `0x6E598` id `0x4A`, then zeroes the timer and advances the latch.
 * `team_candidates` is required only when that arm runs. */
fifa96_err_t fifa96_action_receive_step(fifa96_action_receive *state,
                                        const fifa96_entity_candidate *team_candidates,
                                        uint32_t candidate_count,
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
  /* FU-142 OL-32 / M2 Task 12: the stage-1 target arm inputs (native
   * 0x82FF3..0x8305A). `cam_f0` is the word at 0x1577F0 (`dword[0x1577EE] >>
   * 16`); `cam_f8`/`cam_fe` are word 0x1577FA / word 0x157800. The two vector
   * triples are the native 0x157794 and 0x157788 dword triples (the FU-141
   * frame shadows 0x157788/0x157794 with the camera). */
  int16_t cam_f0;
  int32_t vec794_x, vec794_y, vec794_z;
  int32_t vec788_x, vec788_y, vec788_z;
} fifa96_action_tackle;

typedef struct fifa96_action_tackle_out {
  uint8_t reset;
  uint8_t install_0e;
  uint8_t install_0f;
  uint8_t stage;
  uint8_t ran;            /* native byte[+0x9E] = 1 (stage 0) */
  uint8_t target_set;     /* the stage-1 target triple was written */
  int32_t target_x;       /* the written +0x4D dword */
  int32_t target_y;       /* +0x51 */
  int32_t target_z;       /* +0x55 (already side-nudged) */
  uint8_t receiver_timer; /* native FUN_00079B58(rec) (gated on +0x99 == 0) */
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
  /* FU-142 OL-32 / M2 Task 12: the resolution path (native 0x84A94..0x84AD5)
   * latches +0x9A, runs the 0x4C324 bind and the 0x7DAB4 reset subset; */
  uint8_t occupied;    /* native byte[+0x9A] = 1 */
  uint8_t bind;        /* native FUN_0004C324(0x15774C) request */
  uint8_t target_set;  /* native stage-1 target write (0x900, 0, 0) */
  int32_t target_x;    /* the written +0x4D word */
  int32_t target_z;    /* the written +0x55 word */
} fifa96_action_duel_out;

fifa96_err_t fifa96_action_duel_step(fifa96_action_duel *state, uint16_t delta, uint8_t input_byte,
                                     fifa96_action_duel_out *out);
fifa96_err_t fifa96_action_duel_split(int16_t own_metric, int16_t opp_metric,
                                      uint8_t opp_is_duel_type, uint8_t *own_code,
                                      uint8_t *opp_code);

/* FU-142 OL-32 / M2 Task 12: the row-18 resolution helpers, first-hand.
 *
 * `FUN_0008DB6C` (NSEARCH, `0x8DB6C..0x8DC48`, EAX = origin block, EDX = team
 * base, BX = skip index, ECX = fallback flag): walks 11 records at 0xB2 stride
 * skipping `[+0x20] != 0`, `i == (int16)skip_index`, `rec == [team+0x7BF]`,
 * `i == 0 && team[+0x829] == 0`, `[+0x9A] != 0` and `[+0x98] != 0`; the kept
 * value is `-0x8DC68(origin - rec)`, and the native shell-sorts (gap n/2..1,
 * no swap on equal) the values with the record indices mirrored, returning the
 * record of the first index slot. With no candidate and `fallback` set, it
 * returns the first record with `[+0x20] == 0` and `[+0x9A] == 0` (the other
 * gates ignored); else NONE (-1). */
typedef struct fifa96_action_duel_candidate {
  int16_t pos_x;      /* native +0x59 word */
  int16_t pos_z;      /* native +0x61 word */
  uint8_t has_slot;   /* native +0x20 != 0 */
  uint8_t skip_98;    /* native +0x98 != 0 */
  uint8_t skip_9a;    /* native +0x9A != 0 */
  uint8_t is_chosen;  /* derived: record == team+0x7BF (the pointer identity) */
} fifa96_action_duel_candidate;

typedef struct fifa96_action_duel_search_in {
  int16_t x;            /* native origin word +0 (0x158897) */
  int16_t z;            /* native origin word +8 (0x158897) */
  int16_t skip_index;   /* native BX, sign-extended */
  uint8_t record0_gate; /* native byte[team+0x829] != 0 */
  uint8_t fallback;     /* native ECX != 0 */
} fifa96_action_duel_search_in;

fifa96_err_t fifa96_action_duel_search(fifa96_action_duel_search_in const *in,
                                       const fifa96_action_duel_candidate *records,
                                       uint32_t count, int32_t *index);

/* `FUN_000786A0` (SWAP, `0x786A0..0x786EA`): when `from` holds the control
 * slot and `to` does not, the slot pointer moves (`to[+0x20] =
 * from[+0x20]`, `[[from+0x20]] = to`, `from[+0x20] = 0`) and the new slot's
 * words +4/+6/+8/+0xA/+0xC/+0x14/+0x16 are zeroed. The derived surface moves
 * the record-visible `has_slot` flag only; the slot block's field clears are
 * the numbered leg OL-68 (the derived engine does not model the slot block). */
typedef struct fifa96_action_duel_slot {
  uint8_t has_slot;   /* native record +0x20 != 0 */
} fifa96_action_duel_slot;

fifa96_err_t fifa96_action_duel_swap(fifa96_action_duel_slot *from,
                                     fifa96_action_duel_slot *to);

/* `FUN_0004C324` (`0x4C324..0x4C372`, the row-18 resolution bind): runs the
 * `FUN_00053DC4` recorder-arm latch, then writes the native [0x1074A4]:
 * `zero_extend(side_826)` from the double-indirect `[0x1587D4]` block plus the
 * `FUN_00036200(0)` stub when `byte[0x157AC2] >= 4` and the pointer is live,
 * else `sign_extend(byte[0x1587E6])` (`dword[0x1587E3] >> 24`). The recorder
 * block and [0x1074A4] are unmodeled (OL-68); `bound` is the derived value and
 * `stub_36200` the documented no-op request. */
typedef struct fifa96_action_duel_bind_in {
  uint8_t mode_157ac2; /* native byte [0x157AC2] */
  uint8_t team_present;/* native dword [0x1587D4] != 0 */
  uint8_t side_826;    /* native byte [[[0x1587D4]] + 0x826] */
  uint8_t fallback_e6; /* native (int8)(dword [0x1587E3] >> 24) */
} fifa96_action_duel_bind_in;

typedef struct fifa96_action_duel_bind_out {
  int32_t bound;       /* native [0x1074A4] value */
  uint8_t stub_36200;  /* native FUN_00036200(0) ran */
} fifa96_action_duel_bind_out;

fifa96_err_t fifa96_action_duel_bind(fifa96_action_duel_bind_in const *in,
                                     fifa96_action_duel_bind_out *out);

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

/* ===== FU-139 §9 (M2 arms-and-wiring Task 11): rows 07/0F kick machines =====
 *
 * `row 0x07` (`0x814B0..0x81737`, FU-76 §3.2 first-hand re-verified):
 *   - phase != 2 or `word[+0x81] != 0` -> the `FUN_0007DAB4` tail (`0x814B8`);
 *   - `timer89 += (uint16)delta` (`0x814D9`, word zero-extended);
 *   - stage 0 (`0x81512..0x815CD`): `[+0x9E]=1`; the gate
 *     (`lane > 0x40` or `(int16)(word[+0x5D]+0x70) < word[0x157750]`) with
 *     `timer89 > 0x3C` resets, else waits; the target through the tested
 *     `fifa96_action_kick_stage_target`, then `timer89 = 0`, `+0x92++`;
 *   - stage 1 (`0x815CD..0x816FC`): the mode word SI (`0x40` for the
 *     `[team+0x7CB]` record, else `word[slot+6]`, else `0x40` when the staged
 *     byte `0x158743 == 3`, else -1); SI == 0x40 runs the `FUN_0007E600`
 *     decision (ported inline; a non-zero result requests install `0x0E` and
 *     returns); the `[0x14C32A]`/`[0x15B680]==4` downgrade to `0x20`; the kick
 *     request (`fifa96_ball_kick_target` with `mode = SI`); the post-kick
 *     opponent `0x22` invoke (staged z `< 0x30`, `timer89 < 5`, the opponent
 *     `+0x8E==6`, no slot, lane `< 0xD0`, `|angle - word[+0x7D]| < 0x55`);
 *     `timer89 = 0`, `+0x92++`;
 *   - stage 2 (`0x816FC`): `byte[+0x44] != 0` -> the tail;
 *   - the `FUN_0007DAB4` tail (`0x81702`): the reset (with the `0x7C990`
 *     forced-decision install when phase 2 and active) and, for the team
 *     target, ball-actor install 4 + the `FUN_00079B58` receiver timer.
 * `row 0x0F` (`0x82AD0..0x82DCF`):
 *   - phase != 2 -> reset; `timer89 += (uint16)delta`;
 *   - stage 0 (`0x82B21..0x82BC3`): inactive or `word[+0x85] != 0` resets;
 *     the `0x8DCD4` distance to the `0x157788` target with the `< 0x50`
 *     half-vector position nudge, the `0x79B6C` re-anchor/face (anim `0x26`
 *     when inactive), `[+0x9E]=1`, `timer89 = 0`, `+0x92++`;
 *   - stage 1 (`0x82BC9..0x82DA2`): the `0x79B1C` snap, `byte[+0x44]` reset,
 *     `word[+0x81]` wait, the `FUN_0007876C` slot-merge request
 *     (`team+0x828`, `lane < 0xF0`, `[0x1586D7]==0`), the `FUN_00078A84` slot
 *     backup, the lane/bound and opponent-height gates, the lane `0x30` and
 *     `pos_y+0x80` gates, the `FUN_00078AA4` slot restore, kick 1
 *     (`mode = (int16)word[slot+6]`, the native dword[slot+4]>>16), the
 *     predictor distance vs lane and the
 *     corner kick (`0x6DBCC` code 2/3, camera-led staging and the
 *     `0x40`/`0x20` mode with the slot temporarily nulled), then the
 *     `word[+0x81] = 2*word[+0x85] - word[+0x87] + 0x1E` reload.
 * Declared requests: the `0x78B00`/`0x78A84`/`0x78AA4` slot-block calls, the
 * `0x7C990` install and the `0x79B58` receiver timer are caller requests
 * (OL-65); `0x71B9C`/`0x70B94` predictor sources and the `0x6DBCC` table are
 * caller inputs (OL-66); the native `FUN_0007B9C4` calls themselves are
 * returned as `kick`/`corner_kick` requests the engine runs through
 * `fifa96_ball_kick_target` (no library cycle). NULL `state`/`out` ->
 * -FIFA96_ERR_INVALID. */

struct fifa96_rng;

typedef struct fifa96_action_kick {
  uint8_t row;               /* 0x07 or 0x0F */
  uint8_t phase;             /* [0x157A4A]>>24 */
  uint8_t stage92;           /* +0x92 (in/out) */
  uint8_t active;            /* +0x8D */
  uint8_t type8;             /* +0x8B>>24 */
  uint8_t type;              /* +0x8E>>24 */
  uint8_t has_slot;          /* +0x20 != 0 */
  uint8_t byte44;            /* +0x44 */
  uint8_t byte99;            /* +0x99 */
  uint8_t side;              /* team[+0x826] */
  uint8_t is_team_target;    /* rec == [team+0x7B2] */
  uint8_t is_team_second;    /* rec == [team+0x7B6] (0x7C990) */
  uint8_t is_team_cb;        /* rec == [team+0x7CB] (row 07 stage 1) */
  uint8_t team_slot_pool;    /* team[+0x828] (row 0F merge gate) */
  uint8_t merge_gate_1586d7; /* byte[0x1586D7] != 0 blocks the merge */
  uint8_t team_mode_82b;     /* team[+0x82B] (row 0F corner mode) */
  uint8_t downgrade_gate;    /* byte[0x14C32A] (row 07 mode downgrade) */
  int32_t downgrade_word;    /* dword[0x15B680] == 4 (row 07 downgrade) */
  uint8_t decision_excluded; /* rec == [0x1577CA] (0x7E600 gate) */
  uint8_t has_desc_e;        /* (int8)rec[+4][+0xE] (0x7E600 divisor) */
  uint8_t opp_target_present;/* [[team+0x7A6]+0x7B2] != 0 (0x7C990) */
  uint8_t opp_target_carrier;/* that record's +0x9F bit 0 (0x7C990) */
  uint8_t team_target_present;/* [team+0x7B2] != 0 (0x7C990) */
  uint8_t team_target_carrier;/* [team+0x7B2]+0x9F bit 0 (0x7C990) */
  uint8_t post_kick;         /* in: the engine ran `out.kick` */
  int16_t kick_traj;         /* in: word[0x15873E] after the kick (0x81654) */
  uint8_t kick_done;         /* in: 1 = kick 1 ran, 2 = corner kick ran */
  uint8_t kick_staged;       /* in: the kick request staged a row */
  uint8_t staged_code;       /* in: byte 0x158743 (row 07 SI / corner code) */
  uint8_t opp_present;       /* in: [[team+0x7A6]+0x7B2] != 0 (row 07) */
  uint8_t facing;            /* +0x8E octant (in/out; 0x79C50 result) */
  uint16_t delta;            /* [0x157A64] */
  uint16_t timer81;          /* +0x81 (in/out) */
  uint16_t word85, word87;   /* +0x85/+0x87 (row 0F reload) */
  int32_t timer89;           /* +0x89 (in/out) */
  int32_t pos_x, pos_z;      /* +0x59/+0x61 */
  int32_t pos_y;             /* +0x5D dword (row 0F snap copies it) */
  int16_t pos_y_word;        /* (int16)word[+0x5D] */
  int16_t ball_height;       /* word[0x157750] */
  int16_t lane_word;         /* (int16)word[+0x6B] */
  int16_t bound_word;        /* (int16)word[+0x77] */
  int16_t face_word7d;       /* (int16)word[+0x7D] (0x7E600/opponent) */
  int16_t slot_word6;        /* word[slot+6] */
  int16_t kick_vec_x;        /* row 0F corner staging vector (0x158738) */
  int16_t kick_vec_height;   /* 0x15873A */
  int16_t kick_vec_z;        /* 0x15873C */
  struct fifa96_rng *rng;    /* row 0F corner face draw (0x92AC8) */
  /* row 07 opponent view ([[team+0x7A6]+0x7B2]) */
  int16_t opp_lane_word;     /* opponent (int16)word[+0x6B] (the <0xD0 gate) */
  int16_t opp_face_word;     /* opponent (int16)word[+0x7D] */
  uint8_t opp_type;          /* opponent +0x8E>>24 */
  uint8_t opp_has_slot;      /* opponent +0x20 != 0 */
  int16_t opp_angle_x;       /* opponent (int16)dword[+0x6D] = word[+0x6D] */
  int16_t opp_angle_z;       /* opponent (int16)dword[+0x6F] = word[+0x6F] */
  /* row 0F opponent height gate ([[team+0x7A6]+0x7C7]) */
  uint8_t opp2_present;
  int16_t opp2_pos_y_word;   /* (int16)word[+0x5D] */
  int16_t opp2_lane_word;    /* (int16)word[+0x6B] */
  /* caller-supplied block inputs */
  int32_t camera_x, camera_y, camera_z;         /* 0x15774C/50/54 */
  int32_t stage_target_x, stage_target_y, stage_target_z; /* 0x157788 (0F) */
  int32_t predictor_x, predictor_y, predictor_z; /* 0x71B9C(4) (0x7E600) */
  const int8_t *type_off_x;   /* 0x10F334[type8] */
  const int8_t *type_off_z;   /* 0x10F33C[type8] */
  int16_t corner_x, corner_z; /* 0x6DBCC cell output (row 0F) */
  uint8_t corner_code;        /* 0x6DBCC return 2/3 (row 0F) */
  /* stage-0 outputs */
  int32_t target_x, target_y, target_z;         /* +0x4D/+0x51/+0x55 */
  uint8_t target_resolved;
} fifa96_action_kick;

typedef struct fifa96_action_kick_out {
  uint8_t ran;              /* +0x9E latch (both rows' stage 0) */
  uint8_t reset;            /* FUN_0007DAB4 ran (stage92 = 0xFF, timer89 = 0) */
  uint8_t reset_install;    /* 0x7C990/0x7D9A4 install issued */
  uint8_t reset_code;       /* code that install carries (0 = action 0) */
  uint8_t slot_callback;    /* 0x78B00 slot callback request (reset path) */
  uint8_t defender_install; /* 0x7E600 decision -> install 0x0E */
  uint8_t kick;             /* run fifa96_ball_kick_target(mode = kick_mode) */
  uint8_t kick_mode;        /* SI mode word low byte */
  uint8_t opponent_invoke;  /* post-kick install 0x22 on the opponent */
  uint8_t ball_install;     /* tail install 4 on [0x158730] */
  uint8_t receiver_timer;   /* tail FUN_00079B58([0x158734]) */
  uint8_t slot_merge;       /* row 0F FUN_0007876C request */
  uint8_t slot_backup;      /* row 0F FUN_00078A84 request */
  uint8_t slot_restore;     /* row 0F FUN_00078AA4 request */
  uint8_t snap;             /* row 0F FUN_00079B1C applied (target = pos) */
  uint8_t camera_face;      /* row 0F FUN_00079B6C applied */
  uint8_t anim;             /* row 0F resolved animation id */
  uint8_t corner_kick;      /* row 0F corner kick request (corner_code) */
  uint8_t corner_face;      /* the corner 0x79C50 face ran (0x8E/+0x7D) */
  uint8_t corner_kick_mode; /* 0x40 code-3 / 0x20 rng mode */
  uint8_t stage;            /* resulting +0x92 */
  int16_t timer81_reload;   /* row 0F word[+0x81] reload value */
  uint8_t timer81_set;      /* the reload ran (BX != 0 path) */
} fifa96_action_kick_out;

fifa96_err_t fifa96_action_kick_machine(fifa96_action_kick *state,
                                        fifa96_action_kick_out *out);
