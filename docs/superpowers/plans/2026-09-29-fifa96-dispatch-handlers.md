# FIFA96 Vector Dispatch Handlers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Walk and disposition the unique landing offsets of the 26 band+block `JMP word ptr [0x9bc]/[0x9be]` dispatch targets from slice 21's determinability table — dedupe to a handler set, dry-run walk each handler from its cited target offset to a cited exit (the `PUSH/PUSH/MOV BX…/CLI` prelude pattern is the entry signature to verify per handler), then capped-create the handlers at cited boundaries with mechanism-level names only where the walk's own ops support them. This finally gives the indirect dispatch story owners for its landing code — including the first attributed entry points into the block `2978..2ada` (`0x2a5a`/`0x2a60`, adjacent to the slice-16 MSW-clear shape at `2a6c`).

**Architecture:** Read-only evidence pass (dedupe + walks + collision checks, zero Ghidra writes, Task 1), then capped creates + verdicts + map rows (Task 2). Append `## vector dispatch handlers` to `docs/ghidra/loader_rename_map.md`; prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_bytes` (Task 1 `dry_run=true` only), `disassemble_function`, `read_memory` (hex-vs-data protocol), `search_instructions`, `find_code_gaps`, `create_function`, `rename_function`, `set_comment`, `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (dedupe → walk each unique handler to cited exit → capped creates with mechanism-level names where ops support them; one-hop callee guard; handler trees, `[0x9b4]` cell role, `3ed8` arm semantics, islands `08c2/033c/0bc3` stay out/cite-only) + `docs/ghidra/loader_rename_map.md` (`## 2811..296c pocket + 9bc vector` — determinability table: 26 band+block targets, per-arg pair sources and cited landing insns (`0x0938` = `50 53 bb0010 fa`; `0x2a5a` = `50538b1eb409fa`-form); readers `092d`@`dispatch_mode_vector` (`092c..`), `0934`@`FUN_11bd_0931` (`0931..0937`, caller `0d80`); three flow islands ratified+deferred; `mode_vector_source_pair@2820`; `## paging block 2978..2ada` — block fully named-open, `2978` = `CMP byte [0xdfe],0x1`, MSW-clear shape `2a6c..2a73` flagged-not-adopted, quote-protocol note binding; `## 7c62 exit arm` — `092c` called at `7d15/7d1e`; slice-16 row for `2a6c`).

## Global Constraints

- No C, test, CMake, or `tools/` changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` against `data` before verbatim (slice-17 protocol, restated in `## paging block 2978..2ada`).
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only with `dry_run=true`; all reads via `disassemble_function`/`search_instructions`/`read_memory`/`get_function_by_address`/`find_code_gaps`; tool error on a required sub-step → BLOCKED report, never a write. All listing mutations belong to Task 2 at Task-1-cited boundaries.
- Do NOT invent addresses: every handler's start offset must trace to the determinability table row (source cell + cited LE word) — re-quote the row per handler; owner-state at each start re-confirmed live (`get_function_by_address`); far delta `0x1bd0` per far operand.
- Capped write path (inherited): real `disassemble_bytes` → at most ONE `create_function` nudge (`disassemble_first=false`) → RATIFY on second refusal, verbatim outputs; delete nothing beyond this slice's own nudge objects; **never create over defined bytes or into existing FUNs** — if a walk crosses an island (`08c2`/`033c`/`0bc3`), `2824`-family pocket FUN, `092c`/`0931` stub, or any defined body, the handler boundary STOPS one byte short, cited; overlaps that would force a merge → record collision + STOP-BLOCKED per handler, never fight.
- Scope guard: handler callees cited address-only, one hop; no walk into pocket CODE, block interior beyond a handler's own cited exit, `[0x9b4]` consumers, islands' bodies, or `3ed8` arm dispatch semantics — status lines cite prior rows.
- Naming honesty: verb-led snake_case from the handler body's own cited ops; mechanism wording only to the extent bytes show it (a `CLI`/segment-load/MOV-CR-class op names itself; "protected-mode"-flavored words allowed ONLY if a `MOV CR0`/LGDT/LIDT-class op is cited in-body or in the landing prelude — else record the missing leg and keep default names); NOT-CONFIRMED gets no rename/plate beyond the create; no `decode_*`, no direction words.
- Prior sections read-only (pure append, zero deletions outside new section); `save_program` after writes; leave `fifa96.rep` churn unstaged; commit messages exactly per the briefs; no new tests, suite 10/10 gate only.

---

## Scope Check

One deduped handler set, two gates: read-only dedupe+walks (Task 1), capped creates+dispositions (Task 2). Deliverable per task: map rows + suite green. Handler callee trees, `[0x9b4]`/island stories, arm-cascade semantics, block interior beyond cited exits — named non-goals.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## vector dispatch handlers` (Task 1 dedupe table + per-handler walk tables + prelude-signature rows; Task 2 `### Writes` + per-handler verdict rows + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: real disassembly + `create_function` (capped) at cited ranges; rename/plate only where legs cited; `save_program`.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Dedupe + per-handler walks (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## vector dispatch handlers` with dedupe table + walk tables + signature rows)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: determinability table rows (26 band+block targets with source cells/words); landing-insn cites already in the table (`0x0938` first bytes, `0x2a5a` first bytes); owner map rows for readers/callers; slice-16 `2a6c` flag; islands list.
- Produces: dedupe table (`| unique offset | arrived from (arg:source-cell words cited) | owner state at offset (get_function_by_address response) |`) — collapse the 26 targets to unique offsets (band + block); per-handler walk table (`| element | address | evidence (bytes) | calls (address only) |`) via dry-run windows from the cited offset to a CITED exit (`RET`/tail-`JMP`/`JMPF`/`HLT`/boundary stop at defined-byte edge), incl. the prelude-signature row per handler (`PUSH AX;PUSH BX;MOV BX,<base>[,[cell]];<CLI>`-family — exact forms cited, base/cell operands named) and every memory operand + segment store the handler touches (cited; `[0x9b4]`/`[0xdfe]`-style cells logged, not resolved); boundary proposal per handler `[start..exit-end]` + why function start (dynamic vector landing from the dedupe table = the entry cite; alignment vs gap rows); collision dispositions (walk crosses defined byte → stop-short cite).

- [ ] **Step 1: Dedupe the 26**

From `## 2811..296c pocket + 9bc vector` determinability rows: collect band+block target offsets with their arrival edges (arg → source → word). Produce the unique set (with multiplicities); `get_function_by_address` each unique offset — live owner state (expect mostly no-function; any that IS owned → island/known-FUN collision record).

- [ ] **Step 2: Walk each handler**

Dry-run `disassemble_bytes` per unique offset; extend windows until CITED exit or until crossing into defined bytes (islands/pocket FUNs/`092c`/`0931`-family edges) — cite the last owned byte and the first foreign byte; record prelude form, all ops with operands, segment/cell contacts; callees address-only. Keep a per-handler running boundary proposal.

- [ ] **Step 3: Signature + block-adjacency rows**

Per handler, state whether the prelude matches the `0x0938`/`0x2a5a` cited forms (diffs cited); for block handlers (`0x2a5a`/`0x2a60`-family), record distance/flow to the slice-16 `2a6c..2a73` MSW-clear shape (does either handler's walk REACH `2a6c`? cite bytes where flow passes or stops; the flagged shape stays flagged-not-adopted until this slice's walk shows otherwise).

- [ ] **Step 4: Append + verify + commit**

Append `## vector dispatch handlers (verified 2026-09-29, program \`/fifa96.exe\`)` — paragraph + dedupe table + walk tables + signature/adjacency rows + boundary proposals (filled Evidence, calls address-only, no rename column — Task 1 names nothing). Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: dedupe dispatch targets and walk handlers to cited exits" || true`

---

### Task 2: Capped creates + verdicts + dispositions

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdict rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` (per handler: real disassembly at cited range → `create_function` (capped) → rename/plate only if legs cited → `save_program`)

**Interfaces:**
- Consumes: Task-1 dedupe + walk tables + boundary proposals + signatures + adjacency.
- Produces: `### Writes` — before-state per handler (dedupe-table owner responses verbatim + gap row(s)); every command + verbatim response; post-read-backs (bounds vs proposal, side-effect discloses if any auto-island appears — ratified+flagged per slice-17/21 precedent, in-map); verdict rows (`| FUN | address | evidence | new_name | C counterpart |`) — create-only-NOT-CONFIRMED-at-name legal default (entry leg = vector landing always holds; role leg decides rename: only when the body's ops + exit give a mechanism statement within the naming honesty rule — CR0/LGDT/LIDT-class cite for "protected-mode" wording else stop at operand-level wording); plate `C: none — behavioral (<role>)` for renamed only. `### Deferrals` — handler callee trees, `[0x9b4]`+`[0xdfe]` and other cells' consumers, island bodies, block interior past cited exits, `3ed8` arm cascade, readers-side (vector selection logic between `092c`/`0931`), MSW-clear shape status updated only if a walk reached it (else re-cite the flag). Suite + `grep -c "vector dispatch handlers"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Creates (capped)**

Per handler boundary proposal: real disassembly at cited range → `create_function` at start → verify bounds = proposal; deviation → one `disassemble_first=false` nudge → RATIFY with verbatim outputs. No creates where Task 1 recorded a stop-short that makes a defensible boundary impossible.

- [ ] **Step 2: Names + plates where legs cited**

Apply naming honesty rule per handler; record the missing leg (e.g. "exit = tail-JMPF whose segment target is dynamic; role leg open") when withholding. `save_program`; post-state read-backs quoted.

- [ ] **Step 3: Append + verify + commit**

Append `### Writes`, verdict rows, `### Deferrals`. Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: create dispatch handlers and settle verdicts" || true`

---

## Self-Review (ran before save)

- Spec coverage: dedupe with owner-state probes (T1 Step 1 ← design "dedupe the 26 unique offsets"), walk-to-cited-exit per handler with prelude signature verification + block-adjacency to `2a6c` (T1 Steps 2-3 ← design "dry-run walk each handler"; "first attributed entries into the block"), capped creates + collision stop-short rule (T2 Step 1 + Global Constraints ← slice-21 inherited cap + never-over-defined), naming honesty incl. the CR0/LGDT-class bar for "protected-mode" wording (Global Constraints + T2 Step 2 ← design "mechanism level, no direction words beyond bytes"), deferrals covering the guard list (T2 Interfaces ← design).
- Placeholder scan: no TBD/TODO; every branch (owned/foreign collision, stop-short, create-only vs rename, walk-reaches-2a6c vs not) has a stated disposition; per-handler multiplicities make the dedupe countable.
- Type consistency: no C types; addresses (`092c/0931/092d/0934`, `0x0938/093d`, `0x2a5a/2a60`, `2a6c..2a73`, islands `08c2/033c/0bc3`, `2824..284b`/`284c..2863`/`2864..295c`, `mode_vector_source_pair`, `2978..2ada`, band ranges `02d4..0732`/`073c..0928`/`0938..0bd0`, delta `0x1bd0`) match `## 2811..296c pocket + 9bc vector`, `## paging block 2978..2ada`, slice-16 rows; Task 2 consumes Task-1 tables by reference; commit messages distinct; Task 1 zero writes.

(End of file)
