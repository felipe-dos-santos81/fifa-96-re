# FIFA 96 — live render-eye stabilizer (Phase 71)

Goal: remove the live camera lurch without touching the native FU-71 simulation
cell or the M1/M2 goldens.

## Evidence

- `match_run_live_camera_follow` returns early while a native pan is active and
  then hard-assigns `camera.pos_x/z = ball + lead` when the pan ends
  (`port/src/fifa96_engine/fifa96_match_run.c`, the function after the P8 shot
  gate). The rendered eye is `focus + camera-0 offset`; a large focus delta is
  a one-frame render-eye teleport.
- The native FU-71 bounds are already in the repo:
  `fifa96_camera_out_of_bounds` uses `|x| > 0x6C0` / `|z| > 0xAB0`.
- The derived far wall sits at `x = 0x7E0 + 64`; clamping the render focus to
  the native bounds keeps the eye off the wall while leaving the simulation
  cell untouched.

## Steps

1. Add render-only eye state to `fifa96_match_run_render` and a public
   `fifa96_match_run_render_eye`.
2. Update the eye at the top of `match_run_live_camera_follow`, before the pan
   early return: clamp the focus to the FU-71 bounds, add the camera-0 offset,
   snap only when invalid or the error exceeds `0x600`, otherwise step at most
   `0x30` per granted frame.
3. Use the stabilized eye in `fifa96_match_run_render`; keep
   `fifa96_match_run_eye` exact for the existing pins.
4. Add `test_live_render_eye_stabilizer`: bounded per-frame movement and the
   native clamp.
5. Run `make check` and the M1/M2 `cmp` gates.

## Result

Landed. The eye update is live-only, the simulation camera and goldens are
unchanged, and `test_live_render_eye_stabilizer` pins the 0x30 bound and the
native clamp. Gate: 124/124; M1/M2 exact.

## Non-goals

- Not the native director/lens (P10-L6) or the 3dwpts stadium (P10-L2).
- No simulation-camera writes; the ball machine, goal armer and native pan
  keep the exact FU-71 cell.
- No M1/M2 re-pin; the stabilizer is live-only.
