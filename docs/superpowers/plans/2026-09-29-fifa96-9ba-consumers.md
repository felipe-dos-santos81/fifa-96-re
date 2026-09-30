# FIFA96 [0x9ba] Consumers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ])` syntax for tracking.

**Goal:** Attribute consumers of the cell `[0x9ba]` — where slice 19 proved the `0x29bc` arg lands verbatim (`6255 891eba09` `MOV [0x9ba],BX`) — using the discipline the slice-19 final reviewer specified: literal operand sweeps AND the displacement-window sweep that literals can't see (readers computing `CS:[base+disp]` from constant-loaded bases, the mirror of `626d..6277`'s own pattern), delivering a three-way consumer verdict (READERS-FOUND with branch/no-branch outcome / NONE-FROM-DISCIPLINE / PARTIAL), and resolving whether the `0x29bc` value — if armed at runtime — can reach an indirect CALL/JMP anywhere in the static surface after the second layer.

**Architecture:** Read-only attribution pass (enumerated sweeps + base-load inventory + window math + conditional one-hop branch trace, zero Ghidra writes, Task 1), then disposition + deferrals + writes-branch (zero-write unmoved-proof unless a new boundary materializes, capped path otherwise — Task 2). Rows appended to `docs/ghidra/loader_rename_map.md` under `## [0x9ba] consumers`; prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`search_instructions` (mnemonic+operand substring authority), `read_memory` (protocol-reconciled), `disassemble_function` (read-only dumps), `get_function_by_address`, `get_xrefs_to`/`list_data_items_by_xrefs` (control only — dead channel for absolute-operand forms), `find_code_gaps`, `analyze_dataflow` (optional accelerator; disassembly is the authority it must agree with), `disassemble_bytes` (Task 1 `dry_run=true` only; Task 2 real only under materialization), `create_function`/`rename_function`/`set_comment` (Task 2 capped path only), `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (consumer attribution per the reviewer's stated method — literal sweep + displacement-window sweep + control; reader→target one-hop question; one-layer guard from the store; `[0x9b8]` neighbor family cite-only adjacency; capped writes only if a boundary materializes) + `docs/ghidra/loader_rename_map.md` (`## callee arg question` — store site `6255 891eba09`→`[0x9ba]`, arg verbatim, `get_xrefs_to(0x9ba)` ×0, consumers named-and-deferred, `[0x9bc]/[0x9be]` fed at `6270`(`2e8b47fc`)/`6277`(`2e8b47fe`)-family with base `MOV BX,0x2824`@`626d`(`bb2428`) and `BX=SP`→`MOV BX,[BX+0x2]`@`6252` prologue chain; `## 0x29bc slot consumers` — callers zero indirect, `452f`/`4536` re-derives; `## paging block 2978..2ada` — zero static entries, block `2978..2ada` size-355 gap `1000:4548..1000:46aa`, quote-protocol note binding; slice-8 — `publish_mode_vector@6250` writes `[0x9bc]`).

## Global Constraints

- No C, test, CMake, or `tools/` changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler and `analyze_dataflow`; every `read_memory` quote reconciles the response's `hex` field against its own `data` array before being called verbatim (slice-17 quote-protocol — it lives in `## paging block 2978..2ada`).
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only with `dry_run=true`; `disassemble_function`/`search_instructions`/reads are read-only; if a tool errors on a required sub-step, report BLOCKED rather than writing. All listing mutations belong to Task 2 and ONLY under materialization (below), with the inherited cap: real disassembly → at most ONE `create_function` nudge (`disassemble_first=false`) → RATIFY on second refusal; delete nothing beyond objects this slice's own nudge created.
- Do NOT invent addresses: cell addresses and every sweep hit resolve via live tool responses; far-target render delta `0x1bd0` recomputed per hit; `get_xrefs_to`/`list_data_items_by_xrefs` are control-only (dead channel for absolute operands — established slice-16/18/19).
- Sweep completeness criterion: the literal pass must enumerate every pattern run (`0x9ba`, `[0x9ba]`, `CS:[0x9ba]`-render variants, and the family `0x9b8`/`0x9bc`/`0x9be` as adjacency control) with hit counts; the displacement pass must (a) inventory constant base-loads (`MOV BX/SI/DI,imm`-family — at minimum the known `626d` `MOV BX,0x2824` and any further bases found via `search_instructions`) whose `base+disp` window can hit `0x9b8..0x9bf`, (b) run `[BX+`/`[SI+`/`[DI+`/`[BP+` operand-pattern sweeps, and (c) show per candidate hit the arithmetic (base + disp → resolved cell) or its rejection — negatives carry the defined-instructions-only caveat.
- One-layer guard: from the store (`6255`) consumers are attributed; consumers of any found readers are named-and-deferred, not dived; a reader's value reaching an indirect CALL/JMP target is followed ONE hop with citations only — target function bodies not walked, callees address-only.
- `[0x9b8]`-family adjacency (slice-5 `6400`-era open items, `[0x9bc]/[0x9be]` slice-8 writes) stays cite-only: this slice attributes `[0x9ba]` consumers; the neighbor cells appear only in sweep tables and control rows.
- Materialization (the only Task-2 write branch): Task 1 verdicts READERS-FOUND **and** a cited reader's load feeds an indirect transfer whose target resolves statically (e.g. armed value `0x29bc` → lands inside `2978..2ada` or any code-shape target) **and** a defensible function boundary at that target is citable from a dry-run walk to an exit. Only then Task 2 creates (capped); otherwise zero-write disposition with the unmoved-proof pair.
- Verdict honesty: NONE-FROM-DISCIPLINE means "no consumers attributable from enumerated static sweeps" — never "no consumers exist" (runtime legs `[0x40]`/`[0x9c0]` precedent); READERS-FOUND rows must state whether `0x29bc` (conditional-armed) is consumed as data or reaches a transfer; no direction words or caller-lore; NOT-CONFIRMED gets no rename/verdict row.
- Prior sections read-only (pure append, zero deletions); `save_program` after any write batch; leave `fifa96.rep` churn unstaged; commit messages exactly per the briefs.

---

## Scope Check

One question (who reads `[0x9ba]`, and does the armed value reach a transfer?), two gates: read-only attribution (Task 1), disposition + writes-branch (Task 2). Deliverable per task: map rows + suite green. Reader-consumers, `[0x9b8]`-family roles, `1e9f`/`6250` full verdicts, block body, runtime installers — all named non-goals.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## [0x9ba] consumers` section (Task 1 sweep inventory + reader tables + branch-trace/disposition-so-far; Task 2 `### Disposition` + `### Writes` (zero-write proof or capped path) + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only, ONLY under materialization: real `disassemble_bytes` at cited target + `create_function` (capped) + rename/plate if CONFIRMED + `save_program`; otherwise two read-only calls max.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Consumer sweeps + one-hop branch question (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## [0x9ba] consumers` with sweeps table + reader table + branch trace + disposition-so-far)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: slice-19 store cite (`6255` `891eba09`, value = arg = `0x29bc` conditional-armed, `get_xrefs_to(0x9ba)` control ×0, base-load chain facts `6250 8bdc`→`6252 8b5f02`, `626d` literal `0x2824`), slice-18 caller facts (`452f`/`4536` — callers themselves zero indirect), slice-17 protocol note.
- Produces: sweeps inventory table (`| sweep run | hits | classification (read/write/adjacency-control) |`) — literal family runs (`0x9ba` + variants, `[0x9b8]/[0x9bc]/[0x9be]` as control rows), constant base-load inventory (`search_instructions` operand runs for `MOV BX,`, `MOV SI,`, `MOV DI,` imm-forms — hits whose base+window reaches `0x9b8..0x9bf` listed; `626d` reproduced as the known instance), disp-form sweeps (`[BX+`, `[SI+`, `[DI+`, `[BP+`) with per-candidate arithmetic or rejection, control xrefs/data-items quoted; a reader table (`| reader site | bytes | render | cell resolved | value flow |`) — every confirmed read of `[0x9ba]`/window-hit, each classified as literal-disposition, window-resolved, or rejected; the branch question — per reader, does the loaded value feed a CALL/JMP (any transfer form) within one hop (cite site + operand + transfer bytes), else terminal form (store/arith/discard) cited; disposition-so-far line: READERS-FOUND-REACH-TRANSFER (chain cited, materialization possible) / READERS-FOUND-DATA-ONLY / NONE-FROM-DISCIPLINE / PARTIAL, with the armed-conditional phrasing.

- [ ] **Step 1: Literal family sweeps**

`search_instructions` program-wide with `operand_pattern` runs (record each exact pattern + `match_count`): `0x9ba`, `9ba`, `0x9b8`, `0x9bc`, `0x9be` (family control), plus any `CS:[0x9ba]` render variant the tool accepts. Classify every hit (instruction text quoted): read-of-`[0x9ba]` / write / other-cell / false-string (frame-local `[BP+-0x...]`-style lookalikes rejected). Reconcile any `read_memory` follow-up hex-vs-data.

- [ ] **Step 2: Base-load inventory + disp-window sweep**

`search_instructions` runs: mnemonic `MOV` + operand patterns `0x2824`, `0x9b8`, `0x9ba`, `0x9bc`, `0x9be` (imm-source constants), plus operand sweeps `[BX+`, `[SI+`, `[DI+` (all programs). For each `[base+disp]`-rendered hit: find the base's loaded value at that site (constant `MOV base,imm` cited; stack-relative rejected with reason; dynamic base → recorded as OPEN-WINDOW with the site cited, per caveat), compute `base+disp` with the `0x9b8..0x9bf` window, list hits whose window resolves to `[0x9ba]` (or reject with arithmetic). `analyze_dataflow` optional accelerator only, reconciled to disassembly.

- [ ] **Step 3: One-hop transfer question**

For each confirmed reader: `disassemble_function` of its owner (read-only dump, totals quoted as scope) and follow the loaded register/value to any `CALL`/`JMP`/`Jcc`/far form within one hop (site, bytes, operand, resolved-target math). Found-transfer: cite the full chain and determine target status statically (literal target vs dynamic target whose value can only be runtime-armed). No chain: terminal form cited instead. `get_xrefs_to`/`list_data_items_by_xrefs` on `0x9ba` as control (expect ×0 — dead channel).

- [ ] **Step 4: Append + verify + commit**

Append `## [0x9ba] consumers (verified 2026-09-29, program \`/fifa96.exe\`)` — paragraph + sweeps inventory + reader table + branch trace + disposition-so-far line (armed-conditional phrasing per honesty rule). Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → expect 10/10 (docs-only). Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: sweep [0x9ba] consumers literal and window and test one-hop transfer" || true`

---

### Task 2: Disposition + writes-branch + deferrals

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Disposition` + `### Writes` (zero-write proof form OR capped path) + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` — ONLY under materialization: real disassembly at cited target + `create_function` (capped) + rename + plate if CONFIRMED + `save_program`; otherwise read-only calls only

**Interfaces:**
- Consumes: Task-1 sweeps table + reader table + branch trace + disposition-so-far.
- Produces: `### Disposition` — the three-way consumer outcome restated with citations; explicit armed-conditional line: does the `0x29bc` value reach an indirect transfer (YES-where/NO-DATA-ONLY/NO-CONSUMERS-STATICALLY); whether the `0x29bc` entry-lead chain (slices 17→19) is now statically CLOSED-with-answer or re-extended (any reader's own consumers named-and-deferred). `### Writes` — zero-write branch: unmoved-proof pair quoted live (`get_function_by_address(11bd:29bc)` + `find_code_gaps` covering row `1000:4548..1000:46aa` size 355 unchanged); materialization branch: every command + verbatim response + post-read-backs + cap compliance. `### Deferrals` — reader consumers (if any) address-only; block `2978..2ada` status (fully named-open unless materialization shrank it); `[0x9b8]/[0x9bc]/[0x9be]` family roles untouched (cite-only, one line); `1e9f`/`6250` full verdicts non-goal; runtime legs `[0x40]`/`[0x9c0]` unchanged; prior statuses exact (which slice-19 parked minors this section's cites supersede where adjacent). Suite + `grep -c "[0x9ba] consumers"` nonzero.

- [ ] **Step 1: Restate disposition**

Per Task-1 verdict with the sweeps' totals as enumeration scope (literal run counts, inventory size, window hits). Armed-conditional honesty required: readers exist only under armed-state reasoning — the store itself is conditional; say so exactly once in the Disposition, not per row.

- [ ] **Step 2: Writes (branch)**

Zero-write (default expectation): ZERO mutations; record the unmoved-proof pair verbatim in the slice-18 `### Writes` form. Materialization (only if Task 1 satisfied all three conditions): real `disassemble_bytes` at cited target range → `create_function` → post-check bounds; over/under-shoot → one `disassemble_first=false` nudge → second refusal → RATIFY; rename/plate only if CONFIRMED-with-legs; never touch prior-created FUNs or `2811..296c`. `save_program` only if any mutation occurred.

- [ ] **Step 3: Append + verify + commit**

Append the three blocks. Run suite (expect 10/10) + `grep -c "\[0x9ba\] consumers"` nonzero. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: settle 9ba consumer disposition and writes-branch" || true`

---

## Self-Review (ran before save)

- Spec coverage: reviewer's stated discipline verbatim (literal + window + control — T1 Steps 1-2 ← design "per the reviewer's stated method"), three-way consumer verdict + the one-hop transfer question (T1 Step 3 + `### Disposition` ← design "does any reader's value feed a CALL/JMP target — after two layers, or die in data"), armed-conditional phrasing (honesty rule + Step 1 ← design "conditional-armed" framing), one-layer guard (Global Constraints ← design), `[0x9b8]`/`[0x9bc]` cite-only adjacency (Global Constraints + Deferrals ← design), capped-writes-only-on-materialization with three-part trigger (Global Constraints + T2 Step 2 ← design "likely zero-write branch"), unmoved-proof form (T2 Step 2 ← slice-18 precedent).
- Placeholder scan: no TBD/TODO; every branch (four T1 verdicts, two T2 write paths) has named deliverables; window-sweep has explicit resolve-or-reject output requirement, not "check it".
- Type consistency: no C types; cell/store/chain addresses (`0x9ba`, `[0x9b8..0x9be]`, `6255 891eba09`, `626d bb2428`, `6252 8b5f02`, `6270/6277`, `0x29bc`, `452f/4536`, `2978..2ada`, `1000:4548..1000:46aa`, delta `0x1bd0`) match `## callee arg question`, `## 0x29bc slot consumers`, `## paging block 2978..2ada`, slice-8 `publish_mode_vector` rows; Task 2 consumes Task-1 tables by reference; commit messages distinct per task; Task 1 zero writes.

(End of file)
