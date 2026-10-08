/* include/fifa96_engine/fifa96_match_handlers.h — engine-side action/phase
 * dispatch for the M2 match (FU-137).
 *
 * The native match dispatches two static tables (FU-137 §1, re-verified
 * read-only on /FIFA96.EXE):
 *   - action table flat 0x1106E0, 45 slots, codes 0x00..0x2C; sole reader
 *     FUN_0007D9A4 @ 0x7DA77 (`EAX = code; SHL 2; ADD EAX,0x1106E0`), which
 *     stages the chosen handler into the record and optionally invokes it with
 *     EAX = record pointer (FU-137 §2);
 *   - phase table flat 0x110794, 35 slots, phases 0x00..0x22; sole reader
 *     FUN_0006D920 @ 0x6D9B3 (`EAX = [0x157A4A]>>24; SHL 2; ADD
 *     EAX,0x110794`), which stores the entry at record +0x1C for the
 *     FUN_0008D098 per-record loop to call with (record, record+0x4D)
 *     (FU-137 §2/§3); phase 0x16 is the native zero entry the loader turns
 *     into INT3 — no handler by design.
 *
 * This module is the extensible seam the G2 clusters (plan C5-C10) fill: each
 * row carries the ported handler (`fn`, NULL while unported) and an evidence
 * string naming the FU doc/function or the numbered open leg. The dispatchers
 * never guess: a classified-but-unported row returns
 * `-FIFA96_ERR_UNSUPPORTED` with its open-leg marker, a zero/unassigned native
 * slot or an out-of-range code returns `-FIFA96_ERR_NOT_FOUND`, and only a
 * present handler can return `FIFA96_OK`; NULL `mr` is `-FIFA96_ERR_INVALID`.
 * All error results are negated, matching the engine family convention
 * (`fifa96_match_run_*`, `fifa96_*` libraries), so `rc < 0` is always an
 * error. */
#pragma once
#include <stdint.h>
#include "fifa96_loader/fifa96_err.h"

struct fifa96_match_run;

#define FIFA96_MATCH_ACTION_ROWS 45u /* 0x1106E0[45], codes 0x00..0x2C */
#define FIFA96_MATCH_PHASE_ROWS 35u  /* 0x110794[35], phases 0x00..0x22 */

/* Native slot that is zero in /FIFA96.EXE (loader INT3 stub): no body by
 * design, so the phase dispatcher reports NOT_FOUND rather than UNSUPPORTED. */
#define FIFA96_MATCH_PHASE_INT3_SLOT 0x16u

/* A ported row body. Returns FIFA96_OK on success or a -fifa96_err_t; the
 * dispatcher propagates the value verbatim. */
typedef int (*fifa96_match_handler_fn)(struct fifa96_match_run *mr);

struct fifa96_match_handler {
  uint8_t code;               /* native table slot this row describes */
  fifa96_match_handler_fn fn; /* NULL until a G2 cluster ports the body */
  const char *evidence;       /* FU-137 §6 classification or open-leg marker */
};

/* The two native tables enumerated in FU-136 §2/§3 and re-verified in FU-137
 * §1. `fn` is NULL for every row at the Task 4 commit (0 ported); later
 * clusters fill their rows here. */
extern const struct fifa96_match_handler fifa96_match_action_table[FIFA96_MATCH_ACTION_ROWS];
extern const struct fifa96_match_handler fifa96_match_phase_table[FIFA96_MATCH_PHASE_ROWS];

/* Run one table row: NULL `mr` or `row` -> -FIFA96_ERR_INVALID; `row->fn ==
 * NULL` -> -FIFA96_ERR_UNSUPPORTED (explicit open-leg marker, never a silent
 * success); otherwise the handler's return value verbatim. This is the shared
 * core of both dispatchers and the unit-test entry for the seam. */
int fifa96_match_dispatch_row(struct fifa96_match_run *mr,
                              const struct fifa96_match_handler *row);

/* Dispatch action code 0x00..0x2C. NULL `mr` -> -FIFA96_ERR_INVALID; a code
 * past the table -> -FIFA96_ERR_NOT_FOUND; otherwise the row's classification
 * as in fifa96_match_dispatch_row. A FIFA96_OK result also sets the run's
 * `dispatched_ok` observation bit for the code (Task 15 / M2-B; bookkeeping
 * only). */
int fifa96_match_dispatch_action(struct fifa96_match_run *mr, uint8_t code);

/* Dispatch phase 0x00..0x22. NULL `mr` -> -FIFA96_ERR_INVALID; a phase past
 * the table or the native zero slot FIFA96_MATCH_PHASE_INT3_SLOT ->
 * -FIFA96_ERR_NOT_FOUND; otherwise the row's classification as in
 * fifa96_match_dispatch_row.
 *
 * Forward compatibility: the original phase call is per record
 * (`FUN_0008D098` walks 11 records at 0xB2 stride calling
 * `[rec+0x1C](rec, rec+0x4D)`); this seam passes only `mr` today. Task 10
 * (plan C10) may extend the handler typedef when it adds the record walk. */
int fifa96_match_dispatch_phase(struct fifa96_match_run *mr, uint8_t phase);
