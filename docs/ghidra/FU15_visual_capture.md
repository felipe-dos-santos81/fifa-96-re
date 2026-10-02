# FU-15: visual capture and site-7 navigation campaign

Follow-up to `FU14_navigation_campaign.md`. FU-14's blocker was observability:
the headless rig could not say *which* screen the game was on, so navigation
was blind (FILE-mix guessing). FU-15 adds video: `CAPTURE_VIDEO=1` records
DOSBox-X video through `DX-CAPTURE /V` into one AVI per video-mode change,
`tools/fifa96_frames.py` extracts PNGs, and the frames were read directly to
decide the next key sequence. Goal unchanged: reach site 7 (`FUN_00014c18`,
target `0x14C18`) with ≥1 `T_PROBE` frame whose
`caller_link ∈ {0x25D69, 0x27B47}`, cap six attempts.

Result in one line: **site 7 NOT reached** — six valid attempts, zero probe
frames; but the campaign produced the first visually verified map of the
front end, a deterministic navigation model, and a new verified keys file.
The site-8 bonus pass was not run because it is conditioned on site-7 success.

## Method

* `run-fifa96-capture.sh` with `CAPTURE_VIDEO=1` adds `captures=<dir>` to the
  DOSBox-X config and launches `DX-CAPTURE /V FIFA96.EXE`. DOSBox-X starts a
  new AVI on every video-mode change, so the boot/intro/menu phases are
  separate files; the final segment still decodes after the rig SIGKILLs the
  process at timeout.
* `python3 tools/fifa96_frames.py captures/session-<S> --fps 1` writes
  `<session>/frames/fifa96_<seg>-<NNNN>.png` (1 fps) and prints each frame's
  ImageMagick mean. A selection of PNGs was read visually per attempt
  (every ~3rd frame plus all transitions of interest).
* Segments are stable across runs: `000` DOS shell (1.1 s), `001` legal
  notice (3.5 s), `002` EA logo (0.03 s), `003` intro video (5.6 s),
  `004` game (title → menus, until the 120 s timeout). Cumulative time before
  segment 004 is ≈10.3 s, so frame `004-N` ≈ 10.3 + (N−1) s and each key maps
  to a frame index directly.
* Every run: `CAPTURE_VIDEO=1 KEYS_FILE=<f> sh tools/trace_probe.sh 0x14C18 7
  probe-14c18-v<N>` (single-step AUTOTYPE, per FU-13). `TIMEOUT=120`.

## Screen inventory (observed on video)

| screen | how reached | visual |
|--------|-------------|--------|
| Title | automatic after `enter` at 10 s skips intro | FIFA 96 / Virtual Stadium Soccer art |
| GAME SELECT | automatic by ~14–15 s | left: Load Game, Options, Modem Setup, Quit to DOS; right: Friendly, Leagues, Tournaments, Playoffs, Practise; highlight starts on Friendly |
| FRIENDLY | `enter` on Friendly | Home/Visitor team select; country map + flag; International/Brazil/Italy rows; Edit Team (greyed)/Create Team/Play; `enter`/`space` cycles the home country (Brazil→Germany→England; v2: Brazil→USA after six enters) |
| LEAGUE SELECT | `down` then `enter` | green photo background; "International" tab; 12-flag grid; back-arrow widget (bottom right) |
| TOURNAMENT | `down`×2 then `enter` | purple variant of the flag grid |
| PLAYOFF | `down`×3 then `enter` | red variant of the flag grid |
| PRACTISE | `down`×4 then `enter` | training-photo background; "Practise Team" select; Play |
| LOAD GAME | `left` then `enter` | panel over GAME SELECT; ten empty save slots; X/✓ buttons |
| OPTIONS | `left`, `down`, `enter` | full-screen list: Music [ON], Music Volume, Sound Effects [ON], SFX Volume, Play by Play [ON], Speech Volume, Language [ENGLISH], Super VGA [OFF]; Joystick Calibration / Default / X / ✓; `enter` toggles (Sound Effects ON→OFF observed) |
| MODEM SETUP | not reached | — (v6 predicted the return focus wrongly; see below) |

## Navigation model (tested)

* `enter`/`space` activate; `esc` goes back (verified from LEAGUE, LOAD GAME,
  OPTIONS).
* `left`/`right` switch focus columns (`left` from Friendly → Load Game).
* `down` order observed: left column Load Game → Options → Modem Setup →
  Quit to DOS → Practise (from Quit it lands on the right column's bottom
  item), then clamps at Practise; right column Friendly → Leagues →
  Tournaments → Playoffs → Practise, clamping at Practise.
* Returning to GAME SELECT resets the highlight to the focus column's first
  item (right column → Friendly; v5's LOAD GAME return kept the left column
  at Load Game). The return focus is screen-dependent — v6's OPTIONS return
  went to the right column, so `down`×2 landed on Tournaments and MODEM SETUP
  was never opened.
* The committed FU-14 `menu-nav.keys` presses `down` at 14 s, racing the
  title→menu transition (v2: swallowed — highlight stayed Friendly; FU-14
  m1r2: caught). The FU-14 "MOD1/MOD4 deep screen" is the **OPTIONS** screen
  (v6 matched its FILE mix and its Music/SFX list). It does not call site 7.
* `tools/keys/fu15-verified-nav.keys` (new) encodes the timing-hardened,
  frame-verified route: skip intro → down at 18 s → LEAGUE at 22 s → esc at
  34 s → left at 42 s → down at 46 s → OPTIONS at 52 s.

## Per-attempt results

All runs `TIMEOUT=120`; `frames` = extracted video PNGs; `probe frames` =
`T_PROBE site=7` decoder frames. Trace bytes from
`captures/session-<session>/trace.bin`. FILE-mix notes list the non-baseline
modules (baseline in every run: `LAN, LEGA, PLAYAR, GAMEAR, PCC, VID_, TITL,
VS.P, SLIC, HELV, MOUS`).

| # | keys (head) | session | trace bytes | video frames | probe frames | screens observed (frames) | non-baseline FILE mix |
|---|-------------|---------|------------:|-------------:|-------------:|---------------------------|-----------------------|
| 1 | `enter` … `space`×3/`enter`/`space`/`enter` (`skip-intro.keys`) | `probe-14c18-v1` | 76,868 | 120 | 0 | GAME SELECT (Friendly) → FRIENDLY; enter/space cycles home country (Brazil→Germany→England) | `MOD2`×2 `MOD5`×2 |
| 2 | `enter down enter×6` (`menu-nav.keys`) | `probe-14c18-v2` | 76,607 | 121 | 0 | same as #1; the 14 s `down` was swallowed during the title→menu transition | `MOD2`×2 `MOD5`×2 |
| 3 | `enter down@18 enter@22 esc@28 down@32 enter@36` | `probe-14c18-v3` | 81,883 | 120 | 0 | GAME SELECT (down moves Friendly→**Leagues**) → LEAGUE flag grid → esc back (highlight reset) → LEAGUE again | `MOD2`×4 `MOD4`×4 |
| 4 | `enter down×2 enter esc down×3 enter esc down×4 enter` | `probe-14c18-v4` | 88,659 | 120 | 0 | TOURNAMENT → PLAYOFF → PRACTISE (flag grid variants + practise team select) | `MOD2`×6 `MOD4`×4 `MOD9`×2 |
| 5 | `enter left enter esc down×5 enter esc down×6 enter` | `probe-14c18-v5` | 84,927 | 120 | 0 | `left`→Load Game highlight → **LOAD GAME** dialog (10 empty slots) → esc, then down×5 walks Load→Options→Modem Setup→Quit→Practise → PRACTISE; final leg clamps at Practise → PRACTISE | `MOD2`×4 `MOD9`×4 |
| 6 | `enter left down enter` + in-screen probes + esc + `down×2 enter` + probes | `probe-14c18-v6` | 88,557 | 120 | 0 | **OPTIONS** (Music/SFX list; enter toggled Sound Effects OFF); esc returned focus to the right column, so the follow-up opened TOURNAMENT (Spain) instead of MODEM SETUP | `MOD2`×4 `MOD1`×4 `MOD4`×4 |

No attempt was a zero-information failure: every run produced a decodable
120 s video and a non-trivial trace; attempts 3–6 each revealed at least one
previously unseen screen, so the two-consecutive-no-new-screen stop rule never
triggered. The cap of six site-7 attempts is exhausted.

## Site 7 — `FUN_00014c18`: NOT reached

All six traces contain **zero** `T_PROBE` lines. The decoder output is
identical for every run:

```
probe_frames=0
expect_site=0x7 hit=False
```

Static recap (re-checked this campaign, unchanged from FU-12/FU-14): the only
two direct callers are `0x25D64` and `0x27B42`, both inside front-end state
handlers — gated by runtime state `EBX∈{10,11}` at `0x25D4F–0x25D57` and
`ESI==10` at `0x27B1A` — and both load `EAX=[0x47C30]` (the VIV table also
used by `FUN_00023b38`) before calling the 640×480 still loader. Disassembly
of the dispatcher (`0x25CFB–0x25DE5`) shows the still path is a *separate*
branch for states 10/11 from the general draw path (`FUN_0001a280`) used by
the screens we visited. None of the eight screens reachable from GAME SELECT
presents state 10/11 to these handlers within 120 s.

**Goal 1 not met.** This is an honest reachability zero with full visual
evidence, not a crash: every run kept drawing 120 video frames until the rig
killed DOSBox-X, and every trace ends with the normal TSR `T_END`/summary.

## Site 8 — bonus pass not run

The brief runs the site-8 pass only if site 7 is reached. It was not, so no
site-8 attempt was made. The known distinct caller set therefore remains
`{0x24227, 0x24282, 0x246DA}` (3 of 12) from FU-13; no caller_links beyond
those are claimed.

## Remaining untested (for the next campaign)

* MODEM SETUP (the only GAME SELECT leaf not opened; v6 reached everything
  else and predicted its esc return wrongly).
* Deeper interactions: confirming a country/league in LEAGUE/TOURNAMENT/
  PLAYOFF; the Play button on FRIENDLY/PRACTISE (match start, team
  management); Edit/Create Team; Load Game with a populated slot.
* Since the site-7 handler is gated on two adjacent states (10 and 11), a
  more likely trigger is a *deeper* screen whose ID is 10/11, not a
  first-level menu leaf.

## Provenance

Keys were generated under `/tmp/opencode/fu15/` (`v3.keys` … `v6.keys`) and
validated with `python3 tools/fifa96_keys.py --check`. Exact commands:

```
CAPTURE_VIDEO=1 KEYS_FILE=tools/keys/skip-intro.keys sh tools/trace_probe.sh 0x14C18 7 probe-14c18-v1
CAPTURE_VIDEO=1 KEYS_FILE=tools/keys/menu-nav.keys sh tools/trace_probe.sh 0x14C18 7 probe-14c18-v2
CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu15/v3.keys sh tools/trace_probe.sh 0x14C18 7 probe-14c18-v3
CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu15/v4.keys sh tools/trace_probe.sh 0x14C18 7 probe-14c18-v4
CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu15/v5.keys sh tools/trace_probe.sh 0x14C18 7 probe-14c18-v5
CAPTURE_VIDEO=1 KEYS_FILE=/tmp/opencode/fu15/v6.keys sh tools/trace_probe.sh 0x14C18 7 probe-14c18-v6
python3 tools/fifa96_frames.py captures/session-probe-14c18-v<N> --fps 1
python3 tools/fifa96_probe.py captures/session-probe-14c18-v<N>/trace.bin --target-link 0x14C18 --overwrite 6 --expect-site 7
./build/fifa96_trace captures/session-probe-14c18-v<N>/trace.bin
```

`captures/` and `build/` are git-ignored (derived from copyrighted content);
`game/FIFAPCCD96.iso` was never modified (patched copies only). The new
`tools/keys/fu15-verified-nav.keys` is committed; `menu-nav.keys` is left
untouched (its FU-14 timing race is documented here instead). `make test`:
19/19 before and after (no code changed).

## Goals status (explicit)

* Goal 1 (site-7 frame, `caller_link ∈ {0x25D69, 0x27B47}`): **NOT MET** —
  six valid attempts, 0 probe frames.
* Goal 2 (site-8 bonus): **NOT RUN** (conditional on Goal 1); no new
  caller_links claimed.
