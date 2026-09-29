# FIFA96 Boot Preamble Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Delimit the FIFA96 boot/entry preamble (six small helpers + entry setup + bounded `3ed8` role) with instruction evidence and map rows.

**Architecture:** Read-only Ghidra evidence pass (decompile → callees → rename CONFIRMED + plate + save), rows appended to `docs/ghidra/loader_rename_map.md` under `## Boot preamble`. `3ed8` gets a role bound with a scope guard, not a full pass. No C, no tests, no CMake.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `decompile_function`, `get_function_callees`, `rename_function`, `set_comment`, `save_program`), `grep` (content checks), CTest (regression gate only).

**Spec:** `docs/superpowers/specs/2026-09-28-fifa96-boot-preamble-design.md`

## Global Constraints

- No C, test, or CMake changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/`xxd`/golden citation; disassembly wins over prior descriptions.
- No `decode_*` name without an observed byte transform; NOT-CONFIRMED members get no rename and no map row.
- `3ed8` scope guard: role bound only; if bounding exceeds half the pass effort, record "needs own slice" and finish the rest.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each rename batch. Do NOT invent FUN addresses.

---

## Scope Check

Spec slice 5 covers one subsystem (boot preamble) in two gates: six small roles + entry header (Task 1), `3ed8` bound + regression closeout (Task 2). Each task ends with an independently verifiable deliverable (map rows + suite green). No split needed.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## Boot preamble` section (Task 1 rows, Task 2 `3ed8` bound row).
- Modify: Ghidra program `/fifa96.exe` — evidence-based renames + plate comments, CONFIRMED only (Tasks 1–2).
- `src/`, `tests/`, `CMakeLists.txt`, headers — UNTOUCHED.

---

### Task 1: Six small roles + entry header + renames + map rows

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## Boot preamble` section)
- Modify: Ghidra program `/fifa96.exe` (renames + plate comments, CONFIRMED only, then `save_program`)

**Interfaces:**
- Consumes: verified bodies — `FUN_11bd_2f7f` (`11bd:2f7f..11bd:2fa2`), `FUN_11bd_614a` (`11bd:614a..11bd:61a9`, returns `char *`), `FUN_11bd_6028` (`11bd:6028..11bd:6052`, returns `short`), `FUN_11bd_627f` (`11bd:627f..11bd:62f7`), `FUN_11bd_2d43` (`11bd:2d43..11bd:2d4f`), `FUN_11bd_6a2d` (`11bd:6a2d..11bd:6a67`), `entry@11bd:2382` (zero callers); `2d9c`-decompile leads (2f7f char loops, 614a NULL-gated prologue, 6028 second-dispatch gate, 627f tail, 2d43 on nonzero, 6a2d on nested path).
- Produces: one map row per CONFIRMED member (`| FUN | address | evidence | new_name | C counterpart |`); plate comments; `save_program` confirmation; entry-setup header paragraph (segments/stack/argv — cited or OPEN).

- [ ] **Step 1: Decompile the six small targets + entry header**

Run (project `fifa96`, program `/fifa96.exe`):
1. `decompile_function` on `FUN_11bd_2f7f`, `FUN_11bd_614a`, `FUN_11bd_6028`, `FUN_11bd_627f`, `FUN_11bd_2d43`, `FUN_11bd_6a2d`; record body bounds, return contract, and the 2–4 most role-indicative instructions per FUN with addresses.
2. `get_function_callees` on each (confirm the `2d9c`-decompile leads: 2f7f in char loops, 614a prologue position, 6028 gate position, 627f tail, 2d43/6a2d call sites).
3. `decompile_function` on `entry@11bd:2382` at header level: what it establishes before `CALL 2d9c` (segment loads, stack setup, argv/env pointer reads — cite instructions or mark OPEN; no full CRT RE).

Acceptance: per-FUN verdict CONFIRMED (exact evidence) or NOT-CONFIRMED (what is missing); entry paragraph either cites setup instructions or states OPEN. Disassembly wins over the spec's parenthesized leads.

- [ ] **Step 2: Rename + plate-comment CONFIRMED members only, then save**

For each CONFIRMED member: `rename_function` to an evidence-based name (verb-tier; `decode_*` forbidden), `set_comment(address, comment="C: none — behavioral (<role phrase>)", type=plate)`, then `save_program(program=/fifa96.exe)`. NOT-CONFIRMED: no rename, no row.

- [ ] **Step 3: Append the map section**

Append to `docs/ghidra/loader_rename_map.md`:
```markdown
## Boot preamble (verified 2026-09-28, program `/fifa96.exe`)

<entry-setup paragraph: what entry@11bd:2382 establishes before CALL 2d9c, cited or OPEN>

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_2f7f | 11bd:2f7f | <exact instructions> | <new_name> | none — behavioral (<role>) |
```
(One row per CONFIRMED member of the six — only rows with filled Evidence survive review.)

- [ ] **Step 4: Verify suite regression (no C changes expected)**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 10/10 green (docs/Ghidra only).

- [ ] **Step 5: Commit**

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: delimit boot preamble helpers with instruction evidence" || true
```

---

### Task 2: Bound 3ed8 role + regression closeout

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `3ed8` bound row + verdict to the `## Boot preamble` section)
- Modify: Ghidra program `/fifa96.exe` (rename + plate ONLY if `3ed8` role is CONFIRMED, then `save_program`)

**Interfaces:**
- Consumes: Task-1 map section; `FUN_11bd_3ed8` body `11bd:3ed8..11bd:4586` (~1700 bytes); known call context (called with two word args after the second-dispatch check in `2d9c`).
- Produces: `3ed8` bound row (entry conditions, exit shape, load-path relationship) OR a "needs own slice" verdict with gathered entry/exit facts; suite green confirmation. No other files touched.

- [ ] **Step 1: Bound the 3ed8 role (guarded effort)**

Run:
1. `decompile_function` on `FUN_11bd_3ed8`; read entry (first ~40 instructions: arg intake, initial branches) and exit (last ~30 instructions: return value, noreturn paths).
2. `get_function_callees` on `FUN_11bd_3ed8` (limit 100); record whether callees are known loader/script helpers (bounded, composable role) or a new unknown domain (slice-worthy).
3. Apply the scope guard: if entry+exit+callees yield a statable role (one paragraph), verdict BOUNDED and proceed; else verdict NEEDS-OWN-SLICE with the entry/exit/callee facts gathered, and STOP the investigation there (do not sink the pass into `3ed8` internals).

- [ ] **Step 2: Write the bound row or the slice verdict**

BOUNDED: append row `| FUN_11bd_3ed8 | 11bd:3ed8 | <entry/exit/callee evidence> | <new_name> | none — behavioral (<role>) |` + rename + plate + `save_program`. NEEDS-OWN-SLICE: append `### 3ed8 verdict: NEEDS-OWN-SLICE` with entry conditions, exit shape, callee summary, and why it exceeds a bound (no rename).

- [ ] **Step 3: Verify suite + content and commit**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 10/10 green. Verify: `grep -c "Boot preamble" docs/ghidra/loader_rename_map.md` is nonzero.

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: bound 3ed8 role and close preamble regression" || true
```

---

## Self-Review (ran before save)

- Spec coverage: six small roles + entry header (Task 1 ← spec §2.1/§2.3–2.4), `3ed8` bound with guard (Task 2 ← spec §2.2 + §6-guard), renames/rows/regression (both ← spec §2.4–2.5/§7).
- Placeholder scan: no TBD/TODO; `3ed8` conditional fully specified on both sides (BOUNDED row+rename vs NEEDS-OWN-SLICE verdict paragraph); row-survival rule stated.
- Type consistency: no C types; FUN addresses match spec §1 bodies; Task 2 consumes Task-1 section; commit messages distinct per task.

(End of file)
