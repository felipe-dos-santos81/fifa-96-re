# Native engine (`fifa96_engine`)

The native engine layer sits on top of the 56 clean-room `fifa96_*` libraries and
turns them into a running game: platform ABI → SDL3/null backends → engine core
(boot, asset table, clock, intro, front-end, match).

Status: **M1 complete headless; M2 match playable, visible and RGB-visible —
21/80 action rows wired, the derived FU-143 phase driver wired into the run
loop (live period end 2 → 0x0C), the derived C3-OL2 score source, live OL-80
animation inputs, the derived kickoff ball placement, the resource-loaded
formation/record placement (OL-T11-8: `352ko.fmt` seated from
`/ART/GAMEART0.PVI` at begin, so player records receive real positions and
draw), the derived kickoff phase-1 entry (OL-84: begin lands
`FIFA96_MATCH_RUN_KICKOFF_PHASE = 1`), the derived `FUN_0008D098` state-1
kickoff arm + wired action row 01 (M2 playable-match Task 2: a begun run
reaches the live phase 2 naturally via situation 0xB), the derived native
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
acceptance tape **v8** (the phase-8 live-loop acceptance, T5; v7 was the
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
at step 215 with row 01 dispatched and score 0-0). The phase-6 ports S1–S4
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
the leg register. The interactive
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
record 9 carrying action code 02 (`locomotion_restart_target`, unported), so
no wired row consumes the pad for it (the T3 input-row seam runs for it and
finds no matching pressed row; the T3 null probe `probe_t3.out` records the
code-02 state; see "Interactive smoke" and "Known gaps"). A T5 Z-burst during
live play likewise leaves the scene band at AE 0 (full-frame 201 = the clock):
the pad kick is wired but its scene-facing effect needs a carrier record, and
the possession producers are unported. Kick → score stays blocked
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
side — all landed, tape-dormant, screenshots under `docs/screens/p9-*`) and
the SDD workspaces under
`.superpowers/sdd/` for the full record (scratch; may be deleted).

## Layout

| Path | Responsibility |
|---|---|
| `include/fifa96_engine/fifa96_platform.h` | Frozen platform ABI (video/audio/input/time) |
| `include/fifa96_engine/fifa96_platform_null.h` | Headless backend + observability stats |
| `include/fifa96_engine/fifa96_engine.h` | Opaque engine API (`create`/`boot`/`step`/`run`/`destroy`) |
| `include/fifa96_engine/fifa96_surface.h` | 320×240 indexed surface + palette + Mode-X plane packing |
| `include/fifa96_engine/fifa96_asset.h` / `fifa96_cache.h` | ISO mount, path table, on-demand byte cache |
| `include/fifa96_engine/fifa96_clock.h` | 100 Hz PIT model driver (`fifa96_pacing_clock` + `fifa96_tick`) |
| `include/fifa96_engine/fifa96_intro.h` | TGV intro playback into the surface |
| `include/fifa96_engine/fifa96_frontend_run.h` / `fifa96_menu_art.h` | Front-end state machine, key mapping, menu rendering |
| `include/fifa96_engine/fifa96_match_bridge.h` | Front-end start classification → `fifa96_match_run_begin` + default art staging |
| `include/fifa96_engine/fifa96_match_run.h` | Match lifecycle, 30 Hz frame body, input/control slot, render chain, dispatch observation |
| `include/fifa96_engine/fifa96_match_handlers.h` | FU-137 action/phase dispatch tables (45 + 35 rows) and the wired-row seam |
| `include/fifa96_engine/fifa96_match_entities.h` | FU-141 entity/ball pool and FU-67 update chain |
| `include/fifa96_engine/fifa96_match_phase_machine.h` | FU-142a `FUN_0008CEB8`/`FUN_0008D098` installer arms (0x26/0x28/0x2A) |
| `include/fifa96_loader/fifa96_arm_helpers.h` / `fifa96_arm_bodies.h` | Cluster-G record machines (rows 26/27/28/29/2A/2C bodies) |
| `include/fifa96_engine/fifa96_keys.h` | Shared engine key codes (1–9) |

## Building and testing

`make check` configures, builds `-Wall -Wextra -Werror`, and runs all 108 CTest
cases — including the `test_engine_*` cases built under ASan/UBSan. No external
dependency is required for this: the `null` backend is the deterministic
regression source of truth (it hashes Mode-X planes + palette and PCM, replays
input tapes, and advances a virtual 10 ms clock). With SDL3 ≥ 3.x installed,
the build also produces the windowed `fifa96` target (`make game`).

## What runs today

- Boot from the real CD image (`game/FIFAPCCD96.iso`): ISO mount, asset table,
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
- Action dispatch (FU-137): **21/80 rows wired** — `00`, `01`, `02` (M2
  phase-9 T1/FU-75 L4.6: the restart/placement row whose phase-2 install-4
  invoke drives the live held-key movement), `05` (M2 phase-9 T2/FU-142
  OL-63: the carrier row's claim/control/dir/stage landing, so a live code-5
  record claims `[0x158724]`), `04`, `06`,
  `07`, `08`, `0F`, `18`, `1E`, `21`, `23` (playability G1 + arms-and-wiring
  G3; `01` is the M2 playable-match Task 2 kickoff taker), `26`, `28`, `2A`
  (cluster G), `1D` (FU-151 P3 close-down) and `10`, `11`, `12`, `13`
  (FU-149 L13/T1: the throw-in/corner/free-kick/penalty taker machines —
  placement/probe/kick/resolution; the taker codes the P1 arms install now
  execute and hand back to phase 2 through the shared situation-0xB entry);
  dispatch results 58 UNSUP / 21 OK / 1 NOTF.
- **Interactive smoke (phase-8 T5 re-run, this host 2026-10-10; the phase-7 P5
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
  longer pulses; `tests/test_engine_sdl3.c::test_held_key_is_a_state_sample`
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
  — the follow/pan camera that would is phase-9 T3. **T3 (2026-10-10) wired
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
  **Kick (gameplay) and score stay blocked on screen:** a T5 Z-burst during
  live play leaves the scene band at AE 0 (full-frame 201 = the clock), so the
  KICK press dispatches no scene-facing gameplay row. The natural goal
  *producer* exists
  (T2 wired row 04's event outputs to the real pan setter) and the chain was
  closed end-to-end by T4 (natural goal -> score 1-0 in
  `test_engine_match_frame::test_natural_goal_end_to_end`), but the acceptance
  tape's code-4 records never meet the half-line event gates, so the tape
  camera never pans and no goal enters the tape. The
  goal **consumer** chain was landed by S3 (FU-146): begin installs the
  goal-screen machine (`0x92D8C/0x92E2C`), the frame body runs the
  session-gated scheduler (`FUN_000948AC 0x4B1A1`, before the clock body), and
  a queued goal id is consumed by the installed period handler into
  `fifa96_match_run_score_event` — the score increments through the native
  chain in a fixture
  (`test_engine_match_frame::test_goal_consumer_chain_fixture`), while both
  goldens stay byte-identical (the tape camera never pans, so no natural goal
  enters the tape). `fifa96_match_run_goal_queue`'s direct fallback now honours
  the native `[0x157AC2] in {2,3}` phase-5 skip. The P5 smoke shots' measured
  content (identical to the S5 re-run): 3.60% non-black window pixels (the
  kickoff frame: 24913 non-black of 691200), dominated by the HUD bar's
  `#900808` (11301 px); the sprite color
  `#E044A0` (the OL-T11-6 6-bit `0x38/0x11/0x28 << 2`) is live (1200 px).

  Reached vs blocked (T5 smoke, 2026-10-10; identical reach to P5/S5):

  | step | state | evidence |
  |---|---|---|
  | window + intro + front-end draw | reached | `p8-v8-frontend.png` (byte-identical to the P5 shot); ESC exit 0 (clean run) |
  | panel DECLINE/CONFIRM → match start | reached | match canvas replaces the front-end; `p8-v8-match-hud.png` (byte-identical to the P5 shot) |
  | RGB palette on the match canvas | reached | `#E044A0` sprite pixels; tape frame-6 palette assertion |
  | match HUD (bar/score/clock) | reached (on screen) | `p8-v8-match-hud.png` (0-0, 00:00); tape frame-6 bar-pixel assertion |
  | kickoff → phase 2 naturally | reached (on screen) | KICK burst → clock `00:02` in `p8-v8-match-clock.png`; tape v8 `run_natural_probe` (phase 2 at step 215, row 01 dispatched) |
  | move the controlled player | **reached (M2 phase-9 T1; T3 re-run)** | the ported row 02 (FU-75 L4.6) turns the post-kickoff slot record (team 0 record 9, code 02) into code 4 on its first live phase-2 frame and row 04 writes the slot-dir target; the live smoke held **LEFT** for 3 s (`p9-t1-hold-left.png`): record `(-58,-2) -> (-58,+178)`, scene-band AE 45710 (T3 re-run 45902, `p9-t3-hold-left.png`); **UP** (`p9-t1-hold-up.png`): `(-58,-2) -> (+122,-2)`, AE 28535 (T3 re-run identical, `p9-t3-hold-up.png`); **RIGHT** (`p9-t1-hold-right.png`): `(-58,-2) -> (-58,-190)`, scene AE 0 (the yaw/pitch-0 stand-in camera does not draw the negative-z walk; the T3 auto-camera is ported but its high-ball trigger is not live-reachable, FU-148 §13). Headless gates: `test_held_key_moves_live_controlled_record` (ISO), `test_machine_forced_decision_installs_on_slot_record`, `test_machine_no_edge_arm_copies_camera_target` |
  | kick the ball (gameplay) | blocked live; T3 pad kick fixture-proven, T2 carrier live in the headless fixture | the pad kick path is derived and wired first-hand (T3: carrier `+0x91 == 5` → code-1 released row `0x7D110` → install 7 invoke → row 07 kick); the possession/carrier producers are now landed (T2: `test_live_carrier_claim_and_kick` drives movement → claim → code 5 → kick → the staged ball pair on the real path), while a live Z-burst still leaves the scene AE 0 (the on-screen ball view is T4's smoke) — `test_pad_kick_release_runs_kick_row` |
  | score a goal | blocked naturally; chain producer-real in a fixture | the row-04 pan origin is wired (T2) but the acceptance tape's code-4 records stay far from the camera (lane ~1460+), so the tape camera never pans; the S2/S3 chain increments the score in `test_camera_pan_event_chain` (producer seed), `test_row04_live_pan_arms_camera` (live row -> armer -> situation 5 -> score) and `test_natural_goal_end_to_end` (natural kickoff -> pan -> situation 6 -> id 5 -> handler -> score 1-0) |
  | set pieces / restarts (live) | blocked; chains fixture-proven | the restart scanner requires `phase 2 && goal_armed` and the never-panning camera never arms (the live row-04 origin is wired but tape-dormant, T2); FU-149 P1 queue/arm/scan and the counter are fixture-proven, the L13/T1 taker rows 0x10..0x13 execute and resolve in the armed fixtures (`test_taker_armed_rows_resolve`/`test_taker_armed_referee_rows_resolve`), and tape v8 asserts the fresh dispatcher cells (`sp_*`) and the absent taker mask at m 41/m 62 |
  | fouls / offside (live) | blocked; chains fixture-proven | the FU-150 entries have no live producer; `test_referee` + the `test_engine_referee_*` chains prove them, and tape v8 pins `ref_machine == REF_NONE` with no whistle/speech/decision cells |
  | keeper restart rows 1D/1E (live) | blocked; row 1E machine tape-reachable | the taker rows 0x10..0x13 are ported but dormant on the tape (the set-piece arms never fire; FU-149 L13/T1) and row 1E executes on the tape's m 41 staging (v8 pins the claim take + the 0x15774C focus triple and the camera triple it resets to); row 1D is fixture-proven (`test_keeper_machines`) |
  | replay / overlay / sub strip (live) | blocked | zero-gated rows with no producers yet (FU-152 legs 1/2/3/7); tape v8 pins `replay.state`/`sub.active`/`overlay.armed` 0 and `ball_row` NULL |
  | half/period end → exit | reached (headless) | tape: live class-1 period end → phase 0x0C → OVER→POST→EXIT; native periods last minutes, so not run to completion in the smoke |
- `test_engine_m1` pins the 691-frame M1 transcript
  (`tests/golden/engine/m1-frames.txt`).
- `test_engine_m2` replays spec §5 (boot → skip intro → front-end → start match
  → kickoff → move → kick → score → period end → exit to front-end) and pins
  the 165-frame M2-B transcript (`tests/golden/engine/m2-frames.txt`):
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
  `test_engine_match_frame::test_kickoff_enters_phase2_naturally` pins a begun
  run reaching the live phase 2 with no forcing (FU-143 §11). The 0x13/0x14
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
  `./build/test_engine_m2 > tests/golden/engine/m2-frames.txt` (the test exits
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
    callers.
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
    row-02-reactive fixture updates (`test_kickoff_enters_phase2_naturally`
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
    steady/sidetrack/staged/action`): the 0x107508 `{+4,+0xC}` mapping (3->5
    remap, negative clamp; the image -1 cells are the degenerate 0x108B60
    read, engine-clamped) and the quoted clamp/constant level of the four
    0x108B80 bodies plus the pinned 0x108B64 mode table
    (`fifa96_camera_behavior_blocks`: classes {3,1,3,3,1,3}, class-gate cells
    the dword +0xC {0,0,0xA7F8,0,0,0xA21C} — blocks 2/5 take FUN_0004EC9C's
    class-3 arm), wired into the FUN_000505D0 default arm through the staged
    record subset + behavior block (NULL = unported default, tape unchanged).
    The FUN_0004E248/D698/DF34/C7D0/D668 integrators and the yaw-band
    snap-target math stay leg 9.
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
  `tests/golden/engine/m2-frames.txt` unchanged (the keeper rows are not
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
- **Unwired rows (60/80).** 59 rows dispatch `-FIFA96_ERR_UNSUPPORTED`: 21
  unported action rows, 34 phase rows (derived and ported at the loader level by
  FU-143 but not wired into the engine dispatch), the unwired actions
  `27`/`29`/`2C` (ported bodies, no installer entry) and the dead entry `2B`;
  phase `0x16` is the native zero/INT3 slot and the single
  `-FIFA96_ERR_NOT_FOUND` (20 action rows wired OK, FU-149 §7 / T1 + the M2
  phase-9 T1 row `02`).
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
  reconcile in a follow-up (FU-142 §6/K.6.7).
- **OL-63 row 05:** the carrier machine stages 0-3 and the ball staging tail are
  ported; the stage-0 target algebra and the `FUN_0007F7E0` fallback remain.
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
  `OL-72`…`OL-79`. The phase driver `fifa96_match_run_phase_drive` is **wired
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
  producer `mr.global_5882a` at `state.tick_total >= 0x78`. A begun run
  therefore reaches the live phase 2 naturally (pinned by
  `test_engine_match_frame::test_kickoff_enters_phase2_naturally`; FU-143
  §11). The remaining situation-0xB producers (rows 02/0x10..0x13, the
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
  `fifa96_score_display_gate` + `score_display_event`. The engine still
  carries tracked side -1 (`add_goal`-equivalent) until the tracked-side pick
  lands; `fifa96_match_run_add_goal` remains for those unported paths.
- Retail front-end art asset (no OPTIONS-like path exists in the ISO).
- Per-row cluster legs carried in FU-139/FU-141/FU-142 (`OL-56`…`OL-71`:
  unmodeled record bytes, process globals, camera/track inputs, roster
  descriptors, animation selectors).
