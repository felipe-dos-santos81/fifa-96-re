# FIFA96 Vector Selection Logic Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the dispatcher-spine question open since slice 22 — who calls `dispatch_mode_vector@092c` versus `FUN_11bd_0931`, what the `0931` body does (its walk was a named deferral: entry `0931`, current bounds `0931..0937`-class as created in slice 22's H-band carve — re-derive live), and how the arm cascade (`execute_exit_arm@7c62` → the `3ed8`-family `440e`/`44ab` stores → `[BP+-0x5a]` arg → `publish_mode_vector` `6270/6277` chain → `mode_vector_source_pair@2820`/`mode_29bc_source_pair@29b8` → `CS:[BX-4]/[BX-2]` loads → `0x9bc/0x9be` near-offset pair + `[0x9c2]` far cell) selects which cell the dispatch consumes: a full static call+data census of the selection layer, with capped creates for any new body the `0931` walk exposes.

**Architecture:** Read-only evidence pass (callers/callees of both dispatchers with `search_instructions` authority + recompute arithmetic; `0931` walk to cited exits; cell writer/reader census for `{0x9bc, 0x9be, 0x9c2}` including the two `41ee/02b1` writers already recorded; arm-cascade selection legs traced in the `3ed8`/`7d15/7d1e` neighborhood with byte cites — what chooses pair-vs-far-cell) (Task 1), then capped writes for exposed bodies + verdict rows + selection-layer disposition in a new `## vector selection logic` section (Task 2). Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true` only), `search_instructions` (authority), `get_function_callers`/`get_function_callees`, `get_xrefs_to`/`get_function_xrefs` (control only), `read_memory` (hex-vs-data protocol), `find_code_gaps`, Task 2: real `disassemble_bytes`, `create_function`, `rename_function`, `set_comment`, `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-30 ("who calls `dispatch_mode_vector@092c` vs `FUN_11bd_0931`, how the arm selects cells; capped writes only if the `0931` walk lands new bodies") + `docs/ghidra/loader_rename_map.md` (`## 7c62 exit arm` — `dispatch_mode_vector@092c` rename, callers `7d15/7d1e`; `## 0290/0293 fall-through` — `execute_mode_switch` staging; `## vector dispatch handlers` — `FUN_11bd_0931` carve with `0934` load + band legs, `092c`/`0931` split rationale, H13 `clear_msw_and_callfar`; `## 2811..296c pocket + 9bc vector` — `mode_vector_source_pair@2820`, `0x9bc/0x9be` = `CS:[BX-4]/[BX-2]` sourcing, `41ee` sole `[0x9c2]` writer value `0x296d`, pocket `284c/2864` FUNs; `## callee arg question` + `## [0x9ba] consumers` — cell-writer disciplines; `## block head 2978..2a59` — `mode_29bc_source_pair@29b8`, `enable_paging_and_load_tss`, writer `2892/28a0`; `## sweep-aftermath ratification` — H11 ACCEPTED-SPLIT, scan scope `15589`, `[0xdfe]` 3-hit re-render precedent).

## Global Constraints

- No C, test, CMake, or `tools/` changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data` before verbatim (slice-17 protocol).
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only `dry_run=true`; reads only via the read tools; tool error on a required sub-step → BLOCKED report, never a write.
- Caller/callee censuses: `search_instructions` operand runs are the authority (slice-16 dead-channel ruling — `get_xrefs_to`/`get_function_xrefs` quoted as controls only); every computed target shows its delta arithmetic (`−0x1bd0` for `1000:`-space renders; far-thunk rule `0x1000:xxxx − 0x1BD0 = 11bd:xxxx`).
- Capped write path (inherited): real disassembly → at most ONE `create_function` nudge (`disassemble_first=false`) → RATIFY on second refusal, verbatim before/after; never create over defined bytes; stop-short at owned neighbors (`092c` body, band FUNs, islands) citing last-owned + first-foreign byte.
- Analyzer flow side effects: disclose in-map verbatim, never delete (slice-22 ruling 5); if `save_program` triggers a sweep, run the slice-24 census protocol (full pagination, count/gaps before-after) in `### Writes`.
- Naming bar: verb-led snake_case, mechanism-only from the body's own cited ops; CR0/LGDT/LIDT-class cite required for mode-flavored words; NOT-CONFIRMED-at-name = create-only, no rename, no plate beyond behavioral default; plates `C: none — behavioral (<role>)`; no `decode_*`, no direction words.
- Verdict vocabulary: RATIFIED / ACCEPTED / NOT-CONFIRMED-at-name / DYNAMIC-ONLY / OPEN-WINDOW; negatives = "not attributable from enumerated sweeps, defined-insn-only, at this-slice time" never "doesn't exist"; OPEN-WINDOW rows for dynamic bases.
- Scope guard: the arm-cascade TRACE goes one hop from `7c62`'s own recorded callees only; callee trees of the 13 handlers stay deferred; R3's `INT 0x67` cluster is NOT this slice (queued slice 26); `[0x9ba]` armed-value runtime stays trace-blocked (FU notes).
- Leave `fifa96.rep/**` churn unstaged; prior sections byte-identical (append-only); commit messages exactly per the briefs; no new tests; `save_program` only if Task 2 writes.

---

## Scope Check

Spine question (callers + walk + cell-selection) with one defensible write window (exposed bodies from the `0931` walk). Deliverable per task: map rows + suite green. Handler callee trees, R3 IVT, runtime values — named non-goals. If the `0931` walk exposes nothing new, Task 2's write window is empty and it still ships the verdicts — that outcome is enumerated, not a failure.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## vector selection logic` (Task 1: caller census + walk + cell tables + selection trace; Task 2: `### Writes` if any + verdict rows + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: capped creates/renames at Task-1-cited ranges; `save_program` after writes.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Callers, walk, cell selection (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## vector selection logic` with census/walk/cell/trace tables)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: both dispatcher bodies live (`get_function_by_address(11bd:092c)` → `dispatch_mode_vector`, `(11bd:0931)` → `FUN_11bd_0931`, bounds quoted); recorded callers (`7d15/7d1e` legs in `execute_exit_arm`, `0d80`→`0931` from slice 22's walk rows); cell facts (`0x9bc/0x9be` loaders `6270/6277`-chain, writers of `CS:[BX-4]/[BX-2]` source = the pair tables `2820..2823`/`29b8..29bb`, `[0x9c2]` writers `{02b1? — re-verify: 41ee sole static writer value 0x296d, 02b1 READ}` from block-head + pocket sections); band-leg targets for the `0931` internal edges (already mapped, recompute cites only).
- Produces: caller census table (`| caller insn | bytes | nextIP+rel target (arith shown) | resolves to |`) — full `search_instructions` runs for `e8`/`e9`/`ff16`/far-`call` forms with operands resolving to `092c` and `0931` (both segment renders + delta math), `get_function_callers` quoted as control; walk table for `FUN_11bd_0931` from its first foreign byte at entry (`0938` is `enable_paging...`? NO — `0938` is `stage_ss_selector`-adjacent: re-derive live) to cited exits (`c3`/`cb`/`ea`/`e9`-tail forms), classifying every emitted instruction incl. any new body boundary exposed; cell census table for `{0x9bc, 0x9be, 0x9c2}` = writers ∪ readers (literal + window forms per slice-20/22 discipline, per-run pattern + match_count + scope `instructions_scanned` "at this-slice time" uniformity note); selection trace table (`| leg | cite | what it decides |`) one hop from `7c62`'s recorded callees: the arm-cascade store `440e/44ab` family → what selects pair-table vs far-cell path, incl. the `2864` pocket function's role (writes `[0xdfe]` AND is a `2820`-pair consumer?) — verify with its own ops.

- [ ] **Step 1: Dispatcher bounds + callers**

`get_function_by_address(11bd:092c)` + `(11bd:0931)` verbatim; authority runs for operand-resolving callers of each (`26c8`/`e8`-rel16 → target `092c`: nextIP+rel arithmetic shown; far-`call` `ff16/9a` cell forms whose cell value = `092c`-class cited from the pair tables); controls quoted.

- [ ] **Step 2: Walk 0931**

`disassemble_function(11bd:0931)` + dry-run windows beyond current bounds; emit-classify to exits; list newly exposed contiguous CODE runs (with stop-short cites at every foreign byte).

- [ ] **Step 3: Cells + selection trace**

Cell census runs; the one-hop cascade trace with cites; classify what statically distinguishes the `092c` path from `0931`'s (arm value? `[0x9ba]`/cell pairing? caller-side `0d80` context — read `get_function_by_address` for `0d80`'s owner, cite its call insn).

- [ ] **Step 4: Append + verify + commit**

Append `## vector selection logic (verified 2026-09-30, program \`/fifa96.exe\`)` — tables + disposition-so-far (caller counts per dispatcher, walk outcome incl. exposed-bodies list or NONE, cell census status vs prior records — flag any delta from slice-22/23/24 quotes, selection answer or OPEN). Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: census vector dispatcher callers, walk 0931, trace cell selection" || true`

---

### Task 2: Capped writes + verdicts

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdict rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` — ONLY if Task 1 exposed new contiguous CODE runs: capped creates at cited ranges; `rename_function` + plate only at the naming bar; `save_program`

**Interfaces:**
- Consumes: Task-1 walk table exposed-bodies list + selection trace + cell census.
- Produces: `### Writes` — before-state (`get_function_by_address` no-function + gap row verbatim per range), command + verbatim response, post-read-backs (bounds vs proposal, count delta, gap re-page with FULL pagination, scope drift note per slice-24 errata precedent — live `instructions_scanned` number quoted); if NONE exposed: "zero writes — walk fully inside existing ownership" + the byte-level proof (last-owned/first-foreign cites). Verdict rows per dispatcher: `092c` (RATIFIED as recorded), `0931` role verdict (rename at bar if its ops are decisive — e.g. if the walk shows it as the near-offset pair dispatch: name candidates must be mechanism-level, cited; else NOT-CONFIRMED-at-name + create-only/disposition); selection answer row (what statically decides pair-vs-far) or OPEN-WINDOW with the missing leg named; `### Deferrals` — R3 IVT `INT 0x67` cluster (slice 26 queued), handler callee trees, `[0x9ba]` armed-value runtime (FU-blocked), `[0x2fa]` consumers, runtime writers, name-class rename route, Δ1. Suite green; `grep -c "vector selection logic"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Creates (conditional)**

Per exposed run: real `disassemble_bytes` → `create_function` at cited start → bounds check → ONE nudge max → RATIFY verbatim. Stop-short at defined edges.

- [ ] **Step 2: Rename/plate (conditional, bar-gated)**

`0931` (or exposed bodies) renamed only with mechanism-level ops cited from THIS slice's walk; plate `C: none — behavioral (...)`; `save_program` if ANY write; post-read-backs + sweep-census protocol if a save happened (slice-24 procedure: count, gaps full-pagination total, side effects disclosed).

- [ ] **Step 3: Append + verify + commit**

Append `### Writes` + verdicts + `### Deferrals` (+ `### Fix wave` trailer if own-row edits needed). Gate: full build + `ctest` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: execute selection-slice writes with dispatcher verdicts" || true`

---

## Self-Review (ran before save)

- Spec coverage: callers of both dispatchers w/ authority-runs + arith (T1 Step 1 ← design "who calls 092c vs 0931"); `0931` walk to cited exits + exposed bodies (T1 Step 2 ← design + slice-22 deferral); cell selection layer `{0x9bc,0x9be,0x9c2}` census + one-hop arm trace (T1 Step 3 ← design "how the arm selects cells"); capped creates only-if-exposed with enumerated zero-write outcome (T2 Step 1 + Global ← design "capped writes only if the 0931 walk lands new bodies"); verdict vocabulary + bar (T2 ← constraints); save-sweep protocol carry-forward (T2 Step 2 ← slice-24 lesson); R3 excluded (Scope guard ← design "slice 26").
- Placeholder scan: no TBD; exposed/none both enumerated; bar-gated rename has its negative outcome (NOT-CONFIRMED-at-name) specified; zero-writes is a legal `### Writes` content.
- Type consistency: no C types; addresses match map records (`092c` + callers `7d15/7d1e`, `0931`+`0d80`, `41ee`→`0x296d`, `2820..2823`, `29b8..29bb`, `6270/6277`, `2864` pocket + `2892/28a0`, `0x9ba` cell, `0x9bc/0x9be` loaders, exit opcodes `c3/cb/ea/e9`, delta `−0x1bd0`, scope `15589` precedent); Task 2 consumes Task-1 tables by heading; commit messages distinct; date header 2026-09-30 (session date).

(End of file)
