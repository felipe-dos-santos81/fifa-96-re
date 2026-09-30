# FIFA96 Orphan Family Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Answer the family question the last three slices upgraded, over six recorded defined-unowned/artifact regions: `11bd:02da..02f8` (slice-15 orphan tail — RATIFIED truncated, name withheld, direction UNDECIDED), `0976..099a` (H7 remainder behind the `f3f0`@`0974` split), `6328..634e` (slice-27 staging strip — zero static entries, leave-as-bytes), the foreign flip-remnant `64ff..6548` (row `1000:80cf..8118`, belongs to no half), the H9 half `0a35..0a5d` (blocked on the `DAT_11bd_0a35` listing decision — its head is misaligned: true bytes `b080`@`0a35`+`e620`@`0a37` vs artifact `AND DH,0x20`@`0a36`), and the H3-surplus arg-cells `0675..0696` (`9706`=`0x0697` read-back — slice-22 argued DATA). The question: do they share a structural mechanism (sweep-artifact vs runtime-consumer family vs arg-cell data misflagged), does any region already have a cited static inbound edge (the R1 tail `e97fd9`@`29b5` → relay `0337` proves the category exists), and for each region — create with DYNAMIC-ONLY entry (only where the H13 precedent's mechanism requirement is met: a cited runtime-reachability story, not mere un-ownership), leave, or fix-listing-then-create (H9: at most ONE capped `clear_flow_and_repair`/re-align attempt per the write-admission below).

**Architecture:** Read-only family census (per-region current listing state + two-render spans + inbound-edge runs + shape-signature byte comparison + mechanism rows) and listing-decision analysis (Task 1, zero writes), then capped dispositions per proposals + verdict rows + `## orphan family` section (Task 2). Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true`), `read_memory` (hex-vs-data protocol), `search_instructions` (authority), `search_byte_patterns`, `find_code_gaps` (full pagination), `audit_data_region`/`analyze_data_region`, `get_xrefs_to` (control), Task 2: real `disassemble_bytes`, `create_function`, `clear_flow_and_repair` (single capped attempt, see Constraints), `clear_instruction_flow_override`, `rename_function`, `set_comment`, `save_program` + slice-24 census protocol), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-30 (final-review slice-28 triage, verbatim: "the orphan-FAMILY (`02da..02f8`/`0976..099a`/`6328..634e`) + foreign `64ff..6548` + H9 `DAT_11bd_0a35` listing decision + H3-surplus `0675..0696` arg-cells belong together (all are flip-carve/arg-cell artifacts of the slice-23 sweep); carry `674c/675a`, `2cc5/0e3c`, `[0x56]/[0x58]`, runtime writers/RETF-consumer, name-class route, Δ1 unchanged") + `docs/ghidra/loader_rename_map.md` (`## 02b7 twin` — orphan tail `02da..02f8` RATIFIED record incl. the direction-UNDECIDED both-bodies-set-only-`OR [0x40]` note; `## vector dispatch handlers` — H7 `0938..0973` + `f3f0`@`0974` split + `0976..099a` remainder deferral row, H9 relabel row + row `2605..262d`, H3 relabel + surplus finding; `## 2811..296c pocket + 9bc vector`; `## block head 2978..2a59` — R1 `enable_paging_and_load_tss` tail `29b5 e97fd9` → relay `0337` (`caseD_0`, `0337..033b`, `2eff26fa02` cell-`[0x2fa]`), DYNAMIC-ONLY precedent (`2a5a` created with cited pair-table mechanism); `## sweep-aftermath ratification` — flip row `80cf..8118` original state; `## IVT loose ends` — strip full evidence package (`0929`-adjacent, `62f8` writer pair, JNZ→`1000:7f0d`=internal `633d` loop-back, zero static entries); `## far-return halves` — surplus flip RATIFY row `false→true`, `0675` bytes, six verdict rows, merged rulings list incl. "create needs mechanism" principle).

## Global Constraints

- No C, test, CMake, or `tools/` changes; suite stays 10/10 untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data` before verbatim.
- Task 1 writes NOTHING: dry-run only; reads only; tool error on required sub-step → BLOCKED.
- Per-region mandatory evidence set (uniform across the six): current listing state (`get_function_by_address` start±1 + both walls, verbatim; gap row verbatim from FULL pagination pass); two-render windows + maximal render-stable CODE span; inbound-edge census — operand runs for EVERY address in the region's first 4 bytes (both space-views) + the relay-precedent form: cited insns whose recomputed target lands ANYWHERE in the region (R1→`0337` proves inbound static edges exist for this class; run them, don't assume absence); terminator/head-shape byte signature vs the other regions (the family comparison the design asks for: head bytes + loop-back + exit-op multiset per region, rendered as a 6×N signature table); mechanism row per region {pair-table word resolving into it (cite the table row + word), arm-cell writer with value, RETF-consumer story cite, or NONE}.
- Create admission rule (binding, from the merged H9/H3 asymmetry ruling): a body may be created with DYNAMIC-ONLY entry ONLY with a cited runtime-reachability mechanism row (H13's pair-table is the template); zero-static-entries WITHOUT a mechanism ⇒ leave-as-bytes disposition row (slice-27 precedent); DATA-classified regions (arg-cells) ⇒ data-disposition rows, no create over cell content.
- H9 listing decision is the ONE sanctioned flow-repair: at most ONE `clear_flow_and_repair` call seeded in the `0a35..0a5d` region (GUI-action semantics: clears flow from seed, repairs bodies, re-disassembles — follows flow BEYOND the seed; non-idempotent; can clear healthy flow — seed choice and expected-effect reasoning must be printed BEFORE the call, prior state quoted, and the result is RATIFIED-as-returned whatever it is; a second call is NOT sanctioned; if post-repair the aligned head `b080`@`0a35` renders as true code AND `0a34`-wall/`0a5e`-owner intact ⇒ THEN the capped create at `0a35..0a5d` applies with the mechanism row from Task 1, else leave with verbatim before/after). `clear_instruction_flow_override` only as the paired repair if the flow override itself caused damage (quote first).
- Capped-write regime for everything else: real disassembly → ONE nudge → RATIFY, verbatim before/after; never create over owned/defined bytes; stop-short wall cites; save ⇒ FULL slice-24 census protocol (count Δ reconciled to the write-set EXACTLY — the Δ arithmetic is itself the side-effect proof; gaps FULL pagination; neighborhood re-derivations; overlay membership spot-checks; scope line; any flip/absorption named in the ledger).
- Naming bar unchanged: printed-ops mechanism names only (`return`-class needs cited terminator op — near `c3` admissible per slice-28 ruling 1; `far`-class needs segment-pop evidence; IVT/mode words need their cited pairing/CR-class ops); NOT-CONFIRMED-at-name default stands; plate `C: none — behavioral (<role>)`; direction words banned (H7-remainder keeps UNDECIDED status per slice-15/22 records — a family signature does NOT resolve direction unless ops do, cite).
- Scope guard: `674c/675a`, `2cc5/0e3c`, `[0x56]/[0x58]` full stories, `[0x2fa]`/`[0x9ba]` runtime, name-class route, Δ1 — cite-only carry; no handler-primary re-walks; the relay `0337`'s own cell story (`[0x2fa]` writers) stays deferred (its inbound edge is cited as precedent only).
- Append-only map (new section `## orphan family`); prior sections byte-identical; leave `fifa96.rep/**` churn unstaged; commit messages exactly per briefs; no new tests.

---

## Scope Check

Six uniform evidence packages + one mechanism question + one sanctioned flow-repair; every write branch enumerated (create/leave/data-repair/fail-census). The "answer the family question" framing makes the 6×N signature table the reviewable spine.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## orphan family` (Task 1: per-region tables + signature matrix + mechanism rows + listing-decision analysis; Task 2: `### Writes` + verdicts + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: ≤1 flow-repair + ≤1 H9-path create + mechanism-admitted creates per proposals + bar renames; `save_program` + census.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Family census + mechanism analysis (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## orphan family` with per-region evidence + signature matrix + mechanism + proposals)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: the six regions' prior records verbatim (slice-15 tail row, H7 split row, strip package rows, `80cf` flip row, H9 denial + relabel rows, surplus RATIFY row); R1 tail bytes `e97fd9` + relay `0337..033b` row (`2eff26fa02`) as the inbound-edge precedent; H13 mechanism template; delta `−0x1bd0`; live count 334 / gaps 149 / scope 15665 — re-read all.
- Produces: per-region table ×6 (`| item | output (verbatim) |`: listing state probes, gap row, two-window renders + maximal stable span, inbound runs per Global with per-hit arith, terminator/head ops); signature matrix (`| region | head bytes | loop-back forms | exit-op multiset | arg-cell reads? | class candidate |`) computed from THIS pass's bytes; mechanism rows ×6 {table-word cite / arm-writer cite / RETF story cite / NONE}; arg-cell analysis for `0675..0696` (per-cell: who reads each word — the six pairing-stores from slice 28 are candidates — render `9706`-class values vs arg table values, conclude DATA or CODE per cell, cite); H9 listing-decision analysis (current `DAT_11bd_0a35` state via `analyze_data_region(11bd:0a35, …)` read-only; what `clear_flow_and_repair` seeded at `0a35`/`0a36` is expected to do given `0a34`-wall (`0931..0a34`? NO — `0a34` is inside H9-primary `09d7..0a34`… derive live), risks named (non-idempotent, flow beyond seed), the pre-call expected-effect paragraph Task 2 must quote verbatim); disposition proposal per region {create w/ mechanism cite, leave (mechanism NONE), data-repair, flow-repair+create path (H9)} + bar pre-tests.

- [ ] **Step 1: States + spans ×6**

Probes/gap rows/two-window renders; maximal stable spans with byte cites; compare each vs its prior record (deltas are findings — e.g. `02da..02f8`'s state may have drifted through four save-sweeps).

- [ ] **Step 2: Inbound edges ×6**

Operand runs for every start-class address + region-wide target-landing arithmetic (the R1→`0337` precedent run: which insns land in each region — including the strip's own `633d` internal loop-back NOT counted as external); controls quoted.

- [ ] **Step 3: Signatures + mechanisms + arg-cells**

Multiset/head/loop signature matrix; mechanism rows; per-cell arg analysis with reader cites.

- [ ] **Step 4: H9 analysis + proposals + append + commit**

Flow-repair expected-effect paragraph (pre-written for Task 2 to quote); dispositions + bar pre-tests. Append `## orphan family (verified 2026-09-30, program \`/fifa96.exe\`)`. Gate: build + `ctest` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: six-region orphan-family census with signatures mechanisms and proposals" || true`

---

### Task 2: Execute dispositions (capped)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdict rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` — creates only per mechanism rule; ONE flow-repair attempt (H9 path) under the full pre/post protocol; renames at bar; `save_program` + census

**Interfaces:**
- Consumes: Task-1 six packages + proposals + the quoted expected-effect paragraph.
- Produces: `### Writes` — per executed action: before-state verbatim, command, verbatim response, post-read-back, stop-short/wall re-cites; the flow-repair block additionally: Task-1 paragraph quoted FIRST, full pre-capture (region disasm + gap rows + count), the single call, RATIFY-as-returned post-capture, THEN either the capped create (aligned head + walls intact + mechanism cite) or the leave row; regions with mechanism NONE ⇒ explicit not-created rows. Save ⇒ full census protocol (Δ reconciliation to write-set exact, neighborhoods re-derived, flips named). Verdict rows ×6 {disposition final, entry class (DYNAMIC-ONLY w/ mechanism cite or NONE⇒leave), family-class answer row: what the signature matrix says the regions ARE (one paragraph, cited per row)}. `### Deferrals` — carry list updated (closed items removed, family leftovers kept). Suite green; `grep -c "orphan family"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Pre-write re-check ×6**

Live state parity vs Task-1 quotes (any drift ⇒ downgrade that region to disposition-only, both quotes recorded).

- [ ] **Step 2: Execute proposals (capped)**

Creates with mechanism cites; H9 path only if Task-1's preconditions hold at pre-check; zero nudges-to-fight — cap regime verbatim.

- [ ] **Step 3: Save + census + append + commit**

`save_program` if writes; full protocol; append `### Writes`/verdicts/`### Deferrals` (+ `### Fix wave` trailer if needed). Gate: build + `ctest` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: dispose orphan family with flow-repair attempt and mechanism-gated creates" || true`

---

## Self-Review (ran before save)

- Spec coverage: six regions uniform evidence (T1 Steps 1-3 ← design's six named blocks); "do they share a mechanism" answered via signature matrix + family-class verdict row (T1 Step 3 + T2 verdicts ← design question); inbound-edge census including the relay-precedent category (T1 Step 2 ← R1→0337 evidence); create-needs-mechanism admission rule enforcing the merged H9/H3 asymmetry ruling (Global + T2 ← slice-28 final review); H9 listing decision as the single sanctioned flow-repair with pre-quoted expected-effect + RATIFY-as-returned (Global + T1 Step 4 + T2 Step 2 ← design's "listing decision"); arg-cells DATA-vs-CODE per-cell analysis (T1 Step 3 ← slice-22 arg `0x679` story); direction-UNDECIDED carried unless ops resolve (Global naming line ← slice-15 record); scope guard excludes `674c/675a` etc. (Global + T2 Deferrals ← spec triage line); census Δ-reconciliation-as-side-effect-proof (T2 Step 3 ← slice-28 final-review strengthening).
- Placeholder scan: no TBD; flow-repair has precondition-fail branch (leave row); every mechanism outcome has its disposition; "answer the question" is a named deliverable row not vibes.
- Type consistency: no C types; regions (`02da..02f8`, `0976..099a`, `6328..634e`, `64ff..6548`, `0a35..0a5d`, `0675..0696`), relay (`29b5 e97fd9`→`0337`, `2eff26fa02`, `[0x2fa]`), H9 bytes (`b080`@`0a35`, `e620`@`0a37`, artifact `80e620`), H3 (`9706`=arg-`0x679`-cell, `0675..0696` vs row `2245..2266`), walls (`09d4..0a34` primary, `0a5e` next), strip internals (`633d` loop-back via `7f0d`-render, `0f001004` `LLDT`-class tail), flip rows (`2605..262d`, `80cf..8118`), count 334/gaps 149/scope 15665, delta `−0x1bd0` — all match `## far-return halves`, `## IVT loose ends`, `## sweep-aftermath ratification`, `## 02b7 twin`, `## vector dispatch handlers`, `## block head 2978..2a59`; Task 2 consumes Task-1 rows by heading; commit messages distinct; Task 1 zero writes.

(End of file)
