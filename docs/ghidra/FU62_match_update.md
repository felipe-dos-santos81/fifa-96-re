# FU-62: the match update — phase machine, clock and period rollover

Roadmap slice S5 of FU-60 §6 ("match update step"), extended to the phase
variable and its gates (task 1) and to the match clock/period state (task 3).
Derives what the per-frame update does to world state and time, and ports the
evidenced phase/clock core.

Result in one line: **the match update is a 30 Hz drained-frame step whose time
base is Q8: `FUN_0004C394(0x200)` adds 2.0 units to `[0x57A5C]`, `FUN_0004B100`
splits that into a per-frame whole tick delta (always 2 with step `0x200`) at
`[0x57A64]`, a 16-bit cumulative tick counter `[0x57A66]`, and a fractional
remainder `[0x57A5C] & 0xFF`, then calls the world/clock machine
`FUN_0008AF38`; that machine gates all clock activity on the phase-class table
at flat `0x1106AD` indexed by the phase byte `[0x57A4D]` (= `[0x57A4A]>>24`):
class 1 (only phase 2 among 0..0x14) always runs, class 2 (phases
0,3,4,6,7,8,9,0xD) runs unless `[0x4C302]!=0`, class 0 stops; a running clock
adds the delta to the sub-second accumulator `[0x57AC1]` and rolls each 60
units into the period seconds `[0x57AB6]` and total match seconds `[0x57AB4]`,
updates the auxiliary end-condition counter `[0x57ABA]`/`[0x57ABC]` on class-2
phases, and on `period_seconds == length + aux` (length `[0x5881A]` for
periods 0/1, `[0x5881C]` for periods 2/3, from `[0x4C1D1]` minutes × 60/× 20)
sets the completion flag, advances the period `[0x57AC2]` through
`FUN_0008B9CC` and zeroes the period seconds; `FUN_00036208(0x200)` pans the
camera by `0x200>>4 = 0x20` per frame. The phase byte is written by exactly one
setter, `FUN_000740A0` (previous phase saved to `[0x57A4E]`), and `FUN_00049B28`
uses it to short-circuit a frame when input event 2 is active and the phase is
not `0xB`/`0xF`, to pick the `[0x4C32A]`-selected A/B event chain, and to gate
the 13-value `[0x7310]` cadence set.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58/FU-59/FU-60/FU-61).
  All instructions quoted below were read back from Ghidra this slice
  (`disassemble_function`/`disassemble_bytes`/`read_memory`); the decompiler
  was used for callee characterisation where it succeeded.
* **Address convention (FU-59/FU-61, quoted):** code immediates naming
  tables/globals carry LE fixups; the loader adds the owning object's base at
  run time. FU-61 resolved object 1 base `0x10000` and object 4 base
  `0x100000`. This slice applies the same rule to two new tables:
  the phase-class table is encoded `0x106AD` → flat `0x1106AD` (base
  `0x100000`, byte-verified by the `read_memory 0x1106AD` dump below), and the
  period-end jump table is encoded `0x7AF28` → flat `0x8AF28` (base `0x10000`,
  the code object; it sits immediately before `FUN_0008AF38` and its four
  dwords resolve to the four case bodies).
* **Ghidra listing defect met this slice (new errata):** the
  `disassemble_function 0x8AF38` listing is byte-shifted inside the period-end
  region: it shows `0x8B1F0 CMP dword [0x57ABA],0` where the true instruction
  stream (from the case entry `0x8B1DD`, the jump-table target) is
  `0x8B1E3 MOV BX,[0x5881A]` / `0x8B1EA CMP AX,BX` / `0x8B1ED JNZ` /
  `0x8B1EF MOV AX,[0x57AB6]`-family block, etc. `disassemble_bytes 0x8B1D0` and
  `disassemble_bytes 0x8B22F` (started at the true case entries) decode the
  region correctly; the decompile also dies at the embedded jump table
  (`halt_baddata`). The case bodies below are from those two hand-aligned
  listings, and the four case entry addresses were confirmed by reading the
  three tables (jump table, period-class, phase-class) as raw bytes.
* All numeric claims (accumulator arithmetic, class indices, case conditions)
  are quoted from the listings; semantic labels (what a phase "means", what
  `[0x57ABA]` is) are not asserted — the field roles listed are exactly what
  the instructions do with them.

## 1. The phase variable `0x57A4A` / phase byte `0x57A4D`

### 1.1 Storage and writers

`FUN_0004B380` (`0x4B380..0x4B384`: `MOV EAX,[0x57A4A]; SAR EAX,0x18`) returns
the phase as a byte. The byte lives at `0x57A4D`; `0x57A4E` holds the previous
phase and `0x57A4C` is a neighbouring byte cleared by the reset paths. A
`search_instructions operand 57a4d` sweep of all 191,967 defined instructions
finds **exactly two writers**:

* `FUN_00073E28` (match state zero, called from `FUN_00074034`) writes
  `[0x57A4E]=0`, `[0x57A4D]=0` (`0x73E42`/`0x73E48`/`0x73E4E` write
  `0x57A4C`/`0x57A4E`/`0x57A4D` with AH=0).
* `FUN_000740A0` is the setter (below).

All other 132 `57a4*` matches are reads (or the `0x57A4C`/`0x57A4F` side
bytes). So every phase transition outside reset goes through `FUN_000740A0`.

### 1.2 Setter `FUN_000740A0` (18 call sites)

`decompile_function 0x740A0` + listing:

```
0x740A0  [0x57A4E] = [0x57A4D]                  ; previous phase
0x740AC  [0x57A4D] = AL                         ; new phase
0x740A0  [0x57AAF] = param_2 (DL)               ; companion byte
         FUN_0008D098(); FUN_0008D098();
         if (phase == 2) { FUN_0004C380();       ; no-op body (0x4C380 = RET)
                           [0x5781D]=0; [0x57AB2]=1; [0x57A73]=&DAT_0005774C; }
```

`get_xrefs_to 0x740A0` → 18 callers: `FUN_0008A938` (`0x8AA89`),
`FUN_0008B9CC` (`0x8BAC1`), `FUN_0008BAF0` (`0x8BCCF`), and 15 unnamed
call sites in the `0x87xxx..0x94xxx` match-event module (`0x895AC`, `0x94040`,
`0x944A3`, `0x94877`, `0x8A091`, `0x87C71`, `0x88C82`, `0x8915B`, `0x89372`,
`0x8972F`, `0x8AB9F`, `0x8AC1C`, `0x8ADB4`, `0x93DB2`, `0x94681`). The
transition graph those sites implement is not decomposed (open leg); the
evidenced *funnel* is `FUN_0008B9CC` (phase chosen from `[0x57AAC]>>24` and the
end-of-period flags, `0x8B9CC..0x8BAE1`) which is called from the period-change
path of the clock (§4.5) and from `FUN_0008BAF0`.

### 1.3 Phase-class table (flat `0x1106AD`)

`FUN_0008AF38` starts (`0x8AF41..0x8AF59`):

```
MOV EAX,[0x57A4A]; SAR EAX,0x18          ; phase
MOVZX DI, byte [EAX + 0x106AD]           ; encoded -> flat 0x1106AD
MOV AL, byte [EAX + 0x106AD]
```

`read_memory 0x1106AD 256` returns 48 meaningful entries followed by unrelated
function pointers at `+0x30`:

| phase | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 0xA | 0xB | 0xC | 0xD | 0xE | 0xF |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| class | 2 | 0 | 1 | 2 | 2 | 0 | 2 | 2 | 2 | 2 | 0 | 0 | 0 | 2 | 0 | 0 |

| phase | 0x10 | 0x11 | 0x12 | 0x13 | 0x14 | 0x15 | 0x16 | 0x17 | 0x18 | 0x19 | 0x1A | 0x1B | 0x1C | 0x1D | 0x1E | 0x1F |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| class | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 1 | 0 | 0 | 1 | 0 | 1 | 1 | 1 | 0 |

Indices 0x20..0x2F read 0. The table is the clock gate: phase class 1 always
lets the clock run; class 2 runs only while `[0x4C302]==0`; class 0 never
(§4.2). The table's use is purely the gate — no other consumer of the 48 bytes
was found in the cited functions.

### 1.4 Phase uses in the frame path (evidenced sites only)

| site | test | effect |
|---|---|---|
| `FUN_00049B28 0x49BF3..0x49C08` | phase ∉ {0xB,0xF} | input event 2 is processed and the frame body is short-circuited |
| `FUN_00049B28 0x49DFE..0x49E08` | phase ∈ {0xB,0xF} | selected event code is cleared (`EDX=ECX=0`) before chain C |
| `FUN_00049B28 0x49FFA..0x4A02A` | phase ∈ {0,1,3..8,0xA..0xC,0x13,0x14} | `[0x7310]++`; all other phases reset it to 0 |
| `FUN_00091DD8 0x91E3D` | phase == 2 | deadline branch enqueues `0x8D` when `\|[0x57754]\|<0x2D0` |
| `FUN_0008AF38 0x8AF95..0x8AFA7` | phase == 2 | `FUN_00089EB0(delta)` (per-frame, phase-2 only) |
| `FUN_0008AF38 0x8B623..0x8B643` | phase == 2 or 0x10 | `FUN_00088940()` when `[0x5781D]!=0` |
| `FUN_0008AF38 0x8B645..0x8B655` | phase == 0xC | `[0x57ABA]=0`, `[0x5882D]=0` |
| `FUN_0004B100 0x4B2B7` | phase == 2 && `[0x59056]` | `FUN_0007D430(entity)` when the entity pointer at `+0x7B2` non-zero |
| `FUN_0004B308` | phase 3→1/2, 4→3/4, 0x15→8, else 0 | view-class code used for the camera clamp bounds in `FUN_00036208` |
| `FUN_0004B3C0` | phase != 2 && `[0x57AB4]==0` | boolean |
| `FUN_0004B3E0` | phase ∈ {0x13,0xC,0x11,0x14} | boolean |
| `FUN_0004B6FC` | phase == 6 (or period > 3) | entity/score lookup |

No phase→situation label is asserted. What phase 2 is can only be inferred from
behaviour: it is the sole class-1 phase (always-running clock) and the target of
the setter's special arm (`[0x5781D]=0`, `[0x57AB2]=1`).

## 2. The phase/event chain in `FUN_00049B28`

FU-60 §3 gave the loop skeleton; this slice re-read the full clean listing
(`disassemble_function 0x49B28`, 348 instructions, `0x49B28..0x4A064`) and the
raw bytes at the mis-decoded tail (`read_memory 0x49FC8 64`). The byte-exact
tail is:

```
0x49FCA  CALL 0x91DD8                 ; presentation/event pump (FU-60 §4)
0x49FCF  CALL 0x51AB8                 ; [0x4E574] suspend gate
0x49FD4  TEST EAX,EAX / JNZ 0x49FE1
0x49FD8  CALL 0x36C3C                 ; [0x5FFC] in {2,4}
0x49FDD  TEST EAX,EAX / JZ 0x49FF5
0x49FE1  CALL 0x45D0D                 ; input hold/refresh (FU-61 §4.3)
0x49FE6  CALL 0x4B454                 ; [0x4C32A]
0x49FEB  TEST EAX,EAX / JZ 0x4A04A    ; class-2-style arm: [0x4C32A]==0 -> skip to loop tail
0x49FF3  JMP 0x4A045                  ; else FUN_00053A48
0x49FF5  CALL 0x36C70
0x49FFA  CALL 0x4B380                 ; phase
0x49FFF  CMP EAX,3  / JC 0x4A01F
0x4A004  CMP EAX,8  / JBE 0x4A024     ; 3..8 -> increment
0x4A009  CMP EAX,0xA / JC 0x4A02C     ; 9 -> reset
0x4A00E  CMP EAX,0xC / JBE 0x4A024    ; 0xA..0xC -> increment
0x4A013  CMP EAX,0x13 / JC 0x4A02C    ; 0xD..0x12 -> reset
0x4A018  CMP EAX,0x14 / JBE 0x4A024   ; 0x13..0x14 -> increment
0x4A01D  JMP 0x4A02C                  ; > 0x14 -> reset
0x4A01F  CMP EAX,1  / JA 0x4A02C      ; phase > 1 -> reset
0x4A024  INC dword [0x7310]           ; cadence set {0,1,3..8,0xA..0xC,0x13,0x14}
0x4A02C  MOV dword [0x7310],ECX       ; ECX = 0
0x4A032  CMP dword [0x7310],0x5A / JGE 0x4A045
0x4A03B  MOV EAX,1 / CALL 0x6408C
0x4A045  CALL 0x53A48
0x4A04A  CMP ECX,[0x7328] / JL 0x49BA4 ; loop while 0 < remaining
```

Per drained frame (one `FUN_00049B28` iteration, `0x49BA4..0x4A050`):

1. `0x49BA4 CALL 0x45F6A` — input scan/record decode (FU-61 §4); `< 0` aborts
   the whole drain (`0x49BAD` subtracts only processed frames).
2. `0x49BD2 DEC [0x7328]`; `0x49BD8 CALL 0x4B380` — phase in EDX and EBX.
3. `0x49BE1` `FUN_000451F1(2)`: if input event 2 is set **and** phase ∉
   {0xB,0xF} (`0x49BF3`), the block `0x49C0E..0x49CF3` runs the event-2 path
   (menu gate `FUN_00037AE4`, then `FUN_00045268(2)` → player,
   `FUN_00037798`, `FUN_00037DAC`, then record/flush housekeeping) and jumps
   to the loop tail `0x4A04A` — the `FUN_0004C394`/`FUN_0004B100`/
   `FUN_00036208`/`FUN_00091DD8` chain is **skipped for that frame**.
4. Otherwise `0x49CF4` `FUN_0004B454()` = `[0x4C32A]` selects the event chain:
   chain A (`[0x4C32A]!=0`, `0x49D03..0x49D7A`): codes `7, 0xA, 0xB, 8, 0xF`,
   each hit `MOV EDX,code; JMP 0x49EB9` — chain A short-circuits B and C;
   chain B (`==0`, `0x49D7F..0x49DF5`): codes `9, 4, 5, 6, 0xF, 0xB`, each hit
   `MOV EDX,code; JMP 0x49DFE` — it records a code but does **not** skip the
   phase gate/C.
5. `0x49DFE`: if phase ∈ {0xB,0xF}, `EDX=0` (so a chain-B code only survives
   for those phases); then chain C (`0x49E0A..0x49EB4`): codes
   `7, 0xA, 0xC, 0xD, 0xE, 8, 1, 3`, each hit `MOV EDX,code; JMP 0x49EB9`.
   Chain B and chain C code sets are disjoint, so chain C overrides a chain-B
   selection whenever one of its flags is set; chain A always wins.
6. `0x49EB9`: if a code was selected, gate `FUN_00037AE4` (menu) and run
   `FUN_000492D4`/`FUN_00063900`/`FUN_000510C0`/`FUN_00053F08`/
   `FUN_00047878`, then dispatch: `FUN_00045268(code)` (player), unless code 1,
   `FUN_00037798`, `FUN_00038004(code)`.
7. `0x49F49..0x49FB1`: menu gate again; if `[0x9A98]==0` (`FUN_0006400C`) and
   `[0x4E574]==0` (`FUN_00051AB8`) then the frame path:
   `CALL 0x4C394(0x200)` → `CALL 0x4B100` → `CALL 0x36208(0x200)` →
   `CALL 0x91DD8`.
8. `0x49FCF..0x4A045`: post-update suspend recheck; if `[0x5FFC] ∈ {2,4}`
   (`FUN_00036C3C`) then `FUN_00045D0D` and, when `[0x4C32A]!=0`,
   `FUN_00053A48` (else loop tail); otherwise `FUN_00036C70`, the phase cadence
   `[0x7310]`, `FUN_0006408C(1)` when `< 0x5A`, `FUN_00053A48`.
9. `0x4A04A`: `CMP ECX,[0x7328] / JL` — **ECX is the constant 0** (zeroed at
   `0x49BA2`, only read afterwards, and Watcom callees preserve ECX), so this
   is `while (remaining > 0)`. This closes FU-60 §9's register-contract open
   leg: the comparison is 0 vs the decremented count, not a counter.

Drain tail: normal `0x4A056: [0x731C] -= [0x732C]`; abort
`0x49BAD..0x49BC8: [0x732C] -= [0x7328]; [0x731C] -= [0x732C]`.

### Gates

* `[0x4C32A]` (`FUN_0004B454`): selects chain A vs B in step 4 and the
  `[0x5FFC]∈{2,4}` arm in step 8; also gates `FUN_000948AC` in `FUN_0004B100`
  (§3.2) and the ambience/enqueue in `FUN_00091DD8` (FU-60 §4.2). It is set by
  `FUN_0001B7B8` to `([0x4C1D0]==4)` with a companion write.
* `[0x4C312]` (dword): not used by `FUN_00049B28`; inside `FUN_00091DD8` at
  `0x91DF4` it gates the whole body **after** the music start (`0x91DE1`) and
  the ambience call (`0x91DEF`), as FU-60 §4.2 states. `get_xrefs_to 0x4C312`
  still returns exactly two reads (`0x8F19F`, `0x91DF4`) and no static writer.
* The same byte is also used at `0x49FE6` as "state 2/4 arm" selector.

## 3. The three update callees and the `0x200` step

### 3.1 `FUN_0004C394` — Q8 time accumulator

`decompile_function 0x4C394`: `[0x57A5C] += in_EAX`. Callers
(`get_xrefs_to 0x4C394`): `0x49FB1` (`MOV EAX,0x200`), `0x49519` inside the
match setup `0x493A0` (`MOV EAX,0x200`, FU-60 §2.1), and `0x4A255` in the match
reset `FUN_0004A228` (also `MOV EAX,0x200`, after `FUN_0004B380()==0x10`).
All three pass **`0x200` = 2.0 in Q8**; `FUN_0004C394` itself has no other
argument handling.

### 3.2 `FUN_0004B100` — world update head

Byte listing (`disassemble_bytes 0x4B100`, 32 instructions) + decompile:

```
0x4B106  CMP word [0x58822],0 / JZ 0x4B11A   ; match-over reason word; non-zero -> return 1
0x4B11A  AH = [0x57A4F] ^ 1; [0x57A4F] = AH  ; frame parity toggle
0x4B123  ECX = [0x57A5C]; EAX = [0x57A5C] >> 8; [0x57A5C] = ECX & 0xFF
0x4B143  CX = [0x57A66]; ECX += EAX; [0x57A64] = AX; [0x57A66] = CX
0x4B15E  CALL 0x78A54                        ; per-tick entity loop (delta in EAX)
0x4B163  if ([0x5872D] > 0) [0x5872D] -= delta
0x4B181  [0x4C195]=0; [0x4C110]=&DAT_0005774C; CALL 0x736AC  ; entity/coordinate chain
0x4B198  if [0x4C32A]!=0: CALL 0x948AC
0x4B1A6  uVar = FUN_0008AF38()               ; clock/period machine (§4)
0x4B1AB  switch (low word = [0x58822]): ==0 -> FUN_0008D8EC ×2 (+FUN_0007D430
         when phase==2 && [0x59056] && entity[+0x7B2]!=0); ==1 -> stop path;
         ==2 -> FUN_00053DC4/FUN_0004B678 path, [0x58822]=0, return;
         1<v<... -> FUN_000513EC/FUN_00053DC4 when [0x57AC2]<2, else
         FUN_00036BC0 when period is 2/3 (or 4 without [0x57AC0])
```

So `FUN_0004B100` owns: the match-over gate, the frame-parity toggle, the
Q8 split, the entity stack reset (`[0x4C110] = &DAT_0005774C`), the entity
chain, and the result dispatch of `FUN_0008AF38`. The clock itself is computed
before any of the phase logic, so delta/tick counters advance even when the
clock is stopped.

### 3.3 `FUN_00036208` — camera/coordinate step

`disassemble_bytes 0x36208` (first instructions quoted):

```
0x36211  if ([0x5FB4] < 0 || [0x5FC4] != 1) return
0x36234  EDX = EAX (arg); EDX >>= 4           ; 0x200 -> 0x20
0x36236  EAX = [0x5FB4]
0x3623E  CALL 0x4511D                         ; player [0x5FB4] direction state (FU-61 §5)
0x36243  low byte -> axis bits 0x1/0x2/0x4/0x8/0x10/0x20/0x40
0x3625C  ECX/EBX = ±0x20 per axis            ; per-frame pan = arg>>4
0x36290  [0x5FB8] += ECX; [0x5FC0] += EBX     ; camera accumulators
0x362A0  clamps, view class from FUN_0004B308(phase), ±0xA0 velocity clamp,
         FUN_0004B2F8() copies &DAT_0005774C[0..2] to [0x57A77/7B/7F]
```

So the `0x200` argument's meaning is the same Q8 frame step; `>>4` = `0x20` is
the pan increment, and the per-axis sign comes from the local player's input
state. View-class clamps: class 0/1 → ±0x9C0, 2..4 → ±0xB10, 5 → ±0x720/±0xB10,
6 → ±0x720/0xB10, 7 → ±0x720/0xB10, 8 → ±0x630/±0x9F0 (decompile).

## 4. Clock model

### 4.1 State block layout (offsets from the cited instructions)

| address | width | role (evidenced) |
|---|---|---|
| `0x57A4A` | dword | `>>24` = phase; low bytes 0x57A4A..0x57A4C belong to the adjacent `0x57A49` block |
| `0x57A4D` | byte | phase |
| `0x57A4E` | byte | previous phase |
| `0x57A4F` | byte | frame parity: toggled once per `FUN_0004B100` (`0x4B11A..0x4B129`) |
| `0x57A5C` | dword | Q8 frame-time accumulator: `+= step`; `>>8` = whole, `&0xFF` = fraction |
| `0x57A62` | dword | high word `0x57A64` is the per-frame delta (written `0x4B151`) |
| `0x57A64` | word | whole ticks produced this frame |
| `0x57A66` | word | cumulative ticks `+= delta` (`0x4B157`) |
| `0x57A68` | word | accumulated clock `+= delta` (`0x8AF8C`); zeroed by `FUN_0004C374` (`0x4C377`); only read by its own accumulation (open) |
| `0x57AB4` | word | **total match seconds** (`getter FUN_0004B594`), `++` per elapsed second, reset only at match reset |
| `0x57AB6` | word | **period seconds** (`getter FUN_0004B588`), `++` per elapsed second, zeroed at every period change |
| `0x57AC1` | byte | sub-second accumulator: `+= delta`; each `>= 0x3C` subtracts 60 and ticks the seconds counters |
| `0x57AC2` | byte | period index 0..8 (getter `FUN_0004B5F4`); 0/1 = regular halves, 2/3 = extra halves (`FUN_0004B564`), 4+ past that |
| `0x57ABA` | word | auxiliary counter used in the period-end condition (`sec == length + aux`); `++` per class-2 second |
| `0x57ABC` | word | auxiliary tick, saturates at 0x1E |
| `0x5881A` | word | regular half length in seconds; `= [0x4C1D1] * 0x3C` |
| `0x5881C` | word | extra half length in seconds; `= [0x4C1D1] * 0x14` |
| `0x58822` | word | match-over reason; `FUN_0004B100` returns early when non-zero |

### 4.2 Gate

`FUN_0008AF38 0x8AF4B..0x8AF80`:

```
class = table[phase]
advance = (class == 1) || (class == 2 && [0x4C302] == 0)
if (!advance) goto 0x8B590     ; tail only (no clock, no period logic)
```

`[0x4C302]` (dword) has no static writer (`get_xrefs_to 0x4C302` → exactly the
read at `0x8AF63`); it is a runtime/indirect setter — open leg. For the port it
is the `clock_halt` argument.

### 4.3 Second rollover

`0x8AF86..0x8B04A`:

```
AX = [0x57A64]                ; delta
[0x57A68] += AX
if (phase == 2) FUN_00089EB0(delta)
[0x57AC1] += (byte)delta
loop while [0x57AC1] >= 0x3C and completion flag == 0:
    if class == 2:
        limit = (period <= 1) ? [0x5881A] : ([0x57AC2] <= 3 ? [0x5881C] : stale)
        if (period_seconds + 0x14 < aux + limit):
            if (aux_tick == 0x1E) aux++; else aux_tick++;
        else aux++;
    [0x57AC1] -= 0x3C
    [0x57AB4]++; [0x57AB6]++
    period-end switch (case = period, 0..3, table 0x8AF28)
```

With the ported step, delta is always exactly 2 (`0x200 + r` with `r<0x100`
shifts to 2), so one rollover per 30 frames = 60 units/s. The class-2 aux
update runs **before** the second is counted, and the period-end case sees the
incremented seconds; both orders are replicated in the port.

### 4.4 Period-end conditions (cases 0..3)

Jump table at flat `0x8AF28` (encoded `0x7AF28`, `read_memory 0x8AF20`:
padding `8B C0` then `DD B1 07 00 / 2F B2 07 00 / E9 B2 07 00 / 3F B3 07 00`)
→ `+0x10000` = `0x8B1DD`, `0x8B22F`, `0x8B2E9`, `0x8B33F`:

| period | length | end condition (`sec` = `[0x57AB6]`, `aux` = `[0x57ABA]`) |
|---|---|---|
| 0 | `[0x5881A]` | `sec == len + aux` → complete; `sec == len && aux != 0` → `[0x5882D]=1` |
| 1 | `[0x5881A]` | `sec == len + aux` → complete; `sec == len && aux != 0` → `[0x5882D]=1` + `FUN_0008EF38` event 0x7B; `sec == len && aux == 0` → complete plus `[0x57AC5]`/`[0x57AC7]`/`[0x4C2EE]`/`[0x4C1D0]` side effects (`[0x57AC0]=1` or `[0xF378]=1`) |
| 2 | `[0x5881C]` | as period 0 |
| 3 | `[0x5881C]` | as period 1 |

`[0x5882D]` is a byte flag set by those cases (and cleared in the phase-0xC
arm); `[0x57AC0]` is a byte flag; `[0x4C2EE]`, `[0x4C1D0]`, `[0xF378]`,
`[0x57AC5]`, `[0x57AC7]` roles are not asserted (open).

### 4.5 Period rollover

When a case sets the completion flag, the loop exits and `0x8B3AE..0x8B58A`
runs:

```
0x8B3AE  if ([0x5882D] && [0x4C32A]==0) FUN_00054104(0)
0x8B3C7  if (completion == 0) goto tail 0x8B590
0x8B3D0  if (period == 1) -> event/pause branch (0x8B3E0..0x8B553):
             enqueue 0xB8/0xB9/0xBA/0xBB/0xBC, FUN_0004C2C0/FUN_00012230/
             FUN_00012250, EAX=0x12 or 0x13; falls through
         elif (period == 0): enqueue 0xC2; EAX=0x12
         else: fall through
0x8B56F  CALL 0x4C374                  ; [0x57A68] = 0
0x8B574  CALL 0x8B9CC                  ; phase transition (FUN_000740A0 funnel)
0x8B583  [0x57AB6] = 0                 ; period seconds
0x8B58A  [0x57AC2]++                   ; period
```

The tail `0x8B590..0x8B655` additionally: for periods 2/3 with
`[0x57AC5] != [0x57AC7]` and `[0x57AC0]==0`, sets `[0x57AC0]=1`, enqueues
`0xC0` with a team index, calls `FUN_0008B9CC` (skip the increment); for
period 4 with `[0x57AC0]!=0`, calls `FUN_0008B9CC` and `INC [0x57AC2]` (4→5);
otherwise dispatches `FUN_00088940` for phases 2/0x10 and clears
`[0x57ABA]`/`[0x5882D]` for phase 0xC; then `FUN_0008BAF0` when
`[0x4C32A]==0`, and the `[0x58808]` callback. `FUN_0004BA00` temporarily sets
`[0x57AC2]=8` around a `FUN_0004BE44` lookup when `period > 4`.

### 4.6 Start / reset / stop

* **Reset zero**: `FUN_00073EE0` zeroes `[0x57AB4]`, `[0x57AB6]`,
  `[0x57AC1]`, `[0x57ABA]`, `[0x57ABC]`, the `0x57AC3..` block and
  `[0x57AC2]`, then copies the 2×11 player tables from `0x4C360`/`0x4C3A0`/
  `0x4C3B0` to `0x57BD2`/`0x57B90`/`0x57BA6`. `FUN_00073E28` zeroes the phase
  bytes and `[0x57A5C]`/`[0x57A64]`/`[0x57A66]`/`[0x57A68]`. Both are called
  by `FUN_00074034` (`FUN_00073EE0` at `0x74089`), which also sets the camera
  block `[0x57A73]=&DAT_0005774C` and calls `FUN_000886D4`.
* **Period lengths**: `FUN_000886D4` sets `[0x5881A] = [0x4C1D1]*0x3C` and
  `[0x5881C] = [0x4C1D1]*0x14` (`[0x4C1D1]` = configured half length in
  minutes). `FUN_0004B508` overrides to `0x3C`/`0x1E` (60 s / 30 s) and is
  called from the match reset `FUN_0004A228` only when `[0x72F8]!=0`.
* **Kickoff/restart-ish reset**: `FUN_00092E2C` (sole caller `FUN_00092D8C`)
  calls `FUN_000361A4`/`FUN_00036200`, then `FUN_00074034`, `FUN_0007417C`,
  and re-zeroes `[0x57AB4]`, `[0x57AB6]`, `[0x57AC9]`, `[0x57AC7]`,
  `[0x57AC5]`, `[0x57AAB..]`, `[0x57AB1]`.
* **Stop**: `FUN_0004B100` returns immediately (before the clock split) while
  `[0x58822] != 0` (`0x4B106`); `FUN_0008BAF0` sets `[0x58822]` to 1 or 2 when
  a match-end condition fires (`0x8BC42`/`0x8BCxx` region, decompile quoted
  only for the writes). Separately the class-0 phases stop the clock without
  ending the match, and class 2 stops when `[0x4C302]!=0`.

## 5. Entity/coordinate updates and per-team timers

The `0x577xx`/`0x57Axx` entity/coordinate block is updated inside the frame
chain, not by a separate callee:

* `FUN_0004B100 0x4B193 CALL 0x736AC` → `FUN_000736AC` (`0x736AC..0x73Cxx`)
  calls `FUN_00072AC4` at `0x73756` (sole caller) and writes the camera
  shadow `[0x57758]/[0x5775C]/[0x57760]`; it also does the RNG-driven drift
  of `[0x577C0]` and the `[0x5774C]`/`[0x57750]`/`[0x57754]` integration.
* `FUN_00072AC4` (`sole caller FUN_000736AC`) reads the phase at `0x72B1E`,
  tracks the selected entity `[0x57A83]`/`[0x57A87]` and calls
  `FUN_00072478` at `0x72AE5` (sole caller) when the tracked entity changes;
  `FUN_00072478` writes the `0x575xx` tracking block and calls `FUN_00092040`
  (ambience per-team transition, FU-49 §1.10) at `0x7253B`/`0x7254A` and
  `0x725B5`/`0x725C4`.
* `FUN_00088940` (`sole caller 0x8B63E` in `FUN_0008AF38`) runs for
  phase 2/0x10 when `[0x5781D]!=0`: it updates the current entity pointer
  `[0x57A9F]` from `[0x577CE + team*4]`, calls `FUN_00092040(0)`/`(1)` when the
  player lookup yields 0, and `FUN_000651F0`/`FUN_000974F0`/`FUN_0008A938`.
* `FUN_00091F60` (per-team ambience deadline machine, FU-49 §1.10) is called
  by `FUN_00091DD8` at `0x91DEF` only when `[0x4C32A]==0`, i.e. inside the
  drained frame's presentation stage (FU-60 §4.4), not in the update step.

So the frame order for coordinate state is: Q8 clock split → entity/camera
chain `FUN_000736AC`→`FUN_00072AC4`→`FUN_00072478` → clock/period machine
`FUN_0008AF38` (which may re-enter the entity update `FUN_00088940`) →
per-entity `FUN_0008D8EC` → motion step `FUN_00036208` → presentation.

## 6. Port: `fifa96_match_state`

`include/fifa96_loader/fifa96_match_state.h` +
`src/fifa96_loader/fifa96_match_state.c` (caller-owned struct, no globals, no
comments, `-fifa96_err_t` for invalid arguments). Scope: the phase setter and
class table, the Q8 frame accumulator, the clock gate, the second rollover and
the period-end/rollover core. Not ported: phase-transition graph, `FUN_0008B9CC`
phase choice, match-over signalling, period-1/3 side effects, the class-2
aux semantic label.

| original | port |
|---|---|
| byte `0x57A4D` / previous `0x57A4E` | `phase` / `prev_phase`, `fifa96_match_state_set_phase` (stores both) |
| table flat `0x1106AD` (48 entries) | `fifa96_match_state_phase_class` (`>= 0x30` → 0, matching the zero tail) |
| `0x4B123..0x4B157` `acc += step; whole = acc>>8; acc &= 0xFF; delta; tick += delta` | `frame_acc`/`frame_delta`/`tick_total` in `fifa96_match_state_frame` |
| class gate `class==1 \|\| (class==2 && ![0x4C302])` | `clock_halt` argument |
| `0x8AFAC..0x8B04A` sub-second rollover | `second_acc`, `period_seconds`, `total_seconds` |
| class-2 aux update `0x8AFFA..0x8B04A` | `aux_seconds`/`aux_tick` (same two-branch arithmetic) |
| cases 0..3 (`0x8B1DD`/`0x8B22F`/`0x8B2E9`/`0x8B33F`) | completion check `sec == length + aux`, else flag `sec == length && aux != 0` → `aux_flag` (0x5882D) |
| `0x8B574..0x8B58A` `FUN_0008B9CC(); seconds=0; period++` | `fifa96_match_state_advance_period` and the in-frame rollover (phase choice omitted) |
| `[0x5881A]`/`[0x5881C]` | `period_length`/`extra_length` (caller supplies; from `[0x4C1D1]`×60/×20) |
| `FUN_0004C394(0x200)` + `FUN_0004B100` per drained frame | `FIFA96_MATCH_STATE_STEP` and one `..._frame` call per granted pace frame |
| pace grant (`FUN_00049320`) | `fifa96_match_state_tick` wraps `fifa96_match_pace_tick` and steps the clock on `granted` |
| period>3 tail, `[0x57AC0]`/`[0x57AC5]`/`[0x57AC7]` score compare, `FUN_0008B9CC`, `[0x58822]` | not modelled (open legs) |

Tests (`tests/test_match_state.c`, suite 50 → **51**): init zeroing; the whole
48-entry class table including the class-1 phases `{2,0x17,0x1A,0x1C,0x1D,0x1E}`
and the `>=0x30` tail; setter previous-phase handling; the Q8 split
(delta 2/frame, `frame_acc` fraction, `tick_total` 60 per 30 frames, one second
per 30 frames); small-step accumulation (0x10 × 16 = one tick) and a single
large step (0x3C00 → delta 0x3C, one rollover); class-0 held clock still
advances delta/tick; class-2 runs and `clock_halt` blocks; period end at
`length` and rollover to the next period with `total_seconds` preserved;
`aux` extending the end (`sec == length && aux != 0` sets `aux_flag`,
`sec == length + aux` completes); the class-2 aux update (`aux_tick` 1..30 then
`aux_seconds`); manual `advance_period`; pace wiring (3 ticks → no step, 4th →
one step); NULL handling for every entry point and a NULL `period_ended`.
ASan+UBSan build of `test_match_state` clean (`cc -fsanitize=address,undefined
-Iinclude tests/test_match_state.c src/fifa96_loader/fifa96_match_state.c
src/fifa96_loader/fifa96_match_pace.c`).

## 7. Errata (quoted)

* FU-60 §5 table row 4: "`FUN_0004B100` (+`FUN_0008AF38`→`FUN_00088940`),
  preceded by `FUN_0004C394(0x200)` → `[0x57A5C] += 0x200`" — **expanded**:
  `FUN_0004C394` is only the raw accumulator; the `>>8` split, the delta/tick
  counters and the parity toggle are the head of `FUN_0004B100`
  (`0x4B11A..0x4B15E`), and the clock gate/period machine is `FUN_0008AF38`
  called at `0x4B1A6`. `FUN_0004C394(0x200)` is also called by the setup
  `0x49519` and the reset `0x4A255`.
* FU-60 §3 "each hit sets `EDX = code` and falls to `0x49EB9`" — **corrected
  per chain**: chain A hits (`0x49D16/0x49D2E/0x49D46/0x49D5E/0x49D7A`) jump to
  `0x49EB9` and skip the rest; chain B hits (`0x49D92/0x49DAA/0x49DBF/0x49DD4/
  0x49DE9`) jump to `0x49DFE`, where phase `{0xB,0xF}` clears the selection and
  chain C may override it; chain C hits jump to `0x49EB9`. Also added that an
  active input event 2 with phase ∉ `{0xB,0xF}` skips the whole frame body.
* FU-60 §9 open leg "`FUN_00049B28`'s loop register contract (`ECX` …) relies
  on Watcom callee register behaviour" — **closed**: `ECX` is zeroed at
  `0x49BA2` and never written in the loop; `CMP ECX,[0x7328] / JL` is
  `0 < remaining`.
* FU-60 §3 "`0x49FD8 CALL 0x36C3C` … state 2/4 arm" — **confirmed by raw
  bytes** (`read_memory 0x49FC8`): `JNZ 0x49FE1`, then `FUN_00045D0D` and
  `FUN_0004B454`, with `[0x4C32A]==0` skipping to the tail.
* FU-60 §8: "`[0x57A4A]>>24` phase values 0..0x14 are observed only through
  the two gates here … no map of phase→situation is asserted" — **extended**:
  the phase class table at flat `0x1106AD` and the period-end jump table at
  flat `0x8AF28` are now read, the phase byte's only non-reset writer is
  `FUN_000740A0`, and the period lengths `[0x5881A]`/`[0x5881C]` are derived.
  No phase semantic label is asserted.
* FU-60 §5 clock row "`[0x57AB6]` frame/clock counter" — **refined**:
  `[0x57A66]` is the cumulative per-frame tick counter; `[0x57AB6]` is the
  per-period seconds counter and `[0x57AB4]` the total match seconds.
* Ghidra listing defect: `disassemble_function 0x8AF38` is byte-shifted in the
  period-end region (see Method); use `disassemble_bytes` from the case entries.

## 8. Open legs

* **Phase-transition graph**: 15 of the 18 `FUN_000740A0` call sites are
  unnamed/undefined in the `0x87xxx..0x94xxx` region; the phase chosen by
  `FUN_0008B9CC` (`[0x57AAC]>>24`, `[0x57ABE]`/`[0x57ABF]`, `[0x4C1D0]`,
  `FUN_00012230`/`FUN_00012250`) is not decomposed, so no transition table is
  asserted.
* **`[0x4C302]`** (class-2 clock halt): no static writer
  (`get_xrefs_to` → one read at `0x8AF63`).
* **`[0x58822]`** match-over word: writers are `FUN_0008BAF0` (`0x8BC42`
  region) and `FUN_0004B100` reads it; the exact end-of-match conditions were
  not derived.
* **`[0x57ABA]`/`[0x57ABC]` semantics**: the period-end arithmetic is exact,
  but no label ("injury time", "added time") is asserted; `[0x57ABC]`'s
  consumer beyond the saturation branch is not derived.
* **`[0x57AC5]`/`[0x57AC7]`**, `[0x57AC0]`, `[0x4C2EE]`, `[0x4C1D0]`,
  `[0xF378]`: read by the period-1/3 end blocks and the tail; roles not
  asserted.
* **`[0x57A68]`** is accumulated (`0x8AF8C`) and zeroed (`FUN_0004C374`,
  `0x4C377`) but has no static reader (`get_xrefs_to` count 2) — likely read
  through a pointer; undetermined.
* **`0x4C110`/`0x4C195`** (written `0x4B18D`/`0x4B188` before the entity
  chain) and `FUN_00044888`'s data references to `0x4B100` were not
  classified.
* **`FUN_0008AF38` period>3 behaviour and the tail** (`0x8B590..0x8B655`)
  beyond the cited flags is not ported.
* Object-base classification of each quoted global (FU-59 errata) was applied
  only to the three tables read as bytes (`0x1106AD`, `0x8AF28`, and
  FU-61's input tables); other addresses are quoted as Ghidra displays them.

## Provenance

Ghidra MCP on `/fifa96_le.bin`:
`decompile_function` 0x4B380, 0x4B454, 0x4C394, 0x4B100, 0x36208, 0x8AF38,
0x88940, 0x72478, 0x4A228, 0x72AC4, 0x4C374, 0x36200, 0x888FC, 0x4B564,
0x4B588, 0x4B5F4, 0x4B678, 0x4B6FC, 0x4B7D0, 0x4B818, 0x4B8D4, 0x4BA00,
0x4BE44, 0x4BF7C, 0x4B308, 0x4B3C0, 0x4B3E0, 0x4B2F8, 0x4B664, 0x4B594,
0x74034, 0x73EE0, 0x73E28, 0x740A0, 0x73D10, 0x73CD0, 0x7417C, 0x78A54,
0x948AC, 0x8D8EC, 0x8B9CC, 0x8BAF0, 0x8EF38, 0x92E2C, 0x4B508, 0x886D4,
0x736AC;
`disassemble_function` 0x49B28, 0x8AF38;
`disassemble_bytes` 0x8AF38, 0x4B100, 0x36208, 0x8B1D0, 0x8B22F, 0x4A230,
0x4B180;
`read_memory` 0x106AD, 0x206AD, 0x1106AD, 0x7AF28, 0x8AF20, 0x8B1D0, 0x49FC8,
0x8B1E0;
`search_instructions` operand `57a4a`, `57a4`, `57ac2`, `57ab6`, `57ab4`;
`get_xrefs_to` 0x57A4A, 0x57A66, 0x57AC2, 0x57A68, 0x740A0, 0x74034, 0x886D4,
0x4B508, 0x4C394, 0x36208, 0x72478, 0x88940, 0x4C302, 0x4B380, 0x4B100,
0x72AC4, 0x736AC, 0x4B588, 0x4B594, 0x92E2C;
`get_function_by_address` 0x8AF38; `get_current_program_info`.

Analysis-only: no tool, capture-rig, ISO or Ghidra-project change. Port write
set: `include/fifa96_loader/fifa96_match_state.h`,
`src/fifa96_loader/fifa96_match_state.c`, `tests/test_match_state.c`,
`CMakeLists.txt` (one library/test block). `make test`: 50/50 before, **51/51
after**; ASan+UBSan `test_match_state` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
