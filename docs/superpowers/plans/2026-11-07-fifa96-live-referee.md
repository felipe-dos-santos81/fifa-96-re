# FIFA 96 — live referee rules (Phase 70)

Goal: make the live match actually enforce fouls and offside, using the ported
native referee chains, without moving the M1/M2 goldens.

## Evidence

- The act-2 stage-3 hold is real for the immediate re-entry, but the native
  ordinary-match ball appearance pass releases it: `FUN_0008AF38` calls
  `FUN_0008BAF0` only when `[0x14C32A] == 0` (`0x8B65B/0x8B662`), its tail calls
  `FUN_0006FE84` (`0x8C05A`), and the row-0x49 settle animation installs
  successor row 0x00 (flags 0x07) through `FUN_0006E724` at `0x6FF67..0x6FF85`.
  That writes `[0x158882] = 1`, so the next act tick passes `0x894A2`.
- The live opponent tackle currently hands the ball to the fouler and never
  calls the native contact entry.
- The live offside path queries the own-team nearest to the **ball**, not the
  native reception target (`0x157770`), so a live pass cannot fire the check.

## Steps

1. Add the derived ball-settle release state and tick it before the act tick.
   - Start it from the act-2 stage-1 body for `contact_kind != 3` in the
     ordinary session; the native row-0x49 duration sum is `0x280`.
   - Clear it on reset/begin/terminal.
   - Keep the native stage split and the kind-3/session terms untouched.
2. Wire the live opponent contact to `fifa96_match_run_contact` before the
   possession swap, guarded by an idle act slot and a 45-frame cooldown.
3. Route the offside own-nearest query through the receive target computed by
   `fifa96_match_run_ball_receive`, keeping the public ball-based entry for the
   existing direct geometry tests.
4. Add headless live tests: foul -> free kick, the visible stage-2 hold and
   release, the no-foul settings gate, and the receive-target offside check.
5. Update the port census: phase 0x06, action 0x13 and action 0x1F become
   naturally reachable from ordinary fouls, so the blocked set drops 6 -> 3 and
   the natural wired count becomes 61/80. Update README/ENGINE and the census
   test.
6. Run `make check`, M1/M2 byte comparisons, and a release smoke.

## Result

All six steps landed. The settle release uses the native row-0x49 duration sum
`0x280`; the live contact registers through `fifa96_match_run_contact`; the
offside query uses the receive target. Tests: `test_live_ai_tackle_foul_reaches_free_kick`,
`test_live_foul_reaches_penalty`, the updated
`test_foul_taker_held_on_live_default`, and the strengthened
`test_offside_reachable_from_live_receive`. Census: 61/80 natural, 64/80
install-live, 3 blocked. Gate: 124/124; M1/M2 exact.

## Non-goals

- No fake whistle/card; the native foul chain decides.
- No M1/M2 re-pin. The tapes never set `live_start` and never reach the foul
  chain.
