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
  /* FU-147 S1: the `+0x8D` active seed is the record ordinal (FUN_0008C2E0
   * 0x8C329), so only record 0 reads inactive; every other record's row-00
   * phase-2 install keeps code 3 (the 0x7DA26 coercion no longer hits). */
  assert(mr.entities.team[0].records[5].code == 3);
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
      /* 0x19/3 are excluded from the discriminator: row 00 installs them at
       * phase 2 (record 0 inactive -> 0x19; the FU-147 S1 active seed leaves
       * every other record on code 3). No arm code may appear. */
      assert(code != 0x26 && code != 0x25 && code != 0x28 && code != 0x2A);
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
  /* FU-147 S1: the driver now runs the shared mover for every dispatched
   * record, so pin the reference record against integration (target := pos)
   * to keep this fixture about the arm-2 copy. */
  mr.entities.team[0].records[5].target_x = 0x140;
  mr.entities.team[0].records[5].target_y = 0x55;
  mr.entities.team[0].records[5].target_z = 0x1E0;

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
 * the run leaves phase 1 on its own. M2 interactive Task 1: the setup bind +
 * `FUN_0007876C` merge put the live FU-70 slot on that taker, so row 01 stage
 * 1 takes the native `word[slot+6] & 0x70` arm (0x7DCE9..0x7DCF6) — the test
 * supplies the button press/release the native kickoff waits for. */
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
   * action 2, record 0 the 3 -> 0x19 inactive coercion (FU-147 S1: `+0x8D` is
   * the record ordinal, so only record 0 reads inactive); every other record
   * keeps code 3. Both team targets are the first pick. */
  assert(mr.entities.team[0].records[0].code == 0x19u);
  assert(mr.entities.team[0].records[1].code == 1u);
  assert(mr.entities.team[0].records[2].code == 2u);
  assert(mr.entities.team[0].records[10].code == 3u);
  assert(mr.entities.team[1].records[1].code == 3u);
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
   * runs stage 0 and falls into stage 1. M2 interactive Task 1 binds the live
   * slot (setup bind + FUN_0007876C merge), so the stage-1 slot arm is the
   * native `word[slot+6] & 0x70` gate: without a released button the run stays
   * at the kickoff phase (the native kickoff waits for the player). */
  for (int i = 0; i < 30; i++) one_granted_frame(&mr);   /* granted frames 31..60 */
  assert(mr.global_5882a == 1u);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  assert(mr.slot.entity == 1);                 /* the taker owns the slot */
  assert(mr.entities.team[0].records[1].has_slot == 1);
  assert(mr.entities.team[0].records[1].stage92 == 1u);
  for (int i = 0; i < 4; i++) one_granted_frame(&mr);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);   /* still gated */

  /* A button press/release supplies the FU-70 release word the slot arm reads:
   * the slot update runs inside the frame body, so the release sample reaches
   * the row on the next granted frame. */
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(mr.slot.released == 0u);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 6 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  assert(mr.state.prev_phase == 1u);
  assert(mr.phase_machine.state == 2u);
  assert(mr.phase_machine.phase == 2u);
  assert(mr.global_5882a == 1u);
  assert(mr.entities.controlled == 1);   /* row 01 stage 0 [0x157A83] = rec */
  assert(mr.entities.team[0].records[1].stage92 == 3u);
  assert(mr.entities.team[0].records[1].timer89 == 0);
  /* FU-147 S1: with the +0x8D active seed the native `0x8DE8C` skip is the
   * record's ordinal (`+0x8A>>24` = byte +0x8D), so stage 2's nearest pick no
   * longer returns the taker itself; the `0x7DF61` conditional merge moves the
   * FU-70 slot to the nearest teammate and the drain runs FUN_00078670
   * (0x7867A), clearing the release word. The no-ISO fixture puts every record
   * at the origin, so the tie pick is record 0. */
  assert(mr.slot.entity == 0);
  assert(mr.entities.team[0].records[0].has_slot == 1);
  assert(mr.entities.team[0].records[1].has_slot == 0);
  assert(mr.slot.released == 0u);      /* consumed by the handoff reset */
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
  /* M2 interactive Task 1: the bound slot makes row 01 stage 1 wait for the
   * `word[slot+6] & 0x70` release word, so pass it with a KICK press/release
   * after the 60-frame act-1 producer window. */
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 6 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
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

/* Task 1 (M2 interactive match / G1): the pad drives the controlled record.
 *
 * First-hand /FIFA96.EXE evidence:
 *  - `FUN_00078824` (`0x78824..0x7891C`) is the match-setup slot init: it
 *    clears both teams' `+0x828` and every record's `+0x20`, then binds the
 *    four `0x57C64` slots through `FUN_000785E0`; `FUN_000785E0` picks a free
 *    record with `FUN_0008DB6C(0x5774C, team, skip=-1, flag=1)`, sets
 *    `record+0x20 = slot`, `slot+0x1D = team+0x828`, `slot+0x22 = team+0x826`
 *    and `team+0x828++`.
 *  - The state-1 kickoff arm (`0x8D1B1..0x8D243`) resolves the taker through
 *    `FUN_00079CCC` and calls `FUN_0007876C` (slot merge) on it: the donor
 *    loses `+0x20`, the taker gains it.
 *  - Row 00 (`0x7DB10..0x7DBAC`) gates the move on `+0x20 != 0` and calls
 *    `FUN_00079C20(rec, (int8)[slot+0x1D]>>24, (int8)[slot+0x1E]>>24)` — the
 *    unaligned dword reads are slot bytes `+0x20`/`+0x21` (the T2/T3 direction
 *    chain `FUN_00078950` writes), giving `target = pos + dir<<7`.
 *  - `FUN_0007BF20` (FU-77) then integrates target into velocity/position for
 *    every record after its action handler.
 *
 * The test drives a begun run: begin binds the human slot (derived
 * `FUN_00078824`/`FUN_000785E0` subset) and the state-1 arm merges it onto the
 * taker; the reset code 0 is staged, UP is latched, and the granted frames run
 * row 00 + the shared mover. The no-input control run must stay still. */
static void test_pad_drives_controlled_locomotion(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_run mr2;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);

  {
    int32_t taker = mr.entities.team[0].target;
    assert(taker >= 0 && taker < (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
    assert(mr.slot.entity == taker);           /* bind + FUN_0007876C merge */
    assert(mr.entities.team[0].records[taker].has_slot == 1);
    assert(mr.entities.team[0].slot_pool == 1);
    assert(mr.entities.team[1].slot_pool == 0);

    assert(fifa96_match_entities_install(&mr.entities.team[0].records[taker],
                                         (uint8_t)mr.state.phase, 0, 0) == 1);
    {
      fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
      assert(fifa96_match_run_input(&mr, &up, 1) == 0);
    }
    int32_t start_x = mr.entities.team[0].records[taker].pos_x;
    int32_t start_z = mr.entities.team[0].records[taker].pos_z;
    for (int i = 0; i < 10; i++) one_granted_frame(&mr);
    /* UP -> slot T2/T3 dir (1, 0) -> row 00 target pos+(0x80,0) -> the mover
     * ramps vel_x and integrates pos_x; pos_z stays. */
    assert(mr.entities.team[0].records[taker].pos_x > start_x);
    assert(mr.entities.team[0].records[taker].pos_z == start_z);
    {
      /* M2 interactive T1 alias sync: the mover writes the word views, and
       * the dword views the live consumers read must mirror the native bytes
       * (+0x71/+0x73 over dword +0x71, +0x73/+0x75 over dword +0x73). */
      const struct fifa96_match_entity *r = &mr.entities.team[0].records[taker];
      assert(r->vel73 != 0);                     /* velocity ramped (word) */
      assert(r->speed71 == (int16_t)r->vel_x);   /* word +0x71 view */
      assert(r->vel73 == (int16_t)((uint32_t)r->vel_x >> 16));  /* word +0x73 */
      assert(r->vel73 == (int16_t)r->vel_z);     /* same word via dword +0x73 */
      assert(r->vel75 == (int16_t)((uint32_t)r->vel_z >> 16));  /* word +0x75 */
    }
    assert(fifa96_match_run_end(&mr) == 0);
  }

  /* Control: the same begun run with no pad leaves the record still. */
  fifa96_match_run_init(&mr2);
  assert(fifa96_match_run_begin(&mr2, f.engine, 0) == 0);
  {
    int32_t taker = mr2.entities.team[0].target;
    assert(mr2.slot.entity == taker);
    assert(fifa96_match_entities_install(&mr2.entities.team[0].records[taker],
                                         (uint8_t)mr2.state.phase, 0, 0) == 1);
    for (int i = 0; i < 10; i++) one_granted_frame(&mr2);
    assert(mr2.entities.team[0].records[taker].pos_x == 0);
    assert(mr2.entities.team[0].records[taker].pos_z == 0);
    assert(mr2.entities.team[0].records[taker].speed71 == 0);
    assert(mr2.entities.team[0].records[taker].vel_x == 0);
    assert(mr2.entities.team[0].records[taker].vel_z == 0);
    assert(fifa96_match_run_end(&mr2) == 0);
  }

  drop_fixture(f);
}

/* FU-147 S1: the per-frame driver runs the shared mover for every dispatched
 * record (native FUN_0007CA54 tail 0x7CD48 / FUN_000782D0 tail 0x785C5; the
 * pool loop keeps the +0x9A skip) and the BF20 lane block
 * (0x7C776..0x7C7AF, first-hand this slice) refreshes the +0x6B lane,
 * +0x6D/+0x6F camera-delta words, the +0x77 bound and the team
 * camera-nearest tracker (0x7C7C4..0x7C7CF) from the camera-focus stand-in
 * (leg 13: the native 0x15774C/0x157754 focus, engine render camera). */
static void test_ai_record_mover_and_lane_track(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.slot.entity != id);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);
  e->pos_x = 0x100;
  e->pos_z = 0x100;
  e->pos_y = 0;
  e->target_x = 0x300;   /* row 00 (slot-less, phase 1) leaves the target */
  e->target_y = 0;
  e->target_z = 0x100;
  int32_t start_x = e->pos_x;
  assert(e->lane_x == 0 && e->lane_z == 0 && e->bound == 0 && e->cam_dz6f == 0);
  for (int i = 0; i < 3; i++) one_granted_frame(&mr);
  int16_t prev_lane = e->lane_x;
  one_granted_frame(&mr);
  /* The mover integrated the AI record's target (no slot needed). */
  assert(e->pos_x > start_x);
  assert(e->pos_z == 0x100);
  /* The lane block wrote the camera deltas, the metric lane and the bound
   * (old lane) in the native order. */
  int16_t exp_dx = (int16_t)((uint16_t)mr.render.camera.pos_x - (uint16_t)e->pos_x);
  int16_t exp_dz = (int16_t)((uint16_t)mr.render.camera.pos_z - (uint16_t)e->pos_z);
  assert(e->lane_z == exp_dx);
  assert(e->cam_dz6f == exp_dz);
  assert(e->lane_x == (int16_t)fifa96_entity_distance((int32_t)exp_dx, (int32_t)exp_dz));
  assert(e->bound == prev_lane);
  /* The tracker is the team record with the minimal fresh lane (the running
   * replacement at 0x7C7C4/0x7C7CD keeps the incumbent on ties); skip-9A
   * records are not dispatched and keep stale lanes. */
  {
    const struct fifa96_match_team *team = &mr.entities.team[1];
    int32_t best = FIFA96_MATCH_ENTITY_NONE;
    int16_t best_lane = 0;
    assert(team->camera_nearest >= 0 && team->camera_nearest < 11);
    for (int i = 0; i < 11; i++) {
      const struct fifa96_match_entity *r = &team->records[i];
      if (r->skip_9a != 0) continue;
      if (best == FIFA96_MATCH_ENTITY_NONE || r->lane_x < best_lane) {
        best = i;
        best_lane = r->lane_x;
      }
    }
    assert(team->camera_nearest == best);
  }
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-147 S1: the row-1E claim's `+0x9B` possession flag and `[0x157A83]`
 * actor bind must leave the dispatch staging for the pool (the `+0x9B`
 * producer). The claim fires while `stage92 < 3` and the record does not hold
 * the ball; the frame chain stages the record, dispatches row 1E and drains
 * the requests. */
static void test_row1e_claim_reaches_pool(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 5;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  e = &mr.entities.team[1].records[5];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0x1E, 0) == 1);
  assert(e->has_ball == 0);
  assert(mr.entities.controlled != id);
  one_granted_frame(&mr);
  assert(e->has_ball == 1);                    /* +0x9B staged back to the pool */
  assert(mr.entities.controlled == id);        /* [0x157A83] = rec */
  /* the next dispatch sees the flag and does not re-claim (the native
   * `stage < 3 && has_ball == 0` gate) */
  one_granted_frame(&mr);
  assert(e->has_ball == 1);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-147 S1: the keeper/restart stage-3 possession flip end to end. A record
 * at the stage-3 span without the ball resets, runs situation 0xB (phase ->
 * 2) and the drained code-5 install makes it the carrier (`+0x9F` bit 0,
 * 0x7DA42). The next dispatch runs row 05 (unwired, OL-63) — the engine-level
 * claim (0x7F1FF) stays a leg; the install -> carrier bit is the reachable
 * subset. */
static void test_row1e_stage3_possession_flip(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  e = &mr.entities.team[1].records[5];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0x1E, 0) == 1);
  e->stage92 = 3;                       /* the stage-3 span dispatch */
  e->has_ball = 0;
  assert(mr.state.phase == 1u);
  one_granted_frame(&mr);
  assert(mr.state.phase == 2u);         /* situation 0xB */
  assert(e->code == 5);                 /* the drained code-5 install */
  assert(e->stage92 == 0);              /* install stages +0x92 = 0 */
  assert((e->carrier & 1u) != 0u);      /* 0x7DA42 carrier bit */
  assert(e->has_ball == 0);             /* the arm does not claim the flag */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-145 S2: the goal-mouth classifier `FUN_00070074` (first-hand
 * /FIFA96.EXE 0x70074, 47 insns). The return is the "no outside bit"
 * boolean: 1 iff z in [0xB10,0xB90), x in [-0xD0,0xD0) and y <= h(z), where
 * the z comparisons use the sign-extended low word of the 32-bit magnitude
 * (native MOVSX DX at 0x70084) while x/y read the full dwords. */
static void test_goal_zone_classifier(void) {
  assert(fifa96_match_goal_zone(0, 0, 0xB30) == 1);
  assert(fifa96_match_goal_zone(0, 0, -0xB30) == 1);      /* |z| band */
  assert(fifa96_match_goal_zone(-0xD0, 0, 0xB10) == 1);   /* inclusive edges */
  assert(fifa96_match_goal_zone(0xCF, 0, 0xB8F) == 1);
  /* z band edges: below 0xB10 and at 0xB90 set a bit */
  assert(fifa96_match_goal_zone(0, 0, 0xB0F) == 0);
  assert(fifa96_match_goal_zone(0, 0, 0xB90) == 0);
  /* x band edges: -0xD1 outside, 0xD0 outside (x >= 0xD0 sets a bit) */
  assert(fifa96_match_goal_zone(-0xD1, 0, 0xB30) == 0);
  assert(fifa96_match_goal_zone(0xD0, 0, 0xB30) == 0);
  /* y ceiling: z=0xB30 -> h=0xA0; z=0xB70 -> h=0xA0-(0x60-0x30)=0x70 */
  assert(fifa96_match_goal_zone(0, 0xA0, 0xB30) == 1);
  assert(fifa96_match_goal_zone(0, 0xA1, 0xB30) == 0);
  assert(fifa96_match_goal_zone(0, 0x70, 0xB70) == 1);
  assert(fifa96_match_goal_zone(0, 0x71, 0xB70) == 0);
  /* z word truncation: |z|=0x10B30 reads the low word 0xB30 -> inside even
   * though the 32-bit magnitude exceeds 0xB90; |z|=0x10020 reads
   * 0x20 < 0xB10 -> outside (the native MOVSX DX) */
  assert(fifa96_match_goal_zone(0, 0, 0x10B30) == 1);
  assert(fifa96_match_goal_zone(0, 0, 0x10020) == 0);
}

/* FU-145 S2: the pan armer `FUN_0007131C 0x71390..0x713F7` (first-hand
 * disassembly this slice). Gate phase in {2,0x10}: armed==0 and
 * `|camZ|_w > 0xB20 || |camX|_w > 0x730` (the native word-truncated
 * magnitudes at 0x713A6..0x713C0) arm: snapshot := camera (x,z), y := 0,
 * zone := classifier(camera triple). */
static void test_goal_arm_gates_and_snapshot(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  assert(fifa96_match_goal_arm(&mr) == 0);              /* rest position */
  assert(mr.goal_armed == 0);

  fifa96_camera_init(&mr.render.camera, 0x10, 0x20, 0xB21);
  assert(fifa96_match_goal_arm(&mr) == 1);
  assert(mr.goal_armed == 1);
  assert(mr.goal_snap_x == 0x10 && mr.goal_snap_y == 0 && mr.goal_snap_z == 0xB21);
  assert(mr.goal_zone == 1);        /* z in band, x inside, y <= h */

  /* already armed: no re-snapshot (the native jumps to the reflect gate) */
  mr.render.camera.pos_z = 0x2000;
  assert(fifa96_match_goal_arm(&mr) == 0);
  assert(mr.goal_snap_z == 0xB21);

  /* boundary values do not arm: |z| == 0xB20 and |x| == 0x730 */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  fifa96_camera_init(&mr.render.camera, 0x730, 0, 0xB20);
  assert(fifa96_match_goal_arm(&mr) == 0);
  mr.render.camera.pos_x = 0x731;
  assert(fifa96_match_goal_arm(&mr) == 1);              /* the x arm */
  assert(mr.goal_snap_x == 0x731);

  /* phase gate: 1 disarms-capable, 0x10 arms, 0x11 not */
  fifa96_match_run_init(&mr);
  mr.state.phase = 1;
  fifa96_camera_init(&mr.render.camera, 0, 0, -0xB30);
  assert(fifa96_match_goal_arm(&mr) == 0);
  mr.state.phase = 0x11;
  assert(fifa96_match_goal_arm(&mr) == 0);
  mr.state.phase = 0x10;
  assert(fifa96_match_goal_arm(&mr) == 1);
  assert(mr.goal_snap_z == -0xB30);

  /* the native truncates the absolute value to its low word before the
   * comparison (0x7136C/0x71384 store words): INT_MIN stores word 0 -> no
   * arm even though the 32-bit magnitude exceeds every bound. */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  fifa96_camera_init(&mr.render.camera, (int32_t)0x80000000, 0, 0);
  assert(fifa96_match_goal_arm(&mr) == 0);
}

/* FU-145 S2 / FU-71 §6: the already-armed reflect arm `0x718A9..0x7190E`.
 * With the zone classifier 0 and the reflect input bit 0 set, the native
 * clears both arm flags and mirrors the camera about X=±0xE40 / Z=±0x1620
 * (the ported `fifa96_camera_reflect`). Zone != 0 or a clear input bit take
 * the unported angle arm (FU-71 leg 9.6) with no clear. */
static void test_goal_arm_reflect_clear(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.goal_armed = 1;
  mr.goal_zone = 0;
  mr.render.input_bit0 = 1;
  fifa96_camera_init(&mr.render.camera, 0, 0, 0x1300);
  mr.render.camera.vel_z = 0x40;
  assert(fifa96_match_goal_arm(&mr) == 1);
  assert(mr.goal_armed == 0 && mr.goal_zone == 0);
  assert(mr.render.camera.pos_z == 0x1620 - 0x1300);
  assert(mr.render.camera.vel_z == (int16_t)(uint16_t)(0u - 0x40u));

  /* zone != 0: the angle arm, no clear, camera untouched */
  fifa96_match_run_init(&mr);
  mr.goal_armed = 1;
  mr.goal_zone = 1;
  mr.render.input_bit0 = 1;
  fifa96_camera_init(&mr.render.camera, 0, 0, 0x1300);
  assert(fifa96_match_goal_arm(&mr) == 0);
  assert(mr.goal_armed == 1 && mr.render.camera.pos_z == 0x1300);

  /* input bit clear: no clear, no mirror */
  mr.goal_zone = 0;
  mr.render.input_bit0 = 0;
  assert(fifa96_match_goal_arm(&mr) == 0);
  assert(mr.goal_armed == 1 && mr.render.camera.pos_z == 0x1300);
}

/* FU-145 S2: the clock-tail gate and scanner (`FUN_0008AF38 0x8B623..0x8B643`
 * -> `FUN_00088940`). Armed + phase 2/0x10 + `|goal_snap_z| > 0xB20` + zone
 * != 0 queues situation 6 through `fifa96_match_run_goal_queue`; the queued id
 * is 5 (side 0) / 6 (side 1) with the side from the snapshot sign (0x889B6
 * SETL; the [0x157A4C]/[0x1587D4] flag arm is leg L4). The gate-closed /
 * pending fallback takes the direct increment and the shared table-2
 * situation-6 entry. The throw-in (|snap_z| <= 0xB20) and corner (zone == 0)
 * arms stay the w7-b1 set-piece legs. */
static void test_goal_scan_queue_and_fallback(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.session_gate_14c32a = 1;                       /* live session (leg 2 seam) */
  mr.goal_armed = 1;
  mr.goal_zone = 1;
  mr.goal_snap_z = 0xB80;
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.situation_pending == 1 && mr.situation_id == 5);
  assert(mr.state.phase == 2u);                     /* queue path: no phase write */
  assert(mr.score[0] == 0 && mr.score[1] == 0);

  /* snapshot sign selects the side: negative -> record side 1 -> id 6 */
  fifa96_match_run_init(&mr);
  mr.state.phase = 0x10;
  mr.session_gate_14c32a = 1;
  mr.goal_armed = 1;
  mr.goal_zone = 1;
  mr.goal_snap_z = -0xB80;
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.situation_pending == 1 && mr.situation_id == 6);

  /* gates: phase, arm, throw-in band, corner zone */
  fifa96_match_run_init(&mr);
  mr.state.phase = 1;
  mr.goal_armed = 1;
  mr.goal_zone = 1;
  mr.goal_snap_z = 0xB80;
  assert(fifa96_match_run_goal_scan(&mr) == 0);
  mr.state.phase = 2;
  mr.goal_armed = 0;
  assert(fifa96_match_run_goal_scan(&mr) == 0);
  mr.goal_armed = 1;
  mr.goal_snap_z = 0xB20;                           /* throw-in band: leg */
  assert(fifa96_match_run_goal_scan(&mr) == 0);
  assert(mr.situation_pending == 0);
  mr.goal_snap_z = 0xB30;
  mr.goal_zone = 0;                                 /* corner arm: leg */
  assert(fifa96_match_run_goal_scan(&mr) == 0);
  assert(mr.situation_pending == 0);

  /* gate-closed fallback (begun run: the direct increment is lifecycle-gated):
   * direct score + the shared table-2 situation-6 phase-5 write. */
  {
    struct fixture f = make_fixture(10000000ull);
    struct fifa96_match_run fb;
    fifa96_match_run_init(&fb);
    assert(fifa96_match_run_begin(&fb, f.engine, 0) == 0);
    assert(fb.session_gate_14c32a == 1);            /* begin seeds the gate */
    assert(fb.goal_armed == 0 && fb.situation_pending == 0);
    fb.state.phase = 2;
    fb.session_gate_14c32a = 0;                     /* session gate closed */
    fb.goal_armed = 1;
    fb.goal_zone = 1;
    fb.goal_snap_z = 0xB80;
    assert(fifa96_match_run_goal_scan(&fb) == 1);
    assert(fb.score[0] == 1 && fb.score[1] == 0);
    assert(fb.state.phase == 5u);                   /* table-2 row 6 */
    assert(fb.situation_pending == 0);
    /* a pending situation routes to the same fallback */
    fb.state.phase = 2;
    fb.session_gate_14c32a = 1;
    fb.situation_pending = 1;
    fb.situation_id = 6;
    assert(fifa96_match_run_goal_scan(&fb) == 1);
    assert(fb.score[0] == 2);
    assert(fb.state.phase == 5u);
    assert(fifa96_match_run_end(&fb) == 0);
    drop_fixture(f);
  }
}

/* FU-145 S2 / 0x740F6: a phase-2 write clears the pan arm
 * (`FUN_000740A0`; the native clears [0x15781D] after the [0x157A4A] write,
 * and the derived write path clears the zone byte alongside for an
 * observer-clean disarm). The shared table-2 0xB row is the live phase-2
 * writer. */
static void test_goal_phase2_write_clears_arm(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 0x11;
  mr.goal_armed = 1;
  mr.goal_zone = 1;
  mr.goal_snap_z = 0xB80;
  assert(fifa96_match_run_situation(&mr, 0x0B) == 0);   /* -> phase 2 */
  assert(mr.state.phase == 2u);
  assert(mr.goal_armed == 0 && mr.goal_zone == 0);
}

/* FU-145 S2 RED core: a fixture pan drives the FU-71 camera integrator past
 * the arming bounds; the frame body arms, the snapshot freezes, and the clock
 * tail's scan queues situation 6 with the snapshot-freeze order. The native
 * pan producer (rate words 0x1577C0/C2) is leg L1/S4; the fixture uses the
 * equivalent engine seam (the camera velocity pair, as test_camera drives it).
 * Would fail on BASE: no armer, no scanner, no queue. */
static void test_goal_chain_pan_fixture(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.session_gate_14c32a == 1);    /* begin seeds the live session */
  mr.state.phase = 2;
  mr.state.period_length = 90;
  fifa96_camera_init(&mr.render.camera, 0, 0, 0xB00);
  mr.render.camera.vel_z = 0x40;          /* 0x40 * delta 2 per granted frame */
  mr.render.camera.speed = 0x40;          /* the update's integration gate */
  mr.render.camera.anchor_time = 0x7FFF;
  mr.render.camera.anchor2_time = 0x7FFF;
  one_granted_frame(&mr);
  assert(mr.goal_armed == 1);
  assert(mr.goal_snap_z == 0xB80);
  assert(mr.goal_zone == 1);
  assert(mr.situation_pending == 1 && mr.situation_id == 5);

  /* the snapshot is frozen; with no scheduler consumer yet (S3) the next
   * frame's scan sees the pending latch and takes the native fallback (the
   * direct increment + the shared table-2 phase-5 write). Pinned so the S3
   * consumer wiring visibly replaces it. */
  int32_t snap = mr.goal_snap_z;
  one_granted_frame(&mr);
  assert(mr.goal_snap_z == snap);
  assert(mr.score[0] == 1);
  assert(mr.state.phase == 5u);
  assert(mr.situation_pending == 1 && mr.situation_id == 5);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
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
  test_goal_zone_classifier();
  test_goal_arm_gates_and_snapshot();
  test_goal_arm_reflect_clear();
  test_goal_scan_queue_and_fallback();
  test_goal_phase2_write_clears_arm();
  test_goal_chain_pan_fixture();
  test_pad_drives_controlled_locomotion();
  test_ai_record_mover_and_lane_track();
  test_row1e_claim_reaches_pool();
  test_row1e_stage3_possession_flip();
  puts("test_engine_match_frame OK");
  return 0;
}
