# FU-161 — the jump arm's impulsion writes and tables (R9, phase-13 recon wave 2)

**Question (FU159-L1).** Now that the ramp constant (`word[+0x83]` via
`dword[+0x81]>>16`) is fixed and the arc ported: what do the arm's **remaining
native writes** do on the case-`0x0E`/`0x0F` paths — velocity/sine/move stores
(the `0x6EE75..0x6EF20` family and the analogous `0x6ED00..`/`0x6FB20..` arms) —
and which of them are **required for the live jump to carry through**
(position/velocity continuity until landing), vs cosmetic/leg?

**Status: derived with byte cites.** The three windows hold **three different
arms**, not one family: the `0x6ED00` pair is the **impulsion** (`0x0E`, live) and
a **backstep leap** (`0x3A`, live on row `0x5C`); the `0x6EE75..0x6EF20` window is
the `0x0F` snap/relaunch/frame-3 arm (live on row `0x14`, **not** on the staged
`0x5E` chain) plus a **facing flip** (`0x2E`) and a frame-2/shot arm (`0x2C`);
the `0x6FB20` arm (`0x25`) is a 0x50 forward leap with **zero frame-table
consumers** (dead in the static tables). The live jump's carry is driven by a
**three-arm impulsion family** (`0x0E` one-shot at row `0x5E` f0, `0x2A`
sustained at f1, `0x2B` sustained + frame-freeze at f2 — plus `0x0C` grounded
steps and `0x05` snap after landing), and the native **one-shot rule** is the
arm's XOR of its own aux byte out of `rec+0x36` — which the engine does not model
(re-stages every frame). T3 must port: the impulsion (vel/speed), the snap, the
air-steer/freeze, and the aux-word one-shot model; the sine/move stores of the
other arms are position writes for other rows (legs where unported).

All Ghidra reads in this slice are fresh, program **`/FIFA96.EXE`**
(case-sensitive), flat LE-image addresses, standard MCP tools only. Prior doc
sections are not rewritten; the FU-153 §11 erratum (ramp = `word[+0x83]`) is
respected throughout.

---

## 1. Method

* Fresh `disassemble_bytes`: the three arm windows (quoted §2-§4),
  the switch dispatch `0x6E8E8..0x6EA10` (the jump table + two byte passes),
  the live-chain cases `0x6EA63`/`0x6F70B`/`0x6F597`/`0x6EBFB`, the sine-step
  arm `0x6F02D..0x6F0C8`, the `BF20` arc head `0x7C020..0x7C0C0`.
* Fresh `decompile_function`: `0x6E8E8` (full 0x3A-case switch — the source of
  every case semantic below, incl. the `0x92AC8` call context), `0x79C50`
  (face-toward), `0x79B1C` (snap, re-verified), `0x795A4` (scale), `0x8E244`
  (the native locomotion mover).
* Fresh `read_memory`: the switch table `0x6E800` (232 B = 58 dwords, split by a
  read-only script), the sine table `0x114E04` (256 B), the row table window
  `0x10EF00`/`0x10EF7E`, the frame tables `0x10E3E6` (row `0x5E`) and `0x10E432`
  (row `0x0F`), the direction tables `0x10F331` (32 B) and the stride table
  `0x10F680` (64 B).
* A bounded read-only scan of the **reconciled ported tables**
  (`src/fifa96_engine/fifa96_match_run.c`: `match_run_anim_rows` +
  `match_run_frame_records`) enumerating every frame's aux word per row with the
  row's own `last_frame` bound (`row+1`), giving the arm-consumer census §5.
* Engine cites: `fifa96_match_run.c` (arm `:1018`, arc `:1054`, mover `:504`,
  dispatch `:1213`, scene stage `:1376`), `fifa96_action_handlers.c:201` (the
  locomotion step), `fifa96_entity_update.c:63` (`fifa96_entity_sine`),
  `:187` (`fifa96_entity_distance`), row/frame tables.

## 2. The switch map — the arm's real address families

The dispatch (fresh `disassemble_bytes 0x6E8E8..0x6EA10`):

```
0x6E8F1  MOV EBP, EAX                     ; rec
0x6E8F3  ADD EAX, 0x28                    ; rec+0x28 (row pointer cell)
0x6E8F6  MOV DX, word[EAX+0xE]            ; DX = word[rec+0x36] (the aux word)
0x6E8FE  TEST DX,DX; 0x6E901 JZ 0x6FC05   ; empty aux -> the frame-advance tail
0x6E907  XOR EBX,EBX
0x6E909  MOV [ESP+0xc], EDX               ; local aux word copy (both passes)
0x6E90d  MOV word[ESP+0x10], BX           ; CL-slot = 0 (low-byte pass)
0x6E912  MOV EAX,[ESP+0xc]; XOR AH,AH; DEC EAX
0x6E919  CMP AX,0x39; 0x6E91D JA 0x6FBE6  ; case > 0x39 -> tail
0x6E928  JMP dword CS:[EAX*4 + 0x6E800]   ; THE SWITCH TABLE (0x3A entries)
...
0x6FBE6  MOV EDX,[ESP+0xc]; EAX=8; SAR DX,8
0x6FBf3  MOV [ESP+0x10], AX (CL=8); [ESP+0xc] = EDX
0x6FBfc  TEST DX,DX; 0x6FBFF JNZ 0x6E912  ; second pass = the HIGH byte
```

The table at `0x6E800` (fresh read, split; **case index = aux − 1**), with the
semantic label taken from the fresh decompile (the three windows in **bold**):

| aux | case | body | semantics (switch body) |
|---|---|---|---|
| `0x01` | `0x00` | `0x6FAD6` | frame timer := 0 |
| `0x02` | `0x01` | `0x6E930` | if `+0x87 < 2*+0x85`: frame timer := 0 |
| `0x03` | `0x02` | `0x6EA13` | `FUN_000974DC` sink; aux ^= 3 |
| `0x04` | `0x03` | `0x6EA3B` | target := pos; if speed != 0: frame timer := 0 |
| `0x05` | `0x04` | `0x6EA63` | `FUN_00079B1C(rec)` — **the SNAP** (no XOR: sustained) |
| `0x06` | `0x05` | `0x6EA6F` | target := pos (MOVSD×3) |
| `0x07` | `0x06` | `0x6EA7D` | camera-height relaunch (`[0x157802]` constants) |
| `0x08` | `0x07` | `0x6EADB` | snap; if `[0x157750] > 0x60`: frame timer := 0 |
| `0x09` | `0x08` | `0x6EB03` | if `+0x87 < +0x85`: frame timer := 0 |
| `0x0A` | `0x09` | `0x6EEB7` | pos_y := 0; `+0x87` := 0 (shared landing tail) |
| `0x0B` | `0x0A` | `0x6EB2B` | if grounded: target := pos; target += dir×0x20 (pair A) |
| `0x0C` | `0x0B` | `0x6EBFB` | grounded target step 0x20 (pair A, §5.4) |
| **`0x0D`** | **`0x0C`** | **`0x6EC4D`** | if `+0x85 != 0`: vel = dir<<1; speed = metric (decel) |
| **`0x0E`** | **`0x0D`** | **`0x6EDE6`** | **the IMPULSION** (§3.1) |
| **`0x0F`** | **`0x0E`** | **`0x6EE75`** | **snap / frame-3 / relaunch** (§4.1) |
| `0x10` | `0x0F` | `0x6F02D` | forward sine step 0x20; aux ^= 0x10; **snap** |
| `0x11` | `0x10` | `0x6F4CA` | base = ramp(0x20); timer := 0 |
| `0x12` | `0x11` | `0x6F50F` | `FUN_000974F0(500)` |
| `0x13` | `0x12` | `0x6F527` | kick-actor gate (`[0x158730]`): `[0x1104BB]` arm → `FUN_0007A084` else aux ^= 0x13 |
| `0x14` | `0x13` | `0x6F655` | if speed != 0: `word[+0x34] := 0`; `byte[+0x44] := 1` |
| `0x15` | `0x14` | `0x6F673` | target := pos; if phase == 5: install row 0x53 |
| `0x16` | `0x15` | `0x6F6AD` | while `[0x1577B6].hi < 0x5A0`: frame++ + resolve; `word[+0x34] *= 2` |
| `0x17` | `0x16` | `0x6F70B` | snap; install row 0x0F (gates §5.3) |
| `0x18` | `0x17` | `0x6F74C` | byte[rec+0x98] := 0 (shared tail, no XOR) |
| `0x19` | `0x18` | `0x6F786` | clamp `word[+0x7B]` to 2 |
| `0x1A` | `0x19` | `0x6F7A0` | like aux `0x14`: if speed != 0: `word[+0x34] := 0`; `byte[+0x44] := 1` |
| `0x1B` | `0x1A` | `0x6F7BE` | if octant != 6: `byte[+0x44] := 1`; `word[+0x34] := word[+0x32]` |
| `0x1C` | `0x1B` | `0x6F7E9` | snap; team-target keep |
| `0x1D` | `0x1C` | `0x6F82E` | octant-5 branch |
| `0x1E` | `0x1D` | `0x6F865` | if `[0x157750] < 0x10`: base=ramp(0x10), vel=dir<<2 |
| `0x1F` | `0x1E` | `0x6F8E7` | octant rotate −1 |
| `0x20` | `0x1F` | `0x6F926` | octant rotate +1 |
| `0x21` | `0x20` | `0x6F961` | forward sine step 0x30; aux ^= 0x21 |
| `0x22` | `0x21` | `0x6F9EB` | camera event at the ball/tracked record |
| `0x23` | `0x22` | `0x6FAC7` | lane gate |
| `0x24` | `0x23` | `0x6ECBC` | vel = dir×−2; speed = metric (knockback) |
| **`0x25`** | **`0x24`** | **`0x6FB2F`** | **0x50 leap: ramp(0x50), timer 0, forward step 0x30** (§4.3) |
| `0x26` | `0x25` | `0x6FAEA` | ramp(0x10); timer := 0 |
| `0x27` | `0x26` | `0x6EDC7` | `FUN_0008F188(0x62, rec, 4)` (the 0x3A tail) |
| `0x28` | `0x27` | `0x6F339` | ball-directed impulsion + face(vel) + jump |
| `0x29` | `0x28` | `0x6F0C8` | forward sine step 0x10 |
| `0x2A` | `0x29` | `0x6F597` | **row-keyed impulsion** (§5.2) |
| `0x2B` | `0x2A` | `0x6E960` | **air steer + frame freeze** (§5.2) |
| `0x2C` | `0x2B` | `0x6EF14` | frame := 2; random; install row 0x5B/0x5F |
| `0x2D` | `0x2C` | `0x6F17F` | jump + vel<<2 + face(vel) + pos += dir×0x40 |
| **`0x2E`** | **`0x2D`** | **`0x6EECC`** | **facing flip: `FUN_00079C50(−dir)`** (§4.2) |
| `0x2F` | `0x2E` | `0x6F149` | target := pos; if phase == 5: install 0x46 |
| `0x30` | `0x2F` | `0x6EF6B` | frame := 3; aux ^= 0x30 |
| `0x31` | `0x30` | `0x6EB59` | grounded target step 0x10 (pair A, rotated) |
| `0x32` | `0x31` | `0x6FA8F` | `byte[+0x9B] := 1`; `FUN_000700F4`; aux ^= 0x32 |
| `0x33` | `0x32` | `0x6EBAF` | rotated pair-A index `(oct + dword[+0x3F] + 8) & 7`: pos_x += dir×0x30; pos_z += same |
| `0x34` | `0x33` | `0x6EF81` | frame := 2; aux ^= 0x34 |
| `0x35` | `0x34` | `0x6F758` | random gate; install row 0x69 |
| `0x36` | `0x35` | `0x6F004` | `[0x10E124] := 5` countdown |
| `0x37` | `0x36` | `0x6EFCD` | countdown decrement |
| `0x38` | `0x37` | `0x6EFA7` | frame := 1; aux ^= 0x38 |
| `0x39` | `0x38` | `0x6FBE6` | the byte-pass tail (no-op body) |
| `0x3A` | `0x39` | `0x6ED08` | **0x18 leap + backward sine step 0x20** (§3.2) |

The map corrects the FU-159 §3.2 framing in one important way: the family is
**not** a jump-specific block — the case bodies are scattered by the compiler
and the **impulse family is keyed on several aux bytes** (`0x0E`, `0x2A`,
`0x2B`, …), which the same row can chain across adjacent frames (§5).

## 3. Window `0x6ED00..0x6EE75` — impulsion + backstep leap

### 3.1 case `0x0D` / aux `0x0E` — the impulsion (`0x6EDE6..0x6EE70`)

Fresh disassembly (store set; FU-159 §2.2 re-verified, the `+0x83` marker is the
FU-153 §11 erratum):

```
0x6EDE6  MOV word[EBP+0x83], 0x10          ; the ramp marker (transient)
0x6EDEF  MOV EAX, dword[EBP+0x81]; 0x6EDF5 SAR EAX,0x10   ; = word[+0x83] = 0x10
0x6EDF8  CALL 0x702F8                      ; ramp(0x10) = 15
0x6EDFD  MOV word[EBP+0x85], AX            ; jump base  \  = 15
0x6EE04  MOV word[EBP+0x87], AX            ; jump timer /
0x6EE0B  MOV EAX, dword[EBP+0x8B]; 0x6EE11 SAR EAX,0x18   ; octant = byte[+0x8E]
0x6EE14  MOV dword[EBP+0x5D], 0x10         ; pos_y = 0x10
0x6EE1B  MOVSX AX, byte[EAX+0x10F334]      ; dir_x[oct]  (table A)
0x6EE23  SHL EAX,2; 0x6EE26 MOV word[EBP+0x73],AX   ; vel_x = dir_x<<2
0x6EE2A  MOV EAX, dword[EBP+0x8B]; 0x6EE30 SAR EAX,0x18
0x6EE33  MOVSX AX, byte[EAX+0x10F33C]      ; dir_z[oct]  (table B)
0x6EE3B  SHL EAX,2; 0x6EE3E MOV word[EBP+0x75],AX   ; vel_z = dir_z<<2
0x6EE42  MOV EDX, dword[EBP+0x73]; 0x6EE45 MOV EAX, dword[EBP+0x71]
0x6EE48  SAR EDX,0x10                      ; = word[+0x75] = vel_z   (alias)
0x6EE4B  SAR EAX,0x10                      ; = word[+0x73] = vel_x   (alias)
0x6EE52  CALL 0x8DC68                      ; metric(vel_x, vel_z)
0x6EE5B  MOV word[EBP+0x71], AX            ; speed71 = metric
0x6EE5F  MOV EAX,0x0E
0x6EE64  MOV BX, word[EDX+0x0E]            ; rec+0x36
0x6EE68  SHL EAX, CL                       ; CL = 0 (low pass)
0x6EE6A  XOR EBX, EAX; 0x6EE6C MOV word[EDX+0x0E], BX   ; aux ONE-SHOT clear
0x6EE70  JMP 0x6FBE6
```

Effect: the 0x10-height jump re-arm plus the **facing-relative launch velocity**
`vel_x/vel_z = dir[oct]<<2 ∈ {−4,0,4}` and `speed71 = metric(vel_x,vel_z) = 4`
(axis) / `5` (diagonal) — the ported `fifa96_entity_distance`
(`fifa96_entity_update.c:187-205`: `(4,0) -> 4`, `(4,4) -> 5`). The metric call
reads the **just-written** vel words through the dword aliases
(`dword[+0x73]>>16` = `+0x75`, `dword[+0x71]>>16` = `+0x73`) — no separate
speed source. Tables first-hand (FU-159 §2.2, re-read window `0x10F331` this
slice): `0x10F334 = 00 01 01 01 00 FF FF FF`, `0x10F33C = 01 01 00 FF FF FF 00
01`.

### 3.2 case `0x39` / aux `0x3A` — the backstep leap (`0x6ED08..0x6EDE1`)

Fresh disassembly (sine step decoded this slice; FU-159 §2.1 re-verified):

```
0x6ED08  MOV word[EBP+0x83], 0x18; ramp(0x18)=18 -> +0x85/+0x87; pos_y=0x18
0x6ED2D  MOV EAX, dword[EBP+0x7B]; SAR EAX,0x10    ; = word[+0x7D] = facing angle
0x6ED3A  SHL AH,7; SBB EDX,EDX; ADD AH,AH; SBB ECX,ECX
0x6ED43  XOR EAX,ECX; AND EAX,0xFF; SUB EAX,ECX    ; signed 10-bit fold (index)
0x6ED4C  MOV EAX, dword[EAX*4 + 0x114E04]          ; the sine table (see below)
0x6ED53  XOR EAX,EDX; SUB EAX,EDX                  ; conditional negate
0x6ED59  MOV EAX,0x20; CALL 0x795A4                ; (0x20*sin + 0x8000)>>16
0x6ED64  pos_x -= result                           ; [+0x59]
0x6ED72  LEA ECX,[EAX+0x100]  (facing+0x100 = +90°)
0x6ED78..0x6EDA2  the same fold on the +90° index; pos_z -= result   ; [+0x61]
0x6EDB3  CALL 0x974DC                              ; sink
0x6EDB8  MOV EAX,0x3A; ... XOR word[EDX+0x0E],AX   ; aux ^= 0x3A
0x6EDC7  (case 0x27) FUN_0008F188(0x62, rec, 4); continue
```

* The fold is the Watcom signed-10-bit quadrant fold: the index/sign pair
  realizes `sin(angle)` for a **1024-step circle** with a quarter-circle index.
  Simulated on the byte sequence: `angle 0x300 -> index 0x100, negate` =
  `sin 270° = −1.0`; `angle 0x100 -> index 0x100, no negate` = `sin 90° = +1.0`
  — i.e. the table is read at `(quadrant-folded |angle|)` and negated per
  original sign. The `+0x100` second index is `sin(angle+90°) = cos`.
* The table `0x114E04` (fresh read, first entries): `0, 402, 804, 1206, 1608,
  2010, 2412, 2814, …`; `402/65536 = sin(2π/1024)` — a **1024-entry sine table
  of 32-bit 16.16 values**, sampled at the facing word's 0x400 scale. This is
  the *same* table already exposed as `fifa96_entity_sine`
  (`fifa96_entity_update.c:63`, tests pin `sine(0x100) == 65536`); the arm's
  index/mask expression is the native sign-fold of that helper.
* `FUN_000795A4` (fresh decompile) = `(scale*value + 0x8000) >> 16` — the
  rounding scale, exactly the ported `(x + 0x8000) >> 16` idiom.
* Effect: a ≤`0x20`-unit displacement **opposite the facing** (`pos −= 0x20·[sin,
  cos]`), plus the 18-base jump and the sink/chain tail.

### 3.3 the two arms in one family

| arm | jump | velocity | position | consumer rows (bounded census) |
|---|---|---|---|---|
| `0x0E` | `0x10`, base=timer=15 | **vel ±4, speed 4/5** | — | `r12 f2`, `r5E f0` |
| `0x3A` | `0x18`, base=timer=18 | — | **−≤0x20 along facing** | `r5C f2` (aux `0x243A`; high byte `0x24` = case `0x23` knockback `dir×−2`) |

## 4. Window `0x6EE75..0x6EF20` — snap / frame-3 / flip / shot

### 4.1 case `0x0E` / aux `0x0F` (`0x6EE75..0x6EEC7`)

Fresh disassembly (this is the FU-159 §2.3 arm; the `pos_y != 0` branch and the
`FUN_00079B1C` store set re-verified):

```
0x6EE75  CMP dword[EBP+0x5D], 0; 0x6EE79 JZ 0x6EE90
   --- pos_y != 0 ---
0x6EE7B  MOV EBX,3                           ; frame argument
0x6EE80  MOV EDX, [ESP+8]                    ; rec+0x28 (row pointer)
0x6EE84  MOV EAX, EBP; CALL 0x6E490          ; frame := 3 via the resolver
0x6EE8B  JMP 0x6FBE6                         ; no XOR -> sustained
   --- pos_y == 0 ---
0x6EE90  MOV EAX, EBP; CALL 0x79B1C          ; the SNAP
0x6EE97  CMP word[EBP+0x85], 0; 0x6EE9F JNZ 0x6EEB7
0x6EEA1  MOV AX, word[EBP+0x5D]; ADD EAX,0x1C; CWDE; CALL 0x702F8   ; ramp(pos_y+0x1C)
0x6EEB0  MOV word[EBP+0x85], AX              ; relaunch base
0x6EEB7  MOV dword[EBP+0x5D], 0; 0x6EEBE MOV word[EBP+0x87], 0      ; pos_y=0, timer=0
0x6EEC7  JMP 0x6FBE6
```

The snap (`FUN_00079B1C`, fresh decompile) store set: **target triple
(`+0x4D/+0x51/+0x55`) := pos triple; `word[+0x69] = 0` (lane); `word[+0x71] = 0`
(speed); `word[+0x73] = word[+0x75] = 0` (velocity words); `byte[+0x9C] = 0`
(body timer); `word[+0x65] = word[+0x67] = 0` (the mover's distance/delta
words)**.

Consumer census: **aux `0x0F` has exactly one frame-table site — `r14 f0`** (the
mirror row; `next_id 0x7F`), so this arm is **not on the staged `0x5E`/`0x12`
jump chains**. Its snap path is the same helper the live chain needs at aux
`0x05` (§5.3); the `frame := 3` resolve stays the unported remainder (FU159-L1).

### 4.2 case `0x2D` / aux `0x2E` (`0x6EECC..0x6EF0F`) — the facing flip

```
0x6EECC  EAX = octant; DX = dir_z[oct]; AX = dir_x[oct]
0x6EEe5  NEG EDX; NEG EAX                    ; (-dir_x, -dir_z)
0x6EEF1  CALL 0x79C50                        ; face(-dir)
0x6EEFE  MOV EAX,0x2E; ... XOR word[EDX+0x0E],EAX
```

`FUN_00079C50` (fresh decompile) = the **face-toward / 180° flip**:
`if (dx==0 && dz==0) return octant; angle = FUN_000CD474(dx,dz); word[+0x7D] =
angle; byte[+0x8E] = ((angle&0x3FF)+0x40)>>7` — it writes the facing angle
`+0x7D` and octant `+0x8E` from the direction pair (the same call the row
successor's mirror branch uses). **No position/velocity writes.** Census: aux
`0x2E` has **zero** frame-table consumers — in the static tables this case is
dead (the successor's inline mirror path is the live user).

### 4.3 case `0x2B` / aux `0x2C` and the tail seats (`0x6EF14..`)

```
0x6EF14  MOV byte[EAX+0x15], 2               ; [ESP+8]+0x15 = rec+0x3D: frame := 2
0x6EF1C  CALL 0x92AC8                        ; derived gate (RNG/metric family)
0x6EF21  TEST AL,1; 0x6EF23 JZ 0x6EF5D
0x6EF25  EAX=[ESP+8] -> row id via [rec+0x28] deref
0x6EF32  CMP row_id, 0x5B;  if equal EDX=0x5F else EDX=0x5B
0x6EF4D  ECX = octant; CALL 0x6E598          ; install row (frame arg 2)
0x6EF5D  EAX=0x2C; ... XOR word[EDX+0x0E],AX ; one-shot
0x6EF6B  byte[rec+0x3D] = 3; aux ^= 0x30     ; case 0x2F: frame := 3 (one-shot)
0x6EF81  byte[rec+0x3D] = 2; aux ^= 0x34     ; case 0x33: frame := 2 (one-shot)
0x6EFCD  byte[rec+0x9B] = 1; FUN_000700F4; aux ^= 0x36   ; case 0x35 (ball bind + camera reset)
0x6EFA7  case 0x37/aux 0x38: frame := 1; aux ^= 0x38
```

These are **state-store / install arms**, no motion cells. The `0x6EF20` window
ends inside this block; the census (bounded) shows aux `0x2C` live at `r5B f8`/
`r5F f8` (the shot rows) and the frame resolves at `r58 f9`/`r6B f9`.

## 5. The `0x6FB20..` arm and the live-chain family (other windows)

### 5.1 case `0x24` / aux `0x25` (`0x6FB2F`, window `0x6FB20..0x6FBC0`)

```
0x6FB2F  MOV word[EBP+0x83], 0x50; ramp(0x50) -> +0x85
0x6FB4D  MOV word[EBP+0x87], 0x0             ; jump timer := 0 (arc starts at ground)
0x6FB5C  MOV dword[EBP+0x5D], 0x50           ; pos_y = 0x50
0x6FB63..0x6FB87  facing fold; FUN_000795A4(0x30, sin(face))
0x6FB90  ADD EDI, EAX; pos_x += result       ; forward step <=0x30
0x6FB9B..0x6FBC5  the +90° fold; FUN_000795A4(0x30, cos)
0x6FBD2  pos_z += result
0x6FBD4  EAX=0x25; 0x6FBdc SHL EAX,CL; 0x6FBE2 XOR word[EDX+0x0E],AX   ; one-shot
```

Effect: a **0x50 forward leap** (arc from timer 0, forward displacement ≤0x30)
— but the bounded census finds **no frame in any row carrying aux `0x25`**
(the only 0x25 writes are the case's own XOR). **Dead in the static tables**:
cosmetic/leg, not required for any live jump.

### 5.2 the sustained impulsion arms (`0x2A`, `0x2B`) — live chain

**aux `0x2A` (case `0x29`, `0x6F597`)** — fresh disassembly: checks the current
row id byte (`[[rec+0x28]]` deref, `0x6F597..0x6F5DD`) against
`0x4D` ('M'), `0x59` ('Y'), `0x11`, `0x5E` ('^'):
`vel_x/vel_z = dir[oct]<<2` for those rows, `<<1`(=×2) otherwise; then
`speed71 = metric(vel_x, vel_z)` (`0x6F647`); `JMP 0x6FBE6` — **no XOR**, so the
byte stays and the arm **re-fires every frame the frame holds**. Live sites:
`r11 f0..f2`, `r45 f0..f4`, `r4D f0..f7`, `r59 f0..f2`, **`r5E f1`**.

**aux `0x2B` (case `0x2A`, `0x6E960`)**:

```
0x6E960  EAX = word[+0x85] (base); EDX = word[+0x87] (timer); EAX += EAX
0x6E974  CMP EDX,EAX; 0x6E976 JGE 0x6FBE6    ; landed -> plain tail
0x6E97C  row id == 0x5E ? dir<<2 : dir*2     ; the same impulse pair
0x6E9e6  word[+0x75] = dir_z; speed71 = metric(vel_x, vel_z)
0x6EA03  MOV word[EAX+0xa], 0                ; rec+0x32 (frame timer) := 0
0x6EA09  RET                                  ; no XOR -> sustained
```

While `jump_timer < 2*jump_base` this arm **re-applies the impulsion and zeroes
the animation frame timer (freezing the frame until the arc lands)**; after
landing it falls to the plain tail and lets the frame advance. Live sites:
`r45 f5`, `r4D f6`, **`r5E f2`**.

### 5.3 the stop and post-landing arms (`0x05`, `0x17`, `0x0C`)

* **aux `0x05` (case 4, `0x6EA63`)**: `MOV EAX,EBP; CALL 0x79B1C` (bytes
  `e8b2b00000` verified) — the snap. No XOR (sustained). Live on **223 frames
  across the table** incl. `r5E f5` and every `r0F f0..f4` (the `0x1805` low
  byte) — the ubiquitous stop.
* **aux `0x17` (case `0x16`, `0x6F70B`)**: snap, then
  `if octant == 0xC -> skip; if byte[rec+0x99] != 0 -> skip; else
  FUN_0006E598(rec, 0xF, 0)` (`0x6F747`, row 0x0F install) → falls into the
  shared `byte[rec+0x98] := 0` tail. Live sites: **only `r0E f0..f8`** (the
  generic fall row; `next_id 0x0E` self-loop — the arm is its gate to row 0x0F).
* **aux `0x0C` (case `0x0B`, `0x6EBFB`)**: `if pos_y != 0 -> skip; target :=
  pos; target_x += byte[0x10F331+oct]<<5; target_z += byte[0x10F339+oct]<<5`
  (MOVSD×3, `0x6EC05..0x6EC43`; no XOR — sustained). Live at **`r5E f3/f4`**
  (post-landing) and 8 other frames (`r0D`, `r12`, `r50`, `r52`). Note the
  table pair used here is **`0x10F331`/`0x10F339`**, not the impulse pair
  `0x10F334`/`0x10F33C`; fresh read: `0x10F331 = 00 00 00 00 01 01 01 00`,
  `0x10F339 = FF FF FF 01 01 00 FF FF` (different vectors — leg FU161-L3).

### 5.4 the one-shot rule (the key native behaviour)

`word[rec+0x36]` is the frame-resolve **aux word** (FUN_0006E490 copies
`word[frame+2]`, FU-159 §3.1). The dispatch processes its **two bytes as two
passes** (low then high, CL = 0/8). The arms whose body ends in the XOR tail
(`0x6EDC3`, `0x6EE6C`, `0x6FBdc`/`0x6FBE2`, the `LAB_0006fbe0` family) clear
their own byte from `rec+0x36`, so they **fire exactly once per frame resolve**;
the arms that `break`/`return` without the XOR (snap `0x05`, flip-adjacent
`0x18`, the sustained `0x2A`/`0x2B`, `0x0F`, …) **re-fire every frame the frame
holds**. Both classes are per-frame bookkeeping on `rec+0x36`, and the loop's
local copy `[ESP+0xc]` is what the *second* pass reads — the XOR only affects the
**next** call.

Class census (fresh, from the 58 switch bodies; the XOR clears the processed
byte, so the next call's switch sees 0 for that byte):

| class | aux values |
|---|---|
| **one-shot** (unconditional XOR tail) | `0x03 0x0E 0x10 0x11 0x12 0x15 0x1F 0x20 0x21 0x22 0x25 0x26 0x27 0x28 0x29 0x2C 0x2D 0x2E 0x30 0x32 0x33 0x34 0x35 0x36 0x37 0x38 0x3A` (27) |
| **sustained** (no XOR on any path) | `0x01 0x02 0x04 0x05 0x06 0x08 0x09 0x0A 0x0B 0x0C 0x0F 0x14 0x17 0x18 0x19 0x1A 0x1B 0x1C 0x1D 0x23 0x24 0x2A 0x2B 0x2F 0x31 0x39` (26) |
| **conditional** (XOR on one branch only) | `0x07` (case `0x06`: XOR on the gate-true path), `0x0D` (case `0x0C`: XOR only when `+0x85 != 0`), `0x13` (case `0x12`: XOR when `[0x1104BB]` is 0, else `FUN_0007A084`), `0x16` (case `0x15`: XOR when `[0x1577B6].hi >= 0x5A0`), `0x1E` (case `0x1D`: XOR only when `[0x157750] < 0x10`) |

The rule is *body-local* (the presence of the `LAB_0006fbe0`/`caseD_26` XOR
tails), not derivable from the aux name; the conditional arms clear only on the
branch that consumed the event. The `0x0E` impulsion and the live-chain pairs
(`0x2A`/`0x2B` sustained, `0x05` sustained) are on the lists above.

**The engine does not model this**: `match_run_record_jump_arm` is called every
frame with the frame's aux re-read from the ported table (match_run.c:1213), so
the one-shot `0x0E` re-stages `jump_timer = base` on every tick of its frame
record (row `0x5E` f0 lasts `0x40/(2*0x10) = 2` frames), freezing the arc one
step vs the native; the T1 unit test hides this by clearing `anim_id` after one
frame (test_engine_match_frame.c:2367) and the live test only polls `pos_y !=
0`. The native arc + mover order is `0x7CD2B CALL [rec+0x18]` → `0x7CD30 CALL
0x6E8E8` → `0x7CD48 CALL 0x7BF20` (the BF20 head `0x7C057 CMP word[+0x85],0;
0x7C05F JZ ...` then the `0x7C065..0x7C07A` timer advance — fresh disasm this
slice), which the engine's arm→mover→arc order at match_run.c:1213/1229/1232
matches.

## 6. The mover and the carry mechanism

`FUN_0008E244` (fresh decompile, twin of the engine's ported
`fifa96_action_locomotion_step`, `fifa96_action_handlers.c:201`):

* **no `pos_y` gate**: the position integration is
  `if (word[+0x73] != 0) pos_x += delta * (dword[+0x71]>>16);`
  `if (word[+0x75] != 0) pos_z += delta * (dword[+0x73]>>16);`
  — i.e. `pos += delta·vel` **while airborne exactly as on the ground** (the
  `>>16` dword aliases are the same trap: `dword[+0x71]>>16` = `+0x73`).
* the integration block runs `if (distance != 0 || speed != 0)`, so a record at
  its target still coasts while `speed71 != 0` — the impulsion's `speed = 4/5`
  is what keeps a **standing** record moving; the snap's `speed = 0` is what
  stops it.
* the stride retarget (`0x10F680` x / `+0x200` z words via
  `(desired&0x3F0)|attr`, `attr = distance>>3` capped by the stride nibble)
  converges `vel` toward the walk velocity on the body-timer tick; the fresh
  `0x10F680` read shows the x half at 0 for the lowest indices (0..0x15) with
  1s beyond — the retarget does not accelerate a stopped record, and the
  impulsion is pulled toward the facing's walk velocity, not away from it.

**Live-chain synthesis (row `0x5E`, 6 frames — fresh reads `0x10EF00`/`0x10E3E6`,
bounded census §1):**

| frame | aux | arm effect | motion |
|---|---|---|---|
| f0 `0x0040` | `0x0E` | **one-shot**: base=timer=15, pos_y=0x10, vel=dir<<2, speed 4/5 | launch velocity |
| f1 `0x0080` | `0x2A` | **sustained**: row `0x5E` ∈ set → vel=dir<<2, speed=metric | carry held |
| f2 `0x0040` | `0x032B` | **sustained** `0x2B`: while airborne vel=dir<<2 + **frame freeze**; high `0x03` one-shot sink | carry held until landing |
| f3/f4 `0x0080` | `0x000C` | **sustained** grounded target step 0x20 (pair `0x10F331`/`0x10F339`) | post-landing run-out |
| f5 `0x0140` | `0x0005` | **sustained snap** (velocity/lane/speed/body zeroed; target := pos) | the stop |
| (wrap) | — | successor `next_id 0x0F` | — |
| r0F f0..f4 | `0x1805` | low `0x05` snap (hold) + high `0x18` `byte[+0x98]=0` | keep stopped |

Row `0x12` (the other live installer's row, 6 frames) has the impulsion at
**f2** (`0x000E`) with f0 `0x0A` (landing tail), f1 `0x100A` (the **aux-`0x10`
forward step + snap**, case `0x0F` at `0x6F02D`: `pos += 0x20·[sin,cos]` then
`CALL 0x79B1C`, bytes `e80aa50000`), f4 `0x0C03`, f5 `0x0012`. Row `0x5C`
(9 frames) carries the aux-`0x3A` backstep at f2 (`0x243A`) and the knockback
`0x24` (`dir×−2`) at f3..f6; row `0x14` (7 frames) carries aux-`0x0F` at f0.

**Answer to the carry question (live jump).**

* **Required to carry** (native-exact velocity/position continuity until
  landing): the **vel/speed words of case `0x0D`** (one-shot seed at f0) and the
  **sustained re-applications `0x29`/`0x2A`** (f1/f2), plus the **case-`0x2A`
  frame freeze** that holds the mid-air frame until `jump_timer >= 2*base`.
  The height arc itself needs none of them (FU-153 §10) and the mover already
  carries them (`fifa96_action_locomotion_step` is ported).
* **Required to stop** (so the carry is bounded at landing): `FUN_00079B1C`
  via **aux `0x05`** (live chain f5) and the row-`0x0F` `0x1805` hold; the
  aux-`0x0F` snap path is the same helper on row `0x14` (not the staged chain).
* **Position-store arms (sine steps)**: `0x3A` (−≤0x20 backstep, row `0x5C`),
  `0x10` (+≤0x20 + snap, row `0x12` f1), `0x21`/`0x29`/`0x2D` (+≤0x30/0x10/
  0x40), `0x31`/`0x33` (pair-A / same-value steps), `0x25` (+≤0x30 leap —
  **zero consumers**) — these are **not on the `0x5E` carry**; they are the
  other jump variants' displacements (post-landing `0x0C` steps excepted,
  §5.3).
* **Cosmetic/leg**: the `+0x83` markers (transient ramp arguments, modeled by
  the constant), the aux XOR bookkeeping itself (but see the one-shot rule —
  behavioural), the sink calls (`0x974DC`, `0x8F188(0x62)`, FU159-L3), the
  camera-height arms (`0x07`/`0x1E`), the flip `0x2E` (**dead**), the frame
  resolves `0x2C`/`0x30`/`0x34`/`0x38`, the `+0x98`/`+0x99` flags
  (FU161-L4), the shot/attempt install `0x2C` (row `0x5B`/`0x5F`).

## 7. T3 wiring recommendation (concrete)

**Files:** `src/fifa96_engine/fifa96_match_run.c`, `fifa96_engine_internal.h`
(const dir tables), `include/fifa96_engine/fifa96_match_entities.h` (the
`aux_word`/`frame_freeze` cells), `tests/test_engine_match_frame.c`,
`docs/ENGINE.md`/`docs/PROGRESS.md`.

1. **Port the snap as a reusable helper** `match_run_record_snap(r)` (match_run.c
   near the jump helpers), porting `FUN_00079B1C`: `target_{x,y,z} :=
   pos_{x,y,z}; lane = 0; speed71 = 0; vel73 = vel75 = 0; body_timer9c = 0;
   vel_x = vel_z = 0` (the dword views, consistent with match_run.c:544-547; the
   `+0x65/+0x67` distance words are recomputed by the mover/lane staging and
   need no cell). Wire it to **case `0x05`** (sustained) — the live-chain stop —
   and to case `0x0F`'s `pos_y == 0` path (already partly ported).
2. **Port the impulsion as a reusable helper**
   `match_run_record_impulse(r, shift)`:
   `oct = fifa96_match_heading_table[((uint16_t)r->face7d & 0x3FFu) >> 5]`
   (the native `+0x8E` writer, FU-77); new const dir tables
   `{0,1,1,1,0,-1,-1,-1}` / `{1,1,0,-1,-1,-1,0,1}` (0x10F334/0x10F33C) in
   `fifa96_engine_internal.h` + the tables file;
   `vel73 = dir_x<<shift; vel75 = dir_z<<shift;
   speed71 = fifa96_entity_distance(vel73, vel75)`; recompose `vel_x/vel_z` from
   the words (**critical**: the dispatch's word→dword recompose at
   match_run.c:1219-1221 runs after the arm and would otherwise clobber direct
   word writes).
3. **New arm cases in `match_run_record_jump_arm`** (match_run.c:1018):
   * `0x0E`: add the impulse `<<2` (one-shot);
   * `0x2A`: `shift = (anim_id in {0x4D,0x59,0x11,0x5E}) ? 2 : 1;` impulse
     (sustained);
   * `0x2B`: if `jump_base != 0 && (int16_t)jump_timer < 2*(int16_t)jump_base`:
     `shift = (anim_id == 0x5E) ? 2 : 1;` impulse + **`r->frame_freeze = 1`**
     (sustained);
   * `0x05`: snap (sustained); `0x0C`: the grounded target step via the
     `0x10F331`/`0x10F339` pair (sustained; FU161-L3 if out of slice).
4. **The one-shot model (native `+0x36`).** Add `uint16_t aux_word` (+ a
   `aux_seen_row`/`aux_seen_frame` pair) to the pool entity and the staging
   record; in `match_run_dispatch_entity` arm the aux when `(anim_id, frame)`
   changes (covers row install and frame resolve), pass the stored word to the
   arm, and have the one-shot cases XOR their byte out (write back through the
   drain). Sustained cases leave it. This makes the `0x0E` jump seed once per
   resolve (native) instead of every tick, and lets the T1 unit test drop its
   `anim_id = 0` mask line.
5. **Frame freeze.** `frame_freeze` staged by the `0x2B` arm, drained to the
   pool, consumed by `match_run_scene_stage` (match_run.c:1405) — skip
   `fifa96_animation_advance` for that record this frame, then clear it (native
   `+0x32 := 0` + `RET`).
6. **Row successor (FU-159 §5.2 prerequisite)** — port the frame-wrap
   `next_id` install so the live chain `0x5E -> 0x0F -> 0x00` runs; the helper
   `fifa96_animation_successor` already exists (`fifa96_animation.c:82-95`,
   `0x6F` repeat / `<0x70` install / `>=0x70` mirror-`0x70`), the call site in
   `match_run_scene_stage` (on `out.advanced`) is what is missing; row `0x0F`
   then snap-holds via its `0x1805` frames and returns to terminal row `0x00`
   (`row44` 0→0→1).
7. **Tests (RED first).**
   * `test_record_jump_impulsion`: stage `anim_id = 0x5E, frame = 0`, set
     `face7d` to a known angle (e.g. `0x100` → octant 2 → dir `(1,0)`); one
     frame: assert `vel73 == 4, vel75 == 0, speed71 == 4`; a diagonal facing
     asserts `4/4, 5`. Then drive the arc to landing and assert the f5 snap
     zeroes speed/vel and sets target := pos, and the row chain reaches `0x0F`
     → `0x00`.
   * `test_record_jump_one_shot`: with the aux-word model, leave `anim_id =
     0x5E` for the whole f0 duration and assert `jump_timer == 17` (not
     re-staged) — RED against today's per-frame re-stage.
   * `test_record_jump_frame_freeze`: at f2 with an active arc, assert
     `e->frame` does not advance while `jump_timer < 2*jump_base`; after the
     landing edge, the frame advances.
   * ISO `test_live_jump_carry_and_stop` (extends
     `test_live_airborne_long_ball_seed_then_band2_follow`): after the live
     jump staging, assert `speed71 != 0` and `(vel73, vel75) == dir[oct]<<2`
     for the record's facing, that `pos` advanced along that vector between
     the f0 and f2 frames, and that after f5 the speed/vel are zero and
     `target == pos` (the snap observable).
   * Goldens: no re-pin expected (the tape rows `0`/`0x26` carry no jump auxes;
     the successor is a no-op for self-loop rows). Verify
     `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt` (sha
     `09b726b7…`) and the same for M2 (`2759dace…`).
   * Census: no action row changes -> `wired` stays 21/80, README `26%`
     unchanged.

## 8. Numbered legs (new)

* **FU161-L1 — the one-shot aux model (native `+0x36` XOR).** The engine
  re-stages one-shot arms every tick of a frame record; the class table §5.4 is
  the blueprint. Out of T3's wiring rec until step 4 lands; then closed.
* **FU161-L2 — the aux-`0x2B` frame freeze.** `+0x32 := 0` + `RET`
  (`0x6EA03..0x6EA09`) holds the mid-air frame until `jump_timer >= 2*base`;
  unmodeled. (Recommended in T3 step 5; bounded remainder otherwise.)
* **FU161-L3 — the second direction pair `0x10F331`/`0x10F339`.** The grounded
  target-step arms (`0x0B`/`0x0C`/`0x30`/`0x31`) and the `0x33` step use this
  pair (fresh bytes §5.3), distinct from the impulse pair; the vector semantics
  (rotated/diagonal family) and the rotated index
  `(oct + dword[rec+0x3F] + 8) & 7` (`0x6EBBC..0x6EBC2`) are not derived here.
* **FU161-L4 — the `+0x98`/`+0x99` flags.** `byte[rec+0x98] := 0` (case `0x17`,
  live on row `0x0F` every frame) and the aux-`0x17` install gate
  `byte[rec+0x99] == 0`; both unmodeled pool bytes (the nearest-search skip
  pair family).
* **FU161-L5 — the dead arms.** aux `0x25` (`0x6FB2F`, the 0x50 forward leap)
  and aux `0x2E` (`0x6EECC`, the facing flip) have **zero frame-table
  consumers** in the reconciled tables; a reader exhaustively scanning the
  frame records found no carrier. Keep unported unless a dynamic installer
  surfaces (cross-ref FU159-L2's residual sites).
* **FU161-L6 — the aux-keyed row sets.** `0x2A`'s `{0x4D,0x59,0x11,0x5E}`
  full-impulse set and `0x2B`'s `0x5E`-only `<<2` are byte-cited but the row
  semantics (which animations) are only named by id.

## 9. Evidence index (fresh this slice)

* `disassemble_bytes`: `0x6ED00..0x6EE75`, `0x6EE75..0x6EF40`,
  `0x6FB20..0x6FBC0`, `0x6FBBC..0x6FC00`, `0x6E8E8..0x6EA10`,
  `0x6EA63..0x6EA7D`, `0x6F70B..0x6F74C`, `0x6F597..0x6F655`,
  `0x6EBFB..0x6EC4D`, `0x6F02D..0x6F0C8`, `0x7C020..0x7C0C0`.
* `decompile_function`: `0x6E8E8`, `0x79C50`, `0x79B1C`, `0x795A4`, `0x8E244`.
* `read_memory`: `0x6E800` (232 B switch table), `0x114E04` (256 B sine),
  `0x10EF00` (18 B), `0x10EF7E` (72 B), `0x10E3E6` (77 B), `0x10E432` (26 B),
  `0x10F331` (32 B), `0x10F680` (64 B).
* Port scan (bounded by each row's `last_frame` byte): aux→row census §5/§6
  (`/tmp/opencode/r9/scan.py`).
* Engine cites: `fifa96_match_run.c:504-548` (mover), `:1018-1044` (arm),
  `:1054-1071` (arc), `:1213-1232` (dispatch order), `:1376-1437` (scene stage),
  `:47-159` (row table), `:173` (frame records);
  `fifa96_action_handlers.c:201-283` (locomotion port);
  `fifa96_entity_update.c:63` (sine), `:187-205` (metric);
  `fifa96_match_locomotion_tables.c` (heading/stride);
  `tests/test_engine_match_frame.c:2352-2379` (arc test), `:2171-2337` (live
  chain); `include/fifa96_engine/fifa96_match_entities.h:122-127` (record
  words).

---

## Status append — T3 implementation (phase-13, 2026-10-11)

Landed (commit `feat(engine,fu161)` plus the fu159 rider that carries the
one-shot model and the snap):

1. **The impulse family.** `match_run_record_impulse(r, shift)` implements the
   case-0x0D/0x29/0x2A store set: `vel73/vel75 = dir[oct] << shift` (the new
   `fifa96_match_dir_x/_z` tables, flat 0x10F334/0x10F33C — first-hand
   `read_memory`), `speed71 = fifa96_entity_distance(vel)`, the dword views
   recomposed. Wired: aux 0x0E (`<<2`, one-shot), 0x2A (row-keyed
   `{0x4D,0x59,0x11,0x5E} -> <<2`, sustained) and 0x2B (the same keying, while
   `timer < 2*base`, sustained). **FU161-L1 closes for the ported arms.**
2. **The frame freeze.** The 0x2B arm stages `frame_freeze` (native
   `word[+0x32] = 0`, `0x6EA03`); `match_run_scene_stage` consumes it (skips
   the advance for that record, timer = 0). **FU161-L2 closes.**
3. **The snap and the grounded step.** `match_run_record_snap` ports
   `FUN_00079B1C` (aux 0x0F/0x05); aux 0x05 is the live chain's stop. Aux
   0x0C ports the grounded target step (`0x6EBFB..0x6EC48`).
   **FU161-L3 re-scoped (premise erratum):** the fresh disassembly shows the
   0x0C arm reads `dword[oct+0x10F331]` / `dword[oct+0x10F339]` with `SAR
   24`, i.e. the **same** 0x10F334/0x10F33C dir bytes through the dword-base
   alias — there is no second direction pair (FU-162 §1's alias-trap
   resolution reproduced). The rotated `0x33` index stays a leg.
4. **The one-shot model** (recommended in §7 step 4) is implemented in the
   fu159 rider: `e->aux_word` (+0x36) re-arms on the (anim_id, frame) resolve
   edge; the ported one-shot arms (0x0E/0x3A) clear their byte, the sustained
   arms keep it. The arc starter proves the native descent (the old per-tick
   re-stage froze the arc at the crest; the T1 unit test's `anim_id = 0` mask
   is gone).

Tests: `test_record_jump_impulsion`, `test_record_jump_sustained_and_freeze`,
`test_record_jump_grounded_step` (unit) and ISO
`test_live_jump_impulsion_carry_and_stop` (axis impulse (4,0)/4 -> the carry
moves the record -> the f5 aux-0x05 snap zeroes speed/vel and sets
target := pos). Goldens M1/M2 byte-exact; `wired` unchanged by this piece.

Carried legs: FU161-L3 (rotated index), FU161-L4 (+0x98/+0x99), FU161-L5
(dead arms 0x25/0x2E), FU161-L6 (row sets); the row-0x12 f3 `0x0002` and f5
`0x0012` arms stay unported sub-cases of the row-0x0E chain.

**Fix round 1 (2026-10-11, commit `3948564`).** The grounded 0x0C step's
target write used `(int32_t)dir[oct] << 5`, which is UB for the table's `-1`
entries; UBSan tripped as soon as a live record faced a negative-direction
octant (exposed by the extended live row-0x0E chain). Fixed to the exact
`* 0x20` product; the live chain now also asserts the successor chain
`0x12 -> 0x0F -> 0x00` + the terminal `+0x44 = 1` live. `make check`
108/108; M1/M2 byte-exact.
