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
struct fifa96_match_entity;

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

/* FU-191 §2/§6/§9 + FU-198 §5.2 (M2 phase-17 T1): the per-record phase
 * dispatch — the native walk `FUN_0008D098`'s `0x8D107 CALL 0x6D920` +
 * `0x8D110 CALL dword[ECX+0x1C]`. The stager writes
 * `[rec+0x1C] = 0x110794[[0x157A4A]>>24]` and the walk then calls that pointer
 * with the watcom register ABI **EAX = rec**, **EDX = &rec[0x4D]** (the target
 * triple), **EBX = -1** (FU-191 §6). The engine's `fifa96_match_handler_fn` is
 * `int (*)(mr)` — no record — so this sibling threads the record and the native
 * EBX argument through the run's phase-walk cursor
 * (`mr->phase_walk_rec`/`phase_walk_edge`/`phase_walk_active`), which a ported
 * per-record body reads; the native `[rec+0x1C]` staging is modelled here by
 * resolving the phase row (the walk's gate is `fn != NULL`, FU-191 §10).
 * `mr == NULL` -> -FIFA96_ERR_INVALID; a phase past the table or the INT3 slot
 * -> -FIFA96_ERR_NOT_FOUND; a NULL row -> -FIFA96_ERR_UNSUPPORTED (the walk
 * skips); otherwise the handler's return value verbatim. */
int fifa96_match_dispatch_phase_record(struct fifa96_match_run *mr, uint8_t phase,
                                       struct fifa96_match_entity *rec, int32_t edge);

/* FU-191 §6 / FU-198 §5.1 (M2 phase-17 T1): the ported native handler
 * `FUN_0006DCC8` (`0x6DCC8..0x6DD3B`, fresh decompile + disassembly) — the
 * per-record phase-table rows `0x02`/`0x15`. It reads the cursor record's
 * native `[rec+0xC]` direction-byte pair (modelled as
 * `struct fifa96_match_entity.phase_meta`, FU-192 §3), indexes it by the native
 * third argument (`edge < 0` selects the per-side default
 * `word[0x1577DE + side*4] >> 16`; else `edge`), and writes the target triple
 * `rec+0x4D/+0x51/+0x55` = `(int16)(byte0 * +-0x26)`, `0`, `(int16)(byte1 *
 * +-0x21)`, sign-flipped when `[team+0x826] != 0` (FU-198 §5.1). The `[rec+0xC]`
 * source is the OL-13 `.fmt` metadata; since FU-199 §7.2 / FU-200 §1 (M2
 * phase-18 T1) the match-begin bind `fifa96_match_entities_bind_record_meta`
 * stages it live, so the body does its native work on the live path. An unset
 * `phase_meta` (no assets, or an unbound record) still makes this a documented
 * no-op (FU-177 §7 dispatched-but-gated). Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID (NULL `mr`). */
int fifa96_match_phase_02(struct fifa96_match_run *mr);

/* FU-192 §2.1–§2.7 (M2 phase-17 T3): the remaining distinct per-record phase
 * bodies of the `0x00–0x15` family, each reading the walk cursor
 * (`mr->phase_walk_rec`) and writing the target triple, exactly as the native
 * `FUN_0008D098` calls them (EAX=rec / EDX=&rec[0x4D]). All return FIFA96_OK,
 * or -FIFA96_ERR_INVALID for a NULL `mr`; outside the walk (no cursor) they are
 * no-ops. The per-row wiring (which native rows are counted) is in the phase
 * table evidence. */
/* `0x6DE34` — the shared reset/hold (rows `00 0A 0D 0F`): `target := position`
 * (3× MOVSD, FU-192 §2.1). */
int fifa96_match_phase_00(struct fifa96_match_run *mr);
/* `0x6DE44` — rows `03 04 07` (FU-192 §2.4): the `word[0x1577D6/D8+side*4]`
 * side-selected placement. The DX≥0 arm scales the `[rec+0x14]` dword pair
 * (`W + (W>>2)` / `D + (D>>4)`, side-flipped) — gated on the unported
 * `[rec+0x14]` (FU192-L1); the DX<0 cell-map arm (`0x6D870`/`0x15774C`) is
 * FU192-L3. */
int fifa96_match_phase_04(struct fifa96_match_run *mr);
/* `0x6E05C` — row `05` (FU-192 §2.7): the ball-relative line. The derivable
 * arm is the `target := rec position` copy; the ball/`0x8DC68`/`0x114E04` arm
 * is FU192-L1 (the ball team is unmodelled). */
int fifa96_match_phase_05(struct fifa96_match_run *mr);
/* `0x6DD9C` — row `06` (FU-192 §2.3): scales the `[rec+0x10]` dword pair by
 * ×1.25 / ×1.0625 with the `[0x157754]`-sign index and the ±0x60 z bias;
 * gated on the unported `[rec+0x10]` (FU192-L1). */
int fifa96_match_phase_06(struct fifa96_match_run *mr);
/* `0x6DD6C` — rows `08 09` (FU-192 §2.2): the thin wrapper over `0x6DCC8`,
 * selecting the direction index `0xBC` (same side as `[0x157AAC]>>24`) or
 * `0x6F` (other side). */
int fifa96_match_phase_08(struct fifa96_match_run *mr);
/* `0x6DF4C` — row `0C` (FU-192 §2.5): the stateful restart-line timer,
 * `timer89 += [0x157A64]`, then the `0x780`/`0xCC0` target writes past the
 * `0x3C` gate; the `[rec+4]`/`0x157A38`/`[rec+0x8A]`/`0x8DCD4` cells are
 * FU192-L1. */
int fifa96_match_phase_0C(struct fifa96_match_run *mr);
/* `0x6E004` — row `10` (FU-192 §2.6): the pure `0x1105EA` variant table
 * (`(int8)byte << 5`, z negated by side) or the `active == 0` `0x3C0`/`0x840`
 * default. Row `0x10` is not written by any engine source, so its slot stays
 * NULL (an un-dispatched row would be a fake count). */
int fifa96_match_phase_10(struct fifa96_match_run *mr);

/* FU-177 §2/§3/§7 + FU-198 §5.3 (M2 phase-17 T1): the ported action row-03 body
 * `0x7E1A4..0x7E524` — the hold/clamp placement over the FU-138 helpers
 * (`fifa96_action_locomotion_hold`/`clamp_placement`/`placement_counter`/
 * `phase1_clamp`) and the already-ported `fifa96_match_entities_camera_place`
 * (`FUN_00079F3C`, FU-96 leg 5). Its live phase-2 hot path runs the per-record
 * `[rec+0x1C]` call (`0x7E2DD CALL dword[ESI+0x1C]` = `0x110794[2] = 0x6DCC8`)
 * through `fifa96_match_dispatch_phase_record`. **The action-table slot stays
 * NULL / uncounted**: wiring it moved the M2 tape at frame 29 (the forced
 * code-3 record's body and its phase-0x13/0x14 special arm are tape-visible),
 * so per controller R7 the row legs (owner FU177-L1) and the body is exported
 * for direct unit testing. The unported arms (the `FUN_0008C418` team bound
 * block the phase-2 clamp reads, `FUN_00079C20` slot dir step, `FUN_00079B58`
 * +0x93 helper, the phase-0x13/0x14 special arm, the phase-7 `0x158747` wall
 * scan and the phase-8/0xC tails) are named legs (FU177-L1..L6). Returns
 * FIFA96_OK, or -FIFA96_ERR_INVALID for an out-of-range
 * `mr->record.entity_id`. */
int fifa96_match_action_03(struct fifa96_match_run *mr);

/* The ported action row-0x17 body (FU-181 §2): the line-up walk
 * `0x84730..0x849AF`. Its action-table slot (`fifa96_match_action_table[0x17]`)
 * is **NULL** — the row is not counted — because no engine path installs it:
 * the only static installer is act-4 stage 0 (`0x89740`, invoke-now), whose
 * invoker (the `FUN_0008A938` sit-2..4 formation-order gate `0x8AB08..0x8AB62`,
 * FU-149 L12) and the formation matrices are unported (FU181-L1). This is the
 * rows-27/29/2C precedent (a ported body with no engine installer stays
 * unwired and uncounted); the body is exported so it is tested directly and
 * ready for the act-4 seam. Returns FIFA96_OK, or -FIFA96_ERR_INVALID for an
 * out-of-range `mr->record.entity_id` (like every row body it assumes a
 * non-NULL `mr`; the dispatch entry points guard that). */
int fifa96_match_action_17(struct fifa96_match_run *mr);

/* The ported action row-0x22 body (FU-185 §2/§6): the sequence/press machine
 * `0x8539C..0x85481`. Its action-table slot (`fifa96_match_action_table[0x22]`)
 * is **wired** and the row is **COUNTED** (census `58/80 → 59/80 = 73%`) as of
 * M2 phase-25 (FU-259): the OL-65 gate staging landed — the row-07
 * `match_kick_from_record` now re-sources the opponent `word[+0x6F]`
 * (`opp->cam_dz6f`) and `word[+0x7D]` (`opp->face7d`), so the `0x816CC` fold
 * `entity_angle(word[+0x6D], word[+0x6F]) - word[+0x7D]` is satisfiable and the
 * wired row-07 `0x816DF` invoke-now fires, installing `0x22` on the opponent
 * team target (install+dispatch witnessed by
 * `test_action_07_ol65_invoke_installs_row_22`).
 *
 * ERRATA (phase-25, FU-259): the quoted paragraph below is the superseded
 * phase-16 T2 reasoning, kept for the record. It was correct while OL-65 was
 * open (the two angle cells were staged 0, so the fold could never be `< 0x55`);
 * FU-259 §2.4 closes OL-65, so its "slot stays NULL / row uncounted" conclusion
 * no longer holds.
 *   [SUPERSEDED] Its action-table slot was NULL — the row was not counted —
 *   because its only installer (the wired row-07 `0x816DF` invoke-now on the
 *   opponent team target) was structurally ungated in-engine: FU185-L6
 *   (first-hand) showed the gate's opponent `word[+0x6F]`/`word[+0x7D]` angle
 *   inputs were unmodeled (OL-65, staged 0), so
 *   `fifa96_entity_angle(lane_z, 0)` always yielded `+-0x100` and the folded
 *   diff against `opp_face_word == 0` could never be `< 0x55` —
 *   `out.opponent_invoke` never fired (the rows-27/29/2C precedent, a ported
 *   body with no reachable engine installer). **Ruling W8** (phase-16 T2
 *   review) held the slot NULL / the row uncounted, superseding Ruling W3's
 *   count.
 * Returns FIFA96_OK, or -FIFA96_ERR_INVALID for an out-of-range
 * `mr->record.entity_id`. */
int fifa96_match_action_22(struct fifa96_match_run *mr);

/* The ported action row-0x1A body (FU300 §2, phase-28 T4/T5): the FULL bounded
 * body of `0x7662C..0x76B21` — the phase-0xD + `[0x157AAC]>>24` head bind, the
 * `+0x89` timer, the `+0x92 > 4` tail, the 5-arm table
 * `{0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}`, the `[0x157C2C]` direction
 * machine, the movement/`'T'` block, the event ladder `{5,4,9,1,7,0}` and the
 * sit-7/sit-5 emits. Its action-table slot (`fifa96_match_action_table[0x1A]`)
 * is **wired and counted** in T5 after the live proof: the installer is the
 * ported keeper CPU ladder's three `0x1A` arms (`0x7821D/0x78245/0x7826D`),
 * armed live by the static P1 permit (`0x110680`, FU298) and the row-`0x19`
 * `[0x157C5D]` interlock (FU299 Option B); the frame body's single dispatch
 * runs the installed row (`test_keeper_installer_live_reach`). The `[0x157C2C]`
 * stamp is published in-frame before the install (G2). The event/sink
 * surfaces stay FU300-L3 legs. Returns FIFA96_OK, or -FIFA96_ERR_INVALID for
 * an out-of-range `mr->record.entity_id`. */
int fifa96_match_action_1A(struct fifa96_match_run *mr);

/* The ported action row-0x24 lane/anim body (FU-188 §4/§6, phase-16 T4): the
 * 3-stage machine `0x86510..0x866F1`. Its action-table slot
 * (`fifa96_match_action_table[0x24]`) is **NULL** — the row is not counted —
 * because its sole producer is the phase-0x20 / act-0xA timeline's code-0x24
 * install (`0x8B750`/`0x8B78B`, invoke-now on all 22 records), whose only
 * producer is situation 0 — not dispatched by any engine source/test (FU-188
 * §6). This is the rows-27/29/2C precedent (a ported body with no live engine
 * installer stays unwired and uncounted); the situation-0 producer
 * (FU188-L1) is the owner. The body is exported so it is tested directly and
 * ready for the phase-0x20 builder. Returns FIFA96_OK, or -FIFA96_ERR_INVALID
 * for an out-of-range `mr->record.entity_id`. */
int fifa96_match_action_24(struct fifa96_match_run *mr);

/* The ported action row-0x20 body (FU-210 §2/§3/§4, M2 phase-19 T3; wired +
 * COUNTED in M2 phase-20 T3, FU-217/FU-220): the actor-record "ball-camera /
 * restart-commit transition" `0x84EEC..0x85213` (the 7-arm CS table `0x84ED0`).
 * Its action-table slot (`fifa96_match_action_table[0x20]`) is **wired** to
 * `fifa96_match_action_20` — the row is **COUNTED** (`57/80 = 71%`) — because
 * the coupled installer's target (the native actor record `[0x1587D4]`, reached
 * by the phase-0x1D arm-0 `0x899A2` = `FUN_0007D9A4([0x1587D4], 0x20, 0, 0)`)
 * now has a live producer: the sit-8 arm's `[0x1587CC + side*4] ->
 * [0x1587D4]` bind (`0x8AE4B`) is **landed live** as `match_run_free_record` +
 * the `set_piece` sit-8 re-init (T3/FU-217). The arm-0 ball-record place is the
 * T4/FU-220 projection; the remainder — the ball-record anim/frame, the
 * `0x8CFAC` walks, arm-4 `0x7A490`, the block commit and the camera cells —
 * stays the FU220-L1…L5 legs. Tape-dormant. Render writes: arm 0 `pos_x/y/z` +
 * `anim_id` (`0x79B6C`/`0x8CFAC`); arm 4 `pos_x/pos_z` + `anim_id`/`frame`
 * (`0x7B9C4` -> `0x7A490`); `hidden` (`+0x9A`) is not written. Returns
 * FIFA96_OK, or -FIFA96_ERR_INVALID for an out-of-range `mr->record.entity_id`. */
int fifa96_match_action_20(struct fifa96_match_run *mr);

/* The ported action row-0x25 body (FU-243 §4.2/§6, M2 phase-23 T2; FU-82
 * §3.10): the stats sequence `0x880CC..0x886AF` (the 7-arm CS table `0x880B0`).
 * Its action-table slot (`fifa96_match_action_table[0x25]`) is **wired** to
 * `fifa96_match_action_25` — the row is **COUNTED** — because the installer is
 * **live**: the ECX-carried bulk installer `0x8D79D`/`0x8D7AD` (`MOV ECX,0x25
 * -> FUN_0008CEB8 -> FUN_0007D9A4`) sits on the `FUN_0008D098` situation-0x13
 * path and is already modelled by `fifa96_match_phase_machine_step`
 * (`code = ac5!=ac7 ? 0x25 : 3`). The prior FU189-L2 "row 25 has NO static
 * installer" verdict (a `MOV EDX/DX,0x25`-only scan) is REFUTED by FU-242 §2 /
 * FU-243 §4.2: the installer's code travels in `ECX`, which that scan cannot
 * see. The active head block (`0x88106..0x881D2`: `target := [team+0x59]` + the
 * radius-`0xC0` `0x114E04` fold, the `0x8DCD4` metric + `0x79C50` face) is the
 * leg FU243-L4 (no engine team base position / `[rec+0x8B]` angle cell); the
 * `0x8776C` id-init and the `[0x587B8+idx]`/`[0x587BB]`/`[0x587BC]` anim
 * compares are legs (unported id tables, FU184-L1 sibling), the `0x974F0`/
 * `0x36200`/`0x4C324` sinks and the `0x45001` input gate are legs
 * (FU184-L3/L4 siblings). Tape-dormant. Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for an out-of-range `mr->record.entity_id`. */
int fifa96_match_action_25(struct fifa96_match_run *mr);

/* The ported action row-0x15 body (FU-243 §4.3/§6, M2 phase-23 T3): the phase-5
 * celebration sibling of row 0x14 — the teammates' gather-and-face machine
 * `0x87CD0..0x880AE` (the 4-arm CS table `0x87CC0`). Its action-table slot
 * (`fifa96_match_action_table[0x15]`) is **wired** to `fifa96_match_action_15`
 * — the row is **COUNTED** — because the installer is the shared `0x8D433`
 * phase-5 arm (`FUN_0008CEB8(rec, 1, 10, 0x15, -1)` + the actor `0x14` install),
 * which is **already ported** at `fifa96_match_run_phase_arm`'s `phase == 5`
 * block (FU-173 §4.2 / FU-184; the FU-243-L2 "arm not ported" claim is stale —
 * that recon inspected only `fifa96_match_phase_machine_step`, states
 * 1/0x13/0x14). The body's unported sub-parts — the radius-`0xC0` `[rec+0x8A]`
 * fold, the `[0x1587B8]`/`[0x1587BB]` id tables and the `0x741B4`/`0x651F0`
 * sinks — stay numbered legs (FU243-L4). Tape-dormant. Returns FIFA96_OK, or
 * -FIFA96_ERR_INVALID for an out-of-range `mr->record.entity_id`. */
int fifa96_match_action_15(struct fifa96_match_run *mr);
