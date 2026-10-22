# FIFA 96 M2 phase-30 — the natural reach (the first live camera bootstrap + the open-play event)

**Status:** scoped (the user skipped the brainstorming questions, 2026-10-21; "Plan the next phase", up to 4 recon agents). Binding on the phase-30 plans + Stage-2 execution.
**Parent:** `docs/superpowers/specs/2026-10-21-fifa96-m2-phase29-visible-follow-cam-design.md` (the visible follow camera). Phase-29 landed the live chain (the seam, the stand-ins, the keeper band, the chaser, the event chain) and proved it headlessly, but the visible camera was waived: in natural `make game` play every camera cell stays `(0,0,0)` — the chain never wakes (FU307-L1 / FU309-L1).

## 1. Goal

Make the follow/pan camera move in **natural** play: find and land the first honest live camera bootstrap (the route that takes the camera place or Y off `(0,0,0)` without a fixture stand-in) and the natural open-play event that raises the follow's `event_param > 0xF0` (or the pan's mid-band `0x20 < camY < 0x70`), so `make game` shows the camera re-centring/following. Layer-1 = the natural-play witness + the frame-path proof; Layer-2 = the visible capture with the reusable trace.

## 2. What phase 29 left (the binding starting state)

- The follow chain is live and fixture-proven: `fifa96_match_run_camera_follow` (`src/fifa96_engine/fifa96_match_run.c:3924`, the gates at `0x739C6..0x73B65`) + `fifa96_camera_rate_event` (the `> 0xF0` height gate) + the 54-frame window at camY `0x50` + the pan lockout/re-open (FU309; seven mutation riders).
- The camera-Y cell is single and live (`render.camera.pos_y`); the keeper-band raw stores (`0x75A0F` = `0x38`, `0x75EEA` = `0x10`, `0x75F86` = `0x80`) reach it (FU307-L3 closed).
- The chaser is corrected (`ramp_param = 1`, the live `+0x5D`, the int16 wrap; FU306-L2/L3/L4 closed) and the one-shot re-arm shape is pinned.
- The four `[0x157750]` stand-ins and the `follow_speed` seam are live (FU308; L2 closed).
- **The live camera never leaves `(0,0,0)`** in natural play: the phase-29 acceptance's 10-sample `gdb` trace and the second open-play burst keep every camera cell at zero; the tape probe measured `stage_select ran=0 cx1=0 cx4=0 table4=0` over 165 frames (FU307-L1).
- The AX prepass gate: `0x77F9A`/`0x77FA5` → `|cam_x| < 0x420 && |cam_z| > 0x7B0` (+ the sign/side arm), engine `fifa96_keeper_cpu_stage_select` (`src/fifa96_loader/fifa96_keeper_cpu.c:165`). With `(0,0,0)` the `|cam_z| > 0x7B0` term fails.
- The native kickoff reset is `(0,0,0)` too (FU157 §1: `0x88E4B..0x88E6A` pushes the static triple `[0x10F328/2C/30]` = 0 and `CALL 0x700F4` with flag 0) — the zero start is native, not a port bug.
- The restart cuts that place the camera are ported: the goal-kick/restart cut (`fifa96_match_handlers.c:1690`, `0x856CB`, `goal_snap`), the throw-in stage-3 (`handlers.c:1752`, `0x85919` Y = `0x80`, Y preserved), the corner cut (`handlers.c:1837`, `0x85E98`, ±`0x710`/0/±`0xB00`), the free-kick cut (`handlers.c:1983`, `0x83E72`, the incident triple, y = 0), the penalty cut (`handlers.c:2107`, `0x84B6B`, 0/0/±`0x8D0`). Their dispatch reach in the port is the open question (FU163: the throw-in row `0x10` needs situation 2 → phase 3; the queue consumer is unported).
- The airborne kick route: FU153 §5 (the height-table code-4 receive, `d = camY − rec.pos_y >= 0x38`, no band precondition; the mode-`0x40` draw `0x90..0xE0`/`0x100`); its live selector is T1-L1 (the jump row) — unported.
- The pan's mid-band gate is strict `0x20 < camY < 0x70` (FU168 §2.2); the every-frame integrator is `fifa96_camera_update_walk` (`match_run.c:4370`).
- The trace (FU309-L5), the AE baseline (FU309-L6) and the manual riders (FU309-L9) remain legs.

## 3. Scope

**In — the four recon slices:**

1. **FU310 — the camera bootstrap.** Every live natural route that writes the camera place/Y: the restart cuts above (goal kick, throw-in, corner, free kick, penalty), the kickoff reset, the keeper band, the pan integrator. For each: the native dispatch/gate, the port's reach (the situation/phase machine, the row-`0x10` queue consumer, the restart rows), and the cost to make one live. GO/NO-GO on the cheapest honest bootstrap in natural play.
2. **FU311 — the keeper ladder's natural CX=4.** Given a bootstrap place: the AX gate's terms, the CX=4 predicates (`cam_y == 0`, an opposite-side actor, the lane), the stage-select call path (why `ran=0` on the tape), the row-`0x1E` dispatch and the band Y. GO/NO-GO on the ladder route, with the exact natural preconditions.
3. **FU312 — the natural event route.** The kick rail → the code-4 receive row → the tall traj (`> 0xF0`); the airborne height-table route and its live jump selector (T1-L1); the ball machine's natural pass/kick/claim states; what a natural kickoff/kick/open-play pass produces. GO/NO-GO on a natural follow-waking event.
4. **FU313 — the acceptance harness.** The env-gated camera-cell trace (FU309-L5, default-off), a headless natural-play probe (kickoff → an input script → the stage-select/camera/event samples), the Layer-2 capture procedure (same-clock pairs, the AE relative gate), and the attribution rules.

**Out:** the census push; the R11-L4 remainder; the renderer orientation (R2-L1); FU306-L1; the FU-28 selectors; the FU309-L9 mutation-harness automation; any set-piece port beyond the one bootstrap route the wave selects (the rest stay numbered legs).

## 4. Structure

**Stage-1 (recon-ahead):** a four-slice read-only wave, FU310–FU313, one agent per slice (the up-to-4 cap), each a draft `docs/ghidra/FU31N_r2l5_<slug>.md` (single H1, Phase/Provenance line, port shape, evidence table, numbered legs, GO/NO-GO verdict); promoted one-per-commit; read-only Ghidra discipline (fresh reads, the alias traps, LE globals `+0x100000`, unprovable ⇒ a numbered leg with its native owner). No production code. A whole-wave review + errata before Stage-2.
**Stage-2:** a producer plan written after the wave promotes (task text from the FU310–FU313 verdicts), executed via subagent-driven development — the bootstrap first, then the event route, then the keeper route (if it is the chosen one), then the acceptance + close-out.

## 5. Acceptance

- **Layer-1 (required, the hard gate):** a natural-play witness — a headless run from the real kickoff with a scripted input sequence (no staged camera cells; restart/kick inputs may be scripted) in which the camera place/Y leaves `(0,0,0)` and the follow moves it over a sustained window — plus a frame-path fixture that reaches the same chain (staging the producer inputs, never the camera gates), with mutation-RED riders per producer. The witness must show the **natural** route, not only the fixture.
- **Layer-2 (goaled):** `make game` (`DISPLAY=:1`) shows the camera visibly following; the same-clock pan/still captures; the env-gated trace is the primary attribution (FU309-L5), the AE the relative corroboration (FU309-L6).
- **Invariants:** M1 `09b726b7…` immutable; M2 `b281a9b2…` byte-identical through the staged tasks — re-pinned only with a written reason + frame diff (a live path that legitimately moves the tape stops the task for that diff first); the census holds `60/80` unless a row is proven live (the four-doc same-commit rule); the live-chain rule (no staged gates).

## 6. Honest-stop rule

If no bootstrap route can be made live without an unported machine (the restart-phase queue, the jump selector) and no natural event can be raised honestly, the phase **stops and reports the route analysis**: the bootstrap census, the exact blockers, and the new numbered legs. A no-visible-follow close with the evidence is a valid end; the partial producers that are faithful (the bootstrap cut, the jump selector) may still land.

## 7. Decisions (this spec)

- The phase is the **natural reach**, continuing the phase-29 waiver line (the user skipped the questions; the standing goal is the visible follow camera).
- The bootstrap may be a restart; the porting scope is bounded to the one route the wave proves cheapest/honest.
- The keeper-band route (FU307-L1) is one candidate, not a requirement — the visible goal is the gate.
- The process is the repo's phase shape: spec → four-slice recon wave (the cap) → the whole-wave review → the producer plan → SDD, on `main`.

## 8. Builder-check decisions (the calls made)

1. **The bootstrap may be a restart.** Porting the restart-phase queue (situation 2 → phase 3) is in scope **only** to the extent the chosen bootstrap needs it; the rest of the restart machine stays out. If the queue is the only route, Stage-2 lands it as one bounded task and the other restarts stay legs.
2. **The keeper route is optional.** If a restart + event wakes the follow without the ladder, the keeper band stays fixture-proven and FU307-L1 stays a leg; the phase does not force the CX=4 route.
3. **The natural witness is the headless probe.** A scripted input run from the real kickoff (no camera staging) is a legitimate Layer-1 witness; the fixture stages producer inputs only, never the camera gates. The probe is a test-side tool (env-gated/opt-in), not production behavior.
4. **The trace is the primary Layer-2 attribution.** Env-gated, default-off (FU309-L5); the AE gate stays relative (FU309-L6), same-clock pairs.
5. **No new framework.** The wave uses the existing Ghidra MCP discipline; the phase uses the existing test style (frame-path fixtures + mutation riders).
