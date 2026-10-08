/* src/fifa96_engine/fifa96_match_phase_machine.c — M2 arms-and-wiring Task 1 /
 * FU-142a: the `FUN_0008CEB8` multi-record arm helper. Task 2 adds the
 * `FUN_0008D098` state 0x13/0x14 arm block (`fifa96_match_phase_machine_step`).
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix A
 * (read-only /FIFA96.EXE: disassemble_function/decompile_function 0x8CEB8,
 * read_memory 0x8CEE2, get_xrefs_to 0x8CEB8, the 15 call-site windows) and
 * Appendix B (state switch 0x8D178/table 0x8D040, arm block 0x8D693..0x8D820,
 * caller FUN_000740A0 0x740C8/0x740DB). */
#include <string.h>

#include "fifa96_engine/fifa96_match_phase_machine.h"
#include "fifa96_engine/fifa96_match_run.h"

int fifa96_match_phase_machine_init(struct fifa96_match_phase_machine *pm) {
  if (!pm) return -FIFA96_ERR_INVALID;
  memset(pm, 0, sizeof *pm);
  /* Carried Task-1 mirror: the step writes the pool's team->chosen831
   * (0x8D801), so this array is seeded only, and pm->phase is superseded by
   * mr->state.phase (FU-142 Appendix B.6). */
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++)
    pm->chosen831[t] = FIFA96_MATCH_ENTITY_NONE;
  return FIFA96_OK;
}

int fifa96_match_arm_install_multi(struct fifa96_match_entities *pool, uint32_t team,
                                   uint8_t first, uint8_t last, uint8_t code,
                                   int skip_code) {
  uint8_t bound;
  int staged = 0;
  if (!pool || team >= FIFA96_MATCH_ENTITY_TEAMS) return -FIFA96_ERR_INVALID;
  if (first >= FIFA96_MATCH_ENTITY_RECORDS) return 0;        /* 0x8CED2 */
  bound = last > FIFA96_MATCH_ENTITY_RECORDS - 1u            /* 0x8CEE2/0x8CEE7 */
              ? (uint8_t)(FIFA96_MATCH_ENTITY_RECORDS - 1u)
              : last;
  for (uint8_t i = first; i <= bound; i++) {
    struct fifa96_match_entity *e = &pool->team[team].records[i];
    uint8_t want;
    if (e->skip_9a != 0) continue;                           /* 0x8CEFB */
    if ((int8_t)e->code == (int8_t)skip_code) continue;      /* 0x8CF04 */
    want = (i == 0u && code == 3u) ? 0x19u : code;           /* 0x8CF15 */
    staged += fifa96_match_entities_install(e, pool->phase, want, 0); /* 0x8CF3F */
  }
  return staged;
}

int fifa96_match_phase_machine_step(struct fifa96_match_run *mr) {
  struct fifa96_match_phase_machine *pm;
  if (!mr) return -FIFA96_ERR_INVALID;
  pm = &mr->phase_machine;
  /* 0x8D178: the switch byte is 0x13/0x14; the arm block's phase is the same
   * native byte, modeled by the run phase (Task 2 gate). */
  if (!((pm->state == 0x13u || pm->state == 0x14u) &&
        (mr->state.phase == 0x13u || mr->state.phase == 0x14u)))
    return FIFA96_OK;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    struct fifa96_match_team *team = &mr->entities.team[t];
    if ((uint8_t)pm->side_controlled != team->side) {          /* 0x8D728 */
      (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, 0x26, -1);
      continue;                                                /* 0x8D74D/RET */
    }
    if (mr->state.phase == 0x13u) {                            /* 0x8D767 */
      uint8_t code = pm->ac5 != pm->ac7 ? 0x25u : 3u;          /* 0x8D772 */
      (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, code, -1);
      continue;                                  /* 0x8D78B/0x8D7AD, both RET */
    }
    (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, 0x28, -1); /* 0x8D7CF */
    int32_t found = FIFA96_MATCH_ENTITY_NONE;
    for (uint32_t i = 1; i < FIFA96_MATCH_ENTITY_RECORDS; i++) { /* 0x8D7D4 */
      if (team->records[i].skip_9a == 0) {
        found = (int32_t)i;
        break;
      }
    }
    if (found != FIFA96_MATCH_ENTITY_NONE) {
      pm->arm2a_overflow = 0;
      team->chosen831 =                                          /* 0x8D801 */
          (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)found);
      (void)fifa96_match_entities_install(&team->records[found],
                                          mr->entities.phase, 0x2A, 0); /* 0x8D807 */
    } else {
      /* Native 0x8D7F6 exits at EDX=0xB with EAX=team+0x7A6 and installs 0x2A
       * into the record-11 alias (`+0x91` = team+0x837 = next block +0x2);
       * the derived pool models records 0..10 only, so the overflow is
       * recorded instead (FU-142 Appendix B / OL-49). */
      pm->arm2a_overflow = 1;
      team->chosen831 = FIFA96_MATCH_ENTITY_NONE;
    }
    /* 0x8D80C..0x8D80E: the 0x2A arm tail clears the [0x10F35C] chase flag
     * (both the found and the overflow path fall through it). Task 8 completed
     * this write of the arm Task 2 ported, because row 2A's arm 10 sets the
     * flag and the derived frame reads it back (FU-142 Appendix H; OL-56). */
    mr->global_10f35c = 0;
  }
  return FIFA96_OK;
}
