# FU-89: per-frame scene assembly — the render-list body `0x57754`, the delta/cull head `0x590B0`, and the depth threshold `[0x54350]`

Follow-on to FU-88 (open legs 1, 2, 3, 4, 5, 6 and the `0x57753` body cite).
This slice derives the render-list body at **`0x57754`** (FU-88 cited `0x57753`
— errata), its caller chain and cadence, the per-slot projection writes
`0x57D31`/`0x57D54`, the depth override `0x57D5C..0x57DFB`, the delta/cull head
of `FUN_000590B0` including the full `FUN_0005903C` compare, the depth
threshold setter `FUN_000589E0`/`[0x54350]` **with its reader**, the `0x3A`
rotation span of `FUN_00056E50`, and the two `0x4C654` callers; the clean
integer subset is ported as `fifa96_scene`.

Result in one line: **the live match render is
`FUN_00049830` (`0x498BA`) → `FUN_00058B68` → `FUN_00058AC4` → the body at
`0x57754`, which copies one staging triple `[0x54374]→[0x54370]`, seeds the
`0x18`-entry render list's depth keys at `0x4E7B0` from the **z of the rotated
jittered position of each 1-based slot value** (`[0x54370]=block+0x10C8`,
triples at `+0x10D4`), shell-sorts keys+values **descending** with
`FUN_000A1860`, gates each entry on `[0x55AB4] != -10000`, `[0x9A74][s*0xC]
<= 0x8E0` and `[0x54350] <= key`, projects the **clean** rotated triple
`[0x543AC]+s*0xC` to `[0x54364]+s*8` and the **jittered** one
`[0x543C0]+s*xC` to `[0x54388]+s*8` through two `FUN_0004C590(1, in, out, 0)`
calls, applies the caller's depth override (`rect[0x18..0x24]`) to the
jittered y (drawing or skipping), and blits each sprite through the 4-point
warp path (`0x4C654` + `0xA31B0`/`0xA3380`); `[0x54350]` is the **near-depth
threshold** returned in EAX by `FUN_00058A44` after `FUN_000589E0`
(`max(floor((0x14<<16)/(2·obj[5])), 0x78)`, or `[0x9088]` when
`obj[1] < [0x908C]`) — not a reader-less scale (FU-88 leg 2 closed:
`FUN_000589E0` returns it, `FUN_00058A44` passes it on as the driver's EDI);
and `FUN_000590B0`'s head subtracts the cached previous view record from `obj`
and, when the delta is exactly zero **and** `FUN_0005903C`'s six-dword compare
holds, sets `[0x9094]=0` and returns without rotating/projecting, otherwise
adds the delta to the first `0x10F` triples, adds `-obj` from `[0x54358]+0xCB4`
and runs `0xA3010`/`0x4C590` over `0x115`/`0x166` points.**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). `disassemble_function`/`disassemble_bytes` are the
  citation source. The `0x57754` body is not a Ghidra function (the address
  sits inside the analyzer's merged `FUN_00084021` blob); its bytes were
  dumped from Ghidra memory (`run_script_inline` writing a temp file) and
  decoded with `ndisasm -b32` following the control flow, checked against the
  Ghidra instruction stream wherever Ghidra delimits it (`0x57754..0x578B8`,
  `0x57D08..0x57DFF`, `0x58103..0x581ED`, `0x58210..0x5828E`).
* **Address mapping (FU-76/FU-84/FU-85/FU-88, restated).** Code/function
  addresses equal true link addresses; a **data immediate** `A` is storage at
  flat `A+0x100000` (`[0x54324]` → `0x154324`, `[0x4EAE8]` → `0x14EAE8`);
  stored code pointers and inline `CS:` tables resolve through `+0x10000`. A
  stored **data-object** pointer resolves through `+0x100000`.
* Every numeric claim is quoted from the listings/reads; unproven items are
  open legs (no guessed labels).

## 1. Cadence and call graph

`FUN_00049830` (body `0x49830..0x49A50`, the live match update) calls the
render driver at `0x498BA`:

```
0x498B5  MOV EAX,[0x7318]
0x498BA  CALL 0x58B68
```

`FUN_00058B68` (36 insns, `EAX`=arg0, `EDX`=arg1) is the frame composer:

```
0x58B71  CALL 0x58A44                 ; view build; EAX = [0x54350] (§4)
0x58B76  MOV EBP,EAX
0x58B7C  CALL 0x62B40                 ; 2D/panel pass...
0x58B8F  CALL 0x6331C
0x58B98  CALL 0x5FEAC
0x58BA3  CALL 0x610C4
0x58BA8  MOV EBX,EBP; MOV EDX,EDI; MOV EAX,ESI
0x58BAE  CALL 0x58AC4                 ; scene assembly + entity draw
0x58BB3  MOV EAX,ESI
0x58BB5  CALL 0x58538                 ; 5 control-slot marker pass (§3.1)
```

`FUN_00058AC4` (63 insns) is the scene pass called with
`(EAX=arg0, EDX=view obj, EBX=[0x54350])`:

```
0x58ACE  MOV ECX,0x54324
0x58AD3  CALL 0x57754                 ; ← the render-list body (§3)
0x58AD8  MOV EBX,0x54324
0x58AE1  CALL 0x60018
0x58AE6  MOV EDX,[EDI+8]
0x58AE9  CMP EDX,0xB10
0x58AEF  JLE 0x58B16
0x58AF5  CALL 0x62620 ; 0x58B03 CALL 0x56FA4 ; 0x58B0C CALL 0x62588
0x58B16  CMP EDX,0xFFFFF4F0 ; (symmetrical < -0xB10 branch: 0x62588, 0x56FA4, 0x62620)
0x58B43  (default branch: 0x62620, 0x62588, 0x56FA4)
```

`FUN_00056FA4` (`0x56FA4`, FU-85 §2) then walks the same sorted list and calls
`FUN_00057158` (axis-aligned sprite via `FUN_00057080`) plus `FUN_00057594`
— which FU-89 **corrects** to the player-name label drawer (errata §9):
`0x57594` searches `[0x9A88][0..4]` for the slot id, fetches the name through
`FUN_0004BF0C`/`FUN_0004BF7C`, formats it (`0x99E9F`, format `0x1E68`) and
draws it through `0xA06FC` (`0x57686..0x5773F`). So the live frame composes
both the warped 4-point sprite pass (`0x57754`, §3.2) and the axis-aligned
sprite/label pass (`0x56FA4`); their visual division of labour is an open leg.

`FUN_00058BC0` and `FUN_00058D70` are two further variants of the same
driver (same `0x58A44` + `0x58AC4` sequence, `0x58D70` gated by `[0x9094]`
and the `0x43618` state); neither has a static caller (open leg). The body
itself is called from exactly three sites:
`0x58AD3` (`FUN_00058AC4`), `0x58C8C` and `0x58D1F` (`FUN_00058BC0`), all
`CALL 0x57754` (`search_instructions CALL 57754`).

## 2. The render list and its depth keys

The list is initialised by `FUN_00056CF4` (FU-85 §4):

```
0x56CFA  MOV EDX,0x17
0x56CFF  MOV EBX,0x18
0x56D06  MOV [0x8FE8],EDX             ; last active index
0x56D0C  MOV [0x4EAE8],EBX            ; active count = 0x18
0x56D60  loop: [EAX+0x4E6F0] = ESI (0..0x20)   ; 0x21 slots pre-seeded
```

so the active `0x18`-entry list is `[0x4E6F0+i*4]` (1-based slot values, `0`
= empty) with keys `[0x4E7B0+i*4]`. Each frame the body re-seeds the keys
(`0x57779..0x577B4`):

```
0x57779  MOV ESI,[0x54374]            ; block B+0xCB4 (a triple)
0x5777F  MOV ECX,[0x8FE8]
0x57785  MOV EDI,[0x54370]            ; block B+0x10C8 (key base)
0x5778B  INC ECX
0x5778E  SHL ECX,2                    ; (0x17+1)*4 = 0x60
0x57791  MOVSD x3                     ; copy the triple to [0x54370]
0x57798  loop: IMUL EBX,[EAX+0x4E6F0],0xC   ; value * 12
0x5779F         MOV EDX,[0x54370]
0x577A5         ADD EAX,4
0x577A8         MOV EDX,[EDX+EBX+0x8]       ; z of position[value]
0x577AC         MOV [EAX+0x4E7AC],EDX       ; → keys[value_index+1]
0x577B4         JL loop
0x577B6  PUSH 0x4E6F0                  ; values
0x577BB  PUSH 0x4E7B0                  ; keys
0x577C0  MOV EBX,[0x4EAE8]; PUSH EBX
0x577C7  CALL 0xA1860                  ; shell sort
```

Two quirks are quoted, not "fixed": the store lands at `[eax+0x4E7AC]` after
`ADD EAX,4`, so the loop writes **`keys[1..count]` from `list[0..count-1]`**
and leaves `key[0]` untouched (`[0x4E7B0]` stays BSS-zero, which is why the
draw walk's `threshold > key[0]` stop never trips at index 0); and the key is
taken from `[0x54370]+value*0xC+8`, where `[0x54370]=blockB+0x10C8` and the
rotated **jittered** triples start at `blockB+0x10D4` — so
`key[i+1] = z(jittered rotated point of slot list[i])`.

`FUN_000A1860` (59 insns) is a Shell sort over the tandem arrays: gap starts
at `count>>1`, halves each pass; for each `i` from gap it walks `j=i-gap`
down by gap and **swaps keys and values together while `A[j] < A[j+gap]`**
(`0xA18B0 CMP ECX,[EBX]; 0xA18B2 JGE 0xA18D6` exits on `>=`). The result is
**descending** by key: for `{4,2,9,1}/{10,11,12,13}` the ported sequence ends
`keys {9,4,2,1}`, `values {12,10,11,13}`.

## 3. The body `0x57754`

### 3.1 Prologue, gates, and the 5 control-slot marker phase

```
0x57754  PUSH ESI; PUSH EDI; PUSH EBP; SUB ESP,0xC8
0x5775D  [esp+0xC4]=EAX               ; arg0: rect/limit record
0x57764  [esp+0xB8]=EDX               ; arg1: view record (obj)
0x5776B  [esp+0xBC]=EBX               ; arg2: [0x54350] threshold
0x57772  [esp+0xB0]=ECX               ; arg3: 0x54324 matrix
0x577CF  EAX=arg0[2]; EDX=arg0[0]; EAX=(EAX+EDX)>>1; [esp+0x9C]=EAX
0x577F5  EAX=0x4EACC; ECX=5; CALL 0x9E907
0x577FF  CALL 0x57510                 ; marker-phase gate
0x57806  JZ 0x57B41
0x5780C  MOV EBP,[0x9014]
0x57814  JNZ 0x58285                  ; [0x9014] set → skip everything
```

`arg0` is a 16-byte rectangle plus four depth fields: `[0]`, `[4]`, `[8]`,
`[0xC]` bound the clip test, `[0x18..0x24]` drive the depth override, and
`[0x24]` is a screen-y cutoff. `[esp+0x9C]` is `(rect[0]+rect[2])/2`.

The 5-slot phase (`0x5783A..0x57B3C`, `EBP=0..4`) reads the signed byte
`[0x9A88+EBP]` (init `0xFF` for the first 6 bytes by `FUN_00056CF4`
`0x56D12..0x56D1F`); a negative byte skips. For a live id `s`:

```
0x5784B  IMUL EAX,ESI,0xC
0x5784E  CMP [EAX+0x55AB4],0xFFFFD8F0 ; hidden marker
0x5785E  CMP [[0x9A74]+EAX],0x8E0     ; lateral gate
0x57877  EDI = [esp+0xBC]             ; threshold
0x5787E  CMP EDI,[EDX+EAX+8]          ; threshold vs rotated-jittered z
0x57882  JNG 0x578AC
0x5788B  EAX = [esp+0x9C] + EBP*2     ; beyond threshold: synthesized x
0x57892  EBX = 0x80; [esp+0xC0]=EBX   ; flags = 0x80 (off-screen marker)
0x578AC  EAX=[0x54364]+s*8; x=arg>>16; y=arg>>16; flags=0
0x578E3  clip vs rect ± [0x9044]: flags |= 0x10/0x40/0x20/0x80
0x5795F  if flags==0:
           0x5796D CALL 0x45514 / 0x5797C CALL 0x4B428
           0x57A62 [0x543A4] corner table
           0x57A0E CALL 0xA3090(4, …)
           0x57AB8 CALL 0x4C590(4, [esp], [0x54394], 0)
0x57AC7  EDI = signed byte [0x9A88+5]
0x57AD4  SETZ; EAX = [0x901C + eq*4]  ; [0x9020] for the designated id
0x57AE5  CALL 0x5962C
0x57AEA  [0x4E778+EBP*8] = {x,y}      ; control-slot screen pair
0x57B0E  [0x4EACC+EBP*4] = flags      ; control-slot edge flags
```

`[0x4E778]` (5×8 bytes) and `[0x4EACC]` (5 dwords) are consumed after the
scene pass by `FUN_00058538` → `FUN_00058290`, which draws the control-slot
marker arrows from the quad buffers `0x4EB2C..0x4EB68` through `0xA3418`
(`FUN_00058538`: `0x58559 MOV EBP,0x4E778`; `0x58567 CMP [EDI+0x4EACC],1;
0x58584 CALL 0x58290`). The phase is gated by `FUN_00057510`
(`0x57510..0x57562`): `0x63FF0()` nonzero → 1; then `0x53D58`, `0x4B68C`
(two variants), `[0x9014]`, `0x4B3E0`, `0x53D9C` decide. When the gate is 0,
`0x57B41` runs a 4-point projection of the tracked point `[0x5FB8..0x5FC0]`
relative to the view (only if `[0x9014]&2`), then falls into the render-list
phase.

### 3.2 The render-list phase (`0x57CA6..0x5820A`)

After the sort and the marker phase, the body walks the sorted list:

```
0x57CA1  CALL 0x49068
0x57CA8  ECX = [0x4EAE8]; i=0
0x57CBF  EDX = [esp+0xBC]                   ; threshold
0x57CC6  CMP EDX,[EDI+0x4E7B0]              ; threshold vs key[i]
0x57CCC  JG 0x58210                         ; descending keys → stop
0x57CD2  EBX = [EDI+0x4E6F0]; if 0 continue ; 1-based slot value
0x57CE0  EBP = EBX-1; ESI = EBP*0xC
0x57CE6  CMP [ESI+0x55AB4],0xFFFFD8F0       ; hidden
0x57CF6  CMP [[0x9A74]+ESI],0x8E0           ; lateral
0x57D08  [esp+0xB4] = EBP*8                 ; slot stride 8
0x57D1D  CALL 0x4C590(1, [0x543AC]+slot*0xC, [0x54364]+slot*8, 0)
0x57D39  CALL 0x4C590(1, [0x543C0]+slot*0xC, [0x54388]+slot*8, 0)
```

**Slot layout.** `[0x543AC]` is the rotated **clean** triple array
(block B+0x11E8) and `[0x543C0]` the rotated **jittered** one (B+0x10D4),
both 12-byte triples indexed by `slot = value-1`; the outputs are two 8-byte
16.16 pairs at `[0x54364]+slot*8` (clean) and `[0x54388]+slot*8` (jittered),
`[0x54364]=[0x5434C]+0xBF0=B+0x1F7C`, `[0x54388]=[0x5434C]+0xB38=B+0x1EC4`
(§5). Both `FUN_0004C590` calls use `count=1`, `shift=0`, exactly the
per-point divide FU-88 §3.2 derived.

**Depth override (`0x57D5C..0x57DFB`).**

```
0x57D5C  EAX = [0x54388]+slot*8
0x57D71  ECX = [arg0+0x24]
0x57D74  CMP ECX,[EAX+4]                    ; rect.24 vs jittered screen y
0x57D77  JNG 0x57D97                        ; rect.24 <= y → override + skip
0x57D79  EDX = [0x54364]+slot*8
0x57D88  EBX = [EDX+4]                      ; clean y
0x57D92  CMP EBX,[arg0+0x1C]
0x57D95  JG 0x57DB3                         ; clean y > rect.1C → complex
0x57D97  [EAX+4] = [arg0+0x24]; continue    ; no draw
0x57DB3  delta = clean_y - jitter_y
0x57DC6  candidate = [arg0+0x20] + 2*delta
0x57DD6  CMP candidate,[EAX]                ; vs jittered screen x
0x57DD8  JNG 0x57DEC
0x57DDA  alternative = [arg0+0x18] - 2*delta
0x57DEA  JL 0x57E04                         ; candidate>x && alt<x → DRAW
0x57DEC  [EAX+4] = [arg0+0x24]; continue    ; no draw
```

So an entry is drawn only when `rect.24 > jitter_y`, `clean_y > rect.1C`,
`rect.20 + 2·(clean_y − jitter_y) > jitter_x` and
`rect.18 − 2·(clean_y − jitter_y) < jitter_x`; every skip path writes the
jittered y from `rect.24`, which is exactly the cutoff `FUN_00056FA4`
re-tests (`0x57043 CMP EAX,[EBX+0x24]`).

**Draw path (`0x57E04..0x581ED`).** The entry's sprite is resolved at
`0x57E44 CALL 0x78FAC` with `dir = (([0x8FF8] − [0x55BC4+s*4] + 0x1000) &
0xFFFF) >> 13 & 7` (`0x57E0B..0x57E41`), anim `[0x55C20+s]`, frame
`[0x55C37+s]`. When `[0x9028] != 0` (`0x57E52`) the alternate builder
`FUN_00058738` (`0x57E70`) supplies the quad; otherwise the body builds 16
products from the corner tables (`[0x543A0]` at `0x57E97`, `[0x5437C]` at
`0x57FD0`) — the world-space billboard corners scaled by the rotated depth.
The four points are then clip-tested against threshold (all z ≥ threshold,
`0x58103..0x5812E`), projected with **`FUN_0004C654`** at `0x58141`
(`EAX=4, EDX=points, EBX=[0x54384]=B+0xCA8`, the whole-pixel variant FU-88
§3.1), XOR-shuffled (`0x58150..0x581BA`, only when `[esp+0x90] != 0`), and
blitted through `FUN_000A31B0` (anim != 0, `0x581D9`) or `FUN_000A3380`
(anim == 0, `0x581E8`) — the 4-point warped sprite mappers
(`0xA31B0` builds the sprite-sized quad and calls `0xB3F64` twice; `0xA3380`
calls `0xB4050`).

**Tail (`0x58210..0x58285`).** When `([0x9A78] & 0x80) == 0`,
`threshold <= z([0x54374])` and the `[0x9004]`/`FUN_0004B6A8` gate passes,
the body projects `FUN_0004C654(4, [0x54374]+0x18, [0x54378]+0x10)` and
blits bank entry `FUN_000A1920([0x4EAEC], 0)` through `0xA3380`
(`0x58246..0x58282`) — the ball/indicator quad path.

## 4. `FUN_000590B0` — delta, cull, rotate, project

`FUN_000590B0` (`EAX`=arg, `EDX`=obj, `EBX`=matrix, `ECX` overwritten) is the
background/geometry projector called from `FUN_00058A44 0x58A97`. The
decompiler's rendering (Ghidra function, clean) plus the listings:

```
0x590BD  CALL 0x44394                          ; screen dims + reciprocal tables
0x590C2  (*[0x9A78] & 0x40) ? 0x166 : 0x115    ; point count
0x590DE  d = [0x5426C] - obj[0]
0x590EA  d = [0x54270] - obj[1]
0x590F8  d = [0x54274] - obj[2]
0x59107  if (d.x|d.y|d.z == 0):
0x59119      CALL 0x5903C(obj, arg)            ; same-view compare (§4.1)
0x59140      if (ret != 0) { [0x9094]=0; return }   ; CULL
0x5914D  d' = -obj
0x59119  CALL 0xA3110(0x10F, [0x54358], d, [0x54358])   ; add delta to 0x10F
0x5916A  base = [0x54358]+0xCB4
0x59193  CALL 0xA3110(count-0x10F, base, d', base)      ; add -obj to the rest
0x59189  [0x9094] = 1
0x5919B  CALL 0xA3010(count, [0x54358], matrix, [0x54348])
0x591B2  CALL 0x4C590(count, [0x54348], [0x5434C], 0)
```

The point count is `0x115` (277) normally, `0x166` (358) when bit `0x40` of
`[0x9A78]` is set. `[0x5426C..0x54274]` is the cached previous view position
(written by `FUN_00058A44` at `0x58AAC..` from `obj`, §5), and `[0x9094]` is
the "projection valid this frame" flag read by the drivers
(`0x58Bdd`, `0x58D80`) and set to 0 by the cull. The 0x115/0x166 points are
the background/geometry triples in `[0x54358]` (block A) — the same buffer the
delta add touches before rotating into `[0x54348]` (block B) and projecting
into `[0x5434C]`.

### 4.1 `FUN_0005903C` decomposed (FU-88 leg 1)

Bytes from Ghidra memory `0x59055` (`ndisasm`-decoded):

```
0x59041  EBX = EAX                       ; obj
0x59043  CALL 0x37AE4
0x5904A  JNZ 0x59055
0x5904C  CALL 0x63928
0x59053  JZ return 0
0x59055  ECX=[0x54278]; CMP ECX,[EBX+0xC];  JNZ return 0
0x59060  ESI=[0x5427C]; CMP ESI,[EBX+0x10]; JNZ return 0
0x5906B  EDI=[0x54284]; CMP EDI,[EDX+0];    JNZ return 0
0x59075  EBP=[0x54288]; CMP EBP,[EDX+4];    JNZ return 0
0x59080  EBX=[0x5428C]; CMP EBX,[EDX+8];    JNZ return 0
0x5908B  ECX=[0x54290]; CMP ECX,[EDX+0xC];  JNZ return 0
0x59096  EAX=1; return 0x5909F
```

i.e. `1` iff `(FUN_00037AE4() || FUN_00063928())` **and** the six cached
dwords equal `obj[3]`, `obj[4]`, `other[0..3]` (`EDX` = the second argument,
the `EAX` passed to `0x590B0`). The cull therefore means "nothing moved and
the view/position pair is unchanged from the previous frame"; the two
`0x37AE4`/`0x63928` replay/instant gates must be nonzero for the cull to
apply. `[0x9094]=0` makes `FUN_00058BC0`/`FUN_00058D70` skip the render body.

## 5. The drawable blocks and the per-slot tables

`FUN_00058E00` (116 insns) allocates once (FU-88 §4):

```
0x58E0C  FUN_0004AFB8(0x3F,0x20,0x2094) → [0x54358]   ; block A, 0x2094 B
0x58E25  FUN_0004A448(0x1E6C)           → [0x54348]   ; block B, 0x2094 B
```

Block A pointer map: `[0x543B8]=A+0x78`, `[0x543D0]=A+0x780`,
`[0x543B0]=A+0xCB4`, `[0x543DC]=A+0`, `[0x543C8]=A+0xCFC`,
`[0x543BC]=A+0x10D4`, `[0x543A8]=A+0x11E8`, `[0x543A0]=A+0x12FC`,
`[0x543A4]=A+0x132C`, `[0x543B4]=A+0x135C`.

Block B pointer map (the render targets): `[0x543D4]=B+0x78`,
`[0x5439C]=B+0x780`, `[0x54398]=B+0x8A8`, `[0x54378]=B+0x878`,
`[0x54384]=B+0xCA8`, `[0x54374]=B+0xCB4`, `[0x54394]=B+0xCC8`,
`[0x5436C]=B+0xCE8`, `[0x54390]=B+0xCFC`, `[0x54370]=B+0x10C8`,
`[0x543C0]=B+0x10D4`, `[0x543AC]=B+0x11E8`, `[0x5437C]=B+0x12FC`,
`[0x543D8]=B+0x132C`, `[0x54368]=B+0x135C`, `[0x5434C]=B+0x138C`, and
relative to `[0x5434C]`: `[0x5438C]=+0x50`, `[0x5435C]=+0x500`,
`[0x54360]=+0x7F8`, `[0x54388]=+0xB38` (=B+0x1EC4),
`[0x54364]=+0xBF0` (=B+0x1F7C).

The two per-slot tables are adjacent in block B: 23 slots × 8 bytes fill
`B+0x1EC4..B+0x1F7B` (`[0x54388]`) and `B+0x1F7C..B+0x2033` (`[0x54364]`),
inside the `0x2094` allocation. The depth-key base `[0x54370]=B+0x10C8` sits
12 bytes before the rotated jittered array, which is exactly why the prologue
copies one triple into it (§2).

`FUN_00058A44`'s tail caches the view record and returns `[0x54350]`:

```
0x58A9C  EDX = obj[1]
0x58A9F  EAX = obj[5]
0x58AA7  CALL 0x589E0                 ; EAX = [0x54350] (ECX=6 preserved)
0x58AAC  REP MOVSD (ECX=6): obj[0..5] → 0x5426C..0x54283
0x58ABA  ECX=0xA; ESI=EBP; EDI=0x54284; REP MOVSD: obj[0..9] → 0x54284..
```

## 6. `FUN_000589E0` and `[0x54350]` — the near-depth threshold (FU-88 leg 2 closed)

`FUN_000589E0` (29 insns; call site `0x58A9C` `EAX=obj[5]`, `EDX=obj[1]`):

```
0x589E2  CMP EAX,[0x54280]; JNZ compute     ; cache key = obj[5]
0x589EA  CMP EDX,[0x54270]; JZ clamp        ; cache key = obj[1]
0x589F2  CMP EDX,[0x908C]; JGE compute
0x589FA  EAX = [0x9088]; JMP store          ; fallback branch
0x58A01  EBX = EAX*2
0x58A0E  EAX=0x14; CDQ; IDIV EBX            ; q = 0x14/(2·obj[5])
0x58A14  SHRD EAX,EDX,0x10                  ; (r<<16)
0x58A1B  IDIV EBX                           ; (r<<16)/(2·obj[5])
0x58A1D  SHL ECX,0x10; ADD EAX,ECX          ; + q<<16
0x58A22  MOV [0x54350],EAX
0x58A27  CMP [0x54350],0x78; 0x58A30 MOV 0x78 if below
0x58A3A  MOV EAX,[0x54350]; RET
```

So `[0x54350] = max(floor((0x14<<16)/(2·obj[5])), 0x78)`, or the previous
value when `(obj[5],obj[1])` repeat, or `[0x9088]` when `obj[1] < [0x908C]`,
and the same clamp applies to the fallback. `[0x54350]` is only written here
and zero-initialised at `0x58F95` (`search_instructions 54350`). It has **no
memory reader** — the FU-88 leg-2 search missed it because the value is the
**return value**: `0x58A3A MOV EAX,[0x54350]`, `FUN_00058A44` leaves EAX
untouched through its `REP MOVSD` tail, and `FUN_00058B68 0x58B76 MOV EBP,EAX`
feeds it as `EBX` to `0x6331C`/`0x610C4`/`0x58AC4`/`0x57754`/`0x56FA4`. Inside
`0x57754` it is compared against rotated z and against the 4-point quad z
(`0x5787E`, `0x57CC6`, `0x5810E`, `0x5822C`), so it is the **near-depth
threshold** ("draw only entries whose depth key is at least this"), not an
entity scale.

## 7. `FUN_00056E50` and the `0x3A` rotation span (FU-88 leg 4 closed)

`FUN_00056E50` (`EAX`=camera vector `[0x9A70]`, `EDX`=view obj,
`EBX`=matrix `0x54324`) staging, quoted:

```
0x56E5D  [0x9000] = [0x8FFC]
0x56E67  CALL 0x587FC
0x56E7D  memcpy(0x55AB0 → [0x543A8], 0x114)   ; 23 triples, block A+0x11E8 (clean)
0x56E96  memcpy(0x55AB0 → [0x543BC], 0x114)   ; 23 triples, block A+0x10D4 (jittered)
0x56ECF  FUN_000A3110(0x17, [0x543A8], -cam, [0x543A8])
0x56EDB  loop 23×: [0x543BC+i] += -cam.x;
                   +4 += -cam.y + [0x4E9B8+i*4]; +8 += -cam.z
0x56F0F  CALL 0x58870
0x56F24  FUN_000A3090(0x3A, [0x543BC], matrix, [0x543C0])
```

`0x3A` = 58 triples read from `[0x543BC]=A+0x10D4` into `[0x543C0]=B+0x10D4`:
the 23 jittered triples (`A+0x10D4`), the adjacent 23 clean triples
(`A+0x11E8=[0x543A8]`) and **12 further triples** (`A+0x12FC..A+0x138B`) —
three 4-point quads at `[0x543A0]`, `[0x543A4]`, `[0x543B4]`. The 58-triple
output ends exactly at `B+0x10D4+0x2B8 = B+0x138C = [0x5434C]`, the
`0x590B0` projection target. The extra quad writers are:

* `[0x543A0]` (A+0x12FC): `FUN_00056894(EAX=[0x8FF8], EDX=[0x8FF0],
  EBX=[0x8FF4], ECX=[0x543A0])` builds four rotated/scaled corner triples
  (`0x56894`: `0xA1A60` sin/cos, `IMUL`+`+0x8000`+`SHRD 16`, `IDIV 0x18`/`0x30`,
  stores at `[ESI]`, `+0xC`, `+0x18`, `+0x24`); called from `FUN_00056CF4`
  `0x56D43` and `FUN_000585E4` `0x58650`.
* `[0x543A4]` (A+0x132C): `FUN_00056B88(EAX=[0x900C])` writes the 16-dword
  cross/diamond (`0x56B99..0x56BC9`, then four `0xA3090(4, 0x4E838, M, +0x30)`
  passes) and stores the pointer back (`0x56C6A MOV [0x543A4],EBP`).
* `[0x543B4]` (A+0x135C) writer is not located (open leg).

`FUN_00058870` (67 insns) resets `[0x9028]=0`, reads input mode `0x1CAEC(1)`,
fills `[0x4EA70+id*4]=[0x9034]` for control-slot ids `< 0xB`
(`0x588CA..0x588F7`) and, in the `0x1CAEC(2)` branch, for ids `0xB..0x15`
(`0x58929..0x58956`), and its result
`[0x9028]` selects the alternate per-entity builder `FUN_00058738` in the
render body (`0x57E52`). The four-point coords are consumed by the body's
corner builders (`0x579ED [0x543A4]`, `0x57E97 [0x543A0]`, `0x57FD0` uses the
rotated output `[0x5437C]=B+0x12FC`), so the 12 extra triples are the 3
marker/corner quads shipped through the same rotation.

## 8. `FUN_0004C654` callers (FU-88 leg 6)

`FUN_0004C654` is the whole-pixel `FUN_0004C590` wrapper (decompiled):

```
iVar1=[0x146A8]; iVar2=[0x146AC]
[0x146A8] >>= 16; [0x146AC] >>= 16
FUN_0004C590(count, in, out, 0x10)
[0x146A8]=iVar1; [0x146AC]=iVar2
```

`search_instructions CALL 4c654` finds exactly **two** call sites, both in
the `0x57754` body: `0x58141` (4 points `[esp..esp+0x2C]` → `[0x54384]`)
and `0x5825D` (`[0x54374]+0x18` → `[0x54378]+0x10`). FU-88's third caller
`0x57FDF` is inside the unrolled corner-multiply block (errata §9).

## 9. Port: `fifa96_scene`

`include/fifa96_loader/fifa96_scene.h` + `src/fifa96_loader/fifa96_scene.c`
(caller-owned buffers/arrays, no globals, `fifa96_err_t` result, errors as
`-FIFA96_ERR_INVALID`, no comments). It wires the per-slot projection to
`fifa96_projection_screen` and models the list/depth/threshold/cull integer
logic; it does not load resources or touch the match globals.

| original | port |
|---|---|
| `0x57779..0x577B4` key seeding (`keys[i+1] = z([0x54370]+list[i]*0xC)`) | `fifa96_scene_build_keys` |
| `FUN_000A1860` Shell sort, tandem keys/values, descending | `fifa96_scene_sort` |
| loop stop + `0x57CE6`/`0x57CF6` gates | `fifa96_scene_slot_gate` |
| `0x57D5C..0x57DFB` depth override / draw-or-skip decision | `fifa96_scene_depth_override` |
| `FUN_000589E0` fallback + `(0x14<<16)/(2·field)` + clamp `0x78` | `fifa96_scene_threshold` |
| `0x590B0` head + `FUN_0005903C` six compares | `fifa96_scene_reproject` |
| `0x57D31`/`0x57D54` two `FUN_0004C590(1,…)` calls | `fifa96_scene_slot_project` (via `fifa96_projection_screen`) |

Divergences (documented): the original's key-store quirk writes
`keys[1..count]` and leaves `keys[0]` alone — the port reproduces that
(`keys` must hold `count+1` entries) and rejects `list[i] >= position_count`
where the original would read out of bounds; `fifa96_scene_sort` rejects a
NULL array only when `count > 1` (the original dereferences only inside the
gap loops); `FUN_000589E0` divides by zero (`#DE`) for `obj[5]==0` — the port
returns `-FIFA96_ERR_INVALID`; the per-call cache at `[0x54280]`/`[0x54270]`
is caller-owned and not modelled (recomputation is pure); the cull's two
replay gates are folded into one `replay_gate` argument; the original's
32-bit wrapping adds are reproduced by unsigned arithmetic.

## 10. Tests (`tests/test_scene.c`, suite 76 → 77)

* Layout `_Static_assert`s (`fifa96_scene_slot`, strides `3`/`8`, marker
  `-10000`, lateral `0x8E0`).
* `build_keys`: `keys[1..4] = {10,40,20,30}` from `list {0,3,1,2}` with
  `keys[0]` preserved; zero-count no-op; out-of-range value, NULLs.
* `sort`: the exact tandem sequence `{4,2,9,1}/{10,11,12,13}` →
  `{9,4,2,1}/{12,10,11,13}` (descending); equal keys untouched; 0/1 count
  no-ops; NULL cases.
* `slot_gate`: `threshold == key` visible, `threshold > key` not; hidden
  `-10000`; lateral `0x8E0` vs `0x8E1`; NULL.
* `depth_override`: all four branches (y already high, clean y too low,
  draw, candidate/alternative boundaries), NULLs.
* `threshold`: `field 10/100/1000/10000` → `0x10000/6553/655/0x78`; fallback
  branch and its clamp; negative field; `field 0`; NULL.
* `reproject`: zero delta + same view → 0; one moved word, gate 0, one
  mismatched dword → 1; NULLs.
* `slot_project`: equality with two direct `fifa96_projection_screen` calls
  (dim 320/200); near-plane (`z=4`) leaves the output untouched and
  `clean_visible=0`; NULLs.
* `make test`: 76/76 before, **77/77 after**;
  `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
  tests/test_scene.c src/fifa96_loader/fifa96_scene.c
  src/fifa96_loader/fifa96_projection.c` runs clean.

## 11. Errata (quoted)

* FU-88 §4/§9 "the render body at `0x57753`" — **corrected**: the entry is
  **`0x57754`**; `0x57753` is the third byte of the `LEA EAX,[EAX]` padding
  after `FUN_00057594`'s `RET` at `0x57750` (`read_memory 0x57740`:
  `… 5E C3 8D 40 00 56 57 55 81 EC C8 00 00 00`). All three call sites
  (`0x58AD3`, `0x58C8C`, `0x58D1F`) are `CALL 0x57754`.
* FU-85 §2/§4 "`CALL 0x57594(slot, [0x54364], …)` per-entity sprite" —
  **corrected**: `FUN_00057594` is a name-label drawer (slot search in
  `[0x9A88]`, `0x4BF0C`/`0x4BF7C` name fetch, `0x99E9F` format `0x1E68`,
  `0xA06FC` text draw); the sprite blit in that pass is `FUN_00057158`.
* FU-88 §2.3/§9 "no reader of `[0x54350]` was found" — **closed**: the value
  is `FUN_000589E0`'s **return**; `FUN_00058A44` forwards it in EAX and the
  driver passes it as the near-depth threshold (`0x58B76`, `0x5787E`,
  `0x57CC6`, `0x5810E`, `0x5822C`). It is a threshold, not an entity scale.
* FU-88 §3.1/leg 6 "callers `0x57FDF`, `0x58141`, `0x5825D`" — **corrected**:
  only `0x58141` and `0x5825D` are calls; `0x57FDF` is inside the unrolled
  multiply block.
* FU-88 leg 1 "`FUN_0005903C` … not decomposed" — **closed**: §4.1.
* FU-88 leg 4 "the extra 12 points and `FUN_00058870`'s role not derived" —
  **closed**: §7 (three 4-point marker quads; `0x58870` sets the `[0x9028]`
  render mode and the `[0x4EA70]` id table).
* FU-88 leg 2/§4 "`0x54364`/`0x54388` are pointer globals into the drawable
  block" — **confirmed and pinned**: block B = `[0x54348]`
  (`FUN_0004A448(0x1E6C)`, size `0x2094` from `FUN_0004AFB8`'s call),
  `[0x54388]=B+0x1EC4`, `[0x54364]=B+0x1F7C`, 23 slots × 8 bytes each.
* FU-85 §2 "depth key, stop when the caller's key is smaller" — **refined**:
  the list is sorted **descending** by the jittered rotated z
  (`FUN_000A1860` swaps while lower < upper) and the walk stops when the
  caller's threshold is greater than the current key.
* **§2 "the store lands at `[eax+0x4E7AC]` after `ADD EAX,4`, so the loop
  writes `keys[1..count]` from `list[0..count-1]` and leaves `key[0]`
  untouched" — corrected (M2 Task 11; first-hand `disassemble_bytes
  0x57779` on `/FIFA96.EXE`)**: the store `0x577AC MOV [EAX+0x14E7AC],EDX`
  runs with `EAX = 4(k+1)` after `0x577A5 ADD EAX,4`, so it lands at
  `0x4E7B0 + 4k` = `keys[k]`; the 24-iteration loop
  (`ECX = ([0x8FE8]+1)*4 = 0x60`, `0x577B2 CMP EAX,ECX`) fills
  `keys[0..23]` from `list[0..23]`, i.e. keys pair with the **same-index**
  1-based values, and the prologue triple copy to `[0x54370]` exists so the
  value-0 sentinel's key reads the copied triple's z (there is no untouched
  key[0]). The value-0 entry is then skipped by the walk (`0x57CD2 JZ`), and
  the fixed 0x18-entry list is the sentinel + the 23 staged slots. The port
  `fifa96_scene_build_keys` was corrected to `keys[i] = z(list[i])` and the
  engine render list now builds the native-shaped 24 entries; the earlier
  "index-0 gate is inert" divergence note is superseded.
  This correction also supersedes the §9 port-table row
  ("`0x57779..0x577B4` key seeding (`keys[i+1] = z([0x54370]+list[i]*0xC)`)"
  → `fifa96_scene_build_keys`), the §9 divergences sentence ("the original's
  key-store quirk writes `keys[1..count]` and leaves `keys[0]` alone — the
  port reproduces that (`keys` must hold `count+1` entries)") and the §10
  `build_keys` test bullet ("`keys[1..4] = {10,40,20,30}` … with `keys[0]`
  preserved"): the corrected contract is `keys[i] = z(list[i])` over
  `[0,count)` with no preserved slot, and `tests/test_scene.c` pins the
  corrected form.
* **Kickoff record placement source audit (M2 playability-legs Task 5 /
  OL-T11-8 placement item; the plan's "OL-T11-9" label is a numbering slip,
  register `-9` is the direction addend `0xA2A10`; first-hand `/FIFA96.EXE`
  this slice).** The scene staging
  `FUN_00036C70` (§4) reads `rec+0x59/5D/61`; their match-start values come
  from the setup/restart chain, not from the staging body:
  * the kickoff act-1 body (`FUN_0008A938` jump table flat `0x1107EC` entry 1
    -> `0x8ABAB..0x8ABDA`, `disassemble_function 0x8A938`) stores the ball
    spawn `[0x158830] = 0x1E0`, `[0x158838] = 0` and `[0x157AB1] = 0`, then
    invokes the act-1 handler `FUN_00088DC8`;
  * `FUN_00088DC8` stage 0 resets the camera (`FUN_000700F4`), calls
    `FUN_00073E28` (phase 0), then `FUN_000740A0(1, side)` — the native
    phase-1 entry, which runs `FUN_0008D098` per team and so the per-record
    phase handler `0x6E1D0` that writes each *target* triple from the
    formation bytes — and then `FUN_00073E08`;
  * `FUN_00073E08` calls `FUN_0008C24C(0x158830)` (the ball record
    0x15880C/10/14 := the spawn triple) and `FUN_0008CF60` per team;
  * `FUN_0008CF60` loops the 11 records, resolves the phase handler
    (`FUN_0006D920`), calls it with `EDX=&rec+0x4D`, then `FUN_00079F3C`
    (other-team camera placement) and finally `FUN_00079B6C`
    (`0x79B6C..0x79C1C`, single RET at `0x79C1C`; first-hand
    `get_function_by_address 0x79BB5` -> `body_end 0x79C1C`): **position
    +0x59/5D/61 := target +0x4D/51/55, position.y := 0, target := position**,
    word `+0x69` (dz), words `+0x71/+0x73/+0x75` (both velocity halves), word
    `+0x67` (dx), word `+0x65` (distance) and byte `+0x9C` := 0. The same body
    then runs the tail: the `FUN_00079C50` camera-vs-target face
    (`0x79BB5..0x79BCF`, camera = `[0x15774C]`, the reset `[0x10F328/2C/30]`
    triple), the conditional `FUN_0006E48C` `byte[rec+0x3E] = byte[rec+0x8E]`
    write (`0x79BD4..0x79BEC`, gated on the row pointer and row `+0x44` bit 0),
    and the **unconditional `FUN_0006E598(rec, active ? 0 : 0x26, 0)` at
    `0x79C13`** — an animation-id producer on this very path (inactive records
    take row id 0x26; active records re-resolve id 0 and reset `[rec+0x3D]`).
  The per-record formation bytes come from the resolver pointer `[rec+8]`
  (`FUN_0006D920` -> `FUN_0004AFB8(6*formation_id+1)+subtype*4`); the pointer
  table at flat `0x14BFC0` is populated by `FUN_0004A6BC` from the external
  `t%s.dat`/`lay%s.fmt` resources (name templates at `0x101C6C`/`0x101C74`,
  `disassemble_function 0x4A6BC`), so the actual kickoff coordinates are
  resource data, not derivable from the EXE. The plan's cited
  "`FUN_0008D824`-adjacent" source is **not** a placement writer: `0x8D824`
  binds the controlled actor into the nearest record's *target* triple
  (FU-142 K.2), and the per-frame `FUN_0008D098` call at `0x8D11E` is
  `FUN_0008DCD4(EAX=&rec+0x59, EDX=&rec+0x4D, EBX=&rec+0x65)`, whose only
  output is the `+0x65` lane/distance triple (first-hand
  `disassemble_function 0x8DCD4`: the two input triples are read, the three
  `+0x65/+0x67/+0x69` words are written) and which never commits a position.
  **Engine status:** the placement commit `fifa96_match_entities_place`
  (`FUN_00079B6C`'s commit block, span corrected) and the kickoff pass
  `fifa96_match_entities_kickoff_place` (ball spawn 0x1E0/0 with ball.y = 0,
  per-record commit, camera face, and the `0x79C13` selector) are ported and
   fixtured (the per-record formation targets were the resource table's open
   leg at the time — OL-T11-8 partial — so records were then placed from
   caller-seeded targets; superseded by the erratum below), and the tail's
   `0x6E48C` `+0x3E` write stays a leg (no pool field). Note
   `[0x157AB1] = 0` is a process global with no derived home.
* **Formation source correction + first-hand placement derivation, and engine
  landing (M2 visible-match Task 1 / OL-T11-8; `/FIFA96.EXE`, read-only).**
  The audit bullet above is corrected in two places; the resource half is now
  first-hand:
  * the record pointer is set by `FUN_0006D920` (`0x6D94B MOV AX,BX` of `6*id`,
    `0x6D94E SHL ESI,2` of `byte[rec+0x8D]`, `0x6D951 CALL 0x4C384`,
    `0x6D956 ADD EAX,ESI`, `0x6D95E MOV [EDX+8]`) -> `[rec+8] =
    FUN_0004C384((6*formation_id) & 0xFFFF) + (byte[rec+0x8D] << 2)` — **no
    `+1`**; the getter is the thunk `FUN_0004C384` (`0x4C384..0x4C389`:
    `AND EAX,0xFFFF; JMP 0x4AFB8`, the `0x14BFC0` handle table), also called
    for the record's other pointers at `0x6D969`/`0x6D98C`/`0x6D99E` and for
    the team pointers at `0x6DA3E`/`0x6DA51` (`FUN_0006D9C4`). The index is
    the record's `+0x8D` byte, initialized to the record block position by
    `FUN_0008C2E0` (`0x8C324..0x8C336`: `[EAX-0x25] = DL` for
    `EAX = team+0xB2*(i+1)`, so record i's `+0x8D` = i; same byte read at
    `0x6D93B`); `formation_id` is `byte[[team+0x7AE]]`, the
    `0x11033A + id*0x1D` roster row's first byte (`FUN_0006D9C4`
    `0x6D9E4`/`0x6DA25`);
  * the kickoff handler selection is closed first-hand: `0x110794[0x17]` =
    `0x00088DC8` (the phase-0x17 handler), whose stage-0 body calls
    `FUN_000700F4` (camera reset), `FUN_00073E28` (phase 0), then
    `MOV EAX,1` at `0x88E7A` and `CALL FUN_000740A0` at `0x88E82` (with
    `EDX = [0x157AAC]>>24`); `FUN_000740A0 0x740AC` writes `[0x157A4D]=AL`,
    the phase dword's high byte (`[0x157A4A]>>24`), i.e. phase := 1; `0x88E87`
    then calls `FUN_00073E08` -> `FUN_0008CF60`, so the kickoff path really
    runs the derived `FUN_0006E1D0` arithmetic (the cell resolved for phase 1);
  * `FUN_0004A6BC`'s four name formats are `0x101C80 "%s.fmt"` (table slots
    0..29), `0x101C88 "%s.dat"` (30..38), `0x101C94 "%s.%s"` with
    `0x101C90 "lfsh"` (39..55) and `0x101C9C "%s.qfs"` (56..62), resolved
    with the parameters of the `0x107370` 63-entry pointer table through the
    archive `FUN_0004A344(0x101C64, 0x101C6C)` = `art/gameart0.pvi` (first-hand
    `disassemble_function 0x4A6BC`; the loader fills 63 slots at
    `0x14BFC0..0x14C0BF` plus the `0x14C0E0` `flags.qfs` handle). The
    `lay%s.fmt`/`t%s.dat` strings at
    `0x101C7D`/`0x101C87` belong to other callers, not this loader;
  * the `.fmt` files are **BIGF entries of `/ART/GAMEART0.PVI`** (first-hand
    decoded directory, 268194 B, 60 entries: entry 0 `352ko.fmt` 44 B, the
    `ko`/`pl`/`pk`/`ps`/`sp` set per family 352/442/sw/424/433 plus the shared
    `freekick.fmt` at entry 25);
  * the placement is the phase cell `FUN_0006E1D0`
    (`0x6E1D0..0x6E241`, the `0x110794[phase]` handler for phase 1 and 0x12):
    `pair = [rec+8] + ((team_side == [0x157AAC]>>24) ? 2 : 0)` when
    `[0x157AAC]>>24` is the controlled side (`0x6E1ED..0x6E1F1`);
    `target.x = (int8)pair[0] * 0x26` (`0x6E1F4..0x6E233`), `target.y = 0`
    (`0x6E22C`), `target.z = (int8)pair[1] * 0x21` (`0x6E20C..0x6E238`), and
    a non-zero `team+0x826` side negates both (`0x6E21C..0x6E227`). Each
    4-byte record is therefore {opponent x/z, own x/z} and the 11 records
    cover the block positions;
  * **Engine:** `fifa96_scene_formation_load`/`_place` parse the container
    and evaluate the cell; `fifa96_match_run_begin` reads
    `/ART/GAMEART0.PVI` from the ISO, loads `352ko.fmt`, seeds both teams'
    targets (`fifa96_match_entities_seed_formation`) and commits them through
    the existing kickoff pass, so the records receive real non-zero positions
    and the M2 tape/in-game frames draw. The loop's `FUN_00079F3C` camera
    place (`0x8CF7C..0x8CF86`, camera `[0x15774C]`) stays unported, so the
    engine camera remains the `[0x10F328/2C/30]` reset triple (0,0,0) and at
    the kickoff instant only the positive-depth side passes the near gate.
    Remaining legs: the front-end producer
    of the formation id (`[0x14C1E4]`/`[0x14C1E5]`, BSS 0 on this build; the
    engine derives id 0), the unused `.dat`/`.lfsh`/`.qfs` slots and the
    `[team+0x7DB]`/`[team+0x7DF]` (`6*id+3`/`6*id+5`) pointers, the roster
    `+0x90` line code and the `+0x9A` marks (not consumed by placement).
  * **FU-144 erratum (loader name formats).** The `.lfsh` range's second
    format argument is the pointer `0x101C90` = `"fsh"` (the packed string
    `"lfsh"` starts at `0x101C8F`; read `0x101C8F` = `6c 66 73 68 00`), so
    slots 39..55 resolve `%s.fsh` (`PALsys.fsh`, `PALteam.fsh`, ...). The
    match palette source is slot 0x32 = `0x101BA4 "PALsys"` -> `PALsys.fsh`
    (BIGF entry 46 of `/ART/GAMEART0.PVI`); FU-144 holds the full chain.

* **`FUN_00079F3C` camera place landed (M2 interactive Task 2 / G2, T2;
  first-hand `/FIFA96.EXE` this slice).** The bullet above ("the loop's
  `FUN_00079F3C` camera place ... stays unported, so ... at the kickoff
  instant only the positive-depth side passes the near gate") is corrected in
  two places: (a) the function places *record targets*, not the camera — it
  reads the `[0x15774C]/[0x157754]` camera dwords and, for each record of the
  non-controlled team (`byte[[rec]+0x826] != [0x157AAC]>>24`) with the phase
  gate `byte[0x1106C3 + phase] != 0` set, snaps a target whose octagonal
  camera distance (`FUN_0008DCD4`) is `<= 0x180` onto the `0x180` ring along
  its existing direction (`FUN_0008DD70`/`FUN_000CD474` angle, the `0x114E04`
  sine fold, `FUN_000795A4` `(a*b+0x8000)>>16`); the full derivation is FU-96
  §7; (b) the kickoff instant frames only the positive-depth side in the
  native as well (`fifa96_projection_screen` requires `z >= NEAR`; the
  `FUN_000700F4((0,0,0))` camera sits between the halves — first-hand
  0x88E4B..0x88E6A), so the missing place was not the cause of that
  observation. The engine landing `fifa96_match_entities_camera_place`
  (`src/fifa96_engine/fifa96_match_entities.c`) runs at begin between the
  formation seed and the `FUN_00079B6C` commit; the M2 tape is re-pinned
  (v4.2: records 9/10 `(228,264) -> (251,291)` / `(-228,264) -> (-251,291)`;
  first diff frame 49, 117 hash-only lines, no `state=` suffix moved), and the
  in-frame place effect is fixtured (an in-ring record below the `0x78` near
  gate moves onto the ring and draws). Carried: the camera-mode/angle feed
  (FU-96 legs 1/3) and the FU-71 follow writer.

## 12. Open legs

1. **Two sprite passes per frame**: `0x57754` draws the 4-point warped
   quads (`0xA31B0`/`0xA3380`) and `FUN_00056FA4`/`0x57158` the axis-aligned
   sprites; which entities each actually paints (the skip/override branches
   are complementary on `rect.24`, but both passes can still draw the same
   entry) is not proven.
2. **`arg0` owner**: the rect + `[0x18..0x24]` depth fields structure passed
   to `0x57754`/`0x56FA4` (its writer) is not located; the
   candidate/alternative x compare semantics (edge placement) are quoted,
   not interpreted.
3. **`[0x543B4]` quad writer** (A+0x135C) and the `0x56894` angle inputs
   `[0x8FF0]`/`[0x8FF4]`/`[0x8FF8]` writers.
4. **`0x9E907`**: called as `EAX=0x4EACC, ECX=5` (`0x577F5`) and
   `EAX=0x4EA70, EDX=0x10000` (`0x588B8`); the fill semantics are not
   derived.
5. **`FUN_00058BC0`/`FUN_00058D70`** (no static callers) and the
   `0x43618`/`0x43608` state gates.
6. **5 control-slot ids**: `[0x9A88]` is init `0xFF` by `FUN_00056CF4` and
   written at `0x57619` (`FUN_00057594`) but the per-frame producer is not
   located.
7. **Gate internals**: `FUN_00057510`/`FUN_000574B8`/`FUN_00058538`'s
   `0x63FF0`/`0x53D58`/`0x53D9C`/`0x4B3E0`/`0x4B68C` checks and the
   `FUN_00058290` marker internals (`0x4EB2C..`, `0xA3418` sprites) are
   cited only.
8. **FU-88 leg 7** (`/tmp/opencode/fifa96_le.bin` differs from the Ghidra
   session image): carried; all data quotes above are from Ghidra memory
   (the local file is scrambled at the code addresses used here, so the
   body bytes were dumped from Ghidra).

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x590B0, 0x5903C (decompile), 0x589E0, 0x58A44,
0x58B68, 0x58AC4, 0x58BC0, 0x58D70, 0x56FA4, 0x56E50, 0x56CF4, 0x56B88, 0x58538, 0x58290,
0x56894, 0x58870 (bytes), 0x57510 (bytes), 0x57594, 0x58E00, 0xA1860,
0xA31B0, 0xA3380, 0x4C654;
`disassemble_bytes` 0x57740 (48 B), 0x57754 region (4096 B), 0x57600
region (6144 B), 0x57400 (768 B), 0x58870 (240 B), 0x5903C (120 B),
0x59055 (80 B from `read_memory`), 0x49880 (80 B);
`decompile_function` 0x590B0, 0x5903C, 0xA31B0, 0xA3380, 0x4C654;
`search_instructions` operands `57753`, `57754`, `57594`, `58ac4`, `58b68`,
`54350`, `543a0`, `543a4`, `4e778`, `8fe8`; `get_function_callers`
0x56FA4, 0x58B68, 0x58D70, 0x58A44, 0x58BC0; `get_function_by_address`
0x57740, 0x57753, 0x57754, 0x49830; `run_script_inline` memory dumps of
0x57400/0x57600/0x57754. Analysis-only outside the port: no tool,
capture-rig, ISO or Ghidra-project change. Port write set:
`include/fifa96_loader/fifa96_scene.h`,
`src/fifa96_loader/fifa96_scene.c`, `tests/test_scene.c`, `CMakeLists.txt`
(one library/test block). `make test`: 76/76 before, **77/77 after**;
ASan+UBSan `test_scene` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
