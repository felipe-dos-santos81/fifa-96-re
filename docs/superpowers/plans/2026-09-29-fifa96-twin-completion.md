# FIFA96 Twin Completion Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close slice 15's two live pins on `FUN_11bd_02b7`: (a) inventory every static contact with cells `[0xf52]`/`[0xf54]` so the withheld name's missing leg is either filled (rename) or re-declared on harder evidence, and (b) repair the `02d4..02d9` decode-pocket with the byte-authority `ADD`/`ADD` parse so the function body re-joins its orphaned tail `02da..02f8` — or ratify truncation with a stated reason.

**Architecture:** Read-only evidence pass first (cell inventory + neighborhood scan + pocket re-derivation, zero Ghidra writes — Task 1), then writes (pocket `disassemble_bytes` real at the 6 bytes + body re-flow / rename / plate updates + map append — Task 2). Rows appended to `docs/ghidra/loader_rename_map.md` under `## 02b7 twin completion`; slice-15 rows are referenced, never rewritten. No C, no tests, no CMake.

**Tech Stack:** Ghidra-MCP (`search_instructions`, `get_xrefs_to`, `get_function_by_address`, `disassemble_bytes` (Task 1 `dry_run=true` only), `disassemble_function`, `read_memory`, `list_data_items`/`audit_global` for cell state, `rename_function`, `set_comment`, `create_function`, `save_program`), `grep` (content checks), CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (cell roles inventory + pocket repair, rename-if-pinned, ratify-if-analyzer-still-refuses, `2978..2ada` out, one-layer guard) + `docs/ghidra/loader_rename_map.md` (`## 02b7 twin` — Task-1 walk table with both parses of `02d4..02d9`, Task-2 `### Writes` pocket/orphan disclosure + deferral (e), verdict row "NOT-CONFIRMED-at-name, missing leg = cell roles"; `## 0290/0293 fall-through` sibling cite-only).

## Global Constraints

- No C, test, or CMake changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; where they conflict, cite the disassembly and say the decompiler differed.
- No `decode_*` name without an observed byte transform; NOT-CONFIRMED (at-name) keeps no rename — the missing leg must be named on the evidence, not lore.
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only with `dry_run=true`; if it errors or yields no instruction text, report that sub-step BLOCKED rather than writing. All listing mutations (real `disassemble_bytes` on `02d4..02d9`, any `create_function` re-flow, `rename_function`, `set_comment`) belong to Task 2 with before-state recorded.
- Pocket repair is confined to the disclosed bytes `02d4..02d9` (raw `0336540f8306` per slice-15 map) + the analyzer's re-flow of the twin's own range; do not touch `02f9+`, the `2978..2ada` block, or any other function's body. If the analyzer still refuses to join the tail after the pocket is filled, RATIFY the truncation (record the tool's parse, body bounds, and the ratification reason in the map) — no-fight rule stands.
- The rename decision binds to the cell inventory: rename ONLY if the inventory shows what `[0xf52]`/`[0xf54]` ARE (writers/readers pattern with cited addresses); if the inventory is thin (twin-only contacts), the name leg stays NOT-CONFIRMED and the map records the harder evidence needed. No invented direction words ("enter"/"exit"), no caller-lore naming.
- Prior sections are read-only: `## 02b7 twin` rows (verdict, Writes, deferral (e)) are referenced by address/quote, never rewritten; the completion section states what changed post-repair and points back.
- Ghidra work targets project `fifa96`, program `/fifa96.exe`, with `save_program` after each write batch. Do NOT invent addresses — resolve everything live and record actual responses.
- Operand searches see DEFINED instructions only; every negative result must enumerate the searches run and carry the undefined-holes caveat (slice-14 M1 lesson).

---

## Scope Check

Two closely-coupled questions about one function (cell roles → name; pocket → body) in two gates: read-only inventory (Task 1), writes + dispositions (Task 2). Deliverable per task: map rows + suite green. `[0x40]`/`[0x9c0]` runtime installers and the `2978..2ada` block are named non-goals.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## 02b7 twin completion` section (Task 1 inventory + pocket tables; Task 2 disposition rows + rename/plate record + ratify-or-repair outcome).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: real `disassemble_bytes` at `02d4..02d9`, body re-flow if it follows, `rename_function` + `set_comment` if inventory pins the roles, `save_program`.
- `src/`, `tests/`, `CMakeLists.txt`, headers, `tools/` — UNTOUCHED.

---

### Task 1: Cell inventory + pocket re-derivation (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## 02b7 twin completion` with inventory table + pocket table)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: `## 02b7 twin` rows — the twin's own contacts (`ADD SI,[0xf54]`@~02d4 and `ADD [0xf52],0x8`@~02d8 under the adopted parse; tool's alternative `PUSH DX`/`INVD` recorded; pocket raw `0336540f8306`), verdict row (missing leg = cell roles), deferral (e) (pocket + orphan tail).
- Produces: inventory table (`| cell | search run | hits (addr, mnemonic) | role reading |`) — for `[0xf52]` AND `[0xf54]` AND neighborhood `0xf50`/`0xf56`/`0xf58`: `search_instructions` operand patterns (both `0xf52` and `f52`/word variants that the tool accepts), `get_xrefs_to` on the data addresses, `audit_global` current state of each cell; every hit classified read vs write with cited instruction; per the caveat constraint each negative lists the exact searches that ran. Pocket table: re-confirm raw bytes `02d4..02d9` via `read_memory` against the map's quoted string; re-emit the tool dry-run `disassemble_bytes` window `02d4..02d9` (what it emits: nothing? mis-parse? — quote actual output) and the two candidate parses (tool `PUSH DX`/`INVD` vs adopted `ADD`/`ADD`) with byte-by-byte decode (opcode bytes + ModRM fields spelled out per the slice-15 decode).

- [ ] **Step 1: Inventory the cells**

For each of `11bd:0f50`, `0f52`, `0f54`, `0f56`, `0f58` (and note the pair structure `[0xf52]`+`[0xf54]` — 32-bit descriptor-ish pair?): `search_instructions` (operand patterns `0xf52`, `0xf54`, and neighbor forms; record every hit's address + full instruction text + caller function), `get_xrefs_to` (quote count), `audit_global` (name/type state). Classify every hit read/write/both with cited instruction. The neighborhood `0xf4x/0xf5x/0xf6x` operand space: if other cells nearby surface in the same instruction families (e.g. the twin's `AND SI,0x38`@02d1 masking), cite them as context, do not widen scope.

- [ ] **Step 2: Pocket re-derivation**

`read_memory` at `11bd:02d4` length 6 — quote bytes and diff against the slice-15 map's `0336540f8306`. `disassemble_bytes` DRY_RUN at `02d4` length 6 + a small window either side (to show the seam) — quote the tool's actual emit (or emptiness). Record both parses with field-level decode (`03 /r` ModRM `36` = mod00 rm110 disp16 `0f54` → `ADD SI,[0xf54]`; `83 /0 ib` ModRM `06` disp16 `0f52` imm8 `08` → `ADD [0xf52],0x8`) and the tool's `PUSH DX` (`52`) at `02da` / `INVD` (`0f08`) at `02db` seam — where exactly the two parses agree and diverge (slice 15 said they converge at `02dd`).

- [ ] **Step 3: Append the map section + verify + commit**

Append `## 02b7 twin completion (verified 2026-09-29, program \`/fifa96.exe\`)` — one-paragraph summary (cell-roles: PINNED to <what> / THIN (twin-only); pocket: tool parse unchanged / changed) + the two tables (filled Evidence, searches enumerated, caveats stated). No rename column, no writes. Run: `cmake -S . -B build && cmake --build build && ctest --test-dir build` — expect 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: inventory 0xf52/0xf54 cell roles and re-derive 02b7 pocket" || true`

---

### Task 2: Pocket repair + name disposition + write-batch

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Pocket repair` + `### Name disposition` + post-state rows to the completion section)
- Modify: Ghidra program `/fifa96.exe` (real `disassemble_bytes` at `02d4..02d9`; `create_function`/body re-flow if the analyzer joins the tail; rename + plate if PINNED; `save_program`)

**Interfaces:**
- Consumes: Task-1 pocket table (byte confirmation + parse defense), inventory table (PINNED roles or THIN result), slice-15 `### Writes` (before-state to extend: post-repair state = the new record).
- Produces: `### Pocket repair` row set — the exact command run, tool output before/after, resulting body bounds + the disposition: JOINED (`body [02b7..02f8]` tail included) or RATIFIED-TRUNCATION (analyzer still refuses: record its actual output + the sentence why the map keeps the tail orphaned). `### Name disposition` row — PINNED: rename (`FUN_11bd_02b7`→<mechanism name>) + new/updated plate, missing leg closed, verdict updated by reference; THIN: name stays withheld, map records the harder evidence now required (runtime cell writer, `2978+` block disassembly, descriptor-table analysis — the completion of deferral (d)). Post-write re-confirmations quoted: `get_function_by_address` (bounds + name), `get_comment` (plate read-back), `find_code_gaps`/orphan status where applicable. Suite + grep checks green.

- [ ] **Step 1: Pocket repair (writes)**

`disassemble_bytes` REAL at `11bd:02d4` length 6 (exactly the pocket). Re-read `get_function_by_address(11bd:02b7)`: if the analyzer re-flowed the body to include `02d4..02f8` → bounds `02b7..02f8` (or tool's nearest) = JOINED; if not, one sanctioned nudge: `create_function` at `02b7` (disassemble_first=false) and re-read bounds. Second refusal → RATIFIED-TRUNCATION: keep tool state as-is (record it verbatim; delete nothing you created by nudge beyond the function object's own bounds). Cap at the brief's two sanctioned paths; do not invent a third.

- [ ] **Step 2: Name disposition (writes)**

Only if Task-1 inventory PINNED roles: choose a mechanism-level verb-led snake_case name from the roles + the body's own operations (cells touched, hook `CALL [0x9c0]`@02b8, selector staging, set-only `OR [0x40]`, tail `JMP`-fall to `POPA/RET`); `rename_function` + update plate to `C: none — behavioral (<role>)`; record both. THIN: skip writes for name; state the now-required harder evidence list (one line each, cited to what Task 1 could NOT find).

- [ ] **Step 3: Append + verify + commit**

`save_program(program=/fifa96.exe)`; append `### Pocket repair` + `### Name disposition` + post-state rows. Run suite (expect 10/10) + `grep -c "02b7 twin completion"` nonzero. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: repair 02b7 pocket and settle twin name disposition" || true`

---

## Self-Review (ran before save)

- Spec coverage: cell inventory incl. neighborhood + enumerated searches + caveats (T1 Step 1 ← design item 1), pocket re-derivation with both parses at field level (T1 Step 2 ← design item 2 pre-work), repair-or-ratify bounded to two sanctioned paths with verbatim outputs (T2 Step 1 ← design "re-flow or ratify, no-fight stands"), rename-only-if-PINNED with explicit no-lore/no-direction rules (T2 Step 2 ← design "rename if pinned"), prior-rows-untouched + by-reference updates (Global Constraints), `2978..2ada` + `[0x40]`/`[0x9c0]` runtime out (non-goals).
- Placeholder scan: no TBD/TODO; PINNED/THIN and JOINED/RATIFIED are both fully specified with different dispositions each; name examples are pattern-guidance ("e.g. descriptor-ish" only appears as a question in T1 interfaces, not a claimed role).
- Type consistency: no C types; addresses (`0xf52`/`0xf54`/neighborhood, `02d4..02d9`, raw `0336540f8306`, tail `02da..02f8`, `02b7`, `02b8`, `02c6`, `02dd`, `0x9c0`, `0x40`) all match `## 02b7 twin` rows; Task 2 consumes Task-1 tables by reference; commit messages distinct; Task 1 zero writes.

(End of file)
