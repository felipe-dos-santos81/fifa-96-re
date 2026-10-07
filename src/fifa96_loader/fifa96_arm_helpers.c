/* src/fifa96_loader/fifa96_arm_helpers.c — M2 arms-and-wiring Tasks 3/4 /
 * FU-142b: the shared cluster-G helpers `FUN_0008DCD4` (distance/staging),
 * `FUN_00079C50` (face) and the `FUN_0006E598` id-resolution subset.
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C
 * (read-only /FIFA96.EXE: decompile_function + disassemble_function 0x8DCD4,
 * 61 instructions, body 0x8DCD4..0x8DD5B; the two row-0x26-era call sites
 * 0x867D1 and 0x8D11E) and Appendix D (disassemble_function 0x79C50, 28
 * instructions; disassemble_function 0x6E598, 0x6E598..0x6E713). */
#include "fifa96_loader/fifa96_arm_helpers.h"

#include "fifa96_loader/fifa96_action_handlers.h"

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

fifa96_err_t fifa96_arm_face(const fifa96_arm_vec *pos, const fifa96_arm_vec *target,
                             uint8_t *out_lane) {
  int16_t dx;
  int16_t dz;
  int32_t angle;
  if (!pos || !target || !out_lane) return -FIFA96_ERR_INVALID;
  /* 0x8DCDD..0x8DCED: the same 16-bit word differences 0x8DCD4 takes; row 27
   * passes the dx/dz words to 0x79C50 (0x86905..0x86913). */
  dx = (int16_t)((uint16_t)target->x - (uint16_t)pos->x);
  dz = (int16_t)((uint16_t)target->z - (uint16_t)pos->z);
  /* 0x79C59..0x79C68: zero direction returns the stored +0x8E byte untouched
   * (the caller seeds *out_lane with it). */
  if (dx == 0 && dz == 0) return FIFA96_OK;
  /* 0x79C69..0x79C71: FUN_000CD474(DX,BX) = the FU-76 §5 angle port. */
  if (fifa96_action_kick_angle(dx, dz, &angle) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  /* 0x79C7A..0x79C8E: +0x8E = ((angle + 0x40) & 0x3FF) >> 7. */
  *out_lane = (uint8_t)(((uint32_t)(angle + 0x40) & 0x3FFu) >> 7u);
  return FIFA96_OK;
}

fifa96_err_t fifa96_arm_anim_select(uint8_t kind, uint8_t row, uint8_t *out_slot) {
  uint8_t slot;
  if (!out_slot) return -FIFA96_ERR_INVALID;
  slot = kind;
  if (kind == 0) {
    /* 0x6E622..0x6E655: a current row byte in 1..0x61/0x66..0x6E re-selects
     * itself; otherwise the native RNG reroll (0x6E659..0x6E685) is the
     * documented open leg (OL-52) and the derived fallback is id 0. */
    if (row != 0 && !(row >= 0x62u && row <= 0x65u)) slot = row;
    else slot = 0;
  }
  /* 0x6E68E..0x6E69B: (int16)id < 0 || >= 0x6F -> 0. */
  if (slot >= 0x6Fu) slot = 0;
  *out_slot = slot;
  return FIFA96_OK;
}
