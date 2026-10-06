# FU-85: the match renderer — sprite-frame resolver, draw paths and the canvas flow

Follow-on to FU-84 §7 (the resolver `FUN_00078FAC`, the draw paths `0x563C7`/
`FUN_00057158`, and the open legs on the sprite bank and the presentation
consumer). This slice derives the resolver instruction-by-instruction, both
draw paths, the sprite-bank data behind the animator records, the staging that
feeds them, and the canvas the composite lands on; the clean math is ported as
`fifa96_render`.

Result in one line: **`FUN_00078FAC` is a pure frame resolver: it takes
`(anim_id, frame_index, direction, row)`, clamps the id to `0..0x6E`, reads
`sprite = byte[row.frame_table + frame_index*5 + 4]` and `bank = row+8`, walks
a per-id compare tree that folds the 0..7 direction into
`(record, dir', mirrored)` pairs (`8-dir` flips, `dir>=3/4/5` bumps the animator
record), computes a byte offset `animator[+4]*dir' + sprite` and resolves it
through a **segmented resource handle** (`[handle+8]` = entry count,
`handle + [handle+off*8+0x14]` = pointer) whose data is the planar sprite bank;
some id classes additionally resolve a **second overlay frame** (ids
`0x35/0x37/0x39/0x3B` through `banks[bank+2]` with the `0x10F2E7` remap byte,
ids `0x40/0x42` through `banks[bank+1]`, ids `0x60/0x61` through fixed runtime
records) written to the second out slot; both out slots are consumed by the two
raw blits of `FUN_00057158`/`0x563D0` through `FUN_00057080` →
`FUN_000A2E24` (pivot placement) → `FUN_000A2C50` (clip pre-pass + 16.16
stepping) → `FUN_000CEABC`, a 4-plane span writer into the VGA surface state
block `0x131C0..0x131EF` (`[0x131E8]` scanline table, `[0x131C0]` stride,
`[0x131EC]` base, clip `[0x131C8..0x131D4]`) with the `0x14720` 256-byte remap
and `0xFF` = transparent; the `0x563C7` path is the generic version of the same
resolver+blit sequence driven by `[0x8FA8]/[0x8F44]/[0x8E6C]` and its only
caller is the selected-entity overlay at `0x55B4E`; the match does **not** use
the movie-player globals `[0x563CC]`/`ctx[10]`/`0x560C0`, and the FU-56 blit
`FUN_000AE7F0` is not in the match chain — the match rasterizes straight into
the VGA window and presents by installing a new surface state block
(`FUN_000CE6F0`, saved by `FUN_000CD758`).**

## Method

* Static work on the open Ghidra MCP session for `/fifa96_le.bin` (FU-4 linear
  image, link addresses). `disassemble_function`/`disassemble_bytes` are the
  citation source. The `0x563xx` draw region is not a Ghidra function and its
  first byte at `0x563BA` mis-aligns the following byte stream, so the bodies
  at `0x563BC..0x5649F` were hand-decoded from `read_memory 0x56380` (320 B)
  and checked against the Ghidra instruction boundaries that do exist
  (`0x56380/0x56381/0x56382`, `0x56384..`, `0x5644C`, `0x56467`,
  `0x564A0..`); the same method was used for the `0x55B4E` window
  (`0x55900`, 640 B) where the analyzer loses alignment after `0x55A58`.
* **Address mapping (FU-76/FU-84, restated).** Code/function addresses equal
  true link addresses; a **data immediate** `A` is storage at flat
  `A+0x100000` (`[0x57588]` → `0x157588`, `[0x8FA8]` → `0x108FA8`,
  `0x8E6C` → `0x108E6C`, `0x10F2E7` → `0x110F2E7`); stored code pointers and
  inline `CS:` tables resolve through `+0x10000`.
* Every numeric claim is quoted from the listings/reads; unproven items are
  open legs (no guessed labels).

## 1. `FUN_00078FAC` (`0x78FAC..0x79588`, 450 insns) — the frame resolver

Inputs (from the call sites): `AL = anim_id`, `DL = frame_index` (both byte),
`EBX = direction` (masked `& 7`), `ECX = out1` (mirror flag out), stack arg =
`out2` (overlay pointer out). `EDI`/`ESI` are scratch. Entry:

```
0x78FB2  DH = AL
0x78FB4  ESI = ECX                    ; out1
0x78FB6  EAX = [ESP+0x34]             ; out2
0x78FBA  [EAX] = 0                    ; zero out2
0x78FC0  EAX = MOVSX DH
0x78FC3  [ECX] = 0                    ; zero out1
0x78FC9  [ESP+0x1C] = EAX
0x78FCD  if (EAX >= 0x6F) return 0    ; signed byte gate
0x78FDF  EDI = [0x57588]
0x78FE5  ECX = MOVSX DL               ; frame index
0x78FE8  EDI += id*9                  ; row
0x78FEA  EAX = ECX*5
0x78FF3  ECX = [EDI+4]                ; frame table
0x78FF6  EBP = byte[ECX+EAX+4]        ; frame sprite byte
0x78FFD  CL = [EDI+8]                 ; row sprite-bank index
0x79002  EBX &= 7
0x79005  [ESP+0x20] = 0               ; overlay offset
```

The id classes (unsigned tree over `DH`, `0x79009..0x790C4`): `0x1B` →
`0x7939E`; `0x2F` → `0x793EC`; `0x34..0x37` → `0x790C7`; `0x38..0x3B` →
`0x791B4`; `0x3D/0x3E` → `0x79343`; `0x3F..0x42` → `0x7929E`;
`0x4A/0x4F/0x51/0x54/0x56/0x5A/0x67/0x6B` → `0x793EC`; everything else
(including `0x3C`, `0x43..0x49`, `0x57..0x59`, `0x5B..0x66`, `0x68/0x69`,
`0x6C..0xFF`) → `0x79446`. Because the gate at `0x78FCD` is **signed**, ids
`0x80..0xFF` (negative bytes) reach the default handler; the handlers then use
signed compares for the id-specific branches (`0x79151`, `0x792f6`, `0x794b8`).

The animator array is `0x57DE8 + bank*0x18`; each handler first tests
`[rec+0x14]` and, **when it is zero**, calls the lazy loader `0x78DAC`
(`0x79111/0x79176/0x791fe/0x79263/0x7931d/0x79376/0x793b8/0x7941a/0x79497`),
returning 0 if it fails. The record fields used here are `+4` (per-direction
step) and `+0x14` (resource handle).

### 1.1 Handlers

| ids | site | direction fold | record select | offset |
|---|---|---|---|---|
| `0x34..0x37` | `0x790C7` | `0x36/0x37`: `dir=(8-dir)&7`, mirror=1 (`0x790D5..0x790E4`) | `bank + (dir>=4)` (`0x790E7..0x79109`) | `rec[+4]*(dir>=4?dir-4:dir) + sprite` (`0x79126..0x79142`) |
| `0x38..0x3B` | `0x791B4` | `0x3A/0x3B`: `dir=(8-dir)&7`, mirror=1 (`0x791C2..0x791D1`) | `bank + (dir>=4)` (`0x791D4..0x791F6`) | same (`0x79213..0x79233`) |
| `0x3F..0x42` | `0x7929E` | `0x41/0x42`: `dir=(8-dir)&7`, mirror=1 (`0x792AC..0x792BB`) | `bank` (no bump) | `rec[+4]*dir + sprite` (`0x792E7..0x792F2`) |
| `0x3D/0x3E` | `0x79343` | `0x3E`: `dir=(8-dir)&7`, mirror=1 (`0x7934A..0x79359`) | `bank + (dir>=4)` (`0x7935C..0x7936E`) | `rec[+4]*(dir>=4?dir-4:dir) + sprite` (`0x79387..0x79395`) |
| `0x1B` | `0x7939E` | none | `bank + (dir>=4)` (`0x7939E..0x793B0`) | same (`0x793CD..0x793E3`) |
| flat list | `0x793EC` | `dir>=5`: `dir=8-dir`, mirror=1 (`0x793EC..0x793FE`) | `bank + (dir>=3)` (`0x79400..0x79412`) | `rec[+4]*(dir>=3?dir-3:dir) + sprite` (`0x7942F..0x7943D`) |
| default | `0x79446` | `dir>=5`: `dir=8-dir`, mirror=1 (`0x79474..0x7947D`); `id 0x1A`: mirror=1, then the numeric fold with mirror=0 (`0x7945D..0x7946C`) | `bank` | `rec[+4]*dir + sprite` (`0x794AC..0x794B4`) |

The flat list is exactly `{0x2F,0x4A,0x4F,0x51,0x54,0x56,0x5A,0x67,0x6B}`.
Composite selectors inside the handlers:

* `0x34..0x37` (`0x7915F..0x791AF`): for `0x35/0x37`, second =
  `banks[bank+2]` (`0x7915F..0x7918B`), raw = `second[+4]*dir + sprite`,
  `[ESP+0x20] = raw` (`0x7918B/0x79197`), and
  `m = byte[raw + 0x10F2E7]` (`0x79191`): if `m != 0` the overlay offset
  becomes `m-1` and `EDI = second` (`0x7919F..0x791A8`); if `m == 0`,
  `EDI = 0` (`0x791AD`).
* `0x38..0x3B` (`0x7924C..0x79299`): same shape, second for `0x39/0x3B`.
* `0x3F..0x42` (`0x79304..0x79330`): for `0x40/0x42`, `ECX++` and `EDI =
  banks[bank+1]`, overlay offset = `second[+4]*dir + sprite`.
* default `0x60/0x61` (`0x794B8..0x79516`): `0x61` selects the fixed record at
  object-4 flat `0x1584F0` gated by `[0x58504]`; `0x60` selects flat
  `0x1584D8` gated by `[0x584EC]`; both are **zero in the stored image**
  (BSS at runtime, `read_memory 0x1584D8/0x1584F0` = 48 zero bytes), so the
  fixed records are runtime-populated. Overlay offset = `fix[+4]*dir + sprite`.

### 1.2 Tail resolution and the segmented handle

```
0x7951A  if (EDI && EDI[+0x14]) {
             if (count(EDI[+0x14]) > [ESP+0x20])
                 [out2] = data(EDI[+0x14], [ESP+0x20])     ; 0x7952E..0x79547
         }
0x79549  if (!ESI || !ESI[+0x14]) return 0
0x7955C  if (count(ESI[+0x14]) <= [ESP+0x18]) return 0
0x7957E  return data(ESI[+0x14], [ESP+0x18])
```

with (`0xA1918`, 3 insns) `count = dword[handle+8]` and (`0xA1920`, 9 insns)
`data(handle, off) = (off < dword[handle+8]) ? handle + dword[handle+off*8+0x14]
: 0`. So a bank is a **segmented resource**: a 4-byte entry count at `+8`, then
8-byte entries at `+0x14` whose first dword is a displacement relative to the
handle base. The two doublewords of an entry are the displacement and a second
field not read by this path. `FUN_00078FAC` returns the main frame pointer (or
0) and stores the overlay frame pointer (only for the composite classes) at
`out2`; `out1` receives 1 when the frame is mirrored.

`FUN_00078F1C` (`0x78F1C..0x78FAB`, 52 insns) fills the `0x57DE8` records:
`EAX=0x47; CALL 0x4AFB8` (resource entry), `FUN_000A2718(entry)` = count,
then per record: `+0 = i` (`0x78F4C`), `+0x10 = FUN_000A275C(entry, i)`
(`0x78F5B/0x78F61`), `+0xC = FUN_0009E890(+0x10)` (`0x78F69`), and
`+0x14 = (+0xC == 0) ? +0x10 : 0` (`0x78F73..0x78F83`) with `FUN_00078C68`
state dispatch when `+0xC == 0`; tail `FUN_00078B20` (`0x78F9D`). `+4` (step)
is not written here; the loader `0x78DAC` calls `0x9E860([rec+0x10], handle)`
(`0x78E5E`) after `FUN_00098C38` opens the resource and `+0x14` is set
(`0x78E4F`), so `+4` is a property of the loaded bank data (open leg 4).
The `0x57DE8` array is BSS (zeros at rest; cleared by `FUN_00078B20`).

### 1.3 The `0x10F2E7` mirror byte table

`read_memory 0x10F2E0` (64 B): `07 00 66 ee 00 00 53 00 | 00 00 01 02 03 04
00 00 00 00 05 06 07 00 00 00 00 00 08 09 0a 0b 0c 00 00 00 0d ...`. The table
starts immediately after the 111×9-byte row table (`0x10EF00 + 0x3E7 =
0x10F2E7`) and has runs of 5 non-zero values separated by 3 zeros. It is
indexed by the composite raw frame offset and its non-zero value selects the
overlay's **sub-frame index** (`byte-1`) inside `banks[bank+2]`; a zero leaves
the overlay unresolved (offset stays raw, pointer NULL). Role beyond this
lookup is open (leg 7).

## 2. Draw path `FUN_00057158` (`0x57158..0x57277`, 88 insns) — per-entity sprite

Caller `FUN_00056FA4` (`0x56FA4`, 63 insns) walks a sorted draw list:
`[0x4EAE8]` = 0x18 entries; entry `i` = `[i*4 + 0x4E6F0]` (1-based render slot,
0 = empty) and `[i*4 + 0x4E7B0]` (depth key, stop when the caller's key is
smaller, `0x56FDE`). For a live entry the gate is
`[slot*0xC + 0x55AB4] != -10000` (hidden marker, `0x5702A`),
`[9A74][slot*0xC] < 0x8E0` (`0x57027`) and
`[0x54388 + slot*8 + 4] < [obj+0x24]` (`0x57043`); then
`CALL 0x57158(slot)` and `CALL 0x57594(slot, [0x54364], ...)`.

`EAX = slot (1-based)`. Body:

```
0x57163  EBP = slot*8
0x5716a  EDI = [0x54364] + EBP; EDX = [0x54388] + EBP
0x57180  EDI = [EDI+4]; EDX = [EDX+4]; EDI -= EDX      ; size base (16.16)
0x57187  EAX = [slot*4 + 0x55BC4]; EAX -= 0x1000
0x571aa  ECX = [slot*0xC + 0x543A8]; EBX = [slot*0xC + 0x543A8 + 8]
0x571b8  CALL 0xA2A10                                  ; angle addend (CS table 0x929F0)
0x571ca  EAX += [slot*4 + 0x55BC4] - 0x1000
0x571d1  EAX &= 0xFFFF; EAX >>= 13                     ; (angle + addend) → 0..7
0x571df  EBX = 7 - EAX; EBX &= 7                       ; sprite direction
0x571e3  DL = byte[slot + 0x55C37]                     ; frame index
0x571ef  AL = byte[slot + 0x55C20]                     ; animation id
0x571f8  CALL 0x78FAC
0x571fd  EDI = ((EDI + 0x1000) & 0xFFFF0000) * 0x5D1 >> 16   ; scale (16.16)
0x57218  EAX = slot; CALL 0x48DC0                      ; palette install
0x5723c  CALL 0x57080(sprite, scale, &table1[slot], zoom)
0x57241  if (out2) { CALL 0x490F4; CALL 0x57080(out2, ...); }
```

Tables: `[0x54364 + slot*8]` is a 16.16 screen position (`FUN_00057080`
reads `[+0]>>16` as x and `[+4]>>16` as y, `0x57092..0x5709d`), and
`[0x54388 + slot*8 + 4]` is the second depth/size value; their difference
(+0x1000, low word cleared, ×`0x5D1`/0x10000) is the scale passed to the
blitter. `[0x543A8 + slot*0xC]`/`[+8]` are the staged position pair feeding the
angle helper. `[slot*4 + 0x4EA14]` is pushed as the blit argument (zoom/canvas
selector, cited).

`FUN_00048DC0` (`0x48DC0`, 84 insns) is the **palette installer**: when
`[0x68E0] != 1` it calls `0xCE980([slot*4 + 0x4BF60])` (`0x48EB9/0x48EC1`);
when `[0x68E0] == 1` and the slot is 0 or 0xB it copies the palette to the
stack and translates it through the byte tables `0x7287`/`0x727C` with bases
`0x94/0x9B` and `0x82/0x89` and offsets `0xA1/0xA3`/`0x9C/0x9E`
(`0x48E10..0x48EB6`), otherwise the plain path again. Palette semantics beyond
this are open (leg 9).

`FUN_00057080` (`0x57080`, 80 insns): `EAX` = sprite header, `EBX` = scale,
`EDX` = `{x,y}` 16.16, `ECX` != 0 = mirror, stack = zoom. It returns when the
sprite is NULL, reads `x=[EDX]>>16`, `y=[EDX+4]>>16`, then branches on
`0x44BE0()` and the scale: the main branch (`0x570B1..0x5711C`) scales the
sprite header size `[+2]>>16`/`[+4]>>16` by the scale (32×32→16 fixup with
`SHRD`) and calls `FUN_000A2E24`; the alternate branch (`0x5711E..0x57147`)
calls `FUN_0005E1A4`. Both are sprite rasterizers; only the first is decomposed
here.

`FUN_000A2E24` (`0xA2E24`, 78 insns) is the pivot placement: with
`sprite_w=[sprite+2]>>16`, `sprite_h=[sprite+4]>>16`, `pivot_x=[sprite+6]>>16`,
`pivot_y=[sprite+8]>>16`, `scale_x = (w<<16)/sprite_w`,
`scale_y = (h<<16)/sprite_h` (signed by the passed sizes):
positive scale → `x' = x - scale*pivot>>16`, `y' = y - scale*pivot>>16`
(`0xA2E5A..0xA2EB8`); negative scale →
`x' = x + scale*(sprite_w-pivot_x)>>16`,
`y' = y + scale*(sprite_h-pivot_y)>>16` (`0xA2E78..0xA2ED6`). Then it calls
`FUN_000A2C50(sprite, w, h, x', y')` (`0xA2EEB`).

`FUN_000A2C50` (`0xA2C50`, 159 insns) is the clipped, scaled planar copy:

```
0xA2C6E  sprite_w = [sprite+2]>>16; sprite_h = [sprite+4]>>16
0xA2C7E  if (w == 0 || h == 0) return
0xA2C8E  if (x >= [0x131D0] || y >= [0x131D4]) return
0xA2CA6  if (x + |w| <= [0x131C8] || y + |h| <= [0x131CC]) return
0xA2CDE  dx = (sprite_w<<16)/w; dy = (sprite_h<<16)/h
0xA2D0A  src_y = dy/2 (+ sprite_h<<16 if h<0)
0xA2D21  src_x = dx/2 (+ sprite_w<<16 if w<0)
0xA2D3E  if (y < clip.top)    { src_y += (clip.top-y)*dy;  h -= clip.top-y; y = clip.top; }
0xA2D56  if (y + h > clip.bottom) h = clip.bottom - y
0xA2D69  if (x < clip.left)   { src_x += (clip.left-x)*dx; w -= clip.left-x; x = clip.left; }
0xA2D8E  if (x + w > clip.right) w = clip.right - x
0xA2DA8  rowbase = [ [0x131E8] + y*4 ] + x;  stride = [0x131C0]
```

The row writer (`0xA2DF2..0xA2E19`) calls `FUN_000CE9D8(sprite+0x10, 0x5BFB4)`
once per row (`0xA2DD7`), then `FUN_000CEABC(rowbase, (src_y>>16)*sprite_w, w)`
and advances `rowbase += [0x131C0]`, `src_y += dy`. `FUN_000CEABC` (54 insns)
reads the four source **planes** from the pointer table `0x5BFB4`
(`[0x5BFB4+p*4] + src_offset`, `0xCEAE3..0xCEB20`), remaps every byte through
`[0x14720 + byte]` and writes it to `[0x131EC + dest_offset]` unless the remap
yields `0xFF` (`0xCEAF1..0xCEB2D`); the caller's `0xCE9D8` stored
`sprite+0x10` at `[0x14828]` and `0x5BFB4` at `[0x14824]` (`0xCE9D8`, 8
insns). So sprite pixel data is **4-plane planar**, with a per-sprite plane
pointer table and a 256-byte palette-remap/colour-key table; `[0x131EC]` is
the destination write offset inside the VGA window, and the destination byte
write maps through the VGA write mode (planar chain-off dword spans).

## 3. Draw path `0x563C7` → actually `0x563D0` (generic animated object)

The region is not a Ghidra function; hand-decoding `0x56380..0x564BF` gives
three bodies:

* `0x563BC..0x563CC` (17 bytes `8B C0 52 31 D2 89 15 B8 8F 00 00 89 15 BC 8F
  00 00 5A C3`): `[0x8FB8] = 0; [0x8FBC] = 0; RET` — the frame-stepping reset.
* `0x56384..0x563B7`: `RET 4` setup:
  `[0x8FA8] = EAX`, `[0x8FAC] = EDX`, `[0x8FB0] = EBX<<16`,
  `[0x8FB4] = ECX<<16`, `[0x8FBC] = 1`, `[0x8FC0] = [ESP+8]`.
* `0x563D0..0x5649F`: the draw path (referenced by FU-84 as `0x563C7`, which
  is mid-instruction; errata 1).

Draw body: if `[0x8FBC] == 0` (`0x563D8`) return. `EAX = [0x8FA8]`;
`obj = [[0x8F44] + EAX*4]` (`0x563E5/0x563EA`); return if NULL. Frame index:
if `[0x8FB8] == 0` (`0x563F9`) `idx = obj[0]; idx++; obj[0] = idx; if
(obj[idx] < 0) obj[0] = 1;` (`0x56402..0x5640F`) and `[0x8FB8] = 1`
(`0x56415`); else `idx = obj[0]` (`0x5641F`). Then
`desc = 0x8E6C + obj[idx]*3` (`0x56421..0x56433`), `AL = desc[0]`,
`DL = desc[1]`, `BL = desc[2]` (`0x5643B..0x56447`); `ECX = out1 = ESP`,
stack arg `out2 = ESP+4` (`0x56438/0x56443`); `CALL 0x78FAC` (`0x5644C`).
On failure it retries with `EAX=0, EDX=0, EBX=5` and swapped out slots
(`0x56457..0x56467`). It then installs palette `[0x8FAC]` (`0x5646E
MOV EAX,[0x8FAC]; CALL 0x48DC0`) and blits
`FUN_00057080(sprite=ESI, scale=[0x8FC0], ECX=[ESP]=out1,
EDX=0x8FB0, stack=[0x8FC0])` (`0x5647D..0x5648B`), then clears `[0x8FBC]`.

The 3-byte descriptor table at flat `0x108E6C` (read, 64 B) starts
`63 00 05 / 65 0A 04 / 5D 03 05 / 55 01 05 / 55 07 05 / 0C 03 06 / 46 09 05
/ 59 03 05 / 09 02 05 / 15 03 05 / 16 04 05 / 19 06 05 / 1A 02 05 / 67 03 05
/ 6B 03 04 / 1D 00 04 / 3C 02 04 / 62 00 04 / 65 00 04 ...` — i.e.
`{anim_id, frame_index, direction}`. The pointer array `0x8F44` and the
counters at flat `0x108FA8..0x108FC0` are BSS (all zero at rest,
`read_memory 0x108FA0`); the setup/reset functions have no static callers in
this Ghidra database, and the only code caller of the draw path is `0x55B4E`
(`get_xrefs_to`/`search_instructions` `563d0`), inside an unnamed selected-
entity overlay that formats text via `0x99E9F` (format at `0x1DA8`) and reads
the slot selector `[0x8E04]` (`0x5592C..0x55956`). The main match entity path
is §2, not this one.

## 4. Render staging and the projection chain (as evidenced)

`FUN_00036C70` (`0x36C70`, 231 insns; called per frame at `0x49FF5` and
`0x49528`) stages everything the renderer reads:

```
0x36C76  src = ([0x57A6F] != 0 && 0x63FD0() == 0) ? [0x57A6F] : 0x5774C
0x36C9F  [0x55AA4..0x55AAC] = src triple (3 MOVSD)
0x36CA8  if (|X| > 0x1770 || |Z| > 0x1770) { X = Z = -0xFA0; Y = 0; }
0x36CE6  if (Y < 0 || Y > 0x3E80) Y = 0
0x36D1A  for i in 0..10 (team [0x57ABE], records 0x588A4 + i*0xB2):
             [0x55AB0 + i*0xC .. +8] = [rec+0x59..0x61]
             [0x55BC4 + i*4] = ((0x400 - ([rec+0x7B]>>16)) & 0x3FF) << 6
             [0x55C20 + i] = byte[[[rec+0x28]]]
             [0x55C37 + i] = byte[rec+0x3D]
             if (byte[rec+0x9A]) [0x55AB4 + i*0xC] = -10000
0x36D7D  same for i in 11..21 (team [0x57ABF]; 23 slots total)
0x36E12  slot 22 = ball: position 0x5880C, angle from [0x5885A],
             anim = byte[[0x58866]], frame = byte[0x5887B]
```

So the "world position" is staged **unprojected** at `0x55AB0` (three dwords
each), the direction is a 16-bit angle (`0x55BC4`), the row id byte and frame
index are at `0x55C20`/`0x55C37`, and hiding is `Y = -10000`
(`0x55AB4`). Camera height `[0x57750]` and zoom `[0x8DDC]` are only used for
camera/other math in this chain (leg 3). `FUN_0006408C` (`0x6408C`, 131 insns)
then snapshots the whole staging block into a 0x96-entry history ring at
`[0x9AB4]` (stride 0x5D0), not needed for drawing.

The drawable tables are allocated once by `FUN_00058E00` (`0x58E00`, 116
insns): `FUN_0004AFB8(0x3F,0x20,0x2094)` and `FUN_0004A448(0x1E6C)`, then
`0x54348/0x5434C/0x54358/0x54364/0x54388/0x543A8/0x543BC/...` are set to the
two blocks at fixed offsets (`+0x50`, `+0x78`, `+0x780`, `+0xBF0`, `+0xB38`,
`+0x11E8`, `+0x10C8`, ...) (`0x58E41..0x59011`). It is not a per-frame
function.

Per frame `FUN_00058A44` (`0x58A44`, 41 insns) builds the view:

```
0x58A4D  EAX = obj+0xC; CALL 0x4C4B4(EAX, out=0x54324)   ; rotation matrix
0x58A5F  [0x54354] = [obj+4]
0x58A69  EAX = [0x9A70] (camera vector ptr); CALL 0x56E50
0x58A73  FUN_00062828(matrix=0x54324, gate=[camera+8]>0)
0x58A8E  FUN_000590B0(EAX, EDX=obj, ECX=6); FUN_000589E0; copy to 0x5426C/0x54284
```

`FUN_0004C4B4` (23 insns) composes two rotations
(`FUN_0004C3B0(out, -[ptr+4])`, `FUN_0004C414(out, [ptr])`) with
`FUN_000A2991` (matrix multiply) into `0x54324`. `FUN_00056E50` (`0x56E50`,
117 insns) copies the 23 staged triples `0x55AB0 → 0x543A8` and `→ 0x543BC`
(0x114 bytes, `0xA2C50`'s clip reader), then calls `FUN_000A3110` (29 insns,
`out[i] = a[i] + b[i]` dword triples) with the **negated camera vector** so
`0x543A8 + i*0xC` becomes camera-relative; then per slot it adds
`-camera.x` again and `-camera.y + (i%6 + 0x70)` to `0x543BC` (`0x56EDB..0x56F0D`;
`0x4C140` = `i%6 + 0x70`), calls `FUN_00058870`, `FUN_000A3090(0x3A, 0x543BC,
…)` and fills a quad at `0x543B0` from the vector. The matrix consumer
`FUN_00062828` and `0x590B0`/`0x589E0` are cited only (leg 1), and the
per-frame writers of the 16.16 screen positions `0x54364`/`0x54388` are
reached through computed pointers (leg 2). The initialisation
`FUN_00056CF4` (`0x56CF4`, called by the match reset `FUN_0004A228`) sets
`[0x4EAE8] = 0x18`, `[0x4E6F0 + i*4] = i`, fills `0x4E9B8 + i*4 = i%6 + 0x70`
and `0x4EA10 + i*4` with a 0..6 random scaled by `0x5D1` (`0x56D60..0x56E09`,
bounded), and copies `[0x543B0]` five times.

## 5. Canvas and presentation

The match destination is the VGA planar window described by the global
surface-state block at `0x131C0..0x131EF`, not a heap canvas:

* `FUN_000CE6F0` (9 insns) copies **12 dwords from a caller struct into
  `0x131C0`** (`REP MOVSD EDI=0x131c0, ECX=0xC`), and `FUN_000CD758` (13
  insns) copies **24 dwords from `0x131C0` back out** (`ECX=0x18`), so the
  block is the current page/surface state. `[0x131C0]` is the row stride used
  by the span writer, `[0x131C8..0x131D4]` the clip rectangle (the same
  globals FU-56 derived), `[0x131E8]` the y-indexed scanline base table, and
  `[0x131EC]` the destination write offset (`0xA2D3E`, `0xCEAD0`).
* The sprite source planes are pointed by the scratch table `0x5BFB4`
  (`0xA2DCA`, `0xA3B37`, `0xA45E7`, `0xB3BB7`, …), filled by the sprite
  blitters (`FUN_0009B0D0`/`0x9B490`/`0x9B850`/`0x9BC10` push `0x5BFB4`), and
  `0x14720` is a 256-byte remap with `0xFF` = transparent (`0xCEABC`);
  `FUN_000CE980` copies 0x40 dwords from its argument into `0x14720`
  (`0xCE985..0xCE992`, called by the palette installer `0x48DC0`) and
  `FUN_000CE998` copies it back out (`0xCE9A0..0xCE9AA`), so the remap table is
  the current palette's translation buffer.
* `FUN_00048DC0` installs the entity palette from the pointer table
  `0x4BF60` (read failure is expected: the table is runtime-populated).

The movie-player globals are **not** part of the match path:
`search_instructions 563cc/560c0` shows `[0x563CC]` and `0x560C0` used only by
`FUN_000679F4`, `vgt_stream_poll`, `FUN_00068108`, `FUN_00068194` and
`FUN_0006847C` (FU-55 §1/§5), and `FUN_000AE7F0` (FU-56) is called only by
`FUN_00068194`. So the match composite writes VGA directly via the
`0x131C0` state block; presentation is the page-state install
(`0xCE6F0`), not the FU-56 blit (errata 4). The `0x563xx` surface the brief
hypothesised is object-1 code, not a canvas.

## 6. Port: `fifa96_render`

`include/fifa96_loader/fifa96_render.h` +
`src/fifa96_loader/fifa96_render.c` (caller-owned state, no globals,
`-fifa96_err_t` for invalid arguments, no comments). The port models the
resolver over caller-owned banks and the staging/clip math; it does not load
resources or touch VGA.

| original | port |
|---|---|
| `FUN_00036C70` camera copy + `±0x1770` reset to `-0xFA0`/Y=0 + Y `[0,0x3E80]` (`0x36CA8..0x36CFA`) | `fifa96_render_camera_stage` |
| `FUN_00036C70` slot staging: pos copy, `((0x400-heading>>16)&0x3FF)<<6`, anim/frame bytes, hidden `Y=-10000` (`0x36D1A..0x36E47`) | `fifa96_render_slot_stage` |
| `0xA1918`/`0xA1920` count + `handle + [handle+off*8+0x14]` | `fifa96_render_bank` + `fifa96_render_bank_at` |
| `FUN_00078FAC` entry gate, row/frame read, compare tree, all seven handler folds, composite selectors (`0x35/37/39/3B` pair+`0x10F2E7`, `0x40/42` pair, `0x60/61` fixed), tail resolution | `fifa96_render_resolve` |
| `FUN_000A2E24` pivot placement with signed scales (`0xA2E56..0xA2EDE`) | `fifa96_render_place` |
| `FUN_000A2C50` clip pre-pass + 16.16 stepping (`0xA2C7E..0xA2DA8`) | `fifa96_render_cover_rect` |
| `FUN_00057080`, `FUN_000A2C50` span writer `0xCEABC`, `0x14720` remap, `[0x131C0..]` window, `FUN_00078FAC` `0x57DE8`/`0x584D8`/`0x584F0` storage, lazy loader, palette `0x48DC0`, matrix path, history ring | not ported (globals/resources/hardware; open legs) |

Divergences (documented): the original has no bank-count guard (fixed 0x6F
row table, 0x1E animator slots) — the port rejects an out-of-range
`bank_index`/pair index with `-FIFA96_ERR_INVALID`; the original reads a
negative frame byte as `frames[-5..]` — the port rejects a frame index whose
low byte is negative (the driver's frames are `0..last_frame`); the port
treats a NULL bank base/offsets as an unloaded handle (original lazily loads
`0x78DAC`) and reports `sprite`/`overlay` NULL; `fifa96_render_place` errors
on zero sprite dimensions where the original divides by zero.

## 7. Tests (`tests/test_render.c`, suite 73 → 74)

* Layout `_Static_assert`s (`fifa96_render_entity`/`slot`/`cover`).
* `camera_stage`: copy, exact `±0x1770` bounds, both reset branches, Y bounds,
  `INT32_MIN`, NULLs.
* `slot_stage`: position/anim/frame copy, angle fold at heading words
  `0/1/0x400/0x401/0x7FF`, negative dwords (`-1`, `0xFFFF0000`), hidden marker.
* `bank_at`: entries 0/3, negative index, count boundary, NULL base/bank.
* `resolve` errors: id `0x6F`/`0x7F` (signed gate) return no sprite, id `0x80`
  reaches the default fold, NULL args, bank index 3 of 3, negative/low-byte
  frame.
* `resolve` default: offsets and mirror for dirs 0..7, frame truncation
  `0x102 → index 2`, `0x1A` mirror inversion, fixed `0x60`/`0x61` overlays
  (and the wrong-fixed-record rejection).
* `resolve` flat list: full 0..7 direction map incl. the `dir>=3` bank bump
  and `8-dir` flip; `0x2F`; default `0x00`.
* `resolve` pair classes: `0x34..0x37` (flip, bump, `0x35`/`0x37` overlay with
  `0x10F2E7` remap and the zero-byte fallback), `0x38..0x3B` (`0x39`),
  `0x3F..0x42` (`0x40`/`0x41`/`0x42`), `0x3D/0x3E`, `0x1B`, `0x3C` default.
* `place`: positive scale pivot subtraction, negative scale
  `sprite-pivot` addition, zero destination size, zero sprite dims/NULL.
* `cover_rect`: inside, left/top/right/bottom clips with source stepping,
  mirrored x/y source origin (`sprite<<16 - step/2`), off-canvas and
  zero-size invisibility, `x+aw == left` boundary, NULL/zero-dim errors,
  out-zeroing on invisible.
* `make test`: 73/73 before, **74/74 after**;
  `cc -fsanitize=address,undefined -Wall -Wextra -Werror -Iinclude
  tests/test_render.c src/fifa96_loader/fifa96_render.c` runs clean.

## 8. Errata (quoted)

* FU-84 §7 / brief "the draw path `0x563C7` (function at `0x563C7`)" —
  **corrected**: `0x563C7` is the middle of the 6-byte store
  `MOV [0x108FBC], EDX` (`89 15 BC 8F 00 00` at `0x563C5`) inside the
  two-store reset function at `0x563BC`; the draw path's entry is **`0x563D0`**
  (the only code caller is `0x55B4E`, `search_instructions` operand `563d0`).
  FU-84's call-site addresses `0x5644C`/`0x56467` are confirmed.
* FU-84 §7 "if `[ESI+0x14] != 0` CALL `0x78DAC`" — **corrected**: the loader
  is called when `[rec+0x14] == 0` (`0x79111 CMP [ESI+0x14],0; JNZ skip;
  CALL 0x78DAC`), i.e. lazily when the handle is missing.
* FU-84 §7 / open leg 1 "row `+8` sprite-bank semantics" — **extended**: it is
  the index (`*0x18`) into the `0x57DE8` animator records; `FUN_00078F1C`
  fills those records with `+0 = index`, `+0xC`/`+0x10` from
  `0x9E890`/`0xA275C` and `+0x14 = (+0xC ? 0 : +0x10)`, and `+4`/`0x14` are
  finalised by the loader (`0x78DAC`/`0x9E860`).
* FU-84 open leg 3 "sprite-data layout behind frame `+4`" — **closed**: the
  offset `animator[+4]*dir + sprite` selects an entry of a segmented resource
  handle (`[handle+8]` count, `[handle+off*8+0x14]` displacement); the data
  is planar, referenced by the `0x5BFB4` plane pointers and remapped through
  `0x14720` with `0xFF` transparent (§2).
* FU-84 open leg 8 "`FUN_00078FAC` composite resolution ... handler blocks ...
  cited by address only" — **closed**: §1.
* FU-84 §3.1 "row table `0x10EF00..0x10F2E6`" — **confirmed**; the byte at
  `0x10F2E7` starts the composite mirror table quoted in §1.3.
* Brief "the match canvas is likely `ctx[10]`/`ctx[0xB]` surfaces (FU-55) or
  the `0x563xx` surface area ... whether the FU-56 blit is the final step" —
  **corrected**: `[0x563CC]`, `ctx[10]` and `0x560C0` belong to the VGT movie
  player (`search_instructions`), `0x563xx` is object-1 code, and `FUN_000AE7F0`
  has no match caller; the match canvas is the VGA surface-state block
  `0x131C0..0x131EF` written through `0xCEABC`'s planar spans and presented by
  installing a new state block (`FUN_000CE6F0`).
* Brief "record `+0x59/5D/61` projection (camera/height math)" — **reframed**:
  the positions are staged unprojected at `0x55AB0`; the evidenced transform is
  the camera-relative translation of `FUN_00056E50`/`FUN_000A3110` plus the
  per-slot Y addend `i%6+0x70`; the rotation matrix `0x54324` and the 16.16
  screen positions `0x54364/0x54388` feed the blitter but their per-frame
  writers are not located (open legs 1/2). No perspective divide was found
  (leg 3).
* FU-84 §5 "`FUN_0008E008` ... row+8 bank" — unchanged; the driver's frame
  index feeds `0x55C37` (`FUN_00036C70 0x36D52`) and the resolver re-reads it.

## 9. Open legs

1. **Rotation application**: `0x4C3B0`/`0x4C414`/`0xA2991` (matrix build) and
   the consumers `FUN_00062828`/`FUN_000590B0`/`FUN_000589E0` are cited, not
   decomposed.
2. **`0x54364`/`0x54388` per-frame writers** (16.16 screen position and
   size reference) are reached through computed pointers; not located.
3. **Camera height `[0x57750]` / zoom `[0x8DDC]`** role in the entity
   projection; no perspective divide was found in the traced chain.
4. **Resource loading**: `FUN_00098C38`/`0x99E9F`/`0x9E860`/`0x9E890`/
   `0xA275C` and the 4-plane sprite layout behind `0x5BFB4` are cited only.
5. **`0x5BFB4` writers** (`FUN_0009B0D0` family) and the sprite-plane
   geometry (`sprite+0x10` layout, `+6/+8` pivots) are not derived.
6. **`0x14720`** producer (`FUN_000CE980`/`0xCE998`) and the exact
   transparency/palette semantics are not derived.
7. **`0x10F2E7`** value semantics (runs of 5 non-zero, 3 zero) beyond the
   overlay index lookup.
8. **`0x584D8`/`0x584F0` fixed records**: BSS zeros at rest; the runtime
   populator and the gate writers `[0x584EC]`/`[0x58504]` are not found.
9. **`FUN_00048DC0`** palette table `0x4BF60` and the `[0x68E0]` mode branch.
10. **`0x55900` window**: the selected-entity overlay's inputs (`[0x8E04]`,
    `0x546F8`, format `0x1DA8`) and its per-frame caller are not derived.
11. **Ball staging sources** `0x5880C/0x5885A/0x58866/0x5887B` writers.
12. **Draw-list ordering**: writers of `0x4E7B0` and the `0x4E9B8`/`0x4EA10`
    jitter consumers beyond `FUN_00056E50`/`FUN_00056CF4`.
13. **`FUN_000565BC`/`FUN_000560C8`/`FUN_0005619C`** overlay panels: mapped
    at call level (FU-69), not decomposed here.
14. **Page install/flip**: who calls `FUN_000CE6F0` with the match page
    states, and the vsync path.
15. **`FUN_00057080` alternate rasterizer `FUN_0005E1A4`** and the
    `0x44BE0`/`0x445A4` gates.
16. **FU-84 leg 10** (`/tmp/opencode/fifa96_le.bin` differs from the Ghidra
    session image): carried; all data quotes above are from Ghidra memory.

## Provenance

Ghidra MCP on `/fifa96_le.bin`: `get_current_program_info`;
`disassemble_function` 0x78FAC, 0x78F1C, 0x78DAC, 0x78D5C (xref),
0x57158, 0x57080, 0x56FA4, 0x56E50, 0x56CF4, 0x546F8, 0x54640 (xref),
0x54AE4, 0x55B4E window, 0x58E00, 0x58A44, 0x4C4B4, 0x4C140, 0xA2E24,
0xA2C50, 0xA2A10 window, 0xCEABC, 0xCE9D8, 0xCE6F0, 0xCD758, 0x36C70,
0x6408C, 0xA3110, 0xA1918, 0xA1920;
`disassemble_bytes` 0x56380 (320 B), 0x55900 (640 B), 0x56D8A (198 B),
0xA2A10 (192 B);
`read_memory` 0x56380, 0x10F2E0, 0x108E6C, 0x108FA0, 0x1584D8, 0x1584F0;
`get_xrefs_to` 0x56384, 0x563D0, 0x563BC, 0x57158, 0x55BC4, 0x54364,
0x54388, 0x543A8, 0x55C20, 0x55C37, 0x131E8, 0x131C0;
`get_function_callers` 0x57158, 0x56FA4, 0x58E00, 0x56E50, 0x56CF4;
`search_instructions` operands `8fa8`, `8fbc`, `563d0`, `8fac`, `8fb0`,
`131e8`, `131c0`, `4e6f0`, `4e7b0`, `55ab0`, `5bfb4`, `14720`, `563cc`,
`560c0`, `56384`, `563bc`, `563d0`.
Analysis-only outside the port: no tool, capture-rig, ISO or Ghidra-project
change. Port write set: `include/fifa96_loader/fifa96_render.h`,
`src/fifa96_loader/fifa96_render.c`, `tests/test_render.c`, `CMakeLists.txt`
(one library/test block). `make test`: 73/73 before, **74/74 after**;
ASan+UBSan `test_render` clean. `game/FIFAPCCD96.iso` untouched;
`fifa96.rep/**` churn not staged.
