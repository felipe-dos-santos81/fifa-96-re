# FIFA 96 M2 phase-34 recon-ahead plan (FU326–FU329)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce four frozen evidence docs — FU326–FU329 — that settle the no-slot seed arm (FU316-L1): the native algebra and the port's fidelity, the measured seed distribution on the current tree, the natural reachability of a moving no-slot band entry, and the acceptance/golden governance — with a GO/NO-GO on a natural no-slot pan.

**Architecture:** Four independent read-only slices, one agent each. FU326 is a fresh-Ghidra static slice; FU327/FU328 measure with out-of-tree probe builds of the current tree; FU329 is the acceptance/governance design. Drafts land under `docs/ghidra/`, receive a whole-wave review + quoted errata, and are promoted one-per-commit. No production code, no test/golden change.

**Tech Stack:** Ghidra MCP (standard tools only), the repo's C sources + docs, out-of-tree probes linking the prebuilt `build/libfifa96_*.a` libraries.

**Spec:** `docs/superpowers/specs/2026-10-23-fifa96-m2-phase34-noslot-pan-design.md`

## Global Constraints

- **Read-only wave (tasks 1–4):** no repo source/test/golden change; the only repo writes are the four draft docs. Probes/builds live under `/tmp/opencode/fu32N/`; the ISO is read-only. Tasks 1–4 run no git commands and make no commits — task 5 is the controller's promotion task.
- **Ghidra (FU326, and any slice that consults it):** the program argument is exactly `/FIFA96.EXE` (case-sensitive); LE-image globals sit at `+0x100000`; scripts are disabled; standard MCP tools only; caller counts need a fresh `get_xrefs_to` in the same session; verify CALL bytes before trusting a target; the alias traps (`dword[addr]>>16` = the word at `addr+2`, `>>24` = the byte at `addr+3`; sign/width errors; gate-index misreads).
- **Carried vs fresh:** the pan chain is frozen authority in FU316 (its §3/§4 measured it); do not re-derive it. The seed-arm algebra is re-read **fresh** in FU326 (the frozen doc does not carry the full seed table/edge cases).
- **Evidence rule:** every address/byte claim cites a fresh disassembly/raw-byte read or a reconciled `file:line`; anything not statically provable becomes a **numbered leg** with its native owner — never asserted, never faked. Prior doc sections are never rewritten.
- **Doc format:** single H1 `# FU-32N — … (phase-34 R2-L8)`; `**Phase/Provenance:**` line naming the spec §s this slice serves (and the out-of-tree artifacts); the question; the one-page answer; the evidence table; the design/measured sections; numbered legs `FU32N-L…`; GO/NO-GO verdict; known limits; self-audit.
- **Goldens:** M1 `09b726b7…` immutable; M2 `d5446144…` is the phase-33 pin — the wave may **not** re-pin; an expected move is analyzed, never applied.
- **Scope:** the census holds `60/80`; the armer/follow (FU316-L6/L9), the claim/carrier fusion (FU315-L1, FU316-L8), the landing pan-step (FU316-L2), the tracked-gate fidelity (FU316-L3/L4) stay out.

## Review Focus

1. **The no-slot arm's identity** — a slice that confuses the arm's condition (`camY == 0 && !has_slot`) with the slot arm, or the seed word (`word[+0x73]`) with the velocity/int packing; the measured silent bind must reproduce.
2. **The probe hygiene** — a "measured distribution" that actually changed repo files, rebuilt in-tree, or touched the ISO; the build line must be published and the unpatched probe must reproduce the committed golden.
3. **The witness traps** — a natural witness that passes at `lane = 0`, `v0 = 0`, or a zero place delta (the phase-32/33 NO-GO classes); a staged slot/seed cell masquerading as natural.
4. **The travel arithmetic** — `T(s)` from the measured decay, not a remembered formula; the achieved `(|seed|, |v0|, T)` per event.
5. **The M2 exposure** — an acceptance that would move the golden without the faithful-move accounting; the first-diff rule and the stratum-set-gate precedent.

---

### Task 1: FU326 — the native no-slot seed algebra and the port audit

**Files:**
- Create: `docs/ghidra/FU326_r2l8_noslot_seed.md`

**Interfaces:**
- Consumes: the spec §3/§4; FU316 §3.2/§4.3 (the carried chain); the port's seed staging (`src/fifa96_loader/fifa96_outfield.c:685-720`).
- Produces: the seed-arm algebra (condition, words, packing, `v0`, `T(s)`, edge cases), the port-fidelity verdict, GO/NO-GO; legs `FU326-L1…`.

- [ ] **Step 1: Re-read the seed arm fresh.** In `/FIFA96.EXE` (Ghidra MCP): the `0x7F056` arm and its siblings (the camY/slot arms), the seed word(s) and every packing term (dir/vel/type), the `v0` derivation (the `0xC` divisor and the truncation), and the `T(s)` decay table; disassemble the arm and the event-seed site (`FUN_00071C94`/`0x70544`).
- [ ] **Step 2: The edge cases.** Sign/width of the seed word; the zero-velocity parked case; the moving cases (`word[+0x73]` = ±1/±3/±5…); the seed magnitudes that reach the `0x730` armer (the `ceil(0x730/T)` count); anything the frozen FU316 §4.3 left implicit.
- [ ] **Step 3: The port audit.** Read `fifa96_outfield_row04_step`'s seed staging (`outfield.c:685-720`) and every consumer of the seed/`v0` in the engine; compare each cell against the fresh native bytes; name divergences (file:line + native address) or state parity.
- [ ] **Step 4: Write the doc** in the house format: the algebra table, the port-fidelity table, the edge cases, legs, GO/NO-GO (GO if the port stages the native arm; a divergence with a measured blast radius is a separately pinned fix, not this wave's).
- [ ] **Step 5: Self-audit** every claim against a fresh read; unproven → a numbered leg.

---

### Task 2: FU327 — the measured seed distribution (the current tree)

**Files:**
- Create: `docs/ghidra/FU327_r2l8_seed_distribution.md`

**Interfaces:**
- Consumes: the spec §3/§6; FU326's algebra (or the carried FU316 if FU326 runs parallel — forward-cite); the phase-33 measured baseline (FU323 §2/§4); the harness (`tests/test_engine_m2.c`'s natural/claim/offset passes).
- Produces: the per-gate-pass distribution, the no-slot classification, the silent-bind reproduction, the max |seed|/|v0|/T, GO/NO-GO for the distribution question; legs `FU327-L1…`.

- [ ] **Step 1: Build the probe.** An out-of-tree copy of the natural pass (the FU319 §9 / FU323 §3 method: a copy of `tests/test_engine_m2.c` with a dump-gated main linking the repo's prebuilt `build/libfifa96_*.a`; publish the build line; the unpatched binary must reproduce the committed golden byte-for-byte before any instrumentation).
- [ ] **Step 2: Sample every row-04 gate pass.** Over the natural window (`M2_KEYS` verbatim, 12 000 steps) and the two driven passes: the gate tuple (pos, lane, bound, camY, face/bearing/Δ), `has_slot`, the seed arm taken, the seed word, `v0`, the event cells, the place delta, the pan — per pass.
- [ ] **Step 3: Classify + verify the baseline.** The no-slot instances (count, |seed| max, |v0| max, T); confirm the phase-33 silent bind at granted f=1517 (the zero seed, `trk=12`, no motion) reproduces; the slot-arm instances as controls.
- [ ] **Step 4: Answer the distribution question.** Does ANY measured gate pass carry a no-slot `|v0| ≥ 1`? If yes, the exact pass(es); if no, the measured conditions (which velocities would be needed).
- [ ] **Step 5: Write the doc** in the house format with the distribution table, the baseline reproduction, the answer, legs, GO/NO-GO, known limits (sampling, ISO-only).
- [ ] **Step 6: Re-run the probe once** to confirm the distribution is stable.

---

### Task 3: FU328 — the natural reachability sweep

**Files:**
- Create: `docs/ghidra/FU328_r2l8_reachability.md`

**Interfaces:**
- Consumes: the spec §3/§6; FU327's distribution (forward-cite if parallel); FU315's claim producer; the natural probe/drive harness (FU317).
- Produces: the natural-play shapes that give a moving no-slot band entry, the minimal honest drive (if any), the FU316-L7 frequency evidence, GO/NO-GO on a natural witness; legs `FU328-L1…`.

- [ ] **Step 1: Define the target shape.** From FU327's required velocity/conditions: a no-slot, non-parked record entering the half-line cone with `|v0| ≥ 1`; the slots/AI states that produce such a moving record.
- [ ] **Step 2: Sweep the natural-play shapes** (out-of-tree drives; no staging of slot/seed/face cells): the M2_KEYS baseline over longer windows; the second half; restarts; the natural open-play variants (the phase-31/32 drive families as families, not staged seeds); the claim/carrier contexts.
- [ ] **Step 3: Nominate a witness** — the exact drive (keys + phase-2-relative indices), the gate frame, the seed/v0, the expected place delta `T(s)`, the event/pan cells — or the honest NO-GO with the sweep table (which shapes were tried and why none produced the tuple).
- [ ] **Step 4: The frequency evidence.** The pan-count/band-entry rate over the natural windows (the FU316-L7 question), recorded as measured, never asserted.
- [ ] **Step 5: Write the doc** in the house format: the target shape, the sweep table, the nominated drive or the NO-GO, the frequency evidence, legs, GO/NO-GO, known limits (the drive family's honesty, the AI coupling).
- [ ] **Step 6: Re-run the nominated drive once** for stability.

---

### Task 4: FU329 — the acceptance and golden governance

**Files:**
- Create: `docs/ghidra/FU329_r2l8_acceptance.md`

**Interfaces:**
- Consumes: the spec §5/§6/§7/§8; FU326's algebra; FU327's distribution; FU328's witness/NO-GO (forward-cite where parallel); the phase-32/33 acceptance precedents (`docs/screens/p32-l2-notes.md`, the register).
- Produces: the Layer-1/2 acceptance design (or the NO-GO record design), the M2/golden policy, the hazards, GO/NO-GO; legs `FU329-L1…`.

- [ ] **Step 1: The Layer-1 witness design.** The gate tuple + `|v0| ≥ 1` + the event/place delta `T(s)` + the pan cells; the recorded-vs-asserted split (the phase-32/33 rules); the exact pins.
- [ ] **Step 2: The Layer-2 design.** The live run under the witness drive (trace-primary, AE relative, the sprite-only bbox caveat); the timing-waiver option; the segment-aware anchoring (phase 33's lesson).
- [ ] **Step 3: The golden governance.** If the witness moves M2: the faithful-move recipe (the first-diff rule, the state=-suffix check, the stratum-set gate precedent, the written reason, M1 `cmp`); if NO-GO: the no-golden-change statement. Any unexplained diff → stop-and-report.
- [ ] **Step 4: The hazards.** The `lane = 0` knife edge, the zero-seed bind, the staged-slot trap, the moving-record face/target coupling, the armer reach (`0x730`), the m-42 fixture class, the census/README invariants.
- [ ] **Step 5: Write the doc** in the house format with the design per spec §6, legs, GO/NO-GO, known limits.

---

### Task 5: The whole-wave review + promotion

**Files:**
- Modify: the four drafts (errata only: append `## Review erratum (wave review, 2026-10-23)`).
- Commits: one per doc (`docs(fu32N): …`), plus the Ghidra DB churn sweep as its own `chore(ghidra)` commit if any.

**Interfaces:**
- Consumes: the four drafts; the spec; FU315/FU316 + the phase-33 records.
- Produces: the frozen wave the Stage-2 producer plan argues from.

- [ ] **Step 1: Read the four drafts against the spec and the frozen docs.** Every claim fresh or carried with its owner doc; the caller counts fresh where claimed; the alias traps respected; the unproven items legs; the verdicts honest.
- [ ] **Step 2: Cross-check the slices.** FU326's algebra vs FU327's measured seeds; FU327's distribution vs FU328's sweep target; FU328's witness/NO-GO vs FU329's acceptance (no asserted-but-unmeasured number, no circular evidence).
- [ ] **Step 3: Append quoted errata** for every correction (prior text stands).
- [ ] **Step 4: Commit one doc per commit**, pushing each; verify the docs-only change and that M1/M2 are unmoved.
- [ ] **Step 5: Report the wave verdicts** and hand off to the Stage-2 producer plan.
