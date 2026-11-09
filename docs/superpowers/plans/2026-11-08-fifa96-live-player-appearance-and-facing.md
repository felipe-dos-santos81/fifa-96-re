# FIFA 96 — live player skin, hair and camera-relative facing (Phase 84)

Goal: address the live gameplay report "the soccer players physics, walking,
kicking and skin color are bad". The investigation separated four observations:

1. **Skin/hair colour.** The players' faces and hands were green/magenta
   because the live identity translation drew the skin source indices through
   the PALteam kit colours.
2. **Walking/facing.** The sprites did not turn with the camera: the native
   `0xA2A10` view/position angle addend was an open leg, so the direction fold
   used only the record facing.
3. **Kicking.** The ball launch is the ported `FUN_0008BAF0` / `FUN_0007B9C4`
   path; the vertical ball staging (`0x7A490`) remains unported, so the live
   flight is ground-plane only. No kick animation is installed by the native
   row-07 body either (a capstone scan of `0x814B0..0x81737` finds no
   `CALL 0x6E598`), so the kicker stays on its locomotion row in the original
   row too.
4. **Physics.** The shared mover and FU-172 speed/turn tables are already the
   native port; the live pad target is the native `0x79C20` step
   `pos + dir*0x80`.

## Evidence

### Player appearance — `FUN_00047298`

`FUN_00047298` (`0x47298..0x47813`) is the per-record palette-table builder:
it copies a five-record appearance template from `0x106962`, applies the
pattern groups, then for each of 23 slots writes a 256-byte translation table
into the pool `0x14BF60[slot]`. The table maps the player sprite's **source**
indices to the PALsys frame-0 palette indices. The outfield template's computed
destination set (first-hand Ghidra `read_memory` of the static tables at
`0x1068E4`, `0x106A1C`, `0x106B00`, `0x106B70`, `0x106C00`, `0x107080`,
`0x1070D4`, `0x1070E4` plus the `0x47298` disassembly) is:

```
0x75 -> 0xF7   0x76 -> 0xF9   0x8F -> 0x00   0x90 -> 0x00
0x9C -> 0xFD   0x9D -> 0xC0
0xD7..0xDA -> 0x00 (skin-group 0; the retail roster attributes decode to 0)
0xDB..0xE3 -> 0xF6..0xFE (the hair/skin ramp base 0xF6)
```

The native pool (and the per-player variation) remains open; the live fix
applies this appearance subset to both team translations, so every player keeps
the staged kit colours and reaches the raw PALsys skin/hair entries. The live
palette dedicates `0xFE` to the pitch marking, so the ramp's last step is
clamped to `0xFD`.

The full `FUN_00047298` port needs the runtime PCCD match-record pointers and
the five-template pattern pass; it is recorded as an open leg rather than
faked. The static table reads and the algorithm above are the evidence for the
subset.

### Camera-relative facing — `0xA2A10`

`FUN_00057158` stages the direction at `0x57187..0x571DF`:

```
EAX = [0x55BC4 + slot*4] - 0x1000     ; staged angle
ECX = [0x543A8 + slot*0xC]            ; camera-relative clean x
EBX = [0x543A8 + slot*0xC + 8]        ; camera-relative clean z
CALL 0xA2A10                          ; 16-bit atan addend
EAX += [0x55BC4 + slot*4] - 0x1000
EAX &= 0xFFFF; EAX >>= 13
EBX = 7 - EAX; EBX &= 7               ; sprite direction
```

`0xA2A10` (`0xA2A10..0xA2AD5`) is a 16-bit `atan2(x,z)` with `+z = 0` and
`+x = 0x4000` (capstone decode of the 192-byte body; the jump table at
`0xA29F0` supplies the eight quadrant returns). The engine's `fifa96_entity_angle`
is the same 10-bit native `0xCD474` atan, so `angle10 * 0x40` is the 16-bit
addend. The live render now adds it before the `>> 13` fold; the non-live
M1/M2 path keeps the recorded fold byte-exact.

## Implementation

All changes are live-gated (`live_start`); M1/M2 never set it.

1. `match_run_apply_player_remap` applies the native appearance subset to
   `render.remap_team[0]` and `[1]` after the kit-band redirects. The palette
   (kit bands, marking slot) is untouched.
2. `match_run_scene_stage` is split into a per-slot `match_run_animate_entity`
   helper (no behaviour change; it still runs once per granted frame from the
   frame body). This made the frame/facing analysis explicit and keeps the
   animation cadence tied to the 30 Hz simulation.
3. `fifa96_match_run_render` computes `view_addend[slot]` from the
   camera-relative pre-rotation pair and adds it to the staged angle for live
   runs only.

## Verification

- `test_engine_match_render`:
  - `test_live_real_team_kits` asserts the remap subset for both sides and the
    brown skin destination (not the green kit colour);
  - `test_view_addend_cardinals` pins the eight `0xA2A10` quadrant returns
    (`+x 0x4000`, `+z 0`, `-x 0xC000`, `-z 0x8000`, the four diagonals);
  - the existing kit-band and marking-slot assertions still pass.
- `make check` **124/124**.
- M1 `cmp` exact `09b726b7…`; M2 `cmp` exact `41521559…`.
- Live release screenshots (`docs/screens/p84-live-skin-and-facing.png`,
  `p84-live-kick.png`) show brown/dark skin, black hair and the direction
  following the camera-relative position. The native reference remains
  `docs/screens/p80-native-side.png`.

## Open legs

- Full `FUN_00047298` per-player palette pool: the runtime PCCD match-record
  pointers, the five-template pattern pass and the settings gates
  (`FUN_0001CAEC(9/0xD)`).
- Native vertical ball staging `0x7A490` and the row-05 target algebra
  `0x7F3A1..0x7F57B`; the live flight is ground-plane only.
- Native kick animation/pose row (the row-07 body installs no `0x6E598` row).
- `0xA2A10` argument provenance: the live pair is the engine's pre-rotation
  `slot.pos - camera`; the native block is `[0x543A8]` after
  `FUN_000A3110(0x17, ..., -cam, ...)` (evidence above).
