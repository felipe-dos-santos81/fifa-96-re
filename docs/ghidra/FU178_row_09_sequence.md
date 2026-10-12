# FU-178 — row 09 sequence/steer body: stage-0 slice + the `0x809F0` arm map

Provenance: M2 phase-15 wave **R26**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every address/byte/call claim below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` of this session unless marked *cross-cited*
(then the frozen FU doc is named). No writes: no rename, comment, label,
function, script or project save; `/FIFA96.EXE` untouched; no git commands. The
alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` = byte at
`x+3`; e.g. `[EBP+0x3A]>>24` is the byte at `EBP+0x3D`).

Scope (bounded per the brief): derive the row-09 **stage-0 slice**
(`0x80A00..0x80BFA`) and the **`0x809F0` 4-arm table boundary**, locate the
second-order **0x0D install at `0x80F61`** (and the fresh sibling **0x0C install
at `0x80EFA`**), map the four arms, and rank them. The arm bodies (stages 1–3)
are *not* derived — they are the numbered legs. No arm is guessed and no
constant is invented.

Cross-cited prior work: FU-136 row-09 ("not ported (partial); arms unported",
T16-C), FU-81 §2/§2.1 (the stage-family chain model, the `0x809F0` table), FU-138
§2 (the `0x80A87..0x80ADB` stage-0 sketch), FU-174 §2/§4 (two live installers;
rank 3).

---

## 1. Fresh table + boundary re-verification

* **Action table `0x1106E0`** — fresh `read_memory` (180 B) returns all 45
  dwords `0x00..0x2C`; slot **`[9] = 0x080A00`** (bytes at offset `0x24`:
  `00 0a 08 00`). The table ends at `0x110794` (45 × 4 = `0xB4`), where the
  phase table begins — matching FU-174 §1 first-hand.
* **Row-09 handler `0x80A00`** — fresh `get_xrefs_to` = **1** site, a `DATA`
  reference *from* `0x110704` (the action-table slot-9 dword). The row is
  reached only through the table/`[rec+0x18]` dispatch — there is no direct
  `CALL 0x80A00` anywhere.
* **Arm table `0x809F0`** — fresh `read_memory` (16 B) =
  `{0x80A3F, 0x80BFB, 0x80FCC, 0x8103A}` (`3f0a0800 fb0b0800 cc0f0800
  3a100800`), exactly FU-81 §2's `{0x70A3F,0x70BFB,0x70FCC,0x7103A}` resolved.
* **Body extent** — the shared tail epilogue is at `0x8105C..0x81065`, ending
  `0x81065 RET` (`c3`); the next instruction `0x81066 MOV EAX,EAX` (`8bc0`) is
  padding, then row **08** begins at `0x81068 PUSH EBX` (`53`, fresh). So the
  row-09 body is `0x80A00..0x81066` (FU-81's `..0x81067` bound is the same RET
  with the trailing pad byte).

The row's reachability today (fresh install sites, §5) is unchanged from FU-174:
the row is **UNSUP** in the engine (its `fn` is NULL), so its body never runs;
stage 0 below is what a landing would run first.

---

## 2. The install contract that seeds the row (`FUN_0007D9A4`)

Fresh `disassemble_bytes` of the installer head (`0x7D9A4`) and the store block
(`0x7DA63..0x7DA9B`) confirm the contract (EAX = record, `DX` = code, `BX` =
arg/stage, `ECX` = invoke-now flag):

* `0x7D9A9 MOV ESI,EAX`; `0x7D9AB MOV word[ESP+4],DX`; `0x7D9B0 MOV word[ESP],BX`.
* `0x7DA67 MOV byte[ESI+0x91],AL` — code store (`AL` = low byte of `DX`).
* `0x7DA74 SHL EAX,0x2; 0x7DA77 ADD EAX,0x1106E0; 0x7DA7C MOV EAX,[EAX];
  0x7DA8F MOV [ESI+0x18],EAX` — **`[rec+0x18] = 0x1106E0[code]`** (the row
  handler pointer; for code 9 this is `0x80A00`).
* `0x7DA7E MOV dword[ESI+0x89],0` (timer); `0x7DA88 MOV byte[ESI+0x9E],0`;
  `0x7DA92 MOV AL,[ESP]; 0x7DA95 MOV byte[ESI+0x92],AL` — **the stage byte
  `+0x92` is set from the install arg `BX`**. Installing code 9 with `BX=0`
  therefore enters the row at **stage 0**.

So the row is entered through the table pointer with `EAX = rec`; the phase gate
and stage dispatch below are the first thing it does.

---

## 3. Stage-0 slice (`0x80A00..0x80BFA`)

All bytes below are fresh this session. The arm-0 (stage 0) block is
`0x80A3F..0x80BFA`; it ends with a stage advance (so the arm is a full
one-shot step, not a waiter).

### 3.1 Prologue + phase gate + stage dispatch (`0x80A00..0x80A37`)

```
0x80A00  PUSH EBX/ECX/EDX/ESI/EDI/EBP ; 0x80A06 SUB ESP,0x10 ; 0x80A09 MOV EBP,EAX
0x80A0B  MOV EAX,[0x157A4A]          ; the phase dword
0x80A10  SAR EAX,0x18                ; phase = high byte
0x80A13  CMP EAX,0x2   (83 f8 02)
0x80A16  JZ  0x80A24                 ; phase==2 -> stage dispatch
0x80A18  MOV EAX,EBP ; 0x80A1A CALL 0x7DAB4   ; RESET/chooser
0x80A1F  JMP 0x8105C                 ; shared tail
0x80A24  MOV AL,[EBP+0x92]           ; stage byte
0x80A2A  CMP AL,0x3    (3c 03)
0x80A2C  JA  0x8105C                 ; stage > 3 -> tail
0x80A32  AND EAX,0xFF
0x80A37  JMP dword CS:[EAX*4+0x809F0]
```

`FUN_0007DAB4` (fresh head `0x7DAB4`) is the RESET chooser: it writes
`byte[+0x92] = 0xFF`, `dword[+0x89] = 0`, calls `0x78B00` on `[+0x20]` (the
anim) when present, then re-reads the phase and tests `==2`. This matches
FU-81 §2's "RESET = the chooser FU-76 §1.1".

### 3.2 Arm 0 clear branch (`0x80A3F..0x80A86`, `[+0x8D]==0`)

```
0x80A3F  CMP byte[EBP+0x8D],0   (80 bd 8d 00 00 00 00)
0x80A46  JNZ 0x80A87            ; active -> set branch
0x80A48  MOV EAX,EBP ; 0x80A4A CALL 0x7DAB4        ; RESET
0x80A4F  MOV EAX,[EBP]          ; team base
0x80A52  CMP EBP,[EAX+0x7B2]
0x80A58  JNZ 0x80A64
0x80A5A  MOV dword[EAX+0x7B2],0                     ; clear controlled slot
0x80A64  MOV EAX,[EBP]
0x80A67  CMP EBP,[EAX+0x7B6]
0x80A6D  JNZ 0x8105C            ; not the +0x7B6 slot -> tail
0x80A73  MOV dword[EAX+0x7B6],0
0x80A7D..0x80A86  ADD ESP,0x10 ; POP EBP/EDI/ESI/EDX/ECX/EBX ; RET
```

So a non-active record simply RESETs and detaches itself from the team's
`+0x7B2`/`+0x7B6` slots (the same two slots the stage-0 set branch and the
phase-6 keeper arm use).

### 3.3 Arm 0 set branch (`0x80A87..0x80BFA`, `[+0x8D]!=0`) — the steer step

**Target + face (`0x80A87..0x80AB0`).**

```
0x80A87  IMUL AX,[0x1577C0],0xA   ; (666b05 c0771500 0a)
0x80A8F  IMUL DX,[0x1577C2],0xA   ; (666b15 c2771500 0a)
0x80A97  MOV BX,[EBP+0x6F]        ; z offset (word)
0x80A9B  MOV SI,[EBP+0x6D]        ; x offset (word)
0x80A9F  ADD EDX,EBX              ; z*10 + z_off
0x80AA1  ADD EAX,ESI              ; x*10 + x_off
0x80AA3  MOVSX EBX,DX ; 0x80AA6 MOVSX EDX,AX
0x80AA9  MOV EAX,EBP ; 0x80AAB CALL 0x79C50
```

The lead words `[0x1577C0]`/`[0x1577C2]` (fresh reads: this is the ball-relative
lead pair FU-138 §2 named) are scaled ×0xA and added to the record's own
`+0x6D` (x) / `+0x6F` (z) offsets; the result is handed to the face helper
`0x79C50(rec, x=EDX, z=EBX)`.

**Angle fold through `0x114E04` (`0x80AB0..0x80AE6`).**

```
0x80AB0  MOV EAX,[EBP+0x69] ; 0x80AB3 SAR EAX,0x11   ; >>17
0x80AB6  MOV ESI,EAX ; 0x80AB8 CWDE ; 0x80AB9 CMP EAX,0x30
0x80ABC  JLE 0x80AC3 ; 0x80ABE MOV ESI,0x30           ; min(>>17, 0x30)
0x80AC3  MOV EAX,[EBP+0x7B] ; 0x80AC6 SAR EAX,0x10    ; heading word
0x80AC9  SHL AH,0x7 ; 0x80ACC SBB EDX,EDX ; 0x80ACE ADD AH,AH
0x80AD0  SBB ECX,ECX ; 0x80AD2 XOR EAX,ECX ; 0x80AD4 AND EAX,0xFF
0x80AD9  SUB EAX,ECX                                   ; |x| sign-fold
0x80ADB  MOV EAX,[EAX*4+0x114E04]                      ; angle table
0x80AE2  XOR EAX,EDX ; 0x80AE4 SUB EAX,EDX             ; restore sign
```

This is the exact `min(dword[+0x69]>>17, 0x30)` + `0x114E04` fold FU-138 §2
quoted at `0x80A87..0x80ADB`; the fold is applied to **both** the heading word
`+0x7B` (here) and, again, the `+0x7B`-derived term at `0x80AE8..0x80B17`. The
sign-fold idiom (`SHL AH,7; SBB; ADD AH,AH; SBB; XOR; SUB`) is the classic
abs-with-sign-restore.

**Velocity solve + state commit (`0x80AE6..0x80BFA`).**

```
0x80AE6  MOV EBX,EAX           ; folded angle (z-term)
0x80AE8  MOV EAX,[EBP+0x7B] ; 0x80AEE SAR EAX,0x10
0x80AF4  LEA ECX,[EAX+0x100]   ; second angle index (+0x100)
0x80AFA..0x80B0C  same SHL/SBB/XOR/SUB fold ; MOV EAX,[EAX*4+0x114E04]
0x80B13  XOR EAX,EDX ; 0x80B15 SUB EAX,EDX
0x80B17  MOV ECX,EAX ; 0x80B19 MOV EDX,EBX ; 0x80B1B MOV EAX,ESI
0x80B1D  CALL 0x795A4                ; rotate()
0x80B22  MOVSX ECX,CX ; 0x80B25 CWDE
0x80B26  MOV EDI,[EBP+0x59] ; 0x80B2B ADD EDI,EAX ; 0x80B2F MOV [EBP+0x59],EDI
0x80B2D  MOV EAX,ESI ; 0x80B32 CALL 0x795A4 ; 0x80B37 CWDE
0x80B38  MOV EDX,[EBP+0x61] ; 0x80B3B ADD EDX,EAX ; 0x80B49 MOV [EBP+0x61],EDX
0x80B3D  MOV AX,[EBP+0x79] ; 0x80B41 ADD EAX,EAX ; 0x80B43 MOV [EBP+0x7B],AX
0x80B47  MOV EAX,EBP ; 0x80B4C CALL 0x79B58
0x80B51..0x80B6F  three CALL 0x795A4 -> word[+0x73], word[+0x75]
0x80B73  MOV EDX,[EBP+0x73] ; 0x80B76 MOV EAX,[EBP+0x71]
0x80B79/0x80B7C  SAR EDX/EAX,0x10 ; 0x80B7F CALL 0x8DC68   ; distance()
0x80B84  LEA EDI,[EBP+0x4D] ; 0x80B87 LEA ESI,[EBP+0x59]
0x80B8A  MOV word[EBP+0x71],AX          ; speed
0x80B8E..0x80B90  MOVSD x3             ; +0x59 triple -> +0x4D
0x80B91  MOV EAX,[EBP+0x71] ; 0x80B94 SAR EAX,0x10 ; 0x80B99 MOV EBX,[EBP+0x4D]
0x80B9A  SHL EAX,0x3 ; 0x80B9D ADD EBX,EAX ; 0x80BAB MOV [EBP+0x4D],EBX
0x80B9F  MOV EAX,[EBP+0x73] ; 0x80BA2 SAR EAX,0x10 ; 0x80BA5 MOV ECX,[EBP+0x55]
0x80BA8  SHL EAX,0x3 ; 0x80BAE ADD ECX,EAX ; 0x80BB0 MOV EDX,0x11
0x80BB8  XOR EBX,EBX ; 0x80BBA MOV ECX,[EBP+0x8B] ; 0x80BC2 SAR ECX,0x18
0x80BC0  MOV EAX,EBP ; 0x80BC5 CALL 0x6E598        ; animation
0x80BCA  XOR CL,CL ; 0x80BCC MOV byte[EBP+0x9E],1
0x80BD3  MOV byte[0x15877C],CL                     ; clear sequence flag
0x80BD9  MOV dword[EBP+0x89],0 ; 0x80BE9 INC CH
0x80BDF  MOV CH,[EBP+0x92] ; 0x80BEB MOV [EBP+0x92],CH   ; stage++
0x80BF1..0x80BFA  epilogue (ADD ESP,0x10; POP×6; RET)
```

So stage 0 with an active record: solve the ball-relative heading, fold it
through `0x114E04`, rotate via `0x795A4`, accumulate velocity into `+0x59`
(x), `+0x61` (z), set the heading word `+0x7B = +0x79*2`, derive the velocity
pair `+0x73/+0x75` and speed `+0x71` (via `0x8DC68`), copy the `+0x59` position
triple to the placement `+0x4D`, add `speed<<3` / `vel<<3` into `+0x4D/+0x55`,
fire the animation `0x6E598`, set the ran-byte `+0x9E = 1`, clear the
`[0x15877C]` flag, zero `+0x89`, and **advance `+0x92` (stage 0 → stage 1)**.
The arm does *not* touch `[0x15877C]` to 1; it clears it (writes observed at
`0x80BD3`).

---

## 4. The `0x809F0` arm map (the four stages)

Boundaries verified by fresh `disassemble_bytes` at each table entry (each entry
is a real instruction start). Table `{0x80A3F, 0x80BFB, 0x80FCC, 0x8103A}`:

| arm | stage | range | entry bytes | character (fresh head) | second-order |
|---|---|---|---|---|---|
| **0** | 0 | `0x80A3F..0x80BFA` | `80 bd 8d 00 00 00 00` `CMP byte[EBP+0x8D],0` | full steer step: `+0x8D` clear → RESET + detach `+0x7B2/+0x7B6`; set → target/face/`0x114E04` fold/`0x795A4`/velocity/commit, then **stage++** | — |
| **1** | 1 | `0x80BFB..0x80FCB` | `80 3d 7c 87 15 00 00` `CMP byte[0x15877C],0` | gate on the `[0x15877C]` sequence flag; if set, RNG roll `0x92AC8 & 0x3F` vs `3*byte[[+0x4]+0x16]`; emits events `0x5D`/`0x60` via `0x8F188`; **installs 0x0C on ESI at `0x80EFA` and 0x0D on ESI at `0x80F61`** | **0x0C and 0x0D** |
| **2** | 2 | `0x80FCC..0x81039` | `30 f6` `XOR DH,DH` | clears `[0x15877C]`; template byte `[[EBP+0x28]]==0x11` gate; if the record is the `[team+0x7B2]` slot, nearest-select `0x8DE8C` writes `[team+0x7B2]`; else stage++ | — |
| **3** | 3 | `0x8103A..0x8105B` | `89 e8` `MOV EAX,EBP` | finish: `CALL 0x79B1C`; if `byte[+0x44]!=0` → RESET `0x7DAB4`, and if `rec != [0x1577CA]` then `INC byte[0x15872F]`; falls to tail | — |

The shared tail `0x8105C..0x81065` restores the six registers and RETs. Stage
`>3` (`0x80A2C JA 0x8105C`) also falls straight to the tail.

### 4.1 The second-order installs (fresh)

`get_xrefs_to 0x7D9A4` = **77** sites this session (matches FU-174 §1); two are
inside row 09's arm 1:

* **`0x80EFA`** — `0x80EEC MOV ECX,1; 0x80EF1 MOV EDX,0xC; 0x80EF6 MOV EAX,ESI;
  0x80EF8 XOR EBX,EBX; 0x80EFA CALL 0x7D9A4` → **installs code `0x0C` on ESI
  (invoke-now)**. (Fresh; FU-174 §2 named only the 0x0D install.)
* **`0x80F61`** — `0x80F5C CALL 0x8F188` (emit event `0x5D`, `EAX=0x5D`,
  `EDX=EBP`, `ECX=1`); `0x80F61 MOV EDX,0xD; 0x80F66 MOV EAX,ESI;
  0x80F68 XOR EBX,EBX; 0x80F6A CALL 0x7D9A4` → **installs code `0x0D` on ESI**
  (the brief's target; confirmed verbatim). The alternate branch `0x80F71`
  emits event `0x60` (`EAX=0x60`, `EBX=4`, `EDX=ESI`) instead.

Both installs live in **arm 1 (stage 1)** and fire on a record **ESI** that is
not `EBP` (the `0x80F41/0x80F46` compare `EBP` vs `[ESI+0x24]`). So row 09's
second-order installs are reachable **only after stage 0 advances to stage 1**
— i.e. behind `0x80BFB`.

---

## 5. The live installers of code 9 (re-verified, supports the wiring rec)

Two native sites install code 9; both were re-read fresh this session to
confirm the FU-174 §2 attribution:

* **Pad-release / input-row edge** — `0x7CFA9 MOV EDX,0x9; 0x7CFAE MOV EAX,ESI;
  0x7CFB0 XOR EBX,EBX; 0x7CFB2 CALL 0x7D9A4` (within the input-row handler
  `0x7CF54`; guarded by `0x7CF9C CMP AL,[EDX+0x826] / 0x7CFA2 JZ 0x7CFB7`).
* **Row-06 RNG arm** — `0x807A2 MOV word[ESP+0x50],0x9; 0x807A9 CMP word[ESP+0x50],0;
  0x807B1 MOV EDX,[ESP+0x4e]; 0x807BE SAR EDX,0x10; 0x807C1 CALL 0x7D9A4`
  (installs the staged word on `EAX=EBP`). The row-06 body is a **wired** row
  (FU-138 §2), so this installer is live.

The engine's live reach of these two sites (the input-row seam
`match_run.c:718..729` and the wired row-06 `out.install` drain
`handlers.c:2921`) is *cross-cited* FU-174 §2 — not re-derived here.

---

## 6. Arm ranking (for the bounded T3 wiring)

Ranked by port cost × dependency × how deep stage 0 reaches. Only **arm 0** is
derived in §3; arms 1–3 are the legs of §7.

1. **Arm 0 (stage 0, `0x80A3F..0x80BFA`) — the wire target.** The entry arm
   (install seeds `+0x92=0`), fully derived above, resting on the FU-138 pure
   helpers (`place`/`face`) already ported+tested. It is the "deepest provable
   stage-0" the brief names. **Wire this.**
2. **Arm 3 (stage 3, `0x8103A..0x8105B`) — cheapest remainder.** ~34 bytes:
   `0x79B1C` + a `+0x44`-gated RESET + a one-byte counter `[0x15872F]++`.
   Smallest honest follow-on.
3. **Arm 2 (stage 2, `0x80FCC..0x81039`) — medium.** Clears `[0x15877C]`,
   template `0x11` gate, nearest-select `0x8DE8C` writing `[team+0x7B2]`.
4. **Arm 1 (stage 1, `0x80BFB..0x80FCB`) — heaviest.** Holds the two
   second-order installs (0x0C, 0x0D), the `[0x15877C]` sequence-flag
   semantics, an RNG gate (`0x92AC8`), and two event emits (`0x8F188`). Highest
   dependency; last to land.

Because stage 0 **advances** `+0x92` to 1, a stage-0-only landing must decide
what the engine does on the next dispatch (stage 1 = arm 1, unported). See §7
T3 rec and FU178-L1.

---

## 7. Numbered legs

1. **FU178-L1 — arms 1/2/3 bodies (stages 1–3) unported.** `0x80BFB..0x80FCB`,
   `0x80FCC..0x81039`, `0x8103A..0x8105B`. Arm 1's `[0x15877C]` gate, RNG
   `0x92AC8`, `[EBP+0x4]`/`+0x13>>24` chain, the `0x8A3FC`/`0x8F188` event
   emits and the 0x0C/0x0D installs are inside it. **Stage 0 advances here**, so
   a stage-0-only port must define stage-1 dispatch (bounded cue: return the row
   to RESET, or no-op; native falls through only for stage>3).
2. **FU178-L2 — the engine installers of code 9.** Input-row pad edge
   (`0x7CFB2`) + wired row-06 arm (`0x807C1`): native sites fresh here; the
   engine paths (`match_run.c:718..729`, `handlers.c:2921`) are *cross-cited*
   FU-174 §2. Re-derive before wiring if the engine paths move.
3. **FU178-L3 — the helper bodies.** `0x79C50` (face), `0x795A4` (rotate),
   `0x79B58`, `0x79B1C`, `0x8DC68` (distance), `0x6E598` (anim), `0x8F188`
   (ring/event). Called by the stage-0 slice; not derived here.
4. **FU178-L4 — the `+0x114E04` angle table and the `+0x1577C0/+0x1577C2`
   lead words.** Steps/contents not read; the fold indices (`[+0x7B]>>16` and
   `+0x100+…`) are the FU-81/FU-138 shape only.
5. **FU178-L5 — arm-2 nearest-select (`0x8DE8C`).** Writes `[team+0x7B2]`;
   body not derived (leg).
6. **FU178-L6 — `[0x15877C]` shared flag.** Fresh `get_xrefs_to` = **7**
   (`0x80BD3`/`0x80DCC`/`0x80FD1` writes; `0x80BFB`/`0x80FA4` reads;
   `0x839AB`/`0x83AE0` reads in `FUN_00083428`). Semantics (the "sequence
   armed" flag row 09 sets/clears and `FUN_00083428` consumes) is a leg.
7. **FU178-L7 — the alias-flagged reads.** `0x80C35 EAX=[EBP+0x3A]; SAR 24`
   is byte `EBP+0x3D`; `0x80F24 EDX=[[EBP+0x4]+0x13]; SAR 24` is byte
   `[[EBP+0x4]+0x16]`. Both are arm-1 internals (leg).

---

## 8. T3 wiring recommendation (bounded)

**Wire row 09 as a stage-0-only handler.** Add a non-NULL `fn` for action row
`09` in the engine action table and implement `fifa96_match_action_09` running
the §3 stage-0 set/clear branch over `mr->record`: the phase-2 gate
(`[0x157A4A]>>24 == 2` else RESET), the `+0x8D` branch (clear → RESET + detach
`team+0x7B2/+0x7B6`; set → ball-relative target `[0x1577C0/0x1577C2]*0xA` +
`+0x6D/+0x6F`, face, `0x114E04` fold, `0x795A4` velocity/`0x8DC68` speed,
commit `+0x59/+0x61/+0x71/+0x73/+0x75/+0x7B`, `+0x9E=1`, `[0x15877C]=0`,
`+0x89=0`, `+0x92++`). Route through the same installer seam the two live
installers (`0x7CFB2`, `0x807C1`) already reach. Stages 1–3 stay numbered legs
(FU178-L1); the handler must **not** silently no-op a stage it does not
implement — mirror the native `stage>3 → tail` only for stage>3, and treat
stage 1 (the arm-1 installs) as an explicit unported path. This is the deepest
provable stage-0; the second-order 0x0C/0x0D installs land only when arm 1
lands.

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `read_memory` — `0x1106E0` (180 B, full 45-dword action table), `0x809F0`
  (16 B, the 4-arm table).
* `get_xrefs_to` — `0x80A00` (1, DATA from `0x110704`), `0x7D9A4` (77),
  `0x15877C` (7).
* `disassemble_bytes` — `0x80A00` (256 B: prologue/gate/dispatch/arm-0 head),
  `0x80ADB` (288 B: arm-0 set-branch tail → arm-1 head), `0x80BFB` (80 B,
  arm 1 head), `0x80EE0` (48 B, the 0x0C install), `0x80F50` (64 B, the 0x0D
  install), `0x80F00` (113 B, arm-1 RNG gate + install region), `0x80FCC`
  (80 B, arm 2 head), `0x81012` (64 B, arm 2 tail → arm 3), `0x8103A` (64 B,
  arm 3 + shared tail + row-08 head), `0x7D9A4` (48 B, installer head),
  `0x7DA50` (32 B) and `0x7DA6D` (48 B, the code/`+0x18`/stage stores),
  `0x7DAB4` (48 B, RESET chooser head), `0x807A0` (48 B, row-06 install 9),
  `0x7CF9C` (40 B, input-row install 9).

Docs cross-cited: FU-136 row-09 (T16-C "arms unported"), FU-81 §2/§2.1 (the
`0x809F0` table + chain model), FU-138 §2 (the stage-0 sketch
`0x80A87..0x80ADB`), FU-174 §1/§2/§4 (census, the two live installers, rank 3).
No writes beyond this slice file and the workspace report.

---

## 10. Landing note — M2 phase-15 T3 (2026-10-12)

Applied (bounded, per §8): `fifa96_match_action_09` (`fifa96_match_handlers.c`)
is the **stage-0-only** landing — the phase-2 gate (`0x80A0B`, else the
`0x7DAB4` reset), the stage `> 3` tail (`0x80A2A`), and the **arm 0**
(`0x80A3F..0x80BFA`) clear/set branches. The row-09 action-table slot is now
non-NULL and the row is **counted**: its two installers are live (the pad
release edge `0x7CFB2`, the wired row-06 RNG arm `0x807C1`). Fresh
`disassemble_bytes` this slice confirmed the table/arm boundaries
(`0x809F0 = {0x80A3F, 0x80BFB, 0x80FCC, 0x8103A}`, `0x1106E0[9] = 0x080A00`)
and every arm-0 byte. The arm-0 set branch ports the two `0x114E04` sine folds
as `fifa96_entity_sine(face7d)` / `fifa96_entity_sine(face7d+0x100)` fed to a
local `0x795A4` multiply port (`match_row09_mul`) — verified equivalent to the
native `SHL AH,7; SBB; ADD AH,AH; SBB; XOR; SUB` fold (the sign bit and the
mirror bit agree for the sign-extended 10-bit angle). **Erratum to §3.3:** the
second `0x795A4` call at `0x80B32` multiplies by `(int16)t1` (ECX is
overwritten at `0x80B22`), not by the second sine — the port reproduces that
literally. Unmodelled cells staged zero with disclosed producers: the
`0x1577C0/C2` lead words (OL-82), `word[+0x79]` (the `+0x7B` source) and the
`[0x15877C]` shared flag (FU178-L6, write-only here).

**Arms 1/2/3 (stages 1–3) stay FU178-L1.** The handler parks at stage 1/2/3 and
records the reached-but-unported stage in `mr->match09_leg_stage` (observability
— never a silent no-op). The `0x0D`/`0x0C` installs at `0x80F61`/`0x80EFA` are
inside arm 1, so **row 0x0D's action-table slot stays NULL** and it is
**uncounted** until arm 1 lands (the rows-27/29/2C precedent); its body is
ported separately as `fifa96_action_sequence_0d_step` (FU-180 §9).
