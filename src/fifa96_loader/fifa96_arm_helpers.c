/* src/fifa96_loader/fifa96_arm_helpers.c — M2 arms-and-wiring Tasks 3/4/5 /
 * FU-142b: the shared cluster-G helpers `FUN_0008DCD4` (distance/staging),
 * `FUN_00079C50` (face), the `FUN_0006E598` id-resolution subset and the
 * `FUN_0007DAB4` reset subset (`fifa96_arm_reset`).
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C
 * (read-only /FIFA96.EXE: decompile_function + disassemble_function 0x8DCD4,
 * 61 instructions, body 0x8DCD4..0x8DD5B; the two row-0x26-era call sites
 * 0x867D1 and 0x8D11E) and Appendix D (disassemble_function 0x79C50, 28
 * instructions; disassemble_function 0x6E598, 0x6E598..0x6E713); Appendix E
 * (disassemble_function 0x7DAB4, 35 instructions, body 0x7DAB4..0x7DB0C). */
#include "fifa96_loader/fifa96_arm_helpers.h"

#include "fifa96_loader/fifa96_action_handlers.h"

fifa96_err_t fifa96_arm_reset(struct fifa96_arm_record *rec) {
  if (!rec) return -FIFA96_ERR_INVALID;
  /* First-hand 0x7DAB4: the two unconditional writes (0x7DABA/0x7DAC4) and
   * the code-0 re-install (0x7DAFB..0x7DB03, `XOR ECX/EBX/EDX` then
   * `CALL 0x7D9A4`), represented as the record's +0x91 code byte. */
  rec->stage92 = 0xFF;
  rec->timer89 = 0;
  rec->code = 0;
  return FIFA96_OK;
}

fifa96_err_t fifa96_arm_camera_stop(void) {
  /* First-hand /FIFA96.EXE 0x513EC..0x51440 (33 instructions): all of the
   * routine's effects are on the unported camera-mode/recorder block
   * (`[0x14E584]/[0x14E580]` clear 0x513F9/0x513FF, the `[0x14E5A8]`-gated
   * callback via `[0x14E570]+0x38` 0x51413, `[0x14E578] = 2` 0x51421, the
   * `[0x14E574]` first-entry latch + `FUN_00064074` 0x5142B..0x51435), so the
   * derived surface is a documented no-op (Appendix H open leg). The FU-118
   * doc's 0x4E5xx are these EXE operands minus the 0x100000 LE image delta. */
  return FIFA96_OK;
}

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
  if (kind != 0) {
    slot = kind;
  } else {
    /* 0x6E62F..0x6E655 + 0x6E687: the native keeps the current row byte when
     * it is 0 or 0x62..0x65 (EAX=0 -> 0x6E657 JZ 0x6E687); a non-zero
     * non-special byte (EAX=1, 0x6E64C) and the NULL row pointer (0x6E627)
     * take the RNG reroll, which stays OL-52 with the derived fallback 0.
     * `row` is the caller's stand-in for byte[[rec+0x28]] (header contract). */
    if (row == 0 || (row >= 0x62u && row <= 0x65u)) slot = row;
    else slot = 0;
  }
  /* 0x6E68E..0x6E69B: (int16)id < 0 || >= 0x6F -> 0. */
  if (slot >= 0x6Fu) slot = 0;
  *out_slot = slot;
  return FIFA96_OK;
}
