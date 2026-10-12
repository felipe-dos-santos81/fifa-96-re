# FU-193 — act-tail cohort 1: phase `0x17` / `0x1A` / `0x1C` (acts 1 / 4 / 6)

Provenance: M2 phase-17 wave **R41**, 2026-10-12, read-only. Authoritative
program **`/FIFA96.EXE`** (Ghidra MCP; scripts disabled; case-sensitive program
argument). Every address/byte/xref claim below is a fresh `read_memory` /
`disassemble_bytes` / `get_xrefs_to` / `get_function_by_address` of **this
session** unless marked *cross-cited* (then the frozen FU doc is named). Engine
source read first-hand (`src/fifa96_engine/fifa96_match_act.{c,h}`,
`fifa96_match_run.c`, `fifa96_match_handlers.c`,
`src/fifa96_loader/fifa96_action_handlers.c`). No writes: no rename, comment,
label, function, script or project save; `/FIFA96.EXE` untouched; no git
commands. The alias traps are applied (`dword[x]>>16` = word at `x+2`, `>>24` =
byte at `x+3`).

Scope: derive the three act-tail handlers **phase `0x17`** (`0x88DC8`, act 1),
**phase `0x1A`** (`0x89620`, act 4) and **phase `0x1C`** (`0x89110`, act 6)
byte-level — body, per-record loop / target, and **producer** (the `CALL
0x888FC` with `EAX=<act>`; fresh per-act xref census) — then state the honest
producer-live verdict per row under the strict "engine reaches it" test, and the
T4 wiring recommendation per row (body) with the producer remainder as legs.

---

## 1. The act table `0x1107EC` and the per-act `CALL 0x888FC` census (fresh)

Fresh `read_memory 0x1107EC` (64 B):

```
00000000 c88d0800 2c920800 a49f0800 20960800 ec900800 10910800
68980800 98a70800 4c8f0800 88b60800 74b80800 00b90800 ff076006
38ce0700 ff074000
```

`0x1107EC[0]=0`, `[1]=0x88DC8`, `[2]=0x8922C`, `[3]=0x89FA4`, `[4]=0x89620`,
`[5]=0x890EC`, `[6]=0x89110`, `[7]=0x89868`, `[8]=0x8A798`, `[9]=0x88F4C`,
`[0xA]=0x8B688`, `[0xB]=0x8B874`, `[0xC]=0x8B900`; the base is exactly
`0x110794 + 0x16*4`, so **act `a` → phase `0x16+a`** (confirmed byte-for-byte
against the fresh `0x110794` read: `[0x17]=0x88DC8`, `[0x1A]=0x89620`,
`[0x1C]=0x89110`). Byte-identical to FU-175 §1 / FU-190 §1.

**Fresh `get_xrefs_to 0x888FC` = 11** call sites (the same set as FU-175 §1).
Decoding each site's `EAX`/`DL`/`BX` register triple from fresh
`disassemble_bytes`:

| # | site | enclosing body | act (EAX) | stage (DL) | invoke (BX) | arm |
|---|---|---|---|---|---|---|
| 1 | `0x8AB57` | `FUN_0008A938` | **4** | 0 | 1 | the sit-2..4 arm `0x8AB08` |
| 2 | `0x8AB8E` | `FUN_0008A938` | 0xA | 0 | 1 | sit 0 → `0x8AB82` |
| 3 | `0x8ABCF` | `FUN_0008A938` | **1** | 0 | 1 | sit 1 → `0x8ABAB` |
| 4 | `0x8AEBA` | `FUN_0008A938` | 7 | 0 | 1 | sit 8 → `0x8AEAE` |
| 5 | `0x8AED2` | `FUN_0008A938` | 2 | 0 | 1 | sit 9 → `0x8AEC6` |
| 6 | `0x8AEEA` | `FUN_0008A938` | 2 | 1 | 1 | sit 0xA → `0x8AEDE` |
| 7 | `0x8AF1A` | `FUN_0008A938` | 9 / 8 | 0 | 1 | sit 0xC `0x8AF0E` (EAX=9); the sit-0/0xB BX!=0 fallback `0x8AAA3` (EAX=8) |
| 8 | `0x8BAE7` | `FUN_0008B9CC` | 0xB | 0 | — | period-end clear |
| 9 | `0x8A6FE` | `FUN_0008A43C` | 3 | 0 | 1 | the contact/foul arm |
| 10 | `0x8A789` | `FUN_0008A43C` | **6** | 0 | 1 | the phase-2 formation arm |
| 11 | `0x88EE8` | the act-1 body `0x88DC8` stage 1 | **4** | 0 | 1 | the act-1 → act-4 hand-off |

**Per-act census (this slice's three rows):**
* **act 1 (phase `0x17`): exactly 1 static site** — `0x8ABCF` (`FUN_0008A938`
  sit-1 arm, `EAX=1`, `BX=1` invoke-now).
* **act 4 (phase `0x1A`): exactly 2 static sites** — `0x8AB57` (`FUN_0008A938`
  sit-2..4 arm `0x8AB08`) and `0x88EE8` (the act-1 body's stage-1 hand-off).
* **act 6 (phase `0x1C`): exactly 1 static site** — `0x8A789` (`FUN_0008A43C`,
  the phase-2 formation arm, `BX=1` invoke-now).

(For completeness the census also re-confirms **act 5 / phase `0x1B` has 0
sites** — the FU175-L2 / FU-190 §5 blocked claim, re-derived here but owned by
R43.)

---

## 2. Phase `0x17` / act 1 — `0x88DC8` (the kickoff / phase-1 entry)

### 2.1 Body (fresh `disassemble_bytes 0x88DC8`, 320 B; ends `0x88F39` RET)

Head (`0x88DC8..0x88E0C`): prologue; `0x88DCD MOV EAX,3; CALL 0x4C374`; then the
shared timeline add `word[0x158818] += word[0x157A64]` (`0x88DD7..0x88DF5`).
The body then branches on the **act stage byte `[0x158829]`**:
`0x88DFC CMP DX,CX` (word `[0x157AC5]` vs `[0x157AC7]`) → `0x88E39`; else the
`[0x157AC2] >= 2` early path (`0x88E01..0x88E34`) clears `[0x158818]`,
`[0x158808]`, `[0x158828]`, `[0x158829]` and jumps to the epilogue.

**Stage 0 (`0x88E4B..0x88EB7`)** — the **kickoff entry**:
```
0x88E4B PUSH 0 ; EBX=[0x10F330] ; CL=0
0x88E56 MOV byte[0x15882A], CL        ; clear the kickoff gate
0x88E5C ECX=[0x10F32C]; ESI=[0x10F328]; CALL 0x700F4 ; CALL 0x73E28
0x88E74 MOV EDX,[0x157AAC]; 0x88E7F SAR EDX,0x18   ; EDX = side byte [0x157AAF]
0x88E7A MOV EAX,1
0x88E82 CALL 0x740A0                  ; write PHASE 1 (EAX=1) on that side
0x88E87 CALL 0x73E08                  ; the placement commit
0x88E94 MOV EAX,0x15774C ; EDX=0 ; CALL 0x4C324
0x88EA0 CH=[0x158829]; INC CH; word[0x158818]=0; byte[0x158829]=CH ; stage++
```
So stage 0 writes **phase 1** and runs the placement commit — this is the
native `FUN_000740A0(1)` write the engine models as the derived kickoff.

**Stage 1 (`0x88EBC..0x88F24`)**:
```
0x88EBC EDX=[0x157A49]>>24 ; CMP EDX,1 ; JZ 0x88EF3   ; phase/sit byte == 1 -> the gate arm
0x88ECA/0x88ED3 : if [0x157B8E]!=0 || [0x157B8F]!=0:
0x88EDC MOV EBX,1 ; 0x88EE1 MOV EAX,4 ; 0x88EE6 XOR EDX,EDX
0x88EE8 CALL 0x888FC                  ; -> ACT 4 (invoke-now)
   ; else:
0x88EF3 EDX=[0x158816]>>16 (alias: word[0x15881A]) ; CMP EDX,0x78 ; JL epilogue
0x88F01 DH=1 ; ESI=0 ; BL=0
0x88F07 MOV byte[0x15882A], DH        ; [0x15882A] = 1
0x88F0D word[0x158818]=0
0x88F14 byte[0x158828]=BL            ; act id = 0
0x88F1E dword[0x158808]=EDX(0)       ; clear the act slot
0x88F24 byte[0x158829]=BH(0)
```
So act 1 stage 1 either (a) **hands off to act 4** (`0x88EE8`, gated on the
formation bytes `[0x157B8E]/[0x157B8F]`), or (b) at `word[0x15881A] >= 0x78`
sets `[0x15882A]=1` and **ends the act** (clears the slot).

### 2.2 Per-record loop / target

**None.** Act 1 is a global timeline; it writes **phase 1** (a whole-match
write), and the per-record staging happens downstream in the per-team
`FUN_0008D098` walk via the `FUN_000740A0(1)` call (FU-175 §5). No record
pointer is read; no `[rec+...]` is touched.

### 2.3 Producer

**Sole static site `0x8ABCF`** in `FUN_0008A938`, the **sit-1 table arm**:
```
0x8ABAB MOV EBX,0x1E0 ; ... byte[0x157AB1]=0
0x8ABBA MOV dword[0x158830],EBX       ; [0x158830] = 0x1E0
0x8ABC0 MOV EBX,1 ; XOR ECX,ECX ; MOV EAX,EBX(=1) ; dword[0x158838]=0
0x8ABCF CALL 0x888FC                  ; ACT 1, stage 0, invoke-now
```
Entry: `FUN_0008A938(1, side, BX)`. Fresh trace of the dispatcher head
(`disassemble_bytes 0x8A938`, 368 B): the sit-1 direct arm is `0x8AABF`
(`CMP CX,1; JBE 0x8AABF` at `0x8AAA8..0x8AAB2`), which runs
`CALL 0x8C974` on `0x1588A4`/`0x1590D9`, tests the formation bytes
`[0x157B8E]/[0x157B8F]`, writes `[0x15881E]=CX`/`[0x158820]=AX`, then `JMP
0x8AB63` (the tail) → `0x8AB7A JMP CS:[EAX*4+0x8A904]` → table-2 `[1] = 0x8ABAB`.

**Engine route:** `fifa96_action_phase_situations[1] = {phase NONE, act 1, stage 0}`
(`src/fifa96_loader/fifa96_action_handlers.c:1134`) → `match_run_situation_side`
(`fifa96_match_run.c:2614`) → `fifa96_match_act_dispatch(mr, 1, 0, 0)`
(`match_run.c:2656`; act ≠ 2 so `invoke=0`, the tick dispatches). The route is
**modelled**, but the only engine driver of situation 1 is the phase-0x20
terminal `fifa96_match_action_phase_20` (`match_run.c:5444`,
`fifa96_match_run_situation(mr, 1u)`), and phase 0x20's sole producer is
situation 0, which no engine source/test dispatches (FU-188 §6, FU188-L1;
fresh phase-table row `0x20` is NULL, `fifa96_match_handlers.c:4675`).

**Verdict (strict "engine reaches it"): NOT producer-live.** Moreover the
engine's *live* act-1 behaviour is a **separate ad-hoc derived model** — the
begin-time `fifa96_match_run_kickoff_entry` (`match_run.c:2390`: `write_phase(1)`
+ `fifa96_match_phase_machine_kickoff` + drain) mirrors **stage 0**, and the
frame-body arm (`match_run.c:3657..3659`: `phase == KICKOFF && tick_total >=
0x78 → global_5882a = 1`) mirrors **stage 1(b)** — neither goes through the act
seam. Wiring phase `0x17` as the row would therefore **double-run** the kickoff
against the ad-hoc model (the `FU175-L7` class), unless the ad-hoc model is
retired (a behaviour/tape change). **Land body + leg, not a count.**

---

## 3. Phase `0x1A` / act 4 — `0x89620` (the formation-order / line-up walk)

### 3.1 Body (fresh `disassemble_bytes 0x89620`, 400 B; ends `0x897F1` RET)

Head (`0x89620..0x89650`): prologue; `word[0x158818] += word[0x157A64]`; branch
on the stage byte `[0x158829]`. Stage 0 = `0x89655..0x89774`; stage 1 =
`0x89775..0x897D6`; the end block `0x897AE..0x897F1`.

**Stage 0 (`0x89655`) — the per-record walk:**
```
0x89655 CMP byte[0x157B8E],0 ; SETZ AL ; dword[ESP]=AL (the scan key)
0x8966A MOV DL,[EAX+0x157B8E] ; DEC DL ; MOV [EAX+0x157B8E],DL   ; decrement the counter
0x89672 XOR ESI,ESI ; JMP 0x89757                                ; loop on ESI = 0..0xB
loop body (0x8967F):
0x8967F MOV AX,word[ESP]
0x89685 IMUL EDI,EAX,0x7B             ; the 0x157BD2 row offset
0x89688 IMUL EAX,EAX,0x2C             ; the 0x14C360 row offset
0x8968B LEA EBX,[EDX*4] ; 0x89692 LEA ECX,[EAX+EBX] ; 0x89695 ADD EDI,EBX
0x89697 MOV EAX,[ECX+0x157BD2]
0x8969D CMP EAX,[EDI+0x14C360] ; JZ 0x89756 (no match -> INC ESI; loop)
0x896A9 IMUL EDX,EDX,0xB2             ; the record stride 0xB2
0x896B1 MOV AX,word[ESP] ; CALL 0x741B4
0x896BA IMUL EAX,EAX,0x835            ; the TEAM block stride 0x835
0x896C0 ADD EAX,0x1588A4 ; LEA EBP,[EAX+EDX]   ; EBP = &record
0x896C8 CMP byte[EBP+0x9A],0 ; JZ 0x896E2      ; active -> install; else ->
0x896D1 MOV EAX,[ECX+0x157BD2] ; MOV [EDI+0x14C360],EAX ; JMP 0x897AE (end)
install (0x896E2):
0x896E7 MOV EAX,[EBP+4] ; MOV [0x1587F4],EAX
0x896F1 MOV EAX,0x76 ; MOV [0x1587EC],EBP ; CALL 0x8F188   ; emit event 0x76
0x89707 MOV [0x1587F8],EAX
0x8970C MOV EAX,0xB ; CALL 0x4C374
0x89716 MOV EDX,[EBP] ; MOV DL,[EDX+0x826] ; AND EDX,0xFF   ; the team side
0x8971F MOV EAX,0xB ; MOV ECX,1 ; CALL 0x740A0              ; write PHASE 0xB
0x89736 MOV EDX,0x17 ; 0x8973B MOV EAX,[0x1587EC] ; 0x89740 CALL 0x7D9A4
0x89747 word[0x158818]=0 ; INC byte[0x158829]   ; timer=0; stage++
```
So **stage 0 walks the 11 records of a team** (stride `0xB2`; team blocks
`0x1588A4`/`0x1590D9` at stride `0x835`; inactive skip `[rec+0x9A]`), compares
the `0x157BD2` row against the `0x14C360` matrix, and on the first active
match **writes phase `0xB`** (`0x740A0`) and **installs action row `0x17`**
(`CALL 0x7D9A4`, `EDX=0x17`) on the record `[0x1587EC]` (= the matched `EBP`).

**Stage 1 (`0x89775`)**:
```
0x89775 EAX=[0x1587EC]; EAX+=0x59; CALL 0x4C31C
0x89782 EAX=[0x1587EC]; EAX=[EAX+0x8E]>>24 ; AL=[EAX+0x110680] & 1
0x8979D JZ 0x897E8 (epilogue)         ; gate bit clear -> done
0x8979F EAX=[0x158816]>>16 (word[0x15881A]) ; CMP EAX,0xF0 ; JLE epilogue
0x897AE (end block): clear [0x158818], [0x158828], [0x158829], [0x158808]
        ; re-dispatch the stored situation: EDX=word[0x158820], EAX=word[0x15881E]
0x897E3 CALL 0x8A938                  ; the situation re-dispatch with BX=1
```

### 3.2 Per-record loop / target

**Per-record loop:** 11 records (`ESI=0..0xB`) of a team, `0xB2` record stride,
team blocks `0x1588A4`/`0x1590D9` (`0x835` stride), `[rec+0x9A]` active skip.
**Target:** the first active record whose `0x157BD2`/`0x14C360` matrix entry
matches → `[0x1587EC]`; the row install targets that record; the phase-`0xB`
write targets its team side (`[[rec]+0x826]`).

### 3.3 Producer

**Two static sites:**
1. `0x8AB57` (`FUN_0008A938`, the **sit-2..4 formation-order arm** `0x8AB08`):
   entered via the dispatcher head `0x8AAA8 CMP CX,1 → 0x8AAB4 CMP CX,4;
   0x8AAB8 JBE 0x8AB08` for `CX ∈ {2,3,4}` (fresh `disassemble_bytes
   0x8AAA0`). The arm runs `CALL 0x8C974` on `0x1588A4`/`0x1590D9`, tests
   `[0x157B8E]/[0x157B8F]`, and `0x8AB4B MOV EAX,4; 0x8AB57 CALL 0x888FC`
   (`BX=1`, stage 0). Matches **R45 / FU-197**'s target arm.
2. `0x88EE8` (the act-1 body stage 1, §2.1), gated on the formation bytes.

**Engine route: NONE for act 4.** The engine's situation table has **no act-4
row** — `fifa96_action_phase_situations[2] = {phase 3, act NONE}`,
`[3] = {phase 4, act NONE}`, `[4] = {phase 8, act NONE}`
(`fifa96_action_handlers.c:1135..1137`); the sit-2..4 act-4 arm is explicitly
unported (FU-149 L12; `fifa96_match_run.c:2823..2826`). Fresh `get_xrefs_to
0x8AB08` = **1** (`0x8AAB8`, the head jump) — the arm is real but has no engine
counterpart. `grep fifa96_match_act_dispatch` = only `match_run.c:2656` (the
dynamic `out.act`) and `:5488` (act 3).

**Verdict: NOT producer-live.** Body + leg; the producer is **T2/R45/FU-197**
(the act-4 sit-2..4 arm) plus act 1 (the stage-1 hand-off).

---

## 4. Phase `0x1C` / act 6 — `0x89110` (the pre-free-kick timeline)

### 4.1 Body (fresh `disassemble_bytes 0x89110`, 288 B; ends `0x89213` RET)

Head (`0x89110..0x89141`): prologue; `word[0x158818] += word[0x157A64]`; branch
on the **stage byte**, a **3-stage** switch (`CMP AL,1` `JC`/`JBE`/`JZ`
`0x89142/0x89190/0x891C0`).

**Stage 0 (`0x89142..0x8918F`)**:
```
0x8914A MOV EAX,0x15 ; CALL 0x4C374
0x89156 MOV EAX,0xA ; XOR EDX,EDX ; CALL 0x740A0   ; write PHASE 0xA, side 0
0x89160 MOV EAX,0x1E ; CALL 0x974DC
0x8916A MOV EAX,0x190 ; CALL 0x974F0
0x89174 BL=[0x158829]; INC BL; word[0x158818]=0; byte[0x158829]=BL ; stage++
```
**Stage 1 (`0x89190`)**: `CALL 0x4BEC8; TEST AX,AX; JNZ epilogue` (wait); on
zero `MOV EAX,0x10; CALL 0x4C374`, timer=0, stage++.
**Stage 2 (`0x891C0`)**:
```
0x891C0 CALL 0x4BEC8 ; TEST AX,AX ; JNZ epilogue (wait)
0x891CC MOV dword[0x158808],EDX(0)         ; CLEAR the act slot
0x891D2 EDX=[0x15888F] ; EDX=[EDX] ; EDX=[EDX+0x7A6]
0x891E2 DL=[EDX+0x826]                     ; the fouled-side byte
0x891E8 byte[0x158828]=AH(0) ; 0x891EE byte[0x158829]=AH(0)
0x891FA MOV EBX,1 ; 0x891FF MOV EAX,9 ; AND EDX,0xFF
0x8920A CALL 0x8A938                       ; re-dispatch SITUATION 9 -> act 2
```
So act 6 writes **phase `0xA`** in stage 0, waits on `0x4BEC8` across stages
1/2, then **ends the act and re-dispatches situation 9** (the free-kick /
penalty hand-off → act 2). It is a **pre-restart timeline**, not a record walk.

### 4.2 Per-record loop / target

**None.** Act 6 is a global 3-stage timeline. Its only record-keyed access is
`[0x15888F]` (the fouled record) dereferenced to its team side; no per-record
walk. Target: none directly; the terminal re-dispatch (situation 9) hands to
act 2 / phase 0x18.

### 4.3 Producer

**Sole static site `0x8A789`** in `FUN_0008A43C`, the **phase-2 formation arm**
(fresh `disassemble_bytes 0x8A70A`):
```
0x8A735 CMP EBX,3 ; 0x8A738 JNZ epilogue
0x8A73A CMP dword[0x14C2F2],0 ; 0x8A741 JZ epilogue
0x8A743 MOV EBX,4 ; MOV EAX,0x15 ; MOV EDX,EBP ; MOV EDI,0x158897 ; CALL 0x8F188
0x8A759 AL=[ESP] ; MOV EBX,1
0x8A761 MOV dword[0x15888F],EBP ; MOV dword[0x158893],ESI
0x8A772 MOV byte[0x15888C],AL
0x8A76F LEA ESI,[EBP+0x59] ; MOV EAX,0x158897 ; MOVSD ×3   ; copy the incident triple
0x8A77F CALL 0x7D3E4
0x8A784 MOV EAX,6 ; 0x8A789 CALL 0x888FC              ; ACT 6, stage 0, invoke-now
```
`FUN_0008A43C` head (`0x8A43C`) requires `dword[0x157A4A]>>24 == 2` (the live
**phase 2**); fresh `get_xrefs_to 0x8A43C` = **2** callers:
`0x79F2B` (`FUN_00079D5C`, body `0x79D5C..0x79F38`) and `0x81EBF` — the
**row-0x0C contact body** (`0x81EA6 CALL 0x92AC8`, the severity draw the engine
ports at `fifa96_match_run_contact` `match_run.c:5447..5489`). The `EBX==3` arg
selects the act-6 arm over the act-3 arm (`0x8A6FE`, `EAX=3`); the act-6 arm
additionally requires `[0x14C2F2] != 0`.

**Engine route: NONE for act 6.** No situation row yields act 6
(`fifa96_action_phase_situations` acts = `{0xA, 1, 7, 2, 9}`), and
`grep fifa96_match_act_dispatch` has no act-6 call. The engine's contact port
(`fifa96_match_run_contact`) handles only `out.sequence == FIFA96_REF_SEQ_ACT3`
→ act 3 (`match_run.c:5481..5490`) and `out.restart` → sit 9; there is no
act-6 `fifa96_ref_sequence` request.

**Verdict: NOT producer-live.** Body + leg; the producer is the
**contact/referee machine** (T2) needing an act-6 sequence request.

---

## 5. Engine reachability verdicts (strict "engine reaches it")

| phase | act | body | native producer (fresh census) | engine route to the act | verdict |
|---|---|---|---|---|---|
| `0x17` | 1 | kickoff / phase-1 entry (`0x88DC8`) | **1** site: `0x8ABCF` (sit 1) | sit-1 row `out.act=1` exists; **no live driver** (only the unported phase-0x20 terminal); live kickoff is the **ad-hoc** `fifa96_match_run_kickoff_entry` + `global_5882a` arm | **NOT live** (and a **double-model**) |
| `0x1A` | 4 | formation-order walk (`0x89620`) | **2** sites: `0x8AB57`, `0x88EE8` | **no act-4 row, no dispatch** | **NOT live** |
| `0x1C` | 6 | pre-free-kick timeline (`0x89110`) | **1** site: `0x8A789` | **no act-6 row, no dispatch**; the contact port has no act-6 request | **NOT live** |

None of the three rows is producer-live today. The phase table keeps
`fn == NULL` for `0x17`/`0x1A`/`0x1C` (`fifa96_match_handlers.c:4663/4668/4670`),
so a T4 landing that only ports the bodies adds **no wired rows** (the
rows-27/29/2C / phase-0x20 precedent) unless a live producer is added (T2).

---

## 6. T4 wiring recommendation per row

1. **phase `0x17` (act 1):** port the body as the **kickoff/phase-1 entry**
   (already present as `fifa96_match_run_kickoff_entry` + the `match_run.c:3657`
   gate arm). **Do NOT naively wire the phase-table slot**: with the derived
   kickoff live, the seam row would re-run stage 0/1. Either (a) *retire* the
   ad-hoc kickoff and re-express it as `fifa96_match_act_dispatch(mr, 1, 0, 1)`
   (a behaviour/tape change requiring its own proof), or (b) land the body
   documented and keep the slot **NULL/uncounted** with leg `FU193-L1`. Honest
   default: **(b)** for the tape-neutral spine.
2. **phase `0x1A` (act 4):** port the body as the **formation-order walk** and
   wire the slot **only if** the T2/R45 act-4 producer lands (the sit-2..4
   `0x8AB08` arm and/or the act-1 stage-1 hand-off). Otherwise body + leg
   `FU193-L2`.
3. **phase `0x1C` (act 6):** port the body as the **pre-free-kick timeline** and
   wire the slot **only if** the contact/referee machine gains the act-6
   sequence request. Otherwise body + leg `FU193-L3`.

Census impact of T4-as-recommended: **+0 phase rows** (bodies land, producers
remain legs); **+3** phase rows if T2's producers land in the same phase.

---

## 7. Numbered legs

1. **FU193-L1 — phase `0x17` / act-1 producer.** The sit-1 route is modelled
   (`fifa96_action_phase_situations[1].act = 1`) but has no live driver: the only
   engine caller of situation 1 is the phase-0x20 terminal (`match_run.c:5444`),
   and phase 0x20's sole producer is situation 0, undispatched (FU188-L1, fresh
   phase-table `0x20` NULL). The native sit-1 entry is `FUN_0008A938(1, side,
   BX)`. Owner: T2/R44 (the situation producer) + a kickoff-ownership decision.
2. **FU193-L2 — phase `0x17` double-model.** The engine's live act-1 behaviour is
   the ad-hoc `fifa96_match_run_kickoff_entry` (stage 0) + the `match_run.c:3657`
   `global_5882a` arm (stage 1b), not the act seam. Wiring the row without
   retiring the ad-hoc model double-runs the kickoff (the `FU175-L7` class). Note
   stage 1's native gate is `word[0x15881A] >= 0x78` (alias trap) while the
   engine uses `state.tick_total >= 0x78` — the `FU175-L5` divergence class.
3. **FU193-L3 — phase `0x1A` / act-4 producers.** Two native sites (`0x8AB57`
   the sit-2..4 arm `0x8AB08`; `0x88EE8` the act-1 stage-1 hand-off); the engine
   has **no act-4 row** and the sit-2..4 arm is unported (FU-149 L12). Owner:
   T2/R45/FU-197.
4. **FU193-L4 — phase `0x1C` / act-6 producer.** Sole site `0x8A789` in
   `FUN_0008A43C` (phase-2 gated, `EBX==3`, `[0x14C2F2]!=0`), reached from the
   row-0x0C contact body (`0x81EBF`) / `FUN_00079D5C` (`0x79F2B`). The engine's
   contact port has only the act-3 / sit-9 requests; no act-6 request exists.
   Owner: the contact/referee machine (T2).
5. **FU193-L5 — act-4 loop exit semantics.** The stage-0 walk's exit
   (install-then-`RET` unless `ESI==0xB`; the matched-but-inactive branch ends
   the function) and the `0x157BD2`/`0x14C360`/`0x157B8E` formation-order
   matrices are only partly derived here; the arm body is R45/FU-197's scope.
6. **FU193-L6 — `FUN_0008A43C` act-3 vs act-6 selection.** The
   `[0x15888D]`/`EBX`/`[0x14C2F2]` gating that chooses the act-3 arm
   (`0x8A6FE`) over the act-6 arm (`0x8A789`) was not fully derived (out of this
   slice's act-tail scope); it is FU-150's contact machinery.

---

## 8. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `read_memory` — `0x1107EC` (64 B), `0x110794` (140 B), `0x8A904` (56 B).
* `get_xrefs_to` — `0x888FC` (11, twice), `0x8A43C` (2), `0x8A938` (39),
  `0x8AB08` (1).
* `disassemble_bytes` — `0x88DC8` (320 B, act 1), `0x89620` (400 B, act 4),
  `0x89110` (288 B, act 6), `0x88F07` (96 B, act-1 tail), `0x897AF` (200 B,
  act-4 tail), `0x8AB40` (80 B), `0x8AB80` (96 B), `0x8AB00` (96 B), `0x8ABE0`
  (128 B), `0x8AEA0` (96 B), `0x8BAE0` (40 B), `0x8A6F0` (32 B), `0x8A780`
  (32 B), `0x8AAA0` (112 B, the sit-1/2..4 arms), `0x8A938` (368 B, the
  dispatcher head), `0x8A43C` (128 B), `0x8A70A` (160 B), `0x8A4BB` (570 B),
  `0x81EA0` (64 B, the row-0x0C contact caller).
* `get_function_by_address` — `0x79F2B` (`FUN_00079d5c`, body
  `0x79D5C..0x79F38`); `0x89620`/`0x89110`/`0x88DC8` have no defined function.

Engine/loader source read first-hand:
`src/fifa96_engine/fifa96_match_act.c` (whole), `fifa96_match_run.c`
(2100..2239, 2388..2527, 2600..2665, 2760..2880, 3070..3169, 3640..3729,
5200..5269, 5330..5500), `fifa96_match_handlers.c` (4547..4699, both tables),
`fifa96_loader/fifa96_action_handlers.c` (1110..1161).

Docs cross-cited: FU-175 (§1/§3/§4/§5/§7), FU-181, FU-182 (§5/§7), FU-183,
FU-188 (§2/§6), FU-189, FU-190 (§1/§4/§5), FU-186, FU-149 (L12), FU-150,
FU-143 (§10/§11), FU-177, FU-184. No writes beyond this slice file and the
wave report.
