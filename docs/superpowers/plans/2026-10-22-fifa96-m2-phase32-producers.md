# FIFA 96 M2 phase-32 producers — the non-zero-delta natural pan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Land the first natural **non-zero-delta** row-04 camera pan on the current tree — the FU320 drive witness, the Layer-2 acceptance, and the close-out — with **no production change** and M1/M2 byte-identical.

**Architecture:** Test-side only. A new natural-pass witness beside the phase-31 T1 witness drives `M2_KEYS` + LEFT 1820..1849 + DOWN 1850..end, pinning the offset gate tuple, the event/claim, the 27-frame delta run and the stop at `−0x90`; a live `make game` acceptance under the same drive; then the append-only register/PROGRESS close-out. The live-face producer (FU314-L5/L3) is **deferred to phase 33**: the wave proves it is the natural producer but a universal behaviour change (38/111 rows; the undriven tape would pan; the natural-pass zero-camera asserts invert), larger than this phase's pan scope.

**Tech Stack:** C17/CTest, the null-backend harness, SDL3 `make game`, the env-gated camera trace, ffmpeg/ImageMagick for the captures.

**Spec:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md`

## Decisions carried into this plan (the frozen wave, FU318–FU321)

- **FU320 GO — the no-code R2 witness exists.** `M2_KEYS` verbatim + **LEFT 1820..1849** + **DOWN 1850..end**: gate `f=1528`, record 2 at `pos (−32,0,16)`, `lane 36`, `cam_dx +32`, `cam_dz −16`, face `0x100`, bearing `0x14C`, Δ`0x4C`, `has_slot 1`, `dir (−1,0)`, `tracked −1`, `controlled 1`; event `f=1529` (code 5, seed `−0x40`, `trk 2`), claim `f=1530` (`carrier 2`), motion `f=1530..1556` (27 frames), stop `f=1557` at `(−0x90,0,0)`, `pan 0→4`, Y=Z=0, `follow=0`.
- **FU318/FU319 GO on the producer fidelity, but the M2 move is broad.** FU314-L5 `direct_face` is the natural producer (the locomotion turn, latch `+0x43 = row_flags & 2`, `2` for every record after kickoff) and would make the **undriven** tape pan, flipping the natural-pass zero-camera baseline. The spec §8.1 makes the producer conditional. **Ruling: this phase lands the drive-only witness; the live-face fidelity (FU314-L5 + the row-local L3) is phase 33**, with FU319 §5/§6 and FU321 §5/§6 as its starting authority. Cost if wrong: the live face stays static one more phase; the witness remains geometry-luck (FU320-L1).
- **FU321's C1/C2 are satisfied** (lane 36; `|v0| = 5`; the wave erratum corrects the criterion from `v0 ≥ 1` to `|v0| ≥ 1`). The acceptance reads the observed `(|Δ|, frames)` and states the offset only from the Layer-1 tuple.

## Global Constraints

- `make check` green (strict `-Wall -Wextra -Werror`, ASan/UBSan; ~203 s → ≥600 s timeout); work on `main`; commit and push per task.
- M1 `09b726b7…` immutable; M2 `8434cfab…` **byte-identical** — this phase lands no production change (Case A); **any M2 move is a STOP**, not a re-pin.
- The natural witness stages **inputs only** — no camera cell, no gate cell, no `face7d`. The existing T2 unit pins (`test_row04_live_face_passes_face_aligned_gate` / the zero-face rider, `test_live_claim_pan_row04_half_line`) stay as unit pins (spec §9.1) and are not rewritten.
- The offset is a **Layer-1** claim: `lane_at_pass ≥ 1` at the passing dispatch. The live trace carries camera cells only (FU321-L1); Layer 2 reproduces the drive and the arithmetic, never "the trace shows the offset".
- The census holds `60/80 = 75%`; the follow/Y stays waived; the armer NO-GO.
- The drive indices are exact (1820/1849/1850); `run_natural_pass` and the T1 witness stay untouched.
- No producer lands ⇒ no new mutation rider; the existing T2 riders stand, and the frame-path fixture is the existing `test_live_claim_pan_row04_half_line` (same chain, producer inputs only) — no new gate-staging fixture is added.

## Review Focus

1. **The offset pin must fail on the zero-delta drive** — the RED is `lane_at_pass == 0` on the T1 DOWN drive (`pos (0,0)`, bearing `0x80`), not a claim-frame mismatch.
2. **The delta-run count** (27 changed frames) vs the off-zero count (~2,084) — the FU317 §3.4 false pass.
3. **The gate tuple is recorded, not re-derived** — the test must not recompute the half-line predicate (FU317-L2); it asserts the measured `lane`/`bound`/`cam_dx`/`cam_dz`/`face7d`/`bearing`/`Δ` cells.
4. **The drive indices are exact and the zero control stays** — the LEFT window sets the z offset; a walk/metric change moves the gate (re-measure, do not tune).
5. **The m-42 fixture's staged `face7d = 0` is insensitive** — it cannot detect a re-staging regression; the new pins must be their own evidence.

---

### Task 1: The natural non-zero-delta witness

**Files:**
- Modify: `tests/test_engine_m2.c` (a new pass beside `run_natural_claim_pass`; its call in the M2 test main; the shared helpers)
- Test: the same file's focused M2 test

**Interfaces:**
- Consumes: the FU320 drive and tuple above; the T1 witness's helpers (the verbatim tape, the env-gated trace sink, the per-granted-frame sampler, the delta-run rule).
- Produces: `run_natural_offset_pass` (the repo's naming; a sibling of `run_natural_claim_pass`) and the pins Task 2/3 cite. No production symbol.

- [ ] **Step 1: RED — the offset pin on the zero-delta drive.** Add the new pass using the **T1 drive** (`DOWN` from tape index 1820) with the offset assertions (`lane_at_pass ≥ 1`, `(cam_dx,cam_dz) ≠ (0,0)`). Run the focused test. Expected FAIL: `lane_at_pass == 0` (the T1 tuple `pos (0,0)`, bearing `0x80`, the zero-delta knife edge).
- [ ] **Step 2: Implement the FU320 drive.** `M2_KEYS` verbatim entries `0..1799`; `LEFT` held on tape indices **`1820..1849`**; `DOWN` held **`1850..end`**; 12 000 engine steps. Keep the T1 pass's prefix/phase handling untouched.
- [ ] **Step 3: Assert the gate tuple (recorded, not derived).** At `f=1528`: record 2, code 4, active 2, `pos (−32,0,16)`, `lane 36`, `bound 36`, `cam_dx +32`, `cam_dz −16`, `face7d 0x100`, `bearing 0x14C`, `Δ 0x4C`, `timer81 0`, `has_slot 1`, `anim 0`, `dir (−1,0)`, `tracked −1`, `controlled 1`, `slot.entity 2`, camera `(0,0,0)`, `camY 0`. Assert `lane_at_pass == 36` and `Δ ≤ 0xAB`.
- [ ] **Step 4: Assert the event/claim/pan.** Event `f=1529`: code 5, seed `−0x40`, `trk 2`, `timer 0/12`, `ramp 12`; claim `f=1530`: `carrier 2`; motion `f=1530..1556`: 27 changed frames; stop `f=1557` at `(−0x90,0,0)`; `pan 0→4`; speed `5→3→2→1→0`; Y=Z=0; armer/OOB 0; `follow 0`.
- [ ] **Step 5: Assert the delta-run/off-zero separation.** The changed-frame run is 27; the off-zero count is recorded and is **not** the count (assert the relation; never assert `off_zero == 27`).
- [ ] **Step 6: The trace cross-check and the zero control.** The trace is 1 header + 3 598 body lines, `trk=2` on the pan lines, `follow=0` on all; `run_natural_pass` is unchanged (3 598 granted frames, zero camera).
- [ ] **Step 7: GREEN and the suite.** Focused test PASS; `make check` 111/111; `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt`; `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt`. The frame-path fixture is the existing `test_live_claim_pan_row04_half_line` (same chain; producer inputs only); the T2 pair remains the unit pin. No new gate-staging fixture and no new mutation rider.
- [ ] **Step 8: Commit and push** `test(fu320): the non-zero-delta natural pan witness`.

---

### Task 2: The Layer-2 acceptance

**Files:**
- Create: `docs/screens/p32-l2-notes.md`, `docs/screens/p32-l2-pan-a.png`, `docs/screens/p32-l2-pan-b.png`, `docs/screens/p32-l2-still-a.png`, `docs/screens/p32-l2-still-b.png`, `docs/screens/p32-l2-pan-diff.png`, `docs/screens/p32-l2-trace-pan.txt`

**Interfaces:**
- Consumes: Task 1's drive/tuple; FU321 §4 (the procedure, the trace cells, G1–G8).
- Produces: the acceptance note and the artefacts Task 3 registers.

- [ ] **Step 1: Run the live drive.** `DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p32-cam-trace.log make game`; the phase-8 walkthrough (intro RETURN → front-end BACKSPACE → RETURN → match); the tape-equivalent prefix + the tick-synced act-1 KICK; FU320's drive started at **phase 2 + 20 ticks** (never an absolute tick); record both the null-tape index and the live tick/`f` for every anchor.
- [ ] **Step 2: Capture the pairs.** The same-clock pan pair (both inside the displayed second and the trace's delta run) and the still pair; crop `960x591+0+0`; `compare -metric AE`; the HUD crop AE 0; the diff and its bbox.
- [ ] **Step 3: Extract the trace.** The phase-2 line, the gate line, the event/motion/stop window, the still window; check G1 (one delta run, `trk=2`, `pan 0→4`, `pos_y=0`, `follow=0` on all), G2 (the observed `(|Δ|, frames) = (0x90, 27)` against the recorded seed), G4 (`|v0| = 5`).
- [ ] **Step 4: Write the note.** The verdict: **a pan reproduced live under the non-zero-delta drive** — never "the trace shows the offset"; the offset tuple is cited from Task 1 (G3). Record the pan class (slot arm, `−0x90`/27 frames), Gate 1 (`AE_pan > AE_still`, same session) and the Gate 2 bbox caveat (sprite-only; fail expected), the timing limits (FU321-L4), and the exact commands.
- [ ] **Step 5: If the live timing cannot be reproduced**, land the honest **timing waiver** with the exact limits (what was tried, the trace/absence) per the spec §6; do not invent a verdict.
- [ ] **Step 6: Commit and push** `docs(fu320): the non-zero-delta pan acceptance`.
- [ ] **Step 7: Invariants.** `make check` 111/111; M1/M2 `cmp` identical; no code/test/golden change in this task.

---

### Task 3: The close-out

**Files:**
- Modify: `docs/ENGINE.md` (append the phase-32 register), `docs/PROGRESS.md` (append the narrative), `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md` (append the plan erratum)
- Test: the final verification commands

**Interfaces:**
- Consumes: Task 1's pins, Task 2's verdict/artefacts, the frozen wave FU318–FU321.
- Produces: the frozen record.

- [ ] **Step 1: Append the register** (append-only; the house quoted-errata style): the verdict (a natural **non-zero-delta** pan; the offset tuple; the same `−0x90`/27 class; drive-only; no production change); the witness pins; the Layer-2 evidence (the captures, the AE, the bbox caveat, the trace primary); the wave's verdicts (FU318 census GO / NO-GO as the walker's producer; FU319 producer GO / broad M2; FU320 drive GO; FU321 acceptance GO); the **deferred live-face fidelity** (the ruling; FU319 §5/§6 + FU321 §5/§6 as the phase-33 authority; the `|v0|` erratum); the legs (FU318-L1..L8, FU319-L1..L7, FU320-L1..L8, FU321-L1..L11, the live timing).
- [ ] **Step 2: The PROGRESS narrative** — the first non-zero-delta pan; the plateau-riding honesty (`lane == bound`; not a robust crossing); the live face deferred; M1/M2 unchanged.
- [ ] **Step 3: The spec erratum** — the producer-conditional resolves to drive-only; the live face is phase 33; cite the wave's FU321 `|v0|` correction.
- [ ] **Step 4: Final verification** — `make check` 111/111; M1 `09b726b7…`; M2 `8434cfab…`; the census `60/80 = 75%`; the tree clean.
- [ ] **Step 5: Commit and push** `docs(phase32): the close-out — the non-zero-delta pan`.
