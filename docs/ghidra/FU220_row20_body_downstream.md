# FU-220 — action row `0x20` body `0x84EEC` — the downstream field-level completion (phase-20 wave R68)

Provenance: read-only, 2026-10-13. Authoritative program **`/FIFA96.EXE`**
(Ghidra MCP; scripts disabled; case-sensitive program argument; confirmed this
session by `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`,
image base `0`, `min_address 0x10000`). Every address/byte/instruction below is a
fresh `read_memory` / `disassemble_bytes` / `decompile_function` of **this**
session unless marked *cross-cited* (then the frozen FU doc is named). LE-image
globals sit at `+0x100000` from the file offset; the addresses here are
`/FIFA96.EXE` program (runtime) addresses. The alias traps are applied
(`dword[x]>>16` = the word at `x+2`, `>>24` = the byte at `x+3`). Engine and
loader source read first-hand this session
(`src/fifa96_engine/fifa96_match_handlers.c`, `fifa96_match_run.c`,
`src/fifa96_loader/fifa96_ball_pairing.c`,
`include/fifa96_engine/fifa96_match_entities.h`,
`include/fifa96_loader/fifa96_ball_pairing.h`, `tests/test_engine_m2.c`,
`tests/test_engine_match_handlers.c`). **No writes**: no rename, comment, label,
function, script or project save; `/FIFA96.EXE` untouched; ISO untouched; no git.

Scope (R68 brief): **extend** the frozen `docs/ghidra/FU210_action_row20.md`
(the body `0x84EEC`, its 7 arms, the installer `0x899A2` are already derived
there) with the **downstream / field-level** completion: the arm-by-arm store
map (record + ball-camera cells + anim/frame + globals), the exact
`0x7A490`/`0x79B6C` **ball-record place** (FU210-L2), the **port delta**, the
**tape (M2) visibility**, and the **fixture** shape. This slice does not
re-derive the arm dispatch, the asset table or the installer route — those stand
as FU210 §2/§3/§6.

---

## 1. Verdict (one line)

**The 7 arms write the record target triple (`+0x4D/51/55`, arms 0 + 4) and the
record position triple (`+0x59/5D/61`) + anim/frame (arms 0 + 4) — arm 0 places
the ACTOR record and the BALL record `[[rec]+0x7A6]` via `0x79B6C` and then runs
the two `0x8CFAC` 11-stride anim walks (team base + ball record), arm 4 places
the actor via the `0x7B9C4`→`0x7A490` kick — plus the global ball-camera triple
`0x15774C/50/54` and the `0x73E08` block commit; the port `fifa96_match_action_20`
writes only the ACTOR's target+pos+anim, dropping the FU210-L2 ball-record place
(a genuine native **record** write on `[[actor]+0x7A6]`), the `0x8CFAC` walk,
arm 4's `0x7A490` record pos_x/pos_z + anim write-back (the port stages a local
`fifa96_ball_kick_actor` at `handlers.c:1034` and never copies it back), and the
`0x73E08`/`0x73DC4`/`0x700F4` global writes — all **dormant on M2** (no record
ever carries code `0x20`), so wiring them changes nothing on the tape.**

---

## 2. Fresh anchors (this session)

* **Arm table.** Fresh `read_memory 0x84ED0` (32 B) =
  `5c4f0800 56500800 2e510800 53510800 97510800 e1510800 06520800 53515256`
  → `{0x84F5C, 0x85056, 0x8512E, 0x85153, 0x85197, 0x851E1, 0x85206}` followed by
  the next function's prologue `53 51 52 56` (`PUSH EBX/ECX/EDX/ESI`). Byte-for-
  byte FU210 §3.1.
* **Body range.** Fresh `disassemble_bytes 0x84EEC..0x85214` (808 B, 208
  insns, `truncated:false`) — the full 7-arm body. `0x84EF2 MOV EBP,EAX`
  confirms `EBP = rec`; `0x85213 RET` is the epilogue; `0x85214` = row `0x21`.
* **Every CALL target byte-verified this session** (target = next-insn + rel32):
  `0x84F12→0x4C31C`, `0x84F69→0x73DC4`, `0x84F84→0x700F4`, `0x84F96→0x73E08`,
  `0x84FB7→0x79B6C`, `0x84FDB→0x79B6C`, `0x84FF4→0x7876C`, `0x85003→0x8CFAC`,
  `0x85018→0x8CFAC`, `0x8502F→0x4C324`, `0x85039→0x4C374`, `0x85063→0x4BEC8`,
  `0x85071→0x7A028`, `0x85111→0x8F188`, `0x85165→0x78A84`, `0x851B6→0x78AA4`,
  `0x851C4→0x7B9C4`, `0x851E3→0x79B1C`, `0x85208→0x7DAB4`.

Record offsets used below (fresh first-hand `fifa96_match_entities.h:61..141`):
`+0x00` team ptr (deref `+0x7A6` = ball record), `+0x20` control-slot ptr,
`+0x28` anim row ptr (`anim_id = byte[[rec+0x28]]`), `+0x3D` frame,
`+0x44` ack (row44), `+0x4D/0x51/0x55` target x/y/z, `+0x59/0x5D/0x61`
pos x/y/z, `+0x69` lane dword, `+0x6B/0x6D` lane/nudge words, `+0x71/0x73/0x75`
velocity, `+0x89` timer89, `+0x92` stage92, `+0x9A` hidden.

---

## 3. Arm-by-arm field map (fresh `disassemble_bytes 0x84EEC..0x85214`)

`R = EBP = rec`. **Store class** marks each write: **render** = M2 render-read
(the four families FU206 §5 / FU210 §4 — `pos_x/y/z`, `anim_id`, `frame`,
`hidden`), **global** = a non-record cell, **internal** = record bookkeeping the
render never reads.

### 3.1 Prologue (`0x84EEC..0x84F5B`)

| addr | instruction | write | class |
|---|---|---|---|
| `0x84F00` | `MOV dword[0x14C114],EDX` | `[0x14C114]=0` | global (input word 0) |
| `0x84F06` | `MOV dword[0x14C118],EDX` | `[0x14C118]=0` | global (input word 1) |
| `0x84F0C` | `MOV dword[0x14C11C],ECX` | `[0x14C11C]=0x9F0` | global |
| `0x84F12` | `CALL 0x4C31C` | *no-op* (fresh decompile `FUN_0004c31c` = `return;`) | — |
| `0x84F25` | `MOV [0x157A83],EBP` (if `stage92 < 3`) | `[0x157A83]=rec` | global |
| `0x84F41` | `MOV [R+0x89],ESI` | `timer89 += word[0x157A64]` | internal |

The stage is read signed (`0x84F17 MOV EAX,[R+0x8F]; SAR EAX,0x18`), so the
`< 3` test is a signed byte compare; the dispatch `0x84F47 CMP AL,6; JA 0x8520D`
is unsigned.

### 3.2 Arm 0 (`0x84F5C..0x85055`) — ball-camera place + actor/ball place + anim + block commit

Globals (the ball-camera cells and the process cells):

| addr | instruction | write | class |
|---|---|---|---|
| `0x84F63` | `MOV [0x157A83],EBP` | `[0x157A83]=rec` | global |
| `0x84F69` | `CALL 0x73DC4` (EAX=0, EDX=`0x15774C`) | `[0x15774C]=0`, `[0x157750]=0`, `[0x157754]=0x8D0` (fresh `FUN_00073dc4`: `*p=0; p[1]=0; p[2]=0x8d0` for `param_1!=1`) | global (ball-camera triple) |
| `0x84F84` | `CALL 0x700F4` (push `[0x157754],[0x157750],[0x15774C]`) | `[0x15774C/50/54]` re-latched + the `0x1577xx/0x1578xx` camera block reset (fresh `FUN_000700f4`) | global (camera) |
| `0x84F90` | `MOV byte[0x15781D],AH` (AH=0) | `[0x15781D]=0` | global |
| `0x84F96` | `CALL 0x73E08` | `FUN_0008C24C(&0x158830)` + `FUN_0008CF60(&0x1588A4)` + `FUN_0008CF60(&0x1590D9)` | global + **22 records** |
| `0x8502F` | `CALL 0x4C324` (ECX=-1, EAX=`[0x14C110]`, EBX=EDX=0) | `[0x1074A4]` (bind) + conditional `CALL 0x36200` (fresh `FUN_0004c324`) | global |
| `0x85039` | `CALL 0x4C374` (EAX=0x1B) | `word[0x157A68]=0` (fresh `FUN_0004c374`: `_DAT_00157a68=0`, **EAX ignored**) | global |

The `0x73E08` block is the large one. Fresh `disassemble_bytes 0x8CF60` (23
insns) confirms `FUN_0008CF60` walks **11 records** (`ESI=[EAX+0x4D]`, `EDI`
counter `0..0xA`, stride `0xB2`, loop `0x8CF95/0x8CF9B`):

```
0x8CF73 CALL 0x6D920                     ; the per-record phase machine
0x8CF7C CALL dword[ECX+0x1c]             ; [[rec+0x1C]](rec,&rec+0x4D) — per-record dispatch
0x8CF86 CALL 0x79F3C
0x8CF90 CALL 0x79B6C                     ; place rec at (rec+0x4D..) w/ camera triple 0x15774C
```

So arm 0's `0x73E08` **places all 22 team records** (pos + anim via `0x79B6C`)
and dispatches each `[rec+0x1C]`. `FUN_0008C24C` (fresh decompile) writes the
ball block cells `0x15880C`/`0x158814` (or `0x1C80`/`0x2C40` under the session
gate `[0x14C32A]!=0`).

Record writes:

| addr | instruction | write | class |
|---|---|---|---|
| `0x84F9E..0x84FA0` | `MOVSD x3` (`EDI=R+0x4D`, `ESI=0x15774C`) | `[R+0x4D]=[0x15774C]`, `[R+0x51]=[0x157750]`, `[R+0x55]=[0x157754]` | internal (target) |
| `0x84FA7/0x84FAF` | `SUB EBX,0xF0; MOV [R+0x55],EBX` | `target_z -= 0xF0` | internal |
| `0x84FB7` | `CALL 0x79B6C` (`EAX=R`, `EDX=R+0x4D`, `EBX=0x15774C`) | **actor place**: `[R+0x59/0x5D/0x61]=target`, `pos_y=0`, `target:=pos`, clears lane/vel/`+0x65/0x67/0x69/0x71/0x73/0x75/0x9C`, `0x79C50` face, `0x6E598(R, 0x26 if byte[R+0x8D]==0 else 0, 0)` | **render** (pos + anim + frame) |
| `0x84FCA` | `MOV dword[EAX+0x4D],0` (`EAX=[[R]+0x7A6]`) | `[ball+0x4D]=0` | internal (ball target_x) |
| `0x84FD4` | `MOV dword[EAX+0x55],0xB10` | `[ball+0x55]=0xB10` | internal (ball target_z) |
| `0x84FDB` | `CALL 0x79B6C` (`EAX=ball`, `EDX=ball+0x4D`, `EBX=0x15774C`) | **ball place**: `[ball+0x59/0x5D/0x61]=(0,0,0xB10)`, `target:=pos`, clears, `0x79C50` face, `0x6E598(ball, 0x26 or 0, 0)` | **render** (ball pos + anim + frame) |
| `0x84FF4` | `CALL 0x7876C` (gated `[R+0x20]!=0` && `byte[[R]+0x828]!=0`) | merges `[R+0x20]` slot ⇄ the nearest team record (fresh `FUN_0007876c`) | internal |
| `0x85003` | `CALL 0x8CFAC` (`EAX=[R]`, `EDX=0x26`, `EBX=0`) | **11-stride walk** on the team base: `0x6E598([R], 0x26, 0)` + `0x6E598([R]+0xB2*k, 0, 0)` `k=1..10` | **render** (anim + frame ×11) |
| `0x85018` | `CALL 0x8CFAC` (`EAX=[[R]+0x7A6]`, `EDX=0x26`, `EBX=0`) | **11-stride walk** on the ball record | **render** (anim + frame ×11) |
| `0x85044/0x85050` | `MOV [R+0x89],0; MOV [R+0x92],DH+1` | `timer89=0`, `stage92=1` | internal |

`FUN_0008CFAC` (fresh decompile): `FUN_0006E598(param_1, param_2, 0)` then the
`do{ FUN_0006E598(param_1+0xB2, param_3, 0); … }while(<0xB)` — i.e. the base
record gets `param_2` and the next 10 stride-`0xB2` records get `param_3`.

`FUN_00079B6C` (fresh decompile) record writes: `[param_1+0x59]=*param_2`,
`[param_1+0x5D]=*(param_2+2)`, `[param_1+0x61]=*(param_2+4)`, then `[+0x5D]=0`,
`[+0x4D]=[+0x59]`, `[+0x51]=[+0x5D]`, `[+0x55]=[+0x61]`, `word[+0x69]=0`,
`word[+0x71]=0`, `byte[+0x9C]=0`, `word[+0x67]=word[+0x65]=0`,
`word[+0x75]=word[+0x73]=0`, then `0x79C50` face and
`FUN_0006E598(param_1, 0x26|0, 0)`.

### 3.3 Arm 1 (`0x85056..0x8512D`) — event/ring pick

Gate: `byte[0x15882A]!=0` (`0x85056`) **and** `CALL 0x4BEC8()` `AX==0`
(`0x85063`) **and** `CALL 0x7A028()` (`0x85071`) **and** `[R+0x89]>=0x3C`
(`0x85076`). Reads `[0x1587E6]>>24` vs `[R]+0x826`, `CALL 0x92AC8()`, then
`CALL 0x8F188(EAX∈{0x26,0x28,0x25,0x27,0x54}, EBX=0x20, EDX=R or ball)`
(`0x85111`, the FU-63 event sink). Advances `[R+0x89]=0`, `[R+0x92]+=1`.
**No record render store.**

### 3.4 Arm 2 (`0x8512E..0x85152`) — wait

`CMP [R+0x89],0x3C; JL 0x8520D`; else `[R+0x89]=0`, `[R+0x92]+=1`.
**No record render store.**

### 3.5 Arm 3 (`0x85153..0x85196`) — the control-slot predicate

`EDX=[R+0x20]`; if `EDX!=0` and `word[EDX+6]==0` → wait while `[R+0x89]<=0x78`
(`0x8516C`); if `EDX!=0` and `word[EDX+6]!=0` → `CALL 0x78A84(EDX)` (`0x85165`,
fresh `FUN_00078a84`: copies slot `+6→+10`, `+0x10→+0x16`, `+0x23→+0x24`);
advance. **No record render store** (the slot object only).

### 3.6 Arm 4 (`0x85197..0x851E0`) — the kick/commit

| addr | instruction | write | class |
|---|---|---|---|
| `0x85197..0x851A1` | `MOVSD x3` (`ESI=0x15774C`, `EDI=R+0x4D`) | `[R+0x4D/0x51/0x55] := [0x15774C/50/54]` | internal (target) |
| `0x851A2..0x851AB` | `MOV EAX,[R+0x69]; SAR EAX,0x10; CMP EAX,0x40; JG 0x8520D` | (gate: `lane>>16 > 0x40` → epilogue) | — |
| `0x851B6` | `CALL 0x78AA4(slot)` if `[R+0x20]!=0` | slot restore (fresh `FUN_00078aa4`: copies `+10→+6`, `+0x16→+0x10`, `+0x24→+0x23` + the `0x10E1DC/EC/ F5` direction tables) | internal |
| `0x851C4` | `CALL 0x7B9C4` (`EDX=0x40`, `EAX=R`, `EBX=0`) | **kick**: `0x7B9C4`→`FUN_0007A490(R, &0x158738, …)` | **render** (pos_x/pos_z + anim + frame — see §4) |
| `0x851CF/0x851DB` | `MOV [R+0x89],0; MOV [R+0x92],AL+1` | `timer89=0`, `stage92+1` | internal |

### 3.7 Arm 5 (`0x851E1..0x85205`) — snap

`CALL 0x79B1C(R)` (`0x851E3`; fresh decompile: `[R+0x4D/0x51/0x55]=[R+0x59/0x5D/0x61]`,
`word[+0x69]=0`, `word[+0x71]=0`, `byte[+0x9C]=0`, `word[+0x67]=word[+0x65]=0`,
`word[+0x75]=word[+0x73]=0` — **target/velocity only, no render field**). Then
`CMP byte[R+0x44],0; JZ 0x8520D`; else `[R+0x89]=0`, `[R+0x92]+1`.

### 3.8 Arm 6 (`0x85206..0x85213`) — reset

`CALL 0x7DAB4(R)` (`0x85208`; fresh decompile: `byte[R+0x92]=0xFF`,
`[R+0x89]=0`, `0x78B00([R+0x20])` if set, then `FUN_0007D9A4(R,0,0,0)` — the
install-code-0 reset). No render store.

### 3.9 Field-map summary

* **Render fields written by the body:** arm 0 → actor `pos_x/y/z` + `anim_id` +
  `frame` (`0x79B6C`), the ball record `pos_x/y/z` + `anim_id` + `frame`
  (`0x79B6C`), and 11+11 records' `anim_id` + `frame` (the two `0x8CFAC` walks);
  arm 4 → actor `pos_x/pos_z` + `anim_id` + `frame` (`0x7A490`).
* **`hidden` (`+0x9A`) is never written** by any arm.
* **Ball-camera cells** `0x15774C/50/54` are written in arm 0 (via `0x73DC4` then
  `0x700F4`) and read (MOVSD) into the target triple in arms 0 and 4.
* **Other globals** written: `0x157A83` (prologue/arm 0), `0x15781D` (arm 0),
  `0x14C114/118/11C` (prologue), `0x157A68` (arm 0 `0x4C374`), `0x1074A4`
  (arm 0 `0x4C324`), the `0x15880C/14` ball-block cells + the `0x1587xx/0x1588xx`
  camera/team block (arm 0 `0x73E08`).
* **Arms 1/2/3/5/6** write no render field.

---

## 4. The ball-record place (FU210-L2) — the exact `0x79B6C` record write

> **Label reconciliation.** The brief calls FU210-L2 "the `0x7A490` record
> `pos_x/pos_z`+anim write-back". The frozen FU210 §8 L2 defines it as "the
> ball-record place at `(0, 0xB10)`; the 11-stride `0x8CFAC` walk" — which is
> **arm 0's `0x79B6C`** place, not `0x7A490` (that is arm 4's kick). Both are
> genuine native records; both are listed below, byte-exact, and both are port
> gaps.

**(a) The arm-0 ball-record place — `FUN_00079B6C` on `[[actor]+0x7A6]`.** Fresh
bytes at `0x84FBC..0x84FE0`:

```
0x84fbc 8b4500              MOV EAX, dword[EBP]          ; EAX = [actor] (team ptr)
0x84fbf 8b80a6070000        MOV EAX, dword[EAX+0x7a6]    ; EAX = the BALL record
0x84fc5 bb4c771500          MOV EBX, 0x15774c            ; the camera triple
0x84fca c7404d00000000      MOV dword[EAX+0x4d], 0       ; ball target_x := 0
0x84fd1 8d504d              LEA EDX, [EAX+0x4d]          ; EDX = &ball+0x4D
0x84fd4 c74055100b0000      MOV dword[EAX+0x55], 0xb10   ; ball target_z := 0xB10
0x84fdb e88c4bffff          CALL 0x79b6c                 ; place ball @ (0, 0, 0xB10)
```

`0x84FDB` CALL re-computed: `0x84FE0 + 0xFFFF4B8C = 0x79B6C`. ✔ The called
`FUN_00079B6C` (fresh decompile) writes **on the ball record**:
`[ball+0x59]=0` (pos_x), `[ball+0x5D]=0` (pos_y), `[ball+0x61]=0xB10` (pos_z),
then `[ball+0x4D/0x51/0x55] := [ball+0x59/0x5D/0x61]` (target := position), clears
`word[+0x69]`/`word[+0x71]`/`byte[+0x9C]`/`word[+0x65/0x67/0x73/0x75]`,
faces the ball (`0x79C50`), and calls `0x6E598(ball, 0x26|0, 0)` → `[ball+0x28]`
(anim row), `[ball+0x3D]` (frame), `[ball+0x44]`.

**This is a genuine *record* write**: the target object is
`[[actor]+0x7A6]` — the actor team's ball record (`[ESP+0xC]` in the sit-8
install frame, FU194 §3.2 / FU210 §6.1) — and the writes are to that record's
`pos`/`target`/`anim` fields, not to a transient local. Fields + values:
`ball+0x4D=0`, `ball+0x55=0xB10`, `ball+0x59=0`, `ball+0x5D=0`,
`ball+0x61=0xB10`, `ball+0x28` (anim ptr), `ball+0x3D` (frame).

**(b) The arm-4 kick-record place — `FUN_0007B9C4` → `FUN_0007A490` on the ACTOR
record.** Fresh `0x851C4 = e8fb67ffff` → `0x7B9C4`. Fresh decompile
`FUN_0007A490(param_1,…)`:

```
if (*(char *)((int)param_1 + 0x99) != 0 && param_4 in {4,5,6}) return;  // early-outs
if (DAT_00157a6c != 0) return;
...
DAT_00158730 = param_1;          // latch the actor record (the global the kick reads)
...
if (*(int *)((int)param_1 + 0x69) >> 0x10 < 0x60) {
  *(int *)((int)param_1 + 0x59) += *(int *)((int)param_1 + 0x6b) >> 0x11;  // pos_x += nudge_x>>17
  *(int *)((int)param_1 + 0x61) += *(int *)((int)param_1 + 0x6d) >> 0x11;  // pos_z += nudge_z>>17
}
...
FUN_0006e598(param_1, (short)DAT_00158744, (short)DAT_00158745);           // anim + frame
```

`param_1 = DAT_00158730` = the actor record latched by `0x7B9C4` from its `EAX`
arg (arm 4 passes `EAX=EBP=R`). So the kick writes the actor record's
`pos_x`/`pos_z` (`+0x59`/`+0x61`, the same M2 render family) plus `0x6E598`'s
anim + frame. **Genuine record write**, gated `word[+0x69]>>16 < 0x60`.

**(c) The port's equivalent is a *local copy only*.** The engine's kick entry
`fifa96_match_action_20` arm 4 calls `match_sp_kick(mr, NULL, 0x40, 0)`
(`handlers.c:5068`) → `match_kick_run` (`handlers.c:1031`), which builds a
**stack-local** actor:

```c
fifa96_ball_kick_actor a;                 /* handlers.c:1034 */
...
a.pos_x = s->pos_x; a.pos_y = s->pos_y; a.pos_z = s->pos_z;   /* handlers.c:1061..1063 */
rc = fifa96_ball_kick_target(&mr->entities.ball.pair, &a, &slot, input, &ctx, &mr->rng, mode, out);
```

Inside `fifa96_ball_kick_target` the native `0x7A490` nudge is modelled by
`fifa96_ball_pair_stage_tail` (`fifa96_ball_pairing.c:820`), which writes the
caller's `actor` — and the caller's `actor` is the **local `a`**, whose
`pos_x`/`pos_z`/`facing` are copied in from `tail_actor` at
`fifa96_ball_pairing.c:823..825`. **`match_kick_run` never copies `a` back to
`mr->record` or the entity pool** (`handlers.c:1100..1117` ends at
`fifa96_match_run_ball_receive`/`fifa96_ball_pair_clear`). So the port's record
`pos_x`/`pos_z` nudge, ball face and `0x7A490` anim are **dropped into a local**:
the register-level record place of (b) has no engine write-back.

**Confirmed:** (b) is a native **record** write on `param_1 = DAT_00158730` (the
live actor); the port's model is **local-copy-only** (`handlers.c:1034` +
`fifa96_ball_pairing.c:823..825`). The arm-0 ball place (a) is not modelled at
all (`fifa96_match_action_20` arm 0 does only the actor `match_sp_anchor` at
`handlers.c:5024`).

---

## 5. The port delta — `fifa96_match_action_20` vs the native body

`fifa96_match_action_20` is at `src/fifa96_engine/fifa96_match_handlers.c:5001`
(table row `0x20`, `:5149`, `fn == NULL`). Per arm:

| native | port (`handlers.c`) | delta |
|---|---|---|
| prologue `[0x14C114/118/11C]` | not written | missing (input-word seeding, FU210-L1) |
| prologue `[0x157A83]`, `timer89 += delta`, signed `stage92<3` | `:5013..5014` (`r->controlled=1`) | signedness `(int8_t)` noted latent — FU220-L6 |
| arm 0 `0x73DC4`/`0x700F4` (ball-camera `0x15774C/50/54`) | uses `mr->render.camera.pos_x/y/z` as the focus (`:5020..5022`) | missing (`0x73DC4`/`0x700F4`) |
| arm 0 `[0x15781D]=0`, `0x73E08` block commit | `match_sp_commit(r)` (`:5019`) — record-local only | **missing** the ball block + 22-record `0x8CF60` place + `[rec+0x1C]` |
| arm 0 target := camera triple, `z-=0xF0` | `:5020..5023` | ✔ |
| arm 0 actor place `0x79B6C` | `match_sp_anchor(r, target_x, 0, target_z)` (`:5024`) | partial (no face; anim from the next row) |
| arm 0 **ball place** `0x84FBC..0x84FDB` | **absent** | **missing — FU220-L1 (= FU210-L2)** |
| arm 0 `0x7876C` merge | `if (r->has_slot) r->helper_request=1` (`:5028`) | partial (gates approximated) |
| arm 0 `0x8CFAC([R],0x26,0)` + `0x8CFAC(ball,…)` | `record_anim_install(mr, e, 0x26, 0)` (`:5031`) — one actor record | **missing** the two 11-stride walks — FU220-L2 |
| arm 0 `0x4C324`/`0x4C374` | absent | missing (FU210-L1) |
| arm 1 gate (`0x4BEC8`/`0x7A028`) + `0x8F188` event | gate reduced to `global_5882a`+`timer89` (`:5041`); no event | missing (FU210-L1) |
| arm 2 | `:5046..5050` | ✔ |
| arm 3 slot `word[+6]` predicate | `has_slot` + `timer89<=0x78` (`:5053..5055`); `0x78A84` absent | partial (slot body missing) |
| arm 4 target := camera | `:5062..5064` | ✔ |
| arm 4 `lane>>16 > 0x40` gate | `:5065` | ✔ |
| arm 4 `0x78AA4` slot restore | absent | missing |
| arm 4 `0x7B9C4`→`0x7A490` record pos+anim | `match_sp_kick(mr, NULL, 0x40, 0)` (`:5068`) stages the ball block | **missing** the record `pos_x`/`pos_z` + anim write-back — FU220-L3 |
| arm 5 `0x79B1C` snap + `+0x44` | `:5075..5078` | ✔ |
| arm 6 `0x7DAB4` reset | `match_row_reset(mr, e)` (`:5082`) | ✔ |

The **exact missing writes** (verbatim): `[ball+0x4D]=0`, `[ball+0x55]=0xB10`,
ball `pos_x/y/z` + `anim`/`frame` via `0x79B6C`; the 22 team-record anim/frame
via `0x8CFAC` and the ball record's 11-stride anim walk; arm 4's
`[R+0x59] += [R+0x6B]>>17`, `[R+0x61] += [R+0x6D]>>17` (gated `word[R+0x69]>>16 <
0x60`) + `0x6E598` anim/frame; the `0x73E08` block commit and the
`0x73DC4`/`0x700F4` ball-camera writes; the `[0x14C11x]` input words; the
`0x4C324`/`0x4C374` sinks.

The coupled installer is ported (`fifa96_match_run.c`,
`fifa96_match_action_phase_1D` arm 0 `:6098..6118`): the `0x899A2` install maps
to `fifa96_match_entities_install(actor, phase, 0x20, 0)`, gated on
`mr->actor_1587d4`. On the live path `mr->actor_1587d4` stays `NONE`
(`fifa96_match_run_init`), so the install is a null no-op and no record carries
code `0x20` — the FU220-**L5** residual (the actor cell `[0x1587D4]` producer,
FU210-L3/FU194-L8).

---

## 6. Tape (M2) visibility — exact

**No change.** The M2 tape reaches no period-end completion: fresh
`tests/test_engine_m2.c:3523..3525` — *"The tape reaches no period-end
completion (the golden stays `441415fc…`), so the phase-19 producers are pinned
as the durable state-level cross-check"*. Wiring the missing writes therefore
moves **nothing** on the tape:

1. The row-`0x20` body `fifa96_match_action_20` is dispatched **0 times** on the
   live M2 run — no record ever carries code `0x20`. The only place code `0x20`
   is ever installed is the state-level rider `run_phase19_riders`
   (`tests/test_engine_m2.c:3661..3667`), which sets `mr.actor_1587d4 = 1`
   explicitly — a pure-C, ISO-independent rider that `print`s nothing, so the
   transcript is unaffected. Fresh `grep actor_1587d4` over
   `tests/test_engine_m2.c` = **one** hit, the rider fixture at `:3662`; the
   live driver never sets it.
2. Even the sit-8 → act-7 → phase-`0x1D` route (now producer-live, T2) does not
   reach a row-`0x20` install on the tape: the rider `:3627..3657` shows the act
   tick runs phase-`0x1D` arm 0 with `actor_1587d4 == NONE`, and asserts **no**
   record carries code `0x20` (`:3655..3657`).
3. `record_anim_install` and `match_sp_anchor` already write the same actor
   `pos`/`anim`/`frame` the port already writes; the missing writes are *inside
   the row body*, which never runs. So M2 stays byte-identical (`441415fc…`) and
   M1 (`09b726b7…`) is unmoved.

**Which fields would move IF the coupled route also landed (actor cell producer
ported) AND the tape reached a tie-break period-end:** exactly the four M2
render-read families on the records the body reaches —
`pos_x/pos_y/pos_z` (actor arm 0 `0x79B6C`; ball arm 0 `0x79B6C`; actor arm 4
`0x7A490`) and `anim_id` + `frame` (actor arm 0 `0x79B6C`; ball arm 0 `0x79B6C`;
the 22 team records + ball via `0x8CFAC`; actor arm 4 `0x7A490`). `hidden`
(`+0x9A`) does not move. Any such landing must `cmp`-pin the M2 tape first
(AGENTS.md golden rule).

---

## 7. Fixture — what a faithful fixture asserts

**Genuine ported writes only** (the existing body fixture
`tests/test_engine_match_handlers.c:968..1061` already does this):

* arm 0: `record.controlled == 1`; `record.stage92 == 1`; `record.timer89 == 0`;
  `record.pos_x == camera.pos_x`, `pos_y == 0`, `pos_z == camera.pos_z - 0xF0`;
  `record.anim_id == 0x26`; `target_x == pos_x`, `target_z == pos_z`.
* arm 4 (gate fail, `lane>>16 > 0x40`): stage held at `4`.
* arm 4 (advance): stage → `5`, `timer89 == 0`, `target_x/y/z == camera`;
  the kick stages `entities.ball.pair.vector` + `flags` (the ported
  `fifa96_ball_kick_target` effect).
* arm 5: snap `target := pos`; `row44 == 0` holds, `row44 != 0` → stage `6`.
* arm 6: `stage92 == 0xFF`.
* arm 2/3 gate boundaries; `stage92 == 7` head-only; `NULL` → `-FIFA96_ERR_INVALID`.

**A faithful fixture must NOT assert the genuine-but-unported native writes** —
the port does not perform them:

* the ball-record place — `entities.ball` `pos_x == 0`, `pos_z == 0xB10`,
  `anim_id == 0x26` (FU220-L1);
* the 22 team-record / ball 11-stride anim walk (FU220-L2);
* arm 4's `record.pos_x/pos_z += nudge>>17` and the `0x7A490` anim write-back
  (FU220-L3);
* the `0x73E08` block commit / `0x73DC4`/`0x700F4` ball-camera cell writes
  (FU220-L4/L5).

The M2 rider (`tests/test_engine_m2.c:3627..3668`) is the model for the coupled
installer: with `actor_1587d4 == NONE` assert **no** record carries code `0x20`;
with `actor_1587d4 = 1` assert `team[0].records[1].code == 0x20` and
`stage92 == 0` (the deferred install).

---

## 8. Residual legs

1. **FU220-L1 — the FU210-L2 ball-record place.** Native arm 0 (`0x84FBC..
   0x84FDB`) writes the actor team's ball record `[actor]+0x7A6`:
   `[ball+0x4D]=0`, `[ball+0x55]=0xB10`, then `0x79B6C` writes `ball`
   `pos_x=0`/`pos_y=0`/`pos_z=0xB10` + anim/frame. Unported in
   `fifa96_match_action_20` arm 0. Owner: the row-`0x20` port (FU210-L2).
   Confirmed a genuine record write (not a local).
2. **FU220-L2 — the `0x8CFAC` 11-stride anim walks.** Native arm 0 calls
   `0x8CFAC([R],0x26,0)` and `0x8CFAC(ball,0x26,0)` → `0x6E598` on the base
   record with `0x26` and on 10 stride-`0xB2` successors with `0`. The port
   collapses these to one `record_anim_install(actor, 0x26, 0)`.
3. **FU220-L3 — arm-4 `0x7A490` record write-back (local-copy-only).**
   `fifa96_match_action_20` arm 4 (`handlers.c:5068`) stages a **stack-local**
   `fifa96_ball_kick_actor a` (`handlers.c:1034`); the native `0x7A490`
   `pos_x/pos_z += nudge>>17` + `0x6E598` anim (record `param_1 =
   DAT_00158730`) lands in `a` (`fifa96_ball_pairing.c:823..825`) and is never
   copied back. Confirmed.
4. **FU220-L4 — arm-0 `0x73E08` block commit.** `0x73E08` runs
   `0x8C24C(&0x158830)` (ball block `0x15880C/14`) and `0x8CF60(&0x1588A4)` /
   `0x8CF60(&0x1590D9)` — placing all 22 team records via `0x79B6C` and
   dispatching each `[rec+0x1C]`. The port's `match_sp_commit(r)`
   (`handlers.c:5019`) writes only the actor record.
5. **FU220-L5 — the ball-camera cells + input words + sinks.** `0x73DC4`
   (`0x15774C/50/54 := (0,0,0x8D0)`) and `0x700F4` (camera reset) in arm 0; the
   prologue `[0x14C114/118/11C]`; `[0x15781D]`; the `0x4C324`/`0x4C374` sinks.
   The port uses `mr->render.camera` as the focus stand-in. (The FU210-L1 /
   OL-9 helper-port class.)
6. **FU220-L6 — prologue `stage92` signedness.** The prologue reads the stage
   signed (`SAR EAX,0x18`, `0x84F1D`) so `< 3` is a signed compare; the port
   uses `(int8_t)r->stage92 < 3` (`handlers.c:5013`). Benign while row `0x20` has
   no live dispatcher (only the arm-6 reset reaches `0xFF`), recorded for the
   coupled landing.

FU210-L2 (ball place) and FU210-L5 (count rule) remain open: this slice closes
them at the field level (FU220-L1 is FU210-L2's exact write; the row still stays
`NULL`/uncounted per FU210-L5 — nothing here changes the census).

---

## 9. Provenance (this slice)

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit; scripts disabled):

* `get_current_program_info` — `/FIFA96.EXE`, `watcom:LE:32:default`, image base
  `0`, `min_address 0x10000`.
* `read_memory` — `0x84ED0` (32 B, the 7-arm table).
* `disassemble_bytes` — `0x84EEC..0x85214` (808 B, the full 7-arm body, 208
  insns), `0x8CF60..0x8CF9E` (62 B, the `0x8CF60` record walk).
* `decompile_function` — `0x79B6C`, `0x7A490`, `0x7B9C4`, `0x8CFAC`, `0x6E598`,
  `0x6E490`, `0x79B1C`, `0x7DAB4`, `0x7876C`, `0x79C50`, `0x73DC4`, `0x700F4`,
  `0x73E08`, `0x8CF60`, `0x8C24C`, `0x4C324`, `0x4C374`, `0x78A84`, `0x78AA4`.

Engine/loader source read first-hand: `src/fifa96_engine/fifa96_match_handlers.c`
(`fifa96_match_action_20` `:5001..5084`; `match_sp_anchor` `:1472`; `match_sp_commit`
`:1490`; `match_sp_snap` `:1506`; `match_sp_kick` `:1554`; `match_row_reset`
`:2207`; `match_kick_run` `:1031..1118` with the local `a` at `:1034` and the
`:1100` call); `src/fifa96_engine/fifa96_match_run.c`
(`fifa96_match_run_record_anim_install` `:1001`; `fifa96_match_action_phase_1D`
`:6082..6145`, the coupled arm-0 install `:6101..6118`);
`src/fifa96_loader/fifa96_ball_pairing.c` (`fifa96_ball_pair_stage_tail`
`:151..209`; the kick's local-copy call `:820` + copy-back `:823..825`);
`include/fifa96_engine/fifa96_match_entities.h` (`:61..159`, the record field
map); `include/fifa96_loader/fifa96_ball_pairing.h`
(`fifa96_ball_stage_tail_actor` `:139..149`, `fifa96_ball_kick_actor` `:220..238`);
`tests/test_engine_match_handlers.c` (`:968..1091`, the row-0x20 body fixture +
the installer leg); `tests/test_engine_m2.c` (`:3523..3525` the no-period-end
tape note; `:3627..3668` the phase-19 rider + the `actor_1587d4` fixture at
`:3662`; `:3054` the phase-0x1D slot registration).

Docs cross-cited: `docs/ghidra/FU210_action_row20.md` (§2/§3/§4/§6/§7.3/§8),
`docs/ghidra/FU194_act_tail_1d_1e_20.md` (§3.2/§3.4/§6.1), FU-206 §5 (the M2
render-read map), AGENTS.md.

No writes beyond this slice file; no rename/label/comment/function/script/project
save; no git.
