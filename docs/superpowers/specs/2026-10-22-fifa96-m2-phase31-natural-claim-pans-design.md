# FIFA 96 M2 phase-31 — the natural claim pans (the camera walk upstream)

**Status:** scoped (the user's "proceed", 2026-10-22; the phase-30 NO-GO leaves the camera's first broken link as the natural pan trigger). Binding on the phase-31 plans + Stage-2 execution.
**Parent:** `docs/superpowers/specs/2026-10-21-fifa96-m2-phase30-natural-reach-design.md` (the natural reach). Phase-30 proved the camera cannot bootstrap: the only live place writer is the row-04 half-line claim pan (scripted witness `0→0x90→0x120`, one-shot, Y-less), and natural play never fires it — the 12,000-step probe kept every camera cell at zero. The upstream link is the **claim/possession chain** that arms the row-04 pan.

## 1. Goal

Make the camera move in **natural** `make game` play through the native row-04 half-line claim pans — the pan that the scripted witness proves live but the natural probe never triggers — and measure whether repeated claims walk the camera place toward the armer (`|x| > 0x730` / `|z| > 0xB20`), the goal-mouth scanner, and the set-piece places (the next hop). The follow (the Y/`>0xF0` event) stays downstream and is **not** promised this phase.

## 2. What phase 30 left (the binding starting state)

- The camera cells stay `(0,0,0)` in natural play; the phase-30 register records the NO-GO and the full route analysis (`docs/ENGINE.md`, the phase-30 section).
- The only live place writer is the row-04 half-line claim pan: the gate (`0x7ED89..0x7EDE3`: a record within `0x40 - timer81` of the camera, `lane < 0x90`, `≤ bound`, `camY ≤ 0x50`) → `fifa96_camera_event_set` + `fifa96_camera_set_tracked` (`handlers.c` ~`:2741-2752`) → `fifa96_camera_update_walk` integrates `acc_x/acc_z` (`camera.c:110-111`; `match_run.c:4370`). Scripted witness: `0→0x90→0x120`, two pans, Y=0 (FU310 §6, FU313 §5).
- The pan is **one-shot per claim** and the tracked record is excluded from the next arm (`0x7EFEE`, `tracked = 10` after the second pan); the fusion of repeated claims and the tracked lifecycle is the open question (FU310-L9's spike plateaued at `0x120` with the scripted drive; the natural drive was never tested).
- The phase-30 probe (T1) measured 3,598 granted frames with zero camera cells and 6,118 ladder calls / `ran=0`; the pan witness is the only motion.
- The armer/scanner: `fifa96_match_goal_arm` (`match_run.c:3155-3191`) needs `|cam.x| > 0x730 || |cam.z| > 0xB20`; the scanner `FUN_00088940` → situations 2/3/4; the set-piece dispatch is direct (session 0), the phase arm is ported, the T4 restart places are fixed.
- The possession/claim producer: the keeper bind's P3 possession cell is unported (phase-28 register); the claim cells (`has_slot`/`0x73B65`, the record `+0x20`) are partially live; the row-04 gate's `lane`/`bound`/`timer81` inputs are live cells.
- The harness (T1): the default-off `FIFA96_CAMERA_TRACE`, the natural probe, P1–P10 (P10 automated), P6 partial, P7/P8 RED-by-design.
- The camera follow needs a `>0xF0` event + a tracked moving record; the phase-30 kind-3/offside route is default-disabled (`field_4c2f2 = 0`), and the free-kick/penalty act-2 hand-off aborts under the ordinary session 0 — the follow stays out of scope.

## 3. Scope

**In — the four recon slices:**

1. **FU314 — the row-04 pan's natural triggers.** Every native trigger of the half-line pan (the claim/kick/tackle rows, the `0x7ED89` gate, the event/tracked bind), its inputs (`lane`, `bound`, `timer81`, `camY`), and why the 12,000-step natural probe produced no pan. GO/NO-GO on a natural pan with the engine's live cells.
2. **FU315 — the possession/claim chain.** The ball-pair possession states, the claim cells (`has_slot`, the record `+0x20`, the `0x73B65` bind), the P3/P6/P8/P12 producers (the phase-28 register), and the native/engine map. GO/NO-GO on a natural claim that arms the row-04 pan; the bounded port shape.
3. **FU316 — the pan re-arm + the camera walk.** The tracked lifecycle (`0x7EFEE` exclusion, `0x71D27` bind, the clears), the pan decay/stop, and whether repeated natural claims accumulate the camera place to the armer/scanner/set-piece places. The measured chain; GO/NO-GO on the camera walk.
4. **FU317 — the acceptance harness.** Extend the natural probe to drive the claim chain (the input script + the AI), sample the camera place/pan count/tracked state/armer, and design the `make game` capture + the pan/still AE; the Layer-2 attribution (the trace primary).

**Out:** the camera follow/Y/event chain (the phase-30 verdict; the offside/kind-3 route; the act-2 hand-off); the set-piece machine beyond the camera-place consequence; the R11-L4 remainder beyond the claim/possession slice the wave proves necessary; the census push; the renderer orientation; FU311-L5a's rows (the lane deferral stays); the FU307-L8 stage-3/7 X/Z leg.

## 4. Structure

**Stage-1 (recon-ahead):** a four-slice read-only wave, FU314–FU317, one agent per slice (the up-to-4 cap), each a draft `docs/ghidra/FU31N_r2l6_<slug>.md` (single H1, Phase/Provenance line, port shape, evidence table, numbered legs, GO/NO-GO verdict); promoted one-per-commit; read-only Ghidra discipline (fresh reads, the alias traps, LE globals `+0x100000`, unprovable ⇒ a numbered leg with its native owner). No production code. A whole-wave review + errata before Stage-2.
**Stage-2:** a producer plan written after the wave promotes (task text from the FU314–FU317 verdicts), executed via subagent-driven development — the claim chain first, then the pan/fusion, then the acceptance + close-out.

## 5. Acceptance

- **Layer-1 (required, the hard gate):** a natural-play witness — a headless run from the real kickoff with a scripted input sequence that produces a claim and fires the row-04 pan (no staged camera cells; the claim inputs may be scripted), in which the camera place leaves `(0,0,0)` over at least one pan — plus a frame-path fixture that reaches the same chain (staging the producer inputs, never the camera gates), with mutation-RED riders per producer. The witness must show the **natural** route, not only the fixture.
- **Layer-2 (goaled):** `make game` (`DISPLAY=:1`) shows the camera panning in natural play (the place moves; the follow/Y remains waived unless the probe proves it); the same-clock pan/still captures; the env-gated trace is the primary attribution (FU309-L5), the AE the relative corroboration (FU309-L6).
- **Invariants:** M1 `09b726b7…` immutable; M2 `8434cfab…` byte-identical through the staged tasks — re-pinned only with a written reason + frame diff; the census holds `60/80` unless a row is proven live (the four-doc same-commit rule); the live-chain rule (no staged gates).

## 6. Honest-stop rule

If the claim/possession chain cannot be raised without the R11-L4 remainder (or a producer the wave proves unportable in this phase), the phase **lands the bounded slice** it proves (the claim trigger/pan re-arm fixture) and records the remainder as numbered legs; a natural-pan proof is the goal, the follow stays waived. A close with the route analysis and the legs named is a valid end.

## 7. Decisions (this spec)

- The phase is the **natural claim pans** — the camera's first broken link after the phase-30 NO-GO; the standing goal (a visibly moving camera) is served by the pan (X/Z) even without the follow (Y).
- The possession/claim chain is in scope only as far as the row-04 pan needs it; the broader R11-L4 remainder stays out.
- The armer/scanner/set-piece consequence is measured, not promised.
- The process is the repo's phase shape: spec → four-slice recon wave (the cap) → the whole-wave review → the producer plan → SDD, on `main`.

## 8. Builder-check decisions (the calls made)

1. **The natural witness is the probe.** A scripted input run from the real kickoff (the claim drive; no camera staging) is a legitimate Layer-1 witness; the fixture stages producer inputs only, never the camera gates.
2. **The pan is the visible goal; the follow is a probe.** A visible pan (the place moving) satisfies this phase's Layer-2; the follow/Y stays the phase-30 waiver until a `>0xF0` producer is honest.
3. **The claim chain is bounded.** The R11-L4 possession remainder is in scope only for the claim slice the wave proves necessary; the rest becomes numbered legs.
4. **The spike's plateau is a drive limitation, not a proof.** FU310-L9 closed the *scripted* reach; a natural drive (the AI's claims) may fuse further — the wave must test it, not assume either way.
5. **No new framework.** The wave uses the existing Ghidra MCP discipline; the phase uses the existing test style (frame-path fixtures + mutation riders + the T1 harness).
