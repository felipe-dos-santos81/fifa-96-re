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
