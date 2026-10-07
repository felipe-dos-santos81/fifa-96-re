/* tests/test_arm_helpers.c — M2 arms-and-wiring Task 3 / FU-142b: the
 * `FUN_0008DCD4` distance/staging helper port.
 *
 * Evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C (Ghidra
 * read-only /FIFA96.EXE: get_function_by_address/decompile_function/
 * disassemble_function 0x8DCD4, 61 instructions, body 0x8DCD4..0x8DD5B). The
 * native writes a 6-byte out vector `{word distance, word dx, word dz}` at
 * rec+0x65: the derived surface keeps `distance` and `lane` (= the dz word) and
 * drops dx, which no Task-3 consumer reads (Appendix C.3). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "fifa96_loader/fifa96_arm_helpers.h"
#include "fifa96_loader/fifa96_err.h"

#define ARM_INVALID ((fifa96_err_t)-FIFA96_ERR_INVALID)

/* Axis case: dx = 0x100, dz = 0 -> distance = max + f(min) = 0 + 0x100. The
 * y components are not read (Appendix C.3). */
static void test_dist_stage_axis(void) {
  const fifa96_arm_vec from = { 0, 0x1111, 0 };
  const fifa96_arm_vec to = { 0x100, 0x2222, 0 };
  int32_t distance = -1;
  int32_t lane = -1;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 0x100);
  assert(lane == 0);
}

/* dz-only case, negative: the lane output keeps the sign, the distance is the
 * magnitude. */
static void test_dist_stage_lane_sign(void) {
  const fifa96_arm_vec from = { 0, 0, 0x100 };
  const fifa96_arm_vec to = { 0, 0, 0 };
  int32_t distance = -1;
  int32_t lane = -1;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 0x100);
  assert(lane == -0x100);
}

/* Equal magnitudes: ((m >> 1) + (m >> 2)) >> 1 + m = (128 + 64) >> 1 + 256. */
static void test_dist_stage_diagonal(void) {
  const fifa96_arm_vec from = { 0, 0, 0 };
  const fifa96_arm_vec to = { 0x100, 0, 0x100 };
  int32_t distance = -1;
  int32_t lane = -1;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 0x160);
  assert(lane == 0x100);
}

/* |dx| < |dz| with |dx| > |dz|>>1: s = ((|dx|>>2) + (|dx|>>1)) >> 1). */
static void test_dist_stage_min_branch_adjust(void) {
  const fifa96_arm_vec from = { 0, 0, 0 };
  const fifa96_arm_vec to = { 0x40, 0, 0x64 };
  int32_t distance = -1;
  int32_t lane = -1;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 124); /* (16 + 32) >> 1 + 100 */
  assert(lane == 0x64);
}

/* |dx| < |dz| with |dx| <= |dz|>>1: s = |dx| >> 2. */
static void test_dist_stage_min_branch_plain(void) {
  const fifa96_arm_vec from = { 0, 0, 0 };
  const fifa96_arm_vec to = { 0x40, 0, 0x100 };
  int32_t distance = -1;
  int32_t lane = -1;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 0x110); /* 16 + 256 */
  assert(lane == 0x100);
}

/* The final sum is truncated to a 16-bit word (native `MOV [ECX],AX`):
 * (0x7FFF>>1 + 0x7FFF>>2)>>1 + 0x7FFF = 45054 -> -20482. */
static void test_dist_stage_distance_wraps16(void) {
  const fifa96_arm_vec from = { 0, 0, 0 };
  const fifa96_arm_vec to = { 0x7FFF, 0, 0x7FFF };
  int32_t distance = 0;
  int32_t lane = 0;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 45054 - 0x10000);
  assert(lane == 0x7FFF);
}

/* The differences are 16-bit wraps (native word loads/subtracts): 0x7FFF down
 * to 0 wraps to -0x7FFF. */
static void test_dist_stage_difference_wraps16(void) {
  const fifa96_arm_vec from = { 0, 0, 0x7FFF };
  const fifa96_arm_vec to = { 0, 0, 0 };
  int32_t distance = 0;
  int32_t lane = 0;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 0x7FFF);
  assert(lane == -0x7FFF);
}

/* |dx| > |dz| with |dx|>>1 < |dz|: the second-branch adjust
 * `d = ((|dz|>>2) + (|dz|>>1)) >> 1`. Both dx signs, since the branch reads
 * the magnitudes. */
static void test_dist_stage_max_branch_adjust(void) {
  fifa96_arm_vec from = { 0, 0, 0 };
  fifa96_arm_vec to = { 0x64, 0, 0x40 };   /* dx = +100, dz = 64 */
  int32_t distance = -1;
  int32_t lane = -1;
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 124);                 /* (16 + 32) >> 1 + 100 */
  assert(lane == 0x40);

  from.x = 0x64;
  to.x = 0;                                /* dx = -100, |dx| unchanged */
  assert(fifa96_arm_dist_stage(&from, &to, &distance, &lane) == FIFA96_OK);
  assert(distance == 124);
  assert(lane == 0x40);
}

static void test_dist_stage_invalid(void) {
  const fifa96_arm_vec from = { 0, 0, 0 };
  const fifa96_arm_vec to = { 1, 2, 3 };
  int32_t distance = 0;
  int32_t lane = 0;
  assert(fifa96_arm_dist_stage(NULL, &to, &distance, &lane) == ARM_INVALID);
  assert(fifa96_arm_dist_stage(&from, NULL, &distance, &lane) == ARM_INVALID);
  assert(fifa96_arm_dist_stage(&from, &to, NULL, &lane) == ARM_INVALID);
  assert(fifa96_arm_dist_stage(&from, &to, &distance, NULL) == ARM_INVALID);
}

/* --- `FUN_00079C50` face helper (row 27 call site 0x86913, Appendix D.4) ----
 * First-hand `0x79C50` (28 instructions): `DX|BX == 0` returns `byte[+0x8E]`
 * without writes (`0x79C59..0x79C68`); else `word[+0x7D] = FUN_000CD474(DX,BX)`
 * (the ported `fifa96_action_kick_angle`) and `byte[+0x8E] = ((angle + 0x40) &
 * 0x3FF) >> 7` (`0x79C69..0x79C98`). Row 27 passes DX = the `0x8DCD4` dx word
 * and BX = the dz word, so the derived surface recomputes the two 16-bit word
 * differences from pos/target. The octant divides the 0x400 angle circle into
 * 8 sectors of 0x80: +x -> 2, +z -> 0, -x -> 6, -z -> 4, sector boundaries
 * +x+z -> 1, -x+z -> 7, +x-z -> 3, -x-z -> 5. */

/* The native zero-direction guard returns the record's stored `+0x8E`: the
 * derived contract leaves the caller-seeded out value untouched. */
static void test_face_zero_direction_keeps_current(void) {
  const fifa96_arm_vec pos = { 0x100, 5, 0x200 };
  const fifa96_arm_vec target = { 0x100, -7, 0x200 };
  uint8_t lane = 0x77;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 0x77);
}

static void test_face_axis_octants(void) {
  const fifa96_arm_vec pos = { 0, 0, 0 };
  fifa96_arm_vec target = { 100, 0, 0 };
  uint8_t lane = 0xFF;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 2);                  /* +x = angle 0x100 */
  target.x = 0;
  target.z = 100;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 0);                  /* +z = angle 0 */
  target.x = -100;
  target.z = 0;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 6);                  /* -x = angle -0x100 */
  target.x = 0;
  target.z = -100;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 4);                  /* -z = angle 0x200 */
}

/* Equal magnitudes sit exactly on a sector boundary (angle 0x80 or its
 * quadrants); the native table returns 0x80 for the equal case. */
static void test_face_diagonal_octants(void) {
  const fifa96_arm_vec pos = { 0, 0, 0 };
  fifa96_arm_vec target = { 100, 0, 100 };
  uint8_t lane = 0xFF;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 1);
  target.x = -100;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 7);
  target.x = 100;
  target.z = -100;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 3);
  target.x = -100;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 5);
}

/* The direction words are 16-bit wraps (native word loads/subtracts):
 * pos.z = 0x7FFF down to target.z = 0 wraps to -0x7FFF -> -z octant 4. */
static void test_face_difference_wraps16(void) {
  const fifa96_arm_vec pos = { 0, 0, 0x7FFF };
  const fifa96_arm_vec target = { 0, 0, 0 };
  uint8_t lane = 0xFF;
  assert(fifa96_arm_face(&pos, &target, &lane) == FIFA96_OK);
  assert(lane == 4);
}

static void test_face_invalid(void) {
  const fifa96_arm_vec pos = { 0, 0, 0 };
  const fifa96_arm_vec target = { 1, 2, 3 };
  uint8_t lane = 0;
  assert(fifa96_arm_face(NULL, &target, &lane) == ARM_INVALID);
  assert(fifa96_arm_face(&pos, NULL, &lane) == ARM_INVALID);
  assert(fifa96_arm_face(&pos, &target, NULL) == ARM_INVALID);
}

/* --- `FUN_0006E598` id-resolution subset (row 27 0x8695C/0x869F8) -----------
 * First-hand `0x6E598..0x6E713`: a non-zero kind skips the phase/reroll block
 * (`0x6E608..0x6E61C`) and reaches the clamp `(int16)kind < 0 || >= 0x6F -> 0`
 * (`0x6E68E..0x6E69B`); kind 0 with a current row byte in `1..0x61`/`0x66..0x6E`
 * re-selects it (`0x6E622..0x6E655`), otherwise the native RNG reroll maps
 * RNG&3 to `{0, 0x62, 0x65}` (`0x6E659..0x6E685`) — the unmodelled open leg,
 * with the derived deterministic fallback 0. */

static void test_anim_select_passthrough(void) {
  uint8_t slot = 0xFF;
  assert(fifa96_arm_anim_select(0x3C, 0x99, &slot) == FIFA96_OK);
  assert(slot == 0x3C);
  assert(fifa96_arm_anim_select(0x6E, 0x00, &slot) == FIFA96_OK);
  assert(slot == 0x6E);
}

static void test_anim_select_clamps(void) {
  uint8_t slot = 0xFF;
  assert(fifa96_arm_anim_select(0x6F, 0x00, &slot) == FIFA96_OK);
  assert(slot == 0);
  assert(fifa96_arm_anim_select(0x80, 0x00, &slot) == FIFA96_OK);
  assert(slot == 0);
  assert(fifa96_arm_anim_select(0xFF, 0x00, &slot) == FIFA96_OK);
  assert(slot == 0);
}

static void test_anim_select_zero_continues_current_row(void) {
  uint8_t slot = 0xFF;
  assert(fifa96_arm_anim_select(0, 0x12, &slot) == FIFA96_OK);
  assert(slot == 0x12);
  assert(fifa96_arm_anim_select(0, 0x6E, &slot) == FIFA96_OK);
  assert(slot == 0x6E);
}

/* The `{0x62..0x65}` current rows and the no-current-row case take the native
 * RNG reroll; the derived deterministic fallback is id 0 (open leg OL-52). */
static void test_anim_select_zero_reroll_fallback(void) {
  uint8_t slot = 0xFF;
  assert(fifa96_arm_anim_select(0, 0x00, &slot) == FIFA96_OK);
  assert(slot == 0);
  assert(fifa96_arm_anim_select(0, 0x62, &slot) == FIFA96_OK);
  assert(slot == 0);
  assert(fifa96_arm_anim_select(0, 0x63, &slot) == FIFA96_OK);
  assert(slot == 0);
  assert(fifa96_arm_anim_select(0, 0x64, &slot) == FIFA96_OK);
  assert(slot == 0);
  assert(fifa96_arm_anim_select(0, 0x65, &slot) == FIFA96_OK);
  assert(slot == 0);
}

static void test_anim_select_invalid(void) {
  assert(fifa96_arm_anim_select(3, 0, NULL) == ARM_INVALID);
}

int main(void) {
  test_dist_stage_axis();
  test_dist_stage_lane_sign();
  test_dist_stage_diagonal();
  test_dist_stage_min_branch_adjust();
  test_dist_stage_min_branch_plain();
  test_dist_stage_max_branch_adjust();
  test_dist_stage_distance_wraps16();
  test_dist_stage_difference_wraps16();
  test_dist_stage_invalid();
  test_face_zero_direction_keeps_current();
  test_face_axis_octants();
  test_face_diagonal_octants();
  test_face_difference_wraps16();
  test_face_invalid();
  test_anim_select_passthrough();
  test_anim_select_clamps();
  test_anim_select_zero_continues_current_row();
  test_anim_select_zero_reroll_fallback();
  test_anim_select_invalid();
  puts("test_arm_helpers OK");
  return 0;
}
