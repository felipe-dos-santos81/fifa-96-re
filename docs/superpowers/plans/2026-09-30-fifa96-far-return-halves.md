# FIFA96 Far-Return Halves Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Attribute the six far-return half-blocks slice 22 left as DEVIATION→RATIFIED records — the blocks beyond each handler primary's first cited exit that "a landing create cannot statically absorb": H1 `11bd:0443..045d`, H3 `0665..0674`, H4 `06ca..06fb`, H5 `07b9..07e6`, H9 `0a35..0a5d`, H10 `0a86..0a9e` (ranges from slice-22's relabel rows — re-derive live; three of them, `1000:2235..2266`/`2605..262d`/`80cf..8118`-class, moved undefined→DEFINED-UNOWNED by the slice-23 save-sweep and were ratified as status-quo in slice 24; verify current ownership per block before claiming anything). Walk each half from its byte-boundary walls to its cited terminator, establish the entry-leg class (far-return landing ⇒ DYNAMIC-ONLY per the block-head precedent of slice 23: "if statically unreachable, entry = DYNAMIC-ONLY, cited" — while still running the authority entry census for every half), and create the six bodies at cited bounds in Task 2 with stop-short discipline at every owned wall.

**Architecture:** Read-only per-half walk + entry census + wall cites + naming-bar pre-tests (Task 1, zero writes), then capped creates at the six cited ranges (+ bar-gated renames; `save_program` + full slice-24 census protocol) (Task 2). Prior sections referenced, never rewritten. No C, no tests, no CMake, no tools.

**Tech Stack:** Ghidra-MCP (`get_function_by_address`, `disassemble_function`, `disassemble_bytes` (Task 1 `dry_run=true`), `read_memory` (hex-vs-data protocol), `search_instructions` (authority for entry runs — includes `cb`/`ca` far-return and `ea`-landing analysis; operand runs per proposed start), `find_code_gaps` (full pagination), `get_xrefs_to` (control only), Task 2: real `disassemble_bytes`, `create_function`, `rename_function`, `set_comment`, `save_program`), `grep`, CTest (regression gate only).

**Spec:** Approved chat proposal 2026-09-30 ("Far-return halves — the 6 blocked pair-consumer blocks (`0443..045d` etc.) + `674c/675a` dive; bigger, closes slice-22's last structural deferral", order approved by "yes") + `docs/ghidra/loader_rename_map.md` (`## vector dispatch handlers` — the six DEVIATION→RATIFIED relabel rows with re-quoted Task-1 boundary proposals + "shared far-ret `**` note / one-hop deferral pointer", the `0974 f3f0` H7 split deferral and `0976..099a` remainder (family-adjacent — NOT admitted this slice), handler primaries' live bounds as the inner walls; `## sweep-aftermath ratification` — the three halves' undefined→defined-unowned flip rows (drift census (4)); `## block head 2978..2a59` — DYNAMIC-ONLY entry precedent (`2a5a` block entry created with cited statics-zero + runtime arm leg), capped-write format; `## vector selection logic` — scan-scope + entry-run formats; `## R3 IVT cluster` — the `675a`/`674c` consumer note (H4-adjacency? verify segment: `674c` is NOT one of the six — carried, not admitted) and two-render discipline rows).

## Global Constraints

- No C, test, CMake, or `tools/` changes; suite stays 10/10 untouched.
- Never write to `/media/felipe/FIFAPCCD/` or `game/FIFAPCCD96.iso`.
- No address/byte/algorithm claim without an instruction/bytes citation; disassembly wins over the decompiler; every `read_memory` quote reconciles `hex` vs `data` before verbatim.
- Task 1 writes NOTHING: dry-run disassembly only; reads only; tool error on required sub-step → BLOCKED.
- Per-half evidence set is mandatory and uniform: (a) current ownership (`get_function_by_address` at block start + both walls verbatim — an "already owned" result RECLASSIFIES that half: no duplicate create, disposition row instead); (b) two-render check (span-start window + anchor window) per the `173b`/`4c94`/strip precedent before any CODE claim; (c) entry census: operand runs for each proposed start address (both `11bd:`-class and `1000:`-space renders where tools report that space) with per-hit `nextIP+rel` arithmetic + fallthrough check at the inner wall (primary's LAST insn — cite its terminator byte: if the primary's body-end insn does NOT terminate flow, fallthrough-into-half is an OWNERSHIP question for the analyzer, record it as such, do not create over it); (d) terminator cite at the outer wall (`c3`/`cb`/`ca`/`f4`/tail-`e9` as found).
- Capped write path: real disassembly → at most ONE nudge (`disassemble_first=false`) → RATIFY on refusal, verbatim before/after; never create over owned or defined bytes; stop-short citations at every wall (last-owned + first-foreign); if the analyzer auto-absorbs or splits differently, RATIFY-with-disclosure + the slice-24 drift-census protocol (count Δ, gaps FULL pagination total + row re-derivation, side-effect ledger naming every unexpected create/split incl. `1991:` overlay membership check).
- Naming bar: verb-led snake_case from the half-body's OWN cited ops; "return"-class words admissible ONLY with a cited `cb`/`ca` (RET/near-far) terminator in that body; "far"-class claims additionally require the body's stack-frame ops cited (segment-pop before the return); mechanism-weak bodies keep default names NOT-CONFIRMED-at-name; plate `C: none — behavioral (<role>)`; direction words still banned; no `decode_*`.
- Entry verdict vocabulary: DYNAMIC-ONLY requires the cited statics-zero run set + the runtime reachability mechanism named by reference to existing map records (far-call pairing from slice 22's H-rows: which primary's `f4`/exit the half returns-to, cited — a pairing CLAIM without its bytes cited is a hypothesis row, mark it such).
- Scope guard (closes slice 22's LAST structural deferral only): H7's `0976..099a` remainder, `02da..02f8`/`0976..099a`/`6328..634e` orphan FAMILY question, `674c/675a` dive, `[0x56]/[0x58]` stories, `2cc5`/`0e3c` unowned fragments, runtime writers, name-class route, Δ1 — all cite-only deferrals; no handler primary re-walks beyond wall cites.
- Append-only map (new section `## far-return halves`); prior sections byte-identical; leave `fifa96.rep/**` churn unstaged; commit messages exactly per briefs; no new tests.

---

## Scope Check

Six uniform evidence packages + six capped creates; the "one big structural deferral left from slice 22" framing means anything beyond the six (family question, H7 remainder) is explicitly not admitted. Task 2's creates are each individually capped/nudge/ratify; the whole branch is reviewable as one artifact.

## File Structure

- Modify: `docs/ghidra/loader_rename_map.md` — append `## far-return halves` (Task 1: per-half tables (walls/renders/entry/terminators/pairing hypotheses) + disposition proposals; Task 2: `### Writes` (6 capped creates ± nudges) + verdict rows + `### Deferrals`).
- Modify: Ghidra program `/fifa96.exe` — Task 2 only: up to 6 creates at cited ranges + bar-gated renames; `save_program` + census protocol.
- `src/`, `tests/`, `CMakeLists.txt`, `include/`, `tools/` — UNTOUCHED.

---

### Task 1: Six-half evidence (no writes)

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `## far-return halves` with per-half evidence tables + proposals)
- Modify: none in Ghidra (read-only pass)

**Interfaces:**
- Consumes: slice-22's six relabel rows (re-quoted boundary proposals + primary created ranges); slice-24 drift-census flip rows for the three now-defined-unowned halves; primary bodies' LAST-insn bytes (inner-wall fallthrough test); the `**` shared far-ret note + one-hop deferral pointer from slice 22; DYNAMIC-ONLY precedent row format from `## block head 2978..2a59`.
- Produces: per-half table ×6 (`| item | output (verbatim) |`): ownership probes at start±1 + both walls; two-window render pair + the maximal render-stable CODE span (may differ from slice-22's proposed range — record the live span, that's what Task 2 creates); entry census runs (pattern+match_count+scope) with per-hit arith + fallthrough verdict from the primary's terminator cite; outer-wall terminator; pairing hypothesis row (`| half | returns-to primary | mechanism cited (which exit insn bytes of the primary + which frame op of the half) | CLASS: cited-pair / hypothesis |`); bar pre-test row per half (candidate name + the exact ops it rests on, or NOT-CONFIRMED-at-name). Disposition proposal per half: {create at cited span (default-named or bar-passed name), already-owned→disposition-only, render-dependent→leave-as-bytes} — six rows, each naming its evidence lines.

- [ ] **Step 1: Ownership + walls ×6**

Probes per Global (a): starts (`0443`, `0665`, `06ca`, `07b9`, `0a35`, `0a86`) + wall neighbors (`0442`-side owner end, `045e`-side next owner start, etc. — derive each live, the six neighbors are the primaries and their successors from slice 22's rows).

- [ ] **Step 2: Renders + spans ×6**

Two dry-run windows per half; maximal render-stable CODE span per half with byte cites; compare vs slice-22 proposals — deltas are the finding, not the surprise.

- [ ] **Step 3: Entry + fallthrough + terminators ×6**

Operand runs both space-views per proposed start; primary terminator bytes (`f4`? `c3`? — cite each; a non-terminating end reclassifies the inner boundary as an analyzer-absorption question, record); half's own terminator at outer wall.

- [ ] **Step 4: Pairing + bar pre-tests + append + commit**

Pairing rows CLASS-tagged (cited-pair needs primary-exit BYTES + half frame ops); bar candidates printed as pre-tests only. Append `## far-return halves (verified 2026-09-30, program \`/fifa96.exe\`)` — six blocks + disposition proposal rows. Gate: `cmake -S . -B build && cmake --build build && ctest --test-dir build` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: six-half wall render entry and pairing evidence" || true`

---

### Task 2: Capped creates + verdicts

**Files:**
- Modify: `docs/ghidra/loader_rename_map.md` (append `### Writes` + verdict rows + `### Deferrals`)
- Modify: Ghidra program `/fifa96.exe` — creates per Task-1 proposals at cited spans (≤6), renames only at bar, `save_program` + census protocol

**Interfaces:**
- Consumes: Task-1 six evidence blocks + dispositions (binding unless a pre-write live re-check contradicts → RATIFY-with-disclosure per half).
- Produces: `### Writes` — per executed create: before-state (`get_function_by_address` no-function + gap row verbatim), real disassembly, `create_function` (+ ≤1 nudge), verbatim response, post-read-back (bounds vs proposal; auto-extended/split ⇒ RATIFY + disclose with the foreign-byte cites), rename rows (bar pass → `rename_function` + plate verbatim before/after; fail → NOT-CONFIRMED-at-name rows); per-half "not created" rows where Task-1 proposed disposition-only (already-owned/render-dependent), with cites. Then `save_program` + FULL census protocol: count Δ (expect +≤6), gaps total via full pagination (row re-derivation for all six neighborhoods), side-effect ledger (unexpected creates/splits incl `1991:` overlay membership + the three flip-rows' new state), scan-scope drift line (slice-24 errata format). Verdict rows: six × {entry class DYNAMIC-ONLY or found-static-with-cite, pairing CLASS carried or upgraded, naming disposition}. `### Deferrals` — family question, H7 remainder, `674c/675a`, `2cc5`/`0e3c`, cell stories, runtime writers, name-class route, Δ1 + any new legs. Suite green; `grep -c "far-return halves"` nonzero; prior rows byte-identical.

- [ ] **Step 1: Pre-write re-check per half**

Ownership + render-stability + wall bytes live re-read (a half whose state moved since Task 1 → downgrade to disposition-only row with both quotes; never create over new ownership).

- [ ] **Step 2: Creates (capped) + bar renames**

Per admitted half: the capped sequence; stop-short at walls citing last-owned/first-foreign; nudge rule per cap.

- [ ] **Step 3: Save + census + append + commit**

`save_program`; census protocol in full (slice-24 procedure, incl. overlay spot-checks); append `### Writes`/verdicts/`### Deferrals` (+ `### Fix wave` trailer if needed). Gate: build + `ctest` → 10/10. Commit exactly: `git add docs/ghidra/loader_rename_map.md && git commit -m "docs: create six far-return half bodies with pairing verdicts" || true`

---

## Self-Review (ran before save)

- Spec coverage: six uniform evidence packages (T1 Steps 1-4 ← Global's mandatory set + design "walk each half from its walls to its cited terminator"); entry-leg class with authority runs (T1 Step 3 ← design "still running the authority entry census"); fallthrough-reclassification rule (T1 Step 3 + T2 Step 1 ← Global (c)); per-half dispositions incl. already-owned/render-dependent branches (T1 Step 4 + T2 ← design "create at cited bounds in Task 2" + its cap rules); DYNAMIC-ONLY precedent + pairing CLAIM-honesty (T1 Step 4 bar/pairing rows ← design precedent cite); save-sweep census protocol (T2 Step 3 ← slice-23/24 lessons, the six creates are exactly the trigger class); scope guard rows (Global + T2 Deferrals ← design's closes-slice-22 framing; family question explicitly NOT admitted).
- Placeholder scan: no TBD; all three per-half branches (create/disposition-only/downgrade) fully specified; bar vocabulary per-class.
- Type consistency: no C types; the six ranges (`0443..045d`, `0665..0674`, `06ca..06fb`, `07b9..07e6`, `0a35..0a5d`, `0a86..0a9e`) and drift rows (`2235..2266`, `2605..262d`, `80cf..8118`) match `## vector dispatch handlers` relabel rows + `## sweep-aftermath ratification` drift census (2235−0x1bd0=0665 ✓, 2605−0x1bd0=0a35 ✓, 80cf−0x1bd0=64ff ✗ — third flip-row's start maps OUTSIDE the six: Task 1 Step 1 must re-derive its true 11bd extent from the live row instead of assuming, and if it belongs to no half, record it as a scope-check finding); terminators `f4/c3/cb/ca/ea/e9`, walls `0929/0938/0a34/0a85`-family primaries per slice-22; delta `−0x1bd0`; Task 2 consumes Task-1 rows by heading; commit messages distinct; Task 1 zero writes.

(End of file)
