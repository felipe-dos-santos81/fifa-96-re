# FU-58: the top-level loop and mode state machine

Scoping slice for match logic. The data/audio/video/presentation layers are
ported (FU-1..57); match logic is the remaining major subsystem. This document
maps what sequences the game at the top level — the dispatcher, its 20-state
table, the front-end loop and the outer driver — and lays out bounded
candidate slices for the match work. No code was changed.

Result in one line: **the state dispatcher is `FUN_0001442c`, jumping through a
20-entry pointer table at `0x43dc` (states `0..0x13`, otherwise a generic event
pump); the table is populated at load time by the LE fixup stream and its 20
handlers are now statically mapped to `0x14451..0x14B28`; the front-end loop is
`FUN_0001E3A8` under an outer driver at `0x1DD40`, and the 100 Hz tick is the
INT-8 handler at `0x9F5E4` with an 8-slot callback table at `0x5BAE4`.**

## Method

* Static work used the open Ghidra MCP session on `fifa96_le.bin` (link-time
  flat LE addresses; runtime = link + `0x1FC010`, FU-4).
* The state table `0x43dc` is **all zero in the flat image**
  (`read_memory 0x43dc 80` returns 80 zero bytes; the only static xref is the
  dispatcher's own read at `0x14449`). The LE fixup stream that populates it is
  not part of the flat image, so `FIFA96.EXE` was extracted **read-only** from
  `game/FIFAPCCD96.iso` with `xorriso` (never mounted, never written):

  ```
  xorriso -osirrox on -indev game/FIFAPCCD96.iso -extract /FIFA96.EXE \
      /tmp/opencode/fu58/FIFA96.EXE
  # 1526315 bytes, md5 9a461768d610121c2bb5869a7c4cfc9d
  ```

* LE layout used (matches `tools/fifa96_le.py` and FU-4): header file
  `0x290A4`; fixup page table at `header + dword@+0x68` = `0x295EC`;
  fixup record table at `header + dword@+0x6C` = `0x29A0C`. Page `4`
  (which holds `0x43dc`) has records at file `0x2AAE7..0x2ADCE`
  (page-table entries `pt[4]=0x10DB`, `pt[5]=0x13C2`, relative to the record
  table). The parser written for this slice is `/tmp/opencode/fu58/lefix2.py`
  (not committed); its record rule for the state cells is below.
* Static-only checks are marked as such; all negative reachability results
  from FU-12..FU-18 are cross-referenced, not re-run.

## 1. The dispatcher: `FUN_0001442c`

`FUN_0001442c` (entry `0x1442C`; the table jump is the last instruction of the
small block at `0x14449`, and the `EAX > 0x13` case is a `JA 0x14B56` into the
shared tail at `0x14B56`):

```
mov  eax, [caller-set state]
...
jmp  dword ptr CS:[eax*4 + 0x43dc]     ; 0x14449: 2e ff 24 b5 dc 43 00 00
```

* For `EAX <= 0x13` control jumps through `0x43dc[EAX]`
  (decompile `FUN_0001442c`; byte quote from `disassemble_bytes 0x14430 512`).
* For `EAX > 0x13` it instead runs a generic event pump: `FUN_000659f8`,
  `FUN_00065cc0`, then a `FUN_00013f10`/`FUN_00018b18` loop until the event
  flag clears, then zeroes `0x46560` and `0x4f34` and calls `FUN_0009a16c`
  (decompile `FUN_0001442c`; the same pump tail appears at `0x14B56..`).
* There is **no global current-state variable**: the state is carried in
  `EAX` as a register argument. Every call site sets `EAX` and calls
  `FUN_0001442c`; e.g. `MOV EAX,0x11; CALL 0x1442c` at `0x1EB26/0x1EB2B`
  (byte-verified) or `CMP [0x49fbc],1; MOV EAX,8/9; CALL` at
  `0x2CFAB..0x2CFC0`.
* Calls nest: a handler entered through the table runs its screen/loop and
  returns to the instruction after the caller's `CALL 0x1442c`, which then
  performs the caller's own post-state logic (e.g. `FUN_0001E3A8` continues at
  `0x1EB30` with `CMP EBP,8` / `CMP EBP,0xb` and the `FUN_0001E3A8` event-8
  branch at `0x1EDDB..0x1EE19`). The state table is therefore a **screen /
  panel dispatcher**, not a flat game-mode loop.

## 2. State table `0x43dc`: 20 cells, decoded from the LE fixups

The fixup records for source page 4 contain exactly 20 records whose in-page
source offsets cover `0x3DC..0x428` — the 80 bytes `0x43DC..0x442B`, i.e. the
20-entry table. Observed record shape on this page (7 bytes per state cell):

```
07 00 <src u16 LE> 01 <target-object-1 offset u16 LE>
```

Example, from the dump at file `0x2AC8F` (quote of the raw record bytes):

```
07 00 dc 03 01 51 44     ->  cell 0x43DC (state 0) <- 0x10000 + 0x4451 = 0x14451
```

The same page also carries `07 10 ...` records with 4-byte object-4 targets
(e.g. `07 10 43 0f 04 44 2b 01 00`); the general record encoding beyond what
is needed for the 20 state cells was **not** fully decoded and is an open leg.
The interpretation above is validated by: (a) all 20 sources land on 4-byte
strides inside the known pointer table; (b) all 20 targets land in the code
object (base `0x10000`) in `0x14451..0x14B28`, and reading the first bytes at
each target shows every one begins a `PUSH`-immediate handler prologue
(`6a ff` at 16 targets, `6a 01` at 4: `0x14479`, `0x144D0`, `0x14554`,
`0x14A50`; raw-byte script read, `getBytes` × 12 at each target); (c)
`FUN_0001442c`'s jump indexes the same table with `EAX*4`.

| state | cell | handler | evidence / identity |
|------:|------|---------|---------------------|
| 0 | `0x43DC` | `0x14451` | fixup record `07 00 dc 03 01 51 44` (page-4 records `0x2AAE7..0x2ADCE`) |
| 1 | `0x43E0` | `0x144A8` | fixup record `07 00 e0 03 01 a8 44` |
| 2 | `0x43E4` | `0x14479` | fixup record `07 00 e4 03 01 79 44` |
| 3 | `0x43E8` | `0x144D0` | fixup record `07 00 e8 03 01 d0 44` |
| 4 | `0x43EC` | `0x144FF` | fixup record `07 00 ec 03 01 ff 44` |
| 5 | `0x43F0` | `0x14554` | fixup record `07 00 f0 03 01 54 45` |
| 6 | `0x43F4` | `0x145A9` | fixup record `07 00 f4 03 01 a9 45` |
| 7 | `0x43F8` | `0x14648` | fixup record `07 00 f8 03 01 48 46`; setter `0x2D067` (`ESI==2`) |
| 8 | `0x43FC` | `0x146DC` | fixup record `07 00 fc 03 01 dc 46`; setter `0x2CFAB` (`[0x49fbc]==1`), `0x2D067` (`ESI==0`) |
| 9 | `0x4400` | `0x1470E` | fixup record `07 00 00 04 01 0e 47`; setter `0x2CFAB` (else), `0x2D067` (else) |
| 10 | `0x4404` | `0x14742` | fixup record `07 00 04 04 01 42 47`; setter `0x2D96D` (after `FUN_0002D1BC`/`FUN_0002D2F8`/`0x14C90`) |
| 11 | `0x4408` | `0x14798` | fixup record `07 00 08 04 01 98 47`; setter `0x2DA11` (after `0x14BB0`) |
| 12 | `0x440C` | `0x147EE` | fixup record `07 00 0c 04 01 ee 47`; FU-17: site-7 caller `0x27B1A` sets state `0x12` when `[0x5094]==0` |
| 13 | `0x4410` | `0x14882` | fixup record `07 00 10 04 01 82 48` |
| 14 | `0x4414` | `0x1491F` | fixup record `07 00 14 04 01 1f 49` |
| 15 | `0x4418` | `0x149B3` | fixup record `07 00 18 04 01 b3 49` |
| 16 | `0x441C` | `0x14A50` | fixup record `07 00 1c 04 01 50 4a`; set at `0x1EDF8`/`0x1EDFD` after `0x14BB0` and `[0x5440]=0` (and by `FUN_0001F5CC`'s tail per decompile) |
| 17 | `0x4420` | `0x14AAA` | fixup record `07 00 20 04 01 aa 4a`; front-end entry set at `0x1EB26`/`0x1EB2B`, `0x1F8E4`/`0x1F8E9` |
| 18 | `0x4424` | `0x14B01` | fixup record `07 00 24 04 01 01 4b` |
| 19 | `0x4428` | `0x14B28` | fixup record `07 00 28 04 01 28 4b` |

Handler shape (static): each target opens a `PUSH`-immediate parameter block
(screen dimensions and id-like selectors) that lands on the shared tail at
`0x14B4F` (`CALL FUN_00013bd8`) or enters the event pump at `0x14B56`.
Examples: state 16 (`0x14A50`) pushes `0x640`, `0x65`, `0x1E0`, `0x76`;
state 17 (`0x14AAA`) pushes `-0x640`, `0x1E0`, `0x65`, `0x76`; states 18/19
(`0x14B01`, `0x14B28`) push `0x1E0`, `0x640` and selector `0x79`
(disassemble `0x14A40..0x14B56`). **The meaning of those selectors and the
per-state screen identity is not evidenced here** — see open legs.
FU-16/FU-17 evidence ties modes {8,9} to `[0x5524]` and {10,11} to `[0x5528]`
in the competition renderer `0x26B45`, and the FU-17 setter notes tie states
10/11 to the competition module (`0x25xxx..0x2Dxxx`); no such tying exists for
the other 16 states in this slice.

## 3. Top-level loops

### 3.1 Outer driver `0x1DD40` (undefined function in Ghidra)

Prologue `PUSH EBX/ECX/EDX/ESI/EDI/EBP` at `0x1DD40` (disassembly); no static
xrefs were resolved to it (its entry is not in any defined function and no
direct call was recovered), so its own caller is an open leg. Body
(disassemble `0x1DD40..0x1DDC5`):

* Copies the 0x68-byte settings block `0x49278 -> 0x49210`
  (`0x1DD5A..0x1DD69`), then sets `EBP = 0`, or `EBP = 2` when
  `[0x5094] != 0` (`0x1DD6F..0x1DD77`).
* `EBP == 0`: `CALL FUN_0001E3A8` (the front-end loop, `0x1DD87`, its only
  caller). Return 8 maps to `EBP = 1`; returns 10/11 map to `EBP = 2`
  (`0x1DD8C..0x1DDA7`).
* `EBP == 1`: `CALL FUN_0001F5CC` (`0x1DDA9`, its only caller).
* `EBP == 2`: `MOV EAX,1; CALL FUN_0001B5B4` and return (`0x1DDB5`).

So the driver alternates front-end loop and the `FUN_0001F5CC` loop, and exits
through `FUN_0001B5B4(1)` when the front end returns 10/11 (or when `[0x5094]`
was already set).

### 3.2 Front-end loop `FUN_0001E3A8` (entry `0x1E3A8`)

Note on the boundary: Ghidra's auto-analysis body for `FUN_0001E3A8` is
truncated at `0x1E60E` (no return there; the raw instruction stream continues)
and the decompiler's body runs on to the `0x1EE00` region. The addresses below
are all byte-verified independently, so the analysis does not depend on that
boundary. Setup loads the front-end VIV tables and UI sprites
(`FUN_00013888` ×4, a run of `FUN_00015960` sprite/UI setups, `FUN_0001E154`;
decompile `FUN_0001E3A8`), then:

* `if ([0x5440] == 0) { FUN_00014c90([0x47c30], [0x47dc0]); EAX = 0x11;
  CALL 0x1442c; } else FUN_0001a2a0();` — the front-end's initial state is
  **17** (`MOV EAX,0x11` at `0x1EB26`, `CALL 0x1442c` at `0x1EB2B`).
* `(*[0x12afc])()` at `0x1EB30`, then the frame/input loop:
  `FUN_00065920`, `FUN_000125c4`, `FUN_00016350` (input), plus
  `FUN_0001DA58`/`FUN_0001DFB8`/`FUN_0001DF10`/`FUN_0001DDC8`/`FUN_0001DD20`
  for the menu sub-flows. Loop exits when the input code is 8, 10 or 11
  (decompile `FUN_0001E3A8`; tail disassembly at `0x1EDB0..0x1EE19`).
* Event `-10` (0xFFFFFFF6): `FUN_0001771c(); if (FUN_0001d26c(...) == 1) {
  iVar2 = 10; [0x5094] = 1; }` (decompile `FUN_0001E3A8`) — the
  `FUN_0001D26C` confirm gate (39 xrefs) coupled to the `[0x5094]` flag.
* Exit code 8: `[0x5440] = 0; 0x14BB0(); EAX = 0x10; CALL 0x1442c;`
  (`MOV [0x5440],0` at `0x1EDED`, `MOV EAX,0x10` at `0x1EDF8`, call at
  `0x1EDFD`) — event 8 enters **state 16**.
* Other codes run `FUN_0001ADF8` + `FUN_0001A280` (the cleanup pair also used
  by the dispatcher's >0x13 path).

### 3.3 Second loop `FUN_0001F5CC`

Called by the outer driver when the front-end returns 8. It sets up a VIV
screen, enters **state 17** (`MOV EAX,0x11` at `0x1F8E4`, `CALL 0x1442c` at
`0x1F8E9`, byte-verified), then loops on `FUN_00065920`/`FUN_000125c4`/
`FUN_00016350` until the input code is 4 or 5 (decompile `FUN_0001F5CC`).
Tail: `if ([0x5094] == 0) { func_0x00014bb0(); FUN_0001442c(); }
else FUN_0001a280();` (decompile; the `0x14BB0` setup is the same one used for
state 16). FU-16/FU-17's keyboard flows reach the front end through this
family (`MOD5` routes); site 7 was never reached in any probe run
(FU-12/14/16/17/18).

## 4. Competition setters (modes 7..11), byte-verified

All four write `EAX` then `CALL 0x1442c`; `disassemble_bytes` this slice:

| site | code (abridged) | state |
|------|------------------|-------|
| `0x2CFAB` | `CMP [0x49fbc],1; JNZ +; MOV EAX,8; JMP` / else `MOV EAX,9`; call `0x2CFC0` | 8 or 9 |
| `0x2D067` | `CMP ESI,2; MOV EAX,7` / `TEST ESI,ESI; MOV EAX,8` / else `MOV EAX,9`; call `0x2D083` | 7/8/9 |
| `0x2D955` | `FUN_0002D1BC`; `FUN_0002D2F8`; `0x14C90`; `MOV EAX,0xA`; call `0x2D972` | 10 |
| `0x2DA0C` | `0x14BB0`; `MOV EAX,0xB`; call `0x2DA16` | 11 |

The state-10 handler (mode 10) is the FU-17 block `0x2D8F7..0x2D977`
(competition helpers `0x2D1BC`/`0x2D2F8`, VIV `0x14C90`, then state 10) and
the site-7 call sites are `0x25D4F..0x25D64` (`EBX ∈ {10,11}` -> `0x14C18`,
then state `0x10`) and `0x27B1A..0x27B42` (`ESI == 10` -> `0x14C18`, state
`0x12` when `[0x5094] == 0`). Byte-verified at `0x25D4F..0x25D7B` this slice
(`CMP EBX,0xa`, `CMP EBX,0xb`, VIV `[0x47c30]`/`[0x47dc0]`, `CALL 0x14C18`,
`MOV EAX,0x10`, `CALL 0x1442c`).

## 5. Tick machinery (100 Hz)

* INT-8 handler `0x9F5E4..0x9F64B` (disassemble): `PUSHAD`/segment saves;
  increments `[0x12e88]` (`MOV EDX,[0x12e88]` at `0x9F5F5`, `INC EDX` at
  `0x9F5FB`); on a divisor hit increments `[0x12e8c]` and chains the original
  vector with `CALLF [0x12e90]` (`0x9F612..0x9F619`); then walks the 8-slot
  callback table `0x5BAE4` by 4 bytes and calls every non-null entry
  (`0x9F62B..0x9F642`); `IRETD` at `0x9F64B`.
* `0x9F64C` registers a callback in the first empty slot; `0x9F684` clears a
  slot (disassemble; xrefs to `0x5BAE4` are exactly these two functions plus
  the handler).
* Installer `FUN_0009F754` (0x9F754..0x9F7CD) saves the old vector into
  `[0x12e90]`/`[0x12e94]`, writes PIT divisor `0x2E9C` (`out 0x40,0x9C` /
  `out 0x40,0x2E`), i.e. 1,193,180 / 11,932 ≈ **100 Hz**, and zeroes
  `[0x12e8c]`. (FU-37/FU-48 already use this tick as the pacing base.)
* `FUN_000CB2A4` is the global time getter (`return [0x12e88]`); `0x12e88`
  has 10 xrefs — the getter family `FUN_000CB2A4/AA/B6/D1/E1/EF/FF` (nine
  reads, with `FF` reading twice) and the handler read, plus one writer
  `FUN_000CB2CB`.
* Callbacks registered at startup by: `FUN_00094A48` -> `LAB_00085B03`
  (audio stream service), `FUN_0006A2AB` -> `0x5A0B2` (device/CD path, gated
  by `[0xde40]`), `FUN_0006D742` -> `[0x5D296]` (gated by `[0x5756c]`),
  `FUN_000A6265` -> `LAB_000A6A8F` (mode table at `0x14830`, index
  `[0x15fc8]`). Decompiles of those four registrars are the evidence.

## 6. Match-adjacent anchors verified this slice

| anchor | role evidence |
|--------|----------------|
| `FUN_00049B28` (0x49B28) | large gameplay loop; called from `0x496D5` (same undefined 0x49xxx module, alongside `FUN_0004A178`/`FUN_0006400C`); calls `FUN_00091DD8` unconditionally in its event path |
| `FUN_00091DD8` (0x91DD8) | exactly one caller: `FUN_00049B28` (xref `0x49FCA`); calls `FUN_000652B8` (music start) and `FUN_00091F60`, and reads `[0x57ab6]`, `[0x5b35c]` |
| `FUN_00091F60` (0x91F60) | two-slot per-team deadline state machine: loops `iVar5 = 0..1` over `[0x5a980 + 4i]` (deadline) and `[0x5a990 + 4i]` (active flag), calls `FUN_00092040`/`FUN_000920C0` |
| `FUN_00088940` (0x88940) | entity/selection update on `[0x577ca]`/`[0x57a4d]`/`[0x57784]`; single caller `FUN_0008AF38` (0x8B63E) |
| `FUN_00072478` (0x72478) | coordinate/possession-style update on `[0x577ca]`, `[0x57a83]`, `[0x57acc]` with the `0x5774c`/`0x57750` coordinate offsets; single caller `FUN_00072AC4` (0x72AE5) |
| settings | `FUN_0001DCAC` loads 0x68 bytes from `FUN_00068F30`/`FUN_00069338` into `0x49278`; `FUN_0001DE94` zeroes it and installs defaults; `0x49278` is copied to `0x49210` by the outer driver (match settings hand-off) |
| audio init | `FUN_00064F70` called from `FUN_000679F4`; the latter from `FUN_00067FE4`/`0x68108`/`0x68194`/`0x6847C` (0x67xxx audio system init) |
| music start | `FUN_000652B8` (caller `FUN_00091DD8`) and the CRD region `FUN_00065160` (0 xrefs statically — open leg, FU-53) |

## 7. Roadmap: bounded candidate slices for match logic

Each slice is bounded to a named entry function (or two) and one question, so
it can be closed with a static derivation plus, if needed, a targeted probe.

1. **Tick and callback consumers** — anchors `0x9F5E4`, `0x12E88`, `0x5BAE4`,
   `0x9F64C`/`0x9F684`, registrars `FUN_00094A48`/`FUN_0006A2AB`/
   `FUN_0006D742`/`FUN_000A6265`. Bounded: 8 fixed slots. Dependencies: none
   (audio is ported). Deliverable: per-slot owner/cadence map. *(Do this
   first — slices 2/4/6 use `FUN_000CB2A4` time.)*
2. **Match loop and frame body** — `FUN_00049B28` (call site `0x496D5`),
   `FUN_00091DD8`, `FUN_00091F60`. Bounded: one loop + two helpers. Deps:
   slice 1. Deliverable: loop order, per-frame calls, event/exit conditions.
3. **Front-end loop and dispatch close-out** — `0x1DD40` (caller unknown),
   `FUN_0001E3A8`, `FUN_0001F5CC`, `FUN_0001442C` + `0x43DC`. Bounded: the
   three functions plus the 20-cell table already decoded here. Deps:
   none. Deliverable: event-code map (8/10/11/`-10`), `[0x5094]`/`[0x5440]`
   lifecycles, driver caller.
4. **Competition screens (states 7..11)** — setters `0x2CFAB`/`0x2D067`/
   `0x2D96D`/`0x2DA11`, handlers `0x14648`/`0x146DC`/`0x1470E`/`0x14742`/
   `0x14798`, site-7 gates `0x25D4F`/`0x27B1A`. Bounded: the documented
   competition module. Deps: slice 3. Deliverable: state→screen identity for
   7..11 with VIV ids, and the `[0x49fbc]`/`[0x5524]`/`[0x5528]` semantics.
5. **Entity/coordinate update** — `FUN_00072478`, caller `FUN_00072AC4`, and
   `FUN_00088940`, caller `FUN_0008AF38`; the `0x577xx`/`0x57Axx` globals.
   Bounded: two call chains. Deps: slices 1/2. Deliverable: the first
   structure definitions around `[0x577ca]` and the update order.
6. **Per-team timers / event machine** — `FUN_00091F60` and
   `FUN_00092040`/`FUN_000920C0`, array `0x5A980`/`0x5A990`; who writes the
   deadlines and flags. Bounded: 2 slots. Deps: slice 1/2. Deliverable: the
   event semantics (substitution/booking/celebration candidates — not
   asserted until evidenced).
7. **Input and in-match menus** — `FUN_00016350` (event codes), `FUN_0001D26C`
   (confirm gate), `FUN_00016350`'s callers in the match module. Bounded:
   the input funnel. Deps: slice 3. Deliverable: key→event-code table and the
   in-match pause/options path (FU-16/FU-17 frame evidence exists to check).
8. **Match settings hand-off** — `FUN_0001DCAC`/`FUN_0001DE94`, blocks
   `0x49278`/`0x49210`, and the outer driver's copy at `0x1DD5A`. Bounded:
   one 0x68-byte record. Deps: slice 3. Deliverable: field map of the
   settings record as consumed by match code.
9. **Match start path (open)** — no anchor yet: the caller chain into the
   `0x49xxx` module (call site `0x496D5`) and the `FUN_0006400C` gate are the
   starting points. Deps: slices 2/3. Deliverable: the chain from front-end
   input (event 8 / `[0x5094]`) to `FUN_00049B28`.

Not recommended yet (no static anchor recovered in this slice): replay
machinery, ball physics as a separate slice, AI decision tables — nothing in
the reachable static picture separates them from slice 2/5/6 yet.

## 8. Open legs

* **States 0-6, 12-15, 18, 19 are unidentified.** Only the handler addresses
  are proven (fixup records); no citation ties them to a screen or label. Do
  not name them "menu"/"match" without a future derivation.
* **Fixup record format is only decoded for the state cells.** The 20
  mappings are exact; the `07 10` object-4 records on the same page and other
  pages were not decoded. A general LE fixup decoder would let future slices
  recover every runtime table (audio/music tables too).
* **Driver caller unknown.** `0x1DD40` has no resolved xref; the chain from
  the Watcom startup (`0x9FD88`, FU-4) to it is untraced.
* **Event-code semantics are partial.** 8/10/11 are proven to control the
  front-end loop; `-10` is coupled to `FUN_0001D26C`; the rest of
  `FUN_00016350`'s code space is unread.
* **`[0x5440]`, `[0x5094]`, `[0x49fbc]` are only partially evidenced** (flags
  written/read, not their full meaning).
* **Screen identity for states 16/17 is not asserted** — they are the
  front-end entry states (`0x14A50`/`0x14AAA`) per the call sites, but the
  selector ids (`0x65`, `0x76`, `0x79`, `0x21B`, ...) are not bound to assets
  here.
* `FUN_00065160` (CRD load region) has no static xref; its caller is unknown
  (FU-53).
* `FUN_0001F5CC`'s tail bytes could not be cleanly line-decoded (Ghidra flow
  split at `0x1F9C8`); the claim rests on the decompile of the function.

## Provenance

Extraction and fixup decode (read-only on the ISO; `/tmp` outputs not
committed):

```
xorriso -osirrox on -indev game/FIFAPCCD96.iso -extract /FIFA96.EXE \
    /tmp/opencode/fu58/FIFA96.EXE          # md5 9a461768d610121c2bb5869a7c4cfc9d
python3 /tmp/opencode/fu58/lefix.py        # page-table + raw records for page 4
python3 /tmp/opencode/fu58/lefix2.py       # 20-record state-table decode
```

Ghidra MCP calls on `fifa96_le.bin` (open project `fifa96`):
`decompile_function` 0x1442C, 0x14C90, 0x13BD8, 0x1A280, 0x1E3A8, 0x1DF10,
0x1DD20, 0x1DDC8, 0x1E154, 0x16350, 0x1DCAC, 0x1DE94, 0x1B5B4, 0x49B28,
0x91DD8, 0x91F60, 0x88940, 0x72478, 0x94A48, 0x6A2AB, 0x6D742, 0xA6265,
0xCB2A4, 0x9F754, 0x14BB0/0x14C90 disassembly; `disassemble_bytes` 0x14430,
0x1429C0, 0x1E8E0, 0x1F8C0, 0x1F9AC, 0x1F9C2, 0x1DD40, 0x2B0C0, 0x2B150,
0x2CF90, 0x2D060, 0x2D940, 0x2D9F0, 0x49690, 0x14A40, 0x25D4F, 0x9F5E4,
0x9FD88; `get_xrefs_to` 0x1442C, 0x43DC, 0x12E88, 0x5BAE4, 0x9F64C, 0x9F684,
0x9F754, 0x1E154, 0x1E3A8, 0x1F5CC, 0x1DF10, 0x1EF14, 0x1DD40, 0x49B28,
0x88940, 0x72478, 0x91DD8, 0x91F60, 0x679F4, 0x65160;
`search_functions_enhanced` name patterns `FUN_0001d`/`FUN_0001e`/`FUN_0002c`/
`FUN_0002d`; `read_memory` 0x43DC (80 zero bytes) and 0x1429E8 (pointer
table); `get_function_by_address` 0x14451/0x146DC/0x14742/0x1DD40 (all
undefined — handlers sit in the analysis gap named in §2).

`game/FIFAPCCD96.iso` was only read (xorriso `-osirrox on`, no mount, no
write); `FIFA96.EXE` was extracted to `/tmp`, not into the repo. No source,
tool, test or fixture changed: `make test` 47/47 before and after (no code
changed); `git status` shows only `fifa96.rep/` churn plus this document.
