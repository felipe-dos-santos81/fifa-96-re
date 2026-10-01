# FU-4: the FIFA96.EXE protected-mode image

## Finding

Every function in `loader_rename_map.md` lives in the **MZ image** of
`FIFA96.EXE` (file offsets 0..0xF474, Ghidra segments 1000/11bd/1991). That
image is the DOS/4GW stub: startup argv parsing, the "object load" dispatcher,
and the five-site codec funnel are DOS-extender loader internals, not game
code. The game is the **linear-executable (LE) image appended to the same
file**, which no prior analysis had loaded. This explains why the FU-1 sweep
found none of the format tags in the analysed scope.

## Layout (retail build, verified)

| field | value |
| --- | --- |
| LE header | file `0x290A4` (`LE`, byte/word order 0, CPU 386, OS DOS) |
| pages | 263 x 4096, last page `0xFD7` |
| page store | file `0x6DA54` .. EOF (`0x174A2B`), sequential in object order |
| page map | header+`0x124`, 263 dwords = `byteswap16(page) << 8` (identity order) |
| EIP | object 1 + `0x8FD10` -> flat `0x9FD10` (`jmp 0x9FD88`) |
| ESP | object 4 + `0x6AA50` -> flat `0x16AA50` |

Objects (RelocBaseAddress / virtual size / stored pages):

| # | base | vsize | pages | content |
| --- | --- | --- | --- | --- |
| 1 | `0x010000` | `0xC1EA0` | 194 | code and constants |
| 2 | `0x0E0000` | `0x34` | 1 | 32-bit stub (52 bytes, starts at page offset 0) |
| 3 | `0x0F0000` | `0x18` | 1 | data table (24 bytes, starts at page offset 0) |
| 4 | `0x100000` | `0x6AA50` | 67 | data/stack (remaining pages zero-fill) |

Checks used to pin the page base: the store ends exactly at EOF
(`0x6DA54 + 263 pages = 0x174A2B`); object 2/3 data begins at page offset 0;
fixup sources for logical page 1 resolve against page base `0x6DA54` 13x
better than any other candidate; and the resulting entry at `0x9FD10` is
32-bit startup code (`sti; and esp,-4; ...; INT 21h AH=30h ...`).

## Runtime cross-check

A guest-RAM dump taken while the game runs shows an applied internal fixup:
the file stores target offset `0x158DA`, memory holds `0x2E68DA`, i.e. the
loader relocated object 1 from the link base `0x10000` to `0x2E1000`
(delta `0x2D1000`). The link-time image produced by the tool is therefore
also the layout the game runs with, modulo that constant delta.

## Tool

`tools/fifa96_le.py` parses the LE header/objects/page map, rebuilds the flat
link-time image, and verifies the store consumes the file tail exactly.
`tests/test_le.py` covers a synthetic LE plus the retail build.

```
python3 tools/fifa96_le.py /path/to/FIFA96.EXE -o build/fifa96_le.bin
python3 tools/fifa96_le.py --info
```

## Next

Import `fifa96_le.bin` into Ghidra as `x86:LE:32:default` (base 0) and search
the code object for the format decoders. Absolute data references in the flat
image carry the runtime delta (`0x2D1000`) pending fixup application; relative
calls and control flow are already correct.

## Decoder anchors found in the image

Ghidra program `/fifa96_le.bin` (2,066 functions) now carries these names:

| address | name | evidence |
| --- | --- | --- |
| `0x67BA8` | `vgt_stream_poll` | polls a stream buffer; accepts tag dwords `eVGT`..`kVGT`; on `kVGT` copies `0x300` bytes (palette) to `0x560C0` |
| `0xAE4BC` | `vgt_dispatch` | `fVGT` -> `vgt_decode_f`, `kVGT` -> `vgt_decode_k`, else error `FUN_000cbbe8`; decoded size lands at context+`0x28` |
| `0xADEFC` | `vgt_decode_f` | `fVGT` variant decoder |
| `0xAE218` | `vgt_decode_k` | `kVGT` variant decoder |
| `0xBABE0` | `file_open_ro` | two `MOV AH,3Dh; INT 21h` sites (`0xBAC10`, `0xBAD14`); caller `FUN_000AEE41` |

Dispatch is by **tag letters**, not extension: the immediate `CMP EAX,'kVGT'`
(`0x5447566B`) appears at `0x67C43` and `0xAE4E5`, and the poller accepts the
`eVGT`/`fVGT` range around it.

`PCNX` (POG) and `SHPI` (QFS/PVI) occur **only as data** in object 4
(`0x100020`, `0x142F8C`); no 32-bit tag immediate or word-pair compare exists
in the code object. Those formats are therefore selected by the CD index
(`PCCD.DB`, `PCINDEX.NDX`, first bytes of object 4) rather than by a
self-describing magic check. This supersedes the FU-1 note that the tags are
"absent from the image": they are present in the LE image, but POG/QFS decode
paths remain index-typed.

All addresses are link-time flat addresses; the running game adds the
`0x2D1000` relocation delta (see above).
