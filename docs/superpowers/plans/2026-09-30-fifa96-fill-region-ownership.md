# FIFA96 Fill Region Ownership Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Own or disposition the four unowned regions that slice 30's chains map named as "the only route to a future create on the int-FF pair": the two **fill regions** carrying the twin IVT arms — `11bd:0360..040d` (holds `03a7/03a9` staging + twin-A arms `03ab` = `26c706fc03940b`, `03b2` = `268c0efe03`) and `045e..0490` (holds `0462/0464` staging + twin-B `0466/046d`) — the **third-writer band** `0745..076e` (the `075e 8ec0` + `0760` ES-form arm-writer candidate with basis OPEN, re-grounded in slice 30's fix wave), and the **landing band** `0b94..` (the region the int-FF twins' IP value points at, whose current extent is the gap row `1000:26e2..2792`-class — derive live). For each: full listing state, two-render maximal CODE span, inbound-edge census with arith, content classification (tenant data vs code islands), and a mechanism/disposition verdict per the binding create gate — with the honest possibility, stated up front, that mode-dependence discovered at slice 30 means these regions gain ownership WITHOUT the chain ever closing (fill code may itself be the runtime/reset path).

**Architecture:** Read-only four-region evidence packages + chains-table status updates (Task 1, zero writes), then capped creates at cited spans where entry/mechanism grounds pass, else leave/disposition rows + `## fill region ownership` section (Task 2). Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true`), `read_memory` (hex-vs-data protocol), `search_instructions` (authority), `search_byte_patterns` (with the slice-30 flakiness rule: zero-hit exact-forms = existence negatives; positives read-grounded), `analyze_data_region`, `find_code_gaps` (full pagination), `get_xrefs_to` (control only), Task 2: real `disassemble_bytes`, `create_function`, `rename_function`, `set_comment`, `save_program` + census protocol), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-30 ("Fill-region ownership — `0360..040d`/`045e..0490` + the `0745..076e` third writer + landing band `0b94..` (the only known route to future creates on the int-FF chains)", user "yes") + `docs/ghidra/loader_rename_map.md` (`## arm pairs and R6 band` — chains table 15 rows (twin rows HALF w/ CITED-SEMANTICALLY mode-dependent consumer leg; writer-open fill rows; landing-open rows), the fix-wave reconciliations, `0760` ES-form soften w/ `075e` anchor, flakiness rule, Deferrals' fill-region bullet; `## vector dispatch handlers` — band neighbors `02da..02f8` orphan tail, `033c` island, `073c..0744` island, H5 `076f..07b8`, H10 `0a86..0a9e`, `0ae2..0b11` H12 body + its `0b0b JNZ→0ad5` internal edge, landing band row quotes; `## 02b7 twin` — R1/R2 direction-UNDECIDED precedent (fill shapes); `## block head 2978..2a59` — relay `0337..033b` `JMPF [0x2fa]` (the `0360..040d`-neighbor thunk); `## far-return halves` — H1 primary/half created bounds `040e..0442`/`0443..045d` (the walls around fill-A's east end); `## sweep-aftermath ratification` — census protocol).

## Global Constraints

- No C, test, CMake, or `tools/` changes; suite stays 10/10 untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; quote-protocol on every `read_memory` (hex↔data reconciled).
- Task 1 writes NOTHING: dry-run only; reads only; tool error on required sub-step → BLOCKED.
- Per-region mandatory evidence set (uniform ×4): listing state probes at start±1 + both walls verbatim; gap row from FULL pagination; two dry-run render windows (region start + cited anchor) + maximal render-stable CODE span with byte cites; inbound census — operand runs for each proposed start (both space-views, delta `−0x1bd0` shown) + region-wide landing check over the recorded edge families + fallthrough test at the west wall (west-neighbor's LAST insn: `0337..033b` thunk tail, `045d`/`040d`-area owners, island `0744` end, H12 `0b11` end — cite each terminator byte; `e9`/`eb` tails do NOT fall through); content classification per render (tenant-data runs vs code islands vs padding) — fills may be MIXED: sub-span each class with cites.
- Flakiness rule (slice 30, binding): `search_byte_patterns` zero-hit exact-form runs are trusted existence-negatives; any positive printed must be read-grounded in the same pass; 1-hit runs cannot ground universals ("no site found in this run" phrasing).
- Create gate (binding, unchanged): create ONLY where a cited reachability ground passes — a static inbound edge (operand landing INSIDE the span, arith shown), fallthrough from a terminating-refuted west wall, or a cited mechanism chain leg that meets the map's OWN current vocabulary (a CITED-SEMANTICALLY mode-dependent leg does NOT close the gate — the create must not claim the mode it cannot pin; mode-independent grounds only, or DYNAMIC-ONLY with the runtime arm itself cited as the writer-with-value into a cell whose READERS are mode-independent... if none, leave). Half-chains/semantic-only ⇒ leave-as-bytes rows citing the gate text. Alignment-dependent heads ⇒ render-disposition rows first, then gate.
- Naming bar: printed-ops mechanism names; IVT/mode-flavored words need in-body staging writes or INT/IRET pairing per the slice-26 rule, AND the slice-30 mode-dependence bar: any "vector/IVT/install" candidate must survive the RM-#UD/PM-IDT question OR name only mode-independent ops; NOT-CONFIRMED-at-name default stands; plate `C: none — behavioral (...)`; direction words banned (R1/R2 class: fills' UNDECIDED status applies to twin-shaped bodies absent pinning evidence).
- Capped write path: real disassembly → ONE nudge (`disassemble_first=false`) → RATIFY verbatim; never create over owned/defined bytes (stop-short cites at every wall: created H1/H4/H5/H9-adjacent owners, islands, thunk `0337`); save ⇒ FULL census protocol (count Δ to write-set exact, gaps full pagination + neighborhood re-derivations, flips named, overlay spots, scope line).
- Scope guard: `[0x2fa]` consumer story, `[0x9ba]`/`[0x9bc/9be/9c2]` runtime, `2cc5/0e3c`, R1/R2 direction, H9 listing gate, name-class route, Δ1 — cite-only; the int-FF CHAIN's mode question stays OPEN (this slice owns the REGIONS, it does NOT re-close the chain — any new evidence gets a chains-table status-update row with cites, not a relabel-by-hope).
- Append-only map (new section `## fill region ownership`); prior sections byte-identical; `fifa96.rep/**` churn unstaged; commits exactly per briefs; no new tests.

---

## Scope Check

Four uniform region packages, one gate, one capped write branch; the chains-map update rows ride in Task 2. Not admitted: closing the int-FF mode-dependence question by assertion, dispatcher runtime story, anything beyond the four regions' walls.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## fill region ownership` (Task 1: ×4 packages + classification + inbound tables + gate-evidence rows; Task 2: `### Writes` + verdicts + chains-update rows + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: creates at gate-passing cited spans; bar renames; `save_program` + census.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Four-region packages + gate evidence (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## fill region ownership` with packages + classifications + inbound tables + gate rows)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: slice-30 fill/landing/third-writer row cites verbatim (arms `03a7/03a9`+`03ab/03b2`, `0462/0464`+`0466/046d`, `075e 8ec0`+`0760`-candidate, landing value `0xb94`, band row `1000:26e2..2792`-class); walls from created neighbors (H1 half ends `045d` ⇒ fill-A east? NO — fill-A is `0360..040d`, its EAST wall is H1 primary head `040e` (`040e..0442` created slice 28 — re-derive live, bodies may be `040e..0442`/`0443..045d`); west walls: thunk `0337..033b` region, `033c` island, band `02da..02f8` tail); live state 335/149/15694 — re-read + quote at-slice time.
- Produces: ×4 packages (`| item | output verbatim |`): listing probes (start±1, walls), gap row, two-render windows + maximal render-stable span(s) per region WITH class sub-spans (CODE/data-tenant/padding; each sub-span byte-cited), inbound table (operand runs per start both-views with arith + region-wide landings enumerated per recorded families + west-wall fallthrough test w/ terminator cite), gate row per region {static-edge found / fallthrough / mechanism (cited, mode-independent? state it) / NONE}, chains-table update rows (writer-open fills → resolved-or-still-open with the new evidence cite; landing-open likewise — status ONLY, no relabel beyond what bytes give). Disposition proposals {create span at S..E w/ entry ground+class, leave, data-define if tenant-pattern}, bar pre-tests for any create candidate (name from THAT span's printed ops only).

- [ ] **Step 1: Listing state ×4**

Probes/gap rows live; quote current owners at all eight walls (thunk/islands/H1-half/H10/H5/H12 bodies etc. — derive, don't trust this line list).

- [ ] **Step 2: Renders + class sub-spans ×4**

Two windows per region + anchor choices stated; maximal stable spans; class each sub-span (the fill tenant story from slice 30: serialized `0x467` values etc. — if bytes show repeated cell-pattern data vs opcode-dense runs, classify w/ cites).

- [ ] **Step 3: Inbound + gate ×4**

Runs + arith + fallthrough tests; apply the flakiness rule; write each gate row explicitly against the create-gate text (mode-dependence of the int-FF consumer leg RE-CITED from slice 30 fix wave where relevant — a region reached only THROUGH that leg fails the gate).

- [ ] **Step 4: Append + commit**

Append `## fill region ownership (verified 2026-09-30, program \`/fifa96.exe\`)` + disposition-so-far line (per region: span, class, gate). Gate: build + ctest → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: four-region fill and landing packages with gate evidence" || true`

---

### Task 2: Capped creates + chains updates

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdicts + chains-update final rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` — creates ONLY where Task-1 gate rows cite a passing ground; bar renames; `save_program` + census if ANY write

**Interfaces:**
- Consumes: Task-1 packages + gate rows + proposals (binding; pre-check per region: live parity re-read, drift ⇒ disposition-only w/ both quotes).
- Produces: `### Writes` — per executed create: before-state (no-function + gap row verbatim), real disassembly, `create_function` (+ ≤1 nudge → RATIFY), post-read-back bounds vs proposal w/ stop-short wall re-cites; per NOT-created: gate-failure citation row. Renames only per bar (printed ops; mode-words survive the slice-30 bar explicitly or default names stand); plate rows. `save_program` (if any write) + FULL census (count Δ = writes exactly; gaps full pagination; all four neighborhoods + every touched wall re-derived; flips named; overlay spots; scope line). Verdict rows ×4 {disposition, entry class (static/fallthrough/DYNAMIC-ONLY w/ mode-independent cite/none), name status}. Chains-update final rows: fill-writer-open legs → new status ONLY if bytes move it (e.g. a created fill body CONTAINING the arm = writer now owned — that is a status fact, not a chain closure; consumer leg stays mode-dependent as recorded). `### Deferrals` — updated: remaining runtime legs, mode question OPEN, R1/R2, `[0x2fa]`, H9 gate, `674c/675a`, name-class, Δ1 + anything new named. Suite green; `grep -c "fill region ownership"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Pre-check parity ×4** (live re-reads vs Task-1 quotes; contradiction ⇒ RATIFY-with-disclosure downgrade)

- [ ] **Step 2: Executes** (gate-passing creates, capped path; NOTHING over walls/defined; alignment-dependent heads get their render rows first — never a create baked from a false head)

- [ ] **Step 3: Save + census + append + commit** — commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: create gate-passing fill bodies with chains-map updates" || true`

---

## Self-Review (ran before save)

- Spec coverage: four named regions uniform packages (T1 Steps 1-3 ← design list); class sub-spans for MIXED fills (T1 Step 2 ← design "content classification"); inbound+fallthrough+gate per region with mode-dependence made gate-explicit (T1 Step 3 + Global create gate ← design's "honest possibility" clause + slice-30 fix-wave bar); creates ONLY gate-passing w/ cap+nudge+RATIFY (T2 ← Global); chains-update rows as STATUS FACTS not relabel-by-hope (T2 ← design scope guard "owns regions, does not close chain"); flakiness rule applied (Global + T1 ← slice 30 binding); save⇒census (T2 Step 3 ← slice-24/29 protocol); bar + direction-word ban for twin-shaped bodies (Global ← slice-26/28 rulings); scope guard relists all carries (Global ← design).
- Placeholder scan: no TBD; gate has explicit pass AND fail branches; chains "status only" rule prevents aspirational wording; anchor/wall lists marked "derive live".
- Type consistency: no C types; addresses match `## arm pairs and R6 band` (arms `03a7/03a9/03ab/03b2`, `0462/0464/0466/046d` w/ bytes, `075e 8ec0`, `0760`-candidate, `0xb94` landing, band `1000:26e2..2792`, twin rows HALF, fix-wave hedge), walls from `## far-return halves` (H1 `040e..0442`/`0443..045d` live-created, H5 `076f..07e6`+half, H10 `0a86..0a9e`), `## 02b7 twin`/`## vector dispatch handlers` (`02da..02f8`, `033c`-island, `073c..0744`, `0ae2..0b11`+`0b0b` edge), `## block head 2978..2a59` (thunk `0337..033b`), delta `−0x1bd0`, state 335/149/15694; Task 2 consumes Task-1 rows by heading; commit messages distinct; Task 1 zero writes.

(End of file)
