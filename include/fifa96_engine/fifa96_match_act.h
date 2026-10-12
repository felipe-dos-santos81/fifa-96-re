/* include/fifa96_engine/fifa96_match_act.h — M2 phase-15 T1 / FU-175: the
 * native act/phase dispatch seam.
 *
 * The native act machine is `FUN_000888FC` (`0x888FC..0x88930`, decompiled
 * first-hand on /FIFA96.EXE this slice, FU-175 §1):
 *
 *   if (param_1 != byte[0x158828]) {            // act id latch
 *     byte[0x158828] = param_1;                 // 0x8890A
 *     dword[0x158808] = 0x1107EC[param_1];      // 0x88929 (the act handler)
 *     word[0x158818]  = 0;                       // 0x88920 (act timer)
 *     byte[0x158829] = param_2;                 // 0x88918 (stage)
 *     if (BX != 0 && handler != NULL) handler(); // invoke-now
 *   }
 *
 * Watcom register contract: **AX = act id**, **DL = stage**, **BX = invoke-now**.
 * The act table base `0x1107EC` is exactly `0x110794 + 0x16*4`, so act `a`
 * selects phase `0x16 + a` and its handler (`fifa96_action_phase_act`). The
 * clock tail (`0x8B669..0x8B672`, FU-175 §2) ticks the installed handler once
 * per granted frame, ungated by phase:
 *
 *   0x8B669 CMP dword [0x158808], 0
 *   0x8B670 JZ  0x8B678
 *   0x8B672 CALL dword [0x158808]
 *
 * This module mirrors that seam over the derived run. `fifa96_match_act_dispatch`
 * gives `fifa96_match_dispatch_phase` its first live caller (the invoke path)
 * and `fifa96_match_act_tick` gives it the per-frame caller. The act domain is
 * `{0..0xC}` (the fresh `0x1107EC` read shows 13 live slots; act 0 is the NULL
 * slot); the native `FUN_000888FC` has no bounds check, so the port hardens the
 * over-index (FU175-L6). */
#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

struct fifa96_match_run;   /* defined in fifa96_match_run.h */

/* The single native slot: id `[0x158828]`, stage `[0x158829]`, timer
 * `[0x158818]`. `id == 0` means no act is installed (the act-0 NULL slot). */
struct fifa96_match_act {
  int16_t id;      /* native byte [0x158828] (the latch key) */
  uint8_t stage;   /* native byte [0x158829] */
  int16_t timer;   /* native word [0x158818] (reset on every new act) */
};

/* Zero the slot (id 0/stage 0/timer 0). NULL -> -FIFA96_ERR_INVALID. */
int fifa96_match_act_init(struct fifa96_match_act *act);

/* `FUN_000888FC` over the run: on the accepted path (a re-dispatch of the
 * current `act` is a complete no-op, the native latch `0x88906`) install
 * `id = act`, `timer = 0`, `stage = stage`, then, when `invoke` is set and the
 * slot's handler resolves (`fifa96_match_phase_table[0x16 + act].fn != NULL`),
 * run it through `fifa96_match_dispatch_phase(mr, 0x16 + act)` — the live
 * caller. `act` outside `0..0xC`, or a NULL `mr`, -> -FIFA96_ERR_INVALID. The
 * handler's own return value propagates when it runs. */
int fifa96_match_act_dispatch(struct fifa96_match_run *mr, int16_t act, uint8_t stage,
                              int invoke);

/* The `0x8B669..0x8B672` clock-tail tick: when an act is installed (`id != 0`)
 * and its phase row is ported, run `fifa96_match_dispatch_phase(mr, 0x16 + id)`
 * once. A no-op when no act is installed, the act is out of range, or the row
 * is unported (the native `[0x158808] == 0` guard). Returns the handler's value
 * or FIFA96_OK; NULL `mr` -> -FIFA96_ERR_INVALID. */
int fifa96_match_act_tick(struct fifa96_match_run *mr);

/* 1 when an act id outside 0 is installed, else 0 (NULL -> 0). */
int fifa96_match_act_installed(const struct fifa96_match_act *act);

/* The phase-0x1F / act-9 handler body (`0x88F4C`, fresh FU-181 §4/FU-175 §7).
 * The native handler is simultaneously the phase-table row `0x110794[0x1F]` and
 * the act slot `0x1107EC[9]` (the FU165-L9 dual identity), so registering it
 * wires the first honest phase row. Derived stages:
 *  - head (`0x88F51`): `timer += frame_delta` (`word[0x158818] += [0x157A64]`);
 *  - stage 0 (`0x88F81`): clear the gate `[0x15882A] = 0` and advance to the
 *    stage-3 wait (stages 1/2 `0x88FED`/`0x89064` compressed to pass, FU170-L1;
 *    the stage-0 phase-0x15 write is performed by the sit-0xC arm, FU-170 §7);
 *  - stage >= 1: when `timer >= 0x3C` set `[0x15882A] = 1` (the derived
 *    `global_5882a` + `act9_gate` markers) and clear the act slot. The native
 *    stage-3 gate reads `word[0x15881A]` (alias trap `[0x158816]>>16`), a
 *    timeline cell whose producer is unported (FU165-L2/FU175-L5), so the
 *    derived model gates on the act timer.
 * Returns FIFA96_OK or -FIFA96_ERR_INVALID (NULL `mr`). */
int fifa96_match_action_phase_1F(struct fifa96_match_run *mr);
