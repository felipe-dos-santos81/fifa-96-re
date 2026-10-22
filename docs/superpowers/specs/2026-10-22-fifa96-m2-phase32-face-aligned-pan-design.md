# FIFA 96 M2 phase-32 — the live record face and the non-zero-delta pan (FU314-L1a)

**Status:** scoped (the user's "skip questions" + "plan the next phase", 2026-10-22; the phase-31 close-out leaves FU314-L1a as the camera's next named link). Binding on the phase-32 recon wave + producer plan + Stage-2 execution.
**Parent:** `docs/superpowers/specs/2026-10-22-fifa96-m2-phase31-natural-claim-pans-design.md` (the natural claim pans). Phase-31 proved the natural row-04 pan once — but at the **zero-delta knife edge**: the record sits exactly on the parked camera, the bearing folds to the `0x80` default, the face is the step-12 `0x100`, Δ`0x80`. The measured undriven approach from the east fails the `0x7EE7D` gate (face `0x100` vs bearing `0x300`, Δ`0x200`, 2,081/2,081 dispatches). The remaining leg is **FU314-L1a**: a natural pan where the record is **offset** from the camera at the gate — a real bearing — either by geometry (a west-side band entry, FU314 R2) or by a live face producer that points the record at the camera (FU314 R1/L3/L5).

## 1. Goal

Prove a **natural non-zero-delta** row-04 pan: a code-4 record inside the `0x40` band at an offset from the parked camera, whose live `+0x7D` face is within `±0xAB` of the real camera bearing, fires the half-line arm and moves the place — with no staged camera cells and no staged face. Land the face-producer fidelity the leg names — **FU314-L5** (the mover `direct_face` staged 0) and **FU314-L3** (the `+0x7D` producer coverage) — only as far as the wave proves it is the natural producer for that witness (or the robustness route), each with its own RED→GREEN pin and its M2 accounting.

## 2. What phase 31 left (the binding starting state)

- The natural pan is real and visible once: claim `f=1514`, gate `f=1512`, event `f=1513`, one delta run `f=1514..1540` (27 frames), stop `f=1541` at `−0x90`, pan `0→4`, `trk=2`, Y/Z=0; the full trace 1 header + 14 984 body lines; the captures `docs/screens/p31-l2-*`. The pass is the **zero-delta crossing** (`pos == cam` at the gate; bearing `0x80`, face `0x100`, Δ`0x80`).
- The row-04 staging already reads the **live record face** (`handlers.c:2654`, FU314-L2) and the row-0x24 face is the native `(1,0)` (`handlers.c:5394`, FU314-L5a). M1/M2 unmoved (`09b726b7…` / `8434cfab…`).
- The record's face is otherwise **static**. The mover stages `+0x43 direct_face = 0` (`match_run.c:498`) so the locomotion turn (`fifa96_action_handlers.c:219-246`) never rotates it; `fifa96_arm_face` writes only the octant (`fifa96_arm_helpers.c:73-94`) while the native `0x79C50` writes **both** `+0x7D` and `+0x8E` at its 40 call sites (the zero-direction early return writes neither); the per-frame frame-wrap arm (`0x8E1CB`, anim row byte `≥0x70`, vector `−off[+0x8E]` from `0x10F334/0x10F33C`) is unported (FU314-L3).
- The measured geometry: face `0x100` (+x, the step-12 row-0x24 constant) passes the gate only when the record is **west** of the parked camera (`x<0`, `|z| ≤ |x|·tan 60°`) — FU314 **R2**, no code change — or when a live producer points the face at the camera (**R1**) or arm A's `0x13` row byte fires (**R3**, dead in the measured window).
- The pan's other cells: the claim/pan ordering (event `f=1513` precedes motion `f=1514`); the `is_ball_track` controlled fallback (`handlers.c` ~`:2680`) refuses a controlled record; the armer/follow stay out (FU316-L1/L9; the `>0xF0` waiver).

## 3. Scope

**In — the four recon slices:**

1. **FU318 — the `+0x7D` producer census (FU314-L3).** The 40 native `0x79C50` sites, each classified ported / split / unported; the `0x8E1CB` frame-wrap arm (the `≥0x70` gate, the `−off[+0x8E]` vector, the `0x10F334/0x10F33C` tables); the locomotion turn (`0x8E29C`, the `+0x43` byte, `fifa96_action_handlers.c:219-246`); which producer actually runs for a walking code-4 record in natural play. GO/NO-GO on a minimal faithful landing and its M2 risk.
2. **FU319 — the mover's `direct_face` (FU314-L5).** The native `+0x43` producer and semantics, the port record field (or its absence), the `match_run.c:498/532` staging, the locomotion turn's `move_attr > 0x60` / `face_x`/`face_z` branches; the exact port shape and the expected sim diff (gates/AI/ball pairing). GO/NO-GO on staging the live byte; the M2 frame-diff expectation.
3. **FU320 — the non-zero-delta natural drive (R1/R2).** The gate algebra for an offset record (the west-side cone; the live-face variants); which drives can bring a code-4 record into the band at a non-zero offset with face ≈ bearing — the T1 hold-length/start variants, a west-side crossing, the mirrored side / second half, a restart, an over-running defender; the measured tuples. GO/NO-GO on a no-code R2 witness; if none, the minimal producer change that creates one.
4. **FU321 — the acceptance and the golden risk.** The Layer-2 pan/still procedure for a non-zero-delta pan (the place delta from the seeded velocity, not the zero-delta default; the trace cells; the same-clock pairs; the AE relative; the bbox caveat); the M2 policy for a live face (the frame diff + the written reason + the register pin); the controlled-fallback and claim-ordering hazards; the legs.

**Out:** the follow/Y armer route (FU316-L1/L9; the `>0xF0` waiver); the claim tail (FU315-L1/L2); the set-piece machine; the census push (no row joins); the renderer pitch/pitch rendering; FU307-L8; the FU316-L2..L8 fidelity legs except as the witness touches them.

## 4. Structure

**Stage-1 (recon-ahead):** a four-slice read-only wave, FU318–FU321, one agent per slice (the up-to-4 cap), each a draft `docs/ghidra/FU31N_r2l6_<slug>.md` (single H1, Phase/Provenance line, port shape, evidence table, numbered legs, GO/NO-GO verdict); promoted one-per-commit; read-only Ghidra discipline (fresh reads, the alias traps, LE globals `+0x100000`, unprovable ⇒ a numbered leg with its native owner). No production code. A whole-wave review + errata before Stage-2.
**Stage-2:** a producer plan written after the wave promotes (task text from the FU318–FU321 verdicts), executed via SDD — the minimal producer slice first (only if the wave proves it necessary), then the witness, then the acceptance + close-out. If FU320 proves R2, the plan lands a **drive-only witness** and the producers become the robustness/fidelity slice with their own pins.

## 5. Acceptance

- **Layer-1 (required, the hard gate):** a natural-play witness — a headless run from the real kickoff with a scripted input drive (no staged camera cells, no staged face) in which the row-04 pan fires with the record at a **non-zero offset** from the camera at the gate (a real bearing, `pos ≠ cam`), the place leaves `(0,0,0)` over at least one pan, and the delta-run count separates motion from the off-zero count; plus a frame-path fixture that reaches the same chain (staging only producer inputs), with mutation-RED riders per producer. If a producer (L5/L3) is landed, its own pin must show the live cell (the record face follows the producer) and the pre-fix behavior must fail.
- **Layer-2 (goaled):** `make game` (`DISPLAY=:1`) shows the non-zero-delta pan (the place moves; the same-clock pan/still captures; the trace primary, the AE relative corroboration; the bbox caveat may persist with the sprite-only render).
- **Invariants:** M1 `09b726b7…` immutable; M2 `8434cfab…` is the baseline — it may move **only** for a proven faithful producer change, with the frame diff + the written reason + the register pin in the same commit; the census holds `60/80` unless a row is proven live (the four-doc rule); the live-chain rule (no staged gates); the follow/Y stays waived.

## 6. Honest-stop rule

If no natural drive can produce a non-zero-delta face-aligned admission (the west-side crossing is unreachable, no live producer runs), or the producer fixes cannot be landed without an unjustifiable M2 move, the phase lands the bounded slice it proves — the producer census, the drive negative result, the exact blocked link — and records the remainder as numbered legs. A NO-GO close with the route analysis is a valid end; a faked witness is not.

## 7. Decisions (this spec)

- The phase is **FU314-L1a** — the camera's next named link after the phase-31 one-pan zero-delta witness; the visible goal stays the pan (X/Z), not the follow (Y).
- The **live face** (L5/L3) is in scope because the leg names it as L1a's owner and because a static face makes the route geometry-luck; it lands only as far as the wave proves it faithful and golden-safe.
- The **drive route (R2)** is tried first: if a no-code west-side crossing exists, it is the cheapest honest witness, and the producers become the robustness slice with their own pins.
- The M2 policy is **faithful-move**: a producer that changes the sim may move M2 with a written reason + frame diff; it may not be forced to keep M2 by an unfaithful staging.
- The process is the repo's shape: spec → four-slice recon wave (the cap) → whole-wave review → producer plan → SDD, on `main`.

## 8. Builder-check decisions (the calls made)

1. **The non-zero-delta witness is the hard goal; the producers are conditional.** A drive-only R2 witness satisfies L1a; the producer work then lands only if the wave shows it is the natural/robust route and the M2 diff is faithful.
2. **The face producer is the record's live face, not a staged constant.** No fixture may stage `face7d` to force the gate; the fixture stages the producer input (the record byte/target) as FU317 allowed.
3. **The M2 diff is allowed for fidelity, not convenience.** A producer change that moves M2 must show the moved frames are the intended face/gate consequences; otherwise stop.
4. **The armer/follow stay out.** No FU316 work, no armer probing, no `>0xF0` invention.

## 9. Builder-check answers (the calls made)

1. **The existing T2 unit pin is exempt from the no-staged-face rule.** `test_row04_live_face_passes_face_aligned_gate` stages `face7d` and the gate cells deliberately, as a unit pin of the face path; it stays, labelled as a unit pin. The **new** phase-32 acceptance fixture may not stage `face7d` or the camera/gate cells; it stages the producer input only.
2. **The L5 slice includes what the wave proves the locomotion turn reads.** If FU319 finds the native turn also needs the `0x57A73` focus point (or another cell), that belongs to the minimal producer slice when it is in the native locomotion-turn chain; otherwise it is a numbered leg and the L5 pin is deferred to it. No half-faithful turn lands.
3. **A period-staged witness is allowed only if the wave proves R2 needs it.** The witness starts from the real kickoff by default; staging the period/clock is a match-setup input (declared), never a camera/face gate. If the second half is the only route, FU320 must show it and the witness documents the staged period.
4. **L5/L3 are conditional phase-32 tasks with a land/revert gate.** The producer plan carries them only if FU318/FU319 return GO and the Stage-2 M2 diff is faithful; otherwise they are legs and the phase closes on the drive-only witness.
