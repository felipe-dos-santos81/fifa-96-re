# FU-167 — row-0x0E stage-2 else arm: the camera-follow / slot-dir arm (FU160-L4)

> Phase-14 recon wave **R15** (read-only), 2026-10-12. Program **`/FIFA96.EXE`**
> (case-sensitive), Ghidra MCP, scripts disabled; LE flat-image addresses;
> globals cited at the flat address. No repo file was written besides this
> slice and `r15-report.md`; no git command was run; the ISO was only read.
>
> **This slice closes FU160-L4** (the row-0x0E body arm `0x82820..0x82A9C`
> left underived when the T3 port landed) with a fresh byte-level derivation
> of the stage-2 else arm, its gates, every cell it writes, the `0x71C94`
> event/bind tail, and the stage-3 interplay — plus the concrete T1 wiring
> recommendation. The alias traps are applied throughout:
> `dword[EBP+0x69] SAR 0x10` is the word at **`+0x6B`** (not `+0x69`),
> `dword[EBP+0x8B] SAR 0x18` is the byte at `+0x8E`, every
> `[EAX+0x10F331/0x10F339] SAR 0x18` is the byte at `0x10F334/0x10F33C+type`,
> and every `CALL` target below was recomputed from its bytes before being
> trusted.

**Answer in one line.** The else arm runs only when the record is **not**
camera-tracked **and** `word[+0x85] == 0` (the plan's "or" is an AND — §2); it
re-places the camera focus onto the record (`[0x15774C]`/`[0x157754]` =
`rec.pos + 0x40*dir(type)`), builds a **follow vector** from the clamped turn
toward the slot-dir target point (`{slot dir_x<<7 | 0x80-RNG, rec.pos.z ± 0x1E0}`)
scaled by a speed/height amplitude (≤ `0x780`), emits the camera event
`FUN_00071C94(rec, vec, camY + (RNG & 0x3F))` — which **binds
`[0x1577CA] = rec`** (the live camera-follow trigger) plus the direction byte
`[0x157815]` from the 3×3 octant table `0x10E169[dir_x + 4*dir_z]` — then
falls through the `CL=1` flag into the advance block: `timer89 = 0`,
stage `2 -> 3`, and the stage-3 gate (`row44 != 0 -> FUN_0007DAB4` reset)
runs **in the same dispatch**. Gates that do not pass leave the record at
stage 2 (retry next frame); both tracked and `+0x85 != 0` skip the arm and
advance. Nothing on the else path writes a record field except the advance;
no slot cell is written.

---

## 1. Scope and method

Question (plan wave R15): derive `0x82820..0x82A9C` byte-level — entry
conditions, every cell written (slot id/dir, camera follow, record fields),
its gates, the stage-3 tail interplay — and rank what a live port needs first.

All reads are fresh this session, program `/FIFA96.EXE`:

* `disassemble_bytes` `0x82710..0x8275C` (the body head + stage dispatch),
  `0x827C0..0x82806` (the stage-1 install tail that falls into the gate),
  `0x82806..0x828EF`, `0x828F0..0x829DF`, `0x829E0..0x82A4F`,
  `0x82A50..0x82ACF` (the whole stage-2 gate + else arm + advance + RET),
  `0x71C94..0x71D2F` + `0x71D30..0x71DF3` (the event/bind tail),
  `0x974F0..0x9752F` (the `0x1F4` sink trampoline).
* `read_memory`: `0x82700` (16 B, the stage table), `0x10F334` (24 B, the
  per-type direction bytes), `0x10E158` (48 B, the `0x10E169` octant table
  neighbourhood incl. its negative-index side), `0x110F1C` (24 B, the
  `FUN_00092820` request-class table).
* `decompile_function`: `0x8DCD4` (metric), `0x8DD70` (`= FUN_000CD474`),
  `0x795A4` (the 16x16 product), `0x71C94` (the event setter/bind),
  `0x92820` (request sink), `0x8F188` (event `0x6C`), `0x651F0` (media
  helper), `0x974F0` + `0x63B20` (the bucket-class sink), `0x7DAB4` (the
  record reset), `0x92AC8` (the RNG advance).
* `get_xrefs_to 0x71C94` = **11** (fresh; the census holds, `0x82A6C`
  included). Raw-byte reconciliation: the fresh `read_memory` windows match
  every quoted instruction byte-for-byte (spot-checked across all chunks).
* Engine/loader source read (read-only, for the mapping and the wiring rec):
  `src/fifa96_engine/fifa96_match_handlers.c` (1242-1330, 860-886),
  `src/fifa96_engine/fifa96_match_run.c` (560-620, 830-920, 974-1045,
  1100-1215, 1255-1365, 1400-1500, 1500-1700, 3115-3200),
  `src/fifa96_loader/fifa96_camera.c` (40-60, 716-920),
  `src/fifa96_loader/fifa96_arm_helpers.c` (38-94),
  `include/fifa96_loader/fifa96_camera.h` (1-110, 280-390),
  `include/fifa96_engine/fifa96_match_run.h` (400-510),
  `include/fifa96_engine/fifa96_match_entities.h` (78, 134, 162-163, 508).

The arm is in the **row-0x0E body** (`0x82710..0x82ACF`, action table
`0x1106E0[0x0E] = 0x82710`, FU-164 §2.1) whose stage table is
`0x82700 = {0x8275D, 0x827B1, 0x82806, 0x82AB8}` (fresh `read_memory`;
FU-160 §2.2 holds). The arm occupies **stage 2**, entered either by the
stage-table jump (`stage == 2`) or by the stage-1 fall-through in the same
dispatch (`0x82800 -> 0x82806`).

---

## 2. Entry conditions — the stage-2 gate (`0x82806..0x8281A`, fresh)

```
0x82806  CMP EBP, dword ptr [0x001577CA]    3b2dca771500
0x8280C  JZ  0x00082AA0                     0f848e020000
0x82812  CMP word ptr [EBP + 0x85], 0x0     6683bd8500000000
0x8281A  JNZ 0x00082AA0                     0f8580020000
0x82820  (the else arm)
```

**The else arm runs only when BOTH hold:**

1. `EBP != [0x1577CA]` — the record is **not** the camera-tracked record
   (FU-152 §2.9; the engine `mr->render.camera.tracked`). `JZ 0x82AA0` on the
   tracked case: skip the arm, advance.
2. `word[EBP+0x85] == 0` — the record has **no live jump state**
   (`jump_base`, FU-153 §2 / `fifa96_match_run.h:417-423`). `JNZ 0x82AA0`:
   mid-jump records skip the arm and advance.

Correction to the plan's R15 wording ("the record is NOT camera-tracked (or
`word[+0x85] == 0`)"): the second condition is an **AND**, not an OR — both
jumps go to the same `0x82AA0` advance block, so the arm body runs only when
both tests fall through. (This matches FU-160 §7's errata reading; the two
`JZ/JNZ` bytes are fresh above.)

The outer body conditions still apply (fresh `0x82710..0x82755`): phase byte
`[0x157A4D] == 2` (else `FUN_0007DAB4` reset at `0x82ABE`), `timer89 +=
(zero-extended) [0x157A64]` (`0x8272E..0x82742`), stage byte
`byte[EBP+0x92] > 3 -> return` (`0x82748/0x8274A`), else the `0x82700`
stage-table jump. The stage-1 tail that can fall into this gate
(`0x827C3..0x82800`, fresh) is the already-ported `FUN_0006E598(self, 0x12,
frame 0)` install (`0x827E2`, bytes `e8b1bdfeff`, target verified `0x6E598`).

**The in-body gates** (fresh `0x82820..0x82843`), checked before any write:

```
0x82820  MOV ESI, dword[0x00157750]      ; camera Y (the [0x157750] cell)
0x82826  XOR CL, CL                      ; the advance flag := 0
0x82828  CMP ESI, 0x70
0x8282B  JGE 0x82A9C                     ; camY >= 0x70 -> tail (no advance)
0x82831  CMP ESI, 0x20
0x82834  JLE 0x82A9C                     ; camY <= 0x20 -> tail (no advance)
0x8283A  MOV EAX, dword[EBP+0x69]        ; ** alias: the dword at +0x69 **
0x8283D  SAR EAX, 0x10                   ; EAX = (int16)word[EBP+0x6B] (lane_x)
0x82840  CMP EAX, 0x60
0x82843  JGE 0x82A9C                     ; lane_x >= 0x60 -> tail (no advance)
```

3. `0x20 < (int16)word[0x157750] < 0x70` — the camera height (Y) must be
   strictly inside `(0x20, 0x70)`; otherwise the arm returns with `CL = 0`
   (stage stays 2, retried next frame).
4. `(int16)word[EBP+0x6B] < 0x60` — the record's **lane_x** metric (the high
   word of the native `lane_dword` at `+0x69`; the engine exposes it as
   `(int16_t)(e->lane >> 16)`, the same expression the chase gate uses at
   `fifa96_match_run.c:906`; `fifa96_match_entities.h:134` `lane = >>16`).
   **Alias trap:** the gate reads the word at `+0x6B`, not `+0x69` —
   `MOV EAX,dword[EBP+0x69]; SAR 0x10` extracts the high half.

Failing any in-body gate is a **retry**: the three `JGE/JLE` targets are
`0x82A9C` (`TEST CL,CL; JZ 0x82AC5`), i.e. return with `CL = 0` and no stage
bump. When all gates pass and the body completes, `CL = 1` (set at
`0x82A95`) and execution falls into `0x82AA0` (the advance).

---

## 3. The else arm, byte-level (`0x82820..0x82A9C`)

### 3.1 Focus placement — camera x/z := rec.pos + 0x40*dir(type)

```
0x82849  MOV EAX, dword[EBP+0x59]        ; rec.pos.x
0x8284C  MOV [0x0015774C], EAX           ; camera triple x
0x82851  MOV EAX, dword[EBP+0x61]        ; rec.pos.z
0x82854  MOV ESI, dword[EBP+0x20]        ; ESI = the record's slot pointer
0x82857  MOV [0x00157754], EAX           ; camera triple z
0x8285C  MOV EAX, dword[EBP+0x8B]        ; alias: byte[EBP+0x8E] after SAR 0x18
0x82862  MOV EBX, dword[0x0015774C]
0x82868  SAR EAX, 0x18                   ; EAX = the record type byte
0x8286B  MOV ECX, dword[0x00157754]
0x82871  MOV EDX, dword[EAX+0x0010F331]  ; dword load; SAR 0x18 -> byte 0x10F334+type
0x82877  MOV EAX, dword[EAX+0x0010F339]  ; dword load; SAR 0x18 -> byte 0x10F33C+type
0x8287D  SAR EDX, 0x18
0x82880  SAR EAX, 0x18
0x82883  SHL EDX, 0x6
0x82886  SHL EAX, 0x6
0x82889  ADD EBX, EDX
0x8288B  ADD ECX, EAX
0x8288D  MOV [0x0015774C], EBX           ; focus.x = rec.pos.x + (int8)dir_x[type]<<6
0x82893  MOV [0x00157754], ECX           ; focus.z = rec.pos.z + (int8)dir_z[type]<<6
```

The two tables (fresh `read_memory 0x10F334`, 24 B):

```
0x10F334: 00 01 01 01 00 FF FF FF   ; dir_x[0..7] = {0, 1, 1, 1, 0, -1, -1, -1}
0x10F33C: 01 01 00 FF FF FF 00 01   ; dir_z[0..7] = {1, 1, 0, -1, -1, -1, 0, 1}
```

The engine already owns both tables twice: `fifa96_match_dir_x/dir_z`
(`fifa96_match_locomotion_tables.c:31-32`) and the `match_kick_dir_x/z` copy
(`fifa96_match_handlers.c:879-886`), and stages the record type as
`r->type` (`fifa96_match_run.c:1297`, the `+0x8E` octant written by the
stage-0 face call). Indexing the native way is *unmasked*
(`0x10F334 + type`); the engine tables are 8 entries, and the mover masks
`type & 7` (`fifa96_match_run.c:1107/1215`). In practice `r->type` is the
face octant 0..7 — keep the mask or extend the table when porting (leg
FU167-L7).

### 3.2 The slot-dir target point (the x half + the RNG fallback)

```
0x82899  TEST ESI, ESI
0x8289B  JZ 0x828BD                       ; no slot -> RNG
0x8289D  MOV BL, byte[ESI+0x20]           ; slot dir_x
0x828A0  MOV EAX, ESI
0x828A2  TEST BL, BL
0x828A4  JNZ 0x828AC                      ; dir_x != 0 -> use it
0x828A6  CMP byte[ESI+0x21], 0x0          ; slot dir_z
0x828AA  JZ 0x828BD                       ; both dir bytes zero -> RNG
0x828AC  MOV EAX, dword[EBP+0x20]
0x828AF  MOV EAX, dword[EAX+0x1D]         ; alias: byte[slot+0x20] after SAR 0x18
0x828B2  SAR EAX, 0x18
0x828B5  SHL EAX, 0x7                     ; dir_x << 7
0x828B8  MOV [ESP], EAX                   ; target x
0x828BB  JMP 0x828D1
0x828BD  CALL 0x00092AC8                  ; rng draw
0x828C2  MOV EDX, 0x80
0x828C7  AND EAX, 0xff
0x828CC  SUB EDX, EAX
0x828CE  MOV [ESP], EDX                   ; target x = 0x80 - (RNG & 0xFF)
0x828D1  MOV EAX, dword[EBP]              ; team
0x828D4  CMP byte[EAX+0x826], 0x0         ; team side byte
0x828DB  JNZ 0x828E4
0x828DD  MOV EAX, 0x1E0                   ; side 0 -> +0x1E0
0x828E2  JMP 0x828E9
0x828E4  MOV EAX, 0xFFFE20               ; side 1 -> -0x1E0
0x828E9  MOV EDX, dword[EBP+0x61]         ; rec.pos.z
0x828EC  LEA EBX, [ESP+0xC]               ; out struct (0x8DCD4 param_3)
0x828F0  ADD EDX, EAX                     ; target z = rec.pos.z +- 0x1E0
0x828F2  MOV EAX, 0x0015774C              ; from = the just-written focus
0x828F7  MOV [ESP+0x8], EDX               ; target z
0x828FB  MOV EDX, ESP
0x828FD  CALL 0x0008DCD4                  ; metric: {distance, dx, dz}
```

Native slot cells read: `rec[+0x20]` (the slot pointer), `slot+0x20`
(dir_x), `slot+0x21` (dir_z) — the same bytes `fifa96_camera.h:49-51` calls
"the tracked record's slot dir bytes". RNG: `FUN_00092AC8` (fresh decompile:
the 5-word 16-bit LCG advance; EAX carries the drawn word —
`fifa96_rng_step(&mr->rng, &draw)`).

**Dead-output note.** `0x8DCD4` writes `param_3[0] = the octagonal distance`,
`param_3[1] = dx`, `param_3[2] = dz` (fresh decompile:
`param_3[1] = *param_2 - *param_1`, `param_3[2] = param_2[4] - param_1[4]`,
then the `max + f(min)` distance). The arm consumes **only dx/dz** (via the
overlapping dword reads at `0x82902/0x82906`); the distance word at `[ESP+0xC]`
is overwritten by the amplitude at `0x829B7` before any read. A port computes
`dx = target_x - focus_x`, `dz = target_z - focus_z` directly and skips the
octagonal metric. (`fifa96_arm_dist_stage` returns only distance + the dz
lane — its `out_lane` is the *native* `out[2]`; the port needs dx explicitly,
which the helper does not expose.)

Consequences of the slot-dir algebra:
* `target_x = dir_x << 7` when `dir_x != 0`, **or** when `dir_x == 0` but
  `dir_z != 0` (falls to `0x828AC` and reads `byte[slot+0x20]` again → `0`);
  RNG only when the slot is absent **or both dir bytes are zero**.
* `target_z = rec.pos.z ± 0x1E0` by **team side** (`team+0x826`), so the
  follow direction is downfield by side — the z term is the dominant one
  (`dz = ±0x1E0 - dir_z[type]<<6`, independent of the record position).

### 3.3 The turn-clamped angle

```
0x82902  MOV EDX, dword[ESP+0xE]          ; alias: word at ESP+0x10 = dz
0x82906  MOV EAX, dword[ESP+0xC]          ; alias: word at ESP+0xE  = dx
0x8290A  SAR EDX, 0x10
0x8290D  SAR EAX, 0x10
0x82910  CALL 0x0008DD70                  ; = FUN_000CD474(dx, dz) -> angle in AX
0x82915  SUB AX, word[EBP+0x7D]           ; angle - facing (face7d)
0x82919  AND AH, 0x3                      ; wrap to 0..0x3FF
0x8291C  MOVSX EDX, AX
0x8291F  CMP EDX, 0x200
0x82925  JLE 0x8292D
0x82927  SUB EAX, 0x400                   ; normalize to (-0x200, 0x200]
0x8292D  MOVSX EDX, AX
0x82930  CMP EDX, 0x100
0x82936  JLE 0x8293F
0x82938  MOV EAX, 0x100                   ; clamp turn to +0x100
0x8293D  JMP 0x8294C
0x8293F  CMP EDX, 0xFFFFFF00
0x82945  JGE 0x8294C
0x82947  MOV EAX, 0xFFFFFF00              ; clamp turn to -0x100
0x8294C  ADD AX, word[EBP+0x7D]           ; re-add facing
0x82950  AND AH, 0x3
0x82953  MOVSX EDX, AX
0x82956  CMP EDX, 0x200
0x8295C  JLE 0x82964
0x8295E  SUB EAX, 0x400                   ; EAX = the target facing (0x3FF space)
```

`0x8DD70` is a fresh-decompiled tail wrapper over `FUN_000CD474` (the
engine's `fifa96_entity_angle` / `fifa96_action_kick_angle`). The result:
`angle_toward_target`, turned relative to the record's facing `face7d`
(`+0x7D`), clamped to a ±`0x100` (90°) turn, re-based to an absolute facing.
`EAX` then holds the **event bearing** used for the vector (0x3FF space).

### 3.4 The amplitude (follow rate)

```
0x82964  MOV BX, word[EBP+0x71]           ; speed71
0x82968  MOV SI, word[0x001577BE]         ; the camera speed cell
0x8296F  ADD EBX, EBX                     ; 2*speed71
0x82971  ADD EBX, ESI                     ; + camera speed
0x82973  MOVSX EDX, BX
0x82976  CMP EDX, 0x1E
0x82979  JLE 0x82980
0x8297B  MOV EBX, 0x1E                    ; clamp <= 0x1E
0x82980  MOV ECX, 0x30
0x82985  SUB ECX, dword[0x00157750]       ; 0x30 - camY
0x8298B  TEST ECX, ECX
0x8298D  JGE 0x82991
0x8298F  NEG ECX                          ; |0x30 - camY|
0x82991  MOV EDX, 0x60
0x82996  SUB EDX, ECX                     ; 0x60 - |0x30 - camY|
0x82998  MOV ECX, EDX
0x8299A  MOVSX EDX, DX
0x8299D  CMP EDX, 0xa
0x829A0  JGE 0x829A7
0x829A2  MOV ECX, 0xa                     ; clamp >= 0xA
0x829A7  IMUL EBX, ECX                    ; rate = clamped_speed_sum * height_factor
0x829AA  MOV EDX, dword[EBP+0x4]          ; the record descriptor pointer
0x829AD  MOVSX DX, byte[EDX+0x15]         ; signed descriptor byte +0x15
0x829B2  SHL EDX, 0x6
0x829B5  ADD EBX, EDX                     ; + desc[0x15]<<6
0x829B7  MOV word[ESP+0xC], BX            ; amplitude
0x829BC  MOV EDX, dword[ESP+0xA]          ; alias: word at ESP+0xC
0x829C0  SAR EDX, 0x10
0x829C3  CMP EDX, 0x780
0x829C9  JLE 0x829D2
0x829CB  MOV word[ESP+0xC], 0x780         ; clamp <= 0x780
```

`amplitude = clamp( clamp(2*speed71 + cam_speed, <= 0x1E) *
clamp(0x60 - |0x30 - camY|, >= 0xA) + (int8)desc[+0x15]<<6 , <= 0x780)`.

* `speed71` = `+0x71` (engine `r->speed71`, staged `match_run.c:1361`).
* `cam_speed` = `word[0x1577BE]` (engine `cam->speed` = the camera walk's
  tail `fifa96_entity_distance(vel_x,vel_z)`; `fifa96_camera.c:146`).
* The height factor peaks at camY = 0x30 and floors at 0xA.
* `desc[+0x15]`: the roster descriptor byte (`rec[+0x4]` pointer; the engine
  models `rec[+4][+0xD]/[+0xE]` as `player_d/player_e`, both staged **0**
  (`match_run.c:1343-1345`) — `+0x15` is unmapped; stand-in 0, leg FU167-L1).

### 3.5 The vector (sine/cosine against the amplitude)

```
0x829D2  MOVSX EBX, AX                    ; EBX = the target facing (saved)
0x829D5  MOV EAX, EBX
0x829D7  SHL AH, 0x7                      ; the 0x114E04 sine-table fold
0x829DA  SBB EDX, EDX                     ; (sign term)
0x829DC  ADD AH, AH
0x829DE  SBB ECX, ECX                     ; (mask term)
0x829E0  XOR EAX, ECX
0x829E2  AND EAX, 0xFF
0x829E7  SUB EAX, ECX                     ; index = angle & 0xFF folded
0x829E9  MOV EAX, dword[EAX*0x4+0x114E04] ; signed sine
0x829F0  XOR EAX, EDX
0x829F2  SUB EAX, EDX
0x829F4  MOV ECX, dword[ESP+0xA]          ; amplitude
0x829F8  SAR ECX, 0x10
0x829FB  MOV EDX, EAX
0x829FD  MOV EAX, ECX
0x829FF  CALL 0x000795A4                  ; (a*b + 0x8000) >> 16
0x82A04  MOV word[ESP+0xE], AX            ; vec x
0x82A09  MOV EAX, EBX                     ; the same fold at angle + 0x100
0x82A0B  LEA ECX, [EAX+0x100]
0x82A11..0x82A2C  (the cosine fold)
0x82A2E  MOV EBX, dword[ESP+0xA]
0x82A32  SAR EBX, 0x10
0x82A35  MOV EDX, EAX
0x82A37  MOV EAX, EBX
0x82A39  CALL 0x000795A4
0x82A43  MOV word[ESP+0x10], AX           ; vec z
```

`vec_x = mul16(amplitude, sin(facing))`, `vec_z = mul16(amplitude,
sin(facing + 0x100))` — the standard `0xCD474` + `0x114E04` + `0x795A4`
idiom, all three ported (`fifa96_entity_sine`,
`fifa96_action_kick_angle`, and the `camera_mul16` product at
`fifa96_camera.c:719` — static, inline it or export).

### 3.6 The calls and the advance (`0x82A48..0x82ACF`)

```
0x82A48  MOV EAX, EBP
0x82A4A  CALL 0x00092820                  ; sink: request kind 0x0B on rec
0x82A4F  CALL 0x00092AC8                  ; rng draw (2nd)
0x82A54  XOR AH, AH
0x82A56  MOV DX, word[0x00157750]         ; camY
0x82A5D  AND AL, 0x3F                     ; draw & 0x3F
0x82A5F  XOR ECX, ECX                     ; the ramp param (0)
0x82A61  ADD EAX, EDX                     ; height = camY + (draw & 0x3F)
0x82A63  LEA EDX, [ESP+0xC]               ; vec = {amplitude, X, Z}
0x82A67  MOVSX EBX, AX                    ; height in BX
0x82A6A  MOV EAX, EBP
0x82A6C  CALL 0x00071C94                  ; the camera event + tracked bind
0x82A71  MOV EBX, 0x40
0x82A76  MOV EAX, 0x6C
0x82A7B  MOV EDX, EBP
0x82A7D  CALL 0x0008F188                  ; queue event 0x6C, rec, 0x40
0x82A82  XOR EAX, EAX
0x82A84  CALL 0x000651F0                  ; media helper (0)
0x82A89  XOR EAX, EAX
0x82A8B  CALL 0x000651F0                  ; media helper (0)
0x82A90  MOV EAX, 0x1F4
0x82A95  MOV CL, 0x1                      ; ** the advance flag **
0x82A97  CALL 0x000974F0                  ; bucket sink: FUN_00063B20(0x1F4>>4)
0x82A9C  TEST CL, CL
0x82A9E  JZ 0x82AC5                       ; flag clear -> return, stage stays 2
0x82AA0  MOV CH, byte[EBP+0x92]           ; advance block
0x82AA6  MOV dword[EBP+0x89], 0x0         ; timer89 = 0
0x82AB0  INC CH
0x82AB2  MOV byte[EBP+0x92], CH           ; stage += 1 (2 -> 3)
0x82AB8  CMP byte[EBP+0x44], 0x0          ; stage-3 tail, same dispatch
0x82ABC  JZ 0x82AC5
0x82ABE  MOV EAX, EBP
0x82AC0  CALL 0x0007DAB4                  ; row44 != 0 -> the record reset
0x82AC5  ADD ESP, 0x14 / pops / RET
```

All CALL targets verified from bytes this session (`e823f2feff` at `0x82A6C`
-> `0x71C94`; `e8d1fd0000` -> `0x92820`; `e806c70000` -> `0x8F188`; the two
`e867/6027feff` -> `0x651F0`; `e8544a0100` -> `0x974F0`; `e8efafffff` ->
`0x7DAB4`; `e8d2b30000` -> `0x8DCD4`; `e85bb40000` -> `0x8DD70`; `e8a06bffff`
/ `e8666bffff` -> `0x795A4`; `e806020100` / `e874000100` -> `0x92AC8`).
`FUN_0007DAB4` (fresh decompile) = the standard record reset: stage `0xFF`,
timer89 = 0, slot release (`FUN_00078B00`), then the phase-2 re-dispatch
(`FUN_0007C990`) or the clear install (`FUN_0007D9A4(rec,0,0,0)`).

**The `CL` advance flag is sound.** `XOR CL,CL` at `0x82826` marks every
early exit; `MOV CL,1` at `0x82A95` marks the completed body; `TEST CL,CL`
at `0x82A9C` selects advance vs return. The sink chain in between
(`FUN_000974F0` = `CWDE / SAR EAX,4 / JMP FUN_00063B20`, fresh) never writes
ECX, and `FUN_00063B20` (fresh decompile: a pure clamp writing only
`[0x1097BC]`) never writes ECX either — so the flag survives at the byte
level. The advance is unconditional on the body completing; the veto inside
`0x71C94` does **not** suppress it.

---

## 4. What `FUN_00071C94` writes (the bind + the slot-dir octant byte)

`0x71C94` was derived in FU-152 §2.9/FU-153 §2; this slice re-read the head
and tail first-hand for the cells this arm feeds. Fresh disassembly
(`0x71C94..0x71D2F`, `0x71D30..0x71DF3`):

```
0x71C94  push/push/push; MOV EBP,EAX            ; param_1 = rec
0x71C99  CMP byte[0x00157A6C],0; JNZ 0x71DEF   ; the pan veto -> return
0x71CA6  PUSH 0; PUSH [0x157754]; PUSH [0x157750]; PUSH [0x15774C]
0x71CC3  CALL 0x000700F4                        ; camera reset at the triple
0x71CC8  MOVSD                                  ; [0x1577B8]d = vec[0],vec[1]
0x71CC9  MOVSW                                  ; [0x1577BC]w = vec[2]
0x71CCB  MOVSX EAX,BX                           ; param_3 = the height (BX)
0x71CD4  MOV word[0x001577F0], BX
0x71CDB  CMP EAX, [0x157750]; JGE -> else [0x1577F0] = [0x157750]
0x71CF3  CMP dword[0x1577EE]>>16, 0x640; if > -> [0x1577F0] = 0x640
0x71D03  TEST EBP,EBP; JZ 0x71D60
0x71D0C  MOV [0x00158777], 0
0x71D1D  MOV [0x1577CE + side*4], EBP           ; per-side tracked cell
0x71D27  MOV [0x001577CA], EBP                  ; ** the tracked bind **
0x71D2D  MOV byte[EBP+0x9B], 0
0x71D34..  [team+0x7A6]+0x7BF = 0; +0x7CB = 0; [team+0x7CB] = 0 unless == rec
0x71D60  MOVSX EAX,CX; XOR DL,DL; CALL 0x00070544  ; the ramp (param ECX = 0)
0x71D6A  MOV byte[0x00157815], DL               ; dir byte default 0
0x71D70  EDI = [EBP+0x20]; if 0 -> 0x71DBA
0x71D7F  DX = byte[slot+0x20]; AX = byte[slot+0x21]
0x71D89  if dir_x != 0 || dir_z != 0:
0x71D93    EBP = dir_z*4
0x71D9E    AL = byte[0x10E169 + dir_x + dir_z*4]  ; the 3x3 octant table
0x71DA7  else if word[0x1577BE] == 0: [0x157815] = 0xFF
0x71DBA  else:
0x71DCB    CD474(word[0x1577C0], word[0x1577C2]) -> ((angle+0x40)&0x3FF)>>7
0x71DDC  MOV [0x00157815], AL
0x71DE1  MOV byte[0x00157822],0; MOV byte[0x00157820],0
```

Fresh `read_memory 0x10E158` (48 B) resolves the table's negative-index side
(the index is the **signed** `dir_x + 4*dir_z`, so cells sit *before*
`0x10E169`):

```
0x10E164  05        ; (dir_x=-1, dir_z=-1) -> 5
0x10E165  04        ; ( 0, -1) -> 4
0x10E166  03        ; (+1, -1) -> 3
0x10E167  FF        ; (+2, -1) unused
0x10E168  06        ; (-1,  0) -> 6
0x10E169  FF        ; ( 0,  0) unreachable (both-zero branches to the camera-speed arm)
0x10E16A  02        ; (+1,  0) -> 2
0x10E16B  FF        ; (+2,  0) unused
0x10E16C  07        ; (-1, +1) -> 7
0x10E16D  00        ; ( 0, +1) -> 0
0x10E16E  01        ; (+1, +1) -> 1
```

The nine values are exactly the octant of `(dir_x, dir_z)` under the
`((angle+0x40)&0x3FF)>>7` convention (`(0,+1)->0`, `(1,1)->1`, `(1,0)->2`,
`(1,-1)->3`, `(0,-1)->4`, `(-1,-1)->5`, `(-1,0)->6`, `(-1,1)->7`) — a 3×3
direction lookup embedded with base `0x10E169` and negative indexing, used
only to fill the camera direction byte `[0x157815]`.

**The event vector layout** (the `MOVSD`/`MOVSW`): the caller's 3-word vector
`{vec0, vec1, vec2}` = `{amplitude, X, Z}` lands as
`[0x1577B8]d = {amplitude, X}`, `[0x1577BC]w = Z`. The engine models the
native step pair as `event_step_x = [0x1577BA] = X` and
`event_step_z = [0x1577BC] = Z` (`fifa96_camera.h:42`), and
`fifa96_camera_event_set(cam, seed_x, seed_z, height, ramp_param)` is the
documented port of the same call shape (`camera.h:280-301`, used by
`0x70C08` with the vector `{distance, v1, v2}` — the same
"word0 unused by the engine" convention). `[0x1577B8]` (the amplitude here)
has no engine consumer — leg FU167-L4.

**Engine cells vs the native writes:** the engine's `fifa96_camera_event_set`
already models the reset, the seed pair, the height clamp `[target y, 0x640]`,
the ramp, and the velocity/bearing (`camera.c:827-880`); it **clears**
`cam->tracked` in the reset and does **not** re-bind. The native re-binds
right after the reset (`0x71D27`). The engine's separate
`fifa96_camera_set_tracked(cam, id)` (`camera.c:56-60`, `[0x1577CA]`) is the
bind; the arm's port must call it after a successful (non-vetoed) event set.

**Cell-write inventory (native -> engine):**

| native cell | value | engine model |
|---|---|---|
| `[0x15774C]`, `[0x157754]` | `rec.pos.x/z + dir[type]<<6` | `cam->pos_x/pos_z` |
| `[0x1577B8]`d, `[0x1577BC]`w | `{amplitude, X}`, `Z` | `event_step_x/z` = X/Z; `[0x1577B8]` leg |
| `[0x1577F0]` | `max(camY + (RNG&0x3F), camY)` capped `0x640` | `cam->event_param` |
| reset cells (`0x1577EE.hi`/`F2`/`F4`/`F8`, rate bytes, target/anchor/vel/timers) | `FUN_000700F4` | `fifa96_camera_event_set` reset |
| `[0x1577CE+side*4]`, `[0x1577CA]` | `rec` (bind) | `cam->tracked` (`set_tracked`) |
| `byte[rec+0x9B]` | `0` | the engine `has_ball` (+0x9B) |
| `[team+0x7A6]+0x7BF`, `[team+0x7A6]+0x7CB`, `[team+0x7CB]` | `0` / `0` / `0` unless `== rec` | team callback cells (FU-153 §2 family) — leg |
| `[0x158777]` | `0` | unmodeled — leg FU167-L5 |
| `[0x157815]` | the `0x10E169` octant (or `0xFF`, or the vel angle) | unmodeled (OL-T11-79) — leg FU167-L2 |
| `[0x157820]`, `[0x157822]` | `0` | unmodeled — leg FU167-L2 |
| `[0x15B651]/[0x15B659]/[0x15B65C..]`, `[0x15B64D+3]` | via `0x92820(rec, 0xB)` (fresh table `0x110F1C[0xB] & 1 = 1` -> the latch runs: prev triple `[0x157758/5C/60]` + a timestamp + the rec) | unmodeled replay latch — leg FU167-L3 |
| `[0x15B36C]`(event `0x6C` code `0x40` -> `1`), the `0x8F188` queue entry | via `0x8F188(0x6C, rec, 0x40)` (gated `[0x14C32A] == 0 && [0x14C312] != 0`) | unmodeled — leg FU167-L3 |
| `[0x1097BC]` | `FUN_00063B20(0x1F4>>4 = 0x1F)`: `0x1F < 0x46` -> bucket class `1` | unmodeled — leg FU167-L3 |
| `[EBP+0x89]`, `byte[EBP+0x92]` | `timer89 = 0`; stage `2 -> 3` | `r->timer89`, `r->stage92` |
| via `FUN_0007DAB4` | the record reset when `row44 != 0` | `match_row_reset` |

**No slot cell is written** by the arm — the slot pointer `rec[+0x20]` and
the dir bytes `slot+0x20/0x21` are read only (twice: once for the target x,
once inside `0x71C94` for `[0x157815]`).

---

## 5. The stage-3 tail interplay

The advance block (`0x82AA0..0x82AB2`) is shared by every completion path:

* `0x8280C JZ` — the record **is** tracked: skip the arm, `timer89 = 0`,
  stage -> 3.
* `0x8281A JNZ` — `jump_base != 0`: skip the arm, advance.
* the else arm's `CL = 1` fall-through: advance after the camera work.

Immediately after the bump, the **stage-3 gate runs in the same dispatch**
(`0x82AB8`: `if (byte[+0x44] != 0) FUN_0007DAB4(rec)`), because the advance
block falls straight through into it. `+0x44` is the current anim row's
flags bit 0 (the terminal flag; FU-159 §3.1/§5.2, engine
`match_run_row_terminal_flag`). So:

* a stage-2 record that completes the else arm binds the camera and reaches
  stage 3 **on the same frame**; the record then holds in stage 3 (each
  further frame re-runs the `row44` gate) until the installed row-`0x12`
  chain reaches a terminal row, at which point the reset frees the record
  and clears the action;
* a stage-2 record whose in-body gates fail **stays at stage 2** and retries
  the arm next frame — the retry window is where the `+0x85` gate matters
  (a retry landing mid-jump must not re-place the camera).

The existing engine port (`fifa96_match_handlers.c:1319-1325`) already
mirrors the fall-through shape (`if (stage92 == 2) { ... } if (stage92 == 3)
{ ... }` — two independent `if`s), so wiring the arm into the stage-2 block
gets the same-dispatch stage-3 evaluation for free.

---

## 6. Engine reachability and the T1 wiring recommendation (ranked)

**Reachability.** The row-0x0E body is ported and wired
(`fifa96_match_action_0E`, `fifa96_match_handlers.c:1274-1330`; dispatch
`{0x0E, ...}` at `:2964`). The `0x0E` install chain is live from three rows
(the row-23 tackle `handlers.c:2203`, row-07 `handlers.c:1008`, row-04
`fifa96_outfield.c:579`, FU-164 §3.1). The stage-2 block today only
implements the tracked/`jump_base` advance; the else arm is the missing
piece, so **the arm is one port away from live** — and its `0x71C94` call is
FU-164's rank-1 pan caller (the fresh xref count 11 holds; the call site is
attributed to row 0x0E arm 2, FU-164 §2.1 correction confirmed fresh by the
stage table and the fresh `0x82806..0x82AB7` window).

**Ranked port scope (what a live port needs first):**

1. **The tracked bind + focus + event (the functional core).** In
   `fifa96_match_action_0E`'s stage-2 block, add the else arm:
   `cam->pos_x = r->pos_x + dir_x[r->type & 7] * 0x40; cam->pos_z = r->pos_z
   + dir_z[r->type & 7] * 0x40;` then the vector + event +
   `if (event_set(...) == FIFA96_OK) fifa96_camera_set_tracked(cam,
   r->entity_id);`. This is the single highest-value effect: the bind flips
   the record into the tracked branch next frame **and engages the
   already-ported camera-walk follow** (`fifa96_match_run.c:3130+`, "requires
   `[0x1577CA] != 0` and `[[rec]+0x20] != 0`"; FU-152 §2.9) — the live
   sustained-follow coupling the phase-14 goal (a) is about. All primitives
   exist (`fifa96_camera_event_set`, `fifa96_camera_set_tracked`,
   `fifa96_action_kick_angle`, `fifa96_entity_sine`, `camera_mul16`).
2. **The event vector + amplitude (the follow rate).** Build `dx = target_x -
   focus_x`, `dz = target_z - focus_z` (skip the `0x8DCD4` distance — it is
   dead, §3.2); `angle = fifa96_action_kick_angle(dx, dz)`; turn-clamp to
   `face7d ± 0x100`; `X = mul16(amplitude, sine(angle))`,
   `Z = mul16(amplitude, sine(angle + 0x100))`; call
   `fifa96_camera_event_set(cam, X, Z, camY + (rng & 0x3F), 0)`. Without
   the vector the camera binds but does not move (the engine `event` state
   drives the pan/velocity).
3. **The gates + retry semantics (the advance flag).** `camY` must be strictly
   inside `(0x20, 0x70)` and `(int16)(r->lane >> 16) < 0x60`; otherwise
   return leaving `stage92 == 2` (the retry — **do not** advance). The
   advance on the full path is unconditional (`timer89 = 0`, stage -> 3),
   including when the event setter vetoes (`[0x157A6C]`) — the veto is not
   an advance gate.
4. **The amplitude algebra (fidelity of the follow strength).** The
   `2*speed71 + cam->speed` clamped-`0x1E` x height-factor clamped-`0xA`
   product, `+ desc[0x15]<<6` (stand-in 0), clamped `0x780`. Can land after
   (1)-(3) with a simple constant if the T1 test only needs the bind+advance,
   but is cheap and fully derived here.
5. **Legs, not blockers:** the direction byte `[0x157815]` (the `0x10E169`
   3×3 table — FU167-L2), the sinks (`0x92820` kind `0xB`, event `0x6C`,
   `0x651F0` x2, `0x974F0`->`[0x1097BC]` — FU167-L3), the `[0x1577B8]`
   vector word (FU167-L4), the descriptor byte (FU167-L1).

**Tests (RED first).** Unit: drive a `0x0E` pool record to stage 2 with
`camY` in-window and `lane_x < 0x60` (not tracked, `jump_base == 0`) ->
assert `cam->pos_x/pos_z` moved, `camera.tracked == entity_id`,
`event_param == camY + (rng & 0x3F)` (with the engine RNG staged), `stage92
== 3`, `timer89 == 0`; a `camY` out-of-window case -> `stage92` stays 2 and
no bind; a tracked case and a `jump_base != 0` case -> advance without the
camera writes; a `row44 != 0` case -> the same-dispatch reset. ISO: extend
`test_live_row_0e_selector_installs_jump` to assert the tracked bind on the
jumper during the installed row-`0x12` window.

**Census / goldens.** No wired-count change (row `0x0E` is already wired) —
no README recompute from FU-167 alone; the T1 half from R17 (row `0x5E`)
carries any census move. M1 never moves; the M2 tape never dispatches `0x0E`
(FU-160 status), so this arm expects no re-pin — re-check byte-exactness
anyway (the T1 gate).

---

## 7. Numbered legs (new)

* **FU167-L1 — the descriptor byte `[[rec+4]+0x15]<<6`.** The record's
  `+0x4` descriptor byte `+0x15` is unmapped (the engine's `player_d/player_e`
  stand-ins cover `+0xD/+0xE` only, staged 0). The port's stand-in is 0;
  the true byte (a roster/pace term adding up to `(byte<<6)` to the follow
  amplitude) needs the descriptor model.
* **FU167-L2 — the camera direction byte `[0x157815]` and the `0x10E169`
  table.** The 3×3 octant table is fully derived here (cells `0x10E164..0x10E16E`,
  base `0x10E169` with negative indexing) but unmodeled in the engine
  (carried by OL-T11-79 together with `[0x157820]`/`[0x157822]`).
* **FU167-L3 — the arm's sink family.** `FUN_00092820(rec, 0x0B)` (the
  replay latch: `[0x15B64D+3] = 0xB`, and because `0x110F1C[0xB] & 1 == 1`
  it snapshots the previous camera triple `[0x157758/5C/60]` into
  `[0x15B651]/[0x15B659]/[0x15B65C..]` plus a timestamp and the rec),
  `FUN_0008F188(0x6C, rec, 0x40)` (queue event `0x6C`, code `0x40`, gated by
  `[0x14C32A] == 0` / `[0x14C312] != 0`), the two `FUN_000651F0(0)` media
  calls, and `FUN_000974F0(0x1F4)` -> `FUN_00063B20(0x1F)` -> bucket class
  `1` in `[0x1097BC]`. None is consumed by the engine today; the bucket
  global is the cheapest to model if a consumer appears.
* **FU167-L4 — the `[0x1577B8]` vector word.** The native stores the 3-word
  vector `{amplitude, X, Z}`; the engine models only the step pair (X/Z).
  The word-0 cell (`0x1577B8`, the "bearing" slot in the camera.h:284
  language) has no engine consumer; here it carries the amplitude.
* **FU167-L5 — `[0x158777] = 0` and the team callback clears.** `0x71C94`
  zeroes `[0x158777]` and the opponent/own-team `+0x7BF`/`+0x7CB` callback
  cells (the FU-153 §2 family). Consumers unsearched in this slice; no
  engine model.
* **FU167-L6 — `word[0x1577BE]` at the action-dispatch point.** The arm
  reads the camera speed cell between the camera walk (`0x736AC`, which
  recomputes it in its tail) and the record walk. The engine carries both
  `cam->speed` and `cam->follow_speed`; this slice maps the arm's read to
  `cam->speed` (the tail value), which is correct only if no other writer
  runs between — the walk ordering proof is FU-152 §2.9's to own.
* **FU167-L7 — the unmasked type index.** The native indexes
  `0x10F334 + type` with no mask; the engine tables are 8 entries (the mover
  masks `& 7`). Safe while `r->type` is the stage-0 face octant; a
  non-octant writer would read past the engine tables (the native reads the
  neighbouring celebration bytes at `0x10F344`).

Carried (unchanged): FU160-L1/L2/L3, FU159-L5/L6, FU164-L1..L8 (the arm's
`0x71C94` site is FU-164's rank-1 caller; this slice supplies its full
requirement list, so FU164-L1 is answered for this site).

---

## 8. Evidence index (fresh this slice)

* `disassemble_bytes`: `0x82710..0x8275C`, `0x827C0..0x82806`,
  `0x82806..0x828EF`, `0x828F0..0x829DF`, `0x829E0..0x82A4F`,
  `0x82A50..0x82ACF`, `0x71C94..0x71D2F`, `0x71D30..0x71DF3`,
  `0x974F0..0x9752F`.
* `read_memory`: `0x82806` (234 B raw-byte reconciliation),
  `0x828F0` (240 B), `0x82700` (16 B), `0x10F334` (24 B), `0x10E158`
  (48 B), `0x110F1C` (24 B).
* `decompile_function`: `0x8DCD4`, `0x8DD70`, `0x795A4`, `0x71C94`,
  `0x92820`, `0x8F188`, `0x651F0`, `0x974F0`, `0x63B20`, `0x7DAB4`,
  `0x92AC8`.
* `get_xrefs_to`: `0x71C94` = **11** (fresh; matches FU-148/FU-156/FU-164).
* Engine/loader cites: `fifa96_match_handlers.c:1274-1330` (the ported
  body; `:1319-1325` the stage-2 block; `:2964` the dispatch row),
  `:879-886` (`match_kick_dir_x/z`), `:2203`/`:1008` (0x0E installs);
  `fifa96_match_run.c:1269/1283/1297/1303/1360-1361/1460-1497` (staging),
  `:906` (`lane >> 16`), `:1107/1215` (`type & 7`), `:3130+` (the tracked
  follow), `:1416` (`fifa96_rng_step`); `fifa96_camera.c:56-60`
  (`set_tracked`), `:146` (`speed` tail), `:719` (`camera_mul16`),
  `:827-880` (`event_set`); `fifa96_camera.h:42` (`event_step_x/z` = the
  native `0x1577BA/BC`), `:280-301` (`event_set` docs), `:298` (the `0x10E169`
  leg); `fifa96_arm_helpers.c:38-94` (`fifa96_arm_dist_stage` — no dx out —
  and `fifa96_arm_face`); `fifa96_match_run.h:417-423` (`jump_base` =
  +0x85), `:438` (`lane`), `:450-455` (`face7d`/`speed71`/`place_offset`),
  `:489-492` (`type`/`actor_type`); `fifa96_match_entities.h:134`
  (`lane = >>16`), `:162-163` (`place_offset`), `:508` (the primitive list);
  `fifa96_match_locomotion_tables.c:31-32` (`fifa96_match_dir_x/z`).
* Prior docs cited (not re-derived): FU-152 §2.9 (the `0x71C94` model),
  FU-153 §2 (`[0x1577CA]` writers; the JUMP-state writers), FU-159
  §3.1/§5.2 (`+0x44`), FU-160 §1.4/§2 (the installer frame arg; stages 0/1),
  FU-164 §2.1/§3.1 (the arm census and the rank-1 attribution).

---

## 9. Tool/read discrepancies

1. **The plan/R15 "or" wording.** The plan says the arm runs when the record
   is not tracked "(or `word[+0x85] == 0`)". The bytes are an **AND** (§2) —
   both `JZ`/`JNZ` share the `0x82AA0` advance target.
2. **The FU-164 §2.1 attribution confirmed fresh.** The stage table read
   `0x82700 = {0x8275D, 0x827B1, 0x82806, 0x82AB8}` puts the `0x71C94` call
   in row-0x0E arm 2 (`+0x92 == 2`); no correction needed beyond FU-164 §6.
3. **`lane` field comment.** `fifa96_match_run.h:438` calls the engine `lane`
   field "native +0x69 dz word, sign-extended"; the actual native object is
   the **dword** at `+0x69` (low = dz, high `+0x6B`), and the engine reads
   its high word as `e->lane >> 16` (`match_run.c:906`) — which is the
   expression this arm's gate needs. Doc-comment imprecision only; no code
   change implied.
4. **The `0x71C94` decompile loses EDI in the `MOVSD`/`MOVSW` pair** (it
   renders as `unique0x00014800 = *param_2`). The disassembly is
   authoritative: `MOV EDI,0x1577B8; CALL 0x700F4; MOVSD; MOVSW` — the seed
   lands at `[0x1577B8]`d and `[0x1577BC]`w. Cite the disassembly, not the
   decompile, for this store.
5. **`FUN_00092AC8`'s declared return.** The decompile is `void`, but the
   caller consumes EAX (`AND EAX,0xFF`) — the function's final state update
   leaves the drawn word in EAX (the 5-word 16-bit LCG advance). The engine
   `fifa96_rng_step` models the draw; no discrepancy in behaviour.

---

## Status

**FU160-L4 closed** (derivation complete; the wiring lands with T1 per the
ranked rec in §6). No repo file changed besides this slice and the workspace
report; no git command run; `/FIFA96.EXE` untouched.

## T1 wiring status (2026-10-12, phase-14 T1 — append-only)

**Landed.** The §6 rank-1/2/3 wiring is in `fifa96_match_handlers.c`
(`match_sp_pan_arm`, called from the `fifa96_match_action_0E` stage-2 block):
the gates, the direct focus write, the lateral seed, the dx/dz + turn clamp,
the amplitude, the polar vector and the split event-set/tracked-bind pair
(plus the record `+0x9B` clear of `0x71D2D`). The rank-4/5 items stay legs as
listed in §7. Two implementation notes:

1. **The lane gate reads the pool `lane_x`, not `r->lane >> 16`.** The two
   are the same native word `+0x6B`, but only `lane_x` is refreshed by the
   BF20 lane block (`fifa96_match_run.c:1444`); the `e->lane` dword's high
   word is a separate stale view (FU168-L7(d)). §9.4's "(or pool `lane_x`)"
   is the option taken. Tested by
   `test_action_0e_stage2_else_arm_gate_retry` (lane_x 0x60 -> retry with
   `lane` untouched).
2. **The event seed pair feeds the ramp divisor** (`camera_event_ramp`'s
   fast path divides by `f8`), so the test asserts the observable
   `event_param`/`event_peak`/focus/bind/advance rather than `event_step_*`
   (the seed-pair equality would need the `f8` inverse).

Tests (all in `tests/test_engine_match_handlers.c`):
`test_action_0e_stage2_else_arm_binds_camera` (the focus, the tracked bind,
`event_param = camY + (draw & 0x3F)`, `event_peak = ramp(event_param)`, the
`+0x9B` clear, `stage 2 -> 3`), `test_action_0e_stage2_else_arm_gate_retry`
(both strict camY bounds, the lane bound, the tracked and `jump_base` skips)
and the ISO extension in `tests/test_engine_match_frame.c`
(`test_live_row_0e_selector_installs_jump`: the live tracked bind + the
same-frame stage-3 advance inside the row-0x12 install window; the mid-band
camera Y is test-staged — FU168-L5 owns the live producer). Goldens
byte-exact (the tape never dispatches 0x0E); `make check` 109/109.
