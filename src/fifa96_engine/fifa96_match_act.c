/* src/fifa96_engine/fifa96_match_act.c — M2 phase-15 T1 / FU-175: the native
 * act/phase dispatch seam (`FUN_000888FC` + the `0x8B669..0x8B672` clock-tail
 * tick) and the phase-0x1F / act-9 handler body.
 *
 * Every address/byte cited here is a fresh first-hand read of `/FIFA96.EXE`
 * (Ghidra MCP, case-sensitive program argument) recorded in
 * docs/ghidra/FU175_act_phase_seam.md (R23) and
 * docs/ghidra/FU181_row_17_phase_arms.md (R29), and re-verified this slice:
 *  - `FUN_000888FC` decompile (the latch + `0x1107EC` lookup + invoke);
 *  - `read_memory 0x1107EC` (52 B): act 1..0xC = the phase-table tail
 *    `0x110794 + 0x16*4`; act 0 NULL;
 *  - `disassemble_bytes 0x8B655` (the `0x8B669/0x8B670/0x8B672` tick). */
#include <string.h>

#include "fifa96_engine/fifa96_match_act.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_loader/fifa96_action_handlers.h"

int fifa96_match_act_init(struct fifa96_match_act *act) {
  if (!act) return -FIFA96_ERR_INVALID;
  memset(act, 0, sizeof *act);
  return FIFA96_OK;
}

int fifa96_match_act_installed(const struct fifa96_match_act *act) {
  return (act != NULL && act->id != 0) ? 1 : 0;
}

/* The installed handler resolves iff the act's phase row carries a body: the
 * native `dword[0x158808] = 0x1107EC[act]` combined with the `0x8B669
 * CMP dword[0x158808], 0` guard; `0x1107EC[act]` == `0x110794[0x16 + act]`
 * (the module header). act 0 selects the phase-0x16 INT3/NULL slot. */
static int match_act_in_range(int16_t act) {
  return act >= 0 && act <= 0x0C;
}

int fifa96_match_act_dispatch(struct fifa96_match_run *mr, int16_t act, uint8_t stage,
                              int invoke) {
  struct fifa96_match_act *a;
  uint8_t phase;
  if (!mr) return -FIFA96_ERR_INVALID;
  if (!match_act_in_range(act)) return -FIFA96_ERR_INVALID;   /* FU175-L6 hardening */
  a = &mr->act;
  if (a->id == act) return FIFA96_OK;               /* 0x88906: the latch no-op */
  a->id = act;                                      /* 0x8890A */
  a->timer = 0;                                     /* 0x88920 */
  a->stage = stage;                                 /* 0x88918 */
  if (!invoke) return FIFA96_OK;                    /* BX == 0: no invoke */
  if (fifa96_action_phase_act((uint8_t)act, &phase) != FIFA96_OK) return FIFA96_OK;
  if (fifa96_match_phase_table[phase].fn == NULL) return FIFA96_OK;  /* handler == 0 */
  return fifa96_match_dispatch_phase(mr, phase);    /* (*[0x158808])() */
}

int fifa96_match_act_tick(struct fifa96_match_run *mr) {
  struct fifa96_match_act *a;
  uint8_t phase;
  if (!mr) return -FIFA96_ERR_INVALID;
  a = &mr->act;
  if (a->id == 0) return FIFA96_OK;                 /* 0x8B669 CMP / 0x8B670 JZ */
  if (!match_act_in_range(a->id)) return FIFA96_OK;
  if (fifa96_action_phase_act((uint8_t)a->id, &phase) != FIFA96_OK) return FIFA96_OK;
  if (fifa96_match_phase_table[phase].fn == NULL) return FIFA96_OK;
  return fifa96_match_dispatch_phase(mr, phase);    /* 0x8B672 CALL dword[0x158808] */
}

int fifa96_match_action_phase_1F(struct fifa96_match_run *mr) {
  struct fifa96_match_act *a;
  if (!mr) return -FIFA96_ERR_INVALID;
  a = &mr->act;
  /* Head (0x88F51): word[0x158818] += word[0x157A64]. */
  a->timer = (int16_t)(a->timer + (int16_t)mr->state.frame_delta);
  if (a->stage == 0u) {
    /* Stage 0 (0x88F81): clear the [0x15882A] gate (0x88F9A) then advance to
     * the stage-3 wait. The native stage 0 also resets the camera, calls
     * FUN_00073E28/FUN_00073E08 and writes phase 0x15 on the controlled side;
     * the derived sit-0xC arm performs the phase-0x15 write (FU-170 §7) and
     * the presentation sinks stay FU181-L5. Stages 1/2 (0x88FED/0x89064) are
     * compressed to pass (FU170-L1). */
    mr->global_5882a = 0u;                           /* 0x88F9A */
    a->stage = 3u;
    return FIFA96_OK;
  }
  /* Stage 3 (0x890B1): the native gate is word[0x15881A] >= 0x3C; the derived
   * model gates on the act timer (FU175-L5). On pass, [0x15882A] = 1 (0x890C4)
   * and the act slot is cleared (0x890D3..0x890DE). */
  if (a->timer >= 0x3C) {
    mr->global_5882a = 1u;
    mr->act9_gate = 1u;                              /* the derived gate marker */
    a->id = 0;
    a->stage = 0;
    a->timer = 0;
  }
  return FIFA96_OK;
}
