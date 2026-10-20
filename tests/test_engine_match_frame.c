/* tests/test_engine_match_frame.c — Task 13: match frame body, pacing and clock state. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_act.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

/* One match frame is one 0x200 step owed to the state library; each second is
 * 0x3C (60) of those units, so one 30 Hz frame is 2/60 s. The pace grants
 * 3 frames per 10 ticks (0x102/0x35C = 3/10), i.e. 300 granted frames in
 * exactly 1000 engine steps: 300/30 = 10 s of match clock. */
#define STEPS_FOR_10S 1000

/* v22 (M2 phase-22 T4, FU-235 §1.2/§4): the phase-0xC match-over dwell. After
 * the regulation completion the engine holds the class-0 phase-0x0C screen
 * while the end/act timer `word[0x158818]` (`mr.act.timer`) accrues; `mark_over`
 * fires only past `0x168` (360). At the 3-grants-per-10-calls pace and +2 per
 * grant that is ~600 calls plus the act-0xB entry screen; 2000 is a safe bound
 * that still trips a mutation (a broken dwell never resolves). */
#define DWELL_FRAMES 2000

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

/* The library boundary: a 1 s PAST-half-time period ends on the 30th granted
 * frame (the 100th pace tick here); the frame body must mark the lifecycle over.
 * v21 (M2 phase-21 T5, FU-228 §4.2): the clock is seeded at period 1 so the
 * completion rolls `global_157ac2` to 2 (>= 2) with a tied score (flag clear) —
 * the end word is decided, so `mark_over` fires. A half-time completion now
 * continues (see test_phase_drive_reaches_period_end). */
static void test_period_end_marks_lifecycle_over(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  mr.state.period = 1;
  mr.state.phase = 2;
  mr.state.period_length = 1;
  mr.state.extra_length = 1;

  for (int i = 0; i < DWELL_FRAMES; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
  }

  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);
  assert(mr.state.period == 2);
  assert(mr.state.period_seconds == 0);
  assert(mr.state.total_seconds == 1);
  /* v22 (M2 phase-22 T4, FU-235 §1.2/§4): the end word staged and the
   * match-over screen was held through the phase-0xC terminal dwell. */
  assert(mr.end_word_158822 == 1u);
  assert(mr.act.timer > 0x168);
  assert(mr.global_157ac2 == 2u);
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
/* M2 phase-18 T3/T4 (FU-202 §6 rec (b) / FU203-L2): begin a fresh run and
 * isolate it from the now-LIVE situation-0 kickoff chain. The live `begin` arms
 * the native situation-0 act 0xA (the re-placed match-init producer); its
 * phase-0x20 body installs code 0x24 on the pool on the first granted frame,
 * pre-empting the derived kickoff chain these fixtures exercise. Clearing the
 * armed act here lets the derived path run — the native kickoff is the act-1
 * route (FU203-L2), reached after the unported phase-0x20 `0x3E8` cutscene wait.
 * One seam, so the isolation idiom is not copy-pasted per fixture. */
static void begin_isolated(struct fifa96_match_run *mr, struct fifa96_engine *eng) {
  assert(fifa96_match_run_begin(mr, eng, 0) == 0);
  mr->act.id = 0;
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

  begin_isolated(&mr, f.engine);
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
 * this test's contract is the frame body's mark_over itself. v21 (M2 phase-21
 * T5, FU-228 §4.2): the clock is seeded PAST half-time (period 1) so this
 * completion is one that marks over (a half-time completion now continues). */
static void test_begun_period_end_marks_over(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  begin_isolated(&mr, f.engine);
  assert(mr.lc.active == 1);
  assert(mr.lc.registered == 1);
  mr.state.period = 1;
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
  assert(mr.state.period == 2);
  assert(mr.state.period_seconds == 0);
  assert(mr.state.total_seconds == 1);
  /* v22 (M2 phase-22 T4, FU-235 §4): the begin-owned frame body holds the
   * phase-0xC terminal dwell before `mark_over`; drive it out. */
  for (int i = 0; i < DWELL_FRAMES && mr.lc.screen != FIFA96_MATCH_SCREEN_OVER; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER); /* mark_over, begun run */
  assert(mr.end_word_158822 == 1u);                 /* the staged end word (FU-231 §3) */
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
 * A further frame must not re-fire: 0x0C is class 0 (clock stopped).
 *
 * v21 (M2 phase-21 T5, FU-228 §4.2): this is a HALF-TIME completion (index 1), so
 * the lifecycle does NOT exit — it continues (`mr.lc.screen` stays ACTIVE). The
 * sit-1 arm and the phase-0x17 kickoff land on a later granted frame, not this
 * completion frame (the T4/FU225-L5 one-frame cadence). */
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
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);  /* v21 T5: half-time continues */

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

/* FU-213 §2.4/§2.5 (M2 phase-19 T1): the live per-frame clock's period
 * completion drives the extra-time decision. The score tie-break (native W2
 * 0x8B2AC at the regulation end / W4 0x8B3A2 at the extra-time-2 end) sets the
 * OL-85 flag `[0x157AC0]` only when the completed period ends with DIFFERING
 * scores; the chooser `fifa96_action_phase_period_end` then receives the LIVE
 * `extra_time` (replacing the hardwired `0u`) and installs the extra-time arm
 * (act 0xC, phase 0x13/0x14) instead of the no-extra-time arm (act 0xB, phase
 * 0x0C). The roll (native 0x8B58A) advances `global_157ac2` with `state.period`.
 * A tied or non-regulation completion leaves the flag clear. */
static void test_phase_drive_extra_time_tiebreak(void) {
  struct fifa96_match_run mr;

  /* (a) differing scores at the regulation end (completed index 1) -> flag set
   *     and the ET chooser arm (phase 0x13, act 0x0C). */
  fifa96_match_run_init(&mr);
  mr.state.period = 2;                  /* the FU-62 library rolled 1 -> 2 */
  mr.state.phase = 2;                   /* class 1: clock runs */
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 0u;
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac0 == 1u);       /* W2 0x8B2AC: scores differ */
  assert(mr.state.phase == 0x13u);      /* the live extra_time chooser arm */
  assert(mr.act.id == 0x0Cu);           /* act 0xC (the ET act) */
  assert(mr.global_157ac2 == 2u);       /* the roll 0x8B58A */

  /* (b) tied scores at the regulation end -> flag clear, no-extra-time arm. */
  fifa96_match_run_init(&mr);
  mr.state.period = 2;
  mr.state.phase = 2;
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 1u;
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac0 == 0u);       /* the tie keeps extra time off */
  assert(mr.state.phase == 0x0Cu);      /* the no-ET chooser arm */
  assert(mr.act.id == 0x0Bu);           /* act 0xB */
  assert(mr.global_157ac2 == 2u);

  /* (c) the first-half completion (index 0) is not a tie-break site. */
  fifa96_match_run_init(&mr);
  mr.state.period = 1;
  mr.state.phase = 2;
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 0u;
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac0 == 0u);       /* only W2/W4 (index 1/3) */
  assert(mr.state.phase == 0x0Cu);
  assert(mr.global_157ac2 == 1u);       /* the roll */
}

/* v21 (M2 phase-21 T5, FU-227 §5 / FU-228 §4.2): the half-time (first)
 * completion selects the sit-1 arm (act 1, install-only) on the next frame's
 * top-of-call mode-0 producer (T4/FU225-L5 one-frame cadence). The chooser first
 * parks the class-0 phase 0x0C and installs act 0xB; the sit-1 arm then
 * re-dispatches situation 1 -> act 1. Mutation-RED: re-gating the T4 per-frame
 * mode-0 producer (the top-of-phase_drive `fifa96_match_run_period_setup(mr, 0)`,
 * fifa96_match_run.c:2597) leaves act.id == 0x0B. */
static void test_phase_drive_half_time_selects_act1(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.period = 1;                 /* the FU-62 library rolled 0 -> 1 */
  mr.state.phase = 2;                  /* class 1: the clock runs */
  mr.phase_machine.side_controlled = 0;
  mr.clock_period_ended = 1;

  assert(fifa96_match_run_phase_drive(&mr) == 1);   /* the completion frame */
  assert(mr.global_157ac2 == 1u);      /* the roll 0x8B58A, half-time */
  assert(mr.state.phase == 0x0Cu);     /* the chooser parked the class-0 phase */
  assert(mr.act.id == 0x0Bu);          /* the chooser's act before the sit-1 arm */

  /* The sit-1 arm runs on the next frame's top-of-call mode-0 producer. */
  assert(fifa96_match_run_phase_drive(&mr) == 0);
  assert(mr.act.id == 1u);             /* situation 1 -> act 1 (install-only) */
  assert(mr.act.stage == 0u);
  assert(mr.state.phase == 0x0Cu);     /* the phase-1 kickoff write is the act tick's job */
}

/* FU-213 §2.6 (M2 phase-19 T1): the period-index tail. The mid-extra-time W5
 * set (0x8B5CD) raises the flag per-frame while the period index is 2 or 3, the
 * scores differ and the flag is clear; the period-4 skip (0x8B608) runs the
 * chooser and rolls the index to 5 (0x8B61B) when the index is 4 with the flag
 * set. Both are on the no-completion tail. */
static void test_phase_drive_period4_skip_and_w5(void) {
  struct fifa96_match_run mr;

  /* the period-4 skip: no completion, index 4, flag set -> chooser + index 5. */
  fifa96_match_run_init(&mr);
  mr.state.period = 4;
  mr.state.phase = 2;                   /* class 1: the tail runs */
  mr.phase_machine.side_controlled = 0;
  mr.global_157ac0 = 1u;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.state.phase == 0x13u);      /* the chooser (extra_time read live) */
  assert(mr.state.period == 5u);        /* 0x8B61B INC */
  assert(mr.global_157ac2 == 5u);

  /* the mid-extra-time W5 set: no completion, index 2, scores differ -> flag. */
  fifa96_match_run_init(&mr);
  mr.state.period = 2;
  mr.state.phase = 2;
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 0u;
  mr.score[1] = 1u;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac0 == 1u);       /* W5 0x8B5CD */
  assert(mr.state.phase == 0x13u);

  /* an index-3 tied frame stays dormant (no W5, no skip). */
  fifa96_match_run_init(&mr);
  mr.state.period = 3;
  mr.state.phase = 2;
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 1u;
  assert(fifa96_match_run_phase_drive(&mr) == 0);
  assert(mr.global_157ac0 == 0u);
  assert(mr.global_157ac2 == 0u);
}

/* FU-208 §4.2 / FU-209 §6 (M2 phase-19 T2): the situation-8 selection. When the
 * T1 period-index roll lands the index on 4 (the native `[0x157AC2] == 4`
 * extra-time period) and the completed period left the phase-0xC screen, the
 * mode-0 period-end producer (native `FUN_00088860` 0x888C8) selects situation 8
 * -> act 7 (the match re-init). The bit-8 disjunct (0x4B0CB TEST AH,1) stays the
 * FU208-L1 leg (no located writer). Mutation-RED: dropping the selector (or
 * mis-gating it off the roll) leaves act 0xB (the chooser's act), so the
 * `act.id == 7` assertion fails. */
static void test_phase_drive_situation8_selects_act7(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.period = 4;                 /* the FU-62 library rolled 3 -> 4 */
  mr.state.phase = 2;                  /* class 1: the clock runs */
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 1u;                    /* tied -> no ET flag -> chooser phase 0xC */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  /* T4/FU225-L5 (M2 phase-21): the mode-0 producer runs at the TOP of every
   * phase_drive now; on this completion frame the pre-roll phase was not yet
   * 0x0C, so the sit-8 selection lands on the NEXT frame's top call (the native
   * one-frame cadence). */
  assert(fifa96_match_run_phase_drive(&mr) == 0);
  assert(mr.global_157ac2 == 4u);      /* the T1 roll reaches the sit-8 gate */
  assert(mr.global_157ac0 == 0u);      /* tied: the OL-85 flag stays clear */
  assert(mr.state.phase == 0x10u);     /* FU-219 invoke-now: arm 0 wrote phase 0x10 */
  assert(mr.act.id == 7);              /* sit 8 selected -> act 7 */
  assert(mr.act.stage == 1u);          /* arm 0 ran + advanced on the dispatch frame */
}

/* FU-224 §4.3 (M2 phase-21 T3): the disclosed match-config-default stand-in for
 * the two live cells whose front-end producers are unreachable — 0x38990
 * ([0x157AC2] := 4) and 0x389A4 ([0x157AC0] := 1). Both are reached only through
 * FUN_00038630 case 0x13 <- FUN_00039054 (gated on [0x105FFC] in {5,7}) <-
 * FUN_000493A0, and the ported front-end models neither [0x105FFC] nor
 * DAT_0014B018, so they stay unported legs (FU224-L1/L2). The faithful engine
 * form is the native BSS default: `fifa96_match_run_init`/`_begin` stage both
 * cells zero and let the in-match clock roll + score tie-break produce them.
 * No producer is faked. Mutation-RED: seeding a non-zero stand-in aborts. */
static void test_roll_flag_staged_zero(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.global_157ac2 == 0u);   /* [0x157AC2], FU-224 §1.1 (FE 0x38990 unreachable) */
  assert(mr.global_157ac0 == 0u);   /* [0x157AC0], FU-224 §1.1 (FE 0x389A4 unreachable) */
}

/* FU-224 §3.1/§3.2 (M2 phase-21 T3): on a staged extra-time-2 completion
 * (`state.period = 4`; completed index 3) with DIFFERING scores, the live clock
 * raises the OL-85 flag [0x157AC0] (W4 0x8B3A2) and the period-index roll
 * [0x157AC2] := state.period (0x8B58A) lands on 4. Mutation-RED: a wrong roll
 * (or dropping W4) fails the two assertions. The tied counterpart (flag stays
 * clear at period 4) is the existing test_phase_drive_situation8_selects_act7. */
static void test_phase_drive_roll4_flag_differing(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.period = 4;                 /* completed index 3; past half-time */
  mr.state.phase = 2;                  /* class 1: the clock runs */
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 0u;                    /* differing -> W4 raises the flag */
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  assert(mr.global_157ac2 == 4u);      /* the roll (match_run.c:2643) */
  assert(mr.global_157ac0 == 1u);      /* the OL-85 flag (match_run.c:2618, differing) */
}

/* FU-209 §6.4 / FU-219 (M2 phase-19 T2): the registered phase-0x1D slot runs the
 * ported act-7 body end-to-end on a live situation-8. The selection installs act
 * 7 and, per FU-219 (the native `0x8AEAE MOV EBX,1`), invokes it now — arm 0 runs
 * on the dispatch frame inside `phase_drive`, writing phase 0x10 (0x898DD) and
 * installing action row 0x19 on both teams' record 0 (0x89903/0x8990A); the live
 * clock-tail `fifa96_match_run_act_tick` then maps act 7 -> phase 0x1D arm 1,
 * which waits (tick_total < 0x78). Mutation-RED: a NULL phase-0x1D slot makes
 * the tick early-return at fifa96_match_act.c:63, so the stage/phase/record
 * assertions fail. */
static void test_situation8_dispatches_phase_1d_body(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1D].fn == fifa96_match_action_phase_1D);
  mr.state.period = 4;
  mr.state.phase = 2;
  mr.phase_machine.side_controlled = 0;
  mr.score[0] = 1u;
  mr.score[1] = 1u;
  mr.clock_period_ended = 1;
  assert(fifa96_match_run_phase_drive(&mr) == 1);
  /* T4/FU225-L5 (M2 phase-21): the sit-8 selection lands on the next frame's
   * top-of-call mode-0 producer (the native one-frame cadence). */
  assert(fifa96_match_run_phase_drive(&mr) == 0);
  /* FU-219 invoke-now: arm 0 ran on the sit-8 dispatch (inside phase_drive). */
  assert(mr.act.id == 7);                       /* sit 8 -> act 7 installed */
  assert(mr.act.stage == 1u);                   /* arm 0 ran + advanced (0x899ED) */
  assert(mr.state.phase == 0x10u);              /* arm-0 phase-0x10 write (0x898DD) */
  assert(mr.entities.team[0].records[0].code == 0x19u);  /* arm-0 row-0x19 install */
  assert(mr.entities.team[1].records[0].code == 0x19u);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* act 7 -> phase 0x1D arm 1 */
  assert(mr.act.stage == 1u);                   /* arm 1 waits (tick_total < 0x78) */
}

/* The begun-run 2 -> 0x0C -> 0 sequence: the derived write lands on the
 * completing frame (0x0C observable before the lifecycle resolve), and the
 * run_end teardown resets the match state to the native reset phase 0
 * (FUN_00073E28's derived surface, fifa96_match_state_init). v21 (M2 phase-21
 * T5, FU-228 §4.2): the clock is seeded PAST half-time (period 1) so the
 * completion is one that marks the lifecycle OVER (a half-time completion now
 * continues and would never resolve). */
static void test_phase_drive_begun_end_resets_phase(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  begin_isolated(&mr, f.engine);
  mr.state.period = 1;
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
  /* v22 (T4): the phase-0xC match-over dwell resolves the OVER screen. */
  for (int i = 0; i < DWELL_FRAMES && mr.lc.screen != FIFA96_MATCH_SCREEN_OVER; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
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
static void test_kickoff_enters_phase2_when_cutscene_isolated(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
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

/* M2 phase-18 T3 (FU-202 §6 rec (b)): the situation-0 kickoff cutscene runs
 * end to end on the live begun run — the re-placed match-init producer arms the
 * situation-0 act 0xA (surviving the begin-tail act reset), the first granted
 * frame's clock-tail act tick runs the wired phase-0x20 body (code 0x24 on all
 * 22 records), and the next frame's entity chain dispatches the wired row 0x24.
 * This is the RED/GREEN pin for the +2 census landing (the derived-kickoff
 * fixtures clear the arm to isolate the other path). */
static void test_situation0_kickoff_installs_code24(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.period_setup_live == 1u);       /* the live begin activates the producer */
  assert(mr.act.id == 0x0Au);               /* situation 0 armed into the first frame */
  one_granted_frame(&mr);
  assert(mr.act.id == 0x0Au && mr.act.stage == 1u);   /* the phase-0x20 stage advance */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      assert(mr.entities.team[t].records[i].code == 0x24u);   /* the 22-record install */
  assert((mr.dispatched_ok & (1ull << 0x24)) == 0u);       /* not dispatched yet */
  one_granted_frame(&mr);
  assert((mr.dispatched_ok & (1ull << 0x24)) != 0u);       /* the wired row 0x24 ran */
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

  begin_isolated(&mr, f.engine);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_sound_device == 0 && mr.score_display_event == 0);

  /* Goal 1: the begin-installed tracked-side pick is 1 (the [0x15B684]==0
   * flags-zero image default), so side 0's goal takes the untracked arm: the
   * max-diff bookkeeping runs (score[other 0]-score[tracked 1] = 1) and the
   * probe arm reads the staged probe 0 (no 0xD3). */
  assert(fifa96_match_run_score_event(&mr, 0, 0) == 0);
  assert(mr.score[0] == 1 && mr.score[1] == 0);
  assert(mr.score_last_side == 0 && mr.score_last_event == 0);
  assert(mr.score_max_diff == 1);
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
  begin_isolated(&mr, f.engine);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_last_side == -1 && mr.score_tracked_side == 1);
  assert(mr.score_max_diff == 0 && mr.score_last_event == 0);

  /* Situation 6 (goal): phase 5, no score write. */
  assert(fifa96_match_run_situation(&mr, 6) == 0);
  assert(mr.state.phase == 5u);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_last_side == -1 && mr.score_tracked_side == 1);
  assert(mr.score_max_diff == 0 && mr.score_last_event == 0);

  /* The goal-adjacent table-2 rows are phases only: 2 -> 3, 3 -> 4, 4 -> 8,
   * 5 -> 9, 0xB -> 2 (row 01's ported path); the writer cells never move. */
  assert(fifa96_match_run_situation(&mr, 2) == 0 && mr.state.phase == 3u);
  assert(fifa96_match_run_situation(&mr, 3) == 0 && mr.state.phase == 4u);
  assert(fifa96_match_run_situation(&mr, 4) == 0 && mr.state.phase == 8u);
  assert(fifa96_match_run_situation(&mr, 5) == 0 && mr.state.phase == 9u);
  assert(fifa96_match_run_situation(&mr, 0x0B) == 0 && mr.state.phase == 2u);
  assert(mr.score[0] == 0 && mr.score[1] == 0);
  assert(mr.score_last_side == -1 && mr.score_tracked_side == 1);
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
  begin_isolated(&mr, f.engine);
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
    assert(mr.score_last_side == -1 && mr.score_tracked_side == 1);
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
  begin_isolated(&mr, f.engine);

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
  begin_isolated(&mr2, f.engine);
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
  begin_isolated(&mr, f.engine);
  /* M2 phase-10 T2: with the L4.1 unbound walk live, every outfield record
   * runs the forced/chase tail, so the team-target/control roles can move on
   * their own (an AI record's row 04 legitimately claims control/ball).
   * Isolate this pad fixture the way the native walk does: stage the `+0x9A`
   * skip byte on the opponent team's outfield records for the whole run. */
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    mr.entities.team[1].records[i].skip_9a = 1;
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
    /* M2 phase-10 T2: with the L4.1 unbound walk live, every outfield record
     * runs the forced/chase tail, so the team-target/control roles can move on
     * their own (an AI record's row 04 legitimately claims control). Isolate
     * this pad fixture the way the native walk does: stage the `+0x9A` skip
     * byte on every outfield record except the taker. */
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
      for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
        if (t == 0u && (int32_t)i == taker) continue;
        mr.entities.team[t].records[i].skip_9a = 1;
      }
    }
    /* A carrier is the controlled actor natively; pinning it as the fixture
     * precondition keeps FUN_0007C990's type-5 keep arm (the code-5 carrier
     * state below) and the team_select_target skip stable. */
    mr.entities.controlled = taker;
    mr.entities.team[0].target = taker;
    mr.entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
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

    /* FU-159 §4.2 (T3, the ack fix): the kick is a one-shot install, but the
     * record sits on a terminal anim row (row 0, flags 0x07 -> +0x44 = 1), so
     * the next frame's stage-2 gate fires the native reset: the record leaves
     * code 7 into the phase-2 forced decision (code 4 here) and the stage
     * clears. Previously +0x44 staged 0 and the record stuck at stage 2 —
     * the second-kick blocker this task removes. */
    one_granted_frame(&mr);
    assert(rec->row44 == 1u);                   /* row 0 flags bit 0 */
    assert(rec->code == 4u);                    /* the forced decision */
    assert(rec->stage92 == 0u);

    /* T3 review minor 7: the literal second kick in the same possession. The
     * reset freed the record; re-stage the carrier state through the real
     * installer (the ball-recovery producer is the unported possession
     * machinery) and run the same live release path again: the code-1
     * released row installs 7 invoke-now, the row-07 machine stages a second
     * ball pair on the same record in the same match. */
    assert(fifa96_match_entities_install(rec, 2, 5, 0) == 1);
    assert(rec->code == 5u);
    mr.entities.controlled = taker;
    mr.entities.team[0].target = taker;
    fifa96_ball_pair_clear(&mr.entities.ball.pair);   /* consume the 1st staging */
    {
      fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
      assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    one_granted_frame(&mr);
    assert(rec->code == 7u);                    /* the second kick installed */
    assert(mr.entities.ball.pair.actor == taker);
    assert(mr.entities.ball.pair.code != 0u);   /* a second staging */
    assert(mr.dispatched_ok & (1ull << 0x07u));
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
  /* M2 phase-10 T2: with the L4.1 unbound walk live every outfield record runs
   * the machine; isolate this fixture to the slot record (native `+0x9A`). */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 0u && i == 1u)) mr.entities.team[t].records[i].skip_9a = 1;
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
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 0u && i == 1u)) mr.entities.team[t].records[i].skip_9a = 1;
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
  assert(fifa96_camera_init(&mr.render.camera, 0x111, 0, 0x333) == FIFA96_OK);
  mr.input_state[0] = 0x10;          /* KICK held: mapped +0x10, no direction */
  one_granted_frame(&mr);            /* press edge: scan, no arm */
  assert(e->target_x == 0 && e->target_z == 0);
  one_granted_frame(&mr);            /* no edges + prev_mapped 0x10: the arm */
  assert(e->target_x == 0x111);
  /* The Y follows the camera pos_y at the arm; with no live event height the
   * L14.7 producer leaves it 0 (a nonzero Y here would be the row-04 event's
   * height-0 clamp to the live camera Y plus the FUN_00070B94 arc). */
  assert(e->target_y == 0);
  assert(e->target_z == 0x333);
  assert(e->timer93 == 0x10u);       /* 0x79B58 (+0x99 staged 0) */
}

/* M2 phase-10 T2 / FU-75 L4.1: the unbound-record walk. The native
 * FUN_0008D8EC walk (`0x8DB2E..0x8DB5F`, first-hand disassemble_bytes this
 * task) calls FUN_0007CA54 for every outfield record 1..10; the forced
 * decision FUN_0007C990 and the code-8 chase gate `0x7CC93..0x7CD24` run for
 * records with no control slot too. On BASE the engine seam only ran the
 * machine for the slot-bearing record, so an unbound team target kept its
 * entry code and no row dispatched. */
static void test_unbound_team_target_forced_installs_row04(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  e = &mr.entities.team[0].records[1];
  e->has_slot = 0;                   /* the walk's new scope */
  e->active = 1;
  e->code = 3;                       /* flat[0x110680+3] & 1 != 0 */
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 1;    /* [team+0x7B2] = rec -> forced 4 */
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 1;        /* keeps team_select_target off */
  one_granted_frame(&mr);
  assert(e->code != 3u);             /* the forced 4 install (or row-04 coda) */
  assert((mr.dispatched_ok & (1ull << 0x04u)) != 0u);
}

/* The code-8 chase gate (`0x7CC93..0x7CD24`) requires `[rec+0x20] == 0` (the
 * unbound arm), phase 2, the type bit, not team target/second, lane < 0x50,
 * ball height < 0x30, a present user on the other side, `word[+0x81] == 0`
 * and `[rec+0x5D] == 0`. The same record with a bound slot must NOT chase. */
static void test_unbound_chase_installs_and_runs_row08(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  e = &mr.entities.team[0].records[1];
  e->has_slot = 0;
  e->active = 1;
  e->code = 3;
  e->lane = 0;                       /* distance 0 < 0x50 */
  e->timer81 = 0;
  e->pos_y = 0;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 2;    /* not rec */
  mr.entities.team[0].second = 3;    /* not rec */
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 11;       /* team 1: user present, side differs */
  mr.entities.ball.y = 0x10;         /* camera < 0x30 */
  one_granted_frame(&mr);
  assert(e->code == 8u);             /* the chase install */
  assert((mr.dispatched_ok & (1ull << 0x08u)) != 0u);
  /* the bound twin: the chase's `[rec+0x20] == 0` gate refuses. */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  e = &mr.entities.team[0].records[1];
  e->has_slot = 1;
  e->active = 1;
  e->code = 3;
  e->lane = 0;
  e->timer81 = 0;
  e->pos_y = 0;
  mr.slot.entity = 1;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 2;
  mr.entities.team[0].second = 3;
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 11;
  mr.entities.ball.y = 0x10;
  /* isolate the fixture to record 1 (the other unbound records would chase). */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 0u && i == 1u)) mr.entities.team[t].records[i].skip_9a = 1;
  one_granted_frame(&mr);
  assert(e->code != 8u);
  assert((mr.dispatched_ok & (1ull << 0x08u)) == 0u);
}

/* M2 phase-10 T2 (FU-148 §12/§13): the config-cell producer path. The
 * front-end options block (FUN_0001C728 / FUN_0001CAEC) stages the eight
 * cells FUN_0001C9BC consumes, so the reflect / interpolation / walk-gate bits
 * now fire from real settings. */
static void test_input_options_producer_sets_range_bits(void) {
  struct fifa96_match_run mr;
  int32_t options[FIFA96_INPUT_OPTION_COUNT];
  fifa96_match_run_init(&mr);
  for (uint32_t i = 0; i < FIFA96_INPUT_OPTION_COUNT; i++) options[i] = 0;
  options[1] = 2;                    /* 0x105278 == 2 -> the walk gate bit */
  options[2] = 1;                    /* 0x10527C != 0 -> the reflect bit */
  assert(fifa96_match_run_set_input_options(&mr, options) == FIFA96_OK);
  assert((mr.input_range[0] | mr.input_range[1]) & 4u);
  assert(mr.render.input_bit0 == 1);
  assert(mr.render.input_bit2 == 0);
  options[1] = 1;                    /* 0x105278 == 1 -> the interpolation bit */
  options[2] = 0;
  assert(fifa96_match_run_set_input_options(&mr, options) == FIFA96_OK);
  assert((mr.input_range[0] | mr.input_range[1]) & 2u);
  assert(mr.render.input_bit2 == 1);
  assert(mr.render.input_bit0 == 0);
  assert(fifa96_match_run_set_input_options(&mr, NULL) == -FIFA96_ERR_INVALID);
}

/* M2 phase-10 T2 (FU-147 §3.3 leg 7): the FUN_0007BF20 tail's per-record
 * camera event (`0x7C7D3..0x7C8FC`, CALL 0x70C08) now runs in the frame. The
 * fixture satisfies every outer gate (lane 0 <= 0x10 and < bound, both
 * 0x157820/22 flags clear, bearing 5 > 4, camera height 0x10, no tracked
 * record, the +0x5D band) on a paused camera so the lane refresh keeps lane 0;
 * the record event's reflect arm re-seeds the event height to the ball height.
 * M2 phase-11 T1: the height word is the live `dword[0x157750]` = the camera
 * Y (`camera.pos_y`, the FUN_00070B94 arc producer); this fixture stages the
 * value directly (the producer-driven wake is the sibling test below). */
static void test_record_event_tail_gate_fires_in_frame(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  e = &mr.entities.team[0].records[1];
  e->has_slot = 0;
  e->active = 1;
  e->code = 3;
  e->lane_x = 0x20;                  /* old lane: the fresh 0 < 0x20 */
  e->bound = 0x20;
  e->pos_y = 0;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 2;    /* not rec: no forced/target churn */
  mr.entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 11;       /* team 1: the chase/user gates */
  mr.render.camera.pos_y = 0x10;     /* 0x157750 (T1/L14.7) */
  mr.render.camera.paused = 1;       /* no integration: lane stays 0 */
  mr.render.camera.speed = 5;        /* [0x1577BE] bearing > 4 */
  mr.render.camera.event_param = 0;
  /* isolate the fixture to record 1 (other records' rows would move the
   * camera event state the gate reads). */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 0u && i == 1u)) mr.entities.team[t].records[i].skip_9a = 1;
  one_granted_frame(&mr);
  assert(mr.render.camera.event_param == 0x10);
}

/* M2 phase-11 T1 (FU-148 §14.5 / L14.7): the height producer wakes the same
 * tail gate. The camera is live (not paused): the ported FUN_00071C94 event
 * setter seeds the event (height 0x20, F2 = 21, f8 = 42 -> vel_z 6 > the 4
 * bearing gate), the FUN_00070B94 arc raises `pos_y` on the frame's
 * integrator store, and the record event's reflect arm then re-seeds
 * `event_param` to that live Y. On BASE the gate read the (zero) ball record
 * height and skipped. */
static void test_record_event_gate_live_from_height_producer(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.state.period_length = 90;
  e = &mr.entities.team[0].records[1];
  e->has_slot = 0;
  e->active = 1;
  e->code = 3;
  e->lane_x = 0x20;
  e->bound = 0x20;
  e->pos_y = 0;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 2;
  mr.entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.controlled = 11;
  assert(fifa96_camera_event_set(&mr.render.camera, 0, 0xFC, 0x20, 0) ==
         FIFA96_OK);
  assert(mr.render.camera.pos_y == 0);
  assert(mr.render.camera.event_param == 0x20);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 0u && i == 1u)) mr.entities.team[t].records[i].skip_9a = 1;
  one_granted_frame(&mr);
  /* The arc raised the Y on this frame's integrator store (t = 2) and the
   * record event's reflect arm re-seeded event_param to the live height (the
   * arm then re-seeds the camera velocities through its nested event set, so
   * the post-frame speed is the reflected jitter, not the 6 the gate read). */
  assert(mr.render.camera.pos_y > 0);
  assert(mr.render.camera.event_param == (uint16_t)mr.render.camera.pos_y);
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
  begin_isolated(&mr, f.engine);
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
  begin_isolated(&mr, f.engine);
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

/* M2 phase-11 T2 (FU-148 §16, L14.8): the FUN_0008BAF0 ball machine over the
 * frame body. With the fresh camera triple (0,0,0) the native phase-2
 * (period <= 3) lead at 0x8BB55..0x8BBFC is
 *   tx = -3*camZ/8 ± 0x120 = 0x120, tz = 3*camZ/4 ± 0x120 = 0x120
 * (the offsets apply because |(dx,dz)| = 0 < 0x120; `[0x15884E] = 7`), the
 * metric direction to (0x120, 0x120) is the 45° heading 0x80, and the tail's
 * sine velocity targets are (5, 5) — the ±1 ramp and the
 * `x += vel_x*delta; z += vel_z*delta` integration roll the ball record the
 * render slot 22 stages. First-hand windows: `disassemble_function 0x8BAF0`
 * (0x8BB55..0x8BBFC, 0x8BDEC..0x8C052). */
/* Fix round 2: the L14.8.c max-clamp shift count pinned at its own level.
 * The machine build carries no sanitizers and every hardware shift masks the
 * count (x86 `SAR AX,CL`, this host's aarch64 `ASR w,w`), so the `& 0x1F` is
 * behaviorally invisible to a machine-level frame test — this direct pin is
 * the only discriminating check (removing the mask returns 1023 for 0xFFFF).
 * Native `0x8BF41..47`: `MOV ECX,[EBP+0x76]; SAR ECX,6; SAR AX,CL`. */
static void test_ball_head_error_shift_mask(void) {
  assert(fifa96_match_run_ball_head_error_shift(0) == 0);
  assert(fifa96_match_run_ball_head_error_shift(0x3F) == 0);
  assert(fifa96_match_run_ball_head_error_shift(0x40) == 1);
  assert(fifa96_match_run_ball_head_error_shift(0x80) == 2);
  assert(fifa96_match_run_ball_head_error_shift(0xFFFF) == 31);
}

static void test_ball_machine_phase2_open_play_lead(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.entities.ball.head_gate == 2);   /* FUN_000886D4 row 3 & 2 */
  assert(mr.entities.ball.x == 0 && mr.entities.ball.z == 0);
  mr.state.phase = 2;
  mr.state.period_length = 90;               /* no period end in 10 ticks */

  one_granted_frame(&mr);                    /* granted frame 1 */
  assert(mr.entities.ball.machine_ran == 1);
  assert(mr.entities.ball.target_x == 0x120);            /* 0x8BBF0 */
  assert(mr.entities.ball.target_y == 0);
  assert(mr.entities.ball.target_z == 0x120);
  assert(mr.entities.ball.max_speed == 7);               /* 0x8BBE6 */
  /* Frame-1 metric source: (0x120, 0x120) from the record — dist 0x18C and
   * the angle (FUN_000CD474) of the equal pair is the 45° heading 0x80. */
  assert(mr.entities.ball.metric == 0x18C);
  assert(mr.entities.ball.target_heading == 0x80);
  assert(mr.entities.ball.heading == 0x40);              /* clamped to turn */
  /* The visual byte: the native `[EAX+0x1104CF] dword >> 24` (scale-1, no
   * SIB) = the 0x1104D2 byte table; idx (0x40)>>5 = 2 -> 0. */
  assert(mr.entities.ball.visual_dir == 0);
  /* Frame 1's sine velocity targets are (5, 5); the speed/2 ramp timer starts
   * at 0, so both axes step ±1 and integrate by delta 2. */
  assert(mr.entities.ball.vel_x == 1 && mr.entities.ball.vel_z == 1);
  assert(mr.entities.ball.speed == 1);
  assert(mr.entities.ball.x == 2 && mr.entities.ball.z == 2);

  one_granted_frame(&mr);                    /* granted frame 2 */
  one_granted_frame(&mr);                    /* granted frame 3 */
  /* By frame 3 the ramp has stepped to 3 on both axes (heading toward the
   * 5-unit sine target) and the ball has rolled 2+4+6 = 12 units. */
  assert(mr.entities.ball.vel_x == 3 && mr.entities.ball.vel_z == 3);
  assert(mr.entities.ball.speed == 3);
  assert(mr.entities.ball.x == 12 && mr.entities.ball.z == 12);

  /* L14.8.c synthetic pin (the 0x158862 FUN_0008C068 head-error producer is
   * unported; its image default 0 leaves the clamp latent). Frames 4..6 reach
   * the 5-unit sine target (frame 5's speed/2 timer holds), then
   * `head_error = 0xFFFF` -> `(0xFFFF >> 6) & 0x1F = 31` -> `max = 7 >> 31 =
   * 0`: the velocity target collapses to 0 and the ramp steps down to 4 by
   * frame 8 (with head_error 0 the axes would hold at 5/5, speed 6). */
  one_granted_frame(&mr);                    /* granted frame 4: 3 -> 4 */
  one_granted_frame(&mr);                    /* granted frame 5: timer holds */
  one_granted_frame(&mr);                    /* granted frame 6: 4 -> 5 */
  assert(mr.entities.ball.vel_x == 5 && mr.entities.ball.vel_z == 5);
  assert(mr.entities.ball.speed == 6);       /* metric(5,5) = 6 */
  mr.entities.ball.head_error = 0xFFFF;
  one_granted_frame(&mr);                    /* frame 7: max 0, timer 2 -> hold */
  assert(mr.entities.ball.vel_x == 5 && mr.entities.ball.vel_z == 5);
  one_granted_frame(&mr);                    /* frame 8: timer 4 > 3 -> 5 -> 4 */
  assert(mr.entities.ball.vel_x == 4 && mr.entities.ball.vel_z == 4);
  assert(mr.entities.ball.speed == 5);       /* metric(4,4) = 5 */
  mr.entities.ball.head_error = 0;

  /* The record the render stage consumes moved with the machine. */
  if (mr.render.enabled) {
    assert(mr.render.entities[22].stage.pos.x == mr.entities.ball.x);
    assert(mr.render.entities[22].stage.pos.z == mr.entities.ball.z);
  }
}

/* M2 phase-12 T2 (FU-154 §1/§5.1): the render's own clean projection of a
 * staged slot — the FU-88 matrix from the live view angles, the reciprocal
 * tables, the FU-85 camera staging of the FU-71 triple and the 16.16 screen
 * transform (the same primitive sequence as test_engine_m2's
 * m2_slot_clean_screen, which the M2 tape uses for the same ball slot).
 * Returns the near-gate drawable flag and the screen x in 16.16 so the caller
 * can apply the canvas bound. */
static int live_slot_clean_screen(const struct fifa96_match_run *mr,
                                  const struct fifa96_surface *s, uint32_t slot,
                                  int32_t *out_x) {
  const struct fifa96_match_run_render *r = &mr->render;
  fifa96_render_pos cam_src, cam;
  fifa96_render_slot staged;
  fifa96_projection_point center, out;
  fifa96_projection_vec rel, rot;
  int32_t matrix[9];
  int32_t recip_x[FIFA96_PROJECTION_RECIP_COUNT];
  int32_t recip_y[FIFA96_PROJECTION_RECIP_COUNT];
  uint8_t visible = 0;
  assert(r->entity_count > slot);
  if (slot >= r->entity_count) return 0;
  assert(fifa96_projection_matrix(r->yaw, r->pitch, matrix) == FIFA96_OK);
  assert(fifa96_projection_reciprocal(s->width, recip_x) == FIFA96_OK);
  assert(fifa96_projection_reciprocal(s->height, recip_y) == FIFA96_OK);
  cam_src.x = r->camera.pos_x;
  cam_src.y = r->camera.pos_y;
  cam_src.z = r->camera.pos_z;
  assert(fifa96_render_camera_stage(&cam_src, &cam) == FIFA96_OK);
  assert(fifa96_render_slot_stage(&r->entities[slot].stage, &staged) == FIFA96_OK);
  if (r->window.box.w > 0 && r->window.box.h > 0) {
    center.x = r->window.cx_fix;
    center.y = r->window.cy_fix;
  } else {
    center.x = (s->width >> 1) << 16;
    center.y = (s->height >> 1) << 16;
  }
  rel.x = (int32_t)((uint32_t)staged.pos.x - (uint32_t)cam.x);
  rel.y = (int32_t)((uint32_t)staged.pos.y - (uint32_t)cam.y);
  rel.z = (int32_t)((uint32_t)staged.pos.z - (uint32_t)cam.z);
  assert(fifa96_projection_transform(matrix, &rel, &rot) == FIFA96_OK);
  assert(fifa96_projection_screen(recip_x, recip_y, &center, &rot, &out,
                                  &visible) == FIFA96_OK);
  if (out_x) *out_x = out.x;
  return visible;
}

/* M2 phase-11 T2 (FU-148 §16, L14.8): the live ISO open-play producer. The
 * natural kickoff + KICK press reaches phase 2 (the existing live chain); from
 * there the ball machine alone advances the ball record 0x15880C/14 — the
 * T2-visible ball source the FU-120 writer census leaves as the sole per-frame
 * producer. Pins: the machine ran live, the record moved off the kickoff spawn
 * with the phase-2 lead selected, and the staged slot 22 follows it.
 * M2 phase-12 T2 (FU-154 §4/§5.1, L14.8.e): the visible-ball acceptance — the
 * native view seed (reset_render's 0xBF9C/200) projects the staged slot 22 on
 * canvas through the kick-roll window (measured below). */
static void test_live_ball_record_produced_in_open_play(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  int32_t pre_x = 0, pre_z = 0;
  int32_t start_x, start_z;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  /* M2 phase-12 T2: stage the derived art pair exactly as the ISO bridge does
   * at match start (fifa96_match_bridge_from_frontend) so the scene composer
   * stages the live slots — the render's real input for the on-canvas
   * projection below. */
  assert(fifa96_match_run_stage(&mr, f.engine->surface, "/ART/PLAYART.PVI",
                                "/ART/GAMEART0.PVI") == 0);
  assert(mr.render.enabled == 1);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  /* The kickoff phases hold the committed spawn; the machine only starts
   * rolling once the phase flips (the last pre-flip frame observes it). */
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) {
    pre_x = mr.entities.ball.x;
    pre_z = mr.entities.ball.z;
    one_granted_frame(&mr);
  }
  assert(mr.state.phase == 2u);
  assert(pre_x == FIFA96_MATCH_ENTITY_KICKOFF_BALL_X);
  assert(pre_z == 0);
  start_x = mr.entities.ball.x;
  start_z = mr.entities.ball.z;
  /* FU-154 §4/§5.1 (M2 phase-12 T2, L14.8.e): the visible-ball window. With
   * the native view seed the render's own clean projection of the staged ball
   * slot lands on canvas for all 16 kick-roll window frames (measured x'
   * 152 -> 4, exits left at the 17th); with the stand-in yaw 0 the same
   * projection was 0/90 frames on canvas (the settled lead projects
   * x' = 480). The camera stays the (0,0,0) stand-in here — the live pan is
   * the T1/R1 coupling (FU-154 §3, R2-L4). */
  int on_canvas_frames = 0;
  int32_t first_on_x = 0;
  for (int i = 0; i < 30; i++) {
    one_granted_frame(&mr);
    if (i < 16) {
      int32_t sx = 0;
      if (live_slot_clean_screen(&mr, f.engine->surface, 22u, &sx) && sx >= 0 &&
          sx < (f.engine->surface->width << 16)) {
        if (on_canvas_frames == 0) first_on_x = sx;
        on_canvas_frames++;
      }
    }
  }
  assert(on_canvas_frames == 16);
  assert(first_on_x >> 16 == 152);   /* the roll-in x' pin */
  assert(mr.entities.ball.machine_ran == 1);
  assert(mr.entities.ball.x != start_x || mr.entities.ball.z != start_z);
  /* The phase-2 lead is the live target (the camera triple at the kickoff
   * default (0,0,0) -> the 0x120/0x120 spot). */
  assert(mr.entities.ball.target_x == 0x120 || mr.entities.ball.target_z == 0x120);
  if (mr.render.enabled) {
    assert(mr.render.entities[22].stage.pos.x == mr.entities.ball.x);
    assert(mr.render.entities[22].stage.pos.z == mr.entities.ball.z);
  }
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* M2 phase-9 T2 (FU-142 OL-63): the live carrier producers. The natural ISO
 * kickoff + held UP movement walks the slot record through row 02's install 4
 * into row 04's half-line coda (`0x7F133` install 5, invoke-now) on the real
 * dispatch path; the now-wired row 05 then claims the possession block
 * (`0x7F1FF`, `[0x158724]` = the pool ball carrier), binds the team target,
 * requests the controlled actor and writes the dribble dir bytes, and the pad
 * KICK release runs row 07's kick machine off the live record (row 07 ->
 * `fifa96_ball_kick_target` -> the staged ball pair).
 *
 * Discriminating vs BASE: with row 05 unwired the whole sequence still runs
 * (the kick is install-driven), but the possession producers never fire —
 * `ball.carrier` stays NONE and `[0x157A83]` is never re-bound, so the claim
 * assertions below fail at BASE. */
static void test_live_carrier_claim_and_kick(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int32_t id;
  int saw5 = 0;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
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
  assert(mr.entities.ball.carrier == FIFA96_MATCH_ENTITY_NONE);

  /* Held UP walks the record into row 04's half-line coda; the coda installs
   * code 5 (invoke-now) on the real dispatch path. */
  {
    fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
    assert(fifa96_match_run_input(&mr, &up, 1) == 0);
  }
  for (int i = 0; i < 60 && saw5 == 0; i++) {
    one_granted_frame(&mr);
    if (rec->code == 5u) saw5 = 1;
  }
  assert(saw5 == 1);
  assert(mr.entities.ball.carrier == FIFA96_MATCH_ENTITY_NONE);   /* not yet */

  /* The next granted frame dispatches row 05: the claim, the team-target
   * bind, the controlled actor and the slot-dir bytes (lane 2 <= 0x40 and
   * lane <= bound on the install frame). */
  one_granted_frame(&mr);
  assert(rec->code == 5u);
  assert(mr.entities.ball.carrier == id);            /* 0x7F1FF claim */
  assert(mr.entities.controlled == id);              /* 0x7F291 set_control */
  assert((int8_t)mr.entities.ball.pos_dir_x == rec->dir_x);
  assert((int8_t)mr.entities.ball.pos_dir_z == rec->dir_z);
  assert(mr.entities.ball.pos_dir_x == 1 && mr.entities.ball.pos_dir_z == 0);
  assert(rec->ran == 1u);                            /* 0x7F19F latch */

  /* The pad kick: release UP, press KICK (no install), release KICK -> the
   * code-1 released row installs 7 (invoke-now) and the same frame runs the
   * ported kick machine off the live carrier record. */
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(rec->code == 5u);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  assert(rec->code == 7u);
  assert((mr.dispatched_ok & (1ull << 0x07u)) != 0u);
  assert(mr.entities.ball.pair.actor == id);         /* the live carrier */
  assert(mr.entities.ball.pair.traj != 0);           /* the ball impulse */
  assert(mr.entities.ball.carrier == id);            /* possession persists */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* M2 phase-10 T1 (FU-148 §14 / FU-142 OL-88): the live receive arm. The same
 * live carrier chain as T2's `test_live_carrier_claim_and_kick`, but the pad
 * PASS release stages the row-07 carry-table row (code 13 — a receive code, so
 * the staging tail's receive gate fires). The engine's FUN_0007A084 arm then
 * runs on the real path: the camera event binds the actor (tracked) and stores
 * the trajectory height, the controlled actor is released, the staged event is
 * acknowledged and the receiver scan writes the team target. The trajectory
 * stays 0 on this row (the carry row adds nothing), so the follow arm's
 * height > 0xF0 gate still refuses — the visible follow needs the fixture
 * chain above. Discriminating vs BASE: without the T1 wiring the receive
 * fields stay untouched (`ack` 0, `receiver` NONE, `controlled` keeps the
 * carrier). */
/* M2 phase-11 T1 (FU-75 §1.4 / FU-148 §14.5/§14.6, L14.6/L14.7): the live
 * long-ball chain. The probe-derived sequence (first-hand this task):
 *
 *   1. kickoff -> phase 2 (slot rec 9, team target 9, controlled 10);
 *   2. the record walks (code 4) while the ported event setter raises the
 *      camera Y (the L14.7 FUN_00070B94 arc, live: pos_y crosses 0x70);
 *   3. the 0x40 LONG_BALL key press+release runs the record's released code-0
 *      row -> FUN_0007CD60 -> `[team+0x7CB] = rec`, `[team+0x820] = 0x3C`
 *      (the request arm; first arm, `rec == [team+0x7B2]`);
 *   4. the UP walk claims (code 5, carrier = rec) inside the 0x3C window;
 *   5. the mid-kick frame boundary: `pos_y = 0x70` (the band gate admits
 *      exactly d == 0x70) and the ported event cleared so the pin holds;
 *   6. the KICK press/release runs the code-7 machine; stage 1 reads the
 *      live `is_team_cb` -> mode 0x40; the band-2 row (code 4) stages; the
 *      code-4 RNG arm gives traj 0x90 + (r&7)*0x10 = 0x100 (the run's RNG is
 *      seeded 0, so the draw is deterministic);
 *   7. the receive arm (code 4 = `[0x1104BB+4] == 0`) binds the camera event
 *      (event_param = 0x100) and the tracked record, acks and picks the
 *      receiver;
 *   8. the every-frame follow arm consumes the > 0xF0 height and takes the
 *      tracked slot dirs into the camera velocity — the visible follow.
 *
 * Discriminating vs BASE: without the T1 wiring the 0x40 key is unmapped, the
 * request cannot arm (`timer7cb` stays 0), and the kick stages the idle slot
 * row (no 0x40 flags, no receive/event). */
static void test_live_long_ball_mode40_band2_follow(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int32_t id;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  id = mr.slot.entity;
  assert(id == 9);
  rec = &mr.entities.team[0].records[9];
  /* (2) walk four frames and raise the camera Y through the producer (the
   * band-2 gate proof for the kick below). */
  {
    fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
    assert(fifa96_match_run_input(&mr, &up, 1) == 0);
  }
  for (int i = 0; i < 4; i++) one_granted_frame(&mr);
  assert(fifa96_camera_event_set(&mr.render.camera, 0, 0, 0x90, 0) == FIFA96_OK);
  for (int i = 0; i < 40 && mr.render.camera.pos_y < 0x70; i++) one_granted_frame(&mr);
  assert(mr.render.camera.pos_y >= 0x70);          /* the live arc */
  assert(mr.render.camera.event_param == 0x90);    /* still the staged event */
  /* (2b) restore the open-play camera state (Y 0) for the claim: after
   * FU-171 §7 rec 1 the row-04 half-line admission (0x7EDD8) reads the live
   * camera Y, and it is admitted only while `[0x157750] <= 0x50`. The Y raise
   * above was the producer proof; the claim itself is a camY-0 chain. */
  assert(fifa96_camera_init(&mr.render.camera, mr.render.camera.pos_x, 0,
                            mr.render.camera.pos_z) == FIFA96_OK);
  /* (3) the long-ball request through the live input rows. */
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  {
    fifa96_platform_key lb = {FIFA96_ENGINE_KEY_LONG_BALL, 1};
    assert(fifa96_match_run_input(&mr, &lb, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  assert(mr.entities.team[0].timer7cb == (uint32_t)id);
  assert(mr.entities.team[0].timer820 == 0x3C);
  /* (4) walk to the claim inside the request window (camY == 0). */
  {
    fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
    assert(fifa96_match_run_input(&mr, &up, 1) == 0);
  }
  for (int i = 0; i < 60 && mr.entities.ball.carrier != id; i++)
    one_granted_frame(&mr);
  assert(mr.entities.ball.carrier == id);
  assert(rec->code == 5u);
  /* (4c) isolate the claim record: at camY 0 every other record's row-04
   * half-line admission is open too, and their events/merge requests move the
   * control slot (native behavior; the pre-re-stage chain ran the claim at
   * camY >= 0x70 so their admission was closed by the `ball.y` stand-in). The
   * fixture keeps only the claimed record live for the kick sequence. */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 0u && i == 9u)) mr.entities.team[t].records[i].skip_9a = 1;
  mr.slot.entity = id;                 /* the claimed record owns the pad */
  /* (4d) FU168-L5 (T2, open leg): the natural open-play camera-Y producer.
   * The band-2 kick needs `[0x157750] >= 0x70` at the kick frame; the native
   * producers are the restart/keeper cuts (the throw-in stage-3 cut is
   * handler-proven by `test_live_throw_in_camera_y_cut`, but transplanting it
   * here re-anchors the chain on the restart camera — T3's chain) and the
   * arc, which needs an existing event. Until one lands, the kick frame's
   * camera Y stays the pinned stand-in (per-attempt below). */
  /* (5/6) the band boundary and the mode-0x40 kick. The first attempt after
   * a held direction leaves the slot hold-mask dirty (the released word is
   * swallowed); the NULL grant cleans it. */
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  for (int attempt = 0; attempt < 6; attempt++) {
    mr.render.camera.event_param = 0;      /* the pinned-Y staging (FU168-L5) */
    mr.render.camera.pos_y = 0x70;
    mr.entities.ball.pair.code = 0;
    {
      fifa96_platform_key k = {FIFA96_ENGINE_KEY_KICK, 1};
      assert(fifa96_match_run_input(&mr, &k, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    one_granted_frame(&mr);
    one_granted_frame(&mr);
    if (mr.entities.ball.pair.code == 4u) break;
  }
  assert(mr.entities.ball.pair.flags == 0x40);     /* the live mode */
  assert(mr.entities.ball.pair.code == 4u);        /* the receive row */
  assert((int16_t)mr.entities.ball.pair.traj == 0x100);   /* > 0xF0 */
  /* (7) the receive arm ran. */
  assert(mr.entities.ball.pair.ack == 1);
  assert(mr.entities.ball.pair.receiver >= 0);
  assert(mr.render.camera.tracked == id);
  assert(mr.render.camera.event_param == 0x100);
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
  /* (8) the follow arm takes the tracked slot dirs into the camera velocity. */
  rec->dir_x = 1;
  rec->dir_z = 0;
  assert(fifa96_match_run_camera_follow(&mr) == 1);
  assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == 0);
  assert(mr.render.camera.speed != 0);
  /* (9) FU-171 §3.3 / §7 rec 4 (T2, FU171-L1): the sustained window and the
   * pan-counter lockout through the every-frame path. The receive arm's setter
   * cleared `[0x157821]` (0x70153, camera.c:863), opening the follow window;
   * the call-site gate 1 (0x739C6) keeps it open until the first pan step
   * (FUN_000709D0 0x70A65) increments the counter (0x70A3F caps it at 100),
   * which locks `fifa96_match_run_camera_follow` out until the next setter
   * call. Per granted frame while open: the follow re-seeds the velocity from
   * the tracked record's slot dirs x15 (the rate arm, 0x71E4E..0x71EE5) and
   * the integrator advances pos_x by vel*delta; the pan step's ramp then
   * leaves a different velocity. The other records' per-record BF20 chase
   * events are isolated (native per-record behavior would re-arm the event
   * legitimately; the window itself is the subject). */
  {
    int window = 0;
    int32_t prev_pos_x = mr.render.camera.pos_x;
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
      for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
        if (!(t == 0u && i == 9u)) mr.entities.team[t].records[i].skip_9a = 1;
    assert(mr.render.camera.pan_counter == 0);
    assert((int16_t)mr.render.camera.event_param == 0x100);
    while (mr.render.camera.pan_counter == 0 && window < 120) {
      rec->dir_x = 1;                     /* the dispatch may rewrite the pool */
      rec->dir_z = 0;
      one_granted_frame(&mr);
      window++;
      if (mr.render.camera.pan_counter == 0) {
        assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == 0);
        assert(mr.render.camera.pos_x ==
               prev_pos_x + 15 * (int32_t)mr.state.frame_delta);
        prev_pos_x = mr.render.camera.pos_x;
      }
    }
    /* The derived window for the k=7 0x100 event at the pinned 0x70 camera Y:
     * F2 = ramp(0x100) = 0x94 - T[0x150] = 59, F4 = 2*F2 = 118,
     * F6 = F2 - ramp(0x100 - 0x70 = 0x90) = 59 - 45 = 14; the pan step fires
     * on the first frame with timer > F4 and timer advances by delta 2, so
     * the window is (118 - 14)/2 = 52 granted frames. */
    assert(window == 52);
    assert(mr.render.camera.pan_counter == 1);
    /* the lockout: the call-site gate 1 refuses until a new setter call. */
    assert(fifa96_match_run_camera_follow(&mr) == 0);
    for (int i = 0; i < 5; i++) {
      one_granted_frame(&mr);
      assert(mr.render.camera.pan_counter >= 1);
      assert(fifa96_match_run_camera_follow(&mr) == 0);
    }
    /* a new event re-opens the window (the setter's 0x70153 clear; the
     * tracked bind is the caller's 0x71D27 store after the reset, exactly as
     * the receive arm / row-04 arm bind). */
    assert(fifa96_camera_event_set(&mr.render.camera, 0x100, 0x100, 0x100, 0) ==
           FIFA96_OK);
    assert(fifa96_camera_set_tracked(&mr.render.camera, id) == FIFA96_OK);
    assert(mr.render.camera.pan_counter == 0);
    rec->dir_x = 1;
    rec->dir_z = 0;
    one_granted_frame(&mr);
    assert(mr.render.camera.pan_counter == 0);
    assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == 0);
  }
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

static void test_live_pass_kick_runs_receive_arm(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int32_t id;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  id = mr.slot.entity;
  assert(id == 9);
  rec = &mr.entities.team[0].records[9];
  {
    fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
    assert(fifa96_match_run_input(&mr, &up, 1) == 0);
  }
  int saw5 = 0;
  for (int i = 0; i < 60 && saw5 == 0; i++) {
    one_granted_frame(&mr);
    if (rec->code == 5u) saw5 = 1;
  }
  assert(saw5 == 1);
  one_granted_frame(&mr);
  assert(mr.entities.ball.carrier == id);
  assert(mr.entities.ball.pair.ack == 0);
  assert(mr.entities.controlled == id);
  /* PASS press (no install), release -> row 07's carry-table receive row. */
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  {
    fifa96_platform_key pass = {FIFA96_ENGINE_KEY_PASS, 1};
    assert(fifa96_match_run_input(&mr, &pass, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(rec->code == 5u);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  assert(rec->code == 7u);                        /* the kick row ran */
  assert(mr.entities.ball.pair.code == 13u);      /* the receive-coded row */
  assert(mr.entities.ball.pair.ack == 1);
  assert(mr.entities.ball.pair.receiver >= 0);    /* a teammate receiver */
  assert(mr.entities.ball.pair.receiver != id);
  assert(mr.render.camera.tracked == id);         /* 0x71D27 tracked bind */
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.entities.team[0].target == mr.entities.ball.pair.receiver);
  assert(mr.entities.ball.pos_release == 0x14);   /* the carrier release */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-157 §3.4/§5 (T1, R5's measured claim pan): the natural live chain —
 * kickoff -> KICK -> phase 2 -> the long-ball request -> walk UP to the claim —
 * fires the row-04 half-line event (outfield.c:686..703: the `has_slot`,
 * `ball_height == 0` arm seeds `slot_dir_x << 6`, event_code 0x1D),
 * `fifa96_camera_event_set` seeds the pan (the idle-height arm F8 = 0xC, so
 * `vel_x = 64/12 = 5`) and binds `tracked = 9` (0x71D27); the FUN_000736AC
 * integrator then pans `pos_x 0 -> 0x90` with the pan-step decay
 * 5 -> 3 -> 2 -> 1 -> 0 (pan_counter != 0, so FUN_00070544's slow path with
 * `k = pan_rate_lo = 24`: `vel = 24*vel/0x20`). One-shot, not a follow (R5).
 * Measured first-hand at HEAD by the t1probe scratch run and the R5 probes. */
static void test_live_claim_pan_row04_half_line(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int32_t id;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  id = mr.slot.entity;
  assert(id == 9);
  rec = &mr.entities.team[0].records[9];
  {
    fifa96_platform_key lb = {FIFA96_ENGINE_KEY_LONG_BALL, 1};
    assert(fifa96_match_run_input(&mr, &lb, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  one_granted_frame(&mr);
  {
    fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
    assert(fifa96_match_run_input(&mr, &up, 1) == 0);
  }
  for (int i = 0; i < 60 && mr.entities.ball.carrier != id; i++)
    one_granted_frame(&mr);
  /* the claim frame carries the row-04 half-line event: the pan seed is the
   * slot dir << 6 over the idle divisor 0xC (== 5 for dir_x 1), the event is
   * the idle-height call and 0x71D27 bound the record as tracked. */
  assert(mr.entities.ball.carrier == id);
  assert(rec->dir_x == 1 && rec->dir_z == 0);
  assert(mr.render.camera.vel_x == (int16_t)((rec->dir_x << 6) / 0xC));
  assert(mr.render.camera.vel_z == 0);
  assert(mr.render.camera.event_param == 0);
  assert(mr.render.camera.tracked == id);
  /* the integrator pan: pos_x walks monotonically to the measured 0x90 stop
   * with the pan-step decay 5 -> 3 -> 2 -> 1 -> 0. */
  {
    static const int16_t decay[5] = {5, 3, 2, 1, 0};
    int16_t expected = (int16_t)((rec->dir_x << 6) / 0xC);
    int32_t prev_pos = mr.render.camera.pos_x;
    int step = 0;
    assert(expected == decay[0]);
    for (int i = 0; i < 40; i++) {
      one_granted_frame(&mr);
      if (mr.render.camera.vel_x != expected) {
        step++;
        assert(step < 5);
        expected = mr.render.camera.vel_x;
        assert(expected == decay[step]);
      }
      assert(mr.render.camera.pos_x >= prev_pos);
      prev_pos = mr.render.camera.pos_x;
    }
    assert(step == 4);                              /* the whole decay ran */
    assert(mr.render.camera.vel_x == 0);
    assert(mr.render.camera.pos_x == 0x90);         /* the measured stop */
    assert((int16_t)mr.render.camera.speed == 0);
  }
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
  begin_isolated(&mr, f.engine);
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

/* M2 phase-12 T1 (FU-153, leg 15.a): the live airborne seed and the live
 * band-2 crossing with the follow arm held.
 *
 * Two chains, because the engine's row-07 machine cannot complete a second
 * kick in one possession: its stage 2 gates on the native `+0x44` animation
 * ack (`kick_machine_07` case 2), whose producer is the unported
 * animation/event pipeline (the engine `row44` stays 0) — leg T1-L2. So:
 *
 *  A) the live seed: kickoff -> phase 2 -> the live UP claim -> the FUN_0006E8E8
 *     aux starter puts the slot record on the verified jump-aux row (0x5E
 *     frame 0, aux 0x0E; the live row selector is leg T1-L1) with the BF20 arc
 *     running live -> the KICK stages the **height-table** code-4 receive with
 *     the mode-0x40 draw `0x90 + (rng&7)*0x10`; the deterministic RNG pump is
 *     searched (bounded) for the exact-hit `0xE0` (k=5) with an even arc grid
 *     (the `0x70` boundary sample exists; the camera event is set by the
 *     receive arm itself) — no pinned pos_y;
 *
 *  B) the live crossing: a fresh chain; the camera arc is the disclosed
 *     bootstrap stand-in (`fifa96_camera_event_set H = 0xE0`, the live camera
 *     bootstrap producer is leg T1-L3) because the height-table route needs
 *     `d >= 0x38`; the first kick is timed so the row-07 install frame is the
 *     exact `0x70` sample (t=96, the stage-0 gate admits and the same-frame
 *     stage 1 reads the same ball height) -> band 2 -> the active class-2 row
 *     16 code 4 -> the RNG pump search lands the k=7 follow draw -> traj/event
 *     0x100 -> the every-frame follow arm takes the tracked slot dirs into the
 *     camera velocity. */
static void test_live_airborne_long_ball_seed_then_band2_follow(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int32_t id;
  int seed_ok = 0;
  int cross_ok = 0;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  /* ===== A: the live airborne seed. ===== */
  for (int pump = 0; pump < 64 && seed_ok == 0; pump++) {
    uint16_t dump;
    begin_isolated(&mr, f.engine);
    for (int i = 0; i < pump; i++)
      assert(fifa96_rng_step(&mr.rng, &dump) == FIFA96_OK);
    for (int i = 0; i < 61; i++) one_granted_frame(&mr);
    {
      fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
      assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
    assert(mr.state.phase == 2u);
    id = mr.slot.entity;
    assert(id == 9);
    rec = &mr.entities.team[0].records[9];
    {
      fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
      assert(fifa96_match_run_input(&mr, &up, 1) == 0);
    }
    for (int i = 0; i < 80 && mr.entities.ball.carrier != id; i++)
      one_granted_frame(&mr);
    if (mr.entities.ball.carrier != id) {
      assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
      assert(fifa96_match_run_end(&mr) == 0);
      fifa96_match_run_init(&mr);
      continue;
    }
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    one_granted_frame(&mr);
    /* the live jump: the aux row -> the FUN_0006E8E8 arm -> the BF20 arc.
     * T3: staged through the bounded `record_anim_install` helper (the
     * FUN_0006E598 store core) instead of the raw field writes. */
    assert(fifa96_match_run_record_anim_install(&mr, rec, 0x5E, 0) == FIFA96_OK);
    for (int i = 0; i < 8 && rec->pos_y == 0; i++) one_granted_frame(&mr);
    if (rec->pos_y == 0) {
      assert(fifa96_match_run_end(&mr) == 0);
      fifa96_match_run_init(&mr);
      continue;
    }
    /* the T1-L3 bootstrap stand-in (leg): the height-table row needs
     * `d >= 0x38` and no ported producer raises [0x157750] off zero, so the
     * smallest exact-hit event H = 0x74 opens the camera window for the kick. */
    assert(fifa96_camera_event_set(&mr.render.camera, 0, 0, 0x74, 0) == FIFA96_OK);
    for (int i = 0; i < 60; i++) {
      if ((int16_t)mr.render.camera.pos_y >= 0x38) break;
      one_granted_frame(&mr);
    }
    /* T3 (FU-161 one-shot): the kick's height-table route gates on the
     * kicker's live `pos_y` (`fifa96_action_kick_event_row`, loader
     * `fifa96_action_handlers.c:113-117`: `event->height = actor->pos_y`, and
     * `event->height != 0` selects the height table). The T1 stand-in stayed
     * airborne only because the unmodeled one-shot re-fired the 0x0E arm every
     * frame; with the native one-shot the first arc has landed by now, so
     * re-stage the live jump right before the kick (the helper forces the aux
     * re-arm); the arc's ~8-frame life spans the KICK sequence below. */
    assert(fifa96_match_run_record_anim_install(&mr, rec, 0x5E, 0) == FIFA96_OK);
    for (int i = 0; i < 8 && rec->pos_y == 0; i++) one_granted_frame(&mr);
    assert(rec->pos_y != 0);
    {
      fifa96_platform_key lb = {FIFA96_ENGINE_KEY_LONG_BALL, 1};
      assert(fifa96_match_run_input(&mr, &lb, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    one_granted_frame(&mr);
    {
      fifa96_platform_key k = {FIFA96_ENGINE_KEY_KICK, 1};
      assert(fifa96_match_run_input(&mr, &k, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    for (int i = 0; i < 8 && mr.entities.ball.pair.code == 0; i++) {
      one_granted_frame(&mr);
    }
    /* the airborne discriminator: the kick staged code 4 with d < 0x70 — the
     * band table would give code 6 there, so the height table ran. */
    assert(mr.entities.ball.pair.code == 4u);
    assert(mr.entities.ball.pair.flags == 0x40u);
    assert((int16_t)mr.render.camera.pos_y - (int16_t)rec->pos_y < 0x70);
    if ((int16_t)mr.render.camera.event_param == 0xE0 &&
        (mr.render.camera.timer & 1u) == 0u)
      seed_ok = 1;
    else {
      assert(fifa96_match_run_end(&mr) == 0);
      fifa96_match_run_init(&mr);
    }
  }
  assert(seed_ok == 1);
  /* the receive arm bound the camera event and the tracked record (the arc
   * runs from the live event; H=0xE0 samples 0x70 exactly at t=96). */
  assert((int16_t)mr.render.camera.event_param == 0xE0);
  assert(mr.render.camera.tracked == id);
  assert(fifa96_match_run_end(&mr) == 0);
  fifa96_match_run_init(&mr);

  /* ===== B: the live band-2 crossing + follow. ===== */
  for (int pump = 0; pump < 64 && cross_ok == 0; pump++) {
    uint16_t dump;
    begin_isolated(&mr, f.engine);
    for (int i = 0; i < pump; i++)
      assert(fifa96_rng_step(&mr.rng, &dump) == FIFA96_OK);
    for (int i = 0; i < 61; i++) one_granted_frame(&mr);
    {
      fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
      assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    for (int i = 0; i < 300 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
    assert(mr.state.phase == 2u);
    id = mr.slot.entity;
    assert(id == 9);
    rec = &mr.entities.team[0].records[9];
    {
      fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
      assert(fifa96_match_run_input(&mr, &up, 1) == 0);
    }
    for (int i = 0; i < 80 && mr.entities.ball.carrier != id; i++)
      one_granted_frame(&mr);
    if (mr.entities.ball.carrier != id) {
      assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
      assert(fifa96_match_run_end(&mr) == 0);
      fifa96_match_run_init(&mr);
      continue;
    }
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    one_granted_frame(&mr);
    /* the disclosed bootstrap stand-in (leg T1-L3): a clean even-grid event at
     * the cold camera so the arc's descending 0x70 sample is at t=96. */
    assert((int16_t)mr.render.camera.pos_y == 0);
    assert(fifa96_camera_event_set(&mr.render.camera, 0, 0, 0xE0, 0) == FIFA96_OK);
    for (int i = 0; i < 200 && mr.render.camera.timer < 92; i++)
      one_granted_frame(&mr);
    /* the V press/release arms mode 0x40 and its release installs row 07 on
     * the next frame (t=96, the exact 0x70 sample: the stage-0 gate admits and
     * the same-frame stage 1 reads the same ball height -> band 2). */
    {
      fifa96_platform_key lb = {FIFA96_ENGINE_KEY_LONG_BALL, 1};
      assert(fifa96_match_run_input(&mr, &lb, 1) == 0);
    }
    one_granted_frame(&mr);
    assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
    one_granted_frame(&mr);
    if (mr.entities.ball.pair.code == 4u && mr.entities.ball.pair.flags == 0x40u &&
        (int16_t)mr.entities.ball.pair.traj == 0x100 &&
        (int16_t)mr.render.camera.event_param == 0x100)
      cross_ok = 1;
    else {
      assert(fifa96_match_run_end(&mr) == 0);
      fifa96_match_run_init(&mr);
    }
  }
  assert(cross_ok == 1);
  assert(mr.render.camera.tracked == id);
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
  /* the every-frame follow arm picks the > 0xF0 event height up (the pan speed
   * is live from the next frame), then the tracked slot dirs drive the camera
   * velocity. */
  one_granted_frame(&mr);
  rec->dir_x = 1;
  rec->dir_z = 0;
  assert(fifa96_match_run_camera_follow(&mr) == 1);
  assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == 0);
  assert(mr.render.camera.speed != 0);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* M2 phase-12 T1 / FU-153 (15.a): the live record jump producer — the
 * FUN_0006E8E8 aux-keyed starter + the FUN_0007BF20 jump arc. The record is
 * put on the verified jump-aux row (0x5E frame 0, aux 0x0E; the live row
 * selector that reaches it is unported, FU-153-L1's remaining leg), and the
 * per-frame arm stages `ramp(word[+0x83]) = ramp(0x10) = 15` into the
 * +0x85/+0x87 pair and `pos_y = 0x10` (T1 fix round 1: the arm's ramp
 * argument is its own +0x83 marker, NOT the +0x81 countdown — the
 * `dword[+0x81] >> 16` alias; this test sets timer81 = 0 so the trap would
 * show as `base 3`, detected by the `jump_base == 15` assert), and the same
 * frame's arc advances +0x87 by the whole-frame delta 2 and samples the shared
 * 0x10F3C4 arc: t=17 -> 16 (the crest), then 15, 13, 12, 9, 6, 2 and the
 * `t > 2*15` landing (pos_y 0, pair 0, target := position). Every value is a
 * first-hand table sample (`0x10F3C4`). */
static void test_record_jump_arc_starter(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);
  /* T3: staged through the bounded FUN_0006E598 store core. */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 0) == FIFA96_OK);
  e->timer81 = 0;   /* the arm must NOT read it (fix round 1: was 0x10) */
  assert(e->pos_y == 0);
  one_granted_frame(&mr);
  assert(e->pos_y == 16);          /* the aux arm + the same frame's arc */
  assert(e->jump_base == 15 && e->jump_timer == 17);
  /* FU-161 §5.4 (T3): the 0x0E arm is one-shot — the aux byte is cleared on
   * the resolve, so the frame no longer re-stages and the arc descends on its
   * own (no `anim_id` mask needed; previously the unmodeled re-fire froze the
   * arc at the crest). */
  assert(e->aux_word == 0u);
  one_granted_frame(&mr); assert(e->pos_y == 15 && e->jump_timer == 19);
  one_granted_frame(&mr); assert(e->pos_y == 13 && e->jump_timer == 21);
  one_granted_frame(&mr); assert(e->pos_y == 12 && e->jump_timer == 23);
  one_granted_frame(&mr); assert(e->pos_y == 9 && e->jump_timer == 25);
  one_granted_frame(&mr); assert(e->pos_y == 6 && e->jump_timer == 27);
  one_granted_frame(&mr); assert(e->pos_y == 2 && e->jump_timer == 29);
  one_granted_frame(&mr);          /* t=31 > 2*15: the landing edge */
  assert(e->pos_y == 0 && e->jump_base == 0 && e->jump_timer == 0);
  assert(e->target_x == e->pos_x && e->target_z == e->pos_z);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* phase-13 T4 rider (from the phase-12 final review): the aux-`0x3A` jump
 * arm regression pin — the alias-strike class made regression-proof on the
 * second ported one-shot arm (the `0x0E` arm's `timer81 = 0` guard in
 * test_record_jump_arc_starter covers the first). The arm's ramp argument is
 * its own `+0x83` marker (fresh `disassemble_bytes` this task, `0x6ED08`
 * `MOV word[EBP+0x83],0x18`; `0x6ED11 MOV EAX,dword[EBP+0x81]; 0x6ED17 SAR
 * EAX,0x10`; `0x6ED1A CALL 0x702F8`; the stores `0x6ED1F/0x6ED26` and
 * `0x6ED33 MOV dword[EBP+0x5D],0x18`) — `dword[+0x81] >> 16` is the word at
 * `+0x83` (the just-stored 0x18), NOT the `+0x81` countdown; the fifth
 * `dword>>16` alias strike. The row `0x5C` frame-2 aux is `0x243A` (native
 * frame table `0x10EDBE`: `00 01 3A 24 02`, fresh `read_memory` this task),
 * so this row genuinely reaches the arm. `timer81 = 0` would make the alias
 * misread resolve `ramp(0) = 3`; the `jump_base == 18` assert is the RED
 * check without the constant (mutation-verified: reading `timer81` aborts
 * here). */
static void test_record_jump_arm_aux_3a_base_literal(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);
  /* the live path: the row-0x5C frame-2 install stages the native aux word */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5C, 2) == FIFA96_OK);
  assert(e->anim_id == 0x5Cu && e->frame == 2u);
  assert(fifa96_camera_ramp(0x18) == 18);   /* the literal base */
  e->timer81 = 0;   /* the arm must NOT read it (the alias trap) */
  one_granted_frame(&mr);
  /* The arm sets base/timer = ramp(0x18) = 18 and pos_y = 0x18; the same
   * frame's arc advances the timer by delta 2 and samples the shared
   * 0x10F3C4 table at t = 20 (1600 - 1577 = 23). */
  assert(e->jump_base == 18 && e->jump_timer == 20);
  assert(e->pos_y == 23);
  assert(e->aux_word == 0x2400u);   /* one-shot: only the low byte cleared */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-159 §4.1/§5.4 (T3): the FUN_0006E598 store core + the row-flags bit-0
 * producer (+0x44). First-hand row-table values: row 0x00 flags 0x07
 * (terminal -> 1), row 0x5E flags 0x00 (-> 0), row 0x26 flags 0x07 (-> 1).
 * The install clamps the id (`> 0x6E -> 0`), the frame (`> last -> 0`, the
 * FUN_0006E490 rule; row 0x5E last = 5) and forces the aux re-arm; the
 * per-frame dispatch derives row44 from the live anim_id even when the pool
 * byte is stale. */
static void test_record_anim_install_row44(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);

  assert(fifa96_match_run_record_anim_install(&mr, e, 0x00, 0) == FIFA96_OK);
  assert(e->anim_id == 0x00 && e->row44 == 1u);
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 0) == FIFA96_OK);
  assert(e->anim_id == 0x5E && e->row44 == 0u);
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x26, 0) == FIFA96_OK);
  assert(e->anim_id == 0x26 && e->row44 == 1u);
  /* frame clamp: row 0x5E last = 5 -> frame 9 installs at 0 */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 9) == FIFA96_OK);
  assert(e->frame == 0u);
  /* id clamp: 0x80 -> row 0 */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x80, 0) == FIFA96_OK);
  assert(e->anim_id == 0x00 && e->row44 == 1u);
  /* the dispatch derivation overrides a stale pool byte */
  e->anim_id = 0x5E;
  e->row44 = 1u;                    /* stale */
  one_granted_frame(&mr);
  assert(e->row44 == 0u);
  e->anim_id = 0x00;
  one_granted_frame(&mr);
  assert(e->row44 == 1u);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-159 §3.1/§5.2 (T3): the FUN_0008E008 frame-wrap successor over the pool.
 * First-hand rows: 0x5E {last 5, flags 0x00, next 0x0F}, 0x0F {last 4, flags
 * 0x00, next 0x00}, 0x00 {last 0, flags 0x07 (terminal), next 0x00}, 0x0E
 * {self-loop}, 0x14 {next 0x7F: mirror + 0x0F}, 0x26 {terminal self-loop}.
 * The install frame argument is last + 1 with the +0x46 = 1 install default
 * (0x6E6D8), clamped by the new row's last (0x6E490) — so the chain lands at
 * frame 0. */
static void test_record_row_successor_chain(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  e = &mr.entities.team[1].records[4];

  /* 0x5E (last frame) -> 0x0F frame 0, row44 0 */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 5) == FIFA96_OK);
  assert(fifa96_match_run_record_anim_successor(e) == FIFA96_OK);
  assert(e->anim_id == 0x0F && e->frame == 0u && e->row44 == 0u);
  /* 0x0F (last 4) -> 0x00 frame 0, row44 1 (terminal) */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x0F, 4) == FIFA96_OK);
  assert(fifa96_match_run_record_anim_successor(e) == FIFA96_OK);
  assert(e->anim_id == 0x00 && e->frame == 0u && e->row44 == 1u);
  /* terminal rows are this helper's no-op (the movement tables are
   * FU159-L5/FU161-L3) */
  assert(fifa96_match_run_record_anim_successor(e) == FIFA96_OK);
  assert(e->anim_id == 0x00 && e->row44 == 1u);
  /* self-loop 0x0E restarts at frame 0 */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x0E, 8) == FIFA96_OK);
  assert(fifa96_match_run_record_anim_successor(e) == FIFA96_OK);
  assert(e->anim_id == 0x0E && e->frame == 0u);
  /* mirror: row 0x14 next 0x7F -> face flips by 4 and 0x0F installs */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x14, 6) == FIFA96_OK);
  e->type = 3u;
  assert(fifa96_match_run_record_anim_successor(e) == FIFA96_OK);
  assert(e->anim_id == 0x0F && e->frame == 0u && e->type == 7u);
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 5) == FIFA96_OK);
  assert(fifa96_match_run_record_anim_install(NULL, e, 0x5E, 0) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_record_anim_install(&mr, NULL, 0x5E, 0) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_record_anim_successor(NULL) == -FIFA96_ERR_INVALID);
}

/* FU-159 §2.3 (T3): the case-0x0F arm — the `pos_y != 0` branch resolves the
 * frame to 3 through the FUN_0006E490 clamp (`0x6EE7B..0x6EE8B`); the
 * `pos_y == 0` branch runs the FUN_00079B1C snap and the relaunch
 * (`0x6EE90..0x6EEBE`). Row 0x14 f0 carries aux 0x0F (first-hand frame read
 * 0x10E4BC `40 00 0F 00 00`). */
static void test_record_jump_case_0f_snap_frame3(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);

  /* pos_y != 0: frame := 3 (row 0x14 last 6 admits 3) */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x14, 0) == FIFA96_OK);
  e->pos_y = 0x10;
  one_granted_frame(&mr);
  assert(e->frame == 3u);
  assert(e->pos_y == 0x10);          /* no landing writes on this branch */
  /* pos_y == 0: the snap + the relaunch base. Re-arm through the install
   * (the aux one-shot cleared the word). */
  e->pos_y = 0;
  e->lane = 0x1234;
  e->speed71 = 9;
  e->vel73 = 5;
  e->vel75 = -5;
  e->vel_x = 0x12345678;
  e->vel_z = 0x23456789;
  e->body_timer9c = 7;
  e->target_x = 0x111;
  e->target_z = 0x222;
  e->jump_base = 0;
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x14, 0) == FIFA96_OK);
  one_granted_frame(&mr);
  /* the arm's relaunch base survives the same frame's arc: base = ramp(0x1C),
   * timer advanced by delta (t=2), pos_y sampled from the shared arc */
  assert(e->jump_base == fifa96_camera_ramp(0x1C));
  assert(e->jump_timer == 2);
  assert(e->pos_y == fifa96_camera_arc_height((int16_t)fifa96_camera_ramp(0x1C), 2));
  assert(e->lane == 0 && e->speed71 == 0 && e->vel73 == 0 && e->vel75 == 0);
  assert(e->vel_x == 0 && e->vel_z == 0 && e->body_timer9c == 0);
  assert(e->target_x == e->pos_x && e->target_z == e->pos_z);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-160 §2 / FU-159 §3.3 (phase-13 T3): the live row-0x0E selector on the
 * natural kickoff chain. The tackle arm's `install = 0x0E` drain is replaced
 * by its exact engine equivalent (`fifa96_match_entities_install`), and the
 * dispatch runs the ported body live: stage 0/1 build the target + wait the
 * 0x0C timer, then install anim row 0x12 on the record; the row's f2 funnel
 * frame (aux 0x0E) fires the already-ported jump arm + BF20 arc, so the
 * record is airborne within the bounded window. This is the live selector the
 * T1 chain had to stand in for (`rec->anim_id = 0x5E` was the disclosed
 * stand-in). */
static void test_live_row_0e_selector_installs_jump(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int installed = 0;
  int airborne = 0;
  int saw_0f = 0;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  /* stage the match art pair so the scene stage advances the anim frames
   * (the bridge does this in the engine path; the direct-run fixture must
   * call it explicitly) */
  assert(fifa96_match_run_stage(&mr, f.engine->surface,
                                "/ART/PLAYART.PVI", "/ART/GAMEART0.PVI") == FIFA96_OK);
  assert(mr.render.enabled == 1);
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

  rec = &mr.entities.team[1].records[4];
  assert(rec->active != 0u);
  assert(fifa96_match_entities_install(rec, (uint8_t)mr.state.phase, 0x0E, 0) == 1);
  assert(rec->code == 0x0Eu);
  for (int i = 0; i < 80; i++) {
    one_granted_frame(&mr);
    if (rec->anim_id == 0x12u && rec->frame == 0u) {
      installed = 1;
      break;
    }
  }
  assert(installed == 1);          /* stage 1 installed row 0x12 (frame 0) */
  assert(rec->row44 == 0u);        /* row 0x12 flags 0x00 */
  assert(rec->timer93 == 0x10u);   /* the 0x79B58 self receiver timer */
  /* FU-167/FU-168 (phase-14 T1): the stage-2 else arm on the live chain. The
   * record is not yet camera-tracked and has no jump arc (the f2 aux-0x0E
   * arming is one granted frame away), so the arm fires when the camera Y is
   * mid-band. The live Y producer is R19/FU-171's leg (FU168-L5) and the lane
   * is refreshed by the BF20 block after this dispatch, so the test stages
   * both (0x40 in the strict (0x20,0x70) window; lane_x 0x10 < 0x60), then
   * asserts the tracked bind + the same-frame stage-3 advance. */
  {
    int32_t rec_id = (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 4u);
    assert(rec->stage92 == 2u);
    mr.render.camera.pos_y = 0x40;
    rec->lane_x = 0x10;
    one_granted_frame(&mr);
    assert(mr.render.camera.tracked == rec_id);   /* [0x1577CA] := rec */
    assert(rec->stage92 == 3u);
    assert(mr.render.camera.event_param != 0);    /* the 0x71C94 event height */
  }
  mr.render.camera.pos_y = 0;
  for (int i = 0; i < 40 && rec->pos_y == 0; i++) one_granted_frame(&mr);
  airborne = rec->pos_y != 0;
  assert(airborne == 1);           /* the f2 aux-0x0E jump arm + arc */
  /* the live repetition enabler: the row chain 0x12 -> 0x0F -> 0x00 runs
   * through the scene-stage successor; the terminal row sets +0x44 = 1 and
   * the row-0x0E stage-3 arm resets the action record (the free-for-a-new-
   * action edge the tackle chain needs). */
  for (int i = 0; i < 140 && rec->anim_id != 0u; i++) {
    one_granted_frame(&mr);
    if (rec->anim_id == 0x0Fu) saw_0f = 1;
  }
  assert(saw_0f == 1);             /* the successor moved 0x12 -> 0x0F live */
  assert(rec->anim_id == 0u);      /* ... and 0x0F -> 0x00 */
  assert(rec->row44 == 1u);        /* row 0 flags 0x07 (terminal) */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-161 §3.1/§5.2 (T3): the facing-relative impulse — aux 0x0E
 * (`0x6EE1B..0x6EE5B`) and the sustained 0x2A/0x2B family. `dir_x`/`dir_z`
 * are the first-hand 0x10F334/0x10F33C octant bytes; the metric is the ported
 * `fifa96_entity_distance` (4 axis / 5 diagonal). The 0x2B gate freezes the
 * frame (`frame_freeze`) only while `jump_timer < 2*jump_base`. */
static void test_record_jump_impulsion(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);

  /* axis octant 2: dir (1,0) << 2 = (4,0), metric 4 */
  e->type = 2;
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 0) == FIFA96_OK);
  one_granted_frame(&mr);
  assert(e->vel73 == 4 && e->vel75 == 0 && e->speed71 == 4);
  /* the dword views stay in lockstep: +0x71 = {speed, vel73}, +0x73 =
   * {vel73, vel75} */
  assert((int16_t)e->vel_x == 4);
  assert((int16_t)((uint32_t)e->vel_x >> 16) == 4);
  assert((int16_t)((uint32_t)e->vel_z >> 16) == 0);

  /* diagonal octant 1: dir (1,1) << 2 = (4,4), metric 5 (reset the stride
   * timer so the mover's retarget does not pull the fresh impulse) */
  e->type = 1;
  e->body_timer9c = 0;
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 0) == FIFA96_OK);
  one_granted_frame(&mr);
  assert(e->vel73 == 4 && e->vel75 == 4 && e->speed71 == 5);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-161 §5.2 (T3): the sustained 0x2A impulse re-applies every frame the
 * frame holds (no one-shot clear), and the 0x2B arm adds the frame freeze
 * while airborne. Row 0x5E frame 1 aux 0x002A, frame 2 aux 0x032B
 * (first-hand frame reads 0x10E3E6). */
static void test_record_jump_sustained_and_freeze(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);
  e->type = 2;                       /* dir (1,0) */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 1) == FIFA96_OK);
  one_granted_frame(&mr);
  assert(e->vel73 == 4 && e->vel75 == 0 && e->speed71 == 4);
  assert(e->aux_word == 0x2Au);      /* sustained: not cleared */
  e->body_timer9c = 0;               /* keep the stride retarget out */
  one_granted_frame(&mr);
  assert(e->vel73 == 4 && e->speed71 == 4);   /* re-fired */

  /* f2 aux 0x032B: airborne (10 < 30) -> impulse + the frame freeze */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 2) == FIFA96_OK);
  e->body_timer9c = 0;
  e->jump_base = 15;
  e->jump_timer = 10;
  one_granted_frame(&mr);
  assert(e->vel73 == 4 && e->vel75 == 0 && e->speed71 == 4);
  assert(e->frame_freeze == 1u);
  /* landed (timer >= 2*base): the arm's gate refuses, so no freeze is staged
   * (the word is sustained and re-fires, but the gate holds) */
  e->jump_base = 15;
  e->jump_timer = 30;
  e->body_timer9c = 0;
  one_granted_frame(&mr);
  assert(e->frame_freeze == 0u);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-161 §5.3 (T3): the grounded 0x0C target step (`0x6EBFB..0x6EC48`) — with
 * `pos_y == 0` the target is the position plus `dir[oct] << 5`; the
 * `0x10F331`/`0x10F339` dword bases resolve to the same 0x10F334/0x10F33C
 * bytes through the `>>24` alias (fresh this task). */
static void test_record_jump_grounded_step(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t start_x;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);
  e->type = 2;                       /* dir (1,0) -> +0x20 x */
  e->pos_y = 0;
  start_x = e->pos_x;
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x5E, 3) == FIFA96_OK);
  one_granted_frame(&mr);
  assert(e->target_x == start_x + 0x20);
  assert(e->target_z == e->pos_z);   /* the staged pos at the arm */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-161 §3.1/§6 (phase-13 T3): the live jump carry-and-stop on the natural
 * kickoff chain. The 0x5E row's aux 0x0E impulsion carries the record along
 * the facing (`vel = dir[oct] << 2`, the mover integrates `pos += delta*vel`)
 * and the f5 aux-0x05 snap stops it (`FUN_00079B1C`: speed/vel zero,
 * target := pos). The row is staged through `record_anim_install` (the
 * FUN_0006E598 store core) with the art pair staged so the frame advance
 * reaches f5. */
static void test_live_jump_impulsion_carry_and_stop(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  struct fifa96_match_entity *rec;
  int32_t start_x;
  int carried = 0;
  int saw_f5 = 0;
  int snap_ok = 0;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  assert(fifa96_match_run_stage(&mr, f.engine->surface,
                                "/ART/PLAYART.PVI", "/ART/GAMEART0.PVI") == FIFA96_OK);
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

  rec = &mr.entities.team[1].records[4];
  rec->type = 2;                     /* +x facing (octant 2: dir (1,0)) */
  start_x = rec->pos_x;
  assert(fifa96_match_run_record_anim_install(&mr, rec, 0x5E, 0) == FIFA96_OK);
  one_granted_frame(&mr);
  assert(rec->speed71 == 4 && rec->vel73 == 4 && rec->vel75 == 0);
  for (int i = 0; i < 90; i++) {
    one_granted_frame(&mr);
    if (rec->pos_x != start_x) carried = 1;
    if (rec->anim_id == 0x5E && rec->frame == 5) saw_f5 = 1;
    if (saw_f5 && rec->speed71 == 0 && rec->vel73 == 0 && rec->vel75 == 0 &&
        rec->target_x == rec->pos_x && rec->target_z == rec->pos_z) {
      snap_ok = 1;   /* the f5 aux-0x05 snap ran (FUN_00079B1C) */
      break;
    }
  }
  assert(carried == 1);              /* the impulsion carried the record */
  assert(snap_ok == 1);              /* the f5 snap stopped it */
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
  begin_isolated(&mr, f.engine);
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
  begin_isolated(&mr, f.engine);
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

/* FU286 / R11-L1 (phase-26c T3): the claim anim-selection bridge. The row-0x1E
 * keeper claim selects its stage-4 anim from the residual vec_band tiers
 * (native `0x75CF7 CALL 0x6E598` writes `rec+0x28`); the `>= 0x780` tier
 * selects row 0x45, so the same dispatch's stage 5 cuts the release
 * (`0x75D24 CMP byte[[rec+0x28]],0x45` -> `0x157750 := 0x50`). The bridge
 * consumes the produced `out->event` into `record.anim_id` (hence the pool
 * record) and the claim's live row becomes 0x45. Baseline (no bridge): the
 * live row never becomes 0x45, so stage 5 advances without the release and the
 * release triple `0x157750` is never written. `mr.keeper_cam_y` is the engine's
 * `0x157750` stand-in, written only by the stage-5 release block. */
static void test_row1e_claim_anim_bridge(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[0];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0x1E, 0) == 1);
  e->stage92 = 4;                   /* the stage-4 outlet dispatch */
  e->frame = 3;                     /* past the stage-5 `frame < 3` gate */
  e->has_slot = 1;                  /* slot path -> 0x8DCD4(pos, reset) vector */
  e->pos_x = 0;
  e->pos_z = 0;
  mr.keeper_latch_157ab2 = 0;       /* latch clear -> the reset-vector arm */
  mr.keeper_reset_x = 0x800;        /* band 0x800 >= 0x780 -> event 0x45 */
  mr.keeper_reset_y = 0;
  mr.keeper_reset_z = 0;
  assert(mr.keeper_cam_y == 0);     /* the release triple is unset at baseline */
  for (int i = 0; i < 200 && mr.keeper_cam_y != 0x50; i++) one_granted_frame(&mr);
  assert(mr.keeper_cam_y == 0x50);  /* the stage-5 release arm fired */
  assert(e->anim_id == 0x45u);      /* the bridge selected row 0x45 */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU286 / R11-L2 + R11-L3 (phase-26c T4): the row-0x1E claim head place's
 * pause flag and the per-sector offset tables. The head place (stage < 3, no
 * ball) reads `0x10F334/0x10F33C[byte[rec+0x8e]]` (the record's octant) and
 * calls `FUN_000700F4` with `PUSH 1` (raw `0x7559E`); the native reset writes
 * `[0x157820] = [0x157822] = flag` (fresh `0x70271..0x7028a`). The engine's
 * `record.place_offset_x/z` are written from the table pair by the record's
 * sector (R11-L3) and the pause lands on `flag_157820`/`flag_157822` (R11-L2).
 * Mutation-RED: without R11-L3 the offsets stay 0 (the place uses the record
 * pos); without R11-L2 the flag cells stay 0. */
static void test_row1e_claim_place_pause_and_offsets(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.record.entity_id = 0;
  mr.record.stage92 = 1;              /* < 3: the head claim place runs */
  mr.record.has_ball = 0;
  mr.record.has_slot = 0;
  mr.record.pos_x = 100;
  mr.record.pos_y = 20;
  mr.record.pos_z = 200;
  mr.record.type = 3;                 /* octant 3: table entry (1, -1) */
  assert(mr.record.place_offset_x == 0 && mr.record.place_offset_z == 0);
  assert(mr.flag_157820 == 0 && mr.flag_157822 == 0);

  assert(fifa96_match_dispatch_action(&mr, 0x1E) == FIFA96_OK);

  /* R11-L3: the per-sector table writes the record offsets. */
  assert(mr.record.place_offset_x == 1);
  assert(mr.record.place_offset_z == -1);
  /* the head place applies them (<<4) at the record position */
  assert(mr.record.place_valid == 1);
  assert(mr.record.place_x == 100 + 16);   /* dir_x 1 << 4 */
  assert(mr.record.place_z == 200 - 16);   /* dir_z -1 << 4 */

  /* R11-L2: the `PUSH 1` place pause sets both [0x157820]/[0x157822]. */
  assert(mr.flag_157820 == 1);
  assert(mr.flag_157822 == 1);
}

/* FU273 §4.4 (phase-26c T1): the R11-L4 keeper release reach — the phase RED
 * gate.
 *
 * A granted-frame scenario drives the keeper's row-`0x1B` stage-4 tail on the
 * controlled side (phase `0xD`, CPU `global_157a4c`), so the sit-5 emit runs
 * (`fifa96_keeper_reposition_b_finish` -> notify 5), situation 5 selects phase
 * 9, and the phase-9 arm installs row `0x1E` on the team's keeper. The claim
 * machine then walks stages 0..4; the stage-4 outlet resolves event `0x45` and
 * (via the R11-L1 `FUN_0006E598` bridge) stage 5 cuts the release
 * `camY := 0x50` and binds the keeper as the camera-tracked record.
 *
 * This is the phase's Layer-1 landing (Task 9 turns the T1 RED gate green).
 *
 * Three disclosed staging points (the T3 review carries):
 *  (i)  same-dispatch frame-zero divergence — the stage-4 anim install runs the
 *       native `FUN_0006E490` clamp with frame 0, so `rec+0x3D` is zeroed and
 *       the SAME-dispatch stage-5 `frame < 3` gate exits exactly as the native.
 *       The release therefore needs the frame to advance; the test asserts the
 *       frame reads 0 at the outlet and then stages it (below).
 *  (iv) the frame advance is a STAGED input — the engine's per-tick frame
 *       machine reads `render.frames`, absent in this isolated fixture, so the
 *       fixture stages `keeper->frame = 3` to pass the native `frame < 3` gate.
 *
 * The release writes the persistent cell `[0x157750]` (`keeper_cam_y`) and the
 * FU-71 camera triple. The release frame stages no `place`, so the frame body's
 * placement drain (`match_run.c` `match_run_entity_drain` -> `fifa96_camera_init`
 * at :1566) does not re-run this frame and `render.camera.pos_y` still holds
 * `0x50` at the frame end (asserted); the persistent `keeper_cam_y == 0x50` is
 * asserted alongside as the load-bearing cell. */
static void test_keeper_l4_release_reach(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *keeper;
  int reached_outlet = 0;
  int released = 0;

  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  assert(mr.render.camera.pos_y == 0);            /* baseline: event-starved camY */

  /* The keeper (team 1 record 0, entity id 11 so `tracked != 0`) receives row
   * `0x1B` at its stage-4 tail on the controlled side. */
  mr.state.phase = 0x0Du;
  mr.state.period_length = 90;
  mr.entities.team[1].side = 1;
  mr.phase_machine.side_controlled = 1;
  mr.global_157a4c = 0;                           /* CPU: the stage-4 tail notifies 5 */
  keeper = &mr.entities.team[1].records[0];
  assert(fifa96_match_entities_install(keeper, 0x0Du, 0x1B, 0) == 1);
  keeper->stage92 = 4;

  one_granted_frame(&mr);                          /* the sit-5 emit */
  assert(mr.state.phase == 9u);                   /* situation 5 -> phase 9 */
  assert(keeper->code == 0x1Eu);                  /* row 0x1E installed on the keeper */

  /* The claim walks stages 0..4; the stage-4 outlet resolves anim row `0x45`
   * through the R11-L1 bridge. At that same dispatch stage 5 runs the
   * `frame < 3` gate against the just-zeroed frame and exits (carry (i)). */
  for (int i = 0; i < 150 && !reached_outlet; i++) {
    one_granted_frame(&mr);
    reached_outlet = (keeper->stage92 == 5u && keeper->anim_id == 0x45u);
  }
  assert(reached_outlet == 1);                    /* RED at baseline: no bridge */
  assert(keeper->frame == 0u);                    /* the FUN_0006E490 frame-zero */

  /* Carry (iv): stage the frame past the native `frame < 3` gate. */
  keeper->frame = 3u;
  one_granted_frame(&mr);                          /* stage 5 -> 6 -> 7 -> 8 */
  released = (mr.keeper_cam_y == 0x50);
  assert(released == 1);                          /* the stage-5 release fired */

  /* The release triple: the persistent `[0x157750]` and the FU-71 camera. */
  assert(mr.keeper_cam_y == 0x50);
  assert(mr.render.camera.pos_y == 0x50);
  /* Carry (iii): the bridge writes the pool entity (the anim install) and
   * `record.anim_id`; the persistent pool entity carries `0x45`. */
  assert(keeper->anim_id == 0x45u);               /* the claim's selected row */
  assert(mr.render.camera.tracked != 0);          /* the keeper bound as tracked */

  /* The row-0E stage-2 gate (`0x82828..0x82834`, the live camY band
   * `0x20 < [0x157750] < 0x70`): the released camera Y admits the pan arm on
   * the next granted frame (the band the row-0x0E follow consumer reads). */
  one_granted_frame(&mr);
  assert(mr.render.camera.pos_y > 0x20 && mr.render.camera.pos_y < 0x70);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU296 §4 (phase-27 T4): the staged Layer-1 installer reach.
 *
 * A granted-frame scenario drives the ported record-0 machine's CPU ladder
 * (`fifa96_keeper_cpu.c`, the FU296 Seam A) on a live team-0 keeper record, so
 * the ladder's stage-4 arm installs row `0x1B` invoke-now (`0x78294`/`0x78299`,
 * FU283 §4), the entity's single dispatch runs the wired row-`0x1B` body, the
 * sit-5 emit selects phase 9, row `0x1E` dispatches, the claim reaches the
 * stage-5 release, and the camera is cut to `camera.pos_y == 0x50`.
 *
 * MULTI-FRAME (the T1 Ruling). The ladder's install gate is `phase == 2` (P2,
 * FU288 §2.1) while the row-`0x1B` sit-5 emit's HEAD requires `phase == 0xD` —
 * disjoint phase immediates, so they cannot co-hold in one frame (FU288). The
 * drive therefore raises the ladder install on a `phase == 2` frame; on that
 * SAME frame the row-`0x1B` dispatch runs the wide/self-actor re-place path
 * (FU288 §2.3: `[0x157C5C] != 0` -> `0x774DD`, `0x77568 [0x157A83] = rec`),
 * whose emit (`0x77707`) carries NO phase gate, so it reaches the sit-5 emit at
 * phase 2 -> situation 5 -> phase 9 -> row `0x1E`. Subsequent frames walk the
 * row-`0x1E` claim stages 0..4 to the stage-5 outlet (`anim_row == 0x45`), and
 * the stage-5 release cuts `camera.pos_y` to `0x50`.
 *
 * The phase-0xD HEAD path is NOT engine-reachable from this install: the ported
 * row-`0x1B` body reverts to row `0x19` at stage 4 whenever the actor is unset
 * (`fifa96_keeper_reposition_b_finish` not-controlled), so a row installed at
 * `phase == 2` does not persist to a later `phase == 0xD` frame (FU288-L2 leaves
 * the native survival open). The wide/self-actor path is the faithful reach
 * (FU288 §2.3) and is what the brief's Step 1 names.
 *
 * Staged producer INPUTS (the plan's rule: staging a producer's input is
 * legitimate, staging a gate is a fake):
 *  - **P1 `permit` is NOT staged (FU298, phase-28 T1): it is live** — the seam
 *    computes `fifa96_keeper_cpu_permit(e->code)` from the static `0x110680`
 *    table; the keeper's staged code `0x19` reads byte `3` -> bit 0 set, so
 *    this reach's gate opens from the record's own state.
 *  - P13 — the stage index `CX`. **FU290 (phase-27 T5): the index is now
 *    COMPUTED** by `fifa96_keeper_cpu_stage_select` from the record inputs; the
 *    T3 `rec.stage = e->stage` bridge is gone. This fixture stages the machine's
 *    INPUTS so it yields CX=4: the camera z behind the `AX` prepass gate
 *    (`mr.render.camera.pos_z = -0x800`), `keeper->lane` (`0x28 << 16`), and an
 *    opposite-side actor (`mr.entities.controlled` on team 1). The unported
 *    latch cells stay zero (legs); they gate the CX=4 arm only via the
 *    opposite-side actor, which is live.
 *  - P4 `mr.global_157c5d = 1` — the row-`0x19` interlock `[0x157C5D]`.
 *  - The FU292 latch `[0x157C5C]`: **NOT staged** — the machine's own CX=4 arm
 *    sets it (`0x780D7`) and the row dispatch publishes it in-frame (FU290 T5),
 *    so the reach exercises the machine's latch write-back.
 *  - `keeper->stage92 = 2` — the row-`0x1B` stage latch (the phase-26c carry
 *    (iv) precedent; the isolated fixture has no live row-stage producer).
 *  - `team[0].slot_pool = 0` — the FU-70 slot machine's source pool
 *    (`0x78789`); the isolated fixture has no live slot source.
 * Every other gate is real engine state: `phase`, `side_controlled`, the
 * row-`0x19` hold (`code`), the skip/timer gates (P7/P9), the actor skip
 * (P10), no queued action (P3) and the single record dispatch.
 *
 * Baseline (phase-26c): the module is unlinked/unwired, so the ladder never
 * runs — `records[0].code` stays `0x19` and `camera.pos_y` stays `0`. */
static void test_keeper_installer_reach(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *keeper;
  int reached_outlet = 0;
  int released = 0;

  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  assert(mr.render.camera.pos_y == 0);            /* baseline: event-starved camY */

  /* The keeper the ladder installs onto: team 0 record 0, holding row `0x19`
   * with the skip/timer gates clear and no control slot. */
  mr.state.phase = 2;                              /* P2: the ladder gate phase */
  mr.state.period_length = 90;
  mr.entities.team[0].side = 0;
  mr.entities.controlled =
      (int32_t)FIFA96_MATCH_ENTITY_RECORDS;        /* the opposite-side actor (team 1 rec 0) */
  mr.entities.team[0].slot_pool = 0;               /* no live slot source (FU-70) */
  mr.phase_machine.side_controlled = 0;
  mr.global_157a4c = 0;                            /* CPU: the stage-4 tail emits sit-5 */
  mr.global_157c5d = 1;                            /* P4: the row-0x19 interlock */
  mr.render.camera.pos_z = -0x800;                 /* FU290 AX gate: |cam.z| > 0x7B0 */
  keeper = &mr.entities.team[0].records[0];
  keeper->code = 0x19u;                            /* P5: the keeper-hold row */
  keeper->skip_9a = 0u;                            /* P7 */
  keeper->timer81 = 0u;                            /* P9 */
  keeper->has_slot = 0u;                           /* P3 (no queued action) */
  keeper->lane = 0x28 << 16;                       /* FU290: lane_x=0x28 -> CX=4 */
  keeper->stage92 = 2u;                            /* the row-0x1B stage-2 (wide/re-place) */

  /* Frame A (phase 2): the ladder installs row 0x1B (invoke-now); the row's
   * self-actor re-place path reaches the sit-5 emit -> phase 9 -> row 0x1E. */
  one_granted_frame(&mr);
  assert(keeper->code == 0x1Eu);                   /* row 0x1E installed on the keeper */
  assert(mr.state.phase == 9u);                    /* situation 5 -> phase 9 */

  /* Frames B: the row-0x1E claim walks stages 0..4 to the stage-5 outlet (the
   * stage-4 anim selection resolves row 0x45). The live permit is 0 here (the
   * installed code 0x1E reads table byte 0), so the phase-2 install tree does
   * not clobber the claim row — the same suppression the phase-27 fixture
   * staged. */
  for (int i = 0; i < 400 && !reached_outlet; i++) {
    one_granted_frame(&mr);
    reached_outlet = (keeper->stage92 == 5u && keeper->anim_id == 0x45u);
  }
  assert(reached_outlet == 1);                     /* the stage-5 outlet (anim 0x45) */

  /* Frame C: stage the frame past the native stage-5 `frame < 3` gate (the
   * phase-26c carry (iv)) -> the release fires. */
  keeper->frame = 3u;
  one_granted_frame(&mr);
  released = (mr.keeper_cam_y == 0x50);
  assert(released == 1);                           /* the stage-5 release fired */

  /* The release triple: the persistent `[0x157750]` and the FU-71 camera. */
  assert(mr.keeper_cam_y == 0x50);
  assert(mr.render.camera.pos_y == 0x50);
  assert(keeper->anim_id == 0x45u);                /* the claim's selected row */
  assert(mr.render.camera.tracked != 0);           /* the keeper bound as tracked */

  /* The row-0E stage-2 gate (the live camY band `0x20 < [0x157750] < 0x70`). */
  one_granted_frame(&mr);
  assert(mr.render.camera.pos_y > 0x20 && mr.render.camera.pos_y < 0x70);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU298 §4/§6 (phase-28 T1): the P1 flip — the phase-27 T7 waiver, re-aimed.
 *
 * The phase-27 waiver pinned the honest DEAD gate: `permit` was the unported
 * `[0x110680][code] & 1` (FU282-L5) left 0 by `fifa96_keeper_cpu_init`, so no
 * live frame raised it, the ladder never ran, no row-`0x1B`/`0x1E` installed,
 * phase 9 was never reached, and this chain never raised `camera.pos_y`.
 * FU298-L6 named that test RED-by-design for the Stage-2 flip.
 *
 * FU298 ports P1 as what it is in the image — a static 45-byte table with no
 * writer (19 read-only sites) — bound live at the seam:
 * `kc_rec.permit = fifa96_keeper_cpu_permit(e->code)`. The T4 reach's keeper
 * (code `0x19` -> table byte `3` -> bit 0) now opens the gate from the record's
 * own state with NO staged permit, so this test drives the exact T4 scenario
 * minus the staged P1 through the REAL per-frame body (the same
 * `ef.keeper_cpu = &mr->keeper_cpu` seam `make game` uses) and pins the flip:
 * the ladder's `CX=4` arm installs row `0x1B`, its same-frame dispatch reaches
 * the sit-5 emit (phase 9 -> row `0x1E`), and the row-`0x1E` claim walks to the
 * stage-5 outlet — the T4 frame-A/B trajectory, un-staged. On the phase-27 tree
 * the first assert fails (the dead permit leaves the code at `0x19`); the flip
 * is the Stage-2 acceptance signal (FU298-L6), not a regression.
 *
 * The release / visible-camera assertions stay with the T4 reach (they need the
 * stage-5 `frame < 3` staging) and the Layer-2 acceptance (spec §5). */
static void test_keeper_installer_live_permit(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *keeper;
  int reached_outlet = 0;

  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);

  /* The exact T4 reach scenario — minus the staged `permit`. */
  mr.state.phase = 2;
  mr.state.period_length = 90;
  mr.entities.team[0].side = 0;
  mr.entities.controlled = (int32_t)FIFA96_MATCH_ENTITY_RECORDS; /* opposite-side actor */
  mr.entities.team[0].slot_pool = 0;
  mr.phase_machine.side_controlled = 0;
  mr.global_157a4c = 0;
  mr.global_157c5d = 1;                           /* the row-0x19 interlock */
  mr.render.camera.pos_z = -0x800;                /* the FU290 AX-gate input */
  keeper = &mr.entities.team[0].records[0];
  keeper->code = 0x19u;
  keeper->skip_9a = 0u;
  keeper->timer81 = 0u;
  keeper->has_slot = 0u;
  keeper->lane = 0x28 << 16;                      /* the CX=4 lane input */
  keeper->stage92 = 2u;

  /* Frame 1: the live permit opens the gate; the ladder gate (P4 staged) runs
   * the CX=4 arm; row `0x1B` installs invoke-now and dispatches on this same
   * frame (the T4 frame-A trajectory, un-staged). */
  one_granted_frame(&mr);
  assert(keeper->code == 0x1Eu);                  /* installed + dispatched live */
  assert(mr.state.phase == 9u);                   /* situation 5 -> phase 9 */

  /* The claim walk: the installed row-`0x1E` walks stages 0..4 to the stage-5
   * outlet (`anim_id == 0x45`) — the machinery the phase-27 waiver pinned dead. */
  for (int i = 0; i < 400 && !reached_outlet; i++) {
    one_granted_frame(&mr);
    reached_outlet = (keeper->stage92 == 5u && keeper->anim_id == 0x45u);
  }
  assert(reached_outlet == 1);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU304 §3 / FU300 §3.2-G1 (phase-28 T3): the merged Seam A install contract
 * + the `copy_pos` output — the seam-level pins.
 *
 * G1: the native installer `FUN_0007D9A4` stages a whole record (`0x7DA63`..
 * `0x7DA9B`: `+0x91 := code`, `+0x89 := 0`, `+0x9E := 0`, `+0x92 := BL` and
 * `+0x7B := +0x79`) and both Seam A installs — the deferred tree `0x78576`
 * and every ladder arm (fresh FU304 §1: `MOV ECX,1` + `XOR EBX,EBX` before
 * each `CALL 0x7D9A4`, so `BL = 0`) — call it. The phase-27 seam wrote
 * `e->code` only, so a same-frame row-`0x1A`/`0x1B` dispatch could run on a
 * stale `stage92`/`timer89` (FU300 §3.2's mis-route class); FU304's review
 * erratum adopts the merged contract — both installs route through
 * `fifa96_match_entities_install(e, phase, code, 0)`. The `+0x7B := +0x79`
 * write is the helper's own contract (pinned by `test_install_stages` in
 * test_engine_match_entities.c), not a seam-level observable: the seam
 * pre-writes `e->timer7b = e->timer79` before the step.
 *
 * The native install-before-ladder order (fresh): the install tree's
 * `0x78576` call runs before the ladder gate's `0x78599`, and
 * `FUN_0007D9A4` writes `+0x91` at `0x7DA67`; the ladder's head gate reads
 * that live byte (`0x77EB7..0x77EC5`; `get_xrefs_to 0x77EAC` = 1 caller,
 * `0x78599`) and returns unless it is `0x19`. The machine therefore derives
 * the ladder from the POST-install code (fifa96_keeper_cpu.c), and a live
 * `timer81` on a code-`0x19` keeper always installs code 0 first — the
 * `+0x81` copy arm (`0x77F16..0x77F29`) cannot be reached through the only
 * caller. FU304 §2's "installs code 0 and takes the copy arm" composite is
 * refuted by those bytes (the report's fix note carries the analysis); the
 * seam still applies `copy_pos` per FU304 §3(A) (`target_* = pos_*`), and
 * the install-fired pin below enforces that it does NOT fire there.
 *
 * The pins below drive the walk's `i == 0` seam
 * (`fifa96_match_entities_team_update`) and capture the record at dispatch
 * entry — after the seam applies and before the row body, exactly the FU304 §3
 * frozen shape ("both target writers sit before `action(ctx, e)`"). The
 * machine's own inputs are producer inputs (FU290's camera/lane triple, FU299's
 * `[0x157C5D]` gate cell); the install and the copy are what is under test. */
struct seam_capture {
  int calls;
  uint8_t code;
  uint8_t stage92;
  uint8_t ran;
  int32_t timer89;
  int32_t target_x, target_y, target_z;
};

static int seam_first_capture(void *ctx, struct fifa96_match_entity *e) {
  struct seam_capture *c = ctx;
  if (c->calls == 0) {
    c->code = e->code;
    c->stage92 = e->stage92;
    c->ran = e->ran;
    c->timer89 = e->timer89;
    c->target_x = e->target_x;
    c->target_y = e->target_y;
    c->target_z = e->target_z;
  }
  c->calls++;
  return FIFA96_OK;
}

/* The machine exposes its ladder as a seam (`cpu.ladder`); this capture reads
 * the record view the machine hands it — the live record the native `0x78599`
 * call reads (post-install code when the tree fired). */
struct ladder_capture {
  int calls;
  uint8_t code;
};

static int ladder_first_capture(void *ctx,
                                const struct fifa96_keeper_cpu_record *rec,
                                struct fifa96_keeper_cpu_ladder_out *out) {
  struct ladder_capture *c = ctx;
  c->calls++;
  c->code = rec->code;
  (void)out;   /* the step zeroed it; the seam sees no ladder output */
  return FIFA96_OK;
}

/* The deferred install tree (`0x78576`) through the seam, live inputs only:
 * keep code 4 (permit `0x110680[4] & 1 = 1`, FU298) with phase 2 and a zero
 * timer, so the tree's target is the `0x19` escalation — the not-owner arm
 * (`0x7851E` -> the `0x78553` `MOV EAX,0x19`; the seam leaves the unported
 * possession producer `ball_owner` zero) — a different code, installing. The
 * record carries a stale pre-install posture the native installer overwrites
 * (`0x7DA7E/0x7DA88/0x7DA95`); the single `0x785A8` dispatch must see the
 * installer's values. */
static void test_keeper_seam_install_full_state(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f;
  fifa96_keeper_cpu cpu;
  struct fifa96_match_entity *keeper;
  struct seam_capture cap;
  memset(&f, 0, sizeof f);
  memset(&cap, 0, sizeof cap);
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  fifa96_keeper_cpu_init(&cpu);
  f.phase = 2u;
  f.delta = 2u;
  f.keeper_cpu = &cpu;
  keeper = &pool.team[0].records[0];
  keeper->code = 4u;              /* permit 1; the tree target 0x19 != 4 */
  keeper->skip_9a = 0u;
  keeper->timer81 = 0u;           /* the tree keeps the 0x19 escalation */
  keeper->has_slot = 0u;
  keeper->stage92 = 2u;           /* stale pre-install postures */
  keeper->timer89 = 0x55;
  keeper->ran = 1u;
  assert(fifa96_match_entities_team_update(&pool, 0, &f, seam_first_capture,
                                           &cap) == FIFA96_OK);
  assert(cap.calls == (int)FIFA96_MATCH_ENTITY_RECORDS);
  assert(cap.code == 0x19u);      /* 0x78576 install applied */
  assert(cap.stage92 == 0u);      /* +0x92 := BL = 0 (the merged contract) */
  assert(cap.timer89 == 0);       /* +0x89 := 0 */
  assert(cap.ran == 0u);          /* +0x9E := 0 */
}

/* The install-fired state must NOT apply the `+0x81` `pos -> target` copy: the
 * deferred install writes `+0x91 = 0` (`0x7DA67`) before the `0x78599` ladder
 * call, whose head gate (`0x77EB7..0x77EC5`) returns unless `+0x91 == 0x19`.
 * A live `timer81` on the code-`0x19` keeper therefore always installs code 0
 * and the ladder head-gates out; the dispatch must see the install and the
 * target triple untouched (FU304 §2's composite is refuted — see the report).
 * RED on the pre-fix tree: the seam applied the machine's pre-install-code
 * `copy_pos` and overwrote the target with `pos`. */
static void test_keeper_seam_copy_pos_not_after_install(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f;
  fifa96_keeper_cpu cpu;
  struct fifa96_match_entity *keeper;
  struct seam_capture cap;
  memset(&f, 0, sizeof f);
  memset(&cap, 0, sizeof cap);
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  fifa96_keeper_cpu_init(&cpu);
  f.phase = 2u;
  f.delta = 2u;
  f.keeper_cpu = &cpu;
  cpu.interlock_157c5d = 1u;      /* the ladder-gate input cell */
  keeper = &pool.team[0].records[0];
  keeper->code = 0x19u;
  keeper->skip_9a = 0u;
  keeper->timer81 = 1u;           /* the 0x77F16 copy trigger + the install */
  keeper->has_slot = 0u;
  keeper->pos_x = 0x111;
  keeper->pos_y = 0x222;
  keeper->pos_z = 0x333;
  keeper->target_x = 0xAAA;
  keeper->target_y = 0xBBB;
  keeper->target_z = 0xCCC;
  assert(fifa96_match_entities_team_update(&pool, 0, &f, seam_first_capture,
                                           &cap) == FIFA96_OK);
  assert(cap.calls == (int)FIFA96_MATCH_ENTITY_RECORDS);
  assert(cap.code == 0u);         /* the same-step deferred install (0x78576) */
  assert(cap.target_x == 0xAAA);  /* NO 0x77F20 pos -> target copy */
  assert(cap.target_y == 0xBBB);
  assert(cap.target_z == 0xCCC);
}

/* The install-before-ladder order through the seam: the ladder must be handed
 * the POST-install code — the native `0x78599` call reads the live `+0x91`
 * the `0x78576` install just wrote (`0x7DA67`), not the pre-step value. With
 * a live `timer81` the tree installs code 0, so the ladder sees 0, not 0x19.
 * RED on the pre-fix tree (the ladder received the pre-step 0x19). A stub
 * ladder observes the argument only; the ladder's own output contract is
 * pinned by test_keeper_cpu.c. */
static void test_keeper_seam_ladder_sees_post_install_code(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f;
  fifa96_keeper_cpu cpu;
  struct fifa96_match_entity *keeper;
  struct seam_capture cap;
  struct ladder_capture lcap;
  memset(&f, 0, sizeof f);
  memset(&cap, 0, sizeof cap);
  memset(&lcap, 0, sizeof lcap);
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  fifa96_keeper_cpu_init(&cpu);
  cpu.ladder = ladder_first_capture;
  cpu.ladder_ctx = &lcap;
  f.phase = 2u;
  f.delta = 2u;
  f.keeper_cpu = &cpu;
  cpu.interlock_157c5d = 1u;      /* the ladder-gate input cell */
  keeper = &pool.team[0].records[0];
  keeper->code = 0x19u;
  keeper->skip_9a = 0u;
  keeper->timer81 = 1u;           /* the install tree forces target 0 */
  keeper->has_slot = 0u;
  assert(fifa96_match_entities_team_update(&pool, 0, &f, seam_first_capture,
                                           &cap) == FIFA96_OK);
  assert(cap.calls == (int)FIFA96_MATCH_ENTITY_RECORDS);
  assert(cap.code == 0u);         /* the 0x78576 install applied */
  assert(lcap.calls == 1);        /* the ladder gate did call the ladder */
  assert(lcap.code == 0u);        /* the ladder sees the post-install +0x91 */
}

/* The ladder's install arms through the seam (`0x7821D`-class; fresh FU304
 * §3(A)). Live inputs drive the CX=4 arm exactly as the T4 reach does (the
 * FU290 AX-gate camera, lane < 0x30, the opposite-side actor on team 1), and
 * the stale pre-install posture must not survive: the arm's
 * `FUN_0007D9A4(rec, 0x1B, 0, 1)` stages `+0x92 := 0` before the same-frame
 * dispatch, so the row body starts at stage 0 — not the staged 2 (the G1
 * mis-route class). */
static void test_keeper_seam_ladder_install_full_state(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f;
  fifa96_keeper_cpu cpu;
  struct fifa96_match_entity *keeper;
  struct seam_capture cap;
  memset(&f, 0, sizeof f);
  memset(&cap, 0, sizeof cap);
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  fifa96_keeper_cpu_init(&cpu);
  f.phase = 2u;
  f.delta = 2u;
  f.keeper_cpu = &cpu;
  f.cam_pos_x = 0;
  f.cam_pos_y = 0;
  f.cam_pos_z = -0x800;           /* the FU290 AX-gate camera input */
  cpu.interlock_157c5d = 1u;      /* the ladder-gate input cell */
  pool.controlled = (int32_t)FIFA96_MATCH_ENTITY_RECORDS; /* the team-1 actor */
  keeper = &pool.team[0].records[0];
  keeper->code = 0x19u;
  keeper->skip_9a = 0u;
  keeper->timer81 = 0u;           /* no deferred install (target == code) */
  keeper->has_slot = 0u;
  keeper->lane = 0x28 << 16;      /* lane_x 0x28 < 0x30 -> CX=4 */
  keeper->stage92 = 2u;           /* stale: the installer must overwrite to 0 */
  keeper->timer89 = 0x55;
  keeper->ran = 1u;
  assert(fifa96_match_entities_team_update(&pool, 0, &f, seam_first_capture,
                                           &cap) == FIFA96_OK);
  assert(cap.calls == (int)FIFA96_MATCH_ENTITY_RECORDS);
  assert(cap.code == 0x1Bu);      /* the CX=4 arm's install (0x78299) */
  assert(cap.stage92 == 0u);      /* +0x92 := BL = 0 (the merged contract) */
  assert(cap.timer89 == 0);       /* +0x89 := 0 */
  assert(cap.ran == 0u);          /* +0x9E := 0 */
}

/* FU299 §2/§4 (phase-28 T2) — the interlock ordering, rider (a): the cell
 * survival. The native `[0x157C5D]` is ONE process cell with a one-step pulse:
 * each record-0 step clears it (`0x785A2`, fresh FU299 §1) and the row-`0x19`
 * body sets it mid-walk (`0x74707`), so a frame's LAST row-`0x19` set survives
 * to the next step's gate read (`0x7858E`). The phase-27 engine inverted it:
 * the step's cleared view was written back to the cell after the whole walk
 * (`run.c:4257`), erasing the same-walk row set.
 *
 * This rider: both keepers hold code `0x19` (each dispatch runs the row body's
 * unconditional prologue `r->ran = 1; mr->global_157c5d = 1`); team 1's keeper
 * is the frame's LAST row-`0x19` setter. The default camera triple keeps the
 * stage-select AX gate closed (`|cam.z| > 0x7B0` fails, `0x77F9A/0x77FA5`), so
 * no ladder install can rewrite the codes. The assertion is the cell itself —
 * no gate is staged, the setter is the row body's own live write. RED on the
 * phase-27 tree: the frame-tail write-back zeroes the cell. */
static void test_keeper_interlock_survives_frame(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);

  mr.state.phase = 2;
  mr.state.period_length = 90;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].slot_pool = 0;               /* no live slot source (FU-70) */
  mr.entities.team[0].records[0].code = 0x19u;
  mr.entities.team[0].records[0].skip_9a = 0u;
  mr.entities.team[0].records[0].timer81 = 0u;
  mr.entities.team[0].records[0].has_slot = 0u;
  mr.entities.team[1].records[0].code = 0x19u;
  mr.entities.team[1].records[0].skip_9a = 0u;
  mr.entities.team[1].records[0].timer81 = 0u;
  mr.entities.team[1].records[0].has_slot = 0u;
  assert(mr.global_157c5d == 0u);                  /* no staged interlock */

  one_granted_frame(&mr);
  /* The frame's last row-0x19 set (team 1's keeper) survives the walk. */
  assert(mr.global_157c5d == 1u);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU299 §2/§4 (phase-28 T2) — the interlock ordering, rider (b): the same-frame
 * cross-team arm. Native (derived, FU299-L1): the two per-team walks share the
 * ONE cell, so team 0's row-`0x19` set mid-walk is what team 1's step gate
 * reads in the SAME frame — the primary arming path for the team whose step
 * runs second. The phase-27 engine bound the cell at frame head, so team 1's
 * gate read the pre-frame value and the same-frame arm never fired.
 *
 * This rider: team 0's keeper holds `0x19` (the setter; the frame carries no
 * staged P4). Team 1's keeper holds `0x19` with the T4 ladder-open inputs —
 * lane `0x28`, stage `2`, the AX-gate camera — and the actor MOVED off team 1
 * rec 0 onto team 0 rec 0 (`is_actor` skip `0x77F36` for one, setter for the
 * other). The witness is the ladder's own consequence: the CX=4 arm installs
 * row `0x1B` invoke-now, whose same-frame dispatch reaches the sit-5 emit
 * (phase 9 -> row `0x1E`) — the T4 frame-A trajectory, but for team 1 and with
 * NO staged interlock. RED on the phase-27 tree: team 1's gate reads the
 * frame-head bind (0), the ladder never fires, and the code stays `0x19`. */
static void test_keeper_interlock_cross_team_install(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *k0;
  struct fifa96_match_entity *k1;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);

  mr.state.phase = 2;                              /* P2: the ladder gate phase */
  mr.state.period_length = 90;
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.controlled = 0;                      /* the actor on team 0 rec 0 */
  mr.entities.team[0].slot_pool = 0;               /* no live slot source (FU-70) */
  /* The T4 fixture mirrored for the team that steps second: the controlled team
   * is team 1, so the sit-5 emit's phase-9 arm installs `0x1E` on TEAM 1's
   * keeper (the phase-9 branch's controlled team, `run.c` 0x8D098 arm) — the
   * same assertion T4 makes on team 0. The common install-3 prefix re-installs
   * `0x19` on every inactive record first, so the 0x1E write is the frame's
   * last word on team 1 rec 0. */
  mr.phase_machine.side_controlled = 1;
  mr.global_157a4c = 0;
  mr.global_157c5d = 0;                            /* NO staged P4: the set is in-frame */
  /* The T4 ladder-open camera input, sign-flipped for side 1: the AX gate's
   * pair test is `(cam.z > 0) != (side == 0)` (`0x77FE3`/`0x77FD0`), so a
   * positive z opens it for team 1 where T4's negative z opened it for team 0. */
  mr.render.camera.pos_z = 0x800;

  k0 = &mr.entities.team[0].records[0];
  k0->code = 0x19u;                                /* the setter (row-0x19 body) */
  k0->skip_9a = 0u;
  k0->timer81 = 0u;
  k0->has_slot = 0u;

  k1 = &mr.entities.team[1].records[0];
  k1->code = 0x19u;                                /* the T4 ladder-open record */
  k1->skip_9a = 0u;
  k1->timer81 = 0u;
  k1->has_slot = 0u;
  k1->lane = 0x28 << 16;                           /* the CX=4 lane input */
  k1->stage92 = 2u;                                /* the row-0x1B stage-2 (wide/re-place) */

  /* One frame: team 0's row-0x19 set arms team 1's step gate in the SAME walk;
   * the ladder's CX=4 arm installs `0x1B` and its dispatch reaches the sit-5
   * emit -> phase 9 -> row `0x1E` on team 1's keeper. */
  one_granted_frame(&mr);
  assert(k1->code == 0x1Eu);                       /* cross-team armed + dispatched */
  assert(mr.state.phase == 9u);                    /* situation 5 -> phase 9 */

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU295 §2.2/§2.3 (phase-27 T6): the controlled-side actor lift `FUN_0008C33C`
 * (`0x8C3BE`). Fresh `decompile_function 0x8C33C` (this session): after the reset
 * picks the team target, `if (([0x157AAC]>>0x18) == byte[team+0x826])
 * [0x157A83] = [team+0x7B2];` — the pick is lifted into the actor cell ONLY for
 * the controlled team. The engine models `[0x157A83]` as `pool.controlled` and
 * `[0x157AAC]>>0x18` as the caller's `controlled_side`.
 *
 * The lift is PORTED (`fifa96_match_entities_actor_lift`) but its native caller —
 * the per-frame `0x741C4` driver (`0x742C9`/`0x742D2`; not a Ghidra function, 0
 * xrefs) and the restart sink `FUN_0007417C` (`0x7419F`/`0x741A9`, FU196-L6) — is
 * unported (FU295-L1), so it is NOT wired live: the M2 tape's `pool.controlled`
 * stays NONE (FU295-L5). This test pins the ported lift against the native
 * controlled-side gate; a mutation that drops the side compare, lifts the wrong
 * team, or skips the write fails it. */
static void test_actor_lift_controlled_side(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  /* Distinct sides and picked targets per team (the native reset pick output). */
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.entities.team[0].target = 3;                                       /* team 0 pick */
  mr.entities.team[1].target = (int32_t)(FIFA96_MATCH_ENTITY_RECORDS + 7); /* team 1 pick */
  mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;

  /* Controlled side 0: the NON-controlled team 1 must not touch the actor cell. */
  assert(fifa96_match_entities_actor_lift(&mr.entities, 1u, 0u) == FIFA96_OK);
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
  /* ... while team 0 (side == controlled_side) lifts its pick = 3. */
  assert(fifa96_match_entities_actor_lift(&mr.entities, 0u, 0u) == FIFA96_OK);
  assert(mr.entities.controlled == mr.entities.team[0].target);
  assert(mr.entities.controlled == 3);

  /* Controlled side 1: team 1 now lifts its own pick (overwriting the actor). */
  assert(fifa96_match_entities_actor_lift(&mr.entities, 1u, 1u) == FIFA96_OK);
  assert(mr.entities.controlled == mr.entities.team[1].target);
  /* ... and team 0 does nothing against controlled side 1. */
  mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;
  assert(fifa96_match_entities_actor_lift(&mr.entities, 0u, 1u) == FIFA96_OK);
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);

  /* Guards: NULL pool / out-of-range team -> -INVALID (cell untouched). */
  assert(fifa96_match_entities_actor_lift(NULL, 0u, 0u) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_entities_actor_lift(&mr.entities, 2u, 0u) ==
         -FIFA96_ERR_INVALID);
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
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
  mr.session_gate_14c32a = 1;   /* the byte-1 (Practise) session staged
                                 * (FU-166/R14) */
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
  assert(mr.state.phase == 0u);                     /* FU-241 de-compressed: phase-0 write, act 8 armed */
  assert(mr.act.id == 8u);                          /* the BX=1 fallback dispatches act 8 now */
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
    begin_isolated(&fb, f.engine);
    assert(fb.session_gate_14c32a == 0);            /* begin seeds the settled
                                                     * ordinary-match value
                                                     * (FU-166/R14) */
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

/* FU-241 (M2 phase-23 T4): the BX=1 set-piece fallback now dispatches act 8,
 * whose phase-0x1E restart sweep defers the stored-situation re-dispatch to its
 * stage-2 terminal. After the invoke-now has reset the cursor and swept record
 * (0,0), drive the remaining 22 granted frames (21 stage-1 grants + the terminal)
 * so the deferred arm runs. */
static void match_frame_act8_sweep(struct fifa96_match_run *mr) {
  int k;
  for (k = 0; k < 22; k++)
    assert(fifa96_match_action_phase_1E(mr) == FIFA96_OK);
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
  mr.session_gate_14c32a = 1;   /* the byte-1 (Practise) session staged */
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

/* FU-149 §1.1/§3 item 2 + FU-241 §6.3 (M2 phase-23 T4): the BX!=0 fallback
 * (0x8AA80..0x8AAA3) is DE-COMPRESSED. The native writes phase
 * `FUN_000740A0(0, 0)`, stores `[0x15882C] = side` / `[0x15882B] = situation`,
 * then invokes act 8 (`CALL 0x888FC(8,0,BX=1)`); the engine now dispatches the
 * act-8 phase-0x1E sweep invoke-now (stage 0 -> 1, one record) instead of the
 * in-frame `set_piece` re-dispatch. The stored situation/side re-dispatch and
 * the `[0x15882B] = 0xFF` clear are deferred to the sweep's stage-2 terminal. */
static void test_set_piece_bx_fallback(void) {
  struct fifa96_match_run mr;
  int k;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_set_piece(&mr, 2, 1, 1) == 0);
  assert(mr.state.phase == 0u);                    /* 0x740A0 phase-0 write */
  assert(mr.store_15882c == 1u && mr.store_15882b == 2u);
  assert(mr.act.id == 8u && mr.act.stage == 1u);   /* act 8 invoked, one record swept */
  assert(mr.act8_team == 0u && mr.act8_record == 1u);
  /* drive the remaining 21 stage-1 grants then the stage-2 terminal: the stored
   * sit-2 -> phase-3 arm lands then and store_15882b clears (0x8A8D3). */
  for (k = 0; k < 21; k++)
    assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(mr.act.stage == 2u && mr.store_15882b == 2u);  /* still held */
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* terminal */
  assert(mr.state.phase == 3u);                    /* the deferred sit-2 arm */
  assert(mr.store_15882b == 0xFFu);                /* 0x8A8D3 */
  /* sit 3: the phase-0 write precedes the deferred corner arm; drive the sweep
   * to its stage-2 terminal so the deferred sit-3 arm (phase 4 + the corner
   * increment) lands, as the sit-2 block above does. */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  assert(fifa96_match_run_set_piece(&mr, 3, 1, 1) == 0);
  assert(mr.state.phase == 0u && mr.state.prev_phase == 2u);
  assert(mr.corner_count[1] == 0u);                /* deferred, not in-frame */
  assert(mr.store_15882c == 1u && mr.store_15882b == 3u && mr.act.id == 8u);
  for (k = 0; k < 21; k++)
    assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(mr.act.stage == 2u && mr.store_15882b == 3u);  /* still held */
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* terminal */
  assert(mr.state.phase == 4u);                    /* the deferred sit-3 corner arm */
  assert(mr.corner_count[1] == 1u);                /* the corner increment (0x8ABFF) */
  assert(mr.store_15882b == 0xFFu);                /* 0x8A8D3 */
  /* keeper restart through the fallback: drive to the terminal so the deferred
   * sit-5 arm (phase 9) lands. */
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_set_piece(&mr, 5, 0, 1) == 0);
  assert(mr.state.phase == 0u && mr.store_15882b == 5u && mr.act.id == 8u);
  for (k = 0; k < 21; k++)
    assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* terminal */
  assert(mr.state.phase == 9u);                    /* the deferred sit-5 keeper arm */
  assert(mr.store_15882b == 0xFFu);                /* 0x8A8D3 */
}

/* FU-241 §6 (M2 phase-23 T4): the phase-0x1E de-compression pin. The engine now
 * registers the phase-`0x1E` slot and the `set_piece` `bx != 0` branch dispatches
 * act 8 invoke-now (`fifa96_match_act_dispatch(mr, 8, 0, 1)`), which resets the
 * sweep cursor and sweeps one record; the stored-situation re-dispatch is
 * deferred ~22 granted frames to the sweep's stage-2 terminal. The faithful
 * sweep is render-invisible and dormant on the M2 tape (FU-241 §6.1/§6.2), so no
 * presented frame moves. Mutation-RED: a NULL slot or a compressed in-frame
 * re-dispatch aborts the `fn`/phase/act asserts below. */
static void test_phase_1E_decompressed_pin(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_phase_table[0x1E].fn == fifa96_match_action_phase_1E);
  /* Pre-seed the sweep cursor with sentinels so the reset is falsifiable — the
   * de-compressed path runs stage 0 and overwrites them. */
  mr.act8_team = 3u;
  mr.act8_record = 7u;
  mr.act8_selected = &mr.entities.team[0].records[5];
  /* the de-compressed `bx != 0` path: phase 0, store side/situation, dispatch act
   * 8 invoke-now (native 0x8AA80..0x8AAA3 -> CALL 0x888FC(8,0,BX=1)). */
  assert(fifa96_match_run_set_piece(&mr, 2, 1, 1) == 0);
  assert(mr.state.phase == 0u);                        /* 0x740A0 */
  assert(mr.store_15882c == 1u && mr.store_15882b == 2u);
  assert(mr.act.id == 8u && mr.act.stage == 1u);       /* act 8 installed + swept one */
  assert(mr.act8_team == 0u && mr.act8_record == 1u);  /* stage 0 reset, stage 1 advanced */
  assert(mr.act8_selected == NULL);                    /* record 0 fails the select predicate */
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
 * arms the throw-in (sit 2 BX=1 -> act 8 -> the deferred phase 3 + taker 0x10)
 * and the corner (sit 3 -> phase 4 + taker 0x11); the armed record is driven
 * through the dispatch seam with the whole-frame delta staged and the rows hand
 * back to phase 2 through the shared situation-0xB entry with the 0x12C
 * offside-suppression timer. */
static void test_taker_armed_rows_resolve(void) {
  struct fifa96_match_run mr;
  int k;
  /* throw-in: sit 2 through the BX=1 fallback (the direct path with a pending
   * situation); the phase-3 arm is DEFERRED to the sweep terminal (FU-241), so
   * drive the 22-record sweep to completion first, then the stored-sit arm
   * picks team 0 record 1 */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.state.phase = 2;
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  assert(fifa96_match_run_set_piece(&mr, 2, 0, 1) == 0);
  assert(mr.act.id == 8u && mr.state.phase == 0u);     /* act 8 invoked, phase 0 */
  for (k = 0; k < 21; k++)
    assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);
  assert(fifa96_match_action_phase_1E(&mr) == FIFA96_OK);  /* stage-2 terminal */
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

/* FU274 §3/§4 (phase-26b Task 6): the live throw-in taker trigger (row 0x10).
 * The ported scanner (`fifa96_match_run_goal_scan`) and armer
 * (`fifa96_match_goal_arm`) already reproduce the native arms; the missing
 * precondition is a live **phase-2 sideline excursion** — `goal_armed &&
 * phase == 2 && |snap z| <= 0xB20` (native `0x713A6..0x713F7` then `0x88BCC`).
 * The M2 tape arms only the goal arm (its camera sits beyond the goal line), so
 * this drives the real producer path: the natural kickoff chain reaches live
 * phase 2 (row 01 + the KICK release), then the camera is staged out of bounds
 * on the sideline. The frame body's own armer (the `fifa96_camera_out_of_bounds`
 * call at `run.c:4205`) raises `goal_armed` from the staged camera, and the
 * scanner fires sit 2 -> the BX=1 fallback (FU-241: phase-0 write + act-8 sweep);
 * the sweep's stage-2 terminal runs the deferred phase-3 arm (taker 0x10) and
 * the entity chain dispatches it. Mutation-RED: without the sideline excursion
 * (or with the armer/scanner mutated) the taker bit never lands. */
static void test_throw_in_taker_reachable(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  /* the natural kickoff chain -> live phase 2 (row 01 + the KICK release). */
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 6 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  assert(mr.goal_armed == 0u);
  /* the negative control: the parked camera is not out of bounds, so no throw-in
   * is queued and the taker row never lands. */
  assert((mr.dispatched_ok & (1ull << 0x10)) == 0u);

  /* stage the sideline excursion (the camera the armer reads): `|camX| > 0x730`
   * and `|snap z| <= 0xB20` (the throw-in band, not the goal band). A parked
   * camera does not move (the walk gates on speed/event_param == 0). */
  mr.render.camera.pos_x = 0x740;
  mr.render.camera.pos_z = 0;

  /* frame 1: the real armer fires (out-of-bounds) and the scanner fires sit 2
   * through the BX=1 fallback (phase 0 + act 8); the sweep defers the phase-3
   * arm ~22 granted frames. */
  one_granted_frame(&mr);
  assert(mr.goal_armed == 1u);          /* the live armer raised the gate */
  assert(mr.goal_snap_x == 0x740 && mr.goal_snap_z == 0);
  assert(mr.state.phase == 0u);         /* the BX=1 fallback phase write */
  assert(mr.act.id == 8u);              /* the act-8 sweep invoked */
  /* drive the act-8 sweep terminal (the deferred sit-2 phase-3 arm) and then
   * the entity chain that dispatches the installed taker row. */
  for (int i = 0; i < 80 && !(mr.dispatched_ok & (1ull << 0x10)); i++)
    one_granted_frame(&mr);
  assert(mr.state.phase == 3u);         /* the deferred sit-2 arm landed */
  assert((mr.dispatched_ok & (1ull << 0x10)) != 0u);  /* row 0x10 ran live */

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU277 §4.1/§4.3 (phase-26b Task 7): the keeper **row 0x1D** live trigger.
 * The whole sit-4 install chain is already ported and sit-4-capable: the
 * goal-scan's corner/goal-kick arm (`fifa96_match_run_goal_scan` run.c:3316)
 * emits `situation = 3u + cond` with `cond = ((goal_snap_z < 0) == (ball == 1))`
 * (run.c:3315-3317), the direct table-2 sit-4 row writes phase 8 and
 * `fifa96_match_run_phase_arm` installs code **0x1D** on the controlled team's
 * record 0 (run.c:2854-2861). The gate is `phase ∈ {2,0x10} && goal_armed &&
 * goal_zone == 0 && |snap z| > 0xB20` (run.c:3284-3285/:3305/:3293) — its live
 * producer is the ported armer `fifa96_match_goal_arm`, run from the frame body
 * only when the camera leaves the bounds (run.c:4205-4207). This drives the
 * real producer path (never a cell poke): the natural kickoff chain reaches
 * live phase 2 (the FU274 §3 T6 idiom), then the camera is staged past the goal
 * line in the OUT-of-mouth band (`|camera z| word >= 0xB90` ⇒ the goal-mouth
 * classifier returns zone 0) so the scanner takes the goal-kick arm and, with
 * the sign chosen from the live ball side, emits situation 4. Mutation-RED:
 * without the out-of-bounds camera staging (or with the armer/scanner mutated)
 * `goal_armed` never latches and row 0x1D never dispatches. Row 0x1E (the
 * keeper CPU ladder, R11-L4) is out of scope.
 *
 * The row is the *trigger* deliverable (FU277-L1's live half): the install and
 * the dispatch are proven here. The row's own restart emit (the shared sit-0xB
 * tail at closedown stage 3, `fifa96_keeper.c:961`) is NOT asserted: the row
 * machine advances stage 0 -> 1 then holds at stage 1, whose `has_slot != 0`
 * arm needs a `slot_edge & 0x50` / `slot_pressed & 0x20` input edge that only
 * the unported keeper slot/CPU path drives (FU277-L6 / R11-L4, out of scope).
 * The restart *route* needs no new port (FU277 §2.1); its live reach is that
 * downstream leg, recorded honestly rather than faked. */
static void test_keeper_1d_goal_kick_reachable(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  int32_t id;
  uint8_t ball;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  /* the natural kickoff chain -> live phase 2 (row 01 + the KICK release). */
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 6 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);                         /* live play phase */
  assert(mr.goal_armed == 0u);
  /* settle one play frame so the pool's ball side (the scanner's `cond` input)
   * is live, and pin the negative control: the in-bounds parked camera never
   * arms the scan and row 0x1D never dispatches. */
  one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  assert(mr.goal_armed == 0u);
  assert((mr.dispatched_ok & (1ull << 0x1D)) == 0u);

  /* stage the goal-line excursion the armer reads: beyond the goal line in the
   * OUT-of-mouth band (`|camera z| word >= 0xB90` -> the classifier's zone 0)
   * and `|snap z| > 0xB20` (the goal-kick band, not the throw-in band). The
   * sign is chosen from the live ball side so the scanner's `cond` is 1 and the
   * emit is situation 4 (goal kick) rather than situation 3 (corner). A parked
   * camera does not move (the walk gates on speed/event_param == 0). */
  id = mr.entities.controlled;
  if (id < 0) id = mr.entities.ball.carrier;
  ball = (id >= 0 &&
          id < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
             ? mr.entities.team[(uint32_t)id / FIFA96_MATCH_ENTITY_RECORDS].side
             : 0u;
  mr.render.camera.pos_x = 0;
  mr.render.camera.pos_z = ball == 1u ? -0xB90 : 0xB90;

  /* one granted frame: the real armer fires (out of bounds) with the snapshot
   * in the out-of-mouth band, and the scanner emits sit 4 -> the direct table-2
   * phase-8 arm installs row 0x1D on the controlled team's record 0. */
  one_granted_frame(&mr);
  assert(mr.goal_armed == 1u);         /* the live armer raised the gate */
  assert(mr.goal_zone == 0u);          /* the excursion is out of the mouth */
  assert(mr.state.phase == 8u);        /* the sit-4 phase-8 arm landed */
  assert(mr.entities.team[0].records[0].code == 0x1Du);  /* the keeper row */

  /* the entity chain dispatches the installed keeper row 0x1D. */
  for (int i = 0; i < 16 && !(mr.dispatched_ok & (1ull << 0x1D)); i++)
    one_granted_frame(&mr);
  assert((mr.dispatched_ok & (1ull << 0x1D)) != 0u);  /* row 0x1D ran live */

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
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
    begin_isolated(&fb, f.engine);
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
  mr.session_gate_14c32a = 1;   /* the byte-1 (Practise) session staged */
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
  assert(mr.act.id == 8);                        /* FU-241: the BX=1 fallback dispatches act 8 */
  assert(mr.ref_restart_stage == 0u);            /* the act-2 hand-off is deferred */
  assert(mr.state.phase == 0u);                  /* the phase-0 write */
  assert(mr.ref_whistle == 0u);                  /* the soft-foul whistle is deferred too */
  /* FU-241: drive the act-8 sweep to its terminal so the deferred sit-9 -> act-2
   * hand-off (stage 0) runs, then the act-2 decision step (the act-machine tick):
   * phase 7 + the FK taker on the controlled team; the terminal clears the slot */
  match_frame_act8_sweep(&mr);
  assert(mr.act.id == 2);
  assert(mr.ref_restart_stage == 1u);            /* stage 0 ran at the terminal */
  assert(mr.state.phase == 0x0Au);               /* act-2 stage 0 phase write */
  assert(mr.ref_whistle == 0x1Eu);               /* the soft-foul whistle */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.state.phase == 7u);
  assert(mr.act.id == 0);
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
  assert(mr.act.id == 8 && mr.state.phase == 0u);  /* FU-241: deferred */
  match_frame_act8_sweep(&mr);
  assert(mr.state.phase == 0x0Au);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
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
  match_frame_act8_sweep(&mr);                 /* FU-241: the deferred sit-9 -> act-2 arm */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
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
  match_frame_act8_sweep(&mr);                 /* FU-241: the deferred sit-9 -> act-2 arm */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
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

/* FU-166/R14 + FU-150 (T3 re-review round 2): the act-2 net rule under the
 * settled byte-0 default. The `0x89487` abort is a **one-call split**: the
 * stage byte is incremented before it (`0x89478/0x8947F` -> 3) and the shared
 * epilogue (`0x89615`) skips the `0x895E6` teardown, so the machine stays
 * armed and the next driver call dispatches `table[3] = 0x894A2`. The
 * stage-3 gate then decides on the ball row-terminal flag `[0x158882]`:
 * kind == 3 leaves it 1 (the `0x8941D` keep/reroll install lands rows
 * 0/0x62/0x63/0x64/0x65, flags 0x07), so the decision completes on the
 * re-entry call;
 * kind != 3 installs row 0x49 at `0x89464` (flags 0x00) and the gate-0 term
 * aborts (`0x894B7`), still armed at stage 3. The engine models the split as
 * `ref_restart_stage = 2`.
 *
 * FU289 (phase-26c Task 2): the stage-3 gate's `kind` is the **contact-kind
 * cell** `[0x15888C]` (byte `contact_kind`), read at `0x894AD MOV AL,[0x15888c];
 * CMP EAX,3` — NOT the foul-severity cell `[0x15888D]` (`foul_kind`). The gate
 * below stages `contact_kind`; case C stages `foul_kind` to pin the contrast. */
static void test_engine_referee_restart_default_gate_split(void) {
  struct fifa96_match_run mr;
  const int32_t point[3] = {0x500, 0x777, 0};
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 1;             /* soft: kind stays 0 */
  mr.session_gate_14c32a = 0;            /* the settled ordinary-match default */
  mr.situation_pending = 1;              /* force the direct path */
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  assert(fifa96_match_run_contact(&mr, 1, 5, 6, point) == 1);
  assert(mr.referee.foul_kind == 0u);
  assert(mr.act.id == 8u && mr.state.phase == 0u);  /* FU-241: deferred */
  match_frame_act8_sweep(&mr);
  assert(mr.act.id == 2);
  assert(mr.ref_restart_stage == 1u);    /* stage 0 ran: whistle + phase 0xA */
  assert(mr.state.phase == 0x0Au);
  assert(mr.ref_whistle == 0x1Eu);
  /* the split: the machine stays armed, the stage advances, no decision */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.id == 2);
  assert(mr.ref_restart_stage == 2u);
  assert(mr.state.phase == 0x0Au);
  assert(mr.ref_speech == 0u);
  /* the stage-3 hold (kind != 3, gate 0): still armed, still no decision */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.id == 2);
  assert(mr.ref_restart_stage == 2u);
  assert(mr.state.phase == 0x0Au);
  assert(mr.ref_speech == 0u);
  assert(mr.entities.team[1].target != 12);       /* no FK taker */
  assert(mr.entities.team[1].records[1].code != 0x12u);

  /* case B: the FU-289 cell. The stage-3 gate reads the **contact-kind** cell
   * `[0x15888C]` (`0x894AD`/`0x894DB`); staging `contact_kind = 3` completes on
   * the re-entry call — the stage-2 split, then the stage-3 gate via the kind-3
   * term -> phase 7 / row 0x12. */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 1;
  mr.session_gate_14c32a = 0;
  mr.situation_pending = 1;
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  assert(fifa96_match_run_contact(&mr, 1, 5, 6, point) == 1);
  assert(mr.act.id == 8u && mr.state.phase == 0u);  /* FU-241: deferred */
  match_frame_act8_sweep(&mr);
  mr.referee.contact_kind = 3u;          /* FU-289: the [0x15888C] taker cell */
  assert(mr.act.id == 2);
  assert(mr.state.phase == 0x0Au);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* the split */
  assert(mr.act.id == 2);
  assert(mr.ref_restart_stage == 2u);
  assert(mr.state.phase == 0x0Au);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* the stage-3 decision */
  assert(mr.state.phase == 7u);
  assert(mr.act.id == 0);
  assert(mr.ref_speech == 0x2Au);
  assert(mr.entities.team[1].target == 12);
  assert(mr.entities.team[1].records[1].code == 0x12u);

  /* case C (FU-289 contrast): the WRONG cell. `foul_kind = 3` with
   * `contact_kind != 3` must NOT complete — the native gate reads `[0x15888C]`,
   * never the severity cell `[0x15888D]`, so the machine stays armed at stage 2
   * and installs neither taker row. */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c306 = 1;
  mr.session_gate_14c32a = 0;
  mr.situation_pending = 1;
  match_frame_stage_restarts(&mr);
  match_frame_rng_arm_hard_foul(&mr);
  assert(fifa96_match_run_contact(&mr, 1, 5, 6, point) == 1);
  assert(mr.act.id == 8u && mr.state.phase == 0u);  /* FU-241: deferred */
  match_frame_act8_sweep(&mr);
  assert(mr.referee.contact_kind == 1u); /* the live row-0x0C re-call kind */
  mr.referee.foul_kind = 3u;             /* the severity cell: ignored */
  assert(mr.act.id == 2);
  assert(mr.state.phase == 0x0Au);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* the split */
  assert(mr.act.id == 2);
  assert(mr.ref_restart_stage == 2u);
  assert(mr.state.phase == 0x0Au);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* the stage-3 gate holds */
  assert(mr.act.id == 2);
  assert(mr.ref_restart_stage == 2u);
  assert(mr.state.phase == 0x0Au);
  assert(mr.ref_speech == 0u);
  assert(mr.entities.team[1].records[1].code != 0x12u);
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

/* FU-150 §Port contract / FU-187 §5 (phase-16 T4): the hard-foul chain through
 * the act-3 -> phase-0x19 machine. The kind-1 decision dispatches act 3
 * invoke-now (the native FUN_000888FC(3,0,1)), which runs the phase-0x19 stage 0
 * (whistle, foul counter, phase 0xF on the fouled side, install action 0x16);
 * the act clock-tail tick advances stages 1..6 (speech, severity) to the stage-6
 * situation 0xA hand-off, which (with the pending latch) starts the act-2
 * machine at stage 1 and decides the penalty phase 6 in the same call. The
 * ad-hoc `ref_machine = REF_FOUL` route is retired (the act machine owns the
 * sequence). */
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
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);  /* the FOUL route retired */
  assert(mr.act.id == 3u);                       /* the act machine owns the foul */
  assert(mr.act.stage == 1u);                    /* stage 0 ran invoke-now */
  assert(mr.state.phase == 0x0Fu);               /* phase 0xF on the fouled side */
  assert(mr.ref_whistle == 0x1Eu);
  assert(mr.referee.fouls_by_side[0] == 1u);
  assert(mr.entities.team[0].records[5].code == 0x16u);  /* install 0x16 */
  /* stage 1 gate -> stage 2 speech */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.stage == 2u);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.ref_speech == 0x0Du && mr.act.stage == 3u);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* stage 3 -> 4 */
  assert(mr.act.stage == 4u);
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);   /* stage 4: sum 1 -> 6 */
  assert(mr.act.stage == 6u);
  assert(mr.referee.severity[0][5] == 1u);
  /* stage 6: situation 0xA -> the act-8 sweep (FU-241) then the act-2 hand-off
   * decides phase 6 */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.id == 8u && mr.state.phase == 0u);   /* FU-241: delegated to act 8 */
  match_frame_act8_sweep(&mr);
  assert(mr.act.id == 0u);                       /* the act-2 terminal cleared it */
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
  for (int i = 0; i < 4; i++) assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.act.stage == 5u);
  assert(mr.entities.team[0].records[5].code == 0x18u);  /* the downed install */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);       /* held 0: stall */
  assert(mr.act.stage == 5u && mr.referee.team_count[0] == 9u);
  mr.referee.rec_first_held = 1;
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.referee.team_count[0] == 8u && mr.act.stage == 6u);
}

/* FU276-L1 (phase-26b Task 4): the live contact-gate reach proof. The contact
 * chain row-0x0B arm-1 -> row-0x0C -> `fifa96_match_run_contact` is
 * producer-wired but no granted-frame test raised it. Here the real pad
 * `0x7CE38` producer (the `want 0x60` handler `match_run.c:693-700` = native
 * `0x7CE63..0x7CE93`) installs action 0x0B with invoke on the slot-bound
 * record; row-0x0B arm 0 (`0x81AFB`) installs the contact anim {0x0C} and arms
 * stage 1; the next frame's arm 1 (`0x81B1F`..`0x81BEB`) registers contact kind
 * 2 and installs action 0x0C on the found opponent, whose row-0x0C stage 0
 * (`match_handlers.c:3329`) re-calls `fifa96_match_run_contact` (the native
 * block `0x81E8A..0x81ED4`) -> the kind-1 decision dispatches act 3 invoke-now
 * (phase-0x19 stage 0: whistle, phase 0x0F, install action 0x16 on the fouler).
 *
 * The row-0x0B arm-1 gate is the record animation-frame byte `byte[+0x3D]`
 * (`0x81B1F`); its live producer is the render-gated FU-84 advance
 * `FUN_0008E008` (`match_run_scene_stage`, run only when `render.enabled`), so
 * a headless fixture stages the byte between frames exactly as the direct-walk
 * dual tests do (`test_action_0b_duel_chain_installs_row_5e`,
 * handlers `:2108` "native +0x3D (producer OL-80/OL-82)"). The gate cell
 * `ref_machine` is never touched; the contact path routes through the act
 * machine (FU175-L7 retired the ad-hoc `ref_machine` FOUL route), so the gate
 * consequence is `act.id == 3` / `state.phase == 0x0F`. */
static void test_contact_gate_reachable_via_pad_duel(void) {
  struct fifa96_match_run mr;
  struct fifa96_match_entity *fouler;   /* the duel initiator (A) */
  struct fifa96_match_entity *found;    /* the challenged record (B) */
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;                    /* live in-play phase */
  mr.state.period_length = 90;
  mr.config.field_4c306 = 2;             /* the fouls master on (0x14C306) */
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  fouler = &mr.entities.team[0].records[4];
  found = &mr.entities.team[1].records[4];
  /* A: slot-bound, standing (code 0, type_bits & 1), facing +x, band 0x40 from
   * B per the direct-walk fixture (`0x300/0x200` vs `0x340/0x200`). */
  fouler->has_slot = 1;
  fouler->active = 1;
  fouler->code = 0;
  fouler->type = 2;
  fouler->pos_x = 0x300;
  fouler->pos_z = 0x200;
  fouler->anim_id = 0;
  fouler->frame = 0;
  /* B: the nearest opponent, active, band 0x40 (in the arm-0 (0x20,0x70) and
   * arm-1 <= 0x60 windows). */
  found->active = 1;
  found->pos_x = 0x340;
  found->pos_z = 0x200;
  mr.slot.entity = 4;
  mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;   /* not the tracked self */
  mr.entities.team[0].chosen = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  /* isolate the two duel records (native +0x9A pool skip). */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!((t == 0u && i == 4u) || (t == 1u && i == 4u)))
        mr.entities.team[t].records[i].skip_9a = 1;

  /* the pad `want 0x60` press: PASS|LONG_BALL -> mapped 0x60 (the real input
   * map, `match_run_map_key`) -> the scan dispatch code 0 -> handler 0x7CE38. */
  {
    fifa96_platform_key keys[2] = {{FIFA96_ENGINE_KEY_PASS, 1},
                                   {FIFA96_ENGINE_KEY_LONG_BALL, 1}};
    assert(fifa96_match_run_input(&mr, keys, 2) == 0);
  }

  /* granted frame 1: the pad handler installs 0x0B (invoke); row-0x0B arm 0
   * runs, installs the 0x0C contact anim and arms stage 1. */
  one_granted_frame(&mr);
  assert(fouler->code == 0x0Bu);         /* the pad handler's install */
  assert(fouler->stage92 == 1u);         /* arm 0 armed arm 1 */
  assert(fouler->anim_id == 0x0Cu);      /* the standing contact anim choice */
  assert(mr.referee.contact_kind == 0u); /* no contact yet (arm 1 waited) */

  /* the arm-1 frame-byte producer (render-gated headless): stage `byte[+0x3D]`
   * non-zero so arm 1 (`0x81B1F`) fires next frame. */
  fouler->frame = 3u;

  /* granted frame 2: arm 1 registers kind 2 + installs row 0x0C on B; B's
   * row-0x0C stage 0 re-calls `fifa96_match_run_contact` -> kind 1 -> act 3. */
  one_granted_frame(&mr);
  assert(mr.referee.rec_first == 4);     /* A = the initiator */
  assert(mr.referee.rec_second == 15);   /* B = the challenged (team 1 rec 4) */
  assert(mr.entities.team[0].duel_7d7 == 1u);      /* arm-1 `0x81B83` */
  assert(mr.referee.contact_kind == 1u); /* `fifa96_match_run_contact` ran */
  assert(mr.referee.recall_consumed == 1u);        /* past the gate + draw */
  assert(mr.act.id == 3u);               /* the act-3 foul sequence dispatched */
  assert(mr.state.phase == 0x0Fu);       /* phase 0x19 stage 0 phase write */
  assert(fouler->code == 0x16u);         /* install action 0x16 on the fouler */
  assert(mr.referee.fouls_by_side[0] == 1u);   /* the stage-0 foul counter */
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);   /* the retired route */
}

/* FU274 §3/§4 + FU166-L6 (phase-26b Task 6): the foul-taker honest-stop. The
 * ported chain (pad `0x7CE38` -> row 0x0B -> row 0x0C ->
 * `fifa96_match_run_contact` -> act 3 -> phase 0x19 -> sit 0xA -> act 2) is live
 * and faithful, and Task 4 proved the contact **entry** reachable. But the act-2
 * terminal (`match_run_ref_step_restart` stage 2) completes only on the native
 * gate `contact_kind == 3 || session != 0`: the live path emits a **non-foul
 * contact kind (1 ≠ 3)** (`run.c:6542`) and the settled ordinary-match session
 * gate is 0 (FU-166/R14), so the decision never completes — the machine holds
 * armed at stage 2. The final review (FU280) re-disassembled the act-2
 * stage-2/3 body and corrected the old "the native completes via the unported
 * `[0x158882]` leg" attribution: at `0x8941D` an unconditional `FUN_0006E724`
 * install sets `[0x158882]=1`; the kind ≠ 3 branch runs a second install at
 * `0x89464` (`EDX=0x49`) resetting it to 0, skipped only when `[0x15888C]==3`
 * (`0x8944A`), so the stage-3 gate (`0x894A2`) reduces to `contact_kind == 3 ||
 * session != 0` — i.e. `[0x158882] ≡ (contact_kind == 3)` is **already
 * proxied**, and porting it would **not** flip the rows. The next phase needs a
 * live **`contact_kind == 3`** producer. FU289 (phase-26c Task 2) closed the
 * cell divergence: the engine's gate now reads `contact_kind`/`[0x15888C]`
 * (`run.c:6002/6009`), matching the native `0x894AD`/`0x894DB`. Rows 0x12/0x13
 * therefore still stay **LIVE-LEG** (the live path emits `contact_kind == 1`);
 * this pins that the LIVE default path reaches act-2 stage 2 and installs
 * neither taker row. */
static void test_foul_taker_held_on_live_default(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *fouler;   /* the duel initiator (A) */
  struct fifa96_match_entity *found;    /* the challenged record (B) */
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  /* the natural kickoff chain -> live phase 2 (row 01 + the KICK release). */
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 6 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  assert(mr.session_gate_14c32a == 0u);   /* the settled ordinary-match default */
  assert(mr.config.field_4c306 == 2u);    /* the live fouls gate (level 2) */

  /* stage the duel geometry and drive the real pad `0x60` contact (the path
   * Task 4 proved): A = the slot-bound initiator, B = the nearest opponent. */
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  fouler = &mr.entities.team[0].records[4];
  found = &mr.entities.team[1].records[4];
  fouler->has_slot = 1;
  fouler->active = 1;
  fouler->code = 0;
  fouler->type = 2;
  fouler->pos_x = 0x300;
  fouler->pos_z = 0x200;
  fouler->anim_id = 0;
  fouler->frame = 0;
  found->active = 1;
  found->pos_x = 0x340;
  found->pos_z = 0x200;
  mr.slot.entity = 4;
  mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.team[0].chosen = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!((t == 0u && i == 4u) || (t == 1u && i == 4u)))
        mr.entities.team[t].records[i].skip_9a = 1;
  {
    fifa96_platform_key keys[2] = {{FIFA96_ENGINE_KEY_PASS, 1},
                                   {FIFA96_ENGINE_KEY_LONG_BALL, 1}};
    assert(fifa96_match_run_input(&mr, keys, 2) == 0);
  }
  /* frames 1-2: the pad handler -> row 0x0B -> row 0x0C -> the contact. */
  one_granted_frame(&mr);
  fouler->frame = 3u;                     /* the arm-1 `+0x3D` frame-byte gate */
  one_granted_frame(&mr);
  assert(mr.referee.contact_kind == 1u);  /* the live contact ran */
  assert(mr.act.id == 8u && mr.state.phase == 0u);  /* the deferred BX=1 fallback */
  /* drive the chain: the act-8 sweep -> the deferred sit-9/sit-0xA arm -> act 2
   * stage 0/1 -> the stage-2 terminal gate, which holds (no live completion). */
  for (int i = 0; i < 60; i++) one_granted_frame(&mr);
  assert(mr.act.id == 2u);                /* the act-2 machine, still armed */
  assert(mr.ref_restart_stage == 2u);     /* the stage-3 split reached, held */
  assert(mr.state.phase == 0x0Au);        /* no decision phase write */
  /* the honest-stop: neither foul-taker row is installed on the live default. */
  assert((mr.dispatched_ok & ((1ull << 0x12) | (1ull << 0x13))) == 0u);
  assert(mr.entities.team[1].records[1].code != 0x12u);
  assert(mr.entities.team[1].records[1].code != 0x13u);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
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
  assert(mr.act.id == 8u && mr.state.phase == 0u);   /* FU-241: deferred to act 8 */
  match_frame_act8_sweep(&mr);
  assert(mr.act.id == 2);
  assert(mr.state.phase == 0x0Au);               /* act-2 stage 0 on return */
  assert(fifa96_match_act_tick(&mr) == FIFA96_OK);
  assert(mr.state.phase == 7u);                  /* kind 3 forces free kick */
  assert(mr.act.id == 0);
  assert(mr.ref_speech == 0x2Au);
  /* phase-17 T3: the native per-record walk now runs the phase-0x0A handler
   * (0x6DE34, `target := position`) on the 0x0A writes, so the phase-7
   * nearest pick (which stands in for the phase-handler placement output) sees
   * the reset positions — the opponent-side record 2 at (0, 0x3C0) is the
   * strict nearest to the incident (0, 0x3C0). */
  assert(mr.entities.team[1].records[2].code == 0x12u);  /* FK on the fouled side */
  assert(mr.entities.team[1].records[2].target_x == 0);
  assert(mr.entities.team[1].target == 13);

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

/* FU268-L10: the record-state gate (0x79EF7/0x79F00) reads the native `code`
 * byte at +0x91, not the 0-7 `type` octant at +0x8E. The own-nearest record
 * with code == 0x11 is therefore ineligible and the check must not fire; the
 * same geometry with an eligible code fires (the control). */
static void test_engine_offside_l10_reads_code_not_type(void) {
  struct fifa96_match_run mr;
  struct fifa96_ref_metric metric;
  uint8_t offside = 0xAA;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2;
  mr.config.field_4c2f2 = 1;                     /* offside enabled */
  mr.session_gate_14c32a = 1;
  mr.situation_pending = 1;
  match_frame_stage_offside(&mr);
  /* the own-nearest record 1: code == 0x11 with a valid octant type */
  mr.entities.team[0].records[1].type = 0u;
  mr.entities.team[0].records[1].code = 0x11u;
  (void)fifa96_rng_seed(&mr.rng, 0);
  metric.tol2 = 0;
  metric.side_gate = 1;
  assert(fifa96_match_run_offside_reception(&mr, 3, &metric, 0, 0, &offside) == 0);
  assert(offside == 0u);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  /* control: an eligible code (0x12) on the same record fires the check */
  mr.entities.team[0].records[1].code = 0x12u;
  (void)fifa96_rng_seed(&mr.rng, 0);
  assert(fifa96_match_run_offside_reception(&mr, 3, &metric, 0, 0, &offside) == 1);
  assert(offside == 1u);
}

/* Stage the live-receive offside geometry (team 0 = side 0): the ball sits at
 * the side-0 own-nearest (record 1); the receiver/actor (record 3) is level with
 * the team-1 last defender (record 2), beyond the own-nearest depth gate and
 * within the own-nearest distance gate; every other record is parked far so the
 * two nearest queries are deterministic. */
static void match_live_receive_offside_stage(struct fifa96_match_run *mr) {
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      mr->entities.team[t].records[i].pos_x = 0x6000;
      mr->entities.team[t].records[i].pos_z = 0x6000;
      mr->entities.team[t].records[i].skip_98 = 0;
      mr->entities.team[t].records[i].skip_9a = 0;
    }
  mr->entities.team[0].side = 0;
  mr->entities.team[1].side = 1;
  mr->entities.ball.x = 0;
  mr->entities.ball.z = 0x800;
  mr->entities.team[0].records[1].pos_x = 0;      /* the own-nearest */
  mr->entities.team[0].records[1].pos_z = 0x800;
  mr->entities.team[0].records[3].pos_x = 0;      /* the receiver (actor) */
  mr->entities.team[0].records[3].pos_z = 0x400;
  mr->entities.team[1].records[2].pos_x = 0;      /* the last defender */
  mr->entities.team[1].records[2].pos_z = 0x400;
  mr->entities.team[0].records[3].code = 0u;      /* eligible +0x91 code */
  mr->entities.team[0].records[3].active = 1u;
  mr->render.camera.pos_z = 0;                    /* |camera_ref| <= 0x990 */
}

/* FU276-L2 (phase-26b Task 5): the live receive arm FUN_0007A084 calls the
 * offside check at 0x7A448 behind its own `code` (+0x91) eligibility gate
 * (0x7A419..0x7A43F). The engine's live port (`fifa96_match_run_ball_receive`,
 * the exact callee of the live `match_kick_run`, match_handlers.c:1119) dropped
 * that call as a declared leg, leaving `fifa96_match_run_offside_reception`
 * callerless. This drives the live receive arm with the enable `field_4c2f2`
 * (0x14C2F2) on and an eligible receiving record: the offside chain must
 * dispatch (ref_machine -> REF_OFFSIDE, the kind-3 event + phase-0x0A stage 0).
 * The enable-off control must stay dormant. */
static void test_offside_reachable_from_live_receive(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_run mr2;
  fifa96_ball_pair_vector vec;
  /* The 0x158738 staged block IS the vector the arm receives; the offside check
   * reads its +0 word as `tol2` and its +4 word as `side_gate`. tol2 0x200 makes
   * the x2 distance term pass for any 6-bit tolerance (0x400 + 0x400 >= 0x800);
   * side_gate 1 (positive) admits the side-0 receiver. */
  vec.x = 0x200;
  vec.height = 0;
  vec.z = 1;

  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  mr.state.phase = 2;                              /* the phase-2 pre-gate */
  match_live_receive_offside_stage(&mr);
  mr.config.field_4c2f2 = 1u;                      /* offside enabled */
  assert(fifa96_match_run_ball_receive(&mr, 3, &vec, 0x100) == FIFA96_OK);
  assert(mr.ref_machine == FIFA96_MATCH_RUN_REF_OFFSIDE);  /* 0x7A448 fired */
  assert(mr.referee.contact_kind == 3u);           /* the kind-3 offside event */
  assert(mr.ref_whistle == 0x1Eu);
  assert(mr.ref_speech == 0x15u);
  assert(mr.state.phase == 0x0Au);                 /* offside stage 0 */
  assert(fifa96_match_run_end(&mr) == 0);

  /* Negative control: with the enable off the live receive arm leaves the
   * offside machine idle (the port stays dormant). */
  fifa96_match_run_init(&mr2);
  begin_isolated(&mr2, f.engine);
  mr2.state.phase = 2;
  match_live_receive_offside_stage(&mr2);
  mr2.config.field_4c2f2 = 0u;
  assert(fifa96_match_run_ball_receive(&mr2, 3, &vec, 0x100) == FIFA96_OK);
  assert(mr2.ref_machine == FIFA96_MATCH_RUN_REF_NONE);
  assert(mr2.state.phase == 2u);
  assert(fifa96_match_run_end(&mr2) == 0);

  drop_fixture(f);
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
  assert(mr.state.phase == 0u);                             /* FU-241: phase-0 write, act 8 */
  assert(mr.act.id == 8u);
  match_frame_act8_sweep(&mr);
  assert(mr.state.phase == 3u);                             /* the deferred sit-2 arm */
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

  /* the byte-1 (Practise) session: the head queues the restart id instead
   * of writing the phase (L4) */
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
  begin_isolated(&mr, f.engine);
  assert(mr.session_gate_14c32a == 0);    /* begin seeds the settled
                                           * ordinary-match value (FU-166) */
  /* FU-166/R14: the queue -> scheduler -> post/score chain under test is the
   * native **byte-1 (Practise) session** family; the engine's ordinary-match
   * default is byte 0 (the direct goal arm). Stage the byte-1 session
   * explicitly so the ported scheduler machinery is exercised. */
  mr.session_gate_14c32a = 1;
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
  begin_isolated(&mr, f.engine);
  mr.session_gate_14c32a = 1;   /* FU-166: stage the byte-1 family under test */
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

/* FU-170 §7 (phase-14 T3): the derived leg-0 post-goal restart chain, live in
 * the engine — the hold's TAIL advance (`[0x15B688] > 0xB4`), SETUP0's sit-0xC
 * queue (`0x93C41`), the derived act-9 producer (stage-0 phase 0x15 + the
 * stage-3 timer `word[0x158818] >= 0x3C` -> `global_5882a`, the native act
 * tick `0x8B672`), the GATE open, SETUP2's sit-3 queue (`0x93CE4`: corner
 * counter + phase 4 + the 0x11 taker install) and the taker's resolve back to
 * phase 2 through the situation-0xB entry, leaving the screen machine idle at
 * its POST step. RED on the pre-T3 engine: SETUP0 clears `global_5882a` and
 * nothing re-sets it at phase 0, so the restore freezes at the GATE. */
static void test_restart_chain_fixture(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  int taker = -1;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  mr.session_gate_14c32a = 1;   /* the byte-1 family the tape stages */
  for (int i = 0; i < 61; i++) one_granted_frame(&mr);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(&mr, &kick, 1) == 0);
  }
  one_granted_frame(&mr);
  assert(fifa96_match_run_input(&mr, NULL, 0) == 0);
  for (int i = 0; i < 6 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  assert(mr.situation_pending == 0);
  one_granted_frame(&mr);

  /* the goal: the post's writer + phase-0 write; the machine stands at its
   * TAIL step, the kickoff-era `global_5882a` still set (the advance's SETUP0
   * is the only post-goal clear). */
  assert(fifa96_match_run_goal_queue(&mr, 0) == 0);
  one_granted_frame(&mr);
  assert(mr.score[0] == 1 && mr.score[1] == 0);
  assert(mr.state.phase == 0u);
  assert(mr.screen_step == 5u);          /* MATCH_RUN_SCREEN_TAIL */
  assert(mr.screen_timer == 0u);
  assert(mr.global_5882a == 1u);         /* the kickoff-era leftover */

  /* the `[0x15B688] > 0xB4` gate: 90 granted frames bring the timer to exactly
   * 0xB4 without the advance (the native `JLE`), the 91st crosses it. */
  for (int i = 0; i < 90; i++) one_granted_frame(&mr);
  assert(mr.screen_timer == 0xB4u);
  assert(mr.screen_step == 5u);
  assert(mr.state.phase == 0u);
  one_granted_frame(&mr);                /* the advance: FUN_000935A0 */
  assert(mr.screen_step == 1u);          /* SETUP0 ran; the GATE waits */
  assert(mr.screen_timer == 0u);         /* the step advance zeroes it */
  assert(mr.global_5882a == 0u);         /* the SETUP0 clear (0x93C30) */
  assert(mr.state.phase == 0x15u);       /* the derived act-9 stage 0 */
  assert(mr.act.id == 9);                /* the act slot installed (0x8890A) */
  assert(mr.act.timer == 2);             /* the same-frame act tick (0x88F51) */

  /* the stage-3 timer: 30 granted frames from the arm set the gate */
  for (int i = 0; i < 28; i++) one_granted_frame(&mr);
  assert(mr.act.id == 9 && mr.global_5882a == 0u);
  one_granted_frame(&mr);                /* the 29th tick: 2 + 58 = 0x3C */
  assert(mr.global_5882a == 1u);
  assert(mr.act.id == 0);                /* the act ends (0x890D3 clears the slot) */
  assert(mr.act.timer == 0);             /* the completion zeroes it */
  assert(mr.screen_step == 1u);          /* the GATE opens next frame */

  one_granted_frame(&mr);                /* GATE -> SETUP2 -> PHASE waits */
  assert(mr.screen_step == 3u);
  assert(mr.state.phase == 4u);          /* the sit-3 phase-4 write */
  assert(mr.corner_count[0] == 1u);      /* the 0x8ABFF corner counter */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (mr.entities.team[t].records[i].code == 0x11u) taker = (int)i;
  assert(taker >= 0);                    /* the phase-4 arm's 0x11 install */

  /* the corner taker: the stage-0 0x3C wait, the delivery ladder and the
   * situation-0xB resolve back to phase 2; the screen machine's PHASE step
   * then clears the latch and the POST idles. */
  for (int i = 0; i < 400 && mr.state.phase != 2u; i++) one_granted_frame(&mr);
  assert(mr.state.phase == 2u);
  one_granted_frame(&mr);
  assert(mr.screen_step == 4u);          /* POST (idle) */
  assert(mr.situation_pending == 0u);
  assert(mr.score[0] == 1 && mr.score[1] == 0);   /* no double score */
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-173 §4.2 / R21-2 (phase-14 T3): the byte-0 phase-5 celebration arm
 * (`0x8D433`, first-hand `read_memory 0x8D040[5]`): the scanned goal-record
 * team gets code 0x15 over records 1..10 (record 0 kept) and code 0x14 on
 * the goal record with the `[team+0x7B2]` target write; the other team gets
 * code 0 over 0..10 (`0x8D47C`). RED on the pre-T3 engine: `phase_arm(5)` is
 * a no-op. */
static void test_phase5_celebration_arm(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    mr.entities.team[0].records[i].code = 0x26u;
    mr.entities.team[1].records[i].code = 0x26u;
  }
  /* the scan's record store (the [0x157A9F] cell): team 0 record 3 */
  mr.goal_record = 3;
  mr.goal_record_side = 0;
  assert(fifa96_match_run_situation(&mr, 6) == 0);   /* table-2 row 6 */
  assert(mr.state.phase == 5u);
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[0].records[i].code == (i == 3u ? 0x14u : 0x15u));
  assert(mr.entities.team[0].records[0].code == 0x26u);  /* first = 1 */
  assert(mr.entities.team[0].target == 3);               /* [EBP+0x7B2] */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[1].records[i].code == 0u);   /* first = 0, code 0 */

  /* the goal record on the other side: team 1's block is the matching one */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.goal_record = 13;                                   /* team 1 record 2 */
  mr.goal_record_side = 1;
  assert(fifa96_match_run_situation(&mr, 6) == 0);
  assert(mr.entities.team[1].records[2].code == 0x14u);
  assert(mr.entities.team[1].records[1].code == 0x15u);
  assert(mr.entities.team[1].target == 13);
  assert(mr.entities.team[0].records[0].code == 0u);

  /* without a scanned record the native dereferences the stale cell; the
   * port's NONE hardening skips the record-keyed installs (no invention). */
  fifa96_match_run_init(&mr);
  match_frame_stage_restarts(&mr);
  mr.goal_record = FIFA96_MATCH_ENTITY_NONE;
  assert(fifa96_match_run_situation(&mr, 6) == 0);
  assert(mr.state.phase == 5u);
  assert(mr.entities.team[0].records[3].code == 0u);
}

/* FU-172 §7/§10 (phase-14 T3, Tier 1): the row-0x12 f3 aux-`0x02` airborne
 * freeze (the `0x2B` twin without the impulse; sustained) and the f0/f1 low
 * aux-`0x0A` landing tail (`0x6EEB7`: `pos_y := 0; jump_timer := 0`;
 * sustained). RED on the pre-T3 engine: both aux bytes fall into the
 * `default` no-op. */
static void test_jump_arm_f3_freeze_and_f0_landing(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);

  /* f3 aux 0x0002: airborne (10 < 2*15) -> the frame freeze, sustained */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x12, 3) == FIFA96_OK);
  e->jump_base = 15;
  e->jump_timer = 10;
  one_granted_frame(&mr);
  assert(e->aux_word == 0x0002u);     /* sustained: not cleared */
  assert(e->frame_freeze == 1u);
  /* landed (timer >= 2*base): the gate refuses, so no freeze is staged */
  e->jump_base = 15;
  e->jump_timer = 30;
  one_granted_frame(&mr);
  assert(e->frame_freeze == 0u);

  /* f0 aux 0x000A: the landing tail clears the stale arc (base zeroed so the
   * same frame's arc step cannot re-lift it) */
  assert(fifa96_match_run_record_anim_install(&mr, e, 0x12, 0) == FIFA96_OK);
  e->jump_base = 0;
  e->jump_timer = 0x30;
  e->pos_y = 0x20;
  one_granted_frame(&mr);
  assert(e->pos_y == 0);
  assert(e->jump_timer == 0);
  assert(e->aux_word == 0x000Au);     /* sustained */
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
  begin_isolated(&mr, f.engine);
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
  begin_isolated(&mr, f.engine);
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
  begin_isolated(&mr, f.engine);
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
  begin_isolated(&mr, f.engine);
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
    begin_isolated(&mr, f.engine);
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
    begin_isolated(&mr, f.engine);
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
  begin_isolated(&mr, f.engine);
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
  begin_isolated(&mr, f.engine);
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
 * side-1 kick in the byte-1 (Practise) session (staged; FU-166). */
static void test_view_pose_feed_arms_camera(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  /* FU-166/R14: the queued-arm expectation below is the byte-1 session
   * family; stage it explicitly (the engine's ordinary-match default is 0). */
  mr.session_gate_14c32a = 1;
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
  /* FU-149 P1: the corner/goal-kick arm fires (sit = 3 + cond, side =
   * ball ^ 1) and the byte-1 (Practise) session queues the restart id. The
   * ball side is the derived stand-in (the pool controlled actor, then the
   * carrier; the native `[0x1577CA]` team byte is unported). FU-171 §7 rec 1
   * (T2): the row-04 half-line admission now reads the live camera Y, so the
   * settle frame's camY-0 claims survive (team 1) and the posed camera Y
   * (320 > 0x50) closes the frame's own half-line; cond = ((+snap z) ==
   * (ball == 1)) = 0 -> sit 3 (the corner arm), side = 1 ^ 1 = 0 -> id 9. */
  assert(mr.situation_pending == 1);
  assert(mr.situation_id == 9u);
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
  begin_isolated(&mr, f.engine);
  mr.session_gate_14c32a = 1;   /* FU-166: stage the byte-1 family under test */
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
  begin_isolated(&mr, f.engine);
  mr.session_gate_14c32a = 1;   /* FU-166: stage the byte-1 family under test */
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
  e->type = 16;                  /* 0x10F334/3C offsets (21,88)<<5 seed */
  e->anim_id = 0;                      /* row byte != 0x13 -> the event arm */
  e->timer81 = 0;
  /* FU-171 §7 rec 1 (T2): the native `dword[0x157750]` the row-04 half-line
   * reads (0x7EDD8/0x7EFFA) is the CAMERA Y, not the ball record. Stage the
   * camera cell and leave the old `ball.y` stand-in at zero so this fixture
   * discriminates the re-stage (pre-re-stage the row reads `ball.y` = 0 ->
   * the slot/vel seed arm yields nothing at |camera.z| > 0xA20 and no event
   * fires). */
  mr.render.camera.pos_y = 0x20;       /* <= 0x50, != 0 -> the type-table seed */
  mr.entities.ball.y = 0;              /* the retired stand-in */
  /* M2 phase-10 T2: with the L4.1 walk + the BF20 camera-event tail live, a
   * later record's tail event could rebind the camera-tracked cell; isolate
   * the fixture to the row-04 record (native `+0x9A`). */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 1u && i == 4u)) mr.entities.team[t].records[i].skip_9a = 1;

  one_granted_frame(&mr);
  /* the row body fired the event: the camera velocity is the row's, not a
   * fixture poke (vel_z 2816/12 = 234 -> atan-capped to 25), and the event
   * bound the row's record as the camera-tracked entity (0x71D27). */
  assert(mr.render.camera.vel_x != 0 || mr.render.camera.vel_z != 0);
  assert(mr.render.camera.tracked == id);
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

/* FU-171 §7 rec 2 (T2): the row-04 far arm gates on `[0x1577CA]` — the
 * camera-tracked record — not on the controlled actor (`id == controlled`).
 * Same staged scenario as `test_row04_live_pan_arms_camera`, but the record
 * IS the camera's tracked entity, so the native 0x7EDA0 far-arm return
 * (`CMP EBP,[0x1577CA]; JZ`) suppresses the half-line event: no camera
 * velocity seed, no 0x71D27 rebind, no row coda. Before the re-stage the
 * `id == controlled` stand-in missed the tracked cell and the event fired
 * (the pre-fix RED). */
static void test_row04_tracked_gate_suppresses_half_line(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  mr.session_gate_14c32a = 1;
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
  e->pos_z = 0xB00;
  e->target_x = 0;
  e->target_z = 0xB00;
  e->type = 16;
  e->anim_id = 0;
  e->timer81 = 0;
  /* stage both candidate cells so this fixture discriminates ONLY the tracked
   * re-stage (whichever cell the row reads, the event would otherwise fire). */
  mr.render.camera.pos_y = 0x20;
  mr.entities.ball.y = 0x20;
  mr.render.camera.tracked = id;       /* the far-arm 0x7EDA0 gate */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 1u && i == 4u)) mr.entities.team[t].records[i].skip_9a = 1;

  one_granted_frame(&mr);
  assert(mr.render.camera.vel_x == 0 && mr.render.camera.vel_z == 0);
  assert(mr.render.camera.event_param == 0);
  assert(mr.render.camera.tracked == id);   /* no rebind needed */
  /* The 0x7EDA0/0x7EFEE tracked gates cover the FAR arm and the event build,
   * not the near-arm coda: the row still installs code 5 while the camera
   * event (and its tracked rebind) is suppressed. */
  assert(e->code == 5u);
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
  begin_isolated(&mr, f.engine);
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
  e->type = 16;
  e->anim_id = 0x13;                   /* arm A: event_track_reload */
  e->timer81 = 0;
  mr.entities.ball.y = 0x20;
  /* M2 phase-10 T2: isolate to the row-04 record (the walk/BF20-tail events
   * of other records would reset the camera under the fixture). */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (!(t == 1u && i == 4u)) mr.entities.team[t].records[i].skip_9a = 1;

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
 * at the goal-mouth edge and the record's slot dir. The tracked-side pick
 * landed in phase-9 T3 (the flags-zero image default is tracked 1), so the
 * side-0 goal takes the untracked arm and the FUN_000CBC4C probe posts the
 * native 0xD3 (captured; the display gate stays closed at the image sound
 * cells, so `score_display_event` stays 0). */
static void test_natural_goal_end_to_end(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  /* FU-166/R14: the end-to-end chain under test (table-1 queue id 5 ->
   * scheduler -> post -> score) is the native **byte-1 (Practise) session**
   * family; stage it explicitly (the engine's ordinary-match default is the
   * byte-0 direct arm). */
  mr.session_gate_14c32a = 1;
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
   * staged byte-1 session gate (FU-166; the front-end producer is the FU-146
   * leg 2), the leg-0 handler and a fresh score pair. */
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
  e->type = 16;
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
   * handler's post and the writer incremented the score. The tracked-side
   * pick is live (T3): the flags-zero image default is tracked 1, so side 0's
   * goal takes the untracked arm and the FUN_000CBC4C probe fires on the
   * `score[0] == 1 && score[1] < 3` gate; the probe's first result low byte
   * (0xED, the 0x112E68 limb fold) has bits 0/1 set -> the native posts 0xD3.
   * The display gate stays closed at the image sound cells (0/0), so the post
   * is captured but not dispatched. The post re-latches the pending flag
   * (0x93D46) and its phase-0 write (0x93DB2) ends the scripted screen. */
  one_granted_frame(&mr);
  assert(mr.score[0] == 1u && mr.score[1] == 0u);
  assert(mr.score_last_side == 0);
  assert(mr.score_last_event == 0xD3);
  assert(mr.score_display_event == 0u);
  assert(mr.situation_pending == 1u);
  assert(mr.state.phase == 0u);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-146 §7 item 1 / FU-149 §1.1 (T4): the queue-condition fallback arm on the
 * same natural path. With the session gate closed at scan time ([0x14C32A] ==
 * 0 — FU-166/R14: the settled ordinary-match default), the situation-6 entry
 * takes the direct arm: the score increments on the scan frame (the FU-72
 * plain increment, not the writer) and the table-2 row 6 writes phase 5; no id
 * enters the queue and the installer latch stays clear. */
static void test_natural_goal_fallback_arm(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
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
  assert(mr.session_gate_14c32a == 0u);
  /* begin installs the goal-screen machine unconditionally (the engine's
   * model), but the byte-0 session never runs its per-frame scheduler
   * (FU-166), so clear the installer latch through the machine's own
   * phase-clear step explicitly before the scan. */
  mr.global_5882a = 1;
  assert(fifa96_match_run_screen_step(&mr) == 0);
  assert(mr.situation_pending == 0u);

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
  e->type = 16;
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

/* M2 phase-9 T3 (FU-146 §3): FUN_00092D8C's tracked-side pick. The installer
 * head FUN_00078824 (0x92D92 -> 0x78824) zeroes [0x1590CC]/[0x159901] before
 * the pick, and no other writer exists (fresh get_xrefs_to 0x1590CC = 12:
 * 0x7882a the sole WRITE), so the image-default flags path always reads 0 ->
 * tracked 1. The [0x15B684] game-mode dword switches the source to the side
 * argument (its producers are the front-end FUN_00038630 arms, engine BSS 0).
 * With the pick landed the natural goal's untracked arm calls the probe post
 * (0xD3 at probe bits 0/1) instead of the carried -1's early return. */
static void test_tracked_side_pick(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(mr.score_tracked_side == -1);          /* init: no handler installed */
  begin_isolated(&mr, f.engine);
  /* begin installs leg 0 / mode 0 / side 0 -> the flags-zero image default 1 */
  assert(mr.score_tracked_side == 1);
  assert(mr.tracked_flag[0] == 0 && mr.tracked_flag[1] == 0);
  assert(mr.screen_record_mode == 0);

  /* the [0x15B684] != 0 arm: tracked = the side argument */
  mr.screen_record_mode = 1;
  assert(fifa96_match_run_screen_install(&mr, 0, 0, 1) == 0);
  assert(mr.score_tracked_side == 1);
  assert(fifa96_match_run_screen_install(&mr, 0, 0, 0) == 0);
  assert(mr.score_tracked_side == 0);

  /* the flags arm re-zeroes staged flags (the FUN_00078824 head) and picks 1 */
  mr.screen_record_mode = 0;
  mr.tracked_flag[0] = 1;
  mr.tracked_flag[1] = 1;
  assert(fifa96_match_run_screen_install(&mr, 0, 0, 1) == 0);
  assert(mr.tracked_flag[0] == 0 && mr.tracked_flag[1] == 0);
  assert(mr.score_tracked_side == 1);
  assert(fifa96_match_run_end(&mr) == 0);
  /* a staged config cell reaches the range words at begin (FUN_00011B7C ->
   * FUN_0001C9BC): cells[0] == 2 sets bit 2, the pan-step walk gate */
  {
    struct fifa96_match_run mr2;
    fifa96_match_run_init(&mr2);
    mr2.input_cfg[0] = 2;
    begin_isolated(&mr2, f.engine);
    assert(mr2.input_range[0] == 4 && mr2.input_range[1] == 0);
    assert(((mr2.input_range[0] | mr2.input_range[1]) & 4u) != 0u);
    assert(fifa96_match_run_end(&mr2) == 0);
  }
  drop_fixture(f);
}

/* M2 phase-9 T3 (FU-152 §2.9 / FU-145 §1.2): the every-frame FUN_00071DF4
 * call site (FUN_000736AC 0x73B5B..0x73B6B: `[0x1577CA] != 0` and
 * `[[rec]+0x20] != 0` -> FUN_00071DF4). The engine binds the tracked record
 * through `render.camera.tracked` (row-event wiring; FUN_00071C94 0x71D27)
 * and runs the ported first arm (`fifa96_camera_rate_event`) after the FU-71
 * integrator: with the event height > 0xF0 and a slot the velocity is the
 * slot dir bytes x15 clamped to +-15. Without a slot (or no tracked record)
 * the follow does not run, and with the height <= 0xF0 the first arm is
 * skipped (the unported table arm stays the OL-T11-79 leg). */
static void test_auto_camera_follows_tracked_slot(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *e;
  int32_t id = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 4;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  e = &mr.entities.team[1].records[4];
  assert(fifa96_match_entities_install(e, (uint8_t)mr.state.phase, 0, 0) == 1);
  e->has_slot = 1;
  e->dir_x = 1;
  e->dir_z = 0;
  assert(mr.render.camera.tracked == FIFA96_CAMERA_TRACKED_NONE);
  /* the FUN_000736AC call-site state: pan counter 0, a nonzero bearing word,
   * event_param > 0x10 and both rate bytes zero */
  mr.render.camera.pan_counter = 0;
  mr.render.camera.follow_speed = 0x19;   /* the 0x739CE [0x1577BE] value */
  mr.render.camera.event_param = 0x640;
  mr.render.camera.rate_x = 0;
  mr.render.camera.rate_z = 0;

  /* no tracked record: the 0x73B5B arm never runs */
  assert(fifa96_match_run_camera_follow(&mr) == 0);
  assert(mr.render.camera.vel_x == 0 && mr.render.camera.vel_z == 0);

  /* the call-site gates (0x739C6..0x739FD) */
  assert(fifa96_camera_set_tracked(&mr.render.camera, id) == FIFA96_OK);
  mr.render.camera.pan_counter = 1;
  assert(fifa96_match_run_camera_follow(&mr) == 0);      /* 0x739C6 */
  mr.render.camera.pan_counter = 0;
  mr.render.camera.follow_speed = 0;
  assert(fifa96_match_run_camera_follow(&mr) == 0);      /* 0x739CE */
  mr.render.camera.follow_speed = 0x19;
  mr.render.camera.event_param = 0x10;
  assert(fifa96_match_run_camera_follow(&mr) == 0);      /* 0x739DC */
  mr.render.camera.event_param = 0x640;
  mr.render.camera.rate_x = 1;
  assert(fifa96_match_run_camera_follow(&mr) == 0);      /* 0x739ED */
  mr.render.camera.rate_x = 0;

  /* tracked + no slot: the 0x73B65 `[[rec]+0x20] != 0` gate refuses */
  e->has_slot = 0;
  assert(fifa96_match_run_camera_follow(&mr) == 0);
  assert(mr.render.camera.vel_x == 0 && mr.render.camera.vel_z == 0);

  /* tracked + slot + height > 0xF0: the velocity is the sub-object dirs x15
   * clamped to +-15 and the bearing is recomputed */
  e->has_slot = 1;
  assert(fifa96_match_run_camera_follow(&mr) == 1);
  assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == 0);
  assert(mr.render.camera.speed == 15);

  /* the clamp: dir (4, -4) -> (60, -60) -> (15, -15) */
  e->dir_x = 4;
  e->dir_z = -4;
  assert(fifa96_match_run_camera_follow(&mr) == 1);
  assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == -15);

  /* the height gate at the boundary: 0xF0 is refused (native JLE 0xF0) and
   * leaves the velocity untouched, 0xF1 follows */
  e->dir_x = 1;
  e->dir_z = 0;
  mr.render.camera.event_param = 0xF0;
  assert(fifa96_match_run_camera_follow(&mr) == 0);
  assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == -15);
  mr.render.camera.event_param = 0xF1;
  assert(fifa96_match_run_camera_follow(&mr) == 1);
  assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == 0);

  /* an out-of-range tracked id is a no-op */
  assert(fifa96_camera_set_tracked(&mr.render.camera, 9999) == FIFA96_OK);
  assert(fifa96_match_run_camera_follow(&mr) == 0);
  assert(fifa96_match_run_camera_follow(NULL) == -FIFA96_ERR_INVALID);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* M2 phase-10 T1 (FU-148 §14 / FU-142 OL-88): the FUN_0007A084 receive arm
 * and its follow-cam chain. The staged block snapshot (actor/vector/traj)
 * drives `fifa96_match_run_ball_receive`: the camera event stores the
 * trajectory word as the event height (0x7A207..0x7A219), binds the actor as
 * tracked, clears the controlled actor (0x7A227), acknowledges the staged
 * event (0x7A2F0) and resolves the receiver from the camera-led target
 * (0x7A2FF..0x7A3A2). The follow arm (`fifa96_match_run_camera_follow`) then
 * requires the event height > 0xF0: with traj 0x100 the arm follows the
 * tracked slot dirs into the camera velocity; with 0x50 (the row-1E release
 * height) it refuses — that height boundary is the discriminating step vs
 * the pre-T1 state where no live producer ever sets the cell above 0x10. */
static void test_ball_receive_arm_high_ball_chain(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  struct fifa96_match_entity *actor;
  fifa96_ball_pair_vector vec;
  int32_t actor_id = 9;                 /* team 0 record 9 */
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  mr.state.phase = 2;                   /* the 0x7A2F6 phase gate passes */
  /* A quiet team block: every record far from the origin and unexcluded,
   * then one near candidate (record 7) for the receiver pick. */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &mr.entities.team[0].records[i];
    e->pos_x = 0x1000;
    e->pos_z = 0x1000;
    e->pos_y = 0;
    e->skip_98 = 0;
    e->skip_9a = 0;
    e->has_slot = 0;
    e->active = 0;
    e->code = 0;
    e->type = 0;
  }
  mr.entities.team[0].records[7].pos_x = 0;       /* near candidate */
  mr.entities.team[0].records[7].pos_z = 0x20;
  actor = &mr.entities.team[0].records[actor_id];
  actor->has_slot = 1;
  actor->dir_x = 1;
  actor->dir_z = 0;
  mr.entities.ball.pair.receiver = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.ball.pair.ack = 0;
  mr.entities.controlled = actor_id;
  mr.entities.team[1].target = 0;
  mr.entities.team[1].second = 0;
  /* the staged block snapshot the native call point sees (0x158738/3E/30):
   * the middle/z words feed the camera ramp seeds, traj the event height */
  vec.x = 0x5A0;
  vec.height = 0x400;
  vec.z = 0x200;

  assert(fifa96_match_run_ball_receive(NULL, actor_id, &vec, 0x100) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_ball_receive(&mr, actor_id, NULL, 0x100) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_ball_receive(&mr, 0x1000, &vec, 0x100) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_ball_receive(&mr, actor_id, &vec, 0x100) ==
         FIFA96_OK);
  /* The camera event: height = the trajectory word, seeds from the staged
   * vector's middle/z words, tracked bound to the actor. */
  assert((int16_t)mr.render.camera.event_param == 0x100);
  assert(mr.render.camera.tracked == actor_id);
  /* The controlled actor is released; the staged event is acknowledged. */
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.entities.ball.pair.ack == 1);
  /* The receiver pick: record 7 is nearest the camera-led target. */
  assert(mr.entities.ball.pair.receiver == 7);
  assert(mr.entities.team[0].target == 7);
  assert(mr.entities.team[0].second == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.entities.team[1].target == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.entities.team[1].second == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.entities.ball.carrier == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.entities.ball.pos_release == 0);

  /* The every-frame camera phase (next frame): the event height passes the
   * 0x739DC gate and the follow arm fires on the tracked slot dirs. */
  assert(fifa96_camera_update_walk(&mr.render.camera, &mr.rng, 2, 0, 0, 0) ==
         FIFA96_OK);
  assert((int16_t)mr.render.camera.event_param == 0x100);
  assert(fifa96_match_run_camera_follow(&mr) == 1);
  assert(mr.render.camera.vel_x == 15 && mr.render.camera.vel_z == 0);
  assert(mr.render.camera.speed == 15);

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* The discriminating boundary: a staged receive with traj <= 0xF0 (the row-1E
 * release height 0x50) fires the camera event and the call-site gate but the
 * first arm's `height > 0xF0` refuses. At BASE no path sets the cell
 * at all, so the arm's old reachable inputs never fired either; this pins the
 * T1 producer as the only live > 0xF0 source. */
static void test_ball_receive_low_ball_no_follow(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_ball_pair_vector vec;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  mr.state.phase = 2;
  mr.entities.team[0].records[9].has_slot = 1;
  mr.entities.team[0].records[9].dir_x = 1;
  mr.entities.team[0].records[9].dir_z = 0;
  vec.x = 0x5A0;
  vec.height = 0x400;
  vec.z = 0x200;
  assert(fifa96_match_run_ball_receive(&mr, 9, &vec, 0x50) == FIFA96_OK);
  assert((int16_t)mr.render.camera.event_param == 0x50);
  assert(mr.render.camera.tracked == 9);
  assert(fifa96_camera_update_walk(&mr.render.camera, &mr.rng, 2, 0, 0, 0) ==
         FIFA96_OK);
  /* the call-site gate passes (height > 0x10) but the arm refuses (the
   * ramp-derived velocity pair stays untouched) */
  {
    int16_t vx;
    int16_t vz;
    assert((int16_t)mr.render.camera.event_param > 0x10);
    assert(mr.render.camera.follow_speed != 0);
    vx = (int16_t)mr.render.camera.vel_x;
    vz = (int16_t)mr.render.camera.vel_z;
    assert(fifa96_match_run_camera_follow(&mr) == 0);
    assert((int16_t)mr.render.camera.vel_x == vx);
    assert((int16_t)mr.render.camera.vel_z == vz);
  }

  /* the phase-1 gate: the camera event/ack run, the receiver scan does not */
  mr.entities.ball.pair.receiver = 0x7AA;
  mr.entities.ball.pair.ack = 0;
  mr.state.phase = 1;
  assert(fifa96_match_run_ball_receive(&mr, 9, &vec, 0x50) == FIFA96_OK);
  assert(mr.entities.ball.pair.ack == 1);
  assert(mr.entities.ball.pair.receiver == 0x7AA);   /* untouched */
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);

  /* M2 phase-10 T1 fix round 1: the FUN_00071C94 [0x157A6C] bail (0x71C99)
   * returns 1 from the ported setter and skips the 0x71D27 tracked store,
   * but FUN_0007A084 continues — the ack, receiver scan and release run. */
  mr.state.phase = 2;
  mr.render.camera.event_suspended = 1;
  mr.render.camera.tracked = FIFA96_CAMERA_TRACKED_NONE;
  mr.entities.ball.pair.receiver = FIFA96_MATCH_ENTITY_NONE;
  mr.entities.ball.pair.ack = 0;
  mr.entities.controlled = 9;
  assert(fifa96_match_run_ball_receive(&mr, 9, &vec, 0x50) == FIFA96_OK);
  assert(mr.entities.ball.pair.ack == 1);
  assert(mr.entities.ball.pair.receiver >= 0);       /* the scan ran */
  assert(mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
  assert(mr.render.camera.tracked == FIFA96_CAMERA_TRACKED_NONE);  /* bailed */
  mr.render.camera.event_suspended = 0;

  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* The slot-skip rule (native 0x7A335..0x7A36D): `MOV EAX,dword[EBP+0x8E];
 * SAR EAX,0x18` reads the dword alias's high byte — the +0x91 action code
 * (the same idiom as dword[+0x69]>>16 = +0x6B), NOT the +0x8E facing octant.
 * When the action is 1 or 0x10/0x11/0x12, the scan skips the record at the
 * sign_extend8(+0x8D) ordinal; with record 7 skipped the next nearest
 * candidate (record 3) wins. The octant==1 case (action 0) must NOT skip. */
static void test_ball_receive_skip_rule(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_ball_pair_vector vec;
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  mr.state.phase = 2;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &mr.entities.team[0].records[i];
    e->pos_x = 0x1000;
    e->pos_z = 0x1000;
    e->pos_y = 0;
    e->skip_98 = 0;
    e->skip_9a = 0;
  }
  mr.entities.team[0].records[3].pos_x = 0x100;
  mr.entities.team[0].records[3].pos_z = 0;
  mr.entities.team[0].records[7].pos_x = 0;
  mr.entities.team[0].records[7].pos_z = 0x20;
  /* zero seeds keep the camera-velocity lead (0x7A2FF) at zero, so the
   * nearest pick sees the plain target triple */
  vec.x = 0;
  vec.height = 0;
  vec.z = 0;
  /* the octant byte (+0x8E) is NOT the skip source: type == 1 with action 0
   * keeps the nearest candidate (record 7) */
  mr.entities.team[0].records[9].type = 1;
  mr.entities.team[0].records[9].active = 7;    /* the skip ordinal */
  assert(fifa96_match_run_ball_receive(&mr, 9, &vec, 0x100) == FIFA96_OK);
  assert(mr.entities.ball.pair.receiver == 7);

  /* action == 1 skips the ordinal (the native 0x7A346 `== 1` clause) */
  mr.entities.team[0].records[9].code = 1;
  assert(fifa96_match_run_ball_receive(&mr, 9, &vec, 0x100) == FIFA96_OK);
  assert(mr.entities.ball.pair.receiver == 3);

  /* the {0x10,0x11,0x12} action set skips the same ordinal */
  mr.entities.team[0].records[9].code = 0x10;
  assert(fifa96_match_run_ball_receive(&mr, 9, &vec, 0x100) == FIFA96_OK);
  assert(mr.entities.ball.pair.receiver == 3);

  /* a carrier actor arms the release countdown (0x7A40A) */
  mr.entities.team[0].records[9].code = 0;
  mr.entities.ball.carrier = 9;
  assert(fifa96_match_run_ball_receive(&mr, 9, &vec, 0x100) == FIFA96_OK);
  assert(mr.entities.ball.pos_release == 0x14);

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
  begin_isolated(&mr, f.engine);
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

/* FU-199 §7.2 / FU-200 §1/§2 (M2 phase-18 T1): the match-begin OL-13 metadata
 * bind. With the ISO the formation id 0 `.fmt` family loads from GAMEART0 and
 * every record's `phase_meta`/`phase_meta10`/`phase_meta14` point at the
 * 352pl/352pk/freekick blobs at the native per-record strides; the first-hand
 * blob bytes are read back through the bound pointers. Skips without the ISO. */
static void test_record_meta_bind_live(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    const struct fifa96_match_entity *r0 = &mr.entities.team[t].records[0];
    assert(r0->phase_meta != NULL);
    assert(r0->phase_meta10 != NULL);
    assert(r0->phase_meta14 != NULL);
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      const struct fifa96_match_entity *ri = &mr.entities.team[t].records[i];
      assert(ri->phase_meta - r0->phase_meta == i * 0x256);
      assert(ri->phase_meta10 - r0->phase_meta10 == i * 2);
      assert(ri->phase_meta14 - r0->phase_meta14 == i * 0x256);
    }
  }
  /* The id-0 bound blobs (first-hand GAMEART0.PVI decoded entries): 352pl.fmt
   * starts `00 ae ...`, 352pk.fmt `00 00 a5 f5 ...`, freekick.fmt
   * `c0 ff c0 f5 ...` — read back through record 0's native pointers. */
  assert(mr.entities.team[0].records[0].phase_meta[0] == 0x00);
  assert(mr.entities.team[0].records[0].phase_meta[1] == (int8_t)0xAE);
  assert((uint32_t)mr.entities.team[0].records[0].phase_meta10[0] == 0xF5A50000u);
  assert((uint32_t)mr.entities.team[0].records[0].phase_meta14[0] == 0xF5C0FFC0u);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
}

/* FU-199 §7.2 / FU-205 §1 (M2 phase-18 T1; two-axis review): the bind is
 * PER-SIDE (and per-field, through the pool NULL-blob gate). A side-1 failure
 * (here an id past the 5-id table — the same per-side path an absent/short
 * `.fmt` takes) must clear only side 1's pointers; side 0's rows keep running.
 * The previous atomic all-or-nothing bind cleared *every* pointer, silently
 * disabling the whole row family. Also pins the NULL-`mr` guard. Skips without
 * the ISO. */
static void test_record_meta_bind_per_side(void) {
  struct fixture f;
  struct fifa96_match_run mr;
  if (!t1_iso_available()) return;
  f = make_fixture_iso(10000000ull);
  fifa96_match_run_init(&mr);
  begin_isolated(&mr, f.engine);
  assert(fifa96_match_run_record_meta_bind(NULL) == -FIFA96_ERR_INVALID);
  mr.entities.team[0].side = 0;
  mr.entities.team[1].side = 1;
  mr.phase_machine.side_controlled = 0;
  mr.phase_dir_default[0] = 0;
  assert(mr.entities.team[0].records[0].phase_meta != NULL);
  assert(mr.entities.team[1].records[0].phase_meta != NULL);

  /* Side 1 has an id past the 5-id table: only side 1's pointers clear. */
  mr.formation[1] = 5u;
  assert(fifa96_match_run_record_meta_bind(&mr) == 1);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *s0 = &mr.entities.team[0].records[i];
    const struct fifa96_match_entity *s1 = &mr.entities.team[1].records[i];
    assert(s0->phase_meta != NULL && s0->phase_meta10 != NULL &&
           s0->phase_meta14 != NULL);
    assert(s1->phase_meta == NULL && s1->phase_meta10 == NULL &&
           s1->phase_meta14 == NULL);
  }
  /* Side 0's gated row (phase 0x02) still runs; side 1's is a disciplined
   * no-op — each row's gate reflects exactly its own field's blob. */
  {
    struct fifa96_match_entity *e0 = &mr.entities.team[0].records[2];
    struct fifa96_match_entity *e1 = &mr.entities.team[1].records[2];
    e0->team = 0;
    e0->target_x = 1;
    e0->target_y = 2;
    e0->target_z = 3;
    e1->team = 1;
    e1->target_x = 1;
    e1->target_y = 2;
    e1->target_z = 3;
    assert(fifa96_match_dispatch_phase_record(&mr, 0x02u, e0, -1) == FIFA96_OK);
    assert(!(e0->target_x == 1 && e0->target_y == 2 && e0->target_z == 3));
    assert(fifa96_match_dispatch_phase_record(&mr, 0x02u, e1, -1) == FIFA96_OK);
    assert(e1->target_x == 1 && e1->target_y == 2 && e1->target_z == 3);
  }
  /* A valid re-bind restores both sides. */
  mr.formation[1] = 0u;
  assert(fifa96_match_run_record_meta_bind(&mr) == 1);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    assert(mr.entities.team[t].records[0].phase_meta != NULL);
  assert(fifa96_match_run_end(&mr) == 0);
  drop_fixture(f);
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
  begin_isolated(&mr, f.engine);
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
    begin_isolated(&bare, f.engine);
    assert(fifa96_match_run_translation_install(&bare, 0) == -FIFA96_ERR_STATE);
    assert(fifa96_match_run_end(&bare) == 0);
  }
  drop_fixture(f);
}

/* v22 (M2 phase-22 review, Fix 1): the staging-gate terminal seed shared by the
 * index-1 / index-2 blocks of `test_end_word_staging_gate`: the flag-clear
 * phase-0xC terminal with the sit-1 arm installed and the FU-235 dwell met;
 * `ac2` is the past-half-time index under test (the write is gated on `>= 2`). */
static void seed_phase0c_staging(struct fifa96_match_run *mr, unsigned ac2) {
  fifa96_match_run_init(mr);
  mr->state.phase = 0x0Cu;     /* the flag-clear phase-0xC terminal state */
  mr->global_157ac0 = 0u;      /* flag clear: the terminal is present */
  mr->global_157ac2 = ac2;     /* the past-half-time index under test */
  mr->act.id = 1;              /* the sit-1 arm: the re-dispatch is a no-op */
  mr->act.timer = 0x78;        /* the FU-235 dwell is met */
}

/* FU-231 §3 / FU-234-L7 (M2 phase-22 T2 fix): the frame-body end-word STAGING
 * gate. The clock-tail staging (the FUN_0008BAF0 call at `0x8B664`, right after
 * the ball machine) writes `end_word_158822` only when the consumer's
 * past-half-time index holds (`global_157ac2 >= 2u`, the `0x4B1C6 CMP
 * [0x157AC2],2` absorber) AND the FU-235 dwell holds (`act.timer` =
 * `word[0x158818] >= 0x78`). A live phase-`0xC` terminal is seeded: the act
 * slot already resolves to the sit-1 arm (act 1), so the every-frame
 * `period_setup` re-dispatch is the native latch no-op (`FUN_000888FC 0x88906`)
 * and the seeded dwell survives to the staging. (a) half-time (index 1): the
 * word must stay 0 even with the dwell met and a flag-clear terminal present —
 * the early-fire lock FU-234-L7 exists to prevent. (b) post-half-time (index 2):
 * the same state stages word 1. Mutation-RED: dropping the `global_157ac2 >= 2u`
 * term fires the word at (a). */
static void test_end_word_staging_gate(void) {
  /* (a) half-time: index 1 -> the index term blocks the write. */
  {
    struct fifa96_match_run mr;
    seed_phase0c_staging(&mr, 1u);   /* HALF-TIME: not past */
    int granted = 0;
    for (int i = 0; i < 10 && !granted; i++) {
      int rc = fifa96_match_run_frame(&mr);
      assert(rc >= 0);
      granted = rc;
    }
    assert(granted == 1);              /* the staging ran inside a granted frame */
    assert(mr.end_word_158822 == 0u);  /* the index term blocks the write */
  }
  /* (b) post-half-time: index 2 -> the flag-clear terminal stages word 1. */
  {
    struct fifa96_match_run mr;
    seed_phase0c_staging(&mr, 2u);   /* PAST half-time */
    int granted = 0;
    for (int i = 0; i < 10 && !granted; i++) {
      int rc = fifa96_match_run_frame(&mr);
      assert(rc >= 0);
      granted = rc;
    }
    assert(granted == 1);
    assert(mr.end_word_158822 == 1u);  /* the flag-clear terminal stages 1 */
  }
  /* (c) ET-over (M2 phase-22 T4b): phase 0x13 at index 4 with the dwell met
   * stages word 2 and consumes it in the same granted frame (native
   * `0x8BD8A`/`0x8BDD4` set, `0x4B29B` clear; FU-232 §3). The ET-over is a
   * transient consume, NOT a match-over: the over screen is untouched. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.period = 4u;
    mr.state.phase = 0x13u;      /* the chooser's ET-over screen */
    mr.global_157ac0 = 1u;       /* the live extra-time flag */
    mr.global_157ac2 = 4u;       /* post-ET-half-2: no re-arm (T4b bound) */
    mr.act.id = 0;               /* no act: the phase-0x13 dwell accrues */
    mr.act.timer = 0x4B1;        /* the ET-over dwell is already met (> 0x4B0) */
    int granted = 0;
    for (int i = 0; i < 10 && !granted; i++) {
      int rc = fifa96_match_run_frame(&mr);
      assert(rc >= 0);
      granted = rc;
    }
    assert(granted == 1);
    assert(mr.end_word_158822 == 0u);                    /* staged 2, consumed */
    assert(fifa96_match_run_end_word(&mr) == 2u);        /* the ET-over condition holds */
    assert(mr.state.phase == 0x13u);                     /* not re-armed to 2 */
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);  /* no over screen */
  }
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
  test_phase_drive_extra_time_tiebreak();
  test_phase_drive_half_time_selects_act1();
  test_phase_drive_period4_skip_and_w5();
  test_phase_drive_situation8_selects_act7();
  test_roll_flag_staged_zero();
  test_phase_drive_roll4_flag_differing();
  test_situation8_dispatches_phase_1d_body();
  test_phase_drive_begun_end_resets_phase();
  test_kickoff_enters_phase2_when_cutscene_isolated();
  test_situation0_kickoff_installs_code24();
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
  test_phase_1E_decompressed_pin();
  test_set_piece_phase_arm_codes();
  test_taker_armed_rows_resolve();
  test_throw_in_taker_reachable();
  test_keeper_1d_goal_kick_reachable();
  test_corner_counter_side_swap();
  test_engine_referee_contact_fk();
  test_engine_referee_restart_default_gate_split();
  test_taker_armed_referee_rows_resolve();
  test_engine_referee_contact_gates();
  test_engine_referee_foul_sequence();
  test_contact_gate_reachable_via_pad_duel();
  test_foul_taker_held_on_live_default();
  test_engine_referee_offside_chain();
  test_engine_offside_l10_reads_code_not_type();
  test_offside_reachable_from_live_receive();
  test_scan_restart_arms();
  test_goal_chain_pan_fixture();
  test_goal_consumer_chain_fixture();
  test_restart_chain_fixture();
  test_phase5_celebration_arm();
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
  test_unbound_team_target_forced_installs_row04();
  test_unbound_chase_installs_and_runs_row08();
  test_input_options_producer_sets_range_bits();
  test_record_event_tail_gate_fires_in_frame();
  test_record_event_gate_live_from_height_producer();
  test_ball_head_error_shift_mask();
  test_ball_machine_phase2_open_play_lead();
  test_held_key_moves_live_controlled_record();
  test_live_carrier_claim_and_kick();
  test_record_jump_arc_starter();
  test_record_jump_arm_aux_3a_base_literal();
  test_jump_arm_f3_freeze_and_f0_landing();
  test_record_anim_install_row44();
  test_record_row_successor_chain();
  test_record_jump_case_0f_snap_frame3();
  test_record_jump_impulsion();
  test_record_jump_sustained_and_freeze();
  test_record_jump_grounded_step();
  test_live_airborne_long_ball_seed_then_band2_follow();
  test_live_long_ball_mode40_band2_follow();
  test_live_row_0e_selector_installs_jump();
  test_live_jump_impulsion_carry_and_stop();
  test_live_pass_kick_runs_receive_arm();
  test_live_claim_pan_row04_half_line();
  test_live_ball_record_produced_in_open_play();
  test_ai_record_mover_and_lane_track();
  test_row1e_claim_reaches_pool();
  test_row1e_stage3_possession_flip();
  test_row1e_claim_anim_bridge();
  test_row1e_claim_place_pause_and_offsets();
  test_keeper_l4_release_reach();
  test_keeper_installer_reach();
  test_keeper_installer_live_permit();
  test_keeper_seam_install_full_state();
  test_keeper_seam_copy_pos_not_after_install();
  test_keeper_seam_ladder_sees_post_install_code();
  test_keeper_seam_ladder_install_full_state();
  test_keeper_interlock_survives_frame();
  test_keeper_interlock_cross_team_install();
  test_actor_lift_controlled_side();
  test_view_pose_feed_arms_camera();
  test_camera_pan_event_chain();
  test_row04_live_pan_arms_camera();
  test_row04_tracked_gate_suppresses_half_line();
  test_row04_arm_a_track_reload();
  test_natural_goal_end_to_end();
  test_natural_goal_fallback_arm();
  test_tracked_side_pick();
  test_auto_camera_follows_tracked_slot();
  test_ball_receive_arm_high_ball_chain();
  test_ball_receive_low_ball_no_follow();
  test_ball_receive_skip_rule();
  test_formation_producer();
  test_record_meta_bind_live();
  test_record_meta_bind_per_side();
  test_translation_install();
  test_end_word_staging_gate();
  puts("test_engine_match_frame OK");
  return 0;
}
