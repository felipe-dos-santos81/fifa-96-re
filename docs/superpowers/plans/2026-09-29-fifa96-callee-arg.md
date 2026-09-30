# FIFA96 Callee Arg Question Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Answer the one question slice 18 formally carried one layer out: do the callees `11bd:1e9f` and `11bd:6250` (`publish_mode_vector`) indirect-branch on the `0x29bc` arg value they receive from the slot stores — producing a three-way disposition (NONE-INDIRECT / CELL-STORAGE / INDIRECT-ON-ARG) with every hop cited, and at most ONE capped write batch if an entry mechanism materializes.

**Architecture:** Read-only trace pass (dumps + indirect enumeration + arg-propagation one hop, zero Ghidra writes, Task 1), then disposition + capped writes only if INDIRECT-ON-ARG (Task 2). Rows appended to `docs/ghidra/loader_rename_map.md` under `## callee arg question`; prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true` only), `decompile_function`, `analyze_dataflow` (optional accelerator; disassembly is the authority it must agree with), `search_instructions`, `read_memory`, `get_xrefs_to` (control only), `create_function`, `rename_function`, `set_comment`, `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (callee trace for the arg-indirect question only — NOT full verdicts of `1e9f`/`6250`; three-way disposition; one-hop propagation; cell leads named-and-deferred; capped write path) + `docs/ghidra/loader_rename_map.md` (`## 0x29bc slot consumers` — `452f PUSH word ptr [BP+-0x5a]` → `4536 CALL 0x1000:7e20` → `6250` with the slot value; `1e9f` called from `2ec9` on the slot value + `[BP+0x4]` per `2f40`-family cites; zero indirects in the two CALLERS established; callee-condition clause is THIS slice's assignment; `## paging block 2978..2ada` — block `2978..2ada` zero static entries, `0x29bc` inside it; slice-8 — `publish_mode_vector@6250` publishes `[0x9bc]` mode vector).

## Global Constraints

- No C, test, CMake, or `tools/` changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler and over `analyze_dataflow` — any accelerator output must be reconciled with cited instructions; every `read_memory` quote reconciles the response's `hex` field against its own `data` array before being called verbatim (slice-17 protocol).
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only with `dry_run=true`; if it errors or yields no instruction text, report that sub-step BLOCKED rather than writing. All listing mutations belong to Task 2 and ONLY under INDIRECT-ON-ARG, with the inherited cap: real disassembly → at most ONE `create_function` nudge (`disassemble_first=false`) → RATIFY on second refusal; delete nothing beyond objects this slice's own nudge created.
- Do NOT invent addresses: resolve `1e9f`'s body and confirm `6250`'s body/name live at task start; far-target render delta `0x1bd0` recomputed per hit; `get_xrefs_to` is DEAD for absolute-operand forms — `search_instructions` is the authority, xrefs control-only.
- Scope guard: this slice traces `1e9f`/`6250` for the ARG-INDIRECT question only — full verdicts of either function are OUT (name/role stays as previously recorded); if the arg is stored into a cell, that cell is a named-and-deferred lead with one cited propagation hop — no further dive, no verdict on the cell's consumers.
- Verdict honesty: "indirects on the arg" requires the cited chain (arg received at the callee's frame/entry → every store/load hop → an indirect `CALL`/`JMP` form whose operand derives from it without documented clobber); NONE-INDIRECT must show the COMPLETE indirect-form enumeration per function (zero `CALL`/`JMP` with reg-or-`[mem]` operands — the slice-18 reviewer's method: full dumps, every transfer operand a literal address); CELL-STORAGE must cite both the store site and the cell.
- Prior sections read-only (pure append, zero deletions); `save_program` after any write batch; leave `fifa96.rep` churn unstaged; no `decode_*`; NOT-CONFIRMED gets nothing; no direction words or caller-lore in any name unless bit-arithmetic is cited (slice-15 discipline).
- Slice-17 quote-protocol note in `## paging block 2978..2ada` is binding evidence for how read-backs are quoted — this slice's Writes/unmoved-proof rows follow its form.

---

## Scope Check

One question, two callees, three-way disposition, in two gates: read-only trace (Task 1), disposition + capped write (Task 2). Deliverable per task: map rows + suite green. Full verdicts of `1e9f`/`6250`, cell consumers, and the block body are named non-goals — deliberately, this closes the carried thread one layer out, not the callees' stories.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## callee arg question` section (Task 1 site-confirmation + enumeration + propagation + disposition-so-far tables; Task 2 `### Disposition` + optional `### Writes`/walk + deferral lines).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only, ONLY under INDIRECT-ON-ARG: real `disassemble_bytes` at the cited transfer site + `create_function` at the arg-derived target if a boundary materializes (capped) + rename/plate if CONFIRMED + `save_program`. NONE-INDIRECT / CELL-STORAGE execute the zero-write unmoved-proof form (slice-18 `### Writes` pattern).
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Callee dumps + indirect enumeration + arg propagation (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## callee arg question` with confirmation, enumeration, propagation tables + disposition-so-far line)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: slice-18 carries (`1e9f` arg: slot value + `[BP+0x4]` from `2ec9` per `2f40`-family; `6250` arg: `[BP+-0x5a]` slot value via `452f`→`4536`), slice-8 (`publish_mode_vector@6250` writes `[0x9bc]`), slice-17 protocol notes.
- Produces: site-confirmation lines (live `get_function_by_address` on `11bd:1e9f` and `11bd:6250` — quote names/bodies as found, do not assume; `29bc`/block still unmoved NOT required — this is a trace slice); per-function enumeration table (`| function | indirect CALL/JMP form | operand | value source |` — COMPLETE sweep of every `CALL`/`JMP`/`Jcc`/far form in each function: each transfer's operand rendered, classified literal-address vs reg/`[mem]` indirect, with bytes for indirect hits; a row stating zero indirect forms per function must rest on the complete dump, the map row quoting the dump's total-instruction count as the enumeration scope); propagation table per callee (`| arg arrival form | frame slot | every reference (store/load, addr+bytes+operand) | clobbers | terminal form |`) — the `0x29bc`-bearing value from entry (stack push / `[BP+0xN]` read) through each hop until it terminates in a branch form, a cell store, a literal discard, or the function exit — every hop cited; disposition-so-far line: NONE-INDIRECT / CELL-STORAGE (site+cell cited) / INDIRECT-ON-ARG (transfer site + operand chain cited) per function, plus the combined answer to the slice-18 condition (does EITHER callee indirect on the arg?). If INDIRECT-ON-ARG surfaces: dry-run walk ONE hop to where the target value's in-range (`2978..2ada` or otherwise) is determined — cite, no creation.

- [ ] **Step 1: Confirm sites live**

`get_function_by_address` on `11bd:1e9f`, `11bd:6250` (expect `publish_mode_vector` at 6250, `1e9f` as named or `FUN_11bd_1e9f` — record ACTUAL); quote both responses. Re-derive the two caller cites from slice 18 (one instruction read at `2f40`-neighborhood + at `4536`) to anchor the arg forms; hex-vs-data per protocol.

- [ ] **Step 2: Complete dumps + indirect enumeration**

`disassemble_function` on both callees (read-only). For EVERY transfer instruction (`CALL`/`JMP`/`Jcc`/far forms) in each dump: address, rendered operand, classification (literal `0x1000:`/near-literal vs reg/`[mem]` indirect), bytes for indirects; far deltas recomputed (`− 0x1bd0`). Map rows quote each dump's instruction-count as enumeration scope (the slice-18 reviewer's completeness method).

- [ ] **Step 3: Propagation**

Per callee: where does the pushed `0x29bc` value arrive (standard stack frame: arg slot `[BP+0x4]`-family — record actual displacements at ACTUAL addresses), list EVERY reference to those slots (bytes+operands), each register fed from them and its uses until redefinition, terminate in one of the four terminal forms above. `analyze_dataflow` optional; any use must be reconciled against cited disassembly instructions. If the value is stored to a memory cell (e.g. `[0x9bc]` or any other), record the store site + cell operand + bytes — CELL-STORAGE leg; do NOT follow consumers (guard: named-and-deferred).

- [ ] **Step 4: Disposition-so-far + append + verify + commit**

Combined answer line (does either callee indirect on the arg — the slice-18 condition resolved). Append `## callee arg question (verified 2026-09-29, program \`/fifa96.exe\`)` — paragraph + confirmation/enumeration/propagation tables + disposition-so-far. Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → expect 10/10 (docs-only). Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: enumerate callee indirects and trace 0x29bc arg propagation at 1e9f and 6250" || true`

---

### Task 2: Disposition + capped write path + deferrals

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Disposition` + `### Writes` (zero-write proof form OR capped writes) + deferral lines)
- Modify: Ghidra program `/fifa96.exe` — ONLY under INDIRECT-ON-ARG: real disassembly / `create_function` (capped) / rename+plate if CONFIRMED / `save_program`

**Interfaces:**
- Consumes: Task-1 confirmation/enumeration/propagation tables + disposition-so-far.
- Produces: `### Disposition` — the three-way outcome per callee restated with condition citations; whether the slice-18 carried condition is CLOSED (NONE-INDIRECT: "callees do NOT indirect on the arg — entry lead stays as slice-17/18 recorded, mechanism not found in either callee" — and then the arg-question is formally DONE within static scope, the dynamic-entry question belonging only to the runtime legs `[0x40]`/`[0x9c0]` installers and the still-named-open block) or CARRIED (CELL-STORAGE: the cell named, consumers deferred by name; INDIRECT-ON-ARG: mechanism cited, transfer site + target determination). `### Writes` — zero-write branch: unmoved-proof pair quoted verbatim (`get_function_by_address(11bd:29bc)` no-function + `find_code_gaps` covering row `1000:4548..1000:46aa` size 355 unchanged) in slice-17/18 form; capped branch: every command + verbatim tool response, post-read-backs. Deferral lines: cell consumers if stored-to (cell address named, one line); `1e9f`/`6250` full verdicts stay non-goals (one line each); the block `2978..2ada` stays fully named-open unless a branch target lands in it (then arithmetic: shrink language per slice-17's Concern-3 note pattern); runtime legs unchanged (one line); prior-slice lead statuses (all stand unless this slice closed the condition — state exactly which). Suite + `grep -c "callee arg question"` nonzero.

- [ ] **Step 1: Restate disposition**

Per callee + combined; the slice-18 condition answered with its carry sentence now discharged — cite the enumeration totals that prove it (e.g. `CALL`+`[`=0, `JMP`+`[`=0 across both complete dumps, with instruction counts as scope). No direction words, no full-verdict drift.

- [ ] **Step 2: Writes (branch on Task-1 outcome)**

NONE-INDIRECT / CELL-STORAGE: ZERO Ghidra writes; record the unmoved-proof pair verbatim. INDIRECT-ON-ARG only: create per cap {real disassembly → one nudge → ratify} at the arg-derived target IF it lands in `2978..2ada` or a code-shape at the cited site; rename/plate only if the walk gives CONFIRMED-with-legs; never touch `2811..296c`, twin region, existing FUNs.

- [ ] **Step 3: Append + verify + commit**

Append the blocks. Run suite (expect 10/10) + `grep -c "callee arg question"` nonzero. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: settle callee arg question disposition and carry named leads" || true`

---

## Self-Review (ran before save)

- Spec coverage: sites live-confirmed not assumed (T1 Step 1 ← design "read-only trace"; constraint "do NOT invent addresses"), COMPLETE indirect enumeration per callee with dump-scope quoting (T1 Step 2 + `### Disposition` restatement ← design "not their full verdicts — ONE question"), propagation with terminal forms + clobbers (T1 Step 3 ← design one-hop trace), CELL-STORAGE names cell + defers consumers (T1 Step 3 guard + T2 deferrals ← design "stored to a cell? then the cell is the new dynamic-entry lead — cited, next-layer deferral"), capped write ONLY under INDIRECT-ON-ARG with slice-17/18 forms (T2 Step 2 ← design), zero-write unmoved-proof branch (T2 Step 2 ← Task-1's interface + slice-17 protocol), slice-18 condition explicitly closed-or-carried (T1 Step 4 + T2 ← design item this slice exists for).
- Placeholder scan: no TBD/TODO; three branches (NONE/CELL/INDIRECT) each have named deliverables including the write path; enumeration-completeness acceptance criterion = dump totals + `CALL`+`[`/`JMP`+`[` sweeps (proven by slice 18's own reviewer method).
- Type consistency: no C types; addresses (`2978..2ada`, `29bc`, `1000:4548..1000:46aa`, `1e9f`, `6250`, `452f`/`4536`, `2f40`-family, `[BP+-0x5a]`, `[0x9bc]`, `0x1bd0`) match `## 0x29bc slot consumers`, `## paging block 2978..2ada`, and slice-8 `publish_mode_vector` rows; Task 2 consumes Task-1 tables by reference; commit messages distinct per task; Task 1 zero writes.

(End of file)
