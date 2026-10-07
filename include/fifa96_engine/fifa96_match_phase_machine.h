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
 * arm writes (0x8D801). The step/arm bodies land in plan Task 2 (Gate G1). */
#pragma once
#include <stdint.h>
#include "fifa96_engine/fifa96_match_entities.h"
#include "fifa96_loader/fifa96_err.h"

struct fifa96_match_phase_machine {
  uint8_t state;           /* [0x157A4D] switch value (0x8D178) */
  uint8_t phase;           /* latched [0x157A4A]>>24 */
  uint8_t side_controlled; /* [0x157AAC]>>24 (0x8D728) */
  uint8_t ac5, ac7;        /* low bytes of [0x157AC5]/[0x157AC7] (0x8D76C) */
  uint8_t arm2a_overflow;  /* 0x2A scan past record 10 (FU-142 §2 hazard) */
  int32_t chosen831[FIFA96_MATCH_ENTITY_TEAMS]; /* per-team +0x831 chosen record */
};

/* Zero the machine and seed chosen831[0..1] = FIFA96_MATCH_ENTITY_NONE.
 * NULL -> -FIFA96_ERR_INVALID. */
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
