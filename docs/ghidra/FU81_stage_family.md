# FU-81: the stage/transition action family and the `0x2D..0x4F` phase-handler table

Follow-on to FU-76 (action table/class census) and FU-77..FU-79: derive the
stage/transition action codes that chain the per-record state machine, and
close FU-76 open leg 1 — the `0x2D..0x4F` half of the flat `0x1106E0` block.
Ports the proven shared mechanics as `fifa96_action_stage_*` /
`fifa96_action_phase_select`.

Result in one line: **the `0x1106E0` block is two tables end to end — a
45-slot action table `0x00..0x2C` read only by `FUN_0007D9A4`
(`EAX = code<<2; ADD EAX,0x106E0`, installed at `[rec+0x18]`) and a 35-slot
phase table `0x2D..0x4F` read by `FUN_0006D920` (`EAX = [0x57A4A]>>24; EAX<<=2;
ADD EAX,0x10794`, resolver at `0x6D9B3`), whose entry is installed at
`[rec+0x1C]` and invoked `CALL [rec+0x1C]` with `EAX = rec`, `EDX = &rec+0x4D`
(most sites `EBX = -1`) — so `EDX` is the output-triple pointer, not the
selector; the selector is the match phase byte `[0x57A4A]>>24`; the family is
the per-phase record driver (formation placement group `0x6Dxxx` and
timeline/cinematic group `0x8Bxxx`). The stage/transition action codes are the
13 slots whose FU-76 §2 class is a stage machine or stage/phase/gated/camera
transition (`01,02,09,0C,0D,0E,10,11,12,13,15,17,20`); they share the chain
model `gate(phase) → +0x89 += delta → switch(+0x92) arm table (CS: dwords,
target = stored+0x10000) → arm advances +0x92 with +0x89 = 0 / installs a
successor by `CALL 0x7D9A4` / hands off through `FUN_0007DAB4` when `+0x44` is
set`.**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). `disassemble_function`/`disassemble_bytes` are the
  citation source; jump tables are quoted as raw dwords read with
  `read_memory` (stored value + `0x10000`); the decompiler is not used for any
  quote.
* **Address mapping (FU-76, restated).** Code/function addresses equal true
  link addresses (object 1 base `0x10000` already in the image); a **data
  immediate** Ghidra renders as `A` is flat `A+0x100000`
  (e.g. `0xF328` → `0x10F328`); stored code pointers and inline `CS:` tables
  are object-1 relative and resolve through `+0x10000` (e.g. `CS:` operand
  `0x6DBB0` → true table `0x7DBB0`; the phase table immediate `0x10794` →
  flat `0x110794`).
* Every numeric claim is quoted from the listings; bodies pruned by the
  listing are recovered with `disassemble_bytes`/`read_memory` and marked.
  Unproven items are open legs (no guessed labels).

## 1. The `0x1106E0` block is two tables

### 1.1 The phase resolver `FUN_0006D920` (`0x6D920..0x6D9C1`, 51 insns)

```
0x6D924  EDX = EAX                     ; rec
0x6D926  EAX = [EAX]                   ; team
0x6D928  EAX = [EAX+0x7AE]             ; formation/strategy row (set by 0x6D9C4)
0x6D92E  BX  = (int8)[EAX]             ; row[0] = formation id
0x6D932  EBX = BX * 6
0x6D935  ESI = [EDX+0x8A] >> 24 << 2   ; subtype * 4
0x6D93B  CX  = (int8)[EDX+0x8D]        ; active flag
0x6D951  CALL 0x4C384 (EAX = BX)       ; EAX = 0x4AFB8(index) - engine pointer lookup
0x6D956  EAX += ESI
0x6D95E  [EDX+0x8]  = EAX              ; per-record pointer 1
0x6D969  CALL 0x4C384 (EAX = BX+1)
0x6D96E  EAX += ECX*0x256
0x6D970  [EDX+0xC]  = EAX              ; per-record pointer 2
0x6D98C  CALL 0x4C384 (EAX = BX+2)
0x6D991  EAX += ECX*8
0x6D996  [EDX+0x10] = EAX              ; per-record pointer 3
0x6D99E  CALL 0x4C384 (EAX = BX+4)
0x6D9A3  EAX += ECX*0x958
0x6D9A5  [EDX+0x14] = EAX              ; per-record pointer 4
0x6D9A8  EAX = [0x57A4A]
0x6D9AD  SAR EAX,0x18                  ; phase byte
0x6D9B0  EAX <<= 2
0x6D9B3  ADD EAX,0x10794               ; flat 0x110794 = table slot 0x2D
0x6D9B8  EAX = [EAX]
0x6D9BA  [EDX+0x1C] = EAX              ; phase handler pointer
```

`ADD EAX,0x10794` is the only static reader of the second half
(`search_instructions` operand `0x1079` returns exactly `{0x6D9B3}`; FU-76's
operand searches `0x10780`/`0x107E0` missed the `0x2D` base). The resolver
indexes by the **phase byte** and performs **no bounds check**.

### 1.2 The table (`0x110794..0x11081F`, 35 dwords)

`read_memory 0x110794` (140 B), stored value → runtime target (`+0x10000`):

| slot | stored | runtime | slot | stored | runtime |
|---|---|---|---|---|---|
| 2D | `05DE34` | `06DE34` | 3E | `05E1C8` | `06E1C8` |
| 2E | `05E1D0` | `06E1D0` | 3F | `05E1D0` | `06E1D0` |
| 2F | `05DCC8` | `06DCC8` | 40 | `05E244` | `06E244` |
| 30 | `05DE44` | `06DE44` | 41 | `05E244` | `06E244` |
| 31 | `05DE44` | `06DE44` | 42 | `05DCC8` | `06DCC8` |
| 32 | `05E05C` | `06E05C` | 43 | `000000` | `010000` (INT3) |
| 33 | `05DD9C` | `06DD9C` | 44 | `078DC8` | `088DC8` |
| 34 | `05DE44` | `06DE44` | 45 | `07922C` | `08922C` |
| 35 | `05DD6C` | `06DD6C` | 46 | `079FA4` | `089FA4` |
| 36 | `05DD6C` | `06DD6C` | 47 | `079620` | `089620` |
| 37 | `05DE34` | `06DE34` | 48 | `0790EC` | `0890EC` |
| 38 | `05DE34` | `06DE34` | 49 | `079110` | `089110` |
| 39 | `05DF4C` | `06DF4C` | 4A | `079868` | `089868` |
| 3A | `05DE34` | `06DE34` | 4B | `07A798` | `08A798` |
| 3B | `05DE34` | `06DE34` | 4C | `078F4C` | `088F4C` |
| 3C | `05DE34` | `06DE34` | 4D | `07B688` | `08B688` |
| 3D | `05E004` | `06E004` | 4E | `07B874` | `08B874` |
| | | | 4F | `07B900` | `08B900` |

The 35th slot ends at `0x110820`; `0x110820` is already the outfield
pressed-row `{0x07FF,0x60,0x6CE38}` (FU-76 §1.2). Slot `0x43` is zero, which
the loader `+0x10000` turns into `0x10000` (`CC` = INT3), i.e. phase `0x16` has
no handler (deliberate trap/unused).

### 1.3 Call/entry convention

`[rec+0x1C]` call sites (`search_instructions` `CALL dword ptr […+0x1c]`),
record ones:

```
0x799A6   FUN_0007997C   EDX = &rec+0x4D, EAX = rec  (after gate TEST EDX)
0x7E252   FUN_0007E1A4   EDX = &rec+0x4D, EBX = -1, EAX = rec
0x87D29   FUN_00087CD0   EDX = &rec+0x4D, EBX = -1, EAX = rec (code 0x15 arm)
0x8CF7C   FUN_0008CF60   EDX = &rec+0x4D, EBX = -1, EAX = rec, for 11 records
0x8D110   FUN_0008D098   EDX = &rec+0x4D, EBX = -1, EAX = rec, per record
```

`FUN_0008CF60` (`0x8CF60..0x8CFAB`) loops `ECX = rec, ESI = &rec+0x4D` over
`+0xB2`-strided records, calls `FUN_0006D920`, then `CALL [ECX+0x1C]` with
`EDX = ESI`, then `FUN_00079F3C` (camera placement) and `FUN_00079B6C`.
`FUN_0008D098` (`0x8D098`) does the same per record each frame while
`[rec+0x9A] == 0` (`0x8D0F7..0x8D110`), so the handler follows the live phase.
`FUN_0007997C` (`0x7997C`) is the placement/teleport entry: if the incoming
`EDX` is non-zero it copies that triple to `rec+0x59`, else it calls the phase
handler and copies `rec+0x4D` into `rec+0x59` (`0x7998C..0x799B1`).

So the second-half family is a **per-phase per-record driver**, not an action:
`EAX = rec`, `EDX = &rec+0x4D` (the out-position triple that FU-77's
`FUN_0007BF20` consumes), `EBX` unused/`-1`, selection by `[0x57A4A]>>24`.

### 1.4 The `0x6Dxxx` group (`0x2D..0x42`, slots whose stored < `0x60000`)

All write the `EDX` triple and read the resolver-set pointers:

| target | slots | body (quoted) |
|---|---|---|
| `06DE34` | 2D,37,38,3A,3B,3C | `56 57 89D7 8D7059 A5 A5 A5 5F 5E C3` = `out = rec+0x59` position copy |
| `06DE44` | 30,31,34 | formation slot: `BX = word [team_side*2 + 0x577D6]>>16; DX = word [0x577D8 + side*4]` (`0x6DE4F..0x6DE70`); if `DX<0` copies camera `0x5774C` and calls `0x6D870`/`0x6DCC8`; else `out.x = v + v/4`, `out.z = w + w/4` (`0x6DEA2..0x6DEBA`), negated when `word [ESP+0x14] != 0` (`0x6DEC2..0x6DEE0`) |
| `06E1D0` | 2E,3F | `EDX = [rec+8]` (resolver pointer 1) plus `+2` when `[team+0x826] == [0x57AAC]>>24` (`0x6E1EA..0x6E1F4`); `out.x = b0*0x26`, `out.z = b1*0x21`, both negated for side 1 (`0x6E1F4..0x6E238`) |
| `06DCC8` | 2F,42 | `EAX = [rec+0xC]`; `BX < 0` → `BX = [team+0x826]` indexes `0x577DE` word table, `>>16`; `out.x = b0*0x26`, `out.z = b1*0x21`, negated for side 1 (`0x6DCC8..0x6DD3B`) |
| `06DD6C` | 35,36 | wrapper: `EBX = (team side == [0x57AAC]>>24) ? 0xBC : 0x6F`, `CALL 0x6DCC8` (`0x6DD6C..0x6DD98`) |
| `06DD9C` | 33 | ball-line placement: reads `[rec+0x10]`; picks entry 0/1 by `sign([0x57754])` (ball z), `out.x = v + v/4`, `out.z = w + w/4`, then `out.z ±= 0x60` by ball sign, `out.y = 0` (`0x6DD9C..0x6DE31`) |
| `06DF4C` | 39 | timer machine: `[rec+0x89] += delta`; when `>= 0x3C` `[rec+0x7B] = (byte [0x57A38+idx]) >> 1`, `out.x = 0x780`, `out.z = 0`; `+0x8A>>19*5` added to `out.z`; if `|[rec+0x67]>>16| < 0x20` → `out.x = 0xCC0` (`0x6DF4C..0x6E001`) |
| `06E004` | 3D | if `[rec+0x8D] != 0`: `out.x = (int8)(dword [0x105E7+(b-1)*2]>>24)<<5`, `out.z = (int8)(dword [0x105E8+(b-1)*2]>>24)<<5`, z negated for side 1; else `out = (0x3C0, 0, 0x840)` (`0x6E004..0x6E058`; bytes `12 F7 5A 08 EB 0D C7 02 C0 03 00 00 C7 42 08 40 08 00 00 C7 42 04 00…` recovered past the listing cut) |
| `06E1C8` | 3E | `EBX = -1; EDX = &rec+0x4D`, falls into `06E1D0` (writes its own triple) |
| `06E05C` | 32 | ball/offside line: if `[rec]` equals `[0x57A9F]` (ball owner team?) `0x6E06E..0x6E072`; else copies the ball-side position; `CALL 0x8DC68` distance, `>= 0x5A0`/`0x12C0` switch, then `out = ball team position` with `0x14E04` fold offsets (`0x6E078..0x6E1B2`) |
| `06E244` | 40,41 | if `[team+0x826] == [0x57AAC]>>24`: `out` from `[0xF368]`/`[0xF364]` plus `[rec+0x8D]` arm; else jumps to a cut arm (`0x6E244..`; listing mis-decodes at `0x6E280`, body read linearly from `0x6E040` dump) |

The `0x4C384` helper is `AND EAX,0xFFFF; JMP 0x4AFB8` (`0x4C384`) — the
engine's indexed pointer lookup; `FUN_0006D9C4` (`0x6D9C4`, cited above) is the
team setup that walks a 29-byte (`idx*0x1D`) formation row at object-4
`0x1033A`, stores it at `[team+0x7AE]`, writes `record+0x90 = row[0]` for each
of the 4 groups and calls `FUN_0006D920` per record (`0x6DA0A`).

### 1.5 The `0x8xxxx` group (`0x44..0x4F`)

Per-phase timeline/cinematic drivers keyed on their own stage byte `[0x58829]`
and timer word `[0x58818]` (incremented by `[0x57A64]`, e.g.
`0x88DD7..0x88DF5`), using camera `CALL 0x700F4`, sound/event `0x974DC/0x974F0`
and event `0x6E598`; several also install action codes by `CALL 0x7D9A4`:

* `088DC8` (44): `MOV EAX,3; CALL 0x4C374`; ticks `[0x58818]`, compares
  `[0x57AC5]` vs `[0x57AC7]` (team match words), branches on `[0x58829]`
  0/1, resets globals `0x58818/0x58808/0x58828/0x58829`.
* `088F4C` (45): same timer; stage 0..3 `JMP CS:[EAX*4+0x78F3C]` → flat
  `0x88F3C`; stage 0 resets camera (`CALL 0x700F4` with `[0xF328/2C/30]`),
  `CALL 0x740A0`, stages the ball `0x5774C`; reads `[0x5B680]` (mode) in
  3/0/1 checks; increments `[0x58829]`.
* `0890EC` (48): pure reset — zeroes `[0x58818]`, `[0x58828]`, `[0x58829]`,
  `[0x58808]`; `RET`.
* `089110` (49): ticks; stage 0..2; stage 0 `CALL 0x4C374(0x15)`,
  `CALL 0x740A0`, `CALL 0x974DC(0x1E)`, `CALL 0x974F0(0x190)`,
  `[0x58829]++`.
* `08922C` (45), `089620` (47), `089868` (4A): timeline drivers with
  camera/event staging; `089868` installs action code `0x19` through
  `CALL 0x7D9A4` at `0x8990A` (installs action code `0x19`; the group's other
  installs are `0x89740`, `0x8994C/0x899AB/0x899CD`, `0x8A0A0/0x8A32F` and
  `0x8B750/0x8B78B`), and its
  stage 0..9 dispatch reads `[0x57A4A] == 0x10` (`0x898C4`).
* `089FA4` (46): stage 0..6 table `CS:[EAX*4+0x79F88]` → flat `0x89F88`;
  initialises `[0x57A73] = 0x587C0`, `CALL 0x36200`; stage 0 computes a
  camera/ball line from `[0x5888F]` and `[0x58814]` (`0x89FF8..0x8A035`).
* `08A798` (4B): stage 0..2; reads player id from `[0x5882B]>>24 * 0x835 +
  [0x5882C]>>24 * 0xB2 + 0x588A4` (`0x8A7E4..0x8A809`, team-record addressing).
* `08B688` (4D), `08B874` (4E), `08B900` (4F): 2-3 stage timer machines using
  `CALL 0x73E28`/`0x73E08`, `[0x5880C]`, `[0x58814]`, `[0x58830]`,
  `[0x58838]` and `CALL 0x974DC(0x1E)`; `08B874` completes with
  `[0x58808]=[0x58828]=[0x58829]=0`.

Classification: **phase-timeline driver** — the per-record handler the game
runs at each match phase to place a record (formation group) or drive the
phase's per-record animation/camera timeline (cinematic group). It is not
posture/animation in the pose sense (no pose data is touched) and not an event
dispatcher (it emits events; nothing dispatches through it).

## 2. The stage/transition action family

FU-76 §2 labels these codes with stage/phase/gated/camera-transition classes:
`01,02,09,0C,0D,0E,10,11,12,13,15,17,20` (13 slots). All are entered as
`CALL [rec+0x18]` with `EAX = rec` (FU-74 §2). The derived chain model:

1. **gate** — for the gated ones the first block is
   `EAX = [0x57A4A]; SAR EAX,0x18; CMP EAX,g1 [/ CMP EAX,g2]; JNZ RESET`
   (e.g. `0x7DBD3..0x7DBD6`, `0x855FB..0x8560B`, `0x85DF2..0x85E02`).
2. **timer** — `+0x89 += [0x57A64]` (accumulating duration meter; the
   opposite of code 0's decay). Cited per code below.
3. **stage** — byte `+0x92`; dispatch is a CS jump table (`JMP CS:[EAX*4+…]`,
   true table = operand+`0x10000`) with `CMP AL,n; JA tail` bounds or inline
   compares (`02`).
4. **arm hand-off** — arms zero `+0x89` and `INC +0x92` (e.g.
   `0x7DCA9..0x7DCB4`, `0x85805..0x85807`, `0x85FB7..0x85FD2`), or install a
   successor `FUN_0007D9A4` (`INSTALL` column), or call `FUN_0007DAB4`
   (RESET = the chooser FU-76 §1.1), typically when the terminal `+0x44` is
   set (`CMP byte [EBP+0x44],0; JZ end; CALL 0x7DAB4`, e.g.
   `0x7DFB2..0x7DFBA`, `0x85D61..0x85D8F`).

| code | runtime | gate phases | timer | stage table | arms | chain install | reset sites |
|---|---|---|---|---|---|---|---|
| 01 | `07DBC0` | `1` (`0x7DBD6`) | `+=` (`0x7DC42..0x7DC50`) | `0x7DBB0` | 4 | — | `0x7DFBA` if `+0x44` |
| 02 | `07DFCC` | `1`, `2` (`0x7DFE6`,`0x7E01B`) | `+=` (`0x7E069..0x7E081`, non-slot phase 2) | inline `0..2` (`0x7E090..0x7E098`) | 3 | `4` on self when `+0x20` (`0x7E04C..0x7E05A`) | `0x7E194` |
| 09 | `080A00` | `2` (`0x80A13`) | — | `0x809F0` | 4 | — | `0x80A1A` (gate), `0x80A4A` (stage 0) |
| 0C | `081C90` | — | `+=` (`0x81CA3..0x81CAE`) | `0x81C74` | 7 | — | `0x81DC0`, `0x824FD` |
| 0D | `08251C` | — | `+=` (`0x82532..0x8253A`) | `0x8250C` | 4 | — | `0x826F3` |
| 0E | `082710` | `2` (`0x82723`) | `+=` (`0x8273A..0x82742`) | `0x82700` | 4 | — | `0x82AC0` (gate), tail `0x82AC5` |
| 10 | `0855F0` | `2`,`3` (`0x85603..0x8560B`) | `+=` (`0x8561F..0x8562A`) | `0x855B8` (+event `0x855D4`) | 7 | `4` (`0x85C9C..0x85CA1`) | `0x85D8F` |
| 11 | `085DE4` | `2`,`4` (`0x85DFA..0x85E02`) | `+=` (`0x85E19..0x85E21`) | `0x85DA0` (+event `0x85DC8`) | 10 | — | `0x86502` |
| 12 | `083D68` | `2`,`7` (`0x83D7E..0x83D86`) | `+=` (`0x83E03..0x83E0B`) | `0x83D2C`/`0x83D4C` | 8/7 | `4` (`0x844E2..0x844E7`) | `0x83D93` (gate), `0x84556` |
| 13 | `084B00` | `2`,`6` (`0x84B13..0x84B1B`) | `+=` (`0x84B8A..0x84B92`) | `0x84AE4` | 7 | `0x13` on `[team+0x7B2]` (`0x84DA0..0x84DAB`) | `0x84D89`, `0x84EC0` |
| 15 | `087CD0` | `5` (`0x87CE3`) | `+=` (`0x87CFA..0x87D04`) | `0x87CC0` | 4 | — | `0x880A0` |
| 17 | `084730` | `0xB` (`0x84743`) | `+=` (`0x8475D..0x84765`) | `0x84720` | 4 | — | `0x849A1` (gate) |
| 20 | `084EEC` | — | `+=` (`0x84F39..0x84F41`) | `0x84ED0` | 7 | — | `0x85208` |

Additional install sites in family bodies: `0x844E7` (code 12),
`0x84DAB` (code 13), `0x85CA1` (code 10), `0x7E05A` (code 02) — all
`CALL 0x7D9A4` found by `search_instructions operand 7d9a4` (69 total sites).

### 2.1 Per-code deep dives

**Code `01` (`0x7DBC0..0x7DFC8`).** Gate phase 1 (`0x7DBCB..0x7DBD6`); phase
`+0x8F < 2` does a camera reset `CALL 0x700F4([0xF328],[0xF32C],[0xF330])`
(`0x7DBEA..0x7DC01`), sets `[0x57A83] = rec`, `+0x4D = ±0x30` by `+0x4D`
sign, `+0x55 = 0`, `CALL 0x7876C` (`0x7DC06..0x7DC2A`); otherwise copies
`+0x59 → +0x4D` (`0x7DC31..0x7DC39`). Timer `+0x89 += delta`
(`0x7DC3C..0x7DC50`). Stage table `0x7DBB0` = `{0x6DC6B,0x6DCAF,0x6DD29,`
`0x6DFB2}` → `{07DC6B,07DCAF,07DD29,07DFB2}`; the first arm reads
`[0x5882A]` and, when `+0x89 > 0x3C` with a neighbor/`+0x20` condition, emits
event ids `0x84,0x98,0x9B..0xA0` through `CALL 0x8F188` (ring/event) and
advances `+0x92` (`0x7DC97..0x7DCA9`); the next arms compare team scores
`[0x57AB6] - [0x57ABA] >= 0xA` and `[0x57AC5]` vs `[0x57AC7]` to pick
celebration event ids (`0x57,0x84,0x9B..0xA0`), stage the ball through
`FUN_0007A490` at `0x7DF53` and advance (`0x7DF9A..0x7DFAC`); the tail
`if (+0x44) FUN_0007DAB4(rec)` (`0x7DFB2..0x7DFBA`). The listing mis-decodes
`0x7DC89..0x7DCA7` and `0x7DEFE..0x7DF08` (overlap); those windows are not
quoted.

**Code `02` (`0x7DFCC..0x7E1A3`).** Phase 1 arm: `+0x9E = 1`, `+0x89 = 0`,
`+0x92 = 0`, target `+0x4D = -[team+0x7B2].x` (the restart 180° mirror,
`0x7DFEB..0x7E016`). Phase 2 arm: `[team+0x7B2] = rec`; if no control slot
and `[team+0x828] != 0` calls `0x7876C` (`0x7E02D..0x7E041`); **if a control
slot exists it installs code `4` on itself** (`EDX = 4, EAX = EBP, ECX = 1,
CALL 0x7D9A4`, `0x7E04C..0x7E05A`) and returns. Otherwise timer
`+0x89 += delta`, output copies the ball triple `0x5774C`
(`0x7E069..0x7E089`), inline stage compares (`AL 0..2`, `0x7E08A..0x7E098`),
arms at `0x7E0A8..0x7E193`; the tail resets through `0x7E194`.

**Code `09` (`0x80A00..0x81067`).** Gate phase 2 else `FUN_0007DAB4`
(`0x80A0B..0x80A1F`). Stage `+0x92` 0..3, table `0x809F0` =
`{0x70A3F,0x70BFB,0x70FCC,0x7103A}` → `{080A3F,080BFB,080FCC,08103A}`.
Stage 0 arms on `+0x8D`: when clear it calls RESET and clears
`[team+0x7B2]`/`[team+0x7B6]` if they were this record (`0x80A3F..0x80A85`);
when set it computes a ball-relative target from `[0x577C0]/[0x577C2]*0xA`
plus `+0x6D/+0x6F`, `CALL 0x79C50`, and a facing term through the `0x14E04`
table (`0x80A87..0x80ADB`). The listing drops bytes `0x80A24..0x80A25`
(`8A 85`, the head of `MOV AL,[EBP+0x92]`); the decoded tail
`CMP AL,3; JA; JMP CS:[EAX*4+0x709F0]` is intact from `0x80A2A`.

**Code `0C` (`0x81C90..0x8251B`).** No phase gate: timer
`+0x89 += delta` (`0x81C9D..0x81CAE`); if `[rec+0x5D] == 0` (no z position)
the default output is a `+0x59` copy (`0x81CB4..0x81CC0`). Stage 0..6, table
`0x81C74` = `{0x71CDC,0x71F4E,0x71FA7,0x7244B,0x7247D,0x724CD,0x724F2}` →
`{081CDC,081F4E,081FA7,08244B,08247D,0824CD,0824F2}`. Stage 0 requires
`+0x8D != 0` else jumps to the tail (`0x81CDC..0x81CE3`); it reads the ball
record `[0x5888F]`, checks its template byte `[[ECX+0x28]] == 0x59` and
`[ECX+0x3A]>>24 >= 2` (`0x81CE9..0x81D16`), then computes a target from the
ball record's `+0x59/+0x71/+0x73` doubles through `CALL 0x795B4`
(`0x81D1C..0x81D4F`) and a `0x14E04` fold; when `|z| < 0x30` it increments
`[team+0x7CF]` (`0x81D64..0x81D6B`).

**Code `0D` (`0x8251C..0x8270F`).** No phase gate: timer
`+0x89 += delta` (`0x82526..0x8253A`); clears `[0x57A83]` if it is this
record (`0x82534..0x82546`). Stage 0..3, table `0x8250C` =
`{0x72567,0x7261F,0x726B0,0x726D5}` → `{082567,08261F,0826B0,0826D5}`.
Stage 0 requires `+0x8D != 0` else RESET (`0x82567..0x8256E`); it sets
`+0x83 = 0x20`, `CALL 0x702F8(+0x81>>16, 0x10)`, `CALL 0x6E598` (event),
and scales `+0x73/+0x75` by `[0xF334]`/`[0xF33C]` (`0x82574..0x825EE`).

**Code `0E` (`0x82710..0x82ACF`).** Gate phase 2 else RESET
(`0x8271B..0x82726`). Timer `+0x89 += delta`; stage 0..3, table `0x82700` =
`{0x7275D,0x727B1,0x72806,0x72AB8}` → `{08275D,0827B1,082806,082AB8}`.
Stage 0 requires `+0x8D != 0`; calls `0x71B9C(0x12, &rec+0x4D, &rec+0x65)`,
`CALL 0x79C50` with the `+0x65` vector, then advances with `+0x89 = 0`,
copies `0x57794` into `+0x4D`, calls `0x79B58` and waits `+0x89 >= 0xC`
before event `0x6E598(0x12)` and `+0x9E = 1` (`0x8276A..0x827EE`; the listing
mis-decodes `0x82780..0x8278A`).

**Code `10` (`0x855F0..0x85DE3`).** Gate phases 2 and 3
(`0x855FB..0x8560B`), timer `+0x89 += delta` (`0x85613..0x8562A`); the
`+0x8F` marker block re-asserts `[0x57A83] = rec` when `< 3` and holds the
`+0x59` position when `< 5` (`0x85621..0x85651`). Stage 0..6, table `0x855B8`
= `{0x8566D,0x857D6,0x8580D,0x858E4,0x85CC4,0x85D5A,0x85D8D}`; the stage-3/4
arm (visible at `0x857E5`) `CALL 0x832A8`, advances, stages the ball
(`CALL 0x6E598`, `0x79CCC`?), and a later arm calls
`CALL 0x85498(rec, 0x78)` and on success advances (`0x858AB..0x858E4`);
`0x85966..0x85B9B` builds a camera/ball line (`CALL 0x700F4`,
`[team+0x7E4..]` triple with clamping to `[0xF0,0x5A0]`), then
`CALL 0x8DE8C` selects a record and **installs code `4` on it**
(`0x85C96..0x85CA1`). The `0x85A28` event sub-table `0x855D4` =
`{0x85A3F,0x85AA5,0x85A30,0x85A3F,0x85AA5,0x85A30,0x85AF0}` indexes
`[0x587B4]-1`. Tail: `[0x57A6A] = 0x12C`, event `0x8A938`, `CALL 0x4C380`,
`+0x92++`, and RESET (`0x85D19..0x85D8F`).

**Code `11` (`0x85DE4..0x864FF`).** Gate phases 2 and 4
(`0x85DF2..0x85E02`), timer `+0x89 += delta` (`0x85E0A..0x85E21`); the
`+0x8F` marker (<3 `<5`) is again present (same camera/ball blocks as `10`
from `0x85E3C`). Stage 0..9, table `0x85DA0` =
`{0x75E3C,0x75E7D,0x75FA2,0x75FBD,0x75FD8,0x76013,0x760CB,0x7610E,0x7645B,`
`0x76496}`. Stage 0 reads the match record `[0x587D4]`, `CALL 0x7D360`,
`CALL 0x700F4`, `CALL 0x79B6C`, stages through `CALL 0x6E598` and advances
(`0x85E3C..0x85F9C`); the `[0x587B4]` event sub-table `0x85DC8` =
`{0x861F9,0x86235,0x862BD,0x86272,0x862AA,0x862BD,0x862F4}` (`0x861DD..`
`0x861F1`); the 0xF0/0x5A0 camera/ball block repeats at `0x86092..0x862A5`
ending in `CALL 0x7B9C4` (kick application); tail RESET at `0x86502`.

**Code `12` (`0x83D68..0x84597`).** Gate phases 2 and 7 else RESET
(`0x83D76..0x83D93`). A pre-stage block: if `+0x8F < 3` and `+0x8D` and
`[team+0x826] == [0x57AAC]>>24`, calls `0x83B80` and fills
`0x58747..0x58757` with zeros (`0x83DA0..0x83DF0`). Timer
`+0x89 += delta` (`0x83DF4..0x83E0B`); stage 0..7, table `0x83D2C` (8 arms),
with a second table `0x83D4C` = `{0x742AB,0x7432F,0x742E7,0x74368,0x743A0,`
`0x742E7,0x743AE}` (7 arms, event sub-dispatch). Stage 0 stores
`[0x57AAF] = team side`, `[0x57A83] = rec`, `CALL 0x7876C`, waits
`+0x89 >= 0x3C` when `[0x4C32A] == 0` (`0x83E26..0x83E57`); a later arm
**installs code `4` on the selected record** at `0x844D3..0x844E7`
(`[team+0x7B2] = selected`, `EDX = 4, ECX = 1`).

**Code `13` (`0x84B00..0x84EEB`).** Gate phases 2 and 6 else
`FUN_0007DAB4` (`0x84B0B..0x84B1B`); the `+0x8F < 5` block re-asserts
`[0x57A83] = rec`, `CALL 0x7876C`, stages the ball `CALL 0x700F4` with the
ball triple and `CALL 0x73DC4` (`0x84B21..0x84B6B`). Timer
`+0x89 += delta`, stage 0..6, table `0x84AE4` = `{0x74BAD,0x74C48,0x74CBA,`
`0x74DF4,0x74E26,0x74E61,0x74EB1}`. Stage 0 `CALL 0x73E08`; when RESET runs
it installs code `0x13` on `[team+0x7B2]` (`EDX = 0x13`,
`0x84D89..0x84DAB`); tail RESET `0x84EC0`.

**Code `15` (`0x87CD0..0x880CB`).** Gate phase 5 else RESET
(`0x87CE0..0x87CE6`; body head is mis-decoded: bytes `55 83 EC 18 89 C5`
render as `IN/OR/LDS`). Timer `+0x89 += delta`; if `[0x57A49]>>24 == 1`
selects `[0x587D4]` else `[0x57A9F]` as the `+0x1C` call target, then calls
the **phase handler** `CALL [rec+0x1C]` with `EDX = &rec+0x4D, EBX = -1`
(`0x87CFC..0x87D29`) — the only stage-family code that re-enters the phase
table. `CALL 0x741B4`, `CALL 0x651F0`, stage 0..3, table `0x87CC0` =
`{0x77D6F,0x77DA8,0x77F4A,0x77FF5}`; stage 0 requires `+0x8D != 0` else
RESET and sets `[rec+0xAE] = 0x3C` (`0x87D6F..0x87D7C`).

**Code `17` (`0x84730..0x849AF`).** Gate phase 0xB else RESET
(`0x8473B..0x8474A`). Timer `+0x89 += delta`; stage 0..3, table `0x84720` =
`{0x74780,0x747E5,0x8483F,0x8494C}`. Stage 0 copies `+0x59` to `0x587FC`,
sets `+0x4D = -0x680`, `+0x51 = 0`, `[0xF370] = rec`, `+0x55 = ±0x50` by
`[rec+0x61]` sign, `CALL 0x8DCD4` and advances (`0x84780..0x847DF`).

**Code `20` (`0x84EEC..0x85213`).** No phase gate; zeroes/initialises the
input words `[0x4C114] = [0x4C118] = 0`, `[0x4C11C] = 0x9F0`,
`CALL 0x4C31C` (`0x84EF4..0x84F12`), re-asserts `[0x57A83] = rec` when
`+0x8F < 3`, timer `+0x89 += delta`, stage 0..6, table `0x84ED0` =
`{0x74F5C,0x75056,0x7512E,0x75153,0x75197,0x751E1,0x75206}`. Stage 0 stages
the ball (`CALL 0x700F4` with `0x5774C`), clears `[0x5781D]`,
`CALL 0x73E08`, copies the ball triple to `+0x4D`, `+0x55 -= 0xF0`,
`CALL 0x79B6C`, and clears `[team+0x7A6]+0x4D` (`0x84F5C..0x84FCA`).

## 3. Port: `fifa96_action_stage_*` / `fifa96_action_phase_select`

`include/fifa96_loader/fifa96_action_handlers.h` +
`src/fifa96_loader/fifa96_action_handlers.c` (caller-owned state, no globals,
negative `fifa96_err_t` for invalid arguments, no comments).

| original | port |
|---|---|
| phase gate `EAX=[0x57A4A]>>24; CMP/CMP; JNZ RESET` (`0x7DBD3`, `0x855FB`, `0x85DF2`, `0x83D7E`, `0x84B13`, `0x84743`, `0x87CE3`, `0x80A13`, `0x82723`) | `fifa96_action_stage_enter(&state, gates, gate_count, &out)`: `out.allowed = phase ∈ gates`, else `out.reset = 1` |
| timer `[+0x89] += [0x57A64]` (`0x7DC42`, `0x8561F`, `0x85E19`, `0x81CA3`, `0x82532`, `0x8273A`, `0x83E03`, `0x84B8A`, `0x8475D`, `0x87CFA`, `0x84F39`) | `fifa96_action_stage_tick(&state)`: 32-bit wrapping accumulate |
| advance `[+0x89]=0; [+0x92]++` (`0x7DC97..0x7DCB4`, `0x85805`, `0x85FB7`, `0x82799`, `0x83E57`, `0x847D3`) | `fifa96_action_stage_advance(&state, &out)`: `timer89 = 0`, `stage++` (byte wrap), `out.advance = 1` |
| terminal `if (+0x44) FUN_0007DAB4` (`0x7DFB2`, `0x85D61`, `0x83E57`) | `fifa96_action_stage_finish(&state, &out)`: `out.reset = occupied` |
| `+0x8F < 3` re-assert controlled record / `< 5` hold position (`0x85630..0x85651`, `0x84F20..0x84F2B`, `0x84B2A`) | `fifa96_action_stage_marker(marker, &set_leader, &hold)`: `set_leader = marker < 3`, `hold = marker < 5` |
| phase resolver `EAX = [0x57A4A]>>24; EAX<<2; ADD EAX,0x10794; EAX=[EAX]` (`0x6D9A8..0x6D9BA`) | `fifa96_action_phase_select(phase, table, count, &entry)`: caller-owned table lookup with a hard bounds check (the original has none — hardening divergence) |
| arm tables (`0x7DBB0`, `0x809F0`, `0x81C74`, `0x8250C`, `0x82700`, `0x83D2C`, `0x83D4C`, `0x84720`, `0x84AE4`, `0x84ED0`, `0x855B8`, `0x85DA0`, `0x87CC0`) and hand-off bodies | not ported (pointer tables/global FSMs); arm contents are cited per code in §2 |
| `0x6Dxxx`/`0x8Bxxx` phase-handler bodies | not ported (formation tables, camera/sound globals, cut bodies; open legs) |

## 4. Tests (`tests/test_stage_family.c`, suite 69 → 70)

* Layout `_Static_assert`s on all port struct offsets.
* `stage_enter`: single gate, two gates, phase 0, non-member → reset, NULL
  state/gates/out, `gate_count == 0`.
* `stage_tick`: accumulate, zero-extended word delta, 32-bit wrap (no UB),
  NULL.
* `stage_advance`: `+0x92` wrap `0xFF → 0`, timer zeroed, out stage, NULL.
* `stage_finish`: occupied → reset, clear → no reset, NULL.
* `stage_marker`: `0..2` leader+hold, `3..4` hold only, `5..` neither,
  negative marker, NULL out.
* `phase_select`: phase 0, phase 0x22 (last slot), 0x43 zero entry, 0x23
  error, `count == 0`, NULL table/out.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_stage_family.c src/fifa96_loader/fifa96_action_handlers.c
src/fifa96_loader/fifa96_entity_update.c` runs clean. `make test`: 69/69
before, **70/70 after**.

## 5. Errata (quoted)

* FU-76 §1.5 / open leg 1 "slots `0x2D..0x4F` consume `EDX` as a second
  argument; the target of a different, two-argument dispatcher; no static
  reader of that half was found (searches for operands `0x10780`/`0x107E0`
  are empty)" — **corrected**: the half is a 35-slot **phase** table at flat
  `0x110794` read at `0x6D9B3` (`ADD EAX,0x10794`) by `FUN_0006D920`, indexed
  by `[0x57A4A]>>24` and installed at `[rec+0x1C]`; `EDX` is the output
  pointer (`&rec+0x4D`), not the selector. The missed base is `0x2D`'s
  `0x10794` (FU-76 searched `0x10780` = slot `0x28` and `0x107E0` = slot
  `0x40`).
* FU-76 §1.4 slot `0x43` zero — **extended**: the zero is a phase-table
  entry (phase `0x16`), fixed by the loader to `0x10000` (INT3).
* FU-76 §2 class labels name 13 stage/transition slots (`01,02,09,0C,0D,0E,`
  `10,11,12,13,15,17,20`); the task brief's bucket count of 8 is not
  reproducible from FU-76 §2 (no partition is stated in that slice), so this
  doc derives the 13-slot superset and marks the exact 8 as an open leg
  (no guessed classification).
* FU-76 §2 code `0C` "INSTALL `0x0C` via `0x81BEB`" — **corrected**: the
  install at `0x81BEB` lies in code `0B`'s body (`0x81908..0x81C90`), which
  FU-76's own `0B` row already states; code `0C`'s body has no `0x7D9A4`
  call.
* Ghidra listing cuts recovered with `disassemble_bytes`/`read_memory`:
  `0x80A24..0x80A25` (`8A 85` head of `MOV AL,[EBP+0x92]` in code 09),
  `0x82780..0x8278A` (code 0E), `0x87CD7..0x87CDA` (code 15 head
  `55 83 EC 18 89 C5`), `0x6E046..0x6E058` (code 3D tail), `0x6E280+`
  (code 40/41 arm), `0x7DC89..0x7DCA7` and `0x7DEFE..0x7DF08` (code 01
  arms, not quoted).

## 6. Open legs

1. **The 8-slot bucket**: FU-76's internal stage/transition partitioning was
   not recorded in its doc; the derived family is the 13-class superset.
2. **`0x6Dxxx` formation data**: the tables `0x577D4..0x577DE`, the four
   resolver pointers `rec+8/+0xC/+0x10/+0x14`, the `0x1033A` 29-byte
   formation rows (`FUN_0006D9C4`) and the `0x105E7/0x105E8` byte tables are
   cited by address only.
3. **`0x8xxxx` timelines**: `[0x58818]`/`[0x58829]` semantics, the camera
   `[0xF328..]` triples, the `0x4C374/0x4C31C/0x4C324` engine calls, sound
   ids `0x974DC/0x974F0` and the `[0x5B680]` mode meanings are not derived;
   only the structure is.
4. **Arms**: the CS-table arms of `01,09,0C,0D,0E,10,11,12,13,15,17,20` are
   cited by address and count; full arm-by-arm decomposition is open except
   the entry blocks quoted above.
5. **Code `02`**: the phase-1 arm's `-x` mirror and the slot-only install 4
   are quoted; the remaining inline arms are cited at block level.
6. **Code `15`**: the listing head is mis-decoded; the body after
   `0x87D04` is cited from the recovered window only.
7. **`FUN_0007DAB4`** (RESET/chooser) internals are FU-76 open leg; the port
   models only the terminal condition (`+0x44`).
8. **`FUN_0006D920`'s four resolver pointers** and `0x4AFB8` (indexed lookup)
   are cited, not decomposed.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x6D920, 0x6D9C4, 0x7997C, 0x8CF60, 0x8D098, 0x7DBC0,
0x855F0, 0x85DE4; `disassemble_bytes` 0x7DFCC, 0x80A00, 0x81C90, 0x8251C,
0x82710, 0x83D68, 0x84B00, 0x84730, 0x84EEC, 0x87CD0, 0x6DE34, 0x6DE44,
0x6E1D0, 0x6DCC8, 0x6DD6C, 0x6DD9C, 0x6DF4C, 0x6E004, 0x6E1C8, 0x6E244,
0x6E05C, 0x88DC8, 0x88F4C, 0x890EC, 0x89110, 0x8922C, 0x89620, 0x89868,
0x89FA4, 0x8A798, 0x8B688, 0x8B874, 0x8B900, 0x7E230, 0x844C8, 0x84D80;
`read_memory` 0x110794 (140 B), 0x7DBB0, 0x809F0, 0x81C74, 0x8250C, 0x82700,
0x83D2C, 0x83D4C, 0x84720, 0x84AE4, 0x84ED0, 0x855B8, 0x855D4, 0x85DA0,
0x85DC8, 0x87CC0, 0x6E040, 0x7250C, 0x72700; `search_instructions` operand
`0x1079` (single match `0x6D9B3`), `7d9a4`/`7dab4` (69/54 sites), `CALL`
`0x1c]` (7 sites), `JMP` `CS:` (161 tables), `0x57a4a`; `get_function_xrefs`
0x6D920. Analysis-only outside the port: no tool, capture-rig, ISO or
Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`, `tests/test_stage_family.c`,
`CMakeLists.txt` (one library/test block). `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
