# FIFA 96 — camera residual closures (FU-351 L2c residuals)

Goal: close the camera legs left open by FU-351 §14 where the native evidence
is complete: the cursor threshold source, the `FUN_0004CE34` selector gate,
the `[0x107DD4]` variant pitch arm, and the per-frame target source/clamp.

## Evidence

- `disassemble_bytes 0x73A60..0x73AAF`: the 0x70/0x90 threshold reads
  `[0x14C2F6]`.
- `decompile_function 0x53D9C`: overlay state `[0x14E578]` must be 0 or 6.
- `decompile_function 0x4D04C`: `[0x107DD4]` is the variant table value.
- `0x4E05C 0x4E109`: variant 1 uses floor 0x2EE and ground y
  `camera_y << 5`.
- `0x4D2D4` calls `FUN_000365B0`/`FUN_00036544`; image `[0x105FC4]=0`
  returns `[0x109A70]` (`0x155AA4`), then clamps target x/z to
  `±0x720/±0xB10`.
- `0x505D0 0x5062C..0x51056`: EBX = `FUN_0004B8D4(...)`; ECX = the
  `[ESP+0x14]` dword copied from `0x1577C0` for classes 0/1/2.

## Tasks

1. Rename the camera-walk threshold input to `cursor_high`; pass
   `difficulty_14c2f6 != 0`; remove `render.view_class`.
2. Add the `FUN_0004CE34` selector gate from `display.state` and
   `camera_pose.view_mode`.
3. Add `view_variant` to the handler state and implement the 0x2EE /
   `camera_y<<5` pitch arm.
4. Apply `FUN_00036544`'s target clamp in the native live view step.
5. Tests: 0x90 threshold, variant-1 pitch, both selector gates, target clamp.
6. `make check`, M1/M2 `cmp`, windowed smoke, FU-351 §15, commit/push.

## Remaining after this slice

- L2c-1 narrowed to `FUN_0004B8D4`'s `[0x1074A8]`/`[0x157AC2]` latch and the
  packed `0x1577C0` lead dword; the EBX/ECX sources are now known.
- Alternate `[0x105FC4]` target sources 1..4 (image cell 0).
