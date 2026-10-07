#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"
#include "fifa96_loader/fifa96_entity_update.h"

typedef struct fifa96_ball_pair_vector {
  int16_t x;
  int16_t height;
  int16_t z;
} fifa96_ball_pair_vector;

/* FU-139 §3.3: the FU-73 §3.3 camera-led reception target triple (native
 * 0x157770), 32-bit fields. */
typedef struct fifa96_ball_pair_vec3i {
  int32_t x;
  int32_t y;
  int32_t z;
} fifa96_ball_pair_vec3i;

/* FU-139 §3.1: the FU-73 §1 ball staging block (native 0x158730..0x158746):
 * actor pointer, pick result, the 6-byte event vector, the trajectory word
 * (0x15873E), the angle/state word (0x158740), the flags byte (0x158742), the
 * event code byte (0x158743) and the three tail bytes (0x158744/45/46; the
 * 0x158745 field is written only by clears in the read paths). */
typedef struct fifa96_ball_pair_state {
  int32_t actor;      /* 0x158730 */
  int32_t receiver;   /* 0x158734 */
  fifa96_ball_pair_vector vector; /* 0x158738 x, 0x15873A middle, 0x15873C z */
  int16_t traj;       /* 0x15873E */
  int16_t angle;      /* 0x158740 */
  uint8_t flags;      /* 0x158742 */
  uint8_t code;       /* 0x158743 */
  uint8_t sub_code;   /* 0x158744 */
  uint8_t reserved45; /* 0x158745 */
  uint8_t ack;        /* 0x158746 */
} fifa96_ball_pair_state;

typedef struct fifa96_ball_pair_actor {
  fifa96_ball_pair_vector position;
  int16_t velocity_x;
  int16_t velocity_z;
  uint8_t has_ball;
  uint8_t kind;
  uint8_t action;
  uint8_t flag;
} fifa96_ball_pair_actor;

typedef struct fifa96_ball_pair_delta {
  int16_t distance;
  int16_t dx;
  int16_t dz;
} fifa96_ball_pair_delta;

typedef struct fifa96_ball_pair_targets {
  int32_t team0_target;
  int32_t team0_second;
  int32_t team1_target;
  int32_t team1_second;
} fifa96_ball_pair_targets;

int fifa96_ball_pair_offset(const fifa96_ball_pair_vector *from,
                            const fifa96_ball_pair_vector *to,
                            fifa96_ball_pair_delta *out);
int fifa96_ball_pair_decide(const fifa96_ball_pair_actor *interceptor,
                            const fifa96_ball_pair_actor *opponent,
                            int16_t delta,
                            fifa96_ball_pair_vector *out_position);
int fifa96_ball_pair_receive(const fifa96_ball_pair_actor *actor,
                             const fifa96_entity_candidate *candidates,
                             uint32_t count, int16_t target_x, int16_t target_y,
                             int32_t *receiver_index);
int fifa96_ball_pair_assign(fifa96_ball_pair_targets *targets, int team,
                            int32_t receiver_index);
int fifa96_ball_pair_possess(fifa96_ball_pair_actor *actor);
int fifa96_ball_pair_release(fifa96_ball_pair_actor *actor);

/* FU-139 §3.1: FU-73 §1 block clear (native FUN_0007A028 0x7A028..0x7A081):
 * zeroes the pointers, vector, trajectory and angle words, then writes the
 * derived reset pair `flags = 0x20`, `code = 2` and clears the three tail
 * bytes. NULL -> -FIFA96_ERR_INVALID. */
int fifa96_ball_pair_clear(fifa96_ball_pair_state *state);

/* FU-139 §3.1: the FUN_0007A490 staging core (native 0x7A4D3..0x7A4EA):
 * records the actor, copies the 6-byte event vector, and writes the
 * trajectory word and event code byte. The flag bytes are untouched by the
 * core (the code-keyed sub-code/animation tail is an open leg, FU-139 OL-26).
 * NULL state/vector -> -FIFA96_ERR_INVALID. */
int fifa96_ball_pair_stage(fifa96_ball_pair_state *state, int32_t actor,
                           const fifa96_ball_pair_vector *vector, int16_t traj,
                           uint8_t code);

/* FU-139 §3.3: the FUN_0007A084 reception target (native 0x7A2FF..0x7A331):
 * copies the base triple and advances x/z by `(lead >> 16) << 5` (the
 * camera-velocity words 0x1577BE/0x1577C0); y is copied unchanged. NULL
 * arguments -> -FIFA96_ERR_INVALID. */
int fifa96_ball_pair_receive_target(const fifa96_ball_pair_vec3i *base,
                                    int32_t lead_x, int32_t lead_z,
                                    fifa96_ball_pair_vec3i *out);

/* FU-139 §8 (Task 10): the first-hand code-keyed staging tail of
 * `FUN_0007A490` (`0x7A8D1..0x7AA2F`). The native tail reads the staged block's
 * code byte (0x158743) and classifies it through the eligibility byte table
 * `0x1104BB[code]` (read this slice; codes 0..0xE):
 *   {0,1,1,1,0,0,1,0,0,0,0,0,0,0,1}.
 * Eligible codes (1/2/3/6/0xE) take the recompute arm: while the sign-extended
 * word `[+0x69]>>16` (`lane_gate`) is `< 0x60` the record position advances by
 * the two native dword addends shifted right 17 (`nudge_x` = dword[+0x6B],
 * `nudge_z` = dword[+0x6D], both sign-preserving SARs), then the camera
 * velocity words 0x1577BE/0x1577C0/0x1577C2 are zeroed (derived no-op, the
 * camera block is unported). Every other signed code calls the reception
 * updater `FUN_0007A084` (unported; reported as `receive`, OL-62).
 * The inactive actor arm (`+0x8D == 0`) latches sub-code `0x30` for code 2 and
 * `0x31` for codes 1/3/6, and runs the whole-block reset
 * (`fifa96_ball_pair_clear`) for codes 4/5/7. Codes 1/2/3/6 then run the
 * `0x79C50` face over the staged vector's dx/dz words (`vector.height` is the
 * native word 0x15873A, `vector.z` the native word 0x15873C — the block vector
 * is the `FUN_0008DCD4` out triple, FU-139 §8 erratum), and every path resolves
 * an animation id through `0x6E598` (`kind` = the block sub-code; the native
 * EBX input is the block `reserved45`, used only by the unmodeled
 * `FUN_0006E490` frame resolve at `0x6E701..0x6E706`, *not* the current-row
 * byte the derived helper's `row` stands for — `byte[[rec+0x28]]` — so this
 * call passes `row = 0`; EBX/ECX and the RNG reroll stay OL-52/OL-62). A live
 * control slot (`actor.has_slot`) runs the unported `0x78B00` callback
 * (reported as `slot_cb`, OL-62). The tail's per-code target algebra
 * (`0x7AA30..0x7AE2F`) is unported (OL-62): this function ends at the
 * `0x7AA2F` call. NULL state/actor/recompute_table/out ->
 * -FIFA96_ERR_INVALID. */
typedef struct fifa96_ball_stage_tail_actor {
  int32_t pos_x;       /* +0x59 (recompute nudge in/out) */
  int32_t pos_z;       /* +0x61 */
  int32_t nudge_x;     /* dword[+0x6B]: native addend, port shifts >>17 */
  int32_t nudge_z;     /* dword[+0x6D] */
  int16_t lane_gate;   /* sign-extended word[+0x6B] (native [ +0x69 ] >> 16) */
  uint8_t active;      /* +0x8D */
  uint8_t has_slot;    /* +0x20 != 0 (0x78B00 callback gate) */
  uint8_t type8;       /* +0x8B >> 24 (native ECX input) */
  uint8_t facing;      /* +0x8E low byte (face zero-guard seed, in/out) */
} fifa96_ball_stage_tail_actor;

typedef struct fifa96_ball_stage_tail_out {
  uint8_t receive;     /* native FUN_0007A084 call (unported) */
  uint8_t recompute;   /* the code took the eligibility/nudge arm */
  uint8_t nudge;       /* pos_x/pos_z advanced */
  uint8_t camera_zero; /* 0x1577BE/C0/C2 zero arm */
  uint8_t cleared;     /* inactive 4/5/7 whole-block reset */
  uint8_t slot_cb;     /* native 0x78B00 callback (unported) */
  uint8_t face;        /* resulting +0x8E low byte */
  uint8_t anim;        /* resolved 0x6E598 id */
} fifa96_ball_stage_tail_out;

int fifa96_ball_pair_stage_tail(fifa96_ball_pair_state *state,
                                fifa96_ball_stage_tail_actor *actor,
                                const uint8_t *recompute_table,
                                fifa96_ball_stage_tail_out *out);

/* ===== FU-139 §9 (M2 arms-and-wiring Task 11): the FUN_0007B9C4 kick path =====
 *
 * The native `FUN_0007B9C4` (`0x7B9C4..0x7BF16`, 337 instructions) stages the
 * kick into the FU-73 §1 ball block: entry `EAX = actor` (0 reuses
 * `[0x158730]`), `EDX = mode` byte stored to `0x158742`, `EBX = 6-byte input
 * vector or NULL`. This port reproduces the full bounded flow first-hand:
 *   - prologue (`0x7B9D4..0x7BA17`): actor latch, mode store, 6-byte vector
 *     copy (or zero), trajectory word zero;
 *   - slot latch (`0x7BA1E..0x7BA85`): `word[slot+6] == (int8)mode` equality
 *     and latch, the L1 flag (`+0x8D && phase==2 && word6&0x20 && slot[+0x23] <
 *     7`) and the L2 flag (`phase==2 && word6&0x10`);
 *   - normal arm (`0x7BA85..0x7BBE4`): the wing target (`0x7BB13..0x7BB46`,
 *     `FUN_0008DCD4(camera, ±0xF0/pos_z local)`) or the slot/type direction
 *     arm (`0x7BB4B..0x7BBE2`, type table `0x10F334/0x10F33C` or slot
 *     `+0x20/+0x21`) through `FUN_0007B878` (ported here as
 *     `kick_dir_vector`: the `0x1104CA`-free event resolver on the slot word
 *     and direction, the side range `word[0x14C1D4+side*2]` 0x10-bit + slot
 *     word 0x50-bit 1.5×, `slot23² * row[1]` clamped to `row[2]..row[4]`, the
 *     diagonal `*0xB5>>8` and the `FUN_0008DC68` distance);
 *   - negative-mode band (`0x7BBE4..0x7BC15`) via the tested
 *     `fifa96_action_kick_range_band`;
 *   - mode arms (`0x7BC1A..0x7BC80`): the inactive `B57C` arm, the active
 *     mode-0x40 `B194` arm (both ported: goal-line RNG target construction,
 *     `0x8DCD4`, `FUN_000CD474`, the 0x114E04 fold over the re-derived speed,
 *     the nearest-record `FUN_0008DE8C` decoy and the `0x14C2F6` adjust) and
 *     the active `B57C` arm (`mode & 0x20` or L1);
 *   - event row (`0x7BC80..0x7BCC8`) through the tested
 *     `fifa96_action_kick_event_row` and the caller's four 10-byte row tables;
 *   - row application (`0x7BCB6..0x7BE0B`): code/sub-code/traj-add staging,
 *     the mode-0x30 override, the `0x14C1D4` 1.5×, the lower-first clamp and
 *     the `0x114E04` angle fold (the native idiom is ported exactly: the
 *     257-entry sine table, `FUN_000CD474` through the tested
 *     `fifa96_action_kick_angle`, and `FUN_000795A4`'s `(*speed*value+0x8000)
 *     >>16`);
 *   - code-4 RNG/divisor (`0x7BE26..0x7BEC0`): the `row[0] == 4` arm
 *     (mode 0x40: `traj = 0x90 + (rng&7)*(0x10 - desc15)`; else the
 *     `(rng&0x7F)+3` divisor) and `traj = (int16)dz + (int16)x/divisor`;
 *   - the `0x460` cap and the final `fifa96_ball_pair_stage` +
 *     `fifa96_ball_pair_stage_tail` (native `FUN_0007A490`).
 * `out->receive` is the unported `FUN_0007A084` request and `out->slot_cb` the
 * unported `0x78B00` callback (both OL-62); everything else is ported. NULL
 * `state`/`actor`/`slot`/`ctx`/`rng`/`out` -> -FIFA96_ERR_INVALID. */
typedef struct fifa96_ball_kick_slot {
  uint8_t present;    /* [rec+0x20] != 0 */
  int16_t word6;      /* word[slot+6] (in/out: the mode latch) */
  uint8_t counter23;  /* byte[slot+0x23] */
  int8_t dir_x;       /* (int8)byte[slot+0x20] */
  int8_t dir_z;       /* (int8)byte[slot+0x21] */
  int8_t anim_1d;     /* (int8)(slot[+0x1D]>>24) */
} fifa96_ball_kick_slot;

typedef struct fifa96_ball_kick_actor {
  int32_t id;                  /* record identity; 0 reuses `state->actor` */
  int32_t pos_x, pos_y, pos_z; /* +0x59/+0x5D/+0x61 (stage-tail nudge in/out) */
  int32_t vel_x, vel_z;        /* +0x71/+0x73 dwords */
  int32_t nudge_x;             /* dword[+0x6B] (stage-tail SAR-17 addend) */
  int32_t nudge_z;             /* dword[+0x6D] */
  uint8_t active;              /* +0x8D */
  uint8_t type;                /* +0x8E>>24 */
  uint8_t actor_type;          /* +0x8B>>24 */
  uint8_t anim_9d;             /* +0x9D */
  uint8_t byte_99;             /* +0x99 */
  int8_t desc_e;               /* (int8)(rec[+4][+0xE]>>24) */
  int8_t desc_10;              /* (int8)rec[+4][+0x10] */
  int8_t desc_11;              /* (int8)rec[+4][+0x11] */
  int8_t desc_15;              /* (int8)rec[+4][+0x15] */
  uint8_t side;                /* team[+0x826] */
  uint8_t sub_phase1;          /* [0x157A49]>>24 == 1 */
  uint8_t facing;              /* +0x8E low byte (stage-tail face in/out) */
} fifa96_ball_kick_actor;

typedef struct fifa96_ball_kick_candidate {
  int32_t pos_x, pos_z;   /* found record +0x59/+0x61 */
  int16_t vel_x, vel_z;   /* found record words +0x71/+0x73 */
} fifa96_ball_kick_candidate;

typedef struct fifa96_ball_kick_ctx {
  int32_t camera_x, camera_y, camera_z; /* 0x15774C/50/54 */
  uint8_t phase;             /* [0x157A4A]>>24 */
  uint8_t mode_state;        /* [0x157A4D] (B194 slot arm) */
  int32_t goal_gate;         /* [0x14C2F6] == 1 arm */
  int32_t arm_gate;          /* [0x14C326] > 0 short-circuits FUN_0007B57C */
  uint16_t side_range;       /* word[0x14C1D4 + side*2] */
  int16_t ball_height;       /* word[0x157750] */
  /* The sign-extended per-type direction bytes `0x10F334[type8]` /
   * `0x10F33C[type8]` the slot/type arm reads (caller-supplied per the house
   * table convention; the native loads them at `0x7BBC2`/`0x7BBCA`). */
  const int8_t *type_dir_x;
  const int8_t *type_dir_z;
  /* FUN_0008DE8C candidate records for FUN_0007B57C (the actor's team block);
   * `candidate_skip` is the native `(int16)actor[+0x8D]` and `self_index` the
   * found-record identity comparison stand-in (the native pointer equality
   * `found == actor`). */
  const fifa96_entity_candidate *candidates;
  uint32_t candidate_count;
  uint32_t candidate_skip;
  int32_t self_index;
  /* Parallel full-record view of `candidates` (the found record's +0x59/
   * +0x61/+0x71/+0x73 fields the decoy arm reads); 1:1 by index. */
  const struct fifa96_ball_kick_candidate *team_records;
  const uint8_t *sector_table;    /* 0x1104CA resolver bitmask table */
  const uint8_t *recompute_table; /* 0x1104BB stage-tail eligibility table */
  const uint8_t *event_rows[4];   /* 0x1102FE/16E/196/24A, 10-byte rows */
} fifa96_ball_kick_ctx;

typedef struct fifa96_ball_kick_out {
  uint8_t staged;   /* 1 = the row was applied and staged (native return 1) */
  uint8_t band;     /* resulting 0x158742 byte (after the negative-mode band) */
  uint8_t receive;  /* stage-tail FUN_0007A084 request (OL-62) */
  uint8_t cleared;  /* stage-tail inactive 4/5/7 whole-block reset */
  uint8_t slot_cb;  /* stage-tail 0x78B00 callback request (OL-62) */
  uint8_t nudge;    /* stage-tail recompute nudge ran */
  uint8_t anim;     /* stage-tail resolved 0x6E598 id */
} fifa96_ball_kick_out;

struct fifa96_rng;

fifa96_err_t fifa96_ball_kick_target(fifa96_ball_pair_state *state,
                                     fifa96_ball_kick_actor *actor,
                                     fifa96_ball_kick_slot *slot,
                                     const fifa96_ball_pair_vector *input,
                                     const fifa96_ball_kick_ctx *ctx,
                                     struct fifa96_rng *rng,
                                     uint8_t mode,
                                     fifa96_ball_kick_out *out);
