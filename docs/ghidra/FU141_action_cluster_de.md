# FU-141: M2 cluster D/E — entity/ball pool, update chain, drains, RNG

Task 8 of the M2 child plan (`docs/superpowers/plans/2026-10-07-fifa96-m2-match.md`),
scoped by the C6 controller ruling: the absent entity/ball pool was the G2
wiring critical path, so this slice derives it first and drains the write-only
stand-ins (FU-137 OL-1 / FU-138 OL-16 / FU-139 OL-16 / FU-140 OL-37), then wires
the derived FU-67 update chain into the match frame body. The plan's cluster
D/E portions that still need unported arms (outfield decide/chase row wiring,
the FU-61-adjacent input-handler support item) are recorded as numbered legs
below rather than guessed; the derived RNG, which is fully self-contained, is
ported and tested.

Result in one line: **the two `0x835`-stride team blocks (`0x1588A4`/`0x1590D9`,
11 × `0xB2` records), the team fields, the ball record (`0x15880C`) and the
ball staging block are now a new engine pool
(`include/fifa96_engine/fifa96_match_entities.h` + `.c`); the FU-67 frame order
(`FUN_0004B100`: control slots -> camera -> clock -> team 0 -> team 1 ->
pairing) and `FUN_0008D8EC`'s exact counter/selection/interception/timer/record
walk are wired into `fifa96_match_run_frame`, with the FU-137 action dispatch
bound to pool records; the FU-137 §2 installer `FUN_0007D9A4`, the
`FUN_0007876C`+`FUN_00078670` slot merge and row 1E's placement/actor requests
are consumed by the pool (install/ran, helper_request, controlled,
place_valid); the derived RNG (`FUN_00092AA0` seed from flat `0x102D70`
"ArCaDe" + `seed<<25`, `FUN_00092AC8` six-word step) is ported with a pinned
sequence. Rows 00 and 1E keep their ported class, now bounded to the pool; no
other dispatch row becomes fully derivable (every one still needs an unported
arm — §6).**

## Method

* Authoritative program **`/FIFA96.EXE`** (explicit in every call). Ghidra
  **read-only**: no renames, comments, labels, function creation, scripts or
  project saves.
* Tool calls made this slice:
  * `disassemble_bytes` / `disassemble_function` — `FUN_0004B100`
    `0x4B140..0x4B2E0`; `FUN_0008D8EC` `0x8D8EC..0x8DB6F`; `FUN_0007D9A4`
    `0x7D9A4..0x7DAB4`; `FUN_0007DAB4` `0x7DAB4..0x7DB10`; row 00
    `0x7DB10..0x7DBB0`; `FUN_0007CA54` `0x7CA54..0x7CAC0`; `FUN_0007876C`
    `0x7876C..0x78822`; `FUN_00078670` `0x78670..0x7869E`; `FUN_000700F4`
    `0x700F4..0x702F4`; `FUN_0008DC68` `0x8DC68..0x8DCD2`; row 1E
    `0x7550C..0x755D3`; row 08 head `0x81068..0x81188`; `FUN_00092AA0`
    `0x92AA0..0x92AC6`; `FUN_00092AC8` `0x92AC8..0x92BC1`; `FUN_000493A0`
    `0x493A0..0x495AF`;
  * `read_memory` — seed table `0x102D70` (16 B);
  * `search_instructions` — `112e88` (10 sites), `110f44`/`110f4` (12 sites),
    `15880c` (10 sites);
  * `get_xrefs_to` — `0x92AA0` (1), `0x700F4` (29), `0x110F44` (4).
* **Address mapping.** In `/FIFA96.EXE` the loader's object-4 fixups are
  already applied, so data immediates render at their resolved flat addresses
  (`[0x001588a4]` team 0, `[0x0015b650]` ring gate, matching FU-137..FU-140's
  native convention; FU-67/FU-75 wrote the pre-loader `0x588A4` form). Code
  addresses are identical to `/fifa96_le.bin` (FU-130 §6).
* Baseline at task start: `make check` = **98/98** (FU-140 commit `e04af4f`);
  after this task **100/100** (new `test_engine_match_entities`, new
  `test_rng`), ASan/UBSan clean; M1 golden and pinned render hashes unchanged.

## 1. The entity/ball pool (native layout)

### 1.1 Team blocks and stride

`FUN_0004B100` (`disassemble_bytes 0x4B140`):

```
0x4B14A  MOV EBX,0x1588a4        ; team 0 base
0x4B169  MOV EDX,0x1590d9        ; team 1 base
0x4B2A9  MOV EAX,EBX; CALL 0x8d8ec
0x4B2B0  MOV EAX,EDX; CALL 0x8d8ec
0x4B2C4  MOV EAX,[EBX+0x7b2]; MOV ECX,[EDX+0x7b2]; ... CALL 0x7d430
```

`0x1590D9 - 0x1588A4 = 0x835`; `FUN_0008D8EC` walks records with
`LEA EDX,[EBP+0xB2]` / `ADD EDX,0xB2` while `EBX < 0xB`
(`0x8DB3A..0x8DB5F`) — 11 records at `0xB2`, record 0 on the keeper path
(`0x8DB2E CALL 0x782d0`), records 1..10 through `0x8DB4D CALL 0x7ca54` when
`[rec+0x9A]==0`. This re-verifies FU-67 §3.1/§3.2 in the authoritative
program.

### 1.2 Record map re-verified in /FIFA96.EXE

`struct fifa96_match_entity` carries the record fields the derived chain and
the wired rows read; each offset was seen in a first-hand window this slice
(or is carried from FU-67 §3.2 / FU-137 §2 / FU-140 §4 where noted):

| field | offset | first-hand site |
|---|---|---|
| team back-pointer | `+0x00` | `FUN_0007876C 0x78787 MOV EBX,[EAX]`; `0x8D9E3` |
| control slot | `+0x20` | `0x7877D`; row 00 `0x7DB3D`; row 1E `0x75530` |
| slot direction high bytes | `+0x1D/+0x1E` -> `+0x20/+0x21` | row 00 `0x7DB53`/`0x7DB56` (`MOV EBX,[ECX+0x1E]; SAR 0x18`) |
| output triple | `+0x4D/+0x51/+0x55` | row 08 `0x8111E..0x81145`; FU-67 §3.2 |
| position triple | `+0x59/+0x5D/+0x61` | row 1E `0x75571..0x7559B`; `0x8DED2`/`0x8DEC8` (FU-67) |
| velocity pair | `+0x71/+0x73` | FU-67 §4.4 (`0x7D44C`); ported `fifa96_ball_pair_decide` |
| lane dword | `+0x69` | row 08 `0x8114C` (`SAR 0x10; CMP 0x50`); `0x8DAAE` |
| lane words | `+0x6B/+0x6D` | FU-67 §3.2 |
| timer limit | `+0x7F` | `0x7CA66` (`MOV EDX,[EAX+0x7F]; SAR 0x10`) |
| timer word | `+0x81` | `0x7CA5C..0x7CA83` |
| action timer | `+0x89` | `0x7DA7E` (installer zero); row 00 `0x7DB16..0x7DB37` |
| actor type | `+0x8B>>24` | row 1E `0x7555C`; installer `0x7DA0F` |
| active flag | `+0x8D` | `0x7D9E5` context (`0x7DA26`); `0x8DAE6` |
| type | `+0x8E>>24` | FU-75 §1.2; row 1E uses `+0x8B` for the offset table |
| stage byte | `+0x8F>>24` | row 1E `0x7551A`/`0x75544`; row 1F FU-140 |
| action code | `+0x91` | installer `0x7D9CB`/`0x7DA63`; row 00 `0x7DBA3` |
| stage latch | `+0x92` | `FUN_0007DAB4 0x7DABA = 0xFF`; installer `0x7DA95` |
| timer byte | `+0x93` | `0x7CA8A..0x7CAB4` |
| exclusions | `+0x98`, `+0x9A` | installer `0x7D9BE`/`0x7D9DC` |
| ball flag | `+0x9B` | row 1E `0x75553` (return when set) |
| ran byte | `+0x9E` | row 00 `0x7DB1C`; installer `0x7DA88` |
| carrier bit | `+0x9F` bit 0 | installer `0x7DA42..0x7DA5A` |
| timer pair | `+0x79/+0x7B` | installer `0x7DA9B`/`0x7DA9F`; `FUN_000782D0` entry (FU-67 §4.3) |

The port maps the derived `has_slot` flag to `+0x20 != 0`, `active` to
`+0x8D`, `type` to `+0x8E>>24`, `actor_type` to `+0x8B>>24`, `stage` to
`+0x8F>>24`, `ran` to `+0x9E`, `carrier` to `+0x9F` bit 0, `lane` to `+0x69`,
and `timer7f/timer81/timer89/timer93/timer7b/timer79` to the timers above.

### 1.3 Team fields

`struct fifa96_match_team` carries `side +0x826` (`0x8D9E5`), the slot-pool
byte `+0x828` (`0x7878B MOV AL,[EBX+0x828]`), `search_gate +0x829`, mode
`+0x82A`, the update counter `+0x82C` (`0x8D8F7..0x8D912`), the timer cluster
`+0x7CB/+0x81E/+0x820` (`0x8DAF3..0x8DB27`), `flag7be +0x7BE` (`0x8DAE0`),
and the selection pointers `target +0x7B2`, `second +0x7B6`, `chosen +0x7BF`,
`intercept +0x7BA` (FU-67 §3.1/§4.2, FU-75 §4). The pointer fields are stored
as encoded entity ids (`team*11 + index`) because the native stores real
pointers; `-1` is the native NULL.

### 1.4 Ball record and staging block

`search_instructions 15880c` re-verifies the ball record at flat `0x15880C`
(reset `FUN_000886D4 0x8873A/0x88746` writes `-0x720`/`0x1560`; `0x84BDD`
integrates x), matching FU-120 §2. The staging block `0x158730` is the
FU-139 §3.1 `fifa96_ball_pair_state` (ported). The pool holds both; the
derived update chain writes neither (the pairing writes the two controlled
records — §2.3).

## 2. The update chain (FUN_0004B100 order and FUN_0008D8EC body)

### 2.1 Frame order

`FUN_0004B100` calls, in order: control slots `0x4B15E CALL 0x78a54`, pace
countdown `0x4B163`, the entity stack reset `0x4B181`, camera/track
`0x4B193 CALL 0x736ac`, clock `0x4B1A6 CALL 0x8af38`, then `0x4B2A9`/`0x4B2B0`
`FUN_0008D8EC(team0)`/`(team1)` and `0x4B2DA CALL 0x7d430` (pairing) when
phase == 2 and both `[team+0x7B2]` are non-null. The engine frame body already
runs the FU-70 slot and FU-71 camera updates per granted frame; FU-141 adds
the pool update immediately after them (`fifa96_match_run_frame`), so the
slot/camera -> entities relative order matches the native chain (the engine's
clock advance happens in `fifa96_match_state_tick` before the grant loop —
a Task-13 divergence, not changed here).

### 2.2 `FUN_0008D8EC` per-team body (full listing read this slice)

```
0x8D8F7  AH=[team+0x82C]; AH++; [team+0x82C]=AH; if (byte >= 0xB) =0
0x8D919  phase=[0x157A4A]>>24; [0x1586D0]=0
0x8D929  if (phase != 2) goto intercept-clear
0x8D932  if ([team+0x7B2] != 0 && [0x157A83] != 0) goto intercept
0x8D948  AX=word[0x1577FA]
         if (AX < word[0x157800]) target = vector 0x157788
         else if (AX < word[0x157806]) target = vector 0x157794
         else { target = vector 0x157770;
                target.x += ([0x1577BE]>>16)<<5; target.z += ([0x1577C0]>>16)<<5 }
0x8D9AA  [team+0x7B2] = FUN_0008DE8C(target, team, skip 0, out)
0x8D9BD  [team+0x7BE]=0
         if (phase != 2 || [0x157A83]==0) goto intercept-clear
0x8D9E3  if (byte[team+0x826] != byte[[0x157A83]+0x826]) goto clear
0x8D9F7  if (|[0x157754]| <= 0x480) goto clear
0x8DA0D  if (([0x157754]<0) != byte[team+0x826]) goto clear
0x8DA2C  skip = [ [team+0x7B2] +0x8A ] >> 24            ; the +0x8D byte
0x8DA4D  target = 0x10F37C + side*0xC
0x8DA52  [team+0x7BA] = FUN_0008DE8C(target, team, skip, out)
0x8DA5D  if ([nearest+0x20] != 0) { [team+0x7BA]=0; ... }
         else FUN_0008D824([0x157A83], nearest+0x4D)
0x8DA7F  FUN_000795B4(nearest+0x59, nearest+0x4D, &stack); band flag +0x7BE
0x8DAE9  [team+0x7BA]=0                                 ; clear
0x8DAF3  if ([team+0x7CB]!=0) { limit=[team+0x81E]>>16 (signed);
                                d=zero-extended word [0x157A64];
                                if (limit > d) [team+0x820]-=d else {=0; [team+0x7CB]=0} }
0x8DB2E  FUN_000782D0(team, 1)                          ; record 0 (keeper)
0x8DB3A  for (i=1; i<0xB; i++) { rec=team+i*0xB2;
                                 if ([rec+0x9A]==0) FUN_0007CA54(rec) }
```

The port implements the counter, both selections (vector choice and the
interception conditions/skip/slot rejection), the timer and the record walk;
the `FUN_0008D824` bind call and the `FUN_000795B4` band flag were ported by
M2 arms-and-wiring Task 14 (OL-41 closed; `fifa96_entity_intercept_bind` /
`_band`, FU-142 Appendix K), so `flag7be` is now fed. The bind's z comes from
the **actor's** `+0x61` (`0x8D82A`, EAX = `[0x157A83]`), and the band output
is a stack scratch (`0x8DA88 LEA EBX,[ESP+0xC]`) that is not stored back to
the record (fix round 1; the slot-rejected NULL-record band read is OL-71).
The per-record machines themselves
(`FUN_0007CA54` input-row dispatch/no-edge arm/forced decision,
`FUN_000782D0`) remain FU-137 OL-3/OL-4 (OL-44); the pool update runs the
derived machine subset: the timer pair decay and the keeper `+0x79 -> +0x7B`
copy, then the record's installed action through the engine dispatch callback.

### 2.3 Record timers and keeper copy (first-hand)

`FUN_0007CA54 0x7CA54..0x7CAC0`:

```
if (word[rec+0x81] != 0) {
  limit = [rec+0x7F] SAR 16;
  if (limit > (zero-extended word)[0x157A64]) [rec+0x81] -= delta;
  else [rec+0x81] = 0;
}
if (byte[rec+0x93] != 0) {
  if (byte93 > (zero-extended word)[0x157A64]) byte93 -= (uint8)delta;
  else byte93 = 0;
}
```

The keeper machine copies `[rec+0x79]` to `[rec+0x7B]` at entry (FU-67 §4.3);
the pool update does the same for record 0 only. The FU-137 §4.1 wording
"decremented by `[0x157A62]`" is refined to the `[0x157A64]` delta word
(Errata).

### 2.4 Ball/possession pairing

The chain tail calls `FUN_0007D430(team0[+0x7B2], team1[+0x7B2])` only for
phase 2 and only when both pointers are non-null (`0x4B2B7..0x4B2DA`). The
ported `fifa96_ball_pair_decide` (FU-139, tested) models the body (FU-67
§4.4); the pool applies its output to team 0's target output triple
`+0x4D/+0x51/+0x55`.

## 3. Drains (the write-only stand-ins now consumed)

### 3.1 Installer `FUN_0007D9A4` (full 74-instruction listing read)

The port `fifa96_match_entities_install(entity, phase, code, staged)`:

| native | site | port |
|---|---|---|
| occupied `+0x9A` -> no-op | `0x7D9BE` | `skip_9a != 0 -> 0` |
| same code -> no-op | `0x7D9CB..0x7D9D6` | `code == entity->code -> 0` |
| animation arm: `+0x98` set and code != 0x0C and phase not 2/0xA/0xF -> `FUN_0006E598(rec,0x0F,type8,0)` + `[+0x98]=0` | `0x7D9DC..0x7DA1F` | `skip_98 = 0`; the animation call is unported (OL-42) |
| inactive code 3 -> 0x19 | `0x7DA26..0x7DA3B` | `active==0 && code==3 -> 0x19` |
| `+0x9F` bit 0 set for 5/0x21 else cleared | `0x7DA42..0x7DA5A` | `carrier` bit 0 |
| `[+0x91] = code` | `0x7DA63` | `code` |
| `[+0x89]=0; [+0x9E]=0; [+0x92]=staged; [+0x7B]=[+0x79]; [+0x18]=table[code]` | `0x7DA7C..0x7DA9F` | timers/ran/stage latch; the handler slot is the code-indexed FU-137 table |
| invoke when ECX != 0 | `0x7DAA3..0x7DAAA` | the pool chain always stages (row 00 passes ECX=0); the invoke arm is OL-43 |

### 3.2 Slot merge `FUN_0007876C` + `FUN_00078670` (full listings read)

`fifa96_match_entities_merge_slot(pool, team, record)`:

```
if ([rec+0x20] != 0) return;                   0x7877D
team = [rec]; if (byte[team+0x828]==0) return; 0x78787..0x78794
for i = 0..10:                                  0x7879a..0x787f2
  if ([cand+0x20]==0) continue;
  d = FUN_0008DC68(rec[+0x59]-cand[+0x59], rec[+0x61]-cand[+0x61]);
  if (i == 0 || (int16)d > (int16)best) { best = i; best_d = d; }
if (none) return;                               0x787f4..0x787f9
requester[+0x20] = donor slot; donor[+0x20] = 0; slot[0] = requester;
FUN_00078670(slot);                             0x78809..0x78814
```

The ranked pick is "the first slot-holding candidate unconditionally, later
slot-holders only by a strictly greater signed-word distance": the native
counter `INC ECX` at `0x787E4` is bypassed for no-slot candidates
(`0x787A7 JZ 0x787E5`), so `TEST CX,CX` at `0x787C6` means "no slot-holder
seen yet", not "record 0" (`FUN_0008DC68` preserves ECX, verified this
slice). The pool moves the `has_slot` flags and records the merge; the frame
body moves the FU-70 slot binding and calls `fifa96_control_slot_merge_reset`
(`FUN_00078670`: zeroes slot `+4,+6,+8,+0xA,+0xC,+0x14,+0x16` only, pinned in
`test_control`).

### 3.3 Row 1E requests: helper, placement, actor

Row 1E's body (`0x7550C..0x755D3`, re-read) calls `FUN_0007876C` at `0x75536`
(the `helper_request`), then on the claimed path writes
`[0x15774C]=[rec+0x59]+offset_x<<4`, `[0x157754]=[rec+0x61]+offset_z<<4`,
`[0x157750]=[rec+0x5D]+0x38` (`0x75565..0x75593`) and pushes
`(x, y, z, 1)` for `CALL 0x700F4` (`0x7559E..0x755C7`), then sets
`[0x157A83]=rec` (`0x755CF`). The pool consumes `place_valid` into a take-once
request; the frame body applies it through the existing FU-71
`fifa96_camera_init`, which FU-71 §11 maps to `FUN_000700F4`'s pure field
reset (position into the four vectors; velocities/timers/anchors zeroed).
`controlled` becomes the pool actor `[0x157A83]`, which drives the selection
conditions above. This corrects FU-140 §4's call shape `FUN_000700F4(rec,
place, 1)`: `RET 0x10` and the four pushes show the signature
`FUN_000700F4(place_x, place_y, place_z, flag)` — there is no record argument
(Errata).

### 3.4 Init seeds `FUN_0007DAB4` (first-hand)

```
[rec+0x92] = 0xFF; [rec+0x89] = 0;
if ([rec+0x20] != 0) FUN_00078B00(slot);
if (phase == 2 && [rec+0x8D] != 0) FUN_0007C990(rec);
else FUN_0007D9A4(rec, code 0, BL 0, ECX 0);
```

The pool init uses the derived reset subset: `stage92 = 0xFF`, `timer89 = 0`,
`code = 0` (the reset installs action 0); the phase-2 forced-decision arm is
the unported machine path (OL-44).

## 4. The engine port

* New module `include/fifa96_engine/fifa96_match_entities.h` +
  `src/fifa96_engine/fifa96_match_entities.c`, compiled into `fifa96_engine`;
  it links `fifa96_entity_update` (nearest/metric) and `fifa96_ball_pairing`
  (pairing).
* `struct fifa96_match_run` gains `struct fifa96_match_entities entities`
  (init/begin seed the reset state, teardown releases).
* `fifa96_match_run_frame` builds the frame inputs from the FU-71 camera
  triple (all three selection vectors; zero bucket timers/leads — the
  camera/track block is unported, OL-40) and runs
  `fifa96_match_entities_update` with `match_run_dispatch_entity`, which
  stages each pool record into `mr->record`, dispatches its action code
  through the FU-137 table, and repacks the requests.
* Drains: the pool installer consumes `install`/`ran`; the pool merge
  consumes `helper_request`; `controlled` sets the pool actor; `place_valid`
  is taken by the frame body and applied with `fifa96_camera_init`; a
  successful merge rebinds `mr->slot.entity` and clears the slot through
  `fifa96_control_slot_merge_reset`.
* Tests: `tests/test_engine_match_entities.c` (init/release, record
  round-trips, installer semantics, nearest/ranked selection, chain order
  counter fixture, request drains, pairing, side-swap interception);
  `tests/test_engine_match_frame.c` gains the live chain pin;
  `tests/test_control.c` pins `fifa96_control_slot_merge_reset`;
  `tests/test_engine_match_handlers.c` pins the row-1E `place_valid` request
  and now uses a pointer-based fixture (the previous by-value begun-run
  fixture was a latent use-after-return that the larger pool exposed under
  ASan).

## 5. The derived RNG

Fully self-contained, evidence read this slice:

* **State**: six words at flat `0x110F44/48/4C/50/54/58` (`search_instructions
  110f4`).
* **Seed** `FUN_00092AA0` (`0x92AA0..0x92AC6`, sole call `0x493F2` in the
  match init `FUN_000493A0`): for each of six bytes at flat `0x102D70`
  (`read_memory` = `41 72 43 61 44 65`, i.e. "ArCaDe") the word is
  `(int8)byte + (seed << 25)` (`SHL ECX,0x19`).
* **Step** `FUN_00092AC8` (`0x92AC8..0x92BC1`): a six-word additive generator
  with a carry chain (each `t = low(prev) + carry + w[k]`; `w[5] += 1`; when
  a word wraps to zero the next lower word increments, down to `w[0]`), the
  return value is the new `w[0]` word.
* Ported as `fifa96_rng_seed`/`fifa96_rng_step`, with the pinned seed-0
  sequence `{512, 1829, 4927, 11195, 22605, 41818, 6755, 52849}` and the
  carry-chain cases in `tests/test_rng.c`.

`FUN_000CB2A4` (getter of `[0x112E88]`, writers at `0x9F5F5`/`0x9F601`) is a
different global and is not this generator.

## 6. Dispatch rows before / after

The FU-137/FU-138 wiring rule is unchanged: a row is wired only when its
**full record-visible body** is covered by tested C functions. FU-141 makes
the pool the binding for rows 00 and 1E and drains their requests end to end;
it does **not** make any other row fully derivable, because each still needs
an unported arm:

| code | class | what still blocks wiring |
|---|---|---|
| 00 | ported (unchanged) | nothing pool-side; install/ran now drained |
| 1E | ported (unchanged) | helper/place/actor now drained; arm bodies OL-37 |
| 05/06/07/0F | not ported | carrier/pursuit/kick machines (FU-139 OL-29..OL-31) |
| 18/21/23 | not ported | resolution/claim/target arms (FU-139 OL-32) |
| 19..1D/1F | not ported | keeper arm bodies (FU-140 OL-33..OL-36) |
| 01/02/03/04/08/09/0A..17/20/22/24/25 | not ported | their arm/support bodies (FU-138/FU-137 legs) |

Action-table dispatch counts are unchanged: **2 × `FIFA96_OK` (00, 1E),
43 × `-FIFA96_ERR_UNSUPPORTED`, 1 × `-FIFA96_ERR_NOT_FOUND` (phase 0x16)**.
What changed is the binding: the frame body now walks the 22 pool records and
dispatches each record's code, so the two wired rows run live over the pool
and their request fields are consumed rather than left write-only.

## 7. Open legs

* **OL-38 — cluster-D outfield decide/chase row wiring.** The pure helpers
  (`fifa96_outfield_dispatch_code`, `fifa96_outfield_forced_action` =
  `FUN_0007C990`, `fifa96_outfield_chase_action`) were derived and tested in
  FU-75; row 08 (`0x81068`) and row 04 (`0x7E7C8`) still call the unported
  machine helpers (`FUN_0007DAB4`, `FUN_00079B58`, `FUN_00079C50`) and the
  per-type gate table `0x110680`, so neither is wired (row 08 head re-read
  this slice). A follow-up task should port the machine subset
  (input-row dispatch `0x1109D0`/`0x1109E4`, no-edge arm, forced decision,
  chase gate) and then wire rows 04/08. **Status (Task 14): the machine
  subset is ported as `fifa96_outfield_input_row` + `fifa96_outfield_chase_gate`
  (+ the 26-byte `0x110680` gate; FU-142 Appendix K), with the first-hand
  pressed/released either-or correction; the rows 04/08 full bodies
  (`0x7E7C8..0x7F141` ~649 insns, `0x81068..0x814AF` ~231 insns) are split to
  OL-70 and dispatch `-FIFA96_ERR_UNSUPPORTED` until then.**
* **OL-39 — FU-61-adjacent input-handler support item.** The per-frame slot
  update (`FUN_00078950` via `FUN_00078A54`), the direction getter
  `FUN_0004511D`, and the handler-output packaging (FU-61 §5) are not all
  ported; the existing `fifa96_control_slot_update` covers the slot machine,
  but the input-handler method tables/`FUN_0004511D` support item remains a
  follow-up.
* **OL-40 — camera/track block.** The selection vectors/timers/leads
  (`0x157770/88/94`, `0x1577FA/800/806`, `0x1577BE/C0`) and the intercept
  targets (`0x10F37C/0x10F388`) are produced by `FUN_000736AC`/`FUN_00072AC4`
  and executable data; the engine passes its FU-71 camera triple and zero
  timers, so the selection always takes the third vector (FU-67 S3).
* **OL-41 — interception tail — closed (Task 14).** `FUN_0008D824` (bind) and
  `FUN_000795B4` (band) are ported as `fifa96_entity_intercept_bind` /
  `fifa96_entity_intercept_band` (FU-142 Appendix K) and consumed by
  `fifa96_match_entities_team_update`; `team+0x7BE` is fed and cleared per
  frame. The slot-rejected NULL-record band read is OL-71.
* **OL-42 — installer/placement side effects.** The installer's
  `FUN_0006E598` animation call and row 1E's per-type offset tables
  (`0x10F334`/`0x10F33C`, executable object-4 data) are unported;
  `place_offset_*` is caller-supplied. `FUN_000700F4`'s non-position state
  (preset tables `0x1104AB`, `0x14C2FA/FE`, `FUN_00070074`, `FUN_0006D870`)
  is FU-71's open leg; the pool consumes only the derived field reset.
* **OL-43 — installer invoke arm.** The native `ECX != 0` invoke-now arm
  (`0x7DAA3..0x7DAAA`) is not modelled; every derived chain install stages
  without invoking (row 00 passes ECX=0).
* **OL-44 — record machines.** `FUN_0007CA54`'s input-row dispatch/no-edge
  arm/forced decision and `FUN_000782D0`'s keeper body remain FU-137 OL-3/
  OL-4; the pool supplies their record surface.
* **OL-45 — place coalescing and ball updaters.** Multiple same-frame claims
  coalesce to the last placement (the native calls `FUN_000700F4` per claim);
  the FU-120 ball-record writers and the FU-139 staging tail (OL-26) have no
  derived update chain in the pool yet.
* Carried prior legs: FU-137 OL-1/OL-2/OL-5..OL-15, FU-138 OL-17..OL-25,
  FU-139 OL-26..OL-32, FU-140 OL-33..OL-37 — the pool closes the
  record-surface half of OL-1/OL-16/OL-37; their arm halves remain.

## 8. Errata (prior docs)

* **FU-140 §4 (row 1E call shape)** — `FUN_000700F4(rec, place, 1)` is wrong:
  the bytes at `0x7559E..0x755C7` push `1`, then the three dwords
  `0x157754`, `0x157750`, `0x15774C`, and `0x700F4` ends with `RET 0x10`
  (4 dword arguments); the function reads the triple from `[ESP+0x1c]`
  (`0x7014E MOVSD x3`) and never dereferences a record. Corrected to
  `FUN_000700F4(place_x, place_y, place_z, flag)`; the port's `place_*` sink
  is unchanged, only the consumption call shape is.
* **FU-137 §4.1 (record timer source)** — "decremented by `[0x157A62]`/its
  low byte" is refined: `FUN_0007CA54 0x7CA6E/0x7CA9A` loads the word
  `[0x157A64]` (the frame delta) and compares it against `[rec+0x7F]>>16`
  (word timer) or the raw byte (byte timer). Ported exactly.
* **FU-67 §4.2 (selection vectors)** — the "one of the 3-dword vectors"
  choice is now derived: `word[0x1577FA] < word[0x157800]` -> `0x157788`;
  else `< word[0x157806]` -> `0x157794`; else `0x157770` plus the
  `0x1577BE/0x1577C0` lead terms (`0x8D948..0x8D9A6`). Also derived: the
  interception target is `0x10F37C + side*0xC` with skip
  `[[team+0x7B2]+0x8A]>>24` — the *team target's* `+0x8D` byte
  (`0x8DA2C MOV EBX,[EBP+0x7B2]; MOV EBX,[EBX+0x8A]; SAR EBX,0x18`), not the
  `[0x157A83]` actor's — and the side compare is on the two `+0x826` bytes
  (`0x8D9E3`), not the block order.
* **FU-120 §2** — confirmed at flat `0x15880C` in `/FIFA96.EXE`
  (`search_instructions 15880c`); the `-0x720`/`0x1560` reset writers are
  `FUN_000886D4 0x8873A/0x88746`.

## 9. Concerns

* The pool update tolerates `-FIFA96_ERR_UNSUPPORTED` from the action
  callback (an unported record code) and propagates any other error; the
  frame body therefore never fails on an unported record, matching the
  seam's explicit open-leg discipline.
* Pointer fields are encoded `team*11+index` with `-1` = NULL; side is a
  separate `+0x826` byte so side-swapped matches compare correctly
  (`test_update_intercept_select` pins it).
* The engine models one FU-70 control slot (the pre-existing FU-70 port
  scope), so the native four-slot pool is not represented; the merge moves
  the single binding and the `+0x828` producer is an open leg (OL-43/OL-45).
* `fifa96_match_entities_take_place` coalesces; the native applies each
  `FUN_000700F4` call immediately.

## Provenance

Ghidra MCP, read-only, program `/FIFA96.EXE` (explicit): `disassemble_bytes`
`0x4B140..0x4B2E0`, `0x8D8EC..0x8DB6F`, `0x7D9A4..0x7DAB4`,
`0x7DAB4..0x7DB10`, `0x7DB10..0x7DBB0`, `0x7CA54..0x7CAC0`,
`0x7876C..0x78822`, `0x78670..0x7869E`, `0x700F4..0x702F4`,
`0x8DC68..0x8DCD2`, `0x7550C..0x755D3`, `0x81068..0x81188`,
`0x92AA0..0x92AC6`, `0x92AC8..0x92BC1`, `0x493A0..0x495AF`;
`disassemble_function` 0x7876C, 0x78670, 0x700F4, 0x8D8EC, 0x92AC8, 0x493A0,
0x7CA54, 0x7D9A4, 0x7DAB4, 0x4B100; `decompile_function` 0x7876C, 0x92AA0,
0x8D8EC; `read_memory` 0x102D70 (16 B); `search_instructions` `112e88`,
`110f44`, `110f4`, `15880c`; `get_xrefs_to` 0x92AA0, 0x700F4, 0x110F44.
No writes: no rename/comment/label/function/script/project save.
`/fifa96_le.bin` and `/fifa96.exe` untouched.

Repo: `make check` = **100/100** after the task (ASan/UBSan engine tests
included); M1 golden and pinned render hashes unchanged (no render path
touched). Port write set: `include/fifa96_engine/fifa96_match_entities.h`,
`src/fifa96_engine/fifa96_match_entities.c` (new),
`include/fifa96_loader/fifa96_rng.h`, `src/fifa96_loader/fifa96_rng.c` (new),
`include/fifa96_engine/fifa96_match_run.h`,
`src/fifa96_engine/fifa96_match_run.c`,
`src/fifa96_engine/fifa96_match_handlers.c`,
`include/fifa96_loader/fifa96_control.h`,
`src/fifa96_loader/fifa96_control.c`, `tests/test_engine_match_entities.c`
(new), `tests/test_rng.c` (new), `tests/test_control.c`,
`tests/test_engine_match_frame.c`, `tests/test_engine_match_handlers.c`,
`CMakeLists.txt`. `game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not
staged.
