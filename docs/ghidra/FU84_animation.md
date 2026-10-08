# FU-84: the animation selector, row table and per-frame driver

Follow-on to FU-82 §2.1 (the selector `FUN_0006E598` cited by address) and
FU-83 §3.2 (`0x6E598` cited as the "animation selector"): fully derive the
selector, the 9-byte animation-row table behind `[0x57588]`, the frame-record
model, the per-frame driver that advances frames, the `[0x57A38]` pointer
family, and the presentation-side consumer — and port the clean row/driver
model as `fifa96_animation_*`.

Result in one line: **the row table is static object-4 data at flat `0x10EF00`
(111 rows of 9 bytes) installed into `[0x57588]` by `0x73D91
MOV dword [0x57588],0xEF00` (object-relative encoded; runtime flat `0x10EF00`);
each row is `{id, last_frame, flags, next_id, frame_table*, sprite_bank}` with
5-byte frame records `{duration, aux, sprite}`; `FUN_0006E598` is the
side-sensitive selector (`EAX=rec, EDX=id16, EBX=frame index`) that clamps the
id to `0..0x6E`, stores the row at `[rec+0x28]`, resolves the frame via
`FUN_0006E490` and fans the row flags to `+0x43/+0x44/+0x45/+0x46/+0x3F`;
`FUN_0008DF84` is the plain selector (no side gate) with helper
`FUN_0008DF00`, and `FUN_0006E724`+`FUN_0006E518` are the same machine for the
secondary row at `[rec+0x7A]`; the per-frame driver `FUN_0008E008` accumulates
`[0x57A64]<<4` in word `[rec+0x32]`, and when `[rec+0x32] >= [rec+0x34]`
(frame duration) advances the frame index `[rec+0x3D]` by the signed turn
`[rec+0x46]` (±1 from heading-vs-facing), follows row byte `+3` as the
successor id, and re-selects through `FUN_0008DF84`/`FUN_0008DF00` (which zero
the accumulator); the sprite-frame resolver `FUN_00078FAC` (called from the
draw paths at `0x563C7` and `FUN_00057158`) consumes the row's frame table,
frame index, direction `0..7`, row `+8` bank index and frame `+4` sprite
offset; `[0x57A38]/[0x57A3C]/[0x57A40]` are the paired byte tables
`0x10F394/0x10F3A4/0x10F3B4` installed by `FUN_00073CD0` at match init.**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). `disassemble_function`/`disassemble_bytes` are the
  citation source; bodies the listing mis-decodes (`0x6E724` RNG tail,
  `0x4B020` thunk, `0x563C7` draw path, `0x8E008` flag-gated block) were
  decoded linearly with `ndisasm -b32` from the flat image (FU-79 method) — no
  decompiler output is quoted.
* **Address mapping (FU-76/FU-81, restated).** Code/function addresses equal
  true link addresses; a **data immediate** `A` is storage at flat
  `A+0x100000` (so `[0x57588]` is `0x157588`, `[0x57A38]` is `0x157A38`);
  stored code pointers and inline `CS:` tables are object-1 relative and
  resolve through `+0x10000`. An immediate stored *into memory* follows the
  same object rule: `MOV dword [0x57588],0xEF00` (`0x73D91`) stores the
  object-4-relative address whose flat form is `0x10EF00` — and flat
  `0x10EF00` holds the coherent 111-row 9-byte table enumerated in §3.1
  (ids equal indices, frame pointers `0xE2xx..0xEExx`, flag/next fields
  matching the selector and driver below).
* **Data cross-check.** The Ghidra program memory used here matches
  `tools/fifa96_le.py`'s parse of the retail `FIFA96.EXE` at every queried
  address (row table, `0x10F394` family, `0x110794` phase table, code). The
  `/tmp/opencode/fifa96_le.bin` currently on disk at the MCP
  `executable_path` is a **different build** (object-1 bytes differ; object-4
  stored pages zero) and was not used for any quote (open leg 10).
* Every numeric claim is quoted from the listings/parses; unproven items are
  open legs (no guessed labels).

## 1. `FUN_0006E598` (`0x6E598..0x6E713`, 112 insns) — the side-sensitive selector

Inputs: `EAX = rec`, `EDX = animation id` (16-bit; only `DX` is read),
`EBX = frame index` (only `BX`, sign-extended at the helper call). `ECX` is
**not** an input: it is overwritten by the side computation at `0x6E5B3`.

```
0x6E59C  if ([rec+0x8D] == 0) {
0x6E5A9     side = ([0x57754] > 0)                 ; ball z sign
0x6E5B5     team = [rec]
0x6E5C8     if (side == [team+0x826]) {
0x6E5CC..0x6E5FE   if ((int16)EDX in {0x2C,0x2D,0x2E,0x35,0x37,0x39,0x3B,0x5C})
                       [0x57A6C] = 1 else [0x57A6C] = 0
             }
         }
0x6E608  if ([0x57A4A]>>24 == 2 || DX != 0) goto 0x6E68E
         ; id 0 and phase != 2: continue/reroll the current animation
0x6E622  EDI = [rec+0x28]                          ; current row
0x6E625  if (EDI == 0) goto 0x6E659 (RNG)
0x6E629  CL = byte[EDI]                             ; row byte 0 = current id
0x6E62D  if (CL == 0) goto 0x6E653
0x6E631..0x6E655  EAX = (CL in {0x62,0x63,0x64,0x65}) ? 0 : 1
0x6E657  if (EAX == 0) goto 0x6E687
0x6E659  CALL 0x92AC8                               ; RNG side effect (stream advance)
0x6E687  EAX = [rec+0x28]; EDX = DL = byte[EAX]     ; id = current row byte 0
0x6E68E  if ((int16)DX < 0 || (int16)DX >= 0x6F) DX = 0
0x6E69D  EDX = [0x57588] + DX*9
0x6E6B1  [rec+0x28] = EDX                           ; row pointer
0x6E6B4  AL = byte[EDX+2]                           ; row flags
0x6E6BC  [rec+0x45] = AL & 4
0x6E6C4  [rec+0x43] = AL & 2
0x6E6CC  [rec+0x44] = AL & 1                        ; terminal/reset gate
0x6E6D8  [rec+0x46] = 1                             ; default turn/advance sign
0x6E6E0  [rec+0x3F] = (AL & 0x10) ? 2 : (AL & 0x20) ? -2 : 0
0x6E6FE  EDX = &rec+0x28; EBX = (int16)BX; EAX = rec
0x6E706  CALL 0x6E490                               ; resolve frame
0x6E70B  word [rec+0x32] = 0                        ; accumulator reset
```

So "call `0x6E598(rec, 0)`" on a record whose current row byte 0 is non-zero
and not in `{0x62..0x65}` advances the RNG stream and **re-selects the current
id** (the RNG result is discarded); with no current row it also advances the
stream and selects row 0. `[0x57A6C]` is a state byte written here; its only
static readers are `0x71C99` (`FUN_00071C94`) and `0x7A4BF` (`FUN_0007A490`,
ball staging), and it is also written by `FUN_00073E28` (`0x73E7B`) and one
site at `0x7741A` (semantics open, leg 5).

## 2. `FUN_0006E490` (`0x6E490..0x6E517`, 49 insns) — the frame resolver

`EDX = &rec+0x28`, `EAX = rec`, `BX = requested frame index`:

```
0x6E494  if (BX < 0) BX = byte[row+1]                ; last frame
0x6E4A2  else if (BX > byte[row+1]) BX = 0           ; wrap to first
0x6E4B0  [rec+0x3D] = BL                             ; frame index
0x6E4B6  [rec+0x2C] = [row+4] + BX*5                 ; frame record pointer
0x6E4CE  if ([rec+0x44] == 0)                        ; row bit 0 clear
0x6E4F8      [rec+0x34] = word[frame+0]              ; frame duration
         else
0x6E4D0      base = ([rec+0x8D] != 0) ? 0x1052A : FUN_0006E330(rec)
0x6E4E7      h = [rec+0x6F] >> 16
0x6E4EF      [rec+0x34] = byte[base + 1 + 2*h]       ; height-class duration
0x6E4FF  [rec+0x3E] = (int8)[rec+0x8E]               ; heading byte
0x6E50A  [rec+0x36] = word[frame+2]                  ; aux word
```

`FUN_0006E330` (`0x6E330..0x6E443`, 80 insns) selects the height base by game
state: `0x1052A` when `[rec+0x8D] != 0`; otherwise `0x105AA`, `0x1058A` or
`0x105CA` depending on `[0x57A83]`/`[rec+0x9B]`, `[0x57A49]>>24`,
`[rec+0x8E]>>24`, `[0x587D4]`, `[0x58730]`, `[0x58746]`, `[0x577BE]` and
`[rec+0x69]`/`[rec+0x6F]` thresholds (`0x6E343..0x6E443`; selection semantics
open, leg 6).

The siblings are the same resolver for other rows:

| resolver | row pair | index | frame ptr | duration | aux | heading | height base |
|---|---|---|---|---|---|---|---|
| `FUN_0006E490` `0x6E490` | `&rec+0x28` | `+0x3D` | `+0x2C` | `+0x34` | `+0x36` | `+0x3E` ← `[rec+0x8E]` | `0x1052A`/`FUN_0006E330` |
| `FUN_0008DF00` `0x8DF00` | `&rec+0x28` | `+0x3D` | `+0x2C` | `+0x34` | `+0x36` | `+0x3E` ← `[rec+0x8E]` | `[rec+0x8D] ? 0x1052A : 0x105AA` |
| `FUN_0006E518` `0x6E518` | `&rec+0x7A` | `+0x8F` | `+0x7E` | `+0x86` | `+0x88` | `+0x90` ← `[rec+0x66]` | `0x1052A`, `h = [rec+0x5A]>>16` |

All three **selectors** (not the frame helpers) write `word [rec+0x32]`/
`word [rec+0x84] = 0` after the helper call (`0x6E70B`, `0x8E000`,
`0x6E7F4`); the
selectors fan flags to `+0x43/+0x44/+0x45/+0x46/+0x3F` (primary) or
`+0x95/+0x96/+0x97/+0x98` (secondary, `FUN_0006E724` at `0x6E7B9..0x6E7EF`).

## 3. Row-table population: `0x73D91` (static object-4 data, runtime pointer)

`FUN_00073D90` (`0x73D90..0x73DC1`) installs the table and re-arms both teams:

```
0x73D90  PUSH EDX
0x73D91  MOV dword [0x57588], 0xEF00        ; object-4 encoded row table -> flat 0x10EF00
0x73D9B  CALL 0x6FFC0
0x73DA0  EAX = 0x588A4; EDX = 0; CALL 0x8C2E0   ; team A (11 records, 0xB2 stride)
0x73DB1  EAX = 0x590D9; EDX = 1; CALL 0x8C2E0   ; team B
0x73DBB  CALL 0x4C380
0x73DC1  RET
```

Its call site is the stub at `0x4B020` (`E8 6B8D0200` → `CALL 0x73D90`, then
`E9 B68E0200` → `JMP 0x73EE0`). The match-init function `FUN_0004939E`
(`0x4939E..`; sets `[0x72F8]`, calls `0x4C904`, `0x43600`, `0xCB2A4`,
`0x4C698`, `0x1D940(0x18/0x19)`, `0x566A8`) runs the pair:

```
0x49406  CALL 0x73D10        ; clear global blocks (below)
0x4940B  CALL 0x4B020        ; -> CALL 0x73D90 (row table install)
```

`FUN_00073D10` (`0x73D10..0x73D76`) zeroes `0x588A4` (`0x106A`), `0x57A4C`
(`0x68`), `0x587EC` (`0x9F`), `0x57AB4` (`0x176`), `0x57710` (`0x2F`),
`0x5758C` (`0x183`) and `0x57740` (`0xEB`) with `REP STOSB`; note the block at
`0x5758C` starts **after** the row-pointer slot, so the pointer survives the
clear (it is re-written as the first step of the installer).

### 3.1 Row layout (9 bytes, 111 rows at flat `0x10EF00..0x10F2E6`)

Read from `0x10EF00`; `id` byte equals the row index for all 111 rows; row
`i` occupies `0x10EF00 + i*9`:

| +0 | +1 | +2 | +3 | +4..7 | +8 |
|---|---|---|---|---|---|
| id | last frame (count-1) | flags | next id | frame table (object-4 encoded u32) | sprite-bank index |

Examples (flat bytes):

```
row 0   00 00 07 00 00 E2 00 00 00      id 0,  1 frame, flags 7, next 0,  frames @0x10E200, bank 0
row 1   01 0B 07 01 06 E2 00 00 01      id 1, 12 frames, flags 7, next 1,  frames @0x10E206, bank 1
row 13  0D 05 00 0E C8 E3 00 00 0C      id 13, 6 frames, flags 0, next 14, frames @0x10E3C8, bank 12
row 14  0E 08 00 0E 04 E4 00 00 0D      id 14, 9 frames, flags 0, next 14, frames @0x10E404, bank 13
row 110 6E 01 07 00 66 EE 00 00 53      id 110, 2 frames, flags 7, next 0,  frames @0x10EE66, bank 83
```

Flags bit fan-out is exactly `FUN_0006E598` §1: bit0 → `+0x44` terminal, bit1
→ `+0x43`, bit2 → `+0x45`, bit4 → `+0x3F = 2`, bit5 → `+0x3F = -2`. Observed
flag values across the 111 rows: `{0, 2, 4, 6, 7, 16, 32}`. The frame pointer
is a **stored object-4 offset** (`0xE200` → flat `0x10E200`); the LE loader
applies the object-4 base to these row fields, so the in-image value is the
encoded form (same rule as `[0x57588]` itself).

Row `+3` is the **successor id** consumed by the driver (§5): `0x00..0x6E` =
next id, `0x6F` = repeat current row's id, `0x70..0xFF` = next id
`value-0x70` with a face reset. Row `+8` is the **sprite-bank/animator index**
used by `FUN_00078FAC` (`MOV CL, byte [EDI+8]`, `0x78FFD`; observed `0..89`);
`+1` is the last frame index — `0x4C272` returns `byte[row+1] + 1`
(`MOVSX EDX,AL; ADD EAX,EDX; MOV EDX,[0x57588]; MOV AL,[EDX+EAX+1]; INC EAX`)
as the frame count.

There is **no static writer** to `[0x57588]` other than `0x73D91`; the slot
is zero in the LE image's BSS (`0x157588` object-4 offset `0x57588` lies in
the zero-fill tail beyond the 67 stored pages), so the installer is the sole
population source.

## 4. The frame records (5 bytes, preceding the row table)

Row `frame_table + i*5` = `{duration:u16, aux:u16, sprite:u8}`; the frame
region `0x10E200..0x10EF00` holds 625 distinct records referenced 706 times
(rows share frame tables). Examples:

```
row 4 (flags 0), frames @0x10E2BA:
  40 00 0A 00 00    duration 0x40, aux 0x000A, sprite 0
  80 00 07 00 01    duration 0x80, aux 0x0007, sprite 1
  80 00 00 00 02    duration 0x80, aux 0,      sprite 2
  80 00 00 00 03    duration 0x80, aux 0,      sprite 3
  80 00 02 00 04    duration 0x80, aux 0x0002, sprite 4
  C0 00 00 00 05    duration 0xC0, aux 0,      sprite 5
row 1 (flags 7), frames @0x10E206:
  50 00 00 00 00 / 50 00 00 00 01 / ... (w0 0x50, aux 0, sprite = frame index)
```

* **duration** (`+0`) is in the same unit as the driver accumulator: the
  driver adds `[0x57A64] << 4` per call (`0x8E232 SHL EAX,4`), so a `0x40`
  duration is 4 accumulation steps of `delta=1`. The 18 distinct `word[+0]`
  values are `{0x06,0x10,0x20,0x40,0x50,0x60,0x80,0xC0,0x100,0x140,0x180,
  0x1C0,0x280,0x300,0x3C0,0x500,0xF00,0x1400}`. For rows whose flags bit 0
  is set (`FUN_0006E490` §2), `[rec+0x34]` is instead the height-table byte,
  and `word[frame+0]` is not used as duration.
* **aux** (`+2`) is copied to `[rec+0x36]` by the resolver; no static consumer
  was traced (open leg 2).
* **sprite** (`+4`) is read by `FUN_00078FAC` (`MOVZX EBP, byte [ECX+EAX+4]`,
  `0x78FF6`) and added to the frame-data offset in the composite cases; its
  full semantics are open (leg 3).

## 5. `FUN_0008E008` (`0x8E008..0x8E243`, 176 insns) — the per-frame driver

`EAX = rec`; `ESI = &rec+0x28`. Linear decode (`ndisasm`) of the full body:

```
0x8E015  if (row flags & 4) {
0x8E024     d = ([rec+0x7D]>>16 - [rec+0x7B]>>16) & 0x3FF
0x8E039     if (d > 0x200) d = 0x400 - d
0x8E049     [rec+0x46] = (d < 0x100) ? 1 : -1
         }
0x8E05A  AX = word[rec+0x32]; DX = word[rec+0x34]
0x8E065  if (AX < DX) goto 0x8E21F (update heading + accumulate)
0x8E06B  word[rec+0x32] = AX - DX
0x8E06F  AH = [rec+0x44] (row bit 0)
0x8E078  if (AH == 0) goto 0x8E139
         ; row bit 0 set: height/pose byte path
0x8E07E  base = ([rec+0x8D] != 0) ? 0x1052A : 0x105AA
0x8E093  if ([rec+0x46] >= 0) base += 2*([rec+0x6F]>>16)
0x8E0A5  else if ([rec+0x71] != 0) base += 2
0x8E0AF  if ([rec+0x99] != 0 && row.id != 0x5A && byte[base] in {1,2,3})
0x8E0F9      FUN_0008DF84(rec, 0x5A, [rec+0x46]+[rec+0x3D])
0x8E103  else if (byte[base] == row.id)
0x8E15D      FUN_0008DF00(rec, &rec+0x28, [rec+0x46]+[rec+0x3D])
0x8E12F  else FUN_0008DF84(rec, byte[base], [rec+0x46]+[rec+0x3D])
0x8E139  ; row bit 0 clear:
0x8E13B  if (([rec+0x3A]>>24) < row.last_frame)
0x8E15D      FUN_0008DF00(rec, &rec+0x28, [rec+0x46]+[rec+0x3D])
0x8E148  else:
0x8E167      next = row.next_id
0x8E173      if (next == 0x6F) FUN_0008DF84(rec, row.id, [rec+0x46]+[rec+0x3D])
0x8E1A4      else if (next <= 0x6E) FUN_0008DF84(rec, next, [rec+0x46]+[rec+0x3D])
0x8E1A6      else {  ; 0x70..0xFF
0x8E1AF          FUN_00079C50(rec, -(int8)[type+0xF334], -(int8)[type+0xF33C])
0x8E1F7          FUN_0008DF84(rec, next-0x70, [rec+0x46]+[rec+0x3D])
             }
0x8E21F  [rec+0x3E] = [rec+0x8E]
0x8E228  CX = word[rec+0x32] + ([0x57A64] << 4); word[rec+0x32] = CX
```

Consequences (all ported):

* The comparison uses the timer **from the previous call**; the accumulator
  increments at the end. On the advance path the re-selection runs
  `FUN_0008DF84`/`FUN_0008DF00`, whose selector tail zeroes `[rec+0x32]`
  (`0x8E000`), so the
  subtraction remainder is **discarded** and the new timer is `delta<<4`.
* The new frame index is `(int8)[rec+0x46] + (int8)[rec+0x3D]`; the resolver
  wraps negative to `last_frame` and `>last_frame` to 0 (§2). `[rec+0x46]`
  is +1 by default (selector) and ±1 when row flags bit 2 is set (heading vs
  facing).
* `[rec+0x3A]>>24` is a frame-program byte: when it is below the row's last
  frame, the row is re-selected unchanged (same id, next index); when it
  reaches the last frame, the successor (row `+3`) is taken. Its writer is
  not derived (leg 4).
* The height-gated branch (`[rec+0x99] != 0`, row id ≠ `0x5A`, bank byte
  1/2/3) switches to animation `0x5A` with the next index; `0x5A` is the
  FU-79/FU-83 "stride" animation family (leg 7).

Callers: `FUN_0008E748` (`0x8E7EB`) and `FUN_0008E810` (`0x8E8EA`) iterate the
11 records of a team pool based at `0x59914` (`+0xB2` stride; `0x835` per
team; `FUN_0008E748` uses `XOR EAX,[0x57AC2]; AND 1` to pick a team), set
`[rec+0x4D]/[rec+0x55]` from tables at `0x109F9`/`0x10A67`, and call
`FUN_0008E008` per record; an unnamed caller calls at `0x8E5AD`.

## 6. `[0x57A38]` family — `FUN_00073CD0` and its tables

`FUN_00073CD0` (`0x73CD0..0x73D04`) runs during match init (callers
`FUN_000115A0` `0x115A3`, `FUN_00011B7C` `0x11B8E`) and stores four
object-relative immediates:

```
[0x57A34] = 0x2CA4   -> flat 0x102CA4   (object-4 debug/string block; leg 11)
[0x57A38] = 0xF394   -> flat 0x10F394
[0x57A3C] = 0xF3A4   -> flat 0x10F3A4
[0x57A40] = 0xF3B4   -> flat 0x10F3B4
```

`0x10F394` holds three **overlapping byte tables** and a word ramp:
`0x10F394` 32 bytes `06×8, 07×8, 0F 0F 0F 0E 0D 0D 0C 0B 0B 0A 09 09 08 07
07 06`; `0x10F3A4` 16 bytes `0F 0F 0F 0E 0D 0D 0C 0B 0B 0A 09 09 08 07 07 06`;
`0x10F3B4` 16 bytes `10 13 16 1A 1D 20 23 26 2A 2D 30 33 36 3A 3D 40` followed
by the word ramp `00 00 15 00 2B 00 40 00 55 00 6A 00 7F 00 94 00 ...`
(`0x10F3C4`). Readers:

* `FUN_0007997C` `0x799F0..0x79A4D`: `word[rec+0x79] = (byte[[0x57A38] +
  (byte[[rec+4]+0xA]>>24)] | [rec+0x9D])`; also zeroes
  `+0x81/+0x87/+0x89/+0x91/+0x92` and `[rec+0x99]`, and copies `+0x6B→+0x77`,
  `+0x75→+0x73/+0x71` (the record reset/placement entry, FU-83 §1.2).
* Phase `0C` `0x6DF72..0x6DF8B`: `word[rec+0x7B] = (byte[[0x57A38] +
  (byte[[rec+4]+0xA]>>24)]) >> 1`.
* Code `24` `0x86537..0x8654E` (FU-82 §3.9): `[rec+0x7B] = byte[[0x57A38] +
  (byte[[rec+4]+0xA]>>24)] - 2`.
* `FUN_0007A084` `0x7A3D4..0x7A3F5` reads `[0x57A3C]`: `word[rec+0x81] =
  (byte[[0x57A3C] + (byte[[rec+4]+0x10]>>24)] | [rec+0x9D]) + 0xF`.
* `[0x57A40]` has **no static reader** (leg 11).

## 7. Presentation consumer: `FUN_00078FAC` (the sprite-frame resolver)

`FUN_00078FAC` (`0x78FAC..0x79588`, 450 insns). Inputs from the call sites at
`FUN_00057158` `0x571F8` and the draw path `0x5644C`/`0x56467` (function at
`0x563C7`): `AL = animation id`, `DL = frame index`, `EBX = direction 0..7`
(`AND EBX,7`), `ECX = out pointer`, `[ESP+0x34] = frame-data out`. Body head:

```
0x78FC0  [out5] = 0; [ECX] = 0
0x78FCD  if ((int8)id >= 0x6F) return 0
0x78FDF  EDI = [0x57588] + id*9
0x78FF3  ECX = [EDI+4]                                  ; frame table
0x78FF6  EBP = byte[ECX + frame_index*5 + 4]            ; frame sprite byte
0x78FFD  CL = byte[EDI+8]                               ; sprite-bank index
0x79005  ; compare tree over id (0x79009..0x790A5) routes
         ;   0x1B -> 0x7939E; 0x2F -> 0x793EC; 0x34..0x37 -> 0x790C7;
         ;   0x38..0x3B -> 0x791B4; 0x3D/0x3E -> 0x79343;
         ;   0x3F..0x42 -> 0x7929E; 0x4A/0x4F/0x51/0x54/0x56/0x67/0x6B
         ;   -> 0x793EC; everything else -> 0x79446
0x79109  ;   ESI = 0x57DE8 + bank*0x18; if [ESI+0x14] != 0 CALL 0x78DAC
0x7912E  ;   frame data = [ESI+4] * direction + EBP
```

The `0x57DE8` block is a 24-byte-stride **animator-record** array built by
`FUN_00078F1C` (`0x78F1C..0x78FAB`): it takes resolver-table entry `0x47`
(`EAX=0x47; CALL 0x4AFB8`, the `0x4BFC0` indexed table of FU-83), calls
`0xA2718` for the record count, resolves two handles per record
(`0xA275C`/`0x9E890`), zeroes `[rec+0]`, stores `[rec+0x10]`, `[rec+0xC]`,
`[rec+0x14]`, and calls `0x78C68` (state dispatch); the array is cleared by
`FUN_00078B20` (`0x57CF4..`/`0x57D6C..`, `CALL 0xCBE32(0x58674,0x5B)`). The
`0x57DE8` initializer call site is the stub at `0x4AB17` (`CALL 0x78F1C`).

Sprite draw call sites:

* `FUN_00057158` `0x571D6..0x571FD`: per-entity tables `0x54364/0x54388/
  0x543A8`; `EAX = [ent*4 + 0x55BC4] - 0x1000`; direction
  `EBX = (7 - (EAX >> 0xD)) & 7` (`0x571CC..0x571EC`),
  `DL = [ent+0x55C37]`, `AL = [ent+0x55C20]`, then `CALL 0x78FAC`; the result
  is packed through `0x5D1` scaling and blitted by `FUN_00057080`.
* The path at `0x563C7` loads a 3-byte descriptor `[EAX+0]`/`[EAX+1]`/
  `[EAX+2]` from the object-4 table at flat `0x108E6C` (`ADD EAX,0x8E6C`;
  entries `63 00 05`, `65 0A 04`, ... selected through a double indirection
  over the flat globals `0x108F44`/`0x108FA8`), calls `0x78FAC` at `0x5644C`,
  falls back to a second call at `0x56467` (`EAX=0, EBX=5, EDX=0`), and blits
  through `0x57080`; `[0x108FB8]`/`[0x108FBC]` gate the frame stepping.

So the presented pose per frame is: `FUN_0008E008` advances
`[rec+0x3D]`/`[rec+0x2C]` (and `[rec+0x34]/[rec+0x36]/[rec+0x3E]`), and the
renderer resolves `(id, frame_index, direction, row+8 bank, frame+4 sprite)`
through `FUN_00078FAC` into sprite-frame data; the frame record's `+4` byte
and the row's `+8` bank index are the bridge between the two.

## 8. Port: `fifa96_animation_*`

`include/fifa96_loader/fifa96_animation.h` +
`src/fifa96_loader/fifa96_animation.c` (caller-owned state, no globals,
negative `fifa96_err_t` for invalid arguments, no comments).

| original | port |
|---|---|
| id clamp `(int16)id < 0 \|\| >= 0x6F → 0`, row `= [0x57588]+id*9` (`0x6E68E..0x6E6B1`, `0x8DF96..0x8DFA9`) | `fifa96_animation_row_lookup(rows, row_count, anim_id, &row)` — 16-bit truncation, clamp, 9-byte decode; `row_count` guard is a hardening divergence (original indexes a fixed 0x6F table with no check) |
| flag fan-out `AL & 4/2/1`, `0x10 → +2`, `0x20 → -2` (`0x6E6B4..0x6E6FB`) | `fifa96_animation_flags(row_flags, &flags)` |
| frame clamp `BX<0 → last`, `BX>last → 0` + 5-byte record (`0x6E494..0x6E517`) | `fifa96_animation_frame(frames, index, last_frame, &frame)` |
| accumulator + duration gate + `±turn` index + wrap (`0x8E05A..0x8E243`) | `fifa96_animation_advance(&state, duration, last_frame, &out)` — compares the previous timer, resets it on advance (selector zeroing), adds `delta<<4` with 16-bit wrap |
| turn sign fold (`0x8E024..0x8E056`) | `fifa96_animation_turn(heading, facing, &turn)` |
| row `+3` successor (`0x8E167..0x8E1F7`) | `fifa96_animation_successor(next_id, current_id, &anim_id, &face)` — `0x6F` repeat, `<=0x6E` direct, `0x70..0xFF` id `-0x70` with `face=1` |
| `[0x57588]`/row storage, `0x6E490`/`0x8DF00`/`0x6E518` pointer arithmetic, height tables `0x105xx`, `FUN_0008E008`'s row install, `FUN_00078FAC`/`0x57DE8`, draw paths, `[0x57A38]` family | not ported (globals/pointers/renderer; cited in §1–§7, open legs) |

## 9. Tests (`tests/test_animation.c`, suite 72 → 73)

* Layout `_Static_assert`s on `fifa96_anim_row`/`fifa96_anim_advance`.
* `row_lookup`: full 111-row synthetic table, rows `0/0x0E/0x6E`, clamp at
  `0x6F`, `-1`, `0xFFFF`, `0x7FFF`, 16-bit truncation `0x1000F → 0x0F`,
  `row_count 0x6E` with id `0x6E` → error, two-row real-bytes fixture
  (`01 0B 07 01 06 E2 00 00 01` → `0xE206`, bank 1), NULL/zero-count/out.
* `flags`: `0,1,2,4,7,0x10,0x20,0x30,0xFF`, NULL.
* `frame`: in-range, negative → last, beyond → 0, `0x7FFFFFFF` → 0, aux
  `0x0302`, NULLs.
* `advance`: four accumulate calls then the advancing fifth (`0x40`/delta 1),
  forward wrap last→0, backward wrap 0→last, 16-bit accumulator wrap
  (`0xFFF0 + 0x100 → 0x00F0`, duration `0xFFFF`), duration 0 advances every
  call, delta 0, NULLs.
* `turn`: equality, ±0x100/±0x101 fold edges, `0x201/0x2FF/0x300/0x301`
  folds, negative wrap, NULL.
* `successor`: repeat `0x6F`, `0..0x6E`, `0x70/0x7E/0x7F` (−0x70, face 1),
  `0x80..0xFF`, NULLs.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_animation.c src/fifa96_loader/fifa96_animation.c` runs clean.
`make test`: 72/72 before, **73/73 after**.

## 10. Errata (quoted)

* FU-82 §2.1 "Inputs `EAX = rec`, `EDX = animation id`, `ECX = type8`,
  `EBX = param`" — **corrected**: `ECX` is overwritten at `0x6E5B3` by the
  ball-z/side comparison; the body never reads the incoming `ECX`. `EBX` is
  the requested **frame index** (`MOVSX EBX,BX` at `0x6E701`, passed to
  `FUN_0006E490`), not a generic param. FU-82's step labels for the body are
  otherwise confirmed (row store, flag copies, `+0x3F`, `CALL 0x6E490`,
  `word [rec+0x32] = 0`).
* FU-82 §2.1 / open leg 4 "**[0x57588] is a runtime pointer slot (zero
  statically); the 0x6F rows ... are not enumerated**" — **closed**:
  `0x73D91` stores the object-4-encoded `0xEF00` (flat `0x10EF00`); all 111
  rows are enumerated (§3.1).
* FU-82 open leg 5 "[0x57A38] ... runtime target is unproven" — **closed**:
  `FUN_00073CD0` sets it (and `[0x57A3C]/[0x57A40]`) to the flat
  `0x10F394/0x10F3A4/0x10F3B4` tables (§6).
* FU-82 open leg 10 "`FUN_0006E490` (animation frame helper) and the `0x57588`
  row layout are cited by address only" — **closed**: §2 and §4.
* FU-83 §3.2 `0x6E598` "ported in FU-82 as `fifa96_action_sequence_*`" —
  **corrected**: FU-82 ported helpers that *call sites of* `0x6E598` use
  (`sequence_event`, `anim_byte`); the selector itself was not ported. It is
  ported here as `fifa96_animation_row_lookup` + helpers.
* FU-83 §3.2 height tables `0x105xx` — **extended**: the four bases
  `0x1052A/0x1058A/0x105AA/0x105CA` and their selector `FUN_0006E330` are
  cited in §2; selection semantics remain open (leg 6).
* **Row+8 bank chain live inputs (M2 playability-legs Task 5 / OL-80).** The
  engine record pool now carries the two staging fields `FUN_00036C70` reads
  at flat `0x36D44`/`0x36D4F` — first-hand `/FIFA96.EXE` this slice:
  `0x36D44 MOV ECX,[EDX+0x28]; MOV CL,[ECX]; MOV [EAX+0x155C20],CL` (the
  record's current row id = row byte 0 of the pointer the selector
  `FUN_0006E598` stores at `[rec+0x28]`, `0x6E6B1`) and
  `0x36D4F MOV CL,[EDX+0x3D]; MOV [EAX+0x155C37],CL` (the frame resolver
  `FUN_0006E490`'s `+0x3D` index). The engine stages both into the FU-84
  row+8 bank selection and the frame advance, and the arm bodies'
  `fifa96_arm_anim_select` now receives the live id (FU-141 §8 errata).
  The native per-frame advance stays `FUN_0008E008` (§5); the engine's
  derived advance writes its result back into the pool `frame`. The row
  successor/terminal machine and the `+0x8` animator-record semantics
  (leg 1) are unchanged.
* **Selector producer on the kickoff path (M2 playability-legs Task 5 fix
  round 1).** The native setup/restart commit `FUN_00079B6C` ends at `0x79C1C`
  (first-hand `get_function_by_address 0x79BB5` -> `body_end 0x79C1C`) and its
  tail calls `FUN_0006E598` unconditionally at `0x79C13` with
  `EDX = (byte[rec+0x8D] ? 0 : 0x26)` and `EBX = 0`: inactive records select
  row id 0x26, active records re-resolve id 0 (the keep/reroll arm, §1), and
  the frame resolver writes `[rec+0x3D] = 0` and resets the `+0x32`
  accumulator. The engine ports this in
  `fifa96_match_entities_kickoff_place` (`fifa96_arm_anim_select` +
  `frame = 0`), so pool ids/frames are non-zero from match begin; the
  `0x79C13` frame-resolver accumulator reset has no pool field (render-slot
  `anim_timer` is zero at begin).

## 11. Open legs

1. **Row `+8` sprite-bank semantics**: used as index `*0x18` into the
   `0x57DE8` animator records in `FUN_00078FAC`'s composite cases; the record
   count (resource entry `0x47` length) and field meanings (`+0` flag, `+4`
   stride, `+0xC`, `+0x10` handle, `+0x14`) are not derived.
2. **Frame aux word** (`+2` → `[rec+0x36]`): no static consumer traced.
3. **Frame sprite byte** (`+4`): consumed as a base offset by `FUN_00078FAC`;
   the sprite-data layout behind it is not derived.
4. **`[rec+0x3A]>>24` frame-program writer**: compared against row
   `last_frame` by `FUN_0008E008` (`0x8E13B`); setter unknown.
5. **`[0x57A6C]`**: written by the selector (`0x6E5F7/0x6E602`) and by
   `FUN_00073E28`/`0x7741A`; read at `0x71C99`/`0x7A4BF`; meaning not derived.
6. **`FUN_0006E330` selection**: the state machine choosing among
   `0x1052A/0x1058A/0x105AA/0x105CA` is decoded branch-by-branch but the
   game-state meanings (`[0x57A83]`, `[rec+0x9B]`, `[0x587D4]`, `[0x58730]`,
   `[0x58746]`, `[0x577BE]`) are cited only.
7. **`0x5A` stride animation**: `FUN_0008E008`'s height-gated branch and the
   `[rec+0x99]` state are quoted; the stride semantics are FU-77/FU-83 open
   legs.
8. **`FUN_00078FAC` composite resolution**: the compare-tree-routed ids
   (`0x1B`, `0x2F`, `0x34..0x37`, `0x38..0x3B`, `0x3D/0x3E`, `0x3F..0x42`,
   `0x4A`, `0x4F`, `0x51`, `0x54`, `0x56`, `0x67`, `0x6B`) and their six
   handler blocks (`0x790C7/0x791B4/0x7929E/0x79343/0x7939E/0x793EC`), the
   `0x57DE8` records, and the `0xF2E7`/`0x14E04` table usage are cited by
   address only.
9. **`FUN_0006E724` RNG tail**: the listing mis-decodes `0x6E75B..0x6E78A`
   (the `0x92AC8` branch); not quoted.
10. **Tooling**: the `/tmp/opencode/fifa96_le.bin` at the MCP
    `executable_path` currently differs from the Ghidra session's imported
    image (object-1 bytes and object-4 stored pages); all data quotes were
    cross-verified against `tools/fifa96_le.py`'s parse of the retail EXE,
    which matches the Ghidra memory at every cited address.
11. **`[0x57A34]`/`[0x57A40]`**: `0x102CA4` is an object-4 debug/string block
    (`"GDMFC"`, `"afile%d"`, `"TEMPBUF"`); `[0x57A34]` has no static reader
    and `[0x57A40]` none either.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x6E598, 0x6E490, 0x6E330, 0x6E518, 0x6E724, 0x8DF84,
0x8DF00, 0x8DF84 callers, 0x8E008, 0x8E748, 0x8E810, 0x78FAC, 0x78F1C,
0x78C68, 0x78B20, 0x73CD0, 0x115A0, 0x11B7C, 0x57158, 0x68FD0, 0x69338,
0x6B4BC, 0x6D142, 0x72270, 0x72AC4, 0x7997C (0x799F0 window), 0x6DF70
window, 0x7A3D0 window, 0x563C7 window, 0x4AAF0 window;
`disassemble_bytes` 0x4C260, 0x73D05, 0x4B000, 0x56440; `read_memory`
0x10EF00, 0x10F2E0, 0x10F394, 0x12CA4, 0x102C00, 0x110520, 0x157580,
0x102CA0; `search_instructions` operands `57588`, `5758c`, `57a34`, `57a38`,
`57a3c`, `57a40`, `57a6c`, `57de8`, `1058a`, `[0x000575`, `0x3d]`, `0x8f]`;
`search_byte_patterns` `88750500`, `8C750500`; `get_function_xrefs` 0x6E598,
0x6E490, 0x6E724, 0x73CD0, 0x78FAC, 0x78F1C, 0x8E008, 0x8DF84; `get_xrefs_to`
0x57588, 0x57A38, 0x73D90. Linear decode: `ndisasm -b32` over dumped ranges
(`/tmp/opencode/fu84/*.bin`). Data cross-check: `tools/fifa96_le.py` parse of
`/tmp/opencode/fu49/iso/FIFA96.EXE` (retail) for the row/frame/table bytes.
Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_animation.h`,
`src/fifa96_loader/fifa96_animation.c`, `tests/test_animation.c`,
`CMakeLists.txt` (one library/test block). `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
