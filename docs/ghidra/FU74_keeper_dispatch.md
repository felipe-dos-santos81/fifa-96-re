# FU-74: per-team dispatch (S6) and the keeper machine (S7)

Sub-slices S6 (per-team dispatch) and S7 (keeper record) of FU-67 §5,
extended from FU-67's map to the full record walk, the action-install/table
mechanism, the per-record input dispatch tables, and the keeper state machine
(`FUN_000782D0`) including its CPU decision path (`FUN_00077EAC`), target
generation (`FUN_0006DBCC`) and ball staging (`FUN_0006DC88`). Ports the clean
pieces as `fifa96_keeper`.

Result in one line: **`FUN_0008D8EC` walks each team block record 0 (the keeper
— the block itself, since records are `team+0`) unconditionally through
`FUN_000782D0`, then records 1..10 of stride `0xB2` through `FUN_0007CA54`,
skipping only `+0x9A`; both machines first run a mask/want row dispatcher
(pressed word `&0xFF0` for outfield, `&0xF70` for the keeper) whose handlers are
CS-relative code offsets (`+0x10000`), then the keeper tail selects its next
action among `0` (timer `+0x81` active), `0x19` (phase != 2 / not controlled /
opponent's controlled entity type 5), keep-current (own type 5) or `4`, gates on
the type-flag table flat `0x110680[type]&1`, and installs it through
`FUN_0007D9A4` (action fn = flat `0x1106E0[code]`, `+0x10000`); when the keeper
has no control slot in phase 2 with `[0x57C5D]` set, `FUN_00077EAC` runs instead:
it must see type `0x19`, and either stages a ball vector into `0x58738` and
enters action `7` (`FUN_0006DBCC` target → `FUN_0006DC88`), or repositions via
the jump-table arms into actions `0x1A`/`0x1B`/`0x1C` with a direction constant
in `[0x57C2C]`.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-73). All
  instructions quoted below were read back this slice with
  `disassemble_function`/`disassemble_bytes`/`read_memory`; the decompiler was
  not used for the quoted bodies (FU-67/FU-70/FU-73 errata: it prunes or
  mis-infers blocks in this region).
* **Address convention (FU-59/FU-61/FU-67/FU-70, extended).** Data-table
  addresses named by code immediates and pointer cells resolve through
  `+0x100000` (e.g. `0x106E0` → flat `0x1106E0`, `0x10950` → `0x110950`,
  `0x109DC` cell → `0x10870` → `0x110870`). New this slice: **stored code
  pointers inside those tables are CS-relative offsets and resolve through
  `+0x10000`**, not flat. Proofs:
  * action table flat `0x1106E0` stores `0x646E4` at code `0x19`; raw
    `0x646E4` decodes to `LES EBX,[EDX+0xC0850000]` (garbage) while
    `0x646E4+0x10000 = 0x746E4` is a clean prologue
    (`PUSH EBX..PUSH EBP; SUB ESP,0x4C; MOV ECX,EAX; ...; MOV byte [EAX+0x9E],1`,
    `disassemble_bytes 0x746E4`); same for `0x6DB10→0x7DB10`,
    `0x6E7C8→0x7E7C8`, `0x6F194→0x7F194`, `0x71068→0x81068`,
    `0x75214→0x85214`.
  * the keeper's inline jump table `0x78210 JMP dword CS:[EAX*4+0x67e98]` reads
    flat `0x77E98` = `{0x6826D,0x68245,0x6821D,0x68294,0x682B1}`, and the file
    code at `+0x10000` (`0x7826D`, `0x78245`, `0x7821D`, `0x78294`, `0x782B1`)
    is exactly the five switch arms (`disassemble_bytes`); the `CS:` override +
    base `0x10000` is quoted.
  * dispatch-table handlers likewise: `0x66130→0x76130`, `0x6CF20→0x7CF20`,
    `0x6D110→0x7D110`, `0x6D1D4→0x7D1D4` all start clean prologues.
  Direct `CALL rel32` targets are unaffected (they already resolve, e.g.
  `0x7801C CALL 0x6DC88`).
* Every numeric claim below is quoted from the listings; semantic labels beyond
  what the instructions do are not asserted.

## 1. S6 — the per-team dispatch `FUN_0008D8EC`

`FUN_0004B100` calls it once per team, side 0 then side 1
(`0x4B2A9 CALL 0x8D8EC` EAX=`0x588A4`; `0x4B2B0 CALL 0x8D8EC` EAX=`0x590D9`;
FU-67 §4). 179 instructions, `0x8D8EC..0x8DB6A`. Order:

| # | site | step |
|---|---|---|
| 1 | `0x8D8F5..0x8D912` | `EBP = team`; `team[+0x82C]++`, wrap to 0 at `>= 0xB` |
| 2 | `0x8D91E..0x8D923` | `[0x586D0] = 0` |
| 3 | `0x8D929..0x8D9BD` | phase-2 reselect arm (`FUN_0008DE8C` to `team+0x7B2`) |
| 4 | `0x8D9BD..0x8DAF1` | interception arm (`team+0x7BA`, `team+0x7BE`) |
| 5 | `0x8DAF3..0x8DB2C` | `team+0x7CB/+0x81E/+0x820` timer |
| 6 | `0x8DB2E..0x8DB35` | keeper: `FUN_000782D0(EAX=team, EBX=1)` |
| 7 | `0x8DB3A..0x8DB5F` | outfield: records 1..10 via `FUN_0007CA54` |

**Steps 3–4 (quoted exactly, re-verified this slice):**

```
0x8D919  EAX = [0x57A4A] >> 24                       ; phase
0x8D929  if (phase == 2 && (team[+0x7B2]==0 || [0x57A83]==0))
0x8D948     counter = [0x577FA] (word); target = ESP copy of
            0x57788 if counter <  [0x57800]
            0x57794 if counter <  [0x57806]
            else 0x57770 + ([0x577BE]>>16)*0x20 on X and ([0x577C0]>>16)*0x20 on +8
0x8D9AA     EDX=team; ECX=0; EBX=0; CALL 0x8DE8C      ; skip 0
0x8D9B7     team[+0x7B2] = result
0x8D9C5  team[+0x7BE] = 0
0x8D9D5  if (phase == 2 && [0x57A83] != 0 && [team+0x826] == [[0x57A83]+0x826]
0x8D9F7     if (|[0x57754]| > 0x480 && sign([0x57754]) == team side)
0x8DA2C        skip = [team[+0x7B2]+0x8A] >> 24
0x8DA4D        team[+0x7BA] = FUN_0008DE8C(target = 0xF37C + side*0xC, team, skip)
0x8DA5D        if ([team[+0x7BA]+0x20] != 0) team[+0x7BA] = 0
0x8DA72        else FUN_0008D824([0x57A83], team[+0x7BA]+0x4D)
0x8DA7F        FUN_000795B4(team[+0x7BA]+0x59, team[+0x7BA]+0x4D, &stack)
0x8DA94        if (stack word < 0xF0 && [nearest+0x69]>>16 <= 0x1E0 &&
                   |nearest[+0x61]| > |[0x57754]| + 0x90) team[+0x7BE] = 1
0x8DAE9     else team[+0x7BA] = 0
0x8DAF3  if (team[+0x7CB] != 0):
           limit = [team+0x81E] >> 16; delta = [0x57A64] (word)
0x8DB10    if (limit <= delta) { team[+0x820] = 0; team[+0x7CB] = 0; }
0x8DB27    else team[+0x820] -= delta
```

Note `0x8D9F7..0x8DA07` loads `[0x57754]`, negates in 32-bit if negative
(`NEG EAX`), and compares to `0x480` (`JLE`), so the magnitude test is on the
full 32-bit value, as quoted. `[0x57754]` is the camera Z (FU-67 §4.1); the
`0xF37C + side*0xC` target is a 3-dword record (FU-70 §2.3).

**Steps 6–7 (the S6 dispatch itself, quoted exactly):**

```
0x8DB2E  EAX = EBP (team); EBX = 1; CALL 0x782D0      ; record 0, keeper
0x8DB3A  EDX = EBP + 0xB2                              ; record 1
0x8DB40  JMP 0x8DB59
0x8DB42  if (byte [EDX+0x9A] == 0) { EAX = EDX; CALL 0x7CA54; }
0x8DB52  EBX++
0x8DB53  EDX += 0xB2
0x8DB59  EAX = (int16)BX; if (EAX < 0xB) goto 0x8DB42
```

So the walk is **keeper (index 0) unconditionally, then indices 1..10 only when
`record+0x9A == 0`** (`+0x98` is not tested here). The `EBX=1` second argument
to `FUN_000782D0` is dead: the callee's first `EBX` use is the overwrite at
`0x783A8` (see §3). The keeper record *is* the team block (records start at
`team+0`), so `FUN_000782D0`'s `EBP` addresses record 0 directly, and `[EBP]`
(the record's `+0x00` back-pointer) points back to the same block.
The dispatch sequence per `FUN_0004B100` is therefore
`team0 keeper → team0 outfield 1..10 → team1 keeper → team1 outfield 1..10`.

## 2. The action install / dispatch `FUN_0007D9A4` and table flat `0x1106E0`

`FUN_0007D9A4` (73 instructions, `0x7D9A4..0x7DAB2`; callers: the two record
machines, `FUN_00077EAC` arms, and action sites such as the release site
`0x898F3` quoted in FU-73 §3.5). Signature in registers: `EAX = record`,
`DX = action code`, `BX = arg byte`, `ECX = invoke-now flag`.

```
0x7D9B6  if (EAX == 0) return
0x7D9BE  if ([rec+0x9A] != 0) return
0x7D9CB  if ((int8)[rec+0x91] == DX) return                ; already in this action
0x7D9DC  if ([rec+0x98] != 0 && code != 0xC && phase != 2/0xA/0xF):
0x7DA08     FUN_0006E598(EAX=rec, EDX=0xF, ECX=[rec+0x8B]>>24, EBX=0)
0x7DA1F     [rec+0x98] = 0
0x7DA26  if ([rec+0x8D] == 0 && code == 3) code = 0x19
0x7DA42  if (code == 5 || code == 0x21) [rec+0x9F] |= 1
0x7DA5C  else                          [rec+0x9F] &= ~1
0x7DA63  [rec+0x91] = (byte)code
0x7DA6D  EAX = code; EAX <<= 2; EAX += 0x106E0; action = [EAX]   ; flat 0x1106E0
0x7DA7E  [rec+0x89] = 0
0x7DA88  [rec+0x9E] = 0
0x7DA8F  [rec+0x18] = action
0x7DA92  [rec+0x92] = (byte)BX
0x7DA9B  [rec+0x7B] = [rec+0x79]
0x7DAA3  if (ECX != 0) { EAX = rec; CALL [rec+0x18]; }     ; invoke immediately
```

The table at data flat `0x1106E0` holds 40 dwords (codes `0x00..0x27`); each is
a CS-relative code offset (`+0x10000`, §Method):

| code | stored | runtime | code | stored | runtime |
|---|---|---|---|---|---|
| 00 | `6DB10` | `7DB10` | 14 | `7784C` | `8784C` |
| 01 | `6DBC0` | `7DBC0` | 15 | `77CD0` | `87CD0` |
| 02 | `6DFCC` | `7DFCC` | 16 | `74630` | `84630` |
| 03 | `6E1A4` | `7E1A4` | 17 | `74730` | `84730` |
| 04 | `6E7C8` | `7E7C8` | 18 | `749B0` | `849B0` |
| 05 | `6F194` | `7F194` | 19 | `646E4` | `746E4` |
| 06 | `701B4` | `801B4` | 1A | `6662C` | `7662C` |
| 07 | `714B0` | `814B0` | 1B | `66D28` | `76D28` |
| 08 | `71068` | `81068` | 1C | `67728` | `77728` |
| 09 | `70A00` | `80A00` | 1D | `64EB0` | `74EB0` |
| 0A | `71738` | `81738` | 1E | `6550C` | `7550C` |
| 0B | `71908` | `81908` | 1F | `66380` | `76380` |
| 0C | `71C90` | `81C90` | 20 | `74EEC` | `84EEC` |
| 0D | `7251C` | `8251C` | 21 | `75214` | `85214` |
| 0E | `72710` | `82710` | 22 | `7539C` | `8539C` |
| 0F | `72AD0` | `82AD0` | 23 | `72F84` | `82F84` |
| 10 | `755F0` | `855F0` | 24 | `76510` | `86510` |
| 11 | `75DE4` | `85DE4` | 25 | `780CC` | `880CC` |
| 12 | `73D68` | `83D68` | 26 | `766F4` | `866F4` |
| 13 | `74B00` | `84B00` | 27 | `76820` | `86820` |

The two machines call `[rec+0x18]` every frame (`0x7CD29..0x7CD2B` in
`FUN_0007CA54`, `0x785A8..0x785AA` in `FUN_000782D0`), so `+0x18` is the
currently installed per-action state function; `+0x91` is its code and is what
`FUN_0007D9A4` uses to suppress redundant installs.

**Before the action call, both machines run an input-edge dispatcher over
8-byte `{word mask, word want, dword handler}` rows.** The handler is called
with `EAX = record`; `handler == 0` terminates the row list. Outfield searches
the pressed table first (`slot[+4] & 0xFF0`), else the released table
(`slot[+6] & 0xFF0`), and continues to the next row when the handler returns 0
(`0x7CB96..0x7CC11`). Keeper uses `slot[+4|+6] & 0xF70` (bit `0x80` and the
low nibble excluded); the pressed arm runs one handler and stops, the released
arm tests the return (`0x7843B..0x784D1`).

Outfield table pointers (data flat `0x1109D0`, codes 0..4) and their row tables
(all handlers shown at runtime address; mask always `0x07FF`):

```
pressed  code0 -> 110820: {60,6CE38} {40,6CF20} {80,6CF20} term
pressed  code1 -> 110840: {80,6CF20} term
pressed  code2 -> 110850: {10,6D174} {40,6D174} {80,6CF20} term
pressed  code3 -> 110870: {10,6CFD0} {40,6CD60} {80,6CF20} term
pressed  code4 -> 110890: {10,6D054} {40,6D08C} {80,6CF20} term
released code0 -> 1108B0: {30,6CEB0} {20,6D1D4} {20,6CEB0} {10,6CF54}
                          {40,6D0C4} {40,6CD60} term
released code1 -> 1108E8: {50,6D010} {30,6D110} {10,6D110} {20,6D110}
                          {40,6D110} {60,6D110} term
released code2/3/4 -> 110920/110930/110940: {20,6D1D4} term
```

(listed as stored; each handler adds `0x10000`). The dispatch **code** (0..4)
that selects the pointer comes from `0x7CB0D..0x7CB7E`:

```
if (slot[+4] != 0 || slot[+6] != 0):
  2 if ([0x577EE]>>16 >= 0x50)
  1 else if (rec[+0x8E]>>24 == 5)
  3 else if (rec == [0x577CA] && ([0x57A83]==0 || rec == [0x57A83]))
  4 else if (rec == [rec[0]+0x7BF])
  0 otherwise
```

**Keeper table selection** (`0x783D2..0x7843B` pressed; `0x7846A..0x784A9`
released; cell loads resolve `+0x100000`):

```
pressed, mask = slot[+4] & 0xF70:
  rec == [0x577CA]          -> table = [0x109DC] -> flat 110870 (shared code3)
  else [team+0x7BF] != 0    -> table = [0x109E0] -> flat 110890 (shared code4)
  else rec != [0x57A83]     -> table = 10950
  else { EAX = word[user+0x5D]; EAX += 0x70; CWDE; EAX -= 0x10;
         EAX >= [0x57750] } -> 10970 else 10978
released, mask = slot[+6] & 0xF70:
  rec != [0x57A83]          -> 10998
  else { same user[+0x5D] expression >= [0x57750] } -> 109A8 else 109C8
```

Keeper row tables (flat; all handlers runtime; `0x10970` and `0x109C8` are
empty — first row handler 0):

```
10950: {20,66130} {10,66130} {40,66130} term
10978: {20,66130} {10,66130} {40,66130} term
10998: {20,6D1D4} term
109A8: {10,6D110} {20,6D110} {40,6D110} term
```

So the keeper shares the outfield code3/code4 pressed tables when it is the
tracked entity or when `team+0x7BF` is set, and has its own `0x76130` handler
otherwise. The four-handler set `{0x7CD60, 0x7CE38, 0x7CEB0, 0x7CF20, 0x7CF54,
0x7CFD0, 0x7D010, 0x7D054, 0x7D08C, 0x7D0C4, 0x7D110, 0x7D174, 0x7D1D4,
0x76130}` is not decomposed (open leg 8.1).

## 3. S7 — the keeper machine `FUN_000782D0`

Sole caller `0x8DB35` (§1); `EAX = team block = record 0`. 224 instructions,
`0x782D0..0x785D0`. Structure:

### 3.1 One-shot stat counters

```
0x782D8  if ([0x57823] != 0 && rec == [0x57A83] && phase != 5):
0x782F6     [0x57823] = 0
0x78301     idx = FUN_000741B4([rec[0]+0x826])       ; side-index helper
0x78311     word [0x57AD0 + idx*2]++
0x78319     idx = FUN_000741B4(side ^ 1)
0x7832E     word [0x57ACC + idx*2]++
```

This is the same `0x57ACC` word array `FUN_00072478` increments (FU-67 §2), and
`[0x57823]` is the flag `FUN_00072478` clears at `0x724EF` before its own
increment: a "tracked event" is attributed once to the keeper's side and once
to the opponent side. `[0x57823]` is also written by `FUN_000703E8 0x70533`,
`0x773A7` and `0x70021`.

### 3.2 Timers and state fields

```
0x78336  if (word [rec+0x81] != 0):
           limit = [rec+0x7F] >> 16; delta = [0x57A64]
           if (limit <= delta) [rec+0x81] = 0 else [rec+0x81] -= delta
0x7836A  word [rec+0x7B] = word [rec+0x79]          ; keeper only (not outfield)
0x78378  if ([rec+0x93] != 0):                      ; byte countdown, same pattern
           if ([rec+0x93] <= delta) [rec+0x93] = 0 else [rec+0x93] -= delta
```

### 3.3 Input dispatch and the forced-action tail

```
0x783A8  if ([rec+0x20] != 0 && word [rec+0x81] == 0 && phase == 2)
0x783D2     ... pressed/released tables of §2 ...
0x784D3  if ((flat[0x110680 + (rec[+0x8E]>>24)] & 1) == 0) goto 0x7859E
0x784EF     AX = (int8)[rec+0x91]                   ; current action
0x784FE     if (word [rec+0x81] != 0) AX = 0
0x78507     else if (phase != 2)       AX = 0x19
0x78515     else if (rec == [team+0x7B2]):          ; rec is the controlled entity
0x78520        other = [team[+0x7A6]+0x7B2]         ; opponent controlled
0x7852C        if (other != 0 && [other+0x8E]>>24 == 5) AX = 0x19
0x7853E        else if ([rec+0x8E]>>24 == 5)         AX unchanged (keep current)
0x7854C        else                                 AX = 4
0x78553     else                        AX = 0x19     ; not the controlled entity
0x78558     if (AX != (int8)[rec+0x91]):
0x78565        [0x57AB2] = 1
0x78576        FUN_0007D9A4(rec, code=AX, EBX=0, ECX=0)
0x7857B  if (phase == 2 && [rec+0x20] == 0 && [0x57C5D] != 0)
0x78597     FUN_00077EAC(rec)
0x7859E  [0x57C5D] = 0
0x785A2  CALL [rec+0x18]
0x785AB  FUN_0006E8E8(rec)
0x785B2  if ([0x57AA7] == rec) copy +0x59/+0x5D/+0x61 -> +0x4D/+0x51/+0x55
0x785C3  FUN_0007BF20(rec)
```

The type gate reads the byte at flat `0x110680 + type` (immediate `0x10680`,
data base); `type = rec[+0x8E] >> 24` and for type `0x19` the byte is `3`
(`&1 = 1`), so the keeper enters the tail. The forced codes are exactly
`0`, `0x19`, `4` or "keep current"; table `0x1106E0` maps them to `0x7DB10`,
`0x746E4`, `0x7E7C8`. The `+0x7B2` comparison means the forcing only applies
when the keeper itself is the team's controlled entity; `[0x57A83]` is the
user/camera side's controlled entity (FU-70 §2).

## 4. S7 — the CPU keeper decision `FUN_00077EAC`

Called only from `0x78597` (sole xref) when the keeper has **no control slot**
(`rec[+0x20] == 0`), phase 2, and `[0x57C5D] != 0`. 282 instructions,
`0x77EAC..0x782CD`. Head gate:

```
0x77EB7  if ((rec[+0x8E]>>24) != 0x19) return       ; keeper type evidence
0x77ECB  idx = FUN_000741B4([rec[0]+0x826])
0x77EDE  if (word [0x4C1D4 + idx*2] & 0x200) return  ; input/user flag
0x77EF6  if ([rec+0x9A] != 0) return
0x77F03  if (byte [[rec+0x28]] == 0x54) return
```

**Timer / not-our-side gate:** if `[rec+0x81] != 0`, copy the keeper position
triple into the output triple and return (`0x77F16..0x77F29`). If
`[0x57A83] == rec` or `[0x57A83]` exists on the keeper's own side, return
(`0x77F2E..0x77F51`). So the decision only runs for an uncontrolled keeper on
the side opposite the user's controlled entity.

**Behind-goal test** (`0x77F57..0x77FF5`): copies camera `0x5774C..0x57754`;
computes `FUN_000795B4(rec+0x59, 0x57C48, &out)`; takes `|cam.x|` and
`|cam.z|`; `EAX = 1` only when `|cam.x| < 0x420 && |cam.z| > 0x7B0` and the
side/sign check selects the keeper's side: `[0x57A49]>>24 == 1` → `EAX=1` iff
`[0x57754] <= 0` (`0x77FBB`), else `EAX=1` iff `side == 0 ? [0x57754] <= 0 :
[0x57754] > 0` (`0x77FC6..0x77FEE`, `SETZ`/`SETG`/`XOR`).

* **Behind goal (`AX=1`)** → `0x7802B`: clears `[0x57C5C]`, then either:
  * close to the target (`[rec+0x69]>>16 < 0x20 && [0x57750] < 4 &&
    [0x577BC]>>16 < 0xA`): copies camera `0x5774C..` to `rec+0x4D..`,
    `ECX=1`, `0x781DE`; or
  * `[rec+0x69]>>16 < 2*[[rec+4]+0xB]>>24 + 0x30 && [0x57750] == 0 &&
    [0x57A83] != 0 && opposite side`: copies camera to `rec+0x4D..`, adds
    `2*word[user+0x73]` to X and `2*word[user+0x75]` to Z, sets
    `[0x57C5C] = 1`, `ECX = 4`, `0x781E1`; or
  * otherwise looks up `[0x57C44]` against tables `0xE1C4`/`0xE1CE`/`0xE184`
    with `[0x57C4C]` and `[0x57821]`, and picks `ECX = 0/4/5` or a table
    value, copying `0x57C48..` to `rec+0x4D..` (`0x780E2..0x781E0`).
  * then at `0x781E1`: if `ECX == 0` return; if `FUN_000765C4(rec) != 0` →
    `FUN_0007D9A4(rec, code 4, 0, ECX=1)` (`0x781F6`, `0x782BB`); else
    `ECX--` and `JMP dword CS:[ECX*4 + 0x67E98]` (`0x78210`), whose arms are
    (runtime addresses, `0x682xx + 0x10000`):
    * `0x7821D`: `[0x57C2C] = 0x2E`, install action `0x1A`;
    * `0x78245`: `[0x57C2C] = 0x2D`, install `0x1A`;
    * `0x7826D`: `[0x57C2C] = 0x2C`, install `0x1A`;
    * `0x78294`: install `0x1B`;
    * `0x782B1`: install `0x1C`;
    all via `FUN_0007D9A4(rec, code, EBX=0, ECX=1)` (`0x78236`, `0x7825E`,
    `0x78285`, `0x7829E`, `0x782BF`).
* **Not behind goal (`AX=0`)** → `0x77FF5`: if `[rec+0x69]>>16 >= 0x40`
  return; otherwise `RNG = FUN_00092AC8(); code = (RNG & 1) + 2` and
  `FUN_0006DC88(rec, code)` (`0x78009..0x7801C`).

`FUN_000765C4` (34 instructions) is the "tracked teammate" predicate: returns 1
iff `[0x577CA]` exists, shares the keeper's side, and (its type is 5 or the
ball event code `[0x58740]>>24 != 4`).

### 4.1 Target generation `FUN_0006DBCC` and staging `FUN_0006DC88`

`FUN_0006DBCC(EAX=rec, DX=code, EBX=out)` (64 instructions, `0x6DBCC..0x6DC86`):

```
0x6DBD6  team = [rec]; base = [team+0x7DB]              ; per-team table pointer
0x6DBDE  side = byte [team+0x826]
0x6DBE5  scale = [0x577D6 + side*4] >> 16; base += scale << 3
0x6DBF4  if (code < 0) RNG = FUN_00092AC8()
0x6DBFE  index = (code & 3); base += index*2
0x6DC07  b0 = (int8)[base]; b1 = (int8)[base+1]
0x6DC0B  out.x = 0x26 * b0 ; out.z = 0x21 * b1          ; then NEG both if side != 0
0x6DC45  return ([side==0 && out.z >= 0xB10] ||
                 [side==1 && out.z <= -0xB10]) ? 3 : 2
```

`FUN_0006DC88(EAX=rec, DX=code)` (23 instructions) then:

```
0x6DC95  AL = FUN_0006DBCC(rec, code, &stack_vector)
0x6DCA1  byte [0x58743] = AL                            ; ball event code
0x6DCAE  FUN_0008DCD4(EAX=rec+0x59, EDX=&stack_vector, EBX=0x58738)
         ; writes the signed delta from keeper to target into the ball vector
0x7D9A4  FUN_0007D9A4(rec, code = 7, EBX=0, ECX=0)      ; enter action 7
```

So the keeper's far-ball response generates a target from the team's defensive
table (`team+0x7DB`, side-scaled by `[0x577D6+side*4]`), stages the keeper→target
delta into the ball staging vector `0x58738` and the event code `0x58743`
(FU-73 §1/§2 block), and enters action `7` (`0x814B0`), which is one of the
action functions that read the ball vector (`search_instructions` on `0x58738`
lists `0x7F6B6`, `0x7F8FD`, `0x800A5`, `0x8163C`, `0x82D30`, `0x82D81`,
`0x84386`, `0x86290`, `0x852B7`, `0x752CB/0x753EE`, ...).

### 4.2 Ball / trajectory / duel-snap interaction (refining the FU-67 anchor)

The keeper machine `FUN_000782D0` reads **no** `0x587xx` operand itself; its
decision inputs are the camera triple `0x5774C..0x57754`, `[0x57750]`,
`[0x577BC]`, `[0x577CA]`, `[0x57A83]`, the team fields `+0x7A6/+0x7B2/+0x7BF`,
the per-team table `+0x7DB` and the timers/type. The ball *staging* vector
`0x58738` is written by the keeper path (`FUN_0006DC88 0x6DCAE`) and read by
the action functions it installs (`7`, `0x1A`, ...), so the ball enters S7
through the action layer, not through `FUN_000782D0`. The duel snap
`FUN_0007D430` (FU-73 §3.4) is called once per frame by `FUN_0004B100 0x4B2DA`
on the two `team+0x7B2` pointers *after* both team updates; it can rewrite the
keeper's output triple `+0x4D/+0x51/+0x55` only when the keeper is selected as
the team's controlled entity (possible via `FUN_0008DDE0` at reset/roster,
FU-70 §2.2). It is not invoked from the keeper code.

## 5. Structures and fields (evidenced this slice)

Record (stride `0xB2`, 11 per team, record 0 = team block):

| offset | width | evidence |
|---|---|---|
| `+0x00` | dword | team back-pointer; `[rec[0]+0x826]` side reads (`0x782F8`, `0x6DBD6`) |
| `+0x18` | dword | installed action function (`0x7DA8F`); called every frame (`0x7CD2B`, `0x785A8`) |
| `+0x20` | dword | bound control slot (`0x7CABA`, `0x783A8`); its `+4/+6` are the pressed/released words, `+0x10` prev mapped |
| `+0x28` | dword | pointer whose first byte is compared to `0x54` (`0x77F03`) |
| `+0x4D/+0x51/+0x55` | 3 dwords | output position (writes `0x7CC70..0x7CC7D`, `0x78053`, `0x780A5`, `0x781DE`, `0x785C0`, `0x7DAA...`) |
| `+0x59/+0x5D/+0x61` | 3 dwords | position triple (keeper targets `rec+0x59`, `user+0x5D`) |
| `+0x69` | dword | high word compared `0x60`/`0x40`/`0x30` (`0x78032`, `0x77FFA`, `0x7CC35`) |
| `+0x73/+0x75` | words | velocity pair used by the reposition lead (`user+0x73/+0x75` at `0x780B0`) |
| `+0x79/+0x7B` | words | keeper copies `+0x7B = +0x79` (`0x7836A`); installer also copies (`0x7DA9B`) |
| `+0x7A` | dword (unaligned) | dereferenced by action `0x7E7C8` (`0x7E7D9`) |
| `+0x7F` | dword | high word timer limit (`0x78344`) |
| `+0x81` | word | countdown timer; force-action gate (`0x78336`, `0x784FE`) |
| `+0x89` | dword | zeroed by installer (`0x7DA7E`); counted down by actions (`0x7DB16`) |
| `+0x8B` | dword | high byte = type passed to `FUN_0006E598` (`0x7DA0F`) |
| `+0x8D` | byte | flag; `0` + code 3 coerces to `0x19` (`0x7DA26`) |
| `+0x8E` | dword | high byte = type; keeper `0x19` (`0x77EC2`), type-5 branches (`0x7853E`) |
| `+0x91` | byte | current action code (`0x7D9CB`, `0x78558`) |
| `+0x92` | byte | arg byte from installer BX (`0x7DA92`) |
| `+0x93` | byte | countdown timer (`0x78378`) |
| `+0x98` | byte | installer's event arm (`0x7D9DC`), cleared (`0x7DA1F`) |
| `+0x9A` | byte | dispatch skip (`0x8DB42`), installer reject (`0x7D9BE`) |
| `+0x9E` | byte | zeroed by installer (`0x7DA88`), set 1 by most action prologues |
| `+0x9F` | byte | bit 0 set for codes 5/0x21 (`0x7DA53`) |

Team block fields new to this slice:

| offset | width | evidence |
|---|---|---|
| `+0x7DB` | dword | per-team defensive target table pointer (`FUN_0006DBCC 0x6DBD8`) |

Keeper state globals (all read/written in the S7 paths):

| address | evidence |
|---|---|
| `0x57823` | one-shot stat flag (read/clear `0x782D8/0x782FB`; writers `0x70533`, `0x724EF`, `0x773A7`, `0x70021`) |
| `0x57ACC`/`0x57AD0` | 16-bit counter arrays indexed by `FUN_000741B4` (`0x78311`, `0x7832E`; FU-67 §2) |
| `0x57AB2` | set on every forced action install (`0x7856C`) |
| `0x57C2C` | direction constant of the jump-table arms (`0x78230/0x78258/0x7827E`) |
| `0x57C44`, `0x57C48`, `0x57C4C`, `0x57C5C` | read/written by the behind-goal arm (`0x780E2..0x781D9`) |
| `0x57C5D` | CPU-Keeper decision flag: set by actions at `0x74707`, `0x76318`, `0x763AB`; consumed/cleared `0x7858E/0x785A2` |
| `0x577CA` | tracked entity (`0x783E3`, `0x765C7`) |
| `0x577D6` | per-side scale table base (`0x6DBE5`) |
| `0x4C1D4` | per-side user config word; bit `0x200` gates the CPU decision (`0x77EDE`) |
| `0x110680` | type-flag table (data flat): type `0x19` → `3`; gate `&1` (`0x784DC`, `0x761D6`) |

## 6. Port: `fifa96_keeper`

`include/fifa96_loader/fifa96_keeper.h` + `src/fifa96_loader/fifa96_keeper.c`
(caller-owned data, no globals, no comments, `-fifa96_err_t` for invalid
arguments). Scope: the per-team record walk of §1 and the keeper forced-action
selection of §3.3.

| original | port |
|---|---|
| `FUN_0008D8EC 0x8DB2E..0x8DB5F` keeper-then-1..10 walk, `+0x9A` skip | `fifa96_dispatch_begin`/`fifa96_dispatch_next` over `fifa96_dispatch_team`/`fifa96_dispatch_record`; `is_keeper` marks record 0 |
| team order side 0 then side 1 (`0x4B2A9`, `0x4B2B0`) | iterator team-major order |
| `FUN_000782D0 0x784D3..0x78576` type gate + forced code | `fifa96_keeper_select_action(state, type_gate, current, &next)`; `type_gate` is the caller-computed `table[type] & 1`, `state` carries `timer`/`phase`/`controlled`/own/opponent type-5 |
| `FUN_0007D9A4` install semantics (`+0x91`, `+0x18` table) | not ported (pointer table/global) |
| input row dispatchers (outfield/keeper tables) | not ported (table data + handler code) |
| `FUN_00077EAC`, `FUN_0006DBCC`, `FUN_0006DC88`, `FUN_000765C4`, jump arms | not ported (globals/camera/team table/RNG) |
| timers `+0x81/+0x93`, `+0x7B=+0x79`, tails `FUN_0006E8E8`/`FUN_0007BF20` | not ported |

## 7. Tests (`tests/test_keeper.c`, suite 62 → 63)

* Layout: `_Static_assert` on the iterator and state fields.
* `fifa96_dispatch_begin/next`: two-team order (team 0 keeper, team 0 outfield
  1..10 skipping `+0x9A`, team 1 keeper, team 1 outfield); record 0 yielded even
  when its own `+0x9A` is set; single-record team yields only the keeper;
  zero-record team is skipped; full 11-record team yield count; exhaustion
  returns 0; NULL iter/teams/outputs, `team_count == 0`, `records == NULL`
  with `count != 0` return `-FIFA96_ERR_INVALID`.
* `fifa96_keeper_select_action`: `type_gate == 0` no-op (next untouched);
  timer `!= 0` → `0`; phase `!= 2` → `0x19`; not controlled → `0x19`;
  controlled + opponent-controlled type 5 → `0x19`; opponent not type 5 and own
  type 5 → keep (return 0); own not type 5 → `4`; `code == current` returns 0;
  every forced target installs via returned 1; NULL state/out return
  `-FIFA96_ERR_INVALID`.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_keeper.c src/fifa96_loader/fifa96_keeper.c` runs clean.

## 8. Errata (quoted)

* FU-73 §2: "the action-pointer table `0x106E0` holds code addresses at flat
  `0x1106E0`" — **refined**: the table is at flat `0x1106E0`, but its dwords
  are CS-relative code offsets; runtime target = stored + `0x10000` (raw
  `0x646E4` is invalid, `0x746E4` is the prologue; the inline `CS:` jump table
  at `0x77E98` proves the base).
* FU-67 §4.3: "`FUN_0007CA54` dispatches on `[rec+0x8E]>>24` against tables
  `0x109D0`/`0x109E4` and calls `(*(code*)rec[6])()`" — **refined**: the two
  tables are the *input-edge* pointer tables at flat `0x1109D0`/`0x1109E4`
  (code 0..4); the `(*(code*)rec[6])()` call is `[rec+0x18]` and runs after
  the input dispatcher on every frame (`0x7CD29`).
* FU-67 §3.2/S7 "`+0x18` action vtable" — **refined**: `+0x18` holds the
  currently installed action function, written only by `FUN_0007D9A4` from
  table flat `0x1106E0` keyed by `[rec+0x91]`.
* FU-67 §4.2 "`FUN_000782D0(EAX=team, EBX=1)`" — **extended**: the `EBX=1`
  argument is dead (first `EBX` use in the callee is the overwrite at
  `0x783A8`); `EAX` is the team block, which is record 0.
* FU-67 §2 "increments a 16-bit slot of the `0x57ACC` array" — **extended**:
  the same array is incremented for `0x57AD0` (own side) and `0x57ACC` (other
  side) by the keeper one-shot when `[0x57823] != 0` and the keeper is
  `[0x57A83]` (`0x78311`, `0x7832E`).
* FU-73 §1.2 "the keeper reads `[0x577CA]` (`FUN_000782D0 0x783E3`)" —
  **confirmed**, with the effect: it selects the pressed-table pointer cell
  `[0x109DC]` (shared outfield code-3 table).
* FU-67 §7-S7 "keeper: mapped; tables/action ptr open" — **derived here**
  (machine, tables, installer, CPU decision, target/staging paths); the action
  function bodies remain open (8.1).
* Ghidra listing: `0x76235..0x7623D` in the `0x76130` handler decodes into
  implausible instructions (`ADD byte [EAX],AL` etc.) because of overlapping
  entry points; that window is not used for any claim.

## 9. Open legs

1. **Handler bodies** `0x76130` (keeper), `0x7CD60`, `0x7CE38`, `0x7CEB0`,
   `0x7CF20`, `0x7CF54`, `0x7CFD0`, `0x7D010`, `0x7D054`, `0x7D08C`,
   `0x7D0C4`, `0x7D110`, `0x7D174`, `0x7D1D4` are cited by entry only; their
   effects (install codes, movement, events) are not derived. The `0x76130`
   mid-body window `0x76235..0x7623D` is mis-decoded by the listing.
2. **`FUN_00077EAC` middle block** (`0x780E2..0x781E0`): tables `0xE1C4`,
   `0xE1CE`, `0xE184`, globals `[0x57C44]/[0x57C48]/[0x57C4C]/[0x57821]` and
   their writers are not decomposed; only the selection into the enumerated
   `ECX` values is quoted.
3. **Keeper action functions** `0x7DB10` (0), `0x746E4` (0x19), `0x7E7C8` (4),
   `0x801B4` (7), `0x7662C` (0x1A), `0x76D28` (0x1B), `0x77728` (0x1C) are
   entered (prologues/reads cited) but their state machines are not derived.
4. **`[0x57C2C]`** direction constant consumers (actions `0x1A`/`0x1B`/`0x1C`)
   are not located.
5. **Team `+0x7DB` table** content/format and the `[0x577D6+side*4]` scale
   table are not dumped.
6. **Type identities**: type `0x19` (keeper path gate) and type `5` (ball
   carrier/controlled checks) are quoted only as compared values; the roster
   semantics are not asserted (FU-67 open leg).
7. **Loader model**: why data pointers use `+0x100000` while code pointers use
   `+0x10000` is quoted behaviourally (CS-relative offsets), not modelled from
   the LE fixup records.
8. **`[0x57823]` set sites** `0x773A7`/`0x70021` are not inside defined
   functions in the listing; only `FUN_000703E8 0x70533` is named.
9. **Input flag `0x4C1D4 & 0x200`** and its meaning are not derived.
10. **`FUN_00079B58`/`FUN_00079B1C`/`FUN_0007C990`/`FUN_0006E8E8`/
    `FUN_0007BF20`** record tails are cited only.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x8D8EC, 0x7CA54, 0x782D0, 0x7D9A4, 0x77EAC, 0x765C4,
0x6DC88, 0x6DBCC, 0x4B100 (FU-67, re-read); `disassemble_bytes` 0x746E4,
0x646E4, 0x7DB10, 0x6DB10, 0x6E2E4, 0x7E2E4, 0x7E7C8, 0x7F194, 0x81068,
0x6CF20, 0x7CF20, 0x6D110, 0x7D110, 0x6D1D4, 0x7D1D4, 0x76130, 0x761C8,
0x78219, 0x78220, 0x782B1, 0x6DC88, 0x75214; `read_memory` 0x106E0, 0x1106E0
(160 B), 0x109DC/0x1109DC, 0x1109D0, 0x109D0, 0x1109E4, 0x110950, 0x110970,
0x110978, 0x110998, 0x1109A8, 0x1109C8, 0x110820, 0x110840, 0x110850, 0x110870,
0x110890, 0x1108B0, 0x1108D0, 0x1108E8, 0x110680, 0x206E0, 0x20950, 0x20870,
0x77E98, 0x167E98, 0x78219; `get_xrefs_to` 0x782D0, 0x7CA54, 0x57C5D, 0x57C5C,
0x57C48, 0x57823, 0x57ACC, 0x7E2E4, 0x7E2E7, 0x7DB10;
`get_function_by_address` 0x7D9A4, 0x7CA54, 0x782D0, 0x77EAC, 0x66130, 0x6D110,
0x6D1D4, 0x75214, 0x6DB10, 0x765C4, 0x6DBCC, 0x6DC88, 0x7DB10;
`search_instructions` operands 0x58730, 0x58738, 0x5873E, 0x6DB10, 0x75214,
0x6E2E4; `run_script_inline` (action-table target verification, 40 entries).

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_keeper.h`,
`src/fifa96_loader/fifa96_keeper.c`, `tests/test_keeper.c`, `CMakeLists.txt`
(one library/test block). `make test`: 62/62 before, **63/63 after**;
ASan+UBSan `test_keeper` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
