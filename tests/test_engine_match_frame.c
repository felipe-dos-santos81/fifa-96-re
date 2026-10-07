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
  assert(mr.phase_machine.state == 0);   /* fresh installer-arms machine */
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
  assert(mr.state.second_acc == 30);
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
  puts("test_engine_match_frame OK");
  return 0;
}
