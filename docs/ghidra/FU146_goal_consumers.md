# FU-146 — goal consumers

Provenance: recon draft w2, phase-6 wave-1, frozen 2026-10-09; evidence review: PASS with corrections listed inline.

Phase-6 wave-1 recon (plan `docs/superpowers/plans/2026-10-08-fifa96-m2-full-gameplay.md`,
§W2). Read-only slice of the goal-consumer chain: situation-6 queue → frame
gate → scheduler → installer → period-indexed handlers → `FUN_00093944` score
writer → posted-id dispatch. Deliverable: the engine seam that lets a headless
goal increment the score through the native chain.

**Method.** Authoritative program **`/FIFA96.EXE`** (explicit `program`
argument in every call; `/fifa96.exe` and `/fifa96_le.bin` untouched). Ghidra
read-only: no renames, comments, labels, function creation, scripts or project
saves. All windows below are first-hand this slice unless a citation says
otherwise. Addresses are FIFA96.EXE flat/RAM addresses as Ghidra resolves them
(the +0x100000 doc trap is handled by resolving every cited function first;
the word-pair trap `dword[a]>>16 == word[a+2]` is used at `[0x14AF7A]`; all
jump tables are read as raw dwords; gate directions and sign/width are quoted
from bytes).

**Prior work merged, not duplicated.** The writer itself (`FUN_00093944`) and
the handler id-table sketch are already documented in
`docs/ghidra/FU142_installer_arms_scope.md` App. I.10 / App. L and ported as
`fifa96_action_score_event` / `fifa96_match_run_score_event`
(`src/fifa96_loader/fifa96_action_handlers.c:2128`,
`src/fifa96_engine/fifa96_match_run.c:917`). This draft adds the consumer
machinery around it, corrects/extends L.4 where first-hand bytes differ, and
derives the seam. FU-145 (`docs/ghidra/FU145_goal_arming.md`) owns the
producer (`FUN_0007131C` → `FUN_00088940` → `FUN_0008A938(6, side, BX=0)`).

---

## 1. Evidence — situation dispatcher `FUN_0008A938` (`0x8A938..0x8AF25`)

`get_function_by_address 0x8A938` → `FUN_0008a938`, body `0x8A938..0x8AF25`
(RET at `0x8AF25`, 420 disassembled instructions in the window
`0x8A904..0x8AF2F`). `get_xrefs_to 0x8A938` = **39** (fresh; 0x88B44 is the
situation-6 site, App. L.9 census).

**Queue gate (situation 6 takes the queue only under these bytes; all verified):**

| site | bytes | disasm | gate direction |
|---|---|---|---|
| `0x8A944` | `66 85 c0` | `TEST AX,AX` | situation == 0 → `0x8AA7B` (`0f842e010000` `JZ`) |
| `0x8A94D` | `98` | `CWDE` | |
| `0x8A94E` | `83 f8 0b` | `CMP EAX,0xB` | situation == 0xB → `0x8AA7B` |
| `0x8A957` | `80 3d 2a c3 14 00 00` | `CMP byte [0x14C32A],0` | == 0 → `0x8AA7B` (**gate closed → direct path**) |
| `0x8A964` | `83 3d c0 b6 15 00 00` | `CMP dword [0x15B6C0],0` | != 0 → `0x8AA7B` (**situation already pending → direct path**) |
| `0x8A971` | `8b 34 24` / `66 85 f6` / `0f94c0` | `MOV ESI,[ESP]` / `TEST SI,SI` / `SETZ AL` | `[0x15B6B8] = (side_low == 0)` at `0x8A982` (`a3 b8 b6 15 00`) |
| `0x8A97F` | `83 e9 02` | `SUB ECX,2` | queue switch key = situation-2 |
| `0x8A987` | `66 83 f9 08` / `0f87cf000000` | `CMP CX,8` / `JA 0x8AA60` | situation > 10 → id 0xA |
| `0x8A996` | `2e ff 24 85 e0 a8 08 00` | `JMP dword CS:[EAX*4+0x8A8E0]` | **queue table `0x8A8E0`** |

**Queue table `0x8A8E0`** (`read_memory`, 36 B, 9 dwords; immediately before
the function, ends at `0x8A904`):

| situation | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|
| target | `0x8A99E` | `0x8A99E` | `0x8A99E` | `0x8A9CD` | `0x8A9E8` | `0x8A9CD` | `0x8AA60` | `0x8AA23` | `0x8AA23` |

Bodies (first-hand):
* `0x8A99E` (sit 2/3/4): side word 0 → `[0x15B6A8]=9` (`0x8A9A5`), else
  `[0x15B6A8]=0` (`0x8A9B6`); `[0x15B6C0]=1` (`0x8A9BC`); RET.
* `0x8A9CD` (sit 5/7): `[0x15B6A8]=7`; `[0x15B6C0]=1`; RET.
* **`0x8A9E8` (situation 6 — the goal queue)**:
  `0x8A9E8 66 85 f6` `TEST SI,SI`; `0x8A9EB 75 1b` `JNZ 0x8AA08`;
  `0x8A9ED c7 05 a8 b6 15 00 05 00 00 00` → `[0x15B6A8] = 5` (side 0);
  `0x8A9F7 c7 05 c0 b6 15 00 01 00 00 00` → `[0x15B6C0] = 1`; RET (`0x8AA07`).
  `0x8AA08 c7 05 a8 b6 15 00 06 00 00 00` → `[0x15B6A8] = 6` (side 1);
  `0x8AA12` latch `=1`; RET (`0x8AA22`).
* `0x8AA60` (sit 8 and >10): `[0x15B6A8]=0xA`; latch `=1`; RET.
* `0x8AA23` (sit 9/10): `MOV EAX,[ESP-2]; SAR EAX,0x10` → sign-extended side;
  `CMP EAX,1`; `==1` → `[0x15B6A8]=1` (`0x8AA2F`), else `=2` (`0x8AA45`);
  latch `=1`; RET.

**Direct (non-queue) path `0x8AA7B..0x8AB7A`:** `0x8AA7B 66 85 db`
`TEST BX,BX` (param_3). `BX != 0` → `0x8AA80` (`[0x15882C]=side`,
`[0x15882B]=situation`, `FUN_000888FC(8,0,1)`). `BX == 0` → `0x8AAA8`
(`CMP CX,1 / JC`, `CMP CX,4 / JBE`), the situation-1 and 2..4 arms
(`FUN_0008C974(0x1588A4)`/`(0x1590D9)`, `[0x15881E]=situation`,
`[0x158820]=side`, `FUN_000888FC(4,0,1)`), tail `0x8AB63`
(`[0x157AAF]=side`), then **normal table dispatch**:
`0x8AB7A 2e ff 24 85 04 a9 08 00` = `JMP dword CS:[EAX*4+0x8A904]`.

**Normal table `0x8A904`** (52 B, 13 dwords, ends at `0x8A938`):

| sit | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 0xB | 0xC |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| target | `0x8AB82` | `0x8ABAB` | `0x8ABDB` | `0x8ABF3` | `0x8ADC0` | `0x8ADD8` | `0x8AC28` | `0x8ADF0` | `0x8AE08` | `0x8AEC6` | `0x8AEDE` | `0x8AEF6` | `0x8AF0E` |

(raw dwords: `82ab0800 abab0800 dbab0800 f3ab0800 c0ad0800 d8ad0800 28ac0800
f0ad0800 08ae0800 c6ae0800 deae0800 f6ae0800 0eaf0800`.)

**Situation-6 direct body `0x8AC28..0x8ADBF`** (normal table entry for 6):
`0x8AC28 MOV EAX,[0x157A49]; SAR 0x18`; `0x8AC32 [0x157ACB]=0`;
`0x8AC38 CMP EAX,1; JNZ 0x8AC88` — the `[0x157A49]>>24 == 1` arm updates the
`0x1587E4`/`0x1587DA`/`0x1587D8` arrays; the general arm at `0x8AC88`:
`MOV EAX,[ESP-2]; SAR EAX,0x10` (side) → `CALL 0x741B4` (side→score index) →
`0x8AC94 66 ff 04 45 c5 7a 15 00` = **`INC word [EAX*2+0x157AC5]`** (the
direct score write; this is the `add_goal` native path), then the 10-entry
`0x157AC9` ring and `0x157AEE/0x157B02/0x157B0C/0x157B16` goal-log arrays,
`[0x157AB1]=side+1`, `[0x14C1BE]=word 0`, and (unless `[0x157AC2]` is 2 or 3)
`0x8ADAC MOV EAX,5; CALL 0x740A0(5, side)`.

**The phase-2 write (plan item, verified):** normal-table case 0xB body:

| site | bytes | disasm |
|---|---|---|
| `0x8AEF6` | `8b 54 24 fe` | `MOV EDX,[ESP-2]` (side) |
| `0x8AEFA` | `b8 02 00 00 00` | `MOV EAX,2` |
| `0x8AEFF` | `c1 fa 10` | `SAR EDX,0x10` |
| `0x8AF02` | `e8 99 91 fe ff` | `CALL 0x740A0` |

**`FUN_000740A0` (`0x740A0..0x7410D`)** is the phase writer behind those
calls: `[0x157A4E]=[0x157A4D]`; phase byte `[0x157A4A] byte3 = param_1`
(`ram0x00157a4a = CONCAT13(param_1, ...)`); `[0x157AAF]=param_2 low`;
`FUN_0008D098(team block param_2)` then the other block; and on phase == 2:
`FUN_0004C380()`, **`[0x15781D] = 0`** (clears the pan arm — a goal cannot
re-fire while the goal screen runs), `[0x157AB2]=1`, `[0x157A73]=&0x15774C`.

---

## 2. Evidence — scheduler `FUN_000948AC` (`0x948AC..0x949F7`, 83 insns)

`get_xrefs_to 0x948AC` = **2**: `0x4B1A1` (`FUN_0004B100`, the frame body) and
`0x743EA` (the setup/reset block, below; no Ghidra function).

**Frame gate (`FUN_0004B100`), byte-exact:**

| site | bytes | disasm |
|---|---|---|
| `0x4B198` | `80 3d 2a c3 14 00 00` | `CMP byte [0x14C32A],0` |
| `0x4B19F` | `74 05` | `JZ 0x4B1A6` |
| `0x4B1A1` | `e8 06 97 04 00` | `CALL 0x948AC` |
| `0x4B1A6` | `e8 8d fd 03 00` | `CALL 0x8AF38` (clock scan; situation-6 producer side, W1) |

Order matters: the scheduler (and thus the installed handler) runs **before**
the clock scan each frame, so a goal queued by the scan in frame N is consumed
by the handler in frame N+1.

**Second caller `0x743EA`** (setup/reset block `0x74281..0x743xx`):
`0x743CA MOV EDX,0x15774C; 0x743CF MOV AL,[0x14C32A]`; `0x743D4/DA` clears;
`0x743E6 CMP BL,AL` (BL = 0 from `0x7435A`) `; 0x743E8 JZ 0x743EF`;
`0x743EA CALL 0x948AC`. Same `[0x14C32A]` gate.

**Full body (sites; condition → queued id → JMP 0x949E0):**

| site | condition (bytes) | action |
|---|---|---|
| `0x948B2..0x948D1` | `[0x15B688] > [0x15B694]` (`JLE` at `0x948BD`) | `[0x15B6A8] = ([0x15B680]!=5) ? 8 : 7` (`SETNZ AL; ADD EAX,7`), latch=1 |
| `0x948E5..0x94919` | period==2 && `[0x157A83]!=0` && `byte[[0x157A83]]` +0x826 != 0 && `[0x157A97] > 0xF0` | id 3 (`0x94919`) |
| `0x9492A..0x9496C` | period==2 && the possession record is NULL/side-0 (`bVar1`) && `[0x157A97] > 0xF0` | id 4 (`0x9496C`) |
| `0x9497D..0x949AA` | period != 4 && != 2 && `[0x157754] < 0` && `word[0x1577C2] < 0` | id 9 (`0x949AA`) |
| `0x949B7..0x949D4` | period==5 && `word[0x1577C2] < 0` | id 7 (`0x949D4`) |
| `0x949E0..0x949E9` | always (tail) | `CMP dword [0x15B6D4],0` (`83 3d d4 b6 15 00 00`); `JZ`; **`0x949E9 ff 15 d4 b6 15 00` = `CALL dword [0x15B6D4]`** |
| `0x949EF` | — | `XOR EAX,EAX` return |

---

## 3. Evidence — installer `FUN_00092D8C` / `FUN_00092E2C`

`get_xrefs_to 0x92D8C` = **2**: `0x3BF9E` (in `FUN_0003BB1C`, gated at
`0x3BF71 CMP byte[0x14C32A],0 / JZ`), `0x38DCC` (in `FUN_00038630`, the
`EAX==1` screen-machine arm at `0x38DA9 CMP EAX,1 / JNZ 0x39021`).
`get_xrefs_to 0x92E2C` = **1** (`0x92E1F`, inside `FUN_00092D8C`).

**Argument derivation (both callers identical; register ABI EAX=leg, EDX=mode,
BX=side):** `0x38DB2 MOV EBX,[0x14AF5E]; 0x38DB8 MOV EDX,[0x14AF72];
0x38DBE MOV EAX,[0x14AF7A]`, each `SAR x,0x10` → sign-extended **high words**
(= `word[0x14AF60]`, `word[0x14AF74]`, `word[0x14AF7C]`; the word-pair rule).

`FUN_00092D8C` body (`0x92D8C..0x92E28`): `MOV ECX,EAX` (`0x92D90`);
`CALL 0x78824`; `MOVSX EAX,CX` → **`0x92DC5 MOV [0x15B680],EAX`** (game leg);
`MOVSX EAX,DX` → `0x92DD3 MOV [0x15B6BC],EAX` (mode); zeroes
`[0x15B67C]/[0x15B688]/[0x15B6AC]/[0x15B6D4]/[0x15B6B0]`; latch `[0x15B6C0]=1`;
tracked side `[0x15B6B4]`: if `[0x15B684]!=0` → `MOVSX EAX,BX` (`0x92DDF`);
else `[0x1590CC]==0` → 1 (`0x92DEF`), else `[0x159901]==0` → 0 (`0x92E00`),
else -1 (`0x92E08`); copies 3 dwords `0x10F328 → 0x15B6C8`; `CALL 0x92E2C`.

`FUN_00092E2C` body (`0x92E2C..0x92EFF`): `CALL 0x361A4`; `CALL 0x36200(0)`;
zeroes `[0x15B6B8]`, `[0x15B690/69C/6A4]`, `[0x157AB4/AB6/AC9/AC5/AC7]`
(**the score pair `0x157AC5`/`0x157AC7` is zeroed here**), `[0x157AAF/AAE/AB1]`,
`[0x15B68C/698]`; `CALL 0x700F4`; then the leg install:

| site | bytes | disasm |
|---|---|---|
| `0x92EC4` | `66 a1 80 b6 15 00` | `MOV AX,[0x15B680]` |
| `0x92ECA` | `98` / `a3 ac b6 15 00` | `CWDE` / `MOV [0x15B6AC],EAX` |
| `0x92ED0` | `c1 e0 02` | `SHL EAX,2` |
| `0x92ED5` | `05 78 0f 11 00` | `ADD EAX,0x110F78` |
| `0x92EE6` | `8b 00` | `MOV EAX,[EAX]` |
| `0x92EEE` | `a3 d4 b6 15 00` | `MOV [0x15B6D4],EAX` (**installed handler**) |
| `0x92EF3..0x92EF7` | `85 c0 74 06` / `ff 15 d4 b6 15 00` | `TEST EAX,EAX` / `CALL dword [0x15B6D4]` |

**Install table `0x110F78`** (`read_memory` 64 B): legs 0..5 =
`0x93BBC, 0x93E20, 0x940A4, 0x94270, 0x944FC, 0x946C4`; the 64-bit read
continues into adjacent data (`00 00 02 01 ...`), so **leg values are not
bounds-checked** (the installer indexes blindly; the callers gate the range).

**What selects the leg (fresh):** `get_xrefs_to 0x15B680` = **34**, writers
`0x92DC5` (installer arg) and `0x1B7DB` (`FUN_0001B7B8`). The installer arg
comes from `FUN_00038630` `0x38A81..0x38B33`: it sums the `+4` dwords of the
8-byte records at `0x10614C` (first records `{0x47,1},{0x1F6,1},{0x1F7,1},
{0x1F8,1}`) until the running total reaches `[[0x14AFC4]] + [[0x14AFC8]]`,
then `0x38AB6 LEA EDX,[EBX-1]; 0x38ABE CMP EDX,5; JA` → 6-entry jump table
`0x385B8` = `{0x38ACB,0x38AD5,0x38ADC,0x38AE8,0x38AF4,0x38B00}` writing
`dword [0x14AF7C]` = 0 / `EAX(1)` / 2 / 3 / 4 / 5. So **leg = (records
scanned − 1)**; `0x38B1F CMP [0x14AF7C],5` bounds it before the match arm.
The upstream sum inputs (`0x14AFC4`/`0x14AFC8` pointers) are front-end state
(leg 1 below).

---

## 4. Evidence — the eleven `FUN_00093944` sites and their handlers

`get_xrefs_to 0x93944` = **11** (`0x93D98, 0x93DA1, 0x94026, 0x9402F,
0x941E5, 0x941EE, 0x94489, 0x94492, 0x94667, 0x94670, 0x9486E`). Each lies in
one of the six table-`0x110F78` handler windows (none is a Ghidra function):

| leg | handler window (RET) | step table | id table (range) | id → side (first-hand) |
|---|---|---|---|---|
| 0 | `0x93BBC..0x93DE1` | `0x93B80` (6) | `0x93B98` (9: ids 1..9) | 1→1, 2→0, 3→1, 4→none, 5→**0**, 6→**none**, 7/8→none, 9→1 |
| 1 | `0x93E20..0x94071` | `0x93DE4` (6) | `0x93DFC` (9) | same shape as leg 0 |
| 2 | `0x940A4..0x94230` | `0x94074` (5) | `0x94088` (7) | 1→1, 2→0, 3→1, 4→0, 5→**0**, 6→**1**, 7→0 |
| 3 | `0x94270..0x944D3` | `0x94234` (6) | `0x9424C` (9) | 1→1, 2→0, 3→1, 4→none, 5→0, 6→1, 7/8→none, 9→1 |
| 4 | `0x944FC..0x946B1` | `0x944D4` (4) | `0x944E4` (6) | 1→1, 2→0, 3→none, 4→none, 5→0, 6→1 |
| 5 | `0x946C4..0x948A8` | `0x946B4` (4) | none | id==5 → 0; else → 1 (`0x94845`/`0x9485C`) |

Table bytes (fresh): `0x93B80` 60 B = step `{0x93BEE,0x93C59,0x93C7B,
0x93CFE,0x93D2A,0x93DCA}` + id `{0x93D93,0x93D9F,0x93D93,0x93DA8,0x93D9F,
0x93DA8,0x93DA8,0x93DA8,0x93D93}`; `0x93DE4` 60 B analog
(step `{0x93E52,0x93EDB,0x93EFB,0x93F8F,0x93FB9,0x9405A}`, id `{0x94021,
0x9402D,0x94021,0x94036,...}`); `0x94074` 48 B (step `{0x940D6,0x9413F,
0x9414D,0x94179,0x94219}`, id `{0x941E0,0x941EC,0x941E0,0x941EC,0x941EC,
0x941E0,0x941EC}`); `0x94234` 60 B (step `{0x942A2,0x9434C,0x9436E,0x943F1,
0x9441B,0x944BC}`, id `{0x94484,0x94490,0x94484,0x94499,0x94490,0x94484,
0x94499,0x94499,0x94484}`); `0x944D4` 40 B (step `{0x9452E,0x945CF,0x945F9,
0x9469A}`, id `{0x94662,0x9466E,0x94677,0x94677,0x9466E,0x94662}`);
`0x946B4` 16 B `{0x946F6,0x947C3,0x947EF,0x94891}`. (`0x93D93`/`0x94021`/
`0x941E0`/`0x94484`/`0x94662` = `MOV EAX,1; CALL 0x93944`; `0x93D9F`/
`0x9402D`/`0x941EC`/`0x94490`/`0x9466E` = `XOR EAX,EAX; CALL`; `0x93DA8`/
`0x94036`/`0x941F5`/`0x94499`/`0x94677` = `INC dword [0x15B6A0]` no-score.)
**This extends FU-142 L.4: legs 0/1 map the queued goal id 6 to the no-score
counter, while legs 2..5 score it side 1** — the consumer must reproduce the
per-leg table, not a blanket 5→0 / 6→1.

**The writer call sites (fresh `get_xrefs_to 0x93944` = 11):**

| site | handler (leg) | arm |
|---|---|---|
| `0x93D98` | 0 | id 1/3/9 → `EAX=1` (`0x93D93`), `CALL 0x93944` |
| `0x93DA1` | 0 | id 2/5 → `EAX=0` (`0x93D9F`) |
| `0x94026` | 1 | `EAX=1` (`0x94021`) |
| `0x9402F` | 1 | `EAX=0` (`0x9402D`) |
| `0x941E5` | 2 | `EAX=1` (`0x941E0`) |
| `0x941EE` | 2 | `EAX=0` (`0x941EC`) |
| `0x94489` | 3 | `EAX=1` (`0x94484`) |
| `0x94492` | 3 | `EAX=0` (`0x94490`) |
| `0x94667` | 4 | `EAX=1` (`0x94662`) |
| `0x94670` | 4 | `EAX=0` (`0x9466E`) |
| `0x9486E` | 5 | `[0x15B6A8]==5` → 0, else 1 |

**Consumption pattern (identical shape in all six handlers):** the handler
head adds the frame delta to its timer
(`0x93BC4..0x93BD7`: `EDX=[0x15B688]; AX=[0x157A64]; EDX+=EAX;
[0x15B688]=EDX`) and step-dispatches on `[0x15B6B0]`; a middle step tests
`[0x157A4A]>>24 == 2` (`0x93CFE`/`0x93F8F`/`0x9414D`/`0x943F1`/`0x945CF`/
`0x947C3`), **clears the latch** (`0x93D17`/`0x93FA7`/`0x94166`/`0x9440A`/
`0x945E8`/`0x947DC`), advances the step and falls into the latch re-check
(`0x93D2A`/`0x93FB9`/`0x94179`/`0x9441B`/`0x945F9`/`0x947EF`, `JZ` to the
epilogue). The next call's post section reads the pending id
(`[0x15B6A8]`), stashes it (`[0x15B674]`), computes
`minute = [0x15B688]/0x3C` (`[0x15B678]`), accumulates `[0x15B68C]`,
dispatches the id table and calls `FUN_00093944(side)`, then
`FUN_000740A0(0,0)` (`0x93DB2`/`0x94040`/`0x941FF`/`0x944A3`/`0x94681`/
`0x94877`), step++, `[0x15B688]=0`. The last step calls
`FUN_000935A0` when `[0x15B688] > 0xB4`
(`0x93DD6`/`0x94066`/`0x94225`/`0x944C8`/`0x946A6`/`0x9489D`).

**`FUN_000935A0` (`0x935A0..0x9376D`) — advance/restart:** appends the goal-log
triple `{[0x15B670] last scorer, [0x15B674] id, [0x15B678] minute}` to the ring
at `0x15B6D8` (stride 3 dwords, wrap at `0x15B7BC`) keyed by
`[0x15B69C] = word[0x157AC5]+word[0x157AC7]` (total goals), re-installs
`0x110F78[[0x15B680]]` with `[0x15B6C4]=1` (`0x93701`/`0x9370A`), then
`FUN_0004C324()` for leg 0 (also 1/3 when `[0x15B684]==0`) or
`FUN_00054104(0x11, …, leg, [0x15B684])`; `FUN_00037EC4(10)`/`FUN_00037F0C(10|0x14)`
thresholds when a side's score word is 10 (or 0x14 in leg 5 with the tracked
side 1 and side-0 score 0). `get_xrefs_to 0x15B6D4` = 7 (fresh) — the two
scheduler reads plus these writer/reader sites.

---

## 5. Evidence — posted-id dispatch `FUN_0009252C` and the `FUN_000CBC4C` probe

`FUN_0009252C` (`0x9252C..0x92544`, 11 insns; `get_xrefs_to` = **13**: 7 in
`FUN_00093944`, 3 in `FUN_00038630`, `0x39CA0`/`0x395C2`/`0x932DB`):

```
0x9252E MOV EBX,EAX          ; id
0x92530 CALL 0x66E70         ; thunk: 0x66E70 e9 6d 12 04 00 = JMP 0xA80E2
0x92535 TEST EAX,EAX
0x92537 JNZ 0x92542          ; gate closed -> no post
0x92539 MOV EAX,EBX; XOR EDX,EDX
0x9253D CALL 0x66724         ; FUN_00066724(id, 0) — the actual post
```

`FUN_000A80E2`: returns `-1` if `[0x115FCC]==0`, `1` if `[0x114A98]!=0`, else
`0`. `FUN_00066724(id,0)`: gates on `[0x155CE0]`/`[0x155D3C]`, then
`FUN_00065E00(id,0,0,0)` → re-checks the same gate → `FUN_000666A8(id,0)` →
`FUN_000A8084`/`FUN_000A8103`. All of that is HUD/commentary presentation
(OL-89).

`FUN_000CBC4C` (`0xCBC4C..0xCBCB7`) is **not** a hardware RNG: it is a
deterministic 6-limb counter over `0x112E68..0x112E7C`
(`C0=0x112E68 … C5=0x112E7C`): the decompiled body adds the limb chain into
itself, adds 1 to `C5`, propagates the wrap carry up through `C0`, and returns
`C0`. The image seeds the cells (first-hand `read_memory 0x112E68`, 24 B:
`56 0e 2d f2 e9 26 31 88 2f dd 24 c6 9c c4 02 07 7d 3f 35 9e 64 3b df 6f`).
`FUN_00093944` calls it only on the `side != tracked` arm before the `0xD3`
post (App. L.2), and tests `(AL & 3) != 0`. The port can reproduce it exactly
(the engine currently passes `0` — the App. L.6 carried default).

`FUN_00093944` itself (writer, `0x93944..0x93B7D`, one EAX arg = side) is
already derived instruction-by-instruction in FU-142 App. L.2/L.3 and ported
(`fifa96_action_score_event`): `INC word[side*2+0x157AC5]`;
`[0x15B670]=side`; `[0x15B6B4]==-1` early return; tracked-diff update
`[0x15B6A4]`; threshold posts 0xD3/0x9E/0x9F/0xA0 (untracked arm) and
0x9A/0x9B/0x9C/0x9D (tracked arm) through `FUN_0009252C`. **Re-verified this
slice:** same 11 call sites, same window, no new callers.

---

## 6. Derived semantics — the chain end-to-end

```
[W1 arming]  camera pan FUN_0007131C -> [0x15781D]=1, [0x15781E]=class
        |  (clock FUN_0008AF38 tail, phase 2/0x10)
        v
FUN_00088940 goal arm -> FUN_0008A938(EAX=6, EDX=side, BX=0)
        |
        v  [0x14C32A]!=0 (session gate) && [0x15B6C0]==0 (no pending) ?
FUN_0008A938 queue arm @0x8A9E8:  [0x15B6A8] = 5 (side 0) | 6 (side 1)
                                  [0x15B6C0] = 1
        |  (else: direct path @0x8AC88..0x8AC94 INC word[side*2+0x157AC5],
        |   already modelled by fifa96_match_run_add_goal)
        v
per granted frame: FUN_0004B100 0x4B198 CMP [0x14C32A] -> 0x4B1A1 CALL FUN_000948AC
        |                                             then 0x4B1A6 CALL FUN_0008AF38
        v
FUN_000948AC scheduler: timers/period/side may overwrite [0x15B6A8] (3/4/7/8/9);
        |                tail: 0x949E9 CALL dword [0x15B6D4]
        v
installed handler = table 0x110F78[(int16)[0x15B680]]   (leg 0..5)
        |  (installer FUN_00092D8C/92E2C 0x92EC4..0x92EF7; install also
        |   zeroes the score pair and latches)
        v
period handler step machine (step [0x15B6B0], timer [0x15B688]):
   middle step: phase [0x157A4A]>>24 == 2 -> clear [0x15B6C0], step++
   next step:   [0x15B6C0] != 0 -> read [0x15B6A8] -> id table (leg) -> side
        |
        v
FUN_00093944(side):  INC word[side*2+0x157AC5]  (THE SCORE)
                     tracked bookkeeping + threshold commentary ids
        |
        v
FUN_0009252C: CALL 0x66E70 (-> FUN_000A80E2 gate) -> FUN_00066724(id,0)
        (HUD post; unported, OL-89 — derived capture = score_last_event)
   then FUN_000740A0(0,0): phase 0; if phase 2 -> [0x15781D]=0, camera reset
   tail: when [0x15B688] > 0xB4 -> FUN_000935A0 (goal-log ring, re-install)
```

Notes derived from bytes:
* **Two score writers, mutually exclusive by the same gate that routes the
  situation**: `[0x14C32A]==0 || [0x15B6C0]!=0` → direct `FUN_0008A938` case-6
  increment; otherwise → queue → handler → `FUN_00093944`. Wiring both without
  the gate reproduces a goal twice.
* **The handler consumes the goal one frame after it reaches the post step**;
  a goal queued by frame N's scan (which runs after the scheduler) is seen by
  frame N+1's handler. The step-3 latch clear is what makes re-arming work.
* **Leg changes everything**: the id→side table differs per leg (legs 0/1 drop
  queued id 6; legs 2..5 score it side 1; leg 5 trivially id5→0 else 1). The
  tracked side `[0x15B6B4]` (installer param) and the `FUN_000CBC4C` probe
  decide *which commentary id* posts, never whether the score increments.
* The timestamp/home/away (`[0x15B674]/[0x15B678]/[0x15B670]`) and the
  `0x15B6D8` goal-log ring are presentation-side effects of the same path.

---

## 7. Port contract (engine seam; names to use)

Existing, reused:
* `fifa96_match_run_score_event(mr, side, probe)` — the writer (App. L.6);
  increments `mr->score[side]`, tracked bookkeeping, `score_last_event`.
* `fifa96_match_run_situation(mr, situation)` — **stays the table-2/0xB
  path** (it already serves row 01 at `fifa96_match_handlers.c:268`; OL-73).
  The table-1 queue arm lives **solely** in
  `fifa96_match_run_goal_queue` below: S1/S3 must share one 0xB entry point —
  no parallel mechanism (reconciled in the freeze pass).
* `fifa96_match_run_add_goal(mr, side)` — keep as the direct `0x8AC94`
  increment for the `[0x14C32A]==0 || pending` fallback.

New state (suggested fields, `struct fifa96_match_run`; native cell in comment):
* `uint8_t situation_id;` — `[0x15B6A8]` (0 = none; 5/6 = queued goal).
* `uint8_t situation_pending;` — `[0x15B6C0]` latch.
* `uint8_t goal_side_pending;` — `[0x15B6B8]` (side_low == 0) if a consumer
  needs the pre-queue side bit.
* `uint8_t session_gate_14c32a;` — `[0x14C32A]` (seed 1 for a live match;
  native writers are front-end, leg 2).
* `int16_t screen_leg;` — `[0x15B680]` (`0x110F78` index).
* `uint8_t screen_step;` — `[0x15B6B0]`.
* `uint16_t screen_timer;` — `[0x15B688]` frame accumulator.
* `uint16_t screen_period_frames;` — `[0x15B694]` (`duration_table[leg][mode]*60`).
* `uint8_t screen_install_hint;` — `[0x15B6C4]` (1 after `FUN_000935A0`).
* `int8_t goal_tracked_side;` — currently `score_tracked_side` (-1 carried).
* `uint16_t goal_probe_limb[6];` — `0x112E68..0x112E7C` seeded from the image
  bytes above.

New functions:
1. `fifa96_match_run_goal_queue(mr, side)` — the `FUN_0008A938` situation-6
   queue arm: if `session_gate_14c32a && !situation_pending` →
   `situation_id = side ? 6 : 5; situation_pending = 1;` else the direct
   `fifa96_match_run_add_goal(mr, side)` fallback plus the phase-5
   `FUN_000740A0(5, side)` equivalent (exists as `fifa96_match_run_situation`
   table-2 arm). FU-145's producer calls this with the side from `FUN_00088940`.
2. `fifa96_match_run_screen_install(mr, leg, mode, side)` — port of
   `FUN_00092D8C` + `FUN_00092E2C`: set `screen_leg`/mode/tracked side
   (`0x1590CC`/`0x159901` staged), zero `screen_step`/`screen_timer`, zero the
   score pair, seed the period frames from the per-mode duration table
   `0x1110EC[mode*24 + leg]` (dwords; byte offset `mode*0x60 + leg*4`; values
   `{15,15,30,30,60,5}` seconds for legs 0..5 at modes 0..2, verified),
   `situation_pending = 1`, then run the installed handler once (native does).
3. `fifa96_match_run_screen_schedule(mr)` — port of `FUN_000948AC`: the
   `[0x15B688] > [0x15B694]` rollover ids (8/7), the period-2/`[0x157A83]`/
   `[0x157A97]>0xF0` ids 3/4, the `[0x157754]`/`word[0x1577C2]` id 9, the
   period-5 id 7, then `fifa96_match_run_screen_step(mr)`.
4. `fifa96_match_run_screen_step(mr)` — port of the six `0x110F78` handlers,
   table-driven per leg: step table, id table (`enum` of the per-leg
   id→side/none maps above), the phase-2 latch-clear step, the post step
   (`situation_id` → side → `fifa96_match_run_score_event(mr, side,
   fifa96_match_run_goal_probe(mr))`, minute math `[0x15B674/678]`,
   `FUN_000740A0(0,0)` equivalent), and the `>0xB4` advance call.
5. `fifa96_match_run_goal_probe(mr)` — port of `FUN_000CBC4C` over
   `goal_probe_limb[6]` (returns the seeded low limb after one step).
6. `fifa96_match_run_screen_advance(mr)` — `FUN_000935A0` subset: append the
   `{last_side, situation_id, minute}` goal-log triple when
   `word[score0]+word[score1]` changes, reset step/timer, `screen_install_hint=1`,
   re-run the same-leg handler; the `10`/`0x14` presentation thresholds are the
   unported remainder (leg 7).
7. `fifa96_match_run_frame` call order: `if (session_gate_14c32a)
   fifa96_match_run_screen_schedule(mr);` **before** the clock scan seam, then
   the existing phase driver (matches `0x4B1A1` before `0x4B1A6`).

Determinism: everything above is 16-bit-word/byte state plus the deterministic
probe; no wall clock. The step machines run once per 30 Hz granted frame (the
native scheduler is in the frame body, not the render driver).

---

## 8. Numbered legs (unprovable/staged surfaces; no guesses in §7)

1. **Front-end leg selector inputs.** `[[0x14AFC4]] + [[0x14AFC8]]` and the
   `0x10614C` descriptor semantics are unported; §7's `screen_install` takes
   `leg` as an input and the run seeds the native match value once the
   front-end screen state is ported.
2. **`[0x14C32A]` producer.** Writers are front-end only
   (`FUN_0001B7B8` `0x1B7C8` set 1 / `0x1B7D5` clear, `FUN_00032DE0` `0x32DFF`;
   fresh `get_xrefs_to 0x14C32A` = 60 refs, **3 write refs**). A headless match
   must seed the gate (the derived equivalent of the session entry).
3. **Installer args mode/side.** `word[0x14AF74]` (mode) and `word[0x14AF60]`
   (side) writers are the two front-end callers only
   (`0x38BC7/0x38C71/0x38C78`, `0x3BF4C`); no match-code producer found.
   Derived default: mode 0, side 0.
4. **Tracked-side team flags.** `[0x1590CC]`/`[0x159901]` producers unported;
   the derived default keeps `score_tracked_side = -1` (App. L.6) until they
   land.
5. **`FUN_000CBC4C` probe cells.** The exact image seed is recorded above; the
   current engine passes `0` (OL-89). Porting the 6-limb counter is mechanical
   but unverified against a running native.
6. **`FUN_0009252C` display gate.** `FUN_000A80E2`'s `[0x115FCC]`/`[0x114A98]`
   and the `FUN_00066724` chain are presentation (OL-89); the derived capture
   is `score_last_event`.
7. **`FUN_000935A0` remainder.** The `FUN_00037EC4(10)` / `FUN_00037F0C(10|0x14)`
   score-threshold triggers and the `FUN_0004C324`/`FUN_00054104` screen exits
   are unported; the goal-log ring and re-install are the bounded subset.
8. **Handler presentation bodies.** The camera/video copies
   (`0x158897 → 0x15777C/0x15B6C8`, `0x111074`/`0x110FFC` 12-byte records),
   `0x974DC` (called with `EAX=0x1E`, `EBX=0x3C;` its body is
   `FUN_00064EC0`/`FUN_00065CC0`), and the per-step `FUN_0008A938` re-queue
   situations (0xC/3/4/2/1/0xA/0) are visual; the port may stage them as the
   native immediate arms (they are already table-2 rows in
   `fifa96_action_phase_situation`).
9. **Legs 0/1 vs 2..5 id-6 divergence.** The first-hand tables differ; whether
   any real session runs the goal queue under leg 0/1 (gate open) is not
   statically decided. The port reproduces the tables either way.
10. **`[0x15B684]`.** The "screen mode" byte that switches tracked-side source
    and `FUN_000935A0`'s exit path has no located writer this slice (fresh
    operand search `0x0014af`-family only); staged 0.
11. **FU-145 dependency.** The producer (`FUN_00088940` side selection, pan
    arming) is FU-145's slice; §7 assumes its `goal_queue(side)` call.

---

## 9. Risks

* **Double scoring** if the direct and queued writers are wired without the
  `[0x14C32A]`/`[0x15B6C0]` routing (§6) — pin with a gated fixture.
* **Leg seeding** changes the observable id table; a wrong leg silently drops
  queued id 6 (legs 0/1) or mis-sides it — fixture all six tables.
* **Timer cadence** (`[0x157A64]` delta, 0xB4 advance, once per granted 30 Hz
  frame) is part of the state machine; a per-100 Hz-tick call would run the
  screens 3× fast and consume goals early.
* **Word widths**: the score pair wraps at 16 bits (`INC word`), the leg is
  sign-extended from `word[0x14AF7C]`, `[0x15B6B4]` is a ±-1 sentinel —
  keep `int16_t`/`int8_t` widths in the port.
* **Probe determinism**: `FUN_000CBC4C`'s carries are a 6-limb chain; a naive
  `probe++ & 3` is not equivalent (limb interaction). Cite §5 bytes when
  porting (or keep the App. L.6 `0` default and record the divergence).
* **Fresh xref counts** (39/2/11/2/1/13/25/32/7/34/60) are today's; re-pull
  before any "only/sole" freeze wording.

---

## 10. Method record (tool calls, read-only)

`get_function_by_address`: `0x8A938, 0x948AC, 0x92D8C, 0x92E2C, 0x93944,
0x9252C, 0xCBC4C, 0x4B198(->FUN_0004b100), 0x93D98/0x94026/0x941E5/0x94489/
0x94667/0x9486E (all absent = table-referenced handlers), 0x743EA (absent),
0x740A0, 0x935A0, 0x66E70 (absent; thunk bytes read)`.
`decompile_function`: `0x8A938, 0x948AC, 0x92D8C, 0x92E2C, 0x4B100, 0x93944,
0x9252C, 0xCBC4C, 0x974DC, 0x66724, 0xA80E2, 0x543D4, 0x740A0, 0x935A0`.
`disassemble_bytes`: `0x8A904..0x8AF2F` (420 insns), `0x948AC..0x949F7`,
`0x92D8C..0x92F00`, `0x93BBC..0x93E20`, `0x93E20..0x940A4`, `0x940A4..0x94270`,
`0x94270..0x944FC`, `0x944FC..0x94900`, `0x9252C..0x92544`, `0x4B180..0x4B1B0`,
`0x74280..0x74400`, `0x3BF70..0x3BFD0`, `0x38D90..0x38E10`, `0x38A80..0x38B40`,
`0x66E70..0x66E80`.
`read_memory`: `0x110F78` (64), `0x8A8E0` (36), `0x8A904` (52), `0x93B80` (60),
`0x93DE4` (60), `0x94074` (48), `0x94234` (60), `0x944D4` (40), `0x946B4` (16),
`0x1110EC` (96), `0x111074` (120), `0x385B8` (24), `0x10614C` (32),
`0x112E68` (24).
`get_xrefs_to` (fresh): `0x8A938` 39, `0x948AC` 2, `0x92D8C` 2, `0x92E2C` 1,
`0x93944` 11, `0x9252C` 13, `0x15B6A8` 25, `0x15B6C0` 32, `0x15B6D4` 7,
`0x15B680` 34, `0x14C32A` 60 (**3 write refs**: `0x1B7C8` set, `0x1B7D5`
clear, `0x32DFF`), `0x14AF5E/72/7A` 2 reads each.
`search_instructions`: operand `0x0014af` (200+ hits),
`0x0014af60` (1), `0x0014af74` (4).
No writes: no rename/comment/label/function/script/project save. Repo: this
draft file only.

---

## 11. Port landing (S3, 2026-10-09)

Landed from this slice in `src/fifa96_engine/fifa96_match_run.c` (state in
`include/fifa96_engine/fifa96_match_run.h`), with the fixture + unit coverage in
`tests/test_engine_match_frame.c` (8 tests). Both goldens byte-identical, no
re-pin (M1 `09b726b7…`, M2 `2e709151…`).

**Landed mapping.**

| contract item | function | first-hand window / tables |
|---|---|---|
| §7.1 | `fifa96_match_run_goal_queue` (S2) + the `[0x157AC2]∈{2,3}` phase-5 skip (`0x8AD96`/`0x8AD9F`) | `0x8A944..0x8A9E8`, `0x8AA7B`, `0x8AC28..0x8ADB4` |
| §7.2 | `fifa96_match_run_screen_install` | `0x92D8C..0x92E28`, `0x92E2C..0x92EFF`; duration table `0x1110EC` |
| §7.3 | `fifa96_match_run_screen_schedule` | `0x948AC..0x949F7` |
| §7.4 | `fifa96_match_run_screen_step` | six handler windows; step tables `0x93B80/93DE4/94074/94234/944D4/946B4`; id tables `0x93B98/93DFC/94088/9424C/944E4` |
| §7.5 | `fifa96_match_run_goal_probe` | `0xCBC4C..0xCBCB7`; cells `0x112E68..0x112E7C` |
| §7.6 | `fifa96_match_run_screen_advance` | `0x9362B..0x9370A` |
| §7.7 | `fifa96_match_run_frame` call order (`0x4B198` before `0x4B1A6`) | `0x4B180..0x4B1AF` |

**Evidence spot-checks (first-hand `/FIFA96.EXE`, read-only, this slice).**

| claim | tool call | result |
|---|---|---|
| installer bodies incl. the latch `0x92DCD`, the score-pair/`[0x15B6A4]`/totals zeroing, the `0x1110EC` read and the `0x92EF7` handler call | `decompile_function 0x92D8C/0x92E2C` | byte-exact |
| duration table `0x1110EC` 24 dwords: modes 0..2 `{15,15,30,30,60,5}`, mode 3 `{5,1,5,1,3,2}` | `read_memory 0x1110EC` (96 B) | exact (the freeze's modes 0..2 claim extended with mode 3) |
| `0x110F78` legs `{0x93BBC,0x93E20,0x940A4,0x94270,0x944FC,0x946C4}` | `read_memory 0x110F78` (64 B) | exact |
| six step tables + id tables (incl. leg 5's hardcoded id 5→0 else 1 with the >6 no-score bound `0x94655/0x94677`) | `read_memory 0x93B80/93DE4/94074/94234/944D4/946B4`; `disassemble_bytes` all six windows | exact; L.4 correction confirmed |
| the scheduler arms (rollover, leg-2 ids 3/4, id 9, id 7, `0x949E9` tail call) | `disassemble_function 0x948AC` | byte-exact |
| the frame gate `0x4B198 CMP [0x14C32A] / 0x4B1A1 CALL / 0x4B1A6 CALL 0x8AF38` | `disassemble_bytes 0x4B180` | exact |
| the fallback skip `0x8AD96 CMP EAX,2 / JZ` `0x8AD9F CMP EAX,3 / JZ` (byte read `MOV AL,[0x157AC2]` at `0x8AD8A`) | `disassemble_bytes 0x8AD80` | exact |
| `FUN_000CBC4C` fold chain + `INC C5` cascade + `INC EAX` carry-out | `disassemble_function 0xCBC4C` | exact; seed dwords `{0xF22D0E56,0x883126E9,0xC624DD2F,0x0702C49C,0x9E353F7D,0x6FDF3B64}` |
| `FUN_000935A0` ring append (index = new total, `>0x14` shift + slot 19), totals `0x15B698`/`0x15B69C`, hint `0x15B6C4`, re-install `0x9370A` | `disassemble_function 0x935A0` | exact |
| writer probe call site `0x939D1` only on the untracked own==1/opp<3 arm | `disassemble_bytes 0x93944` | exact |
| `[0x15B6B8]` fresh xrefs = 2, both writes (`0x8A982`, `0x92E3F`) | `get_xrefs_to 0x15B6B8` | **write-only**: no consumer; leg |
| `[0x15B6A0]` fresh xrefs = 5, all handler `INC dword` RMWs | `get_xrefs_to 0x15B6A0` | never reset natively; the port resets it per match (hardening) |
| `FUN_00074034`/`FUN_0007417C` (called by `0x92E2C`) are the camera/state reset + side flags + a `JMP 0x78824` slot bind; neither touches `[0x15B6A0]`/`[0x15B6B8]` | `disassemble_function 0x74034/0x7417C` | resets/bind stay legs (begin covers the fresh-match reset) |

**Errata / decisions.**

1. **Setup-step bodies are leg 8, not applied.** The step machines' setup steps
   (leg 0 step 0/2, leg 1 step 0/2, leg 3 step 0/2, leg 4 step 0, leg 5 step 0,
   leg 2 step 0) do their presentation/staging writes and then queue a
   situation via `FUN_0008A938`. Every such call happens with the installer
   latch `[0x15B6C0]==1` (nothing clears it before the phase-clear step), so
   the dispatcher takes the *direct* arm — e.g. leg 0 step 2 (situation 3)
   reaches `0x8ABF3` and writes phase 4. Applying them would flip the live
   phase mid-play, so per §8 leg 8 the port advances the step counter without
   them (the situation re-queues 0xC/3/0xA/4/2/1/0 are likewise visual).
2. **Duration seeded by the installer.** The native computes
   `[0x15B694] = 0x1110EC[mode*24+leg]*60` in the handler's setup step
   (`0x93CB5`-family). Because that step is leg 8, `screen_install` seeds the
   value (contract §7.2). Native divergence: before the setup step runs the
   native cell is 0 (image) and the `timer > period` rollover fires
   id 8 + latch every pre-setup frame; the seeded port suppresses that
   pre-window. Both end at the same state (latch set, no consumer at step 1
   until the `[0x15882A]` gate opens) — unobservable in the ported subset.
3. **The `[0x15781D]` re-arm is not applied.** The setup steps set the pan arm
   with a snapshot copied from `0x158897` (staged by the unported situation-3
   direct arm); the engine carries no `0x158897` producer, and arming with a
   zero triple would make the FU-145 camera armer permanently take the
   already-armed branch. Leg 8.
4. **The `[0x15882A]` clear/set is applied** for legs 0/1/3 (`global_5882a = 0`
   under the staged `[0x15B684]==0` path; leg 10) and the leg-4 arm/snapshot
   clear (`0x9453C..0x9454E`) because those writes hit modelled cells and are
   unambiguous.
5. **`[0x157A97]` and `word[0x1577C2]` are staged caller fields.**
   `screen_actor_age` (`[0x157A97]`, producer `FUN_00072AC4` `0x73414`-family)
   and `screen_lead_z` (`word[0x1577C2]`, the FU-71 lead word; the engine
   conventions OL-72/82 stage 0) default 0, so the leg-2 ids 3/4 and the id
   9/7 arms are byte-exact but dormant until their producers land.
6. **`[0x157754]` is the engine camera z.** The id-9 arm reads the native
   camera-focus dword; the port reads `render.camera.pos_z` (the engine's model
   of the `0x15774C/50/54` triple) as a full dword.
7. **The probe is called lazily at the exact native site.** The writer's only
   `FUN_000CBC4C` call is `0x939D1` on the untracked
   `score[side]==1 && score[other]<3` arm; the post pre-computes the probe iff
   `tracked != -1 && side != tracked && score[side] == 0 && score[other] < 3`
   (the pre-increment equivalent) and passes 0 otherwise. With the carried
   tracked side -1 the probe never advances (native same).
8. **`[0x15B6A0]` reset is engine hardening.** Xrefs are 5 handler RMWs; nothing
   natively clears it. The port resets `goal_no_score` per fresh match
   (init/begin/teardown) for determinism.
9. **Ring index = new total.** `FUN_000935A0` appends at slot `total` (not a
   count) while `total <= 20`, and shifts 1..19 down writing slot 19 beyond;
   `goal_log[21][3]` mirrors this exactly.
10. **Installer hardening.** The native indexes `0x110F78`/`0x1110EC` blindly;
    the port bounds leg 0..5 and mode 0..3 (the callers gate the leg;
    `0x38B1F CMP [0x14AF7C],5`).
11. **`begin` runs the installer** with the derived leg 0 / mode 0 / side 0
    (legs 1/3); the tracked-side pick stays the carried -1 (leg 4/10) and the
    `[0x15B6B8]` side flag is not stored (write-only, errata table).

**Leg status after S3** (this slice's §8 numbering):

| leg | status |
|---|---|
| 1 front-end leg selector inputs | open (derived leg 0) |
| 2 `[0x14C32A]` producer | open (begin seeds 1, S2) |
| 3 installer mode/side producers | open (derived mode 0, side 0) |
| 4 tracked-side team flags | open (carried -1) |
| 5 probe cells vs a running native | open (port exact vs first-hand bytes; no native run comparison) |
| 6 `FUN_0009252C` display gate | open (carried `score_last_event`) |
| 7 `FUN_000935A0` thresholds/exits | open (ring/totals/re-install subset landed) |
| 8 handler presentation bodies | open by design (the setup-step bodies; see errata 1-3) |
| 9 legs 0/1 vs 2..5 id-6 divergence | **closed**: the per-leg id tables reproduce both |
| 10 `[0x15B684]` | open (staged 0) |
| 11 FU-145 dependency | closed (S2 landed `goal_queue`) |

**Tests** (all in `tests/test_engine_match_frame.c`, ASan/UBSan,
`-fno-sanitize-recover=all`): `test_goal_chain_pan_fixture` (S2 fixture updated:
the queued id 5 is now consumed with phase 0), `test_goal_consumer_chain_fixture`
(natural kickoff → settle → queue → consume; RED on BASE), plus
`test_screen_install_state`, `test_screen_schedule_ids`, `test_goal_probe_limbs`,
`test_screen_post_id_tables` (the L.4 correction), `test_screen_step_probe_post`
(the 0xD3 probe arm) and `test_screen_advance_ring`.
