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
 * EBX/ECX inputs and the RNG reroll stay OL-52/OL-62). A live control slot
 * (`actor.has_slot`) runs the unported `0x78B00` callback (reported as
 * `slot_cb`, OL-62). The tail's per-code target algebra
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
