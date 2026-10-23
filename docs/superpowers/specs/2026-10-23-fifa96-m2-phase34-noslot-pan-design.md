# FIFA 96 M2 phase-34 design — the no-slot pan travel (FU316-L1)

## 1. Goal

Complete the pan-travel model for the **no-slot seed arm** and settle whether
the undriven natural tape can produce a real pan from it. The phase measures
the seed distribution over the natural window (every row-04 gate pass: slot
state, seed arm, `v0`, the place delta), audits the port's seed staging against
the native `0x7F056` arm, sweeps the natural-play shapes for a **moving
no-slot band entry** (`|v0| ≥ 1`), and either lands the natural no-slot pan
acceptance or records the honest NO-GO with the measured distribution and the
remaining legs.

## 2. What phase 33 left (the binding starting state)

- The live face is in: every record turns live (`+0x43 = 2` after kickoff), the
  band record (entity 12) arrives facing its focus point and the row-04 gate
  passes — but the row-04 event on the undriven tape is a **silent bind**: the
  band record holds no slot, the seed switch takes the **no-slot arm**
  (`0x7F056`, `word[+0x73]<<2`), the parked record's stride velocity ±1 gives
  seed ±4, `v0 = trunc(±4/0xC) = 0`, and the place never moves (FU323-L1; the
  phase-33 register). The natural-pan owner leg is **FU316-L1** (promoted in
  the phase-33 close-out).
- The pan chain is measured: the gate → the seed arm → `FUN_00071C94` → the
  decay `5→3→2→1→0` → the place `T(s)` with `T(0x40) = 0x90`; a larger
  no-slot seed `s = 0x80` gives `T = 0x174` (FU316 §4.3). The measured pans all
  used `s = 0x40`; **no measurement covers the larger no-slot seeds**
  (FU316-L1).
- The natural window is 3,598 granted frames (the M2 `M2_KEYS` natural pass);
  the driven claim/offset passes are the two chained-segment witnesses
  (phase 33). M1 `09b726b7…` immutable; M2 `d5446144…` is the current pin;
  census `60/80 = 75%`.

## 3. Scope

**In slice:**
- The native no-slot seed algebra: the `0x7F056` arm (`camY == 0 && !has_slot`),
  the seed word(s) and packing, the `v0` derivation, the `T(s)` travel table,
  and the boundary/edge cases (sign, width, the 0x730 armer reach).
- The port audit: `fifa96_outfield_row04_step`'s seed staging
  (`src/fifa96_loader/fifa96_outfield.c:685-720`) and every consumer of the
  seed/`v0`, against the native bytes — a fidelity verdict, not a rewrite.
- The **measured distribution** over the natural window and the driven passes:
  every row-04 gate pass's slot state, seed arm, seed word, `v0`, event, place
  delta; the no-slot instances classified; the phase-33 silent bind reproduced.
- The **natural reachability sweep**: which natural-play shapes produce a
  moving no-slot band entry (`|v0| ≥ 1`), with the honest limits (the drive,
  the tape, the AI's own play), and the FU316-L7 frequency evidence.
- The acceptance design: the Layer-1 tuple for a no-slot pan (gate
  `has_slot 0`, `|v0| ≥ 1`, the event, the place delta = `T(s)`), the Layer-2
  live design, and the M2/golden governance if the tape moves (the
  faithful-move policy, the stratum-set gate precedent, the first-diff rule).
  **The acceptance is conditional**: it lands only if the wave measures a
  witness (a drive that produces the tuple); a NO-GO lands the measured
  distribution + the legs instead, with no invented drive.

**Out slice (stays legs, never asserted):**
- The armer/follow body (FU316-L6, the unarmed mirror FU316-L9), the
  claim/carrier camera fusion (FU315-L1, FU316-L8), the landing pan-step
  (FU316-L2), the tracked-gate fidelity (FU316-L3/L4), the scanner side source
  (FU316-L5), and the phase-33 legs (FU319-L8, FU324-L8/L9, FU325-L8).
- No new camera code, no armer work. If the wave proves one of these blocks the
  natural route, that becomes a numbered leg with its owner — not an
  in-phase producer.

## 4. Native authority (carried, not re-derived)

- The seed switch: `0x7F056` (`camY == 0 && !has_slot`) → seed =
  `word[+0x73] << 2`; the slot arm packs the slot dirs; the seed feeds
  `FUN_00071C94` (`0x70544` seeds the velocity) (FU316 §3.2/§4.3).
- The travel: `T(s)` is the decay sum with `v0 = trunc(s/0xC)`; the measured
  `s = 0x40` → `T = 0x90` with the `5→3→2→1→0` signature; `s = 0x80` →
  `T = 0x174` (FU316 §4.3; FU321 §3.4).
- The armer reach: `ceil(0x730/T(s))` same-direction pans (fewer for larger
  no-slot seeds) — recorded, never asserted (FU316 §4.3/§7.1).
- The phase-33 measured baseline: the undriven band record is entity 12,
  `has_slot 0`, parked; the bind at granted f=1517; camera `pos` never moves;
  `camera_zero_frames` 1517 / `camera_left_zero` 2081 (FU323 §2/§4).

## 5. Structure

Two stages, the phase-33 shape:

- **Stage 1 (recon-ahead wave):** four read-only slices → FU326–FU329, one
  agent each, drafts under `docs/ghidra/`, whole-wave review + quoted errata,
  promoted one-per-commit. Probes are out-of-tree (`/tmp/opencode/`); the repo
  receives only the four draft docs.
- **Stage 2 (SDD execution, after the wave review):** the producer plan.
  Sketch: if FU328 finds a witness — **P1** the acceptance witness (the drive +
  the pins, test-side, like phase 32), **P2** the Layer-2 live acceptance,
  **P3** the close-out; if NO-GO — **P1** the measured-distribution register +
  the port-fidelity verdict (no production change), with the legs handed to the
  next phase. Either way: no production change unless FU326's audit finds a
  measured seed-staging divergence (then a separately pinned fix with its own
  RED/GREEN).

## 6. Acceptance (conditional on the wave's measured numbers)

- **The witness (if found):** on the **undriven natural tape** (`M2_KEYS`
  verbatim, no added holds), a row-04 gate pass with `has_slot 0` and
  **`|v0| ≥ 1`**, the event binding, and the place moving by `T(s)` over the
  decay (pan `0→4` per segment); the gate tuple recorded, not derived; the
  drive named (keys + phase-2-relative indices) and the tuple re-run once for
  stability.
- **NO-GO (if not found):** the measured seed distribution over the natural
  window and the driven passes (the no-slot instance count, the max |seed|,
  the max |`v0`|, the moving-record conditions), the port-fidelity verdict,
  and the remaining legs (FU316-L1 narrowed to the distribution evidence;
  FU316-L7 the natural frequency).
- **Golden governance:** M2 re-pins only if the witness's acceptance changes
  the golden — then under the faithful-move policy (the written reason, the
  frame diff, the state=-suffix check, the stratum-set gate precedent, the
  first-diff rule). M1 `09b726b7…` immutable. Any diff unexplained by the
  no-slot pan is a stop-and-report.
- **Census/README:** `60/80 = 75%` survives (no new action-row wiring); the
  README title is recomputed only if a fresh census says otherwise.
- **The live Layer-2 (if the witness lands):** the driven live run under the
  witness drive; the trace-primary rule, the AE relative, the sprite-only bbox
  caveat; the honest timing-waiver option.

## 7. Honest-stop rule

Any measurement contradicting the carried prediction becomes a numbered leg
with its native owner — never asserted, never explained away. A "witness" that
passes at `lane = 0`, with `v0 = 0`, or with a zero place delta is NO-GO (the
phase-32/33 traps). If the sweep needs a staged slot/seed cell to force the
tuple, that is not a natural witness: it becomes a leg and the phase lands the
distribution, not an acceptance.

## 8. Decisions (this spec)

1. The acceptance vehicle is the **undriven natural tape**; the driven passes
   are measurements, not the witness.
2. The seed/slot cells are **never staged** in a natural witness (the phase-32
   NO-GO condition carries).
3. The wave's measured numbers bind the Stage-2 plan; the carried `T(s)` table
   is a check, not an authority.
4. The no-slot arm's **larger seeds** are measured, not assumed: the sweep
   records the achieved `(|seed|, |v0|, T)` values per event.
5. The port's seed staging is audited against the native bytes; a divergence
   is a separately pinned fix, never folded into the acceptance.
6. The phase is either an acceptance phase (witness found) or a
   measurement/NO-GO phase (not found) — never a staged-slot producer.

## 9. Open items the wave settles

1. **FU326** — the native no-slot seed algebra (the arm, the words, `v0`, the
   `T(s)` table, the edge cases) + the port audit (a fresh Ghidra read).
2. **FU327** — the measured seed distribution on the current tree (the natural
   window + the driven passes; the silent bind reproduced; the no-slot
   instances classified).
3. **FU328** — the natural reachability sweep (which shapes give a moving
   no-slot band entry; the minimal honest drive if any; the FU316-L7
   frequency evidence; GO/NO-GO on the natural witness).
4. **FU329** — the acceptance and golden governance (the Layer-1/2 design, the
   M2 policy, the hazards, the leg ledger), arguing from the measured numbers.
