# FIFA 96 — FU351-L2e/L2f action table, target selector, sound no-op

Goal: close the state-`0xB` action-table tail, the `[0x105FC4]` target switch
and the replay sound leg with exact native evidence.

## Evidence

- `0x36ACC` (40 insns): negated `0x4A2A10` target angle, signed yaw delta,
  `(ratio*3)>>2` step, move only when the remainder stays inside the delta.
- `0x4BAD8` -> `0x4BE44` (port `fifa96_camera_target_slot`); `0x50F6B`
  copies `0x155AB0[slot]` to camera+0x34/38/3C.
- `0x365B0`: source 0/1/2/3/4 switch plus the `0x105FE8` early arm when
  `[0x109A98]&0x80 == 0` and the record class is -1.
- `0x974D8` is a single `RET`.

## Tasks

1. Port `fifa96_camera_yaw_approach`.
2. Port the action-table target/step composition with staged table/event.
3. Port `fifa96_camera_target_select` with staged triples.
4. Port the `0x4CF7C` head gate (`[0x105FFC] == 7`).
5. Tests for both yaw arms, table/no-event, all sources, fallback, bounds,
   gate.
6. Build the 23-slot table in the engine and consume the selector in the live
   view step; test source 1/2/3/4 and source 0.
7. Port `FUN_0004B308`/`FUN_00036208` and wire the phase-`0xC` source-3
   writer.
8. `make check`, M1/M2, docs FU-351 §18/§19, commit/push.

## Remaining

- `0x36200` target-source writer and the pan/record producers.
- `FUN_0004F274` PTR/handler side effects.
- `FUN_0003BB1C` replay call site and `FUN_000505D0` config-record
  bookkeeping (the switch is overwritten by F8C8's start copy).
