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
 *  - P9 / FU-347: the KICK row's goal-mouth aim is steered by the held depth
 *    input (Up +0x80, Down -0x80, none/both centre), so the human can beat
 *    a live keeper; the defending record 0 saves a centre shot (P9 stand-in,
 *    no score/phase write) while a ±0x80 aim or a displaced keeper scores
 *    through the same native chain. The centre save is also pinned on the real
 *    kickoff with only a one-time keeper placement, no per-frame staging.
 *
 * Everything here is live-only (the M1/M2 headless tapes never set live_start).
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_engine_internal.h"
#include "fifa96_engine/fifa96_match_handlers.h"
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
  /* P8: the far-carrier gate may leave a kickoff restart ball loose (an
   * opponent target no longer claims it from across the pitch). Both states are
   * legal here: an opponent contact carrier or a loose ball. */
  if (carrier >= 0) {
    assert(carrier >= (int32_t)((side ^ 1u) * FIFA96_MATCH_ENTITY_RECORDS));
    assert(carrier < (int32_t)((side ^ 1u) + 1u) * (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
  } else {
    assert(carrier == FIFA96_MATCH_ENTITY_NONE);
  }
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

/* Stage an eligible pool for the phase-arm pick (both teams' records 1..10
 * active with distinct targets; the pick skips index 0 and the +0x98/+0x9A
 * exclusions, FUN_00079CCC). */
static void lc_stage_restarts(struct fifa96_match_run *mr) {
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *e = &mr->entities.team[t].records[i];
      e->active = 1;
      e->skip_98 = 0;
      e->skip_9a = 0;
      e->target_x = (int32_t)((t * FIFA96_MATCH_ENTITY_RECORDS + i) * 0x100);
      e->target_z = 0;
      e->pos_x = e->target_x;
      e->pos_z = 0;
    }
  }
}

/* P9 helpers. The pool team index carrying a side byte (the same scan the live
 * control/opponent update uses). */
static int32_t lc_team_for_side(struct fifa96_match_run *mr, uint8_t side) {
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    if (mr->entities.team[t].side == side) return (int32_t)t;
  return -1;
}

/* P9 (FU-347): press KICK together with an optional depth key (Up = +x,
 * Down = -x). The native FU-70 slot only publishes its released 0x10 word when
 * every held key drops (`fifa96_control_slot_update`'s held/held_prev logic),
 * so the aim has to be held at the same poll as the kick press; the release
 * poll then delivers the KICK word and the live aim cell keeps the depth
 * sample. Left/Right are the goal-side z movement axis, not aim inputs. */
static void lc_press_kick(struct fifa96_match_run *mr, int32_t aim_key) {
  fifa96_platform_key keys[3] = {
    {FIFA96_ENGINE_KEY_KICK, 1}, {0, 1}, {0, 1}
  };
  size_t count = 1;
  if (aim_key != 0) keys[count++].raw_code = aim_key;
  assert(fifa96_match_run_input(mr, keys, count) == 0);
}

static void lc_press_key(struct fifa96_match_run *mr, int32_t key) {
  fifa96_platform_key k = {key, 1};
  assert(fifa96_match_run_input(mr, &k, 1) == 0);
}

static void lc_release_keys(struct fifa96_match_run *mr) {
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
}

/* The shipped ISO keeper tracks to `(0, ±0x9EA)` at the shot (P8 AI recon,
 * `P8-ai-recon.md` finding 4: engine probe, defending keeper `(0,2706)` ->
 * `(0,2538)`, the centre shot passes at distance `0x2F`). The headless fixture
 * has no formation seed, so record 0 otherwise starts at the origin; the
 * keeper tests pin that goal spot (fixture staging only — no score/ball
 * write). The no-staging kickoff variant pins it once and then lets the live
 * AI run. */
#define LC_KEEPER_GOAL_Z 0x9EA
static int32_t lc_keeper_goal_z(uint8_t defending_side) {
  return defending_side == 1u ? LC_KEEPER_GOAL_Z : -LC_KEEPER_GOAL_Z;
}
static void lc_pin_keeper(struct fifa96_match_run *mr, int32_t team, int32_t x,
                          int32_t z) {
  struct fifa96_match_entity *k = &mr->entities.team[(uint32_t)team].records[0];
  k->pos_x = x;
  k->pos_z = z;
  k->target_x = x;
  k->target_z = z;
}

/* The pool team index for the defending side. The controlled side is the
 * complement; callers that need it use `lc_team_for_side` directly. */
static void lc_sides(struct fifa96_match_run *mr, uint8_t controlled,
                     int32_t *defending_team, uint8_t *defending_side) {
  *defending_side = (uint8_t)(controlled ^ 1u);
  *defending_team = lc_team_for_side(mr, *defending_side);
  assert(*defending_team >= 0);
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

/* P13 / live carrier dribble: the human carrier's row-05 target is the held
 * direction, not the camera. Before this fix the record target became the
 * camera triple and the player could only kick-and-chase. */
static void test_live_carrier_dribbles_with_ball(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t id = mr->entities.controlled;
  int32_t x0, z0, bx, bz;
  struct fifa96_match_entity *e;
  assert(id > 0 && id < 11);
  e = &mr->entities.team[0].records[id];
  assert(mr->entities.ball.carrier == id);
  /* The no-ISO fixture has no formation seed around the kickoff ball; place
   * the taker on it, the live bootstrap's documented intent, so this test
   * exercises the possession glue rather than the missing `.fmt` placement. */
  stage_ball(mr, e->pos_x, 0, e->pos_z);
  mr->entities.ball.carrier = id;
  /* No-ISO fixture: the opponent formation is not seeded, so a neighbour can
   * sit unrealistically close to the kickoff spot. Suppress the derived AI
   * tackle for this dribble-only case (contact has its own tests). */
  mr->live_ai_tackle_cd = 120u;
  x0 = e->pos_x;
  z0 = e->pos_z;
  {
    fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
    assert(fifa96_match_run_input(mr, &up, 1) == 0);
  }
  for (int i = 0; i < 8; i++) one_granted_frame(mr);
  assert(e->pos_x > x0);
  assert(e->target_x > x0);
  assert(e->pos_x - x0 <= 0x80 * 8);
  assert(e->pos_z == z0);
  bx = mr->entities.ball.x - e->pos_x;
  bz = mr->entities.ball.z - e->pos_z;
  assert(fifa96_entity_distance(bx, bz) <= 0x400);
  /* The carried ball follows the player (the live possession glue for the
   * unported native 0x7A490 staging): it must not stay at the kickoff spot. */
  assert(bx >= -0x40 && bx <= 0x40);
  assert(bz >= -0x40 && bz <= 0x40);
  /* Phase-86: a neutral pad direction must still keep the ball in front of the
   * carrier's facing octant; the old glue pinned it at (0,0) and the sprite
   * covered it. The fallback offset is one 0x20 step, not a teleport. */
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  one_granted_frame(mr);
  bx = mr->entities.ball.x - e->pos_x;
  bz = mr->entities.ball.z - e->pos_z;
  assert((bx != 0 || bz != 0) && abs(bx) <= 0x20 && abs(bz) <= 0x20);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

/* Live off-ball movement: after the human releases the ball, the controlled
 * record's action code is 3 (the native hold/clamp row). The shared action
 * table keeps row 03 NULL so the M2 tape stays byte-identical, but the live
 * windowed path must dispatch the ported body for the bound slot record —
 * otherwise the human can only kick and never chase. */
static void test_live_offball_slot_player_moves(void) {
  struct fifa96_match_run probe;
  memset(&probe, 0, sizeof probe);
  fifa96_match_run_init(&probe);
  assert(fifa96_match_dispatch_action(&probe, 3u) == -FIFA96_ERR_UNSUPPORTED);

  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t id = mr->entities.controlled;
  assert(id > 0 && id < 11);
  struct fifa96_match_entity *e = &mr->entities.team[0].records[id];
  int32_t x0, z0;
  /* Clear the opening touch and make the ball loose. The hold keeps the
   * derived nearest-claim from re-installing carrier state 5. */
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 120u;
  mr->live_save_hold = 0u;
  mr->ball_settle_active = 0u;
  mr->act.id = 0u;
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *r = &mr->entities.team[t].records[i];
      r->pos_x = 0x600;
      r->pos_z = 0x600;
      r->target_x = r->pos_x;
      r->target_z = r->pos_z;
    }
  }
  e->pos_x = 0;
  e->pos_z = 0;
  e->target_x = 0;
  e->target_z = 0;
  e->code = 3u;
  e->stage92 = 0u;
  e->timer89 = 0;
  e->has_slot = 1u;
  mr->slot.entity = id;
  mr->entities.controlled = id;
  mr->entities.team[0].target = id;
  mr->entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].intercept = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].chosen = FIFA96_MATCH_ENTITY_NONE;
  /* Direct live row-03 dispatch: the slot branch uses the pad octant to build
   * pos + dir*0x80 (the native 0x79C20 step). */
  mr->record.entity_id = id;
  mr->record.pos_x = 0;
  mr->record.pos_z = 0;
  mr->record.target_x = 0;
  mr->record.target_z = 0;
  mr->record.has_slot = 1u;
  mr->record.dir_x = 1;
  mr->record.dir_z = 0;
  assert(fifa96_match_dispatch_action(mr, 3u) == FIFA96_OK);
  assert(mr->record.target_x == 0x80);
  assert(mr->record.target_z == 0);
  stage_ball(mr, 0, 0, 0);
  x0 = e->pos_x;
  z0 = e->pos_z;
  {
    fifa96_platform_key up = {FIFA96_ENGINE_KEY_UP, 1};
    assert(fifa96_match_run_input(mr, &up, 1) == 0);
  }
  for (int i = 0; i < 8; i++) one_granted_frame(mr);
  assert(e->code == 3u || e->code == 4u);
  assert(e->pos_x > x0);
  assert(e->target_x > x0);
  assert(e->pos_z == z0);
  assert(mr->state.phase == 2u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
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
  /* The no-ISO fixture has no opponent formation, so the glued kickoff ball
   * can sit next to a synthetic neighbour; keep the derived AI tackle out of
   * this kick-only case (contact has its own tests). */
  mr->live_ai_tackle_cd = 120u;
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

/* Phase-85: a PASS/LONG_BALL flight is projected far enough ahead to be a
 * visible pass. The old fixed x2 mapping put the target two units from the
 * kicker, so the derived flight arrived on its first step and the ball looked
 * stuck. The pair's speed word now scales the reach. */
static void test_live_pass_flight_travels(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  const int32_t taker = mr->entities.controlled;
  struct fifa96_match_entity *e;
  int32_t reach;
  assert(taker >= 0 && taker < 11);
  e = &mr->entities.team[0].records[taker];
  mr->live_ai_tackle_cd = 120u;
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.ball.carrier = taker;
  stage_ball(mr, e->pos_x, 0, e->pos_z);
  /* Stage the FU-73 PASS pair the row-07 body would hand to the latch. */
  mr->entities.ball.pair.actor = taker;
  mr->entities.ball.pair.flags = 0x20u;
  mr->entities.ball.pair.vector.x = 0x400;
  mr->entities.ball.pair.vector.height = 0x200;
  mr->entities.ball.pair.vector.z = 0x100;
  mr->live_kick_pair_consumed = 0u;
  one_granted_frame(mr);
  assert(mr->live_kick_active == 1u);
  assert((mr->entities.ball.pair.flags & 0x70u) == 0x20u);
  reach = fifa96_entity_distance(mr->live_kick_target_x - e->pos_x,
                                 mr->live_kick_target_z - e->pos_z);
  assert(reach > 0x200);   /* two units was the broken stand-in */
  assert(mr->live_kick_target_x >= -0x720 && mr->live_kick_target_x <= 0x720);
  assert(mr->live_kick_target_z >= -0xB10 && mr->live_kick_target_z <= 0xB10);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  drop_lc_fixture(&f);
}

/* P13 live boundary armer/restart fallback: a ball over the touchline is a
 * throw-in even when its z is beyond the old 0xB20 classifier band, and the
 * bounded fallback returns to phase 2 instead of holding the goal-kick row. */
static void test_live_touchline_exit_throw_in(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  for (int i = 0; i < 12 && !mr->live_out_pending; i++) {
    stage_ball(mr, 0x7E1, 0, 0);
    one_granted_frame(mr);
  }
  assert(mr->live_out_situation == 2u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  for (int i = 0; i < 140 && (mr->live_out_pending || mr->state.phase != 2u); i++)
    one_granted_frame(mr);
  assert(mr->state.phase == 2u);
  assert(mr->live_out_pending == 0u);
  assert(mr->entities.ball.x >= 0x780 && mr->entities.ball.x <= 0x800);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  drop_lc_fixture(&f);
}

static void test_live_goal_line_exit_restarts(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  for (int i = 0; i < 12 && !mr->live_out_pending; i++) {
    stage_ball(mr, 0, 0, 0xC1F);
    one_granted_frame(mr);
  }
  assert(mr->live_out_situation == 3u || mr->live_out_situation == 4u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  for (int i = 0; i < 140 && (mr->live_out_pending || mr->state.phase != 2u); i++)
    one_granted_frame(mr);
  assert(mr->state.phase == 2u);
  assert(mr->live_out_pending == 0u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  drop_lc_fixture(&f);
}

static void test_live_inside_ball_never_restarts(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  for (int i = 0; i < 20; i++) {
    stage_ball(mr, 0x6C1, 0, 0xAB1);
    one_granted_frame(mr);
    assert(mr->live_out_pending == 0u);
    assert(mr->state.phase == 2u);
  }
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
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

/* Phase 76 [D]: the manual switch cycles the own outfield record and holds
 * the selection; the physical key level is edge-latched. */
static void press_live_switch(struct fifa96_match_run *mr) {
  fifa96_platform_key key = {.raw_code = FIFA96_ENGINE_KEY_SWITCH, .state = 1};
  assert(fifa96_match_run_input(mr, &key, 1) == 0);
}

static void release_live_switch(struct fifa96_match_run *mr) {
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
}

static void stage_live_switch_fixture(struct fifa96_match_run *mr) {
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 60u;   /* keep the loose-ball claim from firing */
  mr->live_save_hold = 0u;
  mr->ball_settle_active = 0u;
  mr->act.id = 0u;
  mr->phase_machine.side_controlled = 0u;
  mr->entities.team[0].side = 0u;
  mr->entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].intercept = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].chosen = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.controlled = FIFA96_MATCH_ENTITY_NONE;
  /* The opponent possession cell keeps the loose-ball claim out of this test
   * while still exercising the manual cycle. */
  mr->entities.ball.carrier = 12;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &mr->entities.team[0].records[i];
    e->active = 1;
    e->code = 3;
    e->skip_98 = 0;
    e->skip_9a = 0;
    e->pos_x = 0;
    e->pos_z = 0;
    e->target_x = 0;
    e->target_z = 0;
  }
  /* Keep the ball away from every outfield record so the control follow picks
   * the deterministic lowest-index outfield (record 1) without a possession
   * claim; the manual cycle then runs 1 -> 2 -> ... */
  stage_ball(mr, 0x700, 0, 0x700);
}

static void test_live_manual_switch_cycles(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  stage_live_switch_fixture(mr);
  one_granted_frame(mr);
  assert(mr->entities.controlled == 1);

  press_live_switch(mr);
  one_granted_frame(mr);
  assert(mr->entities.controlled == 2);
  assert(mr->slot.entity == 2);
  assert(mr->render.camera.tracked == 2);
  assert(mr->live_manual_id == 2);

  /* A held key does not cycle again until release/re-press. */
  one_granted_frame(mr);
  assert(mr->entities.controlled == 2);
  release_live_switch(mr);
  press_live_switch(mr);
  one_granted_frame(mr);
  assert(mr->entities.controlled == 3);

  /* Hidden records are skipped by the cycle itself. */
  mr->entities.team[0].records[4].skip_98 = 1;
  release_live_switch(mr);
  press_live_switch(mr);
  one_granted_frame(mr);
  assert(mr->entities.controlled == 5);

  /* The manual hold expires and the auto selection returns. */
  mr->live_manual_frames = 1;
  one_granted_frame(mr);
  assert(mr->entities.controlled == 5);
  assert(mr->live_manual_id == FIFA96_MATCH_ENTITY_NONE);
  one_granted_frame(mr);
  assert(mr->entities.controlled == 1);
  drop_lc_fixture(&f);
}

static void test_live_manual_switch_keeps_carrier(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  stage_live_switch_fixture(mr);
  mr->entities.team[0].records[3].pos_x = 0;
  mr->entities.team[0].records[3].pos_z = 0;
  stage_ball(mr, 0, 0, 0);
  mr->entities.ball.carrier = 3;
  one_granted_frame(mr);
  assert(mr->entities.controlled == 3);
  press_live_switch(mr);
  one_granted_frame(mr);
  assert(mr->entities.controlled == 3);
  assert(mr->live_manual_id == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->live_switch_press == 0u);
  drop_lc_fixture(&f);
}

static void press_live_tackle(struct fifa96_match_run *mr);

/* Phase 76/77: switch/tackle presses must never become FU-61 action words. */
static void test_live_switch_is_not_fu61(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  stage_live_switch_fixture(mr);
  press_live_switch(mr);
  assert(mr->input.prev[0] == 0);
  assert(mr->input_state[0] == 0);
  assert(mr->live_switch_press == 1u);
  release_live_switch(mr);
  press_live_tackle(mr);
  assert(mr->input.prev[0] == 0);
  assert(mr->input_state[0] == 0);
  assert(mr->live_tackle_press == 1u);
  drop_lc_fixture(&f);
}

static void press_live_tackle(struct fifa96_match_run *mr) {
  fifa96_platform_key key = {.raw_code = FIFA96_ENGINE_KEY_TACKLE, .state = 1};
  assert(fifa96_match_run_input(mr, &key, 1) == 0);
}

static void release_live_tackle(struct fifa96_match_run *mr) {
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
}

/* Phase 77 [D]: the deliberate human contact key runs the same referee seam as
 * the AI. Fouls Off is a clean steal; Fouls On can award the opponent restart.
 * The stored human side survives the native restart-side flip. */
static void test_live_human_contact_clean_and_foul(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  uint8_t human_side = (uint8_t)(mr->phase_machine.side_controlled & 1u);
  uint32_t human_team = human_side;
  uint32_t opp = 1u - human_team;
  int32_t human_id = mr->entities.controlled;
  int32_t carrier = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + 3u);
  assert(human_id >= 0 && human_id < (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 0u;
  mr->live_save_hold = 0u;
  mr->live_ai_tackle_cd = 60u;   /* keep the AI opponent contest out of this test */
  mr->live_human_tackle_cd = 0u;
  mr->ball_settle_active = 0u;
  mr->act.id = 0u;
  mr->referee.sequence = FIFA96_REF_SEQ_NONE;
  mr->state.phase = 2u;
  mr->live_human_side = human_side;
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *h = &mr->entities.team[human_team].records[i];
    struct fifa96_match_entity *o = &mr->entities.team[opp].records[i];
    h->pos_x = (int32_t)(i == (uint32_t)human_id ? 0 : 0x3000);
    h->pos_z = h->pos_x;
    h->target_x = h->pos_x;
    h->target_z = h->pos_z;
    o->pos_x = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + i) == carrier ? 0x20 : 0x900;
    o->pos_z = o->pos_x;
    o->target_x = o->pos_x;
    o->target_z = o->pos_z;
  }
  stage_ball(mr, 0, 0, 0);
  mr->entities.ball.carrier = carrier;

  /* Fouls Off: no decision, the human wins the clean challenge. */
  mr->config.field_4c306 = 0;
  press_live_tackle(mr);
  one_granted_frame(mr);
  assert(mr->entities.ball.carrier == human_id);
  assert(mr->act.id == 0u);
  assert(mr->state.phase == 2u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  release_live_tackle(mr);

  /* Fouls On: the same key registers a human foul and gives the opponent the
   * restart. Seed 0 + three draws selects the kind-1 path used by the AI test. */
  mr->config.field_4c306 = 2;
  mr->config.field_4c30a = 1;
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *h = &mr->entities.team[human_team].records[i];
    struct fifa96_match_entity *o = &mr->entities.team[opp].records[i];
    h->pos_x = (int32_t)(i == (uint32_t)human_id ? 0 : 0x3000);
    h->pos_z = h->pos_x;
    h->target_x = h->pos_x;
    h->target_z = h->pos_z;
    o->pos_x = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + i) == carrier ? 0x20 : 0x900;
    o->pos_z = o->pos_x;
    o->target_x = o->pos_x;
    o->target_z = o->pos_z;
  }
  stage_ball(mr, 0, 0, 0);
  mr->entities.ball.speed = 0;
  mr->entities.ball.carrier = carrier;
  /* Drop the clean challenge's human carrier code so the opponent possession
   * cell is the one the entity chain sees. */
  mr->entities.team[human_team].records[human_id].code = 3u;
  mr->entities.team[human_team].records[human_id].has_ball = 0u;
  mr->entities.team[human_team].records[human_id].carrier = 0u;
  mr->act.id = 0u;
  mr->referee.sequence = FIFA96_REF_SEQ_NONE;
  mr->state.phase = 2u;
  mr->live_human_tackle_cd = 0u;
  (void)fifa96_rng_seed(&mr->rng, 0);
  for (int i = 0; i < 3; i++) {
    uint16_t value = 0;
    (void)fifa96_rng_step(&mr->rng, &value);
  }
  press_live_tackle(mr);
  one_granted_frame(mr);
  assert(mr->referee.rec_first == human_id);
  assert(mr->referee.rec_second == carrier);
  assert(mr->referee.contact_kind == 1u);
  assert(mr->referee.recall_consumed == 1u);
  assert(mr->act.id != 0u);
  assert(mr->state.phase != 2u);
  assert(mr->entities.ball.carrier == carrier);
  release_live_tackle(mr);

  /* Native restart-side flip: the stored human side still selects the human
   * team after phase 2 resumes. */
  mr->phase_machine.side_controlled = (uint8_t)opp;
  mr->state.phase = 2u;
  mr->act.id = 0u;
  mr->live_kickoff_touch = 0u;
  mr->referee.sequence = FIFA96_REF_SEQ_NONE;
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[human_team].records[human_id].pos_x = 0;
  mr->entities.team[human_team].records[human_id].pos_z = 0;
  stage_ball(mr, 0, 0, 0);
  one_granted_frame(mr);
  assert(mr->entities.controlled >= 0 &&
         mr->entities.controlled < (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
  drop_lc_fixture(&f);
}

/* The full natural loop: live begin, the taker staged in an authentic attack
 * position in front of the +z goal, a KICK + Up press/release (P9 depth aim:
 * +0x80, outside the keeper's reach) whose native row-07 staging points at the
 * goal (the record's own +0x8E facing type 0 = +z), the derived live flight
 * mapping, the live goal armer and the native goal scan. The score is only ever
 * written by the native goal chain. */
static void test_live_kick_flight_scores(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  const int32_t taker = mr->entities.controlled;
  struct fifa96_match_entity *e;
  int32_t keeper_team = lc_team_for_side(mr, 1u);
  int32_t start_z;
  assert(taker >= 0 && taker != 0);
  assert(keeper_team >= 0);
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

  lc_press_kick(mr, FIFA96_ENGINE_KEY_UP);
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
  assert(mr->live_kick_target_x == FIFA96_MATCH_LIVE_SHOT_AIM_X);   /* P9 Up */
  assert(mr->live_kick_target_z > 0xB10);   /* beyond the +z goal line */
  assert(mr->live_kick_speed >= 8);
  start_z = mr->entities.ball.z;

  {
    int goal = 0;
    for (int i = 0; i < 200 && goal == 0; i++) {
      /* The shipped ISO keeper is inside the mouth but 0x80 from the aimed
       * target, so the P9 keeper save must not fire. */
      lc_pin_keeper(mr, keeper_team, 0, lc_keeper_goal_z(1u));
      one_granted_frame(mr);
      if (mr->score[0] == 1u) goal = 1;
    }
    assert(goal == 1);
  }
  assert(mr->score[0] == 1u && mr->score[1] == 0u);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
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
 * controlled side's taker at the kickoff; a KICK + depth press/release aims
 * the flight at that side's opponent goal mouth (P9-fix2: Up is world +x, so
 * side 0 uses Up +0x80; side 1 uses Down -0x80; both are outside the keeper's
 * reach) and the native armer/scan scores. The defending keeper is pinned at
 * its real ISO goal spot, so the aim is proven against a keeper that would
 * save the centre shot. This is the shipped menu-input path, not the staged
 * attack fixture above. */
static void test_live_kickoff_shot_scores_side(uint32_t side) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t def_team;
  uint8_t def_side;
  int32_t aim_key = side == 0u ? FIFA96_ENGINE_KEY_UP : FIFA96_ENGINE_KEY_DOWN;
  int32_t aim_x =
      side == 0u ? FIFA96_MATCH_LIVE_SHOT_AIM_X : -FIFA96_MATCH_LIVE_SHOT_AIM_X;
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
    mr->live_human_side = (uint8_t)side;   /* stored human team follows the fixture flip */
    one_granted_frame(mr);
    one_granted_frame(mr);
  }
  lc_sides(mr, (uint8_t)side, &def_team, &def_side);
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

  lc_press_kick(mr, aim_key);
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_x == aim_x);      /* P9 depth aim */
  assert(mr->live_kick_target_z == (side == 0u ? 0xB30 : -0xB30));
  assert(mr->live_kick_speed >= 8);
  assert(mr->live_kick_frames >= 500u);

  {
    int goal = 0;
    for (int i = 0; i < 500 && goal == 0; i++) {
      /* A real keeper at the goal spot: the 0x80-away aim must beat it. */
      lc_pin_keeper(mr, def_team, 0, lc_keeper_goal_z(def_side));
      one_granted_frame(mr);
      if (mr->score[side] == 1u) goal = 1;
    }
    assert(goal == 1);
  }
  assert(mr->score[side] == 1u);
  assert(mr->score[side ^ 1u] == 0u);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
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
    /* P9: a depth aim (the keeper save would take a centre shot); the staged
     * goal-mouth ball below is what exercises the native chain. */
    lc_press_kick(mr, FIFA96_ENGINE_KEY_UP);
    one_granted_frame(mr);
    assert(fifa96_match_run_input(mr, NULL, 0) == 0);
    for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
      one_granted_frame(mr);
    assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
    assert(mr->live_kick_target_x == FIFA96_MATCH_LIVE_SHOT_AIM_X);
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

/* P8 helpers: `lc_abs32` is the live-zone magnitude used by the block tests. */

static int32_t lc_abs32(int32_t v) { return v < 0 ? -v : v; }

/* P8-T1: the live opponent target rotation at the native third-template anchor
 * (`0x8D948..0x8D9B7`, `0x8D9B2 CALL 0x8DE8C`; anchor = camera.pos +
 * camera.vel*32). The staged opponent record nearest the anchor is re-picked,
 * becomes the row-04/05 carrier record and closes on the ball; the human's team
 * target/control are never moved to the opponent. No score/phase write.
 *
 * The ball is held at the origin each frame so the approach is measured against
 * a fixed contest point (the live ball machine would otherwise carry it away
 * with the camera-relative lead). */
static void test_live_opponent_presses_and_claims(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human_team = lc_team_for_side(mr, (uint8_t)(mr->phase_machine.side_controlled & 1u));
  uint32_t opp;
  int32_t rival;
  int32_t start_distance;
  int32_t end_distance;
  assert(human_team >= 0);
  opp = 1u - (uint32_t)human_team;
  rival = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + 3u);
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_hold = 0u;
  stage_ball(mr, 0, 0, 0);
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    /* Keep the now-mobile human records outside the control range so this
     * opponent-AI fixture still measures the opponent's claim; the old 0x600
     * park was reachable once the human carrier stopped being camera-pinned. */
    mr->entities.team[(uint32_t)human_team].records[i].pos_x = 0x4000;
    mr->entities.team[(uint32_t)human_team].records[i].pos_z = 0x4000;
    mr->entities.team[(uint32_t)human_team].records[i].target_x = 0x4000;
    mr->entities.team[(uint32_t)human_team].records[i].target_z = 0x4000;
    mr->entities.team[opp].records[i].pos_x = 0x600;
    mr->entities.team[opp].records[i].pos_z = 0x600;
    mr->entities.team[opp].records[i].target_x = 0x600;
    mr->entities.team[opp].records[i].target_z = 0x600;
  }
  mr->entities.team[opp].records[3].pos_x = 0x180;
  mr->entities.team[opp].records[3].pos_z = 0;
  mr->entities.team[opp].records[3].target_x = 0x180;
  mr->entities.team[opp].records[3].target_z = 0;
  start_distance = 0x180;
  for (int i = 0; i < 60; i++) {
    stage_ball(mr, 0, 0, 0);
    one_granted_frame(mr);
    if (i == 0) assert(mr->entities.team[opp].target == rival);
  }
  assert(mr->entities.team[opp].target == rival);
  {
    struct fifa96_match_entity *e = &mr->entities.team[opp].records[3];
    end_distance = fifa96_entity_distance(mr->entities.ball.x - e->pos_x,
                                          mr->entities.ball.z - e->pos_z);
    assert(e->code == 5u);                 /* row 04/05 carrier record */
  }
  assert(end_distance < start_distance - 0x80);  /* a real approach, not a tick */
  assert(end_distance < 0x80);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  /* The human's selected record/control stay on the human team. */
  assert(mr->entities.controlled >=
         (int32_t)((uint32_t)human_team * FIFA96_MATCH_ENTITY_RECORDS));
  assert(mr->entities.controlled <
         (int32_t)(((uint32_t)human_team + 1u) * FIFA96_MATCH_ENTITY_RECORDS));
  assert(mr->slot.entity >=
         (int32_t)((uint32_t)human_team * FIFA96_MATCH_ENTITY_RECORDS));
  assert(mr->slot.entity <
         (int32_t)(((uint32_t)human_team + 1u) * FIFA96_MATCH_ENTITY_RECORDS));
  assert(mr->entities.team[(uint32_t)human_team].target >=
         (int32_t)((uint32_t)human_team * FIFA96_MATCH_ENTITY_RECORDS));
  assert(mr->entities.team[(uint32_t)human_team].target <
         (int32_t)(((uint32_t)human_team + 1u) * FIFA96_MATCH_ENTITY_RECORDS));
  drop_lc_fixture(&f);
}

/* P8-T1: the far-carrier repair on the opponent row-05 claim. While the ball is
 * a live flight (or loose), an opponent target at ~0x300 cannot set
 * `ball.carrier`; at contact (~0x20) the same record's claim sticks. No
 * score/phase write either way. */
static void test_live_opponent_far_claim_rejected(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human_team = lc_team_for_side(mr, (uint8_t)(mr->phase_machine.side_controlled & 1u));
  uint32_t opp;
  int32_t claimant;
  assert(human_team >= 0);
  opp = 1u - (uint32_t)human_team;
  claimant = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + 5u);
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_hold = 0u;
  mr->live_kick_active = 0u;
  stage_ball(mr, 0, 0, 0);
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    mr->entities.team[(uint32_t)human_team].records[i].pos_x = 0x700;
    mr->entities.team[(uint32_t)human_team].records[i].pos_z = 0x700;
    mr->entities.team[opp].records[i].pos_x = 0x700;
    mr->entities.team[opp].records[i].pos_z = 0x700;
    mr->entities.team[opp].records[i].target_x = 0x700;
    mr->entities.team[opp].records[i].target_z = 0x700;
  }
  {
    struct fifa96_match_entity *e = &mr->entities.team[opp].records[5];
    e->pos_x = 0x300;
    e->pos_z = 0;
    e->target_x = 0x300;
    e->target_z = 0;
    (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
  }
  mr->entities.team[opp].target = claimant;
  mr->live_kick_active = 1u;
  mr->live_kick_shot = 1u;
  mr->live_kick_frames = 500u;
  mr->live_kick_target_x = 0;
  mr->live_kick_target_z = 0xB30;
  one_granted_frame(mr);
  assert(mr->entities.ball.carrier != claimant);
  assert(mr->entities.ball.carrier == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  /* Contact claim sticks: hold the claimant at the ball while row 05 runs. */
  mr->live_kick_active = 0u;
  mr->live_kick_shot = 0u;
  for (int i = 0; i < 4 && mr->entities.ball.carrier != claimant; i++) {
    struct fifa96_match_entity *e = &mr->entities.team[opp].records[5];
    e->pos_x = mr->entities.ball.x + 0x20;
    e->pos_z = mr->entities.ball.z;
    e->target_x = e->pos_x;
    e->target_z = e->pos_z;
    one_granted_frame(mr);
  }
  assert(mr->entities.ball.carrier == claimant);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

/* P8-T2: a defending record inside the block condition stops a live shot. The
 * shipped kickoff shot runs (no ball staging) and a defending outfield record
 * is placed on the ball once the shot reaches the final approach zone. The
 * block clears the flight, pins the ball and records the defender; the score,
 * phase and goal cells are untouched. */
static void test_live_defender_blocks_shot(uint32_t side) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t defending_team;
  int32_t defender_id;
  int32_t saved = 0;
  assert(mr->entities.ball.x == 480 && mr->entities.ball.y == 0 &&
         mr->entities.ball.z == 0);
  if (side != 0u) {
    mr->phase_machine.side_controlled = (uint8_t)side;
    mr->live_human_side = (uint8_t)side;   /* stored human team follows the fixture flip */
    one_granted_frame(mr);
    one_granted_frame(mr);
  }
  {
    int32_t taker = mr->entities.controlled;
    struct fifa96_match_entity *e;
    assert(taker >= (int32_t)(side * FIFA96_MATCH_ENTITY_RECORDS));
    assert(taker < (int32_t)((side + 1u) * FIFA96_MATCH_ENTITY_RECORDS));
    e = &mr->entities.team[side].records[(uint32_t)taker % 11u];
    if (side != 0u) {
      mr->entities.ball.carrier = taker;
      (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
    }
    assert(e->code == 5u);
  }
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
  assert(mr->live_kick_shot == 1u);
  {
    /* The exact staged kick, saved for the repeat-shot recovery check. */
    struct fifa96_ball_pair_state repeat_pair = mr->entities.ball.pair;
    assert((repeat_pair.flags & 0x70u) != 0u);
    defending_team = lc_team_for_side(mr, (uint8_t)(side ^ 1u));
    assert(defending_team >= 0);
    defender_id = defending_team * FIFA96_MATCH_ENTITY_RECORDS + 2;
    for (int i = 0; i < 240 && saved == 0; i++) {
      if (mr->live_kick_active &&
          lc_abs32(mr->entities.ball.z) >= FIFA96_MATCH_LIVE_SAVE_ZONE) {
        struct fifa96_match_entity *d =
            &mr->entities.team[(uint32_t)defending_team].records[2];
        d->pos_x = mr->entities.ball.x;
        d->pos_z = mr->entities.ball.z;
        d->target_x = d->pos_x;
        d->target_z = d->pos_z;
      }
      one_granted_frame(mr);
      if (mr->live_save_hold != 0u) saved = 1;
    }
    assert(saved == 1);
    assert(mr->live_save_record == defender_id);
    assert(mr->live_kick_active == 0u);
    assert(mr->live_kick_shot == 0u);
    assert(mr->goal_armed == 0u);
    assert(mr->score[0] == 0u && mr->score[1] == 0u);
    assert(mr->state.phase == 2u);
    /* The FU-73 launch pair is cleared so it cannot re-latch, and the consumed
     * kick signature is dropped so the exact repeat can. */
    assert(mr->entities.ball.pair.actor == 0);
    assert(mr->entities.ball.pair.receiver == 0);
    assert(mr->entities.ball.pair.traj == 0);
    assert((mr->entities.ball.pair.flags & 0x70u) == 0x20u);
    assert(mr->entities.ball.pair.code == 2u);
    assert(mr->live_kick_sig_actor == FIFA96_MATCH_ENTITY_NONE);
    assert(mr->live_kick_pair_consumed == 0u);
    {
      const struct fifa96_match_entity *d =
          &mr->entities.team[(uint32_t)defending_team].records[2];
      int32_t ddx = mr->entities.ball.x - d->pos_x;
      int32_t ddz = mr->entities.ball.z - d->pos_z;
      assert(fifa96_entity_distance(ddx, ddz) <= FIFA96_MATCH_LIVE_SAVE_RADIUS);
    }
    /* The 30-frame pin: the save frame pinned, then every remaining hold frame
     * keeps the ball on the block position. */
    {
      const int32_t pinned_x = mr->entities.ball.x;
      const int32_t pinned_z = mr->entities.ball.z;
      const uint16_t hold = mr->live_save_hold;
      assert(hold == FIFA96_MATCH_LIVE_SAVE_HOLD - 1u);
      for (uint16_t i = 0; i < hold; i++) {
        one_granted_frame(mr);
        assert(mr->entities.ball.x == pinned_x);
        assert(mr->entities.ball.z == pinned_z);
        assert(mr->score[0] == 0u && mr->score[1] == 0u);
      }
      assert(mr->live_save_hold == 0u);
    }
    for (int i = 0; i < 10; i++) {
      one_granted_frame(mr);
      assert(mr->score[0] == 0u && mr->score[1] == 0u);
      assert(mr->state.phase == 2u);
    }
    /* Repeat the identical kick: the block cleared the pair, the signature and
     * the consumed marker, so the same actor/traj/vector/flags/code must latch
     * again. Move the ball and the defending records out of the save zone first
     * so the new flight is not immediately blocked by fixture leftovers. */
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *d =
          &mr->entities.team[(uint32_t)defending_team].records[i];
      d->pos_x = 0x700;
      d->pos_z = 0x700;
      d->target_x = d->pos_x;
      d->target_z = d->pos_z;
    }
    stage_ball(mr, 480, 0, 0);
    mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
    mr->live_kick_active = 0u;
    mr->live_kick_shot = 0u;
    mr->live_save_hold = 0u;
    mr->entities.controlled = repeat_pair.actor;
    mr->entities.ball.pair = repeat_pair;
    one_granted_frame(mr);
    assert(mr->live_kick_active == 1u);
    assert(mr->live_kick_shot == 1u);
    assert(mr->score[0] == 0u && mr->score[1] == 0u);
    assert(mr->state.phase == 2u);
  }
  drop_lc_fixture(&f);
}

/* P8 (FU-346) helpers: park a team's outfield records (all visible) far from
 * the ball and stage a live KICK flight at a chosen ball z. Fixture staging
 * only; no score/phase/goal-cell write. */
static void lc_park_outfield(struct fifa96_match_run *mr, int32_t team,
                             int32_t x, int32_t z, uint8_t skip_9a) {
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &mr->entities.team[(uint32_t)team].records[i];
    e->pos_x = x;
    e->pos_z = z;
    e->target_x = x;
    e->target_z = z;
    e->skip_98 = 0u;
    e->skip_9a = skip_9a;
  }
}

static void lc_stage_live_shot(struct fifa96_match_run *mr, int32_t ball_x,
                               int32_t ball_z) {
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_hold = 0u;
  mr->live_kick_active = 1u;
  mr->live_kick_shot = 1u;
  mr->live_kick_frames = 600u;
  mr->live_kick_target_x = 0;
  mr->live_kick_target_z = 0xB30;
  mr->goal_armed = 0u;
  mr->live_save_hold = 0u;
  mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  stage_ball(mr, ball_x, 0, ball_z);
}

/* P8 (FU-346) flight-end signature: a completed flight clears the value
 * signature but keeps the consumed-pair marker, so the still-staged FU-73 word
 * cannot relaunch by itself (the goal/restart phantom-kick guard). A genuine
 * new dispatch clears the marker, and the identical pair then latches again.
 * The block path clears both and is covered by the block test above. */
static void test_live_flight_end_stale_pair_guard(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t actor = mr->entities.controlled;
  assert(actor >= 0 && actor != 0);
  {
    fifa96_platform_key kick = {FIFA96_ENGINE_KEY_KICK, 1};
    assert(fifa96_match_run_input(mr, &kick, 1) == 0);
  }
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert(mr->live_kick_active == 1u);
  assert(mr->entities.ball.pair.actor == actor);
  assert(mr->live_kick_pair_consumed == 1u);

  /* End the flight through the bounded window (no goal/phase change). */
  mr->live_kick_frames = 1u;
  one_granted_frame(mr);
  assert(mr->live_kick_active == 0u);
  assert(mr->live_kick_shot == 0u);
  assert(mr->live_kick_sig_actor == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->live_kick_pair_consumed == 1u);

  /* The same staged pair, with no new dispatch, must not relaunch. */
  stage_ball(mr, 480, 0, 0);
  mr->entities.controlled = actor;
  mr->slot.entity = actor;
  mr->live_kick_active = 0u;
  one_granted_frame(mr);
  assert(mr->live_kick_active == 0u);

  /* A new dispatch clears the consumed marker (match_kick_run); the identical
   * pair then latches a fresh flight. */
  mr->entities.controlled = actor;
  mr->slot.entity = actor;
  mr->live_kick_pair_consumed = 0u;
  one_granted_frame(mr);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_shot == 1u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

/* P8 (FU-346) mouth edge: the block must leave the goal mouth to the native
 * armer/scan. A ball just inside `fifa96_match_goal_zone` with a defender on it
 * arms and scores instead of recording a save; the same ball one unit outside
 * the classifier is still blockable. */
static void test_live_shot_block_mouth_edge(void) {
  int32_t team;
  {
    struct lc_fixture f = make_lc_fixture();
    struct fifa96_match_run *mr = f.mr;
    team = lc_team_for_side(mr, 1u);
    assert(team >= 0);
    /* Just inside: |z| = 0xB40 is in 0xB10..0xB90, |x| = 0 < 0xD0, y = 0. */
    assert(fifa96_match_goal_zone(0, 0, 0xB40));
    lc_park_outfield(mr, team, 0x700, 0x700, 0u);
    {
      struct fifa96_match_entity *d = &mr->entities.team[(uint32_t)team].records[2];
      d->pos_x = 0;
      d->pos_z = 0xB40;
      d->target_x = 0;
      d->target_z = 0xB40;
    }
    lc_stage_live_shot(mr, 0, 0xB40);
    one_granted_frame(mr);
    assert(mr->live_save_hold == 0u);
    assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
    assert(mr->goal_armed == 1u);
    for (int i = 0; i < 8 && mr->score[0] == 0u; i++) one_granted_frame(mr);
    assert(mr->score[0] == 1u && mr->score[1] == 0u);
    assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
    assert(mr->live_save_hold == 0u);
    drop_lc_fixture(&f);
  }
  {
    struct lc_fixture f = make_lc_fixture();
    struct fifa96_match_run *mr = f.mr;
    team = lc_team_for_side(mr, 1u);
    assert(team >= 0);
    /* Just outside: 0xB0F is below the 0xB10 classifier bound. */
    assert(!fifa96_match_goal_zone(0, 0, 0xB0F));
    lc_park_outfield(mr, team, 0x700, 0x700, 0u);
    {
      struct fifa96_match_entity *d = &mr->entities.team[(uint32_t)team].records[2];
      d->pos_x = 0;
      d->pos_z = 0xB0F;
      d->target_x = 0;
      d->target_z = 0xB0F;
    }
    lc_stage_live_shot(mr, 0, 0xB0F);
    one_granted_frame(mr);
    assert(mr->live_save_hold != 0u);
    assert(mr->live_save_record ==
           (int32_t)((uint32_t)team * FIFA96_MATCH_ENTITY_RECORDS + 2u));
    assert(mr->score[0] == 0u && mr->score[1] == 0u);
    assert(mr->goal_armed == 0u);
    drop_lc_fixture(&f);
  }
}

/* P8 (FU-346) hidden records: the block scan uses the native +0x98/+0x9A
 * exclusions. A hidden defender sitting on the ball must not block; the same
 * record with the exclusion cleared does (and a visible record outside the
 * radius never blocks). */
static void test_live_shot_block_skips_hidden_defender(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t team = lc_team_for_side(mr, 1u);
  int32_t defender_id = team * FIFA96_MATCH_ENTITY_RECORDS + 2;
  assert(team >= 0);
  lc_park_outfield(mr, team, 0x700, 0x700, 0u);
  {
    struct fifa96_match_entity *d = &mr->entities.team[(uint32_t)team].records[2];
    d->pos_x = 0;
    d->pos_z = 0xA00;
    d->target_x = 0;
    d->target_z = 0xA00;
    d->skip_9a = 1u;                 /* hidden/occupied record on the ball */
  }
  {
    struct fifa96_match_entity *d = &mr->entities.team[(uint32_t)team].records[3];
    d->pos_x = 0;
    d->pos_z = 0xA00 + FIFA96_MATCH_LIVE_SAVE_RADIUS + 8;
    d->target_x = d->pos_x;
    d->target_z = d->pos_z;
  }
  lc_stage_live_shot(mr, 0, 0xA00);
  one_granted_frame(mr);
  assert(mr->live_save_hold == 0u);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_shot == 1u);
  /* Un-hide the same record: it is now the nearest and blocks. */
  {
    struct fifa96_match_entity *d = &mr->entities.team[(uint32_t)team].records[2];
    d->pos_x = 0;
    d->pos_z = 0xA00;
    d->target_x = 0;
    d->target_z = 0xA00;
    d->skip_9a = 0u;
  }
  lc_stage_live_shot(mr, 0, 0xA00);
  one_granted_frame(mr);
  assert(mr->live_save_hold != 0u);
  assert(mr->live_save_record == defender_id);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

static void test_live_defender_blocks_both_sides(void) {
  test_live_defender_blocks_shot(0u);
  test_live_defender_blocks_shot(1u);
}

/* P8-T2 beatability with P9 aim: the shipped kickoff path plus an Up aim
 * (+0x80) beats a keeper pinned at the real goal spot, and the nearest
 * defending outfield record in the final approach zone stays outside the P8
 * block radius, so the native armer/scan still scores 1-0 with no save. This
 * is the same no-staging shipped path as test_live_kickoff_shot_scores, with
 * both the block and keeper conditions measured. */
static void test_live_shot_still_scores_wide(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t min_zone_distance = 0x7FFF;
  int32_t keeper_team = lc_team_for_side(mr, 1u);
  int zone_frames = 0;
  int goal = 0;
  assert(keeper_team >= 0);
  lc_press_kick(mr, FIFA96_ENGINE_KEY_UP);
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_x == FIFA96_MATCH_LIVE_SHOT_AIM_X);
  {
    int32_t defending_team = lc_team_for_side(mr, 1u);
    assert(defending_team >= 0);
    for (int i = 0; i < 500 && goal == 0; i++) {
      lc_pin_keeper(mr, keeper_team, 0, lc_keeper_goal_z(1u));
      if (mr->live_kick_active != 0u &&
          lc_abs32(mr->entities.ball.z) >= FIFA96_MATCH_LIVE_SAVE_ZONE) {
        zone_frames++;
        /* Measure the same 1..10 outfield set the P8 block searches. */
        for (uint32_t r = 1; r < FIFA96_MATCH_ENTITY_RECORDS; r++) {
          const struct fifa96_match_entity *e =
              &mr->entities.team[(uint32_t)defending_team].records[r];
          int32_t ddx = mr->entities.ball.x - e->pos_x;
          int32_t ddz = mr->entities.ball.z - e->pos_z;
          int32_t d = fifa96_entity_distance(ddx, ddz);
          if (d < min_zone_distance) min_zone_distance = d;
        }
      }
      one_granted_frame(mr);
      if (mr->score[0] == 1u) goal = 1;
    }
  }
  assert(goal == 1);
  assert(zone_frames > 0);
  assert(min_zone_distance > FIFA96_MATCH_LIVE_SAVE_RADIUS);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->live_save_hold == 0u);
  assert(mr->score[0] == 1u && mr->score[1] == 0u);
  assert(mr->state.phase == 5u);
  drop_lc_fixture(&f);
}

/* P8-T2 gate: a PASS flight must not trigger the shot block even with a
 * defending record on the ball in the final zone (the KICK-row latch sets
 * live_kick_shot; PASS/LONG_BALL do not). */
static void test_live_shot_block_ignores_pass(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = mr->entities.controlled;
  assert(human >= 0 && human < 11);
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      if (t == 0u && (int32_t)i == human % (int32_t)FIFA96_MATCH_ENTITY_RECORDS)
        continue;
      mr->entities.team[t].records[i].skip_9a = 1;
    }
  mr->slot.entity = human;
  mr->entities.controlled = human;
  mr->entities.team[0].target = human;
  mr->entities.ball.pair.actor = human;
  mr->entities.ball.pair.flags = 0x20u;       /* PASS */
  mr->entities.ball.pair.code = 6u;
  mr->entities.ball.pair.traj = 777;
  mr->entities.ball.pair.vector.x = 720;
  mr->entities.ball.pair.vector.height = 100;
  mr->entities.ball.pair.vector.z = 100;
  one_granted_frame(mr);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_shot == 0u);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
  /* Stage the whole block condition except the shot marker. */
  {
    struct fifa96_match_entity *d = &mr->entities.team[1].records[2];
    d->skip_9a = 0;
    d->pos_x = 0;
    d->pos_z = 0xA00;
    d->target_x = 0;
    d->target_z = 0xA00;
  }
  mr->entities.ball.x = 0;
  mr->entities.ball.z = 0xA00;
  mr->live_kick_active = 1u;
  mr->live_kick_shot = 0u;
  one_granted_frame(mr);
  assert(mr->live_save_hold == 0u);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

/* P9-fix2 (FU-347) aim: the live view maps world x to the goal-mouth
 * near/far axis, and native movement maps Up (0x01) -> +x / Down (0x02) -> -x.
 * The shot aim therefore uses Up -> +0x80, Down -> -0x80, none/both -> 0;
 * Left/Right are the goal-side z movement axis and must not aim. */
static int32_t lc_aim_target(int key1, int key2, int preheld) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  fifa96_platform_key keys[3] = {
    {FIFA96_ENGINE_KEY_KICK, 1}, {0, 1}, {0, 1}
  };
  fifa96_platform_key pre[3] = { {0, 1}, {0, 1}, {0, 1} };
  size_t pre_count = 0;
  size_t count = 0;
  int32_t target;
  if (preheld != 0) {
    if (key1 != 0) pre[pre_count++].raw_code = key1;
    if (key2 != 0) pre[pre_count++].raw_code = key2;
    assert(fifa96_match_run_input(mr, pre, pre_count) == 0);
    one_granted_frame(mr);
  }
  keys[count++] = (fifa96_platform_key){FIFA96_ENGINE_KEY_KICK, 1};
  if (key1 != 0) keys[count++] = (fifa96_platform_key){key1, 1};
  if (key2 != 0) keys[count++] = (fifa96_platform_key){key2, 1};
  assert(fifa96_match_run_input(mr, keys, count) == 0);
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_z == 0xB30);
  target = mr->live_kick_target_x;
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
  return target;
}

static void test_live_aim_depth_axis(void) {
  assert(lc_aim_target(FIFA96_ENGINE_KEY_UP, 0, 0) ==
         FIFA96_MATCH_LIVE_SHOT_AIM_X);
  assert(lc_aim_target(FIFA96_ENGINE_KEY_DOWN, 0, 0) ==
         -FIFA96_MATCH_LIVE_SHOT_AIM_X);
  assert(lc_aim_target(FIFA96_ENGINE_KEY_UP, FIFA96_ENGINE_KEY_DOWN, 0) == 0);
  /* Left/Right move along the goal-side z axis; they are not aim inputs and
   * must not leave a stale ±0x80. */
  assert(lc_aim_target(FIFA96_ENGINE_KEY_LEFT, 0, 0) == 0);
  assert(lc_aim_target(FIFA96_ENGINE_KEY_RIGHT, 0, 0) == 0);
  /* Both depth keys held before the KICK press resolve to the centre. */
  assert(lc_aim_target(FIFA96_ENGINE_KEY_UP, FIFA96_ENGINE_KEY_DOWN, 1) == 0);
  assert(lc_aim_target(FIFA96_ENGINE_KEY_LEFT, FIFA96_ENGINE_KEY_RIGHT, 1) == 0);
}

/* P9 (FU-347) keeper save: the real kickoff centre shot against a keeper
 * pinned at its shipped ISO goal spot. The P8 outfield block does not fire
 * (the shipped centre path's nearest outfield is far outside 0x28), so the
 * P9 keeper save is the only stopper: no score/phase/goal write, the ball is
 * pinned by the existing hold, and `live_save_record` is the defending
 * record 0. Both sides run the same path. */
static void test_live_keeper_saves_centre_side(uint32_t side) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t def_team;
  uint8_t def_side;
  int32_t keeper_id;
  int32_t saved = 0;
  if (side != 0u) {
    mr->phase_machine.side_controlled = (uint8_t)side;
    mr->live_human_side = (uint8_t)side;   /* stored human team follows the fixture flip */
    one_granted_frame(mr);
    one_granted_frame(mr);
  }
  lc_sides(mr, (uint8_t)side, &def_team, &def_side);
  keeper_id = def_team * FIFA96_MATCH_ENTITY_RECORDS;
  if (side != 0u) {
    int32_t taker = mr->entities.controlled;
    struct fifa96_match_entity *e =
        &mr->entities.team[side].records[(uint32_t)taker % 11u];
    assert(taker >= (int32_t)(side * FIFA96_MATCH_ENTITY_RECORDS));
    assert(taker < (int32_t)((side + 1u) * FIFA96_MATCH_ENTITY_RECORDS));
    mr->entities.ball.carrier = taker;
    (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
  }
  lc_press_kick(mr, 0);                    /* centre shot: no lateral aim */
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_shot == 1u);
  assert(mr->live_kick_target_x == 0);
  for (int i = 0; i < 500 && saved == 0; i++) {
    lc_pin_keeper(mr, def_team, 0, lc_keeper_goal_z(def_side));
    one_granted_frame(mr);
    if (mr->live_save_hold != 0u) saved = 1;
  }
  assert(saved == 1);
  assert(mr->live_save_record == keeper_id);
  assert(mr->entities.ball.carrier == keeper_id);
  assert(mr->live_aim_goal_x == 0);
  assert(mr->live_kick_active == 0u);
  assert(mr->live_kick_shot == 0u);
  assert(mr->goal_armed == 0u);
  assert(mr->goal_zone == 0u);
  assert(mr->score[side] == 0u && mr->score[side ^ 1u] == 0u);
  assert(mr->state.phase == 2u);
  {
    const struct fifa96_match_entity *k = &mr->entities.team[(uint32_t)def_team]
                                               .records[0];
    int32_t dx = mr->entities.ball.x - k->pos_x;
    int32_t dz = mr->entities.ball.z - k->pos_z;
    assert(fifa96_entity_distance(dx, dz) <=
           FIFA96_MATCH_LIVE_KEEPER_SAVE_NEAR);
  }
  /* The ball-machine hold pins the ball on the save position for the hold
   * window; no score/phase write while it drains. */
  {
    const int32_t px = mr->entities.ball.x;
    const int32_t pz = mr->entities.ball.z;
    uint16_t hold = mr->live_save_hold;
    assert(hold == FIFA96_MATCH_LIVE_SAVE_HOLD - 1u);
    for (uint16_t i = 0; i < hold; i++) {
      one_granted_frame(mr);
      assert(mr->entities.ball.x == px);
      assert(mr->entities.ball.z == pz);
      assert(mr->score[side] == 0u && mr->score[side ^ 1u] == 0u);
      assert(mr->state.phase == 2u);
    }
    assert(mr->live_save_hold == 0u);
  }
  /* P14: the keeper outlet releases the saved ball to a teammate through the
   * same live flight rather than leaving it pinned. */
  one_granted_frame(mr);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_shot == 0u);
  assert(mr->score[side] == 0u && mr->score[side ^ 1u] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

static void test_live_keeper_saves_centre(void) {
  test_live_keeper_saves_centre_side(0u);
  test_live_keeper_saves_centre_side(1u);
}

/* P9 (FU-347) no-staging keeper save: the real side-0 kickoff, no ball or
 * taker staging, and the keeper placed at the probed ISO goal spot once before
 * the kick. The live AI then runs untouched for the whole flight (no per-frame
 * re-pin); the centre shot is still saved by record 0, with no score/phase
 * write. Side 1 keeps the per-frame fixture in
 * `test_live_keeper_saves_centre` because its kickoff needs the fixture
 * controlled-side switch. */
static void test_live_keeper_saves_centre_no_staging(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t def_team;
  uint8_t def_side;
  int32_t keeper_id;
  int32_t saved = 0;
  assert(mr->entities.ball.x == 480 && mr->entities.ball.y == 0 &&
         mr->entities.ball.z == 0);
  lc_sides(mr, 0u, &def_team, &def_side);
  keeper_id = def_team * FIFA96_MATCH_ENTITY_RECORDS;
  /* One-time fixture placement only: the keeper's probed goal spot. */
  lc_pin_keeper(mr, def_team, 0, lc_keeper_goal_z(def_side));
  lc_press_kick(mr, 0);                    /* centre shot: no lateral aim */
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_shot == 1u);
  assert(mr->live_kick_target_x == 0);
  for (int i = 0; i < 500 && saved == 0; i++) {
    one_granted_frame(mr);                 /* no keeper re-pin here */
    if (mr->live_save_hold != 0u) saved = 1;
  }
  assert(saved == 1);
  assert(mr->live_save_record == keeper_id);
  assert(mr->entities.ball.carrier == keeper_id);
  assert(mr->live_aim_goal_x == 0);
  assert(mr->live_kick_active == 0u);
  assert(mr->live_kick_shot == 0u);
  assert(mr->goal_armed == 0u);
  assert(mr->goal_zone == 0u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  {
    const struct fifa96_match_entity *k =
        &mr->entities.team[(uint32_t)def_team].records[0];
    int32_t dx = mr->entities.ball.x - k->pos_x;
    int32_t dz = mr->entities.ball.z - k->pos_z;
    assert(fifa96_entity_distance(dx, dz) <=
           FIFA96_MATCH_LIVE_KEEPER_SAVE_NEAR);
  }
  drop_lc_fixture(&f);
}

/* P9 (FU-347) wide-aim goal: the same real kickoff centre path but with a held
 * depth key, against a keeper pinned at the shipped goal spot. The aim is
 * 0x80 from the keeper's x (outside `..._KEEPER_SAVE_REACH`), so the native
 * armer/scan scores and no save is recorded. Both sides and both directions
 * are covered across this test and `test_live_kickoff_shot_scores_side`. */
static void test_live_keeper_beaten_by_aim_side(uint32_t side, int32_t aim_key,
                                                int32_t aim_x) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t def_team;
  uint8_t def_side;
  int goal = 0;
  if (side != 0u) {
    mr->phase_machine.side_controlled = (uint8_t)side;
    mr->live_human_side = (uint8_t)side;   /* stored human team follows the fixture flip */
    one_granted_frame(mr);
    one_granted_frame(mr);
  }
  lc_sides(mr, (uint8_t)side, &def_team, &def_side);
  if (side != 0u) {
    int32_t taker = mr->entities.controlled;
    struct fifa96_match_entity *e =
        &mr->entities.team[side].records[(uint32_t)taker % 11u];
    mr->entities.ball.carrier = taker;
    (void)fifa96_match_entities_install(e, (uint8_t)mr->state.phase, 5u, 0);
  }
  lc_press_kick(mr, aim_key);
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_shot == 1u);
  assert(mr->live_kick_target_x == aim_x);
  for (int i = 0; i < 500 && goal == 0; i++) {
    lc_pin_keeper(mr, def_team, 0, lc_keeper_goal_z(def_side));
    one_granted_frame(mr);
    if (mr->score[side] == 1u) goal = 1;
  }
  assert(goal == 1);
  assert(mr->score[side] == 1u && mr->score[side ^ 1u] == 0u);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->live_save_hold == 0u);
  assert(mr->goal_armed == 1u && mr->goal_zone == 1u);
  assert(mr->state.phase == 5u);
  drop_lc_fixture(&f);
}

static void test_live_keeper_beaten_by_aim(void) {
  /* Opposite directions to the kickoff-goal test above: all four combos run. */
  test_live_keeper_beaten_by_aim_side(0u, FIFA96_ENGINE_KEY_DOWN,
                                      -FIFA96_MATCH_LIVE_SHOT_AIM_X);
  test_live_keeper_beaten_by_aim_side(1u, FIFA96_ENGINE_KEY_UP,
                                      FIFA96_MATCH_LIVE_SHOT_AIM_X);
}

/* P9 (FU-347) displaced keeper: a centre shot against a keeper held at
 * x=0x400 (outside the 0x40 reach) is not saved and the native chain scores.
 * Fixture staging moves only the defender record; the ball and score are the
 * real kickoff path. */
static void test_live_keeper_displaced_centre_goal(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t def_team;
  uint8_t def_side;
  int goal = 0;
  lc_sides(mr, 0u, &def_team, &def_side);
  lc_press_kick(mr, 0);
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_x == 0);
  for (int i = 0; i < 500 && goal == 0; i++) {
    lc_pin_keeper(mr, def_team, 0x400, lc_keeper_goal_z(def_side));
    one_granted_frame(mr);
    if (mr->score[0] == 1u) goal = 1;
  }
  assert(goal == 1);
  assert(mr->score[0] == 1u && mr->score[1] == 0u);
  assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->live_save_hold == 0u);
  assert(mr->state.phase == 5u);
  drop_lc_fixture(&f);
}

/* P9-fix2 (FU-347) goal-side gate: a keeper is only allowed to save a ball
 * that has not already passed it. The +z keeper at 0x9EA saves a ball at
 * 0x9C0, but a ball at 0x9F0 or 0xA40 is already behind it and reaches the
 * native goal. The -z mirror (keeper 0 at -0x9EA, ball -0x9C0/-0x9F0/-0xA40)
 * runs the same rule. */
static void lc_keeper_goal_side_case(uint8_t defending_side,
                                     int32_t ball_abs_z,
                                     int32_t keeper_abs_z, int expect_save) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t team = lc_team_for_side(mr, defending_side);
  int32_t sign = defending_side == 1u ? 1 : -1;
  int32_t score_side = (int32_t)(defending_side ^ 1u);
  int goal = 0;
  assert(team >= 0);
  lc_park_outfield(mr, team, 0x700, 0x700, 0u);
  lc_stage_live_shot(mr, 0, sign * ball_abs_z);
  mr->live_kick_target_z = sign * 0xB30;
  lc_pin_keeper(mr, team, 0, sign * keeper_abs_z);
  one_granted_frame(mr);
  if (expect_save) {
    assert(mr->live_save_hold != 0u);
    assert(mr->live_save_record ==
           team * (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
    assert(mr->entities.ball.carrier ==
           team * (int32_t)FIFA96_MATCH_ENTITY_RECORDS);
    assert(mr->score[0] == 0u && mr->score[1] == 0u);
    assert(mr->goal_armed == 0u);
    assert(mr->state.phase == 2u);
  } else {
    assert(mr->live_save_hold == 0u);
    assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
    assert(mr->live_kick_active == 1u);   /* the flight was not consumed */
    assert(mr->score[0] == 0u && mr->score[1] == 0u);
    /* The native armer/scan still converts a mouth ball with this keeper
     * behind it (the mouth gate owns that ball; see test_live_keeper_mouth_edge
     * for the isolated gate). Stage the ball just inside the mouth and let the
     * native chain score. */
    lc_stage_live_shot(mr, 0, sign * 0xB40);
    mr->live_kick_target_z = sign * 0xB30;
    for (int i = 0; i < 20 && goal == 0; i++) {
      lc_pin_keeper(mr, team, 0, sign * keeper_abs_z);
      one_granted_frame(mr);
      if (mr->score[score_side] == 1u) goal = 1;
    }
    assert(goal == 1);
    assert(mr->score[score_side] == 1u);
    assert(mr->score[defending_side] == 0u);
    assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
    assert(mr->live_save_hold == 0u);
  }
  drop_lc_fixture(&f);
}

static void test_live_keeper_goal_side(void) {
  /* +z goal: team 1's keeper. Before/at it saves; already past it does not. */
  lc_keeper_goal_side_case(1u, 0x9C0, 0x9EA, 1);
  lc_keeper_goal_side_case(1u, 0x9F0, 0x9EA, 0);
  lc_keeper_goal_side_case(1u, 0xA40, 0x9EA, 0);
  /* -z mirror: team 0's keeper. */
  lc_keeper_goal_side_case(0u, 0x9C0, 0x9EA, 1);
  lc_keeper_goal_side_case(0u, 0x9F0, 0x9EA, 0);
  lc_keeper_goal_side_case(0u, 0xA40, 0x9EA, 0);
}

/* P9-fix2 (FU-347) keeper mouth edge. A ball already inside the native
 * `FUN_00070074` mouth is left to the armer even with a near keeper; and the
 * mouth gate is isolated by a keeper staged on the ball at the mouth (the
 * goal-side gate passes there, so only the mouth early-out prevents a save). */
static void test_live_keeper_mouth_edge(void) {
  {
    struct lc_fixture f = make_lc_fixture();
    struct fifa96_match_run *mr = f.mr;
    int32_t team = lc_team_for_side(mr, 1u);
    assert(team >= 0);
    assert(fifa96_match_goal_zone(0, 0, 0xB40));
    lc_park_outfield(mr, team, 0x700, 0x700, 0u);
    lc_stage_live_shot(mr, 0, 0xB40);
    lc_pin_keeper(mr, team, 0, 0x9EA);
    one_granted_frame(mr);
    assert(mr->live_save_hold == 0u);
    assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
    assert(mr->goal_armed == 1u);
    for (int i = 0; i < 8 && mr->score[0] == 0u; i++) one_granted_frame(mr);
    assert(mr->score[0] == 1u && mr->score[1] == 0u);
    drop_lc_fixture(&f);
  }
  {
    struct lc_fixture f = make_lc_fixture();
    struct fifa96_match_run *mr = f.mr;
    int32_t team = lc_team_for_side(mr, 1u);
    assert(team >= 0);
    lc_park_outfield(mr, team, 0x700, 0x700, 0u);
    lc_stage_live_shot(mr, 0, 0xB40);
    /* Keeper just goal-side of the ball at the mouth (0xB60 >= 0xB40), so
     * the goal-side gate passes; the mouth gate is the only early-out. */
    lc_pin_keeper(mr, team, 0, 0xB60);
    one_granted_frame(mr);
    assert(mr->live_save_hold == 0u);
    assert(mr->live_save_record == FIFA96_MATCH_ENTITY_NONE);
    for (int i = 0; i < 8 && mr->score[0] == 0u; i++) one_granted_frame(mr);
    assert(mr->score[0] == 1u && mr->score[1] == 0u);
    drop_lc_fixture(&f);
  }
}

/* P9-fix2 (FU-347) stale-aim two-kick case: the first kick is aimed Up
 * (+0x80); after the flight is stopped, the second KICK is pressed with Left
 * held (a z-axis movement key, not an aim key) and must go to the centre, not
 * inherit the first aim. */
static void test_live_stale_aim_two_kicks(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t actor = mr->entities.controlled;
  assert(actor >= 0 && actor < 11);
  lc_press_kick(mr, FIFA96_ENGINE_KEY_UP);
  one_granted_frame(mr);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 8 && (mr->dispatched_ok & (1ull << 7u)) == 0ull; i++)
    one_granted_frame(mr);
  assert((mr->dispatched_ok & (1ull << 7u)) != 0ull);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_x == FIFA96_MATCH_LIVE_SHOT_AIM_X);

  /* Stop the first flight (fixture state, not a score/phase write), hide the
   * other own outfield records so the follow stays on the taker, and restore
   * the kickoff ball/boot state for the second kick. */
  mr->live_kick_active = 0u;
  mr->live_kick_shot = 0u;
  mr->live_kick_frames = 0u;
  mr->live_kick_hold = 0u;
  mr->live_save_hold = 0u;
  mr->live_save_record = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    if ((int32_t)i == actor) continue;
    mr->entities.team[0].records[i].skip_9a = 1u;
  }
  mr->slot.entity = actor;
  mr->entities.controlled = actor;
  mr->entities.team[0].target = actor;
  mr->entities.ball.carrier = actor;
  (void)fifa96_match_entities_install(&mr->entities.team[0].records[actor],
                                      (uint8_t)mr->state.phase, 5u, 0);
  stage_ball(mr, 480, 0, 0);
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  assert(mr->entities.team[0].records[actor].code == 5u);

  lc_press_kick(mr, FIFA96_ENGINE_KEY_LEFT);   /* Left is z movement, not aim */
  one_granted_frame(mr);
  /* Re-assert the carrier state after the press frame (the live follow clears
   * it when the old pair is still live); the release frame then runs row 07
   * with a valid carrier. */
  (void)fifa96_match_entities_install(&mr->entities.team[0].records[actor],
                                      (uint8_t)mr->state.phase, 5u, 0);
  mr->entities.ball.carrier = actor;
  assert(fifa96_match_run_input(mr, NULL, 0) == 0);
  for (int i = 0; i < 12 && mr->live_kick_active == 0u; i++)
    one_granted_frame(mr);
  assert(mr->live_kick_active == 1u);
  assert(mr->live_kick_target_x == 0);
  assert(mr->live_aim_goal_x == 0);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

/* P14 live CPU carrier action: after a short hold, the opponent carrier
 * launches one pass/shot through the existing flight primitive. The human is
 * parked out of range; score and phase stay untouched. */
static void test_live_cpu_carrier_acts(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = lc_team_for_side(mr, (uint8_t)(mr->phase_machine.side_controlled & 1u));
  uint32_t opp;
  int32_t rival;
  assert(human >= 0);
  opp = 1u - (uint32_t)human;
  rival = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + 3u);
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *h = &mr->entities.team[(uint32_t)human].records[i];
    struct fifa96_match_entity *o = &mr->entities.team[opp].records[i];
    h->pos_x = 0x4000;
    h->pos_z = 0x4000;
    h->target_x = h->pos_x;
    h->target_z = h->pos_z;
    if ((int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + i) == rival) {
      o->pos_x = 0;
      o->pos_z = 0x600;
    } else {
      o->pos_x = 0x900;
      o->pos_z = 0x900;
    }
    o->target_x = o->pos_x;
    o->target_z = o->pos_z;
  }
  stage_ball(mr, 0, 0, 0x600);
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 0u;
  mr->live_save_hold = 0u;
  mr->entities.ball.carrier = rival;
  mr->entities.team[opp].target = rival;
  mr->live_cpu_hold = 0;
  mr->live_cpu_actions = 0;
  mr->live_cpu_carrier = FIFA96_MATCH_ENTITY_NONE;
  /* This fixture pins the derived CPU action, not the referee: keep the
   * opponent's contact clean so the tackle/claim path still hands it the ball
   * (Phase 70 wires the live contact decision into that same path). */
  mr->config.field_4c306 = 0;
  for (int i = 0; i < 48; i++) one_granted_frame(mr);
  assert(mr->live_cpu_actions >= 1u);
  assert(mr->entities.ball.z < 0x600);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  assert(mr->entities.controlled >= 0 && mr->entities.controlled < 11);
  drop_lc_fixture(&f);
}

/* P14 derived opponent contact tackle: the defending side can win a stationary
 * human ball; the cooldown keeps the human from instantly reclaiming. */
static void test_live_ai_tackles_human(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = lc_team_for_side(mr, (uint8_t)(mr->phase_machine.side_controlled & 1u));
  uint32_t opp;
  int32_t carrier;
  int32_t rival;
  assert(human >= 0);
  opp = 1u - (uint32_t)human;
  carrier = mr->entities.controlled;
  assert(carrier > 0 && carrier < 11);
  rival = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + 3u);
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *h = &mr->entities.team[(uint32_t)human].records[i];
    struct fifa96_match_entity *o = &mr->entities.team[opp].records[i];
    if ((int32_t)i != carrier) {
      h->pos_x = 0x3000;
      h->pos_z = 0x3000;
    } else {
      h->pos_x = 0;
      h->pos_z = 0;
    }
    h->target_x = h->pos_x;
    h->target_z = h->pos_z;
    if ((int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + i) == rival) {
      o->pos_x = 0x20;
      o->pos_z = 0;
    } else {
      o->pos_x = 0x900;
      o->pos_z = 0x900;
    }
    o->target_x = o->pos_x;
    o->target_z = o->pos_z;
  }
  stage_ball(mr, 0, 0, 0);
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 0u;
  mr->live_save_hold = 0u;
  mr->live_ai_tackle_cd = 0u;
  mr->entities.ball.carrier = carrier;
  /* Possession-only fixture: the live contact decision is tested separately in
   * test_live_ai_tackle_foul_reaches_free_kick. */
  mr->config.field_4c306 = 0;
  for (int i = 0; i < 4; i++) one_granted_frame(mr);
  assert(mr->entities.ball.carrier == rival);
  assert(mr->live_ai_tackle_cd != 0u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

/* Phase 70 live referee: the opponent contact now runs the native foul
 * decision, the act-2 stage-3 hold is released by the ordinary ball-settle
 * pass, and the free-kick taker arms. */
static void test_live_ai_tackle_foul_reaches_free_kick(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = lc_team_for_side(mr, (uint8_t)(mr->phase_machine.side_controlled & 1u));
  uint32_t opp;
  int32_t carrier;
  int32_t rival;
  uint16_t value;
  int observed_hold = 0;
  assert(human >= 0);
  opp = 1u - (uint32_t)human;
  carrier = mr->entities.controlled;
  assert(carrier > 0 && carrier < 11);
  rival = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + 3u);
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *h = &mr->entities.team[(uint32_t)human].records[i];
    struct fifa96_match_entity *o = &mr->entities.team[opp].records[i];
    if ((int32_t)i != carrier) {
      h->pos_x = 0x3000;
      h->pos_z = 0x3000;
    } else {
      h->pos_x = 0;
      h->pos_z = 0;
    }
    h->target_x = h->pos_x;
    h->target_z = h->pos_z;
    if ((int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + i) == rival) {
      o->pos_x = 0x20;
      o->pos_z = 0;
    } else {
      o->pos_x = 0x900;
      o->pos_z = 0x900;
    }
    o->target_x = o->pos_x;
    o->target_z = o->pos_z;
  }
  stage_ball(mr, 0, 0, 0);
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 0u;
  mr->live_save_hold = 0u;
  mr->live_ai_tackle_cd = 0u;
  mr->entities.ball.carrier = carrier;
  /* 1) The live contact hook registers the duel. With fouls Off the native
   * decision gate skips and the clean tackle still wins the ball. */
  mr->config.field_4c306 = 0;
  one_granted_frame(mr);
  assert(mr->referee.contact_kind == 2u);
  assert(mr->referee.rec_first == rival);
  assert(mr->referee.rec_second == carrier);
  assert(mr->referee.recall_consumed == 0u);
  assert(mr->act.id == 0u);
  assert(mr->state.phase == 2u);
  assert(mr->entities.ball.carrier == rival);

  /* 2) The full live referee flow. The direct contact seam uses the exact
   * native decision inputs; seed 0 + three draws passes the 1-in-8 skip and
   * draw 5 (0x584D & 0x3F = 0x0D) selects foul kind 1. */
  mr->config.field_4c306 = 2;
  mr->config.field_4c30a = 1;
  mr->entities.ball.carrier = carrier;
  mr->act.id = 0u;
  mr->referee.sequence = FIFA96_REF_SEQ_NONE;
  mr->state.phase = 2u;
  (void)fifa96_rng_seed(&mr->rng, 0);
  for (int i = 0; i < 3; i++) (void)fifa96_rng_step(&mr->rng, &value);
  {
    const int32_t point[3] = {0, 0, 0};
    assert(fifa96_match_run_contact(mr, 2u, rival, carrier, point) == 1);
  }
  assert(mr->referee.recall_consumed == 1u);
  assert(mr->referee.contact_kind == 1u);
  assert(mr->referee.foul_kind == 1u);
  assert(mr->act.id == 3u);
  assert(mr->state.phase == 0x0Fu);
  assert(mr->whistle_seq != 0u);
  assert(mr->entities.ball.carrier == carrier);   /* no possession swap */
  assert(mr->score[0] == 0u && mr->score[1] == 0u);

  lc_stage_restarts(mr);
  for (int i = 0; i < 300; i++) {
    one_granted_frame(mr);
    if (mr->act.id == 2u && mr->ref_restart_stage == 2u &&
        mr->ball_settle_active != 0u)
      observed_hold = 1;
    if (mr->state.phase == 7u && mr->act.id == 0u) break;
  }
  assert(observed_hold);                     /* the stage-3 hold is visible */
  assert(mr->state.phase == 7u);
  assert(mr->act.id == 0u);
  assert(mr->ball_settle_active == 0u);
  assert(mr->ball_row_flag_158882 == 1u);
  {
    int found = 0;
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (mr->entities.team[(uint32_t)human].records[i].code == 0x12u) found = 1;
    assert(found);                           /* the free-kick taker is armed */
  }
  one_granted_frame(mr);   /* the live phase-7 owner arms after the decision frame */
  assert(mr->live_sp_active == 1u);
  assert(mr->live_sp_hold == 1u);
  assert(mr->live_sp_kind == 7u);
  assert(mr->live_sp_taker >= (int32_t)(human * FIFA96_MATCH_ENTITY_RECORDS) &&
         mr->live_sp_taker < (int32_t)((human + 1) * FIFA96_MATCH_ENTITY_RECORDS));
  assert(mr->entities.controlled == mr->live_sp_taker);
  {
    int32_t taker = mr->live_sp_taker;
    uint32_t idx = (uint32_t)taker % FIFA96_MATCH_ENTITY_RECORDS;
    int32_t aim = 0;
    uint8_t power = 0;
    /* Put the row on its stage-4 kick and prove the live gate really holds it;
     * without the gate this frame would auto-kick and leave phase 7. */
    mr->entities.team[(uint32_t)human].records[idx].stage92 = 4u;
    mr->entities.team[(uint32_t)human].records[idx].timer89 = 0u;
    for (int i = 0; i < 12; i++) one_granted_frame(mr);
    assert(mr->state.phase == 7u);
    assert(mr->live_sp_active == 1u && mr->live_sp_hold == 1u);
    assert(mr->entities.team[(uint32_t)human].records[idx].code == 0x12u);
    assert(mr->entities.team[(uint32_t)human].records[idx].stage92 == 4u);
    aim = mr->live_sp_aim_x;
    power = mr->live_sp_power;
    lc_press_key(mr, FIFA96_ENGINE_KEY_LEFT);
    one_granted_frame(mr);
    lc_release_keys(mr);
    assert(mr->live_sp_aim_x < aim);
    lc_press_key(mr, FIFA96_ENGINE_KEY_UP);
    one_granted_frame(mr);
    lc_release_keys(mr);
    assert(mr->live_sp_power > power);
    aim = mr->live_sp_aim_x;
    lc_press_key(mr, FIFA96_ENGINE_KEY_KICK);
    one_granted_frame(mr);
    lc_release_keys(mr);
    assert(mr->state.phase == 2u);
    assert(mr->live_sp_active == 0u && mr->live_sp_hold == 0u);
    assert(mr->live_kick_active == 1u);
    assert(mr->live_kick_shot == 1u);
    assert(mr->live_shot_pending == 1u);
    assert(mr->live_kick_target_x == aim);
    assert(mr->live_kick_target_z ==
           (mr->entities.team[(uint32_t)human].side == 0u ? 0xB40 : -0xB40));
    assert(mr->entities.team[(uint32_t)human].records[idx].code != 0x12u);
    {
      int32_t start_x = mr->entities.ball.x;
      int32_t start_z = mr->entities.ball.z;
      for (int i = 0; i < 30; i++) one_granted_frame(mr);
      assert(mr->entities.ball.x != start_x || mr->entities.ball.z != start_z ||
             mr->live_kick_active == 0u || mr->live_save_hold != 0u);
    }
  }
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  drop_lc_fixture(&f);
}

/* Phase 70: the ordinary-session penalty fork. The contact point in the
 * fouler-side spot band selects phase 6 and installs the row-0x13 penalty
 * taker / row-0x1F keeper through the same release. */
static void test_live_foul_reaches_penalty(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = lc_team_for_side(mr, (uint8_t)(mr->phase_machine.side_controlled & 1u));
  uint32_t opp;
  int32_t carrier;
  int32_t rival;
  uint16_t value;
  int32_t point[3];
  assert(human >= 0);
  opp = 1u - (uint32_t)human;
  carrier = mr->entities.controlled;
  if (carrier < human * (int32_t)FIFA96_MATCH_ENTITY_RECORDS ||
      carrier >= (human + 1) * (int32_t)FIFA96_MATCH_ENTITY_RECORDS)
    carrier = (int32_t)((uint32_t)human * FIFA96_MATCH_ENTITY_RECORDS + 1u);
  rival = (int32_t)(opp * FIFA96_MATCH_ENTITY_RECORDS + 3u);
  lc_stage_restarts(mr);
  mr->config.field_4c306 = 2;
  mr->config.field_4c30a = 1;
  mr->state.phase = 2u;
  /* The penalty band is side-relative: side 0 uses z in [-0xB10,-0x7B0],
   * side 1 the mirrored positive band; |x| < 0x420 in both cases. */
  point[0] = 0x100;
  point[1] = 0;
  point[2] = mr->entities.team[opp].side == 0u ? -0x800 : 0x800;
  (void)fifa96_rng_seed(&mr->rng, 0);
  for (int i = 0; i < 3; i++) (void)fifa96_rng_step(&mr->rng, &value);
  assert(fifa96_match_run_contact(mr, 2u, rival, carrier, point) == 1);
  assert(mr->act.id == 3u && mr->state.phase == 0x0Fu);
  for (int i = 0; i < 300; i++) {
    one_granted_frame(mr);
    if (mr->state.phase == 6u && mr->act.id == 0u) break;
  }
  assert(mr->state.phase == 6u);
  assert(mr->act.id == 0u);
  assert(mr->ball_settle_active == 0u);
  assert(mr->ball_row_flag_158882 == 1u);
  {
    int taker = 0, keeper = 0;
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (mr->entities.team[(uint32_t)human].records[i].code == 0x13u) taker = 1;
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      if (mr->entities.team[opp].records[i].code == 0x1Fu) keeper = 1;
    assert(taker);
    assert(keeper);
  }
  one_granted_frame(mr);   /* the live phase-6 owner arms after the decision frame */
  assert(mr->live_sp_active == 1u);
  assert(mr->live_sp_hold == 1u);
  assert(mr->live_sp_kind == 6u);
  {
    int32_t aim = 0;
    int32_t taker = mr->live_sp_taker;
    uint32_t idx = (uint32_t)taker % FIFA96_MATCH_ENTITY_RECORDS;
    mr->entities.team[(uint32_t)human].records[idx].stage92 = 4u;
    mr->entities.team[(uint32_t)human].records[idx].timer89 = 0u;
    for (int i = 0; i < 12; i++) one_granted_frame(mr);
    assert(mr->state.phase == 6u);
    assert(mr->live_sp_active == 1u && mr->live_sp_hold == 1u);
    assert(mr->entities.team[(uint32_t)human].records[idx].stage92 == 4u);
    aim = mr->live_sp_aim_x;
    lc_press_key(mr, FIFA96_ENGINE_KEY_RIGHT);
    one_granted_frame(mr);
    lc_release_keys(mr);
    assert(mr->live_sp_aim_x > aim);
    aim = mr->live_sp_aim_x;
    lc_press_key(mr, FIFA96_ENGINE_KEY_KICK);
    one_granted_frame(mr);
    lc_release_keys(mr);
    assert(mr->state.phase == 2u);
    assert(mr->live_sp_active == 0u && mr->live_sp_hold == 0u);
    assert(mr->live_kick_active == 1u);
    assert(mr->live_kick_shot == 1u);
    assert(mr->live_kick_target_x == aim);
    assert(mr->live_kick_target_z ==
           (mr->entities.team[(uint32_t)human].side == 0u ? 0xB40 : -0xB40));
  }
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  drop_lc_fixture(&f);
}

/* Phase 78 [D]: an opponent free kick must keep the native auto row; the live
 * window binds only a taker on the stored human side. */
static void test_live_setpiece_does_not_hijack_opponent(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = lc_team_for_side(mr, 0u);
  int32_t opp = lc_team_for_side(mr, 1u);
  assert(human >= 0 && opp >= 0);
  mr->state.phase = 7u;
  mr->act.id = 0u;
  mr->phase_machine.side_controlled = 1u;
  mr->live_human_side = 0u;
  lc_stage_restarts(mr);
  mr->entities.team[(uint32_t)opp].records[1].code = 0x12u;
  mr->entities.controlled = opp * FIFA96_MATCH_ENTITY_RECORDS + 1;
  mr->live_sp_active = 0u;
  mr->live_sp_hold = 0u;
  one_granted_frame(mr);
  assert(mr->state.phase == 7u);
  assert(mr->live_sp_active == 0u);
  assert(mr->live_sp_hold == 0u);
  assert(mr->entities.team[(uint32_t)opp].records[1].code == 0x12u);
  for (int i = 0; i < 420 && mr->state.phase != 2u; i++)
    one_granted_frame(mr);
  assert(mr->state.phase == 2u);   /* the native opponent row still resolves */
  drop_lc_fixture(&f);
}

/* Phase 78 [D]: an out-of-play force during a staged set piece must drop the
 * stale taker row, not leave it to auto-fire from the fallback placement. */
static void test_live_setpiece_out_force_clears_taker(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = lc_team_for_side(mr, 0u);
  int32_t taker = human * FIFA96_MATCH_ENTITY_RECORDS + 1;
  int32_t fallback = human * FIFA96_MATCH_ENTITY_RECORDS + 2;
  assert(human >= 0);
  mr->state.phase = 7u;
  mr->act.id = 0u;
  mr->phase_machine.side_controlled = 0u;
  mr->live_human_side = 0u;
  lc_stage_restarts(mr);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &mr->entities.team[(uint32_t)human].records[i];
    e->pos_x = 0x3000;
    e->pos_z = 0x3000;
    e->target_x = e->pos_x;
    e->target_z = e->pos_z;
  }
  mr->entities.team[(uint32_t)human].records[2].pos_x = 0x7D0;
  mr->entities.team[(uint32_t)human].records[2].pos_z = 0;
  mr->entities.team[(uint32_t)human].records[2].target_x = 0x7D0;
  mr->entities.team[(uint32_t)human].records[2].target_z = 0;
  mr->entities.team[(uint32_t)human].records[1].code = 0x12u;
  mr->entities.team[(uint32_t)human].records[1].stage92 = 4u;
  mr->entities.team[(uint32_t)human].records[1].timer89 = 0u;
  mr->entities.controlled = taker;
  mr->slot.entity = taker;
  mr->entities.team[(uint32_t)human].target = taker;
  mr->live_sp_active = 1u;
  mr->live_sp_hold = 1u;
  mr->live_sp_kind = 7u;
  mr->live_sp_power = 3u;
  mr->live_sp_frames = 12u;
  mr->live_sp_taker = taker;
  mr->live_sp_aim_x = 0;
  mr->live_sp_prev_input = 0u;
  mr->goal_snap_x = 1;
  mr->goal_snap_z = 0;
  mr->live_out_pending = 1u;
  mr->live_out_frames = 89u;
  mr->live_out_kind = 1u;
  mr->live_out_situation = 2u;
  mr->live_out_side = 0u;
  one_granted_frame(mr);
  assert(mr->state.phase == 2u);
  assert(mr->live_sp_active == 0u && mr->live_sp_hold == 0u);
  assert(mr->entities.team[(uint32_t)human].records[1].code != 0x12u);
  assert(mr->entities.ball.carrier == fallback);
  drop_lc_fixture(&f);
}

/* Phase 78 [D]: the bounded player window auto-delivers the derived strike.
 * This stages the taker directly (the foul chain is covered above) and pins
 * that an unconfirmed window cannot deadlock either phase 7 or phase 6; the
 * native row-0x13 ack wait is one of the unported legs. */
static void test_live_setpiece_timeout_falls_back(uint8_t phase) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  int32_t human = lc_team_for_side(mr, 0u);
  int32_t taker;
  uint8_t code = phase == 6u ? 0x13u : 0x12u;
  int32_t goal_z;
  assert(human >= 0);
  goal_z = mr->entities.team[(uint32_t)human].side == 0u ? 0xB40 : -0xB40;
  taker = human * FIFA96_MATCH_ENTITY_RECORDS + 1;
  mr->state.phase = phase;
  mr->act.id = 0u;
  mr->phase_machine.side_controlled = 0u;
  mr->live_human_side = 0u;
  lc_stage_restarts(mr);
  mr->entities.team[(uint32_t)human].records[1].code = code;
  mr->entities.team[(uint32_t)human].records[1].stage92 = 0u;
  mr->entities.team[(uint32_t)human].records[1].timer89 = 0u;
  mr->entities.controlled = taker;
  mr->slot.entity = taker;
  mr->entities.team[(uint32_t)human].target = taker;
  mr->live_sp_active = 1u;
  mr->live_sp_hold = 1u;
  mr->live_sp_kind = phase;
  mr->live_sp_power = 3u;
  mr->live_sp_frames = 299u;
  mr->live_sp_taker = taker;
  mr->live_sp_aim_x = 0;
  mr->live_sp_prev_input = 0u;
  one_granted_frame(mr);
  assert(mr->state.phase == 2u);
  assert(mr->live_sp_active == 0u && mr->live_sp_hold == 0u);
  assert(mr->live_kick_active == 1u && mr->live_kick_shot == 1u);
  assert(mr->live_kick_target_z == goal_z);
  assert(mr->entities.team[(uint32_t)human].records[1].code != code);
  drop_lc_fixture(&f);
}

/* FU-351: the live eye is the native class-3 chase (FUN_0004DB38). The target
 * is the derived render focus; the position is clamped to the native
 * ±0x420/±0x810 box and the live Y floor 100. A far focus proves the native
 * clamp is active (the old derived envelope could only reach 0x3C0 there). */
static void test_live_native_class3_camera(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  one_granted_frame(mr);
  assert(mr->render.render_eye_valid == 1u);
  assert(mr->render.view_focus_valid == 1u);
  assert(mr->render.render_focus_valid == 1u);
  assert(mr->render.render_eye_x >= -0x420 &&
         mr->render.render_eye_x <= 0x420);
  assert(mr->render.render_eye_z >= -0x810 &&
         mr->render.render_eye_z <= 0x810);
  assert(mr->render.render_eye_y >= 100);
  assert(mr->render.yaw == 0xC000 && mr->render.pitch == 0x2000);

  /* FU351-L1: the class-3 target is the native focus of the FU-71 place
   * (0x155AA4), not the retired Phase-85 controlled-record blend. Stage a
   * distinct controlled record, a distinct ball follow, and a known FU-71
   * place; the target must be the place.
   */
  mr->state.period = 4u;
  mr->entities.controlled = 0;
  mr->entities.team[0].records[0].pos_x = 0x700;
  mr->entities.team[0].records[0].pos_z = 0x500;
  mr->render.camera.pos_x = 0x300;
  mr->render.camera.pos_y = 0x50;
  mr->render.camera.pos_z = -0x200;
  mr->render.camera.event_param = 0;
  mr->render.camera.speed = 0;
  mr->entities.ball.x = 0x1000;
  mr->entities.ball.y = 0x40;
  mr->entities.ball.z = 0x20;
  mr->entities.ball.vel_x = 0;
  mr->entities.ball.vel_z = 0;
  one_granted_frame(mr);
  assert(mr->render.render_eye_valid == 1u);
  assert(mr->render.action_cam.track_x == mr->render.render_focus_x);
  assert(mr->render.action_cam.track_y == mr->render.render_focus_y);
  assert(mr->render.action_cam.track_z == mr->render.render_focus_z);
  assert(mr->render.render_focus_x == 0x300);
  assert(mr->render.render_focus_y == 0x50);
  assert(mr->render.render_focus_z == -0x200);
  assert(mr->render.view_focus_x != mr->render.render_focus_x);
  assert(mr->render.action_cam.track_x != mr->entities.team[0].records[0].pos_x);
  assert(mr->render.render_eye_x >= -0x420 &&
         mr->render.render_eye_x <= 0x420);
  assert(mr->render.render_eye_z >= -0x810 &&
         mr->render.render_eye_z <= 0x810);
  assert(mr->render.render_eye_y >= 100);
  drop_lc_fixture(&f);
}

/* Phase 81 [D]: a native pan flag must not freeze the live ball focus. The
 * native class-3 director that consumes the pan is unported, so the derived
 * follow keeps the play in frame. */
static void test_live_follow_keeps_ball_during_pan(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  one_granted_frame(mr);
  mr->render.camera.event_param = 0x20;
  mr->render.camera.speed = 1;
  mr->state.period = 4u;   /* freeze the phase-2 ball lead so the focus target is stable */
  mr->entities.ball.x = 0x300;
  mr->entities.ball.z = -0x200;
  mr->entities.ball.vel_x = 0;
  mr->entities.ball.vel_z = 0;
  /* Phase-85: keep the human side on the ball so the new opponent-focus blend
   * is not the variable under test here (this test pins the pan gate). */
  mr->entities.ball.carrier = mr->entities.controlled;
  one_granted_frame(mr);
  assert(mr->render.view_focus_valid == 1u);
  assert(mr->render.view_focus_x == 0x300);
  assert(mr->render.view_focus_z == -0x200);
  assert(mr->render.view_focus_y == 0x50);
  drop_lc_fixture(&f);
}

/* Phase 72: the live off-ball shape moves every idle outfield record from the
 * bound pl.fmt cell metadata (the derived row-03 consumer). */
static void test_live_offball_shape_moves_team(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  static int8_t meta[0x12B * 2];
  int moved = 0;
  for (size_t i = 0; i < sizeof meta; i += 2) {
    meta[i] = 0x10;
    meta[i + 1] = 0x10;
  }
  mr->config.field_4c306 = 0;              /* keep contact out of this slice */
  mr->entities.ball.x = 0;
  mr->entities.ball.z = 0;
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    mr->entities.team[t].target = FIFA96_MATCH_ENTITY_NONE;
    mr->entities.team[t].second = FIFA96_MATCH_ENTITY_NONE;
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      struct fifa96_match_entity *e = &mr->entities.team[t].records[i];
      e->active = 1;
      e->skip_98 = 0;
      e->skip_9a = 0;
      e->code = 3;
      e->phase_meta = meta;
      e->pos_x = 0;
      e->pos_z = 0;
      e->target_x = 0;
      e->target_z = 0;
    }
  }
  for (int i = 0; i < 30; i++) one_granted_frame(mr);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      const struct fifa96_match_entity *e = &mr->entities.team[t].records[i];
      int32_t dx = e->pos_x < 0 ? -e->pos_x : e->pos_x;
      int32_t dz = e->pos_z < 0 ? -e->pos_z : e->pos_z;
      if (dx > 0x20 || dz > 0x20) moved++;
    }
  assert(moved >= 16);                     /* 20 outfield, carrier/target/second skip */
  assert(mr->state.phase == 2u);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  drop_lc_fixture(&f);
}

/* Phase 75 [D]: one possessing-team forward makes a forward support run.
 * The carrier/target/second/control cells stay untouched; the native
 * FUN_0007F7E0 machine is still FU251-L8. */
static void stage_live_support_fixture(struct fifa96_match_run *mr) {
  mr->live_kickoff_touch = 0u;
  mr->live_control_taker = FIFA96_MATCH_ENTITY_NONE;
  mr->live_kick_active = 0u;
  mr->live_kick_hold = 0u;
  mr->live_save_hold = 0u;
  mr->ball_settle_active = 0u;
  mr->act.id = 0u;
  /* No-ISO opponent formation: keep the derived AI tackle out of this
   * shape/support slice (contact has its own tests). */
  mr->live_ai_tackle_cd = 120u;
  mr->formation[0] = 1;   /* 4-4-2 */
  mr->entities.team[0].side = 0;
  mr->entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].intercept = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.team[0].chosen = FIFA96_MATCH_ENTITY_NONE;
  mr->entities.controlled = FIFA96_MATCH_ENTITY_NONE;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &mr->entities.team[0].records[i];
    e->active = 1;
    e->code = 3;
    e->skip_98 = 0;
    e->skip_9a = 0;
    e->has_slot = 0;
    e->phase_meta = NULL;
    e->pos_x = 0;
    e->pos_z = 0;
    e->target_x = 0;
    e->target_z = 0;
  }
  /* Carrier record 5 at the ball; records 9/10 are the 442 forwards. */
  mr->entities.team[0].records[5].pos_z = -0x100;
  mr->entities.team[0].records[9].pos_z = 0;
  mr->entities.team[0].records[10].pos_z = -0x100;
  stage_ball(mr, 0, 0, -0x100);
  mr->entities.ball.carrier = 5;
}

static void test_live_support_run_moves_forward(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  stage_live_support_fixture(mr);
  one_granted_frame(mr);
  assert(mr->live_support_runner == 9);       /* the more advanced forward */
  assert(mr->entities.team[0].records[9].target_z == 0x500);
  assert(mr->entities.team[0].records[9].target_x == 0);
  int32_t start_z = mr->entities.team[0].records[9].pos_z;
  for (int i = 0; i < 30; i++) one_granted_frame(mr);
  assert(mr->entities.team[0].records[9].pos_z > start_z);
  assert(mr->entities.team[0].target != 9);   /* runner is not the team target */
  assert(mr->entities.team[0].second == FIFA96_MATCH_ENTITY_NONE);
  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  drop_lc_fixture(&f);
}

static void test_live_support_run_hysteresis_and_reset(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  stage_live_support_fixture(mr);
  one_granted_frame(mr);
  assert(mr->live_support_runner == 9);

  /* A more advanced teammate does not steal the run inside the hold window. */
  mr->entities.team[0].records[10].pos_z = 0x200;
  one_granted_frame(mr);
  assert(mr->live_support_runner == 9);

  /* After expiry the pick re-evaluates and chooses record 10. */
  mr->live_support_timer = 120u;
  one_granted_frame(mr);
  assert(mr->live_support_runner == 10);

  /* The second-pick exclusion is respected: force a re-pick with 10 excluded. */
  mr->entities.team[0].second = 10;
  mr->live_support_timer = 120u;
  one_granted_frame(mr);
  assert(mr->live_support_runner != 10);

  /* Losing the possession cell clears the runner selection. */
  mr->entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  one_granted_frame(mr);
  assert(mr->live_support_runner == FIFA96_MATCH_ENTITY_NONE);
  drop_lc_fixture(&f);
}

/* P15 derived pause substitution: the two-step OUT/IN menu over the native
 * formation roles, GK-only-GK, the three-sub cap and the used-bench refusal. */
static void stage_lc_sub_roster(struct fifa96_match_run *mr) {
  static const char pos[16] = {'G', 'D', 'D', 'D', 'D', 'M', 'M', 'M',
                               'M', 'F', 'F', 'G', 'D', 'M', 'F', 'F'};
  static const uint8_t rating[16] = {50, 90, 91, 92, 93, 94, 95, 96,
                                     97, 98, 99, 80, 40, 41, 42, 43};
  for (int side = 0; side < 2; side++) {
    mr->formation[side] = 1;   /* 4-4-2 */
    for (int slot = 0; slot < (int)FIFA96_PCCD_ROSTER_COUNT; slot++) {
      (void)snprintf(mr->roster_name[side][slot],
                     sizeof mr->roster_name[side][slot], "P%d", slot + 1);
      mr->roster_number[side][slot] = (uint8_t)(slot + 1);
      mr->roster_pos[side][slot] = pos[slot];
      mr->roster_rating[side][slot] = rating[slot];
    }
  }
  assert(fifa96_match_run_sub_rebuild(mr) == 1);
}

static void test_live_pause_substitutions(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  uint8_t side = (uint8_t)(mr->phase_machine.side_controlled & 1u);
  stage_lc_sub_roster(mr);

  /* Role mapping: the highest-rated GK starts, then the role blocks. */
  assert(mr->sub_lineup[side][0] == 11u);
  for (int pitch = 1; pitch <= 4; pitch++)
    assert(mr->roster_pos[side][mr->sub_lineup[side][pitch]] == 'D');
  for (int pitch = 5; pitch <= 8; pitch++)
    assert(mr->roster_pos[side][mr->sub_lineup[side][pitch]] == 'M');
  for (int pitch = 9; pitch <= 10; pitch++)
    assert(mr->roster_pos[side][mr->sub_lineup[side][pitch]] == 'F');
  assert(mr->sub_bench_avail[side] == 0xF001u);   /* 0,12,13,14,15 */

  assert(fifa96_match_run_set_paused(mr, 1) == 0);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_OPEN) == 1);
  assert(mr->sub_menu_open == 1 && mr->sub_menu_stage == 0);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_DOWN) == 1);
  assert(mr->sub_menu_cursor == 1);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_CONFIRM) == 1);
  assert(mr->sub_menu_stage == 1);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_BACK) == 1);
  assert(mr->sub_menu_stage == 0);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_BACK) == 1);
  assert(mr->sub_menu_open == 0);

  /* Resuming with the menu open closes it; pause again for the substitution. */
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_OPEN) == 1);
  assert(fifa96_match_run_set_paused(mr, 0) == 0);
  assert(mr->sub_menu_open == 0);
  assert(fifa96_match_run_set_paused(mr, 1) == 0);

  /* First outfield substitution: OUT pitch 1, first non-GK bench slot. */
  int out1 = mr->sub_lineup[side][1];
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_OPEN) == 1);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_DOWN) == 1);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_CONFIRM) == 1);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_CONFIRM) == 1);
  assert(mr->sub_menu_open == 0);
  assert(mr->sub_used[side] == 1u);
  int in1 = mr->sub_lineup[side][1];
  assert(in1 != out1);
  assert((mr->sub_bench_avail[side] & (1u << in1)) == 0);
  assert((mr->sub_bench_avail[side] & (1u << out1)) == 0);
  assert(mr->render.sub.active == 1u);
  assert(mr->sub_notice[0] != '\0');

  /* Direct refusal: an outfield player cannot take the bench GK, and a used
   * bench slot cannot return. */
  assert(fifa96_match_run_pause_sub_apply(mr, 1, 0) == 0);
  assert(mr->sub_menu_status == FIFA96_MATCH_RUN_SUB_STATUS_GK_MISMATCH);
  assert(mr->sub_used[side] == 1u);
  assert(fifa96_match_run_pause_sub_apply(mr, 2, in1) == 0);
  assert(mr->sub_menu_status == FIFA96_MATCH_RUN_SUB_STATUS_USED);

  /* GK substitution: pitch 0 has the bench GK as its only compatible IN. */
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_OPEN) == 1);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_CONFIRM) == 1);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_CONFIRM) == 1);
  assert(mr->sub_used[side] == 2u);
  assert(mr->sub_lineup[side][0] == 0u);

  /* Third substitution, then the cap. */
  int bench3 = -1;
  for (int r = 0; r < (int)FIFA96_PCCD_ROSTER_COUNT; r++)
    if ((mr->sub_bench_avail[side] & (1u << r)) != 0 &&
        mr->roster_pos[side][r] != 'G') {
      bench3 = r;
      break;
    }
  assert(bench3 >= 0);
  assert(fifa96_match_run_pause_sub_apply(mr, 2, bench3) == 1);
  assert(mr->sub_used[side] == 3u);
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_OPEN) == 0);
  assert(mr->sub_menu_status == FIFA96_MATCH_RUN_SUB_STATUS_LIMIT);
  assert(strstr(mr->sub_notice, "NO SUBSTITUTIONS") != NULL);

  assert(mr->score[0] == 0u && mr->score[1] == 0u);
  assert(mr->state.phase == 2u);
  assert(fifa96_match_run_set_paused(mr, 0) == 0);
  assert(mr->sub_menu_open == 0);

  /* Non-live runs keep the mutation gate closed. */
  mr->live_start = 0;
  assert(fifa96_match_run_pause_sub(mr, FIFA96_MATCH_RUN_SUB_OPEN) ==
         -FIFA96_ERR_UNSUPPORTED);
  mr->live_start = 1;
  drop_lc_fixture(&f);
}

/* The native PCCD order branch: roster_order[0..10] is used verbatim and the
 * remaining five slots are the bench. A duplicate slot is not a permutation,
 * so the builder must fall back to the derived map. */
static void test_live_sub_native_order(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  uint8_t side = (uint8_t)(mr->phase_machine.side_controlled & 1u);
  static const uint8_t order[FIFA96_PCCD_ROSTER_COUNT] = {
      14, 1, 12, 6, 2, 15, 13, 3, 9, 0, 10, 4, 7, 11, 8, 5};
  for (int s = 0; s < 2; s++) {
    memcpy(mr->roster_order[s], order, sizeof order);
    mr->roster_order_ok[s] = 1;
  }
  assert(fifa96_match_run_sub_rebuild(mr) == 1);
  for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++)
    assert(mr->sub_lineup[side][pitch] == order[pitch]);
  uint16_t bench = 0;
  for (int i = (int)FIFA96_MATCH_ENTITY_RECORDS;
       i < (int)FIFA96_PCCD_ROSTER_COUNT; i++)
    bench |= (uint16_t)(1u << order[i]);
  assert(mr->sub_bench_avail[side] == bench);

  mr->roster_order[side][0] = 0;
  mr->roster_order[side][1] = 0;
  memset(mr->roster_pos[side], 0, sizeof mr->roster_pos[side]);
  memset(mr->roster_rating[side], 0, sizeof mr->roster_rating[side]);
  assert(fifa96_match_run_sub_rebuild(mr) == 1);
  for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++)
    assert(mr->sub_lineup[side][pitch] == pitch);
  drop_lc_fixture(&f);
}

/* No PCCD positions/ratings: the identity map keeps the menu usable. */
static void test_live_sub_identity_fallback(void) {
  struct lc_fixture f = make_lc_fixture();
  struct fifa96_match_run *mr = f.mr;
  memset(mr->roster_pos, 0, sizeof mr->roster_pos);
  memset(mr->roster_rating, 0, sizeof mr->roster_rating);
  assert(fifa96_match_run_sub_rebuild(mr) == 1);
  for (int pitch = 0; pitch < (int)FIFA96_MATCH_ENTITY_RECORDS; pitch++)
    assert(mr->sub_lineup[0][pitch] == pitch);
  assert(mr->sub_bench_avail[0] == 0xF800u);   /* 11..15 */
  drop_lc_fixture(&f);
}

int main(void) {
  test_live_begin_seeds_kickoff_carrier();
  test_live_carrier_dribbles_with_ball();
  test_live_offball_slot_player_moves();
  test_live_touchline_exit_throw_in();
  test_live_goal_line_exit_restarts();
  test_live_inside_ball_never_restarts();
  test_live_kick_release_runs_row07();
  test_live_pass_flight_travels();
  test_live_kick_flight_scores();
  test_live_kickoff_shot_scores();
  test_live_taker_guard_and_relatch();
  test_live_auto_switch_carrier_and_nearest();
  test_live_manual_switch_cycles();
  test_live_manual_switch_keeps_carrier();
  test_live_switch_is_not_fu61();
  test_live_human_contact_clean_and_foul();
  test_live_goal_after_kick();
  test_live_opponent_presses_and_claims();
  test_live_opponent_far_claim_rejected();
  test_live_cpu_carrier_acts();
  test_live_ai_tackles_human();
  test_live_ai_tackle_foul_reaches_free_kick();
  test_live_foul_reaches_penalty();
  test_live_setpiece_does_not_hijack_opponent();
  test_live_setpiece_out_force_clears_taker();
  test_live_setpiece_timeout_falls_back(7u);
  test_live_setpiece_timeout_falls_back(6u);
  test_live_native_class3_camera();
  test_live_follow_keeps_ball_during_pan();
  test_live_offball_shape_moves_team();
  test_live_support_run_moves_forward();
  test_live_support_run_hysteresis_and_reset();
  test_live_pause_substitutions();
  test_live_sub_native_order();
  test_live_sub_identity_fallback();
  test_live_defender_blocks_both_sides();
  test_live_shot_block_mouth_edge();
  test_live_shot_block_skips_hidden_defender();
  test_live_flight_end_stale_pair_guard();
  test_live_shot_still_scores_wide();
  test_live_shot_block_ignores_pass();
  test_live_aim_depth_axis();
  test_live_keeper_saves_centre();
  test_live_keeper_saves_centre_no_staging();
  test_live_keeper_beaten_by_aim();
  test_live_keeper_displaced_centre_goal();
  test_live_keeper_goal_side();
  test_live_keeper_mouth_edge();
  test_live_stale_aim_two_kicks();
  printf("test_engine_live_control: ok\n");
  return 0;
}
