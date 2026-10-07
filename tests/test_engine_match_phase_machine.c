/* tests/test_engine_match_phase_machine.c — M2 arms-and-wiring Task 1 /
 * FU-142a: the `FUN_0008CEB8` multi-record arm helper and the derived
 * `FUN_0008D098` phase-machine state.
 *
 * Native contract (FU-142 Appendix A, first-hand read-only /FIFA96.EXE):
 *   EAX = team block base, DX = first record index, BX = last index,
 *   CX = install code, stack arg = skip-if-current code.
 *   - first guard: `first` must be 0..10 (`TEST DX,DX` 0x8CEC6 /
 *     `CMP ESI,0xB` 0x8CED2);
 *   - last clamp: `last >= 0xB` becomes 0xB (`CMP ESI,0xB` 0x8CEE2 /
 *     `MOV word [ESP+4],0xB` 0x8CEE7; raw bytes `83 FE 0B 7C 07 66 C7 44
 *     24 04 0B 00`);
 *   - per record: skip `+0x9A != 0` (0x8CEFB), skip `MOVSX +0x91 == skip`
 *     (0x8CF04..0x8CF13);
 *   - `i == 0 && code == 3` pre-coerces the staged code to 0x19
 *     (0x8CF15..0x8CF25), before `FUN_0007D9A4(rec, code, 0, 0)`
 *     (0x8CF3F);
 *   - all 15 call sites (all inside `FUN_0008D098`) pass BX=0xA, so the port
 *     clamps `last` to record 10 (the pool's last record; the native index-11
 *     spill `team+0x7A6` is unobservable and recorded as FU-142 OL-49).
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fifa96_engine/fifa96_match_phase_machine.h"
#include "fifa96_engine/fifa96_match_run.h"

#define NONE FIFA96_MATCH_ENTITY_NONE
#define TEAM0 0u
#define TEAM1 1u

/* Task 2 fixtures: a run whose pool and machine are freshly initialized and
 * whose state/phase/side gates are set for the FUN_0008D098 arm block. The
 * pool's latched phase is kept in step with the run phase (production latches
 * it once per granted frame before the arm block runs). */
static void step_setup(struct fifa96_match_run *mr, uint8_t state, uint8_t phase,
                       uint8_t controlled) {
  fifa96_match_run_init(mr);
  mr->state.phase = phase;
  mr->phase_machine.state = state;
  mr->phase_machine.phase = phase;
  mr->phase_machine.side_controlled = controlled;
  mr->entities.phase = phase;
}

/* init zeroes every machine field and seeds the per-team chosen-record cache
 * to NONE; NULL is rejected. */
static void test_init_defaults(void) {
  struct fifa96_match_phase_machine pm;
  memset(&pm, 0xAA, sizeof pm);
  assert(fifa96_match_phase_machine_init(&pm) == FIFA96_OK);
  assert(pm.state == 0 && pm.phase == 0 && pm.side_controlled == 0);
  assert(pm.ac5 == 0 && pm.ac7 == 0 && pm.arm2a_overflow == 0);
  assert(pm.chosen831[0] == NONE && pm.chosen831[1] == NONE);
  assert(fifa96_match_phase_machine_init(NULL) == -FIFA96_ERR_INVALID);
}

/* Multi-record staging: free records 1..3 stage the code through the FU-137
 * installer (the record's +0x91 code, the +0x92 staged byte, the +0x89 timer
 * and +0x9E ran flag are the installer's staging); occupied records are
 * skipped; the helper forwards pool->phase into the installer arm. */
static void test_install_multi_stages(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  /* the new team tail fields init to zero/NONE */
  assert(pool.team[TEAM0].flag830 == 0);
  assert(pool.team[TEAM0].chosen831 == NONE && pool.team[TEAM1].chosen831 == NONE);
  for (uint32_t i = 4; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    pool.team[TEAM0].records[i].skip_9a = 1;
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 1, 10, 0x26, -1) == 3);
  for (uint32_t i = 1; i <= 3; i++) {
    assert(pool.team[TEAM0].records[i].code == 0x26);
    assert(pool.team[TEAM0].records[i].stage92 == 0); /* installer BL=0 */
    assert(pool.team[TEAM0].records[i].timer89 == 0);
    assert(pool.team[TEAM0].records[i].ran == 0);
  }
  assert(pool.team[TEAM0].records[0].code == 0);
  for (uint32_t i = 4; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(pool.team[TEAM0].records[i].code == 0);
  /* pool->phase reaches the installer's +0x98 arm: phase 3 clears skip_98. */
  pool.phase = 3;
  pool.team[TEAM0].records[1].skip_98 = 1;
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 1, 1, 9, -1) == 1);
  assert(pool.team[TEAM0].records[1].skip_98 == 0);
  assert(pool.team[TEAM0].records[1].code == 9);
  /* team 1 stages inside its own block */
  assert(fifa96_match_arm_install_multi(&pool, TEAM1, 1, 2, 0x28, -1) == 2);
  assert(pool.team[TEAM1].records[1].code == 0x28);
  assert(pool.team[TEAM1].records[2].code == 0x28);
}

/* Both native skip tests: an occupied record (+0x9A, 0x8CEFB), a record whose
 * current code equals the skip parameter (0x8CF04), and the installer's own
 * same-code rejection for a record whose current code equals the install code
 * (0x7D9D6) all stay untouched and are not counted. The helper's filter
 * compares the *original* code (not the coerced 0x19). */
static void test_install_multi_skips(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  pool.team[TEAM0].records[1].skip_9a = 1;
  pool.team[TEAM0].records[2].code = 0x0C; /* == skip_code */
  pool.team[TEAM0].records[3].code = 0x26; /* == install code */
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 1, 4, 0x26, 0x0C) == 1);
  assert(pool.team[TEAM0].records[1].code == 0);
  assert(pool.team[TEAM0].records[1].skip_9a == 1);
  assert(pool.team[TEAM0].records[2].code == 0x0C);
  assert(pool.team[TEAM0].records[3].code == 0x26);
  assert(pool.team[TEAM0].records[4].code == 0x26);
  /* skip equal to the install code skips the stage outright */
  pool.team[TEAM0].records[4].code = 0x26;
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 4, 4, 0x26, 0x26) == 0);
  /* record 0 already holding the coerced 0x19 with incoming code 3: the
   * helper filter passes (0x19 != -1) but the installer rejects same-code. */
  pool.team[TEAM0].records[0].code = 0x19;
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 0, 0, 3, -1) == 0);
  assert(pool.team[TEAM0].records[0].code == 0x19);
}

/* `i == 0 && code == 3` pre-coerces to 0x19 regardless of the record's active
 * flag (0x8CF15..0x8CF25); record 5 keeps the installer's own coercion rule
 * (active records keep code 3). */
static void test_install_multi_record0_coerce(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  pool.team[TEAM0].records[0].active = 1;
  pool.team[TEAM0].records[5].active = 1;
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 0, 0, 3, -1) == 1);
  assert(pool.team[TEAM0].records[0].code == 0x19);
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 5, 5, 3, -1) == 1);
  assert(pool.team[TEAM0].records[5].code == 3);
}

/* Bounds and guards: first > last iterates nothing; the native first guard
 * (0x8CED2) rejects first >= 11; last > 10 clamps to the pool's last record;
 * NULL pool / team > 1 -> -FIFA96_ERR_INVALID. */
static void test_install_multi_bounds(void) {
  struct fifa96_match_entities pool;
  assert(fifa96_match_entities_init(&pool) == FIFA96_OK);
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 5, 3, 0x26, -1) == 0);
  assert(pool.team[TEAM0].records[5].code == 0);
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 11, 11, 0x26, -1) == 0);
  assert(fifa96_match_arm_install_multi(&pool, TEAM0, 10, 200, 0x26, -1) == 1);
  assert(pool.team[TEAM0].records[10].code == 0x26);
  assert(fifa96_match_arm_install_multi(NULL, TEAM0, 0, 10, 0x26, -1) ==
         -FIFA96_ERR_INVALID);
  assert(fifa96_match_arm_install_multi(&pool, 2, 0, 10, 0x26, -1) ==
         -FIFA96_ERR_INVALID);
}

/* Task 2: the step is a no-op unless BOTH the FUN_0008D098 switch value
 * (`pm->state`, native `[0x157A4D]`) and the run phase (`mr->state.phase`,
 * native `[0x157A4A]>>24` — the same byte) are 0x13/0x14. NULL is rejected. */
static void test_step_gate(void) {
  struct fifa96_match_run mr;
  step_setup(&mr, 0x12, 0x13, 0); /* state outside 0x13/0x14 */
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      assert(mr.entities.team[t].records[i].code == 0);
  step_setup(&mr, 0x13, 0x12, 0); /* phase outside 0x13/0x14 */
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
      assert(mr.entities.team[t].records[i].code == 0);
  step_setup(&mr, 0x15, 0x14, 0); /* state above the arm block */
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  assert(mr.entities.team[1].records[0].code == 0);
  assert(fifa96_match_phase_machine_step(NULL) == -FIFA96_ERR_INVALID);
  /* both valid entry states run the block (sanity for the negative cases) */
  step_setup(&mr, 0x14, 0x14, 0);
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  assert(mr.entities.team[1].records[0].code == 0x26);
}

/* `0x8D74D` arm 0x26: the team whose `+0x826` side differs from the
 * zero-extended `[0x157AAC]>>24` byte is armed 0x26 across records 0..10 and
 * the per-team call RETs (0x8D73D..0x8D75E). The matching team runs the
 * phase arms: phase 0x13 with equal `[0x157AC5]`/`[0x157AC7]` words installs
 * code 3 (record 0 pre-coerced to 0x19 by the FUN_0008CEB8 index-0 rule). */
static void test_step_arm26_side_mismatch(void) {
  struct fifa96_match_run mr;
  step_setup(&mr, 0x13, 0x13, TEAM0); /* team 0 is the controlled side */
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    mr.entities.team[0].records[i].active = 1; /* keep code 3 (installer 3->0x19 arm) */
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[1].records[i].code == 0x26);
  assert(mr.entities.team[0].records[0].code == 0x19);
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[0].records[i].code == 3);
  assert(mr.entities.team[0].chosen831 == NONE);
  assert(mr.entities.team[1].chosen831 == NONE);
  assert(mr.phase_machine.arm2a_overflow == 0);
}

/* `0x8D78B`/`0x8D7AD`: phase 0x13, controlled side. Equal low bytes of
 * `[0x157AC5]`/`[0x157AC7]` install 3; differing bytes install 0x25. The
 * non-controlled team still gets 0x26. */
static void test_step_arm25_3_phase13(void) {
  struct fifa96_match_run mr;
  step_setup(&mr, 0x13, 0x13, TEAM1); /* team 1 is controlled */
  mr.phase_machine.ac5 = 9;
  mr.phase_machine.ac7 = 4; /* differ -> 0x25 */
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[1].records[i].code == 0x25);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[0].records[i].code == 0x26);
  /* equal words -> code 3 (0x19 on record 0) */
  step_setup(&mr, 0x13, 0x13, TEAM1);
  mr.phase_machine.ac5 = 4;
  mr.phase_machine.ac7 = 4;
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    mr.entities.team[1].records[i].active = 1; /* installer keeps code 3 */
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  assert(mr.entities.team[1].records[0].code == 0x19);
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[1].records[i].code == 3);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[0].records[i].code == 0x26);
}

/* `0x8D7CF` arm 0x28 + `0x8D807` arm 0x2A (phase 0x14 half): the controlled
 * team stages 0x28 across records 0..10, then the scan takes the first record
 * 1..10 with `+0x9A == 0` (record 1 here), caches its encoded id at
 * `team+0x831` and overwrites it with 0x2A; the scan starts at record 1, so
 * record 0 keeps 0x28. The non-controlled team gets 0x26. The arm tail clears
 * `[0x10F35C]` (0x8D80E; Task 8 completed the write). */
static void test_step_arm28_2a_phase14(void) {
  struct fifa96_match_run mr;
  step_setup(&mr, 0x14, 0x14, TEAM0);
  mr.global_10f35c = 1;
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  assert(mr.entities.team[0].records[0].code == 0x28);
  assert(mr.entities.team[0].records[1].code == 0x2A);
  for (uint32_t i = 2; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[0].records[i].code == 0x28);
  assert(mr.entities.team[0].chosen831 == 1); /* 11*team + record */
  assert(mr.phase_machine.arm2a_overflow == 0);
  assert(mr.global_10f35c == 0);              /* 0x8D80E arm tail */
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[1].records[i].code == 0x26);
  assert(mr.entities.team[1].chosen831 == NONE);
  /* team 1 controlled: record 1 is an occupied slot, so the scan skips it
   * and takes record 2 -> chosen831 = 11 + 2. */
  step_setup(&mr, 0x14, 0x14, TEAM1);
  mr.entities.team[1].records[1].skip_9a = 1;
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  assert(mr.entities.team[1].chosen831 == 13);
  assert(mr.entities.team[1].records[1].code == 0);
  assert(mr.entities.team[1].records[1].skip_9a == 1);
  assert(mr.entities.team[1].records[2].code == 0x2A);
}

/* Full pool: records 1..10 occupied -> the native scan exits at EDX=0xB with
 * EAX=team+0x7A6 and would install into the record-11 alias at team+0x837
 * (the next block +0x2). The bounded pool model records the overflow instead:
 * chosen831 = NONE, arm2a_overflow = 1, no pool record gets 0x2A; the 0x28
 * arm still stages the free record 0. A later scan that finds a record clears
 * the flag (it reports the latest scan's outcome). */
static void test_step_2a_overflow(void) {
  struct fifa96_match_run mr;
  step_setup(&mr, 0x14, 0x14, TEAM0);
  for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    mr.entities.team[0].records[i].skip_9a = 1;
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  assert(mr.phase_machine.arm2a_overflow == 1);
  assert(mr.entities.team[0].chosen831 == NONE);
  assert(mr.entities.team[0].records[0].code == 0x28);
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++)
    assert(mr.entities.team[0].records[i].code != 0x2A);
  assert(mr.entities.team[1].records[0].code == 0x26); /* other side untouched */
  /* record 5 frees up: the next scan takes it and clears the flag */
  mr.entities.team[0].records[5].skip_9a = 0;
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  assert(mr.phase_machine.arm2a_overflow == 0);
  assert(mr.entities.team[0].chosen831 == 5);
  assert(mr.entities.team[0].records[5].code == 0x2A);
}

/* The `0x8D728` side test is a zero-extended byte compare: the native zeroes
 * EAX and loads AL from `[EBP+0x826]` (0x8D72E/0x8D733) before the 32-bit
 * compare against the arithmetic high byte of `[0x157AAC]`. A controlled byte
 * with the top bit set (0x80) must still arm the install for both sides (a
 * model that collapsed the byte to its signed value or truncated it would not
 * take the 0x26 arm here). */
static void test_step_side_hi_bit(void) {
  struct fifa96_match_run mr;
  step_setup(&mr, 0x13, 0x13, 0x80);
  mr.phase_machine.ac5 = 7;
  mr.phase_machine.ac7 = 7; /* an equal pair: the controlled path would install 3 */
  assert(fifa96_match_phase_machine_step(&mr) == FIFA96_OK);
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
      uint8_t code = mr.entities.team[t].records[i].code;
      assert(code == 0x26);
      assert(code != 3 && code != 0x19 && code != 0x25);
    }
    assert(mr.entities.team[t].chosen831 == NONE);
  }
  assert(mr.phase_machine.arm2a_overflow == 0);
}

int main(void) {
  test_init_defaults();
  test_install_multi_stages();
  test_install_multi_skips();
  test_install_multi_record0_coerce();
  test_install_multi_bounds();
  test_step_gate();
  test_step_arm26_side_mismatch();
  test_step_arm25_3_phase13();
  test_step_arm28_2a_phase14();
  test_step_2a_overflow();
  test_step_side_hi_bit();
  puts("test_engine_match_phase_machine OK");
  return 0;
}
