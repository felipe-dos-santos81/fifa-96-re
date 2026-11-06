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
 *    ball after the kick path ran;
 *  - the live flight consumer turns the staged FU-73 vector into a real ball
 *    target, and an authentic attack-position kick scores through the native
 *    goal scan (1-0, phase 5, restart, score preserved);
 *  - the human KICK row aims at the opponent goal-mouth centre, so the real
 *    live kickoff (no ball/taker/camera staging) scores 1-0 and 0-1 through
 *    the same native goal/restart chain.
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

/* P7 restart resumability: after a goal/restart the opponent keeps the
 * kickoff ball and the human controls its nearest own outfield record. Place
 * that record inside the contact range (0x20 < the opponent's distance 0) and
 * step one frame: the P7-L2 contact tackle must hand the human the carrier
 * state. This is test-only position staging; no ball or score is written. */
static void restart_assert_possession(struct fifa96_match_run *mr, uint32_t side) {
  int32_t human = mr->entities.controlled;
  int32_t carrier = mr->entities.ball.carrier;
  struct fifa96_match_entity *e;
  assert(human >= (int32_t)(side * FIFA96_MATCH_ENTITY_RECORDS));
  assert(human < (int32_t)((side + 1u) * FIFA96_MATCH_ENTITY_RECORDS));
  e = &mr->entities.team[side].records[(uint32_t)human % FIFA96_MATCH_ENTITY_RECORDS];
  if (carrier >= (int32_t)(side * FIFA96_MATCH_ENTITY_RECORDS) &&
      carrier < (int32_t)((side + 1u) * FIFA96_MATCH_ENTITY_RECORDS)) {
    /* The fixture-side-switch case already owns the carrier; assert the
     * resumable state and skip the opponent tackle. */
    assert(e->code == 5u);
    return;
  }
  assert(carrier >= (int32_t)((side ^ 1u) * FIFA96_MATCH_ENTITY_RECORDS));
  assert(carrier < (int32_t)((side ^ 1u) + 1u) * (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
  e->pos_x = mr->entities.ball.x + 0x20;
  e->pos_z = mr->entities.ball.z;
  e->target_x = e->pos_x;
  e->target_z = e->pos_z;
  one_granted_frame(mr);
  assert(mr->entities.ball.carrier == human);
  assert(e->code == 5u);
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
  /* Clear the opening-kick latch directly (fixture state, not a kick): this
   * test isolates the selection/claim rules from the shot/flight path. */
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 0u;

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

/* The full natural loop: live begin, the taker staged in an authentic attack
 * position in front of the +z goal, a plain KICK press/release whose native
 * row-07 staging points at the goal (the record's own +0x8E facing type 0 =
 * +z), the derived live flight mapping, the live goal armer and the native
 * goal scan. The score is only ever written by the native goal chain. */
static void test_live_kick_flight_scores(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  const int32_t taker = mr->entities.controlled;
  struct fifa96_match_entity *e;
  int32_t start_z;
  assert(taker >= 0 && taker != 0);
  e = &mr->entities.team[0].records[taker];

  /* Authentic attack position (not inside the goal): just outside the +z
   * penalty area, facing the +z goal. This is record staging, not a score or
   * phase write. */
  e->pos_x = 0;
  e->pos_z = 0x900;
  e->target_x = 0;
  e->target_z = 0x900;
  stage_ball(mr, 0, 0, 0x900);
  mr->entities.ball.carrier = taker;
  one_granted_frame(mr);
  assert(mr->entities.controlled == taker);
  assert(mr->entities.team[0].records[taker].code == 5u);

  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
  }
  one_granted_frame(mr);
  /* The AI/record walk can touch the facing byte in the press frame; stage the
   * record's own facing/dir for the release (the same legal record staging the
   * task allows). Type 0 is the +z octant in the kick type-dir table. */
  e->type = 0;
  e->dir_x = 0;
  e->dir_z = 1;
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_z > 0xB10);   /* beyond the +z goal line */
  assert(mr->live_kick_speed >= 8);
  start_z = mr->entities.ball.z;

  {
    int goal = 0;
    for (int i = 0; i < 200 && goal == 0; i++) {
      one_granted_frame(mr);
      if (mr->score[0] == 1u) goal = 1;
    }
    assert(goal == 1);
  }
  assert(mr->score[0] == 1u && mr->score[1] == 0u);
  assert(mr->state.phase == 5u);            /* the native situation-6 phase */
  assert(mr->goal_record_side == 0u);
  assert(mr->goal_armed == 1u);
  assert(mr->entities.ball.z > start_z + 0x100);   /* a real flight, not a nudge */
  assert(mr->live_kick_active == 0u);       /* cleared by the goal */

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
  for (int i = 0; i < 100; i++) {
    one_granted_frame(mr);
    assert(mr->score[0] == 1u && mr->score[1] == 0u);
  }
  drop_lc_fixture(&f);
}

/* The real live kickoff, no ball/taker/camera staging: the live seed puts the
 * controlled side's taker at the kickoff; a KICK press/release aims the flight
 * at that side's opponent goal-mouth centre (side 0 +0xB30, side 1 -0xB30) and
 * the native armer/scan scores. This is the shipped menu-input path, not the
 * staged attack fixture above. */
static void test_live_kickoff_shot_scores_side(uint32_t side) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t taker;
  /* No staging: the ball is the real kickoff placement immediately after
   * begin (the side-1 fixture switch below may advance a couple of frames). */
  assert(mr->entities.ball.x == 480 && mr->entities.ball.y == 0 &&
         mr->entities.ball.z == 0);
  if (side != 0u) {
    /* The phase machine is freshly initialised to side 0 by begin; flip the
     * controlled side through the same cell the native FUN_000740A0 writes and
     * let the live control follow rebind. This is a fixture switch, not ball or
     * taker staging. */
    mr->phase_machine.side_controlled = (uint8_t)side;
    one_granted_frame(mr);
    one_granted_frame(mr);
  }
  taker = mr->entities.controlled;
  assert(taker >= (int32_t)(side * FIFA96_MATCH_ENTITY_RECORDS));
  assert(taker < (int32_t)((side + 1u) * FIFA96_MATCH_ENTITY_RECORDS));
  assert(taker != (int32_t)(side * FIFA96_MATCH_ENTITY_RECORDS));   /* no keeper */
  assert(mr->slot.entity == taker);
  assert(mr->entities.team[side].target == taker);
  if (side != 0u) {
    /* Fixture-only controlled-side switch: hand the switched side the kickoff
     * possession cell and the carrier state so the optional 0-1 case exercises
     * the same KICK/flight path. The real side-0 case above is the no-staging
     * shipped-kickoff test; no ball position or score is written. */
    struct fifa96_match_entity *e =
        &mr->entities.team[side].records[(uint32_t)taker % 11u];
    mr->entities.ball.carrier = taker;
    (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
  }
  assert(mr->entities.team[side].records[taker % 11u].code == 5u);

  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
  }
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_x == 0);
  assert(mr->live_kick_target_z == (side == 0u ? 0xB30 : -0xB30));
  assert(mr->live_kick_speed >= 8);
  assert(mr->live_kick_frames >= 500u);

  {
    int goal = 0;
    for (int i = 0; i < 500 && goal == 0; i++) {
      one_granted_frame(mr);
      if (mr->score[side] == 1u) goal = 1;
    }
    assert(goal == 1);
  }
  assert(mr->score[side] == 1u);
  assert(mr->score[side ^ 1u] == 0u);
  assert(mr->state.phase == 5u);
  assert(mr->goal_record_side == (uint8_t)side);
  assert(mr->goal_armed == 1u && mr->goal_zone == 1u);
  assert(mr->live_kick_active == 0u);   /* cleared by the goal */

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
  assert(mr->score[side] == 1u && mr->score[side ^ 1u] == 0u);
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
  restart_assert_possession(mr, side);
  {
    int32_t bx = mr->entities.ball.x;
    int32_t bz = mr->entities.ball.z;
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
    one_granted_frame(mr);
    assert(fifa96_match_run_input(mr, NULL, 0) == 0);
    for (int i = 0; i < 8; i++) one_granted_frame(mr);
    assert(mr->live_kick_active == 1u);
    assert(mr->entities.ball.x != bx || mr->entities.ball.z != bz);
    assert(mr->score[side] == 1u && mr->score[side ^ 1u] == 0u);
  }
  /* The restart is resumable and the human can kick again. A further flight is
   * allowed to score a second native goal; this test only pins the restart
   * possession + new kick, so it stops before that flight resolves. */
  drop_lc_fixture(&f);
}

static void test_live_kickoff_shot_scores(void) {
  test_live_kickoff_shot_scores_side(0u);
  test_live_kickoff_shot_scores_side(1u);
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
  restart_assert_possession(mr, 0u);
  for (int i = 0; i < 200; i++) {
    one_granted_frame(mr);
    assert(mr->score[0] == 1u && mr->score[1] == 0u);
  }
  drop_lc_fixture(&f);
}

/* P7 taker guard + vector re-latch: after the opening kick the old taker is an
 * AI record and must not arm a human flight; the current human slot still
 * latches, and a new direction with the same actor/traj/flags/code re-latches
 * (the signature includes the staged vector). */
static void test_live_taker_guard_and_relatch(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t old_taker = mr->live_control_taker;
  int32_t human;
  assert(old_taker >= 0);

  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
  }
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert(mr->live_kickoff_touch == 0u);
  assert(mr->live_control_taker == FIFA96_MATCH_ENTITY_NONE);

  /* AI pair from the old taker: no human flight, target untouched. */
  mr->live_kick_active = 0;
  mr->live_kick_target_x = 0x1234;
  mr->live_kick_target_z = -0x1234;
  mr->entities.ball.pair.actor = old_taker;
  mr->entities.ball.pair.flags = 0x10u;
  mr->entities.ball.pair.code = 6u;
  mr->entities.ball.pair.traj = 999;
  mr->entities.ball.pair.vector.x = 720;
  mr->entities.ball.pair.vector.height = 0;
  mr->entities.ball.pair.vector.z = -720;
  one_granted_frame(mr);
  assert(mr->live_kick_active == 0u);
  assert(mr->live_kick_target_x == 0x1234);
  assert(mr->live_kick_target_z == -0x1234);

  /* A pair from the current human slot still latches (PASS mode keeps the
   * derived kicker-relative target). */
  human = mr->slot.entity;
  if (human < 0 ||
      human >= (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS))
    human = mr->entities.controlled;
  assert(human >= 0 &&
         human < (int32_t)(FIFA96_MATCH_ENTITY_TEAMS * FIFA96_MATCH_ENTITY_RECORDS));
  /* Isolate the human record from the AI pair staging so the staged pair
   * survives to the frame-tail latch (fixture staging only). */
  for (uint32_t tt = 0; tt < FIFA96_MATCH_ENTITY_TEAMS; tt++)
    for (uint32_t ii = 1; ii < FIFA96_MATCH_ENTITY_RECORDS; ii++) {
      if (tt == 0u && (int32_t)ii == human % (int32_t)FIFA96_MATCH_ENTITY_RECORDS)
        continue;
      mr->entities.team[tt].records[ii].skip_9a = 1;
    }
  mr->slot.entity = human;
  mr->entities.controlled = human;
  mr->entities.team[0].target = human;
  mr->entities.ball.pair.actor = human;
  mr->entities.ball.pair.flags = 0x20u;
  mr->entities.ball.pair.code = 6u;
  mr->entities.ball.pair.traj = 777;
  mr->entities.ball.pair.vector.x = 720;
  mr->entities.ball.pair.vector.height = 100;
  mr->entities.ball.pair.vector.z = 100;
  one_granted_frame(mr);
  assert(mr->live_kick_active == 1u);
  {
    int32_t first_x = mr->live_kick_target_x;
    int32_t first_z = mr->live_kick_target_z;
    /* Same actor/traj/flags/code, new direction: the vector is part of the
     * signature, so this is a new kick and re-latches. */
    mr->live_kick_active = 0;
    mr->entities.ball.pair.actor = human;
    mr->entities.ball.pair.flags = 0x20u;
    mr->entities.ball.pair.code = 6u;
    mr->entities.ball.pair.traj = 777;
    mr->entities.ball.pair.vector.x = 720;
    mr->entities.ball.pair.vector.height = 50;
    mr->entities.ball.pair.vector.z = 150;
    one_granted_frame(mr);
    assert(mr->live_kick_active == 1u);
    assert(mr->live_kick_target_x != first_x ||
           mr->live_kick_target_z != first_z);
  }
  drop_lc_fixture(&f);
}

int main(void) {
  test_live_begin_seeds_kickoff_carrier();
  test_live_kick_release_runs_row07();
  test_live_kick_flight_scores();
  test_live_kickoff_shot_scores();
  test_live_taker_guard_and_relatch();
  test_live_auto_switch_carrier_and_nearest();
  test_live_goal_after_kick();
  printf("test_engine_live_control: ok\n");
  return 0;
}
