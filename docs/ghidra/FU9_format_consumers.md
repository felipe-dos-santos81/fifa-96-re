# FU-9: format consumers in the protected-mode image

Callers of the resource layer are not in any static pointer table (the game
builds its dispatch tables at runtime), but a direct `E8` scan over the flat
image finds them. All consumers share the same primitives:

* `FUN_0009a45c(w, h, 0x20)` — allocate an aligned surface
* `FUN_0009e860(src, dst)` / `FUN_0009e718` — VGT decompressor (FU-5)
* `FUN_000993ec` / `FUN_0009a16c` — free
* `FUN_0009e890` — raw-vs-compressed probe

## Identified consumers

| function | behaviour | format |
| --- | --- | --- |
| `FUN_00014c18` | allocates `0x280 x 0x1E0` (640x480), decompresses `buf+0x10` into `surface+0x2c+0x10`, then passes BE16 fields at `+8`/`+10` | VGT/TGV still image |
| `FUN_00023b38` | table `PTR_DAT_00047C30`, entry `i` (stride 4); surface cached per slot keyed by `2*size+0x1a+ebx`; decompresses `entry+0x10`; dims BE16 at `+2`/`+4` | table-driven frames (VIV) |
| `FUN_00078dac` | ring of 30 entries; builds names via `FUN_00099e9f(&DAT_00002CAC, idx)`; allocates `2*size`; decompresses; if source byte bit0 set re-decodes into a temp and back; stores pointers at `&DAT_00057D70` | streamed frames (ANM/BNK-style) |
| `FUN_0004a2e0` | generic: `FUN_00068FD0` get source, probe `FUN_0009e890`, allocate by decoded size, `FUN_0009e860` decompress, free source | generic compressed asset |
| `FUN_0004a344` | same shape, called from `0x4A6D7`/`0x4AAF1` | generic compressed asset |

The decompress call sites (`FUN_0009e860`): `0x14C65`, `0x14D76`, `0x18CE8`,
`0x23C0C`, `0x24DE6`, `0x26E47`, `0x4A32A`, `0x4A376`, `0x4A42B`, `0x4A5DC`,
`0x4B000`, `0x78E5E` — these are the per-format entry points.

## Cross-check with the repo reconstruction

The shipped parsers (`src/fifa96_loader/`) model the same shapes: shared
VGT decompression, a `+0x10` header skip, BE16 dimensions, and per-entry
tables. The runtime confirms them and supplies the missing entry addresses.
POG/VIV/QFS containers are parsed after the generic load/decompress; their
consumers are the functions above (and their callers), reached via runtime
dispatch rather than static tables.

## Screen/data-layer consumers (second pass)

* `FUN_00018c90` — generic load: `FUN_00068FD0` source, probe, allocate,
  `FUN_0009e860` decompress, free source; with `param_2 != 0` it copies the
  decoded buffer and frees the original (compacting).
* `FUN_00024b00` — screen setup (mode 1/2/3): configures the layout globals
  (`0x5470`/`0x5474`/`0x5478`/`0x547c`), calls `FUN_000ce70c` with
  `PTR_DAT_00047C30 + (*(int*)PTR_DAT_00047C30 >> 8) + 0x10` (these entries
  carry a big-endian length in the top byte), and decodes
  `PTR_DAT_00047C30 + 0x10` into a surface with `FUN_0009e860`.
* The same table `PTR_DAT_00047C30` drives `FUN_00023b38`, so it is the
  **shared screen/frame asset table**: `entry[0]` is a header whose >>8 gives
  the offset to the first frame, each entry has BE16 dimensions at `+2`/`+4`
  and the compressed payload at `+0x10`.
