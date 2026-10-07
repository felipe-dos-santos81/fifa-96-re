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
