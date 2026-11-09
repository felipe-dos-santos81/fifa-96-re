# FIFA 96 — live kit/skin correction, pass reach and off-ball render focus (Phase 85)

Goal: continue the live gameplay report "the soccer players physics, walking,
kicking and skin color are bad". Phase 84 landed the camera-relative facing and
a first skin subset, but the live capture still showed striped shirts, a yellow
away team and a pass that did not leave the kicker's feet. This phase re-derived
the appearance source bands from the actual PLAYART bank, fixed the team-1 kit
translation composition, and separated the render focus from the simulation
camera.

## Evidence

### 1. The player sprite source bands (PLAYART `walk.fsh`)

The Phase 84 subset treated `0x75/0x76/0x8F/0x90/0x9C/0x9D` as skin. The staged
PLAYART entry 1 (`walk.fsh`, 60 frames) is a raw SHPI bank; its frame-0 pixel
histogram (14,000+ bytes over the 19x44 walk frames) uses:

```
0x4E            eye/collar pixel
0x50..0x65      shirt sleeves/shoulders
0x66..0x70      sock trim
0x71..0x89      bare arms/legs (skin)
0x8A..0x9D      shirt side panels
0x9E..0xA8      sock trim
0xC2..0xC4      collar/chest trim
0xC5..0xCB      shorts
0xCC..0xD6      shirt torso (0xD0..0xD6 is the selected shirt band)
0xD7..0xDA      hair
0xDB..0xE3      face/leg skin (the native 9-entry skin ramp)
0xE0..0xE5      sock/boot band (0xE4/0xE5 are the boot pixels)
0xFF            transparent
```

This is the source-band model now used by `match_run_apply_player_remap`.
`0x8F/0x90/0x9C/0x9D` are shirt/trim indices, not skin; mapping them to the
skin ramp produced the Phase 84 stripes. `0xD7..0xDA` is the 4-entry hair ramp
(the native `DAT_001070D4` set) and `0xDB..0xE3` is the 9-entry skin ramp
(`DAT_001070E4`), consistent with `FUN_00047298`.

### 2. The team-1 kit redirect is a value translation, not a source rewrite

The renderer performs **one** table lookup per sprite byte. Mapping source
`0x50` to palette index `208` and separately setting `remap[208] = 239` does
not redirect source `0x50`; it only redirects source `208`. The away kit
therefore kept the home palette (both teams rendered Brazil yellow). The fix
composes the away redirect over the appearance table: every value in
`195..203` becomes `230..238` and every value in `208..214` becomes `239..245`
after the semantic source mapping. The direct source-index rewrite is removed.

### 3. Pass reach

The Phase 84 derived PASS/LONG_BALL flight used
`target = kicker + 2*(dir_x, dir_z)`. The ball machine ends a flight when it
arrives within 0x20 of the target, so the target was reached on the first step
and the ball stayed at the kicker's feet. The FU-73 pair vector is
speed-scaled; the live flight now normalises it and projects it to
`speed * 0x60` units, clamped to the native pitch bounds. The KICK row keeps
the P9 goal-mouth aim.

### 4. Off-ball render focus

The live camera followed the ball. When the opponent (or a loose ball) owned
the play, the control follow selected the nearest own outfielder but the
camera kept staring at the opponent, leaving the human record off-canvas. A
new **render-only** `render_focus_x/z` blends halfway toward the controlled
record while the human side does not carry. The simulation camera
(`camera.pos_*`) and the ball machine keep the ball focus, so shot/keeper
balance is unchanged.

## Implementation

All live-only (`live_start`); M1/M2 never set it.

- `match_run_apply_player_remap` now maps the source bands above: shirt
  `0x50..0x65/0x8A..0x9D/0xCC..0xCF -> 208..214`, socks
  `0x66..0x70/0x9E..0xA8 -> 224..229`, skin `0x71..0x89/0xDB..0xE3 ->
  0xF6..0xFD`, hair `0xD7..0xDA -> 0xD7..0xDA`, collar `0xC2..0xC4 -> 0xFE`,
  eye `0x4E -> 0x00`. The shorts and sock/boot identity bands stay in place.
- The live palette stages the PALsys dark ramp into `0xD7..0xDA` for the hair
  remap (the kit overlay would otherwise leave the PALteam yellow ramp there).
- `match_run_apply_away_kit_redirect` composes the away bands over the
  appearance values; it runs after the semantic table and before commit.
- The PASS/LONG_BALL flight target uses the normalised pair direction and the
  speed-scaled reach; the KICK goal-mouth aim is unchanged.
- `render.render_focus_*` is a new render-only focus used by
  `match_run_render_eye_update`; `view_focus_*` and `camera.pos_*` remain the
  ball focus for the ball machine, keeper and armer.

## Verification

- `test_engine_match_render::test_live_real_team_kits` pins the composed home
  (`0x50 -> 208`) and away (`0x50 -> 239`) shirt mappings, the skin/hair band
  values and the white collar slot.
- `test_engine_live_control::test_live_pass_flight_travels` stages a PASS pair
  and asserts the target is more than 0x200 units away and inside the pitch
  bounds (the old fixed x2 target would fail).
- `test_engine_live_control::test_live_follow_keeps_ball_during_pan` keeps the
  human side on the ball so the pan-gate assertion is still the variable under
  test.
- `make check` **124/124**.
- M1 `cmp` exact `09b726b7…`; M2 `cmp` exact `41521559…`.
- Live release screenshots: `docs/screens/p85-live-kits-skin.png` (Brazil
  yellow/blue/green, brown skin, dark hair; Germany white/black/green),
  `docs/screens/p85-pass-flight.png` (the passed ball ahead of the kicker) and
  `docs/screens/p85-offball-focus.png` (the controlled Brazil record and the
  German ball carrier in the same frame). Native reference:
  `docs/screens/p80-native-side.png`.

## Open legs

- Full `FUN_00047298` per-player palette pool: runtime PCCD match-record
  pointers, the five-template pattern pass and the `FUN_0001CAEC(9/0xD)`
  settings gates. The live path still uses one semantic appearance table per
  team, so all players in a team share the same skin/hair ramp.
- Native vertical ball staging `0x7A490` and the row-05 target algebra
  `0x7F3A1..0x7F57B`; the live flight remains ground-plane only.
- Native kick pose/animation row: the row-07 body installs no `0x6E598` row,
  and the ball-pairing tail's `out.anim` consumer is OL-52.
- Full class-3 camera integrator/director (P10-L6); the off-ball focus blend
  is a derived presentation stand-in.
- Native 3dwpts stadium (P10-L2) and HUD art (P10-L5).
