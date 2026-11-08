# FIFA 96 — live attacking support run (Phase 75)

Goal: make one possessing-team attacker leave the off-ball block and run
forward, so through balls have a live reception point and the CPU pass
destination moves, without pretending to port `FUN_0007F7E0` and without
touching the M1/M2 tapes.

## Evidence

- `FUN_0007F7E0` is the native teammate-selection/support-run machine: 2 514
  bytes, sole caller `0x7F36F`, own jump table (`FU251_ol63_row05_tail.md`).
  It remains FU251-L8; this slice is derived `[D]`.
- The Phase 72 off-ball shape (`match_run_live_offball_shape`) moves the code-3
  block but never a player beyond it.
- Human PASS is kicker-relative at launch; the flight lands at the derived
  target and the live claim is positional (`0x400` radius). A runner arriving
  near the landing point makes a through ball reachable.
- CPU PASS targets the most advanced record by attacking direction, so moving
  that record moves the pass destination. The code-3 runner cannot claim a
  loose ball, so CPU pass completion is not claimed.
- Formation roles come from the native `0x11033A` table (role 3 forward, role 2
  midfield).

## Steps

1. Add `live_support_runner` / `live_support_timer` and a reset helper.
2. Pick one eligible most-advanced role-3, then role-2, then fallback
   mid/forward; reject keeper, hidden/occupied, carrier, team
   target/second/intercept/chosen and controlled.
3. Apply a forward target after the off-ball shape and before the entity
   chain: `max(runner_front, carrier_front + 0x600)`, clamped to `0xB00` depth
   and the runner's own x channel `±0x760`.
4. Hold the runner for 120 granted frames before re-evaluating; clear on
   possession loss, restart, out-of-play, non-play phase, init and begin.
5. Tests: role pick/target/forward motion; hysteresis, re-pick, second-pick
   exclusion and possession-loss clear.
6. Run `make check`, M1/M2 `cmp`, and keep the census unchanged.

## Result

All six steps landed. The runner is live-only, writes only its target, and
uses no RNG. The windowed smoke also exposed the derived P12 goal-net plane
painting a foreground mesh when the camera passed a goal; a `0x400` world-unit
near clip was added as a presentation guard. `make check` 124/124; M1/M2 exact
by `cmp`; census 78/78 bodies, 61/80 natural, 64/80 install-live; screenshot
`docs/screens/p75-support-run.png`.

## Non-goals

- No `FUN_0007F7E0` port, no second wide runner, no native row-05 claim
  producer.
- No CPU pass-completion claim.
- No M1/M2 re-pin.
- The `0x600` depth and 120-frame hold are derived tuning constants.
- The goal-net `0x400` near clip is a derived presentation guard, not the
  native 3dwpts net renderer (P10-L2).
