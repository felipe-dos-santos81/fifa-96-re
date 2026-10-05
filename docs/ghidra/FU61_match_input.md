# FU-61: the match input path — devices, sampler, records, scan and edge hand-off

Roadmap slice S4 of FU-60 §6 ("input event selection"), extended to the
sampler named in FU-60 §1 (`FUN_000456AF`) and to the record layer between
them. Derives how raw DOS-era device state becomes the per-player actions the
match code reads, and ports the host-agnostic mapping/edge core.

Result in one line: **the input path is a device-method table (`0x46570` IDs
into the LE-fixup table at flat `0x1067C4`, ten handlers: keyboard, mouse, two
analog-joystick variants, a third gameport variant, four `FUN_0009709F`
gameport pollers and a null) sampled at 30 Hz by `FUN_000456AF` from the INT-8
frame grant — each handler returns a byte (`low nibble` = direction bits,
`high nibble` = buttons) that is remapped through a runtime row selectable
from the 5×16 table at flat `0x10683E` (`row = [0x7DEC]`) and stored per device
in `0x4B1D4` — while the same sampler appends per-player 9-byte records into
the `0x57570` rings; the match drain's scan `FUN_00045F6A` peeks
(`FUN_000461C6` → `FUN_0006C6CE`) then consumes (`FUN_00046246` →
`FUN_0006C74C`) them, whose high-nibble code dispatches a switch that writes
the per-player current states `0x4B1C8[0..3]` (code 3/5), the per-event flags
`0x4B1DA` and player bitmaps `0x4B154` (code 1), the analog triple
`0x472D4/D8/DC` (code 4), per-player action bytes `0x4B19C+8` (code 6) and the
pause latch `0x67EC/0x67ED` (code 7); the scan then edge-detects
`0x4B1C8 → 0x4B1CC` (suppressing nonzero→nonzero), aggregates `0x67EC`
(held) / `0x67ED` (fresh) and latches them into `0x67F2`/`0x67F3`; the match
reads `0x4B1DA[code]` via `FUN_000451F1`, `0x4B1C8[player]` via
`FUN_0004511D`, the edge via the `0x45155` getter, and the player for an event
via `FUN_00045268` over `0x4B154`.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat link addresses as in FU-4/FU-58/FU-59/FU-60). All
  instructions quoted below were read back from Ghidra this slice
  (`disassemble_function`/`disassemble_bytes`/`read_memory`); the decompiler
  was used for callee characterisation where it succeeded.
* **Address convention (FU-59 errata, quoted).** Code immediates naming
  tables/globals are object-relative values carrying LE fixups; the loader
  adds the owning object's load base at run time. This slice **resolved the
  object bases for the input tables** with `read_memory` probes of the
  candidate flat addresses: object 1 base `0x10000`, object 4 base `0x100000`
  (FU-59 table). Encoded `0x683E`/`0x67C4`/`0x46570` therefore resolve to
  flat `0x10683E`/`0x1067C4`/`0x146570`, and the encoded method pointers
  `0x368B7…0x36AAA` to flat `0x468B7…0x46AAA`. Probes:
  `read_memory 0x683E` = zeros, `read_memory 0x10683E` = the 5-row nibble
  table; `read_memory 0x67C4` = zeros, `read_memory 0x1067C4` = ten encoded
  dwords; `read_memory 0x46570` = code (the flat `0x46570` *handler* of
  method 4 — a name collision with the device array at `0x146570`), and
  `read_memory 0x146570` = zeros (runtime-built device list).
* **Ghidra gaps met this slice (new errata).** (a) The method-handler table
  entries and five of the ten handlers had no Ghidra functions; they were
  decoded with `disassemble_bytes` at the resolved flat addresses (the
  decompiler was not used for them). (b) `FUN_00046246`'s switch jump table is
  embedded at `0x462FB` and mis-decoded; the six dwords were read from the
  instruction dump and hand-decoded, and the code-1/3/5 bodies at
  `0x46337..0x463A7` were left as undefined bytes by Ghidra and are
  hand-decoded from `read_memory 0x4634C 100` (below). (c) `FUN_000456AF`'s
  handler at flat `0x46570` starts mid-listing in `disassemble_bytes`; its
  prologue bytes are quoted from `read_memory 0x46570`.

## 1. Device abstraction: `0x46570` IDs, table `0x1067C4`, raw state

### 1.1 Device list build `FUN_00014E28` (input config; no args)

`decompile_function 0x14E28` (body `0x14E28..0x15123`): sets `[0x4F40]=0`,
`FUN_0009E907(6,9)`, then appends method IDs to `(&DAT_00046570)[n]`
incrementing `[0x4F40]`:

* always method `0` (keyboard);
* method `1` if `FUN_000CB504()` (mouse driver probe) is non-zero, and in that
  case sets the callback pointer `[0x12AFC] = FUN_000BB423`;
* then, if `FUN_0009701A()==0`: two `FUN_0001EF14` calls and the byte
  `[0x12B44]` select methods `3`+`4` (`& 3 == 3 && & 4 == 4`, both appended),
  or `3` alone (else, fatal `FUN_0001D940()`), or `4` alone (`& 4 == 4`);
* else four `func_0x0009705E()` probes append methods `5`,`6`,`7`,`8` for the
  non-zero probes.

`FUN_00014E18(i)` returns `0x46570[i]`; `FUN_00015444(id)` tests membership;
`FUN_00015474`/`FUN_000154B0` swap methods `2↔3` (keyboard-config variant).
The array is runtime-built (flat `0x146570` is zero in the image). Device
count bound: the append paths give at most `0`+`1`+`2`+`4` = 8, corroborating
the 4-bit-per-device packing in §2.3.

### 1.2 Method table (flat `0x1067C4`; LE-fixup populated)

`read_memory 0x1067C4 48` = ten encoded handler dwords, then zeros:

| method | encoded | flat handler | identity (evidenced) |
|---:|---:|---:|---|
| 0 | `0x368B7` | `0x468B7` | keyboard: reads byte flags at flat `0x112Bxx` (`0x12BC1`,`0x12BC2`,`0x12BDC`,`0x12BC3`,`0x12BDD`,`0x12BC4`,`0x12BC0`,`0x12BB5`,`0x12BEC`,`0x12BF4`,`0x12BEF`,…); returns direction bits `0x1/0x2` and buttons `0x10/0x20/0x40/0x80` (`disassemble_bytes 0x468B7`) |
| 1 | `0x36703` | `0x46703` | mouse: `[0x12AA4]/[0x12AA8]` (position; written by `FUN_000CB394` at `0xCB39D`) minus last `[0x6826]/[0x682A]`, compares ±0xC to set `[0x682E]` direction `8/4/0`, then position wrap/`FUN_000CB394` (`disassemble_bytes 0x46703`) |
| 2 | `0x36639` | `0x46639` | analog joystick A: axes `[0x12B30]`/`[0x12B34]` vs calibration `0x491D8/0x49208/0x491C0/0x491F0`; low nibble `8/4` (axis 0 low/high) and `1/2` (axis 1 low/high); buttons `[0x12B40]^0xF` → `0x10/0x20/0x40/0x80` (`decompile_function 0x46639`) |
| 3 | `0x364A7` | `0x464A7` | analog joystick B: same axes; button map folds two buttons into `0x10/0x20/0x40` (2-button variant) (`decompile_function 0x464A7`) |
| 4 | `0x36570` | `0x46570` | gameport variant: axes `[0x12B38]`/`[0x12B3C]` vs `0x4920C`/`0x491F4`, writes `[0x6832]`; prologue at `0x46570` from raw bytes (`read_memory 0x46570`) |
| 5 | `0x369A5` | `0x469A5` | `FUN_0009709F(0)`, returns `AL & 0xFF`, sets `[0x6810]=1` when bit `0x100` (`disassemble_bytes 0x469A5`) |
| 6 | `0x369E4` | `0x469E4` | `FUN_0009709F(1)`, same |
| 7 | `0x36A26` | `0x46A26` | `FUN_0009709F(2)`, same |
| 8 | `0x36A68` | `0x46A68` | `FUN_0009709F(3)`, same |
| 9 | `0x36AAA` | `0x46AAA` | null handler: returns 0 (`disassemble_bytes 0x46AAA`) |

The handler output byte layout is uniform: **low nibble = direction bits
(bit3/bit2 = axis0 below-low/above-high, bit1/bit0 = axis1 below-low/
above-high), high nibble = button bits**.

### 1.3 Where raw state lands (device-specific; documented, not ported)

* **Gameport**: the timing loop `IN AL,DX` / `LOOPNZ` at `0xCB615..0xCB64C`
  writes flat `0x112B30` (axis 0), `0x112B34` (axis 1), `0x112B38` (axis 2),
  `0x112B3C` (axis 3), `0x112B40` (buttons) — `disassemble_bytes 0xCB620`
  (`MOV [0x12B40],AL` at `0xCB635`, `MOV [0x12B30],EBP` at `0xCB63A`,
  `MOV [0x12B34],ESI` at `0xCB640`, `MOV [0x12B38],EDI` at `0xCB646`,
  `MOV [0x12B3C],EBX` at `0xCB64C`). The analog handlers read these; the four
  `FUN_0009709F(n)` handlers are the gameport equivalents.
* **Mouse**: position `0x12AA4/0x12AA8` and buttons `0x12AAC`; writer
  `FUN_000CB394` (`0xCB39D`, INT-33 wrapper; `FUN_000CB354` sets ranges,
  called by `FUN_000454D3` with `(0x20,0x20,0x80)`/`(0x20,0x20,0x40)`).
* **Keyboard**: byte flags at flat `0x112Bxx` read by method 0; the writer is
  not in the defined-instruction scope (INT-9/BIOS territory) — open leg.
  `0x12AFC` holds the mouse-present callback `FUN_000BB423` (§1.1).

## 2. `FUN_000456AF` — the 30 Hz sampler

### 2.1 Identity and cadence

Sole caller `0x49368` in `FUN_00049320`, the registered INT-8 frame callback:
it runs **once per granted 30 Hz frame**, immediately after `[0x731C]++`
(FU-60 §1). `get_xrefs_to 0x456AF` → exactly `0x49368`. Body
`0x456AF..0x45D0C`.

### 2.2 Entry (`0x456AF..0x4574A`)

```
0x456BD  CALL 0x6D470              ; replay/record sync guard -> 1 = skip
0x456C4  JNZ 0x45D04               ; (return)
0x456CA  CALL 0x9701A / 0x96E01    ; device/timer tick pair
0x456E4  CALL dword [0x12AFC]      ; mouse-present callback (FUN_000BB423)
0x456EA  CALL 0xCB789              ; raw input/time code -> [EBP-8]
0x456F2  if [0x67EE]==0: [0x67EE] = code   ; first-code latch
0x45703  if [0x6811] > 0: [0x6811]--      ; key-repeat countdown (30 Hz units)
0x45712  [0x6810] = 0; [0x680F] = 0
0x45724  [0x683A] = FUN_0004CAE4()        ; = [0x7DEC], mapping row
0x4572E  if (FUN_00037AE4()!=0 || FUN_00063FF0()!=0): [0x683A] = 0
```

`FUN_00037AE4`/`FUN_00063FF0` are the menu and load gates (FU-60 §2.2/§4.4
callers); during them the mapping row is forced to 0.

### 2.3 Per-device loop and mapping (`0x45751..0x45808`)

```
0x45754  CMP EAX,[0x4F40]           ; for i in 0..device count
0x4576F  EAX = [EAX*4 + 0x46570]    ; method id
0x45778  CALL [EAX*4 + 0x67C4]      ; handler() -> AL
0x45783  [EAX + 0x4B1D4] = AL       ; raw per-device state
0x4578C  AL = [i + 0x4B1D4] & 0xF
0x45799  EDX = [0x683A] << 4        ; row*16
0x457A2  EDX += low nibble
0x457B0  DL = [EDX + 0x683E]        ; table lookup (flat 0x10683E)
0x457B9  BL = raw & 0xF0 | DL
0x457BB  [i + 0x4B1D4] = BL         ; mapped state (high nibble preserved)
0x457C7  if method == 1:
0x457D0    if [0x6812] > 0:         ; mouse settle lockout
0x457DC      [i + 0x4B1D4] = 0
0x457E3      [0x6812]--
0x457F4    local_1c = mapped & 0xF0
0x457F9  else: [0x680F] |= mapped
```

The mapping table rows (`read_memory 0x10683E 64`):

| row | 16 bytes |
|---:|---|
| 0 | `00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F` (identity) |
| 1 | same identity |
| 2 | `00 02 01 03 08 0A 09 0B 04 06 05 07 0C 0E 0D 0F` |
| 3 | `00 04 08 0C 02 06 0A 0E 01 05 09 0D 03 07 0B 0F` |
| 4 | `00 08 04 0C 01 09 05 0D 02 0A 06 0E 03 0B 07 0F` |

Rows 0/1 are identity; rows 2..4 are fixed permutations of the low nibble
(they transform the direction bits only; the button nibble passes through).
The row index is `[0x7DEC]`, computed per display update by `FUN_0004D2D4`
(`DAT_00007dec = FUN_0004CA70(...)`) from the current view (`[0x7DC8+0xC]`
index into `0x8B64`, or camera position `[0x7DC8+0x58]`; `FUN_0004CA70`
returns 0..4). No label for the orientations is asserted (open leg).

`0x6810` is a "gameport special state" flag set by the `0x469A5..0x46A68`
handlers (bit `0x100`), consumed in the keyboard branch below.

### 2.4 Hand-off branches (`0x4580D..0x45D04`)

* `[0x6816]==0` and the repeat-window check (`[0x6810]==0 || [0x6811]!=0`)
  take the **keyboard-navigation chain**: a priority ladder over the flat
  `0x112Bxx` flags (`0x12BE7`,`0x12BB7`,`0x12BBC`,`0x12BC6`,`0x12BC5`,
  `0x12BAB`,`0x12BA6`..`0x12BAA`,`0x12BFB`,`0x12BFC`,`0x12BE8`) calling
  `FUN_0006CA17(2)` + `FUN_0006C82C` around the `[0x680B]` gate; several arms
  set `[0x6811]=0x1E` (30) or `[0x67EE]`.
* When `FUN_00037AE4()==0 && FUN_00063FF0()==0` and the per-device mapping
  runs (`0x459xx..0x45C1C`, decompile): a 4-slot player array `0x4B1C4` is
  built by matching each player index against the device id
  `FUN_0003B6A4(slot, [0x67FF])` (table `0x4AE98`, 12-byte records: encoded
  pointer + mapped slot); the last unmatched slot receives
  `[0x680F] | local_1c` (`0x45CBB` region), then `FUN_0006CABE(...)`.
* Otherwise `0x681A` is packed from the per-device states
  (`0x45BD3..0x45C1C`):

```
[0x681A] = 0
for i in 0..[0x4F40]:
  v = ([i+0x4B1D4] & 0xF) >> 2          ; SAR 2
  if ([i+0x4B1D4] & 0xF0) != 0: v |= 4
  [0x681A] |= v << (i*4)
CALL 0x6CBAD([0x681A])
```

* Mouse-absolute and keyboard-record arms call `FUN_0006CAEB(E AX=0x12AA4,
  EDX=0x12AA8, EBX=0x12AAC)` (`0x45C2A`) or `FUN_0006CBDA` (`0x4585E`).

## 3. The record layer (`0x575xx`, `0x6Cxxx`, `0x6Dxxx`)

### 3.1 Producer `FUN_0006C595` and the recorders

`FUN_0006C595(event, param)` (`decompile_function 0x6C595`) is the shared
producer behind `FUN_0006CA17`/`0x6CAEB`/`0x6CBAD`/`0x6CABE`/`0x6CBDA`
(all funnel there with `EAX` pass-through):

* `[0x57538]==0` → returns 0 (recording inactive);
* `in_AL < 8` → `player = FUN_0006D1E5()`; builds a 9-byte record at
  `0x5757A`: `[0x5757A] = (player & 0xF) | event<<4`, `[0x5757B] =
  [player*0xAA9 + [0x57570] + 0xAA4]` (per-player repeat counter, bumped by
  `FUN_0006C82C`), `[0x5757C] = param` dword; appends via `FUN_0006BF4D`
  (`FUN_0006BEF9` copies the 9 bytes); if `[0x5753C]!=0` bumps the write count
  `[player*0xAA9 + [0x57570] + 0xA9C]`, clamping at 300 and setting
  `[0x57540]=1` on overflow; returns 1.

Ring geometry (all offsets into `[0x57570] + player*0xAA9`): 300 × 9-byte
records at `+0`, read index `+0xA8C`, pending count `+0xA98`, write count
`+0xA9C`, repeat counter `+0xAA4`. `FUN_0006BF4D`/`FUN_0006BEF9`
(`disassemble_bytes 0x6BEF9`) append/copy 9 bytes.

### 3.2 Consumers: peek `FUN_0006C6CE`, consume `FUN_0006C74C`

`disassemble_function 0x6C6CE`/`0x6C74C`:

* peek: `[0x57538]==0` or `[+0xA98]==0` → 0; else copy 9 bytes from
  `base + [+0xA8C]*9` to the caller buffer and return `byte0 >> 4`;
* consume: same, then `[+0xA8C] = ([+0xA8C]+1) % 300`, `[+0xA98]--`, return
  `byte0 >> 4`.

`FUN_000461C6(player)` (peek wrapper, `disassemble_function 0x461C6`) adds
two gates when `FUN_0006D1B2() > 1`: `[0x67FB]` returns -1, and
`FUN_0006C3F5()`/`FUN_0006D828()` (device/link checks) return -1 when
inconsistent. `FUN_00046246(player)` is the consume+decode (below).

### 3.3 Supporting helpers (identity only)

`FUN_0006D1B2`: 1 if `[0x5753C]==0`, else 2 (player count). `FUN_0006D1E5`:
0 if `[0x5753C]==0`, else `FUN_0006B765()` (local player). `FUN_0006D470`
(`0x456BD`): 1 when `[0x5753C]!=0` and the two players' `+0xA98` counts differ
by more than 3 (replay/lockstep catch-up), with the `[0xE120]` toggle.
`FUN_0006D828` (`[0x57540]==0 && [0x57558]==0` → 0 else 1), `FUN_0006C3F5`
(`[0x57530]` callback if set), `FUN_0006D510` (`[0x57568]`). `FUN_0004559E`
(input teardown): when player count > 1 sets `[0x67FB]=1` and calls
`FUN_0001771C`/`FUN_0001CFC4`/`FUN_0001CF58`.

## 4. The scan `FUN_00045F6A` and the decode `FUN_00046246`

### 4.1 `FUN_00045F6A` (drain step, called at `0x49BA4`)

Full `disassemble_function 0x45F6A` (140 instructions); model:

```
if [0x67FB] != 0: return 0
[0x67ED] = 0; [0x67EC] = 0
memset(0x4B1DA, 0, 0x12)
memset(0x4B154, 0, 0x48)
if [0x6816] == 0:                      ; normal in-match
  for i in 0..[0x6803]:                ; 0x45FCB
    r = FUN_000461C6(i)                ; peek
    if r < 0: FUN_0004559E(); return 0
    if r == 0: return -1               ; lockstep abort
  for i in 0..[0x6803]:                ; 0x46033
    r = FUN_00046246(i)                ; consume+decode
    if r == 0: FUN_0004559E(); return 0
    if r < 0 ... (decode contract, below)
else:                                  ; 0x46085.. (suspended/hold)
  for i in 0..[0x6803]:
    r = FUN_000461C6(i)
    if r < 0: FUN_0004559E(); return 0
    if r != 0 and FUN_00046246(i) == 0: FUN_0004559E(); return 0
                                    ; 0x460C0: 4-player edge block
for i in 0..4:
  edge = 0x4B1C8[i]
  if 0x4B1D0[i] != 0 && edge != 0: edge = 0
  0x4B1CC[i] = edge
  0x4B1D0[i] = 0x4B1C8[i]
  0x67ED |= 0x4B1CC[i]
  0x67EC |= 0x4B1C8[i]
[0x67F2] |= [0x67EC]
[0x67F3] |= [0x67ED]
return 1
```

Call-site context (FU-60 §3): `0x49B90 CALL 0x46177` clears `0x4B1EC` and the
latches before the loop; `0x49BA4 CALL 0x45F6A` runs per drained frame and a
negative return aborts the whole drain (`0x49BAD` subtracts only processed
frames).

### 4.2 `FUN_00046246` — record decode

`disassemble_bytes 0x46246` plus the hand-decoded switch. Preamble: player
struct pointer `player*0x14 + 0x4B19C`; `code = (consume(player, buf) )`;
`if code == 0 → 0`; `if code == 7` → `[0x67ED]=0x40; if [0x67EC]!=0
[0x67ED]=0; [0x67EC]=0x40; return 1` (pause latch); else if
`FUN_0006D1B2() <= 1 || buf.frame == struct[+4]` → `struct[+4]++` and switch
on `code-1` (table dwords at `0x462FB`: `0x4638B, 0x4648B, 0x46337, 0x463E0,
0x46337, 0x46437` + object base `0x10000`):

| code | body | effect |
|---:|---|---|
| 1 | `0x4638B` | payload dword (`buf[5..8]`) = event code `ev`; when `ev==1`, or `ev!=1` and `FUN_000377B0(player)` non-zero, and `ev` in `0..0x11`: `0x4B1DA[ev]=1`, `0x4B154[ev] |= 1<<player`, `0x4B1EC[ev]=1`; return 1 |
| 2 | `0x4648B` | no-op; return 0 |
| 3, 5 | `0x46337` | for i 0..3: if `FUN_0003B6A4(i, [0x67FF]) >= 0`: `0x4B1C8[i] = buf[5+i]` (`0x4636E`); else `0x67EC |= buf[5+i]` (`0x4637E`); return 1 |
| 4 | `0x463E0` | if players!=1 or gate: `0x472D4 = p & 0x3FF`, `0x472D8 = (p<<0xC)>>0x16`, `0x472DC = (p<<8)>>0x1C`, `0x67EC |= p>>0x18`; return 1 |
| 6 | `0x46437` | for i 0..5: nibble = payload >> 4i & 0xF; `struct[+8+i] = nibble<<2`; if `nibble & 4`: `|= 0x40`; return 1 |

The code-1/3/5 bodies at `0x46337..0x463A7` were undefined in Ghidra;
`read_memory 0x4634C 100` decodes them by hand (copies above). The
`0x4B1C8[i] = buf[5+i]` store at `0x4636E` is the only writer of the edge
detector's current-state array found anywhere in the program (a full
`search_instructions operand 4b1c8` sees only the reads at `0x45140`,
`0x460DD`, `0x4611F`, `0x46140` and the zeroing at `0x45DFB`).

Per-player struct `0x4B19C + player*0x14`: `+4` frame stamp, `+8..+0xD` and
`+0xE..+0x13` two 6-byte action fields (cleared by `FUN_00045390` and
`FUN_00045D0D`; the second field has no writer in the switch — open leg).

### 4.3 Event latches and scan state (match init/reset)

* `FUN_00045390` (called from match setup `0x49450`): `[0x6816]=0`,
  `[0x67FB]=0`, `[0x6803]=FUN_0006D1B2()`, `[0x67FF]=FUN_0006D1E5()`,
  validates `1 <= [0x6803] <= 2` and `0 <= [0x67FF] < [0x6803]` via
  `FUN_000CBBE8`, clears the per-player structs, sets `[0x6812]=0x3C` (mouse
  lockout 60), `[0x6811]=0`, `[0x67EE]=0`, writes `0x72D4/D8/DC` from
  `DAT_0000681E`/`DAT_00006822`, calls `FUN_000454D3` (mouse range) and sets
  `[0x6807]=FUN_0006D510()/2`.
* `FUN_00045D0D` (called at `0x4944B` and teardown): clears `0x4B1C8/CC/D0`
  for 4 players (`0x45DF4..0x45E05`), clears the per-player structs, resets
  `0x472D4/D8/DC`, does the pause/suspend probes (§2.3 handlers) and
  `FUN_00046177` (clear `0x4B1EC`, `0x67F3`, `0x67F2`, decrement `0x67F4`),
  then sets `[0x6816] = 1` iff `FUN_00037AE4()==0 && FUN_00063FF0()==0 &&
  FUN_00051AB8()!=0`, and calls `FUN_00049308`/`FUN_000492E8` (pace
  hold/reset, FU-60 §1).

## 5. What the match reads (hand-off)

| getter | evidence | semantics |
|---|---|---|
| `FUN_000451F1(code)` | `disassemble_function 0x451F1` | `0x4B1DA[code]`, 0 when `code ∉ 0..0x11`, when `code==1 && [0x6803]!=2`, or `code==3 && FUN_0006D1B2()==1` |
| `FUN_0004511D(player)` | `disassemble_function 0x4511D` | `0x4B1C8[player]`, or 0 when `FUN_00051AB8()!=0` (suspend) |
| getter `0x45155` (unnamed) | `disassemble_bytes 0x45155` | `[0x67ED]` when `FUN_0004B454()!=0` (`[0x4C32A]`), else `0x4B1CC[player]` |
| `FUN_00045191(player)` | `disassemble_function 0x45191` | single-player (`FUN_0006D1B2()==1`) event-code match against `[0x67EE]`; 0 in two-player |
| `FUN_00045268(code)` | `decompile_function 0x45268` | lowest player `p < [0x6803]` with bit `p` set in `0x4B154[code]`; 0 when none or `code ∉ 0..0x11` |
| `FUN_000452D8(code)` | `disassemble_bytes 0x452D0` | `0x4B1EC[code]` with the same `0..0x11` bounds |

**Event chains A/B/C** in `FUN_00049B28` (`disassemble_bytes 0x49BD2 720`):
event `2` first (`0x49BE1`); the phase gate `[0x4B380]>>24 ∉ {0xB,0xF}` at
`0x49BF3` (both values since verified in the listing) selects the frame path;
then `FUN_0004B454()` (`[0x4C32A]`) picks chain A (`!=0`): `7, 0xA, 0xB, 8,
0xF` (`0x49D03..0x49D7A`) or chain B (`==0`): `9, 4, 5, 6, 0xF, 0xB`
(`0x49D7F..0x49DF5`); chain C (`0x49E0A..`) re-tests `7, 0xA, 0xC, 0xD, 0xE,
8, 1, 3`; each hit sets `EDX = code` and falls to `0x49EB9` (dispatch via
`FUN_00045268`/`FUN_00037798`/`FUN_00038004`, FU-60 §3). The event codes are
therefore per-player input events (e.g. codes 4/5/6/9 are the analog/action
records of §4.2 code 4/6), not key codes.

## 6. Key structures/globals (encoded, as Ghidra displays; FU-59 errata)

| address | width | evidence | role (evidenced only) |
|---|---|---|---|
| `0x4F40` | dword | `FUN_00014E28` writes, `0x45754`/`0x45BC9` loop bounds | active device count (≤ 8) |
| `0x46570` | dword × count | `FUN_00014E28`/`0x15444`/`0x15474`/`0x154B0` | per-device method IDs; flat `0x146570` |
| `0x67C4` | dword × 10 | `0x45778` handler call; `read_memory 0x1067C4` | method handler table (LE fixups); flat `0x1067C4` |
| `0x683E` | 5 × 16 bytes | `0x457B0`; `read_memory 0x10683E` | direction remap rows 0..4; row = `[0x683A]` |
| `0x683A` | dword | `0x45729`, `0x45740` | mapping row for this frame (from `FUN_0004CAE4` = `[0x7DEC]`; 0 in menus) |
| `0x7DEC` | dword | `FUN_0004CAE4`, `FUN_0004D2D4` write | row input (view/camera via `FUN_0004CA70`) |
| `0x4B1D4` | byte × 8 | `FUN_000456AF` writes; flat `0x14B1D4` | mapped per-device state |
| `0x4B1C8` | byte × 4 | `0x4636E` write, `0x45140`/`0x460DD` reads | per-player current state (edge source) |
| `0x4B1CC` | byte × 4 | `0x460E6`/`0x46115`; `0x4517C` read | per-player rising edge |
| `0x4B1D0` | byte × 4 | `0x460EF`/`0x46128` | previous per-player state |
| `0x67EC` / `0x67ED` | byte | `FUN_00045F6A` aggregate; `FUN_00046246` pause/analog | held-any / fresh-any |
| `0x67F2` / `0x67F3` | byte | `0x46153`/`0x4615E` latches; `FUN_00046177` clear | sticky held/fresh |
| `0x4B1DA` | 0x12 bytes | `FUN_00045F6A` memset, `0x463B4`, `FUN_000451F1` | per-event-code global flags |
| `0x4B154` | 0x12 dwords | `FUN_00045F6A` memset, `0x463CB`; `FUN_00045268` | per-event-code player bitmaps |
| `0x4B1EC` | 0x12 bytes | `0x463D4`, `FUN_00046177` clear, `0x452FC` | per-event-code latches |
| `0x4B19C + p*0x14` | 0x14 bytes | `0x46257`, `0x46316`, `0x4646D` | per-player: `+4` frame stamp, `+8..+0xD`/`+0xE..+0x13` action bytes |
| `0x4B1C4` | byte × 4 | `FUN_000456AF` per-player fill | selected device states for the 4 player slots |
| `0x6812` | dword | `=0x3C` in `FUN_00045390`, `0x457DC` | mouse settle lockout frames |
| `0x6811` / `0x6810` | dword / byte | `0x45703`/`0x45712`, `0x469A5` set | key-repeat countdown / gameport special flag |
| `0x681A` | dword | `0x45BFF..0x45C1C`, `FUN_0006CBAD` | 4-bit-per-device packed state |
| `0x67EE` | dword | `0x456FE` latch, `FUN_00045191` | first raw input code |
| `0x6803` / `0x67FF` | dword | `FUN_00045390` | player count / local device id |
| `0x6816` / `0x67FB` | dword | `FUN_00045D0D`, scan | input-hold / disabled flags |
| `0x57570` | dword ptr | `FUN_0006C595`, `FUN_0006C6CE`/`0x6C74C` | per-player record ring base (allocated) |
| `0x57538`/`0x5753C`/`0x57540` | bytes/dword | producers/consumers | recording on / player count / ring overflow |
| `0x12B30/34/38/3C`/`0x12B40` | dwords/byte | gameport writer `0xCB63A..0xCB64C` | raw gameport axes/buttons (flat `0x112Bxx`) |
| `0x12AA4/AA8/AAC` | dwords | `FUN_000CB394`, `0x45C2A` | raw mouse position/buttons |
| `0x12Bxx` | bytes | handler `0x468B7` reads | keyboard flag table (writer open leg) |
| `0x472D4/D8/DC` | 2×dword, byte | `0x463FE..0x4641A`, `FUN_00045390`/`0x45D0D` | analog absolute axes + button latch |

## 7. Port: `fifa96_input`

`include/fifa96_loader/fifa96_input.h` +
`src/fifa96_loader/fifa96_input.c` (caller-owned struct, no globals, no
comments, `-fifa96_err_t` for invalid arguments). Scope: the host-agnostic
core of the mapping/scan path; device I/O and the record ring are documented,
not ported.

| original | port |
|---|---|
| `0x45799..0x457C0` `(raw & 0xF0) \| row[raw & 0xF]` | `fifa96_input_map` (caller supplies the 16-byte row; NULL → `-FIFA96_ERR_INVALID`) |
| `0x45BD3..0x45C1C` pack `v=((s&0xF)>>2)` + button bit into 4-bit slots | `fifa96_input_pack` (0 ≤ count ≤ 8, NULL/bounds → `-FIFA96_ERR_INVALID`, output untouched on error) |
| `0x460C0..0x46164` edge + aggregates + latches | `fifa96_input_update` (caller supplies `current[4]`; struct holds `prev`/`edge`/`held`/`fresh`/latches) |
| `0x451FF..0x45255` `0x4B1DA` lookup + code 1/3 gates | `fifa96_input_event` |
| `0x45268..0x452CC` player bitmap scan | `fifa96_input_code_player` |
| `0x457D0..0x457F7` method-1 settle lockout | `fifa96_input_lockout` |
| `FUN_00046177` latch/`0x4B1EC` clear | not modelled (the port's `init` zeroes latches) |
| device handlers `0x468B7..0x46AAA`, gameport/mouse/keyboard I/O | not modelled (device-specific) |
| record rings `0x57570`, `FUN_000461C6`/`FUN_00046246` decode | not modelled (replay engine) |
| keyboard-navigation chain `0x4580D..0x45Axx` | not modelled |

Tests (`tests/test_input.c`, suite 49 → **50**): init zeroing; identity rows
0/1 and the evidenced rows 2/3/4 (pair-swap `00 02 01…`, rotate
`00 04 08…`, bit-reverse `00 08 04…`) with high-nibble preservation;
boundary raws `0x00/0xFF`; NULL handling for every entry point; pack of
direction buckets (0..3) + button bit into 4-bit slots, count 0/8 accepted,
count 9/−1 rejected with output untouched; rising-edge detection, held
suppression, release-then-repress, nonzero→nonzero suppression, all-player
aggregates and sticky latches; event lookup bounds and the code-1
(2-player) / code-3 (not single-player) gates; bitmap player scan with the
`players` bound; lockout zeroing/decrement/no-op at 0 and negative. ASan+
UBSan build of `test_input` is clean.

## 8. Errata (quoted)

* FU-60 §3: "`FUN_00045F6A` … clears/sets the `0x4B1DA` (0x12-byte) event
  array via `FUN_000461C6`/`FUN_00046246`" — **refined**: `FUN_00045F6A`
  itself clears `0x4B1DA` (`0x45F9E`) and `0x4B154` (`0x45FAE`); `FUN_000461C6`
  only peeks (no writes); `FUN_00046246` consumes and writes `0x4B1DA[ev]`
  (`0x463B4`), `0x4B154[ev] |= 1<<player` (`0x463CB`) and `0x4B1EC[ev]`
  (`0x463D4`) in event code 1.
* FU-60 §1/§3: "`FUN_000456AF` … walks `DAT_00012xxx` key flags and
  `[0x46570]` method handlers" — **confirmed and expanded**: `0x46570` is the
  per-device method-ID array into the LE-fixup table at flat `0x1067C4`;
  `FUN_000456AF` is the 30 Hz device sampler and record producer, not just a
  key scanner; the `0x12Bxx` flags are the method-0 (keyboard) handler's
  table, read through the handler.
* FU-60 §5 table: "`0x4B1DA` … per-event-code input flags" and "phase
  `[0x57A4A]>>24`" — **confirmed**; add that per-event player selection lives
  in `0x4B154` (bitmap) read by `FUN_00045268`, and the per-player current
  state in `0x4B1C8`.
* FU-59/FU-60 address convention — **applied** to the input tables: the
  encoded `0x683E`/`0x67C4`/`0x46570` and method pointers resolve through
  object bases `0x100000`/`0x10000`; the FLAT reads that prove it are in the
  Method section.
* Ghidra artifact: `get_function_by_address 0x452FC`/`0x45155`/`0x4517C`
  return no function although the code is defined-adjacent to
  `FUN_00045268`/`FUN_0004511D`; they are quoted from `disassemble_bytes`.
  `FUN_00046246`'s decompile fails ("bad instruction data") at its embedded
  switch table; the listing + hand decode above is authoritative.

## 9. Open legs

* **Keyboard flag writer**: who fills flat `0x112Bxx` (INT-9 vs BIOS poll) is
  not derived; `[0x12AFC]=FUN_000BB423` is only placed as the mouse-present
  callback.
* **Mapping row meaning**: the permutations in rows 2..4 and the view/camera
  path through `FUN_0004CA70` (`0x8B64` record `+0x44`/`+8`, or
  `[0x7DC8+0x58]` ranges) are placed mechanically; no label for an
  orientation/view is asserted.
* **`[0x6810]`, `[0x6811]`, `[0x67EE]` semantics**: read/written sites are
  enumerated (`0x469A5..0x46A68`, `0x45703`, `0x45191`); their meaning in the
  keyboard navigation chain is not decomposed (the chain is not ported).
* **Record engine roles**: `[0x57538]`/`[0x5753C]`/`[0x57540]`/`[0x57558]`
  and the `0x6D5D5`/`0x6D142`/`0x6D424` family are only characterised through
  their input call sites; whether this is demo replay, link play or both is
  not asserted.
* **`FUN_00046246` code-2 is a no-op** and code 6's second action byte field
  (`0x4B19C+0xE`) has no writer in the switch; its consumer is not derived.
* **`FUN_000377B0`**: event-delivery gate (menu/screen checks + `0x4B028`
  compare) is decompiled but not decomposed.
* **`0x4B1C4` consumers** beyond the sampler's own fill are not enumerated.
* **`FUN_00045F6A`'s two-loop mode** (`0x6816==0`) is the peek-all-then-
  consume-all lockstep; the peek loop's `-1` return is consumed by the drain
  abort (FU-60 §3) and the decode loop's zero return by `FUN_0004559E`, both
  quoted from the listing only (no register-level contract asserted).

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`decompile_function` 0x456AF, 0x45F6A, 0x451F1, 0x461C6, 0x46246, 0x4559E,
0x4CAE4, 0x4CA70, 0x4D2D4, 0x37AE4, 0x63FF0, 0x6D470, 0x6D1B2, 0x6C6CE,
0x6C74C, 0x6C3F5, 0x6D828, 0x6C595, 0x6D1E5, 0x6D219, 0x6D510, 0x14E28,
0x15444, 0x15474, 0x154B0, 0x14E18, 0x46639, 0x464A7, 0x377B0, 0x46177,
0x4511D, 0x45191, 0x45268, 0x45390, 0x45D0D, 0x4557C, 0x4B454;
`disassemble_function` 0x45F6A, 0x461C6, 0x451F1, 0x4511D, 0x45191, 0x6C6CE,
0x6C74C; `disassemble_bytes` 0x456AF, 0x457CF, 0x45B90, 0x45D00, 0x46246,
0x46313, 0x4634C, 0x468B7, 0x46703, 0x46570, 0x469A5, 0x469E4, 0x46A26,
0x46A68, 0x46AAA, 0x452D0, 0x45155, 0x6BEF9, 0xCB620, 0x454D3, 0x49BD2;
`read_memory` 0x683E, 0x10683E, 0x67C4, 0x1067C4, 0x46570, 0x146570, 0x14B1C4,
0x5B1C4, 0x4B1C4, 0x4B1DA, 0x4634C, 0x10687E, 0x12B2C, 0x14AE98, 0x4AE98;
`search_instructions` operand `4b1c8`, `4b1cc`, `4b1d4`, `4b154`;
`get_xrefs_to` 0x456AF, 0x45F6A, 0x451F1, 0x46570, 0x4F40, 0x6803, 0x67FB,
0x683E, 0x4B1D4, 0x4B1C8, 0x4B154, 0x4B1EC, 0x67ED, 0x67EC, 0x67F2, 0x6812,
0x6816, 0x12B30, 0x12B40, 0x12AA4, 0x7DEC, 0x57570, 0x57538, 0x57558;
`get_function_by_address` 0x456AF, 0x45F6A, 0x4517C, 0x45155, 0x452D0,
0x45191, 0x452FC; `get_current_program_info`.

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change. Port write
set: `include/fifa96_loader/fifa96_input.h`,
`src/fifa96_loader/fifa96_input.c`, `tests/test_input.c`, `CMakeLists.txt`
(one library/test block). `make test`: 49/49 before, **50/50 after**;
ASan+UBSan `test_input` clean (`cc -fsanitize=address,undefined -Iinclude
tests/test_input.c src/fifa96_loader/fifa96_input.c`). `game/FIFAPCCD96.iso`
untouched; `fifa96.rep/**` churn not staged.
