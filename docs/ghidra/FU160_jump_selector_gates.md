# FU-160 — the live jump row-selector gates (R8, phase-13 recon)

**Question (phase-13 plan R8).** Complete the live jump row-selector picture
left open by FU-153 §10-11 / FU-159 §3:

1. Derive exactly what the two **live installers of rows `0x5E`/`0x12`** need
   to fire (row-`0x0C` body `0x81DA4`, row-`0x0E` body `0x827D0`): the
   `[0x7CF]` counter's producer, `rec[+0x20]`, `timer89`, the `0x79B58` call
   chain — and whether each is reachable in the engine's wired subset.
2. Deeper search for the **unfound installers of rows `0x14`/`0x5C`**
   (computed stores, table-driven selectors, the row-`0x0C`/`0x0E` tails,
   frame-record `next_id` variants); bound it if still unfound.
3. Ranked wiring recommendation for T3: which gate to port first for a live
   selector, which stays leg.

All Ghidra reads in this slice are fresh, program **`/FIFA96.EXE`**
(case-sensitive), flat LE-image addresses, standard MCP tools only; the GO
program reports one overlay space (`.image`, unused below — all cites are the
`ram` space). Byte cites are from fresh `disassemble_bytes` /
`decompile_function` unless a site explicitly references a prior doc.

**Headline results.**

* The row-`0x0C` `%7`/`+0x20` gate controls **only the event `0xA2`**, not the
  row-`0x5E` install (quoted erratum to FU-159 §3.3 in §1.3). The install
  needs a narrow state: `[0x15888F]` (contact record A) in anim row `0x59`
  frame `>= 2`, polar magnitude `>= 0x30` between `EBP` and A's lead point,
  and `EBP` stage 0 with `+0x8D != 0`. The install itself uses **frame
  argument 0** (the `ECX = byte[+0x8E]` loads at both sites are dead).
* The row-`0x12` install fires from the row-`0x0E` body **stage 1** once
  `timer89 >= 0x0C`; the `0x79B58` call on the way is the record's own
  receiver timer (`+0x99 == 0 -> +0x93 = 0x10`), already modeled by the
  engine.
* Engine reachability: **row `0x5E` not reachable** (row `0x0C` dispatch NULL;
  the ported foul branch `fifa96_match_run_contact` is test-only); **row `0x12`
  needs only the row-`0x0E` stage 0/1 port** — the install arm (row `0x23` ->
  install `0x0E`) is already wired (`fifa96_match_handlers.c:2203`). This is
  the ranked #1 T3 target.
* Rows `0x14`/`0x5C`: the deeper search is **negative and now bounded** (§3):
  no immediate anywhere in the image, no `next_id`/mirror/table hit across all
  111 anim rows and all 84 installer sites' resolved id sources, and the
  installer's own head references `0x5C` only as a keeper-flag id
  (`0x6E5F2`). Three residual dynamic sites keep a numbered leg; none is tied
  to `0x14`/`0x5C` by any static evidence.

---

## 1. T1 — the row-`0x5E` installer (row-`0x0C` stage-0 arm)

### 1.1 The arm (fresh disassembly, `0x81CDC..0x81DC5`)

The row-`0x0C` body (action table entry 12, `0x81C90`; FU-137 §6 entry) is
entered with `EBP = rec` (the stage-family ABI, FU-81 §2) and dispatches stage
`+0x92` through the arm table `0x81C74` (stage 0 = `0x81CDC`). The stage-0 arm,
reconstructed exactly:

```
0x81CDC  CMP byte[EBP+0x8D], 0
0x81CE3  JZ 0x824FB                      ; index 0 (keeper) -> tail/reset path
0x81CE9  MOV ECX, [0x15888F]             ; A = the contact record (FU-150 E2)
0x81CEF  TEST ECX,ECX; 0x81CF1 JZ 0x81DCF
0x81CF7  MOV EDX,[ECX+0x28]              ; A's row pointer
0x81CFA  MOV EAX,ECX
0x81CFC  MOV DL, byte[EDX]; 0x81CFE AND EDX,0xFF
0x81D04  CMP EDX, 0x59
0x81D07  JNZ 0x81DCF                     ; A not in row 0x59 -> alternate gate
0x81D0D  MOV EDX,[ECX+0x3A]; 0x81D10 SAR EDX,0x18   ; byte[A+0x3D] = frame
0x81D13  CMP EDX, 2;  0x81D16 JL 0x82502 ; frame < 2 -> tail
0x81D1C  EDI=ESP; ESI=&ECX[+0x59]; MOVSD x3          ; stack := A.pos (x,y,z)
0x81D24  EDX=word[A+0x71]; SAR 16; EDX+=EDX; [ESP]+=EDX   ; lead.x = A.x + 2*word[A+0x71]
0x81D34  EAX=word[A+0x73]; SAR 16; ... [ESP+8] = A.z + 2*word[A+0x73]
0x81D48  LEA EAX,[EBP+0x59]              ; arg1 = EBP.pos
0x81D4F  CALL 0x795B4                    ; arg2 = the stack lead point, arg3 = ESP+0xC
0x81D54  MOV EAX, dword[ESP+0xA]; SAR EAX,0x10       ; = word at ESP+0xC = param_3[0]
0x81D5B  CMP EAX, 0x30
0x81D5E  JL 0x81DFE                      ; magnitude < 0x30 -> the FOUL path
        --- magnitude >= 0x30: the stumble ---
0x81D64  MOV EAX,[0x15888F]; MOV EAX,dword[EAX]      ; team = [A]
0x81D6B  INC dword[team+0x7CF]                        ; ++the team counter
0x81D71  MOV EDX,[0x15888F]; MOV EDX,[EDX]; MOV EAX,[EDX+0x7CF]
0x81D7F  MOV EDX,EAX; MOV EBX,7; SAR EDX,0x1F; IDIV EBX
0x81D8B  TEST EDX,EDX; 0x81D8D JNZ 0x81DA4           ; counter%7 != 0 -> skip event
0x81D8F  CMP dword[EBP+0x20],0; 0x81D93 JZ 0x81DA4   ; no slot -> skip event
0x81D95  MOV EBX,4; 0x81D9A MOV EAX,0xA2
0x81D9F  CALL 0x8F188                    ; event 0xA2 (EDX = 0: the IDIV remainder)
        --- the install (both event paths fall here) ---
0x81DA4  MOV EAX,[0x15888F]              ; rec = A
0x81DA9  MOV EDX, 0x5E                   ; id
0x81DAE  MOV ECX,[EAX+0x8B]; 0x81DB6 SAR ECX,0x18    ; dead load (see §1.4)
0x81DB4  XOR EBX,EBX                     ; frame argument = 0
0x81DB9  CALL 0x6E598                    ; install row 0x5E on A. bytes e8dac7feff
0x81DBE  MOV EAX,EBP
0x81DC0  CALL 0x7DAB4                    ; reset(rec) -- the action record, not A
0x81DC5  epilogue RET (0x81DC8..0x81DCE)
```

The alternate gate at `0x81DCF` re-reads A's row and continues to `0x81DFE`
(the foul path) when A is in row `0x11` frame `>= 2`, or in any other row with
frame `>= 3`; otherwise it tails to `0x82502`. The foul path itself
(`0x81DFE..0x81F0B`, fresh) is the FU-150 re-call contract the engine already
ports: EBP's facing-relative `+0x73`/`+0x75` words from the `0x114E04` fold
(`0x81E13..0x81E86`), the settings `[0x14C306]`/`[0x15888F]`/`EBP ==
[0x158893]` gates, the `0x92AC8` 1-in-8 skip (`0x81EA6`), `0x8A43C(kind 1, A,
B)` (`0x81EBF`), `[0x15888E] = 1/0`, `byte[EBP+0x98] = 1`, `0x92AC8 & 3 ->
install row 0xD or 0x52 on EBP` (`0x81EEC..0x81F01`) and whistle
`0x974DC(0x1F)` (`0x81F06`).

**The magnitude word.** `FUN_000795B4(a, b, out)` (fresh decompile) stores
`out[1] = b[0]-a[0]` (dx), `out[2] = b[4]-a[4]` (dz), `out[0] = FUN_000CD514()`
(fresh decompile: `i = FUN_000CD474()` = the angle; folded to `<= 0x100`;
then `(|dx| << 16) / FUN_000CE386()` when `i < 0x81` else
`(|dz| << 16) / FUN_000CE364()`; both CE386/CE364 are fresh-decompiled as
**table lookups into the `0x114E04` quadrant sine table** — the same table the
jump arm uses). The quotient is therefore the polar magnitude (vector length)
of (A.pos+2A.vel) - EBP.pos in whole word units; `< 0x30` sends the arm to the
foul path, `>= 0x30` to the counter/event/install block. (The length-unit
reading is derived from the table chain; the edge behavior at exactly `0x30`
is leg FU160-L3.)

### 1.2 The `[0x7CF]` counter's producer (fresh full-program sweep)

`search_instructions` operand `0x7cf]` over the whole program finds **exactly
three** accesses:

| site | op | role |
|---|---|---|
| `0x81D6B` | `INC dword[EAX+0x7CF]` (bytes `ff80cf070000`) | the arm itself, on the stumble path |
| `0x81D79` | `MOV EAX, dword[EDX+0x7CF]` (`8b82cf070000`) | the arm itself, re-read for the `%7` test |
| `0x8C2E3` | `MOV dword[EAX+0x7CF], 0` (`c780cf07000000000000`) | reset in `FUN_0008C2E0` (bytes verified in the fresh `disassemble_function`) |

`FUN_0008C2E0` is the **team initializer**: it zeroes `team+0x7CF`, sets
`team+0x826 = side`, `team+0x7A6 = 0x1588A4 + (side^1)*0x835` (the opponent
block), then loops `i = 0..10` writing each embedded `0xB2`-byte record's
`+0x8D = i` (`MOV byte ptr [EAX + -0x25], DL` at `0x8C329`, base-relative to
the `0xB2` stride) and `[rec] = team` (`MOV [EAX-0xB2], EBX`). It has exactly
two callers, both in `FUN_00073D90` (`0x73DA7`/`0x73DB6`, fresh
`get_xrefs_to`). So the counter has **no external producer**: it is produced
and consumed inside the stumble arm; the only other touch is the per-team
reset. Its semantic is "the team's stumble counter"; the event `0xA2` fires on
every 7th stumble (`counter % 7 == 0` after the increment).

### 1.3 Erratum — FU-159 §3.3's row-`0x5E` gate (quoted)

Quoted from FU-159 §3.3:

> **Row `0x5E` <- the row-`0x0C` body** (`0x81C90..0x8251B`, FU-137 table
> entry 12, "not ported (partial)") at `0x81DA4..0x81DB9`: `0x81D81 IDIV
> EBX(7); TEST EDX,EDX; JNZ` — the gate is `(team[0x7CF] % 7 == 0)` (the dword
> at `[0x15888F]`->team, `[+0x7CF]` incremented at `0x81D6B`) **and**
> `[EBP+0x20] != 0` (the record carries a slot block); then
> `FUN_0008F188(0xA2, rec, 4)` (`0x81D9A`) and `FUN_0006E598(rec=[0x15888F],
> 0x5E, ECX=byte[+0x8E])` (`0x81DB9`, bytes `e8dac7feff`)

**Corrections (fresh `0x81D40..0x81DE0`).**

1. The `%7`/`+0x20` gate jumps to `0x81DA4` — the **install** address, not a
   skip past it (`0x81D8D JNZ 0x81DA4`, `0x81D93 JZ 0x81DA4`). Both gates
   condition **only the event `0xA2`**; the row-`0x5E` install is
   unconditional on that path.
2. The real install preconditions are the state gates above: `[EBP+0x8D] != 0`,
   `[0x15888F]` in row `0x59` frame `>= 2`, and the polar magnitude `>= 0x30`
   (if `< 0x30`, the arm goes to the foul path and never reaches `0x81DA4`).
3. The `+0x7CF` counter lives on **A's team** (`team = [[0x15888F]]`), not on
   the action record's.
4. Event `0xA2` reaches `FUN_0008F188` with `EDX = 0` (the `IDIV` remainder at
   the gate is exactly 0 there) and `EBX = 4`; the sink stays a leg
   (FU-159-L3 family).
5. The `0x5E` install's third argument is **EBX = 0** (frame 0); the
   `ECX = byte[+0x8E]` load is dead — see §1.4.

### 1.4 Erratum — the install frame argument is `EBX`, not `ECX`

Fresh `disassemble_bytes 0x6E598..0x6E660`: the installer's head does
`MOV ESI,EAX` (`0x6E59A`) and then **overwrites ECX unconditionally** at
`0x6E5B3 MOV ECX,EAX` (the `SETG AL` from the `[0x157754]` sign compare). The
decompile (`param_1, param_2 = DX, param_3`) shows the only use of the third
argument is `FUN_0006E490(param_1, param_1+10, param_3)` — and the frame
resolver's frame argument is `EBX` (fresh jump-arm call `0x6EE80..0x6EE86`:
`MOV EBX,3; ...; CALL 0x6E490`). Both installer sites load `EBX = 0` before
the call (`0x81DB4`/`0x827DD`), so rows `0x5E`/`0x12` install at **frame 0**
and the `ECX = [rec+0x8B] >> 24` loads at `0x81DAE`/`0x827D7` are dead
(consistent with FU-142 App. I's "`0x6E598 reserved45`" remark). This also
matches the T1 test stand-in (`rec->anim_id = 0x5E; rec->frame = 0`,
`tests/test_engine_match_frame.c:2133`).

**Installer head bonus (fresh)**: the head also gates
`byte[rec+0x8D] == 0 && ([0x157754] > 0) == team side` →
`byte[0x157A6C] = (id in {0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C})`
(`0x6E59C..0x6E602`, bytes verified; `0x6E5F2 CMP EAX,0x5c`). Row `0x5C` is
referenced as a **keeper/fall anim id** by the installer itself.

### 1.5 Engine reachability (row `0x5E`)

**Not reachable in the engine's wired subset.**

* Row `0x0C` dispatch is `{0x0C, NULL, "not ported (partial); FU-81 7-arm
  table 0x81C74; OL-9"}` (`fifa96_match_handlers.c:2851`).
* The engine's direct port of the row-`0x0C` **foul** branch
  (`fifa96_match_run_contact`, `fifa96_match_run.c:4805`) has **no live
  caller** — fresh `grep`: only `tests/test_engine_match_frame.c` calls it.
  The engine never stages an install of action `0x0C` either (`grep` over
  `src/` for `0x0C` installs returns only the two dispatch-table NULL rows).
* The stumble branch (counter, event, row-`0x5E` install, reset) is not
  ported at all.
* What the engine *does* already have: `r->has_slot = e->has_slot` (the
  native `+0x20` slot pointer gate), `r->row44`, the pool entity staging, and
  the `0x8DCD4` distance helper (`fifa96_arm_dist_stage`) — but not the
  `0x795B4`/`0xCD514` magnitude, the `0x114E04`-table sine lookups, nor the
  `[0x7CF]` counter. The record-index gate `+0x8D != 0` maps to the engine's
  existing `e->index != 0u` convention (`fifa96_match_run.c:1199` — record 0
  is the keeper machine's own record, comment there).

---

## 2. T1 — the row-`0x12` installer (row-`0x0E` body stages 0/1)

### 2.1 The body head and tick (fresh `0x82710..0x82742`)

```
0x82710  PUSH EBX/ECX/EDX/ESI/EDI/EBP; SUB ESP,0x14
0x82719  MOV EBP,EAX                     ; rec
0x8271B  MOV EAX,[0x157A4A]; SAR EAX,0x18
0x82723  CMP EAX,2; 0x82726 JNZ 0x82ABE  ; phase != 2 -> RESET (0x7DAB4)
0x8272c  XOR EAX,EAX
0x8272e  MOV EDX,[EBP+0x89]              ; timer89
0x82734  MOV AX,[0x157A64]               ; delta word
0x8273a  ADD EDX,EAX
0x82742  MOV [EBP+0x89],EDX              ; timer89 += delta (zero-extended)
0x82748  CMP AL,3; 0x8274A JA 0x82AC5    ; stage > 3 -> tail
0x82755  JMP CS:[EAX*4+0x82700]          ; stage table 0x82700 -> {0x8275D,0x827B1,0x82806,0x82AB8}
```

### 2.2 Stage 0 (`0x8275D..0x827AB`) and stage 1 (`0x827B1..0x827ED`)

```
--- stage 0 ---
0x8275D  CMP byte[EBP+0x8D],0; 0x82764 JZ 0x82ABE   ; index 0 -> RESET
0x8276A  LEA ECX,[EBP+0x4D]; MOV EAX,0x12; MOV EDX,ECX; LEA EBX,[EBP+0x65]
0x82777  CALL 0x71B9C                    ; FUN_00071B9C(0x12, &+0x4D, &+0x65)
0x8277C  LEA EAX,[EBP+0x59]; MOV EDX,ECX; CALL 0x8DCD4   ; dist/face staging
0x82786  EAX=EBP; EDX=word[+0x65]; EBX=word[+0x67]; CALL 0x79C50   ; face
0x82799  MOV DL,[EBP+0x92]; MOV dword[EBP+0x89],0     ; timer89 = 0
0x827A9  INC DL; MOV [EBP+0x92],DL       ; stage 0 -> 1 (falls through)
--- stage 1 (runs every frame while waiting) ---
0x827B1  MOV ESI,0x157794; LEA EDI,[EBP+0x4D]; MOVSD x3
         ; target triple := [0x157794]
0x827B9  MOV EAX,EBP
0x827BE  CALL 0x79B58                    ; the receiver timer on SELF
0x827C3  CMP dword[EBP+0x89],0xC
0x827CA  JL 0x82AC5                      ; timer89 < 0x0C -> tail (wait)
0x827D0  MOV EDX,0x12
0x827D5  MOV EAX,EBP                     ; install on the action record itself
0x827D7  MOV ECX,[EBP+0x8B]; 0x827DF SAR ECX,0x18   ; dead load (§1.4)
0x827DD  XOR EBX,EBX                     ; frame = 0
0x827E2  CALL 0x6E598                    ; install row 0x12. bytes e8b1bdfeff
0x827E7  MOV byte[EBP+0x9E],0x1          ; ran = 1
0x827EE  MOV DH,[EBP+0x92]; MOV [EBP+0x89],0; INC DH; MOV [EBP+0x92],DH
         ; stage 1 -> 2
0x82806  CMP EBP,[0x1577CA]; 0x8280C JZ 0x82AA0   ; tracked branch (stage 2 arm)
```

So the row-`0x12` install fires **once per entry into stage 1**, after
`timer89` has accumulated `>= 0x0C` (12 frames at delta 1) while the target
triple is refreshed from `0x157794` and the self receiver timer is re-armed
every waiting frame.

### 2.3 The `0x79B58` call chain (fresh)

Fresh `decompile_function 0x79B58`: `void FUN_00079B58(rec) { if
(byte[rec+0x99] == 0) byte[rec+0x93] = 0x10; }`. Fresh `get_xrefs_to
0x79B58` = **17 call sites**: `0x7CC7D` (`FUN_0007CA54` input row), `0x7CF48`,
`0x7F657` (row 05), `0x80B4C` (row 09), **`0x827BE` (row 0E, our site)**,
`0x74823`/`0x74B63` (keeper), `0x7E1C4`/`0x7E215` (row 02), `0x7EBF3`/`0x7EDBF`
(row 04), `0x80646` (row 06), `0x81162` (row 08), `0x8172C`/`0x817A3` (row 07),
`0x83057` (row 23), `0x8538D` (row 21). The engine already models exactly this
effect as `e->timer93 = 0x10` under the `+0x99 == 0` gate (staged 0):
`fifa96_match_handlers.c:2460` (kick), `:2704` (row 0F), `:2812` (receive arm),
`match_run.c:716` (row 08). In row `0x0E` the call is on **EBP itself** (self),
not on a receiver record.

Note the chain's remaining pieces are already ported or derivable:
`0x71B9C` writes the target triple from the `0x15774C` focus plus per-kind
offsets (the engine's row-23 port already uses the camera target as its
`0x71B9C` stand-in, `fifa96_match_handlers.c:297`); `0x8DCD4` is
`fifa96_arm_dist_stage`; `0x79C50` is `fifa96_arm_face`; the
`0x157794` triple is the FU-141 frame's second selection vector
(`fifa96_match_entities.c:549`); `0x157A64` delta is `r->delta` (staged per
entity, `match_run.c:1094`).

### 2.4 Engine reachability (row `0x12`)

**Not live today, but one small port away.**

* The **install arm is already wired**: the row-23 tackle port emits
  `out.install_0e != 0 -> mr->record.install = 0x0E`
  (`fifa96_match_handlers.c:2203`; FU-139 §10's "installs 0x0E/0x0F").
* Row `0x0E` dispatch is `{0x0E, NULL, "not ported; FU-81 gate/head, no body
  port; OL-9"}` (`fifa96_match_handlers.c:2857`) — the installed entity's
  code is drained into the pool (`e->code = 0x0E`) and the dispatch returns
  `-FIFA96_ERR_UNSUPPORTED`, so nothing runs.
* Everything the stage 0/1 port needs exists in the engine: the phase gate
  (`mr->state.phase`), `timer89`/`delta`, the record-index gate
  (`e->index != 0`, the same convention the outfield machine uses at
  `match_run.c:1199`), the face/metric helpers, the `timer93` receiver effect,
  the `ran` write-back, and the pool anim staging (`e->anim_id`/`e->frame`).
* Porting the two arms (≈35 instructions of body plus the two calls) and
  wiring row `0x0E` would make the **live jump selector**: row-23 tackle ->
  install `0x0E` -> stage 0/1 -> install row `0x12` (frame 0) -> the already
  ported jump arm at frame 2 (aux `0x0E`, `pos_y = 0x10`,
  `jump_base/timer = ramp(0x10) = 15`) -> the ported `0x7BF20` arc.

### 2.5 Census / goldens impact (for the T3 implementer)

Wiring row `0x0E` flips one dispatch row (58 UNSUP -> 57, 21 -> 22 wired).
Per AGENTS.md the README title must be recomputed **in the same commit**:
`floor(100 * 22/80) = 27%` (the ENGINE.md census line updated with it). M1
never moves. M2 must be re-checked byte-exact; if a live tackle install of
`0x0E` occurs on the tape path the golden re-pins only with the frame diff
recorded.

---

## 3. T1 — rows `0x14`/`0x5C`: the bounded negative

### 3.1 Method of the deeper search

1. **Byte immediates** (fresh `search_byte_patterns`): `ba 5c 00 00 00` = **0
   matches** (reproduces FU-159); `ba 14 00 00 00` = 23 ram matches — every
   site is outside the installer-call address region (`0x7159C..0x89960`); the
   nearest (`0x7B10B`, `0x8D461`, `0x9E691`) is >0x400 bytes from any of the
   84 call sites. `b2 5c` (MOV DL,0x5C) = 3 ram matches (`0xAEB5D`,
   `0xB73E4`, `0xCAB79`) — **all beyond the last installer call site**; `b2 14`
   = 12 ram matches, all below the first installer call site. No
   `MOV EDX/EDX/DL/DX` for `0x5C` can feed any `FUN_0006E598` call.
2. **The row table** (fresh `read_memory 0x10EF00`, 999 bytes = 111 entries
   `0x00..0x6E`, ids verified consistent): **no `next_id` equals
   `0x14`/`0x5C`/`0x84`/`0xCC`** (the mirror sources), matching FU-159's
   ported-table scan. Row `0x14` = `{last 6, flags 00, next 0x7F, tab
   0x10E4BC, bank 0x13}`; row `0x5C` = `{last 8, flags 00, next 0x27, tab
   0x10EDBE, bank 0x47}`.
3. **The successor tails** (`FUN_0006E8E8` `0x6FCE5..0x6FE69`, fresh; the
   pool twin shares the ids): installs are `next_id` (`0x6FE41`), `next_id -
   0x70` after the `0x79C50` mirror (`0x6FE18`), keep-current when
   `next_id == 0x6F` (`0x6FDB3`), constant `0x5A` (`0x6FCFE`), and the
   movement-table byte (`0x6FD36`). The movement tables are the four
   `FUN_0006E330` bases (`0x6E330` fresh decompile), read first-hand:
   `0x11052A` ids `{0x00..0x03}` (16 entries), `0x11058A` `{0x28,0x2B,0x2A}`,
   `0x1105AA` `{0x26,0x29,0x2A}`, `0x1105CA` `{0x27,0x43}` — **no `0x14`/
   `0x5C`**. (The row-27 pair table `0x1103CB` ids `{0x6B,0x68,3,0x16,0x15,
   0x56,0x67,0x50,0x1A,0x19}`; the event table `0x10E17D` bytes `<= 0x3A`.)
4. **All 84 direct sites** (fresh `get_xrefs_to 0x6E598` = 84): FU-159's
   census covers the immediate sites; this slice resolved the remainder —
   `0x71611`/`0x716FB` = `0x5D` (5+2 `MOV EDX,0x5D` sites in `FUN_0007131C`);
   `0x88A35` = `0x5D`, `0x88A9D` = `0x55`; `0x89923` = `0x26`, `0x89960` =
   `0`; `0x7AA2B` = `byte[0x158741]` (see legs); `0x75CF7` = an event/sector
   code (FU-151 §`0x75CF7`); `0x779A7` = `0x3D`/`0x3E` (`0x77988 ADD
   EAX,0x3D`); `0x819`-range: `0x818CB` = `word[ESP+0x8]` set to **`0x17` or
   `0x18`** (`0x817EE MOV ECX,0x17` / `0x81800 MOV EBX,0x18`), `0x81AFB` =
   `word[ESP+0x14]` (FU-82 §3.3's choice `{0x0C,0x59}`); the `FUN_0006E8E8`
   internal arms `0x6EF58` = `{0x5B,0x5F}` (frame EBX=2), `0x6F175` = `0x46`,
   `0x6F6A3` = `0x53`, `0x6F747` = `0x0F`, `0x6F773` = `0x69`; the celebration
   cluster `0x87F12` = `1`, `0x87FCE`/`0x885DA` = `byte[0x1587BB]`,
   `0x88240` = `{0x15,3}`, `0x8845A` = `byte[0x1587B8 + [0x158720]]`,
   `0x88675` = `byte[0x1587BC]` — all bounded by `FUN_0008776C` (fresh
   decompile + `read_memory 0x10F344`: the celebration tables give
   `{0x15,0x03,0x19,0x16,0x1A,0x67,0x58,0x5B,0x6B,0x56,0x50,0x68}`);
   row `0x2A`/`0x28`/`0x29`/`0x2C` sites = the C tables
   (`fifa96_arm_bodies.c`: `{0x60,0x61}`, `{0x15,0x19,3,2}`, `{0,0x58,0x5B,
   0x6B}`, `{0x5D,0x46}`); `0x869F8` = the pair table; `0x7EE3C`/`0x7F5C2` =
   the C constants (`{1}`, `{6,0x30}`).
5. **The installer's own id domain** (fresh head): the head's special-id set
   `{0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C}` (`0x6E5CC..0x6E5F5`) is the
   only reference to `0x5C` anywhere in the image besides row `0x5C`'s own
   table entry — evidence the id was known to the engine (keeper flag
   `[0x157A6C]`), with no caller in this build.

### 3.2 Residual dynamic sites (kept as legs)

| site | id source | bound |
|---|---|---|
| `0x7756E` (row `0x1B`, keeper reposition B) | `word[ESP+0x10] + 1` (`0x77547..0x77554 INC`, re-read via `0x7755C`) | stack-local base not resolved in this slice; the row-`0x1B` body is unported (FU-140 OL-34) |
| `0x7AA2B` (row-05 tail, `FUN_0007A490`) | `byte[0x158741]` | **no static writer** (`search_instructions 158741` = 3 reads only) — FU-159's ball `sub_code` stand-in territory |
| `0x76863` (keeper reposition A) | `word[0x157C2A]` | **no static writer** (`157c2a` = 1 read only); FU-79 §`0x76E3B` ties the twin site to the `0x10E17D` event table (bytes `<= 0x3A`) |

None of the three can be reached from any *static* id feed containing
`0x14`/`0x5C`; their live domains need a runtime trace (legs FU160-L1/L2).

### 3.3 Conclusion

**Rows `0x14`/`0x5C` have no evidenced installer in this image.** The negative
is now bounded by: (a) immediates across all plausible encodings and all
address neighborhoods, (b) the full 111-row `next_id` column and its mirror
form, (c) the successor tails' five id sources with the four movement tables
read first-hand, and (d) the resolved id source of every one of the 84
installer call sites (three residual dynamic sites excepted, none tied to the
rows). Structurally `0x14` is the mirror-shaped sibling of `0x12` (same
`next -> 0x0F`, aux `0x0F` on frame 0) and `0x5C` the keeper-side sibling of
`0x5E` (installer keeper-flag id; bank `0x47` next to row `0x5D`'s `0x48`) —
plausible dev-time variants that no shipped code path installs. Keep as leg
FU159-L2, carried with the new bounds; do not model.

---

## 4. T3 wiring recommendation (ranked)

**#1 — port the row-`0x0E` body stages 0/1 and wire row `0x0E` (the live
row-`0x12` selector).** Cheapest live selector by a wide margin: the install
arm already exists (row `0x23` -> `install = 0x0E`), every input is staged or
trivially derivable, and the jump consumer is already ported. Concrete shape:

* new `fifa96_match_action_0E` in `src/fifa96_engine/fifa96_match_handlers.c`:
  phase `!= 2 -> match_row_reset`; `timer89 += delta`; stage 0: `index != 0`
  gate (else reset), `0x71B9C` stand-in (the row-23 camera-target convention),
  `fifa96_arm_dist_stage` + `fifa96_arm_face`, `timer89 = 0`, stage -> 1;
  stage 1: target := the staged `0x157794` selection vector, `e->timer93 =
  0x10` (the `0x79B58` self call, byte99 staged 0), wait `timer89 >= 0x0C`,
  then install row `0x12` on self via the new install helper
  (FU-159 §5.4's `record_anim_install`; frame 0 — §1.4 here), `ran = 1`,
  stage -> 2. Stages 2/3 (the `[0x1577CA]` tracked arm) stay a leg.
* wire `{0x0E, fifa96_match_action_0E}` in `fifa96_match_action_table`.
* tests (RED first): unit — stage install `0x0E` on a pool record, drive 12
  delta frames, assert `anim_id == 0x12`, `frame == 0`, `ran == 1`, phase gate
  reset, keeper-index reset; ISO — extend the T1 chain with a live
  tackle-install entry and assert the jump arm fires at row-`0x12` frame 2.
* census: wired 21 -> 22; README `floor(100*22/80) = 27%` in the same commit;
  M2 re-check (re-pin only with a diff).

**#2 — port the row-`0x0C` stumble branch for row `0x5E` (stays a hop unless
a live contact entry is wired).** The port itself is straightforward once the
pieces above exist: the `0x795B4`/`0xCD514` magnitude helper (port of the
`0x114E04`-table lookups is small; the `0x114E04` fold is already ported in
the arm bodies), the per-team `[0x7CF]` counter (new pool/team field, reset in
the team init mirror), the event `0xA2` request bit, and the install of row
`0x5E` on `rec_a` (the `fifa96_match_run_contact` argument is already the
contact record A). But it must go **after** a live caller of
`fifa96_match_run_contact` is wired — today that entry is test-only, so
without it the new branch cannot fire live. Note also the narrow trigger
state (record A in row `0x59` frame `>= 2`): the jump would fire from the
*stumble* context, not from a kick.

**#3 — rows `0x14`/`0x5C`: keep leg (FU159-L2 + §3 bounds).** No evidence of
an installer; no wiring is possible without inventing a gate. The T3 test
stand-in (`anim_id` direct write) remains the disclosed stand-in for them.

---

## 5. Numbered legs (new)

* **FU160-L1 — the row-`0x1B` computed install id.** `0x7756E` installs
  `word[ESP+0x10] + 1` (`0x77547`/`0x77554`/`0x7755C`); the stack-local base
  is unresolved (the row-`0x1B` body is unported, FU-140 OL-34). Not an
  immediate; no static tie to `0x14`/`0x5C`.
* **FU160-L2 — the no-static-writer id globals.** `[0x158741]` (read
  `0x7A771`/`0x7AA14`/`0x7BEE5`; written only through a computed store) and
  `[0x157C2A]` (read `0x76846` only) hold run-time anim/event ids whose live
  domains are open; FU-79 ties `[0x157C2A]`'s family to the `0x10E17D` event
  table and FU-159 models `[0x158741]` as the ball `sub_code`. A live trace
  would close them; no static feed contains `0x14`/`0x5C`.
* **FU160-L3 — the `0x795B4` magnitude edge.** The quotient reading
  (|dx|/sinθ or |dz|/cosθ via `0xCD514` + the `0x114E04` table lookups
  `0xCE386`/`0xCE364`) as a whole-unit vector length is derived; the exact
  behavior at the `0x30` boundary and any rounding is unproven.
* Carried (unchanged): **FU159-L1** (arm impulsion), **FU159-L2** (rows
  `0x14`/`0x5C`, now bounded by §3), **FU159-L3** (event sinks incl. `0xA2`),
  **FU159-L5** (the `+0x46` lean byte), **FU159-L6** (now answered for row
  `0x5E`/`0x12` by §1/§2 — the residual is the *live wiring*, rec #1/#2).

## 6. Evidence index (fresh this slice)

* `disassemble_bytes`: `0x81CDC..0x81DDF`, `0x81DCF..0x81F10`,
  `0x824F0..0x8251C`, `0x82710..0x82742`, `0x82740..0x82810`,
  `0x6FB80..0x6FCE5`, `0x6FCE0..0x6FE69`, `0x6EF30..0x6EF70`,
  `0x6F150..0x6F190`, `0x6F680..0x6F6B5`, `0x6F720..0x6F780`,
  `0x6E598..0x6E660`, `0x71600`-range (`0x71580..0x71710`,
  `0x71740..0x71890`), `0x75CE0..0x75D05`, `0x76840..0x76875`,
  `0x774A0..0x77580`, `0x77980..0x779B0`, `0x88A10..0x88AA5`,
  `0x898F0..0x89975`, `0x7AA10..0x7AA35`,
  `0x81780..0x818E0`, `0x81AE5..0x81B10`, `0x87EF0..0x88000`,
  `0x88220..0x88250`, `0x88410..0x88470`, `0x885C0..0x885F0`,
  `0x88658..0x88688`, `0x8CFC0..0x8CFF5`, `0x8C2E0` (function listing).
* `decompile_function`: `0x795B4`, `0xCD514`, `0xCE386`, `0xCE364`, `0x79B58`,
  `0x6E598`, `0x6E330`, `0x8C2E0`, `0x8776C`, `0x71B9C`.
* `get_xrefs_to`: `0x6E598` (84, fresh), `0x79B58` (17, fresh), `0x8C2E0` (2,
  fresh).
* `search_instructions`: `0x7cf]` (3), `158741` (3), `157c2a` (1),
  `1587bb` (5), `1587bc` (5), `0x157794` (7), `+ 0x8d]` (19 loads, no
  store — the team-init store uses the `-0x25` base-relative form).
* `search_byte_patterns`: `ba 5c 00 00 00` (0), `ba 14 00 00 00` (23),
  `b2 5c` (3 ram), `b2 14` (12 ram).
* `read_memory`: `0x10EF00` (999 B, the native row table), `0x110500` (1024
  B), `0x11052A`/`0x1105AA`/`0x11058A`/`0x1105CA` (movement tables),
  `0x1103CB` (96 B), `0x10E17D` (64 B), `0x10ED3E` (40 B, row `0x59` frames),
  `0x10F344` (32 B, celebration tables).
* Engine cites: `fifa96_match_handlers.c:2203` (row-23 install `0x0E`),
  `:2460`/`:2704`/`:2812` (`timer93` `0x79B58` model), `:2851` (row `0x0C`
  NULL), `:2857` (row `0x0E` NULL); `fifa96_match_run.c:4805`
  (`fifa96_match_run_contact`), `:1094` (delta staging), `:1199` (`e->index !=
  0` outfield gate), `:1018-1070` (the ported jump arms);
  `fifa96_match_entities.c:549` (`0x157794` selection vector);
  `fifa96_arm_helpers.c:96-117` (`fifa96_arm_anim_select`);
  `tests/test_engine_match_frame.c:2133` (the `anim_id = 0x5E` stand-in),
  `:2998-3186` (`fifa96_match_run_contact` tests).
* Scratch (read-only scans): `/tmp/opencode/r8/rowtab.py` (the 111-row native
  table parse + `next_id` scan), `/tmp/opencode/r8/movscan.py` (movement-window
  byte scan), `/tmp/opencode/r8/fu160_scan.py` (first-pass combined scan).
