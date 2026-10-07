/* tests/test_engine_match_handlers.c — Task 4: action/phase dispatch tables (FU-137).
 *
 * The two native tables are action `0x1106E0[45]` (codes 0x00..0x2C, sole reader
 * `FUN_0007D9A4 @ 0x7DA77`) and phase `0x110794[35]` (phases 0x00..0x22, sole
 * reader `FUN_0006D920 @ 0x6D9B3`) — FU-137 §1/§2. Task 5 (FU-138) wires the
 * first cluster-A row: action 00 runs the FU-76 §3.1 locomotion step/target on
 * `mr->record` and reports OK. Task 7 (FU-140, cluster C) wires the keeper's
 * fully linear claim/throw row 1E (`fifa96_keeper_claim_place` on `mr->record`);
 * the other six keeper rows stay UNSUPPORTED with their arm/pool legs. Every
 * other action row and all 34 non-zero phase rows still resolve to an explicit
 * UNSUPPORTED open-leg marker, except phase 0x16 (the native zero/INT3 slot)
 * which is NOT_FOUND by contract. Task 6 (FU-139, cluster B) derives the ball
 * staging/resolver/possession/kick pure helpers but wires no additional row:
 * 05/06/07/0F/18/21/23 still need the absent entity/ball pool and their
 * unported arm support (FU-139 §5, OL-29..OL-32). The seam itself must run a
 * handler and propagate its result when one is present. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_engine.h"
#include "fifa96_engine/fifa96_match_handlers.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_engine/fifa96_platform_null.h"

#define UNSUP (-FIFA96_ERR_UNSUPPORTED)
#define NOTF (-FIFA96_ERR_NOT_FOUND)

/* FU-137 §6 row classification mapped to the dispatch contract, updated by
 * FU-138 for row 00 (ported: `fifa96_match_action_00`) and by FU-140 for row
 * 1E (ported: `fifa96_match_action_1E`, the fully linear claim/throw
 * placement). All other action rows are UNSUPPORTED (the six remaining keeper
 * rows 19/1A/1B/1C/1D/1F have tested pure parts but unported arms; the other
 * 43 are `not ported`, of which 27/29/2B/2C are open legs with no static
 * install arm). FU-139 (cluster B) keeps 05/06/07/0F and the possession/tackle
 * rows 18/21/23 UNSUP: their record-visible cores have tested pure helpers, but
 * the carrier/pursuit/kick/receive/resolution arms and the entity/ball pool
 * they bind to are unported (FU-139 §5). */
static const int action_expect[FIFA96_MATCH_ACTION_ROWS] = {
    /* 00 */ FIFA96_OK, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 0A */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 14 */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 1E */ FIFA96_OK, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 28 */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
};

/* FU-137 §6 phase classification: phase 0x16 is the native table's zero entry
 * (loader INT3 stub) with no handler body by design → NOT_FOUND; the other 34
 * rows are classified-but-unported → UNSUPPORTED. */
static const int phase_expect[FIFA96_MATCH_PHASE_ROWS] = {
    /* 00 */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 0A */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 14 */ UNSUP, UNSUP, NOTF,  UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 1E */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
};

struct fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
  struct fifa96_match_run mr;
};

static struct fixture make_fixture(void) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;
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
  fifa96_match_run_init(&f.mr);
  assert(fifa96_match_run_begin(&f.mr, f.engine, 0) == 0);
  return f;
}

static void drop_fixture(struct fixture f) {
  assert(fifa96_match_run_end(&f.mr) == 0);
  fifa96_engine_destroy(f.engine);
  fifa96_platform_destroy(f.plat);
}

/* Every table row carries its code and an explicit evidence/OpenLeg marker —
 * "resolves to a handler or an explicit open leg", never a silent slot: an
 * unported row (fn == NULL) must name its numbered OL- marker, except the
 * phase zero slot which has no handler by design and says so. */
static void test_tables_are_fully_classified(void) {
  for (uint32_t i = 0; i < FIFA96_MATCH_ACTION_ROWS; i++) {
    const struct fifa96_match_handler *row = &fifa96_match_action_table[i];
    assert(row->code == i);
    assert(row->evidence != NULL);
    assert(row->evidence[0] != '\0');
    if (row->fn == NULL) assert(strstr(row->evidence, "OL-") != NULL);
  }
  for (uint32_t i = 0; i < FIFA96_MATCH_PHASE_ROWS; i++) {
    const struct fifa96_match_handler *row = &fifa96_match_phase_table[i];
    assert(row->code == i);
    assert(row->evidence != NULL);
    assert(row->evidence[0] != '\0');
    if (row->fn == NULL && i != FIFA96_MATCH_PHASE_INT3_SLOT)
      assert(strstr(row->evidence, "OL-") != NULL);
  }
  assert(strstr(fifa96_match_phase_table[FIFA96_MATCH_PHASE_INT3_SLOT].evidence,
                "NOT_FOUND") != NULL);
}

/* Per-row dispatch: the FU-137/FU-138 classification is the contract; no
 * unassigned row may return OK (a silent no-op success). */
static void test_action_rows_dispatch_per_classification(void) {
  struct fixture f = make_fixture();
  for (uint32_t code = 0; code < FIFA96_MATCH_ACTION_ROWS; code++) {
    int rc = fifa96_match_dispatch_action(&f.mr, (uint8_t)code);
    assert(rc == action_expect[code]);
    if (fifa96_match_action_table[code].fn == NULL) assert(rc != FIFA96_OK);
  }
  drop_fixture(f);
}

/* FU-138 §4: the wired row 00 runs the FU-76 §3.1 body against the run record:
 * timer decay, control-slot move target clamp, and the phase-2 install request
 * (3 active / 0x19 inactive). */
static void test_action_00_runs_move_step(void) {
  struct fixture f = make_fixture();
  f.mr.state.phase = 2;
  f.mr.record.pos_x = 0x100;
  f.mr.record.pos_z = 0x200;
  f.mr.record.timer89 = 100;
  f.mr.record.delta = 10;
  f.mr.record.timer81 = 0;
  f.mr.record.active = 1;
  f.mr.record.has_slot = 1;
  f.mr.record.dir_x = 1;
  f.mr.record.dir_z = 2;
  assert(f.mr.record.ran == 0);

  assert(fifa96_match_dispatch_action(&f.mr, 0x00) == FIFA96_OK);
  assert(f.mr.record.ran == 1);
  assert(f.mr.record.timer89 == 90);
  assert(f.mr.record.target_x == 0x100 + 128);
  assert(f.mr.record.target_z == 0x200 + 256);
  assert(f.mr.record.install == 0);

  f.mr.record.timer89 = 5;
  f.mr.record.has_slot = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x00) == FIFA96_OK);
  assert(f.mr.record.timer89 == -5);
  assert(f.mr.record.install == 3); /* active != 0 */
  assert(f.mr.record.target_x == 0x100 + 128); /* no slot -> no move */

  f.mr.record.timer89 = 0;
  f.mr.record.install = 0;
  f.mr.record.active = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x00) == FIFA96_OK);
  assert(f.mr.record.install == 0x19);

  f.mr.record.pos_x = 0x700;
  f.mr.record.pos_z = -0xB00;
  f.mr.record.dir_x = 2;
  f.mr.record.dir_z = -2;
  f.mr.record.timer89 = 0;
  f.mr.record.timer81 = 5; /* install gate held by timer81 */
  f.mr.record.has_slot = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x00) == FIFA96_OK);
  assert(f.mr.record.target_x == 0x720);
  assert(f.mr.record.target_z == -0xB10);
  drop_fixture(f);
}

/* FU-140 §4: the wired keeper row 1E runs the fully linear claim/throw
 * placement body (0x7550C..0x755D3, FU-79 §7): the pre-gate slot-merge helper
 * request (`stage < 6 && !has_slot`), then, while `stage < 3` and the record
 * does not hold the ball, the per-type offset placement (native
 * `0x10F334/0x10F33C[type8]`, caller-supplied here), the possession flag
 * (+0x9B) and the actor binding ([0x157A83] = rec). The slot-merge body
 * (FUN_0007876C), the camera-place call (FUN_000700F4) and the actor pointer
 * stay modelled as record requests/flags until the C8/C11 pool and arm ports
 * land (FU-140 OL-37, carrying OL-16). */
static void test_action_1E_runs_claim_place(void) {
  struct fixture f = make_fixture();
  f.mr.record.stage = 1;
  f.mr.record.pos_x = 100;
  f.mr.record.pos_y = 20;
  f.mr.record.pos_z = 200;
  f.mr.record.place_offset_x = -3;
  f.mr.record.place_offset_z = 2;
  assert(f.mr.record.has_slot == 0 && f.mr.record.has_ball == 0);
  assert(f.mr.record.controlled == 0 && f.mr.record.helper_request == 0);

  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.place_x == 52 && f.mr.record.place_y == 76);
  assert(f.mr.record.place_z == 232);
  assert(f.mr.record.has_ball == 1 && f.mr.record.controlled == 1);

  /* stage >= 3 blocks the claim but still emits the pre-gate helper request */
  f.mr.record.stage = 3;
  f.mr.record.has_ball = 0;
  f.mr.record.controlled = 0;
  f.mr.record.place_x = -1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.has_ball == 0 && f.mr.record.controlled == 0);
  assert(f.mr.record.place_x == -1);

  /* already holding the ball -> no placement, no actor rebind */
  f.mr.record.stage = 1;
  f.mr.record.has_ball = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.controlled == 0 && f.mr.record.place_x == -1);

  /* stage 6 clears the helper request; a slot suppresses it */
  f.mr.record.stage = 6;
  f.mr.record.has_ball = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 0);
  f.mr.record.stage = 1;
  f.mr.record.has_slot = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 0);
  assert(f.mr.record.has_ball == 1 && f.mr.record.controlled == 1);
  drop_fixture(f);
}

static void test_phase_rows_dispatch_per_classification(void) {
  struct fixture f = make_fixture();
  for (uint32_t phase = 0; phase < FIFA96_MATCH_PHASE_ROWS; phase++) {
    int rc = fifa96_match_dispatch_phase(&f.mr, (uint8_t)phase);
    assert(rc == phase_expect[phase]);
    if (phase == FIFA96_MATCH_PHASE_INT3_SLOT)
      assert(rc == NOTF);
    else
      assert(rc == UNSUP);
  }
  drop_fixture(f);
}

/* Out-of-range codes are NOT_FOUND (never OK), including the byte maximum. */
static void test_out_of_range_is_not_found(void) {
  struct fixture f = make_fixture();
  assert(fifa96_match_dispatch_action(&f.mr, FIFA96_MATCH_ACTION_ROWS) == NOTF);
  assert(fifa96_match_dispatch_action(&f.mr, 0x2Du) == NOTF);
  assert(fifa96_match_dispatch_action(&f.mr, 0xFFu) == NOTF);
  assert(fifa96_match_dispatch_phase(&f.mr, FIFA96_MATCH_PHASE_ROWS) == NOTF);
  assert(fifa96_match_dispatch_phase(&f.mr, 0x23u) == NOTF);
  assert(fifa96_match_dispatch_phase(&f.mr, 0xFFu) == NOTF);
  drop_fixture(f);
}

/* NULL run -> -INVALID for both dispatchers and the seam runner. */
static void test_null_arguments_are_invalid(void) {
  struct fixture f = make_fixture();
  assert(fifa96_match_dispatch_action(NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_dispatch_phase(NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_dispatch_row(NULL, &fifa96_match_action_table[0]) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_dispatch_row(&f.mr, NULL) == -FIFA96_ERR_INVALID);
  drop_fixture(f);
}

static int seam_calls;
static int seam_rc;

static int seam_handler(struct fifa96_match_run *mr) {
  assert(mr != NULL);
  seam_calls++;
  return seam_rc;
}

/* The extensible seam: a row with a handler runs it and returns its result
 * verbatim; a row without one reports UNSUPPORTED without inventing success. */
static void test_seam_runs_wired_handler(void) {
  struct fixture f = make_fixture();
  struct fifa96_match_handler row = {0x00, seam_handler, "test row"};
  seam_calls = 0;
  seam_rc = FIFA96_OK;
  assert(fifa96_match_dispatch_row(&f.mr, &row) == FIFA96_OK);
  assert(seam_calls == 1);

  seam_rc = -FIFA96_ERR_STATE;
  assert(fifa96_match_dispatch_row(&f.mr, &row) == -FIFA96_ERR_STATE);
  assert(seam_calls == 2);

  row.fn = NULL;
  assert(fifa96_match_dispatch_row(&f.mr, &row) == UNSUP);
  assert(seam_calls == 2);   /* no handler ran */
  drop_fixture(f);
}

int main(void) {
  test_tables_are_fully_classified();
  test_action_rows_dispatch_per_classification();
  test_action_00_runs_move_step();
  test_action_1E_runs_claim_place();
  test_phase_rows_dispatch_per_classification();
  test_out_of_range_is_not_found();
  test_null_arguments_are_invalid();
  test_seam_runs_wired_handler();
  puts("test_engine_match_handlers OK");
  return 0;
}
