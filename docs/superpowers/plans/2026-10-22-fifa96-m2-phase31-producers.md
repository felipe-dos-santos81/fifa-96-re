# FIFA 96 M2 phase-31 Stage-2 — the natural claim pan: the witness, the face fidelity, the visible pan

> **For agentic workers:** REQUIRED SUB-SKILL: `superpowers:subagent-driven-development`. Steps use checkbox (`- [ ]`).

**Goal:** land the phase-31 natural pan honestly: the headless natural-pan witness (the FU317 drive), the row-`0x24` staged-face fidelity that turns the knife-edge zero-delta pass into the native face-aligned gate, and the visible-pan acceptance (the camera place moving in `make game`) — with the armer/camera-walk recorded as a measured NO-GO and the follow staying waived.

**Architecture:** the harness first (the natural-pan probe + the delta-run pins, test-side only); then the face fidelity (the staged-face cluster at `handlers.c:2646`/`:5381`, the native `0x865B5/0x865BE/0x865C6`; the M2 risk handled with the diff/reason rule); then the Layer-2 visible-pan capture (the `make game` drive + the pan/still AE + the trace); then the close-out. No possession producer, no armer spike, no follow — the wave proved each out or unnecessary.

**Tech Stack:** C (strict `-Wall -Wextra -Werror`, ASan/UBSan), the repo's frame-tape tests, the T1 harness, SDL3 `make game`, Ghidra MCP read-only.
**Spec:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase31-natural-claim-pans-design.md` (§4, §5, §6, §8).
**Authority:** the phase-31 wave `docs/ghidra/FU314..FU317` (reviewed, errata appended 2026-10-22) + the phase-30 register.

## Global Constraints

- `make check` green (strict; ASan/UBSan; ~193 s → ≥600 s timeout); work on `main`; commit per task; push per task.
- M1 `09b726b7…` immutable; **M2 `8434cfab…`** is the baseline. T1/T3 are test/docs-side (byte-identical). T2 may move it **only** for the intended face correction: capture the frame diff + the written reason, re-pin the golden in the same commit, and record the new pin in the register. Otherwise STOP.
- The live-chain rule: never fake a cell/gate; the natural witness stages the drive inputs only, never the camera cells.
- The census holds `60/80 = 75%` (no row joins; the four-doc same-commit rule if one does).
- The wave's verdicts are binding: the armer NO-GO, the follow waived, no possession producer, no no-slot armer probe.
- Ghidra read-only; no fakes; the wave's legs carried.

## Review Focus (the input classes most likely to bite)

1. **The natural pan's one-shot/knife-edge** (T1): the zero-delta instant (`lane_x = lane_z = 0` → bearing `0x80`, face `0x100`, Δ`0x80`), one pan, 27 frames, `−0x90`; the delta-run count, not the off-zero count (2,082 undriven frames would false-pass).
2. **The drive timing** (T1/T3): `M2_KEYS` + the DOWN hold from tape index 1820 → the claim f=1514/pan f=1513..1540; the timing coupling (1820→f1514, 2100→f1544); the `LONG_BALL` inertness; the zero-control of `run_natural_pass`.
3. **The face fidelity** (T2): the native constants vs the staged 0; the row-0x24 sites; the M2 risk (the face affects the sim) — the diff/reason/re-pin discipline.
4. **The controlled fallback** (`handlers.c:2672`): a live hazard for any drive whose panning record is `controlled`; the pin must assert `controlled`/`slot.entity`/the staged face.
5. **The Layer-2 attribution** (T3): the trace primary; the pan/still AE relative; the scene shift vs the sprite band.

---

### Task 1: The natural-pan witness (FU317)

**Files:** `tests/test_engine_m2.c` (the probe extension; ~200 LOC); read `docs/ghidra/FU317_r2l6_claim_acceptance.md` §4/§5 + its erratum and `docs/ghidra/FU314_r2l6_pan_triggers.md` + its erratum.

- [ ] **Step 1: RED** — the probe drive: the verbatim `M2_KEYS` + a DOWN hold from tape index 1820; assert the natural pan (the delta run: the camera place `0→−0x90` over 27 frames at f≈1514, the pan counter `0→4`, `tracked=2`, Y/Z=0, armer/follow=0) and the pins the erratum requires (`controlled=1`, `slot.entity=2`, the staged face `0x100` at the gate); the delta-run count, not the off-zero count.
- [ ] **Step 2: Implement** the probe extension; keep the natural pass's zero control; no camera staging.
- [ ] **Step 3:** PASS; `make check`; **M1 and M2 `cmp` byte-identical** (test-side).
- [ ] **Step 4: Commit** `test(fu317): the natural claim-pan witness`.

### Task 2: The row-`0x24` staged-face fidelity (FU314-L1a)

**Files:** `src/fifa96_engine/fifa96_match_handlers.c` (the staged-face sites `:2646`/`:5381`, the row-0x24 face); `fifa96_action_handlers.c` if the `out.tail` face path is the same cell; tests `tests/test_engine_match_frame.c`. Read `docs/ghidra/FU314_r2l6_pan_triggers.md` + its erratum (the native `0x865B5/0x865BE/0x865C6` constants; the port's staged 0).

- [ ] **Step 1: RED** — the staged face is the native row-0x24 constant (not 0); a frame-path pin where the face-aligned gate passes (the Δ `≤ 0xAB`), and the current zero-staged face fails it.
- [ ] **Step 2: Implement** the staged-face constants at the sites the wave names; the face-aligned pan route.
- [ ] **Step 3:** PASS; `make check`; **M2 `cmp`** — on a diff, capture the frame diff + the reason; re-pin only for the intended face correction (same commit) and record the new pin; otherwise STOP.
- [ ] **Step 4: Commit** `fix(fu314): the row-0x24 staged-face fidelity`.

### Task 3: The visible-pan acceptance (Layer 2)

**Files:** `docs/screens/p31-l2-pan-*.png` + the acceptance report; no production code. Read `docs/ghidra/FU317_r2l6_claim_acceptance.md` §6 + the phase-30 acceptance procedure (FU313 §6).

- [ ] **Step 1:** `make game` (`DISPLAY=:1`, the ISO): the phase-8 walkthrough to phase 2, then the FU317 drive (the DOWN hold at the tape-equivalent moment); the pan/still pairs at the same clock (`960x591+0+0`), the AE relative gate, the trace primary (`FIFA96_CAMERA_TRACE`).
- [ ] **Step 2:** Record the verdict: a visible pan (the camera place moves; the scene shifts) or the honest waiver with the exact timing limits; commit the captures + the acceptance note.
- [ ] **Step 3:** `make check` unaffected; M1/M2 unchanged.
- [ ] **Step 4: Commit** `docs(fu317): the visible-pan acceptance`.

### Task 4: The close-out (the natural-pan verdict)

**Files:** `docs/ENGINE.md` (the phase-31 register, append-only), `docs/PROGRESS.md`; no census/README change.

- [ ] **Step 1:** Append the phase-31 register: the natural-pan witness (the one-pan, zero-delta, 27-frame, `−0x90`; GREEN), the face fidelity (T2 outcome + the M2 status), the Layer-2 verdict, the armer NO-GO (the 13-pan ceiling; the no-slot L1 as a Stage-3/4 leg), the carried legs (FU314-L1a, FU316-L1/L9, FU315-L1/L2 out of phase, the follow waiver), and the quoted errata (the FU314/FU317 reconciliation; the row-6-self install correction; the controlled/slot pins).
- [ ] **Step 2:** `docs/PROGRESS.md`: the phase-31 narrative (the camera pans in natural play; the follow stays waived; the census holds).
- [ ] **Step 3:** final `make check`; M1 `09b726b7…`; M2 (identical or the T2 re-pin recorded); census check.
- [ ] **Step 4: Commit** `docs(phase31): the close-out — the natural-pan verdict`.
