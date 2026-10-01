# FU-5: the game's VGT decoder (protected-mode image)

Static analysis of `/fifa96_le.bin` (see `FU4_le_image.md`). All addresses are
link-time flat addresses; the running game adds the `0x2D1000` relocation
delta. Ghidra names set in the saved program:

| address | name |
| --- | --- |
| `0x67BA8` | `vgt_stream_poll` |
| `0xAE4BC` | `vgt_dispatch` |
| `0xADEFC` | `vgt_decode_f` |
| `0xAE218` | `vgt_decode_k` |
| `0xBABE0` | `file_open_ro` |

## Entry chain

`vgt_stream_poll` (0x67BA8) reads a tag dword from a stream buffer, accepts
`eVGT`..`kVGT` (`0x54475665` < tag <= `0x5447566B`), and for `kVGT` first
copies `0x300` bytes (768-byte palette) to `0x560C0`. It then calls
`vgt_dispatch` (0xAE4BC), which routes:

* `fVGT` -> `vgt_decode_f` (0xADEFC)
* `kVGT` -> `vgt_decode_k` (0xAE218)
* anything else -> error `FUN_000cbbe8(&DAT_00004210)`

Both decoders write the decoded size to `ctx+0x28` and return a success flag.

## Shared header

Both read four **big-endian u16** fields (compiled as `dword >> 16`):

| offset | fVGT | kVGT |
| --- | --- | --- |
| +8 | width | width |
| +10 | height | height |
| +12 | block count | (copied to `ctx[2]`) |
| +14 | row count | palette entry count |

Payload starts at +0x14.

## fVGT path (0xADEFC)

1. `ctx[0xf]` = width*8 and `ctx[0x10]` = width*4 scratch, `ctx[0xe]` =
   (block_count+row_count)*16 decoded-block buffer; `FUN_000add60` builds a
   row-offset table at `ctx[0xd]`.
2. The payload begins with a **bit-packed index table**, width entries of
   20 bits each, padded to a dword boundary:
   `(width*0x14 + 0x1f) & ~0x1f) >> 3` bytes.
3. A loop expands it: `ctx[0x10][i] = ctx[0xf][i] + rowtab[ctx[0xf][i+1]] + 4*plane`.
4. `block_count*16` bytes of raw 16-byte blocks are copied to `ctx[0xe]`.
5. `FUN_000addf0` expands each **8-byte source record to a 16-byte block**:
   the first 4 bytes are a 4-colour palette, the next 4 bytes are 16 x 2-bit
   indices (`out[k] = palette[(indices >> (30-2k)) & 3]`).
6. `FUN_000ba8f0(canvas, rowdata, stride, row_count)` decodes the row stream
   and `FUN_000ba994(...)` composites with the index table
   (`ctx[4]`=width, `ctx[0x10]`, `ctx[0xe]`, `*ctx`=pitch, `ctx[1]`=height).

## kVGT path (0xAE218)

1. Palette: `count` RGB triples at payload+0x14 are copied to `ctx+0x44`.
2. Scratch: two 16-byte-header buffers (`ctx[10]`, `ctx[0xb]`) whose header is
   byte `0x7B` plus the BE16 width/height at +4/+6; a row-offset table
   `ctx[0xd][i] = -width*height + i*width`; and a block buffer `ctx[0xc]`.
3. `FUN_0009e860(payload+0x14+count*3, ctx[10]+0x10)` ->
   `FUN_0009e718(..., 1)`, the command-stream decoder.

## kVGT command stream (0x9E718)

The stream is command bytes with a marker: `stream[1]` must be `0xFB`, and the
command is `stream[0] & 0xFE`:

| command | handler |
| --- | --- |
| `0x10`, `0x11` | `FUN_000b18f8` |
| `0x16` | `FUN_0009e1e4` |
| `0x32`..`0x34` | `FUN_0009c7e0` |
| `0x46` | `FUN_0009da14` |
| `0x60`, `0x62`, `0x66`, `0x6E`, `0x70`, `0x72` | `FUN_0009dc1c` |
| `0x6A` | literal copy (`FUN_000cd390`), 24-bit **big-endian** length from bytes [2..4] |
| `0x7A`, `0x7B` | `FUN_0009dc94` |
| other | error `FUN_000cbbe8(&DAT_000033dc, cmd)` |

So kVGT is a command/run-stream codec, while fVGT is a block+index codec with
4-colour 2bpp blocks.

## Relation to the repo reconstruction

`src/fifa96_loader/fifa96_tgv.c` checks the kVGT magic and reads a u32 at +4
only. The runtime decoder adds the BE16 fields at +8..+15, the optional
palette, and the kVGT command stream above; for fVGT the block/index layout
and 4-colour expansion are the parts a behavioural parser still lacks. The
FU-1 note that this tag is "not statically locatable" is superseded by these
anchors.
