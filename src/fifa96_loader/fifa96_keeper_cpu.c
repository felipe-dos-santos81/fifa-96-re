/* src/fifa96_loader/fifa96_keeper_cpu.c — FU282: the R11-L4 record-0 machine
 * `FUN_000782D0` skeleton; FU283 (Task 8): the `FUN_00077EAC` keeper CPU
 * decision ladder + its `FUN_000765C4` tracked-teammate predicate.
 * Evidence: docs/ghidra/FU282_r11l4_record0_machine.md,
 * docs/ghidra/FU283_r11l4_ladder.md. */
#include "fifa96_loader/fifa96_keeper_cpu.h"

#include <string.h>

void fifa96_keeper_cpu_init(fifa96_keeper_cpu *cpu) {
  if (!cpu) return;
  cpu->head_flag = 0;
  cpu->side_xor = 0;
  cpu->counter_ad0[0] = 0;
  cpu->counter_ad0[1] = 0;
  cpu->counter_acc[0] = 0;
  cpu->counter_acc[1] = 0;
  cpu->force_install = 0;
  cpu->interlock_157c5d = 0;
  cpu->latch_157c5c = 0;
  cpu->ladder = fifa96_keeper_cpu_ladder_run;
  cpu->ladder_ctx = 0;
}

int fifa96_keeper_cpu_step(fifa96_keeper_cpu *cpu,
                           const fifa96_keeper_cpu_record *rec,
                           fifa96_keeper_cpu_out *out) {
  uint8_t target;
  if (!cpu || !rec || !out) return -FIFA96_ERR_INVALID;
  out->install = 0;
  out->install_code = 0;
  out->force_install = 0;
  out->ladder = 0;
  memset(&out->ladder_out, 0, sizeof out->ladder_out);

  /* HEAD (`0x782D8..0x78334`): the one-shot actor counter. */
  if (cpu->head_flag != 0 && rec->is_actor != 0 && rec->phase != 5) {
    uint32_t idx = (uint32_t)((rec->side ^ cpu->side_xor) & 1u);
    cpu->head_flag = 0;                            /* 0x782FB */
    cpu->counter_ad0[idx]++;                       /* 0x78311 */
    cpu->counter_acc[idx ^ 1u]++;                  /* 0x7832E (the complement side) */
  }

  /* The `[0x110680][code] & 1` gate (`0x784DC`, `JZ 0x7859E`) skips BOTH the
   * install tree and the ladder gate — 0x7859E sits after the 0x78599 CALL
   * (fresh bytes `0f84af000000`) — but the interlock clear at 0x785A2 is still
   * reached. */
  if (rec->permit != 0) {
    /* INSTALL TREE (`0x784EF..0x78576`). */
    if (rec->timer81 != 0) target = 0x00u;         /* 0x784FE/0x78503 */
    else if (rec->phase != 2) target = 0x19u;      /* 0x78513 */
    else if (rec->ball_owner == 0) target = 0x19u; /* 0x7851E */
    else if (rec->owner_rel_5 != 0) target = 0x19u;/* 0x7853C */
    else target = rec->code == 5u ? 5u : 0x04u;    /* 0x7854A skip / 0x7854C */
    if ((int8_t)target != (int8_t)rec->code) {     /* the same-code guard 0x78560 */
      cpu->force_install = 1;                      /* 0x7856C, [0x157AB2] */
      out->install = 1;                            /* FUN_0007D9A4(rec,target,0,0) */
      out->install_code = target;
      out->force_install = 1;
    }

    /* LADDER GATE (`0x7857B..0x78597`). */
    if (rec->phase == 2 && rec->queued20 == 0 && cpu->interlock_157c5d != 0) {
      out->ladder = 1;
      if (cpu->ladder) cpu->ladder(cpu->ladder_ctx, rec, &out->ladder_out);
    }
  }
  cpu->interlock_157c5d = 0;                       /* 0x785A2 — always clears */
  return FIFA96_OK;
}

int fifa96_keeper_cpu_tracked_predicate(uint8_t tracked_present,
                                        uint8_t tracked_same_side,
                                        uint8_t tracked_code,
                                        uint8_t global_158740) {
  /* First-hand /FIFA96.EXE 0x765C4..0x76615 (34 insns): the `[0x1577CA]`
   * present + same-side (`+0x826`) guard, then the tracked row-5 short-circuit
   * (0x765EC) before the `[0x158740]>>0x18 != 4` disjunct (0x76602). */
  if (tracked_present && tracked_same_side) {
    if (tracked_code == 5u) return 1;
    if (global_158740 != 4u) return 1;
  }
  return 0;
}

int fifa96_keeper_cpu_ladder_run(void *ctx, const fifa96_keeper_cpu_record *rec,
                                 fifa96_keeper_cpu_ladder_out *out) {
  (void)ctx;
  if (!rec || !out) return -FIFA96_ERR_INVALID;
  out->ran = 0;
  out->copy_pos = 0;
  out->install = 0;
  out->install_code = 0;
  out->invoke_now = 0;
  out->stamp_157c2c = 0;

  /* Head gates (`0x77EB7..0x77F29`): each closed gate jumps to the shared tail
   * `0x782C4` with no install. */
  if (rec->code != 0x19u) return FIFA96_OK;      /* byte +0x91 == 0x19 (0x77EC2) */
  /* FU291 (phase-27 T2): the `[0x14C1D4]` bit-9 gate, computed live:
   * `FUN_000741B4` fold (0x77ED9) picks the word, bit 1 of its high byte
   * (`0x77EE8 AND AH,2`) is the gate. */
  {
    uint32_t idx = (uint32_t)((rec->side ^ rec->side_swap) & 1u);
    if (((rec->input_range[idx] >> 9) & 1u) != 0u) return FIFA96_OK; /* 0x77EF0 */
  }
  if (rec->gate_9a != 0) return FIFA96_OK;       /* the +0x9A skip (0x77EFD) */
  if (rec->is_T != 0) return FIFA96_OK;          /* *(param_1[10]) == 'T' (0x77F10) */
  if (rec->timer81 != 0) {                       /* the +0x81 timer (0x77F16) */
    out->copy_pos = 1;                           /* pos +0x59 -> +0x4D (0x77F20) */
    return FIFA96_OK;
  }
  /* Actor/side skip (`0x77F2E..0x77F51`). */
  if (rec->is_actor != 0) return FIFA96_OK;      /* rec == [0x157A83] (0x77F36) */
  if (rec->actor_present != 0 && rec->actor_same_side != 0) return FIFA96_OK;

  out->ran = 1;
  if (rec->stage == 0u) return FIFA96_OK;        /* TEST CX,CX; JZ 0x782C4 (0x781E1) */

  /* `FUN_000765C4` (0x781EC): nonzero installs code 4 via the shared `0x782BB`
   * tail (`0x781F6 MOV ECX,1; 0x781FB MOV EDX,4; 0x78200 JMP 0x782BB`). */
  if (fifa96_keeper_cpu_tracked_predicate(rec->tracked_present, rec->tracked_same_side,
                                          rec->tracked_code, rec->global_158740) != 0) {
    out->install = 1;
    out->install_code = 0x04u;
    out->invoke_now = 1;
    return FIFA96_OK;
  }

  /* The five install arms through `0x77E98`, index `CX-1 ∈ 0..4` (0x78205/0x78215).
   * Each sets ECX=1 and EBX=0: `FUN_0007D9A4(rec, code, 0, 1)` invoke-now. */
  switch (rec->stage) {
    case 1u:                                     /* arm 0x7826D */
      out->stamp_157c2c = 0x2Cu;                 /* 0x7827E */
      out->install_code = 0x1Au;                 /* 0x78277 */
      break;
    case 2u:                                     /* arm 0x78245 */
      out->stamp_157c2c = 0x2Du;                 /* 0x78258 */
      out->install_code = 0x1Au;                 /* 0x7824F */
      break;
    case 3u:                                     /* arm 0x7821D */
      out->stamp_157c2c = 0x2Eu;                 /* 0x78230 */
      out->install_code = 0x1Au;                 /* 0x78227 */
      break;
    case 4u:                                     /* arm 0x78294 */
      out->install_code = 0x1Bu;                 /* 0x78299 */
      break;
    case 5u:                                     /* arm 0x782B1 (falls to 0x782BB) */
      out->install_code = 0x1Cu;                 /* 0x782B6 */
      break;
    default:                                     /* CX-1 > 4 -> 0x782C4 */
      return FIFA96_OK;
  }
  out->install = 1;
  out->invoke_now = 1;
  return FIFA96_OK;
}
