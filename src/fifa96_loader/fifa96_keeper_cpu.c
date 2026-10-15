/* src/fifa96_loader/fifa96_keeper_cpu.c — FU282: the R11-L4 record-0 machine
 * `FUN_000782D0` skeleton. Evidence: docs/ghidra/FU282_r11l4_record0_machine.md. */
#include "fifa96_loader/fifa96_keeper_cpu.h"

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
  cpu->ladder = 0;
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
      if (cpu->ladder) cpu->ladder(cpu->ladder_ctx); /* 0x78599 (the stub seam) */
    }
  }
  cpu->interlock_157c5d = 0;                       /* 0x785A2 — always clears */
  return FIFA96_OK;
}
