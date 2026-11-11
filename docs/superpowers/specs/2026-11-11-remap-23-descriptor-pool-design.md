# 23-Descriptor Remap Pool — Design

## Intent and why

`FUN_00047298` is the native per-player palette-translation builder. The port
currently approximates it with a per-pitch table built from hardcoded kit
bands plus PCCD skin/hair classes; it covers the 11 visible players per side
but not the native 23-block pool, the template/pattern composition, the
home/away blanking gates, or the extra block. This slice replaces the
approximation with a faithful builder so the appearance state matches the
native pass.

This is **project A** of the "A + B + C" closure request. B (the parked stats
legs) and C (the parked replay legs) get their own designs after A.

## Evidence (fresh Ghidra, `/FIFA96.EXE`)

> **Note:** the review errata at the end of this document supersede several
> bullets below (five static templates, 23 outputs, class-4 extra, 28 map
> pointers, and the native class/table pairing). Read the errata with this
> section.

`decompile_function 0x47298` and the table reads:

- The function first copies **23** `0x8E`-byte template blocks
  (`FUN_000CD390`) into a local pool, then builds **23** 256-byte output
  blocks in a `0x17`-iteration loop (`FUN_000CD390(0x100, ...)` each).
- Five **template blocks** are composed from the selector
  `0x107080 = {0, 0, 14, 15, 13}` and the template rows at `0x106C00`
  (stride `0x30`, four pattern entries at `+0`, `+0xC`, `+0x18`, `+0x24`),
  using the band table `0x106B70` (six entries of
  `{dest, len=11, source-group}`, dests `37/48/59/70/81/92`) and the source
  map at `0x1068E4` (0x8E bytes).
- Each output block copies `0x7E` template bytes through `0x1068E4`, then
  writes the hair ramp from `0x1070D4` (four 4-entry classes) at `0xD7..0xDA`
  and the skin base from `0x1070E4` (four bases `0xF6/0xED/0xE4/0xDB`) at
  `0xDB..0xE3` and the template `+6..+0xE`; skin class 0 clears `0xE3` and
  template `+0xE`.
- `FUN_0001CAEC(9)` (home) and `FUN_0001CAEC(0xD)` (away) blank the skin/hair
  writes to `0xFF`.
- The template-`2` rows run a second pattern pass through
  `PTR_DAT_0x106B08` and the same source map; templates above `0x15` add the
  two special entries at `0x106AFD..0x106AFF`.
- The loop index covers **11 home + 11 away + one extra** descriptor. The
  extra block's consumer is not proved; the fresh evidence suggests the
  referee (the 23rd person on the pitch), but the design records it as a
  numbered leg until a consumer is found.
- **Correction (builder check).** `FUN_0004BF04` is only
  `[0x1074B8] = param_1`, not a defaults provider. The extra descriptor's
  template class is the hardcoded `3` written after the second
  `FUN_0004BF04(1)` call; its hair/skin come from the native loop's 23rd
  entries, whose provenance is not proved. The builder transcribes that
  native extra block; the provenance and consumer stay a numbered leg.
- **Correction (builder check).** The band table at `0x106B70` has **12**
  entries (`{dest, len, source-group}` = 37/48/59/70/81/92/101/110/119/15/19/26
  with lengths 11/11/11/11/11/9/9/9/7/4/7/11 and groups 0/0/0/0/0/2/2/2/1/3/1/0),
  and each template row at `0x106C00` carries **12** pattern ints (stride
  `0x30`), not four. The transcription is larger than "small": all 12 bands
  and the full template rows are needed.

## Goals

- Build the native 23-block pool from the static image tables, not the
  hardcoded band approximation.
- Keep the 11 visible home and 11 visible away blocks feeding
  `render.remap_entity[side][pitch]` exactly as the renderer expects.
- Store the extra block in a dedicated destination and leave its consumer a
  numbered leg unless the evidence pass proves one.
- Preserve the substitution rebuild: the pool follows the current
  `sub_lineup`/`roster_order`.
- Keep the live-only gate; M1/M2 never run this path.

## Non-goals

- The native `0x14C360` desired-order producer and the in-match order swaps.
- The 23rd block's consumer (a leg if unproved).
- The `FUN_0008ECC4` stats counter, the `side_swap` producer and the replay
  ring (projects B/C).

## Anti-goals

- Filling `0x8E..0xFF` with identity bytes or collapsing the output to `0x8E`.
- Inventing a 23rd consumer (referee, ball, crowd) without evidence.
- Changing the live palette/kit selection path or the renderer's remap
  contract.

## Constraints

- `make check` 124/124; M1/M2 byte-exact; no golden re-pin.
- No completion-number movement (no new body/row; this is a live render
  internal).
- The existing `remap_team[2][256]` stays for the non-entity translation path;
  the entity pool is built by the new native builder.
- The static tables are transcribed into C arrays with their image addresses
  cited; the engine does not read the EXE at runtime.
- The `FUN_0001CAEC(9/0xD)` gates read the existing options block; if the
  engine has no option cell, stage them 0 (normal match) and record the leg.

## Approaches

1. **Full native table builder (chosen).** A small
   `fifa96_match_remap_native` module transcribes the static tables and
   builds the 23 blocks. Faithful, closes the pool, and isolates the data from
   the renderer. Cost: a table-transcription pass and a new module.
2. **Incremental table pass.** Replace the hardcoded bands with the native
   band/source tables but keep the per-player overlay. Smaller, but the extra
   block and the template composition stay open.
3. **Visible-22 only.** Port the template pass for the 11+11 visible players
   and defer the extra block. Leaves the pool at 22/23.

Approach 1 was chosen because the item is the **23**-descriptor pool and the
tables are small and static.

## The how

### Module and interfaces

- New `port/src/fifa96_loader/fifa96_match_remap_native.h/.c` (loader layer,
  pure data + builder) or the engine layer if the tables are engine-owned.
  Decision: **loader layer**, matching the existing `fifa96_*` pure helpers.
- `void fifa96_match_remap_build(uint8_t pool[23][256],
  const uint8_t hair_code[2][11], const uint8_t skin_code[2][11],
  uint8_t home_blank, uint8_t away_blank);`
  - `hair_code`/`skin_code` are the PCCD ASCII digits for the 11 slots per
    side.
  - `home_blank`/`away_blank` are the `FUN_0001CAEC(9/0xD)` gates.
  - The pool order is home 0..10, away 0..10, extra 22.
- The builder is **native-faithful**: its 256-byte blocks keep the native
  byte 0 = `0x00` (the native sprite convention).
- The engine keeps `remap_entity[2][11][256]` as the renderer contract. When
  it copies the first 22 pool blocks into it, it forces **source-0 →
  `0xFF`**, the engine's current PLAYART colour key (the renderer skips only
  `0xFF`). This is a documented render adapter, not a native change.
- The extra block lives in `render.remap_extra[256]`, copied native
  (byte 0 = `0x00`).
- The native per-sprite palette install `FUN_00048DC0` and its 0xFF-transparent
  convention stay the FU-85 §5 leg; until it is ported, the adapter is the
  correct render bridge. The native block and the render copy are both
  asserted so the divergence is visible.

### Data tables

Transcribe with the image addresses in comments:
- `0x106C00` template rows (stride `0x30`, **12** pattern ints per row; the
  selector uses rows `0/13/14/15`; transcribe all rows the selector
  addresses).
- `0x106B70` the **12** band entries `{dest, len, source-group}` (the
  builder-check correction above).
- `0x106B00` 16 source-map pointers + the 7-entry maps at their targets.
- `0x1068E4` the 0x8E source map.
- `0x1070D4` four hair ramps; `0x1070E4` four skin bases.
- `0x107080` the five template selectors.

The existing `match_run_hair_class`/`match_run_skin_class` digit maps move to
the new module (or stay and are passed in already mapped).

### Builder algorithm

1. Compose the five template blocks from the selector, the template rows, the
   band table and the source maps (the `0x106C00`/`0x106B70`/`0x106B00` pass).
2. For each of the 23 descriptors:
   - select the template block by the descriptor's template class;
   - copy the 0x7E template bytes through `0x1068E4` into the 256-byte output;
   - write the hair ramp at `0xD7..0xDA` and the skin base/ramp at
     `0xDB..0xE3` plus the template `+6..+0xE`;
   - clear `0xE3`/template `+0xE` for skin class 0;
   - apply the home/away blanking gates;
   - run the template-2 second pattern pass;
   - add the template > 0x15 special entries;
   - store the 256-byte block.
3. The 23rd descriptor uses the native loop's extra entries (template class
   `3` and that loop iteration's hair/skin values) and is stored in
   `remap_extra`; its provenance and consumer are a numbered leg.

### Integration

- `match_run_stage_live_kit` builds the pool from the current roster codes
  and gates, copies the first 22 into `remap_entity`, and stores the extra.
- `match_run_rebuild_entity_remaps` rebuilds the pool after a substitution.
- The renderer keeps selecting `remap_entity[side][pitch]`; the extra block
  has no draw path until its consumer is proved.

### Tests

- New `test_remap_native` (or `test_engine_match_render` fixture) asserts:
  - the five template blocks and the 22 visible outputs against the native
    table values for known PCCD classes;
  - the native block byte 0 = `0x00` and the render copy byte 0 = `0xFF`
    (the documented adapter);
  - the hair `0xD7..0xDA` and skin `0xDB..0xE3` bytes for classes 0/3;
  - the home/away blanking turns the skin/hair bytes to `0xFF`;
  - the extra block is present, stable, and keeps byte 0 = `0x00`;
  - the substitution rebuild changes only the incoming player's block.
- Live render fixture: the 22 remaps still draw; M1/M2 byte-exact; smoke.
- No golden re-pin.

## What is left to the builder

- The exact C transcription of the 12 bands, the 12-pattern template rows
  and the source maps.
- The 23rd descriptor's native provenance values and the consumer leg
  number.
- The option-cell mapping for `FUN_0001CAEC(9/0xD)`; stage 0 if absent.
- The module split (loader vs engine) if the engine's render struct is a
  better home for the pool.

## Review errata (fresh Ghidra, 2026-11-11)

The evidence bullets above are superseded by this fresh pass. The prior
sections are left intact per the repo's errata rule. Addresses are from
`/FIFA96.EXE`; the reads are `disassemble_function 0x47298`,
`decompile_function 0x4C00C`/`0x4C098`, and `read_memory` on the cited
tables.

1. **Five static base templates, not 23.** `0x472B8..0x472D3` copies the
   `0x106962` block (0x8E bytes) five times into the local pool (0x2C6
   bytes). The `0x17`-iteration loop at `0x47578` builds the 23 output
   blocks. The first draft's "copies 23 0x8E-byte template blocks" is wrong.
2. **The 23 descriptors.** The class array at `[ESP+0x480]` is home 0 = 2
   (GK), home 1..10 = 0, away 0 = 3 (GK), away 1..10 = 1, extra = 4
   (`0x473F5..0x47435`). `0x107080 = {0,0,14,15,13}` maps class 0/1 -> row 0,
   class 2 -> row 14, class 3 -> row 15, class 4 -> row 13. The extra is
   **class 4**, not class 3.
3. **Class/table pairing.** `FUN_0004C00C` reads
   `*(int*)(entry+0x1A)>>24` = byte `entry+0x1D` = PCCD `+0x1C` (the port's
   `roster_hair_code`). `FUN_0004C098` reads `*(int*)(entry+0x1B)>>24` =
   byte `entry+0x1E` = PCCD `+0x1D` (the port's `roster_skin_code`). In
   `FUN_00047298`, the `0x4C098` class indexes `0x1070D4` (the hair ramp)
   and the `0x4C00C` class indexes `0x1070E4` (the skin base) and controls
   the class-0 `0xE3` clear (`0x475DB`, `0x475E2`, `0x47622`). The port's
   current builder pairs them the other way; the native builder must follow
   the native pairing. "hair"/"skin" are the port's PCCD field labels, not
   the native table roles.
4. **Hair class 2.** `0x1070D4` is
   `00 00 00 00 EB EC FC FD F0 F1 F2 43 D7 D8 D9 DA`; the last class-2 byte
   is `0x43`, not the legacy helper's `0xF3`.
5. **Extra descriptor.** Template class 4 (row 13), skin class 2, hair class
   2 (`0x47412..0x47435`). Its pool slot 22 has no xref
   (`get_xrefs_to 0x14BF9C` = 0); the consumer remains a numbered leg.
6. **Source maps.** `0x106B00` is 28 pointers (4 groups x 7 patterns) into
   the shared `0x106A1C..0x106B00` region (0xE4 bytes), not 16 pointers with
   seven-entry maps. The source index is the map's fixed first byte plus the
   advancing offset: `template[map[off] + map[off+k+1]]` for k=0..len-1.
   This is base+offset, not a sliding `map[k]+map[k+1]`.
7. **Second pass.** For each band whose row entry is 2, the first pass
   zeroes `template[dest]`; the second pass (`0x476B8..0x4775C`) uses map
   index 2 (`0x106B08` = group*7+2) and writes
   `out[0x1068E4[dest+k]] = template[map1[off1] + map1[off1+k+1]]`.
8. **Special branch.** The `0x107080[class] > 0x15` guard at `0x47762` is
   unreachable for classes 0..4 (rows 0/0/14/15/13). It is not an exercised
   path; keep it as a comment or a numbered leg.
9. **Team kit overlay.** The native block's kit bands are default shade
   bytes (`0xBD/0xBC` for the class-0 shirt bands; the shorts/torso/socks
   bands are zero), not the live team destinations
   `37/48/59/70/81/92`. The port's `remap_team` must still be applied over
   the six kit source bands after the native block is copied; dropping it
   replaces the live kit colors with shades. Apply it to all 22 visible
   blocks, including the GK blocks, to preserve the current kit behaviour.
   The native group-2 rows map `0x8A..0x92` as sock-family sources while the
   overlay keeps the port's shirt assignment; that band-group difference and
   the GK-specific `FUN_00048DC0` shade patch (pool blocks 0/0xB) remain
   legs.
10. **`FUN_0004BF04`.** It stores `[0x1074B8]`, which selects the roster
    source (`0` = `0x14C360` desired, `1` = `0x157BD2` current). It is not a
    defaults provider. The extra classes are hardcoded after the second
    call.
11. **Builder interface.** The builder takes the native classes, not the
    PCCD digits:
    `fifa96_match_remap_build(pool, class_a, class_b, home_blank, away_blank)`,
    where `class_a` is the `0x4C00C` class (the engine's
    `match_run_hair_class(roster_hair_code)`) and `class_b` is the
    `0x4C098` class (the engine's
    `match_run_skin_class(roster_skin_code, class_a)`). The extra uses
    class_a 2 / class_b 2.
12. **Corrected engine-test bytes** (class_a = `0x4C00C`, class_b =
    `0x4C098`):
    - roster 0 (`roster_hair_code` '4' -> class_a 0, `roster_skin_code`
      '3' -> class_b 2): `[0xD7..0xDA] = F0 F1 F2 43`,
      `[0xDB..0xE3] = F6..FD, 0`.
    - roster 5 ('4' / '1'): `D7 D8 D9 DA`, `F6..FD, 0`.
    - unassigned default ('0' / '4'): `00 00 00 00`, `DB..E3`.
    - Brazil pitch 0 (hair '1' / skin '2'): `EB EC FC FD`, `E4..EC`.
    - Brazil pitch 1 (hair '2' / skin default '4'): `00 00 00 00`,
      `ED..F5`.
    - Germany pitch 2 (hair '0' / skin '1'): `D7..DA`, `DB..E3`.
    - Germany pitch 4 (hair '0' / skin '2'): `EB EC FC FD`, `DB..E3`.
    The old `[0x75]` assertions (`0xE8`/`0xDF`/`0xF1`) are retired. The native
    `0x75` is the second-pass skin byte (`FA/FB` for class 0, `E8/E9` for the
    extra); the module test pins it, the engine tests do not.
13. **Template bytes for the five classes** (independent of the player
    classes): home GK `[0x50] = 0xC0`, `[0x52] = 0xC0`; home outfield
    `[0x50] = 0xBD`, `[0x52] = 0xBC`, `[0xC2..0xC4] = C0 BE BB`,
    `[0x4E] = 0xBB`, `[0x75..0x76] = FA FB`; away GK `[0x50] = 0x00`;
    away outfield `[0x50] = 0xBD`; extra `[0x50] = 0x00`,
    `[0x51] = 0x00`, `[0x75..0x76] = E8 E9`,
    `[0xD7..0xDA] = F0 F1 F2 43`, `[0xDB..0xE3] = E4..EC`.
    The class-0 shorts/torso/socks bands are native zeros; the team-kit
    overlay fills them.
