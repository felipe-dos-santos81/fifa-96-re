# Playable SDL port — implementation plan

**Spec:** `../specs/2026-11-05-fifa96-playable-sdl-design.md`.
**Recon:** `/tmp/opencode/play/A1..A7-*.md` (fresh, cited).
**Baseline:** HEAD `755c2c2`, `make check` 112/112, M1 `09b726b7…`,
M2 v29 `7c3874f7…`, census 78/78, 58/80, 64/80, 6 blocked.

---

## Task 1 — P1: the visible match (main branch)

**Files:** `port/src/fifa96_engine/fifa96_match_run.c/.h`,
`fifa96_match_bridge.c`, new `fifa96_pitch.c/.h`,
`port/CMakeLists.txt`, `port/tests/test_engine_match_render.c` (new
cases), `port/ENGINE.md`, `docs/PROGRESS.md`.

1. **Live-start gate.** Add `live_start` (default 0) + setter; the bridge
   sets it only when the engine has a surface and is not headless. In
   `begin`, for `live_start` skip the `period_setup(1)` that arms act `0x0A`
   (the action-`0x24` collapse) — keep the kickoff placement commit and
   write live phase 2; seed one camera event and bind `tracked` to the
   controlled record. Ungated code paths keep M2/M1 byte-identical.
2. **Render eye.** For `live_start`, render from
   `eye = focus + (-1920,160,30)` (camera object 0) and keep
   `render.camera` as the focus, following the ball
   (`focus = ball`, `pos_y = 0`); for the non-live path use the current
   `camera` triple. The pitch and sprite passes must use the same eye.
3. **Pitch module.** New `fifa96_pitch.c/.h` staged from `/ART/GAMEFLD3.PVI`
   (`FplainH.fsh`/`FplainV.fsh`, 64x64, copied into an owned buffer) and
   `/ART/GAMESTD1.PVI` (`STACcovr.fsh`). Draw: sky/crowd band, then the
   inverse-projection floor (`306`/`240` loop with one divide per pixel,
   using the existing `fifa96_projection_matrix` transpose), then the white
   markings (goal lines `x=±2016`, touchlines `z=±3102`, halfway line,
   centre circle `r≈0x3C0`), clipped to `render.window`/surface. Add the
   staging path to the bridge. Default-off for the M2 path (render only
   when `live_start`/pitch staged).
4. **Colours.** `PALteam.fsh` frame 0 as the sprite palette base (fix the
   magenta player); a derived team-1 band remap (16 entries into the free
   230..245 slots) so both teams differ. Ball/HUD keep the current remap.
5. **Camera/formation.** After the gate, all 22 records keep their
   formation targets; the focus follows the ball and the tracked record is
   the controlled one. Do not add a referee entity.
6. **Tests + acceptance.** New render probes: after a live begin + 1 frame,
   not all 22 positions equal, the pitch frame has >30% non-background
   pixels, both teams' sprites differ in colour, the eye is the focus
   offset. `make check` green; M1 exact; M2 v29 exact (if a measured move
   is unavoidable, re-pin with the written reason + frame diff and update
   the pins, never M1).
7. **Docs.** The live-start/eye/follow derived legs, the pitch v1 legs
   (native scanline caster, exact texel scale, 3dwpts stadium, field/stadium
   selection), and the palette/kit legs.

---

## Task 2 — P2: the usable menu (worktree `/tmp/opencode/wt-front`)

**Files:** `port/src/fifa96_engine/fifa96_menu_art.c/.h`,
`fifa96_frontend_run.c/.h`, `fifa96_engine.c`, new
`fifa96_lang.c/.h` (or equivalent LANG000/HELV12 decode), `CMakeLists.txt`,
tests, docs.

1. Decode `LANG000.POG` (RefPack -> `"LANGO"` + base `0x94C` + u32 offset
   table, 591 NUL strings) and `HELV12.POG` (FNTI via `fifa96_font_parse`);
   expose `fifa96_lang_string(id)` and the font.
2. Draw the real front-end: `MOD2.POG` frame 0 background (fallback to the
   current OPTIONS.INV/placeholder), a Game Select list with the native
   labels (Friendly 0x81, Options 0xD8, Quit 0xF3; the rest may be shown
   disabled/legs), and a team-select screen with the 44 national names
   (Brazil 0x23, Italy 0xA6, England 0x98-ish ids from A3; use the ids the
   report lists).
3. Navigation/handoff: arrows + Return; Friendly -> team select -> stage
   the two team names into `render.hud_name`, the formation ids via the
   existing setter, and start the match; Options/Quit may be labeled
   stand-ins; keep the existing hash-pinned fallback path intact.
4. Acceptance: keyboard-only start, readable labels, both chosen names in
   the HUD; `make check` green; M1/M2 untouched (the menu changes are
   engine-level, not the match tape).

---

## Task 3 — P3: match audio (worktree `/tmp/opencode/wt-audio`)

**Files:** new `port/src/fifa96_engine/fifa96_match_audio.c/.h`,
`fifa96_engine.c`, `fifa96_engine_internal.h`, `CMakeLists.txt`, tests.

1. Load `/SOUND/SFX_GAME.BNK` and `/SOUND/CHN_GEN1.BNK` via the cache; arm
   the crowd loop (CHN id 55, gain 0x60) and play SFX id 0x1E on a whistle
   edge; render 220/221 frames per 10 ms tick into `plat->audio_submit`.
2. Wire init at match start, step from the engine frame, destroy at exit;
   keep the null backend deterministic (no audio submit or a fixed silent
   path under headless).
3. Tests: the module loads/arms on the ISO; the mixer output is non-silent;
   the null audio hash is unchanged for the tapes.
4. Acceptance: audible crowd + whistle in the windowed game; `make check`
   green; no golden movement.

---

## Task 4 — P4: HUD, pause, result (after P1)

**Files:** `port/src/fifa96_engine/fifa96_match_run.c/.h`,
`fifa96_engine.c`, tests, docs.

1. HUD text: native colours (black shadow, bright body — a palette index
   chosen near-white/light, not the slot number), team names from
   `render.hud_name`, the score/period/clock verified; a unit test for the
   colour indices.
2. Pause: ESC in MATCH pauses via the pace hold, draws a Pause panel
   (Resume / Quit to Menu), Return resumes, X quits to the front-end.
3. Result: on lifecycle OVER/POST, freeze the match, draw a FULL TIME panel
   with the score/period/names for a short dwell, then return to the
   front-end; ESC skips to the menu. Gate so the M2 tape is unaffected
   (lifecycle-keyed, not phase-keyed).
4. Acceptance: menu -> match -> pause -> resume -> full time -> result ->
   menu loop measured in the windowed game; `make check` green; M2 v29
   exact or a single measured re-pin.

---

## Task 5 — P5: integration, play-test, docs

- Merge/land all slices on `main`, resolve conflicts, run `make check`,
  verify M1/M2, then play a full match with real input and capture
  screenshots (intro, menu, kickoff, open play, goal, pause, result).
- Update README (controls + `make game` play notes), ENGINE (new register,
  legs), PROGRESS; list the open legs honestly (native pitch caster,
  PCCD team mapping, commentary, CRD music, POST armer, referee).
- Acceptance: the user can start the game, pick teams, play a match with
  visible pitch/players/HUD, pause, finish, and return to the menu.

---

## Global constraints

- M1 `09b726b7…` immutable. M2 v29 `7c3874f7…` only moves with a measured
  reason, frame diff and state strata; live-only changes stay gated.
- Census unchanged; no dispatch row wired.
- `make check` green at every commit; strict `-Wall -Wextra -Werror`.
- Every derived stand-in labeled; every native claim cited (recon reports
  or fresh reads). No fake producer.
- Worktrees merge to `main`; no force pushes; push at the end of each slice.
