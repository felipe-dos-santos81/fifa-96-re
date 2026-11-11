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
