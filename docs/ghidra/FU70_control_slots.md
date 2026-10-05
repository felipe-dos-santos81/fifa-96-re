# FU-70: control slots `0x57C64` and controlled-entity selection

Sub-slices S2 (control-state slots) and S5 (selection pass) of FU-67 §5,
extended from the slot map to the full per-slot update machine, the slot
binding/initialisation path, and the writers of the per-team controlled-entity
pointer `team+0x7B2`. Ports the clean pieces as `fifa96_control`.

Result in one line: **the 4 slots `0x57C64` (stride `0x25`) are per-human-controller
records — bound one-to-one to player records by `FUN_00078824`→`FUN_000785E0`
(mode `0x4C1E0[i]` 0/2 selects team `[0x57ABE]`/`[0x57ABF]`, otherwise the slot
is unbound by byte `+0x22 = 0xFF`), each frame `FUN_00078A54` walks the active
slots and `FUN_00078950(slot, delta)` maps the `FUN_0004511D(player)` state
through `+0x1E` (table `0x11064E` when 0, raw when 1), keeps the
pressed/released/held word state (`+0x04/+0x06/+0x0C/+0x0E/+0x10/+0x12`), a
`0xFA`-capped `+0x23` counter, and three animation bytes `+0x1F/+0x20/+0x21`
from tables `0x10E1DC/0x10E1EC/0x10E1F5`; the bound record points back through
`record+0x20` and the record state machines read those words. The controlled
entity `team+0x7B2` is picked (a) at team reset by `FUN_0008DDE0` = the
eligible record with the smallest unsigned word `+0x6B`, (b) every phase-2
frame by `FUN_0008D8EC` = nearest record (skip 0) to a camera vector chosen by
`[0x577FA]` against `[0x57800]`/`[0x57806]` when `team+0x7B2 == 0` or the
user-side entity `[0x57A83] == 0`, and (c) on ball-event reception by
`FUN_0007A084` (from `FUN_0007A490`) = nearest record (skip from `+0x8D` on
type gates) to `0x57770` plus `[0x577BE]/[0x577C0]` camera offsets, which also
clears the opposing team's `+0x7B2/+0x7B6`; no manual-switch path was found.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-69). All
  instructions quoted below were read back this slice with
  `disassemble_function`/`disassemble_bytes`; the decompiler was not used for
  the quoted bodies (FU-67 errata: `0x72478`/`0x72AC4`/`0x92998` prune or
  mis-infer blocks).
* **Address convention (FU-59/FU-61/FU-67, applied).** Code immediates naming
  tables are object-relative and the loader adds the owning object base. This
  slice resolved the four slot-update tables by `read_memory` at both candidate
  flats: `0xE1DC`/`0x1E1DC`/`0x1064E` are zero/code, while flat `0x10E1DC`
  (animation), `0x10E1EC`, `0x10E1F5` and `0x11064E` (direction remap) hold
  data, so the encoded operands resolve through base `0x100000`.
* FU-67 §3.5's slot map is the starting point; every field and flow below is
  re-derived from the listings, and the refinements are quoted in §6.

## 1. S2 — the 4 control slots `0x57C64`

### 1.1 Per-frame walk `FUN_00078A54`

Sole caller `0x4B15E` in `FUN_0004B100` (step 3 of FU-67 §4). Full body
(16 instructions, `0x78A54..0x78A83`):

```
0x78A56  EBX = 0
0x78A58  CMP byte [EBX+0x57C86],0 / JL 0x78A76   ; slot+0x22 signed < 0 -> skip
0x78A61  EDX = [0x57A62]                          ; frame clock
0x78A6C  SAR EDX,0x10                             ; delta = high word
0x78A67  EAX = 0x57C64 + EBX
0x78A71  CALL 0x78950                             ; update(slot, delta)
0x78A76  EBX += 0x25 / CMP EBX,0x94 / JNZ 0x78A58
```

So exactly four records at `0x57C64 + i*0x25`, each updated only when the
signed byte at `slot+0x22` is non-negative, with the delta carried as a 16-bit
value (`SAR 0x10` of the frame clock, truncated to a byte by the callee).

### 1.2 Slot update `FUN_00078950` (89 instructions, `0x78950..0x78A52`)

Inputs: `EAX = slot`, `DX = delta` (low 16 bits of `EDX`). Decoded flow:

```
player = (int8_t)([slot+0x19] >> 24)                    ; = byte slot+0x1C
state  = FUN_0004511D(player)                           ; FU-61 §5 getter -> byte
[slot+0x12] = state                                     ; raw input word
if ([slot+0x1E] == 0)                                   ; map_select
   mapped = (state & 0xFF0) | table1064E[state & 0xF]
else
   mapped = state
prev   = [slot+0x10]
rising = (mapped ^ prev) & mapped                       ; pressed
falling= (mapped ^ prev) & prev                         ; released source
[slot+4]  = rising
[slot+6]  = 0
[slot+0x10] = mapped
if (falling != 0):
   if ([slot+0xC] != 0):                                ; held mask
      [slot+0xC] &= ~falling
      if ([slot+0xC] == 0) [slot+6] = [slot+0xE]        ; held save
   else if (mapped != 0):
      [slot+0xC] = mapped                               ; held = new state
      [slot+0xE] = prev & 0xFF0                         ; held save = prev buttons
   else:
      [slot+6] = prev & 0xFF0                           ; released = prev buttons
if (mapped != 0):
   if ([slot+0x23] < 0xFA) [slot+0x23] += (uint8_t)delta
else if ([slot+6] == 0):
   [slot+0x23] = 0
dir = mapped & 0xF
a   = tableE1DC[dir]
[slot+0x20] = tableE1EC[a]                              ; anim_b
[slot+0x21] = tableE1F5[a]                              ; anim_c
[slot+0x1F] = a                                         ; anim_a
```

Instruction citations: player index `0x7895D..0x78963` (`MOV EAX,[EAX+0x19]` /
`SAR 0x18`), getter call `0x78965`, raw store `0x7896E`, map branch
`0x78972..0x78993`, edges `0x78995..0x789C4`, hold machine
`0x789C6..0x789EE`, counter `0x789F2..0x78A11`, animation `0x78A15..0x78A48`.

Field layout (all offsets quoted from the listing; stride `0x25`):

| slot offset | width | written by | read by | semantics (evidenced) |
|---|---|---|---|---|
| `+0x00` | dword | `FUN_000785E0 0x78632` | `FUN_00036C70`, `FUN_00093890` | bound player record; `record+0x20` points back to the slot |
| `+0x04` | word | `0x789B0` | `FUN_0007CA54 0x7CB10/0x7CB81` | rising edge of mapped state this frame |
| `+0x06` | word | `0x789B6/0x789EE` | `FUN_0007CA54 0x7CAEE/0x7CB17` | release word: `prev & 0xFF0`, or `+0x0E` when a hold mask empties |
| `+0x0C` | word | `0x789E4/0x789D4` | update machine only | hold mask (set to the new mapped state, cleared by falling bits) |
| `+0x0E` | word | `0x789E8/0x789DA` | update machine only | hold save captured when the mask is (re)armed |
| `+0x10` | word | `0x789BE` | `FUN_0007CA54 0x7CC61` | previous mapped state |
| `+0x12` | word | `0x7896E` | — | raw input state (before the `0x11064E` remap) |
| `+0x1C` | byte | `FUN_000785E0 0x78637` | `FUN_00078950` reads it | player index fed to `FUN_0004511D`; equals the slot index in `FUN_00078824` |
| `+0x1D` | byte | `FUN_000785E0 0x7864B` | — | `team+0x828` ordinal at bind time; `0xFF` when unbound |
| `+0x1E` | byte | `FUN_000785E0 0x7863E` | `FUN_00078950 0x78972` | map selector; `0` = remap low nibble through `0x11064E`, non-zero = raw |
| `+0x1F` | byte | `0x78A48` | `FUN_00036C70 0x36EBB`, `FUN_00093890` | `anim = T1[mapped & 0xF]` |
| `+0x20` | byte | `0x78A39` | `FUN_00036C70` | `T2[anim]` |
| `+0x21` | byte | `0x78A46` | `FUN_00036C70` | `T3[anim]` |
| `+0x22` | byte | `FUN_000785E0 0x78654`, `FUN_00078824 0x7888D`, `0x78863` | `FUN_00078A54` gate, `FUN_00093890` | team side (`0/1`) when bound; `0xFF` when unbound; signed `<0` = inactive |
| `+0x23` | byte | `0x78A05` / `0x78A11` | — | counter: `+= (uint8_t)delta` while mapped non-zero, capped by a pre-check at `0xFA`; reset to 0 only when mapped is zero and nothing was released |
| `+0x24` | — | none observed | none observed | stride pad (next slot starts at `+0x25`) |

The bytes `+0x08..+0x0B` and `+0x14..+0x1B` are never touched by the
functions read this slice (open leg 6.1); the port keeps them as `reserved_08`/
`reserved_14`.

Animation/remap tables (flat, `read_memory`; 16-byte rows):

```
0x11064E: 00 05 0A 00 06 04 02 00 09 01 08 00 00 00 00 00   (map_select==0)
0x10E1DC: 00 01 05 00 03 02 04 00 07 08 06 00 00 00 00 00   (T1)
0x10E1EC: 00 01 01 00 FF FF FF 00 01 00 00 FF FF FF 00 01   (T2)
0x10E1F5: 00 00 FF FF FF 00 01 01 01 ...                     (T3, indexed by T1)
```

The tables are pure lookups; no meaning is asserted beyond the indexing
(animation labels are FU-67 open leg).

### 1.3 Slot binding `FUN_000785E0` and the bind search `FUN_0008DB6C`

`FUN_000785E0(EAX = team block or 0, DX = slot index, BX = map_select)`
(`0x785E0..0x7866D`) computes `EDI = 0x57C64 + index*0x25`
(`0x785F8..0x7860B`) and then:

* if `EAX != 0`: `EAX = FUN_0008DB6C(0x5774C, EAX, skip = -1, flag = 1)`
  (`0x78611..0x78622`); if the result is non-null, `[result+0x20] = EDI`
  (`0x7862F`).
* `[EDI] = result` (`0x78632`); `[EDI+0x1C] = slot index` (`0x78637`);
  `[EDI+0x1E] = BX` (`0x7863E`).
* if `EAX(team) != 0`: `[EDI+0x1D] = [team+0x828]`,
  `[EDI+0x22] = [team+0x826]`, `[team+0x828]++` (`0x78645..0x78657`);
  else `[EDI+0x1D] = [EDI+0x22] = 0xFF` (`0x7865F..0x78663`).

`FUN_0008DB6C(EAX = target, EDX = team, BX = skip index, ECX = flag)`
(`0x8DB6C..0x8DC49`, 78 instructions) is the per-team "pick a free record"
search: it walks the 11 records of stride `0xB2`, skipping

* records already bound to a slot (`[rec+0x20] != 0`, `0x8DB89`),
* the sign-extended skip index (`0x8DB8F..0x8DB98`),
* the record pointer `[team+0x7BF]` (`0x8DB9A`),
* record 0 when `[team+0x829] == 0` (`0x8DBA2..0x8DBAD`),
* exclusions `[rec+0x9A] != 0` or `[rec+0x98] != 0` (`0x8DBAF..0x8DBBF`),

then computes `FUN_0008DC68(target[+0] - rec[+0x59], target[+8] - rec[+0x61])`
(`0x8DBC1..0x8DBDF`), negates it and stores `(index, -distance)` pairs; after
the scan it sorts the pairs with `FUN_000A1860` and returns
`team + first_index*0xB2` (`0x8DBFB..0x8DC19`). With no pair and `ECX != 0` it
falls back to the first record with `[rec+0x20] == 0 && [rec+0x9A] == 0`
(ignoring `+0x98`) (`0x8DC1B..0x8DC41`), else 0. `FUN_000785E0` passes
`ECX = 1`, so the fallback is live. The sort helper is not decomposed (open
leg 6.2); the port does not reproduce this search.

### 1.4 Slot initialisation `FUN_00078824`

Sole callers: `FUN_0003BB1C 0x3BF67` (team/controller setup),
`FUN_00092D8C 0x92D92`, `FUN_0007417C 0x741AE` (side split). Body
(`0x78824..0x7891C`):

* `[0x590CC] = [0x59901] = 0` (`team+0x828` of both teams), and
  `[0x590CD] = [0x59902] = [0x4C1DA]` (`team+0x829`) (`0x78828..0x78847`).
* for each team (`ECX = 0..1`) and record `EAX = 0..0xA` of stride `0xB2`:
  `[team + i*0xB2 + 0x20] = team` for `i = 1..10` (`0x7885C..0x78871`);
  record 0 is not stamped.
* for each slot `ECX = 0..3`: `[ECX*0x25 + 0x57C86] = 0xFF`,
  `[.. +0x57C64] = 0` (`0x7888A..0x78896`), then choose a team from
  `BH = [ECX+0x4C1E0]`: `BH == 0` → team `0x588A4 + [0x57ABE]*0x835`;
  `BH == 2` → `0x588A4 + [0x57ABF]*0x835`; any other non-zero (`1`) →
  team `0` (unbound); `BL = [ECX+0x4C1DC]` is passed as the map selector
  (`0x7889C..0x788F4`); then `FUN_000785E0` binds the slot
  (`0x788F4..0x788F9`).
* `[0x4C32C] = 0; [0x58784] = 1` (`0x78904..0x78911`).

`[0x57ABE]`/`[0x57ABF]` are the side-indexed team selectors set by
`FUN_0007417C 0x7417C`: `[0x57ABE] = ([0x57AC2] ^ [0x57AC3]) & 1`,
`[0x57ABF] = ^1` (`0x7417C..0x74190`), then `FUN_0007412C`, the two team
objects through `FUN_0008C33C`, and the slot reset. So mode 0 binds the team
currently numbered `[0x57ABE]` and mode 2 the other; mode 1 leaves the slot
unbound (`+0x22 = 0xFF`).

`FUN_0003BB1C` fills `0x4C1E0[0..3]` from the 12-byte records at `0x4AEA0`
(byte at record `+0`), skipping records whose first dword is `1`
(`0x3BF26..0x3BF65`), then calls `FUN_00078824` (`0x3BF67`). `0x4C1E0` is also
read by `FUN_0003B708 0x3B7C6` and returned by `FUN_0004B7B8` (bounds-checked
`0..3`, else 0). The map selector `0x4C1DC` has no per-slot writer in the
defined-instruction scope; `FUN_0003749C 0x374B7..0x374C1` sets all four
bytes to `1` with the `FUN_0009E8D0` byte fill, so every initialised slot
takes the raw path of `FUN_00078950` (open leg 6.3).

### 1.5 Readers and the record back-link

* `FUN_00036C70` (231 instructions): builds a display/radar snapshot at
  `0x55AA4`: per record copies the team position triple
  (`0x36D1A..0x36D25`), reads `record+0x7B`, `record+0x28` and
  `record+0x3D`; for each of the four slots whose `[slot]` entity is non-null
  and `[slot+0x22] >= 0` it reads `entity+0x8D` and `[slot+0x1F]` into
  `0x9A88`-indexed tables   (`0x36E8D..0x36ED8`), adding `0xB` when the slot side byte
  (`[EDX+0x57C83] >> 24` = `slot+0x22`) equals `[0x57ABF]`.
* `FUN_00093890(side)` walks the four slots, and for each slot with
  `slot+0x22 == side` and a non-null entity whose `+0x9A == 0`, scans the
  team's records `0xA..1`, calling `FUN_000786A0(slot entity, record)` for
  each record whose `+0x9A == 0` (`0x938BD..0x93910`, exclusion test at
  `0x938E3`, call at `0x938F0`); `FUN_000786A0`'s body is not decomposed.
* The record state machine `FUN_0007CA54` reads its slot through
  `record+0x20`: `+0x06 & 0x20` (`0x7CAEE`), `+0x04 != 0` (`0x7CB10`),
  `+0x04 & 0xFF0` (`0x7CB81`), `+0x10 & 0xC0` (`0x7CC61`). The body of the
  action machine is FU-67 S8 (open).
* `FUN_00078B20` resets two adjacent 4-entry dword arrays (not slots):
  `0x57CF8[i] = -1`, `0x57D70[i] = 0` and clears `0x58674` (0x5B bytes);
  `FUN_00078D5C` frees/clears `0x57D70 + i*4` and sets
  `0x57CF8 + i*4 = -1`. `FUN_00078A54`'s walk ends at `0x94 = 4*0x25` and
  does not touch them.

## 2. S5 — the controlled entity `team+0x7B2`

### 2.1 The per-team fields

| team offset | width | evidenced use |
|---|---|---|
| `+0x7A6` | dword | pointer to the other team block (`FUN_0007A084 0x7A0A0`, `FUN_0008D8EC 0x8D9E3` reads it as record back-pointer) |
| `+0x7B2` | dword | controlled entity pointer (`FUN_0004B100 0x4B2C4`, `FUN_0007D430` args, `FUN_00072AC4`/`FUN_000736AC` reads, writes in §2.2–2.4) |
| `+0x7B6` | dword | second target pointer: zeroed with `+0x7B2` by `FUN_0007A084`/`FUN_0008C33C`; compared against records in `FUN_0007CA54 0x7CCC5` |
| `+0x7BA` | dword | interception target (`FUN_0008D8EC 0x8DA57`), zeroed each pass |
| `+0x7BE` | byte | set when the interception target passes the distance gates (`0x8DAE0`) |
| `+0x7BF` | dword | record pointer skipped by the bind search (`FUN_0008DB6C 0x8DBA0`), cleared by `FUN_0008C33C 0x8C3AA` |
| `+0x7C3`/`+0x7C7` | dword | shadow copies of `+0x7B2` (`FUN_0008C33C 0x8C381..0x8C399`) |
| `+0x828` | byte | slot bind ordinal counter, zeroed/consulted by `FUN_00078824`/`FUN_000785E0` |
| `+0x829` | byte | `[0x4C1DA]` at reset (`0x7883D..0x78847`); gates record 0 in `FUN_0008DB6C 0x8DBA6` (role beyond that not asserted) |

`[0x57A83]` is a separate global: `FUN_0008C33C` sets it to the freshly picked
`team+0x7B2` only when the team side equals `[0x57AAC] >> 24`
(`0x8C393..0x8C3BE`), `FUN_0007A084` zeroes it (`0x7A227`), and
`FUN_0008C974` invalidates it when the pointer's record has `+0x9A != 0`
(`0x8CB58..0x8CB73`). It is read by `FUN_0008D8EC 0x8D93B` (selection gate),
`FUN_00072AC4 0x72B06/0x72B41` (tracker), `FUN_0007CA54 0x7CCE1` and the
`FUN_00091630`/`FUN_00091774` group. Read together: **`[0x57A83]` is the
controlled entity of the user/camera side**, and `team+0x7B2` the controlled
entity of each team.

### 2.2 Ranked pick `FUN_0008DDE0` (team reset and roster change)

`FUN_0008DDE0(EAX = team, EDX = skip index)` (`0x8DDE0..0x8DE26`, 30
instructions): walks the 11 records, skips the sign-extended index and any
record with `+0x9A != 0` or `+0x98 != 0`, and keeps the record whose unsigned
word `+0x6B` is strictly smaller than the running minimum `0xFFFF`
(`0x8DDE8..0x8DE1E`); returns `NULL` when none. First wins ties.

Sole callers (`get_xrefs_to 0x8DDE0`): `FUN_0008C33C 0x8C36C` (team reset:
`[team+0x7B2] = pick(team, -1)`, shadows copied, `[0x57A83]` set on the
`[0x57AAC]` side, then `team+0x7B6/+0x7BF = 0`, `+0x7CB = 0`, state bytes
`+0x82B..+0x82E = 0`, `+0x822 = 0`, `+0x824 = 0xC0`, `FUN_0008C418`,
`+0x7E7 = 0`), and `FUN_0008C974 0x8CB95` (roster change: if `team+0x7B2` is
non-null with `+0x9A != 0`, reselect via `pick(team, -1)`).

### 2.3 Phase-2 per-frame refresh `FUN_0008D8EC`

The selection arm (`0x8D919..0x8D9BD`, quoted in FU-67 §4.2, re-read):

```
0x8D929  if (phase == 2 &&
0x8D932         (team[+0x7B2] == 0 || [0x57A83] == 0)):
0x8D948     counter = [0x577FA]
0x8D94E     <  [0x57800]  -> target = 0x57788
0x8D963     <  [0x57806]  -> target = 0x57794
0x8D978     else             target = 0x57770
                              + ([0x577BE]>>16)*0x20 on X
                              + ([0x577C0]>>16)*0x20 on the +8 axis
0x8D9AA     team[+0x7B2] = FUN_0008DE8C(target, team, skip = 0, out = NULL)
```

The comparisons are signed 16-bit (`CMP AX,... / JGE`). `phase` is
`[0x57A4A] >> 24`. `FUN_0008DE8C` is the 11×`0xB2` nearest search over the
`+0x98/+0x9A` exclusions already ported in FU-67; record 0 is always skipped
here. The same pass computes `team+0x7BA` from `0xF37C + side*0xC` with a
skip index of `[team[+0x7B2]+0x8A] >> 24` (`0x8DA2C..0x8DA57`) — the
interception pick — which is outside this slice's port.

### 2.4 Ball-event setter `FUN_0007A084` (from `FUN_0007A490`)

`FUN_0007A084` (`0x7A084..0x7A456`, 269 instructions) is the dedicated
ball-event setter; the other `team+0x7B2` writers are §2.2/§2.3, the
interception arm of `FUN_0008D8EC` and the per-record state machines
(`0x7DF5B`, `0x7E82D`, `0x7E8EF`, `0x7F20E`, `0x80867`, `0x80876`, `0x844DC`,
`0x84D93`, `0x85C96`, `0x8642B`, `0x8999C`, not derived here). It is called
twice from `FUN_0007A490` (`0x7A8C7`, `0x7A8EF`), the ball-event handler
(callers: `0x85C68`, `0x753FE`, `0x7541C`, `FUN_0007B9C4 0x7BF05`, `0x81417`,
`0x7DF53`, `0x80DC7`). Structure:

* `EBP = [0x58730]` (the ball-event record), team A `= [EBP]`, team B
  `= [[EBP]+0x7A6]` (`0x7A08D..0x7A0A0`).
* Side/type gates on `[0x58740] >> 24`, `[EBP+0x69] >> 16`, `[EBP+0x8D]`, the
  record type `[[EBP+4]][0]` (`0x18E2` arm) build a target triple near
  `0x57770`/`0x69970` plus the `[0x577BE]`/`[0x577C0]` camera offsets
  (`0x7A0D6..0x7A331`).
* `FUN_0008DE8C(target, team A, skip, out = NULL)` where `skip` is
  `sign_extend8([EBP+0x8D])` when `[EBP+0x8E] >> 24 == 1` or
  `[EBP+0x91] ∈ {0x12,0x10,0x11}`, else 0 (`0x7A335..0x7A376`).
* `[0x58734] = result`; `[teamA+0x7B2] = result`; `[teamA+0x7B6] = 0`;
  `[teamB+0x7B2] = 0`; `[teamB+0x7B6] = 0` (`0x7A37B..0x7A3A2`).
* When `[0x58740] >> 24 == 3`, `[teamA+0x7E7] = 0` (`0x7A3AC..0x7A3B9`).
* Records `1..10` of team B get their `+0x81` timer set from the
  `[[rec+4]+0x10] >> 24` table at `[0x57A3C]` and `[rec+0x9D]` + `0xF`
  (`0x7A3C0..0x7A408`); then a pass counter `[0x5872D] = 0x14` on the
  `[0x58724]` match, and a final `FUN_00079D5C` unless
  `[EBP+0x91] ∈ {0x10,0x11,0x1D,0x1E}` (`0x7A40A..0x7A44D`).

`FUN_0007A490` reaches the two call sites when the recorded code byte
`[0x58744]` is negative or the code `[0x58743]` is outside `0..0xE` / fails
the `0x104BB` type table (`0x7A8BE..0x7A8F4`); its full event table is not
decomposed (open leg 6.5).

### 2.5 Selection/announce pass `FUN_00088940` (S5 of FU-67, re-read)

Entry: `FUN_0008AF38` tail (`0x8B623..0x8B643`) runs the pass for
`phase ∈ {2, 0x10}` when `[0x5781D] != 0`. Body (205 instructions): clears
`[0x57A9F]`/`[0x57A9B]` (`0x88955..0x8895B`), calls
`FUN_00092998(1, 4, any)` (history ring, FU-67 S10), and branches on
`|[0x57784]| > 0xB20` and `[0x5781E]`:

* side = `([0x57A49] >> 24 == 1) ? [[0x587D4]][+0x826] : ([0x57784] < 0)`
  (`0x88996..0x889C0`);
* if the ring record's `+5` pointer has `[ptr[0]+0x826] == side`,
  `[0x57A9F] = ptr` (`0x889E0`);
* else `[0x57A9B] = ptr` (`0x88A1C`);
* `[0x57A9F] = [0x577CE + side*4]` (`0x88AA9..0x88AB3`); if null or
  `[+0x9A] != 0`, `[0x57A9F] = FUN_0008DE8C(0x5777C, team(side), skip 0, NULL)`
  (`0x88AC5..0x88AE3`);
* tail `FUN_000651F0(2/3/4)`, `FUN_000974F0(0xBB8)`,
  `FUN_0008A938(code 6, side of [0x57A9F])` (`0x88B1B..0x88B44`).

`FUN_0008A938` is the announce function (`get_xrefs_to` → 27 call sites:
three in `FUN_00088940`, one in `FUN_0008A43C`, the rest across the
record/event machines); under its code-1..4 arms it calls `FUN_0008C974` for
both team blocks (`0x8AABF..0x8AB17`), i.e. the selection pass can trigger the
roster change and through it the `+0x9A`-invalidation/reselection of
`[0x57A83]` and `team+0x7B2` described in §2.2. Its code table and the
`FUN_0009343C` relation are not decomposed (open leg 6.6).

`[0x57A9F]` consumers include `FUN_0004BD38 0x4BD9F`, `0x89E0C`, `0x87586`,
`0x87D16`, `0x6E068/0x6E0DF/0x6E104` and `0x8AD0F/0x8AD69`; none was derived
here (FU-67 open leg).

### 2.6 Consumers of the controlled pointer

* `FUN_0004B100 0x4B2C4`: when phase 2 and both `team+0x7B2` are non-null,
  `FUN_0007D430(team0[+0x7B2], team1[+0x7B2])` predicts the first record's
  position with its `+0x71/+0x73` velocities and pairs it with the second
  (FU-67 §4.4).
* `FUN_000736AC` reads `[0x59056]`/`[0x5988B]` (team0/team1 `+0x7B2`) for the
  ball-follow render pick (`0x73BC8`, `0x73BF6`).
* `FUN_00072AC4` reads `team+0x7B2` for its tracked-entity comparisons
  (`0x72D48`, `0x730E7`, `0x73132`, `0x7315B`, `0x73210`, `0x732BA`).
* `FUN_000782D0 0x78518/0x78526` and `FUN_0007CA54 0x7CCB9`, `FUN_0007C990`,
  `FUN_0007D1D4` compare the current record against `team+0x7B2`.

### 2.7 Switching model

No path from the input edge words (`slot+0x04/+0x06`) or `FUN_0004511D` to a
`team+0x7B2` writer was found; `FUN_0008DDE0` has exactly two callers (reset
and roster change) and `FUN_0007A084` exactly one (the ball-event handler).
The observable model is therefore: **automatic switching** at
reset/roster-change (smallest `+0x6B`), per phase-2 frame when the pointer or
the user-side pointer is missing (nearest to the camera vector), and on
ball-reception events (nearest to the ball target, clearing the opponent).
Whether any input action reaches `FUN_0007A490` directly is not asserted
(open leg 6.7).

## 3. Port: `fifa96_control`

`include/fifa96_loader/fifa96_control.h` +
`src/fifa96_loader/fifa96_control.c` (caller-owned data, no globals, no
comments, `-fifa96_err_t` for invalid arguments). Links
`fifa96_entity_update`.

| original | port |
|---|---|
| slot record `0x57C64`, stride `0x25`; proven fields listed in §1.2 | `fifa96_control_slot` (offsets `_Static_assert`ed; unknown gaps `reserved_08`/`reserved_14`) |
| `FUN_000785E0` field writes (`+0x00 = NULL`, `+0x1C`, `+0x1E`, `+0x1D`, `+0x22`) | `fifa96_control_slot_init` (`entity = -1` substitutes the null pointer; identity fields only, update state untouched as in the original) |
| `FUN_00078950` full machine (`0x78950..0x78A52`) | `fifa96_control_slot_update` (caller supplies the input state — the `FUN_0004511D(slot+0x1C[player])` result — the direction map and the three animation tables; `map` required only when `map_select == 0`) |
| `FUN_0008DDE0` smallest `+0x6B` record | `fifa96_control_pick_ranked` over `fifa96_control_candidate { rank, skip_98, skip_9a }`; returns index or `-1` |
| `FUN_0008D8EC 0x8D929..0x8D9B7` gate + nearest(skip 0) | `fifa96_control_reselect` (calls `fifa96_entity_find_nearest`; returns 1 when it reselected, 0 when the pointer is kept) |
| `FUN_0008D8EC 0x8D948..0x8D976` target bucket by `[0x577FA]` vs `[0x57800]`/`[0x57806]` | `fifa96_control_target_bucket` (0/1/2; the third vector's camera offsets stay caller-side) |
| `FUN_00078824`/`FUN_000785E0` binding search `FUN_0008DB6C` (sort/fallback) | not ported (tie behaviour of `FUN_000A1860` underived) |
| `FUN_0007A084`, `FUN_0008D8EC` interception arm, `FUN_0008C974` roster swap, `FUN_0008A938` | not ported (globals/objects outside the clean piece) |

## 4. Tests (`tests/test_control.c`, suite 58 → 59)

* Layout: `_Static_assert` on every proven offset (`+0x00..+0x23`) and the
  candidate record.
* `fifa96_control_slot_init`: identity fields, update state untouched, NULL.
* `fifa96_control_slot_update`: raw passthrough when `map_select != 0`
  (raw/prev/pressed/animation); direction-nibble remap with high-nibble
  preservation; identity map; press/release edges; the hold machine
  (mask arithmetic, `held_prev` capture, residual-hold suppression, release
  reporting `held_prev`); counter accumulation/reset/release suppression,
  `0xFA` cap and wrap, delta low-byte semantics; animation chain
  `T1 → T2/T3`; NULL slot/tables and the `map_select == 0` NULL-map error
  leaving state untouched.
* `fifa96_control_pick_ranked`: minimum, skip index, `+0x98/+0x9A` exclusions,
  first-wins tie, `0xFFFF` never selectable, NULL, count 0.
* `fifa96_control_reselect`: keep when controlled and ball present; phase
  gate (0/1/0x10); reselect when either is missing; record 0 skipped; none
  eligible (`-1`, `0xFFFF`); count 0; word-wrap target through
  `fifa96_entity_find_nearest`; NULL arguments.
* `fifa96_control_target_bucket`: signed boundaries at `t0`/`t1` and extremes.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_control.c src/fifa96_loader/fifa96_control.c
src/fifa96_loader/fifa96_entity_update.c` runs clean.

## 5. Errata (quoted)

* FU-67 §3.5: "reads `[slot+0x19]>>24` = player index" — **refined**: the
  dword load is at `+0x19` and the arithmetic shift is `0x18`, so the byte is
  `slot+0x1C` (`0x7895D..0x78963`).
* FU-67 §3.5: "tracks pressed/released bit words `+0x10/+0xC/+0xE/+4/+6`" —
  **refined**: `+0x04` pressed, `+0x06` released, `+0x0C` hold mask, `+0x0E`
  hold save, `+0x10` previous mapped state, `+0x12` raw input.
* FU-67 §3.5: "`+0x22 >= 0` is active" — **refined**: `+0x22` is the team
  side byte when bound and `0xFF` when unbound; `FUN_000785E0` writes it from
  `team+0x826` (`0x78654`).
* FU-67 §3.5: animation bytes "from the tables `0xE1DC/0xE1EC/0xE1F5` (and
  `0x1064E` when `+0x1E == 0`)" — **extended**: encoded operands resolve flat
  through base `0x100000` (`0x10E1DC/0x10E1EC/0x10E1F5` data, `0x11064E`
  direction row; candidate flats `0xE1DC`, `0x1E1DC`, `0x1064E` are
  zero/code), and the chain is `+0x1F = T1[dir]`, `+0x20 = T2[T1]`,
  `+0x21 = T3[T1]`.
* FU-67 §4.2: "`if (phase == 2 && ([team+0x7B2]==0 || [0x57A83]==0))` …
  target = one of the 3-dword vectors" — **confirmed** at
  `0x8D929..0x8D9B7`, with the bucket boundaries `[0x57800]`/`[0x57806]`
  quoted exactly.
* FU-67 §3.4/FU-58: the history-ring pass `FUN_00092998` is called from the
  selection pass `0x88966` with `(EAX=1, EDX=4)`; this slice did not extend
  the ring semantics.

## 6. Open legs

1. **Slot gaps** `+0x08..+0x0B` and `+0x14..+0x1B`: never touched by the
   binding, reset, update, snapshot, mark or record-machine sites read this
   slice; role unknown.
2. **Bind search sort** `FUN_000A1860` (and with it the exact tie order of
   `FUN_0008DB6C`) is not decomposed; the port omits the search.
3. **`0x4C1DC` writers**: only `FUN_0003749C`'s four-byte fill to `1` was
   found; a bulk writer would not appear in `search_instructions`, and the
   meaning of a `0` selector (remap through `0x11064E`) is untested in-game.
4. **`team+0x829`/`[0x4C1DA]`** and the record-0 eligibility it gates
   (`0x8DBA6`) are placed mechanically; role not asserted.
5. **`FUN_0007A490`** event table, `0x58744`/`0x58743` codes and the two
   `FUN_0007A084` trigger conditions are only cited.
6. **`FUN_0008A938`** announce codes (its `CX = 1/2/4` arms calling
   `FUN_0008C974`) and `FUN_0009343C` are not decomposed.
7. **Manual switch**: no input-driven `+0x7B2` write found; whether the ball
   events that reach `FUN_0007A490` are directly input-triggered is not
   asserted.
8. **`[0x57A9F]`/`[0x57A9B]` consumers** (`FUN_0004BD38`, `0x89E0C`, the
   `0x87586`/`0x87D16`/`0x6E0xx`/`0x8AD0F/0x8AD69` sites) are not derived
   (FU-67 open leg carried).
9. **`0x57AAC`** (side selector for `[0x57A83]`) writer/identity.
10. **`0x57B90/0x57BA6/0x57BD2/0x4C360`** roster arrays swapped by
    `FUN_0008C974` are only seen at that site.
11. **`FUN_000786A0`** (slot↔record marking from `FUN_00093890`) body.
12. **Direction/animation table meanings** (4-direction codes, frame ids) are
    not asserted.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x78A54, 0x78950, 0x4511D, 0x785E0, 0x78824, 0x36C70,
0x7417C, 0x73E28, 0x3749C, 0x8DB6C, 0x8D8EC, 0x8DDE0, 0x8C33C, 0x8C974,
0x7A084, 0x7A490, 0x78B20, 0x78D5C, 0x93890, 0x9E8D0, 0x88940, 0x4B7B8;
`disassemble_bytes` 0x88D40 (220 B), 0x3BEF0 (120 B), 0x7CAA0 (144 B),
0x7CB70 (32 B), 0x7CC50 (160 B), 0x8AA90 (160 B), 0x8B600 (96 B), 0x72AC4
(128 B); `read_memory` 0x10E1DC, 0xE1DC, 0x1E1DC, 0x11064E, 0x1064E, 0x4C1DC
(16 B, covers the 0x4C1E0 bytes); `get_xrefs_to` 0x57C64, 0x57C86, 0x57A9F, 0x57A9B, 0x57A83,
0x59056, 0x5988B, 0x4C1DC, 0x4C1E0, 0x57ABE, 0x57ABF, 0x88D76, 0x7A084,
0x8C33C, 0x8C974, 0x7417C, 0x73E28, 0x3BB1C, 0x7A490, 0x8DDE0, 0x3749C,
0x78A54, 0x78824, 0x8A938; `search_instructions` operands `57c`, `0x7b2`, `4c1dc`,
`4c1e0`, function `FUN_0007CA54` `0x20]`; `get_function_by_address` 0x78A54,
0x78950, 0x88D8E, 0x7DF5B, 0x7E82D, 0x8999C, 0x8DB6C.

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_control.h`,
`src/fifa96_loader/fifa96_control.c`, `tests/test_control.c`, `CMakeLists.txt`
(one library/test block). `make test`: 58/58 before, **59/59 after**;
ASan+UBSan `test_control` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
