/* tests/test_arm_helpers.c — M2 arms-and-wiring Task 3 / FU-142b: the
 * `FUN_0008DCD4` distance/staging helper port.
 *
 * Evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C (Ghidra
 * read-only /FIFA96.EXE: get_function_by_address/decompile_function/
 * disassemble_function 0x8DCD4, 61 instructions, body 0x8DCD4..0x8DDBB). The
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

int main(void) {
  test_dist_stage_axis();
  test_dist_stage_lane_sign();
  test_dist_stage_diagonal();
  test_dist_stage_min_branch_adjust();
  test_dist_stage_min_branch_plain();
  test_dist_stage_distance_wraps16();
  test_dist_stage_difference_wraps16();
  test_dist_stage_invalid();
  puts("test_arm_helpers OK");
  return 0;
}
