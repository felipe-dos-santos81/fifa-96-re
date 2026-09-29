# FIFA96 Load-Path Delimitation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Delimit the FIFA96 load path in the EXE (roles of `5db2`/`5bdb`/`5c8b`/`5992`, driver-loop search) with instruction evidence, and refine the FU-3 breakpoint list for the future trace.

**Architecture:** Read-only Ghidra evidence pass (decompile → callers/callees → rename CONFIRMED + plate + save), map rows appended to `docs/ghidra/loader_rename_map.md`, FU-3 doc gains exact breakpoint targets. No C, no tests, no CMake, no emulator execution.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `decompile_function`, `get_function_callers`, `get_function_callees`, `search_instructions`, `rename_function`, `set_comment`, `save_program`), `grep` (content checks), CTest (regression gate only).

**Spec:** `docs/superpowers/specs/2026-09-28-fifa96-load-path-delimitation-design.md`

## Global Constraints

- No C, test, or CMake changes in this slice; the 9-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/`xxd`/golden citation; disassembly wins over prior descriptions.
- No `decode_*` name without an observed byte transform; no format attribution without a tag/envelope reference.
- NOT-CONFIRMED members get no rename and no map row; exhausted caller chains are reported as exhausted.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each rename batch.
- Do NOT invent FUN addresses.

---

## Scope Check

Spec slice 3 covers one subsystem (load-path delimitation) with four FUN targets plus one caller-chain search feeding one FU-3 update. One plan, two tasks. Task 1 (per-FUN roles + renames + map rows) is independently verifiable by file content. Task 2 (driver-loop search + FU-3 breakpoint list) consumes Task 1's roles and is verifiable by file content + suite regression. No split into sub-plans needed.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## Load-path delimitation` section (Task 1 rows) and driver-loop subsection (Task 2).
- Modify: `docs/ghidra/FU3_codec_runtime_capture.md` — append refined breakpoint list (Task 2).
- Modify: Ghidra program `/fifa96.exe` — evidence-based renames + plate comments, CONFIRMED only (Tasks 1–2).
- Modify: `include/fifa96_loader/fifa96_{pog,qfs,viv,tgv,tables}.h` — ONLY if format attribution changes (default: untouched; Task 2 decides with evidence).

---

### Task 1: Delimit 5db2 / 5bdb / 5c8b / 5992 with instruction evidence + renames + map rows

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## Load-path delimitation` section)
- Modify: Ghidra program `/fifa96.exe` (renames + plate comments, CONFIRMED only, then `save_program`)

**Interfaces:**
- Consumes: verified bodies — `FUN_11bd_5db2` (`11bd:5db2..11bd:5dd1`), `FUN_11bd_5bdb` (`11bd:5bdb..11bd:5c8a`), `FUN_11bd_5c8b` (`11bd:5c8b..11bd:5d78`), `FUN_11bd_5992` (`11bd:5992..11bd:5ad5`); slice-2 funnel rows (`load_mf_object@11bd:5dd2`, `mem_grow_relocate@1000:0b12` with its 7 callers incl. `11bd:5c8b`).
- Produces: one map row per CONFIRMED member (`| FUN | address | evidence | new_name | C counterpart |`); plate comments; `save_program` confirmation. `5db2`×3-inside-`5dd2` role statement (or NOT-CONFIRMED).

- [ ] **Step 1: Decompile the four targets and record exact evidence**

Run (project `fifa96`, program `/fifa96.exe`):
1. `decompile_function` on `FUN_11bd_5db2`, `FUN_11bd_5bdb`, `FUN_11bd_5c8b`, `FUN_11bd_5992`; record body bounds, instruction count, and the 2–4 most role-indicative instructions per FUN with addresses (calls, INT 21h, compares against magic bytes, loops).
2. `get_function_callees` on all four (expected leads: `5db2` called 3× inside `load_mf_object`; `5bdb`/`5c8b` shape of the else-branch; `5992` calls `file_read_dos` + `load_mf_object`).
3. `search_instructions` inside `FUN_11bd_5992` for the word-compare against `0x4d` and the `'M'/'F'` magic check; record addresses.

Acceptance: per-FUN verdict CONFIRMED (one-line exact evidence) or NOT-CONFIRMED (what is missing). Correct any spec description the disassembly contradicts — disassembly wins.

- [ ] **Step 2: Rename + plate-comment CONFIRMED members only, then save**

For each CONFIRMED member: `rename_function` to an evidence-based name (verb-tier; `decode_*` forbidden without an observed transform), `set_comment(address, comment="C: none — behavioral (<role phrase>)", type=plate)`, then `save_program(program=/fifa96.exe)`. NOT-CONFIRMED members: no rename, no row. Do NOT invent addresses.

- [ ] **Step 3: Append the map section**

Append to `docs/ghidra/loader_rename_map.md`:
```markdown
## Load-path delimitation (verified 2026-09-28, program `/fifa96.exe`)

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_5db2 | 11bd:5db2 | <exact instruction/call evidence> | <new_name> | none — behavioral (<role>) |
```
(One row per CONFIRMED member: `5db2`, `5bdb`, `5c8b`, `5992` — only rows with filled Evidence survive review.)

- [ ] **Step 4: Verify suite regression (no C changes expected)**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 9/9 green (this task touches docs/Ghidra only).

- [ ] **Step 5: Commit**

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: delimit load path (5db2/5bdb/5c8b/5992) with instruction evidence" || true
```

---

### Task 2: Driver-loop search + FU-3 breakpoint list (+ headers only if attribution changes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Driver loop` subsection to the delimitation section)
- Modify: `docs/ghidra/FU3_codec_runtime_capture.md` (append `## Refined trace targets` section)
- Modify: `include/fifa96_loader/fifa96_{pog,qfs,viv,tgv,tables}.h` — ONLY if a tag/envelope reference is observed (default: untouched; any edit is comment-only)
- Modify: Ghidra program `/fifa96.exe` (only if the caller walk surfaces a CONFIRMED new member: rename + plate + `save_program`)

**Interfaces:**
- Consumes: Task 1 roles + new names for `5db2`/`5bdb`/`5c8b`/`5992`; slice-2 funnel rows; FU-3 gaps (driver loop, `5db2` role, else-branch, next-tag rule).
- Produces: `### Driver loop` verdict (LOOP with the back-edge addresses, or EXHAUSTED with the terminal boundary); `## Refined trace targets` breakpoint list (address + what each break delimits); header edits only on observed attribution.

- [ ] **Step 1: Walk the caller chain upward from 5992 and 5dd2**

Run:
1. `get_function_callers` on `FUN_11bd_5992` and on `load_mf_object` (`11bd:5dd2`); record every caller with address.
2. For each caller: `decompile_function` + `get_function_callers` one level further, looking for a back-edge (re-invocation of `5992`/`5dd2` = LOOP) or a program-entry/exit boundary (EXHAUSTED).
3. `get_function_callees` on `FUN_11bd_5bdb` and `FUN_11bd_5c8b` to close the else-branch shape (Task 1 roles as context, not re-verdicts).

Acceptance: verdict LOOP (back-edge addresses cited) or EXHAUSTED (terminal function/boundary named with the reason no further ascent is meaningful). A loop may NOT be assumed from repetition of calls — only from a cited back-edge.

- [ ] **Step 2: Write the driver-loop verdict into the map**

Append to the `## Load-path delimitation` section:
```markdown
### Driver loop (verdict: LOOP | EXHAUSTED)

<LOOP: back-edge addresses + which invocation each delimits. | EXHAUSTED: ascent path walked, terminal boundary + why.>
```

- [ ] **Step 3: Append the refined trace-target list to FU-3**

Append to `docs/ghidra/FU3_codec_runtime_capture.md`:
```markdown
## Refined trace targets (2026-09-28 delimitation pass)

| Breakpoint | Delimits | Source |
|------------|----------|--------|
| <address> | <e.g. one load_mf_object invocation> | <map row / verdict> |
```
Rows for: `5992` word-compare branch, `load_mf_object` entry/return (`11bd:5dd2`/`11bd:5faa`), `5db2` role sites (if closed), else-branch entry (`5bdb`), driver back-edge (if LOOP), plus the slice-2 INT-21h read points (`11bd:5fe2`, `11bd:6003`) carried over by reference. Every row cites its source; rows without a source do not survive review.

- [ ] **Step 4: Headers only on observed attribution, then verify**

If and only if a tag (`SHPI`/`PCNX`/`BIGF`/`GIMX`/`kVGT`) or envelope field reference is observed in a delimited member: update that decoder header's `// Ghidra:` line to cite it, comment-only. Default (no observation): touch no header. Then run:

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 9/9 green. Verify: `grep -c "Refined trace targets" docs/ghidra/FU3_codec_runtime_capture.md` is nonzero.

- [ ] **Step 5: Commit**

```bash
git add docs/ghidra/loader_rename_map.md docs/ghidra/FU3_codec_runtime_capture.md include/fifa96_loader/fifa96_pog.h include/fifa96_loader/fifa96_qfs.h include/fifa96_loader/fifa96_viv.h include/fifa96_loader/fifa96_tgv.h include/fifa96_loader/fifa96_tables.h
git commit -m "docs: delimit driver loop and refine FU-3 trace targets" || true
```
(Headers in the add list that were untouched simply contribute nothing to the diff.)

---

## Self-Review (ran before save)

- Spec coverage: per-FUN decompile+rename+rows (Task 1 ← spec §2.1–2.3), driver-loop search + FU-3 breakpoint list (Task 2 ← spec §2.2/§2.4 + §6-trace-output), no-codec/no-CRC/no-emulator (both tasks ← spec §2.5/§7).
- Placeholder scan: no TBD/TODO; Task 2's header edits are conditional with an explicit default (untouched) and comment-only constraint; map-row survival rule (filled Evidence or no row) stated in both tasks.
- Type consistency: no C types introduced; FUN addresses match spec §1 bodies (`5db2..5dd1`, `5bdb..5c8a`, `5c8b..5d78`, `5992..5ad5`); Task 2 consumes Task 1's roles/names; FU-3 section names are exact (`## Refined trace targets`, `### Driver loop`).

(End of file)
