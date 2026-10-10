/* tests/test_engine_match_frame.c — Task 13: match frame body, pacing and clock state. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_match_handlers.h"
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

/* M2 phase-9 T1: the live held-key movement is only reachable with the
 * formation-seeded kickoff (the ISO `352ko.fmt` seed places the second kickoff
 * pick on record 9, where the derived slot lands). Without the ISO the test
 * skips, the project's ISO-gated convention. */
#define T1_ISO_PATH "game/FIFAPCCD96.iso"

static int t1_iso_available(void) {
  FILE *iso = fopen(T1_ISO_PATH, "rb");
  if (!iso) return 0;
  fclose(iso);
  return 1;
}

static struct fixture make_fixture_iso(uint64_t step_ns) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = step_ns;
  struct fixture f;
  f.plat = fifa96_platform_null_create(&pcfg);
  assert(f.plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.iso_path = T1_ISO_PATH;
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  f.engine = fifa96_engine_create(&ecfg, f.plat);
  assert(f.engine != NULL);
  assert(fifa96_engine_boot(f.engine) == 0);
  return f;
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
   * FU-70 slot to the nearest teammate (the no-ISO tie pick is record 0) and
   * the drain runs FUN_00078670 (0x7867A), clearing the release word. The
   * same frame's record-2 dispatch then runs the ported row 02 (M2 phase-9
   * T1/FU-75 L4.6): phase 2, no slot, `[team+0x828] != 0` -> the `0x7E041
   * FUN_0007876C` request attaches the slot to the second kickoff pick
   * (record 2), exactly as the native immediate call would. */
  assert(mr.slot.entity == 2);
  assert(mr.entities.team[0].records[0].has_slot == 0);
  assert(mr.entities.team[0].records[1].has_slot == 0);
  assert(mr.entities.team[0].records[2].has_slot == 1);
  assert(mr.entities.team[0].records[2].code == 2u);   /* phase-2 arm waits */
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
  /* T4 (OL-89): the FUN_0009252C display-gate cells carry the image defaults
   * ([0x115FCC]/[0x114A98] = 0; the FUN_000A7FD4/FUN_000A8172 producers are
   * legs) and no event has been dispatched. */
  assert(mr.score_sound_device == 0);
  assert(mr.score_sound_midi == 0);
  assert(mr.score_display_event == 0);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_sound_device == 0 && mr.score_display_event == 0);

  /* Goal 1: the carried -1 default is the plain increment + last side. */
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 1 && mr.score[1] == 0);
  assert(mr.score_last_side == 0 && mr.score_last_event == 0);
  assert(mr.score_max_diff == 0);
  assert(mr.score_display_event == 0);

  /* Goals 2-3 with the tracked side staged: 3-0 posts the native 0x9B. The
   * dispatch gate is closed at the image defaults, so nothing dispatches. */
  mr.score_tracked_side = 0;
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 2 && mr.score_last_event == 0);
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 3 && mr.score_last_event == 0x9B);
  assert(mr.score_display_event == 0);

  /* Goal 4 on the non-tracked arm: 4-0 with the tracked side flipped to 1 is
   * own 4, other 0 -> the native 0x9E. The staged sound-device cell (the
   * unported FUN_000A7FD4 producer is a leg) opens the FUN_000A80E2 gate, so
   * the post dispatches (FUN_0009252C -> the derived
   * `score_display_event`; the FUN_00066724 sink is the OL-89 leg). */
  mr.score_tracked_side = 1;
  mr.score_sound_device = 1;
  assert(fifa96_match_run_score_event(&mr, 0, 3) == 0);
  assert(mr.score[0] == 4 && mr.score_last_event == 0x9E);
  assert(mr.score_display_event == 0x9E);

  /* 5-0 and 6-0 hit no post arm: the dispatch observation clears. */
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 5 && mr.score_last_event == 0);
  assert(mr.score_display_event == 0);
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 6 && mr.score_last_event == 0);

  /* A playing stream ([0x114A98] != 0) closes the gate: 7-0 posts 0x9F but
   * does not dispatch. */
  mr.score_sound_midi = 1;
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 7 && mr.score_last_event == 0x9F);
  assert(mr.score_display_event == 0);

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
 * sets `[0x15781D]`/`[0x15781E]` and the clock scan call
 * (`FUN_0008AF38 0x8B63E` -> `FUN_00088940`, the only situation-6 producer,
 * `0x88B44`) -- so a natural run never queues a goal. The consumers are now
 * ported (FU-146 S3: the `0x4B1A1` scheduler, the installed period handlers
 * `[0x15B6D4]`), but with nothing queued the score pair and the derived writer
 * cells stay fresh over a natural gameplay window: the negative result pinned
 * here (the screen timer stays under the 900-unit rollover, so the handler's
 * tail never fires). */
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

/* T3 (FU-75 §1.2/§1.3 + FU-137 §4.1): the pad kick reaches the kick machine
 * through the outfield record machine's input-row dispatch.
 *
 * First-hand /FIFA96.EXE this task: the slot-bound outfield record (records
 * 1..10 run FUN_0007CA54; record 0 is the keeper machine) selects its
 * dispatch code from the slot edge words — `byte[rec+0x91] == 5` (the action
 * code, `0x7CB36`) selects code 1 — and runs the released table, whose
 * `{mask 0x07FF, want 0x10, handler 0x7D110}` row installs action 7
 * (`0x7D13B..0x7D150`: phase 2, the `0x110680[byte[rec+0x91]] & 1` gate,
 * `EDX=7`, `ECX=1` invoke-now) — the ported row-07 kick machine. The native
 * invokes the new handler immediately and the record-machine tail calls it
 * again in the same frame (`0x7CD29 CALL [rec+0x18]`), so row 07 runs its
 * stage 0 and stage 1 (the kick) on the release frame while the FU-70
 * released word is still live in `word[slot+6]` (the stage-1 mode source).
 * The engine's KICK release therefore lands `code == 7` on the carrier and
 * the same frame's double dispatch runs the ported kick machine, latching the
 * actor and the mode into the FU-73 ball pair. */
static void test_pad_kick_release_runs_kick_row(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  /* M2 phase-9 T1: the natural kickoff reaches live phase 2 (the same path as
   * test_natural_phase2_never_scores); there the ported row 02 (the second
   * kickoff pick, code 02) takes the FU-70 slot and its invoked row 04 stages
   * the carrier state 5 (`0x7F133` install 5 invoke-now, unconditional). */
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
  for (int i = 0; i < 6; i++) one_granted_frame(&mr);   /* row 02/04 settle */

  {
    int32_t taker = mr.slot.entity;
    struct fifa96_match_entity *rec;
    assert(taker == 2);
    rec = &mr.entities.team[0].records[taker];
    assert(rec->has_slot == 1u);
    /* Stage the carrier state through the real installer (the row-04 coda
     * reaches code 5 only once the record is moving; the pad path itself is
     * what this test pins). */
    assert(fifa96_match_entities_install(rec, 2, 5, 0) == 1);
    assert(rec->code == 5u);

    /* KICK press: the code-1 pressed table has only the 0x80 row, so the
     * press installs nothing. */
    {
      fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
      assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(rec->code == 5u);

    /* Release: the FU-70 machine reports `slot.released = 0x10` on this
     * frame's update; the input-row dispatch matches the code-1 released row
     * and installs action 7 (invoke-now), so the same granted frame runs the
     * ported kick machine. */
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    one_granted_frame(&mr);
    assert(rec->code == 7u);
    assert(mr.dispatched_ok & (1ull << 0x07u));
    assert(rec->stage92 == 2u);                 /* stage 0 + same-frame stage 1 */
    /* The kick ran through the FU-73 pairing: the actor latched and the mode
     * is the live FU-70 released word (0x10), the native stage-1 source. */
    assert(mr.entities.ball.pair.actor == taker);
    assert(mr.entities.ball.pair.flags == 0x10u);
    assert(mr.entities.ball.pair.code != 0u);   /* an event row was staged */

    /* Releasing without a further edge changes nothing: the kick is a
     * one-shot install (the record sits at the post-kick stage). */
    one_granted_frame(&mr);
    assert(rec->code == 7u);
    assert(rec->stage92 == 2u);
  }

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* M2 phase-9 T1 / FU-75 L4.1: the record machine's phase-2 forced decision is
 * applied. The native tail `0x7CC82..0x7CD24` runs `FUN_0007C990`
 * (`fifa96_outfield_forced_action`) behind the per-type gate
 * `flat[0x110680+type]&1` and installs its code through `FUN_0007D9A4` (no
 * invoke); the T3 seam computed `out.forced` but discarded it. Here the bound
 * slot record carries code 3 (the type gate passes) and is its team's
 * `[team+0x7B2]` controlled entity with no carrier anywhere, so the forced
 * decision is code 4 — installed the same frame, and the frame's tail dispatch
 * runs row 04, whose unconditional `0x7F133` coda installs the carrier code 5
 * (invoke-now). The observed net `3 -> 5` therefore requires the applied
 * forced install: on BASE the flag was computed but not applied, the record
 * kept code 3, row 04 never dispatched and the code stayed 3. */
static void test_machine_forced_decision_installs_on_slot_record(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  e = &mr.entities.team[0].records[1];
  e->has_slot = 1;
  e->active = 1;
  e->code = 3;                       /* flat[0x110680+3] & 1 != 0 */
  mr.slot.entity = 1;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 1;    /* [team+0x7B2] = rec -> controlled */
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 1;        /* keeps team_select_target off */
  one_granted_frame(&mr);
  assert(e->code == 5u);             /* forced 4 -> row 04's coda 5 */
  assert((mr.dispatched_ok & (1ull << 0x04u)) != 0u);   /* the same tail ran row 04 */
}

/* M2 phase-9 T1 review: the team-second forced arm reads the ball bit of
 * `[team+0x7B2]` (native `FUN_0007C990 0x7CA13`: `ctrl = [team+0x7B2]`,
 * `byte[ctrl+0x9F] & 1`), NOT `[0x157A83]` (`mr->entities.controlled`). The
 * slot-bearing record is the team's `second` (never the target) and
 * `controlled` points at a third record with the opposite carrier bit, so the
 * two mappings disagree: target ball bit clear -> forced code 4 (install, row
 * 04 dispatches) vs target ball bit set -> forced code 3 == current (no
 * install, no row 04). The pre-fix source (controlled = `[0x157A83]`) inverts
 * both cases. */
static void test_machine_second_forced_reads_team_target_ball_bit(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *slot_rec;
  struct fifa96_match_entity *target_rec;
  struct fifa96_match_entity *other_rec;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  slot_rec = &mr.entities.team[0].records[1];
  target_rec = &mr.entities.team[0].records[2];
  other_rec = &mr.entities.team[0].records[3];
  slot_rec->has_slot = 1;
  slot_rec->active = 1;
  slot_rec->code = 3;                /* flat[0x110680+3] & 1 != 0 */
  mr.slot.entity = 1;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 2;    /* [team+0x7B2]: the native ball-bit source */
  mr.entities.team[0].second = 1;    /* [team+0x7B6]: the slot record */
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 3;        /* [0x157A83]: the pre-fix wrong source */

  /* target ball bit clear, controlled record's bit set -> native code 4. */
  target_rec->carrier = 0;
  other_rec->carrier = 1;
  one_granted_frame(&mr);
  assert((mr.dispatched_ok & (1ull << 0x04u)) != 0u);   /* row 04 via the forced 4 */
  assert(slot_rec->code != 3u);      /* 4 / the row-04 coda 5 */

  /* target ball bit set, controlled record's bit clear -> native code 3. */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  slot_rec = &mr.entities.team[0].records[1];
  target_rec = &mr.entities.team[0].records[2];
  other_rec = &mr.entities.team[0].records[3];
  slot_rec->has_slot = 1;
  slot_rec->active = 1;
  slot_rec->code = 3;
  mr.slot.entity = 1;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 2;
  mr.entities.team[0].second = 1;
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 3;
  target_rec->carrier = 1;
  other_rec->carrier = 0;
  one_granted_frame(&mr);
  assert(slot_rec->code == 3u);      /* forced 3 == current: no install */
  assert((mr.dispatched_ok & (1ull << 0x04u)) == 0u);
}

/* M2 phase-9 T1 / FU-75 L4.2: the no-edge arm. With a bound slot, both raw
 * edge words zero, `slot[+0x10] & 0xF0 != 0`, phase 2 and
 * `0x30 < (int16)(lane>>16) < 0x90`, the native `0x7CC70` copies the camera
 * triple `0x5774C/50/54` into `rec+0x4D/+0x51/+0x55` and runs
 * `FUN_00079B58` (`rec+0x93 = 0x10`). A held KICK supplies the `+0x10`
 * previous-mapped word (the FU-70 slot update's `prev_mapped`): the first
 * frame's press edge runs the scan, the second frame has no edges and takes
 * the arm. On BASE `slot_word10` was staged 0, so the arm never fired. */
static void test_machine_no_edge_arm_copies_camera_target(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  e = &mr.entities.team[0].records[1];
  e->has_slot = 1;
  e->active = 1;
  e->code = 3;
  e->timer81 = 1;
  e->lane = 0x40 << 16;              /* 0x30 < lane < 0x90 */
  mr.slot.entity = 1;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 2;    /* rec is not the team target */
  mr.entities.controlled = 1;        /* keeps team_select_target off */
  assert(fifa96_camera_init(&mr.render.camera, 0x111, 0x222, 0x333) == FIFA96_OK);
  mr.input_state[0] = 0x10;          /* KICK held: mapped +0x10, no direction */
  one_granted_frame(&mr);            /* press edge: scan, no arm */
  assert(e->target_x == 0 && e->target_z == 0);
  one_granted_frame(&mr);            /* no edges + prev_mapped 0x10: the arm */
  assert(e->target_x == 0x111);
  assert(e->target_y == 0x222);
  assert(e->target_z == 0x333);
  assert(e->timer93 == 0x10u);       /* 0x79B58 (+0x99 staged 0) */
}

/* M2 phase-9 T1 / FU-75 L4.6 (FU-138 OL-18): the live held-key movement on the
 * real frame path. The formation-seeded kickoff leaves the FU-70 slot on team
 * 0 record 9 (`locomotion_restart_target`, first-hand probe: code 02, vel 0),
 * because the row-01 stage-2 nearest/merge moved it there. With row 02 ported
 * the first phase-2 frame installs 4 (invoke-now), row 04 writes the slot-dir
 * target `pos + dir<<7`, and the shared mover integrates the position; holding
 * RIGHT supplies the FU-70 slot direction bytes (0,-1), so the record's z
 * moves. On BASE the record stays code 02 with zero velocity (the T3 f-up8
 * probe). The no-input control stays still while still reaching code 4. */
static void test_held_key_moves_live_controlled_record(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int32_t id;
  int32_t start_x;
  int32_t start_z;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  id = mr.slot.entity;
  assert(id == 9);                   /* the formation-seeded second pick */
  rec = &mr.entities.team[0].records[9];
  assert(rec->has_slot == 1u);
  assert(rec->code == 2u);           /* row 02 carries the slot, unported on BASE */
  start_x = rec->pos_x;
  start_z = rec->pos_z;
  {
    fifa96_platform_key right = {FIFA96_ENGINE_KEY_RIGHT, 1};
    assert(fifa96_match_run_input(&mr, &right, 1) == 0);
  }
  for (int i = 0; i < 30; i++) one_granted_frame(&mr);
  assert(rec->code == 4u);           /* row 02's install-4 invoke */
  assert((mr.dispatched_ok & (1ull << 0x04u)) != 0u);
  assert(rec->vel75 != 0);           /* the mover ramped the z velocity */
  assert(rec->pos_z < start_z);      /* RIGHT -> slot dir (0,-1) */
  assert(rec->pos_x == start_x);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);

  /* Control: the same kickoff with no held key reaches code 4 (the install
   * does not depend on input) but the zero slot direction leaves the record
   * still. */
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  rec = &mr.entities.team[0].records[9];
  start_x = rec->pos_x;
  start_z = rec->pos_z;
  for (int i = 0; i < 30; i++) one_granted_frame(&mr);
  assert(rec->code == 4u);
  assert(rec->pos_x == start_x && rec->pos_z == start_z);
  assert(rec->vel73 == 0 && rec->vel75 == 0);
  assert(fifa96_match_run_end(&mr) == 0);
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
    assert(team->tracker7c7 >= 0 && team->tracker7c7 < 11);
    for (int i = 0; i < 11; i++) {
      const struct fifa96_match_entity *r = &team->records[i];
      if (r->skip_9a != 0) continue;
      if (best == FIFA96_MATCH_ENTITY_NONE || r->lane_x < best_lane) {
        best = i;
        best_lane = r->lane_x;
      }
    }
    assert(team->tracker7c7 == best);
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
  mr.goal_snap_z = 0xB20;                           /* throw-in band: sit 2 */
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.state.phase == 3u);                     /* BX=1 fallback: phase 0 -> phase 3 */
  assert(mr.situation_pending == 0);
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.goal_armed = 1;
  mr.goal_snap_z = -0xB30;                          /* corner: cond false */
  mr.goal_zone = 0;
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.state.phase == 4u);
  assert(mr.corner_count[1] == 1u);                 /* side 0 ^ 1 = 1 */
  assert(mr.situation_pending == 0);

  /* gate-closed fallback (begun run: the direct increment is lifecycle-gated):
   * direct score + the shared table-2 situation-6 phase-5 write. FU-146 S3:
   * begin also installs the goal-screen machine, whose native installer latches
   * [0x15B6C0]=1 (FUN_00092E2C 0x92DCD); the machine clears it at the
   * phase-clear step once play starts. The direct arm skips the phase-5 write
   * when [0x157AC2] is 2 or 3 (0x8AD96/0x8AD9F). */
  {
    struct fixture f = make_fixture(10000000ull);
    struct fifa96_match_run fb;
    fifa96_match_run_init(&fb);
    assert(fifa96_match_run_begin(&fb, f.engine, 0) == 0);
    assert(fb.session_gate_14c32a == 1);            /* begin seeds the gate */
    assert(fb.goal_armed == 0 && fb.situation_pending == 1);
    fb.state.phase = 2;
    fb.session_gate_14c32a = 0;                     /* session gate closed */
    fb.goal_armed = 1;
    fb.goal_zone = 1;
    fb.goal_snap_z = 0xB80;
    assert(fifa96_match_run_goal_scan(&fb) == 1);
    assert(fb.score[0] == 1 && fb.score[1] == 0);
    assert(fb.state.phase == 5u);                   /* table-2 row 6 */
    assert(fb.situation_pending == 1);              /* the fallback leaves the latch */
    /* a pending situation routes to the same fallback */
    fb.state.phase = 2;
    fb.session_gate_14c32a = 1;
    fb.situation_pending = 1;
    fb.situation_id = 6;
    assert(fifa96_match_run_goal_scan(&fb) == 1);
    assert(fb.score[0] == 2);
    assert(fb.state.phase == 5u);
    /* the [0x157AC2] in {2,3} skip: score still increments, no phase write */
    fb.state.phase = 2;
    fb.session_gate_14c32a = 0;
    fb.global_157ac2 = 2;
    assert(fifa96_match_run_goal_scan(&fb) == 1);
    assert(fb.score[0] == 3 && fb.state.phase == 2u);
    fb.global_157ac2 = 3;
    assert(fifa96_match_run_goal_scan(&fb) == 1);
    assert(fb.score[0] == 4 && fb.state.phase == 2u);
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

/* ===== FU-149 P1: set pieces & restarts ==================================== */

/* Stage an eligible pool for the phase-arm pick: both teams' records 1..10
 * active with distinct targets (the pick skips index 0 and the +0x98/+0x9A
 * exclusions, `FUN_00079CCC`). */
static void match_frame_stage_restarts(struct fifa96_match_run *mr) {
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *e = &mr->entities.team[t].records[i];
      e->active = 1;
      e->target_x = (int32_t)((t * FIFA96_MATCH_ENTITY_RECORDS + i) * 0x100);
      e->target_z = 0;
      e->pos_x = e->target_x;
      e->pos_z = 0;
    }
  }
}

/* FU-149 §1.1/§3 item 2 (first-hand 0x8A938..0x8A996 + the 0x8A8E0 queue
 * table): the dispatcher head gates and table-1 queue ids. With the session
 * gate open and no pending situation every listed situation queues and RETs:
 * sit 2/3/4 -> 9 (side 0) / 0 (side 1); 5/7 -> 7; 6 -> 5/6; 9/10 -> 1 (side 1)
 * / 2 (side 0); 8, sit 1, 0xC and >10 -> 0xA (the native `SUB ECX,2`/`CMP
 * CX,8` underflow/default path 0x8AA60). `sit_side_pending` = (side == 0) is
 * latched first (0x8A982). The queue route writes no phase; sit 0/0xB and the
 * closed-gate/pending cases take the direct path. */
static void test_set_piece_queue_ids(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.session_gate_14c32a = 1;                      /* live session */
  assert(fifa96_match_run_set_piece(&mr, 2, 0, 0) == 0);
  assert(mr.situation_id == 9u && mr.situation_pending == 1u);
  assert(mr.sit_side_pending == 1u);
  assert(mr.state.phase == 0u);                    /* queue: no phase write */
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 2, 1, 0) == 0);
  assert(mr.situation_id == 0u && mr.sit_side_pending == 0u);
  assert(mr.situation_pending == 1u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 3, 1, 0) == 0);
  assert(mr.situation_id == 0u && mr.situation_pending == 1u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 4, 0, 0) == 0);
  assert(mr.situation_id == 9u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 5, 1, 0) == 0);
  assert(mr.situation_id == 7u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 7, 0, 0) == 0);
  assert(mr.situation_id == 7u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 6, 0, 0) == 0);
  assert(mr.situation_id == 5u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 6, 1, 0) == 0);
  assert(mr.situation_id == 6u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 9, 0, 0) == 0);
  assert(mr.situation_id == 2u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 9, 1, 0) == 0);
  assert(mr.situation_id == 1u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 10, 1, 0) == 0);
  assert(mr.situation_id == 1u);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 8, 0, 0) == 0);
  assert(mr.situation_id == 0x0Au);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 1, 0, 0) == 0);   /* kickoff: default */
  assert(mr.situation_id == 0x0Au);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 0x0C, 0, 0) == 0);
  assert(mr.situation_id == 0x0Au);
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 0x0D, 0, 0) == 0); /* >10: default id */
  assert(mr.situation_id == 0x0Au && mr.situation_pending == 1u);
  /* sit 0 and 0xB always take the direct path (0x8A944/0x8A94E) */
  mr.situation_pending = 0; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 0, 0, 0) == 0);
  assert(mr.state.phase == 0x11u && mr.situation_pending == 0u);
  assert(fifa96_match_run_set_piece(&mr, 0x0B, 0, 0) == 0);
  assert(mr.state.phase == 2u);
  /* a pending situation forces the direct route (0x8A964) */
  mr.situation_pending = 1; mr.situation_id = 0;
  assert(fifa96_match_run_set_piece(&mr, 2, 1, 0) == 0);
  assert(mr.state.phase == 3u);
  /* the closed gate forces the direct route (0x8A957) */
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_set_piece(&mr, 3, 1, 0) == 0);
  assert(mr.state.phase == 4u);
  /* the direct path for situations >= 0x0D keeps the hardened rejection (the
   * queue default id 0xA is pinned above) */
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_set_piece(&mr, 0x0D, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_set_piece(NULL, 2, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_set_piece(&mr, 2, 2, 0) == -FIFA96_ERR_INVALID);
}

/* FU-149 §1.1/§3 item 2: the BX!=0 fallback (0x8AA80..0x8AAA3): phase
 * `FUN_000740A0(0, 0)`, the act-8 replay bytes `[0x15882C] = side` /
 * `[0x15882B] = situation`, then act 8's tail (0x8A8A5..0x8A8DE, byte-exact)
 * re-dispatches the stored situation/side with BX=0 and writes
 * `[0x15882B] = 0xFF`. The port compresses the act-8 timeline (leg L6) to the
 * re-dispatch point, so the table-2 row lands on the same call. */
static void test_set_piece_bx_fallback(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_set_piece(&mr, 2, 1, 1) == 0);
  assert(mr.state.phase == 3u);                    /* sit 2 -> phase 3 */
  assert(mr.store_15882c == 1u && mr.store_15882b == 0xFFu);
  /* the phase-0 write precedes the re-dispatch (observable via prev_phase) */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  assert(fifa96_match_run_set_piece(&mr, 3, 1, 1) == 0);
  assert(mr.state.phase == 4u && mr.state.prev_phase == 0u);
  assert(mr.corner_count[1] == 1u);                /* counted with the stored side */
  assert(mr.store_15882b == 0xFFu);
  /* keeper restart through the fallback: sit 5 -> phase 9 + keeper 0x1E */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  assert(fifa96_match_run_set_piece(&mr, 5, 0, 1) == 0);
  assert(mr.state.phase == 9u);
  assert(mr.entities.team[0].records[0].code == 0x1Eu);
}

/* FU-149 §1.5/§2 (first-hand arm table 0x8D040 + `FUN_0008D098` cases):
 * phase 3 installs action 3 over both teams and the controlled team's
 * nearest-to-snapshot record gets taker 0x10 and `[team+0x7B2]`; phase 4 adds
 * the counter, the FUN_0007D360 corner probe camera reset and taker 0x11 plus
 * one RNG draw (0x92AC8 & 3); phase 8/9 install the keeper 0x1D/0x1E on the
 * controlled team's record 0. The non-controlled team early-returns (install
 * 3 only). */
static void test_set_piece_phase_arm_codes(void) {
  struct fifa96_match_run mr;
  struct fifa96_rng before;
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  assert(fifa96_match_run_set_piece(&mr, 2, 0, 0) == 0);
  assert(mr.state.phase == 3u);
  assert(mr.entities.team[0].target == 1);                  /* team 0 record 1 */
  assert(mr.entities.team[0].records[1].code == 0x10u);
  assert(mr.entities.team[0].records[2].code == 3u);
  assert(mr.entities.team[1].records[1].code == 3u);        /* install 3 only */
  assert(mr.entities.team[1].target == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.render.camera.pos_x == 0 && mr.render.camera.pos_z == 0);

  /* phase 4: probe = FUN_0007D360(snapshot) = (±0x710, 0, ±0xB00). Park a
   * record on the probe so the pick is exact. */
  fifa96_match_run_init(&mr);
  (void)fifa96_rng_seed(&mr.rng, 0);
  match_frame_stage_restarts(&mr);
  mr.goal_snap_x = 5;                                       /* + -> +0x710 */
  mr.goal_snap_z = -5;                                      /* - -> -0xB00 */
  mr.entities.team[0].records[7].target_x = 0x710;
  mr.entities.team[0].records[7].target_z = -0xB00;
  before = mr.rng;
  assert(fifa96_match_run_set_piece(&mr, 3, 0, 0) == 0);
  assert(mr.state.phase == 4u);
  assert(mr.corner_count[0] == 1u && mr.corner_count[1] == 0u);
  assert(mr.render.camera.pos_x == 0x710);
  assert(mr.render.camera.pos_z == -0xB00);
  assert(mr.entities.team[0].target == 7);
  assert(mr.entities.team[0].records[7].code == 0x11u);
  assert(memcmp(&mr.rng, &before, sizeof before) != 0);     /* the &3 draw */

  /* phase 3 consumes no RNG (the draw is phase-4 only) */
  fifa96_match_run_init(&mr);
  (void)fifa96_rng_seed(&mr.rng, 0);
  match_frame_stage_restarts(&mr);
  before = mr.rng;
  assert(fifa96_match_run_set_piece(&mr, 2, 0, 0) == 0);
  assert(memcmp(&mr.rng, &before, sizeof before) == 0);

  /* phase 8 (goal kick): keeper record 0 = 0x1D, `[team+0x7B2] = 0` */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  assert(fifa96_match_run_set_piece(&mr, 4, 0, 0) == 0);
  assert(mr.state.phase == 8u);
  assert(mr.entities.team[0].records[0].code == 0x1Du);
  assert(mr.entities.team[0].target == 0);
  assert(mr.entities.team[1].records[0].code == 0x19u);     /* 3 -> 0x19, record 0 */
}


/* FU-149 L13/T1: the P1-armed taker rows execute and resolve. The dispatcher
 * arms the throw-in (sit 2 BX=1 -> phase 3 + taker 0x10) and the corner
 * (sit 3 -> phase 4 + taker 0x11); the armed record is driven through the
 * dispatch seam with the whole-frame delta staged and the rows hand back to
 * phase 2 through the shared situation-0xB entry with the 0x12C
 * offside-suppression timer. */
static void test_taker_armed_rows_resolve(void) {
  struct fifa96_match_run mr;
  /* throw-in: sit 2 through the BX=1 fallback (the direct path with a pending
   * situation); the phase-3 arm picks team 0 record 1 */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.state.phase = 2;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  assert(fifa96_match_run_set_piece(&mr, 2, 0, 1) == 0);
  assert(mr.state.phase == 3u);
  assert(mr.entities.team[0].target == 1);
  assert(mr.entities.team[0].records[1].code == 0x10u);
  mr.record.entity_id = 1;
  mr.record.code = 0x10u;
  mr.record.active = 1;
  mr.record.frame = 4;                  /* the +0x3D animation-frame gate */
  mr.record.delta = 0x80;
  mr.record.stage92 = 0;
  assert(fifa96_match_dispatch_action(&mr, 0x10) == FIFA96_OK);
  assert(mr.record.stage92 == 2u);      /* the stage-2 delivery probe */
  assert(fifa96_match_dispatch_action(&mr, 0x10) == FIFA96_OK);
  assert(mr.state.phase == 2u);         /* the situation-0xB hand-back */
  assert(mr.referee.offside_suppress == 0x12C);
  assert(mr.record.stage92 == 5u);

  /* corner: sit 3 through the table-2 row (closed session gate); the phase-4
   * arm probes (0x710, 0xB00) and picks team 0 record 7 */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.state.phase = 2;
  assert(fifa96_match_run_set_piece(&mr, 3, 0, 0) == 0);
  assert(mr.state.phase == 4u);
  assert(mr.corner_count[0] == 1u);
  assert(mr.entities.team[0].target == 7);
  assert(mr.entities.team[0].records[7].code == 0x11u);
  mr.record.entity_id = 7;
  mr.record.code = 0x11u;
  mr.record.active = 1;
  mr.record.delta = 0x40;
  mr.record.stage92 = 0;
  assert(fifa96_match_dispatch_action(&mr, 0x11) == FIFA96_OK);
  assert(mr.record.stage92 == 5u);      /* 0x3C wait + probe placement */
  assert(fifa96_match_dispatch_action(&mr, 0x11) == FIFA96_OK);
  assert(mr.record.stage92 == 5u);      /* timer 0x40 < 0x78 */
  assert(fifa96_match_dispatch_action(&mr, 0x11) == FIFA96_OK);
  assert(mr.state.phase == 2u);
  assert(mr.referee.offside_suppress == 0x12C);
  assert(mr.record.stage92 == 8u);      /* the +0x44 wait after the kick */
}

/* FU-149 §1.8 (first-hand 0x8ABF3..0x8AC1C): the sit-3 counter index is
 * `FUN_000741B4(side) = (side ^ byte[0x157ABE]) & 1` — the display/score slot
 * swap, not the raw side. The queue route never counts (0x8A99E RETs before
 * the table row) and begin resets the pair (`FUN_00073EE0` 0x73F6A/0x73F9C
 * zeroes 0x157AD4/0x157AD6). Keeper restarts: sit 5 -> phase 9 arm 0x8D5D9
 * (0x1E on record 0); sit 7 -> phase 0xD arm 0x8D65D (code 0 over the
 * controlled team's records 1..10, record 0 kept; 0..10 on the other team). */
static void test_corner_counter_side_swap(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.side_swap = 1;
  assert(fifa96_match_run_set_piece(&mr, 3, 0, 0) == 0);    /* index 0^1 = 1 */
  assert(mr.state.phase == 4u);
  assert(mr.corner_count[1] == 1u && mr.corner_count[0] == 0u);
  assert(fifa96_match_run_set_piece(&mr, 3, 1, 0) == 0);    /* index 1^1 = 0 */
  assert(mr.corner_count[0] == 1u && mr.corner_count[1] == 1u);

  fifa96_match_run_init(&mr);
  mr.session_gate_14c32a = 1;
  assert(fifa96_match_run_set_piece(&mr, 3, 0, 0) == 0);
  assert(mr.situation_id == 9u && mr.corner_count[0] == 0u);
  /* the side-less shared entry (row 01 0xB / the goal fallback) keeps the
   * phase outcome only: no dispatcher side exists to index the counter */
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_situation(&mr, 3) == 0);
  assert(mr.state.phase == 4u && mr.corner_count[0] == 0u);

  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      mr.entities.team[t].records[i].code = 0x22;
  assert(fifa96_match_run_situation(&mr, 5) == 0);          /* sit 5 -> phase 9 */
  assert(mr.state.phase == 9u);
  assert(mr.entities.team[0].records[0].code == 0x1Eu);
  assert(mr.entities.team[1].records[0].code == 0x19u);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      mr.entities.team[t].records[i].code = 0x22;
  assert(fifa96_match_run_situation(&mr, 7) == 0);          /* sit 7 -> phase 0xD */
  assert(mr.state.phase == 0x0Du);
  assert(mr.entities.team[0].records[0].code == 0x22u);     /* controlled: record 0 kept */
  assert(mr.entities.team[0].records[1].code == 0u);
  assert(mr.entities.team[0].records[10].code == 0u);
  assert(mr.entities.team[1].records[0].code == 0u);

  /* begin resets the counter pair (native FUN_00073EE0) */
  {
    struct fixture f = make_fixture(10000000ull);
    struct fifa96_match_run fb;
    fifa96_match_run_init(&fb);
    fb.corner_count[0] = 3;
    fb.corner_count[1] = 4;
    fb.side_swap = 7;
    assert(fifa96_match_run_begin(&fb, f.engine, 0) == 0);
    assert(fb.corner_count[0] == 0u && fb.corner_count[1] == 0u);
    assert(fb.side_swap == 0u);
    assert(fifa96_match_run_end(&fb) == 0);
    drop_fixture(f);
  }
}

/* ===== FU-150 P2: fouls / referee / offside ================================ */

/* Advance the run RNG so the next contact's 1-in-8 skip draw passes and the
 * severity draw lands in the kind-1 band (seed 0: draw 4 = 0x2BBB & 7 = 3,
 * draw 5 = 0x584D & 0x3F = 0x0D). */
static void match_frame_rng_arm_hard_foul(struct fifa96_match_run *mr) {
  uint16_t value;
  (void)fifa96_rng_seed(&mr->rng, 0);
  for (int i = 0; i < 3; i++) (void)fifa96_rng_step(&mr->rng, &value);
}

/* FU-150 §Port contract: the soft-foul chain. Contact with settings 0xA == 1
 * leaves kind 0; the decision routes situation 9 BX=1; the direct path (a
 * pending situation forces 0x8A964) starts the derived act-2 hand-off whose
 * stage 0 writes phase 0xA on the fouled side; the next granted step runs the
 * FK/penalty decision. A |incident x| >= 0x420 point forces free kick 7 (and
 * the FU-149 phase-7 taker 0x12 on the controlled team). */
static void test_engine_referee_contact_fk(void) {
  struct fifa96_match_run mr;
  const int32_t point[3] = {0x500, 0x777, 0};
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;                            /* live in-play phase */
  mr.config.field_4c306 = 1;                     /* soft: kind stays 0 */
  mr.session_gate_14c32a = 1;                    /* live session */
  mr.situation_pending = 1;                      /* force the direct path */
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  assert(fifa96_match_run_contact(&mr, 1, 5, 6, point) == 1);
  assert(mr.referee.contact_kind == 1u);         /* the row-0x0C re-call kind */
  assert(mr.referee.recall_consumed == 1u);
  assert(mr.referee.foul_kind == 0u);
  assert(mr.referee.rec_first == 5 && mr.referee.rec_second == 6);
  assert(mr.incident_x == 0x500 && mr.incident_z == 0);
  assert(mr.referee.sequence == FIFA96_REF_SEQ_NONE);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_RESTART);
  assert(mr.ref_restart_stage == 1u);            /* stage 0 ran immediately */
  assert(mr.state.phase == 0x0Au);               /* act-2 stage 0 phase write */
  assert(mr.ref_whistle == 0x1Eu);               /* the soft-foul whistle */
  /* the act-2 decision step: phase 7 + the FK taker on the controlled team */
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.state.phase == 7u);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  assert(mr.ref_speech == 0x2Au);
  assert(mr.entities.team[1].target == 12);
  assert(mr.entities.team[1].records[1].code == 0x12u);  /* FK on the fouled side */
  assert(mr.entities.team[0].records[5].code == 3u);     /* the install-3 prefix */

  /* the penalty fork: |x| < 0x420 and the fouler-side band [-0xB10,-0x7B0] */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 1;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  {
    const int32_t spot[3] = {0x100, 0, -0x800};
    assert(fifa96_match_run_contact(&mr, 1, 5, 6, spot) == 1);
  }
  assert(mr.state.phase == 0x0Au);
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.state.phase == 6u);
  assert(mr.ref_speech == 0x23u);
  assert(mr.entities.team[1].records[1].code == 0x13u);  /* penalty taker */
  assert(mr.entities.team[1].target == 12);
  assert(mr.entities.team[0].records[0].code == 0x1Fu);  /* other keeper */
  assert(mr.entities.team[0].target == 0);
}


/* FU-149 L13/T1: the referee-armed free-kick and penalty takers resolve. The
 * FU-150 contact chain arms phase 7 (FK, taker 0x12) / phase 6 (penalty, taker
 * 0x13) on the fouled side; the armed record is driven to the shared
 * situation-0xB hand-back (no offside timer on the penalty row). */
static void test_taker_armed_referee_rows_resolve(void) {
  struct fifa96_match_run mr;
  const int32_t point[3] = {0x500, 0x777, 0};
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 1;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  assert(fifa96_match_run_contact(&mr, 1, 5, 6, point) == 1);
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.state.phase == 7u);
  assert(mr.entities.team[1].records[1].code == 0x12u);
  mr.record.entity_id = 12;             /* team 1 record 1 */
  mr.record.code = 0x12u;
  mr.record.active = 1;
  mr.record.delta = 0x80;
  mr.record.stage92 = 0;
  assert(fifa96_match_dispatch_action(&mr, 0x12) == FIFA96_OK);
  assert(mr.record.stage92 == 2u);      /* the incident placement + probe */
  assert(fifa96_match_dispatch_action(&mr, 0x12) == FIFA96_OK);
  assert(mr.state.phase == 2u);
  assert(mr.referee.offside_suppress == 0);   /* the FK row sets no timer */
  assert(mr.record.stage92 == 5u);

  /* penalty fork: the spot band selects phase 6 + taker 0x13 */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 1;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  {
    const int32_t spot[3] = {0x100, 0, -0x800};
    assert(fifa96_match_run_contact(&mr, 1, 5, 6, spot) == 1);
  }
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.state.phase == 6u);
  assert(mr.entities.team[1].records[1].code == 0x13u);
  mr.record.entity_id = 12;
  mr.record.code = 0x13u;
  mr.record.active = 1;
  mr.record.delta = 0x100;
  mr.record.stage92 = 0;
  mr.record.pos_x = 0;
  mr.record.pos_y = 0;
  mr.record.pos_z = -0x7E0;             /* the side-1 spot -0xF0 */
  mr.record.target_x = 0;
  mr.record.target_y = 0;
  mr.record.target_z = -0x7E0;          /* the stage-0 commit keeps the spot */
  assert(fifa96_match_dispatch_action(&mr, 0x13) == FIFA96_OK);
  assert(mr.record.stage92 == 2u);      /* the spot aim distance passes */
  assert(fifa96_match_dispatch_action(&mr, 0x13) == FIFA96_OK);
  assert(mr.record.stage92 == 5u);      /* waiting on the ball actor/ack gate */
  mr.entities.ball.pair.ack = 1;
  assert(fifa96_match_dispatch_action(&mr, 0x13) == FIFA96_OK);
  assert(mr.state.phase == 2u);
  assert(mr.referee.offside_suppress == 0);   /* the penalty row sets no timer */
  assert(mr.record.stage92 == 6u);
}

/* The settings-0 gate and the 1-in-8 skip: no decision, no machine, no phase. */
static void test_engine_referee_contact_gates(void) {
  struct fifa96_match_run mr;
  const int32_t point[3] = {0x500, 0, 0};
  uint16_t value;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 0;                     /* fouls disabled */
  (void)fifa96_rng_seed(&mr.rng, 0);
  assert(fifa96_match_run_contact(&mr, 1, 5, 6, point) == 0);
  assert(mr.referee.recall_consumed == 0u);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  assert(mr.state.phase == 2u);
  /* settings on, draw 1 & 7 == 0 -> the 1-in-8 skip (0x8AEA6/0x81EAB) */
  mr.config.field_4c306 = 2;
  assert(fifa96_match_run_contact(&mr, 1, 5, 6, point) == 0);
  assert(mr.referee.recall_consumed == 0u);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  assert(fifa96_match_run_contact(NULL, 1, 5, 6, point) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_contact(&mr, 1, -1, 6, point) == -FIFA96_ERR_INVALID);
  (void)value;
}

/* FU-150 §Port contract: the hard-foul chain through the phase-0x19 machine.
 * The kind-1 decision starts ACT3 and runs stage 0 immediately (whistle,
 * foul counter, phase 0xF on the fouled side, install action 0x16); the later
 * steps reach stage 2 speech and stage 6's situation 0xA hand-off, which (with
 * the pending latch) starts the act-2 machine at stage 1 and the same call
 * decides the penalty phase 6 from the staged band. */
static void test_engine_referee_foul_sequence(void) {
  struct fifa96_match_run mr;
  const int32_t point[3] = {0x100, 0, -0x800};
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 2;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  assert(fifa96_match_run_contact(&mr, 2, 5, 6, point) == 1);
  assert(mr.referee.foul_kind == 1u);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_FOUL);
  assert(mr.referee.stage == 1u);                /* stage 0 ran immediately */
  assert(mr.state.phase == 0x0Fu);               /* phase 0xF on the fouled side */
  assert(mr.ref_whistle == 0x1Eu);
  assert(mr.referee.fouls_by_side[0] == 1u);
  assert(mr.entities.team[0].records[5].code == 0x16u);  /* install 0x16 */
  /* stage 1 gate -> stage 2 speech */
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.referee.stage == 2u);
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.ref_speech == 0x0Du && mr.referee.stage == 3u);
  assert(fifa96_match_run_referee_step(&mr) == 1);   /* stage 3 -> 4 */
  assert(mr.referee.stage == 4u);
  assert(fifa96_match_run_referee_step(&mr) == 1);   /* stage 4: sum 1 -> 6 */
  assert(mr.referee.stage == 6u);
  assert(mr.referee.severity[0][5] == 1u);
  /* stage 6: situation 0xA -> the act-2 hand-off decides phase 6 in one call */
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  assert(mr.state.phase == 6u);
  assert(mr.ref_speech == 0x23u);
  assert(mr.entities.team[1].records[1].code == 0x13u);
  assert(mr.entities.team[1].target == 12);
  assert(mr.entities.team[0].records[0].code == 0x1Fu);

  /* the no-cards negative: the sum >= 2 path enters stage 5, which stalls
   * while the fouler is not held and never decrements the team count (E10:
   * no booking state exists; leg 6) */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 2;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  mr.referee.severity[0][5] = 1;                 /* sum >= 2 -> install 0x18 */
  mr.referee.team_count[0] = 9;                  /* kind 2 (acc != 0) */
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  assert(fifa96_match_run_contact(&mr, 2, 5, 6, point) == 1);
  assert(mr.referee.foul_kind == 2u);
  for (int i = 0; i < 4; i++) assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.referee.stage == 5u);
  assert(mr.entities.team[0].records[5].code == 0x18u);  /* the downed install */
  assert(fifa96_match_run_referee_step(&mr) == 1);       /* held 0: stall */
  assert(mr.referee.stage == 5u && mr.referee.team_count[0] == 9u);
  mr.referee.rec_first_held = 1;
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.referee.team_count[0] == 8u && mr.referee.stage == 6u);
}

/* Stage the offside geometry: the receiver (team 0 record 3) beyond the own
 * nearest (record 1) and the team-1 last defender (record 2), all other
 * records parked far from the ball so the nearest queries are deterministic. */
static void match_frame_stage_offside(struct fifa96_match_run *mr) {
  match_frame_stage_restarts(mr);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      mr->entities.team[t].records[i].pos_x = 0x6000;
      mr->entities.team[t].records[i].pos_z = 0x6000;
    }
  mr->entities.ball.x = 0;
  mr->entities.ball.z = 0;
  mr->entities.team[0].records[3].pos_x = 0x100;
  mr->entities.team[0].records[3].pos_z = 0x3C0;
  mr->entities.team[0].records[1].pos_x = 0;
  mr->entities.team[0].records[1].pos_z = 0x3C0;
  mr->entities.team[1].records[2].pos_x = 0;
  mr->entities.team[1].records[2].pos_z = 0x3C0;
}

/* FU-150 §Port contract: the offside reception chain. The derived pool query
 * resolves the own-team nearest (record 1) and the opponent last defender
 * (team 1 record 2), the kind-3 event fires on the own-nearest record, the
 * phase-0x1C machine writes phase 0xA side 0 and then dispatches situation 9,
 * and the act-2 hand-off forces the free kick phase 7 (contact kind 3). */
static void test_engine_referee_offside_chain(void) {
  struct fifa96_match_run mr;
  struct fifa96_ref_metric metric;
  uint8_t offside = 0xAA;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c2f2 = 1;                     /* offside enabled */
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;                      /* force the direct path */
  match_frame_stage_offside(&mr);
  (void)fifa96_rng_seed(&mr.rng, 0);             /* draw 1 = 0x200, tol 0 */
  metric.tol2 = 0;
  metric.side_gate = 1;
  assert(fifa96_match_run_offside_reception(&mr, 3, &metric, 0, 0, &offside) == 1);
  assert(offside == 1u);
  assert(mr.referee.contact_kind == 3u);
  assert(mr.referee.rec_first == 1);             /* the own-nearest record */
  assert(mr.referee.point[0] == 0 && mr.referee.point[2] == 0x3C0);
  assert(mr.incident_x == 0 && mr.incident_z == 0x3C0);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_OFFSIDE);
  assert(mr.state.phase == 0x0Au);               /* offside stage 0 */
  assert(mr.ref_whistle == 0x1Eu);
  assert(mr.ref_speech == 0x15u);                /* the kind-3 0x8F188(0x15) */
  /* stage 1 gate, then stage 2 dispatches situation 9 -> the act-2 hand-off */
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.referee.stage == 2u);
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_RESTART);
  assert(mr.state.phase == 0x0Au);               /* act-2 stage 0 on return */
  assert(fifa96_match_run_referee_step(&mr) == 1);
  assert(mr.state.phase == 7u);                  /* kind 3 forces free kick */
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  assert(mr.ref_speech == 0x2Au);
  assert(mr.entities.team[1].records[1].code == 0x12u);  /* FK on the fouled side */
  assert(mr.entities.team[1].target == 12);

  /* gates: settings off, suppression timer, the camera mirror gate */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  match_frame_stage_offside(&mr);
  (void)fifa96_rng_seed(&mr.rng, 0);
  assert(fifa96_match_run_offside_reception(&mr, 3, &metric, 0, 0, &offside) == 0);
  assert(offside == 0u && mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  mr.config.field_4c2f2 = 1;
  mr.referee.offside_suppress = 0x12C;
  assert(fifa96_match_run_offside_reception(&mr, 3, &metric, 0, 0, &offside) == 0);
  assert(offside == 0u);
  mr.referee.offside_suppress = 0;
  assert(fifa96_match_run_offside_reception(&mr, 3, &metric, 0x991, 1, &offside) == 0);
  assert(offside == 0u);
  /* re-seed: the mirror-blocked call consumed the tolerance draw */
  (void)fifa96_rng_seed(&mr.rng, 0);
  assert(fifa96_match_run_offside_reception(&mr, 3, &metric, 0x991, 0, &offside) == 1);
  assert(offside == 1u);
  assert(fifa96_match_run_offside_reception(NULL, 3, &metric, 0, 0, &offside) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_offside_reception(&mr, -1, &metric, 0, 0, &offside) ==
         -FIFA96_ERR_INVALID);
  /* the suppression countdown (0x7438D..0x743A5) drives from the frame delta */
  mr.state.frame_delta = 2;
  assert(mr.referee.offside_suppress == 0u);
  mr.referee.offside_suppress = 0x20;
  (void)fifa96_match_run_referee_step(&mr);
  assert(mr.referee.offside_suppress == 0x1Eu);
}

/* FU-149 §1.3 (first-hand scanner arms 0x88B53..0x88C0E): the clock-tail
 * scan's throw-in (|snap z| <= 0xB20 -> sit 2, BX=1) and corner/goal-kick
 * (|snap z| > 0xB20, zone 0 -> sit 3 + ((snap z < 0) == (ball team == 1)),
 * BX=0) arms, both with `EDX = ball team ^ 1` from [0x1577CA]. Both are
 * phase-2-only (0x88B5B/0x88BD4); the goal arm keeps the phase 2/0x10 gate. */
static void test_scan_restart_arms(void) {
  struct fifa96_match_run mr;
  /* |snap z| <= 0xB20: throw-in to the ball team's opponent */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.state.phase = 2;
  mr.goal_armed = 1;
  mr.entities.controlled = (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 3);  /* team 1 */
  mr.goal_snap_x = 0;
  mr.goal_snap_z = 0xB20;
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.state.phase == 3u);                             /* sit 2 -> phase 3 */
  assert(mr.entities.team[0].records[1].code == 0x10u);     /* ball 1 ^ 1 = 0 */
  assert(mr.corner_count[0] == 0u && mr.corner_count[1] == 0u);

  /* corner: cond false (snap z positive, ball team 1) -> sit 3 */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.state.phase = 2;
  mr.goal_armed = 1;
  mr.entities.controlled = (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 3);
  mr.goal_snap_z = 0xB30;
  mr.goal_zone = 0;
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.state.phase == 4u);
  assert(mr.corner_count[0] == 1u);

  /* goal kick: cond true (snap z negative, ball team 1) -> sit 4 -> phase 8
   * + keeper 0x1D on the controlled team's record 0 */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.state.phase = 2;
  mr.goal_armed = 1;
  mr.entities.controlled = (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 3);
  mr.goal_snap_z = -0xB30;
  mr.goal_zone = 0;
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.state.phase == 8u);
  assert(mr.entities.team[0].records[0].code == 0x1Du);
  assert(mr.entities.team[0].target == 0);

  /* both restart arms are phase-2 only: phase 0x10 returns a gate no-op */
  fifa96_match_run_init(&mr);
  mr.state.phase = 0x10;
  mr.goal_armed = 1;
  mr.goal_zone = 0;
  mr.goal_snap_z = 0xB30;
  assert(fifa96_match_run_goal_scan(&mr) == 0);
  assert(mr.state.phase == 0x10u && mr.corner_count[0] == 0u);
  mr.goal_snap_z = 0xB20;                                   /* the throw-in band */
  assert(fifa96_match_run_goal_scan(&mr) == 0);
  assert(mr.state.phase == 0x10u);

  /* live session: the head queues the restart id instead of writing the
   * phase (L4) */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.session_gate_14c32a = 1;
  mr.goal_armed = 1;
  mr.entities.controlled = (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 3);
  mr.goal_snap_z = 0xB20;
  assert(fifa96_match_run_goal_scan(&mr) == 1);
  assert(mr.state.phase == 2u);
  assert(mr.situation_id == 9u && mr.situation_pending == 1u);
  assert(mr.corner_count[0] == 0u);
}

/* FU-145 S2 core / FU-146 S3 consumer: a fixture pan drives the FU-71 camera
 * integrator past the arming bounds; the frame body arms, the snapshot freezes,
 * and the clock tail's scan queues situation 6. FU-146 S3: the begun run's
 * installer arms the step machine at match setup; raising the kickoff gate
 * ([0x15882A]) settles it at the post step ([0x15B6B0]) and the next frame's
 * scheduler consumes the queued id 5 through the leg-0 id table (side 0) into
 * the FUN_00093944 writer — the post's FUN_000740A0(0,0) writes phase 0. The
 * native pan producer (rate words 0x1577C0/C2) is leg L1/S4; the fixture uses
 * the equivalent engine seam (the camera velocity pair, as test_camera drives
 * it). */
static void test_goal_chain_pan_fixture(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.session_gate_14c32a == 1);    /* begin seeds the live session */
  mr.state.phase = 2;
  mr.state.period_length = 90;
  /* the kickoff-complete gate settles the machine (step 1 -> 4) and the
   * phase-clear step drops the installer latch */
  mr.global_5882a = 1;
  one_granted_frame(&mr);
  assert(mr.situation_pending == 0);

  fifa96_camera_init(&mr.render.camera, 0, 0, 0xB00);
  mr.render.camera.vel_z = 0x40;          /* 0x40 * delta 2 per granted frame */
  mr.render.camera.speed = 0x40;          /* the update's integration gate */
  mr.render.camera.timer_limit = 0x7FFF;  /* T2: keep the pan-step re-arm out */
  mr.render.camera.anchor_time = 0x7FFF;
  mr.render.camera.anchor2_time = 0x7FFF;
  one_granted_frame(&mr);
  assert(mr.goal_armed == 1);
  assert(mr.goal_snap_z == 0xB80);
  assert(mr.goal_zone == 1);
  assert(mr.situation_pending == 1 && mr.situation_id == 5);
  assert(mr.score[0] == 0 && mr.score[1] == 0);   /* queued, not yet consumed */

  /* the snapshot is frozen; the next frame's scheduler runs the post step,
   * which consumes the queued id through the native score writer. The
   * no-consumer fallback pinned by S2 is replaced by this chain. */
  int32_t snap = mr.goal_snap_z;
  one_granted_frame(&mr);
  assert(mr.goal_snap_z == snap);
  assert(mr.score[0] == 1 && mr.score[1] == 0);
  assert(mr.score_last_side == 0);
  assert(mr.state.phase == 0u);           /* post FUN_000740A0(0,0) */
  assert(mr.situation_pending == 1 && mr.situation_id == 5);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-146 S3 RED core: the native goal-consumer chain end to end on the natural
 * kickoff chain. The begun run installs the goal-screen machine; the natural
 * kickoff (act-1 producer + wired row 01) raises [0x15882A] and writes phase 2,
 * settling the machine at its post step and clearing the install latch. The
 * scanner's situation-6 queue arm then latches a queued goal id, and the next
 * frame's scheduler consumes it: the leg-0 id table maps queued id 5 to side 0
 * and the post runs fifa96_match_run_score_event — the score increments through
 * the native consumer path. Fails on BASE: no installer/scheduler/handler, so
 * the queued id is never consumed. */
static void test_goal_consumer_chain_fixture(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
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
  /* the kickoff transition settled the machine and cleared the install latch */
  assert(mr.situation_pending == 0);
  one_granted_frame(&mr);                 /* steady-state scheduler frame */

  assert(fifa96_match_run_goal_queue(&mr, 0) == 0);   /* the scanner's arm */
  assert(mr.situation_id == 5 && mr.situation_pending == 1);
  assert(mr.score[0] == 0 && mr.score[1] == 0);

  one_granted_frame(&mr);
  assert(mr.score[0] == 1 && mr.score[1] == 0);       /* RED on BASE */
  assert(mr.score_last_side == 0);
  assert(mr.state.phase == 0u);           /* the post's FUN_000740A0(0,0) */
  assert(mr.situation_pending == 1);      /* the post re-latches */
  assert(mr.situation_id == 5);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-146 S3: the installer state (`FUN_00092D8C`/`FUN_00092E2C`). begin runs
 * the derived leg 0 / mode 0 / side 0 install; an explicit install switches
 * leg/mode, zeroes the score pair/writer bookkeeping, seeds the per-mode
 * duration and runs the handler once. */
static void test_screen_install_state(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.screen_leg == -1);            /* init: no handler installed */
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.screen_leg == 0 && mr.screen_mode == 0);
  assert(mr.screen_period_frames == 900u);   /* mode 0 leg 0 = 15 s * 60 */
  assert(mr.situation_pending == 1);         /* the installer latch 0x92DCD */
  assert(mr.screen_step == 1);               /* step 0 ran; the gate is closed */
  assert(mr.screen_install_hint == 0);

  mr.score[0] = 3;
  mr.score[1] = 2;
  mr.score_max_diff = 5;
  assert(fifa96_match_run_screen_install(&mr, 2, 1, 0) == 0);
  assert(mr.score[0] == 0 && mr.score[1] == 0 && mr.score_max_diff == 0);
  assert(mr.screen_leg == 2 && mr.screen_mode == 1);
  assert(mr.screen_period_frames == 1800u);  /* mode 1 leg 2 = 30 s * 60 */
  assert(mr.screen_step == 1);               /* leg 2 step 0 RETs (0x9413E) */
  assert(mr.screen_timer == 0);
  assert(mr.situation_pending == 1);

  /* the native installer indexes the table blindly; the port hardens */
  assert(fifa96_match_run_screen_install(&mr, 6, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_screen_install(&mr, -1, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_screen_install(&mr, 0, 4, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_screen_install(NULL, 0, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);

  struct fifa96_match_run dead;
  fifa96_match_run_init(&dead);
  assert(fifa96_match_run_screen_install(&dead, 0, 0, 0) == -FIFA96_ERR_STATE);
  assert(fifa96_match_run_screen_schedule(&dead) == -FIFA96_ERR_STATE);
  assert(fifa96_match_run_screen_step(&dead) == -FIFA96_ERR_STATE);
  assert(fifa96_match_run_screen_advance(&dead) == -FIFA96_ERR_STATE);
}

/* FU-146 S3: the scheduler's queue arms (`FUN_000948AC` 0x948B2..0x949DA). */
static void test_screen_schedule_ids(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  /* the screen-over rollover: leg 0 -> id 8 */
  mr.screen_timer = (uint16_t)(mr.screen_period_frames + 1u);
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 8 && mr.situation_pending == 1);
  /* leg 5 -> id 7 */
  assert(fifa96_match_run_screen_install(&mr, 5, 0, 0) == 0);
  mr.situation_id = 0;
  mr.situation_pending = 0;
  mr.screen_timer = (uint16_t)(mr.screen_period_frames + 1u);
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 7 && mr.situation_pending == 1);
  /* leg 2, a side-1 controlled record, actor age > 0xF0 -> id 3 */
  assert(fifa96_match_run_screen_install(&mr, 2, 0, 0) == 0);
  mr.situation_id = 0;
  mr.situation_pending = 0;
  mr.entities.controlled = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 5;
  mr.entities.team[1].side = 1;
  mr.screen_actor_age = 0xF1;
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 3 && mr.situation_pending == 1);
  /* side 0 -> id 4 */
  mr.situation_id = 0;
  mr.situation_pending = 0;
  mr.entities.controlled = 5;
  mr.entities.team[0].side = 0;
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 4 && mr.situation_pending == 1);
  /* no controlled record -> id 4 */
  mr.situation_id = 0;
  mr.situation_pending = 0;
  mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 4 && mr.situation_pending == 1);
  /* the age bound is exclusive */
  mr.situation_id = 0;
  mr.situation_pending = 0;
  mr.screen_actor_age = 0xF0;
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 0 && mr.situation_pending == 0);
  /* leg 0, camera z and lead z negative -> id 9 */
  assert(fifa96_match_run_screen_install(&mr, 0, 0, 0) == 0);
  mr.situation_id = 0;
  mr.situation_pending = 0;
  mr.render.camera.pos_z = -1;
  mr.screen_lead_z = -1;
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 9 && mr.situation_pending == 1);
  /* leg 5, lead z negative -> id 7 */
  assert(fifa96_match_run_screen_install(&mr, 5, 0, 0) == 0);
  mr.situation_id = 0;
  mr.situation_pending = 0;
  mr.render.camera.pos_z = 0;
  mr.screen_lead_z = -1;
  assert(fifa96_match_run_screen_schedule(&mr) == 0);
  assert(mr.situation_id == 7 && mr.situation_pending == 1);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
  assert(fifa96_match_run_screen_schedule(NULL) == -FIFA96_ERR_INVALID);
}

/* FU-146 S3: the `FUN_000CBC4C` six-limb probe over the image-seeded cells.
 * First call folds to 0x559A51ED (low byte 0xED); a full wrap cascade adds the
 * carry out of C0 (0xCBCB0..0xCBCB7). */
static void test_goal_probe_limbs(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_goal_probe(&mr) == 0xEDu);
  assert((fifa96_match_run_goal_probe(&mr) & 3u) != 0u);
  for (int i = 0; i < 6; i++) mr.goal_probe_limb[i] = 0;
  assert(fifa96_match_run_goal_probe(&mr) == 0u);
  mr.goal_probe_limb[5] = 0xFFFFFFFFu;
  assert(fifa96_match_run_goal_probe(&mr) == 0u);
  assert(mr.goal_probe_limb[0] == 0u && mr.goal_probe_limb[5] == 0u);
  assert(fifa96_match_run_goal_probe(NULL) == 0u);
}

/* FU-146 S3 / the reviewer's L.4 correction: the per-leg id tables. Legs 0/1
 * map the queued goal id 6 to the no-score counter; legs 2..5 score it side 1.
 * The latch-clear step must consume the queued id on the following call. */
static void test_screen_post_id_tables(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.screen_leg == 0);
  mr.global_5882a = 1;                    /* the kickoff-complete gate */
  mr.state.phase = 2;
  assert(fifa96_match_run_screen_step(&mr) == 0);   /* settle: step 1 -> 4 */
  assert(mr.screen_step == 4);
  assert(mr.situation_pending == 0);      /* the phase-clear dropped the latch */
  mr.situation_id = 6;
  mr.situation_pending = 1;
  assert(fifa96_match_run_screen_step(&mr) == 1);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.goal_no_score == 1);          /* leg 0: id 6 -> INC [0x15B6A0] */
  assert(mr.goal_last_id == 6);
  assert(mr.state.phase == 0u);           /* the post's phase write */
  assert(mr.screen_step == 5);
  assert(fifa96_match_run_end(&mr) == 0);

  /* leg 2: queued id 6 -> side 1 (0x94088 table) */
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_screen_install(&mr, 2, 0, 0) == 0);
  mr.state.phase = 2;
  assert(fifa96_match_run_screen_step(&mr) == 0);   /* settle: step 1 -> 3 */
  assert(mr.screen_step == 3);
  mr.situation_id = 6;
  mr.situation_pending = 1;
  assert(fifa96_match_run_screen_step(&mr) == 1);
  assert(mr.score[0] == 0 && mr.score[1] == 1);
  assert(mr.score_last_side == 1);
  assert(mr.goal_no_score == 0);
  assert(mr.state.phase == 0u);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-146 S3 review fix: leg 5's post has no id bound and no no-score path —
 * `0x94845 CMP EDX,5 / JNZ 0x9485C` -> `0x94869 MOV EAX,1`: id 5 -> side 0 and
 * every other id -> side 1 (the `0x94655/0x94677` bound is leg 4's dispatch).
 * Pins both halves; the bounded-table reading fails the id-7 half. */
static void test_screen_leg5_unbounded_id_map(void) {
  struct fixture f = make_fixture(10000000ull);

  /* id 5 -> side 0 */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
    assert(fifa96_match_run_screen_install(&mr, 5, 0, 0) == 0);
    mr.state.phase = 2;
    assert(fifa96_match_run_screen_step(&mr) == 0); /* settle at the post step */
    assert(mr.screen_step == 2);
    assert(mr.situation_pending == 0);
    mr.situation_id = 5;
    mr.situation_pending = 1;
    assert(fifa96_match_run_screen_step(&mr) == 1);
    assert(mr.score[0] == 1 && mr.score[1] == 0);
    assert(mr.goal_no_score == 0);
    assert(mr.state.phase == 0u);
    assert(fifa96_match_run_end(&mr) == 0);
  }

  /* id 7 -> side 1 (no bound on leg 5) */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
    assert(fifa96_match_run_screen_install(&mr, 5, 0, 0) == 0);
    mr.state.phase = 2;
    assert(fifa96_match_run_screen_step(&mr) == 0); /* settle again */
    assert(mr.screen_step == 2);
    assert(mr.situation_pending == 0);
    mr.situation_id = 7;
    mr.situation_pending = 1;
    assert(fifa96_match_run_screen_step(&mr) == 1);
    assert(mr.score[0] == 0 && mr.score[1] == 1);
    assert(mr.goal_no_score == 0);
    assert(mr.state.phase == 0u);
    assert(fifa96_match_run_end(&mr) == 0);
  }
  drop_fixture(f);
}

/* FU-146 S3: the post's lazily-computed probe reaches the writer's 0xD3 arm.
 * Leg 2, tracked side 0, a side-1 goal: the native writer hits the untracked
 * `score[side] == 1 && score[other] < 3` arm and calls FUN_000CBC4C; the first
 * probe byte 0xED has (0xED & 3) != 0 and posts 0xD3. */
static void test_screen_step_probe_post(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_screen_install(&mr, 2, 0, 0) == 0);
  mr.state.phase = 2;
  assert(fifa96_match_run_screen_step(&mr) == 0);   /* settle at the post step */
  assert(mr.screen_step == 3);
  mr.score_tracked_side = 0;
  mr.situation_id = 6;
  mr.situation_pending = 1;
  assert(fifa96_match_run_screen_step(&mr) == 1);
  assert(mr.score[1] == 1 && mr.score[0] == 0);
  assert(mr.score_last_event == 0xD3);    /* the probe-gated post */
  assert(mr.goal_probe_limb[5] == 0x6FDF3B65u);   /* exactly one probe call */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-146 S3: the FUN_000935A0 advance subset — `screen_timer > 0xB4` on the
 * tail step appends the goal-log triple at the new-total slot, records the
 * totals, sets the install hint and re-runs the handler (which clears the
 * kickoff gate on its setup step). */
static void test_screen_advance_ring(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  mr.global_5882a = 1;
  mr.state.phase = 2;
  assert(fifa96_match_run_screen_step(&mr) == 0);   /* settle: step 1 -> 4 */
  assert(mr.screen_step == 4);
  mr.situation_id = 5;
  mr.situation_pending = 1;
  assert(fifa96_match_run_screen_step(&mr) == 1);   /* consume queued id 5 */
  assert(mr.score[0] == 1);
  assert(mr.screen_step == 5);
  assert(mr.goal_total == 0);

  mr.screen_timer = 0xB5;                           /* > 0xB4 on the tail step */
  assert(fifa96_match_run_screen_step(&mr) == 0);   /* tail -> advance */
  assert(mr.goal_total == 1);
  assert(mr.goal_log_prev_total == 1);
  assert(mr.goal_log[1][0] == 0);                   /* score_last_side */
  assert(mr.goal_log[1][1] == 5);                   /* goal_last_id */
  assert(mr.goal_log[1][2] == (int32_t)mr.goal_minute);
  assert(mr.screen_install_hint == 1);
  assert(mr.screen_step == 1);                      /* re-install ran step 0 */
  assert(mr.global_5882a == 0);                     /* the setup step cleared it */

  /* a second advance with no score change does not append */
  mr.screen_timer = 0xB5;
  assert(fifa96_match_run_screen_step(&mr) == 0);
  assert(mr.goal_total == 1);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-148 S4: the FUN_000505D0 pose feed wired into the granted frame. Mode
 * 6/0x10 selects record 7 of the default behavior block (the +0x4C array
 * 0x107F1C, class 3): {x,y,z} = {-83, 320, 3631}; |z|_w > 0xB20 and the
 * camera lands out of bounds, so the frame's armer fires on the fed pose (the
 * arming gate is the native word-truncated |camZ| > 0xB20). The classifier
 * reports zone 0 (|z| >= 0xB90), so the scan takes the corner/goal-kick arm
 * (FU-149 P1): snap z positive and ball side 0 (the fresh run's
 * [0x1577CA] stand-in is unset) -> cond true -> sit 4, queued id 0 for the
 * side-1 kick in the live session (the begun run's gate is open). */
static void test_view_pose_feed_arms_camera(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  mr.global_5882a = 1;
  one_granted_frame(&mr);                       /* settle the kickoff machine */
  assert(mr.situation_pending == 0);
  assert(mr.render.camera_pose.view_mode == 0); /* the fresh-match default */

  mr.render.camera_pose.view_mode = 0x10;
  one_granted_frame(&mr);
  assert(mr.render.camera.pos_x == -83);
  assert(mr.render.camera.pos_y == 320);
  assert(mr.render.camera.pos_z == 3631);
  assert(mr.render.yaw == 33732 && mr.render.pitch == 2203);
  assert(mr.render.view_ratio == 3712);
  assert(mr.goal_armed == 1);
  assert(mr.goal_snap_x == -83 && mr.goal_snap_y == 0 && mr.goal_snap_z == 3631);
  assert(mr.goal_zone == 0);
  /* FU-149 P1: the corner/goal-kick arm fires (cond true -> sit 4, side
   * 0 ^ 1 = 1) and the live session queues the side-1 restart id 0. */
  assert(mr.situation_pending == 1);
  assert(mr.situation_id == 0u);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-148 S4 / FU-145 S2 L1 hand-off: the real pan producer
 * (`fifa96_camera_event_set` = FUN_00071C94 + FUN_00070544) drives the FU-71
 * integrator past the arming bounds. The camera starts at z=0xB00; the event
 * seed step 2000 with height 0x30 gives F2=25/F4=50/F6=0: divisor 50, timer 0
 * (the native [0x1577FA] = F6 cell), vel_z = 40 -> the > 0x19 atan walk caps
 * the bearing to 25, so one granted delta-2 frame lands z=0xB32 — inside the
 * mouth band (0xB10..0xB8F), zone 1 — and the same frame's scan queues
 * situation 5 for side 0. The next frame's scheduler consumes it through the
 * S3 consumer chain: score 1-0. This is the S2 pan-source closure: the
 * velocity words are produced by the ported event machine, not poked by the
 * fixture. The natural row-04 invoker is covered by
 * `test_row04_live_pan_arms_camera`; the remaining 71C94 callers stay legs. */
static void test_camera_pan_event_chain(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  mr.global_5882a = 1;
  one_granted_frame(&mr);
  assert(mr.situation_pending == 0);

  assert(fifa96_camera_init(&mr.render.camera, 0, 0, 0xB00) == FIFA96_OK);
  assert(fifa96_camera_event_set(&mr.render.camera, 0, 2000, 0x30, 0) == FIFA96_OK);
  assert(mr.render.camera.vel_z == 25 && mr.render.camera.timer == 0);
  one_granted_frame(&mr);
  assert(mr.render.camera.pos_z == 0xB32);
  assert(mr.goal_armed == 1);
  assert(mr.goal_snap_z == 0xB32);
  assert(mr.goal_zone == 1);
  assert(mr.situation_id == 5 && mr.situation_pending == 1);
  assert(mr.score[0] == 0);

  one_granted_frame(&mr);
  assert(mr.score[0] == 1 && mr.score[1] == 0);
  assert(mr.score_last_side == 0);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-148 §2.1(c)/§6.2 + FU-152 §2.9 (T2): the natural gameplay-row pan
 * origin. A live action-04 record (the ported `fifa96_outfield_row04_step`
 * half-line arm) produces `out.events` and the engine wires it to the real
 * `fifa96_camera_event_set`; no fixture pokes the camera velocity. The camera
 * starts at z=0xB00 so the row's first event lands it past the 0xB20 arming
 * bound; the frame's armer then fires (zone 1) and the scan queues situation
 * 5, consumed by the next frame (score 1-0). The row's own coda installs
 * code 5 on the record, so exactly one row event drives the burst. */
static void test_row04_live_pan_arms_camera(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  mr.global_5882a = 1;
  one_granted_frame(&mr);
  assert(mr.situation_pending == 0);
  assert(mr.slot.entity != id);

  assert(fifa96_camera_init(&mr.render.camera, 0, 0, 0xB00) == FIFA96_OK);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 4, 0) == 1);
  e->active = 1;
  e->pos_x = 0;
  e->pos_y = 0;
  e->pos_z = 0xB00;                    /* at the camera: lane/angle gates pass */
  e->target_x = 0;
  e->target_z = 0xB00;
  e->actor_type = 16;                  /* 0x10F334/3C offsets (21,88)<<5 seed */
  e->anim_id = 0;                      /* row byte != 0x13 -> the event arm */
  e->timer81 = 0;
  mr.entities.ball.y = 0x20;           /* <= 0x50, != 0 -> the type-table seed */

  one_granted_frame(&mr);
  /* the row body fired the event: the camera velocity is the row's, not a
   * fixture poke (vel_z 2816/12 = 234 -> atan-capped to 25). */
  assert(mr.render.camera.vel_x != 0 || mr.render.camera.vel_z != 0);
  assert(e->code == 5);                /* the row coda installed code 5 */

  one_granted_frame(&mr);
  assert(mr.goal_armed == 1);
  assert(mr.goal_snap_z > 0xB20);
  assert(mr.goal_zone == 1);
  assert(mr.situation_id == 5 && mr.situation_pending == 1);

  one_granted_frame(&mr);
  assert(mr.score[0] == 1 && mr.score[1] == 0);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-148 §2.1(c) + §12 (T2 review): the row-04 **arm A** (`[rec+0x28]` byte
 * 0x13) tail. The native 0x7EFD4 writes `[0x1577FA] = F2 + F2/4`
 * unconditionally after the FUN_00071C94 call; with the height-0 call the
 * FUN_00070544 idle arm gives F2 = 6, so the pan timer becomes 7 even though
 * event_param stays 0 (the pre-review `event_param != 0` guard wrongly
 * skipped it). Arm A's step seed is the type table << 6 (1344, 5632). */
static void test_row04_arm_a_track_reload(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  mr.global_5882a = 1;
  one_granted_frame(&mr);
  assert(mr.slot.entity != id);

  assert(fifa96_camera_init(&mr.render.camera, 0, 0, 0xB00) == FIFA96_OK);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 4, 0) == 1);
  e->active = 1;
  e->pos_x = 0;
  e->pos_y = 0;
  e->pos_z = 0xB00;
  e->target_x = 0;
  e->target_z = 0xB00;
  e->actor_type = 16;
  e->anim_id = 0x13;                   /* arm A: event_track_reload */
  e->timer81 = 0;
  mr.entities.ball.y = 0x20;

  one_granted_frame(&mr);
  assert(mr.render.camera.vel_z != 0);           /* the row's event fired */
  assert(mr.render.camera.event_param == 0);     /* the idle-height call */
  assert(mr.render.camera.timer == 7);           /* F2(6) + F2/4 = 7 */
  assert(e->code == 5);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-145 S2/S3 + FU-146 (T4, OL-87/88/89): the natural goal end-to-end. The
 * sequence is played, not forced: the kickoff runs its 0x13 countdown and the
 * T3 KICK press (the pad path; no manual phase or [0x5882A] write) enters live
 * phase 2. A live action-04 record then fires its ground-ball sub-object arm
 * (the native 0x7F035 arm: the slot dir bytes with ball height 0, so no ball
 * staging) and every native link runs in the frame-body order: the ported row
 * event -> `fifa96_camera_event_set` -> the pan integrator -> the boundary
 * armer (zone 1) -> the clock-tail scanner FUN_00088940 (situation 6) -> the
 * table-1 queue (id 5, the `[0x14C32A] != 0 && [0x15B6C0] == 0` condition) ->
 * the scheduler -> the leg-0 handler's post -> `fifa96_match_run_score_event`
 * (1-0). The staged inputs are the scenario's record install, the camera park
 * at the goal-mouth edge and the record's slot dir; the tracked-side installer
 * pick stays the carried -1 leg, so the writer posts no id (score_last_event
 * 0). */
static void test_natural_goal_end_to_end(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  one_granted_frame(&mr);

  /* the live entry: the installer latch cleared by the phase-2 step, the
   * begin-seeded live session gate (the front-end producer is the FU-146 leg
   * 2), the leg-0 handler and a fresh score pair. */
  assert(mr.session_gate_14c32a == 1u);
  assert(mr.situation_pending == 0u);
  assert(mr.screen_leg == 0);
  assert(mr.score[0] == 0u && mr.score[1] == 0u);

  assert(fifa96_camera_init(&mr.render.camera, 0, 0, 0xB00) == FIFA96_OK);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 4, 0) == 1);
  /* M2 phase-9 T1 (FU-75 L4.1): the applied forced decision re-asserts the
   * team-role policy in phase 2, so a live code-4 record is its team's
   * `[team+0x7B2]` controlled entity (otherwise FUN_0007C990 installs 3). */
  mr.entities.team[1].target = id;
  e->active = 1;
  e->pos_x = 0;
  e->pos_y = 0;
  e->pos_z = 0xB00;
  e->target_x = 0;
  e->target_z = 0xB00;
  e->actor_type = 16;
  e->anim_id = 0;
  e->timer81 = 0;
  e->has_slot = 1;                     /* [rec+0x20] != 0 -> the 0x7F035 arm */
  e->dir_x = 1;                        /* native slot +0x20/+0x21 dir bytes */
  e->dir_z = 0x40;
  assert(mr.entities.ball.y == 0);     /* natural ground-ball height */
  assert(mr.slot.entity != id);

  /* link 1: the live row body produced the camera event (not a fixture poke) */
  one_granted_frame(&mr);
  assert(mr.render.camera.vel_z != 0);
  assert(e->code == 5);                /* the row coda: one event per episode */

  /* link 2: the pan integrator moved the camera past the arming bound and the
   * boundary armer fired with the goal-mouth zone the same frame */
  one_granted_frame(&mr);
  assert(mr.goal_armed == 1u);
  assert(mr.goal_snap_z > 0xB20);
  assert(mr.goal_zone == 1u);

  /* link 3: the clock-tail scanner queued situation 6 through the open-session
   * queue arm (id 5 for side 0; the score has not moved yet) */
  assert(mr.situation_id == 5u && mr.situation_pending == 1u);
  assert(mr.score[0] == 0u && mr.score[1] == 0u);

  /* link 4: the next frame's scheduler consumed the id through the leg-0
   * handler's post and the writer incremented the score. `tracked_side` is
   * still the carried -1 (the installer pick is the OL-87 leg), so no id is
   * posted. The post re-latches the pending flag (0x93D46) and its phase-0
   * write (0x93DB2) ends the scripted screen. */
  one_granted_frame(&mr);
  assert(mr.score[0] == 1u && mr.score[1] == 0u);
  assert(mr.score_last_side == 0);
  assert(mr.score_last_event == 0u);
  assert(mr.situation_pending == 1u);
  assert(mr.state.phase == 0u);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-146 §7 item 1 / FU-149 §1.1 (T4): the queue-condition fallback arm on the
 * same natural path. With the live-session gate closed at scan time
 * ([0x14C32A] == 0; its producers are the unported front-end block, FU-146 leg
 * 2 — staged here as the closed-session state), the situation-6 entry takes
 * the direct arm: the score increments on the scan frame (the FU-72 plain
 * increment, not the writer) and the table-2 row 6 writes phase 5; no id
 * enters the queue and the installer latch stays clear. */
static void test_natural_goal_fallback_arm(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  one_granted_frame(&mr);
  assert(mr.session_gate_14c32a == 1u && mr.situation_pending == 0u);

  assert(fifa96_camera_init(&mr.render.camera, 0, 0, 0xB00) == FIFA96_OK);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 4, 0) == 1);
  /* the applied L4.1 forced decision keeps a live code-4 record only on its
   * team's `[team+0x7B2]` controlled entity (else code 3 is installed). */
  mr.entities.team[1].target = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  e->active = 1;
  e->pos_x = 0;
  e->pos_y = 0;
  e->pos_z = 0xB00;
  e->target_x = 0;
  e->target_z = 0xB00;
  e->actor_type = 16;
  e->anim_id = 0;
  e->timer81 = 0;
  e->has_slot = 1;
  e->dir_x = 1;
  e->dir_z = 0x40;

  one_granted_frame(&mr);
  assert(mr.render.camera.vel_z != 0);
  /* close the session before the scan frame: the same pan arms, but the
   * scanner's situation-6 entry takes the direct arm. */
  mr.session_gate_14c32a = 0;
  one_granted_frame(&mr);
  assert(mr.goal_armed == 1u && mr.goal_zone == 1u);
  assert(mr.score[0] == 1u && mr.score[1] == 0u);   /* direct increment */
  assert(mr.score_last_side == -1);                 /* not the writer */
  assert(mr.score_last_event == 0u);
  assert(mr.state.phase == 5u);                     /* table-2 row 6 */
  assert(mr.situation_id == 0u);
  assert(mr.situation_pending == 0u);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-148 §3 (S4): the formation-id producer. FUN_0008EA70 writes
 * `[0x14C1E4+side]`; the match-init copy (FUN_00011620) sources the team
 * record +0x12 byte, and the placement family comes from the 0x14BFC0
 * `6*id` slot (`FUN_0004A6BC` builds the names from the loader table
 * 0x107370: id 0 "352ko.fmt", 1 "442ko.fmt", 2 "swko.fmt", 3 "424ko.fmt",
 * 4 "433ko.fmt"). FUN_0006D9C4 indexes the layout row 0x11033A + id*0x1D. */
static void test_formation_producer(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.formation[0] == 0 && mr.formation[1] == 0);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.formation[0] == 0 && mr.formation[1] == 0);
  assert(fifa96_match_run_set_formation(&mr, 0, 1) == 0);
  assert(mr.formation[0] == 1);
  assert(fifa96_match_run_set_formation(&mr, 1, 4) == 0);
  assert(mr.formation[1] == 4);
  assert(fifa96_match_run_set_formation(&mr, 2, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_set_formation(NULL, 0, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_end(&mr) == 0);
  assert(fifa96_match_run_set_formation(&mr, 0, 0) == -FIFA96_ERR_STATE);
  drop_fixture(f);

  assert(strcmp(fifa96_match_formation_fmt_name(0), "352ko.fmt") == 0);
  assert(strcmp(fifa96_match_formation_fmt_name(1), "442ko.fmt") == 0);
  assert(strcmp(fifa96_match_formation_fmt_name(2), "swko.fmt") == 0);
  assert(strcmp(fifa96_match_formation_fmt_name(3), "424ko.fmt") == 0);
  assert(strcmp(fifa96_match_formation_fmt_name(4), "433ko.fmt") == 0);
  assert(fifa96_match_formation_fmt_name(5) == NULL);

  fifa96_match_formation_block blocks[4];
  assert(fifa96_match_formation_layout(0, blocks) == 4);
  assert(blocks[0].role == 0 && blocks[0].count == 1 && blocks[0].slots[0] == 0);
  assert(blocks[1].role == 1 && blocks[1].count == 3);
  assert(blocks[1].slots[0] == 1 && blocks[1].slots[1] == 2 && blocks[1].slots[2] == 3);
  assert(blocks[2].role == 2 && blocks[2].count == 5);
  assert(blocks[2].slots[0] == 4 && blocks[2].slots[1] == 5 && blocks[2].slots[2] == 6 &&
         blocks[2].slots[3] == 7 && blocks[2].slots[4] == 8);
  assert(blocks[3].role == 3 && blocks[3].count == 2);
  assert(blocks[3].slots[0] == 9 && blocks[3].slots[1] == 10);
  assert(fifa96_match_formation_layout(1, blocks) == 4);
  assert(blocks[1].count == 4 && blocks[1].slots[3] == 4);
  assert(blocks[2].count == 4 && blocks[2].slots[0] == 5 && blocks[2].slots[3] == 8);
  assert(fifa96_match_formation_layout(5, blocks) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_formation_layout(0, NULL) == -FIFA96_ERR_INVALID);
}

/* FU-148 §4.2 (S4): the per-entity translation install through the pool seam.
 * With a staged pool (partition of a caller buffer), the non-kit path copies
 * the entity slot verbatim (0xCE980's 0x40 dwords); the kit path
 * ([0x1068E0]==1 analog = palette_ready, entity 0/0xB) applies the band
 * translation. */
static void test_translation_install(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  static uint8_t pool[0x3000];
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  for (size_t i = 0; i < sizeof pool; i++) pool[i] = (uint8_t)(i * 7u);
  assert(fifa96_palette_pool_partition(pool, sizeof pool, &mr.render.palette_pool) ==
         FIFA96_OK);

  assert(fifa96_match_run_translation_install(&mr, 1) == 0);
  assert(memcmp(mr.render.remap, pool + 0x100, 256) == 0);

  mr.render.palette_ready = 1;
  pool[0] = 0x94;
  pool[1] = 0x9B;
  pool[2] = 0x9A;
  pool[3] = 0x93;
  assert(fifa96_match_run_translation_install(&mr, 0) == 0);
  assert(mr.render.remap[0] == 0xA1);       /* table1[0] + 0xA1 */
  assert(mr.render.remap[1] == 0xA3);       /* table2[0] + 0xA3 */
  assert(mr.render.remap[2] == 0xA2);       /* table1[6] + 0xA1 */
  assert(mr.render.remap[3] == 0x93);       /* outside the bands */
  assert(fifa96_match_run_translation_install(&mr, 23) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_translation_install(NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_end(&mr) == 0);
  assert(fifa96_match_run_translation_install(&mr, 0) == -FIFA96_ERR_STATE);
  {
    /* a fresh live run without a staged pool: the pool identity is leg 11 */
    struct fifa96_match_run bare;
    fifa96_match_run_init(&bare);
    assert(fifa96_match_run_begin(&bare, f.engine, 0) == 0);
    assert(fifa96_match_run_translation_install(&bare, 0) == -FIFA96_ERR_STATE);
    assert(fifa96_match_run_end(&bare) == 0);
  }
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
  test_set_piece_queue_ids();
  test_set_piece_bx_fallback();
  test_set_piece_phase_arm_codes();
  test_taker_armed_rows_resolve();
  test_corner_counter_side_swap();
  test_engine_referee_contact_fk();
  test_taker_armed_referee_rows_resolve();
  test_engine_referee_contact_gates();
  test_engine_referee_foul_sequence();
  test_engine_referee_offside_chain();
  test_scan_restart_arms();
  test_goal_chain_pan_fixture();
  test_goal_consumer_chain_fixture();
  test_screen_install_state();
  test_screen_schedule_ids();
  test_goal_probe_limbs();
  test_screen_post_id_tables();
  test_screen_leg5_unbounded_id_map();
  test_screen_step_probe_post();
  test_screen_advance_ring();
  test_pad_drives_controlled_locomotion();
  test_pad_kick_release_runs_kick_row();
  test_machine_forced_decision_installs_on_slot_record();
  test_machine_second_forced_reads_team_target_ball_bit();
  test_machine_no_edge_arm_copies_camera_target();
  test_held_key_moves_live_controlled_record();
  test_ai_record_mover_and_lane_track();
  test_row1e_claim_reaches_pool();
  test_row1e_stage3_possession_flip();
  test_view_pose_feed_arms_camera();
  test_camera_pan_event_chain();
  test_row04_live_pan_arms_camera();
  test_row04_arm_a_track_reload();
  test_natural_goal_end_to_end();
  test_natural_goal_fallback_arm();
  test_formation_producer();
  test_translation_install();
  puts("test_engine_match_frame OK");
  return 0;
}
