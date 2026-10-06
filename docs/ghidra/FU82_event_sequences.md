# FU-82: the event/sequence action family (the 10 classified codes)

Follow-on to FU-76 (action table/class census) and FU-81 (stage family): fully
derive the ten event/sequence-classified action codes
`01,02,0B,0D,14,16,20,22,24,25` — their inputs, state written, stage/arm
progression, event emission and hand-off — and port the proven shared mechanics
into `fifa96_action_handlers` as `fifa96_action_sequence_*`.

Result in one line: **the ten codes are per-record stage machines that emit
animation ids through `FUN_0006E598` (not an event dispatcher: it selects the
9-byte animation row `[0x57588] + id*9`, installs it at `[rec+0x28]` and copies
the row byte bits to `+0x43/+0x44/+0x45/+0x46` — `+0x44` is the row's bit 0 and
is the `FUN_0007DAB4` reset gate every family body checks), signal the FU-63
event queue through `FUN_0008F188` (code `0x20` sets the `0x5B374` ring flag),
and progress through `gate(phase) → +0x89 += [0x57A64] → switch(+0x92) arm
table (inline compares or `CS:` dwords, target = stored+0x10000) → arm advances
`+0x92` with `+0x89 = 0`, installs a successor via `CALL 0x7D9A4`, hands off to
a phase handler (`CALL 0x6E1D0`), or resets when `+0x44`. Codes `14` and `25`
share the RNG-scattered five-point script block `0x586D8` and the event-id
initializer `FUN_0008776C`; code `0B` installs code `0C` on the nearest record;
code `16` is installed by the `0x89FA4` timeline driver (slot `0x46`) and code
`24` by the `0x8B688` driver (slot `0x4D`).**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4
  linear image, link addresses). `disassemble_function`/`disassemble_bytes`
  are the citation source where the listing defines complete flow; where the
  listing mis-decodes or prunes blocks (the FU-76/FU-81 tooling errata), the
  bodies were dumped to host files with `run_script_inline` and decoded
  linearly with `ndisasm -b32` (FU-79's method) — no decompiler output is
  quoted anywhere.
* **Address mapping (FU-76/FU-81, restated).** Code/function addresses equal
  true link addresses; a **data immediate** `A` is flat `A+0x100000` (so
  `[0x57A4A]` is storage at flat `0x157A4A`, `[0x58866]` at `0x158866`); stored
  code pointers and inline `CS:` tables are object-1 relative and resolve
  through `+0x10000` (e.g. `CS:` operand `0x6dbb0` → true table `0x7DBB0`).
* **Tooling errata.** `disassemble_function 0x84630` (code `16`) loses sync at
  `0x84665` and emits garbage from the mis-decoded overlap; the linear dump
  decodes cleanly. The same method was used for `0x8784C` (14), `0x880CC`
  (25), `0x8539C` (22), `0x86510` (24), `0x81908` (0B) and `0x84EEC` (20).
* Every numeric claim is quoted from the listings; unproven items are open
  legs (no guessed labels).

## 1. The family and its partition

FU-76 §2 classifies the rows with per-slot evidence but does not state the
bucket partition. The FU-76 working report records the 45-slot partition
counts: locomotion/decision **1**, placement/chase **7**, kick/pass **2**,
possession/carrier **2**, keeper **7**, **event/sequence 10**,
tackle/duel/interception **2**, stage machines/transitions **8**,
stub/unclassified **6**. The only assignment of the §2 class labels that makes
the counts sum to 45 (`1+7+2+2+7+10+2+8+6`) is:

| bucket | codes | §2 class labels |
|---|---|---|
| event/sequence (10) | `01,02,0B,0D,14,16,20,22,24,25` | camera/event transition; restart/set-piece; event/duel with ring post; event transition; scripted/celebration sequence; sequence with vector pop; transition with input table; phase-2 short event; stats/commentary sequence; stats sequence |
| stage machines/transitions (8) | `09,0C,0E,10,11,12,13,17` | stage machine ×2, stage transition, phase transition ×3, gated transition, camera reset transition |
| stub/unclassified (6) | `15,28,29,2A,2B,2C` | transition (body mis-decoded), unclassified ×2, chosen-record action, `RET` stub, unanalyzed prologue |

This also reads the FU-81 discrepancy: FU-81's 13-code "stage/transition
superset" = the 8 stage codes + the 4 event/sequence codes whose §2 class
contains the word *transition* (`01,02,0D,20`) + the stub `15`. The partition
here is a **reconstruction** from the report counts and §2 class names, not a
quote from FU-76 §2 (open leg 1).

All ten are entered as `CALL [rec+0x18]` with only `EAX = rec` (FU-74 §2).
Per-code stage counts: `01`:4, `02`:3 (inline), `0B`:3, `0D`:4, `14`:3 +
5 script arms, `16`:3, `20`:7, `22`:3, `24`:3, `25`:7.

## 2. Shared machinery

### 2.1 `FUN_0006E598` — the animation selector ("EVENT")

FU-76 §3.2 labels `CALL 0x6E598` as `EVENT`; the body (`0x6E598..0x6E713`,
112 insns) proves it is the animation-row selector. Inputs `EAX = rec`,
`EDX = animation id`, `ECX = type8`, `EBX = param` (0 in all ten bodies):

```
0x6E59C  if ([rec+0x8D] == 0) { side vs ball-z sign gate; if the current animation
             byte [[rec+0x28]] is set and not in {0x62..0x65} call 0x92AC8 }
0x6E608  if (phase [0x57A4A]>>24 == 2 || anim_id != 0) goto 0x6E68E
0x6E68E  if ((int16)anim_id < 0 || (int16)anim_id >= 0x6F) anim_id = 0
0x6E6A9  EDX = [0x57588] + anim_id*9          ; 0x6F rows of 9 bytes (runtime pointer slot)
0x6E6B1  [rec+0x28] = EDX                     ; animation row pointer
0x6E6B4  AL = byte [EDX+2]
0x6E6BC  [rec+0x45] = AL & 4
0x6E6C4  [rec+0x43] = AL & 2
0x6E6CC  [rec+0x44] = AL & 1                  ; <- the reset/terminal gate
0x6E6D8  [rec+0x46] = 1
0x6E6E0  [rec+0x3F] = (AL & 0x10) ? 2 : (AL & 0x20) ? -2 : 0
0x6E706  CALL 0x6E490                          ; frame/timer helper
0x6E70B  word [rec+0x32] = 0
```

So `+0x44` — the byte every family body tests before `CALL 0x7DAB4` — is
**bit 0 of the selected animation row's byte `+2`**. The rows themselves are
pointed to by the runtime slot `[0x57588]` (flat `0x157588`; zero statically —
open leg 4), indexed `id*9` over the valid id range `0..0x6E`.

### 2.2 `FUN_0008F188` — the FU-63 event-queue command ring

`0x8F188..0x8F2C3` takes `EAX = id`, `EDX = record pointer (param)`,
`EBX = code` and, when `[0x4C32A] == 0 && [0x4C312] != 0`, sets the queue
flags FU-63 derived — code `8 → [0x5B370]`, `0x20/0x21 → [0x5B374]`,
`0x40 → [0x5B36C]` — then enqueues into the 10-entry ring at `0x5AAE0`
(stride `0x20`: `+0` id, `+4` RNG value via `0xCB2A4`, `+8` zero, `+0xC` code,
`+0x10` the param record's `+0x59` triple, `+0x1C` param pointer), cursor
`[0x5AADC]`, count `[0x5AAD8]++`.
This is FU-63's `fifa96_event_queue_signal` exactly; the family emits through
it in `01` (code 4), `0B` (code 4), `20` (code `0x20`).

### 2.3 `FUN_0008776C` — the celebration event-id initializer

`0x8776C..0x8784B` (a function not auto-created until this slice) resets the
script index and fills the five event-id bytes `0x587B8..0x587BC`:

```
0x87770  [0x58720] = 0
0x87776  [0x587B8] = t344[rng() & 1]                     ; t344 = object-4 0x10F344 (2 bytes)
0x87789  if ([0x587B8] == 0x67 && (rng() & 1)) { [0x587B9..0x587BC] = 0x67; return }
0x877ba  [0x587B9] = [0x587B8]
0x877cb  [0x587BA] = t349[rng() % 3]                     ; 0x10F349 (3 bytes)
0x877e4  [0x587BB] = [0x587BC] = t34C[rng() % 9]         ; 0x10F34C (9 bytes)
0x877fb  if (rng() & 1) [0x587B9] = t346[rng() % 3]      ; 0x10F346 (3 bytes)
0x8781d  if ([0x587BB] ∈ {0x58,0x5B,0x5F,0x6B} && (rng() & 3) == 0) [0x587BC] = 0x68
```

Static table bytes at flat `0x10F344`: `15 03 19 16 1A 15 67 03 58 5B 6B 56 56
50 50 67` (`t344` `0x10F344` = `{15,03}`, `t346` = `{19,16,1A}`,
`t349` = `{15,67,03}`, `t34C` = `{58,5B,6B,56,56,50,50,67}`; `t34C[1] = 0x5B`
is in the check set). Codes `14` and `25` compare the current animation byte
`[[rec+0x28]]` against these ids and re-post when different
(`0x87AD1..0x87AFB`, `0x88430..0x8845A`, `0x885BC..0x885DA`,
`0x88653..0x88675`).

### 2.4 The scripted block `0x586D8` and the script index `[0x58720]`

Codes `14` and `25` build five candidate position triples at `0x586D8` (stride
`0xC`): slot 0 = a copy of `rec+0x59`; slots 1..4 from RNG-drawn offsets
(§3.5/§3.6). `[0x58720]` is the arm index, incremented once per timed arm; the
active target is slot `3*(idx+1)` (`0x87A93..0x87AB9`, `0x883F2..0x88418`).
All five triples are clamped `x ∈ [-0x720,0x720]`, `z ∈ [-0xB10,0xB10]`
(`0x87A43..0x87A81`, `0x883A2..0x883E0`).

### 2.5 Shared helpers (cited once)

| helper | address | semantics used by the family |
|---|---|---|
| `FUN_00079C50` | `0x79C50..0x79C98` | face: if `(dx,dz)==0` return heading byte `+0x8E`; else angle `0xCD474` → `[rec+0x7D]`, sector `(([rec+0x7B]>>16)+0x40)&0x3FF>>7` → `[rec+0x8E]` |
| `FUN_000795B4` | `0x795B4..0x795F0` | polar delta: out `+0` = angle `0xCD514(p2.x-p1.x, p2.z-p1.z)`, out `+2` = dx, out `+4` = dz |
| `FUN_0008DCD4` | `0x8DCD4..0x8DD5B` | deltas `+2/+4` plus octagonal distance at `+0` |
| `FUN_0008DC68` | `0x8DC68` | octagonal distance (FU-79) |
| `FUN_00079B1C` | `0x79B1C..0x79B56` | tail: copy `+0x59 → +0x4D`, zero `+0x69/+0x71/+0x9C/+0x65/+0x67/+0x73/+0x75` |
| `FUN_00079B58` | `0x79B58` | if `+0x99 == 0` set `+0x93 = 0x10` |
| `FUN_0008DE8C` | `0x8DE8C` | nearest record over `team+0x7A6` (11 × `0xB2`) |
| `FUN_00092AC8` | `0x92AC8..0x92BC1` | 6-tap LCG RNG, returns EAX |
| `FUN_0007D9A4` / `FUN_0007DAB4` | `0x7D9A4`/`0x7DAB4` | install `[rec+0x18]` / reset-chooser (FU-76 §1.1) |

## 3. Per-code deep dives

### 3.1 Code `01` (`0x7DBC0..0x7DFC8`, 4 arms) — post-match celebration sequence

Gate phase 1 (`0x7DBD3`), else reset when `+0x44` (`0x7DFB6..0x7DFBA`).
Marker `[rec+0x8F]>>24 < 2`: camera reset `CALL 0x700F4([0xF328],[0xF32C],
[0xF330],0)` (`0x7DBEA..0x7DC01`), `[0x57A83] = rec`, `out.x = (out.x<0) ? -0x30
: 0x30`, `out.z = 0`, `CALL 0x7876C`; marker `>= 2` copies `+0x59 → +0x4D`
(`0x7DC31`). Timer `+0x89 += delta` (`0x7DC42`). Arm table `0x7DBB0` =
`{0x6DC6B,0x6DCAF,0x6DD29,0x6DFB2}` → `{0x7DC6B,0x7DCAF,0x7DD29,0x7DFB2}`:

* **arm 0** (`0x7DC6B`): `[0x5882A] == 0` → exit; when `+0x89 >= 0x3C`,
  `CALL 0x974DC(0x1E)` (sound), zero-timer and advance.
* **arm 1** (`0x7DCAF`): nearest to the ball `0x5774C` (`FUN_0008DE8C`, range
  `[rec+0x8A]>>24`); both heights `+0x69>>16 <= 0x40`; advance when a slot bit
  `slot[+6] & 0x70` is set, or after `+0x89 > 0x78` without a slot.
* **arm 2** (`0x7DD29`): score-differential chain selecting a celebration ring
  id through `FUN_0008F188(EAX=id, EBX=4|8, EDX=rec|0)`: `[0x57AB6]-[0x57ABA]
  >= 0xA` → if `[0x57AC5] == [0x57AC7]` and `FUN_0008EF38() > 3` → `0x84`
  (`EBX=8`); else `[0x590CC]`/`[0x59901]`/`FUN_000741B4`/`FUN_000CBC4C`
  (`0x7DD87..0x7DEE7`) pick `0x57` or, by `|[0x57AC5]-[0x57AC7]|` band and
  leading side, `0x98` (0xC), `0x9D`/`0xA0` (9), `0x9C`/`0x9F` (6),
  `0x9B`/`0x9E` (3); else `0x57`. Then nearest to `rec+0x59`, vector
  `FUN_0008DCD4` into `0x58738`, `[0x5873A] >>= 1`, `FUN_00092820(rec, 1)`,
  ball stage `FUN_0007A490(rec, 0x58738, [0x58736]>>21, event 6)` (pushes
  `0, 6`), `[team+0x7B2] = selected`, `FUN_0008A938(0xB, side, 0)`,
  `CALL 0x4C380`, zero-timer and advance (`0x7DF9A..0x7DFAC`).
* **arm 3** (`0x7DFB2`): `if (+0x44) FUN_0007DAB4(rec)`.

### 3.2 Code `02` (`0x7DFCC..0x7E1A2`, 3 inline arms) — restart/set-piece

`+0x9E = 1` at entry (`0x7DFD7`). Phase 1 (`0x7DFE6`): `[team+0x7B2]`'s x is
mirrored — `out.x = -([team+0x7B2]+0x59)`, `out.z = 0`, timers zeroed
(`0x7DFEB..0x7E016`). Phase 2 (`0x7E01B`): `[team+0x7B2] = rec`; if no control
slot and `[team+0x828] != 0` → `0x7876C`; **if a control slot exists install
code 4 now on self** (`EDX=4, ECX=1, 0x7E04C..0x7E05A`) and return. Otherwise
timer `+= delta`, output = ball triple `0x5774C`, inline arms `0..2`
(`0x7E08A..0x7E098`):

* **arm 0** (`0x7E0B0`): while `+0x69>>16 > 0x40` wait for `+0x89 >= 0x78`
  then reset + advance; otherwise wait `+0x89 >= 0xA` then advance.
* **arm 1** (`0x7E0F6`): copy `+0x59` to the stack, offset z by `±0x1E0` by
  `[team+0x826]` side, nearest (`0x8DE8C`), vector `0x8DCD4` into `0x58738`,
  `FUN_00092820(rec, 1)` (`EDX=1, ECX=2`), ball stage `FUN_0007A490`
  (`0x7E152..0x7E168`), advance.
* **arm 2** (`0x7E185`): tail `0x79B1C`; `if (+0x44) reset`.

### 3.3 Code `0B` (`0x81908..0x81C71`, 3 arms) — event/duel with ring post

Gate phase 2 (`0x81921`). Arm 0 requires `+0x8D != 0` (`0x81951`):

* If the record template `[[rec+4]] == 0x18DE` (`0x81961`): animation `0x48`
  on self, then animation `0x68` on the ten records at `[team+0x7A6] + k*0xB2`
  (`0x81980..0x819B3`), `[0x58724] = 0`, reset.
* Copy `rec+0x59` to the stack, add the 6-shifted type offsets
  `[0x10F331+type8] << 6` / `[0x10F339+type8] << 6`, nearest over the ball
  record (`0x8DE8C`), polar vector `0x795B4` into `[esp+0xC]`
(`0x81A06..0x81A2B`). The polar result's angle word at `[esp+0xC]` (read
sign-extended by `0x81A30 MOV EAX,[ESP+0xA]; SAR EAX,0x10`) must be in
`(0x20,0x70)` else reset (`0x81A37..0x81A3D`); if `[rec+0x6F]>>16 <= 4` the
event is `0x0C`, else
  `0x59` when `|([rec+0x7B]>>16) - atan(dx,dz)| < 0x1000`
  (`0x81A58..0x81ABD`); `[esp+0x14]` holds the choice. Then `0x79B6C`,
  animation (event) chosen, `+0x9E = 1`, advance (`0x81ADA..0x81B19`).

Arm 1 (`0x81B1F`): requires `+0x3D != 0`; re-runs nearest, gates on
`[found+0x8D]`, delta angle `<= 0x60` (and `<= 0x30` when the chosen id was
`0x59`), `[0x4C32A] == 0`; increments `[team+0x7D7]` and every 10th frame with
a control slot signals `FUN_0008F188(0xA1, 0, code 4)` (`0x81B89..0x81BB2`);
`FUN_0008ED40(rec, -3)`; `FUN_0008A3FC(2, rec, found, &found+0x59)`;
**installs code `0x0C` on the found record** (`EDX=0x0C, EBX=0, ECX=0,
EAX=found, 0x81BD4..0x81BEB`); then side-dependent
`FUN_00092040(0xC8,0/1)`, `FUN_000651F0(4|1)`, `FUN_000974F0(0x258)`, advance.

Arm 2 (`0x81C55`): tail `0x79B1C`; `if (+0x44) reset`.

### 3.4 Code `0D` (`0x8251C..0x826FE`, 4 arms) — event transition

No phase gate. Timer `+= delta`; if `[0x57A83] == rec` clear it (`0x82540`).
Arm table `0x8250C` = `{0x72567,0x7261F,0x726B0,0x726D5}` →
`{0x82567,0x8261F,0x826B0,0x826D5}`:

* **arm 0** (`0x82567`): `+0x8D == 0` → reset. `word [rec+0x83] = 0x20`;
  `CALL 0x702F8([rec+0x81]>>16, 0x10)` → `word [rec+0x85]`, `word
  [rec+0x87] = 0`; animation call; type table `[0x10F334]/[0x10F33C]` scaled by
  3 when both nonzero else 4 into `+0x73/+0x75`; octagonal distance `0x8DC68`
  into `+0x71`; advance (`0x82574..0x82619`).
* **arm 1** (`0x8261F`): while `+0x85 != 0` stay; copy `+0x59`, add 6-shifted
  `[0x10F331]/[0x10F339]` offsets to `+0x4D/+0x55`, vector `0x8DCD4` from
  `+0x59` to `+0x4D` into `+0x65`, animation `0x0B`, advance.
* **arm 2** (`0x826B0`): with `+0x71 != 0` only the `+0x44` reset exits; else
  advance and `0x79C50(+0x71>>16, +0x73>>16)`; `+0x89 > 0x14` resets.
* **arm 3** (`0x826D5`): `0x79C50`; `+0x89 > 0x14` resets.

### 3.5 Code `14` (`0x8784C..0x87CBC`, 3 stages) — scripted celebration sequence

Timer `+= delta`; `FUN_000974F0(0x9C4)`; `[0x57AA3] = rec`; `FUN_00036200(2)`.

**Stage 0** (`0x878A9`): wait `+0x89 >= 0x3C`. If the ball record
`[team+0x7A6]` template byte is `0x26`, animates `0x46`/`0x47` chosen by
`rng&1` on it (`0x878B6..0x878F0`). Then builds the scripted block:
`base = rec+0x59 → 0x586D8`; `dir_x = (x<0)?-1:1` (forced `0` when
`[0x57A49]>>24 == 1`), `dir_z = (z>0)?-1:1` (`0x87900..0x87924`); eight RNG
draws give

```
p1.x = base.x + ((r0&0x7F)+0xA0)*dir_x      p1.z = base.z + ((r1&0x7F)+0xA0)*dir_z
p2.x = p1.x  + ((r2&0x7F)+0x140)*dir_x      p2.z = p1.z  + ((r3&0x7F)+0x20)*dir_z
p3.x = (0x5E0 - (r4&0x1FF))*dir_x           p3.z = p2.z  + ((r5&0xFF)+0x140)*dir_z
p4.x = p3.x  + ((r6&0x7F)+0x50)*dir_x       p4.z = p3.z  + ((r7&0x1FF)+0x280)*dir_z
```

(`0x87933..0x87A1F`), all triples clamped (`0x87A2A..0x87A81`),
`FUN_0008776C()` (`0x87A83`), then the arm target: `idx = [0x58720]`, point
`3*(idx+1)`, vector `0x8DCD4`, move `0x79C50`, and if the animation byte
`[[rec+0x28]] != [0x587B8+idx]` post that id (`0x87AD1..0x87AFB`); advance.

**Stage 1** (`0x87B1F`): `FUN_00045001()` non-zero with `+0x89 > 0xB4` advances
early; otherwise the same arm target/move, and advance when
`[rec+0x63]>>16 < 0x20` or `+0x89 > 0x4B0`, or `idx == 3 && +0x89 > 0x12C`, or
`idx == 4 && +0x89 > 0xB4`; otherwise `idx++`; while `idx < 5` decrement the
stage byte and re-enter the arm block (`0x87BD9..0x87BEF`); at `idx == 5`
advance (`0x87BF4..0x87C0B`).

**Stage 2** (`0x87C13`): `FUN_0007DAB4(rec)`; `[0x57A49]>>24 == 1` exits,
else camera reset `0x700F4([0xF328],[0xF32C],[0xF330])`, `[0x58720] = 0`,
`0x73E28`, `[0x57AAF] = [team+0x826]^1`, `0x740A0`, `0x73E08`, `0x8CFAC`
on both team and ball record with `0x26`, `0x513EC`, ball reset
`0x4C324(0,0,0,0x5774C,0)` (`0x87C2B..0x87CB1`).

### 3.6 Code `16` (`0x84630..0x8471C`, 3 stages) — vector-pop sequence

Stage 0 (`0x84654`): copy `+0x59 → +0x4D`; when `[[0x58866]] & 0xFF == 0x48`
(marker gate; `[0x58866]` is the runtime pointer read by `FUN_00036C70` at
`0x36E20` and zeroed by `0x88724` — open leg 3) zero the timer and advance.
Stage 1 (`0x84692`): `FUN_000795B4(rec+0x59, 0x5880C, out=stack)` polar pop,
`FUN_00092AC8()` → animation `(rng&1) ? 0x6A : 0x55` (`0x846AF..0x846BF`),
`FUN_00079C50`, zero-timer and advance. Stage 2 (`0x846FD`): copy `+0x59 →
+0x4D`; `if (+0x44) FUN_0007DAB4(rec)`.

### 3.7 Code `20` (`0x84EEC..0x85213`, 7 arms) — transition with input table

Zeroes the input words `[0x4C114]/[0x4C118]`, `[0x4C11C] = 0x9F0`,
`CALL 0x4C31C` (`0x84EF4..0x84F12`); re-asserts `[0x57A83] = rec` when
`+0x8F < 3`; timer `+= delta`; arm table `0x84ED0` =
`{0x74F5C,0x75056,0x7512E,0x75153,0x75197,0x751E1,0x75206}`.

* **arm 0** (`0x84F5C`): stages the ball camera (`CALL 0x73DC4`, `0x700F4`
  with `0x5774C..0x57754`), clears `[0x5781D]`, `0x73E08`, copies the ball
  triple to `+0x4D`, `+0x55 -= 0xF0`, `FUN_00079B6C`, places the ball record
  `[team+0x7A6]` at `(0, 0xB10)`, `0x8CFAC ×2`, `0x4C324`, `CALL 0x4C374(0x1B)`,
  advance (`0x84F5C..0x85050`).
* **arm 1** (`0x85056`): when `[0x5882A] != 0` and `FUN_0004BEC8() == 0` and
  `FUN_0007A028()` and `+0x89 >= 0x3C`, picks a ring id by `[0x587E6]>>24`
  vs the team side and RNG (`0x26`/`0x28` on the ball record, `0x25`/`0x27`,
  or `0x54`) and `CALL 0x8F188(EAX=id, EBX=0x20, EDX=record)` — code `0x20`
  raises the FU-63 `0x5B374` flag; advance (`0x85056..0x85128`).
* **arm 2** (`0x8512E`): wait `+0x89 >= 0x3C`, advance.
* **arm 3** (`0x85153`): with a slot, `slot[+6] != 0` decides through
  `FUN_00078A84`; without a slot wait `+0x89 > 0x78`; advance.
* **arm 4** (`0x85197`): copy ball triple to `+0x4D`; `+0x69>>16 <= 0x40`;
  `FUN_00078AA4`; kick apply `FUN_0007B9C4(rec, EDX=0x40)`; advance.
* **arm 5** (`0x851E1`): `0x79B1C`; occupied → advance then reset.
* **arm 6** (`0x85206`): `FUN_0007DAB4(rec)`.

### 3.8 Code `22` (`0x8539C..0x85481`, 3 stages) — phase-2 short event

Gate phase 2 (`0x853AB`), else reset. Timer `+= delta`; `0x79B1C`.
**Stage 0** (`0x853E9`): `+0x8D == 0` → reset; else `+0x9E = 1`, `0x79B1C`,
`+0x43 = 0`, advance. **Stage 1** (`0x85428`): `[rec+0x69]>>16 > 0x30` →
reset when `+0x89 > 0x1E`, else wait; height `<= 0x30` → animation/event
`0x4F` (`EDX=0x4F, 0x85441..0x85453`), advance. **Stage 2** (`0x85470`):
`+0x44` → reset (`0x85476`). Installed on the opponent by code 7's stage 1 at
`0x816DF` (`EDX=0x22, ECX=1`; FU-76 §3.2).

### 3.9 Code `24` (`0x86510..0x866F1`, 3 stages) — stats/commentary sequence

Timer `+= delta`; if `+0x8D != 0`, writes the animation byte
`[rec+0x7B] = byte[[0x57A38] + (byte[[rec+4]+0xA]>>24)] - 2`
(`0x86537..0x8654E`). **Stage 0** (`0x8656F`): `+0x59 = 0x9F0`, direction
words zeroed, z = `±6*subtype` by `[team+0x826]` side, `0x79C50(EDX=1)`;
waits while `+0x89 <= ((subtype * (([rec+0x8A]>>25) * 60)) >> 4) + side*60`
(`0x865CB..0x8660B`); then `+0x55 = +0x61`, `+0x4D = 0x7E0 + 6*subtype`,
advance (`0x86611..0x8665B`). **Stage 1** (`0x8665C`): gated by
`FUN_0008EEC0() == 0 && [0x57AC2] == 0 && [0x4C32A] == 0` →
`FUN_00067800(1)`, `FUN_0008EEB4()`; `+0x4D = 0x7E0 + 6*subtype`; advances
when `[rec+0x63]>>16 < 0x20` (`0x866E5..0x866EE`), otherwise waits.
**Stage 2** (`0x866DD`): hand-off to the
phase placement handler `FUN_0006E1D0(EAX=rec, EDX=&rec+0x4D, EBX=-1)` — the
`0x2E/0x3F` slot of the FU-81 phase table. Installed by the `0x8B688` timeline
driver (slot `0x4D`) on each of the 11 records (`ESI += 0xB2`):
`0x8B747 EDX=0x24; 0x8B750 CALL 0x7D9A4`, `0x8B782 EDX=0x24; 0x8B78B CALL
0x7D9A4` (both `ECX=1`).

### 3.10 Code `25` (`0x880CC..0x886AF`, 7 arms) — stats sequence

Timer `+= delta`; `FUN_000974F0(0x9C4)`; `[0x57AA3] = [rec]` (team);
`FUN_00036200(2)`. While `+0x8D != 0`: output = `[team+0x59]` plus a circular
offset of radius `0xC0` — both components are `(int8)([0x14E04][(angle) &
0xFF]) * 0xC0 >> 16` with the angle words `(([rec+0x8A]>>24)<<6)` and
`+0x80` (the two `shl/sbb/xor/sub` folds at `0x8811F..0x881BA`) — then vector
`0x8DCD4` and move `0x79C50`. Arm table `0x880B0` =
`{0x781ED,0x7847E,0x78568,0x7859B,0x785FE,0x78645,0x78699}`.

* **arm 0** (`0x881ED`): wait `+0x89 >= 0x3C`. Active: animation `0x15`/`0x03`
  by `rng&1` unless the current byte is already one of them; if `[0x58720] == 3`
  jump to stage 3 with `[rec+0xAE] = (rng&0x7F)+0x20`. Inactive: build the
  scripted block with 7 draws + `rng % 0xF0` (`p1.x = (r0%0xF0)*dir_x`,
  `p1.z = base.z + ((r1&0xFF)+0x1E0)*dir_z`, `p2.x = p1.x +
  ((r2&0x7F)+0x20)*dir_x`, `p2.z = p1.z + ((r3&0xFF)+0x140)*dir_z`,
  `p3 = (p2.x, p2.z + ((r4&0xFF)+0x3C0)*dir_z)`, `p4 = (p3.x +
  ((r5&0x7F)+0x50)*dir_x, p3.z + ((r6&0x1FF)+0x3C0)*dir_z)`,
  `0x8827B..0x88359`), clamp, `FUN_0008776C`, arm target, `0x8DCD4`/`0x79C50`,
  animation compare against `[0x587B8+idx]`, advance.
* **arm 1** (`0x8847E`): `FUN_00045001()` non-zero advances; else the same arm
  target and advance conditions `[rec+0x63]>>16 < 0x20`, `+0x89 > 0x708`,
  `idx == 3 && +0x89 > 0x258`, `idx == 4 && +0x89 <= 0x12C` stay; `idx++`;
  `idx < 5` decrement stage and re-enter; else advance (`0x8852C..0x88567`).
* **arm 2** (`0x88568`): reset; if now inactive, ball reset `0x4C324` and
  `[0x58720] = 0`.
* **arm 3** (`0x8859B`): wait `+0x89 > [rec+0xAE]` or height `+0x63>>16 > 0x60`;
  animation `[0x587BB]` compare; advance.
* **arm 4** (`0x885FE`): only when `[0x58720] == 4`; new countdown
  `[rec+0xAE] = (rng&0x7F)+0x20`; advance.
* **arm 5** (`0x88645`): wait `+0x89 > [rec+0xAE]`; animation `[0x587BC]`
  compare; advance.
* **arm 6** (`0x88699`): when `[0x58720] == 0` reset.

## 4. Hand-offs and installs

| code | installed by | site/citation |
|---|---|---|
| `0B` | installs `0C` on the nearest record | `0x81BD4 EDX=0x0C … 0x81BEB CALL 0x7D9A4` |
| `16` | phase-timeline driver `0x89FA4` (slot `0x46`) | `0x8A096 EDX=0x16; 0x8A09B EAX=[0x5888F]; 0x8A0A0 CALL 0x7D9A4` (`ECX=1`) |
| `22` | code `07` stage 1 | `0x816DF FUN_0007D9A4(opp, 0x22, ECX=1)` (FU-76 §3.2) |
| `24` | phase-timeline driver `0x8B688` (slot `0x4D`) | `0x8B747/0x8B782 EDX=0x24; 0x8B750/0x8B78B CALL 0x7D9A4` per record (11 × `0xB2`, `ECX=1`) |
| `02` | installs `4` on self when a control slot exists | `0x7E04C..0x7E05A` |
| `24` | stage 2 delegates to the phase handler | `0x866E7 CALL 0x6E1D0` (phase `0x2E/0x3F`) |
| `01,0D,14,20,25` | no static install site found (FU-76 §1.3 census) | open leg 2 |

Every arm hand-off that ends the record's participation runs
`if (+0x44) FUN_0007DAB4(rec)`; `+0x44` comes from the animation row bit 0
(§2.1). Advance is uniformly `+0x89 = 0; +0x92++`.

## 5. Event-queue and timeline ties

* **FU-63 queue.** `FUN_0008F188` (§2.2) is FU-63's command-ring enqueue:
  `01` arm 2 signals celebration ring ids (`EBX=4`), `0B` arm 1 signals `0xA1`
  every 10th team frame with a slot, `20` arm 1 signals `0x26/0x28/0x25/0x27/
  0x54` with code `0x20` (sets `[0x5B374]`). The ported API is
  `fifa96_event_queue_signal(q, id, param, code)`.
* **Animation posts.** All ten bodies express their events as
  `FUN_0006E598(rec, anim_id, type8, 0)`; the 9-byte animation rows and their
  side effects (`+0x43/+0x44/+0x45`) are the event mechanism (§2.1).
* **Timeline/cinematic drivers (FU-81 open leg 2).** `0x89FA4` (slot `0x46`)
  installs `16`; `0x8B688` (slot `0x4D`) installs `24`; `0x8776C` (called by
  `14`/`25`) initializes the celebration ids; the `0x886D4` reset helper
  zeroes `[0x58866]` (the code-`16` marker slot) and the `0x58818/0x58808/
  0x58828/0x58829` timeline state.
* **Stats gates.** `24` stage 1 uses `FUN_0008EEC0`/`FUN_0008EEB4`/
  `FUN_00067800` gated by `[0x57AC2]`/`[0x4C32A]`; `25` reuses the same
  `0x974F0(0x9C4)`/`0x36200(2)` prologue and the `0x8776C` ids.

## 6. Port: `fifa96_action_sequence_*`

`include/fifa96_loader/fifa96_action_handlers.h` +
`src/fifa96_loader/fifa96_action_handlers.c` (caller-owned state, no globals,
negative `fifa96_err_t` for invalid arguments, no comments).

| original | port |
|---|---|
| arm dispatch `CMP AL,n; JA tail; JMP CS:[EAX*4+table]` (`0x7DBB0`, `0x8250C`, `0x84ED0`, `0x880B0`) | `fifa96_action_sequence_select(stage, arms, count, &arm)` — bounds-checked stage→arm lookup |
| animation compare `[[rec+0x28]] != [0x587B8+idx]` (`0x87AD1`, `0x88430`, `0x885BC`, `0x88653`) | `fifa96_action_sequence_event(anim_byte, event_id, &post)` |
| marker gate `[[0x58866]] == 0x48` (`0x84665..0x84674`) | `fifa96_action_sequence_marker(marker, want, &match)` |
| RNG event pick `(rng&1) ? odd : even` (`0x846AF`, `0x878DC`, `0x8822B`, `0x8509B`) | `fifa96_action_sequence_rng_event(rng, even_id, odd_id, &event_id)` |
| window `[rec+0xAE] = (rng&0x7F)+0x20` (`0x8825C`, `0x88615`) | `fifa96_action_sequence_countdown(rng, &countdown)` |
| animation byte `[rec+0x7B] = table[idx]-2` (`0x86537..0x8654E`) | `fifa96_action_sequence_anim_byte(table, index, &value)` |
| code `24` lane `±6*subtype`, `x = 0x7E0+6*subtype`, timer `((subtype*(([+0x8A]>>25)*60))>>4)+side*60` (`0x86577..0x8660B`) | `fifa96_action_sequence_lane(subtype, side, boost, &out)` |
| code `14` five-point scatter + clamp (`0x87933..0x87A81`) | `fifa96_action_sequence_scatter_celebration(base, dir_x, dir_z, rng[8], points[5])` |
| code `25` five-point scatter (`0x8827B..0x883E0`) | `fifa96_action_sequence_scatter_stats(base, dir_x, dir_z, rng[7], points[5])` |
| code `0B` event choice (`0x81A37..0x81ABD`) | `fifa96_action_sequence_duel_event(delta_angle, aim, facing, atan_delta, &out)` |
| code `22` height/timer event (`0x85428..0x85453`) | `fifa96_action_sequence_press_event(height, timer89, &out)` |
| `FUN_0008776C` celebration id init (`0x8776C..0x8784B`) | `fifa96_action_sequence_event_ids(t344, t346, t349, t34c, rng[7], ids[5])` |
| timer/gate/stage/advance/finish (`+0x89`, phase gates, `+0x44`) | already ported as `fifa96_action_stage_*` (FU-81) |
| `0x6E598`, `0x8F188`, `0x587xx`/`0x586D8` block storage, animation rows, `0x8Bxxx`/`0x89xxx` timeline drivers, `0x4Cxxx` engine calls | not ported (globals/pointers/tables; cited in §2/§5, open legs) |

## 7. Tests (`tests/test_event_sequences.c`, suite 70 → 71)

* Layout `_Static_assert`s on the new structs.
* `sequence_select`: 4-arm (`01`) and 7-arm (`20`/`25`) lookups, last stage,
  out-of-range/`0xFF`, `NULL` table/out, `count == 0`.
* `sequence_event`: differ/equal/zero/`0xFF`, NULL.
* `sequence_marker`: `0x48` match, off-by-one, both-zero, NULL.
* `sequence_rng_event`: even/odd/`0xFFFFFFFF`/zero pairs, NULL.
* `sequence_countdown`: `0`, `0x7F`, `0x80`, `0xFFFFFFFF`, NULL.
* `sequence_anim_byte`: `0 → 0xFFFE`, `2 → 0`, `5 → 3`, `0xFF → 0xFD`, NULL.
* `sequence_lane`: side negation, `subtype 0`, boost `3<<25`, zero boost,
  negative boost (`0xFE000000`, arithmetic `>>4`), `0x01FFFFFF`, NULL.
* `scatter_celebration`: the eight-draw path, both clamp axes and the `dir = 0`
  degeneracy, NULL base/rng/points.
* `scatter_stats`: modulo draw, absolute `p1.x`, clamps, NULLs.
* `duel_event`: window edges `0x20/0x21`, `0x6F/0x70`, `aim <= 4`, facing-diff
  edges `±0x1000`, NULL.
* `press_event`: height `0x30/0x31`, timer `0x1E/0x1F`, negative height, NULL.
* `event_ids`: normal path, `0x67` short-circuit, check-set `0x68` override,
  `t346` override, all six NULLs.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_event_sequences.c src/fifa96_loader/fifa96_action_handlers.c
src/fifa96_loader/fifa96_entity_update.c` runs clean. `make test`: 70/70
before, **71/71 after**.

## 8. Errata (quoted)

* FU-76 §2 `EVENT` = `CALL 0x6E598` — **refined**: `0x6E598` is the animation
  selector (input `EDX = animation id 0..0x6E`; row `[0x57588] + id*9` stored
  at `[rec+0x28]`; row byte `+2` bits → `+0x45/+0x43/+0x44`; `+0x46 = 1`;
  `+0x3F = 2/-2/0`; `CALL 0x6E490`). The `+0x44` reset gate every
  stage-family and event-sequence body tests is the row's **bit 0**, so
  "occupied" is animation-driven, not a separate event.
* FU-76 §2 code `0B` "INSTALL `0x0C` via `0x81BEB`" — **confirmed** (the
  EDX=0x0C install is at `0x81BEB`, target = nearest record, `ECX=0`).
* FU-76 §2 code `22` "installed on the opponent by code 7's stage 1
  (`0x816DF`)" — **confirmed**.
* FU-76 §2 code `24` "installed `0x8B750/0x8B78B`" — **confirmed**
  (`EDX=0x24, ECX=1`, 11-record loop).
* FU-81 §1.5 timeline install list "`0x8A0A0`" — **refined**: that site
  installs action code `16` (`0x8A096 EDX=0x16; 0x8A09B EAX=[0x5888F];
  0x8A0A0 CALL 0x7D9A4`, `ECX=1`).
* FU-76 §2 counts vs FU-81 13-code stage superset — **explained** by the
  partition reconstruction of §1: 8 stage codes + 4 event/sequence codes with
  *transition* class labels (`01,02,0D,20`) + the mis-decoded stub `15`. The
  reconstruction itself is not quoted from FU-76 §2 (open leg 1).
* Tooling errata: `disassemble_function 0x84630` loses sync at `0x84665`
  (code `16`); the body was decoded linearly (`ndisasm -b32`) with no
  decompiler output. Bodies `14`/`25`/`22`/`24`/`0B`/`20` were dumped and
  decoded the same way (Method).

## 9. Open legs

1. **Partition reconstruction**: FU-76 §2 does not state the 8-stage/10-event
   split; the assignment in §1 is the unique one consistent with the FU-76
   report counts and class labels, but is inferred, not quoted.
2. **No static installer for `0D`, `14`, `25`**: the FU-76 §1.3 census has
   direct immediates only for `16`/`24`/`22`/`02`/`0B` among this family; how
   `0D`/`14`/`25` enter a record is unknown (possibly computed or table-driven).
3. **`[0x58866]` writer**: the code-`16` marker pointer is read at `0x36E20`
   (`FUN_00036C70`, render snapshot) and zeroed at `0x88724`; no other static
   writer was found, and the pointer's target byte `0x48` is unproven data.
4. **Animation rows**: `[0x57588]` is a runtime pointer slot (zero statically);
   the 0x6F 9-byte rows and the fields beyond `+2` are not enumerated.
5. **`[0x57A38]`** (the code-`24` animation-byte table) is read as a pointer;
   its runtime target is unproven.
6. **Gate helpers** `0x8EF38`, `0xCBC4C`, `0x45001`, `0x741B4`, `0x8EEC0`,
   `0x8EEB4`, `0x4BEC8`, `0x7A028`, `0x78A84`, `0x78AA4`, `0x8A3FC`,
   `0x8ED40`, `0x92040` are cited by call only (FU-81 open leg 3 class).
7. **`0x4C324`/`0x4C374`/`0x4C31C`/`0x4C110..0x4C11C`** engine/input calls are
   cited, not decomposed.
8. **Code `01` arms**: the listing windows `0x7DC89..0x7DCA7` and
   `0x7DEFE..0x7DF08` are mis-decoded (FU-81 erratum) and not quoted; the
   celebration-id chain is quoted around them.
9. **Code `02` arm 1** nearest/vector block is quoted at instruction level but
   the `0x92820`/`0x7A490` parameters (EDX=1/ECX=2, event 6) are not traced
   into the ball-staging code.
10. **`FUN_0006E490`** (animation frame helper) and the `0x57588` row layout
    are cited by address only.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`; `switch_program`;
`disassemble_function` 0x6E598, 0x795B4, 0x79C50, 0x8DCD4, 0x79B1C, 0x79B58,
0x8DD70, 0x36C70, 0x8F188, 0x92AC8, 0x7DBC0, 0x7DFCC, 0x81908, 0x8251C,
0x8784C, 0x84630, 0x84EEC, 0x8539C, 0x86510, 0x880CC (the last six confirmed
mis-decoded or cut and re-decoded linearly); `disassemble_bytes` 0x7E440,
0x8B740, 0x8A080, 0x8776C; `run_script_inline` body dump to
`/tmp/opencode/fu82/*.bin` + arm-table dumps; `read_memory` 0x1106E0-related
arm tables 0x7DBB0, 0x8250C, 0x84ED0, 0x880B0, 0x158866, 0x157588, 0x10F344;
`search_instructions` operand `58866` (3 sites), `EDX, 0x16/0x14/0x25` (anim
ids, not installs); `get_function_by_address` 0x7DBC0, 0x7DFCC, 0x81908,
0x8251C, 0x8784C, 0x84630, 0x84EEC, 0x8539C, 0x86510, 0x880CC. Linear decode:
`ndisasm -b32 -o<origin>` over the dumped ranges. Analysis-only outside the
port: no tool, capture-rig, ISO or Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_event_sequences.c`, `CMakeLists.txt` (one test block).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
