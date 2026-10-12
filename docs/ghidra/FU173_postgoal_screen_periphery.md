# FU-173 — post-goal screen/hold periphery (R21)

> Phase-14 wave R21 (2026-10-12), **read-only** on **`/FIFA96.EXE`** (explicit
> `program` argument in every call; no renames, comments, labels, function
> creation, scripts or project saves). Scope: what the engine must model for
> the post-goal hold to *run* and *release* naturally — the goal queue ids 5/6
> (`0x8A9E8`), the scheduler `FUN_000948AC` (`0x4B1A1`, `0x949E9 CALL
> [0x15B6D4]`), the installer `FUN_00092D8C`/`FUN_00092E2C` (`0x92EEE`), the
> advance `FUN_000935A0` — against what the port already has (FU-146 S3/T4,
> FU-142 App. L, FU-165, FU-166), with the minimal T3 rec and numbered legs.
>
> **Bounded-slice note.** This slice landed the bounded part (the four named
> periphery pieces + the two session-shaped run/release routes) and numbered
> the rest; no scheduler explosion. The engine's live default (byte-0) turns
> out to take a *different* post-goal route than the tape's staged byte-1
> chain; both are derived here to the release write.

## 1. Answer in one line

The four named periphery pieces are **already derived (FU-146) and landed in
the engine**; first-hand this slice re-verified every load-bearing byte,
found one doc mislabel (FU-146 §2 reads "period" for arms that test the **leg
`[0x15B680]`** — the engine implements the leg, correctly) and one table hole
(FU-149 §1.5 omits phase 5's arm). The hold's **run** is session-shaped
(FU-166): byte-1 (Practise) takes queue → scheduler → handler (ported); **the
engine's settled byte-0 live default never queues** — its natural goal takes
the direct fallback, writes **phase 5**, and phase 5's native arm **`0x8D433`**
(not in FU-149's table) installs code 0x15/0x14, whose **code-0x14 stage 2
releases to phase 1 at `0x87C71`** — none of which the engine models. For the
tape's byte-1 hold, the **release** is FU-165 R1 (act-9 producer + the two
re-queues); the periphery needs **no new work**.

## 2. Method / traps

All windows first-hand this slice unless marked *cross-cited* (the frozen FU
doc is named). Alias traps applied at every load; the load-bearing pair reads
are `[0x157A4A]>>24` (phase, `0x93CFE`), `[0x15B680]` dword (leg — **no**
dword/word alias involved; the scheduler reads the cell whole), and the
`[0x158816]`-family pair rule is FU-165's (cross-cited). All call targets
quoted from the CALL bytes. Fresh censuses this slice: `get_xrefs_to 0x948AC`
= **2**, `0x92D8C` = **2**, `0x92E2C` = **1**, `0x935A0` = **6**,
`0x15B6D4` = **7**, `0x8A938` = **39**, `0x157A9F` = **18** (matches FU-146/FU-165
where they intersect). `read_memory`: `0x110F78` (24 B), `0x8D040` (92 B),
`0x110794` (24 B), `0x1106E0` (88 B).

## 3. Evidence — the four named periphery pieces

### 3.1 Goal queue ids 5/6 (`0x8A9E8`) — re-verified

Fresh `disassemble_bytes 0x8A9E0..0x8AA30`:

| site | bytes | disasm |
|---|---|---|
| `0x8A9E8` | `66 85 f6` / `75 1b` | `TEST SI,SI` / `JNZ 0x8AA08` |
| `0x8A9ED` | `c7 05 a8 b6 15 00 05 00 00 00` | `[0x15B6A8] = 5` (side 0) |
| `0x8A9F7` | `c7 05 c0 b6 15 00 01 00 00 00` | `[0x15B6C0] = 1` |
| `0x8AA08` | `c7 05 a8 b6 15 00 06 00 00 00` | `[0x15B6A8] = 6` (side != 0) |
| `0x8AA12` | `c7 05 c0 b6 15 00 01 00 00 00` | latch = 1 |

Entry gate (FU-146 §1, cross-cited and unchanged): `0x8A957 CMP byte
[0x14C32A],0 / JZ 0x8AA7B` — **a byte-0 session never reaches this arm**; its
situation-6 call takes the direct path (`0x8AA7B` → table 6 `0x8AC28`). The
engine mirrors the arm exactly (`fifa96_match_run_set_piece` case 6,
`fifa96_match_run.c:2656-2658`) and the gate (`:2643`).

### 3.2 Scheduler `FUN_000948AC` (`0x948AC..0x949F7`, 83 insns) — fresh full window

Frame gate (fresh `0x4B190..0x4B1B0`): `0x4B193 CALL 0x736AC`;
`0x4B198 80 3d 2a c3 14 00 00` = `CMP byte [0x14C32A],0`; `0x4B19F 74 05` =
`JZ 0x4B1A6`; `0x4B1A1 e8 06 97 04 00` = `CALL 0x948AC`; `0x4B1A6 e8 8d fd 03
00` = `CALL 0x8AF38`. Order: **scheduler before the clock body**; both gated
on the byte being **non-zero** (byte-1 session).

Scheduler body (fresh, all sites):

| site | bytes | disasm | arm |
|---|---|---|---|
| `0x948B2` | `a1 88 b6 15 00` | `MOV EAX,[0x15B688]` | screen timer |
| `0x948B7` | `3b 05 94 b6 15 00` | `CMP EAX,dword [0x15B694]` | vs period frames |
| `0x948BD` | `7e 26` | `JLE 0x948E5` | no rollover |
| `0x948BF` | `83 3d 80 b6 15 00 05` | `CMP dword [0x15B680],5` | **leg==5?** |
| `0x948CE` | `83 c0 07` | `ADD EAX,7` | id 8 (leg != 5) / 7 |
| `0x948D1` | `a3 a8 b6 15 00` | `[0x15B6A8]=EAX`; latch (`0x948D6`) | |
| `0x948E5` / `0x9492A` | `83 3d 80 b6 15 00 02` | `CMP dword [0x15B680],2` | **leg==2** for ids 3/4 |
| `0x948EE..0x94919` | `8b 0d 83 7a 15 00` … | `[0x157A83]` record, `[[..]]+0x826 != 0` | id 3 |
| `0x94933..0x9496C` | `8b 2d 83 7a 15 00` … | same, side==0/NULL | id 4 |
| `0x9497D` | `8b 15 80 b6 15 00` / `83 fa 04` / `83 fa 02` | `[0x15B680]` != 4 && != 2 | id 9 gate |
| `0x9498D` / `0x94996` | `83 3d 54 77 15 00 00` / `66 83 3d c2 77 15 00 00` | `[0x157754] < 0` && `word[0x1577C2] < 0` | id 9 (`0x949AA`) |
| `0x949B7` | `83 3d 80 b6 15 00 05` | `CMP dword [0x15B680],5` | **leg==5** |
| `0x949C0` | `66 83 3d c2 77 15 00 00` | `word[0x1577C2] < 0` | id 7 (`0x949D4`) |
| `0x949E0`/`0x949E9` | `83 3d d4 b6 15 00 00` / `ff 15 d4 b6 15 00` | `CMP [0x15B6D4],0` / **`CALL dword [0x15B6D4]`** | the installed handler |

Fresh `get_xrefs_to 0x948AC` = **2**: `0x4B1A1` (`FUN_0004B100`) and `0x743EA`
(the setup/reset block). Fresh `get_xrefs_to 0x15B6D4` = **7** exactly
(writes `0x92DB9`/`0x92EEE`/`0x93701`; reads/calls `0x92EF7`/`0x9370A` and the
scheduler's `0x949E0`/`0x949E9`).

**Erratum candidate (doc, not port).** FU-146 §2's condition column labels the
ids-3/4/9/7 arms "period==2 / period != 4 && != 2 / period==5". First-hand the
code reads **`[0x15B680]` (the leg)** at `0x948E5`, `0x9492A`, `0x9497D`,
`0x949B7` — never `[0x15B6BC]` (mode) or a period cell. The engine's
`fifa96_match_run_screen_schedule` implements the leg
(`fifa96_match_run.c:3043-3064`: `screen_leg == 2`, `!= 4 && != 2`, `== 5`) and
is **correct**; only the FU-146 wording is wrong.

### 3.3 Installer `FUN_00092D8C` (`0x92D8C..0x92E28`) + `FUN_00092E2C` (`0x92E2C..0x92EFF`)

Fresh full window (98 insns). The load-bearing sites:

| site | bytes | disasm |
|---|---|---|
| `0x92D90`/`0x92D92` | `89 c1` / `e8 8d 5a fe ff` | `MOV ECX,EAX` / `CALL 0x78824` (the tracked-flag zeroing head) |
| `0x92DC5` | `a3 80 b6 15 00` | `[0x15B680] = leg` |
| `0x92DCD` | `89 3d c0 b6 15 00` | `[0x15B6C0] = 1` (the latch) |
| `0x92DD3` | `a3 bc b6 15 00` | `[0x15B6BC] = mode` |
| `0x92DDF`/`0x92DEF`/`0x92E00`/`0x92E08` | — | tracked side: arg / 1 / 0 / −1 |
| `0x92E12..0x92E1E` | `bf c8 b6 15 00` … `a5 a5 a5` | `0x10F328 → 0x15B6C8` (3 dwords) |
| `0x92E1F` | `e8 08 00 00 00` | `CALL 0x92E2C` |
| `0x92E3F` | `89 15 b8 b6 15 00` | `[0x15B6B8] = 0` |
| `0x92E45`/`0x92E4A` | `e8 ea 11 fe ff` / `e8 2d 13 fe ff` | `CALL 0x74034` / `CALL 0x7417C` |
| `0x92E7B`/`0x92E82` | `66 89 15 c7 7a 15 00` / …`c5 7a 15 00` | **score pair zeroed** (`0x157AC7`/`0x157AC5`) |
| `0x92EC4` | `66 a1 80 b6 15 00` / `98` | `MOV AX,[0x15B680]` / `CWDE` |
| `0x92ED5` | `05 78 0f 11 00` | `+ 0x110F78` (the handler table) |
| `0x92EEE` | `a3 d4 b6 15 00` | **`[0x15B6D4] = handler`** |
| `0x92EF3..0x92EF7` | `85 c0 74 06` / `ff 15 d4 b6 15 00` | `TEST EAX,EAX` / `CALL dword [0x15B6D4]` (runs it once) |

Fresh `read_memory 0x110F78` (24 B) = `bc 3b 09 00 20 3e 09 00 a4 40 09 00
70 42 09 00 fc 44 09 00 c4 46 09 00` → legs 0..5 =
`{0x93BBC, 0x93E20, 0x940A4, 0x94270, 0x944FC, 0x946C4}` (FU-146 §3 exact).
Fresh `get_xrefs_to 0x92D8C` = **2**: `0x38DCC` (in `FUN_00038630`, the
match-screen arm — the front-end install, FU-146 §3's argument path from
`0x38A81..0x38B33`) and `0x3BF9E` (in `FUN_0003BB1C`). Fresh `0x92E2C` = **1**
(`0x92E1F`).

**Hold-relevant read:** the installer zeroes the score pair and latches the
handler; it is the *match-screen entry* install (front-end, once), **not** a
per-goal reset — the per-goal re-install is the advance's handler-pointer
write (`0x93701`), which does **not** touch the score. The engine's
`fifa96_match_run_screen_install` (`fifa96_match_run.c:3069-3113`) carries the
same split (begin-only; `begin` calls it at `:2229`).

### 3.4 Advance `FUN_000935A0` — re-verified head/tail (body FU-165 §2)

Fresh `get_xrefs_to 0x935A0` = **6**, exactly the six handler TAIL sites
`0x93DD6, 0x94066, 0x94225, 0x944C8, 0x946A6, 0x9489D`. Fresh tail window
`0x936D2..0x9370A`:

```
0x936D2 66 a1 80 b6 15 00   MOV AX,[0x15B680]     ; leg
0x936DE a3 ac b6 15 00      [0x15B6AC] = leg
0x936E8 05 78 0f 11 00      ADD EAX,0x110F78
0x936ED 89 3d c4 b6 15 00   [0x15B6C4] = 1        ; install hint
0x936F3 89 2d 88 b6 15 00   [0x15B688] = 0        ; timer
0x936FB 89 2d b0 b6 15 00   [0x15B6B0] = 0        ; step
0x93701 a3 d4 b6 15 00      [0x15B6D4] = handler  ; re-install
0x9370A ff 15 d4 b6 15 00   CALL dword [0x15B6D4] ; run step 0
```

So the advance **re-enters the same-leg handler** and does not restart play
itself (FU-165 §2 stands: the exits are the video selector `FUN_0004C324`
and the screen register `FUN_00054104(0x11)`). The engine's
`fifa96_match_run_screen_advance` (`:2999-3031`) mirrors the subset.

## 4. Evidence — the hold's run and release are session-shaped

### 4.1 Byte-1 (the tape's staged session): run ported, release = FU-165 R1

The tape stages `[0x14C32A] = 1` (FU-166 T3 status), so its goal queues id 5/6,
the next frame's scheduler dispatches the installed handler, the handler's
POST step consumes the id through the writer and writes phase 0
(`0x93DB2`), and step 5 waits `[0x15B688] > 0xB4`. Fresh leg-0 bytes confirm
the engine's model of every step:

* SETUP0 `0x93C30 88 25 2a 88 15 00` = `MOV byte [0x15882A],AH` (AH=0 clear);
  `0x93C41 e8 f2 6c ff ff` = `CALL 0x8A938` (sit 0xC).
* GATE `0x93C59 80 3d 2a 88 15 00 00` / `0f 84 75 01 00 00` =
  `CMP byte [0x15882A],0 / JZ 0x93DDB` — the engine's second freeze layer.
* SETUP2 `0x93C7B..0x93CFD`: `0x93C87 88 35 1d 78 15 00` = `[0x15781D]=1`
  (pan re-arm), the `0x158897` copies, the duration
  `0x1110EC[mode*24+leg]*60` (`0x93CB5`, `0x93CCA`), the two word clears
  (`0x93CCF`/`0x93CD6`), `0x93CE4 e8 4f 6c ff ff` = `CALL 0x8A938` (sit 3).
* Step 3 `0x93CFE a1 4a 7a 15 00 / c1 f8 18 / 83 f8 02` = phase gate.
* TAIL `0x93DCA 81 3d 88 b6 15 00 b4 00 00 00 / 7e 05 / 0x93DD6 e8 c5 f7 ff
  ff` = `CMP dword [0x15B688],0xB4 / JLE / CALL 0x935A0`.

**Release** (tape path): the SETUP0 sit-0xC and SETUP2 sit-3 queues and the
act-9 gate producer are the only missing links (FU-165 §6/§7 R1); the
scheduler/installer/advance periphery needs **no new work** — first-hand §3
above plus FU-146 S3/T4. The scheduler's rollover arm (§3.2) is the only
*natural* escape from a wedged hold (queues id 8/7 when the timer exceeds the
seeded period) and is already modeled.

### 4.2 Byte-0 (the engine's live default): a different hold, with a native release

Under byte-0 (`fifa96_match_run_begin` seeds 0; FU-166/R14) the situation-6
call takes the direct path: `match_run_goal_fallback` (`:2623-2628`) =
`add_goal` + table-2 row 6 → **phase 5** (`0x8ADAC`, FU-146 §1). Phase 5's
native arm was **missing from FU-149 §1.5** and is first-hand here:

Fresh `read_memory 0x8D040` (22 dwords, phases 0..0x15):
`0x8D192, 0x8D1B1, 0x8D255, 0x8D2A3, 0x8D35B, 0x8D433, 0x8D4A2, 0x8D57B,
0x8D5D9, 0x8D5D9, 0x8D192, 0x8D63E, 0x8D77B, 0x8D65D, 0x8D63E, 0x8D192,
0x8D77B, 0x8D814, 0x8D814, 0x8D693, 0x8D693, 0x8D77B` →
**`[5] = 0x8D433`**.

Fresh `0x8D433..0x8D4A1`:

```
0x8D433 8b 15 9f 7a 15 00   MOV EDX,[0x157A9F]   ; the goal record pointer
0x8D439 8b 12               MOV EDX,[EDX]        ; its team pointer
0x8D43B 8a 85 26 08 00 00   MOV AL,[EBP+0x826]   ; the armed team's side
0x8D441 3a 82 26 08 00 00   CMP AL,[EDX+0x826]
0x8D447 75 33               JNZ 0x8D47C          ; other team
  same team:
0x8D449 6a ff               PUSH -1              ; skip
0x8D44B b9 15 00 00 00      MOV ECX,0x15         ; code 0x15
0x8D450 bb 0a 00 00 00      MOV EBX,0xA          ; last=10
0x8D455 ba 01 00 00 00      MOV EDX,1            ; first=1
0x8D45A 89 e8               MOV EAX,EBP          ; team base
0x8D45C e8 57 fa ff ff      CALL 0x8CEB8         ; install 0x15 over records 1..10
0x8D461 ba 14 00 00 00      MOV EDX,0x14         ; code 0x14
0x8D466 a1 9f 7a 15 00      MOV EAX,[0x157A9F]
0x8D46F 89 85 b2 07 00 00   [EBP+0x7B2] = rec
0x8D475 e8 2a 05 ff ff      CALL 0x7D9A4         ; install 0x14 on the goal record
  other team (0x8D47C):
0x8D47E bb 0a ... 31 c9 31 d2   first=0, last=10, code=0 → install 0
```

The release is inside **code 0x14** (`0x8784C`, the 3-stage scripted
celebration, FU-82 §3.5 cross-cited) — fresh `0x87C48..0x87C8F`:

```
0x87C49 CALL 0x700F4        ; camera reset (the 0x10F328/2C/30 triple)
0x87C4E CALL 0x73E28        ; the phase reset (FU-143 §1.3)
0x87C53 8b 45 00 / 8a 80 26 08 00 00  MOV EAX,[EBP] / MOV AL,[EAX+0x826]
0x87C5C 34 01               XOR AL,1
0x87C5E a2 af 7a 15 00      [0x157AAF] = team side ^ 1
0x87C63 8b 15 ac 7a 15 00   MOV EDX,[0x157AAC]
0x87C69 b8 01 00 00 00      MOV EAX,1            ; phase 1 (kickoff)
0x87C6E c1 fa 18            SAR EDX,0x18         ; controlled side
0x87C71 e8 2a c4 fe ff      CALL 0x740A0         ; FUN_000740A0(1, controlled)
0x87C76 e8 8d c1 fe ff      CALL 0x73E08         ; placement commit
0x87C7B/0x87C88             EAX=[EBP]; EDX=0x26; CALL 0x8CFAC  (team, then ball)
```

So the byte-0 natural post-goal chain is: **goal → direct sit-6 → score +
phase 5 → arm 5 installs code 0x15 (records 1..10 of the goal-record team)
and code 0x14 (the `[0x157A9F]` record) → code 0x14's 3 stages run → stage 2
writes phase 1 (`0x87C71`) + commits → the kickoff arm (native phase-1 arm
`0x8D040[1] = 0x8D1B1`, ported as `fifa96_match_phase_machine_kickoff`) →
row 01 kick → phase 2.** The clock is stopped for the whole phase-5
celebration (class 0, `0x1106AD`; first-hand `0x110794[5] = 0x6E05C`, the
FU-83 distance line).

Fresh `get_xrefs_to 0x157A9F` = **18**: writers `0x73E66` (`FUN_00073E28`,
the reset) and `0x88955`/`0x889e0`/`0x88ab3`/`0x88ae3` (all in `FUN_00088940`
— the goal scanner); readers include the phase-5 arm (`0x8D433`/`0x8D466`),
`FUN_0008A938`'s direct sit-6 body (`0x8AD0F`/`0x8AD69`), the phase-05
placement handler family (`0x6E068`/`0x6E0DF`/`0x6E104`) and code bodies
(`0x87586`/`0x87D16` = code 0x14/0x15 ranges, `0x89E0C`). The engine computes
the same record in `match_run_goal_nearest` and **discards it**
(`fifa96_match_run.c:2757` `(void)match_run_goal_nearest(...)`), so the cell
is unmodeled today (the header's `sub.special` is only the
`[0x1587D4] == 0x157A9F` comparison, `fifa96_match_run.h:225`).

### 4.3 `0x1106E0` row check

Fresh `read_memory 0x1106E0` (88 B): `[0x14] = 0x8784C` (the celebration
row), `[0x15] = 0x87CD0` (the "gate phase 5 else RESET" row, FU-81 §2). Both
rows are **unwired** in the dispatch census (`docs/ENGINE.md:262`: wired =
`00,01,02,05,04,06,07,08,0E,0F,18,1E,21,23,26,28,2A,1D,10,11,12,13`, 22/80);
FU-136's row table lists row 0x14 "not ported (partial) — stage 2 and arm
timing unported" (OL-9) and row 0x15 not ported.

## 5. What the port already models (status)

| native piece | FU derivation | engine status | first-hand this slice |
|---|---|---|---|
| goal queue ids 5/6 (`0x8A9E8`) | FU-146 §1 | WIRED (`set_piece` case 6, `:2656`) | bytes re-read ✓ |
| direct fallback (`0x8AA7B`→`0x8AC28`, phase 5) | FU-146 §1 / FU-149 §1.1 | WIRED (`match_run_goal_fallback`, `:2623`; test `test_natural_goal_fallback_arm`) | phase-5 body cross-cited |
| frame gate `0x4B198` + scheduler | FU-146 §2 | WIRED, byte-1 gated (`:3550`); byte-0 skip | gate bytes ✓ |
| scheduler body (rollover, ids 3/4/9/7, tail call) | FU-146 §2 | WIRED (`:3033-3067`), leg-keyed (correct) | 83-insn window ✓ |
| installer `0x92D8C`/`0x92E2C` (`0x92EEE`) | FU-146 §3 | WIRED (`:3069-3113`, begin-only) | full window ✓ |
| six handlers / step machines (`0x110F78`) | FU-146 §4 | WIRED (`screen_step`, `:2917`; kinds table `:2813`) | SETUP0/GATE/SETUP2/step3/TAIL ✓ |
| advance `FUN_000935A0` subset | FU-146 §4 / FU-165 §2 | WIRED (`:2999-3031`); thresholds/exits leg | tail ✓ |
| act-9 gate producer + sit-0xC/3 queues | FU-165 §6/§7 R1 | **MISSING** (the two freeze links) | step bytes ✓ |
| phase-5 arm `0x8D433` + the `[0x157A9F]` cell | **new here** | **MISSING** (`phase_arm` set `:2395-2399` has 3/4/6/7/8/9/0xD only) | arm + release ✓ |
| row 0x14 stage-2 release (`0x87C71`) | FU-82 §3.5 / FU-136 row 14 | **MISSING** (scatter celebration partial, unwired) | release tail ✓ |
| row 0x15 (`0x87CD0`) | FU-81 §2 | **MISSING** | table entry ✓ |
| phase-1 kickoff arm `0x8D1B1` | FU-143 §11 / phase machine | WIRED (`fifa96_match_phase_machine_kickoff`, begin-only today) | table entry ✓ |

## 6. Minimal rec for T3 (bounded, ranked)

**R21-1 — the tape's hold release (unblocks T3's gate; no periphery work).**
Land FU-165 §7 R1 exactly: (a) `SETUP0` calls `set_piece(mr, 0xC, 0, 0)` after
the `global_5882a=0` write; (b) a derived act-9 producer (phase 0x15 +
`global_5882a=1` at the native-shaped gate) — FU-165 L1/L2 bound the waits;
(c) `SETUP2` calls `set_piece(mr, 3, 0, 0)`. Everything downstream is wired
(taker rows 0x10..0x13, sit 0xB → phase 2, PHASE/POST). Add the phase arms the
derived route touches (0x15 = install 3 over 0..10, `0x8D77B`; 0xB = install 0,
`0x8D63E`) only if the route writes those phases with arms enabled — the
engine's `phase_arm` currently no-ops them (`:2395-2399`). Pin the scheduler's
rollover arm (ids 8/7) as the natural-escape regression.

**R21-2 — the live byte-0 natural continuation (the "not just the tape"
piece).** Three small landings make the engine's settled live session
continue past a natural goal:
1. store the scan's goal record: replace the `(void)match_run_goal_nearest`
   discard (`:2757`) with a new run field (the `[0x157A9F]` cell; record id +
   team), written by `match_run_goal_scan` on both the throw-in/corner arms
   and the goal arm (the native scanner writes it in all four arms);
2. add `phase_arm(5)` = the celebration install (first-hand `0x8D433`):
   per team, if `team->side == goal_record_team`: install 0x15 over records
   1..10 via `fifa96_match_arm_install_multi(pool, t, 1, 10, 0x15, -1)` and
   install 0x14 on the goal record (`fifa96_match_entities_install(rec, 5,
   0x14, 0)`), else install 0 over 0..10 (`(0,10,0,-1)`);
3. the release: wire row 0x14's stage-2 write — at the derived end of the
   celebration, `match_run_write_phase(mr, 1)` **plus** the kickoff arm and
   placement commit the engine already runs at begin (`fifa96_match_phase_
   machine_kickoff` + the `:2186-2200` block; factor a shared
   `match_run_kickoff_entry`). Row 0x14's stages 0/1 (timers/RNG script,
   FU-82 §3.5) may land as the bounded remainder; if the celebration timing
   is too big for T3, land (1)+(2)+the `0x87C71` write behind the existing
   partial scatter port and number the stage bodies (FU173-L3).
Ranking: R21-1 is required for T3's tape extension; R21-2 is what makes the
*live* match "continue past a goal" (and what extends
`test_natural_goal_fallback_arm` past its current phase-5 assertion).

## 7. Numbered legs (FU-173)

1. **FU173-L1 — `[0x157A9F]` producer order.** Of the 18 xrefs, four scanner
   writes (`0x88955`/`0x889e0`/`0x88ab3`/`0x88ae3`) and the reset
   (`0x73E66`); which write is the final pre-goal selection on each scanner
   arm (throw-in/corner/goal) is not derived here. Derive before wiring the
   cell.
2. **FU173-L2 — row 0x15 (`0x87CD0`).** The celebration gate body's stages
   (`0x87CC0` table, `CALL [rec+0x1C]` re-entry) and whether omitting it
   changes the release timing (the release lives in row 0x14; row 0x15 may be
   observer-only).
3. **FU173-L3 — row 0x14 stage machines.** FU-82 §3.5 stages 0/1 (the
   `+0x89` waits, the 8-RNG script, `FUN_00045001` early advance, the
   `0x58720` arm index) — FU-136 row 14's "stage 2 and arm timing unported"
   (OL-9). The release needs stage 2 only; the timers decide when.
4. **FU173-L4 — the phase-reset equivalence.** `FUN_00073E28` (called by the
   release at `0x87C4E`) is modeled at begin only; whether a mid-match
   invocation is needed (and what it resets that the engine carries) is not
   derived.
5. **FU173-L5 — the scheduler rollover during the hold.** No native run
   measures whether `[0x15B688] > [0x15B694]` (period seed 900 frames for
   leg 0/mode 0) can fire inside the 0xB4 hold wait or the act-9 gates; the
   arms are byte-exact but the timing is unmeasured (extends FU165-L7).
6. **FU173-L6 — the tape's extended frame count.** Cross-ref FU165-L7: the
   T3 v14 extension measures ≥91 granted frames + the taker cascade; no
   native frame count exists.
7. **FU173-L7 — phase-arm coverage enumeration.** The native table has arms
   for phases 0/1/2/3/4/5/6/7/8/9/0xA/0xB/0xC/0xD/0xE/0xF/0x10/0x11/0x12/
   0x13/0x14/0x15 (`0x8D040`, first-hand); the engine wires
   3/4/6/7/8/9/0xD (`phase_arm`) + 0x13/0x14 and 1 (phase machine). Wire only
   the arms the restart/continuation chains keep (folds FU-165 L8; the
   missing arms are honest no-ops today).

## 8. Discrepancies / errata candidates (prior docs not rewritten)

1. **FU-146 §2 condition wording.** The ids 3/4/9/7 arms' key is the **leg
   `[0x15B680]`** (`0x948E5`/`0x9492A`/`0x9497D`/`0x949B7`), not "period".
   The engine is correct; the doc's "period==2/5" column is wrong.
2. **FU-149 §1.5's table.** It omits the phase-5 row (`0x8D433`, first-hand
   `0x8D040` read) and describes the phase-0xD arm as "install **1** over
   records 1..10" — first-hand `0x8D65D..0x8D681`: `first=1`, `code=0`
   (`XOR ECX,ECX` at `0x8D67F`); the "1" is the first index, not the code.
   The engine's `phase_arm(0xD)` is correct.
3. **The plan's R21 shorthand** ("the goal queue ids 5/6") is byte-1-only
   routing: under the engine's settled byte-0 default these ids never queue
   (FU-166); T3's scope should name which session the continuation models.
4. **FU-146 §2 xref sanity**: fresh counts identical (2/2/1/6/7/39) — no
   errata.

## 9. Risks

* **T3 scope fork.** Wiring the byte-1 release extends the tape (v14) but not
  the live session; wiring the byte-0 release continues the live match but
  moves no tape frame. Doing both is two slices; doing one must be named.
* **Double scoring** stays gated by the session byte (`:2643`); wiring a
  queue path with the byte-0 default would double-score the natural goal —
  the existing fixtures pin the single-writer routing.
* **`[0x157A9F]` identity.** The phase-5 arm installs on the goal record's
  team; the engine's scan currently derives that record from the snapshot
  sign + nearest — the same stand-in the corner/throw-in arms already use
  (FU-145 L4), so the arm inherits the same derivation risk.
* **Cadence.** Everything ticks once per granted 30 Hz frame; the phase-5
  celebration and the 0xB4 hold both consume `word[0x157A64]` per frame.
* **Doc churn.** §8's errata touch frozen FU-146/FU-149 text; land them as
  quoted errata in the implementing slice, not by rewriting.

## 10. Method record (tool calls, read-only)

`disassemble_bytes`: `0x948AC..0x949F7` (83), `0x4B190..0x4B1B0`,
`0x8A9E0..0x8AA30`, `0x92D8C..0x92F00` (98), `0x936D2..0x93710`,
`0x93C24..0x93D08`, `0x93DCA..0x93DE3`, `0x8D433..0x8D4A2`,
`0x8D098..0x8D0E0`, `0x8D65D..0x8D6A0`, `0x87C48..0x87C90`.
`read_memory`: `0x110F78` (24 B), `0x8D040` (92 B), `0x110794` (24 B),
`0x1106E0` (88 B).
`get_xrefs_to` (fresh): `0x948AC` 2, `0x92D8C` 2, `0x92E2C` 1, `0x935A0` 6,
`0x15B6D4` 7, `0x8A938` 39, `0x157A9F` 18.
Repo reads (read-only): `src/fifa96_engine/fifa96_match_run.c`
(`:2301-2311, :2395-2399, :2623-2631, :2643-2666, :2705-2766, :2757,
:2864-2997, :2999-3131, :3441-3586, :3544-3552`),
`include/fifa96_engine/fifa96_match_run.h` (`:225, :611, :706-729`),
`include/fifa96_engine/fifa96_match_phase_machine.h` (`:74-114`),
`tests/test_engine_match_frame.c` (`:4549-4614`, `:4458-4547`),
`docs/ENGINE.md:262`.
No writes: no rename/comment/label/function/script/project save. Repo: this
slice + `r21-report.md` only.
