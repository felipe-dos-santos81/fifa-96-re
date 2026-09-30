# FIFA96 Arm Cell Pairs and R6 Band Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the two entangled deferrals of slice 29: (1) the far-pointer arm-cell pairs — full writer∪reader census of `{[0x467], [0x469]}` (R4's pair: `64d3 MOV [0x469],CS`-class + `64d7 MOV [0x467],0x6506`, DS-default) and `{[0x3fc]}` (+ its natural pair-mate, derived live) as written by `09fc MOV ES:[0x3fc],0xa35`-class from the H9 story, PLUS the R6 band's own arm value `CS:0xb94` target-cell: for every pair, enumerate ALL writers with values (image constants vs runtime legs), ALL readers/consumers (`ff26/ff2e` indirect forms + `ea` far-pointer forms with segment context per the slice-29 segment-physicality ruling), and build the arm→cell→consumer→landing map: which mode/arg values cause which code to store which far-ptr where, and which tail jumps consume it — the dispatcher-story's LAST statically-open leg that is statically answerable; (2) the R6 CODE band `11bd:0675..0696` ownership: two-render head analysis at `0675` (aligned? the flip row is `1000:2245..2266` size 34 undefined — verify current state), inbound census (every arm whose VALUE `0x675`/`75 06`-class words lands in tables/cells — search both byte-patterns and the 14-arg determinability appendix re-scan for `0x675`-target arms: does any arg store point AT `0675`?), exit/terminator cites, mechanism verdict per the create gate (cited reachability ⇒ capped create, else leave), and the R4 head stub `64ff..6505` (7 B — walk its two renders, classify CODE/data, disposition).

**Architecture:** Read-only census + map-building (Task 1, zero writes), then capped dispositions (create R6-band and/or R4-head stub only per mechanism rule; bar renames; save + full census protocol) (Task 2). Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true`), `read_memory` (hex-vs-data protocol), `search_instructions` (authority — operand + mnemonic+operand runs), `search_byte_patterns` (value-word patterns `06 65`, `75 06`, `94 0b`, cell-disp forms `67 04`/`69 04`/`fc 03`), `find_code_gaps` (full pagination), `get_xrefs_to` (control only), Task 2: real `disassemble_bytes`, `create_function`, `rename_function`, `set_comment`, `save_program` + slice-24/29 census protocol), `grep`, CTest (regression gate only).

**Spec:** Approved chat design 2026-09-30 (post-slice-29: candidates (a) R6 CODE-band ownership story and (b) `[0x467]/[0x469]` pair-consumer + segment-physicality — merged into one slice because R6's own arm-writer op IS a pair-writer; user "yes") + `docs/ghidra/loader_rename_map.md` (`## orphan family` — R4 create row `restore_ss_sp_and_modify_pic_masks` `6506..6548` + mechanism cites `64d3/64d7` + caveat row (DS-default vs ES arms differ physically) + consumer deferral line; R6 not-created row with the DATA→CODE supersession quote + `0684 = 26c7066704940b` arm bytes + `ES:[0x467],0xb94` value; R5/H9 arm `09fc MOV ES:[0x3fc],0xa35` `26c706fc03350a`; flip rows `80cf..80d5` (R4 head-stub span) and `2245..2266`; head-stub `64ff..6505` left-unowned note; family MIXED answer row), (`## far-return halves` — the six pairing-store regions `042c`/`05bf..05c6`/`06ae..06b2`/`078b..0795`/`09fc`/`0a6a..0a74` + ES←0x38 caveat; H9 relabel; surplus heritage), (`## vector dispatch handlers` — H-family primaries' tails `f4`/`ebfe`/`ebf5` exit forms (who far-RETs INTO a half: the `cb`/`ca` consumers), arg-cells `0675..0696` original DATA claim now superseded), (`## callee arg question` — determinability appendix: the 14 `[BP+-0x5a]` args and where each pair-table word lands — re-scan for `0x675`/`0xb94`-class values), (`## 2811..296c pocket + 9bc vector` — cell-pair forms precedent `CS:[BX-4]/[BX-2]`, `[0x9bc]/[0x9be]`, `0x296d` arm), (`## block head 2978..2a59` — R1 tail `e97fd9`→relay `0337`, `2eff26fa02 JMPF [0x2fa]`-class consumer precedent).

## Global Constraints

- No C, test, CMake, or `tools/` changes; suite stays 10/10 untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data`; hex-vs-data protocol on every pattern quote.
- Task 1 writes NOTHING: dry-run only; reads only; tool error on required sub-step → BLOCKED.
- Census uniformity: every run = pattern + `match_count` + `scope` + `truncated` + "defined-insn-only, at this-slice time" on negatives; controls (`get_xrefs_to`/listing) quoted not relied; segment context per site (DS-default/ES←0x38/ES←0/CS) — the slice-29 segment-physicality rule: same nominal cell offset under different staging segment = DIFFERENT physical cell; every writer/reader row carries its effective segment basis.
- Value-word searches: two-byte little-endian patterns (`67 04`-style disp forms vs value forms `75 06`/`06 65`/`94 0b`) carry false-positive classes (string/imm/data hits) — classify each hit (operand-disp / operand-value / containing-insn imm / data-table word / string) with the render or read; raw `search_byte_patterns` results marked; the quote-protocol reconciliation on every quoted byte string.
- Create admission (binding from slice-29 merged ruling): cited runtime-reachability mechanism (arm→cell→consumer chain landing IN the region, or pair-table word resolving into it, or a cited inbound static edge) ⇒ create with entry class DYNAMIC-ONLY (or found-static); NONE ⇒ leave-as-bytes; alignment-dependent heads ⇒ render-disposition rows only; mechanism chains may end at an OPEN-WINDOW leg (writer exists, value runtime) — then the region is NOT created and the chain is recorded with the gap named (a half-chain is not a mechanism).
- Capped write path: real disassembly → ONE nudge → RATIFY, verbatim before/after; stop-short at walls; never over defined/owned; save ⇒ FULL census protocol (count Δ to write-set exact, gaps full pagination + all touched neighborhoods, flips named, overlay spot, scope line).
- Naming bar: printed-ops mechanism names only (return-class needs cited terminator op; PIC/segment-class words need their literal port/segment ops cited); NOT-CONFIRMED-at-name default stands; plate `C: none — behavioral (<role>)`; direction words banned.
- Scope guard: `[0x2fa]`/`[0x9ba]`/`[0x9bc]/[0x9be]/[0x9c2]` full stories, `674c/675a`, `2cc5/0e3c`, R1/R2 direction, R3 stub, name-class route, Δ1 — cite-only; runtime values stay OPEN-WINDOW rows; the four H-family pairing-store regions are read as mechanism sources, not re-walked.
- Append-only map (new section `## arm pairs and R6 band`); prior sections byte-identical; `fifa96.rep/**` churn unstaged; commits exactly per briefs; no new tests.

---

## Scope Check

Two entangled questions (pair census + R6/head-stub dispositions) sharing the arm→cell→consumer machinery; Task 2's create branches are mechanism-gated and individually enumerated (create/leave/half-chain). Not admitted: the full dispatcher runtime story (FU-blocked), H9's misaligned head (listing-blocked, closed as leave in slice 29 — only NEW evidence reopens it), orphan tails R1/R2.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## arm pairs and R6 band` (Task 1: pair tables + arm→cell→consumer→landing map + R6/head-stub packages + proposals; Task 2: `### Writes` + verdict rows + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: ≤2 capped creates (R6 band, R4 head stub) + bar renames; `save_program` + census.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Pair census + map + region packages (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## arm pairs and R6 band` with census tables + map + proposals)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: cell addresses/arms from `## orphan family` (`64d3`=`8c0e6904`, `64d7`=`c70667040665`, `09fc`=`26c706fc03350a`, `0684`=`26c7066704940b`); H-family pairing-store regions + ES←0x38 caveat; determinability appendix (14 args) for value re-scan; flip rows `80cf..80d5`/`2245..2266` current live state; relay precedent `2eff26fa02` (the `[0x2fa]` consumer form class — as a pattern TEMPLATE for consumer searches, story stays deferred); live state (count 335, gaps 149, scope 15694 — re-read).
- Produces: pair census tables ×3 (`| form (pattern/operand) | run output verbatim | site (owner?) | effective segment basis | class W/R/false | value if visible |`): `{[0x467],[0x469]}` writers (image values incl `0x6506`, plus any others), readers (`ff16/ff26/ea`-class operand runs + disp forms BOTH segment-contexts — who tail-jumps through the pair, landing cited by arith), pair-mate derivation for `[0x3fc]` (is `[0x3fe]` its CS half? `09fc` wrote `ES:[0x3fc],0xa35` — value `0xa35` = the H9 half head! verify whether any consumer lands at `0a35`-class via the pair; the H9 leave gets its mechanism answer here — chain-complete-or-half-named); R6 band package (two-render at `0675`, current gap row verbatim, inbound: value-word runs `75 06`-class + appendix re-scan for an arm value equal to `0x675`/`675`-class + region-wide landing check per orphan-family Step-2 method, exit cites, mechanism row {complete/none}); R4 head-stub package (`64ff..6505`: two renders — the earlier W-A showed `64ff..6500` SKIPPED + `6501/6503/6504` stub insns: classify each render's stability + inbound (does any cell/table value `0x64ff`-class land there? the `ea`/far-ptr word patterns `ff64`-class already cited as JMPF `64fa`... careful: value `64ff` vs JMPF — re-derive) + mechanism); arm→cell→consumer→landing MAP table (the spine artifact: one row per chain with every link cited, statuses: complete / consumer-open / writer-open / half). Proposals per region {create w/ chain cite, leave, half-chain-leave} + bar pre-tests.

- [ ] **Step 1: Pair tables**

Runs per Interfaces for `{0x467,0x469}`, `{0x3fc,(0x3fe)}`, with segment bases; values reconciled (`0x6506`, `CS`, `0xa35`); consumers arith-checked to landing addresses (does a `ff26 [0x467]`-class reader exist at all? if NONE in defined insns, that IS the finding: R4's half is reached only via a reader outside the defined layer — classify which class could read it: `ea`-literal? self-modifying? name the OPEN leg).

- [ ] **Step 2: R6 package**

Render pair; gap row; value-word runs (`75 06` classified per hit — operand/table/string/imm); appendix re-scan (grep the map's own appendix table for `0x675`/`0x679`-adjacent — the arg `0x679` story vs the band `0675..0696`: is the band's `0684` arm-writer INSIDE a region any arg names? derive); landing check region-wide (targets into `0675..0696`); exit/terminator; mechanism row.

- [ ] **Step 3: R4 head-stub package**

`64ff..6505` renders (both windows + `analyze_data_region`-read on any defined unit); inbound value runs (`ff 64`/`64ff`-class words); stability/class; mechanism row.

- [ ] **Step 4: Map + proposals + append + commit**

Assemble the chains table; proposals + bar pre-tests. Append `## arm pairs and R6 band (verified 2026-09-30, program \`/fifa96.exe\`)` + disposition-so-far line (chains complete count, R6 answer, stub answer). Gate: build + ctest → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: census arm cell pairs, build chains map, package R6 band and head stub" || true`

---

### Task 2: Mechanism-gated dispositions (capped)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdict rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` — creates ONLY where Task-1 chain is COMPLETE (≤2), bar renames, `save_program` + census

**Interfaces:**
- Consumes: Task-1 tables + proposals + chains map (binding; pre-check per region live).
- Produces: `### Writes` — per action before/command/after verbatim (capped path), or not-created rows citing the chain status; census protocol in full (Δ-to-write-set; touched neighborhoods `2245..2266`/`80cf..80d5`-family + any flip changes named; overlay spot; scope line). Verdict rows: R6 {disposition, entry class, chain cite}, R4-head {same}, pair chains final {per-pair: complete/consumer-open/writer-open with the named leg}. `### Deferrals` — updated: consumer-open legs (runtime), H9 chain if newly COMPLETE carry the listing-block note (chain completion does NOT auto-create over the Alignment unit — gate stays), remaining list. Suite green; `grep -c "arm pairs and R6 band"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Pre-write parity per region** (live re-check; drift ⇒ disposition-only with both quotes)

- [ ] **Step 2: Executes** — capped creates where chains complete + aligned/defined-safe heads (NEVER over the `2235..2236`/`0665` Alignment precedent respected — R6 head `0675`: if render-stable and aligned, create at cited span with stop-short walls `0674` (owner `FUN_11bd_0667` end) / `0697` owner; head stub: same rule); bar renames per pre-tests.

- [ ] **Step 3: Save + census + append + commit** — `save_program` if any write; full protocol; append rows. Gate. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: dispose R6 band and head stub per chain mechanism" || true`

---

## Self-Review (ran before save)

- Spec coverage: pair census BOTH segment-bases + consumer arith to landings (T1 Step 1 ← design "enumerate ALL writers... ALL readers... arm→cell→consumer→landing map"); `[0x3fc]` pair-mate + `0xa35`-landing re-open of the H9 chain with the Alignment-gate preserved (T1 Step 1 + T2 ← design goal (1) + slice-29 close; chain≠auto-create rule explicit); R6 ownership: two-render + value-word inbound + appendix re-scan + landing check + mechanism per create-gate (T1 Step 2 + T2 ← design goal (2)); R4 head stub (T1 Step 3 ← design "(64ff..6505, walk, classify, disposition)"); half-chains named not guessed (Global mechanism rule ← slice-29 ruling "a half-chain is not a mechanism"); segment-physicality per-site (Global + T1 ← slice-29 caveat); capped ≤2 + census protocol (T2 ← Global); deferrals updated (T2 ← scope guard list).
- Placeholder scan: no TBD; both create outcomes + both leaves + half-chain branch enumerated; consumer-NONE outcome has its classification requirement (name the class that could read it) not just "none".
- Type consistency: no C types; cell addresses (`0x467/0x469`, `0x3fc/0x3fe`), arm bytes (`8c0e6904`, `c70667040665`, `26c706fc03350a`, `26c7066704940b`), values (`0x6506`, `CS`, `0xa35`, `0xb94`), regions (`0675..0696`, `64ff..6505`, flip rows `2245..2266`/`80cf..80d5`), walls (`0667..0674`, `0697`, `6506`, `64b7..64fe`), pairing-stores six regions, appendix 14-args, relay `2eff26fa02`, count 335/gaps 149/scope 15694 — all match `## orphan family`, `## far-return halves`, `## 2811..296c pocket + 9bc vector`, `## callee arg question`; Task 2 consumes Task-1 rows by heading; commit messages distinct; Task 1 zero writes.

(End of file)
