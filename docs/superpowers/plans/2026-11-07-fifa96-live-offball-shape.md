# FIFA 96 — live off-ball team shape (Phase 72)

Goal: make the live match move the whole outfield shape, not only the team
target and ball carrier, without wiring row `0x03` into the shared table (which
moves the M2 tape).

## Evidence

- The forced outfield action gives every non-target record code 3
  (`fifa96_outfield_forced_action`); row `0x03` is the off-ball target producer
  but its dispatch slot is `NULL`, so the code-3 majority keeps its kickoff
  target and stands still.
- The native row-03 path is the `0x6D870` pitch-cell map plus the `0x6DCC8`
  direction-pair scaler over the bound `pl.fmt` metadata (`rec->phase_meta`).
  `fifa96_match_phase_04` already ports both pieces; the M2 tape moved when the
  shared row-03 slot was wired, so the live path must consume the same metadata
  directly.

## Steps

1. Export the pure `0x6D870` cell map (`fifa96_match_pitch_cell`) and the
   `0x6DCC8` applier (`fifa96_match_phase_apply_cell`).
2. Add `match_run_live_offball_shape`: map the camera focus, then write the
   per-record target for every active idle code-3 outfield record that is not
   the ball carrier, team target or second pick. Live-only, phase 2 only.
3. Call it before the entity chain so the same frame's mover consumes the
   targets.
4. Add `test_live_offball_shape_moves_team` with synthetic metadata; pin that
   at least 16 of the 20 outfield records move, with no score/phase write.
5. Run `make check` and the M1/M2 `cmp` gates.

## Result

Landed live-only. The `0x6D870`/`0x6DCC8` helpers are exported, the cell shape
runs before the entity chain, and the test moves 17/20 outfield records with
the carrier/target/second exempt. Gate: 124/124; M1/M2 exact.

## Non-goals

- Row-03 dispatch-table wiring (M2 tape risk; this is a live-only consumer).
- The native `FUN_0007F7E0` attacking support-run machine (separate slice).
- Any change to the shared table or census.
