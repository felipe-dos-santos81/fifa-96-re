# FU-351 — native class-3 side camera (FUN_0004DB38)

Date: 2026-11-09. Scope: the live class-3 side camera (`FUN_0004DB38`) and
its `0x4C7D0/77C/7F8/49388` helpers. The live target producer and the
camera-type/view-mode setup remain open legs.

## 1. Native contract

### 1.1 Dispatch

`FUN_000505D0` selects the class from camera record `+0x04`; class 3 maps to
camera slot 5 and handler table `0x108B80[3]` = `FUN_0004DB38` (FU-100 §1/§2).
`FUN_0004D134(4)` also uses the same handler for the replay/action camera.
`FUN_0006400C` returns `[0x109A98] == 0` (not replay).

### 1.2 `FUN_0004DB38` (fresh `disassemble_function`)

Inputs: camera record `EAX`, speed `ECX` (the class-3 table passes `0x1F4` =
500 from `0x108B7C`). The behavior block in `EDX` is not read.

```
if (FUN_0006400C() == 0) speed += 100;      // replay is 100 farther
target[3] = camera[+0x34/38/3C];            // staged triple
yaw   = -atan2_16(target.x - cam.x, target.z - cam.z)
pitch = -atan2_16(target.y - cam.y,
                  |dx| + |dz| - min(|dx|,|dz|)/2)
pitch clamp:
  (0x8000, 0xF618) -> 0xF618
  (0x2000, 0x8000) -> 0x2000
offset = X(pitch) * Y(-yaw) * {0,0,speed}
cam.x = target.x - offset.x
cam.z = target.z - offset.z
new_y = target.y - offset.y
if (new_y < cam.y) cam.y += (new_y - cam.y) >> 3;
else               cam.y = new_y;
clamp cam.x +-0x420, cam.z +-0x810, cam.y >= (live ? 100 : 120)
yaw   += (target_yaw - yaw) * tick / 0x1E0000
pitch += (target_pitch - pitch) * tick / 0x3C0000
pitch clamp (same band)
cam[+0x5A] = cam[+0x5E] = 0;                // high words of yaw/pitch
camera[+0x40/44/48] = target[3];            // target history
```

The helpers:

- `FUN_0004C7D0`/`FUN_0004C77C` are `-FUN_000A2A10(...)` with the
  `0xFFFF` mask. `FUN_000A2A10` is the 16-bit atan: `+z = 0`, `+x = 0x4000`,
  `-z = 0x8000`, `-x = 0xC000`; 45 deg = `0x2000`. `FUN_0004C77C` uses the
  native octagonal range `|dx|+|dz|-min/2` before the pitch atan.
- `FUN_0004C7F8` builds `Y(-yaw)` at `0x4C414`, `X(pitch)` at `0x4C3B0`,
  multiplies them with `0xA2991`, and applies the product to `{0,0,speed}`
  with `0xA2914`.
- `FUN_00049388` is `return [0x107300]` — the 100 Hz elapsed time x12/5
  (FU-60 §2.3/FU123). The port's 30 Hz frame delta is 4x smaller.
- `0x108B98/0x108B9C` = 100/120 (live/replay Y floor).
- `0x108B7C` = `0x1F4` (500).

The live fixed point for a level target is
`target (0,80,0), camera (-353,433,0), yaw 0xC000, pitch 0x2000`. The camera
is exactly 500 units from the target (353 horizontal, 353 vertical).

## 2. Evidence

Fresh `disassemble_function 0x4DB38` (186 instructions), `0x4C7D0/77C/7F8`,
`0x4C414/3B0`, `0xA2991/2914`, `0x49388`, `0x6400C`; first-hand
`read_memory 0x108B7C/98/9C`, `0x108B80`, `0x108B64`, `0x107B98`.
`FU-100` §1/§2 maps class 3 to slot 5 and the handler table; `FU-152` §2.8
and `FU-348` §5 record the same body. The port's `fifa96_entity_angle`
(0x400-circle, `0x100` = 90 deg) and `fifa96_entity_sine` (257-entry
quarter-sine, `0x100` = 90 deg) supply the atan/sin/cos primitives with the
native `*0x40` / `>>6` scaling.

## 3. Port implementation

- `fifa96_camera_behavior_action(st, live, speed, tick, out)` in
  `port/src/fifa96_loader/fifa96_camera.c` now implements the full body:
  look-at yaw/pitch, pitch snap, `X*Y` direction offset, position clamps,
  Y ease, live/replay speed and Y floor, yaw/pitch slew, brake clear.
- `fifa96_camera_pose_args` gained `handler_tick`; the pose-feed action arm
  passes it through.
- The live path in `match_run_live_camera_follow` (`fifa96_match_run.c`) calls
  the handler with speed 500 and `state.frame_delta * 4`. The target is the
  existing Phase-85 derived render focus; the phase-71 derived envelope is
  now only the first-frame seed. Non-live runs never enter the handler.
- The projection consumes the handler's stored `yaw`/`pitch` and the eye
  position; the FU-71 simulation camera remains separate.

## 4. Verification

- `make check`: **124/124**.
- M1 `cmp` byte-exact `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`.
- M2 `cmp` byte-exact `41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`.
- `test_camera` pins the fixed point, replay distance 600 / Y floor 120, the
  `+0x420` position clamp and the `0xF618` pitch snap.
- `test_engine_live_control` pins the live path: the eye is clamped by the
  native `+-0x420` box when the focus is pushed to `0x1000` (the old derived
  envelope could only reach `0x3C0`).
- Windowed Friendly (Brazil vs Germany) at `DISPLAY=:1`:
  `docs/screens/p90-native-class3-kickoff.png` and
  `docs/screens/p90-native-class3-follow.png`. The player is visibly larger
  than the old derived envelope and the side chase follows the moving player.

## 5. Legs

- **FU351-L1** native live target producer (`FUN_000505D0`/FU-71 pose and
  pan events). The live target is still the Phase-85 derived render focus.
- **FU351-L2** camera-type/view-mode selection (`FUN_0004D7E8`, the
  `0x14E57C` view mode and the options camera setting). The live class-3
  path is selected by the port, not by the native menu state.
- **FU351-L3** exact `FUN_000A1A60` sine/cos 6-bit interpolation. The port
  uses the 257-entry table at `>>6` resolution; the fixed point and clamps are
  exact, the angle is within the table step.
- **FU351-L4** the class-3 tail target-history copy
  (`camera+0x40/44/48 = +0x34/38/3C`); no port consumer yet.
- **FU351-L5** native pan/pose interaction with the class-3 target; the
  current live follow keeps the derived freeze/return behavior.

**Errata (quoted).** Phase 86 legs: "the native lineup matrix `0x14C360` is
unported" was corrected by FU-350; the class-3 camera claim in the same
paragraph ("the class-3 camera integrator remains open") is corrected here:
the `FUN_0004DB38` handler is ported. FU-348-L4 ("`FUN_0004DB38` and its
helpers are mapped but not ported; the live eye is the derived envelope") is
closed for the handler and live eye; the target producer and camera-type
selection remain FU351-L1/L2.

## 6. FU351-L3 closed — exact 0xA1A60 sincos (2026-11-09)

**Erratum (quoted).** §5 FU351-L3: "exact `FUN_000A1A60` sine/cos 6-bit
interpolation. The port uses the 257-entry table at `>>6` resolution..." —
closed. `fifa96_camera_behavior_action` now calls
`fifa96_projection_sincos` (the FU-88 port of the `0xA1A60`/`0xCE3B0`
257-entry table plus the 6-bit correction), not the entity sine table at
`>>6`. `fifa96_camera` links `fifa96_projection` for the primitive. The
camera geometry, tick and clamps are unchanged; `test_camera` and the live
camera test still pass. `make check` **124/124**; M1/M2 `cmp` exact.
