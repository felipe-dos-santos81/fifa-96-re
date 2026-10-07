/* include/fifa96_loader/fifa96_arm_helpers.h — M2 arms-and-wiring Tasks 3/4/5
 * / FU-142b: the cluster-G record view, the shared `FUN_0008DCD4`
 * distance/staging helper, the `0x79C50`/`0x6E598` row-27 helpers and the
 * `FUN_0007DAB4` reset subset used by rows 2C/29.
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
#include "fifa96_loader/fifa96_entity_update.h"

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
  uint8_t anim_overflow;   /* derived: pair walk left the table (step-cleared) */
  struct fifa96_rng *rng;  /* RNG for the bodies that draw (Tasks 4+) */
  /* Row 29 (`0x874E4`, FU-142c Appendix F) fields. */
  uint8_t phase;           /* [0x157A4A]>>24: the phase-5 machine gate */
  uint8_t skip_9a;         /* +0x9A: occupied/skip byte (install pre-check) */
  uint8_t team_index;      /* this record's index in `team_candidates` */
  uint8_t ball_skip;       /* byte [[0x157A9F]+0x8D]: nearest-search skip index */
  uint8_t chase;           /* derived: [0x10F36C] == this record (stage gates) */
  uint8_t side_controlled; /* [0x157AAC]>>24: the phase-cell pair selector */
  uint8_t install;         /* derived install request code, 0 = none (row 29) */
  int8_t cell[2][2];       /* [rec+8] descriptor: [0] = +0/+1, [1] = +2/+3 */
  fifa96_arm_vec ball_pos; /* [[0x157A9F]+0x59]: nearest-search target (x, z) */
  const fifa96_entity_candidate *team_candidates; /* [rec+0]: the 11 records */
};

/* `FUN_0007DAB4` (`0x7DAB4..0x7DB0C`, 35 instructions) derived reset subset, as
 * called by row 2C at `0x84624` and (later) row 29. First-hand:
 * `[rec+0x92] = 0xFF` (`0x7DABA`), `[rec+0x89] = 0` (`0x7DAC4`), then
 * `FUN_0007D9A4(rec, code 0, staged 0, no-invoke)` (`0x7DAFB..0x7DB03`).
 * The derived subset (FU-141 §3.4) sets `stage92 = 0xFF`, `timer89 = 0` and
 * the code-0 re-install (`code = 0`). The native `[rec+0x20]` slot callback
 * `FUN_00078B00` (`0x7DAD2`), the phase-2 `FUN_0007C990` forced-decision arm
 * (`0x7DAEF`, FU-141 OL-44) and the native installer's accepted-install tail
 * (which would overwrite `+0x92` with the staged byte) stay FU-142 OL-54.
 * NULL `rec` -> -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_arm_reset(struct fifa96_arm_record *rec);

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
 * at `0x8695C`/`0x869F8`. First-hand: a non-zero `kind` skips the
 * current-row/RNG block (`0x6E608..0x6E61C`) and clamps `(int16)kind < 0 ||
 * >= 0x6F` to 0 (`0x6E68E..0x6E69B`). For `kind == 0` the native reads the
 * current row `[rec+0x28]` (`0x6E622..0x6E627`): a NULL pointer takes the RNG
 * reroll (`0x6E659`); a row byte 0 of `0` or `0x62..0x65` keeps that byte
 * (`0x6E62F`/`0x6E638..0x6E64A` -> `0x6E653` EAX=0 -> `0x6E657 JZ 0x6E687`);
 * a non-zero non-special byte takes the RNG reroll `0x92AC8`
 * (`0x6E64C..0x6E651`), `RNG & 3` -> `{0, 0x62, 0x65}` (`0x6E659..0x6E685`).
 *
 * Derived surface: `kind != 0` -> clamp -> `kind`; `kind == 0` models only the
 * native special-byte keep rule with the caller-supplied `row` as a stand-in
 * for the row byte (`row == 0` or `row in {0x62..0x65}` -> `row`). The native
 * source (`byte[[rec+0x28]]`; the row-27 step passes its last resolved id
 * `anim_sel`, and the 0x10EF00 row-table byte is neither staged nor
 * established to equal it), the RNG reroll and the `[0x57A4A]>>24 == 2` phase
 * gate are unmodeled (OL-52); the derived reroll fallback is 0. Every
 * `0x1103CB` id is `3..0x6B`, so the branch is unreachable from row 27.
 *
 * `kind` is `uint8_t` and cannot represent the native signed 16-bit id: it is
 * exact for the `0x1103CB` ids (0..0x6B, low-byte-lossless) and for values
 * `>= 0x6F` (the native clamps every positive id >= 0x6F to 0), but a 16-bit
 * argument outside 0..0xFF (e.g. `0x0168`, or a negative word) truncates and
 * is not modelled. NULL `out_slot` -> -FIFA96_ERR_INVALID. */
fifa96_err_t fifa96_arm_anim_select(uint8_t kind, uint8_t row, uint8_t *out_slot);
