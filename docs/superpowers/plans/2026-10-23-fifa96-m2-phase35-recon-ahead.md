# FIFA 96 M2 phase-35 recon-ahead plan (FU330–FU333)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce four frozen evidence docs — FU330–FU333 — that settle the native placement-limit producer (`+0x79`, FU328-L1) and its `+0x7B` staging (FU328-L2): the fresh native census, the port audit + producer design, the measured whole-formation/M2 blast radius, and the acceptance/governance for the natural no-slot pan — with a GO/NO-GO on the undriven witness.

**Architecture:** Four independent read-only slices, one agent each. FU330 is a fresh-Ghidra static slice; FU331 is the port audit + design (repo-static); FU332 measures with an out-of-tree patched build; FU333 is the acceptance/governance design. Drafts land under `docs/ghidra/`, receive a whole-wave review + quoted errata, and are promoted one-per-commit. No production code, no test/golden change.

**Tech Stack:** Ghidra MCP (standard tools only), the repo's C sources + docs, out-of-tree probes linking the prebuilt `build/libfifa96_*.a` libraries.

**Spec:** `docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md`

## Global Constraints

- **Read-only wave (tasks 1–4):** no repo source/test/golden change; the only repo writes are the four draft docs. Probes/builds under `/tmp/opencode/fu33N/`; the ISO is read-only. Tasks 1–4 run no git commands and make no commits — task 5 is the controller's promotion task.
- **Ghidra (FU330, and any slice that consults it):** the program argument is exactly `/FIFA96.EXE`; LE-image globals sit at `+0x100000`; scripts are disabled; standard MCP tools only; caller counts need a fresh `get_xrefs_to` in the same session; verify CALL bytes before trusting a target; the alias traps (`dword[addr]>>16` = the word at `addr+2`, `>>24` = the byte at `addr+3`; sign/width errors; gate-index misreads).
- **Carried vs fresh:** the phase-34 chain (the `+0x7B = min(([+0x63]>>22)+1, word[+0x79])` counter, the stride/seed/v0/T chain, the diagnostic) is frozen authority in FU326–FU328 — do not re-derive it. The `+0x79` producer (`FUN_0007997C`, the `0x10F394` table, the `[rec+0x9D]` OR, the values/timing) is **fresh** (FU330): the wave flagged "6..15 is not a proven cap".
- **Evidence rule:** every address/byte claim cites a fresh disassembly/raw-byte read or a reconciled `file:line`; anything not statically provable becomes a **numbered leg** with its native owner — never asserted, never faked. Prior doc sections are never rewritten.
- **Doc format:** single H1 `# FU-33N — … (phase-35 R2-L9)`; `**Phase/Provenance:**` line naming the spec §s (and the out-of-tree artifacts); the question; the one-page answer; the evidence table; the design/measured sections; numbered legs `FU33N-L…`; GO/NO-GO verdict; known limits; self-audit.
- **Goldens:** M1 `09b726b7…` immutable; M2 `d5446144…` is the current pin — the wave may **not** re-pin; the patched probe's output is evidence only.
- **Scope:** the census holds `60/80`; the out-of-slice items (FU324-L5's re-measure clause, FU326-L1, FU328-L4-L7, the claim/carrier fusion, the armer/follow, FU319-L8) stay out.

## Review Focus

1. **The producer's identity** — a slice that confuses `+0x79` (the placement limit) with `+0x7B` (the counter), the `0x10F394` table byte with the `[rec+0x9D]` OR, or the per-record kickoff value with the per-frame store.
2. **The row-09 interplay** — the OL-82 `timer7b = 0` assumption and the row-09 velocity staging: a producer that changes `timer7b` without auditing the row-09 consumer is a fidelity bug, not a win.
3. **Probe hygiene** (FU332) — an out-of-tree build only; the build line published; the unpatched probe reproducing the golden `d5446144…`; the repo and the ISO untouched.
4. **The blast radius honesty** — the whole-formation movement and the M2 diff measured, not extrapolated; a first-diff frame attributed to the producer's chain; the strata and `state=` suffixes checked.
5. **The acceptance traps** — `lane = 0`, `v0 = 0`, a zero place delta, or a staged cell in a "natural" witness; the A3 re-pin's measured pacing.

---

### Task 1: FU330 — the native `+0x79` producer census

**Files:**
- Create: `docs/ghidra/FU330_r2l9_placement_timer.md`

**Interfaces:**
- Consumes: the spec §3/§4; FU84 §6 (carried); FU328 §4/§5/L1-L3 + its erratum 2.
- Produces: the producer's instruction-level census (the table, the OR, the values, the timing, the kickoff values per row/state), the value semantics, GO/NO-GO; legs `FU330-L1…`.

- [ ] **Step 1: Re-read `FUN_0007997C` fresh.** In `/FIFA96.EXE` (Ghidra MCP): the full function (the `0x799F0..0x79A4D` body and its callers — fresh `get_xrefs_to`); the exact store to `+0x79`, the table lookup, the `[rec+0x9D]` OR (OL-72), and the store's timing in the frame chain.
- [ ] **Step 2: The table.** Read the `0x10F394` table (raw bytes + the row/state indexing); enumerate the values per row/state; confirm or correct the "6..15" seen range; the OR's per-record overrides.
- [ ] **Step 3: The readers.** Every reader of `+0x79` (esp. the `+0x7B` counter's `min(...)`) and the per-record values at kickoff for all rows/teams.
- [ ] **Step 4: Write the doc** in the house format: the census table, the semantics, the values/timing, legs, GO/NO-GO.
- [ ] **Step 5: Self-audit** every claim against a fresh read; unproven → a numbered leg.

---

### Task 2: FU331 — the port audit and the producer design

**Files:**
- Create: `docs/ghidra/FU331_r2l9_producer_design.md`

**Interfaces:**
- Consumes: the spec §3/§4/§8; FU330's census (or the carried FU328 chain if parallel — forward-cite); the port's `timer79`/`timer7b` sites.
- Produces: the port audit (every site, the OL-82 assumption, the row-09 interplay), the exact producer design, the RED/GREEN plan, GO/NO-GO; legs `FU331-L1…`.

- [ ] **Step 1: Audit every `timer79`/`timer7b` site** (`include/fifa96_engine/fifa96_match_entities.h:129`; `src/fifa96_engine/fifa96_match_entities.c:94/209/744`; `src/fifa96_engine/fifa96_match_handlers.c:497/737/749/848/866/2344/4861/5034`; any others fresh); classify: the producer's target, the copies, the consumers, the row-09 velocity staging, the OL-82 assumption.
- [ ] **Step 2: The row-09 interplay.** What the row-09 staging does with `timer7b` (the `vel73/vel75` computation) and what a live `+0x79` does to it; the native side (FU330); the OL-82 correction.
- [ ] **Step 3: The producer design.** The exact cell, the writer site(s) and timing, the values (from FU330), the interaction with the `+0x7B` reset (`entities.c:94`), and the FU328-L1/L2/L3/L8 mapping; the RED/GREEN unit pins (the producer, the counter, the row-09 consumer).
- [ ] **Step 4: Write the doc** in the house format: the audit table, the design, the pins, legs, GO/NO-GO.
- [ ] **Step 5: Self-audit.**

---

### Task 3: FU332 — the measured blast radius

**Files:**
- Create: `docs/ghidra/FU332_r2l9_blast_radius.md`

**Interfaces:**
- Consumes: the spec §3/§6; FU330's census; FU331's design (forward-cite if parallel); the FU328 §5.4 diagnostic; the phase-32/33 golden recipe.
- Produces: the measured whole-formation movement, the M2 diff (first-diff, size, `state=`, strata), the natural band entry/pan under the producer, the frequency, the golden exposure; legs `FU332-L1…`.

- [ ] **Step 1: Build the patched out-of-tree probe.** Apply the producer (per FU331's design / FU330's values) in `/tmp/opencode/fu332/`; the unpatched probe must reproduce the golden `d5446144…`; publish the build line and the patch.
- [ ] **Step 2: Measure the whole-formation movement.** Every record's `+0x79`/`+0x7B`/stride/attr/velocity over the natural window; the first frame any of them moves; the row-09 staging effect.
- [ ] **Step 3: Measure the M2 diff.** The first differing frame, the moved-line count, the `state=` suffixes, the strata; the attribution of the first diff to the producer's chain.
- [ ] **Step 4: Measure the natural pan.** The natural band entry under the producer: the gate tuple, `has_slot`, the seed, `v0`, the event, the place delta `T(s)`, the pan; the silent bind's status; the frequency.
- [ ] **Step 5: Write the doc** in the house format: the movement table, the diff accounting, the pan result, legs, GO/NO-GO, known limits.
- [ ] **Step 6: Re-run once** for stability.

---

### Task 4: FU333 — the acceptance and governance

**Files:**
- Create: `docs/ghidra/FU333_r2l9_acceptance.md`

**Interfaces:**
- Consumes: the spec §5–§8; FU329 Branch B (`docs/ghidra/FU329_r2l8_acceptance.md`); FU332's measured numbers (forward-cite); the phase-33 golden governance (the stratum-set gate, the faithful-move).
- Produces: the Branch B concretized, the A3 re-pin, the Layer-2, the faithful-move recipe, the hazards, GO/NO-GO; legs `FU333-L1…`.

- [ ] **Step 1: Concretize the witness.** The exact tuple/pins from FU332 (or the designed slots if NO-GO); the no-staged-cell rule; the drive naming.
- [ ] **Step 2: The A3 re-pin.** The phase-33 inversion's window constants (`tests/test_engine_m2.c:3972-3979`) with the measured pacing shift; the exact values from FU332.
- [ ] **Step 3: The golden recipe.** The faithful-move procedure (the first-diff, the `state=` check, the stratum-set gate, M1 `cmp`, the written reason), the tripwires, the P1/P2 commit split.
- [ ] **Step 4: The hazards.** The row-09 interplay, the OL-82 retirement, the whole-formation AI movement, the `+0x9D` OR, the no-slot/`lane = 0`/zero-seed traps, the census/README invariants.
- [ ] **Step 5: Write the doc** in the house format: the design sections, legs, GO/NO-GO.

---

### Task 5: The whole-wave review + promotion

**Files:**
- Modify: the four drafts (errata only: append `## Review erratum (wave review, 2026-10-23)`).
- Commits: one per doc (`docs(fu33N): …`), plus the Ghidra DB churn sweep as its own `chore(ghidra)` commit if any.

**Interfaces:**
- Consumes: the four drafts; the spec; FU326–FU329 + the phase-33/34 records.
- Produces: the frozen wave the Stage-2 producer plan argues from.

- [ ] **Step 1: Read the four drafts against the spec and the frozen docs.** Every claim fresh or carried with its owner doc; the caller counts fresh where claimed; the alias traps respected; the unproven items legs; the verdicts honest.
- [ ] **Step 2: Cross-check the slices.** FU330's values vs FU331's design vs FU332's measured diff vs FU333's acceptance (no asserted-but-unmeasured number, no circular evidence; the producer's timing consistent across the four).
- [ ] **Step 3: Append quoted errata** for every correction (prior text stands).
- [ ] **Step 4: Commit one doc per commit**, pushing each; verify the docs-only change and that M1/M2 are unmoved.
- [ ] **Step 5: Report the wave verdicts** and hand off to the Stage-2 producer plan.
