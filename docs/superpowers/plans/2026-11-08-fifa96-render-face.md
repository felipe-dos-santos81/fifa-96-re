# FIFA 96 — render-stage the live record facing (Phase 79)

Goal: stage each pool record's native `+0x7D` facing word into the render slot
so live players draw in the direction they actually face, then re-pin M2 with
the measured hash-only diff if the golden moves.

## Evidence

- Native `FUN_00036C70` stages the record angle as
  `((0x400 - (dword[rec+0x7B] >> 16)) & 0x3FF) << 6`; `>>16` is the `+0x7D`
  face word. `fifa96_render_slot_stage` already performs that fold from
  `heading >> 16`.
- The ball slot already stages `ball.heading << 16`; the 22 player slots leave
  `stage.heading` zero, so every record draws direction 0.
- `face7d` is now live on the pool: the mover/normalizer writes it every
  dispatched record; M2's phase-33/35 provenance records the live face.
- Terminal-row steering is a separate animation-selection machine and does not
  write the face; it is not part of this slice.

## Steps

1. Stage `re->stage.heading = (int32_t)((uint32_t)(uint16_t)e->face7d << 16)`
   in the player-slot loop; update the stale open-leg comment.
   Check: `test_scene_stages_pool_entities` builds and asserts positive and
   wrapped face values.
2. Measure the M2 candidate transcript with the ISO; classify the diff
   (`state=` unchanged, line count unchanged, hash-only) and verify M1 exact.
   Check: `cmp`/`cut`/`grep` evidence; stop if a state or line changes.
3. If the diff is the intended facing move, append the written provenance
   block, re-pin M2, update ENGINE/PROGRESS/README, run `make check` and the
   release smoke. Commit on `main`.
   Check: full gate green, M1 exact, M2 producer matches the new golden.

## Result

The record stage now publishes `face7d` in the slot heading. M2 re-pin is
measured: first differing line 2545, 3,978 contiguous hash-only lines
(2545..6522), all `state=` tags/counts unchanged, 295,089 B / 6,522 lines, old
sha `7c3874f7…` -> new `41521559…`; M1 exact. `make check` 124/124. Terminal
pose/duration steering and the `0xA2A10` view angle remain legs.

## Non-goals

- No terminal-row steering tables/selection (FU172/FU159-L5).
- No native `0xA2A10` view-angle addend (P10-L6 leg).
- No change to the face producer, mover, or animation rows.
