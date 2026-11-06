/* port/tests/test_engine_live_goal.c — P6 task 2 (FU-344): the live goal path.
 *
 * Exercises the live-only producer end to end on the null backend: a live
 * `begin` (phase 2, ordinary-match session gate 0), the ball staged into the
 * native goal-mouth classifier (0xB10..0xB90 in |z|, |x| < 0xD0, y <= height),
 * the P6-L1 armer, the native goal scan -> situation 6 -> direct fallback, the
 * phase-5 celebration install, the row-0x14 stage-2 release, the kickoff
 * placement (480,0,0) and the phase-2 release. Both sides are covered; the
 * away side is the regression the old `r->side != 1u` gate froze in phase 5.
 *
 * The ball is only *positioned* (x/y/z + target); no score or phase cell is
 * written. The KICK tape advances the kickoff taker after the celebration
 * release, exactly like the native input route.
 *
 * The lifecycle case scores both sides (1-1) and shortens the periods: a
 * differing regulation end arms the engine's extra-time branch (FU-213), so a
 * tied regulation end is the path to the native flag-clear phase-0x0C
 * terminal and the FULL TIME lifecycle. That is a property of the existing
 * engine, not of this test.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

#define LIVE_TAPE_LEN 40000u
#define LIVE_GOAL_CAP 200
#define LIVE_RELEASE_CAP 4000
#define LIVE_KICKOFF_CAP 2000
#define LIVE_NORESCORE_STEPS 400
#define LIVE_PERIOD_CAP 24000

/* One 100 Hz tick per engine step; the KICK pulse is the probe/recon cadence
 * (10 held ticks every 90) that the kickoff taker and the row-01 release need. */
static fifa96_platform_key live_tape[LIVE_TAPE_LEN];
static int live_tape_ready;

static void live_tape_init(void) {
  if (live_tape_ready) return;
  for (uint32_t i = 0; i < LIVE_TAPE_LEN; i++) {
    memset(&live_tape[i], 0, sizeof live_tape[i]);
    if (i % 90u < 10u) {
      live_tape[i].raw_code = FIFA96_ENGINE_KEY_KICK;
      live_tape[i].state = 1;
    }
  }
  live_tape_ready = 1;
}

struct live_fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
  struct fifa96_match_run *mr;
};

static struct live_fixture make_live_fixture(void) {
  live_tape_init();
  struct live_fixture f;
  memset(&f, 0, sizeof f);
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per step */
  pcfg.tape = live_tape;
  pcfg.tape_len = LIVE_TAPE_LEN;
  f.plat = fifa96_platform_null_create(&pcfg);
  assert(f.plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;            /* the goal path needs no render assets */
  ecfg.iso_path = NULL;
  f.engine = fifa96_engine_create(&ecfg, f.plat);
  assert(f.engine != NULL);
  assert(fifa96_engine_boot(f.engine) == 0);
  f.mr = &f.engine->match_run;
  assert(fifa96_match_run_set_live_start(f.mr, 1) == 0);
  assert(fifa96_match_run_begin(f.mr, f.engine, 0) == 0);
  /* The live bootstrap: native ordinary-match session gate (byte 0), class-1
   * play phase, no pre-match cutscene arm. */
  assert(f.mr->state.phase == 2u);
  assert(f.mr->session_gate_14c32a == 0u);
  assert(f.mr->act.id == 0u);
  return f;
}

static void drop_live_fixture(struct live_fixture *f) {
  if (f->mr->running) assert(fifa96_match_run_end(f->mr) == 0);
  fifa96_engine_destroy(f->engine);
  fifa96_platform_destroy(f->plat);
  memset(f, 0, sizeof *f);
}

/* Position-only staging: the ball triple and its target, no score/phase write
 * (the producer reads the ball, the scan reads the armed snapshot). */
static void stage_ball(struct fifa96_match_run *mr, int32_t x, int32_t y, int32_t z) {
  mr->entities.ball.x = x;
  mr->entities.ball.y = y;
  mr->entities.ball.z = z;
  mr->entities.ball.target_x = x;
  mr->entities.ball.target_y = y;
  mr->entities.ball.target_z = z;
}

static void step(struct live_fixture *f) {
  assert(f->engine->match == f->mr);
  assert(fifa96_engine_step(f->engine) == 0);
}

/* Wait for the staged ball to be consumed by the native goal chain. */
static int wait_for_goal(struct live_fixture *f, int side) {
  for (int i = 0; i < LIVE_GOAL_CAP; i++) {
    step(f);
    if (f->mr->score[(uint32_t)side] == 1u) return i + 1;
  }
  return -1;
}

static int wait_for_release(struct live_fixture *f) {
  for (int i = 0; i < LIVE_RELEASE_CAP; i++) {
    step(f);
    if (f->mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE) return i + 1;
  }
  return -1;
}

static int wait_for_phase2(struct live_fixture *f) {
  for (int i = 0; i < LIVE_KICKOFF_CAP; i++) {
    step(f);
    if (f->mr->state.phase == 2u) return i + 1;
  }
  return -1;
}

/* A side scores from the goal mouth, the celebration installs the native
 * 0x15/0x14 codes, the row-0x14 release kicks off and phase 2 resumes with the
 * score intact and no second goal. */
static void test_live_goal_side(int side) {
  struct live_fixture f = make_live_fixture();
  struct fifa96_match_run *mr = f.mr;
  const int32_t z = side ? -0xB30 : 0xB30;
  stage_ball(mr, 0, 0, z);

  int goal_step = wait_for_goal(&f, side);
  assert(goal_step > 0);
  /* The P6-L1 armer snapshot: the native y-force (0) and the ball's x/z. */
  assert(mr->goal_armed == 1u);
  assert(mr->goal_zone == 1u);
  assert(mr->goal_snap_x == 0);
  assert(mr->goal_snap_y == 0);
  assert(mr->goal_snap_z == z);
  assert(mr->score[(uint32_t)side] == 1u);
  assert(mr->score[(uint32_t)side ^ 1u] == 0u);
  assert(mr->state.phase == 5u);            /* the situation-6 phase write */
  assert(mr->goal_record_side == (uint8_t)side);
  assert(mr->goal_record >= side * (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
  assert(mr->goal_record < (side + 1) * (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
  /* The phase-5 arm installs 0x15 on the scoring side and 0x14 on the scanned
   * goal record; the non-scoring side is cleared to code 0. */
  {
    int32_t gi = mr->goal_record % (int32_t)FIFA96_MATCH_ENTITY_RECORDS;
    for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
      for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
        const struct fifa96_match_entity *e = &mr->entities.team[t].records[i];
        if ((int32_t)t == side) {
          if ((int32_t)i == gi) {
            assert(e->code == 0x14u);
          } else if (i != 0u) {
            assert(e->code == 0x15u);
          }
        } else {
          assert(e->code == 0u);
        }
      }
    }
  }

  /* The row-0x14 stage-2 release: the camera reset + kickoff placement, then
   * the KICK tape drives the phase-1 taker into live play. */
  assert(wait_for_release(&f) > 0);
  assert(mr->score[0] == (side == 0 ? 1u : 0u));
  assert(mr->score[1] == (side == 1 ? 1u : 0u));
  assert(mr->entities.ball.x == 480);
  assert(mr->entities.ball.y == 0);
  assert(mr->entities.ball.z == 0);
  /* The live follow keeps the native FU-71 height [0x157750] = 0x50 (the
   * row-04 pan-arm gate), not the old zero. The kickoff camera is (0,0,0). */
  assert(mr->render.camera.pos_x == 480);
  assert(mr->render.camera.pos_y == 0x50);
  assert(mr->render.camera.pos_z == 0);

  assert(wait_for_phase2(&f) > 0);
  assert(mr->score[0] == (side == 0 ? 1u : 0u));
  assert(mr->score[1] == (side == 1 ? 1u : 0u));
  assert(mr->goal_armed == 0u);
  assert(mr->goal_zone == 0u);

  /* No re-score: the released ball sits at the kickoff spot, the armer is
   * clear, and the score stays put. */
  for (int i = 0; i < LIVE_NORESCORE_STEPS; i++) {
    step(&f);
    assert(mr->score[0] == (side == 0 ? 1u : 0u));
    assert(mr->score[1] == (side == 1 ? 1u : 0u));
  }
  drop_live_fixture(&f);
}

/* Both sides can score through the live producer and restart; the away side is
 * the celebration-release regression (native [0x157A4C], not the record side). */
static void test_live_goal_both_sides(void) {
  test_live_goal_side(0);
  test_live_goal_side(1);
}

/* Score once per side (1-1), shorten the halves, and walk the native
 * lifecycle: half-time (phase 0x0C / period index 1), the second-half kickoff,
 * then the flag-clear regulation terminal -> OVER/POST with the score intact.
 * A 1-0/0-1 regulation end arms the engine's extra-time branch, so the tie is
 * the reachable full-time path (see the file header). */
static void test_live_goal_score_survives_periods(void) {
  struct live_fixture f = make_live_fixture();
  struct fifa96_match_run *mr = f.mr;

  stage_ball(mr, 0, 0, 0xB30);
  assert(wait_for_goal(&f, 0) > 0);
  assert(mr->score[0] == 1u && mr->score[1] == 0u);
  assert(wait_for_release(&f) > 0);
  assert(wait_for_phase2(&f) > 0);
  assert(mr->score[0] == 1u && mr->score[1] == 0u);

  /* The away-side goal after the restart: 1-1 before the shortened half ends. */
  stage_ball(mr, 0, 0, -0xB30);
  assert(wait_for_goal(&f, 1) > 0);
  assert(mr->score[0] == 1u && mr->score[1] == 1u);
  assert(mr->state.phase == 5u);
  assert(mr->goal_record_side == 1u);
  (void)fifa96_match_run_set_period(mr, 3, 3);

  /* Half-time: the first period completion writes phase 0x0C and rolls the
   * index to 1. The second goal's celebration must release first. */
  int half = -1;
  for (int i = 0; i < LIVE_PERIOD_CAP; i++) {
    step(&f);
    if (mr->state.phase == 0x0Cu && mr->global_157ac2 == 1u) { half = i + 1; break; }
  }
  assert(half > 0);
  assert(mr->score[0] == 1u && mr->score[1] == 1u);

  /* The sit-1 half-time arm and the phase-1 kickoff release the second half. */
  int second_half = -1;
  for (int i = 0; i < LIVE_PERIOD_CAP; i++) {
    step(&f);
    if (mr->state.phase == 2u && mr->global_157ac2 == 1u) {
      second_half = i + 1;
      break;
    }
  }
  assert(second_half > 0);
  assert(mr->score[0] == 1u && mr->score[1] == 1u);

  /* Full time: the tied regulation end keeps the extra-time flag clear, so the
   * phase-0x0C terminal stages end word 1 and the frame loop marks OVER; the
   * live step resolves it to the POST hold. */
  int full = -1;
  for (int i = 0; i < LIVE_PERIOD_CAP; i++) {
    step(&f);
    if (mr->lc.screen != FIFA96_MATCH_SCREEN_ACTIVE) { full = i + 1; break; }
  }
  assert(full > 0);
  assert(mr->lc.screen == FIFA96_MATCH_SCREEN_POST);
  assert(mr->score[0] == 1u && mr->score[1] == 1u);
  assert(mr->end_word_158822 == 1u);
  assert(mr->global_157ac2 == 2u);
  assert(mr->global_157ac0 == 0u);

  /* The POST hold keeps the score until the dwell expires. */
  for (int i = 0; i < 100; i++) {
    step(&f);
    assert(mr->score[0] == 1u && mr->score[1] == 1u);
  }
  drop_live_fixture(&f);
}

int main(void) {
  test_live_goal_both_sides();
  test_live_goal_score_survives_periods();
  printf("test_engine_live_goal: ok\n");
  return 0;
}
