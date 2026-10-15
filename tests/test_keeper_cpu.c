/* tests/test_keeper_cpu.c — the R11-L4 keeper-CPU chain (phase-26c T6-T8).
 *
 * Task 6 (FU281): the pre-pass `FUN_0008D8EC` reconciled with the FU-141 walk.
 * FU281 §2 read the tree as having "no engine equivalent" of `FUN_0008D8EC`;
 * the fresh read shows the pre-pass is already hosted by the FU-141 per-record
 * walk (`fifa96_match_entities_team_update`): the `+0x82C` counter (`0x8D8F7`),
 * the phase-2 sector-template select + nearest target
 * (`0x8D929..0x8D9B7` -> `team+0x7B2`), the phase-2 interception select + band
 * flag (`0x8D9BD..0x8DAE9` -> `team+0x7BA`/`+0x7BE`), the team timer decay
 * (`0x8DAF3`) and the `0xB2`-stride record dispatch (`0x8DB2E`/`0x8DB3A`). The
 * walk's single `action(ctx, e)` call per record IS the native dispatch
 * (`FUN_000782D0` / `FUN_0007CA54` -> `CALL dword[rec+0x18]`), so the
 * reconciliation is AUGMENT: the state rides the existing FU-141 walk and no
 * second pre-pass is wired — a standalone pre-pass call would double-dispatch
 * every record. These tests pin that contract. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_entities.h"

#define NONE FIFA96_MATCH_ENTITY_NONE
#define TEAM0 0u
#define TEAM1 1u
#define RECS FIFA96_MATCH_ENTITY_RECORDS

static struct fifa96_match_entities_frame zero_frame(void) {
  struct fifa96_match_entities_frame f;
  memset(&f, 0, sizeof f);
  return f;
}

static uint32_t cp_dispatches;
static uint8_t cp_order[64];
static int cp_cb(void *ctx, struct fifa96_match_entity *e) {
  (void)ctx;
  cp_order[cp_dispatches++] = (uint8_t)(e->team * RECS + e->index);
  return FIFA96_OK;
}

/* The pre-pass rides the FU-141 walk: per call every non-skipped record
 * dispatches exactly once (the `0xB2`-stride loop `0x8DB2E`/`0x8DB3A`) and the
 * `+0x82C` counter (`0x8D8F7`) advances exactly once per team — no second
 * pre-pass, no double-dispatch, across consecutive frames. */
static void test_prepass_no_double_dispatch(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  f.delta = 2;
  for (uint32_t frame = 1; frame <= 4; frame++) {
    cp_dispatches = 0;
    assert(fifa96_match_entities_update(&pool, &f, cp_cb, NULL) == FIFA96_OK);
    assert(cp_dispatches == 2u * RECS);
    for (uint32_t i = 0; i < RECS; i++) {
      assert(cp_order[i] == (uint8_t)i);
      assert(cp_order[RECS + i] == (uint8_t)(RECS + i));
    }
    assert(pool.team[TEAM0].update_count == frame);
    assert(pool.team[TEAM1].update_count == frame);
  }
  /* `+0x82C` wraps to 0 at 0xB (0x8D8F7..0x8D912): seven more calls -> 0. */
  for (uint32_t frame = 0; frame < 7; frame++)
    assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].update_count == 0);
  assert(pool.team[TEAM1].update_count == 0);
}

/* The (c)/(e) arms are phase-2 only (`0x8D929` / `0x8D9CC`). A phase-2 pass
 * with the (c) gate open selects the nearest record into `team+0x7B2`; a
 * non-phase-2 pass clears the interception pick + band flag and leaves the
 * target (the native skips the arms, not the `0xB2` walk). */
static void test_prepass_gates_are_phase2_only(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  f.phase = 2;
  f.delta = 2;
  /* (c) gate: phase 2 AND (target == 0 OR actor == 0). Leave both unset so the
   * nearest pick runs; put a lone candidate near the (bucket-2, zero-lead)
   * anchor so the result is deterministic (record 5, the entities-suite seed). */
  for (uint32_t i = 0; i < RECS; i++) pool.team[TEAM0].records[i].pos_x = 0x400;
  pool.team[TEAM0].records[5].pos_x = 4;
  pool.team[TEAM0].records[5].pos_z = 0;
  assert(pool.team[TEAM0].target == NONE && pool.controlled == NONE);
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].target == 5);           /* 0x8D9B7 */
  assert(pool.team[TEAM0].intercept == NONE);     /* actor unset: no (e) arm */
  /* a non-phase-2 pass clears the interception pick + flag, keeps the target. */
  pool.team[TEAM0].intercept = 7;
  pool.team[TEAM0].flag7be = 1;
  f.phase = 0;
  assert(fifa96_match_entities_update(&pool, &f, NULL, NULL) == FIFA96_OK);
  assert(pool.team[TEAM0].intercept == NONE);
  assert(pool.team[TEAM0].flag7be == 0u);
  assert(pool.team[TEAM0].target == 5);
}

int main(void) {
  test_prepass_no_double_dispatch();
  test_prepass_gates_are_phase2_only();
  puts("test_keeper_cpu OK");
  return 0;
}
