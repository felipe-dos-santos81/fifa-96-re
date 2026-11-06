# P9 implementation plan — contestable shots

**Spec:** `../specs/2026-11-06-fifa96-p9-shot-contest-design.md`.
**Recon:** `/tmp/opencode/play/P8-ai-recon.md`, `P9-shot-report.md`,
`P9-fix2-report.md`.
**Base:** `312029c`; final code gate at `4cca8cd` + docs.

This plan is the retrospective process record for the P9 wave, added after the
P9 code review found the slice had no SDD plan/spec. Execution order below is
the order the work actually took.

## Task 1 — aimed shots

- Add the live aim latch: Up/Down depth on a real edge, ±0x80 target x, centre
  default; clear on latch/save/flight end; no score/phase writes.
- Files: `fifa96_match_run.c/.h`.
- Check: target-x tests (Up, Down, both, stale two-kick), side-1, no score.

## Task 2 — live keeper save

- Add the defending-keeper save before the goal armer: goal-side, reach
  0x40, near 0x180, mouth bounds, pin 30 frames; no score/phase/goal writes.
- Preserve the P8 defender block and opponent contest.
- Files: `fifa96_match_run.c/.h`, tests.
- Check: centre save, Up/Down aim goal, displaced keeper, behind-ball,
  mouth-edge, no double goal; full-time path.

## Task 3 — held long ball

- Add LONG_BALL to the SDL3 held-code set; keep the release edge and menus
  unchanged.
- Files: `platform_sdl3.c`, `test_engine_sdl3.c`, `test_engine_match_input.c`.
- Check: held V/West re-presents; a match-level LONG_BALL reaches `0x40`.

## Task 4 — controls/docs/acceptance

- Controls page: Up/Down aim, keeper central shot, pad/keyboard dual roles,
  half-time C/PASS; fit 15 lines above the footer (`MENU_OPT_HELP_PITCH 13`).
- README/ENGINE/PROGRESS: acceptance, evidence errata, numbered legs.
- Windowed: centre Z → 0-0 save; Up+Z → 1-0; full time 1-0; no staged score.

## Task 5 — review fixes (code-review `--fix`)

- Aim axis corrected to Up/Down (was Left/Right).
- Keeper goal-side gate added (no save past the keeper).
- Stale-aim clearing; no-staging keeper-save test; match-level long-ball test.
- Cleanups: `match_run_live_team_for_side`, `match_run_live_aim_reset`,
  named masks/mouth constants, `live_aim_goal_x` rename, merged live guard,
  removed unused test out-param; keeper-position citation.
- Deferred as legs: keeper possession install (P9-L11), half-time auto/pulses
  (P9-L10), native aim/keeper.

## Acceptance

- `make check` 119/119; M1 `09b726b7…`/M2 v29 `7c3874f7…` exact; census
  unchanged.
- Shipped screenshots as listed in the spec; the deterministic save test is
  the save proof, not the 0-0 capture.
