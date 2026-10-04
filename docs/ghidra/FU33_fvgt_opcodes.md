# FU-33: fVGT opcode map for `vgt_decode_f` (0xADEFC)

Static transcription of the fVGT delta-frame pipeline in
`/fifa96_le.bin` (FU-4 linear image, link-time flat addresses), derived
instruction-by-instruction from Ghidra decompilation and disassembly. It
closes FU-19's fVGT open legs (`FU19_vgt_decode_kernels.md` §8.1b/§8.2:
the `ctx[0]`/`ctx[1]`/`ctx[0xd]` producers) and corrects FU-5's fVGT step
list (`FU5_vgt_decoders.md` §"fVGT path"). The spec is validated
byte-exactly: a throwaway simulator replays the committed pre-state plus
the committed input chunk and reproduces the committed post-state
(76800/76800 bytes, matching SHA-256). No external codec documentation
was used.

Golden pair (FU-31/FU-32, `tests/golden/vgt/fvgt-01.{in,pre,out}.bin`,
`fvgt-01.json`): chunk tag `fVGT`, declared length 9972, decoder fields
`(239, 5, 409, 10)`, 320x240 canvas.

## 1. Scope and entry chain

* `vgt_stream_poll` (`0x67BA8`, FU-5) reads a tag dword and, for
  `0x54475665` < tag <= `0x5447566B`, calls `vgt_dispatch`
  (`0x67C5C CALL 0xAE4BC`). `vgt_dispatch` (`0xAE4BC`)
  dispatches tag `0x54475666` (`'fVGT'`) to `vgt_decode_f` and returns
  `ctx[10]` (surface pointer); tag `0x5447566B` (`'kVGT'`) goes to
  `vgt_decode_k` (`0xAE218`). `get_function_callers 0xADEFC` returns
  exactly one caller: `vgt_dispatch`.
* Both decoders receive the same context: `vgt_stream_poll` passes
  `DAT_000563e8` (`0x67C55`/`0x67C78` reads). That pointer is written
  once by `FUN_000679f4` (`0x67A26 WRITE`) from `FUN_000ae490`, which
  allocates and zeroes `0x344` bytes (`decompile 0xAE490`). The context
  therefore persists across the whole VGT stream, and `vgt_decode_k` is
  the writer of `ctx[0]`, `ctx[1]` and `ctx[0xd]` (§4).
* Calling convention: cdecl `(int *ctx, int chunk)`; after
  `PUSH ESI/EDI/EBP` + `SUB ESP,0x28` (`0xADEFC..0xADEFF`) the args are
  `[ESP+0x38]`/`[ESP+0x3c]` (`0xADF02`/`0xADF06`).
* Return value is the local success flag `bVar16`: set false when any
  scratch allocation fails (`0xAE03D`, `0xAE048`, `0xAE09A` write 0 to
  `[ESP+0x14]`), returned at `0xAE20B MOV EAX,[ESP+0x14]`. For the
  captured call it is `true` (all four allocations non-null).
* Entry swap: `0xADF25 MOV EAX,[EBP+0x28]`,
  `0xADF28 MOV EDX,[EBP+0x2c]`, `0xADF2B MOV [EBP+0x2c],EAX`,
  `0xADF31 MOV [EBP+0x28],EDX` — `ctx[10]` (byte `0x28`) and `ctx[0xb]`
  (byte `0x2c`) are exchanged before any decode step (§8).

## 2. Chunk header

| offset | size | field | ctx slot | fvgt-01 |
| --- | --- | --- | --- | --- |
| `+0` | 4 | tag `'fVGT'` (`0x54475666`) | — | fVGT |
| `+4` | 4 | chunk length, LE32 | — | 9972 |
| `+8` | 2 | index count (width) | `ctx[4]` | 239 |
| `+0xA` | 2 | raw 16-byte block count | `ctx[5]` | 5 |
| `+0xC` | 2 | 8-byte palette record count | `ctx[6]` | 409 |
| `+0xE` | 2 | row-stream bits per entry | `ctx[7]` | 10 |
| `+0x10` | 4 | zero, no reader in `vgt_decode_f` | — | 0 |
| `+0x14` | ... | payload (index table, blocks, row stream) | — | |

Field reads are **little-endian u16**, compiled as a dword load at
`field-2` shifted right 16 (Ghidra shows the `+8` read as
`*(uint *)(param_2 + 6) >> 0x10`):

* width: `0xADF2E LEA EAX,[ESI+8]`, `0xADF34 MOV EAX,[ECX+EAX-4]` with
  `ECX=2` → dword at `+6`; `0xADF41 SHR EAX,CL` (CL=0x10) → u16 at `+8`;
  stored `0xADF4E MOV [EBP+0x10],EAX` (`ctx[4]`).
* height/raw: base `+8` (`0xADF54`) → u16 at `+0xA`; `0xADF6C` (`ctx[5]`).
* block count: base `+0xA` (`0xADF72`) → u16 at `+0xC`; `0xADF8A` (`ctx[6]`).
* row bits: base `+0xC` (`0xADF90`) → u16 at `+0xE`; `0xADFA3` (`ctx[7]`).

FU-5's "big-endian u16" and FU-19 §6.1's "BE16" are wrong: the bytes at
`+8` are `ef 00` and the structural equation below only holds with
239/5/409/10, not with the byte-swapped values. The LE16 naming in
FU-31/FU-32 is correct.

FU-31's chunk-size equation is confirmed: `in_len = 0x14 +
((width*20+31)&~31)>>3 + raw*16 + pal*8 +
((bits*(out_len/16)+31)&~31)>>3`, here
`0x14 + 600 + 80 + 3272 + 6000 = 9972`. The simulator asserts the
payload cursor reaches `in_len` exactly.

Scratch allocations (guarded by `ctx[8]`/`ctx[9]` high-water marks):

| slot | size | instructions |
| --- | --- | --- |
| `ctx[0xf]` | `width*8` | `0xADFF4 SHL EAX,3`, `0xADFFD CALL` |
| `ctx[0x10]` | `width*4` | `0xAE01B SHL EAX,2`, `0xAE024 CALL` |
| `ctx[0xe]` | `(raw+pal)*16` | `0xAE050 ADD EDI,[ESP+0xc]`, `0xAE07C SHL EAX,4`, `0xAE085 CALL` |

## 3. Stage 1 — bit-unpacked index table (`unpack_signed_fields` 0xADD60)

Byte count: `0xAE0AD LEA EAX,[EDX*4]` + `0xAE0B4 ADD EAX,EDX` +
`0xAE0B6 SHL EAX,2` = `width*20`; `0xAE0BE ADD EAX,0x1f`,
`0xAE0C4 AND AL,0xe0`, `0xAE0C9 SAR EAX,3` =
`((width*20+31)&~31)>>3` = 600 bytes for width 239 (`0xAE0CE` saves it).

Call setup: `0xAE0C1 ADD ESI,0x14` (payload), `0xAE0B9 MOV ECX,0xA`
(bits), `0xAE0C6 LEA EBX,[EDX+EDX]` (`2*width`), `0xAE0CC MOV EDX,ESI`
(src), `0xAE0D2 MOV EAX,[EBP+0x3c]` (`ctx[0xf]` dst),
`0xAE0D5 CALL 0xADD60`. So it unpacks **`2*width` signed 10-bit fields**
into `ctx[0xf]` (FU-5's "row-offset table at `ctx[0xd]`" is superseded,
as FU-19 §8.1b already noted).

Algorithm (`0xADD60..0xADDED`), register args EAX=dst, EDX=src,
EBX=count, ECX=bits:

```
bit = (count-1)*bits                     ; 0xADD71/0xADD74
for i in count-1 .. 0:                   ; store descending, 0xADDDD/0xADDE0
    v = (LE32(src + (bit>>3)) >> (bit & 7)) & ((1<<bits)-1)   ; 0xADDBB/0xADDC0/0xADDC4
    if v & (1<<(bits-1)): v |= -1<<bits  ; sign-extend, 0xADDD4/0xADDD8
    dst[i] = v
    bit -= bits                          ; 0xADDD2
```

The descending bit cursor plus descending store means the output array is
in **natural stream order**: `dst[i]` is the field at bit offset
`i*bits`, least-significant-bit-first. Each 20-bit table entry `j` is the
pair `a = fields[2j]`, `b = fields[2j+1]`. A dword load at the end can
overrun the table by up to 3 bytes; the overrun bits are masked.

## 4. Stage 2 — row-offset expansion and the `ctx[0xd]` producer

Inline loop `0xAE0F6..0xAE127` (no helper call):

```
base = ctx[0xd] + ctx[1]*4               ; 0xAE0DE/0xAE0E1/0xAE0E8, saved [ESP+0x24]
for j in 0 .. width-1:                   ; bound width*4, 0xAE0F6
    b = fields[2j+1]                     ; 0xAE104 MOV EDX,[EDI+EBX+4]
    a = fields[2j]                       ; 0xAE116 MOV ESI,[EDI+EBX]
    ctx[0x10][j] = a + *(int*)(base + b*4)   ; 0xAE10C/0xAE10F/0xAE114/0xAE11C/0xAE121
```

i.e. `ctx[0x10][j] = a_j + rowtab[ctx[1] + b_j]`. FU-19's
`ctx[0x10][i] = ctx[0xf][i] + ctx[0xd][height + ctx[0xf][i+1]]` is the
same relation with the field array indexed as pairs. There is **no plane
term** in this build (the brief's `+4*plane` variant is absent).

**Producer resolution (FU-19 open leg 2).** `vgt_decode_f` only reads
`ctx[0xd]` (`0xAE0DE`); the writer is `vgt_decode_k`:

* `ctx[0] = LE16(+8)` — `0xAE22E MOV EAX,[ECX+EAX-4]` (base `+6`),
  `0xAE23B SHR EAX,CL`, `0xAE241 MOV [ESI],EAX`.
* `ctx[1] = LE16(+0xA)` — `0xAE24F`, `0xAE262 MOV [ESI+4],EAX`.
* `ctx[0xd] = alloc(height*8)` — `0xAE373 SHL EAX,3`,
  `0xAE380 CALL 0x98BF8`, `0xAE387 MOV [ESI+0x34],EAX`.
* row-table loop `0xAE447..0xAE458`: `0xAE432 NEG EDI`,
  `0xAE438 IMUL EDI,[ESP+0x10]` (width) → `start = -width*height`;
  `0xAE44A MOV [EDX+EAX],EDI`, `0xAE451 ADD EAX,4`,
  `0xAE454 ADD EDI,EDX` (width), `0xAE456 CMP EAX,EBX`,
  `0xAE458 JL`. The decompile bounds the loop at `uVar5 << 3` bytes
  (`height*8`), i.e. **`2*height` dword entries**:
  `rowtab[i] = i*width - width*height`, `i` in `[0, 2*height)`.

Because `vgt_stream_poll` shares one context between the decoders and
`vgt_decode_f` never writes `ctx[0]/[1]/[0xd]`, an fVGT delta frame
inherits the geometry and row table of the last kVGT keyframe. For the
committed vector `rowtab[i] = (i-240)*320`, so
`rowtab[ctx[1]+b] = b*320`; the chunk's `b` values span `[-76, 127]`,
giving row-table indices `[164, 367]`, inside `[0, 480)`.

## 5. Stage 3 — block buffer (raw copy + `expand_palette_block` 0xADDF0)

Payload pointer after the index table: `0xAE129..0xAE131`
(`ADD EDI,EAX` with the saved table byte count).

* **Raw blocks.** `0xAE133 MOV EAX,[ESP+0x10]` (`ctx[5]`),
  `0xAE137 SHL EAX,4` → `raw*16`; `0xAE146 MOV ESI,[ESP+0x20]`,
  `0xAE14A MOV ECX,EAX`, `0xAE14C MOV EDI,[EBP+0x38]` (`ctx[0xe]`),
  `0xAE14F MOVSB.REP`. Copies **field `+0xA`** raw 16-byte records to
  `ctx[0xe]` (not `block_count` as FU-5 step 4 says; FU-19's correction
  stands).
* **Palette records.** `0xAE15B MOV EAX,[ESP+0xc]` (`ctx[6]`),
  `0xAE15F SHL EAX,3` → `pal*8`; `0xAE16E..0xAE185` push
  `(count=ctx[6], dst=ctx[0xe]+raw*16, src=payload after raw blocks)` and
  call `expand_palette_block`.
* `expand_palette_block` (`0xADDF0..0xADEFA`) per 8-byte record:
  `idx = LE32(src+4)` (`0xADE09`), palette bytes `src[0..3]`; for
  `k = 0..15`: `out[k] = src[(idx >> (30-2k)) & 3]`
  (`0xADE0E..0xADEF1`; shift amounts `0x1E,0x1C,...,0x2,0x0`, `AND 3`,
  byte-granular palette lookup). `src += 8` (`0xADED1`),
  `dst += 16` (`0xADEE4`).

`ctx[0xe]` therefore holds `raw + pal = 414` 16-byte blocks; composite
block ids `[width, width+raw+pal)` select record `idx-width`.

## 6. Stage 4 — row stream (`unpack_block_indices` 0xBA8F0)

Call setup: `0xAE19B MOV EAX,[ESP]` (`ctx[0]` pitch),
`0xAE19E IMUL EAX,EDX` (`ctx[1]` height), `0xAE1A5 SAR EAX,4` →
`count = pitch*height/16 = 4800`; `0xAE1A8 IMUL EDX,EAX` with
`EDX=ctx[7]`, `0xAE1AB ADD EDX,0x1f`, `0xAE1AE AND DL,0xe0`,
`0xAE1B1 SAR EDX,3` → row-stream bytes
`((bits*count+31)&~31)>>3 = 6000`. Push order
`0xAE1C0..0xAE1CB`: bits, count, src (`payload + raw*16 + pal*8`), dst
(`ctx[0xc]`).

`unpack_block_indices` (`0xBA8F0..0xBA990`), cdecl
`(dst, src, count, bits)`: `mask = (1<<bits)-1` (`0xBA909..0xBA913`);
main loop while `count > 3` (`0xBA910 SUB EDX,4`, `0xBA916 JL`), four
values per iteration at bit offsets `B, B+bits, B+2*bits, B+3*bits`,
each `ROR(LE32(src + (off>>3)), off&7) & mask` (`SHRD EAX,EAX,CL` at
`0xBA925`, `0xBA931`, `0xBA948`, `0xBA955`; pair-2 base `0xBA939`),
then `B += 4*bits` (`0xBA963`) and `count -= 4` (`0xBA960`); tail
`0xBA96D..0xBA98A` stores the remaining `count` values, `B += bits`.
The stream is **LSB-first, one 10-bit block id per value**, consumed
row-major by the compositor. (For `bits=10` the rotation never wraps a
dword boundary; the `SHRD` form and a plain shift agree.)

## 7. Stage 5 — composite (`composite_4x4_blocks` 0xBA994)

cdecl 9-arg call, pushed right-to-left at `0xAE1D3..0xAE202`:

| arg | source | value | role |
| --- | --- | --- | --- |
| 1 | `ctx[0xc]` | 4800 dwords | block ids |
| 2 | `ctx[0xb]+0x10` | pre surface | source (previous frame) |
| 3 | `ctx[10]+0x10` | post surface | destination |
| 4 | `ctx[4]` | 239 | delta/block id threshold |
| 5 | `ctx[0x10]` | 239 dwords | expanded delta offsets |
| 6 | `ctx[0xe]` | 6624 B | raw + palette-expanded blocks |
| 7 | `ctx[0]` | 320 | pitch |
| 8 | `ctx[0]>>2` | 80 | blocks per row |
| 9 | `ctx[1]>>2` | 60 | row bands |

Body (`0xBA994..0xBAA45`):

* `[EBP-8] = blocks - width*16` (`0xBA9A2..0xBA9AD`),
  `[EBP-0xc] = prev - dst` (`0xBA9B0..0xBA9B6`),
  `ECX = 3*pitch` (`0xBA9BC`).
* Outer loop `0xBA9C5` runs `rows4` times; inner loop `0xBA9D0` runs
  `pitch4` times per band, loading one id per iteration (`ESI` cursor
  `ctx[0xc]`).
* **Delta id** (`id < width`, `0xBA9D5 JGE` taken for blocks):
  `src = ctx[0x10][id] + (prev-dst) + dst_cursor`
  (`0xBA9DD/0xBA9E0/0xBA9E3`), then four dword copies to
  `dst_cursor + {0,pitch,2*pitch,3*pitch}` (`0xBA9E5..0xBA9F8`). With
  `ctx[0x10][id] = a + b*pitch` this reads the previous frame at
  `(row+b, col+a)` for each of the four rows — `a` is a byte
  (pixel-column) displacement, `b` a row displacement.
* **Block id** (`id >= width`, `0xBAA0E`):
  `src = blocks + (id-width)*16` (`0xBA9A2..0xBA9AD` base +
  `0xBAA11 LEA EAX,[EAX*8]`), four dwords copied (`0xBAA1B..0xBAA2E`).
* `dst_cursor += 4` (`0xBA9FB`); column counter
  `[EBP+0x24] -= 1` (`0xBA9FE`/`0xBAA34`); band end
  `0xBAA3A ADD EDI,ECX` (3*pitch) then `[EBP+0x28] -= 1`
  (`0xBAA3C`/`0xBAA40`).

Every one of the 4800 4x4 blocks writes 16 bytes, so the full 76800-byte
canvas is rewritten. There are no bounds checks on `a`/`b` or on
`id-width`.

## 8. Delta / double-buffer semantics

The entry swap makes the post-swap `ctx[0xb]` the **previous** surface
(entry `ctx[10]`) and the post-swap `ctx[10]` the **new** surface (entry
`ctx[0xb]`); this is the FU-31/FU-32 capture contract (`pre` = entry
`ctx[10]+0x10`, `post` = `ctx[10]+0x10` at the return). The compositor
reads `ctx[0xb]+0x10` and writes `ctx[10]+0x10` only (FU-32 §2), so the
pre bytes are intact throughout the call. Delta ids therefore copy from
the pre-state; block ids materialize new pixels. The simulator applies
the pipeline to `pre + in` and overwrites the whole canvas.

## 9. Validation

Throwaway simulator `/tmp/opencode/fu33/fvgt_sim.py` (**not committed**)
implementing §2–§8 exactly. Decisive command and verbatim result:

```
$ python3 /tmp/opencode/fu33/fvgt_sim.py
fields width=239 raw=5 pal=409 bits=10 len=9972
table_bytes=600 fields[0:4]=[0, 0, 119, 1]
blocks=6624B (raw 5 + pal 409)
indices=4800 row_bytes=6000 max=652 delta=4363
oob_reads=0 sample=[]
sha256(in)  =45844f059594a28279f1e07463e8dbbdcc3d89b016365bcb6581d2cfb2a2046d
sha256(pre) =8fdcba20ca2f086e8fe63efb26afc7c58d096884582ddd7a409f393b81db4347
sha256(sim) =4b5c900d6df16f8a5a7534e0c19725a92d816d0b757ba08c7f84120645a87ccd
sha256(ref) =4b5c900d6df16f8a5a7534e0c19725a92d816d0b757ba08c7f84120645a87ccd
byte_exact=True sim_len=76800
return_flag=true (bVar16: all ctx allocations non-null)
$ echo $?
0
```

Result: the simulator's 76800 output bytes are **byte-identical** to
`tests/golden/vgt/fvgt-01.out.bin` and all three SHA-256 values match
`fvgt-01.json` (independently confirmed with `sha256sum`). Coverage of
the composite: 4363 delta references and 437 block references; maximum
id 652 = `width + raw + pal - 1`; zero out-of-range source reads (all
delta reads stayed inside the 76800-byte pre buffer). `make test` is
**28/28** before and after the doc-only change.

## 10. Open legs

1. **Keyframe history is runtime state.** The producers of `ctx[0]`,
   `ctx[1]` and `ctx[0xd]` are located (`vgt_decode_k`, §4), but the
   pair does not record which prior kVGT keyframe installed them; the
   byte-exact replay only proves the values `pitch=320`, `height=240`,
   `rowtab[i]=(i-240)*320` for the indices this chunk touches. Other
   streams could carry different geometry (FU-31's shape census).
2. **No bounds checks.** `rowtab[ctx[1]+b]` assumes
   `b in [-height, height)`; `blocks + (id-width)*16` assumes
   `id-width < raw+pal`; delta offsets `a`/`b` are unconstrained. A
   malformed chunk can read outside `ctx[0xd]`/`ctx[0xe]`/the surfaces.
   The committed vector stays in range (`max=652`, `b in [-76,127]`,
   `oob_reads=0`).
3. **One committed vector.** FU-31/FU-32 captured a single fVGT shape
   `(239,5,409,10)`; the 160x120/other-shape pairs were not committed.
4. **Header bytes `+0x10..+0x13`.** Four zero bytes before the payload
   have no reader in `vgt_decode_f`; their producer/semantics are
   unknown (alignment is plausible but uncited).
5. **Bit-reader over-reads.** Both unpackers load 32-bit dwords at the
   tail and can read up to 3 bytes past their region; the bits are
   masked, but a port must either reproduce or bound these reads.
6. **FU-5 corrections confirmed.** Raw block count is field `+0xA`,
   palette count `+0xC`; `unpack_signed_fields` fills `ctx[0xf]`;
   `vgt_dispatch` returns `ctx[10]`; the composite `width` argument is
   `ctx[4]` (the chunk field), not the canvas pitch.

## 11. Provenance

Ghidra program `/fifa96_le.bin` (Raw Binary, `x86:LE:32:default`, base 0,
bridge 2026-10-04; program already carrying the FU-19/FU-31 names):

* `decompile_function`: `0xADEFC`, `0xADD60`, `0xADDF0`, `0xBA8F0`,
  `0xBA994`, `0xAE218`, `0xAE4BC`, `0x67BA8`, `0xAE490`, `0x679F4`,
  `0x68108`, `0x68194`, `0xAE738`, `0x68B58`.
* `disassemble_function`: `0xADEFC` (259 insns), `0xADD60` (54),
  `0xADDF0` (97), `0xBA8F0` (63), `0xBA994` (66), `0xAE218` (197).
* `get_function_callers 0xADEFC` (only `vgt_dispatch`),
  `get_function_callers 0xAE490` (only `FUN_000679f4`),
  `get_xrefs_to 0x563e8` (1 write + 3 reads, no other writer).
* `search_instructions` for `+0x34` stores and `0x563e8` references
  (writer attribution).
* Shell: `sha256sum` on the three golden files;
  `python3 /tmp/opencode/fu33/fvgt_sim.py`; `make test` (28/28).

Self-review: every algorithmic claim above cites the listed tool output.
The decisive addresses (field reads `0xADF34..0xADFA3`, the
`unpack_signed_fields` call `0xAE0C6..0xAE0D5`, the expansion loop
`0xAE0F6..0xAE127`, the `expand_palette_block`/`unpack_block_indices`/
`composite_4x4_blocks` call sites `0xAE185`/`0xAE1CB`/`0xAE203`, the
`ctx[0xd]` builder `0xAE438..0xAE458`) were read twice: once in the
decompiler, once in the printed disassembly. The simulator is throwaway
and not committed; the only tracked change in this slice is this file.
