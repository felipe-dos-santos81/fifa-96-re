# FIFA96 R3 IVT Cluster Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the write-window carry-forward of slice 23's R3: walk `FUN_11bd_29bc` (`29bc..2a59`, created default-named in slice 23, "NOT-CONFIRMED-at-name: `INT 0x67`/`JMP BX`/cluster consumers one hop out") — enumerate its control flow to cited exits including the `2a58`/`2a59` tail question (does its last insn land on `2a59` boundary vs the `2a5a` handler entry: the `e9` at `2a58` cited `→2a26` internally — resolve every tail form with byte arithmetic), classify every `INT 0x67` site, trace each `JMP BX`'s BX value backward through the body's own ops, sweep the image for vector-`0x67` staging (IVT offset `0x67×4 = 0x19C` absolute — writers/readers of `[...+0x19c]`-class forms incl. zero-base segments), and dispose the two final-review freebies: the 3-byte owned body `FUN_11bd_0929` (`0929..092b`, never listed) and the `016c` gate tail `023a JZ→0242`-into-`9b` WAIT structure.

**Architecture:** Read-only walk + dataflow + staging sweep + freebies (Task 1, zero writes), then capped writes if the walk exposes new bodies or the naming bar passes for R3 from its own ops (Task 2) + map section `## R3 IVT cluster`. Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true`), `get_function_by_address`, `read_memory` (hex-vs-data protocol), `search_instructions` (authority for operand/contact runs), `analyze_dataflow` (PCode backward walk per site), `get_function_xrefs`/`get_xrefs_to` (controls only), `find_code_gaps` (full pagination), Task 2: real `disassemble_bytes`, `create_function`, `rename_function`, `set_comment`, `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-30 ("R3 `INT 0x67`/`JMP BX` IVT cluster — smaller, new surface"; freebies folded per slice-25 final-review triage: `0929..092b` "promote to first-class walk candidate", `023a/0242` WAIT-gate "freebie since the 60-insn context is now dumped", wording nits "fold next save") + `docs/ghidra/loader_rename_map.md` (`## block head 2978..2a59` — R1 `enable_paging_and_load_tss`, TABLE `mode_29bc_source_pair` (`29b8..29bb` = words `0x2a5a/0x2a60`), R3 `FUN_11bd_29bc` created, internal edges incl `2a58→2a26`, `2a60 = CLI fa` byte verified inside `clear_msw_and_callfar`; `## callee arg question` — arg `0x29bc` = data-only at `[0x9ba]`, stored by `2f18@2ec9`, pair-sourcing math `CS:[BX-4]/[BX-2]`; `## vector selection logic` — DS-clamp storm ledger, `016c` patch legs `0216/021b/0229/022f` heads-`90`, selection rows, deferral bullet "R3 IVT cluster (slice 26 queued)"; `## sweep-aftermath ratification` — census protocol).

## Global Constraints

- No C, test, CMake, or `tools/` changes; the 10-test suite stays green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data` before verbatim.
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` dry-run only; `analyze_dataflow` is a read tool (no cache-mutating claims — quote outputs verbatim); tool error on a required sub-step → BLOCKED.
- Every contact/operand run: pattern + `match_count` + scope (`instructions_scanned`, "at this-slice time" uniformity note) + pagination where applicable (full-paging protocol); controls quoted not relied on; negatives = "not attributable from enumerated sweeps, defined-insn-only" never "doesn't exist"; OPEN-WINDOW rows for dynamic bases with the missing leg named.
- `JMP BX` backward traces: PCode/dataflow steps quoted per site (varnodes, boundaries); when the chain reaches a call-boundary or memory-load, classify (CONSTANT / CELL value with writer cite / DYNAMIC-OPEN) — never assert a BX value without the chain.
- `INT 0x67` classification is static-evidence-only: the IVT is runtime memory — staging-writer sweeps cover what the IMAGE shows (`search_byte_patterns` + operand runs for `0x19c`/`0x1a0`-class offsets with every base-window per the DS-clamp discipline, `mov es,0`-style zero-segment staging functions named); if no writer is attributable statically: OPEN-WINDOW with the exact sweep scope, NOT "no IVT hook exists".
- Capped write path inherited: real disassembly → ONE nudge → RATIFY with verbatim outputs; never create over owned bytes (R3/H13/stub walls: cite last-owned + first-foreign); analyzer side effects disclosed never deleted; `save_program` only after writes, with slice-24 census protocol (count, full gap total, side-effect ledger with `1991:` overlay pagination check).
- Naming bar: verb-led snake_case, mechanism-only from THIS slice's cited ops; IVT-flavored words ("install"/"hook" class) require cited staging writes or a cited `INT`/`IRET` pairing pattern inside the named body — `INT 0x67` alone is a CALL, not an install claim; NOT-CONFIRMED-at-name = default name stands, no plate beyond behavioral.
- Scope guard: handler callee trees stay deferred; `[0x2fa]`/`[0x9ba]` runtime legs stay FU-blocked; `296d`/`02b5` landing-pad family cite-only; the `2a5a` H13 body beyond boundary questions is read-only context.
- Leave `fifa96.rep/**` churn unstaged; prior sections byte-identical (append-only); commit messages exactly per briefs; no new tests.

---

## Scope Check

One function walk + two bounded freebies + one image-sweep question. Task 2's write window is conditional and enumerated both ways (R3 rename at bar / default stands; exposed bodies if the tail resolves into unowned bytes — expected NONE since `29bc..2a59` is fully owned; possible zero-writes legal outcome).

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## R3 IVT cluster` (Task 1: flow/exits + INT/BX tables + staging sweep + freebie rows; Task 2: `### Writes` + verdicts + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only if bar passes (rename + plate) or exposed bodies (none expected); `save_program` after writes.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Walk, trace, staging sweep, freebies (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## R3 IVT cluster` with flow/BX/INT/staging/freebie tables)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: R3 live bounds (`get_function_by_address(11bd:29bc)`) + slice-23 stream classification (49 insns, internal edges incl `2a24→2a39`, `2a58→2a26`); `disassemble_function(11bd:29bc)` body; H13 boundary bytes at `2a5a`; `mode_29bc_source_pair` row (`29b8..29bb` → `0x2a5a/0x2a60`); `016c` context (`FUN_11bd_016c` `016c..0245`, gate rows from slice-25 walk); `FUN_11bd_0929` (`0929..092b`) live listing; DS-clamp discipline template from `## vector selection logic` storm row.
- Produces: flow table (`| insn | bytes | class | target/exit cite |`) for the whole R3 body incl. the tail resolution (last instruction end-offset vs `2a59`, and whether any path falls through or jumps to `2a5a`); `INT 0x67` site list (`| site | bytes | preceding context (one cited insn) | enclosing block |`); BX-trace table (`| JMP BX site | backward chain (varnode steps) | outcome CLASS |`); staging sweep (`| pattern/base-window run | match_count | scope | hits classified READ/WRITE/false-string |`) for `0x19c`-class IVT offsets across zero-segment staging shapes (`c7069c01`, `ea`-pointer stores, `8c`-segment-MOV + disp `019c`), with per-site classification and OPEN-WINDOW rows for dynamic bases; freebie rows: `0929..092b` listing disposition (what the 3 bytes are, whether the function body matches the bytes, entry vs body) + `016c` `023a/0242` WAIT-gate one-row structure cite.

- [ ] **Step 1: Flow + tail resolution**

`get_function_by_address(11bd:29bc)` + `disassemble_function` full body; walk every exit path to cited terminator (`c3`/`cb`/`ea`/tail-`e9`/fallthrough); resolve the tail: last insn's encoding end-offset vs `2a59`; any edge to `2a5a`-class cites the instruction + arithmetic.

- [ ] **Step 2: INT + BX sites**

Collect `INT 0x67` (`cd67`) sites with bytes + one-cited-insn context; each `JMP BX`-class site gets an `analyze_dataflow` backward walk (quote the steps output; classify CONSTANT/CELL/DYNAMIC-OPEN).

- [ ] **Step 3: Staging sweep**

`search_byte_patterns` for `c7 06 9c 01`-class stores + `search_instructions` operand runs for `0x19c`-disp forms; base-window arithmetic per discipline (constant-base census, partition runs, OPEN-WINDOW holes, DS-clamp where zero-segment staging); classify every hit; controls quoted.

- [ ] **Step 4: Freebies + append + commit**

`0929..092b` live `disassemble_function`/listing quote + disposition row; `016c` gate-tail cite row. Append `## R3 IVT cluster (verified 2026-09-30, program \`/fifa96.exe\`)` — tables + disposition-so-far (exit classes, INT site count, BX outcome classes, staging answer or OPEN-WINDOW named). Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: walk R3 to exits, trace BX chain, sweep IVT-19c staging" || true`

---

### Task 2: Bar verdict + capped writes if any

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdict rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` — ONLY if the bar passes (rename + plate) or Task 1 exposed an unowned CODE run (none expected — the tiling is owned); `save_program` after writes

**Interfaces:**
- Consumes: Task-1 flow/INT/BX/staging/freebie tables.
- Produces: `### Writes` — either verbatim rename+plate sequence with before/after (`get_comment`, `get_function_by_address`, `rename_function` response, post-read-back of signature+comment) or the zero-writes disclosure (bar test shown from cited ops, staging sweep outcome class). Verdict row for R3 (name decision + the bar test as printed; role leg per what the walk showed — INT-consume vs staging-writer are different roles); staging disposition row (attributed-writers or OPEN-WINDOW with scope); freebie ratification rows; `### Deferrals` — callee trees (R3's own `INT 0x67` target = runtime IVT: FU-blocked line), `2a5a` H13 internals beyond boundary, `[0x2fa]`/`[0x9ba]` runtime, `0d62` dive, handler callee trees, runtime writers, name-class route, Δ1. Suite green; `grep -c "R3 IVT cluster"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Bar test → rename decision**

From Task-1 ops: does R3 show cited staging writes (zero-segment + `0x19c`-disp stores) or a save/restore pairing around `INT` forms that gives a mechanism-level role? Print the bar test (ops cited, candidate name, gate). Pass → `rename_function` + plate `C: none — behavioral (role)` + `save_program` + post-read-backs + census protocol; fail → default name stands, NOT-CONFIRMED-at-name row.

- [ ] **Step 2: Exposed bodies (conditional)**

Only if Task 1 found unowned contiguous CODE: capped create path (real disasm → ONE nudge → RATIFY). Zero-writes expected — disclose the byte-level proof (owned walls cited).

- [ ] **Step 3: Append + verify + commit**

Append `### Writes` + verdicts + `### Deferrals` (+ `### Fix wave` trailer if own-row edits). Gate: full build + `ctest` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: R3 verdict with IVT staging disposition" || true`

---

## Self-Review (ran before save)

- Spec coverage: R3 walk to cited exits + tail resolution (T1 Steps 1 ← design "enumerate exits incl 2a58/2a59 tail"); `INT 0x67` classification per site (T1 Step 2 ← design); `JMP BX` backward chains with classes (T1 Step 2 ← design "trace each JMP BX's BX value backward"); staging sweep for vector-`0x67`/`0x19C` with DS-clamp discipline + OPEN-WINDOW not-nonexistent (T1 Step 3 ← design); freebies `0929` + `023a/0242` (T1 Step 4 ← final-review triage verbatim); bar for IVT-flavored words requires staging writes or INT/IRET pairing — `INT`-alone ≠ install claim (Global + T2 Step 1 ← honesty rules + design); zero-writes legal outcome enumerated (T2 ← design); save protocol (Global ← slice-24 lesson).
- Placeholder scan: no TBD; both rename outcomes specified; exposed-bodies branch enumerated with expected-NONE + proof requirement.
- Type consistency: no C types; addresses match `## block head 2978..2a59` (`29bc..2a59`, `2a24→2a39`, `2a58→2a26`, `29b8..29bb`→`0x2a5a/0x2a60`, `2a60=fa`), `## callee arg question` (`0x29bc` data-only, `2f18@2ec9`, `BX-4/BX-2`), `## vector selection logic` (`016c..0245`, `0216/021b/0229/022f/023a/0242`, `FUN_11bd_0929 0929..092b`, DS-clamp row), IVT math `0x67×4=0x19C`; Task 2 consumes Task-1 tables by heading; commit messages distinct; Task 1 zero writes.

(End of file)
