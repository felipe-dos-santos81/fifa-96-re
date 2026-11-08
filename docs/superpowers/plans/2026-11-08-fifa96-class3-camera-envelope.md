# FIFA 96 — class-3 camera envelope (Phase 81)

Goal: close the gap between the Phase 80 derived side camera and the native
default Friendly view: the native sequence is a high, steep side view whose
play stays framed, while the Phase 80 view was a low 17-degree sideline view
with the stand in frame and a ball focus that could freeze behind a native pan.

## Evidence

- The native default Friendly sequence (`session-probe-14c18-mode`, FU-16) is
  640x400; players are ~25 px tall, the field fills the frame, the far stand is
  outside the view, and the ball stays near frame centre across the sequence.
- Ghidra `FUN_0004db38` (class 3) clamps the pitch into the `0x2000` (45°) band,
  uses the 500 (`0x1F4`) side distance constant, and clamps camera x/z to
  `±0x420`/`±0x810`. FU-100/FU-104 record the side block and the class-to-slot
  mapping.
- The Phase 80 seed used pitch `0x0C00` and a `(-1600,80,0)` offset. The live
  smoke showed the low stand view, and the ball moved off-centre because the
  presentation eye was derived from the FU-71 focus, which deliberately freezes
  while a native pan event is active.

## Steps

1. Replace the live seed with the class-3 envelope: `x=-500`, focus-relative
   `y=420` (eye height `0x1F4`), `z=0`, yaw `0xC000`, pitch `0x2000`, ratio
   `4608`.
   Check: `test_engine_match_render` pins the high side eye; `make check` and
   M1/M2 remain exact.
2. Separate the presentation focus from the native FU-71 focus. The live
   presentation focus follows the ball every granted frame; the native focus
   keeps its ball-machine/pan semantics. The render eye is clamped around the
   presentation focus.
   Check: `test_live_follow_keeps_ball_during_pan` and
   `test_live_render_eye_stabilizer`; the simulation/manual-switch tests stay
   unchanged.
3. Update the formation render check for the high framing (the far stand is
   outside the class-3 view), capture the release view and update
   ENGINE/PROGRESS/README.

## Result

The live view is now a high class-3 side envelope at 45° with the ball kept in
frame by a render-only focus. `make check` **124/124**; M1/M2 byte-exact. The
release screenshot is `docs/screens/p81-class3-envelope.png`; the native
reference remains `docs/screens/p80-native-side.png`.

## Non-goals

- The full `FUN_0004DB38` yaw/pitch integrator and the native camera director
  remain open legs.
- The native lens/ratio path and the `0xA2A10` addend remain open.
- Player animation cadence and shadows are handled separately.
