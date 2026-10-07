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
 * dispatch. Task 6 ports row 29's body (`fifa96_arm_29_step`, the phase-5
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
 * invocation: its evidence records the FU-142f dead verdict. The seam itself
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
 * (cluster B)
 * keeps 05/06/07/0F and the possession/tackle rows 18/21/23 UNSUP: their
 * record-visible cores have tested pure helpers, but the
 * carrier/pursuit/kick/receive/resolution arms are unported (the FU-141
 * pool they also waited on now exists). */
static const int action_expect[FIFA96_MATCH_ACTION_ROWS] = {
    /* 00 */ FIFA96_OK, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 0A */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 14 */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 1E */ FIFA96_OK, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, FIFA96_OK, UNSUP,
    /* 28 */ FIFA96_OK, UNSUP, FIFA96_OK, UNSUP, UNSUP,
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
 * land (FU-140 OL-37, carrying OL-16). */
static void test_action_1E_runs_claim_place(void) {
  struct fixture f;
  make_fixture(&f);
  f.mr.record.stage = 1;
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
  f.mr.record.stage = 3;
  f.mr.record.has_ball = 0;
  f.mr.record.controlled = 0;
  f.mr.record.place_x = -1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.has_ball == 0 && f.mr.record.controlled == 0);
  assert(f.mr.record.place_valid == 0 && f.mr.record.place_x == -1);

  /* already holding the ball -> no placement, no actor rebind */
  f.mr.record.stage = 1;
  f.mr.record.has_ball = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 1);
  assert(f.mr.record.controlled == 0 && f.mr.record.place_x == -1);
  assert(f.mr.record.place_valid == 0);

  /* stage 6 clears the helper request; a slot suppresses it */
  f.mr.record.stage = 6;
  f.mr.record.has_ball = 0;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 0);
  f.mr.record.stage = 1;
  f.mr.record.has_slot = 1;
  assert(fifa96_match_dispatch_action(&f.mr, 0x1E) == FIFA96_OK);
  assert(f.mr.record.helper_request == 0);
  assert(f.mr.record.place_valid == 1);
  assert(f.mr.record.has_ball == 1 && f.mr.record.controlled == 1);
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
  assert(fifa96_match_dispatch_action(&f.mr, 0x28) == FIFA96_OK);
  assert(f.mr.record.target_x == 0x100 + 133);
  assert(f.mr.record.target_z == 0x200 + 55);
  assert(f.mr.record.stage92 == 1);    /* arm 0 latch + arm 1 wait */
  assert(f.mr.record.timer89 == 0);
  assert(f.mr.record.scratch_a2 == 0x20); /* seed-0 draw 512 & 0x7F + 0x20 */

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
  assert(fifa96_match_dispatch_action(&f.mr, 0x2A) == FIFA96_OK);
  assert(f.mr.record.target_x == -0x720);
  assert(f.mr.record.target_z == 0);
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

static void test_phase_rows_dispatch_per_classification(void) {
  struct fixture f;
  make_fixture(&f);
  for (uint32_t phase = 0; phase < FIFA96_MATCH_PHASE_ROWS; phase++) {
    int rc = fifa96_match_dispatch_phase(&f.mr, (uint8_t)phase);
    assert(rc == phase_expect[phase]);
    if (phase == FIFA96_MATCH_PHASE_INT3_SLOT)
      assert(rc == NOTF);
    else
      assert(rc == UNSUP);
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

int main(void) {
  test_tables_are_fully_classified();
  test_action_rows_dispatch_per_classification();
  test_action_00_runs_move_step();
  test_action_1E_runs_claim_place();
  test_action_26_runs_body();
  test_action_28_runs_body();
  test_action_2A_runs_body();
  test_action_2A_overflow_fixture();
  test_action_27_unwired_entry();
  test_action_2C_unwired_entry();
  test_action_29_unwired_entry();
  test_dead_2b_evidence();
  test_phase_rows_dispatch_per_classification();
  test_out_of_range_is_not_found();
  test_null_arguments_are_invalid();
  test_seam_runs_wired_handler();
  puts("test_engine_match_handlers OK");
  return 0;
}
