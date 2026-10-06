# FU-129 — the `+0x100000` data base and the image's identity layer

Cycle B, Leg A (library fingerprint). While looking for the region that
`0x9xxxx–0xAFFFF` belongs to, the probe turned up a structural fact about the
whole image: **data references in the code are offset by `-0x100000` from
their real linear addresses.** This slice records the rule, the image layout,
what the strings identify, and the erratum it forces on earlier slices' *data
addresses*.

Result in one line: **the image is code at `0x0–0xFFFFF` and a read-only data
segment based at `0x100000`; the LE fixups are not reflected in the loaded
image, so a code constant `X` that denotes a global/table/string must be read
at `X + 0x100000`. The data segment identifies the game (`EA Superstars`,
`CTEAM%.2d.DAT`, `C:\FIFA96\INSTALL.DAT`, the `art/*.pvi`, `*.qfs`, `*.dat`
resource paths) plus a statically linked support layer (DOS memory manager,
VESA window manager, SoundBlaster/Gravis device tables) under
`RATIONAL DOS/4G`.**

## 1. The `+0x100000` rule (empirical)

| code operand (as Ghidra shows it) | bytes there | real address | bytes there |
|-----------------------------------|-------------|--------------|-------------|
| `[ESI + 0x7370]` | all `00` | `0x107370` | `30 1A 00 00  38 1A 00 00  40 1A 00 00 …` an ascending table |
| `MOV EDX,0x1C64` | all `00` | `0x101C64` | `"GAMEART"` (then `art/gameart0.pvi`, `lay%s.fmt` …) |
| `[0x736C]` | `00` | `0x10736C` | `00 00 00 00` (BSS, the archive handle) |
| `[0x9090]` | `00` | `0x109090` | `00 … 01 … 05 09 0D…` (the frame counters) |
| `[0x4BFC0]` | all `00` | `0x14BFC0` | all `00` (BSS, filled by `FUN_0004a6bc`) |

All five witnesses differ by exactly `0x100000`. The `0x7370` case is the
cleanest: the code indexes a **pointer table** (`MOV EDI,[ESI+0x7370]`,
`ADD ESI,4`) whose real home `0x107370` holds `0x1A30, 0x1A38, 0x1A40, …` —
resource-name string offsets (`0x1A30 + 0x100000 = 0x101A30`).

Mechanism (to confirm, §7): the LE data segment's linear address is
`0x100000`, but the image as loaded carries **segment-relative** pointer
values — the `+0x100000` fixup is absent. Whatever the cause, the decoding
rule is unconditional for data.

## 2. Image layout

* Code: `0x0 – 0xFFFFF` — 2644 functions, all `FUN_*` entries are below
  `0x100000` (`search_functions ^FUN_001` = 0).
* A zero pad at `0xFFF00`–`0x100000` (256 bytes seen, all `00`) — the segment
  boundary.
* Data: `0x100000 – 0x16AA4F` — strings, tables, BSS; **no functions**.

## 3. What the data segment identifies

Game strings (all in `0x100000+`):

* `"EA Superstars"`, `"(EA Sports"`, `"FIFAPCCD"` (the program id in
  `"Not enough DOS memory to run FIFAPCCD"`), `"C:\FIFA96\INSTALL.DAT"`,
  `"D1TEAM A"`/`"TEAM B"`, `"LEAGUE teams"`, `"SEASON teams"`,
  `"WORLD_CUP teams"`, `"Big Player"`, `"DB Team String Data"`.
* Resource paths: `"GAMEART"`, `"art/gameart0.pvi"`, `"art/gamestd%d.pvi"`,
  `"art/gamefld%d.pvi"`, `"art/playart.pvi"`, `"playart.pvi"`,
  `"gfl /gamefld1.pvi"`, `"CTEAM%.2d.DAT"`, `"LANG%.3d.DAT"`,
  `"SGAME%.2d.DAT"`, `"options.dat"`, `"hiscores.dat"`, `"3dwpts.dat"`,
  `"*.qfs"`, and a table `GAMEART0.PVI/GAMEFLD1..5/GAMESTD1..5/PLAYART.PVI/XTCHAMP.PVI`
  at `0x10A2EA+`.

Support layer strings:

* DOS memory manager: `reservemem`, `resizemem`, `unlinkmemclassarea - Release
  Memory Not Found`, `initmemman`/`extinitmemman`, `savefileblock - OPEN
  FAILED`, `createwindowblock - OUT OF MEMORY requested %dx%d`.
* VESA window manager: `"VESA MEM"`, `getvesainfo - INVALID VESA INFO`, `"VESA
  video bios driver required. Please install a driver…"`.
* Audio device tables: `SoundBlaster 1.0/1.5/2.0/Pro 1.0/Pro 2.0/16`,
  `Gravis UltraSound`.
* `RATIONAL DOS/4G` and the `EXIT TO DOS (Y/N)` prompt — the extender banner.
* Author strings: `Rob Bailey`, `Robert Kaill`.

No `Miles`/`AIL`/`Adlib`/`Causeway`/`Watcom`/`Borland` strings appear, so the
audio layer is bespoke, not a third-party audio library.

## 4. Leg A conclusion

The `0x9xxxx–0xAFFFF` region is the game's **statically linked support/engine
library** (memory/VESA/window/audio platform layer), not the DOS/4G extender
(the extender is only the banner/loader strings) and not a third-party audio
library. It is still among the game's own code, so it stays in scope — but its
*identity* now organizes the remaining work: FU-115's audio device block and
FU-127's render paths sit on top of this layer. Because the code↔data xrefs
are broken by the addressing artifact (§1), data xrefs cannot be recovered
mechanically; string consumers must be found via the **offset form** of the
constant (e.g. search operand `0x1c64`, not `0x101c64`).

Call-direction evidence (xref fan-in into the region, all from the flat
names): `FUN_00099e9f` 118 callers, `FUN_000993ec` 112, `FUN_00092ac8` 111,
`FUN_000986b4` 83, `FUN_00098bf8` 73, `FUN_000984b4` 65 — the classic
format/alloc/free profile called from both game code (`0x0–0x8FFFF`) and
library-internal code.

## 5. Erratum — data addresses in earlier slices

The **semantics** derived from disassembly remain valid; only the *address
labels* (and any *initial value* that was read from memory) are wrong. Add
`0x100000`:

| earlier slice | label used | real address |
|---------------|-----------|--------------|
| FU-114 | `0x4AE44`/`0x4AEEC`/`0x4AF0A` scratch | `0x14AE44`/`0x14AEEC`/`0x14AF0A` |
| FU-115 | `0x55CE0` audio/MIDI device block | `0x155CE0` |
| FU-116/FU-127 | `[0x9090]`/`[0x9094]` counters | `[0x109090]`/`[0x109094]` |
| FU-119 | `0x4BFC0` resource table, `0x8E1C`, `0x55C84` | `0x14BFC0`, `0x108E1C`, `0x155C84` |
| FU-126/127 | `[0x677C]`/`[0x6780]` state, `[0x7300]`/`[0x7304]`/`[0x730C]` | `[0x10677C]`/`[0x106780]`, `[0x107300]`/`[0x107304]`/`[0x10730C]` |
| FU-128 | `0x736C`/`0x7370`, templates `0x1C64…0x1CA4` | `0x10736C`/`0x107370`, `0x101C64…0x101CA4` |

Note the corollary for FU-128: the "name templates" really are the game's
resource paths (`GAMEART`, `art/gameart0.pvi`, `art/gamestd%d.pvi`, …), so the
`0x7370` table is the **resource-name offset table**.

## 6. Provenance

Ghidra MCP on `/fifa96_le.bin`: `search_strings`
`(Borland|Watcom|Copyright|… )`, `(AIL|Miles|… )`, `(FIFA|team|PLAYER)`,
`(\.pvi|\.qfs|\.dat|GAMEART)`; `list_imports`/`list_exports` (both empty);
`get_metadata` (x86:LE:32, 2644 functions, 0x16AA4F max); `get_entry_points`
(single `entry` at `0x0`); `inspect_memory_content` at `0x7370`/`0x107370`,
`0x1C64`/`0x101C64`, `0x55CE0`/`0x155CE0`, `0x4BFC0`/`0x14BFC0`,
`0x9090`/`0x109090`, `0x736C`/`0x10736C`; `search_functions ^FUN_001`;
`read_memory`/`disassemble_bytes` `0x4A6BC` (confirms 32-bit opcodes: `0F 84
rel32`, `BA imm32`); `search_instructions` operands `0x1037d4`/`0x104404`/
`0x1001d4`/`0x101c64` (all 0 — the pointers are offset-form). Address mapping
as FU-88.

## 7. Open legs

1. Confirm the mechanism against the raw LE header/fixup records (is the data
   segment linear address `0x100000`, and were fixups skipped by the
   extractor or by Ghidra's loader?).
2. Exact start of the support/engine library below `0x100000` (data xrefs are
   unavailable; needs offset-form operand tracing).
3. `FUN_000a6a03`'s single caller `0x64fb2` and whether it is app or library.
4. Re-label the affected data globals in the Ghidra project (`+0x100000`)
   during the next cleanup pass.
