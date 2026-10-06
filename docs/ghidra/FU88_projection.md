# FU-88: the projection — view matrix, consumers and the reciprocal divide

Follow-on to FU-85 §4/§8 (open legs 1–3: matrix builders, the consumers
`0x62828`/`0x590B0`/`0x589E0`, the `0x54364`/`0x54388` writers, camera
height/zoom, "no perspective divide found"). This slice decomposes the view
matrix and its builders, the three cited consumers, the per-entry screen
writer `FUN_0004C590` and its reciprocal tables, the per-slot screen tables
`0x54364`/`0x54388`, and the full world→screen transform; the clean integer
math is ported as `fifa96_projection`.

Result in one line: **the match view matrix `0x54324` is
`Ry(obj[3]) × Rx(−obj[4])` in row-major 16.16, built by
`FUN_0004C4B4` = `FUN_0004C414(yaw) × FUN_0004C3B0(−pitch)` through the
`0xA2991` row-major multiply (64-bit accumulate, `SHRD 16`), with the angle
pair from `0xA1A60`/`0xCE3B0` (a 1025-entry `sin(2πk/1024)<<16` table plus a
6-bit first-order correction); the consumers are `FUN_000590B0` (which adds
the camera/entity delta to a point array, rotates it with `0xA3010`/`0xA2914`
and projects it) and the per-entity render body at `0x57753` (which projects
two point arrays into the per-slot tables `[0x54364]+slot*8` and
`[0x54388]+slot*8`); `FUN_00062828` is **not** a matrix consumer (it never
reads its `EBX = 0x54324`); the perspective divide **exists**: `FUN_0004C590`
normalises `z` into the table range and applies `(x·(W<<16)/z') >> shift` with
the reciprocals built by `FUN_0004C4F0` as `(dim<<16)/max(k+1,10)`, so the
screen position is `center + x·W/z` in 16.16 with `center` from `0x43E48` —
FU-85's open leg 3 is closed as a corrected claim.**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). `disassemble_function`/`disassemble_bytes` are the
  citation source; where the analyzer loses function boundaries the raw bytes
  were decoded with `ndisasm -b32` and checked against the bytes Ghidra does
  delimit (the FU-86/FU-85 method). The render body at `0x57753`
  (writers `0x57D1D`/`0x57D39`) is not a Ghidra function; it was decoded from
  `read_memory`.
* **Address mapping (FU-76/FU-84/FU-85, restated).** Code/function addresses
  equal true link addresses; a **data immediate** `A` is storage at flat
  `A+0x100000` (`[0x54324]` → `0x154324`, `[0x74C0]` → `0x1074C0`,
  `0x14E04` table → Ghidra `0x114E04`); stored code pointers and inline `CS:`
  tables resolve through `+0x10000`. A stored **data-object** pointer resolves
  through `+0x100000` (the reciprocal-table pointers `0x4C460`/`0x4D460` in
  the image are the runtime `0x14C460`/`0x14D460` BSS tables).
* Every numeric claim is quoted from the listings/reads; unproven items are
  open legs (no guessed labels).

## 1. The view matrix `0x54324` and its builders

### 1.1 `FUN_00058A44` sets up the frame

`FUN_00058A44` (`0x58A44`, 41 insns) is the view builder; `FUN_00058B68`
(`0x58B68`, the live match path, caller `FUN_00049830`) calls it as
`FUN_00058A44(EAX = ?, EDX = obj)`:

```
0x58A4D  LEA EAX,[EDX + 0xC]          ; angle pair pointer = obj+0xC
0x58A50  MOV EDX,0x54324
0x58A5A  CALL 0x4C4B4                 ; build matrix
0x58A5F  MOV EAX,[ESI + 0x4]
0x58A64  MOV [0x54354],EAX            ; matrix-region +0x30 = obj[1]
0x58A69  MOV EAX,[0x9A70]             ; camera vector pointer
0x58A6E  CALL 0x56E50                 ; camera-relative staging
0x58A73  MOV EAX,[0x9A70]
0x58A78  CMP dword ptr [EAX + 0x8],0x0
0x58A7C  SETG AL                      ; gate = camera.z > 0
0x58A84  MOV EBX,0x54324
0x58A89  CALL 0x62828                 ; EBX is not read (see §2.1)
0x58A8E  MOV EDX,ESI
0x58A90  MOV EAX,EBP
0x58A92  MOV ECX,0x6
0x58A97  CALL 0x590B0                 ; matrix consumer / projector
0x58A9C  MOV EDX,[ESI + 0x4]
0x58A9F  MOV EAX,[ESI + 0x14]
0x58AA7  CALL 0x589E0                 ; scale term [0x54350]
```

`obj` is the camera/view record: `obj[0..2]` position (also copied to the
`0x5774C` camera triple, FU-71), `obj[3]`/`obj[4]` the two view angles,
`obj[5]` the field feeding `0x589E0`. `[0x9A70]` points at the camera vector
(the same triple FU-71 tracks at `0x5774C/50/54`).

### 1.2 `FUN_0004C4B4` — compose the matrix

`FUN_0004C4B4` (`0x4C4B4`, 23 insns; `EAX` = angle pair, `EDX` = out):

```
0x4C4B6  SUB ESP,0x48
0x4C4BD  MOV EDX,[EAX + 0x4]          ; angle B = obj[4]
0x4C4C0  MOV EAX,ESP
0x4C4C2  NEG EDX
0x4C4C4  CALL 0x4C3B0                 ; M1 = X-rotation(-obj[4]) at ESP
0x4C4C9  LEA EAX,[ESP + 0x24]
0x4C4CD  MOV EDX,[EBX]                ; angle A = obj[3]
0x4C4CF  CALL 0x4C414                 ; M2 = Y-rotation(obj[3]) at ESP+0x24
0x4C4D4  PUSH ECX                     ; out
0x4C4D5  LEA EAX,[ESP + 0x4]
0x4C4D9  PUSH EAX
0x4C4DA  LEA EAX,[ESP + 0x2c]         ; M2
0x4C4DE  PUSH EAX
0x4C4DF  CALL 0xA2991                 ; out = M2 × M1
```

**Layout.** The result is a row-major 3×3 of 16.16 dwords at `0x54324`:
`m[0..2]` row 0, `m[3..5]` row 1, `m[6..8]` row 2, followed by the stray
fields `[0x54350]` (+0x2C, §2.3) and `[0x54354]` (+0x30 = `obj[1]`).

**Builders.** `FUN_0004C3B0` (`0x4C3B0`) and `FUN_0004C414` (`0x4C414`) call
`0xA1A60(angle)` and lay out (quoted post-`CALL` stores):

* `0x4C3B0` (angle = `−pitch`): `0x4C3C8 [ESI]=0x10000`, `0x4C3CE [ESI+4]=0`,
  `0x4C3D5 [ESI+8]=0`, `0x4C3EA [ESI+0x10]=out2`, `0x4C3F6 [ESI+0x14]=out1`,
  `0x4C400 [ESI+0x1C]=−out1` (EDX negated), `0x4C40B [ESI+0x20]=out2`:
  `M1 = [1,0,0; 0,cos,sin; 0,−sin,cos]` with `(sin,cos) = 0xA1A60(−pitch)`.
* `0x4C414` (angle = `yaw`): `0x4C42F MOV EAX,[ESP+4]` (=out2),
  `0x4C43A [ESI]=out2`, `0x4C458 [ESI+8]=−out1`, `0x4C446 [ESI+0x10]=0x10000`,
  `0x4C465 [ESI+0x18]=out1`, `0x4C46C [ESI+0x20]=out2`:
  `M2 = [cos,0,−sin; 0,1,0; sin,0,cos]` with `(sin,cos) = 0xA1A60(yaw)`.

**The multiply.** `FUN_000A2991` (`0xA2991`, 41 insns) is a row-major 3×3
multiply: with `ESI = A`, `EBX = B`, `EDI = out`, each output is
`(A[i]*B[j] + A[i+1]*B[j+3] + A[i+2]*B[j+6])`, accumulated with
`IMUL`/`ADD`/`ADC` over 64 bits and truncated by
`0xA29A9 SHRD EAX,EDX,0x10` (floor of the 16.16 product). So
`0x54324 = M2 × M1` and a **row vector** `v` transforms as `v × M`
(rotation order: yaw, then pitch).

### 1.3 The angle pair — `0xA1A60` / `0xCE3B0`

`FUN_000A1A60` (`0xA1A60`, 32 insns) takes a 16-bit-step angle and returns
`(sin, cos)` in 16.16:

```
0xA1A6B  MOV EAX,[ESP + 0x14]        ; angle
0xA1A6F  SAR EAX,0x6
0xA1A73  CALL 0xCE3B0                ; coarse (sin,cos) of angle>>6, 1024 steps
0xA1A7B  MOV EDX,[ESP + 0xc]         ; angle
0xA1A7F  AND EDX,0x3f
0xA1A82  IMUL EDX,EDX,0x6487e
0xA1A8E  SAR EDX,0x9                 ; k = ((angle&0x3F)*0x6487E)>>9
0xA1A8B  MOV EAX,[ESP]  ; cos coarse
0xA1A8B  SAR EAX,0x2
0xA1A91  IMUL EAX,EDX
0xA1A98  SAR EAX,0x15
0xA1A9B  ADD EBX,EAX                 ; sin = sin_c + ((cos_c>>2)*k)>>21
0xA1AA7  ... (cos = cos_c − ((sin_c>>2)*k)>>21, 0xA1AA3..0xA1AB9)
```

`FUN_000CE3B0` (`0xCE3B0`, 40 insns) folds the 10-bit coarse index
`k = (angle>>6)&0x3FF` by quadrant and returns `[EBX] = sin`, `[ECX] = cos`
from a single table, using the quadrant-reduced index (the `DEC AH`/`XOR AH,AH`
steps reduce `EAX` to `k & 0xFF`; `0xA1A60`'s output roles are confirmed by the
Q0 path `0xCE3C6 MOV EDX,[EAX*4+0x14E04]; MOV [EBX],EDX` then
`0xCE3D1 MOV EDX,[EAX*4+0x15204]; MOV [ECX],EDX` with
`0x15204 = 0x14E04 + 0x400`):

* Q0 (`k<256`): `sin = T[k]`, `cos = T[256−k]`
* Q1: `sin = T[256−v]`, `cos = −T[v]`, `v = k & 0xFF`
* Q2: `sin = −T[v]`, `cos = −T[256−v]`
* Q3: `sin = −T[256−v]`, `cos = T[v]`

The table `T` is 257 dwords at link `0x14E04` (Ghidra `0x114E04..0x115208`,
read: `T[0]=0`, `T[1]=0x192=402`, `T[2]=0x324=804`, …, `T[128]=0xB504=46340`,
`T[200]=0xF109=61705`, `T[246]=0xFF84=65412`, `T[255]=0xFFFE=65534`,
`T[256]=0x10000`). It is `floor(sin(2πk/1024)·65536)`: `sin(2π/1024)·65536 =
402.06`, `sin(π/4)·65536 = 46340.95` → `0xB504` quoted. The low 6 bits are
then corrected by the linear term above (`0x6487E = 411774`).

## 2. The cited consumers

### 2.1 `FUN_00062828` is not a matrix consumer (FU-85 errata 1)

`FUN_00062828` (`0x62828`, 125 insns) is called from `0x58A89` with `EBX =
0x54324` and `EAX = camera.z > 0`, but **never reads EBX**: after the
prologue it is `0x6282E MOV [0x96B4],EAX; 0x62833 CALL 0x63FD0` (the same
render-gate helper used by `FUN_00036C70`), then walks byte strings at
`[0x9A7C]`/`[0x9A80]` and copies `0x12C` bytes from `[0x543C8]` (the drawable
block +0xCFC) to `0x54CAC` with `0xCD390` before `0xCD390`/palette work:
a score/commentary **text** routine. `EBX` is preserved at
`0x62828`/`0x6290C` and never used. The matrix consumers are §2.2 and §4.

### 2.2 `FUN_000590B0` — delta, rotate, project

`FUN_000590B0` (`0x590B0`, 100 insns; `EAX` scratch, `EDX = obj`,
`EBX = 0x54324`, `ECX = 6` from `0x58A97`). Decoded body:

```
0x590BD  CALL 0x44394                ; refresh screen dims + reciprocal tables (§3)
0x590C2  EAX=[0x9A78]; AL=[EAX]; AND AL,0x40
0x590D2  EDI = 0x166                 ; else 0x115 (mode/flag selects the point count)
0x590DE  d.x = [0x5426C] − obj[0]    ; cached previous view-record x
0x590EA  d.y = [0x54270] − obj[1]
0x590F8  d.z = [0x54274] − obj[2]
0x59107  if (d.x==0 && d.y==0 && d.z==0) → 0x59135
0x59119  FUN_000A3110(0x10F, dst=[0x54358], a=[0x54358], b=d)   ; add d to 0x10F triples
0x59135  else: if (FUN_0005903C(obj, ECX) != 0) { [0x9094]=0; return; }  ; cull path
0x5914D  d' = −obj (x,y,z)
0x5916A  base2 = [0x54358] + 0xCB4   ; = 0x10F triples further in
0x59193  FUN_000A3110(EDI−0x10F, dst=base2, a=base2, b=d')      ; add −obj to the rest
0x59189  [0x9094] = 1
0x5919B  FUN_000A3010(EDI, src=[0x54358], M=EBX, dst=[0x54348]) ; rotate each point
0x591B2  FUN_0004C590(EDI, in=[0x54348], out=[0x5434C], 0)      ; project to 16.16 screen
```

`FUN_000A3110` (`0xA3110`, 29 insns) is `out[i] = a[i] + b[i]` over dword
triples. `FUN_000A3010` (`0xA3010`, 51 insns, identical shape to
`FUN_000A3090`) walks the source triples and calls `FUN_000A2914`
(`0xA2914`, 52 insns) per point; `0xA2914` computes

```
0xA2923  EAX = v[0]*M[0]; 0xA2931 ADD EBP,EAX; 0xA293b ADD EAX,EBP; ...
0xA293F  SHRD EAX,EDX,0x10            ; out[0] = (v0*M0 + v1*M3 + v2*M6)>>16
0xA2962  SHRD ... out[1] = (v0*M1 + v1*M4 + v2*M7)>>16
0xA2986  SHRD ... out[2] = (v0*M2 + v1*M5 + v2*M8)>>16
```

i.e. `out = v × M` in the same row-vector/64-bit-accumulate convention as
`0xA2991` (this is the same helper `FUN_00056E50` uses to rotate the staged
entity points into `[0x543C0]`, below). `[0x54358]` is a scratch point array
(the resource-0x3F block), `[0x54348]` the rotated output, `[0x5434C]` the
projected 16.16 screen output. The delta add (`[0x5426C..74] − obj`) uses the
previous view record cached by `0x58A44`; its purpose beyond relative
re-basing is open (leg 1).

### 2.3 `FUN_000589E0` — the scale term `[0x54350]`

`FUN_000589E0` (`0x589E0`, 29 insns; `EAX = obj[5]`, `EDX = obj[1]` from
`0x58A9C..0x58AA7`), with a two-word cache at `[0x54280]`/`[0x54270]`:

```
0x58A01  EBX = arg1*2
0x58A08  EAX = 0x14
0x58A0E  IDIV EBX                     ; q = 0x14 / (2·arg1), r = remainder
0x58A14  SHRD EAX,EDX,0x10
0x58A1B  IDIV EBX
0x58A1D  SHL ECX,0x10
0x58A20  ADD EAX,ECX                  ; (0x14<<16) / (2·arg1)
0x58A22  MOV [0x54350],EAX
0x58A27  CMP [0x54350],0x78
0x58A30  MOV [0x54350],0x78           ; clamp low
0x58A3A  MOV EAX,[0x54350]; RET
```

So `[0x54350] = max(trunc((0x14<<16)/(2·obj[5])), 0x78)`: a 16.16
reciprocal-like scale (20/(2·field)) stored in the matrix region at +0x2C.
No reader of `[0x54350]` was found (`search_byte_patterns 50430500`: only
`0x589E0` and the initialiser `0x58E00`), so its role is an open leg
(leg 2); it is **not** the entity projection scale (that is §3).

## 3. The perspective divide — `FUN_0004C590` and the reciprocal tables

### 3.1 `FUN_0004C4F0` builds two reciprocal tables

`FUN_0004C4F0` (`0x4C4F0`, 52 insns) is called by `FUN_00044394` whenever the
screen size changes. `0x44394`:

```
0x44397  EAX = [0x67AC]; if changed: [0x67AC] = [0x4B0E0]; [0x67B0] = [0x4B0E4]
0x443CA  [0x146B0] = [0x67AC] >> 16   ; integer screen width
0x443D7  [0x146B4] = [0x67B0] >> 16   ; integer screen height
0x443DF  CALL 0x4C4F0
```

`0x4C4F0` fills the X table at `[0x74C0]` and the Y table at `[0x74C4]`:

```
0x4C505  EBX=0; ECX=0xa
0x4C50C  loop: EDX = [0x146B0]; EAX = (EDX<<16); IDIV ECX (10)   ; [0x74C0][EBX]
         ... EDX = [0x146B4]; IDIV ECX; [[0x74C4]][EBX] = ...    ; first 10 entries
0x4C53B  EBX=0xa; ECX=0x28
0x4C545  loop: EAX = ([0x146B0]<<16); IDIV EBX (10,11,12,…)
         ... [[0x74C4]] likewise; until EBX == 0x400               ; 1014 more entries
```

So for `k = 0..0x3FF`: `tableX[k] = (W<<16)/max(k+1,10)` and
`tableY[k] = (H<<16)/max(k+1,10)` (positive IDIV = floor). The pointers are
the image values `0x4C460`/`0x4D460` (`read_memory 0x1074C0`: `60 C4 04 00 60
D4 04 00`), stored data-object pointers that resolve to the BSS tables
`0x14C460`/`0x14D460` (`read_memory 0x14C460/0x14D460` = zeros at rest);
`0x4C4F0` writes them in place.

The screen centre is `FUN_00043E48` (`0x43E48`, 88 insns):

```
0x43F00  EAX = arg5 (width); SAR EAX,1; ADD EAX,EDI (viewport x0); SHL EAX,0x10
0x43F21  MOV [0x146A8],EAX            ; centre x = (W/2 + x0)<<16
0x43F26  EAX = arg6 (height); SAR EAX,1; ADD EAX,ESI (viewport y0); SHL EAX,0x10
0x43F42  MOV [0x146AC],EAX            ; centre y
```

`FUN_0004C654` (`0x4C654`) saves/restores those two dwords around a
whole-pixel variant (it shifts the centre down to integer before the call and
restores the 16.16 value after), so the divide path can also render at pixel
resolution.

### 3.2 `FUN_0004C590` — the divide

`FUN_0004C590` (`0x4C590`, 58 insns): `arg1 = count`, `arg2 = in` (12-byte
triples), `arg3 = out` (8-byte pairs), `arg4 = shift base`. Per element:

```
0x4C5B6  EAX = in[2] (z)
0x4C5B9  CMP EAX,0x5; JL skip                 ; z < 5 → not visible, out untouched
0x4C5BE  EDX = arg4 (0)
0x4C5C2  if (z >= 0x10000) { EDX = arg4+8; SAR z,8 }
0x4C5CF  if (z >= 0x1000)  { SAR z,4; EDX += 4 }
0x4C5DC  if (z >= 0x400)   { SAR z,2; EDX += 2 }   ; z now the table index
0x4C5E9  EBP = z*4
0x4C5F0  ECX = [0x74C0]
0x4C5F8  IMUL EAX,[ECX + EBP]                 ; (x·tableX[z]) low 32
0x4C5FC  MOV CL,DL
0x4C5FE  SAR EAX,CL                           ; >> shift
0x4C602  ADD ECX,[0x146A8]                    ; + centre x
0x4C60D  MOV [ESI],ECX                        ; out.x
0x4C60F  EBP += [0x74C4]
0x4C611  EAX = in[1] (y)
0x4C614  IMUL EAX,[EBP]
0x4C618  SAR EAX,CL
0x4C61A  EDX = [0x146AC]; SUB EDX,EAX
0x4C624  MOV [ESI + 0x4],EDX                 ; out.y
0x4C627  ESI += 8; EBX += 0xC; next element
```

With `z' = z >> shift` and `tableX[z'] = (W<<16)/max(z'+1,10)`, this is

```
out.x = centre.x + (x · W<<16 / z) >> 0        (16.16)
out.y = centre.y − (y · H<<16 / z)
```

i.e. a **true perspective divide** implemented as a reciprocal-table multiply
plus shift. The `max(k+1,10)` floor and the `z' = z>>shift` normalisation mean
the divide is exact only up to truncation (the table uses `z'+1`, and the
quadrant shifts discard low bits) — the port reproduces the integer sequence
bit-for-bit. The `[0x4C654]` wrapper (`0x4C654`) calls it with `shift base
0x10` for already-small `z`; `FUN_0004C640` (`0x4C640`) is the single-point
wrapper `0x4C590(1, EAX, EDX, 0)`.

## 4. The per-slot screen tables `0x54364`/`0x54388` (FU-85 leg 2)

**They are pointers, not positions.** `FUN_00058E00` allocates the drawable
block (`FUN_0004A448` = `FUN_00098C38(name=0x1E6C, size=0x2094, flags=0x220)`,
quoted `0x58E25 MOV EAX,0x1e6c; 0x58E2a CALL 0x4A448` with `EDX=0x2094` from
the resource call) and sets (`0x58F5C MOV EDX,[0x5434C]` = block+0x138C):

```
0x58FA1  LEA EBX,[EDX + 0xbf0]        ; block+0x1F7C
0x58FAD  MOV [0x54364],EBX
0x58FB3  LEA EBX,[EDX + 0xb38]        ; block+0x1EC4
0x58FBF  MOV [0x54388],EBX
```

So `[0x54364] = [0x5434C]+0xBF0` and `[0x54388] = [0x5434C]+0xB38`; the 16.16
positions live at `[[0x54364]+slot*8]`, the second table at
`[[0x54388]+slot*8]` — exactly what `FUN_00057158` reads (`0x5716A MOV
EAX,[0x54364]; 0x5716F LEA EDI,[EAX+EBP]` with `EBP=slot*8`;
`0x57180 MOV EDI,[EDI+4]`, `0x57227 MOV EDX,[0x54364]; ADD EDX,EBP` feeding
`0x57080`).

**The writer is the render body at `0x57753`** (the prologue quoted from
`read_memory`: `0x57753 PUSH ESI; PUSH EDI; PUSH EBP; SUB ESP,0xC8`; Ghidra
does not delimit it — the preceding function `FUN_00057594` returns at
`0x57750`). Inside its per-entity loop (the same draw list
`0x4EAE8`/`0x4E7B0`/`0x4E6F0` that `FUN_00056FA4` walks):

```
0x57CE0  EBP = [0x4E6F0+i*4] − 1          ; staging slot
0x57CE3  ESI = EBP*12
0x57CE6  if ([0x55AB4 + ESI] == −10000) skip
0x57CFB  if ([0x9A74][ESI] > 0x8E0) skip
0x57D08  EBX = EBP*8
0x57D1D  EAX = [0x54364]; PUSH 0; EAX += EBX; PUSH EAX
0x57D27  EAX = [0x543AC]; EAX += ESI; PUSH EAX; PUSH 1
0x57D31  CALL 0x4C590                     ; project A+0x11E8[slot] → [0x54364]+slot*8
0x57D39  EAX = [0x54388]; ECX = EBP*8; PUSH 0; EAX += ECX; PUSH EAX
0x57D4A  EAX = [0x543C0]; EAX += ESI; PUSH EAX; PUSH 1
0x57D54  CALL 0x4C590                     ; project A+0x10D4[slot] → [0x54388]+slot*8
0x57D5C  EAX = [0x54388] + slot*8
0x57D71  ECX = [obj + 0x24]
0x57D74  if (ECX > [EAX+4]) {             ; depth override
0x57D7F      if ([0x54364]+slot*8+4 > [obj+0x1C]) { … [0x54388]+slot*8+4 = [obj+0x18]−2Δ … }
0x57D97      else [0x54388]+slot*8+4 = [obj+0x24]
         }
```

The two source arrays are the rotation output of `FUN_00056E50`:

```
0x56E6C  memcpy(src=0x55AB0, dst=[0x543A8], 0x114)   ; res+0x11E8, 23 triples
0x56E85  memcpy(src=0x55AB0, dst=[0x543BC], 0x114)   ; res+0x10D4
0x56EBF  FUN_000A3110(0x17, [0x543A8], −cam, [0x543A8])
0x56EDB  loop 23×: [0x543BC+i] += (−cam.x, −cam.y + [0x4E9B8+i*4], −cam.z)  ; jitter only here
0x56F0F  CALL 0x58870
0x56F24  FUN_000A3090(0x3A, src=[0x543BC], M=0x54324, dst=[0x543C0])
```

Both chunks start as copies of the staged positions `0x55AB0` (FU-85 §4),
both get `−camera`; chunk `[0x543BC]` (res+0x10D4) additionally gets the
per-slot `i%6+0x70` jitter; the rotation runs over 58 triples from
`[0x543BC]` (the two 23-point chunks plus 12 adjacent points) into
`[0x543C0] = block+0x10D4`. So `[0x543AC] = block+0x11E8` holds the rotated
**clean** positions, `[0x543C0]` the rotated **jittered** ones; the render
body projects the clean array to `[0x54364]` and the jittered array to
`[0x54388]`. `FUN_00057158` then computes the sprite scale from the two
screen-y values (`0x57187..0x571FD`: `EDI = [0x54364+slot*8+4] −
[0x54388+slot*8+4]`, `+0x1000`, low word cleared, `×0x5D1>>16`), so the
`[0x54388]` y is a screen-space size reference, later overridden by the
`obj+0x24` depth logic above. The jitter semantics of the `i%6+0x70`
addend and the `obj+0x18/0x1C/0x20/0x24` depth fields are open (leg 3).

## 5. The world→screen transform

For one entity at staging slot `s` (world position `p`), the evidenced chain
is:

```
q = p − cam (+ y-jitter for the second array)      ; 0x56E50, 0x590B0
r = q × (Ry(obj[3]) × Rx(−obj[4]))                  ; 0xA2914 / 0x590B0's 0xA3010
z' = r.z >> shift, normalised into [0x100,0x3FF]   ; 0x4C590
screen.x = ((W/2 + x0)<<16) + (r.x · W<<16 / r.z)>>0
screen.y = ((H/2 + y0)<<16) − (r.y · H<<16 / r.z)   ; center from 0x43E48
```

The result is 16.16 screen coordinates; the rasteriser later scales by
`0x5D1` and integer-truncates (`FUN_00057158` / `FUN_00057080`). There is
**no FOV or zoom term** in this transform: the scale is `W/z`/`H/z`
(the reciprocal tables), and the camera height `[0x57750]` enters only as the
Y of the `[0x9A70]` camera vector used by the camera-relative subtraction
(FU-71's camera model). `[0x8DDC]` (zoom) is used by the 2D radar/panel
routines (`FUN_00053240`, `FUN_00054AE4`, `FUN_0005619C`, …), not by the
entity projection (`search_instructions 8ddc`: no hit in the 0x57xxx/0x59xxx
chain). The near plane is `z < 5` (skip, out untouched) and depth is
otherwise unclipped; the tables saturate at index 0x3FF, so the port marks a
normalised index of 0x400+ invisible instead of reading one entry past (the
original reads `[0x74C0]+0x1000`, the first Y-table dword).

## 6. Port: `fifa96_projection`

`include/fifa96_loader/fifa96_projection.h` +
`src/fifa96_loader/fifa96_projection.c` (caller-owned buffers, no globals,
negative `fifa96_err_t` for invalid arguments, no comments). The port models
the integer transform exactly; it does not load resources or touch the match
globals.

| original | port |
|---|---|
| `0x4C3B0`/`0x4C414` layouts + `0xA2991` row-major multiply (64-bit accumulate, floor `>>16`) | `fifa96_projection_matrix` |
| `0xCE3B0` quadrant fold over the 257-entry table + `0xA1A60` 6-bit correction | `fifa96_projection_sincos` (table embedded from `0x114E04`) |
| `0xA2914` row-vector × matrix | `fifa96_projection_transform` |
| `0x4C4F0` `(dim<<16)/max(k+1,10)` over 0x400 entries | `fifa96_projection_reciprocal` |
| `0x4C590` z-normalise + reciprocal multiply + centre add | `fifa96_projection_screen` |
| camera-relative subtraction + rotate + project | `fifa96_projection_project` |
| `0x28`-style resource/screen plumbing (`FUN_00044394`, `0x67AC`, `0x4B0E0`, `0x43E48`), the `[0x54350]`/`[0x54354]` matrix-region strays, `FUN_00056870`-family render bodies | not ported (globals/resources; cited in §1–4) |

Divergences (documented): the original reads `[0x74C0]+0x1000` (one entry
past the X table) when the normalised index reaches 0x400; the port returns
`visible = 0` without writing `out`. The original `IMUL` truncates the 32-bit
product; the port wraps the same way with `uint32_t` multiplication. The
original `0x4C4F0` shifts `dim<<16` in 32 bits (overflow above `0x7FFF`); the
port rejects `dim <= 0` or `dim > 0x7FFF`. The port's `fifa96_projection_point`
is written only when `visible = 1` (the original leaves it untouched).

## 7. Tests (`tests/test_projection.c`, suite 75 → 76)

* Layout `_Static_assert`s (`fifa96_projection_vec`/`point`) and the
  `0x400`/`5` constants.
* `sincos`: axis angles (`0x0000`, `0x4000`, `0x8001`, `0xC000`, `−0x4000`),
  the 1-step correction (`0x0001 → (6, 65536)`, computed from the quoted
  `0x6487E` term), table-derived `0x1000/0x2000/0x3456`, NULL errors.
* `matrix`: identity at `(0,0)`; `yaw = 0x2000`; `pitch = 0x4000`
  (`[0x10000,0,0, 0,0,−0x10000, 0,0x10000,0]`); both-angles case; NULL.
* `transform`: identity passthrough, the `yaw = 45°` map of `(65536,65536,0)`
  and of `(−1,−2,−3)` (negative-product floor), NULLs.
* `reciprocal`: `dim=320` boundaries `[0]/[9]/[10]/[255]/[256]/[512]/[1023]`,
  `dim=200`, invalid `0`/negative/`0x8000`/NULL.
* `screen`: exact 16.16 values for hand points (`513`/`−513` at `z=0x2000`,
  `z=0x10000`, `0x3FF` vs `0x400` normalisation), `z<5` and
  `z=0x1000000` (index 0x400, out untouched, visible 0), `z=5`, NULLs.
* `project`: identity matrix + zero camera equals `screen`; camera subtraction
  invariance (`(1026,0,0x4000) − (513,0,0x2000)` projects like
  `(513,0,0x2000)`); near-plane invisibility; NULL.
* `make test`: 75/75 before, **76/76 after**;
  `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
  tests/test_projection.c src/fifa96_loader/fifa96_projection.c` runs clean.

## 8. Errata (quoted)

* FU-85 §2/§5 "the 16.16 screen positions `0x54364`/`0x54388`" and leg 2
  "per-frame writers ... reached through computed pointers" — **corrected and
  closed**: `0x54364`/`0x54388` are **pointer globals** into the drawable
  block (`0x58FA1..0x58FBF`: `0x54364 = [0x5434C]+0xBF0`,
  `0x54388 = [0x5434C]+0xB38`); the 16.16 pairs are `[[0x54364]+slot*8]`.
  The writers are `FUN_0004C590` calls at `0x57D31` and `0x57D54` in the
  unanalysed render body starting `0x57753`.
* FU-85 §4 / leg 1 "matrix consumers `FUN_00062828`/`FUN_000590B0`/
  `FUN_000589E0`" — **corrected**: `FUN_00062828` never reads the matrix
  pointer passed in EBX (`0x6282E MOV [0x96B4],EAX` then a text/score path);
  the consumers are `FUN_000590B0` (rotate+project a point array) and the
  `0x57753` render body (project two entity point arrays per slot).
  `FUN_000589E0` merely computes the matrix-region scale `[0x54350]`.
* FU-85 §8 / leg 3 "no perspective divide was found (leg 3)" — **corrected**:
  `FUN_0004C590` performs the divide via the reciprocal tables built by
  `FUN_0004C4F0` (`(dim<<16)/max(k+1,10)`), with the z shift-normalisation at
  `0x4C5C2..0x4C5E6`; the screen result is `centre + x·dim/z` in 16.16.
* FU-85 §8 "camera height `[0x57750]` / zoom `[0x8DDC]` role in the entity
  projection" — **reframed**: `[0x57750]` (camera Y, FU-71) enters through the
  `[0x9A70]` camera vector; `[0x8DDC]` is not in this chain (2D radar/panels);
  the entity scale is the screen dimension over depth.
* FU-85 §4 "`FUN_0004C4B4` composes two rotations (`FUN_0004C3B0(out,
  -[ptr+4])`, `FUN_0004C414(out, [ptr])`)" — **confirmed** and pinned:
  `M = Ry([obj+0xC]) × Rx(−[obj+0x10])`, row-major 16.16; the `0xA2991`
  multiply is `A × B` (rows of A · columns of B) with a single 64-bit floor.

## 9. Open legs

1. **`FUN_000590B0` delta semantics**: why `[0x5426C..74] − obj` is added to
   the first 0x10F triples of `[0x54358]` and whether the cached record is the
   previous frame's or a sibling render context's; the `0x5903C` cull path
   is not decomposed.
2. **`[0x54350]` reader**: the scale computed by `FUN_000589E0` has no
   located consumer (`search_byte_patterns 50430500`).
3. **Jitter/depth fields**: the `i%6+0x70` addend (`0x4E9B8`),
   `0x4EA10`, and `obj+0x18/0x1C/0x20/0x24` in the `0x57D5C..0x57DFB` depth
   override are cited, not derived.
4. **`FUN_00056E50`'s rotation count 0x3A** over a 46-point staging pair plus
   12 adjacent points: the extra 12 points and `FUN_00058870`'s role are not
   derived.
5. **`0x57753` body**: only the writer/loop sites above are decoded; the
   function's full prologue/consumers (it copies `[0x54374]→[0x54370]` at
   +0xC8 size) are open.
6. **`0x4C654` whole-pixel variant**: its callers (`0x57FDF`, `0x58141`,
   `0x5825D`) and the 16-entry quads it writes are cited only.
7. **FU-85 leg 16** (`/tmp/opencode/fifa96_le.bin` differs from the Ghidra
   session image): carried; all data quotes above are from Ghidra memory
   (the local file shows zeros at the `0x114E04` table).

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x58A44, 0x4C4B4, 0x4C3B0, 0x4C414, 0xA2991, 0xA2914,
0xA3010, 0xA3090, 0xA1A60, 0xCE3B0, 0x62828, 0x590B0, 0x589E0, 0x57158,
0x56FA4, 0x56E50, 0x56CF4, 0x58E00, 0x4C590, 0x4C4F0, 0x4C640, 0x4C654,
0x44394, 0x43E48, 0x4A448, 0xA3110, 0x572E8, 0x57FDF, 0x5FF24, 0x60018,
0x62588, 0x62620, 0x57594;
`disassemble_bytes` 0x4C3B0 (261 B), 0x57754 (160 B), 0x57686 (288 B),
0x590B0 (336 B), 0x59960 (144 B), 0xA2A10 (64 B), 0x5B448;
`read_memory` 0x1074C0, 0x114E04 (257-entry table), 0x115204, 0x14C460,
0x14D460, 0x4C3C8, 0x55753/0x5773C, 0x57880, 0x57A80, 0x57B48, 0x57BE0,
0x57D00, 0x57FD8, 0x114A04, 0x115E04;
`search_instructions` operands `54324`, `54364`, `54388`, `54350`, `5434C`,
`54380`, `5435C`, `54368`, `57750`, `8ddc`, `146a8`, `146ac`, `67ac`,
`74c0`, `58b68`, `0x11e8`, `0xbf0`;
`search_byte_patterns` `64430500`, `88430500`, `4c430500`, `50430500`,
`ac430500`, `c0430500`, `a8430500`, `68430500`, `6efeffff`;
`get_function_xrefs` 0x4C590, 0xA3010, 0x4C640, 0x4C654; `get_function_callers`
0x56FA4, 0x58B68, 0x58BC0, 0x58D70; `get_function_by_address` 0x590B0,
0x58870, 0x57754.
Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_projection.h`,
`src/fifa96_loader/fifa96_projection.c`, `tests/test_projection.c`,
`CMakeLists.txt` (one library/test block). `make test`: 75/75 before,
**76/76 after**; ASan+UBSan `test_projection` clean.
`game/FIFAPCCD96.iso` untouched; `fifa96.rep/**` churn not staged.
