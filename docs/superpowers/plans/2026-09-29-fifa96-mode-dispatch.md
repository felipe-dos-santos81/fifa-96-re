# FIFA96 3ed8 Mode Dispatch Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Delimit all ~20 arms of the `3ed8` SI mode-word dispatch (`11bd:42de..4368`) — per-arm condition, target, and published constant — with instruction evidence and map rows.

**Architecture:** Read-only Ghidra evidence pass in two gates: arm-table skeleton first (chain walk, no dives), then one-layer callee verdicts for arms that stay in scope. Rows appended to `docs/ghidra/loader_rename_map.md` under `## 3ed8 mode dispatch`. No C, no tests, no CMake.

**Tech Stack:** Ghidra-MCP (`disassemble_function`, `decompile_function`, `get_function_by_address`, `get_function_callees`, `rename_function`, `set_comment`, `save_program`), `grep` (content checks), CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (all arms one pass + one-FUN-layer guard per arm + skeleton-plus closeout) + `docs/ghidra/loader_rename_map.md` (`### SI-producer boundary: 11bd:3f9c` derivation facts, `### 3ed8 verdict: NEEDS-OWN-SLICE` exit facts).

## Global Constraints

- No C, test, or CMake changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/`xxd`/golden citation; disassembly wins over prior descriptions.
- No `decode_*` name without an observed byte transform; NOT-CONFIRMED members get no rename and no map row.
- Scope guard (per arm): a callee dive may descend at most one FUN layer past the arm body; deeper fan-out is recorded (address + call-site) with the dive deferred. If more than a third of arms hit the guard, the slice still closes with the arm table complete — a skeleton-plus, never a balloon.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each rename batch. Do NOT invent FUN addresses — resolve every body via `get_function_by_address` and record actual bounds.

---

## Scope Check

One subsystem (mode dispatch) in two gates: arm-table skeleton with zero dives (Task 1), one-layer callee verdicts + renames + closeout (Task 2). Each task ends with an independently verifiable deliverable (map rows + suite green). The consumers of the published values downstream of `4368` are a named non-goal (follow-up slice).

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## 3ed8 mode dispatch` section (Task 1 arm table, Task 2 callee rows + guard-deferred notes).
- Modify: Ghidra program `/fifa96.exe` — evidence-based renames + plate comments, CONFIRMED only (Task 2 only; Task 1 renames nothing).
- `src/`, `tests/`, `CMakeLists.txt`, headers — UNTOUCHED.

---

### Task 1: Arm-table skeleton (chain walk, no dives)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## 3ed8 mode dispatch` section with arm table)
- Modify: none in Ghidra (no renames, no plates, no save — read-only pass)

**Interfaces:**
- Consumes: entry anchor `MOV AX,SI` at `11bd:42db`; chain shape `DEC AX`/`JNZ` + `SUB AX,0x3/0x10a/0x64/0x171a/0xdad` at `11bd:42de..4368` (slice-5/6 verdicts); known publish targets `[BP-0x5a]`/`[0x10ee]`/`[0x2e]` + globals `0xeca`/`0xecc`/`0xece`/`0x11d4`/`0x46`/`0x47`; exit handoff toward the `451e..4586` tail (slice-5 exit facts).
- Produces: arm table with one row per arm (`| # | condition (value + instructions) | target address | publishes (target + constant + instructions) | calls (address only, no verdict) |`); count of arms recorded; guard-hit count (arms whose call list goes deeper than one layer — recorded, not pursued).

- [ ] **Step 1: Walk the chain arm by arm**

Run (project `fifa96`, program `/fifa96.exe`):
1. `disassemble_function` on `FUN_11bd_3ed8` (or targeted range reads) covering `11bd:42db..4368`; starting at `MOV AX,SI` (`42db`), follow the `DEC AX`/`JNZ` chain and each `SUB AX,<const>` fork. Number the arms in chain order (Arm 1, Arm 2, …).
2. Per arm record: the condition (exact constant + the `DEC`/`SUB`/`JNZ`/`JZ` instructions with addresses), the jump target address, every publish in the arm body (`MOV [target],<const>` with addresses — watch `[BP-0x5a]`, `[0x10ee]`, `[0x2e]`, and the known globals), and every `CALL` in the arm body (target address + call-site address only — NO decompile of callees, NO verdicts; that is Task 2).
3. Record the fall-through/default path (what happens when no arm matches) and the merge point where arms rejoin toward the `451e` tail.

Acceptance: every reachable arm from `42db` has a table row with filled condition + target + publishes; the arm count is stated; any address range in `42de..4368` not covered by a row is explicitly named as NOT-COVERED with its bounds (no silent gaps).

- [ ] **Step 2: Append the map section**

Append to `docs/ghidra/loader_rename_map.md`:
```markdown
## 3ed8 mode dispatch (verified 2026-09-29, program `/fifa96.exe`)

<one paragraph: chain shape, arm count, entry (MOV AX,SI at 42db), default path, merge toward 451e tail>

| # | Condition | Target | Publishes | Calls (address only) |
|---|-----------|--------|-----------|----------------------|
| 1 | SUB AX,0x3 at <addr>; JNZ <addr> | 11bd:<tgt> | MOV [0x10ee],<const> at <addr> | CALL 0x1000:<x> at <addr> |
```
(One row per arm — only rows with filled Condition + Target survive review. No rename column: Task 1 renames nothing.)

- [ ] **Step 3: Verify suite regression (no changes expected beyond docs)**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 10/10 green (docs-only).

- [ ] **Step 4: Commit**

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: map 3ed8 mode-dispatch arm table with instruction evidence" || true
```

---

### Task 2: One-layer callee verdicts + renames + closeout

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append callee rows + guard-deferred notes to the `## 3ed8 mode dispatch` section)
- Modify: Ghidra program `/fifa96.exe` (rename + plate ONLY for CONFIRMED one-layer callees, then `save_program`)

**Interfaces:**
- Consumes: Task-1 arm table (arm numbers, call-site addresses, guard-hit list); the per-arm scope guard (one FUN layer max; deeper fan-out recorded-with-address, dive deferred).
- Produces: one callee row per in-scope call (`| arm # | call site | callee FUN + bounds | verdict + evidence | new_name or — |`); guard-deferred notes (`### Guard-deferred: <arm #> <address>` with what is known + why it exceeds one layer); `save_program` confirmation if any rename; suite green confirmation. Downstream consumers past `4368` are NOT touched.

- [ ] **Step 1: Verdict the in-scope callees (guarded)**

Run:
1. For each Task-1 arm NOT on the guard-hit list: resolve each called address via `get_function_by_address` (record actual bounds; unresolvable → NOT-CONFIRMED with resolver output, move on), `decompile_function`, `get_function_callees` (confirm leaf-or-one-layer; a callee that itself fans out moves its arm to the guard-deferred list — record, do not pursue).
2. Verdict each callee CONFIRMED (role with config-source/publish-style evidence as in slices 5–6) or NOT-CONFIRMED (what is missing).
3. Apply the closeout rule: if guard-deferred arms exceed one third of the Task-1 arm count, STOP diving and close skeleton-plus (table complete, dives deferred) — record the fraction explicitly.

- [ ] **Step 2: Write callee rows, guard notes, renames**

CONFIRMED one-layer callees: append row + `rename_function` (verb-tier, no `decode_*`) + plate `C: none — behavioral (<role>)`, then `save_program(program=/fifa96.exe)`. NOT-CONFIRMED: no rename, no row. Guard-deferred arms: append `### Guard-deferred: arm <N> (<address>)` with call-site, known facts, and the one-layer excess reason (no rename).

- [ ] **Step 3: Verify suite + content and commit**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 10/10 green. Verify: `grep -c "3ed8 mode dispatch" docs/ghidra/loader_rename_map.md` is nonzero.

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: verdict dispatch-arm callees and close mode-dispatch regression" || true
```

---

## Self-Review (ran before save)

- Spec coverage: all-arms table with no dives (Task 1 ← design "arm table, cited, no callee dives"), one-layer verdicts + renames + guard closeout (Task 2 ← design "scope guard" + "skeleton-plus"), downstream consumers named non-goal (Scope Check + Task-2 interfaces).
- Placeholder scan: no TBD/TODO; Task-1 NOT-COVERED rule stated for silent-gap prevention; Task-2 closeout fraction rule (`>1/3` → skeleton-plus) explicit; unresolvable-callee path specified; row-survival rules stated for both tables.
- Type consistency: no C types; anchor addresses (`42db`, `42de..4368`, `451e`) match slice-5/6 verdicts; publish targets match the committed verdict lists; Task 2 consumes Task-1 table + guard-hit list; commit messages distinct per task; Task 1 correctly specifies zero Ghidra writes.

(End of file)
