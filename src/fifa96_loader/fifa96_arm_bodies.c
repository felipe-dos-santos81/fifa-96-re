/* src/fifa96_loader/fifa96_arm_bodies.c — M2 arms-and-wiring Task 3 /
 * FU-142b: the row 0x26 body and the shared 0x36200 stub.
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C
 * (read-only /FIFA96.EXE: disassemble_bytes 0x866F4, read_memory 0x110778 =
 * row-0x26 table entry 0x000866F4, read_memory 0x10F394 table bytes,
 * disassemble_bytes 0x36200). */
#include "fifa96_loader/fifa96_arm_bodies.h"

/* Flat 0x10F394, the 32-byte table pointed to by the runtime [0x157A38]
 * (FUN_00073CD0 stores 0xF394; FU-84 §6 re-reads the same bytes). Row 26 reads
 * it with the signed byte rec[+0x4][+0xD]. */
static const uint8_t fifa96_arm_26_timer_table[32] = {
  0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
  0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
  0x0F, 0x0F, 0x0F, 0x0E, 0x0D, 0x0D, 0x0C, 0x0B,
  0x0B, 0x0A, 0x09, 0x09, 0x08, 0x07, 0x07, 0x06
};

fifa96_err_t fifa96_arm_stub_36200(void) {
  return FIFA96_OK;
}

fifa96_err_t fifa96_arm_26_step(struct fifa96_arm_record *rec) {
  int16_t off16;
  int32_t distance;
  int32_t lane;
  if (!rec) return -FIFA96_ERR_INVALID;
  if (rec->player_d < 0 || rec->player_d > 31) return -FIFA96_ERR_INVALID;
  /* 0x86702..0x8670D: zero-extended word delta added to the 32-bit timer. */
  rec->timer89 = (int32_t)((uint32_t)rec->timer89 + (uint32_t)rec->delta);
  /* 0x86713..0x86729: timer7b = 0x10F394[rec[+4][+0xD]] >> 1. */
  rec->timer7b = (uint16_t)(fifa96_arm_26_timer_table[(uint8_t)rec->player_d] >> 1);
  /* 0x8672D..0x86740: stage latch gate; 2..0xFF return after the prologue. */
  if (rec->stage92 == 0) {
    if (15 * (int32_t)rec->player_e > rec->timer89) return FIFA96_OK; /* 0x86764 */
    rec->timer89 = 0;
    rec->stage92 = 1;
  } else if (rec->stage92 != 1) {
    return FIFA96_OK;
  }
  /* 0x86782..0x867C2: target.x = 0x780, target.z = +/-6 * (active >> 1) with
   * the sign in active bit 0 (the byte is a sign-magnitude offset class). */
  off16 = (int16_t)(6 * ((int32_t)((int8_t)rec->active) >> 1));
  rec->target.x = 0x780;
  rec->target.z = (rec->active & 1u) ? (int32_t)off16 : -(int32_t)off16;
  {
    fifa96_err_t rc = fifa96_arm_dist_stage(&rec->pos, &rec->target, &distance, &lane);
    if (rc != FIFA96_OK) return rc;           /* 0x867D1 */
  }
  rec->lane = lane;                           /* helper out[2] at +0x69 */
  if (lane < 0) lane = -lane;                 /* 0x867D6..0x867EB */
  if (lane >= 0x20) return FIFA96_OK;         /* 0x867F0 */
  rec->target.x = 0xCC0;                      /* 0x867F2..0x86812 */
  rec->target.z = 0;
  rec->timer89 = 0;
  rec->stage92 = (uint8_t)(rec->stage92 + 1u);
  return FIFA96_OK;
}
