/* include/fifa96_loader/fifa96_arm_helpers.h — M2 arms-and-wiring Task 3 /
 * FU-142b: the cluster-G record view and the shared `FUN_0008DCD4`
 * distance/staging helper.
 *
 * Native evidence (read-only /FIFA96.EXE, FU-142 Appendix C): the helper body
 * `0x8DCD4..0x8DD5B` is 61 instructions taking EAX = record+0x59 (position),
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
  uint8_t anim_cycle;      /* [0x158782] 0..7 animation-row cycle (row 27) */
  uint16_t anim_cursor;    /* [0x10F374] 0x1103CB pair cursor (row 27) */
  uint8_t anim_sel;        /* last id resolved by 0x6E598 (row 27 derived) */
  uint8_t flag44;          /* native +0x44 anim-row terminal flag (row 27) */
  uint8_t anim_overflow;   /* derived: pair walk left the 24-pair table */
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

/* `FUN_00079C50` at row 27's call site (`0x86913`): first-hand the helper is
 * 28 instructions, `0x79C50..0x79C98`, and the row 27 caller passes
 * `DX = word[rec+0x67]` (the position-target dx) and `BX = word[rec+0x69]`
 * (dz) — both from the `0x8DCD4` out triple — after the `0x86905..0x86910`
 * dword loads shifted by 16. Guard `DX|BX == 0` returns `byte[rec+0x8E]`
 * unmodified (`0x79C59..0x79C68`); otherwise the native stores
 * `FUN_000CD474(DX,BX)` at `rec+0x7D` and writes the facing octant
 * `((angle + 0x40) & 0x3FF) >> 7` to `rec+0x8E`, returning it
 * (`0x79C69..0x79C98`). The angle primitive is the ported
 * `fifa96_action_kick_angle` (FU-76 §5, `FUN_000CD474` + atan table
 * `0x14072C`).
 *
 * Derived surface: `dx`/`dz` are recomputed from `pos`/`target` with the same
 * 16-bit word differences `0x8DCD4` uses; `out_lane` receives the octant and
 * the zero-direction guard leaves `*out_lane` unchanged (the caller seeds it
 * with the record's current `+0x8E` byte). The native `+0x7D` angle word has
 * no row-27 consumer and is not exposed (Appendix D). NULL arguments ->
 * -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_arm_face(const fifa96_arm_vec *pos, const fifa96_arm_vec *target,
                             uint8_t *out_lane);

/* `FUN_0006E598` id-resolution subset (`0x6E598..0x6E713`) as used by row 27
 * at `0x8695C`/`0x869F8`. First-hand: a non-zero `kind` skips the current-row
 * continuation/reroll branch and clamps `(int16)kind < 0 || >= 0x6F` to 0
 * (`0x6E68E..0x6E69B`); `kind == 0` with a current row byte in `1..0x61` or
 * `0x66..0x6E` re-selects that current row (`0x6E622..0x6E655`), otherwise
 * the native draws RNG `0x92AC8` and maps `RNG & 3` to `{0, 0x62, 0x65}`
 * (`0x6E659..0x6E685`). The derived helper covers the deterministic
 * resolution/clamp: `kind != 0` -> `kind`; `kind == 0` and a valid non-zero
 * `row` -> `row`; `kind == 0` and `row == 0` or `row in {0x62..0x65}` -> 0
 * (the RNG reroll draws and the `[0x57A4A]>>24 == 2` phase gate are the
 * documented open leg; row 27's `0x1103CB` ids are never 0). NULL `out_slot`
 * -> -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_arm_anim_select(uint8_t kind, uint8_t row, uint8_t *out_slot);
