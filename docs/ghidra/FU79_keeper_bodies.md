# FU-79: the keeper action family — the 7 keeper-classified bodies

Follow-on to FU-74 (S7 keeper machine/selection) and FU-76 (action table/class
census): fully derive the bodies of the seven keeper-classified action codes
`0x19..0x1F` — `0x746E4`, `0x7662C`, `0x76D28`, `0x77728`, `0x74EB0`,
`0x7550C`, `0x76380` — and port the clean pieces into `fifa96_keeper`.

Result in one line: **the seven bodies are one family of stage machines around
the shared staging block `0x58730..0x58746`: `0x19` (hold/claim) tracks the
deepest keeper in `team+0x7C7`, falls back to the camera guard/weighted
placement and dives at the opponent carrier's velocity; `0x1A`/`0x1B`/`0x1D`
are 5-arm stage machines (`CS:` tables `0x76618`/`0x76D14`/`0x74E9C`) that
drive out from the camera with a shared vector helper `0x74E2C` and stage the
ball through `0x7A490`; `0x1C` is the dive/lunge (`0x77728`) using the
`0x14E04` sine table and the `0x76B28` steering helper; `0x1E`/`0x1F` are the
claim placement and the arm/claim stage machine (event `0x28`, ball-ack
`0x58746`, install `0x1B`).**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). Where the Ghidra listing defines complete flow, the
  quotes are Ghidra `disassemble_function`/`disassemble_bytes` output; where
  the listing prunes jump-table arms or the function body stops at a cut
  (FU-74 §9.3), the bytes were read back with `read_memory`/a `run_script_inline`
  memory dump and decoded linearly; the decode was checked instruction by
  instruction against the Ghidra listing on every overlapping window.
* **Address mapping (FU-57/FU-74/FU-76, used verbatim).** Code addresses are
  true link addresses; data immediates render through object-4 as
  `A → flat A+0x100000`; stored code pointers and inline `CS:` tables resolve
  through `+0x10000`.
* **Tooling errata.** `disassemble_function 0x746E4` stops at `0x7471D` (the
  decompiler/analyzer cut); the body continues to `0x74E2B` and was recovered
  from a memory dump. `0x76380`'s arm `0x7647E`, its post-RNG blocks
  `0x764E4..0x76507`/`0x7652B..0x76567` and `0x74EB0`'s pre-staged table reads
  are pruned by the listing (undefined bytes) and were recovered the same way.
  No decompiler output is quoted anywhere.
* The jump tables themselves are quoted as raw dwords read from Ghidra memory
  (§1.2); the arm addresses are the stored value + `0x10000`.

## 1. The keeper slots and their stage tables

### 1.1 Slot map (FU-76 §1.4/§2)

| code | runtime | FU-76 class | body range (this slice) |
|---|---|---|---|
| `0x19` | `0746E4` | keeper hold/claim | `0x746E4..0x74E2B` |
| `0x1A` | `07662C` | keeper reposition A | `0x7662C..0x76B27` |
| `0x1B` | `076D28` | keeper reposition B | `0x76D28..0x77727` |
| `0x1C` | `077728` | keeper dive/lunge | `0x77728..0x77E94` |
| `0x1D` | `074EB0` | keeper close-down | `0x74EB0..0x754E1` |
| `0x1E` | `07550C` | keeper claim/throw | `0x7550C..0x755D3` |
| `0x1F` | `076380` | keeper arm state | `0x76380..0x765C2` |

All seven are entered as `CALL [rec+0x18]` with only `EAX = rec` (FU-74 §2);
each prologue saves `EBX/ECX/EDX/ESI/EDI/EBP` and takes `EBP = EAX`.
`0x19` additionally uses `ECX` as the record register.

### 1.2 Inline stage tables (read at flat addresses; stored + `0x10000`)

`0x1A` (`0x7668B JMP CS:[EAX*4+0x66618]`, timer gate `stage > 4 → return`):

```
0x76618  {0x66693, 0x666DD, 0x666F5, 0x66AAD, 0x66AE8}
runtime  {0x76693, 0x766DD, 0x766F5, 0x76AAD, 0x76AE8}
```

`0x1B` (`0x76DEE JMP CS:[EAX*4+0x66D14]`, same gate):

```
0x76D14  {0x66DF6, 0x66E3B, 0x66FC7, 0x6769F, 0x676C1}
runtime  {0x76DF6, 0x76E3B, 0x76FC7, 0x7769F, 0x776C1}
```

`0x1D` (`0x74F58 JMP CS:[EAX*4+0x64E9C]`, same gate):

```
0x74E9C  {0x64F60, 0x65045, 0x6524B, 0x6529A, 0x6549B}
runtime  {0x74F60, 0x75045, 0x7524B, 0x7529A, 0x7549B}
```

`0x1F` (`0x76432 JMP CS:[EAX*4+0x66370]`, gate `stage > 3 → return`):

```
0x76370  {0x6643A, 0x6647E, 0x664B0, 0x66580}
runtime  {0x7643A, 0x7647E, 0x764B0, 0x76580}
```

`0x1C` has no table: `0x7774F CMP AL,1 / JC / JNA` selects stage 0 (`0x77762`),
stage 1 (`0x7778E`), stage 2 (`0x779D1`), anything else → `0x779DE`.

## 2. Code `0x19` — hold/claim (`0x746E4..0x74E2B`)

### 2.1 Prologue constants and the deepest-keeper tracker

```
0x746ED  ECX = rec
0x746EF  EDX = 0x9F0      ; staged at [esp+0x38]
0x746F4  EBX = 0xAE0      ; staged at [esp+0x44]
0x746F9  [rec+0x9E] = 1
0x74700  AH = 1
0x74702  ESI = 0xC0       ; staged at [esp+0x40]
0x74707  [0x57C5D] = 1    ; CPU-keeper decision flag (FU-74 §3.3)
0x7470D  EAX = [rec]      ; team block
0x74717  EBX = [team+0x7C7]           ; current tracked keeper (may be 0)
0x74721  DX  = word [rec+0x6B]
0x74725  CMP DX,word [EBX+0x6B]
0x74729  JNL 0x74731                  ; keep tracked if candidate.x >= tracked.x
0x7472B  [team+0x7C7] = rec           ; else the candidate becomes tracked
```

`+0x6B` is the record's position-x word (compare is signed 16-bit). The three
stack constants are the clamp limits used later: `0x9F0` (`[esp+0x38]`),
`0xC0` (`[esp+0x40]`), `0xAE0` (`[esp+0x44]`).

### 2.2 The `[rec+0x98]` early out and the control-slot branch

```
0x74731  if (byte [rec+0x98] != 0) { copy pos +0x59.. -> out +0x4D..; goto 0x74CAE; }
0x74748  EBP = [rec+0x20]                            ; bound control slot
0x7474F  if (EBP != 0) {
0x74757     if ([0x57A49]>>24 == 1) { copy pos -> out; return; }   ; human side
0x7476F     else FUN_00079C20(rec, slot[+0x1D]>>24, slot[+0x1E]>>24); return;
          }
```

`+0x98` is the installer's event-arm byte (FU-74 §2); `[0x57A49]>>24` is the
human/CPU phase byte. The slot branch's second arm is exactly the code-0
movement primitive (`FUN_00079C20`, FU-76 §3.1) applied to the slot direction
bytes.

### 2.3 No slot, phase != 2 → method tail

```
0x7478C  if (phase != 2) {
0x74C8E     EDX = &out; EBX = -1; EAX = rec;
0x74C98     CALL [rec+0x1C]                 ; record method, not decomposed
0x74CA3     if ([0x57A49]>>24 == 1) word [rec+0x7B] = 1;
0x74CAE     epilogue
```

### 2.4 No slot, phase == 2 — the camera guard

```
0x7479D  EAX = [rec]; EBP = [[team+0x7A6]+0x7B2]   ; opponent controlled entity
0x747A5  EDX = [0x57A83]                            ; user controlled entity
0x747B1  if (EDX == 0 && [rec+0x69]>>16 < 0x2D0 && [0x57750] == 0 &&
             team[+0x7C7] == rec &&
             (EBP == 0 || rec[+0x6B] >= EBP[+0x6B])) {
0x747EC     copy camera 0x5774C/0x57750/0x57754 -> out +0x4D/+0x51/+0x55
0x747F7     if (|out.z| > 0xAE0) out.z = (side==0) ? -0xAE0 : 0xAE0
0x74821     FUN_00079B58(rec)
0x74831     return
          }
```

This is the "last man back, ball still at z==0" guard: the keeper is placed on
the camera (pitch/ball focus) with a z clamp, and the `+0x7B` timer is armed by
`FUN_00079B58` (FU-76 `TMR93`). The port takes the camera triple, the sign and
the body side as caller inputs (§11).

### 2.5 The prediction block (`0x74832..0x74ACC`), block level

The remaining phase-2 path is a large safety/interception predictor:

* `0x7483C FUN_00071C40(EAX=0x90, EDX=1)` then `0x7484F FUN_00071B9C` fill a
  12-byte local from a 10-bit angle; `0x74854` tests camera z sign against the
  body side, `0x7487B`/`0x74893` require non-zero camera velocity components
  and `[0x57750] >= 0` or `[0x57821] != 0`, `0x748A9` requires `[0x57A83]==0`.
* `0x749A8..0x74AA4` projects the keeper/ball/camera onto the camera velocity
  line (integer `IMUL`/`IDIV` by the squared velocity), accepting the
  projection only when `0 < t < 0x96` and the resulting point passes the
  `0xC0` height and `±0xB08` z tests; on acceptance it stores the point to
  `0x57C48`, the scalar to `[0x57C44]` and copies the point to `out`
  (`0x74AA6..0x74AC1`).
* On rejection (`0x74ACC`) the opponent-carrier dive branch (§2.6) is tried,
  else the weighted camera fallback (§2.7).

The middle of this block (globals `0x71C40/0x71B9C/0x57821` semantics) is an
open leg (§14).

### 2.6 The opponent-carrier dive target (`0x74ACC..0x74B71`)

```
0x74ACC  if (EBP != 0 && EBP == [0x57A83] &&
             rec[+0x69]>>16 < ([rec[+4]+0xB]>>24 <<4) + 0x1E0 &&
             0x74584(...) && 0x745EC(...)) {
0x74B28     scale = [rec+0x69] >> 20
0x74B34     out.x = [0x5774C] + scale * ([EBP+0x73]>>16)
0x74B4D     out.z = [0x57754] + scale * ([EBP+0x75]>>16)
0x74B5C     out.y = 0
0x74B63     FUN_00079B58(rec); return
          }
```

`+0x73/+0x75` is the opponent controlled entity's velocity pair (FU-74 §5).
This is the keeper's dive at the ball carrier: camera position plus
`(distance>>20) * carrier velocity`.

### 2.7 The weighted camera fallback (`0x74B72..0x74C8D`)

```
0x74B72  base = (side==0) ? -0x9F0 : 0x9F0
0x74B99  base += FUN_0008DC50((int16)([0x57752]>>16), 4)     ; >>4, trunc
0x74BBA  clamp base to ±0xAE0
0x74BD0  if (rec[+0x69]>>16 >= 0x780 || |[0x57778]| <= 0x9F0 || word [0x577C2]==0)
             out.x = sign-preserving (|cam.x|/8 + |cam.x|/16 + |cam.x|/32)
0x74BF6  else { out.x = FUN_00074694(base); clamp to ±0xC0 }
0x74C7C  out = local triple; return
```

The weighted camera expression is exact: for `cam.x >= 0`,
`x = (x>>3) + (x>>4) + (x>>5)`, and the negative case mirrors it with `NEG`.

### 2.8 The guard clamp `FUN_00074CDC` (`0x74CDC..0x74D84`)

Standalone code block in the recovered body gap with **no static xref**
(listing search; likely an unbound record method alongside the `0x74CB8`
re-aim block that calls `0x8DE8C`/`0x786A0`):

```
x = out.x; x = clamp(x, -0x390, 0x390)
side==0: z = clamp(z, -0xAE0, -0x7E0)
side!=0: z = clamp(z,  0x7E0,  0xAE0)
copy stack triple back to out
```

### 2.9 The clear vector `FUN_00074E2C` (`0x74E2C..0x74E99`)

Called by `0x1D` (`0x752D0`) when the keeper has no control slot, and by
`0x75C36` (undefined code in the `0x755D4..0x7612F` gap) when
`[0x57C2E]>>16 < 0x5A0`:

```
0x74E2C  ECX = rec; EBX = out; ESI = 0x780
0x74E38  RNG = FUN_00092AC8(); EDX = RNG % 0x780
0x74E41  [out+2] = EDX - 0x3C0                       ; int16 x, [-0x3C0,0x3BF]
0x74E4B  range = 0x780 + 2 * (int8)([[rec+4]+0xD]>>24)
0x74E5A  RNG = FUN_00092AC8(); EDX = RNG % range
0x74E61  [out+4] = EDX + 0x3C0                       ; int16 z
0x74E78  if (side == 1) [out+4] = -[out+4]
0x74E83  [out] = FUN_0008DC68(x, z)                  ; octagonal distance
```

The 6-byte vector layout is `{word distance, word x, word z}` — the same
layout `FUN_0008DCD4` writes (`0x58738..0x5873D` ball staging, FU-73 §1).

## 3. Code `0x1A` — reposition A (`0x7662C..0x76B27`)

Prologue: phase `0xD` and `[team+0x826] == [0x57AAC]>>24` sets `[0x57A83]=rec`
(`0x76634..0x7665C`); `+0x89 += [0x57A64]`; stage `+0x92` selects table
`0x76618` (§1.2).

| stage | arm | evidenced |
|---|---|---|
| 0 | `0x76693` | `FUN_000765C4(rec)` (tracked-teammate predicate, FU-74 §4); if it returns non-zero → `FUN_0007DAB4(rec)` (reset/install `0`) then `FUN_0007D9A4(rec, 4, 0, 1)` → **install `4`**; else `[rec+0x9E]=1`, stage += 2, falls into stage 2 |
| 1 | `0x766DD` | stage++ and falls through to stage 2 (`0x766F5`) |
| 2 | `0x766F5` | `FUN_00079B1C(rec)`; `[0x57C2C]` direction machine (§3.1); common movement/face/event block (§3.2); stage++ arms |
| 3 | `0x76AAD` | if `byte [rec+0x44] != 0` → stage++ and return; else `[0x57A83]=rec` and copy pos → out |
| 4 | `0x76AE8` | `FUN_00079B1C`, `FUN_0007DAB4`, and for CPU side `FUN_0008A938(5, side, 1)` |

### 3.1 The `[0x57C2C]` direction machine (`0x766FC..0x767E7`)

```
[0x57C2C]==0x2D: if ([0x577EE]>>16 < 0x60 && [0x577BC]>>16 > 0x12 &&
                     [0x577F8]>>16 < 0x1E && |rec.z| < 0xA90)
                     { [0x57C2C]=0x5C; signal 6; }
[0x57C2C]==0x2E: if ([0x577BC]>>16 > 0x14 && [0x577F8]>>16 < 0x1C &&
                     [0x57750] < 0x90 && (RNG&0x1F) > ([rec+4]+0x10>>24))
                     { [0x57C2C]=0x54; signal 6; }
                 else { AX = word[0x57750]-0x70; if (AX>0) {
                          EDX=min((int16)AX,0x60); EAX=FUN_000702F8(EDX);
                          word[rec+0x87]=0; word[rec+0x85]=AX; } }
[0x57C2C]==0x54: if (rec[+0x69]>>16 > 0x40 && rec[+0x89] > 0xF) {
                     FUN_0007DAB4(rec); return; }
```

### 3.2 The common movement/event block (`0x76816..0x76AA7`)

```
FUN_00079C50(rec, -word[0x577C2], -word[0x577C0])   ; face + sector byte +0x8E
word [rec+0x7F] = FUN_000CD474(rec[+0x6B]>>16, rec[+0x6D]>>16)
FUN_0006E598(rec, [0x57C2A]>>16, type8, 0)
if (byte [[rec+0x28]] == 0x54) {
    RNG picks a shift (0..3) and sign for [0x577C0]/[0x577C2];
    distance 0x8DC68 -> word[rec+0x71]; [0x5774C]/[0x57754] += camera velocity;
    FUN_0007DAB4(rec); return;
}
if ([0x57750] > 0x20 && [0x577BC]>>16 > 0xA) { FUN_000974DC(4); FUN_000974F0(0x4B0); }
... selects EAX in {5,4,9,1,7,0} and calls FUN_0008F188(EAX, rec) ...
EAX = ([0x577BC]>>16 < 8 || [0x577F8]>>16 > 0x78) ? 0xF
    : ([0x577F8]>>16 > 0x3C ? 1 : 0xE); FUN_000651F0(EAX)
FUN_0008A938(7, side, 0); stage++
then at 0x76AAD: if (byte [rec+0x44] != 0) stage++ and return;
else copy pos -> out and [0x57A83] = rec
```

The `0x14E04` 10-bit-angle sine table rotation appears at `0x768B8..0x76905`
(the `0x54` camera-velocity arm), and the `0x76A95`/`0x76AB3` stage increments
are the tail into `0x76AAD`/`0x76AD2`.

## 4. Code `0x1B` — reposition B (`0x76D28..0x77727`)

Prologue mirrors `0x1A`: phase `0xD` side match sets `[0x57A83]=rec` through
the same `0xF331/0xF339` offset pair (`0x76D61..0x76DBF`: camera x/z =
keeper position + `(int8)table[type8] << 4`, `[0x57750] = rec.y + 0x38`,
`[rec+0x9B] = 1`, `FUN_000700F4`, `[0x57A83]=rec`). Stage table is `0x76D14`
(§1.2); `+0x89 += [0x57A64]`.

| stage | arm | evidenced |
|---|---|---|
| 0 | `0x76DF6` | `[rec+0x9E]=1`; clears `0x57C54/0x57C57/0x57C58/0x57C59/0x57C5A/0x57C5B`; stage++; `FUN_0008DCD4(rec+0x59, rec+0x4D, rec+0x65)`; face via `0x79C50`; `word[rec+0x7F]=atan2(delta)`; falls into stage 1 |
| 1 | `0x76E3B` | `[rec+0x51] += 0x20`; rotate the delta by `[rec+0x7D]` using table `0x14E04`/`0x795A4` to write `+0x67/+0x69`; `out = pos + delta`; `[0x57C58] = (|out.z| >= 0xAB0)`; cross-product flag `[0x57C54]`; event code table `0xE17D + side + 2*(out.y>0x50)` via `0x6E598`; `FUN_00076B28(rec, 0xA0, 0x60)`; `[0x57C55]=0`; stage++ |
| 2 | `0x76FC7` | the long ball-rush test (§4.1) |
| 3 | `0x7769F` | `if (byte[rec+0x44] != 0) & side/human conditions: face/event 0x33, stage++` |
| 4 | `0x776C1` | `FUN_0007DAB4(rec)` (reset); copy pos → out; if `rec != [0x57A83]` install `0x19`; else if human side return, else `FUN_0008A938(5, side, 1)` |

### 4.1 The stage-2 rush test (`0x76FC7..0x77636`)

Block-level, with globals as exact operands: gates `[0x57C55]==0`,
`rec != [0x57A83]`, `rec != [0x577CA]` (tracked teammate), then a pocket
condition from `[rec+0x28]` first byte (`0x3C` or `[rec+0x3A]>>24`), the
per-side settings word `[0x4C2F6]` (sets the thresholds `0x780/0x5A0`, the
steering seeds `0x60/0xA0`, and the extra offsets `+0x70/+0x10`), the tracked
teammate distance/`0x74584` predicate, `[0x57750]` and the ball/keeper
distance. On success it stages the rush (`0x71C94`, event `0x1A`, ring and SFX
calls), sets `[0x57C59]=1`, `[0x57C57]=1`, `[0x57823]` one-shot counter pair
and `word [rec+0x81]`; on the alternate arm it writes the camera placement
(`0x774DD`) with the `0xF331/0xF339` offsets plus `FUN_000700F4` and the side
event `0x1A`/`0x4`/`5`/`0x19` selection (`0x77428..0x77618`). The exact
floating predicates are quoted at instruction level in the provenance dump;
the model is not ported (§11).

## 5. Code `0x1C` — dive/lunge (`0x77728..0x77E94`)

Prologue `EBP=rec`, `+0x89 += [0x57A64]`; stage dispatch (`0x7774F`,
§1.2).

* **Stage 0** `0x7776A`: `[rec+0x9E]=1`, `[0x57C56]=0`, stage++ → falls into
  stage 1.
* **Stage 1** `0x7778E`:
  ```
  0x7778E  FUN_0008DCD4(rec+0x59, rec+0x4D, rec+0x65)   ; delta pos -> out
  0x7779C  FUN_00079C50(rec, -[0x577C2], -[0x577C0])
  0x777B4  word [rec+0x7F] = atan2(rec[+0x6B], rec[+0x6D])
  0x777C7  angle = atan2(word[rec+0x65], [rec+0x51])    ; delta.dx vs out.y
  0x777E6  if (|angle| < 0x55) {
  0x777F6     rec.x += FUN_0008DC50(delta.dx, 1)         ; trunc(dx/2)
  0x77817     rec.z += FUN_0008DC50(delta.dz, 1)
  0x77832     EBX=0x3C; ESI=0x80; EDI=0x40
           } else {
  0x77856     delta.dx += 0x40; rotate delta by [rec+0x7D] through
              table 0x14E04 with FUN_000795A4 -> +0x67/+0x69, out = pos + delta
  0x778EF     cross-product flag [0x57C54]
  0x7791F     event code = 0x36/0x34 (out.y < 0x40, cross sign) else
              0x3D + (cross sign)
  0x7798D     ESI=0x50; EDI=0x80
           }
  0x77997  FUN_0006E598(rec, event, type8, 0)
  0x779AC  FUN_00076B28(rec, EBX=steer_x, EDX=steer_z)  ; §5.1
  0x779B9  stage++; +0x89=0
  ```
* **Stage 2** `0x779D1`: `if (byte[rec+0x44] != 0) FUN_0007DAB4(rec)`.
* **Common tail** `0x779DE..0x77E8B`: when `word[rec+0x81]==0` and
  `[rec+0x28][0] == 0x3C : [rec+0x3A]>>24 == 2` (or else `== 3`), it predicts
  the dive point (`0x77A15..0x77B26`), tests keeper-to-point distance and the
  `0x76B28` steering thresholds, fires SFX (`0x651F0`, `0x974DC(1)`,
  `0x974F0(0x4B0)`), increments the `0x57ACC/0x57AD0` one-shot counters when
  `[0x57823]` is set (`0x77BAD..0x77BF5`), then randomizes the camera velocity
  (`0x77233..0x77275`; components `(RNG&7)+8`, angle corrected by `0xCD514`,
  side-scaled by `[rec+4]+0x1B`), re-stages the ball through `FUN_00071C94`
  (`0x77DC6`) and finally sets `word [rec+0x81] = FUN_0006E444(&rec[0x28]) + 0xA`
  and `[0x57C56]=1` (`0x77E6F..0x77E84`).

### 5.1 The steering helper `FUN_00076B28` (`0x76B28..0x76D11`)

Inputs `EAX=rec`, `EBX`, `EDX` (target x/z words):

```
AX = (word)[rec+0x51] - BX + 0x20 -> word[rec+0x83]
if (<= 0) { [rec+0x83]=0; [rec+0x85]=0; }
else { clamp [rec+0x83] to 0x60; [rec+0x85] = FUN_000702F8([rec+0x81]>>16); }
word local = clamp((word)[rec+0x65] - DX + 0x20, 0x10, 0x70)
angle = atan2(word[rec+0x67], word[rec+0x65])
out = pos; out.x/z += rotated(magnitude local>>16) through 0x14E04/0x795A4
FUN_000795B4(rec+0x59, out, rec+0x65)      ; writes the +0x65 triple
if [rec+0x85] != 0:
    speed = clamp(([rec+0x65]>>16) / ([rec+0x83]>>16), 0, 0xF)
    word[rec+0x73], word[rec+0x75] = rotated speed components
word[rec+0x71] = FUN_0008DC68(...)
```

This is the rush steering applied at the end of each lunge/dive arm; it is
cited block level, not ported.

## 6. Code `0x1D` — close-down (`0x74EB0..0x754E1`)

Prologue builds a camera-target local from global `[0x644D0]` plus the
per-side table `[0x57C5E] + side*0x18 + ([0x5777C]>=0 ? 0 : 0x24)`
(`0x74EC3..0x74F11`); for `stage < 3` it sets `[0x57A83]=rec` and calls
`FUN_0007876C` (slot merge). Stage table `0x74E9C` (§1.2); `+0x89 += [0x57A64]`.

* **Stage 0** `0x74F60`: `if (byte[rec+0x44]!=0 && (rec[+0x89]>=0x1E ||
  [0x4C32A]!=0))` → camera place `0x700F4`, `0x73E08`, per-side z offset
  `[ebp+side*2-0x12]>>16`, `0x79B6C`, event `0x26`, `0x4C324`, `[0x57AB2]=1`,
  `0x918CC`, stage++; `[0x4C32A]` timer arm `0x974DC(0x1E)`/`[rec+0x9E]`.
* **Stage 1** `0x75045`: `0x700F4` with `[rec+0x9B]`, `0x79B6C`, optional
  `0x744D4`, slot-button handling (`0x36200/0x361A4/0x4C380/0x4C320/0x361B0`),
  `0x4B0` timer arm (`0x8F188(0xA4, 4, 0)`), `0x4C31C`; stage++.
* **Stage 2** `0x7524B`: copy camera → out; `if (rec[+0x69]>>16 > 0x40)` arms
  the `0x4B0` timer and returns; else stage++.
* **Stage 3** `0x7529A`: `[0x58743]=1`; `FUN_00092820(rec, 6)`; then
  `if (slot != 0) FUN_0008DCD4(rec+0x59, 0x57A77, 0x58738)` else the clear
  vector `FUN_00074E2C(rec, 0x58738)` (§2.9); trajectory `[0x58736]>>16`
  compared to `0x5A0`/`0x3C0`, `0x8DE8C`, `0x8DD70`, table `0x114E04`,
  `0x795A4`, then **ball staging** `FUN_0007A490(rec, 0x58738,
  [0x58736]>>19 or >>4, event 0x30 or 0x31)` (`0x753E2/0x75405`), ring
  `0x8F188(0x22, 4, rec)`, `0x786A0`, `FUN_0008A938(0xB, side, 0)`, clear
  `[0x57AB2]`, `0x4C380`, stage++, `0x79B1C`.
* **Stage 4** `0x7549B`: `0x79B1C`; `if (byte[rec+0x44]==0)` tail; else
  `[0x57AB2]=0`, `FUN_0007DAB4(rec)`; tail `if ([0x57AB2] && stage>0)
  FUN_0004C31C(rec+0x59)`.

The clear vector (ported, §11) plus the `0x7A490` staging is the ball-outcome
leg of the close-down; the surrounding camera/slot/UIT blocks are cited, not
ported.

## 7. Code `0x1E` — claim/throw placement (`0x7550C..0x755D3`)

Full body (59 instructions, no branches other than the two gates):

```
0x75517  [EBP-0xC] = rec
0x7551A  EAX = rec[+0x8F]>>24                            ; stage
0x75528  if (stage < 6 && rec[+0x20] == 0) CALL 0x7876C(rec)
0x7553E  if (stage >= 3) return
0x75553  if (byte [rec+0x9B] != 0) return                ; already has ball
0x7555C  EAX = rec[+0x8B]>>24                            ; type8
0x75565  EBX = (int8)[EAX + 0xF331] ; EBX <<= 4          ; object-4 table
0x75571  EDX = rec[+0x59] ; EDX += EBX
0x75579  [0x5774C] = EDX                                 ; camera/place x
0x7557F  EDX = (int8)[EAX + 0xF339] ; EDX <<= 4
0x7558E  EAX = rec[+0x61] ; EAX += EDX
0x75593  [0x57754] = EAX                                 ; camera/place z
0x7559B  EAX = rec[+0x5D] + 0x38
0x755A9  [0x57750] = EAX                                 ; camera/place y
0x755C0  byte [rec+0x9B] = 1                             ; possession flag
0x755C7  CALL 0x700F4                                    ; camera place
0x755CF  [0x57A83] = rec                                 ; controlled/actor
```

So a keeper at `stage < 3` without the ball (`+0x9B == 0`) is placed at its own
position plus the per-type sprite offset (`0x10F331/0x10F339`, object-4; two
signed bytes `<<4`), 0x38 above its y, takes possession (`+0x9B=1`) and becomes
`[0x57A83]`. This is the only body in the family that is fully linear; it is
the primary port target (§11).

## 8. Code `0x1F` — arm/claim stage machine (`0x76380..0x765C2`)

Prologue `EBP=rec`; if `rec[+0x20]==0 && team[+0x828]!=0` → `FUN_0007876C`
(slot merge). `[0x57C5C]=0`; `stage = rec[+0x8F]>>24`.

```
0x763B1  if (stage < 2) {
0x763B6     out.x=0; out.y=0;
0x763C7     out.z = (side==0 || phase==0x10) ? 0xB10 : -0xB10
0x763F5     if (stage < 1) FUN_00079B6C(rec, 0x5774C, &out)
         }
0x76409  +0x89 += [0x57A64]
0x76425  if (byte [rec+0x92] > 3) return
0x76432  JMP CS:[stage*4 + 0x66370] {0x7643A, 0x7647E, 0x764B0, 0x76580}
```

* **Stage 0** `0x7643A`: `[rec+0x9E]=1`; `BH = [0x5882A]`; if `BH==0` return;
  else `FUN_0006E598(rec, 0x28, type8, 0)`, `+0x89=0`, stage++ → falls into
  stage 1.
* **Stage 1** `0x7647E`: `if ([0x58730]==0 || byte[0x58746]==0) return`;
  else stage++ (to 2) → falls into stage 2.
* **Stage 2** `0x764B0`: if `rec[+0x20] != 0` → `0x76568` (stage++ to 3, tail);
  else the randomized dive lunge:
  ```
  0x764BA  out = pos
  0x764D4  divisor = 0x64 - 2*((int8)[[rec+4]+0xF] + (int8)[[rec+4]+0x10])
  0x764DF  rem = RNG % divisor
  0x764E6  sign = (rem < 0x19) ? ((word[0x5873A] > 0) ? 1 : 0)
                               : (RNG & 1)
  0x76508  if (sign == 0) sign = -1
  0x7650E  out.z += ([rec+0x61] > 0) ? -0x30 : 0x30
  0x76526  mag = (RNG & 0x3F) + 0x30
  0x76534  out.x = sign * mag
  0x76541  out.y = (RNG % 0x30) + 0x20
  0x7655C  FUN_0007D9A4(rec, 0x1B, BX=1, ECX=1)   ; install 0x1B, invoke now
  ```
* **Stage 3 / `0x76568` tail**: stage++, copy pos → out; if slot and
  `word[slot+4] != 0` → `FUN_00076130(rec)` (the keeper input handler, §8.1);
  if `type8 == 0x1F && [0x57A4C]==0` → `FUN_0007DAB4(rec)`.

### 8.1 The keeper input handler `FUN_00076130` (`0x76130..0x7636C`)

Sole caller `0x76599` (the `0x1F` tail) with `EAX = rec`. Gates and behavior
(quoted exactly; `AX` is the phase-2 camera-bounds "inside" flag):

```
0x7613B  if (rec[+0x20] == 0) return 0
0x76148  phase = [0x57A4A]>>24; EAX = 1
0x76156  if (phase != 2) {
0x761A5     if ([0x57A49]>>24 == 1 && byte[0x5882A] != 0) continue;
0x761BC     else return 0;
         } else {
0x7615B     if (|cam.x| > 0x420) EAX = 0
0x76179     else if (side != 0 ? cam.z >= 0x7B0 : cam.z <= -0x7B0) EAX = 0
         }
0x761C8  type gate: type8 != 0x1F && (0x110680[type8]&1) == 0 -> return 0
0x761F2  if (rec == [0x57A83] || type8 == 5) return 0
0x76214  saved = (int16)[rec+0x91]
0x7621F  if (AX == 0) {
0x76224     if (rec[+0x69]>>16 > 0x60) goto 0x76350 (return changed?);
0x76233     install 4
         } else if (rec[+0x69]>>16 <= 0x40) {
0x76251     copy camera -> out; install 0x19
         } else if (slot[+0x20] == 0 && slot[+0x21] == 0) {
0x76286     copy pos -> out; out.y = 0xA0; install 0x1C
         } else {
0x762BC     out.x = rec.x + (int8)slot[+0x20]*0x70
            out.z = rec.z + (int8)slot[+0x21]*0x70; out.y = 0x40
            clamp out.z to ±0xAF0; FUN_0008DCD4(rec+0x59, out, rec+0x65)
            install 0x1B + (dist >= 0x70)
         }
0x76350  return (int16)([rec+0x91] != saved)
```

So a slot-bound keeper in phase 2 rushes (`4`) when the camera is outside the
`±0x420`/`±0x7B0` box and the keeper is far (`>0x60`), holds (`0x19`) when
close (`<=0x40` with the camera inside), and otherwise takes the slot direction
(`0x1B` far / `0x1C` close, or `0x1C` from rest).

## 9. Decision → action → ball-outcome flow

1. **Selection (FU-74).** `FUN_0008D8EC` walks record 0 through
   `FUN_000782D0`; the forced tail installs `0` (timer `+0x81`), `0x19`
   (phase≠2 / not `team+0x7B2` / opponent-controlled type 5), keeps the
   current action (own type 5) or `4`, gated by `0x110680[type]&1`
   (`0x784D3..0x78576`). `0x19 → 0x746E4`, `4 → 0x7E7C8`, `0 → 0x7DB10`
   (FU-76 table).
2. **CPU decision (FU-74 §4).** For an uncontrolled keeper in phase 2 with
   `[0x57C5D]` set, `FUN_00077EAC` either stages a target into `0x58738` and
   enters `7` (`FUN_0006DC88`) or installs `0x1A`/`0x1B`/`0x1C` through the
   `0x67E98` jump arms. `0x19` can then run the dive at the carrier (§2.6);
   `0x1F` runs the input handler that arms `0x1B`/`0x1C` (§8.1).
3. **Stage machines (this slice).** `0x1A` → `0x1B` (rush) → `0x1C`
   (lunge/dive) → `0x1D` (close-down, ball out) → `0x1E` (claim placement) →
   `0x1F` (arm then `0x1B`). `0x1F` consumes the ball-actor/`0x58746` ack pair
   that FU-73 §1 defines.
4. **Ball outcome.** `0x1D` stages a clearance through `FUN_0007A490` with
   event `0x30`/`0x31` and puts the random clear vector (`0x74E2C`) or the
   slot-relative delta (`0x8DCD4`) into `0x58738`; `0x1E` takes possession
   (`+0x9B=1`, `[0x57A83]=rec`); `0x19`'s intercept writes `out` only (the
   record tail `FUN_00079B58` arms the action timer). No keeper body calls
   `FUN_0007A490` except `0x1D`; the others hand the ball to the shared
   staging/possession paths (FU-73/FU-78).

## 10. Citation table

| body | range | reader used |
|---|---|---|
| `0x19` | `0x746E4..0x74E2B` | Ghidra `disassemble_function` + memory dump (`0x7471D..0x74E2B`) |
| `FUN_00074E2C` | `0x74E2C..0x74E99` | Ghidra listing; memory dump for `0x74E3F..0x74E60` |
| `0x1D` | `0x74EB0..0x754E1` | Ghidra `disassemble_function` + memory dump |
| `0x1E` | `0x7550C..0x755D3` | Ghidra `disassemble_function` |
| `0x76130` | `0x76130..0x7636C` | memory dump (Ghidra prunes from `0x76235`) |
| `0x1F` | `0x76380..0x765C2` | Ghidra `disassemble_function` + memory dump (`0x7647E`, `0x764E4..0x76507`, `0x7652B..0x76567`) |
| `0x1A` | `0x7662C..0x76B27` | Ghidra `disassemble_function` |
| `FUN_00076B28` | `0x76B28..0x76D11` | memory dump |
| `0x1B` | `0x76D28..0x77727` | memory dump |
| `0x1C` | `0x77728..0x77E94` | Ghidra `disassemble_function` |
| tables | `0x76370`, `0x76618`, `0x76D14`, `0x74E9C` | Ghidra `read_memory` (raw dwords) |
| helpers | `0x8DC68` dist, `0x8DC50` trunc-shift, `0x795A4` mul-hi, `0x8DCD4` delta | Ghidra `disassemble_function` |
| `0x7DAB4` reset | `0x7DAB4..0x7DB0C` | Ghidra `disassemble_function` |

## 11. Port: `fifa96_keeper` bodies

`include/fifa96_loader/fifa96_keeper.h` + `src/fifa96_loader/fifa96_keeper.c`
(caller-owned data, no globals, negative `fifa96_err_t`, no comments).

| original | port |
|---|---|
| `FUN_0008DC68` (`0x8DC68..0x8DCD2`) | `fifa96_keeper_distance(x, z, &d)`: `d = max + ((min>>2 + min>>1)>>1)` on absolute values (the branch pair collapses to this; §Method) |
| `FUN_0008DCD4` (`0x8DCD4..0x8DDB`) | `fifa96_keeper_vec_from_delta(from, to, &vec)`: `dx/dz` = 16-bit deltas, `distance` from those |
| `FUN_00074E2C` (`0x74E2C..0x74E99`) | `fifa96_keeper_clear_vector(rng_x, rng_z, range_attr, side, &vec)`: RNG remainders are caller-provided (`rng % 0x780`, `rng % (0x780+2*attr)`) |
| `0x7550C..0x755CF` (`0x1E`) | `fifa96_keeper_claim_place(pos, offset_x, offset_z, stage, has_ball, has_slot, &place, &helper_request, &claimed)`: helper request = `stage<6 && !has_slot`; claimed = `stage<3 && !has_ball`, place = pos + offset<<4 (x/z), y+0x38 |
| `0x74721..0x7472B` (`0x19`) | `fifa96_keeper_hold_track(deepest_x, candidate_x)` → 1 when candidate.x < deepest.x |
| `0x747EC..0x74820` (`0x19`) | `fifa96_keeper_hold_guard(cam, side, &out)`: `out=cam`; `|z| > 0xAE0 → -0xAE0` (side 0) / `+0xAE0` (side 1) |
| `0x74B28..0x74B5A` (`0x19`) | `fifa96_keeper_hold_intercept(cam, distance, carrier_vx, carrier_vz, &out)`: `out.x = cam.x + (distance>>20)*vx`, `out.z = cam.z + (distance>>20)*vz`, `out.y = 0` |
| `0x74CDC..0x74D84` | `fifa96_keeper_guard_clamp(&p, side)`: x clamp `±0x390`; z clamp `[-0xAE0,-0x7E0]` (side 0) / `[0x7E0,0xAE0]` (side 1) |
| `0x764B0..0x76560` (`0x1F` stage 2 no-slot) | `fifa96_keeper_dive_target(pos, rng_a, divisor, rng_b, trajectory_z, rng_c, rng_d, &dive)`: exact sign/mag/z/y selection; requests install `0x1B` with invoke |
| `0x76380..0x765C2` (`0x1F` stage machine) | `fifa96_keeper_arm_step(...)`: stage 0 event `0x28`, stage 1 ball-ack gate, stage 2 slot split, stage 3 tail flags |
| `0x76693..0x766DD` (`0x1A` stage 0) | `fifa96_keeper_reposition_a_gate(has_tracked_teammate, &out)`: teammate → reset + install `4`; else stage advance 2 and `+0x9E=1` |
| `0x776C1..0x7771E` (`0x1B` stage 4 finish) | `fifa96_keeper_reposition_b_finish(is_controlled, human_side, &install_code, &notify_code)`: reset, copy pos, install `0x19` when uncontrolled, side notify `5` when controlled on CPU side |
| `0x777E6..0x77832` (`0x1C` stage 1 on-target arm) | `fifa96_keeper_lunge_track(delta, angle, &pos, &on_target, &steer_x, &steer_z, &event_code)`: `|angle| < 0x55` → pos += delta>>1, event `0x3C`, steer `(0x80,0x40)`; otherwise not ported |
| `0x1A`/`0x1B`/`0x1D` middle stage bodies, `FUN_00076130`, `FUN_00076B28`, `0x1C` rotated arm, `0x19` predictor/weighted fallback | not ported (globals/tables/record methods) |

## 12. Tests (`tests/test_keeper_bodies.c`, suite 67 → 68)

* `_Static_assert`s on every new struct's offsets.
* `distance`: identity, axis values, the `max + 3/8·min` mixed cases, signs,
  boundary `0x4000`, NULL.
* `vec_from_delta`: positive/negative deltas, 16-bit wrap, distance reuse,
  NULL.
* `clear_vector`: both RNG-remainder edges (`x=-0x3C0/0x3BF`), side negation,
  attribute-scaled range, distance field, NULL.
* `claim_place`: placement offsets, `y+0x38`, helper request below stage 6
  without a slot, suppression at stage ≥ 3 / has-ball, NULL.
* `hold_track`: smaller/larger/equal candidate, negative x, int16 edges.
* `hold_guard`: inside, both signs at the bound, side-dependent clamp, x/y
  untouched, NULL.
* `hold_intercept`: scaling by `distance>>20`, zero/negative shift, add to
  camera, y zeroed, NULL.
* `guard_clamp`: x both rails, side-0 z band, side-1 z band, interior values,
  NULL.
* `dive_target`: `rem < 0x19` trajectory sign, random-bit sign fallback, the
  zero-rem `-1` coercion, mag/`y` ranges, divisor 0 invalid, NULL.
* `arm_step`: stage 0 event gate, stage 1 ball-actor/ack gate, stage 2 slot
  split and install `0x1B`, stage 3 tail flags (`type8==0x1F` reset), stage 4
  no-op, NULL.
* `reposition_a_gate`: teammate reset+install `4`, else advance 2/flag, NULL.
* `reposition_b_finish`: uncontrolled install `0x19`, controlled human no-op,
  controlled CPU notify `5`, NULL.
* `lunge_track`: on-target move/event/steer, threshold boundary, negative
  angle, NULL.

ASan+UBSan: `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
tests/test_keeper_bodies.c src/fifa96_loader/fifa96_keeper.c` runs clean.
`make test`: 67/67 before, **68/68 after**.

## 13. Errata (quoted)

* FU-74 §9.3 / FU-76 §7 open leg 3 "keeper action functions `0x746E4`,
  `0x7662C`, `0x76D28`, `0x77728`, `0x74EB0`, … state machines are not
  derived" — **derived here** (§2–§8); the bodies' stage tables are quoted.
* FU-76 §2 code `0x19` "body continues past the Ghidra cut at `0x7471D`" —
  **confirmed and bounded**: the body runs to `0x74E2B`; `0x74E2C` is the
  clear-vector helper `FUN_00074E2C` (called from `0x1D 0x752D0` and
  `0x75C36`).
* FU-74 §4 "jump arms install `0x1A` (three arms), `0x1B`, `0x1C`" —
  **extended**: the same bodies reinstall within the family (`0x1F 0x7655C`
  installs `0x1B` with `BX=1`/`ECX=1`; `0x76130` installs `0x1C` at
  `0x762A4`, and `0x1B`+1 at `0x7634B`; `0x1A` installs `4` at `0x766B4`;
  `0x1B` installs `0x19` at `0x77719`). No direct-immediate install of
  `0x1D`/`0x1E` is in the FU-76 census (open leg).
* FU-74 §5 "`+0x73/+0x75` velocity pair" — **refined**: in the `0x19`
  carrier dive the velocity words are read at `[carrier+0x73]>>16` and
  `[carrier+0x75]>>16`, i.e. from the dword loads at `+0x71`/`+0x73`
  (`0x74B2E`, `0x74B47`).
* FU-74 §4.1 "`FUN_0006DBCC` target generation" — unchanged; this slice adds
  the receiving side: `0x74E2C` writes the same 6-byte `{dist,dx,dz}` layout
  into `0x58738` when the close-down has no slot (`0x752D0`).
* FU-76 §3.3 `0x1F` "installed `0x7655C/0x782A2`" — the `0x7655C` install is
  confirmed to be the stage-2 no-slot dive (`EDX=0x1B`, `BX=ECX=1`).

## 14. Open legs

1. **Install sites of `0x1D`/`0x1E`**: not in the FU-76 direct-immediate
   census; the computed-install arms are not located.
2. **`rec+0x1C` record method** called by `0x19` (`0x74C98`) is not
   decomposed.
3. **`0x19` predictor** (`0x74832..0x74ACC`): `FUN_00071C40`/`FUN_00071B9C`,
   the `0x57C44/0x57C48` latches, `FUN_00074584`/`FUN_000745EC` predicates and
   the `0x57821` gate are quoted at block level only.
4. **`0x1A`/`0x1B` mid stages**: the `0x8ED40`/`0x8F188`/`0x71C94` call
   semantics and the `[0x4C2F6]` per-side settings branches are cited, not
   decomposed.
5. **`0x1C` rotated arm** (`0x77856..0x7798D`): the `0x14E04` table rotation
   and the `0x36/0x34/0x3D/0x3E` event selection are quoted; the port covers
   only the on-target arm.
6. **`0x1D` camera-target construction** (`[0x644D0]`, `[0x57C5E]`,
   `[0x5777C]` tables) is quoted by address only.
7. **`0x1E` per-type sprite offsets** `0x10F331/0x10F339` (object-4) are not
   dumped; the port takes them as caller inputs.
8. **`0x1F` event flag `0x5882A`** and `[0x57A4C]` latch writers are not
   located.
9. **`FUN_00076130` gate values** (`slot[+0x20]/[+0x21]`, `0xAF0` clamp,
   `action+0x1B` install arithmetic) are cited; the handler is not ported.
10. **`0x755D4..0x7612F` gap**: undefined code (its `0x75C36` block calls
    `0x74E2C` and stages through `0x57C2E/0x57C30/0x57C32`), and the
    `0x74CB8`/`0x74CDC` blocks have no static xref; their record-method
    binding is unknown.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x746E4, 0x74EB0, 0x7550C, 0x76380, 0x7662C, 0x76D28,
0x77728, 0x76130 (`0x76235` mis-decoded), 0x8DC68, 0x8DC50, 0x795A4, 0x8DCD4,
0x79C50, 0x7876C, 0x7DAB4; `disassemble_bytes` 0x76432, 0x7643A, 0x7647E,
0x76493, 0x764E4, 0x7652B, 0x746E4 (cut recovery); `read_memory` 0x76370,
0x76618, 0x76D14, 0x74E9C (20 B each), 0x76370 (16 B); `run_script_inline`
(memory-range dumps to `/tmp/opencode/fu79/mem/*.bin` for the nine ranges, and
`Fu79List` for the 0x74000..0x79000 function map). Analysis-only outside the
port: no tool, capture-rig, ISO or Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_keeper.h`,
`src/fifa96_loader/fifa96_keeper.c`, `tests/test_keeper_bodies.c`,
`CMakeLists.txt` (one test block). `make test`: 67/67 before, **68/68 after**;
ASan+UBSan `test_keeper_bodies` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
