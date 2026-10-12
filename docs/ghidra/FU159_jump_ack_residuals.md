# FU-159 — the jump/ack residuals (R7, phase-13 recon)

**Question (phase-13 plan R7).** What do the jump/ack residuals need to make the
live sequence **repeatable end-to-end** (long-ball -> airborne receive ->
descending-window kick -> band-2 follow, across possessions)?

1. **T1-L1** — the `FUN_0006E8E8` jump arm's remaining pieces (FU-153 §10-11):
   the arm's velocity/sine/move writes now that the ramp is the `+0x83`
   constant; the **live row selector** that reaches the jump-aux rows
   `0x5E`/`0x12`/`0x14`/`0x5C`; the `0x6EE90/0x6EE92` snap (`FUN_00079B1C`).
2. **T1-L2** — the row-07 stage-2 `+0x44` ack producer: the native writer(s)
   of `byte[rec+0x44]` and under what conditions, enabling a second kick per
   possession.

**Status: derived with byte cites.** The byte44 producer is **found and
live-reachable natively** (the row installers, `0x6E6CC`/`0x8DFC1`) — the
engine merely never runs it (`s.byte44`/`s.row44` staged 0). The jump arm's
remaining writes are the *impulsion* (a `0x20` facing step at aux `0x3A`; the
speed/direction words at aux `0x0E`) plus the case-`0x0F` `FUN_00079B1C` snap
and its `pos_y != 0` frame-3 resolve — none required for the height arc
(`pos_y` is driven solely by `+0x85/+0x87`), all required for native-exact
motion and **repetition**. The live row selector: rows `0x5E` and `0x12` have
identified installers inside the **unported** row-`0x0C`/row-`0x0E` action
bodies (`0x81DA4`, `0x827D0`); rows `0x14`/`0x5C` have **no identified
installer** in this slice (leg FU159-L2). Repetition additionally needs the
row-successor chain (`0x5E -> 0x0F -> 0x00`) because byte44 is a *row-flags*
byte: only a terminal row (flags bit 0) lets the kick-07 stage-2 reset fire.

All Ghidra reads in this slice are fresh, program **`/FIFA96.EXE`**
(case-sensitive), flat LE-image addresses, standard MCP tools only; the GO
program reports one overlay space (`.image`, unused below — all cites are the
`ram` space).

---

## 1. Method

* `disassemble_bytes` of the three arm ranges (fresh, quoted §2):
  `0x6ED00..0x6EDC3` (aux `0x3A`), `0x6EEDC0..0x6EE90` (aux `0x0E`),
  `0x6EE75..0x6EEBE` (aux `0x0F`), plus the `FUN_0006E8E8` cases read from the
  full `decompile_function 0x6E8E8`.
* `decompile_function` of `0x6E598` (row installer), `0x6E490` (frame
  resolver), `0x79B1C` (snap), `0x795A4` (scale helper), `0x8DC68` (metric),
  `0x8DF84` (pool row installer), `0x8E008` (pool advance/successor),
  `0x8E5BC`/`0x8E910` (kickoff placements).
* Fresh `get_xrefs_to 0x6E598` (**84 sites**), `0x8DF84` (7 sites), and a
  full-program `search_instructions` sweep for **every byte access at
  `[reg+0x44]`** (214 matches) to census the byte44 writers/readers — the
  fresh-sweep requirement.
* `read_memory` of the native row table rows (`0x10F236` window), the four
  jump rows' frame tables (`0x10E3E6`, `0x10E48A`, `0x10E4BC`, `0x10EDBE`),
  the movement tables (`0x110500`), the direction tables (`0x10F334`,
  `0x10F33C`), and the row-handler table (`0x1106E0`).
* A read-only scan (`/tmp/opencode/r7/scan.py`) over the **ported** row and
  frame-record tables (`src/fifa96_engine/fifa96_match_run.c`) to enumerate
  every row whose frames carry the jump auxes — the four-row claim reproduced.
* Call-site id census: every direct `FUN_0006E598` call in the action-body
  range was disassembled at its site (the id is in `EDX` in the Watcom
  register convention); all ids are listed in §3.3.

---

## 2. T1-L1a — the jump arm's remaining writes (byte-level)

The arm runs inside `FUN_0006E8E8` (the per-record animation event machine,
native call position `0x7CD30`, engine `match_run_record_jump_arm`). The
switch index is `(aux & 0xFF) - 1`; the three arms and their **full store
sets** (fresh disassembly; the alias `dword[+0x81]>>16` = word `+0x83` is the
FU-153 §11 erratum and is respected):

### 2.1 aux `0x3A` -> case `0x39` (`0x6ED08..0x6EDE1`)

```
0x6ED08  MOV word[EBP+0x83], 0x18          ; the marker (ramp argument)
0x6ED11  MOV EAX, dword[EBP+0x81]; 0x6ED17 SAR EAX,0x10   ; = word[+0x83] = 0x18
0x6ED1A  CALL 0x702F8                      ; ramp(0x18)
0x6ED1F  MOV word[EBP+0x85], AX            ; jump base   \  = 18
0x6ED26  MOV word[EBP+0x87], AX            ; jump timer  /
0x6ED33  MOV dword[EBP+0x5D], 0x18         ; pos_y = 0x18
0x6ED3A..0x6EDB0  the facing-relative STEP:
         dir octant = sign-extended byte at +0x7E (read through the
         dword[+0x7B] alias: MOV/SAR 0x10 + the SHL AH,7/SBB fold);
         EAX = dword[EAX*4 + 0x114E04]      ; the quadrant sine table
         0x6ED5E  CALL 0x795A4(0x20, value) ; (0x20*v + 0x8000) >> 16
         0x6ED64..0x6ED6F  pos_x -= result  ; [+0x59]
         0x6ED9C  CALL 0x795A4(0x20, value) ; second component (octant+1)
         0x6EDA2..0x6EDB0  pos_z -= result  ; [+0x61]
0x6EDB3  CALL 0x974DC                      ; (sink; FU159-L3)
0x6EDB8  MOV EAX,0x3A ... 0x6EDC3 XOR word[EDX+0x0E],AX   ; the +0x36 aux chain
0x6EDC7..0x6EDD7  FUN_0008F188(0x62, rec, 4)              ; event sink
0x6EDDC  MOV EAX,0x27; 0x6EDE1 JMP 0x6FBDC                ; continue the chain
```

Effect: the `0x18` jump plus a **one-time ±0x20 displacement opposite the
facing** (the hop), then the event/chain tail.

### 2.2 aux `0x0E` -> case `0xD` (`0x6EDE6..0x6EE70`)

```
0x6EDE6  MOV word[EBP+0x83], 0x10          ; marker
0x6EDEF/0x6EDF5  SAR dword[+0x81],0x10; CALL 0x702F8   ; ramp(0x10)
0x6EDFD  / 0x6EE04  word[+0x85] = word[+0x87] = AX     ; = 15
0x6EE0B  EAX = byte[+0x8E] (facing octant, via dword[+0x8B]>>24)
0x6EE1B  MOVSX AX, byte[EAX+0x10F334]; 0x6EE23 SHL EAX,2
0x6EE26  MOV word[EBP+0x73], AX            ; vel component (dir_x[type]<<2)
0x6EE33  MOVSX AX, byte[EAX+0x10F33C]; 0x6EE3B SHL EAX,2
0x6EE3E  MOV word[EBP+0x75], AX            ; vel component (dir_z[type]<<2)
0x6EE14  MOV dword[EBP+0x5D], 0x10         ; pos_y = 0x10
0x6EE42..0x6EE52  CALL 0x8DC68(word[+0x73], word[+0x75]) ; the metric
0x6EE5B  MOV word[EBP+0x71], AX            ; speed = metric(dir_x<<2, dir_z<<2)
0x6EE5F..0x6EE6C  +0x36 ^= 0x0E<<CL        ; aux self-continuation
0x6EE70  JMP 0x6FBE6
```

No position write in this arm — it is the **impulsion** (speed + direction
words) the mover consumes. First-hand direction tables (both 16 bytes read):

* `0x10F334` (dir_x, octants 0..7): `00 01 01 01 00 FF FF FF` = {0,1,1,1,0,-1,-1,-1}
* `0x10F33C` (dir_z, octants 0..7): `01 01 00 FF FF FF 00 01` = {1,1,0,-1,-1,-1,0,1}

`FUN_000795A4(scale, value) = (scale*value + 0x8000) >> 16` (decompile: a
`longlong` product rounded by `0x8000`). `FUN_0008DC68` is the FU-147 §3.3
metric (abs folding: `d = a + b`, with `+ a/2 + a/4` refinement) already ported
as `fifa96_entity_distance` — so `speed = 4` (axis) / `5` (diagonal) for the
`<<2` direction words.

### 2.3 aux `0x0F` -> case `0xE` (`0x6EE75..0x6EEBE`) — the snap + relaunch

```
0x6EE75  CMP dword[EBP+0x5D], 0
0x6EE79  JZ 0x6EE90                       ; pos_y == 0 -> the relaunch path
          --- pos_y != 0 arm ---
0x6EE7B  MOV EBX, 3                       ; frame argument = 3
0x6EE80  MOV EDX, [ESP+8]                 ; rec+0x28
0x6EE84  MOV EAX, EBP
0x6EE86  CALL 0x6E490                     ; FUN_0006E490(rec, rec+0x28, 3)
0x6EE8B  JMP 0x6FBE6                      ; frame := 3 (clamped by the resolver)
          --- pos_y == 0 arm ---
0x6EE90  MOV EAX, EBP
0x6EE92  CALL 0x79B1C                     ; the SNAP (bytes e885ac0000)
0x6EE97  CMP word[EBP+0x85], 0
0x6EE9F  JNZ 0x6EEB7
0x6EEA1  MOV AX, word[EBP+0x5D]; ADD EAX,0x1C; CWDE
0x6EEAB  CALL 0x702F8                     ; ramp(pos_y + 0x1C)
0x6EEB0  MOV word[EBP+0x85], AX           ; jump base = ramp(0x1C) = 18? (see FU159-L5)
0x6EEB7  MOV dword[EBP+0x5D], 0           ; pos_y = 0
0x6EEBE  MOV word[EBP+0x87], 0            ; jump timer = 0
```

**The snap** (`FUN_00079B1C`, decompiled fresh — full store set):

```
target triple [+0x4D/+0x51/+0x55] := position [+0x59/+0x5D/+0x61]
word[+0x69] = 0            ; lane
word[+0x71] = 0            ; speed
byte[+0x9C] = 0            ; body timer
word[+0x67] = word[+0x65] = word[+0x69]   ; lane x/z
word[+0x75] = word[+0x73] = word[+0x71]   ; velocity words
```

**Answers to the task's two sub-questions.**

* *Are the writes required for the live jump to carry through?* The **height**
  arc needs none of them: `match_run_record_jump_step` reads only
  `+0x85/+0x87` (FU-153 §10). The **motion** needs them: the mover consumes
  `speed71`/`vel73`/`vel75` (engine `match_run_record_mover`, match_run.c:504-548
  — the same words), so without them the jump rises in place and the native
  facing-relative carry is absent. The `0x3A` step changes `pos_x/pos_z` by at
  most `0x20 * sin` (tiny) — it is the "hop", not a teleport.
* *What do the remaining native writes do?* They are the jump's impulsion
  (`0x0E` arm: speed/direction), the hop (`0x3A` arm: `0x20` facing step), the
  snap (`0x0F`: zero target delta, lane, velocity, body timer) and the
  `pos_y != 0` resolve (`0x0F`: frame := 3, which stops the relaunch re-arming
  mid-air).

---

## 3. T1-L1b — the live row selector

### 3.1 The row machinery (first-hand)

* **Row install** = `FUN_0006E598(rec, id, frame)` (decompile + disasm
  `0x6E640..0x6E6FF`): clamps `id` (`<0 || >0x6E -> 0`), stores the row pointer
  `rec+0x28 = [0x157588] + id*9`, derives the flag bytes from the row's `+2`
  flags, resolves the frame (`FUN_0006E490`, frame index -> `rec+0x3D`,
  frame-record pointer `rec+0x2C`, aux word `rec+0x36 = word[frame+2]`) and
  zeroes `rec+0x32`. `[0x157588]` is the runtime row-table base (the ported
  `match_run_anim_rows` = its image content at `0x10EF00`).
* **Row successor** = the tail `0x6FC05..0x6FE69` of `FUN_0006E8E8` (and its
  pool twin `FUN_0008E008` for the `0x159914` records, decompiled §4.2): on
  frame-duration expiry, **`if (byte[rec+0x44] == 0)`** the successor is the
  **row's `next_id`** (`row+3`; `0x6F` = keep current row; `>= 0x70` = mirror
  face via `0x79C50` then install `next_id - 0x70`); **else** (terminal row)
  the successor comes from the movement/steering tables at `0x11052A`/`0x1105AA`
  via `FUN_0006E330`. `0x6FBE6`/`0x6FE69` bump the frame timer
  (`+0x32 += [0x157A62].hi * 0x10`).
* The frame-record `byte+3` is the **high byte of the aux word** stored to
  `rec+0x36` (no reader found for it in this slice — leg FU159-L4).

### 3.2 The four jump rows and where their auxes sit (fresh scan of the ported tables, reproduced from the native frame tables)

| row | frames (aux low byte) | row flags | next_id | bank |
|---|---|---|---|---|
| `0x12` | f2 aux `0x0E` | `0x00` | `0x0F` | `0x11` |
| `0x14` | f0 aux `0x0F` | `0x00` | `0x7F` (mirror -> `0x0F`) | `0x13` |
| `0x5C` | f2 aux `0x3A` | `0x00` | `0x27` | `0x47` |
| `0x5E` | f0 aux `0x0E` | `0x00` | `0x0F` | `0x0C` |

First-hand frame reads: `0x10E3E6` (row `0x5E` f0 `40 00 0E 00 01`),
`0x10E48A` (row `0x12` f2 `80 00 0E 00 01`), `0x10E4BC` (row `0x14` f0
`40 00 0F 00 00`), `0x10EDBE` (row `0x5C` f2 `00 01 3A 24 02` — aux `0x243A`,
low `0x3A`). Row entries re-read at `0x10F236` window. All four rows have
flags `0x00`, so a record in a jump row has `byte44 == 0` (the non-terminal
branch of the successor machine — the row chain, not the steering tables).

### 3.3 The identified installers (both in **unported** action bodies)

* **Row `0x5E` <- the row-`0x0C` body** (`0x81C90..0x8251B`, FU-137 table
  entry 12, "not ported (partial)") at `0x81DA4..0x81DB9`:
  `0x81D81 IDIV EBX(7); TEST EDX,EDX; JNZ` — the gate is
  `(team[0x7CF] % 7 == 0)` (the dword at `[0x15888F]`->team, `[+0x7CF]`
  incremented at `0x81D6B`) **and** `[EBP+0x20] != 0` (the record carries a
  slot block); then `FUN_0008F188(0xA2, rec, 4)` (`0x81D9A`) and
  `FUN_0006E598(rec=[0x15888F], 0x5E, ECX=byte[+0x8E])` (`0x81DB9`, bytes
  `e8dac7feff`), then `CALL 0x7DAB4` (the shared reset). Row `0x0C` is the
  foul/collision decision body (FU-150) — the `0x5E` install is its
  stumble/jump arm.
* **Row `0x12` <- the row-`0x0E` body** (`0x82710..0x82ACF`, table entry 14)
  at `0x827D0..0x827E2`: stage++/timer89=0, target := `[0x157794]` triple,
  `CALL 0x79B58` (receiver gate), then `if (timer89 < 0x0C) wait`
  (`0x827C3/0x827CA`) else `FUN_0006E598(rec, 0x12, octant)` (`0x827D0`,
  bytes `e8b1bdfeff`).
* **Rows `0x14`/`0x5C`: no installer identified.** The fresh census found no
  `MOV EDX,0x14` adjacent to a `0x6E598` call, **no `MOV EDX,0x5C` /
  `MOV DX,0x5C` anywhere in the image** (`search_byte_patterns`:
  `ba 5c 00 00 00`, `66 ba 5c 00` both 0 matches), no row whose `next_id` is
  `0x14`/`0x5C`/`0x84`/`0xCC` (scan of the ported table = the native table),
  and no such value in the movement tables `0x11052A`/`0x1105AA` or the
  direction/tables read. The residual dynamic call sites (the 8-entry rolling
  table at `0x8695C`, the `0x10E17D` table at `0x76F91`, the stack/global id
  sites `0x759xx`/`0x76863`/`0x87603`/`0x87667`/`0x818CB`/`0x81AFB`/
  `0x8CFD2`/`0x8CFED`) were inspected but do not resolve to `0x14`/`0x5C` in
  the read windows — **leg FU159-L2** (their live reachability is open).

**The full direct-call id census this slice** (id at each fresh site; `D` =
dynamic register): `0x0D`/`0x52` (`0x81F01`), `0x0F` (`0x824B0`, `0x7DA1A`),
`0x0B` (`0x82693`, `0x811AA`), `0x12` (`0x827E2`), `0x10` (`0x825AD`),
`0x04` (`0x82BA5`), `0x5D` (`0x845EA`, `0x88A35`, `0x7159C`, `0x71651`,
`0x71766`, `0x71807`, `0x71877`), `0x55`/`0x6A` (`0x846CC`), `0x4A`
(`0x85335`), `0x46`/`0x47` (`0x878F0`), `0x00` (`0x87AFB`, `0x7E4AD`,
`0x85F2C`), `0x25` (`0x7E472`), `0x51` (`0x7A5E3`), `0x1C` (`0x80826`),
`0x11` (`0x80BC5`), `0x48` (`0x8197B`), `0x68` (`0x819A0`), `0x26`
(`0x75000`, `0x79C13`), `0x27` (`0x756E8`), `0x32` (`0x75754`), `0x28`
(`0x76461`), `0x33` (`0x77682`), `0x5E` (`0x81DB9`), `0x4F` (`0x85453`),
`0x05` (`0x85745`, `0x85829`), `0x01` (`0x848F1`), `0x0F` (`0x6EDxx`
internal), `D` (the sites listed above + the `FUN_0006E8E8` internal
successor calls `0x6EF58`/`0x6F175`/`0x6F6A3`/`0x6F747`/`0x6F773`/`0x6FCFE`/
`0x6FD36`/`0x6FDB3`/`0x6FE18`/`0x6FE41`). **No ported-wired action body
installs any jump row**; rows `0x5E`/`0x12` come only from the unported
`0x0C`/`0x0E` bodies.

### 3.4 Engine status (what is live today)

The engine's live row choice goes through `fifa96_arm_anim_select`
(`src/fifa96_loader/fifa96_arm_helpers.c:96`), called by the ported arm bodies
with kinds `{0x5D, 0x6B, 0x68, 0x03, 0x16, 0x15, 0x56, 0x67, 0x50, 0x1A, 0x19,
0x60, 0x61, 0x01, 0x02, 0x58, 0x5B, dynamic, keep}` plus the kickoff place
(`active ? 0 : 0x26`, `fifa96_match_entities.c:514`) and the ball pairing
(`state->sub_code`, `fifa96_ball_pairing.c:202`). **No path produces
`0x5E`/`0x12`/`0x14`/`0x5C`**; the T1 test stages the jump by writing
`rec->anim_id = 0x5E; rec->frame = 0` directly
(`tests/test_engine_match_frame.c:2133`) — the disclosed stand-in for this leg.

---

## 4. T1-L2 — the row-07 stage-2 `+0x44` ack producer

### 4.1 The writer census (fresh full-program sweep)

`search_instructions` over **all byte accesses at `[reg+0x44]`** (214 matches)
splits into ~35 read gates and exactly these **writers of a record `+0x44`**:

| site | function | value |
|---|---|---|
| `0x6E6CC` | `FUN_0006E598` (row install) | `row_flags & 1` |
| `0x8DFC1` | `FUN_0008DF84` (pool row install) | `row_flags & 1` |
| `0x8E6AB` | `FUN_0008E5BC` (kickoff placement) | `1` (then `FUN_0008DF84(rec,0,0)` re-derives it) |
| `0x8EA08` | `FUN_0008E910` (kickoff placement) | `1` (same) |

(A fifth store `0xAE2B6 MOV [EDI+0x44],BL` exists in `FUN_000AE218`, outside
the match-record address family; not resolved here and not counted.)

The main installer's exact bytes (fresh):

```
0x6E6A9  MOV EDX, [0x157588]           ; the row table base
0x6E6B1  MOV [ESI+0x28], EDX           ; rec+0x28 = row table + id*9
0x6E6B4  MOV AL, byte[EDX+0x2]         ; row flags
0x6E6B7  MOV AH, AL; 0x6E6B9 AND AH,0x4; 0x6E6BC MOV [ESI+0x45],AH
0x6E6BF  MOV AH, AL; 0x6E6C1 AND AH,0x2; 0x6E6C4 MOV [ESI+0x43],AH
0x6E6C7  MOV AH, AL; 0x6E6C9 AND AH,0x1; 0x6E6CC MOV [ESI+0x44],AH  <-- THE PRODUCER
0x6E6D8  MOV byte[ESI+0x46], 0x1       ; the reload/lean marker
```

So **`byte44` is not an event ack — it is the current anim row's flags bit 0**
(the loader already names it `terminal`, `fifa96_animation_flags`). It is
re-derived on **every row install**, i.e. under every
`FUN_0006E598`/`FUN_0008DF84` call.

### 4.2 Why it gates the kick machine (and the second kick)

* **Reader:** `0x816FC CMP byte[EBP+0x44],0` — the row-07 machine's **stage-2
  case** (the stage dispatch at `0x814F3..0x81505`: `AL==1 -> 0x815CD`,
  `AL==2 -> 0x816FC`, `AL>=3 -> RET`), and **stage 1 falls through to it**
  (`0x816E4..0x816F6`: `timer89=0; stage++` then `0x816FC`). If
  `byte44 != 0` -> `0x81702 CALL 0x7DAB4` (the reset) + the ball-actor install
  + receiver timer. If `byte44 == 0` -> return (the machine waits at stage 2).
* The normal kick fires from an idle/walk row (flags `0x07`, e.g. row `0`
  after kickoff) -> `byte44 == 1` -> the reset fires **the same frame**.
* The reset is what lets the record take a new action: with
  `s.byte44`/`s.row44` staged 0 (`fifa96_match_handlers.c:2649` row 08;
  `match_kick_from_record` leaves it 0 by the memset + the OL-65 comment), the
  engine's `kick_machine_07` **case 2 never resets** — the stage sticks at 2
  and a second kick by the same record in one possession cannot stage (the
  T1 report's leg, confirmed in code).
* `byte44` is also the branch selector of the **advance machines**
  (`FUN_0008E008` §4.2 / the `0x6FC05` twin): non-terminal rows play their
  `next_id` chain; terminal rows re-derive the next row from the steering
  tables. That makes it part of the row-selector semantics, not only a wait
  flag.

The other reader gates (fresh list, for wiring) include `0x811DF`/`0x8148B`
(row 08), `0x82BD0` (kick 0F), `0x84517`/`0x84706` (rows 0x12/0x16),
`0x84EB8`/`0x851E8`/`0x85352`/`0x85470` (set pieces), `0x74F60`/`0x754A6`/
`0x75611`/`0x75773`/`0x760A8`/`0x76AAD`/`0x77636`/`0x7769F`/`0x779D1`
(keeper rows), `0x79BDA` (`FUN_00079B6C`), `0x7DFB2`/`0x7E18C`/`0x7F627`/
`0x85D61`/`0x86466`/`0x869AF`/`0x89B6A`.

### 4.3 Live-reachability

**Yes natively:** every row install runs the producer; a kick from idle has
`byte44 == 1` and resets in the fire frame. **No in the engine:** the producer
is simply unported; `row44 = e->row44` pass-through
(`match_run.c:1117/1271`) carries the always-zero pool byte. The fix is a
one-liner derived from the ported row table — but for the *repeat* it must be
paired with the row-successor chain (`0x5E -> 0x0F -> 0x00`, table cites §3.2)
so the record returns to a terminal row (flags `0x07`, `byte44 = 1`) after the
jump animation; otherwise even a correct producer leaves `byte44 == 0` at the
stage-2 check while the record sits in row `0x5E` forever.

---

## 5. T3 wiring recommendation (concrete)

**Files:** `src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`tests/test_engine_match_frame.c` (unit + ISO), `docs/ENGINE.md`.

1. **Produce `row44` (the T1-L2 fix).**
   * In `match_run_dispatch_entity` (match_run.c:1117 region), replace the
     `r->row44 = e->row44` pass-through with
     `fifa96_animation_row_lookup(match_run_anim_rows, …, e->anim_id, &row)`
     -> `r->row44 = row.flags & 0x01` (and write it back at 1271). This is
     exactly equivalent to the native (the native writes the same value only
     at install, and `anim_id` only changes at install).
   * `fifa96_match_handlers.c`: `s->byte44 = r->row44;` in
     `match_kick_from_record` (currently 0 via memset, OL-65 comment) and
     `s.byte44 = r->row44;` at the row-08 staging (line 2649).
2. **Port the minimal row successor (the repetition enabler).** In the
   frame-advance write-back (`match_run_scene_stage`, match_run.c:1430-1435)
   or a new `match_run_record_row_successor(r, row)`: when the frame advance
   **wraps** (`out.frame_index == 0` after `advanced`), install the row's
   `next_id` per `FUN_0008E008` (`0x8E0F9`/`0x8E12F`/`0x8E19A`/`0x8E1F7`/
   `0x8E218`): `0x6F` = keep; `< 0x70` = install `next_id`; `>= 0x70` =
   `fifa96_arm_face`-mirror + install `next_id - 0x70`; then recompute
   `row44` (step 1) and reset the frame to 0. **Bind the native frame argument
   `byte[+0x46] + frame` as leg FU159-L5** (the `+0x46` lean byte is unmodeled;
   `0` is the pool default and preserves the self-loop rows `0`/`0x26` of the
   tape). Cite: rows `0x5E -> 0x0F -> 0x00` and the flags column of
   `match_run_anim_rows`.
3. **Port the case-`0x0F` snap + resolve (T1-L1).** In
   `match_run_record_jump_arm` case `0x0F`:
   * add the `FUN_00079B1C` store set (`0x6EE90/0x6EE92`; §2.3) **before** the
     `+0x85 == 0` check: `target := pos; lane/target-delta/speed/body zero`,
     `+0x65/+0x67/+0x73/+0x75 := 0` (expose as `match_run_record_snap`);
   * add the `pos_y != 0` else-branch: frame := 3 through the resolver clamp
     (`0x6EE7B..0x6EE8B`, `FUN_0006E490(rec, rec+0x28, 3)`).
   * Unit test (extend `test_record_jump_arc_starter`): with `pos_y != 0`,
     stage nonzero lane/vel and assert the snap + `frame == 3`; with
     `pos_y == 0 && jump_base == 0`, assert `jump_base = ramp(0x1C)`,
     `pos_y == 0`, `jump_timer == 0`.
4. **Row-selector (T1-L1b) — bounded honest step.** Add
   `fifa96_match_run_record_anim_install(mr, rec, id, frame)` porting the
   `FUN_0006E598` store core (clamp, row pointer semantics = `anim_id`/`frame`/
   `row44`, frame resolve) and use it in the tests in place of the raw
   `rec->anim_id = 0x5E` write (test 2133). The **live** installers stay a
   leg: rows `0x5E`/`0x12` are installed by the unported `0x0C`/`0x0E` action
   bodies (`0x81DA4`, `0x827D0`) — porting either body is out of T3's slice
   (record it as the next hop). Do not invent a live gate.
5. **Tests (RED first).**
   * `test_row44_is_anim_row_terminal_flag` (unit): row `0x00 -> 1`,
     row `0x5E -> 0`, row `0x26 -> 1` (table values, first-hand).
   * `test_live_second_kick_same_possession` (ISO, extends the T1 chain):
     after the band-2 follow, drive frames and assert the row chain
     `0x5E -> 0x0F -> 0x00` via the successor and `row44 0 -> 0 -> 1`; assert
     the kick-07 machine's reset ran (`match_row_reset` observable; stage back
     to 0) and then re-arm the long ball + KICK **on the same record in the
     same possession** and assert a second staging (`pair.code == 4` again).
   * Keep the T1 `test_live_airborne_long_ball_seed_then_band2_follow` as the
     per-half seed (no change expected beyond the helper call).
6. **Goldens:** no re-pin expected — the successor is a no-op for the tape's
   self-loop rows (`0`/`0x26`), and the T1/T3 chains are live-only. M1 never
   moves; verify with `cmp` (M1 `09b726b7…`, M2 `2759dace…`).
7. **Census:** no action row changes -> `wired` stays 21/80, README `26%`
   unchanged.

**Optional (leg-gated, not required for the gate):** the aux-`0x3A`/`0x0E`
motion writes (dir tables `0x10F334`/`0x10F33C` first-8 entries are read
first-hand above; `FUN_000795A4` = the rounded `>>16` product; `FUN_0008DC68`
is the ported metric) — porting them gives native-exact jump translation;
without them the jump is visually vertical. If T3 wants the smoke jump to
*carry*, port them in the same helper and pin `speed71/vel73/vel75` in the
unit test; otherwise keep as FU159-L1.

---

## 6. Numbered legs (new)

* **FU159-L1 — the jump arm's impulsion.** The aux-`0x3A` `±0x20` facing step
  (`0x6ED3A..0x6EDB0`, table `0x114E04` + `FUN_000795A4`) and the aux-`0x0E`
  speed/direction words (`0x6EE1B..0x6EE5B`, tables `0x10F334`/`0x10F33C` +
  `FUN_0008DC68`) are unported. Not required for the `pos_y` arc; required for
  native-exact airborne motion. (Tables read first-hand in §2.2.)
* **FU159-L2 — the rows `0x14`/`0x5C` installers.** No immediate install
  (`ba 5c 00 00 00`/`66 ba 5c 00` = 0 matches; `MOV EDX,0x14` sites not
  adjacent to `0x6E598` calls), no `next_id` hit, no movement-table hit. The
  residual dynamic sites (`0x8695C`, `0x76F91` table `0x10E17D`, `0x759xx`,
  `0x76863`, `0x818CB`, `0x81AFB`, `0x87603`, `0x87667`, `0x8CFD2`,
  `0x8CFED`) are unresolved register/table sources; their live reachability
  is open.
* **FU159-L3 — the aux-`0x39` tail sinks.** `0x974DC` (`0x6EDB3`),
  `FUN_0008F188(0x62, rec, 4)` (`0x6EDD7`) and the `+0x36` aux-chain stores
  (`0x6EDC3`, `0x6EE6C`, `0x6EDDC` value `0x27`) — event/audio sinks (the
  OL-52/OL-82 family); the chain-continuation semantics of `+0x36` are not
  modeled in the engine.
* **FU159-L4 — the frame-record `byte+3` (aux high byte).** The high byte of
  `word[frame+2]` (e.g. row `0x5C` f2 = `0x24`, row `0x12` f1 = `0x10`) has no
  reader found: the successor reads the **row's** `next_id` (`row+3`), not the
  frame's. Confirm with a fresh `+0x36`/`+0x37` reader sweep before modeling.
* **FU159-L5 — the `+0x46` lean byte / successor frame argument.** The flags
  bit-2 fold (`FUN_0008E008` `0x8E02x`: `+0x46 = 1` or `0xFF` by the facing
  diff) fed into the successor's frame argument (`byte[+0x46] + frame`,
  `0x8E19C..`) is unmodeled; the pool default 0 is used in the T3 step.
* **FU159-L6 — the live jump-row install gate.** The row-`0x0C` gate
  `(team[0x7CF] % 7 == 0)` + `rec[+0x20] != 0` (`0x81D81..0x81DB9`) and the
  row-`0x0E` gate (`timer89 >= 0x0C`, `0x827C3..0x827E2`) are the native
  live-reachable sources of rows `0x5E`/`0x12`; both bodies are unported.
  Next-hop for a fully live (non-staged) jump.

## 7. Evidence index (fresh this slice)

* `decompile_function`: `0x6E8E8`, `0x6E598`, `0x6E490`, `0x79B1C`, `0x795A4`,
  `0x8DC68`, `0x8DF84`, `0x8E008`, `0x8E5BC`, `0x8E910`.
* `disassemble_bytes`: `0x6ED00..0x6EDC3`, `0x6EDC0..0x6EE90`,
  `0x6EE75..0x6EEBE`, `0x6E640..0x6E700`, `0x814B0..0x81514`,
  `0x816C0..0x81740`, `0x81D40..0x81DC5`, `0x81EF0..0x81F05`,
  `0x82495..0x824B5`, `0x82560..0x825AE`, `0x82678..0x82698`,
  `0x82798..0x827F0`, `0x82B88..0x82BAB`, `0x845CD..0x846D1`,
  `0x848D0..0x848F6`, `0x85318..0x85454`, `0x85724..0x8582E`,
  `0x85F0B..0x85F31`, `0x86900..0x869FD`, `0x875E6..0x8766C`,
  `0x878D3..0x87B00`, `0x88A18..0x88AA2`, `0x8CFB5..0x8CFF2`,
  `0x74FDE..0x75002`, `0x756CB..0x75759`, `0x75CC0..0x75CF8`,
  `0x76444..0x76868`, `0x76F74..0x76F96`, `0x77551..0x77687`,
  `0x7A5C6..0x7AA30`, `0x7E455..0x7E4AE`, `0x7F5A5..0x7F5C7`,
  `0x80809..0x8082B`, `0x80BA8..0x80BCA`, `0x8118D..0x811AF`,
  `0x818AE..0x81B00`, `0x79BF1..0x79C19`, `0x110500` (read).
* `get_xrefs_to 0x6E598` (84), `0x8DF84` (7); `search_instructions`
  `[reg+0x44]` (214 byte matches); `search_byte_patterns`
  `ba 5c 00 00 00`, `66 ba 5c 00`, `ba 14 00 00 00`, `66 ba 14 00`.
* `read_memory`: `0x10F236` (row table rows `0x5B`..`0x5E`), `0x10E3E6`,
  `0x10E48A`, `0x10E4BC`, `0x10EDBE` (frame tables), `0x1106E0` (row-handler
  table), `0x110500` (movement/steering tables), `0x10F334`, `0x10F33C`
  (direction tables), `0x10E17D`.
* Engine/port cites: `src/fifa96_engine/fifa96_match_run.c:47-159` (row
  table), `:974-985` (frame aux), `:1018-1071` (arm/arc), `:1117`,
  `:1213-1232`, `:1369-1437` (advance), `:504-548` (mover);
  `src/fifa96_loader/fifa96_action_handlers.c:1534-1627` (kick 07),
  `:1629-1690` (kick 0F); `src/fifa96_loader/fifa96_animation.c:51-71`
  (advance), `:1-30` (row lookup/flags: `terminal = flags & 1`);
  `src/fifa96_loader/fifa96_arm_helpers.c:96-117` (anim_select);
  `src/fifa96_engine/fifa96_match_handlers.c:1135-1159` (`fifa96_match_action_07`),
  `:895-1000` (`match_kick_from_record`), `:2649` (row-08 byte44 stand-in);
  `tests/test_engine_match_frame.c:2090-2183` (the T1 chain; the `anim_id =
  0x5E` stand-in at `:2133`), `:2271` (`test_record_jump_arc_starter`).
* The read-only scan `/tmp/opencode/r7/scan.py` (jump-aux rows from the ported
  tables) and `scan2.py` (next_id hits) — sources kept for the controller.
```

---

## Status append — T3 implementation (phase-13, 2026-10-11)

Landed (commit `feat(engine,fu159)` + the fu160/fu161 riders):

1. **byte44 producer (T1-L2 closed).** `+0x44` is the current anim row's
   flags bit 0 (`0x6E6CC`); the engine derives it from `anim_id` at the
   dispatch staging (`match_run_row_terminal_flag`) instead of the always-zero
   pool pass-through, and stages it into `match_kick_from_record`'s
   `s->byte44` and the row-08 `s.byte44`. The kick-07 stage-2 reset now fires
   (`test_pad_kick_release_runs_kick_row` pins the post-kick reset into the
   forced decision) — the second kick in one possession is unblocked.
2. **Minimal row successor (§5.2).** `fifa96_match_run_record_anim_successor`
   ports the FUN_0008E008 frame-wrap chain: non-terminal rows install the
   `next_id` (`0x6F` keep; `< 0x70` install; `>= 0x70` mirror `(type+4)&7` +
   install `- 0x70`); the install frame argument is the row's last frame + 1
   (the `+0x46 = 1` install default at `0x6E6D8`) clamped by the new row
   (`FUN_0006E490`). Wired at the scene stage's wrap edge
   (`out.advanced && prev_frame >= row.last_frame`), so `0x5E -> 0x0F -> 0x00`
   runs and the record returns to a terminal row (row44 0 -> 0 -> 1).
   Terminal rows take the movement/steering tables — still leg **FU159-L5**
   (the `+0x46` flags-bit-2 update is the residual; the install default 1 is
   modeled and exact for the non-terminal rows with flags bit 2 clear).
3. **Install helper (§5.4).** `fifa96_match_run_record_anim_install` ports the
   FUN_0006E598 store core (id clamp `> 0x6E -> 0`, frame resolve, row44, the
   `+0x32 = 0` frame-timer reset, forced aux re-arm). The T1 test stand-in
   (`rec->anim_id = 0x5E` raw writes) is replaced in the arc starter and the
   live chain.
4. **Case-0x0F snap + frame-3 resolve (§2.3).** `match_run_record_snap`
   (FUN_00079B1C: target := pos, lane/speed/vel/body zero) is used by the
   0x0F pos_y==0 branch and by aux 0x05 (the live-chain stop); the pos_y != 0
   branch resolves frame 3 through the FUN_0006E490 clamp.
5. **One-shot aux wording.** The `+0x36` one-shot model (FU-161 §5.4) is
   implemented: `e->aux_word` re-arms on the (anim_id, frame) resolve edge and
   the ported one-shot arms clear their byte. The arc starter proves the
   descent without the old `anim_id = 0` mask.

Tests: `test_record_anim_install_row44`, `test_record_row_successor_chain`,
`test_record_jump_case_0f_snap_frame3` (unit); the live T1 chain re-stages the
jump via the helper for the airborne kick. Goldens: M1/M2 byte-exact, no
re-pin; `wired` unchanged by this piece (the row-0x0E wiring is FU-160's).

Carried legs: FU159-L1 (the other-row sine steps; the jump arm's own impulse
is now ported under FU-161), FU159-L2 (rows 0x14/0x5C installers), FU159-L3
(sinks), FU159-L4 (frame byte+3), FU159-L5 (refined above), FU159-L6 (row
0x5E's live installer — the row-0x0C stake; row 0x12's is now live via
row-0x0E).
