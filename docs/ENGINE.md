# Native engine (`fifa96_engine`)

The native engine layer sits on top of the 56 clean-room `fifa96_*` libraries and
turns them into a running game: platform ABI → SDL3/null backends → engine core
(boot, asset table, clock, intro, front-end, match).

Status: **M1 complete headless; M2 match playable, visible and RGB-visible —
14/80 action rows wired, the derived FU-143 phase driver wired into the run
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
acceptance tape **v6** (the phase-6 full-gameplay acceptance, S5; v5 was the M2
interactive-match G3 close-out and the lineage v4 G4 acceptance / v4.1 pad
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
summary, the re-pin ledger and the leg register. The interactive `make game`
smoke re-run on this host (2026-10-09, phase-6 S5) reaches match start and
shows the **match HUD on screen** (bar + score 0-0 + `00:00` at kickoff in
`docs/screens/s5-v6-match-hud.png`); a KICK burst reaches the row-01 kickoff
release gate, so the live clock starts ticking (`00:02` in
`docs/screens/s5-v6-match-clock.png`) — the natural phase-1 → 2 transition
visible through the HUD. The scene sprites stay in the placement pose
(movement is still not observable interactively: a 40-press RIGHT burst
leaves the scene band byte-identical, `compare -metric AE` = 0, while the
presses reach `input_state[0]`; the live kickoff record carries the native
0x19 code and the SDL hold policy drops auto-repeat — OL-T4-1 — so the row-04
pad arm still meets an inactive record). Kick → score stays blocked
interactively: the S2/S3 goal chain is landed but has no natural producer on
the static camera (pan origin OL-T11-79 absent), and the score source is
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
S1–S5 execution + Track B read-only recon-ahead) and the SDD workspaces under
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

`make check` configures, builds `-Wall -Wextra -Werror`, and runs all 106 CTest
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
- Action dispatch (FU-137): **14/80 rows wired** — `00`, `01`, `04`, `06`,
  `07`, `08`, `0F`, `18`, `1E`, `21`, `23` (playability G1 + arms-and-wiring
  G3; `01` is the M2 playable-match Task 2 kickoff taker) and
  `26`, `28`, `2A` (cluster G); dispatch results 65 UNSUP / 14 OK / 1 NOTF.
- **Interactive smoke (phase-6 S5 re-run, this host 2026-10-09; the
  follow-up-5 T4 run first verified the walkthrough and the follow-up-4 G4 run
  first verified Task 13):** `make game` window opens (960×720
  integer-scaled SDL3; ESC quits, exit 0 — verified in a clean run; the intro
  and the procedural front-end draw). The walkthrough reaches **match start** —
  intro RETURN skip → front-end BACKSPACE (DECLINE/panel) → RETURN (panel
  confirm → FU-66 STATE16 bridge) — with the derived palette (OL-T11-6) and
  the **match HUD** (P0.2/OL-T11-7) drawn on screen: the Frames.fsh bar at the
  bottom-left with the score pair (0-0) and the `%02d:%02d` clock at kickoff
  `00:00` (`docs/screens/s5-v6-match-hud.png`). **A KICK burst latches row
  01's native release gate**, so the begun run leaves the kickoff wait for the
  live phase 2 and the clock ticks on screen (`00:02` in
  `docs/screens/s5-v6-match-clock.png`; the HUD band changes again over 2 s,
  AE 185, pinning the running clock). (The SDL keyboard path presents a hold
  as single press pulses — the backend filters key auto-repeat, OL-T4-1 — so
  the run used `xdotool key --repeat` bursts.) **Movement is still not
  observable on screen:** a 40-press RIGHT burst leaves the scene band
  byte-identical (crop `y < 591` of the 960×720 window, `compare -metric AE` =
  0; only the HUD clock changes, full-window AE = 1311 over the sample) though
  the presses do reach `input_state[0]` (the T4 in-process probe observed
  `in=04`; the headless fixture and the v6 tape pin the pad → target →
  velocity → position seam). The slot-bound record carries the native 0x19
  code after the kickoff transition (the `0x7DA26` inactive-record mapping;
  row 19 is UNSUP), so no wired row consumes the pad for it; the natural pad
  consumer (row 04's slot-dir arm) is producer-complete since S1 (FU-147) but
  the live path stays gated by the 0x19 code and the hold policy. The
  reachable pad → target → velocity → position seam is exercised headlessly
  (`test_engine_match_frame::test_pad_drives_controlled_locomotion`) and is
  what drives the tape's mover-integrated motion.
  **Kick (gameplay) and score stay blocked:** the KICK press that lands the
  kickoff transition dispatches no gameplay row (the possession/selection
  invokers are unported) and a natural gameplay goal still has no *producer*:
  the arming→scan→queue chain (FU-145, S2) is dormant on the static tape camera
  (pan origin OL-T11-79 absent) and the set-piece scanner arms landed in
  phase-7 P1 (FU-149: throw-in/corner/goal-kick reach their phases + taker
  codes; the tape camera never arms one). The
  goal **consumer** chain was landed by S3 (FU-146): begin installs the
  goal-screen machine (`0x92D8C/0x92E2C`), the frame body runs the
  session-gated scheduler (`FUN_000948AC 0x4B1A1`, before the clock body), and
  a queued goal id is consumed by the installed period handler into
  `fifa96_match_run_score_event` — the score increments through the native
  chain in a fixture
  (`test_engine_match_frame::test_goal_consumer_chain_fixture`), while both
  goldens stay byte-identical (the tape camera never arms, so no natural goal
  enters the tape). `fifa96_match_run_goal_queue`'s direct fallback now honours
  the native `[0x157AC2] in {2,3}` phase-5 skip. The S5 smoke shots' measured
  content: 3.60% non-black window pixels (the kickoff frame: 666287/691200
  black), dominated by the HUD bar's `#900808` (11301 px); the sprite color
  `#E044A0` (the OL-T11-6 6-bit `0x38/0x11/0x28 << 2`) is live (1200 px).

  Reached vs blocked (S5 smoke, 2026-10-09):

  | step | state | evidence |
  |---|---|---|
  | window + intro + front-end draw | reached | `s5-v6-frontend.png`; ESC exit 0 (clean run) |
  | panel DECLINE/CONFIRM → match start | reached | match canvas replaces the front-end; `s5-v6-match-hud.png` |
  | RGB palette on the match canvas | reached | `#E044A0` sprite pixels; tape frame-6 palette assertion |
  | match HUD (bar/score/clock) | reached (on screen) | `s5-v6-match-hud.png` (0-0, 00:00); tape frame-6 bar-pixel assertion |
  | kickoff → phase 2 naturally | reached (on screen) | KICK burst → clock `00:02` in `s5-v6-match-clock.png`; tape v6 `run_natural_probe` (phase 2 at step 215, row 01 dispatched) |
  | move the controlled player | blocked on screen | 40-press RIGHT burst: scene-band AE = 0; presses reach `input_state[0]` (`in=04`) but the kickoff reset leaves the record on the native 0x19 code (row 19 UNSUP) and the SDL hold policy drops auto-repeat (OL-T4-1); the S1 pad seam is fixture-proven headlessly |
  | kick the ball (gameplay) | blocked | no gameplay row dispatched (tape steps 12/15); possession/selection invokers unported |
  | score a goal | blocked naturally; chain fixture-proven | no goal producer reachable (pan origin OL-T11-79 dormant); the S2/S3 chain (armer → scan → queue → installer → scheduler → handler → `score_event`) increments the score in `test_camera_pan_event_chain` / `test_goal_consumer_chain_fixture` |
  | half/period end → exit | reached (headless) | tape: live class-1 period end → phase 0x0C → OVER→POST→EXIT; native periods last minutes, so not run to completion in the smoke |
- `test_engine_m1` pins the 691-frame M1 transcript
  (`tests/golden/engine/m1-frames.txt`).
- `test_engine_m2` replays spec §5 (boot → skip intro → front-end → start match
  → kickoff → move → kick → score → period end → exit to front-end) and pins
  the 165-frame M2-B transcript (`tests/golden/engine/m2-frames.txt`):
  `frame=<n> hash=<hex>` plus `state=<phase>/<home>-<away>` while a match is
  live, the forced kickoff phases 0x13/0x14, the wired-row `FIFA96_OK` dispatch
  set, and the score step. Tape **v6** (the phase-6 full-gameplay acceptance,
  S5; v5's M2 interactive G3 acceptance and the v4 G4/v3 provenance are
  retained) stages the
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
  **byte-identical** (no re-pin): the tape camera never leaves the reset
  triple, so the new armer never fires and the clock-tail goal scan is a gate
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
  no natural invoker, so the tape camera still never moves. The S2 L1 chain is
  now producer-proven (`test_engine_match_frame::test_camera_pan_event_chain`:
  the real `fifa96_camera_event_set` pans the integrator into the arming band
  → queued situation 5 → S3 consumer score).
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
  byte-identical. Regenerate with
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
  open: the remaining `OL-84` situation-0xB producers (rows 02/0x10..0x13) and
  row 01's event/camera/ball-stage sinks, plus the FU-73 keeper/restart
  producers
  `0x7546E`/`0x75B58`/`0x76072`; `OL-85` extra-time flag wiring; `OL-87`/
  `OL-88`/`OL-89` goal invokers (Task 3 verdict: no invoker reachable from the
  ported state — FU-142 App. L.9); `OL-81`/`OL-83` row-field wrinkles; `OL-82`
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
  carrier-bit (`0x7DA42`) -> next-dispatch hand-off, fixtured. The tape is
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
  Carried legs (FU-145 §6): L1 the pan source/camera director (S4 if
  camera-scoped) and the armer head counter/`FUN_00070DE0`; L2 `FUN_00092998`;
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
  `[0x14C32A]` producer (leg 2), the tracked-side flags (leg 4), the
  `FUN_000CBC4C` cells' live-native verification (leg 5), the `FUN_0009252C`
  display gate (leg 6), the `FUN_000935A0` thresholds/exits (leg 7), the
  `[0x15B684]` mode byte (leg 10). The `[0x15B6B8]` side flag is write-only
  (fresh xrefs: its two writes) and is a leg.
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
    corner/tracked-player tails stay legs (OL-T11-79).
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
  producer, shade cube); plus carried `OL-T2-1…3`, `OL-T4-1` (SDL hold policy),
  `OL-84`'s remaining situation-0xB producers / row-01 sinks, `OL-85/86`,
  `OL-87/88/89` (superseded in part by S2/S3; keep the goal-invoker register),
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
  **landed in M2 full-gameplay S1** (FU-147). Carried (T4 smoke): **OL-T4-1**, the
  SDL hold policy — the backend drops key auto-repeat
  (`src/fifa96_engine/platform_sdl3.c:194`), so a held key arrives as press
  pulses and live on-screen movement needs a repeat/hold policy or the gamepad
  path, landing with the row-04 pad arm in the S1 possession/locomotion port
  (FU-147/S1). The goal chain that blocks the tape's
  score step (`OL-87`/`OL-88`/`OL-89`) is owned by the next plan phase via the
  frozen FU-145 (arming) / FU-146 (consumers) slices (phase-6 S2/S3).
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
- **Unwired rows (66/80).** 65 rows dispatch `-FIFA96_ERR_UNSUPPORTED`: 27
  unported action rows, 34 phase rows (derived and ported at the loader level by
  FU-143 but not wired into the engine dispatch), the unwired actions
  `27`/`29`/`2C` (ported bodies, no installer entry) and the dead entry `2B`;
  phase `0x16` is the native zero/INT3 slot and the single
  `-FIFA96_ERR_NOT_FOUND`.
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
  the tape's pre-score assertions. Open legs: **OL-87** (handlers + scheduler +
  installer), **OL-88** (camera-pan goal detection), **OL-89** (posted-id
  dispatch `FUN_0009252C` and the `FUN_000CBC4C` probe). The engine
  carries tracked side -1 (`add_goal`-equivalent) until OL-87 lands;
  `fifa96_match_run_add_goal` remains for those unported paths.
- Retail front-end art asset (no OPTIONS-like path exists in the ISO).
- Per-row cluster legs carried in FU-139/FU-141/FU-142 (`OL-56`…`OL-71`:
  unmodeled record bytes, process globals, camera/track inputs, roster
  descriptors, animation selectors).
