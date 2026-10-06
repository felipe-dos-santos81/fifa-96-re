# FU-98 — kit palette remap tables, the shade-cube builder, and the 5/12 note

Follow-on to FU-91 §3/§9 (the palette shuffle tables and `FUN_000A0AA0`).
Both are now derived: the shuffle tables are **static data** (my earlier
zero read was the unrelocated operand address), and `FUN_000A0AA0` is an
RGB→index shade-cube builder, not the DAC stage. The remap is ported as
`fifa96_sprite_palette_kit_remap`.

Result in one line: **`pal[dst[i]] = pal[src[i]]` with
`dst = {132,135,140,143,146,150,153,158,161,164}` and `src = {156..165}`,
read from the static tables at image `0x1070E8`/`0x1070F2`; the palette
install then feeds a shade cube (`FUN_000A0AA0`) built over the 1024-byte
BGRA table.**

## 1. The kit remap tables

`FUN_00048B60` (`0x48B60`) and `FUN_00048ED8` (`0x48ED8`) read two 10-byte
tables at the relocated addresses (operand + object-4 base `0x100000`):

```
image 0x1070E8 (operand 0x70E8): 132, 135, 140, 143, 146, 150, 153, 158, 161, 164
image 0x1070F2 (operand 0x70F2): 156, 157, 158, 159, 160, 161, 162, 163, 164, 165
```

`get_xrefs_to 0x70E8`/`0x70F2` returns only the two readers — there is **no
writer**, and the storage is non-zero statically, so FU-91's
"runtime-populated" open leg is closed: they are link-time data.

The loop (decompiled `FUN_00048B60`, `0x48BA9..0x48BC8`):

```
for i in 0..9:
  pbVar1 = (&0x70F2)[i] ; pbVar2 = (&0x70E8)[i]
  memmove(palette + pbVar2*3, copy + pbVar1*3, 3)
```

so with `dst = 0x70E8` and `src = 0x70F2`:

```
palette[dst[i]] = snapshot[src[i]]      ; 10 RGB triples
```

The original reads the source from a stack copy (snapshot semantics). In
place the result is identical because every destination index that is also a
source (158, 161, 164) is consumed before it is written:
158 as `src[2]` → dst 140, written as `dst[7]`; 161 as `src[5]` → dst 150,
written as `dst[8]`; 164 as `src[8]` → dst 161, written as `dst[9]`.

`FUN_00048ED8` applies the same remap to the shaded copy at `0x4B500` (FU-91
§3): `[0x4B500]` entries `dst[i]` receive `[0x4B200]` entries `src[i]` — the
team-kit hue relocation performed once per palette install, after which
`[0x68E0] = 1` enables the `FUN_00048DC0` kit translation.

## 2. `FUN_000A0AA0` — the shade cube (not the DAC)

`FUN_000A0AA0(param_1 = cube, param_2 = 1024-byte BGRA table from
FUN_000A1368, param_3 = offset)` with `BL` = bits per channel (FU-91 §3's
install path `FUN_000a154c` → `FUN_000a129c` → here):

```
local_10 = 8 - BL
[0x5BB38] = 1 << BL          ; levels per channel
[0x5BB34] = 1 << local_10    ; channel mask step
[0x5BB3C] = 1 << (2*local_10)
[0x5BB50] = levels^2         ; plane stride
[0x5BB4C] = levels
fill (levels^3) dwords of the cube with 0xFFFFFFFF
for i in 0..count-1:
  if ([0x1447C + i] == 0) continue
  src = param_2 + i*4                       ; {B,G,R,A}
  per channel n = src[2-n] >> local_10      ; quantised level
  frac = channel - (level * step + step/2)  ; half-step offset
  [0x5BB68] = fracB^2 + fracG^2 + fracR^2   ; squared distance
  [0x5BB54/5C/58] = 2*((level+1)*2^(2*local_10) - channel*step)
  cube cell = param_1 + lvlR*levels^2 + lvlG*levels + lvlB
  FUN_000A0CB8(levels, cube + cell + param_3)
```

So it is an **RGB→palette-index quantization cube** (validity flags
`0x1447C`, per-cell search via `FUN_000A0CB8`), sized `2^B` per channel —
the sprite blender's colour lookup, not a DAC upload. The earlier open leg
"`FUN_000A0AA0` internals (the final DAC/shade stage)" is closed as
"shade cube; `FUN_000A0CB8` internals remain open".

## 3. Port

`fifa96_sprite_palette_kit_remap(rgb6, count)` implements §1 exactly
(sequential in-place loop, `count >= 166` required);
`tests/test_sprite_palette.c::test_palette_kit_remap` pins all ten moves,
the consumed-before-written cases (158/161/164), the untouched neighbours
and the argument errors. API: `include/fifa96_loader/fifa96_sprite.h`.

## 4. Note — the `5/12` factor (FU-96 §6 leg 2)

`ratio2/ratio1 = (5·dim/12)/(dim/2) = 5/6`. Two readings fit the constant
(not decided statically): the 320×200→4:3 pixel aspect
(`(4/3)/(320/200) = 5/6`) or the 320×240-page vs 200-line viewport height
ratio (`200/240 = 5/6`). Either way the second numerator is the first scaled
by `5/6`; the port computes it exactly as `floor(5·dim/12)`.

## 5. Provenance

Ghidra MCP on `/fifa96_le.bin`: `read_memory` 0x1070E8/0x1070F2 (the two
10-byte tables); `get_xrefs_to` 0x70E8/0x70F2 (readers only);
`decompile_function` 0x48B60, 0x48ED8, 0xA0AA0; `decompile_function`
0x44BE0 (tail-calls the settings getter `FUN_0001D940` — FU-93 leg 2 closed
as degenerate). Port write set: `include/fifa96_loader/fifa96_sprite.h`,
`src/fifa96_loader/fifa96_sprite.c`, `tests/test_sprite_palette.c`.
Analysis-only otherwise.

## 6. Open legs

1. **`FUN_000A0CB8`**: the cube-cell search (which index wins, the
   `[0x5BB*]` scratch layout) is cited only.
2. **`0x1447C + i`**: the palette-validity flags' producer.
3. **Chunk `+6/+8/+0xA`** (FU-91 §9) still unread by the derived consumers.
4. **Bits per channel `BL`**: who calls the install path with which `BL`
   (`FUN_000A129C`'s caller chain).
