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
 * every record. These tests pin that contract.
 *
 * Task 7 (FU282): the `FUN_000782D0` record-0 machine SKELETON in
 * `fifa96_loader/fifa96_keeper_cpu.c` — the head counter, the deferred
 * install, and the ladder gate (the `0x78599` ladder is a no-op seam). */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_loader/fifa96_keeper_cpu.h"

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

/* ── Task 7 (FU282): the `FUN_000782D0` record-0 machine skeleton ────────── */

static uint32_t kc_ladder_runs;
static void kc_ladder_stub(void *ctx) {
  (void)ctx;
  kc_ladder_runs++;
}

static struct fifa96_keeper_cpu_record kc_rec_zero(void) {
  struct fifa96_keeper_cpu_record r;
  memset(&r, 0, sizeof r);
  return r;
}

/* FU282 §1.1 (`0x782D8..0x78334`): the head counter runs only when
 * `[0x157823] != 0 && rec == [0x157A83] && phase != 5`; it consumes the flag
 * and bumps `[0x157AD0 + idx*2]` plus the complement-side `[0x157ACC]`, with
 * `idx = (side ^ [0x157ABE]) & 1`. */
static void test_keeper_cpu_head_counter(void) {
  fifa96_keeper_cpu cpu;
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_out out;
  fifa96_keeper_cpu_init(&cpu);
  cpu.head_flag = 1;                       /* [0x157823] */
  cpu.side_xor = 0;                        /* [0x157ABE] */
  rec = kc_rec_zero();
  rec.side = 0;                            /* idx = (0 ^ 0) & 1 = 0 */
  rec.phase = 2;
  rec.is_actor = 1;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(cpu.head_flag == 0);              /* 0x782FB consumes the flag */
  assert(cpu.counter_ad0[0] == 1);         /* 0x78311 */
  assert(cpu.counter_acc[1] == 1);         /* 0x7832E (the complement side) */
  assert(cpu.counter_ad0[1] == 0 && cpu.counter_acc[0] == 0);

  /* not the actor record -> the head is skipped, the flag survives. */
  cpu.head_flag = 1;
  rec.is_actor = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(cpu.head_flag == 1 && cpu.counter_ad0[0] == 1);

  /* phase 5 -> the head is skipped. */
  rec.is_actor = 1;
  rec.phase = 5;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(cpu.head_flag == 1 && cpu.counter_ad0[0] == 1);

  /* side ^ [0x157ABE] flips the index. */
  cpu.head_flag = 1;
  cpu.side_xor = 1;
  rec.phase = 2;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(cpu.counter_ad0[1] == 1);
  assert(cpu.counter_acc[0] == 1);
}

/* FU282 §1.3 (`0x784D3..0x78576`): the DEFERRED install — `FUN_0007D9A4(rec,
 * code, EBX=0, ECX=0)` (no invoke-now) with `[0x157AB2] := 1` on a target
 * mismatch; gated on `[0x110680][code] & 1` and suppressed by the same-code
 * guard (`0x78558`). Targets: 0x0 (timer live), 0x19 (phase != 2 / not owner /
 * owner-rel 5), 0x4 (owner path, own code != 5). */
static void test_keeper_cpu_deferred_install(void) {
  fifa96_keeper_cpu cpu;
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_out out;
  fifa96_keeper_cpu_init(&cpu);
  rec = kc_rec_zero();
  rec.phase = 2;
  rec.permit = 1;                          /* [0x110680][code] & 1 */
  rec.ball_owner = 0;                      /* not the ball owner -> 0x19 */
  rec.code = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.install == 1 && out.install_code == 0x19);
  assert(out.force_install == 1 && cpu.force_install == 1);  /* [0x157AB2] */

  /* owner path, own code != 5 -> 0x4. */
  cpu.force_install = 0;
  rec.ball_owner = 1;
  rec.owner_rel_5 = 0;
  rec.code = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.install == 1 && out.install_code == 0x4);
  assert(cpu.force_install == 1);

  /* owner-rel record [+0x91] == 5 -> 0x19. */
  cpu.force_install = 0;
  rec.owner_rel_5 = 1;
  rec.code = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.install_code == 0x19);

  /* the +0x81 timer live -> target 0x0; own code already 0 -> same-code guard
   * (`0x78558`) suppresses the install (no latch). */
  cpu.force_install = 0;
  rec.owner_rel_5 = 0;
  rec.timer81 = 1;
  rec.code = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.install == 0 && out.force_install == 0 && cpu.force_install == 0);

  /* own code == 5 on the owner path -> the same-code skip. */
  rec.timer81 = 0;
  rec.code = 5;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.install == 0);

  /* the `0x784DC` permission gate: bit 0 clear -> nothing installed. */
  rec.permit = 0;
  rec.code = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.install == 0 && out.force_install == 0);
}

/* FU282 §1.4 (`0x7857B..0x785A2`): the ladder gate
 * `phase == 2 && [rec+0x20] == 0 && [0x157C5D] != 0` -> the `0x78599` seam.
 * Fresh disasm (`0x784E9 = 0f84af000000`, `JZ 0x7859E`) confirms the
 * `[0x110680][code] & 1` permission gate skips the ladder gate too (0x7859E is
 * after the 0x78599 CALL), so the gate is reachable only when `permit != 0`.
 * The interlock `[0x157C5D]` is cleared UNCONDITIONALLY (`0x785A2`), whether or
 * not the ladder ran. `[0x157C5C]` is the distinct ladder-internal latch. */
static void test_keeper_cpu_ladder_gate(void) {
  fifa96_keeper_cpu cpu;
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_out out;
  fifa96_keeper_cpu_init(&cpu);
  cpu.ladder = kc_ladder_stub;
  cpu.ladder_ctx = NULL;
  cpu.interlock_157c5d = 1;                /* [0x157C5D] the GATE */
  cpu.latch_157c5c = 0x5A;                 /* [0x157C5C] the LATCH (distinct) */
  rec = kc_rec_zero();
  rec.permit = 1;                          /* the 0x784DC gate is open */
  rec.code = 5;                            /* owner path, own code 5 -> the
                                            * 0x7854A same-code skip: no install */
  rec.ball_owner = 1;
  rec.phase = 2;
  rec.queued20 = 0;

  kc_ladder_runs = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.ladder == 1 && kc_ladder_runs == 1);
  assert(out.install == 0);
  assert(cpu.interlock_157c5d == 0);       /* 0x785A2 always clears */
  assert(cpu.latch_157c5c == 0x5A);        /* the ladder latch is untouched */

  /* a queued action (`[rec+0x20] != 0`) closes the gate; the clear still runs. */
  cpu.interlock_157c5d = 1;
  rec.queued20 = 1;
  kc_ladder_runs = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.ladder == 0 && kc_ladder_runs == 0);
  assert(cpu.interlock_157c5d == 0);

  /* only phase 2 opens the gate. */
  cpu.interlock_157c5d = 1;
  rec.queued20 = 0;
  rec.phase = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.ladder == 0);
  assert(cpu.interlock_157c5d == 0);

  /* the permission gate also closes the ladder gate (0x784E9 -> 0x7859E), but
   * the interlock still clears. */
  cpu.interlock_157c5d = 1;
  rec.permit = 0;
  rec.phase = 2;
  kc_ladder_runs = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.ladder == 0 && kc_ladder_runs == 0);
  assert(cpu.interlock_157c5d == 0);

  /* the default no-op seam: the gate is reported, the clear still runs, and
   * no `0x1A`/`0x1B`/`0x1C` row is installed by the stub. */
  cpu.ladder = NULL;
  cpu.interlock_157c5d = 1;
  rec.permit = 1;
  rec.phase = 2;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.ladder == 1 && out.install == 0);
  assert(cpu.interlock_157c5d == 0);
}

int main(void) {
  test_prepass_no_double_dispatch();
  test_prepass_gates_are_phase2_only();
  test_keeper_cpu_head_counter();
  test_keeper_cpu_deferred_install();
  test_keeper_cpu_ladder_gate();
  puts("test_keeper_cpu OK");
  return 0;
}
