/* port/tests/test_engine_match_completion.c — Task 3: match completion (periods, score, exit).
 *
 * P8 (FU-346): the live Friendly regulation end (win and draw) reaches the
 * phase-0x0C full-time result/return instead of the port's phase-0x13 ET
 * re-arm; the non-live/competition route keeps the native flag/phase-0x13
 * behaviour. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"
#include "fifa96_loader/fifa96_match_lifecycle.h"

/* The 100 Hz pace grants 3 frames per 10 ticks (0x102/0x35C); 100 ticks grant
 * 30 frames, and one granted frame advances the FU-62 clock by 0x200 = 2 of
 * the 60 counter units per second. So 100 direct frame calls complete a 1 s
 * period exactly. */
#define TICKS_PER_SECOND 100

/* v22 (M2 phase-22 T4, FU-235 §1.2/§4): after the regulation completion the
 * phase-0xC match-over screen dwells on the end/act timer `word[0x158818]`
 * (`mr.act.timer`) and `mark_over` fires only past `0x168` (360). At the
 * 3-grants-per-10-calls pace and +2 per grant that is ~600 calls plus the
 * act-0xB entry screen; 2000 is a safe bound. */
#define DWELL_TICKS 2000

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

/* P8 (FU-346): the live Friendly regulation-end fixtures drive the shipped
 * windowed path (engine steps + the KICK cadence the half-time sit-1 kickoff
 * release needs). The tape is the same 10/90 KICK pulse the P6/P7 live tests
 * use; no score/phase is staged at the end — the score is the derived goal
 * writer and the two periods are produced by the real FU-62 clock. */
#define P8_TAPE_LEN 20000u
static fifa96_platform_key p8_tape[P8_TAPE_LEN];
static int p8_tape_ready;

static void p8_tape_init(void) {
  if (p8_tape_ready) return;
  for (uint32_t i = 0; i < P8_TAPE_LEN; i++) {
    memset(&p8_tape[i], 0, sizeof p8_tape[i]);
    if (i % 90u < 10u) {
      p8_tape[i].raw_code = FIFA96_ENGINE_KEY_KICK;
      p8_tape[i].state = 1;
    } else if (i % 300u < 3u) {
      p8_tape[i].raw_code = FIFA96_ENGINE_KEY_UP;
      p8_tape[i].state = 1;
    }
  }
  p8_tape_ready = 1;
}

static struct fixture make_p8_live_fixture(void) {
  p8_tape_init();
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;
  pcfg.tape = p8_tape;
  pcfg.tape_len = P8_TAPE_LEN;
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
  assert(fifa96_match_run_set_live_start(&f.engine->match_run, 1) == 0);
  assert(fifa96_match_run_begin(&f.engine->match_run, f.engine, 0) == 0);
  return f;
}

/* v22 (M2 phase-22 review, Fix 1): the extra-time terminal seed shared by the
 * T4/T4b ET state fixtures (`test_et_entry_dwell_rearm`,
 * `test_et_rearm_index_bound`, `test_et_over_terminal_reached`): the class-0 ET
 * phase `0x13` with the extra-time flag set and the 90 s ET periods. `period`
 * also sets the past-half-time index (`global_157ac2 == period`); callers set
 * the act slot (`act.id`/`act.timer`). */
static void seed_et_terminal(struct fifa96_match_run *mr, unsigned period) {
  fifa96_match_run_init(mr);
  mr->state.period = period;
  mr->state.period_length = 90;
  mr->state.extra_length = 90;
  mr->state.phase = 0x13u;     /* the ET-entry/over screen (the chooser's arm) */
  mr->global_157ac0 = 1u;      /* the extra-time flag (the differing reg end) */
  mr->global_157ac2 = period;
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
  /* v22 (T4): drive the phase-0xC match-over dwell to the OVER screen. */
  for (int i = 0; i < DWELL_TICKS && mr.lc.screen != FIFA96_MATCH_SCREEN_OVER; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);
  assert(mr.end_word_158822 == 1u);                 /* the staged end word */
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
  while (f.engine->mode == FIFA96_ENGINE_MODE_MATCH && steps < 12 * TICKS_PER_SECOND) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 12 * TICKS_PER_SECOND);  /* the loop exits through the engine */
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
 * by the real `fifa96_match_run_frame` clock, not hand-set).
 *
 * P8 (FU-346) scope: this is the NON-live (competition/tape) route. The live
 * windowed Friendly path is pinned by `test_live_friendly_regulation_win_full_time`
 * / `..._draw_full_time` below and deliberately does not enter phase 0x13. */
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
    /* v22 (T4): drive the phase-0xC match-over dwell out to OVER. */
    for (int i = 0; i < DWELL_TICKS && mr.lc.screen != FIFA96_MATCH_SCREEN_OVER; i++)
      assert(fifa96_match_run_frame(&mr) >= 0);
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

/* FU-231 §3/§4 (M2 phase-22 T2): the FUN_0008BAF0 end machine, ported as the
 * `[0x158822]` end word. word 1 = regulation-over (the flag-clear phase-0xC
 * terminal, 0x8BCD9); word 2 = extra-time-over (the phase-0x13/0x14 dwell past
 * the timer gate, 0x8BD8A/0x8BDD4); 0 = none. The no-ET screen 0x37F54
 * ([0x105FFC]=5) has no engine cell -> FU-231 leg. The reader is a pure
 * function of {phase, global_157ac0, act.timer}; the frame body stages the
 * cell gated on the past-half-time index and the FU-235 dwell (FU-234-L7). */
static void test_end_word_arms(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  /* regulation-over: the flag-clear phase-0xC terminal (FU-231 §3 arm 1). */
  mr.state.phase = 0x0Cu;
  mr.global_157ac0 = 0u;
  mr.global_157ac2 = 2u;
  assert(fifa96_match_run_end_word(&mr) == 1u);
  /* extra-time-over: the phase-0x13/0x14 dwell past the timer gate (FU-239 §5). */
  mr.state.phase = 0x14u;
  mr.act.timer = 0x4B1u;
  assert(fifa96_match_run_end_word(&mr) == 2u);
  /* no end word: still 0. */
  mr.state.phase = 0x02u;
  assert(fifa96_match_run_end_word(&mr) == 0u);
}

/* FU-232 §4.1/§4.3 (M2 phase-22 T3): the frame-loop over-condition,
 * single-sourced in `fifa96_match_run_should_mark_over`. Over iff the end word
 * is `1` AND the period index `>= 2`, except index `4` with the extra-time flag
 * set (`0x4B237`). Word `2` (the ET-over) is a consume (FU-232 §3), NOT a
 * match-over, so it must return 0. */
static void test_mark_over_on_end_word(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  /* FU-232 §4.3: over iff word==1 AND period>=2 AND !(period==4 && ET-flag). */
  mr.global_157ac2 = 2u;
  mr.global_157ac0 = 0u;
  mr.end_word_158822 = 1u;
  assert(fifa96_match_run_should_mark_over(&mr) == 1);   /* regulation-over, period 2/3 */
  mr.global_157ac2 = 4u;
  assert(fifa96_match_run_should_mark_over(&mr) == 1);   /* period 4, flag clear */
  mr.global_157ac0 = 1u;
  assert(fifa96_match_run_should_mark_over(&mr) == 0);   /* period 4, flag set -> no over */
  mr.global_157ac2 = 2u;
  mr.global_157ac0 = 0u;
  mr.end_word_158822 = 2u;
  assert(fifa96_match_run_should_mark_over(&mr) == 0);   /* ET-over = consume (FU-232 §3) */
  mr.end_word_158822 = 0u;
  assert(fifa96_match_run_should_mark_over(&mr) == 0);
  /* the half-time index is below the 2 gate even with the word set. */
  mr.global_157ac2 = 1u;
  mr.end_word_158822 = 1u;
  assert(fifa96_match_run_should_mark_over(&mr) == 0);
  assert(fifa96_match_run_should_mark_over(NULL) == 0);
}

/* FU-232 §3 (M2 phase-22 T3): the word-`2` ET-over is a CONSUME. The frame body
 * clears the staged word (`0x4B297 MOV word[0x158822],SI`) and writes no
 * `[0x5FFC]` screen, so the run continues. Mutation-RED: dropping the consume
 * leaves the word at 2. */
static void test_end_word_et_over_consumed(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 2u;                 /* class 1: the clock runs, no terminal */
  mr.state.period_length = 90;
  mr.global_157ac2 = 2u;               /* past half-time: would let word 1 fire */
  mr.global_157ac0 = 0u;
  mr.end_word_158822 = 2u;             /* the staged ET-over word */
  for (int i = 0; i < 10 && mr.end_word_158822 != 0u; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.end_word_158822 == 0u);                    /* the consume (FU-232 §3) */
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);  /* no screen on word 2 */
}

/* v22 (M2 phase-22 T4, FU-235 §1.2/§4): the match-over SCREEN dwell. With the
 * end word staged, `mark_over` is HELD while the phase-0xC end/act timer
 * `word[0x158818]` (`mr.act.timer`) is <= 0x168 and fires once it passes — the
 * native no-input terminal gate (`0x8BCAC CMP EAX,0x168 ; JLE tail`, i.e.
 * `> 0x168`). Mutation-RED: dropping the `mr->act.timer > 0x168` term fires the
 * OVER while the timer is still ~0x100 (the held assert aborts); a wrong phase
 * bound never fires. */
static void test_mark_over_screen_dwell(void) {
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  mr.state.phase = 0x0Cu;      /* the phase-0xC match-over screen */
  mr.state.prev_phase = 2u;
  mr.state.period = 2u;
  mr.global_157ac2 = 2u;       /* past half-time (the roll 0x8B58A) */
  mr.global_157ac0 = 0u;       /* flag clear: the regulation terminal */
  mr.end_word_158822 = 1u;     /* the end word is set (0x8BCD9) */
  mr.act.id = 0;               /* no act: the phase-0xC block accrues the timer */
  mr.act.timer = 0x100;        /* below the terminal gate: HELD */

  /* Held while the end timer is <= 0x168 (the screen is on, not OVER). */
  for (int i = 0; i < 60; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
    assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  }
  assert(mr.act.timer <= 0x168);           /* the dwell accrued, still held */
  assert(mr.end_word_158822 == 1u);        /* the word is staged, not fired */

  /* The frame that passes 0x168 fires the OVER screen. */
  for (int i = 0; i < DWELL_TICKS && mr.lc.screen != FIFA96_MATCH_SCREEN_OVER; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_OVER);
  assert(mr.act.timer > 0x168);            /* fired past the terminal gate */
}

/* v22 (M2 phase-22 T4, FU-239 §1/§3/§4): the ET-entry screen dwell + the live
 * re-arm. The chooser parks the ET entry in the class-0 phase `0x13`/`0x14`
 * and installs the act-`0xC` screen (`0x8B900`); the entry screen holds until
 * that act clears, then the period-over re-arm lands the live half in the
 * class-1 play phase `2` (the only class-1 phase with a static writer,
 * FU-239 §3). Mutation-RED: dropping the re-arm leaves phase `0x13`. */
static void test_et_entry_dwell_rearm(void) {
  struct fifa96_match_run mr;
  seed_et_terminal(&mr, 2u);
  mr.act.id = 0x0Cu;           /* the act-0xC entry screen is installed */

  /* (a) the entry screen holds while the act-0xC screen runs. */
  for (int i = 0; i < 6; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
    assert(mr.state.phase == 0x13u);
  }

  /* (b) the screen completed (the act slot cleared): the re-arm lands phase 2. */
  mr.act.id = 0;
  for (int i = 0; i < 6 && mr.state.phase == 0x13u; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.phase == 2u);            /* the live ET half (FU-239 §3) */
  assert(fifa96_match_state_phase_class(mr.state.phase) == 1u);
}

/* v22 (M2 phase-22 T4b, FU-239 / g8 §3.1): the re-arm index bound. The native
 * per-ET-half re-arm fires at the ET-play window (indices 2 and 3, `0x8B58A`);
 * at the ET-half-2 COMPLETION (index 4) the chooser parks phase `0x13`/`0x14`
 * and the native takes the post-ET terminal, NOT another re-arm. Mutation-RED:
 * the un-bounded `>= 2u` guard re-arms phase 2 here. */
static void test_et_rearm_index_bound(void) {
  struct fifa96_match_run mr;

  /* (a) index 3 (the ET-half-1 -> ET-half-2 boundary): the re-arm fires. */
  seed_et_terminal(&mr, 3u);
  mr.act.id = 0;                 /* the act-0xC screen has cleared */
  for (int i = 0; i < 20 && mr.state.phase == 0x13u; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.phase == 2u);  /* the live ET half (FU-239 §3) */

  /* (b) index 4 (the ET-half-2 completion): NO re-arm — the phase-0x13/0x14
   * ET-over screen persists (native g8 §3.1: index 4 is the post-ET terminal). */
  seed_et_terminal(&mr, 4u);
  mr.act.id = 0;
  for (int i = 0; i < 20; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
    assert(mr.state.phase == 0x13u);   /* NOT re-armed to the play phase 2 */
  }
}

/* v22 (M2 phase-22 T4b, FU-231 §3 / FU-232 §3 / g2 §1 / g4 §2b): the ET-over
 * terminal is live-reachable. At the ET-half-2 completion (index 4) the bounded
 * re-arm leaves the chooser-written phase `0x13` in place; the end machine then
 * accrues the dwell timer `word[0x158818]` (`mr.act.timer`, native `0x8BCFE`,
 * gated on no act) past `0x4B0`, so `fifa96_match_run_end_word` returns `2`; the
 * frame body stages word `2` and the ET-over consume (`0x4B29B`) clears it —
 * the native transient one-frame consume (FU-232 §3, FU-232-L6 closed by g3).
 * Mutation-RED: (a) the un-bounded guard re-arms phase 2; (b) the dwell accrues
 * only in phase `0x0C`, so `act.timer` caps below `0x4B0` and the word never
 * stages. */
static void test_et_over_terminal_reached(void) {
  struct fifa96_match_run mr;
  seed_et_terminal(&mr, 4u);
  mr.act.id = 0;                 /* the act-0xC entry screen has cleared */
  mr.act.timer = 0;

  /* Drive the frame loop until the ET-over dwell passes 0x4B0 (native). */
  int i = 0;
  for (; i < 3000; i++) {
    assert(fifa96_match_run_frame(&mr) >= 0);
    if (mr.act.timer > 0x4B0) break;
  }
  assert(i < 3000);
  /* the run reached the ET-over terminal, not the period-5 / phase-2 hold. */
  assert(mr.act.timer > 0x4B0);                 /* the dwell accrued (0x8BCFE) */
  assert(mr.state.phase == 0x13u);              /* NOT re-armed to phase 2 */
  assert(fifa96_match_run_end_word(&mr) == 2u); /* the ET-over condition is live */
  assert(mr.end_word_158822 == 0u);             /* staged 2, consumed (0x4B29B) */

  /* The ET-over is self-terminating and writes no screen (FU-232-L6): the loop
   * resumes with the over screen untouched. */
  for (int j = 0; j < 10; j++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_ACTIVE);
  assert(mr.end_word_158822 == 0u);
}

/* FU-240 §2/§4 (M2 phase-23 T6, FU240-L2; WIRED phase-51 T1/FU-340): the
 * phase-`0x13` positive — the row 0x13 slot is live and the shared write runs
 * the per-record `fifa96_match_phase_13` body for both teams. The ET chooser
 * (`test_loopbacked_tiebreak_completion`'s route) reaches
 * `match_run_write_phase(0x13)` on the golden tail, so the wire injects the
 * native 4-draws-per-active-record consumption ahead of the tail frames that
 * also read the shared stream. The assertions:
 *   (a) the live chooser write still lands phase `0x13` (act 0xC);
 *   (b) `fifa96_match_phase_table[0x13].fn == fifa96_match_phase_13`, and a
 *       forced `write_phase(0x13)` advances `mr.rng` by exactly `4 × active`
 *       with the producer's draw-free `ac2 >= 4` branch;
 *   (c) the tape-shaped `ac2 < 4` write adds the producer's own four draws per
 *       team (`0x8D693..0x8D722`, `write_phase` walk -> scatter per team);
 *   (d) phase `0x14` stays NULL — no live writer — and dispatches UNSUPPORTED.
 * Mutation-RED: NULLing the slot gates the walk off (the `write_phase` walk is
 * gated on `fn != NULL`), so the (b)/(c) draw counts drop to zero. A future
 * phase-`0x14` writer must also wire the slot, not just feed the seam
 * (FU-340 §6 erratum). */
static uint32_t rng_draws_between(struct fifa96_rng from, struct fifa96_rng to) {
  struct fifa96_rng probe = from;
  uint16_t value = 0;
  uint32_t n = 0;
  while (memcmp(&probe, &to, sizeof probe) != 0) {
    assert(n < 4096u);                   /* the generator's period is far larger */
    assert(fifa96_rng_step(&probe, &value) == FIFA96_OK);
    n++;
  }
  return n;
}

static void test_phase_13_14_rng_gate(void) {
  struct fifa96_match_run mr;
  struct fifa96_rng before;
  uint32_t active = 0;
  uint32_t draws;

  /* (a) The live ET chooser write (the same route the golden tail reaches):
   * the loopbacked preamble mirrors test_loopbacked_tiebreak_completion — a 1 s
   * period rolls 0 -> 1, then 1 -> 2, and the regulation-end completion writes
   * phase 0x13 through `match_run_write_phase`. */
  {
    struct fixture f = make_fixture(10000000ull);
    assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
    assert(fifa96_match_run_set_period(&mr, 1, 1) == 0);
    mr.state.phase = 2;                                /* class 1: the clock runs */
    assert(fifa96_match_run_add_goal(&mr, 0) == 0);    /* differing score: 1-0 */
    for (int i = 0; i < TICKS_PER_SECOND; i++)
      assert(fifa96_match_run_frame(&mr) >= 0);
    mr.state.phase = 2;
    for (int i = 0; i < TICKS_PER_SECOND; i++)
      assert(fifa96_match_run_frame(&mr) >= 0);
    assert(mr.state.period == 2u);
    assert(mr.global_157ac0 == 1u);                    /* the live extra-time flag */
    assert(mr.state.phase == 0x13u);                   /* the chooser's phase-0x13 write */
    assert(mr.act.id == 0x0Cu);                        /* the ET-set act */

    /* (b) The positive: row 0x13 is wired to the ported body; row 0x14 stays
     * NULL (no live writer) and dispatches UNSUPPORTED. */
    assert(fifa96_match_phase_table[0x13].fn == fifa96_match_phase_13);
    assert(fifa96_match_phase_table[0x14].fn == NULL);
    assert(fifa96_match_dispatch_phase(&mr, 0x14u) == -FIFA96_ERR_UNSUPPORTED);

    /* Count the records the body will draw for: side-matched, active and not
     * `[rec+0x9A]`-skipped (the walk's exclusion). */
    mr.phase_machine.side_controlled = 0;
    for (uint32_t team = 0; team < FIFA96_MATCH_ENTITY_TEAMS; team++) {
      for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
        const struct fifa96_match_entity *e = &mr.entities.team[team].records[i];
        if (e->skip_9a != 0 || e->active == 0u) continue;
        if (e->team >= FIFA96_MATCH_ENTITY_TEAMS) continue;
        if (mr.entities.team[e->team].side !=
            mr.phase_machine.side_controlled)
          continue;
        active++;
      }
    }
    assert(active > 0u);   /* records 1..10 active on the ET frame (FU-147 S1) */

    /* (b) The producer's `ac2 >= 4` arm draws nothing, so one forced write is
     * exactly the body's four draws per served record. */
    mr.global_157ac2 = 4u;
    before = mr.rng;
    assert(fifa96_match_run_write_phase(&mr, 0x13u) == FIFA96_OK);
    draws = rng_draws_between(before, mr.rng);
    assert(draws == 4u * active);

    /* (c) The tape-shaped `ac2 < 4` write adds the producer's magnitude/sign
     * pairs: four draws for each of the two per-team scatter calls. */
    mr.global_157ac2 = 1u;
    before = mr.rng;
    assert(fifa96_match_run_write_phase(&mr, 0x13u) == FIFA96_OK);
    draws = rng_draws_between(before, mr.rng);
    assert(draws == 4u * active + 8u);

    drop_fixture(f);
  }
}

/* FU-343 (P4): the live pause is the native pace hold (no granted frames ->
 * the frame body and clock stop); resume releases it. Non-live runs refuse the
 * toggle, keeping the M2 tape and the direct-begin fixtures byte-static. */
static void test_live_pause_holds_pace(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_set_live_start(&mr, 1) == 0);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(mr.paused == 0 && mr.pace.hold == 0);

  assert(fifa96_match_run_set_paused(&mr, 1) == 0);
  assert(mr.paused == 1 && mr.pace.hold == 1);
  uint32_t seconds = mr.state.total_seconds;
  for (int i = 0; i < 300; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.total_seconds == seconds);      /* the clock did not advance */

  assert(fifa96_match_run_set_paused(&mr, 0) == 0);
  assert(mr.paused == 0 && mr.pace.hold == 0);
  for (int i = 0; i < TICKS_PER_SECOND; i++)
    assert(fifa96_match_run_frame(&mr) >= 0);
  assert(mr.state.total_seconds == seconds + 1);

  assert(fifa96_match_run_end(&mr) >= 0);
  drop_fixture(f);

  /* Non-live gating: both P4 controls refuse and mutate nothing. */
  struct fixture f2 = make_fixture(10000000ull);
  struct fifa96_match_run plain;
  fifa96_match_run_init(&plain);
  assert(fifa96_match_run_begin(&plain, f2.engine, 0) == 0);
  assert(fifa96_match_run_set_paused(&plain, 1) == -FIFA96_ERR_UNSUPPORTED);
  assert(plain.paused == 0 && plain.pace.hold == 0);
  assert(fifa96_match_run_result_continue(&plain) == -FIFA96_ERR_UNSUPPORTED);
  assert(plain.post_skip == 0);
  assert(fifa96_match_run_end(&plain) >= 0);
  drop_fixture(f2);
}

/* FU-343 (P4): a live OVER holds the POST screen for the dwell with the pace
 * held, then exits through the existing run_end path; result_continue skips
 * the remaining dwell. */
static void test_live_post_dwell_and_skip(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_set_live_start(&mr, 1) == 0);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);

  mr.lc.screen = FIFA96_MATCH_SCREEN_OVER;
  assert(fifa96_match_run_step(&mr) == 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_POST);
  assert(mr.pace.hold == 1);
  assert(mr.post_timer == FIFA96_MATCH_RUN_POST_DWELL_TICKS);
  assert(mr.running == 1);

  /* The held POST screen is presented across steps (the render panel gate). */
  assert(fifa96_match_run_step(&mr) == 0);
  assert(mr.lc.screen == FIFA96_MATCH_SCREEN_POST);
  assert(mr.post_timer == FIFA96_MATCH_RUN_POST_DWELL_TICKS - 1);
  assert(mr.running == 1);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_MATCH);

  /* ENTER/ESC skip: the next step requests the exit and ends the run. */
  assert(fifa96_match_run_result_continue(&mr) == 0);
  assert(mr.post_skip == 1);
  assert(fifa96_match_run_step(&mr) >= 0);
  assert(mr.running == 0);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);
  assert(mr.paused == 0 && mr.post_timer == 0 && mr.post_skip == 0);
  drop_fixture(f);

  /* The dwell itself expires: DWELL-1 more decrements reach 0 and end. */
  struct fixture f2 = make_fixture(10000000ull);
  struct fifa96_match_run mr2;
  fifa96_match_run_init(&mr2);
  assert(fifa96_match_run_set_live_start(&mr2, 1) == 0);
  assert(fifa96_match_run_begin(&mr2, f2.engine, 0) == 0);
  mr2.lc.screen = FIFA96_MATCH_SCREEN_OVER;
  assert(fifa96_match_run_step(&mr2) == 0);
  for (uint32_t i = 1; i < FIFA96_MATCH_RUN_POST_DWELL_TICKS; i++) {
    assert(mr2.running == 1);
    assert(fifa96_match_run_step(&mr2) == 0);
    assert(mr2.post_timer == FIFA96_MATCH_RUN_POST_DWELL_TICKS - i);
  }
  assert(mr2.running == 1 && mr2.post_timer == 1);
  assert(fifa96_match_run_step(&mr2) >= 0);
  assert(mr2.running == 0);
  assert(f2.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f2.engine->match == NULL);
  drop_fixture(f2);
}

/* P8 (FU-346): a live Friendly regulation end reaches the full-time result, not
 * the extra-time arm. Fresh native evidence (see the phase_drive comment):
 * `[0x157AC0]` is raised on a differing regulation score (W2 0x8B2AC), but the
 * flag's phase-0x13 arm is the class-0 post-match stats/result presentation
 * (phase-0x13 installs the row-0x25 stats sequence when the score differs,
 * 0x8D76C/0x8D772; the end machine holds a decisive screen at 0x8BDA2), not a
 * clocked ET half. Friendly is Game-Select mode 0 (`FUN_00024B00 0x24BEB XOR
 * EAX,EAX` -> `FUN_0001B7B8`), so `FUN_0003749C` leaves the `[0x14C2EE]`
 * force-ET override 0; the competition/tie ET route stays separate. The
 * native phase-0x13 stats screen body is unported (P8-L1), so the live Friendly
 * winner uses the existing phase-0x0C result terminal (the same terminal a
 * Friendly draw uses). This test drives the real clock through half-time and
 * the second half: 1-0 in, phase 0x0C / act 0x0B / flag 0 out, end word 1,
 * OVER -> POST -> EXIT -> front-end, score intact. */
static void test_live_friendly_regulation_win_full_time(void) {
  struct fixture f = make_p8_live_fixture();
  struct fifa96_match_run *mr = &f.engine->match_run;
  assert(mr->live_start == 1u);
  assert(fifa96_match_run_set_period(mr, 1, 1) == 0);
  mr->state.phase = 2;                         /* class 1: the clock runs */
  assert(fifa96_match_run_add_goal(mr, 0) == 0);   /* the derived 1-0 score */

  /* The first completion (half-time) parks phase 0x0C and rolls index 1. */
  int steps = 0;
  while (mr->state.period != 1u && steps < 6000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 6000);
  assert(mr->state.phase == 0x0Cu);
  assert(mr->global_157ac2 == 1u);
  assert(mr->score[0] == 1u && mr->score[1] == 0u);

  /* The live sit-1 restart releases the second half. */
  steps = 0;
  while (mr->state.phase != 2u && steps < 6000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 6000);
  assert(mr->score[0] == 1u && mr->score[1] == 0u);

  /* The real second-half clock: regulation end. */
  steps = 0;
  while (mr->state.period != 2u && steps < 6000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 6000);
  assert(mr->state.phase == 0x0Cu);            /* not the ET phase 0x13/0x14 */
  assert(mr->global_157ac0 == 0u);             /* no ET flag on the live Friendly arm */
  assert(mr->act.id == 0x0Bu);                 /* the no-ET period-end act */
  assert(mr->global_157ac2 == 2u);             /* the roll 0x8B58A */
  assert(mr->score[0] == 1u && mr->score[1] == 0u);

  /* The phase-0x0C end machine stages word 1 after the act/dwell gates. */
  steps = 0;
  while (mr->end_word_158822 != 1u && steps < 3000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 3000);
  assert(fifa96_match_run_should_mark_over(mr) == 1);

  /* OVER -> POST (the FULL TIME panel dwell), then the skip returns. The
   * engine step consumes OVER into POST in the same step, so the observable
   * terminal is POST with the result panel holding. */
  steps = 0;
  while (mr->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE && steps < 3000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 3000);
  assert(mr->lc.screen == FIFA96_MATCH_SCREEN_POST);
  assert(mr->score[0] == 1u && mr->score[1] == 0u);
  assert(fifa96_match_run_result_continue(mr) == 0);
  for (int i = 0; i < 8 && mr->running; i++)
    assert(fifa96_engine_step(f.engine) == 0);
  assert(mr->running == 0u);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);
  drop_fixture(f);
}

/* P8 (FU-346): the Friendly draw is a full-time result too. Fresh native:
 * a tied regulation end takes W2's equal-score else-arm, and for mode 0
 * `[0x14C1D0]==0` sets the `[0x10F378]` no-extra-time latch (0x8B2DA); the
 * phase-0x0C end machine then calls `0x37F54` -> `[0x105FFC]=5`
 * (0x8BC80..0x8BC97), the no-ET terminal. The engine has no screen-5 cell
 * (FU231-L2), so the existing phase-0x0C result terminal stands in. The score
 * is 0-0 in the shipped path (no staging). */
static void test_live_friendly_regulation_draw_full_time(void) {
  struct fixture f = make_p8_live_fixture();
  struct fifa96_match_run *mr = &f.engine->match_run;
  assert(fifa96_match_run_set_period(mr, 1, 1) == 0);
  mr->state.phase = 2;
  assert(mr->score[0] == 0u && mr->score[1] == 0u);

  int steps = 0;
  while (mr->state.period != 1u && steps < 6000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 6000);
  steps = 0;
  while (mr->state.phase != 2u && steps < 6000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 6000);
  steps = 0;
  while (mr->state.period != 2u && steps < 6000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 6000);
  assert(mr->state.phase == 0x0Cu);            /* the no-ET phase-0x0C terminal */
  assert(mr->global_157ac0 == 0u);             /* W2 equal-score arm: flag clear */
  assert(mr->act.id == 0x0Bu);
  assert(mr->global_157ac2 == 2u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);

  steps = 0;
  while (mr->lc.screen == FIFA96_MATCH_SCREEN_ACTIVE && steps < 4000) {
    assert(fifa96_engine_step(f.engine) == 0);
    steps++;
  }
  assert(steps < 4000);
  assert(mr->end_word_158822 == 1u);           /* the regulation-over word */
  assert(mr->lc.screen == FIFA96_MATCH_SCREEN_POST);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(fifa96_match_run_result_continue(mr) == 0);
  for (int i = 0; i < 8 && mr->running; i++)
    assert(fifa96_engine_step(f.engine) == 0);
  assert(mr->running == 0u);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);
  drop_fixture(f);
}

/* v22 (M2 phase-22 T4, FU-234 §4.2/§4.3): the run's return to the front-end.
 * `fifa96_match_run_end` maps the native leave/teardown
 * (`[0x105FFC]=4`/`[0x107304]=3`) to the mode switch
 * (`fifa96_match_run.c:4241`). */
static void test_run_end_lands_frontend(void) {
  struct fixture f = make_fixture(10000000ull);
  struct fifa96_match_run mr;
  fifa96_match_run_init(&mr);
  assert(fifa96_match_run_begin(&mr, f.engine, 0) == 0);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_MATCH);
  assert(fifa96_match_run_end(&mr) == 0);
  assert(f.engine->mode == FIFA96_ENGINE_MODE_FRONTEND);
  assert(f.engine->match == NULL);
  assert(mr.running == 0);
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
  test_end_word_arms();
  test_mark_over_on_end_word();
  test_end_word_et_over_consumed();
  test_mark_over_screen_dwell();
  test_et_entry_dwell_rearm();
  test_et_rearm_index_bound();
  test_et_over_terminal_reached();
  test_phase_13_14_rng_gate();
  test_live_pause_holds_pace();
  test_live_post_dwell_and_skip();
  test_live_friendly_regulation_win_full_time();
  test_live_friendly_regulation_draw_full_time();
  test_run_end_lands_frontend();
  test_mark_over_only_past_half_time();
  test_score_plumbing();
  puts("test_engine_match_completion OK");
  return 0;
}
