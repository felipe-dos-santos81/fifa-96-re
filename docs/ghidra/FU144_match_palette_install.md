# FU-144 — the native match palette install (OL-T11-6)

Follow-on to FU-85 §5 (`0x14720` remap), FU-91 (sprite palette chain), FU-98
(kit remap tables), FU-57 §4 (DAC shadow) and FU-64 §4 (match-data load). This
slice derives the palette the native match-data load installs, ports it, and
wires it into the engine's match render path so the indexed draw becomes
RGB-visible (`OL-T11-6`).

All addresses are first-hand `/FIFA96.EXE` (Ghidra MCP, read-only).

Result in one line: **`FUN_00048ED8` (single caller `FUN_0003BB1C 0x3BB45`)
builds the match palette from resource slot 0x32 = `PALsys.fsh` frame 2
(`FUN_0004AFB8(0x32)` -> `FUN_000A1920(handle, 2)` -> `FUN_00047814`), applies
the FU-98 kit remap and two chunk-range appends, and `FUN_00048C8C` installs
the 6-bit result into `0x4B200` and rebuilds the 8-bit RGB copy at `0x4B800`
with `v << 2` (`FUN_000479A0`).**

## 1. The install chain (first-hand disassembly)

`FUN_00048ED8` (`0x48ED8..0x48FF2`):

```
0x48EE5  CMP [0x1068E0],1 / JZ end            ; match-data-loaded guard
0x48EF6  PUSH 2; EAX=0x32; CALL 0x4AFB8       ; resource handle, slot 0x32
0x48F03  CALL 0xA1920                         ; frame 2 (FUN_000A1920)
0x48F0B  CALL 0x47814; MOV EDI,EAX            ; EDI = the frame's 0x22 chunk+16
0x48F1B  EAX=0x4B200; CALL 0x48B60            ; [0x7104]==0: build the palette
0x48F25  memmove(src=0x4B200 -> local, 0x300) ; current palette snapshot
0x48F3C  memmove(local -> 0x4B500, 0x300)
0x48F55  10x memmove(src=0x4B500+[0x1070E8+i]*3,
                     dst=local +[0x1070F2+i]*3, 3)
0x48F86  memmove(src=EDI+0xF0  -> local+0xF0,  0x54)
0x48F9F  memmove(src=EDI+0x186 -> local+0x186, 0x4E)
0x48FBA  FUN_00048C8C(local, [0x7104])        ; install
0x48FD5  [0x1070FC]=1; [0x11471C]=0x4783C; [0x1068E0]=1
```

`FUN_00048B60` (`0x48B60..0x48C36`) is the same build on its own arguments:

```
0x48B6E  PUSH 2; EAX=0x32; CALL 0x4AFB8; CALL 0xA1920; CALL 0x47814
0x48B88  MOV EBP,EAX                          ; PALsys frame-2 chunk data
0x48B92  memmove(src=EDI(0x4B200) -> local, 0x300)
0x48BA7  10x memmove(src=EDI+[0x1070E8+i]*3, dst=local+[0x1070F2+i]*3, 3)
0x48BD5  memmove(src=EBP+0xF0  -> local+0xF0,  0x54)
0x48BEE  memmove(src=EBP+0x186 -> local+0x186, 0x4E)
0x48C1A  [0x7104] = FUN_000A154C(local)       ; register the built palette
```

The copy helper `FUN_000CD390` (disassembly `0xCD390`: `ESI=[EBP+8]`,
`EDI=[EBP+0xC]`, `MOVSB.REP ES:EDI,ESI`) copies **arg1 -> arg2**, so in the
loops above the *first* pushed address is the source and the second is the
destination. With the image tables (read `0x1070E8`, 20 bytes):

```
0x70E8 (0x1070E8): 132,135,140,143,146,150,153,158,161,164   ; snapshot source
0x70F2 (0x1070F2): 156,157,158,159,160,161,162,163,164,165   ; write target
```

the transform is `pal[0x70F2[i]] = snapshot[0x70E8[i]]` — the FU-98 kit remap
with **its source/destination roles inverted** (erratum below).

`FUN_00048C8C` (`0x48C8C..0x48D34`):

```
0x48C91  memmove(src=arg1 -> 0x4B200, 0x300)  ; install the built palette
0x48CA4  EAX=0x4B800; CALL 0x479A0            ; 8-bit RGB + ARGB shadow
0x48CF1  EAX=0x4B200; CALL 0x47A70            ; near-black key table (0x4BF28)
0x48CFB  0x5F500(0x4B200); 0x5AE64(0x4B200)
0x48D1B  0x47184(0x4B800); [0x11471C]=0x4783C; 0x19B44; [0x11471C]=0
```

`FUN_000479A0` (decompile `0x479A0`): per entry `out[3i+c] = pal[3i+c] << 2`
and the ARGB dword `0xFFRRGGBB` at `0x15BBAC`. So the native 8-bit RGB form is
`v << 2`, not FU-91's `v*255/63` sprite-palette scaling.

## 2. The palette source is `PALsys.fsh` frame 2

`FUN_0004A6BC` (disassembly) fills the 63-slot handle table
`0x14BFC0..0x14C0BF` from `art/gameart0.pvi`:

* slots 0..29: `sprintf(buf, "%s.fmt", table[i])` (0x101C80);
* slots 30..38: `"%s.dat"` (0x101C88);
* slots 39..55: `sprintf(buf, "%s.%s", table[i], "fsh")` — the pointer pushed
  is `0x101C90`, which is the `fsh` inside the packed string
  `lfsh` at `0x101C8F` (read `0x101C8F` = `6c 66 73 68 00`);
* slots 56..62: `"%s.qfs"` (0x101C9C), plus `flags.qfs` (0x101CA4) -> `[0x14C0E0]`.

The parameter table `0x107370`'s slot 50 (0x32) is `0x101BA4` = `"PALsys"`
(read + `search_strings`), so **slot 0x32 is `PALsys.fsh`**. The retail
`/ART/GAMEART0.PVI` BIGF directory (decoded with the committed codecs) holds it
as entry 46: SHPI total 3160, 3 frames, each 16x15 with a type-`0x22` chunk at
second offset 0x100. Frame 2's chunk (6-bit RGB) starts
`00 00 00 | 38 11 28 ...`; `FUN_000A1920` resolves the frame as
`handle + [handle+0x14+2*8]` and `FUN_00047814` returns `chunk+16` for the
`0x22` chunk (`*frame >> 8` = second offset).

## 3. The derived engine palette

* source: the frame-2 `0x22` chunk (256x6-bit RGB);
* kit remap: `pal[156..165] = snapshot[{132,135,140,143,146,150,153,158,161,164}]`
  with pre-write snapshot semantics (FU-144 erratum vs FU-98);
* appends: chunk bytes `[0xF0,+0x54)` and `[0x186,+0x4E)` copied over the
  built palette (ranges 80..107 and 130..155);
* conversion: `v << 2` (`FUN_000479A0`), installed onto the engine surface.

The native base buffer `0x4B200` is the previously installed front-end palette
and is not statically derivable; the engine substitutes `base := the chunk`
(`fifa96_match_palette_from_bank(..., base6 = NULL, ...)`), under which the
native appends are identity. Recorded leg: the exact front-end base.

## 4. Port and wiring

* `fifa96_match_palette_from_bank` (`fifa96_match_run.c`): SHPI frame 2 ->
  `0x22` chunk -> 256 entries (else `-FIFA96_ERR_UNSUPPORTED`; the native
  copies 0x300 bytes unconditionally, the port hardens) -> optional 6-bit base
  -> `fifa96_sprite_palette_kit_remap` (corrected) -> appends -> `v << 2`.
* `fifa96_match_run_palette_install(mr, s)`: `fifa96_surface_set_palette8` of
  the staged `render.palette` (`-FIFA96_ERR_STATE` when `palette_ready == 0`).
* `fifa96_match_run_stage`: finds the pitch container's case-insensitive
  `PALsys.fsh` BIGF name (the native `FUN_000A81A5` compare lowercases),
  extracts the palette into `render.palette`, and clears `palette_ready` on a
  re-stage without that entry. Failure stays atomic.
* `fifa96_match_run_render`: installs the staged palette before the plane
  conversion, so every presented match frame carries RGB. Unstaged runs
  (menu/tests) are unchanged.

Tests: `test_engine_match_render.c` (`test_match_palette_from_bank` covers the
direction, snapshot, appends and `v << 2`; `test_match_palette_install` covers
the surface install and render integration), `test_engine_match_staging.c`
(ISO: staged palette equals the pure extraction of `banks[91+46]`, retail
entry 1 -> `E0 44 A0`, GAMEFLD1 re-stage clears it), `test_engine_m2.c` (RGB at
the first match present), `test_sprite_palette.c` (native remap direction).
The M2 golden was re-pinned for the palette upgrade (see ENGINE.md).

## 5. Errata

* **FU-98 §1 (kit remap direction) — corrected.** The native loop writes the
  0x70F2 table from the pre-write snapshot of the 0x70E8 table:
  `pal[0x70F2[i]] = snapshot[0x70E8[i]]` (first-hand `0x48BA9`/`0x48BBB` and
  `0x48F57`/`0x48F69`; `FUN_000CD390` copies arg1(src) -> arg2(dst)). FU-98's
  `pal[dst=0x70E8[i]] = snapshot[src=0x70F2[i]]` and the ported
  `fifa96_sprite_palette_kit_remap` were inverted; the function is corrected to
  the native direction with snapshot semantics (the 156/161/164 overlap the
  write targets, so an in-place loop would differ). The ported function had no
  production caller before this slice.
* **FU-89 §11 (loader name formats) — corrected.** The `.lfsh` slots use the
  pointer `0x101C90` = `"fsh"` (the packed `"lfsh"` string starts one byte
  earlier at `0x101C8F`), so slots 39..55 resolve `%s.fsh` — `PALsys.fsh`,
  `PALteam.fsh`, `Npost.fsh`, ... — not `%s.lfsh`.
* **FU-91 §3 (palette install chain) — extended.** The DAC-side install for the
  match is `FUN_00048ED8`/`FUN_00048B60`/`FUN_00048C8C`/`FUN_000479A0` above;
  the per-entity translation install `FUN_00048DC0` -> `FUN_000CE980(0x14720)`
  is unchanged and remains the engine's identity stand-in (open leg).

## 6. Open legs

1. **Base palette `0x4B200`**: the front-end-installed palette the match load
   appends to; not statically derivable (engine uses base := the chunk).
2. **Per-entity translation tables** `0x4BF60[slot]` (the native `0x14720`
   remap): `FUN_00046F80` partitioning of a loaded "palettes" resource; the
   resource identity is FU-91's open leg. The engine keeps the identity remap
   (with `0` as the 0xFF key).
3. **`FUN_00048DC0` `[0x68E0]==1` branch** (the entity 0/0xB kit-recolour byte
   translation via `0x7287`/`0x727C`).
4. **`FUN_00047A70` near-black key table** (`0x4BF28`) and the shade cube
   (`FUN_000A0AA0`, FU-98 §2) are built from the palette but not consumed by
   the port.
5. **DAC/shadow upload**: `FUN_000CE70C`/`FUN_000CE754` and the `0x15BBAC`
   shadow are presentation bookkeeping the SDL3 backend replaces.

## 7. Provenance

Ghidra MCP on `/FIFA96.EXE` (read-only): `disassemble_function` 0x48ED8,
0x48B60, 0x48C8C, 0x4A6BC, 0xCD390; `decompile_function` 0x47814, 0xA1920,
0xA1918, 0x479A0, 0x4AFB8, 0xA2614, 0xA246C, 0xA81A5, 0x99E9F, 0x48DC0,
0x48C8C; `disassemble_bytes` 0x48BA7 (34 B), 0x48F55 (36 B), 0x4A758 (32 B),
0x48DF0 (48 B); `read_memory` 0x1070E8 (20 B), 0x1070F2 (20 B), 0x101C80
(48 B), 0x101C8F (24 B), 0x101C6C (64 B), 0x107370 (192 B), 0x107430 (64 B);
`search_strings` `PAL` -> `PALsys` 0x101BA4, `PALteam` 0x101BD4;
`get_xrefs_to` 0x101BA4/0x101BD4/0x48ED8/0x4A6BC; `search_instructions`
`14b200` (10 sites); `search_byte_patterns` `d8 8e 04 00` (0).
Retail data: `fifa96_play` on the ISO's `/ART/GAMEART0.PVI` (entry 46
`PALsys.fsh`, frame 0 PPM sha256 `96890aa25d41f57ab1722ba149c19b4c466fbb506a9d7d6f68fc94542657b4bf`).
Engine write set: `fifa96_match_run.c`/`.h`, `fifa96_sprite.c`,
`tests/test_engine_match_render.c`, `tests/test_engine_match_staging.c`,
`tests/test_engine_m2.c`, `tests/test_sprite_palette.c`,
`tests/golden/engine/m2-frames.txt` (palette upgrade re-pin).
