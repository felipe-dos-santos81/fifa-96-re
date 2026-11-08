# FIFA 96 — live walk rows and ground shadows (Phase 82)

Goal: fix the remaining “players slide while standing still” look and make the
players sit on the pitch. The native view uses speed-selected walk rows and a
long dark ground shadow; the port kept the idle row and an almost invisible
90%-brightness ellipse.

## Evidence

- FU-172 §4/§5: terminal rows re-select the movement tables
  `0x11052A` (active) / `0x1105AA` (inactive) at every frame-timer expiry.
  Active speed 1–2 -> row `0x01`, 3–5 -> `0x02`, 6+ -> `0x03`; speed 0 ->
  row `0x00`. The terminal duration is the table duration, not the frame
  record duration.
- Live player trace before the fix: the controlled record stayed on
  `anim=00 frame=0` while its `speed71` reached 6. After the fix it cycles
  `anim=01/02/03` with frames advancing at the speed cadence.
- Native capture frames show long dark shadows offset down-left; the port’s
  `pitch_shade` pass maps the grass to a 90%-brightness near-identical colour,
  so the ellipse is effectively invisible.

## Steps

1. Port the FU-172 terminal steering tables and the live scene-stage
   selection/duration. Keep the non-live wrapper’s terminal no-op so the
   M1/M2 tape remains byte-identical; the live scene stage calls the new
   `_turn` entry.
2. Add a live-only 55%-brightness shadow palette map, a longer/offset shadow
   ellipse, and draw it under every live sprite.
3. Add unit coverage for the steering rows, capture the live view, update
   ENGINE/PROGRESS/README, run `make check` and commit.

## Result

The live controlled record now switches through rows `0x01/0x02/0x03` by
speed and the players carry a visible ground shadow. M2 remains exact because
the non-live path keeps the wrapper no-op and the scene-stage steering/duration
is gated by `live_start`. The native `+0x7F` lean producer and the full
appearance machine remain FU172-L2 legs.

## Non-goals

- Native `+0x7F`/`anim_turn` producer (FU172-L2) and the full appearance
  animation machine.
- Native 3dwpts stadium/goal-net art (P10-L2).
- The class-3 position/yaw/pitch integrator and camera director.
