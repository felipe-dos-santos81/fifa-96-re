# FU-168 — the rank-1 pan arm completed: row-0x0E arm 2 (`0x82A6C`)

Provenance: M2 phase-14 wave **R16** (planned FU-168), 2026-10-12, read-only.
Authoritative program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled;
case-sensitive program argument; standard tools only). No repo file was
modified by this slice; no git commands; `/FIFA96.EXE` untouched. Every
address/byte below is a fresh `disassemble_bytes` / `read_memory` /
`get_xrefs_to` / `get_function_by_address` of this session; engine/loader
source cites are read-only reads. Addresses are LE-image flat addresses (the
FU-148/FU-152/FU-156/FU-164 citation model). The alias traps are applied
(`dword[x]>>16` = word at `x+2`, `>>24` = byte at `x+3`); every load-bearing
aliased read is called out inline.

Question (plan R16): take FU-164 §4's **rank-1** arm — the `0x82A6C` family
(FU-164 §2.1: row **0x0E** arm 2, not row 0D) — and fully derive its
installer, its required inputs, every cell it writes, and the minimal wiring
that makes the pan reachable **live** in open play. Next-ranked arms stay a
ranked fallback list. This slice also owns the FU-164 §3.1 erratum (§9.1).

---

## 1. Scope, status, and the one-paragraph result

* **Status: complete for the arm + installer + inputs + cells + wiring rec.**
  The stage-2 else arm `0x82820..0x82A9E` (called "FU160-L4" by the landed
  phase-13 row-0x0E port) is decoded instruction-level; the pan call
  `0x82A6C` and the whole `FUN_00071C94` event body are fresh; both code-0x0E
  installers are fresh; the engine status is measured against the built tree.
* **Result in one line:** the pan is a **one-shot, mid-camera-height event**
  fired by a code-0x0E record at stage 2 when it is *not* the tracked record
  and has no jump arc; the installer chain is already live engine-side (rows
  04/07/23 all install `0x0E`), so T2 is **one self-contained row-body block
  + the existing event-setter/tracked-bind pair** — no new installer, no new
  record field, no census move; the honest precondition is a **mid-band
  camera Y** (`0x20 < [0x157750] < 0x70`), which the current live chain only
  reaches during a height event (§7.3, leg FU168-L5).
* **Overlap note (R15/FU-167).** FU-167 derives the *same* else block for the
  T1 row-0x0E completion. This slice derives the pan call's contract and the
  wiring; the two must land as one row-body port (one writer per block), and
  FU-168's cells/inputs tables are the pan half of that port.

---

## 2. The arm at byte level (fresh `disassemble_bytes 0x82710..0x82ACF`, 262 insns)

### 2.1 The dispatch context (fresh, unchanged from FU-160 §2)

The row-0x0E body is entered with `EBP = rec`. Fresh arm table
`read_memory 0x82700` (16 B) = `5d 27 08 00 b1 27 08 00 06 28 08 00 b8 2a 08
00` → `{0x8275D, 0x827B1, 0x82806, 0x82AB8}`; the dispatch
`0x82748 CMP AL,3; 0x8274A JA 0x82AC5; 0x82750 AND EAX,0xFF; 0x82755 JMP
CS:[EAX*4+0x82700]`. The pan call `0x82A6C` lies in **arm index 2**
(`0x82806..0x82AB7`), i.e. `byte[rec+0x92] == 2` (the FU-164 §2.1 correction
holds, re-verified).

**Arm-2 head (the stage-2 gate, already ported at `fifa96_match_handlers.c:1319-1325`):**

```
0x82806  3b 2d ca 77 15 00        CMP EBP,[0x1577CA]      ; rec == the tracked record?
0x8280c  0f 84 8e 02 00 00        JZ 0x82AA0              ; yes -> stage advance (no pan)
0x82812  66 83 bd 85 00 00 00 00  CMP word[EBP+0x85],0    ; jump_base (the BF20 arc peak)
0x8281a  0f 85 80 02 00 00        JNZ 0x82AA0             ; arc live -> stage advance (no pan)
```

So the pan arm is entered only when the record is **not** the camera-tracked
record (`[0x1577CA]`) **and** has no live jump arc (`word[+0x85] == 0`).

### 2.2 The else arm `0x82820..0x82A9E` (the pan block; FU160-L4) — full decode

**(a) The three gates — all bail to `0x82A9C` with `CL = 0` (no stage advance):**

```
0x82820  8b 35 50 77 15 00        MOV ESI,[0x157750]      ; dword read (camera Y)
0x82826  30 c9                    XOR CL,CL               ; the "pan ran" flag
0x82828  83 fe 70                 CMP ESI,0x70
0x8282b  0f 8d 6b 02 00 00        JGE 0x82A9C             ; Y >= 0x70 -> skip
0x82831  83 fe 20                 CMP ESI,0x20
0x82834  0f 8e 62 02 00 00        JLE 0x82A9C             ; Y <= 0x20 -> skip
0x8283a  8b 45 69                 MOV EAX,[EBP+0x69]
0x8283d  c1 f8 10                 SAR EAX,0x10            ; = word[rec+0x6B] (the lane)
0x82840  83 f8 60                 CMP EAX,0x60
0x82843  0f 8d 53 02 00 00        JGE 0x82A9C             ; lane >= 0x60 -> skip
```

The window is **strict**: `0x20 < (int32)[0x157750] < 0x70` and
`(int16)word[+0x6B] < 0x60`.

**(b) The camera-focus write (the `[0x15774C]/[0x157754]` cells = the FU-71
camera position, FU-157 §1):**

```
0x82849  8b 45 59                 MOV EAX,[EBP+0x59]      ; rec.pos_x (dword)
0x8284c  a3 4c 77 15 00           MOV [0x15774C],EAX
0x82851  8b 45 61                 MOV EAX,[EBP+0x61]      ; rec.pos_z
0x82857  a3 54 77 15 00           MOV [0x157754],EAX
0x8285c  8b 85 8b 00 00 00        MOV EAX,[EBP+0x8B]
0x82868  c1 f8 18                 SAR EAX,0x18            ; = byte[rec+0x8E] (face octant)
0x82871  8b 90 31 f3 10 00        MOV EDX,[EAX+0x10F331]
0x8287d  c1 fa 18                 SAR EDX,0x18            ; = byte[0x10F334 + type]
0x82883  c1 e2 06                 SHL EDX,0x6             ; * 0x40
0x82889  01 d3                    ADD EBX,EDX             ; [0x15774C] += off_x
0x8288d  89 1d 4c 77 15 00        MOV [0x15774C],EBX
0x82877/86/8b/93  (same for [0x10F339] -> byte[0x10F33C+type] -> [0x157754])
```

So the focus := `rec.pos + 0x40 * (int8)0x10F334/3C[byte[rec+0x8E]]`
(**per-type** offsets, the same flat table FU-162-L3 fixed for the throw-in
cut). Y is **not** written (the camera Y stays the mid-band value).

**(c) The lateral seed (slot dir or RNG):**

```
0x82854  8b 75 20                 MOV ESI,[EBP+0x20]      ; slot pointer
0x82899  85 f6 / 74 20            (slot != 0 && (byte[slot+0x20] != 0 ||
0x828a6  80 7e 21 00                       byte[slot+0x21] != 0))
0x828af  8b 40 1d / c1 f8 18      -> (int8)byte[slot+0x20] (the +3 alias of +0x1D)
0x828b5  c1 e0 07                 -> << 7
0x828b8  89 04 24                 [ESP] = seed                          ; slot path
0x828bd  e8 06 02 01 00           CALL 0x92AC8           ; else: match RNG
0x828c2  ba 80 00 00 00           EDX = 0x80
0x828c7  25 ff 00 00 00           EAX &= 0xFF
0x828cc  29 c2 / 89 14 24         [ESP] = 0x80 - (RNG & 0xFF)           ; no-slot path
```

**(d) The metric and the angle (0x8DCD4 + 0x8DD70):**

```
0x828d1  8b 45 00                 MOV EAX,[EBP]           ; the record's owner object
0x828d4  80 b8 26 08 00 00 00     CMP byte[EAX+0x826],0   ; the side flag
0x828dd  b8 e0 01 00 00           EAX = 0x1E0  (flag 0) / 0x828e4 EAX = -0x1E0 (flag != 0)
0x828e9  8b 55 61                 EDX = [EBP+0x61]        ; rec.pos_z
0x828ec  8d 5c 24 0c              EBX = &[ESP+0xC]        ; the out triple
0x828f0  01 c2                    EDX += EAX              ; pos_z +- 0x1E0
0x828f2  b8 4c 77 15 00           EAX = 0x15774C          ; from = the camera focus
0x828f7  89 54 24 08              [ESP+8] = EDX           ; to.z
0x828fb  89 e2                    EDX = ESP               ; to = {seed, [ESP+4]*, pos_z+-0x1E0}
0x828fd  e8 d2 b3 00 00           CALL 0x8DCD4            ; out = {dist, dx, dz}
```

`0x8DCD4` fresh: `0x8DCEF MOV word[ECX+2],DX` and `0x8DCF3 MOV word[ECX+4],BX`
write **out[1] = dx, out[2] = dz** (the C surface `fifa96_arm_dist_stage`
exposes only distance + dz — leg FU168-L7), and `0x8DD55 MOV word[ECX],AX` the
octagonal distance. It reads only `to+0`/`to+8`, so the `to` y at `[ESP+4]` is
a don't-care (unwritten) — no FU156-L1 analog here.

```
0x82902  8b 54 24 0e / c1 fa 10  EDX = word[ESP+0x10] = dz
0x82906  8b 44 24 0c / c1 f8 10  EAX = word[ESP+0x0E] = dx
0x82910  e8 5b b4 00 00           CALL 0x8DD70            ; = 0xCD474 angle wrapper
0x82915  66 2b 45 7d              AX -= word[EBP+0x7D]    ; minus the record facing
0x82919  80 e4 03                 AH &= 3                 ; mod 0x400
0x8291f/30/3f  (wrap > 0x200; clamp to [-0x100,0x100])
0x8294c  66 03 45 7d              AX += word[EBP+0x7D]    ; re-add the facing
0x82950  80 e4 03                 AH &= 3
0x82956  ...                      (wrap > 0x200 once more)
```

`0x8DD70` fresh = `MOVSX EDX,DX; PUSH EDX; CWDE; PUSH EAX; CALL 0xCD474` —
the ported `fifa96_action_kick_angle(dx, dz, &angle)`.

**(e) The polar magnitude (the event triple word 0):**

```
0x82964  66 8b 5d 71              BX = word[EBP+0x71]     ; speed71
0x82968  66 8b 35 be 77 15 00     SI = word[0x1577BE]     ; the camera bearing
0x8296f/71/73/76                 EBX = 2*speed71 + bearing (32-bit adds); the
                                 clamp compares the low word: (int16)BX > 0x1E
                                 -> EBX = 0x1E (the unclamped path keeps the
                                 32-bit sum)
0x82985  2b 0d 50 77 15 00        ECX = 0x30 - [0x157750] ; 0x30 - camera Y
0x8298f  f7 d9                    (NEG -> |..|)
0x82991/96/9a/9d/a2              ECX = max(0x60 - |0x30 - Y|, 0xA)
0x829a7  0f af d9                 EBX *= ECX
0x829aa  8b 55 04                 EDX = [EBP+4]           ; the descriptor pointer
0x829ad  66 0f be 52 15           EDX = MOVSX byte[EDX+0x15]   ; [[rec+4]+0x15]
0x829b2  c1 e2 06 / 01 d3         EBX += (that byte) << 6
0x829b7  66 89 5c 24 0c           word[ESP+0xC] = BX      ; triple word 0 = the magnitude
0x829bc  8b 54 24 0a / c1 fa 10  EDX = word[ESP+0xC] (the +2 alias)
0x829c9/ cb                       clamp > 0x780 -> 0x780
```

**(f) The polar vector (triple words 1/2 = the `0x71C94` seed pair):**

```
0x829d2  0f bf d8                 EBX = MOVSX AX          ; the facing-adjusted angle
0x829d7..0x829f2  (the 0x114E04 fold: idx = (angle&0xFF) ^ -bit8, += bit8, bit9 neg)
0x829ff  e8 a0 6b ff ff           CALL 0x795A4            ; (mag * sine(angle) + 0x8000)>>16
0x82a04  66 89 44 24 0e           word[ESP+0xE] = AX      ; triple word 1
0x82a09  8d 88 00 01 00 00        ECX = angle + 0x100     ; (0x82a11..0x82a2c fold again)
0x82a39  e8 66 6b ff ff           CALL 0x795A4            ; mag * sine(angle + 0x100)
0x82a43  66 89 44 24 10           word[ESP+0x10] = AX     ; triple word 2
```

**(g) The sink request, the RNG height, and the event call:**

```
0x82a3e  ba 0b 00 00 00           EDX = 0x0B
0x82a48  89 e8                    EAX = EBP
0x82a4a  e8 d1 fd 00 00           CALL 0x92820            ; sink (rec, 0x0B)  [OL-27 family]
0x82a4f  e8 74 00 01 00           CALL 0x92AC8            ; match RNG (fifa96_rng_step)
0x82a54  30 e4                    XOR AH,AH
0x82a56  66 8b 15 50 77 15 00     DX = word[0x157750]     ; the camera-Y low word
0x82a5d  24 3f                    AL &= 0x3F
0x82a5f  31 c9                    ECX = 0                ; FUN_00070544 ramp param 0
0x82a61  01 d0                    EAX += EDX
0x82a63  8d 54 24 0c              EDX = &[ESP+0xC]        ; the triple
0x82a67  0f bf d8                 EBX = MOVSX AX          ; height = (RNG & 0x3F) + camera Y
0x82a6a  89 e8                    EAX = EBP               ; rec
0x82a6c  e8 23 f2 fe ff           CALL 0x71C94            ; THE PAN EVENT
0x82a71  bb 40 00 00 00 / b8 6c 00 00 00 / 89 ea
0x82a7d  e8 06 c7 00 00           CALL 0x8F188            ; sink (0x6C, 0x40, rec)
0x82a84/0x82a8b  e8 67 27 fe ff  CALL 0x651F0 x2         ; sinks (EAX=0)
0x82a90  b8 f4 01 00 00           EAX = 0x1F4
0x82a95  b1 01                    CL = 1                  ; "the pan ran"
0x82a97  e8 54 4a 01 00           CALL 0x974F0            ; announce sink (0x1F4)
0x82a9c  84 c9 / 74 25            TEST CL,CL / JZ return
0x82aa0  ...                      CH = byte[EBP+0x92]; [EBP+0x89] = 0; CH++;
0x82ab2                           byte[EBP+0x92] = CH     ; stage -> 3
```

**Timing shape:** the pan fires at most **once per code-0x0E activation** (the
event advances stage 2 -> 3), and only on a frame where the record is not yet
tracked and has no jump arc. If the gates fail, the record **stays at stage 2
and retries** every frame (the `JZ return` leaves `CL = 0`).

---

## 3. The event contract — `FUN_00071C94` fresh (`0x71C94..0x71DF2`, 81 insns)

Fresh `get_xrefs_to 0x71C94` = **11**, the exact set of FU-148 §13.4 /
FU-156 §2 / FU-164 §2.1: `0x71B8A`, `0x70DD1`, `0x7A219`, `0x6FA62`,
`0x75DC4`, `0x77423`, `0x7F4E7`, `0x82A6C`, `0x77DC6`, `0x7EFCF`, `0x7F0D1`
(five wired + this slice's six; `0x82A6C` is the row-0x0E site).

Body, in order (fresh bytes; the ported counterpart is
`fifa96_camera_event_set`, `fifa96_camera.c:827`, documented in
`fifa96_camera.h:280-301`):

```
0x71c99  CMP byte[0x157A6C],0 / 0x71ca0 JNZ 0x71def      ; the suspension bail
0x71ca6..0x71cc3  PUSH 0; PUSH [0x157754]; PUSH [0x157750]; PUSH [0x15774c];
                  EDI=0x1577B8; CALL 0x700F4              ; the place reset (focus triple, flag 0)
0x71cc8/0x71cc9  MOVSD+MOVSW: [0x1577B8..0x1577BD] = the caller 6-byte triple
0x71cd4  word[0x1577F0] = BX (the height arg)
0x71cdb..0x71cf8  if height < word[0x157750] -> height = word[0x157750]; clamp 0x640
0x71d03  TEST EBP,EBP / JZ 0x71d60                       ; NULL rec -> skip the bind block
0x71d07..0x71d1d  dword[0x1577CE + byte[[rec]+0x826]*4] = rec   ; the per-side table
0x71d27  dword[0x1577CA] = rec                           ; THE TRACKED BIND
0x71d2d  byte[rec+0x9B] = 0
0x71d34/0x71d49/0x71d5a  dword[[[rec]+0x7A6] + 0x7BF/0x7CB] = 0
0x71d60  EAX = MOVSX CX (the ramp param); XOR DL,DL; CALL 0x70544
0x71d6a  byte[0x157815] = DL
0x71d70..0x71ddc  [0x157815] = the slot-dir table byte / 0xFF / ((angle+0x40)&0x3FF)>>7
0x71de1..0x71de9  byte[0x157822] = 0; byte[0x157820] = 0
```

`0x700F4` (fresh-cited via FU-157 §1, not re-derived here) resets the whole
event block; `0x70544` is the ported `camera_event_ramp` (`camera.c:728`).

**Call-argument mapping for the engine.** Native `0x71C94(EAX=rec, EDX=triple,
EBX=height, ECX=ramp_param)`; the engine's public pair is
`fifa96_camera_event_set(cam, seed_x, seed_z, height, ramp_param)` +
`fifa96_camera_set_tracked(cam, id)` (the bind is *inside* the native body but
split out in the port — the exact convention already used by the wired row-04
arm at `fifa96_match_handlers.c:2540-2546` and the receive arm at
`fifa96_match_run.c:3186-3191`). For this arm:
`seed_x = word[1]`, `seed_z = word[2]`, `height = (RNG & 0x3F) + camera Y`,
`ramp_param = 0`, bind id = `r->entity_id`. Triple word 0 (the magnitude) is
stored at `0x1577B8` but consumed by nothing in `0x70544`'s fast path — the
engine drops it (leg FU168-L2, carried by `fifa96_camera.h:296-299`).

---

## 4. The installer (fresh)

### 4.1 Native

Both installers are the same five-instruction tail (`code 0x0E`, **stage
argument 0**, invoke-now `ECX = 1`):

```
0x82F62  b9 01 00 00 00   MOV ECX,1        ; FUN_00082DD0 tail (row-23 body family)
0x82F67  ba 0e 00 00 00   MOV EDX,0x0E
0x82F6c  89 f0            MOV EAX,ESI      ; rec
0x82F6e  31 db            XOR EBX,EBX      ; stage arg 0
0x82F70  e8 2f aa ff ff   CALL 0x7D9A4

0x7E7A4  b9 01 00 00 00   MOV ECX,1        ; FUN_0007E600 tail
0x7E7A9  ba 0e 00 00 00   MOV EDX,0x0E
0x7E7AE  89 f0            MOV EAX,ESI
0x7E7B0  31 db            XOR EBX,EBX
0x7E7B2  e8 ed f1 ff ff   CALL 0x7D9A4
```

Fresh caller censuses (this session):
* `get_xrefs_to 0x82DD0` = **1**: `0x8307D` — inside the row-23 tackle machine
  `0x82F84..0x83163` (the FU-164 §2.1 site, re-verified).
* `get_xrefs_to 0x7E600` = **3**: `0x7EC89` (the row-04 defender arm the engine
  ports as `row04_defender_decision`), `0x8160F` (inside the row-07 body
  `0x814B0..0x81737`, gated by a computed word `== 0x40`), and `0x7D0F7` (an
  input-adjacent helper: gates `EAX & 1`, `byte[rec+0x99] == 0`, calls
  `0x7E600`, then returns `byte[rec+0x91] == 0x0E`).

The dispatcher `FUN_0007D9A4` fresh (`0x7D9A4..0x7DAB3`, 74 insns) writes:
`byte[rec+0x91] = code` (`0x7DA67`), `dword[rec+0x89] = 0` (`0x7DA7E`),
`byte[rec+0x9E] = 0` (`0x7DA88`), `dword[rec+0x18] = 0x1106E0[code]`
(`0x7DA8F`), `byte[rec+0x92] = BL` (the stage arg, `0x7DA95`),
`word[rec+0x7B] = word[rec+0x79]` (`0x7DA9F/0x7DAA3`), the `+0x9F` bit-0 set
for code 5/0x21 else clear (`0x7DA53/0x7DA5C`), the optional `0x6E598(rec,
0x0F, byte[+0x8E])` + `byte[+0x98] = 0` (`0x7DA08..0x7DA1F`), then the
invoke-now `CALL [rec+0x18]` when `ECX != 0` (`0x7DAAA`).

### 4.2 Engine — already live, no installer work for T2

* Row 23 tackle: `out.install_0e -> mr->record.install = 0x0E`
  (`fifa96_match_handlers.c:2312`; the tackle step's `install_0e` gate is
  pinned by `tests/test_action_possession.c:326`).
* Row 07: `out->defender_install -> r->install = 0x0E`
  (`fifa96_match_handlers.c:1011`).
* Row 04: `row04_add_install(out, FIFA96_OUTFIELD_ROW04_INSTALL_SELF, 0x0E, 0,
  1)` (`fifa96_outfield.c:577-580`) drained by the wired row-04 loop
  (`fifa96_match_handlers.c:2555-2567`).
* Tests can force it through the same pool seam as the landed live test:
  `fifa96_match_entities_install(rec, phase, 0x0E, 0)`
  (`tests/test_engine_match_frame.c:2633`).

The engine models the dispatcher's effect as `record.install` + the pool
installer; the row body's `stage92` starts at 0 (native `[rec+0x92] = BL = 0`).

---

## 5. Required inputs (arm + event)

| input | native (fresh) | engine field | notes |
|---|---|---|---|
| action code | `byte[rec+0x91] == 0x0E` (dispatch) | `r->code` / the pool `code` | installer chain live (§4.2) |
| phase | `byte[0x157A4A]>>24 == 2` (`0x8271B`) | `mr->state.phase == 2` | ported head |
| stage | `byte[rec+0x92] == 2` (`0x82755` table) | `r->stage92` | ported |
| not tracked | `rec != [0x1577CA]` (`0x82806`) | `camera.tracked != r->entity_id` | ported gate |
| no jump arc | `word[rec+0x85] == 0` (`0x82812`) | `r->jump_base == 0` | ported gate |
| camera Y band | `0x20 < (int32)[0x157750] < 0x70` (`0x82820..0x82834`) | `camera.pos_y` | **the live precondition** (§7.3) |
| lane | `(int16)word[rec+0x6B] < 0x60` (`0x8283a`) | `(int16_t)(r->lane >> 16)` / pool `lane_x` | fresh SAR-0x10 load; see FU168-L7 note (d) |
| record pos | `dword[rec+0x59]`, `dword[rec+0x61]` | `r->pos_x`, `r->pos_z` | the focus write |
| face octant | `byte[rec+0x8E]` (`0x8285c` via `+0x8B>>24`) | `r->type` (live, FU-162-L3) | indexes the dir tables |
| per-type offsets | `(int8)byte[0x10F334/0x10F33C + type] << 6` | `fifa96_match_dir_x/_z[type & 7]` (`fifa96_match_locomotion_tables.c:31-32`) | the same 0x10F334/3C bytes |
| slot | `dword[rec+0x20]`, `byte[slot+0x20]`, `byte[slot+0x21]` | `r->has_slot` + pool `dir_x`/`dir_z` | the lateral seed / the 0x157815 byte |
| side flag | `byte[[rec]+0x826]` (`0x828d4`) | `r->side` (pool team side) | selects `pos_z ± 0x1E0` |
| facing | `word[rec+0x7D]` (`0x82915/0x8294c`) | `r->face7d` | the angle add/subtract |
| speed | `word[rec+0x71]` (`0x82964`) | `r->speed71` | the magnitude term |
| camera bearing | `word[0x1577BE]` (`0x82968`) | `cam->speed` | the magnitude term |
| descriptor byte | `byte[[rec+4]+0x15]` (`0x829aa`) | **unmodeled** (stages 0) | leg FU168-L1 |
| RNG | `0x92AC8` x2 (`0x828bd`/`0x82a4f`) | `mr->rng` (`fifa96_rng_step`) | lateral + height |
| event triple | `{mag, mul16, mul16}` at `[ESP+0xC..0x11]` | `seed_x/seed_z` args | word 0 dropped (leg) |
| ramp param | `ECX = 0` (`0x82a5f`) | `ramp_param = 0` | the `F2 - ramp(...)` sign |

---

## 6. Every cell it writes

### 6.1 The arm (fresh)

| addr | write | engine |
|---|---|---|
| `0x8284c/0x8288d` | `dword[0x15774C] = pos_x + 0x40*0x10F334[type]` | `camera.pos_x` (FU-71 mapping; the `record.place_x` request is the alternative, §7.2) |
| `0x82857/0x82893` | `dword[0x157754] = pos_z + 0x40*0x10F33C[type]` | `camera.pos_z` |
| `0x82a4a` | sink `0x92820(rec, 0x0B)` | no consumer (OL-27 request family) |
| `0x82a6c` | the `0x71C94` event (§6.2) | `event_set` + `set_tracked` (partial) |
| `0x82a7d` | sink `0x8F188(0x6C, 0x40, rec)` | no consumer (`camera.h:331-334` legs) |
| `0x82a84/0x82a8b` | sink `0x651F0(0)` x2 | no-op candidate |
| `0x82a97` | announce `0x974F0(0x1F4)`, `CL = 1` | no-op candidate |
| `0x82aa6` | `dword[rec+0x89] = 0` | `r->timer89 = 0` |
| `0x82ab2` | `byte[rec+0x92]++` (-> 3) | `r->stage92 = 3` |

Locals only: `[ESP]` (the lateral seed), `[ESP+8]` (to.z),
`[ESP+0xC..0x11]` (the event triple), `[ESP+4]` (unread). **No record
pos/target/velocity writes.**

### 6.2 The `0x71C94` event (fresh; the engine status column is against the
ported `fifa96_camera_event_set`, `camera.c:827-880`)

| addr | write | engine |
|---|---|---|
| `0x71ca6..0x71cc3` | the `0x700F4(focus, flag 0)` reset: event state + the four anchors `0x157764/157770/157788/157794` + `0x157821` (FU-157 §1) | the setter's reset subset (anchors from `pos_*`) |
| `0x71cc8/0x71cc9` | `[0x1577B8..0x1577BD]` = {mag, seed_x, seed_z} | only the seed pair kept (leg FU168-L2) |
| `0x71cd4/0x71ce5` | `word[0x1577F0]` = clamp(max(height, camera Y), 0x640) | `cam->event_param` |
| `0x71d0c` | `dword[0x158777] = 0` | unmodeled |
| `0x71d1d` | `dword[0x1577CE + side*4] = rec` | unmodeled (the per-side tracked table) |
| `0x71d27` | `dword[0x1577CA] = rec` | `fifa96_camera_set_tracked` (split out) |
| `0x71d2d` | `byte[rec+0x9B] = 0` | unmodeled by the setter (the pool `has_ball` is not cleared) |
| `0x71d34/0x71d49/0x71d5a` | `[[rec]+0x7A6]+0x7BF/0x7CB = 0` | unmodeled |
| `0x71d6a..0x71ddc` | `byte[0x157815]` = slot table / 0xFF / `((angle+0x40)&0x3FF)>>7` | the rate byte is an argument of `fifa96_camera_rate_event`/`rate_table`, not stored |
| `0x71de3/0x71de9` | `byte[0x157822] = 0`, `byte[0x157820] = 0` | `cam->paused` mapped but not cleared by the setter |
| inside `0x70544` | timer/limit/event_peak/ramp_divisor/vel/anchors/rate bytes | ported (`camera_event_ramp`) |

---

## 7. Engine status and the concrete T2 wiring recommendation

### 7.1 Status

`fifa96_match_action_0E` (`fifa96_match_handlers.c:1274-1330`) ports the head,
arms 0/1, the stage-2 tracked/jump gate and arm 3; the stage-2 else block is a
comment (`handlers.c:1319-1325`: "else: the `0x82820..0x82A9C`
camera-follow/slot-dir arm (FU160-L4)"). Row `0x0E` is **wired** (22/80,
`ENGINE.md:266-274`); the M2 tape never dispatches it
(`tests/test_engine_m2.c:2668`) and the tackle is not key-drivable
(`ENGINE.md:1890-1895`). All helper primitives exist:
`fifa96_arm_dist_stage` (`0x8DCD4`), `fifa96_action_kick_angle` (`0x8DD70` →
`0xCD474`), `fifa96_entity_sine` (the `0x114E04` fold), a `0x795A4` mul16
idiom (three private copies: `camera.c:719`, `outfield.c:720`,
`ball_pairing.c:224`), `fifa96_camera_event_set`, `fifa96_camera_set_tracked`,
`fifa96_rng_step` (`0x92AC8`, `fifa96_rng.c:2`).

### 7.2 Recommendation (T2) — one row-body block + the existing event pair

In `fifa96_match_action_0E`, replace the stage-2 else comment with the block
below (native addresses = the citation; every helper named above; **no** new
record field, **no** installer change):

1. **Gates** (`0x82820..0x82843`): `int32_t cam_y =
   mr->render.camera.pos_y;` then `0x20 < cam_y && cam_y < 0x70` and
   `(int16_t)(r->lane >> 16) < 0x60`. On failure: `return` (stay stage 2,
   retry semantics preserved).
2. **Focus write** (`0x82849..0x82893`): `mr->render.camera.pos_x = r->pos_x +
   0x40 * fifa96_match_dir_x[r->type & 7u]; pos_z` likewise with `_z`. *Do not*
   route this through `record.place_*`: the place drain
   (`match_run_entity_drain`, `fifa96_match_run.c:1548-1550`) runs later the
   same frame (`:3534`) and calls `fifa96_camera_init`, which clears
   `tracked` (`camera.c:50`) — it would destroy the bind set below. The direct
   write is the native's own `[0x15774C]/[0x157754]` store and is what
   `fifa96_camera_event_set` anchors at.
3. **Lateral seed** (`0x82899..0x828ce`): slot path `e->dir_x << 7` when
   `r->has_slot` and `(e->dir_x != 0 || e->dir_z != 0)`; else one
   `fifa96_rng_step(&mr->rng, &v)` draw → `0x80 - (v & 0xFF)`.
4. **Metric** (`0x828d1..0x82910`): from = the focus triple; to = {seed, x,
   `pos_z ± 0x1E0`} (`r->side` selects the sign); compute `distance` and the
   **dx/dz pair** (inline `(int16_t)(to.x - from.x)` × `(int16_t)(to.z -
   from.z)` or extend the helper — leg FU168-L7), then
   `fifa96_action_kick_angle(dx, dz, &angle)`.
5. **Angle algebra** (`0x82915..0x82956`): `angle -= r->face7d; angle &= 0x3FF;
   wrap to ±0x200; clamp to ±0x100; angle += r->face7d; angle &= 0x3FF; wrap
   > 0x200`.
6. **Magnitude** (`0x82964..0x829cb`): `mag = (clamped 2*r->speed71 +
   (int16)cam->speed) * max(0x60 - |0x30 - cam_y|, 0xA) + (desc byte term << 6)`
   clamped to `0x780` (the clamp/paths per §2.2(e); the descriptor term is 0
   live — leg FU168-L1).
7. **Polar vector** (`0x829d2..0x82a43`): `seed_x = mul16(mag,
   fifa96_entity_sine(angle)); seed_z = mul16(mag, fifa96_entity_sine(angle +
   0x100))` (mul16 = a local static of the three existing copies).
8. **Event + bind** (`0x82a4a..0x82a6c`): report the `0x92820(rec,0x0B)` sink
   request; one `fifa96_rng_step` draw; then
   `ev = fifa96_camera_event_set(&mr->render.camera, seed_x, seed_z,
   (int16_t)((v & 0x3F) + (int16_t)cam_y), 0);` and, exactly like the row-04
   arm (`handlers.c:2540-2546`) and the receive arm (`match_run.c:3186-3191`):
   `if (ev < 0) return ev; if (ev == FIFA96_OK)
   (void)fifa96_camera_set_tracked(&mr->render.camera, r->entity_id);`
   On the `ev == 1` suspension bail the bind is skipped but the arm
   continues (native: `0x71C94` returns, the caller's sinks/stage advance
   still run).
9. **Sinks + advance** (`0x82a71..0x82ab2`): the `0x8F188(0x6C,0x40,rec)` /
   `0x651F0` x2 / `0x974F0(0x1F4)` sinks as documented no-op/request
   entries; then `r->timer89 = 0; r->stage92 = 3;` **unconditionally once the
   pan block ran** (the native `CL = 1` is set on the pan path even when the
   event bails).

**Red/GREEN tests.**
* RED (unit, no ISO): stage a record with `stage92 = 2`, `jump_base = 0`,
  `lane` high word < 0x60, not tracked, `camera.pos_y = 0x40`; call the
  dispatch; assert `camera.event_param != 0`, `camera.tracked ==
  r->entity_id`, `r->stage92 == 3`, `r->timer89 == 0`. Mirror case:
  `camera.pos_y = 0` (or lane `0x60`) → no event, not tracked, stage stays 2.
* GREEN (ISO, live): extend `test_live_row_0e_selector_installs_jump`
  (`tests/test_engine_match_frame.c:2603`): after the install, ensure the
  camera Y is mid-band across the stage-1->2 edge (fire a height-carrying
  event first, or stage `camera.pos_y` in the test — see leg FU168-L4/L5) and
  assert the tracked bind + `event_param != 0` on the stage-2 frame; the
  following frames then run `fifa96_match_run_camera_follow`
  (`match_run.c:3505` → `:3128-3146`) — the sustained coupling is R19/FU-171's
  subject.
* **Gate expectations:** row `0x0E` is already wired → **no census/README
  move**; T2 is live-only → **no tape re-pin** (T3 owns the extension; M1
  immovable).

### 7.3 Reachable live in open play — the honest chain

1. **Record:** a tackle/defender contact installs code `0x0E` (rows 23/04/07,
   all wired — §4.2). No new installer.
2. **Stages 0/1:** already ported; stage 1 waits `timer89 >= 0x0C` and installs
   anim row `0x12` (FU-160 §2), whose f2 aux-0x0E frame arms the BF20 jump
   (`jump_base != 0`).
3. **The pan window:** the stage-2 else arm is entered on the frame(s)
   *between* the anim install and the f2 jump arming — the engine runs the
   scene-stage successor after the entity dispatch in the same frame
   (`match_run.c:3531` then `:3576`), so the native-faithful window is ~1-2
   granted frames (leg FU168-L4). The camera Y must be mid-band **then**.
   Today the live camera Y is 0 except during height events (the claim pan's
   event height is 0, `handlers.c:2541`), so a live pan needs a mid-band Y
   producer in that window — the wired receive arm's `traj` height
   (`match_run.c:3187`) or the FU-157-flagged throw-in `[0x157750] = 0x80`
   cut. This is the honest T2 limit (leg FU168-L5, FU-157 §5 overlap).
4. **After the pan:** the tracked bind persists; the every-frame follow
   (`fifa96_match_run_camera_follow` requires `tracked >= 0 && e->has_slot`,
   `match_run.c:3138-3146`) drives `fifa96_camera_rate_event` — the sustained
   follower.

---

## 8. Ranked fallback list (FU-164 §4, re-ranked; one line each)

1. `0x82A6C` — **this slice** (installed by rows 04/07/23; wires with the
   row-0x0E arm; medium cost / high value).
2. `0x7F4E7` — row-05 stage-0 tail (OL-63): needs the `0x7F3A1..0x7F57B` tail +
   `FUN_0007F7E0` ported and `out.tail` consumed; medium/medium.
3. `0x6FA62` — `FUN_0006E8E8` aux-0x22 arm: cheap into
   `match_run_record_jump_arm`, **blocked** by the missing aux-0x22 frame
   producer (FU164-L2 / FU156-L2).
4. `0x71B8A` — `FUN_0007131C` angle arm: medium; needs the prev-triple capture
   and a camera beyond ±0x720/±0xB10 (fidelity, once the follow makes it
   reachable).
5. `0x77DC6` — row-1C common tail: deep (the keeper chain rows 0x1F/1B/1C +
   `FUN_00076130`/`FUN_00077EAC`).
6. `0x77423` — row-1B stage-2 rush: deep (shares rank 5's upstream plus the
   `0x76FC7..0x77636` rush predicates).

---

## 9. Discrepancies / errata

1. **FU-164 §3.1 erratum (helper names).** Quote: "the polar vector staging
   `0x82806..0x82A50` (the `0x114E04` fold + `0x795A4` rotate — both
   primitives already ported) and the `0x8DC68` metric". **Corrected (fresh):**
   the pan block calls `0x8DCD4` (the {distance, dx, dz} out triple,
   `0x828fd`) and `0x8DD70` (the `0xCD474` angle wrapper, `0x82910`) —
   `0x8DC68` is **not** called anywhere in `0x82820..0x82A9E`; the two
   `0x795A4` calls are the product pair (`camera_mul16`), not a "rotate".
2. **FU-164 §3.1 item 3 erratum (cells).** Quote: "No new globals/cells: the
   arm reads only `[0x157750]` and the match RNG." **Corrected (fresh):** the
   else arm also reads `[0x1577BE]` (the bearing, `0x82968`), the focus cells
   it writes (`0x15774C/54`), and the record cells in §5. The gist holds —
   no cell outside the FU-71 camera set + the record — but the "reads only"
   wording overstates.
3. **FU-156 §3.6 attribution** (row 0D arm 3): carried; the FU-164 §2.1
   correction stands (fresh: row 0D RETs at `0x826FE`; the call is row-0x0E
   arm 2).
4. **Engine lane-word reads (pre-existing, out of scope to change).**
   `handlers.c:926/2215/2289` read `(int16_t)r->lane` with a `word[+0x6B]`
   comment, while the dword layout (FU-142 table: `dword[+0x69]` = low dz,
   high `+0x6B`; `fifa96_match_entities.c:319-333`) and the other ported sites
   (`handlers.c:238/1511/1711/1861/2007`) use `(int16_t)(r->lane >> 16)`. The
   pan gate must use the `>> 16` form (or pool `lane_x`); the three other
   sites are a candidate inconsistency to re-audit (leg FU168-L7 note (d)).
5. **`fifa96_arm_dist_stage` drops dx.** The C helper returns distance + dz
   only (`fifa96_arm_helpers.c:38-71`), but the native out triple carries
   `out[1] = dx` (`0x8DCEF`, fresh) which this arm needs for the angle. The
   row-0E wiring must recompute dx inline or extend the helper (leg
   FU168-L7).
6. **Fresh installer-census delta.** FU-164 §3.1 named two immediate
   installers; fresh `get_xrefs_to 0x7E600` = 3 (adds `0x7D0F7`, the
   input-adjacent helper) and `0x82DD0` = 1 — consistent, no code claim moves.

---

## 10. Numbered legs (FU168-L*)

1. **FU168-L1 — the descriptor term.** `byte[[rec+4]+0x15] << 6` in the event
   magnitude (`0x829aa..0x829b5`) is unmodeled engine-side (the descriptor
   staging is 0, FU-142b); with 0 it vanishes. The live value needs the
   roster-descriptor byte (same family as FU-142b's `player_d/e`).
2. **FU168-L2 — the dropped triple word 0.** `0x1577B8` stores the magnitude
   (`0x71cc8`); `fifa96_camera_event_set` keeps only the seed pair
   (`camera.h:296-299`). Whether word 0 matters outside the read window is
   open.
3. **FU168-L3 — the `0x71C94` tail cells.** `[0x1577CE+side*4]`,
   `[0x158777]`, `[rec+0x9B]`, `[[rec]+0x7A6]+0x7BF/0x7CB`, `[0x157815]`,
   `[0x157820]/[0x157822]` are unmodeled by the setter (the camera header's
   OL-T11-79 leg list carries them).
4. **FU168-L4 — the live pan window.** The stage-2 else arm is reachable only
   in the ~1-2 granted frames between the stage-1 anim install and the f2
   aux-0x0E jump arming (the engine's scene stage runs after the dispatch,
   `match_run.c:3531/3576`); the native's exact frame accounting is
   ISO-observable only.
5. **FU168-L5 — the mid-band camera-Y producer.** The gate needs `0x20 <
   [0x157750] < 0x70`; the current live chain's Y is 0 outside height events.
   FU-157 §5's throw-in `0x80` cut (`handlers.c:1467` drop) and the receive
   arm's `traj` are the candidate producers (R19/FU-171 overlap).
6. **FU168-L6 — the per-type offset staging.** `record/entity place_offset_x/z`
   are staged from the pool but never written live (grep) → 0; the arm should
   read `fifa96_match_dir_x/_z` directly (the same bytes) or stage the pool
   offsets.
7. **FU168-L7 — helper surface gaps.** (a) `fifa96_arm_dist_stage` drops dx
   (§9.5); (b) three lane-word reads use the wrong width (§9.4); (c) the
   `0x795A4` mul16 has three private copies and no shared symbol.

---

## 11. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`disassemble_bytes` `0x82710..0x82AD0` (960 B, 262 insns — the whole row-0x0E
body), `0x71C94..0x71DD4` (320 B) + `0x71DD1..0x71E1F` (79 B) (the full
`0x71C94` + the `0x71DF4` prologue), `0x82F58..0x82F84` (44 B, the
`FUN_00082DD0` install tail), `0x7E798..0x7E7D0` (56 B, the `FUN_0007E600`
tail), `0x7D9A4..0x7DAB4` (272 B, the full dispatcher), `0x8DCD4..0x8DD60`
(140 B, the metric), `0x8DD70..0x8DDA8` (56 B, the angle wrapper),
`0x73840..0x738B0` (112 B, the integrator's focus write), `0x7D0E0..0x7D118`
(56 B, the `0x7D0F7` caller), `0x815F0..0x81620` (48 B, the `0x8160F`
caller); `read_memory 0x82700` (16 B, the arm table); `get_xrefs_to 0x71C94`
(11), `0x82DD0` (1: `0x8307D`), `0x7E600` (3: `0x7EC89`, `0x8160F`,
`0x7D0F7`); `get_function_by_address 0x7D0F7`/`0x8160F` (no function).
Engine/loader source read: `fifa96_match_handlers.c` (852-931, 995-1015,
1190-1330, 2185-2215, 2340-2577, 2940-3000), `fifa96_match_run.c` (1095-1117,
1270-1320, 1428-1472, 1525-1564, 1780-1815, 3130-3259, 3440-3600),
`fifa96_match_entities.c` (290-340, 405-465), `fifa96_match_entities.h`
(120-200), `fifa96_match_run.h` (380-500), `fifa96_camera.c` (1-150, 700-940,
1000-1100), `fifa96_camera.h` (1-140, 270-436), `fifa96_arm_helpers.c`
(1-117), `fifa96_arm_helpers.h` (1-160), `fifa96_outfield.c` (555-600),
`fifa96_entity_update.h` (1-95), `fifa96_match_locomotion_tables.c` (25-35),
`tests/test_engine_match_frame.c` (2590-2665), `tests/test_engine_m2.c`
(1090-1110, 2655-2670), `tests/test_action_possession.c` (40-50, 320-345),
`docs/ghidra/FU156/FU157/FU160/FU164` (cross-cited) and `docs/ENGINE.md`
(260-280, 1815-1900). No writes: no rename, comment, label, function, script
or project save.
