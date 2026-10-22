# FU311 — the keeper ladder's natural CX=4 (R2-L5, phase 30 recon-ahead)

**Phase/Provenance:** phase-30 "the natural reach", Stage-1 recon-ahead wave,
task 2 of 4 (`docs/superpowers/plans/2026-10-21-fifa96-m2-phase30-recon-ahead.md`,
Task 2 — `FU311: the keeper ladder's natural CX=4`). Read-only derivation:
Ghidra MCP against `/FIFA96.EXE` (`watcom:LE:32:default`, image base 0, `.image`
overlay; `get_current_program_info` fresh this session) + read-only engine
sources + one `gdb` measurement pass over the existing `build/test_engine_m2`.
No repo file was modified except this draft; no git; no Ghidra write.

---

## 1. The question

> Given a bootstrap place, can the keeper stage select reach `CX=4`
> naturally, or is `stage_select ran=0` a second, independent blocker?

The phase-29 tape probe measured
`stage_select ran=0 cx1=0 cx4=0 table4=0` over the natural M2 transcript
(`docs/ENGINE.md:2337`, `docs/PROGRESS.md:2554`). That datum is ambiguous by
construction: `ran`, `cx1`, `cx4` and `table4` are all counters that read zero
both when the AX prepass gate closes **and** when the ladder is never called.
This slice separates the two with a fresh measurement (§4), then states the
exact natural preconditions for `ran=1` (§5) and for `CX=4` (§6), and the
consequence for Stage-2 (§8).

Two things are *not* in question, because they are proven below:

* the ladder **is called** on the natural tape — 2019 times out of 18124
  record-0 steps (§4.2); `ran=0` is the AX gate (`0x77F9A`/`0x77FA5`), not the
  call path;
* the keeper's row-`0x1B` install target (`CX=4` → `0x78294`) writes the camera
  only in **phase `0x0D`** (`0x76D3D`), while the ladder gate requires **phase
  2** (`0x78583`). The two never hold on the same frame (§6.3).

---

## 2. Native — the stage-select call path (`0x77F57..0x78220`)

All bytes below are **fresh this session** (`disassemble_bytes`
`0x77E90..0x782CF`, `read_memory 0x77E98`).

### 2.1 The camera copy and the `0x795B4` call (`0x77F57..0x77F90`)

```
0x77f57  MOV EDI,ESP
0x77f59  MOV ESI,0x15774C
0x77f5e  MOVSD x3                    ; stack[0..0xB] = [0x15774C/50/54]
0x77f61  MOV ESI,[ESP]               ; cam.x
0x77f64  TEST ESI,ESI / JL ..        ; ABS
0x77f74  MOV ESI,EAX                 ; SI = |cam.x| (16-bit reg)
0x77f70  MOV EDI,[ESP+8]             ; cam.z   (0x77F70 executes before 0x77F74)
0x77f76  TEST EDI,EDI / JL .. / NEG  ; ABS -> EAX
0x77f8b  MOV EDI,EAX                 ; DI = |cam.z|
0x77f82  LEA EBX,[ESP+0xC]           ; the 3-word scratch
0x77f86  MOV EDX,0x157C48            ; the focus triple argument
0x77f8d  LEA EAX,[EBP+0x59]          ; rec pos
0x77f90  CALL 0x795B4
```

The camera triple the machine reads is exactly `[0x15774C] / [0x157750] /
[0x157754]` — the same three cells the row-`0x1B`/`0x1E` heads store and the
same Y cell `render.camera.pos_y` carries (FU307-L4, carried). The scratch
`[ESP+0xC]` is read later as the stage-table "bearing" (`0x780E2`/`0x781A1`);
its producer (`0x795B4` out[0] = the folded-angle band, not an angle —
`include/fifa96_loader/fifa96_entity_update.h:84-106`) stays **FU290-L2**
(carried), so the stage-table arm is out of scope here beyond noting that it is
the third branch.

### 2.2 The AX prepass gate (`0x77F95..0x77FF5`)

```
0x77f95  MOVSX EDX,SI                ; |cam.x| truncated to 16 bits, sign-extended
0x77f98  XOR EAX,EAX                 ; AX = 0 (the gate flag)
0x77f9a  CMP EDX,0x420
0x77fa0  JGE 0x77ff5                 ; gate fails
0x77fa2  MOVSX EDX,DI                ; |cam.z| truncated to 16 bits, sign-extended
0x77fa5  CMP EDX,0x7b0
0x77fab  JLE 0x77ff5                 ; gate fails
0x77fad  MOV EDX,[0x157A49]
0x77fb3  SAR EDX,0x18                ; EDX = byte[0x157A4C]  (the alias: 0x157A4C, NOT 0x157A49)
0x77fb6  CMP EDX,0x1
0x77fb9  JNZ 0x77fc6
0x77fbb  CMP dword[0x157754],0       ; the `[0x157A4C]==1` arm
0x77fc2  JLE 0x77ff5                 ; cam.z must be > 0
0x77fc4  JMP 0x77ff0                 ; AX = 1
0x77fc6  MOV EDX,[EBP]               ; rec class object
0x77fc9  CMP byte[EDX+0x826],0       ; the record's side byte
0x77fd0  SETZ DL                    ; side0 = (side == 0)
0x77fd3  MOV EBX,EDX / AND EBX,0xff
0x77fd5  MOV ESI,[0x157754]          ; cam.z
0x77fe1  TEST ESI,ESI
0x77fe3  SETG DL                    ; posz = (cam.z > 0)
0x77fe6  AND EDX,0xff
0x77fec  XOR EDX,EBX
0x77fee  JZ 0x77ff5                  ; equal -> gate fails
0x77ff0  MOV EAX,0x1                 ; AX = 1
0x77ff5  TEST AX,AX
0x77ff8  JNZ 0x7802b                 ; gate open -> the stage selector
```

**The gate, verbatim:** `|cam.x| < 0x420 && |cam.z| > 0x7B0`, where both abs
values are the **low 16 bits sign-extended** (`MOVSX EDX,SI/DI`), and then

* `byte[0x157A4C] == 1` → `cam.z > 0`;
* else → `(cam.z > 0) XOR (rec_side == 0)`.

So the sign arm is a **side-matching** arm: with `[0x157A4C] == 0` a `+z` place
opens only for the side-1 keeper and a `-z` place only for the side-0 keeper.

The closed arm (`AX == 0`) does not return to a no-op:

```
0x77ffa  MOV EAX,[EBP+0x69] / SAR 0x10
0x78000  CMP EAX,0x40                ; lane_x < 0x40
0x78003  JGE 0x782c4                 ; else leave
0x78009  CALL 0x92ac8                ; the RNG-ish pick
0x78010  AND AL,1 / ADD EAX,2        ; 2 or 3
0x7801a  MOV EAX,EBP
0x7801c  CALL 0x6DC88                ; the non-ladder arm
```

`FUN_0006DC88` (fresh `0x6DC88..0x6DCC7`) writes the `[0x158743]` byte from
`0x6DBCC(rec, 2|3)` (`0x6DCA1`), runs the `0x8DCD4` metric (`0x6DCAE`) and
installs **code `0x07`** deferred (`0x6DCBC CALL 0x7D9A4(rec,7,0,EBX=0)`).
That is the `FU290-L6` non-ladder arm; the port has no counterpart (§3.1).

### 2.3 The CX selector (`0x7802B..0x781E0`)

```
0x7802b  XOR AL,AL
0x7802d  MOV [0x157C5C],AL           ; the latch := 0 (re-set in the CX=4 arm)
0x78032  MOV EAX,[EBP+0x69]
0x78035  SAR EAX,0x10                ; lane_x = word[rec+0x6B]

; CX=1 (0x78038..0x78060)
0x78038  CMP EAX,0x20
0x7803b  JGE 0x78065
0x7803d  CMP dword[0x157750],0x4     ; cam_y < 4 (signed dword)
0x78044  JGE 0x78065
0x78046  MOV EAX,[0x1577BC]
0x7804b  SAR EAX,0x10                ; follow_speed = word[0x1577BE]
0x7804e  CMP EAX,0xa
0x78051  JGE 0x78065
0x78053  MOV ESI,0x15774C / LEA EDI,[EBP+0x4D]
0x7805b  MOV ECX,0x1                 ; CX = 1
0x78060  JMP 0x781de                 ; the MOVSDx3 copy, then the tail

; CX=4 (0x78065..0x780DD)
0x78065  MOV EAX,[EBP+0x4]
0x78068  MOV EAX,[EAX+0xb]
0x7806b  SAR EAX,0x18                ; sub_0b = byte[[rec+4]+0xE], SIGN-EXTENDED
0x7806e  MOV EDX,[EBP+0x69]
0x78071  ADD EAX,EAX                 ; 2*sub_0b
0x78073  SAR EDX,0x10                ; lane_x
0x78076  ADD EAX,0x30
0x78079  CMP EDX,EAX
0x7807b  JGE 0x780e2                 ; lane_x >= 2*sub_0b+0x30 -> the stage table
0x7807d  CMP dword[0x157750],0x0     ; cam_y == 0 EXACTLY
0x78084  JNZ 0x780e2
0x78086  MOV EBX,[0x157A83]          ; the actor cell
0x7808c  TEST EBX,EBX
0x7808e  JZ 0x780e2                  ; no actor -> the stage table
0x78090  MOV EDX,[EBX] / MOV EAX,EBX ; actor class
0x78094  MOV EBX,[EBP]               ; rec class
0x78097  MOV DL,byte[EDX+0x826]
0x7809d  CMP DL,byte[EBX+0x826]
0x780a3  JZ 0x780e2                  ; same side -> the stage table
0x780a5  MOV ESI,0x15774C / LEA EDI,[EBP+0x4D] / MOVSD x3
0x780b0  MOV EDX,[EAX+0x71] / SAR 0x10 / ADD EDX,EDX
0x780b6  MOV EBX,[EBP+0x4d] / ADD EBX,EDX / MOV [EBP+0x4d],EBX
0x780c0  MOV EAX,[EAX+0x73] / SAR 0x10 / ADD EAX,EAX
0x780c6  MOV ECX,[EBP+0x55] / ADD ECX,EAX / MOV [EBP+0x55],ECX
0x780cd  MOV BH,0x1
0x780d2  MOV ECX,0x4                 ; CX = 4
0x780d7  MOV [0x157C5C],BH           ; the latch := 1
0x780dd  JMP 0x781e1
```

**Alias note (the `>>16` trap):** the x copy term is `dword[actor+0x71] >> 16`
= the **word at `actor+0x73`**, and the z term is `dword[actor+0x73] >> 16` =
the **word at `actor+0x75`**. The port's `actor_vel_x` / `actor_vel_z`
(`fifa96_keeper_cpu.h:91-92`) hold the dwords, so `>> 16` there is correct —
but any future re-bind must keep the dword, not the word at `+0x71`.

**The CX=1 arm is tested first and wins ties.** With `cam_y < 4` and
`follow_speed < 0xA` — the state after any `0x700F4` place, which zeroes the
speed (`0x701fd`/`0x70204`, §2.6) — the only escape from CX=1 is
`lane_x >= 0x20`. CX=4 then needs `lane_x < 2*sub_0b + 0x30`. `sub_0b` is
sign-extended, so the CX=4 lane window is

```
0x20 <= lane_x < 0x30 + 2*sub_0b        with sub_0b in [-128,127]
=> the window is at most [0x20, 0x12E)   (302 wide) and can be empty
```

### 2.4 The tail (`0x781E1..0x78220`) — the FU307 erratum, settled

```
0x781e1  TEST CX,CX
0x781e4  JZ 0x782c4                  ; CX == 0 -> no install
0x781ea  MOV EAX,EBP
0x781ec  CALL 0x765C4                ; the tracked-teammate predicate
0x781f1  TEST AX,AX
0x781f4  JZ 0x78205
0x781f6  MOV ECX,0x1 / MOV EDX,0x4 / JMP 0x782bb   ; predicate -> install 0x04, invoke-now
0x78205  DEC ECX
0x78206  CMP CX,0x4
0x7820a  JA 0x782c4                  ; CX-1 > 4 -> no install
0x78210  XOR EAX,EAX / MOV AX,CX
0x78215  JMP dword CS:[EAX*4+0x77e98]
```

`read_memory 0x77E98` (20 B) = `{0x7826D, 0x78245, 0x7821D, 0x78294, 0x782B1}`
for `CX-1 = 0..4`. The five arms (fresh `0x7821D..0x782C3`):

| CX | table idx | arm | install | side effects |
|----|-----------|-----|---------|--------------|
| 1 | 0 | `0x7826D` | `0x1A` | `[0x157C2C] = 0x2C` (`0x7827E`) |
| 2 | 1 | `0x78245` | `0x1A` | `[0x157C2C] = 0x2D` (`0x78258`) |
| 3 | 2 | `0x7821D` | `0x1A` | `[0x157C2C] = 0x2E` (`0x78230`) |
| **4** | **3** | **`0x78294`** | **`0x1B`** | none |
| 5 | 4 | `0x782B1` | `0x1C` | falls into `0x782BB` |

Every arm is `FUN_0007D9A4(rec, code, EDX, ECX=1)` — **invoke-now**, unlike the
machine's deferred `ECX=0` install (`0x7856A XOR ECX,ECX`, §2.6).

**FU307 erratum 1 (quoted) — "`0x780dd e9ff000000 JMP 0x781E1 ; the install
tail`" — is hereby confirmed and closed:** the tail is conditional, exactly as
the erratum's corrected listing says. The erratum's open sub-point is settled:
the review's shorthand ("only CX=0 takes the DEC") is **wrong** — `0x781E4 JZ
0x782C4` sends CX=0 away *without* the DEC, and `0x78205 DEC ECX` sits only on
the `CX != 0 && predicate == 0` path. **So a CX=4 with the `0x765C4` predicate
nonzero installs row `0x04`, not row `0x1B`; the `0x38` route is the
predicate-zero, index-3 path.**

### 2.5 The predicate `0x765C4` (fresh `0x765C4..0x76615`)

```
0x765c7  MOV EDX,[0x1577CA]          ; the tracked record
0x765cd  TEST EDX,EDX / JZ 0x76610    ; absent -> 0
0x765d1  MOV EBX,[EDX] / MOV ECX,[EAX]
0x765d5  MOV AL,[EBX+0x826]
0x765db  CMP AL,[ECX+0x826]
0x765e1  JNZ 0x76610                 ; different side -> 0
0x765e3  MOV EAX,[EDX+0x8e] / SAR 0x18   ; the tracked's byte +0x91
0x765ec  CMP EAX,0x5 / JNZ 0x765fa
0x765f1  MOV EAX,0x1 / RET           ; tracked code == 5 -> 1
0x765fa  MOV EAX,[0x158740] / SAR 0x18   ; byte[0x158743]
0x76602  CMP EAX,0x4 / JZ 0x76610
0x76607  MOV EAX,0x1 / RET           ; != 4 -> 1
0x76610  XOR EAX,EAX
```

= "present **and** same side **and** (tracked code `== 5` **or**
`byte[0x158743] != 4`)". The port's
`fifa96_keeper_cpu_tracked_predicate` (`fifa96_keeper_cpu.c:110-122`) is
byte-equivalent. Note the cross-link found this session: `byte[0x158743]` is
written by the *non-ladder* arm (`0x6DCA1`, §2.2) — so a closed AX gate seeds
the very byte that later diverts a `CX=4` install to row `0x04`.

On the natural tape `tracked_present == 0` at every ladder call (§4.4), so the
predicate is 0 and the `CX=4` arm would install row `0x1B`.
### 2.6 The ladder caller and its gates (`0x784D3..0x785AD`)

```
0x784d3  MOV EAX,[EBP+0x8e] / SAR 0x18      ; the code, byte +0x91
0x784dc  MOV AL,byte[EAX+0x110680]          ; the permit table
0x784e2  AND AL,0x1
0x784e9  JZ 0x7859e                          ; permit 0 -> skip install AND ladder
0x784ef  ... the install tree (0x784EF..0x78553)
0x78558  MOVSX DX,byte[EBP+0x91]
0x78560  CMP AX,DX / JZ 0x7857b              ; the same-code guard
0x78565  ... MOV [0x157AB2],BH(1); CALL 0x7D9A4  (deferred, ECX=0)
0x7857b  MOV EAX,[0x157A4A] / SAR 0x18
0x78583  CMP EAX,0x2 / JNZ 0x7859e           ; phase must be 2
0x78588  CMP dword[EBP+0x20],0x0 / JNZ 0x7859e
0x7858e  CMP byte[0x157C5D],0x0 / JZ 0x7859e ; THE INTERLOCK
0x78597  MOV EAX,EBP
0x78599  CALL 0x77EAC                        ; the ladder
0x7859e  XOR CH,CH
0x785a2  MOV byte[0x157C5D],CH               ; unconditional clear
0x785a8  CALL dword[EBP+0x18]                ; the same-frame row dispatch
```

* **Permit (`0x110680`, fresh 45 B):** `{3,0,0,3,3,3,3,2, 0×8, 2,2,2,2, 0×5,
  3, 0×…}` → `byte[0x110680 + 0x19] = 3`, `& 1 = 1`. `[0x110680 + 0x1E] = 0`.
  The port's table (`fifa96_keeper_cpu.c:15-20`) is identical.
* **The `[0x14C1D4]` bit-9 gate (`0x77ED9..0x77EF0`):**
  `ECX = (side ^ byte[0x157ABE]) & 1` (`FUN_000741B4`, fresh `0x741B4..0x741C3`
  = `MOV DL,byte[0x157ABE]; XOR EAX,EDX; AND EAX,1`), then
  `AX = word[EAX*2 + 0x14C1D4]`, `AH & 2`; nonzero → the tail.
  **Fresh `read_memory 0x14C1D4` (16 B) = all zero**, but `get_xrefs_to` shows
  **runtime writers**: `0x1C9D4` (`= 0`), `0x1C9E3` (`= 2`) in `FUN_0001C9BC`,
  and `0x24B98` (`= BX`) in `FUN_00024B00`. All three store a **word** whose
  high byte is 0 (`2`) or an XOR of small values, so `AH & 2` has no observed
  setter; the gate is effectively open but *not* statically provable — leg
  FU311-L3.
* **Interlock `[0x157C5D]`, fresh 3-ref census:** read `0x7858E`, clear
  `0x785A2`, set `0x74707` (the row-`0x19` body). FU299's Option B is exact.
* **The ladder's only caller, fresh `get_xrefs_to 0x77EAC` = 1** (`0x78599`).

### 2.7 The row-`0x1B` install target's own camera gate

`0x78294` installs `0x1B`; the row body is `0x76D28` (fresh
`read_memory 0x1106E0`: `[0x1B] = 0x76D28`, `[0x19] = 0x746E4`,
`[0x1A] = 0x7662C`, `[0x1E] = 0x7550C`). Its head:

```
0x76d31  MOV EBP,EAX
0x76d33  MOV EAX,[0x157A4A] / SAR 0x18
0x76d3d  CMP EAX,0xD
0x76d40  JNZ 0x76dc5                 ; PHASE 0x0D ONLY
0x76d46  MOV EAX,[EBP] / MOV AL,[EAX+0x826]
0x76d4f  MOV EDX,[0x157AAC] / SAR 0x18
0x76d5d  CMP EDX,EAX / JNZ 0x76dc5   ; rec side == the controlled side
0x76d61..0x76d81  cam_x/cam_z = pos + byte[0x10F334/0x10F33C + sector] << 4
0x76d9f  MOV EAX,[EBP+0x5d] / ADD EAX,0x38
0x76db5  MOV [0x157750],EAX          ; cam_y = pos_y + 0x38
0x76dba  CALL 0x700F4                ; the camera PLACE
0x76dbf  MOV [0x157A83],EBP          ; the actor cell
```

So a `CX=4` install at phase 2 dispatches a row body whose camera place is
**skipped** (`0x76D40`). The Y=`pos_y+0x38` write at `0x76DB5` is
phase-`0x0D`-only. Only a later frame that reaches phase `0x0D` with the record
still on row `0x1B` could place it — and phase `0x0D`'s producer (situation 7,
`handlers.c:6414`) is unported (`FU261-L6`, carried).

### 2.8 The row-`0x1E` dispatch and the band write

**Dispatch identity (fresh `read_memory 0x1106E0`):** `[0x1E] = 0x7550C` — the
same table FU151 §3.1 names (`0x1106E0[0x1E] = 0x7550C`).

**Producer (fresh `0x8D5D9..0x8D63D`):** the phase-8/9 arm of `FUN_0008D8EC`:

```
0x8d5d9  PUSH -1 / MOV ECX,3 / MOV EBX,0xA / CALL 0x8CEB8   ; install 3 over 0..0xA
0x8d5ee  MOV EDX,[0x157AAC] / SAR 0x18                       ; the controlled side
0x8d5f9  MOV AL,byte[EBP+0x826]
0x8d5ff  CMP EAX,EDX / JNZ 0x8d814                            ; other side -> leave
0x8d60f  CMP EAX,9 / SETZ AL                                  ; AL = (phase == 9)
0x8d61c  ADD EAX,0x1d                                         ; 0x1E if phase 9, else 0x1D
0x8d626  MOV [EBP+0x7b2],EBP
0x8d62c  CALL 0x7D9A4(rec, code, 0, ECX=0)                    ; deferred install
```

Natively: **phase 9 → row `0x1E`; phase 8 → row `0x1D`**, on record 0 of the
controlled team. The port mirrors it at `fifa96_match_run.c:3026-3034`
(`phase == 8 || phase == 9` → `install(records[0], phase == 9 ? 0x1E : 0x1D)`).

**The head place (fresh `0x7550C..0x755D4`, `read_memory 0x754E4`):**

```
0x75517  MOV [EBP-0xC],EAX
0x7551a..0x75536  stage < 6 && [rec+0x20]==0 -> CALL 0x7876C (the slot-merge request)
0x7553b..0x7554a  stage >= 3 -> 0x755d4
0x75553  CMP byte[EAX+0x9B],0 / JNZ 0x755d4   ; no-ball gate
0x7555c..0x75579  cam_x = pos_x + byte[0x10F334+sector]<<4
0x7557f..0x75593  cam_z = pos_z + byte[0x10F33C+sector]<<4
0x7559b  MOV EAX,[EAX+0x5d] / ADD EAX,0x38
0x755a9  MOV [0x157750],EAX                   ; THE BAND WRITE (pos_y + 0x38)
0x755c7  CALL 0x700F4                         ; the camera PLACE, flag 1
0x755cf  MOV [0x157A83],EAX                   ; the actor cell
```

The ten-stage table `0x754E4` (fresh) = `{0x75611, 0x7565A, 0x75770, 0x75795,
0x75B90, 0x75D17, 0x75DF3, 0x75E82, 0x7609D, 0x760C6}` and holds four more Y
writers, all **raw** (no `0x700F4`):

| site | stage arm | value | fresh? |
|------|-----------|-------|--------|
| `0x755A9` | head (stage<3, `+0x9B==0`) | `pos_y + 0x38` | fresh (+ `0x700F4` at `0x755C7`) |
| `0x7568B` | stage 1 (`0x7565A`) | `pos_y` (the triple `0x7568A/8B/8C`) | fresh |
| `0x75A0F` | stage 3 (`0x75795`) | `0x38` (triple `0x759FD..0x75A02`) | fresh |
| `0x75D69` | **stage 5** (`0x75D17`; gate `[[rec+0x28]]==0x45 && [rec+0x3A]>>24>=3`) | `0x50` (triple `0x75D52..0x75D54`) | fresh |
| `0x75EEA` | stage 7 (`0x75E82`) | `0x10` | fresh |
| `0x75F86` | stage 7 (`0x75E82` window) | `0x80` (`MOV [0x157750],ESI`) | carried FU307 |

**Erratum (FU151 §3.1's "Outlet/throw (stages 4, 7)") — the `0x50` store is
stage 5, not stage 7.** Quote: "**Outlet/throw (stages 4, 7)** and release
(stage 5): … stage 7 places the throw origin". **Corrected (fresh):** the
`0x50` write at `0x75D69` sits in the arm that begins at the table entry
`0x75D17` = **stage 5** (the `anim row 0x45` release arm), and the `0x10`/`0x80`
pair sits in the stage-7 arm `0x75E82`. The arm boundaries are the fresh table
entries above, not the prose's stage grouping.

**FU311 relevance:** `0x38` and `0x50` are both inside the pan's strict mid-band
`0x20 < camY < 0x70` (FU168 §2.2, carried), and the head place is a real
`0x700F4` place (camera triple + event reset). That makes the row-`0x1E` head a
**camera writer reachable without the ladder** — but only from phase 9's
controlled-team install (§8.3).

**FU307-L7, settled (confirmed):** `0x7568A/8B/8C` is a genuine
`dword[0x157750]` write (the `MOVSD` middle at `0x7568B`, value `pos_y`) with
**no** `0x700F4` anywhere in the stage-1 arm (fresh `0x7565A..0x756A9`). It is
outside the three-literal raw-store scope, so the engine still does not deliver
it (`fifa96_keeper.c:516-523` writes `s->cam_y = s->pos.y`;
`handlers.c:515-525` publishes only on `cam_y_written`).

### 2.9 `0x700F4` resets the follow speed

`0x700F4` is the camera **place**: it copies its argument triple into
`0x15774C/50/54` (`0x7014e..0x70150`), clears `pan_counter`
(`0x70153`), `event_param` (`0x70159` → `[0x1577EE]`), `event_peak`
(`0x70160`), `[0x1577F4]` (`0x70167`), `[0x157800]/[0x157802]`
(`0x7016e/0x70175`), `[0x1577F6]` (`0x70180`), and **`[0x1577BC] = 0` /
`[0x1577BE] = 0`** (`0x701fd`/`0x70204`) — i.e. `follow_speed := 0`. It also
zeroes `[0x1577EE]` at `0x70239` and re-copies the triple into the four anchor
triples (`0x157764`, `0x157770`, `0x157788`, `0x157794`).

**Consequence:** every place, including any bootstrap cut, leaves
`follow_speed == 0 < 0xA`, so the CX=1 speed term is *always* satisfied right
after a place and CX=1 is escaped only by `lane_x >= 0x20`.

---

## 3. Port — the shape as it stands

### 3.1 `src/fifa96_loader/fifa96_keeper_cpu.c`

* `fifa96_keeper_cpu_stage_select` (`:141`): the camera copy and abs
  (`:158-161`, incl. the `MOVSX` truncation), the AX gate (`:165`) and the
  sign/side arm (`:166-172`) are byte-equivalent to §2.2; `if (ax == 0) return`
  (`:174`) — the non-ladder arm is absent (`FU290-L6`, carried).
* `lane_x = rec->lane69 >> 16` (`:177`), latch clear (`:179`), CX=1
  (`:182-189`), CX=4 (`:193-202`, bound `rec->sub_0b * 2 + 0x30`, `cam_y == 0`,
  actor present + opposite side, latch := 1, the `2*(>>16)` copy terms), the
  stage table (`:204-232`).
* The ladder (`:236`): head gates (`:256-281`), the AX call (`:286`),
  `if (!sel.ran) return` (`:294`), the `0x765C4` predicate → install `0x04`
  (`:299-305`), the five arms (`:309-330`) with `case 4` → `0x1B` (`:322-324`).
* `fifa96_keeper_cpu_tracked_predicate` (`:110-122`).
* The permit table (`:15-20`) and `fifa96_keeper_cpu_permit` (`:22-25`).

### 3.2 `src/fifa96_engine/fifa96_match_entities.c` — the bind

The record-0 machine runs in the `i == 0` branch (`:732-868`), before the single
dispatch (`:872`). The view (`:743-813`):

| native input | bind | line |
|---|---|---|
| `byte[rec+0x826]` | `pool->team[team].side` | `:747` |
| `byte[rec+0x91]` | `e->code` | `:748` |
| `[0x157A4A]>>24` | `frame->phase` | `:749` |
| `rec == [0x157A83]` | `act == ENTITY_ID(...)` | `:750` |
| `[0x157A83]` / `+0x826` cmp / `+0x71,+0x73` | `actor_present`, `actor_same_side`, `actor_vel_x/z` | `:757-764` |
| `word[rec+0x81]`, `dword[rec+0x20]`, `byte[rec+0x9A]` | `timer81`, `queued20`, `gate_9a` | `:766-768` |
| `[0x110680][code]&1` | `fifa96_keeper_cpu_permit(e->code)` | `:773` |
| `[0x15774C/50/54]` | `frame->cam_pos_x/y/z` ← `render.camera.pos_*` (`match_run.c:1590-1592`) | `:774-776` |
| **`dword[rec+0x69]`** | **`kc_rec.lane69 = e->lane`** | **`:777`** |
| `word[0x1577BE]` | `frame->follow_speed` | `:778` |
| `byte[0x157A4C]` | `frame->global_157a4c` (`match_run.c:1601`) | `:779` |
| `[0x157C5D]`/`[0x157C5C]`/`[0x157AB2]` | the FU299 hug pointers | `:813-826` |

The install apply (`:844-849`) and the target copy (`:852-866`) run **before**
the same-frame dispatch (`:872`) — the native `0x785A8` order.

**The lane bind is wrong (new, measured):** the native reads
`dword[rec+0x69] >> 16` = **`word[rec+0x6B]`**, and the port maintains that word
as `e->lane_x` (`fifa96_match_entities.c:221`, refreshed at `0x799D5`;
`lane` is a *separate* `int32` field, `include/fifa96_engine/fifa96_match_entities.h:141-148`).
The bind passes `e->lane`, whose high word is never re-synced from `lane_x`
(only the low word is masked, `:361`). Measured: `e->lane >> 16 == -1` on every
one of 3700 phase-2 samples while `e->lane_x` runs 0..3176 (§4.3). So the port's
`lane_x` is a stale `-1`, not the live band.

### 3.3 Handler bodies

* Row `0x1B` head: `fifa96_match_handlers.c:4230-4246` — `phase == 0x0D &&
  team->side == side_controlled`, the sector offsets, `camera.pos_y = pos_y +
  0x38` (`:4239`), the flag-1 place (`:4240-4243`), `has_ball = 1` (`:4244`),
  `controlled = 1` (`:4245`, the actor cell). **Faithful to `0x76D33..0x76DBF`.**
* Row `0x1B` cadence: `stage == 0` → clears the six latches and sets
  `stage = 1` (`:4260-4276`), then `if (stage == 1)` … `if (stage == 2)` in the
  same dispatch — the FU307-L6 divergence (§7).
* Row `0x1E` head: `src/fifa96_loader/fifa96_keeper.c:489-501` — `stage < 3 &&
  has_ball == 0` → `cam_y = pos.y + 0x38` (`:491`), `place = 1`,
  `place_pause = 1` (`:493-497`), `has_ball = 1` (`:498`), `controlled = 1`
  (`:500`). Stage 1 (`:516-523`) reproduces the `0x7568A` triple into
  `s->cam_y`, but the apply (`handlers.c:515-525`) publishes only
  `cam_y_written` — FU307-L7.
* Row `0x19` sets the interlock: `handlers.c:4792` (`mr->global_157c5d = 1`).
* The phase arms: `match_run.c:3011-3019` (phase 6 penalty place
  `(0,0,±0x8D0)`), `:3026-3034` (phases 8/9 → `0x1D`/`0x1E` on record 0),
  `:3047-3054` (phase 4 corner place `±0x710/0/±0xB00`), `:3037-3045`
  (phase 3 goal-snap place).

---

## 4. Measurement — the tape probe, separated

**Method.** One read-only `gdb` batch pass over the existing
`build/test_engine_m2` (Debug, `-g`, unstripped; no rebuild, no source change):
breakpoints on `fifa96_keeper_cpu_step`, `fifa96_keeper_cpu_ladder_run`,
`fifa96_keeper_cpu_stage_select`, and on
`fifa96_match_entities.c:821` (the step call) printing the whole `kc_rec` plus
`e->lane_x`. CWD = repo root. Scripts: `/tmp/opencode/fu311_trace.gdb`,
`/tmp/opencode/fu311_bind.gdb`; logs `fu311_m2.log`, `fu311_bind.log`.

### 4.1 Counts (18124 record-0 steps)

| counter | value |
|---|---|
| `fifa96_keeper_cpu_step` calls | 18124 |
| `fifa96_keeper_cpu_ladder_run` calls (the `0x78599` gate fired) | **2019** |
| `fifa96_keeper_cpu_stage_select` calls (the head gates passed) | **1010** |
| `stage_select` calls whose printed inputs pass **both** AX terms | **0** (derived; matches the probe's `ran=0`) |
| steps with `phase==2 && permit==1` | 3700 |

### 4.2 The call path is open

`fifa96_keeper_cpu_step` input distribution (the code-`0x19` record):

| steps | code | phase | permit | interlock | slot |
|---|---|---|---|---|---|
| 2019 | 0x19 | 2 | 1 | **1** | 0 |
| 1681 | 0x19 | 2 | 1 | 0 | 0 |
| 2360 | 0x19 | 1 | 1 | 1 | 0 |
| 905 | 0x19 | 4 | 1 | 1 | 0 |
| 729 | 0x19 | 12 (0x0C) | 1 | 1 | 0 |
| 3161 | 0x19 | 19 (0x13) | 1 | 0 | 0 |

`2019 =` exactly the number of `ladder_run` calls — the permit, the code, the
phase, the no-slot and the interlock all hold on 2019 step-instances. The
other team's record 0 is code `0x26` with permit 0 and never enters.

### 4.3 The lane (the bind defect, measured)

3700 `phase==2 && permit==1` samples:

* `e->lane_x` (the native word `+0x6B`): mean 2028 (0x7EC); min 0, max 3176;
  `< 0x20` in **2** samples; `< 0x30` in **2**; `< 0x12E` in **2**;
  `>= 0x7B0` in **1510**.
* `kc_rec.lane69 >> 16` (what `stage_select` actually sees): **`-1` in all
  3700** samples.

So the port's `lane_x` is `-1` (always inside the CX=1 term) while the native's
is ~0x7EC (essentially never inside either CX window).

### 4.4 The stage_select inputs (1010 calls)

| cam triple | calls | `|x| < 0x420` | `|z| > 0x7B0` | sign/side (side 0, `[0x157A4C]=0`) |
|---|---|---|---|---|
| `(0,0,0)` | 900 | pass | **fail** | (not reached) |
| `(0x710,0,0xB00)` = `(1808,0,2816)` | 110 | **fail** | pass | (not reached) |

`cam_y == 0` at every call; `follow_speed == 0`; `[0x157A4C] == 0`;
`sub_0b == 0` (staged). The actor: 1010 calls have `actor_present=1,
actor_same_side=0`; the 1009 `ladder_run` calls that never reach `stage_select`
are all `actor_present=1, actor_same_side=1` (the `0x77F51` skip). The split is exact: the
`ladder_run` calls that never reach `stage_select` number `2019 - 1010 = 1009`,
and all 1009 are the `0x77F51` actor-same-side skip. `tracked_present == 0` at
all 2019 ladder calls, so the `0x765C4` predicate (`§2.5`) is 0 throughout.

The `(0x710,0,0xB00)` place is the port's phase-4 arm
(`match_run.c:3047-3054`; the native arm cite `0x8D35B` is carried from that
port comment); the `(0,0,0)` is the kickoff reset (FU157 §1, carried).

### 4.5 The verdict on the probe

**`ran=0` is the AX prepass gate, not the call path.** The ladder ran 2019
times and `stage_select` 1010 times; at every one of the 1010 entries at least
one AX term failed, at two distinct live places and for two distinct terms:

* `(0,0,0)` → `|cam.z| = 0 <= 0x7B0` (`0x77FA5`/`0x77FAB`);
* `(0x710,0,0xB00)` → `|cam.x| = 0x710 >= 0x420` (`0x77F9A`/`0x77FA0`), and the
  `+z` sign arm would also refuse a side-0 record.

The probe's `cx1=0 cx4=0 table4=0` are therefore *downstream* zeros: the CX
selector is never entered. The carried prose "the ladder does not run at all —
the AX prepass gate never opens" (`docs/ENGINE.md:2338`) is **half right**: the
gate never opens, but the ladder runs on every gated step. That distinction is
the whole point of this slice — see §6.1.

---

## 5. The natural precondition chain for `ran=1`

`ran=1` is exactly the AX gate (§2.2). Everything above it is the call path,
and the call path is live (§4.2). The chain in native order:

| # | precondition | site | natural status |
|---|---|---|---|
| 1 | record 0 of a team is stepped by `FUN_000782D0` | `0x8DB35`; port `entities.c:742` | **live** (2019+1681 steps) |
| 2 | `byte[0x110680][code] & 1` | `0x784DC`; `0x110680[0x19] = 3` | **live** (permit 1) |
| 3 | code stays `0x19` (the same-code guard) | `0x78560` | **live** (code 0x19) |
| 4 | `phase == 2` | `0x78583` | **live** (2019 steps) |
| 5 | `dword[rec+0x20] == 0` | `0x78588` | **live** (slot 0) |
| 6 | `byte[0x157C5D] != 0` | `0x7858E` | **live** (set by the row-0x19 body `0x74707`) |
| 7 | `byte[rec+0x91] == 0x19` | `0x77EC2` | **live** |
| 8 | `[0x14C1D4]` bit 9 clear | `0x77EDE..0x77EF0` | open, effectively inert (§2.6) — **leg FU311-L3** |
| 9 | `byte[rec+0x9A] == 0`, not `'T'`, `word[rec+0x81] == 0` | `0x77EF6..0x77F1E` | **live** (measured 0) |
| 10 | `rec != [0x157A83]` and (no actor or opposite side) | `0x77F34..0x77F51` | **live** (1010 opposite-side, 1009 same-side) |
| 11 | `|cam.x| < 0x420 && |cam.z| > 0x7B0` (16-bit MOVSX) | `0x77F9A`/`0x77FA5` | **BLOCKED on the tape** |
| 12 | `[0x157A4C]==1 ? cam.z > 0 : (cam.z>0) XOR (side==0)` | `0x77FAD..0x77FEE` | `[0x157A4C]=0`; the side arm decides |

**So `ran=1` needs exactly one thing the natural play does not have: a camera
place in the window**

```
|place.x| < 0x420  &&  |place.z| > 0x7B0     and the sign/side match
```

which is a **FU310** product, not a FU311 one. `ran=1` is *conditionally
reachable*: the phase-6 penalty place `(0,0,±0x8D0)` (port
`match_run.c:3014-3016`; the native arm cite `0x8D4A2` is carried from that
port comment) is inside the window (|x|=0, |z|=0x8D0) and has
`cam_y == 0`; whether it survives into phase 2 is FU310's question. Note the
sign: the port's phase-6 arm places `+0x8D0` for side 0 and `-0x8D0` for side 1,
and the sign/side arm then opens the gate for the *other* side — i.e. the
defending keeper, which is the only coherent reading.

---

## 6. The natural precondition chain for `CX=4`

### 6.1 The gate is a separate blocker from the call path

Answered by §4.5: two independent facts, one measurement. The **call path** is
open (2019 calls). The **gate** is closed (0 opens in 1010 entries). They are
independent blockers in the sense the task asks about: fixing the camera place
(`ran=1`) does *not* fix the CX=4 lane/side/phase chain, and vice versa.

### 6.2 The CX arm order and the lane arithmetic

Given `ran=1`, the CX=4 arm requires, in this order:

1. **CX=1 refused** (`0x78038`): `lane_x >= 0x20` **or** `cam_y >= 4` **or**
   `follow_speed >= 0xA`. A place zeroes the speed (`0x701FD/0x70204`), and
   `cam_y == 0` is itself a CX=4 precondition, so in practice:
   **`lane_x >= 0x20`**.
2. **CX=4 accepted** (`0x78065`): `lane_x < 0x30 + 2*sub_0b` with `sub_0b` a
   signed byte (max bound `0x12E = 302`), `cam_y == 0`, `[0x157A83] != 0` with
   `+0x826` != the record's own object's `+0x826`.
3. **The tail predicate** (`0x781EC`): `0x765C4 == 0`, else row `0x04` instead
   of row `0x1B`.

So the CX=4 lane window is `0x20 <= lane_x < 0x30 + 2*sub_0b`, at most
`[0x20, 0x12E)`, and `lane_x` is the **band from the record to the camera
focus** (`0x795B4(pos, 0x157C48, &+0x6B)`, port `entities.c:215-223` as
`e->lane_x`).

**Against the measured tape:** `lane_x` (native `e->lane_x`) is `< 0x12E` in
2 of 3700 phase-2 samples and `>= 0x7B0` in 1510; the phase-2 band sits mostly
in `0x700..0x800` (2258/3700) with mean 0x7EC. So during open play the keeper
sits ~0x7EC from the camera focus, and the CX windows are closed. The two
samples with a small band are the frames where the keeper and the camera are
both at the origin (kickoff) — where the AX gate is closed by `|cam.z| = 0`.

**Against the qualifying places:** a place that satisfies the AX gate must be
`> 0x7B0` from the origin in z. The keeper's typical distance from the origin
in phase 2 is ~0x7EC (the `0x700..0x800` band above). The tightest
qualifying place is the phase-6 penalty `(0,0,±0x8D0)`:
`0x8D0 - (0x700..0x800) = 0xD0..0x1D0` (208..464) from the keeper, *on the
keeper's own side* (the far-side place is thousands away) — and the measured
tail of the distribution (`0x797..0x7AA`, 1943..1962) gives the *best* case
`0x126..0x139` (294..313). The CX=4 bound can never exceed `0x12E` (302). So:

* with the port's staged `sub_0b = 0` (bound `0x30`), the place must land within
  **48** units of the keeper — no ported cut does;
* natively the answer is **decided by `sub_0b`, which has no producer**
  (FU290-L5 — this slice's FU311-L2). The typical place geometry (band
  `0xD0..0x1D0`) needs `sub_0b >= 0x50..0x9A` to open CX=4; the best-case
  geometry (band `0x126..0x139`) needs `sub_0b >= 0x7B..0x85`, i.e. the top of
  the signed-byte range or above it.

**`CX=4` is therefore NO-GO on every produced chain, and only parametrically
possible in the native** (`sub_0b` in the upper range + the keeper at the deep
end of its measured band). **In the port it is a hard NO-GO**: the port's
`lane_x` is the stale `-1` (`< 0x20`, §4.3), so `ran=1` alone would always take
**CX=1** — the port's CX selector is *biased to CX=1* whenever the gate opens.
That is the exact shape FU305 §5.3 predicted, now explained mechanically: the
lane bind (§3.2), not the gate.

### 6.3 The installed row's camera write is out of phase

Even a successful CX=4 → row `0x1B` install cannot move the camera in the frame
that allows it: the ladder gate needs **phase 2** (`0x78583`) and the row-`0x1B`
head place needs **phase 0x0D** (`0x76D3D`). The install's `0x38` band write at
`0x76DB5` is phase-`0x0D`-only, and phase `0x0D`'s only producer (situation 7)
is unported. So the ladder is a **keeper-reposition installer**, not a camera
bootstrap.

### 6.4 What CX=4 *would* do (the port-shape target)

If the gate and lane ever open, the native chain is complete and ported:

```
machine ladder gate (permit 1, code 0x19, phase 2, slot 0, interlock 1)
 -> CALL 0x77EAC -> head gates -> 0x77F9A/0x77FA5 gate -> 0x7802B latch=0
 -> CX=1 refused (lane_x >= 0x20) -> 0x78065 CX=4 accepted
 -> target +0x4D = cam + 2*(word[actor+0x73], word[actor+0x75])
 -> [0x157C5C] = 1 (0x780D7) -> 0x781E1 TEST CX -> 0x765C4 == 0
 -> DEC -> index 3 -> 0x78294 -> FUN_0007D9A4(rec, 0x1B, 0, ECX=1) invoke-now
 -> 0x785A8 same-frame dispatch -> row 0x1B stage 0 (0x76DF6: +0x9E=1, +0x89=0,
    the six latch clears, +0x92 += 1)
 -> (phase 0x0D only) 0x76DB5 cam_y = pos_y + 0x38, 0x76DBA CALL 0x700F4
```

All of it is ported (`keeper_cpu.c:286-330`, `entities.c:844-849`,
`handlers.c:4230-4276`); nothing here is missing except the two gates above.

---

## 7. Errata and numbered legs

### 7.1 Errata (quoted, corrected by fresh reads/measurement)

1. **`docs/ENGINE.md:2338` — "the ladder does not run at all".** Quote: "the
   ladder does not run at all — the AX prepass gate (`0x77F9A`/`0x77FA5`)
   never opens because the live camera stays `(0,0,0)`". **Corrected
   (measured, §4):** the ladder **ran 2019 times** and `fifa96_keeper_cpu_stage_select`
   **1010 times** over the same 165-frame M2 transcript; the probe's `ran=0` is
   the *gate inside* stage_select, not an absent call. The conclusion (the band
   is not natural-play-proven) stands; the mechanism sentence does not.
   `docs/PROGRESS.md:2554` repeats the same sentence and carries the same
   correction.
2. **FU307's erratum 1 sub-point.** Quote: "the review's shorthand 'only CX=0
   takes the DEC' does not match the carried bytes". **Settled (fresh
   `0x781E1..0x78220`, §2.4):** `CX==0` leaves at `0x781E4` *without* the DEC;
   the DEC at `0x78205` is on the `CX != 0 && predicate == 0` path. The
   erratum's corrected listing is right; the sub-point is retired.
3. **FU151 §3.1's stage grouping for the `0x50` band.** Quote: "**Outlet/throw
   (stages 4, 7)** and release (stage 5)". **Corrected (fresh table
   `0x754E4` + `0x75D17..0x75D69`):** the `0x50` store is in the **stage-5**
   arm (`0x75D17`, the anim-row-`0x45` release); the stage-7 arm (`0x75E82`)
   holds the `0x10`/`0x80` pair. Arm boundaries = the table entries.
4. **FU307-L7's "out of band (0)" label.** Confirmed as a value
   (`0x7568B` = `pos_y`), but note it is a *triple* write
   (`0x7568A/8B/8C` → `0x15774C/50/54`), so it also moves the focus X/Z that
   `lane_x` is measured against — not only Y.

### 7.2 Numbered legs (native owner named)

* **FU307-L1 — the natural-tape CX=4 reach: NO-GO (this slice).** CX=4 needs
  `0x20 <= lane_x < 0x30+2*sub_0b` (≤ `0x12E`) with `cam_y == 0` and an
  opposite-side actor, i.e. the camera within ≤302 units of the keeper, while
  the AX gate needs the camera `> 0x7B0` from the origin. The modal keeper depth
  (0x700..0x800, 2258/3700 samples) puts the tightest AX-qualifying place (the
  phase-6 penalty `(0,0,±0x8D0)`) 0xD0..0x1D0 away (0x126..0x139 in the best
  case) — inside the window only for `sub_0b` at the top of the signed range
  (FU311-L2), and never in the port (`sub_0b = 0` → 48 units). Owner: the place
  set (FU310) + `0x78065` (the CX=4 lane term). Not a call-path defect (§4.5).
* **FU307-L6 — the row-`0x1B` stage cadence: CONFIRMED, still a port-shape
  divergence.** Fresh `read_memory 0x76D14` = `{0x76DF6, 0x76E3B, 0x76FC7,
  0x7769F, 0x776C1}` and `0x76DEE JMP CS:[EAX*4+0x76D14]`; stage 0 increments
  `+0x92` (`0x76E2D..0x76E35`) — **one stage per call**. The port runs
  0→1→2 in one dispatch (`handlers.c:4260-4277`). Owner: `fifa96_match_action_1B`
  / `0x76DEE`. Moves *when* the band lands (same frame vs two frames later),
  never the value.
* **FU307-L7 — the row-`0x1E` stage-1 triple: CONFIRMED open.** `0x7568A/8B/8C`
  is a raw `0x15774C/50/54` write (Y = `pos_y`) with no `0x700F4`. The engine
  keeps it in `s->cam_y` only (`keeper.c:520-523`, `handlers.c:515-525`).
  Owner: `fifa96_keeper_claim_step` / `0x7568A`.
* **FU311-L1 (new) — the `lane69` bind reads a stale cell.** Native
  `dword[rec+0x69]>>16` = `word[rec+0x6B]`; the port binds `e->lane` whose high
  word is never refreshed from `e->lane_x` (`entities.c:777`, measured `-1` in
  3700/3700 samples vs `e->lane_x` 0..3176). Consequence: the port's CX
  selector is biased to CX=1, and once FU290-L6 lands it would also take the
  `0x77FFA` closed-gate arm (`lane_x < 0x40`) that the native bails
  (`0x78000`/`0x78003`). Owner: the bind `fifa96_match_entities.c:777` / the
  native `+0x69`–`+0x6B` aliasing.
* **FU311-L2 (new) — `sub_0b` has no producer.** `byte[[rec+4]+0xE]`, signed;
  it sets the CX=4 lane bound (`0x7806B`/`0x78076`). Carried as FU290-L5; this
  slice adds the *impact*: it is the difference between a 48-unit and a
  302-unit window, i.e. it decides whether the penalty place could ever reach
  CX=4. Owner: `0x78068` / the `[rec+4]` descriptor's `+0xE` byte.
* **FU311-L3 (new) — the `[0x14C1D4]` bit-9 gate is runtime-written.** Fresh
  reads: static 16 B = 0, writers `0x1C9D4` (0), `0x1C9E3` (2) in
  `FUN_0001C9BC`, `0x24B98` (`BX`) in `FUN_00024B00`; the gate tests `AH & 2`,
  which needs a word ≥ 0x200 — no observed writer can produce it, but the
  `FUN_00024B00` argument domain is unproven. The port stages
  `input_range = {0,0}` (`entities.c:746` memset) and so always opens the gate.
  Owner: `FUN_0001C9BC`/`FUN_00024B00` (the `fifa96_input_range_words`
  producers) + `0x77EE8`.
* **FU311-L4 (new) — the row-`0x1E` head place is a ladder-free camera writer
  (hand-off to FU310).** Phase 9's `0x8D60F..0x8D62C` installs `0x1E` on record 0
  of the controlled team; the row-`0x1E` head then writes
  `camera = (pos + sector*0x10, pos_y + 0x38)` and calls `0x700F4`
  (`0x755A9`/`0x755C7`), with `0x38` inside the pan mid-band. The port carries
  both halves (`match_run.c:3026-3034`, `keeper.c:489-501`). Untested in the M2
  tape (no phase 9 in the transcript); owner: `0x8D60F` + `0x755C7` (FU310's
  bootstrap census).
* **FU290-L6 (carried) — the non-ladder `0x6DC88` arm is unported.** This slice
  adds its shape: `0x6DC88` installs code `0x07` deferred and writes the
  `[0x158743]` byte (`0x6DCA1`) that the `0x765C4` predicate later reads.
  Owner: `0x77FFA`/`0x6DC88`.
* **FU298 / FU299 — SETTLED (mechanically live, measured).** FU298: the
  `0x110680` permit read matches the port byte-for-byte and fired (permit 1 on
  3700 phase-2 steps). FU299: the fresh `[0x157C5D]` 3-ref census (`0x7858E`
  read, `0x785A2` clear, `0x74707` set) is exact, and the one-step pulse is
  live — 2019 gated vs 1681 ungated steps. Neither is a blocker; the ladder's
  reach is gated only by the AX prepass gate.

---

## 8. Verdict

### 8.1 GO/NO-GO

**NO-GO — the keeper ladder cannot be the phase-30 natural camera bootstrap,
and `CX=4` is not naturally reachable.** Three independent, evidenced reasons:

1. **The gate, not the call path** (§4.5): the ladder is live (2019 calls); the
   AX prepass gate (`0x77F9A`/`0x77FA5`) closes in all 1010 entries, at two
   distinct places with two distinct failing terms. `ran=0` is *not* a second
   independent blocker of the call path — it is *the* blocker, and it is a
   camera-place blocker owned by FU310.
2. **The CX=4 lane window is closed for every produced place** (§6.2): the
   window is at most 302 wide (`0x30 + 2*sub_0b`, signed byte); the qualifying
   places land 208..464 from the keeper (294..313 in the best case), and the
   port's staged `sub_0b = 0` narrows the window to 48. Natively the outcome
   depends on the unproduced `sub_0b` (FU311-L2) — the honest verdict is
   **unproven, no natural producer**; in the port it is closed.
3. **The row-`0x1B` camera write is phase-`0x0D`-only while the ladder gate is
   phase-2-only** (§6.3): even a CX=4 install writes no camera cell in the frame
   that permits it. This one is **structural** — it does not depend on `sub_0b`,
   the lane, or any unported cell.

`ran=1` alone is a **conditional GO** (it is a pure gate): the exact natural
chain is a place with `|x| < 0x420`, `|z| > 0x7B0`, `y == 0`, and the
sign/side arm — the phase-6 penalty/goal-kick class `(0,0,±0x8D0)` is the only
ported candidate, and it must survive into a phase-2 frame. **What that buys is
CX=1 → row `0x1A`** (in the port always, because of FU311-L1; natively because
`lane_x ~0x7EC` refuses both CX arms and the stage table runs), **not the
keeper band.**

### 8.2 The exact natural chain (for Stage-2)

```
[FU310] a camera place P with |P.x| < 0x420, |P.z| > 0x7B0, P.y == 0
        (port: the phase-6 penalty arm, match_run.c:3014-3016, (0,0,±0x8D0))
   and the sign arm: [0x157A4C]==0 (measured) -> sign(P.z) != sign-of-side-0
        => only the defending keeper's side passes
[FU311] phase 2 + record 0 + code 0x19 + permit 1 + no slot + [0x157C5D]!=0
        (all measured live, 2019 steps)
   -> stage_select ran=1
   -> CX=1 (lane_x < 0x20: the port's stale -1; the native's ~0x7EC would run
      the stage table instead)
   -> row 0x1A install (0x7826D, stamp 0x2C), invoke-now
   -> no camera write (row 0x1A's only camera effect is the 0x702F8 Y clamp,
      carried from handlers.c:6331)
```

**Do not chase CX=4.** If Stage-2 wants the keeper band in natural play, the
honest producer is **FU311-L4** (the phase-9 → row-`0x1E` head place at
`0x755C7`, Y = `pos_y + 0x38`, a real `0x700F4` place inside the pan mid-band),
which needs no ladder at all — and belongs in FU310's bootstrap census, not in a
"ladder reach" task.

### 8.3 What would flip the verdict

* a place inside `(0x7B0, 0x8D0)` on the keeper's own side (a *closer* cut than
  the penalty/`0x8D0` place) — then CX=4's lane term is met with a maximal
  `sub_0b`, or with `sub_0b = 0` if the place is within 48 of the keeper;
* **and** the row `0x1B` phase gate (`0x76D3D`) would still need a phase-`0x0D`
  frame afterward, so the band write would be a *second* frame's event.

Both are FU310/FU311-L2 dependencies; neither is in this slice.

---

## 9. Provenance

**Ghidra MCP** (read-only; program `/FIFA96.EXE`, explicit; scripts disabled;
`get_current_program_info` fresh: `watcom:LE:32:default`, image base 0, `.image`
overlay, 2708 functions):

* `disassemble_bytes`: `0x77E90..0x782CF` (1088 B), `0x78144..0x782E4`,
  `0x765C4..0x76624`, `0x75570..0x75610`, `0x7550C..0x7557C`,
  `0x76D28..0x76D88`, `0x76D80..0x76E10`, `0x76DF0..0x76E50`,
  `0x774D0..0x77530`, `0x759F0..0x75A30`, `0x75D40..0x75D80`,
  `0x7565A..0x756AA`, `0x75D17..0x75D77`, `0x75ED0..0x75F00`,
  `0x75F70..0x75FA0`, `0x784C0..0x785B0`, `0x8D580..0x8D5E4`,
  `0x8D5C0..0x8D640`, `0x6DC88..0x6DCCA`, `0x700F4..0x70184`,
  `0x70180..0x701F0`, `0x701EC..0x7024C`, `0x741B4..0x741C8`,
  `0x1C9C4..0x1C9F4`, `0x24B80..0x24BA8`.
* `read_memory`: `0x77E98` (20 B), `0x754E4` (40 B), `0x76D14` (20 B),
  `0x110680` (46 B), `0x1106E0` (160 B), `0x14C1D4` (16 B).
* `get_xrefs_to`: `0x77EAC` (**1**: `0x78599`), `0x157C5D` (**3**: `0x7858E`
  read, `0x785A2` clear, `0x74707` set), `0x157A83` (**112**, incl. the
  writers `0x755CF`, `0x76DBF`, `0x77568`, `0x83E40`, `0x84F63`, `0x85E4B`,
  `0x8C3BE`), `0x14C1D4` (**21**, incl. the writers `0x1C9D4`, `0x1C9E3`,
  `0x24B98`).
* `get_function_by_address`: `0x700F4` (`FUN_000700f4`,
  `0x700F4..0x702F6`), `0x6DC88` (`FUN_0006dc88`, `0x6DC88..0x6DCC7`),
  `0x782D0` (`FUN_000782d0`, `0x782D0..0x785D0`), `0x77EAC` (`FUN_00077eac`,
  `0x77EAC..0x782CD`), `0x741B4` (`FUN_000741b4`), `0x75571`/`0x76DB5` (no
  function — the jump-table band).

**Measurement** (read-only; existing binary, no rebuild): `gdb -batch` on
`build/test_engine_m2` (Debug, `-g`, unstripped), breakpoints on
`fifa96_keeper_cpu_step` / `_ladder_run` / `_stage_select` and on
`fifa96_match_entities.c:821`; scripts `/tmp/opencode/fu311_trace.gdb`,
`/tmp/opencode/fu311_bind.gdb`; logs `fu311_m2.log` (27690 lines),
`fu311_bind.log` (3700 samples). Counts: 18124 steps, 2019 ladder calls, 1010
stage_select calls, 3700 phase-2/permit samples.

**Engine sources read (read-only):** `src/fifa96_loader/fifa96_keeper_cpu.c`
(whole), `include/fifa96_loader/fifa96_keeper_cpu.h` (whole),
`src/fifa96_engine/fifa96_match_entities.c` (140-249, 700-905),
`include/fifa96_engine/fifa96_match_entities.h` (135-155, 411-440),
`src/fifa96_engine/fifa96_match_handlers.c` (500-530, 570-600, 4192-4281,
4760-4809, 6320-6345, 6414), `src/fifa96_engine/fifa96_match_run.c`
(1590-1620, 2960-3060, 3905-3950, 4380-4450, 6512-6570),
`src/fifa96_loader/fifa96_keeper.c` (470-529),
`src/fifa96_loader/fifa96_camera.c` (1150-1236),
`include/fifa96_loader/fifa96_entity_update.h` (84-123).

**Carried docs:** FU147 (§S1, leg 13), FU149, FU151 (§3.1, §2.9), FU157 (§1),
FU163, FU168 (§2.2), FU220, FU282/FU283/FU290/FU291/FU292/FU298/FU299/FU300/
FU302/FU303/FU304/FU305 (§5.2/§5.3)/FU307 (§2.2, §9.2, the review erratum, the
L7 addendum), FU309, `docs/ENGINE.md` (phase-28/29 register),
`docs/PROGRESS.md` (phase-29 close).

Status: recon complete; **this draft is the only file written**; no census
change (60/80); no golden touched; no git.

---

## Review erratum (wave review, 2026-10-21)

The frozen text above stands; only the following claims change. Fresh gdb on the
existing `build/test_engine_m2` (no rebuild) + fresh reads this review.

1. **The count window.** Quote: "the ladder **is called** on the natural tape —
   2019 times out of 18124 record-0 steps (§4.2)". Fresh gdb this review:
   `fifa96_keeper_cpu_step` = **18 124** and `fifa96_keeper_cpu_ladder_run` =
   **2 019** over the whole `build/test_engine_m2` binary, not the natural tape.
   The natural window is FU313's 12 000-step probe: **6 118** ladder calls and
   **6 118** `stage_select` calls, `ran=0` (`/tmp/opencode/fu313_sel`,
   reproduced). *Stage-2 consequence:* the call-path verdict stands and is
   stronger on the natural window; the `tracked_present == 0` generalization
   (§2.5/§4.4) is not measured on the natural window.

2. **The lane defect is a family, and it is not "stale".** Quote: "The bind
   passes `e->lane`, whose high word is never re-synced from `lane_x` (only the
   low word is masked, `:361`). Measured: `e->lane >> 16 == -1` on every one of
   3700 phase-2 samples while `e->lane_x` runs 0..3176 (§4.3). So the port's
   `lane_x` is a stale `-1`, not the live band." `e->lane` is the
   **sign-extended +0x69 word** (`include/fifa96_engine/fifa96_match_entities.h:148`;
   every writer stores a word: `match_run.c:1327/1527`, `handlers.c:3946`,
   `fifa96_arm_bodies.c:70/154/269/550`), so `>>16` is 0/-1 **by construction**
   and can never be `lane_x`; the bind log (`/tmp/opencode/fu311_bind.log`:
   `lane=-2678 lanehi=-1 lane_x=3144`) confirms. The same high-word read recurs
   at `handlers.c:240/246/1683/1887/2037/2183/3701/4546/4823/5176/5900` and
   `match_run.c:915`; `handlers.c:972` feeds the **kick** gate `lane_word` the
   low word where native `0x81519` reads the high word. *Stage-2 consequence:*
   the fix is a class fix (route every `+0x6B` read to `lane_x`; keep `lane` as
   `+0x69`), not a one-line bind patch; **FU311-L5**.

3. **The row-0x1E "ladder-free" claim.** Quote: "**FU311-L4 (new) — the
   row-`0x1E` head place is a ladder-free camera writer (hand-off to FU310).**"
   The head place is real (`0x755A9`/`0x755C7`, Y = `pos_y + 0x38`, fresh), but
   it is **ladder-downstream**: phase 9 ← situation 5, and situation 5's only
   producers are row 0x1A stage 4 (`0x76B1C`) and row 0x1B (`0x77707`), both
   installed by the ladder (0x1A: only `0x7821D/0x78245/0x7826D`; 0x1B: the
   CX=4 arm `0x78294` or the row-0x1F dive `0x76557` at phase 6). No qualifying
   place → no ladder → no 0x1A/0x1B → no sit 5 → no phase 9 → no 0x1E (FU163
   §5's two-site census). *Stage-2 consequence:* FU311-L4 is a dispatch-shape
   fact, not a bootstrap; §8.2's "the honest producer is FU311-L4" is refused.

4. **Phase 0x0D is writable, but ladder-downstream.** Quote: "Only a later
   frame that reaches phase `0x0D` with the record still on row `0x1B` could
   place it — and phase `0x0D`'s producer (situation 7, `handlers.c:6414`) is
   unported (`FU261-L6`, carried)." The port **does** write phase 0x0D from the
   row-0x1A sit-7 emit (`handlers.c:4647`, `set_piece(7)` on the session-0
   direct path), but row 0x1A is ladder-installed; the row-0x1B sit-7 tail
   (`0x77589`) is skipped (`FU285-L8`). The structural NO-GO stands; the
   "unported producer" wording is stale. *Stage-2 consequence:* the register's
   `handlers.c:6414` text needs the quoted erratum at promotion.

5. **The depth numbers.** Quote: "**FU307-L1 — the natural-tape CX=4 reach:
   NO-GO (this slice).** ... the modal keeper depth (0x700..0x800, 2258/3700
   samples)". The verdict stands on the structural phase mismatch + the lane
   defect + the absent place; the 2258/3700 distribution is the whole-binary
   window, not the natural tape. *Stage-2 consequence:* keep the NO-GO; treat
   the depth arithmetic as corroborative only.
