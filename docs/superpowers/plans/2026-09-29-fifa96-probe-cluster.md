# FIFA96 3ed8 Probe Cluster Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Delimit the `3ed8` config-probe cluster (four probe callees + SI mode-word producer) with instruction evidence and map rows.

**Architecture:** Read-only Ghidra evidence pass (resolve body → decompile → callees → rename CONFIRMED + plate + save), rows appended to `docs/ghidra/loader_rename_map.md` under `## 3ed8 probe cluster`. The `42de..4368` mode-word dispatch arms are explicitly out of scope (follow-up slice). No C, no tests, no CMake.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `decompile_function`, `get_function_callees`, `rename_function`, `set_comment`, `save_program`), `grep` (content checks), CTest (regression gate only).

**Spec:** `docs/ghidra/loader_rename_map.md`, `### 3ed8 verdict: NEEDS-OWN-SLICE` (entry/exit/38-callee facts) + approved chat design 2026-09-29 (probe cluster first, dispatch later; one-FUN-layer scope guard on the SI producer).

## Global Constraints

- No C, test, or CMake changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/`xxd`/golden citation; disassembly wins over prior descriptions.
- No `decode_*` name without an observed byte transform; NOT-CONFIRMED members get no rename and no map row.
- Scope guard: the SI-producer hunt may descend at most one additional FUN layer past the four probes; if the producer is deeper, name the boundary and stop — remainder defers to the dispatch slice.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each rename batch. Do NOT invent FUN addresses — resolve every body via `get_function_by_address` and record actual bounds.

---

## Scope Check

One subsystem (probe cluster) in two gates: four probe callees (Task 1), SI mode-word producer + closeout (Task 2). Each task ends with an independently verifiable deliverable (map rows + suite green). No split needed. The mode-word dispatch arms (`11bd:42de..4368`) are a named non-goal for the follow-up slice.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## 3ed8 probe cluster` section (Task 1 probe rows, Task 2 producer row + boundary note).
- Modify: Ghidra program `/fifa96.exe` — evidence-based renames + plate comments, CONFIRMED only (Tasks 1–2).
- `src/`, `tests/`, `CMakeLists.txt`, headers — UNTOUCHED.

---

### Task 1: Four probe callees + renames + map rows

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## 3ed8 probe cluster` section)
- Modify: Ghidra program `/fifa96.exe` (renames + plate comments, CONFIRMED only, then `save_program`)

**Interfaces:**
- Consumes: Task-2 (slice-5) verdict facts — probe loops at `3f66..41a1` calling `4665`/`4645`/`6120`/`45c3`/`195d`; BIOS equipment-word probe at `11bd:3f1b` (`MOV SI,word ptr ES:[BX]`, ES=0xF000, BX=0xFFFE; `CMP SI,0xfb` at `3f22` / `0xfd` at `3f28`); status staging `[0x11d4]` at `3f0d`/`3f3d`. All four probes live in segment `11bd` — resolve bodies via `get_function_by_address` on `11bd:4665`, `11bd:4645`, `11bd:6120`, `11bd:45c3` and record actual bounds (do NOT assume bounds).
- Produces: one map row per CONFIRMED probe (`| FUN | address | evidence | new_name | C counterpart |`); plate comments; `save_program` confirmation.

- [ ] **Step 1: Resolve and decompile the four probes**

Run (project `fifa96`, program `/fifa96.exe`):
1. `get_function_by_address` on `11bd:4665`, `11bd:4645`, `11bd:6120`, `11bd:45c3`; record actual body bounds (if any offset does not resolve to a function body, mark that member NOT-CONFIRMED with the resolver output and move on — do not hunt for it).
2. `decompile_function` on each resolved body; record return contract and the 2–4 most role-indicative instructions with addresses (what config source does it read — BIOS area, port, DOS call, globals? what does it publish — `[BP-0x5a]`, `[0x10ee]`, `[0x2e]`, other?).
3. `get_function_callees` on each (confirm whether the probe is a leaf reader or itself fans out; a probe that fans out deeper than one layer is still in scope for its own role verdict, but its callees are NOT — record their addresses only).

Acceptance: per-probe verdict CONFIRMED (exact evidence: config source + publish target) or NOT-CONFIRMED (what is missing). Disassembly wins over the slice-5 verdict's characterizations.

- [ ] **Step 2: Rename + plate-comment CONFIRMED probes only, then save**

For each CONFIRMED probe: `rename_function` to an evidence-based name (verb-tier; `decode_*` forbidden), `set_comment(address, comment="C: none — behavioral (<role phrase>)", type=plate)`, then `save_program(program=/fifa96.exe)`. NOT-CONFIRMED: no rename, no row.

- [ ] **Step 3: Append the map section**

Append to `docs/ghidra/loader_rename_map.md`:
```markdown
## 3ed8 probe cluster (verified 2026-09-29, program `/fifa96.exe`)

<one paragraph: what the 3f66..41a1 probe loops feed and what each probe publishes, cited or OPEN>

| Ghidra FUN | Address | Evidence | New name | C counterpart |
|------------|---------|----------|----------|---------------|
| FUN_11bd_4665 | 11bd:4665 | <exact instructions> | <new_name> | none — behavioral (<role>) |
```
(One row per CONFIRMED probe — only rows with filled Evidence survive review.)

- [ ] **Step 4: Verify suite regression (no C changes expected)**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 10/10 green (docs/Ghidra only).

- [ ] **Step 5: Commit**

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: delimit 3ed8 config-probe callees with instruction evidence" || true
```

---

### Task 2: SI mode-word producer + regression closeout

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append producer row + boundary note to the `## 3ed8 probe cluster` section)
- Modify: Ghidra program `/fifa96.exe` (rename + plate ONLY if the producer role is CONFIRMED, then `save_program`)

**Interfaces:**
- Consumes: Task-1 map section; known anchor `MOV SI,word ptr ES:[BX]` at `11bd:3f1b` (ES=0xF000 segment override, BX=0xFFFE — BIOS equipment-list word at F000:FFFE); the loops at `3f66..41a1` that transform SI into the mode word dispatching at `42de..4368`.
- Produces: producer row (where the SI value comes from, what transforms it before the dispatch) OR a boundary note naming the layer where the trail goes deeper than one FUN; suite green confirmation. Dispatch arms themselves are NOT touched.

- [ ] **Step 1: Trace the SI mode-word producer (guarded effort)**

Run:
1. `decompile_function` on `FUN_11bd_3ed8` restricted to the `3f1b..41a1` window: starting from the `3f1b` SI load, which instructions derive the mode value (which probe outputs feed it, which globals/registers carry it toward `42de`)?
2. If the derivation leads into exactly one additional FUN layer (a helper called from this window that computes or publishes the mode value), resolve it via `get_function_by_address`, decompile, and verdict it (CONFIRMED/NOT-CONFIRMED) the same way as Task-1 probes.
3. Apply the scope guard: if the trail goes deeper than one additional FUN, STOP and write the boundary note instead (name the address where the trail goes deep + what is known so far). Do not sink the pass into the dispatch arms or deeper layers.

- [ ] **Step 2: Write the producer row or the boundary note**

CONFIRMED producer: append row `| FUN_11bd_<off> | 11bd:<off> | <source-to-dispatch evidence> | <new_name> | none — behavioral (<role>) |` + rename + plate + `save_program`. Boundary hit: append `### SI-producer boundary: <address>` with the derivation facts gathered and why it exceeds one layer (no rename).

- [ ] **Step 3: Verify suite + content and commit**

Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build`
Expected: PASS — 10/10 green. Verify: `grep -c "3ed8 probe cluster" docs/ghidra/loader_rename_map.md` is nonzero.

```bash
git add docs/ghidra/loader_rename_map.md
git commit -m "docs: trace SI mode-word producer and close probe-cluster regression" || true
```

---

## Self-Review (ran before save)

- Spec coverage: four probes (Task 1 ← design "four probe callees"), SI producer with one-layer guard (Task 2 ← design "scope guard"), renames/rows/regression (both ← standing slice conventions); dispatch arms named non-goal in Scope Check + Task-2 interfaces.
- Placeholder scan: no TBD/TODO; Task-2 conditional fully specified on both sides (CONFIRMED row+rename vs boundary note); unresolvable-offset path specified in Task-1 Step 1; row-survival rule stated.
- Type consistency: no C types; offsets (`4665`/`4645`/`6120`/`45c3`) match slice-5 verdict §"Why it exceeds a bound"; anchor addresses (`3f1b`, `3f22`, `3f28`, `3f66..41a1`, `42de..4368`) match the committed verdict; Task 2 consumes Task-1 section; commit messages distinct per task.

(End of file)
