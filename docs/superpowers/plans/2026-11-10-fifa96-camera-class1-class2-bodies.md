# FIFA 96 — class-1/class-2 camera bodies (FU-351 L2c)

Goal: replace the partial `FUN_0004E3A8`/`FUN_0004EC9C` handlers with their
complete native bodies. Live class selection stays 3 until both bodies are
ported and tested; wiring the preset resolver is the following slice.

## Evidence

- Fresh `decompile_function`/`disassemble_function` `0x4E3A8` (363
  instructions), `0x4EC9C` (425), `0x4E248` (136), `0x4E05C` (153),
  `0x4D698` (130), `0x4D668` (22), `0x4C77C` (43), `0x4C7D0` (19),
  `0xA1A60` (32), `0xA2AD8` (4), `0xA2AEE` (5).
- Fresh `disassemble_function 0xCE3B0` (40), the trig table fold
  `FUN_000A1A60` shares with `fifa96_projection_sincos`.
- `read_memory 0x10896C` (504 B) and the three distinct `+0x50`
  sub-records at `0x1085AC`, `0x10862C`, `0x1085EC`.

## Tasks

1. **Shared native pieces.** Generalize the yaw step to an explicit point,
   generalize the pitch arm to an explicit `(x,y,z)` point, and factor the
   pan update/horizon builders shared by handlers 0/1/2. Verify
   `test_camera`.
2. **Sidetrack body `FUN_0004E3A8`.** Port the class-2 x-slide arm, the
   non-class-2 reversed x bounds, the pitch-target class split, the four
   yaw-snap conditions and the `track_x` gates. Verify `test_camera`.
3. **Staged body `FUN_0004EC9C`.** Port the `FUN_000A1A60` sin/cos scale into
   `block+0x10` and sub-record `+0x18`, the pan cell clamp, the class gate,
   both x-slide branches, the four snap arms and the `+0x1620` pitch clamp.
   Verify `test_camera`.
4. **Dispatch plumbing.** Feed `handler_tick` to both handlers, make the
   block mutable at the dispatch, and refresh the struct/table comments.
   Verify `test_camera` and `test_engine_match_render`.
5. **Gate + docs.** `make check`, M1/M2 byte-exact, append FU-351 §13,
   update `port/ENGINE.md` and `docs/PROGRESS.md`. Commit and push.

## Non-goals / remain open

- Live class selection remains hardcoded 3 until the wiring slice.
- The computed-call `FUN_000505D0 0x51056` register-leftover arguments stay
  the existing numbered leg; this slice ports the view-driver `(0,0)` path.
- The `[0x107DD4] == 1` replay pitch arm stays the existing numbered leg.
