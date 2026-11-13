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

## 9. FU351-L2 class-0 integrator primitives (2026-11-09)

**Landed.** The two pure bodies the class-0 steady handler (`FUN_0004E834`)
calls for the camera x/z cells are ported and tested:

- `fifa96_camera_ramp_factor` = `FUN_0004D668` (`0x4D668..0x4D695`): the
  native rate*15 left shift, the two IDIV steps and the quotient/remainder
  reconstruction reduce to `floor(tick * 0x10000 / (rate * 15))` on the live
  positive domain. `tick` is `FUN_00049388`; `rate` is behavior `+0x28`
  (`param_2[10]`).
- `fifa96_camera_slide` = `FUN_0004D698` (`0x4D698..0x4D7E5`): clamp
  `target - hist_offset` to `[lo, hi]`, step by the Q16 factor, then the
  native mode gates. Mode -1 rejects a positive step when `target - hist <
  -0xA0`; mode 1 rejects a negative step when `0xA0 < target - hist`; mode 2
  has no gate because `[0x108B94] = 1` (fresh `read_memory 0x108B90` =
  `A0 00 00 00 01 00 00 00`; both cells have no writer). The outside
  `(lo, hi)` branch uses the fixed rate 8 and does not clamp high.

**Call-site mapping (fresh `decompile_function 0x4E834`).** The handler
calls the slide twice:

- z: `slide(cam+0x18, [cam+0x3C], [cam+0x48], mode, -C[2], C[3],
  ±block[+0x18], block[+0x28], tick)`, where mode is -1 when behavior class
  4, else 1, and the C bounds swap in that branch;
- x: `slide(cam+0x10, [cam+0x34], [cam+0x40], 2, -C[0], C[1], sign-adjusted
  target, block[+0x28], tick)`.

`C` is the behavior block's `+0x50` table record (`0x85AC`/`0x85EC`/`0x862C`
plus variant*0x20; the first records read `{150,150,1000,3800}`,
`{900,1900,900,3750}`, `{600,2400,1500,1500}`).

**Wired.** `fifa96_camera_handler_block` and the six-row behavior table now
carry `+0x10`, `+0x18`, `+0x28` and the `+0x50` C record
(`c0..c3`); `fifa96_camera_behavior_steady` runs the z slide then the x slide
(the sign-adjusted target offset), with the pose feed's `handler_tick` as the
native `FUN_00049388` tick. The slides consume `hist_x`/`hist_z`, so the
class-3 x/z target history has a wired consumer (L4 closed for x/z;
`hist_y` has no reader in the ported class-0 body). The
handler's yaw arm is now `FUN_0004DF34` (§11); the pitch arm
(`FUN_0004E05C`), the class-3 snap and the pan/roll block remain. The live
class is still 3, so this has no open-play effect yet; the next L2c slice
ports the pitch arm and then selects the live class from the native preset
(`FUN_0004CE34` / `FUN_0004B308`).

**Verification.** `test_camera::test_camera_slide` pins the ramp factors and
the inside/outside, mode-gate, low/high-clamp and history-offset branches;
`test_camera::test_behavior_steady_slides` pins both handler calls (x 10,
z 67 at tick 8 for block 0) and the z mode-1 history gate; the behavior-table
test pins the six new rows. `make check` **124/124**; M1/M2 `cmp` exact.

## 10. FU351-L2 native preset/class resolver (2026-11-09)

**Landed.** `fifa96_camera_view_select` ports the selection path without
wiring it:

- `FUN_0004B308` (fresh decompile): phase `[0x157A4A] >> 24` 3 -> preset 1
  when `[0x14C2F6] == 1`, else 2; 4 -> 3/4; 0x15 -> 5 when `[0x15B680] == 3`,
  6 when it is 0, else 8; every other phase -> 0.
- `FUN_0004D04C` (`0x4D04C..0x4D10F`): clamps the preset to 0..6 and selects
  slot `{1,1,0,0,2,2,5}` and variant `{0,1,0,1,0,1,1}`.
- The selected camera record's `+0x04` class is slot 0 -> 0, 1 -> 1, 2 -> 2,
  5 -> 3 (slots 3/4 are disabled). Preset 8 clamps to 0, i.e. slot 1/class 1.

The helper returns `{preset, slot, variant, class_of}`. It is **not wired**:
the live camera is still forced to class 3, and the class 0/1/2 pitch arm is
not ported. It is the resolver the next slice will call from the camera pose
staging.

**Verification.** `test_camera::test_camera_view_select` pins the phase-3/4,
0x15/b680 and default rows, the preset-8 clamp and NULL handling.
`make check` **124/124**; M1/M2 `cmp` exact.

## 11. FU351-L2 yaw arm `FUN_0004DF34` (2026-11-09)

**Landed.** The class-0 steady yaw cell is the native step, not the placeholder
clamp.

Fresh disassembly:

- `FUN_0004C7D0` (`0x4C7D0..0x4C7F5`, 19 instructions) reads
  `dz = point[+0x8] - origin[+0x8]`, `dx = point[0] - origin[0]`, pushes
  `(dz, dx)` into `FUN_000A2A10`, then returns `-angle & 0xFFFF`. The port's
  `camera_atan16(dx, dz)` is the same 16-bit angle, so the wrapper is
  `-camera_atan16(dx, dz)`.
- `FUN_0004D5F4` (`0x4D5F4..0x4D667`, 54 instructions) is the ±ratio
  deadband blend: `d = wrap16(atan2 - atan1)`; if `d > 0` return
  `atan1 + ratio/2` when `d > ratio`, else `atan1 + d/2`; if `d <= 0` return
  `atan1 - ratio/2` when `|d| > ratio`, else `atan1 - |d|/2`. `param4 == 1`
  selects the `atan2`-side half-step (the pitch caller).
- `FUN_0004DF34` (`0x4DF34..0x4E059`, 110 instructions):
  `atan1` is the target look-at (`camera+0x34` from `camera+0x10`), `atan2`
  the look-at of the stack point `{x=0, z=class==4 ? +0xB10 : -0xB10}`. It
  blends them with `camera+0x4C`, subtracts `camera+0x58` as a 16-bit wrap,
  and:
  - `|delta| < [0x108BAC]=30` -> return `delta`;
  - else `factor = FUN_0004D668(block+0x24)` =
    `floor(tick*0x10000/(rate*15))`, `step = round(delta*factor/0x10000)`;
  - negative `step` in `(-30,0)` -> `-30`; positive in `(0,30)` -> `+30`
    (zero stays zero);
  - `band = |wrap16(atan1 - yaw)| - ratio/2 - ratio/4`; when `band > 0`,
    raise a negative/positive step to `-band`/`+band` when it is closer to
    zero.
- The steady call (`0x4E97D..0x4E99F`) passes `EAX=camera`,
  `EDX=&stack-point`, `EBX=block+0x24`, `ECX=camera+0x4C`, then adds the
  returned step to `camera+0x58` and masks. The stack point's z is the same
  class-based `±0xB10` cell set at `0x4E8D1`/`0x4E8FB`.

Port: `fifa96_camera_handler_block` and the six-row table gained `+0x24`
(all six native rows are 8); `camera_steady_yaw_delta` implements the two
look-ats, the blend, the ramp, the ±30 minimum and the catch-up band; the
steady handler adds it to `st->yaw`. The placeholder `yaw_lo`/`yaw_hi`
clamp (and those two struct fields) was removed — the native steady yaw is
not a clamp.

**Not ported in this arm:** the pitch arm `FUN_0004E05C`, the class-3 yaw
snap (`0x4E9A2..0x4EA9B`), the pan/roll block (`FUN_0004E248`,
`0x4E84A..0x4E87F`), the mutable `+0x18` cell and its sub-record clamp
(`0x4E8A4..0x4E8C8`), the y-ease/slope block (`0x4EAB0..0x4EC5A`) and the
ratio easing (`0x4EC5D..0x4EC87`). The yaw cell is now native, but the
steady body is still partial.

**Verification.** `test_camera::test_behavior_steady` pins the +x/-z
scenario (`0xC2EE`), the +z tick-1 minimum (`30`) and the ratio-0 early
return (`0`); the table test pins `+0x24 = 8`. `make check` **124/124**;
M1/M2 `cmp` exact.

## 12. FU351-L2c steady body completion (2026-11-09)

**Erratum (quoted).** §9: “z: `slide(cam+0x18, [cam+0x3C], [cam+0x48], mode,
-C[2], C[3], ±block[+0x18], ...)`” is corrected by a fresh
`disassemble_function 0x4E834`; the same §9 line “the outside `(lo, hi)`
branch uses the fixed rate 8” is correct.

- `0x4E8D1..0x4E8F9` (class 4): mode `-1`, `lo=-C[3]`, `hi=C[2]`,
  `hist_offset=+block[0x18]`.
- `0x4E8FB..0x4E925` (else): mode `1`, `lo=-C[2]`, `hi=C[3]`,
  `hist_offset=-block[0x18]`.
- `FUN_0004D698 0x4D6D4` loads rate 8 on the **outside** branch;
  `0x4D705` loads the block rate on the strict **inside** branch.
  `test_camera_slide` pins both.

**Landed.** `fifa96_camera_behavior_steady` is now the full `0x4E834` body:

- **Pan/roll `FUN_0004E248`** (`0x4E248..0x4E3A5`, 136 instructions):
  `fifa96_camera_steady_pan_step` implements the four Q16 factors
  `(tick * n * 0x1111 + 0x8000) >> 16` for n=20/32/32/block+0x40 and the
  four native branches (zero first input, zero second input, opposite signs
  with the `|camera+0x3c| <= 0x587` gate, same signs). The zero-first-input
  arm returns `-pan` inside the `20*0x1111` step and `-f20` when positive
  (the native decays the cell toward zero; only a negative cell returns
  `+f20`). The steady handler calls it with the view driver’s proven `(0,0)`
  arguments, adds the result to block+0x34, clamps to `±block+0x3C` and to
  `[-(0xB10+camera+0x3C), 0xB10-camera+0x3C]`, then adds the same step to
  block+0x18 and clamps to the `+0x50` record’s `+0x18/+0x1C` cells.
  `FUN_0004D908 0x4D932..0x4D948` is the zero-argument caller.
- **Pitch `FUN_0004E05C`** (`0x4E05C..0x4E222`, 153 instructions):
  `camera_steady_pitch_delta` runs the two `FUN_0004C77C` looks around the
  native `camera+0x38 = 0` write, the `ratio/8` midpoint, the
  `FUN_0004D5F4` `param4=1` half-step at `ratio*7/4`, the
  `(ratio-param4)*(atan1-angle3)/ratio` correction, the low-altitude
  `(0,0x140,±0xB10)` look (floor `[0x108BB4]=0x140`; the `[0x107DD4]==1`
  arm uses `(0,camera_y<<5,±0xB10)` and floor `0x2EE`), and the
  `FUN_0004D668(block+0x20)` rate with the ±30 minimum. The hand-off point
  is `[0x107DC8]+0x18 ± block+0x30`, clamped `±0x1620`.
- **Class-3 yaw snap** (`0x4E9A2..0x4EA9B`): `fifa96_camera_yaw_snap`
  implements both class-3 arms (`track_z > 0x8A0`, point `±0x720/0xB60`,
  full ratio) and both non-class-3 arms (`track_z < -0x8A0`, point
  `±0x720/-0xB60`, half ratio). The point angle is the negated
  `FUN_0004C7D0` wrapper; the native stores the raw 32-bit `angle ± ratio`
  in `camera+0x58` and only masks the comparison delta.
- **Horizon / y / ratio / tail.** The horizon is the native
  `(-11*block[0])>>3 + 0xE09` and `(-13*block[0])>>3 + 0xAA1`, not the old
  `rnd` argument. The y ease uses `block[0]` (`+100` while
  `block[0] < 300 && camera+0x18 > 0xB10`), the ratio eases to `block+0x04`,
  both at `FUN_0004D668(8)`, and the handler copies
  `+0x40/44/48 = +0x34/38/3C`.

**Block state.** The native block is writable. The port’s
`fifa96_camera_handler_block` now carries `cell18/cell1c/pan/pan_step` and
the `+0x50` record’s `sub18/sub1c`; the six table rows carry the initial
`+0x1C/+0x20/+0x38/+0x3C/+0x40` cells and the sub-record bounds read from
`0x10896C` and the `+0x50` records. Callers pass a mutable block copy.

**Verification.** `make check` **124/124**; M1
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` and M2
`41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`
byte-exact. `test_camera` pins the pan-step branches, the two pan/cell
clamps, the class-3 and class-4 slide bounds, the slide rate split, the
pitch correction and low-altitude arms, the class-3/non-class-3 yaw snap, the
native horizon, and the y/ratio eases.

**Remaining legs.**

- **FU351-L2c-1** the computed handler call `FUN_000505D0 0x51056` passes
  caller-register leftovers as `FUN_0004E834`’s third/fourth arguments; only
  the view-driver `(0,0)` is statically proven. The port uses `(0,0)`.
- **FU351-L2c-2** `[0x107DD4] == 1` selects the `0x2EE` pitch floor and
  the `(0,camera_y<<5,±0xB10)` point. The image cell is 0 and no writer is
  known; the replay arm is not wired.
- **FU351-L2c-3** the live class remains 3. The native resolver
  (`fifa96_camera_view_select`) is ported, but class 1/2
  (`FUN_0004E3A8`/`FUN_0004EC9C`) are still partial; selecting them now would
  replace one stand-in with another.

## 13. FU351-L2c class-1/class-2 body completion (2026-11-10)

**Evidence.** Fresh `decompile_function`/`disassemble_function` for
`0x4E3A8` (363 instructions) and `0x4EC9C` (425), plus `0x4E248` (136),
`0x4E05C` (153), `0x4D698` (130), `0x4D668` (22), `0x4C77C` (43),
`0x4C7D0` (19), `0xA1A60` (32), `0xA2AD8` (4), `0xA2AEE` (5) and
`0xCE3B0` (40). `read_memory 0x10896C` (504 B) and the three `+0x50`
sub-records `0x1085AC/0x10862C/0x1085EC`.

**Landed.** The two handlers are now the full native bodies:

- **`FUN_0004E3A8` sidetrack.** Pan step `FUN_0004E248`, the `+-block+0x3C`
  clamp and the camera-relative `+-0xB10` band; the yaw point x is
  `(offset - pan) * track_x / offset` (32-bit product before the IDIV) with
  the class-2 `+0x68` / else `-0x68` bias and point z `track_z + pan`;
  `FUN_0004DF34` yaw; the four `0x4E4A0..0x4E60E` snap arms (gated by
  `track_x < -0x360` for class 1 / `< 0x360` otherwise, point z
  `pos_z +- 0x4B0` with only the outer side clamped); the `block[0]` horizon
  `0xE3B/0xAD3`; the class-2/else pitch-target split clamped `+-0xE40`; the
  `FUN_0004E05C` pitch point `(target, |target|/32, track_z)`; the x slide
  (class 2: mode 1, `lo=C[0]`, `hi=C[1]`, `hist_offset=-sub+0x10`; else:
  mode -1, `lo=-C[1]`, `hi=-C[0]`, `hist_offset=+sub+0x10`); the z slide
  (mode 2, `-C[2]..C[3]`, `hist_offset=cell18`); the rate-8 y/ratio eases
  and the history copy. It does **not** touch `track_y` except through the
  pitch arm's native `camera+0x38 = 0`.
- **`FUN_0004EC9C` staged.** `FUN_000A1A60` is
  `fifa96_projection_sincos(field3)`: the native `CE3B0` fold plus the
  `(id&0x3F)*0x6487E` correction. The cos arm lands in `block+0x10`, the
  sin arm in sub-record `+0x18`, both via
  `abs((sub+0x14 * value + 0x8000) >> 16)`. Then the pan step, `cell1c =
  step`, `cell18 += step` clamped `[sub+0x18, sub+0x1C]`, the `block[3]`
  class gate (stored in `block+0x08`), the z slide (class 3: `-C[2]..C[3]`,
  class 4: `-C[3]..C[2]`), the x slide split at `block[3] < 0x8000` (below:
  `C[0]..C[1]`; at/above: `-C[1]..-C[0]`), the yaw step, the four
  `0x4EEF2..0x4F00E` snap arms (point `(+-0x720,+-0xB10)`, 0x2000/0xA000
  band, half ratio, class 3/4 sign swap), the `0xE3B/0xAD3` horizon, the
  class-3/4 pitch target from `track_z` clamped `+-0x1620`, the
  `(track_x,0,target)` pitch point, the rate-8 eases and the history copy.
- **Model.** `fifa96_camera_handler_block` gains the sub-record `+0x10/+0x14`
  cells (`sub10/sub14`); the six table rows carry
  `{sub10,sub14} = {0,0}, {0x437,0x720}, {0,0x5DC}` for rows 0/3, 1/4, 2/5.
  The handlers take a mutable block copy; `fifa96_camera_pitch_target_a/b`
  and the two snap helpers are exported for the native branch tests.
  `camera_pose_handler_arm` now feeds `handler_tick` to both bodies (the old
  `rnd` feed was wrong: both horizons come from `block[0]`).

**Verification.** `make check` **124/124**; M1
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` and M2
`41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`
byte-exact (295,089 B). `test_camera` pins the pan decay/clamp/band, both
x-slide bound mappings and sub-record offset signs, the z cell, the class
gate and A1A60 scale, the two pitch-target formulas, and all eight snap
arms.

**Remaining legs.** `FU351-L2c-1` (computed-call register leftovers) and
`FU351-L2c-2` (`[0x107DD4] == 1` replay pitch arm) are unchanged.
`FU351-L2c-3` is now only the live wiring: the bodies are complete, but the
live camera still selects class 3 until the resolver is connected.

## 14. FU351-L2c-3 live native view selection (2026-11-10)

**Evidence.** Fresh `decompile_function`/`disassemble_function` `0x4D04C`
(67 instructions) and `0x4CE34` (32); `FUN_0004D908` (53). Fresh
`read_memory 0x1074D0`/`0x1074EC` (the slot/variant tables) and the camera
records `0x107508/0x107578/0x1075E8/0x107738`. `read_memory 0x107DD0`
starts as slot 1 / variant 0.

**Landed.** The live render camera now follows the native selection path:

- `FUN_0004D908` switches on the record `+0x08` handler selector, not the
  `+0x04` class. Slots 0/1/2 carry selector 0/1/2 and behavior row 0/1/2;
  slot 5 carries selector 5 (the class-3 action body) and behavior row -1
  (the degenerate `0x108B60` read, unused by the action handler).
- The render state carries `native_view_preset/slot/variant/class` and three
  persistent record states (`native_view_cam[0..2]`,
  `native_view_block[0..2]`, `native_view_valid[0..2]`). Each is seeded from
  the image record: eye `(-1920,160,30)`, ratio `0x1500`, yaw `0xBF9C`,
  pitch `0xC8`, behavior row = class.
- Each live frame calls `fifa96_camera_view_select(phase,
  difficulty_14c2f6, screen_leg)`. A slot/variant change takes
  `FUN_0004D04C`'s 20-step settle (`FUN_0004D908` loop) after copying the
  native focus into the target/history; otherwise one handler step runs. The
  selected class 0/1/2 body publishes its record eye/yaw/pitch/ratio to
  `render_eye_*`, `yaw`, `pitch`, `view_ratio`. Class 3 keeps the action
  handler as the slot-5 fallback.
- `fifa96_camera_behavior_block` is the mutable-row copy helper used to seed
  the handler blocks. `fifa96_match_run_eye` returns the native selected
  record eye when one is valid; the old class-3 side envelope remains only as
  the pre-selection fallback.

**Verification.** `make check` **124/124**; M1
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` and M2
`41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`
byte-exact under the wired selector (M2's transcript is unchanged). The windowed
Release build ran until the 25 s smoke timeout without an error.
`test_engine_live_control` pins phase-2 -> preset 0 / slot 1 / variant 0 /
class 1, the class-1 C bounds, the handler track (focus x/z, native-zeroed
track y) and the record->render eye/orientation publication.

**Remaining legs.** `render.view_class` (the FU-71 walk input) stays 0; the
native class source at that call site is a separate leg. The per-frame target
refresh is still the engine's native-place mapping; a distinct per-frame
target producer outside `FUN_0004D04C` is not established. The
`FUN_0004CE34` view-mode gates (`[0x14E57C]` in 0x13/0x14/0x1C/0x1D skip the
selector, plus the `FUN_00053D9C` early-out) are not staged; the engine calls
the resolver every live frame and the image/default `view_mode` is 0.
FU351-L2c-1 and L2c-2 are unchanged.

## 15. FU351-L2 residual camera closures (2026-11-10)

**Evidence.** Fresh `disassemble_bytes` `0x73A60..0x73AAF` (the FU-71 walk
threshold), `0x505D0` head/tail (`0x505D0..0x5062F`, `0x5062C..0x5072F`,
`0x51020..0x51064`), and `0x4D2D4` (118 instructions). Fresh
`decompile_function` `0x53D9C` (overlay gate), `0x365B0`/`0x36544` (the
per-frame target source/copy), `0x4B8D4` (the handler EBX selector) and
`0x4B64C` (the lead-pair copy). `read_memory 0x105FC4` = 0,
`0x109A70` = `0x155AA4`, `0x107DD0` = slot 1, `0x107DD4` = 0.

**Errata (quoted).** FU71 §2/§3 calls the `0x70/0x90` threshold input
“view_class”; `0x73A80` actually reads
`[0x14C2F6] == 0 ? 0x70 : 0x90`. FU351-L2c-2 calls `[0x107DD4] == 1` the
“replay pitch arm”; `FUN_0004D04C 0x4D07F` writes `[0x107DD4]` from the
variant table `0x1074EC`, so the condition is the selected view variant.
FU351-L2c-1 calls the handler EBX/ECX “caller-register leftovers”; the
computed call has concrete sources (below).

**Erratum (quoted).** §13/§14 say slots 0/1/2 use behavior rows 0/1/2.
That is the **camera-config** record `0x107508+slot*0x70`; the active
per-frame record is `0x107968+slot*0x70` (`FUN_0004D04C 0x4D0FE`), whose
`+0xC` is 3/4/5. The rows are duplicates in the image; the engine now uses
3/4/5 so the `FUN_0004F1C8` variant overlay lands on the right row.

**Landed.**

- **FU-71 cursor threshold.** `fifa96_camera_update`/`_walk`’s parameter is
  renamed `cursor_high`; the engine passes `difficulty_14c2f6 != 0`, matching
  `0x73A80`. The old `render.view_class` cell is removed (it had no writer
  and was not the native input).
- **View selector gate.** `FUN_0004CE34`: `FUN_00053D9C()` is false when the
  overlay state `[0x14E578]` is 0 or 6, and `FUN_00053D50()` values
  0x13/0x14/0x1C/0x1D skip `FUN_0004D04C`. The live selector now checks
  `render.display.state` and `camera_pose.view_mode`; a gated frame keeps the
  previous selected class and still runs its handler.
- **Variant pitch arm.** `fifa96_camera_handler_state` gains `view_variant`;
  the live native view sets it from `native_view_variant`. `FUN_0004E05C`’s
  `[0x107DD4] == 1` arm now uses floor `0x2EE` and ground y `camera_y << 5`;
  variant 0 keeps the `0x140` floor/point.
- **Variant C record.** `FUN_0004F1C8` writes `target0` from
  `0x107E0C[variant]` (0x340/0xB4) and points `+0x50` at
  `0x1085AC/0x10862C/0x1085EC + variant*0x20`. The table rows now carry the
  `+0x20` half (`v1_c0..v1_c3`, `v1_sub10..v1_sub1c`);
  `fifa96_camera_behavior_block_variant` applies it on init and the live step
  overlays it each frame without disturbing pan/cell state.
- **Per-frame target source.** `FUN_0004D2D4` calls
  `FUN_000365B0`/`FUN_00036544` every live frame. With the image
  `[0x105FC4] == 0`, `FUN_000365B0` returns `[0x109A70]` (the FU-71 place
  `0x155AA4`), and `FUN_00036544` copies it into camera `+0x34/38/3C` and
  clamps x to `+-0x720`, z to `+-0xB10`. The engine’s per-frame
  `render_focus` staging now applies that target clamp before the handler.

**Verification.** `make check` **124/124**; M1
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` and M2
`41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`
byte-exact. Windowed Release smoke ran to the 20 s timeout without error.
`test_camera` pins the `0x90` threshold, the variant-1 pitch result and the
mutable block copy; `test_engine_live_control` pins the overlay/view-mode
gates (same phase-4 input selects class 1 while gated and class 2 when
clear) and the `+-0x720/+-0xB10` target clamp.

**Remaining legs.**

- **FU351-L2c-1 (narrowed).** The computed `0x51056` call reads EBX from
  `FUN_0004B8D4(camera+0x3c - camera+0x48, ...)` at `0x50627..0x5062C`; the
  function returns `-1/0/1` from `[0x157AC2]`, `[0x1074A8]` and the input.
  `ECX` is `[ESP+0x14]`: for classes 0/1/2 that is the dword copied from
  `0x1577C0` (the packed lead pair), and class 3 overwrites it with
  `[0x108B7C]+0xC8` = 800. The pose-feed default arm still passes `(0,0)`;
  the open part is the `FUN_0004B8D4` side latch `[0x1074A8]`/`[0x157AC2]`
  and the packed lead dword, not unknown registers.
- **FU-351 §14 alternate target sources.** `[0x105FC4]` values 1..4 select
  `0x105FB8`, `FUN_0004B97C`, `0x109A74+0x108` or
  `0x109A74+[0x108E04]*0xC`; the image cell is 0 and those producers are not
  staged. The image/default path above is closed.

## 16. FU351-L2c-1 closure: the computed 0x51056 axes and guard (2026-11-10)

**Evidence.** Fresh `get_xrefs_to 0x505D0` = 3 (`0x4D3D9` in
`FUN_0004D2D4`, `0x4CF51` in `FUN_0004CEF4`, `0x4D009` in
`FUN_0004CF7C`). Fresh `disassemble_bytes` `0x505D0..0x50635`,
`0x506A0..0x50720`, `0x51020..0x51066`, `0x4B64C..0x4B690`,
`0x4B8D4..0x4B979`, `0x4B7D0..0x4B816`, `0x4B6FC..0x4B760`,
`0x53D50..0x53D55`, `0x6401C..0x6402E`; fresh `decompile_function`
`0x4B8D4`, `0x4BA00`. Fresh `read_memory 0x1074A8` = 0,
`0x157A83` = 0, `0x157AC2` = 0.

**Errata (quoted).** FU351 §15 says the handler `ECX` is “the dword copied
from `0x1577C0` (the packed lead pair)”. `FUN_0004B64C 0x4B658..0x4B660`
reads `dword[0x1577C0]`, `SAR 0x10`, so `[ESP+0x14]` is the sign-extended
word at `0x1577C2` — the lead **z** word, not a packed pair. FU351 §12 says
only the view-driver `(0,0)` is statically proven; the pose path’s EBX/ECX
are now resolved too. FU351 §15’s “the pose-feed default arm still passes
`(0,0)`” is superseded.

**Caller map.**

- `FUN_0004D2D4 0x4D3B3..0x4D3D9`: `EAX=1` (`0x4D3D4`) enters
  `FUN_000505D0`; `ECX=1` at the `0x51047` guard.
- `FUN_0004CF7C 0x4D002..0x4D009`: `EAX=0`, `ECX=0xA`; the guard is taken
  and the handler runs.
- `FUN_0004CEF4 0x4CF4D..0x4CF51`: `EAX=0`, `EBX=0`; it does **not** set
  `ECX`, so the `0x51047` guard is whatever its 10 callers left there. The
  port exposes that case as the staged `handler_skip` input.

`FUN_000505D0 0x50616` calls `FUN_0004B64C` first (`[ESP+0x18] =
lead_x`, `[ESP+0x14] = lead_z`), then `0x50627` calls
`FUN_0004B8D4(track_z - hist_z)` and `0x5062C` puts its result in `EBX`.
`FUN_0004B8D4` reads `[0x157AC2]`, stores the persistent `[0x1074A8]`
latch and returns `-1/0/1`; its `FUN_0004BA00` slot comes from
`[0x157A83]` through `FUN_0004BE44`. `FUN_0004B64C`, `FUN_0004B8D4`,
`FUN_0004B6FC` and `FUN_0004B7D0` preserve `ECX`; `FUN_00053D50` and
`FUN_0006401C` do not write it, so the caller’s `ECX` reaches `0x51047`.
At `0x5104B` the fourth handler argument is reloaded from `[ESP+0x14]`
(the lead z word), independent of the guard.

**Landed.** `fifa96_camera_handler_state` carries `pan_axis_a/b`,
`axis_ac2`, `axis_present`, `axis_roster`, `axis_side`, `axis_lead_z` and
`axis_latch`. `fifa96_camera_pose_axis_a` is the `FUN_0004B8D4` body;
`fifa96_camera_pose_feed` resolves the event slot with the existing
`fifa96_camera_target_slot` (`FUN_0004BE44`), runs the axis/latch before
the pose switch, and sets `pan_axis_b` from the staged lead word.
`camera_pan_update` now passes `pan_axis_a/b` to
`fifa96_camera_steady_pan_step` instead of hard-coded `(0,0)`. A new
`handler_skip` pose argument stages the zero-guard case (the axis/latch still
run before the skipped call, matching `0x505D0`); `FUN_0004CEF4` itself
inherits `ECX` from its callers, so no fixed value is asserted. The live
`FUN_0004D908` driver continues to stage the proven `(0,0)` pair.

**Verification.** `make check` **124/124**; M1
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` and M2
`41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`
byte-exact. `test_camera` pins every `FUN_0004B8D4` branch, the
zero-guard skip, and the pose-step `+17/-17` effects.

**Remaining legs.** The `FUN_0004CF7C` replay driver itself
(`[0x107DD8]=1`, the `FUN_0004F8C8` downstream) is still unported, as are
the alternate `[0x105FC4]` target sources and the non-live row-0x17
action-camera installer.

## 17. FU351-L2d replay blend body (2026-11-10)

**Evidence.** Fresh `decompile_function`/`disassemble_function` `0x4F8C8`
(202 instructions), `0x4F830` (67), `0x4CF7C` (63), `0x45069` (30) and
`0x37114` (14). Fresh `read_memory`: `0x14E470..0x14E488` = 0,
`0x14E4AC` = 0, `0x14E4B8/0x14E4BC` = 0, `0x1067F3/4` = 0,
`0x107DE0/DE4/DE8` = 0, `0x107DD8` = 0.

**Erratum (quoted).** FU351 §16 says the `FUN_0004CF7C` replay driver is
still unported. The `FUN_0004F8C8`/`FUN_0004F830` blend body and the
`0x4CF7C` start constants are now ported; the remaining part is the replay
target producer/call site, not the interpolation.

**Native shape.**

- `FUN_0004F8C8(camera, start, target, in, out)` copies `start` into the
  camera when non-NULL, snapshots it at `0x14E4F8`, and computes six
  deltas to `target` at `0x108BD4..0x108BE8`. Yaw/pitch deltas wrap through
  `&0xFFFF` and take the `>0x8000` negative arm. With no `start`, the
  current camera is snapshotted instead.
- The target arm sets `0x107DE4 = in*10`, `0x107DE8 = out*10`,
  `0x107DE0 = 0` and returns false. `FUN_0004CF7C` passes `ECX=0xA` and
  `0x1E`, so the transition is 100/300 ticks.
- Per-frame, `FUN_0004F8C8` returns true once `in+out <= elapsed` and clears
  `[0x107DD8]`; otherwise it adds `FUN_00049388`, clamps to `in+out`, and
  writes `start + FUN_0004F830(delta)`. Yaw/pitch store a dword then clear
  `+0x5A`/`+0x5E`, i.e. the low 16 bits.
- `FUN_0004F830` is the integer ease: `delta*t^2/(2*in*out)` for
  `t < in`, `delta*(2*t-in)/(2*out)` for `in <= t <= out`, and
  `delta - delta*(in+out-t)^2/(2*in*out)` for `t > out`. Each native
  product is the low 32-bit EAX result before the signed `IDIV`.
- The `0x4FA36..0x4FA80` gate runs only while `[0x107DD8] == 0`:
  `FUN_00045069`'s high-nibble result plus `FUN_00053D50() != 0x14` zeroes
  the times/elapsed/latch and returns true. `FUN_00045069` also latches
  `[0x1067F4] = 5`; `FUN_00037114(1)` issues the `0x974D8` sound request.

**Target producer.** Fresh `FUN_0004CF60` (`0x4CF60..0x4CF78`) is a
`MOVSD.REP` of the active camera record to `0x14E460`; its `+0x10/+0x14/
+0x18/+0x58/+0x5C/+0x4C` cells are exactly the `0x14E470/4/8`,
`0x14E4B8/C` and `0x14E4AC` target. `FUN_00037B34 0x37B49` calls it. So
the target is the saved camera pose, not an unlocated producer.

**Landed.** `fifa96_camera_pose6` and `fifa96_camera_replay_blend` mirror
the six native record fields and the `0x14E4F8`/`0x108BD4`/
`0x107DE0..DE8`/`[0x107DD8]` state. `fifa96_camera_replay_snapshot` is
`FUN_0004CF60`; `fifa96_camera_replay_cancel` is `FUN_00045069`'s
high-nibble/latch-5 body; `fifa96_camera_replay_ease` is `FUN_0004F830`;
`fifa96_camera_replay_begin` is the `FUN_0004CF7C` start arm with `10/30`;
`fifa96_camera_replay_step` is the per-frame `FUN_0004F8C8` arm, with
`cancel_input`/`view_mode` staging the gate.

**Verification.** `test_camera` pins the `FUN_0004CF60` snapshot, the
`FUN_00045069` high-nibble/latch-5 input, the ease at
0/50/100/200/300/350/400/500, the start copy and deltas, the 100/300 total,
the finish latch, the NULL-start snapshot, the cancel gate, the
`view_mode == 0x14` exemption and the invalid arguments. `make check`
**124/124**; M1
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` and M2
`41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`
byte-exact.

**Remaining.** The `FUN_0004CF7C` call site (`FUN_0003BB1C`) and its
`FUN_000505D0` config-record pose switch are not wired; the blend inputs are
staged. The `FUN_00037114(1)` `0x974D8` sound request is not modelled. The
state-`0xB` action-table consumer remains producer-blocked by the non-live
row-0x17 installer.

## 18. FU351-L2e/L2f action-table consumer, target switch and sound no-op (2026-11-10)

**Evidence.** Fresh `decompile_function`/`disassemble_function`
`0x36ACC` (40 instructions), `0x4BAD8`, `0x4BE44` (already ported as
`fifa96_camera_target_slot`), `0x4F274`, `0x365B0` and `0x974D8`
(1 instruction: `RET`). Fresh `get_xrefs_to 0x974D8` = 18; every
`FUN_00037114` arm jumps to the `RET`.

**Sound no-op.** `FUN_00037114(1)` loads `[0x106000]` then calls
`FUN_000974D8`, which is a bare `RET` in the shipped image. The replay
transition has no audible sink to model.

**State 0xB tail.** `0x50F6B..0x50FD0`: after `FUN_0004F8C8` reports the
blend finished, `FUN_0004BAD8` returns -1 for a NULL `[0x10F370]` or the
`FUN_0004BE44` slot otherwise. A non-negative slot copies the 12-byte
`0x155AB0 + slot*0xC` entry to camera `+0x34/38/3C`, then
`FUN_00036ACC` runs. `FUN_00036ACC` computes the negated
`FUN_0004A2A10` angle of the target minus the eye, takes the signed yaw
delta, and if the `3*ratio/4` step leaves a remainder inside the delta
moves yaw by that remainder.

**`[0x105FC4]` switch.** `FUN_000365B0`: 0 -> `[0x109A70]`, 1 ->
`0x105FB8`, 2 -> `FUN_0004B97C`'s `0x14C454` record copy, 3 ->
`0x109A74 + 0x108` (slot 22), 4 -> `0x109A74 + [0x108E04]*0xC` for
`[0x108E04]` 0..21, else `[0x109A70]`. The early arm returns
`0x105FE8` when `FUN_00063FD0()` (`[0x109A98] & 0x80`) is zero and the
active record's class is -1.

**Landed.** `fifa96_camera_yaw_approach` is `FUN_00036ACC`;
`fifa96_camera_action_target` resolves the staged event through
`fifa96_camera_target_slot` and copies the staged 23-entry table;
`fifa96_camera_action_step` composes the replay step + target;
`fifa96_camera_target_select` is `FUN_000365B0` with staged triples;
`fifa96_camera_replay_begin_gated` is the `FUN_0004CF7C` head gate
(`[0x105FFC] == 7` skips the blend). The engine now builds the 23-slot
`0x155AB0` table from its two team record blocks + the ball and feeds
`render.camera_target` into the live view step; source 0 defaults to the
existing focus, and sources 1/2/3/4 are testable staging inputs. The
row-0x17 `[0x10F370]` producer remains non-live (FU-201), the `0x36200`
writer is not wired, and the pan/record target producers remain staged.

**Verification.** `test_camera` pins both yaw-approach signs, the no-event
and NULL cases, the immediate and unfinished action-step arms, all five
target sources, the 0..21 index bound, the replay fallback, the `0x105FFC`
gate and the NULL table/source cases. `test_engine_live_control` pins the
live source-1/2/3/4 staging, the source-3 ball slot, the indexed player
slot, the force-focus and replay-fallback gates, and the source-0 default.
`make check` **124/124**; M1/M2 byte-exact.

**Remaining.** `FUN_0004F274`'s PTR/handler side effects, the
`FUN_0003BB1C` replay call site and the `FUN_000505D0` config-record
bookkeeping (the latter is overwritten by `FUN_0004F8C8`'s start copy and
cannot change the blend pose). The pan/record/slot-index target producers are
not staged.

## 19. FU351-L2g pan-target producer and phase-0xC source writer (2026-11-10)

**Evidence.** Fresh `decompile_function` `0x4B308` (48 instructions) and
`0x36208` (227), `read_memory 0x105FC8` = `{0x12C0, 0x2580×7, 0}` and
`0x8BC01..0x8BC48` (`phase == 0xC` sets `0x158830 = (0x960,0,0)` then
`EAX=3; CALL 0x36200`).

**Preset helper.** `FUN_0004B308` is phase 3 -> 1/2, phase 4 -> 3/4,
phase 0x15 -> 5/6/8, else 0, keyed by `[0x14C2F6] == 1` and
`[0x15B680]`. It is now `fifa96_camera_view_preset`, shared by
`fifa96_camera_view_select`.

**Pan producer.** `FUN_00036208` is active only when `[0x105FB4] >= 0`
and `[0x105FC4] == 1`. Its input low byte maps bits 1/2 to x ±speed and
8/4 to z ±speed; bits 0x10/0x20/0x40 suppress both. The mode's
`0x105FC8` offset builds the z window around the `0x15774C+8` place,
then the mode arms clamp: 0 -> ±0x9C0, 1..4 -> ±0xB10, 5 -> x ±0x720
with z 0..0xB10, 6/7 -> x ±0x720 with the same z, 8 -> x ±0x630 with
z 0..0x9F0 and the ±0x7AF rail. The deltas zero at a bound and clamp
±0xA0.

**Landed.** `fifa96_camera_pan_target_step` is the `FUN_00036208` body
with `FUN_0004511D`'s input/speed staged. `fifa96_camera_view_preset`
is `FUN_0004B308`. The phase-`0xC` frame now writes
`render.camera_target.source = 3` at the native `0x8BC43` point.

**Verification.** `test_camera` pins the input bits, mode-0 x/z clamps,
mode 5/6/8 bounds and the device/source gates. `test_engine_live_control`
pins the phase-`0xC` source-3 write plus the live source-1/2/3/4
consumer. `make check` **124/124**; M1/M2 byte-exact.

**Remaining.** `0x36200` values other than the phase-0xC 3
(the action-row 2 writes) are not wired; the source-2 `[0x157AA3]`
record target and the source-4 `FUN_0004BCC4` lineup index remain
unportable without those producers. `FUN_0004F274`'s PTR/handler
bookkeeping and the `0x3BB1C` replay UI call site remain. The row-0x17
`[0x10F370]` producer stays non-live (FU-201).

## 20. FU351-L2h row-2A source-2 writer (2026-11-10)

**Evidence.** Fresh `0x86A6B..0x86A71` and `0x86B6E..0x86B7E`:
arm 2 and the signed-stage >2 pre-dispatch both set `[0x157AA3] = rec`,
`EAX = 2; CALL 0x36200`. `FUN_0004B97C 0x4B983..0x4B995` copies
`[[0x157AA3]+0x59/5D/61]` to `0x14C454`, and `FUN_000365B0` source 2
returns that triple.

**Landed.** `fifa96_arm_record` carries `target_source`; the row-2A
pre-dispatch and arm 2 set it to 2, and row-28 arm 0 sets it to 2 for the
`[rec+0]` team-base store. `fifa96_match_action_2A` stages the current
record's `pos_x/pos_y/pos_z`; `fifa96_match_action_28` stages
`team[side].records[0]` (the team+0x59 base). Both set
`render.camera_target.source = 2`.

**Verification.** `test_engine_match_handlers` pins row-2A arm 2 and the
signed-stage >2 pre-dispatch, plus row-28 arm 0's team-record-0 target.
`make check` **124/124**; M1/M2 byte-exact.

**Remaining.** Row 29's `[nearest+0]` store is on the unwired row-29
entry (FU-142f/OL-48), so there is no live caller to attach. The
source-4 `FUN_0004BCC4` lineup index, `FUN_0004F274` PTR/handler
bookkeeping and the `0x3BB1C` replay UI call site remain. The row-0x17
`[0x10F370]` producer stays non-live (FU-201).

## 21. FU351-L2i source-4 selector body and camera-spine closure (2026-11-10)

**Evidence (fresh this section).** `decompile_function`/`disassemble_function`
`0x4BBFC` (the goal-ring equaliser scan, `0x4BC04..0x4BC51`) and `0x4BCC4`
(`0x4BCC4..0x4BD35`). `get_xrefs_to 0x4BCC4` = **1**: `0x5179F` inside the
`0x515A6` replay camera state machine (jump table `0x515A8`; state 4 is
`0x51799..0x517A4`, which stores the return at `0x108E04`). The only other
`0x108E04` writer is the init `FUN_000537F8` (`0x53922 MOV [0x108E04],-1`).
`FUN_0004BBFC` reads `0x157AC5/7`, `0x157AC9`, `0x157B02`, `0x157B16` and
`0x14C360`; `FUN_0008EE04` reads/bumps the `0x15A998/9E` 2x16 table. The
ring producer is `FUN_0008A938` case 6 (`0x8ACFD..0x8AD77`); the activity
table is initialised by `FUN_00091BC4` (`0x91C24..`) and updated by
`FUN_0008ED40` (35 fresh xrefs) before `FUN_0008EE04` selects the maximum.

**Landed.** `fifa96_camera_replay_choice_select` is the exact
`FUN_0004BBFC` + `FUN_0004BCC4` selector: the score-difference gate, the
`0x157B02` ring scan (the selected index is the first event *after* a
nonzero level, not the equaliser itself), the `0x32` bump, the strict
maximum scan with the `[0][0]` seed, the `0x14C360` first-match slot scan,
the `0`/`0x10` special returns and the `side*0xB+1` normal return. The
result also carries the un-reordered `0x155AB0` pitch index
(`side*0xB+slot`) and the native `0x157BD6` copy slot.

**Deliberate divergence.** The engine does not apply the `0x157BD6` order-1
copy or the `FUN_0004728C` -> `FUN_00047298(1)` 23-descriptor pool rebuild.
It would target the selected pitch record directly. The native copy is
reported as `copy_slot = 1`; the pool rebuild is the remaining
`FUN_00047298` leg.

**Verification.** `test_camera` pins: the tied/zero choice, the 1-0 score
(no equaliser, so no bump), a pre-existing activity count, the `0,1,0`
ring (selected index 2), the mirrored 1-2 ring, the `0x10` special, the
side-1 maximum and the NULL cases. `make check` **124/124**; M1
`09b726b7…` and M2 `41521559…` byte-exact.

**Closure status.**

| Item | Evidence | Verdict |
| --- | --- | --- |
| `FUN_0004BBFC`/`FUN_0004BCC4` | fresh `0x4BBFC`/`0x4BCC4`; sole caller `0x5179F` | selector body ported and tested |
| source-4 live producer | `0x515A6` state 4 writes `0x108E04`; `0x15A99E` activity table has no full engine producer | **target writer live** via `fifa96_match_run_replay_target` (FU-351 §23); non-goal `FUN_0008ED40` updates and the replay UI trigger remain |
| `FUN_0004F274` | 6 xrefs, all `FUN_000505D0`; copies a `0x107508+idx*0x1C` config record and runs a `0x108B80` handler | no camera-pose consumer: the replay start copy (`FUN_0004F8C8`, §18) overwrites its pose; superseded |
| `FUN_0003BB1C` | `0x3BB1C..0x3C000`; sole camera call `0x3BFED FUN_0004CF7C` | the camera body is ported (`fifa96_camera_replay_begin_gated`); the pad/audio/UI loop is a separate slice |
| `0x505D0` config-record switch | `0x505D0..0x51066`; `FUN_0004F274` callers `0x50866/0x50971/0x50CA0/0x50E50/0x50EE6/0x50F78` | superseded by the engine's direct `match_run_native_view_step`; no pose consumer |
| row-29 source 2 | FU-142f/OL-48 | no live entry; closed as unreachable |
| row-0x17 `[0x10F370]` | FU-201 | proven non-live; closed |

**Remaining.** The source-4 target writer is now §23: the selector body,
goal ring and `roster_order` ids are live through
`fifa96_match_run_replay_target`. The non-goal `FUN_0008ED40` activity
updates and the `0x515A6`/`0x3BB1C` replay UI trigger remain; they are not
part of the camera-pose spine.

## 22. FU351-L2j `0x36200` writer audit (2026-11-10)

`FUN_00036200` is exactly `MOV [0x105FC4],EAX; RET` (`0x36200..0x36205`).
Fresh `get_xrefs_to 0x36200` = **26** call sites. The source value is the
preceding `EAX`; `0` restores the focus, `2` the current/selected record,
`3` the ball slot. The live ported bodies now write their exact source:

| Site | Owner | Source | Status |
| --- | --- | --- | --- |
| `0x8BC43` | `FUN_0008BAF0` phase 0xC | 3 | wired (`render.camera_target.source = 3`) |
| `0x86A71` | row 2A pre-dispatch | 2 | wired, current record |
| `0x86B7E` | row 2A arm 2 | 2 | wired, current record |
| `0x87154` | row 28 arm 0 | 2 | wired, team record-0 |
| `0x8787F` | row 0x14 head | 2 | wired, current record |
| `0x88101` | row 0x25 head | 2 | wired, team record-0 (`[rec]`) |
| `0x892F9` | phase 0x18 head | 2 | wired, `contact_kind==3 ? rec_first : rec_second` |
| `0x89FC3` | phase 0x19 head | 2 | wired, `rec_first` |
| `0x895B8`/`0x895F2` | restart terminal | 0 | wired, focus reset |
| `0x8B864` | phase 0x20 stage 2 | 0 | wired after the situation-1 dispatch |
| `0x8BA97` | `FUN_0008B9CC` extra-time setup | 3 | wired through the chooser `camera_source` on the tied-score/no-probe arm |
| `0x875B1` | row 29 stage 0 | 2 | body writes `target_source = 2`; entry is unwired (FU-142f/OL-48) |
| `0x83937` | `FUN_00083428` slot 0x20 receiver cut | 2 | wired on the second 0x20 edge; the other machine branches stay legs |
| `0x4B204`/`0x4B283` | `FUN_0004B100` init | 0 | reset default |
| `0x4C35B` | `FUN_0004C324` | 0 | reset default |
| `0x744D6`/`0x744E8` | `FUN_000744D4` | 0 | reset default |
| `0x75125`/`0x75165` | keeper closedown | 0 | wired from `out.ui` bits 0x20/0x40 |
| `0x757E0`/`0x75805`/`0x75841` | keeper claim | 0 | wired from `out.ui` bits 0x10/0x40/0x80 |
| `0x8339B` | `FUN_000832A8` no-slot arm | 0 | wired from `match_sp_setup_832a8`; the slot-arm staging stays a leg |
| `0x92E36` | `FUN_00092E2C` | 0 | reset default |

**Landed.** Rows 0x14/0x25, phases 0x18/0x19, the restart terminal and the
phase-0x20 terminal write their native source and target triple. The row-0x25
target is the team record-0 base (`[0x157AA3] = [rec]`), matching row 28;
phase 0x18 uses `contact_kind == 3 ? rec_first : rec_second`; phase 0x19 uses
`rec_first`. The source-0 resets restore the focus after the restart and after
the phase-0x20 situation-1 re-dispatch.

**Verification.** `test_engine_match_handlers` pins the row-0x14 current
record, the row-0x25 team record-0 target, the row-0x10 no-slot `0x8339B`
focus reset, the keeper closedown edge bit 0x50 and the keeper claim edge
bit 0x10 focus resets. `test_engine_match_act`
pins the phase-0x18 rec_second, phase-0x19 rec_first, the restart focus
reset and the phase-0x20 stage-2 reset. `test_phase_drivers` pins the
`0x8BA97` source-3 `camera_source` on the tied-score/no-probe arm.
`test_engine_match_handlers` also pins the row-0x10 second-0x20-edge
receiver cut (`0x83937` source 2). `test_arm_bodies` pins the row-29
`target_source = 2` body write. `test_outfield` pins the
`fifa96_sp_target_triple`/`fifa96_sp_pick_receiver` helpers. `make check`
**124/124**; M1/M2 byte-exact (non-live tapes never read `camera_target`).

**Remaining.** The `0x83937` camera branch and the delivery returns
1/6/4/3/5/2 are wired (the latter live-gated to keep M2's derived
timer/draw arm); the `0x158780` sub-index timers, the target staging
velocity and the `FUN_000786A0`/`FUN_0004C374`/`FUN_000361A4` sinks stay
legs. Row 29's source-2 store is on the unwired row-29 entry. The
source-4 replay target writer is FU-351 §23; its non-goal `FUN_0008ED40`
activity updates and the `0x515A6`/`0x3BB1C` replay UI trigger remain.

## 23. FU351-L2k source-4 replay target writer (2026-11-10)

**Evidence.** `FUN_0008A938` case 6 appends the goal side (`0x157B02`) and
the scorer descriptor (`0x157B16`) to the 10-entry ring, shifting left when
full (`0x8ACFD`). `FUN_00091BC4` initialises the `0x15A998/9E` 2x16 table
from `[[0x14C360]]`; `FUN_0004BBFC` bumps the selected event by 0x32 through
`FUN_0008EE04`. `FUN_0004BCC4` is called only at `0x5179F`, which stores its
return at `0x108E04`; `FUN_000365B0` source 4 reads that index (FU-351 §21).

**Landed.** `struct fifa96_match_run_replay` now carries the goal ring
(`goal_side`/`goal_roster`/`goal_count`) and the activity counts.
`fifa96_match_run_score_event` appends a ring entry when the score word
changes, storing the scorer's `desc_d` roster slot. `fifa96_match_run_replay_target`
builds the `0x14C360` ids from `roster_order` when the PCCD order is
available (else `sub_lineup`, else the slot identity), calls
`fifa96_camera_replay_choice_select`, persists the bumped counts, and stages
`render.camera_target.source = 4` with the selected `0x155AB0` pitch index.
The 0x81 -> PLAY arm of `fifa96_match_run_replay_step` runs it; the exit arm
restores source 0 and index -1.

**Deliberate divergence.** The engine targets the selected pitch record
directly (`side*0xB + slot`) instead of applying the native `0x157BD6`
order-1 copy and `FUN_0004728C` -> `FUN_00047298(1)` pool rebuild (the
FU-351 §21 `copy_slot` seam).

**Verification.** `test_engine_match_render` pins the `0,1,0` ring selection
(side 0 / pitch 7), the `0,1,1` ring special (side 1 / pitch 0 -> target 11),
the ARMED->PLAY writer, the exit reset and the score-event ring append.
`make check` **124/124**; M1/M2 byte-exact.

**Legs.** The non-goal `FUN_0008ED40` activity updates (35 native call sites)
are not wired, so the table only carries goal-event bumps; the native
`0x515A6` replay camera state machine and the `0x3BB1C` UI loop still gate
the replay trigger in the host. The selector body and the goal ring are
exact; those legs are the remaining source-4 fidelity work.

### 23.1 Erratum (2026-11-11) — the non-goal activity wiring

§23's "35 native call sites are not wired" is superseded for the ported
handlers. The run now carries the native 2x16 table
(`struct fifa96_match_stats`), the 2x-weight `fifa96_stats_weight` path, the
max triple and the `render.replay.activity_count` mirror; a non-goal stat now
feeds the source-4 selector (`test_replay_target_consumes_stats`). Twelve
native addresses are wired: row 1A `0x7694F/0x76A04/0x76A35`, row 1B
`0x77451/0x77493/0x7749F` (plus the dead `0x774B2` shared call), row 09
`0x80D6C/0x80D99`, row 08 `0x813CD` (dead while `user_row_byte` is staged 0)
/`0x813F5`, and row 0B `0x81BBE`. The remaining 23 sites are 19 in unported
camera/announce/row-1C-tail bodies, 2 in the row-1B `0x7759C` tail and 2 in
the `FUN_0007A490` tail; FU-72's closure section lists them. The fresh
`get_xrefs_to 0x8ED40` census is 35 (the earlier 29 omitted `0x775FD`, the
row-1A three and the `FUN_0007A490` two). The rebuild/trigger legs named in
§23 are unchanged.

### 23.2 Erratum (2026-11-11) — the replay trigger (§9 closure)

§23's "the `0x3BB1C` UI loop still gate[s] the replay trigger in the host" is
superseded for the trigger, the state machine and the overlay. The native arm
`FUN_00064270` (`0x64270`) is `if ([0x109AB0] != 0) { FUN_0005865C();
[0x109A98] = 0x81; }` (FU-111 §4, FU-116 §1). `fifa96_match_run_replay_begin`
stores **`0x81` only** — the native has no `MOV [0x109A98],0x80` producer and
none is invented — and stages `hud_armed`, `phase_counter = 0`, `cursor = 0`,
`camera_index = 0` plus `fifa96_match_run_replay_camera_set(0)`.

**Buffer-gate model.** `[0x109AB0]` is the allocated recorded-frame buffer
(`FUN_00063EBC`). The engine has no frame ring yet, so `begin`'s modeled gate
is the live, unpaused, `FIFA96_MATCH_SCREEN_ACTIVE` run; the buffer's own
size/count gate is the recorded-frame-ring leg.

**Trigger.** `FIFA96_ENGINE_KEY_REPLAY` (14) is SDL3 `R` / gamepad `BACK`. The
MATCH dispatch arms on its 0→1 edge (`match_replay_held` latch, reset by the
frontend bridge) and only while the run is ACTIVE, unpaused and outside the
replay family. The key never enters the FU-61 action word.

**Button map.** While the state family bit `0x80` is set,
`fifa96_match_run_input` maps the host keys onto the native button word:
Return/CONFIRM → `1`, Down → `2`, Left → `4`, Right → `8`, C/PASS → `0x20`,
Esc/QUIT → `0x80`. Rising edges accumulate in `input_buttons` across polls and
persist until the granted-frame body consumes the word, so a tap followed by a
non-granted step is not lost and a held key produces no new edge.

**Per-frame step.** `fifa96_match_run_frame` calls
`fifa96_match_run_replay_step(mr, input_buttons, state.frame_delta)` at the top
of each granted-frame iteration and then clears the word — the `FUN_000510DC`
state-1 `FUN_000642FC` call (FU-116 §1).

**ESC exit.** The engine's live ESC pause block is guarded by the replay
family, so while the replay is active ESC stages `0x80` and leaves through
`fifa96_match_run_replay_exit` (`FUN_00064E8C`'s reachable stores: state 0,
`[0x109AC4]` arm cleared, camera source 0 / index -1) instead of pausing the
match.

**Verification.** `test_replay_begin_and_step` pins the arm state, the
first-step PLAY and source-4 target, the button walk and the exit reset, and
its overlay assertions pin `ROW_NONE` while armed and `ROW_HUD` at PLAY
through `fifa96_match_run_replay_row` (the armed state has no row; the
`FUN_00064C34` row additionally needs the `hud_armed`/`[0x109AC4]` arm).
`test_replay_input_edges` pins the edge accumulation and the live clear;
`test_replay_begin_edges` pins the begin gate;
`test_engine_replay_key_arms` pins the R edge, the release, the ESC
suppression and the eventual exit. `make check` **124/124**; M1/M2
byte-exact; windowed smoke `124`. No completion number moved: no new native
body and no dispatch row.

**Legs.** The native request path `FUN_0004B100` mode 7 → `FUN_000513EC`/
`FUN_00053BB8` → `FUN_00051270`/`0x51370`/`0x512B0` → `FUN_000510DC` →
`FUN_00064270` is not wired (the host key is the trigger); the recorded-frame
ring (`FUN_00063D34`/`FUN_00063CBC`/`FUN_0006428C`) and the pause drain
`[0x9AA8]` are absent; the live-match world/clock freeze is not modeled; the
`0x5865C` pre-arm audio call is not invoked; and the full `FUN_0003BB1C`
match-screen menu/scoreboard/audio loop stays a separate UI slice. The `0x80`
state promotion in `replay_step` remains a defensive path with no native
producer.

## 24. FU351-L2l set-piece target chain closure (2026-11-10)

**Erratum (quoted).** §22's "Remaining" paragraph says the `0x158780`
sub-index timers, the target staging velocity and the
`FUN_000786A0`/`FUN_0004C374`/`FUN_000361A4` sinks stay legs. That is
superseded for the timers, velocity and the two reset clears: they are now
ported (FU-275 §4). The `0x83937` row's "other machine branches stay legs" is
also superseded for the path builder, interpolation, input tail and no-slot
build.

**Landed.** `fifa96_sp_build_path` + `fifa96_sp_target_step` implement the
`FUN_0006DAB4` three-point path and the `0x158780` interpolation. The live
slot probe builds at `0x83441` (previous frame's `0x15877B._3`), steps once,
and rebuilds at the two `0x40`-return sites `0x839BC`/`0x83AF1` with
`0x15877F`; the no-slot arm builds at `0x85572` from the draw. The raw
direction map is 8 -> 0, 1 -> 1, 4 -> 2, 2 -> 3 (`0x83723`/`0x83755`/
`0x83786`/`0x837B5`). The `0x157A6F` override now drives the live camera
focus through `match_run_live_camera_follow` (clamped by
`fifa96_camera_focus_clamp`), and the gate-fail arm restores the FU-71 focus.
The delivery returns clear the override (`0x854F4`/`0x855A3`). The cut writes
`0x157A68 = 0` (`FUN_0004C374`, twice) and `0x105FB0 = 0` (`FUN_000361A4`)
through reset-only run cells.

**Verification.** `test_engine_match_handlers` pins the live head path, all
four direction codes, the delivery override clear, the `0x40` resets, the
gate-fail reset and the `0x832A8` seed; `test_engine_match_render` pins the
override consumer and the per-team table seed; `test_outfield` and
`test_engine_team_data` pin the pure builder/interpolation and the real
`352sp.fmt`/`442sp.fmt` entries. `make check` **124/124**; M1/M2 byte-exact.

**Remaining.** All of `FUN_000786A0` (the taker list-head clear, the receiver
list link and the seven control-word clears) stays **FU275-L6**. The engine
does not set the receiver `has_slot`; that partial write left two slot holders
and was removed. Row 29's source-2 store remains on the unwired row-29 entry;
the source-4 non-goal activity updates and replay UI trigger are unchanged.

## 25. C2 closure — the native mode-7 request and the host-key adapter (2026-11-11)

**Fresh census.** `get_xrefs_to 0x64270` returns three callers: `0x383B3`
in `FUN_00038004` (the live event-0xB dispatch: `FUN_00038004` is called from
`0x49F14` in `FUN_00049B28`, the live frame's drain head, and its
`param_1 == 0xB` arm calls `FUN_00064270`), `0x38E81` in `FUN_00038630` (the
front-end screen case) and `0x5112D` in `FUN_000510DC` (the pause/replay
state machine). The fresh `FUN_000510DC` decompile shows the begin arm
(`param_1 != 0`): `[0x14E58C] = 1`, `FUN_00064270()`, `FUN_00037114(4)`,
`[0x108E0C] = 1`, `[0x108E10] = 0`.

**Verdict — documented adapter, no request seam.** The request is produced by
the live event-0xB dispatch (`FUN_00038004` `0x383B3`), the front-end screen
case (`FUN_00038630` `0x38E81`) and the pause/replay state (`FUN_000510DC`
`0x5112D`); the §23 chain (`FUN_0004B100` mode 7 → `FUN_000513EC`/
`FUN_00053BB8` → `FUN_00051270`/`0x51370`/`0x512B0` → `FUN_000510DC` →
`FUN_00064270`) still describes the request path. The port has no replay
menu item; the host `R`/BACK key remains the documented adapter at the arm
(`FUN_00064270`), and `replay_begin` already stores the native arm (`0x81`).
The native request cells (`[0x8E0C]`/`[0x8E10]`/`[0x4E58C]`) and the timed
exit (`FUN_00064E74`/`FUN_00064DFC`) stay a numbered leg: the request path is
entangled with the recorded-frame ring, so a partial seam would claim more
than it proves. No code in this slice.

**Remaining.** The request cells, the timed exit and the recorded-frame ring
(`FUN_00063D34`/`FUN_00063CBC`/`FUN_0006428C`, the `0x118`-byte record
pack/apply and the `0x55AA4` live block) stay one leg; the full
`FUN_0003BB1C` match-screen loop stays a separate UI slice.

## 26. C1/C3/C4 closure — replay input, live freeze and pre-arm audio (2026-11-12)

Native facts restate the 2026-11-11 design §6.5/§6.6/§6.10/§6.11 session and
the Task 2–6 reports; this slice made no fresh Ghidra reads.

**C1 — the three input minors (code).** (1) `struct fifa96_match_run_replay`
gains `input_primed`: the first replay-family poll after `replay_begin` only
primes `input_buttons_held` and consumes no edge, so a key held across the arm
(the host kickoff auto-PASS is the reachable case) cannot fire a spurious
`0x20`; later polls keep the rising-edge accumulation. (2)
`struct fifa96_match_run` gains `input_resync`: `replay_begin` clears
`live_switch_press`/`live_tackle_press` and the replay input word,
`replay_exit` sets the flag, and the first non-replay poll adopts the current
switch/tackle down levels, clears both press flags, clears the flag and skips
the two edge branches (the FU-61 word fold still runs); `init`/`begin` clear
it. (3) `fifa96_engine.c` computes `replay_down`/`replay_edge`/`replay_guard`
before the kickoff block and puts the whole auto-PASS + `match_halftime_*`
dwell block (including the non-wait reset) inside `if (!replay_guard)`, so the
arming edge and every replay frame inject no PASS and do not tick the dwell;
the first post-exit step resumes it. The auto-PASS is a host adapter (the
native half-time gate is player-driven), hence the gate lives in the engine
driver, not the match body.

**C1 tests.** `test_engine_match_render` `test_replay_arm_held_key` (held PASS
primes with no edge; release then press edges `0x20`) and
`test_replay_switch_tackle_resync` (arm clears the latches; the exit poll
adopts a held SWITCH with no edge, a second held poll adds none, a release
clears the level, a fresh press edges once); the existing
`test_replay_input_edges` primes with one explicit no-key poll and keeps its
prior assertions. `test_engine_match_bridge`
`test_kickoff_auto_pass_replay_gated` stages `live_start = 1`, arms on the R
edge, asserts no injected PASS and `hold == 10` on the arm step and on a later
replay frame, then a bounded post-exit dwell resume. Review `22e8d1d` added
the exit re-sync and the later-frame/exit assertions and observed both
relevant mutations as RED before reverting them.

**C3 — the live-match freeze (code).** Native: the draw loop `FUN_000495B0`
tests `FUN_00063FD0` at `0x4974F` and, while the family is set, runs the
replay path (`FUN_0004A068`, `FUN_000635B8`, `FUN_00053D7C`, conditionally
`FUN_00053BB8`) and skips the live path (`get_xrefs_to 0x63FD0` = 14
readers). `fifa96_match_run_frame` branches at the top of the body: the pace
grant is consumed directly with `fifa96_match_pace_tick` (the state library
couples its grant to the clock and would starve the replay step),
`state.frame_delta` is the same `FIFA96_MATCH_STATE_STEP >> 8` whole-frame
delta, and `fifa96_match_run_replay_step` runs once per granted frame with the
replay input word (cleared after each). The skipped live-only set is the
clock/`tick_total`/period, the kickoff timer `[0x5882A]`, the live input and
control-slot model, the camera out-of-bounds goal arm, the entity/ball pool
and its shapers, the referee, the phase machine/driver, the screen scheduler,
the shot/keeper/goal blocks, the ball machine, the act timer and the
end-word/`mark_over` tail. The kept tail is the pose feed, the live view driver
(`match_run_live_camera_follow`, the only source-4 camera/view consumer and
live `render_eye_*` writer; `replay_exit` only resets the source) for live
runs, the FU-90 display timer, positions-only staging
(`match_run_scene_stage_positions`) and the camera trace. The live pan/event
walk is deliberately not run so frozen frames spend no shared `mr->rng`
(review F3); the animator split keeps the sim-visible
`rec->frame`/`rec->anim_id` frozen (review F2). The `0x20` cycle only stores the
camera-record selection (`camera_kind`/`camera_sub`/`record_index`), which the
view does not consume (leg).

**C3 tests** (`test_engine_match_frame`). `test_replay_freezes_live_frame`:
three granted frozen frames leave the clock, `tick_total` and record `pos_x`
unchanged with the replay state `PLAY` and `frame_delta != 0`; the next
post-exit granted frame advances `tick_total`. This was the RED witness for the
freeze. `test_replay_frozen_live_view_applies_target` pins the source-4 target
consumed into `native_view_cam[1]`/`render_eye_*` on a frozen live frame.
`test_replay_frozen_scene_stage_keeps_record_anim` pins the positions-only
staging (render slot restaged, record/slot anim fields unchanged).

**C4 — the pre-arm audio duck (code).** Native `FUN_0005865C` sets
`[0x109014] = 0`, calls `FUN_00056B88(0x2B)` (a channel stop) and, when
`[0x14E9B8] > 0x7F`, divides the 0x5C-byte channel-volume array by 3 and calls
`FUN_00056894` (the audio setup). The engine applies the persistent shape to
the one channel it owns: `replay_arm` stops the crowd voice
(`fifa96_mixer_stop`, `crowd_voice = -1`) and sets `replay_ducked`;
`match_audio_percent` returns `sfx_percent / 3` while ducked and every gain
caller takes it; `audio_step` does not re-arm the crowd while ducked;
`replay_release` only clears the flag and the next step re-arms through the
existing voice-active check. Both hooks are idempotent and silent with no
bank. The run cell `replay_audio_armed` (set in `replay_begin`, cleared in
`replay_exit`/init/begin) is the engine edge: the non-headless
`audio.active` branch calls arm/release before `fifa96_match_audio_step`.

**C4 tests.** `test_engine_audio` `test_no_assets_is_silent` (flag set,
`crowd_voice == -1`, release clears, no-bank silence) and the ISO-gated
`test_retail_banks` (crowd stop on arm, no re-arm on a ducked zero-tick step,
idempotent second arm, release re-arms, whistle/goal gains read
`sfx_percent / 3` on both arms and `sfx_percent` after release).
`test_engine_match_bridge` `test_engine_replay_key_arms` pins the run cell and
`test_engine_replay_audio_duck_edge` (non-headless) pins the engine edge
through the R arm, the R release and the replay exit. Review `5945cdb` added
the engine-edge and 1/3-gain coverage and observed both mutations as RED.

**Remaining.** The `0x20` camera-record cycle's view consumption in the native
replay driver and the recorded-frame ring; the request cells/timed exit and the
full `FUN_0003BB1C` match-screen loop from §25. **C4-L1:** the native `0x2B`
channel identity (the stop may be commentary/music rather than crowd) and the
native restore are unproved; the engine release is the adapter.

## 27. Closure (2026-11-12) — the recorded-frame ring leg landed

**Erratum (quoted).** §26's "Remaining" line names "the recorded-frame ring"
as open; that leg is now landed by the recorded-frame-ring slice (design
`2026-11-12-recorded-replay-ring`, Tasks 1–5). The engine now records every
recordable live frame into the 150 × `0x118` ring, applies records back into
the `0x155AA4` view, and renders the applied view (22 player slots + the ball
+ camera pose) instead of the frozen live pool. The remaining ring-scope legs
are **RR-L5** (the `0x20` replay camera record cycle / matrix precedence — the
non-pose cameras still use the existing source-4 path) and **RR-L7** (the
mode-7 request/timed exit). The §25 request cells/timed exit, the §26 `0x20`
view-consumption note and the `FUN_0003BB1C` UI loop are all folded into
RR-L5/RR-L7 and the standing UI leg; C4-L1 is unchanged. No completion-number
movement and no golden re-pin.

## 28. Erratum (2026-11-12) — the C4-L1 audio attribution is a misread

Fresh decompiles (the 2026-11-12 open-items-closure design §4.4/§9) show the
C4 audio attribution is wrong. §26 records the arm as audio: `FUN_0005865C`
"calls `FUN_00056B88(0x2B)` (a channel stop)" and "divides the 0x5C-byte
channel-volume array by 3". Both are misreads of a screen/projection
transform.

`FUN_0005865C` at `0x5865C` is called from the replay arm `FUN_00064270` at
`0x6427F`. `decompile_function 0x56B88` shows `FUN_00056B88(0x2B)` writes a
12-dword table at `0x1543A4` from `param_1` and `-param_1`, copies it to
`0x14E838`, adjusts `0x14E838`/`0x14E840`/`0x14E844`/`0x14E84C`/`0x14E850`/
`0x14E85C`, and calls `FUN_0004C478` over `0x14E868..0x14E9B8`.
`decompile_function 0x56894` shows `FUN_00056894` builds a 12-value `0x1543A0`
matrix from `param_2`/`param_3 << 8` and the `FUN_000A1A60` outputs. The
`0x14E9B8` 0x5C-byte array is transform data divided by 3, not a
channel-volume array; there is no `0x2B` mixer channel stop at the arm.

The engine audio duck is therefore an **adapter** with no native audio gate at
the replay arm. C4-L1 is **closed as a misattribution**, not as a proved `0x2B`
channel. The arm's real `0x5865C` screen/projection reset (the replay view
setup) is a new **C4R-L1** leg owned by Slice B. No code, no golden and no
completion-number movement.

## 29. Closure (2026-11-13) — the Slice B camera/stats spine landed

The class-3 camera's live chain (the spine the FU-71/FU-72 stat sites hang from)
is now ported end-to-end in the engine frame, in native order:

```text
fifa96_camera_walk_jitter   (0x736B2..0x73722 — head jitter, Task 6)
fifa96_camera_commit        (0x73723 + shadows 0x73728..0x73750, Task 5)
save-head gate / tracker    (0x72ACA..0x72AEA, called at 0x73756)
fifa96_camera_save_tail     (0x72AEA..0x736A9, Task 9) + stats drain
cam->paused = flag_157820   (0x7375E bridge, Task 6)
fifa96_camera_update_walk   (prev capture, pause test, walk)
```

The armer head (`[0x15781C]` counter age/cap + the unconditional `0x7134F`
`fifa96_camera_reposition` call) and the six `FUN_0007131C` post-arm sites run
in `fifa96_match_goal_arm` / `match_run_goal_post_arm`. The reposition's bit-0x8
boundary arm and its same-zone `flags_prev != flags_cur` step-back guard
(`0x710FD/0x71101/0x71106`) are ported; the `0x71231` reseed stays **T4-L1**.

**Actual wired count (19):** 13 emitting + resolving (the two `0x70F44`/`0x70FCE`
+3 boundary arms, the six post-arm sites, the tracker `0x72A52` +0xF, and the
four row-1C tail sites `0x77DFD`/`0x77E38`/`0x77E4E`/`0x77E61`), 2 emitted but
drained to zero (`0x72DE3` +1 / `0x72ED1` +2, `REC_1575DA` OPEN), and 4 parked
open legs (`0x72649` +0x32, `0x72660` −0x4B, `0x7270F` +0x19, `0x8AD54` −0x96).
The M2 golden is re-pinned (SHA `3bdfbbe…`, 3,978-frame hash-only diff) because
the abridged stream omitted the native `0x72CF9`/`0x7309E` target-block draws.
Completion unchanged (78/78, 61/80 wired / 64/80 install-live).
