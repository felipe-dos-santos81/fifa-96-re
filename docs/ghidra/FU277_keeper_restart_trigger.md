Promoted canonical slice — phase-26b (FU277); read-only Ghidra session, authoritative program **`/FIFA96.EXE`**; case-sensitive. No rename/comment/label/function/script/project save.

Phase/Provenance: M2 phase-26b recon-ahead wave `2026-10-14-fifa96-m2-phase26b-recon-ahead`, Task 5 (bounded, read-only derivation), promoted to canonical by the phase-26b close-out. No production code; the derivation and its ruling are unchanged by promotion.

# W26b-5 / FU277 — the keeper restart rows `1D`/`1E`: the live upstream trigger + the `FU269-L5` emit-route divergence

Provenance: M2 phase-26b recon-ahead wave, Task 5, 2026-10-14, **read-only** Ghidra
MCP. Authoritative program exactly **`/FIFA96.EXE`** (`watcom:LE:32:default`,
image base 0, `.image` overlay present, 2706 functions; confirmed this session by
`list_instances` + `get_current_program_info`). Every native address/byte below is
a **fresh** `disassemble_bytes` / `read_memory` / `get_xrefs_to` /
`decompile_function` / `get_function_by_address` of **this** session unless marked
*carried* (the frozen `docs/ghidra/FU*` slice is named). Engine-source facts cite
the built tree by line, read first-hand. Alias traps applied (`dword[x]>>16` = the
word at `x+2`; `>>24` = the byte at `x+3`, e.g. `[0x158828]>>24` = byte `0x15882B`,
`[0x158829]>>24` = byte `0x15882C`; CALL bytes verified). **No writes**: no rename,
comment, label, function, script or project save; `/FIFA96.EXE` untouched; the ISO
never touched; no git command. One new draft file plus the run report.

**Result in one line.** The rows `1D`/`1E` install at phase 8/9 through the
**table-2 direct route** (`FUN_0008A938` BX=0 → `[4]=0x8ADC0`/`[5]=0x8ADD8` →
`FUN_000740A0(8/9)` → `FUN_0008D098` `0x8D607..0x8D62C`); their **situation-4
producer is the goal-mouth scanner** (`FUN_00088940` `0x88BBD`, BX=0) and their
**situation-5 producer is the keeper row `0x1B` stage-4 tail** (`0x77707`,
**BX=1**) — and the engine **does** carry a live-capable situation-4 caller
(`fifa96_match_run_goal_scan` `run.c:3316`, `set_piece(mr, 3u+cond, …)`, `cond==1`
⇒ situation 4), so the missing live piece is **not** a missing call site but the
unported live **`goal_armed` producer** (`FU269-L2`/`FU277-L1`); the row-`1E`
trigger is the unported **keeper CPU ladder** (`FUN_0008D8EC`→`FUN_000782D0`→
`FUN_00077EAC`, `FU277-L2`). The `FU269-L5` divergence is **confirmed and now
precisely characterised**: native `FUN_0008A938(5, record_side, BX=1)` takes the
queue/act-8 detour (phase 0 + the ~22-frame act-8 sweep, phase 9 only at the sweep
terminal `0x8A8CE` with BX=0), while the engine's row-`0x1B` tail
(`fifa96_match_handlers.c:3783`) calls the side-less `fifa96_match_run_situation(mr, 5)`
(`run.c:2948-2950`, side=0/BX=0), reaching phase 9 in one call — a fidelity
divergence, and the very shortcut that makes the engine's phase-9 install
reachable at all.

---

## 1. Step 1 — the upstream trigger: phase 8/9 ← situations 4/5

### 1.1 The row entries and their stage tables (fresh)

Fresh `read_memory 0x1106E0` (128 B): index `0x1D` at `0x110754` = `b0 4e 07 00`
= **`0x74EB0`** (row 1D); index `0x1E` at `0x110758` = `0c 55 07 00` = **`0x7550C`**
(row 1E). Fresh heads: `0x74EB0` = `PUSH EBX/ECX/EDX/ESI/EDI/EBP; MOV EBP,ESP;
SUB ESP,0x28; MOV [EBP-0x8],EAX; …`; `0x7550C` = `… SUB ESP,0x20;
MOV [EBP-0xc],EAX; MOV EAX,[EAX+0x8f]; SAR EAX,0x18` (the stage byte at `+0x92`).

Fresh stage tables (fresh `read_memory`):
* row-1D table `0x74E9C` (32 B) = `{0x74F60, 0x75045, 0x7524B, 0x7529A, 0x7549B}` (5).
* row-1E table `0x754E4` (64 B) = `{0x75611, 0x7565A, 0x75770, 0x75795, 0x75B90,
  0x75D17, 0x75DF3, 0x75E82, 0x7609D, 0x760C6}` (10).

Both confirm FU-151 §2.3/§2.4 and FU-269 §1.1.

### 1.2 The computed install: the `FUN_0008D098` phase-8/9 arm (fresh)

Fresh `disassemble_bytes 0x8D5E0` (this slice) reproduces FU-269 §1.3 byte-exact:

```
0x8d5e0  MOV EBX,0xa
0x8d5e5  MOV EAX,EBP
0x8d5e7  XOR EDX,EDX
0x8d5e9  CALL 0x8ceb8                         ; multi-install code 3 over records 0..0xA
0x8d5ee  MOV EDX,[0x157aac]
0x8d5f4  XOR EAX,EAX
0x8d5f6  SAR EDX,0x18                         ; byte[0x157AAF] = the setter side
0x8d5f9  MOV AL,byte[EBP+0x826]               ; the team's side byte
0x8d5ff  CMP EAX,EDX
0x8d601  JNZ 0x8d814                          ; team side != setter side -> early return
0x8d607  MOV EAX,[0x157a4a]
0x8d60c  SAR EAX,0x18                         ; EAX = phase
0x8d60f  CMP EAX,0x9
0x8d612  SETZ AL
0x8d615  AND EAX,0xff
0x8d61a  XOR ECX,ECX
0x8d61c  ADD EAX,0x1d                         ; EAX = 0x1D (phase 8) or 0x1E (phase 9)
0x8d61f  XOR EBX,EBX
0x8d621  MOVSX EDX,AX
0x8d624  MOV EAX,EBP
0x8d626  MOV dword[EBP+0x7b2],EBP             ; team->target = record 0
0x8d62c  CALL 0x7d9a4                         ; INSTALL code 0x1D/0x1E on record 0
```

The install runs **only** from `FUN_000740A0` (fresh `get_xrefs_to 0x740A0` = 27),
which sets `[0x157A4D] = phase` / `[0x157AAF] = side` and calls `FUN_0008D098` for
both teams (`0x740C8`/`0x740DB`; `FUN_0008D098` fresh `get_xrefs_to` = **2**). So
phase 8 ⇒ row `0x1D` on the setter-side team's record 0; phase 9 ⇒ row `0x1E`.

### 1.3 The situation 4/5 producers

Fresh `read_memory 0x8A904` (52 B) decoded: table-2
`= {0x8AB82, 0x8ABAB, 0x8ABDB, 0x8ABF3, 0x8ADC0, 0x8ADD8, 0x8AC28, 0x8ADF0,
0x8AE08, 0x8AEC6, 0x8AEDE, 0x8AEF6, 0x8AF0E}`; fresh `get_xrefs_to 0x8ADC0` = 2
(DATA from `0x8A914`, COMPUTED_JUMP from `0x8AB7A`), `0x8ADD8` = 2 (DATA from
`0x8A918`, COMPUTED_JUMP from `0x8AB7A`). Fresh `disassemble_bytes 0x8ADA0`:

```
0x8adc0  MOV EDX,[ESP-2]; MOV EAX,0x8; SAR EDX,0x10; CALL 0x740a0   ; [4] sit 4 -> phase 8
0x8add8  MOV EDX,[ESP-2]; MOV EAX,0x9; SAR EDX,0x10; CALL 0x740a0   ; [5] sit 5 -> phase 9
```

**situation 4 (the goal kick).** The goal-mouth scanner `FUN_00088940`
(body `0x88940..0x88C0E`; fresh `get_xrefs_to 0x88940` = 1, from `0x8B63E` in the
clock tail `FUN_0008AF38`), corner/goal-kick arm (fresh `disassemble_bytes 0x88B40`):

```
0x88b53  EAX=[0x157a4a]>>24; 0x88b5b CMP EAX,2; 0x88b5e JNZ 0x88c05   ; phase-2 only
0x88b64  MOV EAX,0x1e; CALL 0x974dc                                   ; the scanner sound
0x88b6e  EAX=[0x1577ca] (ball record); 0x88b75 DL=byte[EAX+0x826]; 0x88b7d XOR DL,1; AND EDX,0xff
0x88b99  ESI=[0x157784]; 0x88ba5 TEST ESI,ESI; 0x88ba7 SETL AL        ; (snap-z sign)
         ... CMP EAX,1/AND/XOR/SETZ ...                                ; cond = (snapz<0)==(ballside==1)
0x88bb9  ADD EAX,3                                                    ; EAX = 3 + cond
0x88bbc  CWDE; 0x88bbd CALL 0x8a938                                   ; EBX=0 (0x88b7b)
```

So `FUN_0008A938(EAX=3+cond, EDX=ball_side^1, EBX=0)`; `cond==1` ⇒ **situation 4**,
`situation 3` otherwise, always **BX=0**.

**situation 5 (the keeper CPU rush).** Row `0x1B` = `0x76D28` (`0x1106E0[0x1B]`;
FU-272 §2.1) stage-4 tail `0x776C1..0x77707` (fresh `disassemble_bytes 0x776C1` +
`0x776E0`):

```
0x776c1  MOV EAX,EBP; 0x776c3 MOV byte[EBP+0x92],4; 0x776ca CALL 0x7dab4   ; stage 4 reset
0x776cf  LEA EDI,[EBP+0x4d]; LEA ESI,[EBP+0x59]; MOVSD x3                 ; target = pos
0x776d8  EDI=[0x157a83]; 0x776de CMP EBP,EDI; 0x776e0 JNZ 0x7770e        ; actor == the slot actor?
0x776e2  EAX=[0x157a49]>>24; 0x776ea CMP EAX,1; 0x776ed JZ 0x7771e       ; phase-0x?? gate
0x776ef  EDX=[EDI]; 0x776f1 DL=byte[EDX+0x826]                           ; EDX = record side
0x776f7  MOV EBX,0x1
0x776fc  MOV EAX,0x5
0x77701  AND EDX,0xff
0x77707  CALL 0x8a938                                                      ; FUN_0008A938(5, side, BX=1)
0x7770c  JMP 0x7771e                       ; 0x7770e: MOV EDX,0x19 (install 0x19)
```

So `FUN_0008A938(EAX=5, EDX=record_side, EBX=**1**)`. The row-`0x1A` sit-5 emit is
the same shape (fresh `disassemble_bytes 0x76B00`: `0x76b0c MOV EBX,1; 0x76b11 MOV
EAX,5; 0x76b16 AND EDX,0xff; 0x76b1c CALL 0x8a938`) — carried as FU-272 §2.2. Both
sit-5 producers emit **BX=1**; the sit-4 producer emits **BX=0**.

### 1.4 The two routes (fresh `decompile_function 0x8A938`)

`FUN_0008A938(situation, side, BX)`:

* **Queue arm** — taken iff `situation ∉ {0,0xB}` **and** `[0x14C32A] != 0`
  (session gate open) **and** `[0x15B6C0] == 0` (no pending): stores
  `[0x15B6B8] = (side==0)` and, for **sit 2/3/4** → `[0x15B6A8] = 9` (side 0) / `0`
  (side≠0); for **sit 5/7** → `[0x15B6A8] = 7`; then `[0x15B6C0] = 1`. **No phase
  write.** (This is the FU-146/FU-149 screen queue; `FU277-L4`.)
* **Direct arm** — reached when `situation ∈ {0,0xB}` **or** the session gate is
  closed **or** a situation is pending. Then:
  * **BX == 0**: `0x8AB63 MOV AL,[ESP]; MOV [0x157AAF],AL; CMP CX,0xC; JA 0x8AF1F;
    JMP CS:[EAX*4+0x8A904]` → **table-2** (`sit 4 → 0x8ADC0 → phase 8`, `sit 5 →
    0x8ADD8 → phase 9`).
  * **BX != 0**: `0x8AAA8 → 0x8AA80 XOR EDX,EDX; XOR EAX,EAX; MOV EBX,1; CALL
    0x740A0` (**phase 0**) `; MOV AL,[ESP]; MOV [0x15882C],AL; MOV EAX,8; MOV
    byte[0x15882B],CL; JMP 0x8AF1A` → `CALL 0x888FC(EAX=8, …)` = **act 8
    invoke-now** (the ~22-frame record sweep).

**The act-8 sweep terminal** (fresh `disassemble_bytes 0x8A8C0` + `0x8A895`):
`0x8A89F MOV [0x158818],AX; 0x8A8A5 XOR EDX,EDX; 0x8A8A7 XOR EBX,EBX` (BX=0);
`0x8A8AB MOV dword[0x158808],0; MOV byte[0x158829],0; MOV byte[0x158828],0;
0x8A8BD EDX=[0x158829]; 0x8A8C3 EAX=[0x158828]; 0x8A8C8 SAR EDX,0x18; 0x8A8CB SAR
EAX,0x18; 0x8A8CE CALL 0x8a938; 0x8A8D3 MOV byte[0x15882b],0xff`. So `EDX =
byte[0x15882C]` (side), `EAX = byte[0x15882B]` (the stored situation), **BX=0** —
the sweep re-dispatches the stored situation/side on the **direct table-2 route**.

> **The sit-5 route natively reaches phase 9 only twice-removed:** `BX=1` ⇒ phase 0
> + act 8; the sweep terminal then re-dispatches the **stored** sit-5 with `BX=0`
> ⇒ table-2 `[5]` ⇒ phase 9. When the queue arm is *open* the sit-5 emit never
> reaches phase 9 at all (it queues id 7).

### 1.5 The engine caller census — and a correction to the brief

The brief's Step 1 asserts "the engine set-piece call census (…`3316`…) passes no
situation 4/5 — the missing live caller." **First-hand this is imprecise.** The
census (fresh read of every site) is:

| engine site | situation arg | note |
|---|---|---|
| `run.c:3058` | `6u` | the sit-6 goal fallback |
| `run.c:3220` | `0u` | period-setup mode 1 |
| `run.c:3230` | `8u` | ET kickoff |
| `run.c:3233` | `1u` | half-time kickoff |
| `run.c:3242` | `8u` | the extra-time gate |
| `run.c:3300` | `2u`, **bx=1** | the goal-scan throw-in arm |
| `run.c:3316` | **`3u + cond`** (`3` or **`4`**), bx=0 | **the goal-scan corner/goal-kick arm** |
| `run.c:3525` | `0x0Cu` | screen leg-0 re-queue |
| `run.c:3574` | `3u` | screen re-queue |
| `run.c:5907` | `out->situation` (variable), bx=1 | the FU-150 ref chain |
| `run.c:6162` | `store_15881e` (stored), bx=1 | the phase-0x1A/act-4 tail |
| `run.c:6406` | `store_15882b` (stored), bx=0 | the act-8 sweep terminal |
| `run.c:6537` | `9u`, bx=1 | the foul restart |

`run.c:3316` (`set_piece(mr, (uint8_t)(3u + cond), (uint8_t)(ball ^ 1u), 0u)`,
`run.c:3316-3317`) is exactly the situation-4 producer of §1.3 (the native
`0x88BBD`): `cond = ((mr->goal_snap_z < 0) == (ball == 1u))` (`run.c:3315`) can be
`1`, so the site **does** pass situation 4. So the census contains a live-capable
sit-4 producer; **no engine site passes situation 5** (the row-`0x1B` tail uses the
side-less wrapper — §3). The true "missing live caller" is therefore **not** a
missing call site but the **unported live `goal_armed` producer** (`FU277-L1`), the
gate on `fifa96_match_run_goal_scan` (`run.c:3284-3285`: `phase ∈ {2,0x10} &&
goal_armed`) — i.e. `FU269-L2`, not a new leg.

---

## 2. Step 2 — the stage ladder (the three restart-emit sites) and `OL-86`

### 2.1 The three situation-0xB restart-emit sites (fresh)

All three are the shared restart tail inside the two keeper stage machines; all
fresh `disassemble_bytes` this session.

**Row 1D stage-3 tail `0x7546E`** (fresh `0x75456`):

```
0x75456  EDX=[EBP-0x8]; EDX=[EDX]; DL=byte[EDX+0x826]; 0x75461 MOV EAX,0xb; AND EDX,0xff
0x7546c  XOR EBX,EBX
0x7546e  CALL 0x8a938                          ; FUN_0008A938(0xB, side, BX=0)
0x75473  XOR AH,AH; 0x75475 MOV byte[0x157ab2],AH   ; clear the hold latch
0x7547b  CALL 0x4c380; … ; 0x75493 INC DL; MOV byte[EAX+0x92],DL   ; stage++
```

**Row 1E stage-3 no-ball exit `0x75B58`** (fresh `0x75B26`):

```
0x75b26  EAX=[EBP-0xc]; 0x75b29 CMP byte[EAX+0x9b],0; 0x75b30 JNZ 0x760df  ; holding -> exit
0x75b36  EDX=[EBP-0xc]; 0x75b39 CALL 0x7dab4         ; RESET the record
0x75b3e  EDX=[EDX]; DL=byte[EDX+0x826]; 0x75b46 MOV EAX,0xb; AND EDX,0xff
0x75b51  XOR EBX,EBX; 0x75b53 MOV ECX,0x1
0x75b58  CALL 0x8a938                                 ; FUN_0008A938(0xB, side, BX=0, ECX=1)
0x75b5d  MOV EDX,0x5; … CALL 0x7d9a4                  ; INSTALL code 5 on the keeper
```

**Row 1E stage-7 outlet `0x76072`** (fresh `0x76050`):

```
0x76050  EAX=[EBP-0xc]; 0x76053 CALL 0x786a0          ; slot hand-off
0x76058  EDX=[EBP-0xc]; EDX=[EDX]; DL=byte[EDX+0x826]; 0x76063 MOV EAX,0xb; AND EDX,0xff
0x7606e  XOR EBX,EBX; 0x76070 XOR CL,CL
0x76072  CALL 0x8a938                                 ; FUN_0008A938(0xB, side, BX=0, ECX=0)
0x76077  MOV byte[0x157ab2],CL; 0x7607d CALL 0x4c380; …
```

All three emit situation `0xB` with **BX=0**. Because `0xB ∈ {0,0xB}`, the
dispatcher head jumps straight to the direct arm (`0x8A94E CMP EAX,0xB; JZ
0x8AA7B`); `0x8AB63 CMP CX,0xC; JA 0x8AF1F; JMP CS:[EAX*4+0x8A904]` → table-2
`[0xB] = 0x8AEF6` → fresh `disassemble_bytes` `0x8AEF6 MOV EDX,[ESP-2]; MOV EAX,0x2;
SAR EDX,0x10; CALL 0x740a0` = `FUN_000740A0(2, side)` = **phase 2**. So the
restart route itself needs no new port (the engine's keeper machines emit
`situation_0b` → `fifa96_match_run_situation(mr, 0x0B)` →
`fifa96_action_phase_situations[0x0B].phase = 2`, `fifa96_match_handlers.c:532`
(sit-1E) / `:631` (sit-1D)); the three sites are **live iff their row is
dispatched** — i.e. Step 1's trigger.

### 2.2 `OL-86` — the post-period 0x0C hold (`0x8BC01..0x8BCE6`) (fresh)

Fresh `disassemble_bytes 0x8BBF0` (256 B) reproduces FU-269 §3.1 byte-exact:

```
0x8bc01  CMP EAX,0xc; JNZ 0x8bce7                  ; the phase-0x0C arm
0x8bc0a  CMP dword[0x158808],0; JNZ 0x8bc20        ; act installed -> skip accrual
0x8bc13  AX=[0x157a64]; ADD word[0x158818],AX      ; the hold timer += frame delta
0x8bc20  EBX=0x960; …; 0x8bc43 CALL 0x36200        ; camera lead
0x8bc57  EAX=[0x158816]>>16; CMP EAX,0x78; JLE 0x8bde7
0x8bc68  CALL 0x45001; TEST AL,AL; JNZ 0x8bc80
0x8bc71  EAX=[0x158816]>>16; CMP EAX,0xb4; JLE 0x8bca3
0x8bc80  CMP dword[0x10f378],0; JZ 0x8bca3
0x8bc89  CALL 0x37f54                             ; the no-ET "match over" screen
0x8bc8e  [0x10f378]=0; [0x158818]=0; JMP 0x8bde7
0x8bca3  CALL 0x45001; …; 0x8bcac CMP EAX,0x168; JLE 0x8bde7
0x8bcbf  CMP byte[0x157ac0],0; JZ 0x8bcd9         ; the extra-time flag
0x8bcc8  EAX=0x13; CALL 0x740a0                   ; ET -> phase 0x13
0x8bcd9  MOV word[0x158822],0x1                   ; MATCH OVER
```

The 0x0C hold (`word[0x158818]` = `[0x158816]>>16`, accrued by the frame delta only
when `[0x158808]==0`) is **landed in the engine** as the derived terminal dwell
(`run.c:4264-4266`, `:4275-4278`, `:4319-4320`, FU-231/232/235); FU-269 §3.3. The
residuals are `FU269-L3` (the `[0x158816]` low half) and `FU269-L4` (`FUN_00045001`
`0x45001`, the `[0x10F378]` gate, `0x37F54`). No new leg here — carried forward as
`FU277-L7` (coupling note only).

---

## 3. Step 3 — the `FU269-L5` emit-route divergence (confirmed + refined)

**Native (row `0x1B` stage-4):** `FUN_0008A938(EAX=5, EDX=byte[[EDI]+0x826]
(record_side), EBX=**1**)` — fresh `0x776f7 MOV EBX,0x1`; `0x776fc MOV EAX,0x5`;
`0x77701 AND EDX,0xff`; `0x77707 CALL 0x8a938`. As §1.4 shows, a `BX=1` sit-5 emit
takes the **queue** (id 7) when open, else the **phase-0 + act-8** detour, reaching
phase 9 only at the act-8 sweep terminal (`0x8A8CE`, BX=0).

**Engine (row `0x1B` stage-4):** `fifa96_match_handlers.c:3782-3783` —
`if (notify_code != 0) return fifa96_match_run_situation(mr, notify_code);` with
`notify_code = 5` (`fifa96_keeper_reposition_b_finish`: `controlled != 0` and
`human_side == 0` ⇒ `*notify_code = 5`, `fifa96_keeper.c:252-263`). And
`fifa96_match_run_situation(mr, s)` = `match_run_situation_side(mr, s, **0u**, 0)`
(`run.c:2948-2950`) → consult `fifa96_action_phase_situations[5] = { .phase = 9u }`
(`fifa96_action_loader...`, `fifa96_action_handlers.c:1136`) → `match_run_write_phase(9)`
+ `fifa96_match_run_phase_arm(9)` (`run.c:2909-2914`).

**Verdict.** The divergence is **real and three-fold**:
1. **Route**: native `BX=1` (queue/act-8 detour); engine `BX=0` semantics (the
   side-less direct wrapper — it runs neither the queue switch nor the BX=1
   `set_piece` branch, `run.c:3144-3157`).
2. **Side**: native `EDX = record_side`; engine hardcodes `side = 0`.
3. **Timing**: native reaches phase 9 only at the act-8 sweep terminal (~22 granted
   frames later); the engine reaches phase 9 on the emit frame.

The engine *does* carry the de-compressed act-8 branch (`run.c:3144-3157`,
FU-241 §6.3); the row-`0x1B` tail simply does not use it (`fifa96_match_run_situation`
≠ `fifa96_match_run_set_piece(..., 1u)`). So the fix shape is a route-swap, not a
new machine: `return fifa96_match_run_set_piece(mr, notify_code, record_side, 1u);`
(or the native's side from the record) — **but** that swap changes when phase 9 is
written (deferred to the sweep terminal) and, with the queue gate open, makes the
emit *not* reach phase 9. Filed as `FU277-L3`; the side divergence is cosmetic for
the phase-9 install (phase/arm are side-independent; `match_run_situation_side` uses
`side` only for the sit-3 corner counter, `run.c:2903-2904`).

---

## 4. Step 4 — port shapes and the live-trigger ruling

### 4.1 Live-vs-leg ruling

| chain | native producer | engine state | ruling |
|---|---|---|---|
| row `1D` install (phase 8) | `FUN_0008D098 0x8D607..0x8D62C` ← `FUN_000740A0(8)` ← table-2 `[4]=0x8ADC0` ← **sit 4** ← goal scanner `FUN_00088940 0x88BBD` (BX=0) | landed: `fifa96_match_run_phase_arm` `run.c:2853-2861` + `fifa96_match_run_goal_scan` `run.c:3316` (sit-4-capable) | **LIVE-LEG** — `fifa96_match_run_goal_scan` runs every granted frame (`run.c:4243`) with a sit-4 caller (`:3316`); gated on `phase ∈ {2,0x10} && goal_armed && goal_zone==0 && |snap_z|>0xB20` (`run.c:3284-3285`, `:3305`, `:3293`). **The gate is `goal_armed`**, whose live producer is the camera pan to the mouth (`fifa96_match_goal_arm` `run.c:4177-4179` ← `FUN_0007131C` ← `FUN_000736AC 0x73B9B`) — that producer is the ~4400-step band walk (FU-168-L5, `ENGINE.md:1752`). **Tape-dormant**: the m 42 `goal_armed` is a staged fixture input. Not smoke-visible. |
| row `1E` install (phase 9) | same arm ← `FUN_000740A0(9)` ← table-2 `[5]=0x8ADD8` ← **sit 5** ← row `0x1B` stage-4 `0x77707` (**BX=1**) / row `0x1A` `0x76B1C` (**BX=1**) | landed (`run.c:2853-2861` + `fifa96_match_action_1B` stage-4 `fifa96_match_handlers.c:3782-3783`) | **fixture-only (leg)** — (a) the emit is **route-divergent** (`FU277-L3`); (b) the sit-5 host rows are installed by the unported **keeper CPU ladder** (`FU277-L2`) or row-`0x1F`'s dive installer `0x76557` (needs phase 6, the FU-150 LIVE-LEG chain, `FU277-L5`); (c) tape-isolated (`skip_9a`, `test_engine_m2.c:2605`). |
| the restart emit (`0x7546E`/`0x75B58`/`0x76072`) | sit `0xB` → `0x8AEF6` → phase 2 | landed (`fifa96_match_run_situation(mr, 0x0B)`; `fifa96_match_handlers.c:532/631`) | **live iff the row is dispatched** — the route needs no new port; dead only because neither row is reached live on the tape. |
| `OL-86` 0x0C hold | `FUN_0008BAF0 0x8BC01..0x8BCE6` | landed (derived dwell, FU-231/232/235) | **resolved**; residuals `FU269-L3/L4` (`FU277-L7`). |

### 4.2 Port shapes (all the route pieces are already landed; the trigger is the work)

* **Row entries**: `{0x1D, fifa96_match_action_1D}` / `{0x1E, fifa96_match_action_1E}`
  (`fifa96_match_handlers.c:5518/5520`).
* **Install producer**: `fifa96_match_run_phase_arm` phase-8/9 arm (`run.c:2853-2861`);
  mapping `fifa96_action_phase_situations[4] = {8u}`, `[5] = {9u}`
  (`fifa96_action_handlers.c:1126-1138`).
* **Situation runner**: `fifa96_match_run_set_piece` (queue + direct + the BX!=0
  act-8 branch, `run.c:3111-3199`) → `match_run_situation_side` (`run.c:2896-2946`).
* **Restart route**: `fifa96_match_run_situation(mr, 0x0B)` → phase 2.
* **The two missing triggers** (the deliverable): the **live `goal_armed`
  producer** (`FU277-L1`) and/or the **keeper CPU ladder** (`FU277-L2`) — *not* the
  keeper machine. The row-`0x1B` emit route-swap (`FU277-L3`) is a fidelity fix to
  be made **with** the ladder port (a route change alone, without the ladder, still
  cannot dispatch row `0x1B` live).

### 4.3 Live upstream trigger — one line

* Row **`1D`**: `fifa96_match_run_goal_scan`'s situation-4 emit (`run.c:3316`;
  native `FUN_00088940 0x88BBD`), live once `goal_armed` has a live producer (the
  FU-145 armer `FUN_0007131C` reached with the camera out of bounds,
  `FUN_000736AC 0x73B9B`).
* Row **`1E`**: the row-`0x1B` stage-4 sit-5 emit (`0x77707`; engine
  `fifa96_match_handlers.c:3783`), live once row `0x1B` is installed by the keeper
  CPU ladder (`FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`) or row-`0x1F`'s dive.

No NULL row is wired by this slice; the census is unchanged (**59/80**).

---

## 5. Numbered open legs (native owners)

1. **FU277-L1 — the row-`1D` live trigger: the live `goal_armed` producer.** The
   sit-4 chain is ported and sit-4-capable (`run.c:3316`), but `fifa96_match_goal_arm`
   (`run.c:2982-3018`) is reached only with the camera out of bounds (`run.c:4177-4179`);
   the armer needs the pan history to reach the mouth (`ENGINE.md:1752`, FU-168-L5).
   Native owners: `FUN_000736AC` (`0x736AC`, caller `0x73B9B`) / `FUN_0007131C`
   (`0x7131C..0x71B98`, the armer; note **`0x713C8` is an interior instruction, not
   the function entry** — fresh `get_function_by_address 0x713C8` → body
   `FUN_0007131C`). Cross-owner: FU-145/FU-168. ≡ FU269-L2.
2. **FU277-L2 — the row-`1E` live trigger: the keeper CPU ladder installer.** The
   sit-5 producers (rows `0x1A`/`0x1B`) have no live installer; the ladder
   `FUN_0008D8EC` (`0x8D8EC`) → `FUN_000782D0` (`0x782D0`) → `FUN_00077EAC`
   (`0x77EAC`) installs `0x1A` (`0x78227/0x7824F/0x78277`), `0x1B` (`0x78299`),
   `0x1C` (`0x782B6`) — fresh `get_xrefs_to` `0x77EAC`(1)/`0x782D0`(1)/`0x8D8EC`(4,
   from `FUN_0004B100` ×2 + `0x74429/0x74430`). Unported. Native owner:
   `FUN_00077EAC` (`0x77EAC`). Cross-owner: FU186-L5/L6/L7, FU180-L4/L5, FU-151 §8
   item 8. ≡ FU272 T2a-L1.
3. **FU277-L3 — the `FU269-L5` emit-route divergence.** Native row-`0x1B` stage-4
   `FUN_0008A938(5, record_side, BX=1)` (`0x776F7/0x776FC/0x77707`) vs the engine's
   side-less `fifa96_match_run_situation(mr, 5)` (`fifa96_match_handlers.c:3783` →
   `run.c:2948-2950`, side=0/BX=0). Route + side + timing differ (see §3). Native
   owners: row-`0x1B` body `0x76D28` stage-4 `0x776C1`; `FUN_0008A938` (`0x8A938`,
   the queue vs direct-BX split); the act-8 sweep terminal `0x8A8CE`.
4. **FU277-L4 — the `situation-4/5` queue consumer (id 7).** The queue arm
   (`0x8A9CD → [0x15B6A8]=7 / [0x15B6C0]=1`) drains into the **FU-146/FU-149 screen
   machinery**, never to phase 9 (per-leg `match_run_screen_ids` id-7 mapping returns
   −1). Native owners: `FUN_0008A938` (`0x8A938`) / the `0x8A9CD` store; the FU-146
   screen chain. ≡ FU269-L1, FU272 T2a-L5.
5. **FU277-L5 — the row-`0x1B` second installer + phase-6 liveness.** Row-`0x1F`'s
   no-slot dive installs `0x1B` invoke-now (`0x76557 MOV EDX,0x1b; CALL 0x7d9a4`,
   fresh `0x76540`, EBX=ECX=1); row `0x1F`'s installer is the phase-6 arm
   (`FUN_0008D098 0x8D4A2/0x8D558`), whose only located writer is the FU-150
   foul/ref chain (`run.c:5956-5999`, LIVE-LEG). Native owners: row-`0x1F` body
   `0x76380` / `FUN_0008D098` (`0x8D098`). Cross-owner: FU-176, FU-150.
6. **FU277-L6 — the row-`0x1E` stage-3/7 restart reach.** Even with row `0x1E`
   installed, its stage-3 (`0x75B58`) / stage-7 (`0x76072`) tails need the claim
   machine to advance; the claim anim bridge `FUN_0006E598` (R11-L1) is unported
   ("without it the claim machine's stages 5–7 are unreachable", FU-163 §7). Native
   owner: `FUN_0006E598`. ≡ FU272 T2a-L4.
7. **FU277-L7 — `OL-86` residuals (carried).** `word[0x158818]`'s dword low half
   (`FU269-L3`) and the `FUN_00045001`/`[0x10F378]`/`0x37F54` gates (`FU269-L4`).
   Native owner: `FUN_0008BAF0` (`0x8BAF0`).

---

## 6. Fresh-tool inventory (this session, all `/FIFA96.EXE`, read-only)

`list_instances`; `get_current_program_info`; `get_function_by_address` `0x713C8`,
`0x88940`; `decompile_function` `0x8A938`; `get_xrefs_to` `0x8A938`(39),
`0x8D098`(2), `0x740A0`(27), `0x77EAC`(1), `0x782D0`(1), `0x8D8EC`(4),
`0x76557`(0), `0x78299`(0), `0x8ADC0`(2), `0x8ADD8`(2), `0x7131C`(1), `0x713C8`(0),
`0x88940`(1), `0x88B53`(1); `read_memory` `0x8A904`(52 B), `0x1106E0`(128 B),
`0x74E9C`(32 B), `0x754E4`(64 B); `disassemble_bytes` `0x8D5E0`, `0x776E0`,
`0x75456`, `0x75B26`, `0x76050`, `0x8ADA0`, `0x88B40`, `0x8A938`, `0x8BBF0`,
`0x8AA60`, `0x8AB40`, `0x8AF00`, `0x76B00`, `0x76540`, `0x78280`, `0x776C1`,
`0x8A8C0`, `0x8A895`, `0x74EB0`, `0x7550C`, `0x71390`. Engine reads:
`src/fifa96_engine/fifa96_match_run.c` (`2840-3340`, `3490-3578`, `4160-4269`,
`5895-5911`, `6145-6174`, `6385-6408`, `6525-6542`),
`src/fifa96_engine/fifa96_match_handlers.c` (`500-547`, `3676-3786`, `5505-5524`),
`src/fifa96_loader/fifa96_action_handlers.c` (`1121-1165`),
`src/fifa96_loader/fifa96_keeper.c` (`230-275`). No write outside this draft; no
tool/ISO/Ghidra-project change.

**Carried docs (read, not rewritten):** FU-269 §1-4 (the install + `OL-86` +
`FU269-L1..L5`), FU-272 §1-5 (R11-L4, the CPU ladder, `T2a-L1..L6`), FU-151
§2.3/§2.4/§2.5/§5/§8, FU-180 §2/§4, FU-186 §1/§4, FU-163 §7 (R11-L1), FU-241 §6.3,
FU-145 S2, `ENGINE.md:1752`.
