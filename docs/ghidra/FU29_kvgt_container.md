# FU-29 — kVGT container port (in-memory frame layout)

Date: 2026-10-04. Program `/fifa96_le.bin`. Result: the kVGT container
layer is ported (`fifa96_kvgt_decode`) against the disassembly-cited
in-memory frame layout, with an integration test over the committed
`record-30`/`record-46` vectors. The raw `VIDEO/*.TGV` file format is
**not** this layout and stays an open leg (see §3).

## 1. Cited layout (`vgt_decode_k` 0xAE218)

Disassembly `0xAE218..0xAE48F`; `vgt_dispatch` (0xAE4BC) passes the frame
buffer whose first dword is the tag (`*param_2 == 0x5447566B`, LE
`'kVGT'`). Field reads (each `MOV EAX,[...+2-4]` + `SHR EAX,16`, i.e.
BE16 at the address below):

| frame offset | value | ctx slot | instruction |
|---|---|---|---|
| +8 | width | ctx[0] | `0xAE22B..0xAE241` |
| +10 | height | ctx[1] | `0xAE247..0xAE262` |
| +12 | count | ctx[2] | `0xAE265..0xAE282` |
| +14 | palette_count | ctx[3] | `0xAE285..0xAE2A6` |
| +0x14 | palette_count RGB triples | ctx+0x44 | `0xAE2B3..0xAE2CE` |
| +0x14+palette_count*3 | compressed record | | `0xAE45A..0xAE474` (`CALL 0x9E860`) |

The original copies the palette with no bound (ctx+0x44 is 0x300 bytes);
the port rejects `palette_count > 256`. The original returns 1/0 (a
success flag), not a length; the port returns the record's decoded
length via `fifa96_record_decode` (selector dispatch per FU-19/FU-21).

## 2. Port and tests

* `include/fifa96_loader/fifa96_kvgt.h` + `src/fifa96_loader/fifa96_kvgt.c`:
  `fifa96_kvgt_decode(src, src_len, dst, dst_cap, out_len, info)` —
  tag/field/palette parse, record dispatch, bounds-checked throughout,
  `info` optional.
* `tests/test_kvgt.c`: synthetic frames embedding the committed vectors —
  `record-30` (huff) inside a 96x100 frame with a 2-entry palette, and
  `record-46` (tree) inside a 320x200 palette-less frame; asserts header
  fields, palette copy, decoded bytes byte-identical, guard bytes
  untouched; negatives: bad tag, `palette_count > 256`, truncated
  header/palette/record, `dst_cap` too small, NULL args.
* Suite 25 -> **26**, ASan/UBSan clean.

## 3. Open leg: the raw VIDEO file format

A feasibility probe parsed the `kVGT` magic frames in
`/VIDEO/VID_BULL.TGV` directly: fields at +8/+10 read as BE16 give
`0x6000/0x6400` (implausible), and frame spacing (`0x2724`) does not
match any field. `vgt_stream_poll` (0x67BA8) is the video-player loop:
frames come from the stream layer (`FUN_00095cb3`/`0x95eb8`/`0x95ed4`,
timing at `0x563c4..0x563f8`) and only then reach `vgt_dispatch`; the
file bytes are therefore not the in-memory frame bytes. Porting the
`VIDEO/*.TGV` reader requires reverse-engineering that stream layer (a
separate slice); this port implements only the cited in-memory layout,
which is what `vgt_decode_k` actually consumes.

## 4. Provenance

* Ghidra: `decompile_function 0xAE218`, `disassemble_function 0xAE218`
  (197 instructions, quoted above), `decompile_function 0xAE4BC`
  (tag check), `decompile_function 0x67BA8` (stream loop).
* Assets: `/VIDEO/VID_BULL.TGV` extracted from the read-only ISO via
  `tools/fifa96_bind.iso_files` (probe only, not committed).
* `make test`: 26/26.
