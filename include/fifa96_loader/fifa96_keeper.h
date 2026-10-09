#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_rng.h"

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

typedef struct fifa96_keeper_input {
  uint8_t has_slot;
  uint8_t phase;
  uint8_t human_phase;
  uint8_t event_flag;
  uint8_t team_side;
  uint8_t code;     /* byte +0x91 action code: the row-1F gates `[rec+0x8E]>>24`
                     * `== 0x1F` / `== 5` read the dword at +0x8E shifted 24 =
                     * byte +0x91 (FU-151 erratum 1; first-hand 0x761D1/
                     * 0x765A7; the former `type8` name indexed the +0x8E
                     * octant the gates never read) */
  uint8_t type_gate;
  uint8_t is_actor;
  int32_t lane;
  int32_t cam_x;
  int32_t cam_y;
  int32_t cam_z;
  int32_t pos_x;
  int32_t pos_z;
  int8_t slot_dir_x;
  int8_t slot_dir_z;
} fifa96_keeper_input;

typedef struct fifa96_keeper_decision {
  uint8_t install;
  uint8_t invoke;
  uint8_t copy_cam;
  uint8_t copy_pos;
  uint8_t clear_c5c;
  int32_t target_x;
  int32_t target_y;
  int32_t target_z;
} fifa96_keeper_decision;

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
                                    uint8_t code, uint8_t phase_latch,
                                    fifa96_keeper_arm_out *out);
fifa96_err_t fifa96_keeper_reposition_a_gate(uint8_t has_tracked_teammate,
                                             fifa96_keeper_rep_a_out *out);
fifa96_err_t fifa96_keeper_reposition_b_finish(uint8_t is_controlled, uint8_t human_side,
                                               uint8_t *install_code, uint8_t *notify_code);
fifa96_err_t fifa96_keeper_lunge_track(const fifa96_keeper_vec *delta, int16_t angle,
                                       fifa96_keeper_point *pos, uint8_t *on_target,
                                       int16_t *steer_x, int16_t *steer_z, uint8_t *event_code);
fifa96_err_t fifa96_keeper_hold_fallback(const fifa96_keeper_point *cam, uint8_t side,
                                         int32_t cam_vel_z, int32_t lane, int32_t dir,
                                         uint16_t dir_word, int32_t vel_x, int32_t lead_x,
                                         fifa96_keeper_point *out);
fifa96_err_t fifa96_keeper_arm_camera(uint8_t stage, uint8_t team_side, uint8_t phase,
                                      fifa96_keeper_point *target, uint8_t *camera_hook);
fifa96_err_t fifa96_keeper_input_decide(const fifa96_keeper_input *in,
                                        fifa96_keeper_decision *out);

/* ===== FU-151 §Port contract items 1/2 — the two full keeper machines =====
 *
 * `fifa96_keeper_claim_step` is the row-1E (`0x7550C`) ten-stage machine
 * (`0x7550C..0x7612F`, stage table `0x754E4`), first-hand /FIFA96.EXE this
 * slice. One call runs one dispatch; the native fall-through chains (stage 0
 * falls into 1, stage 1 may fall into 2/3, stage 2 into 3, stage 3 into 4,
 * 4 into 5, 5 into 6, 6 into 7, 7 into 8, 8 into 9, 9 into the common exit)
 * are preserved. Caller-owned state mirrors the record cells the body reads
 * and writes (`+0x89`, `+0x92`, `+0x9B`, `+0x20`, `+0x8E`, `+0x3D`,
 * `byte[[rec+0x28]]`, `+0x44`, `word[+0x71]`) plus the process cells
 * `0x15774C/50/54` (camera focus), `0x157A77` (reset triple), `0x157C30`
 * `{band,dx,dz}`, `0x157C36` (saved point), `0x157C42` (travelled gauge) and
 * `[0x157AB2]` latch. Effects the engine can apply are requests in
 * `fifa96_keeper_claim_out`; the unported sinks (0x744D4, 0x74CDC, 0x79B1C,
 * 0x7B878, 0x36200/0x361A4/0x4C320/0x4C380/0x4C31C, 0x7A490, 0x8DE8C+0x786A0)
 * are named request bits with FU-151 legs. `mates`/`mate_count`/`skip_index`
 * feed the stage-4 `FUN_0008DE8C` nearest (skip = `[rec+0x8A]>>24` = byte
 * `+0x8D`, the record's active ordinal); the `0x10F334/0x10F33C` per-sector
 * sprite offsets are caller inputs `offset_x`/`offset_z`. NULL state/out ->
 * -FIFA96_ERR_INVALID. */
typedef struct fifa96_keeper_claim {
  fifa96_keeper_point pos;        /* +0x59/5D/61 */
  int32_t cam_x, cam_y, cam_z;    /* 0x15774C/50/54 in/out */
  int32_t reset_x, reset_y, reset_z; /* 0x157A77 in/out */
  int32_t target_x, target_z;     /* +0x4D/+0x55 */
  int16_t vec_band, vec_dx, vec_dz; /* 0x157C30/32/34 in/out */
  int16_t saved_x, saved_z;       /* 0x157C36/3A in/out */
  int16_t gauge;                  /* 0x157C42 in/out (travelled distance) */
  int32_t timer89;                /* +0x89 */
  uint16_t delta;                 /* [0x157A64] */
  int16_t timer7b;                /* +0x7B out */
  uint8_t stage92;                /* +0x92 in/out */
  uint8_t side;                   /* team +0x826 */
  uint8_t has_ball;               /* +0x9B in/out */
  uint8_t has_slot;               /* +0x20 */
  uint8_t slot_edge;              /* byte[slot+6] (the release edge bits) */
  int8_t slot_dir_x, slot_dir_z;  /* (int8)(slot[+0x1D/+0x1E]>>24) */
  uint8_t sector;                 /* +0x8E in/out (facing octant) */
  uint8_t frame;                  /* +0x3D */
  uint8_t anim_row;               /* byte[[rec+0x28]] live row id */
  uint8_t row44;                  /* byte[+0x44] */
  uint8_t vel71_nonzero;          /* word[+0x71] != 0 */
  uint8_t latch_157ab2;           /* [0x157AB2] in/out */
  int8_t offset_x, offset_z;      /* 0x10F334/0x10F33C[sector] */
  const fifa96_entity_candidate *mates; /* own-team records (0x8DE8C) */
  uint32_t mate_count;
  uint8_t skip_index;             /* [rec+0x8A]>>24 = +0x8D active */
  int8_t range_attr;              /* rec[+4][+0xD] (0x74E2C range; unmodeled -> 0) */
  struct fifa96_rng *rng;         /* FUN_00092AC8 (dive roll / clear vector) */
} fifa96_keeper_claim;

typedef struct fifa96_keeper_claim_out {
  uint8_t claimed;        /* the head claim ran (+0x9B set, place staged) */
  uint8_t released;       /* stage 5 cleared +0x9B */
  uint8_t helper;         /* 0x7876C slot-merge request */
  uint8_t controlled;     /* [0x157A83] = rec */
  uint8_t place;          /* 0x700F4 camera place (place_*) */
  int32_t place_x, place_y, place_z;
  uint8_t reset;          /* 0x7DAB4 record reset */
  uint8_t situation_0b;   /* 0x8A938(0xB, side, 0) */
  uint8_t install;        /* 0x7D9A4 code request, 0 = none */
  uint8_t event;          /* last 0x6E598 event code, 0 = none */
  uint8_t ran;            /* +0x9E set (stage 1) */
  uint8_t slot_fill;      /* 0x744D4 (leg) */
  uint8_t guard;          /* 0x74CDC (leg) */
  uint8_t snap;           /* 0x79B1C (leg) */
  uint8_t handoff;        /* 0x8DE8C + 0x786A0 (leg) */
  uint8_t ball_stage;     /* 0x7A490 staging (leg) */
  uint8_t ball_event;     /* the 0x7A490/0x158743 event byte (leg) */
  uint8_t vector_build;   /* 0x7B878 slot vector (leg) */
  uint8_t clear_vec;      /* 0x74E2C clear vector (leg) */
  uint8_t scenario;       /* 0x92820/0x71C94 stage-5/7 sinks (leg) */
  uint16_t ui;            /* slot-edge UI/audio call mask (leg) */
  uint8_t flag_write;     /* the stage-6 [0x157820]/[0x157822] wrote */
  uint8_t flag_157820;
  uint8_t flag_157822;
  uint8_t held_exit;      /* exited 0x760DF while still holding the ball */
} fifa96_keeper_claim_out;

fifa96_err_t fifa96_keeper_claim_step(fifa96_keeper_claim *state,
                                      fifa96_keeper_claim_out *out);

/* `fifa96_keeper_closedown_step` is the row-1D (`0x74EB0`) five-stage
 * close-down machine (`0x74EB0..0x754E1`, stage table `0x74E9C`), first-hand
 * this slice for stages 0..4. Same request/leg discipline as the claim
 * machine; the 0x7A490 staging carries the FU-151 §2.5 events 0x30/0x31 (the
 * `band` selects `band>>3` high / `band>>4` low) and the `0x8F188(0x22)`
 * ring sink is a leg (`ring`). The stage-3 clearance vector is computed with
 * the `0x8DD70`/`0x114E04`/`0x795A4` fold the entity primitives port. */
typedef struct fifa96_keeper_closedown {
  fifa96_keeper_point pos;
  int32_t cam_x, cam_y, cam_z;    /* 0x15774C/50/54 in/out */
  int32_t reset_x, reset_y, reset_z; /* 0x157A77 in/out */
  int32_t target_x, target_z;     /* +0x4D/+0x55 */
  fifa96_keeper_vec vec;          /* 0x158738 {band,dx,dz} in/out */
  int32_t timer89;
  uint16_t delta;
  int32_t lane;                   /* (signed word) [+0x69]>>16 */
  uint8_t stage92;
  uint8_t side;
  uint8_t has_ball;               /* +0x9B */
  uint8_t has_slot;               /* +0x20 */
  uint8_t slot_edge;              /* byte[slot+6] (the release edge bits) */
  uint8_t slot_pressed;           /* byte[slot+4] (the 0x75137 latch-edge
                                   * `& 0x20` test; the +4 pressed word's low
                                   * byte, not the +6 released word) */
  uint8_t sector;
  uint8_t row44;
  uint8_t session_gate;           /* [0x14C32A] */
  uint8_t latch_157ab2;           /* [0x157AB2] in/out */
  int8_t offset_x, offset_z;      /* 0x10F334/0x10F33C[sector] */
  int16_t cam_off_z;              /* the per-side local camera z offset */
  const fifa96_entity_candidate *mates;
  uint32_t mate_count;
  uint8_t skip_index;
  int8_t range_attr;              /* rec[+4][+0xD] (0x74E2C range; unmodeled -> 0) */
  struct fifa96_rng *rng;         /* FUN_00092AC8 (0x74E2C clear vector) */
} fifa96_keeper_closedown;

typedef struct fifa96_keeper_closedown_out {
  uint8_t helper;         /* 0x7876C (stage < 3) */
  uint8_t controlled;     /* [0x157A83] = rec */
  uint8_t place;          /* 0x700F4 (place_*) */
  int32_t place_x, place_y, place_z;
  uint8_t commit;         /* 0x79B6C -> the derived place() commit */
  uint8_t reset;          /* 0x7DAB4 */
  uint8_t situation_0b;
  uint8_t event;          /* 0x6E598 event code, 0 = none */
  uint8_t ran;            /* +0x9E set */
  uint8_t snap;           /* 0x79B1C (leg) */
  uint8_t slot_fill;      /* 0x744D4 (leg) */
  uint8_t handoff;        /* leg */
  uint8_t ball_stage;     /* 0x7A490 (leg) */
  uint8_t clear_vec;      /* 0x74E2C clear vector (leg) */
  uint8_t scenario;       /* 0x92820/0x71C94/[0x158743] sinks (leg) */
  uint8_t ring;           /* 0x8F188(0x22) (leg) */
  uint8_t sink_4b0;       /* 0x8F188(0xA4,4,0) (leg) */
  uint8_t clearance_event; /* 0x30 high / 0x31 low trajectory */
  uint16_t ui;            /* UI/audio call mask (leg) */
  uint8_t tail_hold;      /* the 0x754BC tail ran with the latch set */
} fifa96_keeper_closedown_out;

fifa96_err_t fifa96_keeper_closedown_step(fifa96_keeper_closedown *state,
                                          fifa96_keeper_closedown_out *out);
