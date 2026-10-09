/* tests/test_engine_match_frame.c — Task 13: match frame body, pacing and clock state. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

/* One match frame is one 0x200 step owed to the state library; each second is
 * 0x3C (60) of those units, so one 30 Hz frame is 2/60 s. The pace grants
 * 3 frames per 10 ticks (0x102/0x35C = 3/10), i.e. 300 granted frames in
 * exactly 1000 engine steps: 300/30 = 10 s of match clock. */
#define STEPS_FOR_10S 1000

static void test_init_resets_state(void) {
  struct fifa96_match_run mr;
  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);
  assert(mr.state.frame_acc == 0);
  assert(mr.state.frame_delta == 0);
  assert(mr.state.tick_total == 0);
  assert(mr.state.period_seconds == 0);
  assert(mr.state.total_seconds == 0);
  assert(mr.state.aux_seconds == 0);
  assert(mr.state.aux_tick == 0);
  assert(mr.state.period_length == 0);
  assert(mr.state.extra_length == 0);
  assert(mr.state.second_acc == 0);
  assert(mr.state.period == 0);
  assert(mr.state.phase == 0);
  assert(mr.state.prev_phase == 0);
  assert(mr.state.aux_flag == 0);
  /* FU-142a: the installer-arms machine is reset with the run state */
  assert(mr.phase_machine.state == 0);
  assert(mr.phase_machine.phase == 0);
  assert(mr.phase_machine.side_controlled == 0);
  assert(mr.phase_machine.arm2a_overflow == 0);
  assert(mr.phase_machine.chosen831[0] == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.phase_machine.chosen831[1] == FIFA96_MATCH_ENTITY_NONE);
}

/* 1000 granted-cadence calls deliver exactly 300 frames (10 s at 30 Hz): the
 * pace accumulator returns to zero with no fractional garbage and the fixed
 * 0x200 step never leaves a fraction in frame_acc. */
static void test_300_grants_ten_seconds_no_drift(void) {
  struct fifa96_match_run mr;
  int granted = 0;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;            /* class 1: the clock always runs */
  mr.state.period_length = 90;   /* no period end inside the 10 s window */

  for (int i = 0; i < STEPS_FOR_10S; i++) {
    int rc = fifa96_match_run_frame(&mr);
    assert(rc >= 0);
    granted += rc;
    assert(mr.state.frame_acc < 0x100u);           /* fraction < one step */
    assert(mr.pace.acc < FIFA96_MATCH_PACE_FRAME); /* pace remainder bounded */
  }

  assert(granted == 300);
  assert(mr.state.frame_delta == 2);
  assert(mr.state.frame_acc == 0);
  assert(mr.state.tick_total == 600);
  assert(mr.state.second_acc == 0);
  assert(mr.state.period_seconds == 10);
  assert(mr.state.total_seconds == 10);
  assert(mr.state.period == 0);
  assert(mr.state.phase == 2);
  assert(mr.pace.acc == 0);            /* exactly 300 * 0x35C after 1000 * 0x102 */
  assert(mr.pace.pending == 300);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
}

/* The library boundary: a 1 s first period ends on the 30th granted frame
 * (the 100th pace tick here); the frame body must mark the lifecycle over. */
static void test_period_end_marks_lifecycle_over(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  mr.state.phase = 2;
  mr.state.period_length = 1;
  mr.state.extra_length = 1;

  for (int i = 0; i < 100; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
  }

  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);
  assert(mr.state.period == 1);
  assert(mr.state.period_seconds == 0);
  assert(mr.state.total_seconds == 1);
  assert(mr.pace.pending == 30);
}

static void test_null_guard(void) {
  assert(fifa96_match_run_frame(NULL) == -FIFA96_ERR_INVALID);
}

struct fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
};

static struct fixture make_fixture(uint64_t step_ns) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = step_ns;
  struct fixture f;
  f.plat = fifa96_platform_null_create(&pcfg);
  assert(f.plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  f.engine = fifa96_engine_create(&ecfg, f.plat);
  assert(f.engine != NULL);
  assert(fifa96_engine_boot(f.engine) == 0);
  return f;
}

static void drop_fixture(struct fixture f) {
  fifa96_engine_destroy(f.engine);
  fifa96_platform_destroy(f.plat);
}

/* The frame body runs once per 100 Hz PIT tick, not once per engine step.
 * The null default step is 16666667 ns (~60 Hz): 30 steps fire 50 PIT ticks
 * (30 * 16.67 ms = 500 ms), so the pace must consume 50 ticks and grant 15
 * frames (50 * 0x102 = 15 * 0x35C) even though only 30 run steps happened.
 * Driving the frame body from run_step would grant only 9 frames here. */
static void test_engine_step_drives_frame_body(void) {
  struct fixture f = make_fixture(16666667ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.total_seconds = 99;
  mr.phase_machine.state = 0x13;      /* begin must reset the FU-142a machine */
  mr.phase_machine.arm2a_overflow = 1;

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.state.total_seconds == 0);   /* fresh match clock */
  /* The stale pre-begin mirror (0x13) is reset by begin and then carries the
   * derived kickoff entry (T2/OL-84): phase_machine.state == phase 1. */
  assert(mr.phase_machine.state == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  assert(mr.phase_machine.arm2a_overflow == 0);
  assert(mr.pace.pending == 0);

  for (int i = 0; i < 30; i++) {
    assert(fifa96_engine_step(f.engine) == 0);
  }
  assert(mr.ticks == 50);                /* one trampoline hit per PIT tick */
  assert(mr.steps == 30);                /* engine steps are NOT the cadence */
  assert(mr.pace.pending == 15);         /* 50 pace ticks * 258/860 */
  assert(mr.pace.acc == 0);
  assert(mr.state.tick_total == 30);
  /* The derived kickoff entry is phase 1 (class 0): the FU-62 clock's seconds
   * stop at kickoff (native 0x8AF41 gate), so the granted frames advance the
   * tick counter but not the second accumulator. */
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  assert(mr.state.second_acc == 0);
  assert(mr.state.total_seconds == 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);

  assert(fifa96_match_run_end(&mr) == 0);
  assert(mr.state.tick_total == 0);      /* teardown clears the match state */
  drop_fixture(f);
}

/* Period end on a begun lifecycle: begin registers the 100 Hz hook, 100 PIT
 * ticks (10 ms each) grant the 30 frames that complete the 1 s period, and
 * the frame body marks that same begun lifecycle over. The 100th tick is
 * driven through a direct frame-body call: an engine step would now resolve
 * the OVER in the same step (G1 live exit, pinned by
 * test_engine_match_completion::test_live_period_end_exits_to_frontend), and
 * this test's contract is the frame body's mark_over itself. */
static void test_begun_period_end_marks_over(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.lc.active == 1);
  assert(mr.lc.registered == 1);
  mr.state.phase = 2;
  mr.state.period_length = 1;
  mr.state.extra_length = 1;

  for (int i = 0; i < 99; i++) {
    assert(fifa96_engine_step(f.engine) == 0);
  }
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);   /* period still open */
  assert(fifa96_match_run_frame(&mr) == 1);             /* 30th grant: period end */
  assert(mr.ticks == 99);                  /* 99 trampoline hits; direct call has no tick */
  assert(mr.steps == 99);
  assert(mr.pace.pending == 30);
  assert(mr.state.period == 1);
  assert(mr.state.period_seconds == 0);
  assert(mr.state.total_seconds == 1);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER); /* mark_over, begun run */
  assert(mr.lc.active == 1);                        /* not torn down */
  assert(mr.lc.registered == 1);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-141: the frame body drives the derived entity/ball pool once per granted
 * 30 Hz frame, after the FU-70 control slot and FU-71 camera updates. Ten pace
 * ticks grant 3 frames, so both team update counters advance 3 times; the
 * records start on the native reset action 0 (FU-137 §4.3), row 00 runs and
 * its phase-2 install request is consumed by the FU-137 §2 installer (inactive
 * -> 0x19). */
static void test_frame_drives_entity_chain(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  for (int i = 0; i < 10; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
  }
  assert(mr.entities.team[0].update_count == 3);
  assert(mr.entities.team[1].update_count == 3);
  assert(mr.entities.team[0].records[0].code == 0x19);
  assert(mr.entities.team[1].records[0].code == 0x19);
  assert(mr.entities.team[0].records[5].code == 0x19);
}

/* FU-142a: the frame body runs the FUN_0008D098 state 0x13/0x14 arm block
 * once per granted frame, after the FU-141 entity chain. State/phase 0x13 with
 * side_controlled 0: team 1 (side 1) is the non-controlled side and all its
 * records stage 0x26; team 0 (controlled, ac5 == ac7) stages 3 (record 0 ->
 * 0x19). At phase 2 the hook is off: the entity chain still runs row 00 but no
 * arm code appears on any record. */
static void test_phase_machine_hook(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 0x13;
  mr.phase_machine.state = 0x13;
  mr.phase_machine.phase = 0x13;
  mr.phase_machine.side_controlled = 0;
  mr.state.period_length = 90;        /* no period end inside the 10 ticks */
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    mr.entities.team[0].records[i].active = 1; /* installer keeps code 3 */
  for (int i = 0; i < 10; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);   /* 3 granted frames */
  }
  assert(mr.entities.team[1].records[0].code == 0x26);
  assert(mr.entities.team[1].records[10].code == 0x26);
  assert(mr.entities.team[0].records[0].code == 0x19);
  assert(mr.entities.team[0].records[1].code == 3);
  assert(mr.phase_machine.arm2a_overflow == 0);

  struct fifa96_match_run mr2;
  fifa96_match_run_init(&mr2);
  mr2.state.phase = 2;                /* not the 0x13/0x14 slice */
  mr2.phase_machine.state = 2;
  mr2.state.period_length = 90;
  for (int i = 0; i < 10; i++) {
    assert(fifa96_match_run_frame(&mr2) >= 0);
  }
  for (uint32_t t = 0; t < 2; t++) {
    for (uint32_t r = 0; r < FIFA96_MATCH_ENTITY_RECORDS; r++) {
      uint8_t code = mr2.entities.team[t].records[r].code;
      /* 0x19 is excluded from the discriminator: row 00 installs it at
       * phase 2 for inactive records (the positive fixture above). */
      assert(code != 0x26 && code != 3 && code != 0x25 &&
             code != 0x28 && code != 0x2A);
    }
  }
}

/* FU-142b: the row-26 body round-trips through the FU-141 pool record staging
 * and repack (`match_run_dispatch_entity`). Both fixtures are pool team-0
 * records with code 0x26 at phase 2 (no installer arm, no period end):
 *  - record 1 (stage92 = 2): the body returns after the prologue, so the
 *    staged lane/targets round-trip unchanged while `timer89` gains one delta
 *    per granted frame (3 grants) and `timer7b` takes the staged `player_d = 0`
 *    table value (6 >> 1 = 3);
 *  - record 2 (stage92 = 0, pos.z = 10): the latch walks 0 -> 1 -> 2 in the
 *    first grant (`target.z = +6` from active 3, `lane = 6 - 10 = -4` inside
 *    the `0x20` gate -> retarget `(0xCC0, 0)`), then the prologue adds the two
 *    remaining deltas.
 * A broken staged stage92 would leave the latch at the 0xFF reset seed (or run
 * it on the pass record), a broken lane/target staging would zero the pass
 * round-trip, and an unstaged `player_d` would be 0 here but out of range if
 * garbage were passed through (the body rejects before writing). */
static void test_action_26_repack_round_trips_fields(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *pass;
  struct fifa96_match_entity *latch;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;       /* no period end inside the 10 ticks */

  pass = &mr.entities.team[0].records[1];
  pass->code = 0x26;
  pass->stage92 = 2;                 /* past the latch: prologue only */
  pass->timer89 = 100;
  pass->timer7b = 0xABCD;
  pass->lane = 0x00DEADBE;
  pass->target_x = 0x111;
  pass->target_z = 0x222;
  pass->active = 3;

  latch = &mr.entities.team[0].records[2];
  latch->code = 0x26;
  latch->stage92 = 0;
  latch->timer89 = 0;
  latch->timer7b = 0;
  latch->lane = 0;
  latch->target_x = 0x999;
  latch->target_z = 0x999;
  latch->active = 3;
  latch->pos_z = 10;                 /* target.z = +6 -> lane = -4 */

  for (int i = 0; i < 10; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);   /* 3 granted frames, delta 2 */
  }

  assert(pass->code == 0x26);                 /* staged current code */
  assert(pass->stage92 == 2);                 /* staged latch, not rewritten */
  assert(pass->timer89 == 106);               /* 100 + 3 * delta 2 */
  assert(pass->timer7b == 3);                 /* player_d staged 0 */
  assert(pass->lane == 0x00DEADBE);           /* no helper call at stage 2 */
  assert(pass->target_x == 0x111 && pass->target_z == 0x222);

  assert(latch->code == 0x26);
  assert(latch->stage92 == 2);                /* 0 -> 1 -> 2, repacked */
  assert(latch->timer89 == 4);                /* grant 1 zeroes; +2, +2 */
  assert(latch->timer7b == 3);
  assert(latch->lane == -4);                  /* target.z +6 vs pos.z 10 */
  assert(latch->target_x == 0xCC0 && latch->target_z == 0);
}

/* FU-142d: the row-28 body round-trips through the FU-141 pool record staging
 * and repack, and the pool resolves `team+0x831` into the chosen-record
 * position. Two pool team-0 records carry code 0x28 at phase 2 (no installer
 * arm, no period end); the run RNG is seeded 0 (draws 512, 1829, 4927, 11195,
 * 22605, 41818, 6755, 52849, 54403, 17912, 16644, ...).
 *  - record 1 (stage92 = 0): frame 1 runs arm 0 (target = [0x10F364/368] folds
 *    -> (144, 0)), draws 512 -> `scratch_a2 = 0x20` and latches to stage 1;
 *    frames 2-3 wait below the gate (timer89 2 -> 6) while the prologue face
 *    repacks `type` (octant 2 for the +x target).
 *  - record 2 (stage92 = 1, scratch_a2 = 1, team flag830 = 1): frame 1 fires,
 *    runs the `0x87014` six-draw setup plus the arm's four draws (a2 -72,
 *    a6 77, aa 371, ae 152, a0 0, a1 1) and latches to stage 2; frames 2-3 run
 *    arm 2's chosen-record path with team `chosen831` = record 5, so each call
 *    resets the target to record 5's position plus the two gate offsets:
 *    (0x140, 0x1E0) + (-72, 77) = (0xF8, 0x22D) with `target_y = 0x55`. The
 *    prologue distance stays above 0x20 so each call returns after the copy.
 * A broken scratch repack would re-run the setup path on frames 2-3 (changing
 * the pinned scratch cells), a broken `chosen` resolution would leave the
 * target at the staged value and a broken `type` repack would keep 0x33. */
static void test_action_28_repack_round_trips_fields(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *latch;
  struct fifa96_match_entity *chase;
  fifa96_match_run_init(&mr);
  assert(fifa96_rng_seed(&mr.rng, 0) == FIFA96_OK);
  /* phase 0: the record walk still dispatches every code, but the phase-2
   * team selection/ball pairing (which rewrites a selected record's target)
   * stays off so the row-28 target writes are observable. */
  mr.state.phase = 0;
  mr.state.period_length = 90;       /* no period end inside the 10 ticks */

  latch = &mr.entities.team[0].records[1];
  latch->code = 0x28;
  latch->stage92 = 0;
  latch->timer89 = 0;
  latch->active = 0;                 /* folds: x +144, z 0 */
  latch->type = 0x33;
  latch->target_x = 0;
  latch->target_z = 0;

  chase = &mr.entities.team[0].records[2];
  chase->code = 0x28;
  chase->stage92 = 1;
  chase->timer89 = 0;
  chase->scratch_a2 = 1;             /* fires on the first frame's delta 2 */
  chase->target_x = 0xAA;
  chase->target_z = 0xBB;
  mr.entities.team[0].flag830 = 1;

  mr.entities.team[0].chosen831 = 11 * 0 + 5;   /* record 5 of team 0 */
  mr.entities.team[0].records[5].pos_x = 0x140;
  mr.entities.team[0].records[5].pos_y = 0x55;
  mr.entities.team[0].records[5].pos_z = 0x1E0;

  for (int i = 0; i < 10; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);   /* 3 granted frames, delta 2 */
  }

  assert(latch->code == 0x28);
  assert(latch->stage92 == 1);                 /* arm 0 latch, arm 1 wait */
  assert(latch->timer89 == 6);                 /* +2 per granted frame */
  assert(latch->scratch_a2 == 0x20);           /* seed-0 draw 512 & 0x7F */
  assert(latch->target_x == 144 && latch->target_z == 0);
  assert(latch->type == 2);                    /* +x face octant repacked */

  assert(chase->code == 0x28);
  assert(chase->stage92 == 2);                 /* flag path latch */
  assert(chase->timer89 == 4);                 /* zeroed, then +2, +2 */
  assert(chase->scratch_a2 == -72);
  assert(chase->scratch_a6 == 77);
  assert(chase->scratch_aa == 371);
  assert(chase->scratch_ae == 152);
  assert(chase->scratch_a0 == 0);
  assert(chase->scratch_a1 == 1);
  assert(chase->target_x == 0xF8 && chase->target_z == 0x22D);
  assert(chase->target_y == 0x55);             /* chosen triple y */
}

/* FU-143 wiring (M2 playability Task 3): the frame body steps the derived
 * phase driver each granted frame. A live class-1 phase (2) on a 1 s period
 * reaches the derived selector-0 period end WITHOUT the fixture writing the
 * post-period phase: the FU-62 match clock's `sec == limit + aux` completion
 * (fifa96_match_state.c) is staged by the frame body, and the driver runs the
 * derived FUN_0008B9CC chooser (extra_time clear, period < 4 -> phase 0x0C on
 * the controlled side, act 0xB). `prev_phase` records the 2 -> 0x0C write.
 * A further frame must not re-fire: 0x0C is class 0 (clock stopped). */
static void test_phase_drive_reaches_period_end(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;               /* the native in-play phase (class 1) */
  mr.state.period_length = 1;
  mr.state.extra_length = 1;

  for (int i = 0; i < 100; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
  }
  assert(mr.state.period == 1);
  assert(mr.state.period_seconds == 0);
  assert(mr.state.phase == 0x0Cu);          /* derived chooser, not forced */
  assert(mr.state.prev_phase == 2u);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);

  assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.phase == 0x0Cu);          /* class 0: no second write */
  assert(mr.state.prev_phase == 2u);
}

/* The driver's class-1/class-2 gate and the one-shot staging contract, without
 * the clock: a class-0 phase (0x01) must not consume a staged completion into a
 * period-end write (the native 0x8AF41 gate stops the clock), the explicit
 * class-2 accept path (phase 0x00, the begin default; the FU-143 table's flat
 * 0x1106AD[0] class byte and the engine's clear `[0x14C302]` halt) runs the
 * chooser like class 1, and the live class-1 phase (0x02) runs the derived
 * chooser on the completed period (`mr.state.period - 1`). NULL ->
 * -FIFA96_ERR_INVALID. */
static void test_phase_drive_class_gate(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.period = 1;              /* a completed period for the chooser */
  mr.state.phase = 1;               /* class 0 */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 0);
  assert(mr.state.phase == 1u);
  assert(mr.clock_period_ended == 0u);      /* the staging is consumed */

  mr.state.phase = 2;               /* class 1 */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.state.phase == 0x0Cu);
  assert(mr.state.prev_phase == 2u);
  assert(mr.phase_machine.state == 0x0Cu);
  assert(mr.phase_machine.phase == 0x0Cu);
  assert(mr.clock_period_ended == 0u);

  /* class 2 accepts a staged completion (the halt gate is clear in the
   * engine: `fifa96_match_state_tick` runs with `clock_halt = 0`). */
  mr.state.phase = 0;               /* class 2 (the begin default) */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.state.phase == 0x0Cu);
  assert(mr.state.prev_phase == 0u);
  assert(mr.phase_machine.state == 0x0Cu);
  assert(mr.phase_machine.phase == 0x0Cu);
  assert(mr.clock_period_ended == 0u);

  /* one-shot: without a staged completion the next call is a no-op */
  assert(fifa96_match_run_phase_drive(&mr) == 0);
  assert(mr.state.phase == 0x0Cu);
  assert(fifa96_match_run_phase_drive(NULL) == -FIFA96_ERR_INVALID);
}

/* The begun-run 2 -> 0x0C -> 0 sequence: the derived write lands on the
 * completing frame (0x0C observable before the lifecycle resolve), and the
 * run_end teardown resets the match state to the native reset phase 0
 * (FUN_00073E28's derived surface, fifa96_match_state_init). */
static void test_phase_drive_begun_end_resets_phase(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  mr.state.phase = 2;
  mr.state.period_length = 1;
  mr.state.extra_length = 1;

  for (int i = 0; i < 99; i++) {
    assert(fifa96_engine_step(f.engine) == 0);
  }
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  assert(fifa96_match_run_frame(&mr) == 1);      /* 30th grant: period end */
  assert(mr.state.phase == 0x0Cu);               /* derived 2 -> 0x0C */
  assert(mr.state.prev_phase == 2u);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);

  assert(fifa96_match_run_resolve(&mr) == 1);    /* OVER -> POST -> EXIT -> end */
  assert(mr.state.phase == 0u);                  /* teardown reset -> 0 */
  assert(mr.state.prev_phase == 0u);

  drop_fixture(f);
}

/* Run ticks until exactly one granted 30 Hz frame body ran (the pace grants
 * 3 frames per 10 ticks; the loop bound is generous but deterministic). */
static void one_granted_frame(struct fifa96_match_run *mr);

/* FU-143 §10/§11 (M2 playable-match Task 2 / OL-84 residual): the natural
 * kickoff chain from the derived phase-1 entry to the live phase 2.
 *
 * First-hand native chain (/FIFA96.EXE): begin models the phase-0x17 handler
 * stage-0 `FUN_000740A0(1, side)` write (0x88E82); the same setter call runs
 * `FUN_0008D098` per team, whose state-1 arm (0x8D1B1..0x8D243) stages code 3
 * over both team blocks, resolves the record nearest the kickoff point
 * (0x8D1C6 FUN_00079CCC) and, on the controlled side, installs action 1
 * (0x8D200) on it and action 2 (0x8D238) on the next nearest. Action row 01
 * (0x7DBC0) runs each frame with the `phase == 1` gate; its stage 0 is armed by
 * `[0x5882A]`, which the act-1 handler stage 1 sets at the shared timeline
 * timer `[0x58818] >= 0x78` (0x88EF3..0x88F07) — the engine's `tick_total`
 * is the same whole-delta accumulation, so the derived producer fires at 60
 * granted frames. Stage 2 then calls `FUN_0008A938` situation 0xB (0x7DF90),
 * whose table-2 arm 0x8AEF6 -> 0x8AF02 FUN_000740A0(2, side) writes the live
 * phase 2. This test drives the begun run without any forcing: the state-1
 * arm's action 1 lands on record 1 (zero-target fixture: the 0x79CCC derived
 * pick skips index 0, ties keep the first candidate), so row 01 runs on it and
 * the run leaves phase 1 on its own. */
static void test_kickoff_enters_phase2_naturally(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE); /* derived entry */
  assert(mr.state.prev_phase == 0u);
  assert(mr.phase_machine.state == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  assert(mr.phase_machine.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  assert(mr.global_5882a == 0u);

  /* The state-1 arm landed at begin: record 1 carries action 1, record 2
   * action 2, the rest the 3 -> 0x19 inactive coercion; both team targets are
   * the first pick. */
  assert(mr.entities.team[0].records[0].code == 0x19u);
  assert(mr.entities.team[0].records[1].code == 1u);
  assert(mr.entities.team[0].records[2].code == 2u);
  assert(mr.entities.team[0].records[10].code == 0x19u);
  assert(mr.entities.team[1].records[1].code == 0x19u);
  assert(mr.entities.team[0].target == 1);
  assert(mr.entities.team[1].target == 12);
  assert(mr.entities.phase == 1u);

  /* 30 granted frames (tick_total 60 < 0x78): the derived act-1 producer has
   * not fired, so row 01 waits at its [0x5882A] gate and the run stays at the
   * kickoff-placement phase (class 0: the FU-62 clock seconds stop). */
  for (int i = 0; i < 30; i++) one_granted_frame(&mr);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  assert(mr.global_5882a == 0u);
  assert(mr.state.tick_total == 60u);
  assert(mr.state.total_seconds == 0);

  /* The producer fires at tick_total >= 0x78 (60 granted frames); row 01 then
   * runs stage 0/1/2 and the derived situation 0xB writes the live phase 2.
   * Native cadence: stage 0 waits 0x3C of its own timer89 (30 frames) and
   * stage 1's no-slot arm waits another 0x78 (60 frames), so phase 2 lands
   * around granted frame 121. */
  for (int i = 0; i < 170 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  assert(mr.state.prev_phase == 1u);
  assert(mr.phase_machine.state == 2u);
  assert(mr.phase_machine.phase == 2u);
  assert(mr.global_5882a == 1u);
  assert(mr.entities.controlled == 1);   /* row 01 stage 0 [0x157A83] = rec */
  assert(mr.entities.team[0].records[1].stage92 == 3u);
  assert(mr.entities.team[0].records[1].timer89 == 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* C3-OL2 (M2 playability Task 4): the derived FUN_00093944 score-event writer
 * wired as the live run's score source. The native callers are the unported
 * period-indexed goal-screen handlers (first-hand census, FU-142 App. I.10:
 * 11 sites at 0x93D98..0x9486E; no wired action/phase row contains one), so
 * the run exposes the derived source on the same live-run API the M2 tape's
 * score step uses. The carried tracked-side default is -1 (the native
 * FUN_00092D8C producer reads the unported team+0x828 flags, OL-87): the
 * source is then the FU-72 increment + last-side record. Staging the tracked
 * side exercises the full bookkeeping and the 0x9B/0x9E posts; the derived
 * loader branches are fixtured in test_action_handlers.c. */
static void test_score_event_wired_run_path(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.score_tracked_side == -1);
  assert(mr.score_last_side == -1);
  assert(mr.score_max_diff == 0);
  assert(mr.score_last_event == 0);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.score[0] == 0 && mr.score[1] == 0);

  /* Goal 1: the carried -1 default is the plain increment + last side. */
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 1 && mr.score[1] == 0);
  assert(mr.score_last_side == 0 && mr.score_last_event == 0);
  assert(mr.score_max_diff == 0);

  /* Goals 2-3 with the tracked side staged: 3-0 posts the native 0x9B. */
  mr.score_tracked_side = 0;
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 2 && mr.score_last_event == 0);
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 3 && mr.score_last_event == 0x9B);

  /* Goal 4 on the non-tracked arm: 4-0 with the tracked side flipped to 1 is
   * own 4, other 0 -> the native 0x9E. */
  mr.score_tracked_side = 1;
  assert(fifa96_match_run_score_event(&mr, 0, 3) == 0);
  assert(mr.score[0] == 4 && mr.score_last_event == 0x9E);

  /* The live frame body still runs with the updated score (rendering stays
   * off, so no parity claim over the unmodeled screen/presentation paths). */
  assert(fifa96_match_run_frame(&mr) >= 0);

  assert(fifa96_match_run_score_event(NULL, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_score_event(&mr, 2, 0) == -FIFA96_ERR_INVALID);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);

  struct fifa96_match_run dead;
  fifa96_match_run_init(&dead);
  assert(fifa96_match_run_score_event(&dead, 0, 0) == -FIFA96_ERR_STATE);
}

/* G3 (M2 playable-match Task 3 / OL-87/88/89): the derived FUN_0008A938
 * situation seam is the table-2 phase dispatcher, not the score writer. The
 * goal situation 6 natively queues through the table-1 arm (`0x8A9E8`: queued
 * id 5 for side 0, 6 for side 1, `[0x15B6C0]=1`) and is consumed only by the
 * unported period-indexed goal-screen handlers (0x93BBC..0x946C4); the
 * dispatcher's table-2 row 6 (`0x8AC28`) writes phase 5 with the score/stat
 * tables unported. The engine models the table-2 arm, so a goal-situation
 * dispatch must leave the FUN_00093944 writer cells (score pair, last side,
 * tracked side, max diff, last event) untouched. */
static void test_goal_situation_dispatch_is_not_the_writer(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_last_side == -1 && mr.score_tracked_side == -1);
  assert(mr.score_max_diff == 0 && mr.score_last_event == 0);

  /* Situation 6 (goal): phase 5, no score write. */
  assert(fifa96_match_run_situation(&mr, 6) == 0);
  assert(mr.state.phase == 5u);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_last_side == -1 && mr.score_tracked_side == -1);
  assert(mr.score_max_diff == 0 && mr.score_last_event == 0);

  /* The goal-adjacent table-2 rows are phases only: 2 -> 3, 3 -> 4, 4 -> 8,
   * 5 -> 9, 0xB -> 2 (row 01's ported path); the writer cells never move. */
  assert(fifa96_match_run_situation(&mr, 2) == 0 && mr.state.phase == 3u);
  assert(fifa96_match_run_situation(&mr, 3) == 0 && mr.state.phase == 4u);
  assert(fifa96_match_run_situation(&mr, 4) == 0 && mr.state.phase == 8u);
  assert(fifa96_match_run_situation(&mr, 5) == 0 && mr.state.phase == 9u);
  assert(fifa96_match_run_situation(&mr, 0x0B) == 0 && mr.state.phase == 2u);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_last_side == -1 && mr.score_tracked_side == -1);
  assert(mr.score_max_diff == 0 && mr.score_last_event == 0);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* G3 reachability probe (M2 playable-match Task 3 / OL-87/88/89): a match
 * driven naturally into the live phase 2 (state-1 arm + wired row 01) runs the
 * ported gameplay rows with no goal invoker. The native producer chain stays
 * unported -- the camera-pan arming (`FUN_0007131C 0x713A6..0x713F7`) that
 * sets `[0x15781D]`/`[0x15781E]`, the clock scan call (`FUN_0008AF38 0x8B63E`)
 * and `FUN_00088940` (the only situation-6 producer, `0x88B44`) -- and so do
 * the consumers (scheduler `FUN_000948AC` at `0x4B1A1`, the installed
 * period-indexed handlers `[0x15B6D4]`). The score pair and the derived writer
 * cells therefore stay fresh over a natural gameplay window: the negative
 * result pinned here. */
static void test_natural_phase2_never_scores(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  for (int i = 0; i < 170 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  for (int i = 0; i < 300; i++) {
    one_granted_frame(&mr);
    assert(mr.score[0] == 0 && mr.score[1] == 0);
    assert(mr.score_last_side == -1 && mr.score_tracked_side == -1);
    assert(mr.score_max_diff == 0 && mr.score_last_event == 0);
  }
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* Run ticks until exactly one granted 30 Hz frame body ran (the pace grants
 * 3 frames per 10 ticks; the loop bound is generous but deterministic). */
static void one_granted_frame(struct fifa96_match_run *mr) {
  for (int i = 0; i < 16; i++) {
    int rc = fifa96_match_run_frame(mr);
    assert(rc >= 0);
    if (rc == 1) return;
  }
  assert(!"no granted frame within 16 ticks");
}

/* FU-142e: row 2A round-trips through the pool staging/repack at phase 0 (the
 * record walk dispatches every code; no installer arm and no phase-2 target
 * rewrite). Record 1 (stage92 0): the first granted frame runs arm 0 -> target
 * (-0x720, 0), team flag830 0, globals 0, latch 1, timer89 0. The next frames
 * wait at the arm-1 distance gate (staged from pos/target: 0x720 > 0x20),
 * adding delta 2 each. Poking pos = target stages distance 0 and fires arm 1:
 * id 0x61, flag830 1 (repacked to the team), target (-0x540, 0), latch 2.
 * Stage 9 with pos = target runs arm 9: target = pos, velocity zero and
 * [0x10F358] = 1 (repacked to the run). Stage 10 with timer89 + delta reaching
 * 0x708 runs arm 10: target (0xCC0, 0) and [0x10F35C] = 1 (repacked). A missing
 * distance staging breaks the arm-1/9 gates; a missing flag830/global repack
 * fails the assertions. */
static void test_action_2A_repack_round_trips_fields(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rush;
  fifa96_match_run_init(&mr);
  mr.state.phase = 0;
  mr.state.period_length = 90;       /* no period end inside the loop */
  rush = &mr.entities.team[0].records[1];
  rush->code = 0x2A;
  rush->stage92 = 0;
  rush->timer89 = 0;
  rush->target_x = 0x111;
  rush->target_z = 0x222;
  mr.entities.team[0].flag830 = 1;

  one_granted_frame(&mr);                          /* arm 0 */
  assert(rush->stage92 == 1);
  assert(rush->target_x == -0x720 && rush->target_z == 0);
  assert(rush->timer89 == 0);
  assert(mr.entities.team[0].flag830 == 0);        /* arm-0 clear repacked */
  assert(mr.global_10f358 == 0 && mr.global_10f35c == 0);

  one_granted_frame(&mr);                          /* arm 1 waits */
  assert(rush->stage92 == 1 && rush->timer89 == 2);
  one_granted_frame(&mr);
  assert(rush->timer89 == 4 && rush->target_x == -0x720);

  rush->pos_x = -0x720;                            /* staged distance 0 */
  rush->pos_z = 0;
  one_granted_frame(&mr);                          /* arm 1 fires */
  assert(rush->stage92 == 2);
  assert(rush->target_x == -0x540 && rush->target_z == 0);
  assert(rush->timer89 == 0);
  assert(mr.entities.team[0].flag830 == 1);        /* arm-1 set repacked */

  rush->stage92 = 9;                               /* arm 9 syncs and sets */
  rush->pos_x = 0x44; rush->pos_y = 0x55; rush->pos_z = 0x66;
  rush->target_x = 0x44; rush->target_y = 0x55; rush->target_z = 0x66;
  rush->vel_x = 7; rush->vel_z = 8;
  one_granted_frame(&mr);
  assert(rush->stage92 == 10);
  assert(rush->target_x == 0x44 && rush->target_z == 0x66);
  assert(rush->vel_x == 0 && rush->vel_z == 0);
  assert(mr.global_10f358 == 1);                   /* arm-9 global repacked */

  rush->stage92 = 10;                              /* arm 10 timer gate */
  rush->timer89 = 0x706;                           /* + delta 2 = 0x708 */
  rush->target_x = 0; rush->target_z = 0;
  one_granted_frame(&mr);
  assert(rush->stage92 == 11);
  assert(rush->target_x == 0xCC0 && rush->target_z == 0);
  assert(mr.global_10f35c == 1);                   /* arm-10 global repacked */
}

int main(void) {
  test_init_resets_state();
  test_300_grants_ten_seconds_no_drift();
  test_period_end_marks_lifecycle_over();
  test_null_guard();
  test_engine_step_drives_frame_body();
  test_begun_period_end_marks_over();
  test_frame_drives_entity_chain();
  test_phase_machine_hook();
  test_action_26_repack_round_trips_fields();
  test_action_28_repack_round_trips_fields();
  test_action_2A_repack_round_trips_fields();
  test_phase_drive_reaches_period_end();
  test_phase_drive_class_gate();
  test_phase_drive_begun_end_resets_phase();
  test_kickoff_enters_phase2_naturally();
  test_score_event_wired_run_path();
  test_goal_situation_dispatch_is_not_the_writer();
  test_natural_phase2_never_scores();
  puts("test_engine_match_frame OK");
  return 0;
}
