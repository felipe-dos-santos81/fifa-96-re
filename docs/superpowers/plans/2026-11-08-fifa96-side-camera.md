# FIFA 96 — side-camera gameplay view (Phase 80)

Goal: replace the live type-0 end/behind-play view with the native-style
sideline view used by the default Friendly match, then verify the camera
follows play without moving the M1/M2 tapes.

## Evidence

- The default native Friendly path (FU-16, `probe-14c18-mode`; a second manual
  DOSBox Friendly run) starts with the close **class-3 side camera**, not the
  type-0 end preset. FU-100 maps class 3 to camera slot 5; FU-104 records the
  side block `{-755,120,295,45425,0,4608}`; the class-3 handler is
  `FUN_0004DB38` (OL-T11-76, still unported).
- The port's live begin installed the type-0 preset
  `{-100,488,1100,33900,3900,4608}` and the side block was never consumed, so
  the live camera looked along the pitch from an end position with tiny
  sprites and a large exposed stadium band.
- The native capture `session-probe-14c18-mode` frame `fifa96_005-0053.png`
  is the reference; it is a 640x400 frame from a Friendly match started
  through the standard key route.

## Steps

1. Add a live-only side seed (`x=-1600`, focus-relative `y=80` giving eye
   `0xA0`, `z=0`, yaw `0xC000`, pitch `0x0C00`, ratio `4608`) and publish it
   from the live begin/eye path.
   Check: the live-eye tests assert the side seed and the M1/M2 tapes stay
   exact because the tape path never sets `live_start`.
2. Update the live render expectations that assumed the camera-0 vertical
   halfway line: the side view still draws the z=0 line, but its screen
   direction is camera-dependent.
   Check: `test_engine_match_render` and `test_engine_live_control` pass.
3. Capture the windowed side view, record the native comparison, update
   ENGINE/PROGRESS/README, run `make check` and the release smoke, then commit.
   Check: 124/124, M1/M2 exact, release screenshot shows the side view.

## Result

The live view is now a side broadcast stand-in near the left touchline,
following the FU-71 focus; the native class-3 position/pitch integrator stays
a leg. `make check` **124/124**, M1 byte-exact, M2 byte-exact; the release
screenshot is `docs/screens/p80-side-camera.png` and the native reference is
`docs/screens/p80-native-side.png`.

## Non-goals

- No full class-3 `FUN_0004DB38` position/yaw/pitch integrator port.
- No native 3dwpts stadium bowl, goal-net renderer, or camera director.
- No M1/M2 re-pin; all changes are `live_start`-gated.
