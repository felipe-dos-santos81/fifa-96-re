# FIFA96 Paging Block Entry Attribution + First Functions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Attribute static entries into the unanalyzed block `11bd:2978..2ada` (the "paging block" — reachable only by fallthrough today, holding the MSW-clear shape at `2a6c..2a73`), walk the first attributed functions to cited exits, and verdict/create/rename what the evidence supports; hand every unattributed byte back as a named-open residue.

**Architecture:** Read-only evidence pass first (gap state + enumerated entry-attribution searches + walks of attributed entries, zero Ghidra writes — Task 1), then writes (real disassembly + `create_function` at cited boundaries + rename + plates + entry-attribution note — Task 2). Rows appended to `docs/ghidra/loader_rename_map.md` under `## paging block 2978..2ada`; prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`find_code_gaps`, `get_function_by_address`, `disassemble_bytes` (Task 1 `dry_run=true` only), `disassemble_function`, `search_instructions`, `search_byte_patterns`, `get_function_callees`, `create_function`, `rename_function`, `set_comment`, `save_program`), `grep` (content checks), CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (entry attribution + first functions; `28e7` skip and the `02da..02f8` twin orphan out; one-layer guard; honest zero-entry result possible) + `docs/ghidra/loader_rename_map.md` (`## 296d hook target`: `restore_fs_gs_and_resume` `296d..2977` created; `## 02b7 twin` + `## 02b7 twin completion`: cells `[0xf52]`/`[0xf54]` PINNED base/cursor, `[0x40]`/`[0x9c0]` installers runtime-open, MSW-clear shape `2a6c..2a73` flagged-not-adopted, gap splits `2811..296c` / `2978..2ada`; `[0x9c2]` vector: sole writer `41ee` value `0x296d`, gate `CMP byte [0x2f],0x3`@`41e7`/`JL 4257`).

## Global Constraints

- No C, test, CMake, or `tools/` changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; where they conflict, cite the disassembly and say the decompiler differed.
- No `decode_*` name without an observed byte transform; NOT-CONFIRMED gets no rename and no verdict row — missing piece named instead.
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only with `dry_run=true`; if it errors or yields no instruction text, report that sub-step BLOCKED rather than writing. All listing mutations (real `disassemble_bytes`, `create_function`, `rename_function`, `set_comment`) belong to Task 2 with before-state recorded.
- Created-function boundaries must be exactly Task-1-cited walks; the no-fight cap is inherited from slice 15/16: real disassembly → at most ONE `create_function` nudge (`disassemble_first=false`) → RATIFY on second refusal, with verbatim outputs; delete nothing beyond the function objects this slice's own nudge created inside the walked range.
- Scope guard: at most TWO first-attributed functions walked and created (address order); every further attributed entry is recorded with its site and deferred by name. `11bd:2811..296c` (the skip-pocket gap) and the twin orphan `02da..02f8` are OUT — not entered, not repaired, not cited as in-scope evidence beyond status lines.
- Entry attribution negatives must enumerate the searches run (patterns + hit counts) and carry the defined-instructions-only caveat (slice-16 lesson: `get_xrefs_to` is dead for these operand forms — `search_instructions` is the authority; far targets render as `0x1000:xxxx` with `0x1bd0` segment offset, near as `11bd:xxxx`).
- `[0x40]` contents are runtime (slice-15 direction UNDECIDED + `[0x9c0]` NOT-IN-EXE): no enter/exit/direction claim from any walk unless the body shows the bit arithmetic itself; names mechanism-level only.
- The `1991:` cross-segment caveat is inherited: same-runtime-cell identity for absolute operand pairs rests on DS/ES←0x20 staging, not static symbols — state it wherever a walk leans on cross-segment cells.
- Prior sections read-only (pure append, zero deletions); `save_program` after each write batch; do NOT invent addresses — resolve everything live.
- Leave `fifa96.rep` churn unstaged (map-only discipline).

---

## Scope Check

One discovery (entry attribution for a whole block) in two gates: read-only evidence (Task 1), writes + dispositions (Task 2). Deliverable per task: map rows + suite green. Remaining unattributed bytes and beyond-two-function fan-out are named non-goals — deliberately, the block is larger than any one slice may claim.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## paging block 2978..2ada` section (Task 1 gap + attribution + walk tables; Task 2 verdict rows + `### Entry attribution` conclusion + `### Writes` + deferral lines).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: real `disassemble_bytes` at cited entries, `create_function` (capped), `rename_function`, `set_comment`, `save_program`.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Gap state + entry attribution sweep + walks (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## paging block 2978..2ada` section with gap-state row set, attribution table, walk table)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: `## 296d hook target` (block's left neighbor `restore_fs_gs_and_resume` ends `2977`; original gap `2811..2ada` split), `## 02b7 twin completion` (MSW-clear `2a6c..2a73` flagged; `[0x40]`/`[0x9c0]` open; dead-xref lesson; far-thunk `0x1bd0` arithmetic).
- Produces: gap-state lines (current `find_code_gaps` row covering `2978..2ada` verbatim: start/end/size/neighbors); attribution table (`| search run | hits (addr, mnemonic → target) | verdict |`): every defined-insn `CALL`/`JMP` whose resolved target lands in `2978..2ada` (far `0x1000:` forms + near forms), pointer-table-style data hits (`search_byte_patterns` for LE pairs of plausible entry offsets — bounded list, e.g. the walked candidate entries' bytes), and any `[cell]`-indirect vector writers pointing into the range beyond `41ee` (re-check: expect none); walk table for each attributed entry (max 2, address order): `disassemble_bytes` DRY_RUN from entry to CITED exit with bytes, callees address-only, cells/globals touched classified. If the sweep finds ZERO attributed entries: that IS the deliverable — record the enumerated negative, no walks, exit.

- [ ] **Step 1: Gap state**

`find_code_gaps` navigated to the row(s) covering `11bd:2978..2ada`; quote verbatim (start/end/size/nearest neighbors both sides — `get_function_by_address` on the neighbor ends to confirm names, e.g. `restore_fs_gs_and_resume` end `2977`). Confirm `2978` and `2ada` boundaries against slice-14/16 rows.

- [ ] **Step 2: Attribution sweep**

`search_instructions` runs — enumerate every pattern tried with hit count: operand patterns `0x1000:29`, `0x1000:2a`, `11bd:29`, `11bd:2a`, `:296`, `:297`, `:298`, `:299`, `:29a`, `:29b`, `:29c`, `:29d`, `:29e`, `:29f`, `:2a0`, `:2a1`, `:2a2`, `:2a3`, `:2a4`, `:2a5`, `:2a6`, `:2a7`, `:2a8`, `:2a9`, `:2aa`, `:2ab`, `:2ac`, `:2ad` (or the tool's closest supported forms — record what was ACTUALLY run). For every hit: resolved target address + segment math re-computed (`0x1000:xxxx − 0x1bd0 = 11bd:xxxx`) + in-range or rejected with reason. Separately re-check vector cells: `search_instructions` operand `[0x9c2]` writer forms + any absolute-cell store whose immediate lands in `2978..2ada` (expect only `41ee`→`0x296d`, confirm). Data hits: `search_byte_patterns` for LE pairs targeting the two candidate entries you will walk (bytes of `2978`, `2a6c`, `2a30`... — record which offsets probed) across the whole image.

- [ ] **Step 3: Walk attributed entries**

For up to TWO attributed entry addresses (address order; MSW-clear `2a6c`'s owning entry included if it surfaces): `disassemble_bytes` at `11bd:<entry>` with `dry_run=true`, extending windows in address order to the CITED exit (`RET`/tail-`JMP`/`JMP [cell]`/`IRET`) with bytes; if flow runs INTO `2811..296c` or past `2ada`, stop and cite the boundary byte. Walk table rows: entry, every control-transfer, cells/globals touched (classify read/write; `[0x40]`/`[0x9c0]`/`[0x9c2]`/`[0xd60]`/`[0xd62]`/`[0xf52]`/`[0xf54]` familiar — note re-encounters), callees address-only. If zero attributed entries: skip walks; the negative stands.

- [ ] **Step 4: Append + verify + commit**

Append `## paging block 2978..2ada (verified 2026-09-29, program \`/fifa96.exe\`)` — paragraph + gap-state lines + attribution table (enumerated searches, caveats) + walk table(s) (or `No attributed entries` disposition line). Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build` — expect 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: attribute paging-block 2978..2ada entries and walk first two functions" || true`

---

### Task 2: Verdicts + create/rename/plate + attribution note

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Verdicts` + `### Entry attribution` conclusion + `### Writes` + deferral lines)
- Modify: Ghidra program `/fifa96.exe` (real disassembly at cited entries → `create_function` (capped) → rename + plates for CONFIRMED → `save_program`)

**Interfaces:**
- Consumes: Task-1 attribution table + walk tables + gap state; slice-16 disposition rules verbatim.
- Produces: verdict row per walked function (CONFIRMED → verb-led mechanism name + plate `C: none — behavioral (<role>)`; NOT-CONFIRMED → create-only per the legal middle, missing leg named, no rename); `### Entry attribution` conclusion (entries found: sites + targets cited / zero-entry honest negative — the block remains fallthrough-only); `### Writes` (before-state: gap row verbatim + no-function responses; every command + verbatim tool output; post-state: bounds + name + plate read-backs; cap compliance); deferral lines (unwalked attributed entries by name/address, unattributed ranges within the block, `[0x40]`/`[0x9c0]` runtime legs unchanged, out-of-scope `2811..296c` + twin orphan); suite + grep green.

- [ ] **Step 1: Verdict**

Per walked function: role from the walk between cited entry and cited exit, mechanism-level (no direction words unless the body itself shows bit arithmetic; `[0x40]`-mask users named by the op set/clear, nothing more); exit cited with bytes; callees leaf-or-guard-deferred (one-layer rule). Any leg missing → NOT-CONFIRMED-at-name (create allowed, no rename).

- [ ] **Step 2: Writes**

For each function to create: `disassemble_bytes` REAL at the cited entry/range → `create_function` at the entry (disassemble_first=false nudge path only as needed) → post-check bounds = cited walk; apply the inherited cap + ratify rule; never touch `2811..296c`, `02da..02f8`, or any pre-existing function body. Renames/plates for CONFIRMED. Record before/after states + `save_program` response verbatim.

- [ ] **Step 3: Append + verify + commit**

Append the three note blocks + deferral lines. Run suite (expect 10/10) + `grep -c "paging block 2978" docs/ghidra/loader_rename_map.md` nonzero. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: create and verdict paging-block functions, settle attribution" || true`

---

## Self-Review (ran before save)

- Spec coverage: current-state gap proof (T1 Step 1 ← design "gap state"), enumerated multi-form attribution sweep incl. far/near/thunk arithmetic + vector re-check + byte-pattern data hits (T1 Step 2 ← design "every defined CALL/JMP... thunk-form... near-form... data-operand hits"), max-two first functions walked to cited exits (T1 Step 3 + T2 ← design "1-2 functions"), honest zero-entry result branch (T1 Step 2/4 + T2 `### Entry attribution` ← design "including an honest zero-entry result"), unattributed remainder handed back named (T2 deferral ← design), out-of-scope `28e7`/`02da..02f8` guards (Global Constraints + T2 Step 2).
- Placeholder scan: no TBD/TODO; zero-entries, create-only, ratify branches all have dispositions; the pattern list is a floor, the "record what was ACTUALLY run" clause makes negatives checkable.
- Type consistency: no C types; addresses (`2978..2ada`, `296d..2977`, `2a6c..2a73`, `2811..296c`, `02da..02f8`, `41ee`, `0x9c2`, cells `[0x40]/[0x9c0]/[0x9c2]/[0xd60]/[0xd62]/[0xf52]/[0xf54]`, far delta `0x1bd0`) match `## 296d hook target`, `## 02b7 twin`, `## 02b7 twin completion` rows; Task 2 consumes Task-1 tables by reference; commit messages distinct; Task 1 zero writes.

(End of file)
