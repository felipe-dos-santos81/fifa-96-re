# FU-145 — goal arming

Provenance: recon draft w1, phase-6 wave-1, frozen 2026-10-09; evidence review: PASS with corrections listed inline.

Phase-6 (M2 full gameplay) parallel-recon W1 (`docs/superpowers/plans/2026-10-08-fifa96-m2-full-gameplay.md`
§W1): the goal-arming chain — camera-pan arming, the clock phase gate, the
situation-6 producer, and the port contract for arming + firing it.

## Scope

Derived first-hand, read-only, on authoritative program **`/FIFA96.EXE`**
(native Watcom LE; every call passed `program=/FIFA96.EXE`; Ghidra MCP
read-only: no renames/comments/saves). `/fifa96.exe` and `/fifa96_le.bin` were
not used as evidence. All call targets below were diffed from the CALL bytes
(rel32 arithmetic shown); the +0x100000 rule is not needed here (all quoted
addresses are EXE link addresses); `dword[addr]>>16 = word at addr+2` is
applied where noted.

Covered: (1) the pan arming window `FUN_0007131C 0x713A6..0x713F7`
(`[0x15781D]=BH`, classifier call, `[0x15781E]=AL`), its sole caller and the
camera track that supplies the pan; (2) the clock tail gate
`FUN_0008AF38 0x8B623..0x8B63E`; (3) `FUN_00088940` (boundary, sole caller,
the situation-6 tail `0x88B37/0x88B42/0x88B44` and both other dispatcher
calls); (4) the queue arm `FUN_0008A938 0x8A9E8..0x8AA22`; (5) the engine's
current camera/arm state and the port contract. Not covered (other W/S tasks):
the scheduler/handler consumption (S3), the camera director that ultimately
moves the camera (FU-103/105), the score writer (already ported).

Method tool calls this slice (all `/FIFA96.EXE`): `get_function_by_address`
`0x7131C`, `0x8AF38`, `0x88940`, `0x736AC`, `0x71DF4`, `0x703E8`, `0x70074`;
`get_xrefs_to` `0x7131C`(1), `0x88940`(1), `0x736AC`(2), `0x8AF38`(2);
`disassemble_bytes` `0x7131C`(132 B), `0x713A0`(128 B), `0x718D0`(56 B),
`0x71900`(21 B), `0x8B5F0`(112 B), `0x88940`(96 B), `0x88B20`(240 B),
`0x8A9E8`(60 B), `0x73B40`(104 B), `0x737E0`(98 B), `0x73840`(64 B),
`0x7387D`(51 B), `0x4B170`(80 B), `0x70240`(48 B), `0x93C70`(24 B),
`0x93C88`(56 B), `0x94362`(25 B), `0x9437B`(53 B), `0x94534`(12 B),
`0x84F80`(20 B), `0x740EC`(12 B); `disassemble_function` `0x70074`(47 insns);
`decompile_function` `0x88940`; `search_instructions` mov with operands
`[0x0015774c]`(74), `[0x0015781d]`(8), `[0x0015781e]`(5),
`[0x0015781c]`(6), `[0x00157acb]`(3), `[0x001577c6]`(10), `[0x001577c8]`(6);
repo `grep`/`glob` over `src/`, `include/`, `tests/` for the engine state.

## 1. Evidence

### 1.1 The frame body order — `FUN_0004B100` (0x4B181..0x4B1AB, raw bytes)

| address | bytes | meaning |
|---|---|---|
| `0x4B181` | `30 c0` | `XOR AL,AL`; `[0x14C195] = 0` |
| `0x4B183` | `be 4c 77 15 00` | `ESI = 0x15774C` (the camera triple address) |
| `0x4B18D` | `89 35 10 c1 14 00` | `[0x14C110] = ESI` (camera pointer copy) |
| `0x4B193` | `e8 14 85 02 00` | `CALL 0x736AC` (diff: `0x4B198+0x28514` = `0x736AC`) — camera track |
| `0x4B198` | `80 3d 2a c3 14 00 00` | `CMP byte [0x14C32A],0` (session gate) |
| `0x4B1A1` | `e8 06 97 04 00` | `CALL 0x948AC` (diff: `0x4B1A6+0x49706`) — situation scheduler, gate-open only |
| `0x4B1A6` | `e8 8d fd 03 00` | `CALL 0x8AF38` (diff: `0x4B1AB+0x3FD8D`) — clock machine |

So each granted frame: **camera track → scheduler (gate) → clock**, and the
clock's tail is where the goal scan call sits.

### 1.2 The camera track `FUN_000736AC` (body `0x736AC..0x73CC4`) and the sole armer caller

* Callers (`get_xrefs_to 0x736AC` = **2**, both `UNCONDITIONAL_CALL`):
  `0x4B193` in `FUN_0004B100` (the frame body) and `0x743EF` (no function
  definition; the `0x742xx` setup/reset block).
* The camera position globals `0x15774C` (x) / `0x157750` (y) / `0x157754` (z)
  are advanced *inside this function* (dword views of the word pairs; the
  `>>16` trap applied):

| address | disassembly | meaning |
|---|---|---|
| `0x737ED` | `XOR EAX,EAX` | step counter 0 |
| `0x737EF` | `MOV [0x1577C6],AX` | `vel_x := 0` |
| `0x737F5` | `MOV [0x1577C8],AX` | `vel_z := 0` |
| `0x737FB..0x73832` | loop `BX` times: `vel_x += word[0x1577C0]` (`0x73800/0x73815/0x73821`), `vel_z += word[0x1577C2]` (`0x73817/0x7381F/0x73828`) | velocity ramp from the rate words |
| `0x73834..0x7383F` | `MOV EDX,[0x1577C6]; SAR EDX,0x10` = word[`0x1577C8`] = vel_z | (trap) |
| `0x7383A..0x73845` | `EAX=[0x1577C4]; SAR EAX,0x10` = word[`0x1577C6`] = vel_x; `CALL 0x8DC68` | velocity metric |
| `0x7384A` | `MOV [0x1577C4],AX` | speed word |
| `0x73855/0x73864/0x7386B` | `EDI=[0x15774C]; ADD EDI,EAX` (EAX = vel_x); `MOV [0x15774C],EDI` | **camX += vel_x** |
| `0x7387D/0x73883` | `EAX=[0x1577C6]>>16` (vel_z); `EBP=[0x157754]` (`0x7385E`); `ADD EBP,EAX`; `MOV [0x157754],EBP` | **camZ += vel_z** |
| `0x73898..0x738A0` | when the timer passes: copy `0x15774C`→`0x157788` (3 `MOVSD`), store `word[0x157800]` | the `0x157788` track triple |

* Pan sub-drivers called from the same function: `0x73B54 CALL 0x703E8`
  (diff `0x73B59-0x3771`; body `0x703E8..0x70541`), `0x73B6B CALL 0x71DF4`
  (diff `0x73B70-0x1D7C`; body `0x71DF4..0x7200F`, the ball-follow pan, called
  only when the tracked-record pointer `[0x1577CA]` is non-zero and
  `[EBP+0x20] != 0`).
* **The only caller of `FUN_0007131C`** (`get_xrefs_to 0x7131C` = **1**,
  `UNCONDITIONAL_CALL`) is `0x73B9B` in `FUN_000736AC`:

| address | disassembly | meaning |
|---|---|---|
| `0x73B70..0x73B80` | `EAX=[0x15774C]; abs; CMP EAX,0x6C0; JG 0x73B9B` | `|camX| > 0x6C0` → call |
| `0x73B82..0x73B99` | `EDX=[0x157754]; abs; CMP EAX,0xAB0; JLE 0x73BA0` | else `|camZ| > 0xAB0` → fall through |
| `0x73B9B` | `e8 7c d7 ff ff` | `CALL 0x7131C` (diff `0x73BA0-0x2884`) |

No argument registers are set for the call: the armer reads the globals
directly. The "pan argument" is therefore the camera position state
`[0x15774C]`/`[0x157754]` plus the frame delta `[0x157A64]`, not a call
parameter. The velocity pair `[0x1577C6]`/`[0x1577C8]` (`mov` census, 10/6
hits) is written by `FUN_000700F4` (`0x7024A`/`0x70260`, the camera reset —
also `[0x1577C4]=AX`, `[0x1577CA]=0`), `FUN_00070544` (`0x709BA`/`0x709C1`) and
`FUN_000736AC` itself (`0x737EF`/`0x737F5`); the rate words
`0x1577C0`/`0x1577C2` are the camera director's output (FU-103/105, unported).

### 1.3 The arming window — `FUN_0007131C` head (body `0x7131C..0x71B98`)

| address | bytes | disassembly | meaning |
|---|---|---|---|
| `0x71325` | `8a 25 1c 78 15 00` | `MOV AH,[0x15781C]` | pan counter |
| `0x7132F..0x71349` | `a0 64 7a 15 00` … | `AL=[0x157A64]`; `[0x15781C] = AH+AL` (byte wrap); `> 0x1E` → `[0x15781C]=0` | counter aged by the frame delta, capped |
| `0x7134F` | `e8 8c fa ff ff` | `CALL 0x70DE0` (diff `0x71354-0x574`) | camera mode/pan handler ([0x15781C] writer at `0x71231`) |
| `0x71354/0x7136C` | `8b 15 4c 77 15 00` / `66 89 44 24 0c` | `EDX=[0x15774C]`; `abs`; `word[ESP+0xC]=|camX|` | (sign/width: word truncation) |
| `0x71366/0x71383` | `8b 1d 54 77 15 00` / `66 89 44 24 10` | `EBX=[0x157754]`; `abs`; `word[ESP+0x10]=|camZ|` | |
| `0x7137D..0x7138A` | `8a 1d 1d 78 15 00` / `84 db` / `0f 85 19 05 00 00` | `BL=[0x15781D]; TEST BL,BL; JNZ 0x718A9` | already armed → skip to the reflect gate |
| `0x71390..0x7139D` | `a1 4a 7a 15 00` / `c1 f8 18` / `83 f8 02` / `74 09` / `83 f8 10` | `EAX=[0x157A4A]; SAR 0x18; CMP 2; JZ 0x713A6; CMP 0x10` | phase ∈ {2,0x10} else `0x713A0 JNZ 0x718A9` |
| `0x713A6..0x713B2` | `8b 44 24 0e` / `c1 f8 10` / `3d 20 0b 00 00` / `7f 12` | `EAX=[ESP+0xE]>>16` = word[ESP+0x10] = `|camZ|`; `CMP EAX,0xB20; JG 0x713C6` | arm on `|camZ| > 0xB20` (trap) |
| `0x713B4..0x713C0` | `8b 44 24 0a` / `c1 f8 10` / `3d 30 07 00 00` / `0f 8e e3 04 00 00` | `EAX=[ESP+0xA]>>16` = word[ESP+0xC] = `|camX|`; `CMP EAX,0x730; JLE 0x718A9` | else arm only if `|camX| > 0x730` (trap) |
| `0x713C6` | `b7 01` | `MOV BH,1` | |
| `0x713C8` | `8d 54 24 08` | `LEA EDX,[ESP+8]` | classifier out-word |
| `0x713CC/0x713D1/0x713D6` | `b8 4c 77 15 00` / `bf 7c 77 15 00` / `be 4c 77 15 00` | `EAX=0x15774C`; `EDI=0x15777C`; `ESI=0x15774C` | snapshot dest/src |
| `0x713DB` | `88 3d 1d 78 15 00` | **`MOV [0x15781D],BH`** | **arm := 1** |
| `0x713E1..0x713E5` | `31 c9` / `a5 a5 a5` | 3× `MOVSD` `0x15774C`→`0x15777C` | **snapshot:= camera x/y/z** |
| `0x713E6` | `88 3d cb 7a 15 00` | `MOV [0x157ACB],BH` | latched pan flag := 1 |
| `0x713EC` | `89 0d 80 77 15 00` | `MOV [0x157780],ECX` (ECX=0) | snapshot y := 0 |
| `0x713F2` | `e8 7d ec ff ff` | `CALL 0x70074` (diff `0x713F7-0x1383`) | goal-mouth classifier |
| `0x713F7` | `a2 1e 78 15 00` | **`MOV [0x15781E],AL`** | **zone := classifier result** |

The reflect/mirror arm reached when already armed (`0x718A9`) re-checks
`[0x15781E]` (`0x718EF`) and clears both flags: `0x718ED XOR AH,AH`;
`0x71908 MOV [0x15781D],AH`; `0x7190E MOV [0x15781E],AH` (bytes
`88 25 1d 78 15 00` / `88 25 1e 78 15 00`). The arming block ends at
`0x713FC` (`TEST AL,AL; JZ 0x71468`); everything after is the lead/goal-side
body (FU-71 §6 scope).

### 1.4 The classifier `FUN_00070074` (body `0x70074..0x700F1`, 47 insns)

`bool FUN_00070074(int *triple, ushort *out)`; called with `EAX=0x15774C`,
`EDX=&stack word`.

* `z = |triple[2]|` (sign-extended low word); `bits = 0`; `z < 0xB10` → `8`;
  `z >= 0xB90` → `4`; else `0` (`0x7007B..0x700A3`).
* `x = triple[0]`; `x < -0xD0` → `|1`; `x >= 0xD0` → `|2`
  (`0x700A5..0x700BB`).
* ceiling `h = 0xA0` when `z-0xB10 <= 0x30`, else `0xA0-(z-0xB40)`
  (`0x700BD..0x700D7`); `h < triple[1]` → `|0x10` (`0x700DE..0x700E3`).
* `*out = bits`; **returns `bits == 0`** (`0x700E5..0x700EB`: `TEST AX,AX; SETZ AL`).

So `[0x15781E] != 0` means the camera sits in the narrow goal-mouth band:
`-0xD0 <= camX < 0xD0`, `0xB10 <= |camZ| < 0xB90`, `camY <= h`.

### 1.5 The phase gate — `FUN_0008AF38` (body `0x8AF38..0x8B687`)

Callers (`get_xrefs_to 0x8AF38` = **2**): `0x4B1A6` (`FUN_0004B100`, §1.1) and
`0x7440B` (setup block). Raw bytes of the gate window:

| address | bytes | disassembly | meaning |
|---|---|---|---|
| `0x8B601..0x8B60B` | `31 c0` / `a0 c2 7a 15 00` / `83 f8 04` / `75 16` | `AL=[0x157AC2]`, `CMP EAX,4`, `JNZ 0x8B623` | period 4 special-cased |
| `0x8B60D..0x8B621` | `80 3d c0 7a 15 00 00` / `74 0d` / `e8 b1 03 00 00` / `fe 05 c2 7a 15 00` / `eb 38` | `CMP [0x157AC0],0`; `JZ 0x8B623`; `CALL 0x8B9CC`; `INC [0x157AC2]`; `JMP 0x8B65B` | extra-time period-4 path **skips the scan** |
| `0x8B623` | `a1 4a 7a 15 00` | `MOV EAX,[0x157A4A]` | phase dword |
| `0x8B628` | `c1 f8 18` | `SAR EAX,0x18` | phase byte |
| `0x8B62B` | `83 f8 02` | `CMP EAX,2` | |
| `0x8B62E` | `74 05` | `JZ 0x8B635` | phase 2 accepted |
| `0x8B630` | `83 f8 10` | `CMP EAX,0x10` | |
| `0x8B633` | `75 10` | `JNZ 0x8B645` | phase 0xC falls to the aux-zero block (`0x8B64A..0x8B655`); else return |
| `0x8B635` | `80 3d 1d 78 15 00 00` | `CMP byte [0x15781D],0` | arm flag |
| `0x8B63C` | `74 1d` | `JZ 0x8B65B` | not armed → return |
| `0x8B63E` | `e8 fd d2 ff ff` | **`CALL 0x88940`** (diff `0x8B643-0x2D03`) | **the goal scanner** |
| `0x8B643` | `eb 16` | `JMP 0x8B65B` | |

Owner of the gate: `FUN_0008AF38` (the FU-62 clock machine, called at
`0x4B1A6`). The census line "`FUN_00088940` at `0x8B63E`" is verified.

### 1.6 `FUN_00088940` (body `0x88940..0x88C0E`, 719 B)

* Callers (`get_xrefs_to 0x88940` = **1**, `UNCONDITIONAL_CALL`): `0x8B63E`
  only — the clock gate above.
* Head (`0x88940..0x88990`, bytes):

| address | disassembly | meaning |
|---|---|---|
| `0x88946` | `SUB ESP,8` | |
| `0x88955/0x8895B` | `[0x157A9F]=0; [0x157A9B]=0` | selection outputs cleared |
| `0x88966` | `CALL 0x92998` (EAX=1, EDX=4, EBX=-1) | possession/search helper (leg L2) |
| `0x8896B..0x88983` | `ECX=[0x157784]`; `abs`; `CMP EDX,0xB20; JLE 0x88BCC` | **frozen snapshot `|z| <= 0xB20` → throw-in arm** |
| `0x88989` | `CMP byte [0x15781E],0` | zone gate |
| `0x88990` | `JZ 0x88B53` | zone == 0 → corner/keeper arm |

* Goal arm (decompile + `0x88B20..0x88B53`): side select `0x88996..0x889AC`
  (`[0x157A49]>>24 == 1` → team side of `*[0x1587D4]`, else
  `snapshot z < 0`), possession search over `[0x1577CE+side*4]` /
  `0x1588A4+side*0x835` with `FUN_0008DE8C` (nearest, snapshot triple at
  `0x15777C`), `FUN_000795B4`/`FUN_00079C50`/`FUN_0006E598`, `FUN_000741B4`
  outcome, `FUN_000651F0(iVar3)`, `FUN_000974F0(0xBB8)`, then:

| address | bytes | disassembly |
|---|---|---|
| `0x88B2A` | `a1 9f 7a 15 00` | `MOV EAX,[0x157A9F]` |
| `0x88B2F/0x88B31` | `8b 10` / `8a 92 26 08 00 00` | `EDX=[EAX]`; `DL=[EDX+0x826]` (side) |
| `0x88B37` | `b8 06 00 00 00` | **`MOV EAX,6`** |
| `0x88B3C` | `81 e2 ff 00 00 00` | `AND EDX,0xFF` |
| `0x88B42` | `31 db` | **`XOR EBX,EBX`** |
| `0x88B44` | `e8 ef 1d 00 00` | **`CALL 0x8A938`** (diff `0x88B49+0x1DEF`) — situation 6, side = possession record's team side, BX=0 |

* Corner arm `0x88B53..0x88BCB` (`EAX=3+(cond)` from the zone signs,
  `EDX = ball-team^1`, `EBX=0`): `0x88BBD CALL 0x8A938` (diff
  `0x88BC2+0x1D76`).
* Throw-in arm `0x88BCC..0x88C0E` (phase 2 only): `EDX = [0x1577CA]` record's
  team `^1`, `EBX=1`, `EAX=2`; `0x88C00 CALL 0x8A938` (diff
  `0x88C05+0x1D33`).

### 1.7 The queue arm — `FUN_0008A938` table 1, situation 6 (`0x8A9E8`)

| address | bytes | disassembly | meaning |
|---|---|---|---|
| `0x8A9E8` | `66 85 f6` | `TEST SI,SI` | side (DX) == 0 ? |
| `0x8A9EB` | `75 1b` | `JNZ 0x8AA08` | side != 0 |
| `0x8A9ED` | `c7 05 a8 b6 15 00 05 00 00 00` | `MOV dword [0x15B6A8],5` | **queue id 5 (side 0)** |
| `0x8A9F7` | `c7 05 c0 b6 15 00 01 00 00 00` | `MOV dword [0x15B6C0],1` | **situation pending** |
| `0x8AA08` | `c7 05 a8 b6 15 00 06 00 00 00` | `MOV dword [0x15B6A8],6` | **queue id 6 (side 1)** |
| `0x8AA12` | `c7 05 c0 b6 15 00 01 00 00 00` | `MOV dword [0x15B6C0],1` | pending |

Both ids are the goal pending slots (consumed by the goal-screen handlers,
5 = side-0 goal, 6 = side-1 goal per the L.3/L.4 writer tables). Per-leg
id-6 semantics — cross-ref FU-146 §4/§8: the queue layer queues side-1 id 6;
consuming legs 0/1 map it to the no-score counter `INC dword [0x15B6A0]`,
legs 2–5 score side 1. The head of
`FUN_0008A938` routes here only when situation != 0/0xB, `[0x14C32A] != 0`
(session gate) and `[0x15B6C0] == 0`; otherwise the fallback takes the table-2
immediate arms (FU-143 §3.2; `0x8AC28` = table-2 situation 6 → phase 5, and it
clears `[0x157ACB]` at `0x8AC32`).

### 1.8 Writer/reader censuses (`search_instructions`, MOV-scoped unless noted)

The MOV-scoped censuses below were extended with the all-mnemonic CMP readers
(freeze pass); every added CMP reader tests `!= 0`, so the L6 "bits dead"
conclusion stands.

**[0x15781D] — 11 total sites (MOV-scoped: 1 read `0x7137D` + the 7 writes; plus
CMP readers `0x8B635`, `0x72AD3`, `0x89A7A`):**

| site | function | value | role |
|---|---|---|---|
| `0x7137D` | `FUN_0007131C` | — | read (already-armed test) |
| `0x713DB` | `FUN_0007131C` | `BH=1` | **pan armer** |
| `0x71908` | `FUN_0007131C` | `AH=0` (`0x718ED XOR AH,AH`) | reflect/mirror clear |
| `0x740F6` | `FUN_000740A0` | `DL=0` (`0x740F2 XOR DL,DL`), only when the new phase == 2 (`0x740E0..`) | setter clear |
| `0x84F90` | `0x84F8x` restart body | `AH=0` (after `0x84F84 CALL 0x700F4`) | restart clear |
| `0x93C87` | goal handler (`0x93BBC` body) | `DH=1` + snapshot `0x158897`→`0x15777C` and →`0x15B6C8` | post-goal **re-arm** |
| `0x9437A` | goal handler (`0x94270` body) | `BL=1` + same snapshot copy | post-goal **re-arm** |
| `0x9453C` | goal handler (`0x944FC` body) | `AH=0` (`0x94538 XOR AH,AH`) | post-goal clear |

Added CMP readers: `0x8B635` (`FUN_0008AF38` phase gate, §1.5), `0x72AD3`,
`0x89A7A`.

**[0x15781E] — 9 total sites (MOV-scoped: 3 writes + 2 reads; plus CMP readers
`0x88989`, `0x714D3`, `0x72ACA`, `0x89A83`):** writes `0x7026C` (`FUN_000700F4`
camera reset re-runs the classifier and stores it), `0x713F7` (armer), `0x7190E`
(clear); reads `0x70A3F` (`FUN_000709D0`), `0x718EF` (reflect gate); the four
added CMP readers only test `!= 0`.

**[0x15781C] — 6 sites:** reads `0x36EF3` (`FUN_00036C70` render staging),
`0x71325`; writes `0x7001B` (`FUN_0006FFC0`), `0x71231` (`FUN_00070DE0`, called
from the armer head), `0x7133C`/`0x71349` (the armer's cap). No arming gate
depends on it in the scanned sites; it is a camera/pan timer.

**[0x157ACB] — 3 sites:** write `0x713E6` (`=1`, the armer), write `0x8AC32`
(`FUN_0008A938` table-2 situation-6 arm, `DH=0`), read `0x8FCC8`.

### 1.9 Engine state (first-hand repo reads)

* The brief's `src/fifa96_engine/fifa96_match_render.c/.h` **do not exist**
  (glob over the tree: only `tests/test_engine_match_render.c` and
  `docs/ghidra/FU85_match_renderer.md`). The camera/presentation state lives in
  `include/fifa96_engine/fifa96_match_run.h` (`struct fifa96_match_run_render`,
  lines 114–150: `struct fifa96_camera camera`) and the FU-71 port
  `include/fifa96_loader/fifa96_camera.h` / `src/fifa96_loader/fifa96_camera.c`.
* Camera reset: `fifa96_match_run.c:641` `fifa96_camera_init(&r->camera, 0, 0,
  0)` (fresh match) and `:470` on the row-1E placement request (the kickoff
  `(0,0,0)` triple).
* Camera advance: `fifa96_match_run.c:1060` `fifa96_camera_update(...)` once per
  granted frame; the update only integrates when
  `(int16_t)cam->speed != 0 || (int16_t)cam->event_param != 0`
  (`fifa96_camera.c:45`), and the repo grep shows no engine writer of
  `vel_x/vel_z/rate_x/rate_z/event_param` (only `fifa96_camera.c` itself and
  `tests/test_camera.c`). **The run's camera never leaves spawn.**
* `fifa96_camera_reflect`/`fifa96_camera_out_of_bounds` are ported
  (`fifa96_camera.c:115/134`) but have **no engine call site** (grep over
  `src/fifa96_engine`).
* No engine symbol/field contains the native arm state: grep over `src/`,
  `include/`, `tests/` for `15781d|15781e|15b6a8|15b6c0|14c32a|888940|
  goal_armed|goal_scan` returns nothing. The queue/scheduler/handler cluster is
  unported (FU-142 App. L.5/L.6, FU-143 §10).
* Frame body seams for the port: `fifa96_match_run_frame`
  (`fifa96_match_run.c:1016`): camera update at `:1060`, entity chain
  `:1068`, phase machine `:1076`, `fifa96_match_run_phase_drive` at `:1081`
  (the derived `FUN_0008AF38` position). `fifa96_match_run_situation`
  (`:998`) models only the table-2 phase write and must not be used for the
  goal (it would take situation 6 → phase 5, the fallback arm; FU-142 L.9
  verdict, pinned by `test_goal_situation_dispatch_is_not_the_writer`).

## 2. Derived semantics

1. **Frame order.** Each granted frame runs the camera track
   (`FUN_000736AC`), then (session gate `[0x14C32A]`) the situation scheduler,
   then the clock (`FUN_0008AF38`) whose tail calls the scanner
   (`0x8B63E`). The camera therefore updates before the scan reads the frozen
   snapshot in the same frame.
2. **Who pans the camera.** `FUN_000736AC` is the per-frame camera integrator:
   it ramps velocity `vel_x/vel_z` from the rate words `[0x1577C0]`/`[0x1577C2]`
   (0x737FB..0x73832), adds them to `[0x15774C]`/`[0x157754]`
   (0x73864/0x7386B/0x73883) and runs the ball-follow `FUN_00071DF4` when the
   tracked record exists. It calls the boundary handler `FUN_0007131C` only
   when `|camX| > 0x6C0` or `|camZ| > 0xAB0` (0x73B7B/0x73B94/0x73B9B). The
   whole match's pan source is the camera director feeding those rate words
   (FU-103/105, unported; leg L1).
3. **Arming.** Inside `FUN_0007131C`, when not already armed
   (`[0x15781D]==0`) and the phase byte is 2 or 0x10 and
   (`|camZ| > 0xB20` **or** `|camX| > 0x730`): set `[0x15781D]=1`, copy the
   camera triple `0x15774C/50/54` to the snapshot `0x15777C/80/84` and
   immediately zero the snapshot y (`0x713EC`), set `[0x157ACB]=1`, run the
   goal-mouth classifier `FUN_00070074` on the current camera triple and store
   its return in `[0x15781E]`. Because the track only calls the arm body once
   `|camX| > 0x6C0` or `|camZ| > 0xAB0` — and the arm needs
   `|camZ| > 0xB20` or `|camX| > 0x730` — the arm fires only after the camera
   has been pushed deep into an end.
4. **Classifier.** `[0x15781E] = 1` iff the camera is inside
   `-0xD0 <= camX < 0xD0`, `0xB10 <= |camZ| < 0xB90`,
   `camY <= h(|camZ|)` (h = 0xA0 while `|camZ|-0xB10 <= 0x30`, then
   `0xA0-(|camZ|-0xB40)`). Combined with the arming bound this is the
   goal-mouth window `0xB20 < |camZ| < 0xB90`, `|camX| < 0xD0`.
5. **Firing.** Inside the clock tail, when the phase is 2/0x10 and the arm
   flag is set, `FUN_00088940` runs: `|snapshot z| <= 0xB20` → situation-2
   throw-in (phase-2 only, BX=1); `|snapshot z| > 0xB20` and zone != 0 → the
   goal arm; `|snapshot z| > 0xB20` and zone == 0 → situation 3|4 corner
   (phase-2 only, BX=0). The goal arm selects the possession record (side from
   the goal-side flag `[0x157A49]>>24==1` → the `[0x1587D4]` record's team,
   else the snapshot sign; nearest over that team's records from the snapshot
   via `FUN_0008DE8C`), then calls `FUN_0008A938(6, record_side, BX=0)`.
6. **Queue.** With the session gate open and no pending situation, situation 6
   takes the table-1 arm: `[0x15B6A8] = 5` (side 0) / `6` (side 1),
   `[0x15B6C0] = 1`. With the gate closed or a situation pending, the
   dispatcher's fallback takes table-2 situation 6 (phase 5, the "score
   screen" arm, clearing `[0x157ACB]`). The frame body consumes the queue via
   `FUN_000948AC` (0x4B1A1) — the S3 scope.
7. **Clears and re-arms.** `[0x15781D]` is cleared by the phase setter on a
   phase-2 write (0x740F6), by the reflect/mirror arm (0x71908), by the
   restart body (0x84F90) and by one goal handler (0x9453C); two goal handlers
   (0x93C87, 0x9437A) **re-arm it to 1** with the snapshot copied from
   `0x158897` (the goal-position triple, also copied to `0x15B6C8`). The clock
   path also clears `[0x157ACB]` on the table-2 situation-6 fallback (0x8AC32).

## 3. Port contract (engine names, call flow)

State (suggested, `struct fifa96_match_run`; native name in comment):

* `uint8_t goal_armed;` — native `[0x15781D]` (0x713DB/0x71908/0x740F6).
* `uint8_t goal_zone;` — native `[0x15781E]` (0x713F7; the classifier return).
* `int32_t goal_snap_x, goal_snap_z;` — native `[0x15777C]`/`[0x157784]`
  (`goal_snap_y` is always zeroed; keep it if the port wants the full triple).
* `uint8_t goal_counter;` — native `[0x15781C]` (aged by
  `mr->state.frame_delta`, cap 0x1E; only needed if the camera director port
  consumes it — `FUN_00036C70` reads it for rendering).
* `uint8_t situation_id;` — native `[0x15B6A8]` (dword; 5/6 for goals).
* `uint8_t situation_pending;` — native `[0x15B6C0]`.
* Session-gate equivalent for `[0x14C32A]`: the native sets it 1 in match
  session mode (`FUN_0001B7B8` 0x1B7C8 / `FUN_00032DE0` 0x32DFF, FU-142 L.5);
  the engine must model "live session" as 1 for the queue path, or goals will
  take the table-2 fallback (phase 5) instead of queueing 5/6.

Functions and call flow (native → engine):

1. `fifa96_camera_update` site (`fifa96_match_run_frame`, `fifa96_match_run.c:1060`;
   native `FUN_000736AC 0x4B193`) → add
   `fifa96_match_goal_arm(mr)` run **after** the camera update and before the
   entity chain, implementing `FUN_0007131C 0x71390..0x713F7`: gate
   `mr->state.phase == 2 || == 0x10`, `!goal_armed`,
   `|camera.pos_z| > 0xB20 || |camera.pos_x| > 0x730`; then
   `goal_armed = 1`, snapshot := camera `(x,z)` (y zero),
   `goal_zone = fifa96_match_goal_zone((int16_t)camera.pos_x, camera.pos_y, camera.pos_z)`.
   The native call itself is made by the track only when out of bounds
   (`fifa96_camera_out_of_bounds`, already ported); a faithful order is either
   "call the arm from the camera-track step when out of bounds" or an
   equivalent gate at this site — the observable state is the same.
2. `fifa96_match_run_phase_drive` (`fifa96_match_run.c:1081`; native `FUN_0008AF38`
   at 0x4B1A6) → add `fifa96_match_run_goal_scan(mr)` after the phase driver in
   the frame loop, implementing `0x8B623..0x8B643`: gate
   `(phase == 2 || phase == 0x10) && goal_armed` (period-4/extra-time path skips
   it, `0x8B601..0x8B621`); then the derived `FUN_00088940`:
   * `|goal_snap_z| <= 0xB20` → if phase == 2, queue situation 2 (throw-in);
   * else if `goal_zone != 0` → goal: pick the possession side/record
     (derived: the pool nearest search `fifa96_entity_find_nearest` over the
     goal-side team from `(snap_x, 0, snap_z)`; side from the goal-side flag /
     snapshot sign) and queue situation 6 with that record's side;
   * else if phase == 2 → queue situation 3|4 (corner).
   Queue write (`0x8A9E8`): if `situation_pending == 0` (and the session gate
   is opted in): `situation_id = (side == 0) ? 5 : 6; situation_pending = 1`.
   The engine must not route this through `fifa96_match_run_situation` (derived
   to table 2 → phase 5; FU-142 L.9).
3. Clears: `goal_armed = goal_zone = 0` on (a) every phase-2 write
   (`fifa96_match_state_set_phase` callers / the setter path — native
   `0x740F6`), (b) the FU-71 reflect arm (`0x71908`; wire
   `fifa96_camera_reflect` and clear alongside), (c) the restart body
   (`0x84F90`; engine reset paths), (d) `0x9453C` goal handler (S3).
   The two goal-handler re-arms (`0x93C87`/`0x9437A`, snapshot source
   `0x158897`) belong to S3 and are an explicit port decision (leg L5).
4. Preconditions for the chain to fire: a live phase 2/0x10, a pan source that
   drives `|camera.pos_z| > 0xB20` (or `|x| > 0x730`), the session gate
   modelled 1, and a situation-6 consumer (S3: `FUN_000948AC` + handlers).
   **Today the run has none of these**: the camera is static at (0,0,0), no
   arm fields exist, and no queue exists. An S2 fixture can drive the camera
   position directly (the FU-71 struct is caller-owned) to exercise arm → fire
   → queue without the native camera director (leg L1), and a test-only camera
   drive mirrors the plan's "deepest provably reachable step" honesty rule.

## 4. Numbered legs

1. **L1 — pan source (camera director).** The rate words `0x1577C0`/`0x1577C2`
   and the timers `0x1577FA/0x157800/0x157806` producers (FU-103/105 camera
   modes, `FUN_00070544`, `FUN_000700F4`, `FUN_00071DF4`) are not derived here;
   without them the native camera motion is a black box. Cross-ref: the pan
   producer is located in FU-148 §2.1(c) — `FUN_00071C94` +
   `FUN_00070544`/`0x709D0`/`0x70DE0`/`0x71DF4`, 11 gameplay callers. Settled
   by a FU-103/105 pass or a dynamic trace of `0x15774C`/`0x157754` during play.
2. **L2 — `FUN_00092998(1,4,-1)` semantics** (head of `FUN_00088940`, called at
   `0x88966`). Not decomposed; it seeds the possession selection
   (`[0x157A9B]`). Window: `0x92998..` (uncased body).
3. **L3 — possession-selection internals.** `FUN_0008DE8C` (nearest over the
   side's team block from the snapshot), `FUN_000795B4` (band), `FUN_00079C50`
   (face), `FUN_0006E598` (selector), `FUN_000741B4` (outcome), `FUN_000651F0`
   (event/sound), `FUN_000974F0(3000)` (sound), `FUN_00092040(200,0/1)`
   (announce) and `FUN_000974DC` (throw-in/corner sound) are unported
   auxiliaries on the arm paths. The derived scanner can substitute the pool
   nearest search (the FU-143 §11.1 row-01 precedent) and skip the sinks; the
   exact side/record selection must be reproduced or recorded as a stand-in.
4. **L4 — `[0x1587D4]`/`[0x1577CA]` record identities.** The goal-side side
   source (`*[0x1587D4]+0x826`) and the ball-team side source
   (`*[0x1577CA]+0x826` for throw-in/corner) are record pointers with no
   derived pool binding yet; writers/producers unread this slice.
5. **L5 — post-goal re-arm.** `0x93C87`/`0x9437A` set `[0x15781D]=1` and copy
   the `0x158897` triple into the snapshot (`0x15777C`) and `0x15B6C8`; the
   producer of `0x158897` and the consequences for the next scan are S3 scope.
6. **L6 — the zone-bit consumers.** Only `[0x15781E] != 0` is consumed by the
   scanner and the reflect gate; the classifier's out word (bits 1/2/4/8/0x10,
   written at `0x700E5`) is discarded by the armer (`EDX=ESP+8` stack word) —
   settled: the bits are dead in this chain (the full-mnemonic census adds only
   CMP `!= 0` readers, §1.8; no site reads the individual bits).
7. **L7 — `[0x157ACB]` semantics.** Armed by the pan arm (0x713E6), cleared by
   the table-2 situation-6 fallback (0x8AC32), read at `0x8FCC8`. Not needed
   for the queue path; if the port drops it, record the drop.
8. **L8 — native period-4 skip.** On the extra-time period-4 path
   (`0x8B60D..0x8B621`) the clock increments the period and joins the tail
   **without** the scan; the port must keep that skip if it models the
   extra-time branch (the engine currently passes `extra_time = 0`, FU-143
   OL-85).

## 5. Risks

* **Static-only "sole caller" claims.** `get_xrefs_to` proves the call sites
  in the loaded image; indirect calls (function-pointer tables) are not
  excluded by this method. For `0x7131C`/`0x88940` the count-1 result plus the
  matching byte-verified CALL is as strong as the static method allows.
* **Dormant chain.** With the engine camera static, wiring the armer/scan alone
  changes nothing observable until a pan source or a fixture drives
  `|camera.pos_z| > 0xB20`. The plan's honesty rule accepts this as the
  deepest reachable step; S2 should include the fixture so the wiring is
  testable.
* **Queue vs fallback.** If the port fires the goal through
  `fifa96_match_run_situation` or omits the session-gate/queue model, the
  native queue id 5/6 becomes the table-2 phase-5 arm — a visible divergence
  already pinned against (`test_goal_situation_dispatch_is_not_the_writer`).
* **Snapshot freeze.** The scanner reads the frozen `[0x157784]`, not the live
  camera; a port that scans the live camera would fire on a different frame if
  the camera moves between arming and the clock call. Keep the snapshot.
* **Re-arm sites.** If S3 lands the goal handlers without the `0x93C87`/
  `0x9437A` re-arm (or with the wrong snapshot source `0x158897`), the goal
  chain diverges after the first goal. Flagged in L5.
* **Cross-task interfaces.** S2 (this chain) depends on the camera director
  (L1) for natural motion and on S3 for consumption; W2's pad/possession work
  may move the same records the scanner selects. Keep the state engine-owned in
  `fifa96_match_run` to avoid parallel writers.

## 6. Port landing (S2, 2026-10-09)

Landed from the frozen slice during phase-6 wave-2 S2 (`src/fifa96_engine/fifa96_match_run.c`,
declarations in `include/fifa96_engine/fifa96_match_run.h`; first-hand
re-verified on `/FIFA96.EXE` this slice: `disassemble_bytes` `0x7131C`/`0x718A9`/
`0x740D0`/`0x8B620`/`0x4B180`/`0x84F80`/`0x73B70`, `decompile_function 0x88940`,
`disassemble_function 0x70074`, `get_xrefs_to 0x7131C` = 1 (sole caller
`0x73B9B`), `get_xrefs_to 0x88940` = 1 (`0x8B63E`)):

1. **`fifa96_match_goal_zone(x, y, z)`** — the classifier `FUN_00070074`
   (`0x70074..0x700F1`), exact native widths: the z band reads the
   sign-extended low word of the 32-bit magnitude (`0x70084`/`0x700C3`), x/y
   are full dwords, and the y ceiling uses the same word-truncated
   intermediate (`0x700BD..0x700D7`). Returns the "no outside bit" boolean.
2. **`fifa96_match_goal_arm(mr)`** — the armer window `0x71390..0x713F7`:
   phase in {2,0x10}, `!goal_armed`,
   `|camera.pos_z|_w > 0xB20 || |camera.pos_x|_w > 0x730` with the native
   **word-truncation trap** reproduced (`0x71354..0x71384` stores the absolute
   value as a word and re-reads it sign-extended; the port's
   `match_run_goal_abs_word`), then `goal_armed = 1`, the frozen snapshot
   (`goal_snap_x/y/z`; y forced 0 at `0x713EC`) and the classifier over the
   live camera triple. The already-armed reflect arm `0x718A9..0x7190E`: with
   `goal_zone == 0` and the reflect input bit set, `fifa96_camera_reflect`
   runs and both flags clear; `zone != 0` is the unported angle arm (FU-71
   leg 9.6). The armer **head** (`0x71325..0x71354`: the `[0x15781C]` counter
   age/cap and `FUN_00070DE0`) is out of the port contract window and is not
   ported (no `goal_counter` field; leg L1).
3. **Frame wiring** (`fifa96_match_run_frame`): the armer runs after
   `fifa96_camera_update` when `fifa96_camera_out_of_bounds` holds — the
   native camera-track call gate `0x73B70..0x73B9B` (sole armer caller,
   xref re-verified) — and `fifa96_match_run_goal_scan` runs after
   `fifa96_match_run_phase_drive` (the `FUN_0008AF38` tail; frame order
   `0x4B193` camera → `0x4B198` session gate → `0x4B1A1` scheduler →
   `0x4B1A6` clock re-verified).
4. **`fifa96_match_run_goal_scan(mr)`** — the `0x8B623..0x8B643` gate
   (phase 2/0x10 and armed; the period-4 extra-time skip is unreachable with
   the carried extra_time 0, L8) then the derived `FUN_00088940`: the
   full-32-bit snapshot magnitude vs `0xB20` (NEG wrap preserved), the zone
   gate, and the goal arm — side from the snapshot sign (`0x889B6 SETL`), the
   derived possession nearest over that side's team block from the snapshot
   triple (`FUN_0008DE8C` skip 0 → `fifa96_entity_find_nearest`), then
   `fifa96_match_run_goal_queue(mr, side)`.
5. **`fifa96_match_run_goal_queue(mr, side)`** — landed here (FU-146 §7 item 1
   names it; the FU-145 producer calls it): the table-1 arm `0x8A9E8`
   (`situation_id` = 5 side 0 / 6 side 1, `situation_pending = 1`) when the
   session gate is open and nothing is pending; otherwise the `0x8AC28`/
   `0x8AC88` fallback = the direct increment (`fifa96_match_run_add_goal`) plus
   the table-2 situation-6 phase-5 write through the shared
   `fifa96_match_run_situation` entry (no parallel table-2 mechanism; the
   freeze reconciliation's shared-0xB rule).
6. **State** (`struct fifa96_match_run`): `goal_armed` = `[0x15781D]`,
   `goal_zone` = `[0x15781E]`, `goal_snap_x/y/z` = `[0x15777C/80/84]`,
   `situation_id` = `[0x15B6A8]`, `situation_pending` = `[0x15B6C0]`,
   `session_gate_14c32a` = `[0x14C32A]` (init/teardown 0; **begin seeds 1**),
   plus `render.input_bit0` (the reflect input bit 0, caller-staged default
   0).
7. **Clears.** Every engine phase write goes through the new
   `match_run_write_phase` (FU-62 setter + FU-142a mirror); a phase-2 write
   clears `goal_armed`/`goal_zone` (`0x740F6`; the native clears only
   `[0x15781D]` — see errata 2). init/begin/teardown drop the arm/queue state
   with the match (the `0x84F90` restart clear).
8. **Tests** (`tests/test_engine_match_frame.c`, ASan/UBSan):
   `test_goal_zone_classifier`, `test_goal_arm_gates_and_snapshot`,
   `test_goal_arm_reflect_clear`, `test_goal_scan_queue_and_fallback`,
   `test_goal_phase2_write_clears_arm`, `test_goal_chain_pan_fixture` (a
   fixture pan drives the FU-71 velocity seam past the bounds;
   arm → snapshot → scan → queued id 5; the second frame pins the
   no-consumer fallback so S3 visibly replaces it). No ISO is required.
   `make check` 105/105; **M1 and M2 goldens byte-identical** (the tape camera
   is static, so nothing arms — the S2 risk "dormant chain" materialised
   exactly as predicted).

### Errata / port decisions

1. **L7 `[0x157ACB]` dropped.** Armed at `0x713E6`, cleared at `0x8AC32`, sole
   read `0x8FCC8` (unported); not needed for the queue path.
2. **Zone clear on the phase-2 write.** The native `0x740F6` clears only
   `[0x15781D]`; the port also clears `[0x15781E]`. The zone is consumed only
   while armed and the camera reset `0x7026C` re-runs the classifier, so the
   extra clear is observer-clean.
3. **`input_bit0` producer.** Native `(word[0x14C1D4] | word[0x14C1D6]) & 1`
   (the FU-139 per-side range words, image-zero; no ported runtime producer,
   L1). The engine carries a caller-staged `render.input_bit0` (default 0),
   mirroring `render.input_bit2` (the sibling bit 1 the FU-71 update reads);
   with the bit 0 the reflect clear is structurally present but does not fire.
4. **Side-selection stand-in.** The `[0x157A4C]>>24 == 1` arm reads the
   `[0x1587D4]` goal-side record's team byte (`0x889A4..0x889AC`); both the
   flag producers and the record identity are unported (L4), so the derived
   side is always the snapshot sign; the scanned record's team byte equals the
   searched block's side, so the queued side is exact.
5. **Throw-in/corner arms are w7-b1 legs.** Per the S2 ruling, the
   `|snap_z| <= 0xB20` throw-in arm (`0x88BCC`, situation 2) and the
   `zone == 0` corner arm (`0x88B53`, situations 3|4) return without queueing
   (their ball-record side source `[0x1577CA]` and queue semantics belong to
   the wave-7 B1 set-piece slice).

**FU-149/FU-150 pointer (2026-10-09):** the throw-in/corner/free-kick/penalty chains (situations 2/3/4/7/9 → phases 3/4/6/7; arms `0x8D2A3`/`0x8D35B`/`0x8D57B`/`0x8D4A2`) are frozen in FU-149 (set pieces/restarts) and FU-150 (fouls/referee/offside).
6. **L8 skip unmodelled.** The native period-4 branch joins the tail without
   the scan only on the extra-time path (`[0x157AC0] != 0`); the engine
   carries extra_time 0 (FU-143 OL-85), documented at the scan gate.
7. **Scanner magnitude wrap.** The `0x88971..0x8897B` NEG of INT_MIN wraps to
   itself (negative), so the signed `<= 0xB20` test takes the throw-in arm;
   the port reproduces the wrap instead of negating (avoiding C UB).
8. **Fallback lifecycle gate.** `fifa96_match_run_add_goal` returns
   `-FIFA96_ERR_STATE` on a non-running run (engine API convention, not
   native); the fallback is only reachable inside a live run's frame body.
9. **No-consumer latch.** Without the S3 scheduler, `situation_pending` stays
   latched (only the S3 handler step clears it), so the next granted frame's
   scan takes the fallback; pinned by the pan fixture.

### Legs status after S2 (updated at the phase-6 S5 close)

| leg | status |
|---|---|
| L1 pan source (rate words / camera director, `FUN_00071C94` lead) | **landed producer-level, row caller wired (phase-8 T2)**: the event setter `fifa96_camera_event_set` is complete (bail gate, ramp sign param, corrected `[0x1577FA] = F6` timer cell, slow path, `> 0x19` atan walk), `fifa96_camera_pan_step`/`reposition`/`rate_table` are derived ports and `fifa96_camera_update` calls the pan step at the native `0x737da` site; `fifa96_match_action_04` consumes the row-04 `out.events` (native `0x7EFCF`/`0x7F0D1`), so a live gameplay row now pans the camera (`test_row04_live_pan_arms_camera`: row → armer → situation 5 → S3 score; FU-148 §12). Carried: the other 9 `FUN_00071C94` callers, the `> 0x70` anchor branch and the sound/table sinks (OL-T11-79) |
| L2 `FUN_00092998(1,4,-1)` preselection | open (selection outputs stay zero) |
| L3 possession-selection sinks (`0x795B4`/`0x79C50`/`0x6E598`/`0x741B4`/`0x651F0`/`0x974F0`) | open — nearest search substituted |
| L4 `[0x1587D4]`/`[0x1577CA]` record identities / `[0x157A4C]` flag | open — snapshot-sign stand-in |
| L5 post-goal re-arm (`0x93C87`/`0x9437A`) | **reflect clear live**: S2 landed the armer's reflect arm (`0x718A9..0x7190E`) and S3 the leg-4 setup clear (`0x9453C..0x9454E`); the `0x93C87`/`0x9437A` post-goal re-arm body remains unported (FU-146 §8) |
| L6 zone-bit consumers | settled (bits dead) |
| L7 `[0x157ACB]` | dropped (errata 1) |
| L8 period-4 skip | unreachable/unmodelled (errata 6) |
