# FU-18: site-8 caller remap (`FUN_00023b38`)

Follow-up to `FU17_deep_flows.md`. FU-13 fired site 8 (`FUN_00023b38`, target
`0x23B38`) once and matched three of its twelve static callers
(`0x24227`, `0x24282`, `0x246DA`); FU-14 re-fired two of those and added
nothing. FU-18's brief: drive the table-frame loader from the visually
mapped screens and collect `T_PROBE site=8` `caller_link` values beyond the
three known. Cap: six runs, stop after two consecutive runs with no new
`caller_link`.

Result in one line: **five new callers mapped in two productive runs**
(`0x242BF`, `0x2431C`, `0x24373`, `0x25DDA`, `0x24789`); the loader is a
FRIENDLY team-select/front-end draw path, not a tournament/league one — the
tournament deep route produced a clean zero. Four of twelve callers remain
unmapped (`0x24062`, `0x240C1`, `0x2488F`, `0x24938`); three of the four are
statically Visitor-side redraws, and the keyboard routes found so far never
focus the Visitor team column.

## Method

* Per run: `CAPTURE_VIDEO=1 TIMEOUT=<t> KEYS_FILE=<f> sh
  tools/trace_probe.sh 0x23B38 8 probe-s8-<N>` (site-8 overwrite length 5).
  The wrapper decodes with `python3 tools/fifa96_probe.py <trace>
  --target-link 0x23B38 --overwrite 5 --expect-site 8`.
* Video as in FU-15…FU-17: DOSBox-X `DX-CAPTURE /V` writes one AVI per
  video-mode change, `python3 tools/fifa96_frames.py
  captures/session-probe-s8-<N> --fps 1` extracts PNGs, and frames were read
  directly. Segment 004 starts at ≈10.3 s, so frame `004-N` ≈ 10.3 + (N−1) s;
  key times below are absolute seconds from launch.
* Caller attribution: the T_PROBE stream has no timestamps, so callers were
  attributed by **burst composition** (a screen entry is one burst; each
  FRIENDLY value cycle is a `0x24227+0x24282` pair followed by one
  `0x246DA`/`0x24789`) cross-checked against the frame-verified screen at
  that time. Trace FILE opens anchor bursts to module loads
  (`MOD5` = FRIENDLY team select; `MOD2` = GAME SELECT).
* Static checks used the open Ghidra project on `fifa96_le.bin` (LE-image
  offsets, the same space as the probe's `caller_link`); `fifa96.exe` was
  never modified.

## Caller → screen/engine table (mapped callers)

`caller_link` = static `ret` address = opcode+5. "Screen" is the
frame-verified screen live when the caller's burst fired; frame ids are
`captures/session-probe-s8-<N>/frames/fifa96_004-NNNN.png`.

| caller_link | static site | screen / engine | frame-verified evidence | first mapped |
|-------------|-------------|-----------------|-------------------------|--------------|
| `0x24227` | `0x24222` in the front-end menu draw switch (~`0x24200`, case → `0x24213`) | FRIENDLY team select: row-frame pair on screen entry and on every Home value cycle; also (burst-attributed) team-editor entry | s8-2 `004-0018` (FRIENDLY entry), `004-0089` (team editor); s8-3 `004-0024` France/Auxerre, `004-0066` Brazil/A Mineiro, `004-0096` USA/Atlanta | FU-13 (known) |
| `0x24282` | `0x2427D`, same switch (case → `0x24261`) | same as `0x24227` (always paired with it) | same frames as `0x24227` | FU-13 (known) |
| `0x242BF` | `0x242BA`, same switch (case → `0x242AE`) | FRIENDLY team-select entry draw (one specific row/widget) | s8-2 `004-0018`/`004-0025` entry burst (4×), absent from value-cycle and editor bursts | **FU-18 (new)** |
| `0x2431C` | `0x24317`, same switch (case → `0x24308`) | FRIENDLY team-select entry draw and (burst-attributed) team-editor entry | s8-2 `004-0018` (entry), `004-0089` (editor) | **FU-18 (new)** |
| `0x24373` | `0x2436E`, same switch (case → `0x2435F`) | FRIENDLY team-select entry draw and (burst-attributed) team-editor entry (most frequent of the two) | s8-2 `004-0018`, `004-0089` | **FU-18 (new)** |
| `0x25DDA` | `0x25DD5`, tail of the table-frame cache reset (`EAX=-1` → `FUN_00023b38`; static `0x25DBC–0x25DE5`) | screen-transition cache reset: fires as the last frame of the FRIENDLY-entry burst and again at (burst-attributed) team-editor entry | s8-2 entry burst end, editor burst end | **FU-18 (new)** |
| `0x246DA` | `0x246D5` in block `0x24680` (sets `[0x5470] = index−1`, draws slot 0) | Home value cycle to the previous/wrapping value | s8-3 cycles (9×), s8-4 (6×), s8-5 (3×); e.g. s8-3 `004-0024`, `004-0066` | FU-13 (known), frame-mapped FU-18 |
| `0x24789` | `0x24784` in block `0x24740` (sets `[0x5470] = (index+1) % count`, draws slot 0) | Home value cycle to the next value | s8-3 `004-0066`–`004-0096` (3×) | **FU-18 (new)** |

Counts per run (decoded frames): s8-2 `0x24227`×6 `0x24282`×6 `0x242BF`×4
`0x2431C`×4 `0x24373`×8 `0x25DDA`×2; s8-3 `0x24227`×33 `0x24282`×33
`0x246DA`×9 `0x24789`×3; s8-4 `0x24227`×25 `0x24282`×25 `0x246DA`×6;
s8-5 `0x24227`×8 `0x24282`×8 `0x246DA`×3.

Static context for the switch: the five `0x242xx`/`0x243xx` call sites are
distinct cases of one front-end draw routine that dispatches on the low byte
of its return address (`MOV AL,[ESP]; JMP CS:[EAX*4+0x140FC]` and a second
dispatch `…+0x1411C` later in the routine); both tables are
LE-fixup-populated at runtime, so the case→screen binding can only be
resolved at runtime. The `0x24680`/`0x24740` blocks compute the previous /
next list index (`[0x5470]`) and redraw slot 0; `0x24840`/`0x24900` are
their slot-1 (Visitor) twins (see below).

## Per-run table

All runs target `0x23B38`/site 8. Trace bytes from
`captures/session-probe-s8-<N>/trace.bin`; "probe" = decoded `T_PROBE site=8`
frames; distinct = distinct `caller_link`s; modules from
`./build/fifa96_trace` (baseline every run: `LAN, LEGA, PLAYAR, GAMEAR,
PCC, TITL, VS.P, SLIC, HELV, MOUS, MOD2`).

| # | keys | session | trace bytes | AVIs / frames | probe | distinct callers | screens observed (frame evidence) |
|---|------|---------|------------:|--------------:|------:|------------------|----------------------------------|
| 1 | `tools/keys/fu17-deep-flows.keys` | `probe-s8-1` | 80,752 | 5 / 150 | 0 | none | TOURNAMENT grid (`004-0025`) → TEAM SELECT → SCHEDULE (`004-0093`) → STANDINGS; modules `MOD2 MOD4 MOD1 MOD7` |
| 2 | `fu18-friendly-walk.keys` | `probe-s8-2` | 81,512 | 5 / 151 | 30 | `0x24227` `0x24282` `0x242BF` `0x2431C` `0x24373` `0x25DDA` | FRIENDLY team select (`004-0018`, `004-0025` Create Team focus, `004-0050`/`004-0060` Visitor row highlighted) → **team editor** Brazil roster (`004-0089`, `004-0105`) → GAME SELECT (`004-0125`); modules `MOD2 MOD5` |
| 3 | `fu18-friendly-cycle.keys` | `probe-s8-3` | 78,506 | 5 / 161 | 78 | `0x24227` `0x24282` `0x246DA` `0x24789` | FRIENDLY team select, Home values cycled: France/Auxerre (`004-0024`), Brazil/A Mineiro (`004-0066`), USA/Atlanta (`004-0096`), Sweden/Trelleborg (`004-0130`); Visitor stayed International/Italy; modules `MOD2 MOD5` |
| 4 | `/tmp/opencode/fu18/friendly-visitor.keys` | `probe-s8-4` | 78,044 | 5 / 151 | 56 | `0x24227` `0x24282` `0x246DA` | FRIENDLY team select, Home values cycled: USA/Atlanta (`004-0060`), Sweden/Öster (`004-0100`); Visitor unchanged; modules `MOD2 MOD5` |
| 5 | `/tmp/opencode/fu18/friendly-visitor2.keys` | `probe-s8-5` | 77,180 | 5 / 49 | 19 | `0x24227` `0x24282` `0x246DA` | FRIENDLY team select, France/Auxerre (`004-0030`, `004-0039`); **early exit** at ≈49 s (see crashes) |

Runs 4 and 5 added no new `caller_link` (both repeat the Home-cycle set), so
the two-consecutive-no-new stop rule fired after run 5; the sixth run was
deliberately not used.

## Negative result: tournament/league screens do not call site 8

Run 1 reached the full FU-17 tournament route (grid → team select → schedule
→ standings; frames `004-0025`, `004-0093`, `004-0100`) and produced
**zero** site-8 frames while loading `MOD1/MOD4/MOD7` (no `MOD5`). The
tournament/league routes in earlier campaigns (`probe-flow-1`, `probe-sim-1`)
also never open `MOD5`, while every run that fired site 8 did. The observed
correlation — site 8 fires only on the `MOD5`-family (FRIENDLY) screens — is
a useful pre-filter for future site-8 campaigns: no `MOD5` open ⇒ expect
zero site-8 frames.

## Unmapped callers and the flows that would likely reach them

| caller_link | static site | static role | likely flow |
|-------------|-------------|-------------|-------------|
| `0x24062` | `0x2405D` in `FUN_00024044` | **Home-side** slot-0 redraw wrapper (`EAX=0`, `EDX=[0x5470]`, then `0x12D7C`) called from the selection handler at `0x24A29` (`EBX==4`, `EAX!=0`) and `0x24A86` (`EBX==3`) | a team *selection* event on the Home side (choose/confirm the Home team) |
| `0x240C1` | `0x240BC` in `FUN_000240a0` | **Visitor-side** slot-1 redraw wrapper (`EAX=1`, `EDX=[0x5478]`, then `0x10ED4`) called at `0x24A0F` (`EBX==4`, `EAX==0`) and `0x24A64` (`EBX==3`) | same selection event on the Visitor side |
| `0x2488F` | `0x2488A` in block `0x24840` (sets `[0x5478]`, draws slot 1 via `0x23EB8`) | Visitor value cycle to the previous/wrapping value | focus the Visitor team row and cycle its value (left/right) |
| `0x24938` | `0x24933` in block `0x24900` (same, `[0x5478]`/`[0x547C]`) | Visitor value cycle to the next value | same as `0x2488F` |

In all five runs the Visitor column stayed `International / Italy` and no
keyboard sequence tried (up/down through the bottom buttons, left/right,
`space`/`enter` cycling) moved the active focus to the Visitor rows; the
probe never saw the slot-1 redraw callers. The static evidence is strong:
`0x2488F`/`0x24938` are the Visitor twins of the mapped Home
`0x246DA`/`0x24789`, and `0x24062`/`0x240C1` are the selection-handler
redraw wrappers for the Home/Visitor columns. A next campaign should first
solve "how to focus the Visitor team row" (candidate: the run-2 `up` path
from the bottom buttons reached a Visitor-highlighted state at
`004-0050`/`004-0060`; cycling there was not attempted with the right
focus), then cycle with `space`/`left`/`right` and press `enter` to trigger
the selection wrappers.

## Crashes, aborts and data quality (honest log)

* Run 5 exited at ≈49 s wall: video segment 004 stops at `004-0039` (last
  frame still FRIENDLY France/Auxerre), the trace ends with the normal TSR
  `END`, and no error text was captured. This matches the FU-16 run-3 crash
  pattern (DOS/4GW exception while cycling country/club rows) but is not
  proven; it is reported as an unexplained early exit.
* Runs 2–5 show serial decode loss (`SUMMARY end=END lost=10…39
  seqgaps=7…32`). This is the same order as FU-13's site-8 success trace
  (`lost=15 seqgaps=11`): the probe writes bursts of 21-byte frames and the
  capture serial drops some. Decoded callers are genuine, but lost frames
  could hide additional calls; the distinct-caller counts are lower bounds.
* Run 2's `MOD2 opens=4` (vs 2 elsewhere) is the return to GAME SELECT after
  the final `esc`; its second probe burst (`24227 24282 2431C 24373 25DDA`)
  is attributed to the team-editor entry (`004-0089`), the only screen change
  between the FRIENDLY entry and the return to GAME SELECT. The burst itself
  carries no timestamp, so that attribution is by elimination, not proof.

## Provenance

```
CAPTURE_VIDEO=1 TIMEOUT=150 KEYS_FILE=tools/keys/fu17-deep-flows.keys \
    sh tools/trace_probe.sh 0x23B38 8 probe-s8-1
CAPTURE_VIDEO=1 TIMEOUT=150 KEYS_FILE=/tmp/opencode/fu18/friendly-walk.keys \
    sh tools/trace_probe.sh 0x23B38 8 probe-s8-2
CAPTURE_VIDEO=1 TIMEOUT=160 KEYS_FILE=/tmp/opencode/fu18/friendly-cycle.keys \
    sh tools/trace_probe.sh 0x23B38 8 probe-s8-3
CAPTURE_VIDEO=1 TIMEOUT=150 KEYS_FILE=/tmp/opencode/fu18/friendly-visitor.keys \
    sh tools/trace_probe.sh 0x23B38 8 probe-s8-4
CAPTURE_VIDEO=1 TIMEOUT=170 KEYS_FILE=/tmp/opencode/fu18/friendly-visitor2.keys \
    sh tools/trace_probe.sh 0x23B38 8 probe-s8-5
python3 tools/fifa96_frames.py captures/session-probe-s8-<N> --fps 1
python3 tools/fifa96_probe.py captures/session-probe-s8-<N>/trace.bin \
    --target-link 0x23B38 --overwrite 5 --expect-site 8
./build/fifa96_trace captures/session-probe-s8-<N>/trace.bin
```

Timelines were generated with `/tmp/opencode/fu17/gen.py` and validated with
`python3 tools/fifa96_keys.py --check`. Static checks used the Ghidra MCP
session on `fifa96_le.bin`: disassembly of `0x24200–0x243BF`,
`0x24680–0x247A9`, `0x24840–0x2495F`, `0x249DE–0x24A95`, `0x25D90–0x25DE5`,
and `python3 tools/fifa96_callers.py 0x23B38` (census unchanged: 12 callers).
`captures/` and `build/` are git-ignored (derived from copyrighted content);
`game/FIFAPCCD96.iso` was never written (patched copies only). `make test`:
19/19 before and after (no code changed).

## Deliverable status

1. **Keys files: committed.** `tools/keys/fu18-friendly-walk.keys` (run 2,
   maps 4 new callers + the team-editor transition) and
   `tools/keys/fu18-friendly-cycle.keys` (run 3, the minimal driver for the
   `0x246DA`/`0x24789` prev/next redraws).
2. This document.

## Honest gaps

* Four of twelve callers unmapped (`0x24062`, `0x240C1`, `0x2488F`,
  `0x24938`); three are statically Visitor-side redraws (`0x240C1`,
  `0x2488F`, `0x24938`) and the campaign never focused the Visitor team
  column; the fourth (`0x24062`) is the Home-side selection wrapper, which
  no reached screen invoked.
* The `0x242xx`/`0x243xx` case→widget binding is runtime-only (the dispatch
  table at `0x1411C` is LE-fixup-populated), so the caller→widget rows above
  are empirically grouped by burst, not proven per widget.
* Decode loss (see above) makes every distinct-caller count a lower bound.
* Run 5's early exit is unexplained; whether `space`-cycling a specific
  club category crashes the game (as FU-16 run 3 did with `enter`) is not
  determined.
* The Visitor column's focus mechanics (which key moves the active column)
  remain unknown; run 2's `004-0050`/`004-0060` frames show a
  Visitor-highlighted state but no probe frame was emitted there, so the
  path is not yet proven.

## Visitor-side retry (2026-10-02)

Follow-up to the four unmapped callers: focus the Visitor column and drive
its value cycles. Two runs, both productive; the two Visitor cycle callers
are now mapped, leaving only the two selection wrappers, which the static
evidence places behind the modem/direct-link message queue.

### Method delta (what the FU-18 campaign missed)

Frame forensics (re-extracting `captures/session-probe-s8-2/video/fifa96_004.avi`
at 4 fps and sampling the Visitor-row highlight region at x=392,y=344) show
the Visitor column became active between wall 49.3 s and 50.0 s — at the
second `up` of run 2's walk (`up@49`), **not** at `right@59`. Run 3's
`right@60` never moved focus because its walk (spaces only) never left the
Home column. The retry therefore reuses the run-2 walk
(`down`×4 then `up`×2) as the Visitor-focus recipe, then cycles there.

Fresh frame checks confirm `left` moves focus to the Home column (run-1
`004-0065`; run-2 4 fps sampling: Visitor focused t≈80–90 s, Home focused
t≈95–120 s after `left@90`). The way back to the Visitor column was not
cleanly isolated: run-1's `right@74`/`right@84` did not visibly refocus
Visitor in the 1 fps frames (`004-0066`, `004-0076` still Home), yet the
Visitor was focused again by `004-0085`. `space` cycles the focused
column's country.

### Runs

All runs target `0x23B38`/site 8. Trace bytes from
`captures/session-probe-s8v-<N>/trace.bin`; "probe" = decoded `T_PROBE
site=8` frames; modules every run `MOD2 MOD5` (no tournament route).

| # | keys | session | trace bytes | AVIs / frames | probe | distinct callers | screens observed (frame evidence) |
|---|------|---------|------------:|--------------:|------:|------------------|----------------------------------|
| 1 | `tools/keys/fu18-visitor.keys` | `probe-s8v-1` | 91,082 | 5 / 171 | 83 | `0x24227` `0x24282` `0x242BF` `0x2431C` `0x24373` `0x25DDA` **`0x2488F`** | FRIENDLY, Visitor column focused: International/Italy (`004-0045`) → Germany/1860 München (`004-0050`) → England/Arsenal (`004-0060`) → Brazil/A Mineiro (`004-0085`); `left@69` refocused Home (`004-0065`), Visitor again by `004-0085` |
| 2 | `tools/keys/fu18-visitor-next.keys` | `probe-s8v-2` | 82,350 | 5 / 173 | 81 | `0x24227` `0x24282` `0x242BF` `0x2431C` `0x24373` `0x25DDA` `0x24789` `0x2488F` **`0x24938`** | Visitor Germany/1860 `004-0056` → International/Algeria `004-0066` (increment block); `left@90` refocused Home, spaces then cycled Home to Malaysia/Brunei (`004-0096`) |

Counts per run (decoded frames):

* s8v-1: `0x2488F`×9, `0x24373`×29, `0x242BF`×26, `0x24227`×7,
  `0x24282`×7, `0x2431C`×3, `0x25DDA`×2 (`SUMMARY end=END lost=38
  seqgaps=37`).
* s8v-2: `0x24373`×22, `0x242BF`×20, `0x24227`×13, `0x24282`×13,
  `0x2488F`×4, `0x24789`×4, `0x24938`×2, `0x2431C`×2, `0x25DDA`×1
  (`SUMMARY end=END lost=38 seqgaps=35`).

Burst attribution (no timestamps in `T_PROBE`, so callers are grouped by the
draw calls around them): Visitor cycles are preceded by `0x242BF`/`0x24373`
redraw pairs (slot-1 draw path via `FUN_00023EB8`); Home cycles by
`0x24227`/`0x24282` pairs (slot-0 path via `FUN_00023D14`). In s8v-2 the two
`0x24938` bursts sit inside `242BF/24373` groups (decoded-stream positions 22
and 27) and the four `0x24789` bursts inside `24227/24282` pair groups
(positions 41, 46, 51, 56), cross-checked against the frame-verified screens
above.

### Verdict on the four unmapped callers

| caller_link | status after this campaign |
|-------------|----------------------------|
| `0x2488F` | **Mapped (run 1)**: Visitor slot-1 country *decrement* range (static block `0x24840`, `[0x5478]--`); fires while the Visitor column is focused and cycled, 9× in `probe-s8v-1`. |
| `0x24938` | **Mapped (run 2)**: Visitor slot-1 country *increment* range (static block `0x24900`, `([0x5478]+1) % count`); fires after `right`-directed cycles on the Visitor column, 2× in `probe-s8v-2`. |
| `0x24062` | **Not keyboard-reachable (static verdict).** Its wrapper `FUN_00024044` has exactly two callers, both inside `FUN_00024a9c` (`0x24A29`, `0x24A86`; events 3/4). `FUN_00024a9c` reads the link/message queue (`FUN_0001d51c` → `FUN_0006ccca`) and is gated on `DAT_0005753c`; the binary carries `modem/direct.cfg`, `modem/modems.cfg` and `_CONNECT…` strings, placing that queue in the modem/direct-link subsystem (GAME SELECT's Modem Setup). Fires only when a remote/link peer changes the Home selection. |
| `0x240C1` | Same as `0x24062` for the Visitor side (`FUN_000240a0`, `0x24A0F`/`0x24A64`); needs the same link message path. |

The `0x24062`/`0x240C1` verdict is static and was not runtime-proven (no
modem session was started). Both wrappers also consult `FUN_0006d1e5`
(returns `DAT_0000dfb0`) to pick the side, consistent with a remote-player
selection event, and the handler copies 0x2f-byte (team record) / 9-byte
payloads out of the message.

### Honest notes

* Run-2's cycle direction was not fully reduced to a key rule: `right`
  presses preceded the two Visitor increments and `left` the Visitor
  decrements, but after `left@90` refocused Home the following `space`
  presses all used the Home *increment* block. Recorded as observed; the
  direction-state update rule is an open question.
* Decode loss again makes all distinct-caller counts lower bounds
  (`lost=38`, `seqgaps=37/35`).
* Both runs ended normally (`SUMMARY end=END`); no crash, no early exit.
* `0x24789` (already mapped in FU-18) fired in run 2; no other previously
  unmapped caller appeared.

### Provenance

```
CAPTURE_VIDEO=1 KEYS_FILE=tools/keys/fu18-visitor.keys TIMEOUT=170 \
    sh tools/trace_probe.sh 0x23B38 8 probe-s8v-1
CAPTURE_VIDEO=1 KEYS_FILE=tools/keys/fu18-visitor-next.keys TIMEOUT=172 \
    sh tools/trace_probe.sh 0x23B38 8 probe-s8v-2
python3 tools/fifa96_frames.py captures/session-probe-s8v-<N> --fps 1
python3 tools/fifa96_probe.py captures/session-probe-s8v-<N>/trace.bin \
    --target-link 0x23B38 --overwrite 5 --expect-site 8
./build/fifa96_trace captures/session-probe-s8v-<N>/trace.bin
```

Static checks used the open Ghidra MCP session on `fifa96_le.bin`:
`get_function_xrefs 0x24044`/`0x240a0` (only callers inside `FUN_00024a9c`),
`FUN_00024a9c` (`0x249DE–0x24AFC`), `FUN_0001d51c`, `FUN_0006ccca`,
`FUN_0006c74c`, `0x5753c` xrefs, and strings matching
`modem|serial|_CONNECT`. `game/FIFAPCCD96.iso` was never written (patched
copies only); `captures/` and `build/` are git-ignored; `make test`: 19/19
before and after (no code changed).

### Deliverable status (retry)

1. Keys committed: `tools/keys/fu18-visitor.keys` (run 1: Visitor focus +
   decrement cycles, fires `0x2488F`) and `tools/keys/fu18-visitor-next.keys`
   (run 2: Visitor increment cycles, fires `0x24938`).
2. This section.
