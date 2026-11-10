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

## 7. Handler correction — Q16 slew and the two missing arms (2026-11-09)

**Erratum (quoted).** §1.2 line 37 ("`yaw += (target_yaw - yaw) * tick /
0x1E0000`") and §3 ("now implements the full body") were incomplete. A fresh
186-instruction `disassemble_function 0x4DB38` gives:

- `0x4DCCA..0x4DCF6` / `0x4DD2C..0x4DD58`: the slew rate is Q16
  `floor(tick * 0x10000 / 30)` (yaw) or `/ 60` (pitch), built with
  `SHL EAX,0x10` plus two `IDIV` remainder steps. The previous port divided
  by `0x1E0000` before the Q16 multiply, so tick 8 produced zero and the
  stored yaw/pitch never moved. The `SHL` is 32-bit: tick `0x3C0000` wraps
  it to zero.
- `0x4DBCA..0x4DBF3`: when the *old* eye is at/outside any clamp bound
  (`|x| >= 0x420`, `|z| >= 0x810`), the native shifts `off_y >> 3` before
  the position update.
- `0x4DC82..0x4DC96`: the native recomputes the look-at from the clamped
  eye before the slew; that second pitch is raw (not snapped). The
  post-slew pitch snap still runs.
- `0x4DD95..0x4DD9D`: the target-history copy
  `camera+0x40/44/48 = +0x34/38/3C`. Ported as `hist_x/y/z`; no consumer
  reads it yet.
- `0x4DCF9`/`0x4DD5B` clear the high words of yaw/pitch. The port stores
  the 16-bit values, so those clears are implicit; the separate `brake_*`
  fields were removed.

The true fixed point is `camera (-353,434,0)` at yaw `0xC000`, pitch
`0x2000` (the exact 45-degree offset truncates to -354); replay is
`(-424,505,0)`. §5 FU351-L4's copy is ported; only a downstream consumer
remains absent.

**Verification.** `test_camera` now pins the Q16 `+0x1111`/`-0x1111`
moves, the 32-bit tick wrap, the old-eye `345`/`306` Y split, the replay
`505`/`120` values, the fixed point, the target history and the post-slew
`0xF618` snap. `test_engine_match_render` pins its identity view
(yaw/pitch 0) after the live drive so the ball-art fixture is independent
of the live camera. `make check` **124/124**; M1/M2 `cmp` exact.

## 8. FU351-L1 live focus landed; the action target table is blocked (2026-11-09)

**Erratum (quoted).** §5 FU351-L1: "native live target producer
(`FUN_000505D0`/FU-71 pose and pan events). The live target is still the
Phase-85 derived render focus." The live target is now the native focus
triple; the `0x109A74` table path is separately producer-blocked.

**The live focus is `0x155AA4`.** First-hand `read_memory 0x109A70` (8 B) =
`a4 5a 15 00 b0 5a 15 00`: `[0x109A70] = 0x155AA4` (the camera-relative
basis staged by the composer) and `[0x109A74] = 0x155AB0` (23-slot table,
12-byte stride). Fresh `disassemble_bytes 0x36C70..0x36D05` gives the focus
head:

- `0x36C76..0x36C93`: source = `[0x157A6F]` (a **camera-triple override**
  seeded from `0x15774C`; FU-154 §2.1 corrects FU-112's "ball pointer"
  naming) when it is nonzero **and** `FUN_00063FD0()` is zero, else
  `0x15774C` (the FU-71 triple). `FUN_00063FD0` is
  `return [0x109A98] & 0x80` (fresh decompile).
- `0x36C9F..0x36CA1`: three `MOVSD` into `0x155AA4/8/C`.
- `0x36CA2..0x36CCC`: x/z clamp. x: `> 0x1770` or `< -0x1770` -> fallback;
  z: `> 0x1770` or `< -0x1770` -> fallback. Fallback `0x36CCE..0x36CE0`:
  x = `-4000` (`0xFFFFF060`), y = 0, z = `-4000`.
- `0x36CE6..0x36CFA`: y `< 0` or `> 0x3E80` (16000) -> 0.
- The 23 player/ball entries continue at `0x36D1A` (team A), the second team
  and the slot-22 ball write at `0x36F83`; the focus is the table header, not
  an entry.

**The live class-3 target is this focus.** Fresh
`disassemble_bytes 0x4D04C..0x4D10F` (`FUN_0004D04C`, the view selector):
`0x4D0A3 MOV ESI,[0x109A70]`, `0x4D0AC..0x4D0AE` three `MOVSD` into
`camera+0x34/38/3C` (the class-3 target), `0x4D0AF..0x4D0C0` stages the same
triple on the stack and copies it into `camera+0x40/44/48` (the history),
then `0x4D0C1` re-parameterises the descriptor and `0x4D0C6..0x4D0DB` runs
`FUN_0004D908` 20 times to settle the view. So on a view change the
class-3 target and history both take the native focus.

**The `0x109A74` table path.** `FUN_00036C70` fills `0x155AB0 + 0xC*i` from
the 22 player records and the ball; `FUN_0004BAD8` (`0x4BAD8`) returns the
slot of `[0x10F370]` through `FUN_0004BE44` (`0x4B99F..0x4B9FF`): it reads
`byte[rec+0x8D]` (`[rec+0x8A] >> 24`) and adds `0xB` for one side. The side
addend is not a plain team bit: with `[0x157AC2] & 1` set and
`FUN_0004B678()` (`[0x157AC2] >= 4`) clear, the `0xB` is added when the
record’s `+0x826` side byte is 0; otherwise it is added when the side byte is
1 (fresh `disassemble_bytes 0x4B678..0x4B68A`, `0x4B99F..0x4B9FF`).
`FUN_000505D0` state `0xB` (`0x50F6B`) then copies
`0x155AB0[FUN_0004BAD8()]` into `camera+0x34/38/3C`.

This path is **not live**: the only writer of `[0x10F370]` is row `0x17`
(`0x84780`/`0x8494C`), whose only installer is act-4 stage 0 (`0x89740`);
FU-201 proves the act-4 formation-order arm is not live, and the row-0x17
port entry is explicitly NOT counted for that reason
(`fifa96_match_handlers.c`, action `0x17`). The table path therefore stays
unwired; it is the replay/action-camera producer, not the open-play target.

**L4 consumer.** The class-3 history copy is read by class 0:
`FUN_0004E834 0x4E972 MOV EBX,[ESI+0x40]` (history x) is passed with
`EDX=[ESI+0x34]` (target x) and `EAX=[ESI+0x10]` (eye x) to
`FUN_0004D698` (`0x4E96E..0x4E978`). The copy therefore seeds the class-0
integrator when a view switches from class 3 to class 0. The `FUN_0004D698`
body remains the FU-100 §3 leg 9; only the consumer is now identified.

**Port landed.**
- `fifa96_camera_focus_clamp` (`fifa96_camera.c/.h`) implements the
  `0x36C70` head exactly (source selection stays in the engine; the clamp is
  the native body).
- `match_run_live_camera_follow` now stages the native focus from the current
  FU-71 place (`cam->pos_*`, the pan-updated `0x15774C` mapping) through the
  clamp; `render_focus_{x,y,z}` carry it and `action_cam.track_*` uses it.
  This matches the native order (pan update -> composer stages `0x155AA4` ->
  view driver). The Phase-85 controlled-record blend is retired; the
  `view_focus_*` ball follow (P6 lead) is still written for the simulation
  camera. The simulation cells and the pan integrator are unchanged.
  **[D]** The port has no `[0x157A6F]` override cell; the FU-71 place is the
  only source, which is the native default branch and the only non-clear
  writer's seed (FU-154 §2.1). The native view selector writes the target on
  a view change; the port refreshes it per frame so the class-3 handler
  follows the place. The native preset/event cadence is L2.
- `fifa96_camera_target_slot` implements `FUN_0004BE44`'s record->slot
  resolution exactly (the `[0x157AC2] & 1`, `>= 4` and `+0x826` side matrix)
  and `test_camera` pins it. It is deliberately **not wired**: its producers
  are the row-0x17 `[0x10F370]` and the selection `[0x157A83]` paths, and the
  former's installer is non-live (FU-201).
- **L5** is closed for the target interaction: the class-3 target/eye update
  runs before the native-pan freeze gate, so an active FU-71 pan no longer
  selects a different class-3 target; it only gates the simulation focus.
- **L2** remains open: the live class is still forced to 3. The native
  preset/view selection (`FUN_0004D04C`, `FUN_0004D7E8`, `[0x14E57C]`) is
  unported. Entry-point evidence for the next slice: `FUN_0004D04C`'s only
  caller is `FUN_0004CE34` (`0x4CE62`), which passes the preset from
  `FUN_0004B308`: phase `[0x157A4A] >> 24` 3 -> 1 when
  `[0x14C2F6] == 1` else 2; 4 -> 3/4; 0x15 -> 5/6/8; else 0.
  `FUN_0004D04C` clamps to 0..6 and selects slot `{1,1,0,0,2,2,5}`; slot 5 is
  class 3, slots 0/1/2 are classes 0/1/2. The live class is therefore a
  function of the phase/preset and the options camera cell, not a constant.

**Verification.** `make check` **124/124**; M1 `09b726b7…` and M2
`41521559…` byte-exact. `test_camera::test_focus_clamp` pins the ±6000 /
0..16000 bands and the `-4000` fallback; `test_engine_live_control` pins
the class-3 target to the native focus (and away from a distinct controlled
record) plus the eye clamps.
