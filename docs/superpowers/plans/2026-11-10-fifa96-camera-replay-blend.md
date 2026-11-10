# FIFA 96 — FU351-L2d replay camera blend

Goal: port the `FUN_0004F8C8`/`FUN_0004F830` replay-camera blend and the
`FUN_0004CF7C` start constants without inventing the replay target globals.

## Evidence

- `0x4F8C8` decompile/disassembly (202 instructions): start copy, six
  deltas, 0x107DE0/DE4/DE8 times, `[0x107DD8]` latch.
- `0x4F830` decompile/disassembly: the three ease arms with 32-bit EAX
  products before the signed IDIV.
- `0x4CF7C` 0x4D002 `ECX=0xA`; 0x4D027 pushes `0x1E`; target block read
  from `0x14E470/4/8`, `0x14E4B8/C`, `0x14E4AC`.
- `0x4CF60` copies the active camera record to `0x14E460`; its
  `+0x10/14/18/58/5C/4C` cells are the `0x14E470/4/8`, `0x14E4B8/C`,
  `0x14E4AC` target. `0x37B49` calls it.
- `0x45069`/`0x37114` gate: high-nibble cancel, `[0x1067F4]=5`, `0x974D8`.

## Tasks

1. Add the six-field pose and blend state.
2. Port the `0x4CF60` snapshot, the `0x45069` cancel latch, the ease and the
   start/step arms.
3. Test all ease arms, snapshot, cancel latch, start/delta, finish and invalid
   args.
4. `make check`, M1/M2, docs FU-351 §17, commit/push.

## Remaining

- `0x3BB1C` call site and `0x505D0` config-record switch.
- `0x37114(1)` sound request.
- State-0xB action table (row-0x17 producer non-live).
