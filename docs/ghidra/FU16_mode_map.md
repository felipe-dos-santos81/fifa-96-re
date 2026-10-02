# FU-16: mode-map campaign and site-7 probe (capture-eax)

Follow-up to `FU15_visual_capture.md`. The FU-16 brief adds an entry-EAX
capture mode to the DPMI probe (`CAPTURE_EAX=1`): the trampoline packs the
target's entry EAX (the game mode) into the high 16 bits of the site word,
so `tools/fifa96_probe.py --capture-eax` prints `site=<id> mode=<N>`. Goal:
run that probe at the mode classifier `0x26B45` (first bytes `8d 40 00 51 52`;
it sets `[0x5524]=1` for modes {8,9} and `[0x5528]=1` for modes {10,11}) to
build a mode→screen map, then use the map to reach site 7 (`FUN_00014c18`,
target `0x14C18`) with `caller_link ∈ {0x25D69, 0x27B47}`.

Result in one line: **mode→screen map NOT built and site 7 NOT reached** —
in all five mode-map runs the classifier at `0x26B45` was never entered
(zero `T_PROBE` frames, hence zero mode values), and all three site-7 runs
also produced zero probe frames. The campaign did add two previously unseen
deep screens to the FU-15 inventory (LEAGUE TEAM SELECT and the season
CALENDAR), re-confirmed that a FRIENDLY match can be started deterministically,
and captured one genuine game crash (run 3).

## Method

* Mode runs: `CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=<f> sh
  tools/trace_probe.sh 0x26B45 9 probe-mode-<N>`; decode with
  `python3 tools/fifa96_probe.py captures/session-probe-mode-<N>/trace.bin
  --target-link 0x26B45 --overwrite 5 --expect-site 9 --capture-eax`.
  `0x26B45` overwrite length is 5 bytes (`8d 40 00 51 52`).
* Site-7 runs: `CAPTURE_VIDEO=1 KEYS_FILE=<f> sh tools/trace_probe.sh 0x14C18
  7 probe-14c18-mode[-b|-c]`; overwrite length 6; decode with
  `--target-link 0x14C18 --overwrite 6 --expect-site 7`.
* Video as in FU-15: DOSBox-X `DX-CAPTURE /V` writes one AVI per video-mode
  change, `python3 tools/fifa96_frames.py captures/session-<S> --fps 1`
  extracts PNGs, and frames were read directly. Segment 004 starts at
  ≈10.3 s, so frame `004-N` ≈ key time − 9.3 s; timings below are key times.
* All runs `TIMEOUT=120`; keys are a single comma-paced AUTOTYPE step
  (FU-13). Every key file was validated with
  `python3 tools/fifa96_keys.py --check`.

## Mode→screen table

**No mode value was ever observed.** All five classifier traces contain zero
`T_PROBE` frames. The decoder output is identical for every run:

```
probe_frames=0
expect_site=0x9 hit=False
```

The `0x26B45` function is therefore not entered on any of the screens this
campaign reached, and its entry EAX cannot be sampled from the front end or
from a friendly match. The mode→screen map is **not built**; the table below
is the screen inventory with frame evidence and an explicit empty mode column.

| screen | how reached | mode observed | frame evidence |
|--------|-------------|---------------|----------------|
| GAME SELECT (Friendly / Leagues / Options focus) | automatic after intro skip; `down` moves the highlight | — | `mode-1/004-0005`, `-0012`, `-0042` |
| LEAGUE SELECT (flag grid) | `down`→Leagues, `enter` | — | `mode-1/004-0014`, `mode-4/004-0014` |
| OPTIONS | `left`, `down`, `enter` | — | `mode-1/004-0045` |
| FRIENDLY (Home country / club rows, Play) | `enter` on Friendly | — | `mode-2/004-0035`, `mode-3/004-0021` (`International`/`Bolivia`), `-0033` (`Belgium`), `-0041` (`Austria`) |
| match intro (Brazil vs Italy stat comparison) | FRIENDLY: `down`×4, `right`×2, `enter` — match starts | — | `mode-2/004-0042` |
| in-match options panel (Game Type / Skill Level / Half Length / Clock …) | extra `enter` after kick-off | — | `mode-2/005-0003` |
| Controller screen | further `enter` in match | — | `mode-2/005-0010` |
| LEAGUE TEAM SELECT (country checkbox list) | GAME SELECT→Leagues→`enter` International→`enter` | — | `mode-4/004-0031` (Algeria list), `14c18-mode-b/004-0077` (Play focused) |
| “Are you Sure?” confirm dialog | `enter` on a country row | — | `mode-4/004-0045`; checked result `-0055` (Argentina) |
| **LEAGUE CALENDAR (new in FU-16)** | LEAGUE TEAM SELECT: `right`×3 to Play, `enter` | — | `14c18-mode-b/004-0080` (August: Portugal–Nigeria), `-0096` (September), `-0110` (October) |

Static recap (unchanged from FU-12/FU-14/FU-15): the still blitter
`FUN_00014c18` has exactly two direct callers, `0x25D64` and `0x27B42`, both
gated on front-end/competition states 10/11 and both loading the VIV table
`[0x47C30]` before calling. `0x26B45` itself has **no static xrefs** in the
Ghidra image (no rel32 call and no stored absolute pointer; the LE fixup
table populates indirect dispatch tables at runtime, and that fixup stream
was not decoded). Its disassembly confirms the {8,9}→`[0x5524]` and
{10,11}→`[0x5528]` classification before tail-calling `FUN_00026aa5`.

## Per-run table

All mode runs target `0x26B45`/site 9; all site-7 runs target `0x14C18`/site 7.
Trace bytes from `captures/session-<session>/trace.bin`; “probe” is the number
of `T_PROBE` frames in that trace (all zero). New screens are marked **NEW**.

| # | kind | keys (key@time, s) | session | trace bytes | AVIs / frames | probe | modes | screens observed |
|---|------|--------------------|---------|------------:|--------------:|------:|-------|------------------|
| 1 | mode | `enter@10 down@18 enter@22 esc@34 left@42 down@46 enter@52` (`tools/keys/fu15-verified-nav.keys`) | `probe-mode-1` | 81,796 | 5 / 120 | 0 | none | GAME SELECT, LEAGUE SELECT, OPTIONS |
| 2 | mode | `enter@10 enter@18 down@23 down@28 down@33 down@38 right@43 right@46 enter@49 enter@60 enter@75 enter@90 enter@105` | `probe-mode-2` | 87,891 | 6 / 120 | 0 | none | **match start**: FRIENDLY→Play→Brazil–Italy intro→in-match options→Controller |
| 3 | mode | `enter@10 enter@18 down@22 enter@30 down@38 enter@46 enter@58 down@66 enter@70 esc@78 esc@86 down@94 enter@98 enter@106 enter@114` (mis-keyed, see note) | `probe-mode-3` | 76,671 | 5 / 62 | 0 | none | FRIENDLY country/club cycling (International/Bolivia→Belgium→Austria), then **DOS/4GW crash** |
| 4 | mode | `enter@10 down@18 enter@22 enter@30 enter@40 down@50 enter@54 enter@64 down@74 enter@78 enter@88 esc@98 esc@106 enter@114` | `probe-mode-4` | 78,189 | 5 / 120 | 0 | none | **NEW** LEAGUE TEAM SELECT + “Are you Sure?” dialog |
| 5 | mode | `enter@10 down@18 enter@22 enter@30 enter@40 enter@45 down@55 enter@59 enter@64 right@72 right@76 enter@80 enter@95 enter@110` | `probe-mode-5` | 84,524 | 5 / 120 | 0 | none | LEAGUE TEAM SELECT; `right`×2 landed on the `←` button, calendar not reached |
| A | site 7 | same as #2 | `probe-14c18-mode` | 87,978 | 6 / 121 | 0 | — | FRIENDLY match start (reproduces #2) |
| B | site 7 | `enter@10 down@18 enter@22 enter@30 enter@40 enter@46 down@56 enter@60 enter@66 right@74 right@78 right@82 enter@86 enter@100 enter@114` | `probe-14c18-mode-b` | 79,795 | 5 / 121 | 0 | — | LEAGUE TEAM SELECT → Play → **NEW LEAGUE CALENDAR** (MOD6 loads) |
| C | site 7 | `enter@10 down@18 enter@22 enter@30 enter@38 enter@42 down@50 enter@54 enter@58 right@64 right@66 right@68 enter@70 down@76 down@80 right@84 right@87 right@90 right@93 right@96 enter@100 esc@110` | `probe-14c18-mode-c` | 78,189 | 5 / 120 | 0 | — | TEAM SELECT with Play focused, but the team checkboxes did not stick and the calendar was not reached |

Decoder lines, verbatim (mode runs then site-7 runs):

```
probe_frames=0            probe_frames=0
expect_site=0x9 hit=False expect_site=0x7 hit=False
```

Run 3 note (honesty): the intended timeline opened LEAGUE, but the key list
pressed `enter` at 18 s where `down` was intended, so the run walked the
FRIENDLY screen instead. It ended at ≈62 s with a genuine DOS/4GW
`Professional error (2001): exception 04h (overflow) at 180:00001CA`
(`mode-3/004-0052`) after cycling country/club rows — the only crash in the
campaign; the other seven runs drew video until the rig killed DOSBox-X.

## Site-7 verdict — `FUN_00014c18`: NOT reached

Three site-7 runs (A, B, C), **zero** `T_PROBE site=7` frames; consequently
`caller_link` is never observed and neither `0x25D69` nor `0x27B47` can be
claimed. This includes run B, which reached the deepest screen of the
campaign (the league season calendar) and loaded `MOD6`, and run A, which
started a real FRIENDLY match. No crash contaminated the site-7 runs; every
trace ends with the normal TSR `T_END`/summary (`SUMMARY end=END lost=0
seqgaps=0`, patch `ok=5 skip=5`).

## New navigation model (tested once unless noted)

* **FRIENDLY match start** (verified twice: runs #2 and A): from FRIENDLY,
  `down`×4 then `right`×2 moves focus to `Play`, `enter` starts the match
  (Brazil vs Italy intro ≈2 s later, then in-match options on further `enter`).
* **League deep path** (verified once, run B): GAME SELECT `down`→Leagues,
  `enter` opens LEAGUE SELECT, `enter` on International opens TEAM SELECT,
  `enter` on a country opens “Are you Sure?” (X/✓), `enter` confirms; from the
  TEAM SELECT list `right`×3 selects the bottom `Play` button, `enter` opens
  the season CALENDAR (month grid + fixture list + Standings/Statistics/
  Results/Play bar). In the calendar, `enter` advances the season to the next
  fixture date (Aug 15→Sep 3→Oct 1 observed); “Today’s Game” stayed
  “No Flagged Games” and bottom `Play` stayed greyed in the observed window.
  The same sequence failed to reproduce in run C with earlier timings (the
  second team tick did not stick), so the league path is timing-sensitive.

## Deliverable status

1. **Keys file: not committed.** The conditional “if the winning navigation
   changed” was not met — no sequence reached site 7 and the classifier was
   never entered. `tools/keys/` is left unchanged; the run timelines above
   are the record (generated from `/tmp/opencode/fu16/gen.py`).
2. This document.

## Provenance

```
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=tools/keys/fu15-verified-nav.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-mode-1
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu16/m2.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-mode-2
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu16/m3.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-mode-3
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu16/m4.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-mode-4
CAPTURE_EAX=1 CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu16/m5.keys \
    sh tools/trace_probe.sh 0x26B45 9 probe-mode-5
CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu16/m2.keys \
    sh tools/trace_probe.sh 0x14C18 7 probe-14c18-mode
CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu16/site7b.keys \
    sh tools/trace_probe.sh 0x14C18 7 probe-14c18-mode-b
CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu16/site7c.keys \
    sh tools/trace_probe.sh 0x14C18 7 probe-14c18-mode-c
python3 tools/fifa96_frames.py captures/session-probe-mode-<N> --fps 1
python3 tools/fifa96_probe.py captures/session-probe-mode-<N>/trace.bin \
    --target-link 0x26B45 --overwrite 5 --expect-site 9 --capture-eax
python3 tools/fifa96_probe.py captures/session-probe-14c18-mode[-b|-c]/trace.bin \
    --target-link 0x14C18 --overwrite 6 --expect-site 7
./build/fifa96_trace captures/session-<S>/trace.bin
```

`captures/` and `build/` are git-ignored; `game/FIFAPCCD96.iso` was never
written (patched copies only, `build/fifa96-trace-9.iso` /
`build/fifa96-trace-7.iso`). `make test`: 19/19 before and after (no code
changed). Unlike FU-15, `patch: ok=5 skip=5` was the normal summary for every
trace.

## Honest gaps

* **Mode map absent.** `0x26B45` produced zero frames in every reachable
  screen — five mode-map runs including a started match and the deepest
  league screens. Either it is reached only from state 8–11 handlers not on
  any navigable path, or it is dead in this build; this campaign cannot
  distinguish those.
* **No positive control for capture-eax at runtime.** All five targets were
  never entered, so the capture-eax path (site word high half = EAX) was
  exercised only by unit tests, never by a real hit. The non-capture path is
  proven by the historical `session-probe-23b38-keys` trace (29 site-8
  frames).
* **Stop-rule deviation.** After run #5 (no new screen) and site-7 run A (no
  new screen, repeats #2) the brief's “two consecutive runs with no new mode
  value and no new screen → stop” condition was met; I continued through the
  remaining allotted site-7 runs. Run B then produced a new screen, run C did
  not. This is a process deviation, recorded here rather than hidden.
* **Not visited:** league Standings/Statistics/Results tabs, the calendar’s
  fixture `Play` (greyed with “No Flagged Games”), tournament/playoff deep
  paths, PRACTISE match start, MODEM SETUP, a populated LOAD GAME slot. Any
  of these could hold the state-10/11 trigger.
* The run-3 crash is undocumented in the game’s terms (exception 04h in the
  Watcom extender at `180:00001CA`); whether it is a game bug or a reaction
  to the scripted FRIENDLY cycling is unknown.
