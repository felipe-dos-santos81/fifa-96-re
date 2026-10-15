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
static int kc_ladder_stub(void *ctx, const struct fifa96_keeper_cpu_record *rec,
                          struct fifa96_keeper_cpu_ladder_out *out) {
  (void)ctx;
  (void)rec;
  (void)out;
  kc_ladder_runs++;
  return FIFA96_OK;
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

/* A ladder record with every head gate open (byte +0x91 == 0x19, all skip
 * cells clear); only `stage` (CX) varies. */
static struct fifa96_keeper_cpu_record kc_ladder_open(uint8_t stage) {
  struct fifa96_keeper_cpu_record r = kc_rec_zero();
  r.code = 0x19;
  r.stage = stage;
  return r;
}

/* FU283 §4 — the `0x77E98` switch: `CX-1 ∈ 0..4` selects the arm, and the three
 * `0x1A` arms stamp `[0x157C2C]` 0x2C/0x2D/0x2E. Every arm installs
 * `FUN_0007D9A4(rec, code, 0, ECX=1)` — invoke-now. */
static void test_keeper_cpu_ladder_five_arms(void) {
  static const uint8_t stamps[3] = {0x2Cu, 0x2Du, 0x2Eu}; /* CX 1/2/3 */
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_ladder_out out;
  for (uint8_t cx = 1; cx <= 3; cx++) {
    rec = kc_ladder_open(cx);
    memset(&out, 0, sizeof out);
    assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
    assert(out.ran == 1);
    assert(out.install == 1 && out.install_code == 0x1Au);   /* 0x78227/4f/77 */
    assert(out.invoke_now == 1);                             /* 0x78222/4a/72 */
    assert(out.stamp_157c2c == stamps[cx - 1u]);             /* 0x78230/58/7e */
    assert(out.copy_pos == 0);
  }
  /* CX 4 -> the `0x78294` arm installs row 0x1B, no stamp. */
  rec = kc_ladder_open(4);
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.install == 1 && out.install_code == 0x1Bu && out.invoke_now == 1);
  assert(out.stamp_157c2c == 0);
  /* CX 5 -> the `0x782B1` arm installs row 0x1C (falls into the 0x782BB tail). */
  rec = kc_ladder_open(5);
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.install == 1 && out.install_code == 0x1Cu && out.invoke_now == 1);
  assert(out.stamp_157c2c == 0);
}

/* FU283 §6 — `FUN_000765C4` (`0x765C4`): 1 iff the tracked record is present,
 * on the same side, and (its `byte +0x91 == 5` or `[0x158740]>>24 != 4`). */
static void test_keeper_cpu_ladder_tracked_predicate(void) {
  assert(fifa96_keeper_cpu_tracked_predicate(1, 1, 5, 0) == 1);   /* row 5 */
  assert(fifa96_keeper_cpu_tracked_predicate(1, 1, 0, 0) == 1);   /* global != 4 */
  assert(fifa96_keeper_cpu_tracked_predicate(1, 1, 4, 4) == 0);   /* both false */
  assert(fifa96_keeper_cpu_tracked_predicate(0, 1, 5, 0) == 0);   /* absent */
  assert(fifa96_keeper_cpu_tracked_predicate(1, 0, 5, 0) == 0);   /* other side */
}

/* FU283 §5 — the `FUN_000765C4 != 0` fall-through installs **code 4** via the
 * shared tail `0x782BB` (`0x781F6..0x78200`), not a second `0x1B`. */
static void test_keeper_cpu_ladder_code4_fallthrough(void) {
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_ladder_out out;
  rec = kc_ladder_open(1);
  rec.tracked_present = 1;
  rec.tracked_same_side = 1;
  rec.tracked_code = 5;
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.install == 1 && out.install_code == 0x4u && out.invoke_now == 1);
  assert(out.stamp_157c2c == 0);                 /* the 0x1A arm is skipped */
  /* it wins over every stage: CX 4 (would be 0x1B) still installs 4. */
  rec.stage = 4;
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.install_code == 0x4u);
  /* with the predicate false the switch runs (here back to the CX-4 arm). */
  rec.tracked_code = 4;
  rec.global_158740 = 4;
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.install == 1 && out.install_code == 0x1Bu);
}

/* FU283 §1/§2 — the head gates and the actor/side skip: every closed gate exits
 * to the shared tail `0x782C4` with no install (`ran` marks the selector
 * reached). */
static void test_keeper_cpu_ladder_head_gates(void) {
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_ladder_out out;
  rec = kc_ladder_open(1);
  rec.code = 0x18;                              /* 0x77EC2: byte +0x91 != 0x19 */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0 && out.install == 0);

  rec = kc_ladder_open(1);
  rec.input_range[0] = 0x200u;                  /* 0x77EF0: the [0x14C1D4] bit-9 gate */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0 && out.install == 0);

  rec = kc_ladder_open(1);
  rec.gate_9a = 1;                              /* 0x77EFD: the +0x9A skip */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0);

  rec = kc_ladder_open(1);
  rec.is_T = 1;                                 /* 0x77F10: *(param_1[10]) == 'T' */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0);

  rec = kc_ladder_open(1);
  rec.is_actor = 1;                             /* 0x77F36: rec == [0x157A83] */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0);

  rec = kc_ladder_open(1);
  rec.actor_present = 1;
  rec.actor_same_side = 1;                      /* 0x77F51: same-side actor skip */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0);
  rec.actor_same_side = 0;                      /* other side -> not skipped */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.install_code == 0x1Au);

  /* the +0x81 timer: copy pos (+0x59 -> +0x4D) and return, no install (0x77F20). */
  rec = kc_ladder_open(1);
  rec.timer81 = 1;
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.copy_pos == 1 && out.ran == 0 && out.install == 0);

  /* stage 0 returns at the selector (0x781E4): ran, no install. */
  rec = kc_ladder_open(0);
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.install == 0);

  /* stage > 5 fails the DEC/CMP/JA bound (0x78206): no install. */
  rec = kc_ladder_open(6);
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.install == 0);
}

/* FU291 §Port shape (phase-27 T2) — the ladder head gate is now COMPUTED from
 * live `input_range` (`[0x14C1D4]`) and `side ^ side_swap` (the `FUN_000741B4`
 * fold `0x77ED9`), not a caller-supplied flag. The tested bit is **bit 9** of
 * the selected word (`0x77EE8 AND AH,2`); side/side_swap pick the word. */
static void test_keeper_cpu_prepass_gate_from_input_range(void) {
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_ladder_out out;

  rec = kc_ladder_open(1);
  rec.side = 0;
  rec.side_swap = 0;                            /* idx = (0 ^ 0) & 1 = 0 */
  rec.input_range[0] = 0x200u;                  /* bit 9 set -> gate closed */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0 && out.install == 0);

  rec.input_range[0] = 0x100u;                  /* bit 8 is NOT the gate bit */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.install == 1);

  rec.side_swap = 1;                            /* idx = 1 -> word 1 selected */
  rec.input_range[0] = 0x200u;                  /* word 0's bit is now inert */
  rec.input_range[1] = 0u;
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 1 && out.install == 1);

  rec.input_range[1] = 0x200u;                  /* word 1's bit 9 closes */
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0 && out.install == 0);

  rec.side = 1;                                 /* idx = (1 ^ 1) & 1 = 0 */
  rec.input_range[1] = 0u;
  memset(&out, 0, sizeof out);
  assert(fifa96_keeper_cpu_ladder_run(NULL, &rec, &out) == FIFA96_OK);
  assert(out.ran == 0 && out.install == 0);
}

/* FU283 §Port shape / Task 8 — the ladder is wired INTO the FU282 seam (the
 * default is the ported body, replacing the no-op stub). The live stage index
 * cannot be raised (its selector inputs are unported, FU283-L6), so a 0x19
 * record reaches the selector with `stage == 0` and installs nothing. */
static void test_keeper_cpu_ladder_seam(void) {
  fifa96_keeper_cpu cpu;
  struct fifa96_keeper_cpu_record rec;
  struct fifa96_keeper_cpu_out out;
  fifa96_keeper_cpu_init(&cpu);
  assert(cpu.ladder == fifa96_keeper_cpu_ladder_run);   /* the ported body */
  cpu.interlock_157c5d = 1;
  rec = kc_rec_zero();
  rec.permit = 1;
  rec.code = 0x18;                              /* the ladder's own head gate closes */
  rec.ball_owner = 1;
  rec.phase = 2;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.ladder == 1);
  assert(out.ladder_out.ran == 0 && out.ladder_out.install == 0);

  cpu.interlock_157c5d = 1;
  rec.code = 0x19;
  rec.ball_owner = 0;                           /* machine target 0x19 == code: suppressed */
  rec.stage = 0;
  assert(fifa96_keeper_cpu_step(&cpu, &rec, &out) == FIFA96_OK);
  assert(out.ladder == 1);
  assert(out.ladder_out.ran == 1 && out.ladder_out.install == 0);
  assert(out.install == 0);
}

/* FU296 Seam A (phase-27 T3): the record-0 machine is stepped from the `i == 0`
 * branch of the FU-141 walk (`fifa96_match_entities_team_update`) using the
 * frame's machine view, and the walk's single `action()` dispatch still runs
 * once per record — the install feeds that one dispatch, it does not add a
 * second. The step reaches each team's record 0 (the `0x782D8` head counter
 * runs for the actor record of either team); a NULL view leaves the walk inert. */
static void test_seam_a_step_once_per_team(void) {
  struct fifa96_match_entities pool;
  struct fifa96_match_entities_frame f = zero_frame();
  fifa96_keeper_cpu cpu;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  fifa96_keeper_cpu_init(&cpu);
  cpu.ladder = kc_ladder_stub;
  cpu.ladder_ctx = NULL;
  cpu.side_xor = 0;
  f.phase = 2;
  f.delta = 2;
  f.keeper_cpu = &cpu;

  /* team 0 record 0 is the actor: its head counter (`0x782D8`) advances. */
  pool.controlled = 0;                          /* ENTITY_ID(0,0) */
  cpu.head_flag = 1;                            /* [0x157823] */
  cp_dispatches = 0;
  assert(fifa96_match_entities_update(&pool, &f, cp_cb, NULL) == FIFA96_OK);
  assert(cpu.counter_ad0[0] == 1);              /* stepped for team 0 */
  assert(cpu.counter_acc[1] == 1);
  assert(cp_dispatches == 2u * RECS);           /* the single dispatch, no double */

  /* team 1 record 0 is the actor: the step reaches team 1's walk too. */
  pool.controlled = (int32_t)RECS;              /* ENTITY_ID(1,0) */
  cpu.head_flag = 1;
  cp_dispatches = 0;
  assert(fifa96_match_entities_update(&pool, &f, cp_cb, NULL) == FIFA96_OK);
  assert(cpu.counter_ad0[1] == 1);
  assert(cpu.counter_acc[0] == 1);
  assert(cp_dispatches == 2u * RECS);

  /* a NULL view is inert (the fixture-frame default). */
  cpu.head_flag = 1;
  f.keeper_cpu = NULL;
  assert(fifa96_match_entities_update(&pool, &f, cp_cb, NULL) == FIFA96_OK);
  assert(cpu.counter_ad0[1] == 1);              /* unchanged */
}

int main(void) {
  test_prepass_no_double_dispatch();
  test_prepass_gates_are_phase2_only();
  test_keeper_cpu_head_counter();
  test_keeper_cpu_deferred_install();
  test_keeper_cpu_ladder_gate();
  test_keeper_cpu_ladder_five_arms();
  test_keeper_cpu_ladder_tracked_predicate();
  test_keeper_cpu_ladder_code4_fallthrough();
  test_keeper_cpu_ladder_head_gates();
  test_keeper_cpu_prepass_gate_from_input_range();
  test_keeper_cpu_ladder_seam();
  test_seam_a_step_once_per_team();
  puts("test_keeper_cpu OK");
  return 0;
}
