# FU-165 — post-goal restart legs (the end of the phase-0 hold)

Provenance: recon draft, phase-13 second recon wave R13 (2026-10-11), read-only
on **`/FIFA96.EXE`** (explicit `program` argument in every call; no renames,
comments, labels, function creation, scripts or project saves). Scope: the
FU-146 leg-8 restart legs — what ends the post-goal phase-0 hold natively,
its producers/gates, engine reachability and the ranked wiring cost. The M2
v12 tape (`tests/golden/engine/m2-frames.txt`, 85 lines) records 20 post-goal
phase-0 frames (66..85) and stops; the acceptance holds phase 0
(`tests/test_engine_m2.c:830`).

**Method / traps.** All windows below are first-hand this slice unless marked
*cross-cited* (then the frozen FU doc is named). The alias traps were applied
at every load: `dword[a]>>16 == word[a+2]` (the load-bearing reads
`0x158816`→`word[0x15881A]`, `0x15881C`→`word[0x15881E]`,
`0x15881E`→`word[0x158820]`, `0x158828`→`byte[0x15882B]`,
`0x158829`→`byte[0x15882C]`, `0x15882B`→`byte[0x15882E]`,
`0x15882C]`→`byte[0x15882F]`). Fresh censuses this slice:
`get_xrefs_to 0x740A0` = **27** (matches FU-143 §3.1), fresh
`search_instructions` CALL `0x8A938` = **39** (matches FU-149 §1; the goal
handler sites are unchanged). All call targets quoted from the CALL bytes
(rel32 already resolved by Ghidra).

---

## 1. Evidence — where the phase-0 hold sits

**Goal-screen handler leg 0 (window `0x93BBC..0x93DE1`; step table `0x93B80`
= `{0x93BEE,0x93C59,0x93C7B,0x93CFE,0x93D2A,0x93DCA}`, `0x93B80` read
cross-cited FU-146 §4).** Fresh `disassemble_bytes 0x93BBC..0x93DE3`:

* Head `0x93BC4..0x93BD7`: `[0x15B688] += word[0x157A64]` (the granted-frame
  delta), step = `[0x15B6B0]`, `CMP EAX,5 / JA 0x93DDB` (`83 f8 05` /
  `0f 87 f5 01 00 00`), `JMP CS:[EAX*4+0x93B80]` (`2e ff 24 85 80 3b 09 00`).
* **POST step 4 (`0x93D2A..0x93DC5`)**: `CMP [0x15B6C0],0 / JZ 0x93DDB`
  (`83 3d c0 b6 15 00 00` / `0f 84 a4 00 00 00`); `[0x15B6C0]=1` re-latch
  (`0x93D46` `89 0d c0 b6 15 00`); id read `0x93D51`; `[0x15B674]=id`
  (`0x93D5C`); `IDIV 0x3C` minute (`0x93D66`, EBX=0x3C from `0x93D41`);
  accumulator `0x93D73/0x93D80`; id-table dispatch `0x93D79..0x93D89`
  (`MOV EAX,[0x15B6A8]; SUB EAX,ECX(=1); CMP EAX,8; JA 0x93DA8; JMP
  CS:[EAX*4+0x93B98]`); writer/no-score arms `0x93D93..0x93DA6` /
  `0x93DA8`; **`0x93DAE XOR EDX,EDX; 0x93DB0 XOR EAX,EAX; 0x93DB2 CALL
  0x740A0` = `FUN_000740A0(0,0)` → phase 0**; step++ `0x93DB7..0x93DC5`;
  falls into the TAIL step body in the same call.
* **TAIL step 5 (`0x93DCA..0x93DD9`)**: `CMP dword [0x15B688],0xB4`
  (`81 3d 88 b6 15 00 b4 00 00 00`) / `JLE 0x93DDB` / `CALL 0x935A0`.

So the hold is a **pure timer wait at step 5**: the timer zeroed by the POST
(`0x93DBF`) accumulates `word[0x157A64]` (=2) per granted frame; the advance
fires at `> 0xB4` (180) = the **91st post-goal granted frame**. The tape's
post-goal window (20 frames) is ~71 frames short. Engine mirror:
`match_run_screen_kinds[0][5] = MATCH_RUN_SCREEN_TAIL`
(`src/fifa96_engine/fifa96_match_run.c:2536`), TAIL case `:2712`, advance
`:2722`.

**Step 3 is the latch-clear gate that the restart must satisfy**: `0x93CFE
MOV EAX,[0x157A4A] / SAR 0x18 / CMP EAX,2 / JNZ 0x93DDB`; on phase==2
`0x93D17 MOV [0x15B6C0],0` and step++ → the POST waits for the next queue.
The native `FUN_000740A0`'s only phase-2 call site is the situation-0xB arm
`0x8AF02` (fresh `disassemble_bytes 0x8AEF0`:
`0x8AEF6 MOV EDX,[ESP-2]; 0x8AEFA MOV EAX,2; 0x8AEFF SAR EDX,0x10;
0x8AF02 CALL 0x740A0`; 27 call sites fresh, FU-143 §3.1 census) — **phase 2
is written exclusively by situation 0xB** (the resume dispatch).

## 2. Evidence — the advance `FUN_000935A0` (`0x935A0..0x9376D`) does not itself restart play

Fresh `disassemble_bytes 0x935A0..0x9376D` (135 instructions):

| site | disasm | meaning |
|---|---|---|
| `0x935A6`/`0x935AB` | `CALL 0x9343C` ×2 | formation sync: `FUN_0009343C` runs `FUN_0008C974` on both blocks, decrements the changed-formation counters `[0x157B8E]`/`[0x157B8F]`, walks records 0..10 comparing `[0x14C360+team*0x7B+i*4]` vs `[0x157BD2+team*0x2C+i*4]`, stores `[0x1587EC/F4/F8]`/`[0x157BA6]` and calls `FUN_0008C758` (decompile first-hand) |
| `0x935B0`/`0x935B5`, `0x935BA`/`0x935C1` | `EAX=0x1588A4; CALL 0x937DC` / `EAX=0x1590D9; EDX=0; CALL 0x937DC` | per-team roster mask: `FUN_000937DC` gates `[0x14C32A]!=0 && [0x15B6BC]<3`, reads `0x110F90 + [0x15B6AC]*6 + mode*0x24` (3 bytes) and calls `FUN_00093770(team, b0, b1, b2)` = the per-record `+0x9A` active-mask loop (decompiles first-hand) |
| `0x935C8..0x9362A` | ring append (score-total keyed), `[0x15B69C]` | ported (FU-146 §11) |
| `0x9362B..0x93643` | `[0x15B698]=[0x15B69C]`; `[0x15B688]=0`; `[0x15B6B0]=0` | totals/timer/step reset |
| `0x93649..0x936CD` | `[0x15B684]`/tracked-side 10/0x14 tests → `FUN_00037EC4`/`FUN_00037F0C` | match-end thresholds (FU-146 leg 7) |
| `0x936D2..0x9370A` | `EAX=0x110F78[[0x15B680]]`; `[0x15B6AC]=leg`; `[0x15B6C4]=1`; `[0x15B688]=0`; `[0x15B6B0]=0`; `[0x15B6D4]=handler`; `CALL [0x15B6D4]` | **re-install + run step 0** (ported) |
| `0x93710..0x93767` | leg==0 (or leg∈{1,3} && `[0x15B684]==0`) → `FUN_0004C324`; `[0x15B684]!=0` → `FUN_00054104(0x11)` | screen exits |

`FUN_0004C324` (decompile first-hand) = `FUN_00053DC4` + writes `[0x1074A4]`
from the period/goal-side record byte — a **video/presentation selector, not
a phase write**. `FUN_00054104(0x11)` (decompile) sets `[0x14E674]=0x8011` /
`[0x14E684]=0`; 0x11 is not in its gfx case set — a **screen-register write,
not a phase write**. **Conclusion: the advance re-enters the screen machine;
it does not restart play by itself.**

## 3. Evidence — the leg-0 restart chain (the M2 tape's leg)

The engine's begin installs the derived leg 0 (`screen_install(mr,0,0,0)`,
`fifa96_match_run.c:1952`), so the tape's restart is the leg-0 script.

### 3.1 Step 0 — `[0x15882A]=0` + situation 0xC (`0x93BEE..0x93C58`)

* `0x93BEE CMP [0x15B6C4],0 / JNZ 0x93C03`: hint==0 → `0x93BF7
  MOV [0x157A77],0x720` (`c7 05 77 7a 15 00 20 07 00 00`); hint!=0 → sign of
  `[0x157A77]` → `±0x720` (`0x93C0C`/`0x93C13`).
* `0x93C24 XOR AH,AH`; `0x93C2A [0x157A7B]=EDI(0)`; **`0x93C30 MOV
  byte [0x15882A],AH`** (`88 25 2a 88 15 00`); `0x93C3B MOV
  [0x157A7F],EBP(0xB10)`.
* `0x93C36 MOV EAX,0xC` (`b8 0c 00 00 00`); `0x93C41 CALL 0x8A938`
  (`e8 f2 6c ff ff`) — **situation 0xC, side 0 (EDX zeroed at `0x93C28`),
  BX=0 (`0x93C26 XOR EBX,EBX`)**.
* `0x93C4D [0x15B688]=0`; `0x93C53 [0x15B6B0]=step+1`; falls into step 1.

### 3.2 The dispatcher takes the latched direct route for 0xC

`FUN_0008A938` head (fresh `0x8AA7B..0x8AB7F`): situation 0xC is not 0/0xB;
the re-latch `[0x15B6C0]=1` was set by the goal POST (`0x93D46`) and is only
cleared by the step-3 PHASE gate, so `0x8A964 CMP [0x15B6C0],0 / JNZ 0x8AA7B`
sends 0xC direct. `0x8AA7B TEST BX,BX / JZ 0x8AAA8`; `0x8AAA8
CMP CX,1 / JC 0x8AB63`; `0x8AAB2 JBE 0x8AABF` (sit 1); `0x8AAB4 CMP CX,4 /
0x8AAB8 JBE 0x8AB08` (sit 2..4); **`0x8AABA JMP 0x8AB63` — CX=0xC > 4 falls
to the tail** (`0x8AB63 MOV AL,[ESP]` / `0x8AB66 MOV [0x157AAF],AL`;
`CMP CX,0xC / JA 0x8AF1F` at `0x8AB6B`; `JMP CS:[EAX*4+0x8A904]` at
`0x8AB7A`).
Normal table entry 0xC = `0x8AF0E` (fresh): `MOV EBX,1` (`bb 01 00 00 00`);
`MOV EAX,9` (`b8 09 00 00 00`); `XOR EDX,EDX`; `CALL 0x888FC` — **act 9,
side 0, invoke=1**.

`FUN_000888FC` (decompile): `[0x158828]=code`, `[0x158808]=0x1107EC[code]`,
`[0x158818]=0`, `[0x158829]=stage`, invoke if BX != 0. Fresh `read_memory
0x1107EC` (64 B) — **act table = the phase table `0x110794` slots
0x16..0x22** (FU-143 §2.1): `[1]=0x88DC8 (act 1 = phase 0x17)`,
`[2]=0x8922C`, `[3]=0x89FA4`, `[4]=0x89620 (phase 0x1A)`,
`[5]=0x890EC`, `[6]=0x89110`, `[7]=0x89868`, `[8]=0x8A798 (phase 0x1E)`,
**`[9]=0x88F4C` (phase 0x1F)**, `[0xA]=0x8B688 (phase 0x20)`,
`[0xB]=0x8B874`, `[0xC]=0x8B900`. Phase-table cross-cite: FU-143 §2
(phase 0x1F → 0x88F4C; 0x1A → 0x89620; 0x1E → 0x8A798).

### 3.3 Act 9 = `0x88F4C` (fresh window `0x88F4C..0x890E9`)

Head `0x88F51 MOV AX,[0x157A64] / [0x158818] += AX` (the act timer) /
`MOV AL,[0x158829] / CMP AL,3 / JA return / JMP CS:[EAX*4+0x88F3C]`; fresh
`read_memory 0x88F3C` (16 B): stages = `{0x88F81, 0x88FED, 0x89064,
0x890B1}`.

* **Stage 0 (`0x88F81..0x88FEC`)**: `PUSH 0,0x10F330,0x10F32C,0x10F328` →
  `CALL 0x700F4` (camera reset to the `0x10F328/2C/30` triple);
  `0x88F97 XOR AH,AH`; **`0x88F9A MOV byte [0x15882A],AH`** (clear the gate);
  `CALL 0x73E28` (phase reset, FU-143 §1.3); **`0x88FAA MOV
  EDX,[0x157AAC]; 0x88FB0 MOV EAX,0x15; 0x88FB5 SAR EDX,0x18; 0x88FB8 CALL
  0x740A0` = phase 0x15 on the controlled side**; `CALL 0x73E08` (placement
  commit); `0x88FC2..0x88FD1 PUSH 0,PUSH 0,EAX=0x15774C,ECX=EBX=EDX=0, CALL
  0x4C324` (the presentation exit); stage++ (`0x88FD6..0x88FE7`); leg ∈
  {0,1,3} fall through to the stage-1 body (`0x88FF3`/`0x89015`); legs
  2/4/5 jump stage 0→3 (`0x89001..0x89010`).
* **Stage 1 (`0x88FED..`/`0x89015..0x89063`)**: `[0x14C15A]=1`;
  `CALL 0x74110` → decompile = `FUN_0008D014(0x1588A4,…)` then
  `(0x1590D9,…)`; `FUN_0008D014` = the "no free record" walk (records 0..10
  while `[rec+0x9E]!=0 || [rec+0x9A]!=0`, `>10` → 1); `TEST EAX / JZ return`;
  `CALL 0x93850(0)` (a 4-slot table walk `0x157C64`, stride 0x25, matching
  `[slot+0x1F]>>24`), `CALL 0x361B0` (presentation); stage++ falls into
  stage 2.
* **Stage 2 (`0x89064..0x890B0`)**: `CALL 0x93850(0/1)`;
  `DX=[EAX+4]; DH=0; DL &= 0x70`; `==0 → CALL 0x93930; return`; else
  `CALL 0x361A4`; stage++ falls into stage 3.
* **Stage 3 (`0x890B1..0x890E3`)**: `MOV EAX,[0x158816]; SAR EAX,0x10` —
  **alias trap: `[0x158816]>>16 = word[0x15881A]`** (the pair low word at
  `0x158818` is the act timer; fresh `search_instructions` shows **no**
  dword/word write to `0x158816` itself: 19 reads only); `CMP EAX,0x3C /
  JL return`; else **`0x890C4 MOV byte [0x15882A],CL(1)`**, `[0x158818]=0`,
  `[0x158808]=0` (act handler), `[0x158828]=0`, `[0x158829]=0` — the act
  ends and the gate opens.

**The act machine ticks from the clock tail**: fresh
`disassemble_bytes 0x8B650`: `0x8B65B CMP byte [0x14C32A],0 / JNZ 0x8B669;
CALL 0x8BAF0; 0x8B669 CMP dword [0x158808],0 / JZ 0x8B678; 0x8B672 CALL
dword [0x158808]` — inside `FUN_0008AF38`'s tail (after the class-gated
body; session gate only around `FUN_0008BAF0`), so **acts tick every granted
frame, any phase**. The goal scanner `0x8B63E` is a different, phase-gated
block (`0x8B623..0x8B643`, FU-145 §1.5).

### 3.4 Step 1 gate → step 2 (sit 3) (`0x93C59..0x93CFD`)

* `0x93C59 CMP byte [0x15882A],0 / JZ 0x93DDB` — waits for act 9 stage 3.
* When open: step++ (`0x93C75`) **falls into step 2 in the same call**:
  `0x93C7B MOV DH,1; 0x93C87 MOV byte [0x15781D],DH` (pan re-arm);
  MOVSD ×3 `0x158897→0x15777C` and `→0x15B6C8` (`0x93C8D..0x93C9C`);
  duration `0x93C9D..0x93CCA`: `EDX=[0x15B6BC] (mode); EAX=3*mode; EDX=[0x15B680]
  (leg); EDX<<=2; EDX=[EDX+EAX*8+0x1110EC]; EAX=EDX*60; [0x15B694]=EAX`;
  `0x93CCF/0x93CD6 word [0x157AB4]=word[0x157AB6]=0`;
  **`0x93CDD MOV EAX,3; 0x93CE2 XOR EDX,EDX; 0x93CE4 CALL 0x8A938` —
  situation 3, side 0, BX=0 (EBX zeroed `0x93CC3`)**; step→3.
* Step 3 (`0x93CFE`) waits phase==2 (the section-1 gate).

### 3.5 Situation 3 latched direct → the `0x8AB08` arm → act 4

`0x8AAA8 CMP CX,1 / JC 0x8AB63` (`66 83 f9 01` / `0f 82 b1 00 00 00`);
`JBE 0x8AABF`; `CMP CX,4 / JBE 0x8AB08` (`66 83 f9 04` / `76 4e`). For
situation 3 → **`0x8AB08..0x8AB62`** (fresh): `EAX=0x1588A4; CALL 0x8C974;
EAX=0x1590D9; CALL 0x8C974` (the formation-order scan); `0x8AB1C
MOV EAX,[0x157A49]; SAR 0x18; CMP EAX,1; JZ 0x8AB63`; else
`CMP byte [0x157B8E],0 / JNZ; CMP byte [0x157B8F],0 / JZ 0x8AB63`; else
`0x8AB3B MOV EAX,[ESP]`; `0x8AB3E MOV EBX,1`; `0x8AB45 MOV word
[0x158820],AX` (side); `0x8AB4B MOV EAX,4`; `0x8AB50 MOV word [0x15881E],CX`
(situation); `0x8AB57 CALL 0x888FC`; **`0x8AB5C..0x8AB62` RET — no
normal-table re-dispatch on this arm**.

So when the formation counters are clean, **act 4 = `0x89620` (the
phase-0x1A handler)** is invoked with the stored situation 3 / side 0.

### 3.6 Act 4 = `0x89620` (fresh window `0x89620..0x897F1`)

Head `0x89629..0x89646`: timer tick; stage dispatch: `TEST AL,AL / JBE
0x89655` (stage 0), `CMP AL,1 / JZ 0x89775` (stage 1), else `JMP 0x897E8`.

* **Stage 0 (`0x89655..0x89757`)**: the per-team formation-diff loop
  (`local_1c = [0x157B8E]==0`, index walk `[0x157BD2+team*0x2C+i*4]` vs
  `[0x14C360+team*0x7B+i*4]`, record `EAX = 0x1588A4 + side*0x835 +
  i*0xB2`, decompile cross-check `FUN_0009343C`); for a differing record
  with `[rec+0x9A]==0`:
  * `0x896E2 MOV EBX,8`; `0x896E7 EAX=[rec+4]`; `0x896EA EDX=rec`;
    `0x896EC MOV [0x1587F4],EAX`; `0x896F1 MOV EAX,0x76`;
    `0x896F6 MOV [0x1587EC],rec`; `0x896FC CALL 0x8F188` (event);
  * `0x89701/0x89707 [0x1587F8]`; `0x8970C EAX=0xB; CALL 0x4C374`;
  * `0x89716 EDX=[[rec]] (team); DL=[EDX+0x826]`; **`0x8971F MOV EAX,0xB;
    0x8972A MOV ECX,1; 0x8972F CALL 0x740A0` = phase 0xB on the record's
    side**;
  * `0x89736 EDX=0x17; 0x8973B EAX=[0x1587EC]; 0x89740 CALL 0x7D9A4` =
    **install record action 0x17**; `0x89745 [0x158818]=0`; stage++.
  * Cross-cite: action 0x17 = the line-up walk row (`0x1106E0[0x17]=0x84730`;
    FU-81 §2 "Code 17: gate phase 0xB else RESET; +0x4D = -0x680").
* **Stage 1 (`0x89775..0x897E7`)**: `EAX=[0x1587EC]+0x59; CALL 0x4C31C`;
  `EAX=[[0x1587EC]+0x8E]>>24` = byte[rec+0x91] (the action code);
  `AL = 0x110680[code] & 1`; `TEST / JZ 0x897E8`; `EAX=[0x158816];
  SAR 0x10; CMP EAX,0xF0; JLE 0x897E8` — **again `word[0x15881A]` (alias
  trap)**; then the **terminal re-dispatch**: `0x897AE EDX=[0x15881E]`;
  `0x897B4 EAX=[0x15881C]`; `0x897BD [0x158818]=0`;
  `0x897C4 [0x158828]=0`; `0x897CA [0x158829]=0`; `0x897D0 SAR EDX,0x10`
  (= `word[0x158820]` = stored side); `0x897D3 SAR EAX,0x10`
  (= `word[0x15881E]` = stored situation); `0x897D8 EBX=1`;
  `0x897DD [0x158808]=0`; **`0x897E3 CALL 0x8A938`** — re-dispatch
  situation 3 / side 0 with **BX=1**.
* Stage ≥2 entry `0x897E8` returns.

### 3.7 BX=1 → phase 0 + act 8 (phase 0x1E) → BX=0 re-dispatch

`0x8AA80..0x8AAA3` (fresh): `EDX=0; EAX=0; EBX=1; CALL 0x740A0` (phase 0);
`0x8AA8E AL=[ESP]; 0x8AA93 MOV [0x15882C],AL` (side);
`0x8AA98 MOV EAX,8; 0x8AA9D MOV byte [0x15882B],CL` (situation);
`0x8AAA3 JMP 0x8AF1A` → `CALL 0x888FC` (act 8 = `0x8A798`).
Fresh `disassemble_bytes 0x8A798..0x8A8DF`: stage 0 `0x8A7B2` inits
`[0x15882E]/[0x15882F]` (the loop counters) and `[0x158829]=1`; stage 1
`0x8A7D6` `CALL 0x4BEC8 / JNZ return`, then the dual-team record scan for
`byte[+0x91]==0xC` && `byte[+0x99]!=0` && `byte[+0x9A]==0` → `[0x1587F0]` +
`CALL 0x4C374(0x19)`; stage 2 `0x8A895` `CALL 0x4BEC8`, then
`0x8A8A5..0x8A8CE`: clear the act/timer (`[0x158818]=[0x158808]=0`),
`EAX=[0x158828]>>24` = **byte[0x15882B]** (alias trap: stored situation),
`EDX=[0x158829]>>24` = **byte[0x15882C]** (stored side), `EBX=0`,
`CALL 0x8A938` — **the stored situation 3 re-dispatched with BX=0**, then
`0x8A8D3 MOV byte [0x15882B],0xFF`.

### 3.8 The BX=0 re-dispatch → phase 4 → taker 0x11 → phase 2

Situation 3 BX=0, latch still 1 → the same `0x8AAA8` route, but now the tail
`0x8AB63` → normal table `0x8A904[3] = 0x8ABF3` (fresh):
`MOV EAX,[ESP-2]; SAR EAX,0x10; CALL 0x741B4` (side ^ `[0x157ABE]`);
`MOV BX,word [EAX*2+0x157AD4]` (`66 8b 1c 45 d4 7a 15 00`); `INC EBX`;
`MOV word [EAX*2+0x157AD4],BX` (corner counter); **`0x8AC17 MOV EAX,4;
0x8AC1C CALL 0x740A0` = phase 4**.

Phase 4 = the corner (FU-149 §1.5 ported arm `0x8D35B`): install 3 over both
teams; the controlled team's nearest-to-(±0x710,0,±0xB00) record gets action
**0x11**. Row 0x11 (`0x85DE4`, ported T1) resolves via situation 0xB
(`0x863F9`) → `FUN_000740A0(2, side)` at the sole phase-2 site `0x8AF02` →
**phase 2**; then the handler's step-3 PHASE gate opens, the latch clears, and
the POST step waits for the next queued situation — **live play**.

## 4. Per-leg restart table (fresh, all six handler windows)

The re-queue calls were read on their exact windows:

| leg | step 0 re-queue | step 2 re-queue | downstream |
|---|---|---|---|
| 0 | `0x93C41` sit 0xC (`0x93C36 b8 0c 00 00 00`) | `0x93CE4` sit 3 (`0x93CDD b8 03 00 00 00`) | act 9 → act 4 → act 8 → **phase 4** → taker 0x11 → 0xB → phase 2 |
| 1 | `0x93EC1` sit 0xC (`0x93EBC b8 0c 00 00 00`) | `0x93F75` sit 0xA (`0x93F6E b8 0a 00 00 00`) | act 9 → act 2 stage 1 (`0x8AEDE`) → phase 7/6 → taker → 0xB → phase 2 |
| 2 | `0x94120` sit 4 (`0x9411B b8 04 00 00 00`) — step 0 only (5-step table, no gate step) | — | 0x8AB08 → act 4 → act 8 → **phase 8** → keeper 0x1D (FU-151/P3) → 0xB → phase 2 |
| 3 | `0x94332` sit 0xC (`0x9432D b8 0c 00 00 00`) | `0x943D7` sit 2 (`0x943D2 b8 02 00 00 00`) | act 9 → act 4 → act 8 → **phase 3** → taker 0x10 → 0xB → phase 2 |
| 4 | `0x945A5` sit 1 (`0x9459E b8 01 00 00 00`) | — | the sit-1 arm (`0x8AABF..0x8AB06`) → normal entry `0x8ABAB`: `[0x158830]=0x1E0`, `EAX=1, EDX=0, EBX=1, CALL 0x888FC` → act 1 = `0x88DC8` → **phase 1** (cross-cited FU-143 §10: `0x88E82 FUN_000740A0(1,·)`) → state-1 arm → row 01 → 0xB → phase 2 |
| 5 | `0x947AB` sit 0xA (`0x947A6 b8 0a 00 00 00`) | — | act 2 stage 1 → phase 7/6 → taker → 0xB → phase 2 |

All re-queues are side 0 / BX=0 (EDX zeroed immediately before each call;
leg 0 `0x93CE2`, leg 1 `0x93F59`, leg 2 `0x94114`, leg 3 `0x943CB`, leg 4
`0x94597`, leg 5 `0x9479F`). Legs 0/1/3 clear `[0x15882A]` in step 0
(`0x93C30`/`0x93EB6`/`0x94327`); legs 2/4/5 do not (no gate step).

**Note the interpretation:** under the end convention the leg-0 restart
presentation is a **corner** (phase 4), leg 2 a goal kick (8), leg 3 a throw-in
(3), legs 1/5 a free kick/penalty chain (7/6 via act 2) and leg 4 the
kickoff (1). The `0x110F78` handlers are the per-leg match directors whose
cycle is "consume screen situation → phase 0 → (3 s) → restart script"; the
leg selects the restart presentation. Which leg a normal session installs is
the FU-146 leg-1 front-end selector (leg 0 is the engine's derived default).

## 5. Negative results (what does NOT end the hold)

* **The situation/set-piece restart scanner `FUN_00088940` cannot fire**: its
  gate is `phase ∈ {2,0x10}` and `[0x15781D]!=0` (`0x8B623..0x8B643`, FU-145
  §1.5). The hold is phase 0/0x15/0xB — the scanner's phase gate is closed;
  it only arms the *next* goal after phase 2 resumes (no re-arm needed: the
  step-2 pan re-arm `0x93C87` restores `[0x15781D]=1` with the `0x158897`
  snapshot; FU-145 L5).
* **`FUN_000935A0`'s exits are not phase writes** (§2: `FUN_0004C324`
  video selector, `FUN_00054104(0x11)` screen register).
* **The clock machine's phase-0 (class 2) stopclock is not the producer**: it
  drives period completion (`FUN_0008BAF0`/`FUN_0008B9CC`, FU-143 §3.3/§4),
  not a restart; the end-of-hold producer is the screen-machine re-entry.
* **The match-reset `FUN_00073EE0`** (corner counters) is the fresh-match
  reset (FU-149 §1.8), not this chain.

## 6. Engine reachability (wired subset at v12/21-of-80)

| native piece | engine status |
|---|---|
| TAIL timer > 0xB4 → `FUN_000935A0` → re-install + step 0 | **WIRED** (`screen_advance`, `fifa96_match_run.c:2722`, called at `:2712`) |
| step 0 `[0x15882A]=0` | **WIRED** (SETUP0 `:2651..2668`) |
| step 0 sit-0xC queue (`0x93C41`) | **MISSING** (FU-146 §11 errata 1, leg 8 skip) |
| act 9 (`0x88F4C`): phase-0x15 write, `[0x15882A]` producer | **MISSING** — the engine has no act machine; row 0xC = `{phase NONE, act 9}` (`fifa96_action_handlers.c:1145`) and `match_run_situation_side` only executes act 2 (`:2238`); so `set_piece(0xC)` is currently a no-op |
| step-1 gate `[0x15882A]!=0` | **WIRED** (GATE `:2673..2677`) — and it is the engine's **second freeze layer** (SETUP0 clears `global_5882a` at `:2659`, nothing re-sets it at phase 0) |
| step 2 sit-3 queue (`0x93CE4`) | **MISSING** (SETUP2 `:2678..2688` is a bare step++); `word[0x157AB4]/[0x157AB6]=0` also missing (trivial) |
| pan re-arm + `0x158897` snapshot (`0x93C87`) | **SKIPPED by decision** (FU-146 §11 errata 3; `0x158897` producer unported) |
| duration re-seed | WIRED in `screen_install` instead (errata 2) |
| the `0x8AB08` arm + act 4 (phase 0xB + action 0x17) | **MISSING**; the engine's derived route is the table-2 row (P1 erratum 7) |
| act 8 re-dispatch | **WIRED as the P1 compression** (`set_piece` bx!=0, `:2393..2405`) |
| normal-table sit 3 → corner counter + phase 4 + arm | **WIRED** (P1: `match_run_situation_side`, `fifa96_match_run_phase_arm(4)` at `:2231`) |
| taker rows 0x10..0x13 | **WIRED** (phase-8 T1) |
| sit 0xB → phase 2 | **WIRED** |
| step-3 PHASE gate + POST | **WIRED** (`:2693..2711`) |
| act 2 (legs 1/5 sit 0xA) | **WIRED** (FU-150 P2 referee machine, stage 1 = the decision `:4763..4800`) |
| act 1 (leg 4 sit 1) | **MISSING** (row 1 = `{NONE, act 1}`; only act 2 executes) — begin/phase-machine have the kickoff entry |
| row 0x1D (leg 2 phase 8) | **MISSING** (FU-151/P3) |
| `FUN_000935A0` periphery (0x9343C/0x937DC/0x4C324/0x54104/thresholds) | **MISSING** (FU-146 leg 7) |
| phase-0x15 arm (`0x8D040[0x15]=0x8D77B` install 3, fresh `read_memory 0x8D040`: 0x16 entries, `[0xB]=0x8D63E`, `[0xE]=0x8D63E`, `[0x15]=0x8D77B`; phases ≥0x16 read past the table) | MISSING from `fifa96_match_run_phase_arm` (covers 3/4/6/7/8/9/0xD) |

**Verdict: not reachable today — the freeze has two wired layers (TAIL timer,
then GATE) and the two links between them (the re-queues + act 9) are the
missing pieces; every downstream consumer they feed is already wired.**

## 7. Ranked wiring cost (for a future slice; not implemented here)

**R1 — close the freeze on the M2 tape (leg 0), derived-faithful route.**
(a) `SETUP0`: call `fifa96_match_run_set_piece(mr, 0x0Cu, 0u, 0u)` after the
`global_5882a=0` write (the native order `0x93C30` then `0x93C41`).
(b) a derived act-9 producer (the native `0x88F4C` stages 0..3 collapsed):
write `phase 0x15` through `match_run_write_phase` (optional `phase_arm(0x15)`
= install 3, `0x8D77B`) and set `global_5882a=1` on the native-shaped gate
(`word[0x15881A] >= 0x3C`, i.e. effectively the next frame; the native stage
1/2 waits are legs). (c) `SETUP2`: `set_piece(mr, 3u, 0u, 0u)` (leg 0; leg
table for 1/3 is the same shape) + the two `0x157AB4/6` word clears if the
fields are added. Everything downstream already runs: row 3 → corner counter
→ phase 4 → arm → taker 0x11 → sit 0xB → phase 2 → PHASE/POST.
**Cost: one small derived act state (~phase-0x15 + the gate) + 2 queue calls
+ the small 0x15 arm + tests — but the acceptance must extend the tape:
≥91 granted frames past frame 66 before the restart even starts, plus the
taker row's stage cascade; M2 re-pin + `M2_WIRED_MASK`/state pins (row 0x11
joining) move.** Risk: the derived route skips the act-4 line-up, so the
corner taker starts from the goal-celebration positions and its natural
resolution on the tape is unproven (the T1 fixtures stage inputs).

**R2 — native line-up fidelity (visual).** Act 4 (`0x89620`) + act 8
(`0x8A798` timeline, replacing the P1 compression) + the phase-0xB arm
(`0x8D63E` install 0) + record action 0x17 row (`0x84730`, and the
`0x1106E0[0x17]` row wiring) + the 0x9343C formation sync. Same end state
(phase 4 → 2), different on-screen positions; a full slice.

**R3 — per-leg completeness + periphery.** Leg 4's act 1 (kickoff re-entry;
`0x88DC8` stage 0 = phase 1 + `FUN_00073E08`, cross-cited FU-143 §10);
leg 2's row 0x1D (FU-151/P3); `FUN_000935A0`'s 0x9343C/0x937DC/(0x4C324)/
0x54104/thresholds (FU-146 leg 7); the other legs' act-4 route.

**Cheapest single wire in the table (not the tape's leg):** for leg 5 (and
leg 1's step 2) the downstream act 2 is already wired — `SETUP0`'s sit-0xA
call alone would close the freeze on a leg-5 tape (the M2 begin installs
leg 0, so this is a future-tape option, not the v12 closure).

## 8. Numbered legs

1. **FU165-L1 — act-9 stage 1/2 gate semantics.** `FUN_00074110` /
   `FUN_0008D014` (the two-team "no free record" walk), the `0x93850` 4-slot
   `[slot+0x1F]>>24` walk, the `byte[rec+4]&0x70` stage-2 gate + `0x93930`,
   and the `0x361B0`/`0x361A4` presentation calls are byte-derived only; the
   derived compression (immediate gate) diverges in timing. Settle by a
   dynamic trace of `[0x15882A]`/`[0x158829]` after a native goal.
2. **FU165-L2 — `word[0x15881A]` dynamics.** `[0x158816]>>16` is the pair
   high word `word[0x15881A]` (no dword/word write to `0x158816`: 19 reads;
   writers `0x88710` FUN_000886D4 = `period*60`, `0x4B514` FUN_0004B508
   = 0x3C, `0x375E4` FUN_0003749C; the clock `FUN_0008AF38` reads it). The
   act-9 stage-3 `>= 0x3C` and the act-1 stage-1 `< 0x78` (`0x88EF3..0x88F07`,
   fresh) are therefore config/timeline gates, not timer waits — candidate
   erratum for FU-143 §10/§11's "waits `[0x58816]>>16 > 0x78`" wording and
   the engine's tick-based act-1 producer. Settle by tracing
   `[0x15881A]` across a period.
3. **FU165-L3 — the natural leg.** Which leg a normal session installs
   (front-end selector, FU-146 leg 1). The restart presentation is
   leg-dependent (corner / free kick / goal kick / throw-in / kickoff).
4. **FU165-L4 — act-4 stage gates and the terminal re-dispatch.** The
   `0x110680[action] & 1` gate, `FUN_0008C758`, the `[0x157A49]>>24` arm and
   the `[0x14C360]`/`0x157BD2` matrices; whether the stored-situation
   re-dispatch (BX=1) always follows.
5. **FU165-L5 — act-8 timeline.** The `0x4BEC8` wait length (the P1
   compression) and the `[0x1587F0]` record pick (`byte[+0x91]==0xC`).
6. **FU165-L6 — advance periphery.** `0x9343C` formation sync, the
   `0x937DC`/`FUN_00093770` roster-mask table `0x110F90`, the
   `FUN_0004C324`/`FUN_00054104` exits, the 10/0x14 threshold screens
   (FU-146 leg 7).
7. **FU165-L7 — acceptance measurement.** No native run measures the restart
   frame count (≥91 + the taker stages) or the corner taker's natural
   resolution on the tape; the derived route (no act-4 line-up) changes the
   positions the taker sees.
8. **FU165-L8 — the phase-0x15/0xB arms.** `0x8D040` (fresh, 0x16 entries)
   `[0x15]=0x8D77B` (install 3 over 0..10) and `[0xB]/[0xE]=0x8D63E`
   (install 0 over 0..10, skip −1); phases ≥0x16 have no entry (the native
   indexes blindly). Wire only the writes the derived route keeps.
9. **FU165-L9 — dual act/phase identity.** Acts 4/8/9 are the phase handlers
   0x1A/0x1E/0x1F (`0x1107EC = 0x110794+0x16*4`); a port must not run both
   the phase-table dispatch and the act invocation for the same byte.

## 9. Risks

* **Double restart writes.** The sit 0xC/3 calls take the latched direct
  route only because the POST re-latches `[0x15B6C0]=1` (`0x93D46`); wiring
  them without preserving the latch lifecycle would take the queue arm and
  wedge the handler at step 3 (phase never 2).
* **Two freezes, one wire.** Wiring only the sit-0xC call (without act 9)
  moves the engine from the TAIL freeze to the GATE freeze — unobservable
  progress. The gate producer must land with the queue.
* **Alias traps** at `0x158816`/`0x15881C`/`0x15881E`/`0x158828..2C` (five
  strikes scale): any port reading the dword low half instead of the pair
  high word changes the act gates and the re-dispatch arguments.
* **Tape/acceptance churn.** The M2 v12 tape stops 71 frames before the
  advance fires; closing the freeze is a new key tape + re-pin + mask/state
  rider (v13).
* **Cadence.** The screen timer/act timer consume `word[0x157A64]` once per
  granted 30 Hz frame; a per-tick call would run the 3 s hold 3× fast.
* **Derived-route positions.** The corner taker's resolution from the goal
  celebration positions is untested (R1 risk above).

## 10. Method record (tool calls, read-only)

`disassemble_bytes`: `0x93BBC..0x93DE3`, `0x8AA7B..0x8AB7F`,
`0x8AB8A..0x8ABAC`, `0x8ABAB..0x8ABDF`, `0x8ABDB`, `0x8ABF3..0x8AC28`,
`0x8AEF0..0x8AF0E`, `0x8AF0E..0x8AF26`, `0x8A798..0x8A8E0`,
`0x935A0..0x9376E`, `0x88F4C..0x890EC`, `0x89770..0x89880`,
`0x89620..0x89770`, `0x88EA0..0x88EC6`, `0x93F50..0x93F80`,
`0x940D6..0x9413F`, `0x94310..0x94337`, `0x943B8..0x943E0`,
`0x9452E..0x94530`, `0x94590..0x945B0`, `0x94790..0x947B5`,
`0x88EE0..0x88F10`, `0x8B650..0x8B690`, `0x8AF38..0x8AF90`,
`0x4B508..0x4B5C0`.
`decompile_function`: `0x888FC`, `0x4C324`, `0x54104`, `0x886D4`,
`0x9343C`, `0x937DC`, `0x74110`, `0x93770`, `0x8D014`, `0x93850`.
`read_memory`: `0x1107EC` (64 B), `0x88F3C` (16 B), `0x8D040` (132 B).
`search_instructions`: CALL `0x8A938` (39), operand `0x0015882A` (24),
`0x0015881A` (21), `0x00158818` (40, truncated), `0x00158816` (19),
`0x00158808` (21). `get_function_xrefs`: `0x740A0` (27), `0x886D4` (2).
No writes: no rename/comment/label/function/script/project save.
