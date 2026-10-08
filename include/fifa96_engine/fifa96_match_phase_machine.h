/* include/fifa96_engine/fifa96_match_phase_machine.h — M2 arms-and-wiring
 * Task 1 / FU-142a: the `FUN_0008CEB8` multi-record arm helper and the state
 * of the `FUN_0008D098` installer-arms machine.
 *
 * `FUN_0008CEB8` (0x8CEB8..0x8CF5D, 52 instructions; 15 calls, all
 * UNCONDITIONAL_CALL inside `FUN_0008D098` — re-verified read-only on
 * /FIFA96.EXE; FU-137 §5.1, FU-142 §2/Appendix A) stages one action code
 * across a contiguous range of a team block's 0xB2-stride records:
 * EAX = team base, DX = first record, BX = last record, CX = install code,
 * stack arg = skip-if-current code. Records with `+0x9A != 0` and records
 * whose `+0x91` code sign-extends equal to the skip code are passed over; the
 * code is pre-coerced `3 -> 0x19` for loop index 0 (0x8CF15..0x8CF25) and
 * staged through the FU-137 §2 installer `FUN_0007D9A4(rec, code, 0, 0)`.
 * Every one of the 15 static call sites passes BX=0xA; the native `last`
 * clamp is 0xB (0x8CEE7), which would also visit index 11 = `team+0x7A6` (the
 * same spill slot the 0x2A arm targets directly). The derived pool models
 * records 0..10 only, so this port clamps to 10; the divergence is
 * unreachable from the static sites and recorded as FU-142 OL-49.
 *
 * The machine state mirrors the globals `FUN_0008D098` gates its arms on:
 * `[0x157A4D]` (switch value, 0x8D178), `[0x157A4A]>>24` (phase, read by
 * `FUN_0006D920` and the per-record loop), `[0x157AAC]>>24` (controlled side,
 * 0x8D728), the low bytes of the `[0x157AC5]`/`[0x157AC7]` words
 * (0x8D76C/0x8D772) and the per-team `+0x831` chosen-record cache the 0x2A
 * arm writes (0x8D801). The native state switch byte at `0x157A4D` IS the high
 * byte of the `0x157A4A` dword (0x157A4A + 3), so `state` and `phase` are two
 * views of the same native byte; the derived step keeps the plan's two-field
 * gate and reads the arm-block phase from `mr->state.phase`.
 *
 * `fifa96_match_phase_machine_step` is the state 0x13/0x14 subset of
 * `FUN_0008D098` (Task 2, FU-142 Appendix B): the state switch jump table
 * (`0x8D040`) sends both 0x13 and 0x14 to `0x8D693`, whose arm block runs the
 * `0x26`/3/0x25/0x28 arms and the `0x2A` record scan over one team block per
 * call; the native caller `FUN_000740A0` (`0x740C8`/`0x740DB`) invokes it once
 * per team right after writing `[0x157A4D]`, so the derived step loops both
 * teams. */
#pragma once
#include <stdint.h>
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_loader/fifa96_err.h"

struct fifa96_match_run;   /* step argument; defined in fifa96_match_run.h */

struct fifa96_match_phase_machine {
  uint8_t state;           /* [0x157A4D] switch value (0x8D178); the step gate */
  uint8_t phase;           /* latched [0x157A4A]>>24; SUPERSEDED by
                              mr->state.phase, which the step gates/reads (the
                              two are the same native byte); kept as a carried
                              mirror for the frame/reset surface */
  uint8_t side_controlled; /* [0x157AAC]>>24 (0x8D728) */
  uint8_t ac5, ac7;        /* low bytes of [0x157AC5]/[0x157AC7] (0x8D76C);
                              the native compare is a full 16-bit word compare,
                              the high-byte loss is FU-142 OL-81 */
  uint8_t arm2a_overflow;  /* 0x2A scan past record 10 (FU-142 §2 hazard) */
  int32_t chosen831[FIFA96_MATCH_ENTITY_TEAMS]; /* per-team +0x831 chosen
                              record; carried Task-1 mirror: the step writes
                              fifa96_match_team.chosen831 (0x8D801) instead, so
                              this array is only seeded by init */
};

/* Zero the machine and seed the carried `chosen831[0..1]` mirror to
 * FIFA96_MATCH_ENTITY_NONE (the step writes the pool team field, not this
 * mirror). NULL -> -FIFA96_ERR_INVALID. */
int fifa96_match_phase_machine_init(struct fifa96_match_phase_machine *pm);

/* `FUN_0008CEB8` over the derived pool: for `i = first` while
 * `i <= min(last, FIFA96_MATCH_ENTITY_RECORDS - 1)`, skip `records[i]` when
 * its `+0x9A` exclusion is set or its `+0x91` code equals `skip_code`
 * (sign-extended byte compare), pre-coerce `i == 0 && code == 3` to 0x19,
 * then stage through `fifa96_match_entities_install(entity, pool->phase,
 * code, 0)`. Returns the number of records staged (>= 0; a record the
 * installer rejects is not counted) or -FIFA96_ERR_INVALID for NULL `pool` or
 * `team > 1`. */
int fifa96_match_arm_install_multi(struct fifa96_match_entities *pool, uint32_t team,
                                   uint8_t first, uint8_t last, uint8_t code,
                                   int skip_code);

/* The `FUN_0008D098` state 0x13/0x14 arm block over one run (FU-142
 * Appendix B). A no-op returning FIFA96_OK unless `pm->state` and
 * `mr->state.phase` are both 0x13/0x14 (native: the state switch value and
 * the phase are the same byte; the derived gate keeps both fields). Per team:
 *  - `+0x826 != (uint8_t)side_controlled` (0x8D728..0x8D73B) -> stage 0x26
 *    across records 0..10 and return (0x8D74D, the per-team call RETs);
 *  - phase 0x13 (0x8D767): `ac5 != ac7` -> 0x25, else 3 (record 0 pre-coerced
 *    to 0x19 by the helper's index-0 rule); both return;
 *  - phase 0x14: stage 0x28 across records 0..10 (0x8D7CF), then scan records
 *    1..10 for the first `skip_9a == 0` (0x8D7D4..0x8D7F6): found ->
 *    `chosen831 = 11*team + i` and a direct `fifa96_match_entities_install`
 *    of 0x2A into it; none -> `chosen831 = FIFA96_MATCH_ENTITY_NONE` and
 *    `arm2a_overflow = 1` (the bounded model of the native scan exiting at
 *    EDX=0xB into the record-11 alias `team+0x7A6`, install byte at
 *    `team+0x837` = the next block +0x2; no pool record is written).
 * `arm2a_overflow` reports the latest scan's outcome (cleared by a found
 * scan). The installer phase argument is the pool's latched `phase`. Returns
 * FIFA96_OK, or -FIFA96_ERR_INVALID for NULL `mr`. */
int fifa96_match_phase_machine_step(struct fifa96_match_run *mr);
