# FU-162 — the aux-0x32 family's live selector (R5-L2) and the throw-in cut's neighbours (R5-L3)

Recon wave **R10** of the phase-13 second wave (FU-160..FU-166), read-only. All
Ghidra claims below are **fresh this session**: `disassemble_bytes`,
`decompile_function`, `read_memory`, `search_instructions`,
`search_byte_patterns`, `get_xrefs_to` on program **`/FIFA96.EXE`**
(case-sensitive; scripts disabled; flat LE addresses; LE globals at
`+0x100000`). Engine cites are the T1 HEAD (`d1d4696`) sources. No repo file
was modified by this slice besides the two R10 outputs.

**Answers in one line each.**

1. **R5-L2 — the aux-0x32 live selector is FOUND and live natively.** The
   rows `0x2C/0x2D/0x2E/0x5C` are the **keeper save-type animation ids**: the
   CPU keeper decision `FUN_00077EAC` (sole caller `0x78599`) latches
   `[0x157C2C] := {0x2C,0x2D,0x2E}` and installs **action row `0x1A`**; the
   row-`0x1A` body (`0x7662C`, handler-table `[0x1A]`) then runs the save
   state machine (`0x2D→0x5C`, `0x2E→0x54`) and installs the animation
   `FUN_0006E598(rec, word[0x157C2C], frame 0)` at `0x76863`. The installed
   row's aux-`0x32` frame fires the `FUN_0006E8E8` arm `[49] = 0x6FA8F`,
   which is a **camera cut to the keeper + pause + pan veto** — **not** a
   pan/follow producer (it zeroes the event state and suppresses
   `FUN_00071C94` via `[0x157A6C]`).
2. **R5-L3 — the throw-in neighbours: none of the dropped stores writes the
   camera; the restored Y cut is sufficient for the *place*.** The one
   dropped neighbour that *is* required for the throw-in path's **live
   camera chain** (place → ball → receive → follow) is the **ball staging
   `0x7A490(rec, &triple, face, mode 0xA, {-1,0})` at `0x85C56`** (unported,
   OL-62) — without it the throw-in never produces a ball vector and the
   `0x80` Y can never reach the receive arm's `event_param > 0xF0`. Two
   camera-relevant **divergences** were also found: the offset index source
   (`r->actor_type`, never written, vs the native face octant `[rec+0x8E]`
   = `r->type`), and the pick origin (`cam place` vs the native `0x157770`
   anchor).

---

## 1. Method

Fresh reads (all quoted below): `disassemble_bytes` `0x6FA70..0x6FAE0`,
`0x6E8E8..0x6E960`, `0x6FBD0..0x6FC20`, `0x6E5C0..0x6E612`,
`0x71C94..0x71EBF`, `0x7A4A0..0x7A4E0`, `0x858E4..0x85CB0`,
`0x4C380..0x4C3C0`, `0x766C0..0x767FF`, `0x76800..0x7684A`,
`0x76830..0x76870`, `0x78294..0x782D0`, `0x78200..0x782A0`,
`0x73E60..0x73E95`, `0x77400..0x77440`, `0x75CD0..0x75D00`,
`0x76830`, `0x76F40..0x76F95`, `0x86890..0x86920`, `0x86700..0x86892`;
`read_memory` `0x6E800` (232 B), `0x10E640` (96 B), `0x10EDBE` (48 B),
`0x10F08C` (27 B), `0x10F23C` (9 B), `0x10E160` (64 B), `0x10F328` (32 B),
`0x1103CB` (96 B), `0x1106E0` (320 B), `0x110520` (208 B), `0x77E98` (24 B),
`0x10E6EC/0x10E73C/0x10E78C/0x10E7DC` (8 B each);
`decompile_function` `0x765C4`, `0x7D9A4`; `search_instructions`
`CALL 0x0006e598` (84), `MOV EDX,0x2c/0x2e`, `[0x00157c2c]`, `[0x00157c2a]`;
`search_byte_patterns` `2c2d2e`, `3537393b`, `35363738`, `b25c`, `b22c`;
`get_xrefs_to` `0x8DF84` (7), `0x77EAC` (1), `0x7CA54` (1).
`get_function_by_address` `0x7CA54`, `0x8695C` (no function).

**Alias-trap note (resolved in place).** The stage-3 offset build disassembles
as `MOV EDX,[EAX+0x10F331]` / `MOV EAX,[EAX+0x10F339]` followed by
`SAR ...,0x18`; the `0x10F331/0x10F339` values are the **dword bases** whose
high bytes are the dir tables at `0x10F334`/`0x10F33C` (`dword[a]>>24` =
byte `a+3`). FU-157 §4.2's table names were right; a bare read of the
disassembly operand would have been a sixth alias strike. Fresh bytes
`0x10F328..0x10F347` = `00 00 00 00 | 00 00 00 00 | 00 00 00 00 | 01 01 01 00
ff ff ff | 01 01 00 ff ff ff 00 01 | 15 03 19 16` — i.e. the static triple at
`0x10F328/2C/30` `(0,0,0)`, then `0x10F334 = {0,1,1,1,0,-1,-1,-1}` and
`0x10F33C = {1,1,0,-1,-1,-1,0,1}`.

---

## 2. R5-L2 — the aux-0x32 family

### 2.1 The arm `[49] = 0x6FA8F` (fresh bytes, full store set)

`read_memory 0x6E800` (58 dwords) entry `[49]` = `8f fa 06 00` = `0x6FA8F`;
the switch index is `(chain_low - 1)` with `chain = word[rec+0x36]`
(`0x6E8F6 DX = [EAX+0xE]` where `EAX = rec+0x28`; `0x6E916 XOR AH,AH;
DEC EAX; CMP AX,0x39; JA exit` — FU-84 §1). Fresh `disassemble_bytes
0x6FA8F..0x6FAC2`:

```
0x6FA8F  PUSH 0x1                     ; flag: the pause argument
0x6FA91  MOV EDI,[EBP+0x61]           ; z
0x6FA94  PUSH EDI
0x6FA95  MOV EAX,[EBP+0x5D]           ; y
0x6FA98  PUSH EAX
0x6FA99  MOV EDX,[EBP+0x59]           ; x
0x6FA9C  MOV CL,[ESP+0x1c]            ; the byte-lane shift (see below)
0x6FAA0  PUSH EDX
0x6FAA1  MOV byte[EBP+0x9B],0x1       ; the has-ball/claim marker
0x6FAA8  MOV EDX,[ESP+0x18]           ; the anim-state pointer (rec+0x28)
0x6FAAC  CALL 0x700F4                 ; camera := rec.pos, flag 1
0x6FAB1  MOV EAX,0x32
0x6FAB6  MOV BX,[EDX+0xE]             ; word[rec+0x36] (the aux chain word)
0x6FABA  SHL EAX,CL
0x6FABC  XOR EBX,EAX                  ; clear THIS lane's aux
0x6FABE  MOV [EDX+0xE],BX
0x6FAC2  JMP 0x6FBE6                  ; the common frame-timer/advance tail
```

The tail (`0x6FBE6..0x6FBFF`, fresh): `EDX = [ESP+0xC]; SAR DX,8;
CL += 8; TEST DX,DX; JNZ 0x6E912` — the chain word is consumed **one byte
lane at a time** (low byte first, then `>>8`), each arm clearing its own lane
with `aux << CL`. So a multi-lane aux word fires several arms in sequence
(row `0x5C` f2 aux `0x243A`: first the `0x3A` jump arm, then `0x24`).

**Store set:** the FU-71 camera triple `0x15774C/50/54` (via `FUN_000700F4`),
the pause pair `[0x157820]`/`[0x157822]` (`FUN_000700F4`'s 4th-argument flag;
FU-157 §1 re-verified: `0x7021B MOV ECX,[ESP+0x28]`), the whole event state
zeroed (`0x1577F0/F2/F4/F8`, `0x1577BE/C0/C2`, `0x1577CA`, `0x157821` —
FU-157 §1's list), `byte[rec+0x9B] = 1`, and the aux-chain lane clear. No
other cell. The arm has **no** conditional branch of its own: it runs every
frame the resolved frame record carries aux low byte `0x32` (the frame-aux
driver is per record per frame, FU-153/FU-159).

### 2.2 The rows that carry aux `0x32` (fresh frame reads)

Row table entries (fresh `read_memory 0x10F08C`/`0x10F23C`; layout
`{id,last,flags,next,frame_table,bank}`):

```
row 0x2C  2c 03 00 27 50 e6 10 00 1f   frames @0x10E650 (4)
row 0x2D  2d 03 00 27 64 e6 10 00 20   frames @0x10E664 (4)
row 0x2E  2e 05 00 27 78 e6 10 00 21   frames @0x10E678 (6)
row 0x5C  5c 08 00 27 be ed 10 00 47   frames @0x10EDBE (9)
```

Frame records (fresh `read_memory 0x10E640`, `0x10EDBE`; `{dur u16, aux u16,
sprite u8}`):

| row | aux-0x32 frame | other aux lanes (low byte) |
|---|---|---|
| `0x2C` | f0 `{0x40, 0x0032, 0}` | f1..f3 none |
| `0x2D` | f0 `{0x40, 0x0032, 0}` | f1..f3 none |
| `0x2E` | f2 `{0x40, 0x0032, 2}` | f0 `0x10`, f4 `0x02` |
| `0x5C` | f1 `{0x40, 0x0032, 1}` | f2 `0x243A` (the jump), f3..f8 `0x24/0x03/0x05` |

All four chains end at `next_id = 0x27` (the terminal self-loop row).

### 2.3 The live selector — the keeper save-type chain (fresh, byte-cited)

**(a) The decision.** `FUN_00077EAC` (fresh; sole xref `0x78599` inside
`FUN_000782D0`, i.e. the CPU keeper band; also FU-74 §4 documents this body as
S7 at runtime `0x782xx`). Fresh gates quoted: `0x77EB7` byte
`[rec+0x91] == 0x19` (keeper type evidence), `0x77ED9 CALL 0x741B4` +
`[0x14C1D4 + idx*2] & 0x200` (input flag), `0x77EF6 [rec+0x9A] == 0`,
`0x77F03` current row id `!= 0x54`, `0x77F16` `word[rec+0x81] != 0` → copy
pos to out/return, `0x77F2E` `[0x157A83] != rec`/side checks, then the
behind-goal geometry (`0x795B4(rec+0x59, 0x157C48, &out)`, `|cam.x| < 0x420
&& |cam.z| > 0x7B0` + side/sign). Fresh jump table `0x77E98` (5 dwords) =
`{0x7826D, 0x78245, 0x7821D, 0x78294, 0x782B1}`; `0x78205 DEC ECX;
CMP CX,4; JA; EAX=CX; JMP CS:[EAX*4+0x77E98]`. Fresh arm bytes:

```
0x7826D  MOV EAX,0x2c               ; ECX=1
0x7827E  MOV [0x157C2C],EAX        ; the save-type latch
0x78285  CALL 0x7D9A4              ; EDX=0x1A, ECX=1, EBX=0
0x78245  MOV EDI,0x2d               ; ECX=2
0x78258  MOV [0x157C2C],EDI
0x7825e  CALL 0x7D9A4
0x7821d  MOV ESI,0x2e               ; ECX=3
0x78230  MOV [0x157C2C],ESI
0x78236  CALL 0x7D9A4
0x78294  MOV ECX,1; MOV EDX,0x1b; CALL 0x7D9A4   ; ECX=4
0x782b1  MOV ECX,1; MOV EDX,0x1c; CALL 0x7D9A4   ; ECX=5
```

`FUN_0007D9A4` (fresh decompile) is the action installer: stores
`rec+0x91 = code`, `rec+0x28 = handler_table[code]`, `+0x89=0`, `+0x9E=0`,
`+0x92 = stage`, `+0x7B = +0x79`, and invokes the body when the 4th arg `!= 0`.
So the decision latches the **save type** into `[0x157C2C]` and switches the
keeper to **action row 0x1A** (handler table `0x1106E0` entry `[0x1A] =
0x7662C`, fresh). The ECX value comes from the behind-goal branch
(`ECX=1` near-target / `ECX=4` the through-ball arm / `ECX=5` the
`[0x157C4C] >= 0x100` arm / `ECX = trajectory class` from the fresh tables
`0x10E184` (`{1,2,2,3,3,5,5,5,...}`) + `0x10E1C4`/`0x10E1CE` lookup at
`0x780E2..0x781E0`; FU-74 §4's census). `FUN_000765C4` (fresh decompile:
tracked-teammate predicate) returning non-zero diverts to action `0x1C`
(`0x781F6..0x782BB`).

**(b) The install.** Action row `0x1A`'s direction machine (fresh
`0x766C0..0x76870`; FU-79 §3.1/§3.2 documents it): after the state advances

```
0x766FC  if ([0x157C2C]==0x2D &&
            [0x1577EE].hi < 0x60 && [0x1577BC].hi > 0x12 &&
            [0x1577F8].hi < 0x1E && |rec.pos_z| < 0xA90)
0x76742    ESI=0x5C; 0x7674C MOV [0x157C2C],ESI; sound 6
0x76757  if ([0x157C2C]==0x2E &&
            [0x1577BC].hi > 0x14 && [0x1577F8].hi < 0x1C &&
            [0x157750] < 0x90 && (RNG&0x1F) > ([rec+4]+0x10>>24))
0x7679F    EDX=0x54; 0x767A9 MOV [0x157C2C],EDX; sound 6
          else 0x767B6: base = min((int16)([0x157750]-0x70),0x60);
                  ramp(base) -> word[rec+0x85], word[rec+0x87]=0
0x76816  (any state except 0x54) the face + install block:
          0x79C50(rec, -word[0x1577C2], -word[0x1577C0])
          0xCD474(rec[+0x6B].hi, rec[+0x6D].hi) -> word[rec+0x7F]
          EDX = word[0x157C2C]  (0x76846/0x7685E)
          EBX = 0, ECX = byte[rec+0x8E]   (0x76855/0x7685B)
0x76863  CALL 0x6E598(rec, EDX, frame 0)
0x76868  if row id == 0x54: RNG-scaled camera-velocity shake
          (0x1577C0/C2 = -(vel >> rng(0..3)); 0x15774C/54 += vel;
           0x8DC68 metric -> word[rec+0x71]; 0x7DAB4 reset)
```

So the animation id installed **is `word[0x157C2C]`** for the states
`{0x2C,0x2D,0x2E,0x5C}` (the `0x54` state resets instead). The keeper then
rides the installed row: `0x2C/0x2D` fire the aux-`0x32` arm at f0,
`0x2E` at f2, `0x5C` at f1 (then its `0x243A` jump lane at f2) — the camera
cut fires **per frame the aux frame holds** (duration `0x40`).

**The `[0x157A6C]` install side-effect.** Great care is needed here — the
*fresh* installer head `0x6E5C0..0x6E612` shows: when `[rec+0x8D]==0` and
`side == [team+0x826]` (`side = ([0x157754] > 0)`, `0x6E5A9`,
`0x6E5C8 JNZ 0x6E608`), `(int16)DX ∈ {0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C}`
→ `0x6E5F7 MOV byte[0x157A6C],1` (bytes `c6056c7a150001`), else
`0x6E600 XOR CL,CL; MOV [0x157A6C],CL` — **every install re-derives the
flag** (FU-84 §1's special set, first-hand re-verified; FU-151 §2.9 calls
these the keeper "event codes").

### 2.4 What the chain does to the camera — and the pan veto

* **The cut:** `FUN_000700F4(rec.pos_x, rec.pos_y, rec.pos_z, flag=1)` —
  camera place := the keeper's position (Y included, unlike the throw-in's
  `0x80` override), **pause on** (`[0x157820]=1`), `byte[rec+0x9B]=1`, event
  state zeroed.
* **The veto (fresh):** `FUN_00071C94` head `0x71C99 CMP byte[0x157A6C],0;
  0x71CA0 JNZ 0x71DEF`; `0x71DEF..0x71DF2` = `POP EBP; POP EDI; POP ESI;
  RET`. So while a special-row install holds the flag, **every pan/event set
  through `FUN_00071C94` is a no-op** (the row-04 half-line pan, the keeper
  release event, the whole 11-site family).
* **The second veto:** `FUN_0007A490` (the ball staging) fresh
  `0x7A4BF CMP byte[0x157A6C],0; 0x7A4C6 JNZ 0x7AE2F` — the staging's
  `0x158738` triple copy is skipped too.
* **The `0x54` shake:** with `[0x157C2C]==0x54` and the current row id `0x54`
  (`0x76872`), the body RNG-jitters `[0x1577C0]/[0x1577C2]` and adds them to
  `0x15774C/54` (`0x76880..0x768FA`) — a direct camera *velocity* write
  (then the record resets).
* **The camera→jump coupling:** the `0x2E` else arm (`0x767B6..0x767E7`)
  derives the keeper's jump base from the camera Y
  (`ramp(min([0x157750]-0x70, 0x60))`).

**Verdict: NOT a producer for the pan/follow chain — it is a cut plus a pan
veto.** `FUN_000700F4` zeroes `[0x1577BE]` (speed, the integrator gate) and
the event height, so neither `FUN_000736AC`'s integration nor
`FUN_00071DF4`'s follow (gate `[0x1577EE].hi > 0xF0`) can run from it; and
the `[0x157A6C]` veto actively kills pan events for the save's duration. Its
live camera effect is the *place* (cut to the keeper), re-applied per aux
frame, plus the pause.

### 2.5 The `timer81` negative (bounded)

`word[rec+0x81]` is **not read or written anywhere in this chain**: the arm's
full store set is §2.1; `FUN_000700F4`'s store list does not include it; the
row-`0x1A` direction machine writes `rec+0x7F/0x85/0x87` but not `+0x81`.
The only `word[+0x81]` consumer found is `FUN_0007CA54` (fresh xrefs = 1,
caller `0x8DB4D` in `FUN_0008D8EC`, the team AI pre-pass): it decays the word
by `[0x157A64]` and zeroes it at expiry (`0x7CA5C CMP; 0x7CA78 MOV 0;
0x7CA83 SUB`; the port already names this the "0x7CA54 input-row machine",
`fifa96_match_handlers.c:2839`). Also `0x77F16` in the decision reads
`word[rec+0x81] != 0` as a guard — a read only. No timer81 block is part of
the aux-`0x32` camera path.

### 2.6 Engine status (HEAD `d1d4696`)

* `match_run_record_jump_arm` (match_run.c:1018) handles aux `0x0E/0x3A/0x0F`
  only — aux `0x32` falls to the `default` and does nothing.
* The action table has **row 0x1A = NULL** — "not ported (partial);
  keeper_reposition_a_gate; OL-34" (`fifa96_match_handlers.c:2872`), so the
  installer can never run.
* `grep -rn 157C2C src/ include/` = **no hits** — the save-type latch is not
  modelled; `fifa96_keeper.c` (loader) has no save-type chain (fresh grep:
  only `fifa96_keeper_dive_target`).
* No `[0x157A6C]` cell exists engine-side (no setter, no veto).

So engine-side the aux-`0x32` family is entirely dark: no selector, no
install, no arm.

### 2.7 Errata candidates (quoted, first-hand)

1. **FU-79 §3.2 install call.** "`FUN_0006E598(rec, [0x57C2A]>>16, type8, 0)`"
   — the third argument is **not** the frame: fresh `0x76855 XOR EBX,EBX`
   sets the frame register to 0; the octant lives in `ECX` (`0x7685B
   SAR ECX,0x18`), which `FUN_0006E598` overwrites (FU-84 §1: "ECX is not an
   input"). The install is `(rec, word[0x157C2C], frame 0)`.
2. **FU-151 §2.9 naming.** The `{0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C}`
   set are not generic "event codes": fresh §2.2/§2.3 show them as the
   **animation row ids** the keeper save machine installs (the `0x32` aux is
   the camera-cut lane of four of them; the `0x0A`/`0x3A`/etc. lanes are the
   jump/land arms — rows `0x35/0x37/0x39/0x3B` f0 aux `0x0A`, fresh
   `0x10E6EC/0x10E73C/0x10E78C/0x10E7DC` = `{0x40,0x000A,0x00}`).

---

## 3. R5-L3 — the throw-in cut's neighbours

### 3.1 The restored cut, re-verified (fresh `0x858E4..0x85970`)

The T1 restore is exact: `0x85911 MOV EAX,0x80`; `0x85916..0x85918` MOVSD
x3 (`rec+0x59` → `0x15774C`); `0x85919 MOV [0x157750],EAX`; the offset build
`0x8591E..0x85960` reads the **face octant** `[EBP+0x8B]>>24` = byte
`[rec+0x8E]` (`0x8591E/0x85924`) and indexes the dword bases whose high bytes
are `0x10F334`/`0x10F33C` (`0x85929/0x8592F`, alias trap §1); the cut pushes
`(x+dx<<6, [0x157750]=0x80, z+dz<<6)` and `PUSH 0` (flag **0**, no pause) at
`0x85927`, `CALL 0x700F4` at `0x8596B`. The port's shape
(`handlers.c:1471`) is faithful **except for the index source** (§3.3.A).

### 3.2 The dropped neighbours (fresh, byte-cited)

* **(A) `0x858E4 CALL 0x4C380`** — fresh `0x4C380 = C3` (`RET`): a genuine
  1-byte no-op. Cosmetic; dropping is exact.
* **(B) `0x858E9..0x858F2`** — the frame gate (`[rec+0x3D] < 4` → return).
  Ported (`handlers.c:1455`).
* **(C) `0x85970..0x85A14` — the team/offside block.** `[ESP+0x14] = 4` (a
  local relay code); `team = [rec]`; if `team[+0x7E7] != 0`: index
  `i = team[+0x7E7]`, copy the 12-byte triple `team[+0x7E8 + 12*i]` to the
  stack, side-gate `[ESP+8]` vs `±0xB10`; the out case writes
  **`[0x1587B4] = 7`** (`0x859D6`, bytes `b307`/`881db4871500`) and clears
  `team[+0x7E7]`; the hold case increments `team[+0x7E7]` (zeroed at > 2).
  Junction `0x85A14`: `AL = [0x1587B4]; DEC AL; if > 6 exit;
  JMP CS:[EAX*4 + 0x855D4]` (the 7-arm table FU-149 §7 line 638 lists).
* **(D) `0x85A30..0x85B68` — the target/vector arms.** `0x85A30` copies the
  `0x15879C` triple; `0x85A3F` slot arm (`[rec+0x20]`, `0x78AA4`,
  slot dirs with the `0x10F334/0x10F33C` fallback); `0x85AA5` builds a lead
  point from `[0x1587AC]` (`pos + [rec+0x71/0x73].hi<<5`, relay code 8 when
  `[0x1587B4].hi == 5`); `0x85AF0` the `±0x3C0/±0xB10` clamp + the
  `0x92AC8 % 0x2D0` jitter. All arms write only the stack triple and the
  local relay code.
* **(E) `0x85B68..0x85C45` — the vector build.** `0x8DCD4(rec+0x59, [ESP],
  [ESP+0xC])` (the lane metric); band clamp `0xF0..0x5A0`
  (`0x85B76..0x85B9B`); `0x8DD70` angle; the `0x114E04` sine fold +
  `0x795A4` scaling; then **`ADD EDX,[0x157750]` (`0x85C35`) and
  `0x79C50(rec, dx, dz + camY)` (`0x85C45`)** — the camera Y enters the
  facing vector; and **`0x92820(rec, 3)`** (`0x85C4A/0x85C51`).
* **(F) `0x85C56..0x85C66` — the throw launch.** `PUSH 0; EBX=(int16)CX;
  ECX=0xA; PUSH -1; EDX=&triple; EAX=rec; CALL 0x7A490` — the ball staging
  of the throw (mode `0xA`), the **only** ball producer in the row-0x10
  body. Fresh `0x7A490`'s own `[0x157A6C]` gate (§2.4) governs it.
* **(G) `0x85C6D..0x85C96` — the receiver pick.** `0x8DE8C(EAX=0x157770,
  EDX=team, EBX=0, ECX=0)` (the nearest walk from the **`0x157770` camera
  anchor triple**), `[rec+0x9A] = 1` around the call, then
  `[team+0x7B2] = pick`.
* **(H) `0x85C9C` — `MOV EDX,4; CALL 0x7D9A4`** with **EAX = the pick**
  (the `0x8DE8C` return is not reloaded) — installs the pick record with
  action code 4, stage 1, no invoke. (Record bookkeeping, no camera cell.)

### 3.3 Port comparison and the camera verdict

| native site | port HEAD | verdict |
|---|---|---|
| `0x858E4` `0x4C380` | dropped, "no-op (leg)" | **exact** — `RET` |
| `0x858E9` frame gate | `r->frame < 4` (`:1455`) | ported |
| `0x8591E` index source | `r->actor_type & 0x1F` (`:1464`) | **DIVERGENCE (camera-relevant)** |
| `0x85911..0x8596B` Y cut | `fifa96_camera_init(..., 0x80, ...)` (`:1471`) | ported (T1) |
| `0x85970..0x85CA1` | dropped (`:1474` comment) | see below |
| `0x85C6D` `0x8DE8C` origin | `match_sp_pick_install(..., cam_x, cam_z)` (`:1476`/`:1363`) | derived; origin divergence |
| `0x85C9C` install 4 | installs the **pick** with 4 (`:1370`) | **faithful** (EAX = pick) |

**A. The index divergence (fresh + port read).** The native indexes the
`0x10F334/0x10F33C` tables with the **live face octant** `byte[rec+0x8E]`
(the byte `FUN_00079C50` maintains, 0..7). The port indexes with
`r->actor_type` (`:1464`), staged from `e->actor_type` (`match_run.c:1116`)
— and a full-tree grep finds **no writer of `e->actor_type`** (the struct's
own comment says "the pool never writes it (OL-83)"). So in the live engine
the index is always 0 → the cut offsets are always
`(dir_x[0]<<6, dir_z[0]<<6) = (0, 0x40)` regardless of the taker's facing,
while the port has `r->type` = the native `+0x8E` byte (maintained by
`fifa96_arm_face`, `fifa96_match_entities.c:511`). The restored cut is
camera-relevant: the place differs by up to `±0x80` from native whenever the
taker's octant ≠ 0. (The T1 unit expectation `(0x110, 0x440)` holds only for
octant 0.)

**B. The dropped block, camera-classified.**

* **(C)/(D)** the team block and the `0x855D4` arms write **no** camera cell
  (stack triples, `[0x1587B4]`, `team+0x7E7`, the `0x15879C`/`0x1587AC`
  reads). Not required for the *place*. They **are** required for the
  native throw's target/relay state (`[0x1587B4]` also feeds the `0x855D4`
  arms and, via the latch, the delivery machine); the port's
  `match_sp_probe_85498` writes `mr->sp_delivery` (`:1314`) but **nothing
  reads it** (fresh grep: only the two zero-writes in `match_run.c:1753/1836`)
  — the ported latch dead-ends.
* **(E)** `0x92820(rec,3)` (port OL-27 sink) and the `0x8F188(0x18, rec, 4)`
  event at `0x858F8..0x85909` (the port's stage-3 comment names it but the
  derived subset emits neither) — event/audio sinks, no proven camera write.
  `0x8DD70`/the sine fold/`0x795A4` build the launch vector.
* **(F) THE required drop for the throw-in's live camera chain.** The
  `0x7A490` staging is the row-0x10 body's only ball producer; the FU-157 §5
  chain (ball → receive arm → `event_param > 0xF0` + tracked → the follow)
  cannot start from a throw-in without it. It is unported (OL-62); the
  restored `Y = 0x80` therefore currently ends at the cut.
* **(G)** the pick origin: native `0x157770` (the FU-71 anchor[1] triple,
  copied at the `FUN_000700F4` reset) vs the port's just-written camera
  *place* — a fidelity divergence (the two coincide only right after a
  reset), not a starvation.

**C. The verdict.** Around the restored `0x85911..0x8596B`, **no dropped
store writes the camera triple, the pause cell, or the event state** — the
T1 Y cut is complete for the *place* as delivered. The neighbours are
(i) camera-*in*-fluencing **reads** (`[0x157750]` into the face vector at
`0x85C35`; `0x157770` into the pick), (ii) the **launch** (`0x7A490`, the
downstream-chain requirement), and (iii) record/team/event bookkeeping plus
one confirmed no-op (`0x4C380 = RET`).

---

## 4. Numbered legs (new)

* **FU162-L1 — the keeper save-type state machine's live input mapping.**
  Which ball states drive the decision's `ECX` to 1/2/3 (the three save anims
  `0x2C/0x2D/0x2E`) vs 4/5 (actions `0x1B`/`0x1C`) — the fresh trajectory
  tables `0x10E184`/`0x10E1C4`/`0x10E1CE` and the gauge `[0x157C44]` enter
  FU-74 §4's census; the exact game-state correspondence is not derived.
* **FU162-L2 — the engine port of the aux-0x32 chain (keeper scope).**
  `match_run_record_jump_arm` case `0x32` (place+pause+`+0x9B`), the
  `[0x157C2C]` latch, action row `0x1A` (NULL at HEAD), and the `[0x157A6C]`
  veto in the event setter and the ball staging. Not required for the
  open-play live camera (FU-157 §5); bundle with the keeper save machine.
* **FU162-L3 — the throw-in offset index.** `handlers.c:1464` must use the
  live face octant (`r->type`, native `[rec+0x8E]`) instead of the
  never-written `r->actor_type` (OL-83 overlap); camera-place relevant.
* **FU162-L4 — the throw launch.** `0x7A490(rec, &triple, face, 0xA, {-1,0})`
  at `0x85C56` (port OL-62): the throw-in path's only ball vector; the
  prerequisite for any live throw-in → receive → follow chain.
* **FU162-L5 — the throw target/relay machine.** The `0x85970..0x85B68`
  block (`[0x1587B4]` latch, `team+0x7E4/0x7E7` triple array, the `0x855D4`
  arms, the `0x8DCD4`/clamp/angle build) — the source of the launch triple;
  and the dead-ended ported `sp_delivery` consumer.
* **FU162-L6 — the pick origin.** Native `0x157770` anchor vs the port's
  camera place in `match_sp_pick_install` (`handlers.c:1363/1476`).

## 5. Provenance (this slice, fresh)

Ghidra MCP read-only, program `/FIFA96.EXE`: the §1 read list. Engine reads:
`fifa96_match_handlers.c` (860-1000, 1280-1512, 2872), `fifa96_match_run.c`
(960-1071, 1090-1134, 1753/1836), `fifa96_match_entities.c` (500-515),
`fifa96_match_entities.h` (55-114), `fifa96_match_run.h` (435-480),
`fifa96_keeper.c` (loader, grep). Doc cross-refs: FU-74 §4, FU-79 §3,
FU-84 §1, FU-140 line 88, FU-149 §7 (line 638), FU-151 §2.7/2.9, FU-153,
FU-157 §1/§4/§5/§7, FU-159 §3.

Carried: FU-96 §7 (anchors), FU-71 (camera triple), FU-70 (record layout).

---

## Status append — T3 implementation (phase-13, 2026-10-11)

**FU162-L3 closed.** `fifa96_match_handlers.c` (row 0x10 stage 3) now indexes
the 0x10F334/0x10F33C offsets with the live face octant `r->type` (native
`byte[rec+0x8E]`, fresh `0x8591e MOV EAX,[EBP+0x8b]; SAR EAX,0x18`) instead of
the never-written `r->actor_type` (OL-83). `test_live_throw_in_camera_y_cut`
was re-derived per octant (all eight octants, with `actor_type` deliberately
mismatched; mutation-RED verified against the old index) — the cut's dx/dz are
now faithful for a real taker's facing. The port still masks to the 32-entry
derived tables (the existing hardening divergence).

**Not landed (unchanged):** FU162-L1 (the save-type input mapping), FU162-L2
(the aux-0x32 keeper chain — keeper scope), FU162-L4 (the `0x7A490` throw
launch — the live throw-in -> receive -> follow prerequisite; OL-62), FU162-L5
(the throw target/relay machine + the dead-ended `sp_delivery`), FU162-L6 (the
pick origin `0x157770` vs the camera place).
