# FIFA 96 M2 phase-7 recon-ahead + phase-6 wave-2 execution

> Executes `docs/superpowers/plans/2026-10-08-fifa96-m2-full-gameplay.md` S1–S5 (serialized) and, concurrently, a phase-7 recon-ahead wave (parallel, read-only) over the UNSUP long tail + residual unported chains. Approved design: two tracks, one wall-clock.

**Baseline:** `make check` 105/105; wired 14/80; tail `03355ea`; goldens M1 `09b726b7…`, M2 v5 (`8c2cf55` re-pin: 160 hash-only lines, first diff frame 6).

## Track A — serialized ports (critical path)

Order S1 → S2 → S3 → S4 → S5, each from its frozen slice, SDD loop as established (brief → implementer → task review → fix loop → controller verify (`make check`, tape `cmp`, tree clean) → push).

- **S1: possession/locomotion** — from `docs/ghidra/FU-147_possession_locomotion.md` (verdict: yes; lane writer `0x7C7AF`, `+0x8D` seed `0x8C329`, driver `FUN_0008D8EC`, situation-0xB producers, possession flips). Port the reachable subset; pad-driven possession changes in a headless fixture; golden re-pins only with reason + frame diff.
- **S2: goal arming** — from `docs/ghidra/FU-145_goal_arming.md` (armer `FUN_0007131C`, classifier `FUN_00070074`, scanner `FUN_00088940`, queue arm).
  **Ruling (approved):** S2 lands the armer against a fixture-driven pan; the pan producer (`FUN_00071C94` lead in FU-148 §2.1(c)) completes in S4 if it proves camera-scoped — S2/S3 proceed on the seam, no reordering.
- **S3: goal consumers** — from `docs/ghidra/FU-146_goal_consumers.md` (queue `0x8A8E0`, scheduler `FUN_000948AC`, installer `0x92D8C/0x92E2C`, reachable leg handlers → `fifa96_match_run_score_event`). Shared 0xB entry point per the freeze reconciliation (no parallel mechanism; `fifa96_match_run_situation` stays table-2/row-01).
- **S4: presentation completion** — from `docs/ghidra/FU-148_presentation_hud_camera.md` §2–§4 (camera legs FU-96 1/3, FU-71 event bodies, formation-id producer, palette translation tables) as reachable.
- **S5: acceptance v6** — possess → score → restart → period end; deepest-reachable honesty rule; every remaining forcing listed with its leg; fresh smoke; whole-phase review.

Phase-6 gate: milestone DoD met (or deepest reachable chain, full legs), tape v6 green, smoke honest, M1 immovable.

## Track B — phase-7 recon-ahead (parallel, read-only, 4 agents)

Same discipline as wave 1: Ghidra read-only; each agent writes ONLY `docs/ghidra/drafts/w7-<n>-*.md`; evidence floor (first-hand bytes/addresses, fresh xrefs for census claims, numbered legs, port contract); controller freeze review then canonical FU promotion in a serialized commit. No `src/`/`test/`/shared-doc edits, no commits.

- **B1 set pieces & restarts** — throw-in/corner/goal-kick/free-kick dispatch rows + their situation producers (extend FU-146 legs 1–4); restart placement/state.
- **B2 fouls / referee / offside** — the foul/offside/referee rows and their match-state effects (cards, free-kick awards, whistle chain).
- **B3 goalkeeper + AI team logic** — row 1D/1E stage flows beyond FU-147, keeper claim/restart chain bodies, the non-controlled mover's target selection (beyond the FU-147 lane/track seam).
- **B4 presentation residual** — replay/cutscene rows, residual HUD overlays (presenter siblings of `FUN_00055C24`), camera handlers `0x108B80[0..3]` bodies, formation-id producer wiring details, palette translation pool identity.

**Track B gate:** all four drafts frozen (evidence floor passed) → phase-7 ports planned from them in a later cycle.

## Constraints (both tracks)

- Evidence floor + numbered legs; no stale census (fresh xrefs for any "only/sole/exactly"); `/FIFA96.EXE` authoritative; +0x100000 rule; CALL-byte/misread traps.
- Determinism: null backend source of truth; M1 never moves; M2 re-pins only for intended upgrades with written reason + frame diff.
- Track B never touches code; Track A never starts from an unfrozen slice; controller serializes commits.

## Ledger

`.superpowers/sdd/2026-10-08-fifa96-m2-full-gameplay/progress.md` (S1..S5 + wave-7 lines; BASEs recorded per task).
