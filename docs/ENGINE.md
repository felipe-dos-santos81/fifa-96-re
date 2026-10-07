# Native engine (`fifa96_engine`)

The native engine layer sits on top of the 56 clean-room `fifa96_*` libraries and
turns them into a running game: platform ABI → SDL3/null backends → engine core
(boot, asset table, clock, intro, front-end, match foundation).

Status: **M1 complete headless; M2 foundation complete; playable match split to a
child plan.** See `docs/superpowers/specs/2026-10-06-fifa96-native-engine-port-design.md`
and `.superpowers/sdd/2026-10-06-fifa96-native-engine-port/progress.md` for the
full record (the SDD workspace is scratch and may be deleted).

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
| `include/fifa96_engine/fifa96_match_run.h` | Match lifecycle, 30 Hz frame body, input/control slot, render chain |
| `include/fifa96_engine/fifa96_keys.h` | Shared engine key codes (1–9) |

## Building and testing

`make check` configures, builds `-Wall -Wextra -Werror`, and runs all 93 CTest
cases — including 13 `test_engine_*` cases built under ASan/UBSan. No external
dependency is required for this: the `null` backend is the deterministic
regression source of truth (it hashes Mode-X planes + palette and PCM, replays
input tapes, and advances a virtual 10 ms clock).

The `fifa96` executable and the SDL3 backend are **not built yet**: SDL3 is not
installed on the development host. `find_package(SDL3 QUIET)` degrades cleanly to
a headless-only build. To build the windowed game, install SDL3 (≥3.x) and
re-run `make build`; the target and `make game` arrive with the deferred Task 2.

## What runs today (headless)

- Boot from the real CD image (`game/FIFAPCCD96.iso`): ISO mount, asset table,
  loader-spine resource scan.
- Intro playback of `VIDEO/*.TGV` (M1 heuristic: prefers `/VIDEO/VID_INTR.TGV`;
  the true boot-intro choice awaits the underived load-order table). Input skips
  the intro.
- Front-end state machine with key mapping, menu rendering (procedural fallback —
  the retail front-end art asset is unresolved; FU135 erratum) and quit.
- Match foundation: lifecycle begin/step/end, 30 Hz frame body and period clock,
  input → `fifa96_control_slot` per granted frame, deterministic camera/scene/
  sprite render chain (hash-pinned).
- `test_engine_m1` pins a 691-frame deterministic transcript
  (`tests/golden/engine/m1-frames.txt`).

## Known gaps

- SDL3 backend + `make game` + human-visible M1 acceptance (deferred Task 2;
  must also baseline the engine clock at boot).
- Retail front-end art asset (no OPTIONS-like path exists in the ISO).
- Playable match: the entity/ball/action-handler port (~13 tasks per
  `docs/ghidra/FU136_action_handler_port_scope.md`) is the child plan; it will
  also wire the front-end→match bridge, period length, and OVER→POST→EXIT.
