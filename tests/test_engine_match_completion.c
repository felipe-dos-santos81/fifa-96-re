/* tests/test_engine_match_completion.c — Task 3: match completion (periods, score, exit). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"

/* The 100 Hz pace grants 3 frames per 10 ticks (0x102/0x35C); 100 ticks grant
 * 30 frames, and one granted frame advances the FU-62 clock by 0x200 = 2 of
 * the 60 counter units per second. So 100 direct frame calls complete a 1 s
 * period exactly. */
#define TICKS_PER_SECOND 100

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

/* Argument contract: NULL -> INVALID everywhere; a run that is not live
 * (before begin, after end) -> STATE for set_period/resolve/add_goal. */
static void test_not_running_state_errors(void) {
  assert(fifa96_match_run_set_period(NULL, 1, 1) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_resolve(NULL) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_run_add_goal(NULL, 0) == -FIFA96_ERR_INVALID);

  struct fifa96_match_run mr;
  memset(&mr, 0xAA, sizeof mr);
  fifa96_match_run_init(&mr);
  assert(mr.score[0] == 0);                          /* init zeroes the new field */
  assert(mr.score[1] == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == -FIFA96_ERR_STATE);
  assert(fifa96_match_run_resolve(&mr) == -FIFA96_ERR_STATE);
  assert(fifa96_match_run_add_goal(&mr, 0) == -FIFA96_ERR_STATE);
  assert(mr.state.period_length == 0);               /* rejected calls mutate nothing */
  assert(fifa96_match_run_add_goal(&mr, 2) == -FIFA96_ERR_INVALID);
}

/* begin installs the FU-62 §4.6 derived default lengths: the default settings
 * half (2 minutes, FU-68 §4.1) gives 120 s / 40 s for selector 0 and the
 * FUN_0004B508 reset override 60 s / 30 s for a non-zero selector. The old
 * state-init zero length made a match end ~1 s after begin; 3 s of clock must
 * stay in period 0 on the default. */
static void test_begin_installs_derived_periods(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.running == 1);
  assert(mr.state.period_length == FIFA96_MATCH_RUN_PERIOD_SECONDS_DEFAULT);
  assert(mr.state.extra_length == FIFA96_MATCH_RUN_EXTRA_SECONDS_DEFAULT);
  mr.state.phase = 2;                    /* class 1: the clock always runs */
  for (int i = 0; i < 3 * TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.total_seconds == 3);
  assert(mr.state.period == 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  assert(fifa96_match_run_end(&mr) == 0);

  assert(fifa96_match_run_begin(&mr, f.engine, 1) == 0);
  assert(mr.state.period_length == FIFA96_MATCH_RUN_PERIOD_SECONDS_RESET);
  assert(mr.state.extra_length == FIFA96_MATCH_RUN_EXTRA_SECONDS_RESET);
  assert(fifa96_match_run_end(&mr) == 0);

  drop_fixture(f);
}

/* FU-223 (phase-21 T2, spec §5 T2 + errata E3): the live period length is the
 * match-init constant the engine already stages — there is no producer to port.
 * The native clock reads [0x15881A] (regular periods 0/1) / [0x15881C] (extra
 * periods 2/3), both = [0x14C1D1] minutes x 0x3C/0x14 (FU-223 §2.4); the writers
 * are FUN_0003749C 0x375E4/0x375ED (settings translator, FU-68), FUN_000886D4
 * 0x88710/0x88730 (match-start re-derive) and FUN_0004B508 0x4B514/0x4B51B (the
 * 60/30 reset override, gated [0x1072F8]!=0). The default half (table flat
 * 0x37170[settings[0x0E]=0] = 2 min) derives 2*0x3C = 120 s and 2*0x14 = 40 s,
 * exactly what begin installs for selector 0 (fifa96_match_run.c:2368..2372).
 * Pin the literal numerics — not just the header macros — so a silent constant
 * change cannot pass this test. */
static void test_period_length_pin(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);

  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.state.period_length == 120u);   /* [0x15881A]: [0x14C1D1] 2 min * 0x3C */
  assert(mr.state.extra_length == 40u);     /* [0x15881C]: [0x14C1D1] 2 min * 0x14 */
  assert(mr.config.period_length == 120);   /* FU-68 hand-off, same derive (unwired) */
  assert(mr.config.extra_length == 40);
  assert(fifa96_match_run_end(&mr) == 0);

  /* The native FUN_0004B508 60/30 reset override (selector != 0; FU-223 §2.2). */
  assert(fifa96_match_run_begin(&mr, f.engine, 1) == 0);
  assert(mr.state.period_length == 60u);
  assert(mr.state.extra_length == 30u);
  assert(fifa96_match_run_end(&mr) == 0);

  drop_fixture(f);
}

/* A live set_period writes both derived fields exactly; resolve while the
 * screen is still ACTIVE is a no-op (nothing to resolve yet). */
static void test_set_period_and_active_resolve(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);

  assert(fifa96_match_run_set_period(&mr, 7, 3) == 0);
  assert(mr.state.period_length == 7);
  assert(mr.state.extra_length == 3);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  assert(fifa96_match_run_resolve(&mr) == 0);        /* ACTIVE: no OVER/POST/EXIT */
  assert(mr.running == 1);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  assert(mr.lc.active == 1);
  assert(fifa96_match_run_end(&mr) == 0);

  drop_fixture(f);
}

/* Boundary completion: a 1 s period ends on the 30th granted frame (the 100th
 * pace tick); the frame body marks the lifecycle OVER, then resolve drives
 * OVER -> POST -> EXIT per FU-64 and the existing run_end path (teardown +
 * register cancel + post-exit) returns the engine to FRONTEND.
 *
 * v21 (M2 phase-21 T5, FU-228 §4.2 / FU228-L4): the exit positive is now a
 * PAST-half-time completion — the half-time completion continues. The clock is
 * seeded at period 1 so the completion rolls `global_157ac2` to 2 (>= 2) with a
 * tied score (the OL-85 flag stays clear), so the end word is decided and
 * `mark_over` fires (`fifa96_match_run.c` FU-228 §4.2 gate). */
static void test_period_end_resolve_returns_to_frontend(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
  mr.state.period = 1;
  mr.state.phase = 2;

  for (int i = 0; i < TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.period == 2);
  assert(mr.state.period_seconds == 0);
  assert(mr.state.total_seconds == 1);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);
  assert(mr.lc.active == 1);

  assert(fifa96_match_run_resolve(&mr) == 1);       /* post-exit ran */
  assert(mr.running == 0);
  assert(mr.lc.active == 0);
  assert(mr.lc.registered == 0);
  assert(mr.lc.target == FIFA96_MATCH_LIFECYCLE_RESUME);
  assert(mr.state.period_length == 0);               /* teardown cleared the clock */
  assert(mr.ticks == 0);
  assert(mr.steps == 0);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);

  /* The run is reusable after resolve. */
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_end(&mr) == 0);

  drop_fixture(f);
}

/* Ordering guard (parent final-review hazard): the frame body runs in the
 * clock advance before run_step consumes exit staging, so a period end landing
 * on the same tick as a staged request_exit must not clobber EXIT->OVER. The
 * 100th tick is the coincident boundary; the period still rolls, but EXIT
 * survives and the next run step ends the match. v21 (M2 phase-21 T5): the
 * completion is seeded PAST half-time (clock at period 1) so it is one that
 * WOULD mark_over — the guard is exercised, not vacuous (a half-time completion
 * never marks over now). */
static void test_staged_exit_survives_period_end(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
  mr.state.period = 1;
  mr.state.phase = 2;

  for (int i = 0; i < TICKS_PER_SECOND - 1; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  assert(fifa96_match_lifecycle_request_exit(&mr.lc) == 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_EXIT);

  assert(fifa96_match_run_frame(&mr) >= 0);          /* the boundary tick */
  assert(mr.state.period == 2);                      /* the period still rolled */
  assert(mr.state.total_seconds == 1);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_EXIT);  /* guard: EXIT wins over mark_over */

  assert(fifa96_match_run_step(&mr) == 1);           /* exit consumed -> run_end */
  assert(mr.running == 0);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);

  drop_fixture(f);
}

/* G1 live-loop exit: begin on the engine-owned run, force the period end the
 * same way the boundary test does (1 s period, class-1 phase so the clock
 * runs), then drive the real engine dispatch until it leaves MATCH. The 100 Hz
 * trampoline marks the lifecycle OVER during a step's clock advance and the
 * run step must drive resolve (OVER -> POST -> EXIT -> end) so the engine
 * returns to FRONTEND with no live match. Bound: 1 s period = 100 steps; 300
 * steps is three periods, so a run that never exits fails the loop. Finding 3
 * (recorded): the selector-0/phase-0 default never ends a period without the
 * Task 10 phase driver, hence the forced class-1 phase here. */
static void test_live_period_end_exits_to_frontend(void) {
  struct fixture f = make_fixture(10000000ull);
  assert(fifa96_match_run_begin(&f.engine->match_run, f.engine, 0) == 0);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(f.engine->match == &f.engine->match_run);
  assert(fifa96_match_run_set_period(&f.engine->match_run, 1, 1) == 0);
  /* v21 (M2 phase-21 T5, FU-228 §4.2): seed the clock PAST half-time so the
   * forced completion rolls `global_157ac2` to 2 and exits (a half-time
   * completion now continues instead of leaving MATCH). */
  f.engine->match_run.state.period = 1;
  f.engine->match_run.state.phase = 2;   /* class 1: the clock always runs */

  int steps = 0;
  while (f.engine->mode == FIFA96_ENGINE_MODE_MATCH && steps < 3 * TICKS_PER_SECOND) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 3 * TICKS_PER_SECOND);   /* the loop exits through the engine */
  assert(steps >= TICKS_PER_SECOND);      /* ...after the forced period end */
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);
  assert(f.engine->match_run.running == 0);

  drop_fixture(f);
}

/* Loop-backed tie-break completion (phase-19 final review, Important 1): the
 * T1 W2 tie-break raised through the REAL clock. The earlier T2/T4 producer
 * tests staged `clock_period_ended`/`state.period` by hand; this one rolls the
 * FU-62 clock for real. A 1 s period completes on the 100th granted frame, so
 * period 0 -> 1 completes first (the first-half end: no tie-break site), then
 * period 1 -> 2 completes on the next 100 — the completed index 1 is the
 * regulation end (W2 0x8B2AC), so a differing score raises the OL-85 flag
 * `global_157ac0` and the derived chooser installs the extra-time arm (phase
 * 0x13 / act 0xC), whose registered phase-0x22 slot runs stage 0. The only
 * staging is the phase-class re-arm between the two periods: the period-end
 * chooser parks the clock in the class-0 phase 0x0C, and the native kickoff
 * restart that re-arms the next period (FUN_00088860 / FUN_0004B02C) is the
 * unported FU-143 §5 / FU213-L2 leg — so this is the strongest loop-backed
 * form this harness admits (the completion and `state.period` are both produced
 * by the real `fifa96_match_run_frame` clock, not hand-set). */
static void test_loopbacked_tiebreak_completion(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
  mr.state.phase = 2;                    /* class 1: the clock runs */
  assert(fifa96_match_run_add_goal(&mr, 0) == 0);   /* the derived score writer: 1-0 */

  /* Period 0 -> 1 by the real frame/clock (100 granted frames). */
  for (int i = 0; i < TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.period == 1);
  assert(mr.state.period_seconds == 0);
  assert(mr.global_157ac0 == 0u);        /* completed index 0 is not a tie-break site */
  assert(mr.global_157ac2 == 1u);        /* the roll 0x8B58A, live */
  assert(mr.state.phase == 0x0Cu);       /* the chooser parked the class-0 phase */
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);  /* v21 T5: half-time continues */

  /* Re-arm the clock for the next period (the unported kickoff restart) and roll
   * period 1 -> 2 for real; the completed index 1 is the regulation end. */
  mr.state.phase = 2;
  for (int i = 0; i < TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.period == 2);          /* the real FU-62 roll */
  assert(mr.state.period_seconds == 0);
  assert(mr.global_157ac0 == 1u);        /* W2 0x8B2AC, raised by the real clock completion */
  assert(mr.state.phase == 0x13u);       /* the extra-time chooser arm (live flag read) */
  assert(mr.act.id == 0x0Cu);            /* act 0xC, not the no-ET arm 0xB */
  assert(mr.act.stage == 1u);            /* phase-0x22 stage 0 ran invoke-now */
  assert(mr.ref_whistle == 0x1Eu);       /* the 0x974DC(0x1E) event */
  assert(mr.global_157ac2 == 2u);        /* the roll reached the regulation-end index */
  /* v21 T5 (FU-228 §4.2 / FU228-L6): the flag-set extra-time entry does NOT end
   * the match — the end word stays 0, so `mark_over` is skipped. */
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);

  drop_fixture(f);
}

/* v21 (M2 phase-21 T5, FU-227 §5 / FU-228 §4.2): the half-time (first)
 * completion CONTINUES the match — it runs the sit-1 route live instead of
 * marking the lifecycle OVER. The sit-1 arm installs act 1 (invoke deferred)
 * on the next granted frame's top-of-call mode-0 producer (T4/FU225-L5
 * one-frame cadence), and the same frame's clock-tail act tick runs the
 * phase-0x17 body stage 0 (`fifa96_match_run_kickoff_entry`) writing the phase-1
 * kickoff. Mutation-RED: re-gating the T4 per-frame mode-0 producer (the
 * top-of-phase_drive `fifa96_match_run_period_setup(mr, 0)`,
 * fifa96_match_run.c:2597) leaves the sit-1 arm dormant -> act.id stays 0x0B and
 * state.phase stays 0x0C -> the phase == 1 assert aborts. */
static void test_half_time_completion_runs_sit1_kickoff(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
  mr.state.phase = 2;                    /* class 1: the clock runs */

  for (int i = 0; i < TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.period == 1u);         /* the half-time completion (0 -> 1) */
  assert(mr.global_157ac2 == 1u);        /* the roll 0x8B58A, half-time */
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);  /* v21 T5: no exit */

  /* The sit-1 arm + the phase-0x17 kickoff run on the next granted frame. */
  for (int i = 0; i < 6 && mr.state.phase != FIFA96_MATCH_RUN_KICKOFF_PHASE; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE);  /* == 1u */
  assert(mr.state.prev_phase == 0x0Cu);  /* the 0x0C -> 1 kickoff write */
  assert(mr.act.id == 1u);               /* the sit-1 arm installed act 1 */

  drop_fixture(f);
}

/* v21 (M2 phase-21 T5, FU-228 §4.2 / FU228-L4): the lifecycle `mark_over` fires
 * only on a PAST-half-time, end-word-decided completion — a phase-0x0C completion
 * that rolls `global_157ac2 >= 2` with the extra-time flag `global_157ac0` clear.
 * (a) a half-time completion (index 1, flag clear) CONTINUES; (b) a
 * past-half-time flag-clear completion EXITS. Mutation-RED: the raw
 * `period_ended` gate fires `mark_over` at (a) -> the ACTIVE assert aborts; a
 * missing `past_half_time` bound fires at neither. */
static void test_mark_over_only_past_half_time(void) {
  /* (a) half-time (index 1, flag clear): no exit. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.phase = 2;
    mr.state.period_length = 1;
    mr.state.extra_length = 1;
    for (int i = 0; i < TICKS_PER_SECOND; i++)
      assert(fifa96_match_run_frame(&mr) >= 0);
    assert(mr.state.period == 1u);       /* the completion rolled 0 -> 1 */
    assert(mr.global_157ac2 == 1u);      /* not past half-time */
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  }
  /* (b) past-half-time + flag clear + end decided: exit (OVER). The clock is
   * seeded at period 1 so the completion rolls the index to 2; the tied 0-0
   * score keeps the OL-85 flag clear, so the end word is decided. */
  {
    struct fifa96_match_run mr;
    fifa96_match_run_init(&mr);
    mr.state.period = 1;
    mr.state.phase = 2;
    mr.state.period_length = 1;
    mr.state.extra_length = 1;
    assert(mr.global_157ac0 == 0u);      /* tied: the flag stays clear */
    for (int i = 0; i < TICKS_PER_SECOND; i++)
      assert(fifa96_match_run_frame(&mr) >= 0);
    assert(mr.state.period == 2u);
    assert(mr.global_157ac2 == 2u);      /* past half-time */
    assert(mr.state.phase == 0x0Cu);     /* the end-word screen */
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);
  }
}

/* FU-239 §3 (M2 phase-22 T1): the extra-time half is class-1 phase 2 and must
 * clock. The period-over re-arm (native FUN_0004B02C(mode 0) -> FUN_00088860,
 * sit-8 on the extra-time period index) collapses the kickoff staging into the
 * live class-1 play phase 2; the FU-62 clock -- already faithful, `extra_length`
 * for periods 2/3 (fifa96_match_state.c:63-65) -- then completes the ET half
 * (period 2 -> 3). Mutation-RED: leaving the ET half parked in the class-0 phase
 * 0x13 (the pre-T1 park) never accrues `extra_length`, so the period-3 roll and
 * the phase == 2 assert both fail. The clock itself is untouched (the fix is the
 * re-arm, not the clock). */
static void test_extra_time_half_completes(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
  mr.state.period = 2u;                        /* the first extra-time half */
  fifa96_match_run_period_over_rearm(&mr);     /* the ET (sit-8) arm */
  assert(mr.state.phase == 2u);                /* class 1 -- the live ET phase (FU-239 §3) */
  assert(fifa96_match_state_phase_class(mr.state.phase) == 1u);
  /* The FU-62 clock: 100 granted frames == 1 s at the 0x200 step. */
  for (int i = 0; i < TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.period == 3u);               /* the ET half completed (extra_length) */
  assert(mr.state.period_seconds == 0u);
  drop_fixture(f);
}

/* Derived score plumbing: the FU-72 §2.4 per-side goal words. begin zeroes
 * both, add_goal increments one side, invalid sides are rejected without a
 * write, and teardown/begin reset the pair. The original's trigger is the
 * not-yet-ported FUN_00093944 call sites (action/phase handler cluster), so
 * only the derived increment is exposed for now. */
static void test_score_plumbing(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.score[0] == 0);
  assert(mr.score[1] == 0);

  assert(fifa96_match_run_add_goal(&mr, 0) == 0);
  assert(mr.score[0] == 1);
  assert(mr.score[1] == 0);
  assert(fifa96_match_run_add_goal(&mr, 0) == 0);
  assert(mr.score[0] == 2);
  assert(fifa96_match_run_add_goal(&mr, 1) == 0);
  assert(mr.score[0] == 2);
  assert(mr.score[1] == 1);
  assert(fifa96_match_run_add_goal(&mr, 2) == -FIFA96_ERR_INVALID);
  assert(mr.score[0] == 2);
  assert(mr.score[1] == 1);

  assert(fifa96_match_run_end(&mr) == 0);
  assert(mr.score[0] == 0);                          /* teardown reset */
  assert(mr.score[1] == 0);
  assert(fifa96_match_run_add_goal(&mr, 0) == -FIFA96_ERR_STATE);
  assert(mr.score[0] == 0);

  drop_fixture(f);
}

int main(void) {
  test_not_running_state_errors();
  test_begin_installs_derived_periods();
  test_period_length_pin();
  test_set_period_and_active_resolve();
  test_period_end_resolve_returns_to_frontend();
  test_staged_exit_survives_period_end();
  test_live_period_end_exits_to_frontend();
  test_loopbacked_tiebreak_completion();
  test_half_time_completion_runs_sit1_kickoff();
  test_extra_time_half_completes();
  test_mark_over_only_past_half_time();
  test_score_plumbing();
  puts("test_engine_match_completion OK");
  return 0;
}
