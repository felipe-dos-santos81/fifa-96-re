/* tests/test_engine_match_handlers.c — Task 4: action/phase dispatch tables (FU-137).
 *
 * The two native tables are action `0x1106E0[45]` (codes 0x00..0x2C, sole reader
 * `FUN_0007D9A4 @ 0x7DA77`) and phase `0x110794[35]` (phases 0x00..0x22, sole
 * reader `FUN_0006D920 @ 0x6D9B3`) — FU-137 §1/§2. Task 5 (FU-138) wires the
 * first cluster-A row: action 00 runs the FU-76 §3.1 locomotion step/target on
 * `mr->record` and reports OK. Task 7 (FU-140, cluster C) wires the keeper's
 * fully linear claim/throw row 1E (`fifa96_keeper_claim_place` on `mr->record`);
 * the other six keeper rows stay UNSUPPORTED with their arm legs. Every other
 * action row and all 34 non-zero phase rows still resolve to an explicit
 * UNSUPPORTED open-leg marker, except phase 0x16 (the native zero/INT3 slot)
 * which is NOT_FOUND by contract. Task 6 (FU-139, cluster B) derives the ball
 * staging/resolver/possession/kick pure helpers but wires no additional row:
 * 05/06/07/0F/18/21/23 still need their unported arm support (FU-139 §5,
 * OL-29..OL-32). Task 8 (FU-141, cluster D/E) lands the entity/ball pool and
 * drains row 00's install/ran and row 1E's helper/place/actor requests through
 * it (the frame-body dispatch callback); the rows themselves stay the same two
 * ported rows because every other body still needs an unported arm. FU-142b
 * (cluster G) wires row 26: arm 0x8D74D (FU-142a), the `0x866F4..0x8681C`
 * placement body and the pool binding are all bounded, so
 * `fifa96_match_action_26` binds `fifa96_arm_26_step` to `mr->record` and the
 * dispatch expectation flips to FIFA96_OK for exactly that row. FU-142b
 * Task 4 ports row 27's body (`fifa96_arm_27_step`) but no static entry
 * exists, so row 27 keeps `fn == NULL`, its evidence names the
 * FU-142f/OL-48 entry verdict and its dispatch expectation stays UNSUP. FU-142b
 * Task 5 ports row 2C's body (`fifa96_arm_2c_step` + `fifa96_arm_reset`) under
 * the same verdict: no static entry, `fn == NULL`, OL-48 evidence, UNSUP
 * dispatch. FU-139 §10 (Task 12) wires the possession/tackle rows 18/21/23
 * (`fifa96_match_action_18`/`_21`/`_23` bind the duel/receive/tackle machines
 * plus the new NSEARCH/SWAP/bind resolution arms), so their expectations flip
 * to FIFA96_OK and the resolution/claim/target tests run over the pool.
 * Task 6 ports row 29's body (`fifa96_arm_29_step`, the phase-5
 * stage machine) with the same verdict (no static installer of 0x29; the
 * phase-5 handler installs nothing), so row 29 keeps `fn == NULL`, OL-48
 * evidence and UNSUP dispatch. FU-142d (Task 7) wires row 28: the FU-142a arm
 * 0x8D7CF, the ported `0x870E8..0x874E3` 4-arm body + `0x87014` helper
 * (Appendix G) and the pool binding are bounded, so `fifa96_match_action_28`
 * sets the row's `fn` and the dispatch expectation flips to FIFA96_OK. FU-142e
 * (Task 8) wires row 2A the same way: arm 0x8D807 installs code 0x2A, the
 * `0x86A34..0x87010` 12-arm body (Appendix H) and the distance/flag830/global
 * pool binding are bounded, so `fifa96_match_action_2A` sets the row's `fn`
 * and its expectation flips to FIFA96_OK. FU-142f (Task 9) closes the entry
 * question for the three remaining bodies: an exhaustive first-hand census of
 * all 77 `FUN_0007D9A4` call sites (constants and every register-derived
 * argument; Appendix I) finds no 0x27/0x29/0x2C invocation anywhere, and no
 * stored pointer to the installer exists (the address bytes occur nowhere in
 * the image), so rows 27/29/2C keep `fn == NULL`, their OL-48 evidence and
 * UNSUP dispatch. Row 2B's entry 0x87738 is the shared row-29 epilogue RET
 * (FU-142 §1.1), not a standalone stub, and the same census finds no 0x2B
 * invocation: its evidence records the FU-142f dead verdict. FU-139 §8
 * (Task 10) ports the row-05 carrier machine and the ball staging tail; M2
 * phase-9 T2 wires row 05 over the pool (claim/team-target/control/dirs/
 * stages/hand-off), leaving the stage-0 target algebra and the FUN_0007F7E0
 * fallback as OL-63 residual legs named in the evidence. FU-139 §11
 * (Task 13) wires row 06: `fifa96_match_action_06` binds
 * `fifa96_action_pursuit_step` (`0x801B4..0x809EF`) over the pool, so
 * `action_expect[0x06]` flips to FIFA96_OK; the plan's `..0x81067` span end is
 * the row-09 handler (`0x80A00`, slot `0x1106E0[9]`), which stays UNSUP. The seam itself
 * must run a handler and propagate its result when one is present. */
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
 * FU-138 for row 00 (ported: `fifa96_match_action_00`), by FU-140 for row
 * 1E (ported: `fifa96_match_action_1E`, the fully linear claim/throw
 * placement) and by FU-142b for row 26 (ported: `fifa96_match_action_26`,
 * the FU-142 Appendix C placement machine; the first cluster-G row whose
 * install arm + body + pool binding are all bounded) and by FU-142d for row
 * 28 (ported: `fifa96_match_action_28`, the FU-142 Appendix G 4-arm machine)
 * and by FU-142e for row 2A (ported: `fifa96_match_action_2A`, the FU-142
 * Appendix H 12-arm machine). All other action rows
 * are UNSUPPORTED (the six remaining keeper rows 19/1A/1B/1C/1D/1F have
 * tested pure parts but unported arms; rows 27, 2C and 29 have ported bodies
 * but are unwired with their entries unresolved per the FU-142f census; the
 * rest are `not ported`, with 2B now a dead entry per FU-142f). FU-139
 * (cluster B) keeps 05/06 UNSUP (their carrier/pursuit arms are unported);
 * FU-139 §10 (Task 12) wires the possession/tackle rows 18/21/23
 * (`fifa96_match_action_18`/`_21`/`_23` bind the duel/receive/tackle machines
 * plus the new NSEARCH/SWAP/bind resolution arms), so their expectations flip
 * to FIFA96_OK and the resolution/claim/target tests run over the pool. */
static const int action_expect[FIFA96_MATCH_ACTION_ROWS] = {
    /* 00 */ FIFA96_OK, FIFA96_OK, FIFA96_OK, UNSUP, FIFA96_OK, FIFA96_OK, FIFA96_OK, FIFA96_OK, FIFA96_OK, UNSUP,
    /* 0A */ UNSUP, FIFA96_OK, FIFA96_OK, UNSUP, FIFA96_OK, FIFA96_OK, FIFA96_OK, FIFA96_OK, FIFA96_OK, FIFA96_OK,
    /* 14 */ UNSUP, UNSUP, UNSUP, UNSUP, FIFA96_OK, UNSUP, UNSUP, UNSUP, UNSUP, FIFA96_OK,
    /* 1E */ FIFA96_OK, UNSUP, UNSUP, FIFA96_OK, UNSUP, FIFA96_OK, UNSUP, UNSUP, FIFA96_OK, UNSUP,
    /* 28 */ FIFA96_OK, UNSUP, FIFA96_OK, UNSUP, UNSUP,
};

/* FU-137 §6 phase classification: phase 0x16 is the native table's zero entry
 * (loader INT3 stub) with no handler body by design → NOT_FOUND; the other 34
 * rows are classified-but-unported → UNSUPPORTED. */
static const int phase_expect[FIFA96_MATCH_PHASE_ROWS] = {
    /* 00 */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 0A */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 14 */ UNSUP, UNSUP, NOTF,  UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 1E */ UNSUP, FIFA96_OK, UNSUP, UNSUP, UNSUP,
};

struct fixture {
  fifa96_platform *plat;
  struct fifa96_engine *engine;
  struct fifa96_match_run mr;
};

static void make_fixture(struct fixture *f) {
  struct fifa96_platform_null_config pcfg;
  memset(&pcfg, 0, sizeof pcfg);
  pcfg.step_ns = 10000000ull;
  f->plat = fifa96_platform_null_create(&pcfg);
  assert(f->plat != NULL);
  struct fifa96_engine_config ecfg;
  memset(&ecfg, 0, sizeof ecfg);
  ecfg.width = 320;
  ecfg.height = 240;
  ecfg.headless = 1;
  f->engine = fifa96_engine_create(&ecfg, f->plat);
  assert(f->engine != NULL);
  assert(fifa96_engine_boot(f->engine) == 0);
  fifa96_match_run_init(&f->mr);
  assert(fifa96_match_run_begin(&f->mr, f->engine, 0) == 0);
  /* Handler unit fixtures see the pristine native reset seed (code 0,
   * +0x92 0xFF): begin's derived kickoff state-1 arm stages 0x19 on the
   * records (M2 playable-match Task 2), and these tests pin the row bodies
   * against FUN_0007DAB4; the arm itself is pinned by
   * test_engine_match_phase_machine and test_engine_match_frame. */
  assert(fifa96_match_entities_init(&f->mr.entities) == FIFA96_OK);
}

static void drop_fixture(struct fixture *f) {
  assert(fifa96_match_run_end(&f->mr) == 0);
  fifa96_engine_destroy(f->engine);
  fifa96_platform_destroy(f->plat);
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
  struct fixture f;
  make_fixture(&f);
  for (uint32_t code = 0; code < FIFA96_MATCH_ACTION_ROWS; code++) {
    int rc = fifa96_match_dispatch_action(&f.mr, (uint8_t)code);
    assert(rc == action_expect[code]);
    if (fifa96_match_action_table[code].fn == NULL) assert(rc != FIFA96_OK);
  }
  drop_fixture(&f);
}

/* FU-138 §4: the wired row 00 runs the FU-76 §3.1 body against the run record:
 * timer decay, control-slot move target clamp, and the phase-2 install request
 * (3 active / 0x19 inactive). */
static void test_action_00_runs_move_step(void) {
  struct fixture f;
  make_fixture(&f);
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
  drop_fixture(&f);
}

/* FU-140 §4: the wired keeper row 1E runs the fully linear claim/throw
 * placement body (0x7550C..0x755D3, FU-79 §7): the pre-gate slot-merge helper
 * request (`stage < 6 && !has_slot`), then, while `stage < 3` and the record
 * does not hold the ball, the per-type offset placement (native
 * `0x10F334/0x10F33C[type8]`, caller-supplied here), the possession flag
 * (+0x9B) and the actor binding ([0x157A83] = rec). The slot-merge body
 * (FUN_0007876C), the camera-place call (FUN_000700F4) and the actor pointer
 * stay modelled as record requests/flags until the C8/C11 pool and arm ports
 * land (FU-140 OL-37, carrying OL-16). T2-review erratum: the native `< 6`
 * (`0x7551A`) and `< 3` (`0x7553E`) tests read `MOV EAX,[EAX+0x8F]; SAR
 * EAX,0x18` = byte `+0x92`, so the fixture stages `record.stage92`, not the
 * never-written `record.stage`. */
static void test_action_1E_runs_claim_place(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.stage92 = 1;
  f.mr.record.pos_x = 100;
  f.mr.record.pos_y = 20;
  f.mr.record.pos_z = 200;
  f.mr.record.place_offset_x = -3;
  f.mr.record.place_offset_z = 2;
  assert(f.mr.record.has_slot == 0 && f.mr.record.has_ball == 0);
  assert(f.mr.record.controlled == 0 && f.mr.record.helper_request == 0);
  assert(f.mr.record.place_valid == 0);

  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.place_x == 52 && f.mr.record.place_y == 76);
  assert(f.mr.record.place_z == 232);
  assert(f.mr.record.place_valid == 1);
  assert(f.mr.record.has_ball == 1 && f.mr.record.controlled == 1);

  /* stage >= 3 blocks the claim but still emits the pre-gate helper request */
  f.mr.record.stage92 = 3;
  f.mr.record.has_ball = 0;
  f.mr.record.controlled = 0;
  f.mr.record.place_x = -1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.has_ball == 0 && f.mr.record.controlled == 0);
  assert(f.mr.record.place_valid == 0 && f.mr.record.place_x == -1);

  /* already holding the ball -> no placement, no actor rebind */
  f.mr.record.stage92 = 1;
  f.mr.record.has_ball = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.controlled == 0 && f.mr.record.place_x == -1);
  assert(f.mr.record.place_valid == 0);

  /* stage 6 clears the helper request; a slot suppresses it */
  f.mr.record.stage92 = 6;
  f.mr.record.has_ball = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 0);
  f.mr.record.stage92 = 1;
  f.mr.record.has_slot = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 0);
  assert(f.mr.record.place_valid == 1);
  assert(f.mr.record.has_ball == 1 && f.mr.record.controlled == 1);
  drop_fixture(&f);
}

/* FU-149 L13 (T1): the throw-in taker row 0x10. Stage 0 places the record at
 * the 0x15777C snapshot ±0x10 (the goal_snap stand-in) and advances to stage 1
 * (FUN_000832A8 stand-in); stage 2's FUN_00085498 probe waits on the 0x78
 * timer and then latches the delivery row; stage 3's animation-frame gate then
 * picks the relay record and installs code 4; stage 4 resolves through the
 * shared situation-0xB entry (phase 2) and writes the 0x12C offside timer. */
static void test_action_10_runs_throw_in_body(void) {
  struct fixture f;
  make_fixture(&f);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &f.mr.entities.team[0].records[i];
    e->active = 1;
    e->pos_x = (int32_t)(i * 0x100);
    e->pos_z = 0;
    e->target_x = e->pos_x;
  }
  f.mr.record.entity_id = 1;               /* team 0, index 1 */
  f.mr.record.active = 1;
  f.mr.record.code = 0x10;
  f.mr.record.frame = 4;                   /* the +0x3D animation-frame gate */
  f.mr.record.delta = 0x80;                /* the whole-frame [0x157A64] */
  f.mr.record.stage92 = 0;
  f.mr.state.phase = 3u;                   /* the throw-in phase gate */
  f.mr.session_gate_14c32a = 1;            /* the byte-1 (Practise)
                                            * session staged (FU-166) */
  f.mr.goal_snap_x = 0x100;
  f.mr.goal_snap_y = 0;
  f.mr.goal_snap_z = 0x400;

  /* first dispatch: stage 0 placement -> stage 1 setup -> stage 2 probe wait */
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  assert(f.mr.record.controlled == 1);
  assert(f.mr.record.helper_request == 1);        /* FUN_0007876C request */
  assert(f.mr.record.place_valid == 1);
  assert(f.mr.record.place_x == 0x100 && f.mr.record.place_z == 0x400);
  assert(f.mr.record.pos_x == 0x110);              /* snapshot x ± 0x10 */
  assert(f.mr.record.pos_z == 0x400);
  assert(f.mr.record.target_x == 0x110 && f.mr.record.target_z == 0x400);
  assert(f.mr.referee.offside_suppress == 0 && f.mr.state.phase == 3u);

  /* second dispatch: probe fires, the pick installs code 4 and stage 4
   * resolves through the shared situation-0xB -> phase 2 */
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);
  assert(f.mr.referee.offside_suppress == 0x12C);
  assert(f.mr.record.stage92 == 5u);               /* past the resolution */
  assert(f.mr.entities.team[0].target == 2);       /* self skipped by the +0x9A stamp */
  assert(f.mr.entities.team[0].records[2].code == 4u);

  /* the |x| >= 0x720 re-queue: with the [0x157821] gate set, stage 4 loops
   * through situation 2 (phase 0 -> the table-2 phase 3 + a fresh arm) */
  f.mr.state.phase = 3;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_z = 0;
  f.mr.record.timer89 = 0;
  f.mr.record.frame = 4;
  f.mr.goal_snap_x = 0x800;                /* |camera x| >= 0x720 at stage 4 */
  f.mr.sp_157821 = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);  /* stage 2 */
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
  assert(f.mr.state.phase == 3u);          /* situation 2 -> phase 3 */
  assert(f.mr.entities.team[0].records[8].code == 0x10u);  /* the fresh arm */
  f.mr.sp_157821 = 0;

  /* phase gate: a forced phase resets the row */
  f.mr.state.phase = 5;
  f.mr.record.stage92 = 2;
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
  /* the FUN_0007DAB4 reset installs code 0 (the record held 3 after the fresh
   * arm), so the staged stage byte lands on 0 */
  assert(f.mr.record.stage92 == 0u);
  assert(f.mr.entities.team[0].records[1].code == 0u);
  drop_fixture(&f);
}

/* FU-157 §4.2/§5 (T1) + FU-162 §3.3.A (T3 fix): the throw-in stage-3 camera
 * cut restores the native `[0x157750] = 0x80`. Native 0x85911..0x85919
 * (`MOV EAX,0x80`; the record position MOVSD x3; `MOV [0x00157750],EAX`) then
 * 0x85950/0x85958 pushes the just-written 0x80 as the FUN_000700F4 Y (CALL
 * 0x700F4 at 0x8596B), so the cut is FUN_000700F4(rec.pos_x + dir_x[oct]<<6,
 * 0x80, rec.pos_z + dir_z[oct]<<6, 0) with the **live face octant**
 * `oct = byte[rec+0x8E]` (fresh `0x8591e MOV EAX,[EBP+0x8b]; SAR EAX,0x18`),
 * not the never-written `+0x8B` actor-type byte (FU-162-L3). The Y > 0 wakes
 * the BF20 record-event tail gate (0x7C81B `dword[0x157750] != 0`,
 * match_run.c:589) and puts a grounded taker's height-table band
 * `d = [0x157750] - rec.pos_y` at 0x80 >= 0x38 (the airborne kick-route
 * predicate, 0x1102FE / fifa96_action_handlers.c:113-115) — the bootstrap the
 * live follow needs (T3 carries the live row selector). The offset tables are
 * the ported `match_kick_dir_x`/`_z` (native 0x10F334/0x10F33C). */
static void test_live_throw_in_camera_y_cut(void) {
  static const int expected_x[8] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int expected_z[8] = {1, 1, 0, -1, -1, -1, 0, 1};
  struct fixture f;
  make_fixture(&f);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &f.mr.entities.team[0].records[i];
    e->active = 1;
    e->pos_x = (int32_t)(i * 0x100);
    e->pos_z = 0;
    e->target_x = e->pos_x;
  }
  f.mr.record.entity_id = 1;               /* team 0, index 1 */
  f.mr.record.active = 1;
  f.mr.record.code = 0x10;
  f.mr.record.frame = 4;                   /* the +0x3D animation-frame gate */
  f.mr.record.delta = 0x80;                /* the whole-frame [0x157A64] */
  f.mr.record.stage92 = 0;
  f.mr.state.phase = 3u;                   /* the throw-in phase gate */
  f.mr.session_gate_14c32a = 1;            /* the byte-1 (Practise)
                                            * session staged (FU-166) */
  f.mr.goal_snap_x = 0x100;
  f.mr.goal_snap_y = 0;
  f.mr.goal_snap_z = 0x400;

  /* stage 0 placement -> 1 setup -> 2 probe wait */
  assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  /* the stage-3 cut loops over the eight face octants: the record's live face
   * octant (`type`, native +0x8E) selects the 0x10F334/0x10F33C offsets; the
   * staged `actor_type` (native +0x8B) must be ignored. */
  for (int oct = 0; oct < 8; oct++) {
    f.mr.record.stage92 = 2;
    f.mr.record.timer89 = 0x100;           /* crosses the 0x78 probe timer */
    f.mr.record.frame = 4;
    f.mr.record.pos_x = 0x110;
    f.mr.record.pos_z = 0x400;
    f.mr.record.type = (uint8_t)oct;
    f.mr.record.actor_type = (uint8_t)((oct + 3) & 7);   /* must be ignored */
    assert(fifa96_match_dispatch_action(&f.mr, 0x10) == FIFA96_OK);
    assert(f.mr.render.camera.pos_y == 0x80);
    assert(f.mr.render.camera.pos_x == 0x110 + expected_x[oct] * 0x40);
    assert(f.mr.render.camera.pos_z == 0x400 + expected_z[oct] * 0x40);
  }
  /* the BF20 tail gate's height operand (0x7C81B) is the camera Y: nonzero. */
  assert(f.mr.render.camera.pos_y != 0);
  /* the height-table band d = [0x157750] - rec.pos_y >= 0x38 (a grounded
   * taker: rec.pos_y = 0), the airborne-kick route precondition. */
  {
    int32_t d = (int32_t)f.mr.render.camera.pos_y - (int32_t)f.mr.record.pos_y;
    assert(d >= 0x38);
  }
  drop_fixture(&f);
}

/* FU-160 §2 (phase-13 T3): the row-0x0E live jump selector. Stage 0 builds
 * the target from the camera stand-in (0x71B9C) + the 0x8DCD4/0x79C50 metric
 * and face; stage 1 re-arms the self receiver timer (0x79B58) and installs
 * anim row 0x12 (frame 0, FUN_0006E598) once `timer89 >= 0x0C` (12 units at
 * delta 2 = 6 frames); stage 2 advances on the tracked (`[0x1577CA]`)/
 * jump_base gate; stage 3 resets when the anim row is terminal (the live
 * byte44 producer).
 * The phase gate resets any non-phase-2 call. */
static void test_action_0e_live_jump_selector(void) {
  struct fixture f;
  struct fifa96_match_entity *e;
  make_fixture(&f);
  e = &f.mr.entities.team[0].records[1];
  e->active = 1;
  f.mr.record.entity_id = 1;                /* team 0, index 1 */
  f.mr.record.active = 1;
  f.mr.record.code = 0x0E;
  f.mr.record.stage92 = 0;
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 2;
  f.mr.record.pos_x = 0x300;
  f.mr.record.pos_y = 0;
  f.mr.record.pos_z = 0x200;
  f.mr.record.type = 0;
  f.mr.render.camera.pos_x = 0x310;
  f.mr.render.camera.pos_y = 0;
  f.mr.render.camera.pos_z = 0x200;
  f.mr.state.phase = 2u;

  /* the phase gate resets (0x8271B..0x82726) */
  f.mr.state.phase = 1u;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0xFFu && e->stage92 == 0xFFu);
  f.mr.state.phase = 2u;
  f.mr.record.stage92 = 0;
  f.mr.record.timer89 = 0;

  /* stage 0 -> stage 1 in the same call (target/face/timer89 reset) */
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 1u);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.target_x == 0x310 && f.mr.record.target_z == 0x200);
  assert(e->timer93 == 0x10);               /* 0x79B58 on self */
  /* stage 1 waits: timer89 accumulates by delta (2/frame) */
  for (int i = 0; i < 5; i++)
    assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 1u);        /* 10 < 0x0C */
  assert(e->anim_id != 0x12);
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(e->anim_id == 0x12 && e->frame == 0u && e->row44 == 0u);
  assert(f.mr.record.anim_id == 0x12 && f.mr.record.ran == 1);
  assert(f.mr.record.stage92 == 2u && f.mr.record.timer89 == 0);

  /* stage 2 -> 3 on the camera-tracked gate: the native compare is
   * `CMP EBP,[0x1577CA]` (0x82806) — render.camera.tracked, NOT the
   * controlled [0x157A83] the pre-fix port read. */
  f.mr.entities.controlled = FIFA96_MATCH_ENTITY_NONE;
  f.mr.render.camera.tracked = f.mr.record.entity_id;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 3u);
  /* ... and on the +0x85 word (0x82812) when tracked does not name it */
  f.mr.record.stage92 = 2u;
  f.mr.render.camera.tracked = FIFA96_CAMERA_TRACKED_NONE;
  e->jump_base = 15;
  f.mr.record.jump_base = 15;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 3u);
  /* stage 3: a terminal anim row resets through the live +0x44 (row 0
   * flags 0x07 -> row44 1; the forced decision installs code 4 here) */
  f.mr.record.row44 = 1u;
  f.mr.entities.team[0].target = 1;
  e->type = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0u);
  assert(e->code == 4u);
  drop_fixture(&f);
}

/* FU-149 L13 (T1): the corner taker row 0x11. Stage 0 waits the 0x3C timer,
 * stage 1 runs the FUN_0007D360 corner probe (±0x710,0,±0xB00) ±0x50 and
 * anchors the record there, stages 2..4 advance/setup, stage 5's probe waits
 * on the 0x78 timer, stage 6 gates the lane, stage 7 kicks, resolves (phase 2
 * + the 0x12C offside timer) and installs the relay, stage 8 waits the +0x44
 * animation ack. */
static void test_action_11_runs_corner_body(void) {
  struct fixture f;
  make_fixture(&f);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &f.mr.entities.team[0].records[i];
    e->active = 1;
    e->pos_x = (int32_t)(i * 0x100);
    e->pos_z = 0;
    e->target_x = e->pos_x;
  }
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x11;
  f.mr.record.delta = 0x40;
  f.mr.record.stage92 = 0;
  f.mr.state.phase = 4u;                   /* the corner phase gate */
  f.mr.session_gate_14c32a = 1;
  f.mr.goal_snap_x = 5;                    /* + -> +0x710 */
  f.mr.goal_snap_z = -5;                   /* - -> -0xB00 */

  /* dispatch 1: the 0x3C wait passes, the probe places the record and the
   * cascade stops at the stage-5 probe */
  assert(fifa96_match_dispatch_action(&f.mr, 0x11) == FIFA96_OK);
  assert(f.mr.record.stage92 == 5u);
  assert(f.mr.record.controlled == 1);
  assert(f.mr.record.pos_x == 0x760 && f.mr.record.pos_z == -0xB50);
  assert(f.mr.record.place_x == 0x710 && f.mr.record.place_z == -0xB00);

  /* dispatch 2: timer 0x40 < 0x78 -> the probe still waits */
  assert(fifa96_match_dispatch_action(&f.mr, 0x11) == FIFA96_OK);
  assert(f.mr.record.stage92 == 5u);

  /* dispatch 3: timer 0x80 fires the probe; the kick, the resolution and the
   * relay install run and the row stops at the stage-8 +0x44 wait */
  assert(fifa96_match_dispatch_action(&f.mr, 0x11) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);
  assert(f.mr.referee.offside_suppress == 0x12C);
  assert(f.mr.record.stage92 == 8u);
  assert(f.mr.entities.team[0].target == 7);
  assert(f.mr.entities.team[0].records[7].code == 4u);

  /* dispatch 4: the +0x44 ack advances to stage 9 and the target clamp runs;
   * the row then waits on the timer/distance gate (distance 0 here -> reset) */
  f.mr.record.row44 = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x11) == FIFA96_OK);
  /* the stage-9 reset at phase 2 runs the FUN_0007DAB4 forced-decision
   * install (0x7C990), so the staged stage byte lands on the installed 0 */
  assert(f.mr.record.stage92 == 0u);
  assert(f.mr.entities.team[0].records[1].code == 3u);
  drop_fixture(&f);
}

/* FU-149 L13 (T1): the free-kick taker row 0x12. Stage 0 places the record at
 * the 0x158897 incident triple + the derived wall-point fold (the angle from
 * the incident to the own (0,0,±0xB10) probe, the 0xA0 step), stage 1 setup,
 * stage 2 the span-3 delivery probe, stage 3 the lane gate, stage 4 the kick +
 * resolution + relay. */
static void test_action_12_runs_free_kick_body(void) {
  struct fixture f;
  make_fixture(&f);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    struct fifa96_match_entity *e = &f.mr.entities.team[0].records[i];
    e->active = 1;
    e->pos_x = (int32_t)(i * 0x100);
    e->pos_z = 0;
    e->target_x = e->pos_x;
  }
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x12;
  f.mr.record.frame = 4;
  f.mr.record.delta = 0x80;
  f.mr.record.stage92 = 0;
  f.mr.state.phase = 7u;                   /* the free-kick phase gate */
  f.mr.session_gate_14c32a = 1;
  f.mr.incident_x = 0x500;
  f.mr.incident_z = 0;

  /* dispatch 1: incident placement + wall fold, then the stage-2 probe wait */
  assert(fifa96_match_dispatch_action(&f.mr, 0x12) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  assert(f.mr.record.controlled == 1);
  assert(f.mr.record.place_x == 0x500 && f.mr.record.place_z == 0);
  assert(f.mr.record.pos_x != 0x500 || f.mr.record.pos_z != 0);  /* folded */
  assert(f.mr.record.target_x == f.mr.record.pos_x);

  /* dispatch 2: probe fires (span 3 -> delivery row 3), kick + resolution.
   * The native tail 0x84480..0x84495 never writes [0x157A6A] (unlike rows
   * 0x10/0x11), so the offside timer stays clear. The relay 0x8DE8C pick
   * skips index 0 and the record's +0x9A self stamp: record 0 is parked on
   * the camera focus and record 5 wins the tie. */
  f.mr.entities.team[0].records[0].pos_x = 0x500;
  f.mr.entities.team[0].records[0].pos_z = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x12) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);
  assert(f.mr.referee.offside_suppress == 0);
  assert(f.mr.record.stage92 == 5u);
  assert(f.mr.entities.team[0].target == 5);           /* keeper 0 skipped */
  assert(f.mr.entities.team[0].records[5].code == 4u);

  /* dispatch 3: the +0x44 ack walks to the stage-7 reset */
  f.mr.record.row44 = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x12) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0u);   /* the forced-decision reset install */
  drop_fixture(&f);
}

/* FU-149 L13 (T1): the penalty taker row 0x13. Stage 0 commits the placement
 * and clears the ball staging block, stage 1 aims at the spot ±0xF0 and gates
 * the 0x30 distance, stage 2 waits on the opponent/slot/timer gate, stage 3
 * the lane gate, stage 4 the mode-0x40 penalty strike, stage 5 resolves
 * through situation 0xB once the ball ack clears (no offside timer), stage 6
 * the +0x44 reset. */
static void test_action_13_runs_penalty_body(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x13;
  f.mr.record.delta = 0x100;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_y = 0;
  f.mr.record.pos_z = 0x7E0;               /* the stage-1 spot -0xF0 (side 0) */
  f.mr.record.target_x = 0;
  f.mr.record.target_z = 0x7E0;
  f.mr.state.phase = 6u;                   /* the penalty phase gate */
  f.mr.session_gate_14c32a = 1;

  /* dispatch 1: the stage-1 distance gate passes, the stage-2 no-slot timer
   * (0xF0) waits */
  assert(fifa96_match_dispatch_action(&f.mr, 0x13) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  assert(f.mr.record.controlled == 1);
  assert(f.mr.record.ran == 1);            /* the +0x9E latch */
  assert(f.mr.record.place_x == 0 && f.mr.record.place_z == 0x8D0);
  assert(f.mr.render.camera.pos_z == 0x8D0);

  /* dispatch 2: the 0x100 timer crosses 0xF0, the strike runs and the row
   * waits at stage 5 on the ball actor/ack gate */
  assert(fifa96_match_dispatch_action(&f.mr, 0x13) == FIFA96_OK);
  assert(f.mr.record.stage92 == 5u);
  assert(f.mr.state.phase == 6u);          /* still phase 6 (unresolved) */

  /* dispatch 3 is the delivered record's ack path: the resolution writes
   * phase 2 without the row-10/11/12 offside timer */
  f.mr.entities.ball.pair.ack = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x13) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);
  assert(f.mr.referee.offside_suppress == 0);
  assert(f.mr.record.stage92 == 6u);

  /* dispatch 4: the +0x44 ack resets */
  f.mr.record.row44 = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x13) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0u);   /* the forced-decision reset install */
  drop_fixture(&f);
}

/* FU-142b (FU-142 Appendix C): the wired cluster-G row 26 runs the
 * `0x866F4..0x8681C` placement machine over `mr->record`: the stage-latch
 * walk (stage92 0 -> 1 -> 2), the 0x10F394 timer7b read, the ±6 * (active>>1)
 * target.z offset by active bit 0, the 0x8DCD4 lane gate (|dz| < 0x20 ->
 * retarget 0xCC0) and the 0x157A64 delta add. The staging/repack fields are
 * `stage92`/`timer7b`/`lane`; `player_d`/`player_e` stand in for the native
 * `rec[+4]` descriptor bytes and default to 0 in the pool path (Appendix C.5
 * open leg). */
static void test_action_26_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.pos_x = 0;
  f.mr.record.pos_y = 0;
  f.mr.record.pos_z = 0;
  f.mr.record.target_x = 0;
  f.mr.record.target_z = 0;
  f.mr.record.timer89 = 7;
  f.mr.record.delta = 0;
  f.mr.record.active = 3;      /* sign 1, magnitude 1 -> target.z = +6 */
  f.mr.record.stage92 = 1;
  f.mr.record.player_d = 0;    /* table[0] = 6 -> timer7b = 3 */
  f.mr.record.player_e = 0;
  assert(f.mr.record.lane == 0);

  assert(fifa96_match_dispatch_action(&f.mr, 0x26) == FIFA96_OK);
  assert(f.mr.record.target_x == 0xCC0);
  assert(f.mr.record.target_z == 0);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 2);
  assert(f.mr.record.timer7b == 3);
  assert(f.mr.record.lane == 6);

  /* past the latch only the prologue advances (timer89 += delta, timer7b) */
  f.mr.record.timer89 = 1;
  f.mr.record.delta = 2;
  f.mr.record.target_x = 0x123;
  assert(fifa96_match_dispatch_action(&f.mr, 0x26) == FIFA96_OK);
  assert(f.mr.record.timer89 == 3);
  assert(f.mr.record.target_x == 0x123);
  assert(f.mr.record.stage92 == 2);
  drop_fixture(&f);
}

/* FU-142d (FU-142 Appendix G): the wired cluster-G row 28 runs the
 * `0x870E8..0x874E3` 4-arm machine over `mr->record`. The staged arm is the
 * record's stage92; the prologue runs the 0x8DCD4 out triple + 0x79C50 face,
 * arm 0 builds the set-piece target and the 0x114E04 fold, arm 1 waits on the
 * +0xA2 gate (here it fires and syncs target = pos with flag830 clear). The
 * run RNG is seeded by begin (seed 0; first draw 512 -> +0xA2 0x20), the five
 * process globals default to 0 (their native producers are unported, OL-56)
 * and the derived scratch cells round-trip through the run record. */
static void test_action_28_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  /* stage 3 (the shared epilogue): prologue only; no RNG needed */
  f.mr.record.stage92 = 3;
  f.mr.record.pos_x = 0;
  f.mr.record.target_x = 0x200;
  f.mr.record.type = 0x55;
  f.mr.record.timer89 = 7;
  assert(fifa96_match_dispatch_action(&f.mr, 0x28) == FIFA96_OK);
  assert(f.mr.record.type == 2);       /* +x face octant */
  assert(f.mr.record.timer89 == 7);
  assert(f.mr.record.stage92 == 3);

  /* stage 0 inside the distance gate: arm 0 writes the set-piece target, the
   * run RNG draw seeds +0xA2 and arm 1 waits in the same call */
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_z = 0;
  f.mr.record.target_x = 0;
  f.mr.record.target_z = 0;
  f.mr.record.active = 1;              /* 22.5 deg: x +133, z +55 */
  f.mr.record.side = 0;
  f.mr.record.global_157ac2 = 4;       /* no side-0 negation */
  f.mr.record.global_10f364 = 0x100;
  f.mr.record.global_10f368 = 0x200;
  f.mr.record.timer89 = 5;
  f.mr.record.delta = 0;
  f.mr.record.scratch_a2 = 0;
  f.mr.record.anim_id = 0;               /* OL-80 selector input */
  assert(fifa96_match_dispatch_action(&f.mr, 0x28) == FIFA96_OK);
  assert(f.mr.record.target_x == 0x100 + 133);
  assert(f.mr.record.target_z == 0x200 + 55);
  assert(f.mr.record.stage92 == 1);    /* arm 0 latch + arm 1 wait */
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.scratch_a2 == 0x20); /* seed-0 draw 512 & 0x7F + 0x20 */
  /* OL-80: pins the anim_id write-back; the constant kind cannot
   * discriminate the input staging (render round-trip fixture does). */
  assert(f.mr.record.anim_id == 0x15);

  /* stage 1 with the gate fired and flag830 clear: target = pos, id 1, the
   * +0xA2 re-arm (seed-0 second draw 1829 -> 0x45) */
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 0xFFFF;
  f.mr.record.scratch_a2 = 1;
  f.mr.record.flag830 = 0;
  f.mr.record.pos_x = 0x77;
  f.mr.record.pos_y = 0x55;
  f.mr.record.pos_z = 0x88;
  f.mr.record.target_x = 0;
  f.mr.record.target_y = 0;
  f.mr.record.target_z = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x28) == FIFA96_OK);
  assert(f.mr.record.target_x == 0x77 && f.mr.record.target_z == 0x88);
  assert(f.mr.record.target_y == 0x55);  /* target = pos copies all three axes */
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 1);
  assert(f.mr.record.scratch_a2 == 0x45);
  drop_fixture(&f);
}

/* FU-142e (FU-142 Appendix H): the wired cluster-G row 2A runs the
 * `0x86A34..0x87010` 12-arm machine over `mr->record`; the staged arm is the
 * record's stage92 and the native `+0x65` distance word is
 * `mr->record.distance` (staged by the frame path from pos/target; set
 * directly here). Arm 0 writes (-0x720, 0), clears team flag830 and both
 * globals and advances to stage 1; arm 8 draws four run-RNG words into the
 * target. A no-op handler would leave the target and stage untouched. */
static void test_action_2A_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.stage92 = 0;
  f.mr.record.timer89 = 5;
  f.mr.record.delta = 3;
  f.mr.record.target_x = 0x111;
  f.mr.record.target_z = 0x222;
  f.mr.record.distance = 0;
  f.mr.record.flag830 = 7;
  f.mr.record.global_10f358 = 9;
  f.mr.record.global_10f35c = 9;
  f.mr.record.anim_id = 0x99;            /* OL-80 selector input */
  assert(fifa96_match_dispatch_action(&f.mr, 0x2A) == FIFA96_OK);
  assert(f.mr.record.target_x == -0x720);
  assert(f.mr.record.target_z == 0);
  assert(f.mr.record.anim_id == 0x60);   /* OL-80: arm-0 0x60 select output */
  assert(f.mr.record.flag830 == 0);
  assert(f.mr.record.global_10f358 == 0);
  assert(f.mr.record.global_10f35c == 0);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 1);

  /* arm 8 through the run RNG (seed 63: 0x7801, 0xEB29, 0xD746, 0x3DC7):
   * target = (-1, +297), timer89 = 0, latch advances to 9 */
  assert(fifa96_rng_seed(&f.mr.rng, 63) == FIFA96_OK);
  f.mr.record.stage92 = 8;
  f.mr.record.distance = 0;
  f.mr.record.pos_z = 0;
  f.mr.record.timer89 = 5;
  f.mr.record.delta = 0;
  f.mr.record.target_x = 0x10;
  f.mr.record.target_z = 0x20;
  assert(fifa96_match_dispatch_action(&f.mr, 0x2A) == FIFA96_OK);
  assert(f.mr.record.target_x == -1);
  assert(f.mr.record.target_z == 297);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 9);
  drop_fixture(&f);
}

/* FU-142e: the FU-142a arm's overflow (all records 1..10 occupied) leaves
 * `chosen831 == NONE` and no 0x2A install; the arm tail clears the `[0x10F35C]`
 * chase flag (`0x8D80E`). Dispatching row 2A anyway is still FIFA96_OK because
 * the body reads no chosen-record field. */
static void test_action_2A_overflow_fixture(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.state.phase = 0x14;
  f.mr.phase_machine.state = 0x14;
  f.mr.phase_machine.side_controlled = 0;   /* team 0 controlled */
  f.mr.global_10f35c = 1;
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    f.mr.entities.team[0].records[i].skip_9a = 1;
  assert(fifa96_match_phase_machine_step(&f.mr) == FIFA96_OK);
  assert(f.mr.phase_machine.arm2a_overflow == 1);
  assert(f.mr.entities.team[0].chosen831 == FIFA96_MATCH_ENTITY_NONE);
  assert(f.mr.global_10f35c == 0);          /* 0x8D80E arm tail clear */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(f.mr.entities.team[0].records[i].code != 0x2A);

  f.mr.record.stage92 = 0;
  f.mr.record.distance = 0;
  f.mr.record.delta = 0;
  f.mr.record.target_x = 0x111;
  assert(fifa96_match_dispatch_action(&f.mr, 0x2A) == FIFA96_OK);
  assert(f.mr.record.target_x == -0x720);
  assert(f.mr.record.stage92 == 1);
  drop_fixture(&f);
}

/* FU-142b (M2 arms-and-wiring Task 4, FU-142 Appendix D) + FU-142f (Task 9,
 * FU-142 Appendix I): row 27's body `0x86820..0x86A02` is ported
 * (`fifa96_arm_27_step` + the 0x79C50/0x6E598 helpers) but its entry is
 * unresolved: the only reference to `0x86820` in the program is the
 * action-table slot `0x11077C` itself, the two `MOV EDX,0x27` sites are
 * animation-row arguments (0x756D5 -> `CALL 0x6E598`, 0x1F52E -> the
 * FUN_0001F440 jump-table tail), `MOV ECX,0x27` has no site and the 0x8CEB8
 * arms stage no code 0x27. FU-142f extends the census to all 77
 * `FUN_0007D9A4` call sites including every register-derived argument
 * (Appendix I): none passes 0x27 and no stored installer pointer exists. The
 * row therefore stays `fn == NULL` with the FU-142f/OL-48 marker and still
 * dispatches UNSUP. */
static void test_action_27_unwired_entry(void) {
  struct fixture f;
  const struct fifa96_match_handler *row = &fifa96_match_action_table[0x27];
  make_fixture(&f);
  assert(row->fn == NULL);
  assert(strstr(row->evidence, "FU-142b") != NULL);
  assert(strstr(row->evidence, "FU-142f") != NULL);
  assert(strstr(row->evidence, "OL-48") != NULL);
  assert(strstr(row->evidence, "UNSUPPORTED") != NULL);
  assert(action_expect[0x27] == UNSUP);
  assert(fifa96_match_dispatch_action(&f.mr, 0x27) == UNSUP);
  drop_fixture(&f);
}

/* FU-142b (M2 arms-and-wiring Task 5, FU-142 Appendix E) + FU-142f (Task 9,
 * FU-142 Appendix I): row 2C's body `0x84598..0x8462D` is ported
 * (`fifa96_arm_2c_step` + the shared `fifa96_arm_reset`) but its entry is
 * unresolved: the only reference to `0x84598` in the program is the
 * action-table slot `0x110790` itself, the three `MOV EDX,0x2C` sites are
 * non-match constants (0x3035E/0x40A8E/0x40C5C, none calls FUN_0007D9A4),
 * `MOV ECX,0x2C` has no site and the 0x8CEB8 arms stage no code 0x2C.
 * FU-142f's exhaustive 77-site installer census (Appendix I) finds no 0x2C
 * invocation and no stored installer pointer. The row therefore stays
 * `fn == NULL` with the FU-142f/OL-48 marker and still dispatches UNSUP. */
static void test_action_2C_unwired_entry(void) {
  struct fixture f;
  const struct fifa96_match_handler *row = &fifa96_match_action_table[0x2C];
  make_fixture(&f);
  assert(row->fn == NULL);
  assert(strstr(row->evidence, "FU-142b") != NULL);
  assert(strstr(row->evidence, "FU-142f") != NULL);
  assert(strstr(row->evidence, "OL-48") != NULL);
  assert(strstr(row->evidence, "UNSUPPORTED") != NULL);
  assert(action_expect[0x2C] == UNSUP);
  assert(fifa96_match_dispatch_action(&f.mr, 0x2C) == UNSUP);
  drop_fixture(&f);
}

/* FU-142c (M2 arms-and-wiring Task 6, FU-142 Appendix F) + FU-142f (Task 9,
 * FU-142 Appendix I): row 29's body `0x874E4..0x87738` is ported
 * (`fifa96_arm_29_step`) but its entry is unresolved: the only reference to
 * `0x874E4` in the program is the action table slot `0x110784` itself; the
 * sole `MOV EDX,0x29` site is `0x1F53C` inside the non-match `FUN_0001F440`
 * (tail `MOV EAX,0x6B; CALL 0x13600`, no installer call); the phase-5
 * handler `0x6E05C..0x6E1B2` (the body's own phase gate) contains no
 * `FUN_0007D9A4` call; and the body's own `0x8753C` install is code 3 (the
 * phase != 5 self-install). FU-142f's exhaustive 77-site installer census
 * (Appendix I) finds no 0x29 invocation and no stored installer pointer. The
 * row therefore stays `fn == NULL` with the FU-142f/OL-48 marker and still
 * dispatches UNSUP. */
static void test_action_29_unwired_entry(void) {
  struct fixture f;
  const struct fifa96_match_handler *row = &fifa96_match_action_table[0x29];
  make_fixture(&f);
  assert(row->fn == NULL);
  assert(strstr(row->evidence, "FU-142c") != NULL);
  assert(strstr(row->evidence, "FU-142f") != NULL);
  assert(strstr(row->evidence, "OL-48") != NULL);
  assert(strstr(row->evidence, "UNSUPPORTED") != NULL);
  assert(action_expect[0x29] == UNSUP);
  assert(fifa96_match_dispatch_action(&f.mr, 0x29) == UNSUP);
  drop_fixture(&f);
}

/* FU-142f (M2 arms-and-wiring Task 9, FU-142 Appendix I): row 2B's native
 * entry 0x87738 is the final `RET` of the row-29 body — the shared epilogue
 * (FU-142 §1.1), not a standalone stub — and the exhaustive installer census
 * finds no 0x2B invocation anywhere. Dead entry: `fn == NULL`, expectation
 * UNSUP, evidence records the FU-142f dead verdict. */
static void test_dead_2b_evidence(void) {
  struct fixture f;
  const struct fifa96_match_handler *row = &fifa96_match_action_table[0x2B];
  make_fixture(&f);
  assert(row->fn == NULL);
  assert(strstr(row->evidence, "FU-142f") != NULL);
  assert(strstr(row->evidence, "dead") != NULL);
  assert(strstr(row->evidence, "0x87738") != NULL);
  assert(action_expect[0x2B] == UNSUP);
  assert(fifa96_match_dispatch_action(&f.mr, 0x2B) == UNSUP);
  drop_fixture(&f);
}

/* FU-147 S1: the row-04 staging takes the lane fields from the pool
 * (`s.bound = e->bound` native +0x77, `s.word6f = e->cam_dz6f` native +0x6F,
 * `s.is_team_7c7` from the team camera-nearest tracker). The bound gate
 * (`0x7EDCE` `lane > bound` -> return before half_line) and the half_line
 * angle `fifa96_entity_angle(word6d, word6f)` (`0x7EE4B`) make the staging
 * observable: half_line clears `team+0x7B6` when the record is the team
 * second, so the pool `second` goes NONE only when bound admits and the
 * word6d/word6f angle stays <= 0xAB. */
static void test_action_04_stages_lane_fields(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.record.active = 1;
  f.mr.record.has_slot = 0;
  f.mr.record.timer81 = 0;
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 0;
  f.mr.record.anim_id = 0;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_y = 0;
  f.mr.record.pos_z = 0;
  f.mr.entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  f.mr.entities.team[0].second = 0;          /* the record is the team second */
  f.mr.entities.team[0].records[0].lane_x = 0x20;   /* native word +0x6B */
  f.mr.entities.team[0].records[0].lane_z = 0x100;  /* native word +0x6D */
  f.mr.entities.team[0].records[0].cam_dz6f = 0x100;/* native word +0x6F */
  f.mr.entities.ball.y = 0;

  /* bound 0x30 admits the 0x20 lane; angle (0x100, 0x100) = 0x80 <= 0xAB ->
   * half_line clears the team second. */
  f.mr.entities.team[0].records[0].bound = 0x30;    /* native word +0x77 */
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(f.mr.entities.team[0].second == FIFA96_MATCH_ENTITY_NONE);

  /* bound 0x10 rejects the lane at 0x7EDCE: the second survives. (Case A set
   * the team target to self through the half_line write, so clear it back to
   * keep the early 0x7E939 arm out of the way.) */
  f.mr.entities.team[0].second = 0;
  f.mr.entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  f.mr.entities.team[0].records[0].bound = 0x10;
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(f.mr.entities.team[0].second == 0);

  /* bound admits but cam_dz6f 0 gives angle 0x100 > 0xAB: the second
   * survives. */
  f.mr.entities.team[0].second = 0;
  f.mr.entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  f.mr.entities.team[0].records[0].bound = 0x30;
  f.mr.entities.team[0].records[0].cam_dz6f = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(f.mr.entities.team[0].second == 0);
  drop_fixture(&f);
}

/* FU-171 §7 rec 2 (T2): the row-04 half-line sound arm's side compare reads
 * `[0x1577CA]` — the camera-tracked record's owner side byte — not the
 * controlled actor's side. Fresh bytes 0x7EE83..0x7EEA3: `ECX = [0x1577CA]`;
 * skip when ECX == 0 or rec == ECX; `AL = [rec_owner+0x826]` vs
 * `[tracked_owner+0x826]`; draw only on equality. The arm's only engine
 * observable is its `fifa96_rng_step` draw (its 0x974F0/0x651F0 sinks have no
 * consumer), so this test counts RNG movement across one row-04 dispatch:
 * with the tracked record on the OTHER team from the row record there is no
 * draw; tracked on the record's own team (and the TRACKED_NONE controlled
 * fallback) witness that the arm can still fire. Pre-re-stage the
 * `id == controlled` side stand-in drew in the other-team case (the RED). */
static void test_action_04_ball_track_side_from_tracked(void) {
  struct fixture f;
  struct fifa96_rng before;
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;                 /* team 0, index 0 */
  f.mr.record.active = 1;
  f.mr.record.has_slot = 1;                  /* slot_word6 0 -> no backup arm */
  f.mr.record.timer81 = 0;
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 0;
  f.mr.record.anim_id = 0;                   /* row byte != 0x13 */
  f.mr.record.pos_x = 0;
  f.mr.record.pos_y = 0;
  f.mr.record.pos_z = 0;
  f.mr.entities.team[0].target = FIFA96_MATCH_ENTITY_NONE;
  f.mr.entities.team[1].target = FIFA96_MATCH_ENTITY_NONE;
  f.mr.entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  f.mr.entities.team[0].side = 0;
  f.mr.entities.team[1].side = 1;
  f.mr.entities.controlled = 2;              /* team-0 mate: present, not self */
  f.mr.entities.team[0].records[0].lane_x = 0x20;   /* native word +0x6B */
  f.mr.entities.team[0].records[0].lane_z = 0x100;  /* native word +0x6D */
  f.mr.entities.team[0].records[0].cam_dz6f = 0x100;/* native word +0x6F */
  f.mr.entities.team[0].records[0].bound = 0x30;    /* native word +0x77 */
  f.mr.render.camera.pos_y = 0;              /* ball_height <= 0x50 */

  /* tracked on the other team: the side compare refuses (no draw). */
  f.mr.render.camera.tracked = (int32_t)FIFA96_MATCH_ENTITY_RECORDS + 3;
  before = f.mr.rng;
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(memcmp(&before, &f.mr.rng, sizeof before) == 0);

  /* tracked on the record's own team: the arm draws. */
  f.mr.entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  f.mr.render.camera.tracked = 3;
  before = f.mr.rng;
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(memcmp(&before, &f.mr.rng, sizeof before) != 0);

  /* TRACKED_NONE: the controlled stand-in keeps the witness firing. */
  f.mr.entities.team[0].second = FIFA96_MATCH_ENTITY_NONE;
  f.mr.render.camera.tracked = FIFA96_CAMERA_TRACKED_NONE;
  before = f.mr.rng;
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(memcmp(&before, &f.mr.rng, sizeof before) != 0);
  drop_fixture(&f);
}

/* FU-147 S1: the row-1E stage-3 span tail (0x75A40..0x75B6F, first-hand
 * this slice): at the stage-3 span with `+0x9B == 0` the native resets the
 * record (`0x7DAB4`), calls situation 0xB (`0x8A938`, phase -> 2) and installs
 * code 5 (`0x7D9A4`, the carrier claim; the install is the engine's deferred
 * `record.install` request). With `+0x9B != 0` the native takes the 0x760DF
 * arm (FU-147 leg 2) and this derived subset does nothing. The stage-3 entry
 * gate from 0x75795 and the camera-focus writes stay FU-147 legs 1/13. */
static void test_action_1E_stage3_possession_flip(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.entity_id = 1;      /* team 0 record 1 */
  f.mr.record.stage92 = 3;        /* the stage-3 span dispatch */
  f.mr.record.has_ball = 0;
  f.mr.record.has_slot = 0;
  f.mr.state.phase = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);              /* situation 0xB */
  assert(f.mr.phase_machine.state == 2u);
  assert(f.mr.record.install == 5);            /* the code-5 install request */
  /* the 0x7DAB4 reset writes +0x92 = 0xFF; its code-0 re-install is
   * same-code rejected on the fresh record, so the byte stays 0xFF. */
  assert(f.mr.entities.team[0].records[1].stage92 == 0xFF);
  assert(f.mr.entities.team[0].records[1].code == 0);

  /* already holding the ball: the arm must not fire (leg 2 arm). */
  f.mr.state.phase = 1;
  f.mr.phase_machine.state = 1;
  f.mr.record.install = 0;
  f.mr.record.has_ball = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.state.phase == 1u);
  assert(f.mr.record.install == 0);

  /* stage 1 (the claim span): no situation, no code-5 install. */
  f.mr.record.stage92 = 1;
  f.mr.record.has_ball = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.state.phase == 1u);
  assert(f.mr.record.install == 0);
  drop_fixture(&f);
}

/* FU-151 P3: row 1E's ten-stage machine through the engine binder. Stage 5
 * with row 0x45 / frame 3 releases the ball and falls through 6/7 (ball
 * staging + situation 0xB)/8/9 to the common exit; the stage-7 restart writes
 * the live phase 2. */
static void test_action_1E_machine_release_chain(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.entity_id = 1;
  f.mr.record.stage92 = 5;
  f.mr.record.has_ball = 1;
  f.mr.record.anim_id = 0x45;       /* OL-80 live row: byte[[rec+0x28]] */
  f.mr.record.frame = 3;
  f.mr.record.row44 = 1;
  f.mr.record.place_offset_x = 1;
  f.mr.record.place_offset_z = 2;
  f.mr.state.phase = 1;
  f.mr.phase_machine.state = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.has_ball == 0);            /* stage 5 release */
  assert(f.mr.state.phase == 2u);               /* stage 7 situation 0xB */
  assert(f.mr.phase_machine.state == 2u);
  assert(f.mr.record.stage92 == 9);             /* 5->6->7->8->9 */
  assert(f.mr.record.install == 0);             /* no install on this path */
  assert(f.mr.keeper_latch_157ab2 == 0);        /* stage 7 clears the latch */
  assert(f.mr.record.timer7b == 3);             /* the head write */
  /* M2 phase-9 T3 (FU-148 §2.1(c) 0x75DC4): the stage-5 release cuts the FU-71
   * camera to the staged release triple (0x15774C/50/54 = pos + offset<<6,
   * y 0x50), posts the FUN_00071C94 event (vec 0, EBX = word[0x157750] = 0x50,
   * ECX 0) and binds the tracked record ([0x1577CA] = rec, 0x71D27). */
  assert(f.mr.render.camera.pos_x == f.mr.record.pos_x + (1 << 6));
  assert(f.mr.render.camera.pos_y == 0x50);
  assert(f.mr.render.camera.pos_z == f.mr.record.pos_z + (2 << 6));
  assert(f.mr.render.camera.event_param == 0x50);
  assert(f.mr.render.camera.vel_x == 0 && f.mr.render.camera.vel_z == 0);
  assert(f.mr.render.camera.tracked == f.mr.record.entity_id);

  /* stage 4 with no slot clears the outlet vector and the band selects the
   * 0x44 event; the chain reaches stage 8 and exits on row44. */
  f.mr.phase_machine.state = 2;
  f.mr.state.phase = 1;
  f.mr.record.stage92 = 4;
  f.mr.record.has_ball = 1;
  f.mr.record.anim_id = 0;
  f.mr.record.frame = 0;
  f.mr.record.row44 = 0;
  f.mr.keeper_reset_x = 0x300;
  f.mr.keeper_reset_z = 0;
  f.mr.keeper_latch_157ab2 = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 8);
  assert(f.mr.record.has_ball == 1);            /* no release at row 0 */
  drop_fixture(&f);
}

/* FU-151 P3: row 1D's five-stage close-down through the engine binder. Stage 0
 * (row44 + timer gate) places/commits and advances; stage 3 with no slot
 * clears the outlet vector, runs situation 0xB and resets the record. */
static void test_action_1D_runs_closedown(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.entity_id = 1;
  f.mr.record.stage92 = 0;
  f.mr.record.row44 = 1;
  f.mr.record.timer89 = 0x1E;
  f.mr.keeper_cam_x = 10;
  f.mr.keeper_cam_y = 20;
  f.mr.keeper_cam_z = 30;
  f.mr.state.phase = 1;
  f.mr.phase_machine.state = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1D) == FIFA96_OK);
  assert(f.mr.record.place_valid == 1);
  assert(f.mr.record.place_x == 10 && f.mr.record.place_y == 20 &&
         f.mr.record.place_z == 30);
  assert(f.mr.record.controlled == 1);         /* stage < 3 actor bind */
  assert(f.mr.record.helper_request == 1);     /* stage < 3 slot merge */
  assert(f.mr.record.stage92 == 1);            /* stage 0 -> stage 1 */
  assert(f.mr.keeper_latch_157ab2 == 1);

  /* stage 3 no slot: situation 0xB -> phase 2 and the 0x7DAB4 reset. */
  f.mr.state.phase = 1;
  f.mr.phase_machine.state = 1;
  f.mr.record.stage92 = 3;
  f.mr.record.has_slot = 0;
  f.mr.record.row44 = 0;
  f.mr.record.helper_request = 0;
  f.mr.record.controlled = 0;
  f.mr.record.place_valid = 0;
  f.mr.keeper_latch_157ab2 = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1D) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);
  assert(f.mr.phase_machine.state == 2u);
  assert(f.mr.record.install == 0);            /* 1D installs no code */
  assert(f.mr.record.stage92 == 4);            /* stage 3 -> 4 then the tail */
  drop_fixture(&f);
}

/* FU-139 §8 (M2 arms-and-wiring Task 10) + M2 phase-9 T2 (FU-142 OL-63):
 * row 05's carrier machine (`fifa96_action_carrier_arm`, native
 * `0x7F194..0x7F665` stages 0-3) is now wired over the pool: the claim
 * (`0x7F1BF..0x7F205`, `[0x158724] = rec` + the `0x158728..0x15872F` block
 * reset), the team-target bind (`0x7F20B..0x7F217`), the capped/additive
 * timer (`0x7F221..0x7F23A`), the camera target copy (`0x7F24E..0x7F258`),
 * the stage-0 gates/control/dir bytes (`0x7F274..0x7F397`), the
 * `0x7876C` merge request and stages 1-3 (the `0x92820`/`0x6E598` sinks, the
 * `0x79B1C` snap, the `0x79C50` face and the stage-3 hand-off). The
 * stage-0 target algebra (`0x7F3A1..0x7F57B`) and the `FUN_0007F7E0`
 * fallback stay OL-63 legs (the machine reports them as requests with no
 * engine consumer). */
static void test_action_05_wired_carrier(void) {
  struct fixture f;
  const struct fifa96_match_handler *row = &fifa96_match_action_table[0x05];
  make_fixture(&f);
  assert(row->fn != NULL);
  assert(strstr(row->evidence, "FU-139") != NULL);
  assert(strstr(row->evidence, "OL-63") != NULL);   /* residual leg named */
  assert(action_expect[0x05] == FIFA96_OK);
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  drop_fixture(&f);
}

/* Row 05 over the pool: claim, team target, control, camera target, dirs and
 * the stage 1-3 walk. The possession-block fields `pos_*` are the derived
 * `0x158728..0x15872F`; `ball.carrier` is `[0x158724]`. */
static void test_action_05_claim_dirs_and_stages(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 5;
  f.mr.record.stage92 = 0;
  f.mr.record.has_slot = 1;
  f.mr.record.timer89 = 0x10;
  f.mr.record.delta = 2;
  f.mr.record.dir_x = 0x12;
  f.mr.record.dir_z = (int8_t)0xF0;
  f.mr.entities.team[0].records[1].lane_x = 0x20;
  f.mr.entities.team[0].records[1].bound = 0x40;
  f.mr.entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  f.mr.entities.ball.pos_index = 3;
  f.mr.entities.ball.pos_rotation = 2;
  f.mr.entities.ball.pos_dir_x = 9;
  f.mr.entities.ball.pos_dir_z = -9;
  f.mr.entities.ball.pos_counter_c = 7;
  f.mr.entities.ball.pos_release = 6;
  f.mr.entities.ball.pos_counter_e = 5;
  f.mr.entities.ball.pos_counter_f = 4;
  f.mr.entities.controlled = 5;   /* pre-set: set_control must re-request */
  f.mr.render.camera.pos_x = 7;
  f.mr.render.camera.pos_y = 8;
  f.mr.render.camera.pos_z = 9;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.entities.ball.carrier == 1);            /* 0x7F1FF claim */
  assert(f.mr.entities.ball.pos_index == 0 && f.mr.entities.ball.pos_rotation == 0);
  assert(f.mr.entities.ball.pos_dir_x == 0x12 &&
         (uint8_t)f.mr.entities.ball.pos_dir_z == 0xF0);   /* slot-dir arm */
  assert(f.mr.entities.ball.pos_counter_c == 0 &&
         f.mr.entities.ball.pos_release == 0 &&
         f.mr.entities.ball.pos_counter_e == 0 &&
         f.mr.entities.ball.pos_counter_f == 0);          /* block reset */
  assert(f.mr.entities.team[0].target == 1);
  assert(f.mr.entities.team[0].second == FIFA96_MATCH_ENTITY_NONE);
  assert(f.mr.record.controlled == 1);                      /* set_control */
  assert(f.mr.record.target_x == 7 && f.mr.record.target_y == 8 &&
         f.mr.record.target_z == 9);                        /* camera copy */
  assert(f.mr.record.timer89 == 0x12);                      /* +delta */
  assert(f.mr.record.stage92 == 0 && f.mr.record.ran == 1);
  assert(f.mr.record.helper_request == 0);

  /* lane > 0x40 clears the controlled actor immediately ([0x157A83] = 0). */
  f.mr.entities.team[0].records[1].lane_x = 0x41;
  f.mr.entities.controlled = 1;
  f.mr.record.controlled = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.entities.controlled == FIFA96_MATCH_ENTITY_NONE);
  assert(f.mr.record.controlled == 0);

  /* The team-gate path (no slot, no type height) raises the slot-merge
   * request; the close > bound wait returns before the dir arm, after the
   * set_control request. */
  f.mr.entities.team[0].records[1].lane_x = 0x20;
  f.mr.record.has_slot = 0;
  f.mr.entities.team[0].slot_pool = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);                  /* 0x7F356 */
  assert(f.mr.record.controlled == 1);

  /* stage 1: the 0x92820 sink, the camera-velocity zero request, the
   * 0x6E598 animation id, the timer clear and the latch advance. */
  f.mr.record.stage92 = 1;
  f.mr.record.timer89 = 0x55;
  f.mr.record.has_slot = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2);
  assert(f.mr.record.timer89 == 0);

  /* stage 2: the 0x79B1C snap (target := pos) and the 0x79C50 face; a zero
   * slot word waits at the latch, a live one loops to stage 0. */
  f.mr.record.stage92 = 2;
  f.mr.record.pos_x = 0x11;
  f.mr.record.pos_y = 0x22;
  f.mr.record.pos_z = 0x33;
  f.mr.record.dir_x = 0x20;
  f.mr.record.dir_z = 0;
  f.mr.slot.entity = 1;
  f.mr.slot.released = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2);
  assert(f.mr.record.target_x == 0x11 && f.mr.record.target_y == 0x22 &&
         f.mr.record.target_z == 0x33);
  assert(f.mr.record.type == 2);                            /* octant 2 */
  f.mr.slot.released = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0);

  /* stage 3: the +0x44 wait, the latch reset and the team-target hand-off
   * (code 4 on the staged ball actor + the 0x79B58 receiver timer). */
  f.mr.record.stage92 = 3;
  f.mr.record.row44 = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.record.stage92 == 3);
  f.mr.record.row44 = 1;
  f.mr.entities.team[0].target = 1;
  f.mr.entities.ball.pair.actor = 5;
  f.mr.entities.ball.pair.receiver = 6;
  assert(fifa96_match_dispatch_action(&f.mr, 0x05) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0);
  assert(f.mr.entities.team[0].records[5].code == 4);
  assert(f.mr.entities.team[0].records[6].timer93 == 0x10);
  drop_fixture(&f);
}


/* FU-142 Appendix K.5/K.6 (M2 playability-legs Tasks 1/2 / OL-70/OL-70a):
 * rows 04 and 08 are wired — `fifa96_match_action_04`/`_08` bind
 * `fifa96_outfield_row04_step`/`_row08_step` over `mr->record`/the FU-141
 * pool (the full record-visible bodies and the pool binding; row 08 installs
 * no code, it is the stage-0/1/2 approach/scan). The run assertions exercise
 * the row-04 prologue reset + no-slot camera target and the row-08 reset,
 * stage-0 camera/face, the +0x3D-gated projection scan and the stage-2
 * snap. */
static void test_action_04_and_08_wired(void) {
  struct fixture f;
  const struct fifa96_match_handler *r8 = &fifa96_match_action_table[0x08];
  make_fixture(&f);
  assert(fifa96_match_action_table[0x04].fn != NULL);
  assert(strstr(fifa96_match_action_table[0x04].evidence, "OL-70") != NULL);
  assert(r8->fn != NULL);
  assert(strstr(r8->evidence, "OL-70a") != NULL);
  assert(strstr(r8->evidence, "K.6") != NULL);
  assert(action_expect[0x04] == FIFA96_OK);
  assert(action_expect[0x08] == FIFA96_OK);
  assert(fifa96_match_dispatch_action(&f.mr, 0x08) == FIFA96_OK);

  /* phase != 2: the row's FUN_0007DAB4 request resets the pool record. */
  f.mr.state.phase = 1;
  f.mr.record.entity_id = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(f.mr.entities.team[0].records[0].stage92 == 0xFF);
  assert(f.mr.entities.team[0].records[0].timer89 == 0);

  /* phase 2, no slot, w1577F0 0: the camera+lead target arm writes the
   * staging record; lane < 0x3C0 with |camera.z| > 0x570 requests the
   * 0x79B58 receiver timer on the pool record. */
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.record.has_slot = 0;
  f.mr.record.active = 1;
  f.mr.record.timer81 = 0;
  f.mr.entities.team[0].records[0].lane_x = 0x20;   /* native word +0x6B */
  f.mr.render.camera.pos_x = 7;
  f.mr.render.camera.pos_y = 8;
  f.mr.render.camera.pos_z = 0x600;
  assert(fifa96_match_dispatch_action(&f.mr, 0x04) == FIFA96_OK);
  assert(f.mr.record.target_x == 7 && f.mr.record.target_y == 8 &&
         f.mr.record.target_z == 0x600);
  assert(f.mr.entities.team[0].records[0].timer93 == 0x10);

  /* Row 08, phase != 2: the 0x81080 reset request lands on the pool record. */
  drop_fixture(&f);
  make_fixture(&f);
  f.mr.state.phase = 1;
  f.mr.record.entity_id = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x08) == FIFA96_OK);
  assert(f.mr.entities.team[0].records[0].stage92 == 0xFF);
  assert(f.mr.entities.team[0].records[0].timer89 == 0);

  /* Row 08 stage 0, slotted: the 0x15774C camera copy into the target, the
   * 0x79C50 face (lane 0x20, word +0x6D 0 -> angle 0x100 -> octant 2), the
   * +0x9E latch and the stage advance to 1. */
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.record.stage92 = 0;
  f.mr.record.active = 1;
  f.mr.record.has_slot = 1;
  f.mr.record.timer89 = 0;
  f.mr.record.type = 0;
  f.mr.entities.team[0].records[0].lane_x = 0x20;   /* face dir X */
  f.mr.entities.team[0].records[0].lane_z = 0;      /* face dir Z */
  f.mr.render.camera.pos_x = 7;
  f.mr.render.camera.pos_y = 8;
  f.mr.render.camera.pos_z = 9;
  assert(fifa96_match_dispatch_action(&f.mr, 0x08) == FIFA96_OK);
  assert(f.mr.record.target_x == 7 && f.mr.record.target_y == 8 &&
         f.mr.record.target_z == 9);
  assert(f.mr.record.stage92 == 1 && f.mr.record.timer89 == 0);
  assert(f.mr.record.type == 2);   /* native +0x8E octant */
  assert(f.mr.record.ran == 1);

  /* Row 08 stage 0 with the native +0x3D frame gate open: the 0x79C50 face
   * (lane 1/1 -> angle 0x80 -> octant 1), the projection scan (both sine
   * words 0xB504, offset 0 distance 0 -> hit) and the record arm advance the
   * stage, latch +0x9E and zero timer89. */
  drop_fixture(&f);
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.record.stage92 = 0;
  f.mr.record.active = 1;
  f.mr.record.has_slot = 1;
  f.mr.record.frame = 1;            /* native +0x3D (producer OL-80/OL-82) */
  f.mr.record.type = 0;
  f.mr.entities.team[0].records[0].lane_x = 1;   /* face dir X */
  f.mr.entities.team[0].records[0].lane_z = 1;   /* face dir Z */
  f.mr.render.camera.pos_x = 0;
  f.mr.render.camera.pos_y = 0;
  f.mr.render.camera.pos_z = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x08) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.type == 1);    /* native +0x8E octant */
  assert(f.mr.record.ran == 1);

  /* Row 08 stage 2: the 0x79B1C snap copies the position into the target. */
  f.mr.record.stage92 = 2;
  f.mr.record.pos_x = 0x11;
  f.mr.record.pos_y = 0x22;
  f.mr.record.pos_z = 0x33;
  f.mr.record.timer89 = 4;
  assert(fifa96_match_dispatch_action(&f.mr, 0x08) == FIFA96_OK);
  assert(f.mr.record.target_x == 0x11 && f.mr.record.target_y == 0x22 &&
         f.mr.record.target_z == 0x33);
  drop_fixture(&f);
}

/* FU-139 §9 (Task 11): the wired row-07 machine. Stage 0 gates/latches and
 * the stage-target arm run over `mr->record`; the stage-2 `+0x44` tail resets
 * and hands the ball/receiver through the pool. The kick requests run the
 * 0x7B9C4 path on `mr->entities.ball.pair`. */
static void test_action_07_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  /* phase != 2 -> the 0x81702 tail reset. */
  f.mr.state.phase = 1;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0x100;
  assert(fifa96_match_dispatch_action(&f.mr, 0x07) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0xFF);
  assert(f.mr.record.timer89 == 0);

  /* stage 0 target: lane 0 (<= 0x40), word[+0x5D] 0, ball height 0x40 ->
   * the camera-copy target arm (slot word 0 != 0x60/0x8000). */
  drop_fixture(&f);
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.entities.ball.y = 0x40;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_y = 0;
  f.mr.record.lane = 0;
  f.mr.record.entity_id = 0;
  f.mr.record.has_slot = 1;
  f.mr.render.camera.pos_x = 7;
  f.mr.render.camera.pos_y = 8;
  f.mr.render.camera.pos_z = 9;
  assert(fifa96_match_dispatch_action(&f.mr, 0x07) == FIFA96_OK);
  assert(f.mr.record.ran == 1);
  assert(f.mr.record.stage92 == 1);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.target_x == 7 && f.mr.record.target_y == 8 &&
         f.mr.record.target_z == 9);

  /* stage 1: the slot word 0 (has_slot) selects mode 0, the kick runs on the
   * staged ball block, then the latch advances. */
  f.mr.record.stage92 = 1;
  f.mr.record.actor_type = 1;
  f.mr.record.has_slot = 1;
  f.mr.record.target_x = 0x321;   /* the stage-1 kick path must carry it */
  assert(fifa96_match_dispatch_action(&f.mr, 0x07) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2);
  assert(f.mr.record.target_x == 0x321);
  assert(f.mr.record.type == 0);  /* no face arm ran: +0x8E untouched */
  assert(f.mr.entities.ball.pair.actor == 0);
  assert(f.mr.entities.ball.pair.code != 0);
  /* Final-review sentinel guard: entity_id 0 (team-0 record 0) with no long-
   * ball request (`timer7cb` 0) must not read as `is_team_cb` — the native
   * 0x815D0 compares record pointers, never NULL — so the stage-1 mode is the
   * slot word (0), not the 0x40 long-ball arm. */
  assert(f.mr.entities.team[0].timer7cb == 0);
  assert((f.mr.entities.ball.pair.flags & 0x40u) == 0);

  /* stage 2 without byte[+0x44] (unmodeled, OL-65) waits. */
  f.mr.record.stage92 = 2;
  assert(fifa96_match_dispatch_action(&f.mr, 0x07) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2);
  drop_fixture(&f);
}

/* FU-139 §9 erratum (fix round 1): the kick machine's 0x110680/0x7C990 gates
 * index the native byte +0x91 (the action code), which the record staging now
 * carries in `record.code`; `record.type` is only the +0x8E face octant. The
 * row-07 stage-1 decision is reachable through the `staged_code == 3` SI arm
 * (`fifa96_action_kick.staged_code` = the pool ball pair code): with
 * `record.code = 4` the gate passes and installs 0x0E, with the native kick
 * code 7 the gate refuses and the kick proceeds. `record.type` is left at the
 * old divergent value (4 would pass it) to prove it is not consulted. The
 * 0x7C990 type-5 carrier check is discriminated at the loader level
 * (`test_action_handlers.c::test_kick_machine_07_reset_decision`): in the
 * engine the team-target tail's ball install 4 overwrites `record.install`
 * either way, so it has no engine-visible observable. */
static void test_action_07_decision_uses_code_byte(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.record.stage92 = 1;        /* stage 1 */
  f.mr.record.has_slot = 0;
  f.mr.entities.ball.pair.code = 3;   /* SI = 0x40 through the staged-code arm */
  f.mr.record.lane = 0x100;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_z = 0x7B0;
  f.mr.render.camera.pos_x = 0x10;
  f.mr.render.camera.pos_y = 0x30;
  f.mr.render.camera.pos_z = 0x7B0;

  f.mr.record.code = 4;           /* flat[4] & 1 == 1: decision fires */
  f.mr.record.type = 7;           /* the +0x8E octant must not be the gate */
  assert(fifa96_match_dispatch_action(&f.mr, 0x07) == FIFA96_OK);
  assert(f.mr.record.install == 0x0E);

  f.mr.record.code = 7;           /* flat[7] & 1 == 0: decision refuses */
  f.mr.record.type = 4;           /* the octant 4 would have passed the gate */
  f.mr.record.install = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x07) == FIFA96_OK);
  assert(f.mr.record.install == 0);
  drop_fixture(&f);
}

/* FU-139 §9 (Task 11): the wired row-0F machine stage 0 (the 0x8DCD4 nudge,
 * the 0x79B6C re-anchor and the anim request) and the stage-1 reload. */
static void test_action_0F_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.stage92 = 0;
  f.mr.record.active = 1;
  f.mr.record.pos_x = 0x100;
  f.mr.record.pos_z = 0x200;
  f.mr.render.camera.pos_x = 0x100;
  f.mr.render.camera.pos_z = 0x300;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0F) == FIFA96_OK);
  assert(f.mr.record.ran == 1);
  assert(f.mr.record.stage92 == 1);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.target_x == 0x100);
  assert(f.mr.record.target_z == 0x200);

  /* Stage 1: the derived bound word [+0x77] has no pool producer (OL-65), so
   * the lane > bound gate waits and the timer reload never runs; the loader
   * fixture `test_kick_machine_0F_reload_and_corner` covers the reload. */
  f.mr.record.stage92 = 1;
  f.mr.record.lane = 0x20;
  f.mr.record.timer81 = 7;
  f.mr.record.pos_y = 0x40;
  f.mr.entities.ball.y = 0x40;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0F) == FIFA96_OK);
  assert(f.mr.record.timer81 == 7);
  assert(f.mr.record.stage92 == 1);
  drop_fixture(&f);
}

/* M2 arms-and-wiring Task 12 / FU-142 OL-27+OL-32: the wired rows 18/21/23.
 * Row 18 is the downed/claim slot hand-off machine (`0x849B0..0x84AE1`): the
 * resolution path (0x4C324 bind + `+0x9A` latch + NSEARCH/SWAP + `0x7DAB4`
 * reset subset). Row 21 is the controlled receive/claim machine
 * (`0x85214..0x8539B`): the phase/tracked gate, the camera copy, the stage
 * gates and the `0x8DE8C`/`0x4A` resolution arm. Row 23 is the tackle/lunge
 * machine (`0x82F84..0x83163`): the stage gates, the `0x82DD0` attempt, the
 * stage-1 target arm and the installs 0x0E/0x0F. */
static void test_action_18_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.entity_id = 0;
  f.mr.record.pos_x = 0x200;
  f.mr.record.pos_z = 0x200;
  f.mr.record.stage92 = 2;
  f.mr.record.timer89 = 0x78;
  f.mr.record.distance = 0x1F;   /* < 0x20: resolution regardless of input */
  f.mr.record.has_slot = 1;
  f.mr.entities.team[0].records[0].has_slot = 1;
  f.mr.entities.team[0].records[0].code = 0x18;
  f.mr.entities.team[0].records[1].pos_x = 0x210;
  f.mr.entities.team[0].records[1].pos_z = 0x200;
  f.mr.entities.team[0].records[1].has_slot = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x18) == FIFA96_OK);
  assert(f.mr.record.timer7b == 2);       /* word[+0x7B] = 2 (0x849B7) */
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 0xFF);    /* the 0x7DAB4 reset ran */
  assert(f.mr.entities.team[0].records[0].skip_9a == 1);  /* +0x9A latch */
  assert(f.mr.entities.team[0].records[0].has_slot == 0);
  assert(f.mr.entities.team[0].records[1].has_slot == 1); /* SWAP moved it */
  assert(f.mr.entities.team[0].records[0].code == 0x18);  /* rejected install */
  drop_fixture(&f);

  /* Resolution without a slot: timer > 0x12C forces it regardless of the
   * distance and input byte. */
  make_fixture(&f);
  f.mr.record.entity_id = 0;
  f.mr.record.stage92 = 2;
  f.mr.record.timer89 = 0x12D;
  f.mr.record.distance = 0xFF;
  f.mr.record.has_slot = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x18) == FIFA96_OK);
  assert(f.mr.entities.team[0].records[0].skip_9a == 1);
  assert(f.mr.record.stage92 == 0xFF);
  drop_fixture(&f);

  /* The +0x65 distance gate: timer 0x78 with distance 0x20 and a zero input
   * byte waits (no resolution, no +0x9A latch). */
  make_fixture(&f);
  f.mr.record.entity_id = 0;
  f.mr.record.stage92 = 2;
  f.mr.record.timer89 = 0x78;
  f.mr.record.distance = 0x20;
  f.mr.record.has_slot = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x18) == FIFA96_OK);
  assert(f.mr.entities.team[0].records[0].skip_9a == 0);
  assert(f.mr.record.stage92 == 2);
  drop_fixture(&f);
}

static void test_action_21_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  /* phase != 2 -> the 0x7DAB4 reset subset. */
  f.mr.state.phase = 1;
  f.mr.record.entity_id = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x21) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0xFF && f.mr.record.timer89 == 0);
  drop_fixture(&f);

  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.entities.controlled = -1;   /* not the controlled record */
  assert(fifa96_match_dispatch_action(&f.mr, 0x21) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0xFF);
  drop_fixture(&f);

  /* phase 2 + the controlled record: camera copy then the stage-0 advance
   * through the 0x8DE8C nearest and the 0x4A animation id. */
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.entities.controlled = 0;
  f.mr.record.entity_id = 0;
  f.mr.record.pos_x = 0x200;
  f.mr.record.pos_z = 0x200;
  f.mr.record.stage92 = 0;
  f.mr.record.active = 1;
  f.mr.record.lane = 0;            /* word[+0x6B] <= 0x40 */
  f.mr.record.timer89 = 0x100;
  f.mr.entities.team[0].records[3].pos_x = 0x210;
  f.mr.entities.team[0].records[3].pos_z = 0x200;
  f.mr.render.camera.pos_x = 7;
  f.mr.render.camera.pos_y = 8;
  f.mr.render.camera.pos_z = 9;
  assert(fifa96_match_dispatch_action(&f.mr, 0x21) == FIFA96_OK);
  assert(f.mr.record.ran == 1);
  assert(f.mr.record.stage92 == 1 && f.mr.record.timer89 == 0);
  assert(f.mr.record.target_x == 7 && f.mr.record.target_y == 8 &&
         f.mr.record.target_z == 9);
  drop_fixture(&f);
}

static void test_action_23_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  /* stage 0: the ran latch and the latch advance. */
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.record.stage92 = 0;
  f.mr.record.active = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x23) == FIFA96_OK);
  assert(f.mr.record.ran == 1);
  assert(f.mr.record.stage92 == 1 && f.mr.record.timer89 == 0);
  /* stage 1 with the unmodeled camera/track inputs at 0: the attempt refuses
   * and the install-0F gate falls to the tail (no install). */
  f.mr.record.stage92 = 1;
  f.mr.record.has_slot = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x23) == FIFA96_OK);
  assert(f.mr.record.install == 0);
  assert(f.mr.record.stage92 == 1);
  drop_fixture(&f);

  /* stage 0 inactive -> the 0x7DAB4 reset; phase 2 + inactive takes the code
   * 0 install, which is accepted for an unoccupied record. */
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 0;
  f.mr.record.stage92 = 0;
  f.mr.record.active = 0;
  f.mr.entities.team[0].records[0].code = 0x23;   /* the current action code */
  assert(fifa96_match_dispatch_action(&f.mr, 0x23) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0);
  assert(f.mr.record.timer89 == 0 && f.mr.record.ran == 0);
  assert(f.mr.entities.team[0].records[0].code == 0);
  drop_fixture(&f);
}

/* FU-139 §11 (M2 arms-and-wiring Task 13 / OL-30): the wired row 06 pursuit
 * machine (`0x801B4..0x809EF`). The reset path, the carrier-gate install 4,
 * and the full no-slot V1 target + install 8/anim path over the pool. */
static void test_action_06_runs_body(void) {
  struct fixture f;
  make_fixture(&f);
  /* phase != 2 -> the shared 0x7DAB4 reset (code-0 install on the pool). */
  f.mr.state.phase = 1;
  f.mr.record.entity_id = 1;
  f.mr.entities.team[0].records[1].code = 0x06;
  assert(fifa96_match_dispatch_action(&f.mr, 0x06) == FIFA96_OK);
  /* The 0x7DAB4 code-0 install clears the +0x9E latch (0x7DA88) and stages
   * +0x92 = 0 (0x7DA95, the accepted install). */
  assert(f.mr.record.ran == 0);
  assert(f.mr.record.stage92 == 0 && f.mr.record.timer89 == 0);
  assert(f.mr.entities.team[0].records[1].code == 0);
  drop_fixture(&f);

  /* Carrier absent -> the early install 4 (invoke) request. */
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.entities.ball.carrier = FIFA96_MATCH_ENTITY_NONE;
  assert(fifa96_match_dispatch_action(&f.mr, 0x06) == FIFA96_OK);
  assert(f.mr.record.install == 4);
  assert(f.mr.record.ran == 1);
  drop_fixture(&f);

  /* Carrier present, no slot: the camera metric/fold path. team 0 side 0 ->
   * V2 = {0,0,-0xB10}; camera (0,0,0) -> dx 0, dz -0xB10 -> scaled 0xB1,
   * angle 0x200; V1 folds to (0, 0, -0xB1); actor == team target -> target =
   * V1; the V4 metric is 0 -> install 8. */
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.state.frame_delta = 0;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_z = 0;
  f.mr.record.timer81 = 0;
  f.mr.record.delta = 0;
  f.mr.entities.team[0].target = 1;
  f.mr.entities.ball.carrier = 5;
  f.mr.entities.ball.y = 0;
  f.mr.render.camera.pos_x = 0;
  f.mr.render.camera.pos_y = 0;
  f.mr.render.camera.pos_z = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x06) == FIFA96_OK);
  assert(f.mr.record.install == 8);
  assert(f.mr.record.target_x == 0);
  assert(f.mr.record.target_z == -0xB1);
  assert(f.mr.record.ran == 1);
  drop_fixture(&f);

  /* pass_parity 1 runs the claim arm: mate 2 at the target (0,0,-0xB1) beats
   * the actor (0xB1 away) -> team+0x7B2 = entity id 2, team+0x7B6 = NONE. */
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.state.frame_delta = 0;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_z = 0;
  f.mr.record.timer81 = 0;
  f.mr.record.delta = 0;
  f.mr.entities.team[0].target = 1;
  f.mr.entities.ball.carrier = 5;
  f.mr.entities.ball.y = 0;
  f.mr.entities.team[0].records[2].pos_x = 0;
  f.mr.entities.team[0].records[2].pos_z = -0xB1;
  f.mr.pass_parity = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x06) == FIFA96_OK);
  assert(f.mr.entities.team[0].target == 2);
  assert(f.mr.entities.team[0].second == FIFA96_MATCH_ENTITY_NONE);
  drop_fixture(&f);

  /* Live scores feed the 0x803EE conditional second subtraction: score[0] 0
   * < score[1] 1 with timer89 0x10 and delta 0 subtracts the timer twice, so
   * the side-0 fold puts target z at -0x91 (a single subtraction gives
   * -0xA1). */
  make_fixture(&f);
  f.mr.state.phase = 2;
  f.mr.state.frame_delta = 0;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.pos_x = 0;
  f.mr.record.pos_z = 0;
  f.mr.record.timer81 = 0;
  f.mr.record.timer89 = 0x10;
  f.mr.record.delta = 0;
  f.mr.score[0] = 0;
  f.mr.score[1] = 1;
  f.mr.entities.team[0].target = 1;
  f.mr.entities.ball.carrier = 5;
  f.mr.entities.ball.y = 0;
  f.mr.render.camera.pos_x = 0;
  f.mr.render.camera.pos_y = 0;
  f.mr.render.camera.pos_z = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x06) == FIFA96_OK);
  assert(f.mr.record.target_x == 0);
  assert(f.mr.record.target_z == -0x91);
  assert(f.mr.record.install == 8);
  drop_fixture(&f);
}

static void test_phase_rows_dispatch_per_classification(void) {
  struct fixture f;
  make_fixture(&f);
  for (uint32_t phase = 0; phase < FIFA96_MATCH_PHASE_ROWS; phase++) {
    int rc = fifa96_match_dispatch_phase(&f.mr, (uint8_t)phase);
    assert(rc == phase_expect[phase]);
    if (phase == FIFA96_MATCH_PHASE_INT3_SLOT)
      assert(rc == NOTF);
    else if (fifa96_match_phase_table[phase].fn == NULL)
      assert(rc == UNSUP);           /* an unported row never returns OK */
  }
  drop_fixture(&f);
}

/* Out-of-range codes are NOT_FOUND (never OK), including the byte maximum. */
static void test_out_of_range_is_not_found(void) {
  struct fixture f;
  make_fixture(&f);
  assert(fifa96_match_dispatch_action(&f.mr, FIFA96_MATCH_ACTION_ROWS) == NOTF);
  assert(fifa96_match_dispatch_action(&f.mr, 0x2Du) == NOTF);
  assert(fifa96_match_dispatch_action(&f.mr, 0xFFu) == NOTF);
  assert(fifa96_match_dispatch_phase(&f.mr, FIFA96_MATCH_PHASE_ROWS) == NOTF);
  assert(fifa96_match_dispatch_phase(&f.mr, 0x23u) == NOTF);
  assert(fifa96_match_dispatch_phase(&f.mr, 0xFFu) == NOTF);
  drop_fixture(&f);
}

/* NULL run -> -INVALID for both dispatchers and the seam runner. */
static void test_null_arguments_are_invalid(void) {
  struct fixture f;
  make_fixture(&f);
  assert(fifa96_match_dispatch_action(NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_dispatch_phase(NULL, 0) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_dispatch_row(NULL, &fifa96_match_action_table[0]) == -FIFA96_ERR_INVALID);
  assert(fifa96_match_dispatch_row(&f.mr, NULL) == -FIFA96_ERR_INVALID);
  drop_fixture(&f);
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
  struct fixture f;
  make_fixture(&f);
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
  drop_fixture(&f);
}

/* FU-143 §11 (M2 playable-match Task 2 / OL-84 residual): the wired row 01
 * kickoff body (native 0x7DBC0..0x7DFC8, stage table 0x7DBB0 = targets
 * 0x7DC6B/0x7DCAF/0x7DD29/0x7DFB2). The `phase == 1` gate (0x7DBD3); the
 * marker branch (0x7DBDC..0x7DC39); the timer accumulation (0x7DC3A..0x7DC50);
 * stage 0 (0x7DC6B: +0x9E, [0x5882A] gate, timer89 >= 0x3C -> timer 0, stage 1,
 * fall into stage 1); stage 1 (0x7DCAF: 0x8DE8C nearest, lane <= 0x40 gates,
 * the no-slot timer89 > 0x78 arm -> timer 0, stage 2, fall into stage 2); stage
 * 2 (0x7DD29..0x7DF95: nearest from the record position, team+0x7B2 = nearest,
 * the FUN_0008A938 situation 0xB call at 0x7DF90 -> phase 2, timer 0, stage
 * 3); the tail (0x7DFB2) checks +0x44 (staged zero, OL-68). The camera reset,
 * the event/sound sinks (0x974DC/0x8F188/0x92820), the 0x8DCD4 metric and the
 * 0x7A490 ball staging stay derived requests/legs (OL-84). */
static void test_action_01_runs_kickoff_body(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.entity_id = 1;   /* team 0 record 1, the derived kickoff taker */

  /* gate: only phase 1 runs; a mismatched phase jumps to 0x7DFB8, the
   * unconditional FUN_0007DAB4 reset (the jump lands past the stage-3 +0x44
   * test). FU-147 S1: record 1's `+0x8D` seed is index 1, so the reset's
   * phase-2 `active != 0` arm (0x7DAE6 -> 0x7C990) runs the forced decision
   * (code 3 for a record that is neither team target nor second) and the
   * code-3 install stages +0x92 = 0. */
  f.mr.state.phase = 2;
  f.mr.record.stage = 0;
  f.mr.record.timer89 = 7;
  f.mr.record.delta = 1;
  f.mr.record.target_x = -5;
  f.mr.record.stage92 = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 0);
  assert(f.mr.entities.team[0].records[1].stage92 == 0);
  assert(f.mr.entities.team[0].records[1].code == 3);
  assert(f.mr.entities.team[0].records[1].timer89 == 0);
  assert(f.mr.record.target_x == -5);
  assert(f.mr.record.ran == 0);
  assert(f.mr.record.controlled == 0);
  assert(f.mr.record.place_valid == 0);

  /* marker branch (stage < 2): the FUN_000700F4 camera reset (derived constant
   * triple [0x10F328/2C/30] = (0,0,0)), the [0x157A83] actor bind, the
   * +/-0x30 kickoff x and z = 0, the FUN_0007876C merge request; stage 0 sets
   * +0x9E and, with [0x5882A] clear, accumulates timer89 and waits. */
  f.mr.state.phase = 1;
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 2;
  f.mr.record.target_x = -5;
  f.mr.record.target_z = 0x77;
  f.mr.record.stage92 = 0;   /* the gate reset above left 0xFF */
  f.mr.global_5882a = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.record.ran == 1);
  assert(f.mr.record.timer89 == 2);
  assert(f.mr.record.controlled == 1);
  assert(f.mr.record.place_valid == 1);
  assert(f.mr.record.place_x == 0 && f.mr.record.place_y == 0 &&
         f.mr.record.place_z == 0);
  assert(f.mr.record.target_x == -0x30);
  assert(f.mr.record.target_z == 0);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.stage92 == 0);

  /* positive-side target mirror */
  f.mr.record.target_x = 7;
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.record.target_x == 0x30);

  /* stage 0 fires at timer89 >= 0x3C once [0x5882A] is set and falls into
   * stage 1 in the same call; the stage-0 advance zeroes the timer, so stage
   * 1's no-slot arm (timer89 > 0x78) waits and the run stays at phase 1. */
  f.mr.record.timer89 = 0x3C;
  f.mr.global_5882a = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.record.stage92 == 1);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.state.phase == 1u);

  /* stage 1 past the 0x78 no-slot gate advances to stage 2 in the same call
   * and the derived situation 0xB writes the live phase 2; the stage-2
   * nearest (origin = the record position, skip = active) rewrites team+0x7B2
   * and the stage latch lands at 3. */
  f.mr.record.timer89 = 0x79;
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);
  assert(f.mr.state.prev_phase == 1u);
  assert(f.mr.phase_machine.state == 2u);
  assert(f.mr.entities.team[0].target == 1);   /* nearest to pos (0,0) */
  assert(f.mr.record.stage92 == 3u);
  assert(f.mr.record.timer89 == 0);
  /* G3 (M2 playable-match Task 3 / OL-87/88/89): the row's only situation call
   * is the ported table-2 arm 0x7DF90 (0xB), a phase write; it is not a score
   * source. No goal invoker is reachable from the ported rows, so the derived
   * FUN_00093944 writer cells stay fresh through the kickoff body. */
  assert(f.mr.score[0] == 0 && f.mr.score[1] == 0);
  assert(f.mr.score_last_side == -1 && f.mr.score_tracked_side == 1);
  assert(f.mr.score_max_diff == 0 && f.mr.score_last_event == 0);

  /* Discriminating marker case (T2 review): the 0x7DBDC source is byte +0x92
   * (`[EBP+0x8F]>>24`), i.e. `stage92`, not the never-written `record.stage`.
   * `stage92 = 3` with `stage = 0` takes the position-copy branch; a read of
   * `record.stage` would take the marker branch instead and leave the wrong
   * target/requests behind. */
  f.mr.state.phase = 1;
  f.mr.record.stage = 0;
  f.mr.record.stage92 = 3;
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 2;
  f.mr.record.pos_x = 0x777;
  f.mr.record.pos_y = 0x123;
  f.mr.record.pos_z = -0x555;
  f.mr.record.target_x = 0x111;
  f.mr.record.target_y = 0x222;
  f.mr.record.target_z = 0x333;
  f.mr.record.place_valid = 0;
  f.mr.record.controlled = 0;
  f.mr.record.helper_request = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.record.target_x == 0x777);
  assert(f.mr.record.target_y == 0x123);
  assert(f.mr.record.target_z == -0x555);
  assert(f.mr.record.place_valid == 0);
  assert(f.mr.record.controlled == 0);
  assert(f.mr.record.helper_request == 0);
  assert(f.mr.record.timer89 == 2);
  assert(f.mr.state.phase == 1u);

  /* M2 interactive Task 1: with the record bound to the live slot, stage 1
   * takes the native `word[slot+6] & 0x70` arm (0x7DCE9..0x7DCF6) instead of
   * the no-slot timer arm. */
  f.mr.state.phase = 1;
  f.mr.record.stage92 = 1;
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 2;
  f.mr.record.has_slot = 1;
  f.mr.slot.entity = f.mr.record.entity_id;
  f.mr.slot.released = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.record.stage92 == 1u);          /* waits for the release word */
  assert(f.mr.state.phase == 1u);
  f.mr.slot.released = 0x40;                  /* a released button bit */
  assert(fifa96_match_dispatch_action(&f.mr, 0x01) == FIFA96_OK);
  assert(f.mr.state.phase == 2u);             /* stage 2 -> situation 0xB */
  assert(f.mr.record.stage92 == 3u);
  f.mr.slot.released = 0;
  f.mr.record.has_slot = 0;
  f.mr.slot.entity = -1;
  drop_fixture(&f);
}

/* M2 phase-9 T1 / FU-75 L4.6 (FU-138 OL-18): row 02 restart/placement.
 *
 * First-hand /FIFA96.EXE this task (disassemble_bytes 0x7DFCC..0x7E1A2):
 *  - `0x7DFD7` byte[+0x9E] = 1 (ran);
 *  - phase 1 (`0x7DFE6..0x7E016`): target x = -dword[[team+0x7B2]+0x59],
 *    target z = 0, `+0x89` = 0, `+0x92` = 0, then the tail return; the y dword
 *    `+0x51` is not written;
 *  - phase 2 (`0x7E01B`): `[team+0x7B2] = rec`; no slot + `byte[team+0x828]`
 *    -> `CALL 0x7876C`; with a slot `FUN_0007D9A4(rec, 4, EBX=0, ECX=1)` and a
 *    direct return (`0x7E046..0x7E068`) — the install-4 invoke-now; without a
 *    slot the camera triple target (`0x7E069..0x7E089`), `+0x89 += delta` and
 *    the `+0x92` stage machine (`0x7E08A..0x7E0F5`) whose stage-0 wait is
 *    `lane > 0x40 ? 0x78 : 0xA` with the `FUN_0007DAB4` reset on the
 *    `lane > 0x40` ready path (`0x7E0C8`);
 *  - any other phase (`0x7E192`): `FUN_0007DAB4` reset.
 * The stage-1 arm (nearest/metric/event/ball staging, `0x7E0F6..0x7E184`) and
 * the stage-2 snap (`0x7E185`) stay OL-18 legs. */
static void test_action_02_restart_and_phase2_arms(void) {
  struct fixture f;
  struct fifa96_match_entity *rec;
  make_fixture(&f);
  f.mr.record.entity_id = 1;                 /* team 0 record 1 */
  rec = &f.mr.entities.team[0].records[1];
  f.mr.entities.team[0].target = 1;          /* [team+0x7B2] = rec */

  /* phase 1: the restart mirror, z = 0, timers/stage reset; y untouched. */
  f.mr.state.phase = 1;
  rec->pos_x = 0x1234;
  f.mr.record.target_x = -5;
  f.mr.record.target_y = 0x777;
  f.mr.record.target_z = 0x555;
  f.mr.record.timer89 = 7;
  f.mr.record.stage92 = 3;
  assert(fifa96_match_dispatch_action(&f.mr, 0x02) == FIFA96_OK);
  assert(f.mr.record.ran == 1);
  assert(f.mr.record.target_x == -0x1234);
  assert(f.mr.record.target_z == 0);
  assert(f.mr.record.target_y == 0x777);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 0);

  /* phase 2, no slot: camera triple target, timer += delta, stage-0 wait. */
  f.mr.state.phase = 2;
  f.mr.record.has_slot = 0;
  f.mr.record.stage92 = 0;
  f.mr.record.timer89 = 0;
  f.mr.record.delta = 2;
  f.mr.record.lane = 0x20 << 16;             /* lane 0x20 <= 0x40 -> 0xA */
  f.mr.entities.team[0].slot_pool = 1;
  f.mr.record.helper_request = 0;
  assert(fifa96_camera_init(&f.mr.render.camera, 0x11, 0x22, 0x33) == FIFA96_OK);
  assert(fifa96_match_dispatch_action(&f.mr, 0x02) == FIFA96_OK);
  assert(f.mr.entities.team[0].target == 1);
  assert(f.mr.record.helper_request == 1);   /* 0x7E041 CALL 0x7876C */
  assert(f.mr.record.target_x == 0x11);
  assert(f.mr.record.target_y == 0x22);
  assert(f.mr.record.target_z == 0x33);
  assert(f.mr.record.timer89 == 2);
  assert(f.mr.record.stage92 == 0);          /* waits */

  /* the 0xA threshold fires the stage advance (timer zeroed, stage 1). */
  f.mr.record.timer89 = 0xA;
  f.mr.record.helper_request = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x02) == FIFA96_OK);
  assert(f.mr.record.stage92 == 1);
  assert(f.mr.record.timer89 == 0);

  /* the `lane > 0x40` ready path runs FUN_0007DAB4 before the advance
   * (0x7E0C8): the reset installs the forced code (rec is [team+0x7B2], so 4)
   * and stages `+0x92 = 0`, then the shared advance increments to 1 (the
   * native fall-through into the stage-1 arm stays the OL-18 leg). */
  f.mr.record.stage92 = 0;
  f.mr.record.timer89 = 0x78;
  f.mr.record.lane = 0x41 << 16;
  rec->active = 1;
  rec->code = 2;
  assert(fifa96_match_dispatch_action(&f.mr, 0x02) == FIFA96_OK);
  assert(rec->code == 4u);
  assert(f.mr.record.stage92 == 1);
  assert(f.mr.record.timer89 == 0);

  /* phase 2 with a slot: [team+0x7B2] = rec, install 4 invoke-now; the
   * record's timer81 early-returns the invoked row 04 so the installed code
   * is observable. */
  f.mr.record.has_slot = 1;
  rec->has_slot = 1;
  rec->code = 2;
  rec->timer81 = 1;
  f.mr.record.timer81 = 1;
  f.mr.record.stage92 = 0;
  f.mr.record.timer89 = 0x55;
  assert(fifa96_match_dispatch_action(&f.mr, 0x02) == FIFA96_OK);
  assert(f.mr.entities.team[0].target == 1);
  assert(rec->code == 4u);                   /* FUN_0007D9A4(rec, 4, 0, ECX=1) */
  assert(rec->timer89 == 0);                 /* installer 0x7DA7E */
  assert((f.mr.dispatched_ok & (1ull << 0x04u)) != 0u);  /* invoke-now ran row 04 */

  /* any other phase: FUN_0007DAB4 reset. */
  f.mr.state.phase = 0;
  f.mr.record.stage92 = 3;
  f.mr.record.timer89 = 9;
  rec->code = 2;
  assert(fifa96_match_dispatch_action(&f.mr, 0x02) == FIFA96_OK);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 0);          /* code-0 install staged byte */
  drop_fixture(&f);
}

/* ===== Phase-14 T1 / FU-167 + FU-168: the row-0x0E stage-2 else arm ========
 *
 * Fresh bytes `0x82820..0x82A9E` (verified this task): when the record is NOT
 * camera-tracked (`rec != [0x1577CA]`, `0x82806`) and has no live jump arc
 * (`word[+0x85] == 0`, `0x82812`), the in-body gates
 * `0x20 < (int32)[0x157750] < 0x70` (`0x82828..0x82834`) and
 * `(int16)word[+0x6B] < 0x60` (`0x8283a..0x82843`, the pool `lane_x`) must
 * hold, else the record stays at stage 2 and retries (the `CL = 0` tail).
 * On success the arm re-places the camera focus at
 * `rec.pos + 0x40*0x10F334/3C[type]` (`0x82849..0x82893`), builds the pan
 * vector and calls `FUN_00071C94` (`0x82A6C`) — which binds `[0x1577CA] = rec`
 * (`0x71D27`) and clears `byte[rec+0x9B]` (`0x71D2D`) — then advances
 * `timer89 = 0; stage -> 3` unconditionally (`CL = 1`, `0x82A95`). */
static void test_action_0e_stage2_else_arm_binds_camera(void) {
  struct fixture f;
  struct fifa96_match_entity *e;
  fifa96_camera *cam;
  struct fifa96_rng rng_copy;
  uint16_t draw1 = 0;
  uint16_t draw2 = 0;
  make_fixture(&f);
  e = &f.mr.entities.team[0].records[1];
  e->active = 1;
  e->lane_x = 0x20;                 /* word +0x6B < 0x60 */
  e->has_ball = 1;                  /* 0x71D2D clears it on the bind */
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x0E;
  f.mr.record.stage92 = 2;
  f.mr.record.timer89 = 5;
  f.mr.record.delta = 2;
  f.mr.record.pos_x = 0x300;
  f.mr.record.pos_y = 0;
  f.mr.record.pos_z = 0x200;
  f.mr.record.type = 0;             /* dir (0,+1) -> focus = pos + (0,0x40) */
  f.mr.record.face7d = 0x100;
  f.mr.record.speed71 = 5;
  f.mr.record.jump_base = 0;
  f.mr.state.phase = 2u;
  cam = &f.mr.render.camera;
  assert(fifa96_camera_init(cam, 0x300, 0x40, 0x200) == FIFA96_OK);
  cam->speed = 0x10;                /* word[0x1577BE] */
  /* the two draws the arm consumes: the lateral seed (no slot), the height */
  rng_copy = f.mr.rng;
  assert(fifa96_rng_step(&rng_copy, &draw1) == FIFA96_OK);
  assert(fifa96_rng_step(&rng_copy, &draw2) == FIFA96_OK);

  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  /* the focus re-placement (0x8284c/0x8288d/0x82857/0x82893) */
  assert(cam->pos_x == 0x300);
  assert(cam->pos_z == 0x240);
  /* the pan event + the tracked bind (0x82A6C -> 0x71D27) */
  assert(cam->tracked == 1);
  assert(cam->event_param == (int16_t)(0x40 + (int16_t)(draw2 & 0x3F)));
  assert(cam->event_peak == fifa96_camera_ramp((int16_t)cam->event_param));
  assert(e->has_ball == 0u);
  /* the advance: timer89 = 0, stage 2 -> 3 in the same dispatch */
  assert(f.mr.record.stage92 == 3u);
  assert(f.mr.record.timer89 == 0);
  /* the RNG seed was drawn: the pan actually moved the event step pair (the
   * exact seed pair is divided through the ramp divisor, FU-152 §2.9) */
  assert(cam->event_step_x != 0 || cam->event_step_z != 0);
  drop_fixture(&f);
}

/* The same arm's retry semantics: each failed in-body gate leaves the record
 * at stage 2 with no camera write and no bind (the `JZ 0x82AC5` tail). The
 * tracked / `jump_base != 0` advance skips the arm entirely. */
static void test_action_0e_stage2_else_arm_gate_retry(void) {
  struct fixture f;
  struct fifa96_match_entity *e;
  fifa96_camera *cam;
  make_fixture(&f);
  e = &f.mr.entities.team[0].records[1];
  e->active = 1;
  e->lane_x = 0x20;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x0E;
  f.mr.record.stage92 = 2;
  f.mr.record.timer89 = 5;
  f.mr.record.pos_x = 0x300;
  f.mr.record.pos_z = 0x200;
  f.mr.record.type = 0;
  f.mr.record.jump_base = 0;
  f.mr.state.phase = 2u;
  cam = &f.mr.render.camera;
  assert(fifa96_camera_init(cam, 0x300, 0x40, 0x200) == FIFA96_OK);

  /* camY = 0x20 (the gate is strict: > 0x20) -> retry */
  cam->pos_y = 0x20;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  assert(f.mr.record.timer89 == 5);
  assert(cam->pos_x == 0x300 && cam->pos_z == 0x200);
  assert(cam->tracked == FIFA96_CAMERA_TRACKED_NONE);

  /* camY = 0x70 (strict: < 0x70) -> retry */
  cam->pos_y = 0x70;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  assert(cam->tracked == FIFA96_CAMERA_TRACKED_NONE);

  /* lane_x = 0x60 (strict: < 0x60) -> retry */
  cam->pos_y = 0x40;
  e->lane_x = 0x60;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  assert(cam->tracked == FIFA96_CAMERA_TRACKED_NONE);
  assert(cam->pos_x == 0x300 && cam->pos_z == 0x200);

  /* already tracked -> advance without the camera writes */
  e->lane_x = 0x20;
  cam->tracked = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 3u);
  assert(cam->pos_x == 0x300 && cam->pos_z == 0x200);

  /* a live jump arc -> advance without the camera writes */
  cam->tracked = FIFA96_CAMERA_TRACKED_NONE;
  f.mr.record.stage92 = 2;
  f.mr.record.jump_base = 0x10;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 3u);
  assert(cam->pos_x == 0x300 && cam->pos_z == 0x200);
  assert(cam->tracked == FIFA96_CAMERA_TRACKED_NONE);

  /* the gate restored -> the arm completes. This closing half is the
   * discriminating assertion: with the else arm absent every dispatch above
   * and this one would leave stage 2 / the focus / the bind untouched. */
  f.mr.record.jump_base = 0;
  f.mr.record.stage92 = 2;
  f.mr.record.timer89 = 5;
  cam->pos_y = 0x40;
  e->lane_x = 0x20;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0E) == FIFA96_OK);
  assert(f.mr.record.stage92 == 3u);
  assert(f.mr.record.timer89 == 0);
  assert(cam->tracked == 1);                    /* the [0x1577CA] bind */
  assert(cam->pos_x == 0x300);                  /* dir_x[0] = 0 */
  assert(cam->pos_z == 0x200 + 0x40);           /* dir_z[0] = 1 */
  assert(cam->event_param != 0);
  drop_fixture(&f);
}

/* ===== Phase-14 T1 / FU-169: the row-0x5E live installer chain ============
 *
 * Fresh bytes (`0x81908..0x81C71` action-0x0B, `0x81C90..0x81DC5`
 * action-0x0C): the duel body's arm 0 installs the contact anim
 * `{0x0C,0x59}` on itself (`0x81AFB`), arm 1 registers the contact (kind 2,
 * A = the initiator, B = the found opponent, `0x81BCF`) and installs action
 * 0x0C on the found record (`0x81BEB`), then falls into arm 2 (the snap).
 * The found record's action-0x0C stage 0 (`0x81CDC..0x81DC5`) finds A in row
 * 0x59 at frame >= 2 and the `0x795B4` lead band >= 0x30 -> increments the
 * initiator team's `+0x7CF`, installs anim row 0x5E on A (`0x81DB9`) and
 * resets itself. */
static void test_action_0b_duel_chain_installs_row_5e(void) {
  struct fixture f;
  struct fifa96_match_entity *rec;   /* the duel initiator (A) */
  struct fifa96_match_entity *opp;   /* the challenged record (B) */
  make_fixture(&f);
  f.mr.state.phase = 2u;
  rec = &f.mr.entities.team[0].records[1];
  opp = &f.mr.entities.team[1].records[1];
  rec->active = 1;
  opp->active = 1;
  opp->pos_x = 0x340;                /* band 0x40 from rec.pos */
  opp->pos_z = 0x200;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x0B;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0x300;
  f.mr.record.pos_z = 0x200;
  f.mr.record.type = 2;              /* dir (1,0) -> search point = pos + 0x20 */
  f.mr.record.speed71 = 4;           /* <= 4 -> the standing 0x0C choice */
  f.mr.record.face7d = 0;

  /* arm 0: the choice anim on self + the 0x79B6C snap + face toward found */
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(rec->anim_id == 0x0Cu && rec->frame == 0u);
  assert(f.mr.record.ran == 1u);
  assert(f.mr.record.stage92 == 1u);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.target_x == f.mr.record.pos_x);   /* the snap */
  assert(f.mr.record.target_z == f.mr.record.pos_z);
  assert(f.mr.record.lane == 0 && f.mr.record.speed71 == 0);
  assert(f.mr.record.face7d == 0x100);                 /* toward opp (+x) */

  /* arm 1 waits for the anim frame byte (+0x3D) to leave 0 */
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(f.mr.record.stage92 == 1u);
  f.mr.record.frame = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(f.mr.record.stage92 == 2u);
  assert(f.mr.entities.team[0].duel_7d7 == 1u);        /* 0x81B83 */
  assert(f.mr.referee.contact_kind == 2u);             /* the registrar kind */
  assert(f.mr.referee.rec_first == 1);                 /* A = the initiator */
  assert(f.mr.referee.rec_second == 12);               /* B = the challenged */
  assert(opp->code == 0x0Cu && opp->stage92 == 0u);    /* action 0x0C, no invoke */
  assert(opp->type == f.mr.record.type);               /* the +0x8E copy */
  /* arm 2 ran in the same dispatch (the native fall-through): the snap holds */

  /* the challenged record runs the action-0x0C stage 0 */
  rec->anim_id = 0x59u;
  rec->frame = 2u;
  f.mr.record.entity_id = 12;
  f.mr.record.active = 1;
  f.mr.record.code = 0x0Cu;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0x340;
  f.mr.record.pos_z = 0x200;
  f.mr.record.delta = 2;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0C) == FIFA96_OK);
  assert(rec->anim_id == 0x5Eu && rec->frame == 0u);   /* the live row-0x5E install */
  assert(rec->row44 == 0u);
  assert(f.mr.entities.team[0].stumble_7cf == 1u);     /* the team +0x7CF counter */
  /* B reset itself (0x81DBE FUN_0007DAB4) */
  assert(f.mr.record.stage92 == 0u);
  assert(opp->code != 0x0Cu);
  /* the frame-wrap successor chains 0x5E -> 0x0F -> 0x00 (FU-169 §5) */
  assert(fifa96_match_run_record_anim_successor(rec) == FIFA96_OK);
  assert(rec->anim_id == 0x0Fu);
  assert(fifa96_match_run_record_anim_successor(rec) == FIFA96_OK);
  assert(rec->anim_id == 0x00u);
  assert(rec->row44 == 1u);                            /* row 0 flags 0x07 terminal */
  drop_fixture(&f);
}

/* The action-0x0C stage-0 gates: the frame >= 2 retry, the alternate gate
 * thresholds (row 0x11 frame >= 2 / any other row frame >= 3), and the
 * `< 0x30` foul path (the B velocity write + the `fifa96_match_run_contact`
 * re-call; the row-0x11 entry skips the velocity write). */
static void test_action_0c_stage0_gates_and_foul_path(void) {
  struct fixture f;
  struct fifa96_match_entity *a;    /* the initiator ([0x15888F]) */
  struct fifa96_match_entity *b;    /* the running record */
  make_fixture(&f);
  f.mr.state.phase = 2u;
  a = &f.mr.entities.team[0].records[1];
  b = &f.mr.entities.team[1].records[1];
  a->active = 1;
  b->active = 1;
  a->pos_x = 0x320;                  /* band 0x20 < 0x30 from b */
  a->pos_z = 0x200;
  a->anim_id = 0x59u;
  a->frame = 2u;
  f.mr.referee.rec_first = 1;
  f.mr.referee.rec_second = 12;
  f.mr.record.entity_id = 12;
  f.mr.record.active = 1;
  f.mr.record.code = 0x0Cu;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0x300;
  f.mr.record.pos_z = 0x200;
  f.mr.record.speed71 = 8;
  f.mr.record.face7d = 0;
  f.mr.record.delta = 2;

  /* frame < 2 -> the retry tail (no reset, no stage change, no tail) */
  a->frame = 1u;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0C) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0u && b->anim_id != 0x5Eu);
  a->frame = 2u;

  /* band < 0x30 -> the foul path. The velocity write uses A's facing, not
   * B's (0x81DFE loads A into EAX, 0x81E13 reads word[A+0x7D]; the magnitude
   * 0x81E36 is B's speed71): A face 0x100 vs B face 0 discriminates the two
   * sources (sine(0x100) = 0x10000, sine(0x200) = 0). */
  assert(fifa96_entity_sine(0x100) == 0x10000);
  assert(fifa96_entity_sine(0x200) == 0);
  a->face7d = 0x100;
  assert(f.mr.record.face7d == 0);
  assert(fifa96_match_dispatch_action(&f.mr, 0x0C) == FIFA96_OK);
  assert((int16_t)f.mr.record.vel73 == 8);   /* mul16(8, sine(A face 0x100)) */
  assert((int16_t)f.mr.record.vel75 == 0);   /* mul16(8, sine(A face +0x100)) */
  assert(f.mr.referee.contact_kind == 1u);   /* the disclosed re-register */
  assert(f.mr.referee.rec_first == 1 && f.mr.referee.rec_second == 12);
  /* the 0x81ED5..0x81F48 foul tail: the +0x98 latch, the anim row 0x0D/0x52
   * draw on B, ran, timer89 = 0 and the stage 0 -> 1 advance */
  assert(b->skip_98 == 1u);
  assert(b->anim_id == 0x0Du || b->anim_id == 0x52u);
  assert(f.mr.record.ran == 1u);
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.stage92 == 1u);
  assert(b->anim_id != 0x5Eu);

  /* the alternate gate: A in row 0x11 frame 2 -> the foul path without the
   * velocity write (the 0x81E11 JZ skips 0x81E13) */
  a->anim_id = 0x11u;
  a->frame = 2u;
  f.mr.record.stage92 = 0;                   /* the tail advanced; re-arm stage 0 */
  f.mr.record.ran = 0;
  f.mr.record.vel73 = 0x1234;
  f.mr.record.vel75 = 0x2345;
  f.mr.referee.contact_kind = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0C) == FIFA96_OK);
  assert(f.mr.record.vel73 == 0x1234 && f.mr.record.vel75 == 0x2345);
  assert(f.mr.referee.contact_kind == 1u);
  /* ... row 0x11 frame 1 -> retry (no contact, no tail) */
  a->frame = 1u;
  f.mr.record.stage92 = 0;
  f.mr.referee.contact_kind = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0C) == FIFA96_OK);
  assert(f.mr.referee.contact_kind == 0u);
  assert(f.mr.record.stage92 == 0u);

  /* ... any other row: frame 2 -> retry, frame 3 -> the foul path */
  a->anim_id = 0x00u;
  a->frame = 2u;
  f.mr.record.stage92 = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0C) == FIFA96_OK);
  assert(f.mr.referee.contact_kind == 0u);
  a->frame = 3u;
  f.mr.record.stage92 = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0C) == FIFA96_OK);
  assert(f.mr.referee.contact_kind == 1u);
  assert(f.mr.record.stage92 == 1u);
  drop_fixture(&f);
}

/* The action-0x0B gates: the band window (0x20,0x70), the session gate on
 * arm 1, and the aligned-facing 0x59 dive choice. */
static void test_action_0b_duel_gates_and_choice(void) {
  struct fixture f;
  struct fifa96_match_entity *rec;
  struct fifa96_match_entity *opp;
  make_fixture(&f);
  f.mr.state.phase = 2u;
  rec = &f.mr.entities.team[0].records[1];
  opp = &f.mr.entities.team[1].records[1];
  rec->active = 1;
  opp->active = 1;
  opp->pos_x = 0x340;
  opp->pos_z = 0x200;
  f.mr.record.entity_id = 1;
  f.mr.record.active = 1;
  f.mr.record.code = 0x0B;
  f.mr.record.stage92 = 0;
  f.mr.record.pos_x = 0x300;
  f.mr.record.pos_z = 0x200;
  f.mr.record.type = 2;
  f.mr.record.face7d = 0x100;        /* aligned with the +x direction */
  f.mr.record.speed71 = 5;           /* > 4 -> the facing test decides */
  f.mr.entities.team[0].target = 1;  /* the forced-decision code on reset */
  rec->code = 0x0B;                  /* the dispatched code (the reset gate) */

  /* the aligned dive: |face7d - atan| = 0 < 0x1000 -> the 0x59 choice */
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(rec->anim_id == 0x59u);

  /* the band window is strict: opp too close (band 0x20) resets */
  rec->anim_id = 0;
  rec->code = 0x0B;
  opp->pos_x = 0x320;
  f.mr.record.stage92 = 0;
  f.mr.record.type = 2;
  f.mr.record.pos_x = 0x300;
  f.mr.record.speed71 = 4;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(rec->anim_id != 0x59u && rec->anim_id != 0x0Cu);
  assert(f.mr.record.stage92 == 0u);   /* the reset install reinstalled */

  /* ... and too far (band >= 0x70) resets */
  rec->code = 0x0B;
  opp->pos_x = 0x380;
  f.mr.record.stage92 = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(rec->anim_id == 0u);

  /* the session gate on arm 1 (0x81B73): the found record's action-0x0C
   * install is vetoed and the duel record resets */
  rec->code = 0x0B;
  opp->pos_x = 0x340;
  f.mr.record.stage92 = 0;
  f.mr.record.frame = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(rec->anim_id == 0x0Cu);
  opp->code = 0;                       /* clear the arm-0 side effects */
  f.mr.record.frame = 1;
  f.mr.session_gate_14c32a = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x0B) == FIFA96_OK);
  assert(f.mr.record.stage92 == 0u);   /* reset, never stage 2 */
  assert(opp->code != 0x0Cu);
  drop_fixture(&f);
}

int main(void) {
  test_tables_are_fully_classified();
  test_action_rows_dispatch_per_classification();
  test_action_00_runs_move_step();
  test_action_01_runs_kickoff_body();
  test_action_02_restart_and_phase2_arms();
  test_action_0e_stage2_else_arm_binds_camera();
  test_action_0e_stage2_else_arm_gate_retry();
  test_action_0b_duel_chain_installs_row_5e();
  test_action_0c_stage0_gates_and_foul_path();
  test_action_0b_duel_gates_and_choice();
  test_action_1E_runs_claim_place();
  test_action_10_runs_throw_in_body();
  test_live_throw_in_camera_y_cut();
  test_action_0e_live_jump_selector();
  test_action_11_runs_corner_body();
  test_action_12_runs_free_kick_body();
  test_action_13_runs_penalty_body();
  test_action_26_runs_body();
  test_action_28_runs_body();
  test_action_2A_runs_body();
  test_action_2A_overflow_fixture();
  test_action_27_unwired_entry();
  test_action_2C_unwired_entry();
  test_action_29_unwired_entry();
  test_dead_2b_evidence();
  test_action_05_wired_carrier();
  test_action_05_claim_dirs_and_stages();
  test_action_04_and_08_wired();
  test_action_04_stages_lane_fields();
  test_action_04_ball_track_side_from_tracked();
  test_action_1E_stage3_possession_flip();
  test_action_1E_machine_release_chain();
  test_action_1D_runs_closedown();
  test_action_07_runs_body();
  test_action_07_decision_uses_code_byte();
  test_action_0F_runs_body();
  test_action_18_runs_body();
  test_action_21_runs_body();
  test_action_23_runs_body();
  test_action_06_runs_body();
  test_phase_rows_dispatch_per_classification();
  test_out_of_range_is_not_found();
  test_null_arguments_are_invalid();
  test_seam_runs_wired_handler();
  puts("test_engine_match_handlers OK");
  return 0;
}
