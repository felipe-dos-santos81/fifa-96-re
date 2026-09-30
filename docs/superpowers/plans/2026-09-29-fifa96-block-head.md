# FIFA96 Block Head Disposition Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Disposition the block head `11bd:2978..2a59` — the last ownerless stretch of the former `2978..2ada` block (gap row `1000:4548..4629`, size 226) — classifying every byte-run CODE/TABLE/PADDING/UNKNOWN with citations, answering the reachability question for its head (unconditional `JMP` tail at `2975` means nothing falls in from `296d` — so what statically enters?), sweeping the `[0xdfe]` cell contacts (the `CMP byte [0xdfe],0x1`@`2978` reader and its still-runtime-open writers), confirming the `0x29b8/0x29ba` pair bytes' table role (arg `0x29bc`'s pair source in the determinability appendix), and applying the inherited write path at cited boundaries (creates; `ushort[2]`+label defines on the `mode_vector_source_pair` COMPLETE_80 template; PADDING untouched).

**Architecture:** Read-only evidence pass (stream classification + reachability re-verify + cell sweep + table role, zero Ghidra writes, Task 1), then capped writes at cited ranges + dispositions + map append `## block head 2978..2a59` (Task 2). Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`find_code_gaps`, `disassemble_bytes` (Task 1 `dry_run=true` only), `disassemble_function`, `read_memory` (hex-vs-data protocol), `search_instructions` (authority), `get_function_by_address`, `get_xrefs_to`/`list_data_items_by_xrefs` (control only — dead channel), `search_byte_patterns`, `create_function`, `create_array_type`/`apply_data_type`/`create_label`/`set_global`/`audit_global`, `rename_function`, `set_comment`, `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-29 (byte-run classification with contiguous 226 tiling; reachability re-verify incl. new handler edges and determinability pair-source math resolving into the head; `[0xdfe]` contact sweep; `0x29b8/0x29ba` table role; capped creates/defines on the proven templates; `[0xdfe]` writer and callee trees deferred) + `docs/ghidra/loader_rename_map.md` (`## 296d hook target` — `restore_fs_gs_and_resume` tail `JMP 0x1000:1e85`@`2975` unconditional, `2978` = `CMP byte [0xdfe],0x1`; `## paging block 2978..2ada` — zero static entries from defined insns + fall-in caveat, `CS:[0x2ad9]` tail ownership, Concern-3 shrink note now moot; `## vector dispatch handlers` — carve `2978..2a59` gap row `1000:4548..4629` size 226 + handler `2a5a..2ad8` + tail row `1000:46a9..46aa` size 2; determinability appendix `29b8`→`5a2a602a` words `0x2a5a/0x2a60`; `2820..2823` `mode_vector_source_pair` COMPLETE_80 define template + plate format; `## 2811..296c pocket + 9bc vector` — sweep discipline + quote-protocol note binding; `## callee arg question` — six far-return halves deferred status, `[0x9b4]`/`[0xdfe]` listed as unconsumed-in-scope).

## Global Constraints

- No C, test, CMake, or `tools/` changes in this slice; the 10-test suite must stay green untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data` before verbatim (slice-17 protocol).
- Task 1 writes NOTHING in Ghidra: `disassemble_bytes` only `dry_run=true`; reads only via `disassemble_function`/`search_instructions`/`read_memory`/`search_byte_patterns`/`get_function_by_address`/`find_code_gaps`; tool error on a required sub-step → BLOCKED report, never a write. All listing mutations belong to Task 2 at Task-1-cited ranges only.
- Do NOT invent addresses: every run start/exit cites emitted bytes; reachability claims cite either an instruction operand with recomputed arithmetic (`e9`/`e8` rel16 with next-IP math; `ff16/ff26` cell forms with cell + value source) or the no-entry finding itself.
- Capped write path (inherited): real `disassemble_bytes` → at most ONE `create_function` nudge (`disassemble_first=false`) → RATIFY on second refusal, verbatim outputs; NEVER create over defined bytes; if a CODE run abuts an existing FUN (`2a5a`-family handler, islands, stubs), the boundary stops one byte short, cited; `0x29b8/0x29ba` define must not collide with any create the same batch wants (data-vs-code claim conflict → BLOCKED, don't both).
- TABLE defines: `ushort[2]`-style via `create_array_type`+`apply_data_type`+`create_label` on the `mode_vector_source_pair` precedent (same plate format, `audit_global` verify, COMPLETE band quote optional); CODE creates default-named unless walk ops give a mechanism-level role at the naming bar (CR0/LGDT/LIDT-class cite required for mode-flavored words; NOT-CONFIRMED-at-name = no rename/plate beyond behavioral default); no `decode_*`, no direction words.
- Sweep discipline (slice-16/20): every run lists pattern + `match_count` + scope (`instructions_scanned` uniformity noted "at this-slice time" per slice-21 ruling); `[base+disp]` window arithmetic for the `[0xdfe]` reach class (constant-base census + digit/sign partitions + OPEN-WINDOW holes, never silent rejects); controls (`get_xrefs_to`, `list_data_items_by_xrefs`, raw byte patterns) reported, not relied on; negatives carry the defined-instructions-only caveat + MOVS/STOS/offset-render blind spots.
- Scope guard: callee trees ONE hop with cites; `[0x9b4]`/`[0x40]` consumer stories, `[0xdfe]` runtime writers, islands' bodies, six far-return halves, twin/band regions — cite-only status lines; the `2a5a` handler's body is read-only context (its bytes are owned; no re-walk beyond cited entry ops).
- Prior sections read-only (pure append, zero deletions outside new section; own-section fixes allowed per slice-20/21/22 rulings); `save_program` after any write batch; leave `fifa96.rep` churn unstaged; commit messages exactly per the briefs; no new tests.

---

## Scope Check

One byte-range disposition plus one cell sweep plus one reachability question, two gates: evidence (Task 1), writes + map rows (Task 2). Deliverable per task: map rows + suite green. Handler callees, runtime writers, far-return halves — named non-goals.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## block head 2978..2a59` (Task 1 edge/gap confirm + classification + reachability + sweep tables; Task 2 `### Writes` + verdict rows + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: real disassembly + creates at cited CODE ranges; `ushort[2]`+label define at cited TABLE range; plates; `save_program`.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Stream classification + reachability + [0xdfe] sweep (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## block head 2978..2a59` with confirmation/classification/reachability/sweep tables)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: gap rows `1000:4548..4629` (head, 226) + neighbors (`restore_fs_gs_and_resume`@`296d..2977` before, `clear_msw_and_callfar`@`2a5a..2ad8` after) from `## vector dispatch handlers` `### Writes`; `2978` first bytes (`80 3e fe 0d 01`-class — the `CMP byte [0xdfe],0x1` cite); determinability appendix `29b8` row; `mode_vector_source_pair@2820` template; pocket `9bc/9be` sweep counts as the reproduction target for controls.
- Produces: confirmation rows (both edge `get_function_by_address` responses; head gap row verbatim; delta math `0x4548−0x1bd0=2978`, `0x4629−0x1bd0=2a59`, size 226 = `2a59−2978+1` shown); contiguous classification table (`| run | range | evidence (hex+data reconciled) | class |`) tiling `2978..2a59` with the tiling sum stated; reachability table (`| entry candidate | evidence | verdict |`) — for the head offset `2978`: re-assert the unconditional tail (`JMP`@`2975` bytes + no fallthrough path), enumerate static candidates (defined-insn `CALL`/`JMP` with targets recomputed into `2978..2a59` — expected zero per slice-17, re-run the `0x1000:45/46`-family patterns to catch the NEW post-carve state; cell-vector forms: `41ee`→`0x296d` re-cite as the only static `[0x9c2]` writer; determinability-pair-source words resolving into the head — e.g. any `[BX-4]/[BX-2]`-loaded cell whose static bytes land in `2978..2a59`), each with cited math or a no-hit count; `[0xdfe]` contact table (`| pattern run | match_count | hits + classification |`) + base-window arithmetic rows + controls, with per-hit read/write/false-string classification incl. `2978` as reader; `0x29b8/0x29ba` role row (pair bytes + the `publish_mode_vector` load ops at `6270/6277` cited via map rows + `read_memory(29b8,4)` reconciliation + home: currently inside the head's CODE/TABLE class claim).

- [ ] **Step 1: Confirm edges**

`get_function_by_address(11bd:2978)` (expect no-function) + `(2977)`/`(2a59)`/`(2a5a)` owner responses quoted (restore stub / head / handler); `find_code_gaps` head row verbatim + size arithmetic; note the tail-`JMP` cite from `## 296d hook target` (byte-level, not re-walked — the 2975 `e9` render is enough, quote from prior row AND re-cite live `read_memory(2975,3)`).

- [ ] **Step 2: Classify the stream**

Dry-run `disassemble_bytes` windows across `2978..2a59`; every maximal run: emitted bytes + class; decode-skips disclosed per the pocket precedent (cite both sides + raw); PADDING classed by byte pattern; `0x29b8/0x29ba` region checked against the CODE stream claim (overlap = conflict to resolve in Step 3 reachability evidence, not by fiat).

- [ ] **Step 3: Reachability + sweep**

Re-run the pattern families (`0x1000:29`, `11bd:29`, `0x1000:45`, near forms) with per-hit target recomputes — hits landing in `2978..2a59` = the static entries (expect none; report any); cell forms: `search_instructions` operand `[0x9c2]` writer re-verify (`41ee` sole) + determinability table re-scan: any of the 28 target words landing in the head range (compute, cite rows); `[0xdfe]` sweep per the discipline; `read_memory(29b8,4)` reconciled and quoted.

- [ ] **Step 4: Append + verify + commit**

Append `## block head 2978..2a59 (verified 2026-09-29, program \`/fifa96.exe\`)` — paragraph + tables + disposition-so-far line (per-class counts, head-entry verdict, `[0xdfe]` reader found/writers open, pair-role claim). Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: classify block head, re-verify static entry, sweep 0dfe contacts" || true`

---

### Task 2: Capped creates + define + dispositions

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdict rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` (real disassembly + `create_function` at cited CODE ranges; `ushort[2]`-style define + label at the cited TABLE range; plates; `save_program`)

**Interfaces:**
- Consumes: Task-1 confirmation/classification/reachability/sweep tables + the Step-2 conflict resolution.
- Produces: `### Writes` — before-state per range (no-function response + gap row verbatim); every command + verbatim response; post-read-backs (bounds vs proposals, define `audit_global` + `analyze_global_completeness` band, gap re-page arithmetic for the head shrink, count delta); cap compliance rows for any deviation; TABLE define on the `mode_vector_source_pair` template (`ushort[2]` + label snake_case behavioral name from the cited use + plate `C: none — behavioral (<role>)`); CODE creates default-named unless the naming bar passes per body; verdict rows for created FUNs (entry leg = the reachability evidence — if head is statically unreachable, entry = DYNAMIC-ONLY, cited; role leg decides rename, else create-only NOT-CONFIRMED-at-name); `### Deferrals` — `[0xdfe]` runtime writers (writer set still zero, one line), `[0x9b4]`/`[0x40]` consumers, created-handler callee trees one hop out, six far-return halves, islands' bodies, tail `2ad9..2ada`; conflict notes from Step 2 resolved (pair bytes: which claim survived). Suite green; `grep -c "block head 2978"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Creates**

Per CODE run: real disassembly → create → bounds check vs proposal → one nudge max → RATIFY with verbatim outputs; stop-short at defined edges (handler@`2a5a`, stub@`296d..2977`); never create over the TABLE-claimed bytes.

- [ ] **Step 2: Define + labels + plates**

TABLE run (the `29b8..29bb` pair or a wider table region Task 1 evidenced): `create_array_type` + apply + label (role wording from the determinability use: near-offset pair for mode-`0x29bc` args — mechanism-level, no "vector"-lore beyond cited `[0x9bc]/[0x9be]` load chain) + plate; `audit_global` verify. Default names for creates; plates only at the bar. `save_program`.

- [ ] **Step 3: Append + verify + commit**

Append `### Writes` + verdict rows + `### Deferrals` (+ `### Fix wave` trailer if own-row edits were needed per slice-22 practice). Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: create block-head CODE runs and define pair table with dispositions" || true`

---

## Self-Review (ran before save)

- Spec coverage: contiguous 226-tiling classification with disclosed skips (T1 Steps 1-2 ← design (1)); reachability re-verify including NEW post-carve state + cell-vector + determinability-words resolving into the head (T1 Step 3 ← design "what statically enters?"); `[0xdfe]` sweep with window arithmetic + per-hit classification (T1 Step 3 + sweep interface ← design); `0x29b8/0x29ba` role confirmation w/ load-chain cites (T1 interface + T2 Step 2 ← design "TABLE candidate alongside the 2820 precedent"); CODE-vs-TABLE conflict rule (T1 Step 2 + T2 Step 1 "never create over the TABLE-claimed bytes" ← design conflict caution); capped creates + default-naming + bar (T2 Steps 1-2 + Global Constraints ← design); `[0xdfe]` writer + callee trees + far-return halves deferrals (T2 `### Deferrals` ← design).
- Placeholder scan: no TBD/TODO; head-entry outcome (none / found) both have dispositions; conflict branch defined (BLOCKED, not both); define template anchored to a named precedent.
- Type consistency: no C types; addresses (`2978`, `2a59`, `0x4548..0x4629`, `2975` tail, `[0xdfe]`, `29b8/29ba`, `6270/6277`, `2820..2823`, `41ee`, `2a5a..2ad8`, `296d..2977`, delta `0x1bd0`) match `## 296d hook target`, `## paging block 2978..2ada`, `## 2811..296c pocket + 9bc vector`, `## vector dispatch handlers` carve rows, `## callee arg question`; Task 2 consumes Task-1 tables by reference; commit messages distinct; Task 1 zero writes.

(End of file)
