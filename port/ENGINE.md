# Native engine (`fifa96_engine`)

The native engine layer sits on top of the 56 clean-room `fifa96_*` libraries and
turns them into a running game: platform ABI → SDL3/null backends → engine core
(boot, asset table, clock, intro, front-end, match).

Status: **M1 complete headless; M2 match playable, visible and RGB-visible —
78/78 native dispatch bodies ported (100%) — 61/80 dispatch slots wired
(64/80 install-live)
(38 action + the wired phase rows 0x02, 0x0D, 0x10, 0x13, 0x17, 0x18, 0x19, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22
and the per-record phase-17 T3 cohort 0x00/0x03/0x04/0x05/0x06/0x07/0x08/0x09/0x0A/0x0C/0x0F/0x15), the
derived FU-143 phase driver wired into the run
loop (live period end 2 → 0x0C), the derived C3-OL2 score source, live OL-80
animation inputs, the derived kickoff ball placement, the resource-loaded
formation/record placement (OL-T11-8: `352ko.fmt` seated from
`/ART/GAMEART0.PVI` at begin, so player records receive real positions and
draw), the derived kickoff phase-1 entry (OL-84: begin lands
`FIFA96_MATCH_RUN_KICKOFF_PHASE = 1`), the derived `FUN_0008D098` state-1
kickoff arm + wired action row 01 (M2 playable-match Task 2: the derived chain
reaches phase 2 via situation 0xB; **M2 phase-18 T3/T4 — the live situation-0
kickoff cutscene pre-empts the derived chain, but M2 phase-18 T4 registered the
native act-1 route (phase `0x17`) and retired the derived ad-hoc stage-0 model,
so a begun run again reaches phase 2 naturally via situation 1 → act 1 → phase
`0x17` → the kickoff taker → situation 0xB; FU-203**), the derived native
match palette
(OL-T11-6: `PALsys.fsh` frame 2 installed onto the presented surface, so the
indexed draw is RGB under SDL3 — FU-144), the derived match HUD (OL-T11-7:
the Frames.fsh bar + the FNTI-font name/score/period/clock overlay staged by
BIGF name from GAMEART0 and drawn on the indexed canvas — FU-148; M2
full-gameplay P0.2, golden re-pinned for the HUD) and **pad-driven locomotion
for the
controlled record (M2 interactive Task 1 / G1: the derived FU-70 setup slot
bind + `FUN_0007876C` merge attach the human slot to the kickoff taker, row
00's slot-direction target feeds the wired FU-77 shared mover
`FUN_0007BF20` for that record, and the M2 golden is re-pinned for the
resulting movement — first differing line frame 59, 107 hash lines).** The M2-B
acceptance tape **v10** (the phase-10 visible-layer acceptance, T4: the
natural goal reaches the tape — the T3 re-pin, 85 lines; v9 was the
phase-9 live-pad-loop acceptance, T4, v8 was the
phase-8 live-loop acceptance, T5; v7 was the
phase-7 acceptance, P5, v6 the phase-6
full-gameplay acceptance, S5, and v5 the M2
interactive-match G3 close-out; the lineage v4 G4 acceptance / v4.1 pad
locomotion / v4.2 camera place / v5 HUD / v5.1 possession producers is retained
as provenance) is green: it asserts the drawing directly (the formation-placed
records reach the
indexed canvas from the first granted frame; the golden was re-pinned for that
upgrade, T1), the kickoff entry at begin (T2; transcript byte-identical), the
RGB palette at the first match present **and on the drawn canvas** (T1/T4 of
follow-up 4; the golden was re-pinned for the palette — first differing line
frame 6, 160 of 165 lines, state suffixes unchanged), the HUD overlay (P0.2:
golden v5 re-pinned — first differing line frame 6, 160 of 165 lines, state
suffixes unchanged; the frame-6 assertion pins the Frames.fsh bar and the
font staging), the natural phase-1 → 2
chain (T2: the golden stays byte-identical with the kept 0x13/0x14/2 forcing,
whose mismatch evidence is in FU-143 §11.5) and the natural-phase-2 probe (T4:
`run_natural_probe` replays the tape with no directives and lands live phase 2
at step 215 with row 01 dispatched and score 0-0). **The phase-10
visible-layer tasks T1–T4** (the `FUN_0007A084` receive arm + the ball
render path, the L4.1 unbound-record walk + the `0x105278` config
producers, the band-reachable acceptance, this acceptance) landed with the
tape **re-pinned to v10** (85 lines, the first re-pin since the S1 v5.1):
the natural goal runs on the tape (m 47..65, `state=0/1-0` from frame 66)
and the v10 assertion layer pins the staging inputs, the pan/tracked follow
state, the queue and the exact dispatch set; **phase-11 T1 (2026-10-11)
landed the `0x40` long-ball key and the `0x157750` camera-height producer**
(FU-148 §15), and **phase-11 T2 (2026-10-11) landed the `FUN_0008BAF0` ball
machine (L14.8)** — the ball record's open-play position producer (FU-148
§16; the live trace pins the ball slot resolving and drawing at moving
positions, while the on-screen ball waits on the camera place, leg
L14.8.e); **phase-11 T3 (2026-10-11) is the acceptance v11 layer** — the
ball machine pinned live on the tape (the m 42.. phase-2 lead/integration
with exact off-spawn positions, the visual/anim strata) together with the
honest negatives (the ball slot's clean projection not drawable — the
goal-mouth camera puts the ball behind the near-depth gate; no live
height, so no `d == 0x70` boundary and no
visible follow — legs L14.8.e/15.a), the long-ball chain referenced to its
fixtures, and the deferred docs/comment minors swept; no re-pin. **phase-12 T1
(2026-10-11) landed the live record jump producer and the live band-2
crossing** (FU-153, leg 15.a): the FUN_0006E8E8 animation-aux-keyed starter +
the `FUN_0007BF20 0x7C072..0x7C10C` arc over the pool, and the two ISO chains
that put the airborne height-table receive live (`event_param 0xE0`, k=5
draw + even grid) and cross band 2 at the exact `0x70` arc sample with the
follow arm held (`event_param/traj 0x100`); the live camera bootstrap (T1-L3)
and the row-07 stage-2 `+0x44` ack that blocks a second kick in one
possession (T1-L2) stay numbered legs — the `p12-t1-*` smoke is honest (live
motion, no visible follow-cam). **phase-12 T2 (2026-10-11) lands the on-screen
ball — the native camera-0 view seed (L14.8.e, FU-154 §5.1):**
`fifa96_match_run_reset_render` seeds yaw 0xBF9C / pitch 200 from the image
statics (first-hand 0x107560), the phase-2 camera-derived lead now projects
on canvas (live ISO pin: 16/16 kick-roll frames, x' 152 -> 4; the stand-in
was 0/90), and the M2 tape is **re-pinned to v12** (lines 9..85 re-render the
native orientation; the ball slot draws on canvas in the pre-park window;
the goal-mouth-park samples flip to drawable with the lateral bound pinned).
The camera *place/pan* in live play stays the T1/R1 coupling (R2-L4); the
ball-slot sprite content is the new leg T2-L1 (the documented synthetic
per-row frame tables + the staged bank order). M1 unmoved. The smoke
stays honest (T4 re-verified `p12-t4-*`: movement + the ball slot on screen,
no re-centering); the phase-10 T4 close-out entry below carries the whole-range
summary, the re-pin ledger and the leg register. The phase-6 ports S1–S4
landed under the same tape without moving a presented frame: S1 possession/
locomotion (FU-147), S2 goal arming (FU-145), S3 goal consumers (FU-146) and
S4 presentation completion (FU-148 §2–§4) — all fixture-proven, tape-dormant
(no pan origin), with v6 asserting their dormant defaults and the S1 mover/
track state; the phase-6 close-out entry below carries the whole-range
summary, the re-pin ledger and the leg register. The phase-7 ports P1–P4
(FU-149 set pieces/restarts, FU-150 fouls/referee/offside, FU-151 keeper
machines + AI mover, FU-152 presentation residual) landed under the same tape
without moving a presented frame: the row-1E keeper machine **is** tape-
reachable through the m 41 staging (v7 pins the claim take: `+0x9B` and the
0x15774C focus triple), while the set-piece, referee and replay/overlay chains
stay tape-dormant and fixture-proven (v7 asserts the fresh cells at m 41/m 62
and names each dormancy gate); the phase-7 close-out entry below carries the
whole-range summary, the re-pin ledger and the leg register. The phase-8
live-loop tasks T1–T4 (FU-149 §7 taker rows, FU-148 §12 camera live feed,
OL-T4-1 hold/kick, OL-87/88/89 natural goal) landed under the same tape
without moving a presented frame, and **v8** asserts each layer's tape-level
status: the taker rows are wired but armed-path dormant (the set-piece arms
need `goal_armed`, which the never-panning camera never sets), the row-04 pan
origin is wired live but the tape's code-4 rows carry lane 1460+ so no event
fires (the camera's only triple change is the row-1E claim place, not a pan),
the SDL hold policy and the `FUN_0007CA54` pad-kick seam are landed but the
tape's edges land at phase 0x13 where the handler phase gates refuse, and the
goal chain is closed producer-to-writer with the natural end-to-end goal
proven in a fixture (`test_natural_goal_end_to_end`, score 1-0); the phase-8
close-out entry below carries the whole-range summary, the re-pin ledger and
the leg register. The phase-9 live-pad-loop tasks T1–T3 (FU-75 L4.1/L4.2/L4.6
on-screen movement, FU-142 OL-63 live carrier producers, FU-148 §13 pan
origin/auto-camera/tracked-side) landed under the same tape without moving a
presented frame, and **v9** asserts each layer's tape-level status: the
row-02 reset of the second kickoff pick dispatches inside the forced kickoff
window (`dispatched_ok` bit 2) while the live held-key machine path stays
fixture-proven, the carrier row 05 never dispatches (no code-5 record, the
`0x158728..0x15872F` possession block fresh, `ball.carrier` NONE) with the
live claim/kick fixture-proven, and the pan layer stays dormant with T3's
reason (zero range words, no `camera.tracked` bind, `follow_speed` 0, no
stage-5 row-1E event, the probe-0 direct score step keeping the 0xD3 gate
closed) with the tracked-side pick and the natural 0xD3 post fixture-proven;
the phase-9 close-out entry below carries the whole-range summary, the re-pin
ledger and the leg register. The interactive
`make game` smoke re-run on this host (2026-10-10, phase-8 T5) reaches match
start and
shows the **match HUD on screen** (bar + score 0-0 + `00:00` at kickoff in
`docs/screens/p8-v8-match-hud.png`); a KICK burst reaches the row-01 kickoff
release gate, so the live clock starts ticking (`00:02` in
`docs/screens/p8-v8-match-clock.png`) — the natural phase-1 → 2 transition
visible through the HUD. The T5 fresh front-end and kickoff captures are
byte-identical (`cmp`) to the phase-7 P5 shots, so the phase-8 landings moved
no live pixels. The scene sprites stay in the placement pose:
T3 landed the hold policy (OL-T4-1; the SDL backend now presents a held key
as a per-poll state sample) and the T5 re-run smoke holds RIGHT after the
kickoff (HUD clock `00:14`) with the scene band staying
byte-identical (`compare -metric AE` = 0 on the `y < 591` crop; full-frame
AE 1345 = the HUD clock/score board only) — the live slot record is team 0
record 9 carrying action code 02 (`locomotion_restart_target`; the phase-8
state — M2 phase-9 T1 wired row 02, so the record now reaches code 4 and the
mover integrates the pad target, see the T1/T3 smoke paragraphs below), so
no wired row consumed the pad for it (the T3 input-row seam runs for it and
finds no matching pressed row; the T3 null probe `probe_t3.out` records the
code-02 state; see "Interactive smoke" and "Known gaps"). A T5 Z-burst during
live play likewise leaves the scene band at AE 0 (full-frame 201 = the clock):
the pad kick is wired and the possession/carrier producers landed in
phase-9 T2 (headless fixture; the on-screen ball view stays blocked), so
neither the T5 nor the T4 burst staged a scene-facing pair. Kick → score stays blocked
interactively: the S2/S3 goal chain is landed and the pan origin is now wired
from row 04 (T2), but the interactive smoke never reaches a code-4 half-line
event, so the score source is
fixture-proven instead (see "Interactive smoke" and "Known gaps"). The M2 interactive-match plan's
G1 closed (T1, after the review fix round), G2 closed **carried-with-legs**
(T2 place landed, kickoff framing carried; T3 HUD landed under the phase-6
P0.2 wave) and G3 is accepted by T4 here; the M2 visible-match plan's G1/G2
closed **carried-with-legs** (indexed draw and phase-1 entry landed; RGB
palette `OL-T11-6` was carried and is now landed by follow-up 4, and the OL-84
residual is landed by T2) and its G3 accepted earlier (follow-up 4); the
playability-legs plan's G1/G2 closed, G3 carried
and G4 accepted. See
`docs/superpowers/specs/2026-10-06-fifa96-native-engine-port-design.md` (parent),
`docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md` (child),
`docs/superpowers/plans/2026-10-07-fifa96-m2-arms-and-wiring.md` (split
follow-up), `docs/superpowers/plans/2026-10-07-fifa96-m2-playability-legs.md`
(playability close-out),
`docs/superpowers/plans/2026-10-07-fifa96-m2-visible-match.md` (visible-match
close-out), `docs/superpowers/plans/2026-10-07-fifa96-m2-interactive-match.md`
(interactive-match close-out: T1 control, T2 camera place, T3 HUD, T4
acceptance) and `docs/superpowers/plans/2026-10-08-fifa96-m2-full-gameplay.md`
(phase-6 wave: T2 review = P0.1, T3 HUD = P0.2, T4 acceptance = P0.3),
`docs/superpowers/plans/2026-10-08-fifa96-m2-phase7-recon-ahead.md` (Track A
S1–S5 execution + Track B read-only recon-ahead), the phase-9 live-pad-loop
plan `docs/superpowers/plans/2026-10-09-fifa96-m2-phase9-live-pad-loop.md`
(T1 on-screen movement, T2 live carrier, T3 pan origin/auto-camera/tracked
side, T4 acceptance v9 — all landed, tape-dormant, screenshots under
`docs/screens/p9-*`) and
the SDD workspaces under
`.superpowers/sdd/` for the full record (scratch; may be deleted).

## Layout

| Path | Responsibility |
|---|---|
| `port/src/fifa96_engine/fifa96_platform.h` | Frozen platform ABI (video/audio/input/time) |
| `port/src/fifa96_engine/fifa96_platform_null.h` | Headless backend + observability stats |
| `port/src/fifa96_engine/fifa96_engine.h` | Opaque engine API (`create`/`boot`/`step`/`run`/`destroy`) |
| `port/src/fifa96_engine/fifa96_surface.h` | 320×240 indexed surface + palette + Mode-X plane packing |
| `port/src/fifa96_engine/fifa96_asset.h` / `fifa96_cache.h` | ISO mount, path table, on-demand byte cache |
| `port/src/fifa96_engine/fifa96_clock.h` | 100 Hz PIT model driver (`fifa96_pacing_clock` + `fifa96_tick`) |
| `port/src/fifa96_engine/fifa96_intro.h` | TGV intro playback into the surface |
| `port/src/fifa96_engine/fifa96_frontend_run.h` / `fifa96_menu_art.h` | Front-end state machine, key mapping, menu rendering |
| `port/src/fifa96_engine/fifa96_match_bridge.h` | Front-end start classification → `fifa96_match_run_begin` + default art staging |
| `port/src/fifa96_engine/fifa96_match_run.h` | Match lifecycle, 30 Hz frame body, input/control slot, render chain, dispatch observation |
| `port/src/fifa96_engine/fifa96_match_handlers.h` | FU-137 action/phase dispatch tables (45 + 35 rows) and the wired-row seam |
| `port/src/fifa96_engine/fifa96_match_entities.h` | FU-141 entity/ball pool and FU-67 update chain |
| `port/src/fifa96_engine/fifa96_match_phase_machine.h` | FU-142a `FUN_0008CEB8`/`FUN_0008D098` installer arms (0x26/0x28/0x2A) |
| `port/src/fifa96_loader/fifa96_arm_helpers.h` / `fifa96_arm_bodies.h` | Cluster-G record machines (rows 26/27/28/29/2A/2C bodies) |
| `port/src/fifa96_engine/fifa96_keys.h` | Shared engine key codes (1–9) |

## Building and testing

`make check` configures, builds `-Wall -Wextra -Werror`, and runs all 108 CTest
cases — including the `test_engine_*` cases built under ASan/UBSan. No external
dependency is required for this: the `null` backend is the deterministic
regression source of truth (it hashes Mode-X planes + palette and PCM, replays
input tapes, and advances a virtual 10 ms clock). With SDL3 ≥ 3.x installed,
the build also produces the windowed `fifa96` target (`make game`).

## What runs today

- Boot from the real CD image (`data/game/FIFAPCCD96.iso`): ISO mount, asset table,
  loader-spine resource scan.
- Intro playback of `VIDEO/*.TGV` (M1 heuristic: prefers `/VIDEO/VID_INTR.TGV`;
  the true boot-intro choice awaits the underived load-order table). Input skips
  the intro.
- Front-end state machine with key mapping, menu rendering (procedural fallback —
  the retail front-end art asset is unresolved; FU135 erratum) and quit.
- Front-end → match bridge for the panel-confirm start classification (selector
  0), with the derived `/ART/PLAYART.PVI` + `/ART/GAMEART0.PVI` staging pair
  (soft failure without the ISO).
- Match: FU-64 lifecycle, FU-60 30 Hz pace, FU-61 input → FU-70 control slot
  per granted frame, FU-71 camera / FU-90 display / FU-141 entity/ball chain
  (including the OL-80 live `anim_id`/`frame` staging into the FU-84 bank
  chain, the derived kickoff ball placement and the OL-T11-8
  formation/record placement at begin: `352ko.fmt` from `/ART/GAMEART0.PVI`
  seeds both teams' targets before the `FUN_00079B6C` commit; the FU-96 leg-5
  per-record camera place `fifa96_match_entities_camera_place` then snaps the
  non-controlled side's in-ring records onto the 0x180 ring — M2 interactive
  T2), FU-142a
  installer arms in phases 0x13/0x14, the FU-143 phase driver
  (`fifa96_match_run_phase_drive`: the derived class gate + `FUN_0008B9CC`
  period-end chooser, so a live class-1 period end writes phase 0x0C on the
  selector-0 default), deterministic FU-85/88/89 render chain, the derived
  native match palette (OL-T11-6: `fifa96_match_palette_from_bank` +
  `fifa96_match_run_palette_install` install `PALsys.fsh` frame 2's
  kit-remapped/appended chunk as the surface palette before the plane
  conversion — FU-144), period end
  (`resolve` OVER→POST→EXIT) back to the front-end.
  **Pad-driven locomotion (M2 interactive Task 1 / G1):** begin runs the
  derived match-setup slot bind (`fifa96_match_entities_bind_slot`, the
  `FUN_00078824`→`FUN_000785E0`/`FUN_0008DB6C` subset) and consumes the
  state-1 arm's `FUN_0007876C` merge, so the FU-70 slot is attached to the
  kickoff taker; the per-frame dispatch stages the slot's T2/T3 direction
  bytes (`+0x20/+0x21`) into the record and runs the FU-77 shared mover
  `fifa96_action_locomotion_step` (`FUN_0007BF20` blocks A–E, tables
  `0x1104D2`/`0x10F680` pinned first-hand) for the slot-bound record, so a
  held pad direction drives row 00's `FUN_00079C20` target into
  velocity/position (headlessly: the frame fixture and the forced tape; the
  live natural path resets the kickoff taker to the inactive 0x19 code — the
  natural row-04 pad arm stays carried, see the smoke table). Row 01 stage 1
  now takes the native
  `word[slot+6] & 0x70` release gate, so the natural kickoff waits for a
  button press/release exactly as the native (observed live in the T4 smoke).
- Action dispatch (FU-137): **60/80 rows wired** (37 action + the phase rows
  `0x02`/`0x10`/`0x17`/`0x18`/`0x19`/`0x1D`/`0x1E`/`0x1F`/`0x21`/`0x20`/`0x22` and the phase-17 T3 per-record cohort
  `0x00`/`0x03`/`0x04`/`0x05`/`0x06`/`0x07`/`0x08`/`0x09`/`0x0A`/`0x0C`/`0x0F`/`0x15`)
  — `00`, `01`, `02` (M2
  phase-9 T1/FU-75 L4.6: the restart/placement row whose phase-2 install-4
  invoke drives the live held-key movement), `05` (M2 phase-9 T2/FU-142
  OL-63: the carrier row's claim/control/dir/stage landing, so a live code-5
  record claims `[0x158724]`), `04`, `06`,
  `07`, `08`, `0E`, `0F`, `18`, `1E`, `21`, `23` (playability G1 +
  arms-and-wiring G3; `01` is the M2 playable-match Task 2 kickoff taker;
  `0E` is the phase-13 T3 live jump selector — FU-160 §2: the row-23 tackle
  install (`install = 0x0E`) now runs the stage 0/1/2/3 body and installs
  anim row 0x12 on the record, whose f2 aux-0x0E frame fires the ported jump
  arm + BF20 arc; the phase-14 T1 else arm `0x82820..0x82A9E` completes the
  stage 2 body — FU-167/FU-168: the camera-follow/pan arm and the
  `[0x1577CA]` tracked bind), `0B`, `0C` (phase-14 T1/FU-169: the duel body
  and the contact-decision body — the live row-0x5E installer chain),
  `26`, `28`, `2A`
  (cluster G), `1D` (FU-151 P3 close-down) and `10`, `11`, `12`, `13`
  (FU-149 L13/T1: the throw-in/corner/free-kick/penalty taker machines —
  placement/probe/kick/resolution; the taker codes the P1 arms install now
  execute and hand back to phase 2 through the shared situation-0xB entry).
  *(This enumeration is the pre-phase-15 core; the phase-15/16 action rows
  `09`, `0x14`, `0x16`, `0x19`, `0x1B`, `0x1C`, `0x1F` join the wired set in
  the bullets below — the `33 action` count above is the authority.)*
  Row `17` (M2 phase-15 T1/FU-181: the line-up walk `0x84730..0x849AF`) has a
  **ported body but is not counted** — the rows-27/29/2C precedent (a ported
  body with no engine installer stays unwired and uncounted): its only
  producer, act-4 stage 0 `0x89740`, is behind the unported sit-2..4
  formation-order arm `0x8AB08..0x8AB62` (FU-149 L12 / FU181-L1). M2
  **phase-15 T2** (FU-176/FU-179/FU-180) wires the keeper cluster: `19` (the
  keeper-hold body `0x746E4..0x74CB7`; the install producer is live on row-04
  `0x7E866`/`0x7E8C1`), `1B` (the reposition/rush bounded body
  `0x76D28..0x77727`, installed invoke-now by row `0x1F`'s no-slot dive
  `0x76557`) and `1F` (the keeper dive/arm body `0x76380..0x765C2`; installer
  live on the phase-6 penalty arm `0x8D558`). Row `19` is dispatched by the
  tape's row-04 code-0x19 install and is **frame-neutral** there (the body
  writes only `+0x9E`/`[0x157C5D]`/the deepest-keeper tracker at the tape's
  `phase != 2` state, so M2 stays byte-exact — no golden re-pin, the observed
  `M2_WIRED_MASK` just grows); rows `1B`/`1F` are install-live but not reached
  (the phase-6 penalty arm is not on the tape). The unported `0x14E04` sine
  rotation / `0x76B28` steering (row `1B`, FU180-L1/L2/L3) and the row-`19`
  predictor projection (FU179-L2/L3) stay numbered legs. The phase
  table contributes its first honest row: **phase `0x1F` (= act 9 handler
  `0x88F4C`)** — the FU-175 act/phase seam gives `fifa96_match_dispatch_phase`
  a live caller and the sit-0xC act-9 producer (FU-170 §7) reaches the row
  every granted frame. M2 **phase-15 T3** (FU-177/FU-178/FU-180) wires **row
  `09`** (the sequence/steer stage-0 arm `0x80A00..0x80BFA`; the ports derive
  the phase-2 gate, the `0x809F0` arm-0 clear/set branches, the `0x114E04` sine
  folds via `fifa96_entity_sine` and the `0x795A4` multiply, and the anim
  `0x11`/`0x79B58` effects); its installers are live (the pad release edge
  `0x7CFB2` + the wired row-06 RNG arm `0x807C1`), and the row is
  install-live/tape-dormant so M2 stays byte-exact (the tape carries no code-9
  record, so the exact `M2_WIRED_MASK` is unchanged — no golden re-pin). Arms
  1/2/3 (stages 1–3, holding the `0x0C`/`0x0D`
  installs) stay **FU178-L1** (the reached stage is recorded in
  `mr->match09_leg_stage`, never a silent no-op). **Row `0x0D`'s body is ported
  (`fifa96_action_sequence_0d_step`) but the row is uncounted** — its only
  installer, row-09 `0x80F61`, is inside the unported arm 1 (the rows-27/29/2C
  precedent); the slot stays NULL. **Row `03` stays UNSUP** (the FU-177 §7
  T3-R2 fake is rejected; the live phase-2 path calls `[rec+0x1C] = 0x6DCC8`,
  FU177-L1). The T3 carry applies the FU180 errata: the row-0x1B stage-0 latch
  clear writes only `0x157C54/57/58/59/5A/5B` (`0x76E0F`, six stores), leaving
  `0x157C55/56` for the stage-2 rush gate (FU180-L7).
  **Phase-16 T2** (FU-184/FU-185) wires **row `0x14`** (the celebration/scatter
  3-stage body `0x8784C..0x87CBC`: the 5-point
  `fifa96_action_sequence_scatter_celebration` + the 6-slot buffer, the idx
  walk and the stage-2 reset + release through the factored
  `fifa96_match_run_kickoff_entry`; the installer is the live phase-5 goal arm
  `match_run.c:2494`, whose route to the row is proven end-to-end). **Row `0x22`
  is ported but NOT counted** (the rows-27/29/2C precedent): its only installer
  (the wired row-07 `0x816DF` invoke-now) is structurally ungated in-engine —
  FU185-L6 (first-hand) shows the gate's opponent `word[+0x6F]`/`word[+0x7D]`
  angle inputs are unmodeled (OL-65, staged 0), so `fifa96_entity_angle(lane_z,
  0)` always yields `±0x100` and the folded diff against `opp_face_word == 0`
  can never be `< 0x55` (a brute-force sweep confirms `out.opponent_invoke`
  never fires); the body `fifa96_match_action_22` is exported and unit-tested
  directly, ready for the phase when OL-65 lands. The row-07 `opp_type` source
  is corrected to the native `+0x91` code byte (FU185-L1; no engine-level
  discriminator yet — FU185-L6 blocks the gate). Both landed rows are install-
  live/tape-dormant, so M2 stays byte-exact (no re-pin).
  **M2 phase-28 T5** wires **row `1A`** after the Layer-1 live proof
  (`test_keeper_installer_live_reach`): the keeper-CPU ladder installs it on the
  real frame path — the static P1 permit (`0x110680`, FU298), the live
  row-`0x19` `[0x157C5D]` interlock (FU299 Option B one-step pulse) and the
  FU290 computed `CX` (the `CX=4` arm installs/dispatches row `0x1B` and the
  camera chain; `CX=1` installs/dispatches row `0x1A` with the G2 stamp `0x2C`)
  — through the merged `fifa96_match_entities_install` contract (FU304). Census
  `59/80 → 60/80 = 75%`; M1 `09b726b7…` / M2 `b281a9b2…` byte-identical (no
  re-pin).
  dispatch results 45 UNSUP / 34 OK / 1 NOTF (sum 80; action 45 = 31 OK + 14
  UNSUP, phase 35 = 3 OK + 31 UNSUP + 1 NOTF). *(errata, phase-16 T2 review:
  the `49 UNSUP / 31 OK / 1 NOTF` this line replaced was likewise off by one —
  the corrected derived numbers above are the authority. phase-16 T3: row `0x1C`
  wired, so `48 UNSUP / 31 OK / 1 NOTF` → `47 UNSUP / 32 OK / 1 NOTF`. phase-16
  T4: row `0x16` + phase `0x19` wired, so `47 UNSUP / 32 OK / 1 NOTF` → `45
  UNSUP / 34 OK / 1 NOTF`.)*
- **M2 phase-16 T3 (FU-186) — the keeper chain: row `0x1C` wired/counted; row
  `0x1A` ported/uncounted; `0x1B` legs re-pinned.** FU-186 corrects FU-182 §5's
  installer census (the true `0x1A` installers are only the three `FUN_00077EAC`
  ladder arms `0x78227`/`0x7824F`/`0x78277`; the true `0x1C` installers are
  `FUN_00077EAC` `0x782B6` and `FUN_00076130` `0x76291`/`0x7634B`; FU-182's
  `0x773DC`/`0x70DAF`/`0x80814`/`0x9E665` are false positives — mailbox/animation
  arguments and a constant getter). **`0x1C`** (`fifa96_match_action_1C`, the
  bounded `0x77728..` lunge body: head `timer89 += delta`, stage 0
  (`ran`/`timer89=0`/`keeper_latch[2]=0`/stage→1), stage 1 = the `0x8DCD4`
  metric + the `fifa96_keeper_lunge_track` on-target pos shift
  (`|angle(distance,target_y)| < 0x55`), stage 2 (`row44` → `match_row_reset`))
  is **wired and counted**: its installer is live today — the wired row-0x1F
  stage-3 tail drains `fifa96_keeper_input_decide`, whose both-slot-dirs-zero
  arm installs `0x1C` invoke-now (`FUN_00076130` `0x76291`→`0x762A4`) and the
  tail then dispatches it (proven by `test_action_1c_live_route`). **`0x1A`**
  (`fifa96_match_action_1A`, the bounded `0x7662C..` stage-0 slice) is ported
  and exported but its slot stays **NULL/uncounted**: its only installer is the
  unported `FUN_00077EAC` CPU ladder on the unported `FUN_0008D8EC →
  FUN_000782D0` pre-pass (FU186-L5/L6/L7). The deep remainder — the `0x1A`
  `[0x157C2C]` direction machine + movement block (FU186-L1), the `0x1C`
  rotated arm (`0x114E04` sine + `0x795A4`), the `0x6E598` anim, the `0x76B28`
  steering and the `0x779DE` tail (FU186-L2/L3), `FUN_000765C4` (FU186-L4), the
  two `FUN_00077EAC`/`FUN_000782D0` machines (FU186-L5/L6) and the `0x92820`
  mailbox / `FUN_0009E420` getter (FU186-L8/L9) — stays numbered legs. The
  row-0x1B sine/steering arms are re-pinned to FU186-L2/L3 (no change to its
  FU-180 landing). Both rows are install-live/tape-dormant, so M1/M2 stay
  byte-exact (no re-pin); census 31/80 → **32/80 = 40%**.
- **M2 phase-16 T4 (FU-187/FU-188) — row `0x16` + phase `0x19` wired/counted;
  row `0x24` + phase `0x20` ported/uncounted.** The act machine's second payload
  (controller Ruling W5). **Phase `0x19`** (`fifa96_match_action_phase_19`) is
  the native phase-0x19 / act-3 handler `0x89FA4` — the 7-stage foul sequence —
  ported as the derived `fifa96_ref_foul_sequence_step` + `match_run_ref_apply`;
  it aliases `mr->act.stage/timer` <-> `referee.stage/timer` so the native head
  add is counted once (FU187-L3), and clears the act slot before the stage-6
  situation-0xA re-dispatch (the native `0x8A3D0/0x8A3D6/0x8A3EA` order).
  **Row `0x16`** (`fifa96_match_action_16`, the `0x84630..0x8471C` 3-stage
  face-the-ball body) is **wired and counted**: the act-3 producer now routes
  through the seam — the row-0x0C contact's `FIFA96_REF_SEQ_ACT3` decision calls
  `fifa96_match_act_dispatch(mr, 3, 0, 1)` (the native `0x8A6FE FUN_000888FC
  (3,0,1)`), the phase-0x19 stage 0 installs code `0x16` on `[0x15888F]`
  (`0x8A0A0`), and the clock-tail tick advances the sequence (the retired
  `ref_machine = REF_FOUL` double-run, FU175-L7). Census +1 action +1 phase.
  **Row `0x24`** (`fifa96_match_action_24`, the `0x86510..0x866F1` lane/anim
  body) and **phase `0x20`** (`fifa96_match_action_phase_20`, the `0x8B688`
  code-0x24 timeline) are **ported and exported but their slots stay
  NULL/uncounted**: FU-188 proves the pair's only producer is situation 0, which
  no engine source/test dispatches (the rows-27/29/2C precedent; the owner leg is
  **FU188-L1**, the situation-0 producer). No producer was faked. The row-0x16
  stage-0 gate polarity (`[0x158866] == 0x48`, FU187-L1) and the anim/face/band
  sinks (FU187-L2/L4), the phase-0x19 stage/timer aliasing (FU187-L3) and the
  row-0x24 `0x157A38` lane table / `0x6E1D0` release (FU188-L3/L5) stay legs.
  Both counted units (row `0x16` + phase `0x19`) are install-live/tape-dormant,
  so M1 `09b726b7…` and M2
  `7b4b241a…` stay **byte-exact (no re-pin)**; census 32/80 → **34/80 = 42%**.
- **M2 phase-16 T5 (FU-189/FU-190) — installer hunt + rows `03`/`0x0D`
  completion: all four conditional rows stay NULL/uncounted (honest legs, no
  invented wiring).** The controller's Ruling **W6**. **Row `0x20`**: the
  installer *exists* (`0x899a2` = `MOV EDX,0x20; MOV EAX,[ESP+8]; CALL 0x7D9A4`,
  fresh) inside the phase-0x1D handler `0x89868`, but `0x89868`'s sole reference
  is the phase-table slot `0x110808` (`[0x1D]`, fresh `get_xrefs_to = 1`) — the
  act-7 route — and no engine source dispatches situation 8 (the act-7 producer;
  the dispatched set is 1/2/3/6/9/0xB/0xC), so the phase-0x1D route is absent and
  the installer never fires (leg **FU189-L1**, owner: the phase-0x1D route).
  **Row `0x25`**: no static installer exists — the bounded exhaustive
  `MOV EDX/DX,0x25` scan (`BA25000000`/`66BA2500`/`C7C225000000`) finds no
  `FUN_0007D9A4` site (the `0x7E460` hit is the `0x6E598` anim sink) — so the row
  stays uncounted and no installer is invented (leg **FU189-L2**). **Row `03`**:
  both completion dependencies — `0x6DCC8` (FU177-L1, fresh `get_xrefs_to = 5`)
  and the per-record `[rec+0x1C]` seam (FU175-L1) — stay unported (legs
  **FU177-L1 + FU175-L1**). **Row `0x0D`**: its only installer `0x80F61`
  (`ba0d000000`/`e835caffff`) sits in row-09 arm 1 (FU178-L1), whose install path
  is gated by unported producers (`[0x15877C]` FU178-L6, `[rec+0x3D]` the FU-84
  frame, `[rec+0x24]` the owner, `[[rec+4]+0x13]`) and helpers/sinks
  (`0x8f188`/`0x8ed40`/`0x92820`/`0x7a490`/`0x8a3fc`) — FU-178/FU-190 do not show
  it reachable, so the row stays uncounted (leg **FU178-L1**). No census change
  (the wired set is unchanged, `34/80 = 42%`); M1 `09b726b7…` and M2
  `7b4b241a…` stay **byte-exact (no re-pin)**; `port/tests/test_engine_match_handlers.c`
  pins the four legs (NULL slot + named owner + UNSUP dispatch) and the
  phase-0x1D gap (`fifa96_match_dispatch_phase(0x1D) == UNSUP`).
- **M2 phase-16 T6 (acceptance) — the tape is v16 (byte-identical), the review
  riders folded in, the smoke recorded, the census reconciled.**
  **Tape decision: v16 strata, no re-pin.** Every phase-16 landing is
  install-live/tape-dormant, so no presented frame moved: M1 `09b726b7…` is
  unmoved and M2 is held at the v15 bytes `7b4b241a…` (792 lines, `cmp` clean
  against `port/tests/golden/engine/m2-frames.txt`). The v16 label records that this
  phase's acceptance strata exist at the acceptance layer (the phase-15
  v14→v15 precedent labels the acceptance layer, not the tape bytes); holding
  v15 was the alternative, recorded here as rejected for precedent
  consistency. `port/tests/test_engine_m2.c` gains the v16 provenance block and the
  phase-16 riders (`run_phase16_riders`: phase 0x18's act-timer head add
  [FU-183], phase 0x19's stage-0 advance [FU-187], row 0x14's stage-0 timer
  gate [FU-184], row 0x16's stage 0→2 advance [FU-187]); the phase-15 census
  riders are renamed `run_census_riders` (they now carry T1–T5). Mutation-RED:
  disabling the phase-0x18 head add aborts `run_phase16_riders` at its own
  assert.
  **Review riders (the T1–T5 deferred minors, each verified first-hand).**
  (1) `test_action_16_runs_body` tightened to `stage92 == 2u` (the `0xFF`
  reset needs `row44 != 0`, false on that path); (2) the stale AGENTS.md
  completion-percentage example (`24/80 → 30%`) swept to `34/80 → 42%`;
  (3) this file's T4 line now reads "both counted units (row `0x16` + phase
  `0x19`)"; (4) the T2 errata back-reference corrected from the wrong
  `50/29/1` to the actual predecessor `49 UNSUP / 31 OK / 1 NOTF` (sum 81, off
  by one); (5) the action-dispatch enumeration is marked the pre-phase-15 core
  (the phase-15/16 rows are named in the bullets); (6) the row-0x1C source
  comment's drafting artifact removed; (7) `run_phase15_riders` renamed
  `run_census_riders`. No rider moved the tape.
  **Smoke (`DISPLAY=:1`, this host 2026-10-12, `make game`):** the window
  opens (960×720 client) and the EA-SPORTS intro (`docs/screens/p16-t6-intro.png`)
  → front-end (`docs/screens/p16-t6-frontend.png`) draw; ESC exits. The
  phase-16 landings are install-live/tape-dormant, so **no visible delta is
  claimed**. The SDL exit leaks 1942 B (dbus `dbus_bus_register` +
  nvidia-GL under `SDL_CreateWindowAndRenderer`, `platform_sdl3.c:78`) under
  LSan — **pre-existing host-library noise**, not an engine leak.
  **Census: unchanged, `34/80 = 42%`** (`floor(100 × 34/80) = 42`); dispatch
  `45 UNSUP / 34 OK / 1 NOTF` (sum 80; action 45 = 31 OK + 14 UNSUP, phase 35
  = 3 OK + 31 UNSUP + 1 NOTF); README title `42%` — all consistent.
  **Remaining legs (owners; derivations live in the FU slice named).**
  Row-0x17 producer chain **FU181-L1** (FU-183); the phase-18 FU-83 timeline
  remainder **OL-13**; the row-0x14 scatter divergence **FU184-L1** and the
  celebration/arm/release remainders **FU184-L2/L3/L4/L7**; the row-0x22
  `opponent_invoke` angle-input gate **OL-65 / FU185-L6** (0x22 uncounted);
  the keeper-chain remainders **FU186-L1..L9** (incl. the `0x1A`
  `FUN_00077EAC` installer ladder **FU186-L5**); the row-0x16 `[0x158866]`
  polarity **FU187-L1** + anim/face sinks **FU187-L2/L4** + phase-0x19
  stage/timer aliasing **FU187-L3**; the phase-0x20 situation-0 producer
  **FU188-L1** + row-0x24 lane table/release **FU188-L3/L5**; row `0x20`
  (phase-0x1D-gated) **FU189-L1** + row `0x25` (no installer) **FU189-L2**;
  row `0x0D` row-09 arm 1 **FU178-L1**; row `03` `0x6DCC8` **FU177-L1** + the
  per-record `[rec+0x1C]` seam **FU175-L1**; the phase-row cohort envelope
  **FU190-L1/L2**. Every phase-16 FU leg is unchanged from its slice; this
  bullet is the owner register, not a re-derivation.
- **M2 phase-17 T1 (FU-191/FU-198) — the per-record phase seam + phase row
  `0x02` wired/counted, action row `03` ported but leg'd (tape-visible).**
  The native per-record walk `FUN_0008D098` (`0x8D0F2..0x8D166`) is landed as
  `fifa96_match_run_record_phase_walk` and called from `match_run_write_phase`
  for both teams after every phase write (the native `FUN_000740A0`
  `0x740C8`/`0x740DB`) — 11 records at the `0xB2` stride, the `[rec+0x9A]`
  occupied-record skip, and the `[rec+0x1C]` handler called with the native ABI
  (EAX=rec / EDX=&rec[0x4D] / EBX=-1) through the new
  `fifa96_match_dispatch_phase_record` + the run's phase-walk cursor
  (`fifa96_match_handlers.{c,h}` / `fifa96_match_run.h`; the `fn(mr)` ABI gap is
  FU191-L3). **The walk is per-phase** — it dispatches whichever non-NULL row it
  is handed, gated on `fn != NULL`. **On the live engine it dispatches only
  phase `0x02`**, because that is the sole phase that is both written by
  `match_run_write_phase` and non-NULL. **The engine-reach set** (the phases a
  live `match_run_write_phase` call can write, derived from the live call sites,
  not the table's possibility) is the game-phase family
  `{0,1,2,3,4,5,6,7,8,9,0xA,0xC,0xF,0x11,0x13,0x14,0x15}`
  (all `< 0x16`; verified over the 11 call sites + the
  `fifa96_action_phase_situation`/`fifa96_action_phase_period_end`/`fifa96_ref_sequence`
  sources) — `0x0D` is a possible argument (via `fifa96_action_phase_situations[7]`
  = `set_piece(mr, 7, …)`) but no live caller dispatches situation 7, and
  `0x0B`/`0x0E`/`0x10` likewise have no live writer (the only `0x0B`/`0x10` writers
  sit inside the unwired rows `0x1A`/`0x1D`); only `0x02` of the reach set has a
  non-NULL `fifa96_match_phase_table` row at T1 — the act-tail rows
  `0x18/0x19/0x1F/0x20` are never written by this function. *(The reach set is
  cited identically in FU-175 §7B and FU-195 §5; the final-review C1
  reconciliation below removes `0x0D` from the per-record count.)*
  The divergence from the native unconditional `FUN_0008DCD4` distance
  write is **FU191-L2**, recorded not hidden. **Brief deviation (recorded):** the
  T1 brief said "replace the global `fifa96_match_dispatch_phase` call with the
  native per-record walk", but there is no `fifa96_match_dispatch_phase` caller
  in the frame body (its only callers are the act seam). The native walk is
  instead **added** to `match_run_write_phase` — the mirror of the native
  `FUN_000740A0`, the walk's sole caller (`0x740C8`/`0x740DB`) — leaving the
  act-tail `fifa96_match_dispatch_phase` callers intact (their domain
  `0x17..0x22` is disjoint from the walk's per-record family). The
  phase-table row `0x02` = the ported native `0x6DCC8`
  (`fifa96_match_phase_02`): reads `[rec+0xC]`, indexes the direction pair by
  the walk's `EBX=-1` (the per-side default `word[0x1577DE+side*4]>>16`) or an
  explicit index, and writes `target_x = byte0*±0x26` / `target_y=0` /
  `target_z = byte1*±0x21` (side-flipped by `[team+0x826]`); the `[rec+0xC]`
  OL-13 source is unported (**FU192-L1**), so the live body is
  dispatched-but-gated (the phase-16 row-`0x16` W9 precedent). The walk +
  phase `0x02` are **tape-inert** (targets are render-invisible; the render
  reads pos/anim/frame/hidden only), verified byte-exact. **Phase `0x15` was
  deliberately left unwired**: the native row `0x110794[0x15] = 0x6DCC8` is the
  same body as `0x02` and `match_run_write_phase(mr, 0x15)` is a live write
  (`fifa96_match_run.c:2689`, the act-9 sit-0xC producer), so a **conservative
  35** is recorded — a defensible **36** exists by wiring `0x15` too. T1 pinned
  only the phase-`0x02` writer path tape-inert; the shared `0x15` row's own
  writer path (phase-`0x15` frames) was not separately pinned, so it is deferred
  to T3 (the per-record body cohort that owns phases `0x00–0x15`), not counted on
  an unpinned assumption. **Action row `03`**
  (`fifa96_match_action_03`: the `0x7E1A4..0x7E524` hold/clamp body over the
  FU-138 helpers + `fifa96_match_entities_camera_place`, its live phase-2 hot
  path calling `[rec+0x1C]` = `0x110794[2] = 0x6DCC8` through the T1 seam) is
  **ported but its slot stays NULL / uncounted**: wiring it moved the M2 tape at
  transcribed frame 29 (controller R7), so the row **legs** (owner
  **FU177-L1**; the body is exported and direct-tested). Remaining legs: the
  `FUN_0008C418` team bound block (the phase-2 clamp inputs), `FUN_00079C20`,
  `FUN_00079B58`, the phase-0x13/0x14 special arm, the phase-7 `0x158747` wall
  scan and the phase-8/0xC tails (**FU177-L1..L6**). Row `03` remains a live
  force-decision code (`match_run.c:960`) so its uncounted status is honest, not
  dormant. Census `34/80 → **35/80 = 43%**` (`floor(100 × 35/80) = 43`); dispatch
  `44 UNSUP / 35 OK / 1 NOTF` (sum 80; action 45 = 31 OK + 14 UNSUP, phase 35
  = 4 OK + 30 UNSUP + 1 NOTF); README title `43%` — all consistent. M1
  `09b726b7…` and M2 `7b4b241a…` stay **byte-exact (no re-pin)**.
  `port/tests/test_engine_match_handlers.c` pins the seam (the `0x6DCC8` arithmetic
  incl. the side flip and the OL-13 gate), the walk (scan + skip + NULL-row
  gate) and the row-03 direct body; mutation-RED discriminators: the `0x26`
  scale and the `0x9A` skip. *(errata, M2 phase-25 W25-8 / FU-258 / FU-261 /
  FU-263: (i) the frame-29 attribution above is corrected — the mover is action
  row `03`'s `phase-0x13/0x14` special arm (`0x7E1CE`), **not** the phase-2
  `[rec+0x1C] = 0x6DCC8` hot path: at the transcribed frame 29 the global phase
  byte `0x157A4D` is `0x14` (fresh: the M2 golden `frame=29 … state=20/0-0`),
  the special arm `RET`s before the slot gate, and `0x110794[0x14] = 0x6E244`
  (`0x6DCC8` is phase `0x02`/`0x15` only) — so `0x6DCC8` cannot run at frame 29;
  the two phrasings name the install window and the dispatch window (FU258
  §4.3). (ii) The situation-7 reach note above is sharpened: the correct
  phrasing is *"situation 7's two native producers lie in unported action arms"*
  (row `0x1A` `0x76a90`, row `0x1B` stage-2 wide `0x77589`), not *"no live
  caller / no native producer"* (FU261 §3.2/§4, `FU261-L6`). The old text is kept
  verbatim.)*
- **M2 phase-17 T2 (FU-196/FU-197) — the situation producer machine + the
  act-4 formation arm, both landed gated (census unchanged `35/80 = 43%`).**
  The native situation producers the engine lacked are ported as
  `fifa96_match_run_period_setup(mr, mode)`: `mode == 1` is the match-init
  `FUN_000493A0 -> FUN_0004B02C(1)` (0x4B0A5 → situation 0 = phase 0x11 + act
  0xA), `mode == 0` is the `FUN_00088860` period-end arm (0x4B064: phase-0xC
  gated → situation 8 on the extra-time period index with the RNG side 0x888C8,
  else situation 1 on the controlled side 0x888F2), plus the common extra-time
  gate (0x4B0CB `(input_range[0]|input_range[1]) & 0x0100` → situation 8 + the
  period index 4). Situation 0xB was already producer-live (row 01). **The live
  `begin` path is gated**: `period_setup_live` defaults 0 (the tape-neutral
  sequence), because situation 0 fires phase 0x20/act 0xA and situation 8 fires
  phase 0x1D/act 7, whose bodies are NULL until T5 (FU196-L8); wire it live only
  with T5's phase-0x20/0x1D bodies (the phase-16 T1 producer-first ordering, R9).
  The act-4 sit-2..4 formation-order arm (`0x8AB08..0x8AB62`: the two
  `FUN_0008C974` sorts, the `[0x157A4C] != 1 && ([0x157B8E]|[0x157B8F])` gate,
  the `[0x158820]`/`[0x15881E]` store cells and the act-4 invoke) is ported over
  the derived order matrices (`formation_order`/`formation_current`/
  `formation_mismatch`) + `fifa96_match_run_formation_sort`, but **gated on
  `formation_order_ok`, which stays 0**: the desired-order `0x14C360` producer is
  the unported formation/line-up setup `FUN_00011620`/`FUN_00012010`
  (**FU197-L4**), so the arm is provably dormant — sorting the zero stand-in
  would compute a residual 0 and act as a fake producer (R45/FU-197 §8). **Row
  `0x17` stays NULL / uncounted** (its installer `0x89740` is act-4 stage 0,
  phase `0x1A`, still NULL; owners **FU181-L1/FU183/FU197**). Remaining legs:
  the `FUN_00074034`/`FUN_0007412C`/`FUN_0007417C`/`FUN_0008CBB0`/`FUN_00078824`
  restart/refresh sinks + the extra-time bit-8 semantics (**FU196-L1/L2/L6**),
  the `0x157B90`/`0x157BA6` companions + the `0x8C758` entity re-link + the
  `0x4728C` placement-commit sink + the sit-1 arm twin (**FU197-L1/L2/FU149-L12**),
  and the OL-13 `[rec+0xC]` metadata source (**FU192-L1** remains a leg — its
  genuine producer is the same unported formation setup, so porting it would
  un-gate T1's row-03 rewrite, R4). `port/tests/test_engine_match_handlers.c` pins the
  producer modes (`p17-t2`), the sorter and the arm gate; mutation-RED
  discriminators: the extra-time mask `0x0100` and the `formation_order_ok` gate.
  The `test_engine_m2` `run_phase17_t2_riders` pins the dormancy; M1 `09b726b7…`
  and M2 `7b4b241a…` stay **byte-exact (no re-pin)**.
- **M2 phase-17 T3 (FU-192) — the per-record phase handlers `0x00–0x15`: seven
  distinct bodies ported; 12 per-record rows wired/counted (census `35/80 →
  47/80 = 58%`).** The remaining bodies of the per-record phase family are
  ported over the T1 walk cursor (`fifa96_match_phase_00` = the shared `0x6DE34`
  reset `target := position`, rows `00/0A/0F`; `fifa96_match_phase_04` =
  `0x6DE44` (rows `03/04/07`), the `word[0x1577D6/D8+side*4]` side-selected
  placement scaling the `[rec+0x14]` dword pair `W+(W>>2)` / `D+(D>>4)`,
  side-flipped; `fifa96_match_phase_06` = `0x6DD9C` (row `06`), the
  `[rec+0x10]` ×1.25/×1.0625 scale with the `[0x157754]`-sign index and the
  ±0x60 z bias; `fifa96_match_phase_08` = the `0x6DD6C` wrapper (rows `08/09`)
  selecting the shared `0x6DCC8` index `0xBC`(same side)/`0x6F`(other);
  `fifa96_match_phase_05` = `0x6E05C` (row `05`), the derivable
  `target := position` arm; `fifa96_match_phase_0C` = `0x6DF4C` (row `0C`), the
  stateful `[rec+0x89] += [0x157A64]` restart-line timer with the `0x780`/`0xCC0`
  target writes past the `0x3C` gate; `fifa96_match_phase_10` = `0x6E004` (row
  `10`), the `0x1105EA` variant table). The shared T1 body `0x6DCC8` also fills
  row `0x15` (T1 deliberately deferred it). **Gated-count caveat (read the 58%
  with this):** 7 of the 12 newly-wired rows are live **dispatched-but-gated
  no-ops** — `0x03`/`0x04`/`0x07` (`0x6DE44`, `[rec+0x14]` NULL,
  `fifa96_match_handlers.c:4668`), `0x06` (`0x6DD9C`, `[rec+0x10]` NULL, `:4729`),
  and `0x08`/`0x09` (`0x6DD6C`) + `0x15` (the shared `0x6DCC8`) via the NULL
  `phase_meta` (`:4594`/`:4603`/`:4748`). Their phases *are* engine-written and
  their rows *are* dispatched (so they count under the repo's own definition —
  the FU-177 §7 / T1-phase-`0x02` convention), but the ported bodies perform the
  native work only when a fixture supplies the unported input; the remaining 5
  (`0x00` reset, `0x05`, `0x0A`/`0x0F` reset aliases, `0x0C`) do their
  work with no input gate. **Tape-inert (R10):** M1
  `09b726b7…` and M2 `7b4b241a…` stay **byte-exact (no re-pin)** — the on-tape
  rows (`0x00` reset, `0x04` the gated `0x6DE44`, `0x15` the shared `0x6DCC8`)
  are render-invisible (targets are render-invisible, FU-191 §10; the metadata
  bodies are NULL-gated no-ops), and the off-tape engine-written rows
  (`0x03/0x05/0x06/0x07/0x08/0x09/0x0A/0x0C/0x0F`) never dispatch on the
  tape. **Legs:** `0x6E244` (rows `0x13/0x14`) is not wired — it consumes the
  PRNG `0x92AC8` twice per active record and its `0x10F364/0x10F368` producer is
  the unported `FUN_0008D098` case-0x13/0x14 scatter (**FU192-L2**); rows
  `0x01`/`0x11`/`0x12` (`0x6E1D0`/`0x6E1C8`, the formation placement) are not
  wired — the placement is already realized by the seeding path
  (`fifa96_match_entities_seed_formation`) and wiring the phase row would
  double-run it (R10 "do not double-wire"; **FU192-L6/L7**); rows
  `0x0B`/`0x0D`/`0x0E`/`0x10` stay NULL because no live engine source writes those phases
  (a wired row would be an un-dispatched/fake count, the rows-27/29/2C
  precedent; `0x0D`'s sole argument, situation 7, has no live dispatcher — the
  final-review C1 leg, owner FU-146). The `[rec+0x10]`/`[rec+0x14]` OL-13 metadata pointers
  (**FU192-L1**) and the `0x6DE44` DX<0 cell-map arm `0x6D870`/`0x15774C`
  (**FU192-L3**) remain legs. One characterization shift (recorded): wiring the
  reset row `0x0A` makes the native `FUN_000740A0` walk reset the records'
  targets on the phase-0x0A writes, so the phase-7 nearest pick (which stands in
  for the phase-handler placement output) sees the reset positions —
  `test_engine_match_frame`'s offside chain now installs the FK on the
  opponent-side record 2 (`target == 13`), the native-faithful nearest (the
  pre-T3 expectation assumed un-reset zero targets). `port/tests/test_engine_match_handlers.c`
  pins each body (`p17-t3`); mutation-RED discriminators: the `0x6DE34` y-copy,
  the `0x6DE44` `W>>2`, the `0x6DD9C` ±0x60 bias, the `0x6DD6C` `0xBC`, the
  `0x6DF4C` timer add, the `0x6E004` `<< 5` scale and the `0x6E05C` position
  copy. The `test_engine_m2` `run_phase17_t3_riders` pins the cohort dormancy.
  Census `35/80 → **47/80 = 58%**` (`floor(100 × 47/80) = 58`); dispatch
  `32 UNSUP / 47 OK / 1 NOTF` (sum 80; action 45 = 31 OK + 14 UNSUP, phase 35 =
  16 OK + 18 UNSUP + 1 NOTF); README title `58%` — all consistent.
  *(errata, phase-17 final review C1: the `48/80 = 60%` this line replaced
  counted phase `0x0D`, over-counted by one — the row has no live writer.)*
- **M2 phase-17 T4 (FU-193) — the act-tail cohort `0x17`/`0x1A`/`0x1C`: the
  three bodies ported, `+0` counted (census unchanged `47/80 = 58%`).** Per
  Ruling R8, FU-193 finds each native producer statically but proves none is
  live under the strict "engine reaches it" test, so the three phase-table slots
  stay **NULL/uncounted** (the rows-27/29/2C precedent) and the bodies are
  exported for direct unit testing with the producer remainder as numbered legs.
  Ported bodies (`port/src/fifa96_engine/fifa96_match_run.c`, the act-tail family
  with `0x18`/`0x19`/`0x20`): `fifa96_match_action_phase_17` (the act-1
  kickoff/phase-1 entry `0x88DC8`: head `timer += frame_delta`; stage 0 clears
  `[0x15882A]` + runs the derived kickoff entry, stage++; stage 1 hands off to
  act 4 `0x88EE8` on `[0x157A4C] != 1 && a residual mismatch`, else the
  `word[0x15881A] >= 0x78` gate sets `[0x15882A] = 1` and clears the slot);
  `fifa96_match_action_phase_1A` (the act-4 formation-order walk `0x89620`:
  the `formation_current != formation_order` walk gated on
  `mr->formation_order_ok`, the phase-0xB write + action-row-0x17 install
  `0x89740`, and the `0x897AE` end-block re-dispatch of
  `store_15881e`/`store_158820`); `fifa96_match_action_phase_1C` (the act-6
  pre-restart timeline `0x89110`, a thin alias of the already-ported
  `fifa96_ref_offside_sequence_step` in the phase-0x19 FU187-L3 style). **The
  producer legs:** `0x17` — the sit-1 arm `0x8ABCF` has no live driver, and the
  engine's live act-1 behaviour is the **ad-hoc** `fifa96_match_run_kickoff_entry`
  + the frame-body `global_5882a` arm, so wiring the row would **double-run** the
  kickoff (**FU193-L1/L2**); `0x1A` — no act-4 row/producer: the T2 sit-2..4 arm
  is gated dormant on the unported desired-order matrix
  (`formation_order_ok` stays 0, FU197-L4) and the act-1 stage-1 hand-off is
  itself unwired (**FU193-L3**); `0x1C` — no engine source dispatches act 6, and
  the existing driver is the offside machine
  (`fifa96_match_run_offside_reception` → `fifa96_ref_offside_sequence_step`), so
  the row would double-run it (**FU193-L4**). Other legs: `0x1A`'s phase-0x1A
  stage-1 `[rec+0x8E]`→`[0x110680]` gate and the `FUN_0007D9A4` invoke-now
  (**FU193-L8/FU197-L6**); `0x17`'s head guard (the goal-log words
  `[0x157AC5]`/`[0x157AC7]` + `[0x157AC2]`) and presentation sinks (**FU193-L7**).
  **Tape-inert (R8):** M1 `09b726b7…` and M2 `7b4b241a…` stay **byte-exact (no
  re-pin)** — the three slots are NULL, so nothing dispatches. Tests:
  `port/tests/test_engine_match_act.c` (`test_phase_17_*`, `test_phase_1A_*`,
  `test_phase_1C_*`, the `*_not_counted` NULL pins); mutation-RED
  discriminators verified per body (the `0x17` stage advance, the `0x1A`
  action-row install, the `0x1C` stage alias). `test_engine_m2`'s
  `run_census_riders` pins the three NULL slots. Census unchanged `47/80 = 58%`
  (dispatch `32 UNSUP / 47 OK / 1 NOTF`); README title `58%` — all consistent.
- **M2 phase-17 T5 (FU-194) — the act-tail cohort `0x1D`/`0x1E`/`0x20`
  (act 7/8/0xA): the two new bodies ported (`0x1D`/`0x1E`; `0x20` was already
  ported, FU-188), `+0` counted (census unchanged `47/80 = 58%`).** Per Ruling
  R8, FU-194 finds each native producer statically (sit 8 → act 7; the
  `FUN_0008A938` BX!=0 fallback → act 8; sit 0 → act 0xA) but proves none is live
  under the strict "engine reaches it" test, so the three phase-table slots stay
  **NULL/uncounted** (the rows-27/29/2C precedent) and the bodies are exported
  for direct unit testing with the producer remainder as numbered legs. Ported
  bodies (`port/src/fifa96_engine/fifa96_match_run.c`):
  `fifa96_match_action_phase_1D` (the act-7 10-arm set-piece timeline `0x89868`:
  head `timer += frame_delta`; stage 0 clears `[0x15882A]`, writes phase 0x10 and
  installs action row 0x19 on the **team base** (`records[0]`, once per team — the
  native `0x8990A` CALL sits outside the 11-record ESI loop); stage 1 fires the
  whistle `0x974DC(0x1E)` + `[0x15882A] = 1` at `state.tick_total >= 0x78`; stage 9
  the terminal clears the act slot + `[0x157A4C]`); `fifa96_match_action_phase_1E`
  (the act-8 restart sweep `0x8A798`: stage 0 resets the walk cursor
  `act8_team`/`act8_record`; stage 1 sweeps team 0 records 0..10 then team 1,
  clears `act8_selected` before every gate test (`0x8A814`) and selects the
  code-`0xC`/`+0x99`/`!+0x9A` record into `act8_selected`; stage 2 clears the act
  slot and re-dispatches the stored situation/side, then `store_15882b = 0xFF`).
  **The coupled `0x1D` + row-`0x20` unit:** arm 0 of `0x89868` carries the
  row-0x20 installer `0x899A2`, but the route needs the
  act-7 producer (situation 8) and the engine reaches situation 8 only through
  the T2 `fifa96_match_run_period_setup` + `period_setup_live`, which is **gated
  off** (R9 / FU196-L8) — so **neither** slot is producer-live and both stay
  NULL/uncounted (**FU194-L1/L8**). Phase `0x1E` stays NULL because the engine
  already models the act-8 fallback as a *compressed* stored-situation
  re-dispatch, so registering the full sweep would de-compress a tape-visible
  derivation (**FU194-L2**). Phase `0x20` stays NULL (sit-0 producer absent,
  FU188-L1). Erratum honoured: `0x4C374` is a `word[0x157A68]` clear (it ignores
  EAX), not the FU-188 §5.1 / FU-189 §3.4 "install code EAX" reading
  (**FU194-L3**). Other legs: the coupled row-0x20 installer + the ball row-0x1F
  install (`0x899CD`) + the `0x7A028` reset (**FU194-L7/L8**); the arm-8
  selection block `0x89BB6..0x89E6D` (**FU194-L7**); the arm-0 phase-0x10 write
  (**FU194-L9**). **Tape-inert (R8):** M1 `09b726b7…` and M2 `7b4b241a…` stay
  **byte-exact (no re-pin)** — the three slots are NULL, so nothing dispatches.
  Tests: `port/tests/test_engine_match_act.c` (`test_phase_1D_*`, `test_phase_1E_*`,
  the `*_not_counted` NULL pins); mutation-RED discriminators verified per body
  (the `0x1D` stage advance and terminal, the `0x1E` selection and terminal).
  `test_engine_m2`'s `run_census_riders` pins the two new NULL slots. Census
  unchanged `47/80 = 58%` (dispatch `32 UNSUP / 47 OK / 1 NOTF`); README title
  `58%` — all consistent.
- **M2 phase-17 T6 (FU-195) — the act-tail cohort `0x21`/`0x22` + the `0x1B`
  verdict: phase `0x21` wired/counted, `+1` (`47/80 → 48/80 = 60%`).** The two
  act-tail handlers `0x8B874` (phase `0x21` / act 0xB, the native
  extra-time-CLEAR period-end timer) and `0x8B900` (phase `0x22` / act 0xC, the
  extra-time-SET three-stage analogue) are ported (`fifa96_match_action_phase_21`
  / `_22`, raw `disassemble_bytes`; both are table-only targets with no Ghidra
  function). **phase `0x21` is the one counted row:** its producer
  `FUN_0008B9CC`'s tail (`0x8BAE7 CALL 0x000888FC(0xB,0,BX=1)`) is mirrored
  live — T6 adds the native act-selector invocation to
  `fifa96_match_run_phase_drive` (after the phase write, the `LAB_0008bac6`
  position), so the derived chooser (`fifa96_action_phase_period_end`,
  `out.act = 0x0B` under the selector-0 default) drives
  `fifa96_match_act_dispatch` → the phase-`0x21` body invoke-now. Body: head
  `timer += frame_delta` (`0x8B884`); stage 0 (`0x8B89E`) fires the
  `0x974DC(0x1E)` event (staged `ref_whistle`, the sibling phase-1C/1D
  convention), timer 0, stage 1; stage 1 (`0x8B8C3`) waits the derived
  `timer >= 0x3C` (`word[0x15881A]` gate, **FU195-L4**) then fires the event
  and clears the act slot (`0x8B8DE..0x8B8F3`); stage > 1 no-op. Render-invisible
  (act slot + audio sink only). **phase `0x22` is producer-dormant** — ported but
  its slot stays NULL: the native invoker fires act 0xC only when the extra-time
  flag `[0x157AC0] != 0`, whose engine producer is unported and `phase_drive`
  passes as `extra_time = 0u`, so `out.act` is always `0x0B` (**leg FU195-L2**).
  **phase `0x1B` / act 5 stays a leg** (**FU195-L1**, owner **FU175-L2**): the
  fresh 11-xref `0x888FC` census finds no `EAX=5` site, no indirect/data
  reference, and no `FUN_000740A0` phase-`0x1B` writer — a negative census, no
  invoker invented. The `0x974DC(0x1E)` audio ring-push/audio-handle remainder
  (**FU195-L3**) and the `0x974DC` sink stay legs. **Tape-inert:** M1
  `09b726b7…` and M2 `7b4b241a…` stay **byte-exact (no re-pin)** — `cmp`-verified;
  the phase-`0x21` row fires only on a period completion, which the golden
  fixtures never stage, and its writes are render-invisible. Tests:
  `port/tests/test_engine_match_act.c` (`test_phase_21_*`, the producer-live
  `test_phase_21_producer_live_via_phase_drive`, `test_phase_22_body_direct`, the
  `test_phase_1B_leg_no_invoker` pin); mutation-RED discriminators verified (the
  `0x21` stage-1 act-slot clear, the `phase_drive` dispatch, the `0x22` stage-1
  timer reset). `test_engine_m2`'s `run_census_riders` pins the wired `0x21`, the
  NULL `0x22`/`0x1B`. Census `47/80 → **48/80 = 60%**`
  (`floor(100 × 48/80) = 60`); dispatch `31 UNSUP / 48 OK / 1 NOTF` (sum 80;
  action 45 = 31 OK + 14 UNSUP, phase 35 = 17 OK + 17 UNSUP + 1 NOTF); README
  title `60%` — all consistent.
- **M2 phase-17 T7 (acceptance) — the tape is v17 (byte-identical), the
  phase/act-seam riders folded in, the smoke recorded, the census reconciled
  (`48/80 = 60%`).** **Tape decision: v17 strata, no re-pin.** Every phase-17
  landing is install-live, producer-gated off, or tape-dormant, so no presented
  frame moved: M1 `09b726b7…` is unmoved and M2 is held at the v16 bytes
  `7b4b241a…` (792 lines, `cmp` clean against
  `port/tests/golden/engine/m2-frames.txt`). The v17 label records that this phase's
  acceptance strata exist at the acceptance layer (the phase-15 v14→v15
  precedent labels the acceptance layer, not the tape bytes); holding v16 was
  the alternative, recorded here as rejected for precedent consistency. The one
  frame-body touch — T1's native per-record walk — was landed with nothing new
  dispatched (it dispatches only phase `0x02`, whose body is metadata-gated on
  the unported `[rec+0xC]`/OL-13 source), so it provably cannot move the tape;
  action row `03` is ported but leg'd (its live rewrite moved M2 at transcribed
  frame 29, owner FU177-L1). `port/tests/test_engine_m2.c` gains the v17 provenance
  block and the phase-17 acceptance riders (`run_phase17_riders`): the **seam
  rider** (the per-record `[rec+0x1C]` seam is walk-only, one dispatch per
  non-skipped record routed to the staged record — a silent re-seam, a
  double-run or a store slip aborts), the **per-record walk pin** (only
  non-NULL rows dispatch), the **census shape + gated-row set** (`31` action +
  `17` phase = `48/80 = 60%`; the seven dispatched-but-gated no-op rows
  `0x03/0x04/0x06/0x07/0x08/0x09/0x15` are metadata-gated), **phase `0x21`'s
  dispatch**, and the **leg pins** (row `03` NULL/FU177-L1, `0x22`/`0x1B` NULL,
  `0x13/0x14` NULL). Mutation-RED: the per-rider mutations are recorded in the
  phase-17 T7 report (the double-run, the store slip, the re-seam, wiring a
  never-written row, dropping a wired row, dropping the `0x21` stage advance
  and wiring a leg row each abort the acceptance tape at their own assert).
  **Review minors folded (docs/comments only, no behaviour change):** the
  extra-time-gate cite `0x4B0F6` → `0x4B0CB` (T2); the T3 gated-count caveat's
  inline source anchors corrected to `fifa96_match_handlers.c:4668`/`:4729`/
  `:4594`/`:4603`/`:4748`; the `phase_1D` doc-comment "both teams' records" →
  the fixed team base `records[0]` (T5); the `[0x157AB1]` leg cite → FU-89 §11
  (T5); and a dated additive erratum on FU195-L3 (its "staged as a no-op" text
  vs the landed `ref_whistle` code). **Smoke (`DISPLAY=:1`, this host
  2026-10-12, `make game`):** the window opens and the EA-SPORTS intro
  (`docs/screens/p17-t7-intro.png`) → the front-end panel
  (`docs/screens/p17-t7-frontend.png`) → the kickoff match
  (`docs/screens/p17-t7-match.png`: the dark pitch, two placed players, the
  ball and the `0-0` HUD/clock) draw; ESC exits. The phase-17 landings are
  install-live/gated/tape-dormant, so **no visible delta is claimed**. The SDL
  exit leaks 1942 B (dbus/nvidia-GL under `SDL_CreateWindowAndRenderer`,
  `platform_sdl3.c:78`) — **pre-existing host-library noise**, not an engine
  leak. **Census: `48/80 = 60%`** (`floor(100 × 48/80) = 60`); dispatch
  `31 UNSUP / 48 OK / 1 NOTF` (sum 80; action 45 = 31 OK + 14 UNSUP, phase 35
  = 17 OK + 17 UNSUP + 1 NOTF); README title `60%` — all consistent.
  **Remaining legs (owners; derivations live in the FU slice named).** The
  per-record seam remainder **FU191-L1..L6** (incl. the `FUN_0008DCD4` distance
  triple **FU191-L2** and the record-threaded ABI **FU191-L3**); the
  `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` OL-13 metadata source **FU192-L1**, the
  `0x6E244` RNG producer **FU192-L2** and the `0x6DE44` cell-map arm
  **FU192-L3**; the act-tail `0x17`/`0x1A`/`0x1C` producers **FU193-L1..L8**;
  the act-tail `0x1D`/`0x1E`/`0x20` producers + the coupled row-`0x20`
  installer **FU194-L1..L9**; **FU195-L2** (`0x22` producer-dormant),
  **FU195-L3** (the `0x974DC` audio sink) and **FU195-L4** (`0x15881A` gate);
  the situation producer tails + the bit-8 semantics **FU196-L1..L8**; the
  act-4 formation arm remainder **FU197-L1..L4**; the per-record cohort
  envelope + row-03 completion **FU198-L1..L6**; row `03`'s live
  `[rec+0x1C]` rewrite (owner **FU177-L1**), the row-09 arm-1 installer for row
  `0x0D` **FU178-L1**, the row-`0x17` installer **FU181-L1** and the
  situation-0 producer **FU188-L1**; **OL-13** (the loader metadata) and
  **OL-65** (the row-0x22 angle-input gate). Every phase-16/17 FU leg is
  unchanged from its slice; this bullet is the owner register, not a
  re-derivation.
- **M2 phase-17 final review (C1) — the per-record census over-counted phase
  `0x0D`; corrected to `48/80 = 60%`.** Phase-table row `0x0D` was wired
  (`fifa96_match_phase_00`) and counted by T3, but the phase has **no live engine
  writer**: its only writer is `match_run_write_phase(mr, out.phase)` with
  `out.phase == 0x0D` from `fifa96_action_phase_situations[7]`
  (`fifa96_action_handlers.c:1141`, i.e. `set_piece(mr, 7, …)`), and no live
  caller in `src/`/`tools/` passes situation 7 — the only passes-7 sites are the
  direct test calls in `port/tests/test_engine_match_frame.c` (gate-open queue route,
  which writes no phase). The per-record walk therefore never dispatches the row,
  so it belongs with the never-live-written `0x0B`/`0x0E`/`0x10` (`NULL`), not the
  counted class. **Fix:** `fifa96_match_phase_table[0x0D].fn = NULL` (body/evidence
  comment retained; owner leg **FU-146**, the unported situation-7 screen
  producer). **Recomputed census** (the pre-correction `48`/`49` totals are quoted
  in the T3–T7 errata parentheticals, kept verbatim): T3 `35/80 → 47/80 = 58%`
  (12 per-record rows: 5 ungated + 7 gated); T4/T5 unchanged `47/80 = 58%`; T6
  `47/80 → 48/80 = 60%`; T7 `48/80 = 60%`. Dispatch `31 UNSUP / 48 OK / 1 NOTF`
  (sum 80; action 45 = 31 OK + 14 UNSUP, phase 35 = 17 OK + 17 UNSUP + 1 NOTF).
  The `port/tests/test_engine_m2.c` census riders (`phase_wired == 17`) and
  `port/tests/test_engine_match_handlers.c`'s `phase_expect[0x0D] = UNSUP` pin the
  correction. **Tape:** M1 `09b726b7…` and M2 `7b4b241a…` stay **byte-exact (no
  re-pin, `cmp`-verified)** — the row was never dispatched, so removing it moves
  nothing. **M6:** the T1 bullet's `match_run_write_phase` call-site count is
  corrected `9 → 11` (the two added writers sit in the unwired rows
  `0x1A`/`0x1D`). **I1:** one engine-reach set is stated (the T1 bullet above,
  cross-cited in FU-175 §7B/FU-195 §5) and excludes `0x0D`/`0x0B`/`0x0E`/`0x10`.
- **M2 phase-18 T1 (FU-199/FU-200) — the OL-13 metadata bind landed; M2
  byte-identical (no re-pin); census unchanged.** The per-record OL-13 metadata
  pointers `[rec+0xC]`/`[rec+0x10]`/`[rec+0x14]` are now produced on the live
  path — the engine mirror of the native `FUN_0006D920` (entered at match begin
  `FUN_0007412C → FUN_0008CE78 → FUN_0006D9C4 → FUN_0006D920`, FU-199 §5/FU-200
  §1). **Representation:** `fifa96_match_entities_bind_record_meta`
  (`fifa96_match_entities.c`) stages every record's `phase_meta`/`phase_meta10`/
  `phase_meta14` at `idx = (int8_t)byte[rec+0x8D]` (the pool `active`) with the
  native strides `idx*0x256` bytes / `idx*2` dwords / `idx*0x256` dwords. **The
  loader:** `match_run_record_meta_bind` (`fifa96_match_run.c`) decodes the
  GAMEART0 bank and copies the formation id's `.fmt` family
  (`352pl`/`352pk`/`freekick` for id 0 — the `0x107370` loader names, every
  6-name group {ko,pl,pk,ps,freekick,sp}) into the run-owned `mr->meta_owner`; it
   soft-fails (pointers NULL) with no assets/ISO. **Placement (errata, two-axis
   review):** the native position — immediately after the formation seed and
   **before** the derived slot bind and the screen-install handler. Fresh
   `disassemble_function` this review: `FUN_00092e2c` runs the match begin
   `FUN_00074034` (the phase-cell seed), then the bind `FUN_0007417c`
   (`FUN_0007412C` + the slot bind `FUN_00078824`), then the screen handler
   `0x92EF7 CALL [0x15B6D4]` that writes phase 0x15 — so the phase-0x15 write
   legitimately sees the bound metadata. The slice had placed the bind **last**,
   after the screen install, on the premise that the derived phase-0x15 walk
   preceded the bind; the review re-measured the native position
   **byte-identical** (`441415fc…`, `cmp` clean — the target write is
   render-invisible on this tape), so **no re-pin** and no artifact leg. **The
   bind is per-field, per-side:** the loader copies each side's three `.fmt`
   blobs independently, so a short/absent file (or an out-of-range id) clears
   only that side's own field's pointer — never a sibling field or the other side
   (the earlier all-or-nothing bind silently disabled all eight rows on one bad
   file).
  **The gate is not removed:** the native NULL check
  (`handlers.c:4594`/`:4669`/`:4730`) stays; the live path now supplies the
  pointer, so the eight per-record rows (`0x02`/`0x03`/`0x04`/`0x06`/`0x07`/
  `0x08`/`0x09`/`0x15`) perform their native work when the assets load, and stay
  gated without them. **Tape:** the on-tape rows DO execute (the `0x6DCC8` body
  measured at frames 209/766 (phase 2) and 363 (phase 0x15)) yet the M2 replay
  is **byte-identical** (`7b4b241a…`, 792 lines, `cmp` clean) — the target write
  is render-invisible on this tape, so **no re-pin** (FU200-L2 closed for the
  tape by measurement). M1 `09b726b7…` unmoved. **Census `48/80 = 60%`
  unchanged** — the eight rows were already wired/counted; T1 makes them perform
  their native work (fidelity, not a count). **Legs:** the
  `0x1577DE`/`0x157754`/`0x1577D6/D8` index selectors are staged 0
  (**FU200-L3** — the body runs at sub-index 0, not necessarily the native
  default), the `0x6D870` cell-map arm (**FU200-L4**), the `form*6+3` slot
  (**FU200-L6**), the `0x6D9C4` `[rec+0x90]` role byte (**FU200-L7**); FU-199's
  loader-content legs FU199-L1/L7 stand. **Tests:**
  `test_engine_match_entities::test_bind_record_meta_strides` (non-ISO
  stride/index bind), `test_engine_match_frame::test_record_meta_bind_live`
  (ISO; first-hand blob readback `352pl`/`352pk`/`freekick`) and the M2 v18
  metadata rider; mutation-RED = a wrong `pk` stride and a disabled bind each
  abort their test. The `test_engine_m2_noiso` rider stays green (the bind
  soft-fails without assets → pointers NULL → the rider is ISO-guarded).
- **M2 phase-18 T2 (FU-201/FU-204) — the order-matrix + act-4 formation arm:
  honest legs, docs-only, `+0` (census unchanged `48/80 = 60%`).** R49/FU-201
  proves the sit-2..4 act-4 arm (`0x8AB08..0x8AB62`) is **not live even under a
  faithful desired-order fill**: its gate `[0x157B8E]/[0x157B8F]` is a
  **permutation residual** (`FUN_0008C974` selection-sorts the current order
  `0x157BD2` onto the desired `0x14C360` and counts the unreconciled positions)
  and the match-init reset `FUN_00073EE0` copies desired→current — so after the
  reset `current == desired` and the residual is **0**. Un-gating
  `formation_order_ok` on R47's fill alone is a **no-op** (current copied) or,
  with current left zeroed, a **fabricated residual / fake producer** (R45). The
  genuine trigger is a runtime **non-permutation** of current vs desired
  (**FU201-L1**, the substitution/line-up divergence; carries FU197-L1) which no
  native routine in the fresh write census performs and no engine source emits.
  **T2 therefore does NOT un-gate the arm and adds no engine code** (per Ruling
  R15): action row `0x17` stays `{0x17, NULL, …}`
  (`fifa96_match_handlers.c:5012`) and phase `0x1A` stays `{0x1A, NULL, …}`
  (`:5111`) — both **NULL/uncounted (+0)**; the `form` doc strings
  (`port/src/fifa96_engine/fifa96_match_run.h:794..810`, the sorter `:1296`, the
  arm, the phase-`0x1A` bodies) now carry the §6 verdict. **The desired-order
  matrices are a staged leg (FU201-L4):** fresh this slice, the fill is a
  **front-end menu** producer (`FUN_00011620` has 5 front-end callers) and the
  engine has **no match-begin consumer of the desired matrix** — its only
  match-begin formation consumer, `match_run_formation_seed`
  (`fifa96_match_run.c:1911`), reads the `.fmt` **placement blob**, not the
  `0x14C360` descriptor-pointer order matrix; the reset copy is not modelled
  either, and the engine's only `formation_order` consumers are the gated
  sorter/arm/phase-`0x1A`. Scope correction (R49): action row `0x1A`
  (`0x7662C`, the keeper chain) is **not** the act-4 arm — the act-4 producer is
  phase `0x1A` (`0x89620`); FU-204 §10 carries the erratum. **Tape:** M1
  `09b726b7…` and M2 `7b4b241a…` **byte-identical** (no presented frame
  touched; `cmp`-verified — docs-only). **Tests:** the existing
  `action_expect[0x17]`/`phase_expect[0x1A]` (`UNSUP`) pins assert the leg
  posture and still hold; no test added (no code). **Legs:** FU201-L1 (the
  divergence producer), FU201-L4 (the setup's engine landing), FU201-L6
  (invoke-now has no generic engine port), FU181-L1 (row `0x17` entry); FU-204's
  `0x1A` ladder (FU186-L5), row-`0x0D` (FU178-L1) and the R50 `0x24`/`0x20` pair
  stand. README title `60%`; dispatch `31 UNSUP / 48 OK / 1 NOTF` — consistent.
  *(errata, M2 phase-25 W25-8 / FU-263: (i) the handler line-refs above are
  stale — the fresh action table (verified at HEAD `dad8665`) puts action row
  `0x17` at `fifa96_match_handlers.c:5508` and phase `0x1A` at `:5623`, not
  `:5012`/`:5111` (the `:5500`/`:5603` first recorded by FU248 §5 / FU263 §1.5
  were themselves pre-Task-2 and are superseded — `:5603` is now
  `{0x10, fifa96_match_phase_10,`); (ii) the M2 golden sha is
  `b281a9b2…` (v22, 6522 lines; FU247 §4.1 / FU258 baseline), not `7b4b241a…`;
  (iii) `FU201-L1`'s candidate owner is now named — `FUN_00041494` (`0x41494`)
  case 5, the in-match substitution swap `0x41B12`/`0x41B5F`/`0x41B91`
  (desired-only, reconciled in the same flow by the case-7
  `FUN_0008EAB8`/`FUN_0009343C`), so the leg is a dynamic-trace leg with a named
  native owner (FU248 §4.2/§4.3; FU263 Erratum 5). The old text is kept
  verbatim.)*
- **M2 phase-18 T3 (FU-202) — the native situation machine live + the
  phase-`0x20`/row-`0x24` pair wired/counted; M2 re-pinned (`+2`).** The native
  match-init producer `FUN_000493A0 → FUN_0004B02C(1)` (0x4B095, situation 0 at
  `0x4B0A5` → the sit-0 arm `0x8AB8E` invokes act `0xA` at `0x8B688`) is now
  **live on `begin`**: its call is **re-placed** to run *after* the begin-tail
  `fifa96_match_act_init` (which had cleared the armed act before any tick —
  FU-202 §3A, the old `:2244` gate was dead). T3 re-derives that the producer's
  armed act `0xA` then **survives into the first granted frame's clock-tail act
  tick** (`fifa96_match_act_tick`), which runs the now-registered phase-`0x20`
  body; its stage 0 installs code `0x24` on all 22 records and the now-registered
  action row `0x24` (`fifa96_match_action_24`) dispatches on the pool. **The two
  slots are wired/counted:** phase `0x20` (`fifa96_match_handlers.c:5123`,
  `fifa96_match_action_phase_20`) and action row `0x24` (`:5038`,
  `fifa96_match_action_24`) — **`+2` → `50/80 = 62%`** (32 action + 18 phase).
  **Tape (Ruling R15):** the row-`0x24` stage-0 `pos_x = 0x9F0` write is
  render-consumed (`match_run_scene_stage` reads `pos_x`), so **M2 re-pins**
  (`7b4b241a…` v17 → `441415fc…` v18, 792 lines; the frame diff is **frames
  12..792, hash-only — every `state=` suffix is unchanged**, `cmp`/diff
  measured). Isolated move-proof: disabling the re-placed producer stops the
  row-`0x24` dispatch (`mask_final` loses bit `0x24`), so the move is the native
  situation-0 chain, **not** the derived kickoff. **M1 `09b726b7…` unmoved.**
  **Retained/leg'd:** the derived ad-hoc `fifa96_match_run_kickoff_entry` stays
  at its begin position (the derived act-1 stage-0 stand-in) because the native
  **act-1 route** (situation 1 → act 1 → phase `0x17`) is **unported** — the
  situation-0 cutscene pre-empts the derived kickoff chain, so the phase-`0x17`
  row/body stays `NULL/uncounted` (**leg FU203-L2/FU202-L2**, the phase-2
  natural-chain gap). *(errata, M2 phase-18 T4: the act-1 route is now **live**
  — phase `0x17` is registered and counted, the derived ad-hoc begin kickoff is
  re-expressed through it, and a begun run again reaches phase 2 naturally; the
  section above is the T3 record, kept verbatim — see the phase-18 T4 bullet.)*
  The coupled phase-`0x1D`/row-`0x20` pair is **not wired**
  (its situation-8 route needs the unported extra-time flag + the actor-record
  producer; **leg FU205-L2**); phase `0x1E` stays `NULL` (the compressed act-8
  fallback, FU194-L2). The extra-time flag `[0x157AC0]` (OL-85) has **no live
  engine cell** — **leg FU202-L1** (not faked); its situation-8 gate/producer is
  nevertheless **fixtured** by the pre-existing
  `test_engine_match_handlers::test_period_setup_mode1_extra_time_sit8`
  (`input_range & 0x100`) and `test_period_setup_mode0_extra_time_sit8`
  (`global_157ac2 == 4`) — the brief's "extra-time writer fixtured" gate item,
  satisfied by those fixtures (the flag writer itself stays the FU202-L1 leg).
  **Tests:** the situation
  producer pin `test_engine_match_handlers::test_situation_producer_live_at_begin`
  (armed act `0xA` after `begin`), `test_engine_match_act::test_phase_20_body_direct`
  (the wired slot runs the 22-record install) and the wired-slot row-`0x24`
  dispatch; the M2 census riders (`action_wired == 32 && phase_wired == 18`,
  `phase_expect[0x20] == OK`, `action_expect[0x24] == OK`) pin the count. The
  derived-kickoff fixtures (`test_kickoff_enters_phase2_when_cutscene_isolated`,
  `test_natural_phase2_never_scores`, the record-machine live tests) now **clear
  the armed act** to isolate the derived path from the live situation-0
  cutscene. README title `62%`; dispatch `29 UNSUP / 50 OK / 1 NOTF` (sum 80).
  *(errata, M2 phase-18 T4: the current dispatch is `28 UNSUP / 51 OK / 1 NOTF`,
  README title `63%` — see the phase-18 T4 bullet; the T3 figures are kept
  verbatim.)*
- **M2 phase-18 T4 (FU-203) — the act-1 route live: phase `0x17`
  wired/counted; M2 byte-identical (no re-pin).** R51/FU-203 §2: the act-1
  kickoff/phase-1 entry `0x88DC8` has exactly **one** static reference — the
  phase/act dual-identity slot `0x110794[0x17] == 0x1107EC[1] == 0x88DC8`
  (fresh `get_xrefs_to 0x88DC8` = **1**, from `0x1107F0`), so the native kickoff
  is reached **only** through the act-1 route (there is **no** direct
  match-start call; T3's completion gap). T4 **registers phase `0x17`**
  (`fifa96_match_handlers.c`, `fifa96_match_action_phase_17`) and **retires the
  derived ad-hoc stage-0 model**: the begin-time
  `fifa96_match_run_kickoff_entry(mr)` call is **re-expressed** as
  `fifa96_match_act_dispatch(mr, 1, 0, 1)` (R51 §5 rec 1), so the kickoff is
  entered through the act-1 route whose phase-`0x17` body runs
  `fifa96_match_run_kickoff_entry` at stage 0. **The producer is LIVE:** the
  phase-`0x20` body's stage-2 terminal re-dispatches situation 1
  (`fifa96_match_run_situation(mr, 1)`, native `0x8B85D`), the loader row arms
  act 1 (`match_run_situation_side` → `fifa96_match_act_dispatch(mr, 1, 0, 0)`),
  and the clock-tail `fifa96_match_act_tick` maps act 1 → phase `0x16+1` and
  dispatches the now-registered body (the sit-1 arm `0x8ABAB`, the sole producer
  `0x8ABCF`, census 1). **Counted `+1` → `51/80 = 63%`** (32 action + 19 phase).
  **Tape:** the registered body fires only after the phase-`0x20` `0x3E8`
  cutscene wait (~500 granted frames), **past the scripted tape's forced window**
  (the tape self-terminates ~792 frames), so **M2 is byte-identical (`cmp` clean
  against the v18 `441415fc…` golden — NO re-pin)**; M1 `09b726b7…` unmoved. The
  begin re-expression is observably identity: the phase-`0x17` stage 0 runs the
  same `fifa96_match_run_kickoff_entry`, `global_5882a` is already 0, and the
  begin-tail `fifa96_match_act_init` clears the armed act (`cmp`-verified). **The
  T3 gap is closed:** the natural probe (no directives) now observes the act-1
  arming at engine step **1679** and the **live phase 2 at step 1802**
  (`test_engine_m2`), the probe's post-cutscene KICK release (M2_KEYS
  `[1794..1799]`, past the scripted window) supplying the FU-70 slot release the
  kickoff taker waits for (mirroring the native player kick). **Legs (row
  `0x1C` stays NULL/uncounted):** the act-6 producer `0x89110` rides the
  **offside** path `FUN_00079D5C` (`AX=3`), whose engine counterpart is the
  dormant offside machine (`REF_OFFSIDE`, no live producer) — wiring the row
  would **double-model** the same body (**FU203-L3/L4**). The frame-body derived
  `global_5882a` arm (`fifa96_match_run.c:3908`) is **kept** as the derived
  stage-1 remainder (idempotent with the phase-`0x17` stage-1 arm; the
  isolated-kickoff fixture + the row-`0x14` release path use it) — **leg
  FU203-L2** (its full retirement is a follow-up). **Name collision:** PHASE row
  `0x17` is distinct from ACTION row `0x17` (the act-4 formation-arm installer,
  `fifa96_match_action_table[0x17]`, NULL/leg) — kept distinct (FU-203 §2.4).
  README title `63%`; dispatch `28 UNSUP / 51 OK / 1 NOTF` (sum 80). Tests:
  `test_engine_match_act::test_phase_17_counted` (mutation-RED: a NULL slot →
  UNSUPPORTED and the phase unwritten), the natural-probe act-1/phase-2
  observations, and the M2 census riders (`action_wired == 32 && phase_wired ==
  19`, `phase_expect[0x17] == OK`, `phase_table[0x17] != NULL`).
- **M2 phase-18 T5 (FU-205) — the remaining producer-gated bound: the gated-no-op
  gate truth-table pinned; the residual rows stay honest legs; census unchanged
  `51/80 = 63%` (`+0`); M2 byte-identical (no re-pin).** The eight
  dispatched-but-gated per-record rows (`0x02`/`0x03`/`0x04`/`0x06`/`0x07`/
  `0x08`/`0x09`/`0x15`) collapse onto three bodies, each with a **single**
  semantic gate — the OL-13 metadata `NULL` early-return (`phase_meta` `[rec+0xC]`
  rows `0x02`/`0x08`/`0x09`/`0x15`; `phase_meta14` `[rec+0x14]` rows `0x03`/
  `0x04`/`0x07`; `phase_meta10` `[rec+0x10]` row `0x06`, `fifa96_match_handlers.c`
  `:4595`/`:4671`/`:4733`). T5 pins the truth-table cell-per-row
  (`test_engine_match_handlers::test_gated_rows_gate_truth_table`): asset-absent
  (`NULL`) → disciplined no-op (target untouched); asset-present (the T1 bind
  `fifa96_match_entities_bind_record_meta`) → the body runs (target rewritten);
  wrong-field (only the *other* two pointers bound) → still no-op, so the gate is
  exactly each body's own field. **T3/T4-opened check: `+0`** — the live
  situation-0 → act `0xA` → phase `0x20` → row `0x24` chain (T3) and the act-1
  route (T4) install neither row `0x0D`/`0x1A`/`0x20` nor dispatch phase
  `0x1D`/`0x1E`; the situation-8 producer (act 7 → phase `0x1D` → row `0x20`) is
  still extra-time-gated and the phase-`0x1D` slot stays `NULL`. **Residual
  frozen rows (honest legs, no fake producer):** row `0x0D` — the row-09 arm-1
  installer at `0x80F61` (`BA 0D 00 00 00`, `CALL 0x7D9A4` at `0x80F6A`, fresh
  disassembly), inside the unported row-09 arm 1, **FU178-L1**; row `0x1A` —
  the unported keeper CPU ladder `FUN_00077EAC` (its three `#x1A` installers;
  fresh `get_xrefs_to 0x77EAC` = 1 into `FUN_000782D0`), **FU186-L5**; the coupled
  phase-`0x1D`/row-`0x20` pair (`0x899A2` `MOV EDX,0x20`; fresh phase-slot
  `0x110808` = `0x89868`), **FU205-L2**; phase `0x1E` (act-8 compressed
  fallback), **FU194-L2**. *(This supersedes the plan brief's pre-T3 frozen list,
  which named row `0x24`/phase `0x20` — T3 made that pair live; §Addendum of
  FU-205 records the correction.)* **Tape:** M2 `cmp` clean against the v18
  `441415fc…` golden (**NO re-pin**); M1 `09b726b7…` unmoved. Dispatch
  `28 UNSUP / 51 OK / 1 NOTF` (sum 80).
- **M2 phase-18 T6 (FU-206) — acceptance: the tape strata v18 + the producer
  riders; census unchanged `51/80 = 63%`.** `port/tests/test_engine_m2.c` is
  re-labelled **v18** (the phase-18 producer-spine acceptance strata; the
  phase-18 T3 re-pin documented, no further re-pin). **Tape decision:** M2 was
  **re-pinned once** at T3 (`7b4b241a… → 441415fc…`; frame 12 first diff,
  781/792 lines **hash-only**, 0 `state=` suffix diffs — the situation-0
  cutscene's render-consumed row-`0x24` `pos_x` write, the earned T3 move per
  FU-202 §6 rec (b)); T4/T5/T6 moved **no** presented frame — M2 is
  **byte-identical** at `441415fc…` (`cmp` clean), M1 `09b726b7…` unmoved. Any
  *unexplained* drift is a regression to investigate, never a re-pin. **Producer
  riders (`run_phase18_riders`; every rider mutation-RED):** (1) the **silent
  re-gate** — the phase-`0x20`/row-`0x24` slots are installed *and* the
  phase-`0x20` stage-0 body installs code `0x24` on all 22 records (a NULLed
  slot aborts here / `run_census_riders`; a broken install chain aborts here);
  (2) the **double-run** — the begin kickoff is entered through the live act-1
  route, one entry writes `prev_phase 0` (a re-instated derived
  `fifa96_match_run_kickoff_entry` beside the route moves `prev_phase` to 1 and
  aborts `run_tape`'s begin `prev_phase == 0`); (3) the **metadata slip** — the
  T1 bind opens the per-record gate (a bound record's body rewrites the target;
  an unbound record is a no-op), so an undone bind aborts here and at
  `run_tape`'s ISO bind assert. **Census reconciled against R54/FU-206**
  (fresh first-hand re-verification: action `32` + phase `19` = `51/80 = 63%`;
  dispatch `28 UNSUP / 51 OK / 1 NOTF`, sum 80; the two tables
  `0x1106E0`/`0x110794` + the installer xref census are byte-identical to
  FU-198/FU-190). **Smoke:** the windowed SDL3 engine's build/boot posture is
  unchanged since the T4 smoke (§ below); no new PNG artifact is warranted — the
  phase-18 landings moved no *live* pixels that a fresh capture would show
  (the M2 golden's only move is the directive-forced cutscene at `begin`, and
  no visual/menu change landed). `make check` **110/110**. Remaining legs: the
  T1–T5 numbered legs (§ phase-18 T1/T2/T3/T4/T5) + **FU206-L1..L5**.
- **M2 phase-19 T2 (FU-208/FU-209) — the situation-8 selection + the phase-`0x1D`
  slot: phase `0x1D` counted, census `51/80 → 52/80 = 65%`.** The T1/FU-213
  period-index roll (`mr->global_157ac2`) now feeds the situation-8 selector:
  `fifa96_match_run_phase_drive` runs the mode-0 period-end producer
  (`fifa96_match_run_period_setup(mr, 0)`, native `FUN_00088860` `0x888C8`) once
  the rolled index reaches 4, whose `state.phase == 0x0C && global_157ac2 == 4`
  gate selects situation 8 → act 7; the registered phase-`0x1D` slot
  (`fifa96_match_handlers.c` `{0x1D, NULL}` → `fifa96_match_action_phase_1D`)
  then dispatches the ported body through the clock-tail act tick (arm 0 clears
  `[0x15882A]`, writes phase `0x10` and installs action row 0x19). Legs (not
  faked): the bit-8 sit-8 disjunct (`0x4B0CB`, **FU208-L1**; no located range-word
  writer), the coupled action row-0x20 body/installer (FU194-L7/L8, T3), the
  phase-`0x10` row (FU214-L4), the act-7 invoke-now cadence (**FU208-L4**) and the
  arm-9 `[0x157AC0]` write-back (FU209-L5); FU208-L2 (the NULL phase-`0x1D` slot)
  is RESOLVED. **Tape:** M2 `441415fc…` byte-identical (the tape reaches no
  `global_157ac2 == 4` completion), M1 `09b726b7…` unmoved; dispatch
  `27 UNSUP / 52 OK / 1 NOTF` (sum 80); README title `65%`; AGENTS.md census note
  swept.
- **M2 phase-19 T3 (FU-210) — the action row-`0x20` body + the phase-`0x1D`
  arm-0 installer: row `0x20` stays a leg, census `52/80 = 65%` (unchanged).**
  The row-`0x20` body `0x84EEC..0x85213` (the 7-arm CS table `0x84ED0`) is now
  ported as `fifa96_match_action_20` (arm 0 = the `0x73E08` commit + the
  `0x79B6C` place `pos_x/y/z` + the `0x8CFAC` anim `0x26`; arm 3 = the control
  slot predicate; arm 4 = the `0x7B9C4` mode-`0x40` kick `pos_x/pos_z` + anim;
  arm 5 = the `0x79B1C` snap; arm 6 = the `0x7DAB4` reset; arms 1/2 advance),
  and the phase-`0x1D` arm-0 coupled installer `0x899A2`
  (`FUN_0007D9A4([0x1587D4], 0x20, 0, 0)`, deferred) is ported — gated on the
  new `mr->actor_1587d4` cell. But the installer **target** is the native actor
  record `[0x1587D4]`, whose producer (the sit-8 arm's `[0x1587CC + side*4] ->
  [0x1587D4]` bind `0x8AE4B`) is unported (**FU210-L3** / FU194-L8): the cell
  stays NONE live, so no record ever carries code `0x20` and the row **cannot
  dispatch** — it stays `NULL`/uncounted (the rows-27/29/2C precedent; body
  exported for direct unit testing, installer fixtured). The row-`0x20` evidence
  now cites FU-210; the **FU210-L1** helper ports, the **FU210-L2** ball-record
  place and the **FU210-L3** actor-cell producer stay legs; FU189-L1 (the
  installer hunt) is RESOLVED. **Tape:** M2 `441415fc…` byte-identical (the tape
  reaches no sit-8 / no code-`0x20` record, and the cell defaults NONE), M1
  `09b726b7…` unmoved; dispatch `27 UNSUP / 52 OK / 1 NOTF` (sum 80); README
  title `65%`; AGENTS.md census note unchanged.
- **M2 phase-19 T4 (FU-211) — the phase-`0x22` / act-`0xC` registration: phase
  `0x22` COUNTED, census `52/80 → 53/80 = 66%`.** The already-ported act-`0xC`
  extra-time-SET three-stage timer body (`fifa96_match_action_phase_22`, native
  `0x8B900`; the `0x110794[0x22] == 0x1107EC[0xC]` FU165-L9 dual identity) is now
  registered (`fifa96_match_handlers.c` `{0x22, NULL}` →
  `fifa96_match_action_phase_22`). Its producer is LIVE: the T1/FU-213 OL-85 flag
  `mr->global_157ac0` drives the derived `FUN_0008B9CC` chooser
  (`fifa96_action_phase_period_end`) to `out.act = 0x0C` on the extra-time arm
  (W2 `0x8B2AC`/W4 `0x8B3A2` set the flag on a regulation-end / ET-2 completion
  with differing scores; W5 `0x8B5CD` is the mid-ET set), and
  `fifa96_match_run_phase_drive` dispatches it (`0x8BAE7 CALL 0x888FC(0xC,0,BX=1)`),
  whose act-to-phase-`0x16 + act` map reaches this row. The body is
  render-invisible (the act slot + the `0x974DC(0x1E)` audio sink staged as
  `ref_whistle`; no record, no phase write, and it does not write `[0x157AC0]` —
  FU-211 §3). The end-of-match word `[0x158822]` / `FUN_0008BAF0` stays the
  **FU213-L3** leg (the engine lifecycle owns exits; no faithful mapping without
  the unported end machine), the audio ring-push/audio-handle remainder is
  **FU211-L3/FU195-L3** and the `word[0x15881A]` cell producer is
  **FU211-L2/FU195-L4**. **Tape:** M2 `441415fc…` byte-identical (the tape reaches
  no extra-time completion, and the body is render-invisible regardless), M1
  `09b726b7…` unmoved; dispatch `26 UNSUP / 53 OK / 1 NOTF` (sum 80); README title
  `66%`; AGENTS.md census note swept.
- **M2 phase-19 T5 (FU-212/FU-214) — the phase-`0x1E` honest bound + the
  residual-leg register: census unchanged `53/80 = 66%` (no row wired).** The
  phase-`0x1E` / act-8 restart sweep (native `0x8A798`) keeps the engine's
  **compressed** model and is **not** wired this phase. FU-212 §6 (fresh
  first-hand) proves the faithful 22-record sweep is a **timing-only** delta: it
  is render-invisible (stores a selected pointer into `[0x1587F0]`, clears
  `word[0x157A68]`; it never rewrites a code) and the M2 tape never enters the
  compressed `set_piece(bx != 0)` path (the golden carries no `state=3`), so
  de-compressing would move no presented frame — the slot stays `NULL` and the
  model is **pinned** (`port/tests/test_engine_match_frame.c::
  test_phase_1E_compressed_model_pin`; the `run_census_riders` +
  `test_phase_1E_not_counted` NULL pins stand; mutation-RED: registering the slot
  aborts the pin). The **de-compression leg is FU212-L1** (owner FU-212; reference
  FU194-L2). The cohort's residual legs are registered, each with its native
  owner: **FU207-L2** (the OL-85 flag's front-end match-init producer `0x389A4`;
  the **FU207-L1** flag *cell* was resolved by T1 — owner FU-207), **FU208-L1** + **FU214-L2**
  (the bit-8 sit-8 range-word writer gap — no located writer), **FU208-L8** (the
  sit-8 session-gate co-condition `[0x14C32A]`), **FU209-L1** (the act-7 producer
  inert path — resolved for the `global_157ac2 == 4` arm by T2, the bit-8 arm
  stays FU208-L1), **FU213-L1** (the `[0x157AC0]` cell/producer — resolved by T1,
  the front-end stays FU207-L2), **FU213-L2** (`period_setup(mode 0)` live re-arm
  — resolved for the sit-8 path by T2), **FU214-L1** (the cohort's producer
  dependency — partly resolved; action `0x20` still blocked by FU210-L3). **FU211-L1
  is RESOLVED** by T4 (not re-legged). No fake producer. **Tape:** M2 `441415fc…`
  byte-identical (no presented frame moves), M1 `09b726b7…` unmoved; dispatch
  `26 UNSUP / 53 OK / 1 NOTF` (sum 80); README title `66%`; no golden re-pin.
- **M2 phase-19 T6 (FU-214) — acceptance: the tape strata v19 + the
  period-end-spine producer riders; census unchanged `53/80 = 66%`.**
  `port/tests/test_engine_m2.c` is re-labelled **v19** (the phase-19
  period-end-spine acceptance strata). **Tape decision: strata-only label bump,
  no re-pin** (the phase-15 v14→v15 precedent). The phase-19 landings moved **no
  presented frame** at any point — every producer completes only past the
  scripted tape's path (no period-end completion reaches the roll's
  `global_157ac2 == 4` sit-8 gate; the coupled row-`0x20` actor-cell producer is
  unported) and the registered phase-`0x1D` / `0x22` bodies are render-invisible
  (a phase write, an act slot, row-`0x19`/`0x20` installs on off-tape records,
  the `0x974DC(0x1E)` audio sink) — so M2 is **byte-identical** at `441415fc…`
  (`cmp` clean) across T1–T5, M1 `09b726b7…` unmoved. The golden was **not**
  touched. Any *unexplained* drift is a regression to investigate, never a
  re-pin. **Producer riders (`run_phase19_riders`; every rider mutation-RED):**
  (1) the **silent re-gate** — the live `global_157ac0` flag drives the chooser,
  a differing-score regulation-end completion installs the extra-time arm (phase
  `0x13`, act `0xC`) and the registered phase-`0x22` slot runs (a reverted
  hardwired-`0u` chooser input aborts here); (2) the **wrong tie-break** — the
  T1 flag is raised only on a differing-score completion at the regulation end
  (index 1) or the ET-2 end (index 3), a tie / the first-half completion leaves
  it clear (an inverted difference test or a dropped index constraint aborts
  here); (3) the **selector slip** — the roll's index-4 gate drives the mode-0
  situation-8 selector → act 7 (a gate off the index-4 roll leaves act `0xB` and
  aborts here); (4) the **phase-slot dereference** — the registered phase-`0x1D`
  slot's arm 0 writes phase `0x10` + installs row `0x19`, and the coupled
  row-`0x20` install is gated on the actor cell, NONE live (a NULLed slot aborts
  `run_census_riders`; a dropped arm-0 phase write aborts this rider). **Census
  recomputed against R62/FU-214:** `53 OK = 32 action + 21 phase`; dispatch
  `26 UNSUP / 53 OK / 1 NOTF` (sum 80); `floor(100 × 53/80) = 66`. **Smoke:** the
  phase-19 landings are render-invisible and display-gated; the honest headless
  smoke (the null-backend M2 tape, byte-identical `441415fc…`) plus the
  `make game` build/boot posture is recorded (see the T6 report); no
  `docs/screens/*` frame moved (the phase-19 producers are off the windowed
  path). `make check` **110/110**. Remaining legs: the T1–T5 numbered legs
  (§ phase-19 T1/T2/T3/T4/T5) + **FU207-L2, FU208-L1/L8, FU209-L1 (bit-8 arm),
  FU210-L3/L5, FU211-L2/L3, FU212-L1, FU213-L3, FU214-L1/L2** — each with its
  native owner in the T5 register.
- **M2 phase-20 (FU-215…FU-222) — the sit-8 completion: phase `0x10` + action
  `0x20` counted, census `53/80 → 55/80 = 68%`; the residual-leg register.**
  The phase-19 period-end spine ends in the sit-8 route (mode-0 `period_setup`
  on the `global_157ac2 == 4` roll → situation 8 → act 7 → phase `0x1D`); phase
  20 closes its top-down tail without fabricating a producer. **T1/FU-219** fixes
  the act-7 invoke-now cadence (native `0x8AEAE MOV EBX,1` runs the phase-`0x1D`
  arm 0 on the dispatch frame; the port passed `invoke = 0`, `match_run.c:2890`)
  — the one-expression fix for FU208-L4 (resolved at FU219-L1). **T2/FU-222**
  registers the already-ported phase-`0x10` row, whose writer (the phase-`0x1D`
  stage-0 arm `FUN_000740A0(0x10)` `0x898DD`; port `match_run.c:6090-6091`) is
  live via the counted phase-`0x1D`, so the walk gate opens and
  `fifa96_match_phase_10` dispatches → `+1` phase (closes FU214-L4/FU222-L2, at
  the `53/80 → 54/80 = 67%` commit). **T3/FU-217** derives the actor-cell producer
  `[0x1587CC + side*4] → [0x1587D4]` (the sit-8 arm `0x8AE08..0x8AE54` of
  `FUN_0008A938`, guarded by `byte[0x157A4C] != 1`), ports it
  (`match_run_free_record` + the `set_piece` re-init + the per-side
  `actor_1587cc[2]` cells) and flips the row-`0x20` slot → `+1` action (closes
  FU210-L3/FU217-L1/L2; census `55/80 = 68%`). **T4/FU-220** adds the row-`0x20`
  arm-0 ball-record place projection (native arm 0 writes the actor team's ball
  record `[[actor]+0x7A6]` at `(0, 0, 0xB10)`; the port maps the reachable `pos`
  subset onto `mr->entities.ball`). **Tape:** M2 `441415fc…` byte-identical across
  T1–T4 (the tape reaches no `global_157ac2 == 4` completion, so sit 8 never fires
  and the row-`0x20` body dispatches 0 times), M1 `09b726b7…` unmoved; dispatch
  `24 UNSUP / 55 OK / 1 NOTF` (sum 80); README title `68%`. **No fake producer:**
  the dead bit-8 gate (FU-215 — no instruction sets bit 8 of `word[0x14C1D4]` /
  `word[0x14C1D6]`; the only builders write bits `{0..7, 9}` or 0) is documented,
  never synthesized. **FU211-L2 is dissolved** (the phase-`0x22` gate cell is
  `word[0x158818]`, the act timer, not `word[0x15881A]`; FU221 §3 / FU-170 §4), so
  the ported `mr->act.timer >= 0x3C` is faithful — no leg. **Residual legs (each
  with its native owner):** **FU215-L1..L5** — L1 the bit-8 gate's intent vs the
  bit-9 config mapping (`0x4B0CB TEST AH,1`; owner FU196-L1/FU208-L1); L2 the
  front-end `[0x157AC2] = 4` producer `FUN_00038630` `0x38990` (owner the front-end
  setup slice / FU202-L7); L3 `FUN_00088860`'s live caller `0x74244` (no Ghidra
  function; owner FU196-L6); L4 the M2 tape never arms sit 8 (owner the
  tape/phase-drive slice); L5 the `0x105270`-family config-block producer chain
  (owner the input-options slice / FU-148 §12). **FU216-L1/L3** — L1 the unnamed
  sibling body `0x741C4..0x744CE` (a third range-word reader + bit-8 → sit-8 site
  `0x74312`; entry/reachability unresolved); L3 the engine's missing analog of that
  blob. **FU217-L1/L2/L3/L5** — L1 the `FUN_000897F4` scan exactness + the `+0x9A`
  occupancy source; L2 the phase-`0x1D` arm-8 no-winner rebind `0x89DCF`; L3 the
  per-side cells' consumer; L5 the `[0x157AAF]` side store (no engine cell).
  **FU219-L1..L6** — L1 the invoke-now cadence (**LANDED** by T1); L2 the seam
  act-change latch `0x88908`; L3 the arm-0 per-record byte writes (`+0x9B` / anim /
  row-3 loop; owner FU194-L7); L4 the native frame-boundary gate `0x4950F`; L5 the
  arm-9 `[0x157AC0] = 1` write-back (owner FU209-L5); L6 the fall-through-vs-`switch`
  body shape. **FU220-L1..L6** — L1 the FU210-L2 ball-record place remainder; L2 the
  two `0x8CFAC` 11-stride anim walks; L3 arm-4 `0x7A490` record write-back
  (local-copy-only); L4 the arm-0 `0x73E08` block commit; L5 the ball-camera cells
  `0x15774C/50/54` + input words + `0x4C324`/`0x4C374` sinks; L6 the prologue
  `stage92` signedness. **FU221-L1** — the `0x974DC(0x1E)` audio subsystem
  (`0x974DC` → the `0x155C8C` ring → the `FUN_00065CC0`/`FUN_000A7705` voice
  trigger) is unported and **parked** (out of scope), never counted. **FU208-L8**
  (the `[0x14C32A]` sit-8 session-gate co-condition; FU218-L3) stays a leg; the
  FU218 carrier legs fold into FU215-L1 (L1), the landed FU219-L1 (L2) and the
  landed FU217 bind (L4).
- **M2 phase-20 T6 (FU-214) — acceptance: the tape strata v20 + the
  sit-8-completion rider classes; census unchanged `55/80 = 68%`.**
  `port/tests/test_engine_m2.c` is re-labelled **v20** (the phase-20 sit-8-completion
  strata). **Tape decision: strata-only label bump, no re-pin** (the phase-15
  v14→v15 / phase-19 v18→v19 precedent). Every phase-20 landing is tape-inert —
  the T1/FU-219 cadence fix fires only on the dormant sit-8 route, the T2/FU-222
  phase-`0x10` writer and the T3/FU-217 coupled row-`0x20` install both gate on
  the sit-8 actor, and the T4/FU-220 ball place lands only inside the row-`0x20`
  body — so M2 is **byte-identical** at `441415fc…` (`cmp` clean) across T1–T5,
  M1 `09b726b7…` unmoved. The golden was **not** touched. Any *unexplained* drift
  is a regression to investigate, never a re-pin. **Rider classes (not
  duplicated; T1/T3 landed them at the v18/v19 layer; every one mutation-RED,
  re-verified first-hand in this task):** (1) the **cadence** — reverting the
  act-7 invoke-now (`match_run.c:2915`) aborts `run_phase19_riders` (block 3,
  `mr.state.phase == 0x10`); (2) the **phase-`0x10` registration** — NULLing the
  slot drops `phase_wired` and aborts `run_census_riders` (`:3020`), reverting the
  arm-0 phase write aborts block 4; (3) the **sit-8 bind** — setting
  `global_157a4c = 0` (`match_run.c:3166`) aborts `run_phase19_riders` (block 4,
  `:3664`); (4) the **ball place** — dropping `ball.z = 0xB10` (`handlers.c:5035`)
  aborts `test_action_20_body_arms` (`test_engine_match_handlers.c:1001`).
  **Smoke (honest):** `make game` under `DISPLAY=:1` opened the 960×720 window,
  booted the ISO and reached the match — the post-intro front-end frame is
  byte-identical (AE = 0) to `docs/screens/p9-v9-frontend.png` and the live match
  tracks the phase-8/9 HUD record; no `docs/screens/*` moved (the landings are
  render-invisible). **Census recomputed:** `55 OK = 33 action + 22 phase`;
  dispatch `24 UNSUP / 55 OK / 1 NOTF` (sum 80); `floor(100 × 55/80) = 68`.
  `make check` **110/110**. Remaining legs: the phase-20 leg register above
  (**FU215-L1..L5, FU216-L1/L3, FU217-L1/L2/L3/L5, FU219-L2..L6, FU220-L1..L6,
  FU221-L1** parked, **FU208-L8**) — each with its native owner.
- **M2 phase-21 T6 (FU-223…FU-230) — acceptance: the live period-end tape v21
  (the re-pin) + the phase-21 rider classes; census unchanged `55/80 = 68%`.**
  `port/tests/test_engine_m2.c` is re-labelled **v21** and the golden is
  **re-pinned** (the phase-14 T3 v13→v14 form; the written reason + frame diff
  live in the v21 provenance block and the phase-21 T6 report). T4/FU-225 wired
  the per-frame mode-0 producer and T5/FU-227/FU-228 re-pointed the `mark_over`
  lifecycle, so the committed v20 tape no longer stops at the goal restart: the
  scripted run drives the FU-62 clock through **two live period completions** —
  period 0 → 1 (half-time) and period 1 → 2 (regulation end → extra-time arm) —
  and stops at **extra-time entry** (spec §2 non-goal: no full ET match).
  Measured schedule (FU230-L1; never invented): the period-0 completion at
  engine step 845 (roll `global_157ac2 = 1`); `M2_M_HALF_OPEN` (m 841) opens the
  ordinary-match session (`[0x14C32A] = 0`, FU-227 §2.1/§2.2 — the period-end
  route's caller requires it) so the sit-1 half-time kickoff runs;
  `M2_M_HALF_TIME` (m 845) pins the phase-1 kickoff arrival; the period-1
  (differing, 1-0) completion at engine step 2502 raises the OL-85 flag
  (W2 `0x8B2AC`; errata E1: a differing reg end, not a tie) → phase `0x13` /
  act `0xC` / roll 2; `M2_M_ET_ENTRY` (m 2498) pins the extra-time entry.
  **Frame diff:** lines 1..792 byte-identical (`head -792 new | cmp - old`
  clean, 34820 bytes; `diff` = `792a793,2522`), first appended line 793; N =
  **2522** lines. Old M2 `441415fc…` (792) → new M2
  `0b87fd18ad41948c8294a6cc46e40af02c8bfe5e92d708e09c3a1a2103c947c0` (2522);
  M1 `09b726b7…` unmoved. **Rider classes (every one mutation-RED,
  `run_phase21_riders`):** the half-time sit-1 route, the reg-end → ET arm, the
  tie-does-NOT-arm-ET polarity discriminator (errata E1), the roll → the
  index-4 sit-8 gate, the ET-entry phase-`0x1D` slot dereference, the
  clock-completion schedule (the `main` `half_step`/`et_step` anchors), and
  additivity/determinism (the 3-replay gate + the `head -792` prefix). The
  sit-8 extra-time *kickoff* (`global_157ac2 == 4` → act 7 → phase `0x10`)
  stays off the tape: the native bit-8 `[0x4B0CB]` range-word gate that raises
  the index to 4 is **dead** (no instruction sets bit 8; spec §2 "No dead-gate
  synthesis", FU-215); `main`'s `et_phase == 0x13` records the reachable
  extra-time entry. `make check` **110/110**. **Smoke (honest):** `make game`
  under `DISPLAY=:1` opened the windowed SDL3 engine, booted the ISO and drew
  the intro/front-end for 12 s with no crash (the SIGTERM shutdown
  LeakSanitizer report is from `libdbus`/`libnvidia-glcore`/SDL3, not the
  engine); the **live clock → completions reach is proven headless** by the v21
  tape (`test_engine_m2`, byte-stable over 3 replays).
- **M2 phase-22 (FU-231…FU-239) — the end spine (extra time → match over): the
  live extra-time halves, the ET-over terminal + the return; the M2 tape is
  re-pinned v21 → v22; census unchanged `55/80 = 68%`.** The phase theme is the
  **full end spine** (spec §2): the tape now plays both live extra-time halves
  and the end-of-match terminal, then stops. **The model correction (FU-239; the
  phase-22b unblock):** the ET half does **not** run in the class-0 phases
  `0x13`/`0x14` (their clock is halted) — it runs in the **class-1 play phase
  `2`** (the only class-1 phase in the `0x1106AD` table with any static writer),
  reached by the period-over re-arm (`fifa96_match_run_period_over_rearm`, T1;
  FU-239 §1/§3); `0x13`/`0x14` are the **ET-over screen dwell**. **T1/FU-239**
  lands the re-arm helper + the ET-half clock proof. **T2/FU-231** ports the
  `FUN_0008BAF0` end machine as the end word `[0x158822]` (word `1` = regulation
  over, word `2` = ET over). **T3/FU-232** re-points the frame-loop `mark_over`
  (word `1` **and** period index `>= 2`, minus the index-4/ET-flag skip; word `2`
  is a transient consume — **FU-232-L6 CLOSED**). **T4/FU-235** adds the
  phase-`0xC` terminal dwell + the match-over screen (`act.timer > 0x168`, native
  `0x8BCAC`) and the ET-entry dwell + the live re-arm. **T4b/FU-233·FU-237**
  bound the re-arm to the ET window `{2,3}` (not `>= 2`) and add the ET-over
  dwell (`act.timer > 0x4B0`, native `0x8BD5A`/`0x8BDAC`). **T5** extends the
  tape + re-pins. **T6** this acceptance. **The re-pin (T5, written reason +
  frame diff):** lines 1..2522 byte-identical (`head -2522 new | cmp - old`
  clean, 112487 bytes; `diff` = `2522a2523,6522`), first appended line 2523;
  N = **6522** lines; old M2
  `0b87fd18ad41948c8294a6cc46e40af02c8bfe5e92d708e09c3a1a2103c947c0` (2522) →
  new M2
  `b281a9b25b263e665a7f775f7d34046ffea5992c507e8b0ce9c662a162378f4a` (6522);
  M1 `09b726b7…` unmoved. **Two terminals:** the forced-ET tape reaches the
  ET-over **consume** (word `2`, same-frame resume; no over-screen, FU-232-L6);
  the word-`1` regulation-over (OVER screen → front-end return) is off the
  forced-ET tape and covered by `run_regulation_over_probe`. **Rider classes
  (every one mutation-RED, `run_phase22_riders`):** the word-`1` regulation over
  (the flag-clear phase-`0xC` terminal), the word-`2` ET-over terminal
  (`> 0x4B0`), the frame-loop consumer (`>= 2`), the live ET half (phase `2`
  clocks to the completion at `extra_length`), the front-end return (`run_end`
  detach + the FRONTEND flip), the screen timing gate (`> 0x168`), and
  additivity/determinism (the 3-replay byte-stability gate + the in-test
  `head -2522` prefix assertion). **Errata landed (append-only):** FU-233 §3 /
  FU-238 §2.3 → **FU-239** (the ET half is phase `2`; `0x13`/`0x14` is the
  ET-over screen; the phase-`0xC` gate is `0x168`, not the `0x4B0` dwell);
  **FU-213 §11** → FU-239 §1 (`FUN_00088860` is the dispatcher, not the re-arm).
  **Anchor reconciliation (append-only; the arm-vs-write-vs-latch roles):** the
  ET-over dwell is the arm's `CMP EAX,0x4B0` at `0x8BD52` (phase `0x14`) /
  `0x8BDA4` (phase `0x13`); the `word[0x158822]=2` write is `0x8BD8A` (phase
  `0x14`) / `0x8BDD4` (phase `0x13`); the `[0x10F360]` latch is `0x8BDDD`. The
  T4b cite `0x8BD5A`/`0x8BDAC` above is the arm's inner `CMP` byte, superseded by
  the arm-entry cite `0x8BD52`/`0x8BDA4`.
  `make check` **110/110**. **Residual legs:** **FU239-L1** (the exact
  `0x13 → 0x0C → sit-8 → phase 2` route — the situation dispatch is indirect),
  **FU239-L3** (`[0x157AC2]=4` vs the tape's forced ET), **FU-231** (the
  `[0x105FFC]=5` arm has no engine cell), **FU-234-L6** (the return's clean
  pinnability — resolved off-tape by the probe), **FU-233-L1/L5**, **FU-237-L1**,
  **FU239-L5** (the engine collapses `0x13`/`0x14` and omits the native
  phase-`0x13` score-equality guard `0x8BD95` (`[0x157AC5]==[0x157AC7]`), so the
  differing-score ET-over is a **Reading-B model reach** via the collapsed arm —
  owner FU-237/FU-239).
  No phase row wired; census `55/80 = 68%` unchanged.
- **M2 phase-23 (FU-242…FU-243) — the census/ceiling correction: the row-25 and
  row-15 installer refutations; census `55/80 → 57/80 = 71%`.** The phase theme
  is the **installer census** (spec §2): FU-242 §2 proves the action-table
  installer's parametric bulk path `FUN_0008CEB8(rec,i0,i1,code,skip)` carries
  the code in **`ECX`** (`B9`), invisible to the `MOV EDX/DX,code` literal scan
  the prior verdicts used. **T2/FU-243 §4.2/§6** re-derives the row-25 installer
  `0x8D79D`/`0x8D7AD` (`MOV ECX,0x25 -> FUN_0008CEB8 -> FUN_0007D9A4`,
  `[0x157AC5] != [0x157AC7] ? 0x25 : 3` on the `FUN_0008D098` situation-0x13
  arm) and **REFUTES FU189-L2** ("row 25 has NO static installer"); the installer
  is already modelled by `fifa96_match_phase_machine_step`. T2 ports the body
  `0x880CC..0x886AF` (the FU-82 §3.10 stats sequence, 7-arm CS table `0x880B0`)
  as `fifa96_match_action_25` and wires its action-table slot — the row is
  **COUNTED** (an honest +1; the `{0x00..0x26} ∪ {0x28,0x2A}` installer domain
  always contained `0x25`). Tape-dormant (no `M2` frame dispatches it; `b281a9b2…`
  unmoved). **Residual legs:** **FU243-L4** (the row-25 active head block
  `0x88106..0x881D2` — the `target := [team+0x59]` copy + the radius-`0xC0`
  `0x114E04` sine fold + the `0x8DCD4`/`0x79C50` payload, gated on the unported
  team base position and `[rec+0x8B]` angle cell), the `0x8776C` id-init and the
  `[0x587B8+idx]`/`[0x587BB]`/`[0x587BC]` anim compares (unported id tables,
  FU184-L1 sibling), the `0x974F0`/`0x36200`/`0x4C324` sinks and the `0x45001`
  input gate (FU184-L3/L4 siblings). The FU-189 row-25 evidence string / FU189-L2
  errata is the FU243-L1 leg (owner: a `docs(fu189)` errata).
  **T3/FU-243 §4.3/§6** re-derives the row-15 installer: the **shared `0x8D433`
  phase-5 arm** (`FUN_0008CEB8(rec, 1, 10, 0x15, -1)` + the actor `0x14` install)
  — the row-14 celebration sibling. This arm is **already ported** at
  `fifa96_match_run_phase_arm`'s `phase == 5` block (FU-173 §4.2 / FU-184), so
  **FU-243-L2's "the `0x8D433` arm itself is not ported" verdict is STALE** (that
  recon inspected only `fifa96_match_phase_machine_step`, whose states are
  `1`/`0x13`/`0x14`) and the FU-136/OL-14 "head mis-decoded, stub bucket" label is
  superseded. T3 ports the body `0x87CD0..0x880AE` (the 4-arm CS table `0x87CC0`)
  as `fifa96_match_action_15` and wires its action-table slot — the row is
  **COUNTED** (an honest +1, the row-14 precedent: same live installer, body with
  numbered legs). Tape-dormant (`b281a9b2…` unmoved). **Residual legs:** the
  radius-`0xC0` `[rec+0x8A]` fold, the `[0x1587B8]`/`[0x1587BB]` id tables and the
  `0x741B4`/`0x651F0` sinks are **FU243-L4** (the write is dropped, not faked).
- **M2 phase-23 T4 (FU-241) — the phase-`0x1E` restart-sweep de-compression;
  census `57/80 → 58/80 = 72%`.** FU-241 §6.3 lifts the FU-212 compressed act-8
  model: `fifa96_match_run_set_piece`'s `BX!=0` branch now dispatches act 8
  invoke-now (`fifa96_match_act_dispatch(mr, 8, 0, 1)`; the native `0x8AAA3 JMP
  0x8AF1A CALL 0x888FC(8,0,BX=1)`) instead of the in-frame stored-situation
  re-dispatch, and the phase-`0x1E` slot is registered — the row is **COUNTED**
  (an honest +1). The 22-record sweep defers the stored-situation/side
  re-dispatch (`0x8A8CE`) ~22 granted frames to its stage-2 terminal. FU-241
  §6.1/§6.2 (fresh first-hand) proves it **render-invisible** (stores a selected
  pointer into `[0x1587F0]`, clears `word[0x157A68]`; never rewrites a record
  code) and **dormant on M2** (the golden has no `state=3`, so no tape frame
  enters the `bx!=0` path). **Golden unmoved:** M2 `b281a9b2…` and M1
  `09b726b7…` byte-identical (cmp-verified). The de-compression defers the
  bx=1-driven hand-offs (goal_scan throw-in, set-piece stage-4 re-queue,
  phase-`0x1A`/`0x1C` terminals, ref/foul `ref_apply`, referee restart decision);
  the affected unit tests were re-expressed to drive the sweep to its terminal.
  **Residual legs:** **FU241-L1** (non-tape compression equivalence — the
  deferral on those non-tape paths), **FU241-L2** (`word[0x157A68]`, FU194-L4),
  **FU241-L3** (act-slot residency across the sweep), **FU241-L4** (the `+0x99`
  selection predicate), **FU241-L5** (the stage-2 re-dispatch target), **FU241-L6**
  (the `0x8A8E0` BX==0 table), **FU241-L7** (the stale FU-212 golden anchor).
  `make check` **110/110**. *(errata, M2 phase-26b, 2026-10-14 / FU279 §3(b):
  **`FU241-L1` is port-closed** — no port remains. The landed act-8 invoke-now +
  one-sweep-record-per-granted-frame model (terminal on the 23rd call) is
  **exactly the native cadence**; the seam
  `fifa96_match_act_dispatch`/`_tick` reproduces `FUN_000888FC` / `0x8B669..`
  instruction-for-instruction (`FU255` §1/§2, cross-cited). What stays open is a
  **test-coverage** residual (the non-tape deferral is unexercised by a landed
  fixture), **not a producer gap** — see leg `FU279-L5`. The residual-leg list
  above is kept verbatim; only its `FU241-L1` framing is corrected.)*
- **M2 phase-25 (FU-259) — the OL-65 row-07 gate staging fix wires action
  `0x22`; census `58/80 → 59/80 = 73%`.** FU-259 §2.4 closes OL-65 with the
  exact two-line fix in `fifa96_match_handlers.c:983-993`: the row-07
  `match_kick_from_record` opponent staging now re-sources the live pool cells
  `opp->cam_dz6f` (native `word[+0x6F]`) and `opp->face7d` (native `word[+0x7D]`)
  instead of hardcoding both to 0 (`opp->lane_z` = native `word[+0x6D]` is
  unchanged; its wrong `dword[+0x6D]>>16` alias comment is corrected — the field
  is the word itself). The row-07 post-kick gate (`0x816CC CMP EAX,0x55 / 0x816CF
  JGE`) folds `entity_angle(word[+0x6D], word[+0x6F]) − word[+0x7D]` and fires iff
  `< 0x55`; the old `0/0` staging made it unsatisfiable (`entity_angle(x,0)` is
  `±0x100` against `face7d == 0`), so FU185-L6's "structurally ungated in-engine"
  verdict is now stale. With the gate live, row-07's `0x816DF` invoke-now fires
  `match_kick_invoke_opponent` → `fifa96_match_entities_install(opp, 0x22)`; the
  action-table slot `{0x22, fifa96_match_action_22}` is registered (FU259-L2
  closed), so the installed row dispatches. **Install+dispatch witnessed** by
  `test_action_07_ol65_invoke_installs_row_22` (the fix provider; mutation-RED on
  either re-sourced field aborts it) plus `test_action_22_runs_body` (the body)
  and the census rider (`test_engine_m2.c`). **Tape-dormant**, FU259-L1
  accepted: `./build/test_engine_m2 | cmp - port/tests/golden/engine/m2-frames.txt`
  byte-identical — M2 `b281a9b2…` and M1 `09b726b7…` unmoved (the two re-sourced
  cells are consumed only by the row-07 post-kick gate, so the tape's staged
  row-07 kick does not reach `diff < 0x55` with live cells). **Residual legs:**
  **FU259-L3** (the natural-tape row-07 geometry — whether an unforced match
  yields `entity_angle(cam−opp_pos) ≈ opp->face7d` within `0x55` with a code-6,
  no-slot, `<0xD0` opponent; carried forward from FU247-L1).
- **M2 phase-25 leg register (FU-256…FU-263) — the census close-out's
  residual-leg register: the 8 frozen recon slices, each leg with its native
  owner; census holds `59/80 = 73%`.** The phase-25 wave (`port/spec/
  FU256…FU263`) re-derived the producer frontier behind every unwired row and
  proved no further honest wire remains (the sole survivor, OL-65, landed as
  FU-259 above). Every other candidate is an **honest numbered leg with a named
  native owner** — appended here; prior sections never rewritten. **Row `0x0D` /
  the row-09 arm-1 chain (FU-256 §8):** **FU256-L1** — the row-09 arm 1 body
  `0x80BFB..0x80FCB` unported (carries the sole `0x0D` installer `0x80F61` and
  the `0x0C` installer `0x80EFA`; owner **FU178-L1**); **FU256-L2** — the
  `0x0D`-install-enabling producers `[[rec+4]+0x16]` and `[rec+0x24]` (no
  producer located; owner **FU204-L2**); **FU256-L3** — the `[rec+0x3D]==2`
  frame condition (owner **OL-80 / FU-141**); **FU256-L4** — the arm-1
  helpers/sinks `0x8F188`/`0x8ED40`/`0x92820`/`0x7A490`/`0x8A3FC` (owner
  **FU180-L8 / FU204-L4**); **FU256-L5** — the row-`0D` body sinks `0x702F8`/
  `0x6E598`/`0x79C50` (owner **FU180-L8**); **FU256-L6** — the native
  `[rec+0x24]` producer reachability (owner the pairing/possession cluster,
  FU-139/FU-142 class). **Row `0x1A` (FU-257 §5):** **FU257-L1** — the CPU
  ladder `FUN_00077EAC` (`0x77EAC..0x782CD`, the `0x1A` installer; owner the
  keeper-ladder slice); **FU257-L2** — `FUN_000782D0` record-0 tail (owner the
  keeper-ladder slice); **FU257-L3** — the `FUN_0008D8EC` driver pre-pass
  callers (owner the frame-chain slice); **FU257-L4** — the `FUN_000765C4`
  tracked-teammate predicate (owner the keeper-chain slice); **FU257-L5** — the
  `[0x157C2C]` direction machine + movement/event block and stages 1–4 (owner
  the keeper-chain slice); **FU257-L6** — the `[0x157C5D]` consumer wiring
  (owner the keeper-ladder slice); **FU257-L7** — the tape question (an earned
  re-pin iff a ported ladder installs `0x1A` on M2; owner the ladder slice that
  lands L1/L2); **FU257-L8** — `FUN_0009E420` constant-code getter (ruled out
  as an installer; owner **none — closeable**). **Row `0x03` / the
  `[rec+0x1C]=0x6DCC8` chain (FU-258 §6), tape-visible:** **FU258-L1** — the
  exact frame-29 mover field (carries **FU246-L1**; owner **FU177-L1 /
  controller**); **FU258-L2** — why frame 29 (owner **FU177-L1**); **FU258-L3** —
  the doc erratum misattribution (**RESOLVED** by the T3 errata sweep `d32e3c3`,
  the quoted `ENGINE.md` phase-17 T3 erratum); **FU258-L4** — the `[rec+0x1C]`
  `EBX=-1` third argument (owner the row-03 rewrite chain); **FU258-L5** — the
  OL-13 re-stage remainder (carries **FU249-L1**; owner the FU197-L2
  order-routine producer). **The installer domain (FU-260 §7):** **FU260-L1** —
  rows `27`/`29`/`2C` installer-absence over the ECX-inclusive census (owner
  **FU-142f / the cluster-G close-out**; rows stay `NULL`); **FU260-L2** — row
  `2B` dead entry `0x87738 = RET` (owner **FU-142 §1.1**; OL-15 confirmed);
  **FU260-L3** — row `0A` installer reachability `0x7CDD8` xref-less (owner the
  row-0A body cluster / FU-75 OL; OL-14 confirmed); **FU260-L4** — the bounded
  `rec+0x91` writer census (owner **FU-260 / FU-142f**); **FU260-L5** — the
  other unwired rows `03`/`0D`/`17`/`1A`/`22` (owner each row's FU doc —
  FU177-L1, FU178/180, FU201/FU248, FU186-L5, FU185-L6). **The phase slots
  (FU-261 §5):** **FU261-L1** — the `FUN_0008A938` table-1 (`0x8A8E0`) /
  table-2 (`0x8A904`) arm→phase semantics (owner the `FUN_0008A938` phase-walk
  slice); **FU261-L2** — the three variable-situation call sites `0x895AC`/
  `0x897E3`/`0x8A8CE` (owner **FU143-OL-72**); **FU261-L3** — the
  `seed_formation` vs the native phase-`01`/`11`/`12` walk (`0x6E1D0`)
  equivalence (render-invisible on M2; owner: FU-261 (inferred)); **FU261-L4** —
  whether `0x6E1C8` (row 11) carries a meaning distinct from the `0x6E1D0`
  fall-through (owner: FU-261 (inferred)); **FU261-L5** — the `[0x157A4D]`
  non-setter writers `FUN_00073E28`/`FUN_00073D10` (both write 0; owner: FU-261
  (inferred));
  **FU261-L6** — the sharpened sit-7 census (**RESOLVED** as the ENGINE.md
  situation-7 note correction `d32e3c3`; owner leg **FU-146**). **The phase
  slots (FU-262 §6):** **FU262-L1** — the phase-`0x13`/`0x14` body `0x6E244`
  unported + the RNG tape-inertness gate (owner **FU-192 / FU-240**);
  **FU262-L2** — the phase-`0x14` NUMBER write not live (`probe[2]/[3]` cells
  hardwired `{0,0,0,0}`; errata **FU240-L1 corrected**; owner **FU-240 /
  FU-213**); **FU262-L3** — the phase-`0x1A`/act-4 producers present but
  dormant (`0x88EE8`/`0x8AB57`; owner **FU-201 / FU-193 / FU-244**);
  **FU262-L4** — phase `0x1B`/act 5 has no producer at all (owner **FU-195**);
  **FU262-L5** — the phase-`0x1C`/act-6 producer `0x8A789` unported as the row
  (owner **FU-193 / R8**); **FU262-L6** — the FU-239 collapsed arm (the engine
  omits the native `0x8BD95` score-equality guard + per-phase second gates;
  owner **FU-239 / FU-237**); **FU262-L7** — the clock-class table `0x1106AD`
  (`0x13`/`0x14`/`0x1B` class 0; no action). **The formation-arm errata
  (FU-263 §4):** **FU263-L1** — the FU247 OL-65 erratum promotion (**RESOLVED**
  `d32e3c3`; owner the FU247 doc owner / OL-65); **FU263-L2** — the OL-83
  unification + reader-list correction (**OPEN**; owner the FU-142/OL-83
  Stage-2 fidelity task; the `fifa96_match_run.h:503` site owed); **FU263-L3** —
  the `ENGINE.md` stale handler line-refs (**RESOLVED** `d32e3c3`; owner the
  controller at reconciliation); **FU263-L4** — the FU-84 §1 reroll erratum
  promotion (**RESOLVED** `d32e3c3`; owner the FU-84 doc owner / the anim
  task); **FU263-L5** — the `FU201-L1` owner annotation (**RESOLVED** `d32e3c3`;
  owner the FU201 doc owner). **Tape:** no golden re-pin (the register is
  doc-only); M2 `b281a9b2…` and M1 `09b726b7…` byte-identical; `make check`
  **110/110**. **The honest ceiling:** the row `0D`/`1A`/`03`/`0A`/`17`/`1B`
  and the phase-`0x13`/`0x14`/`0x1A`/`0x1C` slots stay `NULL`/uncounted — no
  fake installer, no fake producer.
- **M2 phase-26b leg register (FU273…FU280; 2026-10-14) — the Stage-2 producer
  close-out: the resolved/forbidden legs (appended; prior sections never
  rewritten). Census holds `59/80 = 73%` (`36 action + 23 phase`); M1
  `09b726b7…` and M2 `b281a9b2…` byte-identical (no re-pin).** The wave
  (`port/spec/FU273…FU280`) froze the remaining phase-7 producer frontier; the
  Stage-2 plan landed the live arms reachable **without R11-L4** (each
  tape-dormant, so the census is unmoved) and routed the rest honestly.
  **Landed:** T2 the `FU268-L10` eligibility-byte fix
  (`port/src/fifa96_engine/fifa96_match_run.c:6597`, the `+0x8E`/`type` read → the
  `+0x91`/`code` read — correctness, no re-pin); T3 the `sp_157821` shadow
  unified onto the single native cell `camera.pan_counter` (`fifa96_camera.h`,
  the shadow field deleted); T4 the contact gate **headless-proven reachable**
  via the real pad `0x7CE38` (`want 0x60`) handler (row-`0x0B` arm-1 →
  row-`0x0C` → `fifa96_match_run_contact` → act 3); T5 the `0x7A448` offside
  call ported **into the live receive arm**
  (`fifa96_match_run_ball_receive`, `field_4c2f2`-gated, dormant);
  T6 row `0x10` **live** (the real armer `run.c:4205` + scanner, the camera an
  input) with rows `0x12`/`0x13` an **honest-stop LIVE-LEG** (the final review
  corrects the blocker: the live path emits a **non-foul contact kind (1 ≠ 3)**;
  the native gate reduces to `contact_kind == 3 || session != 0`, i.e.
  `[0x158882] ≡ contact_kind == 3`, **already proxied** — so the next phase needs
  a live **`contact_kind == 3`** producer, not a `[0x158882]` port; FU280); T7
  row `0x1D` **live** (the `goal_scan` sit-4 emit; the goal-line excursion is a
  **staged camera pose** — the camera's own producer rides R11-L4). **This
  register appends the wave's
  leg resolution:** **FU274 W26b-L1** (the throw-in precondition) **resolved**
  (T6 — row `0x10` live through the ported armer/scanner; the camera is an
  input, not a missing producer); **FU274 W26b-L2** (the foul-adjudicator entry,
  the row-`0x0C` contact gate) **resolved to LIVE-LEG** (T4 proves the *entry*
  reachable; the row-`0x04` AI duel stays wired-but-unraised pending the
  unported descriptor/`+0x9D` family, `fifa96_outfield.c:595`); **FU276-L1**
  (the live contact invoker) **resolved** by T4 (the pad handler now raises it
  in a granted-frame test — the FU276 §1.3 "no headless proof raises it" premise
  is superseded); **FU277-L1** (the row-`0x1D` live trigger) **resolved** by T7
  (the armer + sit-4 scanner install and dispatch row `0x1D`; `goal_armed` is
  raised via the same live armer `FUN_0007131C`, i.e. **FU277-L1 ≡ FU269-L2,
  not R11-L4** — FU280 §1.4); **the `0x12`/`0x13` taker rows** route to the
  **FU166-L6** leg with the blocker corrected (the final review, FU280: the native
  act-2 stage-3 gate `0x894A2` reduces to `contact_kind == 3 || session != 0`;
  the `0x8941D` keep/reroll sets `[0x158882]=1` and the `0x89464` row-`0x49`
  install resets it 0 unless `[0x15888C]==3` (`0x8944A`), so `[0x158882] ≡
  contact_kind == 3` and is **already proxied** — the old "a `[0x158882]` port
  must flip this" reading is **dropped**; the true remaining blocker is a live
  **`contact_kind == 3`** producer, T6). **Leg (pre-existing divergence):** the
  engine's act-2 gate reads `foul_kind`/`[0x15888D]` (`run.c:6002`) where the
  native reads `contact_kind`/`[0x15888C]` (`0x894AB`);
  **R11-L4** (the keeper CPU chain `FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC`,
  rows `0x1A`/`0x1B`, the phase-6/`0x1F` chain, the claim bridge) stays **NO-GO /
  phase-scale** (`FU273`) and **routes onward to phase-26b-ii** as the enabling
  leg for the camera/pan/follow and the keeper row `0x1E`. The **replay /
  overlay / sub** leg is recorded separately in the smoke table below (`FU278`
  §3.3). **Honest ceiling:** no row wired (census unmoved), no faked producer;
  the `0x12`/`0x13` and `0x1E` legs each carry a named owner.
- **M2 phase-26c leg register (FU281…FU289; 2026-10-15) — the R11-L4 keeper CPU
  chain close-out: the quoted errata + the leg register (appended; prior sections
  never rewritten). Census holds `59/80 = 73%` (`36 action + 23 phase`); M1
  `09b726b7…` and M2 `b281a9b2…` byte-identical (no re-pin).** The recon wave
  (`port/spec/FU281…FU289`) froze the keeper-CPU producer frontier; the phase
  landed the bounded wins + the ported skeleton, and **no task proved a live
  install+dispatch**, so the census is unmoved (FU288). **Landed:** T2 the FU289
  contact-cell fix (below); T3/T4 the **R11-L1 claim bridge** + R11-L2/L3
  (`port/src/fifa96_loader/fifa96_keeper.c` — the `out->event`→`record.anim_id`
  consume, the `flag_157820`/`flag_157822` place pause, and the
  `place_offset_x/z` tables); T5 the row-`0x1B` stage-2 wide/steer + the `0x77423
  CALL 0x71C94` pan setter + the ungated self-actor `0x77568`→emit (below); T6
  the pre-pass reconciliation (below); T7 the `FUN_000782D0` record-0 machine
  skeleton; T8 the `FUN_00077EAC` ladder + the shared predicates; T9 the
  granted-frame release reach **GREEN** (Layer 1) + the **honest Layer-2 waiver**
  (below). **Errata (quoted):**
  - **FU289** — `[0x158882]` is **NOT** the contact-kind cell; it is a distinct
    ball-record terminal flag (fresh `get_xrefs_to 0x158882` = 1 READ at
    `0x894A2`; **no direct write xref** — the cell is written **via the helper
    `FUN_0006E724` at `0x8941D`**, the keep/reroll install). The contact-kind cell is **`[0x15888C]`** (`0x894AD
    MOV AL,[0x0015888c]; CMP EAX,0x3`); its **sole** writer of the value `3` is
    `0x8A772` in `FUN_0008A43C` (the **kind-3 offside arm**, `0x8A735 CMP
    EBX,0x3`), reached only from the offside check `FUN_00079D5C 0x79F2B`. The
    engine's act-2 restart gate read the **wrong cell** (`foul_kind`/`[0x15888D]`,
    `fifa96_match_run.c:6002`) — fixed by T2 to `contact_kind`
    (`fifa96_match_run.c:6009/6016`). The **live raise** of `contact_kind==3` is
    **leg FU289-L1** (the offside chain is callerless/settings-gated, FU276-L2/L3);
    the swap is latent-correctness, not a new live producer.
  - **FU284** — row `0x1A` stays **NULL/uncounted** (**NO-GO**): the sit-5 emit
    is `FUN_0008A938(5, side, BX=1)` → **queue id-7 → FU-146**, never phase 9; the
    row's body is ported (`fifa96_match_action_1A`) but its only installer is the
    unported ladder (`FU284-L4`), so the slot stays NULL (the rows-27/29/2C
    precedent). Legs `FU284-L1..L6`.
  - **FU287** — the **live phase-6 producer** does not close: the phase-6 arm
    (`FUN_0008D098 0x8D4A2`, reached from `FUN_000740A0`) installs row `0x1F`
    **only for the non-controlled team's record 0** (`0x8D558`; the controlled
    team takes the penalty-spot reset code `0x13`), so the producer stays **leg
    T2b-L2** — the FU-150 chain is structurally live but unproven end-to-end.
  - **FU281 §2 erratum** — the pre-pass `FUN_0008D8EC` **is ALREADY modelled** by
    `fifa96_match_entities_team_update` (since commit `9b7abbe8`, 2026-10-07):
    the counter `+0x82C`, the sector-template select, the nearest `+0x7B2`/`+0x7BA`,
    the flag `+0x7BE`, the countdown `+0x7CB` and the `0xB2`-stride walk whose
    single `action(ctx,e)` **is** the native `CALL dword[rec+0x18]` are all
    present, so FU281 §2's "no engine equivalent of `FUN_0008D8EC`" and "the only
    modelled fragment is `fifa96_match_entities.c:698`" are **wrong**, and a
    standalone pre-pass would **double-dispatch**. T6 wired **no** second pass.
    The `0x74442` consumer of the written cells is **leg FU281-L5** (ungated).
    *(FU281-L2/L3 resolved as ported: `fifa96_entity_find_nearest` =
    `FUN_0008DE8C`; `fifa96_entity_distance` = `FUN_0008DC68`.)*
  - **FU283** — `0x114E04` is the **257-entry sine table** (`0 … 0x10000`),
    running `0, 402, 804, …` and ending at `0x10000` with *decreasing*
    increments — the same table `fifa96_ball_pairing.c:217` reads and
    `fifa96_entity_sine` consumes (moved down as `entity_sine_table[257]`,
    `fifa96_entity_update.c:27`); **FU273's "`0x114E04` sine" label was
    correct**. What FU283 genuinely established is that it is a **data table**
    (91 DATA xrefs, no calls) — a stage-select input, not code. *(Erratum: the
    close-out's "linear ramp table (0, 402, 804, …)" claim was false — it read
    only the first, near-linear entries; a pure step-402 ramp would end at
    `402×256 = 0x19200`, not `0x10000`.)* The ladder installs five
    `FUN_0007D9A4(rec, 0x1a/0x1b/0x1c/4, 0, 1)` arms (all `ECX=1` invoke-now);
    the **live stage index** stays unraised — legs **FU283-L1/L6** (the
    stage-select inputs `[0x14C1D4]` and the stage tables
    `0x10E184`/`0x10E1C4`/`0x10E1CE`); the table itself stays an **unported
    leg**.
  - **FU288** — the `[0x157A83]` actor ordering is **resolved** (GO, leg T2b-L1):
    the ladder install is gated `phase==2` (`0x7857B`) and the row-`0x1B`
    (`0x76DBF`) / row-`0x1A` (`0x7665C`) head actor-write is gated
    `phase==0xD && side==controlled` — **disjoint phase values**, so the install
    and the actor transition never share a frame, and the head-write→emit pair is
    the **same** row-`0x1B` dispatch. The engine-line citations **drift** with
    every `fifa96_match_handlers.c` edit (the plan's `:5512` action table →
    FU288's `:5517` row-`0x1A` → post-T5 the table begins at `:5575`, row `0x1A`
    at `:5633`, row `0x1B` at `:5635`): **treat the row names, not the line
    numbers, as authoritative.**
  - **FU285/FU288 mutual exclusion** — `0x76FD5 CMP EBP,[0x157A83]; JZ 0x77636`:
    the head write `0x76DBF` (gated `phase==0xD && side`) and the wide self-actor
    write `0x77568` (**ungated**) are **mutually exclusive per dispatch** — a
    head-pass jumps to `0x77636` and skips the wide block; a head-fail runs the
    wide block and `0x77568` self-sets the actor, so the stage-4 sit-5 emit
    `0x77707` is reachable **via the wide path** (T5 ports it). Legs **FU285-L1**
    (the deep `0x7707D..0x77420` pan/descriptor interpolation,
    `mr->match1b_wide_leg`), **L4/L5/L6/L7**, and **L8** (the re-place
    fall-through tail `0x7759c..0x77631`, leg-class; depends on the unported L1
    descriptor state).
  - **FU282 §1.3-vs-§1.4 clarification** — the record-0 install is **DEFERRED**
    (`FUN_0007D9A4(rec, code, EBX=0, ECX=0)`, `0x78553`/`0x7854C`/`0x78576`); the
    `(rec, code, 0, 1)` invoke-now text belongs to the **§1.4 ladder arms**
    (`ECX=1`). The `0x110680` permit gate (`0x784DC MOV AL,[EAX+0x110680]; AND
    AL,1; JZ 0x7859E`) **ALSO gates the ladder gate** (`0x784E9 JZ 0x7859E`) — a
    clarification of §1.4's three-term gate, not a contradiction. Legs
    `FU282-L1..L7`.
  - **FU163 §5 errata (carried)** — row `0x1B` **is wired**
    (`{0x1B, fifa96_match_action_1B}`, `fifa96_match_handlers.c:5635`); **only**
    row `0x1A` is NULL (`fifa96_match_handlers.c:5633`). The phase-9 arm cite is
    `fifa96_match_run.c:2853-2861` (the `:2172-2180` cite is **stale**).
  **The R11-L1/L2/L3 bridge is now ported** (T3/T4); the ladder is
  wired-but-unraised **LIVE-LEG** (no live stage index can be raised); row `0x1A`
  is NULL. **Layer-2:** an **honest waiver** (spec §6) — the granted-frame reach
  test is GREEN (Layer 1), but the visible camera is **NOT** made live (the
  installer + stage machine remain unported; there is no live installer).
  **Honest ceiling:** no row wired (census unmoved), no faked producer; every
  unreachable surface carries a numbered leg with a named native owner.
- **M2 phase-27 leg register (FU290…FU297; 2026-10-15) — the keeper-CPU
  installer (the camera unlock): the quoted errata + the leg register (appended;
  prior sections never rewritten). Census holds `59/80 = 73%` (`36 action + 23
  phase`); M1 `09b726b7…` and M2 `b281a9b2…` byte-identical (no re-pin).** The
  wave (`port/spec/FU290…FU297`) froze the installer frontier past R11-L4; the
  Stage-2 plan wired the ported machines into a **live frame** (the FU296 Seam A)
  and **ported the `CX` stage-select machine live** (T5: a non-zero stage from
  real engine state), but **no task proved a live end-to-end install**, so the
  census is unmoved and the phase closes as a **Layer-1 reach + a Layer-2 waiver**
  (spec §6; the phase-26a/26c shape). **Landed:** T2 the FU291 gate value; T3 the
  FU296 **Seam A**; T4 the staged Layer-1 reach (GREEN, with three mutation-RED
  riders); T5 (FU290/FU292) the ported `CX` machine + the readers of the FU292
  cells; T6 (FU294/FU295) the actor lift; T7 the honest Layer-2 waiver. **Errata
  (quoted):**
  - **FU290 — the `CX` stage machine is `CX ∈ {0,1,4,5,A}`. FU283 §3 erratum.**
    The only **immediates** are `{0,1,4,5}` (`b9 01000000`@`0x7805B`,
    `b9 04000000`@`0x780D2`/`0x781BA`, `b9 05000000`@`0x781CD`, `31c9`@`0x781AF`);
    the values `2`/`3` arise **only** via `MOV ECX,EDX`@`0x781DC` from the table
    byte `A = byte[0x10E184+idx] ∈ {1..5}`. **FU283 §3's "`ECX ∈ {0,1,2,4,5}`"
    conflates the immediate path with the `A` path.** The `CX` machine is **PORTED
    and live-wired (permit-gated)** (`fifa96_keeper_cpu_stage_select`,
    `0x77F57..0x781E1` exactly; it is reachable **only** through `ladder_run`
    under `if (rec->permit != 0)` (`fifa96_keeper_cpu.c:50,65-78`), so it does
    **not** run on a live frame — no live installer); the
    FU296 Seam A feeds it from real engine state and consumes its install via
    `kc_out.install`/`kc_out.ladder_out.install` — **not** a `kc_rec.stage` field
    (the T3 `rec.stage = e->stage` bridge is retired; the machine computes `CX`
    internally — see the FU290/FU296 errata). Erratum: `[ESP+0xA]>>16` is the
    `0x795B4` **bearing**, not camZ (the `>= 0xF0`/`>= 0x80` thresholds are bearing
    thresholds). The `0x10E1xx` stage tables are PORTED as data (their geometry
    **meaning** stays undecoded, FU290-L1); `0x114E04` is the 257-entry sine table.
  - **FU291 — the gate value is PORTED.** `prepass_ah2 = (input_range[(side ^
    side_swap) & 1] >> 9) & 1` (`fifa96_keeper_cpu.c:234-236`, the `0x77EE8 AND
    AH,2` fold); the `[0x14C1D4]` table was already `fifa96_input_range_words` →
    `mr->input_range`. **Zero-δ** (the gate's only consumer — the keeper-CPU ladder
    — is not wired live). Legs FU291-L1 (`[0x157ABE]`'s producer `FUN_0007417c`),
    L3 (the stats counters `FUN_00042c88`), L4/L5 stay open.
  - **FU294 — the pan/camera setter is ALREADY PORTED. FU285 §2 erratum.**
    `fifa96_camera_event_set` (`FUN_00071C94` + `FUN_00070544` subset, FU-148
    §12.1) and `fifa96_camera_set_tracked` → `camera.tracked` are ported and wired
    (`handlers.c:549/552`, `:1374/1379`, `:2717/2722`, `:3871`). FU285 §2's blanket
    "None of these are in the engine" is **superseded** for the camera triple /
    `camera.tracked` / `event_step` / `event_param`. The `0x71C94` **vector
    construction** at the row-`0x1B` `0x77423` site is still the `match1b_wide_leg`
    stub (`handlers.c:3870`). Legs FU294-L1 (`[0x1577CE+side*4]`), L2
    (`[0x157815]` dir byte), L3 (the `0x15780C..0x157822` smoothing family), L4
    (`[0x1577B8]` shared), L5 (the side-record clears).
  - **FU295 — the `0x74442` consumer cell-split + `0x76AD8`.** The driver
    `0x741C4` (§1.2) reads **only** `[+0x7B2]` at the `0x74442` consumer; the
    `[+0x7BA]` pick #2 and the `[+0x7BE]` visibility flag belong to the AI/lunge
    controller `FUN_0007F7E0` (and the unnamed `0x7E1B8`), **not** `0x74442` — the
    brief's "reads 7B2/7BA/7BE" **conflates three cells**. The controlled-side
    actor lift `FUN_0008C33C` (`0x8C3BE`) is ported (T6,
    `fifa96_match_entities_actor_lift`) — an **unraised port** (no `src/` caller).
    The actor `[0x157A83]` producer census = 22 WRITE (17 set→rec / 5 clear→0); the
    keeper route's are `0x76DBF` (row-`0x1B` head, **LIVE**), `0x77568` (row-`0x1B`
    wide self-set, **LIVE**), `0x7665C` (row-`0x1A` head, **NOT live** — NULL
    slot), `0x76AD8` (row-`0x1A` **body** set, **NOT live** — the FU186-L1 body
    remainder) and the `0x8C3BE` lift (**NOT live**). Legs FU295-L1..L6.
  - **FU296 — Seam A; P13 the blocker.** The `fifa96_keeper_cpu_step` call lives in
    the `i == 0` branch of `fifa96_match_entities_team_update`, before the single
    `action()` (the FU-141 walk; **no second pre-pass**, no double-dispatch —
    pinned `2*RECS`). **P13 / FU296-L5** (the stage index) was the phase's blocker;
    FU290's machine now **ADVANCES** it (a non-zero `CX` from live cells). The
    remaining end-to-end blocker is **P1 `[0x110680]`** (the install-permission
    table, FU296-L2/FU282-L5) — a *different* unported producer, zero live — **AND
    the dead interlock arming path** (the final-review co-blocker, below), plus
    the **`CX=1`→row-`0x1A` NULL lane** (FU296-L7). Legs FU296-L1 (link/reconcile —
    done), L2 (P1), L3 (the P4 interlock bind), L4 (the test seam), L5 (P13), L6
    (P6), L7 (the `0x1A` NULL slot), L8 (the invoke-now reconciliation).
  - **FU297 — row `0x1A` stays NULL; the `60/80` dead-count warning; the +116
    drift.** The ladder **does** install row `0x1A` (cases 1/2/3) and its body
    **does** lift the actor (`0x76AD8`), but the body is **<½ ported** (head +
    stage-0 only; no stage switch `0x76618`, no direction machine, no event, no
    emit) and the sit-5 emit is the **FU-146 queue id-7** route, **never** the
    engine's live direct-phase route. **Verdict: FU284's NO-GO STANDS** — the slot
    stays `{0x1A, NULL}`. **A slot flip alone** would quote `60/80 = 75%`
    (`action_wired 36→37`) but is a **dead count** (a wired half-ported body with
    no faithful route — the FU289 live-vs-leg / rows-27/29/2C shape; the census
    counts `fn != NULL`, not routes, FU297-L5). **Line drift:** FU284 cites the
    row-`0x1A` body at `:5517`; FU297 fresh at `:5633` — a consistent **+116**
    drift (the file grew); trust the row names, not the line numbers. Legs
    FU297-L1..L5.
  - **FU292 — the latch + geometry cells (the ladder's stage-select inputs).** Two
    cell families: the target/band block `0x157C44..0x157C50` (`0x157C44` scalar,
    the `0x157C48` 3-dword vector, `0x157C4C` its Y, the `0x157C2C` band code, the
    `0x157C5C` ladder latch) and the camera-band block `0x1577F0..0x1577FA`
    (`0x1577F4`/`F8` extents, `0x1577BC`, the packed `0x157821`, the `0x158740`
    predicate gate). **Ported (T5) — the readers:** `0x157A49>>24` = `[0x157A4C]`
    LIVE, `0x1577BC>>16` = `follow_speed` LIVE, `0x157821` byte0 = `pan_counter`
    LIVE; the `0x157C44`/`0x157C48` writer **is recovered**
    (`fifa96_keeper_hold_predict`, FU-179 §4 → `mr->predict_c44`/`predict_c48_*`,
    bound to `rec->latch_157c44`/`target_157c48` — **FU292-L1 corrected**, read
    zero live only because the projection sub-leg FU179-L2/L3 is unported); the
    ladder now **WRITES** `[0x157C5C]` (**FU292-L6 advanced**, published in-frame
    via `latch_written`). **Their WRITERS stay unported:** **FU292-L1** (the
    projection sub-leg), **FU292-L2** (the `0x1577F0..FA` band struct words),
    **FU292-L3** (`0x157821` byte3); plus FU292-L4 (`0x158740` mode byte), FU292-L5
    (`0x157C2C` alphabet).
  - **FU293 — the row-`0x1B` deep pan/descriptor interpolation stays a leg.** The
    `0x7707D..0x77420` interpolation (`FU285-L1`, `mr->match1b_wide_leg`) is
    unported: **FU293-L1** (the pan cells `[0x1577BE]`/`[0x1577C0]`/`[0x1577C2]` —
    the load-bearing L1 state), L2 (the `[0x14C2F6]` difficulty + the ratio
    descriptors), L3 (the `[0x1577F2]`/`[0x1577F8]` pan-vector pair), L4 (the
    `0x8DCD4`/`0xCD474`/`0xCD514`/`0x8DC68`/`0x74584`/`0x702F8` folds), L5 (the
    `EDI` sign-match + the install selection), L6 (the sit dispatch), L7
    (`[0x1577CA]` stand-in), L8 (`FUN_00076B28` steering), L9 (row `0x1B` is not a
    Ghidra function). The interpolation legs are the load-bearing remainder of the
    wide path.
  - **The interlock co-blocker (final whole-branch review, T8) — P1 alone does NOT
    arm the ladder.** The register above blames **P1 `[0x110680]` alone**; that is
    **incomplete**. The seam unconditionally writes the machine view back over the
    run cells **after** the entity walk
    (`port/src/fifa96_engine/fifa96_match_run.c:4257`
    `mr->global_157c5d = keeper_cpu.interlock_157c5d`), and the step clears that
    view every frame (`port/src/fifa96_loader/fifa96_keeper_cpu.c:80`, the native
    `0x785A2` clear). The only **live** writer of `[0x157C5D]=1` is the row-`0x19`
    handler (`port/src/fifa96_engine/fifa96_match_handlers.c:4097`), which runs
    **inside** that same walk — so its write is clobbered before the next frame's
    bind (`port/src/fifa96_engine/fifa96_match_run.c:4249`) can read it. Net: the
    ladder gate `interlock != 0` (`fifa96_keeper_cpu.c:65`) is **always false**,
    so **porting P1 `[0x110680]` ALONE will still not arm the ladder.** The
    remaining end-to-end blocker is **P1 `[0x110680]` AND the dead interlock
    arming path** — P1 must be paired with fixing that read→step→write ordering
    (or publishing the interlock in-frame, exactly as T5 did for `[0x157C5C]` via
    `latch_written`), or the camera unlock still won't fire. **Inert today
    (`permit == 0`) and M2-invisible.** Cross-refs: **FU296-L3** ("`[0x157C5D]`
    … is **never read** by any engine code … set-and-forgotten",
    `port/spec/FU296_r11l4_installer_seam.md:208-210`) and **FU257-L6** (the
    `[0x157C5D]` consumer wiring, `port/ENGINE.md` FU257 leg register
    `:1552` pre-edit).
  **The open legs the phase leaves, named:** **P1 `[0x110680]`** (the
  install-permission, FU296-L2/FU282-L5) — the decisive end-to-end blocker — **AND
  the dead interlock arming path** (the co-blocker above; **FU296-L3** / **FU257-L6**:
  the seam clobbers the row-`0x19` `[0x157C5D]=1` before the next bind reads it,
  so P1 alone does not arm the ladder);
  **the `CX=1`→NULL row-`0x1A` lane** (FU296-L7/FU297); **FU290-L1** (the
  `0x10E1xx` stage-table semantics — ported as data, meaning undecoded),
  **FU290-L2** (the `0x795B4` bearing chain — caller-supplied), **FU290-L5** (the
  `[[rec+4]+0xB]`/`+0x10` sub-fields), **FU290-L7** (`[0x158740]` inside
  `FUN_000765C4`); **FU292-L1/L2/L3** (the writers); **FU295-L1** (the `0x741C4`
  driver's caller/registration); **FU196-L6** (the `FUN_0007417C` restart sink —
  the lift's caller; wiring it un-gates `team_select_intercept` and moves M2);
  **FU281-L5** (the un-gated `0x74442` consumer); and **FU290-L10 (new) — the
  `0x77F20` `+0x81` `pos→target` copy** (`out->copy_pos`,
  `port/src/fifa96_loader/fifa96_keeper_cpu.c:241`) is **reported-but-unapplied** (the
  seam consumes `install`/`copy_valid`, not `copy_pos`) — the same class as the
  fixed `copy_src`; native owner `FUN_00077EAC` (`0x77F16..0x77F20`, FU186-L5/L6);
  zero live. **Honest ceiling:** no row wired (census unmoved), no faked producer;
  every unreachable surface carries a numbered leg with a named native owner.
- **M2 phase-28 leg register (FU298…FU305; 2026-10-21) — the installer live
  (the camera unlock's mechanism) + the Layer-2 acceptance: the quoted errata +
  the leg register (appended; prior sections never rewritten). Census
  `60/80 = 75%`
  (`37 action + 23 phase`); M1 `09b726b7…` and M2 `b281a9b2…` byte-identical
  (no re-pin).** The Stage-2 plan (T1–T9) drove the keeper-CPU ladder from
  wired-but-unraised to a proven live install+dispatch (**Layer 1, GREEN**)
  and attempted the visible-camera acceptance (**Layer 2**): the windowed smoke
  runs, but the live camera cells stay `(0,0,0)` — the visible-camera goal is
  an **honest §6 waiver** with the remainder legs named (the FU305 item below).
  **Landed:** T1 the FU298 permit; T2 FU299 Option B; T3 FU304; T4 FU300; T5
  the Layer-1 reach + the census; T6/T7 FU301/FU302; T8 FU303; T9 the
  acceptance + this register.
  - **FU298 — P1 `[0x110680]` is a static 45-byte table; the read is bounded
    ≤`0x2C`; the phase-27 waiver is re-armed.** Fresh `read_memory 0x110680`
    (96 B) + `search_instructions`/`get_xrefs_to` = 19 sites, **all reads, no
    writer** — so "porting P1" is a byte-exact data table + a live bind
    (`fifa96_keeper_cpu_permit_tbl[0x2D]`,
    `fifa96_keeper_cpu_permit(code)` with `code < 0x2D`), not a producer. The
    FU305 erratum stands: the extra "permit=1" codes
    `{0x2F,0x44,0x47,0x49,0x4A,0x4B,0x5D,0x5E}` are the mode-table bytes at
    `0x1106AD`/`0x1106C3`, not permit entries. The FU298 §5 "`static const`"
    sketch vs the exported table is an **erratum** (the port exposes the table
    for the byte-exactness pin). **The re-armed waiver:** the phase-27
    `test_keeper_installer_live_waiver` (the dead `permit == 0`) is re-aimed as
    `test_keeper_installer_live_permit` — RED on the phase-27 tree, GREEN after
    the bind (FU298-L6), exactly the red-by-design flip the FU298 doc named.
    Legs (native owners in FU298 §7): **FU298-L1** the exhaustive `+0x91`
    state domain (owner: the `FUN_0007D9A4` call-site census; no native bound
    check at the read sites, the port's bound is the faithful 45-byte physical
    bound), **L2** the loader-stub pre-entry patch (owner: the 16-bit loader
    spine; neither excluded nor evidenced), **L3** the `&3` bit-1 semantics
    (owner: the `&3` sites' consumers, `0x7CFE8`/`0x7D06C`), **L4** the mode
    tables `0x1106AD`/`0x1106C3` (owner `FUN_0008AF38`/`FUN_00079F3C`), **L5**
    the record-0 code trajectory on M2 (owner: the M2 golden harness).
  - **FU299 — the interlock ordering is Option B: per-team bind → step →
    publish.** The native walk reads `[0x157C5D]` at the ladder gate
    (`0x7858E`), the step clears it (`0x785A2`), and row-`0x19`'s body sets it
    (`0x74707`); the engine now binds the cell into the machine view immediately
    before `fifa96_keeper_cpu_step` and publishes the view back immediately
    after it, inside `team_update` and **before** the single dispatch — the
    frame-head bind and frame-tail write-back
    (`fifa96_match_run.c:4249`/`:4257`) are deleted, and the `[0x157AB2]`
    publish moved into the per-step publish as the same block (FU304-L2 /
    FU299-L5). Net: a row-`0x19` dispatch arms the *next* step (a one-step
    pulse), and the same-frame cross-team arm is **derived, not captured** —
    **FU299-L1** (native owner `FUN_0004B100 0x4B2AB/0x4B2B2` +
    `FUN_0008D8EC 0x8DB35` + the shared cell); the engine rider (b) tests the
    engine side only. **FU299-L2** (an outfield record holding code `0x19` with
    no in-walk clear; owner `FUN_0007CA54`, carried FU-179 row-04 producer
    `0x7E866`/`0x7E8C1`) stays unproven; **L3** the cell's port shape is now
    settled (the entities-frame cell + the machine view); **L4** the
    invoke-now reconciliation is carried (FU296-L8); **L5** is closed by the
    block migration.
  - **FU300 — row `0x1A` is ported in full and wired (counted).** The 5-arm
    stage machine `{0x76693,0x766DD,0x766F5,0x76AAD,0x76AE8}`, the
    `[0x157C2C]` direction machine (`0x2D`/`0x2E`/`0x54` + the `0x702F8`
    clamp), the movement block (`0x79C50` → `fifa96_entity_face`; `0xCD474` →
    `word[+0x7F]`; `0x6E598` → the store core), the event ladder `{5,4,9,1,7,0}`
    + anim select `{0xF,0xE,1}`, the sit-7 emit and the stage-4 sit-5 emit; the
    slot `{0x1A, fifa96_match_action_1A}` is live and the census is a proven
    count, not a slot flip (the FU297 "dead count" warning is satisfied).
    **The `'T'` metric survives the dispatch decomposition** — the native
    `word[+0x71] = 0x8DC68(new C0, new C2)` is mirrored into `vel_x`'s low
    word via the dispatcher's exact decomposition (the FU-141 rebuild otherwise
    overwrote `r->speed71`). **The FU84 leg 5 / FU-159 side state:** the
    `0x6E598` helper's `[0x157A6C]` write is the pre-existing animation
    selector's side effect, carried, not row-`0x1A`'s and not modelled by the
    store core (FU300 E8). **The E9 `[0x157750]` binding:** the body reads Y
    through `mr->keeper_cam_y` (the FU-151 process-cell mapping shared with
    rows `0x1B`/`0x1D`/`0x1F`), not the render camera; widths stay native —
    `0x7677E`/`0x7690C` are dword compares, the `0x767B6` clamp is the word
    read. **The `match1a_anim` last-call semantics:** the cell records the
    **last** `0x651F0` argument — 6 from the direction arms (observable on
    both `0x54` exits) then `0xF`/`0xE`/1 from the event select. **Erratum:**
    FU300 E4's quoted `disassemble_bytes 0x7687b` span is **61 B**, not the
    "(40 B)" in the frozen quote. Legs: **FU300-L1** (`0x1577F0`/`0x1577FA`
    and the descriptor byte `[[rec+4]+0x10]+3`; owners FU292-L2/FU294-L1/L2/
    FU301), **L2** the remaining movement cells (`0x79C50`/`0xCD474`/`0x6E598`/
    `0x702F8` are ported; the residual is FU186-L2/FU300-L2), **L3** the sink
    bodies (`0x8ED40`/`0x8F188`/`0x651F0`/`0x974DC`/`0x974F0`), **L4** closed
    (the seam stamp), **L5** the row-`0x1B` emit shape (owner `0x776F7`/
    `0x76B1C`).
  - **FU301 — the deep pan I: the seed + Blocks A–E (`0x77035..0x77312`).**
    The three corrections vs FU293 are landed: the seed `ECX = 0x60-(8-d)`,
    Block A's **unconditional** `ECX -= ECX>>2`, and the code-3 reduce touching
    **only** `[0x1577C2]`. The split is recorded: the plan title says A–F, but
    Block F's `EDI` sign-match + pan vector (`0x77313..0x77420`) landed in
    **FU302** (T7), so the phase-28 T6 commit ends at `0x77312` — the "Blocks
    A–E (`0x77035..0x7733E`)" end address is an **erratum**. The `0x92AC8`
    stream positions are pinned (FU301-L9): two draws on the `BE==0` seed, one
    jitter draw only inside the Block C condition, one reduce draw always.
    Legs: **FU301-L2** (`[0x14C2F6]` unmodeled; owner `FUN_0003749C 0x374FB` /
    `FUN_00038630 0x3878C` + the `0x149278` table), **L3** the ratio
    descriptors (`[desc+0xC]` bits, `+0xF`, `+0x1B`; owner the in-window read
    sites `0x77044/0x7704F/0x770B6/0x770BE/0x771F9/0x771FF`), **L4** the band
    family's wide-block alias delta (owner `0x7733F..0x77350`), **L8** the
    `[0x157C59]`/`[0x157C57]` latches (now written; owner `0x7721B`/`0x7738C`),
    **L9** the RNG contract (owner the `0x92AC8` sites
    `0x7719F/0x77233/0x77247/0x772C5`).
  - **FU302 — the deep pan II: Blocks F/G + the exact `0xCD514`; the seat
    adjudicated.** The folds `0x8DCD4`/`0xCD474`/`0x8DC68`/`0x74584` and the
    exact trig-normalized `0xCD514` (`fifa96_entity_hypot`: `hypot(a,b)` via
    the better-conditioned branch, full 32-bit quotient) are ported; Block E
    and the `0x795B4` band use the exact primitive (the row-`0x1B`/`0x1C`
    heading-order correction is pinned). **The seat camY adjudication:** the
    `0x77420` `BX` is `SI = dword[0x157750]` loaded at `0x77228` — the
    camY-derived height, **not** the jitter; **FU302-L10 is superseded**
    (the wave review adjudicated FU301-L6 correct), and the install-tree
    product is the post-seat pan-z cell (`09a24c3`). The `0x795B4` band's own
    sibling `fifa96_entity_intercept_band` is **not** an exact `0xCD514`
    port (transposed angle, fixed `|dx|` dividend) — **FU302-L1 stays a
    residual leg** with its six callers named (row-`0x0B` ×2, row-`0x0C`,
    row-`0x16`, the team lane refresh, `team_select_intercept`); the native
    owner is `0xCD514`/`0x795B4`. **The rotated steering arm remains
    FU186-L2:** the on-target `0x76B28` call is ported with the native
    `0x80/0x40` constants; row-`0x1B`'s `0x76FA2` caller still sits behind
    the unported stage-1 block. **FU302-L8 stays open** (the install gate
    `byte[0x157A4D] == 0x10` vs the head's `== 0xD` and the FU285-L2
    `0x157A4C` reconciliation). **FU302-L7 stays partially attached:** the
    tracked-id → pool-record resolution gives Block A/C live keeper positions
    (the row-`0x1A` precedent), but the `[0x1577CA]` producer/stand-in question
    and any tracked id with no pool record remain open; an unresolvable id
    takes the `d = 0` near arm (owner `0x7707D`/`0x771C1`) — no closure is
    claimed. The sink bodies (`0x8F188`/`0x8ED40`/
    `0x974DC`, FU302-L5 owner `0x92820`, OL-27/OL-82) stay legs with their
    computed arguments recorded in the `match1b_*` observables.
  - **FU303 — the visible-motion cells: the needed/waivable split.** Needed
    and landed: the dir byte `[0x157815]` producer
    (`fifa96_camera_event_dir` at the setter sites), the `FUN_00071DF4` rate
    table (`0x10E169` compass + the `0x11042B` 64-pair table) →
    `camera.rate_x/rate_z` → the `update_walk` pan-push (`rate*(timer_limit −
    timer)` into target/anchor, `rate` into velocity), and F6
    (`band_1577f6 = timer_limit − ramp_divisor`, the `0x705e8` identity) +
    the honest byte-3 `0xFF` staging (`FUN_000703E8`'s entry init; the
    classifier's input mapping is the carried **FU303-L11**, so no producer
    is faked). Waivable, staying legs: **FU294-L1** (the per-side tracked
    array — goal-scan only; owner the ported `fifa96_camera_event_set`),
    the **FU294-L3(a)** smoothing loop (proven closed, `[0x157814]` has 0
    readers; owner `fifa96_camera_update_walk`), the `[0x157820]` freeze rider
    and the `[0x157822]`→composer-bit rider (**FU303-L4**; owners
    `fifa96_camera_update_walk` and the composer staging), `[0x158777]`
    (**FU303-L5**; owner the FU-152 §2.9 setter clear), and **FU294-L2**'s
    keeper ring (`FUN_00092998(1,2,-1)`, OL-27). Two further T8 leg families
    are classified here as **waivable for the first visible install, needed
    for the A/5 arms / dive-window fidelity**: **FU292-L1 / FU303-L6** — the
    row-`0x19` predict call site stages its three leads as literal `0,0,0`, so
    `fifa96_keeper_hold_predict` takes the FU-179 §4 step-3 bail and the latch
    cells (`0x157C44`/`0x157C48` → `latch_c44`/`target_c48`) read 0 live;
    CX=1/CX=4 do not read the latch, the A/5 stage-table arms and the row-`0x19`
    hold target do (owner: the row-`0x19` call site / FU-179); **FU290-L5 /
    FU303-L9** — the `[[rec+4]+0xB]`/`+0x10` (`sub_0b`/`sub_10`) descriptor
    bytes are staged 0, monotone conservative (the CX=4 lane window shrinks
    from `2b+0x30` to `0x30`; the A early-out widens to `0x13`), no wrong
    motion created (owner: FU180-L1/L3). The rate arm's sub-object dir
    pair is the entity-pool copy (the pre-existing first arm's mapping; a
    one-drain lag for a slot-bound record — the established semantics, not
    changed). **FU303-L11**'s out-of-domain `0x10E169` read is bounded to the
    live `{-1,0,1}` slot domain (the native's unchecked read is unreachable
    from that writer; owner `fifa96_camera` / FU-148 §12.1).
  - **FU304 — the seam contract merged; `copy_pos` applies; FU304-L7.** Both
    Seam A installs route through `fifa96_match_entities_install` (the full
    native `FUN_0007D9A4` contract: `stage92`/`timer89`/`ran`/`+0x7B`, not a
    bare `e->code = install_code`), so row-`0x1A`'s first dispatch cannot
    mis-route on stale stage state; `copy_pos` applies `target_* = pos_*`
    (`0x77F20`) and the `[0x157C2C]` stamp publishes in-frame (G2). The
    install-before-ladder order is the native order (`0x78576` install then
    `0x7857B..0x78599` ladder), so the ladder derives from the post-install
    code. **FU304-L7:** the `+0x81` copy arm is statically unreachable through
    the ladder's only caller (`timer81 != 0` forces the install of code 0
    before the `0x77EC2` head gate), so the seam apply is a branch contract
    (`test_keeper_seam_copy_pos_applies`, a stub ladder), not a live claim —
    no producer is faked to exercise it. **FU304-L4** (the seam's
    `input_range` cells leave the bit-9 gate honestly open, zeros open it;
    owner the seam promotion with FU291-L2) stays a leg; **L2/L3** are closed
    by the per-step publish and the in-frame stamp; **FU304-L5** (the interlock
    arming order) is **resolved** by the FU299 Option-B per-step publish
    (above), and **FU304-L6** (the invoke-now reconciliation) is carried via
    **FU299-L4**.
  - **FU305 — the Layer-2 acceptance + the tape prediction; the honest
    verdict.** The tape prediction (§5.3) is **measured**: enable the permit
    live but the install tree is same-code-suppressed on the M2 route (record
    0 already holds the code the tree would write), so **M2 never moved** —
    `09b726b7…`/`b281a9b2…` byte-identical through T1–T9, no re-pin, no
    stop; the post-m-42 ladder-arm question (the §5.3 prediction) produced no
    presented change, and the exact tape-side refusal (the AX/CX arm census
    over the parked-camera windows) stays **FU305-L1/L2** — not re-derived
    here. The
    acceptance ran on this host (`make game`, `DISPLAY=:1`, SDL3, the ISO
    present; the phase-8 walkthrough: intro RETURN → front-end BACKSPACE →
    RETURN, then the post-cutscene KICK release at ~20 s — the phase-18 act-1
    route's phase-2 landing — clock `00:05`): the **pan pair**
    (`docs/screens/p28-l2-pan-a/b.png`, same clock second) has scene-band AE
    **1582** (`y < 591` crop of the 960×720 client area); the **still pair**
    (`p28-l2-still-a/b.png`, no key held, same clock second `00:07`) has
    **2052**, not 0. The pan AE (1582) also sits ~20× below the FU305
    precedent (~2.9e4–4.6e4) and *below* the still control (2052),
    independently corroborating that no camera pan ran. The scene animates
    continuously on the live path (the phase-10+ entity roll), so the AE
    witness cannot attribute motion to the camera — and a read-only `gdb`
    sample of the live run (no code change; 517
    phase-2 samples over ~52 s of repeated direction holds and kicks) shows the
    camera cells static for the whole phase-2 window:
    `pos=(0,0,0)`, `vel=(0,0)`, `rate=(0,0)`, `tracked=-1`, `follow_speed=0`,
    `event_param=0`, `pan_counter=0`, `timer=0` (the follow/pan chain's gates
    never open). **Verdict: Layer 1 GREEN, Layer 2 WAIVED** (spec §6) — the
    visible camera is **not** made live by this phase; the Layer-1 reach (the
    same-commit attribution: `camera.pos_y` `0x38`→`0x50` and
    `camera.tracked == 11` through the real seam, with the mutation-RED riders)
    is the landing, and the visible-camera remainder keeps its numbered owners:
    **FU305-L5** (the follow/pan needs a live event source: `FUN_00071DF4`'s
    first arm + FU293-L1/L3 and the every-frame auto-camera's `[0x1577EE].hi >
    0xF0` gate; the FU-148 §13 ball-staging producers), **FU305-L1** (the
    record-0 census; owner `FUN_000782D0 0x784C0..0x785A8`), **L2** (the AX/CX
    tape verdict; owner `FUN_00077EAC 0x77F9A/0x77FA5/0x78032..0x78051`),
    **L3** (`[0x157A4C]`; owner `0x77FAD..0x77FEE` / the FU-217 sit-8 arm),
    **L4** (the unported `AX==0 ∧ lane<0x40` arm; owner `FUN_0006DC88` +
    `FUN_00092AC8`), **L6** (the AE attribution limit; owner the staging
    `FUN_00036C70 0x36C9F` + the acceptance procedure), **L7** (the second
    `[0x157ABE]` writer `0x742B4`, the FU196-L6-adjacent class).
  - **Errata folding the phase's review minors (quoted; prior text not
    rewritten):** (1) the **phase-16 dispatch-results ledger**
    (`45 UNSUP / 34 OK / 1 NOTF`, `port/ENGINE.md` phase-16 T2) is
    **historical** — the current dispatch over the 80 rows is
    **`19 UNSUP / 60 OK / 1 NOTF`** (T5's `60/80`; the ledger's errata chain
    ends at phase-16 T4 and must not be read as this phase's result). (2) The
    phase-27 register's **FU285-L1 "the `0x7707D..0x77420` interpolation is
    unported"** statements (`port/ENGINE.md:1752` / `:1873` / `:1874`) are
    **superseded** by FU301+FU302: the interpolation is ported
    (`match_1b_deep_pan`), and only the sink bodies / FU302-L7 / FU302-L8 / the
    rotated steering arm remain legs.
    (3) The phase-27 register's **FU292-L2/L3 "writers stay unported"** is
    **advanced**: F6 is bound live (`band_1577f6`), byte-3 is honestly staged
    `0xFF` (producer FU303-L11), and the `[0x157815]` chain is live.
    (4) The phase-27 register's **FU294-L1** cell and the FU294-L3(a) loop are
    confirmed **waivable** (no render reader / proven closed). (5) The
    **FU300 E4** quote span is 61 B (above). (6) The phase-27 T4 reach's weak
    `camera.tracked != 0` is strengthened in the Layer-1 reach to the exact
    id **11** (`FIFA96_MATCH_ENTITY_RECORDS`), and
    `test_keeper_seam_copy_pos_applies` is labelled "staged (branch contract;
    not live reach)" inline. (7) **FU299-L1** (the cross-team same-frame arm)
    and **FU304-L7** stay open with their native owners — neither is claimed
    live.
  - **Fix round 1 (T9 review, 2026-10-21) — the two omissions + the
    clarifications (quoted; this register's text above amended in place, the
    prior phases untouched):** **(i) FU302-L7 added** to the FU302 item — the
    tracked-keeper deep reads are **partially attached**, not closed
    (`[0x1577CA]` producer/stand-in + the no-pool-record id → the `d = 0` near
    arm; owners `0x7707D`/`0x771C1`; the FU302-L8 sentence's "only … remain"
    now names L7). **(ii) The FU303 needed/waivable split now classifies the
    two remaining T8 families**: **FU292-L1/FU303-L6** (the row-`0x19` predict
    leads staged `0,0,0`; waivable for CX=1/CX=4, needed for the A/5 arms +
    the row-`0x19` hold target; owner the row-`0x19` call site / FU-179) and
    **FU290-L5/FU303-L9** (`sub_0b`/`sub_10` staged 0; waivable, monotone
    conservative, needed for the dive-window fidelity; owner FU180-L1/L3).
    **(iii) FU304-L5 disposition named** (resolved by the FU299 Option-B
    per-step publish; the separate `[0x157C5C]` write-back block is FU299-L5)
    and **FU304-L6** (invoke-now) carried via **FU299-L4**. **(iv)** The
    FU285-L1 errata cite now includes `:1873` (the FU293 header "stays a
    leg"). **(v)** The register title is "the camera unlock's mechanism" (the
    visible camera itself is waived below). **(vi)** The Layer-2 AE
    corroboration sentence added to the FU305 item (pan AE 1582 ≈ 20× below
    the precedent and below the still control → no camera pan). No code or
    test change; `make check` unaffected (docs-only), M1/M2 unmoved.
  - **Final fix wave (whole-branch review, 2026-10-21) — row-`0x1A` live
    cells + the one pan pair (quoted; prior text above not rewritten).**
    **(i) Row `0x1A` now reads the LIVE camera cells.** Fresh bytes
    `0x766fc`/`0x7671a`/`0x7672c`/`0x76757`/`0x76868` show `[0x1577EE]>>16` =
    word `0x1577F0` = `camera.event_param`, `[0x1577BC]>>16` = word `0x1577BE`
    = `camera.speed`, and `[0x1577F8]>>16` = word `0x1577FA` = `camera.timer`;
    the body had bound the zero-only run cells `band_1577f0`/`band_1577fa`
    and the derived pre-tail `camera.follow_speed` (not the `0x1577BE` cell),
    so the `0x1` event arm, the `0x2D`/`0x2E` timer gates and the anim `>0x78`
    arm were dead/misread. The two run-level band fields are retired; the
    entity frame's `timer_a` binds the same live `camera.timer`, and only the
    sibling bucket timers `0x157800`/`0x157806` (+ their FU-67 S3 producer)
    stay zero. **FU300-L1 is therefore narrowed**: its `0x1577F0`/`0x1577FA`
    part is closed (live camera cells), the `0x1577C0`/`0x1577C2` writer part
    closes with (ii); the descriptor byte `[[rec+4]+0x10]+3` (FU290-L5) and
    `[0x158740]>>24` (FU292-L4) remain the row's numbered legs with their
    existing owners. Pins: `test_action_1a_live_camera_cells_frame_path` (real
    frame path; a live `fifa96_camera_event_set` produces event_param `0x50`,
    speed `0x19`, timer 0→2, and the `0x2D` arm stamps `0x5C` → id `4`/anim
    `0xE`; the old mapping was RED: id `7`/anim `0xF`).
    **(ii) The `0x1577C0`/`0x1577C2` pan pair is ONE pair.** Row-`0x1B` Block
    E and row-`0x1A`'s `'T'` arm now both write `camera.vel_x`/`vel_z` (the
    native words); the duplicate run-level `screen_lead_x/z` cells are
    retired, and row-`0x1A`'s movement face plus the goal-screen selection
    (`0x9497D`/`0x949B7`) read the camera fields. Pin:
    `test_row1b_pan_feeds_row1a_and_screen` (a row-`0x1B` pan → the same run's
    row-`0x1A` face and the screen id-`9`/id-`7` selection; RED under the
    two-cell model). **FU302-L12 is not opened** — the unification is
    structural and live, so no leg register entry is owed.
    **(iii) Minor fix-wave items:** `match1a_event_id`/`match1a_anim` reset at
    row-`0x1A` dispatch entry (head-divert/install-4 no longer leaks prior
    values; `handlers.c:4180-4193` parity); the `match_1b_steering` inner
    `dx`/`dz` renamed `band_dx`/`band_dz` (no parameter shadow); the stale
    producer comments in `fifa96_match_entities.c` (`:750-753`),
    `fifa96_match_run.c` (`:1560-1565`), `fifa96_match_run.h` (`:713-716`) and
    the row-`0x1A` header binding block are corrected; FU300 E4's `(40 B)` is
    errata'd as 61 B in the FU300 doc (E10) and here; the PROGRESS "two
    fidelity riders" line is aligned to **FU292-L1/FU303-L6** +
    **FU290-L5/FU303-L9**.
    **(iv) Historical marker:** the **"Unwired rows (59/80)"** bullet
    (`port/ENGINE.md:4765`) is **historical** (phase-7 era). The current
    dispatch over the 80 rows is **`19 UNSUP / 60 OK / 1 NOTF`**
    (`60/80 = 75%`; the same correction as errata (1) above); it must not be
    read as the phase-28 census. `make check` 111/111 (ASan/UBSan), census
    unchanged `60/80 = 75%`, M1 `09b726b7…` / M2 `b281a9b2…` byte-identical
    (no re-pin).
- **M2 phase-29 leg register (FU306…FU309; 2026-10-21) — the visible follow
  camera's producers (the live Y + the event chain): the quoted errata + the
  leg register (appended; prior sections never rewritten). Census
  `60/80 = 75%` (`37 action + 23 phase`); M1 `09b726b7…` and M2 `b281a9b2…`
  byte-identical (no re-pin).** The Stage-2 plan (T1–T6) landed the live
  `follow_speed` seam (T1/FU308), the four live `[0x157750]` stand-ins
  (T2/FU308), the keeper-band camera-Y delivery + the single cell
  (T3/FU307), the chaser corrections (T4/FU306) and the Layer-1 follow chain
  (T5/FU309); T6 ran the Layer-2 acceptance (**WAIVED, spec §6**) and this
  register. **Landed:** T1 the seam; T2 the stand-ins; T3 FU307-L3/L4 (the
  raw stores + the single cell); T4 FU306-L2/L3/L4; T5 the event chain; T6
  the acceptance.
  - **The Layer-2 acceptance (T6) — an honest §6 waiver; no visible follow.**
    `make game` (`DISPLAY=:1`, SDL3, the ISO): the phase-8 walkthrough (intro
    RETURN → front-end BACKSPACE → RETURN → the kickoff KICK release; the
    pairs taken in phase 2 at clock `00:05`). The **pan pair**
    (`docs/screens/p29-l2-pan-a/b.png`, the same displayed second `00:05`,
    a direction held) has scene-band AE **3421** on the `960x591+0+0` crop;
    the **still pair** (`p29-l2-still-a/b.png`, the same second `00:05`, no
    key held) has **2988**. The relative gate `AE_pan > AE_still` passes by
    433 px (14 %), but the delta is confined to the sprite band, which
    cannot distinguish sprite motion from a pan: the
    `compare -compose src` diff bbox is `476x56+230+296` (the sprite band;
    the still pair is `434x53+251+299`), while a live follow's
    `vel = dirs × 15` would move the whole scene, so the gate is
    **corroboration only** and does not attest a pan (the p28 datum
    re-verified this session: pan 1582 vs still 2052). The primary
    attribution is the live cell trace: a read-only `gdb` sample of the same
    run (no code change; **10 samples at a ~30-step stride** through
    `fifa96_match_run_step`'s `mr` argument, while a direction was held)
    shows every camera cell static —
    `pos=(0,0,0)`, `vel=(0,0)`, `speed=0`, `event_param=0`,
    `pan_counter=0`, `timer=0/0`, `tracked=-1` — at every sample:

    | sample | clock / frame | `pos` | `tracked` | `event` |
    |---|---|---|---|---|
    | S0 | n.r. / ~0 step | (0,0,0) | -1 | 0 |
    | S1 | n.r. / ~30 steps | (0,0,0) | -1 | 0 |
    | S2 | n.r. / ~60 steps | (0,0,0) | -1 | 0 |
    | S3 | n.r. / ~90 steps | (0,0,0) | -1 | 0 |
    | S4 | n.r. / ~120 steps | (0,0,0) | -1 | 0 |
    | S5 | n.r. / ~150 steps | (0,0,0) | -1 | 0 |
    | S6 | n.r. / ~180 steps | (0,0,0) | -1 | 0 |
    | S7 | n.r. / ~210 steps | (0,0,0) | -1 | 0 |
    | S8 | n.r. / ~240 steps | (0,0,0) | -1 | 0 |
    | S9 | n.r. / ~270 steps | (0,0,0) | -1 | 0 |

    `n.r.` = not retained: the session log kept the sample index and the
    cell vector, not the absolute frame counter or the displayed clock (the
    stride above is the report's ~30 steps; the run is the phase-2
    acceptance run whose capture window is `00:05`, direction held). Every
    row also carried `vel=(0,0)`, `speed=0`, `event_param=0`,
    `pan_counter=0`, `timer=0/0`. This table carries the full retained
    per-sample record; the durable per-sample form is FU309-L5.
    `docs/screens/p29-l2-runaway.png` is the same run at `02:00` with the
    controlled record upper-middle (roughly y 270–350 of 720) and the camera
    unmoved. A second
    `make game` run with an open-play burst (direction changes +
    KICK/PASS/LONG_BALL presses) and six further samples likewise kept every
    camera cell at zero — no follow fired in that burst. **Verdict:
    Layer 1 GREEN, Layer 2 WAIVED** (spec §6): the camera does **not**
    visibly follow; the reason is the natural reach (FU307-L1 / FU309-L1) —
    the live chain never produces a `>0xF0` event for the follow arm (and no
    mid-band camera Y for the pan arm). The Layer-1 landing (T5's GREEN
    54-frame window + the seven mutation riders) is the phase's acceptance.
  - **FU306 — the record-event chaser (`FUN_00070C08`): the ramp fix + the
    one-shot re-arm.** T4 landed the three fidelity corrections: `ramp_param
    = 1` at the setter call (native `0x70DBA MOV ECX,1`, preserved by the
    `0x92820` sink's PUSH/POP ECX and consumed by `0x71D60 MOVSX EAX,CX` —
    FU306 erratum 1; the engine passed 0, the receive/row-04 arm's sign), the
    live `+0x5D` (the BF20 tail reads `r->pos_y` after the
    `0x7C057..0x7C113` jump arc, not the stale `e->pos_y` written after the
    call), and the int16 seed wrap of the `0x8DC50` correction. Pins:
    `test_record_event_chaser_ramp_param_one` (timer 36/limit 42/divisor 6 vs
    the wrong arm's 6/36), `test_record_event_seed_word_wrap`,
    `test_record_event_uses_live_height_5d`, `test_record_event_one_shot_rearm`
    (`0x7C83E` tracked skip + `0x71D27` rebind: tracked 2/2/1). **FU306-L1 —
    the same-record continuous re-arm: NO-GO (the numbered dead-end).** The
    tracked record is excluded by `0x7C83E` and the setter rebinds
    `tracked := rec` (`0x71D27`), so the native mechanism is a per-record
    **hand-over**, not a per-frame loop; the probe/verdict stands (FU306 §6).
    Legs: **L2/L3/L4 closed** (the ramp param, the live `+0x5D`, the int16
    wrap); **L5** the second caller `0x7C90C..0x7C98C` (no xref, no Ghidra
    function, not in the `0x1106E0` table; reachability unproven; owner the
    `FUN_0007CA54`/`FUN_000782D0` computed-target census), **L6** the
    record-class/activity prerequisites (the tracked record must be active
    and code ∉ {8,9}; the engine mirrors it; the Layer-1 fixture obeys it),
    **L7** the `0x92820(rec,0x1C,ECX=1)` sink has no consumer (OL-27), **L8**
    the chaser cannot bootstrap a cold camera (the tall admission
    `ev > ~0xC8` / the reflect `camY > 0xF0` presuppose a near-tall event —
    the Y producer stays FU307's residue).
  - **FU307 — the keeper-band camera-Y delivery + the single cell.** T3
    landed the three row-`0x1E` **raw-store** cuts (`0x75A0F` = `0x38`,
    `0x75EEA` = `0x10`, `0x75F86` = `0x80`) as the `cam_y_written` output →
    `render.camera.pos_y` (no `0x700F4`), and the **single camera-Y cell**
    (the `keeper_cam_y` shadow deleted; `dword[0x157750]` = the render camera
    Y; the keeper readers, the claim/closedown staging and the arc all read
    one cell). The T3 fix round added the stage-3 slot **place-order** fix
    (the deferred place carries the post-store Y; native place `0x75976`
    then store `0x75A0F`) and the frame-path
    `test_row1e_slot_hold_follow_band_frame`. **FU307-L1 — the natural CX=4
    reach: OPEN.** A fresh probe on the natural M2 tape (temporary, reverted
    instrumentation in `fifa96_keeper_cpu_stage_select`) measured
    `stage_select ran=0 cx1=0 cx4=0 table4=0` over the 165-frame transcript:
    the ladder does not run at all — the AX prepass gate
    (`0x77F9A`/`0x77FA5`, `|cam.x| < 0x420 && |cam.z| > 0x7B0`) never opens
    because the live camera stays `(0,0,0)`; the tape's own m62 block keeps
    the keeper cells fresh and never dispatches the staged row-`0x1E`
    record. The live path is the same chicken-and-egg (the phase-28 gdb
    sample and T6's fresh 10-sample trace both pin the camera at `(0,0,0)`).
    The band is fixture/installer-proven, **not** natural-play-proven; the
    Layer-2 waiver is the consequence (spec §5 Layer-2 + §6). **L2** the
    band → pan co-occurrence (the `0x0E` retry + the `0x8D5D9` code rewrite
    + the stage-7 `0x10`/`0x80` stamps + the `0x737B4` arc) stays open; **L3
    closed** (the raw stores reach the render camera); **L4 closed, Y-only**
    (the single camera-Y cell; the asymmetry is named here: the same
    stage's X/Z stores (`0x759FD` → `0x15774C`, `0x75A25` → `0x157754`)
    still land only on the engine's `keeper_cam_x`/`keeper_cam_z`
    stand-ins — the pre-existing FU-147 leg-13 relation (`0x15774C/50/54`
    is the camera-focus point, asserted not proven). The closure delivers
    the three literal Y cuts and leaves the focus-point X/Z at that leg);
    **L5** the `0x74514` helper's zero-reference status (static
    only; owner the 16-bit loader); **L6** the row-`0x1B` stage cadence (one
    native stage per call vs the engine's 0→1→2; moves when, not the value;
    owner `0x76DEE`); **L7 (new)** the row-`0x1E` stage-1 triple
    (`0x7568A`/`0x7568B` `MOVSD`, no `0x700F4`) is a real `dword[0x157750]`
    write outside the three-literal scope: the claim's first dispatch ends at
    the deferred head place's `pos.y + 0x38` where the native ends at
    `pos.y` (both out of band for a grounded keeper); owner
    `fifa96_keeper_claim_step` / the `0x7568A` site; fold into FU307-L2 if a
    live consumer needs it.
  - **FU308 — the `[0x157750]` stand-ins + the keeper `follow_speed` seam;
    the phase-28 parked seam CLOSED.** T1 fixed the phase-28 final review's
    parked seam: the keeper bind is now
    `f->follow_speed = (int16_t)mr->render.camera.speed` (the native word
    `[0x1577BE]`) instead of the derived pre-tail `camera.follow_speed`
    snapshot; `camera.follow_speed` stays for the `0x739CE` gate only. Pins:
    the unit threshold (`9 → CX=1`, `0xA → closed`) and the frame-path seam
    (walk-tail `speed 0xF` with `follow_speed == 0` → no `0x1A` install/G2
    stamp; mutation-RED under the old bind). T2 re-staged the four `ball.y`
    stand-ins to the live camera Y — rows 05/06/08
    (`0x7F2C8`/`0x7F2E6`, `0x80247`/`0x8061B`, `0x81157`) and the record
    machine's chase gate (`0x7CCD8`), with the chase height widened to
    `int32_t` (signed native compares; `-1`/`-0x8000` stay inside). Pins: the
    three per-row camera-vs-ball pairs, the frame-path chase arm, the chase
    signed-widening arms. **L1** the row-05 second `[0x157750]` read
    (`0x7F4D0`, `SETG`) has no engine consumer (FU171-L6 unported; when it
    lands it must read `render.camera.pos_y`); **L2 closed** (the signed
    widening); **L3** negative camera-Y reachability (owner the `[0x157750]`
    writer set, FU168-L5/FU264-L1); **L4** the same-frame pre-keeper
    `0x1577BE` writers (the bind is order-agnostic; no test stages the
    window; owner the `FUN_0008D8EC` per-record order); **L5** the int16
    truncation in the already-live rows 04/kick (faithful for the bounded
    event heights; owner FU171-L3).
  - **FU309 — the event source + the Layer-1 chain + the acceptance.** T5
    extended `test_keeper_installer_live_reach` Part A on the real frame
    path: the keeper release (`pos_y 0x50`, `tracked 11`) → the row-`0x5E`
    jump → the real `fifa96_match_run_ball_receive` (`traj 0x100`, nonzero
    vector, `has_slot`) → the follow arm (`vel = dirs × 15`) over the
    **54-frame** window derived from the set's own F2/F4/F6 (never a
    hard-coded 52) → the pan-counter lockout + a new setter re-open; the
    row-`0x0E` pan arm is labelled **corroboration**. Seven mutation riders
    each RED (the event call, the receive bind, the `>0xF0` height gate, the
    pan-counter INC, the setter clear, the row-`0x0E` band, the jump arm).
    **FU309-L1** the live `>0xF0` producer chain is not proven live (the
    receive row needs the row-`0x5E` airborne selector or the grounded
    band-2 height and FU307's mid-band Y; owner
    `FUN_0007A084 0x7A207..0x7A219` + `0x15873E` + the code-4 draw +
    `match_run.c:3946`); **L2** the pan-push anchor pair (the engine adds
    `dx/dz` to `target_*` as well as `anchor_*`; the native
    `0x73B30/0x73B36/0x73B42/0x73B48` touch the anchors only; fix site
    `fifa96_camera.c:139-142`); **L3** the rate-nonzero call-site arm is not
    modelled as a branch (structurally unreachable today; owner
    `0x73A03`/`0x73B5B`); **L4** the event-cursor step word (`0x73AA2`;
    owner `0x70240`/`0x709B3`); **L5** no env-gated live camera-cell trace
    exists (this acceptance's read-only `gdb` sample is the corroboration;
    the reusable trace stays a leg; owner the acceptance procedure /
    `fifa96_match_run_frame`); **L6** the still-control AE is host/state
    dependent (this session pan 3421 vs still 2988; the p28 datum re-verified
    1582 vs 2052; the relative gate must be re-measured, no absolute floor);
    **L7** the follow rate needs a moving tracked record (`vel = slot dirs ×
    15`; a stationary tracked record gives `speed == 0` and gate 2 refuses;
    owner the entity pool's dir producer + the acceptance step); **L8** the
    table arm's keeper event ring (`FUN_00092998(1,2,-1)` staged 0, OL-27;
    owner FUN_00092998, carried FU303/FU294-L2); **L9** the Layer-1 mutation
    riders are manual (seven one-line reversions applied once; the spec's
    baseline-RED bar is the erratum below; no automated negative harness
    exists, so a later refactor could weaken the fixture silently). Owner
    `port/tests/test_engine_match_frame.c` + the acceptance procedure; a durable
    form is a small script that applies each one-line reversion into a
    temporary worktree and asserts the fixture fails.
  - **The measured plan erratum (T5; quoted, the plan text is not rewritten).**
    Quote (plan T5 Step 1): "the follow/height assertion fails on the
    phase-29 baseline". **Measured (T5):** the extended fixture is **GREEN on
    the phase-29 baseline `423669f`** — every arm on the staged chain landed
    in phases 9–14 (the follow + rate arm 9, the receive setter 10, the
    window/pan counter 14) and T1–T4 touch cells off this chain (measured
    inert on it). The only measured baseline-RED cell, the T3 `0x38` raw
    store, cannot coexist with the stage-5 release
    (`held_exit`/situation 0xB), so the baseline-RED clause is
    **superseded**: the witness is the **GREEN chain + the seven
    mutation-RED riders** (each producer proven necessary by its own task's
    pin), not a baseline failure. The controller accepted this erratum; no
    test was weakened and no RED faked. The `52`/`54` split is binding: `54`
    at camY `0x50` (this fixture), `52` at `0x70` (FU171-L1).
  - **Deferred minors (T1–T5 reviews; not fixed here).** T5: the
    test-name/300-line split of the extended fixture; the `0x38` premise
    label; the `skip_9a` isolation timing; the report cites. T1: the
    relocated RED note; the `camera.speed` vs `vel_z` discriminator; the
    `0xA` stamp `== 0`; the 82-line duplicate. T2: the
    equal-boundary (`0x38`/`0x70`) arms, the negative-camY row pin, the
    `0x10000` width pin, the reset-helper DRY, the chase fixture helper. T3:
    the two test-comment inaccuracies (the release-list wording; the stage-1
    jump comment) and the out-of-scope FU307-L3/L4 closure notes (now
    recorded here). T4: the z-seed wrap case; the word-diff cast helper.
    None is a fidelity defect; each is a test/comment/clarity cleanup.
  - **Tape:** M1 `09b726b7…` and M2 `b281a9b2…` byte-identical through T1–T6
    (`cmp`-verified; no re-pin, no stop). `make check` 111/111 (strict,
    ASan/UBSan). Census unchanged **`60/80 = 75%`** (`37 action + 23
    phase`); no row joins, so `README.md`, `AGENTS.md` and the
    `port/tests/test_engine_m2.c` pins are untouched.
- **M2 phase-30 leg register (FU310…FU313; 2026-10-22) — the natural reach
  (the first live camera bootstrap + the open-play event): the NO-GO verdict,
  the harness (P1–P8), the fidelity fixes (T2–T5), the bounded spike (T6) and
  the quoted errata (appended; prior sections never rewritten). Census
  `60/80 = 75%` (`37 action + 23 phase`); M1 `09b726b7…` and the final M2
  `8434cfab…` (the T2 re-pin, below) byte-identical.**
  *(See the T7 close-out errata and the final fix-wave erratum appended below;
  this register text stands as written.)*
  - **The verdict — NO-GO for a live natural camera Y/follow bootstrap.**
    The Stage-2 plan (T1–T6) measured, fixed and stopped under spec §6; no
    camera producer was invented. **Layer-1 partial = the scripted pan
    witness only** (the copied-tape claim/kick: place `0 → 0x90 → 0x120`,
    X-only, `Y=0`/`Z=0`, one-shot; P6 partial); **P7/P8 are RED by design**
    (no sustained follow window, no pan-lockout/re-open), and the natural
    window keeps every camera cell at zero (3 598/3 598 granted frames,
    101 s). The reason chain, every link first-hand in the phase-30 wave:
    (a) **the pan is place-only** — the row-04 half-line arm
    (`0x7ED89..0x7EDE3` → `FUN_00071C94` → `FUN_000736AC`) writes
    `pos_x/pos_z` and never Y, and its measured reach is two `0x90` slides
    (`0x120`); (b) **every Y writer is circular** — the restart cuts need the
    goal scanner (`situation 2/3/4`), whose armer (`0x713A6..0x713C0`) needs
    the camera out of bounds (`|x| > 0x730 || |z| > 0xB20`) and whose call
    site needs `|x| > 0x6C0 || |z| > 0xAB0`; no set-piece place crosses both
    (the corner `(±0x710,0,±0xB00)` reaches the boundary handler and fails the
    armer by `0x20`; the penalty, goal-kick and restart-commit places all sit
    below the call site);
    the free-kick/penalty hand-off sits behind the act-2 session gate
    (below); the keeper band and the row-`0x1E` head are ladder-downstream;
    the event arc needs a prior setter; (c) **the keeper ladder is
    structurally out of phase** — the AX prepass needs
    `|cam.x| < 0x420 && |cam.z| > 0x7B0` (plus the sign/side arm), the
    ladder gate requires phase 2 (`0x78583`), while the row-`0x1B` camera
    write is phase `0x0D`-only (`0x76D3D`), and the CX=4 lane window
    (`0x20 <= lane_x < 0x30 + 2*sub_0b`, at most `0x12E`) is closed for every
    produced place — NO-GO even with the lane defect fixed (FU311);
    (d) **the act-2 session gate** — a byte-0 `[0x14C32A]` session aborts the
    native stage-2 body (`0x89487`) before every other term; `kind == 3`
    passes only at the stage-3 re-entry (`0x894B2/0x894B5`), so no ordinary
    (non-offside) contact completes phase 6/7.
  - **The harness (T1: `672f35a` + `55346ec`) — the trace + the natural
    probe + the P1–P8 matrix.** `FIFA96_CAMERA_TRACE` (default-off): a
    filesystem path opens one `"w"` sink at the end of
    `fifa96_match_run_begin`, one line per granted frame at the
    `fifa96_match_run_frame` loop tail, 16 fields (`frame clock phase pos
    vel spd fspd ev cur trk pan tim ramp rate follow tick`; the frame field
    prints as `f`), closed
    idempotently at `_end`/`_teardown`; unset/empty ⇒ no file and one
    pointer test per frame; stdout is never touched (the env-set M2 stdout is
    `cmp`-identical). The probe replays a **copy** of `M2_KEYS` (12 000
    steps): match start 5, the situation-0 row-`0x24` kick at 12, act 1 at
    1 679, **phase 2 at 1 802** (recorded, not hard-coded), **3 598 granted
    frames / 101 s**, every camera cell zero **3598/3598**, the FU282 ladder
    seam **6 118 calls / stage_select `ran=0` / cx1=0 / cx4=0** (sampled view
    `cam=(0,0,0)`, `follow_speed=0`, `lane_x=2541` after T2 — the pre-T2 bind
    read 0). The scripted pan witness (a copied tape + the probe-local
    LONG_BALL 230..233 / UP 240..520 tail, `act.id = 0`, the documented
    `begin_isolated` isolation of a non-camera cell — not a staged gate)
    gives `0 → 0x90 → 0x120` at steps 425/605 (first motion 339, tracked 9),
    velocity `5→3→2→1`, 358 granted frames, `event_param=0`, Y=0/Z=0. The
    matrix: **P1 GREEN** (start 5, phase 1), **P2 GREEN** (natural phase 2),
    **P3 GREEN** (`pos=(0,0,0)` at start — no staging), **P4 GREEN** (the
    ladder seam counts + the sampled input view), **P5 GREEN** (two replays
    byte-identical, an FNV-1a hash over every granted-frame sample), **P6
    PARTIAL** (the scripted pan only; the natural window 0/3 598), **P7 RED
    by design** (no sustained follow window), **P8 RED by design** (no
    lockout/re-open). **P9** (stage-select `cx4`) is recorded, not asserted —
    the keeper route stays FU307-L1; **P10** (the trace self-check) stays
    manual this phase (1 header + 3 598 body lines; the env-set M2 stdout
    identical).
  - **T2 — the `+0x6B` lane-read family (FU311-L5): `24da206` + `0414360`.**
    The keeper bind now packs the native dword
    (`lane69 = (uint16)e->lane | ((uint16)e->lane_x << 16)`; native
    `0x78032/0x78035`), and every `dword[rec+0x69]>>16` reader routes to the
    live `lane_x` word: the kick gate (`handlers.c:972`; native
    `0x81519/0x81520`), the outfield pack, the chase (`match_run.c:915`;
    `0x7CCCD`), the low-word masks (native `0x79B6C` clears `+0x69` only) and
    the remaining family sites. **Option B (the controller ruling):** rows 01
    (`handlers.c:240/246`; `0x7DCCC/0x7DCD7`) and 11 (`:1887`; `0x860DC`)
    keep the legacy low-word read behind quoted comments + **FU311-L5a**:
    the faithful read removes the port's derived kickoff/phase-2 entry
    (`phase 2 at 1802 → -1`, ladder 6 118 → 0 calls) and the M2 corner/ET
    trajectory (golden frame 773 phase 2→4, the ET entry lost) — both are
    reachable only through the missing camera place (FU310's NO-GO). **The
    M2 re-pin (the only one this phase):** the chase correction moves
    **5 568 hash-only lines, frame 955..6522**; every frame id and `state=`
    tag is identical, no assertion or ET directive moved; new sha
    **`8434cfab105b926e78279ca3ea03c62bd3a0ae9654d2e11f0930c0cb8605cee1`**
    (was `b281a9b2…`). Reason: the chase gate now reads the live `+0x6B` lane
    instead of the constant `0xFFFF`; the first chase decision changes at
    frame 955 and the deterministic replay follows. The natural probe is
    unchanged (3 598/1 802/6 118/`ran=0`/zero); M1 identical.
  - **T3 — the per-stage act-2 gate (FU310-L8): `b247126` + `136e795` +
    `56a17cd`.** The flat `kind == 3 || session != 0` compression is gone.
    Engine stage 1 = native stage 2: the byte-0 session performs the
    split/hold (`0x89472..0x8947F` increments, `0x89487 JZ 0x89615`) and
    never decides; engine stage 2 = native stage 3: the `[0x158882]` flag
    (assumed 0, FU166-L6) → `kind == 3` (`0x894B2/0x894B5`) → the session
    term (`0x894B7/0x894BE`), else hold; under byte 0 a kind-3 contact decides
    on the second call. Case B pins the two-call net (split then decision); case C
    isolates the session term. The FU310 act-2 erratum was appended (the T3
    review's Important finding: the plan's "no kind-3 OR" is true for the
    stage-2 body only).
  - **T4 — the restart place triples (FU310-L1/L2/L3): `591cf1a`.** The
    free-kick Y now reads the incident triple's `[0x15889B]`
    (`fifa96_match_run.incident_y`, mirrored by both live producers; the
    foul adjudicator still forces y=0, the offside route preserves it);
    row `0x1D` places the native `0x1104F2` table triple
    `{(±0x1D0,0,±0xA00)}` selected by `[0x157C5E] + side*0x18 +
    12*([0x15777C] >= 0)` (fresh `0x74EC3..0x74F10`, `FUN_000744F4`); the
    per-side ±0xA0 (`0x744D0` = `{-0xA0,+0xA0}`) is the record-copy word,
    not the table. Row `0x20` arm 0 places the fixed `(0,0,+0x8D0)`
    (`0x84F61 XOR EAX,EAX` → `0x73DC4` → `0x700F4`; no side is read — the
    side sign belongs to the row-`0x13` penalty cut `0x84B6B`); the
    `cam_off_z`/`cam_y` caller stand-ins are gone. The old FU307-L4
    row-`0x1D` witness was replaced (the native overwrites Y before the
    record copy); the single-Y-cell read side stays pinned. M1/M2 identical.
  - **T5 — the row-`0x1E` stage-1 triple (FU307-L7 closed): `72d3a66`.**
    The stage-1 cut `0x7568A/8B/8C` is a raw full-triple store
    (`[0x15774C/50/54]`, Y = `pos_y`, the `0x7569F/0x756AA` ±0x20 Z adjust
    stored at `0x756B5`) with no
    `0x700F4`; `fifa96_keeper_claim_out` gains `cam_written` and the binder
    writes `render.camera.pos_x/y/z` raw, carrying the post-store triple on
    the same-step deferred place (the native place order; the head place no
    longer wins at the frame drain); a stage-1 fall-through uses the
    end-of-step triple. The two pre-L7 pins were superseded with the reason
    (not weakened): the head place's offset/pause pins remain. M1/M2
    identical.
  - **T6 — the bounded armer spike (FU310-L9): `ffc4095` + `7f6476c`;
    FAILS.** The scratch probe (`/tmp/opencode/fu310_l9`, not committed)
    reproduced the T1 witness and ran seven scripted drives of 12 000 steps
    / 3 598 granted frames. **The ceiling is identical in every run: max
    x = 0x120 (288), z = 0, y = 0** — the two row-04 pans, still far below
    the OOB call site (`0x6C0`/`0xAB0`) and the armer (`0x730`/`0xB20`); the
    OOB predicate, the armer gate and `goal_armed` are 0 on every frame, the
    scanner returns at its armed gate, and the phase stays 2. Mechanism
    measured: after pan 2 the camera tracks record 10 (`0x71D27`) and the
    half-line event arm's tracked-record exclusion (`0x7EFEE`) refuses it;
    record 10 crosses the arm's own gates on gf 224..226 (angle
    `0xA8/0x97/0xA0` ≤ `0xAB`) but stops at gf 227 (284,−2, angle `0xB4`);
    the long-ball request `0x7CD60` fires once (step 239) and every later
    scripted press is code-gated. **FU310-L9 — closed by measurement; the
    NO-GO stands; no producer and no new leg.** The documented residual (a
    non-pan tracked-cell changer and ≥13 same-direction turns to cross
    `0x730`) is a Stage-3/4 question. The T6 scoped re-review's two text
    minors are folded in the appended FU310 erratum.
  - **Quoted errata (append-only; prior text not rewritten).**
    **(1) "the ladder does not run at all" — refuted by measurement.**
    Quote (`port/ENGINE.md:2338`, repeated at `docs/PROGRESS.md:2554`): "the
    ladder does not run at all — the AX prepass gate (`0x77F9A`/`0x77FA5`)
    never opens because the live camera stays `(0,0,0)`." **Corrected (FU311
    §4 + the review erratum item 1):** the ladder **is called** — 2 019
    times over the whole-binary probe window (18 124 record-0 steps) and
    **6 118 times** on the natural 12 000-step window (`stage_select` 6 118
    too); `ran=0` is the **AX prepass refusal inside** stage_select, not an
    absent call. The conclusion (the band is not natural-play-proven,
    FU307-L1) stands; the mechanism sentence does not. FU313-L3 names the
    two-counter attribution.
    **(2) The row-`0x1E` "dispatch-dead" partial — the body is
    dispatch-ready; the producer is ladder-downstream.** Quote (phase-29
    register, FU307-L1): "The tape's own m62 block keeps the keeper cells
    fresh and never dispatches the staged row-`0x1E` record"; quote
    (FU311 §8.2): "the honest producer is **FU311-L4** (the phase-9 →
    row-`0x1E` head place at `0x755C7`, Y = `pos_y + 0x38`, a real `0x700F4`
    place inside the pan mid-band), which needs no ladder at all".
    **Corrected (FU311 review erratum item 3):** the head place is real and
    the port carries both halves (`match_run.c:3026-3034`,
    `keeper.c:489-501`), but phase 9 ← situation 5, and situation 5's only
    producers are row `0x1A` stage 4 (`0x76B1C`) and row `0x1B` (`0x77707`),
    both ladder-installed; no qualifying place → no ladder → no
    `0x1A`/`0x1B` → no sit 5 → no phase 9 → no `0x1E`. FU311-L4 is a
    **dispatch-shape fact, not a bootstrap**; the row body is dispatch-ready
    (T5 made the stage-1 triple faithful), the missing producer is the
    ladder chain.
    **(3) FU307-L1 — the structural NO-GO.** Quote (phase-29 register,
    `port/ENGINE.md:2334`): "**FU307-L1 — the natural CX=4 reach: OPEN.**"
    **Corrected (FU311 §6.3/§7.2):** NO-GO by three independent reasons:
    (a) the ladder gate requires phase 2 (`0x78583`) while the row-`0x1B`
    camera place is phase-`0x0D`-only (`0x76D3D`) — structural, independent
    of any unported cell; (b) the CX=4 lane window is at most `0x12E` wide
    (`0x30 + 2*sub_0b`, `sub_0b` signed and unproduced — FU311-L2), and every
    produced qualifying place lands 208..464 from the keeper (294..313 best
    case) — closed; (c) there is no qualifying live place at all (FU310).
    `ran=1` would buy **CX=1 → row `0x1A`**, not the keeper band. The band
    stays fixture-proven; the `sub_0b` / lane-window arithmetic is
    corroborative (FU311 review erratum item 5).
    **(4) The FU310 act-2 item 2 — the stage distinction.** Quote (FU310
    review erratum item 2): "`kind == 3` is only the `0x8944A CMP EAX,0x3`
    ball-row `0x49` choice, and `[0x158882]` is tested at `0x894A2` **after**
    the session gate." **Corrected (the appended FU310 erratum, the T3 fix
    round):** the quoted sentence is true of the **stage-2 body only**
    (`0x8947A..0x89487`); the stage-2 abort increments the stage byte
    (`0x89472..0x8947F`), and the next call's **stage-3 body**
    (`0x894A2..0x894C4`) tests the flag, then `kind == 3`
    (`0x894B2/0x894B5` → decision `0x894C4`) **before** the session term
    (`0x894B7/0x894BE`). Under byte 0 a kind-3 contact is a two-call
    decision; the engine models engine stage 1 = native stage 2 and engine
    stage 2 = native stage 3 (T3/`56a17cd`). `[0x158882]` is the only
    unmodelled term (assumed 0, FU166-L6).
    **(5) FU307-L7 — closed.** Quote (phase-29 register): "**L7 (new)** the
    row-`0x1E` stage-1 triple (`0x7568A`/`0x7568B` `MOVSD`, no `0x700F4`) is
    a real `dword[0x157750]` write outside the three-literal scope …; owner
    `fifa96_keeper_claim_step` / the `0x7568A` site". **Closed by T5
    (`72d3a66`):** the raw triple is delivered to `render.camera.pos_x/y/z`
    (X/Y/Z, Y = `pos_y`), the place fix-up carries the post-store triple to
    the drain, and no `0x700F4` is staged. FU311 §7.1 item 4 adds the X/Z
    scope note: `0x7568A/8B/8C` moves the focus X/Z too (the triple, not
    only Y) — exactly the delivered shape. The stage-3/7 raw stores stay
    Y-only when a step starts there (the T3 scope; a start-at-3/7 X/Z
    delivery is not claimed).
    **(6) FU310-L1/L2/L3 — closed by T4.** Quote (FU310 §8): "**FU310-L1 —
    the free-kick cut's Y source.** … the engine passes literal `0`
    (`handlers.c:1983`)"; "**FU310-L2 — row `0x1D`'s place triple.** The
    engine uses `keeper_cam_*` + `cam_off_z = 0`"; "**FU310-L3 — row `0x20`'s
    camera cells.** `0x73DC4`/`0x700F4` are absent (`handlers.c:5844-5851`)".
    **Closed (`591cf1a`):** L1 reads `[0x15889B]` (both producers mirrored);
    L2 places the `0x1104F2` table triple selected by
    `0x157C5E`/side/`[0x15777C]>=0`, with the per-side ±0xA0 as the
    record-copy word; L3 places `(0,0,+0x8D0)`. The plan's "row-`0x20`
    places `(0,0,±0x8D0)` by the team side" is **indicative**: the native
    `0x84F61 XOR EAX,EAX` + `0x73DC4` is the fixed `+0x8D0`; the side sign
    is the row-`0x13` penalty cut `0x84B6B`.
    **(7) FU311-L5 / L5a — the lane family.** Quote (FU311 §3.2): "The bind
    passes `e->lane`, whose high word is never re-synced from `lane_x` (only
    the low word is masked, `:361`). … So the port's `lane_x` is a stale
    `-1`, not the live band." **Corrected (review erratum item 2):** `e->lane`
    is the sign-extended `+0x69` word, so `>>16` is 0/−1 **by construction**
    and can never be `lane_x`; the same high-word read recurs at the family
    sites (`handlers.c:240/246/1683/1887/2037/2183/3701/4546/4823/5176/5900`,
    `match_run.c:915`), and `handlers.c:972` fed the **kick** gate the low
    word where native `0x81519` reads the high word. The fix is the class
    fix, not a one-line bind patch. **FU311-L5 closed by T2** (`24da206`:
    the bind pack + every family site except rows 01/11). **FU311-L5a
    OPEN:** those two derived-row sites (`handlers.c:240/246`, `:1887`) keep
    the legacy low-word read until the missing camera place lands (FU310);
    their faithful read removes the port's natural phase-2 entry and the M2
    corner/ET trajectory (measured, T2) — retest together with any future
    bootstrap.
    **(8) FU310 §5's stale port-state prose.** Quote (FU310 §5): "**Row
    `0x1D` places the wrong triple** …" and "**Row `0x20` does not place the
    camera at all** (`handlers.c:5844-5851`)". **Superseded by T4** (the
    register's (6) above); `port/ENGINE.md:5236/5277` (the phase-7 P3 block and
    the phase-7 close-out ledger) still name the retired `cam_off_z` caller
    stand-in — prior text stands as written, this register is the current
    state. No code or test was changed for the errata.
  - **Deferred minors (T1–T6 reviews; not fixed here; none is a fidelity
    defect).** T1: the witness wording says "appended" though it overwrites
    copied entries; only the last ladder input view is retained; the "no
    follow window" line is inferred from zero cells; P10 deferred. T2:
    `test_action_1a_direction_54` still stages `lane = 0x40 << 16`; the
    `match_run.h` "no writer" comment is false (two writers); the probe
    comment says `lane_x=0` (measured 2541); the chase fixtures stage
    `lane_x=0` (no direct chase pin); the row-26 test stages a dword into
    `lane`; the T2 report §5 "no >>16 remains" overstates the two deferrals.
    T3: the stage-3 decision-word check wording; the `0x81EB4` row-`0x0C`
    attribution missing (the reviewer supplied `0x1106E0[0x0C] = 0x81C90`);
    the comment blocks duplicate. T4: the row-`0x20` sign lacks a side-1
    arm; the `incident_y` producer mirror has no pin; the FU307-L4 coverage
    comment names write tests not the read test; two frame-test comments
    overstate the drain; the RED evidence covers L1/L2 only. T5: the
    stage-1→stage-3 fall-through is unpinned; the frame pin has no RED of its
    own (the RED aborts earlier); the frame pin does not prove the place was
    consumed; the stage-3-start X/Z scope comment should cite FU307-L3/L4.
    T6: the scoped re-review's two minors are folded in the FU310 erratum
    (the kick/pass pan attribution and the five→six count). **The residual
    X/Z shadow split:** `keeper_cam_x/z` is the engine `[0x15774C/54]`
    shadow while the other camera writers still write only
    `render.camera.pos_x/z`; row-`0x1D`'s own place now syncs, but the cells
    can diverge after other places (the FU307-L4 X/Z residual; a full
    unification is a separate slice).
  - **Tape:** M1 `09b726b7…` byte-identical; M2 **`8434cfab…`** byte-identical
    after the T2 re-pin (recorded above; no further re-pin in T3–T7).
    `make check` **111/111** (strict `-Wall -Wextra -Werror`, ASan/UBSan).
    Census unchanged **`60/80 = 75%`** (`37 action + 23 phase`); no row
    joins, so `README.md`, `AGENTS.md` and the `port/tests/test_engine_m2.c` pins
    are untouched.
- **Interactive smoke (phase-8 T5 re-run, this host 2026-10-10; the phase-9 T4
  acceptance re-run is below; the phase-7 P5
  run first verified the phase-7 reach, the phase-6 S5
  run the phase-6 reach and the follow-up-5 T4 run the
  walkthrough):** `make game` window opens (960×720
  integer-scaled SDL3; ESC quits, exit 0 — verified in a clean run; the intro
  and the procedural front-end draw). The walkthrough reaches **match start** —
  intro RETURN skip → front-end BACKSPACE (DECLINE/panel) → RETURN (panel
  confirm → FU-66 STATE16 bridge) — with the derived palette (OL-T11-6) and
  the **match HUD** (P0.2/OL-T11-7) drawn on screen: the Frames.fsh bar at the
  bottom-left with the score pair (0-0) and the `%02d:%02d` clock at kickoff
  `00:00` (`docs/screens/p8-v8-match-hud.png`). **A KICK burst latches row
  01's native release gate**, so the begun run leaves the kickoff wait for the
  live phase 2 and the clock ticks on screen (`00:02` in
  `docs/screens/p8-v8-match-clock.png`). The T5 front-end and kickoff captures
  are byte-identical to the P5 shots (`cmp`), so the phase-8 T1–T4 landings
  moved no live pixels. **The hold
  policy is landed (T3/OL-T4-1):** the SDL backend keeps the pressed set and
  re-presents every held key as a per-poll state sample, so a held key no
  longer pulses; `port/tests/test_engine_sdl3.c::test_held_key_is_a_state_sample`
  holds one KEY_DOWN with no repeats and keeps `input_state[0] == 0x04` while
  the controlled row-00 record moves across the granted frames. **Movement is
  now observable on the live smoke (M2 phase-9 T1):** the ported row 02 turns
  the post-kickoff slot record (team 0 record 9, code 02) into code 4 on its
  first live phase-2 frame and the invoked row 04 writes the slot-direction
  target; the T1 smoke (this host 2026-10-10, the same walkthrough as the T5
  runs) held a direction key for 3 s after the clock started (`00:02`): with
  **LEFT** (`docs/screens/p9-t1-hold-left.png`) the record walked
  `(-58,-2) -> (-58,+178)` and the scene band changed by `compare -metric AE`
  = **45710** (crop `y < 591` of the 960×720 window); with **UP**
  (`docs/screens/p9-t1-hold-up.png`) `(-58,-2) -> (+122,-2)` and scene AE
  **28535**. Holding **RIGHT** instead moves the record away from the
  stand-in camera (`(-58,-2) -> (-58,-190)`; `p9-t1-hold-right.png`, scene AE
  0): the engine's yaw/pitch-0 kickoff view does not draw the negative-z walk
  — the follow/pan camera that would is phase-9 T3. *(Errata, phase-12 T2
  2026-10-11: the yaw/pitch-0 view cited in this pin is superseded —
  `fifa96_match_run_reset_render` now seeds the native camera-0 yaw 0xBF9C /
  pitch 200, FU-154 §5.1; the historical AE pins above stand as measured on
  the pre-seed tape, and the RIGHT-walk framing is re-checkable on the seeded
  view.)* **T3 (2026-10-10) wired
  the pan producers and re-ran the smoke:** the UP hold still gives scene AE
  **28535** (`docs/screens/p9-t3-hold-up.png`) and LEFT **45902**
  (`p9-t3-hold-left.png`), so movement is unchanged; the every-frame
  auto-camera (FU-00071DF4 first arm) is ported but its live trigger (a
  high-ball event height > 0xF0 with a zero-rate camera) is not reachable from
  the ported rows yet, so the visible follow-cam stays the ball-staging leg
  (FU-148 §13). The headless gates are
  `test_engine_match_frame::test_held_key_moves_live_controlled_record` (held
  RIGHT -> position/velocity delta on the real frame path, ISO-gated) and the
  L4.1/L4.2 fixtures
  (`test_machine_forced_decision_installs_on_slot_record`,
  `test_machine_no_edge_arm_copies_camera_target`). The T5 RIGHT-hold datum
  (scene AE 0 with `input_state[0] = 0x04` persisting) is superseded: the
  record reaches code 4 live and the mover integrates the pad target.
  **T4 (2026-10-10) re-ran the smoke for the phase-9 acceptance:** four fresh
  sessions (one per direction/kick, the walkthrough above; client-area capture
  960×720) reproduce the live pixels exactly — the fresh front-end/kickoff
  captures are byte-identical to `p8-v8-frontend.png`/`p8-v8-match-hud.png`,
  and every held-direction capture is byte-identical to its committed T1/T3
  twin (LEFT/UP to the T3 shots, RIGHT to the T1 shot): **LEFT**
  scene-band AE (`y < 591` crop, vs the run's `00:02` kickoff-clock frame)
  **45902** in `docs/screens/p9-v9-hold-left.png`, **UP** **28535** in
  `p9-v9-hold-up.png`, **RIGHT** 0 (`p9-v9-hold-right.png`, the walk away from
  the stand-in camera; full-frame AE 228 = the clock only). A live-play KICK
  burst leaves the
  scene band at AE 0 (full-frame 228 = the clock; the captured frame is
  byte-identical to the RIGHT capture at the same clock value): the pad
  kick's carrier is now producer-live **headlessly**
  (T2/OL-63 — `test_live_carrier_claim_and_kick`), but nothing in the ported
  live path puts the ball on the canvas (the ball staging/render producers
  stay legs), so the smoke claims no on-screen kick. **The camera does not
  visibly follow yet** (`make game` scene band static apart from the moved
  record): T3 ported the every-frame auto-camera call site + first arm and the
  row-04 pan origin, but its live trigger needs the ball-staging high-ball
  producer (`FUN_0007A490`/`FUN_0007A084`), which stays an OL-T11-79 leg —
  no overclaim.
  **T4 (2026-10-11) re-ran the smoke for the phase-10 acceptance (fresh
  `p10-t4-*` captures, the same walkthrough; two hold sessions plus a
  no-hold kick-delta session):** the front-end and HUD captures are
  byte-identical to `p9-v9-frontend.png`/`p9-v9-match-hud.png`, and
  **movement is still clearly visible** — the LEFT hold changes the scene
  band (`y < 591` crop) by ~5.7e4 px and the UP hold by ~1.5e4 px. The
  scene band no longer reproduces byte-identically across sessions (the hold
  captures differ from the phase-9 twins; the HUD band still matches
  exactly): with the phase-10 T2 L4.1 unbound walk the live roll now moves
  the non-controlled records every frame — a no-key 0.6 s pair already
  changes 4094 scene px — so capture timing modulates the walked record's
  position. **No visible follow-cam and no on-screen ball, honestly:** the
  camera stays the static stand-in view (the walked record slides to the
  frame edge instead of the camera re-centering; `p10-t4-hold-left/up.png`),
  no ball sprite appears (the no-hold kick session's `p10-t4-kick.png` and
  the UP session's `p10-t4-pass.png`), and the no-hold KICK burst's scene
  change (10709 px) sits inside the
  live-roll range (baseline 4094, the following no-key interval 11676) — no
  kick-attributable on-screen element (the ball record's open-play position
  producer is the L14.8 leg).
  **Kick (gameplay) stays blocked on screen; the score now runs on the
  acceptance tape (phase-10 T3/v10).** A Z-burst during live play still
  shows no kick-attributable scene change (the T4 delta measurement above and
  the `p10-t4-kick.png` capture),
  so the KICK press dispatches no scene-facing gameplay row. The natural
  goal *producer* exists
  (T2 wired row 04's event outputs to the real pan setter) and the chain is
  closed end-to-end by T4 (natural goal -> score 1-0 in
  `test_engine_match_frame::test_natural_goal_end_to_end`), and the phase-10
  T3 band-reachable acceptance now puts the natural goal on the tape
  (v10: m 47..65, `state=0/1-0` from frame 66); the interactive smoke still
  never reaches a code-4 half-line goal, so on screen the score stays
  tape/fixture-proven. The
  goal **consumer** chain was landed by S3 (FU-146): begin installs the
  goal-screen machine (`0x92D8C/0x92E2C`), the frame body runs the
  session-gated scheduler (`FUN_000948AC 0x4B1A1`, before the clock body), and
  a queued goal id is consumed by the installed period handler into
  `fifa96_match_run_score_event` — the score increments through the native
  chain in a fixture
  (`test_engine_match_frame::test_goal_consumer_chain_fixture`); the
  S3-era note "both goldens stay byte-identical (the tape camera never pans,
  so no natural goal enters the tape)" is superseded by the phase-10 v10
  re-pin. `fifa96_match_run_goal_queue`'s direct fallback now honours
  the native `[0x157AC2] in {2,3}` phase-5 skip. The P5 smoke shots' measured
  content (identical to the S5 re-run): 3.60% non-black window pixels (the
  kickoff frame: 24913 non-black of 691200), dominated by the HUD bar's
  `#900808` (11301 px); the sprite color
  `#E044A0` (the OL-T11-6 6-bit `0x38/0x11/0x28 << 2`) is live (1200 px).
  **Phase-11 T1 smoke (2026-10-11, `DISPLAY=:1`, fresh session,
  `p11-t1-*`):** the front-end and HUD captures are byte-identical to the
  phase-10 twins; the long-ball walkthrough (walk UP ~1.2 s -> V -> walk UP
  ~0.6 s -> eight Z presses over ~4 s) reaches live play (clock 00:12 ->
  00:17) with the movement/live roll visible in the scene band
  (`p11-t1-walk.png` -> `p11-t1-long-ball.png` -> `p11-t1-kick.png` ->
  `p11-t1-after-kicks.png`; kick-burst scene-band AE 14894/24230), and —
  honestly — **no visible follow-cam and no ball sprite**: the camera stays
  the static stand-in. The live camera Y cannot reach the `d == 0x70`
  band-2 stage boundary from a cold start (no wired live event carries
  height >= 0x74; FU-148 §15.3 leg 15.a), so the follow remains the
  headless chain (`test_live_long_ball_mode40_band2_follow`).

  Reached vs blocked (T4 acceptance re-run, 2026-10-10; identical reach to
  T5/P5/S5):

  | step | state | evidence |
  |---|---|---|
  | window + intro + front-end draw | reached | `p9-v9-frontend.png` (byte-identical to the `p8-v8-frontend.png` T5 shot); ESC exit 0 (clean run) |
  | panel DECLINE/CONFIRM → match start | reached | match canvas replaces the front-end; `p9-v9-match-hud.png` (byte-identical to the T5 shot) |
  | RGB palette on the match canvas | reached | `#E044A0` sprite pixels; tape frame-6 palette assertion |
  | match HUD (bar/score/clock) | reached (on screen) | `p9-v9-match-hud.png` (0-0, 00:00); tape frame-6 bar-pixel assertion |
  | kickoff → phase 2 naturally | reached (on screen) | KICK burst → clock `00:02` in `p9-v9-match-clock.png` (byte-identical to the T5 shot); tape v10 `run_natural_probe` (phase 2 at step 215, row 01 dispatched) *(errata, phase-18 T3/T4: the live situation-0 kickoff cutscene moved the natural phase 2 behind the native act-1 route; `run_natural_probe` now observes phase 2 at step 1802 after the act-1 arming at step 1679 — the phase-18 T3/T4 bullets; the phase-9 measurement is kept verbatim)* |
  | move the controlled player | **reached (M2 phase-9 T1; T4 re-run)** | the ported row 02 (FU-75 L4.6) turns the post-kickoff slot record (team 0 record 9, code 02) into code 4 on its first live phase-2 frame and row 04 writes the slot-dir target; the T4 re-run held **LEFT** for 3 s (`p9-v9-hold-left.png`, byte-identical to the T3 shot): scene-band AE 45902; **UP** (`p9-v9-hold-up.png`): AE 28535; **RIGHT** (`p9-v9-hold-right.png`): scene AE 0 (the yaw/pitch-0 stand-in camera does not draw the negative-z walk; *(errata, phase-12 T2 2026-10-11: that view premise is superseded by the native camera-0 seed — FU-154 §5.1 — the AE pins above stand as measured pre-seed)*). Headless gates: `test_held_key_moves_live_controlled_record` (ISO), `test_machine_forced_decision_installs_on_slot_record`, `test_machine_no_edge_arm_copies_camera_target` |
  | follow-cam / auto-camera (live) | **not visibly following yet (M2 phase-12 T1; honest smoke; `p12-t1-*` 2026-10-11: live motion, no re-centering; T4 re-verified `p12-t4-*`: movement + ball slot on screen, no re-centering; phase-13 T1 `p13-t1-*`: the claim pan is asserted live, the throw-in Y cut restored, no visible follow; phase-14 T2: the sustained window + pan-counter lockout are pinned through the granted-frame path (52 frames per k=7 event), the row-04 camera-Y/tracked coupling is native, and the visible follow still needs the live camera-Y producer (FU168-L5 / FU171-L4) — the `p14-t2-*` smoke is honest)** | the every-frame FUN-00071DF4 call site + first arm, the row-04 pan origin, the `FUN_0007A084` receive arm (wired in `match_kick_run`) and — phase-11 T1 — the `0x157750` camera-height producer (FUN_00070B94 + the integrator store, `fifa96_camera_height`) with the record-event tail gate and kick band on the live cell are ported; the follow arm's `height > 0xF0` step is fixture-proven through the producer (`test_ball_receive_arm_high_ball_chain`) and **the `0x40` long-ball key is live** (the V request -> code-7 stage-1 `is_team_cb` -> mode 0x40: `test_live_long_ball_mode40_band2_follow` proves flags 0x40/band-2/code-4/traj 0x100/receive/follow-1 with the camera Y at the band boundary); **phase-12 T1 (FU-153) ports the live record jump producer** — the FUN_0006E8E8 animation-aux-keyed starter (the verified rows 0x5E/0x12/0x14/0x5C aux arms) + the `FUN_0007BF20 0x7C057..0x7C113` jump arc (`jump_base`/`jump_timer`, the shared `0x10F3C4` arc via `fifa96_camera_arc_height`) over the pool — and the live ISO chains: the airborne height-table code-4 receive sets the camera event to the k=5 exact-hit `0xE0` (`test_live_airborne_long_ball_seed_then_band2_follow`, A) and a second fresh chain crosses band 2 at the exact `0x70` arc sample with `traj/event_param 0x100` and `fifa96_match_run_camera_follow == 1` (B); the recording still cannot run fully live end-to-end: the live camera bootstrap (no ported producer raises `0x157750` off zero; the height table needs `d >= 0x38`) is leg **T1-L3** (phase-13 T1 restored the throw-in stage-3 Y producer, `[0x157750] = 0x80`; the live-selector hand-off stays T1-L1/T3) and the row-07 stage-2 `+0x44` animation ack that blocks the second kick of one possession is leg **T1-L2** (FU-153 §10) — the earlier `p11-t1-*` posture holds (`p12-t1-*`: no re-centering); **phase-14 T2 (FU-171) pins the sustained contract**: the follow rate is the tracked slot dirs x 15 re-set every frame while `[0x1577F0] > 0xF0` and `[0x157821] == 0`, the window closes at the first pan step and re-opens at the next setter call, and the row-04 snapshot stages the native `[0x157750]`/`[0x1577CA]` cells (`camera.pos_y`/`camera.tracked`); the remaining live-Y producer stays FU168-L5 (the keeper release is dispatch-dead R11-L4; the throw-in cut is restart-driven), and the residual `ball.y` stand-in sites are FU171-L7. *(errata, phase-26a, 2026-10-14: the live camera-Y producer is now ruled the **honest Layer-2 waiver** (spec §7.1/D1) — the only sound event-raising producer for the follow/pan window, the keeper stage-5 release (`camY := 0x50`), is **dispatch-dead behind R11-L4** (rows `0x1A`/`0x1B` sit-5 producers with no live installer — the situation-5 → phase-9 → row-`0x1E` chain), which is **phase-scale** (~the keeper CPU chain `FUN_0008D8EC`→`FUN_000782D0`→`FUN_00077EAC` + the phase-6/row-0x1F chain + the claim bridge). The camera/follow chain therefore stays the numbered legs **FU264-L1/L2**; R11-L4 routes onward to a later phase as the enabling leg for the eventual visible anchor. Derivation: `port/spec/FU272_r11l4_derivation.md`.)* |
  | kick the ball (gameplay) | **carrier + receive arm live (T2/phase-10 T1, headless); live long-ball kick path landed (phase-11 T1, headless); ball machine landed (phase-11 T2): the record is live, drawn and moving; the on-screen ball landed — the phase-12 T2 native camera-0 view seed projects the slot on canvas (16/16 kick-roll frames in the live ISO pin; M2 v12 re-pinned; the camera place/pan stays the T1/R1 event leg R2-L4, and the slot's sprite content is leg T2-L1)** | the pad kick path is derived and wired first-hand (T3: carrier `+0x91 == 5` → code-1 released row `0x7D110` → install 7 invoke → row 07 kick); the possession/carrier producers are landed (T2: `test_live_carrier_claim_and_kick`) and the PASS kick now runs the `FUN_0007A084` receive arm live (T1: `test_live_pass_kick_runs_receive_arm` — ack/receiver/tracked/controlled/release); phase-11 T1 adds the live `0x40` long-ball request (the V key → FUN_0007CD60) and the mode-0x40 kick chain (`test_live_long_ball_mode40_band2_follow`); **phase-11 T2 (FU-148 §16)** ports the `FUN_0008BAF0` ball machine (L14.8) — the only per-frame ball-record position producer — into `match_run_ball_machine` (the phase-2 camera-derived lead + the metric/heading/velocity-ramp/integration tail), seeds the reset/commit posture and stages the reset's row-3 appearance (PLAYART bank 3). Live, the record moves and the slot resolves/draws (`test_live_ball_record_produced_in_open_play`, `test_ball_machine_moves_ball_slot_pixels`, smoke trace `slot22 … sprite=yes`); **phase-12 T2 (FU-154 §5.1)** seeds the native camera-0 view (yaw 0xBF9C/pitch 200 in `fifa96_match_run_reset_render`), so the camera-derived lead now projects on canvas (the live ISO pin measures 16/16 kick-roll frames, x' 152 -> 4; the tape's m 58/61/72 park samples flip drawable with the lateral bound pinned; M2 re-pinned to v12) — the live camera *place/pan* stays the T1/R1 event coupling (leg R2-L4); **phase-13 T2 (FU-158)** lands the slot's real ball art: the render captures GAMEART0 `ball.fsh` (`render.ball_bank == 128`) and draws it at slot 22's projection with the composer roll byte (`render.ball_roll`, the native `[0x155C4E]` low nibble; ball-motion stand-in for the camera-rate gate, L158.2; presenter sanitizer 0x5739D), so the smoke shows a soccer ball instead of the PLAYART pose (M2 re-pinned to v13); the FU-84 row-3 install stays the first-hand record and the executable per-row tables remain the player-path leg; the ball height (`y`) stays 0 (leg 15.a/15.b unchanged) |
  | score a goal | **reached naturally on the tape (M2 phase-10 T3); the tape now continues past it (phase-14 T3, M2 v14)** | the band-reachable acceptance drives the natural chain on the tape: the m 42 scenario (every forcing declared) stages the camera park + record placement (the f-up8 T4 fixture's staged inputs) plus the claim-isolation and act-1 flag forcings, and from there the row-04 slot-dir event -> pan -> armer (zone 1) -> situation 6 -> table-1 queue (id 5) -> scheduler -> leg-0 handler -> FUN_00093944 writer scores 1-0 with the untracked arm's 0xD3 post (tape frames 47..65, `state=0/1-0` from frame 66); the m 41 code-4 record's lane enters the band natively (measured, ~62 granted frames after m 41), the claim machine's mid-walk slot steal and the ~4400-step mouth co-migration are the recorded blockers that make the rest staged; fixtures `test_camera_pan_event_chain`, `test_row04_live_pan_arms_camera`, `test_natural_goal_end_to_end` retain the unforced positives; **phase-14 T3 (FU-170)**: the hold releases through the derived restart chain and the tape runs to completion — act-9 phase 0x15 (lines 370..469) -> sit-3 phase 4 (470..772) -> the corner taker's situation-0xB resolve back to phase 2 (773..792, `state=2/1-0`), the taker row 0x11 in the post-run mask |
  | set pieces / restarts (live) | **the post-goal restart chain runs (M2 phase-14 T3, v14); the byte-0 live release is bounded** | the restart scanner requires `phase 2 && goal_armed`; on the tape the post-goal hold's TAIL advance releases through the derived FU-170 wire (SETUP0 sit-0xC -> the act-9 producer -> SETUP2 sit-3 -> the corner arm -> phase 2), the taker rows 0x10..0x13 execute and resolve in the armed fixtures (`test_taker_armed_rows_resolve`/`test_taker_armed_referee_rows_resolve`) and row 0x11 now dispatches on the tape; the engine's byte-0 default scores via the direct `0x8AA7B` arm + `phase_arm(5)` (the celebration codes), but the release (row-0x14 stage 2) stays unported, so a live byte-0 match still holds at phase 5 (FU173-L2/L3); FU-149 P1 queue/arm/scan and the counter are fixture-proven. *(errata, phase-16 T2, 2026-10-12: the "release (row-0x14 stage 2) stays unported (FU173-L2/L3)" premise above is SUPERSEDED, not rewritten — T2 lands exactly that release: `fifa96_match_action_14` stage 2 → `fifa96_match_run_kickoff_entry` (native `0x87C71`), so a live byte-0 match now continues past a goal; the old text is kept verbatim)* |
  | fouls / offside (live) | blocked; chains fixture-proven | the FU-150 entries have no live producer; `test_referee` + the `test_engine_referee_*` chains prove them, and tape v10 pins `ref_machine == REF_NONE` with no whistle/speech/decision cells |
  | keeper restart rows 1D/1E (live) | blocked; row 1E off the v10 tape (T3 isolation) | the taker rows 0x10..0x13 are ported but dormant on the tape (the set-piece arms never fire; FU-149 L13/T1); row 1E's m 41 staging would reach the phase-2 window, but the phase-10 T3 goal scenario isolates the claim record before the first phase-2 grant (the native `+0x9A` exclusion set at m 42 — on the pre-re-pin tape its 1E machine dispatched and stole the slot mid-walk every granted frame, measured, which kept the code-4 record's row-04 event arm off the slot-dir path), so the v10 tape never dispatches it: the staged record keeps code 0x1E and the keeper cells are pinned fresh at the goal link (m 58: code 0x1E, `has_ball` 0, the `keeper_cam` triple zero; the exact masks and the golden cover the rest); the claim's own chain stays fixture-proven (`test_keeper_claim_*`/`test_keeper_machines`); row 1D is fixture-proven (`test_keeper_machines`) |
  | replay / overlay / sub strip (live) | blocked | zero-gated rows with no producers yet (FU-152 legs 1/2/3/7); tape v10 pins `replay.state`/`sub.active`/`overlay.armed` 0 and `ball_row` NULL |
  | half/period end → exit | reached (headless) | tape: live class-1 period end → phase 0x0C → OVER→POST→EXIT; native periods last minutes, so not run to completion in the smoke |
  *(errata, phase-26a, 2026-10-14: **`port/spec/FU265` §1.4 rows 6/9 status is
  stale.** Those rows assert the `FUN_00071C94` caller bodies at `0x77423` (row
  1B) and `0x77dc6` (row 1C) are "**NO** (row unwired) … dispatch
  `-FIFA96_ERR_UNSUPPORTED`". Rows `0x1B`/`0x1C` are **wired/counted**
  (`fifa96_match_handlers.c:5514/5516`; FU-180 §8). The accurate statement: the
  rows are wired, but their `0x71C94`-bearing arms — the row-1B stage-2 wide body
  (`0x77423`, FU-180-L1) and the row-1C tail (`0x77dc6`, FU-186-L2) — are
  unported, so those two callers are not reached, but not for the stated reason.
  See `port/spec/FU272_r11l4_derivation.md` §6.)*
  *(errata, phase-26b, 2026-10-14: (a) the **fouls / offside (live)** row above
  reads "the FU-150 entries have no live producer" — the accurate statement is
  **producer-wired, unraised** (`FU276` §1.3): the pad handler `0x7CE38`
  (want 0x60) and the row-`0x04` AI duel each install row `0x0B` with invoke, and
  the ported chain row-`0x0B` arm-1 → row-`0x0C` → `fifa96_match_run_contact`
  exists and is unit-proven; what was missing was the headless/tape/smoke raise.
  Phase-26b T4 supplies exactly that — the contact gate is now
  **headless-proven reachable** via the pad handler (leg **FU276-L1 resolved**),
  while the row-`0x04` duel stays an unraised leg pending the unported
  descriptor/`+0x9D` family (`fifa96_outfield.c:595`). The row is kept verbatim.
  (b) the **replay / overlay / sub strip (live)** row above is refined from
  "zero-gated rows with no producers yet" to an explicit **LEG** ruling (**no
  port**): `replay.state` / `sub.active` / `overlay.armed` are **LEG** (the whole
  period/screen-machine caller chain is unported — `FU278` §3.3: the per-tick
  dispatcher `FUN_0004B100` from the match loop `FUN_000493A0` →
  `FUN_000513EC`/`FUN_00053BB8` → `FUN_00051270` → `FUN_000510DC`, with the mode
  machine `FUN_00052030` via `FUN_00053A48` as installer) and `ball_row` stays
  **pinned dormant** (`FU278` §2.4: `[0x14E660]`; `FUN_00056690`'s only caller
  passes 0). On the port no producer raises the gates — every replay/overlay/sub
  engine row (`fifa96_match_run_replay_step` `run.c:4624`, `_overlay_arm`
  `:4756`, `_overlay_timeout_step`, the `_sub_*` setters,
  `fifa96_match_run_ball_row_reachable`) is referenced only from `port/tests/`, so any
  "live" claim would be a fixture-only stand-in. New legs `FU278-L1..L7`; no port
  this phase.)*
- `test_engine_m1` pins the 691-frame M1 transcript
  (`port/tests/golden/engine/m1-frames.txt`).
- `test_engine_m2` replays spec §5 (boot → skip intro → front-end → start match
  → kickoff → move → kick → score → post-goal hold) and pins the 85-frame M2-B
  transcript (`port/tests/golden/engine/m2-frames.txt`, re-pinned by the phase-10 T3
  natural goal — the first re-pin since the S1 v5.1; the old tape's period-end
  exit belonged to the direct-score path, which T3 removed):
  `frame=<n> hash=<hex>` plus `state=<phase>/<home>-<away>` while a match is
  live, the forced kickoff phases 0x13/0x14, the wired-row `FIFA96_OK` dispatch
  set, and the score step. Tape **v8** (the phase-8 live-loop acceptance, T5;
  v7's phase-7 acceptance — P5 —, v6's
  phase-6 full-gameplay acceptance — S5 — and v5's M2 interactive G3
  acceptance with the v4 G4/v3 provenance are retained as the assertion
  lineage) stages the
  wired rows (including the G1 rows 04/08), asserts the natural path:
  the KICK press reaches the run but dispatches no gameplay row, the kickoff
  placement is pinned at match start (ball 0x1E0/0/0, `anim_id` 0x26), the
  **formation seed** loaded `352ko.fmt` so records carry real positions
  (record 0: team 0 z = -2376, team 1 z = +2508), and the live FU-143 driver's
  derived 0x0C write is read back after the exit step (where the teardown has
  reset the match state but not the FU-142a mirror). The Task 3 acceptance
  asserts the **drawing directly**: the pre-grant match frames render the empty
  list (0 non-background pixels) and the first granted frame (step 9) stages
  all 23 render slots and puts the placed records on the indexed canvas
  (pixels > 0; team 1 record 0 staged at z = +2508 with the kickoff selector
  row 0x26) — the golden hash chain is corroborated, not trusted alone. Task 2
  adds the **derived
  kickoff phase entry (OL-84)**: `fifa96_match_run_begin` installs
  `FIFA96_MATCH_RUN_KICKOFF_PHASE = 1` (the native `FUN_00088DC8` stage-0
  `FUN_000740A0(1, side)` write at `0x88E82`), so a begun run starts at the
  kickoff-placement phase (class 0) and the tape's m 1 directive forces the
  live phase over it. Follow-up-4 T2 lands the **residual**: begin also runs
  the derived `FUN_0008D098` state-1 arm
  (`fifa96_match_phase_machine_kickoff`), the wired action row 01 turns the
  derived act-1 producer (`global_5882a` at `tick_total >= 0x78`) into the
  situation-0xB -> `0x8AEF6` -> `0x8AF02` phase-2 write, and
  `test_engine_match_frame::test_kickoff_enters_phase2_when_cutscene_isolated`
  pinned a begun run reaching the live phase 2 with no forcing (FU-143 §11).
  **M2 phase-18 T3 correction:** the live situation-0 kickoff cutscene now
  pre-empts this derived chain, so a begun run **holds in the cutscene and does
  not reach phase 2 naturally**; the fixture reaches phase 2 only with the armed
  act cleared (the native act-1 route is unported, leg FU202-L2/FU203-L2).
  *(errata, M2 phase-18 T4: the act-1 route is now **live** — a begun run again
  reaches the live phase 2 naturally via the phase-`0x20` stage-2 re-dispatch →
  situation 1 → act 1 → phase `0x17`; the T3 text above is kept verbatim — see
  the phase-18 T4 bullet.)* The 0x13/0x14
  forced window **stays** because the natural chain is not frame-for-frame
  identical to it (the forced window drives the FU-142a 26/28/2A arm staging
  and its `state=19/20` lines; measured mismatch, v3-era: hashes identical
  through golden line 48, first divergence at line 49, natural phase 2 at
  presented frame 410 = granted frame ~121, the 0x78 + 0x3C + 0x78 timers —
  FU-143 §11.5; the T1 key-tape change moved the natural probe's phase-2
  landing to engine step 215 and did not retire the forcing). The T2 tape
  transcript is
  **byte-identical** after the wiring (no re-pin; the wired-row observation set
  grows to 14 rows: row 01 joins and row 00 dispatches through the reset path a
  wired row requests — S1 erratum: with the `+0x8D` seed the reset installs a
  decision code instead, so the tape stages code 0 explicitly at m 41). The
  **T1** transcript changed
  and the golden was re-pinned for the drawing upgrade: the first differing
  line is frame 9 (the first granted render staging), 157 lines differ (the
  null backend chains its present hash across all presented frames, so the
  post-exit front-end lines carry the earlier change while their canvases are
  repainted identically). The **follow-up-4 T1** transcript changed again and
  the golden was re-pinned for the **palette upgrade** (`OL-T11-6`):
  `fifa96_match_run_render` installs the staged palette from the first match
  present on, so the first differing line is frame 6 (match start is frame 5;
  frame 6 is the first MATCH-branch present), all 160 lines 6..165 differ, and
  every `state=` suffix is unchanged. The **full-gameplay P0.2** transcript
  changed again and the golden was re-pinned for the **HUD upgrade**
  (`OL-T11-7`, FU-148 §10): the bridge stages the HUD entries from GAMEART0
  (clockfnt.fsh/playfnt.fsh/frames Frames.fsh) and the render draws the
  derived bar + score/clock overlay from the first match frame, so the first
  differing line is frame 6 (golden `eece28cb8ebe5731`, actual
  `9c940e7b1ac18675`) and 160 lines 6..165 differ (6..145 carry the canvas
  change, 146..165 the chained present hash); every `state=` suffix is again
  unchanged. The **full-gameplay S1** transcript changed again and the golden
  was re-pinned for the **possession/locomotion producers** (`FU-147`, phase-6
  wave-2 S1): the `+0x8D` active seed (`FUN_0008C2E0 0x8C329`) changes the
  kickoff `0x79C13` selector row for records other than 0 (0x26 -> 0), the
  shared mover now runs for **every** dispatched record (`0x8DB2E..0x8DB5F`)
  with the BF20 lane/bound track, and the row-1E `+0x9B` write-back plus the
  stage-3 possession flip are live; the tape also stages code 0 explicitly
  because the seeded native reset installs a decision code on active records
  (`FUN_0007DAB4 0x7DAE6 -> 0x7C990`; see the S1 gap entry). The first
  differing line is frame 9 (the first granted render staging; frames 1..8 —
  boot, intro, front-end, palette- and HUD-only match frames — stay
  byte-identical), 157 lines 9..165 differ, and every `state=` suffix is again
  unchanged; the natural probe still lands live phase 2 at step 215. The
  **full-gameplay S2** (goal arming, `FU-145`, phase-6 wave-2 S2) transcript is
  **byte-identical** (no re-pin): the tape camera never pans (the only triple
  change is the row-1E claim place, T5), so the new armer never fires and the
  clock-tail goal scan is a gate
  no-op — the "dormant chain" risk materialised as predicted; the chain is
  fixture-proven (`test_engine_match_frame::test_goal_chain_pan_fixture`, a
  camera-velocity pan past the bounds → armed → snapshot → queued situation 6).
  The **full-gameplay S3** (goal consumers, `FU-146`, phase-6 wave-2 S3)
  transcript is likewise **byte-identical** (no re-pin): with the camera never
  arming no queued goal id exists, so the begin-installed screen machine stays
  gated and the scheduler/session gate only run the dormant path; the chain is
  fixture-proven (`test_engine_match_frame::test_goal_consumer_chain_fixture`:
  installer → scheduler → handler → `score_event` scores 1-0).
  The **full-gameplay S4** (presentation completion, `FU-148` §2–§4) transcript
  is likewise **byte-identical** (no re-pin): the pose feed's `view_mode`
  default 0 is the unported handler arm, no translation pool is staged, and the
  formation id defaults 0 (`352ko.fmt` as before); the new pan event setter has
  no natural invoker, so the tape camera never pans (T5: still true — the only
  triple change is the row-1E claim place). The S2 L1 chain is
  now producer-proven (`test_engine_match_frame::test_camera_pan_event_chain`:
  the real `fifa96_camera_event_set` pans the integrator into the arming band
  → queued situation 5 → S3 consumer score).
  The **phase-8 T2** (camera live feed, `FU-148` §12) transcript is likewise
  **byte-identical** (no re-pin): the row-04 pan origin is now wired live, but
  the tape's code-4 rows dispatch with lane ~1460+ (the half-line event gates
  fail), so no event fires and the pan never runs (v8 asserts the pan/event
  cells fresh; the row-1E claim place is the camera's only triple change); the
  natural chain is
  producer-real in `test_engine_match_frame::test_row04_live_pan_arms_camera`
  (live action-04 record → event → armer → queued situation 5 → score 1-0).
  The
  tape's scene-pixel evidence is counted above the derived HUD
  band (`bar_y = 240 - 41 - 2 = 197`; count `y < 190`), so frames 6..8 still
  assert zero SCENE pixels, and frame 6 asserts the staged bar's first-hand
  frame-13 pixel (0,0) = 0x45 at (2,197). The tape asserts the RGB directly: at
  frame 6 `render.palette_ready == 1`, the surface palette's staged **chunk
  entry 1** is 6-bit (0x38,0x11,0x28) -> (0xE0,0x44,0xA0), and > 700 of the 768
  palette bytes are nonzero; `render.palette` equals the pure extraction over
  the staged `PALsys.fsh` bank (`test_engine_match_staging`). **v4** adds the
  canvas-level RGB assertion (some non-background frame-9 canvas pixel maps
  through the installed surface palette to a non-black triplet), the
  `run_natural_probe` (the same tape with no directives reaches the live phase
  2 at step 215 with row 01 dispatched and the score/writer cells fresh — the
  v4 text's step 409 was the pre-T1 key-tape timing) and
  re-states the T3 negative score and the 14-row dispatch shape (row 01 joins;
  row 00 dispatches from the explicit m 41 code-0 staging — the S1 seed change
  means the reset no longer produces it). The transcript is
  byte-identical in v4 (no re-pin), so playable-match T2/T3/T4 moved no
  presented frame. **v5/T4** (M2 interactive Task 4 / G3) adopts the transcript
  as the interactive-match acceptance: the m 1 witness pins the pad bind
  (`slot.entity == team[0].target`, `has_slot`) plus the single action-1
  taker, the m 41 witness pins the slot-bound record's moved position and the
  T1-review word/dword velocity lockstep, the match-start block pins the T2
  ring targets (records 9/10 at `(251,291)`/`(-251,291)`, controlled side
  untouched) and the frame-6 block pins the HUD readiness + the bar pixel. The
  transcript is byte-identical (the `cmp` against the committed v5 golden is
  the recorded check; no further re-pin); every forcing is listed with its leg
  in the file's v5-acceptance provenance, and the goal-invoker work that
  blocks the score step is owned by the next plan phase (frozen FU-145/FU-146
  → phase-6 S2/S3). **v6/S5** (phase-6 full-gameplay acceptance) adopts the
  transcript as the milestone acceptance: it adds the phase-6 assertions —
  the S1 `+0x8D` active seed and the slot record's BF20 lane-track invariant
  (`lane_x`/`lane_z`/`cam_dz6f` camera-relative, `bound`), the S2/S3
  dormancy/freshness pins (armer, queue, screen gate, live-session gate), the
  S4 defaults (`view_mode == 0`, formation id 0) and the retained HUD entry —
  and is byte-identical (no re-pin; `cmp` is the check; the current golden is
  the S1 v5.1 re-pin, 157 hash-only lines from frame 9). M1
  stays
  byte-identical. **v7/P5** (phase-7 acceptance) adds the P1–P4 assertion
  layer: the row-1E keeper machine is tape-reachable through the m 41 staging
  (the claim take sets `+0x9B` and writes the 0x15774C focus triple —
  `(-118,56,-66)` ISO / `(-4,56,0)` no-ISO — pinned at m 62), the FU-149/
  FU-150/FU-152 cells are asserted fresh at m 41 and m 62 with each dormancy
  gate named (scanner `goal_armed`, `ref_machine == REF_NONE`, the zero-gated
  presentation rows), and the remaining forcing inventory is restated with its
  owning legs. It is byte-identical to v6 (no re-pin; `cmp` clean, 165 lines;
  M1 unmoved) — the phase-7 ports moved no presented frame. **v8/T5**
  (phase-8 live-loop acceptance) adds the phase-8 assertion layer: the T1
  taker mask absent + the `sp_*` staging cells fresh (armed-proof in the
  taker fixtures), the T2 camera at the reset triple at m 41 and at the
  row-1E claim-place triple at m 62 with every pan/event cell fresh (no pan
  ever runs; the place is the only triple change), the T3 `phase != 2` pins at
  the KICK edges plus the zeroed ball pair/carrier (the pad-kick seam gates
  out; fixture-proven), and the T4 display-gate freshness with the score step
  noted as the direct writer call (the natural chain is fixture-proven:
  `test_natural_goal_end_to_end` scores 1-0). It is byte-identical to v7 (no
  re-pin; `cmp` clean, 165 lines; M1 unmoved) — the phase-8 T1–T4 landings
  moved no presented frame. Regenerate with
  `./build/test_engine_m2 > port/tests/golden/engine/m2-frames.txt` (the test exits
  non-zero while rewriting the file; re-run `make check` to verify).

## Known gaps

- **Visible-match leg register (M2 visible-match plan close-out, 2026-10-08;
  follow-up-4 update 2026-10-08).**
  Landed: `OL-T11-8` (formation/record placement — indexed draw live, golden
  re-pinned), `OL-84`'s phase-1 entry (begin installs
  `FIFA96_MATCH_RUN_KICKOFF_PHASE = 1`; transcript byte-identical), the
  `OL-84` residual kickoff chain (follow-up-4 T2: the `FUN_0008D098` state-1
  arm + action row 01 + the derived act-1 producer; the begun run reaches the
  live phase 2, transcript byte-identical with the forcing kept per FU-143
  §11.5) and
  `OL-T11-6` (the derived native match palette — FU-144; golden re-pinned for
  the RGB upgrade) and `OL-T11-7` (the match score/clock HUD — FU-148 §10;
  M2 full-gameplay P0.2, golden v5 re-pinned for the HUD upgrade; remaining
  HUD legs `OL-T11-71`…`OL-T11-75`: the nibble colour ramp, the team-name
  stage, extra-time strings/period offset, the settings/suppress gate writers
  and the bar blit's exact span scaler). Carried
  open: the remaining `OL-84` situation-0xB producer (row 02; rows 0x10..0x13
  landed with FU-149 §7 / T1) and row 01's event/camera/ball-stage sinks, plus
  the FU-73 keeper/restart producers
  `0x7546E`/`0x75B58`/`0x76072`; `OL-85` extra-time flag wiring; `OL-87`/
  `OL-88`/`OL-89` goal invokers (**closed by phase-6 S2/S3 + phase-8 T2/T4** —
  FU-142
  §L.10; the tracked-side pick landed in T3 and `FUN_00071DF4`'s first arm +
  call site are wired in T3 (FU-148 §13), leaving the display sink and the
  FUN-00071DF4 table/keeper arm as legs);
  `OL-81`/`OL-83` row-field wrinkles; `OL-82`
  row-08 scan producer; the T1 formation-id producer
  (`[0x14C1E4]`/`[0x14C1E5]`, engine derives id 0) and the camera-mode/angle
  feed the live per-record framing would use (FU-96 legs 1/3; the place itself
  landed in T2); `OL-62`..`OL-71` residuals. The items
  have their detailed entries below / in the FU docs. Follow-up-4 G4 (M2
  playable-match Task 4) closes the plan's whole-range review: G1
  (RGB pitch) and G2 (natural kickoff → phase 2) **carried-with-legs**, G3
  (goal invokers) **carried-with-legs — verified negative**, G4 accepted
  (tape v4 + natural probe + the G4 smoke); every forcing in the tape is
  listed with its leg in the `test_engine_m2.c` v4 provenance.
- **M2 full-gameplay S1 (possession/locomotion producers, FU-147; 2026-10-09).**
  Landed from the frozen FU-147 slice: `fifa96_action_locomotion_track`
  (the BF20 lane block `0x7C776..0x7C7AF`: camera deltas +0x6D/+0x6F and
  `lane = fifa96_entity_distance` = the `0x8DC68` call, first-hand), the pool
  fields `bound` (+0x77) / `cam_dz6f` (+0x6F) / team `camera_nearest` (+0x7C7,
  team-relative index, signed-word replacement at `0x7C7C4`), the `+0x8D`
  active seed (`fifa96_match_entities_init` writes the record ordinal,
  `FUN_0008C2E0 0x8C329`), the per-record driver mover/track
  (`match_run_dispatch_entity` runs the shared mover for every dispatched
  record after its handler, pool walk keeps the `+0x9A` skip), row-04 staging
  of bound/word6f/`is_team_7c7`, the row-1E `+0x9B`/`[0x157A83]` write-back,
  and the row-1E stage-3 possession flip (reset -> `fifa96_match_run_situation`
  0xB -> code-5 install request; the shared table-2 entry, no `_0b`). The
  natural phase-2 pad path is now producer-complete (row 04's slot-dir arm
  gates can run); the reachable engine-level row-05 carrier claim (`0x7F1FF`)
  still waits on OL-63 (row 05 unwired) — the S1 subset is the install ->
  carrier-bit (`0x7DA42`) -> next-dispatch hand-off, fixtured. (**M2 phase-9
  T2 update, 2026-10-10:** row 05 is now wired — the claim writes the pool
  `ball.carrier` and the `ball.pos_*` block on the next dispatch of a code-5
  record; see the phase-9 T2 entry below.) The tape is
  re-pinned (157 hash-only lines 9..165, no `state=` moves; M1 unmoved).
  Carried legs (FU-147 §8/§9): the row-1E stage-flow entry gates (legs 1-3),
  the install `ECX`/invoke flag (leg 5), the reset-lane path (leg 6), the BF20
  visibility arm (leg 7), `[0x157AB2]` (leg 8), the driver pre-pass
  `0x8DA43..0x8DA90`/`FUN_0008D824` (leg 9), the `0x795B4` helper's `0xCD514`
  metric (leg 10; the BF20 block uses `0x8DC68`, an FU-147 errata), the
  E244 formation context (leg 12) and **leg 13 live**: the track's camera
  focus is the engine render-camera stand-in for `0x15774C/0x157754`, so lane
  values are stand-in-derived; the pool `lane` dword (+0x69) also keeps the
  FU-142b dz-word model while the track writes `lane_x` (+0x6B) — row 04
  reads the word, rows 01/26/28/2A read the dword (`>>16`).
- **M2 full-gameplay S2 (goal arming, FU-145; 2026-10-09).** Landed from the
  frozen FU-145 slice: `fifa96_match_goal_zone` (the `FUN_00070074` goal-mouth
  classifier, native widths incl. the z word-truncation), `fifa96_match_goal_arm`
  (the `FUN_0007131C 0x71390..0x713F7` armer: phase 2/0x10, the
  `|camZ|_w > 0xB20 || |camX|_w > 0x730` gate with the native
  word-truncation trap, the frozen `0x15777C` snapshot with y zeroed, the
  classifier — plus the already-armed reflect arm `0x718A9..0x7190E`, which
  mirrors through `fifa96_camera_reflect` and clears the arm when the zone is
  0 and the caller-staged `render.input_bit0` is set), the frame-body wiring
  (armer after the camera update under the native out-of-bounds call gate
  `0x73B70..0x73B9B`; `fifa96_match_run_goal_scan` after
  `fifa96_match_run_phase_drive` = the `FUN_0008AF38` tail `0x8B623..0x8B643`),
  the scanner (`FUN_00088940`: full-32-bit snapshot magnitude vs `0xB20`, zone
  gate, side from the snapshot sign `0x889B6`, the derived
  `FUN_0008DE8C` nearest over the side's team block, then the situation-6
  call) and `fifa96_match_run_goal_queue` (FU-146 §7 item 1, landed here
  because the scanner calls it: the table-1 arm `0x8A9E8` → `situation_id`
  5/6 + `situation_pending` when the live-session gate `session_gate_14c32a`
  is open and nothing is pending; else the `0x8AC28`/`0x8AC88` fallback =
  direct increment + the shared table-2 situation-6 entry). Run state:
  `goal_armed`/`goal_zone`/`goal_snap_x/y/z`/`situation_id`/`situation_pending`
  (`[0x15781D/1E/7C/80/84/15B6A8/15B6C0]`); begin seeds the session gate 1 and
  a phase-2 write (now the shared `match_run_write_phase`) clears the arm
  (`0x740F6`), the init/begin/teardown paths clear it as the `0x84F90`
  restart. **Fixture-proven only until the pan producer lands:** the engine
  camera is static, so a natural run never arms (the S2 risk "dormant chain"
  verified — both goldens are byte-identical, no re-pin); the pan fixture
  drives the FU-71 velocity seam past the bounds and pins arm → snapshot →
  scan → queued id 5, and the second frame pins the no-S3-consumer fallback.
  Carried legs (FU-145 §6): L1 the pan source/camera director — **narrowed by
  T2**: the event setter is complete and the row-04 caller is wired live
  (FU-148 §12), so the producer exists; the other row callers and the
  FUN-00070DE0 boundary arm stay carried — and the armer head
  counter/`FUN_00070DE0`; L2 `FUN_00092998`;
  L3 the possession-selection sinks (nearest substituted); L4 the
  `[0x1587D4]`/`[0x157A4C]` goal-side flag/record (snapshot-sign stand-in);
  L5 the post-goal re-arm (`0x93C87`/`0x9437A`, S3);   L7 `[0x157ACB]` dropped
  (sole read `0x8FCC8` unported); L8 the period-4 extra-time skip unreachable
  with the carried extra_time 0 (OL-85). The throw-in/corner situation arms
  (`0x88BBD`/`0x88C00`) stay the wave-7 B1 set-piece hand-off (landed in
  phase-7 P1, FU-149 — see its entry).
- **M2 full-gameplay S3 (goal consumers, FU-146; 2026-10-09).** Landed from the
  frozen FU-146 slice: `fifa96_match_run_screen_install` (the
  `FUN_00092D8C`/`FUN_00092E2C` installer: leg/mode, step/timer clear, the
  `[0x15B6C0]` latch `0x92DCD`, the score-pair/`[0x15B6A4]`/goal-log-total
  resets, the `0x1110EC[mode*24+leg] × 60` duration seed, then the installed
  handler runs once), `fifa96_match_run_screen_schedule` (the `FUN_000948AC`
  scheduler: the `> [0x15B694]` rollover ids 8/7, the leg-2 `[0x157A97] > 0xF0`
  ids 3/4, the `[0x157754]`/`word[0x1577C2]` id 9, the period-5 id 7, then the
  `[0x15B6D4]` tail call), `fifa96_match_run_screen_step` (the six `0x110F78`
  handlers, table-driven: the per-leg step-kind table and id tables — the
  reviewer's L.4 correction: legs 0/1 map queued id 6 to `goal_no_score`,
  legs 2..5 score it side 1 — the `[0x15882A]` gate, the phase-2 latch clear,
  the post into `fifa96_match_run_score_event` with the lazily-computed probe
  exactly on the native untracked `score[side]==1 && other<3` arm, and the
  `> 0xB4` advance), `fifa96_match_run_screen_advance` (the `FUN_000935A0`
  subset: the score-total-gated goal-log ring append at the new-total slot, the
  totals, hint, and the same-leg re-install) and `fifa96_match_run_goal_probe`
  (the `FUN_000CBC4C` six-limb counter over the image-seeded cells). Run state:
  `screen_leg/mode/step/timer/period_frames/install_hint/actor_age/lead_z`,
  `goal_no_score/last_id/minute/screen_accum`, `goal_log*` and
  `goal_probe_limb` (native `[0x15B680/6BC/6B0/688/694/6C4/157A97/1577C2/
  15B6A0/15B674/15B678/15B68C/15B698-15B6D8/112E68]`). begin installs the
  derived leg 0 / mode 0 / side 0 (FU-146 legs 1/3) and the frame body runs the
  scheduler before `phase_drive`+`goal_scan` (the native `0x4B1A1` before
  `0x4B1A6`). **Both goldens byte-identical, no re-pin** (the static tape camera
  never arms; the screen timer stays under the 900-unit rollover). Carried legs
  (FU-146 §8, restated in the slice's §11): the handler presentation bodies
  (the ±0x720 hint, the `0x10F328`/`0x15B6C8`/`0x158897` copies, the
  `[0x15781D]` re-arm + snapshot — **not applied**: the `0x158897` snapshot
  producer is unported and arming with a zero triple would poison the FU-145
  armer — the situation re-queues 0xC/3/0xA/4/2/1/0, `FUN_000974DC`,
  `FUN_0004C324`), the mode/side/leg front-end arg producers (legs 1/3), the
  `[0x14C32A]` producer (leg 2), the tracked-side flags (**T3 update: the pick
  is ported — FUN_00078824's zeroing + the FUN_00092D8C pick; the flag/mode
  producers stay legs**), the
  `FUN_000CBC4C` cells' live-native verification (leg 5), the `FUN_0009252C`
  display gate (leg 6), the `FUN_000935A0` thresholds/exits (leg 7), the
  `[0x15B684]` mode byte (**T3 update: engine field `screen_record_mode`, BSS
  0; the FUN_00038630 producer is the leg**). The `[0x15B6B8]` side flag is
  write-only (fresh xrefs: its two writes) and is a leg.
- **M2 full-gameplay S4 (presentation completion, FU-148 §2–§4; 2026-10-09).**
  Landed from the frozen FU-148 slice (camera/formation/palette sections):
  - **Camera pose feed** (`fifa96_camera_pose_blocks` /
    `fifa96_camera_pose_apply` / `fifa96_camera_pose_feed`): the three
    image-default `+0x4C` pose arrays (0x107F1C class 3, 0x1080FC class 1,
    0x1082DC class 3; the 0x48 alternate is a leg) plus the mode-0x15 fixed
    record 0x108714, with the FUN_000505D0 arms 1/0x12 (full, incl. the
    selector/class mirrors), 3/4 (FUN_000504E0/FUN_00050518 record selection),
    6/0x10 (record 7 + sub mirror), 8 (records 6/5) and 0x15 (yaw fold/z
    negate). Wired once per granted frame from `render.camera_pose` after the
    FU-71 update; `view_mode == 0` is the unported handler arm, so the tape is
    untouched. The 0x108B80 handler bodies and the 0x51xxx `[0x14E57C]`
    writers stay legs (OL-T11-76/77).
  - **FU-71 event setter** (`fifa96_camera_event_set`): the FUN_00071C94 +
    FUN_00070544 reachable subset — reset, target/height clamp, the
    FUN_000702F8 ramp over the pinned 0x10F4EE table
    (`fifa96_camera_ramp`), the signed fast-path velocity, the
    FUN_0008DC68 bearing (= `speed`), the step products and the anchor A/B
    sets. This is the S2 L1 pan-source closure at the producer level: a
    test-only seed now pans the real integrator past the arming bounds
    (`test_camera_pan_event_chain`: seed -> vel_z 40 -> z 0xB50 -> armer ->
    queued situation 5 -> S3 consumer scores). The natural invoker remains
    absent: the 11 FUN_00071C94 callers are unported gameplay-row bodies and
    the armer's own angle arm requires pre-existing event state
    (`[0x1577EE].hi == 0 && [0x1577BE] == 0` early return). FUN_000709D0 /
    FUN_00070DE0 / FUN_00071DF4, the >0x19 atan walk and the smoothing/
    corner/tracked-player tails stay legs (OL-T11-79). (**M2 phase-9 T3
    update, 2026-10-10:** FUN_000709D0's walk gate producer (FUN_0001C9BC),
    the every-frame FUN_00071DF4 call site + first arm, the row-1E stage-5
    caller and the row-04 tracked bind are now wired — FU-148 §13.1/§13.2;
    the eight remaining callers + the table/keeper arm stay legs.)
  - **Formation id**: run `formation[2]` + `fifa96_match_run_set_formation`
    (FUN_0008EA70), the 0x11033A layout accessor
    (`fifa96_match_formation_layout`) and the 0x14BFC0 `6*id` placement names
    (`fifa96_match_formation_fmt_name`: 352ko/442ko/swko/424ko/433ko.fmt);
    `match_run_formation_seed` now reads the derived id instead of the
    hard-coded 0. The FUN_00011620 team-record producer and the FUN_0007412C
    layout install stay legs (OL-T11-81).
  - **Palette residual** (`fifa96_palette_pool_partition` = FUN_00046F80,
    `fifa96_palette_translate_kit` = FUN_00048DC0, kit tables 0x107287/
    0x10727C, `fifa96_palette_translate_slot` = FUN_000CE980) plus the engine
    seam `fifa96_match_run_translation_install(entity)` over a caller-staged
    `render.palette_pool`. Not wired into the render path: the pool resource
    identity (which loaded file fills `[0x107290]`) stays leg 11/OL-T11-80,
    so the identity remap remains the stand-in. The shade cube has no static
    consumer (re-verified) and is not part of the match contract (OL-T11-82).
  **Both goldens byte-identical, no re-pin** (default `view_mode` 0, no pool
  staged, formation 0 -> the same 352ko.fmt seed). First-hand errata recorded
  in FU-148 §11.5: the image-default pose array is `+0x4C` (not `+0x48`/
  NULL), the pool floor is 0x3000 (not 0x3600), the pose-feed ordering choice
  and the handler-table typos.
- **M2 phase-8 T1 (taker/corner row machines, FU-149 §7; 2026-10-09).**
  Landed from the frozen FU-149 L13 + a fresh first-hand derivation (the slice
  did not freeze the row bodies): `fifa96_match_action_10/11/12/13`
  (throw-in/corner/free-kick/penalty taker machines) with the head gates and
  marker blocks, the full stage cascades with every native gate, the
  record/camera-visible writes, the RNG draws and both resolution exits
  (situation 0xB -> phase 2; row 0x10's `|x| >= 0x720` situation-2 re-queue;
  rows 0x10/0x11 write `[0x157A6A] = 0x12C`, rows 0x12/0x13 never touch it),
  the shared helpers (`match_sp_entity`/`_nearest` with the `+0x9A` self
  stamp, `_anchor`/`_commit`/`_snap`, `_setup_832a8`, `_probe_85498`,
  `_resolve`, `_kick` through the ported `fifa96_ball_kick_target`) and the
  run state `sp_flag_158784`/`sp_delivery`/`sp_157821`. The wired count goes
  15/80 -> **19/80** (dispatch 60 UNSUP / 19 OK / 1 NOTF); the FU-137 §7/§6.1
  tables and FU-149 §7 carry the refreshed counts and the §1.6 erratum
  (full windows; row 0x12's second event table; row 0x13 has no offside
  timer). **Both goldens byte-identical, no re-pin** (the tape never
  dispatches 0x10..0x13: the set-piece arms stay dormant on the never-panning
  camera; v8 pins the taker mask absent and the `sp_*` cells fresh). Fixtures:
  `test_action_10..13_runs_*_body`, `test_taker_armed_rows_resolve`
  (P1 set-piece arm: throw-in BX=1 fallback -> phase 3 + 0x10; corner ->
  phase 4 + 0x11; both resolve) and `test_taker_armed_referee_rows_resolve`
  (FU-150 contact chain -> phase 7 + 0x12 / phase 6 + 0x13; both resolve).
  Carried legs (FU-149 §7.3, L13.1..L13.7): L13.1 the
  `FUN_000832A8`/`FUN_00083164`/`FUN_0008DB6C` throw setup; L13.2 the
  input-driven `FUN_00083428` throw/claim machine (replaced by the no-slot
  timer+draw completion so a slotted live taker still resolves); L13.3 the
  kick event sub-table arms and their vector sources (collapsed to one derived
  delivery per kick stage — the real `fifa96_ball_kick_target` runs, the exact
  arm vector/RNG cadence diverge); L13.4 the presentation sinks
  (`0x974DC(0x1E)`, the `0x8F188` ids); L13.5 the `0x8DE8C` pick origins;
  L13.6 the state producers (`[0x157821]`/`[0x157A6A]`); L13.7 the penalty
  remainder (`0x84D5B` free-record hand-off, `0x14C114` input words, slot
  gate). Hardening divergences documented: the `0x10F334/0x10F33C` index mask
  to 0x1F; the reset path's forced-decision install is the existing shared
  `match_row_reset`.
- **M2 phase-8 T2 (camera live feed / pan origin, FU-148 §12; 2026-10-09).**
  Landed the producer chain OL-T11-79 asked for:
  - **Event setter complete** (`fifa96_camera_event_set`): the `[0x157A6C]`
    bail gate, the FUN_00070544 ramp sign param, the corrected timer cell
    (`[0x1577FA] = F6`, not F8 — S4 errata), the fast/slow path selection and
    the `> 0x19` atan walk (`fifa96_entity_angle`/`sine` + the FUN_000795A4
    products), plus the cursor/acc/rate tails.
  - **Pan producers**: `fifa96_camera_pan_step` (FUN_000709D0: band, the
    `[0x157821]` counter, the height decay, the rng random walk, the
    FUN_00070544(0) re-arm) is called in `fifa96_camera_update` at the native
    `0x737da` site when `timer > timer_limit`; `fifa96_camera_reposition`
    (FUN_00070DE0 derived core) and `fifa96_camera_rate_table` (FUN_00071DF4
    table/keeper arm) are ported loader functions.
  - **Live row caller**: `fifa96_match_action_04` consumes the already-ported
    row-04 event outputs (`out.events` -> event set + the arm-A track-reload
    tail). This is the first reachable gameplay-row pan origin;
    `test_row04_live_pan_arms_camera` drives it through the real frame
    dispatch (no fixture poke) -> armer -> situation 5 -> S3 score.
  - **Tape dormant, no re-pin**: M1/M2 `cmp` byte-identical. Reason: on the
    acceptance tape the code-4 rows dispatch with lane ~1460..1592 (probe:
    `lane > 0x90`, `timer81 + lane > 0x40`), so the half-line event arm never
    fires; the camera stays static and no presented frame changes. The
    natural path is producer-real in the fixture instead.
  - Carried (OL-T11-79, FU-148 §12): the `> 0x70` anchor branch,
    FUN_000703E8, the 0x15780C/0E smoothing words, the sound sinks
    (FUN_00065CF8/FUN_000974DC/FUN_000651F0/FUN_000974F0), the tracked-player
    tail (0x1577CA/CE + table 0x10E169), FUN_00071DF4's 0x11042B/0x11042C
    lookup, the reposition boundary arm and the other nine FUN_00071C94
    callers. (**M2 phase-9 T3 update:** row 04 + row 1E are wired, so eight
    body callers remain — FU-148 §13.4; the auto-camera call site + first arm
    landed, the table/keeper arm stays.)
- **M2 phase-8 T3 (input hold policy + pad kick, OL-T4-1/FU-75; 2026-10-09).**
  Landed:
  - **Hold policy (OL-T4-1 closed).** `platform_sdl3.c` keeps the held-key set
    from the SDL press/release edges and re-presents every held mapped key as
    a state-1 sample on each poll (auto-repeat stays filtered). This matches
    the engine's per-poll `input_state` model and the native make/break flag
    array; the alternative (OS auto-repeat passthrough) was rejected because
    the repeat delay would still pulse the hold and the match layer's
    absence-as-release contract (`fifa96_input_update`) cannot carry a hold
    across polls. Consequence: a physically held key now repeats in the
    front-end too (the front-end consumer takes every state-1 entry; native
    repeat cadence unported — leg); the null tapes are unaffected.
  - **Pad kick (row 07 via the code-1 input row).** First-hand derivation:
    the `FUN_0007CA54` input-row dispatch selects code 1 when
    `byte[rec+0x91] == 5` (`0x7CB36`) and the code-1 released row runs
    `0x7D110` (`0x7D13B MOV ECX,1` / `0x7D140 MOV EDX,7`), installing action
    7 — the ported kick machine — for a KICK/PASS release on a carrier. The
    seam is wired in `fifa96_match_run.c` for slot-bearing outfield records
    (records 1..10; record 0 is the keeper machine's own tables) with the
    reachable handler subset `0x7CE38/0x7CEB0/0x7CF20/0x7CF54/0x7CFD0/
    0x7D010/0x7D054/0x7D08C/0x7D110/0x7D174/0x7D0C4` (gates first-hand);
    an `ECX=1` install re-dispatches the new row in the same frame and the
    machine tail dispatches it again (the native invoke + `0x7CD29
    CALL [rec+0x18]` pair). `match_kick_from_record` now stages
    `slot_word6` from the live FU-70 released word (the native stage-1 mode
    source). `test_engine_match_frame::test_pad_kick_release_runs_kick_row`
    proves carrier → release → code 7 → row 07 → ball-pair actor/flags
    (mode 0x10)/event-row staging; `test_engine_sdl3::
    test_held_key_is_a_state_sample` proves the hold sample + movement.
  - **Tape dormant, no re-pin**: M1/M2 `cmp` byte-identical (the acceptance
    tape's KICK edges land at phase 0x13/1, gated out by the handlers'
    `phase == 2`; the held RIGHT selects no pressed row for the staged
    codes).
  - Legs (owned by the follow-up waves): **L4.1** the machine
    `out.forced`/`out.chase` application (`FUN_0007C990`/the code-8 gate,
    FU-75 §1.5/§1.6 — the live movement blocker: the slot record's code 02
    would be re-selected by this arm), **L4.2** the no-edge arm
    (FU-75 §1.7), **L4.3** the `0x7E600` decision call inside `0x7D0C4`,
    **L4.4** the `0x7CD60` `+0x7CB` arm body, **L4.5** the `0x7D1D4`
    control-selection switch, **L4.6** row 02 `locomotion_restart_target`
    (FU-138 OL-18) and the keeper-machine input tables (`FUN_000782D0`,
    FU-74 §2) — with those, the live smoke pad movement becomes reachable.
    The front-end held-key repeat cadence is a leg under the hold policy.
    (**M2 phase-9 T1 update:** L4.1/L4.2/L4.6 landed — row 02 ported+wired
    and the forced/chase/no-edge outputs applied in the seam, so the live pad
    movement is reached (see the phase-9 T1 entry); L4.1 residual = the
    unbound-record machine walk, L4.6 residual = the row-02 stage-1/2 arms +
    the keeper tables, L4.3–L4.5 stay legs.)
- **M2 phase-8 T4 (natural goal chain close-out, OL-87/88/89; 2026-10-09).**
  Landed:
  - **Natural goal end-to-end (gate T4: score from replicated gameplay).**
    `test_engine_match_frame::test_natural_goal_end_to_end` plays the natural
    kickoff (the 0x13 countdown + the T3 KICK press; no forced phase or
    `[0x5882A]`) into live phase 2, fires the live action-04 ground-ball
    sub-object arm (native `0x7F035`: the slot dir bytes with ball height 0,
    no ball staging) and asserts each link: the row event -> the pan
    integrator -> the armer (zone 1) -> the clock-tail scanner situation 6 ->
    the queue id 5 (the `[0x14C32A] != 0 && [0x15B6C0] == 0` condition) ->
    the scheduler -> the leg-0 handler post -> score 1-0 (tracked side -1, no
    post id). `test_natural_goal_fallback_arm` covers the queue-condition
    closed arm (gate 0 at scan time -> the FU-72 direct increment + the
    table-2 phase-5 write, no id queued).
  - **OL-89 display boundary ported (the last named unported piece).**
    First-hand `FUN_0009252C` -> `FUN_000A80E2` (`[0x115FCC]==0 -> -1`;
    `[0x114A98]!=0 -> 1`; else 0); every posting arm of the writer runs the
    shared tail (`0x93B73 CALL 0x9252C`), so `fifa96_action_score_event` now
    returns `out.dispatched` through `fifa96_score_display_gate` and the run
    carries the gate cells (`score_sound_device`/`score_sound_midi`, image
    0/0) plus the observation `score_display_event` (0 = none). The
    `FUN_00066724(id, 0)` text/audio chain stays a leg; at the image defaults
    the gate is -1 and nothing dispatches (goldens byte-identical).
  - **Re-verification (fresh windows in FU-142 §L.10):** the 11 `FUN_00093944`
    call sites, the `0x8A944..0x8A96B` queue condition + the `0x8A8E0` id
    table, the `0x88B44` situation-6 arm of `FUN_00088940`, the `FUN_000CBC4C`
    probe, and the `0x73B6B` `FUN_00071DF4` call site inside `FUN_000736AC`.
  - **Tape dormant, no re-pin**: M1 `09b726b7…` / M2 `2e709151…` byte-identical
    (no tape record reaches the half-line band and no pan producer fires); the
    m62 freshness block now also pins the fresh display cells.
  - **Carried legs** (FU-142 §L.10): the
    `FUN_000A7FD4` display-gate cell producers (the gate reads
    `[0x115FCC]`/`[0x114A98]`, image 0/0; the `FUN_000A8172` clear is the
    other writer), `FUN_00071DF4`'s table/keeper second arm
    (T3 update: the tracked-side pick, the call site + first arm are now
    wired — FU-148 §13.2/§13.3), the `FUN_00066724` sink and the handler
    presentation bodies.
- **M2 phase-8 close-out (T5 acceptance; 2026-10-10).** Whole-range summary:
  the phase-8 plan `2026-10-09-fifa96-m2-phase8-live-loop.md` closed the live
  gameplay loop's four named gaps — T1 the taker/corner row machines (L13;
  wired 19/80), T2 the camera live feed / pan origin (OL-T11-79; the first
  reachable gameplay-row pan producer), T3 the input hold policy + pad kick
  (OL-T4-1 + the `FUN_0007CA54` seam) and T4 the natural goal chain
  (OL-87/88/89 closed producer-to-writer) — serialized under subagent-driven
  development, each reviewed (T1 one fix round, T2 one fix round, T3 approved
  clean, T4 approved clean), then T5 (this acceptance: tape v8, the smoke
  above, these docs). Gate status against the plan: **Gate T1** taker/corner
  rows execute (armed fixtures) and L13 narrows with evidence — pass after
  the fix round (row-0x12 timer/relay pick, counts/docs); **Gate T2** the
  natural pan path can arm the goal chain — pass after the fix round (idle
  rate 24, arm-A tail, event gate, jitter centering; live row-04 fixture);
  **Gate T3** pad moves the record and kicks the ball in a headless fixture
  (hold proven at BASE discrimination; kick row 07 via the seam) — pass,
  with the on-screen movement clause **not met, carried on L4.1/L4.6**; **Gate
  T4** score arises from replicated gameplay — pass
  (`test_natural_goal_end_to_end` scores 1-0 on the natural kickoff; the
  tape's direct call stays the honest dormant-tape form); **Gate T5** tape v8
  green, smoke honest, whole-plan review material landed — pass. Every task's
  `make check` was green in sequence (108/108 throughout; the suite did not
  grow in phase 8 — the tasks extended existing fixtures), with the engine
  suites under ASan/UBSan.
  **Re-pin ledger (phase-8): no re-pin at any task.** M1 immovable
  (`09b726b7…`, unchanged since M1); M2 byte-identical through T1–T5
  (`2e709151…`, the S1 v5.1 re-pin lineage — each task's `cmp` evidence; the
  T5 acceptance re-ran `./build/test_engine_m2` with the ISO present and
  `cmp` clean, 165 lines). v8 adds assertions only. One acceptance-time
  correction to earlier prose: the tape camera does change triple once — the
  row-1E keeper claim place (FU-151, part of the S1-lineage golden) resets it
  to the focus triple at the mechanics window; "the camera never moves"
  meant "never pans", and v8 states it precisely (T5 re-verified the place is
  golden-material: skipping the drain changes frames 49..165).
  **Leg register carried out of phase 8** (each task's entry above and the FU
  docs hold the detail): **T1/FU-149 §7.3** L13.1–L13.7 (throw setup; the
  `FUN_00083428` input machine; kick event arms/vectors; presentation sinks;
  pick origins; state producers; penalty remainder); **T2/OL-T11-79 residue**
  the nine carried `FUN_00071C94` callers, the `FUN_00070DE0` reposition
  bit-8 boundary arm, the `walk_gate` frame path, the sound sinks, the
  `FUN_00071DF4` 0x11042B/C lookup and the tracked-player tail (**T3 update:
  the row-1E caller, the walk gate + FUN-0001C9BC words, the FUN-00071DF4
  call site + first arm and the row-04 tracked bind landed — FU-148 §13; the
  eight body remainder callers, the reposition bit-8 arm, the sound sinks and
  the 0x11042B/C lookup stay**); **T3/FU-75**
  L4.1 the forced-decision/chase application, L4.2 the no-edge arm, L4.3
  `0x7E600`, L4.4 `0x7CD60`, L4.5 the `0x7D1D4` switch, L4.6 row 02 +
  keeper input tables, plus the front-end repeat cadence; **T4/OL-87/88/89
  residual** the tracked-side pick (**T3 update: landed**), the
  `FUN_000A7FD4` producers,
  `FUN_00071DF4` (**T3 update: call site + first arm landed; the
  table/keeper arm stays**) and the `FUN_00066724` sink. Deferred minors rolled up for
  the final review: T1's FU-137 §7 errata parenthetical arithmetic (fixed at
  T5); T2's review minors (all fixed in the fix round; the census is nine
  carried callers); T3's three comment minors (L4.5 stub wording, L4.2
  flags-inert note, L4.1 `+0x99` gate note — fixed at T5); T4's two doc
  minors (the ENGINE.md phase-6 S2/S3 attribution and the FU-146 leg-5
  no-native-run qualifier — fixed at T5). No open phase-8 minor blocks any
  gate; the triage list is the final-review input.
- **M2 phase-9 T1 (on-screen movement — FU-75 L4.1/L4.2/L4.6; 2026-10-10).**
  Landed:
  - **Row 02 ported + wired (L4.6, FU-138 OL-18).** First-hand
    `disassemble_bytes 0x7DFCC..0x7E1A2` on /FIFA96.EXE; the body is ported as
    `fifa96_match_action_02`: phase 1 mirrors `-dword[[team+0x7B2]+0x59]` into
    `+0x4D` (`+0x55`/`+0x89`/`+0x92` zeroed; y untouched); phase 2 writes
    `[team+0x7B2] = rec`, requests the `FUN_0007876C` merge when slotless with
    `byte[team+0x828] != 0`, installs `4` invoke-now when slot-bound (direct
    return), else writes the `0x15774C` camera target + `+0x89 += delta` and
    runs the `restart_wait` stage-0 gate (`lane > 0x40 ? 0x78 : 0xA`, reset
    first on the `lane > 0x40` ready path); any other phase resets. The
    stage-1/2 arms and the keeper input tables remain the leg.
  - **L4.1 forced-decision/chase and L4.2 no-edge arm applied.** The T3 seam
    (`match_run_outfield_input`) now applies `out.forced`/`out.chase` through
    the pool installer (no invoke, native `0x7CA48`/`0x7CD24`), copies the
    camera triple and runs the `0x79B58` receiver timer on
    `out.no_edge_arm` (`0x7CC70`), and builds the chase state to the native
    fields (`camera = [0x157750]` ball height, the side compare, `+0x5D`) plus
    the live `slot[+0x10] = prev_mapped`; the seam re-stages
    `+0x91/+0x92/+0x89/+0x9E` so the same frame's tail dispatch runs the
    installed row. Residual: the native machine walks the forced/chase tail for
    every outfield record while this seam keeps the slot-record scope, so the
    chase arm's `[rec+0x20] == 0` remains unreachable on the live path (the
    unbound-record walk leg).
  - **Live movement.** ISO headless: the post-kickoff slot record reaches code
    4 and the mover integrates the slot-dir target
    (`test_held_key_moves_live_controlled_record`: held RIGHT moves z and
    ramps `vel75`; the no-input control stays still while still reaching code
    4). The ISO smoke (this host) showed on-screen movement with held LEFT
    (scene AE 45710) / UP (AE 28535); RIGHT walks away from the stand-in camera
    (AE 0 while positions move) — the follow-cam is T3. See the "Interactive
    smoke" section for the shots.
  - **Tests:** `test_engine_match_handlers::test_action_02_restart_and_phase2_arms`
    (phase-1 mirror, no-slot camera/timer/stage wait, the `lane > 0x40` reset
    path, the phase-2 install-4 invoke and the reset), the `action_expect[2] =
    FIFA96_OK` flip, `test_machine_forced_decision_installs_on_slot_record`,
    `test_machine_no_edge_arm_copies_camera_target`,
    `test_held_key_moves_live_controlled_record` (ISO-gated), and the
    row-02-reactive fixture updates (`test_kickoff_enters_phase2_when_cutscene_isolated`
    now pins the slot handoff onto record 2; the pad-kick and natural-goal
    fixtures stage the live role state the applied forced decision requires).
  - **Goldens: no re-pin.** M1 byte-identical (`cmp` clean, M1 immovable); M2
    byte-identical (`cmp` clean, 165 lines) — the tape's code-2 record only
    exercises row 02's `phase != 2` reset inside the forced 0x13 window, so no
    presented frame moves; the tape's observed dispatch set grows to 15 rows
    (`M2_WIRED_MASK` gains row 02, `mask_kick` gains it too).
  - **Legs:** L4.3 `0x7E600`, L4.4 `0x7CD60`, L4.5 `0x7D1D4` remain; L4.6
    residual = row-02 stage-1/2 arms + the keeper input tables (FU-138 OL-18
    as narrowed, FU-75 §11); L4.1 residual = the unbound-record machine walk
    (chase reachability); the front-end repeat cadence stays a hold-policy leg.
- **M2 phase-9 T2 (live carrier producers — FU-142 OL-63; 2026-10-10).**
  Landed:
  - **Row 05 ported + wired (OL-63 narrowed).** `fifa96_match_action_05`
    binds the ported `fifa96_action_carrier_arm` (`0x7F194..0x7F665`
    stages 0-3) over `mr->record`/the FU-141 pool and applies the bounded
    record-visible effects: the `0x7F1FF` `[0x158724]` claim — the pool
    `ball.carrier` — with the derived `0x158728..0x15872F` block
    (`ball.pos_index/rotation/dir_x/dir_z/counter_c/release/counter_e/
    counter_f`) reset on a new carrier, the `[team+0x7B2] = rec`/`+0x7B6 = 0`
    bind, the capped/additive `+0x89`, the camera triple copy to
    `+0x4D/+0x51/+0x55`, the stage-0 `lane > 0x40` clear-control /
    `[0x157A83] = rec` set-control and the `0x15872A/B` dir writes
    (`0x7F386`/`0x7F397`), the `0x7876C` merge request (`helper_request`), the
    `0x79B1C` snap, the `0x79C50` face (`+0x8E`) and the stage-3 team-target
    hand-off (install `4` on the staged ball actor `[0x158730]` + the
    `0x79B58` receiver timer on `[0x158734]`). The stage-0 target algebra
    `0x7F3A1..0x7F57B` and the `FUN_0007F7E0` fallback remain named OL-63
    requests (`out.tail`/`out.fallback`) with no derived consumer; the
    `0x92820`/`0x6E598` sinks stay OL-27/OL-52.
  - **Possession block producers.** `fifa96_match_entities_update` now decays
    the `0x15872D` release countdown (native `0x4B163..0x4B17B`, signed byte
    minus the zero-extended delta) and the row-06 pursuit staging reads the
    live `0x15872A`/`0x15872F` bytes (`adjust_x`/`byte_15872f`), closing the
    producer half of OL-69's row-05 note. The keeper `+0x9B`/`[0x157A83]`
    flip (FU-147 §4.2) was already landed by FU-147 S1/row 1E and is
    unchanged.
  - **Live fixture (ISO-gated).** `test_engine_match_frame::
    test_live_carrier_claim_and_kick`: natural kickoff -> held UP movement ->
    row 04's half-line coda installs `5` on the real dispatch path -> the
    next frame's row 05 claims (`ball.carrier`/`[0x157A83]` = the slot
    record, dir bytes = the slot dirs) -> pad KICK release -> row `07` ->
    the staged ball pair (`pair.actor = the carrier`, `pair.traj != 0`).
    Discriminating vs BASE (rehearsed with row 05 forced to `-UNSUPPORTED`:
    the claim assert fails).
    `test_engine_match_entities::test_possession_release_decay` pins the
    `[0x15872D]` integrator.
  - **Tests:** `test_engine_match_handlers::test_action_05_wired_carrier` /
    `test_action_05_claim_dirs_and_stages` (claim/block reset/dirs/control/
    stages/hand-off) and the `action_expect[0x05] = FIFA96_OK` flip.
  - **Goldens: no re-pin.** M1 byte-identical (`cmp` clean, M1 immovable); M2
    byte-identical (`cmp` clean, 165 lines) — the tape never stages code `5`
    (its records stay on the staged/arm codes), so the newly wired row never
    dispatches and `M2_WIRED_MASK` stays 15 rows. The wired count moves
    20 -> 21/80 (dispatch 58 UNSUP / 21 OK / 1 NOTF, FU-137 §6.1/§7).
  - **Legs:** OL-63 residual = the stage-0 target algebra + the
    `FUN_0007F7E0` fallback; the FU-147 §9 legs are unchanged; the OL-69
    record-byte/lead/callback/swap residue stays.
- **M2 phase-9 T3 (pan origin, auto-camera, tracked-side — OL-T11-79/OL-87;
  2026-10-10).** Landed from the phase-9 plan's Task 3 (first-hand
  `/FIFA96.EXE`; full derivation in FU-148 §13):
  - **Range words + walk gate (FU-148 §13.1).** `fifa96_input_range_words`
    ports FUN_0001C9BC (the `[0x105278]`-family config cells + the
    `[0x15753C]` entry gate) and the run builds the power-on-zero
    `[0x14C1D4]`/`[0x14C1D6]` words in `begin` (the native FUN_00011B7C call);
    the frame path derives `(w0|w1) & 4` as the FUN-000709D0 pan-step walk
    gate, threaded through the new `fifa96_camera_update_walk`.
  - **Every-frame auto-camera (FU-148 §13.2).** The FUN_000736AC
    `0x739C6..0x73B6B` call site is ported as
    `fifa96_match_run_camera_follow` (pan counter 0, the `[0x1577BE]` bearing
    nonzero at the gate point (`follow_speed`), `[0x1577EE].hi > 0x10`, both
    rate bytes zero, `[0x1577CA]` + slot) and runs the existing
    `fifa96_camera_rate_event` first arm (slot dirs x0xF clamped ±15 when the
    event height exceeds 0xF0). The camera carries `tracked` ([0x1577CA];
    `fifa96_camera_set_tracked` = the 0x71D27 write, cleared by the 0x700F4
    reset) and row 04 binds its event record. The class gate
    (`[[rec]+4][0] == 0x18D8`) reduces to the pool-record identity (the only
    two `[0x1577CA]` writers store pool records or clear). The table/keeper
    second arm stays an OL-T11-79 leg.
  - **Row 1E stage-5 pan (0x75DC4).** `match_keeper_claim_apply` consumes the
    stage-5 `released`/`scenario` pair: the camera cuts to the machine-staged
    release triple, `fifa96_camera_event_set(camera, 0, 0, 0x50, 0)` posts the
    event and the releasing keeper becomes the tracked record
    (`test_action_1E_machine_release_chain`).
  - **Tracked-side pick (OL-87 residual, FU-148 §13.3).**
    `fifa96_match_run_screen_install` now ports FUN_00078824's flag zeroing +
    the FUN_00092D8C pick: the flags-zero image default yields
    `score_tracked_side = 1`, so the natural goal's untracked arm posts the
    probe id. `test_natural_goal_end_to_end` now asserts `score_last_event ==
    0xD3` (the probe's first fold low byte 0xED, bits 0/1 set; the display
    gate stays closed at the image sound cells).
  - **Remaining callers (§13.4).** Row 1E wired; the row-05 tail (0x7F4E7,
    the OL-63 tail algebra), the armer angle arm (0x71B8A), the reposition
    sub-body (0x70DD1), the ball staging (0x7A219), `FUN_0006E8E8` (0x6FA62)
    and rows 1B/1C/0D (0x77423/0x77DC6/0x82A6C) stay numbered legs.
  - **Tests:** `test_input::test_range_words_builder`, `test_camera::
    test_update_walk_gate`/`test_tracked_record`, `test_engine_match_frame::
    test_tracked_side_pick`/`test_auto_camera_follows_tracked_slot` (+ the
    natural-goal post id and the row-04 tracked bind). `make check` 108/108.
  - **Goldens: no re-pin.** M1 `cmp` clean (immovable); M2 byte-identical
    (165 lines) — the tape's code-4 rows still carry lane ~1460+ (the
    half-line band gate fails), no row-1E record reaches stage 5, the range
    words are zero and no event sets `camera.tracked`, so no frame moved. Not
    the expected first re-pin: no intended upgrade reached the tape.
  - **Smoke:** the T1 UP hold reproduces scene AE 28535 (`p9-t3-hold-up.png`;
    LEFT 45902, `p9-t3-hold-left.png`); the auto-camera's high-ball producer
    (height > 0xF0 with a zero-rate camera) is not reachable from the ported
    rows, so the visible follow-cam stays the ball-staging leg.
- **M2 phase-9 close-out (T4 acceptance; 2026-10-10).** Whole-range summary:
  the phase-9 plan `2026-10-09-fifa96-m2-phase9-live-pad-loop.md` made the
  live surface playable: T1 on-screen movement (FU-75 L4.1/L4.2/L4.6 — row 02
  ported+wired, the forced-decision/no-edge outputs applied in the
  `match_run_outfield_input` seam), T2 live carrier producers (FU-142 OL-63 —
  row 05 wired, the claim/team-target/control/camera/dirs/stages/hand-off over
  the pool plus the possession-decay producer), T3 pan origin/auto-camera/
  tracked-side (FU-148 §13 — the FUN-0001C9BC range words + walk gate, the
  every-frame FUN-00071DF4 call site + first arm, the row-1E stage-5 caller,
  the row-04 tracked bind, the FUN-00092D8C pick) — serialized under
  subagent-driven development, each reviewed with a bounded fix loop (T1 one
  round, T2 one round docs-only, T3 one round docs-only), then T4 (this
  acceptance: tape v9, the smoke above, these docs). Gate status against the
  plan: **Gate T1** headless held-key movement + on-screen smoke movement —
  pass (ISO `test_held_key_moves_live_controlled_record`; v9 pins the row-02
  reset dispatch inside the forced kickoff window; smoke LEFT AE 45902 / UP
  28535); **Gate T2** the kick has a live carrier, driven end-to-end
  headlessly — pass (`test_live_carrier_claim_and_kick`; the on-screen ball
  view stays blocked on the ball-staging/render legs); **Gate T3** the camera
  pans naturally — pass at the row-event level
  (`test_row04_live_pan_arms_camera` + the natural end-to-end goal's 0xD3
  post) with the visible follow-cam carried on the ball-staging high-ball leg;
  **Gate T4** tape v9 green, smoke honest, whole-plan review material landed —
  pass. Every task's `make check` was green in sequence (108/108 throughout;
  the suite did not grow — the tasks extended existing fixtures), engine
  suites under ASan/UBSan.
  **Re-pin ledger (phase-9): no re-pin at any task.** M1 immovable
  (`09b726b7…`); M2 byte-identical through T1–T4 (`2e709151…`, the S1 v5.1
  re-pin lineage). The plan expected T3 to produce the first re-pin since S1
  (live pan/auto-camera and/or a natural tape goal); it did not land — no
  intended upgrade reached the tape (T3's no-re-pin reason: the m 41-staged
  code-4 rows carry lane ~1460+ so the half-line exit `0x7EDB2` skips the
  event, no row-1E record reaches stage 5, the range words are zero, no
  tracked bind fires, and the direct score step passes probe 0). T4 re-ran
  `./build/test_engine_m2` with the ISO present: `cmp` clean, 165 lines; the
  T4 smoke captures are byte-identical to the T3/T5 shots, so no landing moved
  a live pixel. **Leg register carried out of phase 9** (each task's entry
  above and the FU docs hold the detail): **T1/FU-75** — L4.1 residual (the
  native machine walks the forced/chase tail for every outfield record while
  the seam keeps the slot-record scope: the unbound-record walk), L4.2
  applied, L4.3 `0x7E600`, L4.4 `0x7CD60`, L4.5 `0x7D1D4`, L4.6 residual
  (row-02 stage-1/2 arms + the keeper input tables `FUN_000782D0`, FU-74 §2)
  and the front-end repeat cadence; **T2/FU-142 OL-63** — the stage-0 target
  algebra `0x7F3A1..0x7F57B` and the `FUN_0007F7E0` fallback (the row-05
  producers live; the FU-147 §9 legs and the OL-69 residue unchanged, with
  the row-06 row byte live since OL-80); **T3/OL-T11-79** — the eight
  remaining `FUN_00071C94` callers (§13.4: row-05 tail, armer angle arm,
  reposition sub-body, ball staging, `FUN_0006E8E8`, rows 1B/1C/0D), the
  `FUN_00070DE0` reposition bit-8 boundary arm, the tracked-player tail, the
  `FUN_00071DF4` table/keeper second arm, the `[[rec]+4][0] == 0x18D8`
  record-class gate as the pool-record identity equivalence, the
  `0x105278`-family config-cell producers (the range words and the
  `(w0|w1) & 4` walk gate stay zero) and the ball-staging high-ball producer
  (`FUN_0007A490`/`FUN_0007A084`) that gates the visible follow-cam;
  **OL-87** — the tracked-side pick closed (default 1, the natural goal posts
  0xD3), the residual presentation/display sinks stay (FU-146 §8 handler
  bodies, `FUN_00066724`, `FUN_000A7FD4` gate cells). Deferred minors rolled
  up for the final review: T1's unbound-record walk note (L4.1 residual);
  T2's FU-139 §11.7 row-byte observation (fixed at T4: the row byte is live
  since OL-80); T3's `follow_speed` capture-point re-check and the LEFT
  45710→45902 within-run spread note (documented, not a defect). No open
  phase-9 minor blocks any gate; the triage list is the final-review input.
- **M2 phase-10 T1 (ball-staging producers — visible follow-cam + ball;
  FU-148 §14; 2026-10-10).** The phase-9 T3 leg is closed: `FUN_0007A084`'s
  receive arm is derived first-hand (the 269-insn body, the `FUN_0008DE8C`
  scan, the `FUN_00071C94` seed mapping) and ported as
  `fifa96_match_run_ball_receive`, wired from `match_kick_run` whenever the
  ported staging tail reports `receive` (rows 07/0F and the shared set-piece
  deliveries). Live effects: the camera event with the staged trajectory as
  the event height plus the tracked actor bind (the high-ball follow
  producer), the controlled-actor release, the staged-event ack, the
  own-team receiver scan (the `+0x8E`/`+0x91` skip rule) and the
  receiver/team-target/opponent-clear writes, and the carrier release
  countdown. Chain gate: `test_ball_receive_arm_high_ball_chain` (staging ->
  event > 0x10 -> follow arm > 0xF0), the low-ball discriminating twin, the
  skip/carrier fixtures, `test_live_pass_kick_runs_receive_arm` (ISO: the
  live carrier -> PASS release -> row 07 carry row code 13 -> the receive
  fields live), and `test_ball_record_draws_ball_slot` (the FU-85 §4
  staging + FU-89/85 renderer draw the ball record's slot; pixel assert).
  **Goldens: no re-pin** (M1/M2 byte-identical; the tape never stages a
  receive-coded kick). **Smoke (honest): no visible follow-cam and no ball
  sprite yet** — the wired KICK/PASS rows stage traj 205/0 (< 0xF0) and the
  ball record's open-play position producer is unported. Legs carried:
  L14.1–L14.8 in FU-148 §14 (the `0x18E2` class rebuild, the sound sink, the
  timer81 loop, the `+0x7E7` cell, the `FUN_00079D5C` foul body, the `0x40`
  long-ball key, the `0x157750` ball-height producer, and the code-`16`
  ball-record machine). `make check` 108/108; ASan/UBSan clean.
  **Phase-11 T1 update (2026-10-11):** the `0x40` long-ball key and the
  `0x157750` camera-height producer are landed (FU-148 §15; the visible live
  follow awaits the band-boundary bootstrap — leg 15.a); **phase-11 T2
  (2026-10-11):** the `FUN_0008BAF0` ball machine (L14.8) is landed — the
  ball record is produced, staged (row 3) and drawn live; the on-screen ball
  waits on the camera place (leg L14.8.e), and the ball height `y` stays 0
  (15.a/15.b unchanged; FU-148 §16).
- **M2 phase-10 T2 (live-path carry-ons — unbound walk, config producers,
  pan caller; FU-75 §12, FU-148 §13.1/§13.4, FU-147 §3.3 leg 7;
  2026-10-10).** Three carry-ons from the phase-9/10 ledgers:
  - **L4.1 unbound-record walk (FU-75 §12).** Fresh
    `0x8DB28..0x8DB67`/`0x7CC80..0x7CD33`: `FUN_0008D8EC` calls `FUN_0007CA54`
    for every outfield record 1..10, and the phase-2 forced/chase tail (step
    5, `FUN_0007C990` + the code-8 gate) runs before the same frame's
    handler dispatch (step 6). `match_run_dispatch_entity` now runs the ported
    machine for every outfield record, so unbound records install codes
    3/4/6 and chase 8 through the pool installer and the same-frame
    re-dispatch runs rows 04/06/08. Tests:
    `test_unbound_team_target_forced_installs_row04`,
    `test_unbound_chase_installs_and_runs_row08` (discriminating vs the
    pre-T2 slot-only scope).
  - **`0x105278` config-cell producers (FU-148 §13.1).** The "no static
    writer" census resolved to the 13-dword front-end options block
    `0x105274..0x1052A4`: editor FUN_0001C728 (increment + wrap at the
    `0x105108` stride-`0x1C` max table `{4,3,2,3,5,2,5,2,3,5,2,5,2}`),
    query FUN_0001CAEC. Ported `fifa96_input_option_max`/`_step`/
    `fifa96_input_options_to_cells` and the engine producer
    `fifa96_match_run_set_input_options` (eight cells -> range words). The
    derived consumers are now real: `render.input_bit0` (reflect, the FU-145
    follow-up) and `render.input_bit2` (the `0x73A07..0x73A18` interpolation
    bit), with the `range & 4` pan walk gate already threaded. Tests:
    `test_input::test_option_producer_*`,
    `test_input_options_producer_sets_range_bits`.
  - **Remaining `FUN_00071C94` callers (FU-148 §13.4/§13.2).** Fresh
    `get_xrefs_to 0x71C94` = 11 (same set). One wired: `0x70DD1`
    (`FUN_00070C08`) with its previously undecomposed caller gate
    (`FUN_0007BF20` tail `0x7C7D3..0x7C8FC`, FU-147 §3.3 leg 7): ported as
    `fifa96_camera_record_event` + `match_run_record_camera_event`, run after
    the BF20 lane block; it rebinds `[0x1577CA]` like FUN_00071C94 (`0x71D27`)
    and reports the `0x92820(rec,0x1C)` sink (OL-27 leg). Dormant until the
    `0x157750` ball-height producer (L14.7); fixture-pinned
    (`test_record_event_tail_gate_fires_in_frame`, `test_camera::
    test_record_event_*`). **Phase-11 T1 update (L14.7 landed):** the gate
    reads the live camera Y and the producer wakes it
    (`test_record_event_gate_live_from_height_producer`); the pan caller is
    live whenever an event height is active. The other six (`0x7F4E7`, `0x71B8A`, `0x6FA62`,
    `0x77423`, `0x77DC6`, `0x82A6C`) stay ledgered with their containing
    bodies.
  - **Goldens: no re-pin** (M1/M2 byte-identical, 165 lines). The M2
    observed dispatch set shrinks to 13 rows (the staged code-0/6 records are
    natively re-forced to 3 before dispatch; v9.1 provenance in
    `port/tests/test_engine_m2.c`). `make check` 108/108; ASan/UBSan clean.
- **M2 phase-10 T3 (band-reachable acceptance — the natural goal on the
  tape; spec §5; 2026-10-10).** The first M2 re-pin since the S1 v5.1: the
  natural goal reaches the acceptance tape. **Lane derivation (first-hand,
  measured live on the tape's own m 41 state):** the phase-2 code-4 record
  is the L4.1 walk's forced-decision team-target install (the m 41 team
  target), and its lane enters the half-line band natively — the unslotted
  camera-arm target (`0x7E9BB`) has the record chase the camera at ~2 units
  per granted frame, crossing 0x40 ~62 granted frames after m 41. **The
  measured blockers that make the rest staged:** (1) the m 41 row-1E claim
  machine steals the slot mid-walk every granted frame (the walk order
  puts the claim's 1E dispatch before the target record's 04, and its
  helper request makes the per-record drain merge the slot onto the
  claimant first), so the row-04 handler always sees `has_slot == 0` and
  its event arm degenerates to the velocity-words seed (the stride nibble
  is 0 on this tape, so `(0, ±4)`/12 pans nothing); (2) the camera must
  cross `|camZ| > 0xB20` inside the goal mouth for the boundary armer, and
  the pan-history co-migration from the claim place to the mouth measures
  ~2 units per granted frame (the record-bound burst/coast cycle) —
  ~4400 steps, outside a scripted tape. **The scenario (m 42, before the
  first phase-2 grant so the forced-decision install lands on the placed
  record; every forcing declared in the test):** the claim record isolated
  with the native `+0x9A` exclusion (the T2 fixture precedent), the act-1
  timeline flag (the natural phase-1 producer is bypassed by the m 1
  forcing; staging it reproduces the natural chain's own phase-2 entry
  state — the consumer machine clears the pending latch so the scan's
  queue arm runs), the goal-mouth camera park and the record placement at
  the camera inside the band (the f-up8 T4 fixture's own staged inputs;
  lane 0x10, the staged bound 0x40, the camera inside the record's +z
  angle cone). From the staged inputs on the whole chain is natural: the
  row-04 slot-dir event (the held LEFT's live slot dir `(0, +1)` -> the
  `(0, +64)` seed -> vel_z +5), the pan integrator, the boundary armer
  (zone 1, snapshot 0xB28), the clock-tail scanner's table-1 queue (id 5,
  side 0), the scheduler, the leg-0 handler's post and the FUN_00093944
  writer: **1-0, the untracked arm's 0xD3 post (captured; the display gate
  stays closed), the phase-0 write ending the scripted screen.** The match
  then holds at phase 0 (class 2: the stoppage clock; the restart producers
  are the FU-146 leg-8 situation re-queue legs), so the tape records 20
  post-goal frames and stops — no period-end exit exists on the
  natural-goal path (the honest engine state; the old tape's frame-145
  exit belonged to the direct-score path). **Re-pin (the first since S1):
  the golden is 85 lines (was 165); lines 1..46 byte-identical; the first
  differing line is frame 47 (the m 42 scenario renders through the parked
  camera); every line from 47 on differs (the goal chain 47..65, the
  writer frame's post state `state=0/1-0` from 66, the 20 post-goal hold
  frames 66..85); the key tape holds LEFT from index 45 on (the live +z
  event seed). The observed dispatch set: row 05 joins (the coda's
  `0x7F133` install + the same-frame carrier claim) and row 1E leaves (the
  claim isolation) while row 00 rejoins on the post-goal phase-0 frames
  (the L4.1 walk's step-6 dispatch runs once the phase-2 re-forcible tail
  is closed by the hold) — 14 rows. M1 untouched (`09b726b7…`).**
  `make check` 108/108; ASan/UBSan clean.
- **M2 phase-10 T4 (acceptance v10 + close-out; 2026-10-11).** The phase-10
  acceptance adopts the re-pinned tape as **v10** and adds the assertion
  layer (no engine write, no forcing change, no re-pin — the T3 re-pin is
  the record): the m 42 staging inputs are pinned at the staging site (the
  claim `+0x9A` isolation, the act-1 flag, the parked camera triple, the
  placed record and its declared zeroed velocity words); the m 58 goal link
  pins the pan in flight (+5 event velocity, zone 1, the 0xB28 snapshot, the
  tracked bind), the table-1 queue (id 5, side 0), the row-05 join and the
  **exact** dispatch set (the accepted mask minus row 00); the m 61 scored
  frame pins 1-0 / 0xD3 / the phase-0 write; the m 72 directive (top of the
  frame-77 step) pins the **exact** 14-row `M2_WIRED_MASK` (row 00 rejoined
  post-goal) and `run_tape` re-reads `dispatched_ok` after the final frame,
  so the exact compare also covers the remaining post-goal frames 77..85
  (the golden alone would only catch render-visible dispatches); the P1/P2/P4
  dormancy cells (set-piece dispatcher + taker mask, referee machine,
  replay/sub/overlay, `ball_row`) are re-asserted post-goal — the old
  direct-score block's coverage restored with the natural goal's own
  consequences named (`sit_side_pending` 1, `situation_id` 5, `screen_step`
  5, `situation_pending` 1). The natural-goal writer frame is pinned exactly
  (frame 65, engine step match_start + 60).
  **Re-pin ledger (phase-10): one re-pin — the T3 natural goal** (85 lines,
  lines 1..46 byte-identical, first differing line frame 47; see the T3
  entry above); T1/T2/T4 moved no presented frame (T4 `cmp` clean against
  the T3 golden, verified with the ISO present); M1 byte-identical
  (`09b726b7…`) throughout. The T3 review minors are folded: the
  fixture-precedent wording (test + FU-148 §13 addendum + the T3 report —
  the fixture stages camera park + placement only; the claim isolation and
  the act-1 flag are additional declared forcings), the stale m 62
  goal-link / never-panning-camera / direct-score notes marked historical or
  corrected, the dangling FU-148 §13.2 reference retargeted, the exact mask
  checks, the velocity-word zeroing declared, and the dormancy coverage
  restored.
  **Smoke (DISPLAY=:1, fresh `p10-t4-*` session; honest):** the movement
  and no-follow/no-ball results are in the "Interactive smoke" section
  below. `make check` 108/108 (the suite did not grow — the task extended
  the acceptance); ASan/UBSan clean.
  **Leg register carried out of phase 10** (each task's entry above and the
  FU docs hold the detail): the visible follow-cam / ball render (FU-148
  §14: L14.6 the `0x40` long-ball key, L14.7 the `0x157750` ball-height
  producer, L14.8 the code-`16` ball-record producer); the claim machine's
  per-dispatch helper emission (the FU-151 stage ladder — the m 42 isolation
  is its tape stand-in); the pan-history producers (the mouth
  co-migration); the FU-146 leg-8 situation re-queue (the post-goal
  restart); the six remaining `FUN_00071C94` callers + the `FUN_00071DF4`
  table/keeper second arm + the `FUN_00070DE0` bit-8 boundary arm; the
  taker rows' armed-path scanner (a phase-2 window after the goal); the
  FU-150 fouls/offside live producers; the FU-152 replay/overlay producers;
  the L4.3–L4.6 residuals and the unported row partials behind the six
  ledgered `FUN_00071C94` callers (rows 1B/1C/0D).
- **M2 phase-11 T1 (camera-height producer + long-ball key; FU-148 §15;
  2026-10-11).** The two §14.3/§14.4 legs land: **L14.7** FUN_00070B94
  (`0x70B94..0x70C05`, the `[0x157744] = 0x10F3C4` 149×16-bit arc; the
  `fifa96_camera_update_walk` store at `0x737a1..0x737b4` — after the timer
  accumulation, before the pan-step check) is ported as
  `fifa96_camera_height` + `event_peak`; the two exact-cell consumers are
  corrected to `camera.pos_y`: the `0x7C81B` record-event tail gate (waking
  the T2-phase-10 pan caller) and FUN_0007AE70's event ball height
  (`match_kick_from_record`'s kick band). **L14.6**: `FIFA96_ENGINE_KEY_LONG_BALL`
  (SDL3 V / gamepad WEST) ORs the native word 0x40; the FUN_0007CD60 request
  handler (pressed code-3 / released code-0; the `[team+0x7CB]` callback +
  the `+0x820 = 0x3C` timer) replaces the L4.4 stub; `is_team_cb` stages from
  `team.timer7cb`; the team `+0x820` countdown semantics corrected
  (`0x8DAF3` reads the alias word, not a separate limit cell); the mode-0x40
  slot-arm **errata** fixed (the dir_z side flip and the mode-6 `(dir_z+1)`
  term, `test_kick_target_mode40_slot_traj`). The live ISO test drives the
  whole chain: the live V request arms, the Z kick stages `flags 0x40` /
  code-4 / traj 0x100, the receive arm binds the event/tracked, and
  `fifa96_match_run_camera_follow` returns 1 with the tracked dirs in the
  camera velocity. **Goldens: no re-pin** (M1 `09b726b7…` / M2 `5524cf40…`
  byte-identical — the tape stages no camera event height and no long-ball
  input). **Smoke (DISPLAY=:1, fresh `p11-t1-*`; honest):** the front-end/HUD
  captures are byte-identical to the phase-10 twins; the long-ball
  walkthrough reaches live play (clock 00:12..00:17) with the movement/live
  roll visible, **no visible follow-cam and no ball sprite** — the live
  camera Y cannot bootstrap to the band boundary (no wired live event
  carries height >= 0x74; FU-148 §15.3 leg 15.a), so the visible follow
  stays the fixture chain. `make check` **108/108**; ASan/UBSan clean.
- **M2 phase-11 T2 (the `FUN_0008BAF0` ball machine, L14.8; FU-148 §16;
  2026-10-11).** First-hand re-derivation of the "ball-record machine" the
  phase-10 T1 report named: `FUN_0008BAF0` (379 insns; the
  `0x8BDEC..0x8C065` tail re-read) is the only per-frame producer of the
  ball record position `0x15880C/14` — the `0x8B664` clock-tail call under
  `[0x14C32A] == 0` (the writer census in FU-148 §16.1 shows every other
  writer is a reset/commit/penalty placement). Ported as
  `match_run_ball_machine`: the phase-2 (period <= 3) camera-derived lead
  (`tx = -3*camZ/8 ± 0x120`, `tz = 3*camZ/4 ± 0x120`, `max_speed 7`), the
  `FUN_0008DCD4` metric, the `FUN_000CD474` target heading, the `0x158881`
  row-flags gate (the reset's `FUN_0006E724(0x1587EC, 3, 0)` install; the
  `<= 0x60` re-aim at the focus record `[0x157A73]` = the camera triple),
  the `±((0x10 - speed) << 2)` steer clamp with the 10-bit wrap, the
  `0x1104CF` dword-top-byte visual byte, the `metric >> 3` speed clamp, the
  `0x114E04` sine velocity targets, the `speed/2` ±1 ramp and the
  `x/z += vel*delta` integration (`y` untouched). **Erratum (FU-148 §16.2):**
  the code-`16` sequence `0x84630` is the fouled player's face-the-ball
  action (the phase-0x19 foul installer), a *reader* of the ball record —
  the producer is `FUN_0008BAF0`. Ported posture: pool fields `target_*`,
  `speed`, `vel_x/z`, `max_speed`, `target_heading`, `ramp_timer`,
  `metric*`, `head_gate`, `head_error`, `visual_dir`, `appear_row`; the
  scene staging now passes the reset's row-3 appearance (PLAYART bank 3)
  instead of the zeroed row 0. Tests (RED-verified):
  `test_ball_machine_phase2_open_play_lead`, the ISO
  `test_live_ball_record_produced_in_open_play`, the render
  `test_ball_machine_moves_ball_slot_pixels`; fixtures parked against the
  machine. **Goldens: no re-pin** (M1 `09b726b7…` / M2 `5524cf40…`
  byte-identical — the tape's live window keeps the ball off the drawable
  set: the goal-mouth camera parks it behind the near-depth gate, FU-148
  §16.4/§17.1). **Smoke (DISPLAY=:1, fresh `p11-t2-*`;
  honest):** the match starts (front-end → X → panel confirm), the walk
  works, and an instrumented run pins the ball slot live and moving
  (`slot22 pos=(296,0,244) anim=3 bank=3 frame=8 dir=6 vis=1 draw=1
  sprite=yes off=32 err=0` through `pos=(264,0,264)`) — but no ball is
  visible on screen: with the static stand-in camera place
  `(0,0,0)` the lead parks the ball where the projection `x' ≈ 480` lands
  off the right edge (leg L14.8.e: the visible ball waits on a live camera
  place/pan). The ball height `y` stays 0 (the machine never writes it), so
  legs 15.a/15.b are unchanged. `make check` **108/108**; ASan/UBSan clean.
  **T2 fix round 1 (2026-10-11):** the review's Critical landed — the visual
  byte's table was a stride-4 misread of `0x8BF01`/`0x8C236` (`MOV
  EAX,[EAX+0x1104CF]`, bytes `8b 80 cf 04 11 00`, no SIB): the native read
  is the byte table at `0x1104D2` (0×3, 1..7 ×4, 0), now ported
  byte-granular (FU-148 §16.5 quoted errata); the test pin corrected
  (`visual_dir == 0`), the `dz` formula fixed to `three_q - camZ`
  (`0x8BBAD`), the L14.8.c `head_error` shift path gained a synthetic
  `0xFFFF` pin, and the `0e19173` subject's "8086 CL width" wording is
  corrected in place (the 8086 uses the full CL; the 186+/386 mask to
  5 bits — the `& 0x1F` code is exact). Goldens re-verified byte-identical.
- **M2 phase-11 T3 (acceptance v11 + the deferred docs-minors sweep;
  2026-10-11).** The phase-11 acceptance layer: the M2-B tape is now **v11**
  — assertions, provenance, comment/doc sweeps only, no engine write and no
  forcing change, so the transcript stays byte-identical to v10 (`cmp` clean,
  85 lines) with M1 unmoved (`09b726b7…`; M2 `5524cf40…`; **no re-pin** —
  the T1/T2 landings each re-verified here as frame-neutral, FU-148 §16.4).
  What v11 pins at the spec §5 points:
  - **the ball machine live on the tape** (T2/L14.8): from the m 41
    phase-2 flip the FUN_0008BAF0 tail runs on the m 42.. grants and the
    phase-2 (period <= 3) camera lead integrates the record off the kickoff
    spawn — the m 58 goal-link block pins the stratum exactly (`machine_ran`
    1; the reset posture `appear_row` 3 / `head_gate` 2; the lead beyond the
    0x120 offset window with `max_speed` 5 and target (-1071, 2142) from
    camZ = 0xB28; the record at (456, 28) with the ramp en route, vel
    (-3, 4) / speed 4 and the 0x1104D2 octant byte 7), and the m 61/m 72
    blocks pin the continued integration ((450, 36) one run later, (432, 60)
    three runs later). The exact pins exist because the golden cannot see
    the off-canvas ball;
  - **the long-ball chain by reference** (T1/L14.6-L14.7): the tape holds no
    0x40 key; the chain stays fixture/live-proven
    (`test_live_long_ball_mode40_band2_follow`, `test_camera_height_producer`,
    `test_kick_target_mode40_slot_traj`);
  - **the honest negatives**: no on-screen ball — the tape computes the
    engine's own FU-89 clean projection of the ball slot (the render's exact
    staging primitives) and pins it not drawable (`clean_visible` 0) at every
    sample: the camera is parked at the goal mouth (pos_z 0xB28/0xB32/0xB48
    at the m 58/61/72 samples), so the ball sits behind it and the near-depth
    gate (`rot.z < 5`) rejects it (leg L14.8.e); no visible follow — the ball
    height and the camera Y are both 0, so `d = ball_height - pos_y` can
    never present the `d == 0x70` boundary the band-2 kick needs (leg 15.a in
    its crossing form; the follow arm refuses at its 0x739DC height gate,
    `event_param` 0 <= 0x10, though the pan sets `follow_speed` 5), and the
    machine produces no ball height (legs 15.a/15.b unchanged).
  The deferred minors sweep landed as modifications: the tape header
  v9→v11 + the stale period-end sequence line; the dead exit-path leftovers
  removed (the resolving-frame phase/score samples, the FU-142a mirror, the
  consumed clock and the post-exit break cadence — the exit stays detected
  for the `exit_step < 0` negative); `fifa96_match_entities.h`'s `type`
  comment corrected to the native byte +0x8E (the `dword[+0x8E]>>24` alias
  is +0x91 — the recurring alias trap); `fifa96_input.h`'s "no static
  writer" twin and `fifa96_match_run.h`'s `input_bit0` note retargeted to
  the phase-10 T2 producer path (FUN_0001C728 / `fifa96_match_run_set_input_options`);
  the row 04/05/06/1E + chase `ball_height` stand-in comments in
  `fifa96_match_handlers.c`/`fifa96_match_run.c` named as FU-75 §1.6 /
  **leg 15.b** proxies (not the exact `[0x157750]` cell); the run.c machine
  block's table phrase, `dz` form and unbalanced paren fixed;
  `port/tests/test_ball_pairing.c`'s insertion artifact repaired; and the FU-148
  §14/§15 errata quoted as §17 (the `+0x8D` entry-gate inversion, the
  one-team timer81 loop, the 298 B window / `0x157744` cite, the `0x73793`
  load, the `0x81536` JGE and the leg 15.a crossing form).
- **M2 phase-12 T1 (the live record jump producer + the band-2 crossing,
  FU-153 leg 15.a; 2026-10-11).** The phase-12 T1 landing. **Ported (first-hand
  `/FIFA96.EXE`):** the `FUN_0007BF20` jump arc `0x7C057..0x7C113` (re-verified
  this task: the `+0x85 != 0` gate at `0x7C057/0x7C05F`, the `+0x87 += delta`
  advance `0x7C065..0x7C07A`, the `+0x87 > 2*(+0x85)` landing edge `0x7C094/
  0x7C096` zeroing pos_y/both words and copying position -> target
  `0x7C098..0x7C0BA`, the shared `0x10F3C4` sample `0x7C0BF..0x7C101`) into
  `match_run_record_jump_step` over the new pool/staging `jump_base`/`jump_timer`
  fields; the FUN_0006E8E8 animation-aux-keyed jump arms (first-hand
  `0x6ED11..0x6ED69` aux 0x3A -> `pos_y 0x18`, `0x6EDF5..0x6EE3E` aux 0x0E ->
  `pos_y 0x10`, `0x6EE90..0x6EEB7` aux 0x0F relaunch; arm velocity/move writes
  leg **T1-L1**), read from the newly ported executable frame records
  (`0x10E200..0x10EF00`, reconciled byte-for-byte with `read_memory`) at the
  native call position (action -> FUN_0006E8E8 -> mover, `0x7CD2B/0x7CD30/
  0x7CD48`); the shared arc sampler is exposed as
  `fifa96_camera_arc_height(peak, timer)` (same table as FUN_00070B94).
  **Re-verified (the FU-153 spot-checks):** the stage-0 JGE and band algebra,
  the arc/ramp tables, the install height constants (`0x6ED33`/`0x6EE14`/
  `0x6FB5C`), and the exact-hit set `{0x71..0x74, 0x95..0x9C, 0xC1..0xC4,
  0xDD..0xE4, 0x2CD..0x2DC, 0x4D5..0x4E8}` (60 values) reproduced
  independently from the reconciled tables. **Push-back vs the R1
  recommendation (evidence):** the airborne height-table seed kick needs
  `d = camera.pos_y - rec.pos_y >= 0x38` (`fifa96_action_handlers.c`'s height
  arm), and no ported producer raises `0x157750` off zero
  (the camera-height writer census; the FUN_00070C08 re-seed needs
  `ball != 0`) — so the chain cannot run fully live; the direct-cut rows and
  the R1 "installer constant" reading are disproven (the heights live in
  FUN_0006E8E8's anim-aux arms, not in the installer `FUN_0007D9A4`, whose
  decompile writes no height). **New legs:** **T1-L2** the row-07 machine's
  stage 2 gates on `+0x44` (`kick_machine_07` case 2), whose animation/event
  producer is unported (engine `row44` 0), so a second kick by the same record
  in one possession cannot stage; **T1-L3** the live camera bootstrap (above);
  T1-L1 as named. Tests (RED-verified at BASE: the unit test's first
  `pos_y == 16` assert): `test_record_jump_arc_starter` (the aux starter + the
  arc trace 16/15/13/12/9/6/2/landing, all first-hand table values) and the
  ISO `test_live_airborne_long_ball_seed_then_band2_follow` (A: the live
  claim -> the aux-row jump -> the KICK height-table code-4 receive, flags
  0x40, `d < 0x70` discriminator, the RNG pumped to the k=5 `0xE0` + even
  grid; B: a fresh chain, the disclosed bootstrap event, the kick timed to the
  exact `0x70` sample -> band 2 -> code 4 -> the pumped k=7 `0x100` ->
  `camera_follow == 1` with vel (15, 0)). **Goldens: no re-pin** (M1
  `09b726b7…` / M2 `5524cf40…` `cmp` byte-identical — the tape enters no
  jump-aux row and no jump state, so the arc is dormant there). **Smoke
  (`DISPLAY=:1`, fresh `p12-t1-*`; honest):** front-end/HUD draw, the
  long-ball walkthrough reaches live play (clock 00:04..00:14), players move
  (scene AE 9381..38998) and **no visible follow-cam** — the camera stays the
  stand-in (the pitch is not drawn; no re-centering), consistent with legs
  T1-L3/L14.8.e. `make check` **110 assertions / 108/108**; ASan/UBSan clean;
  the `wired` census stays 21/80 (no action row changed). **T1 fix round 1
  (2026-10-11):** the review's Critical — the arm's ramp argument is the arm's
  own `+0x83` marker read through the `dword[+0x81] >> 16` alias
  (`0x6ED08`/`0x6ED11`/`0x6ED17`: store 0x18 then `ramp(0x18)`; likewise
  `0x6EDE6`/`0x6EDEF`/`0x6EDF5` 0x10, `0x6FB2F`/`0x6FB38` 0x50), **not**
  `word[+0x81]` (`0x7CA5C` proves the countdown is a different cell) — is
  fixed (the fifth `dword>>16` alias strike; FU-153 §11 errata); the live jump
  base is a constant `ramp(0x10) = 15` (crest 16) / `ramp(0x18) = 18` with no
  `timer81` seed needed, so the earlier "tiny blip" framing is withdrawn; the
  unit test now stages `timer81 = 0` and still pins the crest-16 trace; the
  test-A bootstrap injection is labelled T1-L3 and the case-0x0F missing
  `FUN_00079B1C` snap (`0x6EE92`) is on the T1-L1 leg.
- **M2 phase-12 T2 (the on-screen ball — the native camera-0 view seed,
  L14.8.e; FU-154 §5.1/§9; 2026-10-11).** The phase-12 T2 landing closes
  **L14.8.e** at the bounded orientation step (FU-154 Step 1):
  `fifa96_match_run_reset_render` seeds the fresh-match view from the native
  camera-0 static matrix fields — image [0x107DC8] = 0x107508 (re-verified
  first-hand this task), +0x58 yaw = 0xBF9C, +0x5C pitch = 200
  (`read_memory 0x107560` = `9c bf 00 00 c8 00 00 00`, copied to 0x14E4EC/F0
  for the projection matrix at `0x4C918..0x4C928`) — the same object whose
  +0x4C = 0x1500 the engine already carries as MATCH_RUN_VIEW_RATIO. The
  render's `fifa96_projection_matrix(r->yaw, r->pitch, …)` consumes the seed,
  and the phase-2 ball lead is a pure function of the camera triple
  (first-hand `0x8BB7C..0x8BBF6`, FU-154 §1), so the ball's slot now lands on
  canvas while the camera *place/pan* in live play stays the T1/R1 event
  coupling (R2-L4; no live event fires in the smoke window). Measured live
  (ISO chain, deterministic): the staged slot 22's render-own clean
  projection is on canvas for all 16 kick-roll window frames (x' 152 -> 4,
  exits left) where the stand-in yaw 0 was 0/90 frames (settled x' = 480);
  pinned in `test_live_ball_record_produced_in_open_play` (the derived art
  pair staged exactly as the bridge does), the reset pin in
  `test_engine_match_render` (yaw 0xBF9C / pitch 200); the scene fixtures
  re-stage yaw = pitch = 0 (their documented identity-view premise), so
  `test_ball_machine_moves_ball_slot_pixels` and their hashes stay pinned.
  **Goldens: M2 re-pinned to v12 with reason + frame diff** — lines 1..8
  byte-identical (the pre-grant empty scene), lines 9..85 (77 lines) all
  differ: from frame 9 the staged 23-slot scene renders the native camera-0
  orientation; in the pre-park window (frames 9..45, camera (0,0,0)) the ball
  slot now draws on canvas (projected x' ≈ 157 at the (480,0,0) kickoff
  spawn), and at the m 58/61/72 goal-mouth-park samples the clean projection
  flips not-drawable -> drawable with the lateral screen bound asserted
  off-canvas (measured x' 2030/2055/2128 — the v12 stratum adds the bound the
  v11 near-gate flag lacked). M1 unmoved (`09b726b7…`); new M2 sha
  `2759dace…`; `make check` 108/108; ASan/UBSan clean. **Smoke
  (`DISPLAY=:1`, fresh `p12-t2-*`; honest):** front-end/HUD draw, the match
  runs (the clock starts on the KICK burst), and the ball slot is **drawn on
  screen** — the instrumented SDL run (scratch, reverted) traces 626
  `match_run_draw_sprite` calls for slot 22 at x = 156 (320-space) at the
  kickoff spawn, where the pre-seed slot never passed the near gate; the
  roll window then moves it left. Caveat (new leg **T2-L1**): the sprite
  content at the ball's spot resolves to a PLAYART player-pose frame (bank 3
  frame 0, 19..24x39..44 family) because the render's frame source is the
  documented synthetic identity table — the real per-row frame tables
  (`0x10E200..` family) stay the open leg (Task-2/T5), and the native
  assembled bank-list order vs the FU-84 row +8 index is part of T2-L1; the
  view-object handler that keeps the ball framed after the roll is leg
  R2-L1, the live pan reachability R2-L4. `wired` census stays 21/80 (no
  action row changed). **T2-L1 (new leg):** the ball-slot sprite provenance —
  the FU-84 row +8 bank index (row 3 -> 3, first-hand `read_memory 0x10EF1B..`
  `03 0b 07 03 7e e2 10 00 03`) against the staged bank list (PLAYART first,
  91 banks; GAMEART0's banks stage count-0/absent), plus the executable
  per-row frame tables as the true sprite source. **T2-L2 (new test-harness
  leg, recorded by the T2 review round):** `test_engine_m2`'s no-ISO
  (no-assets) mode aborts in the clean-projection helper's
  `assert(r->entity_count > slot)` — the scene composer only stages slots
  when the render is enabled, so the helper's live-slot assert fires before
  the documented no-assets self-consistency path runs. Pre-existing since the
  phase-11 T3 helper addition (verified at BASE); the ISO-present acceptance
  path is unaffected, and the no-assets mode needs the helper to skip (or the
  assertions to be mode-gated) in a future sweep.
  **Build topology note (candidate ledger item, no change made):** the
  `fifa96_engine` target builds **without** ASan/UBSan (CMakeLists attaches
  the sanitizer flags to the test targets only; `build/CMakeFiles/
  fifa96_engine.dir/flags.make` = `-g -std=gnu11 -Wall -Wextra -Werror`), so
  UB inside engine TUs is invisible to the suite — recorded for a future
  build-topology slice (T3 changes no build system). `make check`
  **108/108**; ASan/UBSan clean; RED evidence for the v11 layer: disabling
  the machine call aborts on `mr->entities.ball.machine_ran == 1` (scratch,
  restored).
- **M2 phase-12 T3 (engine sanitizer build — the build-topology slice;
  2026-10-11).** The phase-12 T2 entry's build-topology note is closed by
  this landing — quoted: the `fifa96_engine` target built **without**
  ASan/UBSan ("UB inside engine TUs is invisible to the suite — recorded for
  a future build-topology slice"). Now `fifa96_engine` compiles with
  `-fsanitize=address,undefined -fno-sanitize-recover=all` alongside the kept
  strict flags (`build/CMakeFiles/fifa96_engine.dir/flags.make` =
  `-g -std=gnu11 -Wall -Wextra -Werror -fsanitize=address,undefined
  -fno-sanitize-recover=all`); a static archive has no link step, so the
  `-fsanitize=address,undefined` runtime propagates to every consumer via the
  target's INTERFACE link options, and the SDL3 `fifa96` main carries the same
  private compile+link pair inside the `SDL3_FOUND` guard (`ldd build/fifa96`
  -> `libasan.so.8` / `libubsan.so.1`; the headless-only path re-verified on a
  fresh tree with `-DCMAKE_DISABLE_FIND_PACKAGE_SDL3=ON` — configure + full
  build, 0 errors). Run under the sanitized engine: `make check` **108/108**;
  M1/M2 byte-exact to the goldens (`cmp` clean; `09b726b7…` / `2759dace…`; no
  re-pin); a 12 s bounded `DISPLAY=:1` run of the sanitized `fifa96` main
  (boot/front-end idle) ends only by timeout: no ASan/UBSan memory-error
  report; LSan flags 3 third-party allocations at exit
  (`SUMMARY: AddressSanitizer: 1942 byte(s) leaked in 3 allocation(s)`, all
  allocation sites `libdbus-1.so.3` via `libnvidia-glcore.so` <-
  `SDL_GL_LoadLibrary` reached from `sdl3_init` — a driver/dbus artifact, no
  engine frame in any allocation site; a bounded observation, not an engine
  leg).
  **Triage: no engine-code sanitizer finding** on any exercised path — no UB
  surfaced, so this slice lands no fixes and no new legs; the report channel
  stays live for future slices. **T3 fix round 1 (2026-10-11):** the original
  smoke wording ("no ASan/UBSan report") was false as recorded — the first
  run had leak detection suppressed; the wording above is the first-hand
  re-verification (recipe + quoted LSan output in the t3-report fix-round
  section), and `docs/PROGRESS.md` carries the same claim. `wired` 21/80
  (unchanged).
- **M2 phase-12 T4 (acceptance v12 completion + the phase-12 leg register;
  2026-10-11).** The final phase-12 slice adds the acceptance strata for the
  phase's visible legs on the T2 v12 tape (`port/tests/test_engine_m2.c`) without
  moving it — the transcript stays byte-identical to the v12 golden
  (`2759dace…`; no further re-pin), M1 unmoved (`09b726b7…`):
  - **the on-canvas ball stratum** at the visible window: at the first granted
    frame (9) slot 22's clean projection is drawable ON canvas, pinned
    exactly (clean x `10266855` = x' 156.66 — the pixel the smoke draw trace
    measured; the stand-in yaw measured it off the right edge) — plus the
    staged art identity (anim_id 3, the reset's row-3 appearance install;
    frame 0 of the synthetic per-row table, leg T2-L1);
  - **the follow-arm chain reference**: at m 58 the tape asserts its own
    predicate `fifa96_match_run_camera_follow(mr) == 0` and the pool-wide
    jump-state dormancy (no record carries `jump_base`/`jump_timer`); the live
    positive is the T1 ISO chain
    (`test_engine_match_frame::test_live_airborne_long_ball_seed_then_band2_follow`
    — the exact-`0x70` sample -> band 2 -> follow 1), referenced rather than
    duplicated; a scratch RED (the T2 seed zeroed, reverted) fires the tape's
    first projection stratum (`ball_link_drawable == 1`, rc 134);
  - **T2-L2 closed**: the clean-projection samples are mode-gated on
    `render.enabled`, so the no-ISO run now reaches the documented
    `SKIP golden comparison (no ISO)` path green (rc 0, first-hand) instead of
    aborting in the helper's live-slot assert (the phase-11 T3 residue); the
    ISO-present behavior is unchanged.

  **Smoke (`DISPLAY=:1`, fresh `p12-t4-*`; honest):** the front-end capture is
  byte-identical to `p12-t2-frontend` (AE 0); the tape navigation reaches the
  match, the kickoff view draws the ball slot on screen at the centre (the T2
  observation reproduced; the HUD band AE 0 vs `p12-t2-ball-kickoff`, the
  scene band differs only by the live animation phase, AE 8812); the KICK
  burst starts the clock and the roll window empties the slot (scene AE 9834
  kickoff -> after-roll); holding UP moves the controlled record (scene AE
  2636 across the hold; clock 00:02 -> 00:15) and the view does **not**
  re-centre — **no visible follow-cam**, consistent with legs T1-L3/R2-L4;
  captures
  `docs/screens/p12-t4-{frontend,ball-kickoff,kick,walk,after-roll}.png`.
  **Phase-12 leg register (folded here):** **T1-L1** (the jump arm's
  velocity/sine/move writes + the live row selector + the case-0x0F
  `FUN_00079B1C` snap `0x6EE92`), **T1-L2** (the row-07 stage-2 `+0x44`
  animation ack: a second kick per possession cannot stage), **T1-L3** (the
  live camera bootstrap: nothing raises `0x157750` off zero; the height table
  needs `d >= 0x38`), **T2-L1** (the ball-slot sprite provenance: the
  executable per-row frame tables `0x10E200..` and the FU-84 row +8 bank
  index vs the staged bank order), **T2-L2 closed above**, **R2-L1** (the
  live view-object handler that keeps the ball framed after the roll),
  **R2-L4** (the live camera place/pan reachability: no live event fires; the
  camera stays the stand-in), **L155.1** (R3/FU-155: the native machine's
  legal window is byte==0 sessions, so the engine's run-every-frame composite
  is the disclosed approximation — L155.2..4 stay on FU-155). T3's bounded
  LSan observation (3 third-party allocations at SDL exit) and the
  unsanitized non-engine libraries remain recorded observations (a topology
  candidate), not legs. `make check` 108/108 (ASan/UBSan engine + test TUs);
  `wired` 21/80 unchanged; README title stays 26%.
- **M2 phase-13 T1 (live camera — place/pan producers; FU-157; 2026-10-11).**
  Lands R5's two bounded pieces (FU-157 §5 steps 1-2), no producer beyond
  them:
  - **The claim pan asserted live** (new ISO test
    `test_engine_match_frame::test_live_claim_pan_row04_half_line`): the
    natural chain kickoff -> KICK -> phase 2 -> long-ball request -> walk UP
    to the claim produces the row-04 half-line event (seed `slot_dir_x << 6`
    => `vel_x = 64/0xC = 5`), `tracked = 9`, `event_param = 0`, and the
    FUN_000736AC pan walks `pos_x 0 -> 0x90` monotonically with the pan-step
    decay `5 -> 3 -> 2 -> 1 -> 0`. Sensitivity RED-verified by a scratch
    mutation of the row-04 event wiring (reverted; the assert aborts).
  - **The throw-in stage-3 Y cut restored** (FU-157 §4.2: native
    `[0x157750] := 0x80` at `0x85911/0x85919`, handed to FUN_000700F4 at
    `0x85950/0x85958`): `fifa96_match_action_10` stage 3 now places
    `(rec.x + dir<<6, 0x80, rec.z + dir<<6)` instead of the dropped Y = 0.
    New unit test
    `test_engine_match_handlers::test_live_throw_in_camera_y_cut` drives the
    real row dispatch and asserts `pos_y == 0x80`, the BF20 record-event
    tail gate operand nonzero (`0x7C81B`) and the height-table band
    `d = 0x80 - rec.pos_y >= 0x38`. RED first (the pre-fix port passed 0),
    GREEN after.
  - **The follow stays T3-gated**: the restored Y is the bootstrap (the only
    live in-open-play `[0x157750]`-off-zero producer the port dropped); the
    airborne route throw-in -> jump kick -> height-table code-4/0x40 ->
    receive `event_param > 0xF0` -> `camera_follow == 1` -> sustained pan
    needs T1-L1/T3's live row selector (R5-L2).
  - **Goldens unmoved** (live-only paths): M1 `09b726b7…` byte-exact, M2
    `2759dace…` byte-exact; `make check` 108/108 (sanitized); `wired` 21/80
    unchanged; README title 26%. **Smoke (`DISPLAY=:1`, fresh `p13-t1-*`;
    honest):** the live run reaches phase 2 (clock ticks on the held-Z
    kickoff kick) and the pad moves the controlled record, but the sampled
    walk windows show **no whole-scene pan translation** (a standing record's
    column fixed within +-15 px); one window shows a hard camera re-place to
    a close-up (clock 00:19; unidentified event, consistent with a restart
  cut). The `0 -> 0x90` slide is asserted by the ISO test, not reproduced
  on screen; captures
  `docs/screens/p13-t1-{frontend,kickoff,live-play,walk,camera-cut}.png`.
- **M2 phase-13 T2 (ball-slot art — real ball sprites; FU-158; 2026-10-11).**
  Closes T2-L1's visible half (slot 22 no longer draws a PLAYART pose):
  - **The bank capture** (FU-158 §5/§8.1): `fifa96_match_run_stage` matches
    the pitch container's BIGF name `ball.fsh` and commits
    `render.ball_bank` = the player-container offset + entry (first-hand ISO:
    PLAYART 91 entries, GAMEART0 entry 37 `ball.fsh` 8 frames 16x16 -> 128);
    no `ball.fsh` keeps the `0xFF` sentinel and the FU-84 resolve.
  - **The composer roll** (FU-158 §4.3/§8.2): `render.ball_roll` advances the
    native `[0x155C4E]` byte op (`lo+1` OR'd into the high nibble; the fresh
    bytes correct §8's `& 0xF` sketch) once per granted frame while the ball
    moves (the disclosed stand-in for the camera rate words, L158.2), with
    the presenter sanitizer (0x5739D: frame >= count -> 0, nibble cleared)
    applied at the drawn-frame-equivalent composer site.
  - **The dedicated draw** (FU-158 §6/§8.3): `fifa96_match_run_render` draws
    `banks[ball_bank]` frame `roll & 0xF` at slot 22's FU-85 clean x/y/scale
    through `match_run_draw_sprite`, bypassing `fifa96_render_resolve` for
    that slot (no direction fold). `match_run_anim_rows`/PLAYART/the resolver
    are untouched; the FU-84 row-3 install stays the first-hand record.
  - **Tests (RED first)**: `test_engine_match_render::`
    `test_ball_slot_draws_ball_bank_frame` (bank frame vs pose pixel; roll
    selection; sanitizer; 0xFF fallback) and
    `test_ball_roll_advances_with_ball_motion` (parked = no advance; +1 per
    moving frame with the count-2 sanitize cycle asserted per frame); the ISO
    staging test + the M2 frame-9 stratum pin `ball_bank == 128`.
  - **M2 re-pinned to v13** with the reason + frame diff (85 lines;
    lines 1..8 identical; 77 hash-only lines 9..85 differ; zero `state=`
    diffs; first differing line frame 9 `ab8bded120648826` ->
    `40270bb4a15d2302`; new M2 sha `5f7e7692…`; M1 unmoved `09b726b7…`).
    This is the intended visible-quality upgrade (FU-158 / plan T2), recorded
    in the `test_engine_m2.c` v13 section. `make check` 108/108 (sanitized);
    `wired` 21/80 unchanged; README title 26%.
  - **Smoke (`DISPLAY=:1`, fresh `p13-t2-*`)**: the kickoff view draws the
    ball as an actual soccer ball (`ball.fsh`; the 4x crop
    `docs/screens/p13-t2-ball-crop.png` is unambiguous); post-kick the ball
    leaves the fixed view within ~1 s (the phase-2 lead; no live follow —
    R2-L4), so the roll frames do not land in the stills — the roll's runtime
    evidence is the unit-test sequence and the v13 re-pin.
  - **Legs**: L158.1 (native entity-pass slot-22 pose suppression, DOSBox-X),
    L158.2 (extended: sanitizer write-back site + motion stand-in),
    L158.3/L158.4 unchanged; **new L158.5** (the presenter's `[0x109000]`
    scale / `FUN_00057278` screen-pair source).
- **M2 phase-13 T3 (jump/ack residuals — the repeatable live sequence;
  FU-159/160/161/162/166; 2026-10-11).** The jump chain is now live and
  repeatable in the engine; five pieces landed:
  - **The row-0x0E live jump selector (FU-160 §2/§4 rec #1; the census
    headline).** The row-23 tackle arm's `install = 0x0E` drain
    (`fifa96_match_handlers.c:2203`) now runs the ported body
    (`0x82710..0x82ACF`): the phase gate + `timer89` accumulate, stage 0
    (index gate, the `0x71B9C` camera-target stand-in, the `0x8DCD4` metric +
    `0x79C50` face), stage 1 (the `0x157794` target, the `0x79B58` self
    receiver timer, `timer89 >= 0x0C` -> `FUN_0006E598(self, 0x12, frame 0)`,
    ran), stage 2 (controlled/`jump_base` -> 3; the camera/slot arm
    `0x82820..0x82A9C` is leg **FU160-L4**), stage 3 (`+0x44` reset). The
    installed row 0x12's f2 funnel (aux 0x0E) runs the already-ported jump
    arm + BF20 arc. **wired 21 -> 22/80** (dispatch 57 UNSUP / 22 OK /
    1 NOTF); README title recomputed to **27%** in the same commit. Tests:
    `test_action_0e_live_jump_selector` (unit stage/gate/reset) +
    `test_live_row_0e_selector_installs_jump` (ISO: natural kickoff ->
    install 0x0E -> anim 0x12 frame 0 -> airborne). Rec #2 (row-0x5E via the
    row-0x0C stumble) still waits on a live contact entry; rec #3 (rows
    0x14/0x5C) stays the bounded negative.
  - **The byte44 ack producer + row successor + install helper (FU-159
    §4.1/§5.2/§5.4).** `+0x44` is the current anim row's flags bit 0
    (native row install `0x6E6CC`), derived from `anim_id` at the dispatch
    staging instead of the always-zero pass-through, so the kick-07/08
    stage-2 gates now fire (`s.byte44` staged live) — the second kick in one
    possession is unblocked (`test_pad_kick_release_runs_kick_row` pins the
    reset). `fifa96_match_run_record_anim_install` ports the FUN_0006E598
    store core (id clamp, frame resolve, row44, the `+0x46 = 1` install
    default, the `+0x32 = 0` frame-timer reset) and the public
    `fifa96_match_run_record_anim_successor` ports the FUN_0008E008
    frame-wrap `next_id` chain (0x6F keep, `< 0x70` install, `>= 0x70` mirror
    `(type+4)&7` + install; install frame = row last + 1 with the clamp;
    terminal rows take the movement tables — leg FU159-L5), wired at the
    scene stage's wrap edge. Tests: `test_record_anim_install_row44`,
    `test_record_row_successor_chain`, `test_record_jump_case_0f_snap_frame3`
    (unit) + the live chain reaches row `0x5E -> 0x0F -> 0x00`.
  - **The aux one-shot model + impulsion + freeze + stop (FU-161
    §3.1/§5.2/§5.4).** `e->aux_word` (native `+0x36`) re-arms on the
    (anim_id, frame) resolve edge; the ported one-shot arms (0x0E/0x3A)
    clear their byte, the sustained arms (0x0F/0x05/0x2A/0x2B/0x0C) keep it —
    the arc no longer re-stages every tick (`test_record_jump_arc_starter`
    proves the descent without the old anim mask). `match_run_record_impulse`
    (`vel = dir[oct] << shift`, speed = the `0x8DC68` metric) + the new
    `fifa96_match_dir_x/_z` tables (flat 0x10F334/0x10F33C) drive 0x0E (`<<2`
    one-shot), 0x2A (row keyed `{0x4D,0x59,0x11,0x5E} -> <<2`), 0x2B (while
    `timer < 2*base`, row 0x5E `<<2`, + the frame freeze consumed by the
    scene stage) and 0x0C (grounded target step; the `0x10F331`/`0x10F339`
    dword bases resolve to the same dir bytes through the `>>24` alias, fresh
    this task) and 0x05 (`FUN_00079B1C` snap). Tests:
    `test_record_jump_impulsion`, `test_record_jump_sustained_and_freeze`,
    `test_record_jump_grounded_step`, ISO
    `test_live_jump_impulsion_carry_and_stop` (axis impulse (4,0)/4 -> the
    carry -> the f5 snap zeroes speed/vel and sets target := pos).
  - **The throw-in cut index fix (FU-162-L3).** The row-0x10 stage-3 camera
    cut now indexes the `0x10F334/0x10F33C` offsets with the **live face
    octant** `record.type` (native `byte[rec+0x8E]`, fresh `0x8591e`), not
    the never-written `actor_type` (OL-83);
    `test_live_throw_in_camera_y_cut` re-derived per octant (mutation-RED
    verified). The `0x7A490` throw launch (FU-162-L4) stays the live
    throw-in -> receive -> follow prerequisite.
  - **The session seed (FU-166/R14).** `fifa96_match_run_begin` now seeds
    `session_gate_14c32a = 0` — the settled ordinary-match value (every
    GAME-SELECT item but Practise writes mode 0; byte 1 is the `ptmsel`
    Practise session). The goal-consumer/scheduler fixtures stage the byte-1
    session explicitly (it is the native family they exercise); the M2
    acceptance tape stages gate 1 at the kickoff directive to keep pinning
    the ported queue/scheduler chain, so **M2 stays v13 byte-exact**. The
    default-path tape re-derivation (byte 0 -> the direct `0x8AA7B` goal arm)
    is leg **FU166-L5** (T4 scope).
  - **Goldens**: M1 `09b726b7…` byte-identical (immovable); M2 `5f7e7692…`
    byte-identical (no re-pin) — the tape never dispatches row 0x0E and its
    records stay on terminal self-loop rows. `make check` 108/108
    (sanitized, strict). **Smoke** (`DISPLAY=:1`, `p13-t3-*`): the live
    match runs (kickoff `00:00` with the ball art -> live play `00:05` with
    moving records); the tackle-driven row-0x0E jump is not key-drivable, so
    the live chain's evidence is the ISO tests.
  - **New legs**: FU160-L4 (row-0x0E stage-2 camera-follow/slot arm),
    FU159-L5 (refined: the `+0x46` lean flags-bit-2 update; the install
    default 1 is modeled), FU161-L3 (re-scoped: the "second direction pair"
    is the same 0x10F334/0x10F33C bytes through the `0x10F331`/`0x10F339`
    dword bases; the `0x33` rotated index stays), FU166-L5 (above); the
    row-0x5E live installer (FU160 rec #2), FU159-L1's other-row sine steps,
    FU161-L4 (+0x98/+0x99) and FU161-L5/L6 are carried unchanged. The
    one-shot model covers the ported arms only; the unported one-shot arms
    (e.g. row 0x5C's high 0x24) keep their bytes (no-op repeats).
  - **Fix round 1 (2026-10-11, commits `3948564` + the review fix).** The
    grounded 0x0C step's signed shift was UBSan-UB (`3948564`, now `* 0x20`;
    the live row-0x0E test also asserts the successor chain
    `0x12 -> 0x0F -> 0x00` + the terminal `+0x44 = 1` live). The review's
    act-2 finding is resolved with fresh bytes: `0x8947A` aborts the referee
    act-2 hand-off under byte 0 for **every** contact kind (the family is the
    Practise session's), and `[0x158882]` is the ball block's `+0x96` byte
    (`FUN_0006E724` writes `row_flags & 1`, fresh decompile) — the old
    `foul_kind == 3 || gate` compression let kind-3 contacts through under
    byte 0; the engine now aborts iff the gate is 0
    (`test_engine_referee_restart_default_gate_aborts`; the byte-1 fork stays
    covered by `test_engine_referee_contact_fk`). L14.8.d's open question is
    answered (FU-148 §18 errata): the two `[0x14C32A]` poles are consistent
    per session, byte 0 = machine + no scheduler, which is the engine's
    settled default. Also: the row-0x0E `timer89` delta is zero-extended
    (`0x82734 MOV AX,[0x157A64]`), the scene-stage successor re-stages the
    new row id/bank for the same-frame draw, and the pad fixture now executes
    a literal second kick in one possession. New leg **FU166-L6** (the act-2
    multi-stage compression: the `0x89214` 6-entry table, the decision-word
    check `0x89494/0x89499`, the camera-word gates). `make check` 108/108;
    M1/M2 byte-exact.
  - **Fix round 2 (2026-10-11, re-review): the act-2 net rule corrected.**
    Quoted from the round-1 entry above:

    > the engine now aborts iff the gate is 0
    > (`test_engine_referee_restart_default_gate_aborts`; ...)

    That was a **single-call misreading**: the `0x89487`/`0x894B7` abort jumps
    to the shared epilogue (`0x89615`) and **skips the `0x895E6` teardown**
    (`[0x15882A]=1`, `[0x158828]=0`, `[0x158808]` step pointer = 0,
    `[0x158829]` stage = 0), so the machine stays armed; the dispatcher reads
    the stage from memory (`0x892FE MOV AL,[0x158829]`, table `0x89214`) and
    the aborting body already incremented it (`0x89478/0x8947F` -> 3), so the
    next driver call (`0x8B669/0x8B672`) re-enters at `table[3] = 0x894A2`,
    bypassing `0x89487`. The stage-3 gate's `[0x158882]` term is the ball
    block `+0x96` row-terminal flag: the stage-2 `FUN_0006E724(0x1587EC, 0)`
    keep/reroll leaves rows 0/0x62/0x63/0x64/0x65 (flags 0x07 -> 1), so
    **kind == 3
    completes under byte 0 on the re-entry call** (the old `kind==3 || gate`
    exception was net-right), while kind != 3 installs row 0x49 (flags 0x00)
    and holds armed. The engine models the split as `ref_restart_stage = 2`
    (`fifa96_match_run.c`, fresh comment), and
    `test_engine_referee_restart_default_gate_split` pins both byte-0 cases
    (soft holds; kind 3 completes) and the byte-1 fork stays covered by
    `test_engine_referee_contact_fk`. FU166-L6 is extended with the re-entry
    semantics and the ball-base writer catalogue (`0x887D5` row 3,
    `0x8C2CE` row 0, `0x8A18C` row 0x48, `0x8941D` keep/reroll,
    `0x89464` row 0x49).
- **M2 phase-13 T4 (acceptance v13 + the two riders + smoke; 2026-10-12).**
  The final phase-13 slice adds the acceptance strata on the T2 v13 tape
  without moving it — the transcript stays byte-identical to the v13 golden
  (`5f7e7692…`; no further re-pin), M1 unmoved (`09b726b7…`):
  - **Acceptance strata** (`port/tests/test_engine_m2.c`, the v13
    acceptance-completion block): the T1 positives are referenced
    (`test_live_claim_pan_row04_half_line`,
    `test_live_throw_in_camera_y_cut`); the T2 ball stratum is extended — the
    captured-bank pin `ball_bank == 128` stays and the composer roll is now
    pinned at all four acceptance samples (`0` at frame 9 — the kickoff
    spawn's zero velocity gates the +1; `5`/`6`/`1` at m 58/61/72; the drawn
    frame stays in 0..7, the 0x5739D bank-8 bound); the T3 jump chain is
    referenced (the row-0x0E selector, the byte44/successor/install and the
    impulsion/freeze/stop pins) and its tape dormancy asserted
    (`(mask_final & (1 << 0x0E)) == 0`, `jumps_active == 0`,
    `follow_on_tape == 0`); the honest dormancy is named (FU160-L4's
    stage-2 camera/slot arm, the partial live follow, the non-key-drivable
    jump).
  - **FU166-L5 decided: keep the M2 tape's staged byte-1 session** — the
    tape pins the deepest ported chain end-to-end (queue -> scheduler ->
    post -> writer, natively the byte-1 Practise family); the engine's
    byte-0 default is pinned where it belongs (the begin seed, asserted at
    match start) and covered end-to-end by
    `test_engine_match_frame::test_natural_goal_fallback_arm` (the direct
    `0x8AA7B` arm: score + table-2 situation 6/phase 5). Re-deriving would
    trade the deepest chain for the shallower direct arm on the acceptance
    tape and re-pin a v14 for no coverage gain. Recorded in FU-166's T4
    status.
  - **Riders (from phase-12's final review).** (1) The aux-`0x3A` jump-arm
    regression pin (`test_record_jump_arm_aux_3a_base_literal`): the row
    `0x5C` frame-2 aux `0x243A` (fresh `read_memory 0x10EDBE`:
    `00 01 3A 24 02`) reaches the arm; `fifa96_camera_ramp(0x18) == 18` is
    asserted literally and the frame asserts `jump_base == 18` /
    `jump_timer == 20` / `pos_y == 23` / the one-shot `0x2400` — RED-verified
    by reintroducing the `dword[+0x81] >> 16` alias strike (reading the
    `+0x81` countdown resolves `ramp(0) = 3` and aborts exactly there), so
    the alias class is regression-proof on the second ported one-shot arm
    (the 0x0E guard stays in `test_record_jump_arc_starter`). (2) The no-ISO
    tape path is now CI-covered: the CTest registration
    `test_engine_m2_noiso` runs the same tape from the build directory,
    where `data/game/FIFAPCCD96.iso` does not resolve, so
    `SKIP golden comparison (no ISO)` (rc 0) is exercised even with the ISO
    present in the checkout; the primary registration keeps pinning the
    golden.
  - **Docs sweep**: the keep/reroll set corrected to `{0,0x62,0x63,0x64,0x65}`
    (from `0/0x62/0x65`) at all five sites (the `fifa96_match_run.c`
    comment, the frame test, this section's fix-round-2 text, FU-150,
    FU-166 x2) — fresh `decompile_function 0x6E724` re-verifies the keep set
    and the reroll switch; FU166-L6's four residuals are folded explicitly
    (kind proxy, decision-word check, camera gates, teardown poke) and the
    round-2 blockquote's rewording is marked (FU-166 T4 status).
  - **Goldens**: M1 `09b726b7…` byte-exact; M2 `5f7e7692…` (v13)
    byte-exact, no re-pin. `make check` **109/109** (sanitized, strict;
    +1 = the no-ISO registration). `wired` 22/80 unchanged; README title
    stays **27%** (verified `floor(100*22/80) = 27`).
  - **Smoke** (`DISPLAY=:1`, `p13-t4-*`; honest): the panel -> match shows
    the kickoff with the ball art (byte-identical to `p13-t2-kickoff`,
    deterministic across sessions); the kick starts the live play
    (kickoff -> play AE 6280; records move, held-key walk AE 2636-3458 per
    window) and the visible window then cuts to a near-field close-up (the
    T1-reported class; no sustained whole-scene pan isolated on screen); the
    late match holds a sparse near-field framing with a lone record (clock
    advancing, AE ~150/window — the honest visible state); the jump stays
    not key-drivable (the tackle drain is contact-gated), so its runtime
    evidence is the ISO tests. Captures:
    `docs/screens/p13-t4-{frontend,live-kickoff,live-play,camera-cut,walk,hold}.png`.
  - **Leg register after T4**: closures/decisions — FU166-L5 decided (keep;
    the byte-0 tape re-derivation is no longer planned), FU166-L6 residuals
    folded (still a leg), T1-L1's alias rider closed by the pin. Carried
    unchanged: FU160-L4 (stage-2 camera arm), FU159-L5, FU161-L3/L4/L5/L6,
    the row-0x5E live installer, the row-0x12 f3/f5 sub-arms, R5-L1..L7,
    R2-L1/R2-L4, L155.1..4, the FU-149..152 leg sets.
- **M2 phase-13 final whole-plan review fix (2026-10-12).** The review's MF1:
  the row-0x0E stage-2 gate read `entities.controlled` (`[0x157A83]`); the
  native at `0x82806 CMP EBP,[0x001577CA]` (`3b2dca771500`) compares the
  **camera-tracked** record (`render.camera.tracked`, FU-152 §2.9) and
  `0x82812 CMP word[EBP+0x85],0` the jump word — both to the same advance
  block (`0x82AA0`: `timer89 = 0`, stage `2 -> 3`). The port now reads
  `mr->render.camera.tracked == r->entity_id` (FU-160 §7 errata; the old
  `entities.controlled` read was a cell conflation, not a recorded
  stand-in). The unit pin `test_action_0e_live_jump_selector` asserts
  the tracked-only and `+0x85` arms separately (RED-verified against the
  pre-fix read). Minors: the successor compare cite corrected to
  `0x8E143/0x8E146` (was `0x8E0E9`, the tail of the frame-arg build), the
  `ball_bank` capture gains the undecoded-slot guard, the
  `0x6E598..0x6E713` range notation fixed, and AGENTS.md's census line synced
  to `22/80 -> 27%`. `make check` 109/109; M1/M2 byte-exact (the tape never
  dispatches row 0x0E, so no re-pin).
- **M2 phase-14 T2 (sustained follow — the rate arm + window coupling;
  FU-168/FU-171; 2026-10-12).** The phase-13 "follow is starved" ceiling is
  closed: the sustained follow is now proven live from the event through the
  every-frame path, and the two row-04 staging stand-ins FU-171 flagged are
  re-staged. **Wired 24/80 unchanged; README title 30% unchanged; M1/M2
  byte-identical (no re-pin).**
  - **The follow rate arm re-verified first-hand** (`0x71DF4..0x71F30` fresh:
    tracked `[0x1577CA]` + class `[[rec]+4][0] == 0x18D8` + slot
    `[[rec]+0x20]` + the `[0x1577EE]>>16 > 0xF0` gate -> `vel =
    clamp(slot dirs x 15, +-15)` with the alias-word clamps; the call site
    `0x739C6..0x73B6B` gates pan_counter/speed/height/rate bytes; the walk
    tail `0x73CB8` confirms the gate-time speed split). No code change was
    needed — the port (`fifa96_camera.c:525-533`,
    `match_run.c:3128-3147`) is exact; the verification is recorded in
    FU-171's T2 status.
  - **The coupling recs landed** (`fifa96_match_handlers.c`, row-04
    snapshot): `s.ball_height` now reads `mr->render.camera.pos_y` (the
    native `dword[0x157750]`; the FU-75 §1.6 `ball.y` stand-in retired —
    **FU171-L3 closed**) and `s.is_ball_track`/`s.ball_track_side` come from
    `mr->render.camera.tracked` (the native `[0x1577CA]`; the controlled
    comparison survives only as the `TRACKED_NONE` fallback — **FU171-L2
    closed**). The native admission ripple is real: the long-ball claim now
    happens at `camY == 0` (`0x7EDD8` reads the live camera Y), and the
    pose-feed situation expectation moved to the native claimant's side.
    Tests: the re-staged `test_row04_live_pan_arms_camera` (mutation-RED),
    the new `test_row04_tracked_gate_suppresses_half_line` (the far-arm
    tracked gate; the near-arm coda is not tracked-gated — pinned) and
    `test_action_04_ball_track_side_from_tracked` (the side-compare RNG
    witness).
  - **The sustained window and pan-counter lockout pinned (FU171-L1
    closed).** The extended ISO `test_live_long_ball_mode40_band2_follow`
    walks granted frames from the receive event: while
    `pan_counter == 0` (the setter's `0x70153` clear) the every-frame follow
    re-seeds `vel = dirs x 15` and `pos_x` advances by `vel*delta`; the
    window is exactly **52 granted frames** for the k=7 `0x100` event at the
    pinned `0x70` camera Y (F2 = 59, F4 = 118, F6 = 14 — the FU-171 §3.3
    arithmetic), the first pan step (`0x70A65`) increments the counter and
    locks `fifa96_match_run_camera_follow` out, and a new setter call
    (+ the caller's `0x71D27` bind) re-opens it. Mutation-RED: disabling the
    increment breaks the window.
  - **Legs.** The camera-Y producer stays **FU168-L5 / FU171-L4** (not
    cheap: the row-1E keeper release is dispatch-dead R11-L4, the throw-in
    cut is restart-driven and re-anchors the chain — T3, and the
    `0x10F3C4` arc needs an existing event); the kick frame's Y is the
    disclosed pin. **FU171-L7 (new): the remaining `[0x157750]` `ball.y`
    stand-in sites** (row-05/06/08 + the record machine's chase gate). The
    throw-in cut's handler proof stands (`test_live_throw_in_camera_y_cut`).
  - **Goldens/smoke.** M1 `09b726b7…` / M2 `5f7e7692…` byte-identical;
    `make check` 109/109 (sanitized, strict). Smoke (`DISPLAY=:1`,
    `p14-t2-*`): see the phase-14 T2 report / PROGRESS.md — the visible
    sustained follow still needs the Y producer (gate = deepest provable +
    legs).
- **M2 phase-14 T3 (post-goal continuation — the restart chain + the
  celebration arm + the row-0x12 Tier-1 arms; FU-170/FU-172/FU-173;
  2026-10-12).** The match now continues past a goal on the tape: **M2
  re-pinned v13 -> v14 (additive; 85 -> 792 lines, the first 85
  byte-identical)**; M1 `09b726b7…` never moved; **wired 24/80 unchanged**
  (no action row joins — the restart rides the screen machine + the derived
  act-9 + the already-wired taker/relay rows), README title **30%**
  unchanged.
  - **The 3-link restart wire (FU-170 §7, R18; first-hand bytes re-verified
    this round: leg table `0x93B80`, id table `0x93B98`, act-9 stage table
    `0x88F3C`, act handler table `0x1107EC` [9]=`0x88F4C`, situation table
    `0x8A904` [0xC]=`0x8AF0E`, the `0x93C30`/`0x93C41`/`0x93CDD`/`0x93CE4`
    queues, the `0x93DCA` `[0x15B688]>0xB4` TAIL gate, the `0x8AF0E` act-9
    entry, the `0x890B1..0x890E3` stage-3 completion writing
    `[0x15882A]=1`, the `0x8AB08` formation-order gate and the `0x8ABF3`
    corner counter + `0x740A0(4)` arm).** Landed in
    `port/src/fifa96_engine/fifa96_match_run.c`: `MATCH_RUN_SCREEN_SETUP0` (leg
    0) now also queues sit 0xC (`set_piece`, the `0x93C41` call) and
    `MATCH_RUN_SCREEN_SETUP2` (leg 0) queues sit 3 (`0x93CE4`);
    `match_run_situation_side` gains the derived act-9 arm (stage-0
    `match_run_write_phase(0x15)` + `act9_armed`/`act9_timer`); the frame
    body ticks the act after `match_run_ball_machine` (the native act tick
    `0x8B672` after `0x8B664`) and the stage-3 timer (`word[0x158818] >=
    0x3C`, 30 granted frames) sets `global_5882a`. The sit-3 arm is gated on
    the derived **act-9 -> GATE handshake** (`act9_gate`): the native
    `[0x15882A]` gate's only reachable producer *inside the restart window*
    is act 9 stage 3 (`0x890C4`) — SETUP0 clears it first (`0x93C30`; legs
    1/3 write their `[0x15B684]`-selected value at
    `0x93E84`/`0x942F5`/`0x93EB6`/`0x94327`), stage 0 clears it again
    (`0x88F9A`) while writing phase 0x15, and the machine then waits with
    the singleton act slot holding act 9; the kickoff gate (act 1 sets
    `0x88F07` / clears `0x88E56`), act 2's tail (`0x895EC`), the reset
    (`0x886F4`) and staged values are the refused non-act-9 values, so a
    staged gate (the M2 v13 tape's m 42 compression — a native-unreachable
    state) advances without the corner arm; this disclosed hardening is what
    keeps the v13 strata byte-identical (the faithful SETUP2 measured taking
    the m 44 sit-3 route and moving frame 48+; see the T3 report). **Errata
    (phase-14 T4): the T3 report's "exactly one runtime producer" was
    falsified by fresh xrefs (24 touches, writers as above) — the
    window-scoped statement is the corrected form; FU170-L9's
    remove-and-re-pin criterion is unchanged.** Downstream
    needed no new code: the corner counter + phase 4 + the 0x11 arm, the
    taker ladder (`fifa96_match_action_11`), the situation-0xB resolve, the
    PHASE latch clear and the POST idle were all wired. The begin install's
    handler-run residue (the install-time sit-0xC -> phase 0x15) is
    superseded by a derived reset mirror (save/restore the phase cells; no
    setter edge) — the native front-end install precedes the match reset.
    Legs: **FU170-L1** (act-9 stage 1/2 gate fidelity: the derived route
    passes them in the arm frame), **FU170-L2** (stage-0 camera reset /
    presentations / the phase-0x15 arm — the record positions keep the
    pre-goal values into phase 4), **FU170-L3** (legs 1/2/4/5 re-queues),
    **FU170-L4** (act-4/act-8 dirty-counter path), **FU170-L5** (act-slot
    singleton vs `ref_machine`), **FU170-L6** (`word[0x15881A]` live
    semantics), **FU170-L7** (measurement), **FU170-L8** (timing
    compression sign-off), plus **FU170-L9 (new): the `act9_gate` hardening
    review** — the derived handshake is a documented divergence for the
    native-unreachable staged gate; if the tape is ever re-scoped (a
    non-additive v15), remove the gate and re-pin.
  - **The v14 tape strata.** `port/tests/test_engine_m2.c`: the post-goal break
    (`M2_POST_GOAL_FRAMES`) becomes the restart completion (phase 2 again +
    `screen_step == 4` + `corner_count[0] == 1`) + `M2_POST_RESTART_FRAMES`
    (20); new observations `phase15_step`/`phase4_step`/`restart_step`/
    `mask_restart`/`restart_corner`; the post-run mask is
    `M2_WIRED_MASK | M2_RESTART_ROWS_MASK` (row 0x11 — the first tape taker
    dispatch), the m 72 exact set unchanged; the state-line asserts gain
    `state=21/1-0`, `state=4/1-0` and the resumed `state=2/1-0`. Golden:
    v13 `5f7e7692…` (85 lines) -> v14
    `7b4b241a72c461a354121c53b900d8d11cf59f59f03495e08e5fa08b75e45da8`
    (792 lines); `head -85 new | cmp - old` clean; the first differing line
    is line 86 (the first appended). Windows: 0x15 lines 370..469, phase 4
    470..772, resumed phase 2 773..792. The run no longer stops at the hold
    (the v13 "restart producers are FU-146 leg-8 legs" narrative is
    superseded; no period-end exit on the extended path — the stoppage aux
    overshoots the limit so the class-1 completion cannot fire).
  - **The byte-0 live release (FU-173 §4.2/§6, R21-2; bounded landing).**
    (1) the scan's goal record is stored (`goal_record`/`goal_record_side`,
    the native `[0x157A9F]` cell; the goal arm is derived, the other three
    scanner arms stay **FU173-L1**); (2) `phase_arm(5)` lands the celebration
    install set (first-hand `0x8D433..0x8D4A1`: matching team -> code 0x15
    over 1..10 + code 0x14 + the `[team+0x7B2]` write on the goal record;
    other team -> code 0 over 0..10; a NONE record takes the port's
    no-record hardening). The release (row-0x14 stage 2's phase-1 write
    `0x87C71` + the shared kickoff entry) stays unported — **FU173-L2/L3**
    (the row-0x14/0x15 handlers are unwired, so the byte-0 celebration still
    needs the row port); **FU173-L4** (`FUN_00073E28` mid-match), L5
    (rollover timing), L6 (tape frame count), L7 (phase-arm coverage
    enumeration) stand. Tests: the extended `test_natural_goal_fallback_arm`
    (the scan store) and the new `test_phase5_celebration_arm` (both team
    shapes + the NONE hardening).
  - **The row-0x12 Tier-1 arms (FU-172 §7/§10, R20).** `match_run_record_jump_arm`
    gains `case 0x02` (the f3 airborne freeze-only twin of 0x2B: while
    `jump_timer < 2*jump_base` zero the frame timer; sustained — fresh
    `0x6E930..0x6E95F`) and `case 0x0A` (the f0/f1 landing tail `pos_y = 0;
    jump_timer = 0`; sustained — fresh case-index 9 -> `0x6EEB7`). Test:
    `test_jump_arm_f3_freeze_and_f0_landing` (freeze armed/landed + the
    stale-arc clear). Tier 2/3 stay legs (the call-site split touches
    successor staging the tape exercises; the fold producer, the `+0x3F`
    bias, the row-0x0C stake and the sinks): **FU172-L1..L10** stand
    (L7 closed by this landing; L2/L3/L4/L5/L6/L8/L9/L10 carried).
  - **Folded T2-review minors.** FU-171 T3 status (append-only): the T2
    status's cited window `0x739C0..0x73B6F` is corrected to the method
    record's `0x739B0..0x73B80` (it excluded its own `0x739BB` cite), and
    the pan step's second admission `0x737C8` (fresh bytes this round:
    `83 3d 5c 77 15 00 00 / 7e 0e` = `prevY > 0 && camY <= 0`) is cited.
  - **Goldens/smoke.** M1 `09b726b7…` byte-identical; M2 v14 as above;
    `make check` 109/109 (sanitized, strict). Smoke (`DISPLAY=:1`,
    `p14-t3-*`): the live match boots to the kickoff still (byte-identical
    to the T2 kickoff frame), dismisses to live play and runs continuously
    (00:02 -> 00:11 -> 00:24 match clock; `docs/screens/p14-t3-frontend.png`,
    `p14-t3-kickoff.png`, `p14-t3-live-play.png`, `p14-t3-play2.png`,
    `p14-t3-late.png`). **No live goal/post-goal segment exists in the
    smoke** (scoring is a gameplay outcome the port cannot force on the SDL
    session); the post-goal continuation's runtime evidence is the v14 tape
    + the fixtures.
- **M2 phase-15 T4 (acceptance v15 + the census close-out; 2026-10-12).**
  Docs/assertions only — **no behavior change**: M2 v15 **byte-identical** to
  the v14 golden (`7b4b241a…`, 792 lines; `./build/test_engine_m2 | cmp -`),
  M1 `09b726b7…` unmoved, `make check` **110/110** (sanitized, strict), wired
  **29/80 = 36%** and the README title **36%** unchanged. The phase is adopted
  as the v15 acceptance (`port/tests/test_engine_m2.c`): a strata-only layer (no
  re-pin — T1's act seam reproduced the exact FU-170 §7 act-9 cadence and
  T2/T3 are install-live/tape-dormant) with a provenance block plus the T1–T3
  regression **riders** as tape/strata-level cross-checks (`run_phase15_riders`,
  ISO-independent; **all seven riders mutation-RED**, each mutation aborting
  `run_phase15_riders` at its own assertion — the per-rider mutation is recorded
  in the phase-15 T4 report): the census shape (exact 28 action + 1
  phase, the phase-15 slots pinned so a silent re-wire/un-census/slot-flip
  fails the tape), the act seam + phase `0x1F` dispatch (stage-0 `[0x15882A]`
  clear), row `0x17`'s NULL slot + UNSUPPORTED dispatch (body reachable by
  direct call), the keeper-cluster install (`0x1F` stage-2 no-slot dive installs
  `0x1B` invoke-now) and the row-`0x19` tracked-opponent guard (`0x747EA` →
  the weighted fallback), row `09`'s arm-0 set branch (anim `0x11`) and row
  `0x0D`'s NULL slot, and the row-`0x1B` stage-0 six-store latch partial clear
  (`0x157C54/57/58/59/5A/5B` cleared, `0x157C55/56` surviving — FU180-L7's
  stage-2 rush gate; the `memset(sizeof)` form is the regression). **Census
  confirmed 29/80 = 36%** (28 action + the first honest phase row `0x1F`),
  mutually consistent across README/PROGRESS.md/here.
  - **Phase-15 legs (named owners).** The remaining legs each carry their owner
    (the phase-16 work that resolves it; R30/FU-182 §7 frontier). **Closed:**
    FU176-L1 (T2's `fifa96_keeper_arm_step` stage-3 off-by-one fix), FU180-L7
    (T3's six-store latch form + the v15 rider), FU175-L3 (T1's `mr->act`
    absorb). **Act-seam follow-on (phase-16):** FU175-L1 (the per-record
    `[rec+0x1C]` walker / record-threaded handler signature — unblocks rows
    03/`0x17` and the phase rows) and FU175-L5 (`word[0x15881A]` act-9 gate
    producer; FU165-L2 tie). **Row-03 (phase-16):** FU177-L1..L6 (`0x6DCC8`, the
    `0x158747` wall scan, `FUN_00079B58`, the phase-`0x13`/`0x14` arm, the
    `EBX=-1` third arg, the phase-8/0xC tails). **Row-09 arms (phase-16):**
    FU178-L1 (arms 1/2/3, holding the `0x0C`/`0x0D` installs — co-wires row
    `0x0D`) and FU178-L7 (the aliased descriptor reads). **Row-`0x19`
    remainder (phase-16):** FU179-L1 (predictor live leads) and FU179-L7 (the
    `FUN_00079B58` receiver-timer arms; also L2/L3 the projection, L4 the
    `0x79C20` clamp, L5 the `[rec+0x1C]` method tied to FU175-L1).
    **Keeper-chain (phase-16):** FU180-L4 (the `FUN_000782D0` record-0 machine),
    FU180-L5 (the `FUN_00077EAC` CPU decision) and FU180-L8 (the row-`0x0D`
    unported sinks — co-wire with FU178-L1). **Row-`0x17` producer
    (phase-16):** FU181-L1..L6 (the act-4/sit-2..4 formation-order arm + the
    act-9 stage 1/2 gates + the act-8 record pick). The R30-first frontier
    itself (rows `0x14`/`0x22`, then `0x1A`/`0x1C`, `0x16`, `0x24`, `0x20`,
    `0x25`) carries FU182-L1..L4. Full per-leg owner text in `docs/PROGRESS.md`.
  - **Smoke (`DISPLAY=:1`, `p15-t4-*`).** SDL3 present: the windowed game boots
    and walks intro → front-end → **match** (`docs/screens/p15-t4-intro.png`,
    `p15-t4-frontend.png`, `p15-t4-match.png`, `p15-t4-live.png`). The match
    frame is the same front-end/intro/match walkthrough as T2/T3 (mean 0.0687
    vs T3's 0.0684; stddev 0.117 — a real scene, not the black bridge) and no
    **live phase-15 row is visible** — the phase-15 rows are
    install-live/tape-dormant (the act seam + restart chain is tape-pinned), so
    no phase-15 visible delta is claimed. The exit log carries only the
    external SDL/dbus/nvidia LSan leaks (not a regression).
- **M2 phase-15 T1 (the native act/phase dispatch seam + row 0x17; FU-175 /
  FU-181; 2026-10-12).** The act seam (`FUN_000888FC` + the `0x8B669..0x8B672`
  clock-tail tick) is ported in `port/src/fifa96_engine/fifa96_match_act.c` and
  wired into `fifa96_match_run_frame`'s clock-tail slot; it gives
  `fifa96_match_dispatch_phase` a live caller and absorbs the FU-170 §7 ad-hoc
  act-9 singleton. Phase `0x1F` (= act 9 handler `0x88F4C`) is the first wired
  phase row; action row `0x17` (the line-up walk) has a **ported body but is
  not counted** (no engine installer — the rows-27/29/2C precedent: its only
  producer, act-4 stage 0, is behind the unported sit-2..4 formation-order arm,
  FU181-L1). **M1 `09b726b7…` / M2 `7b4b241a…` byte-identical** (the seam
  reproduces the exact FU-170 §7 act-9 cadence), so **no tape re-pin**;
  `make check` **110/110** (sanitized, strict; the new
  `test_engine_match_act`). **Census 24/80 → 25/80 = 31%** (README title
  recomputed). **Smoke (`DISPLAY=:1`, this session; honest):** the windowed
  game boots and draws the intro splash and the front-end menu
  (`docs/screens/p15-t1-intro.png`, `p15-t1-frontend.png`), but the
  post-confirm STATE16 bridge drew black in this session — the visible match
  was **not reached**, so no live gameplay reach is claimed; T1 is
  tape-dormant, so no visible delta is expected.
- **M2 phase-16 T1 (the act machine owns phase 0x18 / act 2; FU-183;
  2026-10-12).** The one genuinely producer-live unported phase row lands:
  phase `0x18` = the act-2 (`0x8922C`) free-kick/penalty hand-off, ported as
  `fifa96_match_action_phase_18` over the existing
  `match_run_ref_step_restart`. The ACT MACHINE owns the row (the sit-9 /
  sit-0xA producer dispatches act 2 invoke-now — the native
  `FUN_000888FC(2, stage, BX=1)` — and the clock tail ticks it), and the ad-hoc
  `ref_machine = REF_RESTART` route (the FU175-L7 double-run) is **retired**:
  the act slot is the single owner, so there is no double-run. The sit-2..4
  formation-order arm / row `0x17` producer chain is a proven **no-op** (its
  `FUN_0008C974` counter is fed only by the unported `0x14C360`/`0x157BD2`
  matrices) and is recorded as numbered legs — row `0x17` stays NULL and
  uncounted (FU181-L1), no dead arm code. **M1 `09b726b7…` / M2 `7b4b241a…`
  byte-identical** (the row reproduces the retired route's exact same-frame
  stage-0 + tick advance, and the restart producer is off the tape), so **no
  tape re-pin**; `make check` **110/110** (sanitized, strict; the new
  `test_engine_match_act` phase-0x18 tests + the updated restart tests).
  **Census 29/80 → 30/80 = 37%** (README title recomputed). **Smoke
  (`DISPLAY=:1`, this session; honest):** the windowed game boots, draws the
  intro splash and the front-end menu (`docs/screens/p16-t1-intro.png`,
  `p16-t1-frontend.png`); the row is install-live/tape-dormant (M2
  byte-identical), so no visible delta is claimed. The exit log carries only
  the external SDL/dbus/nvidia LSan leaks (not a regression).
- **M2 phase-14 T4 (acceptance v14 completion + the T1–T3 review riders;
  2026-10-12).** Docs/comments + test strata only — **no behavior change**:
  M2 v14 byte-exact (`./build/test_engine_m2 | cmp -`), M1 `09b726b7…`
  unmoved, `make check` 109/109 (sanitized, strict), wired **24/80** and
  README title **30%** unchanged.
  - **The v14 acceptance strata complete** (`port/tests/test_engine_m2.c`): the
    new per-window state pins in the existing strata style (no golden
    duplication) — the 0x15 window entry carries the derived act-9 armed
    with the gate just cleared (`phase15_armed` 1 / `phase15_gate` 0), the
    GATE production lands 96 steps in (`gate_marker` 1, the arm spent), the
    sit-3 phase-4 entry 100 steps in carries `corner_count[0] == 1` and
    leaves the screen machine holding at its PHASE step (3), the taker
    ladder runs 303 steps, the restart completes at the POST idle with the
    latch clear (`restart_screen_step` 4 / `restart_pending` 0) and the tape
    records `M2_POST_RESTART_FRAMES`. The T3 ordering pins, the m 72 exact
    set and the ISO/noiso paths are retained. Mutation-RED verified:
    bypassing the PHASE hold fails `phase4_screen_step == 3`; shifting the
    stage-3 threshold three steps fails `gate_step - phase15_step == 96`.
  - **T3 review Important 1 (the falsified producer claim).** The
    `act9_gate` rationale is reworded window-scoped at the header, the
    SETUP2 body, the test provenance, here, PROGRESS.md and the FU-170 T4
    append: the native `[0x15882A]` gate has multiple writers (fresh
    `get_xrefs_to` = 24; act-1 `0x88E56`/`0x88F07`, act-2 tail `0x895EC`,
    act-7 `0x899F0`/`0x89A39`, act-9 stage-0 `0x88F9A` / stage-3 `0x890C4`,
    the reset `0x886F4`, SETUP0 leg 0 `0x93C30` and legs 1/3
    `0x93E84`/`0x942F5`/`0x93EB6`/`0x94327`) — the correct statement is that
    *inside the restart window* only the act-9 stage-3 completion can open
    it (the singleton act slot holds act 9 while the machine waits at step
    1). FU170-L9's remove-and-re-pin criterion stands.
  - **T3 review minors.** (i) **Errata pointer** (old line not rewritten):
    the "pins the 85-frame M2-B transcript" figure in the phase-10-era
    acceptance list above is superseded — the tape lineage is v10 85 -> v13
    85 (additive) -> **v14 792 lines**. (ii) The phase-5 arm's else path
    drops the trailing native presentation sink `0x8D490 CALL 0x513BC`
    (EAX=0 -> `0x513CC CALL 0x566D0`, the `[0x108E1C]` cell; fresh bytes) —
    named in the `fifa96_match_run.c` comment and minted **FU173-L8**.
    (iii) The begin reset mirror's derived ordering equivalence is minted
    **FU170-L10** (no native run measures the front-end-install vs
    reset/kickoff order).
  - **Smoke (`DISPLAY=:1`, `p14-t4-*`).** Intro -> menu panel -> match: the
    kickoff still is **byte-identical to the T3 kickoff shot** (`cmp` clean;
    and T3 verified that one byte-identical to T2's), so the still needs a
    *held* gameplay key to dismiss (a tap is missed between 30 Hz polls; the
    pre-existing freeze, not a regression), after which the live match runs
    continuously (00:02 -> 00:05 -> 00:31;
    `docs/screens/p14-t4-{frontend,kickoff,live-play,play2,late}.png`). **No
    live goal/post-goal segment exists in the smoke** (scoring cannot be
    forced on the SDL session); the continuation's runtime evidence is the
    v14 tape + the fixtures.
- **M2 phase-14 T1 (row-0x0E stage-2 else arm + the live row-0x5E installer;
  FU-167/168/169; 2026-10-12).** Two pieces landed; **wired 22 -> 24/80**
  (dispatch 55 UNSUP / 24 OK / 1 NOTF); README title recomputed to **30%** in
  the same commit:
  - **The row-0x0E stage-2 else arm (FU160-L4 closed; FU-167 §6 rank-1,
    FU-168).** The missing `0x82820..0x82A9E` block now runs inside
    `fifa96_match_action_0E` (`match_sp_pan_arm`): the strict gates
    `0x20 < camera.pos_y < 0x70` and `(int16)word[+0x6B] < 0x60` (the live
    pool `lane_x`; FU-168 §9.4's "(or pool `lane_x`)" — the `e->lane` dword
    high word is a separate stale view, FU168-L7(d)), the focus re-placement
    `pos + 0x40*dir[type]` as the direct camera write (not the `place_*`
    drain — FU-168 §7.2 hazard), the slot-dir/RNG lateral seed, the dx/dz
    pair + `0xCD474` turn clamp, the amplitude (`2*speed71 + cam speed`
    clamped `0x1E` x the height factor clamped `0xA`, `<= 0x780`), the polar
    vector (`0x795A4` products), and the `0x71C94` event through the split
    `fifa96_camera_event_set` + `fifa96_camera_set_tracked` pair — the bind
    `[0x1577CA] = rec` plus the record's `+0x9B` clear (`0x71D2D`) is the
    live follow trigger; a gate failure leaves the record at stage 2 (the
    native `CL = 0` retry semantics), success advances `timer89 = 0;
    stage -> 3` in the same dispatch. Legs: the `0x92820`/`0x8F188`/`0x651F0`/
    `0x974F0` sinks (FU167-L3), the descriptor term (FU167-L1), the
    `[0x157815]` byte (FU167-L2), the `0x71C94` tail cells (FU167-L5) and
    the live mid-band camera-Y producer (FU168-L5). Tests:
    `test_action_0e_stage2_else_arm_binds_camera` (focus/bind/event/advance),
    `test_action_0e_stage2_else_arm_gate_retry` (both camY bounds, the lane
    bound, the tracked and jump-base skips) and the extended ISO
    `test_live_row_0e_selector_installs_jump` (the live tracked bind + the
    stage-3 advance on the row-0x12 window).
  - **The live row-0x5E installer chain (FU-169 rec #1, full).** Two new
    bodies: `fifa96_match_action_0B` (the duel body `0x81908..0x81C71`: arm
    0's derived point + nearest opponent + strict `(0x20,0x70)` band + the
    speed/facing `{0x0C,0x59}` choice + the `0x79B6C` snap/face + the anim
    install on self; arm 1's frame gate, band `<= 0x60`, `[0x14C32A]`
    session gate, the `+0x7D7` duel counter, the kind-2 registrar, the
    `+0x8E` type copy and the action-0x0C install on the found record; the
    same-dispatch arm-2 snap/`+0x44` reset — the native fall-through) and
    `fifa96_match_action_0C` (head + stage 0 `0x81CDC..0x81DC5`: the active
    reset, A = `[0x15888F]`, the A anim-0x59/frame-`>= 2` gate, the `0x795B4`
    lead band (`A.pos + 2*A.vel`) `>= 0x30` -> the `+0x7CF` stumble counter,
    the anim row 0x5E install on A and the self reset; the `< 0x30` and
    alternate-gate foul branches run `fifa96_match_run_contact` — the
    disclosed kind-1 re-register — after the `0x81E13` B velocity write).
    The new per-team `duel_7d7`/`stumble_7cf` counters are staged into the
    row-04 `team_7d7`/`opp_7d7` inputs (the native `[team+0x7D7]` gate).
    Tests: `test_action_0b_duel_chain_installs_row_5e` (the full unit chain
    0x0B arm0 -> arm1 -> 0x0C stage 0 -> anim 0x5E on A -> the successor
    0x5E -> 0x0F -> 0x00 + the terminal `+0x44`), the gate/choice and
    foul-path pins. **Erratum (fresh bytes):** arm 1's `choice == 0x59`
    compare (`0x81B63`) reads the local zeroed at the body head (`0x8191D`)
    on every entry, so the port's gate is the `<= 0x60` band alone. Legs:
    the ring-post `[[rec+4]] == 0x18DE` (FU169-L3), the events/sinks/`0x8ED40`
    (FU169-L2), the `0x0C` stages 1..6 + the retry exit (FU169-L4), the
    action-0x09 `0x0C` producer (FU169-L1), the timing window (FU169-L5) and
    the `0x79B6C` anim re-anchor/transient row install (FU169-L8 new).
  - **Goldens**: M1 `09b726b7…` byte-identical (immovable); M2 `5f7e7692…`
    byte-identical (no re-pin — the tape never dispatches 0x0E/0x0B/0x0C).
    `make check` 109/109 (sanitized, strict). **Smoke** (`DISPLAY=:1`,
    `p14-t1-*`): the live match runs with the new bodies — the kickoff draws
    the players + ball (`docs/screens/p14-t1-kickoff.png`, 0-0/00:00) and a
    KICK burst starts the clock with the records moving
    (`docs/screens/p14-t1-live-play.png`, 00:03, the ball leaves the fixed
    view — the R2-L4/T2 follow leg, unchanged). The duel/0x5E chain's runtime
    evidence is the dispatch-chain tests (the pad producer needs the
    PASS+LONG_BALL bits and a band/RNG window).
  - **Fix round 1 (2026-10-12, review).** Two must-fixes in the 0x0C port:
    (i) the foul-path velocity's sine input is **A's** face (`0x81DFE` loads A
    into EAX, `0x81E13` reads `word[A+0x7D]`; the magnitude `0x81E36` is B's
    `speed71`) — FU-160 §1.1's "EBP's facing-relative +0x73/+0x75 words"
    erratum appended there; the test stages A face `0x100` vs B `0` (mutation
    RED-verified); (ii) the foul tail `0x81ED5..0x81F48` is ported —
    `byte[B+0x98] = 1`, one RNG draw -> anim row 0x0D/0x52 on B, `ran`,
    `timer89 = 0` and the **stage 0 -> 1** advance (otherwise B re-fouled
    every frame); the whistle `0x974DC(0x1F)` + the `0x8DDE0` re-pick are the
    new leg FU169-L9. The re-call cite is corrected to `0x81E8A..0x81ED4` and
    FU169-L4's stage range re-verified as `0x81F4E..0x8250B` (the doc's
    `..0x8251B` over-runs into the next body). The retry test gained the
    discriminating completing half (arm-disabled mutation RED-verified).
    `make check` 109/109; M1/M2 byte-exact; census 24/80 unchanged.
- **M2 phase-7 P4 (presentation residual, FU-152; 2026-10-09).** Landed from
  the frozen FU-152 slice (`fifa96_match_run_render` rows R1/R2/R3 + the R5
  dormancy pin, the camera handler bodies, the FU-71 residual helpers, the
  palette pool identity):
  - **R1 sub strip** (`fifa96_window_strip_layout`, `fifa96_font_draw_centered`
    /`fifa96_font_blit_outlined`, `fifa96_match_run_sub_mark`/
    `_sub_numbers`): the FUN_00053240 0x14E53C row layout (settings-4 "wide"
    doubling, `(v*scale+0x8000)>>16`), the FUN_00054640 centre (narrow
    `0xA0-((w>>1)*scale)`, wide `0x140-((w>>2)*scale)`), the FUN_0004BD38
    mark states and FUN_0004BDF8 numbers; the engine draws the pair, a staged
    mark glyph and the row-clamped names.
  - **R2 replay row** (`fifa96_match_run_replay_row`/`_step`/`_blink_step`/
    `_progress`/`_camera_set`): the FUN_000565BC gate chain, the reachable
    FUN_000642FC button subset incl. the 0x80->0x81 arm, FUN_000564A0's blink
    and FUN_000642B0's progress; FUN_0004D134's camera index map. Captions/
    glyphs/bar assets stay runtime legs 1/2/3.
  - **R3 overlay** (`fifa96_match_run_overlay_row`/`_arm`/`_timeout_step`/
    `_visible`): the FUN_000550E4 case table, the `id|0x8000` arm + second
    overlay seeds, FUN_000542D4/FUN_00053E08 timeout flips; the per-case
    helper layouts stay leg 7 (lines staged).
  - **R5 ball row**: dormant pinned (`render.ball_row` NULL; FUN_00056690's
    only caller passes EDX=0, disasm 0x56E1B..0x56E41).
  - **Camera handlers** (`fifa96_camera_type`, `fifa96_camera_behavior_
    steady/sidetrack/staged/action`): the 0x107508 `{+4 class, +8 handler
    selector, +0xC behavior}` mapping (3->5 remap, negative clamp; the image
    -1 cells are the degenerate 0x108B60 read, engine-clamped) and the quoted
    clamp/constant level of the four 0x108B80 bodies plus the pinned 0x108B64
    mode table
    (`fifa96_camera_behavior_blocks`: classes {3,1,3,3,1,3}, class-gate cells
    the dword +0xC {0,0,0xA7F8,0,0,0xA21C} — blocks 2/5 take FUN_0004EC9C's
    class-3 arm), wired into the FUN_000505D0 default arm through the staged
    record subset + behavior block (NULL = unported default, tape unchanged).
    The class-0 steady body is complete: the FUN_0004E248 pan step, the
    FUN_0004D698 slides (strict inside block rate / outside rate 8), FUN_0004DF34
    yaw, the 0x4E9A2 yaw snap, FUN_0004E05C pitch, the y/ratio eases and the
    history copy are ported (FU-351 §12). The class-1 sidetrack and class-2
    staged bodies are complete too: FUN_0004E3A8's projected yaw point,
    four snap arms, class-2/else pitch target and reversed x-slide bounds;
    FUN_0004EC9C's FUN_000A1A60 sin/cos scale, stored class gate, pan-cell
    clamp, x/z slide split, four snap arms and ±0x1620 pitch target; the
    +0x50 sub-record +0x10/+0x14 cells are in the block model (FU-351 §13).
    The live render camera now consumes `fifa96_camera_view_select`: phase 2
    resolves to preset 0 / slot 1 / class 1, slots 0/1/2 run the completed
    steady/sidetrack/staged bodies with persistent per-class record state and
    the 20-step view-change settle; slot 5 keeps the class-3 action fallback
    (FU-351 §14). The FU-71 cursor threshold now uses the native
    `[0x14C2F6] != 0` selector (0x70/0x90), the `FUN_0004CE34` overlay/view-mode
    gates are staged, the `[0x107DD4]` variant selects the 0x2EE pitch arm and
    the active view rows 3/4/5 plus the `FUN_0004F1C8` C-record overlay, and
    the per-frame target takes FUN_000365B0's image/default `[0x109A70]` source
    with FUN_00036544's +-0x720/+-0xB10 clamp (FU-351 §15). The pose-feed
    computed-call arguments are closed: FUN_0004B8D4's side axis plus the
    `0x1577C2` lead word drive `FUN_0004E248`, and the `0x51047` ECX guard
    is the staged `handler_skip` (FU-351 §16). The replay camera blend body is
    ported too: `FUN_0004CF60`'s camera snapshot, `FUN_0004F8C8`/
    `FUN_0004F830` with the `0x4CF7C` 100/300-tick start, six-field deltas,
    finish latch and staged cancel gate (FU-351 §17); the `0x3BB1C` call site
    and `0x505D0` config-record switch remain open.
  - **FU-71 residual** (`fifa96_camera_classify` = FUN_00070074 full bits,
    `fifa96_camera_pan_band` = FUN_000709D0 ladder,
    `fifa96_camera_rate_event` = FUN_00071DF4 first arm): the FUN_00070DE0
    stepping/sound body, the 71DF4 table arm and the octant-dispatch atan2
    (FUN_000CD474) stay OL-T11-79 legs.
  - **Palette pool identity** (`fifa96_palette_pool_identity` +
    `fifa96_palette_pool_create/release`): request size 0x34E8 / type 0x220 /
    tag "palettes" pinned; the content producer (which file fills `[0x107290]`)
    stays OL-T11-80 and the shade cube stays OL-T11-82.
  **Both goldens byte-identical, no re-pin** (all new rows are zero-gated at
  rest: `replay.state` 0, `sub.active` 0, `overlay.armed` 0, `ball_row` NULL;
  `make check` 108/108 with the new row/camera/palette assertions). FU-152 §8
  records the first-hand errata (settings-4 "wide", the signed FUN_00053D58
  compare, the wide centring quarter, the FUN_0004BDF8 +0x10/+0x11 bytes) and
  the post-P4 leg ledger; 15/80 action rows unchanged (presentation rows are
  outside the action dispatch).
- **M2 full-gameplay phase-6 close-out (S5 acceptance; 2026-10-09).** Whole-
  range summary: Phase 0 (follow-up-5 close-out: T2 review/fix, HUD P0.2,
  acceptance v5 P0.3) + wave-1 recon (FU-145/146/147/148 frozen) + wave-2 ports
  S1–S4 (serialized, each reviewed with a bounded fix loop) + S5 (acceptance
  v6, the smoke above, these docs). Phase-6 gate status against the plan
  `2026-10-08-fifa96-m2-full-gameplay.md` — milestone DoD reached at the
  deepest provable chain: **G1 RGB** (palette `OL-T11-6` live; tape frame-6
  chunk-entry + canvas-level RGB assertions; smoke `#E044A0`), **G2 live
  progression + score source** (natural kickoff→phase-2 row 01, the FU-143
  phase driver's live period end 2 → 0x0C and the FU-72 `score_event` source
  wired — the direct score step lands at frame 67 (the first `state=2/1-0`)
  and frame 145 is the last live frame (the class-1 period end); the S2/S3
  goal chain landed but tape-dormant, fixture-proven), **G3 presentation
  carried** (HUD
  `OL-T11-7`, camera place v4.2, the S4 pose/formation/palette seams landed;
  kickoff framing + the natural pan origin carried on numbered legs), **G4
  acceptance** (tape v6 green — byte-identical golden, assertion layer only —
  plus the S5 smoke). `make check` 106/106 with ASan/UBSan; M1 immovable
  (`09b726b7…`). Re-pin ledger (M2 only; M1 never re-pinned): T1 drawing (157
  hash lines from frame 9), follow-up-4 palette (160 from frame 6), v4.1 pad
  locomotion (107 from frame 59), v4.2 camera place (117 from frame 49), v5 HUD
  (160 from frame 6), S1 v5.1 possession (157 hash-only lines from frame 9) —
  all hash-only with no `state=` suffix moved; S2, S3, S4 and S5 are
  byte-identical (no re-pin). Current M2 sha `2e709151…`. Leg register added or
  kept open this wave: S1/FU-147 §8/§9 legs (row-1E stage gates, install flag,
  reset-lane path, BF20 visibility arm, `[0x157AB2]`, driver pre-pass
  `FUN_0008D824`, `0x795B4` metric, E244 formation context) with leg 13 live
  (camera-focus stand-in); S2/FU-145 L1 (pan source — S4 closed the producer
  level; the natural invoker stays absent) and L2–L8; S3/FU-146 legs 1–10
  (handler presentation bodies, mode/side/leg arg producers, `[0x14C32A]`,
  tracked-side flags, probe cells, `FUN_0009252C` display gate, `FUN_000935A0`
  thresholds, `[0x15B684]`); S4 **OL-T11-76…82** (camera handlers/mode writers,
  selector-3 preamble arrays, FU-71 tails, palette-pool identity, team-record
  producer, shade cube); plus carried `OL-T2-1…3`, `OL-T4-1` (**closed by
  phase-8 T3**: the SDL backend presents a held-key state sample; the pad kick
  is wired through the input-row seam — see the phase-8 T3 entry),
  `OL-84`'s remaining situation-0xB producers / row-01 sinks, `OL-85/86`,
  `OL-87/88/89` (**closed by S2/S3/T2/T4** — see the phase-8 T4 entry and
  FU-142 §L.10 for the residual legs),
  the `OL-62…OL-83` residuals and the wave-7 B1–B4 phase-7 clusters (set
  pieces/restarts, fouls/referee, keeper+AI, presentation residual). Deferred
  minors roll-up (recorded for triage; doc/optional unless noted): P0.2 —
  `FUN_000490f4` translation install identity stand-in (restated as
  OL-T11-80), bar index-0 transparent convention, font depth/`0x7F` leniency,
  MTNF variant listing; S1 — tape header v5.1 provenance and row-00 reset
  wording (both closed by v6), FU-147 §8 legs sentence, `lane_z` naming; S2 —
  disarmed-reflect merge implication when a `render.input_bit0` producer lands,
  pre-existing 32-bit camera-reflect abs; S3 — 32-bit signed vs `uint16`
  duration cells (contract note), installer-only cells unmodelled
  (native-zero); S4 — all review minors fixed in the fix round. The
  goal-handler presentation/restart tail, the 15 s screen-duration rollover
  (S3 report §8.2; first fidelity gap in long natural runs) and the pan origin
  are the phase-7 openings.
- **M2 interactive T1 (pad-driven locomotion / G1) legs.** The derived setup
  bind models the engine's single human slot: the native four `0x4C1E0` mode
  rows are unported (derived default mode 0 = the controlled side), the
  `FUN_0008DB6C` sort/tie order of the free-record pick is substituted by the
  shared `fifa96_entity_find_nearest` (`FUN_000A1860` order is the leg; the
  no-candidate fallback `0x8DC1B` is bind leg 7). The AI-side mover
  integration and the row-04 `+0x6B` lane / `+0x8D` active producers **landed
  in S1** (see the FU-147 entry above), so the natural phase-2 pad path is no
  longer producer-blocked; the reachable pad consumer is row 00's slot-dir arm
  (wired) plus row 01's stage-1
  `word[slot+6] & 0x70` release gate (now wired to the live FU-70 release
  word) and the stage-2 conditional `FUN_0007876C` merge (live now that the
  bind increments `+0x828`; consumed by the frame drain). The mover's `+0x6F`
  stride rate, `+0x43` direct-face and `0x57A73` point inputs are staged zero.
  The word views `speed71`/`vel73`/`vel75` and the dword aliases
  `vel_x`/`vel_z` are kept in lockstep: the mover recomposes the dwords from
  the words, the dispatch decomposes arm-dword writes back onto the words, and
  the `FUN_00079B6C` commit zeroes the three words (T1 review fix; M2 golden
  unmoved by it).
- **M2 interactive T2 (per-record camera place `FUN_00079F3C` / G2).** The
  derived place runs at begin after the formation seed and before the
  `FUN_00079B6C` commit
  (`fifa96_match_entities_camera_place(pool, controlled_side, phase, cam_x,
  cam_z)`): per record of the non-controlled team (`byte[team+0x826] !=
  [0x157AAC]>>24`) with the live phase gate `byte[0x1106C3 + phase] != 0`, the
  `FUN_0008DCD4` octagonal camera distance is compared to `0x180` and a record
  at or inside the ring has its target snapped onto the `0x180` ring along its
  existing direction through the native `FUN_000CD474` angle, the `0x114E04`
  sine fold and the `FUN_000795A4` `(a*b + 0x8000) >> 16` multiply. The
  primitive ports are shared with FU-141 (`fifa96_arm_dist_stage`,
  `fifa96_entity_angle`, `fifa96_entity_sine`). The M2 golden is re-pinned
  (v4.2): the non-controlled side's in-ring records 9/10 move
  `(228,264) -> (251,291)` / `(-228,264) -> (-251,291)` before the commit,
  lines 1..48 stay byte-identical (those records project off-canvas at both
  radii — the place preserves the screen direction), the mechanics entry at
  frame 49 is the first differing line and 117 hash lines differ with no
  `state=` suffix moved. The place preserves the target's camera direction, so
  it cannot move the controlled side out from behind the camera; the one-sided
  kickoff draw is the **engine's stand-in view** (yaw/pitch 0), and native
  kickoff framing is carried on FU-96 legs 1/3 (the camera-mode/angle feed
  `[0x14E57C]`/`FUN_000505D0` presets `0x108B64` handlers `0x108B80`) plus the
  FU-71 follow writer `FUN_00071C94`, so T2 corrects the
  earlier "both sides draw at kickoff" expectation to "both teams placed where
  the native places them" — the visible place effect is fixtured where it is
  on-canvas (`test_camera_place_moves_near_record_into_frame`: an in-ring record
  below the `0x78` near gate moves onto the ring and draws). Without the
  formation resource the place is skipped so the documented zero-target
  degradation is preserved. Carried (the T2 report's numbered legs): **OL-T2-1**
  the phase gate's `>= 0x1D` range (the native reads the adjacent
  action-pointer table), **OL-T2-2** the camera-mode/angle feed (FU-96 legs 1/3)
  and **OL-T2-3** the FU-71 follow writer that moves the camera during live
  play; the other `FUN_00073E08` call sites (set-piece/event situations, eight
  sites) run the place natively too while the engine wires only the kickoff
  path.
- **M2 interactive leg register / follow-up-5 close-out (2026-10-09).**
  Landed: T1 pad-driven locomotion (G1; the setup slot bind + the
  `FUN_0007876C` merge + the FU-77 mover for the slot-bound record, golden
  v4.1), T2 per-record camera place (G2; `FUN_00079F3C`, golden v4.2; kickoff
  framing carried on the OL-T2-1..3 legs above) and T3 the match HUD
  (`OL-T11-7`, landed under the phase-6 P0.2 wave, golden v5). The plan's
  whole-range review: G1 **pass/complete** after the T1 review fix round
  (word/dword velocity lockstep, `120260a`; headless input moves the
  controlled record —
  `test_engine_match_frame::test_pad_drives_controlled_locomotion`; the tape
  pins the position move + lockstep), G2 **closed carried-with-legs** (the
  place is arithmetic-exact and fixtured; the "both sides frame at kickoff"
  expectation is corrected to the engine stand-in view, native framing
  carried on OL-T2-2/OL-T2-3; the HUD landed with `OL-T11-71`…`OL-T11-75`
  remaining — glyph ramp, team names, extra-time, gate writers, bar scaler)
  and G3 **accepted** by T4 (tape v5 green with the byte-identical golden and
  the T4 smoke table above). T1's remaining legs: the four
  `0x4C1E0` mode rows, the `FUN_0008DB6C` sort/tie substitution, bind leg 7
  (`0x8DC1B`) and the mover's staged-zero `+0x6F`/`+0x43`/`0x57A73` inputs
  (FU-77 errata + `test_engine_m2.c` v4.1 provenance); the AI-side mover
  integration and the row-04 `+0x6B` lane-word / `+0x8D` active-seed producers
  **landed in M2 full-gameplay S1** (FU-147). **OL-T4-1 landed in phase-8 T3**
  (2026-10-09): `platform_sdl3.c` keeps the pressed set and presents each held
  key as a per-poll state sample, so a held direction key reaches the engine's
  per-poll input model as a true hold; the same task wired the pad kick
  (carrier + KICK/PASS release → the `0x7D110` code-1 row → action 7) through
  the `FUN_0007CA54` input-row seam (see the phase-8 T3 known-gaps entry; the
  live smoke movement stays blocked by the unported machine forced decision /
  row 02, legs L4.1/L4.6). The goal chain that blocked the tape's score step
  (`OL-87`/`OL-88`/`OL-89`) was closed in phase-8 T2/T4 (see the T4 entry);
  the tape's goal step stays the direct writer call because no tape record
  reaches the half-line pan band (FU-142 §L.10).
- **M2 phase-7 P1 (set pieces & restarts, FU-149; 2026-10-09).** Landed from
  the frozen FU-149 slice: the `FUN_0008A938` dispatcher head as
  `fifa96_match_run_set_piece(mr, situation, side, bx)` — the head gates
  (sit 0/0xB, closed `session_gate_14c32a`, pending), the `0x8A8E0` table-1
  queue ids (sit 2/3/4 → 9 side 0 / 0 side 1; 5/7 → 7; 6 → 5/6; 9/10 →
  1 side 1 / 2 side 0; sit 1, 8, 0xC and >10 → the `0x8AA60` id 0xA), the
  `[0x15B6B8]` side latch (`sit_side_pending`), the BX!=0 fallback (phase
  `FUN_000740A0(0, 0)` + the `[0x15882B]`/`[0x15882C]` act-8 replay bytes +
  act 8's byte-exact re-dispatch with BX=0 and `[0x15882B] = 0xFF`) and the
  BX==0 table-2 route; the `FUN_0008D098` phase arm
  (`fifa96_match_run_phase_arm`) for phases 3/4/6/7/8/9/0xD — install 3 over
  both teams, the controlled-side gate `[0x157AAC]` with the non-controlled
  early return, the `FUN_00079CCC` pick (pool nearest over record targets,
  skip 0, team-base fallback), the camera resets (snapshot for 3, the
  `FUN_0007D360` corner probe `(±0x710, 0, ±0xB00)` for 4, the
  `FUN_00073DC4` penalty spot `(0, 0, ±0x8D0)` for 6), the taker/keeper codes
  0x10/0x11/0x12/0x13/0x1D/0x1E/0x1F (phase 6 non-controlled 0x1F on its
  record 0) and the phase-0xD code-0 installs (no install-3 prefix,
  first-hand); the scanner restart arms (`0x88BCC` throw-in sit 2, BX=1;
  `0x88B53` corner/goal-kick sit `3 + ((snap z < 0) == (ball == 1))`, BX=0;
  both phase-2 only, `EDX = ball_team ^ 1` under the pool `[0x1577CA]`
  stand-in = controlled actor, then ball carrier); the corner counter
  `corner_count[side ^ side_swap]` (`[0x157AD4]/[0x157AD6]`, zeroed by
  begin, write-only statically — FU-149 L10) and the loader row's
  `corner_increment` request. `fifa96_match_run_situation` stays the shared
  table-2 entry (row 01's 0xB; the goal fallback delegates through
  `fifa96_match_run_goal_queue` = `set_piece(6, side, 0)`) and now runs the
  phase arm after the write. No live producer calls sit 9/0xA yet (FU-150/P2),
  so the free-kick/penalty arms are fixture-dormant but derived. **Both
  goldens byte-identical, no re-pin** (the tape camera never arms a set piece
  and the forced phases bypass the situation seam; `cmp` clean, M1 unmoved);
  `make check` 106/106, ASan/UBSan on the frame suite. Carried legs (FU-149
  §6): L1–L11 as before plus **L12** (the sit 2..4 `[0x157B8E]`/`[0x157B8F]`
  formation-order gate + act-4 arm and the sit-1 arm; the port routes the
  table-2 row) and **L13** (the taker/keeper rows 0x10/0x11/0x1D — the arms
  install the codes but the stage machines are unported, so the §1.6 resume
  tails are not yet executable; FU-151 owns 0x1D). First-hand erratum: the
  incident z cell is `0x15889F` (`FUN_00079CCC` reads +8), not the slice's
  `0x15889B` (the y dword).
- **M2 phase-7 P2 (fouls / referee / offside, FU-150; 2026-10-09).** Landed
  from the frozen FU-150 slice as the `fifa96_referee` loader module
  (`fifa96_ref_contact_register` = `FUN_0008A3FC`; `fifa96_ref_foul_decide` =
  the `FUN_0008A43C` normal path with the `[0x14C306]` level, the staged
  duel/active preconditions, the severity RNG bands and the kind-0 → sit-9 /
  kind!=0 → foul-log + act-3 fork; `fifa96_ref_offside_check` = the
  `FUN_00079D5C` inequalities; `fifa96_ref_offside_event` = the kind-3 arm;
  `fifa96_ref_foul_sequence_step` = the 7-stage phase-0x19 machine;
  `fifa96_ref_offside_sequence_step` = the 3-stage phase-0x1C machine) plus
  the engine seam: the staged `fifa96_match_config` (init zero; begin installs
  the FU-68 default-settings handoff — foul level 2, offside off),
  `fifa96_match_run_contact` (the registrar + row-0x0C re-call: the settings
  gate, the 1-in-8 skip draw, the literal kind 1, the severity draw, the
  `[0x15888E]` flag; ACT3 runs stage 0 immediately, kind 0 routes situation 9
  BX=1 through the P1 `set_piece`), `fifa96_match_run_offside_reception` (the
  derived pool nearest queries — own team to the ball triple as the `0x157770`
  stand-in, opponent to `(0, ±0xB10)` — the staged metric/camera/mirror inputs,
  the tolerance draw and the kind-3 event on the own-nearest record),
  `fifa96_match_run_referee_step` (one step per granted frame; applies the
  whistle/speech requests, the phase write + FU-149 arm, the rec_first
  install and the situation dispatch, and runs the derived act-2
  free-kick/penalty hand-off: phase 0xA on the fouled side, then phase 7
  default / phase 6 for `contact_kind != 3`, `|incident x| < 0x420` and the
  fouler-side z-band `[-0xB10,-0x7B0]`/`[0x7B0,0xB10]`, with speech
  0x23/0x2A and the phase-7/6 taker arm). **Both goldens byte-identical, no
  re-pin** (no live producer calls the staged entries and the stepper is idle
  in the tape; `cmp` clean, M1 unmoved); `make check` 107/107 (106 + the new
  `test_referee`), ASan/UBSan on both suites. First-hand errata in FU-150 §Port
  landing: the offside side gate is the metric block `[+4]` (not
  `rec+0x69>>16`), the kind-3 event record is the own-team nearest (not the
  receiver), and the referee-object wait proceeds when the code is **not**
  0x48. The P1-review carry-in is recorded: the sit-9/0xA hand-off runs the
  native phase-0/0xA `FUN_0008D098` arms' code-0 installs only as far as the P1
  phase-arm subset (3/4/6/7/8/9/0xD), so the left-behind code-0 installs stay
  the FU-83 `0x8D192` body; the FK/penalty arms are installed. The
  `FUN_000740A0` side byte (the `[0x157AAF]` high byte of `[0x157AAC]`) is
  staged into `phase_machine.side_controlled` before each referee phase arm, so
  the phase-7/6 taker installs on the fouled side. Carried legs:
  FU-150 §Port landing legs table (settings labels, RNG identity, whistle
  mapping, referee identity, team-count predicate, downed/sent-off, `0x15888E`
  lifecycle, offside geometry inputs, row-0x11 writer, `FUN_0004BEC8`/`0x6E724`,
  foul-log consumers, the foul-log `rec[+4]` → `fouler->id` stand-in and the
  dropped stage-4 `0x14C3A0` stats copy; new: the act-2 camera-lead/`word[ESP]`
  gates, the `[0x158882]` producer, and the **L4 carry-in**: a live session's
  sit 9/0xA dispatcher queue path (ids 1/2 / 0xA) has no traced consumer yet,
  so the reachable FK chain is the native direct/pending path).
- **M2 phase-7 P3 (keeper state machines + AI mover, FU-151; 2026-10-09).**
  Landed from the frozen FU-151 slice (first-hand re-verification of the
  0x79C50/0x8DDE0/0x8C33C/0x7997C/0x7F374/0x761F2 bodies and the whole
  0x7550C/0x74EB0 windows during the port). Loader: the **row-1E ten-stage
  machine** `fifa96_keeper_claim_step` (`0x7550C..0x7612F`, stage table
  `0x754E4`; claim/hold/outlet/release/restart with the native fall-through
  chain 0->1->2/3->4->5->6->7->8->9 and the 0x760DF common exit gauge) and the
  **row-1D five-stage close-down** `fifa96_keeper_closedown_step`
  (`0x74EB0..0x754E1`, table `0x74E9C`; the 0x30/0x31 clearance staging, the
  0x3C0 sine rotation, the shared restart tail). Effects are request bits
  (place/helper/controlled/install/reset/situation + the leg sinks); the
  engine binder (`fifa96_match_action_1D`/`_1E`) stages the run's keeper
  process cells (`keeper_cam_*` = the 0x15774C focus stand-in, `_reset_*` =
  0x157A77, `keeper_vec_*` = 0x157C30, `_saved_*` = 0x157C36, `_gauge` =
  0x157C42, `_latch_157ab2`) and applies them, with the sector (+0x8E)
  write-back on `record.type` and the action code (+0x91) on `record.code`.
  `+0x44` is modelled as `entity/record.row44` (producer unported, a leg).
  AI seam: `fifa96_entity_face` (= `FUN_00079C50`, facing word + octant),
  `fifa96_match_entities_team_pick` (= `FUN_0008DDE0` unsigned min-`+0x6B`),
  `fifa96_match_entities_reset_lane` (= `FUN_0008C33C` + derived
  `FUN_0007997C`; per-record reset + `0x795B4` lane refresh + forced install,
  then `target`/`tracker7c7` = the pick and `second`/`timer7cb` cleared), the
  0x10F37C `{(0,0,0x990),(0,0,-0xA90)}` intercept constants staged in
  `match_run_entity_frame`, and `team.tracker7c7` (renamed from
  `camera_nearest`, the FU-147 S1 +0x7C7 tracker). **Errata application
  (FU-151 §3.5 erratum 1):** audited findings — the engine's action-code
  gates read +0x91 correctly (`record.code`, pool `code`,
  `outfield_type_gate(s->code)`, the row-07/0F kick staging per FU-139 §9.7);
  the +0x8E byte is correctly the face octant (`entity.type`, the legacy
  alias `actor_type`, OL-83 naming); **two wrong-field reads found in unwired
  loader bodies and fixed**: `fifa96_action_carrier_arm`'s 0x7F374 tail gate
  now reads a new `carrier.code` (+0x91) instead of `type8`, and
  `fifa96_keeper_input_decide`/`fifa96_keeper_arm_step`'s 0x761D1/0x765A7
  gates now read `input.code` / the `code` parameter (the row-1F bodies stay
  unwired, so no tape effect; tests pin both). **Both goldens byte-identical,
  no re-pin**: the M2 tape (ISO present) passes against the committed
  `port/tests/golden/engine/m2-frames.txt` unchanged (the keeper rows are not
  dispatched in the natural tape and the forced window bypasses them; the
  phase-2 intercept constants did not move the pinned frames); M1 unmoved.
  `make check` 108/108 (107 + `test_keeper_machines`; +ASan/UBSan on the
  engine suites). Carried legs: FU-151 §5 legs 1-15, notably the code
  0x1D/0x1E install sites (computed), the stage-3 UI/audio calls, the
  `FUN_0006E8E8` arms, `FUN_0008D098`, the `0x8922C` body, the `0x7D6F8`
  container, the sector consumers, phase-0xA semantics and the
  `[0x157820]/[0x157822]` consumers; new in the port: the `[rec+0x1C]`
  reset-target virtual (reset-lane stand-in keeps the live position), the
  0x157C5E per-side table / `cam_off_z`, and the `0x10F334/0x10F33C` offsets
  as caller inputs.
- **M2 phase-7 close-out (P5 acceptance; 2026-10-09).** Whole-range summary:
  the phase-7 plan `2026-10-09-fifa96-m2-phase7-ports.md` ported the four
  frozen phase-7 slices — P1 set pieces & restarts (FU-149), P2 fouls /
  referee / offside (FU-150), P3 keeper machines + AI mover (FU-151), P4
  presentation residual (FU-152) — serialized under subagent-driven
  development, each reviewed with a bounded fix loop (P1 clean, P2 one round,
  P3 one round, P4 one round), then P5 (this acceptance: tape v7, the smoke
  above, these docs). Gate status against the plan: **Gate P1** reachable set
  pieces verified (queue/arm/scanner/counter; legs recorded) — pass; **Gate
  P2** foul/offside chains fixture-reachable with the negative pins honest —
  pass after the fix round (the phase-write side and offside speech); **Gate
  P3** keeper machine + AI target selection fixture-reachable — pass after the
  fix round (the 1E/1D topology and the reset-lane metric); **Gate P4**
  reachable presentation verified (R1/R2/R3 rows, camera handlers, FU-71
  helpers, pool identity; R5 dormancy pinned) — pass after the fix round (mode
  table image data, R1 gate); **Gate P5** tape v7 green, smoke honest, this
  whole-plan review material landed — pass. Every task's `make check` was
  green in sequence: 106/106 (P1) → 107/107 (P2, +`test_referee`) → 108/108
  (P3, +`test_keeper_machines`; P4/P5 keep 108/108) with the engine suites
  under ASan/UBSan.
  Re-pin ledger (phase-7): **no re-pin at any task.** M1 immovable
  (`09b726b7…`, unchanged since M1); M2 byte-identical through P1–P5
  (`2e709151…`, the S1 v5.1 re-pin lineage — the phase-7 landings moved no
  presented frame: the set pieces/referee/presentation rows never run on the
  tape, and the row-1E keeper machine that does run writes only run-state
  cells the render chain does not read). Tape v7 adds assertions only;
  `cmp` against the committed golden is the recorded check, M1 `cmp` clean.
  Leg register carried out of phase 7 (each slice's section holds the detail):
  **FU-149** L1–L13 (L2 derived-landed, dynamic-trace confirmation open; L12
  the `[0x157B8E]`/`[0x157B8F]` gate + act-4/sit-1 arms; L13 the taker row
  bodies 0x10/0x11 — the arms install the codes, the stage machines stay
  unported; 0x1D closed in P3 / FU-151); **FU-150** the 11 §Port-contract legs plus the L4 carry-in
  (the live-session sit 9/0xA queue consumer untraced — the reachable FK chain
  is the native direct/pending path) and the new legs (act-2 camera-lead
  gates, `[0x158882]` producer, foul-log `rec[+4]` stand-in, dropped
  `0x14C3A0` stats copy); **FU-151** the 15 legs of §5 (§8 landing table:
  code-0x1D/1E install sites, stage-3 UI/audio, `FUN_0006E8E8` arms,
  `FUN_0008D098`, the `0x8922C` body, the `0x7D6F8` container, sector
  consumers, phase-0xA semantics, `[0x157820]/[0x157822]` consumers, plus the
  new `[rec+0x1C]` virtual, the 0x157C5E table/`cam_off_z` and the
  `0x10F334/0x10F33C` caller offsets); **FU-152** the 14 legs of its §8
  ledger (legs 1–4 runtime strings/glyphs/audio, 5/6 closed-or-pinned, 7 the
  per-case overlay helpers, 8/10 partially landed, 9 the camera integrators,
  11 the palette-pool content producer, 12–14 the staged strip inputs). The
  goal-invoker register (`OL-87/88/89`) was the score-blocking carry through
  phase 7; **phase-6 S2/S3 + phase-8 T2/T4 closed it** (see the T4 entry /
  FU-142
  §L.10) — the residual legs are the tracked-side pick, `FUN_00071DF4` and
  the `FUN_00066724` sink. Deferred-minor
  triage (recorded for the final review; doc/comment-level unless noted):
  P1 — the FU-149 §1.5/§2 "code 0 / first index 1" erratum cross-ref and the
  test-comment lifecycle-seed note stay open (the erratum-5 act-only-row
  boundary note closed via FU-150 erratum 8 and the L4 carry-in row);
  P2 — the `contact_kind` pre-store before the row-0x0C gates stays open as a
  behavior note (the native stores the entry kind inside `FUN_0008A43C`; the
  engine stages the literal 1 before the decide call — observationally
  equivalent on the reachable paths, re-derive if a caller ever passes a
  different row-0x0C kind); the round-1 doc minors (foul-log id substitution,
  `0x14C3A0` drop, L4 row) closed; P3 — all review minors closed in fix
  round 1 (dropped-sink listing, `0x7997C` subset note, §2.4 truncation
  prose, §8 wording, README/FU-137 errata, the dead RNG test arm); P4 — all
  review minors closed in fix round 1 (staged band predicate remap + leg-9
  note, `replay_camera_set` default store, the report quote). No open
  phase-7 minor blocks any gate; the triage list is the final-review input.
- **Unwired rows (59/80).** 58 rows dispatch `-FIFA96_ERR_UNSUPPORTED`: 24
  unported action rows, 34 phase rows (derived and ported at the loader level by
  FU-143 but not wired into the engine dispatch) — among the action rows the
  unwired actions `27`/`29`/`2C` (ported bodies, no installer entry) and the
  dead entry `2B`; phase `0x16` is the native zero/INT3 slot and the single
  `-FIFA96_ERR_NOT_FOUND` (21 action rows wired OK: FU-149 §7 / T1 plus the M2
  phase-9 T1 row `02` and the M2 phase-9 T2 row `05`; dispatch 58 UNSUP / 21 OK
  / 1 NOTF, FU-137 §7).
- **OL-48 rows 27/29/2C:** bodies ported and tested (FU-142b/c), but the
  FU-142f census finds no installer invocation for their codes anywhere in the
  image, so they stay unwired; row `2B` is a dead entry (shared row-29 RET).
- **OL-70/OL-70a rows 04/08:** the outfield decide/chase machine subset and the
  interception tail are ported; row `04`'s full body is ported and wired
  (FU-142 Appendix K.5 Task 1, `fifa96_outfield_row04_step` /
  `fifa96_match_action_04`; unmodeled inputs OL-72) and row `08`'s body
  `0x81068..0x814AF` is ported and wired (FU-142 Appendix K.6 Task 2,
  `fifa96_outfield_row08_step` / `fifa96_match_action_08`; no installer arm,
  unmodeled inputs/sinks OL-82). Both OL-70/OL-70a are closed.
- **OL-81 / OL-83 (playability-legs review legs).** OL-81: the FU-142a
  decision compares `[0x157AC5]`/`[0x157AC7]` byte-wide (the
  `fifa96_match_phase_machine` `ac5`/`ac7` fields are `uint8_t`) while the
  native words are 16-bit; no producer binds either word yet, so the
  divergence is unobservable until one lands (FU-142 §6). OL-83: rows
  04/06/07/18 read `record.actor_type` for the native `+0x8E` byte while rows
  08/28/2A write `record.type`, so a row-08 face write is invisible to the
  rows that natively share the byte — a latent wired-row divergence to
  reconcile in a follow-up (FU-142 §6/K.6.7). *(errata, M2 phase-25 W25-8 /
  FU-263: the reader over-list above is corrected — row `18` does **not** read
  `+0x8E` (fresh FU253 §2.4/§3.3: `0x849B0..0x84AE1` touches only `+0x28`,
  `+0x65`, `+0x63`, `+0x89`, `+0x92`, `+0x9A`, `+0x7B` — no `+0x8E` and no
  `+0x8B` access); the wired `actor_type` readers are rows `04`/`06`/`07` (+ the
  shared kick binder and rows `0B`/`21`). And the ruling is **unify**: `type`
  and `actor_type` denote the **same** native byte `+0x8E` (the pool's
  `byte[rec+0x8E]` idiom vs the `dword[rec+0x8B]>>24` idiom), so unify on
  `record.type` and retire the `actor_type` alias — a Stage-2 fidelity fix, no
  wire, no census change (FU253 §3.3 `FU253-L3` + FU252 §5.1 `FU252-L4`, ≥9
  `+0x8E` writer functions). **Deferred (Stage-2, OL-83, owed):** the second
  over-list site — `port/src/fifa96_engine/fifa96_match_run.h:503` (the comment
  *"the row-04/06/07/18 readers use `actor_type`, OL-83"*) — carries the
  identical row-18 over-list and is owed to that same Stage-2 fidelity task; it
  is **not** touched here (no code change). The old text is kept verbatim.)*
- **OL-63 row 05:** the carrier machine stages 0-3 and the ball staging tail are
  ported and the row is **wired (M2 phase-9 T2)** — the claim
  (`0x7F1FF`), the team-target bind, control set/clear, dir bytes, merge
  request and the stage hand-off run over the pool on a live code-5 dispatch
  (see the phase-9 T2 entry); the stage-0 target algebra
  (`0x7F3A1..0x7F57B`) and the `FUN_0007F7E0` fallback remain.
- **OL-80 render anim inputs — closed (M2 playability Task 5; links FU-141
  OL-42):** `FUN_00036C70` stages each slot's `anim_id`
  (`byte[[rec+0x28]]`, 0x36D44) and `frame` (`byte[rec+0x3D]`, 0x36D4F). The
  FU-141 pool now carries both fields (`fifa96_match_entity.anim_id`/`.frame`,
  first-hand re-measured on `/FIFA96.EXE`: `0x36D44 MOV ECX,[EDX+0x28]; MOV
  CL,[ECX]`, `0x36D4F MOV CL,[EDX+0x3D]`), the scene staging seeds each slot
  from the live pool record and writes the FU-84 advance back into `frame`,
  the run dispatch stages `anim_id` into the arm record's `anim_sel` (rows
  28/2A write it back through `fifa96_arm_anim_select`) and rows 04/06/18
  stage `byte[[rec+0x28]]` from it, so the FU-84 row+8 bank selection and the
  row-08 `+0x3D` gate follow live values. The `0x6E598` RNG-reroll arm stays
  OL-52 and the installer animation arm that produces non-zero ids outside
  the wired bodies stays FU-141 OL-42.
- **Render legs `OL-T11-1`…`OL-T11-9` (Task 11 close-out register).** Task 11
  left the render chain's remaining legs under these IDs: FU-84 frame tables
  not staged (`-1`; advance uses identity durations, `sprite = frame`), row
  successor/terminal/height machine (`-2`), record facing `+0x7D` (`-3`),
  camera-type ratio setup `FUN_0004D7E8` (`-4`; static `0x1500` default),
  sentinel key scratch producer (`-5`), palette install/kit remap identity
  stand-in (`-6`; `FUN_00048DC0`/`FUN_000CE980`), HUD/overlays (`-7`), kickoff
  formation/record placement (`-8`),
  direction addend `0xA2A10` (`-9`); live anim inputs are `OL-80` above.
  `OL-T11-7` is now **landed (M2 full-gameplay P0.2; FU-148 §10)**: the match
  HUD chain (`FUN_000565BC` gates -> `FUN_00055C24`) is ported — the GAMEART0
  entries are staged by BIGF name (clockfnt.fsh slot 0x35 / playfnt.fsh 0x36,
  the FNTI decode in `fifa96_font`; the Frames.fsh bar frame 13 with frame 4's
  layout height, closing FU-148 §7 leg 2 via `FUN_00053930`'s `0x14E624`
  frame table), and `fifa96_match_run_render` draws the bar + name/score/
  period/clock overlay with the FU-148 §1.3/§1.4 layout (outline colour 6,
  main colour 0). The remaining HUD legs are `OL-T11-71`…`OL-T11-75` (FU-148
  §10 "Remaining numbered legs": colour ramp, name source, extra-time, gate
  writers, bar scaler); overlay draws outside the match HUD (marker/menu) stay
  unported.
  `OL-T11-6` is now **landed (M2 playable-match follow-up 4 T1; FU-144)**: the
  match-data load's palette chain (`FUN_00048ED8`/`FUN_00048B60` ->
  `FUN_00048C8C`/`FUN_000479A0`) is derived — `PALsys.fsh` (resource slot
  0x32, GAMEART0 entry 46) frame 2's type-0x22 chunk, the FU-98 kit remap in
  its native direction and the two chunk-range appends, `v << 2` to 8-bit —
  and installed on the presented surface (`fifa96_match_palette_from_bank`,
  `fifa96_match_run_palette_install`, staged by `fifa96_match_run_stage`), so
  the indexed draw is RGB in `make game`. Remaining `-6` legs: the
  per-entity translation tables `0x14BF60[slot]` (the native `0x14720` remap
  `FUN_00048DC0`/`FUN_000CE980` installs; the engine keeps the identity
  stand-in) and the untied `0x14B200` front-end base (FU-144 §6).
  `OL-T11-8` is now **landed (M2 visible-match Task 1)**:
  `FUN_00079B6C` (`0x79B6C..0x79C1C`) is ported in full commit + tail form —
  position := target, y = 0, target := position, both velocity pairs and the
  lane low word cleared, the camera-vs-target face, and the unconditional
  `0x79C13` `FUN_0006E598(rec, active ? 0 : 0x26, 0)` selector that gives
  inactive records row id 0x26 and resets the frame; the kickoff act-1 ball
  spawn (`[0x158830] = 0x1E0`, z = 0, ball.y = 0 via `FUN_0008C24C` `0x8C299`)
  is ported too (`fifa96_match_entities_place`/`_kickoff_place`, called at
  match begin after the camera reset). The per-record formation targets are
  now resource-landed: `fifa96_scene_formation_load`/`_place` evaluate
  `FUN_0006E1D0` over the 44-byte `.fmt` record (`[rec+8] =
  FUN_0004AFB8(6*id) + byte[rec+0x8D]*4`; {opp x/z, own x/z} pair;
  `x = (int8)b0*0x26`, `z = (int8)b1*0x21`, side-1 negated), the run loads
  `352ko.fmt` from the staged `/ART/GAMEART0.PVI` at begin
  (`fifa96_match_entities_seed_formation`) and the commit lands the non-zero
  positions, so player entities draw. The corrected name formats
  (`%s.fmt`/`%s.dat`/`%s.lfsh`/`%s.qfs` over the `0x107370` table) and the
  BIGF-entry location are recorded in the FU-89 §11 erratum; the remaining
  legs are the front-end formation-id producer (`[0x14C1E4]`/`[0x14C1E5]`,
  BSS 0; the engine derives id 0), the unused `.dat`/`.lfsh`/`.qfs` slots and
  the `[team+0x7DB]`/`[team+0x7DF]` pointers, and the roster `+0x90`/`+0x9A`
  bytes. The tail's conditional `FUN_0006E48C` `+0x3E` write stays a leg (no
  pool field). `[0x157AB1] = 0` is a process global with no derived home.
  Note the plan's "kickoff placement (OL-T11-9)" label is a numbering
  erratum: the register's `-8` is the placement item and `-9` is the
  direction addend. `OL-T11-10` (FU-89 key-seeding consumers) closed with the
  FU-89 §11 errata.
- **Phase table (FU-143):** the 35 FU-83 phase rows (handlers `0x110794`,
  classes `0x1106AD`) and the transitions `FUN_000740A0` / `FUN_000888FC` /
  `FUN_0008A938` / `FUN_0008B9CC` are derived and ported at the loader level
  (`fifa96_action_phase_row/_act/_situation/_period_end`), with open legs
  `OL-72`…`OL-79`. **Phase-15 T1 (FU-175/FU-181):** phase `0x1F` (act 9 handler
  `0x88F4C`) is the first wired phase row — the native act/phase dispatch seam
  (`FUN_000888FC` + the `0x8B669..0x8B672` clock-tail tick, ported in
  `port/src/fifa96_engine/fifa96_match_act.c`) gives `fifa96_match_dispatch_phase`
  a live caller, and the derived act-9 producer (FU-170 §7) reaches the row
  each granted frame; the handler's `0x73E28`/`0x93850`/`0x361B0`/`0x4C324`
  sinks stay FU181-L5. The phase driver `fifa96_match_run_phase_drive` is **wired
  into the run loop** (M2 playability Task 3): each granted frame it applies
  the class-1/class-2 gate and consumes the FU-62 clock's `sec == limit + aux`
  completion (`mr.clock_period_ended`), running the derived `FUN_0008B9CC`
  chooser — a live class-1 period end writes phase 0x0C (2 → 0x0C) on the
  selector-0/no-extra-time default, and the run-end teardown resets to 0.
  `fifa96_match_run_begin` now installs the **derived kickoff phase entry**
  (M2 visible-match Task 2 / `OL-84`): the native phase-0x17 handler stage-0
  write `FUN_000740A0(1, side)` (`0x88E82`, after `FUN_00073E28` and before the
  `FUN_00073E08` placement commit the begin path models) lands as
  `FIFA96_MATCH_RUN_KICKOFF_PHASE = 1`, mirroring the `phase_machine` switch
  byte. Follow-up-4 Task 2 lands the **OL-84 residual**: begin also runs the
  derived `FUN_0008D098` state-1 arm
  (`fifa96_match_phase_machine_kickoff`: code-3 multi-install, the
  `FUN_00079CCC` nearest pick — formation-target substitution — the action 1/2
  taker installs and the team targets), the wired action row 01
  (`fifa96_match_action_01`) walks the native stage machine and its
  situation-0xB call runs `fifa96_match_run_situation` (`0x8AEF6` ->
  `0x8AF02` = phase 2), and `fifa96_match_run_frame` arms the derived act-1
  producer `mr.global_5882a` at `state.tick_total >= 0x78`. **M2 phase-18 T3
  correction:** the live situation-0 kickoff cutscene now installs code `0x24`
  on the pool and **pre-empts this derived chain**, so a begun run **holds in
  the cutscene and does not reach phase 2 naturally**; the native act-1 route
  (situation 1 -> act 1 -> phase `0x17`) that would re-establish the kickoff is
  **unported** (leg FU202-L2/FU203-L2), so the derived chain is exercised only
  with the armed act cleared
  (`test_engine_match_frame::test_kickoff_enters_phase2_when_cutscene_isolated`;
  FU-143 §11). *(errata, M2 phase-18 T4: the act-1 route is now **live** — a
  begun run again reaches phase 2 naturally via the phase-`0x20` stage-2
  re-dispatch → situation 1 → act 1 → phase `0x17`; the T3 text above is kept
  verbatim — see the phase-18 T4 bullet. The retained frame-body `global_5882a`
  arm is the remaining leg FU203-L2.)* The remaining situation-0xB producers (rows 02/0x10..0x13, the
  keeper/restart bodies, the computed act-8 call) and row 01's
  camera/event/ball-stage sinks stay open legs (`OL-84` narrowed).
  The extra-time flag producer (`OL-85`, re-verified at `0x8B2AC`/`0x8B2C1`)
  and the post-period 0x0C hold/reset timing (`OL-86`) stay open, and the
  natural chain is not frame-for-frame identical to the forced window
  (FU-143 §11.5), so the M2-B
  tape keeps its declared phase forcing (0x13/0x14 kickoff, phase 2 mechanics)
  and stays byte-identical. See FU-143 §9/§10/§11 (integration errata).
- **Score event source (child `C3-OL2`, closed; invokers unreachable — M2
  playable-match Task 3 verdict).** The FU-72
  `FUN_00093944` writer is derived and ported
  (`fifa96_action_score_event`, FU-142 Appendix L): score increment, last-side
  and tracked-side goal-difference bookkeeping, and the
  `FUN_0009252C` threshold posts (0x9A..0xA0, 0xD3). It is wired as the live
  run's source (`fifa96_match_run_score_event`, with the writer's four state
  cells and `score_last_event`), and the M2 tape's goal step uses it
  (transcript byte-identical). **No writer invoker is reachable from the ported
  rows/state** (FU-142 App. L.9, first-hand): all eleven `FUN_00093944` sites
  lie in the six period-indexed handlers (`0x110F78`); the only situation-6
  producer is `0x88B44` in `FUN_00088940`, reached only through the camera-pan
  arming (`FUN_0007131C 0x713A6..0x713F7` sets `[0x15781D]`/`[0x15781E]`) and
  the clock scan call (`FUN_0008AF38 0x8B63E`) — the engine camera never
  leaves spawn; the goal queue (`0x8A9E8` ids 5/6), the scheduler
  (`FUN_000948AC 0x4B1A1`, `0x949E9 CALL [0x15B6D4]`) and the handler
  installer (`FUN_00092D8C` ← screen machine `FUN_00038630` case 0xF;
  `FUN_00092E2C 0x92EEE`) are unported. Act 8's computed `0x8A8CE` is resolved
  as the phase-0x1E handler re-dispatching the situation the dispatcher stored
  (`[0x15882B]`), not a goal path. The negative is pinned by
  `test_goal_situation_dispatch_is_not_the_writer`,
  `test_natural_phase2_never_scores`, row 01's score-freshness assertions and
  the tape's pre-score assertions. Open legs at the Task-3 window: **OL-87**
  (handlers + scheduler + installer), **OL-88** (camera-pan goal detection),
  **OL-89** (posted-id dispatch `FUN_0009252C` and the `FUN_000CBC4C` probe).
  **Superseded by phase-6 S2/S3 + phase-8 T2/T4** (FU-142 §L.10): OL-87/88 are
  closed
  and OL-89 is closed except the `FUN_00066724` sink, ported as
  `fifa96_score_display_gate` + `score_display_event`. The tracked-side pick
  landed in phase-9 T3 (FUN-00078824's flag zeroing + the FUN-00092D8C pick;
  the flags-zero default is tracked 1, so the natural goal posts the probe id
  0xD3) — the residual display/presentation sinks stay legs;
  `fifa96_match_run_add_goal` remains for those unported paths.
- Retail front-end art asset (no OPTIONS-like path exists in the ISO).
- Per-row cluster legs carried in FU-139/FU-141/FU-142 (`OL-56`…`OL-71`:
  unmodeled record bytes, process globals, camera/track inputs, roster
  descriptors, animation selectors).
---

## T7 close-out erratum (fix round 1, 2026-10-22)

The phase-30 register above stands as written; this append-only section
corrects the register's share of the T7 review's three documentation
Importants and its precision minors. No code/test/golden/census change:
`make check` **111/111** stands; M1 `09b726b7…` / M2 `8434cfab…` unmoved; the
census holds `60/80 = 75%`. (The nested-bold heading at the register's first
line was a markup typo and is fixed in place — the only in-place edit.)

  - **1. The stale evidence line cites (Important 3).** Quoted (register, T2
    bullet): "the kick gate (`handlers.c:972`; native `0x81519/0x81520`), the
    outfield pack, the chase (`match_run.c:915`; `0x7CCCD`)"; "[rows 01]
    (`handlers.c:240/246`; `0x7DCCC/0x7DCD7`) and 11 (`:1887`; `0x860DC`)".
    Quoted (register, errata (7)): "the same high-word read recurs at the
    family sites (`handlers.c:240/246/1683/1887/2037/2183/3701/4546/4823/
    5176/5900`, `match_run.c:915`), and `handlers.c:972` fed the **kick**
    gate the low word". Quoted (register, errata (8)):
    "`port/ENGINE.md:5236/5277` (the phase-7 P3 block and the phase-7
    close-out ledger)".
    **Corrected (fresh `grep -n` this round; the exact tokens):**
    - the live kick read is **`fifa96_match_handlers.c:998`** —
      `s->lane_word = (int16_t)r->lane_x; /* 0x81519 dword[+0x69]>>16 = +0x6B */`
      (the 972 cite is retired);
    - the live chase read is **`fifa96_match_run.c:920`** —
      `s.chase.distance = (uint16_t)e->lane_x; /* 0x7CCCD dword[+0x69]>>16 */`
      (915 is the section start, not the read);
    - the current legacy row-01 reads are **`fifa96_match_handlers.c:249`**
      (`int16_t lane = (int16_t)(r->lane >> 16); /* LEG FU311-L5a */`) and
      **`:255`** (`near_lane = (int16_t)(team->records[near].lane >> 16);
      /* LEG FU311-L5a */`); the row-11 read is **`:1921`**
      (`if ((int16_t)(r->lane >> 16) > 0x40) return FIFA96_OK; /* LEG
      FU311-L5a */`) — the 240/246 and 1887 cites are stale. The other family
      sites in errata (7)'s list are the FU311 review-era classification
      (they were fixed by T2; their line numbers are historical provenance,
      not current reads);
    - the `cam_off_z` tokens are at **`port/ENGINE.md:5239`** and **`:5280`**
      (fresh `grep -n`); 5236/5277 are the neighbouring lines.
  - **2. The register completion — FU312 and FU313 (Important 2).** The
    PROGRESS close-out says "The full leg register (FU310–FU313, the harness
    matrix, the fixes, the spike and the quoted errata) is appended to
    `port/ENGINE.md`." At the original close-out that was false: the register
    carried no FU312 leg list and no FU313-L1/L2/L4/L5 list. The missing legs
    follow (names/owners carried from the wave docs), so the register is now
    complete:
    **FU312 — the natural event route** (`port/spec/FU312_r2l5_natural_event.md`
    §5).
    - **FU312-L1 — the camera-height bootstrap (the event's prerequisite).**
      No open-play port producer raises `[0x157750]`; the only native writers
      in the usable range are the restart/keeper cuts (throw-in `0x85919` =
      `0x80`, keeper `0x75D69` = `0x50`, `0x75F86` = `0x80`, `0x75A0F` =
      `0x38`, the `rec.pos_y + 0x38` rows), all dispatch-dead in the port.
      Owner: `FUN_00085911..0x8591C` (row `0x10`) + the restart queue
      (FU163/FU310); `FUN_00075D17..0x75DD1` / `0x75F6C` (keeper rows
      `0x1A`/`0x1B`) + R11-L4 (FU311). This is the minimal missing producer;
      FU309-L1/FU307-L1 are the same leg from the camera side.
    - **FU312-L2 — the kick reach at a high ball height.** At camY `0x80` a
      grounded kicker is refused (`0x81531/0x81536`: `pos_y + 0x70 < camY`);
      the throw-in route needs an airborne kicker (the height table) and the
      crest window is 1..few frames (aux `0x0E` crest `0x10` = one frame; aux
      `0x3A` crest `0x18` = a few). Owner: `0x81519..0x8153F` + the jump rows
      (T1-L1). The keeper `0x50` cut avoids this (the window is the jump's
      life).
    - **FU312-L3 — T1-L1 (the live jump on the kicker), open.** The jump
      arm/arc and both jump-row installers are ported (`0x5E` ← the row-`0x0C`
      body `0x81DA4`; `0x12` ← the row-`0x0E` body `0x827D0`; engine
      `fifa96_match_handlers.c:3476`, `:1478`), but no proven natural route
      puts the ball carrier airborne at the kick: the row-`0x0E` install from
      the row-07 defender arm is gated on `kick_decision`'s `predictor_y =
      camY ∈ [0x20,0x60]` (`fifa96_action_handlers.c:1579`;
      `fifa96_match_handlers.c:1048-1050`), and the row-23 tackle's attempt
      gate `cam_f8 >= 0x14` (`fifa96_action_handlers.c:642`) is never staged
      (the memset gives 0), so `install_0e` never fires; the
      row-`0x0C`→`0x5E` path is a contact/duel record. Owner:
      `0x7E600`/`0x82FF3..0x8305A`/`0x82E85` + the row-`0x0C` body.
    - **FU312-L4 — FU309-L7 (the tracked record must move).** The receive
      binds `tracked` to the kicker; the follow's velocity is the tracked slot
      dirs × 15 (`0x71E63/0x71E69`; `fifa96_match_run.c:3942-3948`), so a
      stationary tracked record gives `follow_speed == 0` and gate 2
      (`0x739CE`) refuses. The natural witness must hold a direction across
      the kick (the cold-camera zero-dir kick is a no-op). Owner: the
      acceptance input script + the slot-dir producer
      (`fifa96_match_run.c:1401-1404`).
    - **FU312-L5 — the airborne discriminator width.** Native `TEST
      dword[rec+0x5D]` (`0x7AF0D`) vs the port's 16-bit `event->height != 0`
      (`fifa96_action_handlers.c:115`); equivalent while the record's `+0x5F`
      high word is 0 (the mover writes the low word). Owner: `0x7AF08/0x7AF0D`
      + the pool mover.
    - **FU312-L6 — the `0x40` draw is RNG-state-dependent.** `>0xF0` needs
      `(r&7)==7` (1/8) with `desc15 <= 2`; `desc15` is the OL-72 stand-in 0,
      so the live draw is exactly `0x100` on 1/8 of the draws, and the
      deterministic kickoff RNG fixes it for a given input script. The
      harness must expose the draw (or pin the script that lands it). Owner:
      `0x7BE6F` (`FUN_00092AC8`) + the RNG state; FU313's harness.
    - **FU312-L7 — the per-record chaser hand-over can clobber the tall
      event.** The probe observed the receive's `0x100` event re-seeded to
      `0xDC` (and `tracked` flipped to another record) within ~6 granted
      frames when the other records were not isolated; `FUN_00070C08`'s
      reflect/ramp arms re-arm with camY/target (FU306 §4). The witness must
      assert the follow at the first granted frame after the setter or isolate
      the chaser. Owner: `0x70C08` / `fifa96_camera_record_event` + the
      acceptance.
    **FU313 — the acceptance harness**
    (`port/spec/FU313_r2l5_natural_acceptance.md` §9).
    - **FU313-L1 — the probe's camera assertions need the producer.** P6–P8
      stay RED until FU310 (bootstrap) and FU312 (event) land; they are
      written RED-first. Owner: the phase-30 acceptance task / the next
      bootstrap producer.
    - **FU313-L2 — the input-timing coupling.** A held-direction tail appended
      after `M2_KEYS` suppresses the natural phase-2 entry (measured twice);
      the cause is unproven — the candidate is the row-01 kickoff release gate
      consuming the slot edge. Owner: `fifa96_match_run_input` + the row-01
      body (`fifa96_match_run.c:436-455`, the kickoff release path). Until
      settled, replay the measured tape and append only after phase 2.
    - **FU313-L3 — the ladder-vs-stage-select attribution split.** A `ran`-only
      observable cannot distinguish "the ladder gate never fired" from "the AX
      prepass refused"; the probe/trace must carry both counters. Owner: the
      acceptance procedure (+ FU311's register text). **Addressed this
      phase:** the T1 probe carries `calls`/`select_calls`/`select_ran`, and
      errata (1) above quotes the measurement.
    - **FU313-L4 — the bbox threshold is proposed, not measured.** `864x443`
      (0.9×960 × 0.75×591) is derived from "a camera translation moves the
      whole scene", not from a live follow capture (none exists). The
      acceptance must measure the first real pair and record the observed
      bbox; re-pin with that datum, never silently. Owner: the Layer-2
      acceptance task.
    - **FU313-L5 — no visible-camera capture in the repo for a pan.**
      `docs/screens/*` has no whole-scene-shift pair; the phase-9 AE data are
      entity-motion deltas. The first live follow capture has no pixel
      precedent to compare against — only the AE magnitude guidance (§6.5).
      Owner: the Layer-2 acceptance task.
    `FU309-L6` stays **closed as a procedure** (the phase-29 close-out);
    `FU309-L9` stays **open** (the mutation-rider automation, deferred by
    spec §3).
  - **3. The explicit closures (Minor 1).** **FU309-L5 — CLOSED (T1
    `672f35a`):** the env-gated camera-cell trace landed (default-off
    `FIFA96_CAMERA_TRACE`, 16 fields, the loop-tail line, the idempotent
    close; P10 is manual). **FU310-L8 — CLOSED (T3 `56a17cd`):** the flat
    act-2 compression is replaced by the native per-stage gate (stage 2
    split/hold; stage 3 kind → session, else hold); `[0x158882]` remains
    FU166-L6. (FU307-L7 was recorded closed in the T5 bullet / errata (5).)
  - **4. FU307-L1 in the remainder (Minor 2).** Quoted (register, P9): "the
    keeper route stays FU307-L1". **Corrected:** in every remainder list the
    entry is **FU307-L1 (NO-GO)**; the structural NO-GO is errata (3) above
    (the ladder phase 2 vs the row-`0x1B` place phase `0x0D`). `FU307-L2/L5/L6`
    stay open as listed.
  - **5. The deferred-list corrections (Minors 3–4).** Quoted (register):
    "**Deferred minors (T1–T6 reviews; not fixed here; none is a fidelity
    defect).**" **Corrected:** the **T6** scoped-re-review minors are
    **fixed** (the appended FU310 erratum), so the heading's "T1–T6 / not
    fixed here" applies to **T1–T5** only. The deferred list gains the four
    residual concerns:
    - **T2 residual — the chase `uint16_t` signed/unsigned compare.** The
      chase gate's `uint16_t` comparison (documented as the native "unsigned"
      compare in `fifa96_outfield.h:51-55`) is a pre-existing signed/unsigned
      divergence; T2 corrects only the source word, not the compare.
    - **T4 residual — row-`0x1D` stage 1's `0x700F4` 4th argument.** Native
      `0x7509A MOV AL,[EDX+0x9B]; PUSH EAX` (the `place_pause` flag) is not
      modelled; `[0x157820]/[0x157822]` are not written by this row (part of
      the FU-151 consumers leg).
    - **T5 residual — the release+hold-follow same-step ordering.** A stage-3
      hold-follow place then a stage-5 release still drains a place after the
      apply's `camera_event_set`/`set_tracked`; the ordering predates T5 and
      is unchanged (the place value is the post-store triple).
    - **T6 residual — the scratch-probe reproducibility.** The
      `/tmp/opencode/fu310_l9` probe source is not committed; the FU310 doc
      records the drive and the measured numbers, but the runnable source is
      ephemeral (the same class as the wave's throwaway probes).
  - **6. The exact ladder quote (Minor 5).** Quoted (register, errata (1)):
    "the ladder does not run at all — the AX prepass gate
    (`0x77F9A`/`0x77FA5`) never opens because the live camera stays
    `(0,0,0)`." **Corrected:** the original (`port/ENGINE.md:2338-2340`) reads
    "the ladder does not run at all — the AX prepass gate
    (`0x77F9A`/`0x77FA5`, `|cam.x| < 0x420 && |cam.z| > 0x7B0`) never opens
    because the live camera stays `(0,0,0)`; the tape's own m62 block …". The
    register's quote dropped the parenthetical without an ellipsis; the
    correction (2 019/6 118, `ran=0` is the AX prepass) stands.
  - **7. The reader scope (Minor 6).** Quoted (register, T2): "every
    `dword[rec+0x69]>>16` reader routes to the live `lane_x` word".
    **Corrected:** every **fixed** reader routes to `lane_x`; rows 01/11
    remain the **FU311-L5a** deferral (the legacy low-word view), so they are
    not routed. The fixed-site list is the T2 report's classification table.
  - **8. The test-pin sentence (Minor 7).** Quoted (register, Tape): "no row
    joins, so `README.md`, `AGENTS.md` and the `port/tests/test_engine_m2.c` pins
    are untouched." **Corrected:** the golden-path **assertion** pins are
    unchanged; the T2 re-pin is recorded above (the T2 fixture lanes changed,
    and the test comment's provenance still says `b281a9b2…`). README and
    AGENTS are untouched.

---

## T7 close-out erratum (fix round 2, 2026-10-22)

The round-1 erratum above stands as written; this append-only section corrects
the scoped re-review's FU312 leg-cite findings and the row-01 comment-quote
truncation. Every cite below was **re-resolved against the post-T2 tree**
(fresh `grep -n`/read this round, 2026-10-22), not carried from the wave docs.
No code/test/golden change: `make check` 111/111 stands; M1 `09b726b7…` / M2
`8434cfab…` unmoved; the census holds `60/80 = 75%`.

**1. The FU312 leg cites (New Important).** Quoted (round-1 erratum, FU312-L3):
"`0x5E` ← the row-`0x0C` body `0x81DA4`; `0x12` ← the row-`0x0E` body
`0x827D0`; engine `fifa96_match_handlers.c:3476`, `:1478`". Quoted (round-1
erratum, FU312-L3): "the row-`0x0E` install from the row-07 defender arm is
gated on `kick_decision`'s `predictor_y = camY ∈ [0x20,0x60]`
(`fifa96_action_handlers.c:1579`; `fifa96_match_handlers.c:1048-1050`)".
Quoted (round-1 erratum, FU312-L4): "the follow's velocity is the tracked slot
dirs × 15 (`0x71E63/0x71E69`; `fifa96_match_run.c:3942-3948`)". Quoted
(round-1 erratum, FU312-L4): "Owner: the acceptance input script + the slot-dir
producer (`fifa96_match_run.c:1401-1404`)".

**Corrected (fresh reads; the 3476/1478/1048-1050/3942-3948/1401-1404 numbers
are the pre-T2 wave-doc lines):**

- the `0x5E` installer is **`fifa96_match_handlers.c:3512`** —
  `if (fifa96_match_run_record_anim_install(mr, a, 0x5E, 0) != FIFA96_OK)`;
  the `0x12` installer is **`:1504`** —
  `if (fifa96_match_run_record_anim_install(mr, e, 0x12, 0) != FIFA96_OK)`;
- the `predictor_y` assignment is **`fifa96_match_handlers.c:1075`** —
  `s->predictor_y = mr->render.camera.pos_y;`;
- the follow function starts at **`fifa96_match_run.c:3967`** —
  `int fifa96_match_run_camera_follow(struct fifa96_match_run *mr) {` — and
  the `fifa96_camera_rate_event` call is at **`:3985`** —
  `if (fifa96_camera_rate_event(cam, e->dir_x, e->dir_z,`;
- the slot-dir assignment is **`fifa96_match_run.c:1407-1411`** —
  `r->dir_x = (int8_t)mr->slot.anim_b;` / `r->dir_z = (int8_t)mr->slot.anim_c;`
  (the slot arm) and `r->dir_x = e->dir_x;` / `r->dir_z = e->dir_z;` (the
  else arm).

**2. The row-01 comment quote (Minor).** Quoted (round-1 erratum, item 1):
"(`int16_t lane = (int16_t)(r->lane >> 16); /* LEG FU311-L5a */`)".
**Corrected:** the code at `fifa96_match_handlers.c:249` reads
`int16_t lane = (int16_t)(r->lane >> 16);     /* LEG FU311-L5a (legacy view) */`
— the quoted comment truncated the `(legacy view)` tail. (The `:255` and
`:1921` comments are exactly `/* LEG FU311-L5a */`.)

---

## Phase-30 final fix-wave erratum (2026-10-22)

The register above stands as written; this append-only section records the
final whole-branch review's three Importants and the two deferred-list
corrections. One behaviour fix (the row-`0x1D` stage-2 camera shadow), one
harness assertion (P10), one comment truth fix, one new leg, and two doc
corrections. No camera producer was invented: **the phase-30 NO-GO stands**.
M1 `09b726b7…` and M2 `8434cfab…` are byte-identical (`cmp`-verified); the
census holds `60/80 = 75%`; `make check` 111/111.

**1. The row-`0x1D` stage-2 target shadow sync (Important 1).** The native
stage-2 body copies `[0x15774C]/[0x157754]` into the record target at
`0x75259` — the camera **after** the stage-1 `0x750B5` `0x700F4` place of the
`0x1104F2` triple. The port staged the place (deferred to the frame drain) but
never synced the machine's own `cam_x/cam_z`; the binder sources them from the
pre-step `keeper_cam_x/z`, so the stage-1 → 2 fall-through (`has_slot == 0 &&
timer89 > 0xB4`, or the slot `0x50` edge) targeted the old camera. The
reviewer's repro (`stage92=1, timer89=0xB5, has_slot=0, snap_x_nonneg=1,
cam=(10,30)`) gave `place=(0x1D0,0,-0xA00)` but `target=(10,30)`; the native
targets `(0x1D0,-0xA00)`. **Fixed:** `fifa96_keeper_closedown_step` now sets
`s->cam_x = place[0]; s->cam_z = place[2];` after both the stage-0 (`0x74F99`)
and stage-1 (`0x750B5`) places — the native post-place cells. Pins:
`test_closedown_stage1_slot_paths` (machine `s.target_x/z`) and
`test_row1d_stage2_target_fallthrough` (engine `record.target_x/z`). The M2
golden is unmoved (the tape does not reach this fall-through).

**2. P10 — the trace self-check (Important 2).** Quoted (register, harness):
"**P10** (the trace self-check) stays manual this phase (1 header + 3 598 body
lines; the env-set M2 stdout identical)." **Corrected:** P10 is now asserted
in `run_natural_probe` (`port/tests/test_engine_m2.c`): it sets
`FIFA96_CAMERA_TRACE` to a unique temp path for ONE natural pass, restores the
caller's environment, then requires exactly one header + **3 598** body lines,
contiguous run-relative `f=` ids, the phase cell on every line, and every
sampled `pos` triple at `(0,0,0)`. The earlier "P1–P10 matrix landed" wording
was wrong only because P10 was manual, not because the trace was wrong: the
sink is opened `"w"` by **every** `fifa96_match_run_begin`, so
`FIFA96_CAMERA_TRACE=/tmp/x ./build/test_engine_m2` keeps only the LAST
scenario's file (measured 182 lines), not the natural pass's 3 599. The
"P10 is manual" parenthetical in the explicit FU309-L5 closure above is
corrected the same way: P10 is asserted now, not manual. The trace remains
default-off and the M2 stdout is `cmp`-identical.

**3. The false "no writer" comment (Important 3).** Quoted
(`port/src/fifa96_engine/fifa96_match_run.h`, pre-fix): "Staged from the pool
record and never written back (the native `+0x6B` has no writer in the ported
engine path)." **Corrected:** the pool `+0x6B` has two writers — the BF20 lane
block (`fifa96_match_run.c:1507`, `e->lane_x = lane; /* 0x7C7AF: +0x6B */`)
and the reset (`fifa96_match_entities.c:221`,
`e->lane_x = band.band; /* +0x6B */`). The comment now reads: the pool
`+0x6B` is written by the BF20 lane block (`match_run.c:1507`); this
run-record copy is staged read-only.

**4. FU307-L8 — the claim stage-3 / closedown stage-7 raw X/Z store is not
delivered on a direct stage start (new leg).** Quote (FU307 §2.2 table): "the
0x1E stage-3 `0x38` | `disassemble_bytes 0x759B0..0x75A18`:
`0x759EA..0x75A0F`". Fresh (the T5 read cited there):
`0x759FD 8D7659 LEA ESI,[ESI+0x59]; 0x75A00 A5 A5 A5 MOVSD x3` copies
`rec+0x59/5D/61` into `0x15774C/50/54` — the full X/Y/Z triple, no `0x700F4`
— and `0x75A0F MOV [0x157750],0x38` overwrites Y. The closedown stage-7
literals have the same shape (`0x75EA6` triple + `0x75EEA` Y=`0x10`;
`0x75F66` triple + `0x75F86` Y=`0x80`; the `0x75F9E`/`0x75FEE` offset
recompute). The port delivers Y through `cam_y_written`; the binder branch
(`fifa96_match_handlers.c:546`) writes `render.camera.pos_y` only, so on a
**direct** stage-3/7 start — no same-step `0x700F4` place whose deferred drain
would carry the triple — the native X/Z store is undelivered and the next
frame reads the previous X/Z. Consequence: a restored/direct stage-3/7 entry
diverges in X/Z; every reachable path advances through the head place in one
process, which is why no fixture moves today. Owner:
`fifa96_keeper_claim_step` / `fifa96_keeper_closedown_step` + the
`cam_y_written` binder branch; fix with the FU307-L7 stage-1-triple and the T5
release+hold-follow residuals. **No behaviour change this wave.**

**5. The `0x81EB4` row-`0x0C` attribution (deferred-list correction).**
Quoted (register, deferred minors): "the `0x81EB4` row-`0x0C` attribution
missing (the reviewer supplied `0x1106E0[0x0C] = 0x81C90`)". **Corrected:** the
deferred item is stale — `136e795` (`chore(fu310): cite the stable 0x81EB4
for the live contact kind`) added the attribution in
`port/tests/test_engine_match_frame.c` (the stale `run.c:6542` cite became "the
row-0x0C literal `0x81EB4`"); the body head is cited
`fifa96_match_handlers.c:3466` (`Row 0x0C 0x81C90..0x8250B`) and the engine
producer carries the same cite (`fifa96_match_run.c:6822`). No code or test
change in this wave.

**6. The test/comment corrections folded here.** `port/tests/test_engine_m2.c`:
the sampler comment now says `lane_x=2541` (the measured view, not 0); the
golden-provenance block records the T2 pin (`8434cfab…`, 5 568 hash-only
lines, frames 955..6522, no state-tag change); the pan witness asserts
`granted_frames == 358`; its comments say the probe tail **overwrites the
copied tape's tail** (not "appends"); the stderr label is "P6 partial; P7/P8
RED by design". `port/tests/test_engine_match_frame.c`:
`test_action_1a_direction_54` stages `lane_x = 0x40` (the gate reads the high
word); the offside chain asserts the `incident_y` producer mirror
(`point[1]`). See `.superpowers/sdd/2026-10-21-fifa96-m2-phase30-producers/
final-fix-report.md` for the wiring and the command output.

---

## M2 phase-31 register (FU314…FU317; 2026-10-22) — the natural pan: the camera pans in natural play (the follow waived; the armer NO-GO)

**Scope/provenance.** Phase-31 spec
`docs/superpowers/specs/2026-10-22-fifa96-m2-phase31-natural-claim-pans-design.md`
(§4–§6/§8) and plan
`docs/superpowers/plans/2026-10-22-fifa96-m2-phase31-producers.md` (T1–T4);
the reviewed four-slice wave `port/spec/FU314_r2l6_pan_triggers.md`,
`FU315_r2l6_claim_chain.md`, `FU316_r2l6_pan_walk.md`,
`FU317_r2l6_claim_acceptance.md` (promoted `b85bb05`, the wave-review errata
appended to each). Append-only: the phase-30 text and every prior section
stand as written; the corrections below are quoted errata. Census
**`60/80 = 75%`** (`37 action + 23 phase`; no row joined); M1 `09b726b7…` and
M2 `8434cfab…` (295 087 B) byte-identical (`cmp`-verified; the T2 face
fidelity did **not** re-pin); `make check` **111/111** (strict
`-Wall -Wextra -Werror`, ASan/UBSan). The README title stays `75%`.

- **The verdict — the camera pans in natural play; the follow/Y stays waived;
  the armer is NO-GO.** The phase-30 NO-GO's first broken link is closed for
  one pan: the row-04 half-line arm (`0x7ED89..0x7EDDF` → `FUN_00071C94` →
  `FUN_000736AC`) fires from the **natural route** — the real kickoff, the
  engine's own phase 2, the live slot/claim cells, the spec's scripted pad
  drive, **no camera cell staged, no `act.id` staging** — and the camera place
  leaves `(0,0,0)` for the first time in natural play: `0 → −0x90` over
  **27 granted frames** (`f=1514…1540`, exactly one maximal changed-place
  run), `pan 0→4`, `tracked = 2`. The claim (`ball.carrier = 2`) follows
  **one frame later** (`f=1514`): the row-04 pan is upstream of the row-05
  claim, not produced by it. The reach is **one pan (`0x90`)**; the measured
  one-shot is the pan's own **row-05 install** (`outfield.c:719`, the native
  `0x7F133`), the tracked exclusion (`outfield.c:682`) being a latent second
  guard. **Layer-1 GREEN** through the headless witness (T1) and the
  frame-path face pair (T2); **Layer-2 visible and trace-attributed** (T3).
  The **follow/Y stays waived**: `ev=0x0` on the pan, `follow=0` on every
  trace line (3 598 null / 14 984 live), Y=Z=0; P7/P8 stay RED by design (the
  phase-30 waiver). The **armer stays NO-GO**: the walk model is GO (FU316),
  but a natural reach needs **13 same-direction pans** at the measured seed
  (`T(0x40) = 0x90`; 12 pans land exactly on `0x6C0`), every measured window
  ceilings at **2 pans / `0x120`** (scripted, phase-30 T6) or **1 pan /
  `0x90`** (natural, this phase), and arming is transient — it queues a
  situation-2 throw-in whose own place resets the camera.

- **T1 — the natural witness (FU317; `dcf171b`).** `port/tests/test_engine_m2.c`:
  the drive tape is the **verbatim `M2_KEYS` prefix plus a plain DOWN hold
  from tape index 1820** through the 12 000-step window (`M2_CLAIM_*`,
  `:1867-1878`; no `LONG_BALL`, no button — measured inert; the hold must
  start after 1800, and 1810 is also measured safe — FU313-L2/FU317-L1), run
  from the real kickoff on the null backend at 10 ms steps
  (`run_natural_claim_pass`, `:4007-4235`; no `act.id` staging, no
  `begin_isolated`, no camera write; the keeper-ladder observation seam is
  the measured-neutral control seam). The measured pins (all GREEN):
  - phase 2 at step **1 802** (recorded, not hard-coded; match start 5);
  - the claim at **`f=1514`**: `ball.carrier = slot.entity = 2`,
    `pair.code/ack = 0/0` (the row-05 possession block, not the pass pair);
  - the gate at **`f=1512`**: record 2 `code=4`, `lane_x = lane_z = 0`,
    `cam_dz6f = 0`, `bound=2`, `timer81=0`, `has_slot=1`, `dir=(−1,0)`,
    `pos=(0,0,0)`, pool `face7d=0x100`; **`controlled=1`, `slot.entity=2`,
    `tracked=−1`**; the angle tail passes at the **zero-delta degeneracy**
    (bearing `0x80`; the staged and the pool face both fold to Δ`0x80`, which
    is why the pass never depended on the pre-T2 staged constant);
  - the event at **`f=1513`**: `code=5` (the row-05 install), camera
    `vel=(−5,0)`, `speed=5`, `tim=0/12`, `ramp=12`, `trk=2`;
  - the pan: **exactly one delta segment `f=1514…1540`** (27 changed frames;
    the off-zero count `2 084` is recorded and is *never* the count — the
    FU317 §3.4 false-pass trap), stop `f=1541` at `(−0x90,0,0)`, monotone X
    decrease, `pan 0→4`, `speed 5→3→2→1→0`, Y/Z nonzero 0/0; record 2 never
    returns to code 4 (`row04_after_event_frames == 0`, code 5 at the window
    end — the one-shot attribution above);
  - armer/OOB 0 on every frame; the FU282 ladder seam **6 118 calls /
    `ran=0`** (identical to the control);
  - the trace (P16): **1 header + 3 598 body lines**, contiguous `f=`,
    `ev=0x0` and `ph=` on every line, `trk=−1` at the gate and `trk=2` at the
    event and across the delta run, `pan=0..4`, `follow=0` everywhere; the
    in-memory and trace changed-run/off-zero counts agree.
  The undriven control (`run_natural_pass`) is untouched: **3 598/3 598**
  zero camera frames and 6 118/`ran=0` (P15). The RED path is recorded: no
  hold → `claim_f` fails; an off-zero count (2 084) instead of the delta run
  (27) fails.
- **T2 — the row-`0x24` staged-face fidelity (FU314-L2/L5a, the L1a
  prerequisite; `2598ae2`).** Two handler sites:
  (i) the row-04 half-line staging now reads the **live record face**,
  `s.face_word7d = r->face7d` (`port/src/fifa96_engine/fifa96_match_handlers.c:2651`;
  was the constant 0, OL-72) — the native `0x7EE5C SUB AX,word[EBP+0x7d]`;
  (ii) the row-0x24 stage-0 face now passes the **native constant vector
  `(1,0)`** (`handlers.c:5391`, `fifa96_entity_face(1, 0, …)`; was
  `(r->pos_x, r->pos_z)`) — the native `0x865B5 MOV EDX,1` /
  `0x865BE XOR EBX,EBX` / `0x865C6 CALL 0x79C50`. Pins: the frame-path pair
  `test_row04_live_face_passes_face_aligned_gate` /
  `test_row04_zero_face_fails_face_aligned_gate`
  (`port/tests/test_engine_match_frame.c:7646` / `:7664`, the shared setup
  `:7602`) proves the live face opens the face-aligned gate (Δ0) and the
  pre-fix zero face rejects it (Δ`0x100 > 0xAB`); the row-0x24 constant pin
  is `test_action_24_face_constant_vector`
  (`port/tests/test_engine_match_handlers.c:3920`). **M2 `8434cfab…`
  byte-identical — no re-pin.** Reason (recorded, not assumed): the measured
  witness passes at the zero-delta point where the pre-T2 staged `0` and the
  live `0x100` both fold to Δ`0x80`; the row-0x24 call shapes coincide under
  the engine's staging (`player_e = 0` folds `(pos_x,pos_z)` and `(1,0)` to
  `0x100`/octant 2); and the one M2 spot where the old staged 0 was
  load-bearing — the m-42 band's staged south arrival — now stages its
  record **producer** input (`er->face7d = 0`, `port/tests/test_engine_m2.c:2862`),
  declared at the site and not a camera gate. The full-tape diff is empty.
- **T3 — the visible pan (FU317 §5/§6; `fbe36e2`, `13e4e37`, `026a204`).**
  `make game` on `DISPLAY=:1` with `FIFA96_CAMERA_TRACE`, the tape-equivalent
  live drive (RIGHT/KICK/LEFT at ticks 7/14/47/140, the act-1 KICK at 1790,
  DOWN at tick 1917 = phase 2 + 20). The live phase 2 lands at tick **1 897**
  (`f=568`; the null run's 1 802), but the gate/event/motion land at ticks
  **5 044/5 047/5 050** (`f=1512/1513/1514`) exactly as in the null run —
  the walk/claim timing is **absolute, not phase-2-relative**. The trace
  reproduces the natural row-04 pan: clock **00:31…00:32**, place
  `0 → −0x90` over **27 frames** (`f=1514…1540`), `pan 0→4`, `trk=2`,
  Y=Z=0; the full trace is **1 header + 14 984 body lines** (`f=0…14983`,
  1 914 264 B), `follow=0` on all of them. The pan pair
  (`docs/screens/p31-l2-pan-a/b.png`, both displayed **00:31**, 0.25 s
  apart) lies inside the delta run; the still pair
  (`p31-l2-still-a/b.png`, **00:33**, after the keyup at tick 5 270 =
  `f=1580`) is the same run. **Gate 1 (relative, binding): AE_pan 73 402 >
  AE_still 27 849 (2.6×)**, the HUD clock crop AE 0 within each pair.
  **Gate 2 (the whole-scene bbox `864×443`) FAILS and cannot pass in this
  build**: the renderer draws sprites on black (no pitch), so the diff is
  sprite-limited (`317×382`); the AE corroborates a scene change and does
  **not** attribute it — the trace is the attribution (FU317 §5.4;
  FU313-L4/FU305-L6). Evidence: `docs/screens/p31-l2-notes.md`, the five
  PNGs (pan/still pairs + `p31-l2-pan-diff.png`) and
  `docs/screens/p31-l2-trace-pan.txt` (84 lines: the phase-2 line, the pan
  window `f=1495…1560`, the still window `f=1578…1590`). The bare-DOWN
  negative run (no M2 prefix; 1 header + 21 798 body lines, no pan) records
  the timing limit and stays uncommitted. **Visible pan — YES,
  trace-attributed.**
- **The wave's verdicts (FU314…FU317, promoted `b85bb05`; errata appended).**
  - **FU314 (the natural triggers) — NO-GO on the undriven tape, GO on the
    chain.** The half-line admission is live (2 081 dispatch record-frames
    inside `lane <= 0x40 − timer81` and `lane <= bound`, `timer81 = 0`), but
    the only failing link on the undriven 12 000-step tape is the `0x7EE7D`
    angle gate (`face 0x100` vs bearing `0x300`, Δ`0x200 > 0xAB`, in
    2 081/2 081); arm A is independently dead (`anim_id == 0` in
    6 120/6 120). The minimal natural trigger is the gate tuple; the live
    routes are the face arms (R1), a band entry from the west (R2) and the
    `0x13` row byte (R3). The face verdict: native-faithful value,
    port-defective data flow (L2/L5a — T2 fixed the two sites).
  - **FU315 (the claim chain) — GO on the natural claim; the P3 cell is not
    the missing link; the claim is not the pan's producer.** The claim is
    already test-pinned live (`test_live_carrier_claim_and_kick`); P3 is
    `dword[rec+0x20]`, bound live from `has_slot`
    (`fifa96_match_entities.c:768`), and the `entities.c:741` "P3 possession
    unported" comment is stale (the actual unported possession cell is
    FU282-L3). The pan's seed is the existing control-slot dir (or, at
    `camY == 0`/no slot, the record velocity words `0x7F056`), so the claim
    is optional for the pan. The row-05 claim tail (`0x7F4E7`, the
    claim-time camera event/tracked bind) is **unported** (L1); the
    slot-pointer projection is the one real port delta (L2).
  - **FU316 (the walk) — GO on the model; NO-GO on the natural armer
    reach.** The tracked cell is a single-slot rotation with exactly three
    writers (`0x71D27` bind; `0x70258`/`0x70054` clears) and eleven
    `FUN_00071C94` event sites, so the `0x120` plateau is not a lifecycle
    cap. Each pan adds the affine `T(s)` (`T(0x40) = 0x90`; the 6/7/7/7
    decay `5→3→2→1`); the armer needs `|x| > 0x730` (13 pans; 12 land on
    `0x6C0`) or `|z| > 0xB20` (20 pans; 19 land on `0xAB0`). The measured
    ceiling is 2 pans/`0x120`; arming yields a situation-2 throw-in that
    re-places the camera (transient). L1 (the no-slot seed/travel) is a
    Stage-3/4 leg; L9 (the unarmed mirror) is new (below).
  - **FU317 (the harness) — GO.** The drive, the trace and the acceptance
    matrix (P11–P18); the honesty boundary: the admission passes at the
    **zero-delta degeneracy** (`lane_z = cam_dz6f = 0` → bearing `0x80`,
    Δ`0x80`; the approach frames carry Δ`0x200`), so the witness is
    knife-edged (L10). The AE is not attribution; the second pan and the
    follow are not claimed.
- **Quoted errata (append-only; the phase-30 text and the wave drafts stand
  as written).**
  1. **The FU314 NO-GO is tape-scoped (the undriven vs the driven tape).**
     Quote (FU314 §8): "**NO-GO on a natural pan with the engine's live
     cells as measured.** … in **2,081 / 2,081** band dispatches."
     **Corrected (FU314 review erratum; FU317 §1/§3.8; the T1 witness):**
     the undriven `M2_KEYS` window never brings a code-4 record to `(0,0)`;
     the DOWN hold from 1820 does (gate `f=1512`, event `f=1513`, pan
     `f=1514…1540`). FU314's 2 081 (dispatch record-frames) and FU317's
     2 082 (the test-side algebra over granted frames) are one counter
     boundary apart, not a disagreement. The face finding stands: the east
     approach fails at Δ`0x200`; the pass is the zero-delta crossing.
  2. **The one-shot is the row-05 install, not the tracked exclusion.**
     Quote (FU317 §7.2): "the same record becomes `tracked`, and the 0x1D
     arm's own exclusion (`0x7EFEE`, `outfield.c:682`) refuses it afterwards
     — measured as `pan_segments == 1`". **Corrected (FU317 review erratum;
     T1):** record 2 is `code 4` at `f=1512`, `code 5` at `f=1513`, and
     `code 5` through the window end; the pan's own tail installs row 5
     (`outfield.c:719`, the native `0x7F133`) and the record never
     re-enters row 04 — that is the proximate one-shot. The `0x7EFEE`
     exclusion is a **latent second guard** for a record that does re-enter
     row 04 (the T1 pin: `row04_after_event_frames == 0`). The row-6
     diversion (`0x7F106..0x7F111` → `outfield.c:712-714`) means pan→row-5
     is not guaranteed: a lane-greater self install goes to row 6 (no
     invoke) and skips the claim.
  3. **The gate's `controlled`/`slot.entity`/`tracked` (the P13 label).**
     Quote (FU317 §3.3, P13): "**the gate tail (f = 1512)** …
     `face7d = 0x100` (bearing `0x80`, Δ `0x80` — the zero-delta pass)."
     **Corrected (FU317 review erratum; T1):** at `f=1512`
     `controlled = 1`, `slot.entity = 2`, `tracked = −1`, and the panning
     record is **record 2, not the controlled record** — the port's
     `is_ball_track` controlled fallback (`handlers.c:2673-2677`) would have
     refused the event if record 2 were controlled (FU316-L3). The gate's
     face operand is the **staged** `s.face_word7d`: pre-T2 the constant 0
     while the pool cell was `0x100`; both fold to Δ`0x80` at the zero-delta
     point (which is why the witness never depended on the staging), and T2
     restaged it live (`handlers.c:2651`). The T1 pin's
     `M2_CLAIM_STAGED_FACE` moved `0 → 0x100`
     (`port/tests/test_engine_m2.c:1878`) and is now a duplicate of production
     (deferred minor).
  4. **The claim follows the pan (the row-04 event is upstream).** Quote
     (FU317 review erratum, of the draft's §7.1): "The claim ... fires the
     row-04 pan." **Corrected (FU317 review erratum; FU315's erratum; T1):**
     the row-04 event is `f=1513`; the first motion and `ball.carrier = 2`
     are `f=1514`; the pan's seed is the **existing** control-slot dir
     (`slot.entity = 2`, `dir = (−1,0)`), not a new possession. The
     acceptance wording is "a natural row-04 pan that also produces the
     claim" (the T3 note title). The claim chain is not a pan producer;
     FU315's possession slice is follow/claim-frame work (L1/L2 out of
     phase-31).
  5. **The row-6-self install correction (FU316 §3.2).** Quote (FU316
     §3.2): "`0x7F125` is reached **unconditionally** … so *any* record that
     passes the half-line gate … leaves row 04 with row 5 installed."
     **Corrected (FU316 review erratum):** the `> lane` arm at
     `0x7F106..0x7F111` installs **row 6 on self** (no invoke) and skips
     `0x7F125`; the measured witness took the row-5 path (`code 5` at
     `f=1513`), so the T1 one-shot pin stands, but the re-arm model must
     carry the diversion (`outfield.c:712-714`; errata item 2).
  6. **FU310-L9's closure attribution (FU316 §7.1).** Quote (FU316 §7.1):
     "**FU310-L9 (the pan-cumulative armer spike) — closed by this
     slice.**" **Corrected (FU316 review erratum):** L9 was closed by the
     phase-30 T6 measurement (`port/ENGINE.md:2582-2599`: the `0x120`
     ceiling in seven scripted drives, "no producer and no new leg"); FU316
     supplies the **mechanism** (the three blocks and the `T(s)` model), not
     the closure. FU316-L1 is a **new** leg (the no-slot seed/per-pan
     travel), not a re-open.
  7. **The mirror blocks the armer, not the pan (FU316 §5.1).** Quote
     (FU316 §5.1): "a reflect-bit config blocks the pan route to the
     armer". **Corrected (FU316 review erratum):** the mirror
     (`|x| > 0x720`, `|z| > 0xB10`) sits below the armer (`0x730`/
     `0xB20`), so a set reflect bit blocks the **armer**, not the pan; the
     measured run has `input_bit0 = 0` (`match_run.c:468`) and the witness
     `|x| = 0x90`, so the bit is irrelevant to the pan. The port's
     `fifa96_match_goal_arm` calls `fifa96_camera_reflect` only in the
     already-armed branch (`match_run.c:3200-3212`); the native mirror is
     also reached on the unarmed armer-gate fail path — **FU316-L9** (no
     armer work this phase).
- **The legs — met, open and out of phase.**
  - **FU314-L1 — MET for the one-pan witness** (it carried FU310-L5): the
    natural route fires the real row-04 half-line arm and the place leaves
    `(0,0,0)` (`f=1514…1540`, `−0x90`); no camera cell was staged.
  - **FU314-L1a — OPEN (Stage-3 leg): the face-aligned/robust natural
    witness.** Its prerequisite landed (T2: the live record face + the
    native row-0x24 `(1,0)`), but the natural **non-zero-delta** witness is
    not produced — the measured pass is the zero-delta crossing. Owner:
    FU314-L3 (the `+0x7D` producer coverage — the `0x8E1CB` frame-wrap arm
    gated on the animation row byte `>= 0x70`, unported) + FU314-L5 (the
    mover `direct_face` staged 0, `match_run.c:498`) + a west-side/no-slot
    drive (FU314 R2).
  - **FU316-L1 — OPEN (Stage-3/4): the no-slot seed distribution and the
    per-pan travel.** `0x7F056` (`word[+0x73]<<2`); no measurement covers
    the larger seeds (`s = 0x80 → T = 0x174`); the armer count is
    `ceil(0x730 / T(s))` pans. Owner: `fifa96_outfield.c:692-701` +
    `fifa96_outfield_row04_step`.
  - **FU316-L9 — OPEN (new; armer-only): the unarmed mirror.** The native
    `0x713C0 JLE 0x718A9` mirror path is unmodeled; the port reflects only
    once `goal_armed` is set (`match_run.c:3200-3212`), so the port would
    arm where the native mirrors. No armer work in phase-31.
  - **FU316-L8 — OPEN (claim/follow fidelity): the row-05 carrier camera
    event is unported.** `0x7F4E7`; `fifa96_action_carrier_arm`
    (`port/src/fifa96_loader/fifa96_action_handlers.c:439`) emits no event. Not
    needed for the measured pan (the pan binds `tracked` at
    `handlers.c:2793`); needed only for a claim-frame bind / the
    pan→row-5→rebind fusion.
  - **FU315-L1/L2 — OUT OF PHASE-31.** L1: the row-05 stage-0 tail
    (`0x7F3A1..0x7F4E7`, the claim event/tracked bind + the release
    countdown; `out.tail` has no engine consumer). L2: the slot-pointer
    projection (the row-0x18 hand-off moves the native pointer + dirs; the
    port swaps only `has_slot`). Claim/follow fidelity, not pan producers
    (FU315's GO stands).
  - **Carried unchanged:** **FU307-L8** (the row-`0x1E` claim stage-3 /
    closedown stage-7 raw X/Z delivery gap on a direct stage start); the
    phase-30 remainder as registered there (**FU307-L1 (NO-GO)/L2/L5/L6**,
    **FU311-L1..L4/L5a**, **FU312-L1..L7**, **FU313-L4/L5**, **FU310-L4..L7**,
    the open **FU309** legs); this wave's port-fidelity legs
    **FU316-L2..L7** (the landing pan-step disjunct, the controlled
    fallback, the unguarded bind, the scanner side source, the armed body,
    the natural pan rate); and the FU317 measurement legs
    **FU317-L1..L7/L9/L10** (the drive boundary, the no-re-derivation rule,
    the one-shot, the claim-vs-pair, the pad axis, the AI-only pan, the
    visible-shift geometry, the interactive timing, the zero-delta knife
    edge). The follow/Y waiver stands (the phase-30 circular route; no
    `>0xF0` event is produced).
- **Deferred minors (T1–T3 reviews; not fixed here; none is a fidelity
  defect).** T1: the "verbatim prefix" `memcmp`
  (`port/tests/test_engine_m2.c:4017-4018`) is construction-true and has no
  static bound (a future `M2_KEYS` edit still needs the measured re-run);
  the staged-face pin duplicates production by hand
  (`M2_CLAIM_STAGED_FACE`, `:1878`); some FU317 §4.3 sampler items are
  omitted (the nearest code-4 census, the goal-zone cell — recorded fields
  only); the gate/event frame pins are ring-derived (the two-frame ring,
  post-frame sampling); the scope is +490 LOC vs the ~200 estimate.
  T2: the stale `handlers.c:2646` cites (`port/tests/test_engine_m2.c:1866-1875`
  and `:4117-4119` say 2646; the staging is `:2651`); the
  `M2_CLAIM_STAGED_FACE` label is now tautological; the frame-path helper
  lacks the `controlled` assertion; the negative rider's mutation is not
  executed in CI; the site comment's receiver-gate note; ~40 lines
  duplicated between the two frame pins; the M2 m-42 fixture's
  insensitivity (its staged `face7d=0` would also be the pre-fix constant,
  so that fixture cannot detect a re-staging regression). T3: the report's
  PNG-size phrasing (an out-of-diff cosmetic; the captures and the AE
  numbers are unchanged).
- **Tape and gate.** M1 `09b726b7…` and M2 `8434cfab…` (295 087 B)
  byte-identical through T1–T4 (`cmp`-verified; no re-pin); `make check`
  **111/111** (strict, ASan/UBSan); census **`60/80 = 75%`** (`37 action +
  23 phase`); no README/AGENTS change. The trace sink remains default-off
  (the M2 stdout is `cmp`-identical with and without the env), and no
  `make game`/capture artefact outside `docs/screens/p31-l2-*` was added.

---

## M2 phase-31 close-out errata (T4 review, 2026-10-22)

The phase-31 register above stands as written; this append-only section
corrects the T4 review's three documentation-accuracy Importants and the
same-area precision minors. **Docs-only:** no code/test/golden change;
`make check` **111/111** (strict, ASan/UBSan); M1 `09b726b7…` and M2
`8434cfab…` unmoved; the census holds **`60/80 = 75%`**. The verdict (the
natural pan is real, one pan, trace-attributed; the follow waived; the armer
NO-GO) is unchanged.

**1. The `FU310-L5` contradiction (Important 1).** Quote
(`port/ENGINE.md:6091-6092`): "**FU314-L1 — MET for the one-pan witness** (it
carried FU310-L5)". Quote (`:6126`, the carried list):
"**FU310-L4..L7**". **Corrected:** a met L5 and an "open L5" cannot coexist.
**FU310-L5 is MET for the one-pan witness** — the same scope as FU314-L1: the
natural route's single pan (`f=1514…1540`, `−0x90`), carried by the T1
witness. **The carried list is `FU310-L4/L6/L7`.** L5's broader/robust
continuation is **FU314-L1a** (the face-aligned/non-zero-delta route), which
stays open.

**2. The `2 084` counter vs the `2 082`/`33` algebra trap (Important 2).**
Quote (`:5908-5910`): "the off-zero count `2 084` is recorded and is *never*
the count — the FU317 §3.4 false-pass trap". **Corrected:** these are two
different counters. **`2 084` is the driven run's off-zero count**:
`port/tests/test_engine_m2.c:3981` increments `off_zero_frames` on
`pos != (0,0,0)`, and `:4141-4142` asserts
`off_zero_frames == granted_frames − first_motion_f`; the delta-run rule
(27 changed frames, `:4137-4140`) is the count, and the off-zero/27 check is
T1's own RED-2 trap. **The FU317 §3.4 false pass is the test-side algebraic
half-line predicate**, true on **2 082** granted frames of the undriven
control (which never pans) vs **33** driven frames — the warning is "never
re-derive the gate algebra in the test" (FU317-L2), not the delta-run rule.
The register conflated the two.

**3. The Layer-1 certification fixture (Important 3).** Quote
(`:5877-5878`): "**Layer-1 GREEN** through the headless witness (T1) and the
frame-path face pair (T2)". **Corrected:** the spec §5 same-chain fixture is
**`test_live_claim_pan_row04_half_line`**
(`port/tests/test_engine_match_frame.c:2610`; FU317 §6 H5): the real frame path
with `begin_isolated` (the fixture allowance), kickoff → KICK → phase 2 →
LONG_BALL → UP, asserting the claim (`ball.carrier == id`, `:2647`), the
tracked bind (`:2652`), the `0x90` stop and the `5→3→2→1→0` decay
(`:2656-2675`). **Layer-1's certification = the T1 headless witness + that
fixture.** The T2 pair (`test_row04_live_face_passes_face_aligned_gate` /
`test_row04_zero_face_fails_face_aligned_gate`, `:7646`/`:7664`) is the
**added face pin**, not the same-chain fixture: it runs one granted frame and
stops at the row-05 code install (`e->code == 5`), never the claim.

**4. `FU317-L8` — settled by T1 (Minor 1).** Quote (`:6131`):
"**FU317-L1..L7/L9/L10**". **Corrected:** **FU317-L8 (the probe cost) is
settled by T1**, not dropped silently: the extra 12 000-step natural pass
landed (one run, ~+9 s measured; `test_engine_m2` 65.90 s in the final suite).
The carried set is L1..L7/L9/L10.

**5. `FU311-L1` — closed by the phase-30 T2 bind pack (Minor 2).** Quote
(`:6126`): "**FU311-L1..L4/L5a**". **Corrected:** **FU311-L1 is CLOSED** —
the phase-30 T2 bind pack (`port/ENGINE.md:2524-2543`; the code at
`port/src/fifa96_engine/fifa96_match_entities.c:784-785`:
`lane69 = (uint16)e->lane | ((uint16)e->lane_x << 16)`, native
`0x78032/0x78035`) refreshes the high word from `e->lane_x`, which was
FU311-L1's stale-cell defect. The carried list is **`FU311-L2/L3/L4/L5a`**.
The phase-30 PROGRESS remainder list (`docs/PROGRESS.md:2652`) carries the
same stale L1; this erratum supersedes it (prior text not rewritten).

**6. The named deferred minors (Minor 3).** Quote (`:6149`): "the site
comment's receiver-gate note". Quote (`:6153`): "the PNG-size phrasing".
**Named/corrected:**
- **The receiver-gate note:** the T2 row-04 face-site comment
  (`port/src/fifa96_engine/fifa96_match_handlers.c:2646-2650`) names only the
  `0x7EE5C` half-line angle consumer. It omits the **second `face_word7d`
  consumer**, the `0x7E600` no-slot defender/receiver decision
  (`port/src/fifa96_loader/fifa96_outfield.c:355-388`, the read at `:384`,
  reached from `0x7EC89`/`outfield.c:581`). Comment-level; no behaviour
  change.
- **The PNG-size phrasing:** the T3 report's clock-corrections
  Verification says "identical frames, **byte-identical PNG sizes
  16,675 B** at 55 s"; equal PNG size is not a pixel-identity proof (the
  report also states the frames are identical, so the claim is cosmetic).
  No capture or AE number changed.

**7. The acceptance labels (Minor 4).** Quote (`:6007-6008`): "the
acceptance matrix (P11–P18)". **Corrected:** FU317 §6's acceptance matrix is
**H1–H7** (H1 the natural witness, H2 the claim state, H3 the pan, H4 the
tracked/gate/armer, H5 the frame-path fixture + riders, H6 the visible
capture, H7 the invariants). **P11–P18 are the §4.4 probe assertions** (the
T1 witness carries P11–P17); **P18 is manual** — the nine one-line mutation
riders are not in CI (FU317 §4.5; FU309-L9 stays open). The register's
"acceptance matrix (P11–P18)" conflated the two labels.

**8. The hold-start boundary (Minor 5).** Quote (`:5891-5892`): "the hold
must start after 1800, and 1810 is also measured safe". **Corrected:** the
measured-safe starts are **1810 and 1820**; a held tail from **1800
suppresses phase 2**, and **1801..1809 is unmeasured** (FU317-L1). The
register's "after 1800" implied 1801 was safe.

**9. The run-1 count provenance (Minor 7).** Quote (`:5971-5972`): "The
bare-DOWN negative run (no M2 prefix; 1 header + 21 798 body lines, no pan)".
**Qualified:** the 21 798 body lines are from the **uncommitted scratch
trace** `/tmp/opencode/p31-cam-trace.log` (now recorded in
`docs/screens/p31-l2-notes.md`'s appended errata); they are not a committed
artefact. The committed evidence is the acceptance run's
`docs/screens/p31-l2-trace-pan.txt`.

**10. Tape and gate.** No code/test/golden change this round; the goldens
and the suite are re-verified (see the fix report). M1 `09b726b7…` / M2
`8434cfab…` byte-identical; `make check` 111/111; census `60/80 = 75%`;
README/AGENTS untouched.

**11. The live trace's second changed-place event (final review, Minor).**
Quote (`:5870-5872`): "`f=1514…1540`, exactly one maximal changed-place
run". **Scoped:** "exactly one" is the **T1 witness window** (the 27-frame
delta run, `f=1514…1540`, inside the 3 598-frame null natural pass). The
**full live acceptance trace** has a second changed-place event at
**`f=4170`**: `f=4168` still reads phase `12` at `pos=-144,0,0`; `f=4169`
reads phase `1` at the same place; `f=4170` reads `pos=0,0,0`, `trk=-1`,
`pan=0`, `tim=0/0` — the **phase-12 → phase-1 camera reset to zero**, not a
pan. The T1 `cl->segments == 1` pin is window-scoped; the full tape has one
pan and one reset.

**12. `FU311-L1`'s label (final review, Minor).** Quote (item 5 above,
`:6223-6224`): "which was FU311-L1's stale-cell defect". **Corrected
framing:** the `lane69` bind's high word is **0/−1 by construction** (the
sign-extended `+0x69` word), per the phase-30 erratum (`:2677-2684`); the
phase-30 T2 bind pack (`port/src/fifa96_engine/fifa96_match_entities.c:784-785`)
refreshes the high word from `lane_x`. The leg is **CLOSED** as item 5 says;
the "stale-cell" label is superseded by the "0/−1 by construction" framing,
and the carried list stays `FU311-L2/L3/L4/L5a`.

---

## M2 phase-32 register (FU318…FU321; 2026-10-22) — the non-zero-delta pan: the first offset pan (drive-only; the live face deferred)

**Scope/provenance.** Phase-32 spec
`docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md`
(§5–§9), producer plan
`docs/superpowers/plans/2026-10-22-fifa96-m2-phase32-producers.md` (T1–T3) and
the reviewed four-slice wave `port/spec/FU318_r2l6_face_producers.md` …
`FU321_r2l6_nonzero_acceptance.md` (promoted one-per-commit: `9b5c8c0`
FU318, `e2dae90` FU319, `2736973` FU320, `19b0db2` FU321; the wave-review
errata are appended to each — FU318's staged-metric erratum, FU321's `|v0|`
erratum + C1/C2 check). Task commits: T1 `6fb8062`, T2 `c139c15`. Append-only:
the phase-31 text and every prior section stand as written; the corrections
below are quoted errata. Census **`60/80 = 75%`** (`37 action + 23 phase`; no
row joined); M1 `09b726b7…` and M2 `8434cfab…` (295 087 B) byte-identical
(`cmp`-verified; the phase lands no production change); `make check`
**111/111** (strict `-Wall -Wextra -Werror`, ASan/UBSan). The README title
stays `75%`.

- **The verdict — the first natural non-zero-delta row-04 pan; drive-only; the
  live face deferred.** The row-04 half-line arm fires from the natural route
  (the real kickoff, the engine's own phase 2, the live record/camera cells,
  **no staged camera/gate/face**) with the record **offset** from the parked
  camera: at the passing dispatch (trace key `f=1528`; the dispatch at `f+1`
  evaluates it, FU320-L6) record 2 is at `pos (−32,0,16)`, `lane 36`,
  `bound 36`, `cam_dx +32`, `cam_dz −16`, `face7d 0x100`, bearing `0x14C`,
  Δ`0x4C` ≤ `0xAB`, `has_slot 1`, `dir (−1,0)`, `tracked −1`, `controlled 1`,
  `slot.entity 2`, camera `(0,0,0)`, `camY 0`. That is `pos ≠ cam` and a real
  bearing — the FU321 §3.8/G3 signature (`lane_at_pass ≥ 1`; measured 36),
  not the phase-31 zero-delta knife edge. The pan itself is the **same
  slot-arm `T(s)` class** as phase 31: place `0 → −0x90` over **27 granted
  frames**, `pan 0→4`, `trk=2`, Y=Z=0, `follow=0`; the seed is the slot dir
  `(−1,0) << 6 = −0x40`, not the gate bearing, so the offset changes *where*
  the admission happens, not the pan axis. **No production change** (the
  FU321 Case A): the goldens and the suite are unmoved. The offset is a
  **Layer-1** claim — the live trace carries camera cells only
  (`port/src/fifa96_engine/fifa96_match_run.c:4591-4607`, FU321-L1); Layer 2
  reproduces the drive and the arithmetic. The **live-face fidelity is
  deferred to phase 33** (the ruling below).
  - **The drive:** `M2_KEYS` verbatim entries `0..1799`; **LEFT
    1820..1849**; **DOWN 1850..end**; 12 000 steps, 3 598 granted frames.
  - **The honesty caveat — plateau-riding, not a robust crossing.** Every
    measured pass (`L20…L80`, `R20…R80`) has `lane == bound` on an **integer
    plateau** of the octagonal metric, and the nominated pass rides the branch
    switch at `|x| = 2z` (`(2z−2) + 3z/8 = 2z + z/4 = 36` at `z = 16`). The
    record is inside the band west of the camera for `f=1513…1528`, but
    `lane > bound` refuses 12 of those frames and the cone opens only after
    `|x| ≥ |z|/1.75`; the eastbound entry is unreachable with the static `+x`
    face. **A robust approach-side crossing needs the live-face producer**
    (FU320-L1/L3, FU319); this witness is geometry-luck until then.

- **T1 — the natural witness (FU320; `6fb8062`).** `port/tests/test_engine_m2.c`:
  `run_natural_offset_pass` (`:4279`) beside the untouched T1 claim pass
  (`run_natural_claim_pass`, `:4031`; called `:4580`, the offset pass
  `:4583`); the drive constants `M2_OFFSET_*` (`:1884-1888`), the structural
  tape asserts (`:4291-4315`), the offset pin (`:4386-4392`), the gate tuple
  (`:4394-4429`), the event/claim (`:4431-4447`), the delta run
  (`:4449-4475`), the trace cross-check (`:4477-4547`). Pins (recorded, not
  derived; no staging): gate `f=1528` — the tuple above, with `bearing 0x14C`
  recomputed only by the port's own `fifa96_entity_angle` and the exact fold;
  event `f=1529` — `code 5`, `dir (−1,0)`, `vel (−5,0)`, `spd 5`,
  `trk 2`, `tim 0/12`, `ramp 12`, `ev 0`; claim `f=1530` — `carrier 2`
  (`slot 2`, `pair 0/0`); **one delta run `f=1530…1556`** (27 changed
  frames), stop `f=1557` at `(−0x90,0,0)`, `pan 0→4`, speed
  `5→3→2→1→0`, monotone X (27 decrease / 0 increase), Y=Z=0,
  `row04_after_event_frames 0`, armer/OOB 0; the off-zero count 2 068 =
  `granted_frames − first_motion_f` and **not** the run count (the FU317
  §3.4 trap); the trace **1 header + 3 598 body lines**, contiguous `f`,
  `trk=−1` at the gate and `trk=2` across the run, `follow=0` and `ev=0` on
  every line. The RED is the offset pin on the T1 DOWN drive:
  `lane_at_pass == 0` at `f=1512` (`pos (0,0)`, bearing `0x80`) — the
  zero-delta case FU321 §3.8 forbids. The undriven control
  (`run_natural_pass`, `:3634`) stays 3 598/3 598 zero.

- **T2 — the Layer-2 acceptance (FU321; `c139c15`).**
  `docs/screens/p32-l2-notes.md`, the five PNGs
  (`p32-l2-pan-a/b.png`, `p32-l2-still-a/b.png`, `p32-l2-pan-diff.png`) and
  the trace extract `docs/screens/p32-l2-trace-pan.txt` (80 lines = 4
  comments + 1 header + 75 body: the phase-2 line, the gate/event/pan window
  `f=1520…1562`, the still window `f=1685…1710`, the half-time reset
  `f=4138…4142`). The run: `make game` on `:1` with `FIFA96_CAMERA_TRACE`;
  the M2 prefix replayed by trace tick and FU320's tail scheduled on the
  **wall clock** at phase 2 + 20 ticks (a first run with a lossless 60 fps
  clip encoding compressed the LEFT window to 31 ms / ~3 ticks and replayed
  the T1 **zero-delta** timeline instead — the FU321-L4 limit; the accepted
  run removed the encoder). Live phase 2 at tick **1 800** (`f=539`); the
  gate/event/motion/stop at ticks **5 097/5 100/5 104…5 190/5 194** =
  `f=1528/1529/1530…1556/1557`, **tick-for-tick with the null trace**. The
  trace: place `0 → −0x90` over 27 changed frames, `pan 0→4`, `trk=2`,
  `pos_y=0` and `follow=0` on all **15 043** body lines (1 921 636 B;
  sha256 `aa887570…`); the only other changed-place frame is the half-time
  reset `f=4141` (`ph 12→1`, camera back to `0,0,0`, `trk=−1`, `pan=0` — a
  period reset, not a pan; included in the extract). **Gate 1 (relative,
  binding): AE_pan 63 720 > AE_still 0**, same session; the HUD clock crop
  AE 0 within each pair; the pan pair at displayed **00:33** (0.215 s apart,
  inside the delta run) and the still pair at **00:38** (camera constant at
  `−144`). **Gate 2 (whole-scene bbox) fails and cannot pass in this build**:
  `335×397+281+0` vs `864×443`, because the renderer draws sprites on black
  (no pitch) — the documented sprite-only caveat, never an attribution. The
  **trace attributes**; the AE corroborates; the offset stays the Layer-1
  tuple (FU321-L1).

- **The wave's verdicts (FU318…FU321, promoted one-per-commit).**
  - **FU318 (the `+0x7D` face census) — GO on the census and the production
    contract; NO-GO on the L3 helper-site/frame-wrap route as the walker's
    natural producer; GO-conditional on FU319.** All 40 `FUN_00079C50` call
    sites are classified (**8 A** always-both, **1 B** constant `(0,0)` no-op,
    **31 C** runtime vector) and the port's split is real: `fifa96_arm_face`
    is sector-only (10 sites) while `fifa96_entity_face` is the true
    both-cell helper (5 engine rows + 4 keeper sites with the face dropped +
    1 unwired row + 1 kick inline). The frame-wrap arm `0x8E1CB` fires only
    at the two row wraps `0x14→0x7F` / `0x52→0x7E` (2 of 111 rows), so it is
    not the walker's producer; the per-frame producer is the **locomotion
    turn**, gated by the installed row's `flags & 2` (`+0x43`) — **38 of 111
    rows**, including the plausible initial rows `0x00`/`0x26` and the walk
    row `0x13`.
  - **FU319 (the `+0x43` producer) — GO on the producer and the focus
    staging, with a broad intended M2 move; NO-GO on the two shortcuts.**
    The producer is singular: `+0x43` is written at exactly three record sites
    (`0x6E6C4` install, `0x8DFBA` pool twin, `0x8540C` row-0x22 clear), read
    only by the turn twins (`0x7BF89`/`0x8E29C`), and its value is **2 for
    every record after the kickoff placement** (rows 0/0x26 flags `0x07`).
    The minimal staging shape (§5): a `row43` latch beside `row44`, written
    by the two installs + the kickoff placement, cleared at `0x8540C`; the
    mover stages `direct_face = row43` and `face_x/face_z` from the camera
    triple `0x15774C` (the default `[0x157A73]` focus). The M2 move is
    **broad**: 38/111 rows armed; the open-loop replay turns on 4 140/67 320
    record-frames (6.1 %) with 20 distinct steady-state faces; the band record
    settles at face `0x300` = the camera park, so the **undriven** tape would
    pan and the natural-pass zero-camera baseline
    (`port/tests/test_engine_m2.c:3722-3724`) would flip **by design** — the
    P7/P8 window/lockout assertions must land with the inversion (FU321-L11).
    The two NO-GOs: deriving `direct_face` in the mover from `anim_id` (it
    loses the latch across the `+0x28 = 0` reset / row-0x22 clear, FU319-L2)
    and folding the `move_attr` metric fix (`0x8DCD4` vs `0x8DC68`) into the
    same pin (FU319-L3; 2 record-frames straddle `0x60`, max |Δ| 115).
  - **FU320 (the drive) — GO on the no-code R2 witness; NO-GO on calling it
    a robust crossing.** The 31-run sweep nominates `LEFT 1820..1849` +
    `DOWN 1850..end`; the static-face pass set is the west cone
    (`dx > 0`, `4|dz| < 7dx`, with `angle(0,0) = 0x80` the only tie); the
    measured pass is plateau-riding (`lane == bound == 36`); the eastbound
    entry is unreachable with the static `+x` face; the near-misses are
    measured (`L10` Δ`0xB4` at its only angle-ready frame; `L120` never
    inside the band; a released hold stops at `x = 1828`, 900+ units short).
    The pan is X-only because the arm-B seed is the slot dir, not the bearing
    (FU320-L5); with a live face the east approach would pass at `lane 0x40`
    with Δ`0` (FU320-L3, a prediction, not a measurement).
  - **FU321 (the acceptance and the M2 policy) — GO; C1/C2 satisfied; the
    `|v0|` erratum.** The pan's place delta is **seed-determined, not
    offset-determined**; the closed-form `T(s)` reproduces five published
    instances exactly (`0x90/0x140/0x174/0x1FA/0x3B8`); the trace cells,
    G1–G8, the Case A (drive-only: byte-identical) / Case B (producer: frame
    diff + written reason + register pin) accounting and the fixture hazards
    (the m-42/link fixtures, the additivity tautology, the multi-consumer
    face) are the phase-33 M2 authority. The review erratum corrects C2/G4's
    `v0 ≥ 1` to **`|v0| ≥ 1`**; the nominated seed `−0x40` gives
    `v0 = trunc(−0x40/0xC) = −5`, `speed 5`, so C1/C2 hold.

- **The plan ruling — drive-only phase 32; the live face is phase 33.** The
  spec §8.1/§9.4 made FU314-L5 (+ the row-local L3) conditional on the wave
  proving it the natural/robust route and the M2 diff faithful. The wave
  proves the **producer** (FU319) but its move is a **universal fidelity
  change** — 38/111 rows; the undriven tape would pan; the natural-pass
  zero-camera baseline inverts — broader than this phase's pan scope. The
  plan therefore lands the **drive-only** FU320 R2 witness (Case A) and
  **defers FU314-L5 + the row-local L3 to phase 33**, with **FU319 §5/§6**
  (the fields, the staging shape, the expected sim/M2 diff) and **FU321
  §5/§6** (the faithful-move accounting and the hazards) as its starting
  authority. Cost if wrong: the witness remains plateau-riding geometry-luck
  (FU320-L1) one more phase; no fidelity claim is made for the live face.

- **Quoted errata (append-only; the wave drafts and the phase-31 text stand as
  written).**
  1. **The `|v0|` criterion (FU321 §1/§9 C2/§4.6 G4).** Quote: "a seed with
     `v0 ≥ 1`". **Corrected (FU321 review erratum 1):** the seed is signed —
     the nominated FU320 drive is `dir = (−1,0)`, seed `−0x40`,
     `v0 = −5` — so the binding criterion is **`|v0| ≥ 1`** (the event's
     `speed > 0`). As written, C2/G4 would reject the intended witness; every
     other criterion stands. The T1/T2 evidence uses `(|v0|, frames) =
     (5, 27)` and `(|Δ|, frames) = (0x90, 27)`.
  2. **The staged `move_attr` metric (FU318 §8).** Quote: "The `move_attr`
     target distance is already staged (`match_run.c:521-522`)". **Corrected
     (FU318 review erratum; cross-slice FU319 §4/§7.3):** it is staged from
     `fifa96_arm_dist_stage` — the `0x8DCD4` octagon — not the native `+0x65`
     metric (`0x8DC68`) the turn's `>0x60` branch reads. FU319 measures 2
     record-frames where the two metrics straddle `0x60` (max |Δ| 115) and
     carries the correction as FU319-L3; the verdict is unchanged.
  3. **The wave's port-line cites predate T1.** FU319 §6/FU321 §5.4 cite the
     natural-pass baseline at `port/tests/test_engine_m2.c:3701-3707`; fresh it is
     `:3722-3724`. FU320 §1's T1 pass `:4007-4263` is fresh `:4031-4269`.
     No claim moves (T1 inserted the offset pass beside them).

- **The legs — met, open and out of phase.**
  - **FU318-L1..L8 —** L1 the 40-site port mapping for the unported rows (the
    row bodies + the animation event machine); L2 the frame-wrap face write
    (`match_run.c:1055-1059` flips the octant only; native `0x8E1CB` writes
    `+0x7D` too) — the animation driver; L3 the `[0x157A73]` live value and
    the turn's arm at the band entry (FU319 owns it); L4 the native anim row
    at the band entry and the turn's `speed` (`word[+0x71]`); L5 the
    keeper/arm face cells (4 keeper sites discard the face; the arm record
    has no `face7d`); L6 the row-0x0D both-cell output dead (FU178-L1); L7
    the alternates (`0x6F90A/0x6F945`, `0x81BE3`, `0x8E691`/`0x8E9EE`);
    L8 the FU314 stale line numbers (superseded by the fresh sites; FU314
    is not edited).
  - **FU319-L1..L7 —** L1 the non-default focus pointers (the foul spot
    `0x1587C0`, the restore `0x8A3B9`, the phase setters); L2 the "row
    pointer NULL, latch set" state (`FUN_0007997C` clears `+0x28` without
    `+0x43`); L3 the `move_attr` metric (a separately pinned sub-change);
    L4 the frame-wrap install's frame position (the successor runs from the
    scene stage); L5 the sibling flag cells (`+0x45`, `+0x3F`); L6
    **checked, not a leg** (the face-range parity; re-open only on a measured
    wrap edge); L7 the M2 blast radius (Stage-2 must produce the actual
    diff).
  - **FU320-L1..L8 —** L1 the plateau-riding admission; L2 the `z` offset is
    tape-timing coupled; L3 the producer-route prediction (the T1 east
    approach passes at `lane 0x40`, Δ`0`, with the walk-bearing face;
    unmeasured on the tree); L4 the second half/restart/defender over-run
    unreachable in the 3 598-frame window; L5 the pan stays X-only; L6 the
    sampling boundary (`f` is the trace key; the dispatch at `f+1` evaluates
    it); L7 the controlled-record refusal (`is_ball_track` fallback; the
    witness's record 2 ≠ controlled 1); L8 the pad→target model.
  - **FU321-L1..L11 —** L1 the trace cannot prove the offset; L2 the
    zero-seed silent event; L3 the no-slot `T(4w)` instances are formula-only;
    **L4 the live timing** — exercised and recorded by T2 (live phase 2 tick
    1 800; the gate/event/motion/stop tick-for-tick with the null; the
    lossless-clip collapse of run 1), open as a general rule (the phase-2
    landing drifts: −3 ticks here vs +95 in the phase-31 T3 run; schedule on
    the wall clock, no encoder); L5 the m-42 fixture's re-staging
    insensitivity; L6 the additivity gate is not a re-pin guard; L7 the
    visible shift scales with the seed; L8 the period-staged witness
    (conditional, unused); L9 the claim may be absent (the witness's claim is
    that drive's, not the pan's requirement); L10 the face is a
    multi-consumer cell (the strongest M2 collateral); L11 the natural-pass
    baseline asserts flip with a universal producer.
  - **FU314-L1a — the witness half is MET; the robust/live-face half stays
    open.** The natural non-zero-delta pan is measured (T1 Layer 1, T2 Layer
    2) with the live record face read, never staged; but the face is still
    static, so the robust approach-side crossing (FU320-L3) and the producer
    fidelity (FU319) are **phase 33**. The phase-31 one-pan witness
    (`f=1514…1540`) remains the zero-delta witness beside the new offset pass;
    the undriven `run_natural_pass` is the zero control.
  - **Carried unchanged:** FU307-L8; the phase-30 remainder as registered;
    the phase-31 fidelity legs FU315-L1/L2, FU316-L1..L9, FU317-L1..L7/L9/L10
    (the follow/Y waiver stands; no `>0xF0` event is produced).

- **Deferred minors (T1/T2 reviews; not fixed here; none is a fidelity
  defect).** T1: the near-verbatim harness duplication between
  `run_natural_offset_pass` and `run_natural_claim_pass`; the gate resting
  cells (`vel/spd/tim/ramp`) not pinned; the ladder counters recorded but
  unused; `trk` not checked after the stop frame; the plateau caveat absent
  from the site comment. T2: the still pair is ~4.6 s after the stop (the
  note says ~2 s); the still PNGs are byte-identical (two distinct grabs of a
  static framebuffer) and the note should say so; the pan pair also shows a
  pose change (the trace attributes the camera translation); the offset tuple
  should cite Task 1 by name; `goal_armed = 0` should be stated; the extract
  arithmetic is 80 lines = 4 comments + 1 header + 75 body; the scope line
  omits the trace extract; the `+20` label vs the ≈`+22` wall-derived row; the
  watcher-name parenthetical is backwards; the live/null `f=` is
  run-relative; the `make check` citation points at the scratch log.

- **Tape and gate.** M1 `09b726b7…` (22 004 B) and M2 `8434cfab…`
  (295 087 B) byte-identical through T1–T3 (`cmp`-verified; no re-pin);
  `make check` **111/111** (strict, ASan/UBSan); census **`60/80 = 75%`**
  (`37 action + 23 phase`); no README/AGENTS change; no code/test/golden
  change in phase 32's Stage 2. The trace sink stayed default-off outside
  the acceptance run; the capture/trace artefacts are
  `docs/screens/p32-l2-*`.

---

## M2 phase-32 close-out errata (T3 review, 2026-10-22)

The phase-32 register above stands as written; this append-only section
corrects the T3 review's three Important record-accuracy defects and the
same-area minors. **Docs-only:** no production code/golden change; the
phase's only code change is T1's test-side witness (`6fb8062`, +349 lines);
`make check` **111/111** (strict, ASan/UBSan); M1 `09b726b7…` (22 004 B) and
M2 `8434cfab…` (295 087 B) unmoved; the census holds **`60/80 = 75%`**. The
verdict (the first **natural** non-zero-delta pan; drive-only; the offset is
Layer-1; the live face is phase 33) is unchanged.

**1. The live-timing drift sign (Important 1).** Quote (`:6516-6519`): "the
phase-2 landing drifts: −3 ticks here vs +95 in the phase-31 T3 run".
**Corrected:** the live phase 2 landed at **`f=539`/tick 1 800** against the
null trace's **`f=538`/tick 1 797** — **+3 ticks vs the null trace**
(`docs/screens/p32-l2-notes.md:23`, `:36-37`), or **−2 against the probe's
step 1 802** (the probe's step counter and the trace's `tick` are different
clocks; the reference must be named). The phase-31 `+95` is the live tick
1 897 vs the null 1 802. The T2 report's prose (`task-2-report.md:169`)
copied the phase-31 comparison's sign; its table (`:33`) has it right
("+3 ticks"). No pin moves: the drive is phase-2-relative and the
gate/event/motion/stop ticks are null-identical.

**2. The plateau refusal count (Important 2).** Quote (`:6338-6339`): "the
record is inside the band west of the camera for `f=1513…1528`, but
`lane > bound` refuses 12 of those frames". **Corrected:** the 16 frames
split **2 angle fails** (`f=1513` lane 16/16, `f=1515` lane 17/17), **13
lane refusals** (`f=1514` alone, lane 17/16, plus the 12-frame
`f=1516…1527` block), and **1 pass** (`f=1528`, lane/bound 36/36) —
`port/spec/FU320_r2l6_nonzero_drive.md:92-98`. The "12" is the block's
length, not the refusal count. The plateau-riding conclusion stands.

**3. "No code/test/golden change" (Important 3).** Quote (`:6555-6556`):
"no code/test/golden change in phase 32's Stage 2". **Corrected:** the T3
commit is docs-only, but the phase's Stage 2 is **not** code-free — T1 added
the test-side witness (`port/tests/test_engine_m2.c`, **+349 lines**, `6fb8062`;
the register's own T1 bullet, `:6344-6345`). The accurate formula is: **no
production code/golden change; the only code change is T1's test-side
witness (`6fb8062`, +349 lines)**.

**4. The errata scope (Minor 1).** Quote (`:6301-6304`): "the wave-review
errata are appended to each". **Corrected:** only **FU318** and **FU321**
carry `## Review erratum (wave review, 2026-10-22)` sections; **FU319 and
FU320 have none** (their final sections are Known limits / Provenance). The
registered errata are FU318's staged-metric erratum and FU321's `|v0|`
erratum + C1/C2 check. "Appended to FU318 and FU321" is the correct scope.

**5. The half-time reset frame (Minor 2).** Quote (`:6383-6384`): "the
half-time reset `f=4141` (`ph 12→1`, camera back to `0,0,0` …)".
**Corrected:** the phase transition and the place reset are different
frames: **`ph=12` at `f=4139`**, **`ph=1` at `f=4140`** (place still
`−144`), and the **place/`trk`/`pan` reset at `f=4141`** (`pos=0,0,0`,
`trk=-1`, `pan=0`) — `docs/screens/p32-l2-trace-pan.txt:77-79`. "One
changed-place frame at `f=4141`" is right; the `ph 12→1` belongs to
`f=4139→4140`.

**6. The heading (Minor 3).** Quote (`:6294`): "the first offset pan".
**Corrected:** **"the first natural offset pan"** — FU321 E21 records a
**staged** non-zero-delta offset pan in the m-42 fixture (camera park
`(0,0,0xB00)`, record `(0,0,0xAF0)`, lane `0x10`), so the unqualified
"first offset pan" is too broad. The register's verdict already says
"natural"; the heading now matches it.

**7. The 3701→3722 shift cause (Minor 4).** Quote (`:6480-6483`): "No claim
moves (T1 inserted the offset pass beside them)". **Corrected:** the shift
comes from T1's **earlier** insertions — the `M2_OFFSET_*` macros
(`port/tests/test_engine_m2.c:1884-1888`) and the struct/sampler additions — not
from the offset pass at `:4279`. The fresh baseline is `:3722-3724`; "No
claim moves" stands.

**8. The §5 frame-path fixture (Minor 5).** The spec §5's Layer-1 half
requires a frame-path fixture that reaches the same chain (producer inputs
only). The record names it: the existing **`test_live_claim_pan_row04_half_line`**
(`port/tests/test_engine_match_frame.c:2610`; FU317 §6 H5) — the real frame path
with `begin_isolated`, kickoff → KICK → phase 2 → LONG_BALL → UP, asserting
the claim, the tracked bind, the `0x90` stop and the `5→3→2→1→0` decay.
**With no producer landed, no new fixture and no new mutation rider is
added**; the T2 unit pair (`test_row04_live_face_passes_face_aligned_gate` /
`…_zero_face_fails…`) remains the added face pin, not the same-chain
fixture.

**9. Tape and gate.** Docs-only errata; no production code/golden/test
change. M1 `09b726b7…` (22 004 B) / M2 `8434cfab…` (295 087 B)
byte-identical (`cmp`-verified); `make check` **111/111**; census
**`60/80 = 75%`**; README/AGENTS untouched. The verdict is unchanged.

## M2 phase-32 close-out errata (code review, 2026-10-22)

The phase-32 register and its T3 errata stand as written; this append-only
section records the two-axis code review of `c8b1a6c..HEAD` and the fixes it
produced. The verdict (a natural non-zero-delta pan; drive-only; the offset
Layer-1; the live face deferred) is unchanged.

**1. The stale test-line cites (Standards).** Quote (`:6345-6347`):
"`run_natural_offset_pass` (`:4279`) … called `:4580`, the offset pass
`:4583`". **Corrected (fresh at HEAD):** `run_natural_offset_pass` is at
`port/tests/test_engine_m2.c:4285`; the two calls are `:4599`/`:4602`; the offset
pin is `:4397`; the gate tuple's lane pin `:4409`; the claim `:4457`; the
delta run `:4463-4481`; the trace cross-check `:4494-4567`;
`run_natural_claim_pass` remains `:4031`. The shift is the `b72e2b8`
witness-hardening insert — the same class the T3 erratum 3 records
(`:6626-6631`).

**2. The deferred-minors list is superseded (Standards/Spec).** Quote
(`:6538-6553`): "**Deferred minors (T1/T2 reviews; not fixed here…)**",
listing the gate resting cells, the plateau comment, the ladder counters,
the `trk` tail, and the T2 note's ~2 s / byte-identity / scope / +20 /
watcher / run-relative / `make check` items. **Corrected:** `b72e2b8` fixed
those items (the witness hardening; the note's twelve corrections). The
remaining deferred items are the near-verbatim harness duplication (declared
at the site; the extraction is a follow-up), the pre-existing empty-env
guards outside the offset pass (`port/tests/test_engine_m2.c:3863`, `:4061`), and
the trace extract being the committed evidence (the full live trace is
scratch).

**3. The §5 fixture gap (Spec).** Quote (T3 erratum 8): "With no producer
landed, no new fixture and no new mutation rider is added; the T2 unit pair
… remains the added face pin, not the same-chain fixture." **Corrected:**
the existing `test_live_claim_pan_row04_half_line` reaches the row-04
arm/pan chain but pins no `lane`/`bound`/`face7d`/Δ, so it does not exercise
the face-aligned gate this phase is about. The code-review fix closes the
gap with the T2 unit-pin class (spec §9.1 exempt): the positive pair now
records `lane_x 0x40`, `bound 0x40`, `lane_z 0x40` (cam_dx), `cam_dz6f 0`,
`face7d 0x100`, the bearing `0x100` and Δ `0`; the new
`test_row04_opposite_face_fails_face_aligned_gate` pins the direction rider
(the same offset, `face 0x300`, Δ `0x200` → no event, no bind, code 4).
This is a test-side pin, not a gate-staging acceptance fixture; the natural
witness remains the full-engine frame path. The §5 fixture half is now met
by the unit pin + the natural witness + the pan/claim fixture, with the
coverage stated rather than implied.

**4. The deferral reason (Spec).** Quote (the register's plan ruling): the
producer is deferred because it is "larger than this phase's pan scope".
**Corrected:** the spec §8.1's condition is two-part — the natural/robust
route **and** a faithful M2 diff; FU319-L7 leaves the actual M2 diff
unmeasured, so golden-safety was not proven. That unmeasured diff is the
primary condition; the scope breadth is secondary. The deferral stands.

**5. The `docs(phase32)` commit type (Standards, soft).** AGENTS.md's commit
list is `feat`/`fix`/`docs(fuNNN)`/`test`/`chore`; `43e7410`/`4dc8925` use
`docs(phase32)`. The phase-name form has the `docs(fuNNN)` precedent class;
no rewrite (history is immutable).

**6. Test-side fix and the invariant.** This code-review fix changes
`port/tests/test_engine_match_frame.c` only (the tuple asserts, the opposite-face
rider, the call); no production code, no golden. `make check` 111/111
(strict, ASan/UBSan); M1 `09b726b7…` / M2 `8434cfab…` byte-identical; the
census `60/80 = 75%`. The T3 erratum 9's "Docs-only errata; no … test
change" described that commit; this one is test-side.

---

## M2 phase-33 register (FU322…FU325; 2026-10-22) — the live record face: the `+0x43` latch staged live (the silent-bind baseline; the driven two-segment pan; the measured 852 re-pin)

**Scope/provenance.** Phase-33 spec
`docs/superpowers/specs/2026-10-22-fifa96-m2-phase33-live-record-face-design.md`
(§5/§6/§7/§8 + the **Plan erratum (wave close)** — the P1/P2 sketch re-shaped
into the atomic producer commit), producer plan
`docs/superpowers/plans/2026-10-22-fifa96-m2-phase33-producers.md` (T1–T4) and
the recon-ahead plan (`2026-10-22-fifa96-m2-phase33-recon-ahead.md`); the
reviewed four-slice wave `port/spec/FU322_r2l7_baseline_inversion.md` …
`FU325_r2l7_acceptance.md` (promoted one-per-commit: `dbeecdd` FU322,
`a0044e7` FU323, `55a1581` FU324, `6ac277b` FU325; the wave-review errata are
appended to each — FU322's five-item re-shape, FU323's two-item loose-table
erratum, FU324's metcmp-split erratum, FU325's two-item fallback erratum; the
spec's Plan erratum landed `dc335b5`; FU319's phase-33 erratum landed
`8603dcf`; the two-axis review fixes `2b1c3ee`). Task commits: T1 `302b746`
(**the atomic producer commit** — implementation + unit pins + the re-shaped
inversion + the segment-aware witness re-pins + the golden re-pin + the
stratum-set gate + the provenance note, one commit, `fix(fu314)`), T2
`7b3cc10` (`fix(fu319)`, the metric contract pin), T3 `eb9e521`
(`docs(fu325)`, the Layer-2 acceptance — data/captures/trace/note only). This
close-out is docs-only. Append-only: the phase-32 text and every prior section
stand as written; the corrections below are quoted errata. Census
**`60/80 = 75%`** (`37 action + 23 phase`; no row joined — fresh re-count this
close-out: the action table's non-NULL `fn` rows 37 of 45
(`FIFA96_MATCH_ACTION_ROWS`), the phase table's 23 of 35
(`FIFA96_MATCH_PHASE_ROWS`; the `row43` work wires no dispatch row)); M1
`09b726b7…` (22 004 B, 691 lines) byte-identical (`cmp`-verified at every
task); M2 re-pinned in T1 `8434cfab…` → **`d5446144…`** (295 087 B, 6 522
lines — same byte and line counts, hash-only), **byte-identical to FU323's
predicted patched-build sha**; `make check` **111/111** (strict
`-Wall -Wextra -Werror`, ASan/UBSan; ~210 s). The README title stays `75%` —
the census did not move, so no recompute (AGENTS.md's derived rule).

- **The verdict — the live `+0x43` record face and the live focus point are
  landed; the undriven tape's honest baseline is "faces live, the silent
  code-5 bind, camera pos still zero"; the pan claim is carried by the
  re-pinned driven witnesses (two chained segments each, more robust than
  phase 32); the golden moved by the measured 852 chain under the stratum-set
  gate.** FU314-L5 closes (the mover's staged zero no longer suppresses a
  live, universal native behaviour — the latch value is `2` on every record
  after the kickoff placement); FU319-L7 closes (the actual M2 diff is
  measured and fully attributed); FU319-L3's magnitude is falsified (the
  metric sub-change is a measured no-op, FU324-L1). The **natural (undriven)
  pan clause is NOT-GO-as-measured** — the natural route stays the open leg
  with **FU316-L1 promoted to its owner leg** (the no-slot seed/travel).
  - **The landing (FU319 §5 verbatim).** The `row43` pool latch beside
    `row44` in both structs; the three writers — the anim-row install
    (`0x6E6C4` → `fifa96_match_run.c:1068`, `row.flags & 0x02`), the successor
    twin (`0x8DFBA` → `:1108`), the kickoff narrow install (`0x79C13` →
    `fifa96_match_run_record_kickoff_row43`, `:1042-1049`, called at
    `fifa96_match_entities.c:576` — no routed full install, no
    `anim_timer`/`aux_seen` side effects); the one clear (the row-0x22 stage-0
    arm, `0x8540C` → `fifa96_match_handlers.c:5220-5221`, both views); the
    mover staging `s.direct_face = r->row43`,
    `s.face_x/face_z = (int16_t)camera.pos_x/pos_z` (`:524-526` — the default
    focus `[0x157A73] → 0x15774C/0x157754`; the int16 truncation is the native
    word width), with the staging load `:1399` and the drain write-back
    `:1583`. **No `anim_id`-derived `direct_face`** (the rejected shortcut —
    pinned negative by the P-MOV anti-derivation rider), no non-default focus
    pointers (FU319-L1), no metric fold (T2's own commit).
  - **The measured re-pin (FU323 §6's recipe, tripwire-clean).** First
    differing `frame=` line **852** — not a face-event frame directly: the
    faces settle live at **engine step 9** (all 22 records turn from the
    placement constant `0x100` to their live camera bearings), the first
    face-driven position movement is **frame 52** (t0#6, the mover's
    stride-error face term), the first hash-visible frame is **852** (the
    sit-1 half-time kickoff's camera reset to `(0,0,0)` revealing the
    since-372–379 face-driven t1 walk-in sprites); **852 ≥ 52 ≥ 9** is the
    golden rule (FU325's erratum 1 re-words the tripwire to this measured
    chain). **5 671 moved hash-only lines** (contiguous 852..6522), the
    `state=` suffixes and field order byte-unchanged (the cut-diff empty),
    frames 1..851 byte-identical, the line count stays 6 522. Because 852 sits
    **inside** the v21 2 522-line prefix, **Outcome B** applies: the
    additivity gate is **replaced by the stratum-set gate** in the same commit
    (`port/tests/test_engine_m2.c:6971-7039`; the 10-tag registered set — a NEW
    `state=` tag fails the lookup, a LOST tag fails the all-seen assert;
    FU323 measured the stratum set unchanged: `state=1` ×951, `state=19`
    ×2 620, `state=2` ×2 100 = 5 671). M1 is byte-identical on the patched
    engine and on the tree. The house-form provenance note lives at
    `port/tests/test_engine_m2.c:1716-1749` (the shas, the 9/52/852 chain, the
    strata, the anchors crossed — m 41 / the m-42 band / m 58 / the v21
    prefix — and the collateral: "every moved frame is the walk-in/AI-chase
    class the live face drives; no unexplainable frame").
  - **The undriven honest baseline (the re-shaped inversion; FU322's errata
    1–2, FU323 §2/§4, leg FU323-L1).** The patched undriven natural tape does
    **not** pan — native-faithful. The band record (entity 12) holds no
    control slot, so the row-04 seed switch takes the **no-slot arm**
    (`word[+0x73/+0x75] << 2` = ±4 → `v0 = 0`) and the code-5 event at granted
    **f=1517** (trace tick 5060; the sampler's engine step 5065 — different
    clocks, both frozen) is a **silent bind**: `trk=12`, `tim 0/12`,
    `ramp 12`, `vel (0,0)`, camera `pos (0,0,0)` through f=3597; what engages
    is the camera **event state** (`camera_zero_frames` **1517** /
    `camera_left_zero` **2081** — the standing `:3722` assert's measured RED,
    the inversion's own evidence). The landed assert set
    (`port/tests/test_engine_m2.c:3935-3966`, ISO-gated): **A1**
    `camera_left_zero >= 1` (locked; the comment re-worded to the bind), **A2**
    the identity `camera_zero_frames == first_left_zero_f` (measured
    1517 == 1517), **A3** `> phase2_step` + the window
    `M2_NAT_FACE_FIRST_LO/HI` = 4930/5200 (the measured bind step 5065 ± the
    re-based pacing slack — no metric transient exists to cover, FU322's
    erratum 3; the exact step recorded, never pinned). The degraded no-ISO
    window stays recorded-only (`:3967-3989`; FU322-L10). The **A4/A5
    pan-window/lockout pair is void undriven** (`segments == 0` measured) and
    is **relocated to the driven passes**; the undriven bounds (Y/Z,
    armer/OOB) stay recorded-only (not in FU323's undriven dump).
  - **The driven witnesses re-pinned — REAL pans, two chained segments each**
    (the segment-aware authority is FU322's erratum 2; FU323 §5's loose tables
    mix each pass's first dispatch with its LAST segment's cells and are never
    cited directly):
    - **The claim pass** (M2_KEYS + DOWN hold from tape index 1820;
      `run_natural_claim_pass`, `port/tests/test_engine_m2.c:4352`, the re-pin
      block `:4444-4619`): the claim moved 1514 → **1482**; **seg[0]** = the
      row-04 one-shot coda's own pan — gate **f=1480** (code 4, lane 64/−64,
      bound 66, `cam_dz6f` 0, `face7d −0x100`, `has_slot 1`, pos (64,0,0),
      `tracked −1`), bind/event **f=1481** (code 5, vel (−5,0), spd 5, trk 2),
      motion f=1482…1508, stop **f=1509** at **(−0x90,0,0)**; **seg[1]** = the
      **new code-3 walk camera event the frozen world did not have** — gate
      **f=1552** (code 3, lane 128/−128, bound 128, `face7d −0x100`,
      `has_slot 0`, pos (−16,0,0), trk 2), event **f=1553** (code 3, trk 1),
      motion 1554…1580, stop **f=1581** at **(−0x120,0,0)**; totals **54
      changed / 2 116 off-zero** (first off-zero 1482), `pan_max 4`, the decay
      seq truncated at the site (5, 0), `row04_after_event_frames 0`, Y=Z=0,
      armer/OOB 0, the ladder seam 6118/`ran=0`. The gate's folded Δ is **0**
      — the settled live face IS the bearing (the staged/pool constant split
      is void; the zero-delta lane-0/0 gate pin of phase 31 is re-pinned out
      of existence — the measured gate is a real offset entry, FU322-L3).
    - **The offset pass** (the FU320 drive — LEFT 1820..1849 + DOWN 1850..end;
      `run_natural_offset_pass`, `:4720`, the re-pin block `:4884-4992`): the
      claim moved 1530 → **1484**; **seg[0]** gate **f=1482** (code 4, lane
      64/−60, bound 66, `cam_dz6f −16`, `face7d −0x100`, `has_slot 1`, pos
      (60,0,16)), bind **f=1483** (code 5, trk 2), motion f=1484…1510, stop
      **f=1511** at **(−0x90,0,0)**; **seg[1]** gate **f=1552** (code 3,
      **`lane_x 129` = `bound 129`**, `cam_dz6f −4`, `face7d −261`, **Δ=0**,
      pos (−16,0,4)), event **f=1553** (code 3, trk 1), motion 1554…1580,
      stop **f=1581** at **(−0x120,0,0)**; totals 54 / **2 114** (first
      off-zero 1484). **G3's criterion survives, better than predicted:**
      `lane_at_pass 129 ≥ 1` (was the plateau-riding 36),
      `(cam_dx,cam_dz) = (−128,−4) ≠ (0,0)`, Δ=0 — the phase-32
      "geometry-luck until then" caveat (FU320-L1's pinned 36/36 branch-switch
      ride with the static face) is **gone**; both passes assert the
      per-segment `T(s)` closed form from their own recorded seeds
      (`m2_pan_curve`, `:2462` — arm-agnostic, zero-seed-proof) and the A5
      lockout identities.
  - **The unit pins (T1's two-step RED discipline — the RED evidence is the
    staged-zero tree + FU323 §2's probe, which fires the standing `:3722`
    baseline assert on the patched build; the first session's RED captures
    died with the session failure, the GREEN state is the verified record).**
    **P-W1** the install writer (extended
    `test_record_anim_install_row44`, `port/tests/test_engine_match_frame.c:3072`;
    rows `0x00`/`0x26`/`0x80→0` land `row43 == 2`, row `0x5E` the negative
    half); **P-W2** the successor (extended `test_record_row_successor_chain`,
    `:3118`; `0x0F→0x00` lands 2, the terminal no-op keeps it, the mirrors
    clear both ways); **P-W3** the kickoff narrow write (extended
    `test_kickoff_place_face_and_selector`,
    `port/tests/test_engine_match_entities.c:1055` — every placed record
    `row43 == 2`, the kept-special `0x62`/fallback/inactive included; the
    value is pinned, never a site claim); **P-CLR** the clear (extended
    `test_action_22_runs_body`, `port/tests/test_engine_match_handlers.c:4057` —
    the stage-0 clear zeroes BOTH views `:4092-4100`, and the **shared-reset
    rider** `:4067-4072` proves the shared reset does NOT clear the latch);
    **P-MOV** the mover staging (NEW `test_mover_row43_live_turns_toward_focus`,
    `port/tests/test_engine_match_frame.c:3173`) — a settled record turns
    `0x100 → 0x140` in one granted frame toward the **off-origin** staged
    camera `(0,0,0x100)` and the octant cell follows the heading table; the
    executor-confirmed fixture note (`:3168-3172`): the sketch's camera
    `(0,0,0)` could never turn the face — **the port's focus guard is the
    raw-point test (`action_handlers.c:223`) where the native
    `0x8E2B7..0x8E2D0` tests the deltas**; riders: the latch-poke no-turn
    (the anti-`anim_id`-derivation rider), the far-target branch-B rider, the
    clear-freeze rider through the real row-0x22 clear, the re-arm rider
    pinning the **`0x3C` executor limit** (the far target's stride ramped
    `speed71` to 1 → the limit `(0x10−1) << 2`; `0x140 + 0x3C = 0x17C`,
    `:3252-3255`), and the int16 width-contract rider on a twin record;
    **P-TURN** the turn arm's live inputs (extended `test_step_direct_face`,
    `port/tests/test_action_locomotion.c:118` — the **z-only focus arm**
    `:156-163` (`face_z = −1` → desired 0x200, the facing steps to the
    `0x40` limit; the pre-existing pin only exercised `face_x`) and the
    **native-value-2 equivalence** `:168+` (the native byte is 2 and the
    consumer `0x8E29C` is a zero/nonzero test, so `direct_face = 2` is
    identical to `= 1`; GREEN-today by construction — its RED was P-MOV).
  - **The metric contract pin (T2, `7b3cc10`; FU324).** The mover's staged
    `s.move_attr` high word is now the **native block-A distance** —
    `fifa96_entity_distance` (the `FUN_0008DC68` verbatim C port) over the
    record's low-word target-minus-pos (`fifa96_match_run.c:539-541`) —
    replacing the `0x8DCD4`-shaped `fifa96_arm_dist_stage` call (the mover's
    lane output was already unused). **The staging SOURCE changed; the VALUE
    did not** — the two native metric cores are the same computation
    (FU324 §3.3: 800 boundary pairs + 4 000 000 random = **4 000 800 sampled
    pairs, 0 diffs**; the replay over FU319's own dump: 0 diffs), a **measured
    no-op** (0 frames moved alone and 0 composed on the latch build —
    `cmp`-proven; **the goldens untouched**). The contract pin
    `test_step_move_attr_metric_contract` (`port/tests/test_action_locomotion.c:240`,
    beside `test_step_direct_face`) is **GREEN at commit** per FU324 §6 — no
    natural RED tuple exists (the cores agree everywhere) and none was
    manufactured; its guard is FU324's **recorded mutant run**
    (`max + min/3`: 37 219 stride firings in the window, **0 frames moved** —
    the `stride_len ≤ 3` clamp masks every metric difference at the current
    `+0x7B` nibble staging), stated at the assert site
    (`:178-198`). FU319-L3's predicted straddles ("97 vs 96 at 4912/5292, max
    |Δ| = 115") do not reproduce — the named rows give **96 under both
    metrics** (the erratum leg FU324-L1; FU319's phase-33 erratum `8603dcf`
    records it; the leg's *direction* — the native metric is `0x8DC68` —
    stands, and T2 aligned the staging source to it).
  - **The Layer-2 live acceptance (T3, `eb9e521`; `docs/screens/p33-l2-notes.md`
    + the eight PNGs (incl. the two diffs) + the 120-line trace extract).**
    **"The driven pan reproduced live under the live face — YES,
    trace-attributed, both segments, tick-for-tick against the re-pinned
    timeline."** Live phase 2 at `f=539`/tick 1800 (p32's own cells); the
    drive lands seg[0] gate f=1482/tick 4944 → bind f=1483/tick 4947 → stop
    f=1511/tick 5040 `(−0x90,0,0)`, the gap (camera constant `(−144,0,0)`,
    trk 2), then seg[1] gate f=1552/tick 5177 → event f=1553/tick 5180 (trk
    re-binds 2→1, the pan counter restarts 0) → stop f=1581/tick 5274
    `(−0x120,0,0)` — **tick-for-tick with the tree's offset-pass asserts**;
    `pan 0→4` per segment, `pos_y=0`/`follow=0` on all **31 091** body lines;
    the only other changed-place frame is the half-time roll **f=4141** (a
    period reset — `ph=12` at f=4139, `ph=1` at f=4140, the place reset at
    f=4141 — included with the post-reset sentinels f=20000/f=31089; no later
    band entry panned). The AE: **pan 52 466 (seg[0]) / 69 828 (seg[1]) >
    still 29 120** (Gate 1, relative, same session/build; the HUD clock crops
    AE 0 on all three pairs); **Gate 2 (whole-scene bbox) fails as
    documented** — every diff is sprite-limited (`272×391+329+0` seg[0],
    `592×406+0+0` seg[1], `200×200+416+347` still — the renderer draws
    sprites on black), the FU313-L4/FU305-L6 caveat, never an attribution;
    **the trace attributes, the AE corroborates; the offsets are Layer-1
    citations from the tree's re-pinned asserts** (the trace carries camera
    cells only, FU321-L1). **The natural (undriven) clause is recorded
    NOT-GO-as-measured** in the note's own first section (the silent code-5
    bind, `camera_zero_frames` 1517 / `camera_left_zero` 2081,
    `segments 0 / changed 0 / off-zero 0`) — the natural route stays the open
    leg with its native owner (the seed-switch census, `outfield.c:685-701`),
    never silently upgraded. **Timing limit found (the FU321-L4/FU325-L2
    class): the `--window` (XSendEvent) keyups were not seen by the engine's
    key model** — the act-1 KICK's release arm (`match_handlers.c:263`) never
    re-arms without them and three attempts stalled before phase 2; **global
    XTEST is the required arm for any future live replay** (the wall-clock
    scheduling and the phase-2 landing drift stand; the DOWN release moved to
    the FINAL stop for the two-segment shape).

- **The wave's verdicts (FU322…FU325, promoted one-per-commit).**
  - **FU322 (the inversion design) — GO as re-shaped by its own errata
    1–5.** The draft's premise "the undriven natural tape pans" is **falsified
    by the wave's own measurement** (FU323-L1): (A1) survives with its comment
    re-worded to the bind, (A2) stands as written, (A3) re-anchors to the
    measured bind with the pacing slack re-based (no metric transient exists —
    FU324's falsification), **(A4/A5 are void undriven and relocate to the
    driven claim/offset passes)** as the per-segment `T(s)` window + the
    lockout identities, the golden-governance anchor is the measured 9/52/852
    chain (not the m-42 staged dispatch), and the additivity disposition is
    decided by measurement (**Outcome B** — the stratum-set gate). **New leg
    FU322-L12 — the re-shape dependency** (the inverted baseline binds only
    numbers FU323's tables measure). The mode gating stands (the no-ISO
    window recorded-only, FU322-L10).
  - **FU323 (the measured M2 diff) — GO with two wording conditions;
    FU319-L7 closed.** The blast radius is large but fully attributed: the
    faces settle live (hash-invisible — the drawn facing is the slot heading,
    not `+0x7D`; the render stage still does not copy the face, that open leg
    stays), the walk-in positions move (drawn from 852), the AI's code-4
    population collapses (6 120 → 1 959 record-frames — the face-reading
    defender/chase chain, the FU321-L10 multi-consumer collateral now
    measured), the undriven window binds with a zero seed (no motion), and
    the keeper ladder seam is untouched (6118/`ran=0`). Condition 1: the
    Stage-2 acceptance must NOT assert "the camera leaves (0,0,0)" undriven
    (the FU319 §6 pan wording corrected by erratum — the spec's Plan erratum
    carries it). Condition 2: the M2 accounting must classify the 852..6522
    window as the same producer's intended consequence (the provenance note's
    collateral line).
  - **FU324 (the metric census) — GO as a measured no-op with a contract
    pin; NO-GO on any behavioral claim.** The two metric cores are
    instruction-level the same computation (different plumbing only); the
    consumer census exhausts `+0x63`: `>>16` the two turn twins, `>>19` the
    two stride-attr twins, `>>22` the row-03 counter (dormant, FU177-L1); the
    low word `+0x63..+0x64` is dead space (no writer, no word-width reader);
    `+0x65` is written per-frame by block A (`0x8E278`) in both mover twins.
    The sub-change moves **0 frames** alone and **0 frames** composed
    (latch+metric == latch-only, `cmp`-clean); **FU319-L3's magnitude is
    falsified** (erratum FU324-L1). The wave-review erratum corrects the
    metcmp split (800 boundary + 4 000 000 random = 4 000 800; the total
    stands) and re-verifies the census + the no-op artifacts fresh.
  - **FU325 (the acceptance design) — GO with the Layer-2 vehicle decided by
    measurement; the fallback TRIGGERED.** The draft's "no driven variant
    required" decision is **void** — measured, the undriven pan does not
    exist (the zero-seed silent event is §6.1's own first fail shape), so
    §6.1's fallback governs: (a) the natural clause recorded
    NOT-GO-as-measured as a leg (no fixture tuning), (b) the driven variant
    is the Layer-2 witness replayed per the phase-32 procedure and re-pinned
    on the segment-aware tables, (c) the register wording names the pan's
    vehicle "the drive" and keeps the natural route open. The erratum also
    re-words TW1 (the tripwire anchors to the measured 852 chain, not a
    pre-pan camera frame).

- **Quoted errata (append-only; the wave drafts, the spec sections above the
  Plan erratum, and tasks 1–3's committed texts stand as written).**
  1. **The p33 note's re-pin-block commit cite (Minor).** Quote
     (`docs/screens/p33-l2-notes.md:69-71`): "The re-pinned cells are the
     tree's offset-pass asserts (`port/tests/test_engine_m2.c`, the segment-aware
     re-pin block — task 2's commit `7b3cc10`)". **Corrected:** the
     segment-aware re-pin blocks (both driven passes) landed in **task 1's
     atomic producer commit `302b746`** (`port/tests/test_engine_m2.c`, +916
     changed lines); `7b3cc10` touched only
     `port/src/fifa96_engine/fifa96_match_run.c` + `port/tests/test_action_locomotion.c`
     (the metric contract pin). The cells themselves are correct — they match
     the offset pass's landed asserts and the live run tick-for-tick; the
     commit attribution is the defect.
  2. **The unit pins' RED-phase capture (record-accuracy).** The first
     implementer session died mid-task and its RED-phase assert outputs are
     not preserved; the landed record is the GREEN state (controller-verified
     in task 1's report) plus the wave's recorded RED evidence (FU323 §2's
     probe: the patched build fires the standing `:3722` baseline assert —
     the measured RED of the staged-zero world). The two-step RED discipline
     remains FU325 §4's; no pin's RED story is invented (T2's pin is
     GREEN-at-commit by the discipline ruling, its guard the recorded mutant
     run).

- **The legs — met, open and out of phase.**
  - **FU322-L1..L12 —** L1 the window constants (landed from FU323's bind:
    4930/5200 around step 5065; the exact step stays recorded); L2 the
    natural pan's seed/axis (measured: the no-slot zero seed — void undriven;
    FU323-L1); L3 the claim pass's zero-delta witness (measured: the timeline
    moved wholesale and the zero-delta degeneracy is gone — the measured gate
    is a real offset entry with Δ=0; re-pinned with the reason, never
    silently deleted); L4 the offset pass's plateau tuple (measured: moved to
    lane 129 = bound 129 — the same integer-plateau class off the pinned
    numbers; the criterion survives); L5 the scripted witness's pre-emption
    (measured: survives — the undriven pan does not exist to pre-empt;
    `first_tracked == 9` and the stop/decay pins held); L6 the golden's first
    differing frame (measured: 852 — the 9/52/852 chain; FU325's erratum 1);
    L7 the additivity disposition (measured: Outcome B — the stratum-set gate
    landed in T1's commit); L8 the seam counts (measured: 6118/`select_ran 0`
    unchanged — no re-value needed); L9 the match_frame fixtures' survival
    (measured: the suite green — the goal-chain one-frame links held); L10
    the no-ISO natural window (open — recorded-only, unmeasured); L11 the
    follow-waiver wording (landed: the relocated pan window/lockout pair; the
    FU313 follow form stays waived and is now asserted per trace line); L12
    the re-shape dependency (landed — the atomic commit and this register are
    its implementation).
  - **FU323-L1..L7 —** L1 the undriven silent bind (the seed-switch census
    `outfield.c:685-701` is the native owner — **promoted: this is the
    natural-pan owner leg; see FU316-L1 below**); L2 the 852 attribution
    chain is measured, not proven frame-by-frame (the pixel-bbox ↔
    sprite-slot binding is inferred from the clean-position dump; owner: the
    M2 analysis if it needs the per-sprite proof); L3 the settled-face census
    (10 distinct of 22, none `0x100`; the open-loop "20 distinct" was an
    over-count; re-open only on a wrap-edge claim); L4 the AI code-4 collapse
    (6 120 → 1 959 record-frames; the anim ids stayed `{0, 0x26}` — the codes
    moved to the chase class); L5 the row-0x22 clear staged but unexercised
    on the probe tape (row43 is 2 on all 67 320 phase-2 record-frames, zero
    2→0 transitions; the P-CLR unit pin landed GREEN in T1); L6 the
    install-frame staging lag (carried from FU319-L4 — the wrap-successor's
    per-frame placement is the FU-84 driver's); L7 the mover's stride-error
    face term is the movement channel (`fifa96_action_handlers.c:257-258`;
    pinned).
  - **FU324-L1..L7 —** L1 the FU319-L3 erratum (the measured numbers do not
    reproduce; the direction stands; owner: the wave review — landed
    `8603dcf`); L2 the `+0x63` low half is dead space (checked, closed —
    re-open only if a future row port writes the low word); L3 the second
    snap body's reach-in (`0x8E510..0x8E54A`, zero direct xrefs; dead-or-
    reached unresolved; owner: the record-machine aux tables); L4 the
    0x8DCD4 out-pointer subset (7 confirmed of 78 callers; owner: the
    FU-142b caller-table extension); **L5 the attr sensitivity returns with
    the `+0x7B` nibble staging — the re-measure clause: when a nibble staging
    goes live (row-26's `0x10F394` table writes, the row-03 counter), re-run
    the mutant protocol of FU324 §5** (owner: the mover staging + the row-26
    timer staging; carried into T2's ledger note); L6 the row ids of the
    `0x87xxx-0x89xxx` readers (owner: the row-body census family
    FU-137/FU-185); L7 the composition re-run clause (if the latch build
    changes before a future stage binds, re-run `cmp` latch+metric vs
    latch-only).
  - **FU325-L1..L7 —** L1 the natural Layer-2 pan is a prediction until
    measured (measured: NOT-GO — the pan does not exist undriven); L2 the
    driven fallback's live timing (exercised and recorded by T3: the
    global-XTEST requirement, the wall-clock scheduling, the phase-2 drift;
    the general rule stays open); L3 the kickoff narrow write's pool-row44
    question (the narrow write writes `row43` only; the pool `row44` consumer
    census between the placement and the first dispatch is not taken — any
    pre-face-event diff that a `row44`-at-placement explains is a STOP); L4
    the successor placement (carried, FU319-L4); L5 the natural run's later
    windows (the extract covers the whole run wholesale — the half-time reset
    and the post-reset sentinels are in it; no later band entry panned); L6
    the trace's run-relative `f` and the pair window (the tick agreement is
    the strong claim; the numbers per-run); L7 the metric sub-change has no
    acceptance here (the separation rule — P3 its own commit; the P-MOV
    fixtures knife-edge-free; TW7).
  - **FU325-L8 (new; the still-pair AE honesty).** The p33 still pair is
    **NOT pixel-identical** — AE **29 120**, the ball sprite animates
    post-pan (the diff bbox confined to the ball, `200×200+416+347`; the
    outfield sprite is pixel-identical between the grabs), unlike p32's
    byte-identical settled still; the sibling controls (s1 AE 61 195, the
    player still settling / s3 AE 29 591) show the ball never settles and the
    player does. **Gate 1 is relative and holds** (AE_pan 52 466 / 69 828 >
    29 120, same session); the still control stays a same-second, no-key,
    camera-constant (trace) pair recorded as measured — never tuned away.
    Owner: the Layer-2 note (carried); re-open only if a still pair's trace
    cells stop being constant.
  - **FU324-L8 (new; the `r->distance` (+0x65) persistence gap).** The
    port's mover computes the block-A distance and stages it into
    `s.move_attr` but does **not** persist it to the run record's
    `r->distance` cell: the mover's write-back writes pos/face/speed/vel/
    body_timer/type only (`port/src/fifa96_engine/fifa96_match_run.c:549-556`),
    while the native mover's block A writes the cell per frame
    (`0x8E278 MOV word[EBX+0x65],AX`); the run-record field's comment
    describes the gap (`fifa96_match_run.h:466` — "native +0x65 0x8DCD4
    out[0] word (FU-142e row 2A)", the row-body `out[0]` cell). The row-body
    readers (`0x864E8`, `0x87B92`, `0x866B4`, `0x884E5`, `0x885AD` — the
    port's `fifa96_match_handlers.c:1963`/`:5145`/`:5416`/`:6116`/`:6148`)
    read a cell the mover never refreshes: between mover runs it holds
    whatever the last row-body `0x8DCD4` call left. **Unmeasured** (no M2
    diff is attributed to it; the golden moved exactly on the FU323 chain
    with this gap in place). Owner: the mover staging / FU-77's block A
    (the persistence write-back is a separate, unmeasured change — never
    folded silently into a producer commit).
  - **FU324-L9 (new; the negative-metric-word stride index).** A negative
    metric word (the metric overflows the AX store at the `−0x8000` edges —
    e.g. `(-0x8000,-0x8000)` → word `+0x5000`, the pin's shape assertion
    covers the truncation) drives `attr = move_attr >> 19` negative and the
    stride `index = (desired & 0x3F0) | attr` below the row's table base —
    a wild table read; the native reads its table at the same wild index
    (the port is faithful so far), but the behavior is **unclassified**
    (which rows/values the table exposes below the base is unread). The
    contract pin deliberately keeps its stride tuples non-negative —
    commented at the assert site (`port/tests/test_action_locomotion.c:286-288`,
    FU324 §6's clamp scope). Owner: FU324 §6's attribution + the row-body
    census (the stride table's negative-index class).
  - **FU316-L1 — PROMOTED: the natural-pan owner leg.** The undriven tape's
    silent bind makes the no-slot arm's seed distribution and per-pan travel
    the natural pan's open gate: the band record holds no control slot, the
    seed switch takes `0x7F056` (`word[+0x73] << 2`), and
    `v0 = trunc(±4/0xC) = 0` binds without motion; a pan needs `|v0| ≥ 1`
    from that arm's real seed (larger seeds: `s = 0x80 → T = 0x174`; the
    armer count `ceil(0x730 / T(s))` pans). Owner: the seed-switch census
    `fifa96_outfield.c:685-701` + `fifa96_outfield_row04_step` (the phase-31
    leg's port cite `:692-701` stands).
  - **FU314-L1a — the live-face half is MET** (the face is live, never
    staged; the driven witnesses pass with Δ=0 and `lane_at_pass 129`); the
    robust **natural** route stays open on FU316-L1, which now owns it.
    **FU314-L5 — CLOSED** (the mover staging is live; the atomic commit's
    subject).
  - **Carried unchanged:** FU307-L8; the phase-30 remainder as registered;
    the phase-31 fidelity legs FU315-L1/L2 (the claim tail), FU316-L2..L7,
    FU317-L1..L7/L9/L10 (the follow/Y waiver stands — asserted per trace line
    now; no `>0xF0` event is produced); **FU316-L8** (the row-05 carrier
    camera event `0x7F4E7` unported — needed only for a claim-frame bind) and
    **FU316-L9** (the unarmed mirror — no armer work); the phase-32 legs
    FU318-L1..L8, FU319-L1..L7 (L3's magnitude falsified by FU324-L1 — the
    direction stands), FU320-L1..L8 (L1's caveat is retired as *pinned* — the
    integer-plateau class note stays with the new numbers), FU321-L1..L11.

- **Deferred minors (T1–T3 reviews; none a fidelity defect).** The
  near-verbatim claim/offset sampler duplication is now three passes deep
  (the natural/claim/offset delta-run machines; declared at the sites — the
  extraction is a follow-up); the ring identity `first_bind_id ==
  M2_NAT_BAND_ENTITY` (FU322 §5 item 5) is **recorded, not asserted** in the
  landed ISO branch (the re-shape routed the identity to the stderr records;
  the driven passes pin the tracked cells directly); the p33 note's
  `FU325_r2l7_acceptance.md:253` cite for the whole erratum section is
  imprecise (the section is `:249-253`); the first session's RED-phase
  capture loss (erratum 2 above) is a process, not a code, debt.

- **Tape and gate.** M1 `09b726b7…` (22 004 B, 691 lines) byte-identical
  through T1–T3 (`cmp`-verified at every task); M2 re-pinned in T1
  `8434cfab…` → `d5446144…` (295 087 B, 6 522 lines; the 852 chain, hash-only
  — the reason + the frame diff + the register pin in the same commit) and
  byte-identical through T2–T3 (`cmp`-verified); `make check` **111/111**
  (strict, ASan/UBSan) at every task; census **`60/80 = 75%`** (`37 action +
  23 phase`; fresh re-count); the README title stays `75%`; AGENTS.md
  untouched. The phase's production changes are T1's live-face staging
  (`302b746`) and T2's value-identical metric-source alignment (`7b3cc10`); T3
  and this close-out are docs-only. The trace sink stayed default-off outside
  T3's acceptance run; the capture/trace artefacts are `docs/screens/p33-l2-*`.

## Final-review erratum — the phase-33 register (2026-10-23)

Three corrections from the final whole-branch review. One is an in-place edit
to the Layer-2 artifact sentence (its original wording is quoted in the
artifact item below); every other prior section stands as written.

- **The artifact set (Layer-2 sentence).** Quote: "the eight PNGs + the two
  diffs + the 120-line trace extract". **Corrected:** the two diff PNGs are
  two of the eight committed PNGs, not additional files; the artefact set is
  **the eight PNGs (incl. the two diffs) + the 120-line trace extract** (the
  Layer-2 sentence above was corrected in place; its original wording is the
  quote in this item). Nine files beyond
  the note: `p33-l2-pan-a..d.png`, `p33-l2-pan-diff.png`,
  `p33-l2-seg1-diff.png`, `p33-l2-still-a..b.png`, plus
  `p33-l2-trace-pan.txt`.
- **FU319-L8 (new; the raw-point focus guard vs the native delta test).** The
  port's turn-close branch guards the focus point with the raw staged-point
  test `face_x != 0 || face_z != 0`
  (`port/src/fifa96_loader/fifa96_action_handlers.c:223`), while the native
  `0x8E2B7..0x8E2D0` tests the **deltas** (the focus point minus the
  record's position). The two forms diverge at the **undriven camera origin**
  `(0,0)`: the raw test disables the focus branch (the face keeps its
  current value), while the native delta test would steer toward the staged
  point even when that point is `(0,0,0)`. The divergence is why the P-MOV
  fixture stages an off-origin camera
  (`test_mover_row43_live_turns_toward_focus`; the plan erratum records the
  sketch correction). **Blast radius unmeasured** — no moved M2 frame is
  attributed to it (the golden is byte-identical under the phase-33 dispatch
  fix); re-open with a camera-origin witness or a direct unit pin of the
  native delta form. Owner: the action-handlers seam
  (`fifa96_action_locomotion_step`).
- **The decay-sequence cell (claim-pass sub-bullet) and the pass-source line
  refs.** Quote: "the decay seq truncated at the site (5, 0)". **Corrected:**
  the `(5, 0)` pair was a sampler artifact, not a measured truncation — the
  pass-level `speed_seq` restarted at each segment start while its `stop_f`
  gate still held the previous segment's stop, so only the first-motion and
  stop speeds were recorded. The per-segment recording now pins the real
  signature **5, 3, 2, 1, 0 on both segments** (claim asserts
  `port/tests/test_engine_m2.c:4587-4590` and `:4644-4647`; offset asserts
  `:4970-4973`, `:4994-4997`, `:5027-5030`) and the stderr line prints it;
  the round-1 fix report (`final-fix-report.md`, the `speed_seq` item)
  records the sampler defect and the choice of per-segment recording. The
  frozen tables' other cells stand unchanged. **Fresh line refs for the
  pass citations in the register's driven-witness bullets above** (the
  functions moved in round 1):
  `run_natural_claim_pass` is now `port/tests/test_engine_m2.c:4381` (was
  `:4352`), its re-pin assert set `:4473-4659` (was `:4444-4619`; the
  segment block is `:4557-4659`); `run_natural_offset_pass` is now `:4758`
  (was `:4720`), its re-pin assert set `:4861-5041` (was `:4884-4992`; the
  segment block is `:4946-5041`).

## M2 phase-34 wave erratum (the phase-33 register's seg[1] cells)

Quote (this register's driven-witness bullets, the phase-33 tail): "**the new
code-3 walk camera event** the frozen world did not have — gate **f=1552**
(code 3, lane 128/−128, bound 128, `face7d −0x100`, `has_slot 0`, pos
(−16,0,0), trk 2), event **f=1553** (code 3, trk 1)"; and the second segment's
"(code 3, …)". **Corrected (FU327, measured on the phase-34 wave):** the driven
**seg[1] is a row-04 event of entity 1** — the firing record's action code is
**4**, `has_slot 1`, seed `(−0x40,0)`, the same `−0x90` pan class. The `code 3`
is **entity 2's pool cell** read through the harness's pool snapshot
(`event.code`), not the firing record's action code; the snapshot attribution
is the FU327 §4.3 caveat (the pool's `event.code` belongs to the record the
pool last wrote, not to the event's record). The `active 2` / `dir (−1,0)`
cells are likewise `active 1` / `dir (0,0)` (the `2` is `row43`, the dir is
entity 2's). **What stands:** the phase-33 Layer-2 verdict (the driven
two-segment pan reproduced live), the two-segment shape, the per-segment
`−0x90` pans, the `(−0x120,0,0)` total, the 54 changed frames, and the
`5,3,2,1,0` per-segment decays. The cell corrections propagate from FU323's
phase-34 erratum (`port/spec/FU323_r2l7_m2_probe.md`); the phase-34 wave
measured 5 row-04 gate passes total (1 no-slot, 4 slot controls — FU327 §3).
The phase-33 register's verdict and golden accounting are unaffected.

## M2 phase-34 register (FU326…FU329; 2026-10-23) — the no-slot pan travel: the measured NO-GO (the arm is native-faithful at parity; the `+0x79` placement-timer producer is the root gap)

**Scope/provenance.** Phase-34 spec
`docs/superpowers/specs/2026-10-23-fifa96-m2-phase34-noslot-pan-design.md`
(§5/§6/§7/§8 + the **Plan erratum (wave close)** — which corrects §2/§4's
larger-seed premise and records the NO-GO), producer plan
`docs/superpowers/plans/2026-10-23-fifa96-m2-phase34-producers.md` (T1, this
close-out) and the recon-ahead plan
(`2026-10-23-fifa96-m2-phase34-recon-ahead.md`); the reviewed four-slice wave
`port/spec/FU326_r2l8_noslot_seed.md` … `FU329_r2l8_acceptance.md`,
promoted one-per-commit (`55357f8` FU326, `4945e4b` FU327, `3ce3dbb` FU328,
`04a78f2` FU329), with the wave-review errata appended to each — FU326's five
items (the zero-seed speed mechanism, the bound's scope, the FU321 §3.5 prior
authority, the `0x19` clamp, the `T = 12` wording), FU327's one (the walk rate
is the velocity word; the place steps 2/frame), FU328's four (the band
`active`, the `+0x79` range, the `ramp` cell, the direction family), FU329's
five (the `0x174` row, H5's inverted premise, H3's example, the M2
exposure/leg hand-off, the m-42 cite); and the phase-34 wave erratum
`996ddda`, which appended the FU323 cell erratum, the FU316 seed erratum, the
spec's Plan erratum and the phase-33 register's seg[1] erratum. This close-out
is **docs-only**: no production, test or golden change (the spec §5 NO-GO
branch — P1 is the measured-distribution register, not a producer).
Append-only: the phase-33 text and every prior section stand as written; the
corrections below are quoted errata. Census **`60/80 = 75%`** (`37 action + 23
phase`; fresh re-count this close-out over the 45+35 dispatch tables — no row
joined); M1 `09b726b7…` (22 004 B, 691 lines) and M2 `d5446144…` (295 087 B,
6 522 lines) **byte-identical** (`cmp`-verified); `make check` **111/111**
(strict `-Wall -Wextra -Werror`, ASan/UBSan). The README title stays `75%` —
the census did not move, so no recompute (AGENTS.md's derived rule).

- **The verdict — no natural pan was achieved, and the reason is measured: the
  no-slot arm is native-faithful and the port is at parity, but the band
  record never leaves its parked stride because the native placement timer
  `+0x79` has no port producer.** The row-04 seed switch's **no-slot arm**
  (`0x7F056`: `camY == 0 && !has_slot`, `|cam_x| ≤ 0x630`, `|cam_z| ≤ 0xA20` →
  `word[+0x73] << 2` / `word[+0x75] << 2`; the `(0,0)` bail still fires the
  event) is exact against the fresh bytes, the `v0 = trunc(seed/0xC)` fast path
  is the native `IDIV` (`0xA2AEE`), the decay is `trunc(0x18·v/0x20)` and the
  travel `T(v0) = 2·(6·v0 + 7·v1 + …)` reproduces the measured
  `5→3→2→1→0` / `T(0x40) = 0x90` instance (FU326 §3–§4). The port's **22
  seed-staging/arithmetic audit cells** match the fresh bytes — **no staging
  fix is warranted** (FU326 §6) — so the spec §5/§8.5 conditional Case B is
  **not triggered** and the phase carries **no production change**. The natural
  witness is **NO-GO** (FU328 §6/§9): every no-slot gate pass on every measured
  shape is the zero-seed **silent bind**, and **the silent bind is the
  consequence of the missing `+0x79` producer, not of the no-slot arm alone** —
  supply the limit (the §5.6 diagnostic) and the same natural record, with no
  drive, pans with `T = −0xC / −0x26 / −0x84`.

- **The algebra (FU326) — the stride bound and the inversion of the carried
  larger-seed premise.**

  | quantity | value | source |
  |---|---|---|
  | the seed words | `word[+0x73]` / `word[+0x75]` (the record's stride velocity) | `0x7F08F` / `0x7F09B` |
  | the stride flat | `0x10F680`, 1 280 words, fresh **min −15 / max +15** (445 negative / 660 positive / 175 zero) | FU326 §4.2 |
  | the seed | `s = 4·w`, so **`\|s\| ≤ 0x3C`** | FU326 §4.2 |
  | `v0` | `trunc(s/0xC) = trunc(w/3)` → **`[−5, +5]`** | FU326 §4.2 (`0x70646`/`0xA2AEE`) |
  | the travel | `T ≤ 0x90` per axis; the delta is the **vector** `Δ = sign(w)·T` | FU326 §4.3 |
  | the silent band | `\|w\| ≤ 2` → `v0 = 0`, `T = 0` (three consecutive words share one step) | FU326 §4.4 |
  | the armer reach | `floor(0x730/T)+1` x / `floor(0xB20/T)+1` z = **13 x / 20 z** at `T = 0x90` — a **floor** | FU326 §4.5 |

  **Quoted correction (the spec §2/§4 and FU316 §4.3/§7.1).** "a larger no-slot
  seed `s = 0x80` gives `T = 0x174`" / "the armer reach … (fewer for larger
  no-slot seeds)". **Corrected (FU326, fresh `read_memory 0x10F680`):** the
  `0x174` row needs `v0 = 10`, i.e. `w = 0x20`, outside the table, so it is
  **arithmetic-only**; the live no-slot maximum is `T = 0x90`, hence 13 x / 20
  z pans is the **floor**, not a best case. This is a **reconciliation, not a
  discovery**: FU321 §3.5 (frozen, phase 32) already carried the `|w| ≤ 15`
  bound and concluded "the no-slot arm's maximum is the same `0x90`". The
  bound's **scope** is the locomotion writer's domain and the measured tape
  (`|w| ≤ 1` on every record-frame); a static impossibility is **not**
  established — the 87/51 non-locomotion `+0x73/+0x75` writers stay open as
  FU326-L1 (FU326's erratum 2).

- **The measured distribution (FU327) — five row-04 gate passes, one no-slot.**
  The three measured passes sample the whole window (`cone_passes` =
  `events_1d` = the fired events: natural 1, claim 2, offset 2), twice per
  process byte-identically, on a probe whose unpatched build reproduces M2
  `d5446144…` **before** any instrumentation (and whose instrumented build
  reproduces the same sha — the hook is observation-only).

  | # | pass | event f (gate = f − 1) | id | `has_slot` | arm | seed `(x,z)` | `v0` | `T` | changed frames |
  |---|---|---|---|---|---|---|---|---|---|
  | 1 | natural | 1517 | 12 | **0** | **`0x7F056`** | **(−4,0)** | **(0,0)** | **0** | **0** |
  | 2 | claim | 1481 | 2 | 1 | `0x7F035` | (−0x40,0) | (−5,0) | −0x90 | 27 |
  | 3 | claim | 1553 | 1 | 1 | `0x7F035` | (−0x40,0) | (−5,0) | −0x90 | 27 |
  | 4 | offset | 1483 | 2 | 1 | `0x7F035` | (−0x40,0) | (−5,0) | −0x90 | 27 |
  | 5 | offset | 1553 | 1 | 1 | `0x7F035` | (−0x40,0) | (−5,0) | −0x90 | 27 |

  The arm census is `0x7F056` ×1, `0x7F035` ×4, `0x7F003` ×0, the `(0,0)` bail
  ×0. The no-slot instance reproduces the phase-33 silent bind **cell for
  cell**: `trk=12`, `tim 0/12`, `ramp 12`, `vel (0,0)`, `speed 0`,
  `pos (0,0,0)` on all 3 598 granted frames, `camera_zero_frames` 1517 /
  `camera_left_zero` 2081, `segments 0 / changed 0 / off-zero 0`. The four
  slot controls are the carried model end to end: seed `−0x40` → `v0 = −5` →
  the `5,3,2,1,0` decay over 6/7/7/7 frame groups → 27 changed frames →
  `T = −0x90` → pan `0→4`, **two per driven pass** accumulating to
  `(−0x120,0,0)` (the second event's clear resets `pan 4→0` before rebinding).
  The measured population is the condition on which a witness fails: the band
  record's `+0x73 ∈ {0,−1}` on all 3 598 frames, the formation-wide
  `max |w| = 1` (0 on 1 074 frames, 1 on 2 524), **zero records at
  `|w| ≥ 3`** on any row — the smallest moving no-slot seed
  (`|w| = 3` → `T = ±0xC`) is two whole velocity units away (FU327 §4/§6).

  **Quoted cell corrections (FU323 §4/§5.1; FU327 §5.1/L2/L3).** Quote: "the
  gate tuple at the band frame … `active 2` … `dir (−1,0)`" and "its passing
  dispatch is no longer the row-04 code-4 gate but the **code-3 chase walk's
  own camera event**". **Corrected:** the band frame's cells are **`active 1`**
  / **`dir (0,0)`** (the `2` is the `row43`/`+0x43` cell; the dir is entity
  2's; FU323's own `anim_patched.txt` step-5062 line carries `active 1`), and
  the driven **seg[1] is a row-04 event of entity 1** (`code 4`, `has_slot 1`,
  seed `(−0x40,0)`) — the harness's `event.code 3` is **entity 2's pool cell**
  (`m2_claim_snapshot` reads `M2_CLAIM_ENTITY = 2`). The frozen pins stand as
  pool-cell pins; only the attribution changes. The phase-33 register's own
  seg[1] erratum (the section above, `996ddda`) carries the same correction.

- **The reachability (FU328) — the 32-run sweep: NO-GO; the labelled
  diagnostic proves the chain.** Thirty-two measured runs — the verbatim
  `M2_KEYS` natural tape at 12 000 / 24 000 / 48 000 / 96 000 / 160 000 steps
  (3 598 … 47 998 granted frames), the direction families
  (DOWN/UP/LEFT/RIGHT), the offset nudge family, the kick/pass/long-ball
  claim/carrier contexts, seven half-time-restart KICK offsets and a DOWN
  tail — every granted frame sampled, every `fifa96_camera_event_set` captured
  with the arriving record's cells; **no shape staged a slot, seed or face
  cell**. **No nominee:** the two no-slot gate passes that exist (natural
  f=1517; f=1590 at the moved camera under the kick/pass family) both carry
  `seed −4 → v0 = 0`, and every real pan in the sweep is the slot arm. The
  port-side cap is one chain (§3.4): `+0x79` unwritten → `+0x7B & 0xF = 0` on
  every record-frame → `stride_len = 1` → `attr ≤ 1` → the stride word `≤ 1` →
  `|seed| ≤ 4` → `v0 = 0`. The **labelled diagnostic** — the placement limit
  staged on the record-locomotion cell only, never a witness, excluded from the
  sweep table, the spec §7 staging ban respected for every sweep shape — pins
  the chain from the other side: **`+0x79` = 3/6/15 → seed `−0xC/−0x18/−0x34` →
  `v0` −1/−2/−4 → `T` −0xC/−0x26/−0x84** at gate f 867/704/607, with the exact
  6/13/27-frame decays. **Frequency (FU316-L7): 1 no-slot band entry / 3 598
  granted frames (0.028 %), 0 pans**; 1/4 138 first-half granted frames
  (0.024 %) over the longer windows; driven **2 slot pans / 3 598 (0.056 %)**,
  still 0 no-slot pans. The 96 000-step window's extra 24 660 granted frames
  (85.6 %) are the **stalled half-time kickoff**, not play (FU328-L4).

- **The acceptance and the golden governance (FU329) — Branch B: recorded, not
  pinned.** **No test change lands.** A *new* assert of today's zero seed would
  pre-commit the acceptance to the no-witness world and invert the day a
  witness appears (the phase-33 inversion lesson), so it is deliberately not
  taken; **A1–A3 stay** as landed (`port/tests/test_engine_m2.c:3962-3979`) and
  nothing is re-pinned. The witness tuple — `code 4`, `anim_id != 0x13`,
  `cam_pos_y == 0`, `has_slot 0`, the arm's camera sub-gate, `lane_x ≥ 1`,
  `|v0| ≥ 1`, the per-axis `T(s)` from the measured `cam_vel` pair, the
  segment cells — is the design the **`+0x79` producer will be measured
  against**; the **future A3 re-pin rides that producer**, not this phase. The
  golden is **Case A, byte-identical**: the natural pass writes no transcript,
  so a test-side acceptance could not move M2 by construction — and this phase
  writes no test at all. Branch A (a measured witness) did not arise, and no
  fixture was staged to manufacture one (spec §6/§8.6). The future producer is
  a **separately pinned, golden-moving** fidelity change (FU328-L8) under the
  faithful-move recipe (the first-diff rule, the `state=` check, the
  stratum-set gate, the written reason), never folded into an acceptance
  (FU329's erratum 4).

- **The wave's verdicts and the errata chain.** **FU326** GO on the algebra and
  the port parity / NO-GO on the carried larger-seed premise — its most
  consequential erratum is item 2 (the bound is the locomotion/measured
  domain, not an impossibility). **FU327** GO on the measured-distribution
  register / NO-GO on a natural witness in the measured passes. **FU328**
  NO-GO on a natural witness (no nominee) + GO on the diagnosis and the fix
  target — its erratum 2 corrects "6..15" to the `0x10F394` **table-entry**
  range and records that the native store also ORs `[rec+0x9D]` (unported,
  OL-72), so entity 12's native value is **unmeasured** (FU328-L3). **FU329**
  GO on the design, closing in Branch B. Errata pointers: the spec's Plan
  erratum; the FU326/FU327/FU328/FU329 review errata; FU323's cell erratum and
  FU316's seed erratum (`996ddda`); the phase-33 register's seg[1] erratum (the
  section above). Every number in this register is a quote of those sections —
  no native fact is re-derived here.

- **Quoted errata (this close-out's own record).**
  1. **The A1–A3 line cite.** Quote (the Branch B bullet as first committed in
     `75dcccd`):
     "`port/tests/test_engine_m2.c:3935-3966`". **Corrected (fresh read,
     fix round 1):** the landed A1–A3 block is **`:3962-3979`** — the
     `if (with_iso)` at 3962, the A1 comment 3963-3966 with its assert at
     3967, A2 at 3968-3971, A3 at 3972-3979 (the authoritative range is
     `port/spec/FU329_r2l8_acceptance.md:373`). `:3935-3966` ends on the A1
     comment and contains **none of the A1–A3 asserts** (its natural-baseline
     asserts live at 3936-3947). The stale range was
     **inherited**, not measured here: its sources are
     `port/spec/FU328_r2l8_reachability.md:597` and the phase-33 register
     (`port/ENGINE.md:6809`), and both still carry it — append-only, they are
     corrected **here** and not rewritten; FU328's draft receives the same
     post-review note.
  2. **The degraded no-ISO window's line cite.** Quote (the phase-33 register,
     `port/ENGINE.md:6816`): "The degraded no-ISO window stays recorded-only
     (`:3967-3989`; FU322-L10)". **Corrected (fresh read):** the recorded-only
     `else` branch (the no-ISO stderr line) is **`:3980-4002`**. Append-only:
     the phase-33 text stands as written; the correction lands here.

- **The legs — met, open and promoted.**
  - **FU326-L1..L4** — L1 the non-locomotion `+0x73/+0x75` writers (87/51
    operands; reachability at the row-04 gate unproven — the only formal escape
    from the bound; owner: `fifa96_outfield_row04_step`'s `vel_int` staging +
    a native writer census); L2 the `camY` int16 narrowing (latent, `Y ∈
    [0, 0x640]`; owner: the `fifa96_match_action_04` staging); L3 the slot-arm
    dir source (dead on the no-slot arm; owner: the `fifa96_match_run` slot
    model); L4 the hardcoded `0x1104AB` rate pair (owner:
    `fifa96_camera_event_set` + the `0x14C2FE`/`0x14C2FA` cells).
  - **FU327-L1..L7** — L1 the parked no-slot seed and the formation-wide
    `|w| ≤ 1` (owner: FU328 — now FU328-L1); L2 the band tuple's
    `active`/`dir` cells (owner: the Stage-2 baseline wording — this register's
    quoted correction lands it); L3 the harness's pass-level `gate`/`event`
    cells are entity-2 pool cells (owner: the acceptance wording); L4 the
    moving-travel ladder is arithmetic-only (owner: FU328's drive — not found;
    the `+0x79` producer's measurement); L5 the no-slot camera-bounds bail is
    unexercised (owner: FU326 §5 item 5 / FU328); L6 the height arm and the
    row-`0x13` code-`0x15` arm are unexercised (owner: none in phase 34);
    L7 the probe's cone predicate is a replica (owner: the next probe; guarded
    by the 5/5 agreement and the golden-equivalence build).
  - **FU328-L1..L8** — L1 **the `+0x79` producer gap — PROMOTED, the
    natural-pan owner leg** (below); L2 the row-0x03 placement counter is
    unwired and its staged `move_attr` is hard 0 (owner: FU177-L1 — the
    *maintenance* half; L1 alone lifts the install-reset cap); L3 the native
    `+0x79` for entity 12 is unmeasured (the 3/6/15 ladder is not the native
    value; owner: FU84 §6 + the roster descriptor, measured with L1 landed);
    L4 the undriven second half is a **stall** (phase 12 at step 13802, phase 1
    at 13805, clock 120 s, no transition through step 160 000; owner: the
    row-01 kickoff release / the FU-84 per-frame driver); L5 the drive-family
    coverage is finite (no live `make game` rate; owner: a future live
    measurement); L6 the cone filter is not the gate (one-frame operand
    offset; the wrap events are the authority); L7 the wrap's arm classifier is
    partial (arms 0/1 exercised; 4/5/6 never observed); **L8 the M2 exposure of
    the L1 fix** (every record's stride rises to the native value and the
    natural tape pans undriven — the golden moves wholesale under the
    faithful-move recipe; the census does not move).
  - **FU328-L1 — PROMOTED: the natural-pan owner leg** (from FU316-L1). The
    native placement limit `+0x79` (`FUN_0007997C`, `0x799F0..0x79A4D`, FU84
    §6) has **no port producer**: `timer79` is read-only in the engine
    (`port/src/fifa96_engine/fifa96_match_entities.h:129`; no writer in
    `src/`, the only assignments are in tests), the install reset
    copies `timer79` (currently 0) into `+0x7B`
    (`entity->timer7b = entity->timer79`,
    `port/src/fifa96_engine/fifa96_match_entities.c:94`, native `0x7DA9B`), so
    the placement nibble is 0 on every record-frame. Owner: the entity
    reset/placement entry + the `+0x9D` byte (OL-72). It is a **future,
    separately pinned, golden-moving fidelity change**: the native value for
    the natural band record measured first (FU328-L3), the first-diff rule and
    the stratum-set gate armed (FU329 §6.2), never folded into an
    acceptance-only commit. The predicted witness (entity 12, gate ≈ f
    607–704 for the native range, `|v0| = 2..5`, `T = 0x26..0x90`) is a
    **prediction**, never an assertion.
  - **FU329-L1..L11** — L1 the witness pin is unwritten until the tuple is
    measured (owner: FU328 → the `+0x79` producer's phase); L2 the `lane = 0`
    conflict is a spec decision (owner: the wave review/spec — unexercised);
    L3 the seed words are not yet a harness cell (owner: the future P1 +
    FU326); L4 the Layer-2 pair window for a small `v0` (owner: the Layer-2
    task — not attempted in Branch B); L5 the no-slot Layer-2 has no slot-arm
    fallback (owner: the Layer-2 task + the register wording); L6 the armer
    class of **chained** no-slot pans is unmeasured (owner: FU326 + FU328 —
    re-worded by the erratum); L7 the natural run's later windows (owner: the
    Layer-2 note); L8 the trace cannot show the arm or the seed (owner: the
    acceptance wording); L9 the m-42/link class is a slot-arm fixture (owner:
    the future P1); L10 the census/README recompute is out of scope (this
    close-out re-counted: `60/80` holds; owner: the close-out — met); L11 Case
    B is conditional on a fix this wave did not make (owner: the future fix +
    FU326).
  - **FU316-L1 — its disposition.** The travel model is **settled within the
    locomotion bound**: the no-slot seed is `4w` with `|w| ≤ 15`, so
    `|s| ≤ 0x3C`, `v0 ∈ [−5,5]` and `T ≤ 0x90` per axis, and the measured tape
    sits far inside it (`|w| ≤ 1`). Its larger-seed premise
    (`s = 0x80 → T = 0x174`) is corrected by FU326/FU329's errata; its
    **non-locomotion writer escape stays open** (FU326-L1). The natural-pan
    **ownership passes to FU328-L1**: the gap is upstream of the seed.
  - **Carried unchanged:** the phase-30/31/32/33 remainder as registered
    (FU315-L1/L2, FU316-L2..L9, FU317-L1..L7/L9/L10, FU318…FU325's legs,
    FU319-L8 included; FU316-L7's M2-window frequency met here, residual =
    FU328-L5); the FU324-L5 re-measure clause now has a named trigger
    — the `+0x7B` nibble staging arrives with FU328-L1.

- **Tape and gate.** Docs-only, proven: M1 `09b726b7…` and M2 `d5446144…`
  byte-identical (`cmp`-verified), no re-pin, no frame diff, no fixture
  re-staging and no `state=` change; `make check` **111/111**; census
  **`60/80 = 75%`** (`37 action + 23 phase`; fresh re-count); the README title
  stays `75%`; the ISO untouched; the wave's probes, builds and logs all
  out-of-tree (`/tmp/opencode/fu326…fu329`), the repo receiving only the four
  drafts, the spec, the plans and the errata. The phase's **next-phase
  hand-off** is FU328-L1 (the `+0x79` producer, the natural-pan owner).

## M2 phase-35 register (FU330…FU333; 2026-10-23) — the placement timer and the natural pan: the producer landed, the measured 852-chain re-pin, the first natural pan (producer-conditioned)

**Scope/provenance.** Phase-35 spec
`docs/superpowers/specs/2026-10-23-fifa96-m2-phase35-placement-timer-design.md`
(§1–§9 + the **Plan erratum (wave close)** — six corrections: the cap, the
port assumption, A3-in-P1, Branch A, the `+0x7B` counter cite, the witness's
producer-conditioned character), the producer plan
`docs/superpowers/plans/2026-10-23-fifa96-m2-phase35-producers.md` (T1–T4) and
the recon-ahead plan (`2026-10-23-fifa96-m2-phase35-recon-ahead.md`); the
reviewed four-slice wave `port/spec/FU330_r2l9_placement_timer.md` …
`FU333_r2l9_acceptance.md`, promoted one-per-commit (`3bf1f35` FU330,
`245172c` FU331, `53d1ffe` FU332, `8883995` FU333), with the wave-review errata
appended to each — FU330's five items (the `XOR AH,AH` store, the index-order
causal sentence, "row-12" → action code `0x0C`, the consumers claim, the
`+0x9D = 0` basis), FU331's six (the existing table, the `0x20` boundary, P1 is
one of three writers, the cadence, the index form/site count, the 15-bound
qualification), FU332's six (the value probe vs the expression, `+0x9D` not
measured, the P2/P3 residual, the 840-frame gap explained, the dump identity,
the x-only `Δ` column), FU333's eight (Branch A/B, the 852 anchor, the
producer-conditioned witness, "row-12", the phase-0C cite, the spec §5/§6, the
memcmp rationale, "probably wholesale" measured); the wave erratum + plan
`09b7352`; the spec `614dff7`. Task commits: T1 **`749cc2f`** (**the atomic
producer commit** — the two cells, the producer, the row-09 correction, the
P1/P2/P3 pins, the A3 re-pin, the survival/driven re-pins, the M2 re-pin and
the provenance, one commit, `fix(fu328)`), with fixes `8aaddb1` / `14ef311`
(the natural-pass narrative re-base, the offset discriminator, the fixture
re-derivations); T2 `673a10d` (`test(fu333)`, the natural no-slot pan witness,
test-side only); T3 `79819a4` + `21f5f44` (`docs(fu333)`, the Layer-2 live
acceptance and its round-1 evidence attributions). This close-out is
**docs-only**. Append-only: the phase-34 text and every prior section stand as
written; the corrections below are quoted errata. Census **`60/80 = 75%`**
(`37 action + 23 phase`; fresh re-count this close-out over the 45+35 dispatch
tables — no row joined); M1 `09b726b7…` (22 004 B, 691 lines) byte-identical
(`cmp`-verified at every task); M2 re-pinned once, in T1, `d5446144…` →
**`6aa2703b…`** (295 087 B, 6 522 lines — same byte and line counts,
hash-only); `make check` **111/111** (strict `-Wall -Wextra -Werror`,
ASan/UBSan; ~220 s). The README title stays `75%` — the census did not move, so
no recompute (AGENTS.md's derived rule).

- **The verdict — the `+0x79` placement-tier producer landed, the M2 golden
  moved by the measured 852 chain, and the undriven tape's natural pan is
  measured and live — producer-conditioned at the port-reachable value 6.**
  FU328-L1's producer gap is **closed for the placement-reset writer**
  (`FUN_0007997C`); its native-per-record **value** leg is FU330-L1 (open) and
  the other two native writers — P2 the lineup rebind and P3 the code-`0x0C`
  no-replacement arm — are the P2/P3 residual (FU333-L15). The witness is
  **FU329 Branch A**: a row-04 gate pass with `has_slot 0`, `|v0| = 2`, the
  event binding at granted **f=704** and a 13-frame `T_x = −0x26` pan
  (705..717, stop 718, counter 0→2) — **with no staged slot/seed/face/timer
  cell**. The silent bind (phases-33/34's f=1517) is **gone, replaced, not
  accompanied**; the frequency is **1 pan / 3 598 granted frames (0.028 %)**.
  **Live**, the same undriven match reproduces the pan frame-exact (T3). The
  row-09 staging is corrected (`+0x7B = +0x79*2`) and **inert** on the measured
  tape.

  - **The landing (FU331's design as reviewed).** The producer is the native
    expression `word[+0x79] = byte[0x10F394 + byte[[rec+4]+0xD]] |
    byte[+0x9D]` — **one shared table, no second copy**: the port's existing
    `fifa96_arm_26_timer_table[32]` (`port/src/fifa96_loader/fifa96_arm_bodies.c:34-39`;
    `static` removed, `extern` in `fifa96_arm_bodies.h`) — the 32-byte tier
    (`06×8 07×8 0F 0F 0F 0E 0D 0D 0C 0B 0B 0A 09 09 08 07 07 06`), **15 the
    in-range maximum**, **6 not a floor**. The `0x20` boundary refuses the
    `0x10F3B4` continuation (the unguarded-index class, FU330-L2), and the
    out-of-range class **leaves the live cells UNCHANGED — never 0** (a zeroing
    fallback would re-create the silent bind; FU331 review erratum 2).
    `fifa96_match_entities_placement_limit`
    (`port/src/fifa96_engine/fifa96_match_entities.c:50-56`) returns 0 for
    `desc_d >= 0x20`; `fifa96_match_entities_placement_seed` (`:59-68`) writes
    `timer79` then `timer7b` (`0x79A4D` / `0x79A86`). The two declared staging
    cells are `desc_d` (`byte[[rec+4]+0xD]`) and `byte9d` (`byte[+0x9D]`,
    OL-72) — `port/src/fifa96_engine/fifa96_match_entities.h:138-139`; both are
    zero today (`player_d = 0`, `port/src/fifa96_engine/fifa96_match_run.c:1443`; no
    `+0x9D` producer). The writers are `fifa96_match_entities_init` (`:104` —
    the **live** derived seam for the unported `0x741C4` driver, FU331-L1) and
    `fifa96_match_entities_reset_lane` (`:266` — the exact native site, dead
    live, FU295-L1); both write `+0x79` then `+0x7B`. **The value is
    `table[0] = 6` on every record — declared staging, not native per record**
    (the roster index is per-player data not present in the image, FU330-L1).
  - **The row-09 correction (OL-82 retired for this cell only).** Native
    `word[+0x7B] = word[+0x79] * 2` (fresh `0x80B3D MOV AX,[EBP+0x79]` /
    `0x80B41 ADD EAX,EAX` / `0x80B43 MOV [EBP+0x7B],AX`); the port's staged
    literal 0 at `port/src/fifa96_engine/fifa96_match_handlers.c:5037` (the plan and
    spec cite `:5034`, the pre-fix line; the corrected row landed three lines
    lower) becomes `r->timer7b = (uint16_t)((uint32_t)e->timer79 * 2u)`. At 6:
    `timer7b = 12`, `vel73 = 12`, `speed71 = 12`, `target = pos + 96`.
    **Inert on the measured tape** — 0 code-9 record-frames (FU332 §7), so the
    retirement is a fidelity correction with a zero blast radius, pinned by P3.
    The `dword[+0x7B] >> 16` reads stay `face7d` (the `word[+0x7D]` alias — the
    FU178 trap, FU331-L5; a future reader must not "fix" the fold onto
    `timer7b`).
  - **The measured re-pin (the faithful move, T1).** First differing line
    **852** (`state=1/1-0`, `964282c7…` → `e04df34d…`), **5 671 / 6 522** lines
    (86.95 %) moved in **one contiguous tail 852..6522** (last diff 6522),
    length **295 087 B** unchanged, 6 517 tagged lines (5 untagged pre-match),
    **the 10-tag stratum set identical** (none added, lost or re-valued); moved
    by tag `1/1-0` 951/953, `19/1-0` 2 620/2 620, `2/1-0` 2 100/2 173
    (FU332's table). `d5446144…` → **`6aa2703b…`**, byte-identical to FU332's
    measured value-6 transcript; M1 `09b726b7…` **byte-identical** (`cmp`).
    The producer's pool diverges at **step 12** (the first mover, id 10; all
    20 active records by f=3); frame **852** is the phase-33 attribution class
    — the sit-1 half-time kickoff's camera reset revealing the since-372–379
    face-driven walk-in positions (FU323 §6; FU332's review erratum 4 narrows
    the "840-frame gap" to this same reveal).
  - **The unit pins (P1/P2/P3, RED→GREEN).** P1
    `test_placement_limit_producer` (`port/tests/test_engine_match_entities.c:1215`):
    every placed record `timer79 == 6` / `timer7b == 6`, the value function's
    index and OR arms (`[22] = 0x0C`, `16 | 0x10 = 0x1F`, `6 | 0x80 = 0x86`),
    the `0x20` refusal with the caller's out **untouched**, the out-of-range
    class keeping `0x0B`, and both teams. P2
    `test_placement_counter_from_producer`
    (`port/tests/test_engine_match_handlers.c:3597`): `desc_d 22 → 0x0C`, the
    install re-copy (`0x7DA9B`), the nibble floor, the no-zeroing boundary. P3
    `test_action_09_placement_limit_velocity` (`:3620`): `timer7b == 12`,
    `vel73 == 12`, `vel75 == 0`, `speed71 == 12`, `target 0x160/0x200`, with
    the zero-value companion keeping the old path reachable by staging
    `timer79 = 0` — a producer pin, not a rewrite. The RED evidence is the
    staged-zero tree (P1 value 0; P2 0/0; P3 0/0/0/0x100).
  - **The A3 re-pin and the survivors (T1).** The measured first camera
    departure is **engine step 2355 (granted f 704)** — FU332's event frame;
    the phase-33 window 4930/5200 → **2351/2359** (the measured step ∓ 4, the
    one-frame gate-vs-capture alignment; constants
    `port/tests/test_engine_m2.c:2007-2008`, asserts `:4175-4189`). A1/A2 survive;
    the A2 identity is now **704 == 704** with `camera_left_zero`
    **2 894 = 3 598 − 704** (the phases-33/34 figures 1 517 / 2 081 are
    history). The natural-pass survivors are unchanged (`granted_frames 3 598`,
    `stage.calls 6 118`, `select_ran 0`, score 0-0, the kick cap, bind id 12).
    The two **driven passes re-pinned to ONE segment each**: the claim pass gate
    **f 673** / event **f 674** (27 changed, stop place `−0x90`, off-zero
    **2 923**; the phase-33 second segment no longer occurs — the one-shot pan
    is consumed earlier), the offset pass gate **f 675** / event **f 676**
    (27 changed, off-zero **2 921**, **Δ = 0x42** at the gate), with its drive
    **re-based LEFT 30 → 10 entries, DOWN from 1830** (a 20+-entry nudge hands
    the one-shot to the producer's natural band pan at f 704 and the pass loses
    its subject — measured, FU333-L3's re-measure duty). The scripted pan
    witness is now a **declared pre-producer control** (it stages
    `timer79 = timer7b = 0`; with the producer on its LONG_BALL/UP chain
    reaches **no camera event in 6 000 steps** — measured). The six
    `test_engine_match_frame` live fixtures were re-derived under the produced
    pacing (drive re-bases, an RNG pump, the turn limit `0x38`/face `0x178`,
    the staged `has_slot` before the 92-frame follow, and the z-seeded
    half-line pan `pos_z 0 → −0x90`; the claim's slot-dir bytes are `(0,0)` on
    that frame, the dir arm covered by the handler unit pins).

- **The natural witness (T2, `673a10d`; FU333 C1–C6, Branch A,
  producer-conditioned).** The undriven `M2_KEYS` tape, no added hold, no
  staged cell. **The measured tuple:** gate **f 703 / step 2352** (id 12,
  code 4, anim 0, active 1, `has_slot 0`, cam (0,0,0), tracked −1, controlled
  1; lane 56/−56, `cam_dz6f` 0, bound 68, timer81 0, face `0xFF00`, dir (0,0);
  chain `timer79 6`, `timer7b 6`, nibble 6, vel (−6,0), `speed71 6`, pos
  (56,0,0), target (0,0); bearing `0xFFFFFF00`, Δ 0) → event **f 704 / step
  2355** (code 5, `has_slot 0`, trk 12 = id 12, cam vel (−2,0), speed 2,
  tim 0/12, ramp 12, record pos (44,0,0)) → first motion **f 705 / step 2359**
  → stop **f 718 / step 2402**, 13 changed frames, stop (−38,0,0), pan 0→2,
  `speed_seq 2,1,0`, seed (−0x18,0), `v0 (−2,0)`, `T (−38,0)`. **The asserted
  side** is the FU333 §4.3 classes: the arm cells exact (C1/W1), the seed chain
  `vel73 −6 → seed −0x18 → v0 −2` through the measured fast path, the chain
  cells exact with the nibble bound structural (C2/W3), the gate cells exact
  with the predicates structural (C3/W2), the event cells exact (C4/W4), the
  run from the pass's own seed through `m2_pan_curve` plus the one-shot
  (C5/W5, W6), and the bind identity `first_bind_id == bind_tracked == 12`.
  **Recorded only:** the exact f/step of every anchor, the bearing/Δ, the
  off-zero counts, the later segments, the achieved seed/v0/T and the
  frequency; the exact f/step values are recorded, never asserted (the
  structural frame order gate `f−2` / event `f−1` / motion `f` is asserted,
  and the witness is **seg[0]**, never inferred from pass-level cells).
  **The no-staged-cell proof (FU333 §4.4):** the diff touches
  `port/tests/test_engine_m2.c` only; no engine write is added in the natural path;
  the pass's copied-tape `memcmp` is silent (`:4140`); no staging `#define`
  exists (the only added define is the recorded-segment bound); the FU328 §5.6
  diagnostic ladder is not in the tree. The witness is
  **producer-conditioned**: the native per-record value is unmeasured
  (FU330-L1), so the exact numbers follow the port-reachable value 6 — the
  register says so, and the note and the assert comments do too.

- **The Layer-2 live acceptance (T3, `79819a4` + `21f5f44`).** The **undriven
  live match reproduced the pan, frame-exact**: event **f=704** (tick 2350),
  motion **705..717**, stop **f=718** (tick 2397), place 0 → (−38,0,0),
  pan 0→2, trk 12, follow 0 and `pos_y 0` on all 4 167 body lines;
  **trace-attributed**. The four anchors sit on a **constant −5-tick
  alignment** vs the null tape (2347/2350/2354/2397 vs 2352/2355/2359/2402 —
  the live scheduler vs the deterministic step clock, both frozen; live phase
  2 at f=539 / tick 1800). **The AE route could NOT corroborate:** the strict
  in-run pair is **AE 0 = the still's AE 0** — the renderer draws no
  pitch/background (the tracked sprite pinned at a fixed screen point), so a
  camera translation has no scene-translation pixel signature. The FU333 §7
  W-b **AE-coverage waiver is applied to the AE only — never to the trace**.
  The run-end pair is AE 502 (`23x41+458+296` — the tracked sprite's animation
  step), the event view switch AE 53 850 (`293x394+317+0` — the event, not the
  pan). The still pair is a **disclosed cross-run camera-constant control**
  (attempt 4: f=942→945 / f=948→950, clock 00:13; the accepted run's raw stills
  were overwritten by the re-run's fixed filenames — the round-1 correction;
  the committed PNGs are unchanged): same second, no key, camera cells
  constant (pos −38, trk 12, pan 2) and the siblings AE 0. The methods are the
  phase-33 ones (`docs/screens/p33-l2-notes.md`'s form): trace primary, the AE
  relative, the sprite-only bbox caveat, the extract a verbatim ordered
  subsequence (**61 lines** = 1 header + 4 comments + 56 body; the phase-2 line
  is full-trace line 541), and the still's own caveat stated. Artifacts:
  `docs/screens/p35-l2-notes.md`, the **seven PNGs** (the strict pair
  `pan-a/b`, the run-end pair `pan-c/d`, `pan-diff`, `still-a/b`) and
  `p35-l2-trace-pan.txt`.

- **The wave's verdicts (FU330…FU333, promoted one-per-commit).**
  - **FU330 (the producer census) — GO on the producer and the tier semantics;
    NO-GO on any native per-record claim.** The store is the word-wide table
    lookup `byte[0x10F394 + byte[[rec+4]+0xD]] | byte[+0x9D]` (the index is
    physically `[[rec+4]+0xD]`; the stored high byte forced 0 — its review
    errata 1–2 correct the omitted `XOR AH,AH` and the index-order sentence).
    The table is 32 bytes at `0x10F394` (the `0x10F3B4` continuation follows);
    **15 is the proven in-range maximum** (the table's `0x0F` at indices
    16..18; the `+0x9D` OR-writer emits only 0/`0x0F`), **6 is not a floor**
    (P2 halves to 3, composed 2), and no universal cap holds only because the
    index is **unguarded** (32..47 reads the continuation, `0x80+` wraps to
    `0x0FF4xx`). The native per-record kickoff values need the roster
    descriptor, which the image does not contain (FU330-L1).
  - **FU331 (the design) — GO on P1 as the placement-reset writer; NO-GO
    beyond the design.** The existing table reuse (erratum 1: no second
    `[0x30]` array), the `0x20` boundary and the no-zeroing fallback (erratum
    2), P1 as **one of three** `+0x79` writers (erratum 3: P2/P3 stay
    FU333-L15), the init-only cadence being exact while the value is constant
    and neither writer runs (erratum 4: the reset pass is prelude-only, four
    callers), the 23 audited sites being timer-cell **lines** (the W class is
    9 lines in 7 rows — erratum 5), and 15 as the in-range max / 6 not a floor
    (erratum 6). **Row 03 must not
    be wired** (FU331-L4: `min(1, limit) = 1 < 6` — a counter-lowering route,
    not a witness enabler).
  - **FU332 (the blast radius) — GO on the measured radius and on the natural
    pan; NO-GO (narrow) on calling the tuple *native* while FU330-L1 is
    open.** The probe was a value probe (the constant `table[0]` at the init
    seam — its erratum 1), so it proves the cell/seam/value, not the table
    lookup/OR (P1's pins own the expression). The whole formation moves at the
    floor value (20/20 records; the first at f 1, all by f 3; `max |v| 1 → 6`);
    the golden's 5 671/6 522-line 852 tail; M1 identical. The `+0x9D` port
    term is 0 **by construction**, the native byte unmeasured (erratum 2);
    the live native limit domain is 2..15 (erratum 3, the P2/P3 residual).
  - **FU333 (the acceptance + governance) — GO; Branch A is the landed
    branch.** C1–C6 + the no-staging clause; the golden recipe (the first-diff
    anchor 852, the `state=` check, the stratum-set gate, the 6 522-line
    invariant, M1 `cmp`, TW1–TW9); the Layer-2 design (the AE-coverage waiver,
    trace primary). Its errata: Branch A is the witness branch, Branch B the
    no-witness one (1); 852 is the measured render bound while 2355 is the
    A3/event anchor (2); the witness is **producer-conditioned**, not "the
    native golden" (3); "row-12" is action code `0x0C` (4); the phase-0C
    hazard cite (5); the spec's §5 **and** §6 carried the A3-in-P2 sentence
    (6); the memcmp is a source-level check, not a platform-consumption proof
    (7); "probably wholesale" is the measured 86.95 % (8).

- **Governance (the Task-1 review's M8 and the phase's own record).**
  - **The docs register pin** is this section + the PROGRESS narrative — the
    M8 item the T1 review deferred to the close-out.
  - **The new A2 figures:** `camera_zero_frames == first_left_zero_f` =
    **704 == 704**, `camera_left_zero` **2 894** (replacing 1 517 / 2 081); the
    A3 window 2351..2359.
  - **FU324-L5's outcome — the trigger fired, the sensitivity re-arms.** The
    `+0x7B` nibble now moves (6 at the install, 12 after row 09), so the
    `stride_len ≤ 3` clamp that masked every metric difference is gone and the
    mover's metric sensitivity **returns**. **The mutant protocol was not
    re-run**: the plan records it as a clause, not a measurement (plan Step 7,
    "FU324-L5's re-measure clause recorded"), and the provenance note carries
    the clause. FU324-L5 is therefore **re-armed, not closed**.
  - **The FU333 H10 deviation, quoted.** Quote
    (`port/spec/FU333_r2l9_acceptance.md:483`): "P1 re-runs the mutant
    protocol and records the result in the register; it is a **clause**, not a
    silent re-measure." **Recorded deviation:** the phase-35 plan deferred the
    run (Step 7 records the clause; the provenance note carries it), so the
    register records the re-arming and the deferral. The pre-producer mutant
    result (37 219 stride firings, 0 frames moved — FU324 §5) is **not**
    evidence for the produced build. Owner: the mover staging + the row-26
    timer staging (FU324-L5, carried).

- **The legs — met, closed and open.**
  - **FU328-L1 — CLOSED for its own subject (the producer gap), narrowed in
    scope.** The placement-reset writer is live
    (`fifa96_match_entities_placement_seed` at `entities_init` and
    `reset_lane`), so the natural pan's root gap is closed (FU331's erratum 3's
    "narrowed, not closed" speaks of the full native three-writer set, not of
    this gap). The **native per-record value is FU330-L1** (open; the
    native-faithfulness owner of the producer-conditioned witness: trap
    `0x79A4D` / `FUN_0007997C` on the M2 tape and log
    `[[rec+4]+0xD]`, `+0x9D`, `+0x79` for all 22 records) and the other two
    writers are the P2/P3 residual (below). **FU328-L8** (the M2 exposure) is
    **met/measured** — the re-pin **at the staged 6** (the native `2..15`
    domain is FU330-L1). **FU328-L2** (the row-03 counter) stays
    NULL/unwired (FU331-L4). **FU328-L3** is FU330-L1.
  - **FU330-L1..L8 (owners).** L1 the native per-record `+0x79` at kickoff —
    **open, the native-faithfulness owner**; owner FU84 §6 + the roster
    descriptor (FU-142b Appendix C / FU188-L5). L2 the unguarded index — owner
    the roster byte's loader (unported). L3 the `+0x9D` override's meaning —
    owner the roster/tactics flags. L4 the operand-based `+0x9D`-writer census
    — owner the roster-flags census. L5 P3's trigger semantics — owner the
    code-`0x0C` arm's phase. L6 the `0x8B832` caller of `FUN_0007417C` (no
    Ghidra function) — owner the match-state reset routine. L7 `+0x79`'s
    downstream algebra — owner FU326/FU328/FU331 (the row-09 site is now
    landed; the reader census stands). L8 the port's missing `+0x9D` cell —
    **closed** (`byte9d` landed; still staged 0, OL-72).
  - **FU331-L1..L8 (owners).** L1 the native call-site cadence — owner FU295-L1
    + the `0x741C4` driver (the derived `entities_init` seam stands; the
    kickoff/period re-store and the P2/P3 re-seeds are residuals). L2 the
    descriptor index is unported — owner FU301-L3 / the roster loader. L3 the
    `+0x9D` OR and the index domain — owner FU330 (the port models `0..0x1F`,
    refuses `>= 0x20`, keeps the live cell; unobservable while `desc_d = 0`).
    L4 **row 03 must stay unwired** — open by instruction; owner
    FU177-L1/FU328-L2. L5 FU178's "heading word `+0x7B`" is `word[+0x7D]` —
    recorded; the port reads `face7d`; owner any future reader (no code move).
    L6 the `+0x7B` distribution is multimodal — owner FU324-L5 (the metric
    clause). L7 the P3 pin's `sine(0x100)`/`distance(12,0)` identities — owner
    the entity trig/metric core. L8 the golden attribution unmeasured here —
    **met** by T1's 852 measurement.
  - **FU332-L1..L7 (owners).** L1 the native producer site is not live — the
    `reset_lane` site landed but is dead live; the live route is the derived
    seam; owner FU295-L1/FU331. L2 the measured value is port-reachable — owner
    FU330-L1. L3 the staging record has no `+0x79` — the pool entity's stands
    in (recorded in the row-09 comment); owner P1 (met). L4 the 840-frame
    render lag — **narrowed/explained** (852 = the phase-33 reveal); the
    per-sprite pixel proof stays FU323-L2's; owner a presentation-side measure
    if ever needed. L5 the out-of-range index, not the OR, is the `> 15` path —
    owner FU330-L2. L6 the A3 window re-pins — **met** (2351..2359). L7 the
    pan's place does not survive the half-time re-entry — carried (the
    half-time reset; FU328-L4).
  - **FU333-L1..L15 (owners).** L1 the tuple is FU332's — met/recorded. L2 the
    A3 commit placement (spec §6 vs the green gate) — the contradiction is
    recorded and the spec's Plan erratum item 3 settles it (P1; owner the
    Stage-2 plan/controller). L3 the driven passes are a second acceptance
    surface — **met** (re-measured, re-pinned, one segment each). L4 the
    natural-pass value pins outside A3 — met (unchanged). L5 the score-bearing
    strata — met (the 10-tag set intact, no value change). L6 the two producer
    channels — met (the first-diff anchor is the render 852; the row-09 channel
    is inert, 0 code-9). L7 the `+0x9D` OR's port value is 0 — carried (OL-72;
    a value divergence, not a mechanism one; owner: FU330 + P1). L8 one
    `+0x7B` channel per commit — met (only the placement copies + row 09
    changed; the H6 stagings untouched). L9 the row-03 idle core clamps to 1
    — met by leaving row 03 unwired (the placement copies are the enabler;
    recorded). L10 the row-26
    read is the same table and index byte — carried (FU331's audit; the port
    carries the bytes; the guard and the `>>1` are the trap). L11 the FU324-L5
    re-measure rides P1 — **deviation recorded** (the clause landed, the run
    deferred; see Governance). L12 Layer 2 conditional on the tuple and the
    measured duration — **met** (Branch A; the tuple and the measured pan
    duration; the AE waiver recorded). L13 the armer/OOB accumulation of a
    natural pan is recorded, not asserted — met (recorded). L14 the suite count
    is per-tree — met (**111/111** on this tree). L15 `+0x79`'s other writers —
    **open** (P2 the lineup rebind, P3 the code-`0x0C` arm; a window crossing
    one of these must be re-measured, never explained by the placement value).
  - **The P2/P3 residual (the live native domain 2..15).** Native P2 halves the
    limit (`+0x79 >> 1`, floor 2 at `0x8C939/0x8C942`, iff `+0x99`) and P3
    writes `max(+0x79>>1, 3)` (`0x822E7/0x822F0`, action code `0x0C` — not row
    `0x12`); a composed P2-after-P3 gives **2 or 3**. **At 3 the pan survives**
    (`|v0| = 1`, `T = −0xC`, 6 frames — measured); **at 2 the chain predicts
    failure** (`v0 = 0` on the predicted seed 8 — **not measured**; FU332's
    erratum 3). The witness therefore survives the **table rows 6..15 and
    the P2/P3 values 3..7**, **not the whole native domain 2..15**; both
    writers are unported, so the measured tuple stands, but a future window
    crossing one must be re-measured (FU333-L15).
  - **Carried unchanged (still open):** FU328-L4..L7 (L4 the undriven
    second-half stall — the row-01 kickoff release / the FU-84 per-frame
    driver; L5 the finite drive-family coverage; L6 the cone filter is not the
    gate; L7 the partial wrap classifier); FU326-L1 (the non-locomotion
    `+0x73/+0x75` writers — the only formal escape from the seed bound);
    FU319-L8 (the raw-point focus guard vs the native delta test); FU324-L8/L9
    (the `r->distance` persistence gap; the negative-metric-word stride index);
    FU325-L8 (the still-pair AE-honesty class — this phase's still is AE 0; the
    class stands); the phase-30/31/32 remainder (FU315-L1/L2, FU316-L2..L9,
    FU317-L1..L7/L9/L10, FU318…FU321) as registered. FU316-L1's travel model
    stays settled within the locomotion bound; its non-locomotion escape is
    FU326-L1.

- **Deferred polish (the T2/T3 reviews; none a fidelity defect).** (1) the
  arm-distinction assert is x-only (`port/tests/test_engine_m2.c:4243`; the vector
  form would compare both axes); (2) the `≤ 0x19` seed-metric assert
  (`:4289`) labels a bound that is the engine's event-pair cap, not the
  metric's; (3) the literal stop `−38` (`:4308`) duplicates the curve assert
  (`:4305-4307`); (4) the `timer7b == 6` equality (`:4248`) over-constrains a
  multi-writer cell — FU333 §4.4 allows a bound, not equality; (5) the
  `ev->code == 5` pin (`:4324`) is an extra pin beyond W6 (label it or drop
  it); (6) the global `y/z_nonzero` counts (`:4310-4311`, and the pass-level
  counters `:4019-4020`) go beyond W5; (7) the `m2_nat_pan` struct comment
  (`:2702-2705`) names frequency fields that do not exist (the frequency is
  derived at print time); (8) `M2_NAT_PAN_SEGMENTS 4` (`:2673`) is a silent
  cap — later segments past the bound are skipped without an assert
  (`:3953`/`:3969`/`:4013`); (9) the `rel` offsets and the sample hash
  (`:4030-4047`) are not printed although the report calls them recorded
  (the witness stderr prints f/step, not `rel`; `:6992-7033`); (10) the claim
  pass's per-segment cells are not zeroed at a segment start
  (`:4686-4703` — the natural sampler `memset`s its segment, the claim sampler
  does not, so `decrease/increase/pan_max/stop_*` can carry over); (11) the
  ring identity asserts in `m2_natural_sample` (`:3953`/`:3969`) run
  unconditionally in the no-ISO mode though FU322-L10 keeps that window
  recorded-only; (12) the witness never closes `g->id == ev->id` (`:4227-4235`
  vs `:4283`); (13) the Task-2 report's no-ISO anchor line (`stop 718`) is off
  by one against the recorded 717 — a report-accuracy item, not a tree claim.
  The Task-3 wording note: `docs/screens/p35-l2-notes.md:41` said "Every trace
  row in this note is this run's own cell" without the still-control
  exception its own row (`:154`) and section (`:178-183`) carry; the note's
  `row-12` label (`:38`) is the wave's corrected action-code-`0x0C` name
  (FU330's review erratum 3). Both are fixed by the final-review wave
  (`fix(fu330)` — the sentence is scoped to the **pan** rows, the still pair's
  bounds are the re-run's control, and the `:38` label carries the
  action-code-`0x0C` name).

- **Tape and gate.** Docs-only, proven: M1 `09b726b7…` (22 004 B, 691 lines)
  and M2 `6aa2703b…` (295 087 B, 6 522 lines) byte-identical (`cmp`-verified);
  no re-pin in this close-out, no frame diff, no fixture change and no
  `state=` change; `make check` **111/111**; census **`60/80 = 75%`**
  (`37 action + 23 phase`; fresh re-count); the README title stays `75%`; the
  ISO untouched. The phase's production change is T1's producer + row-09
  correction (`749cc2f`) plus the T1 fix rounds; T2 is test-side and T3/the
  close-out are docs-only. The trace sink stayed default-off outside T3's
  acceptance run; the capture/trace artefacts are `docs/screens/p35-l2-*`.
  **The next-phase candidates, in order:** the **roster index (FU330-L1) and
  the P2/P3 native live domain** (2..15; 2 fails, 3 pans) — the
  native-faithfulness work that would let the witness stop being
  producer-conditioned; the **claim/carrier fusion** (FU315-L1 + FU316-L8: the
  row-05 carrier camera event `0x7F4E7` and the claim tail — the
  pan→row-5→rebind half does not exist in the engine); the **armer/follow**
  (FU316-L6 + FU316-L9: the `0x7131C` announce/threshold/far-band body and the
  unarmed mirror); the **undriven half-time stall** (FU328-L4: phase 12 →
  phase 1 at step 13805, clock 120 s, no transition through step 160 000 — the
  row-01 kickoff release / the FU-84 per-frame driver).

## M2 phase-36 register (FU334…FU338; 2026-11-04) — the completion census: 78/78 native dispatch bodies ported (100%), 55/80 wired (62/80 install-live)

**Scope/provenance.** The phase-36 spec
`docs/superpowers/specs/2026-11-04-fifa96-m2-phase36-completion-design.md` and
the recon-ahead plan
`docs/superpowers/plans/2026-11-04-fifa96-m2-phase36-recon-ahead.md`; the
thirteen read-only recon reports under `/tmp/opencode/recon/` (R1a/R1b/R2…R12)
fanned out in parallel. The four evidence docs are FU-334 (action `0x0A`),
FU-335 (phases `0x13/0x14`), FU-336 (phase `0x1B`) and FU-337 (the census);
FU-338 is the wiring + natural-reach audit. This section is append-only; the
phase-35 text above stands as written.

- **The metric split (FU-337).** The dispatch tables answer "is the row
  wired?"; the new `fifa96_match_port_table[80]` answers "does every native
  slot that has a body have a C implementation?". Native census: 45 action +
  35 phase = 80 slots; **2 body-less slots** — action `0x2B`
  (`0x1106E0[0x2B] = 0x87738`, the shared row-29 epilogue `RET`) and phase
  `0x16` (`0x110794[0x16] = 0`, the loader INT3 sentinel). **78 native-body
  slots**; all 78 now have a body. The README title is
  `floor(100 * 78 / 78) = 100`; the wired metric is separate:
  **`55/80 naturally wired = 68%`** and `62/80 install-live = 77%` (the seven
  producer-blocked rows below are subtracted from the natural count). The
  census test `port/tests/test_engine_match_port_census.c` prints
  `port-census: bodies 78/78 (100%) no-body 2 wired 55/80 install-live 62/80 action 37/45 phase 25/35`
  and refuses a dispatch/port `fn` mismatch; it also pins the typed body
  pointers (the loader-shaped rows are compile-checked, not string-only).
- **The three missing bodies.** **Action `0x0A`** (`0x81738..0x81905`, not
  `..0x818F5`; FU-334): the phase-2 gate, stage 0 nearest / `> 0x50` reject /
  `0x8DCD4` distance / `0xCD474` bearing / `0x10F394` tier OR the self `+0x9D`
  high byte into `+0x71` / `0x114E04` sine through `0x795A4` into `+0x73` /
  `0x6E598` anim on self, stage 1 `+0x44` ack. The installer cluster `0x7CDD8`
  is absent from all eight collision tables and its address occurs nowhere in
  the image, so the row is ported-but-not-wired (DEAD originator). **Phases
  `0x13/0x14`** (`0x6E244..0x6E32A`; FU-335): the side gate, the inactive
  cell copy with the `ac2 < 4 && side == 0` z negation, and the active
  **four**-draw scatter (two magnitude/sign pairs; x never negated; writes only
  `+0x4D`/`+0x55`). The `FUN_0008D098` case-`0x13/0x14` producer
  `0x8D693..0x8D722` is ported as `fifa96_match_phase_machine_scatter`; the
  cells `0x10F364/0x10F368` are not produced on the live walk yet because the
  body consumes the shared RNG stream and phase `0x13` is on the tape (the
  measured re-pin is the carry). **Phase `0x1B`** (`0x890EC..0x8910D`;
  FU-336): the 34-byte pure act-slot reset (word timer, id, stage, handler;
  no CALL, no phase write). A fresh 11-site `CALL 0x888FC` census passes no
  EAX=5 and the 27 `FUN_000740A0` sites emit no phase `0x1B`; the row is
  ported-but-dormant.
- **The wiring wave (FU-338).** **Phase `0x0D`** is wired to the shared reset
  `fifa96_match_phase_00`: the action-row-`0x1A` stage-2 sit-7 emit
  (`0x76A90`, ported as `set_piece(mr, 7, ...)`) reaches
  `match_run_write_phase(0x0D)` + the `0x8D65D` arm; the earlier NULL verdict
  predates the phase-28 T5 row-1A wiring. **Phase `0x1C`** is wired to
  `fifa96_match_action_phase_1C`: the offside reception now dispatches act 6
  invoke-now (`0x8A784/0x8A789` kind-3 arm) and the clock-tail act tick
  advances it; the duplicate `ref_machine`/`referee_step` route is retired.
  Census `60/80 → 62/80` install-live (+1 phase `0x0D`, +1 phase `0x1C`);
  the natural-reach count is `55/80`. Both are tape-neutral: the golden carries
  no phase-`0x0D` frame and the offside enable `field_4c2f2` defaults 0.
- **The natural-reach audit.** A fresh source audit of the 60 pre-wave wired
  rows found seven counted rows whose producer is currently structurally
  blocked in the engine: action `0x25` (the `ac5 != ac7` discriminator is
  test-only), action `0x28`/`0x2A` (the phase-`0x14` chooser arm is unfed; the
  M2 tape's forced phase `0x14` window is test staging, not a natural
  producer), phase `0x06` (the `contact_kind != 3` arm is unreachable while the
  unmodelled `[0x158882]` flag is 0), action `0x13`/`0x1F` (their only
  installer is the dead phase 6) and phase `0x1F` (the begin act-9 seat is
  zeroed before the first tick). They remain counted under the project's **installer-live**
  definition; FU-338 §4 names each gap and the minimal staging fix. No count
  was lowered and no producer was fabricated.
- **Gate and tape.** `make check` **112/112** (strict `-Wall -Wextra -Werror`,
  ASan/UBSan; the new census test is #112). M1 `09b726b7…` (22 004 B, 691
  lines) and M2 `6aa2703b…` (295 087 B, 6 522 lines) are byte-identical
  (`cmp`); no re-pin, no frame diff, no `state=` change. The README title moves
  `75% → 100%` under the port-coverage derivation; the ENGINE headline keeps
  both numbers. The seven natural-reach gaps, the R1a producer/re-pin legs,
  R8-L1/L2 and the FU193-L4 residual / FU285-L6 second sit-7 producer are the
  carried legs.

## M2 row-`0x25` natural producer (2026-11-04) — natural wired `55/80 → 56/80`

**Scope/provenance.** Fresh Ghidra read of `/FIFA96.EXE`
(`disassemble_bytes 0x8D760`, 80 bytes): `0x8D76C MOV AX,[0x00157AC5]`
(`66a1c57a1500`), `0x8D772 CMP AX,word ptr [0x00157AC7]`
(`663b05c77a1500`), `0x8D779 JNZ 0x8D79D` (`7522`) → install code
`0x25` (`0x8D79F MOV ECX,0x25`, `0x8D7AD CALL 0x8CEB8`); the equal arm
falls to `0x8D77D MOV ECX,0x3` / `0x8D78B CALL 0x8CEB8`. The compare is
**16-bit**; the engine already carries the two goal words as the live
`mr->score[0]`/`mr->score[1]` pair (`fifa96_match_run.h`, written by the
FU-72/C3-OL2 score path).

**Change.** `fifa96_match_phase_machine_step`'s phase-`0x13` arm now reads
`mr->score[0] != mr->score[1]` instead of the test-only `pm->ac5`/`pm->ac7`
byte mirrors, and the `ac5`/`ac7` fields are removed from
`struct fifa96_match_phase_machine`. *(errata, this slice: the OL-81 bullet
above — "the FU-142a decision compares `ac5`/`ac7` byte-wide" — no longer
describes the code; OL-81 is closed by construction. The mirror was only
ever written by tests, so live it read 0 == 0 and always chose 3.)*

**Census.** Action `0x25` leaves `fifa96_match_port_blocked`
(`FIFA96_MATCH_PORT_BLOCKED_ROWS 7 → 6`): the producer is now the live score
pair on the `FUN_0008D098` phase-`0x13` path, the same arm the native installs
from. Install-live stays `62/80`; the natural-reach count moves
**`55/80 = 68%` → `56/80 = 70%`**; the port-coverage title stays `78/78`
(100%). The remaining six producer-blocked rows are action `0x13`/`0x1F`/
`0x28`/`0x2A` and phase `0x06`/`0x1F` (FU-338 §4). The census print line is
now
`port-census: bodies 78/78 (100%) no-body 2 wired 56/80 install-live 62/80 action 37/45 phase 25/35`.

**Tests.** `port/tests/test_engine_match_phase_machine.c` stages
`mr.score[0]/[1]` (9/4 → `0x25`; 4/4 → 3 with record 0 `0x19`) and adds the
OL-81 width case (`0x0100`/`0x0000` → `0x25`, `0x0100`/`0x0100` → 3); a
byte-width model fails the first of those. `port/tests/test_engine_match_port_census.c`
pins `6` blocked rows and `56` natural. The M2 golden is untouched in this
slice; the controller does the final re-pin (expected: row `0x25` joins the
transcript, first diff frame 3299, 3224 lines, `state=` tags unchanged).
## rows09-0c — row `09` arms 1/2/3 + row `0C` stages 1..6 (errata to the FU178/FU169 entries above)

Prior sections above are not rewritten; this is the quoted correction.

- **Row `09` is now FULLY ported** (`0x80A00..0x81066`, table
  `0x809F0 = {0x80A3F,0x80BFB,0x80FCC,0x8103A}`). Arm 1
  (`0x80BFB..0x80FCB`, fresh `disassemble_bytes` this slice) lands the
  `[0x15877C]` head gate, the `[[rec+0x28]]==0x11` template and
  `[rec+0x3D]==2` frame gates, the `[0x157750]`/lane/`0x8DD70`-angle chain,
  the `0x8ED40`/`0x8F188` event path (`[0x15877C]=1`), the
  `0x8DCD4`/`0xCD474` target block, the `0x8A3FC` contact register, and both
  second-order installs: **`0x0C` at `0x80EFA` (invoke-now)** and **`0x0D` at
  `0x80F61` (deferred)**. Arm 2 (`0x80FCC`) and arm 3 (`0x8103A`) land their
  gates, the `0x8DE8C` `[team+0x7B2]` re-select (point `0x157788` = the camera
  triple, skip = the current target's `+0x8D`) and the `0x79B1C` snap +
  `[0x15872F]` counter. `mr->match09_leg_stage` is retired; the native cells
  `[0x15877C]`/`[0x15872F]` are modelled as
  `match09_seq_flag_15877c` / `entities.ball.pos_counter_f`, and the event ids
  as `match09_event_id`. **FU178-L1 is closed.**
  - The head gate shadows the `0x80FA4` flag-set event-`0x5E` arm: a set flag
    already advances at `0x80BFB`, so that arm is dead (kept literal).
  - Legs: the cross-record invoke-now re-entry (the target's row-`0C` body runs
    on its next frame-walk dispatch — the row-07 `match_kick_invoke_opponent`
    precedent; a same-frame nested dispatch needs a record-staging helper);
    `[rec+0x24]` (staged 0 = no owner); the `0x8ED40`/`0x8F188`/`0x92820`/
    `0x7A490`/`0x974F0`/`0x651F0` sinks; `byte[[rec+4]+0x16]` (`desc_16`,
    producer unlocated, staged 0, FU245-L3/FU256-L2).
- **Row `0C` is now FULLY ported** (`0x81C90..0x8250B`, table
  `0x81C74 = {0x81CDC,0x81F4E,0x81FA7,0x8244B,0x8247D,0x824CD,0x824F2}`).
  The native arms **fall through** (each advance at `0x81F8F`/`0x82433`/
  `0x82465`/`0x824B5`/`0x824DA` lands on the next arm; FU-245 §2's
  "row-09 arms fall through" note is the erratum here — row `09`'s arms each
  RET). The stage-0 foul tail's `0x81F48` advance likewise falls into arm 1.
  Stages 1..6: the anim-`0x0D`/`0x52` retry, the `[0x14C30A]`/`0x92AC8`/`+0x99`
  gate with the `0x822DB` swap terminal (`+0x79>>=1` min 3, `+0x93=0`,
  event `0x61`) and the `0x82317` event ladder (`[0x15888C]` kind + the
  `[0x158897]`/`[0x15889F]` point band), the act-id `{3,8}` stage-3 retry, the
  stage-4 `+0x89 >= (+0x99 ? 0x4B0 : 0x12C)` → anim `0x0F` install, the
  stage-5 `+0x44` gate, and the stage-6 `+0x89 > 0x3C` reset.
  - Legs: the `0x14C360/0x14C3A0/0x14C3B0` swap-selection tables (every exit
    reaches `0x822DB`, whose record effects are ported); the `[0x15888E]`
    foul latch (staged 0); the `0x8F188` ring (`match0c_event_id`); the
    `0x92040`/`0x651F0` sit sinks. **FU169-L4 is closed.**
- **Row `0D`'s install is now live but its slot stays NULL.** `0x80F61` runs
  inside the ported arm 1, so `fifa96_action_sequence_0d_step` is installed on
  the target; the dispatch slot remains unwired because wiring it flips
  `action_install 37→38` and must move the README/ENGINE census numbers in the
  same commit (AGENTS.md). This is the remaining wiring decision, not a
  producer leg.
- **Census unchanged** (`78/78` bodies, `55/80` naturally wired, `62/80`
  install-live): no body slot was added and no dispatch slot was wired.
- **Gate.** `test_engine_match_handlers`, `test_engine_m1`, `test_engine_m2`
  and the full CTest suite pass in the worktree; the three ISO-gated Python
  tests (`test_patch`/`test_callers`/`test_vgt_capture`) skip only because
  `data/game/FIFAPCCD96.iso` is not present. The no-ISO `test_engine_m1`/
  `test_engine_m2` output is byte-identical to the base commit (the golden
  `cmp` itself is ISO-gated and therefore not available here); no golden was
  edited.
## Row-1d arm wave erratum (2026-11-04) — act 7 / phase `0x1D` arms 2..8 ported (the body reaches its terminal)

**Scope/provenance.** Garden-variety arm port on the already-counted phase
`0x1D` row; no new body, no new wiring, no census move (the phase table keeps
`{0x1D, fifa96_match_action_phase_1D, ...}`, `78/78` bodies, `55/80` natural /
`62/80` install-live). Every address below is a fresh Ghidra read of
`/FIFA96.EXE` this round: `disassemble_bytes 0x89A40` (576 B) and `0x89C7D`
(562 B) cover the full arm cascade; `disassemble_function` re-read `0x4BEC8`
(`XOR EAX,EAX; RET`), `0x45001` (`AL = byte[0x1067EC] & 0xF0`), `0x4C374`
(`word[0x157A68] = 0`), `0x741B4` (`(x ^ byte[0x157ABE]) & 1`), `0x897F4` (the
free-record bind) and `0x4BDF8`/`0x4BD38` (the `0x1587D4` strip readers);
`get_xrefs_to 0x1587E9` = 4 WRITE / 0 READ.

- **The structural finding.** `0x89868` is a FALL-THROUGH cascade, not a table
  of RETURN-terminated arms: a passing arm advances `[0x158829]` and runs the
  next arm body in the same call (`0x899ED`->`0x899EE`, `0x89A48`->`0x89A4E`,
  `0x89B11`->`0x89B16`, `0x89B4A`->`0x89B50`, `0x89B8E`->`0x89B94`,
  `0x89BB0`->`0x89BB6`, `0x89E68`->`0x89E6E`). Arm 3 is the only exception:
  both of its exits RETURN (`0x89AAE JMP 0x89EA6`; the `0x89AB3` tail ends in
  its own `RET` at `0x89AE0..0x89AE9`), so arm 4 runs on the next call. The
  port now mirrors the cascade with one `switch` in a `for (;;)` loop, so a
  single granted-frame tick can walk the whole tail (including the arm-9
  terminal on the arm-8 pass frame).
- **The arms.** 2 `0x89A4E`: actor record vs `[0x158730]` (the ball-pair
  actor, `mr->entities.ball.pair.actor`) and `byte[0x158746]` (the pair `ack`,
  `0x7A2F0`). 3 `0x89A7A`: `[0x15781D]`/`[0x15781E]` = `goal_armed`/
  `goal_zone` (header bindings at `match_run.h:886-904`) -> `0x651F0(6)` /
  `0x974F0(0x5DC)` (sinks, FU194-L3), then stage 4 + RETURN; the `0x89AB3`
  tail advances on `word[0x1577BE]` (camera.speed) OR `word[0x1577F0]`
  (camera.event_param) and RETURNS. 4 `0x89AEA`: BOTH zero -> stage 5, falls
  into 5. 5 `0x89B16`: `word[0x15881A] >= 0x3C`, and `>= 0x78` or
  `0x45001()`; the derived cell is `state.tick_total` (the same substitute
  arm 1 and phase `0x17` use) and the `0x45001` input cell `[0x1067EC]` has no
  engine home (FU184-L3/FU188-L2 sibling), so the middle window holds. 6
  `0x89B50`: actor code `+0x91 != 0x20`; the `[[actor]+0x7A6]+0x44` gate is
  derived ready — the native ball record is the record-11 spill slot the pool
  does not model (FU-142 OL-49) and its row-`0x1F` producer (`0x899CD`) is
  FU194-L8. 7 `0x89B94`: `0x4BEC8` (constant 0) -> stage 8.
- **Arm 8 (`0x89BB6..0x89E6D`), the substitution-strip selection block.** The
  state cells are the native `0x1587D4` block that the engine already keeps as
  the FU-152 R1 strip view `mr->render.sub`: `record[0..0x11]` =
  `[0x1587D4..0x1587E5]` (the `FUN_0004BDF8` pair is `record[4]/record[5]` for
  `cursor < 5` and `record[0x10]/record[0x11]` above it), `current_side` =
  `byte[0x1587E6]`, `cursor_raw` = `byte[0x1587E7]`, `special` =
  `byte[0x1587E8]`, `flags[i*2+side]` = `byte[0x1587DA+idx]`. Two arms: the
  cursor `< 5` rank compares (`4-sel`, `+1` when the actor's side is the strip
  side) and the `>= 5` number pair; the winner writes the score pair
  `word[0x157AC5 + 2*idx] = 1 / word[0x157AC5 + 2*(idx^1)] = 0` (idx =
  `winner ^ [0x157ABE]`), sets `[0x1587E8]`, posts `0x8F188(0xBA,0x20,rec or
  team)` (recorded as `match1d_event_id`), clears `word[0x157A68]` via
  `0x4C374(7)` and advances to 9. The no-winner arm calls
  `0x897F4(side, (int8)[actor+0x8D]-1)` (the `match_run_free_record` bind),
  copies `[0x1587CC+(side^1)*4]` into `[0x1587D4]` and resets the act to
  stage 0 — this lands the FU217-L2 arm-8 rebind. The cursor rotates only when
  the actor's side differs from the strip side, and a `% 5 == 0` crossing sets
  it to 5 and clears the 10 flag bytes (`0x89D2C..0x89D77`).
- **New cells.** `match1d_event_id` (the `0x8F188` request id; `0xBA`) on the
  run and `render.sub.rank_side` (`[0x1587E9]`, write-only — 4 WRITE / 0 READ
  fresh). Both are zeroed by init/begin (`reset_render` memsets the strip view).
- **Legs after this wave.** The unported sinks `0x651F0`/`0x974F0` (arm 3) and
  the `0x8F188` ring post (arm 8) stay FU194-L3/L7; `0x4C374` ignores its EAX
  and only clears the reader-less `word[0x157A68]` (FU194-L3/L4); the `0x45001`
  gate is the FU184-L3/FU188-L2 sibling; the record-11 ball `+0x44` is FU-142
  OL-49; arm 9's native `[0x157AC0]=1` write remains FU209-L5 (unchanged this
  round). Arm 1's `word[rec+0x65]` gates and arm 0's per-record row-3 install
  remain the existing FU194-L7 arms.
- **Gate and tape.** The targeted suite (`test_engine_match_act`, now with one
  hold test per arm plus the full 0 -> 9 drive) and the full CTest run are
  green; M1 `09b726b7…` and M2 `6aa2703b…` are byte-identical (`cmp`), no
  re-pin.

> **Census erratum (2026-11-04).** The rows09/row1d sections above quote
> `55/80`; they were already superseded when merged (row `0x25` -> 56/80,
> phase `0x1F` -> 57/80). Current is **57/80 natural, 62/80 install-live,
> five blocked** (final wave section below).

> **Erratum pointer (phase-51 T6).** The `57/80 / 62/80 / five blocked` count
> in the paragraph above is historical; the current census is **58/80 natural,
> 64/80 install-live, six blocked** — see the phase-51 close-out register at
> the tail.

## M2 phase-`0x1F` one-shot producer (2026-11-04) — natural wired `56/80 → 57/80`

The native sit-0xC install is invoke-now: `0x8AF0E MOV EBX,1` /
`0x8AF13 MOV EAX,9` / `0x8AF18 XOR EDX,EDX` / `0x8AF1A CALL 0x888FC`, and
`0x8892E TEST BX,BX` / `0x88937 CALL dword[0x158808]` runs the stage-0 body in
the install frame. The engine previously passed `invoke=0` for act 9, then the
begin reset cleared the slot before any act tick.

`match_run_situation_side` now passes `invoke=1` for `out.act == 9` (the same
flag already used for acts 2 and 7). The live begin path executes
`fifa96_match_action_phase_1F` stage 0 once, then the native-shaped match-init
reset and act-0xA install supersede it (`0x4B095`/`0x4B0A5`). This is the only
ordinary-match reach; the sustained restart chain remains Practise-only
(`[0x14C32A]`).

Census: phase `0x1F` leaves `fifa96_match_port_blocked`; `FIFA96_MATCH_PORT_BLOCKED_ROWS`
is 5, install-live is 62/80, and the natural count is **57/80 = 71%**. The
port-coverage title stays 78/78 = 100%.

M2 moves at the restart window: first differing transcript line frame 466
(`state=21/1-0` → `state=4/1-0`), 5985 of 6522 lines; the re-derived pins are
`gate_step - phase15_step` 96 → 93, `phase4_step - phase15_step` 100 → 96, and
`restart_step - phase4_step` 303 → 304. The final re-pin is done with the
other body branches in the same commit; M1 `09b726b7…` is exact.

## Final faithful-body wave (2026-11-04) — body closures, natural 57/80, M2 v24

This wave closed the body gaps the independent audit found, without changing
the 78/78 slot-coverage definition:

- action `0x0A`: the second quarter-sine fold writes `vel75` (`0x818B9`).
- action `0x09`: arms 1–3, including the `0x0C`/`0x0D` installs at
  `0x80EFA`/`0x80F61`.
- action `0x0C`: stages 1–6, including the native fall-through and the
  `0x822DB` swap terminal.
- phase `0x1D`: arms 2–8 as the native fall-through cascade; arm 9 now mirrors
  `[0x157AC0]=1` (`0x89E8C`, FU209-L5 closed).
- action `0x1B`: stage-1 metric/face/heading/rotation/steering; action `0x1C`:
  the off-target `0x77856` rotated arm.
- phase `0x03/04/07`: the DX<0 cell-map arm (`0x6D870` + `0x6DCC8`); phase
  `0x05`: the goal-ring arm.
- action `0x17`: stage 2/3 formation sync, placement limit, anim and stash
  copies; action `0x24`: stage-1 ball gate and stage-2 `0x6E1D0` release.
- producers: the phase-`0x13/0x14` probe seam (`mr->probe_a/probe_b`), action
  `0x25` on the live 16-bit score pair, and phase `0x1F` via the native
  invoke-now sit-0xC arm.

Withheld honestly: the action-`0x24` stage-0 threshold wait (`0x865CB..0x8660B`)
is the FU188-L3 leg; landing it changes the FU333 natural-pan witness
(`code 4 -> 3`, seed pair), so the derived no-wait arm preserves the accepted
tape until that witness is re-derived. The remaining sinks/inputs stay numbered
legs in their FU records.

Census: **78/78 bodies = 100%; 57/80 natural = 71%; 62/80 install-live = 77%;
5 producer-blocked rows** (`0x13`, `0x1F` action, `0x28`, `0x2A`, phase
`0x06`). M1 `09b726b7…` is exact. M2 re-pinned to **v24**
`274e74314d9cb95960e2a8db214ec37528d194f3941e41cb8c7a65d81bfa2f83`:
first differing line 466, 5985/6522 lines, state-tag set unchanged (counts
`21/1-0` 100→96, `4/1-0` 303→304, `12/1-0` 4→7). Final gate: **112/112**.

## Phase 37 — live-record fidelity: row-1B/1C stage-1 tails + row-09 invoke-now (2026-11-04)

Fresh Ghidra reads closed three faithful runtime legs without wiring a dead
producer or moving the census.

- **Row `0x1B` stage 1.** `0x76F5A..0x76F91` selects the `0x10E180` anim row by
  `index = byte[0x157C54] + 2*(dword[+0x51] > 0x50)` (the unaligned dword at
  `0x10E17D + index`, `SAR 0x18`); `read_memory 0x10E180` = `34 36 38 3A`,
  i.e. `{0x34,0x36,0x38,0x3A}`. The install syncs `anim_id/frame/row44`, then
  the native tail stores `timer89=0` (`0x76FA7`), `byte[0x157C55]=0`
  (`0x76FBB`) and advances to stage 2. Row-1B stage 3's separate
  `0x79C50/0x6E598` actor event remains a leg; the stage-1 residue is closed.
- **Row `0x1C` stage 1.** `0x7779C..0x777CC` is the shared `0x79C50` face on
  `-camera.vel_x/-camera.vel_z` plus the `0xCD474(dx,dz)` heading into the
  `timer7f` low word. `0x7791F..0x7798B` picks the off-target id
  `{0x36,0x34}` below `target_y 0x40` and `{0x3E,0x3D}` at/above it, using the
  rotated `vel_x*word[+0x69] > vel_z*word[+0x67]` cross-product; the shared
  `0x77997` install is `0x3C` on-target. Both installs sync
  `anim_id/frame/row44` before the native `0x779D1` `+0x44` test. The
  `0x779DE..0x77E8B` tail stays FU186-L2.
- **Row `0x09` invoke-now.** Native `0x7D9A4` with `ECX=1` calls
  `[rec+0x18]` immediately (`0x7DAA3..0x7DAAA`). The engine now stages the
  target's row-0x0C inputs, dispatches it once, writes the handler's record
  effects back and restores the outer row-09 staging; the target's own walk
  still dispatches row 0x0C again, matching the native installer + machine-tail
  pair. Fixture: the nested foul tail sets `stage92=1`, `skip_98=1`, the
  `0x0D/0x52` anim and the `dispatched_ok` 0x0C bit.
- **Row `0x0D` body fidelity.** The native `0x702F8` argument is the word at
  `+0x83` (`0x82574` stages 0x20; `0x8257D` + `0x82588` is the dword-alias read
  of `dword[+0x81]>>16`), not `+0x81`. `fifa96_camera_ramp(0x20)=21`, so arm 1
  waits; the loader body now stages 21 and reports it. The row's dispatch slot
  stays NULL because the live install gate (`desc_16`/`[rec+0x24]`) is
  unproduced; wiring it would be a fake natural count.

Census unchanged: **78/78 bodies, 57/80 natural, 62/80 install-live, 5 blocked**.
The row-1B stage-1 `+0x44` install now delays the sit-5 emit to the native
successor frame; the frame fixtures step the ported successor explicitly (the
headless loop does not drive the render/anim advance, FU159-L5). M1
`09b726b7…` and M2 `274e7431…` are byte-identical (`cmp`); `make check`
**112/112**.

> **Erratum.** Earlier statements that the row-0x0D installer `0x80F61` sits
> inside an *unported* row-09 arm 1 are superseded: arm 1 is ported and the
> `0x0C` invoke-now is closed; the row-0x0D dispatch slot still stays NULL
> because the live install gate (`desc_16`/`[rec+0x24]`) is unproduced.

## Phase 38 — row `0x0D` wired as producer-blocked; native arm surface (2026-11-04)

This supersedes the phase-37 erratum's "still stays NULL" sentence: the row is
now wired, but explicitly producer-blocked.

Row `0x0D` now has the engine adapter `fifa96_match_action_0D` over the
ported loader body `fifa96_action_sequence_0d_step`; the dispatch table slot is
non-NULL. The sole installer `0x80F61` runs inside ported row-09 arm 1, but its
live gate is dead because `desc_16` and `[rec+0x24]` have no producer. The row
is therefore **install-live and in `fifa96_match_port_blocked`**: natural
stays 57/80, install-live moves 62/80 → **63/80**, blocked 5 → **6**.

The adapter maps the loader state/out surface: `+0x83=0x20`,
`0x702F8(0x20)=21` into `+0x85`, `+0x87=0`, the arm-0/1 anim installs,
velocity scale, target commit, metric, face and the `0x7DAB4` reset. It does
**not** apply row-0x24's `pos_x=0x9F0` stage-0 seed: `0x8657A/0x865AE` belong
to row `0x24`, not `0x0D`. `test_engine_match_handlers` pins the wired/blocked
dispatch and the hold/release path. Residual: the adapter applies the arm-0/1
anim through the engine install after the loader step, so arm 2's `+0x44`
gate reads the pre-install byte; row `0x0D` is producer-blocked and this is
recorded as FU180-L9, not live.

Census: **78/78 bodies = 100%; 57/80 natural = 71%; 63/80 install-live = 78%;
6 producer-blocked rows** (`0x0D`, `0x13`, `0x1F`, `0x28`, `0x2A` action,
phase `0x06`). Action install-live is 38/45. M1 `09b726b7…` and M2
`274e7431…` are byte-identical (`cmp`); `make check` **112/112**.

## Phase 39 — row 0x24 stage-0 threshold wait; FU333 record-21 re-derivation (2026-11-04)

FU188-L3's stage-0 wait is landed. `fifa96_match_action_24` now holds on
`timer89 >= ((subtype*(([+0x8A]>>25)*60))>>4) + side*60` before the target
commit (`0x865CB..0x8660B`; equality releases). The live `player_e` is still
staged 0, so the live threshold is `60*side`; the `[rec+4]+0xE` descriptor
producer remains the open leg. The face/type writes refresh on held frames;
the target and timer/stage reset run only on release.

The wait moved the FU333 natural-pan witness from record 12 to record 21
(team 1 record 10; `active` is index-valued, measured 10). The re-derived
witness passes C1–C6 on its own cells: gate f 703 / step 2352, id 21, code 4,
anim 0, `has_slot 0`, lane 54/-54, `cam_dz6f` 2, bound 66, `timer81` 0,
face -0xFD, pos (54,0,-2), chain `timer79/timer7b` 6/6, vel (-6,0), speed 6;
event f 704 / step 2355, id/trk 21, code 5, vel (-2,0), speed 2, timer 0/12,
ramp 12, pos (42,0,-2); motion 705..717, stop 718, 13 changed frames, place
-38, pan 0→2; seed (-24,0), `v0` (-2,0), `T` (-38,0); first bind f 704 /
step 2355; 1 pan / 3598 granted frames. A2 (704 all-zero frames) and A3
(first departure 2355) survive unchanged; score 0-0 and the ladder counters
survive.

M2 re-pinned to **v25**
`ea18ee1d54c51f4283e0bde1b7fbe8bebfd1e8611353b4b48507b7672149037f`
(old `274e7431…`): first differing line 852 (`state=1/1-0`), 5671/6522 lines
differ, one hash-only tail 852..6522; the 10-tag state set and its counts are
unchanged; 295086 bytes / 6522 lines / 6517 `state=` lines. M1 `09b726b7…`
is exact. Census unchanged: **78/78 bodies, 57/80 natural, 63/80 install-live,
6 blocked, action 38/45, phase 25/35**. `make check` **112/112**.

Residuals: FU188-L3's descriptor `player_e`, FU188-L5's `0x157A38` lane write
and the `0x67800(1)` bank load stay legs; FU333/FU332 record the S6 re-pin
and the record-21 tuple.

## Phase 40 — native timer81 decay alias + drain write-back (2026-11-04)

`FUN_0007CA54` (`0x7CA66..0x7CA74`) loads `dword[rec+0x7F]` and shifts right
16, which is the word at `+0x81` (the timer itself), not the separately
modelled `timer7f` high word. The engine now uses `(int16_t)e->timer81` as
the signed limit; the old `timer7f >> 16` limit was never produced and made
every nonzero `timer81` zero on its first decay. The per-record drain also
copies `r->timer81` back to the pool after dispatch, so a handler's reload
(row 0x0F's `0x82DA7` path) is not lost.

`test_engine_match_entities` now pins the alias trap (`timer7f` high word 2,
`timer81` 5, delta 3 -> 2, where the old limit would zero). M1/M2 are
byte-identical: M1 `09b726b7…`, M2 v25 `ea18ee1d…`; `make check` **112/112**.
Census unchanged: **78/78 bodies, 57/80 natural, 63/80 install-live, 6
blocked, action 38/45, phase 25/35**. The row-0x1C tail remains FU186-L2; this
only removes its `timer81` prerequisite.

## Phase 41 — act-9 stage-0 phase-0x15 ownership (2026-11-04)

The phase-0x1F/act-9 stage-0 body now owns the native phase write:
`fifa96_match_action_phase_1F` calls the exported
`fifa96_match_run_write_phase(mr, 0x15)` (`0x88FB0 EAX=0x15 CALL 0x740A0`).
The sit-0xC install arm no longer writes phase 0x15 before dispatch; act 9 is
still invoke-now, so the write runs on the same install call. This closes the
ownership divergence recorded by FU170/FU175 without changing the phase-0x15
state arm or the stage-1/2 sinks.

`test_engine_match_act` now asserts the phase write from the handler path and
the act-9 tick path. M1/M2 are byte-identical: M1 `09b726b7…`, M2 v25
`ea18ee1d…`; `make check` **112/112**. Census unchanged: **78/78 bodies,
57/80 natural, 63/80 install-live, 6 blocked, action 38/45, phase 25/35**.
FU181-L5 stages 1/2 and the presentation sinks remain legs.

## Phase 42 — row-0x0D arm-2 `+0x44` ordering (FU180-L9, 2026-11-04)

The row-0x0D loader body now refreshes `byte44` to 0 after each arm-0/1
`0x6E598` request. Both requested rows are statically flags-0 (`0x10` and
`0x0B` in the animation table; native store `0x6E6CC`), so arm 2's
`0x826BB` gate sees the installed row byte instead of the pre-dispatch
staging byte. The engine adapter still installs the anim after the step, but
the in-call gate is now native-ordered; the pool view is updated by the
adapter. Fixtures stage a stale `byte44 = 1` and assert the stage-3 advance
in both the loader and engine paths.

FU180-L9 is closed. M1/M2 are byte-identical: M1 `09b726b7…`, M2 v25
`ea18ee1d…`; `make check` **112/112**. Census unchanged: **78/78 bodies,
57/80 natural, 63/80 install-live, 6 blocked, action 38/45, phase 25/35**.
Row `0x0D` remains producer-blocked by `desc_16`/`[rec+0x24]` (OL-22).

## Phase 43 — live-leg audit: act-9 stages 1/2, row-05 claim tail, stage-0 order (2026-11-04)

FU339 records the fresh derivations and the measured reason none of the three
candidate legs is wired in this phase.

- **Act-9 stages 1/2 (FU181-L5).** `0x88FED` holds while `0x74110` finds a free
  record in either bank; the success path retries the four-slot `0x93850`
  lookup (stride `0x25`, side `+0x22`, player `+0x1C`) and calls `0x361B0`.
  `0x89064` waits for `(slot.pressed & 0x70) != 0`; otherwise `0x93930` runs
  and the stage stays. The M2 tape has free records and no `0x70` press, so a
  faithful wire would stall the restart. Stage 3's gate is
  `word[0x158818]` (the act timer), not `word[0x15881A]`; the engine already
  uses the timer, and the stale comments/evidence were corrected.
- **Row-05 claim tail (FU315-L1/FU316-L8).** The full `0x7F3A1..0x7F57B`
  algebra, `0x8DC68` metric, `0x71C94` event/tracked bind and
  `has_slot ? 4 : metric>>2` release are derived. A bounded implementation was
  reverted: the natural-claim probe moved from stop 702 / 27 frames /
  x `-0x90` to 850 / 175 / `-0x736`, and the long-ball `cross_ok` probe
  failed. The parity `0x79CCC`/`0x6DA64` arm and `0x92820` sound sink remain
  legs; the probes need a dedicated re-characterization before wiring.
- **Act-9 stage-0 order (FU170-L2/FU181-L5).** Native `0x700F4` (zero camera
  triple), `0x73E28`, phase write, `0x73E08`, `0x4C324`, then timer 0. Timer
  zero moved the M2 restart gate assertion; camera/placement moved the M2
  scenario record and live held-key position. They stay legs until a full
  stage-context re-pin.

No engine behavior, dispatch table, census, or golden changed. M1
`09b726b7…` / M2 v25 `ea18ee1d…` stay exact; `make check` **112/112**.
Census unchanged: **78/78 bodies, 57/80 natural, 63/80 install-live, 6
blocked, action 38/45, phase 25/35**.

## Phase 44 — unarmed camera mirror (FU316-L9, 2026-11-04)

`fifa96_match_goal_arm` now runs the native `0x718A9` mirror gate on the
disarmed fail paths as well as the armed path. The phase check at `0x713A0`
and the armer bound at `0x713C0` both jump to the mirror, whose tighter
`|z| > 0xB10` / `|x| > 0x720` gate can fire in the `0x720..0x730` gap without
setting `goal_armed`. `match_run_goal_mirror` passes
`transition_active = goal_zone != 0`, so a set zone or clear input bit still
takes the unported angle arm (`0x719A0..0x71B98`, FU-71 leg 9.6); a mirror
clears `goal_armed`/`goal_zone` and negates the mirrored velocity.

Tests pin the disarmed z mirror, the `0x720..0x730` x mirror, the velocity
negation and the zone-set no-op. The M2 tape keeps `input_bit0 == 0`, so the
golden is byte-identical; the live frame suite passes. Census unchanged:
**78/78 bodies, 57/80 natural, 63/80 install-live, 6 blocked, action 38/45,
phase 25/35**.

## Phase 45 — act-9 stage-0 timer zero (2026-11-04)

The phase-0x1F/act-9 stage-0 body now zeroes `word[0x158818]` at `0x88FE0`
before the stage-3 wait, matching the native `0x88F81..0x88FE7` order. This
is the first native stage-0 write the handler was missing; the camera reset
`0x700F4`, `0x73E28`, `0x73E08` placement and `0x4C324` presentation remain
FU181-L5 legs (adding them alone also moves live/M2 behavior).

The change moves the restart window: the act-9 state tag `21/1-0` now extends
through frame 469 instead of 465, and phase-4 (`4/1-0`) starts four frames
later. M2 is re-pinned **v26**:
`6f402095beeb7d50923964cd918874059f03b874e6a134c9c331881209cecf30`
(295087 bytes, 6522 lines, 6517 `state=` lines). First differing line 466;
5985 lines differ (4 state-only, 6 hash+state, 5975 hash-only); the state-tag
set/counts are unchanged. The act restart fixture now pins timer 2 after the
install frame (stage-0 zero + one stage-3 head add) and 56 after 27 more
frames. M1 `09b726b7…` stays exact.

Census unchanged: **78/78 bodies, 57/80 natural, 63/80 install-live, 6
blocked, action 38/45, phase 25/35**; `make check` **112/112**.

## Phase 46 — row-04 tracked gate drops the controlled fallback (FU316-L3, 2026-11-04)

`fifa96_match_action_04` now stages `is_ball_track` as
`mr->render.camera.tracked == id` only. The old `TRACKED_NONE -> id ==
controlled` fallback blocked the controlled record at the native `0x7EDA0`
early return; with the fallback gone, a record with `tracked == NONE` and
`lane >= 0x40` reaches the `0x7EDBD` receiver timer (`e->timer93 = 0x10`).
The unit pins both the NONE and tracked cases, so removing the fallback is
mutation-RED.

M1/M2 are byte-identical: the tape's gate frames already carry a live tracked
cell, and the row-04 caller path is unchanged. Census unchanged: **78/78
bodies, 57/80 natural, 63/80 install-live, 6 blocked, action 38/45, phase
25/35**; `make check` **112/112**.

## Phase 47 — landing pan-step admission (FU316-L2, 2026-11-04)

`fifa96_camera` now carries `prev_y`, the native `[0x15775C]` entering camera
Y copied by `0x73756/0x7375b` before the `0x70B94` height update. The
integrator admits the pan step on the landing arm (`prev_y > 0 &&
pos_y <= 0`, `0x737C8..0x737DA`) as well as on `timer > timer_limit`. A camera
unit pins the landing edge (the arc drops to 0 while the timer is still below
its limit, `pan_counter` advances) and the no-landing twin.

The live long-ball camera window moves 52 -> 51 because the landing arm fires
one frame before the timer gate; the fixture is updated with that measured
value. M1/M2 stay byte-identical; census unchanged: **78/78 bodies, 57/80
natural, 63/80 install-live, 6 blocked, action 38/45, phase 25/35**; `make
check` **112/112**.

## Phase 48 — row-04 sound-arm side fallback drops the controlled actor (FU171-L2, 2026-11-04)

`fifa96_match_action_04` now stages `ball_track_side` from the tracked record,
or a non-matching `0xFF` sentinel when `[0x1577CA] == NONE`. The native
`0x7EE83` arm skips before its side compare; the old controlled-actor fallback
could draw on the record's own side. A unit pins the TRACKED_NONE no-draw case,
and the live long-ball chain's deterministic RNG pump moves 4 -> 5 because the
removed draw shifts the stream.

M1/M2 stay byte-identical; census unchanged: **78/78 bodies, 57/80 natural,
63/80 install-live, 6 blocked, action 38/45, phase 25/35**; `make check`
**112/112**.

## Phase 49 — row-08 process byte and tracked compare (FU142 / OL-82, 2026-11-04)

Row 08 stage 2 now reads `[0x15872F]` from `entities.ball.pos_counter_f` and
drains the `0x814A0` increment back to it; `is_ball_track` compares the record
id with `render.camera.tracked` (`0x81498 EBP,dword[0x1577CA]`), not the
controlled actor. The process byte is shared with row 09 arm 3 and row 05, so
the stage-2 increment is a live consumer. `test_action_08_tracked_process_byte`
pins the increment (tracked NONE, controlled record) and the no-increment twin
(tracked record, different controlled actor). The old controlled fallback
fails both directions.

M1/M2 stay byte-identical; census unchanged: **78/78 bodies, 57/80 natural,
63/80 install-live, 6 blocked, action 38/45, phase 25/35**; `make check`
**112/112**.

## Phase 50 — row-08 stage flag (FU142 / FU253, 2026-11-04)

`[0x15877D]` is now live as `run.process_15877d`: row 08 stages it into the
outfield state, the stage-1 gate (`0x811D6`) reads it, the projection hit
writes 1 (`0x8141C`), and the stage-2 tail writes 0 (`0x8147E`/`0x811CA`). The
only outside readers are in the unported `FUN_00083428` throw machine
(`0x834B6`/`0x834F6`). `test_action_08_stage_flag_15877d` pins the producer and
the gate; the old zero-staged input fails both.

M1/M2 stay byte-identical; census unchanged: **78/78 bodies, 57/80 natural,
63/80 install-live, 6 blocked, action 38/45, phase 25/35**; `make check`
**112/112**.

## Phase 51 — phase-`0x13` wiring + once-per-write scatter (FU-335 / FU-340, 2026-11-04)

The ported phase-`0x13` body is now live: `fifa96_match_phase_table[0x13]` is
`{0x13, fifa96_match_phase_13, …}`, and `fifa96_match_run_write_phase` calls
the `FUN_0008D098` case-`0x13/0x14` scatter (`fifa96_match_phase_machine_scatter`,
native `0x8D693..0x8D722`) after each team's walk — walk team 0 → scatter team
0 → walk team 1 → scatter team 1 — for phases `0x13`/`0x14` only. The cadence
is native: `get_xrefs_to 0x8D098` returns exactly the two `FUN_000740A0` calls
`0x740C8`/`0x740DB`, never a per-frame call. Phase `0x14` stays `NULL`
(no live writer; the probe cells `0x142FE0`/`0x142FE8` remain unfed), and the
`0x651F0(0xD)` sink is the open leg.

M2 carries three live `write_phase(0x13)` calls (steps 2502/3402/4302): 20
body dispatches and 32 body draws each (96/run), plus the producer's four
draws per team (8/write; 120 shared draws/run at `ac2 < 4`). The v27 re-pin is
first differing line **3185**, **3338/6522** lines, all hash-only, state tags
and counts unchanged, 295087 bytes, sha
`54f4498755a74f0945266c66c59164a239ebf1f5fd60d0f78e416fe5316d44fd` (old
`6f402095…`); M1 `09b726b7…` stays exact. The per-frame `machine_step`
placement is rejected by measurement (first diff 852, 5671 lines, sha
`09642d40…`) and by the two-xref cadence.

Census: **78/78 bodies, 58/80 natural, 64/80 install-live, 6 blocked, action
38/45, phase 26/35**; `make check` **112/112**. Evidence: FU-340; tests
`test_phase_13_14_rng_gate` (4 × active; +8 for the producer), the m2 rider
(32 body draws, 40 with the producers), `test_phase_13_write_seam` and the
census pins.

## Phase 51 T4 — far-wall damp/event arm (FU-71 leg 9.6, 2026-11-04)

FU-71 leg 9.6 is landed: `fifa96_camera_wall_arm` (loader camera) ports the
native `0x719A0..0x71B98` non-mirror branch of `FUN_0007131C` (fresh
`disassemble_bytes 0x719A0`: 148 instructions + the shared `0x71972` tail),
and `fifa96_match_goal_arm` now dispatches the `0x718A9` gate on
`goal_zone != 0 || render.input_bit0 == 0` to that arm instead of the no-op.
The dispatch bit is **not** a gameplay input: it is the front-end option row 2
**INVISIBLE WALLS** (`0x1C9BC` reads the value cell `0x10527C`; row descriptor
`0x10513C` -> `0x10069C`, values `0x1006AC` Disabled / `0x1006B8` Enabled).
Disabled = 0 is the image default, so the arm is the default path and the
mirror stays the option-enabled (zone 0, bit set) path.

Ported: the `0x719A0` head gate; chain A (Z) `0x719CF` with the `0xE74` length
gate, the `height < 0x24`/`|prevZ| < 0xE74` `0x8DC50` corridor damp, the
`t >= height` threshold (fixed `0x20` on the negative side) and the ±`0x180`
seed; chain B (X) `0x71A5D` with the strict `height > 0x24` corridor test, its
threshold (`0x20` on the non-negative side) and seed; the small-X `0x71AF2`
damp (`0x6D4 <= |x| < 0x8BC`, `height < 0x1E`, `|prevX| < 0x6D4`); the flag-0
`0x71972` velocity/speed store (`speed = (uint16)0xCD514`, the port's
`fifa96_entity_hypot`); and the flag-1 `0x71B37` announce (the `0x974DC`
request is **skipped for `speed <= 5`**, else 1/2/3 for 6..8/9..0xA/>0xA) with
the `0x71C94` event (`fifa96_camera_event_set`, the tracked dir/re-bind tail,
and the `0x700F4` `0x7026C` goal-zone refresh). The engine records the
`0x974DC` request in the new `camera_arm_announce` cell (the sink itself stays
the OL-27 leg). `prev_x`/`prev_z` are new camera cells captured beside
`prev_y` at `0x73756/0x7375b..0x7375d`.

M2 stays byte-identical (v27 `54f44987…`, M1 `09b726b7…`): the natural tape
never calls the armer, and the scripted goal (`|z| = 0xB28..0xB48`, `x = 0`,
`vel = (0,5)`) sits inside every chain, so the flag-0 tail rewrites the same
pair and `speed = hypot(0,5) = 5`. Residual legs: the same-call
armer->arm fall-through (`0x714DA`/`0x71513`/`0x71530`/`0x7154D`; the
announce table `0x71400..0x718A4` is still unported), the `[0x15781E]`
refresh at the other `0x700F4` callers, the `0x71C94` tracked-tail cells
(FU167-L5/FU168-L3) and the dropped triple word 0 (FU168-L2).

Tests: `test_camera` pins both chains' exact threshold edges (flag 0 and flag
1), the `speed <= 5` announce skip, the 1/2/3 bucket edges, the
width-truncation traps and the prev-triple capture; `test_engine_match_frame`
pins the public dispatch (zone/bit combinations), the event request cell
(`camera_arm_announce`, `event_param`, tracked/dir, the zone refresh, the
ramp step pair) and the M2 link no-op. Twenty-eight hand mutations across
every branch and the engine wiring either fail these tests or, in one case
(a chain seed's `> 0` vs `>= 0`), are provably equivalent because a zero
camera word cannot pass the length gate. Census unchanged:
**78/78 bodies, 58/80 natural, 64/80 install-live, 6 blocked, action 38/45,
phase 26/35**; `make check` **112/112**.

## Phase 51 T5 — phase-6 stage-3 latch + blocked reasons (FU-166 / FU-338, 2026-11-04)

The act-2 stage-3 gate now models the native ball-row latch. `[0x158882]` is
the ball block's (`0x1587EC`) `+0x96` byte = `row_flags & 1`, written by
`FUN_0006E724` (`0x6E7E7 MOV [ESI+0x96],AL`) at every ball anim-row install
and cleared by the match reset (`0x73D3A`). The stage-2 body
(`0x893E2..0x89487`) installs the ball row from the contact kind — the
keep/reroll `0x89401/0x8941D` rows 0/0x62..0x65 (flags 0x07 -> 1) or the
non-3 `0x8944F/0x89464` row 0x49 (flags 0x00 -> 0) — before the stage
increment and the byte-0 split, so the gate at `0x894A2` reads **"kind was 3
at stage 2"**, not "kind is 3 now". The engine stores that net effect in the
new `mr->ball_row_flag_158882`, written at its stage-1 (native stage-2) body
for both sessions before the session-0 split, and the gate is now
`flag || contact_kind == 3 || session != 0`.

Behaviour pinned by `test_engine_referee_restart_stage3_latch` (RED first,
assertion `state.phase == 6` failed before the change): a byte-0 session with
kind 3 at the split and kind 1 at the decision decides phase 6 and installs
`0x13` on the controlled taker (team 1 record 1, target 12) plus `0x1F` on
the other team's record 0; the negative twin (kind 1 on both frames) latches
0 and the stage-3 gate holds armed with no phase write. The four existing
`test_engine_referee_restart_default_gate_split` cases stay green, including
the byte-1 (Practise) stall-through path.

The three rows stay blocked with corrected reasons: actions `0x13`/`0x1F`
("reachable only via the stage-3 latch, the mode-4 Practise term, or a
same-frame ball-row install; no ordinary-play witness") and phase `0x06`
(the same latch; `[0x158882]` has a writer; the `0x8948D` stale-local read is
the leg). The stale "writer-less `[0x158882]`" and "`[0x14C32A]` is the only
blocker" text is retired by append-only errata in FU-338 §7 and FU-166's
phase-51 erratum. Residual legs: the mode-4 Practise term `[0x14C32A]`, the
`0x8948D` read-before-write local, and the other ball-row writers
(`0x887D5`/`0x8C2CE`/`0x6FF37`/`0x6FF85`; R51-L1 release catalogue, R51-L2
runtime).

Census unchanged: **78/78 bodies, 58/80 natural, 64/80 install-live, 6
blocked, action 38/45, phase 26/35**; `make check` **112/112**. Golden: M1
`09b726b7…` exact, M2 v27 `54f44987…` byte-identical (`cmp`); no re-pin.

## Phase 51 close-out register (T6; T1–T5 outcomes, 2026-11-04)

Final register for the natural-reach + fidelity wave. The per-task sections
above stand; this is the consolidated outcome, the measured re-pin, the
census, and the carried-leg list.

**T1 — phase `0x13` wiring (FU-340, `f0762e1`).** The ported
`0x6E244..0x6E32A` scatter body is live: `fifa96_match_phase_table[0x13] =
{0x13, fifa96_match_phase_13, …}`, and `fifa96_match_run_write_phase` runs the
`FUN_0008D098` case-`0x13/0x14` producer (`0x8D693..0x8D722`) after each team's
walk — the native once-per-write cadence (the two `FUN_000740A0` xrefs
`0x740C8`/`0x740DB`), never per frame. Phase `0x14` stays `NULL` (no live
writer). M2 re-pins **v27**
`54f4498755a74f0945266c66c59164a239ebf1f5fd60d0f78e416fe5316d44fd`
(295087 B, 6522 lines; first differing line **3185**, **3338/6522** lines,
all hash-only, `state=` set/counts unchanged). Draw split: **32 body + 8
producer = 40 per write**, `96 body + 24 producer = 120` for the tape's three
writes (the m2 rider pins 32 and 40 per write; 120 is `3 × 40`, golden-pinned).
The per-frame `machine_step` variant is rejected by measurement (first diff
852, 5671 lines, sha `09642d40…`) and by the two-xref cadence.

**T2 — action row `0x03` arm fidelity (FU-341, `66fe985`).** The special arm
`0x7E1CE..0x7E223` now reads `FUN_0008DCD4`'s `out[1] = (int16_t)(0x780 −
pos_x)` through the `dword[ESP]>>16` alias (`0x7E201`/`0x7E204`), not the 2-D
metric; `FUN_00079B58`'s `+0x99` guard is applied at guard A and the arm; the
idle-core counter takes the mover-shaped `move_attr = distance << 16`
(`0x7E2E0`); the team `+0x7C3` last-defender cell (`0x7C741..0x7C770`, reset
`0x8C399`) feeds the clamp's opponent selector (`0x7E360`/`0x7E383`); and slot
branch B lands `fifa96_action_move_target` (`0x79C20` -> `0x7D3E4` clamps)
with the native `0x7E27C JMP 0x7E3A6` tail. **The row-`0x03` slot stays
`NULL`**: R11 ruled the `FUN_0008C418` clamp bounds `PCCD.PDB`-data-blocked
(R51-T2b-defer), so wiring is the carried **FU177-L1** leg. M1/M2
byte-identical; census unchanged. Tests: the dx/guard/counter/branch-B/
opponent-selector fixtures (the phase-2 SET/NONE twin and the phase-1 floor
discriminator are the T6 precision additions), plus the entity writer/reset/
reader units.

**T3 — wire row `0x03` — deferred.** R11 §1.6: the role-band line
percentages come from the `PCCD.POG` `PCDB` payload; the port has no payload
codec. The portable sub-inputs landed in T2; wiring waits the PCCD/PCDB decode
slice (FU177-L1). Cost if wrong: row `0x03` waits one data slice; no code is
thrown away.

**T4 — FU-71 leg 9.6 far-wall damp/event arm (FU-71, `2b27b94`).** The
`0x719A0..0x71B98` non-mirror branch is ported as `fifa96_camera_wall_arm`:
both threshold chains, the `shift_step` damps, flag-0 vel/speed, flag-1 seed +
`0x974DC(1..3)` announce + `0x71C94`, and the `prev_x/prev_z` cells. The
dispatch bit is the front-end **INVISIBLE WALLS** option (Disabled = 0, the
image default). The natural M2 camera never reaches the outer gate, so **M2
stays byte-identical** (v27 `54f44987…`). Twenty-eight hand mutations; the
announce table `0x71400..0x718A4`, the `0x71C94` tracked tails (FU167-L5/
FU168-L3) and the FU303-L12 `fifa96_camera_event_dir` no-slot branch stay
legs.

**T5 — phase-6 stage-3 latch + blocked reasons (FU-166/FU-338, `d23df1a`).**
`mr->ball_row_flag_158882` now models the native ball-row latch
(`[0x158882]` = `0x1587EC+0x96` = `row_flags & 1`, writer `0x6E7E7`, reset
`0x73D3A`): written at the stage-1 (native stage-2) body as
`(contact_kind == 3)` for both sessions, and the stage-3 gate is
`flag || kind == 3 || session != 0`. The RED-first test
(`test_engine_referee_restart_stage3_latch`) proves the kind-3-at-stage-2 ->
kind-1-at-stage-3 decision, and the T6 term-2 case pins kind 1 latched 0 ->
kind 3 at the decision (phase 7). **The three rows stay blocked; blocked stays
6.** The same-frame ball-install clause is ruled out statically (one-frame gap,
ball-row timers `>= 0x40`, spec §2.4); the reason now names only the kind-3 ->
kind-≠3 transition and the mode-4 Practise term. M1/M2 byte-identical.

**Final census.** `78/78` bodies (README title `100%`), **58/80 natural**,
**64/80 install-live**, **6 blocked**, action **38/45**, phase **26/35**;
README status `58/80 (64/80 install-live)`; `make check` **112/112**. M1
`09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd`
exact; M2 v27 `54f44987…` byte-identical (`cmp`).

**Carried legs.** Phase `0x14` producer and its slot wire (the seam is gated
on `fn != NULL`); the phase-`0x13` `0x651F0(0xD)` sink; the native side-first
walk order **FU340-L2** (owner `fifa96_match_run_write_phase`); row `0x03`
wiring FU177-L1 (PCCD.PDB clamp bounds), the counter one-pass lead
**FU341-L1**, the `reset_lane` begin seed **FU341-L3**, and the branch-B
`timer79` stand-in;
FU-71 announce table / `0x71C94` tracked tails / **FU303-L12** no-slot
`fifa96_camera_event_dir`; phase-6 release writers (R51-L1/L2) and the
`0x8948D` stale-local read (the remaining FU166-L6 residuals, not L6 as a
whole); phase-52 act-9 stage-0 camera/placement sinks (FU181-L5); the six
un-dispatchable action rows and dead phase rows from R9.

> Superseded by the Phase 53 close-out register (2026-11-05): the carried
> "phase-52 act-9 stage-0 camera/placement sinks (FU181-L5)" entry is closed
> for the stage-0 scope; only the act-9 stage-1/2 gates remain FU170-L1.

## Phase 52 — the control-selection switch (`0x7D1D4`, FU-75 L4.5, 2026-11-05)

FU-75 L4.5 is closed. The native input-row handler `FUN_0007D1D4`
(`0x7D1D4..0x7D35E`, RET at `0x7D35E`) is ported as the pure loader helper
`fifa96_outfield_select_step` (Task 1: self return, the camera-keyed lane
gate, the anchor triple with the slot-byte/lead fallback, the NSEARCH A/B
arms through `fifa96_action_duel_search`, the `[0x1587AC]` reject and the
switch out; six `test_outfield` fixtures) and bound to the pool by
`fifa96_match_run_select_step` (Task 2). The binder stages the native state
from the record / team / camera / slot cells and applies the result: the
`0x786A0` record-visible swap, the guarded `slot.entity` rebind + the moved
block's `0x786A0` tail (`fifa96_control_slot_merge_reset`) + the `0x78B00`
`+0x24` byte (only when the pre-swap slot was this record's and it held
one), and `[team+0x7CB] = 0` (`timer7cb`) on the switch result. The
released-row `case 0x7D1D4u` returns the handler's 0/1; the `0x7CB03`
direct-arm consumer calls the same binder.

Errata: the tail is `0x7D350 MOV EAX,1` / `0x7D355 ADD ESP,0xC` /
`0x7D35E RET` (not `0x7D355` as the "MOV EAX,1"); the arm-A side-matched
skip is `sext8([[0x1587A8]+0x8D])` used as an NSEARCH **record index**
(the `+0x8A>>24` alias), else -1; `0x78B00`'s only new cell after the
`0x786A0` tail is slot `+0x24`. Residual legs (OL-68/OL-72): the
`[0x1587A8]`/`[0x1587AC]` set-piece producers have no engine cell
(`spawn_id`/`recv_id` stage `NONE`; arm A is dead, the unstaged-pointer
reject edge is reproduced), `[0x157AB0]` has no producer (the direct arm is
latent and its consumer is wired), the `0x786A0` block move remains OL-68
at the other call sites, and slot `+0x24` has no reader.

Tests: `test_engine_match_input::test_control_switch_moves_slot` /
`_self_returns_zero` / `_non_slot_does_not_clear` (the real PASS
press/release frame, the handler return and the slot-clear negative),
`test_outfield::test_input_row_scan` (the latent direct-arm pin) and the
`test_control` `byte24` offset assert. Mutation-RED: stubbing the
`0x7D1D4` case, dropping the `has_slot` term of the clear guard, and
dropping `0x7D33B` each fail the engine fixture.

M1 `09b726b7…` and M2 v27 `54f44987…` byte-identical (`cmp`); census
unchanged: **78/78** bodies, **58/80 natural**, **64/80 install-live**,
**6 blocked**, action **38/45**, phase **26/35**; `make check`
**112/112**.

Residual (fix round 1): **FU75-L4.5a — the tail-ordering seam.** The
outfield seam snapshots `s.chase.unbound` from `e->has_slot` before the
released-row scan, while native `0x7CD03` reads `[rec+0x20]` after
`0x7D1D4`, so a switch frame can defer the code-8 chase install by one
frame for the record that gave up the slot. Owner: the `fifa96_match_run`
outfield seam. The same ordering applies to the latent `0x7CB03`
direct-arm consumer (it runs after `fifa96_outfield_input_row` computes
`out.forced`/`out.chase`/`out.no_edge`; native runs the switch before the
`0x7CC82` tail). The stale §11 "chase arm's `[rec+0x20] == 0` gate keeps
it unreachable" sentence is quoted as superseded in FU-75 §13.

## Phase 53 — the act-9 stage-0 sinks (FU181-L5, FU-342, 2026-11-05)

The act-9 stage-0 body `0x88F81..0x88FE7` runs its presentation/placement
sinks after the `0x88F9A` gate clear: the camera reset `0x88FA0`
(`0x700F4(0,0,0)`), the match/camera state reset `0x88FA5` (`0x73E28`), the
phase-0x15 write `0x88FB8` (`0x740A0`, landed phase 41), the placement commit
`0x88FBD` (`0x73E08`) and the presentation bind `0x88FD1` (`0x4C324`). This
phase lands them in two measured re-pins. R7 is the fresh read
(`/tmp/opencode/recon51/R7.md`); the FU-181 phase-53 erratum quotes the
corrected stage-0 order and the two `0x700F4` omissions.

**T1 — camera reset + present bind (v28, this task).** `fifa96_camera_init`
now stages the native pan constants `pan_decay = 10`, `pan_rate_hi = 16`,
`pan_rate_lo = 24` (`0x7011C/0x7013D/0x70146`, the image-default `0x1104AB`
row) instead of zeros; stage 0 calls it after the gate clear and before the
phase write (`0x88FA0`). The hardcoded image-default row is the **FU326-L4**
residual (the `[0x14C2FE]`/`[0x14C2FA]` table indexing is still unmodelled on
both the init and the `fifa96_camera_event_set` paths). New `mr->present_side`
(native dword `[0x1074A4]`)
and `fifa96_match_run_present_bind` port `0x4C324`: mode `[0x157AC2] >= 4`
with a live `[0x1587D4]` record -> the record team's `+0x826` side through
`fifa96_action_duel_bind` + `FUN_00036200(0)`; else `[0x1587E3]>>24`
sign-extended; called last in stage 0. The bind cell has no engine reader. The
`0x700F4` sound (`0x92820(0,0x24)`), the `0x70074`/`0x6D870` place recompute
and the `0x4C324` `FUN_00053DC4` recorder latch stay named legs (no fake
producer). Fixtures: the stage-0 sinks/final cells in
`test_engine_match_act::test_act9_stage0_camera_and_present_bind` (gate clear
-> camera zero + tracked NONE + pan constants -> phase 0x15 -> present bind),
the `fifa96_camera_init` pan/tracked cells in `test_camera`, and both
present-side arms (mode+pointer vs the sign-extended fallback, plus the
mode<4 and no-pointer fallbacks). M2 re-pins **v28**:
`bd6ac7a796b9020ef9196495e66c2e77947413b4467a5fb6d2d62e4231702995`
(295087 bytes, 6522 lines; first differing line 422 = `frame=422`,
`state=21/1-0`; 6101 lines differ, all hash-only; state set/counts unchanged).
M1 `09b726b7…` exact. Census unchanged: **78/78 bodies, 58/80 natural,
64/80 install-live, 6 blocked, action 38/45, phase 26/35**; `make check`
112/112.

**T2 — `0x73E28` + non-seeding `0x73E08` (v29).** Pending in this phase
(reset cells, the non-seeding placement entry, the act-tick ordering leg, and
the measured re-pin); the register is extended when it lands.

**T2 landed — the match reset + non-seeding placement (v29, this task).** The
pending note above is superseded. `fifa96_match_run_act9_reset` ports
`FUN_00073E28` cell-for-cell (phase/prev/`global_157a4c`, the `0x157A60/5C`
clock cells, the `0x157A73 = 0x15774C` focus comment, goal record, controlled
actor, `frame_delta`, `0x157AAB = 0xFF`, the `0x157A6D` 0xB0/0xA0 height
anchor, `0x157AB0`, `0x157A6F`, the `0x7F144` possession block and the
`0x7A028` pair block with flags 0x20/code 2); new run cells
`clock_saved_157a60`, `deepest_keeper_157aa7`, `flag_157aab`,
`camera_height_157a6d`, `flag_157ab0`, `camera_override_157a6f` hold the
otherwise-unconsumed cells. `fifa96_match_entities_restart_place` ports the
non-seeding `FUN_00073E08` (ball := staged `pool->ball.target_*` x/z, y = 0,
velocity/heading/metric cleared; the shared `0x8CF60` commit/face/anim/row43/
frame-0 loop; no `0x79F3C` ring place at phase 0x15). Stage 0 now runs gate
clear -> `0x700F4` -> `0x73E28` -> `0x740A0` -> `0x73E08` -> `0x4C324` ->
timer 0. The act-tick ordering divergence stays a numbered leg (native
`0x8AF38` before `0x8D8EC`; the engine tick is after the entity chain). M2
re-pins **v29**: `7c3874f794c4a9203311225b68beceeb2bd39943bd11025ea00a3b546e69469f`
(295089 bytes, 6522 lines; first differing line 9; 6514 lines differ — 6501
hash-only + 13 hash+state; state set/counts 0x304, 1x950, 2x2194, 4x303,
12x3, 19x2640, 20x20, 21x103; half markers m 845/849, ET anchors unchanged).
Restart pins: `gate-phase15` 96 -> 100, `phase4-phase15` 100 -> 103,
`restart-phase4` 303; act timer 2 -> 0. The final dispatch mask loses row
0x06 (wired, dormant under the v29 placement). M1 `09b726b7…` exact. Census
unchanged: **78/78 bodies, 58/80 natural, 64/80 install-live, 6 blocked,
action 38/45, phase 26/35**; `make check` green.

## Phase 53 close-out register (FU-342, 2026-11-05)

Final register for the act-9 stage-0 wave. The T1/T2 sections above stand; this
is the consolidated outcome, the two measured re-pins, the census and the
carried legs. The evidence record is
`port/spec/FU-342_act9_stage0_sinks.md`.

**T1 — camera reset + present bind (FU-342, `c238316`).**
`fifa96_camera_init` now stages the native pan constants `pan_decay = 10`,
`pan_rate_hi = 16`, `pan_rate_lo = 24` (`0x7011C/0x7013D/0x70146`) instead of
zeros, and act-9 stage 0 calls it at `0x88FA0`. New `mr->present_side`
(`[0x1074A4]`) and `fifa96_match_run_present_bind` port `0x4C324` (the mode
`[0x157AC2] >= 4` + live `[0x1587D4]` arm through `fifa96_action_duel_bind`
and `FUN_00036200(0)`, else the sign-extended `[0x1587E3]>>24` fallback),
called last in stage 0. M2 re-pins **v28**
`bd6ac7a796b9020ef9196495e66c2e77947413b4467a5fb6d2d62e4231702995`
(295087 B, 6522 lines; first differing line 422, 6101 lines differ, all
hash-only; state set/counts unchanged). M1 `09b726b7…` exact.

**T2 — `0x73E28` + non-seeding `0x73E08` (FU-342, `5f44a94`).**
`fifa96_match_run_act9_reset` ports `FUN_00073E28` cell-for-cell (the phase/
prev/`global_157a4c` bytes, the `0x157A60/5C` Q8 cells, the `0x157A73`
focus comment, goal/controlled NONE, `0x157A6C/6A/64`, `0x157AAB = 0xFF`, the
`0x157A6D` `0xB0`/`0xA0` anchor, `0x157AB0`, `0x157A6F`, the full
`0x7F144` `0x158724..0x15872F` possession block and the `0x7A028` pair block
with flags `0x20`/code 2). `fifa96_match_entities_restart_place` ports the
non-seeding `FUN_00073E08` (ball := the staged `pool->ball.target_*` x/z, y = 0,
the ordinary-branch speed/velocity words cleared, row 3; the shared `0x8CF60`
commit/face/anim/row43/frame-0 loop; no `0x79F3C` ring place at phase 0x15).
Stage 0 runs gate clear -> `0x700F4` -> `0x73E28` -> `0x740A0` -> `0x73E08` ->
`0x4C324` -> timer 0. M2 re-pins **v29**
`7c3874f794c4a9203311225b68beceeb2bd39943bd11025ea00a3b546e69469f`
(295089 B, 6522 lines; first differing line 9; 6514 lines differ — 6501
hash-only + 13 hash+state; state set/counts 0x304, 1x950, 2x2194, 4x303, 12x3,
19x2640, 20x20, 21x103; +2 bytes from ten state-tag width changes — six +1,
four −1 — plus three equal-width changes, with no line change; half markers
m 845/849, ET anchors unchanged). Restart pins: `gate-phase15` 96 -> 100,
`phase4-phase15` 100 -> 103, `restart-phase4` 303; act timer 2 -> 0. The final
dispatch mask loses row 0x06 (wired, dormant under the v29 placement). M1
`09b726b7…` exact.

**T3 — close-out (FU-342, this commit).** Docs/comments only, no behavior
change: the `restart_place` field comments are un-crossed (`0x8C2A6` speed
`[0x158848]`, `0x8C2AD` vel_z `[0x15884C]`, `0x8C2B4` vel_x `[0x15884A]`, the
ordinary branch), the `0x7F144` range is `0x158724..0x15872F`, the height
anchor is stated as mode + `0x70` (`0x702F8` returns EAX and preserves EDX),
the `0x8CF60` `0x6D920`/`[rec+0x1C]` residual and the `skip_9a` caveat are the
named FU342-L2 leg, the M2 v29 header states the +2-byte cause, and the
scripted-pan witness print reports the measured single-`0x90` shape
(`second_stop_step == -1`). M1 `09b726b7…` and M2 v29 `7c3874f7…` are
byte-identical; census unchanged.

**Census (final).** **78/78** bodies, **58/80** natural, **64/80**
install-live, **6** blocked, action **38/45**, phase **26/35**.

**Carried legs.** FU342-L1 — the act-tick ordering (native `0x8AF38` before
`0x8D8EC`; the engine tick follows the entity chain; the begin-order cause is
FU170-L10). FU342-L2 — the `0x8CF60` per-record `0x6D920`/`CALL [rec+0x1C]`
residual; the phase-write walk covers non-`skip_9a` records, so a `skip_9a`
record would commit a stale target. FU342-L3 — the `0x8C24C` `[0x14C32A] != 0`
session arm (`0x1C80`/`0x2C40`). FU342-L4 — the `0x700F4` `0x92820(0,0x24)`
sound and `0x70074`/`0x6D870` place recompute. FU342-L5 — `0x53DC4`.
FU342-L6 — `0x36208`. FU342-L7 — the FU326-L4 `0x1104AB` init row. The
`0x8D040[0x15] = 0x8D77B` walk arm stays the FU170-L2 residual; the act-9
stage-1/2 gates stay FU170-L1 (the original FU-181 §7 item 5), not the
phase-53 stage-0 scope. `make check` green (112/112).

## Phase 54 — P1: the visible match (FU-343, 2026-11-05)

P1 of the playable SDL wave (`docs/superpowers/specs/2026-11-05-fifa96-playable-sdl-design.md`,
plan Task 1; recon `/tmp/opencode/play/A1-field-renderer.md`,
`A2-formation-camera.md`, `A7-sprites.md`). Every new path is gated by
`mr->live_start` (default 0), so the M1 `09b726b7…` and M2 v29 `7c3874f7…`
goldens are byte-identical with **no re-pin**; census unchanged (**78/78**
bodies, **58/80** natural, **64/80** install-live, **6** blocked, action
**38/45**, phase **26/35**); `make check` green (112/112). **[M]** = measured
first-hand this phase (Ghidra image reads, decoded retail containers, live
frames); **[D]** = derived stand-in.

**Live-start gate.** New `fifa96_match_run.live_start` + setter
(`fifa96_match_run_set_live_start`). The windowed bridge sets it only when
`eng->surface != NULL && !eng->cfg.headless`; headless runs, the M2 tape and
every direct-begin fixture leave it 0. In `begin` the live path skips the
`fifa96_match_run_period_setup(mr,1)` that arms act `0x0A` (its phase-`0x20`
body installs action `0x24` on all 22 records, collapsing the kickoff
placement to the `x = 0x7E0` line, A2 §1) and keeps the `352ko` kickoff
placement commit. It then writes the live phase 2 (class-1 play), seeds one
camera event (`fifa96_camera_event_set(…, 0x50, 0)` +
`fifa96_camera_event_dir`) and binds `camera.tracked` to the controlled
record: `entities.controlled` when the native `[0x157A83]` actor is live, else
the FU-70 slot entity (the native actor cell is cleared by the act-9 reset;
the slot is the human-controlled record). The seeded event is **[D]** — the
native has no kickoff event (A2 §2); it is the stand-in for the unported FU-310
camera place.

**Camera follow + render eye.** In the frame body after the entity drain, a
live run with no active native pan (`event_param == 0 && speed == 0`) sets the
FU-71 focus triple to the ball (`pos_x = ball.x`, `pos_y = 0`,
`pos_z = ball.z`) and keeps `tracked` on the controlled record. The render eye
is separate: `fifa96_match_run_eye` returns
`focus + (-1920,160,30)` for live (camera object 0 image `0x107508`;
**[M]** `read_memory 0x107510` = `80 f8 ff ff a0 00 00 00 1e 00 00 00`; A1 §4,
A2 §2) and the focus triple for non-live. The same eye feeds the sprite
projection and the pitch (no split-brain). **[D]** the native rig smoothing and
the live orientation director (`FUN_0004E834`, FU-157 R2-L1) are unported; the
focus is the ball, not a smoothed native rig.

**Live lens.** The fixed FU-88 projection has no FOV/zoom term (A1/A7), so the
320-wide reciprocal lens crops most of the kickoff formation at the native
near-goal standoff. The live path halves the **horizontal** focal dimension
only (`lens_w = 320/2`, `lens_h = 240`) before building `recip_x`; the vertical
lens and the native sprite scale are unchanged, so both teams stay readable
across the width. **[D]** stand-in for the unported native lens director; the
non-live projection keeps the pinned dimensions.

**Pitch v1.** New `port/src/fifa96_engine/fifa96_pitch.c/.h`. Staging
(`fifa96_match_run_stage_pitch`, called by the bridge only on the live path):
`/ART/GAMEFLD1.PVI` → `FplainH.fsh`/`FplainV.fsh` (64x64 SHPI, 4096 B) and
`/ART/GAMESTD1.PVI` → `STACcovr.fsh` (64x64 crowd frame), decoded through the
engine asset table into an owned 3-tile arena; drawing is skipped when the
stage fails. **Measured correction:** the retail field binding is GAMEFLD3
(FU-2), but GAMEFLD3's entries are `FHstripH/V` + `3dwpts.dat` (A1 §1), not
Fplain — the plain tile lives in GAMEFLD1. The stage tries GAMEFLD1 first and
falls back to GAMEFLD3's FHstrip names. Draw order: sky rows (PALsys `0x3C`
blue-grey), 56-row `STACcovr` crowd band (A1 §4 **[D]** approximation), the
inverse-projected floor (one ray/pixel through the transpose of
`fifa96_projection_matrix`, one divide per pixel, real 64x64 texels, H/V
selected by the `FUN_0004C9D4` yaw ranges), then the procedural white markings
(goal lines `x = ±0x7E0 = 2016`, touchlines `z = ±0xC1E = 3102`, halfway line,
centre circle `r = 0x3C0`). Clipped to the render window/surface; drawn after
`fifa96_surface_clear` and before the entity sort. The white marking index is
the unused `0xFE` slot (no PLAYART frame uses > 229); no PALsys/PALteam entry is
white.

**Palette / kit.** The sprite body indices are magenta in the derived PALsys
frame-2 base (A7 §4). **Measured:** PALsys **frame 0** carries the full match
palette — grass indices 1..29 are green — while frame 2 is magenta at those
indices; using PALteam frame 0 as the whole base turns the Fplain grass cyan
(the A1 prototype palette re-run with PALteam: cyan/striped output). The live
stage therefore builds the base from PALsys frame 0 (the derived stand-in for
the native front-end base `0x14B200`, not statically derivable), applies the
FU-98 kit remap and the frame-2 `[0xF0,+0x54)` / `[0x186,+0x4E)` appends, then
overlays PALteam frame 0 over the sprite body bands 30..229 (the Fplain tile
only uses 1..29, so the pitch stays green). Team 1's shirt/shorts bands move
into the free 230..245 slots: `195..203 → 230..238`, `208..214 → 239..245`,
with PALteam frame 2's colours (A7 §4); `remap_team[0]` is the identity,
`remap_team[1]` is the band map. The sprite draw selects them only when
`live_start`; the ball and every HUD/overlay pass keep the current `remap`, so
the non-live tape is untouched. **[D]** the per-kit PALteam frame selection is
still the A7 leg; the native translation pool (leg 11) remains unported.

**Tests.** `test_engine_match_render` gains: the `live_start` default/setter and
the eye offset (`focus + (-1920,160,30)`); an ISO-gated live begin + 1 frame
probe pinning phase 2, no collapsed `x = 0x7E0` line, >10 distinct record
positions, the tracked bind, the pitch's >30% non-background frame, the
identity/team-1 remap bands and distinct shirt/shorts colours, and the white
marking slot. Existing fixtures keep their `live_start = 0` path unchanged.

**Legs (P1).** FU343-L1 the native floor caster `FUN_0005F684`
(`0x5F684..0x5FEA9`) is not line-by-line ported (v1 is the inverse-projection
stand-in). FU343-L2 exact world-units-per-texel (`FUN_0005F684`'s `>>12`
stepping; v1 uses one unit per texel). FU343-L3 the `3dwpts.dat` stand-point
projection / `FUN_000610C4` tiers and the `FUN_0005B448`/`0x62620/0x62588` band
spans (v1 is a screen-space crowd band). FU343-L4 the field/stadium index
producers (`[0x1492D0]/[0x1492D4]` have no absolute writer; the stage fixes
GAMEFLD1/GAMEFLD3 + GAMESTD1). FU343-L5 the native camera rig smoothing / live
orientation director (`FUN_0004E834`, FU-157 R2-L1). FU343-L6 the live lens
(horizontal half-focal stand-in). FU343-L7 the exact native palette base
(`0x14B200` is the front-end palette; PALsys frame 0 is the derived live base)
and the per-kit PALteam frame selection. FU343-L8 the pre-match timeline
(the native situation-0/act-0xA/phase-0x20 order is skipped on the live path;
A2 §4 P6 / FU170-L10). The referee entity stays absent (A7 §2: no live pool
slot/bank).

## P2 register — the usable front-end (2026-11-05)

**Status.** A readable, keyboard-only front-end now starts a friendly match
with two chosen teams. M1 `09b726b7…` is byte-identical, M2 v29 `7c3874f7…` is
unchanged, the census is unchanged (**78/78** bodies, **58/80** natural,
**64/80** install-live, **6** blocked, action **38/45**, phase **26/35**), and
`make check` is green (114/114: two new tests plus the MOD2 case). No dispatch
row is wired.

**LANG000.POG.** New `fifa96_lang.c/.h` decodes the RefPack record
(`10 FB 00 27 54`, declared length 10068) with `fifa96_record_decode`. The
decoded image is `"LANGO"` + `02 00 00` + `base = 0x94C` at +8 + the u32
relative offset table at +0x10 (`count = (0x94C-0x10)/4 = 591`).
`fifa96_lang_string(id)` is the `FUN_0001771C(id)` lookup; every offset is
bounds/NUL-validated at init. Verified ids: Game Select `0x84`, Team Select
`0x13D`, Friendly `0x81`, Options `0xD8`, Quit to DOS `0xF3`, Home `0x98`,
Visitor `0x14A`, Brazil `0x23`, Italy `0xA6`; the 44 national-team ids are the
A3 §2.3 list.

**HELV12.POG.** The same module decodes the RefPack record
(`10 FB 00 13 10`, 4880 B) and parses the FNTI with the ported
`fifa96_font_parse`: first 32 / last 127 / 96 glyphs, default 12x13, 4 bpp,
bitmap 256x31. `fifa96_menu_art` draws labels with the real font (white body,
black +1 shadow; the selected row inverts to black on light yellow).

**Menu art.** `fifa96_menu_art_init_native` decodes `/FEGFX/MOD2.POG` frame 0
through `fifa96_record_decode` -> `fifa96_sprite_bank_parse` -> entry 0 ->
`fifa96_sprite_frame_parse` + the type-`0x22` chunk palette, and scales the
640x480 image to the 320x240 surface. The legacy `fifa96_menu_art_init`
(OPTIONS.INV only) and the compiled-in fallback remain unchanged.

**Game Select (A3 §1.1).** Nine native items in the `FUN_0001FC50` order:
right column Friendly (`0x81`/code 4), Leagues (`0x1BF`/5), Tournaments
(`0x1BB`/6), Playoffs (`0x1BD`/7), Practise (`0xEA`/8); left column Load Game
(`0xB3`/9), Options (`0xD8`/1), Modem Setup (`0xC2`/2), Quit to DOS
(`0xF3`/3). Friendly/Options/Quit are live; the other six are drawn
dim/disabled (P2-L1). Up/Down walk the active column, Left/Right switch
columns; Return activates and the QUIT key exits the engine.

**Team Select (A3 §2.3).** Home/Visitor columns over the 44 LANG national
names, 7-row scrolling windows, native entry defaults `team[0] = 0` (Brazil)
and `team[1] = 4` (Germany). Up/Down change the active column's team with
wrap, Left/Right switch columns, Return starts, X/Backspace backs out. Names
resolve from LANG000; fallback literals exist only for the synthetic tests.

**Handoff (A3 §3.2).** `fifa96_frontend_run` carries `team_id[2]`,
`team_name[2][16]`, `team_code[2][4]` and `formation[2]`. On Team Select
Return it sets `match_start`; after `fifa96_match_run_begin` the bridge calls
`fifa96_match_run_set_formation(side, formation[side])` and writes the
3-letter codes into `mr->render.hud_name[side]` (windowed BRA/ITA measured).
The simulation and the match renderer are untouched.

**Gate.** The engine enables the native menu only for the interactive
(non-headless) boot with both LANG000/HELV12 decoded. The deterministic
headless M1/M2 tapes keep the legacy FU-135 render and the hash-pinned
fallback, which is what keeps the ISO-present M1 golden byte-identical; this
is the derived stand-in P2-L8.

**Tests.** New `test_engine_lang` (synthetic LANGO lookup/malformed tables +
FNTI parse/blit; ISO-gated retail decode) and `test_engine_menu_model` (item
table/nav, the 44-team table, a full keyboard fixture through Team Select ->
bridge with BRA/ITA in `render.hud_name`). `test_engine_menu_art` gains an
ISO-gated MOD2 frame-0 case. Full suite 114/114.

**Legs.** P2-L1 the six other native Game Select screens (Leagues,
Tournaments, Playoffs, Practise, Load Game, Modem Setup) are disabled labels,
not ported screens (A3-L2/A3-L3). P2-L2 Options is a labeled stand-in (the
FU-66 options driver is not re-wired here). P2-L3 MOD2 frames 1–19 decode as
noise (A3-L1); only frame 0 is used and titles are drawn with HELV12. P2-L4
PCCD/PCDB is not decoded: `team_id` is the LANG id and the 3-letter HUD codes
are derived stand-ins; `fifa96_match_run` still has no roster/kit consumer
(A3-L3/A3-L4). P2-L5 `ftmsel.inv`/`ptmsel.inv` are absent from the CD
(A3-L7), so Team Select is drawn from MOD2 + HELV12. P2-L6 the native
two-column navigation/selection semantics are not fully decomposed; the port's
column model is derived from the widget order. P2-L7 `FUN_00017748` is only
the ASCII identity (A3-L5); accented language strings are not remapped. P2-L8
the native menu is disabled on the headless engine to keep M1/M2 deterministic.
P2-L9 no formation chooser: formation ids stay 0.

## P3 match audio register (2026-11-05)

The match is audible through the ported DSP stack for the first time. The
chain is `mr->ref_whistle` (staged by the eight `0x974DC(0x1E)` sinks) ->
`fifa96_match_audio_step` cue edge -> `fifa96_sfx_play_id` ->
`fifa96_mixer_render` -> `plat->audio_submit` -> `SDL_PutAudioStreamData`.
The crowd bed is armed directly, without the native ambience state machine:
`/SOUND/CHN_GEN1.BNK` id **55** (`CHN_GEN1`) through
`fifa96_sfx_arm_alloc` at caller volume **0x60**, master 0x7F (mixer gain 0x60;
the 16 kHz `DELTA_MONO` EACS loop 10753 + 97673 = 6.1 s loops indefinitely).
The whistle is `/SOUND/SFX_GAME.BNK` id **0x1E = 30** at caller volume 0x7F.
The cue edge is `mr->whistle_seq`, the monotone count the run increments at
every staged `0x974DC(0x1E)` write (the eight sites: `match_run_ref_apply`,
`match_run_ref_step_restart`, the act-0x1D wait arm and
`fifa96_match_action_phase_21/22`), so a clear + re-set inside one frame body
is audible; one cue is played per pending write (two writes in one step fire
two cues). `mr->ref_whistle` is the staged level, cleared by
`fifa96_match_run_referee_step` before the frame writers, and remains the
compatibility fallback edge for fixtures that stage the byte directly (a held
slot does not repeat). A score increase fires the labelled goal stand-in id
**15** (the longest crowd-like `SFX_GAME` clip, 6.78 s). The module renders
`22050 * ticks / 100` frames per engine step (220/221 at 10 ms) in 512-frame
chunks.

Wiring: `fifa96_match_audio_start` runs on the first MATCH step after the
bridge begins the run; banks are read through `fifa96_cache_get` (the cache is
created on demand when boot had no intro), and a missing ISO/bank is a soft
failure (silent, playable). The step runs after `fifa96_match_run_step`, and
`fifa96_match_audio_stop` runs when `e->match` clears, on QUIT, and in
`fifa96_engine_destroy`. **Headless (`cfg.headless`) never starts or steps the
module**, so the null tapes submit no PCM: `test_engine_m1` and
`test_engine_m2` now assert `audio_frames == 0` and `audio_hash == 0` for the
full tapes, and M1 `09b726b7…` / M2 v29 `7c3874f7…` are byte-identical.
`test_engine_audio` covers the no-bank silent contract, the retail crowd arm
(loop, gain 0x60), 1 s of non-silent stereo PCM at exactly 22050 frames, the
whistle level edge (one cue per change, none while held, nonzero gain), the
durable `whistle_seq` edge (a clear+re-set inside one step cues; two pending
writes cue twice; the referee clear does not increment), and the goal edge.
Windowed verification (2026-11-05, `DISPLAY=:1`, ISO mounted): the SDL3
device opened at 22050 Hz S16 stereo (`SDL_OpenAudioDevice` dev 25) and bound;
the real game submitted 248 PCM chunks / 377,496 bytes / 94,374 frames
(≈4.3 s) over a live match, with 188,688 nonzero samples (peak 24576) and
exit rc 0 (instrumented with a verification-only `LD_PRELOAD` shim around
`SDL_OpenAudioDevice`/`SDL_BindAudioStream`/`SDL_PutAudioStreamData`; no test
or game code change).

Open legs: **A6-L1** the match effect id map `{1,2,4,8,0xB,0xC,0xE}` and the
kick cue (no port producer stages one; FU-113 §7 leg 1); **A6-L2** the native
goal audio is the `FUN_00066724` commentary chain (OL-89) — id 15 is a
stand-in; **A6-L3** team/event CHN selection (`FUN_000652f0`), the 20..59 s
`FUN_00091F60`/`FUN_000920C0` ambience machine and the `0x37` volume ramp;
**A6-L4** CRD music (raw 8-bit mono tracks, no `fifa96_eacs` format);
**A6-L5** commentary (`PHR_*` phrase banks, `ref_speech` has no consumer);
**A6-L6** the channel/master volume model `[0x55D00..]`/`[0x15FD6]` and
settings sliders 0x11/0x12/0x13 (v1 fixed gains); **A6-L8** the intro video
companion audio (`fifa96_intro.c` discards the `1SNh`/`1SNd` chunks, FU-35 §1)
is out of this slice; **A6-L9** the unported
`0x974DC(0x1E)` sinks (row-01/0x10/0x11/0x12/0x13 and the scanner) do not
move `ref_whistle`/`whistle_seq`. The SDL3 queue still has no backpressure and
a failed device open remains silent (`platform_sdl3.c`, A6-L7; not changed
here).

## Phase 55 — P4: HUD, pause and full-time result (FU-343, 2026-11-05)

P4 closes the playable loop (`docs/superpowers/specs/2026-11-05-fifa96-playable-sdl-design.md`,
`docs/superpowers/plans/2026-11-05-fifa96-playable-sdl.md` Task 4) on top of
the merged P1 live match and P2 front-end. A4 (`/tmp/opencode/play/A4-hud.md`)
and A5 (`/tmp/opencode/play/A5-lifecycle.md`) are the recon sources.

**HUD (A4 §1.3/§4.1/§4.2, no re-pin).** The geometry stays the native 60x41
Frames.fsh frame 13 at `(x0+2, y1-41-2)` = `(2,197)` with the frame-4 height
anchor, the P2 `render.hud_name[2]` rows, and the live score/period/clock
cells (A4 §3). The live draw now applies the native bar pixel rule
`0x00..0xFD` opaque, `0xFE` = shade the **destination**, `0xFF` transparent
(`FUN_000D11CB` 0xD12DB.. / `FUN_000CEB9A` 0xCEBBC..), and the native two-pass
text pair outline `0x00` / body = `hud_ink`. `hud_ink` is the palette index
maximising `min(r,g,b)` (the A4 near-white scan). On the final live palette
the kit stage forces the pitch-marking slot `0xFE` to pure white, so the scan
selects **`0xFE`** and the live HUD text is that white; 30 = (236,232,236) is
only the pre-stage/fallback value on the measured staging palette.
`hud_shade[256]` is the nearest palette entry to half the destination RGB
(`shade[0] = 0`), the documented stand-in for the unported palette-pool table
`[0x14BF2C]`/`0x114274`. Both are rebuilt from the final live palette by
`fifa96_match_run_stage`/`_stage_pitch`.
`fifa96_match_run_hud_colors_from_palette` is unit-pinned. The entire change is
gated on `mr->live_start` (default 0 for the M2 tape and every direct-begin
fixture); non-live frames keep the legacy FU-148 operands and 0-transparent
bar, so **M1 `09b726b7…` and M2 v29 `7c3874f7…` are byte-identical** — no
re-pin was needed or taken.

**Pause (live-only stand-in over the faithful pace hold).** ESC in a live
MATCH toggles `fifa96_match_run_set_paused`; the freeze is the native pace hold
(`fifa96_match_pace_hold`: no granted frames, so the frame body and the FU-62
clock stop — the native mode-7 drain/pace behaviour, A5 §3.1). The panel is a
labelled stand-in for the unported mode-7 screen 0x3DE: a dark plate with
"PAUSED", "ENTER: RESUME", "X: QUIT TO MENU" drawn with the staged HUD font.
RETURN resumes; X/BACKSPACE stages `fifa96_match_lifecycle_request_exit`; a
second ESC also resumes. The SDL backend re-presents held keys, so the engine
latches the ESC 0->1 edge (`match_esc_held`); without it a held key would flap.
Non-live runs return `-FIFA96_ERR_UNSUPPORTED` from the PAUSE/RESULT API and
keep ESC = process quit.

**Full time and return (live-only POST hold).** On a live run
`fifa96_match_run_step` resolves OVER -> POST and holds the native screen 3 for
`FIFA96_MATCH_RUN_POST_DWELL_TICKS` (500 engine steps = 5 s at the 10 ms
engine tick) with the pace held; the render draws the FULL TIME panel (staged
names, final score, `PERIOD <state.period+1>` mirroring the HUD cell, "ENTER:
MENU") over the frozen pitch. ENTER/ESC (`fifa96_match_run_result_continue`)
skips the dwell. The dwell/panel are stand-ins for the unported native screen-3
timer and the period-overlay armer `FUN_00054104(0)` (A5-L1/A5-L2). The
frame-loop `mark_over` tail is now gated on `lc.screen == ACTIVE`, so a held
POST cannot be re-marked every tick; non-live runs keep the one-step
OVER->POST->EXIT->end compression. When `run_end` clears `e->match`, the engine
calls `fifa96_frontend_run_reset`, which re-seeds the library/menu model,
queue, `match_start`, team handoff and `frames` while preserving the decoded
LANG/font and the native-menu flag; the next step draws Game Select and a
second match starts normally (measured twice).

**Verification (2026-11-05).** `make check` 115/115. M1
`./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt` exact
(`09b726b7…`); M2 v29 exact (`7c3874f7…`). Windowed run on `DISPLAY=:1` with
the retail ISO and a verification-only harness (2 s halves, audio step
disabled in the harness only): HUD `BRA`/`GER` white-on-dark at `(2,197)`;
pause and resume; X from pause returns to Game Select; full time holds the POST
panel (score 0-0, period 3) for 500 steps and returns to Game Select; a second
match starts live. Screenshots: `/tmp/opencode/play/p4-pause.png`,
`p4-resumed.png`, `p4-result.png`, `p4-menu-return.png`, `p4-second-match.png`.

**Legs.** **P4-L1** the native pause menu (mode 7, screen 0x3DE, event code 1;
there is no static ESC binding) is unported — ESC/pause is the engine stand-in
listed above. **P4-L2** the native POST screen-3 timer and the period-overlay
armer `FUN_00054104(0)`/`FUN_000550E4` case 0 (the "FULL TIME" caption) are
unported; the 500-step dwell and caption are stand-ins. **P4-L3** the live
`0xFE` shade table is the palette nearest-half-luma derivation, not the native
`0x114274` palette-pool contents (FU-148 leg 10/11). **P4-L4** the live text
body is the palette near-white scan, not the native body byte 0xFF (green in
the staged PALsys palette); the exact native text ramp stays OL-T11-71.
**P4-L5** the crowd band's dark horizontal separation is source art, not a
mapping bug: all three 64x64 `STACcovr` frames (indices 1/4/7) carry a
two-tier stand with `uniq=1..2` rows at y=32/40, and the 56-row band maps 64
source rows linearly; removing it would need an art/venue change, so it is
recorded rather than patched. **P4-L6** the full-time period label mirrors the
HUD cell (`state.period+1`, i.e. 3 after two regulation halves); a
"completed halves" reading would be a separate display decision.

## Phase 56 — P5: play-test polish (FU-343, 2026-11-05)

The first full play-through of the merged P1–P4 build exposed one live-only
display defect: the FULL TIME panel mirrored the HUD's `state.period + 1`
cell, so the post sentinel rendered `PERIOD 3` at the end of a two-half match,
and the HUD behind the panel repeated the same `3`. Fix (live-gated, M1
`09b726b7…` / M2 v29 `7c3874f7…` untouched): `match_run_draw_hud` returns
early on the live OVER/POST screens (the native POST is a separate screen
install, A5 §1), and the result panel no longer prints a period line — it
shows `FULL TIME`, the staged `HOME score - score AWAY`, and
`ENTER: MENU`. This closes P4-L6 as a display fix; the native screen-3
timer/armer remains P4-L2. README gains a **Play** section with the exact
menu and match controls (arrows, Z/C/V, Esc pause, the 120-second halves).

## Phase 57 — F1: native front-end held-key edge latch (FU-343, 2026-11-05)

The Phase 8 T3 hold policy above re-presents every physically held key with
`state == 1` on every 10 ms poll. The native Game Select/Team Select input path
queued every `state == 1`, so a normal held Return advanced Game Select ->
Team Select and the next poll activated START_MATCH; held arrows repeated at
100 Hz, and after the post-match reset a held Enter re-activated Friendly while
a held ESC set `quit_requested` and quit the process. The final review of the
playable-SDL wave caught this as Critical 1.

Fix (native path only; the legacy FU-135 path and the M1/M2 tapes are
byte-identical): `struct fifa96_frontend_run` gains `key_down[16]` (raw engine
codes) and `input_armed`. `fifa96_frontend_run_input` edge-latches when
`native_menu` is set — while disarmed, held keys are only recorded and a poll
with nothing down re-arms; once armed, a key queues only on its 0 -> 1
transition and a held key never requeues. QUIT follows the same latch, so a
held ESC is not a quit (a fresh press still is). `run_init` arms the latch;
`run_reset` and `fifa96_frontend_run_enable_native(…, 1)` disarm and clear it;
`fifa96_engine_step` calls the new `fifa96_frontend_run_disarm_input` on both
INTRO -> FRONTEND handoffs (the input skip and the natural end), so the Return
that skipped the intro cannot activate the menu. The match-side `match_esc_held`
edge handling stays as landed.

Tests: `test_engine_menu_model` adds `test_native_held_confirm_latch` (two held
CONFIRM polls enter Team Select once and do not start a match; release + a
fresh press starts) and `test_native_held_input_after_reset` (a held CONFIRM/
ESC is ignored after reset until a neutral poll and a fresh press; a held ESC
does not set `quit_requested`). The existing handoff fixture feeds one neutral
poll and uses press/release taps; `test_engine_frontend` pins the reset latch
state.

**Verification (2026-11-05).** `make check` 115/115. M1
`./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt` exact
(`09b726b7…`); M2 v29 exact (`7c3874f7…`), legacy path unchanged. Windowed
`DISPLAY=:1` with the retail ISO and real XTest key events: the intro skip is a
held Return (300 ms down, up); Game Select is still showing 1.8 s later (no
auto-advance). A 300 ms held Return then enters Team Select once, and it is
still Team Select 1.2 s later (no START_MATCH); teams select and the live match
starts (ITA 0-0 ENG, clock running). Screenshots:
`/tmp/opencode/play/fix-menu-held.png`, `fix-teams-held.png` (plus
`fix-menu-held-late.png`, `fix-teams-held-late.png`, `fix-match-held.png`).
This closes the "front-end held-key repeat cadence" leg under the hold policy
for the native interactive menu; the legacy FU-135 panel path keeps the
original every-state-1 consumption by design (headless/M1-pinned).

## Phase 58 — P5 follow-up: bounded host catch-up (FU-343, 2026-11-05)

The live play-test (80 scripted kicks with the right direction held) exposed
the A6-L7 catch-up spiral: `fifa96_engine_step` fed the whole
`now - last_ns` delta to `fifa96_clock_advance_ns`, whose unbounded
`while (pending >= tick_ns)` loop runs the 100 Hz tick hook — a full match
frame body — once per fired tick. A long host stall (window drag, debugger,
a starved X server, or the WM's own "not responding" dialog) therefore
replayed thousands of match frames in one engine step, pinning a core and
freezing the window. The fix clamps the per-step delta to 10 ticks (100 ms
at the 10 ms engine tick) and drops the excess, as a normal game drops
frames; headless/null tapes advance exactly one 10 ms tick and are
byte-identical (M1 `09b726b7…`, M2 v29 `7c3874f7…` verified by `cmp`).
Regression `test_engine_platform` drives a virtual 1 s step and asserts the
clamp fires exactly 10 ticks. Re-running the same 80-kick live sequence
leaves the window responsive and the match clock advancing (00:23).

## Phase 59 — code-review fixes (FU-343, 2026-11-05)

Two-axis review (`0ee103d..a5a63f2`): the Spec axis found two implementation
defects and the Standards axis found two hard documentation breaches plus
judgement smells. Fixed:

- **Pitch H/V selector.** `FUN_0004C9D4` (A1 §2) returns 0 for yaw inside
  `[0x2000,0x6000]` or `[0xA000,0xE000]` (selecting **V**) and 1 elsewhere
  (**H**); the v1 condition had the two sets swapped. Fplain H/V pixels are
  identical, so only the GAMEFLD3 `FHstrip` fallback showed the pattern
  rotated 90°. `pitch_tile_pick` now selects V inside those bands and H
  outside.
- **FULL TIME period.** The plan asks for the period on the panel; P5 removed
  the line because the HUD's `period + 1` cell read 3 on the post sentinel.
  The panel now prints `PERIOD <state.period>` (2 after regulation, 3/4 in
  extra time) and keeps the HUD suppressed on POST. This is the P5 note's
  correction (append-only erratum; the P5 text above stands).
- **Standards corrections.** `fifa96_pitch.h` and the render-test banner now
  state GAMEFLD1-first / GAMEFLD3-FHstrip-fallback and no default field path;
  the duplicate second `Phase 57` heading is renumbered **Phase 58**; the
  `fifa96_frontend_run_init` single-init contract is documented (use reset to
  reuse; a second init would drop the decoded LANG/font without freeing);
  `frontend_native_commit_teams` is renamed `frontend_native_sync_teams` and
  `team_selected` is documented as "the native menu supplies the current team
  payload".
- **Spec errata** §6 records the GAMEFLD1/PALteam-base/live-lens/P2-roster/
  result-period/optional-goal corrections above.

Deferred judgement smells (no change, recorded): the three local RefPack
decode/name-finder copies are a module-leaf choice; the three key latches
(front-end/menu, engine ESC, match pause) cover different key spaces; the menu
model's native-code/fallback seam is test/evidence-bearing; the
`engine_step`/`stage_live_kit` breadth is architectural and not worth a
late-wave refactor. `make check` 115/115; M1/M2 exact.

## Phase 60 — P6 Task 1: real team identity from PCCD (FU-344, 2026-11-05)

**Measured (retail ISO, read-only).** `/FEDATA/PCCD.POG` is the only ISO path.
The file is a RefPack record (`10 FB 03 0A 61 E4`); the declared output is
`0x030A61` = 199,265 B and the decoded image starts with `PCDB`. First-hand
decode + byte parse this slice:

- Header: `+0x04` 0x14, team block `+0x08` = 0x4B4, player block `+0x0C` =
  0x8A06, string pool `+0x10` = 0x2866A, mode base `+0x14` = 0x0C, mode count
  `+0x18` = 13, mode offsets at `+0x1C`.
- Team block: `+0x4B4` fixed/dword-prefix size **0x49**, `+0x4B8` count **242**,
  offsets at `+0x4BC`. Each record is **0x89 B** = 0x49 fixed fields + 16
  roster dwords; `0x884 + 0xF2*0x89 = 0x8A06` closes the block.
- Team record: `+0x00` PCCD id (Brazil 59, Italy 47), `+0x04/+0x08/+0x0C` pool
  offsets (long/short/medium; Brazil "Brazil"/"Bra", Italy "Italy"/"Italy"),
  `+0x12` formation (Brazil 1, Italy 2), `+0x25/+0x28/+0x2B/+0x2E` four
  shirt/shorts/socks triples, `+0x31` four print colours, `+0x45` player count,
  `+0x49` sixteen roster record indices (Brazil `2047,3531,2751,1034,...`).
- Player block: `+0x8A06` stride **0x24**, `+0x8A0A` count **0xE1F** = 3615;
  records at `+0x8A0E`, ending exactly at the pool. Record `+0x00` player id,
  `+0x05` 24-bit pool offset, `+0x08` byte, `+0x09` position ASCII,
  `+0x0A..+0x1A` 17 skill bytes, `+0x1B` overall (Bebeto 95, Dunga 90).
- Mode 4 is `"World"`: `mode_base+0x1B0` = 0x1BC, 62 slots through 0x2B3
  (59 distinct teams + the Netherlands/Netherlands/Rep. Ireland tails). Brazil
  is slot 6 (record 0x2B), Italy slot 28 (record 0x20).
- Kit colours are PALteam.fsh indices, not RGB: index `idx` selects frame
  `2*(idx/16)` and ramp `idx%16` (`FUN_0004808C`); the ramp is 16 6-bit RGB
  triples at `ramp*0x30`, first 11 real. Measured: Brazil kit 0 `(26,4,7)` =
  frame-2 ramp10 yellow (255,226,20), frame-0 ramp4 blue (68,16,157), frame-0
  ramp7 green (0,194,16); Italy `(3,30,3)` = frame-0 ramp3 blue (16,52,246),
  frame-2 ramp14 white (246,238,255), blue. Brazil/Italy kit 0 does not clash.

**Derived (port construct).** The 44-name Team Select list has no static
44-id table in the image; each LANG000 id is mapped once to its mode-4 slot
(`fifa96_team_data.c`). Measured key rows: Brazil `0x023`→6, Italy `0x0A6`→28,
Argentina `0x013`→1, England `0x06E`→18, Germany `0x088`→21, France `0x07E`→20,
Spain `0x126`→50, Netherlands `0x0C9`→35, Portugal `0x0E7`→41, USA `0x149`→56,
Israel `0x0A4`→27. LANG `0xA2 "Ireland"` maps to Rep. Ireland slot 42
(P6-L2). The 15 unexposed mode-4 teams are listed under the legs. The HUD code
is the record `+0x08` short name uppercased and truncated to 9 (the native
10-byte local; Brazil `BRA`, Italy `ITALY`, England `ENG`); the no-ISO fallback
keeps the menu's 3-letter codes. The team-id→record resolver does not exist
natively; the mode table is the authority (P6-team-identity §1/§4).

**Landing.** New `fifa96_pccd.{c,h}` (pure bytes, no I/O, all offsets/counts
bounds-checked; team/player/mode accessors) and `fifa96_team_data.{c,h}`
(reads `/FEDATA/PCCD.POG` through the asset table, RefPack-decodes into an
owned arena, keeps the 44-entry LANG→mode-4 map, soft-fails without the ISO).
`fifa96_frontend_run` carries `pccd_index[2]`, `pccd_record[2]`, `pccd_id[2]`,
`formation[2]` (+0x12), `kit[2][4][3]`, `kit_print[2][4]`, `roster[2][16]` and
`team_code[2][10]`, populated from the decoded record; `team_id`/`team_name`
stay the LANG display identity. The engine boot owns the decoded arena and
sets the front-end's borrowed pointer.

The bridge now stages a **pre-begin** `fifa96_match_run_set_matchup` (formation
pair + kit-0 triples). `begin` applies the ids immediately after its formation
reset, seeds both teams from the home `.fmt` (the existing both-team placement)
and reseeds team 1 from its own `.fmt` only when the ids differ; the post-begin
`set_formation` remains the live id setter. No matchup (M2/direct begin) leaves
`matchup_active` 0 and every path byte-identical. The live-only kit stage fills
the measured bank-0 bands — home shirt 208..214, shorts 195..203, socks
224..229; away shirt 239..245, shorts 230..238 through the existing per-team
remap; socks stay shared — from the selected kit-0 triples through the
PALteam frame/ramp mapping. Without a decoded identity the pre-P6 derived
frame-0/2 fallback stays for synthetic fixtures. The ball/HUD paths are
untouched; the HUD name stage now takes the PCCD `+8` code.

**Errata vs the P6-team-identity recon (append-only).** The report's §2 table
labels the kit triples with size 6 but the measured fields are 3 bytes at
`+0x25/+0x28/+0x2B/+0x2E`; §3 places the player overall at `+0x1C`, but the
measured Bebeto 95/Dunga 90 values are at `+0x1B` (the byte at `+0x1C` is 50).
The report's measured values are unchanged; only the two offsets/sizes are
corrected here. The record size is 0x89 (0x49 fixed + 0x40 roster), not a
0x49-byte record.

**Verification (2026-11-05).** `make check` 117/117 (new `test_engine_pccd`,
`test_engine_team_data`; extended `test_engine_menu_model` and
`test_engine_match_render`). M1
`./build/test_engine_m1 | cmp - port/tests/golden/engine/m1-frames.txt` exact
(`09b726b7…`); M2 v29 exact (`7c3874f7…`). Census unchanged (78/78 bodies,
58/80 natural, 64/80 install-live, 6 blocked). Windowed `DISPLAY=:1` retail
run: Brazil (home) vs Italy (visitor) selected in Team Select
(`/tmp/opencode/play/p6-teams-brazil-italy.png`); the live frame
(`p6-kits.png`) shows the HUD `BRA` / `ITALY`, score 0-0 and distinct
yellow/blue Brazil vs blue/white Italy sprites. Formation ids 1/2 are pinned by
the matchup test (home targets identical, team 1 moved to its own `.fmt`).

**Identity legs.** P6-L1 the 62-slot mode-4 table has three duplicate tails and
15 teams not exposed by the port (Algeria, Costa Rica, Croatia, Ghana, Hong
Kong, Iceland, Ivory Coast, Luxembourg, N. Ireland, New Zealand, Peru,
Singapore, South Korea, Ukraine, Zambia). P6-L2 LANG `0xA2 "Ireland"` is mapped
to Rep. Ireland; N. Ireland is not exposed. P6-L3 `set_formation` only stores
the id; the native `FUN_0008EA70` layout/`.fmt` rebind (`FUN_0007412C`) is not
mirrored for mid-match changes. P6-L4 native kit auto-selection
(`FUN_000481EC`/`FUN_0004C154`/`FUN_00047DA0`) and the away/keeper kits
(`+0x28/+0x2B/+0x2E`) are not modelled; only kit 0 is live. P6-L5 the
per-entity 256-byte translation pool (`FUN_00046F80`, `0x14BF60`, FU-144 leg 2)
and non-bank-0 body bands; the band ranges are A7's bank-0 measurement. P6-L6
player fields beyond id/name/position/rating (`+0x0B..+0x1A`, `+0x1C..`) and
the `FUN_00012010` roster/GK sort; only the roster ids are carried. P6-L7 the
team AI block `+0x10/+0x14..+0x21` (`FUN_0008C418` bounds) and the phase-2
clamp fidelity (FU-177). P6-L8 stadium/field cells `0x1492D0/D4` (settings
0x16/0x17) have no absolute writer; the stage keeps GAMEFLD1-first /
GAMEFLD3-FHstrip fallback and GAMESTD1 (FU343-L4). P6-L9 CTEAM/TC override
(`FUN_000110B8`/`FUN_00025E9C`) is dead on the CD; only the DB path is
modelled. P6-L10 kit-2/3 blue-byte quirk (`FUN_0004C154` reads `+0x27/+0x2A`).
P6-L11 `FUN_00011620`'s `count<0x10` second-decode branch is unexercised (all
mode-4 records have 16 players).

## P6 goal-path register — live goals (FU-344, 2026-11-05)

Task 2 of the P6 match-completeness wave (spec
`docs/superpowers/specs/2026-11-05-fifa96-p6-match-completeness-design.md`;
recon `/tmp/opencode/play/P6-goal-path.md` §1–§5, `/tmp/opencode/play/A5-lifecycle.md`).
The live windowed match can now score through the native chain and restart:
ball in the goal mouth -> P6-L1 arm -> scanner -> situation 6 -> direct
fallback -> phase 5 -> celebration -> kickoff -> phase 2, for both sides. Every
new behaviour is `mr->live_start`-gated except the celebration gate, which is a
native-correctness fix that is a no-op on the side-0 M2 tape. M1 `09b726b7…`
and M2 v29 `7c3874f7…` are byte-identical (verified by `cmp`, no re-pin);
census unchanged (**78/78** bodies, **58/80** natural, **64/80** install-live,
action **38/45**, phase **26/35**); `make check` **116/116**.

**Celebration release gate (native fix).** `fifa96_match_action_14` stage 2
tested `r->side != 1u`; native `0x87C1A MOV EAX,[0x157A49]; SAR EAX,0x18;
CMP EAX,1; JZ` reads the high byte of the dword at `0x157A49` = the goal-side
flag `[0x157A4C]` (`mr->global_157a4c`), not the record's team side. The old
test froze every away-side celebration in phase 5 (the measured probe stalled
9 000 steps at `1070,-2152`); it now releases for both sides, and only the
`global_157a4c == 1` sit-8 state skips. `test_engine_match_handlers` pins both:
a side-1 record with flag 0 releases to phase 1, a flag-1 record does not.
The side-1 fix is the only non-live change; side 0 with flag 0 has the same
release as before.

**Goal side through the fallback.** The situation-6 direct fallback
(`0x8AC28`) writes the phase and the goal side through `FUN_000740A0(5,side)`;
the port's `match_run_goal_fallback` called the side-less
`fifa96_match_run_situation(mr, 6)`, which hardcoded side 0. It now calls
`match_run_situation_side(mr, 6u, side, 0u)` with the scanned side, so the
away-side phase-5 arm installs `0x15`/`0x14` on team 1 and the phase-1 release
runs on the correct record. Side 0 is unchanged.

**P6-L1 live armer (labelled stand-in).** The native armer is a camera-pan
scan (`0x71390..0x713EC`: phase 2/0x10, `|camZ|_w > 0xB20 || |camX|_w >
0x730`), and the live follow replaces the unported FU-310 camera place/focus.
`match_run_live_goal_arm` (frame body, immediately before
`fifa96_match_run_goal_scan`/`0x8B63E`) checks `live_start`, phase 2, the
unarmed state and the native goal-mouth classifier `FUN_00070074`
(`0xB10..0xB90` in `|z|`, `|x| < 0xD0`, `y <= height`); on a hit it sets
`goal_armed` (`[0x15781D]`), the `0x15777C/80/84` snapshot triple with the
native y-force 0, and `goal_zone` (`[0x15781E]`). The same frame's scan then
runs the ordinary direct chain. The producer never writes the score; it is a
stand-in for the unported native camera director, not a native producer.

**Live camera height and lead.** `match_run_live_camera_follow` now keeps the
native FU-71 height `[0x157750] = 0x50` instead of pinning y = 0. That is the
native row-04/set-piece pan-arm gate (`0x20 < y < 0x70`, A2 §2), which was dead
on every live pan. The camera also keeps a forward lead of the ball velocity
times the native lead quantum 8, clamped to the ±0x120 ball-machine lead band
(`0x8BBBB`), so a moving ball is led rather than exactly mirrored. Both are
**[D]** stand-ins for FU-310/the camera director; the camera-0 render eye is
unchanged.

**Deterministic test.** New `port/tests/test_engine_live_goal.c` (CMake
`test_engine_live_goal`, ASan/UBSan): live `begin` (phase 2, session gate 0),
ball staged at `(0,0,±0xB30)` (position and target only, no score/phase write),
KICK pulses for the kickoff taker. For each side it asserts the armer
(`goal_armed`/`goal_zone`/snapshot `y = 0`, `z = ±0xB30`), score 1-0 / 0-1,
phase 5, the goal record's side and the phase-5 install (`0x15` on the scoring
side, `0x14` on the goal record, code 0 on the other team), the stage-2 release
to phase 1 with the kickoff ball `(480,0,0)` and the live camera height `0x50`,
phase 2 with the score preserved, and no re-score over 400 further steps. The
lifecycle case scores **both** sides (1-1), sets 3 s halves, and asserts
half-time (phase `0x0C`, period index 1), the second-half phase 2, and the
flag-clear regulation terminal -> OVER/POST with the 1-1 score preserved. The
1-1 score is deliberate: a differing regulation end arms the existing
extra-time branch (FU-213), so a tie is the natural full-time route; a natural
1-0 full time remains a named leg below.

**Windowed evidence (verification-only, not shipped).**
`/tmp/opencode/play/p6_goal_harness.c` (+ `p6_capture.sh`) boots the SDL3
backend on the retail ISO, stages the bridge's live art/pitch pair, sets
BRA/ITA and uses the new armer:
`/tmp/opencode/play/p6-goal.png` (HUD `BRA 1 - 0 ITA`, goal frame),
`/tmp/opencode/play/p6-goal-restart.png` (kickoff, score held) and
`/tmp/opencode/play/p6-goal-result.png` (`FULL TIME`, `BRA 1 - 0 ITA`,
`PERIOD 2`). The 1-0 natural regulation end arms extra time in the current
engine, so the FULL TIME capture stages the same flag-clear phase-`0x0C`
terminal the M2 regulation-over probe uses; the score itself is only the
native goal output.

**Legs (P6 goal path).** **P6-L1** the native FU-310 camera place/focus and
the camera director remain unported; the live armer and camera lead are
labelled stand-ins. **P6-L2** the goal caption/replay: the native ordinary
match has no goal overlay or replay (Practice-only screen machinery), so the
HUD score + celebration + restart are the complete native presentation; a
caption/replay would be a new stand-in. **P6-L3** the half-time caption/dwell:
native half-time is a phase-`0x0C` flash with no screen write (A5 §1.2).
**P6-L4** the differing-score regulation end arms the engine's extra-time
branch, so a natural 1-0 FULL TIME awaits the FU-213/OL-85 native reading (or
a "no extra time" option producer); the deterministic lifecycle test uses a
tied 1-1 regulation end. **P6-L5** the live camera lead is the derived
velocity*8/±0x120 stand-in, not a native lead producer.

**P6 Task 3 — Options and controls (FU-344, 2026-11-05).** The Options screen
is the native table now, not the P2 stand-in. A fresh `read_memory
0x105340` (234 B) decodes the 26 records `{u32 label LANG id; u32 choice-LANG
array; u8 max}` at 9-byte stride (`max` at `0x105348+9*i`); the 20-byte order
at `0x10542A` is `0,18,1,17,2,19,3,4,6,7,21,9,10,11,12,13,14,15,16,66`.
Choice arrays read first-hand: Off/On `0xD2/0xD7` (`0x1052CC`), language
`0x1052B4` = `0x6F/59/7D/72/135/A5`, Player Names `0x105328` =
`0xD2/0xC8/0xC9` (the retail `0xC9` "Netherlands" bug is shown honestly,
P6-L5), Auto Replay `0x105334`, Game Type `0x1052D4`, Fouls `0x1052DC`,
Pitch `0x1052E8`, Clock `0x1052FC`, half `0x105304` = `0x248..0x24E`, Skill
`0x105320`. Slot 19 is descriptor 66, which is dead data (`FUN_0001D948(66)`
reads `0x1F`), so the model stops at the 19 native rows and never selects it.

`fifa96_menu_model` owns the `fifa96_settings` block seeded by
`fifa96_settings_defaults` plus the new
`fifa96_settings_descriptor_max()` (retail bytes
`2,2,2,6,2,2,2,3,3,2,3,2,5,2,7,2,2,100,100,100,0,2,0,0,0,0`).
`fifa96_settings_defaults` still leaves `max[]` zero (FU-49's pinned test).
`fifa96_menu_model_reset` re-enters Game Select while preserving `settings`;
`fifa96_frontend_run_reset` uses it, so values survive a match return (the
native driver keeps the `0x149278` block). Rows 0..18 are the native order;
rows 19/20 are **port additions**: a Controls page and Reset Defaults (there
is no native row for either).

**Live rows (v1).** Only six native rows reach a consumer: Sound Effects
`0x01` (0 ⇒ audio percent 0), Fouls `0x0A` (`[0x14C306]`), Injuries `0x0B`
(`[0x14C30A]`), Half Length `0x0E` (`[0x14C1D1]` + `[0x15881A/1C]`), Offside
`0x10` (`[0x14C2F2]`) and SFX Volume `0x11` (slider step 5). Everything else
is visible but dim/stored. Left/right changes a live row; Enter cycles
(gamepad convenience; sliders step +5). The eight-row window uses the native
geometry divided by two: x 66, width 222, first row y 92, pitch 16, row height
14 (native `0x85/0x1BC/0xB8/0x21` in 640×480). The title is LANG `0xD8`; the
selection bar uses `c->sel_fill`, dim rows `c->dim`, values are right-aligned
and half length carries the port-added `min` suffix; a scrollbar appears for
the 21-row list. Half index 7 (the native 8th table value `0`, `0x37170`) is
displayed honestly as `0 min` even though it has no LANG choice.

**Apply at match start.** Inside the existing
`eng->frontend.team_selected` bridge block, after `begin`, the bridge runs
`fifa96_settings_handoff(&eng->frontend.menu.settings, 0u, &cfg)` →
`fifa96_match_run_set_config(mr, &cfg)`. The setter checks `mr->running`,
copies the whole `fifa96_match_config` (so `field_4c306/30a/2f2` and the other
cells land where the engine reads them) and calls
`fifa96_match_run_set_period(mr, cfg.period_length, cfg.extra_length)`,
mirroring `FUN_0003749C`. The half mapping is minutes → `*0x3C` /
`*0x14`; default index 0 is 120/40 and is a no-op against `begin`'s FU-68
default handoff. The call is gated by `team_selected`, so M1/M2 and every
direct-begin fixture keep `begin`'s defaults.

**Audio gain.** `fifa96_match_audio_start` takes `sfx_percent` (0..100, 100 =
unity). The engine passes `settings.value[1] ? settings.value[0x11] : 0`.
Crowd/whistle/goal base gains are scaled by percent: 100 keeps 0x60/0x7F,
the default 91 gives crowd `0x60*91/100 = 0x57`, 50 gives 0x30 and 0 gives 0.
Headless still never starts match audio (M2 asserts no `audio_submit`);
Music/Speech Volume have no consumer (P6-L2).

**Controls page.** A plain HELV12 block (port addition) with the real current
bindings: arrows (move/menu), Enter (select/resume), X/Backspace (back/quit to
menu), Esc (menu quit, match pause), Z kick, C pass, V long ball, D-pad move,
South=A confirm, East=B back, West long ball; match kick/pass are **not**
mapped on the pad (P6-L6), and the text says so.

**Tests.** Descriptor count/order/max/labels/defaults and the dead slot;
live-row cycling and slider step; half index 0→120/40, 6→2700/900, 7→0;
`fifa96_frontend_run_reset` keeps the settings block; model navigation,
Controls, Back and Reset Defaults; the bridge copies the config cells and
periods only with `team_selected` (direct begin stays 120/40); audio crowd
gain at 100/91/50/0 plus the whistle gain; Options art geometry and the
fallback menu hash. `make check` green; M1 `09b726b7…` and M2 v29
`7c3874f7…` byte-exact by `cmp`; census unchanged (78/78 bodies, 58/80
natural, 64/80 install-live).

**Windowed evidence (verification-only).** `DISPLAY=:1`
`/tmp/opencode/play/p6-options.png` (default list), `p6-options-half.png`
(Half Length 4 min) and `p6-controls.png`; `p6-options-half-return.png`
proves the value survives back → Game Select → Options. No windowed 4-minute
half was waited out; the deterministic bridge case proves 4 min → 240/80.

**Legs (P6 Options; the labels repeat the goal-path register's numbering by
the Task 3 brief — an append-only identifier collision, not a rewrite of the
earlier section).** **P6-L1** `options.dat` persistence: the 104-byte block is
decoded but v1 keeps settings in memory (a CWD write would break hermetic
tests). **P6-L2** Music/Speech Volume: no non-UI consumer (A6-L4/L5 first).
**P6-L3** Skill Level: `[0x14C326]` gates `FUN_0007B57C`; no engine caller
yet. **P6-L4** Clock: `[0x14C302]` is read at `0x8AF63`; the port still
hardcodes `clock_halt = 0` in its live timing. **P6-L5** retail data: slot 19
= descriptor 66 (dead) and Player Names' third choice `0xC9` "Netherlands"
should be `0x1C9` "Numbers". **P6-L6** gamepad match buttons: South/East do
not reach `match_run_map_key`; only West (long ball) works. **P6-L7**
Time Display / Player Names / Auto Replay / Super VGA remain stored-only and
dim until consumers land.

## P7 — live kickoff control and possession (FU-345, 2026-11-06)

**Diagnosis.** The live windowed bootstrap (`fifa96_match_run_begin`'s
`live_start` block) writes phase 2 after the kickoff placement and binds only
`render.camera.tracked`. It skips the phase-1 wait that drives the natural
kickoff producers: row 01 stage 0 publishes `[0x157A83]`
(`entities.controlled`), row 02's `0x7E041` request merges the FU-70 slot to
the second kickoff pick, and row 04's coda installs carrier code 5 (`0x7F133`).
The begun live run therefore left `entities.controlled` = NONE and the human
slot on the setup-bind donor, with no carrier state for the released code-1
input row; KICK did nothing. Two live-only secondary blockers were measured:
the row-07 `0x81519` lane gate reads the record `+0x6B` lane word (~0x2E0 with
the real `.fmt` formation, vs the fixture's 0), so the native `lane_word <=
0x40` arm waits out and resets before stage 1; and an AI `FUN_0007876C` merge
can move `+0x20` earlier in the same record walk, hiding the human's input
edges from the authoritative slot record. The zero-placement fixtures pass
both gates by construction, which is why the natural kickoff test was green
while the windowed match was not.

**Live-control producer.** New `match_run_live_control_seed` at the end of the
live begin selects the controlled side's nearest outfield record to the ball
with the native `FUN_0008D8EC` shape (`fifa96_match_entities_team_select`,
which skips record 0), moves the FU-70 slot through
`fifa96_match_entities_merge_slot`/`take_slot_merge`, publishes the target with
`fifa96_match_entities_actor_lift`, installs carrier state 5 through
`fifa96_match_entities_install`, sets `ball.carrier`, clears the FU-73 pair to
its kickoff posture, and binds `camera.tracked`. New
`match_run_live_control_update` runs after the entity drain: while the opening
kick is pending it holds the seeded taker as the carrier (`live_kickoff_touch`,
cleared by the first row-07 dispatch); afterwards it prefers the controlled
team's `ball.carrier` when it is an outfield record within
`MATCH_RUN_LIVE_CONTROL_RANGE` (0x400) of the ball, else the nearest own
outfield record. It never selects the keeper or an opponent record, leaves the
opponent block unwritten, and rebinds the slot/target/`entities.controlled`/
camera through the same helpers every frame. A kick release clears
`ball.carrier` and holds off re-claiming for 30 frames; the live possession
claim itself (own nearest at least as close as the opponent's nearest) is a
distance stand-in for the unported row-05 possession caller. Two live-only
dispatch seams report the authoritative slot owner (`s.has_slot`/`r->has_slot`)
against a same-frame AI merge, and the row-07 lane gate is fed the ready value
for the human slot owner (the live `+0x6B` producer is unported). Every branch
is `live_start`-gated; M1/M2 are headless and byte-exact. This is the labelled
[D] stand-in for the unported FU-295 live caller chain, not a fabricated score
or goal path.

**Tests.** New `port/tests/test_engine_live_control.c` (4 cases): the live
begin seed (nearest taker, slot/controlled/team target/camera, `has_slot`,
carrier bit, code 5, `ball.carrier`); KICK press then release through
`fifa96_match_run_input` dispatches row 07, latches the FU-73 pair
(`pair.actor`/`pair.flags` 0x10), moves the ball and leaves the score 0-0;
the per-frame follow picks a staged carrier, ignores an opponent possession
cell for the own nearest record, and never selects the keeper; and a staged
goal-mouth ball after the kick still runs the native P6 goal -> phase 5 ->
phase-1 restart -> phase 2 chain with the score intact. `make check`
119/119 green; M1 `09b726b7…` and M2 v29 `7c3874f7…` exact by `cmp`; census
unchanged (78/78 bodies, 58/80 natural, 64/80 install-live, action
38/45, phase 26/35).

**Windowed evidence.** `DISPLAY=:1`, retail ISO, Friendly -> Brazil vs Italy:
`/tmp/opencode/play/p7-kickoff.png` (kickoff, 00:01) and
`/tmp/opencode/play/p7-kick.png` (after the Z press, 00:03); the
`p7-kick-1..10.png` burst and `p7-attack-*.png` frames record the ball moving
off the kickoff spot. Natural scoring was not reached: the FU-73 pair latches,
but the port's live ball machine moves the ball on its camera-relative target
and does not consume the staged kick vector, so there is no live shot/dribble
producer to carry the ball into the goal mouth. No `p7-goal.png`/
`p7-restart.png` is claimed; the staged goal path is green in the test and in
the P6 windowed evidence.

**Legs (P7).** **P7-L1** native auto-switch/AI: the FU-295 live caller chain
(including the player-switch and camera director) is unported; the selector
above is the derived stand-in. **P7-L2** full possession/tackle AI: the row-05
claim/steal chain is unported; the live claim is a nearest-distance stand-in
and the opponent block's own possession cells are ignored, not cleared.
**P7-L3** live kick launch: the FU-73 pair stages the actor/flags but the ball
machine does not consume the kick vector; the native ball-flight/launch
producer (and the paired receive hand-off) is unported, so a windowed shot
does not travel to the goal mouth. **P7-L4** the live row-07 lane gate `+0x6B`
producer (the FU-310/FU-311 live formation lane): the live slot owner is fed
the gate's ready value. **P7-L5** gamepad kick/pass: P6-L6 still stands (only
West/long ball reaches `match_run_map_key`). **P7-L6** natural windowed
scoring: blocked by P7-L1/L2/L3; the native goal/restart itself is green.

## P7 addendum — live ball flight and natural goal (FU-345, 2026-11-06)

**Flight consumer.** The row-07 kick stages the FU-73 block (speed
`vector.x`, direction `vector.height/z`, loft `traj`) but the native per-code
target algebra (`0x7AA3C..0x7AE2F`, OL-62) and the ball-vector consumer are
unported; the phase-2 ball machine kept steering the ball at its camera-relative
target, so the staged kick was only a nudge. New live-only latch
`match_run_live_kick_latch` (called from `match_run_live_control_update`): when
the pair carries the human slot/actor (`slot.entity`, `entities.controlled` or
the seeded `live_control_taker`) and `flags & 0x70`, it records
`live_kick_target = kicker_pos + 2*(vector.height, vector.z)`,
`live_kick_speed = clamp(|vector.x| >> 6, 8, 16)`, `live_kick_frames = 300`
and a pair signature (actor/traj/flags/code) so the same staged pair is
consumed once. `match_run_ball_machine` uses that target/max-speed while
`live_kick_active` (bypassing only the camera-relative target block; the native
metric/heading/velocity/ramp/integration code is untouched) and clears on
arrival, `goal_armed`, phase change, out-of-play (`|x| > 0x6C0`, `|z| >
0xC00`) or the 300-frame window. The mapping is a labelled [D] derived
consumer for the unported per-code algebra: it uses the staged speed/direction
semantics but does not claim the native loft arc, receive hand-off or
per-code target arms. No score/phase/goal cell is written.

**Stationary-kick aim fallback.** The live slot direction is recomputed from
the current input every frame, so a stationary KICK release passes `dir =
(0,0)` into the row's grounded L2 arm and `kick_dir_vector` stages a zero
vector (the fixture's old pair was actor/flags only). `match_kick_run` now
substitutes the record's own `+0x8E` type-dir octant (the same
`0x10F334/0x10F33C` table the native type-dir arm uses; side-flipped for team
1) for the live slot owner when the slot pair is zero. This is a derived
facing fallback, not auto-aim: the direction is the record's facing byte.
Live-only; M1/M2 are byte-exact.

**Tests.** `test_engine_live_control` now has five cases; the new
`test_live_kick_flight_scores` stages the taker/ball at the authentic attack
position `(0,0,0x900)` (just outside the +z goal area, not inside the goal),
stages the record facing type 0 (+z), presses/releases KICK through
`fifa96_match_run_input`, asserts the latched target is beyond the goal line
and the speed >= 8, then drives the native goal scan: score 1-0, phase 5,
`goal_record_side 0`, a ball travel > 0x100, then the native phase-1 restart
and phase-2 resume with the score preserved. The test writes no score/phase.
The earlier four cases remain green. `make check` 119/119; M1 `09b726b7…` and
M2 v29 `7c3874f7…` exact by `cmp`; census unchanged (78/78 bodies, 58/80
natural, 64/80 install-live, action 38/45, phase 26/35).

**Windowed evidence.** A verification-only harness (`/tmp/opencode/play/
p7_flight_harness.c`, not shipped) boots the retail ISO, starts a live match,
stages BRA/ITA HUD names, stages the taker/ball at `(0,0,0x900)` with the
record facing type 0 (+z), and places the camera focus in the attack area on
the pre-kick frames (the live follow may override it); it presses/releases KICK
through SDL events. The
goal is produced only by the native row-07 kick + derived flight + live goal
armer + native goal scan (log: `KICK row07`, `MILESTONE FLIGHT step=158`,
`MILESTONE GOAL step=198 score=1-0 phase=5`, `MILESTONE RESTART step=948 phase=1 ball=480,0,0`). Captures: `/tmp/opencode/play/p7-flight.png`
(attack area, 0-0, ball leaving the taker), `p7-goal.png` (BRA 1 ITA 0) and
`p7-restart.png` (BRA 1 ITA 0, kickoff circle). The earlier real-menu
`p7-kickoff.png`/`p7-kick.png` remain the natural-input kick evidence. The
harness stages the attack position and facing byte; no score or phase is
staged.

**Additional legs (P7 flight).** **P7-L7** native per-code target algebra
(`0x7AA3C..0x7AE2F`, OL-62) and the `FUN_0007A084` receive target: the derived
target uses `kicker + 2*direction` and ignores `traj`/loft, camera offsets and
the per-code arms. **P7-L8** the live facing/aim producer: the stationary
fallback uses the `+0x8E` octant; the native input-driven facing and the
dribble/aim AI remain unported. **P7-L9** pair re-latch: a second kick with the
identical actor/traj/flags/code signature is treated as already consumed; a
new vector/code re-latches. **P7-L10** the native loft arc/ball height and
receive/bounce hand-off are not modelled (target_y is 0, the goal armer sees
y=0). **P7-L11** natural menu-input scoring (arrows+Z without harness staging)
still depends on the live AI/aim legs; the native chain itself is green.

**Errata (P6 T3 review, 2026-11-06 — append-only; the Task 3 section above
stands as written).** Two reads are corrected by a fresh `read_memory` /
`disassemble_bytes` re-read. (1) Player Names' choice array at `0x105328` is
the three dwords `0xD2, 0x1C8, 0x1C9` = Off / Names / Numbers (LANG000
`0x1C8` = "Names", `0x1C9` = "Numbers"); the earlier `0xC8/0xC9` reading was
a width misread (those are the low bytes of `0x1C8/0x1C9`, which decode to
"Name"/"Netherlands"). There is no retail Netherlands bug: P6-L5's Player
Names clause is withdrawn and P6-L5 remains only the dead slot 19. (2) Slot 19
is descriptor 66 at `0x105340 + 66*9 = 0x105592`; its dword is `0x001C0000`
(bytes `00 00 1C 00`), not `0x1F` (`0x1F` is the byte at `0x105590`; `0x1C`
is at `0x105594`). Descriptor 66 is still dead data and the model still never
selects it. The model and test now use `{0xD2,0x1C8,0x1C9}` with the
Off/Names/Numbers fallbacks. The review's remaining gaps are closed by the
native Options state-fill case (`test_engine_menu_model`) and the goal-gain /
Sound-Effects gate cases (`test_engine_audio`).

## P7 addendum 2 — live shot aim and kickoff goal (FU-345, 2026-11-06)

**Shot aim.** The derived flight target was `kicker + 2*direction` from the
record's static `+0x8E`/pair direction, so a normal kickoff shot missed the
mouth. `match_run_live_kick_latch` now recognises the human KICK row
(pair flags mode `0x10`; `(flags & 0x10) != 0` and `(flags & 0x60) == 0`) and
sets the flight target to the opponent goal-mouth centre: `x = 0`,
`z = side == 0 ? +0xB30 : -0xB30`, `target_y = 0`. The sign is the one verified
by the successful windowed goal (side 0 attacks +z). PASS/LONG_BALL
(`0x20`/`0x40`) keep the staged kicker-relative direction. KICK goal shots use
the bounded 600-frame flight window (the kickoff-to-mouth travel); the existing
speed clamp `8..16` and the native metric/heading/velocity/ramp/integration,
goal armer and goal scan are unchanged. This is a labelled [D] shot-aim
stand-in for the unported native aim/camera director; it writes no score,
phase or goal cell. No held-direction intent is consumed: the human KICK always
aims at the mouth centre.

**Tests.** `test_engine_live_control` now has the no-staging
`test_live_kickoff_shot_scores` for both sides. It starts from the real live
kickoff (ball `(480,0,0)`, no ball/taker/camera staging), presses/releases KICK
through `fifa96_match_run_input`, asserts the latched target is
`(0, ±0xB30)` with the 600-frame window, then drives the native goal chain to
score 1-0 (side 0) and 0-1 (side 1, controlled side flipped through
`phase_machine.side_controlled` after begin), phase 5, `goal_record_side` =
the scoring side, `goal_armed`/`goal_zone`, then the native phase-1 restart and
phase-2 resume with the score preserved. No ball position or score is staged in
this test. The staged attack fixture and the earlier four cases remain.

**Windowed evidence (shipped menu path, no harness).** `./build/fifa96
data/game/FIFAPCCD96.iso` on `DISPLAY=:1`, Friendly -> Brazil vs Italy, wait
for the kickoff (`p7-natural-kickoff.png`, 00:01), press Z once: the native
shot flies from the kickoff spot and scores BRA 1 ITA 0.
`/tmp/opencode/play/p7-natural-goal.png` (1-0, ball on the goal line) and
`p7-natural-restart.png` (1-0, kickoff circle) are the captures; the score and
ball are native, nothing is staged. The first Z produced the goal; a second Z
was used to release the native phase-1 kickoff gate and reach the restart
capture (the same FU-70 release the kickoff state machine waits for). The
windowed build's clock/frames run faster than wall time, so the 4-6 s capture
window was sampled until the score appeared.

**Additional leg (P7-L12).** The native aim/camera director (held-direction
intent, goal-side selection, keeper/defender blocking) is unported; the KICK
row always aims at the mouth centre, so a keeper or off-centre save cannot
happen. P7-L7 (native per-code target algebra/receive target) and P7-L10 (loft
arc/receive hand-off) still stand.

## P7 addendum 3 — playable restart possession and taker guard (FU-345, 2026-11-06)

**Restart re-seed.** The live seed ran only at begin, so after a goal the
opponent kickoff carrier could stand on the ball forever: the nearest-only
claim requires a strict distance win and can never beat distance 0. A live-only
`live_restart_pending` is armed on every phase-1 frame and consumed on the
phase-1 -> phase-2 transition: `match_run_live_restart_bind` selects the
controlled side's nearest outfield record to the ball and rebinds the FU-70
slot / `entities.controlled` / team target / camera through the existing
`match_run_live_control_bind`, without awarding possession (the opponent keeps
the kickoff ball). The same transition clears any stale `live_kick_hold` and
`live_kick_active`.

**Contact tackle/claim (P7-L2 stand-in).** `match_run_live_control_tackle` is
the derived contact steal for the static AI restart: when the selected human
record is within 0x40 of a loose/stationary opponent-carried ball
(`ball.speed <= 4`), no live kick is in flight and the post-kick hold is clear,
it sets `ball.carrier` to the human and installs carrier state 5 through the
real installer. The normal nearest claim now uses a **strict** distance win
(`own < opp`), so a tie at distance 0 no longer steals the opponent's restart
ball; the tackle is the explicit contact path. Neither path writes score,
phase or a goal cell. The possession claim is also guarded by
`!live_kick_active` (no re-possessing a ball already in flight).

**Taker guard.** `live_control_taker` is accepted by the flight latch only
while `live_kickoff_touch != 0` (the opening-kick release frame); the touch
clear also sets `live_control_taker = NONE`. A later AI pair staged by the old
taker cannot arm a human flight; only the current human slot/`controlled`
actor can.

**Vector re-latch.** The flight signature now includes the staged
`vector.x/height/z` in addition to actor/traj/flags/code, so a new direction
with the same actor/power/trajectory/event code re-latches. P7-L9's earlier
"a new vector/code re-latches" wording is now true.

**Tests.** `test_engine_live_control` now has **seven top-level cases / eight
functions including `test_live_kickoff_shot_scores_side`** (the earlier
four/five/six counts are historical): the new/strengthened coverage is
`restart_assert_possession` (after the native goal -> phase-5 -> restart ->
phase-2 chain, place the human defender at contact, assert own possession/code
5), a restart KICK press/release that asserts a new live flight and ball
movement with the score still 1-0, the side-0 and side-1 kickoff-shot cases,
and `test_live_taker_guard_and_relatch` (AI pair from the old taker leaves
`live_kick_active` 0 and the target unchanged; a current human pair latches;
a same-actor/traj/flags/code pair with a new vector re-latches). The
goal-after-kick restart also asserts own possession before its no-rescore
window. `make check` 119/119; M1 `09b726b7…` and M2 v29 `7c3874f7…` exact by
`cmp`; census unchanged (78/78 bodies, 58/80 natural, 64/80 install-live,
action 38/45, phase 26/35).

**Windowed evidence (shipped menu path, no harness).** `./build/fifa96
data/game/FIFAPCCD96.iso`, Friendly -> Brazil vs Italy, Z at kickoff -> native
1-0, Z to release the phase-1 kickoff gate, arrows to the ball/contact tackle,
Z to kick. Captures: `/tmp/opencode/play/p7-restart-possession.png` (1-0,
restart centre, human at the ball) and `p7-restart-kick.png` (1-0, ball moving
after the human kick). The score remains the native 1-0; no score/ball staging.
The capture script moved with Left/Right and the kick was taken after the
restart re-seed/contact tackle.

**Closure note (append-only; earlier P7 text remains historical).**
**P7-L3** ("ball machine does not consume the kick vector") is CLOSED by the
flight addendum above. **P7-L6** ("natural windowed scoring blocked") and
**P7-L11** ("natural menu-input scoring still depends on AI/aim") are
SUPERSEDED by addendum 2: the shipped menu path scores from the kickoff with
one Z. The remaining open legs are the native aim/camera director (KICK always
aims at the goal-mouth centre; no held-direction intent, goal-side choice,
keeper or defender blocking), native per-code target algebra/receive target
(P7-L7), and the loft arc/receive hand-off (P7-L10). The restart/tackle is the
P7-L2 stand-in; full possession/tackle AI remains unported.

## P6 Task 4 / final playability acceptance (2026-11-06)

**Measured shipped loop.** `./build/fifa96 data/game/FIFAPCCD96.iso`,
DISPLAY=:1: Game Select -> Options (Half Length 4 min; settings survive
Back -> Options) -> Friendly -> Brazil (home) / Italy (visitor) -> kickoff
(BRA 0 ITA 0). One Z at the kickoff runs the live-control seed, row-07 kick,
the derived live flight and the native goal scan: **BRA 1 ITA 0** (native
score, phase 5/celebration, restart). The second Z releases the native
phase-1 gate; arrows/contact tackle regain possession at the restart, and Z
kicks again with the score preserved. Captures:
`p6-options.png`, `p6-options-half.png`, `p6-controls.png`,
`p6-teams-brazil-italy.png`, `p6-kits.png`, `p7-natural-goal.png`,
`p7-natural-restart.png`, `p7-restart-possession.png`, `p7-restart-kick.png`,
`p7-main-goal-2s.png`/`p7-main-goal-8s.png` (merged-main BRA 1-0, period 2).
The one-Z goal is the shipped menu path (no harness, no staged score); the
P7 tests stage only the preconditions they name.

**Gate.** `make check` **119/119** (strict build + ASan/UBSan); M1
`09b726b7…` and M2 v29 `7c3874f7…` byte-exact by `cmp`; census **78/78**
bodies, **58/80** natural, **64/80** install-live, 6 blocked, action 38/45,
phase 26/35.

**Doc errata (append-only; earlier Task 3 text remains).** (1) The Task 3
"Windowed evidence" sentence that `p6-options-half-return.png` proves the
value survives Back -> Options is corrected: that capture is byte-identical
to `p6-options-half.png`; the deterministic/front-end test is the proof, the
screenshot is not. (2) **P6-L1** (`options.dat`) wording: the 104-byte block
format is decoded in the recon, but no runtime options-file decoder exists in
the tree; v1 intentionally keeps the settings in memory. (3) The Player
Names `0xC8/0xC9` claim in the Task 3 legs is superseded by the P6 review-fix
section above (correct dwords `0xD2/0x1C8/0x1C9` = Off/Names/Numbers).

**Remaining playability legs.** Native aim/camera director (KICK always
goal-centre; no held-direction intent, goal-side choice, keeper/defender
blocking; P7-L12), per-code kick target algebra/receive (P7-L7), loft/receive
hand-off (P7-L10), full possession/tackle/AI + native auto-switch (P7-L1/L2),
regulation-end extra-time branch (P6-L4), replay/captions/half-time dwell,
rosters/attributes/AI, commentary/music, gamepad kick/pass (P6-L6), and the
six other Game Select screens. Each is numbered, not claimed.

## P8 — Friendly full-time branch (FU-346, 2026-11-06)

**Outcome.** A finished live **Friendly** now reaches FULL TIME and the return to
Game Select on both a **1-0 winner** and a **0-0 draw**; it no longer enters the
engine's extra-time re-arm on a winner. The change is one live-gated term in
`fifa96_match_run_phase_drive`'s W2 tie-break (the shipped windowed menu path is
the only live Game Select mode, Friendly); the non-live/competition route (and
therefore M1/M2) is byte-identical.

**Resolved native reading (fresh Ghidra, `/FIFA96.EXE`, this slice).**

* **`[0x157AC0]` is not an extra-time switch in the football sense.** W2
  `0x8B2AC MOV byte[0x157AC0],1` fires on a **differing** regulation score
  (`0x8B2A7 CMP AX,DX; 0x8B2AA JZ` -> W3/`[0x10F378]` else-arm); the chooser
  `FUN_0008B9CC` `0x8B9CF CMP byte[0x157AC0],0 / 0x8B9D6 JZ 0x8BAA7` then writes
  phase `0x13`/`0x14` and installs act `0xC` (`0x8BAC6..0x8BAE7`), while the
  clear arm writes phase `0x0C` and act `0xB`.
* **Phase `0x13`/`0x14` are class-0 post-match screens, not ET halves.** Fresh
  `read_memory 0x1106AD`: indices `0x13`/`0x14` are `0x00`; the live ET half is
  class-1 phase 2 on the period index 2/3 (`0x8BB65`). The phase-`0x13` producer
  installs the row-`0x25` stats sequence (`0x880CC`) when the score pair differs
  (`0x8D76C CMP AX,[0x157AC7]; 0x8D772` code `0x25` vs `3`), and the end machine
  **holds** a decisive screen (`0x8BD95 CMP AX,[0x157AC7]; 0x8BDA2 JNZ 0x8BDE7`).
  Act `0xC` is the three-stage end timer (`0x8B900`, act slot
  `0x1107EC[0xC]`). So a native Friendly 1-0 reaches the post-match stats/result
  presentation, not a clocked extra-time half.
* **The ET configuration is the competition gate.** `FUN_0003749C` writes
  `[0x14C2EE]=1` (`0x374D1`) and clears it (`0x374E7/0x374E9`) when
  `[0x14C1D0]` is 0 or 3. `[0x14C1D0]` is the Game-Select mode byte
  (`FUN_0001B7B8 0x1B7B9`); Friendly is mode 0 (`FUN_00024B00 0x24BEB
  XOR EAX,EAX; CALL 0x1B7B8`), so Friendly's override is 0. W3 (equal score +
  override, `0x8B2B8/0x8B2C1`) is the competition force-ET arm.
* **A Friendly draw does not play extra time either.** W2's equal-score
  else-arm, with `[0x14C1D0]==0`, sets the `[0x10F378]` no-ET latch
  (`0x8B2CD CMP byte[0x14C1D0],0; 0x8B2DA MOV dword[0x10F378],1`); the
  phase-`0x0C` end machine then takes `0x8BC80 CMP [0x10F378],0 /
  0x8BC89 CALL 0x37F54` -> `[0x105FFC]=5` (the no-ET terminal, FU231-L2).
  Earlier wording that left `[0x10F378]` "unported / no producer" is corrected
  here: its producer and gate are the fresh W2 else-arm above.
* **The port's mistake was the re-arm, not the flag polarity.** W2 was
  faithful, but `match_run_period_over_rearm` treated every live `[0x157AC0]=1`
  phase `0x13`/`0x14` as an ET entry and moved to phase 2. Native phase `0x13`
  is decisive-score-held. P8 gates the flag producer on the live Friendly
  regulation-end winner so the engine's existing phase-`0x0C` result terminal
  (the same one the Friendly draw uses) runs; the native phase-`0x13` stats
  screen body is unported.

**Port shape (P8-L1, labelled stand-in).** In
`port/src/fifa96_engine/fifa96_match_run.c` (`phase_drive`, W2):

```
if ((completed == 1u || completed == 3u) && differ &&
    !(mr->live_start && completed == 1u))
  mr->global_157ac0 = 1u;
```

`live_start` is the windowed/menu path whose only shipped Game Select mode is
Friendly; the non-live M2/competition route keeps the native flag/phase-`0x13`
route. The live terminal is phase `0x0C` / act `0xB` -> end word `1` ->
OVER -> POST -> EXIT -> `FIFA96_ENGINE_MODE_FRONTEND`; the score is preserved.

**Tests.** `port/tests/test_engine_match_completion.c` gains
`test_live_friendly_regulation_win_full_time` (live start, a derived 1-0, real
1 s halves through the live sit-1 restart; asserts phase `0x0C` (not `0x13`),
`global_157ac0==0`, act `0x0B`, index 2, end word 1, POST, score 1-0, and
`run_end` -> FRONTEND/`match==NULL`) and
`test_live_friendly_regulation_draw_full_time` (0-0, the same terminal). The
non-live `test_loopbacked_tiebreak_completion` still pins the competition
flag/phase-`0x13` arm. `make check` green (see the gate line below), M1
`09b726b7…` and M2 v29 `7c3874f7…` byte-exact by `cmp`; census unchanged
(78/78 bodies, 58/80 natural, 64/80 install-live, action 38/45, phase 26/35).

**Windowed evidence.** `./build/fifa96 data/game/FIFAPCCD96.iso`, DISPLAY=:1,
default 2-minute halves, Friendly Brazil/Italy. Shipped input only: the first Z
at kickoff seeds live control (still 0-0 at 00:02); the second Z runs the
row-07 kick/flight/native scan -> **BRA 1-0** at 00:06. No further Z (the
half-time sit-1 kickoff was released with C/PASS at 02:00; a bare wait stalls
there, P8-L4), then the second half ran to the terminal:
`/tmp/opencode/play/p8-fulltime.png` (FULL TIME, **BRA 1-0 ITALY, PERIOD 2**)
and `/tmp/opencode/play/p8-return-menu.png` (Game Select). No harness and no
staged score; the goal is the shipped input chain.

**Residual legs.** **P8-L1** the native phase-`0x13`/`0x14` post-match stats/
camera screen and its user-driven leave are unported; the live Friendly uses
the phase-`0x0C` result terminal as the labelled stand-in. **P8-L2** the native
competition extra time (`[0x14C2EE]` override; non-Friendly modes) and the penalty
continuation stay non-live/separate; the phase-`0x13` -> phase-2 re-arm remains
their tape-only route. **P8-L3** the `[0x105FFC]=5` no-ET terminal (a Friendly
draw's native screen) is unported (FU231-L2); the engine's phase-`0x0C` result
panel stands in for it. **P8-L4** the half-time sit-1 kickoff needs a real slot
release: a bare `fifa96_match_run_frame` fixture stalls in phase 1 (no input),
and the windowed run released it with C/PASS at 02:00 (the deterministic tests
use the KICK/UP tape + engine steps). **P8-L5** `[0x14C1D0]`/`FUN_0001B7B8`
and `[0x14C2EE]` remain unmodelled cells (the live gate is the explicit
stand-in).

## P8 — gamepad kick and pass (FU-346, 2026-11-06)

**Mapping.** The SDL3 backend still maps South/A to
`FIFA96_ENGINE_KEY_CONFIRM`, East/B to `FIFA96_ENGINE_KEY_DECLINE` and West to
`FIFA96_ENGINE_KEY_LONG_BALL` (`platform_sdl3.c`, now exposed as the pure
`fifa96_platform_sdl3_map_button`). The front-end keeps its existing
CONFIRM/DECLINE consumption (`fifa96_frontend_run.c`: -10 confirm, 8
back/panel). In the live match, `match_run_map_key` additionally maps
CONFIRM to the FU-61 KICK word `0x10` and DECLINE to PASS `0x20`; keyboard
Z/C/V and the arrows are unchanged.

**Pause/menu routing.** `fifa96_match_run_input` accepts the two dual-role
codes only while the run is unpaused and `lc.screen` is ACTIVE, so the pause
panel's A resume / B exit and the FULL TIME A continue never stage a match
kick/pass. The engine tracks the raw CONFIRM/DECLINE down state across INTRO,
FRONTEND and MATCH and passes only an armed hold to the match input
(`match_pad_down`/`match_pad_allowed` in `fifa96_engine_internal.h`): a key
held across the menu -> match handoff or through a pause must be released
before it becomes a match button. A fresh press stays held like Z/C (the
backend's per-poll state sample), so the pad and keyboard share the same
FU-70 press/release path.

**Controls page.** `fifa96_menu_model_controls_text` now says the true
mapping: `PAD A/SOUTH KICK / CONFIRM`, `PAD B/EAST PASS / BACK`, `PAD WEST
LONG BALL`; the old "PAD KICK/PASS NOT MAPPED" line is gone.

**Tests.** `test_engine_match_input` pins CONFIRM -> 0x10 and DECLINE -> 0x20,
the paired direction OR (`CONFIRM|UP` -> 0x11), the KICK press/release slot
path, the ACTIVE-and-unpaused gate, the held pad sample through consecutive
engine steps, and the pause A-resume/B-exit routes with no KICK/PASS staged.
`test_engine_sdl3` pins the pure button map without a physical pad (including
South/East/West/Start and an unmapped North). `test_engine_menu_model` pins
the new Controls text, and `test_engine_frontend` re-asserts that menu CONFIRM
still latches/clears `frontend.confirm` and menu DECLINE still returns without
quitting.

**Gate.** `make check` **119/119** (strict build + ASan/UBSan); M1
`09b726b7…` and M2 v29 `7c3874f7…` byte-exact by `cmp`; census unchanged
(**78/78** bodies, **58/80** natural, **64/80** install-live, 6 blocked,
action 38/45, phase 26/35).

**Doc errata (append-only; earlier P6/P7 text remains historical).**
**P6-L6** ("South/East do not reach `match_run_map_key`; only West works") is
CLOSED by the mapping above. **P7-L5** ("P6-L6 still stands") is CLOSED for
the same reason. The P6 Task 3 sentence "match kick/pass are **not** mapped on
the pad (P6-L6), and the text says so" is superseded by the Controls page
update. The dual-role keys deliberately avoid a new engine key so the
front-end binding stays byte-for-byte the same; the match gate is the only
new behavior.

## P8 — live opponent movement and shot block (FU-346, 2026-11-06)

**Answer in one line.** The live opponent is no longer the single kickoff
target: a derived live re-select runs the ported native nearest query at the
native third-template anchor, so the nearest outfield record picks up the ball
and contests; the row-05 possession claim is repaired so an opponent can only
take `ball.carrier` at contact, not from ~1100 units. A separate labelled live
shot block lets a defending **outfield** record inside 0x28 of the ball in the
final approach zone stop a KICK flight; it is tuned (defender-only, 0x28,
`|z| >= 0x900`) so the shipped kickoff shot still reaches the native goal.

**T1 — opponent target rotation (P8-L1 stand-in).** Fresh recon (authority
`/tmp/opencode/play/P8-ai-recon.md`) pinned the native gate: `FUN_0008D8EC`
selects a team target only while `[team+0x7B2] == 0 || [0x157A83] == 0`
(`0x8D932 CMP dword[EBP+0x7B2],0`; `0x8D93B CMP dword[0x157A83],0`; the
`0x8D929 CMP EAX,0x2` phase gate). The native clears that re-open the gate live
in unported row bodies and `FUN_0007F7E0`; the port therefore kept the kickoff
target forever, row 03 (hold/clamp) is NULL, and only one opponent record moved.
`match_run_live_opponent_update` (live, phase 2) re-runs the same ported
`fifa96_match_entities_team_select` (`0x8D9B2 CALL 0x8DE8C`, nearest, skip 0)
at the native **third template** anchor (`0x8D948..0x8D9B7`, template 2 =
`0x157770` plus `dword[0x1577BE]>>16<<5` / `dword[0x1577C0]>>16<<5`; the port's
FU-300 lead unpack makes this `camera.pos + camera.vel*32`). Only the opponent
team block is re-targeted; the human's selected record/control are never moved.
A `0x400` hysteresis keeps a target already near the anchor, so the old code-5
record is not abandoned each frame (P8-L5).

**T1 — far-carrier repair (P8-L2 stand-in).** Row 05 maps the native possession
block `[0x158724]` onto `ball.carrier` with no distance gate
(`0x7F1BF..0x7F205`); the recon probe measured an opponent claim at ~1127 units
(carrier 20 vs ball/target separation). `fifa96_match_action_05` now refuses a
non-controlled team's claim beyond
`FIFA96_MATCH_LIVE_OPPONENT_CLAIM_RANGE` (0x40) while live, and
`match_run_live_opponent_update` clears a stale opponent carrier beyond that
range. The possession-block reset travels with the claim, so a refused far claim
does not reset `pos_*`/release either. Human-team claims and every non-live run
keep the exact ported behaviour (M1/M2 never set `live_start`).

**T2 — live shot block / save (P8-L3/L4 stand-in).** No native open-play save
exists: the native goal scan `FUN_00088940` and mouth classifier `FUN_00070074`
are record-free, and the only native dive producer is the phase-6 penalty arm
(`0x8D558`). `match_run_live_shot_block` runs immediately before
`match_run_live_goal_arm` (so it sees the same frame-start shot the armer would)
and, while a live **KICK** flight (`live_kick_shot`, set only by the KICK row)
is in the final approach zone (`|ball.z| >= FIFA96_MATCH_LIVE_SAVE_ZONE` 0x900),
finds the nearest defending **outfield** record (1..10) within
`FIFA96_MATCH_LIVE_SAVE_RADIUS` 0x28 of the ball. On a hit it clears
`live_kick_active`/`live_kick_shot`, clears the FU-73 pair so the launch pair
cannot re-latch, pins the ball for `FIFA96_MATCH_LIVE_SAVE_HOLD` 30 frames
(ball-machine hold branch: `target = ball`, velocity/speed zero), records
`live_save_record` and gives that defender `ball.carrier`. It writes no score,
phase or goal cell; the native armer/scan keep those.

**Tuning evidence (windowed, not a claim of native save).** A keeper-inclusive
0x28 block made the shipped path unwinnable: the live keeper charges the
camera/ball and was measured at 0x1F from the ball at z 0x9D2 (temporary debug
tick 467, team 1 record 0, distance 31), so it blocked the centre shot every
time. A keeper-inclusive 0x18 variant also left a second windowed run 0-0 at
00:09 (the shipped centre shot unscored). The shipped block is therefore
**defender-only** (the task's allowed defender-only tuning); the keeper's own
open-play save stays P8-L3. Outfield
defenders use 0x28 and the shipped kickoff shot's final-zone outfield nearest
stays outside it (headless `test_live_shot_still_scores_wide` asserts the
minimum > 0x28 and 1-0). The block is a labelled stand-in, never a native call.

**Tests.** `port/tests/test_engine_live_control.c` gains five live cases:
`test_live_opponent_presses_and_claims` (staged opponent record 3 at 0x180
becomes the re-picked target 14, code 5, and reaches <0x80 of a held ball in 60
frames; score/phase untouched; human control stays on its team),
`test_live_opponent_far_claim_rejected` (claimant at 0x300 cannot set
`ball.carrier` during a live flight; the same record at 0x20 keeps it),
`test_live_defender_blocks_shot` (both sides: real kickoff, defender staged on
the ball in the zone -> `live_save_record` = the defender, no score/phase,
ball pinned within 0x28, 40 more frames still 0-0),
`test_live_shot_still_scores_wide` (shipped kickoff, min final-zone outfield
distance > 0x28 -> native 1-0, no save), and `test_live_shot_block_ignores_pass`
(a PASS latch sets `live_kick_shot == 0` and the full block condition does not
save). The existing kick/control/restart/side-switch tests are unchanged and
still pass; the P7 restart helper now accepts the legal loose-ball state the
far-carrier repair creates.

**Gate.** `make check` **119/119** (strict build + ASan/UBSan); M1
`09b726b7…` and M2 v29 `7c3874f7…` byte-exact by `cmp`; census unchanged
(**78/78** bodies, **58/80** natural, **64/80** install-live, action 38/45,
phase 26/35).

**Windowed evidence.** `./build/fifa96 data/game/FIFAPCCD96.iso`, DISPLAY=:1,
released ISO, Friendly Brazil/Italy, shipped input only, no harness. The first
Z at the kickoff clears the opening latch (0-0 at 00:02), the second fires the
row-07 kick/flight/native scan -> **BRA 1-0** at 00:05; the native goal frame is
`/tmp/opencode/play/p8-ai-goal.png` (1-0, ball at the +z goal area). The live
opponent contest is visible after the restart: `/tmp/opencode/play/p8-opponent-move.png`
(00:08, Italy players spread/moving around the ball) with
`p8-opponent-move-before.png` (00:07) as the movement reference. No save
occurred on the shipped defender-only path in this run, so `p8-save.png` is
**not** claimed/produced; the block is proven by the staged-defender test. The
match was then played to the terminal: half-time was released with C/PASS (the
P8-L4 sit-1 gate), and the second half ended at **FULL TIME, BRA 1-0 ITALY,
PERIOD 2** (`/tmp/opencode/play/p8-ai-fulltime.png`; `p8-ai-return-menu.png`
captures the Game Select return). The P8 full-time branch itself is unchanged.

**Residual legs.** **P8-L1** the native target-select gate clears and their row
producers; the faithful row-05 fallback `FUN_0007F7E0` (0x9D2 bytes, 11 callees,
only xref `0x7F36F`) remains P9. **P8-L2** the opponent possession range is a
derived 0x40; the native `[0x158724]` vs `ball.carrier` conflation stays the
root cause. **P8-L3** no native open-play keeper save/dive exists
(`0x88940`/`0x70074` record-free); the keeper is excluded from the block
because its live charge made the shipped shot unwinnable. **P8-L4** the ball pin
is the derived `live_save_hold`, not a native `FUN_0008BAF0` carrier-follow arm.
**P8-L5** target rotation hysteresis; old code-5 records are not demoted (row 03
still NULL) so more than one opponent can chase. **P8-L6** full 10-player
shape/possession AI still needs row 03 wiring (blocked M2) or the bounded
`FUN_0007F7E0` port; not claimed here. **P8-L7** the live camera follow now
keeps the ball focus during a human shot even while a native pan event is
active; this is required because the opponent contest emits a row-04 camera
event that otherwise froze the live focus and lost the native goal
(the unported aim/camera director remains the real fix). **P8-L8** native
aim/camera director (shot-side choice, keeper/defender beatability) remains the
eventual replacement for the KICK goal-centre aim and this block.

## P8 final playability acceptance (2026-11-06)

**Measured full loop.** Shipped `DISPLAY=:1`, Friendly Brazil/Italy, default
2-minute halves: one Z at the kickoff -> native **BRA 1-0** at 00:05; the live
opponent moves/contests and a defender can block (defender-only stand-in);
half-time is released with C/PASS (the native sit-1 gate); **FULL TIME BRA
1-0 ITALY, PERIOD 2**; Enter or the five-second hold returns to Game Select.
Captures: `/tmp/opencode/play/p8-ai-goal.png` (1-0, 00:05),
`p8-opponent-move.png`, `p8-ai-fulltime.png`, `p8-ai-return-menu.png`, plus
`p8-fulltime.png`/`p8-return-menu.png`; the shipped proof stages no score or
ball. `make check` **119/119**; M1 `09b726b7…`/M2 v29 `7c3874f7…` exact by
`cmp`; census unchanged (78/78, 58/80, 64/80, action 38/45, phase 26/35).
Gamepad A/South kick + B/East pass are live (menus keep confirm/back; the
keyboard Enter/Backspace dual role is documented and tested); the Controls
page states the dual roles and the half-time C/PASS gate.

**Doc errata (append-only).** (1) The two P8 registers above both use
`P8-L1..L5`: this note disambiguates the full-time branch as **P8-F1..F5**
and the opponent/block work as **P8-A1..A5**; earlier text remains
historical. (2) The Friendly full-time gate is keyed to `live_start`, not a
Game-Select match-mode cell; a future live competition mode must thread that
mode before relying on the native ET/penalties route (the earlier "any future
competition mode" wording is narrowed). (3) The block is defender-only and
the open-play keeper save remains P8-A3/P8-L3; the shipped windowed run did
not observe a natural save, only the staged-defender test proof.

**Remaining playable legs.** Open-play keeper save and native row-05 fallback
`FUN_0007F7E0` (P9), full 10-player AI/row 03 + possession hysteresis, native
aim/camera director, replay/captions, rosters/attributes, commentary/music,
competition ET/penalties, `options.dat` persistence, gamepad held-edge
polish, and the six other Game Select screens. Each is numbered; none is
claimed as complete.

## P9 — aimed human shots and a beatable live keeper save (FU-347, 2026-11-06)

**Answer in one line.** The human KICK now aims the goal-mouth flight with the
held lateral input (left `-0x80`, right `+0x80`, none/both centre), and a
labelled P9 keeper stand-in saves the centre shot while an aimed shot or a
displaced keeper still reaches the native goal chain. The match is contestable
and still winnable; no score/phase/goal cell is written by either stand-in.

**Aim (live-only, derived [D]).** `match_run_live_kick_latch` already replaced
the staged kicker-relative target with the opponent goal-mouth centre for the
KICK row. P9 consumes a new `live_aim_x` cell there. The native FU-70 slot only
publishes its released button word after every held key drops
(`fifa96_control_slot_update`'s `held`/`held_prev` logic), so at the launch
frame the lateral bit is already gone; `match_run_live_aim_update` samples the
held lateral input each frame into `live_aim_x` (left mapped `0x08` ->
`-FIFA96_MATCH_LIVE_SHOT_AIM_X` 0x80, right `0x04` -> `+0x80`, neither/both ->
0). A fresh KICK/PASS/LONG_BALL press with no lateral clears a stale aim, and
the cell is cleared by init/begin, a phase change and the restart rebind. The
goal-side `z` stays `±0xB30` and `target_y=0`; the ±0x80 target is inside the
`|x| < 0xD0` mouth. The native per-code target/loft and the aim/camera director
remain legs (P9-L1); this is still the P7/P9 labelled aim stand-in.

**Keeper save (live-only, derived [D]; P8-L3/P8-A3 stand-in).** No native
open-play save exists (`0x88940`/`0x70074` are record-free; the only dive
producer is the phase-6 penalty arm `0x8D558`). `match_run_live_keeper_save`
runs immediately **after** the P8 outfield shot block and before
`match_run_live_goal_arm`, so a defender on the ball keeps the block authority
and the keeper stops the centre shot the outfielders miss. It requires a live
phase-2 KICK flight that is not armed and not already inside the native mouth
classifier, a goal-mouth target (`|target_x| < 0xD0`, `|target_z| >= 0xB10`),
the defending side's record 0 visible (the native +0x98/+0x9A convention), the
keeper's x within `FIFA96_MATCH_LIVE_KEEPER_SAVE_REACH` (0x40) of the target x,
and `distance(ball, keeper) <= FIFA96_MATCH_LIVE_KEEPER_SAVE_NEAR` (0x180)
while `|ball.z| >= FIFA96_MATCH_LIVE_KEEPER_SAVE_ZONE` (0x9C0). The near-ball
gate is the honesty guard for the no-ISO headless fixture (record 0 starts at
the origin there); the shipped ISO keeper stands at `(0, ±0x9EA)` and the
centre path passes within ~0x30. On a hit the shared `match_run_live_save_apply`
clears `live_kick_active`/`live_kick_shot`, clears the FU-73 pair and the
consumed signature/pair marker (so an identical repeat can latch), pins the
ball for `FIFA96_MATCH_LIVE_SAVE_HOLD` (30) frames through the existing
ball-machine hold branch, records the keeper id in `live_save_record` and
gives it `ball.carrier`. It writes no score, phase or goal cell. The reach /
zone / near values are derived tuning, not native thresholds (P9-L3).

**Order and P8 compatibility.** `test_live_defender_blocks_both_sides` still
records the staged outfield defender (the P8 block runs first);
`test_live_shot_block_mouth_edge` still leaves a ball inside `FUN_00070074` to
the native armer; `test_live_shot_block_skips_hidden_defender` still ignores a
hidden outfield record (and the keeper's near gate is not met in that fixture);
`test_live_flight_end_stale_pair_guard` and
`test_live_shot_block_ignores_pass` are unchanged and green.

**Tests.** `port/tests/test_engine_live_control.c`:
`test_live_aim_both_cancel_to_centre` (KICK+Left+Right in one poll -> target
x=0), `test_live_keeper_saves_centre_side` (both sides; real kickoff, one
centre KICK, keeper pinned at the shipped goal spot -> `live_save_record` is
the defender's record 0, 0-0, phase 2, no `goal_armed`, ball pinned for the
hold), `test_live_keeper_beaten_by_aim_side` (side 0 Left `-0x80`, side 1
Right `+0x80` against the pinned keeper -> native goal, no save),
`test_live_keeper_displaced_centre_goal` (keeper held at x=0x400 -> centre
shot scores). Updated to aim: `test_live_kick_flight_scores` (+0x80),
`test_live_kickoff_shot_scores_side` (side 0 Right `+0x80`, side 1 Left
`-0x80`; keeper pinned at the goal spot), `test_live_goal_after_kick` (wide
aim before the staged mouth ball) and `test_live_shot_still_scores_wide` (aim
+ keeper pin; the P8 outfield final-zone minimum still exceeds 0x28). The
end-to-end `test_engine_match_completion` case is now
`test_live_aimed_kickoff_goal_to_full_time`: one KICK+Right at the real
kickoff -> native goal -> half-time -> second half -> FULL TIME 1-0. The
control page (`fifa96_menu_model.c`) and README state "L/R + Z AIM; CENTRE =
KEEPER"; `test_engine_menu_model` pins the new line.

**Gate.** `make check` **119/119** (strict build + ASan/UBSan); M1
`09b726b7…` and M2 v29 `7c3874f7…` byte-exact by `cmp`; census unchanged
(**78/78** bodies, **58/80** natural, **64/80** install-live, 6 blocked,
action 38/45, phase 26/35). No new port body or dispatch slot; the census
denominator is unchanged.

**Windowed evidence (shipped `DISPLAY=:1`, Friendly Brazil/Italy, default
2-minute halves, no score/ball staging).** A centre **Z** at the kickoff gives
the P9 save: `/tmp/opencode/play/p9-save.png` (0-0 at 00:06, the ball at the
defending keeper's feet in the goal area; the same run was observed 0-0 at
00:05-00:27, whereas the pre-P9 centre path scored at 00:05). A fresh launch
with **Z + Right** gives the native goal: `/tmp/opencode/play/p9-aim-goal.png`
(**BRA 1-0 ITALY** at 00:05). That aimed run then went through the native
half-time gate (released with repeated KICK/C pulses - the P8-L4 sit-1 gate)
and the second half to `/tmp/opencode/play/p9-fulltime.png` (**FULL TIME, BRA
1-0 ITALY, PERIOD 2**). The save and goal screenshots are from separate
launches: after a save the ball is with the defending keeper and a live tackle
requires a chase, so the aimed proof is the fresh kickoff path. Both runs use
the shipped input path only; no score or ball is staged.

**Doc errata (append-only; earlier P7/P8 text remains historical).** (1) The
P8 report's "first Z clears the opening latch, the second fires the row-07
kick" is superseded by measurement: the kickoff KICK press/release **is** the
shot and also releases the opening latch (`live_kickoff_touch`); a second Z
during the live flight is ignored, so the windowed proof uses one aimed Z.
(2) P8-L3 / P8-A3 ("no keeper save; keeper excluded") is addressed by the P9
labelled keeper stand-in; the **native** open-play save remains unported.
(3) The P8 `test_live_kickoff_goal_to_full_time` name is now
`test_live_aimed_kickoff_goal_to_full_time`; the old name referred to the
centre-shot case, which a centre shot no longer is.

**Residual legs.** **P9-L1** the native per-code target/loft algebra and the
aim/camera director (the real shot-side choice); **P9-L2** the native
open-play keeper save/dive body (P8-L3/P8-A3); **P9-L3** the keeper reach
0x40, save zone 0x9C0 and near gate 0x180 are derived tuning, not native
thresholds; **P9-L4** the `live_aim_x` sampling is a derived bridge for the
FU-70 release-word timing; **P9-L5** the half-time sit-1 gate still needs the
KICK/C pulses in the windowed path (P8-L4); **P9-L6** the live tackle/claim
after a save still needs a chase (the P7/P8 contact rules are unchanged).
Row-05 `FUN_0007F7E0`, row 03 and the full 10-player AI remain P9 open legs;
none is claimed complete.

## P9 — held long-ball edge (FU-347, 2026-11-06)

**Answer in one line.** `SDL3_HELD_CODES` stopped the backend's per-poll
held-state sample at `FIFA96_ENGINE_KEY_PASS + 1` (10), so
`FIFA96_ENGINE_KEY_LONG_BALL` (10, keyboard V / pad West) was never tracked by
`sdl3_key_hold` nor re-presented by the held loop. A long-ball press reached
`fifa96_match_run_input` only on its KEY_DOWN poll; the next poll carried no
0x40, `fifa96_input_update` derived a release, and the FU-75 long-ball request
behaved as a one-shot pulse instead of holding like KICK/PASS.

**Fix.** The bound is now `FIFA96_ENGINE_KEY_LONG_BALL + 1`, so the existing
state-sample path covers the whole match action block (KICK 0x10, PASS 0x20,
LONG_BALL 0x40). A held V/West is re-presented with state 1 on every poll,
KEY_UP still yields exactly one state-0 release edge, and menus/front-end
routing are untouched: LONG_BALL has no front-end mapping (the front-end
consumes CONFIRM/DECLINE only).

**Test.** `test_engine_sdl3::test_held_long_ball_is_a_state_sample` pushes
Z/C/V KEY_DOWNs into the dummy-driver queue (no physical pad), asserts all
three codes are re-presented on every poll, then asserts V's KEY_UP produces a
single state-0 edge and later polls carry only KICK/PASS. At BASE the second
poll carried two codes, so the test fails.

**Gate.** `make check` **119/119** (strict build + ASan/UBSan); M1
`09b726b7…` and M2 v29 `7c3874f7…` byte-exact by `cmp`; census unchanged
(**78/78** bodies, **58/80** natural, **64/80** install-live, action 38/45,
phase 26/35).

## P9-fix2 — aim axis and keeper goal-side gate (FU-347, 2026-11-06)

**Answer in one line.** The P9 aim used Left/Right (the goal-side z movement
axis) for the goal-mouth width (world x); the live view/native movement maps
**Up -> +x / Down -> -x**, so P9-fix2 moves the aim to Up/Down. The keeper save
also gained a goal-side gate (no save once the ball is past the keeper), and
the aim now samples a real action/depth rising edge so a new unaimed kick
cannot inherit a stale ±0x80.

**Major 1 — aim axis (P9-L7 errata).** Fresh review evidence (render axis probe
at yaw `0xBF9C`, live eye `camera.x-1920`: world `+z` projects **screen-left**,
`-z` screen-right, world `+x` is farther/higher; live movement: `Up -> pos_x
+0x60`, `Down -> -0x60`, `Left -> pos_z +0x60`, `Right -> -0x60`). The P9
Left/Right -> world-x mapping was therefore the camera depth axis, not the
mouth-width axis. `match_run_live_aim_update` now reads the depth bits
(`held & 0x03`): Up `0x01` -> `x=+0x80`, Down `0x02` -> `x=-0x80`, none/both
-> `x=0`; Left/Right remain the goal-side z movement keys and are not aim
inputs. The goal-side target stays `z=side==0 ? +0xB30 : -0xB30`,
`target_y=0`, and ±0x80 stays inside `|x| < 0xD0`. The controls page now reads
`UP/DOWN + Z  AIM SHOT` / `CENTRE SHOT = KEEPER'S`; the README states the same
native axis.

**Major 2 — keeper goal-side gate.** Before the lateral/near tests, a keeper
whose z is already past the ball cannot save: for a +z goal require
`keeper.pos_z >= ball.z`; for -z require `keeper.pos_z <= ball.z` (inclusive
equality allows a smother at the feet). This stops the P9 save from pinning a
close-range shot after the ball has passed the keeper. The P8 outfield block is
unchanged (its shape predates P9; not copied into the keeper path).

**Minor 3 — stale aim.** The old `input.fresh` clear was dead whenever another
key was held (`fifa96_input_update` zeroes `fresh` when the previous poll was
non-empty). The aim now latches on a real action-button rising edge
(`live_aim_prev_input = held & 0x73`) or a depth-key edge while an action is
held, and is consumed by the KICK latch and cleared on save/flight-end. A new
KICK pressed with no depth bits recomputes the centre even while another
direction is held. A two-kick fixture aims the first shot Up, stops the flight,
then presses KICK+Left; the second target is 0, not the stale +0x80.

**Minor 4 — evidence.** The P9 `p9-save.png` claim was replaced by a fresh
windowed capture, `p9-save2.png`: shipped `DISPLAY=:1`, Friendly Brazil/Italy,
one centre Z at the kickoff, 0-0 at 00:06, the ball at the defending
green/white keeper in the penalty area (the deterministic
`test_live_keeper_saves_centre` remains the primary proof). The old screenshot
is superseded; the old P9 sentence that described it as "at the defending
keeper's feet" was not a reliable frame. The one-Z wording is now the actual
capture script: one centre Z is the shot (and releases the opening latch); no
second Z is needed. The aim run uses one Z+Up.

**Minor 5 — coverage.** New/updated live tests:
`test_live_aim_depth_axis` (Up +0x80, Down -0x80, both 0, Left/Right 0,
both-held-before-KICK 0), `test_live_keeper_goal_side` (+z keeper 1 and -z
keeper 0: ball 0x9C0 saves, 0x9F0/0xA40 do not save and the native mouth chain
converts), `test_live_keeper_mouth_edge` (natural near keeper and a keeper
staged goal-side of the mouth ball; the mouth gate is the isolated early-out),
`test_live_stale_aim_two_kicks` (aimed then KICK+Left -> centre), and the
existing centre-save test now asserts `ball.carrier == keeper_id`. The kickoff
test covers side 0 Up/+0x80 and side 1 Down/-0x80; the keeper-beaten test
covers side 0 Down and side 1 Up, so all four side/direction combinations run.
`test_engine_match_completion`'s end-to-end case uses KICK+Up.

**Gate.** `make check` **119/119** (strict build + ASan/UBSan); M1
`09b726b7…` and M2 v29 `7c3874f7…` byte-exact by `cmp`; census unchanged
(**78/78** bodies, **58/80** natural, **64/80** install-live, 6 blocked,
action 38/45, phase 26/35). No new body or dispatch slot.

**Windowed evidence (shipped `DISPLAY=:1`, Brazil/Italy, default halves, no
score/ball staging).** `p9-save2.png`: one centre Z, 0-0 at 00:06 with the ball
at the defending keeper in the penalty area (the shot did not score). A fresh
launch with **Z + Up** gives `p9-aim-goal2.png` (**BRA 1-0 ITALY**, the score
updates by 00:06; the frame at 00:05 is still 0-0 as the goal chain completes).
That aim run then released the post-goal kickoff (Z) and the half-time sit-1
gate (long C+Z pulses; the sit-1 gate still needed repeated pulses) and
reached `p9-fulltime2.png` (**FULL TIME, BRA 1-0 ITALY, PERIOD 2**). The save
and aim screenshots are separate launches: after a save the ball is with the
defending keeper and a live chase is needed; the aimed proof is the fresh
kickoff. The old `p9-save.png`/`p9-aim-goal.png`/`p9-fulltime.png` are
superseded by the `…2.png` set.

**Doc errata (append-only; the P9 section remains historical).** (1) The P9
"mapped Left/Right -> world x" aim is superseded by Up/Down: world x is the
mouth-width axis and Left/Right are the goal-side z axis. (2) The P9
`p9-save.png` evidence sentence is superseded by `p9-save2.png` and the
deterministic save test. (3) The P9 one-Z wording is confirmed by the new
capture (one Z is the shot); the P8 "first Z clears, second fires" remains an
older narrative, not the shipped path.

**Residual legs.** P9-L7 the aim is still a world-x target band, not the
native per-code target/loft or aim/camera director; P9-L8 the native
open-play keeper save/dive remains unported (this is the labelled stand-in);
P9-L9 the goal-side gate is a plain current-z test, not a native dive reach;
P9-L10 the half-time sit-1 gate still needs repeated KICK/C pulses in the
windowed path; row-05 `FUN_0007F7E0`, row 03 and the full 10-player AI remain
open.

## P9 final acceptance and evidence erratum (2026-11-06)

**Shipped loop.** `DISPLAY=:1`, Friendly Brazil/Italy, default halves: a
centre Z at the kickoff is saved by the live keeper (**0-0** at 00:06); hold
Up or Down with Z to aim at ±0x80 along the goal-mouth width; Z+Up scored
**BRA 1-0** at 00:05; the match reached **FULL TIME BRA 1-0 ITALY, PERIOD 2**.
The keeper goal-side gate, stale-aim clearing, aim-axis/side cases and the
coverage asserts are in the deterministic suite. `make check` **119/119**;
M1 `09b726b7…`/M2 v29 `7c3874f7…` exact by `cmp`; census unchanged (78/78,
58/80, 64/80, action 38/45, phase 26/35).

**Erratum (append-only; earlier P9 text remains historical).** The P9 and
P9-fix2 sentences that `p9-save.png`/`p9-save2.png` show the ball at the
defending keeper are corrected: both captures show 0-0 at ~00:06 from
midfield and do not show the save. The deterministic
`test_live_keeper_saves_centre` (keeper carrier/pin, no score) is the save
proof; the screenshots corroborate only the 0-0 score. The 15-line Controls
help block now clears the footer via `MENU_OPT_HELP_PITCH 13`.

**Remaining legs.** Native aim/camera director and per-code target/loft
(P9-L7), native open-play keeper save/dive and reach (P9-L8/L9), repeated
half-time sit-1 pulses (P9-L10), row-05 `FUN_0007F7E0`/row 03/full AI,
Practice/Leagues/Tournaments/Playoffs/Load Game/Modem Setup, replay/captions,
rosters, commentary/music. Each is numbered; none is claimed native.

## P9 code-review fixes and errata (2026-11-06)

**Process record.** The P9 implementation ran before its SDD spec/plan; the
`2026-11-06-fifa96-p9-shot-contest-{design,plan}.md` files were added after
the P9 code review found the missing process record. The implementation order
is recorded there.

**Doc errata (append-only).**
- The P9 prose citing `test_live_aim_both_cancel_to_centre` and
  KICK+Left+Right is corrected: the test is `test_live_aim_depth_axis`; the
  aim axis is Up/Down (Left/Right are goal-side movement).
- The 1-0 capture is at 00:06; the 00:05 frame is 0-0 before the shot; the
  goal/full-time result is unchanged.
- The keeper goal spot `(0,±0x9EA)` is the P8 AI recon/windowed probe
  (keeper `(0,2706)` -> `(0,2538)`, shot distance `0x2F`), now cited in
  `fifa96_match_run.h`, not a static formation constant.
- `p9-save2.png` shows 0-0 from midfield, not the ball at the keeper; the
  save proof is the deterministic `test_live_keeper_saves_centre` /
  `test_live_keeper_saves_centre_no_staging`; the screenshot only
  corroborates the 0-0.
- The save pins the ball for 30 frames and does not install keeper
  possession; the post-save possession install is P9-L11. The half-time
  sit-1 gate remains pulse-gated (P9-L10).

**Review fixes applied.** `match_run_live_team_for_side` replaces the
duplicated side scans; `match_run_live_aim_reset` unifies the latch/save/
flight-end resets; input masks and goal-mouth bounds are named; the unused
test out-param is removed; `live_aim_x` is renamed `live_aim_goal_x`; the
adjacent live guard is merged; a no-staging keeper-save test and a
match-level held-long-ball test were added. `make check` **119/119**; M1
`09b726b7…`/M2 v29 `7c3874f7…` exact; census unchanged (78/78, 58/80,
64/80).

## Phase 61 — P10 graphics: native 640x480 shell and broadcast match view (2026-11-06)

**Why.** The play-test rejection was visual: the front-end drew HELV12 labels
over the native background and solid yellow selection rectangles, while the
match used the VGA camera-0 rig across the pitch (a wide blue sky, no stadium,
tiny squashed sprites). The assets were already in the ISO; the shell was not
composing them.

**Measured assets (retail ISO, first-hand POG decode this slice).**
`/FEGFX/MOD2.POG` is a 27-frame SHPI bank: frame 0 is the **640x480** Game
Select background (the menu bars are part of the art), frame 1 is the 300x48
selected-row bar. `/FEGFX/MOD1.POG` frame 0 is the 640x480 Team Select
background; `/FEGFX/MOD4A.POG` frame 1 is the 120x91 FIFA globe (the flags are
frames 3..10). `/FEGFX/VS.POG` is 13 80x80 animated VS frames (transparent
key `0xFF`). `/FEGFX/SLICK64.POG` and `/FEGFX/SLICK18.POG` parse as FNTI
fonts (70x59 and 20x18 glyphs), the native title/menu faces; `HELV12` remains
the HUD/fallback font. `MOD4A` uses its own bank palette, so the globe is
remapped once onto the MOD1 palette at load.

**Landing.** `fifa96_menu_art` gains a hires shell (`hires_ready`): Game
Select composes the MOD2 background + the native bar + SLICK64 "GAME SELECT"
(scaled-mask blit) + SLICK18 item labels + the animated VS mark; Team Select
composes MOD1 + SLICK64 "FRIENDLY" + the stacked LEAGUE SELECT column + two
FIFA globes + the SLICK18 team lists; Options/Controls use the MOD1 shell and
the same font. The shell is built on a 640x480 canvas and sampled to the
engine surface (nearest); the legacy 320 composition is kept for ISO-less and
hash-pinned paths. `test_engine_menu_art` still proves native != fallback.

**Match view (derived [D], live_start only).** The goal mouth is on the z
axis (`live_kick_target_z = ±0xB30`), so the old camera-0 yaw looked across
the pitch. The live eye now sits behind the controlled side's goal
(`focus + (30, 320, ∓2000)`) and looks down the pitch (`yaw 0x0300` side 0 /
`0x8300` side 1, pitch `0x0700`), with a 3/4-width lens (aspect preserved;
the old half-width lens squashed sprites). The 22 player slots draw at 3x
the projected scale; the ball keeps native scale; a dark ground shadow is
drawn at each live pivot. `fifa96_pitch` gains a live 75%-brightness shade
map for 128-unit mowing stripes, a scaled crowd band with a stand roof and an
advertising wall, and an extended floor depth (`0x4000`) so the far pitch
reaches the stand instead of a black band. These are explicitly derived
stand-ins for the unported native camera director / 3dwpts stadium / goal-net
passes, not native camera-0 output.

**Gate.** `make check` (final run) and M1/M2 remain the authority; the changes
are live-gated, so the headless tapes are untouched. `test_engine_match_render`
now pins the live eye `(+30, +320, -2000)` and the down-pitch yaw. The engine
surface is still 320x240; the 640 shell is downsampled. Open legs:
**P10-L1** the windowed 640x480 surface (crisp shell/HUD; the null backend
still hashes 320x240 tapes), **P10-L2** the native 3dwpts stadium / goal-net
renderer, **P10-L3** Team Select flag mapping (MOD4A/4B/4C), **P10-L4** the
native Options art, **P10-L5** HUD geometry/scale at 640, **P10-L6** the
native camera director/aim (the broadcast rig and sprite scale are stand-ins).

## Phase 62 — P10 graphics follow-up: native 640x480 windowed surface (2026-11-06)

**Errata.** Phase 61 said: *"The engine surface is still 320x240; the 640 shell
is downsampled. Open legs: **P10-L1** the windowed 640x480 surface (crisp
shell/HUD; the null backend still hashes 320x240 tapes)"*. That is superseded.
The windowed executable now requests a 640x480 surface (`fifa96_main.c`), the
surface backing store is 640x480 (`fifa96_surface.h`), and SDL opens a 1280x960
(2x) window. The headless/tape path still requests 320x240;
`fifa96_surface_hash` now hashes `width * height` indexed pixels plus the
palette, so the 320 tapes keep their pinned hashes. **P10-L1 is closed.**

Phase 61 also said *"The 22 player slots draw at 3x the projected scale"* and
*"a live 75%-brightness shade map for 128-unit mowing stripes"* and the crowd
was the 64x64 frame. The final values are 2.5x player scale, a 90%-brightness
shade map at 64-world-unit stripes, and the 128x128 `STACcovr` frame when the
bank carries it (64x64 fallback). The 640 menu shell now takes the 1:1
`hires_present` path, so no downsample is applied in the windowed build. The
HUD is drawn with an x-scale of 2 (wide, short bar); its y layout stays at the
320 spacing because the 640 SVGA HUD art is not staged. Shadows now darken the
existing grass through the pitch shade instead of painting a flat black patch,
and the pitch line half-width is 6 world units. The 320x240 intro stream now
scales to the windowed surface (nearest fill); the 320 tape path keeps the
original 1:1 top-left blit, so the M1 intro frames are unchanged.

**Remaining legs.** **P10-L2** native 3dwpts stadium / goal-net renderer (the
stands are still the derived crowd band + roof/ad wall), **P10-L3** Team Select
flag mapping (MOD4A/4B/4C), **P10-L4** native Options art, **P10-L5** native
640 HUD art (the current wide bar is an x-scaled 320 bar), **P10-L6** native
camera director/aim. The broadcast camera, sprite scale and stadium layers
remain explicitly derived stand-ins.

**Gate.** `make check` **119/119**; M1/M2 exact by `cmp` remain the authority.
The surface-size change is windowed-only; the M1/M2 tests continue to request
320x240 and the hash change is byte-compatible with the pinned frames.

## Phase 63 — P10 gameplay camera/stadium correction (2026-11-06)

**Errata.** Phase 62's *"The broadcast camera, sprite scale and stadium layers
remain explicitly derived stand-ins"* and Phase 61's *"The live eye now sits
behind the controlled side's goal ... yaw 0x0300 side 0 / 0x8300 side 1, pitch
0x0700 ... a 3/4-width lens ... the 22 player slots draw at 3x"* are superseded
for the windowed live view. The native view object is camera 0 (`[0x107DC8] =
0x107508`; first-hand `read_memory 0x107510` = `80 f8 ff ff a0 00 00 00 1e 00
00 00` -> position (-1920,160,30); `read_memory 0x107560` = `9c bf 00 00 c8 00
00 00` -> yaw 0xBF9C, pitch 200). The live eye is now `focus +
(-1920,160,30)` with `r->yaw`/`r->pitch` (the FU-154 statics already defined as
`MATCH_RUN_VIEW_YAW`/`MATCH_RUN_VIEW_PITCH`), the full 640x480 lens and the
native sprite scale. The P10 behind-goal offset `(30,320,-2000*dir)`, its
0x0300/0x8300 yaw, 0x0700 pitch, 3/4 lens and 2.5x live player scale are
removed; they produced the empty foreground and tiny far players in the
previous p10g-match-view screenshot.

**Stadium.** The native 3dwpts bowl is still unported (**P10-L2**); the retail
attract footage shows the real tiered bowl and goal-net mesh. The live pitch
now draws a derived far-stand wall at `x = +0x7E0 + 64` using the real
GAMESTD1 banks: `STARcovr.fsh` (64x128 seating rows), `STAScovr.fsh` (194x87
EA Sports hoarding) and `STABcovr.fsh` (128x16 banner), plus the existing
128x128 `STACcovr` crowd. The wall is inverse-projected per pixel through the
same projection matrix as the floor, so the stand base, tiers and roof follow
the camera-0 view. `test_engine_match_render` asserts the stand/ad/banner banks
are staged and pins the camera-0 eye. This remains explicitly derived: the
native 3dwpts point/quad renderer (`FUN_000610C4` and the
`0x15438C`/`0x1543D4` projected arrays) and the goal-net pass are still the
P10-L2 open leg.

**Gate.** `make check` **119/119**; M1/M2 exact by `cmp` remain the authority.
Only live_start pixels changed; the headless 320 tapes and the pinned goldens
are untouched.

## Phase 64 — P10 review corrections: pitch axis, bank provenance, leg IDs (2026-11-06)

**Errata — the pitch axes.** Phase 63 and the v1 `pitch_marking` comment
inherited A1 §4's labels *"goal lines x = ±0x7E0 = 2016, touchlines z =
±0xC1E = 3102, halfway line x = 0"*. The goal code is the authority:
`live_kick_target_z = ±0xB30` and `fifa96_match_goal_zone` test |z|
(`fifa96_match_run.c`), while the native camera-0 eye sits at x = -1920. So
**x is the touchline/width axis and z runs goal-to-goal**: touchlines
x = ±0x7E0, goal lines z = ±0xC1E, halfway line z = 0, and the far-stand wall
at x = +0x7E0 + 64 is behind the far touchline. The `pitch_marking` bands and
comments are corrected; the old A1 §4 x/z labels are errata, not a second
authority. The live pixel test now finds the halfway line as a vertical column
away from the frame edges.

**Bank provenance.** The `STARcovr`/`STAScovr`/`STABcovr` dimensions are from
the decoded `/ART/GAMESTD1.PVI` (refpack selector 0x10, declared length
118069): BIGF directory `STACcovr.fsh` off 0x75 size 64744, `STARcovr.fsh` off
0xFD5D size 8232, `STABcovr.fsh` off 0x11D85 size 27616, `STMCcovr` off
0x18965 size 440, `STAScovr.fsh` off 0x18B1D size 16920. The port's SHPI
parser selects STAR 64x128, STAS 194x87 and STAB 128x16 (STAC 32/64/128).
`test_engine_match_render` asserts the retail staging and that removing the
stand banks changes the rendered frame (the wall is consumed, not dead).

**Legs.** The far-stand wall's plane/heights/UV spans/tint thresholds and roof
are the derived **P10-L7** leg (the native 3dwpts point/quad renderer
`FUN_000610C4` with the `0x15438C`/`0x1543D4` arrays remains **P10-L2**). The
native camera director/aim and lens remain **P10-L6**: the live pose uses the
native camera-0 statics and the full surface focal; the earlier half-focal and
3/4 live lenses were derived framing workarounds, not native values. The HUD
is still **P10-L5**. The P10-L2, P10-L5 and P10-L6 legs are unchanged by this
correction.

**Contract.** The stand/ad/banner banks are optional at stage time; the wall
draws from any of them, and the old screen-space band remains the fallback
when none decode. The early no-floor branch now still invokes the wall. The
`staged` contract is floor + crowd required, stand banks optional.

**Gate.** `make check` **119/119**; M1/M2 exact by `cmp` remain the authority.

## Phase 65 — P11: playable Release build + kickoff placement (2026-11-06)

**Why.** The windowed `fifa96` executable was linked with ASan/UBSan like the
test tree. The P10 pitch caster calls `pitch_ray` once per pixel; at 640x480
that is ~300k instrumented calls per frame, and ASan's fake-stack allocation
dominated the frame. The live match ran at ~2 fps, so the 30-frames-per-second
match clock appeared stuck at `00:00` and the game was a slideshow. A second
defect: the live begin skipped the native kickoff cutscene and seeded the
nearest outfield record (`0x199` from the ball) without moving it, so the
opening KICK had no carrier.

**Build split.** `port/CMakeLists.txt` gains
`FIFA96_SANITIZE_ENGINE` (default ON). The engine, `fifa96_pccd` and the
`fifa96` executable use conditional sanitizer flags. `make check` keeps the
instrumented `build/` tree; `make game` and bare `make run` configure
`build-release/` with `CMAKE_BUILD_TYPE=Release` and the option OFF. This is a
build-topology change only: the test gate still compiles and runs the engine
with ASan/UBSan, while the playable executable does not pay the per-pixel
fake-stack cost.

**Kickoff placement.** `match_run_live_control_place_taker` puts the seeded
taker on the ball and faces it at the opponent goal. The live seed calls it
after a successful FU-70 slot/actor/carrier bind; the phase-1 → phase-2 restart
bind now seeds the carrier (`seed_carrier=1`) and uses the same placement
(human restart possession is a labelled [D] stand-in for the unported
conceding-side split). Both are gated on `formation_seeded`: the retail
`.fmt` seed moves the pool and needs the cutscene placement; the no-ISO
synthetic fixtures keep their legacy zero-position/control flow. The clock
itself is phase-class driven and is not gated on the touch; the placement is
what makes the opening KICK possible.

**Verification.** `test_engine_match_render` asserts the taker's position is
the ball position after a live begin. The release play-test trace reaches 355
frames and clock `00:11` after an 11 s run (previously 8 frames / `00:00`);
the committed `docs/screens/p11-match-playable.png` shows the taker on the
ball, the running HUD and the camera-follow view. The sanitized `make check`
gate remains the authority; M1/M2 are untouched by the live-only placement.

**Remaining legs.** The native 3dwpts stadium/goal nets are **P10-L2**, the
native camera director/aim **P10-L6**, the HUD art **P10-L5**, the full CPU
AI/dribble/pass the P9 row-05 legs, and the other front-end screens **P2-L1**.
The release build does not change those claims.

**Gate.** `make check` **119/119**; M1 and M2 exact by `cmp`; the Release tree
builds `fifa96` with no ASan/UBSan runtime dependency.

## Phase 66 — P12: native match camera type 0 and the goal mesh (2026-11-07)

**Errata — the live camera pose.** Phase 63/64 and the P11 playable build used
camera object 0's image defaults (`0x107508`: `-1920,160,30`, yaw `0xBF9C`,
pitch `200`, ratio `0x1500`) as the live eye. That is the attract/pre-match
pose. Fresh Ghidra `disassemble_function 0x4D7E8` shows the camera-type setup
copying `PTR_DAT_00108B64[type] + 0x48 + index*0x18` into the camera object;
the type-0 table at `0x1089B4` points at `0x107E2C`, and `read_memory
0x107E2C` is `9c ff ff ff e8 01 00 00 4c 04 00 00 6c 84 00 00 3c 0f 00 00
00 12 00 00` = `{-100, 488, 1100, 33900, 3900, 4608}`. The live begin now
installs that yaw/pitch/ratio and the eye is the type-0 offset from the FU-71
follow focus. The image defaults stay on non-live/tape runs, so M1/M2 are
byte-identical. The derived ball-follow and the native `FUN_0004E834` director
remain **P10-L6**.

**Goal mesh.** The derived goal pass stages `GAMEART0` entry 36 `Net.fsh`
frame 4 (64x64, gray mesh; 187 background, >=200 white cords) and
inverse-projects the goal-line plane `z=±0xC1E` through the same camera matrix
as the floor. The goal mouth is the native classifier's `|x| < 0xD0`, height
`0xA0`; the interior is dark, the cords/posts/crossbar use the white marking
slot. This is a derived back-net plane; the native 3D goal-net/side/top passes
remain **P10-L2**.

**Stadium.** The far goal-end wall at `z=-(0xC1E+64)` uses the same derived
STAR/STAS/STAB/STAC materials as the far touchline wall, closing the black
void behind the goal. The native 3dwpts point/quad bowl (`FUN_000610C4`,
`0x15438C`/`0x1543D4`) remains **P10-L2**; the wall material/tiers remain
**P10-L7**.

**Verification.** `test_engine_match_render` pins the type-0 eye/angles and
the staged 64x64 net frame; the live wall hash still proves the banks are
consumed. The release screenshot `docs/screens/p12-match-goal-net.png` shows
the goal net inside the end stand. **Gate:** `make check` **119/119**; M1/M2
exact by `cmp`.

## Phase 67 — P13: complete single-player playable wave (2026-11-07)

**Outcome.** The shipped Release build now exposes every Game Select entry as a
functional single-player flow, closes the live-match softlocks found by the
release smoke, and renders the 640 HUD at a uniform 2x. This is a functional
completion, not a claim that the unported native competition art/data or the
native 3dwpts/camera/HUD art is byte-faithful.

**Live match fixes (all `live_start`-gated; M1/M2 byte-identical).**
- Row 05's camera target is overridden for the human carrier with a bounded
  `pos + held-dir*0x80` target (the native stage-0 algebra remains OL-63);
  the player can now run with the ball instead of kick-and-chase.
- The live goal armer is ball-based: goal mouth, touchline (`|x|>0x7E0`) and
  goal line/corner (`|z|>0xC1E`) select the native throw/corner/goal-kick
  situations. A live shot whose high-speed sample skips the mouth band keeps
  its in-mouth target latch. The shared camera-pan armer stays for non-live
  runs; live only permits the goal-mouth case, so it cannot manufacture a
  restart from an in-play ball.
- A bounded direct-restart fallback places the ball/taker and returns to phase
  2 when a native taker row (notably goal kick 0x1D stage 1) has no slot. It
  writes no score/goal cell; the native row remains preferred.
- Half-time shows a panel and, after 3 s, injects one KICK press/release
  through the native sit-1 gate; manual KICK/PASS/Z still starts immediately.
  A pure wait no longer soft-locks at 02:00.
- The 640 live HUD uses uniform 2x bar/text geometry (120x82 at (4,394)); the
  320 tape path remains 1:1 and byte-identical.
- `fifa96_match_run_end` preserves `final_score[2]` for OVER/POST or a staged
  full-time POST->EXIT (`result_pending`); a pause/abandon exit is not a
  result. `fifa96_match_run_set_mode` installs the native mode byte and the
  Practise `[0x14C32A]` gate.

**Single-player modes (derived [D]; native mode bytes 1..4).** New
`fifa96_competition` model: an eight-team round-robin League (28 fixtures,
3/1/0 table), eight-team Tournament and Playoff brackets (replay twice, then
deterministic shootout; user loss simulates the remaining bracket), and a
no-fixture Practise descriptor. New `fifa96_save` writes ten distinct
`F96SAVE%.2d.DAT` slots under `data/game/hdd` (magic/version/length/CRC32) and
never overwrites native `SGAME` files. The front-end adds the hub, table/
bracket, Load Game, Save Game and Modem notice screens, enables
Leagues/Tournaments/Playoffs/Practise/Load Game/Modem Setup, and preserves the
competition/settings across the post-match reset. The engine captures the
preserved result, applies it to the fixture, auto-saves slot 0, and returns to
the hub; Friendly returns Game Select, Practise returns Game Select. Modem
Setup is an honest offline notice: native modem transport is not ported.

**Tests.** New `test_engine_competition`, `test_engine_save`,
`test_engine_frontend_modes`, `test_engine_mode_flow`; extended live-control
(dribble/touchline/goal-line/inside-ball), match-render (640 HUD) and
menu-model tests. `test_engine_mode_flow` also stages the engine's completed
POST->EXIT result and proves the fixture is applied, slot 0 is written, and
the front-end lands on the hub. **Gate:** `make check` **123/123**; M1/M2
exact by `cmp`; Release `make game` smoke covers the menu, League kickoff,
half-time/second half, hub, Load Game, slot 0 load, Simulate next, Table/
Bracket and Save Game. Evidence:
`docs/screens/p13-game-select.png`, `p13-league-select.png`, `p13-match-hud.png`,
`p13-league-hub.png`, `p13-league-table.png`, `p13-load-game.png`.
Tournament/Playoff/Practise/Modem are synthetic-flow tests; their long release
play-through is not claimed here. The native-fidelity legs below remain.

**Legs.** The native 3dwpts stadium bowl/full 3D net and native camera
director/lens remain **P10-L2/L6**; native HUD art **P10-L5**; the native
league/tournament/playoff screen art, fixture/table layouts, seeding/tie-break
rules and the native `SGAME` body remain new P13-L1/L2; native modem transport
P13-L3; full 10-player AI, native open-play keeper dive, fouls/offside live
proof, substitutions and the native penalty shootout remain the P7/P8/P9
legs. The derived competition modes and save records are explicitly not
native-faithful.

## Phase 68 — P14: match contest, bench and cup shootout (2026-11-07)

**Outcome.** The live match now has a contesting opponent, a keeper that
distributes, a pause substitution path, and a player-facing penalty shootout
for drawn cup ties. All new match behavior is derived [D] and `live_start` /
front-end gated; M1/M2 stay byte-identical.

**CPU and keeper (derived, live-only).** After a 30-frame possession hold the
opponent carrier passes to a forward teammate or shoots at the human goal
through the existing live flight/ball-machine primitive
(`match_run_live_cpu_kick`). A stationary human carrier can be won by a nearby
opponent with a 45-frame contact cooldown. The P9 keeper save now releases
through the same primitive to a teammate / the attacking half after the
30-frame pin (`match_run_live_keeper_outlet`). These are not native AI
chooser/keeper-distribution bodies; the native row-03 tail, `FUN_0007F7E0`
and the keeper distribution chain remain legs.

**Substitutions (derived).** While paused, S substitutes the first available
outfield roster slot with the next bench slot, at most three per side. PCCD
roster names/numbers are handed through the new
`fifa96_team_data_player`/`_roster` API and the match bridge; the pause panel
shows the swap (screenshot `docs/screens/p14-substitution.png`). The existing
R1 strip fields are populated, but the native `FUN_00041494` selection/timing
and the native pause-menu body remain legs.

**Penalty shootout (derived).** `fifa96_shootout` is a pure five-round
kick/dive model with deterministic CPU choices and sudden death to a 20-round
cap. A drawn Tournament/Playoff match now opens the front-end `SHOOTOUT`
screen instead of the silent hash/replay; the result is applied to the cup
fixture and auto-saved. Native extra time and a native shootout entry do not
exist in the proven image (FU-237); both remain legs.

**Referee.** The Options Fouls/Offside values still reach
`fifa96_match_config`, but the live contact-foul path is deliberately not
wired: the ported act-2 stage 2 holds at phase 0x0A for live kind-1 contacts
because the native `[0x158882]` release writer is missing. Offside likewise
still lacks a live-pass metric proof. Faking a whistle/restart was rejected;
the next referee slice must port the release producer. This is an explicit
leg, not a silent omission.

**Tests and gate.** New `test_engine_shootout`; extended
`test_engine_live_control` (CPU action, opponent tackle, keeper outlet,
three-sub pause), `test_engine_frontend_modes` (interactive cup decider),
`test_engine_menu_model` (PCCD roster API), SDL3 key map (S/North) and
match bridge roster handoff. **Gate:** `make check` **124/124**; M1/M2 exact
by `cmp`; release smoke covers the pause substitution with real PCCD names and
the match HUD. The native contact fouls/offside, native substitution
selection, native extra time/shootout, 3dwpts/camera/HUD art and native
competition data/save/modem remain the recorded legs.

## Phase 69 — errata: half-time / goal-restart gate (2026-11-07)

**Errata to Phase 67/68.** The late depth smoke was run against the stale P13
release binary, and it exposed a real gate bug that survived in the new code:
the HALF TIME panel was keyed on `state.period == 1`, so a second-half goal
restart (same period index) could draw the half-time panel and freeze. The
engine auto also injected KICK; the native sit-1/sit-8 gate takes the PASS/C
word, so the 3-second auto did not release.

**Correction.** `live_halftime_pending` is now set only at the actual
period-0→1 completion and cleared when phase 2 resumes. The panel is gated on
that flag, never on period alone. The engine auto uses a 10-step PASS hold for
both the half-time gate and goal/phase-0x0C kickoffs, and the match-run control
update has a 90-granted-frame fallback that calls the existing
`period_over_rearm` if input has not released the gate. Goal restarts no longer
show HALF TIME; half-time resumes without a manual key. The render test pins
panel-on-flag and panel-off-without-flag; the live full-time test pins the flag
at half-time.

**Evidence.** Current-release smoke: HALF TIME at 02:00
(`docs/screens/p14-half-time.png`), second half running after the dwell
(`docs/screens/p14-second-half.png`). The stale smoke’s CPU/keeper/old pause
observations are superseded by the P14 binary (CPU action, keeper outlet,
pause S, PCCD names). Contact fouls/offside and the camera/3dwpts art remain
the recorded legs.

**Gate.** `make check` **124/124**; M1/M2 exact by `cmp`.

**Addendum (same session).** The half-time fallback was not sufficient for a
second-half goal restart: the match-run now also has a 90-granted-frame
`live_kickoff_frames` force for phase 1 / the phase-0x0C kickoff flash when no
half-time flag is set. It writes phase 2 (not the period re-arm) and lets the
existing restart bind place the taker, so the half clock is not reset.
`test_live_goal_kickoff_auto_release` pins the phase-2 landing; the stale
smoke's 03:44 goal freeze is the failure this closes. `make check` **124/124**;
M1/M2 exact.

## Phase 70 — live referee rules (2026-11-07)

**Outcome.** Ordinary contact fouls now reach the ported free-kick/penalty
chain, the live opponent tackle runs the native contact decision, and live
offside uses the native reception target instead of the ball. The live referee
is no longer a fixture-only path.

**The missing release (erratum to the Phase 68 leg).** The old “`[0x158882]`
release writer is missing” claim is superseded by fresh evidence. The act-2
stage-3 gate at `0x894A2` really does hold on the immediate re-entry, but the
ordinary-match ball appearance pass releases it:

```
FUN_0008AF38 0x8B65B  CMP byte[0x14C32A],0
              0x8B662  JNZ 0x8B669
              0x8B664  CALL 0x8BAF0        ; ordinary session only
              0x8B669  ... CALL [0x158808] ; the act handler
FUN_0008BAF0 0x8C05A  CALL 0x6FE84         ; appearance machine
FUN_0006FE84 0x6FF67/0x6FF85 CALL 0x6E724(0x1587EC, 0x00)
```

The act-2 stage-2 body installs row `0x49` (flags `0x00` -> latch 0); the
row-`0x49` settle frames are `0x80/0x40/0x1C0` (`read_memory 0x10E9FC`, total
`0x280`), and the successor row `0x00` has flags `0x07` -> `[0x158882] = 1`.
The next act tick then takes `0x894A2 JNZ 0x894C4`. The engine models the
release with `ball_settle_timer`/`ball_settle_active`, ticked before the act
handler with the native `frame_delta << 4` (`0x6FFA5..0x6FFAE`); the visual
frame walk stays the L14.8.b leg.

**Live contact.** `match_run_live_contact_tackle` calls
`fifa96_match_run_contact(mr, 2, tackler, carrier, point)` before the derived
possession swap, with the challenged record's position (the native row-`0x0B`
arm-1 point `0x81BC3`) and an idle-act guard. A decision keeps the fouled
carrier’s ball and starts the ported act-3/act-8 sequence; a clean contact
still swaps possession. The 45-frame tackle cooldown is shared.

**Offside.** The own-nearest query is the reception target computed by
`fifa96_match_run_ball_receive` (the engine stand-in for native `0x157770`),
not the ball; at the synchronous receive the ball is still at the passer.
`match_run_offside_reception_at` threads the query and the public
`fifa96_match_run_offside_reception` keeps the ball-based direct-geometry
behavior. The `[0x157823]` mirror is now passed from the engine cell.

**Census.** Phase `0x06`, action `0x13` and action `0x1F` are naturally
reachable from the ordinary penalty fork; the blocked set drops 6 -> 3 and the
natural wired count **58/80 -> 61/80** (install-live stays 64/80). README and
the census test are updated in the same commit.

**Tests and gate.** `test_live_ai_tackle_foul_reaches_free_kick` drives the
live contact registration, the act-2 settle hold and the phase-7 taker;
`test_live_foul_reaches_penalty` pins the phase-6/row-0x13/0x1F fork;
`test_foul_taker_held_on_live_default` now asserts the release instead of the
old hold; `test_offside_reachable_from_live_receive` stages the ball at the
passer and the target on the own-nearest, so the ball-based query would fail.
**Gate:** `make check` **124/124**; M1/M2 exact by `cmp`.

**Legs.** The human-derived tackle still awards possession without a contact
decision (the native pad row-`0x0B` remains the input route); the visual
appearance frame walk, full 10-player AI, native 3dwpts/camera art and the
native substitution/tie-menu bodies remain open. The `0x8948D` decision-word
read stays FU166-L6.

## Phase 71 — live render-eye stabilizer (2026-11-07)

**Outcome.** The live view no longer teleports when a native pan hands back to
the derived ball-follow. The simulation camera stays the exact FU-71 cell; only
the rendered camera-0 eye is smoothed and clamped.

**Why.** `match_run_live_camera_follow` returns early while a pan is active and
then hard-assigns `camera.pos_x/z = ball + lead`; the rendered eye is
`focus + camera-0 offset`, so a pan handoff can move the eye by the whole
focus delta in one granted frame. The native FU-71 bounds are already in the
repo (`fifa96_camera_out_of_bounds`: `|x| > 0x6C0`, `|z| > 0xAB0`), and the
derived far wall sits at `x = 0x7E0 + 64`, so the unclamped focus could put the
eye within 180 units of the wall.

**Implementation.** `render.render_eye_x/y/z` + `render_eye_valid` are new
render-only state (cleared by the existing `reset_render` memset). The update
runs at the top of `match_run_live_camera_follow`, before the pan early return:

- focus x/z clamp to `[-0x6C0, 0x6C0]` / `[-0xAB0, 0xAB0]`;
- target eye = clamped focus + the camera-0 type offsets (`-100,488,1100`);
- first use or a > `0x600` axis error snaps; otherwise each axis moves at most
  `0x30` per granted frame (a 2-unit deadzone lands exactly).

`fifa96_match_run_render_eye` feeds the render pass; `fifa96_match_run_eye`
keeps the exact `focus + type0` result for the existing tests and callers.

**Tests and gate.** `test_live_render_eye_stabilizer` pins the 0x30 per-frame
bound and the native clamp. `make check` **124/124**; M1/M2 exact by `cmp` (the
stabilizer is live-only and the tapes never set `live_start`).

**Legs.** The native camera director/orientation (`FUN_0004E834`, P10-L6) and
the 3dwpts stadium (`FUN_000610C4`, P10-L2) remain open; the stabilizer is a
derived presentation stand-in, not a native producer.

## Phase 72 — live off-ball team shape (2026-11-07)

**Outcome.** The live match now moves the whole outfield shape, not only the
team target and the ball carrier. The shared row-`0x03` dispatch slot stays
`NULL`; the live path consumes the already-bound `pl.fmt` cell metadata
directly.

**Why.** `fifa96_outfield_forced_action` gives every non-target record code 3,
and row `0x03` (the off-ball target producer) is `NULL`, so those records kept
their kickoff targets. The native body is the `0x6D870` pitch-cell map plus the
`0x6DCC8` direction-pair scaler over `rec->phase_meta` (`0x6DE44`, rows
03/04/07). Wiring the shared slot moved the M2 tape at frame 29
(`test_engine_match_handlers`/`test_engine_m2` pins), so this slice is a
live-only consumer.

**Implementation.** `fifa96_match_pitch_cell` exports the pure 13×23 map
(`x_bin = (x + 0x86B) / 0x14B`, `z_bin = (z + 0xC1E) / 0x10E`, cell =
`z_bin*0xD + x_bin`, side 1 mirrored `0x12A - cell`). `fifa96_match_phase_apply_cell`
wraps the existing `match_phase_dcc8_apply` with an explicit cell.
`match_run_live_offball_shape` runs before the entity chain, phase-2/live-only,
and writes targets for active idle code-3 outfield records that are not the
ball carrier, team target or second pick. Unbound `phase_meta` is the native
gate and stays a no-op.

**Tests and gate.** `test_live_offball_shape_moves_team` stages synthetic
metadata and pins that at least 16 of the 20 outfield records move with no
score/phase write. `make check` **124/124**; M1/M2 exact by `cmp` (the shared
table and tapes are untouched).

**Legs.** The native `FUN_0007F7E0` attacking support-run machine remains a
separate slice (FU251-L8); the cell quantization is a derived live cadence
(the native refresh is event-driven via `FUN_000700F4`).

## Phase 73 — shaded HUD glyphs + crowd-tier tint (2026-11-07)

**Outcome.** The live HUD now renders the native 4bpp glyph shade nibbles, so
the team abbreviations are legible (`BRA`/`GER` in the windowed frame) instead
of the flat-fill degenerate shapes (`B`/`R` counters became solid ink). The
far-stand crowd band also no longer leaks the kit-remapped palette as neon
slivers; it keeps the `STACcovr` texture through the existing four-level stand
tint.

**Erratum (FU-148 §7 OL-T11-71, quoted).** The prior leg text said: *"the
native 4bpp plot path maps each glyph nibble through the runtime-filled
`0x15BBAC` shade table (`FUN_00019E9C` → `FUN_000AFCB0`/`FUN_000B2A90`)"*.
Fresh disassembly corrects that identification:

- `0x15B998` is the 16-dword shade table. `FUN_0009FFF0` loads
  `dword[0x15B998 + nibble*4]` on its shaded 4bpp arm, gated by
  `[0x1127EC] != 0` (the flat arm at `0xA05ED` uses the caller colour).
- `FUN_000AFCB0` resolves the dword for the active display mode; in the 8-bit
  modes (4/8) it returns `dword[0x15BBAC + index*4]`. `0x15BBAC` is the RGB
  palette, not the 16-entry ramp.
- `FUN_00019E9C(idx)` copies the selected palette bank's `+0x1000` 64 bytes
  into `0x15B998`; the exact entries are runtime palette-bank data (statically
  zero in the LE image), so the port's nearest-palette ramp is honestly `[D]`.
- The first-hand glyph scan (retail `clockfnt.fsh`) shows `B` row 1 =
  `FFFB500` and row 2 = `FF3FB00` at bit 174/row 0, and `R` row 1 =
  `FFFB5` at bit 268/row 0; zero nibbles are the only transparent pixels.

**Implementation.** `fifa96_font_blit` stays byte-identical for the non-live
and goldens paths. New `fifa96_font_blit_shaded` performs the same glyph
geometry but writes `shades[nibble]` for every non-zero pixel; new
`fifa96_font_blit_mask` stores the raw nibble so the 2× HUD scaler can expand
the glyph first and apply the ramp at the destination.
`render.hud_text_shades[16]` is built by
`fifa96_match_run_hud_text_shades_from_palette` from the staged palette
(linear black→`hud_ink` RGB interpolation, nearest palette entry, entry 15
pinned to the body index). The live HUD, pause, full-time and half-time
presenters use the shaded body pass; the flat shadow pass and the legacy
sub/replay/overlay presenters keep their operands. The 320 path and the 640/2×
path share the ramp (the latter through the raw mask). Non-live runs pass
`shades = NULL`, preserving the M1/M2 byte streams.

**Crowd tint.** `pitch_wall_pixel`'s crowd tier now returns
`pitch_wall_tint(px, blue, blue_light, blue_dark, white)` instead of the raw
`STACcovr` index. The live kit stage overwrites palette indices 30..229, which
made the raw crowd texels read as neon slivers; the tint maps the texel's
240/140/60 thresholds onto the stand's blue ramp and preserves the crowd
texture. This is the derived **P10-L7** fix; the native 3dwpts point/quad
renderer (**P10-L2**) remains a numbered open leg. The change is live-only
(`mr->live_start && pitch_art.staged`).

**Tests and gate.** `test_font` adds a synthetic 4bpp ramp/mask case and pins
the retail `clockfnt` B/R counter pixels (`B` row 1 x4 = nibble `B`, x5 = `5`;
row 2 x3 = `3`; flat rendering at the same pixels = body, zero stays
transparent). `test_engine_match_render` pins
`fifa96_match_run_hud_text_shades_from_palette`, the live 320 HUD `BRA`/`GER`
counter pixels end-to-end from the retail font, and the 640/2× raw-mask shade
path (a nibble-5 glyph selects entry 5, a nibble-F glyph selects entry 15).
`make check` **124/124**; M1/M2 exact by `cmp`. Windowed capture:
`docs/screens/p73-shaded-hud-crowd.png` (640×480, derived PNG export).

**Legs.** The exact `0x15B998` entries are runtime palette-bank data, so the
nearest-palette ramp is `[D]` (not byte-identical to the native bank). The
native 3dwpts stadium/camera art and the full crowd bowl remain P10-L2/L6;
the tint only removes the kit-palette leak from the derived wall.

## Phase 74 — player-facing pause substitution menu (2026-11-07)

**Outcome.** Pause **S**/pad North now opens a two-step OUT -> IN selection
instead of auto-picking the next bench slot. The menu shows the PCCD names,
numbers and positions, enforces GK-for-GK and the three-sub cap, hides already
used bench players, and keeps the existing R1 strip/pause notice handoff.
After a successful pair the pause plate shows `OUT > IN`; the menu is live-only
and leaves the deterministic tapes byte-identical.

**Native owner.** The native selection state machine is `FUN_00041494` case 5
(`0x418ED..0x41C80`): status 3 is the GK mismatch (`0x41982`), status 5 the
accepted pair (`0x41C80`) and status 8 the used-player refusal (`0x41AE3`).
The native 16-slot desired order (`0x14C360` / `FUN_00012010`), the FSM timing
(`0x14AEC8` timer `0x1E`, status 6 at `0x416BF`, case-7 reconcile),
the desired-order swap (`0x41B12`/`0x41B5F`/`0x41B91`), the per-line counters
(`0x14C3D0`/`0x14C3D1`) and CPU substitutions remain numbered legs. The port
uses the same status codes where observed and the derived 3-cap.

**Implementation.** `fifa96_team_data_roster_full` adds the PCCD position
(+0x09) and rating (+0x1B) bytes to the existing name/number handoff; the
bridge copies them into `render`-adjacent run state. `fifa96_match_run_sub_rebuild`
walks the native `0x11033A` formation blocks and assigns the highest-rated
unused player per role (then unused non-GK, then any unused), producing the
11-record `sub_lineup` and the 5-bit bench mask; with no PCCD data it falls
back to the identity map (pitch i -> roster i, bench 11..15).
`fifa96_match_run_pause_sub_apply` is the single write path; the menu API
(`fifa96_match_run_pause_sub`) drives OPEN/UP/DOWN/CONFIRM/BACK. The engine
latches CONFIRM/DECLINE/SUBST/UP/DOWN on rising edges (`match_menu_held`), so a
held key cannot repeat, and ESC backs out one menu level before resuming. The
260x184 panel uses the staged HUD font and `hud_text_shades`; a selected row
gets an ink underline. The old auto-pick function is removed; the engine's only
consumer is the new API.

**Tests and gate.** `test_engine_live_control` pins the role/rating lineup,
the exact bench mask, OUT->IN navigation, cancel/back, the first outfield and
GK substitutions, the used-bench and GK-mismatch statuses, the three-sub cap,
resume-close and the no-PCCD identity fallback.
`test_engine_match_render` pins the 260x184 panel geometry, selected-row
underline and the non-live suppression. `test_engine_team_data` pins Brazil's
non-GK-first roster and the full position/rating handoff. `make check`
**124/124**; M1/M2 exact by `cmp`. Windowed evidence:
`docs/screens/p15-sub-menu.png` and `docs/screens/p15-sub-applied.png`.

**Legs.** P15-L1 native `FUN_00041494` timing/reconcile; P15-L2 native
16-slot order/sort; P15-L3 native desired-order swap; P15-L4 body/stat
identity; P15-L5 per-line counters and gates; P15-L6 640-scale menu/full
used-player list; P15-L7 CPU substitutions.

## Phase 75 — live attacking support run + near-goal net guard (derived [D], 2026-11-07)

**Outcome.** The possessing team now sends one forward (role 3, then role 2)
on a forward support run while the carrier is live. The existing Phase 72
off-ball block still moves the rest of the shape; the support target is applied
after it and before the entity chain, so the mover consumes both in the same
frame. The runner makes through-ball receptions possible at the existing
landing/claim point, and changes the CPU pass destination; it is deliberately
not advertised as improving CPU pass completion (the CPU runner is a code-3
record and cannot claim).

**Native owner.** The native teammate-selection/support machine is
`FUN_0007F7E0` (FU251-L8): 2 514 bytes, sole caller `0x7F36F`, own jump table.
It remains unported. This slice is a derived live-only stand-in, not its port;
the `FUN_0007F7E0` machine, the second wide runner and the native row-05
reception/claim producer remain numbered legs.

**Implementation.** `live_support_runner`/`live_support_timer` are new live
cells. `match_run_live_support_pick` walks the native `0x11033A` formation
blocks, prefers an unused role-3 forward, then role 2, then a mid/forward
fallback and finally any eligible outfield record; the pick is the most
advanced by the team's attacking direction. `match_run_live_support_eligible`
rejects the keeper, hidden/occupied records, the carrier, `team->target`,
`team->second`, `team->intercept`, `team->chosen` and the controlled record.
The run holds for 120 granted frames before re-evaluating; the target is
`max(runner_front, carrier_front + 0x600)` clamped to the goal-line depth
`0xB00` and the runner's own x channel `±0x760`. Only the runner's target is
written; no score/phase/ball/control cell. The helper is gated on live phase 2
and skips kickoff touch, live flights/holds, saves, ball settlement and any
active act. It clears on init/begin, out-of-play force, restart bind, possession
loss and every non-play phase. No RNG is used.

**Near-goal net guard.** The same smoke exposed the derived P12 goal-net plane
painting a full-screen mesh when the follow camera passed close to a goal: the
inverse-projected plane had no near clip. `match_run_draw_goal_net` now rejects
an intersection nearer than `0x400` world units (`[D]`), so the far-goal
backdrop still draws at its normal depth while a near/behind-camera goal cannot
cover the pitch. This is a presentation guard, not the native 3dwpts net pass
(P10-L2).

**Tests and gate.** `test_live_support_run_moves_forward` pins the role pick,
target `0x500`, forward motion after 30 granted frames, untouched second pick
and score/phase; `test_live_support_run_hysteresis_and_reset` pins the 120-frame
hold, re-pick after expiry, second-pick exclusion and possession-loss clear.
`make check` **124/124**; M1/M2 exact by `cmp` (the tapes never set
`live_start`; no golden re-pin). Census unchanged: 78/78 bodies, 61/80 natural,
64/80 install-live. Windowed evidence: `docs/screens/p75-support-run.png`.

**Legs.** L1 native `FUN_0007F7E0` support machine (FU251-L8); L2 the second
wide runner and lane split; L3 the `0x600` depth and 120-frame hold are derived
tuning constants; L4 the one-frame `team->intercept` target clobber on the
controlled team (FU-341) remains accepted and documented; L5 the `0x400`
goal-net near clip is a derived presentation guard, not the native 3dwpts net
pass. CPU pass completion is not claimed before the native row-05
claim/selection producers land.

## Phase 76 — derived manual player switch (2026-11-08)

**Outcome.** **Q** (keyboard) or **LB** (gamepad) now cycles the controlled
outfield record while the live match is in play. The switch is edge-latched, so
holding the key selects once; the chosen player holds for 90 granted frames and
yields immediately to a genuinely near own ball carrier. The native
auto-switch/AI caller chain remains **P7-L1**; this is a derived control aid, not
a `FUN_0007D1D4` port.

**Why.** The live follow always chose the own carrier or the nearest own
outfield record (`match_run_live_control_update`), and the native nearest-select
handler `fifa96_match_run_select_step` exists but the live follow overwrote it
every frame. The player had no way to choose a different defender or support
player. The existing switch handler stages native swap requests; it is a
nearest-select, not a cycle, so it is not reused here.

**Implementation.** `FIFA96_ENGINE_KEY_SWITCH` (12) is mapped from SDL
`Q`/`LEFT_SHOULDER` and is deliberately excluded from `match_run_map_key`, so
it never becomes an FU-61 action word. `fifa96_match_run_input` edge-latches
the physical level (`live_switch_down`) into `live_switch_press` only while
live, unpaused, ACTIVE, phase 2 and outside the opening-kick latch.
`live_manual_id`/`live_manual_frames` hold the selection.
`match_run_live_control_switch_next` cycles record indices 1..10, skipping the
keeper and the `+0x98`/`+0x9A` excluded records; the cycle starts after the
current controlled/auto/target record. The override yields when the auto
selection is an own carrier within `MATCH_RUN_LIVE_CONTROL_RANGE` (a stale/far
possession cell is not treated as live). It clears on init/begin, staged
restart phases, out-of-play force, bind failure and expiry. No score, phase,
ball, team-target or slot write outside the existing bind.

**Tests and gate.** `test_live_manual_switch_cycles` pins the 1 -> 2 cycle,
held-key single step, hidden-record skip, expiry and auto return;
`test_live_manual_switch_keeps_carrier` pins carrier priority and edge
consumption; `test_live_switch_is_not_fu61` pins the input word separation.
`test_engine_sdl3` pins Q/LB mapping and the held-state sample. `make check`
**124/124**; M1/M2 exact by `cmp` (the tapes never press the switch key; no
golden re-pin). Census unchanged: 78/78 bodies, 61/80 natural, 64/80
install-live.

**Legs.** P7-L1 native FU-295 auto-switch/AI caller and player-switch body;
L2 the native nearest-select/swap (`0x7D1D4`/`0x786A0`) is not this cycle; L3
the 90-frame hold is derived usability tuning; L4 gamepad bindings beyond LB;
L5 the switch does not change camera director or AI control policy.

## Phase 77 — deliberate human contact and stable human side (2026-11-08)

**Outcome.** **T** / pad **RB** now performs a deliberate human contact
challenge on an opponent carrier within the duel range. The challenge runs the
same `fifa96_match_run_contact` registrar and referee decision as the AI duel:
Fouls Off returns a clean steal; Fouls On can award the fouled side a
free-kick/penalty restart. The automatic proximity steal remains the existing
clean derived claim and is not re-routed, so normal defending does not suddenly
give away fouls on every adjacency.

**Why.** The human proximity steal bypassed the referee entirely, while the
manual pad duel (`0x7CE38` -> row `0x0B` -> row `0x0C`) is unreachable in live
play because the controlled record dispatches as code 3 and the live follow
overrides the native control slot each frame. The native auto-switch/AI caller
chain stays P7-L1; this is a derived live contact seam over the already-ported
referee chain.

**Stable human side.** A human foul makes the native restart arm set
`phase_machine.side_controlled` to the fouled (AI) side. The live control
follow, restart bind, opponent/CPU drivers and the pause substitution menu now
use a stored `live_human_side` captured at live begin; the native
`side_controlled` cell still drives the referee/set-piece arms. This keeps the
human on their own team through an opponent free kick rather than handing them
the AI side.

**Implementation.** `FIFA96_ENGINE_KEY_TACKLE` (13) is mapped from SDL `T` and
`RIGHT_SHOULDER`, excluded from the FU-61 action word and edge-latched like
the manual switch. `match_run_live_human_tackle` accepts an opponent carrier
within `0x40`, rejects hidden records, and calls
`match_run_live_contact_tackle` with the human as rec_a and carrier as rec_b.
A returned decision sets a 45-frame cooldown and returns from the control
follow; a clean result takes the ball and installs carrier code 5. The stored
human side is set before `match_run_live_control_seed`, and
`match_run_live_control_team`/`match_run_live_control_bind` plus the pause
substitution side selection use it while the native cell keeps its phase-arm
meaning. No new score/phase/goal writer; no RNG of its own.

**Tests and gate.** `test_live_human_contact_clean_and_foul` pins the Fouls
Off clean steal, the Fouls On registrar (rec_first human / rec_second carrier,
contact_kind 1, decision and stoppage) and the stored human side after a
simulated native restart-side flip. `test_engine_sdl3` pins T/RB and the held
sample; the existing switch test also pins that T never becomes FU-61 input.
`make check` **124/124**; M1/M2 exact by `cmp`. Census unchanged: 78/78 bodies,
61/80 natural, 64/80 install-live.

**Legs.** P7-L1 native auto-switch/AI caller; L2 the native pad duel dispatch
code-3/`0x60` route; L3 the automatic proximity steal remains a clean derived
claim (deliberate T is the adjudicated route); L4 free-kick/penalty player
interaction remains the separate set-piece slice; L5 there is no native card
state/asset, so no card display is claimed.

## Phase 78 — player-controlled free kicks and penalties (2026-11-08)

**Outcome.** When the human side is awarded a free kick (phase 7) or penalty
(phase 6), the installed row-0x12/0x13 taker is bound to the live controls and
the native row is held at its stage-4 kick. **Left/Right** moves a derived
goal-mouth aim, **Up/Down** selects power and **Z**/**Enter** confirms. Confirm
resets the taker row, returns through situation `0xB` to phase 2 and launches
the existing live flight with the derived target. If the player window expires,
the same derived strike is auto-delivered; this is required because the native
row-0x13 stage-5 ball-ack wait is one of the unported FU-266/FU-275 legs and can
hold phase 6 forever. The prompt is a live-only screen-space panel ("FREE
KICK"/"PENALTY", aim, power, confirm keys). Non-human takers keep the native
auto row untouched.

**Evidence.** The phase-7/6 arm and the taker codes are the ported
`match_run_phase_arm` (`0x8D57B` / `0x8D4A2`, FU-149 §1.4). Rows 0x12 stage 4
(`0x841AD`) and 0x13 stage 4 (`0x84E26`) are the auto kick points. Fresh Ghidra
this slice at `0x84E44` shows the row-0x13 strike call as `EDX=0x40`, `EBX=0`,
`EAX=record` (`e87b6bffff`), with the record slot `[EBP+0x20]` inspected at
`0x84E26..0x84E36` and the `0x78AA4` restore call at `0x84E36`; the native
`0x4C114/18/1C` words are written at `0x84CCC..0x84D0C` and passed to the
shipped no-op `0x4C31C` (`e80676fcff`). Those words and the slot `word[+6]`
0x20 edge remain unported, so the live aim/power/confirm mapping is explicitly
derived rather than claimed as the native input machine.

**Implementation.** `live_sp_active/hold/kind/power/frames/taker/aim_x/
prev_input` live in `fifa96_match_run.h`. `match_run_live_setpiece_update`
activates only when the stored human side owns an active, unhidden installed
taker, binds it through `match_run_live_control_bind` (falling back to the
installed record when the headless pool has no slot), samples the arrow/KICK
edges, and auto-delivers after `MATCH_RUN_LIVE_SETPIECE_WAIT` (300 granted
frames). The row holds only when `live_start`, `live_sp_hold` and the matching
taker id are all set.
`match_run_live_setpiece_kick` installs code 0 on the taker at phase 2, clears
the pair/carrier, sets the live flight target (`aim_x`, opponent `±0xB40`),
speed `8 + 2*power` (10..16), shot/pending flags, a 30-frame hold, and calls
situation `0xB`. `match_run_live_out_force` clears a staged taker before its
fallback placement and re-installs it only if the fallback selects the same
record, so a throw/corner force cannot leave a stale 0x12/0x13 row in phase 2.
The existing live shot block, keeper save, goal armer and out-of-play path stay
authoritative. No new score/phase/goal writer and no RNG of its own; M1/M2
never set `live_start`.

**Tests and gate.** `test_live_ai_tackle_foul_reaches_free_kick` now pins the
live phase-7 taker binding, the 12-frame stage-4 hold, Left aim, Up power, the
confirm to phase 2, the side-correct `0xB40` target and actual ball movement.
`test_live_foul_reaches_penalty` pins the phase-6 equivalent. The new
`test_live_setpiece_does_not_hijack_opponent` pins the non-human native auto
path through phase 2; `test_live_setpiece_out_force_clears_taker` pins the
stale-taker reset when a throw/corner force wins the race;
`test_live_setpiece_timeout_falls_back` pins the 300-frame auto-delivery for
both phase 7 and phase 6 (the latter would otherwise hang on the unported
row-0x13 ack). `test_engine_match_render` pins the live-only 250x58
prompt plate and its ACTIVE-screen gate. `make check` **124/124**; M1/M2 exact by
`cmp`. Census unchanged: 78/78 bodies, 61/80 natural, 64/80 install-live.

**Legs.** FU-266/FU-275 L13.7 native `0x4C114/18/1C` input words, slot
`word[+6]`/`0x20` hand-off and `0x78A84/0x78AA4` bodies; the seven-arm FK
delivery sub-table; the native set-piece camera/UI and power semantics; the
row-0x13 head/keeper staging beyond the live shot seam; free-kick wall
obstruction remains the existing derived/native subset.

## Phase 79 — render-stage the live record facing (2026-11-08)

**Outcome.** The 22 player render slots now publish the pool `+0x7D` face word
into the slot heading, so live players draw the direction they actually face
instead of the render-zero direction. This closes the render half of the
FU319/FU323 face producer; the animation **pose/cadence** terminal-row steering
stays a separate leg and this phase makes no walk-pose claim.

**Evidence.** Fresh Ghidra this slice at `0x36D1A..0x36D3E` shows
`MOVSD`×3 copying the record position to the staged slot, then
`MOV ECX,[EDX+0x7B]` / `MOV ESI,0x400` / `SAR ECX,0x10` / `SUB ESI,ECX` /
`MOV ECX,ESI` / `AND ECX,0x3FF` / `SHL ECX,0x6` /
`MOV [EBP+0x155BC4],ECX`. That is the native
record angle `((0x400 - (dword[rec+0x7B] >> 16)) & 0x3FF) << 6`, i.e. the
`+0x7D` face word; `0x36D44/0x36D4F` then stage the anim id/frame and
`0x36D58..0x36D61` the hidden `Y=-10000`. The port's
`fifa96_render_slot_stage` already performs the same fold from
`heading >> 16`; only the scene staging left record headings at zero.

**Implementation.** `match_run_scene_stage` now calls the shared
`match_run_stage_heading(int16_t)` helper beside the position copy:
`(int32_t)((uint32_t)(uint16_t)word << 16)`. Both the record `face7d` and the
ball heading use it, so the 16.16 convention lives in one place. `face7d` is
the live pool word written by the locomotion mover and the face arms. No
renderer, animation-row, mover, or face-producer change. The stale comment
that called the record heading caller-owned is corrected in place.
Terminal-row steering
(flags bit 0 → the `0x11052A`/`0x1105AA` speed tables, duration override and
lean fold) remains FU159-L5/FU172; the native `0xA2A10` view-angle addend
remains P10-L6.

**M2 re-pin (measured).** First differing line **2545** (`frame=2545`,
`state=19/1-0`, hash `298926dde0a231ca` -> `d0e91d189461dd5a`); **3978 of
6522** lines differ, one contiguous hash-only tail **2545..6522**, the 10-tag
`state=` stratum set and all counts unchanged, prefix 1..2544 byte-identical,
length 295089 B / 6522 lines. Old M2 sha
`7c3874f794c4a9203311225b68beceeb2bd39943bd11025ea00a3b546e69469f` -> new
`41521559b2d9beee1890836deea9679f2384d84e0037657d47c3528263f8e75e`; M1
byte-identical `09b726b7…`. The move is the intended first hash-visible record
facing stage, not a regression.

**Tests and gate.** `test_engine_match_render` stages `+0x100` and `-0x100`
face words, asserts the slot heading high-half literals and the folded angles
`0xC000`/`0x4000` (for `-0x100`, `0x400 - (-0x100) = 0x500` masks to `0x100`);
the generic `test_render` vectors stay. `make check` **124/124**; M1 exact; M2
producer matches the new golden. Release smoke: a fresh windowed build starts
a match, accepts input and renders varied record facing
(`docs/screens/p79-facing.png`). Census unchanged: 78/78 bodies, 61/80
natural, 64/80 install-live.

**Legs.** FU159-L5/FU172 terminal-row pose/duration steering; the invalid
non-terminal `next_id >= 0x70` face write (FU318); the native `0xA2A10`
view-angle/render-direction addend (P10-L6); native 3dwpts camera rig (P10-L2).

**Register erratum (OL-T11-3).** The Task-11 close-out row retained at
`port/ENGINE.md:5372` still lists "record facing `+0x7D` (`-3`)" as a remaining
leg. This phase lands that render stage; the historical row is not rewritten,
so this section is the current status.

**FU323 erratum (render verdict).** The retained FU323 verdict at
`port/ENGINE.md:6971-6973` says "the drawn facing is the slot heading, not
`+0x7D`; the render stage still does not copy the face, that open leg stays".
That statement is now historical: this phase stages the `+0x7D` word into the
slot heading, so the drawn facing follows the live face. The FU323 position,
AI-population and camera measurements are unchanged; this correction is
render-staging only.

## Phase 80 — side-camera gameplay view (2026-11-08)

**Symptom.** The live windowed match used the type-0 end/behind-play view:
the camera looked down the pitch from the end, players were tiny, and a large
slice of the derived stand dominated the frame. This was not the native
default Friendly view.

**Root cause.** The live begin installed `FUN_0004D7E8` preset 0
(`0x107E2C = {-100,488,1100,33900,3900,4608}`) and added it to the FU-71
focus. The native default Friendly path uses the **class-3 side camera**
instead: FU-100 maps class 3 to camera slot 5, FU-104 records the read-only
side block `{-755,120,295,45425,0,4608}`, and the class-3 body is
`FUN_0004DB38` (OL-T11-76, still unported). A native default Friendly capture
(`session-probe-14c18-mode`, FU-16's standard key route; frame
`fifa96_005-0053.png`) and a manual DOSBox Friendly run both show the close
sideline view, so the port was selecting the wrong camera class.

**Implementation [D].** The live-only seed is now a derived left-side
follow:

- eye = FU-71 focus + `(-1600, 80, 0)`; the focus y is `0x50`, so the eye
  height is `0xA0` (160);
- yaw `0xC000` (look across `+x`), pitch `0x0C00`, ratio `4608`;
- `match_run_render_eye_update`/`fifa96_match_run_eye` use the same side
  offsets, and the live begin installs the side yaw/pitch/ratio.

This is presentation framing, not a claim of the class-3 output. The native
side block motivates the class; the exact seed is derived and the class-3
position/pitch integrators remain unported. No camera-director/3dwpts claim is
made. All changes are
`live_start`-gated; non-live/tape runs keep the image-default pose, so M1/M2
are untouched.

**Tests and gate.** `test_engine_match_render` pins the side eye/yaw/pitch and
now counts the rendered z=0 line pixels instead of assuming the old camera-0
vertical column; `test_engine_live_control` tracks the side-offset eye bounds.
`make check` **124/124**; M1 byte-exact `09b726b7…`; M2 byte-exact
`41521559…`. Release smoke: the windowed build starts a Friendly match and
shows the side view (`docs/screens/p80-side-camera.png`); the native
comparison is `docs/screens/p80-native-side.png`.

**Errata (Phase 66 type-0 claim).** Phase 66 said the live match installs
camera type 0's entry and the eye is the type-0 offset from the FU-71 follow
focus. That is retained as the native preset-0 record, but it is not the
default gameplay camera: the default Friendly view is the class-3 side camera.
The historical section is not rewritten; this section is the current status.
The pre-existing type-0 constants are removed from the live path.

**Legs.** Full class-3 `FUN_0004DB38` position/yaw/pitch integration; the
native camera director/orientation (`FUN_0004E834`, P10-L6); native 3dwpts
stadium/goal-net geometry (P10-L2); the native lens ratio path
(`FUN_000441D8`, FU94) and the `0xA2A10` view-angle addend.

## Phase 81 — class-3 camera envelope and render-only focus (2026-11-08)

**Root cause.** Phase 80 selected the right camera class but used the wrong
geometry: pitch `0x0C00` (~17°) instead of the class-3 `0x2000` (45°) clamp,
and a `(-1600,80,0)` eye. The native default Friendly sequence shows a high,
steep side view with the field filling the frame and the far stand outside it.
The Phase 80 smoke instead showed the low sideline with the stand and a ball
that drifted because the eye was derived from the FU-71 focus, which freezes
while a native pan flag is active.

**Evidence.** Ghidra `FUN_0004db38` (class 3) clamps pitch to
`[0x2000,63000]` and clamps camera x/z to `±0x420`/`±0x810`; FU-100 records the
class-3 slot-5 mapping and the 500 (`0x1F4`) side distance constant. The
native default Friendly capture `session-probe-14c18-mode` frames 005-0054..
005-0066 show the ball near frame centre throughout the high side view.

**Implementation [D].** The live seed is now:

- eye offset `(-500, 420, 0)` from the render focus; eye height `0x1F4` (500);
- yaw `0xC000`, pitch `0x2000`, ratio `4608`.

The presentation focus is separate from the native FU-71 focus:
`render.view_focus_*` follows the ball + the lead every granted frame, and
`fifa96_match_run_eye`/`match_run_render_eye_update` use it. The FU-71 focus
keeps its native pan freeze for the ball machine and AI, so the simulation
tests and M1/M2 are untouched. This remains a derived presentation envelope,
not the class-3 integrator.

**Tests and gate.** `test_engine_match_render` pins the high eye and now
checks that the far stand is outside the high framing; `test_engine_live_control`
adds `test_live_follow_keeps_ball_during_pan` and keeps the eye-step bounds.
`make check` **124/124**; M1 byte-exact `09b726b7…`; M2 byte-exact
`41521559…`. Release screenshot: `docs/screens/p81-class3-envelope.png`
(kickoff: `docs/screens/p81-class3-kickoff.png`); native reference:
`docs/screens/p80-native-side.png`.

**Errata (Phase 80 pitch/eye).** Phase 80's low `0x0C00` pitch and
`(-1600,80,0)` side eye were a first stand-in; they are superseded by this
class-3 envelope. The Phase 80 class-3 selection and the live-only gate stand.
The historical text is not rewritten.

**Legs.** Full `FUN_0004DB38` yaw/pitch integrator and native camera director;
native lens ratio path (`FUN_000441D8`, FU94); `0xA2A10` view-angle addend;
native 3dwpts stadium/goal-net geometry (P10-L2); player walk-row animation and
shadows are tracked separately.

## Phase 82 — live walk rows and ground shadows (2026-11-08)

**Symptom.** Live players slid around while their sprites stayed on the idle
stance; the ground-shadow pass was present but mapped grass through the 90%
mowing-stripe palette, so it read as nearly the same green.

**Root cause.** FU-172 §4/§5 says terminal rows re-select the movement tables
`0x11052A` (active) / `0x1105AA` (inactive) on every frame-timer expiry:
active speed 1–2 -> row `0x01`, 3–5 -> `0x02`, 6+ -> `0x03`, speed 0 ->
row `0x00`; the terminal duration comes from the same table. The engine's
`fifa96_match_run_record_anim_successor` returned early on terminal rows, so
the controlled record stayed on `0x00` while the mover raised `speed71` to 6
(live trace before the fix: `anim=00 frame=0`, speed 6). The shadow pass used
`pitch_shade` (90%) and a small centred ellipse, so it was effectively
invisible against the mowing stripes.

**Implementation [D].** Added the two 16-pair FU-172 steering tables and
`fifa96_match_run_record_anim_successor_turn`; the live scene stage selects on
every terminal advance, uses the table duration, and re-stages the changed row
and bank. The non-live `fifa96_match_run_record_anim_successor` keeps the
terminal no-op so M1/M2 remain byte-identical. Added a 55%-brightness
`shadow_shade[256]` palette map and a longer/offset (down-left) live shadow
ellipse under every sprite.

**Evidence/verification.** Live trace after the fix:
`anim=02 speed=3 -> anim=03 speed=6` with advancing frames; the controlled
record leaves the idle row. `test_engine_match_frame` adds
`test_record_terminal_steering_rows`; `test_engine_match_render` still passes
the frame-advance hash and live render. `make check` **124/124**; M1 bytes
exact; M2 exact after keeping the legacy wrapper path. Screenshot:
`docs/screens/p82-live-walk-shadows.png`; native reference remains
`docs/screens/p80-native-side.png`.

**Legs.** The `+0x7F`/flags-bit-2 lean producer and the full
FUN_0006FE84/FUN_0006E518 appearance machine (FU172-L2); walk-row cadence is
live-derived; native 3dwpts stadium/goal-net (P10-L2); class-3 camera
integrator/director still open.

## Phase 83 — live ball art, possession follow and off-ball pad movement (2026-11-08)

**Symptom.** The windowed match drew a large, see-through ball that grew as it
neared the camera, and a human controlled player who was not the carrier could
not be moved with the arrows; after a dribble the ball stayed at the old spot
instead of following the player.

**Root causes.**

- The retail `ball.fsh` bank uses palette index 0 for its opaque black outline
  and 0xFF for the transparent background. The generic FU-85 remap keys index 0
  out (`0 -> 0xFF`), so the live draw punched green pitch through every outline
  pixel. The synthetic/ISO fixtures pin the bank; the committed Phase 83 8x
  crop shows the restored opaque outline (the before run showed the green
  holes).
- `FUN_00057278` blits the ball frame 1:1 after rewriting the pivot to
  `(width/2, height)`. The `2 * [0x109000] <= depth` test in `FUN_000572E8` is
  a near-distance cull (`read_memory 0x109000` = 22), not a depth scale. The
  port was running slot 22 through the FU-85 entity scale with the parsed
  centred pivot, so the ball ballooned toward the camera and floated.
- The outfield forced action installs row 04 on the controlled team target when
  the ball is loose. The native row-04 user gate (`0x7E9F6..0x7EA16`) sends the
  controlled record to the camera target (`0x7EB95`); the slot-dir step
  `pos + dir*0x80` is the AI/non-self path (`0x7EBF8..0x7EC13`). With the
  engine's single `entities.controlled` mapping the human took the camera
  branch, so the pad never reached the mover. Row 0x03 (the native hold/move
  body) is ported but its shared action-table slot is NULL because wiring it
  moved the M2 tape at frame 29.
- `FUN_0008BAF0` is the loose ball machine. The native possession staging
  (`0x7A490`, row-05 `0x7F3A1..0x7F57B`) is unported, so a carried ball had no
  follow at all.

**Implementation [D].** Live-only (`live_start`); M1/M2 never set it.

- New `render.ball_remap[256]` identity translation, selected only by the live
  ball slot. The generic player remap still keys index 0.
- Live ball draw uses scale `0x10000` and `pivot = (width/2, height)`, matching
  the native 1:1 bottom-centre blit. Non-live keeps the FU-85 scale/pivot.
- Row 04 live pad override: the bound FU-70 slot's held direction builds the
  native `pos + dir*0x80` target through `fifa96_action_move_target`; no held
  direction keeps the row's own chase target.
- `fifa96_match_dispatch_action` dispatches the ported row-03 body for the live
  bound slot with a held direction. The shared action table stays NULL, so the
  port census and README title are unchanged.
- Ball-machine possession glue: a valid carrier within 0x80 of the ball in
  phase 2, with no live flight, pins the ball to the carrier's feet plus
  `dir * 0x20` and copies the carrier velocity/speed for the roll cadence. The
  save-hold block runs first, so a keeper pin is never moved; a far/stale
  carrier is left to the loose machine.

**Evidence/verification.** Ghidra `disassemble_function`/`disassemble_bytes`
`0x57278`, `0x572E8` (`0x5736a..0x5737c`), `0x7E9B0..0x7EA31`,
`0x7EBF8..0x7EC13`; `read_memory 0x109000`; retail `ball.fsh` census via
`fifa96_play sprite port/tests/golden/gameart0.pvi --name ball.fsh`.
`test_engine_match_render` adds
`test_live_ball_art_is_opaque_and_fixed_size`;
`test_engine_live_control` adds `test_live_offball_slot_player_moves` (live
row-03 dispatch + row-04 pad movement, non-live row 03 remains
`-FIFA96_ERR_UNSUPPORTED`) and tightens the carrier dribble distance to
±0x40. `make check` **124/124**; M1 byte-exact `09b726b7…`; M2 byte-exact
`41521559…` (both `cmp`-verified). Screenshots:
`docs/screens/p83-live-ball-and-movement.png` and
`docs/screens/p83-ball-crop.png`.

**Errata (FU-158 L158.5).** The prior “presenter's own blit scale cell
`[0x109000]`” wording is corrected: `[0x109000]` = 22 is the near cull
(`2*22 <= depth`), and the native presenter blits the 16x16 ball frame 1:1
with pivot `(width/2, height)`. There is no depth-scale cell on this path.

**Legs.** Native trail/secondary draws (L158.3); exact possession staging
`0x7A490` and row-05 target algebra `0x7F3A1..0x7F57B` (the glue and dribble
offset are derived); full human/AI selection handling in row 04; class-3
camera integrator/director (P10-L6); native 3dwpts stadium (P10-L2); HUD art
(P10-L5).

## Phase 84 — live player skin/hair and camera-relative facing (2026-11-08)

**Symptom.** Live players had green/magenta faces and hands, and their sprites
did not turn with the camera while running; the report grouped this with the
walking/physics feel.

**Root causes.**

- The live player draw used the identity translation, so the sprite's skin
  source indices (`0x75/0x76/0x8F/0x90/0x9C/0x9D`) drew the PALteam colours at
  the same palette indices. The native per-record translation
  `FUN_00047298` maps those source indices to the PALsys skin/hair entries.
- `FUN_00057158` adds a camera-relative atan (`0xA2A10`) to the staged record
  angle before the 0..7 direction fold. The engine left the addend as an open
  leg, so the sprite direction was the record facing alone and was only correct
  at one screen position.

**Implementation [D].** Live-only (`live_start`); M1/M2 never set it.

- `match_run_apply_player_remap` applies the outfield template's native
  appearance subset to both team translations after the kit-band redirects:
  `0x75/0x76/0x8F/0x90/0x9C/0x9D -> 0xF7/0xF9/0x00/0x00/0xFD/0xC0`,
  `0xD7..0xDA -> 0`, `0xDB..0xE3 -> 0xF6..0xFD` (`0xFE` is the live marking
  slot). The kit bands, shorts/socks redirects and marking slot are untouched.
- `fifa96_match_run_render` computes `view_addend[slot]` from the
  camera-relative pre-rotation pair (`slot.pos - camera`) with
  `fifa96_entity_angle` (the native 10-bit `0xCD474` atan) scaled `<< 6`, and
  adds it to the staged angle before the fold. The `0xA2A10` quadrant table was
  decoded first-hand: `+x = 0x4000`, `+z = 0`, `-x = 0xC000`, `-z = 0x8000`.
- The scene animation loop is factored into `match_run_animate_entity`; it
  still runs once per granted 30 Hz frame from the frame body (no cadence
  change).

**Evidence/verification.** Ghidra `read_memory` of `0x106962`, `0x1068E4`,
`0x106A1C`, `0x106B00`, `0x106B70`, `0x106C00`, `0x107080`, `0x1070D4`,
`0x1070E4`; `disassemble_function 0x47298`; capstone decode of
`0xA2A10..0xA2AD5` and the `0xA29F0` jump table; `FUN_00057158`
`0x57187..0x571DF`. `test_engine_match_render` extends
`test_live_real_team_kits` with the appearance remap and brown-skin assertions.
`make check` **124/124**; M1 byte-exact `09b726b7…`; M2 byte-exact
`41521559…`. Screenshot `docs/screens/p84-live-skin-and-facing.png` (native
reference `docs/screens/p80-native-side.png`).

**Errata.** Phase 80's summary and the Phase 82 legs list name the `0xA2A10`
view-angle addend as open; this phase ports it for live rendering. The
per-player pool, the native vertical ball staging (`0x7A490`) and the native
row-07 kick pose remain open legs.

**Legs.** Full `FUN_00047298` pool (runtime PCCD match-record pointers, the
five-template pattern pass, the `FUN_0001CAEC(9/0xD)` settings gates);
per-player skin/hair variation; native vertical ball staging `0x7A490` and
row-05 `0x7F3A1..0x7F57B`; native kick pose/animation row; class-3 integrator
(P10-L6); 3dwpts stadium (P10-L2); HUD art (P10-L5).

## Phase 85 — live kit/skin correction, pass reach and off-ball focus (2026-11-09)

**Symptom.** The live match still showed striped shirts and both teams in
Brazil yellow, the away team's white kit never appeared, a PASS stayed at the
kicker's feet, and the controlled player went off-canvas whenever the opponent
had the ball.

**Errata (Phase 84 appearance source model).** Phase 84's "head/hands" list is
quoted here and corrected: `0x75/0x76/0x8F/0x90/0x9C/0x9D` are **not** skin
indices. The staged PLAYART `walk.fsh` frame histogram puts `0x75/0x76` in the
bare-arm skin band `0x71..0x89` and `0x8F/0x90/0x9C/0x9D` in the shirt side
panel band `0x8A..0x9D`; mapping the latter to the skin ramp produced the
stripes. The native skin ramp is the 9-entry `0xDB..0xE3` group
(`DAT_001070E4`); the 4-entry `0xD7..0xDA` group is hair (`DAT_001070D4`). The
face pixels use `0xE0..0xE3`, so leaving that sub-band on the sock identity
made the face green.

**Errata (Phase 84/earlier team-1 translation).** The renderer performs one
table lookup per sprite byte. Setting `remap[208] = 239` redirects **source**
208 only; mapping source `0x50` to the value 208 did not redirect it, so both
teams used the home palette. The away redirect is now composed over the
appearance table (`195..203 -> 230..238`, `208..214 -> 239..245`) after the
semantic mapping, not as a direct source rewrite.

**Implementation [D].** Live-only (`live_start`); M1/M2 never set it.

- `match_run_apply_player_remap` maps the measured source bands: shirt
  `0x50..0x65/0x8A..0x9D/0xCC..0xCF -> 208..214`, sock trim
  `0x66..0x70/0x9E..0xA8 -> 224..229`, skin `0x71..0x89/0xDB..0xE3 ->
  0xF6..0xFD`, hair `0xD7..0xDA -> 0xD7..0xDA`, collar `0xC2..0xC4 -> 0xFE`,
  eye `0x4E -> 0x00`. The shorts identity (`0xC5..0xCB`) and the sock/boot
  identity (`0xE0..0xE5`) stay on their selected bands.
- The live palette copies the PALsys dark ramp into `0xD7..0xDA` for the hair
  remap (the kit overlay would otherwise leave PALteam yellow there).
- `match_run_apply_away_kit_redirect` composes the away bands over every
  appearance value after the semantic table; the direct source rewrite is
  removed.
- PASS/LONG_BALL flights normalise the FU-73 pair direction and project it to
  `speed * 0x60` units (clamped to `±0x720/±0xB10`). The old fixed x2 target
  was reached on the first step, so a pass looked stuck. KICK keeps the P9
  goal-mouth aim.
- Live kicks now apply the ball-pairing tail's computed animation row
  (`out.anim`) on the kicker (`FUN_0007A490` `0x7AA0E..0x7AA2B` ->
  `FUN_0006E598`). The native kick/shoot/header rows (6/7/8/9/0x14/0x1B) now
  play instead of the kicker staying on its locomotion row. Non-live M2 keeps
  the recorded animation state exact.
- `render.render_focus_*` is a new render-only focus for
  `match_run_render_eye_update`: when the human side does not carry, it blends
  halfway toward the controlled record so the selected player stays on
  canvas; when the pair is more than 0x600 apart the far case focuses the
  controlled record directly. `view_focus_*` and `camera.pos_*` remain the
  ball focus for the ball machine/keeper/armer, so shot balance is unchanged.

**Evidence/verification.** PLAYART entry 1 `walk.fsh` frame histogram
(`0x4E/0x50..0x65/0x66..0x70/0x71..0x89/0x8A..0x9D/0x9E..0xA8/0xC2..0xC4/
0xC5..0xCB/0xCC..0xD6/0xD7..0xDA/0xDB..0xE3/0xE0..0xE5/0xFF`); Ghidra
`read_memory 0x106962`, `0x1068E4`, `0x1070D4`, `0x1070E4`;
`disassemble_function 0x47298`. `test_engine_match_render` extends
`test_live_real_team_kits` with the home/away composed shirt mappings and the
skin/hair/collar bands. `test_engine_live_control` adds
`test_live_pass_flight_travels` and keeps the pan-gate test human-on-ball.
`make check` **124/124**; M1 byte-exact `09b726b7…`; M2 byte-exact
`41521559…` (both `cmp`-verified). Screenshots:
`docs/screens/p85-live-kits-skin.png`, `docs/screens/p85-pass-flight.png`,
`docs/screens/p85-kick-pose.png` and `docs/screens/p85-offball-focus.png`
(the last shows the controlled Brazil record and the German ball carrier in
the same frame; native reference `docs/screens/p80-native-side.png`).

**Legs.** Full `FUN_00047298` per-player pool and per-player skin/hair
variation; native vertical ball staging `0x7A490` / row-05
`0x7F3A1..0x7F57B`; the ball-pair stage tail's `sub_code` row provenance
(OL-52) and the full per-code target algebra (`0x7AA3C..0x7AE2F`, OL-62);
class-3 integrator (P10-L6); 3dwpts stadium (P10-L2); HUD art (P10-L5).

## Phase 86 — native per-player appearance and visible dribble ball (2026-11-09)

**Symptom.** Every player in a team shared one skin/hair ramp, the kit overlay
sat on palette `0xE0..0xE5` and overwrote the native skin shades, and a carried
ball disappeared inside the sprite whenever the pad was neutral.

**Errata (Phase 85 palette/kits).** Phase 85 says the sock band is
`224..229` and the skin ramp is `0xF6..0xFD`, with a hair ramp copied into
`0xD7..0xDA`. That was the engine stand-in, not the native layout: the native
`FUN_00047298` kit-band table at `0x106B70` uses destinations `37, 48, 59,
70, 81, 92` (len 0xB) and `101` (len 9), and the palette at `0xDB..0xFD` is
the four native skin ramps. Overlaying PALteam over `0x1E..0xE5` corrupted the
skin shades and forced all players onto `0xF6`.

**Native evidence.** `FUN_0004C00C` (`0x4C00C`) maps the PCCD hair digit
(`+0x1D` at the runtime descriptor, `+0x1C` in the PCCD record) to class
`0x30->3, 0x31->2, 0x34->0, else 1`; `FUN_0004C098` (`0x4C098`) maps the skin
digit (`+0x1E`/`+0x1D`) to `0x30/0x32->1, 0x31->3, 0x33->2, 0x34->0`, else the
hair class (`3` stays `3`, else `0`). Fresh `read_memory`:
`0x1070D4` hair ramps `{0,0,0,0}`, `{EB,EC,FC,FD}`, `{F0,F1,F2,F3}`,
`{D7,D8,D9,DA}`; `0x1070E4` skin bases `F6, ED, E4, DB`; `0x106B70` kit
destinations `37,48,59,70,81,92,101`. Retail PCCD samples: Brazil slot 0
hair `'2'`/skin `'4'`, Germany slot 0 hair `'0'`/skin `'2'`.

**Implementation [D].** Live-only (`live_start`); M1/M2 never set it.

- `fifa96_pccd_player` exposes `hair_code`/`skin_code` from record
  `+0x1C/+0x1D`; `fifa96_team_data_appearance` resolves one roster slot.
- The bridge stores the Team Select LANG ids in `matchup.team_id[]`, fills
  `roster_hair_code`/`roster_skin_code`, and builds the derived `sub_lineup`
  pitch -> roster map.
- `match_run_stage_live_kit` writes the six native kit bands at `37/48/59`
  and `70/81/92`; it no longer overlays PALteam over the skin range. It builds
  `remap_team` (default skin 0/hair 3) and `remap_entity[side][pitch]` with
  the player's native classes. The live sprite draw selects
  `remap_entity[side][pitch]`; the ball and HUD keep their maps.
- The neutral-pad possession glue now offsets the ball along the carrier's
  facing octant (`fifa96_match_dir_x/z[type] * 0x20`) instead of leaving it at
  the sprite centre.

**Verification.** `make check` **124/124**; M1 `cmp` exact `09b726b7…`; M2
`cmp` exact `41521559…`. `test_engine_team_data` pins the PCCD digits;
`test_engine_match_bridge` pins the id/roster handoff;
`test_engine_match_render` pins the six native kit destinations, the native
skin ramps and the per-pitch composition; `test_engine_live_control` pins the
neutral-pad ball offset. Screenshots:
`docs/screens/p86-live-appearance.png` (real windowed Brazil/Germany match),
`docs/screens/p86-live-dribble.png` and `docs/screens/p86-kick-pose.png`.

**Legs.** The native lineup matrix `0x14C360` is unported (the pitch -> roster
map is derived, and a substitution does not rebuild `remap_entity` yet); the
full `FUN_00047298` template pass and `FUN_0001CAEC(9/0xD)` gates remain open;
native possession staging `0x7A490`, row-05 target algebra
`0x7F3A1..0x7F57B`, native vertical ball staging, the ball-pair OL-52/OL-62
row provenance and the class-3 camera integrator remain open legs.

## Phase 87 — substitution remap rebuild; ball-vector/vertical re-read (2026-11-09)

**Symptom.** A pause substitution changed `sub_lineup[side][pitch]` but the
live sprite still drew the outgoing player's skin/hair. `render.remap_entity`
was built once by `match_run_stage_live_kit`; nothing rebuilt it after
`fifa96_match_run_pause_sub_apply`.

**Fix.** `match_run_build_entity_remaps` is now the one builder used by the
atomic stage path and by `match_run_rebuild_entity_remaps`, which
`pause_sub_apply` calls after the lineup/bench update. It composes
`render.remap_team[side]` with the incoming roster slot's native
`FUN_0004C00C`/`FUN_0004C098` skin/hair classes. No score, phase, entity,
ball, camera or census cell is written. The native `0x14C360` desired-order
producer and the full `FUN_00047298` 23-descriptor pass remain legs; the fix
is correct for the derived `sub_lineup` map.

**Ball-vector/vertical re-read.** Fresh `0x7A140`/`0x7F360` disassembly and
`0x7A084`/`0x795B4` decompiles settle the block: `0x158738` = distance,
`0x15873A` = dx, `0x15873C` = dz, `0x15873E` = trajectory (camera
height/angle parameter), `0x158740` = angle. It is a planar triple plus a
trajectory word, not `{x,y,z}`. Fresh `get_xrefs_to 0x158810` shows the
rendered ball record Y is written only by the reset/commit/penalty places
(`0x88761`, `0x8C295/0x8C299`, `0x84BC6`); `FUN_0008C24C` forces y := 0
(`0x8C299`), and `FUN_0008BAF0` only reads y and writes x/z. **Native vertical
ball physics is not a leg because it does not exist in the read paths**; do
not invent gravity/bounce for the native ball record.

**Row-05 tail.** `0x7F3A1..0x7F57B` is fully specified in
`port/spec/FU-348_substitution_remap_and_ball_legs.md` §4 (target triple,
`speed = word[+0x71] + cameraY`, `0x71C94` event with
`{distance, step_x, step_z}`, release `has_slot ? 4 : distance >> 2` plus
`word[0x1577F8]`, clamp `0x3C`, `0x157A4F` parity arm). FU251-L2 is resolved:
the overlapping `[ESP+0x0a] >> 0x12` is exactly `distance >> 2`; FU251's
target-x label is corrected by the alias rule (`dword[+0x6B]>>16` is word
`+0x6D`, while `+0x6B` is the seed/gate lane). The helper is
`fifa96_action_carrier_tail_step`; `fifa96_match_action_05` binds it for the
live human carrier while a pad direction is held (target, `ball.pos_release`,
`0x71C94` event + tracked). Idle/AI carriers, the sink bodies and the full
parity arm remain FU348-L3.

**Class-3 camera.** `FUN_0004DB38` remains the mapped but unported side-camera
integrator (`0x4C7D0/0x4C77C/0x4C7F8/0x49388` helpers); the live eye is the
Phase-81 derived envelope. The carried-ball glue is a camera-director
workaround, not a `0x7A490` stand-in: possession staging never writes the
rendered ball record.

**Verification.** `make check` **124/124**; M1 `cmp` exact `09b726b7…`; M2
`cmp` exact `41521559…`; census unchanged 78/78 ported, 58/80 naturally wired.
New tests: `test_engine_match_render.c::test_live_substitution_rebuilds_remap`
(RED on the pre-fix tree at the incoming skin-class assert) and the
`test_action_possession` tail fixtures. Windowed smoke:
`docs/screens/p87-live-tail-possession.png`. Full record:
`port/spec/FU-348_substitution_remap_and_ball_legs.md`.

## Phase 88 — adaptive-delta mixer cursor pacing (2026-11-09)

**Symptom.** Windowed live play dropped simulation time in episodes: the match
clock and granted frames lagged the 100 Hz PIT counter across goal/restart
phases (measured 5 s → 16 s of clock over ~14 s of wall time). The step
profiler put the stall in `fifa96_match_audio_step` (110–145 ms per step);
render was 9 ms and the match frame body stayed below 5 ms. The mixer decoded
~2.0 million adaptive-delta units for a 441-frame (20 ms) chunk.

**Root cause.** `delta_pos` is the next unit to decode, but the reset test
`unit < delta_pos` treated the ordinary fractional-step hold
(`unit + 1 == delta_pos`) as a backward jump. At the CHN_GEN1 crowd's
0.7256-unit step, ~27% of output frames hold a unit; each hold replayed the
whole delta chain from unit 0, making the 6.1 s loop O(n²) and overrunning the
engine's 100 ms catch-up cap.

**Fix.** `fifa96_mixer_delta_cursor_reset(unit, decoded)` returns 1 only when
`decoded != 0 && unit < decoded - 1`; same-unit holds reuse the cached sample
and keep the chain. Native `0xB832D` wraps only when `loop_end - cursor < step`
(`0xB834C DIV [ESI+0x2c]`, `0xB834F CMP EAX,0`, then `[ESI+0x14] = [ESI+0x1c]`,
`[ESI+0x18] = 0`), and `0xB8610` decodes from the current cursor
(`0xB86B6 ADD [ESI+0x38],EAX`); neither replays from zero for a held frame.

**Verification.** `make check` green; M1/M2 `cmp` exact. New tests:
`test_mixer.c::test_delta_fractional_step` (step-0.5 held samples and
`delta_pos`) and `test_mixer.c::test_delta_cursor_reset` (predicate boundary;
RED at the `(0,1)` case with the pre-fix `unit < decoded` predicate restored).
The same windowed drive held 100 ticks/s with the clock tracking wall time; no
step exceeded 50 ms. No score/phase/entity/ball/camera/render/census cell
changes. Full record: `port/spec/FU-349_mixer_delta_cursor_pacing.md`.

## Phase 89 — control-slot button edges while moving (2026-11-09)

**Symptom.** Kicking or passing while a direction was held did not fire until
the direction was released. A live `FIFA96_PLAYER_TRACE` probe showed the
KICK-up frame (`in=0x04`, `rel=0x00`, `held=0x04`) deferred the `0x10`
release; the slot only surfaced `rel=0x10` when RIGHT was finally dropped, at
which point row 7 installed and the ball left.

**Root cause.** `FUN_00078950` masks the button nibble before the edge/hold
machine (`0x78999 AND EAX,0xFF0`, `0x7899E AND ECX,0xFF0`), so a held
direction cannot enter `held`/`held_prev` or suppress a button release. The
port kept the full mapped word in `held`/the counter gate, so a direction-only
`mapped != 0` looked like a held button. The port's tests encoded that
pre-errata behaviour.

**Fix.** `fifa96_control_slot_update` computes `high = mapped & 0xFF0`,
`prev_high = prev & 0xFF0`, uses those for `pressed`/`released`/`held`/
`held_prev` and the counter, and still stores the full `mapped` in
`prev_mapped` for the direction/animation tables. KICK-up while moving now
reports `released = 0x10` on the same granted frame; row 7 installs and the
kick launches. `FU70_control_slots.md` carries an appended errata quoting the
missed masks.

**Verification.** `make check` green; M1/M2 `cmp` exact. New regression:
`test_control.c::test_slot_update_release_while_direction_held` (fails on the
pre-fix full-word hold). Direction-only counter expectations corrected to the
native high-nibble gate. No simulation/render/census cell changes. Full
record: `port/spec/FU70_control_slots.md` errata.

## Phase 90 — native line-up order and appearance mapping (2026-11-09)

**Outcome.** The match-start pitch-record -> roster-slot map is the native
16-slot desired order (`0x14C360` / `FUN_00012010`), not the derived
role/rating pick. Brazil's first 11 now render Taffarel, Branco, Santos,
Jorginho, Cafu, Zinho, Silva, Dunga, Rai, Bebeto, Romario; Germany's render
Illgner, Kohler, Schulz, Matthaus, Buchwald, Sammer, Hassler, Muller,
Effenberg, Klinsmann, Riedle. Because `FUN_0004C00C`/`FUN_0004C098` read the
hair/skin digits from that same order entry, the visible skin/hair assignment
follows automatically. Synthetic/no-PCCD fixtures keep the derived fallback,
so the M1/M2 tapes are unchanged.

**Root cause.** `FUN_0008C758` binds `rec+4 = 0x157BD2[side][rec+0x8D]`; the
port's `rec+0x8D` analog is `entity.active` (record index 0..10). The port
had no `0x14C360` producer, so `sub_lineup` was built by role/rating. That
selects a different player for several pitch records (the clearest visible
case is Germany: the derived order puts Buchwald on pitch 2 and Schulz on
pitch 4; the native order reverses them, changing skin class 2 vs 1).

**Native evidence.** The slot decoder `FUN_000103D0` maps PCCD `+0x09` to
slot `+0x09` (position) and PCCD `+0x0B` to slot `+0x0C` (the sort key), not
the overall rating at `+0x1B`; the appearance pair remains slot `+0x1D/+0x1E`
= PCCD `+0x1C/+0x1D`. `FUN_00011F0C` reads the signed team-header key as
`*(int *)(header + 0x32 + slot) >> 24` = `header[0x35 + slot]`; `FUN_00011E48`
selects by `slot+9` and the signed `slot+0xC`; the `0x10010` fallback is
`GDDDDMMMMFF` followed by five `-1` entries. `FUN_00012010`'s companion-byte
swaps are the competition card/injury replacement pass (`FUN_00035A1C` clears
and populates `0x14C3A0`); with no event records they are no-ops. Retail
Brazil/Germany keys and full 16-slot orders are pinned in FU-350 §3.

**Implementation.** `fifa96_pccd_team` exposes `order_key[16]` (`+0x35`);
`fifa96_pccd_player` exposes `sort_key` (`+0x0B`). The new
`fifa96_pccd_lineup_order` implements the primary pass with `int8_t` keys
(the `0xFF` sentinel must stay signed on this target), and
`fifa96_team_data_lineup_order` resolves it through the 44-team map. The
bridge fills `mr->roster_order[side]` / `roster_order_ok[side]`;
`fifa96_match_run_sub_rebuild` validates the permutation, assigns its first
11 to `sub_lineup`, and otherwise falls back to the role/rating builder.

**Verification.** `make check` **124/124**. M1 `cmp` exact
`09b726b7…`; M2 `cmp` exact `41521559…`. New coverage: Brazil/Germany keys
and full orders (`test_engine_pccd`), LANG handoff (`test_engine_team_data`),
native/fallback branch and permutation rejection (`test_engine_live_control`),
bridge integration (`test_engine_match_bridge`), and the native order ->
skin-remap composition (`test_engine_match_render`, Germany pitches 2/4).

**Errata (quoted).** Phase 86 legs: "The native lineup matrix `0x14C360` is
unported (the pitch -> roster map is derived...)" — the producer and the
default pitch -> roster map are now ported; the full `FUN_00047298` pool
remains. Phase 87: "The native `0x14C360` desired-order producer ... remain
legs" — the producer is ported; the in-match order swaps are not.

**Legs.** FU350-L1 competition card/injury companion swap
(`FUN_00035A1C` + the two `FUN_00012010` swap passes); FU350-L2 front-end
edited-line-up re-sort; FU350-L3 the full `FUN_00047298` 23-descriptor pool;
FU350-L4 in-match order swaps (`FUN_00041494`/`FUN_0008CBB0`/
`FUN_0008EAB8`/`FUN_0009343C`). Full record:
`port/spec/FU-350_native_lineup_order.md`.

## Phase 91 — native class-3 side camera (2026-11-09)

**Outcome.** The live eye is now the native class-3 side camera
(`FUN_0004DB38`), not the Phase-81 derived envelope. The windowed Friendly
view is a 500-unit chase: the camera sits `(-353,+353,0)` from a level target
at yaw `0xC000`/pitch `0x2000`, clamps its position to ±0x420/±0x810, keeps a
live Y floor of 100 and follows the play with the native yaw/pitch slew. The
player reads visibly larger and the side view tracks movement; the old
derived envelope (653-unit offset) is now only the first-frame seed.

**Native evidence.** Fresh `disassemble_function 0x4DB38` gives the full body:
the live gate (`FUN_0006400C`), the 500/600 distance
(`0x108B7C`, replay +100), the look-at yaw/pitch (`FUN_0004C7D0`/`77C`), the
pitch snap `(0x2000,0x8000)->0x2000`/`(0x8000,0xF618)->0xF618`, the
`X(pitch)*Y(-yaw)` offset (`FUN_0004C7F8`/`4C414`/`4C3B0`/`A2991`/`A2914`),
the position clamps and Y ease, and the `FUN_00049388` tick slew
(`0x1E0000`/`0x3C0000`). `0x108B98/9C` = 100/120 are the live/replay Y floors;
`0x108B80[3]` maps class 3 to the handler. The fixed point for a level target
is `target (0,80,0), camera (-353,433,0)`.

**Implementation.** `fifa96_camera_behavior_action` in
`port/src/fifa96_loader/fifa96_camera.c` now implements the full body, with
the 16-bit angle scaling from the port's 0x400-circle
`fifa96_entity_angle`/`fifa96_entity_sine`. `fifa96_camera_pose_args` carries
`handler_tick`; the live path calls the handler at speed 500 with
`frame_delta*4` (the native `[0x107300]` is 4x the 30 Hz delta). The target
is the existing Phase-85 derived render focus; the phase-71 envelope seeds the
first frame only. Non-live tapes never enter the handler.

**Verification.** `make check` **124/124**; M1 `cmp` exact `09b726b7…`; M2
`cmp` exact `41521559…`. `test_camera` pins the fixed point, the replay
600/120 values, the `+0x420` clamp and the `0xF618` pitch snap.
`test_engine_live_control` pins the live `+-0x420` clamp. Windowed Friendly
screenshots: `docs/screens/p90-native-class3-kickoff.png` and
`docs/screens/p90-native-class3-follow.png`.

**Legs.** FU351-L1 native live target producer (`FUN_000505D0`/FU-71 pose and
pan events); FU351-L2 camera-type/view-mode selection
(`FUN_0004D7E8`/`0x14E57C`/options); FU351-L3 exact `FUN_000A1A60` 6-bit
sine/cos interpolation (port table resolution); FU351-L4 class-3 target
history copy (`+0x40/44/48`); FU351-L5 native pan/pose interaction with the
class-3 target. Full record: `port/spec/FU-351_class3_camera.md`.

**Errata (quoted).** Phase 86's class-3-camera claim and FU-348-L4
("`FUN_0004DB38` and its helpers are mapped but not ported; the live eye is
the derived envelope") are corrected for the handler and live eye. The native
target producer and camera-type selection remain FU351-L1/L2.

## Phase 92 — exact class-3 sincos (2026-11-09)

FU351-L3 closed. `fifa96_camera_behavior_action` now uses
`fifa96_projection_sincos` (the exact FU-88 `0xA1A60`/`0xCE3B0` 257-entry
table + 6-bit correction) instead of the entity sine table at `>>6`.
`fifa96_camera` links `fifa96_projection` for the primitive. Geometry, tick
and clamps are unchanged; the live and unit camera tests still pass.
`make check` **124/124**; M1/M2 `cmp` exact. Record: FU-351 §6.

## Phase 93 — class-3 handler correction (2026-11-09)

Phase 91's "full body" claim was premature. The fresh 186-instruction
`0x4DB38` listing adds three native arms and corrects the slew scale:

- Q16 rate `floor(tick * 0x10000 / 30|60)` from the `SHL EAX,0x10` plus
  remainder divisions. The old tick/divisor form made tick 8 a zero rate;
  the 32-bit shift wraps tick `0x3C0000` to zero.
- Old-eye `off_y >> 3` at `0x4DBCA..0x4DBF3`.
- Post-clamp look-at at `0x4DC82..0x4DC96` (raw pitch, then the existing
  post-slew snap).
- Target-history copy at `0x4DD95..0x4DD9D` (`hist_x/y/z`; no consumer).

The separate `brake_*` fields were removed: the native clears the high
words of yaw/pitch, which the port's 16-bit stores already do. The true
fixed point is `(-353,434,0)`; replay is `(-424,505,0)`. `test_camera`
pins the Q16 moves, the 32-bit wrap, the Y split, the replay 505/120
values, the fixed point and the history. `test_engine_match_render` pins
yaw/pitch 0 for its identity-view ball fixture. `make check` **124/124**;
M1/M2 `cmp` exact. Non-live tapes are unchanged. Record: FU-351 §7.

**Errata (quoted).** Phase 91: "camera `(-353,+353,0)`" — the fixed point
is `(-353,434,0)`. Phase 92: "geometry, tick and clamps are unchanged" —
this phase corrects all three. Phase 91's FU351-L4 history copy is now
ported; FU351-L1/L2/L5 remain open.
