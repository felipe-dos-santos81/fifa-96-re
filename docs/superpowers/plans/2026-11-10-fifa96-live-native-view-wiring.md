# FIFA 96 — live native view selection wiring (FU-351 L2c-3)

Goal: consume the ported `fifa96_camera_view_select` resolver in the live
camera path, run the completed class-0/1/2 bodies for slots 0/1/2, and keep
the class-3 action handler only as the slot-5 fallback.

## Evidence

- Fresh `decompile_function`/`disassemble_function` `0x4D04C` (67) and
  `0x4CE34` (32).
- `FUN_0004D908` switch: record `+0x08` selects handlers 0/1/2 and 4/5/8;
  record `+0x0C` indexes the `0x108B64` behavior table.
- `read_memory 0x1074D0/0x1074EC` (slots/variants) and
  `0x107508/0x107578/0x1075E8/0x107738` records: class `+0x04`, handler
  selector `+0x08`, behavior row `+0x0C`, seed eye `-1920,160,30`, ratio
  `0x1500`, yaw `0xBF9C`, pitch `0xC8`.
- `read_memory 0x107DD0` starts as slot 1 / variant 0.

## Tasks

1. Add the mutable behavior-block copy helper `fifa96_camera_behavior_block`.
2. Add per-class native view state to the render struct (class 0/1/2), seeded
   from the image record and persistent across frames.
3. Resolve the live view each frame from phase / `difficulty_14c2f6` /
   `screen_leg`; on slot/variant change run the native 20-step settle, else one
   handler step.
4. Publish the selected record's eye/yaw/pitch/ratio to the render path and
   keep the class-3 action path as the slot-5 fallback.
5. Update the live-control camera test, add the block-copy pin, run
   `make check`, M1/M2 `cmp`, and a short windowed smoke.
6. Record FU-351 §14 and the engine/progress narrative; commit and push.

## Non-goals / remain open

- `render.view_class` (the FU-71 walk input) is unchanged; its native class
  source at that call site is a separate leg.
- The per-frame target refresh remains the engine's native-place mapping; a
  native per-frame target producer distinct from `FUN_0004D04C` is not
  established.
- The computed-call register-leftover and `[0x107DD4] == 1` pitch arms remain
  the existing FU351-L2c-1/L2c-2 legs.
