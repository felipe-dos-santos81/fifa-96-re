# FU283 — `FUN_00077EAC`, the keeper CPU decision ladder: port shape + shared helpers (draft)

**Phase/Provenance:** M2 phase-26c recon-ahead, Task 3 (bounded, read-only derivation); draft awaiting controller promotion.

Authoritative program **`/FIFA96.EXE`** (case-sensitive; confirmed this session by
`ghidra_get_current_program_info`: `watcom:LE:32:default`, image base 0, `.image`
overlay, 2706 functions). Read-only: no rename/comment/label/create/script/save;
`/FIFA96.EXE` untouched; ISO untouched; no git. Every native address/byte below is
a **fresh** `disassemble_bytes` / `disassemble_function` / `decompile_function` /
`read_memory` / `get_xrefs_to` of *this* session unless marked *carried* (FU273
§1.4 / §6.3 / §7, named). Alias traps applied (`dword[x]>>0x18` = byte `x+3`,
e.g. `[rec+0x8e]>>0x18` = byte `+0x91`; CALL rel32 bytes verified by hand).

## Port shape

`FUN_00077EAC` is the **row installer** for the keeper-CPU chain. Called once per
frame by `FUN_000782D0` (`0x78599`, *carried* FU273 §1.3) when `phase==2 &&
[rec+0x20]==0 && [0x157C5D]!=0`, it:

1. **Head-gates** a record: keep only a keeper whose action row `byte[rec+0x91]==0x19`,
   whose side passes the `[0x14C1D4]` prepass table (`AH&2`), that is not in the
   `+0x9A` skip state, whose `+0x28` string field is not `'T'`, and whose `+0x81`
   timer is clear.
2. **Skips the actor and the actor's side** (`[0x157A83]`).
3. Computes a **stage index `CX ∈ {1..5}`** from a camera/ball geometry block
   (calling the shared vector helper `0x795B4`).
4. Calls the shared predicate `FUN_000765C4`; if nonzero installs **code `4`**.
5. Otherwise dispatches `CX-1` through the jump table `0x77E98` to one of five
   install arms, each ending in `FUN_0007D9A4(rec, code, 0, 1)` **invoke-now**.

The ladder is the `0x1A`/`0x1B`/`0x1C` (and `0x4`) installer; it is **unported**
(FU186-L5 / FU180-L5). The engine models the rows/arms but has **no live installer**
(see GO/NO-GO).

## Step 1 — head gates (fresh bytes, `disassemble_bytes 0x77EAC`, 280 B)

```
0x77EB7  MOV EAX,[EAX+0x8e] ; 0x77EBD SAR EAX,0x18 ; 0x77EC2 CMP EAX,0x19
0x77EC5  JNZ 0x782c4          ; byte[rec+0x91] == 0x19  (dword+0x8E >> 0x18)
0x77ECB  MOV EAX,[EBP] ; MOV AL,[EAX+0x826] ; AND EAX,0xff
0x77ED9  CALL 0x000741b4      ; e8 d6 c2 ff ff -> next 0x77EDE - 0x3D2A = 0x741B4
0x77EDE  MOV AX,word[EAX*2+0x14c1d4]
0x77EE6  XOR AL,AL ; 0x77EE8 AND AH,0x2 ; 0x77EEB AND EAX,0xffff
0x77EF0  JNZ 0x782c4          ; per-side prepass-table gate (AH bit 2)
0x77EF6  CMP byte[EBP+0x9a],0 ; 0x77EFD JNZ 0x782c4     ; +0x9A skip
0x77F03  MOV EAX,[EBP+0x28] ; MOV AL,[EAX] ; AND EAX,0xff
0x77F0D  CMP EAX,0x54 ; 0x77F10 JZ 0x782c4             ; *(param_1[10]) == 'T'
0x77F16  CMP word[EBP+0x81],0 ; 0x77F1E JZ 0x77f2e
0x77F20  LEA EDI,[EBP+0x4d] ; LEA ESI,[EBP+0x59] ; MOVSD×3 ; JMP 0x782c4
```

`+0x81 != 0` copies `pos`(`+0x59`) → `+0x4D` and returns (no install). Every gate
exits to the shared tail `0x782C4` (epilogue `ADD ESP,0x14 / POP×5 / RET`,
`0x782C4..0x782CD`, also fresh).

## Step 2 — actor/side skip (fresh bytes `0x77F2E`)

```
0x77F2E  MOV EDX,[0x157a83] ; 0x77F34 CMP EBP,EDX ; 0x77F36 JZ 0x782c4   ; rec == actor -> return
0x77F3C  TEST EDX,EDX ; 0x77F3E JZ 0x77f57                               ; no actor -> proceed
0x77F40  MOV EAX,[EDX] ; MOV EDX,[EBP]
0x77F45  MOV AL,[EAX+0x826] ; 0x77F4B CMP AL,[EDX+0x826] ; 0x77F51 JZ 0x782c4
```

So the ladder runs for a keeper on the **opposite side** of the actor, and for
**any** keeper while `[0x157A83]==0`. Matches the decompile and FU273 §1.4.

## Step 3 — the stage index `CX` (fresh `disassemble_bytes 0x77FC3`, 480 B)

After the skip the block copies the camera triple `[0x15774C]` to the stack
(`0x77F59`), takes two `abs()` deltas (`0x77F61..0x77F80`), then:
`0x77F82 LEA EBX,[ESP+0xc]; 0x77F86 MOV EDX,0x157c48; 0x77F8D LEA EAX,[EBP+0x59];
0x77F90 CALL 0x000795b4` (`e8 1f 16 00 00` → `0x77F95+0x161F = 0x795B4`). It then
runs a long geometry/gate machine (`0x77F95..0x781A2`) over
`[0x157a49]`, `[0x157750]`, `[0x1577bc]`, `[0x1577f4]`/`[0x1577f8]`, `[0x157821]`,
`[0x1578..]`, `[0x158740]`, and the stage tables `0x10E184`/`0x10E1C4`/`0x10E1CE`,
assigning `ECX` ∈ {0,1,2,4,5} and copying either the camera or the ball/record
triple into `+0x4D`. Fresh anchors:

```
0x780F9  MOV DL,[EAX+0x10e184] ; 0x78106 MOV AX,word[EAX*2+0x10e1c4]
0x78193  MOV EAX,dword[EAX*2+0x10e1ce]
0x780D2  MOV ECX,0x4 ; 0x780D7 MOV byte[0x157c5c],BH  (latch [0x157C5C]:=1)
0x7802D  MOV [0x157c5c],AL                            (latch [0x157C5C]:=0)
```

## Step 4 — the switch table and the five install arms (fresh)

Fresh `read_memory 0x77E98` (24 B) =
`6d 82 07 00 | 45 82 07 00 | 1d 82 07 00 | 94 82 07 00 | b1 82 07 00 | 53 51 52 56`
→ **`{0x7826D, 0x78245, 0x7821D, 0x78294, 0x782B1}`** (the trailing `53 51 52 56`
is `FUN_00077EAC`'s prologue, i.e. the table is exactly 20 B, `0x77E98..0x77EAC`).

Dispatch (fresh `disassemble_bytes 0x781A0`/`0x78210`):

```
0x781E1  TEST CX,CX ; 0x781E4 JZ 0x782c4              ; stage 0 -> return
0x781EA  MOV EAX,EBP ; 0x781EC CALL 0x000765c4        ; e8 d3 e3 ff ff -> 0x765C4
0x781F1  TEST AX,AX ; 0x781F4 JZ 0x78205              ; ==0 -> switch
0x781F6  MOV ECX,0x1 ; 0x781FB MOV EDX,0x4 ; 0x78200 JMP 0x782bb   ; else code 4
0x78205  DEC ECX ; 0x78206 CMP CX,0x4 ; 0x7820A JA 0x782c4        ; CX-1 in 0..4
0x78210  XOR EAX,EAX ; 0x78212 MOV AX,CX
0x78215  JMP dword CS:[EAX*4 + 0x77e98]               ; 2e ff 24 85 98 7e 07 00
```

So the table index is **`CX_orig - 1`** (`CX_orig` ∈ 1..5). The FU273 §1.4 "case
1..5" arm labels are *arm addresses*, not `CX` values; the `CX↔arm` map is:

| `CX_orig` | table idx | arm | installs | extra (fresh bytes) |
|---|---|---|---|---|
| 3 | 2 | `0x7821D` | row `0x1A` | `0x7821D MOV ESI,0x2e`; `0x78230 MOV [0x157c2c],ESI` |
| 2 | 1 | `0x78245` | row `0x1A` | `0x78245 MOV EDI,0x2d`; `0x78258 MOV [0x157c2c],EDI` |
| 1 | 0 | `0x7826D` | row `0x1A` | `0x7826D MOV EAX,0x2c`; `0x7827e MOV [0x157c2c],EAX` |
| 4 | 3 | `0x78294` | row `0x1B` | — |
| 5 | 4 | `0x782B1` | row `0x1C` | falls through to `0x782BB` |

Install immediates + `CALL 0x7D9A4` rel32 sites (all fresh, 5–10 B after the
`MOV EDX`):

```
0x78227 MOV EDX,0x1a  | 0x78236 CALL 0x7d9a4  (e8 69 57 00 00 -> 0x7823b+0x5769 = 0x7D9A4)
0x7824f MOV EDX,0x1a  | 0x7825e CALL 0x7d9a4  (e8 41 57 00 00 -> 0x78263+0x5741)
0x78277 MOV EDX,0x1a  | 0x78285 CALL 0x7d9a4  (e8 1a 57 00 00 -> 0x7828a+0x571a)
0x78299 MOV EDX,0x1b  | 0x782a2 CALL 0x7d9a4  (e8 fd 56 00 00 -> 0x782a7+0x56fd)
0x782b6 MOV EDX,0x1c  | 0x782bf CALL 0x7d9a4  (e8 e0 56 00 00 -> 0x782c4+0x56e0)
```

Each arm sets `ECX=1` and `EBX=0` (invoke-now flags) — so every install is
`FUN_0007D9A4(rec=EBP, code=EDX, 0, 1)`.

## Step 5 — the `FUN_000765C4 != 0` fall-through installs **code 4** (fresh)

```
0x781EC CALL 0x000765c4 ; 0x781F1 TEST AX,AX ; 0x781F4 JZ 0x78205
0x781F6 MOV ECX,0x1 ; 0x781FB MOV EDX,0x4 ; 0x78200 JMP 0x782bb
0x782BB MOV EAX,EBP ; 0x782BD XOR EBX,EBX ; 0x782BF CALL 0x7d9a4
```

Confirms FU273 §1.4: the tracked-teammate branch installs **code `4`** via the
shared tail, *not* a second `0x1B`. The `0x782B1` (`0x1C`) arm falls into the same
`0x782BB` tail.

## Step 6 — the shared helpers (fresh)

* **`FUN_000765C4` (`0x765C4`)** — the tracked-teammate predicate. Fresh
  `get_xrefs_to 0x765C4` = **2** (`0x781EC` in `FUN_00077eac`; `0x76695` — no
  analysed function, the row-`0x1A` region). Fresh `disassemble_function` +
  decompile: if `[0x1577CA]!=0` and same side (`byte +0x826`) then it returns `1`
  when the tracked record's `byte +0x91 == 5`, else returns `1` unless
  `[0x158740]>>0x18 == 4` (in which case `0`). I.e. "hold the keeper CPU while the
  tracked teammate is in row 5 (or the global gate isn't 4)".
* **`0x795B4` (`0x795B4`)** — the band/vector helper. Fresh `disassemble_bytes`:
  `(word[arg0] - word[arg1])>>16` and `(word[arg0+8] - word[arg1+8])>>16` are
  written to `[arg2+2]`/`[arg2+4]`, then `0xCD514` is called (heading/magnitude).
  Fresh `get_xrefs_to 0x795B4` = **12**, including **`0x77F90`** (the ladder) and
  `0x8DA8F` in `FUN_0008D8EC` (the driver pre-pass) — the two share this helper.
* **`0x795A4` (`0x795A4`)** — the fixed-point fold. Fresh `disassemble_bytes`:
  `IMUL EDX ; ADD EAX,0x8000 ; ADC EDX,0 ; SHRD EAX,EDX,0x10 ; RET` = a signed
  32×32 round-and-shift-to-16 multiply. Fresh `get_xrefs_to 0x795A4` = **69**
  (broad use, e.g. `FUN_00076b28`, `FUN_000709d0`, `FUN_0006e8e8`); the ladder
  does **not** call it directly.
* **`0x114E04`** — fresh `read_memory 0x114E04` (32 B) =
  `00 00 00 00 | 92 01 00 00 | 24 03 00 00 | b6 04 00 00 | 48 06 00 00 |
   da 07 00 00 | 6c 09 00 00 | fe 0a 00 00` — a **monotone 32-bit ramp** (0, 402,
  804, 1206, 1608, 2010, 2412, 2814; step `0x192`). Fresh `disassemble_bytes
  0x114E04` yields garbage **data** bytes; fresh `get_xrefs_to 0x114E04` = **91
  DATA** refs (no calls). The doc label "`0x114E04` sine" is **carried** and is
  **not confirmed** by these bytes (see leg FU283-L4).

## GO/NO-GO

**Reachability of the ladder's inputs from ported engine state.**

*Reachable (engine already models the cell under a name):* action row
`byte[rec+0x91]` → `entity.code` (`fifa96_match_run.c:600` reads `tracked[+0x91]`);
side `byte +0x826` → `team.side` (`fifa96_match_entities.c:30`); actor
`[0x157A83]` → `mr->entities.controlled` (`fifa96_match_run.c:890`);
camera `[0x15774C/50/54]` → `render.camera.pos_x/pos_y/pos_z`
(`fifa96_match_run.c:583`, T1/L14.7); tracked `[0x1577CA]` → `camera.tracked`
(`fifa96_match_run.c:571`); record `pos +0x59` → `entity.pos_*`.

*Not reachable (unported cells the ladder reads):* the per-side prepass word
table `[0x14C1D4]` (via `FUN_000741B4`); the stage tables
`0x10E184`/`0x10E1C4`/`0x10E1CE`; the latch/geometry cells
`[0x157C4C]`/`[0x157C48]`/`[0x157C2C]`/`[0x157C5C]`; the height/geometry words
`[0x157750]`(modeled)/`[0x1577F4]`/`[0x1577F8]`/`[0x1577BC]`/`[0x157821]`; the
gate byte `[0x158740]`; the per-record sub-byte `[[rec+0x4]+0xB]` and word fields
`+0x69`/`+0x71`/`+0x73`.

**Verdict: CONDITIONAL — inputs are internal match state, but ~half are unported.**
No input is an external/blocker source (all are in-match record/global cells), so
the ladder is **mechanically wireable**; however, four input families (the
`[0x14C1D4]` prepass table, the `0x10E1xx` stage tables, the `[0x157C4x]` latch
cells, and the stage-geometry words) have **no engine field yet**, so the ladder
**cannot be driven from current engine state** without porting them. This does not
overturn FU273's phase-scale **NO-GO** for the live release — it localises the
ladder's remaining input gap to those four families. A bounded FU283-class task can
port the **ladder itself** (install arms + the actor/side + code-4 logic), but a
*live* stage index still needs the four input families (legs FU283-L1..L6).

## Numbered open legs (with owners)

* **FU283-L1 — the `[0x14C1D4]` per-side prepass word table + `FUN_000741B4`
  side index** (the `AH&2` gate, `0x77EDE`). Content/semantics unported; the
  side-index helper is *carried* (not fresh-verified this pass). Owner:
  FU186-L5 / FU180-L5 (and FU180-L4 for `FUN_000741B4`).
* **FU283-L2 — the stage tables `0x10E184` / `0x10E1C4` / `0x10E1CE`**
  (`0x780F9`/`0x78106`/`0x78193`). Owner: FU186-L3 / FU180-L2.
* **FU283-L3 — the latch/geometry cells `[0x157C4C]`/`[0x157C48]`/`[0x157C2C]`
  and the latch `[0x157C5C]`** (writers `0x7802D`/`0x780D7` here; sole row-`0x19`
  setter `0x74707` for `[0x157C5D]`, *carried* FU273 §1.3). Owner: FU186-L5/L6.
* **FU283-L4 — the "`0x114E04` sine" label is unconfirmed.** Fresh bytes are a
  monotone 32-bit ramp (step 402), data not code, 91 DATA xrefs. The sine
  semantics is *carried* (FU273 §1.4 / FU-186). Owner: FU186-L3 / FU180-L2.
* **FU283-L5 — the per-record stage-geometry fields** `[[rec+0x4]+0xB]` (`>>0x18`,
  `0x78068`) and the words `+0x69`/`+0x71`/`+0x73` (`0x78032`/`0x780B0`). Owner:
  FU180-L1/L3.
* **FU283-L6 — the full `CX` stage-select machine (`0x77F95..0x781A2`)** —
  camera/ball gates over `[0x1577F4/F8]`, `[0x1577BC]`, `[0x157821]`, `[0x158740]`;
  only the anchors are pinned here, not the full truth table. Owner: FU186-L5.
* **FU283-L7 — `0xCD514`** (called by `0x795B4` at `0x795E2`): the heading/atan
  step; not disassembled this pass. Owner: FU180-L2.
* **FU283-L8 — the five-arm installer's live driver** (`FUN_000782D0`
  `0x78599` + the `[0x157C5D]` interlock) remains unported — *carried* FU273
  §1.3. Owner: FU186-L6 / FU180-L4.

## Evidence table (all fresh, `/FIFA96.EXE`, read-only)

| claim | fresh evidence |
|---|---|
| head gates `byte+0x91==0x19`, `CALL 0x741B4`+`word[EAX*2+0x14C1D4]`/`AH&2`, `+0x9A`, `[+0x28]=='T'`, `+0x81` | `disassemble_bytes 0x77EAC` |
| actor/side skip (`[0x157A83]`, same `+0x826`) | `disassemble_bytes 0x77EAC` (`0x77F2E..0x77F51`) |
| switch table = `{0x7826D,0x78245,0x7821D,0x78294,0x782B1}`; 20 B; `53 51 52 56` = prologue | `read_memory 0x77E98` (24 B) |
| dispatch `CX-1 ∈ 0..4` via `JMP CS:[EAX*4+0x77E98]` | `disassemble_bytes 0x781A0`, `0x78210` |
| install immediates `0x1a`×3 / `0x1b` / `0x1c` at `0x78227/4f/77/99/b6` | `disassemble_bytes 0x78210` |
| `CALL 0x7D9A4` rel32 at `0x78236/5e/85/a2/bf` (all → `0x7D9A4`) | `disassemble_bytes 0x78210` |
| `[0x157C2C] := 0x2e/0x2d/0x2c` at `0x78230/58/7e` | `disassemble_bytes 0x78210` |
| code-`4` fall-through `0x781F6..0x78200` → tail `0x782BB` | `disassemble_bytes 0x781A0` |
| `FUN_000765C4` refs = 2 (`0x781EC`, `0x76695`) | `get_xrefs_to 0x765C4` |
| `FUN_000765C4` = tracked-teammate row-5 / `[0x158740]` predicate | `decompile_function 0x765C4`, `disassemble_function 0x765C4` |
| `0x795B4` = 2-D vec-diff + `0xCD514`; refs = 12 incl. `0x77F90`, `0x8DA8F` | `disassemble_bytes 0x795A4`; `get_xrefs_to 0x795B4` |
| `0x795A4` = signed 32×32 round-shift-16 fold; refs = 69 | `disassemble_bytes 0x795A4`; `get_xrefs_to 0x795A4` |
| `0x114E04` = monotone 32-bit ramp (step 402), data, refs = 91 DATA | `read_memory 0x114E04`; `disassemble_bytes 0x114E04`; `get_xrefs_to 0x114E04` |
| stage block anchors `0x10e184`/`0x10e1c4`/`0x10e1ce`, `[0x157c5c]` writes `0x7802D`/`0x780D7` | `disassemble_bytes 0x77FC3` |
| engine cell map: `+0x91`→`entity.code`, `+0x826`→`team.side`, `[0x157A83]`→`entities.controlled`, `[0x15774C/50/54]`→`render.camera.pos_*`, `[0x1577CA]`→`camera.tracked` | `fifa96_match_run.c:583/600/890`, `fifa96_match_entities.c:30` (read-only) |
| engine references essentially none of the ladder's native cells by address | `grep 0x157…/0x158…/0x14c…/0x10e1… src/*.c` = 1 incidental hit |

**Carried (read, not rewritten):** FU273 §1.3/§1.4/§6.3/§7.
