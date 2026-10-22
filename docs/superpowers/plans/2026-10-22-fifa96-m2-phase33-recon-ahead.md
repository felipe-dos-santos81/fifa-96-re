# FIFA 96 M2 phase-33 recon-ahead plan (FU322–FU325)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce four frozen evidence docs — FU322–FU325 — that convert the phase-32 deferral's two unknowns (FU319-L7's actual M2 diff, FU319-L3's stride blast radius) into measured numbers, and design the Stage-2 producer plan's acceptance, baseline inversion, and golden governance for the live record face.

**Architecture:** Four independent read-only slices, one agent each. FU323/FU324 measure with out-of-tree patched-probe builds; FU322/FU325 are repo-static designs. Drafts land under `docs/ghidra/`, receive a whole-wave review + quoted errata, and are promoted one-per-commit. No production code, no test/golden change.

**Tech Stack:** Ghidra MCP (standard tools only; FU324 chiefly), the repo's C sources + docs, out-of-tree patched builds linking the prebuilt `build/libfifa96_*.a` libraries (the FU319 §9 method).

**Spec:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md`

## Global Constraints

- **Read-only wave:** no repo source/test/golden change; the only repo writes are the four draft docs. Patched builds and probes live under `/tmp/opencode/`; the ISO is read-only. **No git commands; no commits** — the controller promotes the drafts.
- **Ghidra (FU324, and any slice that consults it):** the program argument is exactly `/FIFA96.EXE` (case-sensitive); LE-image globals sit at `+0x100000`; scripts are disabled; standard MCP tools only; caller counts need a fresh `get_xrefs_to` in the same session; verify CALL bytes before trusting a target; the alias traps (`dword[addr]>>16` = the word at `addr+2`, `>>24` = the byte at `addr+3`; sign/width errors; gate-index misreads).
- **Carried vs fresh:** the producer facts are **carried** from FU318/FU319 (modeled on FU321 §0: name the frozen doc/section; do not re-derive what is frozen). A slice re-opens a native cell only to answer a question the frozen docs do not settle.
- **Evidence rule:** every address/byte claim cites a fresh disassembly/raw-byte read or a reconciled `file:line`; anything not statically provable becomes a **numbered leg** with its native owner; never asserted, never faked. Prior doc sections are never rewritten.
- **Doc format:** single H1 `# FU-32N — … (phase-33 R2-L7)`; `**Phase/Provenance:**` line naming the spec §s this slice serves (and the out-of-tree artifacts, if any); the question; the one-page answer; the evidence table; the design/measured sections; numbered legs `FU32N-L…`; GO/NO-GO verdict; known limits; self-audit.
- **Goldens:** M1 `09b726b7…` immutable; M2 `8434cfab…` is the re-pin baseline — the wave may **not** re-pin; the patched probe's output is evidence only.
- **Scope:** the census holds `60/80`; the move_attr metric is a separately pinned sub-change and must not be folded into the face producer's design numbers.

## Review Focus

1. **Probe hygiene** — a "measured diff" that actually changed repo files, rebuilt in-tree, or touched the ISO, or whose build line is unpublished so the measurement is not reproducible.
2. **The patch fidelity** — a probe patch that does not match FU319 §5's shape (the narrow kickoff write, the successor placement, the clear), silently measuring a different producer.
3. **The inversion design** — a replacement P7/P8 that merely re-asserts what the golden already pins, or a window asserted from the driven witness rather than the undriven tape.
4. **The metric census** — a `>>19` reader missed, or the `0x8DC68` vs `0x8DCD4` octagon aliasing confused (the two metrics differ by `min/3` vs `min/8` rounding and the `+0x65` store vs the `arm_dist_stage` source).
5. **Circular evidence** — an acceptance number that argues from its own slice's probe instead of an independent measurement or a carried frozen cite.

---

### Task 1: FU322 — the natural-pass baseline inversion design (FU321-L11)

**Files:**
- Create: `docs/ghidra/FU322_r2l7_baseline_inversion.md`

**Interfaces:**
- Consumes: the spec §2/§6 (inversion rule); FU321 §5 (item 7) + §8 L11; `tests/test_engine_m2.c` (the natural pass, the probes, the baseline asserts `:3701-3707`, the P6–P8 label `:2293`/`:6049-6054`); the phase-32 register (the witness pins).
- Produces: the exact replacement-assert design (what the honest post-producer baseline pins, per window) and the inversion commit's shape with hazards and legs `FU322-L1…`.

- [ ] **Step 1: Read the natural-pass harness end to end.** What P6/P7/P8 mean today (`:2293`, the CTA label `:6049-6054`), what cells the natural pass samples (`camera_left_zero`, `first_left_zero_step`, `camera_zero_frames`, `:2457`, `:3588-3608`), and exactly which asserts pin the zero baseline (`:3701-3707`) — file:line for each.
- [ ] **Step 2: Design the inverted asserts.** For the undriven tape: what should replace each assert (the first-left-zero window's pin, the `camera_zero_frames` tail, the pan cells' expected vs merely-recorded status, where Y=Z=0 should or should not survive). Quote FU321-L11's "same commit" restriction and shape the commit (asserts + P7/P8 + golden + provenance note).
- [ ] **Step 3: Decide what becomes a locked pin vs a recorded cell.** Per FU319 §6 (the face trajectory, the settled faces, the turn-transient counts), what is tight enough to assert in the undriven window, and what stays diagnostic-only (the honest rule: an assert that would re-pin on any unrelated change is not an assert).
- [ ] **Step 4: Enumerate the hazards.** Which other tests read the natural window (the m-42 fixtures, the claim/offset passes' own baseline asserts, the match_frame fixtures' camera-zero expectations); what else must move in the same commit or sequenced after.
- [ ] **Step 5: Write the doc** in the required format with legs and a verdict; no numbers asserted beyond the measured cells (the probe's numbers belong to FU323; cite forward).

---

### Task 2: FU323 — the measured M2 diff of the live-face producer (FU319-L7)

**Files:**
- Create: `docs/ghidra/FU323_r2l7_m2_probe.md`

**Interfaces:**
- Consumes: the spec §5/§6/§7; FU319 §5 (the minimal staging — the patch's fidelity definition), §6 (the open-loop prediction to be re-measured), §9 (the probe method); the phase-32 witnesses (`run_natural_offset_pass` `:4285`, `run_natural_claim_pass` `:4031`, the calls `:4599/:4602`).
- Produces: the measured first differing frame, the diff shape per pass, the `state=`-suffix check, the phase-32 witness verdicts, the honest-baseline cell values; legs `FU323-L1…`.

- [ ] **Step 1: Build the patched out-of-tree copy.** Copy the minimum engine sources to `/tmp/opencode/fu323/` (record the exact copy list and every build command), apply exactly the FU319 §5 staging (the three writers, the clear, the mover staging; the kickoff narrow write), uninstall nothing else. Document the patch as a diff in the artifact dir. The repo and `build/` are never written; the ISO is only read.
- [ ] **Step 2: Measure the natural pass.** The undriven `M2_KEYS`-free natural run vs the unpatched build: the first differing `frame=` line, the per-frame moved count, the camera cells (the pan, Y, Z, tracked), the band record's face/lane cells at the gate, and the settled faces. Compare against FU319 §6's open-loop prediction line by line (met / not met / better than predicted); produce the `state=`-suffix check (`diff <(cut -d' ' -f3- …)` style) for the changed frames.
- [ ] **Step 3: Measure the phase-32 witnesses.** The claim pass and the offset pass on the patched build: do the `:4397-4567` pins hold, move (per cell), or fail? Rule each (survives / re-pins with reason / stops the plan) and say which cells drove it.
- [ ] **Step 4: Measure the golden recipe.** For the full M2 run: the first differing frame vs the first face-driven event; the diff-verification commands that Stage 2 must run verbatim (`cmp`/`diff` forms with the actual output snippets as evidence).
- [ ] **Step 5: Guard the result.** If the diff starts earlier or moves something no face-gate reader can explain, do not rationalize: record it, mark the slice NO-GO-unless, and name the suspect chain.
- [ ] **Step 6: Write the doc** in the required format (measured tables vs the prediction, the witness verdicts, the recipe), legs, and known limits (patch breadth, sampling, ISO-only).

---

### Task 3: FU324 — the `move_attr` metric census and blast radius (FU319-L3)

**Files:**
- Create: `docs/ghidra/FU324_r2l7_move_attr.md`

**Interfaces:**
- Consumes: the spec §3/§7 (the separately-pinned rule); FU319 §2.2/§4 (the `+0x63` turn input and the port's `0x8DCD4` audit), §6 (the measured branch-straddles); FU77 §1.2 (block A `0x8E267..0x8E278`); the port (`match_run.c:521-522`, `fifa96_arm_helpers.c:38-71`, `fifa96_entity_distance`).
- Produces: the fresh native reader census of `+0x63 >> 16`/`>> 19`, the port's stride consumers, the sub-change's own measured M2 move (its own probe build), the RED/GREEN design; legs `FU324-L1…`.

- [ ] **Step 1: Fresh native census.** In `/FIFA96.EXE` (Ghidra MCP): find every reader of `byte[rec+0x63]`-word/byte and the `>> 16`/`>> 19` uses in the mover/turn family (`0x8E29C..0x8E2A9` and beyond), and every writer of `+0x63` (block A, and any other). Record counts per `get_xrefs_to` fresh; name the approximate per-frame order (block A runs before the turn? cite the frame chain).
- [ ] **Step 2: Port audit.** Where the port stages `move_attr` (`match_run.c:521-522`) and the exact native semantics it should stage (`+0x63`'s high word = the block-A `0x8DC68` distance stored at `+0x65`; the `>> 19` stride attr = `distance >> 3`); every port consumer of the staged value (the turn threshold, the stride index — file:line each).
- [ ] **Step 3: Measure the sub-change alone.** A second patched util build (the metric swap only, no latch), the natural pass diff: the moved counts, which records/frames, whether the two predicted branch-straddles reproduce; the `>>19` stride symptom in the frames. Compare: the face-producer diff (FU323) and this diff should be additive (record the composition check if feasible: latch-then-metric on one build).
- [ ] **Step 4: Design the RED/GREEN.** The unit pin (the staged value, the branch), the golden effect, the commit shape (after the face producer's accounting, per the spec §8.2), and the risk list (the stride index moves for every record).
- [ ] **Step 5: Write the doc** in the required format: the census table, the audit, the measured table, the design, legs, GO/NO-GO, known limits.

---

### Task 4: FU325 — the Stage-2 acceptance and golden governance

**Files:**
- Create: `docs/ghidra/FU325_r2l7_acceptance.md`

**Interfaces:**
- Consumes: the spec §5/§6/§7/§8; FU322's inversion design; FU323's measured numbers; FU324's sub-change design; FU317's Layer-2 method (`docs/screens/p31-l2-notes.md`); FU321 §5's re-pin procedure; the unit-test surface (`tests/test_action_locomotion.c:118-149`, `tests/test_engine_match_frame.c`).
- Produces: the unit-pin plan, the golden recipe, the Layer-2 live design, the hazard list, GO/NO-GO; legs `FU325-L1…`.

- [ ] **Step 1: The unit-pin plan.** RED→GREEN per change: the three writers, the clear, the mover staging, the turn arm; name the test file/fixture per pin and the minimal harness addition; mark the pre-existing pins reuse (`test_step_direct_face`, the match_frame fixtures) vs new.
- [ ] **Step 2: The golden recipe.** The verbatim Stage-2 procedure (build → run → `diff -u` → the first differing frame → the `state=` check → the M1 `cmp` → the re-pin note in the house form), hardened by FU323's measured output; the "stop-and-report" tripwires spelled out as commands with expected output shapes.
- [ ] **Step 3: The Layer-2 live design.** The natural game run (and whether a driven variant is needed at all under the live face, per FU323's witness verdicts); the trace cells, the AE relative, the sprite-only bbox caveat, the trace-primary rule; the capture stanzas as registered in the phase-31/32 notes.
- [ ] **Step 4: The hazard list.** The one-frame successor placement (FU319-L4), the two `0x8E`-branch installs' values, the m-42/fixture class, the claim ordering, the controlled fallback, the census/README invariants; each with its tripwire.
- [ ] **Step 5: Write the doc** in the required format: the design per acceptance clause of the spec §6, legs, GO/NO-GO, known limits.

---

### Task 5: The whole-wave review + promotion

**Files:**
- Modify: the four drafts (errata only: append `## Review erratum (wave review, 2026-10-22)`).
- Commits: one per doc (`docs(fu322): …` … `docs(fu325): …`), plus the Ghidra DB churn sweep as its own `chore(ghidra)` commit if any.

**Interfaces:**
- Consumes: the four drafts; the spec; FU314/FU318/FU319/FU321.
- Produces: the frozen wave the Stage-2 producer plan argues from.

- [ ] **Step 1: Read the four drafts against the spec and the frozen docs.** Every claim fresh or carried with its owner doc; the caller counts fresh where claimed; the alias traps respected; the unproven items legs; the verdicts honest; no production claim smuggled.
- [ ] **Step 2: Cross-check the slices.** FU322's inversion design vs FU323's measured cells (no asserted-but-unmeasured number); FU323's patch fidelity vs FU319 §5 (narrow kickoff write, successor placement, the clear); FU324's census vs the port's staging; FU325's recipe vs the measured recipe (no circular evidence).
- [ ] **Step 3: Append quoted errata** for every correction (prior text stands; the erratum quotes it and corrects it).
- [ ] **Step 4: Commit one doc per commit**, pushing each; verify the docs-only change and that M1/M2 are unmoved.
- [ ] **Step 5: Report the wave verdicts** and hand off to the Stage-2 producer plan.
