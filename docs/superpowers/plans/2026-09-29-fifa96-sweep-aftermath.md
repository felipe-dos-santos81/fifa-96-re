# FIFA96 Sweep-Aftermath Ratification Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ratify the drift the two save-time auto-analysis sweeps (slices 22 and 23) left in the saved program: the superseded H11 record (`FUN_11bd_0a9f` ratified as `[0a9f..0ae1]` in slice 22, live now as `[0a9f..0ad2]` + `FUN_11bd_0ad5..0ae1` with the retry bytes `11bd:0ad3..0ad4` as no-function gap island `1000:26a3..26a4`), the 8 `FUN_1991_*` overlay bodies that silently re-cut prior-slice gap-row records (`0400`, `21e2`, `2999`, `2b3f`, `4542`, `4930`, `4b0a`, `4e38`), the three far-return halves that moved undefined→defined-unowned (`1000:2235..2266`, `1000:2605..262d`, `1000:80cf..8118`), the three-way `caseD_0` name collision (`1000:0018`, `11bd:0337`, `1991:4f40`), the `FUN_11bd_0c9f` entry-before-body anomaly (entry `0c9f`, body `0c84..0d0b`, 27 B lead + `1000:286b..286e` undefined hole), and the deferred text errata across previously-merged sections (the `0xDE22` slip — true `0xDDFE` — and the `14638`→`15589` scan-size drift with the `[0xdfe]` operand-run now rendering 3 hits since `2978` became defined).

**Architecture:** Read-only drift enumeration + evidence-grounded ratification proposals with a live-flow answer to the H11 question (does anything statically branch INTO `11bd:0ad5`? — if yes, accept the split as legitimate; if no, re-merge is admissible) (Task 1), then capped execution of the rulings (delete-and-recreate ONLY if the H11 no-external-in-flow evidence supports it; `caseD_0@11bd:0337` disambiguation rename if the ruling picks it; otherwise zero program writes) + append-only `### Errata` + `### Drift` rows + verdicts in a new `## sweep-aftermath ratification` section (Task 2). Prior sections stay byte-identical — errata are recorded by QUOTING the earlier text and printing the correction, never by editing it. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true` only), `find_code_gaps`, `search_instructions` (authority), `get_xrefs_to`/`get_function_xrefs` (control only), `list_functions_enhanced`, `get_function_count`, `read_memory` (hex-vs-data protocol), Task 2 only: `delete_function`, `create_function`, `rename_function`, `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (ratify all sweep drift: H11 re-merge-vs-accept ruled on live external-in-flow evidence; 8 overlay bodies ratified against the records they re-cut; 3 defined-unowned halves status-quoted; `caseD_0` collision hygiene ruled before acting; `0c9f`/`286b` anomaly status rows; `0xDE22→0xDDFE` erratum + scan-drift note carried as append-only `### Errata` rows since slice-23's section is now a prior section) + `docs/ghidra/loader_rename_map.md` (`## vector dispatch handlers` — H11 ratified record `[0a9f..0ae1]`, plate, `0ad5..0ae1` shared-tail proposal text, `1000:26a3..26a4` orphan row; `## block head 2978..2a59` — `### Writes` side-effect ledger (3 `11bd` + 8 `1991`, Δ+2, count 329), the `286b..286e` undefined-hole bullet, the `0c9f` 27 B anomaly row, the `14638` scoped counts, the Fix wave 1 `0xe822` refutation with the `0xDE22` slip, the operand-`0xdfe` 1-hit "invisible by construction" pre-state claim; final-review triage minors 1-4).

## Global Constraints

- No C, test, CMake, or `tools/` changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data` before verbatim (slice-17 protocol).
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only `dry_run=true`; reads only via the read tools; tool error on a required sub-step → BLOCKED report, never a write.
- Program mutations belong to Task 2 and are capped: at most ONE `delete_function`/`create_function` pair for the H11 question (gated on Task-1's external-in-flow evidence — if any cited instruction statically branches into `0ad5`, the split is LEGITIMATE and NO delete happens; accept-split needs zero writes), at most ONE `rename_function` for the collision, verbatim before/after for every write, RATIFY-on-refusal, never fight the analyzer, `save_program` after any write batch with the slice-23 archive-repair recurrence check live (stale `user/` entry → backup + clear + disclose per slice-23 precedent).
- A `delete_function` is only admissible when its recreation at the same entry is part of the same bounded attempt (H11 re-merge = delete `0ad5` + delete `0a9f` + create `0a9f`; if create lands at `[0a9f..0ad2]` again, ONE nudge with `disassemble_first=false`, then RATIFY the split and DISCLOSE that re-merge failed live); never delete a function whose record other slices ratified unless the ruling's evidence is in-map.
- Append-only map discipline: prior sections (everything through `## block head 2978..2a59`) byte-identical; errata recorded in the new section's `### Errata` by verbatim quote + correction; no editing of merged text ever.
- Verdict vocabulary: RATIFIED / ACCEPTED-SPLIT / NOT-CONFIRMED-at-name / DYNAMIC-ONLY / OPEN-WINDOW; naming bar unchanged (CR0/LGDT/LIDT-class cite for mode-flavored words); no `decode_*`, no direction words; `caseD_0` hygiene rename to segment-convention `FUN_11bd_0337`-style default name is NOT a semantic claim and needs no bar.
- Scope guard: no handler re-walks, no far-ret-half ATTRIBUTION work (status-only), no `[0x9b4]`/`[0x40]`/`[0x2fa]` consumer sweeps, no open-thread enumeration beyond the drift list — all cite-only deferrals.
- Leave `fifa96.rep/**` churn unstaged (including the slice-23 repair deletions already showing); commit messages exactly per the briefs; no new tests.

---

## Scope Check

One program-state reconciliation plus the standing errata log, two gates: evidence + rulings (Task 1), capped execution + ratification rows (Task 2). Deliverable per task: map rows + suite green. Handler semantics, overlay function roles, runtime writers — named non-goals.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## sweep-aftermath ratification` (Task 1: drift enumeration + evidence tables + ruling proposals; Task 2: `### Writes` if any, `### Ratifications`, `### Errata`, `### Drift`, `### Deferrals`).
- Modify (conditional): Ghidra program `/fifa96.exe` — Task 2 only: H11 re-merge attempt (only if no external in-flow into `0ad5`) and/or `caseD_0` rename; `save_program`.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Drift enumeration + evidence-grounded rulings (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## sweep-aftermath ratification` with enumeration/evidence/decision tables)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: H11 ratified record + plate from `## vector dispatch handlers`; side-effect ledger from `## block head 2978..2a59 ### Writes` (3+8=11, Δ+2, count 329); the `286b`/`0c9f` bullets; the Fix wave 1 `0xe822` row (source of the `0xDE22` slip); live program current state.
- Produces: drift census table (`| item | ratified record (quoted) | live state (verbatim tool output) | class |`) covering: (1) H11 pair `FUN_11bd_0a9f` + `FUN_11bd_0ad5` with BOTH current bounds + `get_comment` + the `1000:26a3..26a4` gap row + `get_function_by_address(11bd:0ad3)` no-function response; (2) the external-in-flow question for `0ad5`: `search_instructions` operand runs for `0x0ad5`/`0ad5` (and near `e9`/`eb` rel16 arithmetic: every JMP/CALL whose computed target = `11bd:0ad5`, incl. `1000:`/`1991:`-space renders with delta math `−0x1bd0` shown) — outcome drives the ruling (any hit → ACCEPTED-SPLIT legitimate; zero hits → re-merge admissible); also `get_xrefs_to(11bd:0ad5)` quoted as control only (dead-channel discipline); (3) the 8 overlay `FUN_1991_*` bodies with bounds vs the specific prior-slice gap-row records each re-cut (row quotes + current rows, reassembly sums where the record still holds); (4) the three defined-unowned halves: current gap rows `1000:2235..2266`/`2605..262d`/`80cf..8118` verbatim + `has_undefined_bytes` + neighbors, mapped to their slice-22 deferral-recorded ranges (`0665..0674`, `0a35..0a5d`, far-ret half family) with delta arithmetic; (5) `caseD_0` triple: `get_function_by_address` all three + `list_functions_enhanced` grep for `caseD_0` + ambiguity demonstration (what `get_function_callers("caseD_0")`-style name lookup would hit — or state name-lookup ambiguity without live call if tools key by address only); (6) `0c9f` anomaly + `286b` hole current rows; (7) scan-size drift: live `get_function_count` + `search_instructions` scope now + `[0xdfe]` operand-run re-render (expect 3 hits incl. the now-defined `2978` — quote) + one of the prior-run scoped counts for comparison. RULINGS section in-map: proposed decision + evidence cite for each actionable item (H11 accept/re-merge; caseD rename/don't; everything else RATIFIED-as-status-quo).

- [ ] **Step 1: H11 pair + flow evidence**

`get_function_by_address(11bd:0a9f)`/`(0ad5)`/`(0ad3)` + `disassemble_function` both + `get_comment` each + gap rows `1000:26a3..26a4` + `1000:2693..26a2`-family context. Then the authority run: `search_instructions` for operand forms resolving to `0ad5` (`e9`, `eb`, `e8`, `ff25/ff16` cell forms where a cell holds `0ad5`-class — cell-holders included only if their writer is cited) + full near-JMP arithmetic for the pocket/band (`X + rel16 + nextIP = 0x0ad5` candidates enumerated from the H12/H11 families already in-map). Quote every hit or the exhaustive zero with pattern + `match_count` + scan scope.

- [ ] **Step 2: Overlay census**

8 × `get_function_by_address(1991:<name-or-addr>)` bounds; current `find_code_gaps` rows for `1000:990e/9d8c/de60/e270/e424/e685/e7a0` families + vanished-span page steps; quote each against the specific prior-slice row it re-cut (from `## vector dispatch handlers` gap quotes and `## block head 2978..2a59` ledger — the ledger already has the row-delta cites; re-verify live, do not copy).

- [ ] **Step 3: Halves, collision, anomaly, drift**

Gap-row verbatims for the three halves + `get_function_by_address` at their starts (expect no-function = unowned-confirmed); `caseD_0` triple + collision demo; `0c9f`/`286b` current rows; live count + scope + `[0xdfe]`-operand re-render with the new `2978` hit classified (READ at `2978` inside `enable_paging_and_load_tss` now).

- [ ] **Step 4: Rulings + append + commit**

Compose `## sweep-aftermath ratification (verified 2026-09-29, program \`/fifa96.exe\`)` — census tables + `### Rulings (proposed)` with per-item evidence cites: H11 → ACCEPTED-SPLIT if external-in-flow found (name the branching instruction verbatim) else RE-MERGE ADMISSIBLE; caseD_0 → rename `11bd:0337` to `FUN_11bd_0337` if ambiguity demonstrably degrades lookups, else leave; all others RATIFY status quo. Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: enumerate sweep-aftermath drift with flow evidence and ruling proposals" || true`

---

### Task 2: Execute rulings + ratification/errata rows

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes`, `### Ratifications`, `### Errata`, `### Drift`, `### Deferrals` to your section)
- Modify: Ghidra program `/fifa96.exe` ONLY if Task-1 rulings demand (H11 re-merge attempt and/or collision rename); `save_program` after writes

**Interfaces:**
- Consumes: Task-1 census + rulings (binding: execute exactly what was proposed unless a pre-write live check contradicts — then RATIFY-with-disclosure instead).
- Produces: `### Writes` (either "zero writes — rulings executed ratification by disclosure" or verbatim delete/create/rename sequence with before-states, nudge rule, post-read-backs, count-delta reconciliation, save + repair-check); `### Ratifications` (per-item verdict rows: H11 ACCEPTED-SPLIT/RE-MERGED with the final bounds + which slice-22 row is superseded + plate disposition; overlays RATIFIED with the re-cut map list; halves status-quo rows; `0c9f`/`286b` rows; caseD_0 disposition); `### Errata` (verbatim quote of the slice-23 `0xDE22` text + corrected arithmetic `0xe822+0xf5dc−0x10000 = 0xDDFE` (both renderings) with the note that the refutation STANDS (`0xDDFE ≠ 0xdfe`) and kept value re-verified; verbatim quote of a representative `14638`-scoped row + drift note → live scope now, `[0xdfe]` operand-run 3-hit re-render with `2978` classified); `### Deferrals` (far-ret-half attribution, overlay roles, `0x2fa`/`0x9b4`/`0x40` consumers, R3 IVT cluster, runtime writers — carry-forward one-liners). Suite green; `grep -c "sweep-aftermath"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Pre-write contradiction check**

Re-run the ONE ruling-critical read live (the `0ad5` external-in-flow evidence — a single repeat of the authority pattern; contradiction (new hit appears / hit vanishes vs Task-1) → switch to RATIFY-with-disclosure per cap rules and say so in `### Writes`).

- [ ] **Step 2: Execute (conditional, capped)**

RE-MERGE branch only if Task-1 found zero external in-flow: before-states (both `get_function_by_address` + `disassemble_function` verbatim, both comments, the ratified slice-22 record quoted); `delete_function(11bd:0ad5)`; `delete_function(11bd:0a9f)`; `create_function(11bd:0a9f, disassemble_first=false)`; bounds check; if lands `0a9f..0ad2` again: ONE nudge `create_function(11bd:0a9f)` default args; RATIFY split on second refusal with verbatim everything. RENAME branch only if ruled: `rename_function(old_name=..., new_name="FUN_11bd_0337")` — expect naming-tool friction on the duplicate `caseD_0` selector (address-keyed retry per tool docs); verbatim refusal if any. `save_program` (archive-repair recurrence check live); post-read-backs: count delta (re-merge: 329→327 or split kept: 329; nudge outcomes quoted), gap rows re-page, `caseD_0` grep count now.

- [ ] **Step 3: Append + verify + commit**

Append `### Writes`/`### Ratifications`/`### Errata`/`### Drift`/`### Deferrals` (+ `### Fix wave` trailer if own-row edits needed). Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: ratify sweep aftermath with errata and scan drift"`

---

## Self-Review (ran before save)

- Spec coverage: H11 re-merge-vs-accept on live external-in-flow evidence (T1 Step 1 + T2 Step 2 ← design "check `26a3` island + entry evidence"); 8 overlay ratifications vs re-cut records (T1 Step 2 + T2 Ratifications ← design); 3 defined-unowned halves status-only (T1 Step 3 ← design "only their defined-unowned status is ratified"); caseD_0 rule-before-act (T1 Step 3 + T2 Step 2 ← design); `0c9f`/`286b` anomaly rows (T1 Step 3); errata as append-only quote+correction (T2 Errata ← design + append-only ruling; slice-23 section is now prior — editing it forbidden, so the errata mechanism is the new `### Errata`, which matches final-review "next slice's wave" triage); drift rows with re-scoped counts (T1 Step 3 + T2 Drift ← Minor 2 triage); auto-analyze-on-save NOT changed this slice (program-option edit is outside the capped-map-writes envelope; carried as Deferrals recommendation ← design guard).
- Placeholder scan: no TBD; H11 both outcomes fully specified (accept = zero-write + named instruction cite; re-merge = exact 3-command sequence + nudge + ratify); rename branch has its friction-handling; "zero writes" is an enumerated legal outcome.
- Type consistency: no C types; addresses match `## vector dispatch handlers` (`0a9f..0ae1` record, `26a3..26a4`, `0ad5`, halves `0665..0674`/`0a35..0a5d`) and `## block head 2978..2a59` (3+8 ledger, `14638`, `0xDE22` slip row, `286b..286e`, `0c84..0d0b`/`0c9f`, `caseD_0@0337..033b`, count 329, delta `−0x1bd0`); Task 2 consumes Task-1 rulings by heading reference; commit messages distinct; Task 1 zero writes.

(End of file)
