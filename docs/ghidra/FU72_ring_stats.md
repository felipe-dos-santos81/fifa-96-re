# FU-72: the entity history ring and the match/player stat accumulators

Roadmap sub-slices **S10** (entity history ring) and **S11** (stat accumulator)
of FU-67 §5. This slice derives the 25×0x15 record ring at `0x5B440`
(`FUN_000927E0` reset / `FUN_00092820` stage / `FUN_00092864` commit /
`FUN_000928F0` direct insert / `FUN_00092998` scan), the per-entity accumulator
table at `0x5A998` (`FUN_0008ED40`, `FUN_0008EE04`, `FUN_0008ECC4` and the five
accessors), and the per-side goal words `0x57AC5`/`0x57AC7`
(`FUN_00093944`), and ports the clean ring and stat-block mechanics as
`fifa96_ring_stats`.

Result in one line: **the ring is a newest-first history of 25 packed records
(type byte +0, timestamp +1, entity pointer +5, camera triple +9..0x14) with
cursor `[0x5B665]` and a 0x15-byte staging record at `0x5B650`; records are
staged by `FUN_00092820` (24 sites), committed on camera-velocity change by
`FUN_00092864`, or inserted directly by `FUN_000928F0`, while type 0x26 is a
suppression sentinel and `FUN_00092998` returns the ordinal-th newest record
matching a type-flag mask / side filter; the stat table is 2 blocks × 16 slots
of stride 0xA (key +0, word counter +4, dword value +6) at `0x5A998`, keyed by
the runtime pointer tables `0x4C360`/`0x4C3DB`, accumulated by
`FUN_0008ED40` with a 2×/1×/1.25× event weight, and the match score is the
word pair `0x57AC5`/`0x57AC7` incremented by `FUN_00093944(side)`.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-71). Every
  instruction quoted was read back this slice with
  `disassemble_function`/`disassemble_bytes`/`read_memory`/`search_instructions`.
* **Disassembler hole (new errata).** `disassemble_function 0x92998` omits the
  four bytes `0x929F0..0x929F3` (the `JNZ` of the type-0x1D test) and resumes
  mid-instruction at `0x929F4`; the block was recovered with
  `read_memory 0x929E8` (48 B) and manual decode. `decompile_function 0x92998`
  starts the ring index at `-0x77` (FU-67 errata); the disassembly starts at
  `[0x5B665]`.
* **Unanalyzed gaps.** The stat accessor cluster `0x8EB80..0x8EEA2`, the
  `0x8ADxx` announce bridge and the `0x700xx`/`0x8AExx` resets are not defined
  functions in the project; entries were taken from prologues/RET boundaries
  and decoded with `disassemble_bytes` (`0x8EB7E`, `0x8EE04`, `0x8AD00`,
  `0x70000`, `0x8AE60`). They are cited by address, not named.
* **Address convention (FU-59/FU-62/FU-67, quoted):** code immediates naming
  tables carry LE fixups and the loader adds the owning object's base. This
  slice reads two new tables as bytes at their **flat** addresses:
  `0x4C360`→`0x14C360` and `0x4C3DB`→`0x14C3DB` (both all-zero in the static
  image — populated at runtime), and the type-flag table at flat `0x110F1C`
  (FU-67: encoded `0x10F19`).
* All numeric claims (offsets, strides, thresholds) are quoted from the
  listings; semantic labels beyond what the instructions do are not asserted.

## 1. The 25×0x15 entity history ring `0x5B440`

### 1.1 Record layout

`FUN_000928F0` writes the four fields at the four explicit offsets
(`0x92938/0x92976/0x9297D/0x92980`), and `FUN_00092864` copies the whole 0x15
bytes from the staging record (`0x928DC..0x928E1`, 5×`MOVSD` + `MOVSB`);
`FUN_00092998` reads them back:

| offset | width | field | evidence |
|---|---|---|---|
| `+0` | byte | type | written `0x92938`; read as unaligned dword `[idx*0x15+0x5B43D]>>24` (`0x929C2`) |
| `+1` | dword | timestamp | `[idx*0x15+0x5B441]=now` (`0x92976`); reader never reads it |
| `+5` | dword | entity pointer | `[idx*0x15+0x5B445]=EBP` (`0x92980`); read `0x92A41/0x929F8` |
| `+9` | 3 dwords | camera triple | copied from `0x57758..0x57763` (`0x9297D..0x9297F`); no reader located (open) |
| `0x15` | — | stride | `IMUL EDX,EDX,0x15` (`0x929BF`), ring count 25 |

The staging record is the same shape at `0x5B650`: type `+0`, timestamp
`+1` (`[0x5B651]`), entity `+5` (`[0x5B655]`), triple `+9` (`[0x5B659..]`) —
`FUN_00092820` writes exactly those offsets (`0x92835/0x9284E/0x92853..56`).

### 1.2 Cursor and reset

* Cursor `[0x5B665]` (also read as dword `[0x5B662]>>24`). Writers:
  reset `0x927FE`, stage/commit `0x928B8`, direct insert `0x92920`; reader
  `0x929A8`. Next slot = `(cursor+1) % 0x19` (`INC`/`IDIV 0x19`, `0x928AB..B8`
  and `0x92913..20`); the slot is then reloaded from `[0x5B662]>>24` before the
  record write (`0x928BE`, `0x92926`), so the cursor always points at the
  **last written** record and the scan walks **newest-first**.
* `FUN_000927E0` (reset, sole caller `0x70030` in an unnamed `0x700xx`
  init): `for i = 0..0x18: byte[0x5B440 + i*0x15] = 0` (`0x927E6..0x927EC`),
  then `[0x5B665]=0`, `[0x5B650]=0`, dword `[0x5B655]=0`, `[0x5771A]=0x64`
  (`0x927FA..0x92815`).
* Time source: `FUN_000CB2A4` is `MOV EAX,[0x12E88]; RET` (`0xCB2A4..A9`) —
  the frame/tick dword.

### 1.3 Stage, commit, direct insert

* **Stage `FUN_00092820(entity EAX, type DL)`** (24 call sites): reads
  `flag = byte[flat 0x110F1C + type]` (`MOVSX EAX,DL / MOV AL,[EAX+0x10F19] /
  AND AL,1`, `0x92827..0x92832`), stores `[0x5B650]=type`, and only when
  `flag & 1` writes `[0x5B651]=now`, the `0x57758` triple, and
  `[0x5B655]=entity` (`0x9283F..0x92856`). Later calls overwrite the staging
  record unconditionally. Spot-checked type arguments: `0`/`1` (`FUN_0007AE70`
  `0x7B183`/`0x7B03E`→`0x7B187`), `3` (`0x85C51`), `6` (`0x752A9`), `0xF`
  (`0x7F67A`), `0x1C` (`0x70DBF`), `0x24` (`FUN_000700F4 0x7020F..16` when
  nothing is staged), `0x26` (below).
* **Commit `FUN_00092864`** (sole caller `0x73723` in the camera chain
  `FUN_000736AC`): if the camera velocity words `0x577BE/0x577C0/0x577C2`
  equal their shadows `0x57825/0x57827/0x57829` (copied by `FUN_000736AC
  0x73728..0x73750`, FU-67 §4.1) it returns; otherwise, if the staged type is
  `0x26` it clears the staging byte and returns (`0x92895..0x928E2`); else
  `slot=(cursor+1)%0x19`, cursor=slot, copies 0x15 bytes `0x5B650`→`0x5B440+
  slot*0x15`, clears `[0x5B650]` (`0x928A2..0x928E4`).
* **Direct insert `FUN_000928F0(entity EBP, type BL)`** (5 call sites:
  `0x7144A` types 0x17/0x18 with the ball or the current entity, and
  `0x7B051/62/73/84` types 2/7/8/3): skips (but still clears staging) when the
  staged type is `0x26` (`0x928F9..0x92904`), else writes the next slot in
  place (type always; timestamp/entity/triple only when the type flag bit0 is
  set, `0x92942..0x9294D`) and clears `[0x5B650]` (`0x92987..0x92989`).
* **The `0x26` sentinel:** three sites stage `0x26` with a NULL entity when
  nothing else is staged, to suppress the frame's push: `FUN_00070544
  0x7066C..0x7067C`, `FUN_00071DF4 0x71E39..0x71E49`, `FUN_000736AC
  0x73AB4..0x73AC4` (each `CMP byte [0x5B650],0 / JNZ / MOV EDX,0x26 /
  XOR EAX,EAX / CALL 0x92820`).

### 1.4 Reader `FUN_00092998`

Signature by register: `AL` = ordinal, `DL` = type-flag mask, `BL` = side
selector (0/1 = exact side, other = any), returns a pointer to the ring record
or 0. The disassembled body (`0x92998..0x92A94`, with `0x929F0..F3` recovered
from `read_memory`):

```
idx = (int8)[0x5B665]
for n = 0..0x18:                                  ; 25 iterations
  type = (int8)([0x5B43D + idx*0x15] >> 24)       ; record +0
  flags = (int8)([0x110F19 + type] >> 24)         ; flat 0x110F1C + type
  if ((flags & (int8)mask) != 0):
    if mask == 4 and (type == 0x1C or type == 0x1D):
      ptr = [0x5B445 + idx*0x15]
      if ptr != 0:
        y = [ptr + 0x61]; s = [ptr[0] + 0x826]
        if ((y > 0) == s) skip                 ; 0x929F2..0x92A2D
    if BL == 0 or BL == 1:
      ptr = [0x5B445 + idx*0x15]
      if ptr == 0 or ptr[0][0x826] != BL: skip ; 0x92A2F..0x92A59
    if (--ordinal == 0) return &[0x5B440 + idx*0x15]
  idx = idx - 1; if (idx < 0) idx = 0x18          ; wrap at 0x18
return 0
```

The type-flag table at flat `0x110F1C` (FU-67 §3.4; read here for 40 bytes):
type 0=0, types 1..0x18=7, 0x19..0x1B=3, 0x1C/0x1D=7, 0x1E..0x23=1,
0x24=9, 0x25=7, 0x26=0. Masks observed at the seven call sites:

| site | function | ordinal AL | mask DL | side BL | result use |
|---|---|---|---|---|---|
| `0x88966` | `FUN_00088940` | 1 | 4 | `0xFF` | `[result+5]` compared to `side` and stored into `[0x57A9F]`/`[0x57A9B]` (`0x889C7..0x88A1C`) |
| `0x7249A` | `FUN_00072478` | 1 | 2 | `0xFF` | mask-2 candidate entity |
| `0x724AB` | `FUN_00072478` | 2 | 2 | `0xFF` | second mask-2 candidate (kept in ESI) |
| `0x724C1` | `FUN_00072478` | 1 | 4 | `0xFF` | third candidate |
| `0x72BD5` | `FUN_00072AC4` | 1 | 2 | side byte `[[0x575D2]+0x826]` | stored to `[0x57599]` |
| `0x71F67` | `FUN_00071DF4` | 1 | 2 | `0xFF` | record type byte compared to 2 (`0x71F74`) |
| `0x7145E` | `FUN_0007131C` | 1 | 4 | `0xFF` | stored to `[0x5B43C]` |

`[0x5B43C]` (the dword just before the ring) is therefore a *last ring-search
result* pointer: written only at `0x71463`, read only at `0x726E9` in
`FUN_00072478`, which tests `record[0] == 0xB` and then calls
`FUN_0008ED40([record+5], 0x19)` (`0x726F6..0x7270F`) — the one evidenced
cross-link between S10 and S11 (a type-0xB ring record's entity receives +0x19
accumulator points). FU-67 §2 called `[0x5B43C]` a "staging record"; refined.

## 2. Stat accumulators

### 2.1 The per-entity table `0x5A998`

Initialized by `FUN_00091BC4` (sole caller `0x4A28B` in the match-init
`FUN_0004A228`), loop `0x91C24..0x91C63`: 16 iterations of stride `0xA` per
block, block 1 at `+0xA0` (`16*0xA`), key from two runtime pointer tables:

| offset | width | field | init (`FUN_00091BC4`) | used by |
|---|---|---|---|---|
| `+0` | dword | key | `[[0x4C360 + i*4]]` (block 0) / `[[0x4C3DB + i*4]]` (block 1) | `FUN_0008ED40` `0x8EDB8`, accessors |
| `+4` | word | counter | 0 (`0x91C32/0x91C50`) | `FUN_0008ECC4`, accessors `0x8EB80/0x8EBC8` |
| `+6` | dword | value | 0 (`0x91C3F/0x91C5A`) | `FUN_0008ED40`, `FUN_0008EE04`, accessors `0x8EC20/0x8EC68` |
| `0xA` | — | slot stride | `IMUL EDX,EDX,0xA` | |
| `0xA0` | — | block stride | `IMUL EBX,EBX,0xA0` | |

The flat key tables `0x14C360`/`0x14C3DB` are all-zero in the static image, so
the slot keys are runtime identity values (the `[[entity+4]]` dword compared at
`0x8EDBE`); the slot-to-player mapping cannot be read off the image.

### 2.2 `FUN_0008ED40` — weighted event accumulation

`fun(entity, value)` (29 direct call sites) decodes as:

```
team = [entity[0] + 0x7AE]
AL = (int8)[entity + 0x8D]; AH = (int8)[team + 0x9]
if (AL > AH):
  if (([entity+0x8A]>>24) <= ([team+6]>>24) + ([team+0xD]>>24))
      value = value + (value >> 2)                 ; 1.25x (SAR)
  ; else value unchanged
else:
  value = value * 2
block = (int8)FUN_000741B4((int8)[entity[0][0x826]])  ; (side ^ [0x57ABE]) & 1
for n = 0..0xF:
  off = n*0xA + block*0xA0
  if ([0x5A998+off] == [[entity+4]]):
    [0x5A99E+off] += value
    if ((int16)[0x5A99E+off] > [0x5AFB0]):
      [0x5AFB0] = (int32)(int16)[0x5A99E+off]
      [0x5B434] = (word)block
      [0x5AFB4] = [[entity+4]]                     ; 0x8EDC2..0x8EDF3
    return
```

`FUN_000741B4` is `(EAX ^ [0x57ABE]) & 1` (`0x741B4..C3`); `[0x57ABE]` is
written by `FUN_0007417C 0x74189` at match reset and read by the direction
helpers (`0x36D02`, `0x370BF`, `0x74130`, `0x788B0`, `0x8BA75`) — a
match-level side byte. The 29 call sites are `0x70F44/0x70FCE` (`FUN_00070DE0`),
six in `FUN_0007131C` (`0x714BE/0x7157C/0x715DD/0x716C7/0x71732/0x717E3`), two
in `FUN_00072AC4` (`0x72DE3/0x72ED1`), four in `FUN_00072478`
(`0x72649/0x72660/0x7270F/0x72A52`), `0x77451/0x77493/0x7749F/0x774B2/0x77613`,
`0x77DFD/0x77E38/0x77E4E/0x77E61`, `0x80D6C/0x80D99/0x813CD/0x813F5/0x81BBE`,
and `0x8AD54`. Values re-read this slice (negative = decay): `+5`, `-0x64`,
`-5`, `+0x32` (50), `-0x4B` (-75), `+0x19` (25), `+0xF` (15), `+1`, `+2`,
`+0x64` (100), `-0x96` (-150); the increments are gated by camera/tracker/ball
proximity conditions re-read in §1's call sites and FU-67 §2.

### 2.3 The accessors and the counter path

* `0x8EB80(key EAX, block EDX)` → word `+4` for the matching key, 0 when
  `(int16)EDX > 1` or `< 0`; `0x8EBC8(entity EAX)` → same via
  `block = FUN_000741B4([entity[0][0x826])`, 0 when the entity is NULL.
* `0x8EC20(key, block)` → dword `+6`; `0x8EC68(entity)` → same via the side.
* `FUN_0008EE04(key EAX, side EDX, delta EBX, out-key ECX)`: adds `delta` to
  the `(key, block==side)` slot, then scans all 32 slots and returns the block
  of the maximum value while writing that slot's key to `*out`
  (`0x8EE16..0x8EE99`; initial max = block0 slot0 value/key). No caller was
  located in analyzed code (the region is an unanalyzed gap) — functional
  evidence only.
* `FUN_0008ECC4(entity EAX)`: side via `FUN_000741B4`, then for the matching
  key increments the `+4` word and sets `[0x5B358] = (count == 3)`
  (`0x8ED02..0x8ED27`). Sole caller `0x8AD77` in the unnamed `0x8ADxx`
  selection-announce bridge (FU-71 §8 row `0x8AD69` copies `[0x57A9F][+4]` into
  `[0x57B16]` first). So `[0x5B358]` is a "third selection" flag, not a
  gameplay stat (its sole other reference is the read at `0x8FF30`).

### 2.4 Per-side goal words `0x57AC5`/`0x57AC7`

* **Increment:** `FUN_00093944(side EAX)` does `INC word [EAX*2 + 0x57AC5]`
  (`0x9394B`) with `[0x5B670]=side`; it then maintains the goal-difference
  bookkeeping `[0x5B6A4]`/`[0x5B6B4]` (`0x93953..0x93992`) and posts the
  threshold event ids `0x9A`/`0x9B`/`0x9C`/`0x9D`/`0x9E`/`0x9F`/`0xA0` through
  `FUN_0009252C` when `score[side]` hits 3/4/5/7/9 and the other side is below
  0/0/3/3/4/5/2, plus a `CALL 0xCBC4C` on `score[side]==1 && other<3`. Nine
  call sites (`0x93D98/0x93DA1/0x94026/0x9402F/0x94489/0x94492/0x94667/
  0x94670/0x9486E`).
* **Reset writers:** `FUN_00092E2C 0x92E7B/0x92E82` (match reset: also zeroes
  `0x57AB4/AB6/AC9/AAF/AAE/AB1`, calls `FUN_00074034`, `FUN_0007417C`,
  `FUN_000700F4`), `FUN_00073EE0 0x73F15/0x73F64` (called from `FUN_00074034
  0x7407F`; also zeroes the `0x57ACE/AD0/AD2/ADA/ADC/ADE/AEA` words — roles not
  derived), and unnamed `0x8AExx 0x8AEA0/0x8AEA7`.
* **Consumers:** `FUN_0004B5DC(out0, out1)` returns the pair for the HUD
  (`0x4B5DF/0x4B5EA`); `FUN_0004B840 0x4B845/0x4B84B` and `FUN_0004B860
  0x4B866/0x4B86D`; `FUN_0004BBFC 0x4BC04/0x4BC0A`; the clock machine
  `FUN_0008AF38` (seven comparisons `0x8B3EC..0x8B5C6`, FU-62 period tail);
  `FUN_0008BAF0 0x8BD95/0x8BD9B`, `FUN_0008B9CC 0x8B9F2/0x8B9F8`; the tracker
  gate `FUN_00072478 0x727E4/0x727EA` (`[0x57AC5] != [0x57AC7]`, FU-67 §2);
  `FUN_0008C418` (difference), `FUN_0008F421/0x8F539` (zero tests); and the
  event pump `FUN_00092548 0x925E3/0x925EA` feeding
  `FUN_00066840(id, [0x57AC7], [0x57AC5])` (FU-63 §state-1 `0xC6`/`0x8D` arm).
  `FUN_00091528` reads both scores and the `team+0x828` flags
  (`0x590CC`/`0x59901`) to post id `0xA7`/`0xA8` (`0x9159F..0x91617`); the
  `team+0x828` role is not derived (open leg).

### 2.5 The `FUN_00091BC4` reset inventory (S11 home)

Besides the stat slots (§2.1) this match-init function (sole caller `0x4A28B`)
also resets, in order: the 10×0x20 command ring at `0x5AAE0` (per-slot
`now`/0/`[0x57A83]` and id `0x57` on slot 0, `[0x5AADC]=0`, `[0x5AAD8]=1`;
FU-60/FU-63); `[0x5AFB0]=0`, `[0x5AFB4]=` block0 slot0 key, `[0x5B434]=0`;
`memset(0x5AFAC, 0, 4)` via `FUN_0009E8D0` (input one-shot flags, FU-63); the
presentation/event words `0x5B330`, `0x5B32C`, `0x5B334`, `0x5B33C`, `0x5B36C`,
`0x5B370`, `0x5B374`, `0x5B37C`, `0x5B358`, `0x5B340`, `0x5B37A`, `0x5B42C`,
`0x5B354` (FU-63/FU-69); the timers `0x5B364=now+0xB4`, `0x5B350=now`,
`0x5B35C=now+0x708`, `0x5B368=0`, `0x5B428=0`; the per-event-id counter array
`0x5AFB8+4i` (0) and cooldown array `0x5AC38+4i` (now) for `ESI=4..0x374`
step 4 — `0xDD` writes (`0x91D3A..0x91D56`; FU-63); and the ambience words `0x5A980=-1`, `0x5A984=-1`, `0x5A990=1`,
`0x5A994=1`, `0x5B430=0`, `0x5B432=1` (FU-49/FU-60).

### 2.6 Not located

No per-player "shots", "fouls" or "possession" counters were found. The only
per-entity accumulators are the `0x5A998` table (event points/word counter),
and the only per-side counters are the goal words; the nearest event statistics
are the FU-63 per-id dispatch counters `0x5AFB8` and cooldowns `0x5AC38`
(initialized here, incremented by the event pump), which count event ids, not
gameplay actions. The `[0x5AFB0]/[0x5AFB4]/[0x5B434]` max triple has no reader
besides the reset (`search_instructions 5afb0/5afb4/5b434`).

## 3. Structural map (FU-67 §5 sub-slices)

| # | slice | anchor | status after this slice |
|---|---|---|---|
| S10 | entity history ring | `0x927E0/0x92820/0x92864/0x928F0/0x92998`, `0x5B440`, staging `0x5B650` | **layout + write/scan paths derived**; role semantics and `+9` consumers open |
| S11 | stat accumulator | `0x8ED40`, `0x8EE04`, `0x8ECC4`, accessors `0x8EB80/0x8EBC8/0x8EC20/0x8EC68`, `0x5A998`, `FUN_00093944`, `0x57AC5/C7` | **per-entity table + score pair derived**; call-site event semantics partially open |

## 4. Port: `fifa96_ring_stats`

`include/fifa96_loader/fifa96_ring_stats.h` +
`src/fifa96_loader/fifa96_ring_stats.c` (caller-owned data, no globals, no
comments, `-fifa96_err_t` for invalid arguments). Scope: the ring push/get/wrap
mechanics with the `FUN_00092998` scan policy, and the `0x5A998` block
mechanics with the `FUN_0008ED40` weight and the `FUN_0008EE04` argmax.

| original | port |
|---|---|
| ring `0x5B440`, stride `0x15`, count 25 | `fifa96_history_record[]` + `capacity`, cursor is a separate caller byte |
| type/+1 time/+5 entity/+9 triple | struct fields `type`, `time`, `entity`, `camera_x/y/z` (natural padding instead of the packed 0x15 bytes) |
| reset `FUN_000927E0` (type byte per slot + cursor) | `fifa96_history_reset` (zeroes whole records + cursor) |
| `FUN_000928F0` next `(cur+1)%0x19`, cursor=newest | `fifa96_history_push` (returns written index) |
| `FUN_00092998` newest-first walk with `DEC/wrap 0x18` | `fifa96_history_get(age)` |
| flag table `flat 0x110F1C + type`, mask DL | `type_flags[type] & mask`; `type >= type_count` treated as 0 (the original table has no bound check) |
| side filter `BL` 0/1 vs `ptr[0][0x826]` | `side` 0/1 + `side_of` callback (other values = any) |
| mask-4 arm types 0x1C/0x1D: `(y>0) == side` skips | `arm_ok` callback; `-FIFA96_ERR_INVALID` if a non-NULL entity of those types is reached without one |
| ordinal AL, `DEC CX`, 0 never matches | `uint16_t ordinal`, wrap on decrement, `-FIFA96_ERR_NOT_FOUND` |
| table `0x5A998`, 2×16, stride `0xA`, key/+4 word/+6 dword | `fifa96_stat_slot { key, count, value }`, `block_count`/`slots_per_block` |
| `FUN_0008ED40` find-by-key + add | `fifa96_stats_accumulate` (returns 1 found / 0 not found) |
| `FUN_0008ED40` weight `2x` / `1x` / `delta + (delta>>2)` | `fifa96_stats_weight(delta, rank_above, class_in_range)` |
| `FUN_0008ECC4` `count++`; `[0x5B358] = (count == 3)` | `fifa96_stats_bump(..., threshold)` returns `(int16_t)count == (int16_t)threshold`; the flag write stays with the caller |
| `0x8EB80/0x8EBC8/0x8EC20/0x8EC68` | `fifa96_stats_get_count` / `fifa96_stats_get_value` |
| `FUN_0008EE04` add + max scan (returns max block, writes max key) | `fifa96_stats_accumulate` + `fifa96_stats_argmax` (returns block, writes key and value) |

Not ported: the stage/commit protocol and the `0x26` sentinel, the camera
triple copy, the global key tables, the `[0x5AFB0]/[0x5AFB4]/[0x5B434]` max
publish, the `[0x5B358]` flag, the entity/team fields used by the weight
(callers supply the two booleans), and the goal words (a plain two-counter
array whose event side effects are out of scope).

## 5. Tests

`tests/test_ring_stats.c`, suite 60 → **61**. Ring: reset zeroing and NULL
errors; push advance and wrap indices; overwrite of the oldest slot with
capacity 3 and capacity 1; newest-first `get` with wrap and `age >= capacity`
`-NOT_FOUND`; `find` flag-mask filtering with newest-first order, ordinal 2,
non-matching masks, out-of-range type treated as 0, side filter (0/1/any) and
its NULL-callback error; the mask-4 `0x1C` arm (reject/accept/NULL entity and
the missing-callback error); ordinal 0 never matches. Stats: reset; accumulate
match/block/miss and negative deltas; dword wrap `0x7FFFFFFF + 1`; bump
threshold equality (3rd bump), not-found, block/slot bounds; uint16 counter
wrap `0xFFFF→0`; value/count accessor `-NOT_FOUND`/`-INVALID`; argmax strict
first-wins across blocks and its error cases; the 2×/1×/1.25× weight including
negative shifts. ASan+UBSan build clean (`cc -fsanitize=address,undefined
-Wall -Wextra -Werror -Iinclude tests/test_ring_stats.c
src/fifa96_loader/fifa96_ring_stats.c`). `make test`: 60/60 before, **61/61
after**.

## 6. Errata (quoted)

* FU-67 §3.6 "`FUN_0008ED40(entity, value)` (called from
  `FUN_00072478`/`FUN_00088940`/`FUN_0008E244`)" — **refined**: the 29 direct
  CALL sites are the list in §2.2; `0x88940` and `0x8E244` do not call it.
* FU-67 §3.4 "cursor `c = [0x5B662]>>24`; `[0x5B665] = (c+1) % 0x19`;
  `[0x5B440 + c*0x15] = type`" — **refined**: after storing the new cursor the
  index is reloaded from `[0x5B662]>>24` (`0x928BE`/`0x92926`), so the write
  slot is the **new** cursor (the cursor points at the last written record).
* FU-67 §3.4 "gate byte `[0x5B650] != 0x26`" — **extended**: `0x26` is staged
  as a suppression sentinel by `FUN_00070544`/`FUN_00071DF4`/`FUN_000736AC`
  when nothing else is staged; both commit and direct insert also clear the
  staging type.
* FU-67 §3.4 "Layout: `+0` type byte, `+5` entity pointer, `+9..0x14` camera
  triple" — **extended**: `+1` is a dword timestamp (`FUN_000928F0 0x92976`,
  `FUN_00092820 0x9284E`), never read back by `FUN_00092998`.
* FU-67 §3.4 "`FUN_00092998` … starts at `[0x5B665]`" — **confirmed**, with the
  full filter decode in §1.4; `disassemble_function` drops `0x929F0..F3`.
* FU-67 §2 "`[0x5B43C]` staging record: if non-null with `byte[0] == 0xB`" —
  **refined**: `[0x5B43C]` is the ring-record pointer returned by
  `FUN_00092998(1,4,any)` at `0x7145E`, written `0x71463`, read `0x726E9`.
* FU-67 §3.6 "publishes `[0x5AFB0]`/`[0x5AFB4] = [[entity+4]]`/`[0x5B434] =
  (word)xor`" — **refined exact fields**: `[0x5AFB0] = (int32)(int16)value`,
  `[0x5AFB4] = [[entity+4]]`, `[0x5B434] = (word)block` (`0x8EDDC..0x8EDF3`).
* FU-67 §3.6 "`xor = (int16)FUN_000741B4(entity-side)`" — **confirmed**:
  `FUN_000741B4(a) = (a ^ [0x57ABE]) & 1` (`0x741B4..C3`).
* FU-67 §3.6 "stride `0xA`, block offset `xor*0xA0`" — **confirmed and
  extended** with the `+4` word counter and its accessors/increment path.
* §2.4 "Nine call sites (`0x93D98/0x93DA1/0x94026/0x9402F/0x94489/0x94492/
  0x94667/0x94670/0x9486E`)" — **corrected to eleven**: the Task 15 first-hand
  census (`get_xrefs_to 0x93944` and `search_instructions` mnemonic `CALL`
  operand `93944`; FU-142 Appendix I.10) returns 11 `UNCONDITIONAL_CALL` sites;
  the §2.4 list omits the `0x941E5`/`0x941EE` pair (the same two-call block
  shape as `0x94026/0x9402F`). `FUN_00093944`'s semantics and the "no wired
  writer" T15 conclusion are unaffected.

## 7. Open legs

* **Ring role semantics**: the record shape (type, time, entity, camera triple)
  and all writers/readers are derived, but the gameplay meaning of the history
  (camera pan memory vs action memory) is not asserted.
* **Ring `+9` camera triple** has no reader in analyzed code.
* **Multiple stages per frame**: `FUN_00092820` overwrites the staging record
  unconditionally; which caller wins when several stage before the commit was
  not derived.
* **`FUN_0008EE04` callers** were not located (unanalyzed gap), and the five
  accessors `0x8EB80/0x8EBC8/0x8EC20/0x8EC68` have no xrefs in analyzed code —
  functional evidence only.
* **`[0x5AFB0]/[0x5AFB4]/[0x5B434]`**: no consumer besides the reset.
* **Goal handler details**: `FUN_0009252C` ids `0x9A..0xA0` and the
  `CALL 0xCBC4C` condition are read from the listing; the called functions'
  semantics (commentary/celebration?) are not derived. `[0x5B6A4]`/`[0x5B6B4]`
  are goal-difference/max-difference bookkeeping by shape only.
* **`team+0x828`** (`0x590CC`/`0x59901`): flag read with the score by
  `FUN_00091528`; role open.
* **The `0x57AD0..0x57AEA` word cluster** zeroed by `FUN_00073EE0`: roles open.
* **`0x5A998` slot `+4` word**: only increments (`FUN_0008ECC4`) and accessors
  are evidenced; its gameplay label is not asserted.
* **`FUN_00091BC4`'s output** is a full match reset; the roles of the
  `0x5B35x/0x5B36x/0x5B37x` timers beyond the FU-63/FU-69 references are not
  re-derived here.
* **Static key tables** `flat 0x14C360`/`0x14C3DB` are runtime-populated; the
  slot identity is a per-match object dword.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`disassemble_function` 0x927E0, 0x92820, 0x92864, 0x928F0, 0x92998, 0x8ED40,
0x8E244, 0x8EEC8, 0x8EF38, 0x93944, 0x91528, 0x741B4, 0x700F4, 0x8ECC4, 0x9E8D0,
0xCB2A4;
`disassemble_bytes` 0x929E0/0x929E8 (read_memory), 0x8EB7E, 0x8EE04, 0x8AD00,
0x70000, 0x8AE60, 0x72478, 0x72BB0, 0x71F48, 0x71450, 0x72630, 0x726F0,
0x72A30, 0x72DC8, 0x72EB0, 0x77430, 0x77E20, 0x80D50, 0x813B0, 0x71418,
0x71490, 0x71550, 0x715B0, 0x716A0, 0x71708, 0x717B8, 0x7B030, 0x70640,
0x71E18, 0x73A98, 0x73EE0, 0x93944, 0x85C30, 0x75290, 0x7F650, 0x70DA0, 0x7B170;
`decompile_function` 0x92998;
`read_memory` 0x110F1C (96 B), 0x14C360 (64 B), 0x14C3DB (64 B);
`search_instructions` 5a998, 5a99e, 5afb0, 5afb4, 5b434, 5a99c, 57ac5, 57ac7,
57abe, 8d8ec;
`get_xrefs_to` 0x927E0, 0x92820, 0x92864, 0x928F0, 0x92998, 0x5B440, 0x5B650,
0x5B655, 0x5B665, 0x5B64D, 0x5B43C, 0x5B358, 0x5A998, 0x5A99E, 0x5AFB0,
0x5AFB4, 0x5B434, 0x8ED40, 0x8ECC4, 0x91BC4, 0x4C360, 0x4C3DB, 0x590CC,
0x57AC5, 0x57AC7, 0x57AC0, 0x93944, 0x935A0, 0x4B5DC, 0x73EE0, 0x700F4;
`get_function_by_address` for the ring/stat/score functions and neighbours.

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_ring_stats.h`,
`src/fifa96_loader/fifa96_ring_stats.c`, `tests/test_ring_stats.c`,
`CMakeLists.txt` (one library/test block). `make test`: 60/60 before, **61/61
after**; ASan+UBSan `test_ring_stats` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.

## 8. Errata — C3-OL2 (M2 playability Task 4)

First-hand re-verification on `/FIFA96.EXE` (read-only) of the `FUN_00093944`
goal writer and its eleven call sites; the full window, tables and citations
live in FU-142 Appendix L.

* **§2.4 "posts the threshold event ids 0x9A/0x9B/0x9C/0x9D/0x9E/0x9F/0xA0" —
  extended with the 0xD3 arm.** When `side != [0x15B6B4]`, `score[side] == 1`,
  `score[side^1] < 3` and the `FUN_000CBC4C` return byte has bit 0 or 1 set
  (`0x939D1 CALL 0xCBC4C` / `0x939D6 TEST AL,3`), the writer posts id **0xD3**
  and returns (`0x939DA..0x939DF`, shared `0x93B73` `CALL 0x9252C`).
* **§2.4 "maintains the goal-difference bookkeeping `[0x5B6A4]`/`[0x5B6B4]`"
  — exact decode.** `[0x15B6B4] == -1` returns after the increment and the
  `[0x15B670]` store only (`0x9395E..0x93961`); otherwise
  `diff = (int16)(score[other] - score[tracked])` (`other = tracked == 0 ? 1 :
  0`, `0x9398B MOVSX DX`) updates `[0x15B6A4] = max(...)` (`0x93992`), and the
  tracked arm's 0x9A test is `diff + 3 == max_diff && max_diff > 3`
  (`0x93AA6..0x93AB4`). The non-tracked arm's 0x9E/0x9F/0xA0 checks are
  `4/other<2`, `7/other<3`, `9/other<4`; the tracked arm's are `3-0`, `5/other<3`,
  `9/other<5`. Every posting arm returns through the shared epilogue: at most
  one id per call.
* **§2.4 call-site contexts — all eleven are goal-screen handler sites.**
  `0x93D98`/`0x93DA1` in `0x93BBC`, `0x94026`/`0x9402F` in `0x93E20`,
  `0x941E5`/`0x941EE` in `0x940A4`, `0x94489`/`0x94492` in `0x94270`,
  `0x94667`/`0x94670` in `0x944FC`, `0x9486E` in `0x946C4`. The six handlers
  are period-indexed by the table at flat `0x110F78` (installed into
  `[0x15B6D4]` by `FUN_00092E2C 0x92EC4..0x92EF7` from `(int16)[0x15B680]`),
  each dispatching the pending id `[0x15B6A8] - 1` through its own inline
  table (side map in FU-142 L.4). No action/phase table slot contains a
  `0x93xxx` address, so no wired dispatch reaches a writer site (this is the
  task's carried leg OL-87/OL-88).
* **§2.4 "Reset writers: `FUN_00092E2C` ... (match reset ...)" — confirmed
  and placed.** `FUN_00092E2C` zeroes the score pair at `0x92E7B`/`0x92E82`
  and `[0x15B6A4]` at `0x92E5B`; `FUN_00092D8C` writes `[0x15B6B4]` (`1` when
  `[0x1590CC]==0`, `0` when `[0x159901]==0`, else `-1`; `0x92DEF/0x92E00/
  0x92E08`) and is called from the replay/screen setup `FUN_0003BB1C`/
  `FUN_00038630`, not the match init.
* **§2.4 "call sites" + §6 "corrected to eleven" — confirmed**: `get_xrefs_to
  0x93944` = 11 `UNCONDITIONAL_CALL` at the addresses listed above; the
  `0x9252C` helper's empty Ghidra xref list (FU-63 §5) is an artifact of the
  unanalyzed goal cluster — the writer is its caller.
* **§4 "Not ported: ... the goal words (a plain two-counter array whose event
  side effects are out of scope)" — superseded.** The writer is now ported as
  `fifa96_action_score_event` (`include/fifa96_loader/fifa96_action_handlers.h`)
  and wired as `fifa96_match_run_score_event`; the engine carries the writer's
  `last_side`/`tracked_side`/`max_diff` cells with the -1 tracked default
  (producer unported, OL-87) and captures the posted id in `score_last_event`.
  `fifa96_match_run_add_goal` remains the plain increment for the unported
  gameplay paths. The M2 tape's goal step runs the derived source and stays
  byte-identical (FU-142 L.6/L.8).
