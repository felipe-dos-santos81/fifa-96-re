# FIFA 96 — complete single-player playable port (2026-11-07)

**Status:** proposed. Master plan for the completion wave; each numbered phase
gets its own slice plan when implementation starts. Prior P1–P12 registers
stand.

## Goal

The shipped Release build (`make game`) must be a complete single-player FIFA
96 shell and match, not a Friendly-only demo:

- every Game Select entry except Modem Setup starts a real flow; Modem Setup
  is an honest offline notice (native network play remains a leg);
- Leagues, Tournaments and Playoffs run a season/bracket with playable and
  simulated fixtures, standings and a champion;
- Practise starts a no-stakes playable session;
- Load Game stores and restores competition progress and settings;
- the match has no dead end: the human can move with the ball, the ball can
  leave play and restart, and a tackle button exists;
- the 640 HUD is uniformly scaled and the goal net is on the native goal
  plane.

Native-fidelity items not required for this functional definition stay
numbered legs (native 3dwpts bowl, full camera director, native competition
screen art, native `SGAME%.2d.DAT` body, modem networking, full 10-player AI,
native penalty shootout).

## Phase 1 — core match playability (first; blocks all modes)

Files: `port/src/fifa96_engine/fifa96_match_run.{c,h}`,
`fifa96_match_handlers.c`, `fifa96_keys.h`, `platform_sdl3.c`,
`port/src/fifa96_loader/fifa96_action_handlers.c`, tests
`test_engine_live_control.c`, `test_engine_match_frame.c`,
`test_engine_match_input.c`, new live restart tests.

1. **Carrier dribble.** Live-only: when the controlled record is the ball
   carrier and a direction is held, target `pos + dir*0x80` with native pitch
   clamps instead of the camera triple. Acceptance: a live claim + held
   direction moves the record and keeps the ball within carrier range; M1/M2
   byte-identical.
2. **Ball-based out-of-play.** Live-only boundary armer from the ball
   (`|x| > 0x7E0` touchline, `|z| > 0xC1E` goal line, goal mouth); disable the
   camera-snapshot live armer. Acceptance: inside-line ball does not arm; ball
   out at x and z reaches throw-in/corner/goal-kick; goal mouth still scores.
3. **Restart resume.** After a live throw/corner/goal-kick, if the native taker
   row holds without a slot, place the taker/ball and return to phase 2 within a
   bounded window. Acceptance: throw-in and goal-kick reach phase 2; score and
   clock unchanged except the restart; test both sides.
4. **Tackle input.** New `FIFA96_ENGINE_KEY_TACKLE` (keyboard key and pad
   North) mapped to native want `0x60`; it reaches the existing contact chain.
   Acceptance: input mapping test + live contact/tackle smoke; no menu
   regression.
5. **Half-time and keeper distribution.** Live half-time releases after a
   bounded dwell or KICK/C; a saved ball is distributed to a defender instead
   of frozen. Acceptance: full-time win/draw still passes; keeper save no
   longer requires chasing.
6. **CPU one-action.** Opponent carrier gets one derived pass/shot after a
   bounded hold. Acceptance: forced opponent possession produces a ball action
   and no score corruption; live-only.

## Phase 2 — competition model + save (pure, no UI)

Files: new `port/src/fifa96_engine/fifa96_competition.{h,c}`,
`fifa96_save.{h,c}`, tests `test_engine_competition.c`,
`test_engine_save.c`, `port/CMakeLists.txt`.

- Eight-team round-robin league: circle schedule, play/simulate, 3/1/0
  standings, deterministic simulated results, champion.
- Eight-team single-elimination cup for Tournaments/Playoffs: bracket, play/
  simulate, draws replay (bounded) then deterministic shootout, champion.
- Practise descriptor: selected team, no fixture, no standings.
- Save record: magic/version/length/CRC, mode/team/fixtures/standings plus the
  existing settings block; ten slots `SGAME00.DAT`..`SGAME09.DAT` under a
  caller-supplied directory; atomic write; corrupt/truncated rejection.
- Acceptance: pure tests cover schedule, standings, bracket, draw path, slot
  round-trip, corruption, and settings; no ISO/filesystem needed for the pure
  test; `make check` green.

## Phase 3 — front-end flows and derived art

Files: `fifa96_menu_model.{h,c}`, `fifa96_frontend_run.{h,c}`,
`fifa96_menu_art.{h,c}`, tests `test_engine_menu_model.c`,
`test_engine_frontend.c`, new `test_engine_frontend_modes.c`.

- Enable Leagues/Tournaments/Playoffs/Practise/Load Game and add the hub,
  load/save and Modem info screens; Team Select is single-team for competition
  and Practise.
- Hub rows: Play next, Simulate next, Table/Bracket, Save game, Exit. Bracket/
  table is a read-only list screen.
- Generic list screen in the hires and 320 art paths, using HELV12 and the
  existing MOD2/team palette. Native competition screen art stays a leg.
- Load Game: ten slots with summaries; Save Game reuses the slot screen in save
  mode; Modem Setup shows the offline notice and backs out.
- Acceptance: synthetic keyboard-only flows model -> Team Select -> hub ->
  match_start for all modes; load/save and back routes; disabled-entry test
  removed only for the newly live entries; M1/M2 untouched.

## Phase 4 — engine integration, modes, post-match

Files: `fifa96_match_bridge.{h,c}`, `fifa96_engine.c`,
`fifa96_engine_internal.h`, `fifa96_frontend_run.{h,c}`,
`fifa96_match_run.{h,c}`, `test_engine_frontend_modes.c`,
`test_engine_match_bridge.c`, new `test_engine_mode_flow.c`.

- Bridge passes the selected mode to the match run; Practise sets the session
  gate after `begin`; competition matches run the same native match engine.
- On match exit copy the result into `final_score[2]` before lifecycle
  teardown (teardown zeros `score[]`), apply it to the competition, auto-save
  slot 0, and return to the hub; Friendly returns Game Select; Practise
  returns Game Select. The ten derived slots are `F96SAVE%.2d.DAT`, never the
  native `SGAME%.2d.DAT`.
- Cup draws replay; league draws stand. Side-swap score mapping is tested.
- Load Game loads competition + settings and returns to the hub.
- Acceptance: synthetic engine flow starts each mode, completes a result
  (staged), applies it, saves, resets to the right screen; existing Friendly
  tests pass; no golden movement.

## Phase 5 — 640 HUD and goal-net plane

Files: `fifa96_match_run.c`, tests `test_engine_match_render.c`.

- Uniform 2x live HUD at 640: 120x82 bar at (4,394), 24px rows, 48px clock,
  scaled font, 320 path byte-identical.
- Keep the derived goal net on the goal-line plane `z=±0xC1E`: the classifier
  `0xB10..0xB90` is a volume, not a plane; moving the net to `0xB10` would put
  it in front of the line. The net-plane alignment remains a labelled
  review item, not a blind change.
- Optional if the core lands early: bounded 3dwpts 35-quad bowl (separate
  slice); full native bowl stays P10-L2.
- Acceptance: 640 live render test pins the HUD rectangle and text scale; net
  plane test; M1/M2 exact; screenshots updated.

## Phase 6 — completion gate

- `make check` green (new tests included), M1/M2 exact by `cmp`.
- Release `make game` smoke on `DISPLAY=:1`: intro -> Game Select -> Friendly,
  Leagues play + simulate + save/load, Tournament, Playoffs, Practise, Options,
  Load Game, Modem notice; screenshot each flow.
- Update README, `docs/PROGRESS.md`, `port/ENGINE.md` with the new mode map,
  derived/native labels, and remaining legs; no prior section rewritten.
- Commit per phase on `main`, push; keep only `main`.

## Non-goals / remaining native legs

- Native 3dwpts point/quad bowl and full native competition screens/art.
- Native camera director/easing and native `SGAME` body layout.
- Full 10-player AI, native open-play keeper dive, native penalty shootout.
- Modem/network play. The UI must say so; it is not a silent disabled label.
- Native save files are never overwritten; the derived format uses a distinct
  file/version.

## Completion record (2026-11-07)

- Core match: live carrier dribble, ball-based touchline/goal-line exits, a
  bounded taker fallback, half-time panel/auto-kick, final-score preservation,
  and the uniform 2x 640 HUD landed. `test_engine_live_control`,
  `test_engine_match_render` and `test_engine_match_completion` pass; M1/M2
  exact. The shared contact tackle remains automatic; a separate tackle key
  was not added because the native `0x60` is a want-word, not a new bit.
- Modes: `fifa96_competition` (League/Tournament/Playoff/Practise),
  `fifa96_save` (ten `F96SAVE` slots), the menu/hub/list/Load/Save/Modem
  screens, and the engine result/mode/save handoff landed. New tests:
  `test_engine_competition`, `test_engine_save`,
  `test_engine_frontend_modes`, `test_engine_mode_flow`.
- Gate: `make check` 123/123; M1/M2 exact; Release smoke menu, League match
  (half-time/second half), hub, Load Game, slot 0 load, Simulate next,
  Table/Bracket and Save Game. `test_engine_mode_flow` stages the engine
  POST->EXIT full-time path and proves result application, auto-save and the
  hub return. The native competition art/data, native save body, modem
  transport, 3dwpts bowl, camera director/lens, native HUD art, full
  AI/fouls/offside proof, substitutions and native penalty shootout remain
  numbered legs.
