# FU-91 — Sprite second chunk and the 6-bit palette

Follow-on to FU-86 (open leg 1: second-chunk payload) and FU-85 (sprite colour
path). This slice derives the frame second chunk's palette form, closes FU-86
leg 1 for the palette case, and wires it into the host runner
(`docs/HOST_RUNNER.md`).

All addresses are link-space; object-1 code = link, object-4 data =
Ghidra+0x100000, stored code pointers/CS tables = stored+0x10000 (FU-76).
Data was read from the retail disc (`game/FIFAPCCD96.iso`, sha256
`d57c5f50818b7111…`, read-only) through the ported decode chain.

## 1. Frame second chunk, observed

The FU-86 frame layout stands: `+0x01 u24 second_offset` (0 = none), pixels at
`+0x10`, then the second chunk at `frame + second_offset`. Chunk starts
observed on the real disc (`/ART/GAMEART0.PVI`, extracted at ISO byte
`0x338800`, decoded through refpack → BIGF):

| Bank | frames | geometry | first `second` | frame stride | chunk type | chunk size |
|---|---|---|---|---|---|---|
| `PALteam.fsh` | 6 | 1×1 | `0x14` (20) | `0x324` (804) | `0x22` | 784 |
| `PALsys.fsh` | 3 | 16×15 | `0x100` (256) | `0x410` (1040) | `0x22` | 784 |
| player banks (`walk.fsh` sample, FU-86 §4.2) | n | w×h | `0x10+align4(w*h)` | pixels+24 | `0x7C` | 24 |

So a palette-bank frame is exactly `[16-byte header + pixels]` followed by a
784-byte chunk; the stride is `pixel area + 784`. The player chunk is a
24-byte record (`0x7C` tag) and is not a palette.

## 2. Type-`0x22` palette chunk layout

Real bytes (`PALteam.fsh` frame 0, chunk header):

```
22 00 00 00 00 01 01 00 00 01 00 00 00 00 00 00   then 768 bytes RGB
```

* `+0` `u8` = `0x22` (chunk type; the only value accepted by
  `fifa96_sprite_chunk_palette`)
* `+4` `u16` = entry count; `0x0100` = 256 in every observed palette chunk
* `+6`, `+8`, `+0xA` observed `1`, `256`, `0` — not consumed by the ported
  reader (open leg)
* `+0x0C` `u32` = 0 in the sample
* `+0x10` onward: `count * 3` bytes of 6-bit RGB; every sampled `PALteam`/
  `PALsys` frame stays within `0..63`

`fifa96_sprite_chunk_palette` bounds-checks `16 + count*3 <= chunk length`.
Because `fifa96_sprite_chunk_parse` reports the chunk length as the distance
to the **end of the bank** (not to the next frame), a type-`0x22` chunk whose
count runs past the bank is rejected as truncated (tested).

`fifa96_sprite_palette_to_rgb` scales 6→8 bit with integer `v * 255 / 63`
(0→0, 0x3F→0xFF).

## 3. Colour path

FU-86 §5 established the blit-time path: a sprite byte is a palette-relative
index; `FUN_000A2C50` writes it through the 256-byte translation buffer at
`0x14720`, where a **translated** `0xFF` means transparent.

The translation source per entity:

* `FUN_00048dc0(entity)` @0x48dc0 (decompile): `src = (&DAT_0004bf60)[entity]`
  (per-entity 256-byte table). For entities `0` and `0xB` when
  `[0x68e0] == 1` the table is copied to the stack and recoloured:
  * entity 0: entries `0x94..0x9A` ← table `0x7287`, base `0xA1`;
    entries `0x9B..0xA5` ← table `0x727C`, base `0xA3`;
  * entity `0xB`: entries `0x82..0x88` ← table `0x7287`, base `0x9C`;
    entries `0x89..0x93` ← table `0x727C`, base `0x9E`.
  Each rewritten entry is `table[old - base_lo] + base_hi` — the team-kit
  recolour.
  Then `FUN_000CE980` @0xce980 copies the 256 bytes into `0x14720`.
* `FUN_00046f80(base)` @0x46f80 partitions one caller-provided buffer into
  256-byte sub-buffers and stores their pointers: `0x4BF60[0..0x16]`
  (23 tables), `0x4BF34[0..6]` (7), `0x4BB00[0..7]` (8), plus nine named
  single tables (`0x4BF50/0x4BF20/0x4BF54/0x4BF2C/0x4BF28/0x4BF30/0x4BF5C/
  0x4BF58/0x4BF24`), a 256-dword run, and an 8-entry copy into `0x4BE0C`.
* The buffer comes from `FUN_0004a448(name, …)` inside `FUN_00049138`
  @0x49138 (single static caller site 0x4AF0A), i.e. a loaded resource, not
  the 784-byte frame chunk.

The match-data **DAC-side** palette install (the 6-bit buffer `0x14B200`, its
`v << 2` 8-bit copy `0x14B800`, and the `FUN_00048ED8`/`FUN_00048B60` ->
`FUN_00048C8C` chain sourced from slot 0x32 `PALsys.fsh` frame 2) is derived in
**FU-144**; the addresses quoted here are the flat-import forms (image =
flat + 0x100000).

Direct VGA-DAC uploads (`FUN_000CE754`, FU-57) have callers only in the
movie/art paths (`FUN_00068194`, `FUN_0006847c`, `FUN_000A1600`,
`FUN_000A1668`, `0xB2481`) — no sprite-palette DAC site was found.

### Palette consumption (closes the DAC leg)

The palette chunk has its own consumer chain, separate from the per-entity
translation tables:

* `FUN_00047814(frame)` @0x47814 — the game's palette accessor: reads the
  frame tag dword, takes `second_offset = *frame >> 8` (the same u24 as
  bytewise `+1..+3`), and returns `chunk + 16` when `chunk[0] == 0x22`,
  else 0.
* Sub-palette geometry: `FUN_0004808c(x)` / `FUN_00048104(x)` select bank
  frame `(x / 16) * 2` (respectively `* 2 + 1`), call `FUN_00047814`, and
  return `palette + (x % 16) * 0x30` — i.e. the 768-byte palette is 16
  sub-palettes of 16 RGB entries; even/odd frames are paired. Bank
  selection goes through `FUN_000a1918` (frame count) / `FUN_000a1920`
  (frame select) on the resource pointer from `FUN_0004afb8`.
* Install: `FUN_00048b60` loads frame 2 of the current palette bank, copies
  the 768 bytes to the stack, rewrites 10 entries via runtime tables
  `0x70e8`/`0x70f2` (zero statically), appends two small blocks at
  `palette+0xF0` (`0x54` B) and `palette+0x186` (`0x4E` B), and stores
  `FUN_000a154c(table)` in `[0x7104]`. `FUN_00048ed8` copies the palette to
  `0x4B200`, builds a shuffled copy at `0x4B500`, calls
  `FUN_00048c8c(1, [0x7104])`, and sets `[0x68e0] = 1` — the very flag that
  enables the team-kit recolour in `FUN_00048dc0` (§3 above).
* Conversion: `FUN_000a1368(count, rgb6, out)` writes a 1024-byte table of
  4-byte entries in order `{B, G, R, A}`, each component `v * 255 / 63`, with
  `A = 0` only for entry `0xFF` (all others `0xFF`) — the exact 6→8 mapping
  the port now exposes as `fifa96_sprite_palette_to_bgra`.
* Install target: `FUN_000a154c` → `FUN_000a129c`, which allocates two named
  buffers (`0x365C`, `0x3668`), calls `FUN_000a0aa0` and frees; the caller
  handles are `[0x7100]`, `[0x7104]` and `[0x14478]` (the base written by
  `FUN_00048c8c` when `param_2 == 0`).

### Disposition

* **Derived:** the type-`0x22` chunk is a 256-entry 6-bit RGB palette; the
  palette install chain above; the exact `FUN_000a1368` conversion; the
  sprite-index translation path through `0x14720` installed from per-entity
  tables built by `FUN_00046f80` from a loaded resource.
* **Open:** the population of the shuffle tables `0x70e8`/`0x70f2` (zero
  statically); `FUN_000a0aa0` internals (the final DAC/shade stage); the
  chunk `+6/+8/+0xA` header fields are unread by the derived consumers;
  entity-table resource identity (`0x34e8` name is runtime-built) stays open.

## 4. Port and runner

* `fifa96_sprite_chunk_parse` / `fifa96_sprite_chunk_palette` /
  `fifa96_sprite_palette_to_rgb` / `fifa96_sprite_palette_to_bgra` are the
  ported API (header `fifa96_sprite.h`); `tests/test_sprite_palette.c` covers
  the chunk layout, a real-size 784-byte palette, truncation, the player
  `0x7C` rejection, the 6→8 scaling boundaries, and the BGRA order with the
  index-255 alpha exception.
* Runner (`tools/fifa96_play.c`): sprite mode now resolves the palette
  `--palette FILE` > bank chunk (`source=bank`) > grayscale (`source=gray`),
  prints `sprite palette: source=…` with `--print-summary`, and adds
  `--dump-bank PATH` / `--dump-palette PATH` (768-byte 8-bit RGB, ready to
  feed `--palette`). `tests/test_play.py` builds synthetic palette/player
  banks and pins the exact PPM colours, the dump contents, override
  precedence, and the non-palette chunk path.

Retail verification (read-only): `PALteam.fsh` renders with
`source=bank`; its dumped palette starts `81 CE FF 79 C2 EE 6D B2 DE 61 A5 CE`
(8-bit) from the 6-bit `20 33 3F 1E 30 3B 1B 2C 37 18 29 33`.

## 5. Citations

| Claim | Evidence |
|---|---|
| Frame layout, `second_offset`, player 24-byte chunk | FU-86 §4.2, re-read here |
| `0x14720` translation, translated-`0xFF` transparency | FU-86 §5 (`0xCEAF1`) |
| Per-entity tables and kit recolour | `decompile 0x48dc0` (function body) |
| Table carving | `decompile 0x46f80` |
| Resource load | `decompile 0x49138`, xref to 0x46f80 (0x49160) |
| DAC callers | `get_xrefs_to 0xce754` (5 callers), FU-57 |
| Chunk bytes | ISO extraction 0x338800 → refpack → BIGF → `PALteam.fsh`/`PALsys.fsh` frame headers and chunks (dump via `fifa96_play sprite --dump-bank`) |
| Palette accessor / sub-palettes | `decompile 0x47814`, `0x4808c`, `0x48104` |
| Install chain / kit gate | `decompile 0x48b60`, `0x48ed8`, `0x48c8c` |
| 6→8 BGRA conversion | `decompile 0xa1368`; install `0xa154c`/`0xa129c` |

## Provenance

Session: `fifa96_play sprite /tmp/opencode/art/GAMEART0.PVI --name PALteam.fsh
--dump-bank … --dump-palette …`; ISO extraction at byte 0x338800; Ghidra MCP
`decompile_function 0x48dc0/0x49138/0x46f80/0xce980`,
`get_xrefs_to 0x4bf60/0x46f80/0xce754`. Retail sha256
`d57c5f50818b7111…`.
