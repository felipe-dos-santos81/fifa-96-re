# FIFA 96 — live ball art, possession follow and off-ball pad movement (Phase 83)

Goal: fix the live gameplay report “the ball is big and transparent and the
players’ physics are bad”. The three observable defects were independent:

1. the retail `ball.fsh` lost its black outline (green pitch showed through);
2. the ball used the entity FU-85 depth scale and a centred pivot, so it grew
   as it approached the camera;
3. the human controlled record was forced into the row-04 AI chase body, whose
   native user arm builds a camera target, so held arrows never reached the
   mover; and the carried ball had no possession follow at all.

## Evidence

### Ball transparency

`port/tests/golden/gameart0.pvi` (`fifa96_play sprite --name ball.fsh`)
decodes the 8-frame 16×16 bank. Palette index 0 is the opaque black outline;
index 0xFF is the transparent background. The generic FU-85 remap
(`render.remap[i] = i == 0 ? 0xFF : i`) keys index 0 out, so every outline
pixel was skipped and the pitch showed through the ball. The live screenshot
before the fix (`/tmp/opencode/fifa-live/07-kick-zoom.png`, session-local)
showed the green holes.

### Ball scale and pivot

Ghidra `/FIFA96.EXE`:

- `FUN_000572E8` (`0x5736a..0x5737c`) culls on
  `2 * [0x109000] <= depth`; `read_memory 0x109000` = `16 00 00 00` = 22, so
  this is a near-distance threshold, **not** a scale. The prior FU-158
  L158.5 wording (“the presenter’s own scale cell”) is corrected here.
- `FUN_00057278` rewrites the frame header before `FUN_0009B664`:
  `pivot_x = dword[frame+2] >> 17` (width/2 for a zero second-offset) and
  `pivot_y = word[frame+6]` (height). The frame is blitted 1:1 with a
  bottom-centre anchor; no depth-scale term exists.
- Native 640×400 capture
  `data/captures/session-probe-14c18-mode/frames/fifa96_005-0053.png` shows
  the same 16×16 ball footprint, not a depth-scaled sprite.

The port drew slot 22 through the FU-85 entity scale (the synthetic fixture
measured ~11 px for an 8 px frame; the live ball grew toward the camera) and
kept the parsed pivot, so the ball floated/overlapped differently.

### Player movement

- The outfield machine’s forced action (`fifa96_outfield_forced_action`)
  installs code 4 on the controlled team target when the ball is loose.
- The native row-04 user gate `0x7E9F6..0x7EA16` sets `ECX=1` for the
  controlled record; `0x7EB95` then builds the camera-chase target. The AI /
  non-self path uses the slot-dir step `pos + dir*0x80` at
  `0x7EBF8..0x7EC13`. The port’s single `entities.controlled` mapping made the
  human take the camera branch, so held arrows never moved the controlled
  record.
- The native hold/move row 0x03 body is ported
  (`fifa96_match_action_03`) but its action-table slot is deliberately NULL:
  wiring it in the shared table moved the M2 tape at frame 29. The live path
  therefore had no row-03 mover when the forced action did not install 4.
- The ball machine (`FUN_0008BAF0`) only has the loose camera-relative target;
  the native possession staging (`0x7A490`, the row-05
  `0x7F3A1..0x7F57B` target algebra) is unported. A carried ball stayed at
  the kickoff spot while the player ran away.

## Implementation [D]

All changes are live-gated; the M1/M2 tapes never set `live_start`.

1. `render.ball_remap[256]` is the identity translation. The live ball draw
   selects it (index 0 stays opaque); the generic `remap` still keys index 0
   for the player sprites. Non-live keeps the existing remap.
2. The live ball draw uses the native 1:1 scale `0x10000` and rewrites the
   parsed pivot to `(width/2, height)`. Non-live keeps the FU-85 scale/pivot.
3. Row 0x04 gets a live pad override: when the bound FU-70 slot owns the
   record and a direction is held, the target is the native slot-dir step
   `fifa96_action_move_target(pos, dir)` instead of the row’s camera target.
4. `fifa96_match_dispatch_action` dispatches the ported row 0x03 body for the
   live bound slot with a held direction; the shared action table remains NULL
   and the census is unchanged.
5. The ball machine gets a live possession glue: when a valid carrier is
   within 0x80 of the ball in phase 2 with no live flight, the ball is pinned
   to the carrier’s feet plus `dir * 0x20`, with the carrier velocity/speed
   copied for the roll cadence. A far/stale carrier is left to the loose ball
   machine, and the save-hold block returns before this so keeper pins are
   untouched. This is the labelled stand-in for the unported `0x7A490` /
   `0x7F3A1` staging.

## Tests and gate

- `test_engine_match_render` adds
  `test_live_ball_art_is_opaque_and_fixed_size`: palette index 0 must stay
  opaque, the 8×8 synthetic frame must draw 8 px wide, and the bottom anchor
  must put it above the projected ground point.
- `test_engine_live_control` adds `test_live_offball_slot_player_moves` (the
  live row-03 direct dispatch plus row-04 pad movement; non-live row 03 stays
  `-FIFA96_ERR_UNSUPPORTED`) and tightens the carrier dribble test to require
  the ball within ±0x40 of the player.
- `make check` **124/124**; M1 byte-exact `09b726b7…`; M2 byte-exact
  `41521559…` (both `cmp`-verified). The README title and census are
  unchanged: the row-03 body was already ported and the table slot stays NULL.
- Release smoke: `docs/screens/p83-live-ball-and-movement.png` (ball opaque,
  correct size, at the carrier’s feet while running) and
  `docs/screens/p83-ball-crop.png` (8× crop).

## Legs

- L158.5 corrected: `[0x109000]` is the near-cull threshold (22); the native
  ball blit is 1:1 with the bottom-centre pivot. The trail/secondary draws
  (L158.3) remain open.
- The exact native possession staging `0x7A490` / row-05 target algebra
  `0x7F3A1..0x7F57B` remains unported; the live glue and offset are derived.
- The row-04 camera/user branch is still the native body; only the live bound
  pad target is overridden. Full native human/AI selection handling remains
  part of the wider AI leg.
- Native class-3 camera integrator/director, 3dwpts stadium and HUD art remain
  the existing P10 legs.
