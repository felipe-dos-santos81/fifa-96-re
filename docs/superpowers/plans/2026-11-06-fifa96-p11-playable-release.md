# P11 — playable Release build and kickoff placement (2026-11-06)

**Spec:** `../specs/2026-11-05-fifa96-playable-sdl-design.md` (P1/P5).
**Problem:** the windowed `fifa96` executable was built with ASan/UBSan. At
640x480 the per-pixel pitch caster calls `pitch_ray` ~300k times per frame;
ASan's fake-stack allocation dominated the frame. The match ran at ~2 fps, and
the 30-frame-per-second match clock appeared stuck at `00:00`. Separately, the
live bootstrap skipped the native kickoff cutscene and selected the nearest
record (0x199 from the ball) without moving it, so the opening KICK had no
carrier.

## Tasks

1. **Separate the playable Release build from the sanitized gate.**
   Add `FIFA96_SANITIZE_ENGINE` (default ON); make the engine, `fifa96_pccd`
   and the `fifa96` executable use the conditional flags. `make game` /
   bare `make run` configure `build-release` with `-O2` and sanitizers OFF;
   `make check` keeps the sanitized `build` tree.
   *Check:* `make game-build` produces `build-release/fifa96`; a live trace
   advances at 30 Hz.
2. **Place the seeded kickoff taker on the ball.** Add
   `match_run_live_control_place_taker`, call it after the successful seed
   bind and on the restart bind (with carrier seeding), gated on
   `formation_seeded` so the no-ISO/synthetic fixtures keep their legacy control
   flow. Live-only.
   *Check:* `test_engine_match_render` asserts the taker's position equals the
   ball; a release play-test shows the taker beside the ball and the clock
   ticking.
3. **Correct the rationale/comments.** The clock is phase-class driven; the
   taker placement is required for the opening KICK, not for the clock.
   *Check:* the live assertions and README wording agree.
4. **Verify and document.** Gate the sanitized tree; keep M1/M2 exact; record
   the release build in README/ENGINE/PROGRESS.

## Acceptance

- `make game` is the fast playable build; `make check` is the sanitized gate.
- `make check` **119/119**; M1/M2 exact by `cmp`.
- Release live trace: 355 frames and clock `00:11` after an 11 s run; the
  release binary has no `libasan`/`libubsan` dependency (567K vs 7.6M for the
  sanitized test executable).
- `docs/screens/p11-match-playable.png` shows the taker on the ball and the
  running clock (`00:02`).

## Out of scope in this slice

Native 3dwpts goal nets/stadium (P10-L2), full CPU AI/dribble/pass
(P9-L1/row-05 legs), Practise/Leagues/Tournaments/Playoffs/Load/Modem screens
(P2-L1), commentary and CRD music (A6-L4/L5). These remain numbered legs.
