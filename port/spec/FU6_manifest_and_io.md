# FU-6: the embedded file manifest and the file-I/O layers

Follow-up to `FU5_vgt_decoders.md` for the "trace PCCD.DB / PCINDEX.NDX to the
POG/VIV/QFS decoders" slice.

## What PCCD.DB / PCINDEX.NDX are

They are **not parsed by name in code**. Both strings exist only in object 4
(`0x100004`, `0x100014`), inside the game's static data, and every apparent
code pointer to them turned out to be operand bytes of unrelated instructions
(`MOV dword [0x4AEE0],0x10` at `0x4001A`, `MOV dword [EDX+0x14],0x1000` at
`0xBAF68`). The same is true for `PCNX` (`0x100020`) and `SHPI`
(`0x142F8C`): data only, no tag immediate anywhere in the code object.

Object 4 begins with a **file manifest**: NUL-terminated names interleaved
with small binary values, including `PCCD.DB`, `PCINDEX.NDX`, `PCNX`,
`PCDB`, and later blocks of runtime asset names:

| block | examples | stride |
| --- | --- | --- |
| `0x100004` | `PCCD.DB`, `PCINDEX.NDX`, `PCNX`, team strings | variable |
| `0x102484` | `PHR_BAS0.VIV`, `PHR_1HF0.VIV`, ... | 16 bytes: name+NUL padded to 13, u16, pad |
| `0x10A3C0` | `XTCHAMP.PVI`, `FW2.QFS`..`FW9.QFS` | name + 4-byte value + 2 pad |
| `0x10AF40` | `PCCD.POG`, `LANG00x.POG`, `PCINDEX.POG`, `TEM_T006.VIV`, `CHN_CHA0.BNK` | name + 4-byte value + pad |

The 4-byte values (`0xB7..`, `0x6B40..`, `0x3CA1..`) are not addresses and
vary per file; they match the role of the CD's `CRCVALS.DAT` checksums. The
manifest gives the file layer names and expected checksums, **not** decoder
types: the `.POG`/`.VIV`/`.QFS` decode paths are not selected by a tag compare
in the image.

## File-I/O layers (32-bit image)

Two independent layers issue `INT 21h`:

* Game layer, `0xBABE0`..`0xBB241`: `file_open_ro` (0xBABE0, two AH=3D sites),
  close (0xBAC4F/0xBAC8A/0xBACFB/0xBB241), write (0xBAC78), create (0xBACE7),
  ioctl (0xBADCB/0xBADF3), read (0xBAFB8). Client so far: `FUN_000AEE41`
  (the VGT stream loader).
* Watcom C runtime, `0xCAA62`..`0xCD725`: open/close/read/write/seek/lseek
  used by C-level code.

## Conclusion for this slice

Static tag dispatch for POG/VIV/QFS does not exist. To reach their decode
entry points, the remaining options are:

1. Runtime breakpoints on the game-layer read wrapper (`0xBAFB8`) and the
   runtime read wrapper (`0xCD666`) while the game opens a `.POG`, `.VIV` or
   `.QFS` file, then walk the return addresses (DOSBox-X debugger; the host
   has a live X11 display for `xdotool`, and the capability-free DOSBox-X copy
   at `/tmp/opencode/dosbox-x-nocap` is memory-dumpable).
2. Find the manifest consumer (the code reading object 4) and follow how a
   manifest entry becomes a file handle and a decode call.
