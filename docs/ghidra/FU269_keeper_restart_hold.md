# W26-6 / FU269 — the keeper restart rows `1D`/`1E` live producers + the post-period hold (`OL-86`)

Provenance: M2 phase-26 wave W26-6, 2026-10-14, read-only recon. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument; confirmed this session by `list_open_programs` — `/FIFA96.EXE`,
`watcom:LE:32:default`, 2706 functions, `.image` overlay present, image base 0).
Every address/byte below is a fresh `disassemble_bytes` / `read_memory` /
`get_xrefs_to` / `decompile_function` of **this** session unless marked
*cross-cited* (the frozen FU doc is named). LE-image globals sit at
**`+0x100000`** from the file offset; the addresses here are `/FIFA96.EXE`
runtime (program) addresses. Alias traps applied: `dword[x]>>16` = the word at
`x+2`, `>>24` = the byte at `x+3` (here `[rec+0x8F]>>24` = the stage byte at
`+0x92`, `[rec+0x8B]>>24` = the sector byte at `+0x8E`, `[0x157A4A]>>24` = the
phase byte at `0x157A4D`). Engine/golden source read first-hand:
`src/fifa96_engine/fifa96_match_run.c`, `fifa96_match_handlers.c`,
`src/fifa96_loader/fifa96_action_handlers.c`, `fifa96_keeper.c`,
`tests/test_engine_m2.c`, `tests/test_engine_match_frame.c`, the golden
`tests/golden/engine/m2-frames.txt`. **No writes**: no rename, comment, label,
function, script or project save; `/FIFA96.EXE` untouched; the ISO never
touched; no git commands. This is one new draft file plus the run report.

**Result in one line.** The rows `1D`/`1E` have **no immediate installer**
(fresh `search_instructions` + action-table xrefs); the computed installer is
the **phase-8/9 arm of `FUN_0008D098` (`0x8D607..0x8D62C`)**, reached only from
`FUN_000740A0(phase 8/9)` (2 callers), whose producers are the **situation-table-2
entries `[4]=0x8ADC0` (phase 8, the goal kick) and `[5]=0x8ADD8` (phase 9, the
keeper CPU rush)**; both mappings are **landed** in the engine
(`fifa96_action_phase_situations[4/5]` + `fifa96_match_run_phase_arm` +
`{0x1D, fifa96_match_action_1D}`/`{0x1E, fifa96_match_action_1E}`). The three
restart-emit call sites (`0x7546E`/`0x75B58`/`0x76072`) are byte-exact and each
needs only the **shared situation-0xB route** (already ported → phase 2). The
`OL-86` post-period 0x0C hold was **later modelled** by FU-231/232/235 (the
phase-0x0C terminal dwell); the remaining gap is the `0x158818` timer's other
writers and the `0x45001`/`0x10F378`/`0x37F54` gates. The tape's m 42 isolation
(explicit `skip_9a`) keeps row 1E **off** the tape; the live reachability of the
row-1D/1E installs is gated on the situation-4/5 upstream producers.

---

## 1. Step 1 — Rows `1D`/`1E`: entries, the computed install, and the m 42 isolation

### 1.1 The row entries (fresh)

The action-row table `0x1106E0` (fresh `read_memory 0x1106E0`, 128 B). Index
`0x1D` sits at `0x1106E0 + 0x1D*4 = 0x110754`; index `0x1E` at `0x110758`:

* `0x1106E0[0x1D] = 0x74EB0` (fresh `get_xrefs_to 0x74EB0` = **1**, DATA from
  `0x110754`) — **row 1D** (the keeper closedown / clearance).
* `0x1106E0[0x1E] = 0x7550C` (fresh `get_xrefs_to 0x7550C` = **1**, DATA from
  `0x110758`) — **row 1E** (the keeper claim / 10-stage machine).
* `0x1106E0[0x1F] = 0x76380` — row 1F (the keeper input handler; FU-186 §1.4).

Row-1D head `0x74EB0` (fresh `disassemble_bytes`): the 6-byte prologue
`53 51 52 56 57 55 89 e5` then `SUB ESP,0x28`, `[EBP-0x8]=EAX` (rec),
`[EBP-0x4]=0`, `[EBP-0x10]=[0x744D0]`, then the side/slot-vector setup
(`EDX=[[rec]+0x826]` side; `ESI = [0x157C5E] + side*7*8`; `SETGE AL` from
`[0x15777C]`), reaching the stage dispatch at `0x74F58`:

```
0x74f58  JMP dword CS:[EAX*4 + 0x74E9C]      ; 5-stage machine
```

Row-1D stage table `0x74E9C` (fresh `read_memory 0x74E9C`, 32 B) =
`{0x74F60, 0x75045, 0x7524B, 0x7529A, 0x7549B}` (5 entries); the dispatch is the
single DATA xref (`get_xrefs_to 0x74E9C` = **1** from `0x74F58`). Row-1E head
`0x7550C` reads `EAX=[rec+0x8F]; SAR 0x18` (the stage at `+0x92`), the claim gate
`CMP EAX,6`, and dispatches at `0x75609`:
`JMP dword CS:[EAX*4 + 0x754E4]`; the row-1E stage table `0x754E4` (fresh
`read_memory`) = `{0x75611,0x7565A,0x75770,0x75795,0x75B90,0x75D17,0x75DF3,
0x75E82,0x7609D,0x760C6}` (10 entries; `get_xrefs_to 0x754E4` = **1** from
`0x75609`). These reconfirm FU-151 §2.3/§2.4.

### 1.2 No immediate installer (fresh census)

Fresh `search_instructions` (program scope, 235055 insns scanned, not truncated):

| query | result |
|---|---|
| `MOV EDX, 0x1D` | **2** — `0x7F0BB` (a `0x92820` argument, overwritten before any `0x7D9A4` per FU-151 §3.5 erratum 5) and `0xC9B01` (`EDX,0x1D4C0`, not a code) |
| `MOV EDX, 0x1E` | **10** — none in the match core; `0x3F6B9` (loader region), the rest are `0x1E0` (`0x1E**0**`) false matches |

So **no immediate install** of codes 0x1D/0x1E exists in the match core; the
installs are computed. This is the FU-151 §5 leg 1 that this slice resolves
(native owner named below).

### 1.3 The computed install: `FUN_0008D098` phase-8/9 arm (fresh)

`FUN_0008D098` (body `0x8D098..0x8D820`; fresh `get_xrefs_to 0x8D098` = **2**,
both `UNCONDITIONAL_CALL` from `0x740C8`/`0x740DB` in `FUN_000740A0`). Fresh
`decompile_function 0x8D098` gives the phase switch; the `case 8: case 9:`
arm is `0x8D5D9..0x8D62C`. Fresh `disassemble_bytes 0x8D5E0` (raw, this slice):

```
0x8d5e0  MOV EBX,0xa
0x8d5e5  MOV EAX,EBP                         ; team block
0x8d5e7  XOR EDX,EDX
0x8d5e9  CALL 0x8ceb8                        ; multi-install code 3 over records 0..0xA
0x8d5ee  MOV EDX,[0x157aac]
0x8d5f4  XOR EAX,EAX
0x8d5f6  SAR EDX,0x18                        ; EDX = byte[0x157AAF] = the setter side
0x8d5f9  MOV AL,byte[EBP+0x826]              ; the team's side byte
0x8d5ff  CMP EAX,EDX
0x8d601  JNZ 0x8d814                         ; not the controlled side -> early return
0x8d607  MOV EAX,[0x157a4a]
0x8d60c  SAR EAX,0x18                        ; EAX = phase
0x8d60f  CMP EAX,0x9
0x8d612  SETZ AL
0x8d615  AND EAX,0xff
0x8d61a  XOR ECX,ECX
0x8d61c  ADD EAX,0x1d                        ; EAX = 0x1D (phase 8) or 0x1E (phase 9)
0x8d61f  XOR EBX,EBX
0x8d621  MOVSX EDX,AX                        ; the code
0x8d624  MOV EAX,EBP                         ; the record = the team base (record 0)
0x8d626  MOV dword[EBP+0x7b2],EBP            ; team->target = record 0
0x8d62c  CALL 0x7d9a4                        ; INSTALL code 0x1D/0x1E, invoke 0, stage 0
```

Semantics (**new first-hand**): when the phase is 8 or 9 and the team block's
side equals the phase-setter's side, the arm (a) installs code 3 over records
0..0xA of that team (`FUN_0008CEB8(team,0,10,3,-1)`), then (b) sets
`team->target = records[0]` and installs **code 0x1D for phase 8 / 0x1E for
phase 9** on the team base (record 0 = the keeper). So the correct field read is
`(phase == 9) + 0x1D`, not a literal immediate — the install census in §1.2 is
explained. This resolves FU-151 §5 leg 1 for the 0x1D/0x1E pair and matches the
engine's derived arm (`src/fifa96_engine/fifa96_match_run.c:2853-2861`, phase 8
→ `0x1Du`, phase 9 → `0x1Eu` onto `records[0]`).

### 1.4 The phase-8/9 producers: the situation-table-2 entries `[4]`/`[5]` (fresh)

`FUN_0008D098` runs only inside `FUN_000740A0` (the phase setter; fresh
`get_xrefs_to 0x740A0` = **27**). Fresh `disassemble_bytes 0x740A0`:
`[0x157A4E]=old phase`, `[0x157A4D]=AL`, `[0x157AAF]=DL` (side), then
`FUN_0008D098(0x1588A4 + side*0x835)` (`0x740C8`) and
`FUN_0008D098(0x1588A4 + (side^1)*0x835)` (`0x740DB`). So only a caller that
sets phase 8 or 9 raises the keeper install.

The two phase-8/9 callers are the **table-2 situation arms** (table `0x8A904`,
fresh `read_memory`, 13 entries; fresh `get_xrefs_to 0x8A904` = **1**, DATA
from `0x8AB7A`). Fresh `disassemble_bytes 0x8ADA0`:

```
0x8adc0  MOV EDX,[ESP-2]; MOV EAX,0x8; SAR EDX,0x10; CALL 0x740a0   ; [4] situation 4 -> phase 8
0x8add8  MOV EDX,[ESP-2]; MOV EAX,0x9; SAR EDX,0x10; CALL 0x740a0   ; [5] situation 5 -> phase 9
```

corroborating table `0x8A904` = `{0x8AB82,0x8ABAB,0x8ABDB,0x8ABF3,0x8ADC0,
0x8ADD8,0x8AC28,0x8ADF0,0x8AE08,0x8AEC6,0x8AEDE,0x8AEF6,0x8AF0E}` — entry
`[4]=0x8ADC0` (phase 8), `[5]=0x8ADD8` (phase 9). So:

* **situation 4 → phase 8 → row 0x1D** on the controlled team's keeper;
* **situation 5 → phase 9 → row 0x1E** on the controlled team's keeper.

The two situation producers, first-hand:

* **situation 4 (the goal kick).** The goal-mouth scanner `FUN_00088940`'s
  corner/goal-kick arm, fresh `disassemble_bytes 0x88B40`: `0x88B53
  EAX=[0x157A4A]>>24; CMP EAX,2; JNZ` (phase-2 only); `0x88B64 EAX=0x1E; CALL
  0x974DC` (the scanner sound); `0x88B6E EAX=[0x1577CA]` (the ball record); the
  `(snap-z sign) == (ball team==1)` predicate (`0x88B7B..0x88BB4`) →
  `EAX = cond + 3`; `0x88BBD CALL 0x8A938(EAX=3+cond, EDX=ball^1, EBX=0)`.
  `cond==1` ⇒ **situation 4**. (Engine: `fifa96_match_run_goal_scan`
  `0x3316` `set_piece(mr, 3+cond, ball^1, 0)`.)
* **situation 5 (the keeper CPU rush).** The row-0x1B stage-4 tail,
  `fifa96_keeper_reposition_b_finish(controlled, human)` → controlled && !human
  ⇒ `notify = 5` (`src/fifa96_loader/fifa96_keeper.c:252-263`). **Native emit
  (fresh `disassemble_bytes 0x776E0`):** `0x776F7 MOV EBX,0x1`; `0x776FC MOV
  EAX,0x5`; `0x77707 CALL 0x8A938` with `EDX = [[EDI]+0x826]` (the record side)
  — i.e. `FUN_0008A938(EAX=5, EDX=record_side, EBX=1)`, the **`BX=1` invoke-now →
  act-8 sweep** path (`0x8AA80..0x8AAA3`, §1.5). **Engine divergence:** the
  apply at `src/fifa96_engine/fifa96_match_handlers.c:3783`
  (`fifa96_match_run_situation(mr, notify_code)`) hardcodes `side = 0` and
  `bx = 0` (`src/fifa96_engine/fifa96_match_run.c:2948-2950` =
  `match_run_situation_side(mr, situation, 0u, 0)`), so it takes the **direct
  table-2 route**, bypassing the native act-8 detour. Tagged as **FU269-L5**
  (emit-route fidelity) as well as the live-dispatch leg. Native owner: row 0x1B
  `0x76D28` body (FU-180 §2; the `0x776C1` stage-4).

### 1.5 Two routes into the situation arm: queue vs direct (fresh)

`FUN_0008A938` head (fresh `disassemble_bytes 0x8A938`):
`0x8A944 TEST AX,AX; JZ 0x8AA7B` (id 0), `0x8A94E CMP EAX,0xB; JZ 0x8AA7B`
(id 0xB), `0x8A957 CMP byte[0x14C32A],0; JZ 0x8AA7B` (**session gate closed**),
`0x8A964 CMP dword[0x15B6C0],0; JNZ 0x8AA7B` (**situation pending**), else
`0x8A971..` → **table 1** (`0x8A8E0`, the *queue* route: stores the pending id +
`[0x15B6C0]=1`). The BX==0 direct path (`0x8AA7B`→`0x8AAA8`) hand-routes CX
(fresh `0x8AAA8 CMP CX,1; JC 0x8AB63`, `0x8AAB2 JBE 0x8AABF`, `0x8AAB4 CMP
CX,4; JBE 0x8AB08`, `0x8AABA JMP 0x8AB63`): CX==0 or CX≥5 → `0x8AB63`
(`JMP CS:[EAX*4+0x8A904]`); **CX==1 → `0x8AABF`** (the sit-1 twin
`0x8AABF..0x8AB06`, FU-149 L12); CX∈{2,3,4} → `0x8AB08`,
which either invokes act 4 (when `[0x157A49]>>24 != 1` and a formation residual
is nonzero) or **falls to `0x8AB63`** (`0x8AB27 JZ 0x8AB63` on
`[0x157A49]>>24==1`, `0x8AB39 JZ 0x8AB63` when both residuals are 0). So
situation 4 reaches table-2 `[4]` (phase 8) on the direct route when the
formation gate is dormant — exactly the engine's `fifa96_match_run_set_piece`
model (`src/fifa96_engine/fifa96_match_run.c:3111-3199`: `formation_order_ok` 0
⇒ the row runs; the queue arm when the gate is open and no pending).

### 1.6 The m 42 isolation issue (the tape)

The tape does **not** dispatch row 1E. The m 41 mechanics step stages code
`0x1E` onto team-0 record 9 — a **fixture install**, not a producer:
`tests/test_engine_m2.c:2566` `fifa96_match_entities_install(&mr->entities.team[0].records[1+i], 2, M2_STAGE_ROWS[i], 0)`
with `M2_STAGE_ROWS = {0x04,0x06,0x07,0x08,0x0F,0x18,0x21,0x23,0x1E,0x00}`
(`:2111`, index 8 → record 9). At **m 42** the scenario sets the native `+0x9A`
exclusion so the staged 1E machine never runs:

```
tests/test_engine_m2.c:2605   mr->entities.team[0].records[9].skip_9a = 1;
```

The issue (verbatim from the test's rationale, `:2600-2604`): the m 41 row-1E
record would otherwise reach the phase-2 window and its **1E machine would steal
the slot mid-walk every granted frame**, keeping the code-4 record's row-04
event arm off the slot-dir path (the goal chain would not run). The isolation is
an **honestly-staged forcing** declared in the tape. The result is asserted at
the goal link (`:2697-2703`): `records[9].code == 0x1E` persists, `has_ball == 0`,
and the keeper cells `keeper_cam_x/y/z` stay 0 — i.e. **row 1E is tape-dormant
on both the v10+ tapes**, and the claim chain is fixture-proven
(`test_engine_match_frame::test_keeper_claim_*`, `test_keeper_machines`). The
native mechanism is the per-record walk's `+0x9A` skip, fresh
`disassemble_bytes 0x8D0E0`: `0x8D0F7 CMP byte[ECX+0x9A],0 ; 0x8D0FE JNZ 0x8D123`
(the walk skips a `+0x9A`-set record); the engine models it at
`src/fifa96_engine/fifa96_match_run.c:2499` and `:6195`.

> Honest note: on the tape, row 1D never dispatches either (its phase-8
> producer — the goal kick — requires `goal_armed && phase 2`, and the
> post-goal phase-0 hold keeps the phase gate closed; `ENGINE.md:1755`). The
> row-1D/1E bodies are wired but tape-dormant; the install producer is the piece
> this slice pins.

---

## 2. Step 2 — The stage ladder (`0x7546E` / `0x75B58` / `0x76072`): what they need to run live

These three addresses are the **situation-0xB restart-emit call sites** inside
the two keeper stage machines (the "stage ladder"). All three fresh
`disassemble_bytes`, this session.

### 2.1 Row 1D — `0x7546E` (stage-3 tail)

Fresh `disassemble_bytes 0x75456`:

```
0x75456  EDX=[EBP-0x8]; EDX=[EDX]; DL=byte[EDX+0x826]; AND EDX,0xff   ; side
0x75461  MOV EAX,0xb
0x7546c  XOR EBX,EBX
0x7546e  CALL 0x8a938                       ; situation 0xB, side, BX=0
0x75473  XOR AH,AH; MOV byte[0x157ab2],AH    ; clear the hold latch
0x7547b  CALL 0x4c380
0x75480  EAX=[EBP-0x8]; DL=byte[EAX+0x92]; [EAX+0x89]=0; DL++; [EAX+0x92]=DL   ; timer 0, stage++
```

Emits `FUN_0008A938(EAX=0xB, EDX=side, EBX=0)` (no ECX set). Confirms
FU-151 §2.5. This is the shared restart tail: `EAX=0xB` (id), `EDX=side`,
`EBX=0` (invoke flag).

### 2.2 Row 1E stage-3 — `0x75B58` (the no-ball exit)

Fresh `disassemble_bytes 0x75B26`:

```
0x75b26  EAX=[EBP-0xc]
0x75b29  CMP byte[EAX+0x9b],0; JNZ 0x760df   ; still holding -> exit, no restart
0x75b36  EDX=[EBP-0xc]; CALL 0x7dab4         ; RESET the record (code 0/stage 0xFF)
0x75b3e  EDX=[EDX]; DL=byte[EDX+0x826]; AND EDX,0xff   ; side
0x75b46  MOV EAX,0xb
0x75b51  XOR EBX,EBX
0x75b53  MOV ECX,0x1
0x75b58  CALL 0x8a938                        ; situation 0xB, side, BX=0, ECX=1
0x75b5d  MOV EDX,0x5; EAX=[EBP-0xc]; XOR EBX,EBX; CALL 0x7d9a4   ; INSTALL code 5 on the keeper
```

Emits `FUN_0008A938(0xB, side, EBX=0, ECX=1)`, then installs code 5 (the keeper
"released/holding" row) on the same record. Confirms FU-151 §2.4 (stage 3 tail).

### 2.3 Row 1E stage-7 — `0x76072` (the outlet/throw)

Fresh `disassemble_bytes 0x76050`:

```
0x76050  EAX=[EBP-0xc]; CALL 0x786a0          ; slot hand-off to the nearest teammate
0x76058  EDX=[EBP-0xc]; EDX=[EDX]; DL=byte[EDX+0x826]; AND EDX,0xff   ; side
0x76063  MOV EAX,0xb
0x7606e  XOR EBX,EBX
0x76070  XOR CL,CL
0x76072  CALL 0x8a938                        ; situation 0xB, side, BX=0, ECX=0
0x76077  MOV byte[0x157ab2],CL               ; clear the hold latch
0x7607d  CALL 0x4c380
0x76082  EAX=[EBP-0xc]; CH=byte[EAX+0x92]; [EAX+0x89]=0; ...
```

Emits `FUN_0008A938(0xB, side, EBX=0, ECX=0)` (invoke flag 0, FU-151 §3.1 pins
the `XOR CL,CL`). Confirms FU-151 §2.4 (stage 7).

### 2.4 What a live emit needs

The three call sites share one route. `FUN_0008A938(0xB, side, EBX=0)`:
`0x8A94E CMP EAX,0xB; JZ 0x8AA7B` — since id==0xB, the **session-gate/pending
gates at `0x8A957`/`0x8A964` are bypassed** (fresh §1.5); `0x8AA7B TEST BX,BX;
JZ 0x8AAA8` (BX==0); `0x8AAA8 CMP CX,1; …; 0x8AABA JMP 0x8AB63`
(CX=0xB > 4); `0x8AB63 CMP CX,0xC; JA 0x8AF1F; JMP CS:[EAX*4+0x8A904]` →
table-2 `[0xB] = 0x8AEF6`: fresh `disassemble_bytes 0x8AEF0`:

```
0x8aef6  MOV EDX,[ESP-2]; MOV EAX,0x2; SAR EDX,0x10; CALL 0x740a0   ; FUN_000740A0(2, side) = phase 2
```

So the restart is `FUN_000740A0(2, side)` = **phase 2**. Requirements to run
live, per site:

1. **The row must be dispatched** on a granted frame — i.e. code 0x1D on the
   controlled keeper with the phase-8 producer (goal kick), or code 0x1E with
   the phase-9 producer (keeper rush). That is **Step 1's chain** (§1.3–1.4).
2. **The stage body must reach the tail**: row 1E reaches `0x75B58` when
   `has_ball == 0` at stage 3 (`0x75B26 JNZ` skip while holding), and `0x76072`
   at stage 7; row 1D reaches `0x7546E` at the stage-3 tail.
3. **The situation-0xB route is already ported**: the engine's keeper machines
   emit `situation_0b` (`src/fifa96_engine/fifa96_match_handlers.c:532` for 1E,
   `:631` for 1D) → `fifa96_match_run_situation(mr, 0x0B)` →
   `fifa96_action_phase_situations[0x0B] = {phase 2}` →
   `fifa96_match_run_phase_arm(2)` (no install for phase 2) → phase 2. So the
   **restart route itself needs no new port**; the live gate is the row install
   (§1.3, `FUN_0008D098` phase-8/9).

---

## 3. Step 3 — `OL-86`: the post-period 0x0C hold/reset timing

### 3.1 Fresh bytes — the `FUN_0008BAF0` phase-0x0C arm (`0x8BC01..0x8BCE6`)

Fresh `disassemble_bytes 0x8BBF0` (256 B):

```
0x8bc01  CMP EAX,0xc; JNZ 0x8bce7              ; the phase-0x0C arm (else 0x13/0x14)
0x8bc0a  CMP dword[0x158808],0; JNZ 0x8bc20    ; act installed -> skip the accrual
0x8bc13  AX=[0x157a64]; ADD word[0x158818],AX  ; the hold timer += frame delta
0x8bc20  EBX=0x960; EAX=0x3; EDX=0x158830; ECX=0
0x8bc31  [0x158830]=0x960; [0x158834]=0; [0x158838]=0
0x8bc43  CALL 0x36200                          ; camera lead (EAX=3, EDX=&0x158830)
0x8bc48  EBX=0x15883c; EAX=0x15880c; CALL 0x8dcd4
0x8bc57  EAX=[0x158816]; SAR EAX,0x10; CMP EAX,0x78; JLE 0x8bde7   ; word[0x158818] > 0x78
0x8bc68  CALL 0x45001; TEST AL,AL; JNZ 0x8bc80
0x8bc71  EAX=[0x158816]; SAR EAX,0x10; CMP EAX,0xb4; JLE 0x8bca3   ; word[0x158818] > 0xB4
0x8bc80  CMP dword[0x10f378],0; JZ 0x8bca3
0x8bc89  CALL 0x37f54                          ; the no-ET "match over" screen
0x8bc8e  [0x10f378]=0; [0x158818]=0; JMP 0x8bde7
0x8bca3  CALL 0x45001; TEST AL,AL; JNZ 0x8bcbf
0x8bcac  EAX=[0x158816]; SAR EAX,0x10; CMP EAX,0x168; JLE 0x8bde7  ; word[0x158818] > 0x168
0x8bcbf  CMP byte[0x157ac0],0; JZ 0x8bcd9      ; the extra-time flag
0x8bcc8  EAX=0x13; XOR EDX,EDX; CALL 0x740a0   ; ET -> phase 0x13
0x8bcd9  MOV word[0x158822],0x1                ; MATCH OVER
```

So the phase-0x0C hold is `word[0x158818]` (the dword `[0x158816]`'s high half),
accrued by the frame delta only when no act is installed (`[0x158808]==0`), with
the gates `> 0x78` (mark-over staging), `> 0xB4`/`> 0x168` (the input-gated
terminal via `FUN_00045001`), the `[0x10F378]` no-ET-screen gate, and the
extra-time flag `[0x157AC0]` selecting phase 0x13. `[0x158822]` (the match-over
word) is set at `0x8BCD9`. Confirms FU-143 §3.4.

### 3.2 Fresh bytes — the reset consumer `FUN_0004B02C(0)` → `FUN_00088860`

Fresh `disassemble_bytes 0x4B02C`: `JBE 0x4B064` on EAX==0 → `CALL 0x7412C`,
`CALL 0x8CBB0` (x2, gated on `[0x14C2F2]`), `CALL 0x88860` (`0x4B08E`). Fresh
`disassemble_bytes 0x88860`: the phase-0x0C gate
(`0x88863 EAX=[0x157A4A]>>24; CMP EAX,0xC; JNZ 0x888F7`), the reset
`0x7417C`/`0x6FFC0`, the camera reset `FUN_000700F4([0x10F328/2C/30])`,
`CALL 0x73E28` (phase := 0), then if `period == 4` (`0x888B0`) the
RNG&1 → `FUN_0008A938(EAX=8, side=RNG&1)` (`0x888C8`) + `FUN_0008F188(0xC1)`,
else `FUN_0008A938(EAX=1, side=[0x157AAB]>>24)` (`0x888F2`) — the kickoff
situation. Confirms FU-143 §4.2 / §10.

### 3.3 The engine state of `OL-86` (first-hand)

`OL-86`'s original gap (FU-143 §8 leg 3, line 621-628): "the engine lifecycle
instead marks the run over on the completion frame and exits on the next step
(`fifa96_match_run_step`'s documented one-step compression), so the 0x0C phase
is not held for its native duration". That premise is **superseded** (not
rewritten) by the phase-22 landings, first-hand in the built tree:

* `src/fifa96_engine/fifa96_match_run.c:4264-4266` — the **phase-0x0C terminal
  dwell**: while `state.phase ∈ {0x0C,0x13,0x14}` and `act.id == 0`,
  `act.timer += frame_delta` (the derived `word[0x158818]`, FU-235 §1.1/§4).
* `:4275-4278` — the **end word staging**: `end_word_158822` is set once
  `global_157ac2 >= 2 && act.timer >= 0x78` (the native `0x8BC5F > 0x78`).
* `:4319-4320` — `mark_over` requires `act.timer > 0x168` (the native `0x8BCAC`
  input-gated terminal), so the OVER screen holds through the dwell instead of a
  one-frame completion.
* The `FUN_0008BAF0` machine itself is modelled (`fifa96_match_run`'s ball
  machine + the phase drive; FU-148 §16 / FU-231 §3).

Residual legs after this slice:

* the **`word[0x158818]` producer** — modelled as `act.timer`; the native also
  accrues it in the phase-0x13/0x14 arm (`0x8BCFE`/`0x8BD04`, FU-235) and the
  dword `[0x158816]`'s low half (the ball-z / timeline timer relationship,
  FU-143 §9 erratum) is not modelled as a separate cell → **FU269-L3**;
* the **`FUN_00045001` input gate** and the **`[0x10F378]` gate / `0x37F54`
  no-ET screen** (`0x8BC68/0x8BC80/0x8BC89`) have no engine cell → **FU269-L4**
  (owner FU231-L2 names `0x37F54`).

---

## 4. Step 4 — The port shapes + the live-trigger ruling

### 4.1 Port shapes (all landed; no new port required)

* **Row entries**: `{0x1D, fifa96_match_action_1D}` and
  `{0x1E, fifa96_match_action_1E}` (`src/fifa96_engine/fifa96_match_handlers.c:
  5518/5520`), binding `fifa96_keeper_closedown_step` / `fifa96_keeper_claim_step`
  (the 5-stage / 10-stage machines, FU-151 §8).
* **The install producer**: `fifa96_match_run_phase_arm` phase-8/9 arm
  (`src/fifa96_engine/fifa96_match_run.c:2853-2861`) — `records[0]` gets
  `0x1D`/`0x1E` on the controlled team; the mapping is
  `fifa96_action_phase_situations[4] = {phase 8}`, `[5] = {phase 9}`
  (`src/fifa96_loader/fifa96_action_handlers.c:1137-1138`).
* **The situation producer**: `fifa96_match_run_set_piece` (the head model,
  queue/direct routes) → `match_run_situation_side` (the situation→phase+arm
  runner) → `fifa96_match_run_phase_arm`.
* **The restart route**: `fifa96_match_run_situation(mr, 0x0B)` → phase 2
  (`fifa96_action_phase_situations[0x0B].phase = 2`), emitted by both keeper
  machines (`fifa96_match_handlers.c:532`/`:631`).
* **The post-period hold**: the derived dwell (§3.3) — `act.timer` as
  `word[0x158818]`.

### 4.2 Live-vs-leg ruling

| chain | native producer | engine state | ruling |
|---|---|---|---|
| row 1D install (phase 8) | `FUN_0008D098` `0x8D607..0x8D62C` ← situation 4 (`0x8ADC0`) ← the goal-mouth scanner goal-kick arm (`0x88B5B`, `0x88BBD`) | landed (`fifa96_match_run_phase_arm` + `fifa96_match_run_goal_scan`) | **live-capable (headless)** — `fifa96_match_run_goal_scan` runs every granted frame (`fifa96_match_run.c:4243`), gated on `goal_armed && phase 2`. **Tape-dormant**: the post-goal phase-0 hold closes the phase gate; the natural band walk to arm `goal_armed` is the ~4400-step blocker (`ENGINE.md:1752`), and the m 42 `goal_armed` is a staged fixture input. Not smoke-visible. |
| row 1E install (phase 9) | same arm ← situation 5 (`0x8ADD8`) ← row 0x1B stage 4 (`fifa96_keeper_reposition_b_finish`, controlled+CPU) | landed (`fifa96_match_run_phase_arm` + `fifa96_match_action_1B` stage 4) | **fixture-only (leg)** — the row-0x1B stage-4 emit is **route-divergent**: native `FUN_0008A938(5, record_side, EBX=1)` (`0x77707`) = the `BX=1` act-8 sweep, while the engine's `fifa96_match_run_situation(mr, 5)` (`fifa96_match_handlers.c:3783`) forces `side=0, bx=0` (`fifa96_match_run.c:2948-2950`) — the direct table-2 route (FU269-L5). Its host row 0x1B's own live dispatch also depends on the keeper CPU ladder / row-1F installer chain (FU-180-L4/L5, FU-186); on the tape, row 1E is explicitly isolated (`skip_9a`) and never dispatched (`test_engine_m2.c:2697-2703`). |
| the restart emit (`0x7546E`/`0x75B58`/`0x76072`) | situation-0xB (`0x8AEF6` → phase 2) | landed (`fifa96_match_run_situation(0x0B)`) | **live iff the row is dispatched** — the route needs no new port; it is dead only because neither row is reached live on the tape. |
| `OL-86` 0x0C hold | `FUN_0008BAF0` `0x8BC01..0x8BCE6` | landed (the derived dwell, FU-231/232/235) | **resolved (dwell modelled)**; two residual cells (FU269-L3/L4). |

No NULL row is wired by this slice; the census is unchanged (**59/80**).

### 4.3 Numbered open legs (native owners)

1. **FU269-L1 — the situation-4/5 → phase-8/9 → install chain's unported
   downstream.** The engine does **not** collapse the native two-table split:
   `fifa96_match_run_set_piece` (`src/fifa96_engine/fifa96_match_run.c:3111-3199`)
   models the table-1 **queue arm** explicitly (`0x8A8E0`; the `situation_id`
   switch + `situation_pending`, `:3117-3142`) **and** the table-2 direct route
   via `fifa96_action_phase_situations` (`:3174-3199`, incl. the `0x8AB08`
   formation arm at `:3174-3184` and the sit-8 re-init `:3192-3198`). The
   residual is the **queue's downstream consumer** (the FU-146/FU-149 screen
   machinery that drains `situation_id`/`situation_pending`) and the formation
   arm's **genuine trigger** (`FU-197-L4`/`FU-201-L1` — a runtime
   non-permutation with no located writer). Native owner: `FUN_0008A938`
   (`0x8A938`) / `FUN_0008D098` (`0x8D098`). Cross-owner: FU-146/FU-149 (the
   situation queue), FU-197/FU-201 (the formation arm).
2. **FU269-L2 — the situation-4 (goal kick) upstream gate.** The phase-2 +
   `goal_armed` + `goal_zone == 0` + `(snap-z sign)==(ball team==1)` predicate is
   ported (`0x88B53..0x88BB4`), but `goal_armed` has no live open-play producer
   (the FU-145 armer needs the pan history to reach the mouth; `ENGINE.md:1752`,
   FU-168-L5). Native owner: `FUN_00088940` (`0x88940`) / `FUN_000713C8`.
3. **FU269-L3 — the `word[0x158818]` hold-timer cell.** The engine aliases it to
   `act.timer`; the native accrues the same word in the phase-0x13/0x14 arm
   (`0x8BCFE`/`0x8BD04`) and reads the dword `[0x158816]`'s high half, whose low
   half (the ball-z / timeline relationship, FU-143 §9 erratum) is unmodelled.
   Native owner: `FUN_0008BAF0` (`0x8BAF0`) / the `0x8BC13` accrual.
4. **FU269-L4 — the `OL-86` input/terminal gates.** `FUN_00045001`
   (`0x45001`), the `[0x10F378]` gate (`0x8BC80`), and the `0x37F54` no-ET
   screen (`0x8BC89`) have no engine cell (owner FU231-L2 for `0x37F54`).
   Native owner: `FUN_0008BAF0` (`0x8BAF0`).
5. **FU269-L5 — the situation-5 keeper-rush producer: emit-route divergence +
   live dispatch.** The native row-0x1B stage-4 emit is
   `FUN_0008A938(EAX=5, EDX=record_side, EBX=1)` (fresh `0x776F7 MOV EBX,0x1`;
   `0x776FC MOV EAX,0x5`; `0x77707 CALL 0x8a938`) — the **`BX=1` invoke-now →
   act-8 sweep** path. The engine applies it via
   `fifa96_match_run_situation(mr, 5)` (`fifa96_match_handlers.c:3783`), which
   hardcodes `side=0, bx=0` (`fifa96_match_run.c:2948-2950`) — the **direct
   table-2 route**, bypassing the native act-8 detour. Separately, row 0x1B's own
   live dispatch needs the keeper CPU ladder / row-1F installer chain
   (FU-180-L4/L5, FU-186). Native owner: `FUN_00077EAC` (`0x77EAC`, the CPU
   ladder) / `FUN_00076130` (`0x76130`, row 1F).

### 4.4 Handoff

* The rows and their install producer are **byte-exact and ported**; no
  implementation is needed for the route itself. What a phase-26 **producer**
  task must add is the upstream trigger — the live `goal_armed` producer
  (FU269-L2, shared with Task 1/FU264's camera-Y work) or the keeper CPU ladder
  (FU269-L5) — **not** the keeper machine.
* **Census**: 59/80 unchanged (no NULL row wired).
* **M2**: byte-identical; the m 42 `skip_9a` isolation stands and must not be
  dropped (dropping it re-steals the slot mid-walk, `test_engine_m2.c:2600-2604`).

---

## 5. Fresh-tool inventory (this session, all `/FIFA96.EXE`)

`list_open_programs`; `get_xrefs_to` `0x8A938`(39), `0x754E4`(1), `0x74E9C`(1),
`0x74EB0`(1), `0x7550C`(1), `0x8D098`(2), `0x740A0`(27), `0x7D9A4`(77),
`0x8A904`(1); `search_instructions` `MOV EDX,0x1D`(2), `MOV EDX,0x1E`(10),
`MOV 0x9a],`(24); `read_memory` `0x754E4`(64 B), `0x74E9C`(32 B), `0x1106E0`
(128 B), `0x8A904`(52 B), `0x8A8E0`(36 B); `disassemble_bytes` `0x7550C`,
`0x75B26`, `0x75456`, `0x76050`, `0x74EB0`, `0x74F58`, `0x8A938`, `0x8AA7B`,
`0x8A9F0`, `0x8ADA0`, `0x8AEF0`, `0x740A0`, `0x8D0E0`, `0x8D5E0`, `0x8BBF0`,
`0x4B02C`, `0x88860`, `0x88B40`, `0x75D17`, `0x776E0`; `decompile_function` `0x8D098`;
`get_function_by_address` `0x8D098`. Engine/golden reads: `fifa96_match_run.c`,
`fifa96_match_handlers.c`, `fifa96_action_handlers.c`, `fifa96_keeper.c`,
`tests/test_engine_m2.c`, `tests/test_engine_match_frame.c`. No write outside
this draft; no tool/ISO/Ghidra-project change.

---

## 6. Fix report (fix round 1/5, 2026-10-14)

Read-only re-verification; same program `/FIFA96.EXE`.

* **F1 (major) — FU269-L1 restated (§4.3 L1).** The prior wording ("the engine
  collapses the native two-table/queue split … into one mapping") is **wrong and
  removed**. First-hand re-read: `fifa96_match_run_set_piece`
  (`src/fifa96_engine/fifa96_match_run.c:3111-3199`) models **both** the table-1
  queue arm (`situation_id` switch + `situation_pending`, `:3117-3142`) **and**
  the table-2 direct route via `fifa96_action_phase_situations`
  (`:3174-3199`, incl. the formation arm `:3174-3184`). L1 now names the
  **unported downstream consumer** (FU-146/FU-149 screen machinery) and the
  formation arm's **genuine trigger** (FU-197-L4/FU-201-L1).
* **F2 (major) — situation-5 emit fidelity disclosed (§1.4, §4.2 row 1E, §4.3
  L5).** Fresh `disassemble_bytes 0x776E0`: the native row-0x1B stage-4 emits
  `FUN_0008A938(EAX=5, EDX=[[EDI]+0x826], EBX=1)` (`0x776F7 MOV EBX,0x1`;
  `0x776FC MOV EAX,0x5`; `0x77707 CALL 0x8a938`) — the **`BX=1` invoke-now →
  act-8 sweep**. The engine applies it through `fifa96_match_run_situation(mr, 5)`
  (`fifa96_match_handlers.c:3783`), which forces `side=0, bx=0`
  (`fifa96_match_run.c:2948-2950`) — the direct table-2 route. The divergence is
  disclosed and tagged FU269-L5; the previous "the situation-5 emit is ported"
  claim is qualified.
* **F3 (minor) — §2.2 header.** "Row 1E stage-5" → **"Row 1E stage-3"** (fresh
  `0x75B26` is the 0-based stage-3 no-ball exit).
* **F4 (minor) — line drift.** §1.6 staging cite `test_engine_m2.c:2564` →
  **`:2566`** (fresh read). §4.2/§4.3 `ENGINE.md:1750` → **`:1752`** (the
  "score a goal" row carries the ~4400-step mouth co-migration).
* **F5 (minor) — §1.5 CX routing.** Added the omitted **`CX==1` → `0x8AABF`**
  branch (the sit-1 twin `0x8AABF..0x8AB06`, FU-149 L12) with the fresh
  `0x8AAA8`/`0x8AAB2`/`0x8AAB4`/`0x8AABA` compares.
* The report's summary/concerns block carries the same F1/F2 restatements.
  Census unchanged (59/80); no code/test/commit.
