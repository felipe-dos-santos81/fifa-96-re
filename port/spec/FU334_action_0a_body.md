# FU-334 — action row `0x0A`: the ball-relative run/placement body

**Scope.** The last unported action body. Phase-36 wave, 2026-11-04.
Read-only `/FIFA96.EXE` (Ghidra MCP, program exactly `/FIFA96.EXE`); the
action-table targets are not Ghidra functions, so every citation is a raw
`disassemble_bytes` / `read_memory` window.

## 1. The body `0x81738..0x81905`

The range in FU-76/FU-136/FU-138 (`..0x818F5`) is short: `0x818F5` is the
in-body RESET arm; the epilogue is `0x818FC ADD ESP,0x10` + six pops + `RET`
at `0x81905`, and row `0x0B` starts at `0x81908` `[M]`.

```
0x81738  prologue; ESI = rec; EAX = [0x157A4A]>>24; CMP 2; JNZ 0x818F5
0x81754  AL = [rec+0x92]; TEST; JBE 0x8176B (stage 0); CMP 1; JZ 0x818EF
0x8176B  CMP byte[rec+0x8D],0; JZ 0x818F5              ; active gate
0x81778  ECX = &loc[0xC]; EDX = [[rec]+0x7A6]; EBX = -1
0x8178E  CALL 0x8DE8C                                  ; nearest, distance word
0x8179C  (int16)dist > 0x50 -> CALL 0x79B58 on self; JMP 0x818F5
0x817AD  CALL 0x8DCD4                                  ; out {dist, dx, dz}
0x817B9  angle(dx, dz) via 0xCD474                     ; AX
0x817CE  AX -= word[rec+0x7D]; AND AH,3; wrap > 0x200 -> -0x400
0x817E9  AX < 0 -> turn 0x17, base = face7d + 0x100
0x81805  else -> turn 0x18, base -= 0x100; same 10-bit wrap
0x81828  DL = byte[0x10F394 + byte[[nearest+4]+0xD]]; DH = 0
0x8183E  BL = byte[rec+0x9D]; OR EDX,EBX              ; +(0x9D << 8)
0x8184B  word[nearest+0x71] = DX                       ; speed71
0x8184F  quarter-sine fold of AX -> 0x114E04 Q16
0x81876  CALL 0x795A4 (FMUL); word[nearest+0x73] = AX  ; vel73
0x818B3  CALL 0x6E598 (row = rec.type, frame = turn) on self
0x818D0  rec.ran = 1; rec.timer89 = 0; rec.stage92++
0x818EF  stage 1: CMP byte[rec+0x44],0; JZ 0x818FC; else reset 0x7DAB4
0x818FC  epilogue; 0x81905 RET
```

Fields read: `+0x00`, `+0x44`, `+0x59/0x5D/0x61`, `+0x7D`, `+0x8D`, `+0x8E`,
`+0x92`, `+0x9D`; team `+0x7A6`; globals `0x157A4A`, `0x157A38`, `0x114E04`.
Writes on self: `+0x89`, `+0x92`, `+0x9E`. Writes on the nearest record:
`+0x71`, `+0x73`.

**Errata.** FU-76 §2's call list is a superset: the body calls exactly
`0x8DE8C`, `0x79B58`, `0x8DCD4`, `0xCD474`, `0x795A4`, `0x6E598`, `0x7DAB4`;
it does not call `0x8DD70`, `0x92AC8` or `0x79C50`, and `0x114E04` is indexed
inline. The angle convention is the port's `fifa96_entity_angle` (+x = `0x100`,
+z = `0`); the `+x` case therefore folds to `turn 0x18, a = 0` and velocity 0,
while the `+z` case gives `a = -0x100` and velocity `-6`.

## 2. The installer is dead

The only code-`0x0A` install is `0x7CE11 MOV EDX,0xA` + `0x7CE1A CALL 0x7D9A4`
(bytes `E8 85 0B 00 00`), inside the contact-response family
`0x7CDD8..0x7CE34` `[M]`. A fresh whole-image `search_byte_patterns` for
`d8 cd 07 00` finds **no hit**, `get_xrefs_to 0x7CDD8` is **0**, and the
handler is absent from all eight collision-response tables reached by
`FUN_0007CA54` (`0x110820`, `0x110840`, `0x110850`, `0x110870`, `0x110890`,
`0x1108B0`, `0x1108E8`, `0x110920`) `[M]`. For code `0x0A` the gate byte
`0x11068A = 0x00`, so even an entry would skip the install `[M]`.

**Verdict:** action `0x0A` has no originator in the shipped binary. The body is
ported (`fifa96_match_action_0A`) and unit-tested, but the action-table slot
stays NULL; wiring it would run a native-dead action.

## 3. The port

`port/src/fifa96_engine/fifa96_match_handlers.c`: `fifa96_match_action_0A`, over
`match_sp_entity`, `match_opponent_nearest`, `fifa96_arm_dist_stage`,
`fifa96_entity_angle`, `fifa96_entity_sine`, `fifa96_camera_mul16`,
`fifa96_arm_26_timer_table` and `fifa96_match_run_record_anim_install`.
Declared in `port/src/fifa96_engine/fifa96_match_handlers.h`.

Fixture `port/tests/test_engine_match_handlers.c::test_action_0a_runs_body`:
`+z` nearest -> `speed71 6`, `vel73 -6`; `+x` -> `speed71 6`, `vel73 0`;
distance `0x60` -> self `+0x93 = 0x10` + reset; stage 1 `+0x44` gate; the
phase gate; NULL.

**Legs.** The `0x10F394` continuation past index 31 is unguarded natively; the
port bounds it and leaves the tier at 0 (the phase-35 placement-limit
convention). The native `0x8DCD4` dx word is recomputed from the same 16-bit
position difference because the shared helper exposes only distance and dz.

## 4. Erratum (2026-11-04) — the second fold and `+0x75`

The §1 write list is incomplete. Fresh
`disassemble_bytes 0x8187f..0x818b9` shows the second quarter-sine fold:

```
0x8187f MOV EAX,EBX                 ; the adjusted angle `a`
0x81881 LEA ECX,[EAX+0x100]
...
0x818a4 MOV EBX,dword[EDI+0x6f]     ; the dword alias: +0x6F+2 = +0x71
0x818a7 SAR EBX,0x10                ; -> the speed word
0x818aa MOV EDX,EAX                 ; sine(a+0x100)
0x818ac MOV EAX,EBX                 ; speed
0x818ae CALL 0x795a4                ; Q16 multiply
0x818b9 MOV word ptr [EDI+0x75],AX  ; vel75
```

So the nearest record receives **both** `vel73` (`0x81876`/`0x8187b`) and
`vel75` (`0x818ae`/`0x818b9`), from the same speed word. The port now writes
both; the fixture pins `+z -> vel73 -6, vel75 0` and
`+x -> vel73 0, vel75 6`.
