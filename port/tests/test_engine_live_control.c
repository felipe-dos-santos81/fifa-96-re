/* port/tests/test_engine_live_control.c — P7 / FU-345: live kickoff control.
 *
 * The live windowed bootstrap (`fifa96_match_run_begin`'s `live_start` block)
 * skips the phase-1 kickoff wait that drives row 01's actor publish, the row-02
 * slot merge and row 04's carrier install, so the human FU-70 slot had no
 * carrier and the kick release row never fired. These tests pin the derived
 * live-control producer:
 *
 *  - live begin seeds the controlled side's nearest outfield taker as the slot
 *    owner / `entities.controlled` / team target / camera tracked record and
 *    installs carrier state 5 through the real installer;
 *  - KICK press then release through `fifa96_match_run_input` dispatches action
 *    row 07 and moves the ball, with no score write;
 *  - the per-frame follow selects the carrier, else the nearest own outfield
 *    record, never the keeper and never an opponent record;
 *  - the existing native goal/restart chain still consumes a staged goal-mouth
 *    ball after the kick path ran.
 *
 * Everything here is live-only (the M1/M2 headless tapes never set live_start).
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

#define LC_TAPE_LEN 4000u
#define LC_GOAL_CAP 200
#define LC_RESTART_CAP 4000

static fifa96_platform_key lc_tape[LC_TAPE_LEN];

struct lc_fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
  struct fifa96_match_run *mr;
};

static struct lc_fixture make_lc_fixture(void) {
  memset(lc_tape, 0, sizeof lc_tape);
  struct lc_fixture f;
  memset(&f, 0, sizeof f);
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;   /* one 100 Hz PIT tick per step */
  pcfg.tape = lc_tape;
  pcfg.tape_len = LC_TAPE_LEN;
  f.plat = fifa96_platform_null_create(&pcfg);
  assert(f.plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  ecfg.iso_path = NULL;
  f.engine = fifa96_engine_create(&ecfg, f.plat);
  assert(f.engine != NULL);
  assert(fifa96_engine_boot(f.engine) == 0);
  f.mr = &f.engine->match_run;
  assert(fifa96_match_run_set_live_start(f.mr, 1) == 0);
  assert(fifa96_match_run_begin(f.mr, f.engine, 0) == 0);
  assert(f.mr->state.phase == 2u);
  assert(f.mr->session_gate_14c32a == 0u);
  return f;
}

static void drop_lc_fixture(struct lc_fixture *f) {
  if (f->mr->running) assert(fifa96_match_run_end(f->mr) == 0);
  fifa96_engine_destroy(f->engine);
  fifa96_platform_destroy(f->plat);
  memset(f, 0, sizeof *f);
}

/* One granted 30 Hz frame body (the FU-62 pace grants one per ~3 ticks). */
static void one_granted_frame(struct fifa96_match_run *mr) {
  for (int i = 0; i < 16; i++) {
    int rc = fifa96_match_run_frame(mr);
    assert(rc >= 0);
    if (rc == 1) return;
  }
  assert(!"no granted frame within 16 ticks");
}

static void stage_ball(struct fifa96_match_run *mr, int32_t x, int32_t y, int32_t z) {
  mr->entities.ball.x = x;
  mr->entities.ball.y = y;
  mr->entities.ball.z = z;
  mr->entities.ball.target_x = x;
  mr->entities.ball.target_y = y;
  mr->entities.ball.target_z = z;
}

/* The live begin seed: nearest controlled-side outfield record to the ball is
 * the slot owner, the pool controlled actor, the team target and the camera
 * tracked record, carrying state 5 through the real installer. */
static void test_live_begin_seeds_kickoff_carrier(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t id = mr->entities.controlled;
  assert(id != FIFA96_MATCH_ENTITY_NONE);
  assert(id >= 0 && id < 11);
  assert(id != 0);                       /* the keeper is never the taker */
  assert(mr->slot.entity == id);
  assert(mr->entities.team[0].side == 0u);
  assert(mr->entities.team[0].target == id);
  assert(mr->entities.team[0].records[id].has_slot == 1u);
  assert(mr->entities.team[0].records[id].code == 5u);
  assert((mr->entities.team[0].records[id].carrier & 1u) == 1u);
  assert(mr->entities.ball.carrier == id);
  assert(mr->render.camera.tracked == id);
  assert(mr->live_kickoff_touch == 1u);
  assert(mr->live_control_taker == id);
  /* The seed is the nearest outfield record to the kickoff ball: re-run the
   * native select shape and compare. */
  {
    int index = fifa96_match_entities_team_select(&mr->entities, 0,
                                                  (int16_t)mr->entities.ball.x,
                                                  (int16_t)mr->entities.ball.z);
    assert(index >= 0);
    assert(index == id);
  }
  drop_lc_fixture(&f);
}

/* KICK press then release through the public input path runs row 07 on the
 * seeded carrier; the ball moves and no score is written. */
static void test_live_kick_release_runs_row07(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  const int32_t taker = mr->entities.controlled;
  int32_t bx0, bz0;
  int16_t vx0, vz0;
  assert(taker >= 0);
  /* A couple of live frames so the frame chain has run once. */
  one_granted_frame(mr);
  assert(mr->entities.team[0].records[taker].code == 5u);
  bx0 = mr->entities.ball.x;
  bz0 = mr->entities.ball.z;
  vx0 = mr->entities.ball.vel_x;
  vz0 = mr->entities.ball.vel_z;
  assert((mr->dispatched_ok & (1ull << 7u)) == 0ull);

  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
  }
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  /* The FU-73 pairing latched the kick actor and the live FU-70 release word. */
  assert(mr->entities.ball.pair.actor == mr->live_control_taker ||
         mr->entities.ball.pair.actor == taker);
  assert((mr->entities.ball.pair.flags & 0x10u) != 0u);
  assert(mr->entities.ball.x != bx0 || mr->entities.ball.z != bz0 ||
         mr->entities.ball.vel_x != vx0 || mr->entities.ball.vel_z != vz0);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);   /* no fabricated score */
  assert(mr->live_kickoff_touch == 0u);               /* the kick released it */
  drop_lc_fixture(&f);
}

/* The per-frame live follow: the carrier wins; otherwise the nearest own
 * outfield record. The keeper and opponent records are never selected. */
static void test_live_auto_switch_carrier_and_nearest(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  /* Run the opening kick so the kickoff latch is released. */
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
    one_granted_frame(mr);
    assert(fifa96_match_run_input(mr, NULL, 0) == 0);
    for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
      one_granted_frame(mr);
  }
  assert(mr->live_kickoff_touch == 0u);
  /* Let the post-kick possession hold expire so the loose-ball claim is live. */
  for (int i = 0; i < 40; i++) one_granted_frame(mr);
  assert(mr->live_kick_hold == 0u);

  /* Carrier priority: record 3 owns the possession cell and sits on the ball. */
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    mr->entities.team[0].records[i].pos_x = 0x600;
    mr->entities.team[0].records[i].pos_z = 0x600;
  }
  mr->entities.team[0].records[3].pos_x = 0x180;
  mr->entities.team[0].records[3].pos_z = 0x40;
  stage_ball(mr, 0x180, 0, 0x40);
  mr->entities.ball.carrier = 3;
  {
    struct fifa96_match_entity *e = &mr->entities.team[0].records[3];
    (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
  }
  one_granted_frame(mr);
  assert(mr->entities.controlled == 3);
  assert(mr->slot.entity == 3);
  assert(mr->entities.team[0].target == 3);
  assert(mr->entities.team[0].records[3].has_slot == 1u);
  assert(mr->render.camera.tracked == 3);

  /* An opponent possession cell must be ignored: the own nearest record 5 is
   * selected instead and the opponent record is never made a slot owner. */
  {
    struct fifa96_match_entity *opp = &mr->entities.team[1].records[1];
    uint8_t opp_code = opp->code;
    assert(opp->has_slot == 0u);
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      mr->entities.team[0].records[i].pos_x = 0x700;
      mr->entities.team[0].records[i].pos_z = 0x700;
    }
    mr->entities.team[0].records[5].pos_x = 0x220;
    mr->entities.team[0].records[5].pos_z = 0x20;
    stage_ball(mr, 0x220, 0, 0x20);
    mr->entities.ball.carrier = 12;   /* the opponent's record 1 */
    one_granted_frame(mr);
    assert(mr->entities.controlled == 5);
    assert(mr->slot.entity == 5);
    assert(mr->entities.team[0].target == 5);
    assert(mr->render.camera.tracked == 5);
    assert(mr->entities.team[0].records[5].has_slot == 1u);
    assert(mr->entities.ball.carrier == 5);   /* cleared the opponent latch */
    assert(opp->has_slot == 0u);              /* opponent record untouched */
    (void)opp_code;
  }

  /* The keeper is never selected even when it is the closest record to the
   * ball and the possession cell is empty. */
  {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      mr->entities.team[0].records[i].pos_x = 0x900;
      mr->entities.team[0].records[i].pos_z = 0x900;
    }
    mr->entities.team[0].records[0].pos_x = 0x240;
    mr->entities.team[0].records[0].pos_z = 0;
    stage_ball(mr, 0x240, 0, 0);
    mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
    one_granted_frame(mr);
    assert(mr->entities.controlled != 0);
    assert(mr->slot.entity != 0);
    assert(mr->entities.team[0].records[0].has_slot == 0u);
  }
  drop_lc_fixture(&f);
}

/* After the live kick test, a legally staged goal-mouth ball still runs the
 * native P6 goal/restart chain; the score is written only by that chain. */
static void test_live_goal_after_kick(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
    one_granted_frame(mr);
    assert(fifa96_match_run_input(mr, NULL, 0) == 0);
    for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
      one_granted_frame(mr);
    assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
    assert(mr->score[0] == 0u && mr->score[1] == 0u);
  }
  stage_ball(mr, 0, 0, 0xB30);
  {
    int goal = 0;
    for (int i = 0; i < LC_GOAL_CAP && goal == 0; i++) {
      one_granted_frame(mr);
      if (mr->score[0] == 1u) goal = 1;
    }
    assert(goal == 1);
  }
  assert(mr->score[1] == 0u);
  assert(mr->state.phase == 5u);        /* the native situation-6 phase */
  assert(mr->goal_record_side == 0u);
  assert(mr->goal_record >= 0 && mr->goal_record < 11);
  {
    int released = 0;
    for (int i = 0; i < LC_RESTART_CAP; i++) {
      one_granted_frame(mr);
      if (mr->state.phase == FIFA96_MATCH_RUN_KICKOFF_PHASE) {
        released = 1;
        break;
      }
    }
    assert(released == 1);
  }
  assert(mr->score[0] == 1u && mr->score[1] == 0u);
  /* The phase-1 kickoff entry waits for the FU-70 release word; pulse the KICK
   * edge like the live_goal fixture's tape until row 01 leaves phase 1. */
  {
    int resumed = 0;
    for (int i = 0; i < LC_RESTART_CAP; i++) {
      if (i % 30 == 0) {
        fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
        assert(fifa96_match_run_input(mr, &kick, 1) == 0);
      } else if (i % 30 == 2) {
        assert(fifa96_match_run_input(mr, NULL, 0) == 0);
      }
      one_granted_frame(mr);
      if (mr->state.phase == 2u) {
        resumed = 1;
        break;
      }
    }
    assert(resumed == 1);
  }
  for (int i = 0; i < 200; i++) {
    one_granted_frame(mr);
    assert(mr->score[0] == 1u && mr->score[1] == 0u);
  }
  drop_lc_fixture(&f);
}

int main(void) {
  test_live_begin_seeds_kickoff_carrier();
  test_live_kick_release_runs_row07();
  test_live_auto_switch_carrier_and_nearest();
  test_live_goal_after_kick();
  printf("test_engine_live_control: ok\n");
  return 0;
}
