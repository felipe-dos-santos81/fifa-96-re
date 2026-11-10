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
8. Wire row-2A's `[0x157AA3] = rec` + `0x36200(2)` source-2 writer and
   row-28's team-record-0 variant.
9. `make check`, M1/M2, docs FU-351 §18–§20, commit/push.
10. Port `FUN_0004BBFC`/`FUN_0004BCC4` as the source-4 selector body and
    close the camera-spine evidence (FU-351 §21).
11. Audit all 26 `0x36200` sites; wire the live ported row-0x14/0x25 and
    phase-0x18/0x19/restart/phase-0x20 writes (FU-351 §22).
12. Wire the source-4 replay target writer from the engine roster order and
    goal ring (FU-351 §23).
13. Port the set-piece target-triple/receiver helpers and the `0x83937`
    receiver-cut branch (FU-275 §3).

## Remaining

- The source-4 live producer: `0x515A6` replay camera state machine +
  `0x15A998/9E` activity table (`FUN_0008ED40` chain).
- `FUN_0004F274` and `0x505D0` have no camera-pose consumer (superseded).
- `FUN_0003BB1C` is the replay UI loop; its `0x4CF7C` camera call is
  already ported.
