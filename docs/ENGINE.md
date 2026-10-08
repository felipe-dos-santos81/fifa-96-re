# Native engine (`fifa96_engine`)

The native engine layer sits on top of the 56 clean-room `fifa96_*` libraries and
turns them into a running game: platform ABI → SDL3/null backends → engine core
(boot, asset table, clock, intro, front-end, match).

Status: **M1 complete headless; M2 match lifecycle complete headless with the
render chain complete at the derived level — palette install, HUD/overlays,
kickoff placement and live anim inputs (`OL-80`) remain open (13/80 action rows
wired) — the M2-B acceptance tape green, and the interactive `make game` smoke
reaching match start and control input on this host.** Kick → score → period
end remains blocked interactively on the unported rows (see "Interactive smoke"
and "Known gaps"). See
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
  per granted frame, FU-71 camera / FU-90 display / FU-141 entity/ball chain,
  FU-142a installer arms in phases 0x13/0x14, deterministic FU-85/88/89 render
  chain, period end (`resolve` OVER→POST→EXIT) back to the front-end.
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
  `dispatched_ok` stayed `0x1` (row `00` only). **Kick, score and period end
  are not interactively reachable:** the zeroed FU-141 pool dispatches row `00`
  only (kick rows `07`/`0F` need the unported possession/selection legs), the
  FU-72 score writers are unported (`C3-OL2`), and the class-1 phase-2 period
  end needs the loader-level FU-143 phase drivers wired into the run loop. The
  M2-B tape covers that sequence headlessly by declaring/forcing its phases and
  calling `fifa96_match_run_add_goal` (recorded carry).
- `test_engine_m1` pins the 691-frame M1 transcript
  (`tests/golden/engine/m1-frames.txt`).
- `test_engine_m2` replays spec §5 (boot → skip intro → front-end → start match
  → kickoff → move → kick → score → period end → exit to front-end) and pins
  the 165-frame M2-B transcript (`tests/golden/engine/m2-frames.txt`):
  `frame=<n> hash=<hex>` plus `state=<phase>/<home>-<away>` while a match is
  live, the forced kickoff phases 0x13/0x14, the wired-row `FIFA96_OK` dispatch
  set, and the score step. Regenerate with
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
- **OL-80 render anim inputs (links FU-141 OL-42):** `FUN_00036C70` stages each
  slot's `anim_id` (`byte[[rec+0x28]]`, 0x36D44) and `frame` (`byte[rec+0x3D]`,
  0x36D4F); the FU-141 pool models neither field, so staged slots keep the
  caller-owned row 0 / frame 0 and only the FU-84 row+8 bank derivation and
  the accumulator advance are live. The row-08 staging record now carries a
  zero `frame` field for the native +0x3D scan gate (FU-142 K.6), still
  producer-less. The anim inputs land when the OL-42 installer animation arm
  and the +0x28/+0x3D pool fields are ported.
- **Render legs `OL-T11-1`…`OL-T11-9` (Task 11 close-out register).** Task 11
  left the render chain's remaining legs under these IDs: FU-84 frame tables
  not staged (`-1`; advance uses identity durations, `sprite = frame`), row
  successor/terminal/height machine (`-2`), record facing `+0x7D` (`-3`),
  camera-type ratio setup `FUN_0004D7E8` (`-4`; static `0x1500` default),
  sentinel key scratch producer (`-5`), palette install/kit remap identity
  stand-in (`-6`; `FUN_00048DC0`/`FUN_000CE980`), HUD/overlays (`-7`;
  marker/name/score passes), kickoff formation/record placement (`-8`; zeroed
  FU-141 pool), direction addend `0xA2A10` (`-9`); live anim inputs are `OL-80`
  above. `OL-T11-10` (FU-89 key-seeding consumers) closed with the FU-89 §11
  errata.
- **Phase table (FU-143):** the 35 FU-83 phase rows (handlers `0x110794`,
  classes `0x1106AD`) and the transitions `FUN_000740A0` / `FUN_000888FC` /
  `FUN_0008A938` / `FUN_0008B9CC` are derived and ported at the loader level
  (`fifa96_action_phase_row/_act/_situation/_period_end`), with open legs
  `OL-72`…`OL-79`. The phase drivers are **not yet wired into the engine run
  loop**: only class-1 phases end periods, phase `2` is the only class-1 live
  phase, and the selector-0/phase-0 default starts class 2 — so the interactive
  match never reaches a period end and the M2-B tape declares/forces its phases
  (0x13/0x14 kickoff, phase 2 mechanics) as the recorded carry.
- **Score event source (child `C3-OL2`):** the FU-72 `FUN_00093944` writers live
  in the unported action/phase clusters; `fifa96_match_run_add_goal` exposes the
  derived increment only, and the tape drives it directly (recorded in FU-142
  Appendix I).
- Retail front-end art asset (no OPTIONS-like path exists in the ISO).
- Per-row cluster legs carried in FU-139/FU-141/FU-142 (`OL-56`…`OL-71`:
  unmodeled record bytes, process globals, camera/track inputs, roster
  descriptors, animation selectors).
