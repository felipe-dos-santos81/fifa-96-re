# FU-76: the per-record action-handler table and the kick/pass path

Follow-on to FU-74/FU-75 (S6/S7/S8): enumerate the action table flat
`0x1106E0`, classify its handlers from their bodies, and derive the controlled
outfield locomotion and the ball-kick/pass math (`FUN_0007B9C4`,
`FUN_0007AE70`, `FUN_000CD474`). Ports the clean pieces as
`fifa96_action_handlers`.

Result in one line: **the table is one 80-dword array at flat
`0x1106E0..0x11081F` whose sole reader is `FUN_0007D9A4` (`EAX = code << 2;
ADD EAX,0x106E0`, runtime target = stored + `0x10000`); direct install sites
prove codes `0x00..0x2A` in use (the highest, `0x2A`, at `0x8D807`), slots
`0x00..0x2C` are action-shaped (called with `EAX = rec` only) while slots
`0x2D..0x4F` consume `EDX` as a second argument and are a different dispatch
family (open leg); code `0` (`0x7DB10`) is the generic outfield locomotion —
it counts `+0x89` down, calls `FUN_00079C20` (out target = position + dir<<7,
clamped `x∈[-0x720,0x720]`, `z∈[-0xB10,0xB10]` by `FUN_0007D3E4`), then
installs `3` when `+0x8D != 0` else `0x19`; code `7` (`0x814B0`) is the
two-stage kick action that calls `FUN_0007B9C4`; `FUN_0007AE70` resolves a
10-byte event row (`row[0]` selects a commentary arm; `row[+2]`/`row[+4]` are
the x clamp bounds, `row[+6]` the trajectory increment, `row[+8]` a divisor)
and `FUN_0007B9C4` clamps the ball x into those bounds (1.5× when the user
range bit and event 1/3 or mode `0x30`), adds the trajectory, caps it at
`0x460`, and finally stages the event through `FUN_0007A490`; `FUN_000CD474`
is a 10-bit atan approximation over the object-4 table `0x14072C`.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (FU-4 linear
  image; all instructions read back this slice with `disassemble_function`,
  `disassemble_bytes`, `read_memory` and two table-enumeration scripts). The
  decompiler is not used for any quote.
* **Address mapping (FU-57, restated; all three cases used below).**
  * code/function addresses equal true link addresses (object 1 base `0x10000`
    is already in the image; e.g. `FUN_0007D9A4` at `0x7D9A4`);
  * **object-4 rule**: a data immediate rendered by Ghidra as `A` has true
    link address `A + 0x100000` (the bytes at `A` are object-1 code); e.g. the
    table immediate `0x106E0` → flat `0x1106E0`, the atan table `0x4072C` is
    really flat `0x14072C`;
  * **stored code pointers** inside the tables are object-1 CS-relative and
    resolve through `+0x10000` (action table target = stored + `0x10000`);
    **inline jump tables inside object-1 code** use the same `+0x10000`
    (e.g. `JMP dword CS:[EAX*4 + 0x6AE38]` reads flat `0x7AE38`, not
    `0x16AE38` — this corrects FU-73).
* **Tooling errata.** `disassemble_function 0x7B9C4` dropped the block
  `0x7BA31..0x7BAE1`; it was recovered with `disassemble_bytes` and is quoted
  below. The action entry points are not auto-functions (they are reachable
  only through the offset table), so functions were created at the 40 runtime
  targets and bodies dumped to a local file for classification; bodies reached
  only through inline `CS:` tables are incomplete in the Ghidra listings and
  are marked as such.
* Every numeric claim is quoted from the listings; unproven items are open
  legs (no guessed labels).

## 1. The table and its reader

### 1.1 Reader and index arithmetic

`FUN_0007D9A4` (73 instructions, `0x7D9A4..0x7DAB2`) is the only install point
(FU-74 §2). The table read is:

```
0x7DA63  [rec+0x91] = (byte)code
0x7DA6D  EAX = code; EAX <<= 2
0x7DA77  ADD EAX,0x106e0          ; flat 0x1106E0
0x7DA7E  action = [EAX]
0x7DA8F  [rec+0x18] = action
```

`search_instructions` operand `0x106e0` returns exactly this one instruction
(`{0x7DA77}`), so no other static reader exists. `FUN_0007D9A4` also rejects
`+0x9A`, suppresses a repeat of `[rec+0x91]`, coerces code 3 → `0x19` when
`+0x8D == 0`, and flips the `+0x9F` bit 0 for codes 5/0x21 (FU-74 §2).

### 1.2 Bounds

`read_memory 0x1106E0` (160 B) and a script enumeration of 80 dwords give the
pointer array; the block above and below it is occupied by other data:

| region | content | evidence |
|---|---|---|
| `0x110680..0x1106AC` | type-flag byte table (FU-74 §5) | `0x7CC9C`, `0x784DC` |
| `0x1106AD` | phase-indexed byte table (FU-75 §2) | `0x8AF4B` |
| **`0x1106E0..0x11081F`** | **action table, 80 dwords (`0x00..0x4F`)** | this slice: all values read + resolved |
| `0x110820..` | outfield pressed/released rows `{mask 0x07FF, want, handler}` | `read_memory 0x110820` = `ff 07 60 00 38 ce 06 00 …`; FU-75 §1.3 |

The 80th slot `0x11081C` stores `0x7B900` (runtime `0x8B900`, a clean
prologue); the next dword `0x110820` is already the first row `{0x07FF,0x60,
0x6CE38}`. So the array ends at flat `0x110820`. (The earlier FU-74 claim of
40 entries stops at `0x110780`, which is in fact slot `0x28`.)

### 1.3 Installed-code census

Every `CALL 0x7D9A4` site in the image was scanned (74 sites) and the
preceding code immediate recovered. Codes installed by direct immediates:
`0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0E,0x0F,0x10,
0x11,0x12,0x13,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1F,0x20,0x21,0x22,0x23,
0x24,0x2A`; the remaining 9 sites are computed (reset `0x7DAB4` installs the
kept/`0x19`/`3` alternatives; keeper `0x78576` installs `0`/`0x19`/`4`).
**The highest installed code is `0x2A`**, at `0x8D807`:

```
0x8D7D4  EDX=1; EAX=team+0xB2
0x8D7E1  loop: if ([EAX+0x9A]==0) break; EDX++; EAX+=0xB2; while (DX<0xB)
0x8D7F8  EDX=0x2A; ECX=0; EBX=0
0x8D801  [team+0x831] = EAX
0x8D807  CALL 0x7D9A4
```

so slots past `0x28` are addressable actions. Representative sites per code
(address of each `CALL`): 3 `0x79AAC/0x7CA48/0x8753C/0x8994C`; 4 `0x766B4/
0x7E05A/0x7E85C/0x8025E/0x81722/0x844E7/0x85383/0x85CA1/0x86436/0x8D291`;
5 `0x75B67/0x7F133`; 6 `0x7F120`; 7 `0x6DCBC/0x7D149/0x7E5C9/0x7EF33/0x7F928/
0x7F970/0x80190`; 8 `0x7CD24/0x7CF05`; 9 `0x7CFB2`; 0x0A `0x7CE1A`;
0x0B `0x7CE93/0x7ED6A`; 0x0C `0x81BEB`; 0x0E `0x7E7B2/0x82F70`;
0x0F `0x7EB5C/0x83145`; 0x19 `0x77719/0x7E8CA/0x8990A/0x8CF3F`;
0x21 `0x7D046`; 0x23 `0x7D1B9`; 0x2A `0x8D807` (full list in the provenance
dump).

### 1.4 The complete slot table

Stored value read at `0x1106E0 + code*4`; runtime target = stored + `0x10000`
(prologues verified at the runtime addresses: e.g. code `19` stored `0x646E4`
is `c4 9a 00 00 …` while `0x746E4` is `53 51 52 56 57 55 83 ec 4c …`; code
`2E` stored `0x5E1D0` is `8b 14 95 …` while `0x6E1D0` is a clean prologue).

| code | stored | runtime | code | stored | runtime |
|---|---|---|---|---|---|
| 00 | `06DB10` | `07DB10` | 28 | `0770E8` | `0870E8` |
| 01 | `06DBC0` | `07DBC0` | 29 | `0774E4` | `0874E4` |
| 02 | `06DFCC` | `07DFCC` | 2A | `076A34` | `086A34` |
| 03 | `06E1A4` | `07E1A4` | 2B | `077738` | `087738` |
| 04 | `06E7C8` | `07E7C8` | 2C | `074598` | `084598` |
| 05 | `06F194` | `07F194` | 2D | `05DE34` | `06DE34` |
| 06 | `0701B4` | `0801B4` | 2E | `05E1D0` | `06E1D0` |
| 07 | `0714B0` | `0814B0` | 2F | `05DCC8` | `06DCC8` |
| 08 | `071068` | `081068` | 30 | `05DE44` | `06DE44` |
| 09 | `070A00` | `080A00` | 31 | `05DE44` | `06DE44` |
| 0A | `071738` | `081738` | 32 | `05E05C` | `06E05C` |
| 0B | `071908` | `081908` | 33 | `05DD9C` | `06DD9C` |
| 0C | `071C90` | `081C90` | 34 | `05DE44` | `06DE44` |
| 0D | `07251C` | `08251C` | 35 | `05DD6C` | `06DD6C` |
| 0E | `072710` | `082710` | 36 | `05DD6C` | `06DD6C` |
| 0F | `072AD0` | `082AD0` | 37 | `05DE34` | `06DE34` |
| 10 | `0755F0` | `0855F0` | 38 | `05DE34` | `06DE34` |
| 11 | `075DE4` | `085DE4` | 39 | `05DF4C` | `06DF4C` |
| 12 | `073D68` | `083D68` | 3A | `05DE34` | `06DE34` |
| 13 | `074B00` | `084B00` | 3B | `05DE34` | `06DE34` |
| 14 | `07784C` | `08784C` | 3C | `05DE34` | `06DE34` |
| 15 | `077CD0` | `087CD0` | 3D | `05E004` | `06E004` |
| 16 | `074630` | `084630` | 3E | `05E1C8` | `06E1C8` |
| 17 | `074730` | `084730` | 3F | `05E1D0` | `06E1D0` |
| 18 | `0749B0` | `0849B0` | 40 | `05E244` | `06E244` |
| 19 | `0646E4` | `0746E4` | 41 | `05E244` | `06E244` |
| 1A | `06662C` | `07662C` | 42 | `05DCC8` | `06DCC8` |
| 1B | `066D28` | `076D28` | 43 | `000000` | `010000` (INT3) |
| 1C | `067728` | `077728` | 44 | `078DC8` | `088DC8` |
| 1D | `064EB0` | `074EB0` | 45 | `07922C` | `08922C` |
| 1E | `06550C` | `07550C` | 46 | `079FA4` | `089FA4` |
| 1F | `066380` | `076380` | 47 | `079620` | `089620` |
| 20 | `074EEC` | `084EEC` | 48 | `0790EC` | `0890EC` |
| 21 | `075214` | `085214` | 49 | `079110` | `089110` |
| 22 | `07539C` | `08539C` | 4A | `079868` | `089868` |
| 23 | `072F84` | `082F84` | 4B | `07A798` | `08A798` |
| 24 | `076510` | `086510` | 4C | `078F4C` | `088F4C` |
| 25 | `0780CC` | `0880CC` | 4D | `07B688` | `08B688` |
| 26 | `0766F4` | `0866F4` | 4E | `07B874` | `08B874` |
| 27 | `076820` | `086820` | 4F | `07B900` | `08B900` |

Slots `0x43` is zero (the loader's `+0x10000` fixes it to `0x10000`, where
`CC` = INT3 is resident); slots `0x2D..0x4F` are a mixed block (below).

### 1.5 Slot families (erratum)

Every action handler is entered as `CALL [rec+0x18]` with only `EAX = rec`
(`0x7CD2B`, `0x785A8`; FU-74 §2). Slots `0x00..0x2C` all start with an
`EAX`-consuming action prologue (e.g. `0x86A34 PUSH …; MOV EBP,EAX; MOV
EDX,[EBP+0x89]; MOV AX,[0x57A64]; ADD EDX,EAX; …`). Slots `0x2D..0x4F`
instead consume `EDX` (or `BX`) as an input:

```
0x6DE34  PUSH ESI; PUSH EDI; MOV EDI,EDX; LEA ESI,[EAX+0x59]; MOVSD x3; RET
0x6E1D0  PUSH ECX; PUSH ESI; PUSH EDI; SUB ESP,4; MOV ECX,EDX; …
0x6DCC8  PUSH ECX; PUSH ESI; PUSH EDI; MOV ECX,EAX; MOV EAX,[EAX+0xC]; TEST BX,BX; …
```

So `0x2D..0x4F` are the target of a **different, two-argument dispatcher**
sharing the same address range; no static reader of that half was found
(searches for operands `0x10780`/`0x107E0` are empty). The action table proper
evidenced here is `0x00..0x2C` (45 slots); the rest are enumerated with their
targets but not classified as actions (open leg 1).

## 2. Handler classification (`0x00..0x2C`)

Method: each runtime target was created as a function and its body dumped
(`/tmp/opencode/fu76_handlers.txt`, 40 targets, plus the 80-slot dump). The
class column states the evident function, with the strongest instruction or
call as evidence; bodies only reachable through inline `CS:` tables may be
incomplete (marked †). `RESET` = `CALL 0x7DAB4`, `TMR93` = `CALL 0x79B58`,
`INSTALL n` = `FUN_0007D9A4` with code n, `KICK` = `CALL 0x7B9C4`,
`STAGE` = `CALL 0x7A490`, `MOVEOFF` = `CALL 0x79C20`, `CAMPLACE` =
`CALL 0x79F3C`, `EVENT` = `CALL 0x6E598`, `EVARM` = `CALL 0x974DC/0x974F0`,
`METRIC` = `CALL 0x8DCD4`, `NEAREST` = `CALL 0x8DE8C`, `ANGLE` =
`CALL 0xCD474`/`0x8DD70`.

| code | runtime | class | evidence |
|---|---|---|---|
| 00 | `07DB10` | outfield locomotion/decision | `+0x89 -= delta`; `MOVEOFF` with slot dir; INSTALL `3` if `+0x8D!=0` else `0x19` (§3.1) |
| 01 | `07DBC0` | camera/event transition | reads `[0x57A83]`, phase 1; `CAMRST 0x700F4`; no install; `RESET` arm |
| 02 | `07DFCC` | restart/set-piece | phase 1 arm builds `team+0x7B2` position, `NEAREST`+`METRIC`, `STAGE` ball via `0x7A490`, `EVENT 0x92820`, INSTALL 4 |
| 03 | `07E1A4` | tracked-entity placement | `CAMPLACE 0x79F3C` (camera-relative output), `TMR93`, `EVENT`; forced code for non-primary records (FU-75 §1.5) |
| 04 | `07E7C8` | chase/pressure machine | 649 insns; `DEC0E 0x7E600`, `RING 0x8F188`, RNG, `NEAREST`, installs `4/0x19/0xF/0xB/7/6/5`; forced target for `team+0x7B2/0x7B6` (FU-75) |
| 05 | `07F194` | carrier/possession state | installer sets `+0x9F` bit for code 5 (FU-74 §2); body reads the `0x58724` block (`[0x58724..0x5872F]`), `RESET` |
| 06 | `0801B4` | pursuit/duel with ball | 597 insns; `METRIC`, `ANGLE`, `FMUL 0x795A4`, `MOVEOFF`, `CLAMP 0x7D3E4`, `NEAREST`, INSTALL 4; forced when opponent carries (FU-75) |
| 07 | `0814B0` | kick/pass action | 2-stage machine, calls `KICK 0x7B9C4` with the slot released word/`0x40`, INSTALL `0x22`, tail INSTALL 4 on `[0x58730]` (§3.2) |
| 08 | `081068` | chase (code 8) | `TMR93`, `FUN_00079C50`, `EVENT`, `TAIL 0x79B1C`; the FU-75 chase-gate target |
| 09 | `080A00` | stage machine (4 arms) | `CS:[EAX*4+0x709F0]` → flat `0x809F0` `{0x70A3F,0x70BFB,0x70FCC,0x7103A}`; arms read `+0x8D`, `team+0x7B2` |
| 0A | `081738` | ball-relative run/placement | `NEAREST`, `ANGLE`, `FMUL`, `TMR93`, `RESET`; installer `0x7CDD8` has no xrefs (FU-75 open leg) |
| 0B | `081908` | event/duel with ring post | `EVENT`x3, `NEAREST`, `VEC3 0x795B4`, `ANGLE`x3, `RING`, INSTALL `0x0C`, `EVARM`, `TAIL` |
| 0C | `081C90` | stage machine (7 arms) | `CS:[EAX*4+0x71C74]` → flat `0x81C74` 7 dwords `0x71CDC..0x724F2`; reads `[0x5888F]` record, `VEC3`, INSTALL 0x0C via `0x81BEB` |
| 0D | `08251C` | event transition | `EVENT 0x6E598`, `FUN_00079C50`, `RESET` |
| 0E | `082710` | stage transition (4 arms) | `CS:[EAX*4+0x72700]` → flat `0x82700` `{0x7275D,0x727B1,0x72806,0x72AB8}`; only `RESET` decoded |
| 0F | `082AD0` | kick action (second) | `METRIC`, `EVENT`, `TAIL`, `KICK 0x7B9C4`; 157 insns |
| 10 | `0855F0` | phase transition | phase 2/3 gates, `RESET` only |
| 11 | `085DE4` | phase transition | phase gate, `RESET` only |
| 12 | `083D68` | gated transition | `RESET`, reads `[0x4C31C]` (settings latch) |
| 13 | `084B00` | camera reset transition | `CAMRST 0x700F4`, `RESET`, reads `[0x57A83]` |
| 14 | `08784C` | scripted/celebration sequence | `RNG 0x92AC8` x8, `EVARM`, `METRIC`, `CAMRST`; reads the `0x58720` block |
| 15 | `087CD0` | transition (body not fully decoded) | prologue `53 51 52 56 57 55 83 ec 0c`, `MOV EAX,[0x57A4A]`; Ghidra listing mis-decodes from `0x87CD6` (overlap) |
| 16 | `084630` | sequence with vector pop | `VEC3 0x795B4`, `RNG`, `EVENT`, `FUN_00079C50` |
| 17 | `084730` | phase transition | `RESET` only |
| 18 | `0849B0` | interception/duel | sets `+0x7B = 2`; `METRIC`, `NSEARCH 0x8DB6C`, `SWAP 0x786A0`; stage byte `+0x92` 0..2 |
| 19 | `0746E4` | keeper hold/claim (long body) | sets `[0x57C5D]=1` (`0x74707`), constants `0x9F0/0xAE0/0xC0`, reads `team+0x7C7`; body continues past the Ghidra cut at `0x7471D` |
| 1A | `07662C` | keeper reposition A | FU-74 §4: installed by `0x78236/0x7825E/0x78285` with `[0x57C2C]` constants; reads `[0x57A83]` |
| 1B | `076D28` | keeper reposition B | installed `0x7655C/0x782A2`; `CAMRST` |
| 1C | `077728` | keeper dive/lunge | installed `0x762A4/0x782BF`; 455 insns, `ANGLE`x4, `FMUL`x5, RNG, `EVARM` |
| 1D | `074EB0` | keeper close-down | reads `[0x57A83]`, `[0x57C5E]`; `[0x4C31C]` gate |
| 1E | `07550C` | keeper claim/throw | reads `+0x8F` stage, `+0x9B` (possession flag); sets `[0x5774C..54]`, `[0x57750]`, `[0x57A83]`, `+0x9B=1`, `CAMRST` (FU-73 §3.1 take site `0x76DAE` shares this pattern) |
| 1F | `076380` | keeper arm state | reads `[0x57C5C]`; gate `[rec+0x20]`, `FUN_00079B6C` |
| 20 | `084EEC` | transition with input table | reads `[0x4C114..0x4C11C]` (per-side input words), `[0x57A83]` |
| 21 | `085214` | carrier state / receive | installer sets `+0x9F` (FU-74 §2); body reads `[0x58730]`/`[0x58734]`, `NEAREST`, `METRIC`, `ANGLE`, INSTALL 4 |
| 22 | `08539C` | phase-2 short event | `EVENT`, `TAIL`x2; installed on the opponent by code 7's stage 1 (`0x816DF`) |
| 23 | `082F84` | tackle/lunge | `+0x92` 0..1 stages; `TMR93`, `DIST 0x8DC68`, calls `0x82DD0`, INSTALL `0x0F`; installed by outfield handler `0x7D174` (FU-75 §1.4) |
| 24 | `086510` | stats/commentary sequence | `FUN_00079C50`, `0x8EEC0/0x8EEB4/0x67800/0x6E1D0`; installed `0x8B750/0x8B78B` |
| 25 | `0880CC` | stats sequence | `EVARM`, `METRIC 0x8DCD4`, `FUN_00079C50`, calls `0x36200` |
| 26 | `0866F4` | placement variant | only `METRIC 0x8DCD4`; mirrors `0x86820`'s timer/`+0x8A` pattern |
| 27 | `086820` | placement variant | `+0x89 += delta`; output `+0x4D = 0x780`, `+0x55 += ±table[0x103CB]` scaled by `+0x8D&1`; body cut by mis-decoded overlap at `0x8687F` |
| 28 | `0870E8` | unclassified action-shaped | `PUSH …PUSH EBP; SUB ESP,0x20; MOV EBP,EAX; MOV ESI,0x6D8B0; LEA EBX,[EAX+0x65]; MOVSD x3 …` (copies a 12-byte template into `+0x65`) |
| 29 | `0874E4` | unclassified action-shaped | `MOV EBP,EAX; MOV word [EAX+0x7B],2; MOV EAX,[0x57A4A]; CMP EAX,5 …` |
| 2A | `086A34` | `+0x831` chosen-record action | installed only at `0x8D807` together with `[team+0x831]=record`; body `MOV EBP,EAX; +0x89 += delta; EAX=[EBP+0x8F]…` |
| 2B | `087738` | one-instruction stub | bytes at `0x87738` = `C3` (`RET`); no install site |
| 2C | `084598` | action-shaped, unanalyzed | bytes `53 51 52 56 89 c6 8a 80 …` (prologue) but Ghidra defines no instruction; no install site |

Classification limits: bodies implemented with inline `CS:` tables (`09`, `0C`,
`0E`, `0F`, `14`) were decoded only up to their table dispatch; the class names
above are the strongest body evidence, not asserted role names.

## 3. Deep dives

### 3.1 Code `0` — the generic outfield step (`0x7DB10`)

Full body (51 instructions, `0x7DB10..0x7DBAC`):

```
0x7DB14  ESI = rec; [rec+0x9E] = 1
0x7DB16  if ([rec+0x89] > 0) [rec+0x89] -= (word)[0x57A64]
0x7DB3D  if ([rec+0x20] != 0 && ([0x57A4A]>>24) != 6) {
0x7DB53     EDX = (int8)([slot+0x1D] >> 24)
0x7DB56     EBX = (int8)([slot+0x1E] >> 24)
0x7DB5F     FUN_00079C20(rec, EDX, EBX)
         }
0x7DB64  if (([0x57A4A]>>24) == 2 && word [rec+0x81] == 0 &&
             [rec+0x89] <= 0) {
0x7DB84     EAX = ([rec+0x8D] != 0) ? 3 : 0x19
0x7DBA3     FUN_0007D9A4(rec, EAX, EBX=0, ECX=0)
         }
```

`FUN_00079C20` (`0x79C20..0x79C4E`, 17 instructions) is the move-target
writer:

```
0x79C21  EDX = (int16)DX
0x79C26  ECX = EDX << 7
0x79C29  EDX = [rec+0x59]
0x79C2C  EAX += 0x4D
0x79C31  EDX += ECX
0x79C36  EBX = (int16)BX
0x79C39  ECX = [rec+0x61]
0x79C3C  [EAX+0]   = EDX             ; out.x = pos.x + dir_x<<7
0x79C43  ECX += EBX<<7
0x79C45  [EAX+4]   = 0               ; out.y = 0
0x79C45  [EAX+8]   = ECX             ; out.z = pos.z + dir_z<<7
0x79C48  CALL 0x7D3E4                ; clamp
```

`FUN_0007D3E4` (`0x7D3E4..0x7D42C`, 23 instructions):

```
0x7D3E6  V = [EAX];   if (V > 0x720) [EAX] = 0x720;
                      else if (V < -0x720) [EAX] = -0x720
0x7D406  W = [EAX+8]; if (W > 0xB10) [EAX+8] = 0xB10;
                      else if (W < -0xB10) [EAX+8] = -0xB10
```

So code 0 is the per-frame default: it decays the `+0x89` action timer by the
frame delta, points the output position `+0x4D/+0x51/+0x55` one 7-bit step
along the control slot's direction bytes `slot[+0x1D]>>24`/`slot[+0x1E]>>24`
(clamped to the pitch box), and, once `+0x81` and `+0x89` are clear in phase
2, installs `3` for an active record (`+0x8D != 0`) or `0x19` for an inactive
one (the same coercion `FUN_0007D9A4` itself applies to code 3).

### 3.2 Code `7` — the kick action (`0x814B0`, 186 instructions)

`+0x92` is the installer's argument byte (stage), `+0x89` an action timer.

**Stage 0 (arg 0)** `0x814D7..0x815CD` (entry `0x814D7..0x814ED` accumulates
`[rec+0x89] += delta` each frame; this action's `+0x89` is a duration meter,
unlike code 0's countdown):

```
0x8150C  [rec+0x9E] = 1
0x81512  d = [rec+0x69] >> 16;  t = word [rec+0x5D] + 0x70
0x81526  if (d > 0x40 || (int16)t < (int16)[0x57750]) {
0x81538     if ([rec+0x89] > 0x3C) return
         }
0x8154C  if ([rec+0x20] != 0 && (word [slot+6] == 0x60 || == 0x8000))
             out.x = [0x5774C] + (int8)table[0xF331 + type8] << 4
             out.z = [0x57754] + (int8)table[0xF339 + type8] << 4
         else copy camera triple [0x5774C..54] -> rec+0x4D..55
0x815B5  [rec+0x89] = 0; [rec+0x92]++
```

where `type8 = [rec+0x8B] >> 24` and `table` = flat `0x10F331/0x10F339`
(object-4 `0xF331`/`0xF339`).

**Stage 1 (arg 1)** `0x815CD..0x816FC`:

```
0x815D0  SI = (rec == [team+0x7CB]) ? 0x40
              : slot ? word [slot+6]
              : (([0x58740] >> 24) == 3 ? 0x40 : -1)
0x81605  if (SI == 0x40 && FUN_0007E600(rec) != 0) return     ; decision 0x0E
0x8161D  if ([0x4C32A] == 0 && [0x5B680] == 4 && SI == 0x40) SI = 0x20
0x8163C  EBX = 0x58738 (the staging vector)
0x81646  ret = FUN_0007B9C4(rec, SI, 0x58738)
0x8164B  if (ret != 0 && (int16)[0x5873C]>>16 < 0x30 &&
             [rec+0x89] < 5 &&
             opp != 0 && [opp+0x8E]>>24 == 6 && [opp+0x20] == 0 &&
             word [opp+0x6B] < 0xD0 &&
             |atan2(opp[+0x6B],opp[+0x6D]) - [opp+0x7D]| < 0x55)
0x816DF     FUN_0007D9A4(opp, code 0x22, EBX=0, ECX=1)        ; invoke now
0x816E4  [rec+0x89] = 0; [rec+0x92]++
```

**Stage 2 / tail** `0x816FC..0x81731`:

```
0x816FC  if ([rec+0x44] == 0) return
0x81702  FUN_0007DAB4(rec)                                    ; reset/choose
0x81709  if (rec == [team+0x7B2]) {
0x81714     FUN_0007D9A4([0x58730], code 4, 0, 0)             ; ball actor chases
0x81727     FUN_00079B58([0x58734])                           ; receiver timer
         }
```

So code 7 is the wind-up → kick → follow-through: stage 0 picks the output
target from the slot animation bytes or the camera; stage 1 hands the ball to
`FUN_0007B9C4` with the released input word (or `0x40` long-ball request) and
snaps the opposing controlled record into action `0x22` when it is a type-6
record lined up near the kick; the tail resets and hands the ball's new actor
to code 4.

### 3.3 `FUN_0007B9C4` — the kick/row application (`0x7B9C4`, 337 insns)

Entry `EAX = actor` (or 0 to reuse `[0x58730]`), `EDX = mode byte`,
`EBX = 6-byte input vector or 0`.

```
0x7B9D4  if (EAX) [0x58730] = EAX else EBP = [0x58730]
0x7B9E5  byte [0x58742] = DL                       ; mode/flags
0x7B9EB  if (EBX) copy 6 bytes EBX -> 0x58738 else zero the vector
0x7BA17  word [0x5873E] = 0                        ; trajectory
0x7BA1E  if ([EBP+0x20] == 0) goto 0x7BBE4         ; no control slot
0x7BA31  ...slot/direction wing-target selection (quoted in FU-73 §2)...
0x7BBE4  if ((int8)[0x58742] < 0) {                ; negative mode = range band
             x = word [0x58738]
             [0x58742] = (x < 0x5A0) ? 0x20 : (x < 0x780) ? 0x30 : 0x10
         }
0x7BC34  if (([0x58742] & 0x20) && [rec+0x8D] != 0)
             byte [0x5873F]>>24 == 0x40 ? FUN_0007B194(...) : FUN_0007B57C(...)
0x7BC80  ECX = [0x5873A]>>16; EBX = [0x58738]>>16;
         EDX = [0x5873F]>>24 (= byte 0x58742);
         row = FUN_0007AE70(actor, EBX, ECX, EDX)
0x7BCA2  if (row == 0 || row[0] == 0) return 0     ; no event
```

**Row application** `0x7BCB6..0x7BE0B`:

```
0x7BCB6  byte [0x58743] = row[0]        ; event code
0x7BCBC  byte [0x58744] = row[9]
0x7BCBF  BX = word row[+2]              ; lower bound
0x7BCC3  SI = byte row[+8]              ; speed divisor (mode-4 branch)
0x7BCCE  CX = word row[+6]              ; trajectory increment
0x7BCD2  DX = word row[+4]              ; upper bound
0x7BCDB  [ESP+0xC] = CX (add) ; ECX = 0
0x7BCE4  if (mode == 0x30) { BX = 0x1C8; DX = 0x780; [ESP+0xC] = 0x30;
                             SI = 0x18; ECX = 1 }
0x7BD06  side = FUN_000741B4(byte [[actor]+0x826])
0x7BD19  if (word [0x4C1D4 + side*2] & 0x10) {          ; user range bit
0x7BD2C     if (ECX != 0 || event(0x58743) == 1 || == 3)
                BX = BX + (BX >> 1); DX = DX + (DX >> 1) ; 1.5x
         }
0x7BD55  DI = word [0x58738]
0x7BD5C  if (BX > DI)      [0x58738] = BX
0x7BD6A  else if (DX < DI) [0x58738] = DX
0x7BD7A  angle = FUN_000CD474(word [0x58738]>>16, word [0x5873A]>>16)
         ... 10-bit angle -> table 0x114E04 dword * speed [0x58736]>>16 via
             FUN_000795A4 -> words 0x5873A and 0x5873C
0x7BE0B  word [0x5873E] += (low word of row[+6])
0x7BE26  if (ball.x != 0 && byte(0x58743) != 3) {
             if (byte(0x58743) == 4) { ...RNG branch (0x7BE45..0x7BE99)... }
             if (SI != 0) [0x5873E] = (word [0x5873C]) + (word [0x58736]) / SI
         }
0x7BECE  if ((int16)word [0x5873C] > 0x460) word [0x5873E] = 0x460
0x7BEDE  FUN_0007A490([0x58730], 0x58738, word [0x5873C]>>16, byte [0x58743])
0x7BF0A  return 1
```

Note the **lower-bound-first** clamp (BX from `row[+2]`, DX from `row[+4]`):
`if (BX > x) x = BX; else if (DX < x) x = DX`, i.e. `x∈[row[+2],row[+4]]`
with `row[+2]` the lower and `row[+4]` the upper bound. The 1.5× extension is
applied to both bounds (only when the user range bit `0x10` of the per-side
input word `0x4C1D4+side*2` is set and the event is 1 or 3, or mode is
`0x30`). The trajectory increment and the final `0x460` cap use the 16-bit
words; the store goes through `FUN_0007A490` (ball staging, FU-73 §1).

### 3.4 `FUN_0007AE70` — the event-row resolver (`0x7AE70`, 188 insns)

Inputs `EAX = actor`, `EDX = code` (DI), `EBX = x word`, `ECX = z word`.

**Class** `0x7AE79..0x7AEA2`:

```
if (code == 0x40) class = 2
else { EAX = code; XOR AH,DH; AND AL,0x10; CWDE; class = (EAX != 0) ? 0 : 1 }
```

(the `AND AL,0x10` tests code bit 4; the `XOR AH,DH` is dead with respect to it).

**Sector/index** `0x7AEAC..0x7AEFA`:

```
if (x == 0 && z == 0)  sector = (int8)(actor+0x8E & 0xFF)
else { a = FUN_000CD474(x, z);                  ; x pushed second = [EBP+8]
       sector = ((a + 0x40) & 0x3FF) >> 7 }     ; 0..7
if (code == 0x40) idx = 0
else idx = (byte [0x1104CA + (actor[+0x8B] >> 24)] >> sector) & 1
```

`0x1104CA` is the object-4 bitmask table (`38 70 E0 C1 83 07 0E 1C 00 00 00
01 01 01 01 02 …`); the first eight bytes are `0x38` rotated left by the
subtype (`0x38,0x70,0xE0,0xC1,0x83,0x07,0x0E,0x1C`), so the extracted bit
selects one of eight sector classes per subtype.

**Distance band and tables** `0x7AEFD..0x7B01A`:

```
d = (int16)(word [0x57750] - word [actor+0x5D])
if ([actor+0x5D] != 0) {
    if (d < 0x38) return 0
    if (class == 1 && slot != 0 && phase == 2 && [actor+0x8D] != 0
        && slot[+0x23] < 7)
        { table = 0x11016E; index = idx }                       ; carry fast path
    else if (slot != 0 && code == 0x60)
        { table = 0x11016E; index = idx + 2 }
    else {
        band = (d < 0x20) ? 0 : (d < 0x70) ? 1 : 2
        table = ([actor+0x8D] != 0) ? 0x110196 : 0x11024A
        index = 2*code + 6*class + idx
    }
}
row = table + 10*index
```

The 10-byte stride multiplies the index through
`LEA EAX,[EDX*4]; ADD EAX,EDX; ADD EAX,EAX` (`0x7B00D..0x7B016`).

**Row[0] dispatch** `0x7B08E..0x7B17B`. `AL = row[0]`; `AL-1` indexes the
inline table `CS:[EAX*4 + 0x6AE38]` (true address **`0x7AE38`**, 14 dwords,
`{0x6B0A7,0x6B0BB,0x6B0CF,0x6B0E3,0x6B0F7,0x6B10B,0x6B11F,0x6B133,0x6B133,
0x6B07D,0x6B147,0x6B147,0x6B15B,0x6B16F}`, runtime `0x7B0A7..0x7B16F`).
The explicit compares before the switch intercept `row[0] ∈
{1,0x10,0x11,0x12,0x13,0x20}`, and the switch itself is entered for
`row[0] ∈ {0,2..0xE,0xF,0x14..0x1F}` with `AL-1`; the resolved dispatch is:

| `row[0]` | path | call |
|---|---|---|
| 0 | `0x7B08E` → `0x7B183` | `FUN_00092820(actor, 0)` |
| 1 | `0x7B03E` | `FUN_00092820(actor, 1)` |
| 2 | `0x7B0BB` | `FUN_000928F0(actor, 0x0F)` |
| 3 | `0x7B0CF` | `FUN_000928F0(actor, 0x11)` |
| 4 | `0x7B0E3` | `FUN_000928F0(actor, 0x0A)` |
| 5 | `0x7B0F7` | `FUN_000928F0(actor, 0x09)` |
| 6 | `0x7B10B` | `FUN_000928F0(actor, 0x14)` |
| 7 | `0x7B11F` | `FUN_000928F0(actor, 0x13)` |
| 8, 9 | `0x7B133` | `FUN_000928F0(actor, 0x16)` |
| 0x0A, 0x10 | `0x7B07D` | `FUN_000928F0(actor, 3)` |
| 0x0B, 0x0C | `0x7B147` | `FUN_000928F0(actor, 4)` |
| 0x0D | `0x7B15B` | `FUN_000928F0(actor, 0x10)` |
| 0x0E | `0x7B16F` | `FUN_000928F0(actor, 0x14)` |
| 0x11 | `0x7B04A` | `FUN_000928F0(actor, 2)` |
| 0x12 | `0x7B05B` | `FUN_000928F0(actor, 7)` |
| 0x13, 0x20 | `0x7B06C` | `FUN_000928F0(actor, 8)` |
| other | `0x7B183` | `FUN_00092820(actor, 0)` |

(The table entry index 0 (`row[0]==1` → `0x7B0A7`, `EDX=0x0D`) is shadowed by
the explicit `row[0]==1` compare and is unreachable; the `0x7B170`/`0x7B174`
window decodes only as part of the `0x7B16F` arm.)

`FUN_000928F0` (`0x928F0..0x92994`) writes `BL = code` into a 25-entry ring at
`0x5B440` (stride `0x15`, cursor `[0x5B662]>>24`) and, when
`table[0x110F1C + code] & 1`, appends an RNG and a copy of the triple at
`0x57758`; `FUN_00092820` (`0x92820..0x92861`) stores the code in `[0x5B650]`
and copies the same triple when the table bit is set. Both are
presentation-side (`0x5Bxxx`) code, so the resolved `row[0]` values above are
the event-row selector meaning this slice can evidence.

### 3.5 `FUN_000CD474` — the 10-bit atan (`0xCD474`, 26 insns + arms)

Inputs `[EBP+8] = x`, `[EBP+0xC] = z` (both sign-extended words from the
callers).

```
0xCD479  flags = 0
0xCD47C  if (z < 0) { flags |= 8;  z = -z }
0xCD47F  if (x < 0) { flags |= 16; x = -x }
0xCD491  if (x <  z) no swap
         else if (x == z) { EAX = 0x80; dispatch }
         else { x<->z; flags |= 4 }                ; |x| > |z| case
0xCD49C  EAX = 0; DIV ECX (max)                    ; q = (min<<32)/max
0xCD4A0  EAX >>= 24; ADC EAX,0                      ; 8-bit interpolated index
0xCD4A6  EAX = byte [0x14072C + EAX]                ; object-4 atan table (257 B)
0xCD4AD  JMP dword CS:[EBX + 0xBD4C0]               ; true table 0xCD4C0
```

The 8 arm stubs at `0xCD4E0..0xCD513` apply the quadrant:

| flags | arm | result |
|---|---|---|
| 0 | `0xCD4E7` | `t` |
| 4 (swap) | `0xCD4E0` | `0x100 - t` |
| 8 (z<0) | `0xCD4E9` | `0x200 - t` |
| 0xC | `0xCD4F2` | `t + 0x100` |
| 0x10 (x<0) | `0xCD4F9` | `-t` |
| 0x14 | `0xCD4FD` | `t - 0x100` |
| 0x18 | `0xCD504` | `t - 0x200` |
| 0x1C | `0xCD50B` | `-t - 0x100` |

with `t = atan_table[round(min*256/max)]` (the table is monotone `0..0x80`
over `0..0xFF`; entry `0x100` = `0x80`). Zero vector returns `0x80`. The
result is a raw 10-bit angle (not always reduced to `0..0x3FF`; callers apply
`AND AH,3` or the folding idiom).

## 4. Port: `fifa96_action_handlers`

`include/fifa96_loader/fifa96_action_handlers.h` +
`src/fifa96_loader/fifa96_action_handlers.c` (caller-owned data, static const
table, no globals, no comments, negative `fifa96_err_t` for invalid
arguments).

| original | port |
|---|---|
| `FUN_00079C20` + `FUN_0007D3E4` (`0x79C20..0x79C4E`, `0x7D3E4..0x7D42C`) | `fifa96_action_move_target(pos_x, pos_z, dir_x, dir_z, &out)`: `out.x = pos_x + dir_x*128`, `out.y = 0`, `out.z = pos_z + dir_z*128`, then clamp `x∈[-0x720,0x720]`, `z∈[-0xB10,0xB10]` |
| code 0 body `0x7DB10..0x7DBAC` (timer, move gate, install) | `fifa96_action_move_step(&state, &out)`: decays `timer89` when `> 0`, sets `out.move` for `has_slot && phase != 6`, sets `out.install/out.code` for `phase == 2 && timer81 == 0 && timer89 <= 0` (`active ? 3 : 0x19`) |
| `FUN_000CD474` + table `0x14072C` | `fifa96_action_kick_angle(x, z, &angle)`: unsigned magnitude/quadrant/wrap algorithm above, 257-byte static const table |
| `FUN_0007B9C4` row application `0x7BCB6..0x7BEDE` core | `fifa96_action_kick_apply(&ball, &row, mode, event_code, user_extend)`: `mode 0x30` override, 1.5× extension on `user_extend && (mode==0x30 || event==1 || event==3)`, `row.lo`-first clamp, `traj += row.traj_add`, cap `comp_z > 0x460 → 0x460` |
| `FUN_0007B9C4` target selection, angle→vector table `0x114E04`, mode-4 RNG branch, `FUN_0007AE70`, `FUN_0007A490` staging | not ported (globals/tables/RNG/actor pointers) |
| action install `FUN_0007D9A4`, slot/option tables | not ported (pointer table/globals) |

## 5. Tests (`tests/test_action_handlers.c`, suite 64 → 65)

* Layout `_Static_assert`s on all port struct offsets.
* `move_target`: identity, `dir<<7` offsets, positive/negative `int8` dirs,
  the four clamp edges, saturation from a large dir, NULL.
* `move_step`: timer decay and negative result; install `0x19` when
  `active == 0`, `3` when `active != 0`; `has_slot`/phase-6 move gate; no
  install while `timer81 != 0` or `timer89` stays positive; phase != 2 no
  install; delta 0 boundary; NULL.
* `kick_angle`: `(0,0)`→`0x80`, the eight quadrant/wrap cases `(1,0)`,
  `(0,1)`, `(1,1)`, `(-1,0)`, `(0,-1)`, `(1,-1)`, `(-1,-1)`, interpolation
  `(2,1)`→`0xB4`, `(1,2)`→`0x4C`, rounding `(3,5)`→`0x58`, `(5,3)`→`0xA8`,
  the `0xFF`/`0x100` table edge `(256,257)`/`(257,256)`/`(0xFFFFFF,0x1000000)`,
  NULL.
* `kick_apply`: inside/above/below clamp, lower/upper boundary, 1.5×
  extension (`event 1/3`, not `event 0/2`), mode-`0x30` bounds, 16-bit
  trajectory wrap, `0x460` cap and the `0x460` boundary, NULL.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_action_handlers.c src/fifa96_loader/fifa96_action_handlers.c` runs
clean. `make test`: 64/64 before, **65/65 after**.

## 6. Errata (quoted)

* FU-74 §2 / FU-75 §5 "the table at data flat `0x1106E0` holds 40 dwords
  (codes `0x00..0x27`)" — **corrected**: the array runs to flat `0x110820`
  (80 dwords / slots `0x00..0x4F`; the following dword is the first outfield
  row `{0x07FF,0x60,0x6CE38}`). Slot `0x2A` is installed at `0x8D807`, and the
  slots `0x2D..0x4F` consume `EDX` (two-argument family), so the action table
  proper is `0x00..0x2C` (open leg 1).
* FU-73 §2 / §8 open leg 3 "`JMP dword CS:[EAX*4+0x6AE38]` points outside the
  image at flat `0x16AE38` and its raw bytes are fixup placeholders" —
  **corrected**: the operand is object-1-relative; true table `0x7AE38`
  (`+0x10000`), 14 static dwords whose runtime arms `0x7B0A7..0x7B16F` are
  quoted in §3.4. This closes FU-73 open leg 3.
* FU-73 §2 "clamp `[0x58738]` to `[row[4],row[2]]`" — **refined**: the
  algorithm is lower-bound-first (`row[+2]` is the lower bound, `row[+4]` the
  upper; §3.3); the 1.5× user-range extension and the exact bounds are quoted.
* FU-74 §9.3 "action functions `0x7DB10` (0), `0x746E4` (0x19), …" — **extended**
  with the code-0 body and the code-7/kick path; the remaining bodies stay
  open (open leg 3).
* FU-74 §2 action table "40 entries" is also the source of FU-75 §5's
  `0x1106E0[code]`/`0x10000` mapping; the mapping itself is confirmed, the
  count corrected.
* `disassemble_function 0x7B9C4` dropped `0x7BA31..0x7BAE1`; recovered with
  `disassemble_bytes` (tooling errata, Method).

## 7. Open legs

1. **Slots `0x2D..0x4F`**: the pointer block shares the action array's address
   range but its targets take `EDX`/`BX` arguments (`0x6DE34`, `0x6E1D0`,
   `0x6DCC8` quoted); no static reader was found (operand searches `0x10780`,
   `0x107E0` empty). Their dispatcher and signature are unknown.
2. **Codes `0x2B`/`0x2C`**: a lone `RET` and an unanalyzed prologue; whether
   they are reachable actions is unknown.
3. **Full bodies** of codes `01,02,05,06,08,09,0A,0B,0C,0D,0E,0F,10..1C,1E..2A`
   are classified from entry blocks/call signatures only; the inline-table
   arms (`0x809F0`, `0x81C74`, `0x82700`, code `0F`'s table) are listed by
   address, not decomposed.
4. **`FUN_0007AE70`**: the commentary bit table `0x110F1C` (gates the camera
   copy in `FUN_000928F0`/`FUN_00092820`) and the meaning of the `EDX` event
   codes are not derived; the class/band/index math is.
5. **`FUN_0007B9C4`** target selection (`0x7BA1E..0x7BBE4`), the 10-bit-angle
   → `0x114E04` vector-component fold, the mode-4 RNG trajectory branch
   (`0x7BE45..0x7BE99`), the `0x58736` speed/divisor source, and the
   `0x5873F/0x58740` byte aliasing are quoted at block level only.
6. **Code 7 stage conditions**: the `FUN_0007E600` gate, the `0x5B680==4`
   long-ball request path and the exact `[0x4C32A]/[0x4C31C]` settings latches
   are cited, not decomposed.
7. **Site `0x8D807`**: the routine installing code `0x2A` has no defined
   function (record-1..10 scan, `[team+0x831] = record`); its caller is not
   located.
8. **`0x1104CA` bit table**: only the rotating-byte entries `0..7` are
   explained by the `(byte >> sector) & 1` extraction; the tail `0,0,0,1,1,1,1,2…`
   is another table start.
9. **Code `0x15`/`0x27`**: Ghidra listing overlap (`0x87CD6`, `0x8687F`)
   prevents a complete decode from the current database.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x7D9A4, 0x7B9C4 (+ `disassemble_bytes`
0x7BA31..0x7BAE2, 0x7BF0A), 0x7AE70 (+ `disassemble_bytes`
0x7B07D..0x7B17B), 0x79C20, 0x79F3C, 0x8E244, 0x7D3E4, 0x795A4, 0xCD474 (+
`disassemble_bytes` 0xCD4E7..0xCD530), 0x928F0, 0x92820; `read_memory`
0x1106E0 (160 B), 0x1104CA (64 B), 0x110780 (96 B), 0x14072C (257 B), 0x7AE38
(64 B), 0xCD4C0 (40 B), 0xCD4E0, 0x1F680, 0x10F680, 0x5E1D0, 0x646E4, 0x7DB10,
0x6DB10, 0x809F0, 0x81C74, 0x82700; `search_instructions` operand `0x106e0`
(single match), `0x10780`, `0x107e0` (empty); `get_function_by_address`
0x746E4, 0x7DB10, 0x6DB10; `run_script_inline` (action entry function
creation + body dump, 80-slot enumeration, install-site census, atan-table
export). Analysis-only outside the port: no tool, capture-rig, ISO or
Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_action_handlers.h`,
`src/fifa96_loader/fifa96_action_handlers.c`,
`tests/test_action_handlers.c`, `CMakeLists.txt` (one library/test block).
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
