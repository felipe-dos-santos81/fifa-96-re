# FU-73: ball pairing — possession flag, reception and the duel snap

Sub-slice S9 (ball/possession pairing) of FU-67 §5: `FUN_0007D430` and
`FUN_0008DCD4` (FU-67 listed both as "position math derived"), extended to the
ball staging block `0x58730..0x58746`, the reception setter `FUN_0007A084`, the
kick/pass hand-off `FUN_0007B9C4`/`FUN_0007A490`/`FUN_0007AE70`, and the
per-record possession flag `+0x9B`. Ports the clean pieces as
`fifa96_ball_pairing`.

Result in one line: **the ball has no `0xB2` pool record — its state lives in the
match staging block `0x58730..0x58746` (`[0x58730]` = event actor pointer,
`[0x58734]` = receiver pointer, `0x58738/0x5873A/0x5873C` = ball vector,
`0x5873E` = trajectory word, `0x58740` = angle/state, `0x58742..0x58746` =
flags/codes), advanced during a kick by `FUN_0007B9C4` from event rows selected
by `FUN_0007AE70` (tables flat `0x11016E`/`0x110196`/`0x11024A`, stride `0xA`),
staged by `FUN_0007A490` and settled by `FUN_0007A084` (nearest record to the
target vector `0x57770 + camera velocity*0x20`, skip `sign_extend8(actor[+0x8D])`
when the actor's type is 1 or its action is `0x10/0x11/0x12`, which sets the
receiving team's `+0x7B2` to the receiver, zeroes `+0x7B6` and clears both fields
on the opponent); **possession is the per-record flag `record+0x9B`** (set 1 at
take sites `0x74567`/`0x76DAE`, cleared at `0x71D2D`/`0x79A6C`/`0x89903` together
with release action code `0x19`); **`FUN_0007D430` is not a ball pairing** — it
snaps team 0's interceptor output position (`+0x4D/+0x51/+0x55`) to team 1's
position `±0x40` when the interceptor's predicted closing distance
(`+0x73/+0x75` words × `[0x57A64]`, metric `FUN_0008DCD4`) is smaller than the
current distance and `< 0x40` (FU-67 errata).**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-72). All
  instructions quoted below were read back this slice with
  `disassemble_function`/`disassemble_bytes`/`read_memory`; the decompiler was
  not used for the quoted bodies (FU-67/FU-70 errata: `0x72478`/`0x72AC4`/
  `0x92998` prune or mis-infer blocks; the same care applies to the unanalyzed
  action/ball regions `0x745xx`/`0x76Dxx`/`0x898xx`, where only byte windows
  around the cited sites were read).
* **Address convention (FU-59/FU-61/FU-67/FU-70, applied).** Code immediates
  naming tables are object-relative and the loader adds the owning object base.
  This slice read new tables at both candidate flats: `0x1016E`/`0x10196`/
  `0x1024A` are code at the raw flat and hold 10-byte rows at flat
  `0x11016E`/`0x110196`/`0x11024A`, and the action-pointer table `0x106E0` holds
  code addresses at flat `0x1106E0` — all three resolve through base `0x100000`.
  The action switch `JMP dword CS:[EAX*4+0x6AE38]` (`0x7B09F`) points outside
  the image at flat `0x16AE38` (> `max_address` `0x16AA4F`) and its raw bytes
  are fixup placeholders (open leg 7.3).
* Every numeric claim below is quoted from the listings; semantic labels beyond
  what the instructions do are not asserted.

## 1. The ball state block `0x58730..0x58746`

`FUN_0007A028` (`0x7A028..0x7A081`, 20 instructions) is the block clear:

```
0x7A02B  [0x58730]=0; [0x58734]=0
0x7A039  word 0x58738=0; word 0x5873C=0; word 0x5873A=0; word 0x5873E=0; word 0x58740=0
0x7A05C  byte 0x58743=2; byte 0x58742=0x20; byte 0x58744=0; byte 0x58745=0; byte 0x58746=0
```

`FUN_0007A490` (`0x7A490..0x7AE37`, sole writers of the block besides clears)
stages an event (entry: `EAX` = actor record, `EDX` = 6-byte vector, `EBX` =
word, `ECX` = code byte):

```
0x7A4CC  EDI=0x58738; ESI=EDX
0x7A4D3  [0x58730]=EBP(actor)
0x7A4DD  MOVSD+MOVSW: copy EDX[0..5] -> 0x58738/0x5873A/0x5873C
0x7A4E3  word 0x5873E=BX
0x7A4EA  byte 0x58743=CL
0x7A4F0  switch on the caller's stack word 0x9 ... (arms set 0x58744=0x30/0x31 at 0x7A949/0x7A964)
0x7A987  codes 1/3/4/5/6/7 clear [0x58730]/[0x58734] and the vectors
```

Field map (all read/written by the functions cited in this slice):

| address | width | writers | readers | evidenced use |
|---|---|---|---|---|
| `0x58730` | dword | `0x7A4D3`, `0x7B9D8`, clears `0x79ABF`/`0x7A02B`/`0x7A987` | `0x7A08D`, `0x7BF00`, `0x6E3A1` | ball-event **actor** pointer (a record) |
| `0x58734` | dword | `0x7A37F`, `0x7A77B`, clears | `0x7F652`, `0x81727`, `0x85388` (action code) | event **receiver** pointer (result of the pick) |
| `0x58738` | word | `0x7A4DD` (MOVSD), `0x7A188`, `0x7B3C1` (via `FUN_0008DCD4`), `0x7B9EF`, `0x7BD61/73` | camera copy `0x71CC8`, clamp `0x7BD55` | ball **x** (position arm) / vector word (kick arm) |
| `0x5873A` | word | `0x7A4DD`, `0x7A1BE`, `0x7BDC8` | `0x7A14E`, `0x7BD7A` | ball vector middle word |
| `0x5873C` | word | `0x7A4DD`, `0x7A201`, `0x7BE05`, `0x7B3C1` | `0x7A211`, `0x7BEF0` | ball **z** / vector word; `>>16` is the direction arg of `FUN_00071C94` |
| `0x5873E` | word | `0x7A4E3`, `0x7A165` (0x80), `0x7BA17` (0), `0x7B3B7`, `0x7BE1F`, `0x7BE66/86`, `0x7BEC0`, cap `0x7BED5` | `0x7A1C4`, `0x7BC34` | trajectory word (`+= row[+6]` per kick call, capped `0x460`) |
| `0x58740` | word | `0x7A17B` | — | angle word; its dword `0x58740..0x58743` aliases the two flag/code bytes below |
| `0x58742` | byte | `0x7A05C` (0x20), `0x7B9E5`, `0x7BC15` | `0x7BB70` | **flags** (bit 0x20/0x30 tested; sign gate at `0x7BBEB`) |
| `0x58743` | byte | `0x7A05C` (2), `0x7A4EA`, `0x7BCB6` | `0x7A0A4`, `0x7A250`, `0x7BD31`, `0x7BEF0` (via dword `[0x58740]>>24`) | event **code** byte |
| `0x58744` | byte | `0x7A949/0x7A964`, `0x7BCC8` | `0x7A8BE` (FU-70 §2.4 event gate) | event **sub-code** |
| `0x58745` | byte | clears only | — | not written by the paths read |
| `0x58746` | byte | `0x7A2F0`, clears | `0x6E3AE` | event **acknowledged/active** flag (set 1 by the picker) |

Reset value: `[0x58742] = 0x20`, `[0x58743] = 2` (`0x7A05C`), so `0x58730 == 0`
(gated by `FUN_0007A084 0x7A08D`) is the "no event" state.

### 1.1 The tracked-entity pointers (`0x577xx`)

* `[0x577CA]` is the camera/stats **tracked entity**: writers are the block
  reset `FUN_000700F4 0x70258` (0) and `FUN_00071C94 0x71D27` (`[0x577CA] = EBP`,
  `[EBP+0x9B] = 0` at `0x71D2D`); readers include the camera chain
  (`FUN_000736AC 0x73A1E/0x73B5B`) and the possession-stat routine
  `FUN_00089EB0 0x89EB5`.
* `FUN_00071C94` also writes the side table `[0x577CE + side*4] = EBP`
  (`0x71D1D`), with `side = byte[EBP[0]+0x826]` (`0x71D12`), and copies the
  event vector `EDX` into `0x577B8` (`0x71CC8`) before calling
  `FUN_00070544`/`FUN_000700F4` (camera reset toward the event).
* `[0x587CC + side*4]` is a second side-indexed entity table whose selected
  entry is copied to `[0x587D4]` (`FUN_00089DCx 0x89DBB..0x89DCF`,
  `FUN_0008AE44..0x8AE54` = the clock machine `FUN_0008AF38` tail). It is read
  by `FUN_00088940 0x889A4` and `FUN_0004B100 0x4B1E5`, and type-gated in
  `FUN_0006E330` (`0x18DA`/`0x18DE` arms at `0x6E382/0x6E3A1`).
* `[0x577CE + side*4]` and `[0x587CC + side*4]` are therefore the per-side
  current/selected entity pointers, distinct from the ball vector. The exact
  identity of the `0x587CC` entries is an open leg (7.2).

### 1.2 The ball has no pool record; record 0 is the keeper

FU-67 §3.2 said record 0 "follows a different code path" and left the role
open. Re-read this slice: the `0xB2` records are addressed only through the two
team blocks and the task/roster searches; no ball position/velocity record
outside them was found. The ball's mutable state is the `0x587xx` vector above,
while `[0x577CA]` tracks an *actor record* (the `FUN_00071C94` callers pass the
action-record `EBP`; `FUN_000736AC 0x73A2B` checks `[[0x577CA]+4][0] == 0x18D8`,
the record-class id, and `FUN_00071DF4 0x71E1C` reads `[[0x577CA]+0x20]`, the
control-slot back-link). So the record-0 path is **not** the ball; the evidence
that it is the keeper remains FU-67's (10 outfield records `1..0xA` updated by
`FUN_0007CA54`, record 0 by `FUN_000782D0`, record 0 skipped by the field
searches `FUN_0008DE8C`/`FUN_0008DB6C`). The ball slice adds only that the
keeper reads `[0x577CA]` (`FUN_000782D0 0x783E3`) like the outfield machines
(`FUN_0007CA54 0x7CB4B`) — role label still not asserted.

## 2. Ball movement: the kick trajectory arms

No per-frame ball integrator exists in the match chain (`FUN_0004B100` calls
only `FUN_0008D8EC`/`FUN_0007D430`, FU-67 §4). Instead, the ball is advanced by
the kicking record's action machine: `FUN_0007D9A4` installs the action pointer
`record+0x18 = table[code]` from flat `0x1106E0` (`0x7DA77..0x7DA8F`) and the
record machines call it every frame (`FUN_0007CA54` `(*(code*)rec[6])()`,
FU-67 §4.3). The kick actions call `FUN_0007B9C4`
(`0x7B9C4..0x7BF17`; 11 call sites: `0x7F6E1`, `0x81646`, `0x82D95`, `0x84246`,
`0x8435E`, `0x84396`, `0x84445`, `0x84E44`, `0x851C4`, `0x861A7`, `0x862A0`),
which per call:

```
0x7B9CD  clear the local word
0x7B9D8  if (EAX != 0) [0x58730] = EAX        ; stage the actor
0x7B9E5  byte 0x58742 = DL                    ; mode/flags
0x7B9EF  if (EBX != 0) copy EBX[0..5] -> 0x58738 ; else zero the vector
0x7BA1E  if ([actor+0x20] != 0):              ; control slot bound
0x7BA...   target selection:
             distance > 0x420        -> camera-relative wing target ESP = (±0xF0, 0, actor[+0x61]),
                                        offset by FUN_0008DCD4(camera 0x5774C, ESP, 0x58738) (`0x7BAE8..0x7BB46`)
             else slot anim +0x20/+0x21 -> FUN_0007B878 (`0x7BB4B..0x7BBDF`)
             slot counter >= 0x38 and y>0x1E0 conditions pick other offsets
0x7BC80  FUN_0007AE70(actor, [0x58738], [0x5873A], code) -> event row
0x7BCB6  apply row: [0x58743]=row[0], [0x58744]=row[9], clamp [0x58738] to [row[4],row[2]]
0x7BD7A  angle math (0xCD474, 0x795A4) -> 0x5873A / 0x5873C
0x7BE1F  [0x5873E] += row[6]; caps and code-4 adjustments; cap 0x460
0x7BF05  FUN_0007A490(actor, 0x58738, [0x5873C]>>16, byte 0x58743)
```

The user input bit `0x10` extends the range by 1.5× in both
`FUN_0007B9C4 0x7BD14..0x7BD53` (per user side `[0x4C1D4 + side*2]`) and
`FUN_0007B878 0x7B8F9..0x7B93D`; no other input path to the ball was found.

`FUN_0007AE70` (`0x7AE70..0x7B193`, 188 instructions) is the event-row
resolver. It picks a table from the actor's `+0x8B` byte and the distance band
`[0x57750] - actor[+0x5D]` (`0x7AEFD..0x7AFDC`: table `0x11016E` at the head,
`0x110196` when `+0x8D != 0`, `0x11024A` otherwise), indexes a 10-byte row by
`actor[+0x91]` and the band, then dispatches on `row[0]-1` through the switch
`0x7B08E..0x7B09A` (`JMP dword CS:[EAX*4+0x6AE38]`, table not readable
statically). The returned row is read by the callers as
`{byte0, byte1, word+2, word+4, word+6, byte+8, byte+9}` (`FUN_0007B9C4
0x7BCB6..0x7BCC8`, `FUN_0007B878 0x7B8D5..0x7B8E5`).

`FUN_0007B878` (`0x7B878..0x7B9C3`) computes a pass vector from the control
slot's animation bytes `slot[+0x20]`/`slot[+0x21]` (`0x7B8B4..0x7B8AD` call
`FUN_0007AE70` with those as x/y), the slot counter `slot[+0x23]` capped `0x3C`
(`0x7B8BF..0x7B8D5`), row fields word+2/word+4 scaled by `0xB5>>8`
(`0x7B971..0x7B990`) and the same 1.5× input gate. `FUN_0007B194`
(`0x7B194..0x7B556`, 289 instructions) is the arm for event code `0x40`
(`0x7BC34..0x7BC49`): it builds an angle from `slot[+0x1D]`, RNG
(`FUN_00092AC8`), phase and `[0x4C2F6]`, writes a speed word at `0x5873E`
(`0x7B3B7`) and sin/cos components through the table `0x14E04`/`FUN_000795A4`
into the vector words (`0x7B4E8..0x7B54B`).

**Errata (FU-67/FU-71 labels).** The ball vector's words are used as a field
position by the camera/reception arms (`FUN_00071C94 0x71CC8` copies
`0x58738..0x5873D` to the camera vector `0x577B8`; `FUN_0007A084 0x7A207`
passes `0x58738` as the event position) and as a direction/velocity vector by
the kick arms (`FUN_0007B194` writes sin/cos components there). The dual
interpretation is quoted per arm; no single "ball physics" model is asserted.
No gravity, bounce or friction term appears in any ball path read this slice —
the only per-call change to `0x5873E` is the row-provided increment with a cap,
and the only per-call change to `0x58738` is the row clamp.

## 3. Possession and reception

### 3.1 The per-record possession flag `+0x9B`

Byte `record+0x9B` is written 1 by the two take sites and 0 by the three clear
sites; it is read by `FUN_0006E330 0x6E34D` and `0x75B29`:

| site | write | context read this slice |
|---|---|---|
| `0x74567` | `[EDX+0x9B]=1` | take: sets camera `0x5774C/50/54` from `record[+0x59/+0x5D+0x38/+0x61]` plus a table offset `[EAX+0xF331/+0xF339]` (`0x74520..0x7454B`), calls `FUN_000700F4`, then `[0x57A83] = EDX` (`0x74578`) |
| `0x76DAE` | `[EBP+0x9B]=1` | same pattern (`0x76D70..0x76DBF`, `[0x57A83] = EBP`) |
| `0x71D2D` | `[EBP+0x9B]=0` | `FUN_00071C94` when the camera is re-targeted to an event entity |
| `0x79A6C` | `[EBP+0x9B]=BH` (0) | `FUN_0007997C` record reset: the record is teleported to its spawn position and the event staging is cleared when it matches `[0x58730]` (`0x79AB1..0x79B0E`) |
| `0x89903` | `[EBP+0x9B]=0` | release: with `FUN_0007D9A4(EBP, code 0x19, …)` (`0x898F3..0x8990A`) and `FUN_0006E598(EBP, type, 0x26)` (`0x89921`) |

No writer sets the flag on reception itself: `FUN_0007A084` only assigns the
target pointer (3.3); the flag becomes 1 when the selected record actually
reaches the ball via a take site. The take sites also set the user-side
`[0x57A83]` to the record, so possession and user-control assignment are the
same event.

### 3.2 Pairing fields

| field | evidenced use | set by | cleared by |
|---|---|---|---|
| `team+0x7B2` | controlled/target entity (FU-70 §2) | `FUN_0007A084 0x7A384` (receiver), per-frame/nearest arms FU-70 §2.3, take/roster arms | `FUN_0007A084 0x7A398` (opponent), FU-70 §2.2/2.4 |
| `team+0x7B6` | second target pointer (FU-70 §2.1) | — | `FUN_0007A084 0x7A38E/0x7A3A2` (both teams), FU-70 §2.2/2.4 |
| `record+0x9B` | possession flag (3.1) | take sites | reset/re-target/release sites |
| `[0x58730]`/`[0x58734]` | event actor / event receiver | `0x7A4D3`/`0x7A37F` | block clears |
| `[0x57A83]` | user-side controlled entity (FU-70 §2.1) | take sites `0x74578`/`0x76DBF`, `FUN_0008C33C 0x8C3BE`, `FUN_00073E28 0x73E6C` | `FUN_0007A084 0x7A227`, `FUN_0008C974 0x8CB73` |

`FUN_0007D430` sets no persistent field: it rewrites the interceptor's output
position for one frame (3.4). Pairing is therefore ephemeral — there is no
"paired-with" pointer anywhere in the block.

### 3.3 Reception `FUN_0007A084` (from `FUN_0007A490`)

`FUN_0007A084` (`0x7A084..0x7A456`, 269 instructions) runs when a ball event is
active; FU-70 §2.4 derived its existence and the two call sites `0x7A8C7`/
`0x7A8EF` (confirmed: `get_function_xrefs 0x7A084` -> exactly those two). Read
back this slice:

```
0x7A08D  EBP = [0x58730]                    ; actor
0x7A093  teamA = [EBP]; teamB = [teamA+0x7A6]
0x7A0D6  if ([[EBP+4]][0] == 0x18E2):       ; special class arm
0x7A0EF     target = (±0x70 or ±0x90 by side+RNG, 0, ±0xB10 by side)   (`0x7A0FB..0x7A13F`)
0x7A149     FUN_000795B4(EBP+0x59, target, 0x58738)                    ; event position
0x7A165     word 0x5873E = 0x80; angle math 0xCD474/0x795A4 ->
           0x58740 (angle), 0x58738 (+= 0x5A0), 0x5873A, 0x5873C     (`0x7A165..0x7A201`)
0x7A207  FUN_00071C94(EBP, 0x58738, [0x5873C]>>16, 0)                  ; camera re-target
0x7A227  [0x57A83] = 0
0x7A234  action-code arms -> FUN_000974DC with code 0x14 / 9..0xA / 0xB..0xD / 0x10 / 0x12..0x13
         (`0x7A246..0x7A2E1`)
0x7A2FF  target = copy of 0x57770; target.x += [0x577BE]>>16 * 0x20;
         target.z += [0x577C0]>>16 * 0x20                              ; camera-velocity lead
0x7A335  skip = (actor[+0x8E]>>24 == 1 || actor[+0x91] ∈ {0x12,0x10,0x11})
                  ? sign_extend8(actor[+0x8D]) : 0                     (`0x7A335..0x7A376`)
0x7A376  EAX = FUN_0008DE8C(target, teamA, skip, NULL)
0x7A37F  [0x58734] = EAX; [teamA+0x7B2] = EAX
0x7A38E  [teamA+0x7B6] = 0; [teamB+0x7B2] = 0; [teamB+0x7B6] = 0
0x7A3AC  if (code == 3) [teamA+0x7E7] = 0
0x7A3C0  for records 1..0xA of teamB: [rec+0x81] = table([rec+0x4][+0x10]>>24, [rec+0x9D]) + 0xF
0x7A40A  if (EBP == [0x58724]) [0x5872D] = 0x14
0x7A441  FUN_00079D5C(EBP, 0x58738) unless action ∈ {0x10,0x11,0x1D,0x1E}
```

So the reception rule is: **the receiving team (the actor's owning block) gets
the nearest eligible record to the camera-led target vector; the other team's
controlled/second pointers are cleared; the actor itself is normally excluded by
the skip index.** This is the pairing transition "ball event -> claimant". Note
that when the special skip rule does not apply the passed skip is `0`, which
`FUN_0008DE8C` still applies literally (`i == 0` is skipped), so record 0 never
receives by default — the port keeps that behaviour (test
`test_receive_nearest_and_skip_zero`).

### 3.4 The duel snap `FUN_0007D430` and `FUN_0008DCD4`

`FUN_0008DCD4` (`0x8DCD4..0x8DD5B`, 61 instructions), re-read exactly:

```
in: EAX = first vector (x word at +0, z word at +8), EDX = second vector,
    EBX = out { word0 distance, word+2 dx, word+4 dz }
0x8DCE3  dx = second.x - first.x (16-bit word)
0x8DCED  dz = second.z - first.z (16-bit word)
0x8DCEF  out.dx = dx; out.dz = dz
0x8DCF7  dx = |dx| (TEST DX / NEG EDX 32-bit)
0x8DCFE  dz = |dz|
0x8DD05  if (dx >= dz) ... else ... (signed 16-bit compares)
result = M + 3/8·m when m > M/2, M + 1/4·m otherwise (M = max, m = min),
         with the 32-bit addends and `CWDE` quirks of the listing
0x8DD55  out.distance = AX
```

For int16 word deltas this is branch-for-branch the same metric as FU-67's
`FUN_0008DC68`: both take the 32-bit `NEG` magnitude (so `dx = -0x8000` gives
`|dx| = 0x8000`), both compare sign-extended 16-bit magnitudes, and the only
difference in the listing — the `CWDE` before the case-B accumulation — is a
no-op because `|m| >> 2` fits in 15 bits. The port therefore delegates the
distance to `fifa96_entity_distance` and adds only the signed `dx`/`dz` words
(verified by the boundary tests in §6).

`FUN_0007D430` (`0x7D430..0x7D4F2`, 70 instructions) — called once per drained
frame from `FUN_0004B100 0x4B2DA` for phase 2 with both `team+0x7B2` non-null
(the call site loads `EAX=[team0+0x7B2]`, `EDX=[team1+0x7B2]`):

```
0x7D44C  pred.x = A[+0x59] + (A[+0x71]>>16 = word A[+0x73]) * (int16)[0x57A64]
0x7D466  pred.z = A[+0x61] + (A[+0x73]>>16 = word A[+0x75]) * (int16)[0x57A64]
0x7D47D  out1 = FUN_0008DCD4(B+0x59, A+0x59)         ; current offset B->A
0x7D48D  out2 = FUN_0008DCD4(B+0x59, pred)           ; closing offset B->pred
0x7D496  if (out2.distance >= out1.distance) return
0x7D4A1  if (out2.distance >= 0x40) return
0x7D4AD  A[+0x4D/+0x51/+0x55] = B[+0x59/+0x5D/+0x61]
0x7D4B6  A[+0x4D] += (out2.dx < 0) ? -0x40 : +0x40
0x7D4D0  A[+0x55] += (out2.dz < 0) ? -0x40 : +0x40
```

The predicted position is only used for the comparison; the copied position is
B's *actual* triple, offset 0x40 further along the direction the interceptor
would have travelled past B. So this is a **duel/tackle snap** between the two
controlled entities, not ball possession (FU-67 errata 1). The threshold 0x40
is compared as a signed word (`0x7D4A8`), and the distance compares are signed
16-bit (`CMP AX, … / JGE`, `0x7D49A`).

### 3.5 Release action

The only release site read this slice is `0x898F3..0x89921`: it clears
`record+0x9B` and calls `FUN_0007D9A4(record, code 0x19, 0, 0)`, i.e. the
record enters action `0x19` (table flat `0x1106E0` index 0x19) and the event
`FUN_0006E598(record, [record+0x8B]>>24, 0x26)` is posted. The surrounding
branch conditions were not reconstructed (the region is not a defined function;
open leg 7.5).

## 4. Pass/shoot hand-off structure

| stage | entry | evidence |
|---|---|---|
| action selection | `FUN_0007D9A4` sets `record+0x18 = table[code]` (flat `0x1106E0`), `+0x91 = code` | `0x7DA63..0x7DA8F`; called from both record machines (FU-67 §4.3) and the release site (3.5) |
| kick action (input-driven bit) | `FUN_0007B9C4` from 11 action-function sites; user input bit `0x10` scales the range 1.5× | §2, `0x7BD14..0x7BD53`, `0x7B8F9..0x7B93D` |
| target selection | distance > 0x420 -> camera-relative wing (`(±0xF0, 0, actor[+0x61])` offset through `FUN_0008DCD4`); else control-slot animation bytes `+0x20/+0x21` through `FUN_0007B878`/`FUN_0007AE70` | §2 |
| event row | `FUN_0007AE70` tables flat `0x11016E`/`0x110196`/`0x11024A` (stride `0xA`), switch `CS:0x6AE38` | §2 |
| staging | `FUN_0007A490` writes `[0x58730]`, vector, `0x5873E`, `0x58743` | §1 |
| settle | `FUN_0007A084` picks the receiver and assigns `team+0x7B2`, clears the opponent | §3.3 |

Intended-target identity is not stored: the short pass derives the direction
from the kicker's control-slot animation bytes, the long pass uses fixed wing
coordinates, and the reception then re-resolves "nearest to the event target"
among the receiving team. An explicit intended-receiver pointer was not found
(`[0x58734]` is written only by the *result* of the pick).

## 5. Port: `fifa96_ball_pairing`

`include/fifa96_loader/fifa96_ball_pairing.h` +
`src/fifa96_loader/fifa96_ball_pairing.c` (caller-owned data, no globals, no
comments, `-fifa96_err_t` for invalid arguments). Links `fifa96_entity_update`.

| original | port |
|---|---|
| `FUN_0008DCD4` signed offset + word metric | `fifa96_ball_pair_offset(from, to, out)` (`out.distance`/`dx`/`dz`; distance via `fifa96_entity_distance`, proven equivalent) |
| `FUN_0007D430` predicted-closing snap | `fifa96_ball_pair_decide(interceptor, opponent, delta, out_position)` — returns 1 when it rewrote the position, 0 otherwise |
| `FUN_0007A084 0x7A335..0x7A376` skip rule + `FUN_0008DE8C` nearest | `fifa96_ball_pair_receive(actor, candidates, count, target_x, target_y, &receiver)` — returns 1 found / 0 none, `-1` out |
| `FUN_0007A084 0x7A37B..0x7A3A2` assign/clear | `fifa96_ball_pair_assign(targets, team, receiver)` (receiving team target+second, opponent both 0) |
| `record+0x9B` take sites `0x74567`/`0x76DAE` | `fifa96_ball_pair_possess(actor)` (flag only; camera/`[0x57A83]` side effects out of scope) |
| release site `0x89903` | `fifa96_ball_pair_release(actor)` (flag only; action `0x19` out of scope) |

Not ported: the `0x587xx` staging block, `FUN_0007B9C4`/`FUN_0007A490`/
`FUN_0007AE70`/`FUN_0007B878`/`FUN_0007B194` (globals, tables, RNG, action
machinery), the camera re-target and stats paths.

## 6. Tests (`tests/test_ball_pairing.c`, suite 61 -> 62)

* Layout: `_Static_assert` on the actor/vector/offset fields.
* `fifa96_ball_pair_offset`: zero/axis; the metric branches
  (`(50,100)`->112, `(51,100)`->118, equal `(100,100)`->137, `(100,50)`->112);
  signed delta storage; negative deltas; the word semantics (`dx=-0x8000` ->
  `-8192`, `32767 - -32768` -> `1`, matching `fifa96_entity_distance`); NULL
  arguments.
* `fifa96_ball_pair_decide`: no pair when the prediction does not close; no pair
  at `distance == 0x40`; pair at `0x3F`; the `±0x40` offset signs for both
  axes; the output position is left untouched when unpaired; NULL arguments.
* `fifa96_ball_pair_receive`: nearest pick; `skip_98`/`skip_9a` exclusion; skip
  from `actor.flag` when `kind == 1` or `action ∈ {0x10,0x11,0x12}`; no skip
  otherwise; none found (`0`, `-1`); NULL arguments.
* `fifa96_ball_pair_assign`: both teams, second-slot zeroing, invalid team,
  NULL.
* `fifa96_ball_pair_possess`/`release`: flag transitions, NULL.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_ball_pairing.c src/fifa96_loader/fifa96_ball_pairing.c
src/fifa96_loader/fifa96_entity_update.c` runs clean.

## 7. Errata (quoted)

* FU-67 §4.4 title "`FUN_0007D430` — ball/possession pairing" and §5 row
  "ball pair": **refined** — the function pairs the two teams' controlled
  entities (`team+0x7B2`) and rewrites the interceptor's output position; it
  never reads the ball state block. §3.4's "ball/possession pairing" label in
  the frame-chain table is correspondingly a duel snap.
* FU-67 §3.2 "velocity pair: `+0x71 + ([+0x71]>>16)*delta`" **refined**: the
  velocity words are `+0x73` (x) and `+0x75` (z); `+0x71` is the speed metric
  (`FUN_0008E244 0x8E4BC..0x8E4CD` computes it, `0x8E4D8..0x8E504` integrate
  `+0x59 += delta*word[+0x73]` and `+0x61 += delta*word[+0x75]`). The dword
  reads in `FUN_0007D430` are the same aliasing (`[ECX+0x71]>>16`).
* FU-67 §4.4 "adds ±0x40 to `+0x4D` and `+0x55` from the signs of `out2` words
  +2 and +4" — **confirmed** exactly, and extended with the strict thresholds
  (`out2 < out1`, `out2 < 0x40`, signed word compares at `0x7D49A`/`0x7D4A8`).
* FU-67 §2 "[0x57A8B] = Ball at the tracked entity": `[0x57A8B]` is assigned
  from `[0x57A87]`/`[0x57A83]` (`0x72C62`/`0x730DB`), i.e. the tracker's
  current *controlled* entity, not a ball record.
* FU-70 §2.4 (ball-event setter) **confirmed** (call sites `0x7A8C7`/`0x7A8EF`
  and the body flow) and extended with the skip rule and the target vector.
* FU-58 §7 "ball physics as a separate slice": the ball's in-play advance is
  the kick action's `FUN_0007B9C4` row-clamp/arc, not a separate per-frame
  integrator; no physics solver was found this slice.

## 8. Open legs

1. **Ball vector dual use**: `0x58738/0x5873A/0x5873C` are a field position in
   the camera/reception arms and an angle/velocity triple in the kick arms; the
   per-event-code convention is not fully mapped.
2. **`0x587CC`/`0x587D0`** side table entries: only the selection into
   `[0x587D4]` and its type gates (`0x18DA`/`0x18DE` in `FUN_0006E330`) are
   evidenced; the entity identity per side is open.
3. **Switch `CS:0x6AE38`** (`FUN_0007AE70 0x7B09F`): runtime-fixed table
   outside the file image; the per-action row semantics are not resolved.
4. **`record+0x9B` consumers** (`0x6E34D`, `0x75B29`) are cited by address
   only; the flag's effect on the action/presentation layer is not derived.
5. **Release branch conditions** (`0x898xx`): the surrounding action machine is
   not a defined function; only the flag clear + action `0x19` are quoted.
6. **Take routines** (`0x745xx`, `0x76Dxx`): only the byte windows around the
   flag write, camera copy and `[0x57A83]` write were read; the guard conditions
   that decide when a take is allowed are not derived.
7. **`0x18E2` class**: checked only in `FUN_0007A084 0x7A0D9`; the other class
   ids (`0x18D8` record, `0x18DA`/`0x18DE` in `FUN_00072AC4`/`FUN_00092548`)
   are cited, their object identity is not asserted.
8. **`FUN_0007B194`/`FUN_0007B9C4` target algebra**: the conditions selecting
   wing vs slot-anim vs camera-relative targets are mapped at block level, not
   decomposed per branch (RNG streams, `[0x4C2F6]` difficulty, `0x577BE`/`0xC0`
   camera leads).
9. **`FUN_0007A084` action-code arms** 9..0x14 and `FUN_00079D5C` are cited by
   address only.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x7D430, 0x8DCD4, 0x8D824, 0x7A028, 0x7A084, 0x7A490
(bytes), 0x7B9C4, 0x7AE70, 0x7B878, 0x7B194, 0x795B4, 0x8E244, 0x71C94,
0x700F4, 0x7997C, 0x6D920, 0x6E330, 0x92548, 0x4B100, 0x71DF4 (head),
0x89EB0, 0x7D9A4; `disassemble_bytes` 0x72B20, 0x72C40, 0x730C0, 0x74520,
0x76D70, 0x898B0, 0x7A1E0, 0x7A480, 0x7A940, 0x8ADD0, 0x89D90, 0x8AE20,
0x773F0, 0x7F4C0, 0x82A40, 0x87460; `read_memory` 0x1106E0, 0x11016E,
0x110196, 0x11024A, 0x6AE38, 0x16AE38 (out of image), 0x69970, 0x58730;
`get_xrefs_to` 0x577CA, 0x57A8B, 0x57A83, 0x58730, 0x587CC, 0x587D4, 0x69970;
`get_function_xrefs` 0x7A084, 0x7A490, 0x7B9C4, 0x7B878, 0x7B194, 0x71C94;
`search_instructions` operands `+ 0x71]`, `+ 0x73]`, `+ 0x9b]`, `0x577ca`,
`0x18d8`, `0x18da`, `0x18de`, `0x18e2`, `0x69970`, `0x587CC`;
`get_function_by_address` 0x7D430, 0x8D824, 0x8E244, 0x7A490, 0x7B9C4, 0x7B194,
0x7B878.

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_ball_pairing.h`,
`src/fifa96_loader/fifa96_ball_pairing.c`, `tests/test_ball_pairing.c`,
`CMakeLists.txt` (one library/test block). `make test`: 61/61 before, **62/62
after**; ASan+UBSan `test_ball_pairing` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
