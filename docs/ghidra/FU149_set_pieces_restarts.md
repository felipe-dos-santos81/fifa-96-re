# FU-149 — set pieces & restarts

Provenance: recon draft w7-b1, phase-7 wave-7, frozen 2026-10-09; evidence review: PASS with corrections applied inline.

Phase-7 recon-ahead, Track B (`docs/superpowers/plans/2026-10-08-fifa96-m2-phase7-recon-ahead.md`).
Read-only slice: the set-piece/restart dispatch rows and their situation
producers for **throw-in, corner, goal kick, free kick** (+ penalty and keeper
restarts where the same rows carry them), the restart placement/state, and the
period/clock interactions. Extends FU-146 legs 1–4 into the *producer* side.

**Method / provenance.** Authoritative program **`/FIFA96.EXE`** (explicit
`program` argument in every call). Ghidra read-only: no renames, comments,
labels, function creation, scripts or project saves. All windows below are
first-hand this slice unless the cell says *cross-cited* (then the frozen FU
doc is named). Fresh xrefs this slice: `get_xrefs_to 0x8A938` = **39**,
`0x740A0` = **27**, `0x88940` = **1**, `0x8A43C` = **2**, `0x79D5C` = **1**;
`search_instructions` operand `0x00157a4d` = 5 sites, `0x00157aac` = 27 read
sites (no direct write), `157ad4` = 3 sites. `read_memory`: dispatcher queue
table `0x8A8E0` (36 B), normal table `0x8A904` (52 B), phase-arm table
`0x8D040` (88 B), phase class table `0x1106AD` (35 B), phase table `0x110794`
(140 B), action table `0x1106E0` (180 B). `dword[a]>>16 = word[a+2]` applied
at the snapshot/incident reads; `+0x100000` BSS rule not needed (all globals
quoted are the runtime/Ghidra addresses).

Traps respected: every cited call target was read from the CALL bytes (rel32
arithmetic already resolved by Ghidra); no flat `fifa96_le.bin` offsets.

---

## 1. Evidence — the dispatcher `FUN_0008A938` (39 call sites, fresh)

### 1.1 Head gates and table selection (first-hand, `0x8A938..0x8A996`)

```
0x8A93E MOV ECX,EAX                 ; situation
0x8A940 MOV word [ESP],DX           ; side
0x8A944 TEST AX,AX / JZ 0x8AA7B     ; situation 0 -> fallback/normal path
0x8A94E CMP EAX,0xB / JZ 0x8AA7B    ; situation 0xB -> fallback/normal path
0x8A957 CMP byte [0x14C32A],0 / JZ 0x8AA7B   ; session gate closed -> direct path
0x8A964 CMP dword [0x15B6C0],0 / JNZ 0x8AA7B ; pending -> direct path
0x8A971 MOV ESI,[ESP] / TEST SI,SI / SETZ AL ; [0x15B6B8] = (side==0)
0x8A97F SUB ECX,2
0x8A987 CMP CX,8 / JA 0x8AA60       ; situation > 10 -> id 0xA
0x8A996 JMP dword CS:[EAX*4+0x8A8E0] ; QUEUE table
```

Queue table `0x8A8E0` (fresh read, 9 dwords): sit 2/3/4 → `0x8A99E`
(`[0x15B6A8] = 9` when side==0 else `0`), sit 5/7 → `0x8A9CD` (`= 7`),
sit 6 → `0x8A9E8` (`5`/`6`), sit 8 and >10 → `0x8AA60` (`= 0xA`),
sit 9/10 → `0x8AA23` (`= (side==1) ? 1 : 2`); every arm latches
`[0x15B6C0] = 1` and RETs (bytes `c7 05 a8 b6 15 00 ..` / `c7 05 c0 b6 15 00
01 00 00 00`).

Direct path `0x8AA7B`: `TEST BX,BX`.
* **BX != 0** → `0x8AA80..0x8AAA3`: `EDX=0; EAX=0; EBX=1; CALL 0x740A0`
  (**phase 0**, side 0), then `[0x15882C]=side` (`0x8AA93`),
  `[0x15882B]=situation` (`0x8AA9D`), `EAX=8`, `JMP 0x8AF1A` →
  `FUN_000888FC(8,0,1)` = **act 8** (overlapping action table `0x1107EC[8]`,
  same address `0x11080C`, value `0x8A798`).
  Act 8's tail (`0x8A8A5..0x8A8DE`, first-hand) re-dispatches the stored
  situation/side with **BX=0** (`MOV EDX,[0x158829]; MOV EAX,[0x158828];
  SAR…; CALL 0x8A938`) and then writes `[0x15882B]=0xFF`.
* **BX == 0** → `0x8AAA8` situation-range gate (`CX<1` `JC`, `≤1` act-1 arm,
  `≤4` sit 2..4 arm at `0x8AB08`, else `0x8AB63`), tail `0x8AB63`
  `[0x157AAF]=side`, then **normal table** `0x8AB7A JMP CS:[EAX*4+0x8A904]`.

Normal table `0x8A904` (fresh read, 13 dwords) → bodies (first-hand):

| sit | body | outcome (bytes quoted) |
|---|---|---|
| 2 | `0x8ABDB` | `MOV EAX,3; SAR EDX,0x10; CALL 0x740A0` → **phase 3** (`b8 03 00 00 00`) |
| 3 | `0x8ABF3` | `EAX=side; CALL 0x741B4; BX=word[EAX*2+0x157AD4]; INC BX; store; EAX=4; CALL 0x740A0` → **corner counter++ then phase 4** (`66 8b 1c 45 d4 7a 15 00` / `66 89 1c 45 d4 7a 15 00`) |
| 4 | `0x8ADC0` | `EAX=8; CALL 0x740A0` → **phase 8** |
| 5 | `0x8ADD8` | `EAX=9; CALL 0x740A0` → **phase 9** |
| 7 | `0x8ADF0` | `EAX=0xD; CALL 0x740A0` → **phase 0xD** |
| 9 | `0x8AEC6` | `EBX=1; EAX=2; EDX=0; CALL 0x888FC` → **act 2 stage 0** (phase 0x18) |
| 0xA | `0x8AEDE` | `EAX=2; EDX=EBX(=1); CALL 0x888FC` → **act 2 stage 1** |
| 0xB | `0x8AEF6` | `EAX=2; SAR EDX,0x10; CALL 0x740A0` → **phase 2** (in play) |

Context rows (not set pieces): 0 → act 0xA + phase 0x11 (`0x8AB82`); 1 → act 1
(kickoff, `0x8ABAB`); 6 → score tables + phase 5 unless period 2/3
(`0x8AC28`); 8 → match re-init + act 7 (`0x8AE08`); 0xC → act 9 (`0x8AF0E`).

`FUN_000740A0` (first-hand `0x740A0..0x7410D`): `[0x157A4E]=old phase`,
`[0x157A4D]=new phase`, `[0x157AAF]=side`, `FUN_0008D098(0x1588A4+side*0x835)`
then the `side^1` block (`IMUL EAX,EAX,0x835` at `0x740B4/0x740D0`); on
new phase == 2: `FUN_0004C380`, `[0x15781D]=0` (`0x740F6`), `[0x157AB2]=1`,
`[0x157A73]=0x15774C`.

`FUN_000741B4` (first-hand `0x741B4..0x741C3`) = `(side ^ byte[0x157ABE]) & 1`
— the display/score slot, so the corner counter and the score words are indexed
by `side ^ side_swap`, not by side.

### 1.2 Producer census — every site's situation argument (39 sites, fresh)

The situation argument was read from the bytes at every site below; cells
marked *FH* are first-hand this slice, the rest are the frozen census in
FU-142 App. L.9 (re-verified count 39, same site list).

| sit | site (FH unless noted) | EAX | EDX | EBX | container / role |
|---|---|---|---|---|---|
| 2 | `0x88C00` | 2 | `[[0x1577CA]]+0x826` ^ 1 | **1** | `FUN_00088940` scanner throw-in (phase==2 gate `0x88BD4`) |
| 2 | `0x85D0A` | 2 | `[[EBP]]+0x826` | **1** | row 0x10 taker, gate = signed `JLE` on `[0x157821]` (`0x85CE4`) |
| 2 | `0x943D7` | 2 | 0 | 0 | handler leg 3 (`0x94270`) exit |
| 3 | `0x88BBD` | `3+cond` | `[[0x1577CA]]+0x826` ^ 1 | 0 | scanner corner/goal-kick arm (phase==2 gate `0x88B5B`) |
| 3 | `0x93CE4` | 3 | 0 | (carried) | handler leg 0 (`0x93BBC`) exit — `b8 03..`/`31 d2` at `0x93CDD/0x93CE2` |
| 4 | `0x88BBD` | `3+cond` | same | 0 | same site; `4` when the condition is true (see §1.3) |
| 4 | `0x94120` | 4 | 0 | 0 | handler leg 2 (`0x940A4`) exit |
| 5 | `0x76B1C` | 5 | `[[EBP]]+0x826` | **1** | row 1A (`0x7662C`) stage-4 CPU tail (`0x7DAB4` reset first) |
| 5 | `0x77707` | 5 | `[[0x157A83]]+0x826` | **1** | row 1B (`0x76D28`) stage-4 tail (`[rec+0x92]=4`, tracked-record test) |
| 7 | `0x76A90` | 7 | `[[EBP]]+0x826` | 0 | row 1A stage-2 common tail (`CALL 0x651F0` event first) |
| 7 | `0x77589` | 7 | `[[EBP]]+0x826` | 0 | row 1B (`[0x157A83]=rec` + `CALL 0x6E598` anim first) |
| 9 | `0x8A729` | 9 | side of `[[[EBP]]+0x7A6]` | **1** | `FUN_0008A43C` severity==0 (free-kick award, §1.4) |
| 9 | `0x8920A` | 9 | `[[[0x15888F]]+0x7A6]+0x826` | **1** | phase-0x1C (act 6) completion tail `0x891CA..0x89213` |
| 6 | `0x88B44` | 6 | `[[0x157A9F]]+0x826` | 0 | scanner goal arm (FU-145/FU-146); kept for context |
| 0xB | 8 rows | 0xB | rec team | 0 | restart resume: `0x7DF90`, `0x85D38*`, `0x863F9*`, `0x84495*`, `0x84E8F*`, `0x7546E`, `0x75B58`, `0x76072` (all FH except `0x7DF90`/keeper-set, FU-147 §3.6) |
| 0/1/8/0xA/0xC | 15 more | — | — | — | non-set-piece: setup/screen (`0x4B0A5/0x4B0D9/0x38B2E/0x38C95/0x742DE/0x74312`), `FUN_00088860` (`0x888C8/0x888F2`), act bodies `0x8B85D/0x8A3F0/0x897E3`, handler tails `0x93C41/0x93EC1/0x93F75/0x94332/0x945A5/0x947AB` (FU-142 L.9) |

`0x8A8CE` (act-8 re-dispatch) is the 39th site and is not a producer (§1.1).

### 1.3 The scanner `FUN_00088940` (fresh xrefs = 1 → `0x8B63E`)

Gate (`0x8B5F0..0x8B643`, first-hand): period-4 extra-time path
(`0x8B603 CMP AL,[0x157AC2]`/`JNZ`, `0x8B60D CMP [0x157AC0]`/`JZ`,
`0x8B616 CALL 0x8B9CC`/`INC [0x157AC2]`/`JMP`) **skips the scan**; otherwise
phase byte `CMP 2 / JZ`, `CMP 0x10 / JNZ`, then `CMP byte [0x15781D],0 / JZ`,
`0x8B63E CALL 0x88940`.

Arms (first-hand `0x8896B..0x88C0E`):
* `|snapshot z| <= 0xB20` → **sit 2** (`0x88BCC..0x88C00`): `CMP EDX,0xB20 /
  JLE`, phase==2 gate `0x88BD4`, `EAX=2`, `EDX = [[0x1577CA]]+0x826 ^ 1`,
  `EBX=1`.
* `|snapshot z| > 0xB20` and `[0x15781E] != 0` → **sit 6** (goal,
  `0x88B37/0x88B44`).
* `|snapshot z| > 0xB20` and `[0x15781E] == 0` → `0x88B53..0x88BBD`:
  `EAX = 3 + ((snapshot z < 0) == (ball team == 1))` — bytes:
  `0x88B86 AL=[EAX+0x826]` / `0x88B91 CMP EAX,1; SETZ` / `0x88B99 ESI=[0x157784]`
  / `0x88BA5 TEST ESI,ESI; SETL` / `0x88BAF XOR EAX,ECX; SETZ` /
  `0x88BB9 ADD EAX,3; CWDE` / `0x88BBD CALL 0x8A938`; `EDX = ball team ^ 1`,
  `EBX=0`, phase==2 gate `0x88B5B`.
  Under the end convention `team 0 defends −z`: cond false → **sit 3
  (corner)**, cond true → **sit 4 (goal kick)**. `[0x15781E]` is the returned
  `bits == 0` of the goal-mouth classifier `FUN_00070074` (FU-145 §1.4), i.e.
  1 when the camera is inside the goal-mouth band.

Scanner sound `0x974DC(0x1E)` precedes both restart arms (`0x88B64`,
`0x88BD9`).

### 1.4 Foul → free kick / penalty (sit 9 / phases 7,6)

* **Offside reception check** `FUN_00079D5C` (fresh xrefs = 1; caller `0x7A448`
  in `FUN_0007A084`; head re-read — B2's slice, cited): requires phase==2,
  `[0x157A6A] == 0` (offside suppression timer) and `[0x14C2F2] != 0`
  (**settings idx 0x10, offside enable**). It runs the nearest-own-team
  defender comparison (last defender `FUN_0008DE28(0xB10, …)`, the pitch
  half-length / goal line) and fires the kind-3 event at `0x79F1F..0x79F2B`:
  `FUN_0008A43C(EAX=3, EDX=receiver rec, ECX=rec+0x59, EBX=0)` — **kind 3 =
  offside** (the gate inside the same function skips records with `+0x91 ∈
  {0x10,0x11,0x1D,0x1E}` at `0x79F08..0x79F1D`). Kind 3 → speech 0x15 + act 6
  = **phase 0x1C** (B2 §E3/E6). This is **not** the contact/foul gate.
* `FUN_0008A43C` (fresh xrefs = 2: `0x79F2B`, `0x81EBF`) is the foul/collision
  adjudicator — **assigned wholly to B2** under the (fuzzy) B1/B2 boundary.
  First branch (word 1/2, gates `[0x14C306]`), **first-hand**:
  RNG `0x92AC8` + table `0x157B90`/`0x14C360` set `[0x15888D] ∈ {0,1,2,3}`
  (`0x8A4xx`); if `[0x15888D] == 0` →
  `FUN_0008A938(9, side, BX=1)` at **`0x8A729`** — bytes
  `0x8A70A MOV EAX,[EBP]` / `0x8A70D MOV EDX,[EAX+0x7A6]` /
  `0x8A713 MOV DL,[EDX+0x826]` / `0x8A719 MOV EBX,1` / `0x8A71E MOV EAX,9` /
  `0x8A729 CALL`. EDX = side of the offender team block's opponent-controlled
  record (`[team+0x7A6]`), i.e. the **fouled side** awarded the restart.
  Severity != 0 → foul-log arrays `0x157B3E..`/`0x157B6x` + `FUN_000888FC(3,0,1)`
  = **act 3** (phase 0x19 card cutscene). Word 3 branch → `FUN_0008F188(0x15,
  rec, 4)` + `FUN_000888FC(6,…,1)` = **act 6** (phase 0x1C).
  It stores the incident triple `[0x158897..0x15889F]`, offender
  `[0x15888F]`, `[0x158893]`, type `[0x15888C] = word low byte`.
* Second foul call: row 0x0C body at `0x81EAF..0x81EBF` re-runs the decision
  with word **1** (`[0x14C306]` gate, `[0x15888F]` rec, `[0x158893]` compare,
  `(RNG & 7) != 0`, `EDX=[0x15888F]`, `ECX=0x158897`) and sets
  `[0x15888E]=1`.
* Act-6 (phase 0x1C, `0x89110`) completion tail (`0x891CA..0x89213`,
  first-hand): when `CALL 0x4BEC8` completes, `[0x158808]=0`,
  `EDX=[0x15888F] → [EDX] → [team+0x7A6] → DL=[rec+0x826]`, `EBX=1`,
  `EAX=9`, `CALL 0x8A938` — sit 9 again after the offside cutscene.
* **Free kick vs penalty decision** in the act-2 body (phase 0x18,
  `0x8922C..`), block `0x894A2..0x895AC` (first-hand):
  `AL = [0x15888C]` (`0x894DB`); `CMP EAX,3 / JNZ 0x894F2` (`0x894E2`; both
  arms store the default 7, semantics unchanged); default `[ESP]=7`
  (`0x894F2`) then: `|incident x| >= 0x420` → keep 7
  (`0x8950F CMP EDX,0x420 / JGE`); else per foul-team side
  (`0x8951F CMP byte[EDX+0x826],0`) the z-band `-0xB10..-0x7B0` (side 0,
  `0x8952B CMP EBX,0xfffff4f0 / JL`, `0x89533 CMP EBX,0xfffff850 / JG`) or
  `0x7B0..0xB10` (side 1) writes `[ESP]=6` (`0x89550`);
  `0x89565 CMP EAX,6 / JNZ` → `FUN_0008F188(0x23, [0x158893], 4)`,
  `0x8957B` (7) → `FUN_0008F188(0x2A, [0x158893], 4)`;
  `0x89590..0x895AC`: `EAX=[0x15888F]; team side ^ 1; EAX=phase;
  CALL 0x740A0` — **phase 7 (free kick) or 6 (penalty) on the fouled side**.

### 1.5 Phase arms `FUN_0008D098` (jump table `0x8D040`, fresh read)

Table `0x8D040[0..0x15]` = `0x8D192, 0x8D1B1, 0x8D255, 0x8D2A3, 0x8D35B,
0x8D433, 0x8D4A2, 0x8D57B, 0x8D5D9, 0x8D5D9, 0x8D192, 0x8D63E, 0x8D77B,
0x8D65D, 0x8D63E, 0x8D192, 0x8D77B, 0x8D814, 0x8D814, 0x8D693, 0x8D693,
0x8D77B`. First-hand arm semantics (all installs via `FUN_0008CEB8` =
team 0..10 multi-install or `FUN_0007D9A4` single record):

| phase | arm | derived action |
|---|---|---|
| 3 throw-in | `0x8D2A3` | install **3** over team; if `byte[EBP+0x826] == byte3 [0x157AAC]` (controlled side): camera reset to the snapshot triple, `chosen = FUN_00079CCC(0x157740)`, `FUN_0008F188(0x17, chosen, 4)`, install **0x10** into chosen (`0x8D32A..0x8D349`) |
| 4 corner | `0x8D35B` | install **3**; if controlled: `FUN_0007D360` + camera reset + `chosen = FUN_00079CCC(0x157740)`; `(RNG 0x92AC8 & 3) != 0` → `FUN_0008F188(0x1A, chosen, 8)`; install **0x11** (`0x8D3F4..0x8D421`) |
| 8 goal kick | `0x8D5D9` | install **3**; if controlled: `code = (phase==9) ? 0x1E : 0x1D` (`0x8D607..0x8D621`), `chosen = EBP` (team base = **record 0, keeper**, FU-141 §: `0x8DB2E` records), `[team+0x7B2]=EBP`, install code |
| 9 keeper | `0x8D5D9` | same arm, code **0x1E** |
| 7 free kick | `0x8D57B` | install **3**; if controlled: `chosen = FUN_00079CCC(0x158897)` (the incident/foul position), install **0x12** (`0x8D5A8..0x8D5C7`); non-controlled returns (`0x8D5A2 JNZ 0x8D814`) |
| 6 penalty | `0x8D4A2` | install **3**; if controlled: stack triple via `FUN_00073DC4`/`0x157740`, camera reset, `chosen = FUN_00079CCC(stack tri)`; install **0x13** (`0x8D532..0x8D546`); else (`0x8D558`) install **0x1F** into EBP = keeper (arm-ready) |
| 0xD keeper | `0x8D65D` | controlled side: install **1** over records 1..10; else install **0** over 0..10 (`0x8D671..0x8D681`) |
| 0/0xA/0xF | `0x8D192` | install **0** over 0..10, skip-if-current 0xC |
| 0xB/0xE | `0x8D63E` | install **0** over 0..10, skip −1 |
| 0xC/0x10/0x15 | `0x8D77B` | install **3** over 0..10 |

`[0x157AAC]`>>24 is used as the taker-side gate in every set-piece arm. Fresh
operand search `0x00157aac` = 27 sites, **all reads — no direct write site in
the program**, so the global is initialized by an unlocated copy (front-end);
inherited label "controlled side" (FU-142 App. B.3, FU-89 §). Flagged as leg
L1.

### 1.6 Taker/keeper action rows (action table `0x1106E0` fresh read)

`0x1106E0[0x10]=0x0855F0` (throw-in taker), `[0x11]=0x085DE4` (corner taker),
`[0x12]=0x083D68` (free-kick taker), `[0x13]=0x084B00` (penalty taker),
`[0x1D]=0x074EB0` (keeper clear), `[0x1E]=0x07550C` (keeper claim/place),
`[0x1F]=0x076380` (keeper arm), `[3]=0x07E1A4` (hold/placement),
`[0x20]=0x084EEC` (restart-commit family).

* **Row 0x10** (window `0x85CE0..0x85D5F`, first-hand): if `[0x157821] > 0`
  signed (the `0x85CE4` gate is a signed `JLE` to the else arm) → re-queue sit 2
  (`EAX=2, EDX=rec team, EBX=1`, `0x85D0A`);
  else `[0x157A6A] = 0x12C` (`0x85D19`), `EAX=0xB, EDX=rec team, EBX=0` →
  sit 0xB (`0x85D38`), `CALL 0x4C380`, stage++.
* **Row 0x11** (`0x85DE4`, first-hand tail `0x863E6..0x863F9`): `EAX=0xB,
  EDX=rec team, EBX=0` → resit 0xB → phase 2.
* **Row 0x12** (`0x83D68`, first-hand `0x84462..0x84495`): `CALL 0x8DE8C` +
  `0x7F668` + `0x4C380`/`0x4C374`, then `EAX=0xB, EDX=rec team, EBX=0` →
  sit 0xB.
* **Row 0x13** (`0x84B00`, first-hand `0x84E61..0x84E8F`): `CALL 0x79B1C`,
  `[0x157730]`/`[0x157746]` gate, `EAX=0xB, EDX=rec team, EBX=0` → sit 0xB.
* **Rows 0x1D/0x1E** (FU-147 §3.6, byte-exact there; keeper clear/claim):
  stage-3 tails call sit 0xB after `FUN_0007A490` ball staging
  (`0x7546E`/`0x75B58`/`0x76072`).

### 1.7 Placement / restart state bodies

* Phase table `0x110794` (fresh read, 35 dwords) maps the set-piece phases to
  the per-record placement handlers derived in FU-83 §2/§3.1 (cross-cited):
  phase 3/4/7 → `0x06DE44` (formation-slot targets `v+v/4`), phase 8/9 →
  `0x06DD6C` (cell placement, `EBX = 0xBC/0x6F` by side), phase 0xD →
  `0x06DE34` (held-position copy `rec+0x59 → out`), phase 6 → `0x06DD9C`
  (ball-line entry by ball z), phase 2 → `0x06DCC8`. Engine equivalents
  already exist as `fifa96_action_phase_cell`/`_slot`/`_ball_line` helpers
  (FU-83 §4).
* Restart-commit body `0x84F60..0x84FBB` (row 0x20 = `0x84EEC..`):
  `[0x157A83]=rec`, `FUN_00073DC4`, camera reset `FUN_000700F4` to the current
  camera triple, `MOV [0x15781D],AH` (AH=0 — **clears the goal arm**,
  `0x84F90`), `CALL 0x73E08` (placement commit), `rec+0x4D := 0x15774C`
  triple, `rec+0x55 -= 0xF0`, `CALL 0x79B6C` (re-anchor). No set-piece dispatch
  row in the traced chains installs action **0x20**; it is reachable from the
  action machine only (negative within the traced chains).
* The taker rows walk the taker to the already-staged ball; the keeper rows
  are the only set-piece rows that call `FUN_0007A490` ball staging
  (FU-73/FU-139 derived).

### 1.8 Period / clock interactions

* Phase class table `0x1106AD` (fresh, 35 B):
  `02 00 01 02 02 00 02 02 02 02 00 00 00 02 00 00 00 00 00 00 00 00 00 01 00
  00 01 00 01 01 01 00 00 00 00` → class 2 = {0,3,4,6,7,8,9,0xD};
  class 1 = {2,0x17,0x1A,0x1C,0x1D,0x1E}; rest 0. **Every set-piece phase is
  class 2**: the clock machine `FUN_0008AF38` runs its body only for class 1 or
  class 2 with `[0x14C302]==0` and uses the class-2 aux arithmetic for period
  completion (FU-143 §3.5/§4.1, cross-cited; class table first-hand).
* The scanner runs only on phase 2/0x10 with the goal arm set; the extra-time
  period-4 path skips it (§1.3).
* The direct/fallback vs queue routing is decided by `[0x14C32A]` (session)
  and `[0x15B6C0]` (pending) in the dispatcher head (§1.1).
* The sit-3 corner counter `word[0x157AD4 + (side^[0x157ABE])*2]++` (fresh
  operand search `157ad4` = exactly 3 sites: reset zero `0x73F6A` in match
  reset `FUN_00073EE0`, the read `0x8ABFF`, the store `0x8AC0F`). **No reader
  exists** — the accumulated corner count is not consumed by any direct
  address reference (indirect aliasing not excluded; leg L10).
* Offside suppression timer `[0x157A6A]` (same datum as B2): set `0x12C` by
  row 0x10's resume branch (`0x85D19`; row 0x11 stores it too —
  `0x863D8 MOV ECX,0x12C` / `0x863DF MOV [0x157A6A],CX`), read `< 1` by the
  offside reception check `FUN_00079D5C 0x79D79`.
* `FUN_000740A0(2, side)` (sit 0xB) clears the goal arm and sets
  `[0x157AB2]=1`, `[0x157A73]=0x15774C` (`0x740E0..0x74107`).

---

## 2. Derived semantics (per piece: producer → row → resolution)

**Throw-in.** Producer: scanner `0x88C00` (`|snap z| <= 0xB20`, phase 2,
`EDX = ball team ^ 1`, `BX=1`) or row 0x10's own re-queue `0x85D0A`
(`[0x157821] != 0`). Row: dispatcher head → queue arm (`[0x15B6A8]=9/0`,
`[0x15B6C0]=1`) or, when the gate is closed/pending, **BX!=0 fallback**:
phase 0 + act 8 (phase 0x1E timeline) → act-8 tail re-dispatches sit 2 with
BX=0 → normal row `0x8ABDB` = `FUN_000740A0(3, side)` → **phase 3**. Phase-3
arm: install action 3 over both teams; the controlled team's nearest record to
the camera snapshot gets action **0x10** (row 0x855F0), which walks the taker
and either loops sit 2 again (`[0x157821] > 0` signed) or resumes via sit 0xB →
**phase 2**. Resolution: engine-visible state = phase byte 3, per-record
placement handler `0x06DE44`, taker action 0x10.

**Corner.** Producer: scanner `0x88BBD` with `cond == 0`
(`3+cond`, `EDX = ball team ^ 1`, `BX=0`, phase 2). Row: queue arm id 9/0 or
normal row `0x8ABF3` = **corner counter++** (`word[0x157AD4 + (side^0x157ABE)*2]`)
then `FUN_000740A0(4, side)` → **phase 4**. Phase-4 arm: install 3; controlled
team's nearest-to-`0x157740` record gets action **0x11** (row 0x85DE4) after a
1-in-4 `FUN_0008F188(0x1A,…)`. Row 0x11 resumes via sit 0xB → phase 2.

**Goal kick.** Producer: scanner `0x88BBD` with `cond == 1`
(`3+cond`, same EDX/BX) — also queued as id 9/0. Row: normal `0x8ADC0` =
`FUN_000740A0(8, side)` → **phase 8**. Phase-8 arm: install 3; controlled
team's **record 0 (keeper)** gets action **0x1D** (row 0x74EB0, close-down /
clear-vector, FU-79 §6); non-controlled team gets nothing (return
`0x8D601..0x8D814`). Row 0x1D's clear stages the ball (`0x7A490`) and resumes
via sit 0xB → phase 2.

**Free kick.** Producer: foul adjudicator `FUN_0008A43C` word-1/2 branch with
severity `[0x15888D]==0` → `0x8A729` sit 9 BX=1 (side = fouled team), or the
act-6 completion tail `0x8920A`. Row: queue arm (`=1/2`) or BX!=0 fallback
(phase 0 + act 8 → re-dispatch sit 9 BX=0) → normal `0x8AEC6` = **act 2**
(phase 0x18). Act-2's decision block writes phase 7 (free kick) or 6 (penalty)
on the fouled side from the incident position. Phase-7 arm: install 3;
controlled team's nearest-to-`0x158897` record gets action **0x12** (row
0x83D68). Row 0x12 resumes via sit 0xB → phase 2. Card path (severity != 0):
act 3 (phase 0x19 cutscene) → phase 0xF; the collision path: act 6 (phase
0x1C) → at completion re-emits sit 9 (`0x8920A`).

**Penalty (same chain).** Incident inside the penalty band (or act-2 reached
with a non-3 incident type) → phase 6: install 3; controlled-side taker gets
action **0x13** (row 0x84B00); the *other* team's keeper (record 0) gets
action **0x1F** (row 0x76380, arm/dive-ready). Row 0x13 resumes via sit 0xB.

**Keeper restarts (sit 5/7).** Producers: keeper rows 1A/1B only (4 sites).
Sit 5 (BX=1) → queue id 7 or fallback phase 0 + act 8 → re-dispatch BX=0 →
normal `0x8ADD8` = phase 9 → keeper **0x1E** (claim/place). Sit 7 (BX=0) →
queue id 7 or normal `0x8ADF0` = phase 0xD → controlled-side outfield walk
(action 1) + held-position placement. Naming of 5 vs 7 (keeper carry/throw vs
goal-kick-adjacent) is left as leg L3.

---

## 3. Port contract (engine names; existing seams reused)

Existing, reused unchanged:
* `fifa96_match_run_situation(mr, situation)` — the table-2 phase-write seam
  (`src/fifa96_engine/fifa96_match_run.c:999`). Today it applies phase 3 for
  sit 2, phase 4 for sit 3 (but **not** the corner counter), phase 8 for sit 4,
  phase 9 for sit 5, phase 0xD for sit 7, phase 2 for sit 0xB, and is a no-op
  for the act-only rows 1/8/9/0xA/0xC (`fifa96_action_phase_situation` rows,
  `src/fifa96_loader/fifa96_action_handlers.c:1108`).
* `fifa96_match_run_phase_drive(mr)` — the class-1/class-2 gate and the
  period chooser (`fifa96_match_run.c:964`). The class table it consumes must
  match §1.8 (set-piece phases are class 2).
* `fifa96_match_run_score_event(mr, side, probe)` — untouched by this slice.
* `fifa96_match_state_set_phase` / `mr->phase_machine` mirrors — the only
  phase-write path.

Gaps found (no seam): **`fifa96_match_run_session` does not exist** (fresh
grep over `src/`, `include/`, `tests/` — no hits). The native session gate
`[0x14C32A]` and the pending latch `[0x15B6C0]` have no engine state; FU-146 §7
proposes `session_gate_14c32a`/`situation_pending`/`situation_id` on the run.

Suggested new/extended surface (names matching the existing family):
1. `fifa96_action_phase_situation` — extend the row set so the derived row
   carries the **corner counter** increment (sit 3) and the act/stage for the
   restarts; or add `fifa96_action_phase_situation_side(situation, side)`
   returning `{phase, act, stage, corner_increment}`. The counter index is
   `side ^ side_swap` (`[0x157ABE]`, unported).
2. `fifa96_match_run_set_piece(mr, situation, side, bx)` — the full
   `FUN_0008A938` head: session/pending gates, queue write
   (`situation_id`/`situation_pending`, ids per §1.1), BX!=0 fallback
   (phase 0 + act-8 marker), else the table-2 row. FU-146 §7's
   `fifa96_match_run_goal_queue` and the 0xB path must stay the single shared
   entry (freeze reconciliation); sit 0xB keeps routing to
   `fifa96_match_run_situation`.
3. `fifa96_match_run_scan_restarts(mr)` — the derived `FUN_00088940`
   remainder: extend FU-145's `fifa96_match_run_goal_scan` with the
   `|snap_z| <= 0xB20 → sit 2` and `else → 3+cond` (corner/goal-kick) arms
   and the `EDX = ball_team ^ 1`/`BX` arguments.
4. `fifa96_match_run_phase_arm(mr, phase)` — the `FUN_0008D098` subset for
   phases 3/4/6/7/8/9/0xD: the per-team `install 3` loop
   (`fifa96_match_entities_install` over records 0..10), the controlled-side
   taker selection (nearest-to-triple, `FUN_00079CCC` derived as the FU-141
   pool nearest search), and the taker/keeper action codes (0x10/0x11/0x12/
   0x13/0x1D/0x1E/0x1F). The non-controlled team's block returns early for
   phases 3/4/7/8/9; phase 6's non-controlled arm installs 0x1F on its own
   record 0 (keeper).
5. `fifa96_match_run_foul_commit(mr, fouled_side, incident_x, incident_z,
   type)` — the `FUN_0008A43C` severity==0 + act-2 block: emits sit 9
   (`fifa96_match_run_set_piece(mr, 9, fouled_side, 1)`) and drives
   `fifa96_match_run_set_piece(mr, 7|6, fouled_side, 0)` with the derived
   penalty-band test (§1.4). The RNG inputs (`0x92AC8 & 0x3f`, `& 3`, `& 7`)
   must come from the engine's deterministic RNG state, not wall time.
6. State (suggested fields, native cell): `situation_id` `[0x15B6A8]`,
   `situation_pending` `[0x15B6C0]`, `session_gate_14c32a` `[0x14C32A]`,
   `sit_side_pending` `[0x15B6B8]`, `store_15882b/2c` (act-8 replay bytes),
   `corner_count[2]` `[0x157AD4]`, `side_swap` `[0x157ABE]`,
   `offside_suppress` `[0x157A6A]`, `incident_x/z` `[0x158897/0x15889B]`,
   `incident_rec` `[0x15888F]`, `foul_type` `[0x15888C]`.
7. `fifa96_match_run_frame` order: the scanner extension (3) belongs in the
   existing `fifa96_match_run_phase_drive` (the native `FUN_0008AF38 0x8B63E`
   runs after the entity chain); the phase arms (4) belong after
   `fifa96_match_state_set_phase` inside `fifa96_match_run_situation`/`_frame`
   (native `FUN_000740A0` runs both team blocks immediately).
8. Reuse `fifa96_match_run_situation` for the FU-143-wired row-01 sit-0xB
   producer (`src/fifa96_engine/fifa96_match_handlers.c:268`) unchanged.

---

## 4. Numbered legs (what would settle each)

1. **L1 — `[0x157AAC]` writer.** All 27 direct sites are reads (fresh operand
   search); the "controlled side" label is inherited (FU-142 App. B.3).
   Settle by locating the front-end block copy that seeds it, or a dynamic
   trace; the port needs the correct gate for every taker install.
2. **L2 — sit 3/sit 4 identity.** Corner vs goal kick rests on `cond` (§1.3)
   plus the phase-arm taker evidence (0x11 field taker vs 0x1D keeper). The
   team/end convention (`team 0 defends −z`) and the ball-team semantics of
   `[0x1577CA]` are not derived here. Settle by a dynamic trace of
   `FUN_0008A938` args at a corner and a goal kick.
3. **L3 — sit 5/sit 7 names.** Produced only by keeper rows 1A/1B; sit 5 →
   phase 9 (keeper 0x1E claim/place), sit 7 → phase 0xD (controlled walk +
   held-position). Which native restart each maps to (keeper throw vs
   goal-kick-adjacent reposition) is not proven. Settle by a keeper-catch
   trace.
4. **L4 — queue vs phase-row reachability in a live session.** With
   `[0x14C32A]!=0` and `[0x15B6C0]==0` every set-piece situation takes the
   queue arm, so the table-2 phase writes are only reached when the gate is
   closed or a situation is pending (the act-8 re-dispatch is the other
   route). Whether live play writes phase 3/4/8/9/7 through a pending-latched
   path must be traced (`[0x15B6C0]`/`[0x14C32A]` during a throw-in). This is
   the FU-146 OL-73/OL-87 boundary.
5. **L5 — session gate producer.** `[0x14C32A]` writers are front-end only
   (FU-146 leg 2); no `fifa96_match_run_session` seam exists. Settle by
   exporting the gate to the run (FU-146 §7 proposal) with a fixture seed.
6. **L6 — act-8 / phase-0x1E timeline body.** The BX!=0 fallback (throw-in
   BX=1, sit 5/9 BX=1) needs `FUN_0008A798` (phase 0x1E) ported only to the
   re-dispatch point (`0x8A8A5..0x8A8DE` is byte-exact); the stages
   `0x4BEC8` wait is unported.
7. **L7 — selection/presentation helpers.** `FUN_00079CCC` (nearest to
   triple), `FUN_0007D360`/`FUN_00073DC4` (phase 4/6 stack triples),
   `FUN_0008F188` event ids 0x17/0x1A/0x23/0x2A, `FUN_000974DC(0x1E)` sound:
   unported; the port substitutes the pool nearest search and skips sinks
   (same ruling as FU-145 L3).
8. **L8 — penalty chain remainder.** Phase 6 arm, taker row 0x13 and keeper
   0x1F are derived; the row-0x13 body and the penalty-specific placement are
   not. Also `FUN_00073DC4`'s triple source.
9. **L9 — card-cutscene resume.** Act 3 (phase 0x19) and act 6 (phase 0x1C)
   bodies are FU-83-level only; the act-6 completion `0x891CA..0x89213` is
   byte-exact (producer) but the phase-0x19 body's route back to act 2 is not
   derived (B2's foul slice overlaps).
10. **L10 — corner counter consumers.** Fresh operand search `157ad4` finds
   only reset + increment; no reader. If the port keeps the counter it is
   write-only presentation (or read via aliasing not found statically).
11. **L11 — `[0x157821]`/`[0x157A6A]` semantics.** Row 0x10's loop gate (the
   signed `JLE` on `[0x157821]`) and the offside suppression timer `[0x157A6A]`
   are read first-hand; their producers (`FUN_000700F4`/`FUN_000709D0`
   writes/increments) are presentation/camera side and not derived here.

---

## 5. Risks

* **Queue vs direct double-write.** Wiring both the queue arm and the table-2
  phase rows without the `[0x14C32A]`/`[0x15B6C0]` routing reproduces the
  FU-146 "two score writers" hazard for every restart (phases would fire on a
  frame the native only queues). Pin with the L4 fixture.
* **Corner counter index.** Indexing by `side` instead of `side ^ [0x157ABE]`
  silently swaps the counter slots (and would also swap the score words if
  reused).
* **Taker gate.** Installing the taker/keeper action for *both* teams, or for
  the wrong team, changes the visible action codes; every set-piece arm gates
  on `byte[team+0x826] == byte3 [0x157AAC]` with an early return for the
  other team (except phase 6's keeper 0x1F arm, which installs on the *other*
  team's record 0).
* **Width/sign.** The penalty-band and |x| tests are 32-bit compares on the
  incident triple (`0x158897/9B`); `player+0x826` is a byte; `[0x15888C]` is
  the low byte of the adjudicator word. Truncate to the wrong width and the
  phase-7/6 split moves.
* **Out-of-range situation.** `FUN_0008A938`'s queue table defaults
  situation >10 to id 0xA; the engine's `fifa96_action_phase_situation`
  rejects `>= 0x0D` — keep the native "default id 0xA" behavior in the new
  head rather than the hardened rejection.
* **RNG cadence.** Foul severity (`& 0x3f`), phase-4 taker event (`& 3`),
  collision decision (`& 7`) consume the shared deterministic RNG
  `FUN_00092AC8`; the port must sequence them exactly or cards/free kicks
  diverge.
* **Cross-task overlap.** B2 owns the referee side wholly (act 3/phase 0x19,
  `FUN_0008A43C` — the foul decision, assigned entirely to B2 despite the
  fuzzy boundary — the word-1/2 classification tables and the foul log
  `0x157B3E..`); B1 derives only the severity==0 → sit 9 edge and the act-2
  free-kick/penalty decision. The port must not fork two foul implementations.

---

## 6. Port landing (P1, 2026-10-09)

Landed from this frozen slice during phase-7 wave-7 P1 (`src/fifa96_engine/fifa96_match_run.c`
/ `fifa96_match_run.h`, `src/fifa96_loader/fifa96_action_handlers.{c,h}`; first-hand
re-verified on `/FIFA96.EXE` this slice: `disassemble_bytes` `0x8A938`/`0x8A99E`/
`0x8A9CD`/`0x8A9E8`/`0x8AA23`/`0x8AA60..0x8AB8F`/`0x8A8A5..0x8A8DE`/`0x8ABDB`/`0x8ABF3`/
`0x88B40..0x88C0E`, `0x855F0` head/`0x85CB0` tail, `0x8D2A3`/`0x8D35B`/`0x8D4A2`/
`0x8D57B`/`0x8D5D9`/`0x8D63E`/`0x8D65D`, `0x73F50..0x73FAA`; `decompile_function`
`0x8D098`/`0x79CCC`/`0x73DC4`/`0x7D360`/`0x8C974`; `read_memory` `0x8A8E0`/`0x8D040`/
`0x10E6E0`/`0x1106E0`; `search_instructions` operand `157ad`/`157b8`/`15889`):

1. **`fifa96_action_phase_situation` row extension** — the row struct/out carries
   `corner_increment` (1 on situation 3 only), the derived request of the native
   `0x8ABF3..0x8AC1C` counter increment. `tests/test_phase_drivers.c` pins every row.
2. **`fifa96_match_run_set_piece(mr, situation, side, bx)`** — the full
   `FUN_0008A938` head: the `0x8A944..0x8A96B` gates (sit 0/0xB, closed
   `session_gate_14c32a`, pending), the `0x8A8E0` table-1 queue ids
   (2/3/4 → 9 side 0 / 0 side 1; 5/7 → 7; 6 → 5/6; 9/10 → 1 side 1 / 2 side 0;
   sit 1, 8, 0xC and >10 → the `0x8AA60` id 0xA), the `[0x15B6B8]` side latch
   (`sit_side_pending`), the BX!=0 fallback (`0x8AA80..0x8AAA3` phase
   `FUN_000740A0(0, 0)` + the `[0x15882B]`/`[0x15882C]` act-8 replay bytes, then
   act 8's tail `0x8A8A5..0x8A8DE` re-dispatching with BX=0 and writing
   `[0x15882B] = 0xFF`), and the BX==0 table-2 route through the shared
   `fifa96_match_run_situation` entry (situation 6 keeps its score fallback;
   situation 3 counts `side ^ side_swap` before the write).
3. **`fifa96_match_run_phase_arm(mr, phase)`** — the `FUN_0008D098` subset for
   phases 3/4/6/7/8/9/0xD: the per-team install 3 over records 0..10
   (`fifa96_match_arm_install_multi`, the `FUN_0008CEB8` port), the controlled
   gate `[team+0x826] == [0x157AAC]>>24` (`phase_machine.side_controlled`) with
   the non-controlled early return, the `FUN_00079CCC` derived pick (pool
   nearest over record targets, skip 0, fallback record 0), the camera resets
   (`fifa96_camera_init`: snapshot for phase 3, the `FUN_0007D360` corner probe
   `(±0x710, 0, ±0xB00)` for 4, the `FUN_00073DC4` penalty spot `(0, 0, ±0x8D0)`
   for 6), the taker/keeper codes 0x10/0x11/0x12/0x13/0x1D/0x1E/0x1F (phase 6's
   non-controlled team arms 0x1F on its record 0), and the phase-0xD arm
   (`0x8D65D`, controlled code 0 over records 1..10 / else 0..10 — **no**
   install-3 prefix, unlike the set-piece arms, first-hand). The
   `FUN_0008F188(0x17/0x1A)` event sinks stay dropped (L7) while the phase-4
   `FUN_00092AC8 & 3` draw is still consumed.
4. **Scanner extensions** (`fifa96_match_run_goal_scan`, `0x8896B..0x88C0E`):
   `|snap z| <= 0xB20` → throw-in sit 2 (`0x88BCC`, phase-2 only, `EDX =
   ball_team ^ 1`, BX=1); `zone == 0` → corner/goal-kick sit
   `3 + ((snap z < 0) == (ball_team == 1))` (`0x88B53`, phase-2 only, EDX =
   ball_team ^ 1, BX=0); else the goal arm (unchanged). `ball_team` is the
   `[0x1577CA]` stand-in (pool controlled actor, then ball carrier — the FU-145
   L4 identity remains unported). The `0x974DC(0x1E)` scanner sound is dropped
   (L7).
5. **State** (`struct fifa96_match_run`): `sit_side_pending` `[0x15B6B8]`,
   `corner_count[2]` `[0x157AD4]/[0x157AD6]` (match-reset/begin zeroed),
   `side_swap` `[0x157ABE]`, `store_15882b`/`store_15882c` `[0x15882B/C]`,
   `incident_x`/`incident_z` `[0x158897]/[0x15889F]` (the FU-150 producer stays
   P2; seeded 0).
6. **Tests** (`tests/test_engine_match_frame.c` + `tests/test_phase_drivers.c`,
   ASan/UBSan): `test_set_piece_queue_ids`, `test_set_piece_bx_fallback`,
   `test_set_piece_phase_arm_codes`, `test_corner_counter_side_swap`,
   `test_scan_restart_arms`, plus the updated `test_goal_scan_queue_and_fallback`
   / `test_view_pose_feed_arms_camera`. The set pieces reach their phases
   through the scanner + arm with the counter, keeper codes and BX fallback
   pinned; resume rows are L13. ISO not required.
   `make check` 106/106; **M1 and M2 goldens byte-identical** (the tape camera
   never arms a set piece and the forced phases bypass the situation seam, so
   no presented frame moved — the dormant-chain outcome, `cmp` clean).

### Errata / port decisions

1. **Incident z cell (slice §3 field list).** The slice's state list writes
   "incident_x/z `[0x158897/0x15889B]`"; first-hand the incident triple is
   12 bytes x@0x158897 / y@0x15889B / z@0x15889F and `FUN_00079CCC` reads
   `param_1[4]` = +8 = **0x15889F** (`0x823D6`/`0x823FA`/`0x89286`/`0x8D5A8`
   family). The port uses 0x15889F (`incident_z`); 0x15889B is the y dword.
2. **Phase-0xD arm has no install-3.** `0x8D65D` starts at the controlled
   check (first-hand), so the 0xD arm only runs the code-0 installs; the port
   splits it from the common install-3 prefix the 3/4/6/7/8/9 arms share.
3. **Pick stand-in.** `FUN_00079CCC`'s per-record distance source is the
   record's phase handler `[rec+0x1C]` placement output (unported); the port
   substitutes the record target triple (the FU-143 §11.1 kickoff-pick ruling,
   L7), keeping skip 0, the `+0x98`/`+0x9A` exclusions, the strict minimum and
   the team-base no-candidate fallback.
4. **`[0x1577CA]` ball-side stand-in.** All four scanner/queue sides derive
   from the native ball-track record's team byte; the record identity is
   unported (FU-145 L4), so the port reads the pool controlled actor, then the
   ball carrier, else side 0.
5. **Act-8 compression (L6).** The BX!=0 fallback's phase-0x1E timeline stages
   are unported; the port models the byte-exact re-dispatch point, so the
   stored situation/side lands on the table-2 row on the same call. The
   phase-0 arm (`0x8D192`) is not run: its installs are overwritten by the
   re-dispatched row's own arm (transient).
6. **Queue ids are the S3 consumer's (L4).** The head writes `situation_id` /
   `situation_pending` only; the FU-146 screen machinery consumes the same
   cell, and whether live play reaches the table-2 phases through a
   pending-latched path stays the L4 trace.
7. **Table-2 route always.** The `0x8AB08` sit 2..4 arm (the `FUN_0008C974`
   formation-order counters `[0x157B8E]`/`[0x157B8F]` gate + act 4) and the
   sit-1 arm (`0x8AABF`) are unported act-handler machinery (L12): the derived
   BX==0 route is the table-2 row.
8. **Keeper phase split.** Phase 9's `team->target`/record-0 writes and the
   phase-0xD installs are derived; their producers (keeper rows 1A/1B) stay
   FU-151/P3, so the sit 5/7 table rows are reachable but dormant in a live
   match.

### Legs status after P1

| leg | status |
|---|---|
| L1 `[0x157AAC]` writer | open — the controlled side is carried (`phase_machine.side_controlled`, seeded 0); the arm gate is faithful |
| L2 sit 3/sit 4 identity | **derived landed**: the scanner cond formula + the taker-code evidence are implemented; the dynamic-trace confirmation stays open |
| L3 sit 5/sit 7 names | open (keeper rows 1A/1B are FU-151/P3) |
| L4 queue vs phase-row reachability | open — the head writes the queue faithfully; the table-2 route is taken when the gate is closed/pending or a situation is already pending |
| L5 session gate producer | open — begin seeds 1 (the FU-146 leg 2 seam) |
| L6 act-8 / phase-0x1E timeline | open — the re-dispatch point is ported, the timeline stages are not |
| L7 selection/presentation helpers | open — pick substituted, `0x8F188`/`0x974DC` sinks dropped, the phase-4 RNG draw kept |
| L8 penalty remainder | open — the phase-6 arm is ported (spot probe, 0x13/0x1F); the row-0x13 body and `FUN_00073DC4`'s triple producer beyond the derived spot stay P2 |
| L9 card-cutscene resume | open (B2/FU-150) |
| L10 corner-counter consumers | open — the pair is written only (no static reader), fixtured |
| L11 `[0x157821]`/`[0x157A6A]` | open — row 0x10/0x11 are L13, so neither the requeue loop gate nor the offside-suppress timer is written yet |
| **L12** sit 2..4 `[0x157B8E]`/`[0x157B8F]` gate + act-4 arm, sit-1 arm | new — unported act-handler machinery; the port routes the table-2 row |
| **L13** taker/keeper row bodies 0x10/0x11/0x1D | new — the arms install the codes, but the rows' stage machines (`0x855F0` 7-stage, `0x85DE4` 10-stage, `0x74EB0` keeper) stay unported, so the FU-149 §1.6 resume tails (sit 0xB, offside timer, requeue) are not yet executable; FU-151 owns 0x1D |
