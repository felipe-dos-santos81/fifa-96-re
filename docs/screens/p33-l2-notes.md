# P33-L2 — the live record face acceptance (the driven variant; the two-segment pan)

**Task:** the phase-33 producers plan task 3
(`docs/superpowers/plans/2026-10-22-fifa96-m2-phase33-producers.md:83-98`).
**Vehicle rule:** FU325 §6 + its review erratum (`FU325_r2l7_acceptance.md:253`)
— the natural clause is **NOT-GO-as-measured** (FU323-L1) and the **driven
variant is the Layer-2 witness**, replayed per the phase-32 procedure and
re-pinned on FU323 §5's measured drive timelines (FU322's erratum 2 tables).
**Procedure:** `docs/ghidra/FU321_r2l6_nonzero_acceptance.md` §4 (the trace
cells, the same-clock pairs, G1–G8, the attribution rules) + the frozen
p32 procedure (`docs/screens/p32-l2-notes.md`, FU317 §5).
**Scope:** the captures + the trace extract (`p33-l2-trace-pan.txt`) + this
note only; no production code, no test change, no golden change.

## The natural (undriven) clause — NOT-GO-as-measured

Recorded first because it is what makes this note a *driven* acceptance. On the
undriven patched tape the band entry (entity 12, engine step 5062) passes the
row-04 gate exactly once and `fifa96_camera_event_set` performs a **silent
code-5 bind** at granted **f=1517 (tick 5060)**: `trk=12`, `tim 0/12`,
`ramp 12`, `vel=(0,0)`, `spd 0`, `pan=0` — the no-slot arm's zero seed
(`word[+0x73/+0x75] << 2` = ±4 → `v0 = 0`). No granted frame has
`pos ≠ (0,0,0)` through f=3597; the aggregate cells are `camera_zero_frames`
**1517** (the pre-bind prefix) / `camera_left_zero` **2081** (the post-bind
samples are non-all-zero via the *tracked* cell, not via pos), and the
delta-run machine records `segments 0 / changed 0 / off-zero 0`
(FU323 §2/§4, leg FU323-L1; FU325's erratum §2; the tree's `m2_natural`
comment and this session's M2 stderr agree: "first bind f 1517 id 12 …
changed 0, off-zero 0"). That is §6.1's own first fail shape, so FU325 §6's
fallback governs: the natural clause is NOT-GO-as-measured with its native
owner (the seed-switch census, `outfield.c:685-701`), and the pan claim below
is carried by the **driven** run — never worded as "the trace shows the
offset", and never silently upgraded to the natural route.

## The run

`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p33/p33-trace.log
./build/fifa96 game/FIFAPCCD96.iso` (the `make game` entry; SDL3; the client
is 960×720, `import -window` used for every capture).

Walkthrough: `windowactivate --sync` on the 960×720 client (id 56623156), then
0.2 s holds — RETURN (the intro FMV, verified on `a4-intro.png`) → BACKSPACE
(the front-end; the red selection marker, `a4-fe.png`) → RETURN (the match).
All keys — walkthrough and drive — are **global XTEST** (`xdotool keydown/keyup`
with no `--window`); see "Timing limits" 1 for why the `--window` (XSendEvent)
route failed here. The live drive is the phase-32 M2 prefix (replayed by trace
tick) + the FU320 tail (wall clock, phase-2-relative), with the DOWN release
moved to the **final** stop of the re-pinned two-segment timeline:

| anchor | re-pinned (the tree's asserts / the null tape) | live (this run) |
|---|---|---|
| phase 2 | step 1802 (probe); null trace `f=538`, `tick 1797` | **`f=539`, `tick 1800`** |
| prefix | M2_KEYS: RIGHT tick 7, KICK 14/17, LEFT 47…140, act-1 KICK 1790/1797 | tick-identical (`driver.log` tick=7/14/17/47/140/1790/1797) |
| **LEFT (drive)** | **tape 1820…1849** | phase 2 + 20 ticks (wall **+0.215 s ≈ tick 1822**), held 0.30 s (≈ tick 1852) |
| **DOWN (drive)** | **tape 1850…end** | keydown **+0.530 s ≈ tick 1853**; keyup after the seg[1] stop (tick 5277) |
| seg[0] gate | `f=1482`, tick 4944 (code 4, lane 64/−60, bound 66, `cam_dz −16`, face7d `−0x100`, `has_slot 1`, pos (60,0,16)) | **`f=1482`, tick 4944** (camera cells still parked) |
| seg[0] bind | `f=1483`, tick 4947 (code 5, vel (−5,0), spd 5, `trk=2`, tim 0/12, ramp 12) | **`f=1483`, tick 4947** (trace: `vel=−5,0 spd=5 trk=2 tim=0/12`) |
| seg[0] motion | `f=1484…1510` (27 frames, 0 → −0x90) | **`f=1484…1510`** |
| seg[0] stop | `f=1511`, tick 5040, **`(−0x90,0,0)`**, pan 4 | **`f=1511`, tick 5040** (`pos=−144,0,0 vel=0 trk=2 pan=4`) |
| gap | camera constant `(−144,0,0)`, `trk=2`, f=1512…1552 | **constant** (the f=1529 sentinel is the half-minute roll) |
| seg[1] gate | `f=1552`, tick 5177 (code 3, **lane 129 = bound 129**, `cam_dz −4`, face7d −261, **Δ=0**, pos (−16,0,4), tracked 2) | **`f=1552`, tick 5177** (camera cells parked; the tuple is Layer-1) |
| seg[1] event | `f=1553`, tick 5180 (code 3, vel (−5,0), **trk=1**, pan restarts 0) | **`f=1553`, tick 5180** (`pos=−144,0,0 vel=−5,0 spd=5 trk=1 pan=0`) |
| seg[1] motion | `f=1554…1580` (27 frames, −0x90 → −0x120) | **`f=1554…1580`** |
| seg[1] stop | `f=1581`, tick 5274, **`(−0x120,0,0)`**, pan 4 | **`f=1581`, tick 5274** (`pos=−288,0,0 vel=0 trk=1 pan=4`) |

The live and the re-pinned timelines agree **tick-for-tick** at every anchor.
The `f=` cells are run-relative granted-frame indices, not a shared clock, so
their equality follows from the matched grant pacing; the **tick** agreement is
the strong claim. The re-pinned cells are the tree's offset-pass asserts
(`tests/test_engine_m2.c`, the segment-aware re-pin block — task 2's
commit `7b3cc10`); the live cells are from the `FIFA96_CAMERA_TRACE` sink.

**The pan (trace primary), cells from the sink:**

| frame | clock | pos | vel | trk | pan | tim | reading |
|---|---|---|---|---|---|---|---|
| f=1483 | 00:31 | 0,0,0 | **−5,0** | **2** | 0 | **0/12** | the seg[0] bind: `spd 5`, `ramp 12`, pos parked |
| **f=1484** | 00:31 | **−10,0,0** | −5,0 | 2 | 0 | 2/12 | seg[0] first motion |
| f=1510 | 00:32 | **−144,0,0** | −1,0 | 2 | 3 | 12/12 | seg[0] last motion |
| **f=1511** | 00:32 | −144,0,0 | **0,0** | 2 | **4** | 0/12 | the `speed == 0` stop |
| f=1552 | 00:33 | −144,0,0 | 0,0 | 2 | 4 | 0/12 | the seg[1] gate frame (parked; the tuple is Layer-1) |
| **f=1553** | 00:33 | −144,0,0 | **−5,0** | **1** | **0** | **0/12** | the seg[1] event: the pan counter restarts 0, the tracked record re-binds 2→1 |
| **f=1554** | 00:33 | **−154,0,0** | −5,0 | 1 | 0 | 2/12 | seg[1] first motion |
| f=1580 | 00:34 | **−288,0,0** | −1,0 | 1 | 3 | 12/12 | seg[1] last motion |
| **f=1581** | 00:34 | −288,0,0 | **0,0** | 1 | **4** | 0/12 | the seg[1] `speed == 0` stop |

**Two chained delta runs, 27 changed frames each (54 total): seg[0] 0 → −0x90
(f=1484…1510), seg[1] −0x90 → −0x120 (f=1554…1580), `pan 0→4` per segment,
Y=Z=0, `trk` 2 then 1.** `follow=0` and `pos_y=0` on all **31 091** body lines
(`f=0…31 090`; 1 header + 31 091 body; `grep -c 'follow=1'` = 0). The only
other changed-place frame is the half-time roll at **f=4141** (`ph=12` at
f=4139, then `ph=1`; the camera resets to `0,0,0` with `trk=−1`, `pan=0` — one
frame, no velocity): a period reset, not a second pan. It is outside the
drive's window and is included in the extract with the post-reset sentinels
f=20000 and f=31089 so the claim is not a cherry-pick: after the reset the
camera stays parked at `0,0,0`, `trk=−1` through the kill — no later band entry
panned (ph=12 at the end of the trace).

- **G1 (trace):** exactly two delta runs inside the drive's window, one per
  segment (`f=1484…1510`, `f=1554…1580`, 27 frames each), `trk=2` across the
  first and `trk=1` across the second, `pan 0→4` per segment, `pos_y=0` and
  `follow=0` on every line.
- **G2 (arithmetic):** the observed per-segment `(|Δ|, frames) = (0x90, 27)`
  read off the trace; against the recorded seed (each segment's event
  `vel=(−5,0)` ⇒ `s = −0x40`, `v0 = −5`) it is `T(−0x40)` (FU321 §3.4,
  FU320 §4.2); the tree's `m2_pan_curve` pins both segments (`fr == 27`,
  `pl == stop_x − prev_stop`) and `Δ=0` on the seg[1] face.
- **G4 (seed):** `|v0| = 5` on both segments' event lines (`spd=5`); the
  `pan`-count trap is avoided: `pan=4` counts pan steps, not frames; the run is
  the 27 changed frames per segment.

**The offset is a Layer-1 claim (G3), not a trace cell.** The trace carries no
record/gate/face cells (its cells are `ph`, `clock`, `tick` and the camera
cells; `match_run.c:4591-4607`); it reproduces the **drive and the arithmetic**
and **cannot show the offset** (FU321-L1). The offsets are cited from the
tree's re-pinned asserts — the same session's `test_engine_m2` stderr prints
them verbatim: seg[0] gate record 2 `code 4`, lane 64/−60, bound 66,
`cam_dz −16`, face7d `0x100`, `has_slot 1`, pos (60,0,16); bind f=1483 `code 5`
trk 2; seg[1] gate `code 3` active 2, **`pos (−16,0,4)`, `lane 129`,
`bound 129`, `cam_dx −128`, `cam_dz −4`, `face7d 0xFFFFFEFB`, bearing
`0xFFFFFEFB`, `delta 0x0`** (the live face has settled onto the bearing),
`has_slot 0`, `slot_entity 1`, `cam_pos (−144,0,0)`; event f=1553 `code 3`
trk 1. The live pan's windows identify the drive (gate `f=1482`/`f=1552`,
events `f=1483`/`f=1553`); the Layer-1 tuples identify the offsets.

## The captures and the AE

The pairs are `import -window` grabs (the FU317 §5.3 primary route), from the
same run as the trace, each grab logged with the trace's PRE/POST line
(`capture.log`); the displayed clock is checked on the HUD crop. The watcher
bursts back-to-back pairs from the first motion to the final stop and anchors
every pair to a named segment's window — **never a mixed span**: the committed
pan grabs are two pairs, one per segment. The committed files are
`pan-00a` → `p33-l2-pan-a.png`, `pan-02b` → `p33-l2-pan-b.png` (the **seg[0]**
pair), `pan-16a` → `p33-l2-pan-c.png`, `pan-19b` → `p33-l2-pan-d.png` (the
**seg[1]** pair), `still-s2-a` → `p33-l2-still-a.png`, `still-s2-b` →
`p33-l2-still-b.png` (the raw names are the watcher's; the `p33-l2-*` names are
the committed ones; each committed PNG is the `-depth 8` conversion of its
16-bit grab — pixel-identical to it, AE 0).

| pair | files | clock | trace bounds (PRE → POST) | AE (`960x591+0+0`) | full-frame AE | HUD clock crop AE (`140x66+0+650`) | bbox |
|---|---|---|---|---|---|---|---|
| pan seg[0] | `p33-l2-pan-a.png`, `p33-l2-pan-b.png` | **00:31** | f=1484→1485 and f=1493→1495 | **52 466** | 52 466 | **0** | **272×391+329+0** (`p33-l2-pan-diff.png`) |
| pan seg[1] | `p33-l2-pan-c.png`, `p33-l2-pan-d.png` | **00:34** | f=1559→1561 and f=1575→1577 | **69 828** | 69 828 | **0** | **592×406+0+0** (`p33-l2-seg1-diff.png`) |
| still | `p33-l2-still-a.png`, `p33-l2-still-b.png` | **00:38** | f=1679→1680 and f=1683→1685 | **29 120** | 29 120 | **0** | **200×200+416+347** (the ball sprite only) |

- Both pan pairs: each grab lies **inside its named segment's delta run**
  (seg[0]: f=1484…1510, 0.33 s apart; seg[1]: f=1554…1580, 0.52 s apart), and
  each pair sits on one displayed second (00:31 / 00:34; the clock crop AE 0
  verifies it). The trace attributes the change to the camera translation: the
  camera place moves −10 → −96 across the seg[0] grabs and −204 → −282 across
  the seg[1] grabs. The sprite **pose** also differs between the two grabs of
  each pair (the sprite animation advances), so the AE alone is not an
  animation discriminator — the trace is the attribution.
- The still pair: same run, camera constant at `−288`, **no key held** (the
  DOWN keyup followed the `f=1581` stop, tick 5277), the `s2` pair ~2.4 s after
  the stop (stop tick 5274 / 00:34; the grabs f=1679…1685 at tick
  5600…5620 / 00:38), 0.15 s apart, both 00:38.
- **Unlike p32's still, this pair is NOT pixel-identical: AE 29 120.** The
  still scene carries a **ball sprite in frame**, and the ball animates between
  the two grabs; the diff bbox is confined to the ball (`200×200+416+347` — the
  outfield sprite is pixel-identical between the grabs). The camera-constant
  property is carried by the trace cells (all four PRE/POST lines read
  `pos=−288,0,0 vel=0,0 spd=0 trk=1 pan=4`), and the two sibling still controls
  agree: `s1` (1 s after the stop) AE 61 195 (the player sprite still settling,
  bbox 449×490+62+0), `s3` (3 s after) AE 29 591 — the ball never settles; the
  player does. The committed still pair is `s2` because it isolates the
  residual to the ball sprite alone.
- **Gate 1 (relative): AE_pan 52 466 / 69 828 > AE_still 29 120**, all measured
  in the same session/build on the same files. The gap is carried by the
  sprite's screen position shift (the camera translation), not by the pose or
  ball animation the still pair already contains. The AE is the relative
  corroboration, **not** the attribution.
- **Gate 2 (whole-scene bbox) FAILS and cannot pass in this build:** every
  diff is sprite-limited (the largest, seg[1]'s 592×406+0+0, is still just the
  sprite's two-position sweep plus the ball's exit dot at the left edge; the
  seg[0] diff is 272×391+329+0), because the renderer draws sprites on black
  (no pitch/background). `p33-l2-pan-diff.png` and `p33-l2-seg1-diff.png` are
  the diff artefacts; this is the FU313-L4/FU305-L6 caveat, never an
  attribution.

## Verdict

**The driven pan reproduced live under the live face — YES, trace-attributed,
both segments, tick-for-tick against the re-pinned timeline.** The camera place
moves `0 → −0x90` over 27 granted frames (stop `f=1511`, tick 5040) and then
`−0x90 → −0x120` over 27 more (stop `f=1581`, tick 5274), `pan 0→4` per
segment, `trk=2` then `trk=1`, and each committed pair lies inside its named
segment's delta run on one displayed second. The drive is FU320's nominated
tape (`LEFT 1820..1849`, `DOWN 1850..end`) replayed at phase 2 + 20 ticks with
the DOWN release held past the final stop; its **offset tuples are cited from
the tree's re-pinned asserts** (`tests/test_engine_m2.c`, the offset pass:
seg[1] gate `pos (−16,0,4)`, `lane 129`, `Δ 0x0`, `face7d −261`; seg[0] bind
`code 5` trk 2 at f=1483) — **not** read from the trace. The pan is the same
`T(s)` family as phases 31/32 (the seed is the event's `vel=(−5,0)`, not the
bearing — FU320-L5). **Follow/Y stays WAIVED** (`follow=0`, `pos_y=0` on every
line); the armer/OOB stays unreachable — **`oob_frames = 0`, `goal_armed_any =
0`** in the tree's offset pass, `|x| = 0x120 < 0x6C0 < 0x730`; the trace has no
armer cell, so the bound and the re-pinned asserts carry it, and the camera's
`y`/`z` are 0 through both pans. Gate 2 remains the documented sprite-only
caveat.

**The natural (undriven) clause stays NOT-GO-as-measured** (the silent code-5
bind, FU323-L1; the numbers above) — the natural route remains the open leg
with its native owner (the seed-switch census, `outfield.c:685-701`); this
note's verdict is worded **"the driven pan reproduced live under the live
face"** and is never a claim about the undriven tape.

## Timing limits found (FU321-L4 / FU325-L2)

1. **The `--window` (XSendEvent) key route failed this session — the keyups
   were not seen by the engine's key model.** Attempts 1–3 stalled the kickoff
   cutscene before phase 2 (`ph=1`, clock 00:00, `pos=(0,0,0)` for 120+ s)
   under the p32 walkthrough replayed via `xdotool key --window`. Attempt 3's
   prefix replay was tick-identical to p32's (`tick=7/14/17/47/140/1790/1797`)
   and its camera cells matched p32's acceptance trace line-for-line through
   f=538 (tick 1797) — then p32's run wrote phase 2 at f=539 and attempt 3 did
   not. The divergence is the act-1 KICK's **release arm**
   (`match_handlers.c:263`: `slot.entity == id && (slot.released & 0x70) != 0`):
   the release word is recomputed per frame from the polled input, so a keyup
   the engine's key model never observes leaves the release armed forever. A
   live diagnostic (a manual global XTEST `z` press/release at ~16.6 min)
   completed the cutscene immediately — the scratch diagnostic's drive never
   ran at the tape's window and is not committed. **The acceptance run used
   global XTEST everywhere** (the p28/p31 route) and phase 2 landed at
   `f=539`, tick 1800 — the p32 acceptance run's own cells. This is the
   FU321-L4/FU325-L2 class honestly recorded: the route is session/build
   dependent (it worked in the p31/p32 runs), so the **global XTEST route is
   the required arm for any future live replay**.
2. **The live timing must be scheduled on the wall clock, not on trace-line
   arrival** (the p32 finding stands): the drive runs at phase 2 + 0.215 s /
   +0.530 s measured (the script's +20-tick anchor = the tape's 1820; the wall
   clock measured ≈ tick 1822 — the same ≈+22-tick offset as p32).
3. **The phase-2 landing drifts between live runs** (tick 1800 here; p32's
   1800; the phase-31 T3 run's 1897; the null probe step 1802 / tick 1797).
   The drive is phase-2-relative, and the gate/event/motion/stop then land on
   the re-pinned ticks (4944/4947/4950…5037/5040, 5177/5180/5184…5270/5274)
   because the walk timeline is absolute in ticks (the T3 finding).
4. **The DOWN release must wait for the FINAL stop, not the first** (the
   two-segment adaptation): releasing after seg[0]'s stop (`f=1511`, tick
   5040) still lets the seg[1] code-3 event fire, but the run's keyup is
   recorded after `f=1581` (tick 5277) so the release order matches the tape's
   1850…end window.
5. **The pans are 27 frames ≈ 0.9 s each** at 30 grants/s (seg[0] displayed
   00:31…00:32, seg[1] 00:33…00:34); each pair must fit inside one displayed
   second. The accepted pairs are 0.33 s (seg[0]) and 0.52 s (seg[1]) apart.
6. **The still scene does not settle to pixel-identity in this run** (the ball
   sprite animates; AE 29 120 on the committed pair) — the still control is a
   same-second, no-key, camera-constant pair recorded **as measured**, with
   the residual attributed by the trace cells and the ball-limited bbox, not
   tuned away.
7. **No absolute AE floor** (FU309-L6): Gate 1 is relative, and the AE's
   attribution is the trace's.

## Invariants

No code/test/golden change. M1 `09b726b7…` (22 004 B) and M2 `d5446144…`
(295 087 B) unchanged; `./build/test_engine_m1 | cmp -
tests/golden/engine/m1-frames.txt` and `./build/test_engine_m2 | cmp -
tests/golden/engine/m2-frames.txt` are identical (re-run at the close of this
task, both OK). The trace sink stayed default-off outside this run; the ISO
was only read. The census stays `60/80 = 75%`.

## Commands (exact)

```sh
# the run (a4-run.sh: the walkthrough taps + the driver + the watcher)
DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p33/p33-trace.log \
  ./build/fifa96 game/FIFAPCCD96.iso          # Makefile's `make game` entry
# walkthrough: windowactivate --sync $WID, then 0.2 s holds
#   Return (intro) / BackSpace (front-end) / Return (match)
# the driver: the M2 prefix by trace tick, the FU320 tail by wall clock,
#   the DOWN release after the FINAL stop (p33-run.py)
python3 /tmp/opencode/p33/p33-run.py "$WID" /tmp/opencode/p33/p33-trace.log \
  /tmp/opencode/p33/driver.log
# the captures (same run, burst pairs from the first motion to the final
#   stop + the s1/s2/s3 stills; p33-capture.py)
python3 /tmp/opencode/p33/p33-capture.py "$WID" /tmp/opencode/p33/p33-trace.log \
  /tmp/opencode/p33
# the depth-8 conversion of each committed grab (16-bit -> 8-bit, AE 0)
convert <raw>.png -depth 8 docs/screens/p33-l2-<name>.png
# the crops, the AE, the diffs, the bboxes
for p in pan-a pan-b pan-c pan-d still-a still-b; do
  convert docs/screens/p33-l2-$p.png -crop 960x591+0+0 +repage /tmp/c-$p.png
  convert docs/screens/p33-l2-$p.png -crop 140x66+0+650 +repage /tmp/clock-$p.png
done
compare -metric AE /tmp/c-pan-a.png /tmp/c-pan-b.png null: 2>&1     # 52466
compare -metric AE /tmp/c-pan-c.png /tmp/c-pan-d.png null: 2>&1     # 69828
compare -metric AE /tmp/c-still-a.png /tmp/c-still-b.png null: 2>&1 # 29120
compare -metric AE /tmp/clock-pan-a.png /tmp/clock-pan-b.png null: 2>&1   # 0
compare -metric AE /tmp/clock-pan-c.png /tmp/clock-pan-d.png null: 2>&1   # 0
compare -metric AE /tmp/clock-still-a.png /tmp/clock-still-b.png null: 2>&1 # 0
compare -compose src /tmp/c-pan-a.png /tmp/c-pan-b.png docs/screens/p33-l2-pan-diff.png
compare -compose src /tmp/c-pan-c.png /tmp/c-pan-d.png docs/screens/p33-l2-seg1-diff.png
convert docs/screens/p33-l2-pan-diff.png  -format '%@' info:   # 272x391+329+0
convert docs/screens/p33-l2-seg1-diff.png -format '%@' info:   # 592x406+0+0
```

The committed PNGs are the 8-bit conversions (`-depth 8`) of the 16-bit
`import` grabs; the window's source pixels are 8-bit, and the AE is unchanged
by the conversion (52 466 / 69 828 / 29 120 measured on the committed files
themselves; each conversion is pixel-identical to its grab, AE 0).

## The extract

`docs/screens/p33-l2-trace-pan.txt` (120 lines: 4 comment lines + 1 trace
header + 115 selected body lines) carries the phase-2 line `f=539` (tick
1800), seg[0]'s window `f=1478…1518` (gate/bind/motion/stop), the gap sentinel
`f=1529` and the gate `f=1550…1552`, seg[1]'s window `f=1553…1588`
(event/motion/stop), the still windows `f=1610…1620` (s1) and `f=1676…1688`
(s2, the committed still pair), the half-time reset `f=4137…4143`, and the
post-reset sentinels `f=20000` and `f=31089`, from the full trace
`/tmp/opencode/p33/p33-trace.log` (1 header + 31 091 body lines, sha256
`3da0c9b09cb331deb2dd74f1dc51503dbe2d245131fd7b744e29030fac83eef9`); every
extract body line is verbatim from the full trace.
