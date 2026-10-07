/* src/fifa96_loader/fifa96_arm_bodies.c — M2 arms-and-wiring Tasks 3/4 /
 * FU-142b: the row 0x26 body, the row 0x27 body and the shared 0x36200 stub.
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C
 * (read-only /FIFA96.EXE: disassemble_bytes 0x866F4, read_memory 0x110778 =
 * row-0x26 table entry 0x000866F4, read_memory 0x10F394 table bytes,
 * disassemble_bytes 0x36200) and Appendix D (disassemble_bytes 0x86820,
 * read_memory 0x1103CB = the 96-byte 24-pair table, read_memory 0x158782 /
 * 0x10F372). */
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

/* Flat 0x1103CB: 96 bytes = 24 {id, time} pairs (first-hand read, Appendix
 * D.2). The stage-1 call selects the row's pair-0 id word (0x86956); the
 * stage-2 walk reads the current pair's high word as a signed time (0x869a4)
 * and, on fire, the next pair's low word as the next id (0x869e9). */
static const struct fifa96_arm_27_pair {
  uint16_t id;
  int16_t time;
} fifa96_arm_27_pairs[24] = {
  { 0x006B, 0x0168 }, { 0x0068, -1 },     { 0x0003, 0x0078 },
  { 0x0003, 0x001E }, { 0x0016, -1 },     { 0x0003, 0x00F0 },
  { 0x0015, 0x001E }, { 0x0056, 0x00F0 }, { 0x0015, 0x00F0 },
  { 0x0067, 0x0168 }, { 0x0003, 0x0078 }, { 0x0003, 0x0078 },
  { 0x0015, 0x001E }, { 0x0050, -1 },     { 0x0003, 0x00F0 },
  { 0x0003, 0x001E }, { 0x001A, -1 },     { 0x0015, 0x00F0 },
  { 0x0003, 0x001E }, { 0x0019, -1 },     { 0x0015, 0x00F0 },
  { 0x0015, 0x001E }, { 0x0068, -1 },     { 0x0003, 0x00F0 }
};

fifa96_err_t fifa96_arm_27_step(struct fifa96_arm_record *rec) {
  fifa96_arm_vec tgt;
  uint8_t face;
  uint8_t sel;
  int32_t distance;
  int32_t lane;
  uint32_t pair;
  int16_t thr;
  if (!rec) return -FIFA96_ERR_INVALID;
  /* The derived bound flag reports this call's walk: cleared on entry. */
  rec->anim_overflow = 0;
  /* 0x86829..0x86837: timer89 += zero-extended delta word. */
  rec->timer89 = (int32_t)((uint32_t)rec->timer89 + (uint32_t)rec->delta);
  /* 0x8683d..0x8687d: target.x = 0x780, target.z =
   * +/- (int16)(6 * ((int8)active >> 1)) with the sign in active bit 0. */
  {
    int16_t off16 = (int16_t)(6 * ((int32_t)((int8_t)rec->active) >> 1));
    rec->target.x = 0x780;
    rec->target.z = (rec->active & 1u) ? (int32_t)off16 : -(int32_t)off16;
  }
  tgt = rec->target;             /* face reads the pre-retarget triple (0x86913) */
  /* 0x8687f..0x86888: the shared 0x8DCD4 distance/out triple. */
  if (fifa96_arm_dist_stage(&rec->pos, &tgt, &distance, &lane) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  rec->lane = lane;
  /* 0x8688d..0x868b5: |lane| < 0x20 -> retarget (0xCC0, 0). */
  if (lane < 0) lane = -lane;
  if (lane < 0x20) {
    rec->target.x = 0xCC0;
    rec->target.z = 0;
  }
  /* 0x868b7..0x868c6: an active record ends after the placement. */
  if (rec->active != 0) return FIFA96_OK;
  /* 0x868cc..0x868e7: the stage latch; 0 -> 1, 1 -> face/select, 2 -> walk. */
  if (rec->stage92 == 0) {
    rec->timer89 = 0;
    rec->stage92 = 1;
  }
  if (rec->stage92 == 1) {
    /* 0x86905..0x86913: face on the (dx, dz) direction. */
    face = rec->type;
    if (fifa96_arm_face(&rec->pos, &tgt, &face) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    rec->type = face;
    /* 0x86918..0x86933: [0x158782] + 1, wrapping at 8. */
    rec->anim_cycle = (uint8_t)(rec->anim_cycle + 1u);
    if (rec->anim_cycle >= 8u) rec->anim_cycle = 0;
    /* 0x8693a..0x8695c: play the row's pair-0 id. */
    if (fifa96_arm_anim_select((uint8_t)fifa96_arm_27_pairs[3u * rec->anim_cycle].id,
                               rec->anim_sel, &sel) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    rec->anim_sel = sel;
    /* 0x86961..0x8697b: timer89 = 0, [0x10F374] = 0, stage 1 -> 2. */
    rec->timer89 = 0;
    rec->anim_cursor = 0;
    rec->stage92 = (uint8_t)(rec->stage92 + 1u);
  }
  if (rec->stage92 == 2) {
    /* 0x86981..0x869a4: pair = row base 3*cycle + cursor. */
    pair = 3u * (uint32_t)rec->anim_cycle + (uint32_t)rec->anim_cursor;
    if (pair >= 24u) {
      rec->anim_overflow = 1;    /* derived bound; native reads past 0x11042B */
      return FIFA96_OK;
    }
    thr = fifa96_arm_27_pairs[pair].time;
    if (thr >= 0) {
      /* 0x869b7..0x869c4: fire iff time < timer89 (JGE skips). */
      if (rec->timer89 <= (int32_t)thr) return FIFA96_OK;
    } else {
      /* 0x869af..0x869b5: negative time fires iff +0x44 != 0. */
      if (rec->flag44 == 0) return FIFA96_OK;
    }
    /* 0x869c6..0x869f8: fire; timer89 = 0, cursor++, play the next id. */
    rec->timer89 = 0;
    rec->anim_cursor++;
    if (pair + 1u >= 24u) {
      rec->anim_overflow = 1;
      return FIFA96_OK;
    }
    if (fifa96_arm_anim_select((uint8_t)fifa96_arm_27_pairs[pair + 1u].id,
                               rec->anim_sel, &sel) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    rec->anim_sel = sel;
  }
  return FIFA96_OK;
}
