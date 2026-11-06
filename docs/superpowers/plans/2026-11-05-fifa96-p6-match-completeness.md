# P6 implementation plan — match completeness

**Spec:** `../specs/2026-11-05-fifa96-p6-match-completeness-design.md`.
**Recon:** `/tmp/opencode/play/P6-team-identity.md`, `P6-goal-path.md`,
`P6-options.md`.
**Baseline:** HEAD `7a6c56c`, `make check` 115/115, M1 `09b726b7…`, M2 v29
`7c3874f7…`, census 78/78, 58/80, 64/80, 6 blocked.

---

## Task 1 — real team identity (main)

**Files:** new `port/src/fifa96_loader/fifa96_pccd.c/.h`; new
`port/src/fifa96_engine/fifa96_team_data.c/.h`;
`fifa96_match_run.c/.h`, `fifa96_match_bridge.c`,
`fifa96_frontend_run.c/.h`, `fifa96_menu_model.c/.h`, `port/CMakeLists.txt`,
tests (`test_engine_pccd`, `test_engine_team_data`, `test_engine_menu_model`,
`test_engine_match_render`), `port/ENGINE.md`, `docs/PROGRESS.md`.

1. `fifa96_pccd`: parse/validate `PCDB` (team block `0x4B4`/`0x4BC`, player
   `0x8A06`, pool `0x2866A`, mode offsets `0x1BC..0x2B3`); `team(mode,index)`
   bounds-checked; fields `id/name_long/short/medium/formation/kit[4][3]/
   print[4]/roster[16]`; `player(id)` optional. No I/O in the loader.
2. `fifa96_team_data`: read `/FEDATA/PCCD.POG`, decode the RefPack, parse;
   own the arena; the 44 LANG-name → mode-4 index table (report §4); lookup
   returns the identity. Soft-fail without the ISO.
3. Front-end: `pccd_index[2]`, `pccd_record[2]`, `pccd_id[2]`,
   `team_code[2][10]`, `formation[2]`, kit triples + print colours; the menu
   model maps its 44 entries to the LANG id / mode-4 index; the code comes from
   the PCCD record `+8`, not the LANG name.
4. Match: `fifa96_match_run_set_matchup` before `begin`; apply after the
   begin reset; seed the home `.fmt` for both teams and reseed team 1 from its
   own `.fmt` only when the formations differ; the post-begin `set_formation`
   stays. All of this inside the `team_selected`/live path.
5. Live kit: `match_run_stage_live_kit` fills the team 0/1 bands from the
   selected kit-0 triples via the `PALteam` frame/ramp mapping; no fixed
   frame-0/2 overlay.
6. Tests: retail PCDB decode (ISO-gated), Brazil/Italy identity (formation,
   codes, kit triples), matchup formation targets per side, HUD codes, kit
   pixels differ, menu handoff; M1/M2 exact.
7. Docs: a P6 register + the identity legs (roster/AI, auto kits, translation
   pool, stadium/field, CTEAM).

**Accept:** a windowed Brazil vs Italy match shows the PCCD codes/formations
and distinct kits; `make check` green; M1/M2 exact.

---

## Task 2 — live goal path (worktree `/tmp/opencode/wt-goal`)

**Files:** `fifa96_match_handlers.c`, `fifa96_match_run.c`,
`test_engine_match_handlers.c`, new `test_engine_live_goal.c`,
`port/CMakeLists.txt`, `port/ENGINE.md`, `docs/PROGRESS.md`.

1. Celebration release: use `mr->global_157a4c`, not `r->side`
   (`handlers.c:6206`); update the handler test (the skip case sets the flag;
   side 1 with flag 0 releases).
2. Fallback side: `match_run_situation_side(..., 6, side, 0)` (native
   `0x8AC28` writes the goal side).
3. Live producer: after the phase-2 ball update and before the goal scan,
   when `live_start` and the ball enters the native goal zone, set
   `goal_armed/snap/zone` (label P6-L1 stand-in); set the live camera
   `pos_y = 0x50` (native `[0x157750]` gate) and keep a forward lead so the
   native pan path can also arm. No fabricated score.
4. `test_engine_live_goal`: live begin; stage the ball at `(0,0,±0xB30)`;
   assert score/phase 5/celebration/kickoff/phase 2 for both sides; score
   preserved; no re-score; half-time/full-time score survival.
5. Windowed capture with the verification harness (goal, celebration,
   kickoff, full time).
6. Docs: the goal-path register and legs (native camera director, replay,
   captions).

**Accept:** both sides can score live and restart; `make check` green; M1/M2
exact.

---

## Task 3 — options and controls (after Task 1)

**Files:** `fifa96_menu_model.c/.h`, `fifa96_menu_art.c`,
`fifa96_frontend_run.c/.h`, `fifa96_match_bridge.c`,
`fifa96_match_audio.c/.h`, `fifa96_match_run.h/.c`, tests
(`test_engine_menu_model`, `test_engine_menu_art`, `test_engine_audio`,
`test_engine_match_render`), `port/ENGINE.md`, `docs/PROGRESS.md`.

1. Menu model: 26 native descriptors/max table, `fifa96_settings` block,
   `fifa96_menu_model_reset` preserving settings, 8-row options window,
   live rows (Half Length, Fouls, Injuries, Offside, SFX Volume, Sound
   Effects), controls page, back navigation.
2. Art/front-end: draw the options rows/values with LANG/HELV12 (native
   geometry ÷2), the controls text block; footer.
3. Bridge: apply `fifa96_settings_handoff` + `set_config`/`set_period` inside
   the `team_selected` block; audio gain from SFX volume × Sound Effects.
4. Tests: descriptors/max/defaults, half mapping, settings survive reset,
   bridge applies config/period and field cells, audio gain 100/50/0.
5. Docs: the P6 options register + legs (`options.dat`, music/speech,
   skill/clock, gamepad kick/pass).

**Accept:** Options changes the half length and the live gameplay settings and
is audible/visible; `make check` green; M1/M2 exact.

---

## Task 4 — integration, verification, docs

- Merge the goal branch, run `make check`, M1/M2 cmp, census.
- Windowed play-through: team select (Brazil/Italy), kits/formation, a scored
  goal, restart, half-time, full time, options half length; screenshots.
- Update README/PROGRESS/ENGINE; keep only `main`; push.

## Global constraints

- M1 `09b726b7…` immutable; M2 v29 `7c3874f7…` exact (new behavior gated by
  `live_start`/`team_selected`; the side-1 fix is side-0-neutral). Any move
  needs the usual measured re-pin.
- Census unchanged; `make check` green; tests for each task; evidence-cited
  claims, labeled stand-ins, append-only errata.
- No fake score/producer; the live armer is a labeled stand-in for the
  unported camera/goal producer.
