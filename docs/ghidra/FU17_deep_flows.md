# FU-17: deep-flows campaign (mode classifier + site 7)

Follow-up to `FU16_mode_map.md`. FU-16 left the classifier at `0x26B45`
unreachable from the front end, a friendly match and the league calendar, and
site 7 (`FUN_00014c18`) un-reached. FU-17's brief: visit the remaining
unexplored flows (TOURNAMENT entry and sub-screens, PLAYOFF, PRACTISE, cup
flows, standings/statistics/results, fixture Play from the calendar) while
watching two signals: `T_PROBE site=9 mode=<N>` at `0x26B45`
(`CAPTURE_EAX=1`, modes 8–11) and `T_PROBE site=7` at `0x14C18`.

Result in one line: **modes 8–11 were never observed (0 of 6 classifier runs)
and site 7 was not reached**; no site-7 confirmation run was executed because
the brief's gate (a mode 8–11 sighting) never fired. The campaign did add
three previously unseen screens (TOURNAMENT TEAM SELECT, TOURNAMENT SCHEDULE,
TOURNAMENT STANDINGS) plus the FRIENDLY pre-match START GAME menu and TEAM
STRATEGY screen, mapped the tournament schedule's keyboard focus, and located
the state-8/9/10/11 setters statically.

## Method

* Classifier runs: `CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=<f> sh
  tools/trace_probe.sh 0x26B45 9 probe-flow-<N>`; decode with
  `python3 tools/fifa96_probe.py captures/session-probe-flow-<N>/trace.bin
  --target-link 0x26B45 --overwrite 5 --expect-site 9 --capture-eax`.
  `0x26B45` overwrite length is 5 (`8d 40 00 51 52`).
* Video as in FU-15/FU-16: DOSBox-X `DX-CAPTURE /V` writes one AVI per video
  mode change, `python3 tools/fifa96_frames.py captures/session-<S> --fps 1`
  extracts PNGs, and frames were read directly. Segment 004 starts at ≈10.3 s
  (000+001+002+003 durations), so frame `004-N` ≈ 10.3 + (N−1) s.
* Keys are single-step comma-paced AUTOTYPE (`tools/keys/fu15-verified-nav.keys`
  style; `,` = 0.2 s). Run timelines were generated with
  `/tmp/opencode/fu17/gen.py` (copy of the FU-16 generator) and validated with
  `python3 tools/fifa96_keys.py --check`. `TIMEOUT` was raised above the
  120 s default for the long runs (411–419 s captures).
* The DOSBox autoexec buffer is the hard budget: a keys line over
  `AUTOEXEC_SIZE` (≈4096 bytes total) aborts at startup
  (`SYSTEM:Autoexec.bat file overflow`). Comma padding costs ≈10 bytes/s, so
  post-match input for the 418 s friendly cannot fit in one AUTOTYPE line.
* Static checking used the open Ghidra project (`fifa96_le.bin`, addresses are
  the LE-image offsets used by the probe); `fifa96.exe` was not modified.

## Per-run table

All runs target `0x26B45`/site 9. "probe" = `T_PROBE` frames (`site=9`); all
were zero. Trace bytes from `captures/session-<session>/trace.bin`; AVIs/PNG
frames from the capture session. New screens are marked **NEW**.

| # | kind | keys (key@time, s) | session | trace bytes | AVIs / frames | probe | modes | screens observed (frame evidence) |
|---|------|--------------------|---------|------------:|--------------:|------:|-------|----------------------------------|
| 1 | classifier | `enter@10 down@18 down@21 enter@25 enter@34 enter@44 down@50 enter@60 enter@64 down@70 right@78 right@82 right@86 enter@90 enter@102 enter@112 enter@118` (`/tmp/opencode/fu17/tournament1.keys`) | `probe-flow-1` | 79,795 | 5 / 119 | 0 | none | **NEW** TOURNAMENT grid (004-0025) → **NEW** TEAM SELECT (004-0035, 004-0046) → **NEW** TOURNAMENT SCHEDULE (004-0093) → **NEW** TOURNAMENT STANDINGS (004-0100) → back to schedule (004-0109) |
| 2 | classifier | `enter@10 down@18 down@21 enter@25 enter@34 enter@44 down@50 enter@60 enter@64 down@70 right@78 right@82 right@86 enter@90 up@96 left@100 left@104 down@108 down@112 down@116 down@120 down@124 enter@130 enter@142 enter@148` (`tournament2.keys`) | `probe-flow-2` | 80,373 | 5 / 150 | 0 | none | TEAM SELECT → SCHEDULE (004-0084…); `left@100` moves bottom-bar focus from Standings to the Load icon (pixel-diff bbox x 352–539, y 438–476); `enter@130` opens **LOAD GAME** slot dialog over the schedule (004-0125, 10 empty slots); `up@96` and the six `down`s are inert |
| 3 | classifier | `enter@10 enter@18 down@23 down@28 down@33 down@38 right@43 right@46 enter@49 enter@60 enter@75 enter@90 enter@105` (FU-16 `m2.keys`, copied to `friendly3.keys`) | `probe-flow-3` | 127,100 | 6 / 419 | 0 | none | FRIENDLY → match intro → pre-match options panel → Controller → kickoff (005-0002) → half-time scoreboard 0–0 at 02:05 (005-0190) → second half (005-0206) → **full-time menu "Play Overtime / Shootout / End In Tie"** (005-0365, ≈418 s wall); no option selected before the 420 s kill |
| 4 | classifier | `friendly4.keys` (FU-16 `m2` + `down@430… enter@472`) | — | — | — | — | — | **aborted at DOSBox start**: keys line 4.7 kB exceeded the autoexec buffer (`Autoexec.bat file overflow`); no session, no trace |
| 5 | classifier | `enter@10 enter@18 down@23 down@28 down@33 down@38 right@43 right@46 enter@49 down@57 down@61 left@65 left@67 left@69 left@71 left@73 left@75 left@77 left@79 enter@84 enter@92 down@320 down@324 down@328 down@332 down@336 enter@342 enter@352 enter@362 enter@372` (`friendly5.keys`) | `probe-flow-5` | 87,891 | 6 / 411 | 0 | none | pre-match options panel: `down@57` → Skill Level (005-0006), `down@61` → Half Length [2] (005-0026); eight `left`s do **not** change the value; `enter@84` → Controller (005-0035); `enter@92` → **START GAME menu** (005-0100: Start Game / Camera Views / Controller / Team Coverage / Team Formation / Team Strategy); no third enter, so no kickoff; `down@320–336` walks the highlight to Team Strategy and `enter@342` opens **TEAM STRATEGY** ("All out Offence", 005-0295) |
| 6 | classifier | `enter@10 down@18 enter@22 enter@30 enter@40 enter@46 down@56 enter@60 enter@66 down@70 right@74 right@78 right@82 enter@86 down@94 down@99 right@104 right@109 enter@116 enter@128 enter@140 enter@152 enter@164 enter@176 enter@188 enter@200` (`calendar6.keys`) | `probe-flow-6` | 79,795 | 5 / 112 | 0 | none | league route (FU-16 run B) → LEAGUE CALENDAR (004-0085: Wednesday 17 August, Zambia–Netherlands, random draw); `down×2` focuses **Simulate** (004-0101, yellow border); the game exited at ≈110 s, before `enter@116` |

Decoder output is identical for every decodable run:

```
probe_frames=0
expect_site=0x9 hit=False
```

Every trace ends with the normal TSR summary (`SUMMARY patch: ok=5 skip=5`,
`SUMMARY end=END lost=0 seqgaps=0`). The failed run 4 produced no trace at all.

## Mode inventory across FU-16 / FU-17

**No mode value has ever been observed.** FU-16 contributed five classifier
runs (zero `T_PROBE site=9` frames) and FU-17 attempted six more (five
decodable traces, zero frames): eleven attempts in total, ten decodable
traces, all empty.
The table below is the combined screen inventory with an explicit empty mode
column. The entries marked NEW are this campaign's additions.

| screen | how reached | mode observed | frame evidence |
|--------|-------------|---------------|----------------|
| GAME SELECT / LEAGUE / OPTIONS / FRIENDLY / PRACTISE grids | front end | — | FU-16 mode-1…5 |
| LEAGUE SELECT, LEAGUE TEAM SELECT, confirm dialog | `down`→Leagues,
  `enter`, `enter` | — | FU-16 mode-4 |
| LEAGUE CALENDAR (Aug/Sep/Oct) | league team select → Play | — | FU-16 run B; FU-17 flow-6 |
| FRIENDLY match intro, pre-match options, Controller, kickoff, half-time, second half, full-time menu | FRIENDLY → Play → enters | — | FU-16 mode-2; FU-17 flow-3 |
| IN-MATCH gameplay | kickoff | — | flow-3 005-0002… |
| **NEW** TOURNAMENT grid → TEAM SELECT | GAME SELECT `down`×2 → `enter` ×2 | — | flow-1 004-0025, 004-0035 |
| **NEW** TOURNAMENT SCHEDULE (Game 1–5, fixtures, Next Game, Simulate; bottom Load/Standings/Play) | TEAM SELECT → Play | — | flow-1 004-0093 |
| **NEW** TOURNAMENT STANDINGS (Group A–D, P W D L C A Pts; All Games/Home/Away/Consecutive tabs) | schedule → Standings | — | flow-1 004-0100 |
| **NEW** START GAME menu (Start Game / Camera Views / Controller / Team Coverage / Team Formation / Team Strategy) | friendly pre-match → Controller ok | — | flow-5 005-0100 |
| **NEW** TEAM STRATEGY ("All out Offence" tactical board) | Start Game menu → Team Strategy | — | flow-5 005-0295 |
| LOAD GAME over schedule | schedule → bottom Load icon | — | flow-2 004-0125 |

## Static notes (state setters and the classifier)

Two checks this campaign changed the static picture from FU-16:

* `0x26B45` itself is not a function boundary in `fifa96_le.bin`; it
  tail-calls `FUN_00026aa5`, which draws with `[0x5524] | 0x80` through
  `FUN_00012fe4`/`FUN_00012490` using VIV tables `[0x47e58]`/`[0x47e60]` and
  the index table `[0x498cc]`. The classifier's `[0x5524]`/`[0x5528]` flags
  are consumed by this draw path, i.e. it is a competition-screen renderer.
* The state dispatcher `FUN_0001442c` jumps through a table at `0x43dc`
  indexed by EAX (states 0…0x13). That table is **all zeros in the static LE
  image** (LE fixups populate it at runtime), so state→handler mapping cannot
  be read statically; this is the same limitation FU-16 hit for `0x26B45`.
* Setters found for the gated states (all 16 call sites of `0x1442c` were
  inspected):
  * `0x2cfc0`: `CMP [0x49fbc], 1` → `EAX=8`, else `EAX=9` → state 8/9.
  * `0x2d083`: `ESI==2` → 7, `ESI==0` → 8, else → 9.
  * `0x2d96d`: after `CALL 0x14c90` (VIV loader) → `EAX=0xa` → state 10.
  * `0x2da16`: after `CALL 0x14bb0` (VIV loader) → `EAX=0xb` → state 11.
* `0x2d8f7–0x2d977` is the state-10 handler: it calls the competition helpers
  `0x2d1bc`/`0x2d2f8`, loads a VIV still with `0x14c90`, sets state 10, then
  polls input (`0x16350`) in a loop until a key condition, sets `[0x5094]` and
  falls through to the state-11 setup (`0x14bb0`, state 11). This and the two
  site-7 callers below are all in the competition module (`0x25xxx–0x2Dxxx`,
  alongside `FUN_0002654c`/`FUN_00026980`, which shift competition tables at
  `0x49dbb`/`0x49dfb`).
* Site-7 gate re-confirmed: `0x25D4F–0x25D64` calls `0x14c18` only when the
  state parameter `EBX` is 10 or 11 (loads VIV `[0x47c30]`/`[0x47dc0]`, then
  sets state 0x10); `0x27B1A–0x27B42` calls it only when `ESI==10` (after
  `FUN_0002654c`/`FUN_00026980`, then state 0x12 when `[0x5094]==0`).

## Site-7 verdict — `FUN_00014c18`: not reached, no confirmation run

The brief runs site 7 only after a mode 8–11 sighting. None of the six
classifier runs produced any `T_PROBE` frame, so the gate never opened and the
two allotted site-7 runs were deliberately not used. No `caller_link` is
claimed; the only positive site-7 evidence remains the historical
`session-probe-23b38-keys` trace (29 site-8 frames) and the FU-13 caller set,
neither of which involves `0x25D69`/`0x27B47`.

The static notes above explain the zero: site 7 lives on a competition screen
whose state (10/11) is entered from the competition match engine
(`0x2Cxxx–0x2Dxxx`), and no keyboard-reachable flow this campaign found starts
a competition match. FRIENDLY is a different engine — its kickoff, half-time
and full-time produced no `T_PROBE site=9` frames across FU-16's and FU-17's
complete friendly runs, which is consistent with the classifier being used by
league/tournament cup screens only.

## New navigation model (tested once unless noted)

* **TOURNAMENT deep route** (verified end to end, flow-1): GAME SELECT
  `down`×2 → `enter` (purple grid, slow fade-in) → `enter` (International) →
  TEAM SELECT → check a country → `right`×3 to Play → `enter` opens the
  schedule; on the schedule `enter` opens STANDINGS, `enter` returns. The
  full sequence is committed as `tools/keys/fu17-deep-flows.keys`.
* **TOURNAMENT SCHEDULE keyboard focus** (flow-1/flow-2): focus starts on the
  bottom bar; `left` moves Standings → Load icon (only one step), `enter` on
  the Load icon opens LOAD GAME. `up` does nothing, the left-column
  Game 1–5/Simulate buttons and the per-fixture Play cells were not reached by
  keyboard; bottom Play stayed greyed.
* **League calendar** (flow-6): `down`×2 from the date grid moves focus to the
  **Simulate** button (frame 004-0101); the run ended before it could be
  pressed. The calendar draw is random per run (FU-16: Portugal–Nigeria,
  Monday 15 August; FU-17: Zambia–Netherlands, Wednesday 17 August).
* **FRIENDLY pre-match flow** (flow-5, new detail): the pre-match options
  panel focuses Game Type by default; `down` highlights Skill Level then Half
  Length, but `left`/`right` did not change the Half Length value in our run
  (the value stayed 2). `enter` confirms the panel → Controller → START GAME
  menu (Start Game is the default focus; `down` walks Camera Views,
  Controller, Team Coverage, Team Formation, Team Strategy). A third `enter`
  (never sent) would start the match; `enter` on Team Strategy opens the
  tactical board.
* **FRIENDLY match timing** (flow-3): kickoff at ≈94 s wall, half-time
  scoreboard at ≈243 s, full time at ≈418 s (Half Length 2, Clock
  Continuous), then the "Play Overtime / Shootout / End In Tie" menu. This
  makes the friendly result screens unreachable with a single AUTOTYPE line
  under the ≈4 kB autoexec budget (≈10 bytes/s of comma padding).

## Aborts and crashes (honest log)

* Run 4: no game data. The generated AUTOTYPE line (4.7 kB) exceeded DOSBox-X's
  `AUTOEXEC_SIZE`, so DOSBox aborted at startup with `Autoexec.bat file
  overflow`; no capture session was created. The retry (run 5) fit the budget
  (`TIMEOUT=410`).
* Run 6: the game exited at ≈110 s wall (video ends at frame 004-0101, no new
  video segment, trace ends with the normal `T_END` summary) while the focus
  was on Simulate, before `enter@116`. The exit is consistent with the DOS/4GW
  exception pattern seen in FU-16 run 3, but no crash text was captured, so
  this is reported as an unexplained early exit, not a proven crash.
* Run 4's failure and run 6's exit are the only anomalous terminations;
  runs 1, 2, 3 and 5 drew video until the rig killed DOSBox-X at timeout.

## Honest gaps

* **No modes, no site 7.** The classifier `0x26B45` never fired in ten
  decodable traces across two campaigns (eleven attempts; run 4 aborted before
  the game started); capture-eax has a positive control only from FU-16's
  post-campaign run at another target.
* **PLAYOFF and PRACTISE were not visited** — the six-classifier-run cap was
  reached, and the campaign spent its runs on tournament, the league calendar
  and the friendly match. Cup flows beyond the schedule/standings (group
  standings update, knockout bracket, per-fixture Play) were not reached.
* **League Statistics/Results and the tournament Standings sub-tabs**
  (All Games/Home Games/Away Games/Consecutive) were not opened.
* **Competition match start is mouse-gated in our runs**: tournament schedule
  Play cells/Simulate were not keyboard-focusable, and the league calendar
  Simulate press was cut off by the run-6 exit. The fixture Play from the
  calendar therefore remains unverified.
* MODEM SETUP, a populated LOAD GAME slot, Edit/Create Team and the
  tournament bracket were not attempted.
* The state→handler table at `0x43dc` and the `0x26B45` caller remain
  runtime-only (LE fixups), so no static proof ties modes 8–11 to a specific
  screen.

## Provenance

```
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu17/tournament1.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-flow-1
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu17/tournament2.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-flow-2
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu17/friendly3.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-flow-3
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu17/friendly4.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-flow-4        # autoexec overflow, no data
CAPTURE_EAX=1 CAPTURE_VIDEO=1 TIMEOUT=410 KEYS_FILE=/tmp/opencode/fu17/friendly5.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-flow-5
CAPTURE_EAX=1 CAPTURE_VIDEO=1 TIMEOUT=240 KEYS_FILE=/tmp/opencode/fu17/calendar6.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-flow-6
python3 tools/fifa96_frames.py captures/session-probe-flow-<N> --fps 1
python3 tools/fifa96_probe.py captures/session-probe-flow-<N>/trace.bin \
    --target-link 0x26B45 --overwrite 5 --expect-site 9 --capture-eax
./build/fifa96_trace captures/session-probe-flow-<N>/trace.bin
```

Static checks used the Ghidra MCP session on `fifa96_le.bin`: disassembly of
`0x25D20–0x25DAF`, `0x27AB0–0x27B5F`, `0x26AA0–0x26B3F`, `0x2D880–0x2DA25`,
`0x2CFAB–0x2CFC0`, `0x2D067–0x2D083` and the 16 call sites of `0x1442c`.
`captures/` and `build/` are git-ignored (derived from copyrighted content);
`game/FIFAPCCD96.iso` was never written (patched copies only). `make test`:
19/19 before and after (no code changed).

## Deliverable status

1. **Keys file: committed** — `tools/keys/fu17-deep-flows.keys` encodes the
   verified tournament deep route (schedule + Standings). It does not reach
   site 7 or the classifier; it is a navigation asset for the next campaign.
2. This document.

## Calendar Simulate retry (2026-10-02)

Follow-up to flow-6, which left the league CALENDAR focus on **Simulate**
(frame 004-0101) but exited at ≈110 s before `enter@116`. This retry presses
it: two classifier runs at `0x26B45`/site 9 with the flow-6 prefix, the risky
`right` presses dropped and the down/enter presses moved later. Result in one
line: **Simulate completes the season instantly (random champions, "Season
Over" calendar with every displayed fixture scored) and does NOT start a
competition match — zero `T_PROBE site=9` frames in both runs, so the site-7
gate stayed closed**.

### Keys

`tools/keys/fu17-simulate.keys` (new, committed; generated with
`/tmp/opencode/fu17/gen.py`, validated with
`python3 tools/fifa96_keys.py --check`):

```
enter@10 down@18 enter@22 enter@30 enter@40 enter@46 down@56 enter@60
enter@66 down@70 right@74 right@78 right@82 enter@86 down@104 down@116
enter@132 enter@160
```

Timeline: league route (the flow-6 prefix) → calendar at 86 s (drawn by
004-0079, ≈88 s wall); `down@104` → Today (004-0105), `down@116` → Simulate
(004-0122); `enter@132` → champions dialog (from 004-0125); `enter@160` →
dismisses it (from 004-0152/0153). Both runs used `TIMEOUT=240`, so the rig,
not the game, ended the captures.

### Runs

| # | session | trace bytes | AVIs / frames | probe | modes | frames-verified states |
|---|---------|------------:|--------------:|------:|-------|------------------------|
| 1 | `probe-sim-1` | 79,882 | 5 / 241 | 0 | none | 004-0079 calendar (Sweden–Switzerland, Brazil–Colombia, Germany–Norway, Wed 17 Aug); 004-0105 `down`#1 → **Today** focus; 004-0122 `down`#2 → **Simulate** focus; 004-0135 dialog **"Spain League Champions"** with ✓; 004-0153 dialog dismissed; 004-0230 season over (Sweden 2-2 Switzerland, Brazil 4-2 Colombia, Germany 3-1 Norway; "Today's Game: Season Over"; Simulate greyed; focus on Standings) |
| 2 | `probe-sim-2` | 80,665 | 5 / 240 | 0 | none | 004-0079 calendar (Denmark–Germany, Netherlands–Italy, Portugal–Mexico, Tue 16 Aug); 004-0122 **Simulate** focus; 004-0135 dialog **"Brazil League Champions"**; 004-0152 dismissed; 004-0230 season over (3-3, 2-2, 2-2; Simulate greyed; focus Standings) |

Decoder output identical in both runs:

```
probe_frames=0
expect_site=0x9 hit=False
```

Both traces end with the normal TSR summary (`SUMMARY patch: ok=5 skip=5`,
`SUMMARY end=END lost=0 seqgaps=0`), and `D:\FEGFX\MOD6` is opened in both
(the league module). Frame means (`tools/fifa96_frames.py`) show the same
transitions in both runs: calendar at 004-0079, a small focus change at
004-0096, dialog 004-0125–0151 (mean ≈11.6 k), season-over state from
004-0152/0153 (mean ≈13.44 k) to the rig kill. The run-6 exit at ≈110 s did
**not** reproduce: both retry runs survived to the 240 s rig kill with no
crash text, so that exit is better explained as a one-off crash than as a
deterministic reaction to the keys.

### What Enter actually did (frame evidence)

* `enter` on Simulate does not open a match. It resolves the whole season in
  one step: a modal **"<country> League Champions"** box (Spain in run 1,
  Brazil in run 2 — the fixture draw and champion are random per run),
  confirmed by the second `enter`, then the calendar returns with every
  displayed fixture scored and "Today's Game" reading **Season Over**;
  Simulate is greyed and the bottom-bar focus ends on Standings.
* The champion dialog is keyboard-dismissable by a plain `enter`; no second
  option or menu is offered.
* The per-fixture `Play` cells were never focused (the `Play` column is empty
  in the calendar frames), so a competition match start remains unverified.

### Site-7 verdict — gate never opened, allotted run not used

Both runs produced zero `T_PROBE site=9` frames, so no `mode=` value existed
to place in 8–11; per the brief the site-7 confirmation run was deliberately
not executed and no `caller_link` is claimed. This is a genuine result, not a
probe failure: the same classifier patch and capture-eax path that produced
FU-16's 145-frame positive control was active, and the trace summaries are
normal. Combined with FU-16/FU-17 (twelve decodable classifier traces now),
the evidence says the league calendar's Simulate path does not call the
competition renderer at `0x26B45`; it bulk-simulates the season without
entering the state 8–11 match flow.

### Honest verdict

The flow-6 gap is closed: Enter on the calendar Simulate was pressed and
frame-verified in two independent runs, and it does **not** start a
competition match. The match engine where states 8–11 live remains unreached;
the keyboard path to a competition match is still unknown (the fixture `Play`
cells stay greyed/empty in our runs, and "No Flagged Games" / "Season Over"
implies a mouse-driven game-flagging step). Site 7 therefore remains at its
historical status.

### Provenance

```
CAPTURE_EAX=1 CAPTURE_VIDEO=1 TIMEOUT=240 \
    KEYS_FILE=/tmp/opencode/fu17/simulate1.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-sim-1
CAPTURE_EAX=1 CAPTURE_VIDEO=1 TIMEOUT=240 \
    KEYS_FILE=/tmp/opencode/fu17/simulate1.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-sim-2
python3 tools/fifa96_frames.py captures/session-probe-sim-<N> --fps 1
python3 tools/fifa96_probe.py captures/session-probe-sim-<N>/trace.bin \
    --target-link 0x26B45 --overwrite 5 --expect-site 9 --capture-eax
./build/fifa96_trace captures/session-probe-sim-<N>/trace.bin
```

`/tmp/opencode/fu17/simulate1.keys` was committed verbatim as
`tools/keys/fu17-simulate.keys` (commit `feat(keys): calendar Simulate
sequence`). `game/FIFAPCCD96.iso` was never written (patched copies only).
`make test`: 19/19 before and after (no code changed).
