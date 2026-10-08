/* src/fifa96_engine/fifa96_match_phase_machine.c — M2 arms-and-wiring Task 1 /
 * FU-142a: the `FUN_0008CEB8` multi-record arm helper. Task 2 adds the
 * `FUN_0008D098` state 0x13/0x14 arm block (`fifa96_match_phase_machine_step`);
 * M2 playable-match Task 2 adds the state-1 kickoff arm
 * (`fifa96_match_phase_machine_kickoff`, FU-143 §11).
 *
 * First-hand evidence: docs/ghidra/FU142_installer_arms_scope.md Appendix A
 * (read-only /FIFA96.EXE: disassemble_function/decompile_function 0x8CEB8,
 * read_memory 0x8CEE2, get_xrefs_to 0x8CEB8, the 15 call-site windows) and
 * Appendix B (state switch 0x8D178/table 0x8D040, arm block 0x8D693..0x8D820,
 * caller FUN_000740A0 0x740C8/0x740DB); docs/ghidra/FU143_phase_rows.md §11 for
 * the state-1 arm (0x8D1B1..0x8D243), FUN_00079CCC and the kickoff phase-2
 * write. */
#include <string.h>

#include "fifa96_engine/fifa96_match_phase_machine.h"
#include "fifa96_engine/fifa96_match_run.h"
#include "fifa96_loader/fifa96_entity_update.h"

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

/* The derived `FUN_00079CCC` kickoff pick (state-1 arm `0x8D1C6`): the record
 * of `team` nearest the kickoff point 0x15774C, skipping index 0 (EBX=0),
 * `+0x9A` and `+0x98`, with the native strict-minimum (ties keep the first
 * candidate) and the native no-candidate fallback to the team base (record 0;
 * `0x79CE4` seeds the result with EDX = the team base and `0x79D4F` returns
 * it). The native per-record distance source is the record's phase handler
 * `[rec+0x1C]` placement output; the phase handlers are unported, so the
 * derived model substitutes the formation-seeded target triple (the same
 * placement before the commit). The probe point is the live camera triple
 * (0x15774C), (0,0,0) at the kickoff. The native best seed `0x7FBC` (a cutoff
 * no formation-coordinate distance reaches) is not reproduced: the shared
 * `fifa96_entity_find_nearest` seeds 0xFFFF (FU-143 §11.1 leg). */
static int32_t match_kickoff_pick(const struct fifa96_match_run *mr, uint32_t team) {
  fifa96_entity_candidate candidates[FIFA96_MATCH_ENTITY_RECORDS];
  int16_t best = 0;
  int index;
  for (uint32_t i = 0; i < FIFA96_MATCH_ENTITY_RECORDS; i++) {
    const struct fifa96_match_entity *e = &mr->entities.team[team].records[i];
    candidates[i].x = (int16_t)e->target_x;
    candidates[i].y = (int16_t)e->target_z;
    candidates[i].skip_98 = e->skip_98;
    candidates[i].skip_9a = e->skip_9a;
  }
  index = fifa96_entity_find_nearest(candidates, FIFA96_MATCH_ENTITY_RECORDS, 0,
                                     (int16_t)mr->render.camera.pos_x,
                                     (int16_t)mr->render.camera.pos_z, &best);
  return index >= 0 ? (int32_t)index : 0;
}

int fifa96_match_phase_machine_kickoff(struct fifa96_match_run *mr) {
  if (!mr) return -FIFA96_ERR_INVALID;
  /* The native FUN_000740A0 write already happened; the installer arms read
   * [0x157A4D] == 1. */
  mr->entities.phase = 1u;
  for (uint32_t t = 0; t < FIFA96_MATCH_ENTITY_TEAMS; t++) {
    struct fifa96_match_team *team = &mr->entities.team[t];
    int32_t first;
    (void)fifa96_match_arm_install_multi(&mr->entities, t, 0, 10, 3, -1); /* 0x8D1C1 */
    first = match_kickoff_pick(mr, t);                                    /* 0x8D1D1 */
    team->target = (int32_t)(t * FIFA96_MATCH_ENTITY_RECORDS + (uint32_t)first);
    if (mr->phase_machine.side_controlled == team->side) {                /* 0x8D1EF */
      int32_t second;
      (void)fifa96_match_entities_install(&team->records[first],
                                          mr->entities.phase, 1, 0);      /* 0x8D200 */
      team->records[first].skip_9a = 1;                                   /* 0x8D211 */
      second = match_kickoff_pick(mr, t);                                 /* 0x8D21D */
      team->records[first].skip_9a = 0;                                   /* 0x8D22C */
      (void)fifa96_match_entities_install(&team->records[second],
                                          mr->entities.phase, 2, 0);      /* 0x8D238 */
    }
    (void)fifa96_match_entities_merge_slot(&mr->entities, t, (uint32_t)first); /* 0x8D243 */
  }
  return FIFA96_OK;
}
