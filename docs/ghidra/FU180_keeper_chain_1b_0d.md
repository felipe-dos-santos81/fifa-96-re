# FU-180 — second-order keeper chain: the 0x1B and 0x0D installs

Provenance: M2 phase-15 wave **R28**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; `get_current_program_info` = `FIFA96.EXE`, watcom LE 32, image base 0,
one overlay `.image`). Every address/byte below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `get_function_by_address` /
`search_instructions` of this session unless marked *cross-cited* (the frozen FU
doc is named). Engine/loader source read first-hand (`fifa96_keeper.c`,
`fifa96_match_handlers.c`, `fifa96_action_handlers.c`). No writes: no rename,
comment, label, function, script or project save; `/FIFA96.EXE` untouched; no
git commands. The alias traps are applied (`dword[x]>>16` = word at `x+2`,
`>>24` = byte at `x+3`; here `[rec+0x8E]>>24` = the action code at `+0x91`,
`[rec+0x8B]>>24` = the sector at `+0x8E`).

Scope (bounded, per the R28 brief): the **two second-order installs** phase-15's
new rows arm — the **0x1B** keeper-reposition install (`0x76557`, row-1F
stage 2) and the **0x0D** event-transition install (`0x80F61`, row-09) — plus
the keeper-chain upstream they draw on (`FUN_000782D0` record-0 machine,
`FUN_00076130`, `FUN_00077EAC`, the `0x76B28` steering; OL-34/OL-35). The
question is what must exist for each install to *execute honestly* (a native
installer invocation the engine already reaches) versus what is deep chain and
becomes a numbered leg.

---

## 1. The two installs (fresh this session)

### 1.1 The 0x1B install — row-0x1F stage-2 no-slot RNG dive (`0x76557`)

Fresh `disassemble_bytes 0x764A0` (200 B) — the tail of row `0x1F`
(`0x76380`), the installer arm:

```
0x764B0  CMP dword[EBP+0x20], 0x0        ; control slot?
0x764B4  JNZ 0x76568                     ; slot -> stage-3 tail (no 0x1B)
0x764BA  LEA EDI,[EBP+0x4D]; LEA ESI,[EBP+0x59]; MOVSD x3   ; pos -> target
0x764C3..0x76508  weighted timer from [+0x4]/[+0x10] desc bytes; RNG 0x92AC8
                  CMP EDX,0x19; word[0x15773A]>0 ? SETG ; else RNG&1 -> EDX
0x76508  TEST DX,DX; JNZ 0x7650E; DEC EDX            ; sign fold
0x7650E  EAX=[EBP+0x61]; ECX=&[EBP+0x55]; EAX>0 ? -0x30 : 0x30; ADD [ECX],EAX
0x76526  RNG 0x92AC8 & 0x3F; +0x30; MOVSX EDX,DX; IMUL EDX,EAX
0x7653C  MOV [EBP+0x4D], EDX            ; target.x = sign * (rng&0x3F + 0x30)
0x76541  RNG 0x92AC8; DIV 0x30; EDX += 0x20
0x76548  MOV ECX,0x1                    ; invoke-now
0x76550  MOV EAX,EBP                    ; rec
0x76552  MOV EBX,ECX
0x76554  MOV [EBP+0x51], EDX            ; target.y = (rng % 0x30) + 0x20
0x76557  MOV EDX,0x1B                   ;  ba 1b 00 00 00
0x7655C  CALL 0x7D9A4                   ;  e8 43 74 00 00
```

So the row-1F no-slot dive arm **pre-seeds `+0x4D` (target.x) and `+0x51`
(target.y) and then installs code 0x1B invoke-now** (`ECX=1`). This is exactly
the ported `fifa96_keeper_dive_target` (`fifa96_keeper.c:165`, `install_code =
0x1B`, `invoke = 1`) + the `fifa96_keeper_arm_step` stage-2 no-slot branch
(`fifa96_keeper.c:214`). The bytes confirm FU-174 §2's "row-1F install 0x1B"
and FU-140 OL-36's "no slot → RNG dive install 0x1B invoke-now".

Fresh `search_instructions MOV EDX,0x1b` = **15**; the two match-core installs
into `0x7D9A4` are:

| site | context | ECX |
|---|---|---|
| `0x76557` → `0x7655C` | row-0x1F stage-2 no-slot dive | **1** (invoke-now) |
| `0x78299` → `0x782A2` | `FUN_00077EAC` ladder (§4.3) | **1** (invoke-now) |

(The other 13 are unrelated `0x1B4/0x1B7/0x1B8` immediates or non-`0x7D9A4`
calls.)

### 1.2 The 0x0D install — row-09 body (`0x80F61`)

Fresh `disassemble_bytes 0x80F50` (48 B):

```
0x80F50  MOV EAX,0x5D; MOV EDX,EBP; MOV ECX,0x1; CALL 0x8F188   ; event ring 0x5D
0x80F61  MOV EDX,0x0D                   ;  ba 0d 00 00 00
0x80F66  MOV EAX,ESI
0x80F68  XOR EBX,EBX                    ;  no invoke
0x80F6A  CALL 0x7D9A4                   ;  e8 35 ca ff ff
```

So the sole match-core installer of 0x0D is the **row-09 body** (`0x80A00`),
install-only (`EBX=0`). Fresh `search_instructions MOV EDX,0xd` = 21; the match
core has one other `0x0D` immediate at `0x7B0A7`, but its next instruction is
`CALL 0x928F0` (`disassemble_bytes 0x7B095`, fresh) — **not** `0x7D9A4`, so it
is an argument, not an install. **0x0D's only installer is row-09.** No input
row installs 0x0D (the input-row handlers install 9/0x0B; FU-174 §2).

Fresh `get_xrefs_to 0x7D9A4` = **77** (matches FU-174 §1 exactly); `0x7655C`
and `0x80F6A` are both in the fresh list.

---

## 2. The 0x1B body — row 0x1B `0x76D28` (5-arm machine)

Fresh `disassemble_bytes 0x76D14` (160 B) + `read_memory 0x76D14` (20 B):

```
0x76D14 = {0x76DF6, 0x76E3B, 0x76FC7, 0x7769F, 0x776C1}   ; 5 arms, stage 0..4
0x76D28  PUSH EBX..EBP; SUB ESP,0x14; MOV EBP,EAX            ; EBP = rec
0x76D31  CL = [0x157A4A]>>24 (= phase); if phase != 0xD -> 0x76DC5
0x76D46  EAX=[rec]; AL=byte[[rec]+0x826]; EDX=[0x157AAC]>>24
0x76D5D  if (side != [0x157AAC]>>24) -> 0x76DC5             ; side-match gate
0x76D61  sector = [rec+0x8B]>>24 (byte +0x8E)
0x76D6A  EBX = (int8)0x10F331[sector]<<4; [0x15774C] = rec+0x59 + EBX
0x76D81  EDX = (int8)0x10F339[sector]<<4; [0x157754] = rec+0x61 + EDX
0x76D9F  [0x157750] = rec+0x5D + 0x38
0x76DAE  byte[rec+0x9B] = 1                                 ; TAKE
0x76DBA  CALL 0x700F4                                       ; camera place
0x76DBF  [0x157A83] = EBP                                   ; actor bind
0x76DC5  timer +0x89 += [0x157A64]; AL = [rec+0x92] (stage)
0x76DE1  CMP AL,4; JA 0x7771E                               ; stage > 4 -> tail
0x76DEE  JMP CS:[EAX*4 + 0x76D14]
```

* **stage 0** `0x76DF6`: `byte[rec+0x9E]=1`, `[rec+0x89]=0`, stage++, clear the
  latches `0x157C54..0x157C5B`; `0x8DCD4(pos, target, +0x65)`; `0x79C50(rec,
  -[0x1577C2], -[0x1577C0])`; `0xCD474` heading → `+0x7F`; `+0x51 += 0x20`;
  then the **sine-table rotation** (`0x114E04` = flat `0x14E04`) folded into
  `+0x67` via `0x795A4` (this is OL-35's `0x14E04` sine arm).
* **stage 1** `0x76E3B`: continues the rotation/track; calls `FUN_00076B28`
  at `0x76FA2` (steering, §4.4). This is the arm that reads `+0x65/+0x67`
  vectors and `+0x63`.
* **stage 2** `0x76FC7` (fresh `disassemble_bytes 0x76FC0`, 96 B) — the **rush
  test**: `byte[0x157C55] != 0 → 0x77636`; `rec == [0x157A83] → 0x77636`;
  `rec == [0x1577CA] → 0x77636`; else `anim = word[[rec+0x28]]`, and
  `word[rec+0x6B] > word[rec+0x77] (signed JG) → 0x77636`; otherwise the wide
  `0x76FD5..0x77636` body (calls `0x71C94` at `0x77423`, §4.1).
* **stage 3** `0x7769F`, **stage 4** `0x776C1` (fresh `disassemble_bytes
  0x776B8`, 80 B): `[rec+0x92]=4`, `FUN_0007DAB4(rec)`, pos→target; then
  **the finish**:
  ```
  0x776D8  EDI=[0x157A83]
  0x776DE  CMP EBP,EDI; JNZ 0x7770E
  0x776E2  if ([0x157A49]>>24 == 1) -> 0x7771E (human side returns)
  0x776EF..0x77707  FUN_0008A938(5, side, 1)               ; CPU side -> situation 5
  0x7770E  MOV EDX,0x19; MOV EAX,EBP; XOR ECX,ECX; XOR EBX,EBX; CALL 0x7D9A4
  ```
  The `0x7770E` install of **0x19** (not-controlled) and the CPU-side
  `FUN_0008A938(5, side, 1)` are exactly the ported
  `fifa96_keeper_reposition_b_finish` (`fifa96_keeper.c:244`: controlled &
  human → notify 0; controlled & CPU → notify 5; not controlled → install
  0x19). **The finish is already ported+tested; the body it finishes is not.**

The 0x1B body is therefore a full 5-arm machine with a sine rotation, a
steering call, a rush gate and a reset/situation finish. **Deep chain.**

---

## 3. The 0x0D body — row 0x0D `0x8251C` (4-arm machine)

Fresh `read_memory 0x8250C` (16 B) + `disassemble_bytes 0x8251C` (482 B):

```
0x8250C = {0x82567, 0x8261F, 0x826B0, 0x826D5}    ; 4 arms
0x8251C  PUSH EBX..EBP; MOV EBP,EAX
0x82526  timer +0x89 += [0x157A64]
0x82534  if ([0x157A83] == rec) [0x157A83] = 0            ; clear actor
0x8254C  stage = [rec+0x92]; CMP AL,3; JA 0x826F8; JMP CS:[EAX*4+0x8250C]
```

* **arm 0** `0x82567`: `byte[rec+0x8D]==0 → 0x826F1` (reset tail). Else
  `word[rec+0x83]=0x20`; `0x702F8((int16)[rec+0x81], 0x10)` → `+0x85`;
  `word[rec+0x87]=0`; `0x6E598(rec, anim, sector)`; velocity scale:
  `type_x=(int8)0x10F334[sector]`, `type_z=(int8)0x10F33C[sector]`,
  `scale = (type_x!=0 && type_z!=0) ? 3 : 4` (`0x825CB..0x825E1`);
  `+0x73 = type_x*scale`, `+0x75 = type_z*scale`; `0x8DC68(vel_x, vel_z)` →
  `+0x71`; `+0x9E=1`, timer=0, stage++.
  This whole block `0x825B2..0x82619` is **already ported** as
  `fifa96_action_sequence_velocity_scale` (`fifa96_action_handlers.c:1390`;
  FU-138 §3 table, row 0D arm 0 `0x825B2..0x82613`).
* **arm 1** `0x8261F`: while `word[+0x85] != 0` stay; else pos→target,
  `target += (int8)0x10F331/0x10F339[sector]<<6`, `0x8DCD4(pos,target,+0x65)`,
  anim `0x0B`, timer=0, stage++.
* **arm 2** `0x826B0`: `if (+0x71 != 0 && byte[+0x44] != 0) → 0x826F8`; else
  timer=0, stage++, fall to `0x826D5`.
* **arm 3** `0x826D5`: `0x79C50(rec, [+0x71]>>16, [+0x73]>>16)`;
  `if (+0x89 > 0x14)` → `0x826F1` reset (`FUN_0007DAB4`).

The 0x0D body is self-contained (no calls into the keeper chain); the only
unported sinks are `0x702F8`, the animation selector `0x6E598` and the face
`0x79C50`/vector `0x8DCD4` (all named helpers elsewhere). FU-82 §3.4 already
derived it byte-exact; this slice re-confirms the arm table and every arm.

---

## 4. The keeper-chain upstream (fresh xrefs this session)

### 4.1 `0x71C94` pan-caller census (fresh = 11, matches FU-174 §5)

Fresh `get_xrefs_to 0x71C94` = **11** sites. The two R28-relevant ones:

| site | window (fresh) | row |
|---|---|---|
| `0x77423` | `disassemble_bytes 0x77400` — preceded by `XOR BL,BL; [0x157A6C]=0; MOVSX EBX,SI` and followed by `EDX=[0x157A4A]>>24; CMP EAX,0x10` | **row 0x1B stage 2 rush** |
| `0x77DC6` | `disassemble_bytes 0x77DA8` — preceded by `EDX=0x19; 0x92820; MOVSX EBX,SI; XOR ECX,ECX` and followed by `CMP phase,0x10` | **row 0x1C tail** |

Both callers are inside the unwired rows 0x1B/0x1C, confirming FU-174 §5.

### 4.2 `FUN_000782D0` — the record-0 machine (`0x782D0..0x785D0`)

Fresh `get_function_by_address 0x782D0` (body `0x782D0..0x785D0`) +
`disassemble_bytes 0x782D0` (96 B): prologue `EBP=EAX`; gates
`byte[0x157823]!=0`, `rec==[0x157A83]`, `phase==5`; then increments
`word[0x157AD0 + side*2]` and `word[0x157ACC + (side^1)*2]` via
`FUN_000741B4`.

**Fresh `get_xrefs_to 0x782D0` = 1**: called from `FUN_0008D8EC` at
`0x8DB35` (fresh `get_function_by_address 0x8D8EC` = body
`0x8D8EC..0x8DB6A`). `FUN_0008D8EC` is the driver pre-pass
(*cross-cited* FU-151 §2.8/§3.4: the per-frame `FUN_000795B4` pre-pass that
writes `[team+0x7BA]`/`[team+0x7BE]`). **So the record-0 machine is not dead —
it is reached every frame from the driver pre-pass.** Whether the engine's
pre-pass reaches it is FU-151 §8 contract item 8 / leg 8 (unported).

### 4.3 `FUN_00077EAC` — the keeper CPU decision (`0x77EAC..0x782CD`)

Fresh `get_function_by_address 0x77EAC` + `disassemble_bytes 0x77EAC` (64 B):
`type8=[rec+0x8E]>>24`; if `!= 0x19` → `0x782C4`; else table
`0x14C1D4[side^1]` word, `AND AH,0x2`. Fresh `get_xrefs_to 0x77EAC` = **1**:
called from `FUN_000782D0` at `0x78599`. The tail is the **keeper escalation
ladder** (fresh `disassemble_bytes 0x78250`, 104 B + `0x782B1`, 40 B):

```
0x7826D  [0x157C2C]=0x2C; ECX=1; EDX=0x1A; CALL 0x7D9A4   ; install 0x1A now
0x78294  ECX=1; EDX=0x1B; CALL 0x7D9A4                    ; install 0x1B now
0x782B1  ECX=1; EDX=0x1C; CALL 0x7D9A4                    ; install 0x1C now
```

This pins the FU-74/FU-140 escalation rows (`0x1A/0x1B/0x1C` successors) to
their byte sites and gives **0x1B a second invoke-now installer** (`0x78299`)
on the live `FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` path.

### 4.4 `FUN_00076B28` — the steering (`0x76B28..0x76D11`)

Fresh `get_function_by_address 0x76B28` (body `0x76B28..0x76D11`) + fresh
`get_xrefs_to 0x76B28` = **2**: `0x76FA2` (row 0x1B stage-1 tail) and
`0x779B4` (row 0x1C). It calls `FUN_000795B4` at `0x76C59`
(*cross-cited* FU-151 §2.7 caller #8, "keeper steering band"). This is OL-35's
`0x76B28` steering.

### 4.5 `FUN_00076130` — the keeper input handler (`0x76130..0x7636C`)

Fresh `get_function_by_address 0x76130` + fresh `get_xrefs_to 0x76130` = **4**:

| site | kind | context |
|---|---|---|
| `0x76599` | UNCONDITIONAL_CALL | row-0x1F **stage-3 tail** (fresh `disassemble_bytes 0x76568`: slot `+0x20 != 0 && word[slot+4] != 0` → call) |
| `0x7845D` | COMPUTED_CALL | inside `FUN_000782D0` (record-0 machine) |
| `0x110954` / `0x11097C` | DATA | the `0x110950..0x1109E0` input-row tables (FU-140 §6) |

So `FUN_00076130` is dispatched both by row-1F's stage-3 tail (install
execution seam) and by the record-0 machine's input-row table.

---

## 5. Bounded verdict — what "the install executes honestly" needs

| install | native installer | live native reach today | engine reach today | body cost |
|---|---|---|---|---|
| **0x1B** | `0x76557` (row-1F stage-2 no-slot dive, invoke-now) **and** `0x78299` (CPU ladder, invoke-now) | the row-1F dive installer fires once row 0x1F is wired (R24); the CPU-ladder installer fires on the live `FUN_0008D8EC` pre-pass | the ported `fifa96_keeper_dive_target`/`_arm_step` already emit `install_code=0x1B, invoke=1` | **high** — 5-arm machine + `0x14E04` sine rotation + `FUN_00076B28` steering + the `0x157C54..5B` latches + the stage-2 rush gate |
| **0x0D** | `0x80F61` (row-09 body, install-only) | only inside row 0x09 (unwired; R26) | `fifa96_action_sequence_velocity_scale` ported; the row body itself unwired | **low** — 4-arm body fully derived (FU-82 §3.4); self-contained; sinks already named |

**Honest conclusion.** Neither install is *live in the engine today*: 0x1B's
row-1F installer is inside the same unwired row R24 is deriving, and its
CPU-ladder installer is on the `FUN_0008D8EC` pre-pass FU-151 has not ported;
0x0D's installer is inside row 0x09 (R26). So both rows can only join the
census **co-scoped with their first-order row**:

* **0x0D** is the cheaper, complete one: the body is byte-exact (this slice +
  FU-82 §3.4) and `velocity_scale` is already ported. Wiring
  `fifa96_match_action_0D` is honest the moment R26's row 0x09 (whose
  `0x80F61` installs it) lands; it is a *second-order* row, not a first-order
  census mover, until then.
* **0x1B** cannot be wired cheaply: the body draws on the record-0 machine,
  the CPU escalation (`FUN_00077EAC`), the `0x76B28` steering and the
  `0x14E04` sine rotation, all OL-34/OL-35. The ported
  `fifa96_keeper_reposition_b_finish` covers only the stage-4 tail; the body
  remainder is the deep chain.

---

## 6. Numbered legs

1. **FU180-L1 — row-0x1B body remainder.** The 5-arm machine
   `0x76D28..0x77727` beyond the stage-0 slice (`0x76DF6..0x76E3B`): stage-1
   rotation (`0x76E3B..0x76FC7`), the stage-2 rush body
   (`0x76FD5..0x77636`, incl. the `0x71C94` call at `0x77423`), stage 3
   (`0x7769F`). Carries OL-34.
2. **FU180-L2 — the `0x14E04` sine-table rotation.** The `FUN_00076B28`/
   `0x795A4`/`0x14E04` fold in row-0x1B stage 0/1 (`0x76E95..0x76EDA`,
   re-confirmed fresh) and row-0x1C. Carries OL-35.
3. **FU180-L3 — `FUN_00076B28` steering** (`0x76B28..0x76D11`, fresh body
   range) and its `FUN_000795B4` call at `0x76C59`. Carries OL-35.
4. **FU180-L4 — the `FUN_000782D0` record-0 machine** (`0x782D0..0x785D0`,
   fresh xref: `FUN_0008D8EC` `0x8DB35`) and its `FUN_00077EAC` callee
   (`0x78599`). Live from the driver pre-pass, but the pre-pass port is FU-151
   §8 item 8 / leg 8.
5. **FU180-L5 — `FUN_00077EAC` keeper CPU decision** full body
   (`0x77EAC..0x782CD`), including the escalation ladder's branch conditions
   (`0x78250` window: which of 0x1A/0x1B/0x1C each path installs). Carries
   FU-140 OL-36.
6. **FU180-L6 — `FUN_00076130` install execution.** The handler is ported as
   `fifa96_keeper_input_decide`, but its `0x7D9A4` execution
   (`0x762A4`/`0x7634B`) and its row-1F stage-3-tail caller (`0x76599`) are
   unported (OL-36).
7. **FU180-L7 — the `0x157C54..0x157C5B` keeper latches.** Row-0x1B stage 0
   clears them (`0x76E0F..0x76E2F`); the stage-2 rush gate reads
   `byte[0x157C55]`. Writers/consumers outside rows 0x1B/0x1C not enumerated.
8. **FU180-L8 — the `0x0D` unported sinks.** `0x702F8` (arm-0 ball
   parameter), the `0x6E598` animation selector and the `0x79C50`/`0x8DCD4`
   face/vector calls — all named helpers, not decomposed here (FU-82 open leg
   6/7).

---

## 7. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `get_xrefs_to` `0x71C94` (11), `0x7D9A4` (77),
`0x782D0` (1), `0x77EAC` (1), `0x76B28` (2), `0x76130` (4);
`get_function_by_address` `0x782D0` (`0x782D0..0x785D0`), `0x77EAC`
(`0x77EAC..0x782CD`), `0x76B28` (`0x76B28..0x76D11`), `0x76130`
(`0x76130..0x7636C`), `0x8D8EC` (`0x8D8EC..0x8DB6A`); `read_memory` `0x76D14`
(20 B), `0x8250C` (16 B); `disassemble_bytes` `0x764A0` (200 B, row-1F dive +
0x1B install), `0x76568` (96 B, row-1F stage-3 tail), `0x76D14` (160 B),
`0x76DB0` (300 B), `0x76FC0` (96 B), `0x776B8` (80 B), `0x7770E` (32 B,
row-0x1B stage-4 finish), `0x77400` (80 B), `0x77DA8` (48 B), `0x8251C`
(482 B, row 0x0D full), `0x80F50` (48 B, row-09 0x0D install), `0x782D0`
(96 B), `0x77EAC` (64 B), `0x78250` (104 B), `0x782B1` (40 B), `0x7B095`
(40 B, ruled out a second 0x0D install); `search_instructions` MOV `EDX, 0xd`
(21), MOV `EDX, 0x1b` (15). Engine/loader source read: `fifa96_keeper.c`
(165..255), `fifa96_match_handlers.c` (3430, 3450..3452, 3511),
`fifa96_action_handlers.c` (1390). Docs cross-cited: FU-74, FU-82, FU-136,
FU-137, FU-138, FU-140, FU-151, FU-174. No writes beyond this slice file and
the workspace report.

---

## 8. Landing note — M2 phase-15 T2 (2026-10-12)

Applied: `fifa96_match_action_1B` (`fifa96_match_handlers.c`) is the bounded
derived body of `0x76D28..0x77727` (the phase-0xD+side head camera place/take/
actor, the timer, stages 0/1/3/4 with the `+0x51 += 0x20`, the `0x77636` rush
exit and the ported `fifa96_keeper_reposition_b_finish`), and the row-0x1B
table slot is set. Its **installer is co-scoped with row 0x1F**: the ported
row-0x1F no-slot dive emits `install(0x1B, staged=1) + dispatch(0x1B)`
(`0x76557` invoke-now), so the chain runs end-to-end and the row is counted.
The `0x14E04` sine rotation (FU180-L2), the `0x76B28` steering (FU180-L3) and
the stage-2 wide body incl. the `0x71C94` call (`0x77423`, FU180-L1) stay
numbered legs — the port keeps the native control flow (all roads reach
`0x77636`) with the unported data writes disclosed. The second installer
`0x78299` and the `FUN_0008D8EC → FUN_000782D0 → FUN_00077EAC` pre-pass stay
FU180-L4/L5. `0x0D` is untouched (T3's, co-wired with row 09).
