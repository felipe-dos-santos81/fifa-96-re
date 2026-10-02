# FU-13: headless scripted input and menu-gated entry reachability

Follow-up to `FU12_runtime_caller_map.md`, using the scripted-input facility
built in Tasks 1–2 (`tools/fifa96_keys.py`, `KEYS_FILE` in
`run-fifa96-capture.sh`) with the proven DPMI probe runner
(`tools/trace_probe.sh` + `tools/fifa96_probe.py`). Goal: reach the two
input-gated targets that FU-12 proved unreachable unattended — site 7
(`FUN_00014c18`, returns `{0x25D69, 0x27B47}`) and site 8 (`FUN_00023b38`,
twelve static callers).

Result in one line: **site 8 reached (29 frames, three matched callers); site 7
still zero after six attempts** (honest zero, no fabrication).

## Facility: `KEYS_FILE` and the keys file format

`run-fifa96-capture.sh` accepts `KEYS_FILE` (unset/empty: previous behavior).
The file is validated with `tools/fifa96_keys.py --check`; its generated
`AUTOTYPE` lines are inserted into the generated DOSBox-X `[autoexec]` just
before `FIFA96.EXE`. `tools/trace_probe.sh` inherits the environment, so:

```sh
KEYS_FILE=tools/keys/skip-intro.keys sh tools/trace_probe.sh <TARGET> <SITE> <SESSION>
```

Format (`WAIT KEYS...`, one step per line):

* `WAIT` — decimal seconds since the previous step (int/float, ≥ 0). The
  helper emits **cumulative absolute** waits: step *n* becomes
  `AUTOTYPE -w <sum of waits 1..n> -p 0.1 <keys>`.
* `KEYS...` — verbatim AUTOTYPE tokens (whitespace/comma separated).
* Blank lines and `#` comments are ignored; malformed lines abort the run with
  `error: line N: <reason>`.

## AUTOTYPE mechanics discovered (hidden complexity)

The design assumed independent scheduled typists. The DOSBox-X 2024.03.01
implementation (`Typer` in `src/gui/sdl_mapper.cpp`) serializes them:

* Each `AUTOTYPE` command spawns a host typing thread and returns. A following
  `AUTOTYPE` command first **joins the previous thread** before returning.
  Therefore, in `[autoexec]`, all but the **last** `AUTOTYPE` command finish
  typing while the DOS shell is still live; `FIFA96.EXE` launches immediately
  before the last command's keys are typed. Only the last line's keys reliably
  reach the game.
* With many lines the autoexec itself is delayed by the serialized typists.
  A 9–10-line keys file stalled the launch beyond the 120 s window: those
  traces contain only the per-AUTOTYPE-program patch passes and an `END`, with
  **zero FILE frames** (the game never started). A 4-line file could still
  launch, but only its final key reached the game.
* Inside the last line, keys are typed at `-p` pace (default 0.1 s) and held
  ~50 ms; a `,` token pauses 2×pace (0.2 s). The shell's command tokenizer
  keeps commas as separate tokens, so runs of commas encode the schedule.
* `-w` is bounded to 0–30 s by the AUTOTYPE implementation (longer waits are
  clamped).

Consequence for future runs: **use one step** whose `KEYS` tail is a single
comma-paced sequence; do not spread a schedule across multiple lines.

## Discovered sequence

`tools/keys/skip-intro.keys` (committed) is one step. Its generated command:

```
AUTOTYPE -w 10 -p 0.1 enter ,,,... space ,,,... space ,,,... space ,,,... enter ,,,... space ,,,... enter
```

(`,`×20 = 4 s at the default pace.) Timeline: `enter` at ~10 s stops the
looping intro video; the title screen loads by ~14 s; the spaced
`space/space/space/enter/space/enter` walk the front end into the `MOD5`
module. Forward-progress oracle: the trace FILE-open mix changes from the
FU-12 baseline (`D:\VIDEO\VID_`-dominated intro repeat) to
`TITL, VS.P, SLIC, HELV, MOUS, MOD2, MOD5`, and traces shrink to 77–86 KB
instead of ~315 KB of repeated `VID_` reads.

## Site 7 — `FUN_00014c18` (zero, cap exhausted)

| # | session dir | keys form | trace bytes | frames | verdict |
|---|-------------|-----------|------------:|-------:|---------|
| 1 | `captures/session-probe-14c18-keys` | 4 lines 8/12/16/20 (`enter`,`enter`,`space`,`esc`) | 94,612 | 0 | reached title; only the last line's key reached the game |
| 2 | `captures/session-probe-14c18-keys2` | 10 lines (10–55 s) | 286 | 0 | autoexec stalled; game never launched |
| 3 | `captures/session-probe-14c18-keys3` | 9 lines (8–52 s) | 326 | 0 | autoexec stalled; game never launched |
| 4 | `captures/session-probe-14c18-keys4` | 1 line, space-path | 76,868 | 0 | reached `MOD5`; no site-7 call |
| 5 | `captures/session-probe-14c18-keys5` | 1 line, down-path | 209,396 | 0 | key race: still in intro at timeout |
| 6 | `captures/session-probe-14c18-keys6` | 1 line, `MOD5` + arrow navigation | 86,012 | 0 | reached `MOD5`; no site-7 call |

All six attempts show **zero** `T_PROBE site=7` frames. Attempts 4 and 6 were
valid runs (game live at `MOD5` when the rig timed out; the single `T_END` in
those streams is the AUTOTYPE helper's exit, one helper per line). Attempt 5
was a timing race (game never left the intro) and is not evidence about the
callers. Neither static caller (`0x25D64`, `0x27B42`) was exercised: the
menu/state handlers that call them were not reached by intro-skip plus
space/arrow navigation. This is an honest reachability zero, not a crash.

## Site 8 — `FUN_00023b38` (reached, 1 attempt)

| # | session dir | keys form | trace bytes | frames | verdict |
|---|-------------|-----------|------------:|-------:|---------|
| 1 | `captures/session-probe-23b38-keys` | 1 line, space-path (same file as site-7 #4) | 77,390 | 29 | **hit** |

Verbatim decoder lines (three distinct callers; `caller_link` is the runtime
RETURN address):

```
T_PROBE site=8 caller=0x00220227 delta=0x1fc000 caller_link=0x24227 target_link=0x23b3d
T_PROBE site=8 caller=0x00220282 delta=0x1fc000 caller_link=0x24282 target_link=0x23b3d
T_PROBE site=8 caller=0x002206da delta=0x1fc000 caller_link=0x246da target_link=0x23b3d
probe_frames=29
expect_site=0x8 hit=True
```

Counts: `0x24227` ×12, `0x24282` ×12, `0x246DA` ×5. Cross-check against the
static census (`python3 tools/fifa96_callers.py 0x23B38`): `0x24227 = 0x24222+5`,
`0x24282 = 0x2427D+5`, `0x246DA = 0x246D5+5` — all three in the twelve-caller
census, no unmatched caller. Measured delta `0x1FC000`, consistent with FU-11
and FU-12; no delta is hardcoded. The `MOD5` front-end loop drove the
table-frame loader repeatedly while the screen was live. The other nine
static callers remain untested at runtime.

## Interpretation

* Scripted keys are effective: the game consumes AUTOTYPE keystrokes, the
  intro is skippable (~10 s), and the front end can be walked to `MOD5` where
  it keeps drawing frames.
* Site 8 is confirmed as part of the `MOD5` front-end draw path through three
  call sites. Site 7's still-loader callers sit in state handlers (`EBX`∈{10,
  11}; `ESI==0x0A`) that were not reached; reaching them likely needs a
  different front-end screen or deeper navigation than intro-skip + arrows.
* The multi-line AUTOTYPE serialization is the notable hidden complexity; it
  invalidated the original cumulative-wait schedule and is documented here so
  later probe campaigns use the single-line comma-paced form.

## Provenance

* Probe commands (captures are git-ignored, derived from copyrighted content):

```
KEYS_FILE=tools/keys/skip-intro.keys TIMEOUT=75 sh tools/trace_probe.sh 0x14C18 7 probe-14c18-keys6
KEYS_FILE=tools/keys/skip-intro.keys TIMEOUT=75 sh tools/trace_probe.sh 0x23B38 8 probe-23b38-keys
```

* Site-8 baseline for comparison (FU-12): `captures/session-probe-23b38/trace.bin`,
  314,753 bytes, zero frames, `VID_`-dominated mix. Site-7 baseline:
  `captures/session-probe-14c18/trace.bin`, 314,318 bytes, zero frames.
* Static census used for matching: `python3 tools/fifa96_callers.py 0x23B38`
  (twelve `ret` addresses); site 7: `0x25D69 = 0x25D64+5`,
  `0x27B47 = 0x27B42+5` from FU-12.
* All probe runs patched a copy of `game/FIFAPCCD96.iso` (read-only) via
  `tools/fifa96_patch.py`; `build/` and `captures/` are not versioned.
