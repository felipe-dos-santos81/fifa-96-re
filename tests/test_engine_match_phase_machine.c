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

#define NONE FIFA96_MATCH_ENTITY_NONE
#define TEAM0 0u
#define TEAM1 1u

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

int main(void) {
  test_init_defaults();
  test_install_multi_stages();
  test_install_multi_skips();
  test_install_multi_record0_coerce();
  test_install_multi_bounds();
  puts("test_engine_match_phase_machine OK");
  return 0;
}
