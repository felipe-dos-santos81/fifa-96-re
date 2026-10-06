# FU-130 — the LE address model and the re-import go/no-go

Cycle 2, Task 1 (address-model repair). FU-129 established the empirical rule
that a code operand `X` denoting game data has its real bytes at
`X + 0x100000`. This slice confirms the *mechanism* against the raw LE header
and the loader, and records the re-import decision that feeds Task 2.

Result in one line: **the appended image is a genuine Rational DOS/4G
linear-executable; its data object (`object 4`) has LE linear base `0x100000`
and the file carries a real fixup page/record table that relocates the code's
segment-relative data addends by that base; `tools/fifa96_le.py` rebuilds the
flat image by copying object pages verbatim and therefore leaves the fixups
unapplied, while Ghidra's native Watcom LE loader applies them — so
`REIMPORT_VIABLE: yes` (and already realised by the `/FIFA96.EXE` program,
which preserves every code address and repairs the data xrefs).**

## 1. Hypothesis and outcome (falsifiable)

Hypothesis (Step 1): `/fifa96_le.bin` was produced from a genuine LE object
whose data segment linear address is `0x100000`; the loaded image kept
segment-relative pointer values (fixups unapplied). Refutation: the
data-segment field is not `0x100000`, or the file is not an LE.

Outcome: **not refuted**. The file is an LE (signature `LE` at file
`0x290A4`); `object 4`'s LE reloc base is exactly `0x100000` (§3); the pages
are stored unrelocated and carry segment-relative addends while the LE's
fixup tables relocate them (§4, §6). The stated mechanism ("fixups unapplied
in the flat rebuild") is exactly right, with one clarification: the fixups
*are present in the file* — it is `tools/fifa96_le.py`'s raw-page copy that
does not apply them.

## 2. Source image and parser interface

`tools/fifa96_le.py` has **no header-print subcommand**, only:

```
usage: fifa96_le.py [-h] [-o OUT] [--info] [exe]

fifa96_le.py — extract the protected-mode image from FIFA96.EXE.

positional arguments:
  exe                path to FIFA96.EXE (default:
                     /media/felipe/FIFAPCCD/fifa96.exe)

options:
  -h, --help            show this help message and exit
  -o OUT, --out OUT     write the flat image to this file
  --info                print layout summary
```

Source found under `game/`:

```
game/FIFAPCCD96.iso: ISO 9660 CD-ROM filesystem data 'FIFAPCCD'
game/hdd/FIFA96/FIFA96.EXE   (1,526,315 bytes)
```

The on-disk `game/hdd/FIFA96/FIFA96.EXE` is byte-identical to the pristine
ISO member `/FIFA96.EXE` (root of the CD):

```
$ sha256sum /tmp/opencode/FIFA96_iso.EXE game/hdd/FIFA96/FIFA96.EXE
5663105600acd29b6da86c0c779cee341def8e2e27d17b3cc881eb95d4fac399  /tmp/opencode/FIFA96_iso.EXE
5663105600acd29b6da86c0c779cee341def8e2e27d17b3cc881eb95d4fac399  game/hdd/FIFA96/FIFA96.EXE
$ cmp /tmp/opencode/FIFA96_iso.EXE game/hdd/FIFA96/FIFA96.EXE && echo IDENTICAL
IDENTICAL
```

So every header finding below is the retail build's, not a local patch.

## 3. The LE header and the data-segment linear address (Step 3)

`python3 tools/fifa96_le.py game/hdd/FIFA96/FIFA96.EXE --info` (verbatim):

```
file            game/hdd/FIFA96/FIFA96.EXE (1526315 bytes)
LE header       0x290a4
pages           263 x 4096 (last 4055)
page store      0x6da54 .. 0x174a2b
object 1        base=0x010000 vsize=0xc1ea0 flags=0x2005 pages=194
object 2        base=0x0e0000 vsize=0x34 flags=0x0005 pages=1
object 3        base=0x0f0000 vsize=0x18 flags=0x2003 pages=1
object 4        base=0x100000 vsize=0x6aa50 flags=0x2003 pages=67
entry           0x9fd10
stack           0x16aa50
```

Header fields read directly (header-relative offsets; header at file
`0x290A4`):

| field | offset | value | note |
|---|---|---|---|
| signature | `0x00` | `4C 45` `"LE"` | genuine LE |
| byte/word order | `0x02`/`0x03` | `0`/`0` | little-endian |
| CPU / OS | `0x08`/`0x0A` | `2` / `1` | 386 / OS2 (DOS/4G) |
| #pages | `0x14` | `0x107` (263) | |
| EIP object / EIP | `0x18`/`0x1C` | `1` / `0x8FD10` | entry = `0x10000+0x8FD10` = `0x9FD10` |
| ESP object / ESP | `0x20`/`0x24` | `4` / `0x6AA50` | stack at top of object 4 |
| page size / last | `0x28`/`0x2C` | `0x1000` / `0xFD7` | |
| fixup section size | `0x30` | `0x443B6` | |
| loader section size | `0x38` | `0x4483A` | |
| object table off / count | `0x40`/`0x44` | `0xC4` / `4` | |
| object page map off | `0x48` | `0x124` | |
| **fixup page table off** | `0x60` | **`0`** | see §4 |
| **fixup record table off** | `0x64` | **`0`** | see §4 |
| (field `0x68`) | `0x68` | `0x548` | points at the fixup page table |
| (field `0x6C`) | `0x6C` | `0x968` | points at the fixup record table |
| data pages off | `0x78` | `0x448FD` | |

Object table (header+`0xC4`, 24-byte entries) — **the data segment**:

| # | reloc base | virtual size | flags | page idx | pages | role |
|---|---|---|---|---|---|---|
| 1 | `0x010000` | `0xC1EA0` | `0x2005` | 1 | 194 | **code** (read+exec) |
| 2 | `0x0E0000` | `0x34` | `0x0005` | 195 | 1 | 52-byte code stub |
| 3 | `0x0F0000` | `0x18` | `0x2003` | 196 | 1 | 24-byte data stub |
| 4 | **`0x100000`** | **`0x6AA50`** | **`0x2003`** | 197 | 67 | **data / stack** (read+write) |

Flags decode: `0x0001`=readable, `0x0002`=writable, `0x0004`=executable;
`0x2000` set on objects 1/3/4. Object 4 is the writable object — the game's
data, with the stack at its top (`ESP object = 4`). This is the
**data-segment linear address: `0x100000`**.

## 4. The fixup finding (Step 3 cont.)

The standard LE fixup-table header fields are **zero** in this build
(`0x60 = 0x64 = 0`), yet `fixup section size = 0x443B6` is non-zero. The
fixup tables are physically present and are found via the `0x68`/`0x6C`
header fields:

```
fixup page table off 0x295ec   (= header 0x290a4 + 0x548, 4*(pages+1) bytes)
fixup record table off 0x29a0c  (= header 0x290a4 + 0x968)
fixup_size 0x443b6
fixup page table first dwords: 0x0, 0x2f7, 0x8b3, 0xc07, 0x10db, ... last 0x43f95
```

The page table is a 264-entry monotonically non-decreasing dword array whose
last entry `0x43F95` sits just inside the declared `0x443B6` fixup section —
unmistakably a fixup page table (one offset into the record table per page,
plus a terminator). Page 58 — the page holding the `0x4A6CD` operand of
`FUN_0004a6bc` — brackets to fixup-record bytes that target **object 4** at
offsets `0x1C64`, `0x1C5C`, `0x7370`, `0x736C`, i.e. the exact data
addresses FU-129 saw in the code:

```
page 58: rec range [0x1445c,0x14968) len=1292
... 07 00 06 1c 64  ... (target object 4, offset 0x1c64)
... 0d 04 70 73 07 00 (target object 4, offset 0x7370)
... 0d 04 6c 73 07 00 (target object 4, offset 0x736c)
```

(The exact LE fixup-record grammar was not fully decoded here — see Open
leg 1. The evidence that these records relocate object-4 references is the
byte-level proof in §6, which does not depend on the record grammar.)

## 5. Live-memory cross-check (Step 4, verbatim)

`ghidra_inspect_memory_content(program="/fifa96_le.bin", address="0x101A30", length=48)`:

```
{"address":"0x101a30","bytes_read":48,"hex_dump":"33 35 32 6B 6F 00 00 14 33 35 32 70 6C 00 00 C8 \n33 35 32 70 6B 00 00 C8 33 35 32 70 73 00 00 78 \n66 72 65 65 6B 69 63 6B 00 07 00 79 33 35 32 73","ascii_repr":"352ko\0\0.352pl\0\0.\n352pk\0\0.352ps\0\0x\nfreekick\0.\0y352s","detected_string":"352ko"}
```

`ghidra_inspect_memory_content(program="/fifa96_le.bin", address="0x1A30", length=48)`:

```
{"address":"0x1a30","bytes_read":48,"hex_dump":"00 00 00 00 ...","ascii_repr":"\0...","is_likely_string":false,"string_length":0}
```

`0x101A30` holds the resource-name table's real bytes (`352ko…`, the entry the
`0x7370` pointer table points at); `0x1A30` is all `00`. This confirms FU-129's
rule against live memory for this exact witness.

## 6. Mechanism: byte-level proof that the fixups are real (Step 4 cont.)

The same function exists at the same address in both Ghidra programs, but the
fixup is applied in one and not the other.

`/fifa96_le.bin` (flat rebuild by `fifa96_le.py`) — raw page bytes copied
verbatim:

```
search_instructions(program="/fifa96_le.bin", operand_pattern="0x1c64"):
  FUN_0004a6bc @ 0x4a6cd  MOV EDX, 0x1c64          bytes ba 64 1c 00 00
search_instructions(program="/fifa96_le.bin", operand_pattern="0x7370"):
  @ 0x4a6e3  MOV EDI, dword ptr [ESI + 0x7370]    bytes 8b be 70 73 00 00
  @ 0x4a71c  MOV EAX, dword ptr [ESI + 0x7370]    bytes 8b 86 70 73 00 00
```

`/FIFA96.EXE` (Ghidra's native `watcom:LE:32:default` loader) — same
location, fixup applied:

```
search_instructions(program="/FIFA96.EXE", operand_pattern="0x101c64"):
  FUN_0004a6bc @ 0x4a6cd  MOV EDX, 0x101c64         bytes ba 64 1c 10 00
search_instructions(program="/FIFA96.EXE", operand_pattern="0x107370"):
  @ 0x4a6e3  MOV EDI, dword ptr [ESI + 0x107370]   bytes 8b be 70 73 10 00
  @ 0x4a71c  MOV EAX, dword ptr [ESI + 0x107370]   bytes 8b 86 70 73 10 00
  @ 0x4a75d  MOV EBX, dword ptr [ESI + 0x107370]   bytes 8b 9e 70 73 10 00
  @ 0x4a799  MOV EDI, dword ptr [ESI + 0x107370]   bytes 8b be 70 73 10 00
```

The raw file bytes are the unrelocated ones; a fresh parser run reproduces
them exactly:

```
image 0x4a6cd: file_off 0xa8121 raw=ba 64 1c 00 00 b8  parser_img=ba 64 1c 00 00 b8
dword at file for image 0x4a6ce: 0x1c64
```

The native delta is exactly `0x100000` = object 4's LE reloc base, and the
loader resolved the string operand to a real symbol
(`s_art_gameart0_pvi_00101c6c` at `0x101C6C`, one `PARAM` xref from
`FUN_0004a6bc` at `0x4a6d2`), whereas the flat image has no xref to
`0x101C6C`. Corpus-wide, the applied/relative forms partition cleanly:

| operand form | `/FIFA96.EXE` (native, fixups applied) | `/fifa96_le.bin` (flat, unapplied) |
|---|---|---|
| `0x101c64` / `0x1c64` | 1 hit (`MOV EDX,0x101c64`) | 0 |
| `0x1c64` / `0x101c64` | 0 | 1 hit (`MOV EDX,0x1c64`) |
| `0x107370` | 4 hits | 0 |
| `0x7370` | 0 | 2 hits |

Native program facts (Ghidra):

| | `/FIFA96.EXE` | `/fifa96_le.bin` |
|---|---|---|
| language | `watcom:LE:32:default` | `x86:LE:32:default` |
| functions / symbols | 2706 / 15239 | 2650 / 10736 |
| memory blocks | `.object1` `0x10000`, `.object2` `0xE0000`, `.object3` `0xF0000`, `.object4` `0x100000–0x16AA4F`, `.image` overlay | single `ram` `0x0–0x16AA4F` |
| entry | `_entry` = `0x9FD10` | only generic entry at `0x0` |
| data xref to `0x101C6C` | 1 (`FUN_0004a6bc`) | 0 |

Both programs keep the code at identical addresses (`FUN_0004a6bc` at
`0x4A6BC`), so adopting the native layout changes **no code address** — it
only turns each data operand from its segment-relative form into the absolute
`+0x100000` form and connects the xrefs.

## 7. Decision

```
REIMPORT_VIABLE: yes
```

Reasoning, all from the above:

* The data object's LE linear base is `0x100000` (object table, §3), matching
  FU-129's rule; and the file demonstrably contains the fixup tables that
  relocate object-4 references by that base (§4), which a correct loader
  applies (§6).
* A correct re-import already exists: Ghidra's built-in Watcom LE loader
  loaded the same file as `/FIFA96.EXE` with `object4` at `0x100000`, the
  `+0x100000` addends applied, and the data xrefs resolved — while keeping
  every code address identical to `/fifa96_le.bin`.
* Therefore Task 2 can proceed: either switch the analysis to the native
  `/FIFA96.EXE` program, or patch `tools/fifa96_le.py` to apply the fixup
  page/record tables (or, equivalently, to add `0x100000` to object-4
  references) before writing the flat image. The former is recommended
  because it is loader-authoritative and already carries 15,239 symbols.

## 8. Surprises recorded

1. **The standard fixup offsets are zero.** Header `0x60`/`0x64`
   (fixup page/record table) read `0` even though `fixup section size` is
   `0x443B6`. The tables are actually addressed by the `0x68`/`0x6C` fields
   (`0x548`/`0x968`) — the offsets the standard spec labels
   imported-modules-name/reference. This is genuine to the retail build
   (byte-identical to the ISO original), so it is a DOS/4GW header-layout
   quirk, not local tampering. (Open leg 1.)
2. **`fifa96_le.py`'s docstring mislabels the data object.** It says
   `obj3 data @ 0x0F0000 (0x18 bytes)` and `obj4 stack @ 0x100000`, but the
   game strings/tables live in **object 4** at `0x100000+` (verified:
   `GAMEART` at `0x101C64`, `art/gameart0.pvi` at `0x101C6C`), and object 3
   is a 24-byte stub (`87 83 81 82 0A0A0A0A 0B0B0B0B 0C0C0C0C 00 02 04 06 01
   03 05 07`). Object 4 is the writable data/stack object. The parser's
   *output* is correct; only its prose comment is wrong, and the prose is
   the likely origin of the "obj3 is data" confusion.
3. **The flat import's entry point is wrong** (`0x0`, in zero padding),
   whereas the native loader records the real `_entry` at `0x9FD10`.

## 9. Provenance

* Files: `game/hdd/FIFA96/FIFA96.EXE` (sha256 `5663…ac399`), byte-identical to
  ISO `/FIFA96.EXE` extracted with `xorriso -osirrox`; `tools/fifa96_le.py`.
* Commands: `python3 tools/fifa96_le.py game/hdd/FIFA96/FIFA96.EXE --info`;
  `od -A x -t x1z -j 0x290a4 -N 256 …`; a header/fixup parser reading the
  object table at `hdr+0xC4`, fixup page table at `hdr+0x548`, fixup record
  table at `hdr+0x968`; `cmp`/`sha256sum` on the ISO extract.
* Ghidra MCP: `list_open_programs`, `get_metadata` (both programs),
  `list_segments` (both), `get_entry_points` (both), `get_address_spaces`,
  `inspect_memory_content` `0x101A30`/`0x1A30`/`0x101C64`/`0x1C64`/`0xF0000`,
  `search_instructions` operands `0x1c64`/`0x101c64`/`0x7370`/`0x107370`
  (both programs), `search_functions ^FUN_001` (both, 0 hits),
  `search_strings GAMEART`, `get_function_by_address 0x100000`,
  `get_xrefs_to 0x101C6C` (both), `decompile_function 0x4a6bc` (both).
* Address mapping convention as FU-88/FU-129; data address renders use the
  `+0x100000` form.

## 10. Open legs

1. Decode the LE fixup-record grammar fully and confirm programmatically that
   page/record tables at `hdr+0x548`/`hdr+0x968` relocate every object-4 (and
   object-2/3) reference by the target object's base. The standard `0x60`/`0x64`
   fields being zero must be understood before Task 2 relies on the tables by
   offset.
2. Decide Task 2's import route: adopt native `/FIFA96.EXE` (recommended) or
   patch `tools/fifa96_le.py` to apply fixups; reconcile the 2706 vs 2650
   function counts and 15239 vs 10736 symbols between the two.
3. Establish whether +`0x100000` applied uniformly to data references is
   equivalent to the fixup tables for all target objects (object 2/3 bases
   `0x0E0000`/`0x0F0000` may also appear as fixup targets).
4. Confirm the native program's `.image` overlay (whole file at `0x0`) does not
   duplicate bytes or pollute analysis of the object segments before adoption.
5. Reconcile the LE `data pages offset` (`0x448FD`, absolute `0x6D9A1`) with the
   observed page-store start `0x6DA54` (delta `0xB3`); confirm the page-store
   base computation is the file tail as `fifa96_le.py` assumes.
6. Re-label the affected data globals in whichever program Task 2 adopts and
   fold the FU-129 erratum into the analysis corpus.
