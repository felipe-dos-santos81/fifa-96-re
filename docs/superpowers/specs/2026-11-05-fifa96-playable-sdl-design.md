# Playable SDL port — the visible match, usable menu, audio (2026-11-05)

**Status:** proposed. Supersedes no prior section; the phase-53 register stands.

## 1. Goal

`make game` must launch a FIFA 96 the user can actually play: a readable
front-end, a visible pitch with both teams, working controls, a legible score
HUD, pause/result/return, and match audio. The simulation and the native
dispatch census already exist; this phase is the presentation and shell.

Measured starting point (fresh, this session):

- `./build/fifa96 data/game/FIFAPCCD96.iso` boots the intro and the menu.
- The menu is a placeholder: yellow header, 8 unlabeled rows, red cursor;
  `X` enters a panel, `Return` starts a match (the only path).
- The match renders black except one ~15 px player sprite, the ball, and a
  60x41 HUD bar at the bottom-left. The sim, input and camera pan are live.
- The diag (`/tmp/opencode/diag_match.c`) shows 23 entities, camera
  `(0,0,0)`, `tracked=-1`, all 22 records at `x≈2012, z≈0..12`.

## 2. Root causes (fresh recon, reports in `/tmp/opencode/play/`)

1. **Wrong render eye.** The native renders from camera object 0
   (`0x107508`: `(-1920,160,30)`, yaw `0xBF9C`, pitch `200`, ratio `0x1500`);
   the port projects from `render.camera.pos_*`, the FU-71 focus triple
   `(0,0,0)`. At `eye_y=0` the floor is degenerate and every player projects
   onto the horizon (A1/A2/A7).
2. **No floor/stadium renderer.** The native pitch is a perspective floor
   caster using a 64x64 tile (`FplainH/V` from `GAMEFLD3`) plus
   `ST*covr` crowd tiles (`GAMESTD1`); the port only clears
   `render.background` and draws sprites (A1).
3. **Kickoff cutscene collapse.** `period_setup(1)` arms act `0x0A`; its
   phase-`0x20` body installs action `0x24` on all 22 records, writing
   `pos_x=0x9F0` / `target_x=0x7E0+6*player_e` (x≈2016) and erasing the
   formation seed. The fabricated `0x3E8` wait keeps it running ~1670
   steps; the native wait is 30 granted frames (A2/A7).
4. **HUD colours/names.** Bar geometry (60x41 at `(2,197)`) is exact, but
   the text passes palette slot numbers 6/0 as indices (black on red), and
   `render.hud_name[2]` is never filled. The native source is the PCCD/PCDB
   team-name triple (A4).
5. **Front-end is the wrong screen.** The native boot screen is Game Select
   (9 items), labels resolved from `LANG000.POG` via `FUN_0001771C`; the
   font `HELV12.POG` parses with the ported `fifa96_font`; `MOD2.POG` frame
   0 is the real background. Team select is cheap: the 44 national-team
   names are LANG ids (A3).
6. **Audio is fully ported but unwired.** `audio_open` exists; no
   `audio_submit` caller. `SFX_GAME.BNK` id 0x1E is the whistle; crowd is
   the `CHN_*` 2-slot chant machine (id 55 on `CHN_GEN1`). The missing link
   is cue -> `fifa96_sfx_play_id` -> `fifa96_mixer_render` -> platform
   submit (A6).
7. **No pause/result.** Full time resolves and returns to the menu with zero
   POST frames; ESC exits the process. The lifecycle hooks exist (A5).

## 3. Slices

### P1 — the visible match (match visuals)
- `live_start` (default 0) set only by the windowed bridge: skip the
  `period_setup(1)` collapse, enter phase 2, seed the camera event/tracked,
  and render the live match from `eye = focus + (-1920,160,30)` while the
  focus follows the ball. The M2 path stays ungated and byte-identical.
- New `fifa96_pitch.c/.h`: stage `GAMEFLD3.PVI` (`FplainH/V`) and
  `GAMESTD1.PVI` (`STACcovr`), draw the perspective floor + a v1 crowd band
  + white markings before the sprite pass, clipped to the render window.
- Real player colours: `PALteam.fsh` frame 0 as the palette base and a
  derived team-1 remap; both teams distinct.
- Fix the live formation/cutscene and camera follow; all 22 records at
  their formation positions, tracked on the controlled record.
- Acceptance: a windowed kickoff shows the pitch, both teams, ball, HUD; the
  controlled player moves with the arrows; the camera follows; M1 exact; M2
  v29 exact (the changes are `live_start`-gated) unless a move is measured
  and re-pinned with the usual written reason.

### P2 — the usable menu
- Real background/art (`MOD2.POG` frame 0) and labels from `LANG000.POG`
  (`HELV12.POG` font), a Game Select home screen with Friendly / Options /
  Quit, and a team-select screen using the 44 LANG national names.
- Pass the chosen teams/formation/names to `fifa96_match_run_begin` (via the
  existing setters and `render.hud_name`), then start the match.
- Keep the existing hash-pinned fallback menu intact (M1/M2).
- Acceptance: keyboard-only navigation to a match, readable labels, chosen
  team names in the HUD; `make check` green.

### P3 — match audio
- New `fifa96_match_audio.{c,h}`: load `SFX_GAME.BNK` + `CHN_GEN1.BNK` via
  the cache, loop crowd id 55, play whistle 0x1E on the `ref_whistle` edge,
  render 220/221 frames per 10 ms tick into `plat->audio_submit`.
- Wire init/step/destroy in the engine; keep the null backend silent and
  deterministic.
- Acceptance: audible crowd + whistle in the windowed game; null audio hash
  unchanged unless measured; `make check` green.

### P4 — HUD, pause and result
- HUD text colours (native black shadow / bright body), team names in the
  HUD, and the score/period/clock verified against FU-148.
- ESC pause (pace hold) with resume/quit; POST result panel with score,
  dwell, then return to the front-end.
- Acceptance: a complete menu -> match -> pause -> full time -> result ->
  menu loop; the M2 tape unaffected by lifecycle gating (or measured
  re-pin).

### P5 — integration, play-test, docs
- Merge the slices, run `make check`, play through with real input and
  screenshots (intro, menu, kickoff, open play, goal, pause, result), fix
  the integration defects, update README/ENGINE/PROGRESS with controls and
  the new legs.

## 4. Invariants

- M1 `09b726b7…` immutable. M2 v29 `7c3874f7…` moves only with a measured
  reason + frame diff + state strata; the live-only changes are gated.
- Census unchanged (78/78, 58/80, 64/80, 6 blocked, action 38/45, phase
  26/35); no dispatch row is wired by this phase.
- Every native claim cites the recon report/address; derived stand-ins are
  labeled and numbered as legs. The field/stadium renderer v1 is derived
  (the native scanline caster is the reference, not a line-by-line port).
- `make check` green at every commit; work lands on `main` (worktree
  isolation is merge-only).

## 5. Out of scope

- The native-fidelity POST/pause armer, full commentary, CRD music, the
  PCCD team-id mapping, the referee entity, and the exact 3dwpts stadium
  projection — all named legs.
- Online/modem, tournaments/leagues, and the other five native front-end
  screens.
