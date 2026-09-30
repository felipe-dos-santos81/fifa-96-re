# FIFA96 IVT Loose Ends Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Dispose the two carried define/create candidates from slice 26's `### Deferrals`: (1) the unowned 20-byte zero-DS staging strip `11bd:6329..633c` (bytes `8bec5257a156008b1658001ebf00008edf8b7e04` — `MOV SP,BP` / `PUSH DX` / `PUSH SI` / `MOV AX,[0x56]` / `MOV DX,[0x58]` / `PUSH DS` / `MOV DI,0` / `MOV DS,DI` / `MOV BX,[SI+4]`; re-derive live, the decode above is the plan's own starting hypothesis to verify) — walk its neighbors/entry candidates, classify CODE-vs-render-dependent (the `173b`/`4c94` disposition class), and decide the capped write (function create at cited bounds if entry-admissible / define / leave-unowned disposition row); (2) the sink byte `11bd:7750` (`cf` IRET) used by `temporarily_patch_int67_vector`'s transient install (IP word `5077` stored at `0:19C`) — dump its neighborhood, enumerate every reference to `7750` (the patch store itself cited from slice 26), and decide: 1-byte function create vs data-define vs leave-as-bytes with disposition.

**Architecture:** Read-only context walk + reference census + disposition proposals (Task 1, zero writes), then capped writes at cited bounds + verdicts + ratification of the two slice-26 deferral lines (Task 2). Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true`), `get_function_by_address`, `read_memory` (hex-vs-data protocol), `search_instructions` (authority), `search_byte_patterns`, `get_xrefs_to` (control only), `find_code_gaps` (full pagination), Task 2: real `disassemble_bytes`, `create_function`, `apply_data_type`/`set_global`-class defines, `set_comment`, `save_program` + slice-24 census protocol), `grep`, CTest (regression gate only).

**Spec:** Approved chat proposal 2026-09-30 ("IVT loose ends (smallest, bytes already re-verified): `6329..633c` unowned zero-DS staging strip + `7750` capped define", user: "yes") + `docs/ghidra/loader_rename_map.md` (`## R3 IVT cluster` — `### Deferrals` bullets for both candidates; staging chain bytes `{76c8,76cc}`, `c7045077`/`8c4c02` → `0:19C/19E`, restore pair `8f4402/8f04`@`76d1/76d4`; Fold-in #2 disclosure row with the strip bytes; `2f65`/`6335/6338` mask-unreliability proof bytes; `## vector selection logic` — `016c` patch-leg family (sibling staging shapes), `0929` live `6a20/1f` pre-entry stub precedent; `## block head 2978..2a59` — `7739` as the `CS:[0x2ad9]` writer (neighborhood context for `7750`); `## sweep-aftermath ratification` — census protocol + name-propagation ledger format).

## Global Constraints

- No C, test, CMake, or `tools/` changes; suite stays 10/10 untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data` before verbatim.
- Task 1 writes NOTHING: dry-run disassembly only; reads only; tool error on required sub-step → BLOCKED.
- Render-dependence discipline: for each candidate span, emit from AT LEAST TWO start offsets (the span start and a preceding anchor) and report both renderings — ownership claims must rest on the authority runs (`search_instructions` operand/bare-mnemonic), not on a single window (slices 26's `173b`/`4c94` rows are the template).
- Entry/admissibility math for any create: cited `CALL`/`JMP`/fallthrough evidence with nextIP+rel arithmetic shown per site; fallthrough entry only when the previous byte-run is owned CODE ending adjacent (cite last-owned insn + its length).
- Capped write path: real disassembly → ONE nudge max (`disassemble_first=false`) → RATIFY with verbatim outputs; never create over owned bytes (stop-short cites); a 1-byte function is legal only if its entry evidence is cited — otherwise the leave-as-bytes/data-define disposition wins; `save_program` after ANY write + full census protocol (count, gaps 151-class total via FULL pagination, side-effect ledger incl. name-propagation rows and overlay membership spot-checks).
- Naming bar: mechanism-level names only from the body's cited ops (segment-ops/stack-ops/stores with rendered cells are admissible; "vector/IVT"-flavored words still require in-body staging writes or INT/IRET pairing at the slice-26 bar); NOT-CONFIRMED-at-name = default name, no plate beyond `C: none — behavioral (...)`.
- Negatives scoped ("defined-insn-only, at this-slice time"); OPEN-WINDOW rows for dynamic bases with the missing leg named; controls quoted not relied on; every run pattern + match_count + scope.
- Scope guard: no new sweeps of `{0x9bc,0x9be,0x9c2,[0x56],[0x58]}` beyond what the strip's OWN ops require (census of referenced cells limited to the two/three the strip reads — writer enumeration one-line each, full stories stay deferred); `[0xe00]`/`[0x2f]`/`[0x2e]` gate-adjacent threads untouched (cite-only); `674c/675a` dive stays deferred.
- Prior sections byte-identical (append-only; new section `## IVT loose ends`); leave `fifa96.rep/**` churn unstaged; commit messages exactly per briefs; no new tests.

---

## Scope Check

Two carried candidates, each with one bounded walk + one capped-write decision. Task 2 may be fully zero-writes (both dispositions leave-bytes) — enumerated legal outcome. Nothing else in the deferrals list is admitted.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## IVT loose ends` (Task 1: strip context + renders + reference/entry tables + 7750 neighborhood + proposals; Task 2: `### Writes` or zero-writes proof + dispositions + `### Deferrals`).
- Modify (conditional): Ghidra program `/fifa96.exe` — Task 2 only: creates/defines at cited bounds; `save_program` after writes.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Strip walk + 7750 census (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## IVT loose ends` with context/render/entry/neighborhood/proposal tables)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: slice-26 deferral bullets verbatim (strip bytes `8bec5257a156008b1658001ebf00008edf8b7e04`, `7750` `cf` no-function, Fold-in #2 text); live neighbors: `get_function_by_address(11bd:6329/633d/6340-)` and the `1000:6..` gap rows; `0929` stub precedent row; `temporarily_patch_int67_vector` staging bytes; scope number.
- Produces: strip table (`| insn | bytes | render-from-6329 | render-from-anchor-prev | class |`) from TWO dry-run windows (`disassemble_bytes(6329,20)` + one from a cited earlier anchor, e.g. `6300` or `6318` — pick from live gap/ownership); ownership census: `get_function_by_address` at `6329` and byte-adjacent addresses BOTH sides (verbatim errors/owners), gap row covering the span (full pagination, quote); entry census: `search_instructions` operand runs `6329`/`0x6329` + bare `e8/e9` candidates recomputed (`nextIP+rel` shown per hit) + fallthrough check (owner ending at `6328` — its last insn cited: does it terminate flow or fall in?); SI-context row (which owning/adjacent bodies set SI, per render); cell micro-census for EXACTLY the strip's reads `{[0x56],[0x58],[SI+4]-class}`: operand runs, writers one-line each with cite or OPEN; `7750` block (`| probe | output |`) — neighborhood `read_memory(7740,32)` reconciled, ownership probes at `774f/7750/7751` + neighbors, reference runs for operand `7750`/`5077`-bytes (authoritative; the known `c7045077`@`76cc` re-cited; any others classified), what owns `7741..774f`-class (the `7739` writer region — quote `## block head`'s row if relevant, don't re-walk). PROPOSALS row: per candidate one of {create with cited entry, data-define with cited role, leave-as-bytes with reason} + naming bar pre-test (strip: segment-op staging shape → admissible mechanism name preview tested against body ops; `7750`: 1-byte create vs leave — entry evidence status).

- [ ] **Step 1: Ownership + renders**

Neighbors `get_function_by_address` ×~4 + gap row + TWO dry-run windows with full emits; per-insn two-render table.

- [ ] **Step 2: Entry + context**

Operand/bare-mnemonic authority runs with per-hit recompute; fallthrough cite; SI-context row; micro-census `{0x56,0x58,[SI+4]}` (writers one-line, OPEN where dynamic).

- [ ] **Step 3: 7750 neighborhood + references**

Reads + ownership probes + reference runs (`7750` operand, `5077` byte-pattern) + `76cc` store re-cite.

- [ ] **Step 4: Proposals + append + commit**

Append `## IVT loose ends (verified 2026-09-30, program \`/fifa96.exe\`)` — tables + proposals + bar pre-test; disposition-so-far line. Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: walk 6329 staging strip context and 7750 sink neighborhood" || true`

---

### Task 2: Execute dispositions (capped)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdicts + `### Deferrals`)
- Modify (conditional): Ghidra program `/fifa96.exe` — creates/defines only per Task-1 proposals + live pre-check; `save_program` + census protocol if any write

**Interfaces:**
- Consumes: Task-1 strip table/entry census/7750 block/proposals + bar pre-test.
- Produces: `### Writes` — per executed write: before-state (`get_function_by_address` + gap row verbatim), real `disassemble_bytes`, `create_function` (+ ≤1 nudge) or define command, verbatim response, post-read-back (bounds vs cited span, stop-short at both walls), naming decision row (bar test final: pass → rename+plate `C: none — behavioral (...)`; fail → default/NOT-CONFIRMED-at-name); if zero-writes: byte-level proofs (leave-as-bytes reason cites: e.g. no entry evidence, or render-dependent ownership); `save_program` outcome + FULL census protocol (count, gaps total via full pagination, side-effect ledger with any propagation rows, overlay spot-checks); verdict rows closing the two slice-26 deferral lines ("carried → DISPOSED here" with the new row cite); `### Deferrals` — carry-overs this slice could not close (whatever remains: `[0x56]/[0x58]` full stories, `674c/675a`, callee trees, runtime legs, name-class route, Δ1 + any NEW open leg named). Suite green; `grep -c "IVT loose ends"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Pre-check proposals against live state**

Re-run the ONE claim each disposition rests on (strip: entry-run parity + adjacency byte `6328`-owner last insn; 7750: reference parity incl `76cc` store + no-function). Contradiction → RATIFY-with-disclosure (downgrade to leave-as-bytes), per cap rules.

- [ ] **Step 2: Executes (conditional)**

Strip create (if entry-cited) at `6329` with bounds landing `..633c` (stop-short cites at `6328`/`633d` walls); `7750` only per its proposal. Rename only if the bar test PASSES on the created body's own ops (staging-shape names admissible; IVT-flavored words need the slice-26 pairing rule).

- [ ] **Step 3: Save + census + append + commit**

`save_program` if any write; census protocol in full (slice-24 procedure); append `### Writes`/verdicts/`### Deferrals` (+ `### Fix wave` trailer if needed). Gate: build + `ctest` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: dispose IVT loose ends with capped dispositions" || true`

---

## Self-Review (ran before save)

- Spec coverage: strip walk with TWO-render discipline + entry math + ownership census (T1 Steps 1-2 ← design "walk neighbors/entry, classify CODE-vs-render"); micro-census limited to strip's own cells (T1 Step 2 + Scope guard ← design "census of referenced cells limited"); 7750 neighborhood + reference runs incl. the known patch store re-cited (T1 Step 3 ← design); three-way dispositions {create, define, leave} with entry-evidence rule for a 1-byte function (T1 Proposals + T2 Steps 1-2 ← design); bar for staging-shape names + slice-26 pairing rule carried (Global + T2 ← slice 26 outcome); zero-writes legal outcome enumerated (T2 Step 1 ← design "leave-unowned disposition row"); census protocol on save (T2 Step 3 ← slice-24/26 precedent); deferral closure lines citing slice-26 bullets (T2 ← design "dispose the two carried candidates").
- Placeholder scan: no TBD; every branch has its evidence + fallback (RATIFY-with-disclosure); SI-context row defined by content, not left abstract.
- Type consistency: no C types; addresses match `## R3 IVT cluster` (strip `6329..633c` bytes `8bec5257a156008b1658001ebf00008edf8b7e04`, `7750 cf`, `76cc c7045077`, `76d1/76d4` restore, `0:19C/19E`), `## block head 2978..2a59` (`7739`), `## vector selection logic` (`0929 6a20/1f`, `173b`/`4c94` render-class rows), slice-26 deferral bullets; delta `−0x1bd0` convention; Task 2 consumes Task-1 tables by heading; commit messages distinct; Task 1 zero writes.

(End of file)
