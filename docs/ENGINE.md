# Native engine (`fifa96_engine`)

The native engine layer sits on top of the 56 clean-room `fifa96_*` libraries and
turns them into a running game: platform ABI → SDL3/null backends → engine core
(boot, asset table, clock, intro, front-end, match).

Status: **M1 complete headless; M2 match lifecycle complete headless with the
render chain complete at the derived level — palette install, HUD/overlays and
the resource-driven kickoff formation remain open (`OL-T11-6..8`), live anim
inputs (`OL-80`) are wired (13/80 action rows
wired), and the derived FU-143 phase driver is now wired into the run loop (the
live period end 2 → 0x0C is derived) — the M2-B acceptance tape green and
byte-identical, and the interactive `make game` smoke reaching match start and
control input on this host.** Kick → score remain blocked interactively on the
unported rows (see "Interactive smoke" and "Known gaps"); the derived kickoff
entry into phase 2 is still `OL-79`/`OL-84`. See
`docs/superpowers/specs/2026-10-06-fifa96-native-engine-port-design.md` (parent),
`docs/superpowers/specs/2026-10-07-fifa96-m2-match-design.md` (child),
`docs/superpowers/plans/2026-10-07-fifa96-m2-arms-and-wiring.md` (split
follow-up) and the SDD workspaces under `.superpowers/sdd/` for the full record
(scratch; may be deleted).

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

`make check` configures, builds `-Wall -Wextra -Werror`, and runs all 104 CTest
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
  chain and the derived kickoff ball placement at begin), FU-142a installer
  arms in phases 0x13/0x14, the FU-143 phase driver
  (`fifa96_match_run_phase_drive`: the derived class gate + `FUN_0008B9CC`
  period-end chooser, so a live class-1 period end writes phase 0x0C on the
  selector-0 default), deterministic FU-85/88/89 render chain, period end
  (`resolve` OVER→POST→EXIT) back to the front-end.
- Action dispatch (FU-137): **13/80 rows wired** — `00`, `04`, `06`, `07`,
  `08`, `0F`, `18`, `1E`, `21`, `23` (playability G1 + arms-and-wiring G3) and
  `26`, `28`, `2A` (cluster G); dispatch results 66 UNSUP / 13 OK / 1 NOTF.
- **Interactive smoke (Task 13, verified on this host 2026-10-08):** `make game`
  window opens (960×720 integer-scaled SDL3; ESC quits, exit 0). The
  walkthrough reaches **match start** — intro RETURN skip → front-end BACKSPACE
  (DECLINE/panel) → RETURN (panel confirm → FU-66 STATE16 bridge) — and the
  match canvas replaces the front-end (screenshots; the canvas is the match
  clear colour: kickoff formation is OL-T11-8 and HUD/overlays OL-T11-7).
  Arrow/Z/C presses reach the run: an in-process gdb probe read
  `input_state[0]` = 0x04 (RIGHT), 0x01 (UP), 0x10 (KICK), 0x20 (PASS), while
  `dispatched_ok` stayed `0x1` (row `00` only). **Kick and score are not
  interactively reachable:** the zeroed FU-141 pool dispatches row `00`
  only (kick rows `07`/`0F` need the unported possession/selection legs) and
  the derived FU-72 score writer has no wired gameplay invoker yet (the
  goal-screen handler cluster and the clock's goal scanner stay unported,
  OL-87/OL-88). The class-1 phase-2 period
  end is now derived (the FU-143 phase driver is wired), but the interactive
  match cannot reach phase 2 without the unported kickoff entry (`OL-79`/
  `OL-84`). The M2-B tape covers the sequence headlessly by declaring/forcing
  its kickoff/mechanics phases and calling the derived
  `fifa96_match_run_score_event` for the goal step (C3-OL2; the carried
  tracked-side default keeps it byte-identical to the old increment).
- `test_engine_m1` pins the 691-frame M1 transcript
  (`tests/golden/engine/m1-frames.txt`).
- `test_engine_m2` replays spec §5 (boot → skip intro → front-end → start match
  → kickoff → move → kick → score → period end → exit to front-end) and pins
  the 165-frame M2-B transcript (`tests/golden/engine/m2-frames.txt`):
  `frame=<n> hash=<hex>` plus `state=<phase>/<home>-<away>` while a match is
  live, the forced kickoff phases 0x13/0x14, the wired-row `FIFA96_OK` dispatch
  set, and the score step. The FU-143 phase driver is wired underneath and the
  transcript is **byte-identical** (the `state=` sample precedes each step, so
  the derived 2 → 0x0C write inside the exit step is not a transcript line);
  the golden is not re-pinned. Regenerate with
  `./build/test_engine_m2 > tests/golden/engine/m2-frames.txt` (the test exits
  non-zero while rewriting the file; re-run `make check` to verify).

## Known gaps

- **Unwired rows (67/80).** 66 rows dispatch `-FIFA96_ERR_UNSUPPORTED`: 28
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
  stand-in (`-6`; `FUN_00048DC0`/`FUN_000CE980`), HUD/overlays (`-7`;
  marker/name/score passes), kickoff formation/record placement (`-8`),
  direction addend `0xA2A10` (`-9`); live anim inputs are `OL-80` above.
  `OL-T11-8` is now **partial (M2 playability Task 5)**: the native
  setup/restart commit `FUN_00079B6C` (position := target) and the kickoff
  act-1 ball spawn (`[0x158830] = 0x1E0`, z = 0) are ported
  (`fifa96_match_entities_place`/`_kickoff_place`, called at match begin), so
  at least the kickoff ball position is non-zero at match start; the
  per-record formation *targets* come from the resource-loaded `0x14BFC0`
  table (`FUN_0004A6BC` reads `t%s.dat`/`lay%s.fmt`), so real formation
  coordinates stay an open leg (FU-89 §11 erratum). Note the plan's
  "kickoff placement (OL-T11-9)" label is a numbering erratum: the register's
  `-8` is the placement item and `-9` is the direction addend. `OL-T11-10`
  (FU-89 key-seeding consumers) closed with the FU-89 §11 errata.
- **Phase table (FU-143):** the 35 FU-83 phase rows (handlers `0x110794`,
  classes `0x1106AD`) and the transitions `FUN_000740A0` / `FUN_000888FC` /
  `FUN_0008A938` / `FUN_0008B9CC` are derived and ported at the loader level
  (`fifa96_action_phase_row/_act/_situation/_period_end`), with open legs
  `OL-72`…`OL-79`. The phase driver `fifa96_match_run_phase_drive` is **wired
  into the run loop** (M2 playability Task 3): each granted frame it applies
  the class-1/class-2 gate and consumes the FU-62 clock's `sec == limit + aux`
  completion (`mr.clock_period_ended`), running the derived `FUN_0008B9CC`
  chooser — a live class-1 period end writes phase 0x0C (2 → 0x0C) on the
  selector-0/no-extra-time default, and the run-end teardown resets to 0. The
  engine still cannot enter live phase 2 without the unported kickoff path
  (`OL-79`, carried as `OL-84`), the extra-time flag producer (`OL-85`) and the
  post-period 0x0C hold/reset timing (`OL-86`) stay open, so the M2-B tape
  keeps its declared phase forcing (0x13/0x14 kickoff, phase 2 mechanics) and
  stays byte-identical. See FU-143 §9 (integration errata).
- **Score event source (child `C3-OL2`, closed; invokers open).** The FU-72
  `FUN_00093944` writer is derived and ported
  (`fifa96_action_score_event`, FU-142 Appendix L): score increment, last-side
  and tracked-side goal-difference bookkeeping, and the
  `FUN_0009252C` threshold posts (0x9A..0xA0, 0xD3). It is wired as the live
  run's source (`fifa96_match_run_score_event`, with the writer's four state
  cells and `score_last_event`), and the M2 tape's goal step uses it
  (transcript byte-identical). The native invokers are unported, so gameplay
  goals cannot reach it: **OL-87** (the six period-indexed goal-screen handlers
  `0x110F78` + the `[0x15B6D4]` scheduler `FUN_000948AC` + the tracked-side
  producer `FUN_00092D8C`), **OL-88** (goal detection: `FUN_0008AF38
  0x8B623..0x8B63E` -> `FUN_00088940` -> `FUN_0008A938(6, side)`), **OL-89**
  (posted-id dispatch `FUN_0009252C` and the `FUN_000CBC4C` probe). The engine
  carries tracked side -1 (`add_goal`-equivalent) until OL-87 lands;
  `fifa96_match_run_add_goal` remains for those unported paths.
- Retail front-end art asset (no OPTIONS-like path exists in the ISO).
- Per-row cluster legs carried in FU-139/FU-141/FU-142 (`OL-56`…`OL-71`:
  unmodeled record bytes, process globals, camera/track inputs, roster
  descriptors, animation selectors).
