# FU-67: the entity/coordinate update — pool, per-frame chain and selection pass

Roadmap slice #5 of FU-58 §7 ("entity/coordinate update"): `FUN_00088940`,
caller `FUN_0008AF38`, and `FUN_00072478`, caller `FUN_00072AC4`, plus the
`0x577xx`/`0x57Axx` globals. This slice derives both functions, traces the
entity pool they address, maps the per-object update chain reached from
`FUN_0004B100`, and ports the clean self-contained piece — the distance/
magnitude helper `FUN_0008DC68` with the nearest-record search `FUN_0008DE8C`.

Result in one line: **the entity pool is two team blocks at `0x588A4`
(side 0) and `0x590D9` (side 1), stride `0x835`, each holding 11 player
records of stride `0xB2` (`x` +0x59, `y` +0x61, exclusion bytes +0x98/+0x9A,
type +0x8E, action vtable +0x18, team back-pointer +0) with record 0 on a
separate update path and the controlled-player pointer at `+0x7B2`; per drained frame
`FUN_0004B100` runs the 4 control-state slots `0x57C64` (`FUN_00078A54`),
the camera/track chain `FUN_000736AC`→`FUN_00072AC4`→`FUN_00072478` (which
posts command-ring ids by thresholds on `[0x5774C]/[0x57750]/[0x57754]`),
the clock machine `FUN_0008AF38` (whose tail calls the entity selection pass
`FUN_00088940` for phases 2/0x10 when `[0x5781D]!=0`), then
`FUN_0008D8EC(team0)`/`FUN_0008D8EC(team1)` — keeper `FUN_000782D0` plus
outfield `FUN_0007CA54` per active record — and the ball/possession pairing
`FUN_0007D430`; the metric `FUN_0008DC68` (a piecewise `max + 3/8·min` / `max + 1/4·min` word-distance
approximation) and the nearest of 11 records `FUN_0008DE8C` are ported as
`fifa96_entity_update`.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-66). All
  instructions quoted below were read back this slice
  (`disassemble_function`/`disassemble_bytes`/`read_memory`); the decompiler
  was used for callee characterisation where it succeeded.
* **Decompiler pruning met this slice (new errata):** `decompile_function
  0x72478` and `0x72AC4` print many `Removing unreachable block` warnings and
  the extracted code is incomplete (the `FUN_00072478` call in `FUN_00072AC4`
  is inside a pruned block; `0x88940`'s `FUN_00092040` arguments are
  mis-inferred). The behaviour below is from the full
  `disassemble_function 0x88940` (205 instructions, clean),
  `disassemble_function 0x72478` (477), `disassemble_function 0x736AC`
  (385), `disassemble_function 0x8D8EC` (179) and `disassemble_bytes`
  windows at `0x72AC4`, `0x8B600`, `0x8DC68`, `0x8DE8C`, `0x927E0`,
  `0x928F0`.
* **Address convention (FU-59/FU-62, quoted):** code immediates naming
  tables carry LE fixups; the loader adds the owning object's base. This
  slice reads one new table as bytes: the entity-type flag table at encoded
  `0x10F19` resolves to **flat `0x110F19`** (base `0x100000`; the byte read
  at `[EDX + 0x10F19] >> 24` is `flat[0x110F1C + type]` — the object-1
  candidate `0x20F19` is code). Direction tables shown as `0xE1DC/0xE1EC/
  0xE1F5/0x1064E` are quoted as Ghidra displays them; their flat form was not
  re-read (open leg).
* All numeric claims (offsets, thresholds, strides) are quoted from the
  listings; semantic labels beyond what the instructions do are not asserted.

## 1. `FUN_00088940` — entity selection/announce pass

**Entry condition.** Sole caller `0x8B63E` in the `FUN_0008AF38` tail
(`get_xrefs_to 0x88940` → `{0x8B63E}`), byte-verified this slice
(`disassemble_bytes 0x8B600`):

```
0x8B623  EAX = [0x57A4A] >> 24
0x8B62B  CMP EAX,2  / 0x8B630 CMP EAX,0x10 / 0x8B633 JNZ 0x8B645
0x8B635  CMP byte [0x5781D],0 / JZ 0x8B65B
0x8B63E  CALL 0x88940
0x8B645  CMP EAX,0xC / JNZ 0x8B65B
0x8B64A  [0x57ABA]=0; [0x5882D]=0        ; phase 0xC arm
```

so the pass runs for phase 2 or 0x10 when the `[0x5781D]` flag is set. The
same flag gates the tracker path in `FUN_00072AC4` (§2). `FUN_000740A0` sets
`[0x5781D]=0` when it enters phase 2 (FU-62 §1.2), so this is a
"reset on phase change, resolve while active" flag.

**Body (205 instructions, `0x88940..0x88C0E`).**

```
0x88949  EBX=-1; EDX=0; EAX=1
0x88955  [0x57A9F]=0; [0x57A9B]=0
0x88961  EDX=4; CALL 0x92998              ; search(1, mask 4, any side)
0x8896b  EDX=|[0x57784]|
0x8897d  CMP EDX,0xB20 / JLE 0x88BCC
```

* **`|[0x57784]| <= 0xB20`** (`0x88BCC`): if phase == 2,
  `FUN_000974DC()` then `FUN_0008A938` with a code derived from the ball-side
  byte `[ball[0]+0x826]` (`0x88BE3..0x88C00`; `side^1` in EDX, a `0/1` value
  plus 3 in EAX). Nothing is written to `[0x57A9F]` on this arm.
* **`> 0xB20` and `[0x5781E] != 0`** (`0x88989`): the side is selected as
  `side = ([0x57A49]>>24 == 1) ? [*[0x587D4]][+0x826] : ([0x57784] < 0)`
  (`0x88996..0x889C0`).
  * If the `FUN_00092998(1,4,any)` result has a non-null pointer at `+5`
    whose `[ptr[0]+0x826] == side`, `[0x57A9F]` is set to it
    (`0x889C7..0x889F1`), `FUN_000741B4` is called and EAX becomes 2 or 3
    (`0x889F6..0x88A09`).
  * Else `[0x57A9B]` is set from `[result+5]` (`0x88A0E..0x88A1C`); when its
    `+0x5D` is null it calls `FUN_0006E598(type = ptr[+0x8B]>>24, 0x5D)`
    (`0x88A25..0x88A35`); when `[0x57A9B+0x8D] != 0` it copies
    `0x588A4 + (side==0)*0x835` (`0x88A53..0x88A61`), calls
    `FUN_000795B4(ptr+0x59, team+0x59)` and `FUN_00079C50`, and calls
    `FUN_0006E598(team[+0x8B]>>24, 0x55)` when `team[+0x5D] == 0`
    (`0x88A86..0x88AA2`). This is the only place this slice reads
    `team_block+0x59/+0x5D/+0x8B` through the `+0x835` stride.
  * `[0x57A9F] = [0x577CE + side*4]`; if null or `[+0x9A] != 0` it is
    replaced by the nearest of the side's 11 records:
    `FUN_0008DE8C(0x5777C, 0x588A4 + side*0x835, skip 0, out NULL)`
    (`0x88AA9..0x88AE3`).
  * `FUN_000741B4`; when it yields 0, `FUN_00092040(0xC8, 0)` and
    `FUN_00092040(0xC8, 1)` run (`0x88AE8..0x88B11`).
  * Common tail `0x88B1B`: `FUN_000651F0(EAX = 2/3/4)`,
    `FUN_000974F0(0xBB8)`, then
    `FUN_0008A938(EAX=6, EDX=[0x57A9F][0][+0x826])` (`0x88B1B..0x88B44`).
* **`> 0xB20` and `[0x5781E] == 0`** (`0x88B53`): phase == 2 → `FUN_000974DC()`
  and `FUN_0008A938` with `EAX = 3 + (side-of-ball != ([0x57784]<0))`
  (`0x88B64..0x88BBD`). This closes FU-62 §5's description of the same path.

FU-62 §5 stated "`FUN_00092040(0)`/`(1)`"; the listing shows the first
argument is `EAX=0xC8` and `EDX=0`/`1` (`0x88AFB..0x88B11`) — EDX is the
team selector, so the FU-62 phrasing is refined, not contradicted
(errata 1).

## 2. `FUN_00072478` — the tracker's command-ring decision pass

**Entry condition.** Sole caller `0x72AE5` in `FUN_00072AC4`
(`get_xrefs_to 0x72478` → `{0x72AE5}`), byte-verified
(`disassemble_bytes 0x72AC4`):

```
0x72ACA  CMP byte [0x5781E],0 / JZ 0x72AEC
0x72AD3  CMP byte [0x5781D],0 / JZ 0x72AEC
0x72ADC  CMP byte [0x5759D],0 / JNZ 0x72AF4   ; once per tracker pass
0x72AE5  CALL 0x72478
0x72AEC  [0x5759D]=0                          ; tracking inactive
0x72AF4  CMP [0x575E8],0 / ... / 0x72B19 CALL 0x721C8  ; ball-only arm
0x72B1E  phase != 2 -> [0x57A87]=[0x57A8B]=0
```

`FUN_00072478` itself sets `[0x5759D]=1` at `0x724C8`, so the pass runs at
most once per `FUN_00072AC4` call while tracking is active.

**Head** (`0x72478..0x72527`): `EDI = ([0x57754] < 0)`,
`FUN_00092998(1, mask 2, any)` then `FUN_00092998(2, mask 2, any)` (result
kept in ESI), then `FUN_00092998(1, mask 4, any)`.

**Previous-phase 0x10 arm** (`0x72A48..0x72A9E`; the test at `0x724CF` is
`[0x57A4B]>>24`, i.e. the **previous** phase byte `0x57A4E`, not the current
phase): `FUN_0008ED40(0x577CA, 0xF)` then a
three-way `FUN_0008F188` on the camera coordinate: `[0x5774C] > 0xA0` →
id `0x33`, `< -0xA0` → id `0x34`, else id `0x32`; all with code 8
(`EBX=8`), target `[0x577CA]`, then return.

**Main arm (phase != 0x10)**

* `[0x57823] != 0` clears the flag, calls `FUN_000741B4(side)`, and
  increments a 16-bit slot of the `0x57ACC` array at the result index
  (`0x724E2..0x724FA`).
* Records from the mask-2/mask-4 searches are accepted when their side
  matches `(side == 0)`; on acceptance, `FUN_0008F188(id=0x35,
  EDX=[0x577CA], EBX=0x40)` or the `[0x57A83]` variant is posted
  (`0x72502..0x725E3`).
* Fall-through `0x725E8`: `EDI = 0xA0 - [0x57750]`,
  `EBX = 0xD0 - |[0x5774C]|`; return if `[0x577CA] == 0` or
  `[ball[0]+0x826] != ([0x57754]<0)`; else
  `FUN_0008ED40([0x577CA], 0x32)` and
  `FUN_0008ED40([ball[0]+0x7A6], -0x4B)` (`0x7263F..0x72660`).
* Ball at the tracked entity `[0x57A8B]` with `[0x57A97] > 0x12C`:
  `|ball[+0x61]| > 0x840` and `|ball[+0x59]| < 0x4B0` →
  `FUN_0008F188(id=0x30, target=ball, EBX=0x40)` and `(id=0x3C, EBX=0x20)`,
  return (`0x72665..0x726E8`).
* `[0x5B43C]` staging record: if non-null with `byte[0] == 0xB`,
  `FUN_0008ED40([rec+5], 0x19)` and `FUN_0008F188(id=0x30, target=ball,
  EBX=0x20)`; then the `FUN_00092998(1,2,any)` result (ESI) is checked for
  `byte[0] == 0x12` and `[+5] != 0` → id `0x39` plus RNG-gated
  `0x93`/`0x3E`/`0x42` posts (`0x726E9..0x727D7`).
* `FUN_0008EEC8(side)` returns a word; when non-zero and
  `[0x57AC5]==[0x57AC7]` and `FUN_0008EF38() > 4` and the code `0x7EF38 < 0xB`
  → `FUN_0008F188(id=0x30, EBX=0x40)` + `(id=0x40, EBX=0x20)`
  (`0x727D8..0x7284D`).
* Distance banding on `[0x575A7]` and `[0x5759F]` (`0x7284E..0x72A47`), with
  the thresholds `0x570`, `0x840`, `0x2D0`, `0x750`, `0x660`, `0x68`, `0x20`,
  `0x30` and RNG picks (`FUN_00092AC8` bit 0x3 / bit 0x1); it posts ids
  `0x30`, `0x3B`, `0x2E`, `0x3E`, `0x93`, `0x2F`, `0x92`, `0x35`, `0x40`.
* Every path ends at `FUN_0008F188` with a target pointer (`0x72ABA` RET is
  `POP`s only when a prior RET jumped there). `[0x57A8F] += delta` on the
  final path.

So the pass is a **threshold-driven command-ring post table around the ball
and the tracked entity** — it computes no positions itself; the position
updates are `FUN_000736AC` (§4) and the team updates (§5). FU-58 §6's
"coordinate/possession-style update" label is refined accordingly (errata 2).

## 3. The entity pool

### 3.1 Team blocks: base, stride, side

`FUN_0004B100` loads the two block bases as immediates
(`disassemble_function 0x4B100`):

```
0x4B14A  EBX = 0x588A4        ; team 0
0x4B169  EDX = 0x590D9        ; team 1
0x4B2A9  EAX = EBX; CALL 0x8D8EC
0x4B2B0  EAX = EDX; CALL 0x8D8EC
0x4B2C4  EAX = [EBX+0x7B2]; ECX = [EDX+0x7B2]; EDX=ECX; CALL 0x7D430
```

`0x590D9 - 0x588A4 = 0x835` and `0x59056 = 0x588A4+0x7B2` (FU-62 §3.2) /
`0x5988B = 0x590D9+0x7B2` confirm the stride and the `+0x7B2` field. The
side byte is `team+0x826` (`FUN_0008D8EC 0x8D9E5`, `FUN_00088940 0x88B6E`).
`FUN_00088940` also indexes the blocks as `0x588A4 + side*0x835`
(`0x88AC8..0x88AD7`). Fields read by this slice:

| team offset | width | evidenced use |
|---|---|---|
| `+0x59` | dword | position triple `+0x59/+0x5D/+0x61` (`FUN_000795B4` arg, `0x88A67`) |
| `+0x5D` | dword | position triple; zero-tested at `0x88A86` |
| `+0x61` | dword | position triple (`FUN_0007D430` copy source) |
| `+0x7A6` | dword | pointer to another object; passed as an entity to `FUN_0008ED40(-0x4B)` (`0x7265A`) and used as `[[team+0x7A6]+0x7B2]` by the tracker/keeper (`FUN_00072AC4`, `FUN_000782D0`) |
| `+0x7B2` | dword | controlled/target entity pointer (`0x4B2C4`, `FUN_0008D8EC` writes, `FUN_000736AC` reads) |
| `+0x7BA` | dword | interception/target entity pointer, zeroed each `FUN_0008D8EC` |
| `+0x7BE` | byte | flag set by the `>0x480` target arm |
| `+0x7CB` | dword | timer enable (paired with `+0x81E`/`+0x820`) |
| `+0x81E` | dword | timer limit, high word compared to the frame delta |
| `+0x820` | word | timer countdown |
| `+0x826` | byte | side (0/1) |
| `+0x82C` | byte | per-update rotation counter, wraps at 0xB |
| `+0x835` | — | block stride |

### 3.2 Player records: stride `0xB2`, 11 per team

`FUN_0008DE8C` walks exactly 11 records of stride `0xB2`
(`0x8DEA7..0x8DEEF`: `CMP ECX,0xB / JL`, `ADD EBX,0xB2`) starting at the
team base. `FUN_0008D8EC` repeats the walk for records 1..10
(`0x8DB3A..0x8DB5F`: `EDX = EBP+0xB2`, `CMP EAX,0xB`, `ADD EDX,0xB2`).
`11 * 0xB2 = 0x7A6`, exactly the offset of the `+0x7A6` pointer, so the
records occupy `team+0 .. team+0x7A5`. Fields evidenced in this slice:

| record offset | width | evidenced use |
|---|---|---|
| `+0x00` | dword | back-pointer to the owning team block; its `+0x826` is the side (`0x889D3`, `0x8D9E3`, `0x7250D`) |
| `+0x04` | dword | pointer dereferenced by `FUN_0008ED40` as the accumulator key (`0x8EDB5`); checked by `FUN_000736AC` as `[[entity+4]][0] == 0x18D8` (`0x73A28`) |
| `+0x18` | dword | per-record action function: `(*(code*)in_EAX[6])()` (`FUN_0007CA54`, `FUN_000782D0`) |
| `+0x20` | dword | pointer zero-tested by the interception arm (`FUN_0008D8EC 0x8DA5D`) |
| `+0x4D/+0x51/+0x55` | 3 dwords | output/world position triple written by `FUN_0008D824`/`FUN_0007D430` |
| `+0x59` | dword | X (`[EDI]`-relative word in `FUN_0008DE8C 0x8DED2`; dword in `FUN_0007D430`) |
| `+0x5D` | dword | third axis/dword (`FUN_000782D0` compares `[sel+0x5D]+0x70-0x10`) |
| `+0x61` | dword | Y (`FUN_0008DE8C 0x8DEC8`) |
| `+0x69` | dword | high word compared (`<0x1E0` at `0x8DAAE`, `<0xC0` in `FUN_00072AC4`, `<0x50` in `FUN_0007CA54`) |
| `+0x71/+0x73` | dwords | velocity pair: `+0x59 + ([+0x71]>>16)*delta` in `FUN_0007D430 0x8D...` (`0x7D430` head) |
| `+0x75` | word | sign-tested in the 0x690 arm of `FUN_00072AC4` |
| `+0x79/+0x7B` | words | `+0x7B = +0x79` copy in `FUN_000782D0` |
| `+0x7F` | dword | high word is a timer limit (`FUN_0007CA54`, `FUN_000782D0`) |
| `+0x81` | word | countdown timer |
| `+0x8A` | dword | high byte read as an index/type (`0x8DA3D`, skip index in `FUN_0008D8EC`'s interception search) |
| `+0x8B` | dword | high byte = type passed to `FUN_0006E598` (`0x88A32`) |
| `+0x8D` | byte | active/controlled flag (`FUN_00088940`, `FUN_00072AC4`) |
| `+0x8E` | dword | high byte = type; compared `!=5`, `<5`; gates table 0x110F1C in `FUN_0007CA54` |
| `+0x91` | byte | compared with the table-selected code in `FUN_000782D0` |
| `+0x93` | byte | countdown timer (`FUN_0007CA54`, `FUN_000782D0`) |
| `+0x98` | byte | exclusion flag (`FUN_0008DE8C 0x8DEBB`) |
| `+0x9A` | byte | exclusion flag (`0x8DEB2`, `FUN_0008D8EC` player loop, `FUN_00088940`) |
| `+0xB2` | — | record stride |

The keeper is record 0: `FUN_0008DE8C` is called with skip index 0 from
`FUN_00088940` (`0x88AD5`), `FUN_0008D8EC`'s interception search skips the
record index taken from `[team+0x7B2][+0x8A]>>24` (`0x8DA36..0x8DA57`), and
`FUN_0008D8EC` updates record 0 through `FUN_000782D0` and records 1..10
through `FUN_0007CA54` (`0x8DB2E..0x8DB5F`). No label "keeper" is asserted —
the evidence is only that index 0 follows a different code path.

### 3.3 Nearest-record search `FUN_0008DE8C` and metric `FUN_0008DC68`

`disassemble_function 0x8DE8C` (42 instructions) is exact:

```
in: EAX = target record (word X at +0, word Y at +8), EDX = record base,
    BX = skip index, ECX = out word pointer
0x8DE9C  SI = 0xFFFF
for ECX = 0 .. 0xA:
  skip if ECX == sign_extend16(BX)                    (0x8DEA7..0x8DEB0)
  skip if [rec+0x9A] != 0 or [rec+0x98] != 0          (0x8DEB2..0x8DEBF)
  EDX = MOVSX16([target+8] - [rec+0x61])              (Y)
  EAX = CWDE([target+0] - [rec+0x59])                 (X)
  CALL 0x8DC68                                        (0x8DED7)
  if AX < SI (unsigned) -> best = rec, SI = AX       (0x8DEDC..0x8DEE3)
  rec += 0xB2
*out = SI; return best (0 if none)
```

`FUN_0008DC68` (`0x8DC68..0x8DCD2`, 54 instructions) is the word
distance/magnitude approximation used by the search and by the camera chain
(`0x73845`, `0x73CB3`) and by `FUN_00070544`/`FUN_000709D0`/`FUN_00070DE0`/
`FUN_00071DF4`/`FUN_00072AC4`. Decoded from the listing with
`m = min(|dx|,|dy|)` (16-bit signed, sign-extended) and `M = max(...)`:

| condition | result |
|---|---|
| `m <= M/2` | `M + (m >> 2)` (arithmetic shift) |
| `m > M/2` | `M + (((m >> 1) + (m >> 2)) >> 1)` |
| `m == M` | `(int16)(m) + (((m >> 2) + (m >> 1)) >> 1)` |

with the exact instruction semantics: the sign test is on the low word
(`TEST AX,AX`), the negation is 32-bit (`NEG EBX`), and the major term added
is the full 32-bit operand (`ADD EAX,EDX`/`ADD EAX,EBX`). For the
sign-extended word inputs the callers pass, this is `M + 3/8·m` when
`m > M/2`, else `M + 1/4·m`. The name of the algorithm is not asserted.

### 3.4 The 25-slot entity history ring `0x5B440`

`FUN_00092998` (`0x92998..0x92A94`) scans a ring of 25 records of stride
`0x15`: cursor `[0x5B665]` read at `0x929A8`, decremented with wrap at
`0x18` (`0x92A78..0x92A7E`), count 0x19 (`0x929B3`). The record was decoded
from the writers:

* `FUN_000927E0` (reset): `for i = 0..0x18: byte[0x5B440 + i*0x15] = 0`
  (`0x927E6..0x927EC`), then zeroes `[0x5B665]`, `[0x5B650]`, `[0x5B655]`,
  and sets `[0x5771A]=0x64` (`0x927FA..0x92815`).
* `FUN_000928F0` (insert, `0x928F0..0x92994`): gate byte `[0x5B650] != 0x26`
  (read as the unaligned dword `[0x5B64D]>>24`); cursor `c = [0x5B662]>>24`
  (byte `0x5B665`); `[0x5B665] = (c+1) % 0x19`;
  `[0x5B440 + c*0x15] = type`, and only when
  `entity_type_table[type] & 1` (flat `0x110F1C`): `[..+0x15*c+0x1] = now`,
  three dwords from `0x57758` copied to `+0x9`, and
  `[..+0x15*c+0x5] = entity pointer`; then `[0x5B650]=0`.

Layout: `+0` type byte, `+5` entity pointer, `+9..+0x14` camera triple,
size `0x15`. `FUN_00092998` reads the type through an unaligned dword at
`0x5B43D + i*0x15` (`>>24` lands on `+0`), reads the pointer at
`+0x5B445 = +5`, reads the type-flags from flat `0x110F1C`, filters by side
via `[entity[0]+0x826]` unless `BL > 1`, and has a special type-4 arm that
excludes types 0x1C/0x1D and checks the sign of `[entity+0x61]` against the
side (`0x929E3..0x92A78`, inside the one pruned window of
`disassemble_function 0x92998`; cited from `decompile_function 0x92998`).
Seven call sites:
`FUN_0007131C`, `FUN_00071DF4`, `FUN_00072AC4`, `FUN_00072478` ×3,
`FUN_00088940`.

### 3.5 The 4 control-state slots `0x57C64`

`FUN_00078A54` (`0x78A54..0x78A83`, sole caller `0x4B15E` in
`FUN_0004B100`) walks 4 records of stride `0x25`:

```
XOR EBX,EBX
0x78A58  CMP byte [EBX+0x57C86],0 / JL skip   ; active byte at slot+0x22
0x78A61  EDX = [0x57A62] >> 16                ; frame delta
0x78A67  EAX = 0x57C64 + EBX
0x78A71  CALL 0x78950                         ; slot, delta
0x78A76  EBX += 0x25 / CMP EBX,0x94 / JNZ
```

`FUN_00078950` (`0x78950..0x78A52`) per slot: reads `[slot+0x19]>>24` =
player index and calls `FUN_0004511D(index, 0)` (direction table flat
`0x4B1C8`, FU-61 §5), stores the direction word at `+0x12`, tracks
pressed/released bit words `+0x10/+0xC/+0xE/+4/+6`, adds the frame delta to
a `0xFA`-capped counter `+0x23`, and writes three animation bytes
`+0x20/+0x21/+0x1F` from the tables `0xE1DC/0xE1EC/0xE1F5` (and `0x1064E`
when `+0x1E == 0`). Slot `+0x22 >= 0` is active (byte compared signed at
`0x78A58`); `+0x1E` selects the table variant. So these are the up-to-4
controller/player control records, not the 22-player world pool.

### 3.6 Per-team stat accumulator `FUN_0008ED40`

`FUN_0008ED40(entity, value)` (called from `FUN_00072478`/`FUN_00088940`/
`FUN_0008E244`; 71 instructions) scales `value` to a multiplier — `2×`,
`1×` or `1.25×` — from the entity's `+0x8D` vs the team block's `+0x9`, and
`[entity+0x8A]>>24` vs `[team+0x6]>>24 + [team+0xD]>>24`
(`0x8ED4F..0x8ED81`), derives `xor = (int16)FUN_000741B4(entity-side)`
(`0x8ED83..0x8ED95`) and scans n = 0..0xF for
`[0x5A998 + n*0xA + xor*0xA0] == [[entity+4]]` (`0x8ED97..0x8EDFC`); on a hit
it adds the scaled value to `[0x5A99E + n*0xA + xor*0xA0]` and, when the
sign-extended low word exceeds `[0x5AFB0]`, publishes
`[0x5AFB0]`/`[0x5AFB4] = [[entity+4]]`/`[0x5B434] = (word)xor`
(`0x8EDC2..0x8EDF3`). So it is a 2-block × 16-slot table, stride `0xA`,
block offset `xor*0xA0`, keyed by a pointer. Role beyond
"per-side accumulators" is not asserted. This is the same `0x5A9xx` area as
FU-49/FU-63's ambience arrays.

## 4. The per-frame chain in `FUN_0004B100`

`disassemble_function 0x4B100` (137 instructions) gives the exact order
(FU-62 §3.2 covered the head; this slice adds the entity calls):

| # | site | call | note |
|---|---|---|---|
| 1 | `0x4B106` | match-over gate `[0x58822]` | returns 1 early |
| 2 | `0x4B11A` | parity toggle + Q8 split | FU-62 §3.2 |
| 3 | `0x4B15E` | `FUN_00078A54` | 4 control slots `0x57C64` with `[0x57A62]>>16` delta |
| 4 | `0x4B163` | `[0x5872D] -= delta` | pace countdown (byte) |
| 5 | `0x4B181` | `[0x4C195]=0; [0x4C110]=0x5774C` | entity stack reset (FU-62 open leg; still unclassified) |
| 6 | `0x4B193` | `FUN_000736AC` | camera/track chain, incl. `FUN_00072AC4`→`FUN_00072478` |
| 7 | `0x4B198` | `FUN_000948AC` if `[0x4C32A]!=0` | |
| 8 | `0x4B1A6` | `FUN_0008AF38` | clock machine; tail calls `FUN_00088940` |
| 9 | `0x4B1AB` | AX/`[0x58822]` dispatch | 0 → `0x4B2A9`; 1 → stop path; 2 → `0x4B257`; else `0x4B1C8` |
| 10 | `0x4B2A9` | `FUN_0008D8EC(0x588A4)`, `FUN_0008D8EC(0x590D9)` | per-team update |
| 11 | `0x4B2B7` | phase==2 && both `[team+0x7B2]` non-null → `FUN_0007D430(team0[+0x7B2], team1[+0x7B2])` | ball/possession pairing |
| 12 | `0x4B1AB`+ | returns 0 (fall-through `0x4B2DF`) | |

### 4.1 `FUN_000736AC` — camera/track chain

Sole caller `0x4B193` (`get_xrefs_to` → 1). 385 instructions; the parts
that touch entities/coordinates:

* `0x736B2..0x73722`: RNG drift of the word velocities
  `[0x577C0]`/`[0x577C2]` gated by `[0x577BC]>>16 > 0xC` and
  `[0x57750] > 0xA0`.
* `0x73723` `FUN_00092864`; `0x73728..0x73750`: copies
  `[0x577BE]/[0x577C0]/[0x577C2]` to shadow `[0x57825/27/29]`.
* `0x73756` `FUN_00072AC4` (sole caller, §2); then `MOVSD` copies the
  three dwords `0x5774C..0x57757` to the camera shadow `0x57758..0x57763`
  (`0x7375B..0x7375D`).
* `0x7376B..0x73834`: frame timer `[0x577FA] += delta`; `[0x57750] =
  FUN_00070B94([0x577F8]>>16)` when `[0x577F0]!=0`; boundary check
  `FUN_000709D0` when `[0x577FA] > [0x577F4]`; the per-tick integration
  `for n = 0..delta-1 { [0x577C6] += [0x577C0]; [0x577C8] += [0x577C2]; }`.
* `0x73834..0x73866`: `[0x577C4] = FUN_0008DC68([0x577C4]>>16,
  [0x577C6]>>16)` then `[0x5774C] += [0x577C4]>>16` and
  `[0x57754] += [0x577C6]>>16`; camera shadows `0x57788`/`0x57794`.
* `0x7386B..0x739D3`: turn accumulators `0x5780C/0x5780E`, the mixed
  `0x57810/0x57812`, and the quantised direction byte `0x57814`.
* `0x739CE..0x73B59`: ball-follow and velocity interpolation: checks
  `[[0x577CA]+4][0] == 0x18D8` (`0x73A28`), the `[0x4C1D4]/[0x4C1D6]` bits,
  `[0x4C2F6]`, `[0x5B650]`, `FUN_00092820`, then scales
  `([0x577F4]-[0x577FA])` by `[0x57816]/[0x57817]` into
  `[0x577C0]/[0x577C2]` and the interpolation buffers
  `0x57788/0x57794/0x57770/0x57778/0x57790`; `FUN_000703E8`.
* `0x73B5B..0x73B6B`: when `[0x577CA]` exists with `+0x20 != 0` →
  `FUN_00071DF4`.
* `0x73B70..0x73C38`: clamp `FUN_0007131C` when `|[0x5774C]| > 0x6C0` or
  `|[0x57754]| > 0xAB0`; `FUN_0006D870(0x5774C, 0x577D6, 0x577DA)` and
  `(0x57770, 0x577E6, 0x577EA)`; ball-follow render pick using `[0x59056]`
  (team 0 `+0x7B2`) then `[0x5988B]` (team 1 `+0x7B2`) with type 5,
  `FUN_0006D870(0x57770, 0x577DE, 0x577E2)`.
* `0x73C38..0x73CB8`: zoom scaling via IDIV `0xE4000`/`0x162000` (inputs
  `[0x577D7]`, `[0x577D6]`), writes `[0x577FC]`/`[0x577FE]`, then
  `[0x577BE] = FUN_0008DC68([0x577BE]>>16, [0x577C0]>>16)`.

### 4.2 `FUN_0008D8EC` — per-team update

Called for both team bases. 179 instructions:

```
0x8D8F5  EBP = EAX (team base)
0x8D8F7  [team+0x82C]++; if >= 0xB -> 0
0x8D919  [0x586D0] = 0
0x8D929  if (phase == 2 && ([team+0x7B2]==0 || [0x57A83]==0)):
           target = one of the 3-dword vectors 0x57788 / 0x57794 / 0x57770
                    (+ [0x577BE]>>16*0x20 and [0x577C0]>>16*0x20 for 0x57770)
           team[+0x7B2] = FUN_0008DE8C(target=ESP copy, team, skip 0, ...)
0x8D9BD  if (phase==2 && [0x57A83] && side matches):
           when |[0x57754]| > 0x480 and ([0x57754]<0)==side:
             skip = [team[+0x7B2]+0x8A]>>24
             team[+0x7BA] = nearest(0xF37C + side*0xC, team, skip)
             if [nearest+0x20]==0 -> FUN_0008D824([0x57A83], nearest+0x4D)
             else team[+0x7BA]=0
             FUN_000795B4(nearest, nearest+0x4D, &stack)
             if stack_word < 0xF0 and [nearest+0x69]>>16 < 0x1E0 and
                |nearest[+0x61]| > |[0x57754]| + 0x90 -> team[+0x7BE]=1
           else team[+0x7BA]=0
0x8DAF3  timer [team+0x7CB]/[+0x81E]/[+0x820] -= delta
0x8DB2E  FUN_000782D0(EAX=team, EBX=1)          ; record 0
0x8DB3A  for i=1..0xA: rec = team + i*0xB2
           if [rec+0x9A]==0: FUN_0007CA54(rec)  ; records 1..10
```

The two target constants: `0xF37C + side*0xC` is a 3-dword target record
(`0xF37C/0xF388`); `0x57788`/`0x57794`/`0x57770` are the camera vectors from
§4.1. The decompile of `FUN_0008D824` was not run (open leg), but its call
site passes `([0x57A83], nearest+0x4D)`.

### 4.3 `FUN_0007CA54` (records 1..10) and `FUN_000782D0` (record 0)

Both are large state machines over one record. Evidenced structure:

* Both decrement the per-record timers `[rec+0x81]` (limit `[rec+0x7F]>>16`),
  `[rec+0x93]`, and `[rec+0x7B]=[rec+0x79]` (the latter only in
  `FUN_000782D0`), then run a table-driven code path:
  * `FUN_0007CA54` dispatches on `[rec+0x8E]>>24` against tables
    `0x109D0`/`0x109E4` (`decompile_function 0x7CA54`) and calls
    `(*(code*)rec[6])()` (the `+0x18` action pointer), then `FUN_0006E8E8`,
    `FUN_0007BF20`, and `FUN_0007D9A4(0, code)` under a condition cluster
    (ball/tracked/`[0x57750] < 0x30`).
  * `FUN_000782D0` uses tables `0x10970/0x10978/0x10998/0x109A8/0x109C8/
    0x109DC/0x109E0` selected by side/tracked state and calls the same
    `rec[6]` action pointer, `FUN_0006E8E8`, `FUN_0007BF20`, plus
    `FUN_00077EAC` when `[0x57C5D]!=0`, and copies `+0x59/+0x5D/+0x61` into
    `+0x4D/+0x51/+0x55` when the record is `[0x57AA7]`.
* The tables and the action pointer are not decomposed (sub-slice S8).

### 4.4 `FUN_0007D430` — ball/possession pairing

`disassemble_function 0x7D430` (70 instructions): given the two
`[team+0x7B2]` entity pointers (EAX = team 0's, EDX = team 1's), predicts
the first's position forward with its `+0x71/+0x73` velocities ×
`(int16)[0x57A64]` (`0x7D44C..0x7D479`, `IMUL` not `SAR`), then
`FUN_0008DCD4(B+0x59, A+0x59, out1)` and
`FUN_0008DCD4(B+0x59, predicted-A, out2)`. It proceeds when the second
distance (`out2` word +0) is smaller than the first (`out1` word +0) and
`out2` word +0 `< 0x40` (`0x7D496..0x7D4AB`), copies team 1's
`+0x59/+0x5D/+0x61` triple into team 0's `+0x4D/+0x51/+0x55`
(`0x7D4AD..0x7D4B5`), and adds ±0x40 to `+0x4D` and `+0x55` from the signs
of `out2` words +2 and +4 (`0x7D4B6..0x7D4E8`). This is called only for
phase 2 (`0x4B2BF`).

## 5. Structural map and bounded sub-slices

Per-object world update of one drained frame:

| stage | entry | object | reads | writes |
|---|---|---|---|---|
| control state | `FUN_00078A54`→`FUN_00078950` | 4 slots `0x57C64` (stride `0x25`) | direction table `0x4B1C8`, tables `0xE1DC/0xE1EC/0xE1F5/0x1064E` | `+0x10/+0x12/+0xC/+0xE/+4/+6/+0x1F/+0x20/+0x21/+0x23` |
| camera/track | `FUN_000736AC` | global track block `0x5774C..0x577FE`, 3-dword vectors | ball `[0x577CA]`, team `[+0x7B2]` | `0x5774C/50/54`, `0x577C0../C8`, `0x577FC/FE`, shadows |
| tracker/decide | `FUN_00072AC4`→`FUN_00072478` | selection globals `0x57A83/A87/A8B/A97`, ring `0x5B440` | team `+0x7A6/+0x7B2`, ball, ring | command ring via `FUN_0008F188`, `[0x575xx]`, `[0x577xx]` shadows |
| selection | `FUN_00088940` | `[0x57A9F]/[0x57A9B]`, `[0x577CE+side*4]` | team blocks, nearest search | `[0x57A9F]`, `FUN_0008A938` codes |
| team | `FUN_0008D8EC` | team block | 3-dword vectors, `[0x57A83]`, phase | `+0x7B2/+0x7BA/+0x7BE`, timer, `FUN_000782D0`/`FUN_0007CA54` |
| player/keeper | `FUN_0007CA54`/`FUN_000782D0` | `record[0]` / `record[1..10]` | timers, type tables, `rec[6]` action ptr | animation/action state |
| ball pair | `FUN_0007D430` | two `[team+0x7B2]` records | `+0x71/+0x73` velocities, `+0x59/5D/61` | `+0x4D/+0x51/+0x55` |

Bounded sub-slices (all static-only unless noted):

| # | slice | anchors | status |
|---|---|---|---|
| S1 | frame chain order in `FUN_0004B100` | `0x4B14A`, `0x4B15E`, `0x4B193`, `0x4B1A6`, `0x4B2A9` | **derived here** |
| S2 | control-state slots | `FUN_00078A54`, `FUN_00078950`, `0x57C64` | derived here (slot map); animation tables open |
| S3 | camera/track integration | `FUN_000736AC`, `FUN_0006D870`, `FUN_00070B94`, `FUN_000709D0`, `FUN_000703E8`, `FUN_0007131C` | mapped here; per-block derivation open |
| S4 | tracker + decision table | `FUN_00072AC4`, `FUN_00072478`, ring `0x5B440` | mapped here; post table open |
| S5 | selection pass | `FUN_00088940` | **derived here** |
| S6 | per-team dispatch | `FUN_0008D8EC` | **derived here** |
| S7 | keeper record | `FUN_000782D0` | mapped; tables/action ptr open |
| S8 | outfield record | `FUN_0007CA54`, `rec[6]`, tables `0x109xx` | mapped; open |
| S9 | ball/possession pairing | `FUN_0007D430`, `FUN_0008DCD4` | position math derived |
| S10 | entity history ring | `FUN_000927E0/0x92864/0x928F0/0x92998`, table flat `0x110F1C` | layout derived here |
| S11 | stat accumulator | `FUN_0008ED40`, `0x5A998` | partial |
| S12 | metric + nearest search | `FUN_0008DC68`, `FUN_0008DE8C` | **ported** |

## 6. Port: `fifa96_entity_update`

`include/fifa96_loader/fifa96_entity_update.h` +
`src/fifa96_loader/fifa96_entity_update.c` (caller-owned data, no globals, no
comments, `-fifa96_err_t` for invalid arguments). Scope: exactly the metric
`FUN_0008DC68` and the nearest search `FUN_0008DE8C`.

| original | port |
|---|---|
| `FUN_0008DC68` word distance/magnitude approximation | `fifa96_entity_distance(dx, dy)` |
| `FUN_0008DE8C` 11×`0xB2` scan; skip index in BX (sign-extended word) | `fifa96_entity_find_nearest(records, count, (uint16_t)skip_index, ...)` |
| record fields `+0x59` X, `+0x61` Y, `+0x98`/`+0x9A` exclusions | `fifa96_entity_candidate { x, y, skip_98, skip_9a }` |
| target word X at `+0`, Y at `+8` | `target_x`/`target_y` arguments |
| 16-bit subtraction then sign extension (`SUB AX` + `MOVSX`/`CWDE`) | `(int16_t)(target_x - rec.x)` |
| unsigned 16-bit best compare (`CMP AX,SI` / `JNC`), first wins | `uint16_t d < best`, strict |
| return NULL when none found, out = `0xFFFF` | return `-1`, `*best_distance = (int16_t)0xFFFF` |
| out pointer / record base non-NULL by contract | `-FIFA96_ERR_INVALID` |

Not ported (globals/objects outside the clean piece): the team blocks, the
history ring, the tracker/selection passes, the per-record state machines,
`FUN_0008ED40`.

Tests (`tests/test_entity_update.c`, suite 55 → **56**): zero/axis
distance; equal-axis branch (`137`, `27`, `6`, `45054`); octagonal branch
boundaries (`100,50`→112 vs `100,51`→118, `100,40`→110, `100,49`→112,
`3,4`/`4,3`→4); signed-branch symmetry; word semantics including the
`NEG`/low-word quirks (`(32767,0)`→32767, `(-32768,0)`→-8192,
`(0x10000,0)`→0, `(0x18000,0)`→-8192); nearest selection by metric value
(`41`), skip index, `skip_98`/`skip_9a` exclusion, equal-distance first-wins,
none-found (`-1`, `0xFFFF`), `count == 0`, target wrap
(`-32768 - 32767` → `1`), and NULL argument errors. ASan+UBSan build clean
(`cc -fsanitize=address,undefined -Iinclude tests/test_entity_update.c
src/fifa96_loader/fifa96_entity_update.c`).

## 7. Errata (quoted)

* FU-58 §6 row `FUN_00072478`: "coordinate/possession-style update on
  `[0x577ca]`, `[0x57a83]`, `[0x57acc]` with the `0x5774c`/`0x57750`
  coordinate offsets" — **refined**: the coordinate offsets are read and the
  function posts command-ring ids by threshold bands, but it performs no
  position integration; the integration is `FUN_000736AC` §4.1 and
  `FUN_0007D430` §4.4.
* FU-62 §5 "calls `FUN_00092040(0)`/`(1)`" — **refined**: the call is
  `FUN_00092040(EAX=0xC8, EDX=0/1)` at `0x88AFB..0x88B11`.
* FU-62 §5 "`FUN_00088940` … updates the current entity pointer `[0x57A9F]`
  from `[0x577CE + team*4]`" — **confirmed**: `0x88AA9..0x88AB3`
  (`MOVSX EAX,DI; MOV EAX,[EAX*4+0x577CE]; MOV [0x57A9F],EAX`), with the
  `FUN_0008DE8C` fallback and the `0xB20` threshold now derived.
* FU-62 §5 "`FUN_00072478` writes the `0x575xx` tracking block and calls
  `FUN_00092040` at `0x7253B/0x7254A` and `0x725B5/0x725C4`" — **confirmed**
  by the full listing (same addresses) and extended with the entry condition
  (§2).
* FU-60 §5 "the `0x577xx`/`0x57Axx` entity/coordinate block is updated inside
  the frame chain, not by a separate callee" — **extended**: the separate
  callees do exist (`FUN_00078A54`, `FUN_000736AC`, `FUN_0008D8EC`,
  `FUN_0007D430`); `FUN_0004B100` only orchestrates them.
* Decompiler: `decompile_function 0x72478` / `0x72AC4` prune reachable
  blocks (listed `ram` warnings), including the `0x72AE5` call itself; the
  listing is authoritative. `decompile_function 0x92998` mis-infers the
  starting ring index (`-0x77`); the disassembly starts at `[0x5B665]`.

## 8. Open legs

* **`[0x57A9F]`/`[0x57A9B]` semantics**: the selection pass resolves the
  first and a backup entity; the consumer of `[0x57A9B]` was not located.
* **`[0x5781E]`/`[0x5781D]`**: only the gates are evidenced (tracker active /
  phase-transition reset); no writer mapping.
* **`[0x57784]`**: sign selects a side and magnitude is compared to `0xB20`;
  the producing coordinate was not located.
* **History ring consumers**: `FUN_00092998`'s callers are five functions
  outside this slice (`0x7131C`, `0x71DF4`, `0x72AC4`, `0x72478`,
  `0x88940`); the ring's gameplay role is not asserted. `[0x5B64D]`, `[0x5B650]`,
  `[0x5B655]`, `[0x5B662]` were only seen at the writer sites.
* **Camera/track block**: the roles of `0x57770` vs `0x5777C` vs
  `0x57788`/`0x57794`, the zoom scalers `[0x577D6]/[0x577D7]`, and the
  `FUN_0006D870`/`FUN_00070B94` helpers are not derived.
* **Tracker internals**: `FUN_00072AC4`'s pruned blocks, the 11×`0x1A`
  slot init at `0x575F1`, `[0x575xx]` fields and the `FUN_0008DC68` pick
  comparisons (`0x73185/0x731A7/0x7323A/0x7325C`) are not decomposed.
* **`FUN_0007CA54`/`FUN_000782D0`**: state tables `0x109xx`, the
  `record[+0x18]` action vtable, `FUN_0007D9A4`, `FUN_0007BF20`,
  `FUN_0006E8E8`, `FUN_00077EAC` not derived.
* **`FUN_0008D824`**, `FUN_0008DCD4`, `FUN_000948AC`, `FUN_000782D0`'s
  event semantics: call sites cited, bodies not derived.
* **Team `+0x7A6`** points to an object read as an entity in
  `FUN_00072478` and whose `+0x7B2` the tracker compares against the
  immediate `0x8FD0E850`; identity open.
* **Direction tables** `0xE1DC/0xE1EC/0xE1F5/0x1064E` and the entity-type
  table flat `0x110F1C` are quoted from listings/one byte read; their full
  ranges are not dumped.
* **`[0x4C110]`/`[0x4C195]`** (written at `0x4B181..0x4B18D`) remain
  unclassified (FU-62 open leg carries over).

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`disassemble_function` 0x88940, 0x72478, 0x736AC, 0x8D8EC, 0x78A54, 0x78950,
0x8DC68, 0x8DE8C, 0x927E0, 0x928F0, 0x92998, 0x4B100, 0x7D430, 0x8ED40;
`disassemble_bytes` 0x8B600 (96 B), 0x72AC4 (128 B);
`decompile_function` 0x72478, 0x72AC4, 0x8D8EC, 0x7CA54, 0x782D0, 0x7D430,
0x8DE8C, 0x8DC68, 0x92998, 0x741B4, 0x651F0, 0x8ED40, 0x4511D;
`read_memory` 0x10F19 (32 B), 0x110F19 (48 B), 0x20F19 (48 B), 0x10E1DC
(48 B), 0x1E1DC (48 B), 0x11064E (32 B);
`get_xrefs_to` 0x88940, 0x72478, 0x72AC4, 0x736AC, 0x78A54, 0x8D8EC,
0x8DC68, 0x7CA54, 0x92998, 0x577CA, 0x5B440, 0x5B665;
`get_function_by_address` 0x72478, 0x8D8EC, 0x7D430, 0x948AC, 0x78A54,
0x736AC, 0x72AC4, 0x92998; `get_current_program_info`.

Analysis-only outside the port: no tool, capture-rig, ISO or
Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_entity_update.h`,
`src/fifa96_loader/fifa96_entity_update.c`,
`tests/test_entity_update.c`, `CMakeLists.txt` (one library/test block).
`make test`: 55/55 before, **56/56 after**; ASan+UBSan `test_entity_update`
clean. `game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.

## Errata (M2 Task 8 / FU-141)

* **§4.2 selection vectors** — the "one of the 3-dword vectors
  0x57788/0x57794/0x57770" choice is derived in /FIFA96.EXE
  (`FUN_0008D8EC 0x8D948..0x8D9A6`): `word[0x1577FA] < word[0x157800]` takes
  0x157788; else `< word[0x157806]` takes 0x157794; else 0x157770 plus the
  `0x1577BE/0x1577C0` lead terms. The interception target is
  `0x10F37C + side*0xC`, its skip index is `[[team+0x7B2]+0x8A]>>24` (the
  `+0x8D` byte), and the side compare is on the two `+0x826` bytes
  (`0x8D9E3`), not the block order.
* **§4.2 team update counter** — `[team+0x82C]` increments and wraps at `0xB`
  (`0x8D8F7..0x8D912`); the team timer `[+0x7CB]/[+0x81E]/[+0x820]` decrements
  by the `[0x157A64]` word with the signed `[+0x81E]>>16` limit
  (`0x8DAF3..0x8DB2B`).
* **§3.2 record `+0x8D`** — the interception skip reads `[rec+0x8A]>>24`,
  which is the `+0x8D` byte, so `+0x8D` doubles as the active flag and the
  skip index (the pool maps it to `active`, FU-141 §1.2/§2.2).
* The pool/chain port is `fifa96_match_entities` (FU-141); FU-67's S1..S6
  map is unchanged otherwise.
