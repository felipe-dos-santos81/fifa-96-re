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
 * First-hand `0x6E598..0x6E713`: a non-zero kind skips the current-row/RNG
 * block (`0x6E608..0x6E61C`) and reaches the clamp `(int16)kind < 0 || >= 0x6F
 * -> 0` (`0x6E68E..0x6E69B`). For kind 0 the native reads the current row
 * `[rec+0x28]` (`0x6E622..0x6E627`): a NULL pointer takes the RNG reroll
 * (`0x6E627` -> `0x6E659`, RNG `0x92AC8`, `RNG & 3` -> `{0, 0x62, 0x65}` at
 * `0x6E659..0x6E685`); a row byte 0 of `0` or `0x62..0x65` sets EAX=0 and
 * keeps that byte (`0x6E62F`/`0x6E638..0x6E64A` -> `0x6E653` -> `0x6E657
 * JZ 0x6E687`); a non-zero non-special byte sets EAX=1 and takes the same RNG
 * reroll (`0x6E64C..0x6E651`). The derived helper covers the native
 * special-byte keep rule with the caller-supplied row-byte stand-in; the
 * native `[rec+0x28]` source and the RNG draws are the OL-52 open leg, with
 * derived reroll fallback 0. */

static void test_anim_select_passthrough(void) {
  uint8_t slot = 0xFF;
  assert(fifa96_arm_anim_select(0x3C, 0x99, &slot) == FIFA96_OK);
  assert(slot == 0x3C);
  assert(fifa96_arm_anim_select(0x6E, 0x00, &slot) == FIFA96_OK);
  assert(slot == 0x6E);
  /* kind != 0 skips the current-row block entirely (0x6E608/0x6E61C). */
  assert(fifa96_arm_anim_select(3, 0x62, &slot) == FIFA96_OK);
  assert(slot == 3);
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

/* kind 0 with the native special row bytes 0 / 0x62..0x65 keeps the byte
 * (`0x6E62F`/`0x6E638..0x6E64A` -> `0x6E653` EAX=0 -> `0x6E657 JZ 0x6E687`). */
static void test_anim_select_zero_special_keeps_row(void) {
  uint8_t slot = 0xFF;
  assert(fifa96_arm_anim_select(0, 0x00, &slot) == FIFA96_OK);
  assert(slot == 0x00);
  assert(fifa96_arm_anim_select(0, 0x62, &slot) == FIFA96_OK);
  assert(slot == 0x62);
  assert(fifa96_arm_anim_select(0, 0x63, &slot) == FIFA96_OK);
  assert(slot == 0x63);
  assert(fifa96_arm_anim_select(0, 0x64, &slot) == FIFA96_OK);
  assert(slot == 0x64);
  assert(fifa96_arm_anim_select(0, 0x65, &slot) == FIFA96_OK);
  assert(slot == 0x65);
}

/* kind 0 with a non-zero non-special row byte takes the native RNG reroll
 * (`0x6E64C..0x6E651` -> `0x6E659..0x6E685`); the draws are the OL-52 open
 * leg, so the derived deterministic fallback is id 0. */
static void test_anim_select_zero_reroll_fallback(void) {
  uint8_t slot = 0xFF;
  assert(fifa96_arm_anim_select(0, 0x12, &slot) == FIFA96_OK);
  assert(slot == 0);
  assert(fifa96_arm_anim_select(0, 0x6E, &slot) == FIFA96_OK);
  assert(slot == 0);
}

static void test_anim_select_invalid(void) {
  assert(fifa96_arm_anim_select(3, 0, NULL) == ARM_INVALID);
}

/* --- `FUN_0007DAB4` derived reset subset (row 2C 0x84624, Appendix E) ------
 * First-hand `0x7DAB4..0x7DB0C` (35 instructions): `[rec+0x92] = 0xFF`
 * (0x7DABA), `[rec+0x89] = 0` (0x7DAC4), then `FUN_0007D9A4(rec, code 0,
 * staged 0, no-invoke)` (0x7DAFB..0x7DB03). The derived subset (FU-141 §3.4)
 * keeps `stage92 = 0xFF`, `timer89 = 0` and the code-0 re-install (`code = 0`);
 * the `[rec+0x20]` slot callback, the phase-2 forced-decision arm and the
 * installer's accepted-install tail stay the OL-54/open-pool surfaces. */

/* The reset writes exactly its three fields; every other record field is
 * untouched. */
static void test_arm_reset_fields(void) {
  struct fifa96_arm_record rec;
  rec.stage92 = 0x77;
  rec.timer89 = 0x1234;
  rec.code = 0x2C;
  rec.target.x = 0x780;
  rec.target.z = -6;
  rec.lane = 0x40;
  rec.active = 3;
  rec.anim_sel = 0x11;
  rec.delta = 7;
  assert(fifa96_arm_reset(&rec) == FIFA96_OK);
  assert(rec.stage92 == 0xFF);
  assert(rec.timer89 == 0);
  assert(rec.code == 0);
  assert(rec.target.x == 0x780);
  assert(rec.target.z == -6);
  assert(rec.lane == 0x40);
  assert(rec.active == 3);
  assert(rec.anim_sel == 0x11);
  assert(rec.delta == 7);
}

static void test_arm_reset_invalid(void) {
  assert(fifa96_arm_reset(NULL) == ARM_INVALID);
}

/* --- `FUN_000513EC` camera stop (row-2A arm-2 site 0x86B6E, Appendix H) ----
 * First-hand `0x513EC..0x51440` (33 instructions): the routine clears the
 * `[0x4E584]`/`[0x4E580]` gate dwords, calls the `[0x4E5A8]`-gated table
 * callback via `[0x14E570]+0x38`, sets camera mode `[0x4E578] = 2` and on first
 * entry latches `[0x4E574]` after the `FUN_00064074` recorder-gate clear. The
 * derived engine models none of those globals (the camera-mode/recorder block
 * is unported), so the derived surface is a documented stateless no-op: every
 * call returns FIFA96_OK and touches no caller state. */
static void test_camera_stop_returns_ok(void) {
  assert(fifa96_arm_camera_stop() == FIFA96_OK);
}

static void test_camera_stop_stateless_repeat(void) {
  assert(fifa96_arm_camera_stop() == FIFA96_OK);
  assert(fifa96_arm_camera_stop() == FIFA96_OK);
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
  test_anim_select_zero_special_keeps_row();
  test_anim_select_zero_reroll_fallback();
  test_anim_select_invalid();
  test_arm_reset_fields();
  test_arm_reset_invalid();
  test_camera_stop_returns_ok();
  test_camera_stop_stateless_repeat();
  puts("test_arm_helpers OK");
  return 0;
}
