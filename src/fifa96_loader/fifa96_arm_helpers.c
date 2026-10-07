/* src/fifa96_loader/fifa96_arm_helpers.c — M2 arms-and-wiring Task 3 /
 * FU-142b: the `FUN_0008DCD4` distance/staging helper.
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C
 * (read-only /FIFA96.EXE: decompile_function + disassemble_function 0x8DCD4,
 * 61 instructions, body 0x8DCD4..0x8DD5B; the two row-0x26-era call sites
 * 0x867D1 and 0x8D11E). */
#include "fifa96_loader/fifa96_arm_helpers.h"

fifa96_err_t fifa96_arm_dist_stage(const fifa96_arm_vec *from, const fifa96_arm_vec *to,
                                   int32_t *out_distance, int32_t *out_lane) {
  int16_t dx;
  int16_t dz;
  int16_t dist;
  if (!from || !to || !out_distance || !out_lane) return -FIFA96_ERR_INVALID;
  /* 0x8DCDD..0x8DCED: word loads and 16-bit subtracts; the y components are
   * never read. */
  dx = (int16_t)((uint16_t)to->x - (uint16_t)from->x);
  dz = (int16_t)((uint16_t)to->z - (uint16_t)from->z);
  *out_lane = dz;                             /* 0x8DCF3: out[2] = dz word */
  if (dx < 0) dx = (int16_t)-dx;              /* 0x8DCF7..0x8DD03 */
  if (dz < 0) dz = (int16_t)-dz;
  /* Octagonal distance `max + f(min)` in the native's 16-bit steps
   * (0x8DD05..0x8DD58). */
  if (dx < dz) {
    int i4 = dx;
    dist = (int16_t)(i4 >> 2);
    if ((int)dz >> 1 < i4) dist = (int16_t)(((int)dist + (i4 >> 1)) >> 1);
    dist = (int16_t)(dist + dz);
  } else {
    if (dz < dx) {
      int i4 = dz;
      dist = (int16_t)(i4 >> 2);
      dz = dx;
      if ((int)dx >> 1 < i4) dist = (int16_t)(((int)dist + (i4 >> 1)) >> 1);
    } else {
      dist = (int16_t)((((int)dx >> 1) + ((int)dx >> 2)) >> 1);
    }
    dist = (int16_t)(dist + dz);
  }
  *out_distance = dist;                       /* 0x8DD55: out[0] = distance */
  return FIFA96_OK;
}
