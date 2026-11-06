# P6 — match completeness: real team identity, live goals, match options (2026-11-05)

**Status:** proposed. The playable SDL wave (P1–P5) stands; this closes the
playability gaps the final review left as legs.

## 1. Goal

A player can choose two real teams and see their kits/formation/codes, score a
goal through the native goal path and watch the score/restart/half-time,
finish the match, and set the half length and the live gameplay options from a
real Options screen.

## 2. Evidence (fresh recon, `/tmp/opencode/play/P6-*.md`)

1. **PCCD/PCDB is mapped** (P6-team-identity): `/FEDATA/PCCD.POG` RefPack →
   `PCDB`; team block `0x4B4` (0x49-byte records, 242 offsets at `0x4BC`),
   players `0x8A06` (0x24 bytes, 3615), string pool `0x2866A`; mode 4 (World)
   at `0x1BC..0x2B3` (62 slots). Team record `+0x12` formation, `+0x25..0x30`
   four kit colour triples, `+0x31..0x34` print colours, `+0x45` player count,
   `+0x49..0x88` roster ids. The front-end selection cells `0x105470/74` are
   `(mode,index)` pairs, not team ids; `FUN_00011620` resolves
   `mode/index → record id → record offset`. Kit indices resolve through
   `PALteam.fsh` frame `2*(idx/16)`, ramp `idx%16`. The port's formation
   handoff is cosmetic: `begin` zeroes `formation[]` before the bridge's
   post-begin setter, so the match plays 0.
2. **Live goals need a producer** (P6-goal-path): the native armer is a
   camera-pan scan (`|camZ| > 0xB20` or `|camX| > 0x730`); the live follow
   pins `cam = ball, y = 0`, and the ball's phase-2 lead self-centers, so the
   armer never fires. Forcing the ball to the goal mouth fires the native
   chain: score, phase 5, celebration install, kickoff `(480,0,0)`, phase 2.
   Away-side goals freeze in phase 5 because the port tests the record side
   (`fifa96_match_handlers.c:6206`) instead of the native goal-side flag
   `0x157A4C` (`global_157a4c`); the fallback hardcodes side 0. The ordinary
   native match has no goal overlay/replay — score + celebration + restart is
   the correct presentation.
3. **Options are real** (P6-options): the 26-descriptor table at `0x105340`,
   values at `0x149278` (also `options.dat`); half length index `0x0E` maps
   `{2,4,6,8,10,20,45,0}` → minutes, default 2 min = the engine's 120/40.
   Fouls `0x0A`, Injuries `0x0B`, Offside `0x10`, SFX volume `0x11`, Sound
   Effects `0x01` are live-able; the rest are presentation/deep legs.

## 3. Tasks

### Task 1 — real team identity (PCCD)
- New loader `fifa96_pccd.{c,h}`: parse/validate PCDB, team lookup by
  `(mode,index)`, player record (id/name/position/rating) optional; bounds-check
  everything; soft-fail.
- New engine `fifa96_team_data.{c,h}`: read `/FEDATA/PCCD.POG` via the asset
  table, own the decoded arena, the 44 LANG-name → mode-4-index table, and
  `lookup(lang_id, identity)` (pccd id, record, code `+8`, formation, kit
  triples, print colours, roster ids).
- Front-end handoff: `pccd_index[2]`, `pccd_record[2]`, `pccd_id[2]`,
  `team_code[2][10]` (record `+8`, not the LANG code), `formation[2]` from
  `+0x12`, kit triples. The bridge stages a **pre-begin matchup**
  (`fifa96_match_run_set_matchup`) so `begin` seeds the home `.fmt` and
  reseeds team 1 only when the formations differ; the post-begin
  `set_formation` call stays for the id.
- Live kit stage: fill team 0/1 kit bands from the selected records' kit-0
  triples through `PALteam` ramps (Brazil yellow/blue, Italy blue/white).
- Tests: PCDB decode (retail, ISO-gated), the 44 mapping (Brazil/Italy),
  matchup formation, HUD codes, kit pixel difference; M1/M2 exact.
- Legs: roster/attributes/AI, auto away/keeper kits, per-entity translation
  pool, stadium/field settings producers, CTEAM overrides.

### Task 2 — live goal path
- Native correctness: celebration release tests `global_157a4c`, not the
  record side (`fifa96_match_handlers.c:6206`); the fallback passes the goal
  side (`match_run_situation_side(..., side, ...)`, `match_run.c:3805`).
- Live-only producer: when phase 2 and the ball enters the native goal zone,
  arm/snap the goal through the native scan path (label P6-L1 stand-in); set
  the live camera height `0x50` instead of 0 so the camera-pan path can also
  arm. No cheat key.
- Deterministic `test_engine_live_goal`: live begin, stage the ball at
  `(0,0,±0xB30)`, assert armer/score/phase 5/celebration/kickoff/phase 2 for
  both sides, score preserved, no re-score; half-time/full-time score
  survival.
- Windowed capture: goal frame, celebration, kickoff, full time (verification
  harness only).
- Legs: native FU-310 camera place/focus, goal replay/overlay (Practice-only
  natively), goal captions.

### Task 3 — options and controls
- `fifa96_menu_model`: 26 native descriptors/max table, settings block,
  reset-preserving-settings, option rows for Half Length, Fouls, Injuries,
  Offside, SFX Volume, Sound Effects; controls page; native 8-row geometry.
- `fifa96_menu_art`/`fifa96_frontend_run`: draw the Options screen and the
  controls page with LANG/HELV12; navigation/back.
- Bridge: apply `fifa96_settings_handoff` + `set_config`/`set_period` inside
  the `team_selected` block (M2 stays default 120/40); audio gain from SFX
  volume/Sound Effects.
- Tests: row/max/defaults, half length mapping, settings survive reset,
  bridge applies config, audio gain 100/50/0; M1/M2 exact.
- Legs: `options.dat` persistence, music/speech volume, skill/clock wiring,
  gamepad kick/pass.

## 4. Invariants

- M1 `09b726b7…` immutable; M2 v29 `7c3874f7…` exact (all new match behavior
  is `live_start`/`team_selected` gated; the side-1 fix is a no-op on the
  side-0 tape). No re-pin without the usual written measured reason.
- Census unchanged; `make check` green; new tests for every task.
- Every native claim cites the P6 reports/addresses; stand-ins labeled and
  numbered; append-only errata for corrections.

## 5. Out of scope

Full player rosters/attributes/AI, commentary/music, the native camera
director/goal replay, stadium selection, CTEAM overrides, and the six other
Game Select screens — named legs.
