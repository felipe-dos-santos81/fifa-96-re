# FU-133 — the loader bridge (`/fifa96.exe`) and the `FUN_00063ebc` caller

Cycle 2, Task 4. Follows FU-114 §6 leg 1 (the "indirect entry, or the 16-bit
loader's phase table") and FU-130 (the LE address model). Question: does the
16-bit loader statically reference the LE image or `FUN_00063ebc`, and does the
LE actually have a static caller?

Result in one line: **the 16-bit `/fifa96.exe` is the third-party Rational
DOS/16M protected-mode run-time — generic, with no game or LE reference
(`0x63ebc`, `0x290a4`, `0x9fd10`, `0x100000`, `0x1fc000` all 0 hits across
15,708 instructions); the loader-bridge hypothesis for `FUN_00063ebc` is a
proven negative, and on the authoritative native `/FIFA96.EXE` the function has
two genuine static callers (`FUN_0004ad4c` @`0x4AE41`, `FUN_0004aea4`
@`0x4AF18`), both reached from `FUN_000493a0`.**

## 1. Loader inventory

* **Program-name collision (MCP).** The bridge resolves the `program=` string
  case-insensitively, so `program="/fifa96.exe"` targets the native
  `/FIFA96.EXE` (2,706 functions, `_entry` `0x9fd10`), **not** this loader. All
  loader results in this slice were taken from the loader as the current
  program (no `program=` argument), confirmed by `get_function_count` = 353 and
  `get_entry_points` → `11bd:2382`.
* Program `/fifa96.exe`: MZ, `x86:LE:16:Real Mode Ex`, image `63,632` bytes,
  **353 functions**, 775 symbols. `executable_path` is
  `/media/felipe/FIFAPCCD/fifa96.exe` (the CD is now unmounted); the on-disk
  `game/hdd/FIFA96/FIFA96.EXE` carries the identical MZ entry
  (`e_cs=0x01bd`, `e_ip=0x2382`), so the MZ view is the same file.
* Entry: `get_entry_points` → `entry` @ `11bd:2382` (external_entry), plus a
  program entry in the `int` overlay. MZ CS:IP `01bd:2382` renders as Ghidra's
  `11bd:2382`.
* Segments (`list_segments`): `CODE_0 1000:0000..1bcf` (7,120 B),
  `CODE_1 11bd:0000..7d3f` (32,064 B), `CODE_2 1991:0000..5963` (22,884 B),
  plus overlays `HEADER` (`0x0..0x1ff`) and `int` (`0x0..0x3ff`).
* Identity: the loader is the **Rational DOS/16M Protected Mode Run-Time**
  (banner `1000:0e20`), with its component names (`RUN.COM` `1000:0e4e`,
  `LOADER3.EXE` `1000:0e56`) and error set (`not a DOS/16M executable`
  `1000:1629`, `no relocation segment` `1000:16d2`, `protected mode available
  only with 386 or 486` `1000:17c6`, `…DOS16M.386…` `1000:1afd`, `makexecable`
  `1991:0794`). `search_strings` for `(FIFA|PCCD|EA |Rational|DOS/4G|…)`
  returns **no game identity string** — the loader is third-party, not
  game-specific.
* Named loader functions already present (from the 336-function loader map):
  `mem_grow_relocate` `1000:0b12`, `vet_file_header` `11bd:304f`,
  `dispatch_object_load` `11bd:5992`, `load_mf_object` `11bd:5dd2`,
  `exec_loaded_image` `11bd:6907`, `run_postload_init` `11bd:627f`,
  `setup_memory_hardware` `11bd:76db`, `print_error_message` `11bd:22ad`,
  `raise_boot_error` `11bd:2d43`, `enable_paging_and_load_tss` `11bd:2978`.

## 2. Load / relocation findings

The loader does contain relocation machinery — but for **DOS/16M 16-bit
objects**, not for the appended LE.

* `vet_file_header` (`11bd:304f`) vets the executable header: it reads 6-byte
  records and branches on the 2-byte magic — `MF` (accumulates module
  paragraph/byte size into `[0x11da]`/`[0x11dc]`), `MZ` (deferred to
  `FUN_11bd_300b`), `BW` (returns 1), else a disk reset and retry.
* `dispatch_object_load` (`11bd:5992`) checks the `MF` magic (`'M','F'`) and
  dispatches to `load_mf_object`, otherwise to the script pair
  (`parse_script_text` / `build_word_table`).
* `load_mf_object` (`11bd:5dd2`) is the object-load orchestrator: read the MF
  record, `alloc_retry_loop` paragraphs, read the object, then **apply
  relocations** — it walks a relocation table and adds the paragraph base to
  each stored far pointer (`*psVar1 = *psVar1 + (sVar10 + 0x15e9 …)`), copies
  the string table, and transfers control with `exec_loaded_image`
  (`11bd:6907`, "SS/SP switch + RETF transfer to loaded image entry").
* `mem_grow_relocate` (`1000:0b12`) is the DOS-memory relocation:
  `DosResizeMemory` (AH=4Ah) + backward slide + retarget of the segment words
  in the `0xf8c`/`0x1028`/`0x22` tables.
* The `no relocation segment` error (`1000:16d2`) has no direct xref because
  `print_error_message` (`11bd:22ad`) walks a message table beginning at
  `1000:15e8`: each entry is `[uint16 errno][string … '\0']` (first errno at
  `0x15e8`, first string at `0x15ea` = `involuntary switch to real mode`).
  That is why none of the error strings show instruction xrefs.

**Absence — the decisive part.** `search_instructions` over the whole loader
(15,708 instructions scanned) for operand patterns `63ebc`, `290a4`, `9fd10`,
`100000`, `1fc000` all return **0 matches**. The loader references neither the
appended LE header (`0x290a4`), nor its entry (`0x9fd10`), nor the data-object
base (`0x100000`), nor the `0x1fc000` load delta (measured at run time in
FU-11/FU-31, not derived in this slice), nor `FUN_00063ebc`. Its relocation
logic is the DOS/16M 16-bit far-pointer form — a
different mechanism from the LE `+0x100000` data fixups, which live in the
appended LE header (FU-130 §§3–6) and are applied by an LE loader. So the
loader does **not** explain `+0x100000`, and there is no loader-side phase/entry
table naming `FUN_00063ebc`.

*Cross-link FU-130:* FU-130 located the LE fixup page/record tables at
`hdr+0x548`/`hdr+0x968` (file `0x295ec`/`0x29a0c`) that relocate object-4
references by `0x100000`. Those tables are in the LE, not in this 16-bit
program.

## 3. `FUN_00063ebc` caller verdict

**Resolved — to game code, not the loader.** On the authoritative native
`/FIFA96.EXE` (fixups applied), `search_instructions` finds exactly two sites
referencing `0x63ebc` (234,567 instructions scanned):

| site | enclosing function | instruction | bytes | arg |
|---|---|---|---|---|
| `0x4AE41` | `FUN_0004ad4c` (`0x4AD4C..0x4AEA0`) | `CALL 0x00063ebc` | `e8 76 90 01 00` | EAX=1 (alloc) |
| `0x4AF18` | `FUN_0004aea4` (`0x4AEA4..0x4AF1C`) | `JMP 0x00063ebc` | `e9 9f 8f 01 00` | EAX=0 (free, tail call) |

`get_bulk_xrefs(0x63ebc)` on `/FIFA96.EXE` returns those two callers only.
Both are called by `FUN_000493a0` (a match-lifecycle/state routine with three
callers `FUN_00018680`, `FUN_00018108`, `FUN_00018014`): `FUN_000493a0` calls
`FUN_0004ad4c` at `0x49424` (alloc) and `FUN_0004aea4` at `0x49567` (free).
`FUN_0004aea4`'s body is exactly the call sequence FU-114 quoted and retracted
(`FUN_00043d94`, `FUN_0004a830`, `FUN_0004b454`, `FUN_00092f04`,
`FUN_00049138`, `FUN_00058e00`, `FUN_00063ebc`). `FUN_00063ebc` is therefore
the alloc/free pair invoked around a scoped buffer in `FUN_000493a0`.

**Reconciling FU-114 (erratum).** FU-114 examined the flat `/fifa96_le.bin` and
concluded the `0x4AE00..0x4AF1D` bytes were a runtime scratch cluster, then
`clearListing`-ed them, reporting `get_xrefs_to(0x63EBC)` empty. Under the
address model of FU-129/FU-130/FU-131, the writer `FUN_0003dbb0` scribbles at
the **data** addresses `0x14AE44`, `0x14AEEC`, `0x14AF0A` (native decompile:
`DAT_0014ae44`, `0x14aeec`, `0x14af0a`; FU-131's plate comment at `0x14AE44`
reads "scratch, element of dword array based 0x14AE40 (code operand 0x4AE44;
+0x100000)"). Those targets are in object 4 and are zero BSS (`read_memory`
`0x14AE40` = 16 zero bytes), so they never overwrite the code at `0x4AE41`.
The bytes at `0x4AE41` are byte-identical in both programs (`read_memory` on
`/FIFA96.EXE` and `/fifa96_le.bin` both = `00 e8 76 90 01 00 …`), i.e. a real
`CALL 0x63ebc`. FU-114's cleanup damaged the flat program — its
`FUN_0004ad4c` body now stops at `0x4ADBB` (native: `0x4AEA0`) and its
`FUN_0004aea4` is emptied (`body_start == body_end`) — which is why
`/fifa96_le.bin` reports only the `0x4AF18` xref. The native program preserves
the code and both callers. **FU-114 §3's "no static caller in the LE image" is
refuted; the caller leg is resolved.**

## 4. Open legs

1. **MZ/LE binding mismatch.** The 16-bit program is a DOS/16M run-time whose
   object loader handles `MF`/`MZ`/`BW`, yet the appended image is an `LE`
   (FU-130). The MZ header declares `e_cp = 123` pages = 62,976 bytes while the
   file is 1,526,315 bytes, so the MZ image stops well before the LE header at
   file `0x290A4`. How (or whether) this stub hands off to the LE is not
   visible in the 16-bit program's image; the LE-loading path, if any, is
   outside the MZ program (32-bit extender body beyond the MZ image, or a
   different binding of the same file).
2. **Completeness of the caller set.** Only the two direct sites above are
   statically visible; an indirect/table entry would not appear, but no such
   pointer to `0x63ebc` was found.
3. **FU-114 erratum closure.** Re-run FU-114's disposition under the
   `+0x100000` model and restore the `clearListing`-damaged
   `FUN_0004ad4c`/`FUN_0004aea4` in `/fifa96_le.bin` (or retire that program in
   favour of native `/FIFA96.EXE`, per FU-130).
4. **Overlay/mode machinery.** The `int::0000` overlay entry and the DOS/16M
   mode-vector/paging machinery were inventoried but not decompiled here.

## 5. Provenance

* Ghidra MCP, `/fifa96.exe` (MZ): `get_function_count` 353;
  `get_entry_points` (`11bd:2382`); `list_functions_enhanced`; `list_segments`;
  `get_metadata`; `search_strings` `(LE|fixup|reloc|FIFAPCCD|0x100000)` and
  `(FIFA|PCCD|EA |Rational|DOS/4G|…)`; `list_strings`; `search_instructions`
  operands `63ebc`/`290a4`/`9fd10`/`100000`/`1fc000` (all 0, 15,708 scanned);
  `decompile_function` `11bd:2382`, `1000:0b12`, `11bd:22ad`, `11bd:304f`,
  `11bd:5992`, `11bd:5dd2`, `11bd:627f`, `11bd:6907`; `search_byte_patterns`
  `d216` (0).
* Ghidra MCP, `/FIFA96.EXE` (native LE): `search_instructions` `63ebc` (2 hits,
  234,567 scanned); `get_bulk_xrefs` `0x63ebc`; `get_function_by_address` /
  `get_function_xrefs` `0x4ad4c`, `0x4aea4`, `0x493a0`; `decompile_function`
  `0x63ebc`, `0x3dbb0`, `0x4ad4c`, `0x4aea4`, `0x493a0`; `disassemble_bytes`
  `0x4ae38`/`0x4af10`; `read_memory` `0x4ae40`/`0x14ae40`; `get_comment`
  `0x4ae44`/`0x14ae44`/`0x63ebc`.
* Ghidra MCP, `/fifa96_le.bin` (flat): `get_bulk_xrefs` `0x63ebc` (1 hit,
  `0x4af18`); `get_function_by_address` `0x4ad4c` (body to `0x4ADBB`),
  `0x4aea4` (empty); `read_memory` `0x4ae40`/`0x14ae40`.
* Raw file (`game/hdd/FIFA96/FIFA96.EXE`, sha256 `5663…ac399`): MZ
  `e_cs=0x01bd`, `e_ip=0x2382`, `e_cp=123`, `e_cparhdr=0x20`; `16M` sig @
  `0xc7f`; `DOS/16M` @ `0x1018`; `no relocation segment` @ `0x18d2`; size
  1,526,315 B.
* Address renders: loader segments `1000:`/`11bd:`/`1991:`; native LE absolute
  addresses; flat `/fifa96_le.bin` offset-form. Mapping convention as
  FU-88/FU-129/FU-130.
