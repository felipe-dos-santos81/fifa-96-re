/* src/fifa96_loader/fifa96_arm_bodies.c — M2 arms-and-wiring Tasks 3/4/5/6/7 /
 * FU-142b/c/d: the row 0x26 body (Task 3), the row 0x27 body (Task 4), the row
 * 0x2C body (Task 5), the row 0x29 body (Task 6), the row 0x28 4-arm body
 * (Task 7) and the shared 0x36200 stub.
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix C
 * (read-only /FIFA96.EXE: disassemble_bytes 0x866F4, read_memory 0x110778 =
 * row-0x26 table entry 0x000866F4, read_memory 0x10F394 table bytes,
 * disassemble_bytes 0x36200), Appendix D (disassemble_bytes 0x86820,
 * read_memory 0x1103CB = the 96-byte 24-pair table, read_memory 0x158782 /
 * 0x10F372), Appendix E (disassemble_bytes 0x84598, 152 B; read_memory
 * 0x110790 = row-0x2C table entry 0x00084598; disassemble_function 0x7DAB4,
 * the reset subset in `fifa96_arm_reset`), Appendix F (disassemble_bytes
 * 0x874E4, 596 B; read_memory 0x110784 = row-0x29 table entry 0x000874E4;
 * the 0x8DE8C nearest and 0x6E1D0 phase-cell windows) and Appendix G
 * (disassemble_bytes 0x870E8, 1024 B; read_memory 0x110780 = row-0x28 table
 * entry 0x000870E8; read_memory 0x870D8 = the 4-arm table; disassemble_bytes
 * 0x87014, 200 B; read_memory 0x7D8B0 and 0x114E04). */
#include "fifa96_loader/fifa96_arm_bodies.h"

#include "fifa96_loader/fifa96_action_handlers.h"
#include "fifa96_loader/fifa96_entity_update.h"
#include "fifa96_loader/fifa96_projection.h"
#include "fifa96_loader/fifa96_rng.h"

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

fifa96_err_t fifa96_arm_2c_step(struct fifa96_arm_record *rec) {
  uint8_t slot;
  if (!rec) return -FIFA96_ERR_INVALID;
  /* 0x8459E..0x845AC: the stage latch; 2 falls to the countdown and >= 3
   * returns at the prologue pop (0x845AE..0x845B2). */
  if (rec->stage92 == 0) {
    /* 0x845B7..0x845BE: an inactive record jumps straight to the reset. */
    if (rec->active == 0) return fifa96_arm_reset(rec);   /* 0x84622..0x84624 */
    rec->timer89 = 0;                                     /* 0x845C6 */
    rec->stage92 = 1;                                     /* 0x845D0..0x845D2 */
  } else if (rec->stage92 > 2) {
    return FIFA96_OK;
  }
  if (rec->stage92 == 1) {
    /* 0x845D8..0x845EA: 0x6E598(rec, id 0x5D, frame 0). EDX=0x5D is a
     * constant; the `MOV ECX,[rec+0x8B]>>24` load is dead (FU-84 §1: ECX is
     * not an input) and EBX=0 is the frame index dropped by the derived
     * selector (FU-142 OL-52). `rec->anim_sel` is the row-byte stand-in and
     * records the resolved id. */
    if (fifa96_arm_anim_select(0x5D, rec->anim_sel, &slot) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    rec->anim_sel = slot;
    rec->timer89 = 0;                                     /* 0x845F5 */
    rec->stage92 = (uint8_t)(rec->stage92 + 1u);          /* 0x845FF..0x84601 */
  }
  /* 0x84607..0x84620: timer89 -= zero-extended word [0x157A64]; the row
   * waits while the result is strictly positive (signed JG). */
  rec->timer89 = (int32_t)((uint32_t)rec->timer89 - (uint32_t)rec->delta);
  if (rec->timer89 > 0) return FIFA96_OK;
  return fifa96_arm_reset(rec);                           /* 0x84622..0x84624 */
}

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

/* Row 0x28 (`0x870E8..0x874E3`, 294 instructions; Appendix G). The native
 * prologue runs the shared 0x8DCD4 out triple and the 0x79C50 face call before
 * the stage92 jump table `0x870D8` = {0x87147, 0x87276, 0x87364, 0x874DA}.
 * `arm` is that table selector (the native stage92 byte; > 3 -> epilogue). */

/* The native x/z folds (`0x87184..0x871C6` / `0x871CD..0x87216`) are the
 * byte-shift quadrant idiom over the 257-entry 0x114E04 table followed by
 * `IMUL 0x90; ADD 0x8000; ADC; SHRD 16`. The port reuses the already-ported
 * table + quadrant fold (`fifa96_projection_sincos`, FU-88) at the 1024-step
 * angle `(int8)active * 0x40` and applies the same 0x90 multiply/round; the
 * equivalence was brute-forced over all 256 active values against a literal
 * transcription of the native idiom (Appendix G.2) and is pinned by the arm-0
 * fold fixtures. */
static int32_t fifa96_arm_28_disp(int32_t component) {
  return (int32_t)(((int64_t)component * 0x90 + 0x8000) >> 16);
}

/* The shared `0x87014` stage-gate setup (`0x87014..0x870D5`, 54 instructions;
 * first-hand `get_xrefs_to 0x87014` = exactly the two row-28 call sites
 * `0x872C5`/`0x873A9`). Six RNG draws: +0xA2 = max(draw0 & 0xFF, 0x48) signed
 * by draw1 bit 0, +0xA6 = max(draw2 & 0xFF, 0x48) signed by draw3 bit 0,
 * timer89 = 0, +0xAA = (draw4 & 0xFF) + 0x78, +0xAE = +0xAA + (draw5 & 0xFF). */
static fifa96_err_t fifa96_arm_28_setup(struct fifa96_arm_record *rec) {
  uint16_t r;
  int32_t gate;
  if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  gate = (int32_t)(r & 0xFFu);
  rec->scratch_a2 = gate < 0x48 ? 0x48 : gate;              /* 0x8701E..0x8702C */
  if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if (r & 1u) rec->scratch_a2 = -rec->scratch_a2;           /* 0x87036..0x87056 */
  if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  gate = (int32_t)(r & 0xFFu);
  rec->scratch_a6 = gate < 0x48 ? 0x48 : gate;              /* 0x87058..0x8706B */
  if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  if (r & 1u) rec->scratch_a6 = -rec->scratch_a6;           /* 0x87075..0x87098 */
  rec->timer89 = 0;                                         /* 0x8709A */
  if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  rec->scratch_aa = (int32_t)(r & 0xFFu) + 0x78;            /* 0x870A4..0x870B1 */
  if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
  rec->scratch_ae = rec->scratch_aa + (int32_t)(r & 0xFFu); /* 0x870B7..0x870CC */
  return FIFA96_OK;
}

fifa96_err_t fifa96_arm_28_step(struct fifa96_arm_record *rec, uint8_t arm) {
  int32_t distance;
  int32_t lane;
  uint8_t face;
  if (!rec) return -FIFA96_ERR_INVALID;
  /* Prologue `0x870E8..0x87127`: EAX=rec, EBX=rec+0x65; the 0x8DCD4 out triple
   * (pos +0x59 vs target +0x4D) then the 0x79C50 face call on the dx/dz words
   * (+0x67/+0x69 -> +0x8E octant). */
  if (fifa96_arm_dist_stage(&rec->pos, &rec->target, &distance, &lane) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  rec->lane = lane;
  face = rec->type;
  if (fifa96_arm_face(&rec->pos, &rec->target, &face) != FIFA96_OK)
    return -FIFA96_ERR_INVALID;
  rec->type = face;
  /* 0x8712C..0x8713F: stage92 jump; the table's arm 3 (0x874DA) is the shared
   * epilogue, and > 3 also exits. */
  if (arm > 3u) return FIFA96_OK;
  if (arm == 0u) {
    /* 0x87147: [0x157AA3] = [rec+0] (unmodeled cross-record store, OL-57);
     * 0x87154: the 0x36200 stub with native EAX=2 (no-op, OL-51). */
    (void)fifa96_arm_stub_36200();
    rec->target.x = rec->global_10f364;                     /* 0x87159..0x87166 */
    rec->target.z = rec->global_10f368;
    /* 0x87169..0x87181: below mode 4, a side-0 team negates the z target. */
    if (rec->global_157ac2 < 4u && rec->side == 0u)
      rec->target.z = -rec->target.z;
    {
      /* 0x87184..0x87216: the two 0x114E04 folds (x = cos, z = sin). */
      int32_t sin16, cos16;
      uint32_t angle = (uint32_t)(int32_t)(int8_t)rec->active << 12;
      if (fifa96_projection_sincos((int32_t)angle, &sin16, &cos16) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      rec->target.x += fifa96_arm_28_disp(cos16);
      rec->target.z += fifa96_arm_28_disp(sin16);
    }
    /* 0x87213..0x87237: skip the constant-id 0x6E598 call when the current row
     * byte is already 0x15 (`anim_sel` is the derived stand-in, OL-57). */
    if (rec->anim_sel != 0x15u) {
      uint8_t slot;
      if (fifa96_arm_anim_select(0x15, rec->anim_sel, &slot) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      rec->anim_sel = slot;
    }
    /* 0x8723C..0x87245: the 0x8DCD4 distance word (+0x65) gates > 0x60. */
    if (distance > 0x60) return FIFA96_OK;
    {
      /* 0x8724B..0x87270: one draw, timer89 = 0, stage92++, +0xA2 gate. */
      uint16_t r;
      if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      rec->timer89 = 0;
      rec->stage92 = (uint8_t)(rec->stage92 + 1u);
      rec->scratch_a2 = (int32_t)(r & 0x7Fu) + 0x20;
    }
    /* Falls through into arm 1 (`0x87270` -> `0x87276`). */
  }
  if (arm == 0u || arm == 1u) {
    /* 0x87276..0x87294: timer89 += zero-extended delta; below the +0xA2 gate
     * the record waits (JL epilogue; signed compare). */
    rec->timer89 = (int32_t)((uint32_t)rec->timer89 + (uint32_t)rec->delta);
    if (rec->timer89 < rec->scratch_a2) return FIFA96_OK;
    {
      /* 0x8729A..0x872A7: re-arm +0xA2. */
      uint16_t r;
      if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      rec->scratch_a2 = (int32_t)(r & 0x7Fu) + 0x20;
    }
    rec->timer89 = 0;                                       /* 0x872B0 */
    if (rec->flag830 == 0u) {
      /* 0x8733A..0x87363: flag830 clear -> target = pos, constant id 1. */
      uint8_t slot;
      rec->target = rec->pos;
      if (fifa96_arm_anim_select(1, rec->anim_sel, &slot) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      rec->anim_sel = slot;
      return FIFA96_OK;
    }
    /* 0x872C3..0x87335: flag830 set -> the 0x87014 setup, then the +0xAA/
     * +0xAE/+0xA0/+0xA1 draws, timer89 = 0, stage92++. */
    if (fifa96_arm_28_setup(rec) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    {
      uint16_t r;
      if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      rec->scratch_aa = (int32_t)(r & 0xFFu) + 0xF0;        /* 0x872CA..0x872D9 */
      if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      rec->scratch_ae = (int32_t)(r & 0x7Fu) + 0x20;        /* 0x872DF..0x872EC */
      if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
      rec->scratch_a0 = (uint8_t)(r & 1u);                  /* 0x872F2..0x872F9 */
      rec->scratch_a1 = 0;                                  /* 0x87305 */
      if (rec->scratch_a0 == 0u) {
        if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        rec->scratch_a1 = (uint8_t)(r & 1u);                /* 0x87310..0x87317 */
      }
    }
    rec->timer89 = 0;                                       /* 0x87323 */
    rec->stage92 = (uint8_t)(rec->stage92 + 1u);            /* 0x8732D..0x8732F */
    return FIFA96_OK;
  }
  /* arm == 2 (`0x87364..0x874D9`); every other selector jumped to the epilogue
   * or returned above. */
  if (arm != 2u) return FIFA96_OK;
  rec->timer89 = (int32_t)((uint32_t)rec->timer89 + (uint32_t)rec->delta);
  if (rec->timer89 > rec->scratch_aa && rec->global_10f358 == 0) {
    /* 0x8738E..0x873CA: timer89 = 0, re-arm +0xAA, the 0x87014 setup, then the
     * id from the 0x7D8B0 dwords low words indexed by RNG & 3. */
    static const uint8_t vb[4] = { 0x15u, 0x19u, 0x03u, 0x02u };
    uint16_t r;
    uint8_t slot;
    rec->timer89 = 0;
    if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    rec->scratch_aa = (int32_t)(r & 0xFFu) + 0x78;
    if (fifa96_arm_28_setup(rec) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if (fifa96_arm_anim_select(vb[r & 3u], rec->anim_sel, &slot) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    rec->anim_sel = slot;
  }
  if (rec->global_10f35c != 0) {
    /* 0x873D8..0x87432: above the +0xAE gate, target = (0xCC0, +0xA6), the
     * constant id 0x15 and the latch advance. */
    if (rec->timer89 > rec->scratch_ae) {
      uint8_t slot;
      rec->target.z = rec->scratch_a6;
      rec->target.x = 0xCC0;
      if (fifa96_arm_anim_select(0x15, rec->anim_sel, &slot) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      rec->anim_sel = slot;
      rec->timer89 = 0;
      rec->stage92 = (uint8_t)(rec->stage92 + 1u);
    }
    return FIFA96_OK;
  }
  /* 0x87433..0x87468: target = the chosen record's position triple
   * ([team+0x831]+0x59) plus the two gate offsets, then the 0x8DCD4 distance
   * word gates >= 0x20. A missing chosen record leaves the target (derived
   * bound, OL-58). */
  if (rec->chosen_ok != 0u) {
    rec->target.x = rec->chosen_pos.x;
    rec->target.y = rec->chosen_pos.y;
    rec->target.z = rec->chosen_pos.z;
  }
  rec->target.x += rec->scratch_a2;
  rec->target.z += rec->scratch_a6;
  if (distance >= 0x20) return FIFA96_OK;
  if (rec->global_10f358 == 0) return FIFA96_OK;
  /* 0x87473..0x874D5: target = pos, the velocity word triple zeroed, then the
   * +0xAE hard-approach gate; above it a new +0xAE and the id from the 0x7D8C0
   * dwords low words indexed by RNG & 3. */
  rec->target = rec->pos;
  rec->vel_x = 0;
  rec->vel_z = 0;
  if (rec->scratch_ae >= rec->timer89) return FIFA96_OK;   /* 0x8749A JGE */
  {
    static const uint8_t vc[4] = { 0x00u, 0x58u, 0x5Bu, 0x6Bu };
    uint16_t r;
    uint8_t slot;
    rec->timer89 = 0;
    if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    rec->scratch_ae = (int32_t)(r & 0x7Fu) + 0x20;
    if (fifa96_rng_step(rec->rng, &r) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if (fifa96_arm_anim_select(vc[r & 3u], rec->anim_sel, &slot) != FIFA96_OK)
      return -FIFA96_ERR_INVALID;
    rec->anim_sel = slot;
  }
  return FIFA96_OK;
}

/* Row 29's `0x874E4` target/velocity sync (the native 3 x MOVSD `+0x59 ->
 * `+0x4D` plus `word [rec+0x71] = 0` and the `+0x75`/`+0x73` writes; the
 * derived vel pair covers +0x71..+0x76). */
static void fifa96_arm_29_sync(struct fifa96_arm_record *rec) {
  rec->target = rec->pos;
  rec->vel_x = 0;
  rec->vel_z = 0;
}

/* Row 29's nearest-search candidate count: the native `CMP ECX,0xB` walk over
 * `[rec+0]`'s 0xB2-stride records (0x8DEEC). */
#define FIFA96_ARM_29_TEAM_RECORDS 11u

fifa96_err_t fifa96_arm_29_step(struct fifa96_arm_record *rec) {
  if (!rec) return -FIFA96_ERR_INVALID;
  /* The phase-5 path dereferences the RNG state and the team candidate array;
   * validate both before the prologue write so no partial state is left on
   * failure (the native has no NULL concept here). */
  if (rec->phase == 5u && (!rec->rng || !rec->team_candidates))
    return -FIFA96_ERR_INVALID;
  /* The install request is per call: the pool drain consumes `install`, so a
   * stale request from a previous call must not survive this one. */
  rec->install = 0;
  rec->timer7b = 2;                          /* 0x874EF */
  if (rec->phase != 5u) {
    /* 0x87502..0x87519: sync, then reset; 0x87524..0x8752B: the +0x9A
     * occupancy pre-check; 0x87531..0x8753C: install code 3. */
    fifa96_arm_29_sync(rec);
    if (fifa96_arm_reset(rec) != FIFA96_OK) return -FIFA96_ERR_INVALID;
    if (rec->skip_9a == 0) rec->install = 3;
    return FIFA96_OK;
  }
  /* 0x87548..0x8755C: timer89 += zero-extended delta. */
  rec->timer89 = (int32_t)((uint32_t)rec->timer89 + (uint32_t)rec->delta);
  if (rec->stage92 < 1u) {
    /* 0x87586..0x8759C: the 0x8DE8C nearest search over [rec+0] (skip index =
     * byte[[0x157A9F]+0x8D]; the ball position at +0x59/+0x61 as the target).
     * 0x875A1..0x875A3: only a nearest == this record continues. */
    int16_t best_distance;
    int best = fifa96_entity_find_nearest(
        rec->team_candidates, FIFA96_ARM_29_TEAM_RECORDS, rec->ball_skip,
        (int16_t)rec->ball_pos.x, (int16_t)rec->ball_pos.z, &best_distance);
    if (best == -FIFA96_ERR_INVALID) return -FIFA96_ERR_INVALID;
    if (best == (int)rec->team_index) {
      /* 0x875A7: [0x157AA3] = [nearest+0], an unmodeled cross-record global
       * (OL-55). 0x875AC..0x875B1: the 0x36200 stub with native EAX = 2
       * (the derived no-op drops the store, OL-51). 0x875B6..0x875CD:
       * sync + [0x10F36C] = rec. */
      (void)fifa96_arm_stub_36200();
      fifa96_arm_29_sync(rec);
      rec->chase = 1;
      {
        /* 0x875D7..0x875EC: one RNG word; AL bit 0 -> id 0x5D / 0x46. */
        uint16_t value;
        uint8_t id;
        uint8_t slot;
        if (fifa96_rng_step(rec->rng, &value) != FIFA96_OK) return -FIFA96_ERR_INVALID;
        id = (value & 1u) ? 0x5Du : 0x46u;
        /* 0x875EF..0x87603: the 0x6E598 selector (ECX/EBX loads dead, OL-52). */
        if (fifa96_arm_anim_select(id, rec->anim_sel, &slot) != FIFA96_OK)
          return -FIFA96_ERR_INVALID;
        rec->anim_sel = slot;
      }
    }
    /* 0x87608..0x8761A: timer89 = 0, stage92 = 1, falling into stage 1. */
    rec->timer89 = 0;
    rec->stage92 = 1;
  } else if (rec->stage92 > 2u) {
    return FIFA96_OK;                        /* 0x87574 latch epilogue */
  }
  if (rec->stage92 == 1u) {
    if (rec->chase != 0) {
      /* 0x87620..0x8766C: the chased record syncs and returns; flag44 != 0
       * re-runs the selector on the stage-0 id (the derived stand-in is the
       * last resolved id; the native stack re-read is undefined in a
       * stage-1-only call). The re-select is a **modeled no-op**: for the
       * native id domain (0x5D/0x46) the derived selector is the identity,
       * and the native selector's record writes stay OL-52; the branch is
       * kept for site fidelity (deleting it changes no observable derived
       * state, pinned by the rng-even reselect fixture). */
      fifa96_arm_29_sync(rec);
      if (rec->flag44 != 0) {
        uint8_t slot;
        if (fifa96_arm_anim_select(rec->anim_sel, rec->anim_sel, &slot) != FIFA96_OK)
          return -FIFA96_ERR_INVALID;
        rec->anim_sel = slot;
      }
      return FIFA96_OK;
    }
    {
      /* 0x87676..0x87694: ((P[+0xE] << 4) - P[+0xE]) << 3 >> 4, signed compare
       * (`JG`): a gate above timer89 waits at the target sync (0x87714). */
      uint32_t gate = (uint32_t)(int32_t)rec->player_e;
      gate = (gate << 4) - gate;
      gate <<= 3;
      if (((int32_t)gate >> 4) > rec->timer89) {
        fifa96_arm_29_sync(rec);
        return FIFA96_OK;
      }
    }
    if (rec->active != 0) {
      /* 0x876C0..0x876D0: the 0x6E1D0 phase-cell wrapper. It reads the
       * [rec+8] descriptor pair at +0/+1, or +2/+3 when
       * `[rec+0x826] == [0x157AAC]>>24` (the derived `side_controlled` ruled
       * as the byte compare, FU-142 Appendix B.3), and negates both axes for
       * side != 0. */
      uint32_t which = (rec->side == rec->side_controlled) ? 1u : 0u;
      fifa96_action_vec3 cell;
      if (fifa96_action_phase_cell(rec->cell[which][0], rec->cell[which][1],
                                   rec->side, &cell) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      rec->target.x = cell.x;
      rec->target.y = cell.y;
      rec->target.z = cell.z;
    } else {
      fifa96_arm_29_sync(rec);               /* 0x876A3..0x876BE */
    }
    {
      /* 0x876D5..0x876EC: the 0x8DCD4 out triple; the distance word
       * (+0x65) gates `> 0x20` and the dz word is the record lane (+0x69). */
      int32_t distance;
      int32_t lane;
      if (fifa96_arm_dist_stage(&rec->pos, &rec->target, &distance, &lane) != FIFA96_OK)
        return -FIFA96_ERR_INVALID;
      rec->lane = lane;
      if (distance > 0x20) return FIFA96_OK;
    }
    if (rec->chase != 0) return FIFA96_OK;   /* 0x876EE..0x876FA */
    rec->timer89 = 0;                        /* 0x87702 */
    rec->stage92 = 2;                        /* 0x8770C..0x8770E */
    return FIFA96_OK;
  }
  /* 0x87714..0x8772B: stage 2 syncs target=pos and the velocity pair. */
  fifa96_arm_29_sync(rec);
  return FIFA96_OK;
}
