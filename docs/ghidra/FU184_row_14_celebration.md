# FU-184 — row-0x14 celebration/scatter body → native port (R32)

Provenance: M2 phase-16 wave **R32**, 2026-10-12, **read-only**. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; case-sensitive program argument; scripts
disabled). Every address/byte/call target below is a fresh
`disassemble_bytes` / `read_memory` / `get_current_program_info` of **this
session** unless marked *cross-cited* (the frozen FU doc is named). The alias
traps are applied (`dword[x]>>16` = the word at `x+2`; `>>24` = the byte at
`x+3`). No rename/label/comment/function/script/project save; no git; write set
= this file + the workspace `R32-report.md`.

Scope: derive the row-0x14 body (`0x1106E0[0x14] = 0x8784C`) byte-level — every
cell/gate written, the phase-5 arm's team gate (`0x8D433`), and the goal-record
release (stage-2 phase-1 write `0x87C71`, FU173-L2/L3) — then map it onto the
ported `fifa96_action_sequence_*` helpers and give the **T2 wiring rec** for the
minimal honest live port. The installer (the engine phase-5 arm
`fifa96_match_run_phase_arm`, `fifa96_match_run.c:2480`) is already live.

---

## 1. Answer in one line

Row `0x14` is the **3-stage scripted celebration** `0x8784C..0x87CBC` (stage 0
builds a five-point scatter and installs the first target; stage 1 walks the
points; stage 2 resets the record and **releases to phase 1 at `0x87C71`**). It
is already installed live by the ported byte-0 phase-5 arm (`match_run.c:2494`,
gated `team->side == mr->goal_record_side`), and its assembly is covered
helper-for-helper by the **already-ported and tested** FU-82
`fifa96_action_sequence_scatter_celebration` + `..._event_ids` +
`..._event` primitives. Wiring the action-table slot `0x14` flips one NULL `fn`
→ **29→30 action rows wired (30/80 → 37%)**; the minimal honest live port lands
the 3-stage machine over `mr->record`, with stage 2's release as the load-bearing
"continue past a goal" write.

---

## 2. Fresh verification (this session)

| call | result |
|---|---|
| `get_current_program_info` | `FIFA96.EXE`, `watcom:LE:32:default`, image base 0 |
| `read_memory 0x1106E0` (88 B) | `[0x14] = 76 78 08 00 = 0x0008784C` |
| `disassemble_bytes 0x8784C..0x87CBB` | 280 instructions, `truncated=false` — the row-0x14 body |
| `disassemble_bytes 0x87C43..0x87CBB` | 36 instructions — the stage-2 release tail |
| `disassemble_bytes 0x8D433..0x8D4A2` | 35 instructions — the phase-5 arm + both teams |
| `disassemble_bytes 0x8776C..0x8784A` | 67 instructions — the celebration-id init |
| `disassemble_bytes 0x45001..0x45024` | the `FUN_00045001` gate (`[0x1067EC] & 0xF0`) |
| `disassemble_bytes 0x700F4..0x7013B` | the `FUN_000700F4` camera reset head |

The action-table decode is unchanged from FU-182 §1 (`0x8784C`), and the
dispatch reader `FUN_0007D9A4 @ 0x7DA77` is the sole reader
(*cross-cited* FU-136 §1.1 / FU-182 §1).

---

## 3. The installer chain — the phase-5 arm's team gate (fresh)

The **live** installer is the byte-0 direct-goal chain (FU-173 §4.2): goal →
situation-6 → score + phase 5 → the phase-5 arm `0x8D433`. Fresh
`disassemble_bytes 0x8D433..0x8D4A2` (EBP = the team base, the arm runs once per
team):

```
0x8D433 8b 15 9f 7a 15 00   MOV EDX,[0x157A9F]     ; the goal-record pointer cell
0x8D439 8b 12               MOV EDX,[EDX]          ; -> the record's team pointer [rec]
0x8D43B 8a 85 26 08 00 00   MOV AL,[EBP+0x826]     ; the armed team's side byte
0x8D441 3a 82 26 08 00 00   CMP AL,[EDX+0x826]     ; == the goal record's team side?
0x8D447 75 33               JNZ 0x8D47C            ; no  -> other-team arm
   ; same team:
0x8D449 6a ff               PUSH -1
0x8D44B b9 15 00 00 00      MOV ECX,0x15           ; code 0x15
0x8D450 bb 0a 00 00 00      MOV EBX,0xA            ; last = 10
0x8D455 ba 01 00 00 00      MOV EDX,1              ; first = 1
0x8D45A 89 e8               MOV EAX,EBP
0x8D45C e8 57 fa ff ff      CALL 0x8CEB8           ; install 0x15 over records 1..10
0x8D461 ba 14 00 00 00      MOV EDX,0x14           ; code 0x14
0x8D466 a1 9f 7a 15 00      MOV EAX,[0x157A9F]     ; the goal record
0x8D46b 31 c9               XOR ECX,ECX
0x8D46d 31 db               XOR EBX,EBX            ; invoke-now = 0
0x8D46f 89 85 b2 07 00 00   MOV [EBP+0x7B2],EAX    ; team->target = the goal record
0x8D475 e8 2a 05 ff ff      CALL 0x7D9A4           ; install 0x14 on the goal record
0x8D47a eb 12               JMP 0x8D48E
   ; other team (0x8D47C): PUSH -1; EBX=0xA; ECX=EDX=0; CALL 0x8CEB8 (code 0 over 0..10)
0x8D48E 31 c0               XOR EAX,EAX
0x8D490 e8 27 3f fc ff      CALL 0x513BC           ; the dropped presentation sink (FU173-L8)
0x8D495 81 c4 00 01 00 00   ADD ESP,0x100
0x8D49b..a1                 POP EBP/EDI/ESI/EDX/ECX/EBX; RET
```

So the gate is exactly: **the armed team's side byte `[team+0x826]` equals the
goal record's team side `[[0x157A9F]+0x826]`** (`0x8D43B..0x8D447`). The
same-team arm installs code `0x15` over records **1..10** and code `0x14` on the
**goal record** with `[team+0x7B2] = the goal-record pointer`; the other-team arm
installs code `0`. The engine's `match_run.c:2494-2504` mirrors this
byte-for-byte (first-hand source read this session):
`team->side == mr->goal_record_side` → `fifa96_match_arm_install_multi(pool, t,
1, 10, 0x15, -1)`, `team->target = mr->goal_record`,
`fifa96_match_entities_install(&team->records[idx], phase, 0x14u, 0)`; else
`fifa96_match_arm_install_multi(pool, t, 0, 10, 0, -1)`. The installer is **live**
(FU-182 §4 rank 1; FU-173 §4.2/T3).

**Dispatch reachability.** `fifa96_match_entities_install` sets `entity->code =
0x14`; the engine's outfield record machine dispatches it at
`fifa96_match_run.c:1410` (`fifa96_match_dispatch_action(mr, e->code)`). The goal
record is the scanner's `match_run_goal_nearest` pick
(`match_run_goal_scan`, `fifa96_match_run.c:2886-2890`), which skips array index
0 (`fifa96_entity_find_nearest(..., 0, ...)`, `:2748`), so it is a record 1..10 —
an outfield record the record machine walks. Wiring slot `0x14` therefore makes
the body reachable on any natural goal.

---

## 4. The row-0x14 body byte-level (`0x8784C..0x87CBC`, EBP = the goal record)

### 4.1 Prologue + head (`0x8784C..0x8787F`)

```
0x8784C..52   PUSH EBX/ECX/EDX/ESI/EDI/EBP; MOV EBP,EAX
0x87854 31 c0               XOR EAX,EAX
0x87856 8b 95 89 00 00 00   MOV EDX,[EBP+0x89]          ; timer89 (dword)
0x8785c 66 a1 64 7a 15 00   MOV AX,[0x157A64]           ; the frame delta word
0x87862 01 c2               ADD EDX,EAX
0x87864 b8 c4 09 00 00      MOV EAX,0x9C4
0x87869 89 95 89 00 00 00   MOV [EBP+0x89],EDX          ; timer89 += delta
0x8786f e8 7c fc 00 00      CALL 0x974F0                ; sink (sound/UI; not decomposed)
0x87874 b8 02 00 00 00      MOV EAX,0x2
0x87879 89 2d a3 7a 15 00   MOV [0x157AA3],EBP          ; the tracked/anim record cell
0x8787f e8 7c e9 fa ff      CALL 0x36200                ; sink (EAX=2; not decomposed)
```

`timer89` is the native dword `+0x89`; the delta is `word[0x157A64]`
(zero-extended). `[0x157AA3]` (a pointer cell) and the `0x974F0`/`0x36200`
sinks stay legs.

### 4.2 Stage dispatch (`0x87884..0x8789C`)

```
0x87884 8a 85 92 00 00 00   MOV AL,[EBP+0x92]           ; stage byte +0x92
0x8788a 3c 01               CMP AL,0x1
0x8788c 72 13               JC 0x878A1                  ; stage 0
0x8788e 0f 86 8b 02 00 00   JBE 0x87B1F                 ; stage 1
0x87894 3c 02               CMP AL,0x2
0x87896 0f 84 77 03 00 00   JZ 0x87C13                  ; stage 2
0x8789c e9 15 04 00 00      JMP 0x87CB6                 ; stage >= 3 -> tail
```

### 4.3 Stage 0 (`0x878A1..0x87B1E`) — build + first target

```
0x878a1 84 c0               TEST AL,AL
0x878a3 0f 85 0d 04 00 00   JNZ 0x87CB6                 ; guard (AL==0 here)
0x878a9 83 bd 89 00 00 00 3c CMP [EBP+0x89],0x3C       ; timer89 >= 60?
0x878b0 0f 8c 00 04 00 00   JL 0x87CB6                  ; wait otherwise
```

**Ball-record anim (0x878B6..0x878F0).** `ESI = [EBP]` (team ptr) then
`ESI = [ESI+0x7A6]` (the team's ball record). `EAX = [ESI+0x28]; AL = [EAX]`
(the current animation row id byte). If it `== 0x26`:

```
0x878ce 8b 8e 8b 00 00 00   MOV ECX,[ESI+0x8B]          ; type byte = +0x8B>>24
0x878d4 e8 ef b1 00 00      CALL 0x92AC8                ; RNG
0x878d9 c1 f9 18            SAR ECX,0x18
0x878dc a8 01               TEST AL,0x1
0x878de 0f 94 c0            SETZ AL                     ; rng&1 == 0 -> 1
0x878e1 25 ff 00 00 00      AND EAX,0xFF
0x878e6 83 c0 46            ADD EAX,0x46                ; 0x47 if rng&1==0 else 0x46
0x878e9 31 db               XOR EBX,EBX
0x878eb 0f bf d0            MOVSX EDX,AX
0x878ee 89 f0               MOV EAX,ESI
0x878f0 e8 a3 6c fe ff      CALL 0x6E598                ; anim select on the ball record
```

So the ball record animates `0x47`/`0x46` by `rng & 1`
(`fifa96_action_sequence_rng_event`).

**Scatter build (0x878F5..0x87A3B).** `base = rec+0x59` copied (3 dwords) to
`0x1586D8` (`MOVSD x3`):

```
0x87900 83 7d 59 00         CMP dword[EBP+0x59],0        ; x
0x87904 7c 07 / 0x87906 ba 01 … JMP   -> EDX = (x < 0) ? -1 : 1        (dir_x)
0x87912 83 7d 61 00         CMP dword[EBP+0x61],0        ; z
0x87916 7f 07 / 0x8791f be ff …      -> ESI = (z > 0) ? -1 : 1        (dir_z)
0x87924 a1 49 7a 15 00      MOV EAX,[0x157A49]
0x87929 c1 f8 18            SAR EAX,0x18
0x8792c 83 f8 01            CMP EAX,0x1
0x8792f 75 02 / 0x87931 31 d2       -> if [0x157A49]>>24 == 1: dir_x = 0
```

Eight `0x92AC8` draws build points[1..4] (`x`,`z` only) exactly as FU-82 §3.5
quotes (`0x87933..0x87A1F`), then:

* **extra slot copy** `0x87A2A: MOV ESI,0x158708` + `MOVSD x3 -> 0x158714`
  (points[4] → a 6th slot; **before** the clamp loop, so unclamped);
* **clamp loop** `0x87A3C..0x87A81`: for 5 points (`EAX` step `0xC` to `0x3C`),
  `x` clamped to `[-0x720,0x720]` and `z` to `[-0xB10,0xB10]`
  (`0x87A2F MOV EBX,0xFFFFF8E0` = −0x720; `0x87A34 MOV EDX,0xFFFFF4F0` = −0xB10;
  `0x87A3E MOV ESI,0xB10`; `0x87A1F MOV ECX,0x720`).

**Celebration-id init (0x87A83).** `CALL 0x8776C` — the id initializer (fresh
`0x8776C..0x8784A`, 67 insns) that fills the byte array `0x1587B8..0x1587BC`
from the id tables `0x10F344/0x10F346/0x10F349/0x10F34C` (the ported
`fifa96_action_sequence_event_ids`). First-hand it matches the port's five
draws, the `0x67` short-circuit, the `0x67`-set override, the
`0x58/0x5B/0x5F/0x6B → 0x68` check, and the `t346` override
(`0x87776/0x87795/0x877ba..0x877e9/0x877fb..0x87818/0x8781d..0x87842`).

**Arm block (0x87A88..0x87B17), run for the first point:**

```
0x87a88 a1 20 87 15 00      MOV EAX,[0x158720]          ; idx
0x87a8d 8d 50 01            LEA EDX,[EAX+1]
0x87a90 8d 5d 65            LEA EBX,[EBP+0x65]
0x87a93 8d 04 95 00 00 00 00 LEA EAX,[EDX*4]
0x87a9a c7 85 89 00 00 00 00 00 00 00  MOV [EBP+0x89],0   ; timer89 = 0
0x87aa4 29 d0               SUB EAX,EDX                 ; EAX = 3*(idx+1)
0x87aa6 8d 7d 4d            LEA EDI,[EBP+0x4D]
0x87aa9 8d 55 4d            LEA EDX,[EBP+0x4D]
0x87aac 8d 34 85 d8 86 15 00 LEA ESI,[EAX*4+0x1586D8]     ; = 0x1586D8 + 12*(idx+1)
0x87ab3 8d 45 59            LEA EAX,[EBP+0x59]
0x87ab6 a5 a5 a5            MOVSD x3                    ; points[idx+1] -> rec+0x4D
0x87ab9 e8 16 62 00 00      CALL 0x8DCD4                ; metric -> rec+0x65
0x87abe 89 e8               MOV EAX,EBP
0x87ac0 8b 5d 67 / 55 65   … MOV EBX,[EBP+0x67]; EDX,[EBP+0x65]; SAR x2,0x10
0x87acc e8 7f 21 ff ff      CALL 0x79C50                ; face toward (EDX,EBX) words
0x87ad1 8b 45 28            MOV EAX,[EBP+0x28]
0x87ad4 8b 15 20 87 15 00   MOV EDX,[0x158720]          ; idx
0x87ada 8a 00               MOV AL,[EAX]                ; live anim row id
0x87adc 8a 9a b8 87 15 00   MOV BL,[EDX+0x1587B8]       ; id table
0x87ae2 38 d8               CMP AL,BL
0x87ae4 74 1a               JZ 0x87B00                  ; same -> no install
   ; differ:
0x87ae6 8b 8d 8b 00 00 00   MOV ECX,[EBP+0x8B]; SAR ECX,0x18
0x87aec 88 da               MOV DL,BL; AND EDX,0xFF
0x87af7 89 e8 / 31 db       MOV EAX,EBP; XOR EBX,EBX
0x87afb e8 98 6a fe ff      CALL 0x6E598                ; anim select (id = BL)
0x87b00 8a bd 92 00 00 00   MOV BH,[EBP+0x92]
0x87b06 c7 85 89 00 00 00 00 00 00 00  MOV [EBP+0x89],0   ; timer89 = 0
0x87b10 fe c7               INC BH
0x87b12 88 bd 92 00 00 00   MOV [EBP+0x92],BH           ; stage++  (0 -> 1)
0x87b18..1e                POP EBP/EDI/ESI/EDX/ECX/EBX; RET
```

The arm copies **points[idx+1]** (idx = `[0x158720]`) into the record target
`+0x4D`, runs the metric `0x8DCD4` (`+0x65`) and the face `0x79C50`, posts anim
`[0x1587B8+idx]` when the live row id differs
(`fifa96_action_sequence_event`), resets the timer and advances the stage.

### 4.4 Stage 1 (`0x87B1F..0x87C12`) — the point walk

```
0x87b1f e8 dd d4 fb ff      CALL 0x45001                ; [0x1067EC] & 0xF0
0x87b24 84 c0               TEST AL,AL
0x87b26 74 2b               JZ 0x87B53                  ; 0 -> arm block
0x87b28 81 bd 89 00 00 00 b4 00 00 00  CMP [EBP+0x89],0xB4
0x87b32 7e 1f               JLE 0x87B53
   ; 0x45001 != 0 && timer89 > 0xB4 -> early advance:
0x87b34..46  timer89=0; stage++; POP…; RET
0x87b53 arm block (identical to §4.3 0x87A88 but without the id compare):
   copy points[idx+1] -> rec+0x4D; 0x8DCD4; 0x79C50
0x87b92 8b 45 63            MOV EAX,[EBP+0x63]; SAR EAX,0x10   ; height word
0x87b98 83 f8 20 / 7c 3c    CMP EAX,0x20; JL 0x87BD9           ; height < 0x20 -> advance
0x87b9d 8b 95 89 00 00 00   MOV EDX,[EBP+0x89]
0x87ba3 81 fa b0 04 00 00   CMP EDX,0x4B0; JG 0x87BD9          ; timer > 0x4B0 -> advance
0x87bab 83 3d 20 87 15 00 03 CMP [0x158720],3
0x87bb2 75 08 / 0x87bb4 81 fa 2c 01 … JG 0x87BD9               ; idx==3 && timer>0x12C
0x87bbc 83 3d 20 87 15 00 04 CMP [0x158720],4
0x87bc3 0f 85 ed 00 00 00   JNZ 0x87CB6                 ; idx != 4 -> wait
0x87bc9 81 bd 89 00 00 00 b4 00 00 00  CMP [EBP+0x89],0xB4
0x87bd3 0f 8e dd 00 00 00   JLE 0x87CB6                 ; timer <= 0xB4 -> wait
0x87bd9 a1 20 87 15 00      MOV EAX,[0x158720]
0x87bde 40                  INC EAX
0x87bdf a3 20 87 15 00      MOV [0x158720],EAX          ; idx++
0x87be4 83 f8 05            CMP EAX,5
0x87be7 7d 0b               JGE 0x87BF4
0x87be9 fe 8d 92 00 00 00   DEC byte[EBP+0x92]          ; stage-- (re-entry, see note)
0x87bef e9 94 fe ff ff      JMP 0x87A88                 ; re-run the arm block
0x87bf4..06                 timer89=0; stage++; POP…; RET   ; idx == 5 -> advance
```

`FUN_00045001` is a tail getter (fresh `0x45001..0x45024`):
`MOV AL,[0x1067EC]; AND AL,0xF0; return` — an input/pad byte gate. The stage-1
advance is therefore: **early** when `([0x1067EC] & 0xF0) != 0 && timer89 >
0xB4`; otherwise the arm block applies `points[idx+1]` and advances `idx` when
`height<0x20` or `timer89>0x4B0` or `(idx==3 && timer89>0x12C)` or
`(idx==4 && timer89>0xB4)`; at `idx==5` the stage advances to 2. (The native
`DEC byte[+0x92]` at `0x87BE9` temporarily drops the stage to 0 so the arm block
at `0x87A88` re-runs and re-increments it at `0x87B00`; this is the FU-82 §3.5
"decrement the stage byte and re-enter" note — a curious re-entry, not a third
state.)

### 4.5 Stage 2 (`0x87C13..0x87CB5`) — reset + release

```
0x87c13 89 e8               MOV EAX,EBP
0x87c15 e8 9a 5e ff ff      CALL 0x7DAB4                ; the central record reset
0x87c1a a1 49 7a 15 00      MOV EAX,[0x157A49]; SAR EAX,0x18
0x87c22 83 f8 01 / 0f 84 …  CMP EAX,0x1; JZ 0x87CB6      ; side == 1 -> exit, no release
0x87c2b 31 db               XOR EBX,EBX; PUSH EBX
0x87c2e 8b 0d 30 f3 10 00   MOV ECX,[0x10F330]; PUSH ECX
0x87c35 8b 35 2c f3 10 00   MOV ESI,[0x10F32C]; PUSH ESI
0x87c3c 8b 3d 28 f3 10 00   MOV EDI,[0x10F328]; PUSH EDI   ; camera triple args
0x87c43 89 1d 20 87 15 00   MOV [0x158720],EBX          ; scatter idx = 0
0x87c49 e8 a6 84 fe ff      CALL 0x700F4                ; camera reset
0x87c4e e8 d5 c1 fe ff      CALL 0x73E28                ; phase reset
0x87c53 8b 45 00 / 8a 80 26 08 00 00  MOV EAX,[EBP]; AL=[EAX+0x826]
0x87c5c 34 01               XOR AL,0x1
0x87c5e a2 af 7a 15 00      MOV [0x157AAF],AL           ; side ^ 1
0x87c63 8b 15 ac 7a 15 00   MOV EDX,[0x157AAC]
0x87c69 b8 01 00 00 00      MOV EAX,0x1                 ; phase 1 (kickoff)
0x87c6e c1 fa 18            SAR EDX,0x18                ; controlled side
0x87c71 e8 2a c4 fe ff      CALL 0x740A0                ; phase entry (1, controlled)  << RELEASE
0x87c76 e8 8d c1 fe ff      CALL 0x73E08                ; placement commit
0x87c7b ba 26 00 00 00      MOV EDX,0x26; MOV EAX,[EBP];   CALL 0x8CFAC   (team record)
0x87c88 8b 80 a6 07 00 00   MOV EAX,[EAX+0x7A6]; EDX=0x26; CALL 0x8CFAC   (ball record)
0x87c9d e8 4a 97 fc ff      CALL 0x513EC
0x87ca2 6a 00               PUSH 0; ECX=0; EAX=0x15774C; PUSH 0; EBX=0; EDX=0;
0x87cb1 e8 6e 46 fc ff      CALL 0x4C324                ; ball reset
0x87cb6 5d 5f 5e 5a 59 5b c3  tail: POP EBP/EDI/ESI/EDX/ECX/EBX; RET
```

The stage-2 head resets the record (`0x7DAB4`) and, unless the record's team
side is 1, performs the release: camera reset `0x700F4`, phase reset `0x73E28`,
`[0x157AAF] = side ^ 1`, and **`0x740A0(1, controlled)` at `0x87C71`** — the
native match-phase entry that latches phase 1 and runs the per-team phase arm
(FU-173 §4.2 / FU-143). Then the `0x73E08` placement commit, the two `0x8CFAC`
record sinks (team + ball, `EDX=0x26`), the `0x513EC` camera stop and the
`0x4C324` ball reset tail.

---

## 5. The release path in engine terms

The engine's *kickoff entry* is already modeled at begin
(`fifa96_match_run.c:2213-2220`):

```
(void)match_run_write_phase(mr, FIFA96_MATCH_RUN_KICKOFF_PHASE);   /* phase 1 */
(void)fifa96_match_phase_machine_kickoff(mr);                      /* the state-1 arm */
```

and `0x740A0`'s phase latch is `match_run_write_phase` (`:2374`), whose
`phase_machine.state/phase` writes and `phase == 2` special-case mirror
`fifa96_match_state_set_phase`. So the row-0x14 release maps to:

1. `match_run_write_phase(mr, 1)` — the `0x87C69/0x87C71` phase-1 latch;
2. `fifa96_match_phase_machine_kickoff(mr)` — the `0x740A0` per-team phase arm
   (native `FUN_0008D098` state-1 arm);
3. the placement commit — `0x73E08` (the engine runs the equivalent
   place+commit block at begin, `:2225-2245`).

FU-173 §6 R21-2 item 3 already names this landing and recommends factoring a
shared `match_run_kickoff_entry`; the current port leaves the release
**unported** because row `0x14` has no handler body (`table[0x14].fn == NULL`,
`fifa96_match_handlers.c:4069`).

---

## 6. Mapping onto the ported helpers

| native site(s) | port | status |
|---|---|---|
| five-point scatter + both clamp axes + `dir_x = 0` degeneracy (`0x87933..0x87A81`) | `fifa96_action_sequence_scatter_celebration(base, dir_x, dir_z, rng[8], points[5])` | **ported + tested** (`test_event_sequences.c:133`, `:140..164`) |
| celebration-id init (`0x8776C..0x8784A`) | `fifa96_action_sequence_event_ids(t344, t346, t349, t34c, rng[7], ids)` | **ported + tested** (`:236..268`) |
| anim compare `[[rec+0x28]] != [0x1587B8+idx]` (`0x87AD1..0x87AFB`) | `fifa96_action_sequence_event(anim_byte, event_id, &post)` | **ported + tested** (FU-82 §6 table) |
| ball-record anim `(rng&1) ? 0x46 : 0x47` (`0x878DC`) | `fifa96_action_sequence_rng_event(rng, 0x47, 0x46, &id)` | **ported + tested** |
| `0x92AC8` RNG | `fifa96_rng_step(&mr->rng, &r)` | **ported** (used by the phase-4 arm `match_run.c:2584`) |
| `0x6E598` anim select | `fifa96_match_run_record_anim_install(mr, e, id, 0)` | **ported** (used by rows 09/0E/1E/… ) |
| `0x8DCD4` metric (`+0x65`) | the FU-142 `{distance,dx,dz}` helper / `fifa96_entity_distance` | **ported** (FU-167) |
| `0x79C50` face | the ported face helper (FU-142 §… layout) | **ported** (rows 09/28/0E) |
| `0x7DAB4` reset | `match_row_reset(mr, e)` (`fifa96_match_handlers.c:2202`) | **ported** |
| stage/advance (`+0x89`/`+0x92`/`+0x44`) | `mr->record.timer89` / `.stage92` / `.row44` | **ported staging** |
| `0x740A0` release | `match_run_write_phase(mr, 1)` + `fifa96_match_phase_machine_kickoff` | **ported at begin only** |
| head sinks `0x974F0`, `0x36200`; `[0x157AA3]`; `0x700F4`/`0x73E28`/`0x73E08`/`0x8CFAC`/`0x513EC`/`0x4C324`; `0x45001` gate producer | not ported | **legs** (FU-82 §9 / FU173-L4) |

So every **arithmetic** primitive of row `0x14` already exists and is tested; the
missing pieces are the sinks/gates and the `mr`-side scatter index/points state.

---

## 7. T2 wiring rec (minimal honest live port)

**T2-1 (wire + body).** Add `fifa96_match_action_14` in
`src/fifa96_engine/fifa96_match_handlers.c` and set
`fifa96_match_action_table[0x14].fn` (replacing the NULL `fn` at `:4069`; keep
an evidence string). The body is the 3-stage machine of §4 over `mr->record`:

* head: `mr->record.timer89 += mr->record.delta`; `[0x157AA3] = rec`; the two
  sinks as no-op requests (legs).
* stage 0: gate `timer89 >= 0x3C`; the ball-record anim via
  `fifa96_match_run_record_anim_install` on the team's ball record (the
  `rng&1 → 0x47/0x46` pick, gated `[[ball+0x28]] == 0x26`); the 5-point scatter
  via `fifa96_action_sequence_scatter_celebration(base = record pos, dir_x,
  dir_z, rng[8], points)`; **a 6-element buffer** with the pre-clamp
  `points[5] = points[4]` copy (§4.3, leg FU184-L1); the id init via
  `fifa96_action_sequence_event_ids`; the arm block for `idx = 0` (target copy,
  metric, face, `fifa96_action_sequence_event`); reset timer, `stage92 = 1`.
* stage 1: `FUN_00045001` gate + the height/timer conditions
  (`height<0x20`, `timer>0x4B0`, `idx==3 && timer>0x12C`, `idx==4 && timer>0xB4`)
  driving `idx`; at `idx==5` reset timer, `stage92 = 2`.
* stage 2: `match_row_reset(mr, e)`; if `mr->record.side != 1`, **release**:
  factor `match_run_kickoff_entry(mr)` from the begin block (`:2213-2220` +
  placement commit) and call it; then the record sinks (`0x8CFAC`, `0x513EC`,
  `0x4C324`) as no-op requests (legs).
* end: return `FIFA96_OK`.

The handler needs three new `mr` cells: the scatter index (`[0x158720]`), the
6-point scatter buffer (`[0x1586D8]`), and the id array (`[0x1587B8..]`) — add
them to `struct fifa96_match_run` (or a derived `mr->celebration` sub-struct).
The `dir_x`/`dir_z` sign rules and the `[0x157A49]>>24 == 1 → dir_x = 0` clamp
are §4.3 first-hand.

**T2-2 (release factoring).** Extract `match_run_kickoff_entry(mr)` from
`fifa96_match_run.c:2213-2245` so begin and the row-0x14 release share the
phase-1 latch + `fifa96_match_phase_machine_kickoff` + placement commit; this is
FU-173 §6 R21-2 item 3 restated at the row-0x14 port.

**Landing scope fork (name it).** A minimal honest landing can implement stages
0/1 as the scatter (the tested helper) + the arm block, and stage 2 as
`match_row_reset` + `match_run_kickoff_entry`; that is enough for a *live*
continuation past a natural goal and flips the census. The head sinks, the
`0x45001` input producer, and the `0x700F4`/`0x73E28`/`0x8CFAC`/`0x513EC`/
`0x4C324` tails can land as numbered legs without changing the dispatch result.

**Wiring-rec headline.** Wire row `0x14` → action rows wired **28 → 29**
(plus the 1 phase row `0x1F` = **30/80**); `floor(100 × 30/80) = 37%` (README
36% → 37%). Cross-check against `docs/ENGINE.md:263` (currently `29/80` = 36% =
28 action + the first phase row `0x1F`) and recompute the README title in the
same commit as the wiring (AGENTS.md census rule).

---

## 8. Numbered legs (FU184)

1. **FU184-L1 — the 6-point scatter slot.** The native copies `points[4]` →
   `0x158714` **before** the clamp loop (`0x87A2A..0x87A39`), and the arm reads
   `points[idx+1]` for `idx ∈ 0..4` (`0x87AAC` = `0x1586D8 + 12*(idx+1)`, so
   `idx=4` reads the copied slot). The ported helper builds/clamps 5 points and
   leaves `points[4]` post-clamp, so a faithful port needs a 6-element buffer
   with `points[5] = pre-clamp points[4]`; the exact `+0x4D` target for
   `idx==4` is not covered by the 5-point helper today. Confirm before wiring.
2. **FU184-L2 — the un-written `y` of points[1..4].** The native writes only
   `x`/`z` for points[1..4] (`0x8794B/0x87968/…`), leaving each `y` at the
   buffer's prior value; the helper sets `points[i].y = base->y`. The arm copies
   all 3 dwords into `rec+0x4D`, so the `y` source is load-bearing and
   unproven.
3. **FU184-L3 — `FUN_00045001` (`[0x1067EC] & 0xF0`).** The stage-1 early
   advance gate is a raw input byte (fresh `0x45001..0x45024`); its producer
   (the pad/config cell `[0x1067EC]`) is unported. Derive or stage-zero it
   before the stage-1 timing is faithful.
4. **FU184-L4 — the head sinks.** `0x974F0` (`EAX=0x9C4`), `0x36200` (`EAX=2`)
   and the pointer cell `[0x157AA3]` are cited by call only; their presentation
   semantics are not derived here.
5. **FU184-L5 — the release tails.** `0x700F4` (camera reset, fresh head
   `0x700F4..0x7013B`), `0x73E28` (phase reset; FU173-L4), `0x73E08` (placement
   commit), the two `0x8CFAC` record sinks (`EDX=0x26`), `0x513EC` and the
   `0x4C324` ball reset are cited, not decomposed. The *phase-1 write* is the
   only load-bearing one (T2-2); the rest are observer-only or already modeled
   at begin.
6. **FU184-L6 — the side gate constant.** Stage 2 exits without releasing when
   `[0x157A49]>>24 == 1` (`0x87C1A..0x87C25`). `[0x157A49]` is the same cell the
   scatter uses for its `dir_x = 0` clamp (`0x8792F`); its writer/semantics
   (which "side" `== 1` means) is unproven here — verify against FU-173's
   controlled-side derivation before wiring the release.
7. **FU184-L7 — the ball-record `0x26` template gate.** Stage 0's ball-record
   anim is gated on `[[team+0x7A6]+0x28]`'s first byte `== 0x26`
   (`0x878BF..0x878CC`); `[team+0x7A6]` is the team's ball-record pointer, whose
   engine producer/stand-in is the pool's ball record (FU-141). Confirm the
   mapping before wiring.
8. **FU184-L8 — row 0x15 (`0x87CD0`).** The sibling celebration gate row stays
   unported (`table[0x15].fn == NULL`, `:4070`); FU173-L2 carries it. It is not
   a mover by itself.
9. **FU184-L9 — the phase-5 arm's else sink.** The dropped `0x8D48E XOR EAX,EAX
   + CALL 0x513BC` presentation sink is FU173-L8 (already minted); restated here
   so the row-0x14 port co-lands it if the celebration timing touches it.

---

## 9. Discrepancies / errata (prior docs not rewritten)

1. **FU-182 §4 nuance.** FU-182/§5 says the phase-5 arm installs code `0x14`
   "on the goal record"; first-hand the arm also writes `[EBP+0x7B2] = the
   goal-record pointer` (`0x8D46F`) — the engine captures this as
   `team->target = mr->goal_record` (`match_run.c:2499`). No contradiction, an
   added cell.
2. **FU-82 §3.5 stage-1 timing.** FU-82 records the early advance as
   "`FUN_00045001()` non-zero with `+0x89 > 0xB4`"; fresh this session
   `0x87B28/0x87B32` confirm `CMP [EBP+0x89],0xB4 / JLE`, i.e. strictly
   `> 0xB4`. Exact.
3. **FU-136 row-14 clue.** FU-136 classified row 14 "not ported (partial):
   stage 2 and arm timing unported" (OL-9). This slice derives all three stages
   and the release; the "partial" label is superseded by T2 once row 0x14 lands.
4. **FU-173 already minted the release rec** (§6 R21-2). This slice re-derives
   the row-0x14 body byte-level and restates the rec as T2-2; no FU-173 text is
   rewritten.

---

## 10. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):
`get_current_program_info`; `read_memory 0x1106E0` (88 B);
`disassemble_bytes 0x8784C..0x87CBB` (280), `0x87C43..0x87CBB` (36),
`0x8D433..0x8D4A2` (35), `0x8776C..0x8784A` (67), `0x45001..0x45024`,
`0x700F4..0x7013B`.

Engine/source reads (read-only, first-hand):
`src/fifa96_engine/fifa96_match_handlers.c` (`:4029-4176`, `:171`, `:2202`),
`src/fifa96_engine/fifa96_match_run.c` (`:2440-2506`, `:2374-2384`,
`:2726-2758`, `:2828-2900`, `:1370-1410`, `:2170-2245`),
`src/fifa96_engine/fifa96_match_entities.c` (`:60-110`, `:170-210`),
`src/fifa96_loader/fifa96_action_handlers.c` (`:1250-1401`),
`include/fifa96_engine/fifa96_match_handlers.h` (`:1-140`),
`include/fifa96_engine/fifa96_match_run.h` (`:411-510`),
`include/fifa96_loader/fifa96_action_handlers.h` (`:673`, `:700-737`),
`tests/test_event_sequences.c` (`:133-164`, `:236-268`).

Docs cross-cited: FU-82 §3.5/§6/§9, FU-136 §2/§3, FU-173 §4.2/§6/§7/§8/status
appends, FU-182 §1/§4, FU-137 §6, AGENTS.md census rule. No rename/comment/
label/function/script/project save; no git. Write set: this file only.
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.

---

## 11. T2 status (phase-16, 2026-10-12) — row 0x14 LANDED

Implementation slice T2 of the phase-16 plan. **No prior section rewritten.**

**What landed.** `fifa96_match_action_14` (`src/fifa96_engine/fifa96_match_handlers.c`)
implements the §4 3-stage machine over `mr->record`:

* head: `timer89 += (uint16)delta` (§4.1 `0x8785C`); the `[0x157AA3]` write and
  the `0x974F0`/`0x36200` sinks stay legs (FU184-L4).
* stage 0 (§4.3): the `timer89 >= 0x3C` gate; the eight `0x92AC8` draws feed
  `fifa96_action_sequence_scatter_celebration` (the FU-82 helper) into the new
  `mr->celebration.points[6]`; the 6th slot is the helper's clamped `points[4]`
  (the pre-clamp copy is FU184-L1, recorded); the shared arm block copies
  `points[idx+1]` into `+0x4D` and runs the `0x8DCD4` metric / `0x79C50` face;
  `timer89 = 0`, `stage92 = 1`.
* stage 1 (§4.4): the `0x87B53` arm walk with the advance conditions
  `distance < 0x20` (`dword[EBP+0x63]>>16` = **word `+0x65`** = the metric
  distance, an alias-trap correction of the slice's "height" gloss), `timer89 >
  0x4B0`, `idx==3 && timer > 0x12C`, `idx==4 && timer > 0xB4`, the `0x87BE9`
  stage-- re-entry, and `idx==5 -> stage 2`. The `FUN_00045001`
  (`[0x1067EC] & 0xF0`) early advance is staged 0 (FU184-L3).
* stage 2 (§4.5): `match_row_reset` (`0x7DAB4`) then, unless
  `mr->record.side == 1` (`0x87C1A`, FU184-L6), the release: the `0x700F4`
  camera reset and `fifa96_match_run_kickoff_entry` (the T2-2 factoring of the
  begin phase-1 entry) plus the `0x73E08` placement commit.

The `[0x157AA3]` pointer cell, the `0x157AAF` display swap and the `0x8CFAC`/
`0x513EC`/`0x4C324` tails stay legs (FU184-L4/L5).

**T2-2 factoring.** `fifa96_match_run_kickoff_entry(mr)` is extracted from
`fifa96_match_run_begin` (`:2213-2224`) — the phase-1 latch
(`match_run_write_phase`), the per-team state-1 arm
(`fifa96_match_phase_machine_kickoff`) and the entity drain. begin calls it;
the row-0x14 release calls it. Tapes byte-identical (M1 `09b726b7…`, M2
`7b4b241a…`) → no re-pin.

**Evidence.** `tests/test_engine_match_handlers.c::test_action_14_runs_body`
(RED/GREEN: the timer gate, the RNG-recomputed scatter target, the idx walk,
the stage-2 reset + release; mutation-RED with the body stubbed aborts at the
first assertion) and `::test_action_14_live_phase5_route` (the phase-5 goal arm
`fifa96_match_run_situation(mr, 6)` installs code `0x14` on the scanned record
and the wired row dispatches OK).

**Legs.** FU184-L1 (6th-slot pre-clamp copy) stands, recorded in the body; the
id-table anim post (the `0x10F344/46/49/4C` tables) is added as an unported-data
leg; FU184-L3/L4/L5/L6/L7 stand; FU184-L2 (the un-written `y`) is moot in the
engine (the helper writes `y = base.y`).
