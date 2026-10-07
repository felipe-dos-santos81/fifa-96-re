/* include/fifa96_loader/fifa96_arm_helpers.h — M2 arms-and-wiring Task 3 /
 * FU-142b: the cluster-G record view and the shared `FUN_0008DCD4`
 * distance/staging helper.
 *
 * Native evidence (read-only /FIFA96.EXE, FU-142 Appendix C): the helper body
 * `0x8DCD4..0x8DDBB` is 61 instructions taking EAX = record+0x59 (position),
 * EDX = record+0x4D (target) and EBX = record+0x65 (out) and writing the
 * 6-byte vector `{word distance, word dx, word dz}` (FU-79 §2.9 records the
 * same layout). The record view carries the cluster-G native fields by offset;
 * the appendix adds fields as a body needs them and never repurposes one. */
#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

struct fifa96_rng;

typedef struct fifa96_arm_vec {
  int32_t x, y, z;
} fifa96_arm_vec;

/* One native 0xB2-stride record as the cluster-G bodies read it. */
struct fifa96_arm_record {
  fifa96_arm_vec pos;      /* +0x59/+0x5D/+0x61 */
  fifa96_arm_vec target;   /* +0x4D/+0x51/+0x55 */
  int32_t vel_x, vel_z;    /* +0x71/+0x73 */
  int32_t lane;            /* +0x69: the 0x8DCD4 dz word, sign-extended */
  uint16_t timer81;        /* +0x81 */
  uint16_t timer7b;        /* +0x7B (row 26 writes it from the 0x10F394 table) */
  uint8_t timer93;         /* +0x93 */
  int32_t timer89;         /* +0x89 */
  uint16_t delta;          /* [0x157A64] zero-extended frame-delta word */
  uint8_t stage;           /* +0x8F>>24 */
  uint8_t type;            /* +0x8E>>24 */
  uint8_t actor_type;      /* +0x8B>>24 */
  uint8_t active;          /* +0x8D (row 26: sign-magnitude offset byte) */
  uint8_t code;            /* +0x91 */
  uint8_t stage92;         /* +0x92 */
  uint8_t has_ball;        /* +0x9B */
  uint8_t side;            /* team +0x826 */
  uint8_t flag830;         /* team +0x830 */
  int32_t chosen831;       /* team +0x831 encoded entity id or NONE (-1) */
  int8_t player_d;         /* rec[+0x4][+0xD]: 0x10F394 byte-table index */
  int8_t player_e;         /* rec[+0x4][+0xE]: signed row-26 stage-0 multiplier */
  struct fifa96_rng *rng;  /* RNG for the bodies that draw (Tasks 4+) */
};

/* `FUN_0008DCD4`: from = the position triple, to = the target triple, both read
 * as their low 16-bit words; the native out vector is
 * `{word distance, word dx, word dz}`. The derived surface returns the 16-bit
 * distance (sign-extended) through `out_distance` and the dz word through
 * `out_lane` (sign-extended); dx has no Task-3 consumer and is recorded in
 * Appendix C.3. NULL `from`/`to`/`out_distance`/`out_lane` ->
 * -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_arm_dist_stage(const fifa96_arm_vec *from, const fifa96_arm_vec *to,
                                   int32_t *out_distance, int32_t *out_lane);
