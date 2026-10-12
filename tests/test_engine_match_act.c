/* tests/test_engine_match_act.c — M2 phase-15 T1 / FU-175: the native
 * act/phase dispatch seam (`FUN_000888FC` + the `0x8B669..0x8B672` clock-tail
 * tick, FU-175 §1/§2) and the phase-0x1F / act-9 handler body it dispatches.
 *
 * First-hand /FIFA96.EXE (this session):
 *   - `FUN_000888FC` (decompile): `if (param_1 != byte[0x158828]) { byte[0x158828]
 *     = param_1; dword[0x158808] = 0x1107EC[param_1]; word[0x158818] = 0;
 *     byte[0x158829] = param_2; if (BX != 0 && handler != NULL) handler(); }`
 *     (AX = act id, DL = stage, BX = invoke-now);
 *   - the `0x8B669 CMP dword[0x158808],0 / 0x8B670 JZ / 0x8B672 CALL
 *     dword[0x158808]` tick;
 *   - act `a` selects phase `0x16 + a` (the act base `0x1107EC` = the phase
 *     table `0x110794 + 0x16*4`; phase 0x1F = act 9 = `0x88F4C`);
 *   - the phase-0x1F/act-9 body (`0x88F4C`): head `0x88F51 word[0x158818] +=
 *     word[0x157A64]`; stage 0 `0x88F81` clears `[0x15882A]` (`0x88F9A`) and
 *     writes phase 0x15 on the controlled side; stage 3 `0x890B1` gates on
 *     `word[0x15881A] >= 0x3C` (alias trap: `[0x158816]>>16`), then sets
 *     `[0x15882A]=1` (`0x890C4`) and clears the act slot
 *     (`0x890D3..0x890DE`).
 *
 * The derived engine keeps the FU-170 §7 model: the sit-0xC arm performs the
 * stage-0 phase-0x15 write and installs act 9 with invoke=0 (the native BX=1
 * invoke-now is deferred to the clock-tail tick, preserving the granted-frame
 * cadence, FU170-L1); stages 1/2 (`0x88FED`/`0x89064`, the leg/`0x74110`/
 * `0x93850`/`0x361B0` gates) are compressed to pass; the stage-3 gate reads the
 * derived act timer (the native `word[0x15881A]` producer is FU165-L2/FU175-L5). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_act.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"

static void test_init_defaults(void) {
  struct fifa96_match_act a;
  memset(&a, 0xAA, sizeof a);
  assert(fifa96_match_act_init(&a) == FIFA96_OK);
  assert(a.id == 0 && a.stage == 0 && a.timer == 0);
  assert(fifa96_match_act_installed(&a) == 0);
  assert(fifa96_match_act_init(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_act_installed(NULL) == 0);
}

/* dispatch installs id/stage/timer; a same-id re-dispatch is the native latch
 * no-op; an id with no ported phase row installs but invokes nothing; an
 * out-of-range act is rejected (the native indexes past the table; the port
 * hardens, FU175-L6). */
static void test_dispatch_installs_and_latches(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  struct fifa96_match_act *a = &mr.act;
  a->id = 7;
  a->stage = 5;
  a->timer = 1234;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 0) == FIFA96_OK); /* BX=0: install only */
  assert(a->id == 9 && a->stage == 0 && a->timer == 0);
  assert(mr.state.phase == 0u);              /* no invoke -> no handler ran */
  /* latch (0x88906): a re-dispatch of the current id is a complete no-op */
  a->timer = 77;
  assert(fifa96_match_act_dispatch(&mr, 9, 2, 1) == FIFA96_OK);
  assert(a->id == 9 && a->stage == 0 && a->timer == 77);
  assert(mr.state.phase == 0u);              /* the invoke was skipped by the latch */
  /* a different id re-installs */
  assert(fifa96_match_act_dispatch(&mr, 2, 1, 0) == FIFA96_OK);
  assert(a->id == 2 && a->stage == 1 && a->timer == 0);
  assert(fifa96_match_act_dispatch(&mr, 0x0D, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_act_dispatch(&mr, -1, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_act_dispatch(NULL, 1, 0, 0) == -FIFA96_ERR_INVALID);
}

/* invoke=1 runs the installed handler through `fifa96_match_dispatch_phase`
 * (the live caller). Act 9 -> phase 0x1F -> the derived body runs its stage-0
 * clear and advances to the stage-3 wait; an act with no ported phase row
 * (act 1 -> phase 0x17, NULL) invokes nothing. The derived sit-0xC arm keeps
 * the stage-0 phase-0x15 write (FU-170 §7), so the handler body itself does
 * not write the phase. */
static void test_dispatch_invoke_runs_handler(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.global_5882a = 1u;
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 1) == FIFA96_OK);
  assert(mr.global_5882a == 0u);             /* 0x88F9A stage-0 clear */
  assert(mr.act.stage == 3u);                /* stages 1/2 compressed */
  assert(mr.act.id == 9 && mr.act.timer == 2);
  /* act 1 resolves to phase 0x17, which is not a registered phase row */
  fifa96_match_run_init(&mr);
  assert(fifa96_match_act_dispatch(&mr, 1, 0, 1) == FIFA96_OK);
  assert(mr.act.id == 1);
  assert(mr.state.phase == 0u);
}

/* tick is a no-op while no act is installed or the installed act's phase row is
 * unported; act 9 accumulates `frame_delta` and at >= 0x3C sets the gate and
 * clears the slot (0x890B1..0x890DE). */
static void test_tick_timer_and_completion(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);     /* id 0 -> no-op */
  assert(mr.act.id == 0 && mr.state.phase == 0u);
  /* act 1 -> phase 0x17 unported: installs, but the tick does nothing */
  assert(fifa96_match_act_dispatch(&mr, 1, 0, 0) == FIFA96_OK);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.timer == 0);
  /* act 9 -> phase 0x1F: 30 ticks of delta 2 reach 0x3C */
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 0) == FIFA96_OK);
  assert(mr.act.stage == 0u);
  for (int i = 0; i < 29; i++) {
    assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
    assert(mr.act.id == 9);
  }
  assert(mr.act.timer == 58);                 /* 29 * 2, still short of 0x3C */
  assert(mr.act.stage == 3u);                 /* stage 0 ran on the first tick */
  assert(mr.global_5882a == 0u && mr.act9_gate == 0u);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* 60 */
  assert(mr.global_5882a == 1u);              /* 0x890C4 */
  assert(mr.act9_gate == 1u);                 /* the derived marker */
  assert(mr.act.id == 0 && mr.act.timer == 0 && mr.act.stage == 0);
  assert(fifa96_match_act_tick(NULL) == -FIFA96_ERR_INVALID);
}

/* FU-183 §5/§6 rank 2 (phase-16 T1): phase 0x18 is the wired act-2 row. The
 * body is the derived free-kick/penalty hand-off; the act machine owns it (the
 * sit-9/sit-0xA producer dispatches act 2 invoke-now, the clock tail ticks it).
 * Mutation-RED: a NULL phase-0x18 slot leaves the phase unwritten and the act
 * slot installed, so every assertion below fails. */
static void test_phase_18_registered(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x18].fn == fifa96_match_action_phase_18);
  assert(fifa96_match_dispatch_phase(&mr, 0x18) == FIFA96_OK);
  assert(mr.state.phase == 0x0Au);      /* the stage-0 hand-off write */
  assert(mr.ref_restart_stage == 1u);
  assert(fifa96_match_dispatch_phase(NULL, 0x18) == -FIFA96_ERR_INVALID);
}

/* The act-2 invoke-now runs the phase-0x18 stage-0 hand-off on the dispatch
 * call (the native FUN_000888FC(2, stage, BX=1)); the act machine then ticks
 * the row and its terminal clears the act slot. */
static void test_phase_18_act_owns_handoff(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 2, 0, 1) == FIFA96_OK);
  assert(mr.act.id == 2 && mr.act.timer == 2);   /* the head timer add */
  assert(mr.state.phase == 0x0Au);               /* the stage-0 phase write */
  assert(mr.ref_restart_stage == 1u);
  assert(mr.ref_whistle == 0x1Eu);               /* the soft-foul whistle */
  /* the tick advances the hand-off (the act machine owns it); the decision
   * terminal clears the act slot */
  mr.session_gate_14c32a = 1;                    /* the decision gate */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.state.phase == 7u);                  /* the FK default decision */
  assert(mr.act.id == 0 && mr.act.stage == 0 && mr.act.timer == 0);
}

/* The genuinely-live producer (the census claim): situation 9 (the referee
 * restart path) reaches the phase-0x18 row through the shared set-piece entry
 * and the act machine. Mutation-RED: a NULL row leaves the phase at 2. */
static void test_phase_18_producer_live_via_sit9(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;                      /* force the direct path */
  assert(fifa96_match_run_set_piece(&mr, 9, 0, 0) == 0);
  assert(mr.act.id == 2);                        /* the act machine installed act 2 */
  assert(mr.ref_restart_stage == 1u);            /* stage 0 ran (invoke-now) */
  assert(mr.state.phase == 0x0Au);               /* the phase-0x18 body wrote it */
}

/* FU-187 §2/§5 rank 7 (phase-16 T4): phase 0x19 is the wired act-3 foul row.
 * The body is the derived 7-stage foul sequence; the act machine owns it (the
 * row-0x0C contact dispatches act 3 invoke-now, the clock tail ticks it).
 * Mutation-RED: a NULL phase-0x19 slot leaves the sequence stage unwritten and
 * the dispatch returns UNSUPPORTED, so every assertion below fails. */
static void test_phase_19_registered(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x19].fn == fifa96_match_action_phase_19);
  assert(fifa96_match_dispatch_phase(&mr, 0x19) == FIFA96_OK);
  assert(mr.referee.stage == 1u);        /* stage 0 ran: install 0x16 + phase 0xF */
  assert(mr.act.stage == 1u);
  assert(mr.state.phase == 0x0Fu);
  assert(fifa96_match_dispatch_phase(NULL, 0x19) == -FIFA96_ERR_INVALID);
}

/* The act-3 invoke-now runs the phase-0x19 stage 0 on the dispatch call (the
 * native FUN_000888FC(3,0,BX=1)); the clock tail advances it. */
static void test_phase_19_act_owns_foul(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 3, 0, 1) == FIFA96_OK);
  assert(mr.act.id == 3 && mr.act.stage == 1u);   /* the head timer add + stage 0 */
  assert(mr.act.timer == 0);                      /* stage 0 resets the timer */
  assert(mr.state.phase == 0x0Fu);                /* the stage-0 phase write */
  /* the tick advances stages 1 (gate) -> 2 (speech) */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.id == 3 && mr.act.stage == 2u);
}

/* FU-188 §2/§6 (phase-16 T4): phase 0x20 is ported but its slot is NULL
 * (uncounted — the sole producer is situation 0, absent from every engine
 * caller). The body is driven directly: stage 0 installs code 0x24 on both
 * teams' 11 records and advances; the NULL slot returns UNSUPPORTED. */
static void test_phase_20_body_direct(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x20].fn == NULL);   /* uncounted */
  assert(fifa96_match_dispatch_phase(&mr, 0x20) == -FIFA96_ERR_UNSUPPORTED);
  mr.act.id = 0x0A;
  mr.act.stage = 0;
  assert(fifa96_match_action_phase_20(&mr) == FIFA96_OK);
  assert(mr.entities.team[0].records[0].code == 0x24u);   /* the 22-record install */
  assert(mr.entities.team[0].records[10].code == 0x24u);
  assert(mr.entities.team[1].records[0].code == 0x24u);
  assert(mr.entities.team[1].records[10].code == 0x24u);
  assert(mr.act.stage == 1u);                             /* 0x8B7BB */
  assert(fifa96_match_action_phase_20(NULL) == -FIFA96_ERR_INVALID);
}

/* Row 0x17 is NOT counted: its table slot is NULL (no engine installer — the
 * rows-27/29/2C precedent; see the handlers.h declaration / FU181-L1), so the
 * dispatcher returns UNSUPPORTED while the ported body stays reachable by
 * direct call (the tests below). */
static void test_row_17_not_counted(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_action_table[0x17].fn == NULL);
  assert(fifa96_match_dispatch_action(&mr, 0x17u) == -FIFA96_ERR_UNSUPPORTED);
}

/* The FU-181 §2 row-0x17 phase gate: a non-0xB phase takes the `0x849A1`
 * FUN_0007DAB4 reset tail; phase 0xB runs the stage walk. The body is called
 * directly (the slot is NULL). */
static void test_row_17_phase_gate_resets(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.record.entity_id = 1;                    /* team 0 record 1 */
  mr.state.phase = 4u;                        /* != 0xB */
  mr.entities.team[0].records[1].code = 7;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.entities.team[0].records[1].code == 0u);    /* reset installed 0 */
  assert(mr.entities.team[0].records[1].timer89 == 0);
}

/* The FU-181 §2 row-0x17 stage walk (phase 0xB): stage 0 offsets the target by
 * -0x680 and the ±0x50 facing from the position-z sign and advances to stage 1;
 * stage 1 waits on the `+0x89` timer to 0x3C then advances through stage 2
 * (position x := -0x860) to stage 3; stage 3 waits and resets. */
static void test_row_17_stage_walk(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.record.entity_id = 1;
  mr.state.phase = 0x0Bu;
  mr.record.delta = 2;
  mr.entities.team[0].records[1].active = 1;
  /* stage 0, pos_z <= 0 -> target z = -0x50 */
  mr.record.stage92 = 0;
  mr.record.pos_z = 0;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.record.target_x == -0x680);
  assert(mr.record.target_y == 0);
  assert(mr.record.target_z == -0x50);
  assert(mr.record.stage92 == 1u);
  assert(mr.record.timer89 == 0);
  /* positive pos_z -> +0x50 */
  mr.record.stage92 = 0;
  mr.record.pos_z = 3;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.record.target_z == 0x50);
  /* stage 1: the timer waits to 0x3C */
  mr.record.stage92 = 1;
  mr.record.timer89 = 0;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.record.stage92 == 1u && mr.record.timer89 == 2);
  mr.record.timer89 = 0x3C - 2;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);  /* stage1->2->3 */
  assert(mr.record.stage92 == 3u);
  assert(mr.record.timer89 == 0);
  assert(mr.record.pos_x == -0x860);          /* the stage-2 +0x59 write */
  assert(mr.record.pos_y == 0 && mr.record.pos_z == 0);
  /* stage 3: wait then reset */
  mr.record.timer89 = 0;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.record.stage92 == 3u && mr.record.timer89 == 2);
  mr.record.timer89 = 0x3C - 2;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.entities.team[0].records[1].code == 0u);   /* 0x849A1 reset */
  /* stage > 3: the 0x8476D JA epilogue, no reset */
  mr.record.stage92 = 4;
  mr.entities.team[0].records[1].code = 9;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.entities.team[0].records[1].code == 9u);
  assert(mr.record.stage92 == 4u);
}

int main(void) {
  test_init_defaults();
  test_dispatch_installs_and_latches();
  test_dispatch_invoke_runs_handler();
  test_tick_timer_and_completion();
  test_phase_18_registered();
  test_phase_18_act_owns_handoff();
  test_phase_18_producer_live_via_sit9();
  test_phase_19_registered();
  test_phase_19_act_owns_foul();
  test_phase_20_body_direct();
  test_row_17_not_counted();
  test_row_17_phase_gate_resets();
  test_row_17_stage_walk();
  puts("test_engine_match_act OK");
  return 0;
}
