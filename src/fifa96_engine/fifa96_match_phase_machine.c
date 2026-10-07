/* src/fifa96_engine/fifa96_match_phase_machine.c — M2 arms-and-wiring Task 1 /
 * FU-142a: the `FUN_0008CEB8` multi-record arm helper.
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix A
 * (read-only /FIFA96.EXE: disassemble_function/decompile_function 0x8CEB8,
 * read_memory 0x8CEDB, get_xrefs_to 0x8CEB8, the 15 call-site windows). */
#include <string.h>

#include "fifa96_engine/fifa96_match_phase_machine.h"

int fifa96_match_phase_machine_init(struct fifa96_match_phase_machine *pm) {
  if (!pm) return -FIFA96_ERR_INVALID;
  memset(pm, 0, sizeof *pm);
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
