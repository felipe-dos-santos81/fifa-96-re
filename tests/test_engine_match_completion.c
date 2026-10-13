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
 * register cancel + post-exit) returns the engine to FRONTEND. */
static void test_period_end_resolve_returns_to_frontend(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
  mr.state.phase = 2;

  for (int i = 0; i < TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.period == 1);
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
 * survives and the next run step ends the match. */
static void test_staged_exit_survives_period_end(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
  mr.state.phase = 2;

  for (int i = 0; i < TICKS_PER_SECOND - 1; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  assert(fifa96_match_lifecycle_request_exit(&mr.lc) == 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_EXIT);

  assert(fifa96_match_run_frame(&mr) >= 0);          /* the boundary tick */
  assert(mr.state.period == 1);                      /* the period still rolled */
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
  test_score_plumbing();
  puts("test_engine_match_completion OK");
  return 0;
}
