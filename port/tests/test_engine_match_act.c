/* port/tests/test_engine_match_act.c — M2 phase-15 T1 / FU-175: the native
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
 *     word[0x157A64]`; stage 0 `0x88F81` clears `[0x15882A]` (`0x88F9A`),
 *     writes phase 0x15 on the controlled side and zeroes the timer
 *     (`0x88FE0`); stage 3 `0x890B1` gates on
 *     `word[0x158818] >= 0x3C` (the `[0x158816] SAR 0x10` load = the act
 *     timer; the old `[0x15881A]` reading was the alias misread), then sets
 *     `[0x15882A]=1` (`0x890C4`) and clears the act slot
 *     (`0x890D3..0x890DE`).
 *
 * The derived engine keeps the FU-170 §7 model: the sit-0xC arm installs act 9
 * with invoke=1; the phase-0x1F stage-0 handler owns the phase-0x15 write —
 * the native arm is
 * `0x8AF0E MOV EBX,1 / 0x8AF13 MOV EAX,9 / 0x8AF18 XOR EDX,EDX / 0x8AF1A CALL
 * 0x888FC`, so `0x88937 CALL dword[0x158808]` runs the phase-0x1F stage-0 body
 * on the install call. The begin-time install is one-shot: the native
 * match-init (`0x4B095`/`0x4B0A5`, situation 0) supersedes the slot with act
 * 0xA. Stages 1/2 (`0x88FED`/`0x89064`, the leg/`0x74110`/`0x93850`/`0x361B0`
 * gates) are compressed to pass; the stage-3 gate reads the same act timer
 * (`word[0x158818]`), which is native-exact. */
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
 * phase-0x1F handler body owns the stage-0 phase-0x15 write (FU-170 §7). */
static void test_dispatch_invoke_runs_handler(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.global_5882a = 1u;
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 1) == FIFA96_OK);
  assert(fifa96_match_run_write_phase(NULL, 0x15u) == -FIFA96_ERR_INVALID);
  assert(mr.global_5882a == 0u);             /* 0x88F9A stage-0 clear */
  assert(mr.state.phase == 0x15u);           /* the stage-0 write */
  assert(mr.act.stage == 3u);                /* stages 1/2 compressed */
  assert(mr.act.id == 9 && mr.act.timer == 0);   /* stage 0 zeroes it */
  /* act 1 resolves to phase 0x17, registered by T4 (FU-203): the invoke runs
   * the phase-0x17 stage-0 kickoff entry (clear the gate + the phase-1 write) */
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  assert(fifa96_match_act_dispatch(&mr, 1, 0, 1) == FIFA96_OK);
  assert(mr.act.id == 1);
  assert(mr.act.stage == 1u);                /* stage 0 -> 1 (0x88EB1) */
  assert(mr.state.phase == 1u);              /* the kickoff phase-1 entry */
}

/* FU-181 L5 / FU-342 (phase-53 T2): the act-9 stage-0 match/camera reset
 * (`0x88FA5 CALL 0x73E28`). Dirty every native cell group, then run the
 * invoke-now path: the reset must clear the phase/clock/focus/goal/controlled
 * cells, zero the frame delta (so the same-frame stage-3 head add reads 0),
 * stage the camera height anchor from the `[0x14C2F6]` mode, zero the
 * possession/carrier block and re-seat the ball-pair block to its native
 * posture (flags 0x20, code 2). The phase write then sets phase 0x15 /
 * prev 0. A second direct call with mode != 0 stages the 0xA0 anchor. */
static void test_act9_stage0_resets_match_cells(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.global_157a4c = 1u;
  mr.state.phase = 0x15u;
  mr.state.prev_phase = 0x15u;
  mr.phase_machine.state = 0x15u;
  mr.phase_machine.phase = 0x15u;
  mr.state.frame_acc = 0x56u;
  mr.clock_saved_157a60 = 0x1234u;
  mr.deepest_keeper_157aa7 = 5;
  mr.goal_record = 7;
  mr.entities.controlled = 3;
  mr.render.camera.event_suspended = 1u;
  mr.referee.offside_suppress = 0x12Cu;
  mr.state.frame_delta = 2u;
  mr.flag_157aab = 0u;
  mr.camera_height_157a6d = 0;
  mr.flag_157ab0 = 1u;
  mr.camera_override_157a6f = 0x1234;
  mr.difficulty_14c2f6 = 0;                 /* the image default -> 0xB0 */
  mr.entities.ball.carrier = 4;
  mr.entities.ball.pos_index = 1u;
  mr.entities.ball.pos_rotation = 2u;
  mr.entities.ball.pos_dir_x = 3;
  mr.entities.ball.pos_dir_z = 4;
  mr.entities.ball.pos_counter_c = 5u;
  mr.entities.ball.pos_release = 6u;
  mr.entities.ball.pos_counter_e = 7u;
  mr.entities.ball.pos_counter_f = 8u;
  memset(&mr.entities.ball.pair, 0xAA, sizeof mr.entities.ball.pair);
  mr.entities.ball.target_x = 0x1E0;
  mr.entities.ball.target_y = 0;
  mr.entities.ball.target_z = 0;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 1) == FIFA96_OK);
  /* The reset stores (0x73E3C..0x73EC7). */
  assert(mr.deepest_keeper_157aa7 == 0);
  assert(mr.global_157a4c == 0u);
  assert(mr.state.prev_phase == 0u);        /* the phase write: prev 0 */
  assert(mr.state.phase == 0x15u);          /* the phase write: 0x15 */
  assert(mr.phase_machine.phase == 0x15u);
  assert(mr.state.frame_acc == 0u);         /* 0x157A5C live Q8 */
  assert(mr.clock_saved_157a60 == 0u);      /* 0x157A60 saved Q8 */
  assert(mr.goal_record == FIFA96_MATCH_ENTITY_NONE);   /* 0x157A9F */
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE); /* 0x157A83 */
  assert(mr.render.camera.event_suspended == 0u);       /* 0x157A6C */
  assert(mr.referee.offside_suppress == 0u);            /* 0x157A6A */
  assert(mr.state.frame_delta == 0u);                   /* 0x157A64 */
  assert(mr.flag_157aab == 0xFFu);                      /* 0x157AAB */
  assert(mr.camera_height_157a6d == 0xB0);              /* 0x157A6D */
  assert(mr.flag_157ab0 == 0u);                         /* 0x157AB0 */
  assert(mr.camera_override_157a6f == 0);               /* 0x157A6F */
  /* 0x7F144: the possession/carrier block 0x158724..2C. */
  assert(mr.entities.ball.carrier == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.entities.ball.pos_index == 0u && mr.entities.ball.pos_rotation == 0u &&
         mr.entities.ball.pos_dir_x == 0 && mr.entities.ball.pos_dir_z == 0 &&
         mr.entities.ball.pos_counter_c == 0u && mr.entities.ball.pos_release == 0u &&
         mr.entities.ball.pos_counter_e == 0u && mr.entities.ball.pos_counter_f == 0u);
  /* 0x7A028: the ball-pair block reset posture. */
  assert(mr.entities.ball.pair.actor == 0 && mr.entities.ball.pair.receiver == 0);
  assert(mr.entities.ball.pair.vector.x == 0 && mr.entities.ball.pair.vector.height == 0 &&
         mr.entities.ball.pair.vector.z == 0);
  assert(mr.entities.ball.pair.traj == 0 && mr.entities.ball.pair.angle == 0);
  assert(mr.entities.ball.pair.flags == 0x20u && mr.entities.ball.pair.code == 2u);
  assert(mr.entities.ball.pair.sub_code == 0u && mr.entities.ball.pair.reserved45 == 0u &&
         mr.entities.ball.pair.ack == 0u);
  /* The 0x73E08 placement commit copied the staged target into the ball. */
  assert(mr.entities.ball.x == 0x1E0 && mr.entities.ball.y == 0 &&
         mr.entities.ball.z == 0);
  assert(mr.act.id == 9 && mr.act.stage == 3u && mr.act.timer == 0);
  /* The other `[0x14C2F6]` arm: mode != 0 stages 0x30 + 0x70 = 0xA0. */
  mr.difficulty_14c2f6 = 1;
  assert(fifa96_match_run_act9_reset(&mr) == FIFA96_OK);
  assert(mr.camera_height_157a6d == 0xA0);
  assert(fifa96_match_run_act9_reset(NULL) == -FIFA96_ERR_INVALID);
}
/* The native sit-0xC arm is invoke-now (`0x8AF0E MOV EBX,1 / 0x8AF13 MOV EAX,9
 * / 0x8AF18 XOR EDX,EDX / 0x8AF1A CALL 0x888FC`; the seam's `0x88937 CALL
 * dword[0x158808]`), so the leg-0 screen install queues situation 0xC and runs
 * the phase-0x1F stage-0 body on the same call. The begin-time install is
 * one-shot: the native match-init (`0x4B095`/`0x4B0A5`, situation 0) supersedes
 * the slot with act 0xA, so this install invoke is the ordinary-match reach.
 * Mutation-RED: dropping the invoke flag leaves `act.stage` at 0 — only the
 * stage-0 body writes 3 (the SETUP0 step already zeroes `global_5882a`, so the
 * stage is the distinguishing cell). */
static void test_sit0c_install_invokes_stage0(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.running = 1;
  assert(fifa96_match_run_screen_install(&mr, 0, 0, 0) == 0);
  assert(mr.screen_leg == 0 && mr.screen_step == 1u);  /* SETUP0 ran */
  assert(mr.state.phase == 0x15u);      /* the stage-0 phase write */
  assert(mr.act.id == 9);               /* act 9 installed (0x8890A) */
  assert(mr.act.stage == 3u);           /* stage 0 ran on the install call */
  assert(mr.global_5882a == 0u);        /* 0x88F9A */
  /* The mechanism pin: the same slot through the BX=0 seam stays at stage 0, so
   * a regression to invoke=0 fails the stage-3 assertion above. */
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.global_5882a = 1u;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 0) == FIFA96_OK);
  assert(mr.act.id == 9 && mr.act.stage == 0u);
  assert(mr.global_5882a == 1u);        /* no body ran */
}

/* Phase-53 T1 (FU-181 L5 / FU-342): the act-9 stage-0 sinks in one invoke.
 * The fixture proves the sinks ran and their final cells — the gate clear, the
 * camera reset (`0x88FA0`/`0x700F4`, the constant-zero pushed `0x10F328/2C/30`
 * triple), the phase-0x15 write (`0x88FB8`) and the presentation bind
 * (`0x88FD1`/`0x4C324`, all args unused) — not the inter-sink order (that is
 * the source statement order). A dirty camera proves the reset; `present_side`
 * proves the bind's first arm. Mutation-RED: dropping the camera call leaves
 * the dirty triple/tracked/pan/prev cells; dropping the bind leaves
 * `present_side` staged. */
static void test_act9_stage0_camera_and_present_bind(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.running = 1;
  mr.global_5882a = 1u;
  mr.state.frame_delta = 2;
  mr.render.camera.pos_x = 0x1234;
  mr.render.camera.pos_y = 0x2345;
  mr.render.camera.pos_z = 0x3456;
  mr.render.camera.prev_x = 0x6789;
  mr.render.camera.prev_y = 0x789A;
  mr.render.camera.prev_z = 0x89AB;
  mr.render.camera.target_x = 0x4567;
  mr.render.camera.anchor_z = 0x5678;
  mr.render.camera.tracked = 5;
  mr.render.camera.pan_counter = 7;
  mr.render.camera.pan_decay = 0;
  mr.render.camera.pan_rate_hi = 0;
  mr.render.camera.pan_rate_lo = 0;
  /* First bind arm: `[0x157AC2] >= 4` with a live `[0x1587D4]` record (team 1
   * -> the pool team's `+0x826` side = 1). */
  mr.global_157ac2 = 4u;
  mr.actor_1587d4 = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 1;
  mr.render.sub.current_side = 0u;
  assert(fifa96_match_act_dispatch(&mr, 9, 0, 1) == FIFA96_OK);
  assert(mr.global_5882a == 0u);                    /* 0x88F9A */
  assert(mr.render.camera.pos_x == 0 && mr.render.camera.pos_y == 0 &&
         mr.render.camera.pos_z == 0);              /* 0x88FA0 */
  assert(mr.render.camera.prev_x == 0 && mr.render.camera.prev_y == 0 &&
         mr.render.camera.prev_z == 0);             /* 0x700F4 prev clear */
  assert(mr.render.camera.target_x == 0 && mr.render.camera.target_y == 0 &&
         mr.render.camera.target_z == 0);
  assert(mr.render.camera.anchor_x == 0 && mr.render.camera.anchor_y == 0 &&
         mr.render.camera.anchor_z == 0);
  assert(mr.render.camera.tracked == FIFA96_CAMERA_TRACKED_NONE);
  assert(mr.render.camera.pan_counter == 0);
  assert(mr.render.camera.pan_decay == 10 &&
         mr.render.camera.pan_rate_hi == 16 &&
         mr.render.camera.pan_rate_lo == 24);
  assert(mr.state.phase == 0x15u);                  /* 0x88FB8 */
  assert(mr.present_side == 1);                     /* 0x88FD1 */
  assert(mr.act.stage == 3u && mr.act.timer == 0);
  /* Fallback arm: mode < 4 keeps the record pointer live but takes
   * `[0x1587E3]>>24`, sign-extended (`0x4C363..0x4C36B`). */
  mr.global_157ac2 = 3u;
  mr.render.sub.current_side = 0xFEu;
  assert(fifa96_match_run_present_bind(&mr) == FIFA96_OK);
  assert(mr.present_side == (int32_t)(int8_t)0xFEu);
  /* Mode >= 4 with no live pointer also takes the fallback. */
  mr.global_157ac2 = 4u;
  mr.actor_1587d4 = FIFA96_MATCH_ENTITY_NONE;
  mr.render.sub.current_side = 1u;
  assert(fifa96_match_run_present_bind(&mr) == FIFA96_OK);
  assert(mr.present_side == 1);
  assert(fifa96_match_run_present_bind(NULL) == -FIFA96_ERR_INVALID);
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
    /* Each tick models one granted frame: the frame head recomputes
     * `[0x157A64]` from the Q8 accumulator before the act tick. The act-9
     * stage-0 reset zeroes the delta in-frame (0x73E28), so the same-frame
     * stage-3 head add reads 0; the next frame's head restores 2. */
    mr.state.frame_delta = 2;
    assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
    assert(mr.act.id == 9);
  }
  assert(mr.act.timer == 56);                 /* the first tick zeroes, 28 * 2 */
  assert(mr.act.stage == 3u);                 /* stage 0 ran on the first tick */
  assert(mr.state.phase == 0x15u);            /* stage-0 phase write */
  assert(mr.global_5882a == 0u && mr.act9_gate == 0u);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* 58 */
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
  mr.record.distance = 0;                     /* 0x8480A: < 0x20 advances */
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);  /* stage1->2->3 */
  assert(mr.record.stage92 == 3u);
  assert(mr.record.timer89 == 0);
  assert(mr.record.pos_x == -0x860);          /* the stage-2 +0x59 write */
  assert(mr.record.pos_y == 0 && mr.record.pos_z == 0);
  /* stage 3: wait then reset (the 0x8496E distance gate) */
  mr.record.timer89 = 0;
  assert(fifa96_match_action_17(&mr) == FIFA96_OK);
  assert(mr.record.stage92 == 3u && mr.record.timer89 == 2);
  mr.record.timer89 = 0x3C - 2;
  mr.record.distance = 0;                     /* 0x8496E: < 0x60 resets */
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

/* Phase 0x01/0x11/0x12 (phase-36 R6): the per-record formation placement
 * `0x6E1D0`. Mutation-RED: the side-selected pair, the ±scales and the
 * unbound gate each fail if dropped. */
static void test_phase_01_formation_body(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  static const uint8_t cells[4] = {1, 2, 3, 4};
  fifa96_match_run_init(&mr);
  assert(fifa96_match_port_table[FIFA96_MATCH_ACTION_ROWS + 0x01].fn ==
         fifa96_match_phase_01);
  rec = &mr.entities.team[0].records[1];
  rec->team = 0;
  rec->phase_meta8 = cells;
  mr.phase_walk_active = 1;
  mr.phase_walk_rec = rec;
  mr.entities.team[0].side = 0;
  mr.phase_machine.side_controlled = 0;

  /* same side -> pair +2 = {3,4}; x = 3*0x26, z = 4*0x21 */
  rec->target_x = 0x111;
  rec->target_y = 0x222;
  rec->target_z = 0x333;
  assert(fifa96_match_phase_01(&mr) == FIFA96_OK);
  assert(rec->target_x == 3 * 0x26);
  assert(rec->target_y == 0);
  assert(rec->target_z == 4 * 0x21);

  /* other side -> pair +0 = {1,2} */
  mr.phase_machine.side_controlled = 1;
  assert(fifa96_match_phase_01(&mr) == FIFA96_OK);
  assert(rec->target_x == 1 * 0x26);
  assert(rec->target_z == 2 * 0x21);

  /* the team-side negation (other side -> pair +0) */
  mr.entities.team[0].side = 1;
  mr.phase_machine.side_controlled = 0;
  assert(fifa96_match_phase_01(&mr) == FIFA96_OK);
  assert(rec->target_x == -(1 * 0x26));
  assert(rec->target_z == -(2 * 0x21));

  /* unbound metadata is the native gate */
  rec->phase_meta8 = NULL;
  rec->target_x = 0x111;
  rec->target_y = 0x222;
  rec->target_z = 0x333;
  assert(fifa96_match_phase_01(&mr) == FIFA96_OK);
  assert(rec->target_x == 0x111 && rec->target_y == 0x222 &&
         rec->target_z == 0x333);
  assert(fifa96_match_phase_01(NULL) == -FIFA96_ERR_INVALID);
}

/* Phase 0x13/0x14 (phase-36, fresh `0x6E244..0x6E32A`): the camera-scatter
 * body. Mutation-RED: the side gate, the inactive z negation, the active
 * four-draw scatter and the no-negation x write each fail if dropped. */
static void test_phase_13_inactive(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  struct fifa96_rng rng_before;
  fifa96_match_run_init(&mr);
  rec = &mr.entities.team[0].records[1];
  rec->team = 0;
  rec->active = 0;
  rec->target_x = 0x111;
  rec->target_y = 0x222;
  rec->target_z = 0x333;
  mr.phase_walk_active = 1;
  mr.phase_walk_rec = rec;
  mr.phase_machine.side_controlled = 0;   /* team[0].side == 0 */
  mr.global_157ac2 = 2;
  mr.global_10f364 = 0x1234;
  mr.global_10f368 = 0xC8;
  rng_before = mr.rng;
  assert(fifa96_match_phase_13(&mr) == FIFA96_OK);
  assert(rec->target_x == 0x1234);
  assert(rec->target_y == 0x222);          /* +0x51 is untouched */
  assert(rec->target_z == -0xC8);          /* ac2 < 4 && side == 0 */
  assert(memcmp(&rng_before, &mr.rng, sizeof rng_before) == 0);
  mr.global_157ac2 = 4;
  assert(fifa96_match_phase_13(&mr) == FIFA96_OK);
  assert(rec->target_z == 0xC8);           /* ac2 >= 4: no negation */
  assert(fifa96_match_phase_13(NULL) == -FIFA96_ERR_INVALID);
}

static void test_phase_13_active(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  fifa96_match_run_init(&mr);
  rec = &mr.entities.team[0].records[1];
  rec->team = 0;
  rec->active = 1;
  rec->target_x = 0x111;
  rec->target_y = 0x222;
  rec->target_z = 0x333;
  mr.phase_walk_active = 1;
  mr.phase_walk_rec = rec;
  mr.phase_machine.side_controlled = 0;
  mr.global_157ac2 = 2;
  mr.global_10f364 = 0;
  mr.global_10f368 = 0x5A0;
  fifa96_rng_seed(&mr.rng, 0);
  assert(fifa96_match_phase_13(&mr) == FIFA96_OK);
  assert(rec->target_z == -0x5F0);         /* 0x5A0 + 0x50, negated */
  assert(rec->target_x == 0x1F);
  assert(rec->target_y == 0x222);
  /* the same staged cells with ac2 = 4 keep the magnitude and flip the sign */
  mr.global_157ac2 = 4;
  fifa96_rng_seed(&mr.rng, 0);
  assert(fifa96_match_phase_13(&mr) == FIFA96_OK);
  assert(rec->target_z == 0x5F0);
  assert(rec->target_x == 0x1F);
  /* the side gate leaves the target and the RNG untouched */
  mr.phase_machine.side_controlled = 1;
  rec->target_x = 0x111;
  rec->target_z = 0x333;
  assert(fifa96_match_phase_13(&mr) == FIFA96_OK);
  assert(rec->target_x == 0x111 && rec->target_z == 0x333);
}

/* The `FUN_0008D098` scatter producer `0x8D693..0x8D722`. */
static void test_phase_scatter_producer(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  fifa96_rng_seed(&mr.rng, 0);
  mr.global_157ac2 = 2;
  assert(fifa96_match_phase_machine_scatter(&mr, 0) == FIFA96_OK);
  assert(mr.global_10f364 == 0x20);
  assert(mr.global_10f368 == 0x61F);
  assert(fifa96_match_phase_machine_scatter(&mr, 1) == FIFA96_OK);
  assert(mr.global_10f364 == -0x2D);
  assert(mr.global_10f368 == 0x5C3);
  {
    uint16_t next = 0;
    assert(fifa96_rng_step(&mr.rng, &next) == FIFA96_OK);
    assert(next == 0xD483);
  }
  fifa96_rng_seed(&mr.rng, 0);
  mr.global_157ac2 = 4;
  assert(fifa96_match_phase_machine_scatter(&mr, 0) == FIFA96_OK);
  assert(mr.global_10f364 == -0x2D0 && mr.global_10f368 == 0x5A0);
  assert(fifa96_match_phase_machine_scatter(&mr, 1) == FIFA96_OK);
  assert(mr.global_10f364 == 0x2D0 && mr.global_10f368 == 0x5A0);
  assert(fifa96_match_phase_machine_scatter(NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_phase_machine_scatter(&mr, 2) == -FIFA96_ERR_INVALID);
}

/* Phase 0x13 write seam (phase-51 T1/FU-340): `fifa96_match_run_write_phase`
 * runs the engine's walk-then-scatter seam — walk team 0, scatter team 0, walk
 * team 1, scatter team 1 — so (a) the body reads the cells left by the
 * previous write, and (b) the team-1 scatter overwrites the team-0 pair before
 * the next body run. The native `FUN_0008D098` order is
 * team[`side_controlled`] then team[`side_controlled ^ 1`]
 * (`0x740C8`/`0x740DB`); M2's writes carry side 0, so this fixture's 0 -> 1
 * order coincides (the side-1 swap is FU-340 §6's carried leg). `ac2 = 4`
 * keeps the producer draw-free so the cell deltas are exact. Mutation-RED:
 * moving the scatter to `machine_step` (per frame) changes the cells/targets
 * below; dropping a team's scatter is caught by the m2 40-draw rider (32 body
 * + 8 producer), not by this cell fixture. */
static void test_phase_13_write_seam(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  struct fifa96_rng rng_mid;
  int32_t t2_x, t2_z, w_x, w_z;
  fifa96_match_run_init(&mr);
  mr.phase_machine.side_controlled = 0;
  mr.global_157ac2 = 4;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    mr.entities.team[t].side = (uint8_t)t;
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      mr.entities.team[t].records[i].team = (uint8_t)t;
      mr.entities.team[t].records[i].active = 0;
      mr.entities.team[t].records[i].skip_9a = 0;
    }
  }
  e = &mr.entities.team[0].records[0];
  e->active = 1;
  fifa96_rng_seed(&mr.rng, 0);
  assert(fifa96_match_run_write_phase(&mr, 0x13u) == FIFA96_OK);
  /* the team-1 fixed scatter is the last write of the seam */
  assert(mr.global_10f364 == 0x2D0 && mr.global_10f368 == 0x5A0);

  /* the second write reads exactly that pair: replaying the same stream with
   * the cells zeroed moves the body's targets by the cell values. */
  rng_mid = mr.rng;
  assert(fifa96_match_run_write_phase(&mr, 0x13u) == FIFA96_OK);
  t2_x = e->target_x;
  t2_z = e->target_z;
  assert(mr.global_10f364 == 0x2D0 && mr.global_10f368 == 0x5A0);
  mr.rng = rng_mid;
  mr.global_10f364 = 0;
  mr.global_10f368 = 0;
  e->target_x = 0;
  e->target_z = 0;
  assert(fifa96_match_run_record_phase_walk(&mr, 0x13u, 0u) == FIFA96_OK);
  w_x = e->target_x;
  w_z = e->target_z;
  assert((int16_t)(t2_x - w_x) == 0x2D0);
  assert((int16_t)(t2_z - w_z) == 0x5A0);
}

/* Phase 0x1C / act 6 (FU-193 §4 + phase-36 R9): the pre-restart timeline is
 * WIRED. The offside machine is the act-6 producer; the body wraps the
 * already-ported `fifa96_ref_offside_sequence_step` (native 0x89110).
 * Mutation-RED: a NULL slot returns UNSUPPORTED. */
static void test_phase_1C_counted(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1C].fn == fifa96_match_action_phase_1C);
  assert(fifa96_match_dispatch_phase(&mr, 0x1Cu) == FIFA96_OK);
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
  assert(mr.global_157ac0 == 1u);     /* 0x89E8C [0x157AC0]=1 */
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

/* ===== FU-209 §2 arms 2..8 (the phase-0x1D middle of the timeline) ========
 *
 * Fresh this round: every arm is a fall-through extension of the previous one
 * (`0x89868` has no per-arm epilogue except arm 3's two RET exits), so one call
 * can walk several stages. The tests below pin one wait gate per arm plus the
 * complete 0 -> 9 drive. */

/* Arm 2 (0x89A4E): the actor/referee contact wait. The gate is the actor
 * record `[0x1587D4]` vs the staged ball actor `[0x158730]` and the
 * `byte[0x158746]` ack. Both a mismatched actor and a clear ack hold (the head
 * timer add stands); the pair opens the stage-3 advance. Mutation-RED: dropping
 * either compare advances without the contact. */
static void test_phase_1D_arm2_contact_wait(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 7;
  mr.act.stage = 2;
  mr.act.timer = 10;
  mr.actor_1587d4 = 3;
  mr.entities.ball.pair.actor = 4;      /* 0x89A52 JNZ -> epilogue */
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 2u && mr.act.timer == 12);   /* head add stands */
  mr.entities.ball.pair.actor = 3;
  mr.entities.ball.pair.ack = 0;        /* 0x89A65 JZ -> epilogue */
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 2u);
  mr.entities.ball.pair.ack = 1;        /* 0x89A6B..0x89A74 */
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 3u && mr.act.timer == 0);
}

/* Arm 3 (0x89A7A) + arm 4 (0x89AEA): the goal-pan gate and the two-phase
 * camera-word handshake. `[0x15781D]`/`[0x15781E]` are the header-bound
 * goal_armed/goal_zone; `word[0x1577BE]`/`word[0x1577F0]` are the live
 * camera.speed/event_param. The pan branch (`armed && zone == 0`) advances to
 * stage 4 and RETURNS (0x89AAE); the `0x89AB3` tail advances on either camera
 * word high and also RETURNS; arm 4 requires BOTH zero and falls into arm 5. */
static void test_phase_1D_arm3_4_camera_handshake(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 7;
  mr.act.stage = 3;
  mr.act.timer = 5;
  /* neither branch: armed clear, both camera words zero -> hold (0x89AC5) */
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 3u && mr.act.timer == 7);
  /* armed but in the zone: still the hold path (0x89A8A JNZ) */
  mr.goal_armed = 1u;
  mr.goal_zone = 1u;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 3u);
  /* the pan branch fires 0x651F0(6)/0x974F0(0x5DC) and returns at stage 4 */
  mr.goal_zone = 0u;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 4u && mr.act.timer == 0);    /* 0x89AA8/0x89AA0 */
  /* arm 4: a high camera word holds (0x89AF4/0x89B02) */
  mr.render.camera.speed = 1;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 4u);
  mr.render.camera.speed = 0;
  mr.render.camera.event_param = 1;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 4u);
  /* both zero: arm 4 advances to 5 and falls into arm 5, which holds below
   * the 0x3C timeline floor. */
  mr.render.camera.event_param = 0;
  mr.state.tick_total = 0x3B;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 5u && mr.act.timer == 0);
  /* the 0x89AB3 tail also advances (goal_armed clear, camera speed high) */
  mr.act.stage = 3;
  mr.render.camera.speed = 1;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 4u);                         /* 0x89ADB/0x89AE0 RET */
}

/* Arm 5 (0x89B16): the shared-timeline gate. Below 0x3C holds; the middle
 * window [0x3C,0x78) is the `0x45001()` (`byte[0x1067EC] & 0xF0`) arm, which
 * has no engine cell (FU184-L3/FU188-L2 sibling) and is derived false, so it
 * holds; at/above 0x78 it advances into arm 6, which holds on actor code 0x20.
 * Mutation-RED: dropping the 0x3C floor advances early. */
static void test_phase_1D_arm5_timeline_windows(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 2;
  mr.act.id = 7;
  mr.act.stage = 5;
  mr.act.timer = 0;
  mr.state.tick_total = 0x3B;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 5u && mr.act.timer == 2);    /* 0x89B21 JL */
  mr.state.tick_total = 0x3C;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 5u);                         /* the 0x45001 window */
  mr.state.tick_total = 0x77;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 5u);
  /* >= 0x78: stage 6, then arm 6 holds while the actor still carries 0x20 */
  mr.actor_1587d4 = 1;
  mr.entities.team[0].records[1].code = 0x20u;
  mr.state.tick_total = 0x78;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 6u && mr.act.timer == 0);    /* 0x89B4A/0x89B60 */
}

/* Arm 6 (0x89B50): the actor code `+0x91` gate. A record still on code 0x20
 * (the arm-0 install) holds; the ball record `+0x44` gate is the derived ready
 * value (the record-11 `[[actor]+0x7A6]` cell has no pool home, FU-142 OL-49),
 * so a non-0x20 actor walks 6 -> 7 -> 8; the zeroed strip state then takes the
 * arm-8 no-winner reset back to stage 0. Mutation-RED: dropping the code gate
 * would advance on the installed 0x20. */
static void test_phase_1D_arm6_code_gate(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 1;
  mr.act.id = 7;
  mr.act.stage = 6;
  mr.act.timer = 4;
  mr.actor_1587d4 = 1;
  mr.entities.team[0].records[1].code = 0x20u;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 6u && mr.act.timer == 5);    /* head add 1 then hold */
  mr.act.timer = 0;
  mr.entities.team[0].records[1].code = 0x19u;
  mr.actor_1587cc[0] = 5;
  mr.actor_1587cc[1] = 7;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  /* 6 -> 7 (0x89B94 0x4BEC8 = 0) -> 8 -> the no-winner reset (stage 0). */
  assert(mr.act.stage == 0u && mr.act.timer == 0);
  assert(mr.actor_1587d4 == 7);                       /* [0x1587CC+(side^1)*4] */
}

/* Arm 8 (0x89BB6..0x89E6D): the no-winner reset. With the strip block zeroed
 * the rank compares select nothing (0x89CDF JGE inverse), so the native rebinds
 * the side's free record (0x897F4) and re-reads the opposite side's cell into
 * `[0x1587D4]` (0x89DCF), resetting the act to stage 0. The cursor rotates only
 * when the actor's side differs from the strip side; a 4 -> 5 crossing clears
 * the 5x2 flag table (0x89D45..0x89D77). Mutation-RED: a missing rebind leaves
 * the actor cell unchanged. */
static void test_phase_1D_arm8_no_winner_reset(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 7;
  mr.act.stage = 8;
  mr.actor_1587d4 = 2;
  mr.actor_1587cc[0] = 5;
  mr.actor_1587cc[1] = 7;
  mr.render.sub.current_side = 1u;               /* byte[0x1587E6] */
  mr.render.sub.cursor_raw = 4u;                 /* byte[0x1587E7] */
  for (int i = 0; i < 10; i++) mr.render.sub.flags[i] = 0xAAu;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 0u && mr.act.timer == 0); /* 0x89DBB/0x89DC8 */
  assert(mr.render.sub.cursor_raw == 5u);          /* 4 -> 5 (the %5 reset) */
  for (int i = 0; i < 10; i++) assert(mr.render.sub.flags[i] == 0u);
  assert(mr.actor_1587cc[0] == 1);                 /* 0x897F4(side 0, start -1) */
  assert(mr.actor_1587d4 == 7);                    /* [0x1587CC + (side^1)*4] */
}

/* Arm 8 winner + arm 9 terminal (0x89DDE..0x89EA5): with the cursor at/over 5
 * and the two strip numbers ordered, the winner scores `word[0x157AC5+2*idx]`
 * 1 / the other 0 (0x89DED/0x89E00), writes `[0x1587E8]` (the actor ==
 * `[0x157A9F]` flag, `mr->goal_record`), posts `0x8F188(0xBA,0x20,...)`
 * (recorded as `match1d_event_id`), advances to stage 9 and falls STRAIGHT into
 * the terminal: the act slot and `[0x157A4C]` clear in the same call.
 * Mutation-RED: dropping the fall-through leaves stage 9 installed. */
static void test_phase_1D_arm8_winner_terminal(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 7;
  mr.act.stage = 8;
  mr.act.timer = 9;
  mr.actor_1587d4 = 1;
  mr.goal_record = 1;                            /* [0x157A9F] == the actor */
  mr.global_157a4c = 1u;
  mr.render.sub.cursor_raw = 5u;                 /* the [0x1587E7] >= 5 arm */
  mr.render.sub.current_side = 1u;               /* side 0 != current side */
  mr.render.sub.record[0x10] = 2u;               /* byte[0x1587E4] */
  mr.render.sub.record[0x11] = 1u;               /* byte[0x1587E5] */
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.score[0] == 1u && mr.score[1] == 0u);   /* 0x89DED/0x89E00 */
  assert(mr.render.sub.special == 1u);              /* [0x1587E8] */
  assert(mr.match1d_event_id == 0xBAu);             /* 0x8F188(0xBA,0x20,..) */
  assert(mr.render.sub.cursor_raw == 6u);           /* 5 -> 6, no %5 clear */
  assert(mr.global_157a4c == 0u);                   /* arm 9 [0x157A4C]=0 */
  assert(mr.act.id == 0 && mr.act.stage == 0u && mr.act.timer == 0);
}

/* The full 0 -> 9 drive (arm 0 install -> arm 1 whistle -> arm 2 contact ->
 * arm 3 pan -> arm 4 clear -> arm 5 timeline -> arm 6 code -> arm 7/8 select ->
 * arm 9 terminal). One wait gate is exercised per arm and the terminal is
 * reached without any direct stage fixture. Mutation-RED: the pre-fix body
 * stalls at stage 2 forever. */
static void test_phase_1D_full_timeline_terminates(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.frame_delta = 1;
  mr.act.id = 7;
  mr.act.stage = 0;
  mr.actor_1587d4 = 1;
  /* arm 0 -> 1; arm 1 holds below 0x78 */
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 1u);
  assert(mr.entities.team[0].records[1].code == 0x20u);  /* the arm-0 install */
  /* arm 1 -> 2 -> 3, where the pan/camera gate holds */
  mr.state.tick_total = 0x78;
  mr.entities.ball.pair.actor = 1;
  mr.entities.ball.pair.ack = 1;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 3u && mr.ref_whistle == 0x1Eu);
  /* arm 3 tail -> 4, arm 4 holds while a camera word is high */
  mr.render.camera.speed = 1;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 4u);
  /* arm 4 -> 5, arm 5 holds in the 0x45001 window */
  mr.render.camera.speed = 0;
  mr.state.tick_total = 0x40;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 5u);
  /* arm 5 -> 6, arm 6 holds while the actor still carries 0x20 */
  mr.state.tick_total = 0x78;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.stage == 6u);
  /* the row-0x20 body runs elsewhere and drops the code: 6 -> 7 -> 8 -> 9 */
  mr.entities.team[0].records[1].code = 0x19u;
  mr.render.sub.cursor_raw = 5u;
  mr.render.sub.current_side = 1u;
  mr.render.sub.record[0x10] = 2u;
  mr.render.sub.record[0x11] = 1u;
  assert(fifa96_match_action_phase_1D(&mr) == FIFA96_OK);
  assert(mr.act.id == 0);                        /* the act slot cleared */
  assert(mr.act.stage == 0u && mr.act.timer == 0);
  assert(mr.global_157a4c == 0u);                /* arm 9 */
  assert(mr.global_157ac0 == 1u);                /* 0x89E8C */
  assert(mr.score[0] == 1u && mr.score[1] == 0u);
  assert(mr.match1d_event_id == 0xBAu);
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

/* Phase 0x0D (phase-36 R7): the row-1A stage-2 sit-7 emit makes
 * `set_piece(7, ...)` live; the shared reset body is the writer's per-record
 * walk. Mutation-RED: a NULL slot leaves the targets at the sentinels. */
static void test_phase_0D_wired(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x0D].fn == fifa96_match_phase_00);
  rec = &mr.entities.team[0].records[1];
  rec->pos_x = 0x111;
  rec->pos_y = 0x222;
  rec->pos_z = 0x333;
  rec->target_x = 0x444;
  rec->target_y = 0x555;
  rec->target_z = 0x666;
  mr.phase_walk_active = 1;
  mr.phase_walk_rec = rec;
  assert(fifa96_match_dispatch_phase(&mr, 0x0Du) == FIFA96_OK);
  assert(rec->target_x == 0x111);
  assert(rec->target_y == 0x222);
  assert(rec->target_z == 0x333);
  assert(fifa96_match_run_set_piece(&mr, 7u, 0u, 0u) == 0);
  assert(mr.state.phase == 0x0Du);
}

/* The phase-0x1B body `0x890EC..0x8910D` (phase-36): the four-store pure
 * act-slot reset. Mutation-RED: dropping any store leaves the staged value. */
static void test_phase_1B_body_reset(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.act.id = 5;
  mr.act.stage = 2;
  mr.act.timer = 0x2A;
  mr.state.phase = 0x11;
  assert(fifa96_match_action_phase_1B(&mr) == FIFA96_OK);
  assert(mr.act.id == 0u);
  assert(mr.act.stage == 0u);
  assert(mr.act.timer == 0);
  assert(mr.state.phase == 0x11u);       /* no phase write */
  assert(fifa96_match_action_phase_1B(NULL) == -FIFA96_ERR_INVALID);
}

/* FU-243 §4.2/§6 (M2 phase-23 T2): row 0x25 is COUNTED. The prior FU189-L2
 * "row 25 has NO static installer" verdict is REFUTED — the ECX-carried bulk
 * installer `0x8D79D`/`0x8D7AD` (`MOV ECX,0x25 -> FUN_0008CEB8`) installs code
 * 0x25 on the live `FUN_0008D098` path and is already modelled by
 * `fifa96_match_phase_machine_step` (`code = mr->score[0] != mr->score[1]
 * ? 0x25 : 3`). Only the body `0x880CC` (the FU-82 7-arm table `0x880B0`)
 * was unported. Mutation-RED: a NULL slot returns UNSUPPORTED and the
 * dispatch assertion fails. */
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
 * assertion below fails. The behavioural path is exercised here too (not a
 * slot-existence-only check, T5 strengthening): the live phase-5 active stage-0
 * record arms `scratch_ae = 0x3C` and advances to stage 1, so a slot that merely
 * returned FIFA96_OK would leave `stage92` at 0. */
static void test_action_15_dispatches(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_action_table[0x15].fn != NULL);
  assert(fifa96_match_dispatch_action(&mr, 0x15u) == FIFA96_OK);
  assert(fifa96_match_dispatch_action(NULL, 0x15u) == -FIFA96_ERR_INVALID);
  mr.record.entity_id = 1;              /* team 0 record 1 */
  mr.state.phase = 5u;                  /* the live phase-5 celebration arm */
  mr.record.stage92 = 0u;
  mr.record.active = 1u;
  assert(fifa96_match_dispatch_action(&mr, 0x15u) == FIFA96_OK);
  assert(mr.record.scratch_ae == 0x3C && mr.record.stage92 == 1u);
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
  test_act9_stage0_resets_match_cells();
  test_sit0c_install_invokes_stage0();
  test_act9_stage0_camera_and_present_bind();
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
  test_phase_01_formation_body();
  test_phase_13_inactive();
  test_phase_13_active();
  test_phase_scatter_producer();
  test_phase_13_write_seam();
  test_phase_1C_counted();
  test_phase_1C_stage0();
  test_phase_1C_terminal();
  test_phase_1D_registered();
  test_phase_1D_install_frame();
  test_phase_1D_wait_whistle();
  test_phase_1D_terminal();
  test_phase_1D_arm0_installs_row_20();
  test_phase_1D_arm2_contact_wait();
  test_phase_1D_arm3_4_camera_handshake();
  test_phase_1D_arm5_timeline_windows();
  test_phase_1D_arm6_code_gate();
  test_phase_1D_arm8_no_winner_reset();
  test_phase_1D_arm8_winner_terminal();
  test_phase_1D_full_timeline_terminates();
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
  test_phase_0D_wired();
  test_phase_1B_leg_no_invoker();
  test_phase_1B_body_reset();
  test_row_25_counted();
  test_row_25_body_direct();
  test_action_15_dispatches();
  test_action_15_body_direct();
  puts("test_engine_match_act OK");
  return 0;
}
