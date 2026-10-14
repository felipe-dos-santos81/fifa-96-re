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
 * clear and advances to the stage-3 wait; act 1 -> phase 0x17 (registered by
 * M2 phase-18 T4) runs its stage-0 kickoff entry (the phase-1 write). The
 * derived sit-0xC arm keeps the stage-0 phase-0x15 write (FU-170 §7), so the
 * handler body itself does not write the phase. */
static void test_dispatch_invoke_runs_handler(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.global_5882a = 1u;
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 1) == FIFA96_OK);
  assert(mr.global_5882a == 0u);             /* 0x88F9A stage-0 clear */
  assert(mr.act.stage == 3u);                /* stages 1/2 compressed */
  assert(mr.act.id == 9 && mr.act.timer == 2);
  /* act 1 resolves to phase 0x17, registered by T4 (FU-203): the invoke runs
   * the phase-0x17 stage-0 kickoff entry (clear the gate + the phase-1 write) */
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 1, 0, 1) == FIFA96_OK);
  assert(mr.act.id == 1);
  assert(mr.act.stage == 1u);                /* stage 0 -> 1 (0x88EB1) */
  assert(mr.state.phase == 1u);              /* the kickoff phase-1 entry */
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
  /* act 1 -> phase 0x17 (registered by T4): installs, and the tick runs the
   * stage-0 kickoff entry (the phase-1 write) then advances to the stage-1 wait */
  assert(fifa96_match_act_dispatch(&mr, 1, 0, 0) == FIFA96_OK);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.id == 1 && mr.act.stage == 1u);
  assert(mr.state.phase == 1u);
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

/* FU-188 §2/§6 (phase-16 T4) + FU-202 §6 rec (b) (M2 phase-18 T3): phase 0x20 is
 * ported and its slot is now WIRED (counted) — the sole producer situation 0 is
 * live via the re-placed match-init producer. The body is also driven directly:
 * stage 0 installs code 0x24 on both teams' 11 records and advances. */
static void test_phase_20_body_direct(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x20].fn != NULL);   /* counted (T3) */
  mr.act.id = 0x0A;
  mr.act.stage = 0;
  assert(fifa96_match_dispatch_phase(&mr, 0x20) == FIFA96_OK);
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

/* ===== FU-193 (M2 phase-17 T4) + FU-203 (M2 phase-18 T4): the act-tail
 * cohort 0x17/0x1A/0x1C =====================================================
 *
 * The three native act-tail handlers `0x88DC8` (phase 0x17 / act 1),
 * `0x89620` (phase 0x1A / act 4) and `0x89110` (phase 0x1C / act 6) are ported
 * as bounded derived bodies. Per Ruling R8 the rows initially landed as bodies
 * + numbered legs; M2 phase-18 T4 (R51/FU-203) then landed phase 0x17 COUNTED:
 * its producer is the live phase-0x20 terminal's situation-1 re-dispatch (act 1
 * -> phase 0x17), and the derived ad-hoc kickoff call is retired (re-expressed
 * through the act-1 route). Phase 0x1A (act-4 formation arm, FU-201/R49) and
 * phase 0x1C (act-6 = the offside machine, a double-model, FU193-L4) stay
 * NULL/uncounted. */

/* Phase 0x17 / act 1 (FU-193 §2 + FU-203): the slot is REGISTERED and counted
 * (M2 phase-18 T4). Mutation-RED: a NULL slot returns UNSUPPORTED and leaves
 * the phase unwritten. */
static void test_phase_17_counted(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x17].fn == fifa96_match_action_phase_17);
  assert(fifa96_match_dispatch_phase(&mr, 0x17u) == FIFA96_OK);
  assert(mr.state.phase == 1u);       /* the stage-0 kickoff phase-1 entry */
}

/* Phase 0x17 stage 0 (0x88E4B): clear the [0x15882A] kickoff gate (0x88E56),
 * run the derived kickoff entry (the FUN_000740A0(1) phase-1 write), timer = 0
 * and stage -> 1 (0x88EAA/0x88EB1). */
static void test_phase_17_kickoff_stage0(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 1;
  mr.act.stage = 0;
  mr.global_5882a = 1u;
  assert(fifa96_match_action_phase_17(&mr) == FIFA96_OK);
  assert(mr.global_5882a == 0u);      /* 0x88E56 */
  assert(mr.state.phase == 1u);       /* the kickoff phase-1 entry */
  assert(mr.act.stage == 1u);         /* 0x88EB1 */
  assert(mr.act.timer == 0);          /* 0x88EAA */
  assert(fifa96_match_action_phase_17(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x17 stage 1 (0x88EBC): the formation hand-off to act 4 (0x88EE8,
 * invoke-now) when the goal-side flag [0x157A4C] != 1 and a residual formation
 * mismatch exists. Mutation-RED: without the fall-through the act slot stays 1. */
static void test_phase_17_stage1_handoff(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 1;
  mr.act.stage = 1;
  mr.global_157a4c = 0u;               /* != 1 -> hand-off permitted */
  mr.formation_mismatch[0] = 1u;
  assert(fifa96_match_action_phase_17(&mr) == FIFA96_OK);
  assert(mr.act.id == 4);              /* the 0x88EE8 hand-off installed act 4 */
}

/* Phase 0x17 stage 1 (0x88EF3): the shared timeline gate word[0x15881A] >= 0x78
 * (the derived `state.tick_total`) sets [0x15882A] = 1 and clears the act slot;
 * below the gate the head add stands and the slot is untouched. */
static void test_phase_17_stage1_timer(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 1;
  mr.act.stage = 1;
  mr.act.timer = 10;
  mr.global_157a4c = 1u;               /* the goal-side flag suppresses the hand-off */
  mr.state.tick_total = 0x40u;         /* < 0x78 */
  assert(fifa96_match_action_phase_17(&mr) == FIFA96_OK);
  assert(mr.act.timer == 12);          /* head add stands (no reset on the JL path) */
  assert(mr.global_5882a == 0u && mr.act.id == 1u);
  mr.state.tick_total = 0x78u;
  assert(fifa96_match_action_phase_17(&mr) == FIFA96_OK);
  assert(mr.global_5882a == 1u);       /* 0x88F07 */
  assert(mr.act.id == 0u && mr.act.stage == 0u && mr.act.timer == 0);
}

/* Phase 0x1A / act 4 (FU-193 §3 / FU-197 §6): the formation-order walk. Its slot
 * is NULL (uncounted — the 0x14C360/0x157BD2 matrices have no engine producer,
 * FU197-L4, so the T2 arm is dormant; the phase-0x1A row would double-run it). */
static void test_phase_1A_not_counted(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1A].fn == NULL);
  assert(fifa96_match_dispatch_phase(&mr, 0x1Au) == -FIFA96_ERR_UNSUPPORTED);
}

/* Phase 0x1A stage 0, dormant: with the desired-order matrix unported
 * (`formation_order_ok == 0`) the head add runs but the walk installs nothing
 * (the honest default; a fake install would be forbidden). */
static void test_phase_1A_dormant(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 4;
  mr.act.stage = 0;
  mr.formation_order_ok = 0u;
  assert(fifa96_match_action_phase_1A(&mr) == FIFA96_OK);
  assert(mr.act.timer == 2);           /* 0x89629 head add */
  assert(mr.act.stage == 0u);          /* dormant: no walk */
  assert(mr.state.phase == 0u);        /* no phase-0xB write */
  assert(fifa96_match_action_phase_1A(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x1A stage 0 install (0x896E2): the first mismatching active record
 * takes the phase-0xB write (0x8972F) and the action-row-0x17 install (0x89740),
 * timer = 0 and stage -> 1. Mutation-RED: skipping the install leaves the phase
 * at 0 and the record code 0. */
static void test_phase_1A_install(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 4;
  mr.act.stage = 0;
  mr.formation_order_ok = 1u;
  mr.formation_current[0][2] = 5;      /* current != desired at record 2 */
  mr.formation_order[0][2] = 7;
  mr.formation_mismatch[0] = 1u;       /* side select s = (mismatch[0]==0)?1:0 = 0 */
  assert(fifa96_match_action_phase_1A(&mr) == FIFA96_OK);
  assert(mr.formation_mismatch[0] == 0u);              /* 0x8966A decrement */
  assert(mr.state.phase == 0x0Bu);                     /* 0x8972F */
  assert(mr.entities.team[0].records[2].code == 0x17u);/* 0x89740 */
  assert(mr.act.stage == 1u && mr.act.timer == 0);     /* 0x89747/0x8974E */
}

/* Phase 0x1A stage 1 terminal (0x897AE): clear the act slot and re-dispatch the
 * stored situation/side (the 0x8AB45/0x8AB50 cells) with BX=1. FU-241 (M2
 * phase-23 T4) de-compresses the BX=1 fallback, so the act slot is now act 8 and
 * the stored-situation re-dispatch is deferred to the act-8 sweep terminal.
 * Mutation-RED: a missing re-dispatch leaves the stored situation uncleared. */
static void test_phase_1A_end_block(void) {
  struct fifa96_match_run mr;
  int k;
  fifa96_match_run_init(&mr);
  mr.act.id = 4;
  mr.act.stage = 1;
  mr.state.tick_total = 0x100u;        /* > 0xF0 (the word[0x15881A] gate) */
  mr.store_15881e = 9u;                /* the stored situation */
  mr.store_158820 = 0u;                /* the stored side */
  assert(fifa96_match_action_phase_1A(&mr) == FIFA96_OK);
  assert(mr.act.id == 8u);             /* FU-241: the BX=1 fallback dispatches act 8 now */
  assert(mr.state.phase == 0u);        /* the phase-0 write; the sit-9 arm is deferred */
  /* drive the sweep to its stage-2 terminal: the deferred sit-9 -> act-2 arm lands */
  for (k = 0; k < 21; k++)
    assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* terminal */
  assert(mr.act.id == 2u);             /* the deferred sit-9 re-dispatch installed act 2 */
  assert(mr.state.phase == 0x0Au);     /* the act-2 phase-0x18 stage 0 wrote it */
}

/* Phase 0x1C / act 6 (FU-193 §4): the pre-restart timeline. Its slot is NULL
 * (uncounted — no act-6 producer; the offside machine is the existing driver,
 * a double-model, FU193-L4). The body wraps the already-ported
 * `fifa96_ref_offside_sequence_step` (the native 0x89110). */
static void test_phase_1C_not_counted(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1C].fn == NULL);
  assert(fifa96_match_dispatch_phase(&mr, 0x1Cu) == -FIFA96_ERR_UNSUPPORTED);
}

/* Phase 0x1C stage 0 (0x89142): write phase 0xA (0x89156), whistle (0x974DC) and
 * stage -> 1 (0x89185). Mutation-RED: without the alias the stage never advances
 * and the phase stays unwritten. */
static void test_phase_1C_stage0(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 6;
  mr.act.stage = 0;
  assert(fifa96_match_action_phase_1C(&mr) == FIFA96_OK);
  assert(mr.state.phase == 0x0Au);     /* 0x89156 */
  assert(mr.act.stage == 1u);          /* 0x89185 */
  assert(mr.act.timer == 0);
  assert(mr.ref_whistle == 0x1Eu);     /* the 0x974DC(0x1E) whistle */
  assert(fifa96_match_action_phase_1C(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x1C stage 2 terminal (0x891C0): clear the act slot and re-dispatch
 * situation 9 on the opponent (0x8920A). Mutation-RED: a missing terminal leaves
 * the act slot installed and the phase unwritten. */
static void test_phase_1C_terminal(void) {
  struct fifa96_match_run mr;
  int k;
  fifa96_match_run_init(&mr);
  mr.act.id = 6;
  mr.act.stage = 2;
  assert(fifa96_match_action_phase_1C(&mr) == FIFA96_OK);
  /* FU-241 (phase-23 T4): the BX=1 sit-9 re-dispatch is de-compressed — act 8 is
   * armed and the sit-9 -> act-2 arm is deferred to the sweep terminal. */
  assert(mr.act.id == 8u);
  assert(mr.state.phase == 0u);
  for (k = 0; k < 22; k++)
    assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(mr.act.id == 2u);             /* the deferred sit-9 re-dispatch installed act 2 */
  assert(mr.state.phase == 0x0Au);     /* the act-2 phase-0x18 stage 0 wrote it */
}

/* ===== FU-194 (M2 phase-17 T5): the act-tail cohort 0x1D/0x1E/0x20 ========
 *
 * The two native act-tail handlers `0x89868` (phase 0x1D / act 7) and `0x8A798`
 * (phase 0x1E / act 8) are ported as bounded derived bodies. As of M2 phase-19
 * T2, phase 0x1D is REGISTERED and COUNTED: the T1 period-index roll
 * (`mr->global_157ac2 == 4`) drives the mode-0 sit-8 selector in `phase_drive`,
 * which installs act 7, and the live clock-tail act tick dispatches this body
 * (see `test_engine_match_frame.c`). Phase 0x1E is now REGISTERED and COUNTED
 * (FU-241 §6.3, M2 phase-23 T4): the `set_piece` BX!=0 fallback is de-compressed
 * to dispatch act 8 invoke-now, whose clock-tail tick drives the phase-0x1E sweep
 * (see `test_phase_1E_counted` and the sweep tests below). Phase 0x1D was one
 * coupled unit with action row 0x20
 * (arm 0 carries the row-0x20 installer `0x899a2`), which is now wired (M2
 * phase-20 T3/T4, FU-217/FU-220). */

/* Phase 0x1D / act 7 (FU-194 §3/§6.1 + T2/FU-208/209): the slot is REGISTERED;
 * the dispatcher resolves and the body runs (arm 0). */
static void test_phase_1D_registered(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1D].fn == fifa96_match_action_phase_1D);
  assert(fifa96_match_dispatch_phase(&mr, 0x1Du) == FIFA96_OK);
  assert(mr.act.stage == 1u);          /* arm 0 ran via the dispatch */
}

/* Phase 0x1D stage 0 (0x898B4): clear the [0x15882A] gate (0x898B4), write
 * phase 0x10 when state.phase != 0x10 (0x898B6..0x898DD), install action row
 * 0x19 on both teams' records (0x89903/0x8990A), timer = 0 and stage -> 1
 * (0x899D7/0x899ED). Mutation-RED: without the stage advance the slot stays at
 * stage 0. */
static void test_phase_1D_install_frame(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 7;
  mr.act.stage = 0;
  mr.global_5882a = 1u;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.global_5882a == 0u);                                /* 0x898B4 */
  assert(mr.state.phase == 0x10u);                              /* 0x898CF.. */
  assert(mr.entities.team[0].records[0].code == 0x19u);         /* 0x8990A team base */
  assert(mr.entities.team[1].records[0].code == 0x19u);
  assert(mr.entities.team[0].records[1].code == 0x00u);         /* NOT fanned out (the native CALL is outside the ESI loop) */
  assert(mr.act.stage == 1u && mr.act.timer == 0);              /* 0x899ED/0x899D7 */
  assert(fifa96_match_action_phase_1D(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x1D stage 1 (0x899EE): the shared-timeline wait. Below the derived
 * `state.tick_total >= 0x78` gate the head add stands and the slot is
 * untouched; at/above it the whistle 0x1E fires and [0x15882A] = 1, timer = 0,
 * stage -> 2. Mutation-RED: a missing gate set leaves global_5882a 0. */
static void test_phase_1D_wait_whistle(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 7;
  mr.act.stage = 1;
  mr.act.timer = 10;
  mr.state.tick_total = 0x40u;        /* < 0x78: hold */
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.timer == 12);         /* head add stands */
  assert(mr.ref_whistle == 0u && mr.global_5882a == 0u && mr.act.stage == 1u);
  mr.state.tick_total = 0x78u;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.ref_whistle == 0x1Eu);    /* 0x974DC(0x1E) */
  assert(mr.global_5882a == 1u);      /* [0x15882A]=1 */
  assert(mr.act.stage == 2u && mr.act.timer == 0);
}

/* Phase 0x1D stage 9 (0x89E6E): the terminal (no situation re-dispatch) clears
 * the act slot ([0x158828]/[0x158808]/[0x158829]) and [0x157A4C]. Mutation-RED:
 * a missing terminal leaves the act slot installed. */
static void test_phase_1D_terminal(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 7;
  mr.act.stage = 9;
  mr.global_157a4c = 1u;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.id == 0u && mr.act.stage == 0u && mr.act.timer == 0);
  assert(mr.global_157a4c == 0u);     /* [0x157A4C]=0 */
}

/* FU-210 §6.1/§8 L3 (M2 phase-19 T3; bind landed M2 phase-20 T3, FU-217): the
 * phase-0x1D arm-0 coupled installer (`0x899A2`). The native target is the actor
 * record `[0x1587D4]` (`FUN_0007D9A4(rec, 0x20, 0, 0)`, deferred); its producer
 * — the sit-8 arm's `[0x1587CC+side*4] -> [0x1587D4]` bind `0x8AE4B` — is now
 * landed live (match_run_free_record + the set_piece re-init), but this fixture
 * sets the cell directly to exercise the ported install (bypassing set_piece):
 * arm 0 installs code 0x20 (deferred, stage 0) on the actor. Mutation-RED:
 * dropping the install leaves the actor's code at 0. */
static void test_phase_1D_arm0_installs_row_20(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.actor_1587d4 = 1;             /* team 0, record 1 (fixtures the unported producer) */
  mr.act.id = 7;
  mr.act.stage = 0;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.entities.team[0].records[1].code == 0x20u);  /* 0x899A2 FUN_0007D9A4(rec,0x20,0,0) */
  assert(mr.entities.team[0].records[1].stage92 == 0u);  /* deferred stage 0 */
  /* without the sit-8 bind the cell stays NONE: no record ever carries 0x20.
   * Mutation-RED on the direct-call path. */
  fifa96_match_run_init(&mr);
  mr.act.id = 7;
  mr.act.stage = 0;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.entities.team[0].records[1].code != 0x20u);
}

/* Phase 0x1E / act 8 (FU-241 §6.3, M2 phase-23 T4): the body is now COUNTED —
 * the phase-table slot is registered (the de-compression), so the dispatcher
 * reaches the body (stage 0 -> 1 on the first call). */
static void test_phase_1E_counted(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1E].fn == fifa96_match_action_phase_1E);
  assert(fifa96_match_dispatch_phase(&mr, 0x1Eu) == FIFA96_OK);
  assert(mr.act8_team == 0u && mr.act8_record == 1u);  /* stage 0 -> 1 swept one */
}

/* Phase 0x1E stage 0 -> 1 (0x8A7B2/0x8A7D6): reset the walk cursor and sweep
 * one record per call. A record with `code [rec+0x91] == 0xC`, `[rec+0x99] != 0`
 * and `[rec+0x9A] == 0` is selected (0x8A831); the selection cell is cleared
 * before every gate test (0x8A814), so a non-matching record AFTER a match
 * clears it again. Mutation-RED: a missing selection leaves act8_selected NULL;
 * a missing per-record clear leaves the previous match stuck. */
static void test_phase_1E_sweep_selects(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 8;
  mr.act.stage = 0;
  mr.entities.team[0].records[3].code = 0x0Cu;  /* 0x8A809 */
  mr.entities.team[0].records[3].skip_99 = 1u;  /* 0x8A81F */
  mr.entities.team[0].records[3].skip_9a = 0u;  /* 0x8A828 */
  /* stage 0 resets the cursor and falls through into stage 1 in the same call */
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(mr.act8_team == 0u && mr.act8_record == 1u);   /* walked record 0 */
  assert(mr.act8_selected == NULL);
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* record 1 */
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* record 2 */
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* record 3 -> select */
  assert(mr.act8_selected == &mr.entities.team[0].records[3]);
  /* the next (non-matching) record clears the cell (0x8A814) */
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* record 4 -> clear */
  assert(mr.act8_selected == NULL);
  assert(fifa96_match_action_phase_1E(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x1E stage 1 completion (0x8A857..0x8A88B): past team 1 record 10 the
 * sweep advances to stage 2 (timer = 0). Mutation-RED: a missing completion
 * leaves the stage at 1. */
static void test_phase_1E_sweep_completes(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 8;
  mr.act.stage = 1;
  mr.act8_team = 1u;
  mr.act8_record = 10u;
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(mr.act8_team == 2u && mr.act8_record == 0u);
  assert(mr.act.stage == 2u && mr.act.timer == 0);      /* 0x8A87A..0x8A88B */
}

/* Phase 0x1E stage 2 terminal (0x8A895): clear the act slot and re-dispatch the
 * stored situation/side (`store_15882b`/`store_15882c`, BX=0) then clear
 * `store_15882b` (0x8A8D3). Mutation-RED: a missing re-dispatch leaves the
 * stored situation uncleared. */
static void test_phase_1E_terminal(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 8;
  mr.act.stage = 2;
  mr.store_15882b = 3u;               /* the stored situation (0x8A8C3) */
  mr.store_15882c = 0u;               /* the stored side (0x8A8BD) */
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(mr.act.id == 0u && mr.act.stage == 0u && mr.act.timer == 0);
  assert(mr.store_15882b == 0xFFu);   /* 0x8A8D3 */
}

/* ===== FU-195 (M2 phase-17 T6): the act-tail cohort 0x21/0x22 + the 0x1B leg
 *
 * The two native act-tail handlers `0x8B874` (phase 0x21 / act 0xB, the
 * extra-time-CLEAR period-end timer) and `0x8B900` (phase 0x22 / act 0xC, the
 * extra-time-SET analogue) are ported as derived bodies. phase 0x21 and phase
 * 0x22 are the two counted rows: their producer `FUN_0008B9CC` (`0x8BAE7`) is mirrored live by
 * `fifa96_match_run_phase_drive` -> `fifa96_action_phase_period_end`
 * (`out.act = 0x0B` clear / `0x0C` set) -> the act-selector dispatch, so wiring
 * each row is an honest +1. phase 0x22's extra-time flag `[0x157AC0]` is LIVE
 * since T1/FU-213 (`mr->global_157ac0`; the hardwired `extra_time = 0` is gone),
 * so T4 registers the phase-0x22 slot (FU-211 §6.1; the T4 live-path test below
 * drives it end-to-end). phase 0x1B / act 5 has
 * no invoker (the fresh 11-xref
 * `0x888FC` census finds no `EAX=5` site), so it stays a leg (owner FU175-L2).
 * Both bodies touch only the act slot + the `0x974DC(0x1E)` audio sink
 * (`ref_whistle`), so they are render-invisible (tape-neutral). */

/* Phase 0x21 stage 0 (0x8B89E): fire the 0x1E event, timer = 0, stage -> 1. */
static void test_phase_21_stage0_advance(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 0x0B;
  mr.act.stage = 0;
  mr.act.timer = 0;
  assert(fifa96_match_action_phase_21(&mr) == FIFA96_OK);
  assert(mr.ref_whistle == 0x1Eu);      /* 0x974DC(0x1E) */
  assert(mr.act.stage == 1u);           /* 0x8B8B9 */
  assert(mr.act.timer == 0);            /* 0x8B8B2 stage-0 reset */
  assert(mr.act.id == 0x0Bu);           /* still installed */
}

/* Phase 0x21 stage 1 (0x8B8C3): below the 0x3C gate the head add stands and the
 * slot is untouched; at/above it fires the 0x1E event and clears the act slot
 * (id/stage/timer). Mutation-RED: without the clear the slot stays installed. */
static void test_phase_21_stage1_gate_clear(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 0x0B;
  mr.act.stage = 1;
  mr.act.timer = 10;
  assert(fifa96_match_action_phase_21(&mr) == FIFA96_OK);
  assert(mr.act.timer == 12);           /* head add stands (JL path) */
  assert(mr.ref_whistle == 0u && mr.act.id == 0x0Bu);
  mr.act.timer = 0x3C - 2;
  assert(fifa96_match_action_phase_21(&mr) == FIFA96_OK);
  assert(mr.ref_whistle == 0x1Eu);      /* 0x974DC(0x1E) */
  assert(mr.act.id == 0u && mr.act.stage == 0u && mr.act.timer == 0u);
}

/* Phase 0x21 stage > 1: the native 0x8B89A POP/RET no-op. */
static void test_phase_21_stage2_noop(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 0x0B;
  mr.act.stage = 2;
  mr.act.timer = 5;
  assert(fifa96_match_action_phase_21(&mr) == FIFA96_OK);
  assert(mr.act.timer == 7);            /* head add stands */
  assert(mr.act.id == 0x0Bu && mr.act.stage == 2u);
  assert(mr.ref_whistle == 0u);
  assert(fifa96_match_action_phase_21(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x21 is COUNTED: its phase-table slot is wired (the one +1 this task). */
static void test_phase_21_registered(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x21].fn == fifa96_match_action_phase_21);
  assert(fifa96_match_dispatch_phase(&mr, 0x21) == FIFA96_OK);
  assert(mr.act.stage == 1u);           /* stage 0 ran via the dispatch */
  assert(fifa96_match_dispatch_phase(NULL, 0x21) == -FIFA96_ERR_INVALID);
}

/* The producer-live route (the census claim): `phase_drive` on a completed
 * period runs the derived `FUN_0008B9CC` chooser (`out.act = 0x0B`) and then
 * the new act-selector dispatch, which installs act 0xB and runs the phase-0x21
 * stage 0 — the native `0x8BAE7 CALL 0x888FC(0xB,0,BX=1)`. Mutation-RED:
 * without the dispatch the act slot stays uninstalled and the phase-0x21 body
 * never runs. */
static void test_phase_21_producer_live_via_phase_drive(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.state.period = 1;                  /* a completed period for the chooser */
  mr.state.phase = 2;                   /* class 1 */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.state.phase == 0x0Cu);      /* derived chooser (period < 4) */
  assert(mr.act.id == 0x0Bu);           /* the 0x8BAE7 dispatch installed act B */
  assert(mr.act.stage == 1u);           /* phase-0x21 stage 0 ran invoke-now */
  assert(mr.ref_whistle == 0x1Eu);      /* the 0x974DC(0x1E) event */
}

/* Phase 0x22 / act 0xC (FU-195 §3, FU-211 §2): three stages, each gated on the
 * derived timer >= 0x3C after a reset; the terminal stage 2 clears the act slot.
 * Counted by T4: the OL-85 extra-time flag producer is live since T1/FU-213
 * (`mr->global_157ac0`), so a regulation-end completion with differing scores
 * installs act 0xC through `phase_drive`, and the registered phase-0x22 slot
 * dispatches. The body is also driven directly here. Mutation-RED: skipping a
 * stage's timer reset makes the next gate pass immediately (the stage
 * assertions below fail). */
static void test_phase_22_body_direct(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 0x0C;
  mr.act.stage = 0;
  assert(fifa96_match_action_phase_22(&mr) == FIFA96_OK);
  assert(mr.ref_whistle == 0x1Eu);      /* stage-0 event */
  assert(mr.act.stage == 1u && mr.act.timer == 0u);
  /* stage 1: below the gate hold; at the gate -> event + stage 2 */
  mr.act.timer = 10;
  assert(fifa96_match_action_phase_22(&mr) == FIFA96_OK);
  assert(mr.act.timer == 12 && mr.act.stage == 1u);
  mr.act.timer = 0x3C - 2;
  assert(fifa96_match_action_phase_22(&mr) == FIFA96_OK);
  assert(mr.act.stage == 2u && mr.act.timer == 0u);
  /* stage 2: below the gate hold; at the gate -> event + clear slot */
  mr.act.timer = 10;
  assert(fifa96_match_action_phase_22(&mr) == FIFA96_OK);
  assert(mr.act.id == 0x0Cu && mr.act.stage == 2u && mr.act.timer == 12);
  mr.act.timer = 0x3C - 2;
  assert(fifa96_match_action_phase_22(&mr) == FIFA96_OK);
  assert(mr.act.id == 0u && mr.act.stage == 0u && mr.act.timer == 0u);
  assert(fifa96_match_action_phase_22(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x22 is COUNTED (T4/FU-211 §6.1): the phase-table slot is wired to the
 * already-ported body, so `fifa96_match_dispatch_phase(0x22)` resolves and runs
 * stage 0 (the act-0xC install is the live `0x8BAE7` route). */
static void test_phase_22_registered(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x22].fn == fifa96_match_action_phase_22);
  mr.act.id = 0x0C;
  assert(fifa96_match_dispatch_phase(&mr, 0x22) == FIFA96_OK);
  assert(mr.act.stage == 1u);           /* stage 0 ran via the dispatch */
  assert(mr.ref_whistle == 0x1Eu);      /* the 0x974DC(0x1E) event */
  assert(fifa96_match_dispatch_phase(NULL, 0x22) == -FIFA96_ERR_INVALID);
}

/* The producer-live route (the census claim, FU-211 §5 + T1/FU-213): a live
 * regulation-end completion with DIFFERING scores raises the OL-85 flag
 * (`global_157ac0`, W2 0x8B2AC) and the derived FUN_0008B9CC chooser installs
 * the extra-time arm — `out.act = 0x0C` — which `phase_drive` dispatches
 * (`0x8BAE7 CALL 0x888FC(0xC,0,BX=1)`). With the phase-0x22 slot registered the
 * body runs invoke-now (stage 0 fires the 0x1E event). Mutation-RED: with the
 * slot NULL the act dispatch early-returns at fifa96_match_act.c:51, so
 * `act.stage` stays 0 and `ref_whistle` stays 0 (the assertions below fail). */
static void test_phase_22_producer_live_via_phase_drive(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.state.period = 2;                  /* the FU-62 library rolled 1 -> 2 */
  mr.state.phase = 2;                   /* class 1: the clock runs */
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 0u;                     /* differing -> W2 raises the flag */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac0 == 1u);       /* the live OL-85 flag */
  assert(mr.state.phase == 0x13u);      /* the extra-time chooser arm */
  assert(mr.act.id == 0x0Cu);           /* act 0xC installed */
  assert(mr.act.stage == 1u);           /* phase-0x22 stage 0 ran invoke-now */
  assert(mr.ref_whistle == 0x1Eu);      /* the 0x974DC(0x1E) event */
  assert(mr.global_157ac2 == 2u);       /* the roll 0x8B58A */
}

/* Phase 0x1B / act 5 (FU195-L1, owner FU175-L2): NO invoker — the fresh
 * 11-xref `0x888FC` census finds no EAX=5 site — so the slot stays NULL/uncounted
 * and no invoker is invented. The act-5 domain is valid (<= 0xC) but resolves to
 * the unwired phase 0x1B. */
static void test_phase_1B_leg_no_invoker(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1B].fn == NULL);
  assert(fifa96_match_dispatch_phase(&mr, 0x1B) == -FIFA96_ERR_UNSUPPORTED);
  /* act 5 installs but invokes nothing (its phase 0x1B row is NULL) */
  assert(fifa96_match_act_dispatch(&mr, 5, 0, 1) == FIFA96_OK);
  assert(mr.act.id == 5);
  assert(mr.state.phase == 0u);         /* no handler ran */
}

/* FU-243 §4.2/§6 (M2 phase-23 T2): row 0x25 is COUNTED. The prior FU189-L2
 * "row 25 has NO static installer" verdict is REFUTED — the ECX-carried bulk
 * installer `0x8D79D`/`0x8D7AD` (`MOV ECX,0x25 -> FUN_0008CEB8`) installs code
 * 0x25 on the live `FUN_0008D098` path and is already modelled by
 * `fifa96_match_phase_machine_step` (`code = ac5!=ac7 ? 0x25 : 3`). Only the
 * body `0x880CC` (the FU-82 7-arm table `0x880B0`) was unported. Mutation-RED:
 * a NULL slot returns UNSUPPORTED and the dispatch assertion fails. */
static void test_row_25_counted(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_action_table[0x25].fn != NULL);
  assert(fifa96_match_dispatch_action(&mr, 0x25u) == FIFA96_OK);
  assert(fifa96_match_dispatch_action(NULL, 0x25u) == -FIFA96_ERR_INVALID);
}

/* The ported row-25 body (`0x880CC`, 7-arm table `0x880B0`) is driven directly
 * (the slot is wired). A fresh record is stage 0, timer 0, inactive: arm 0's
 * `timer89 >= 0x3C` gate holds, so the body returns without advancing. At a
 * passed timer with an inactive record, arm 0 builds the five-point stats
 * scatter (7 RNG draws), arms the first point and advances to stage 1. */
static void test_row_25_body_direct(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.record.entity_id = 1;             /* team 0 record 1 */
  mr.record.delta = 2;
  assert(fifa96_match_dispatch_action(&mr, 0x25u) == FIFA96_OK);
  assert(mr.record.timer89 == 2);
  assert(mr.record.stage92 == 0u);     /* arm 0 waits below 0x3C */
  /* pass the arm-0 gate: the inactive scatter branch arms + advances to 1 */
  mr.record.timer89 = 0x3C - 2;
  assert(fifa96_match_dispatch_action(&mr, 0x25u) == FIFA96_OK);
  assert(mr.record.stage92 == 1u);
  assert(mr.record.timer89 == 0);
  assert(mr.celebration.idx == 0);
  assert(fifa96_match_dispatch_action(NULL, 0x25u) == -FIFA96_ERR_INVALID);
}

/* FU-243 §4.3/§6 (M2 phase-23 T3): row 0x15 is the phase-5 celebration sibling
 * of row 0x14 — the same `0x8D433` arm installs code 0x15 on records 1..10 of
 * the goal team (FU-243-L2's "arm not ported" claim is stale: the port lives in
 * `fifa96_match_run_phase_arm`'s `phase == 5` block). Only the body `0x87CD0`
 * was unported. Mutation-RED: a NULL slot returns UNSUPPORTED, so the dispatch
 * assertion below fails. */
static void test_action_15_dispatches(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_action_table[0x15].fn != NULL);
  assert(fifa96_match_dispatch_action(&mr, 0x15u) == FIFA96_OK);
  assert(fifa96_match_dispatch_action(NULL, 0x15u) == -FIFA96_ERR_INVALID);
}

/* The ported row-0x15 body (`0x87CD0`, the 4-arm table `0x87CC0`). The phase-5
 * gate (`0x87CDB..0x87CE6`) resets on any other phase; at phase 5 the stage
 * walk runs. A fresh record is stage 0, inactive: the stage-0 `[rec+0x8D] == 0`
 * gate resets. An active stage-0 record arms `scratch_ae = 0x3C`, clears the
 * timer and advances to stage 1. Mutation-RED: dropping the stage advance
 * leaves `stage92` at 0. */
static void test_action_15_body_direct(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.record.entity_id = 1;              /* team 0 record 1 */
  mr.record.delta = 2;
  /* phase != 5 -> the reset tail (the record's code returns to 0) */
  mr.state.phase = 4u;
  mr.entities.team[0].records[1].code = 7u;
  assert(fifa96_match_dispatch_action(&mr, 0x15u) == FIFA96_OK);
  assert(mr.entities.team[0].records[1].code == 0u);
  /* phase 5, stage 0, inactive -> reset */
  mr.state.phase = 5u;
  mr.record.stage92 = 0u;
  mr.record.active = 0u;
  mr.entities.team[0].records[1].code = 7u;
  assert(fifa96_match_dispatch_action(&mr, 0x15u) == FIFA96_OK);
  assert(mr.entities.team[0].records[1].code == 0u);
  /* phase 5, stage 0, active -> arm scratch_ae, clear timer, advance to 1 */
  mr.record.stage92 = 0u;
  mr.record.active = 1u;
  mr.record.timer89 = 0;
  assert(fifa96_match_dispatch_action(&mr, 0x15u) == FIFA96_OK);
  assert(mr.record.timer89 == 0);
  assert(mr.record.scratch_ae == 0x3C);
  assert(mr.record.stage92 == 1u);
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
  test_phase_17_counted();
  test_phase_17_kickoff_stage0();
  test_phase_17_stage1_handoff();
  test_phase_17_stage1_timer();
  test_phase_1A_not_counted();
  test_phase_1A_dormant();
  test_phase_1A_install();
  test_phase_1A_end_block();
  test_phase_1C_not_counted();
  test_phase_1C_stage0();
  test_phase_1C_terminal();
  test_phase_1D_registered();
  test_phase_1D_install_frame();
  test_phase_1D_wait_whistle();
  test_phase_1D_terminal();
  test_phase_1D_arm0_installs_row_20();
  test_phase_1E_counted();
  test_phase_1E_sweep_selects();
  test_phase_1E_sweep_completes();
  test_phase_1E_terminal();
  test_phase_21_stage0_advance();
  test_phase_21_stage1_gate_clear();
  test_phase_21_stage2_noop();
  test_phase_21_registered();
  test_phase_21_producer_live_via_phase_drive();
  test_phase_22_body_direct();
  test_phase_22_registered();
  test_phase_22_producer_live_via_phase_drive();
  test_phase_1B_leg_no_invoker();
  test_row_25_counted();
  test_row_25_body_direct();
  test_action_15_dispatches();
  test_action_15_body_direct();
  puts("test_engine_match_act OK");
  return 0;
}
