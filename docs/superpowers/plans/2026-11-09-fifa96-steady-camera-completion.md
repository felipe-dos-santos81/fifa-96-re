# FIFA 96 — steady camera body completion (FU-351 L2c)

Goal: finish the native class-0 steady camera body (`FUN_0004E834`) and its
helpers so the handler is no longer a partial stand-in. The live class stays 3
until the class-1/2 handlers are complete; this slice removes the steady-body
blocker.

## Evidence

- Fresh `disassemble_function 0x4E834` (364 instructions).
- Fresh `disassemble_function 0x4E05C` (153), `0x4E248` (136), `0x4D698`
  (130), `0x4D5F4` (54), `0x4D668` (22), `0x4C77C` (43), `0x4C7D0` (19).
- `read_memory 0x10896C` (504 B), the six `+0x50` records, and
  `0x108BAC`/`0x108BB0`/`0x108BB4`/`0x107DD4`.
- `FUN_0004D908 0x4D932..0x4D948` calls `FUN_0004E834(camera, block, 0, 0)`.

## Tasks

1. **Slide rate mapping.** RED: pin that `FUN_0004D698`'s strict inside
   branch uses the block rate and its outside branch uses rate 8. GREEN:
   keep the native assignment and pin both. Verify `test_camera`.
2. **Pan/roll block and mutable cells.** Port `FUN_0004E248` as a pure step,
   add the native block fields (+0x18/+0x1C/+0x20/+0x34/+0x38/+0x3C/+0x40 and
   the C-record +0x18/+0x1C cells), and run the pan accumulation plus the
   sub-record clamp in the steady handler. Verify `test_camera`.
3. **Pitch arm.** Port `FUN_0004E05C` (the two `FUN_0004C77C` looks, the
   `ratio/8` midpoint, `FUN_0004D5F4` with `param4=1`, the ratio correction,
   the `FUN_0004C77C` ground-point guard, and the `FUN_0004D668` rate). The
   arm also zeroes `camera+0x38`. Verify `test_camera`.
4. **Class-3 yaw snap.** Port `0x4E9A2..0x4EA9B` as a pure helper and test the
   four branches. Verify `test_camera`.
5. **Horizon, y ease, ratio ease, tail copy.** Replace the `rnd` horizon with
   the native `block[0]` products, add the y-ease/slope target and the ratio
   ease, and copy `+0x40/44/48 = +0x34/38/3C`. Verify `test_camera`.
6. **Gate + docs.** `make check`, M1/M2 byte-exact, append FU-351 §12, update
   the header comments and the progress/engine records. Commit and push.

## Non-goals / remain open

- The computed-call path `FUN_000505D0 0x51056` passes whatever is in EBX/ECX
  to `FUN_0004E834`; only the view driver’s explicit `(0,0)` is provable. The
  leftover-register path stays a numbered leg.
- `[0x107DD4] == 1` selects the 0x2EE pitch floor and the
  `(0,camera_y<<5,±0xB10)` point. The image cell is 0 and no writer is known;
  the replay arm stays a numbered leg.
- The live class stays 3: class 1/2 (`FUN_0004E3A8`/`FUN_0004EC9C`) are still
  partial. The native preset resolver is already ported and tested.
