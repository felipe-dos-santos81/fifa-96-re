# FIFA 96 native engine — M2 child: playable match

Date: 2026-10-07
Status: approved for planning (user chose "full M2 child; write spec → plan → execute")
Parent spec: `docs/superpowers/specs/2026-10-06-fifa96-native-engine-port-design.md` (§8 M2, §12 split)

## 1. Context

The parent plan delivered M1 (boot → intro → front-end, `make game` windowed via SDL3)
and the M2 foundation: match lifecycle, 30 Hz frame body, input → `fifa96_control_slot`
per granted frame, and the deterministic camera/scene/sprite render chain. `make check`
is 94/94.

The scope probe (`docs/ghidra/FU136_action_handler_port_scope.md`) measured the
remaining surface: **80 dispatch rows** — action table `0x1106E0[45]` (sole reader
`FUN_0007D9A4@0x7DA77`) and phase table `0x110794[35]` (sole reader
`FUN_0006D920@0x6D9B3`) — of which **0 wired / 3 unwired / 77 not ported** (43 with
partial tested helpers), plus ~20 support items (installer/dispatch mechanics, record
machines, input handler, ball staging/resolver, frame chain, RNG). Estimate ~13 tasks
(range 12–15) → `M2_SCOPE: split` per parent spec §12.

## 2. Goal

A **playable match**: from the front-end, start a match, control players, kick,
score, reach half/end, and return to the front-end — windowed on Linux/macOS via
SDL3, and deterministic headlessly.

### Non-goals

- Bit-exact AI fidelity (the port follows the derived dispatch/bodies; uninferable
  values become open legs).
- Perfect front-end art (waived for M1; the child may add the missing-art RE slice
  only if it is cheap).
- Runtime-capture-blocked legs (TGV companion queue, CRC producer).

## 3. Workstreams

1. **Match bridge & asset staging** — front-end confirm/select → `fifa96_match_run_begin`
   (reconciled with the FU-66 dispatch rather than the current direct call); stage the
   match assets (animation frames/banks, `sprite_data`, window box) from the ISO; make
   MATCH mode reachable in the running game.
2. **Match completion** — set `period_length`/`extra_length` (today `state_init` marks
   OVER ~1 s after begin); resolve OVER→POST→EXIT; guard the frame-body period-end
   against a staged `request_exit` (parent final-review hazard); score/half/end flow.
3. **Handler port clusters** (from FU136's table; each cluster = RE pass → FU doc → C
   port + tests):
   - **A. Locomotion / move / control action codes** (action `00`-family, `0x0A`–`0x13`).
   - **B. Ball** — staging, resolver, pairing, kick/trajectory, possession/tackle.
   - **C. Keeper** — bodies, hold/guard/intercept, dispatch rows.
   - **D. Outfield** — decide/chase, ranked selection.
   - **E. Entity update chain + input handler + RNG** support items.
   - **F. Phase drivers** — the 35 phase-table rows (events/sequences/timelines).
   - **G. Dispatch mechanics** — `FUN_0007D9A4` argument staging, record machines,
     installer arms (`0x26`–`0x2C` region per FU136).
4. **Render completion** — wire the entity animation banks into the render chain;
   close/record the parent Task 15 open legs (near-depth threshold, lateral cull
   `> 0x8E0`, palette remap identity, HUD/overlays, 23-vs-24 slot list).
5. **Acceptance** — scripted kickoff → move → kick → score → half/end → exit tape with
   pinned frame/state hashes; a windowed smoke pass via `make game`.

## 4. Method

Subagent-driven development exactly as the parent: per task a fresh implementer, a
task review (spec + quality), a bounded fix loop, `make check` green after every task,
ASan/UBSan on engine tests, evidence-gated Ghidra work (read-only; FU docs appended,
prior rows never rewritten), Ghidra writes/commits serialized by the controller.

New FU documents use the next free numbers (FU137+), one per RE cluster.

## 5. Acceptance criteria

- **M2-A (interactive):** `make game` → front-end → start match → kickoff; the player
  moves players, kicks, and scores; the match reaches half/end and returns to the
  front-end.
- **M2-B (headless):** a scripted input tape replays the same sequence to identical
  frame/state hashes, pinned in `tests/golden/engine/m2-frames.txt`.
- `make check` green (94 + new tests) with all engine tests under ASan/UBSan.

## 6. Risks

- **RE depth** — 77 rows with unbounded sub-regions (`0x26`–`0x2C` flagged in FU136);
  mitigate by porting cluster-by-cluster with a scope probe per cluster, and by keeping
  unimplemented rows as named open legs rather than guesses.
- **Runtime-gated semantics** — `[0x57A4A]` / phase timelines (FU136 concern 3); the
  controller may schedule a capture leg rather than guess.
- **Hash churn** — wiring entities/animations will move all pinned render hashes; each
  task that changes the render path re-pins and documents why.
- **Input fidelity** — KICK/PASS are the closest-derived `0x10/0x20` (parent Task 14);
  confirm against FU-61/Ghidra before they drive the match.
- **Scope creep** — if a cluster's probe exceeds ~4 tasks, split it into its own plan
  (parent spec §12 rule applies recursively).

## 7. Decomposition note

The child plan is expected to be large (~13 tasks). It is written as one plan with
milestone gates: **G1** bridge/staging/completion, **G2** mechanics clusters A/B/C/D/G,
**G3** phase drivers F + render completion, **G4** acceptance. Gate G1 must land first
(it makes the match reachable); the other gates may parallelize their RE probes but
serialize their code and commits.
