# FU-75: the outfield action selection (S8) and the tracker/decide pass (S4)

Sub-slices S8 (outfield record) and S4 (tracker + decision) of FU-67 §5,
extended from FU-67/FU-74's map to: the outfield per-record decision
`FUN_0007C990`, the mask/want row dispatch tables `0x1109D0`/`0x1109E4` and
their handlers, the chase gate, the match clock/phase machine `FUN_0008AF38`,
the tracker decision pass `FUN_00072AC4`, the lane builder `FUN_00072270`, and
the command ring `FUN_0008F188` with its consumer `FUN_0008F2C4`. Ports the
clean pieces as `fifa96_outfield`.

Result in one line: **per frame `FUN_0007CA54` (records 1..10) decrements
`+0x81`/`+0x93`, and when the record owns a control slot (`+0x20`) it selects a
dispatch code 0..4 (2 if `[0x577EE]>>16 >= 0x50`, 1 if type 5, 3 if the tracked
entity is the user's/absent, 4 if `rec == [team+0x7BF]`, else 0), runs the
pressed (`slot[+4]&0xFF0`) then released (`slot[+6]&0xFF0`) rows of flat
`0x1109D0`/`0x1109E4` matching `0x07FF & word == row.want`, and the matched
handler installs actions 7/8/9/0xB/0x21/0x23/0xE or writes the team role fields
`+0x7BF`/`+0x82A`; the phase-2/type-gated tail then runs `FUN_0007C990`, which
forces code 6 (opponent has the `+0x9F` bit), keeps type-5 carriers, or forces
4 for `team+0x7B2`/`team+0x7B6` and 3 for everyone else, after which an unbound,
far from the camera, opposite-side record gets code 8 (`0x81068`); all codes
install through `FUN_0007D9A4` (`+0x91`, `+0x18` = flat `0x1106E0[code]`).
The tracker is `FUN_00072AC4` (not `FUN_0008AF38`, which is the clock/phase
machine): it picks the tracked entity from `[0x57A83]`, builds 11 opponent lane
slots at `0x575F1` (stride `0x1A`) via `FUN_00072270`, tracks the best two in
`[0x575E9]`/`[0x575ED]`, and posts command ids (`0x55..0xB6`) through the
10-entry ring `0x5AAE0` (`FUN_0008F188`) consumed by `FUN_0008F2C4` via flat
`0x110BB4`; those commands are not the action installer — the action codes come
from the record machine's own decisions and input rows.**

## Method

* Static work on the open Ghidra MCP session for `fifa96_le.bin` (program
  `/fifa96_le.bin`, flat LE link addresses as in FU-4/FU-58..FU-74). All
  instructions below were read back this slice with `disassemble_function`,
  `disassemble_bytes` and `read_memory`; no decompiler output is used for
  quoted bodies (FU-67/FU-70/FU-73/FU-74 errata: it prunes or mis-infers
  blocks in this region).
* **Address convention (FU-59/FU-67/FU-74, extended).** Data-table addresses
  named by code immediates resolve through `+0x100000` (`0x109D0` →
  `0x1109D0`, `0x10680` → `0x110680`, `0x10BB4` → `0x110BB4`); stored code
  pointers inside those tables are CS-relative offsets and resolve through
  `+0x10000` (new proof this slice: ring handler table flat `0x110BB4` entry 0
  stores `0x7F37C`, raw bytes `18 83 F8 05...` are not code while
  `0x7F37C+0x10000 = 0x8F37C` is a clean prologue
  (`PUSH EDX; MOV EDX,[0x5AADC]; SHL EDX,5; ... CALL 0x8F0C4`)). The
  `FUN_0008AF38` inline jump table follows the same rule: `JMP dword ptr
  CS:[EAX*4 + 0x7AF28]` (`0x8B1D5`) reads flat `0x8AF28` =
  `{0x7B1DD, 0x7B22F, 0x7B2E9, 0x7B33F}` and the runtime targets
  (`0x8B1DD`, `0x8B22F`, `0x8B2E9`, `0x8B33F`) are exactly the four arms quoted
  in §2.
* **Tooling errata this slice.** `disassemble_bytes 0x7CD60..0x7D2A0` emitted a
  desynced window at `0x7D190` (`ADD byte ptr [ESI],1; ADD byte ptr [ECX+EAX],AH`),
  while `read_memory 0x7D187` returns `8b 86 8e 00 00 00 c1 f8 18 8a 80 80 06 01 00
  24 01 0f be c0` (`MOV EAX,[ESI+0x8E]; SAR EAX,0x18; MOV AL,[EAX+0x10680];
  AND AL,1; MOVSX EAX,AL`); the handler `0x7D174` gate below is read from the raw
  bytes, not the listing.
* Every numeric claim below is quoted from the listings; semantic labels beyond
  what the instructions do are not asserted.

## 1. S8 — the outfield machine `FUN_0007CA54`

Sole caller `0x8DB4D` inside `FUN_0008D8EC`'s records 1..10 walk
(`get_xrefs_to 0x7CA54` → `{0x8DB4D}`; quoted `0x8DB42 CMP byte [EDX+0x9A],0 /
0x8DB49 JNZ 0x8DB52 / 0x8DB4B MOV EAX,EDX / 0x8DB4D CALL 0x7CA54`). 227
instructions, `0x7CA54..0x7CD53`. Order:

| # | site | step |
|---|---|---|
| 1 | `0x7CA5C..0x7CA88` | `if (word [rec+0x81] != 0) { limit = [rec+0x7F]>>16; delta = [0x57A64]; if (limit > delta) [rec+0x81] -= delta; else [rec+0x81] = 0; }` |
| 2 | `0x7CA8A..0x7CAB8` | same countdown for byte `[rec+0x93]` |
| 3 | `0x7CABA..0x7CCC2` | input dispatch when `[rec+0x20] != 0` (control slot), §1.2/§1.3 |
| 4 | `0x7CC13..0x7CC7D` | no-edge arm (both slot words zero), §1.7 |
| 5 | `0x7CC82..0x7CD24` | phase-2 tail: type gate `flat[0x110680+type]&1`, `FUN_0007C990`, chase code 8, §1.5/§1.6 |
| 6 | `0x7CD29..0x7CD48` | `CALL [rec+0x18]` (installed action); `FUN_0006E8E8(rec)`; if `rec == [0x57AA7]` `FUN_00079B1C(rec)`; `FUN_0007BF20(rec)` |

When `[rec+0x20] == 0` step 3 jumps straight to step 5, so unbound records get
only the phase/type-gated forced decision. `FUN_0007D9A4` (install semantics
quoted in FU-74 §2: rejects `+0x9A`, suppresses when `[rec+0x91] == code`,
coerces code 3 to 0x19 when `[rec+0x8D] == 0`, sets `+0x91` and `+0x18` =
flat `0x1106E0[code]`) is the single install point for every code below.

### 1.1 The control slot

`[rec+0x20]` is the bound 4-slot control record (`0x57C64`, stride `0x25`;
FU-70 §2/§3). Fields read here: `+4` pressed-edge word, `+6` released-edge
word, `+0x10` previous mapped word, dwords `+0x1D`/`+0x1E` whose high bytes are
`+0x20`/`+0x21` (selection handler, §1.4).

### 1.2 Dispatch-code selection (`0x7CB0D..0x7CB7E`)

```
0x7CB0D  slot = [rec+0x20]
0x7CB10  if (slot[+4]==0 && slot[+6]==0) goto no-edge arm
0x7CB22  if ([0x577EE]>>16 >= 0x50)            code = 2
0x7CB36  else if ([rec+0x8E]>>24 == 5)         code = 1
0x7CB4B  else if (rec == [0x577CA] &&
                  ([0x57A83]==0 || rec==[0x57A83])) code = 3
0x7CB6A  else if (rec == [[rec]+0x7BF])        code = 4
0x7CB7C  else                                  code = 0
```

The test at `0x7CB10/0x7CB17` is on the **raw** words; the `&0xFF0` mask is
applied only to the word handed to the row scan (`0x7CB81`, `0x7CBCC`).

### 1.3 Mask/want row dispatch (`0x7CB7E..0x7CC11`)

Pointer tables (flat, `read_memory 0x1109D0`):

| code | pressed cell | pressed table | released cell | released table |
|---|---|---|---|---|
| 0 | `0x1109D0` → `0x10820` | 0x110820 | `0x1109E4` → `0x108B0` | 0x1108B0 |
| 1 | `0x1109D4` → `0x10840` | 0x110840 | `0x1109E8` → `0x108E8` | 0x1108E8 |
| 2 | `0x1109D8` → `0x10850` | 0x110850 | `0x1109EC` → `0x10920` | 0x110920 |
| 3 | `0x1109DC` → `0x10870` | 0x110870 | `0x1109F0` → `0x10930` | 0x110930 |
| 4 | `0x1109E0` → `0x10890` | 0x110890 | `0x1109F4` → `0x10940` | 0x110940 |

Scanner (`0x7CB7E..0x7CBC6` pressed, `0x7CBC8..0x7CC11` released):

```
word = slot[+4] & 0xFF0
if (word == 0) goto released
table = [code*4 + 0x109D0]
loop: if ((row[+0] & word) == row[+2]) {
        if (row[+4] == 0) goto tail          ; terminator always matches when word==0
        ret = handler(rec)
        if (ret != 0) goto tail
      }
      row += 8
released: word = slot[+6] & 0xFF0; if (word == 0) goto tail
          table = [code*4 + 0x109E4]; same loop
```

The rows are 8 bytes `{word mask, word want, dword handler}`; every evidenced
row has mask `0x07FF`. `read_memory 0x110820` (320 bytes, 10 tables) gives
(all handlers runtime, stored offset + `0x10000`):

```
pressed  code0 110820: {7FF,60,7CE38} {7FF,40,7CF20} {7FF,80,7CF20} term
pressed  code1 110840: {7FF,80,7CF20} term
pressed  code2 110850: {7FF,10,7D174} {7FF,40,7D174} {7FF,80,7CF20} term
pressed  code3 110870: {7FF,10,7CFD0} {7FF,40,7CD60} {7FF,80,7CF20} term
pressed  code4 110890: {7FF,10,7D054} {7FF,40,7D08C} {7FF,80,7CF20} term
released code0 1108B0: {7FF,30,7CEB0} {7FF,20,7D1D4} {7FF,20,7CEB0}
                        {7FF,10,7CF54} {7FF,40,7D0C4} {7FF,40,7CD60} term
released code1 1108E8: {7FF,50,7D010} {7FF,30,7D110} {7FF,10,7D110}
                        {7FF,20,7D110} {7FF,40,7D110} {7FF,60,7D110} term
released code2 110920: {7FF,20,7D1D4} term
released code3 110930: {7FF,20,7D1D4} term
released code4 110940: {7FF,20,7D1D4} term
```

`0x7CBC6 JMP 0x7CB96` / `0x7CC11 JMP 0x7CBE1` confirm the row loop; the
terminator row is all zero and matches `(0 & word) == 0` before its handler-0
test, which is how the loop ends.

### 1.4 Handler effects (outfield rows)

The row handler is called with `EAX = rec`; its return value != 0 stops the
scan. Handlers install via `FUN_0007D9A4` or write team role fields:

| handler | rows | body (addresses) | effect |
|---|---|---|---|
| `0x7CD60` | pressed3 want 40; released0 want 40 | `0x7CD6F` phase==2; team=[rec]; `0x7CD76` if `[team+0x7CB]!=0` return 1; `0x7CD7F` EBX=`[team+0x7B2]`; if rec==EBX → `0x7CD89` `[[rec]+0x7CB]=rec`, `[[rec]+0x820]=0x3C` (FU-74 §1 step 5 timer); else `0x7CDA4` require rec==`[0x577CA]` and `[EBX+0x20]==0`, then `0x7CDB4 CALL 0x786A0(EAX=tracked,EDX=controlled)` slot swap, `0x7CDB9` `[[tracked]+0x7CB]=controlled`, `+0x820=0x3C`; return 1 | arms the team `+0x7CB`/`+0x820` timer on the controlled or swappable tracked entity |
| `0x7CE38` | pressed0 want 60 | `0x7CE46` phase==2; type gate `flat[0x110680+type]&1` (`0x7CE54`); `[rec+0x99]==0` (`0x7CE63`); P=`[rec+0x28]`, `[P][0]` not in `{0x0C,0x59,0x5E}` (`0x7CE6C..0x7CE83`) | `0x7CE8A` install code `0x0B` invoke-now (`ECX=1`); returns type==0x0B |
| `0x7CEB0` | released0 want 30 | `0x7CEC1` phase==2; type gate `&1`; `[0x57A83]!=0`; sides differ (`0x7CEE5..0x7CEF5`) | `0x7CEFC` install code `8`; returns type==8 |
| `0x7CF20` | pressed0/1/2/3/4 want 80; keeper code3/4 (FU-74 §2) | `0x7CF2A` phase==2; type gate `&1` | `0x7CF48 CALL 0x79B58` (`if [rec+0x99]==0 → byte [rec+0x93]=0x10`); return 1 |
| `0x7CF54` | released0 want 10 | `0x7CF65` phase==2; type gate `&1`; `[rec+0x99]==0`; `[0x57A83]!=0`; sides differ | `0x7CFA9` install code `9`; returns type==9 |
| `0x7CFD0` | pressed3 want 10 | `0x7CFDD` phase==2; type gate `&3` (`0x7CFEE`) | `0x7CFFA` `[rec[0]+0x7BF]=rec`; `0x7D002` `[rec[0]+0x82A]=0`; return 1 |
| `0x7D010` | released1 want 50 | `0x7D01F` phase==2; type gate `&1` | `0x7D03F` install code `0x21`; return 1 |
| `0x7D054` | pressed4 want 10 | `0x7D061` phase==2; type gate `&3` | `0x7D07E` `[rec[0]+0x82A]=1`; return 1 |
| `0x7D08C` | pressed4 want 40 | `0x7D099` phase==2; type gate `&3` | `0x7D0B6` `[rec[0]+0x82A]=2`; return 1 |
| `0x7D0C4` | released0 want 40 | `0x7D0D2` phase==2; type gate `&1`; `[rec+0x99]==0` | `0x7D0F7 CALL 0x7E600(rec)` (installs `0x0E`, §1.8); returns type==0x0E |
| `0x7D110` | released1 wants 10/20/30/40/60 | `0x7D121` phase==2; type gate `&1` | `0x7D140` install code `7`; returns type==7 |
| `0x7D174` | pressed2 wants 10/40 | `0x7D182` phase==2; type gate `&1` (raw bytes, §Method); `[rec+0x99]==0`; `[rec+0x5D]==0` | `0x7D1AE` install code `0x23`; returns type==0x23 |
| `0x7D1D4` | released0 wants 20/20; released2/3/4 want 20; direct arm `0x7CB03` (type 3 + released bit `0x20`, `[0x57AB0]!=0`, rec != `[0x587AC]`) | `0x7D1DF..0x7D355`: if rec==`[0x57A83]` return 0; distance gate on `[0x57750]`/`[rec+0x69]`; builds a target triple from camera `0x57788`, adding `slot[+0x20]<<7` / `slot[+0x21]<<7` (read as `[slot+0x1D]>>24`/`[slot+0x1E]>>24`, or `[0x577BE]>>16<<4` / `[0x577C0]>>16<<4`); `0x7D2AE` if `[0x587A8]!=0` and phase in `{3,4,7}` searches from `rec+0x59` with skip `[[0x587A8]+0x8A]>>24` when the side matches else skip -1, otherwise `0x7D31B` searches from the camera target with skip -1, both via `FUN_0008DB6C`; if the result is `[0x587AC]` return 0; if non-null `0x7D336 CALL 0x786A0(rec,result)`, `[rec[0]+0x7CB]=0`, `EAX=[result+0x20]` → `FUN_00078B00` | control-selection switch; returns 1 |

`FUN_000786A0` (`0x786A0..0x786EB`) swaps the two records' `+0x20` slots and
zeroes seven words of the newly bound slot (`+4,+6,+8,+0xA,+0x14,+0x16,+0xC`);
`FUN_00078B00` (`0x78B00..0x78B14`) clears `slot+0xA`, `slot+0x16`,
`slot+0x24`; `FUN_00079B58` is the 4-instruction `[rec+0x99]==0 →
[rec+0x93]=0x10` cited above.

`FUN_0008DB6C` (`0x8DB6C..0x8DC49`) is the nearest-record search used by
`0x7D1D4`: over 11 records it skips `[rec+0x20]!=0`, the record at
`[team+0x7BF]`, the skip index in BX, `[rec+0x9A]!=0`, `[rec+0x98]!=0`, and
(when the loop index is 0 and the flag word at `[ESP+0x60]` is 0)
`[team+0x829]==0`; it ranks by `FUN_0008DC68` against the two-word target and
returns the nearest; if none and the flag word is non-zero it returns the first
record with no slot and `[rec+0x9A]==0` (`0x8DC1B..0x8DC41`).

### 1.5 The per-frame forced decision `FUN_0007C990`

Called from the tail `0x7CCB1` and from `FUN_0007DAB4 0x7DAF1` (44 callers;
`FUN_0007DAB4` sets `+0x92=0xFF`, `+0x89=0`, clears the slot via `0x78B00`,
then either `FUN_0007C990` when phase==2 and `[rec+0x8D]!=0` or installs code 0
via `0x7D9A4`). Input `EAX = rec`; body (`0x7C990..0x7CA50`):

```
0x7C993  team = [rec]; ctrl = [team+0x7B2]
0x7C99B  if (rec == ctrl) {
0x7C99F    opp = [[team+0x7A6]+0x7B2]
0x7C9AF    if (opp != 0 && ([opp+0x9F] & 1)) code = 6
0x7C9CA    else if ([rec+0x8E]>>24 == 5)     return        ; keep carrier
0x7C9DC    else                             code = 4
         } else if (rec == [team+0x7B6]) {
0x7C9EB    opp = [[team+0x7A6]+0x7B2]
0x7CA0A    if (opp != 0 && ([opp+0x9F] & 1)) code = 6
0x7CA13    else if (ctrl != 0 && ([ctrl+0x9F] & 1)) code = 3
0x7CA28    else                             code = 4
         } else                              code = 3
0x7CA34  if (code == (int8)[rec+0x91]) return
0x7CA48  FUN_0007D9A4(rec, code, EBX=0, ECX=0)             ; no invoke-now
```

So the record-local policy is: the team's `+0x7B2` entity is the one that
chases/applies pressure (4) unless the opponent's controlled entity carries the
ball (`+0x9F` bit 0, the same bit `FUN_0007D9A4` sets for codes 5/0x21, FU-74
§2), in which case code 6; a type-5 carrier keeps its state. The team's `+0x7B6`
entity does the same but falls back to code 3 when the primary entity carries
the ball. Every other record gets code 3 (coerced to `0x19` by `FUN_0007D9A4`
when `+0x8D == 0`).

### 1.6 Chase gate (code 8)

After `FUN_0007C990` returns, the same tail installs code 8 (action `0x81068`,
invoke-now 0) when all of (`0x7CC93..0x7CD24`):

```
phase == 2                                     (0x7CC8A)
flat[0x110680 + (rec[+0x8E]>>24)] & 1 != 0     (0x7CC9C)
rec != [rec[0]+0x7B2]                          (0x7CCB9)
rec != [rec[0]+0x7B6]                          (0x7CCC5)
[rec+0x69]>>16 < 0x50                          (0x7CCCD)
[0x57750] < 0x30                               (0x7CCD8)
[0x57A83] != 0                                 (0x7CCE1)
[[rec]+0x826] != [[0x57A83]+0x826]             (0x7CCED..0x7CCFC)
[rec+0x20] == 0                                (0x7CD03)
word [rec+0x81] == 0                           (0x7CD09)
[rec+0x5D] == 0                                (0x7CD13)
```

### 1.7 No-edge arm

When both slot words are zero (`0x7CC13..0x7CC7D`): if `(slot[+0x10] & 0xF0) == 0`
or phase != 2, skip; with `d = [rec+0x69]>>16`, either `d <= 0x30 || d >= 0x90`
(side-filtered: skip when `[0x57A83]` exists and shares the side; then require
`slot[+0x10] & 0xC0 != 0`) or `0x30 < d < 0x90` (unfiltered) falls through to
`0x7CC70`: copy camera `0x5774C` triple into `rec+0x4D/+0x51/+0x55` and call
`FUN_00079B58`.

### 1.8 `FUN_0007E600` (the `0x0E` decision, reached via `0x7D0C4`)

`0x7E600..0x7E7C4`: requires phase==2, type gate `&1`, `rec != [0x577CA]`,
`[rec+0x69]>>16 <= 0x180`; calls `FUN_00071B9C(EAX=4, EDX=stack)` and accepts a
band at stack `+4` in `[0x20,0x60]`; `FUN_0008DCD4`/`FUN_0008DD70` compute the
target metric; rejects unless the target is inside the x/z bounds and the
side-dependent field bounds; then computes the facing difference
`(angle - [rec+0x7D]) & 0x3FF` (`0x7E776 SUB AX,[ESI+0x7D]; AND AH,3`), mirrors
it over 0x200 when > 0x200, and installs code `0x0E` invoke-now only when the
result `<= 0x100` (`0x7E7A4..0x7E7B2`). Its callers are not enumerated (open
leg).

### 1.9 Function without callers

`0x7CDD8` (gate phase==2/type `&1`/`[rec+0x99]==0`, installs code `0x0A`,
returns type==0x0A) has **no xrefs** (`get_xrefs_to 0x7CDD8` → 0) and is not in
any row table read above; its invocation route is an open leg.

## 2. S4 — the match clock/phase machine `FUN_0008AF38`

**Anchor correction (errata 1):** the slice note called `FUN_0008AF38`
(sole frame-chain caller `0x4B1A6`, FU-67 §4) a tracker/decide candidate. It is
the match clock + phase machine: `[0x57AC1]` accumulates the frame delta and
carries into `[0x57AB4]`/`[0x57AB6]` modulo 0x3C (`0x8AFA2..0x8B079`), phase
`[0x57AC2]` indexes the inline CS jump table flat `0x8AF28` (four arms
`0x8B1DD/0x8B22F/0x8B2E9/0x8B33F`, §Method), and the arms compare `[0x57AB6]`
against the half lengths `[0x5881A]` (phase 0/1) and `[0x5881C]` (phase 2/3)
plus the added-time word `[0x57ABA]`, setting the end flag `[0x5882D]` or
advancing with `ECX=1` (`0x8B1DD..0x8B3A9`). The advance block
(`0x8B3AE..0x8B58A`) posts ids `0xB8/0xB9/0xBA/0xBB/0xBC/0xC2` via
`FUN_0008F188`, compares the scores `[0x57AC5]`/`[0x57AC7]`, calls
`FUN_0008C374` (code 0x12/0x13) and `FUN_0008B9CC`, then zeroes `[0x57AB6]`
and increments `[0x57AC2]`. Its tail is the entity-selection call in FU-67 §1:
`0x8B623` phase in `{2,0x10}` and `[0x5781D]!=0` → `CALL 0x88940`; phase 0xC
clears `[0x57ABA]`/`[0x5882D]` (`0x8B64A`); then `[0x4C32A]` gates
`FUN_0008BAF0` and `[0x58808]` is called as a callback (`0x8B65B..0x8B678`);
the function returns `[0x58822]`. A phase-indexed byte table at flat `0x1106AD`
is read at entry (`0x8AF4B MOVZX DI,[EAX+0x106AD]`) and its `1`/`2` values gate
the entry/accumulation path (`MOVSX EDX,DI; CMP EDX,1; ...`); the table content
is not asserted (open leg).

## 3. S4 — the tracker/decide pass `FUN_00072AC4`

Sole camera-chain caller `0x73756` (FU-67 §4.1). 766 instructions,
`0x72AC4..0x736A9`. Structure:

### 3.1 Gates

```
0x72ACA  if ([0x5781E]!=0 && [0x5781D]!=0 && [0x5759D]==0) CALL 0x72478
         else [0x5759D] = 0
0x72AF4  if ([0x575E8]==0 && [0x577CA]!=0 && [0x57A83]==0 &&
            (int16)[0x577BE] > 0) CALL 0x721C8            ; ball-only scan arm
0x72B1E  if (phase != 2) { [0x57A87]=0; [0x57A8B]=0; return }
0x72B3C  tracked=[0x57A87]; controlled=[0x57A83]
0x72B47  if (tracked == controlled) goto 0x73114
0x72B4F  if (controlled != 0) goto 0x72C6C
         ; else init arm 0x72B57
```

`FUN_00072478` is FU-67 §2's command pass; `FUN_000721C8`
(`0x721C8..0x7226D`) computes the scan speed `[0x5758D]` from the camera
displacement (`|[0x57758]| <= 0xD0`, `|[0x5774C]| > 0xD0`,
`[0x577C6]!=0`; sign of `[0x5774C]` picks `(0xD0 - [0x57758])` or
`(-0xD0 - [0x57758])`, times `[0x577C6]>>16` divided by `[0x577C4]>>16`, plus
`[0x57760]`) and sets `[0x575E8]=1`.

### 3.2 The three tracking arms

* **No controlled entity** (`0x72B57..0x72C67`): initialises the tracking block
  (`[0x5758C]=1`, `[0x5759E]=0`, `[0x575E8]=0`, `[0x5758D]=0x7FFFFFFF`), copies
  camera `0x5774C` to `0x5759F` and `0x57770` to `0x575B7`
  (`[0x575C7]=[0x577EE]>>16`), stores the previous tracked `[0x575DA]` and its
  team `[0x575D2]`, calls `FUN_00092998(EAX=1, EDX=2, EBX=[team+0x823]>>24)`
  into `[0x57599]` (the 25-slot history ring, FU-67 §3.4), negates
  `[0x575A7]`/`[0x575BF]` on side 1, stores the opponent object
  `[[0x575DA]]+0x7A6` in   `[0x575D6]`, stores the RNG `FUN_000CB2A4` in
  `[0x57591]`, zeroes `[0x575ED]`/`[0x575E9]`, and inits 11 slot records of
  stride `0x1A` (the byte at `+0` 0, dword `+1` 0, dword `+9` 0x3C0; first at
  `0x575F1`, last at `0x576F1`, `0x72C38..0x72C5B`); then `[0x57A8B]=[0x57A87]`.
* **Tracked != controlled, controlled exists** (`0x72C6C..0x730F7`): for the
  controlled object's `[+4][0]` == `0x18DA`/`0x18DE` calls `FUN_000651F0(4)`/
  `(0xA)` (`0x72C6C..0x72C8D`); if `[0x57A87]!=0` re-inits the changed-target
  block (`0x73062..0x730F3`); if `[0x57A83]==[0x57A8B]` accumulates
  `[0x57A97] += [0x57A8F]`; otherwise (`0x72CBC..`) builds the new target block
  `0x57710..0x5773B` (RNG `0x46 - (|[0x57754]|>>7)` plus `&0xF` into
  `[0x5771A]`, target position from `[0x57A83]+0x59`, opponent object
  `[[0x57A83]][+0x7A6]+0x7B2` into `[0x57726]`, camera into `0x575AB`,
  `FUN_000795B4(0x5759F, 0x575AB, 0x575CC)`, `[0x57A97]=0`) and then posts the
  distance/side/ring-result commands listed in §3.4.
* **Tracked == controlled** (`0x73114..0x7368B`): if the stored opponent
  `[0x57726]` equals `[[[0x57A83]]+0x7A6]+0x7B2`, compares the controlled and
  opponent records to the goal line (`y = ±0xB10`) via `FUN_0008DC68` and posts
  id `0x65` once per entry (`[0x5772A]` latch, `0x7312A..0x731FB`,
  `0x73200..0x732A9`), else latches `[0x5772A]=1` and refreshes `[0x57726]`
  (`0x732AB..0x732C6`); then the goal-side position checks on
  `[0x57A83]+0x61`/`+0x75` vs `[0x57737]±0x690` post `0x96`
  (`0x73309..0x733BC`), and the `[0x57A97]` timer section posts `0xD9`, `0x68`,
  `0xB2`, `0xD8` (`0x73404..0x7368B`). If controlled == 0 and
  `[0x57A8B]!=0`, it ends by calling the lane builder
  `FUN_00072270([[[0x57A8B]]+0x7A6])` (`0x7368C..0x7369E`).

### 3.3 Lane slots `FUN_00072270` (sole caller `0x7369E`)

`FUN_00072270` (`0x72270..0x72474`) takes `EAX` = the opponent object
(`[[tracked]+0x7A6]`), starts at `object+0xB2` and walks stride `0xB2` while
`EBP` (slot offset) runs `0x1A..0x11E` — 11 slots at flat `0x575F1 + EBP`,
stride `0x1A`. Per record it skips `[rec+0x9A]!=0`, `[rec+0x98]!=0`,
`[rec+0x69]>>16 >= 0x1E0` and `[slot+0]!=0`; with `[0x5758C]!=0` it requires
`[rec+0x6B]>>16 * [0x5774C] + [rec+0x6D]>>16 * [0x57754] > 0` and sets
`slot.distance = 0x960`, else it keeps the record whose `[rec+0x69]>>16` is
smallest. Then it writes `slot+1` (approach scalar), `slot+5` (RNG
`FUN_000CB2A4`), `slot+9` (distance), `slot+0xD` (the `SETG` of a comparison
over products of `[rec+0x6B/+0x6D]` and `[0x577BE]/[0x577C0]`, `0x723df..0x723f3`),
copies the camera `0x5774C` triple to `slot+0xE`, and publishes the slot into
`[0x575E9]` or `[0x575ED]` depending on the `slot+0xD` flag and on the slot
distance being better than the current best (`0x723FC..0x72449`).
So `[0x575E9]`/`[0x575ED]` are the two best lane slots; `FUN_00072AC4` reads
their `+9` distances in the `0x5C`/`0x55` legs (`0x72F7C..0x72FA4`).

### 3.4 Command ring `FUN_0008F188` and consumer `FUN_0008F2C4`

`FUN_0008F188(EAX=id, EDX=target, EBX=code)` (`0x8F188..0x8F2C3`) gates on
`[0x4C32A]==0 && [0x4C312]!=0`, sets `[0x5B36C]`/`[0x5B370]`/`[0x5B374]` for
`EBX==0x40`/`0x8`/`0x20|0x21`, and writes one 10-entry ring record stride
`0x20` at flat `0x5AAE0` (`entry[+0] = id`, `entry[+4] = FUN_000CB2A4()`,
`entry[+8] = 0`, `entry[+0xC] = code`, `entry[+0x10..0x18] = target+0x59`
triple when target != 0, `entry[+0x1C] = target`), writing at index
`([0x5AADC]+9) % 10` and then `[0x5AADC]=index`, `[0x5AAD8]++`
(`0x8F1E6..0x8F2B9`). The tracker's posted ids this slice: `0x55`, `0x58`,
`0x59`, `0x5A`, `0x5C`, `0x64`, `0x65`, `0x68`, `0x6E`, `0x96`, `0xB2`,
`0xB6`, `0xD8`, `0xD9` (`0x72E41`, `0x72E88`, `0x72EE2`, `0x72F3A`, `0x72F4F`,
`0x72FB2`, `0x72FD9`, `0x7304E`, `0x731D7`, `0x73328`, `0x73396`, `0x73482`,
`0x7359B`, `0x735CA`, `0x7360C`, `0x7364E`, `0x73674`); `FUN_00072478`
posts its own set (FU-67 §2).

`FUN_0008F2C4(EAX=threshold)` (`0x8F2C4..0x8F32E`) drains the ring:
decrements `[0x5AAD8]`; for each entry, if `entry.id < 0xDD` and
`threshold <= entry[+0xC]`, loads the handler `flat[0x110BB4 + id*4]`
(CS-relative, `+0x10000`; §Method) and calls it. `FUN_0008F0C4`
(`0x8F0C4..0x8F144`) registers the ring pointer as a task in the `0x5B380`
table (7 slots stride `0x1C`); `FUN_00091BC4` is another ring writer. The
`0x110BB4` handler bodies are not decomposed (open leg). So the tracker's
"decision" output is a command-ring event stream, not an action install; the
outfield action codes of §1 are chosen inside `FUN_0007CA54` itself.

## 4. Structures and fields (evidenced this slice)

Record/team (stride `0xB2`, FU-67 §3.2, new fields in bold):

| offset | width | evidence |
|---|---|---|
| `+0x18` | dword | installed action function, called `0x7CD2B` |
| `+0x20` | dword | bound control slot; `+4`/`+6` edge words, `+0x10` previous word, `+0x1D/+0x1E` bytes, `+0x20/+0x21` flag bytes (`0x7CB0D`, `0x7D25A`) |
| `+0x28` | dword | pointer; first byte compared `0xC/0x59/0x5E` (`0x7CE6C`) |
| `+0x4D/+0x51/+0x55` | 3 dwords | output triple; camera copy `0x7CC7A..0x7CC7C` |
| `+0x5D` | dword | zero-tested before code `0x23` (`0x7D1A8`) |
| `+0x69` | dword | high word distance (`0x7CC2F`, `0x7D1FC`, `0x722A5`) |
| `+0x6B/+0x6D` | words | lane dot direction (`0x722CC..0x722F2`) |
| `+0x75` | word | velocity sign in the goal-side legs (`0x732FE`, `0x7336F`) |
| `+0x7F` | dword | high word timer limit (`0x7CA66`) |
| `+0x81` | word | countdown (`0x7CA5C`, `0x7CD09`) |
| `+0x89` | dword | zeroed by reset (`0x7DAC4`) |
| `+0x8D` | byte | active flag gating `FUN_0007C990` on reset (`0x7DAE6`) |
| `+0x8E` | dword | high byte type; gate `flat[0x110680+type]` (`0x7CC93`) |
| `+0x91` | byte | current action code (`0x7CA34`) |
| `+0x92` | byte | set `0xFF` by reset (`0x7DABA`) |
| `+0x93` | byte | countdown; set `0x10` (`0x7CAB2`, `0x79B61`) |
| `+0x99` | byte | zero-gate in six handlers (`0x7CE03`, `0x7CF7F`, `0x7D19F`) |
| `+0x9F` | byte | bit 0 = carrier marker; read on controlled entities (`0x7C9AF`, `0x7CA17`) and set by installer codes 5/0x21 (FU-74 §2) |
| team `+0x7B2` | dword | controlled entity (`0x7C995`, `0x7CCB9`) |
| team `+0x7B6` | dword | secondary entity (`0x7C9E3`, `0x7CCC5`); zeroed at reception `FUN_0007A084 0x7A38E/0x7A3A2`, written by action paths `0x7E91F/0x808F4/0x80995/0x8D0D4` |
| team `+0x7BF` | dword | chosen entity (`0x7CFFA`); compared in `FUN_0007C990` (`0x7CB6D`) and `FUN_0008DB6C` (`0x8DB9A`) |
| team `+0x7CB` | dword | entity pointer armed with `+0x820=0x3C` (`0x7CD8B`, `0x7CDBB`); zeroed by `0x7D1D4` (`0x7D33E`) |
| team `+0x820` | word | countdown written `0x3C` (`0x7CD93`, `0x7CDC3`) |
| team `+0x826` | byte | side (`0x7CEF5`, `0x7D2E8`) |
| team `+0x829` | byte | search gate (`0x8DBA6`) |
| team `+0x82A` | byte | mode 0/1/2 (`0x7D002/0x7D07E/0x7D0B6`), cleared `0x7F93B/0x7F983` |
| team `+0x7A6` | dword | object pointer; `[[team+0x7A6]+0x7B2]` = opponent controlled (`0x7C99F`, `0x72D3D`) |

Tracker globals:

| address | evidence |
|---|---|
| `0x57A83` | user/controlled entity (`0x7CB55`, `0x72B41`) |
| `0x57A87` | tracked entity cache (`0x72B3C`, `0x730F8`) |
| `0x57A8B` | tracked entity (`0x72D8E`, `0x7368C`) |
| `0x57A8F` | tracked timer increment (`0x733EE..0x733FE`) |
| `0x57A97` | tracked timer (`0x73414`, `0x730F8` region) |
| `0x5758C` | lane-scan mode flag (`0x72B63`, `0x722C3`) |
| `0x5758D` | scan speed (`0x72B7D`, `0x72263`) |
| `0x57599` | history-ring result (`0x72BDA`, `0x72DD3`) |
| `0x5759D` | once-per-pass flag (`0x72ADC`, `0x724C8`) |
| `0x575E8` | ball-only arm flag (`0x72AF4`, `0x72210`) |
| `0x575E9`/`0x575ED` | best lane slot pointers (`0x72426`, `0x72449`, read `0x72F7C`) |
| `0x575F1` | 11 lane slots, stride `0x1A` (`0x722B6`, `0x7241F`) |
| `0x5AAD8`/`0x5AADC` | command-ring count/cursor (`0x8F2D2`, `0x8F2B9`) |
| `0x5AAE0` | 10 command-ring entries, stride `0x20` (`0x8F1FE`) |
| `0x110BB4` | command-id handler table (CS-relative; entry 0 → `0x8F37C`) |
| `0x1106AD` | phase-indexed byte table (`0x8AF4B`) |
| `0x110680` | type-flag table (FU-74 §5) |
| `0x1106E0` | action function table (FU-74 §2) |
| `0x1109D0`/`0x1109E4` | outfield pressed/released row-pointer tables |
| `0x110820..0x110940` | outfield mask/want/handler rows |

## 5. Outfield action-code space evidenced

Codes installed by this slice's outfield paths (all through `FUN_0007D9A4`,
runtime fn = flat `0x1106E0[code]` per FU-74 §2):

| code | install site | runtime fn |
|---|---|---|
| 3 (→0x19 when `+0x8D==0`) | `FUN_0007C990 0x7CA2F` | `0x7E1A4` |
| 4 | `FUN_0007C990 0x7C9DC/0x7CA28` | `0x7E7C8` |
| 6 | `FUN_0007C990 0x7C9C5/0x7CA0C` | `0x801B4` |
| 7 | `0x7D140` | `0x814B0` |
| 8 | `0x7CEFC`, `0x7CD24` | `0x81068` |
| 9 | `0x7CFA9` | `0x80A00` |
| 0x0A | `0x7CE11` (`0x7CDD8`, unreferenced) | `0x81738` |
| 0x0B | `0x7CE8A` | `0x81908` |
| 0x0E | `0x7E7B2` | `0x82710` |
| 0x21 | `0x7D03F` | `0x85214` |
| 0x23 | `0x7D1AE` | `0x82F84` |

The full table holds 40 entries 0x00..0x27 (FU-74 §2); keeper-installed codes
0/0x19/4 and the CPU variants 0x1A/0x1B/0x1C are FU-74 §3/§4.

## 6. Port: `fifa96_outfield`

`include/fifa96_loader/fifa96_outfield.h` +
`src/fifa96_loader/fifa96_outfield.c` (caller-owned data, static const tables,
no globals, no comments, `-fifa96_err_t` for invalid arguments). Scope: the
mask/want dispatch data + scan of §1.3, the dispatch-code selection of §1.2,
the forced decision of §1.5 and the chase gate of §1.6.

| original | port |
|---|---|
| row tables flat `0x110820..0x110940`, rows `{0x7FF mask, want, handler}` | `fifa96_outfield_rule` + `fifa96_outfield_pressed_rules(code)`/`_released_rules(code)` (static const, 10 tables, handler = runtime id) |
| row test `(row[+0] & word) == row[+2]` | `fifa96_outfield_rule_match(rule, word)` |
| row loop with handler return-gate and terminator (`0x7CB96..0x7CC11`) | `fifa96_outfield_rules_run(rules, word, call, context, &handler)`; the callback models the handler, the terminator ends the scan |
| code selection `0x7CB0D..0x7CB7E`, raw-word gate | `fifa96_outfield_edge` + `fifa96_outfield_dispatch_code` (fields: `pressed`, `released`, `type`, `high_577ee_ge_50`, `tracked`, `user_absent_or_self`, `chaser`) |
| `FUN_0007C990` | `fifa96_outfield_forced_action(state, current, &next)`; returns 1 = install, 0 = keep/none, `-INVALID` |
| chase gate `0x7CC93..0x7CD24` | `fifa96_outfield_chase_action(state, current, &next)` |
| `FUN_0007D9A4` install, handler bodies, `FUN_0007E600`, selection switch, no-edge arm, tracker/ring | not ported (globals/objects/pointer tables) |
| machine subset `0x7CABA..0x7CC82` + no-edge `0x7CC13..0x7CC7D` + type gate `0x110680` | **ported (M2 arms-and-wiring Task 14 / FU-142 Appendix K):** `fifa96_outfield_input_row` (input-row dispatch, the either-or pressed/released scan, the `[0x157AB0]` pre-gate, the no-edge arm and the forced-decision/chase tail) and `fifa96_outfield_chase_gate` (the 26-byte `0x110680` `&1` gate composed with `chase_action`); `test_outfield` fixtures. **Task 1 erratum (M2 playability-legs): the row-04 handler body `0x7E7C8..0x7F141` is now ported as `fifa96_outfield_row04_step` and wired (FU-142 K.5); the machine subset stays a separate seam. The row-08 body remains OL-70a.** |

## 7. Tests (`tests/test_outfield.c`, suite 63 → 64)

* Layout `_Static_assert`s on all port struct offsets.
* Tables: pressed code0/code3 and released code0/code1/code4 rows incl. handler
  ids, order and the zero terminator; out-of-range codes return NULL.
* `rule_match`: mask/want exactness (`0x60` vs `0x40`, `0x61`, `0x620`,
  `0x860`), terminator `(0 & word)==0`, NULL.
* `rules_run`: reject continues to later rows, accept stops, no match leaves the
  out untouched, first-zero-then-accept passes to the second duplicate row,
  NULL rules/callback/out.
* `dispatch_code`: both raw words zero → 0 (code untouched); `0x40` → 0;
  `0x0001` raw → 0; precedence high(2) over type 5(1), type 5(1),
  tracked+user(3), chaser(4), tracked without user and not chaser → 0;
  NULL.
* `forced_action`: controlled+opponent ball → 6; controlled type-5 → keep;
  controlled → 4; second+opponent ball → 6; second+controlled ball → 3;
  second → 4; other → 3; equal current → 0; NULL.
* `chase_action`: all gates → 8; current 8 → 0; one failing gate each
  (phase, type gate, controlled/second, distance `0x4F`/`0x50`, camera
  `0x2F`/`0x30`, user, sides, unbound, timer, third) → 0; NULL.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_outfield.c src/fifa96_loader/fifa96_outfield.c` runs clean.
`make test`: 63/63 before, **64/64 after**.

## 8. Errata (quoted)

* Slice anchor "`FUN_0008AF38` … (tracker/decide candidate)" — **corrected**:
  `FUN_0008AF38` is the clock/phase machine (as FU-67 §4.3 names it); the
  tracker/decide pass is `FUN_00072AC4` → `FUN_00072478` (§3).
* FU-67 §5 S4 "tracker + decision table … mapped here; post table open" —
  **extended**: the caller's arms, the lane builder `FUN_00072270`, the
  best-lane pointers `[0x575E9]/[0x575ED]`, the command ring `0x5AAE0` and its
  consumer `FUN_0008F2C4`/`0x110BB4` are derived here; the post table itself
  remains FU-67 §2.
* FU-74 §9.1 "handler bodies `0x7CD60 … 0x7D1D4` … not derived" — **derived
  here** for the outfield rows (§1.4); `0x7CDD8` (code 0x0A) has no xrefs and
  stays open.
* FU-67 §4.3 "`FUN_0007CA54` … calls `(*(code*)rec[6])()` … under a condition
  cluster" — **refined**: the action call is `[rec+0x18]` at `0x7CD2B`; the
  condition cluster is the phase-2/type-gated tail (§1.5/§1.6).
* `disassemble_bytes` at `0x7D190` desyncs (§Method); the raw bytes
  (`read_memory 0x7D187`) decode the same type gate as the neighbouring
  handlers.
* **§1.3 pressed/released scan (first-hand, M2 arms-and-wiring Task 14).**
  The pseudo-code's fall-through from the pressed loop into the released
  table is wrong: the pressed terminator (`0x7CBB0 JZ 0x7CC82`) and an
  accepting handler (`0x7CBBD JNZ 0x7CC82`) both jump to the tail, and the
  released table is reached only from `0x7CB8A` (`pressed & 0xFF0 == 0`).
  The real machine therefore runs **either** the pressed **or** the released
  scan. Also unquoted by §1.3: the `0x7CAC4..0x7CB08` pre-gate (`byte
  [0x157AB0] != 0`, `rec != [0x1587AC]`, type 3, `released & 0x20`) may call
  `0x7D1D4(rec)` before any code selection. Both are ported exactly by
  `fifa96_outfield_input_row` (FU-142 Appendix K.3).

## 9. Open legs

1. **`0x7CDD8`** (installs code 0x0A) has no xrefs; whether it is dead or
   reached indirectly is unknown.
2. **`FUN_0007E600` callers** beyond the row `0x7D0C4`, and the semantics of
   `FUN_00071B9C`/`FUN_0008DCD4`/`FUN_0008DD70` inputs, are not derived.
3. **`0x7D1D4` selection switch**: `[0x587A8]`/`[0x587AC]`/`[0x58730]` writer
   identities and the `FUN_0008DB6C` flag word at `[ESP+0x60]` producers are
   not located; `FUN_0008DB6C`'s fallback return is quoted only.
4. **`team+0x7B6` writers/consumers** outside the quoted sites
   (`0x7E91F/0x808F4/0x80995/0x8D0D4` zero/assign actions) are not decomposed.
5. **Lane object layout**: `FUN_00072270` starts at `object+0xB2` and walks 11
   records ending at `object+0x7A6` (the field itself for a team block); the
   object reached through `[team+0x7A6]` is not identified (FU-67 open leg
   carries over).
6. **Command ring**: the `0x110BB4` handler bodies, `FUN_0008F2C4`'s threshold
   argument producer, and the `0x5B380` task table are not derived.
7. **`FUN_0008AF38`**: the phase byte table flat `0x1106AD`, the
   `FUN_0008B9CC`/`FUN_0008BAF0`/`[0x58808]` tail effects and the id meanings
   `0xB8..0xC2` are not decomposed.
8. **`+0x82A` modes** (0/1/2 from `0x7CFD0`/`0x7D054`/`0x7D08C`, cleared by
   `0x7F93B`/`0x7F983`) are quoted as values only; their consumers are not
   located.
9. **Type identities**: type 5 (carrier checks), type 3 press arm, and the
   `[rec+0x28]` byte values `0xC/0x59/0x5E` are quoted only as compared values
   (FU-67/FU-74 open legs carry over).
10. **Machine subset closed (M2 arms-and-wiring Task 14, FU-142 Appendix K):**
    the input-row dispatch, the no-edge arm, the forced-decision integration,
    the chase gate and the flat `0x110680` type gate are now ported and tested
    (`fifa96_outfield_input_row`, `fifa96_outfield_chase_gate`, `test_outfield`).
    Still open here: the 14 handler bodies and their `0x7D9A4` installs, the
    `0x7D1D4` selection switch (leg 3), and the row-08 body (leg 9, OL-70a).
    **Task 1 erratum (M2 playability-legs): the row-04 body
    `0x7E7C8..0x7F141` is ported and wired as `fifa96_outfield_row04_step` /
    `fifa96_match_action_04` (FU-142 Appendix K.5, OL-70 closed; OL-72 for its
    unmodeled inputs).**

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x8AF38, 0x7CA54, 0x72AC4, 0x7DAB4, 0x7A084, 0x72270,
0x721C8, 0x8F188, 0x8F2C4, 0x8F0C4, 0x8DB6C, 0x786A0, 0x78B00, 0x651F0,
0x7E600, 0x79B58; `disassemble_bytes` 0x7C990, 0x7CD60 (with the §Method
desync), 0x7D2A0, 0x7D174, 0x8DB3A, 0x7F37C, 0x8F37C; `read_memory` 0x8AF28
(16 B), 0x1109D0 (48 B), 0x110820 (304 B), 0x110BB4 (64 B), 0x7D187 (32 B);
`get_xrefs_to` 0x7CA54, 0x7C990, 0x7DAB4, 0x7CDD8, 0x72270;
`search_instructions` 0x575, 0x5aad8, 0x5aae0, + 0x82a], + 0x7b6].

Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_outfield.h`,
`src/fifa96_loader/fifa96_outfield.c`, `tests/test_outfield.c`, `CMakeLists.txt`
(one library/test block). `game/FIFAPCCD96.iso` untouched; `fifa96.rep/**`
churn not staged.
