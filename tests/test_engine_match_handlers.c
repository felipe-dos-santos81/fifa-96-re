/* tests/test_engine_match_handlers.c — Task 4: action/phase dispatch tables (FU-137).
 *
 * The two native tables are action `0x1106E0[45]` (codes 0x00..0x2C, sole reader
 * `FUN_0007D9A4 @ 0x7DA77`) and phase `0x110794[35]` (phases 0x00..0x22, sole
 * reader `FUN_0006D920 @ 0x6D9B3`) — FU-137 §1/§2. At this commit nothing is
 * wired into the engine dispatch (FU-137 §6: 0 ported, 2 unwired, 73 not
 * ported, 4 open leg), so every row must resolve to an explicit UNSUPPORTED
 * open-leg marker, except phase 0x16 (the native zero/INT3 slot) which is
 * NOT_FOUND by contract. The seam itself must run a handler and propagate its
 * result when one is present. */
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

/* FU-137 §6 row classification mapped to the dispatch contract: every action
 * row is UNSUPPORTED at this commit (00/1E are `unwired` — tested library
 * bodies with no record/entity binding yet; the other 43 are `not ported`, of
 * which 27/29/2B/2C are open legs with no static install arm). */
static const int action_expect[FIFA96_MATCH_ACTION_ROWS] = {
    /* 00 */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 0A */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 14 */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
    /* 1E */ UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP, UNSUP,
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

/* Per-row dispatch: the FU-137 classification is the contract; no unassigned
 * row may return OK (a silent no-op success). */
static void test_action_rows_dispatch_per_classification(void) {
  struct fixture f = make_fixture();
  for (uint32_t code = 0; code < FIFA96_MATCH_ACTION_ROWS; code++) {
    int rc = fifa96_match_dispatch_action(&f.mr, (uint8_t)code);
    assert(rc == action_expect[code]);
    assert(rc != FIFA96_OK);   /* nothing is wired at this commit */
  }
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
  test_phase_rows_dispatch_per_classification();
  test_out_of_range_is_not_found();
  test_null_arguments_are_invalid();
  test_seam_runs_wired_handler();
  puts("test_engine_match_handlers OK");
  return 0;
}
