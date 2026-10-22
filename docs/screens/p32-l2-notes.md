# P32-L2 — the non-zero-delta pan acceptance (a live pan under the FU320 drive)

**Task:** M2 phase-32 Stage-2 task 2
(`docs/superpowers/plans/2026-10-22-fifa96-m2-phase32-producers.md:60-75`).
**Procedure:** `docs/ghidra/FU321_r2l6_nonzero_acceptance.md` §4 (the trace cells,
the same-clock pairs, G1–G8, the attribution rules) + the frozen phase-31 T3
procedure (`docs/screens/p31-l2-notes.md:10-61`, FU317 §5).
**Scope:** the captures + the trace extract (`p32-l2-trace-pan.txt`) + this
note only; no production code, no test change, no golden change.

## The run

`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p32/trace-run2.log make game`
(`./build/fifa96 game/FIFAPCCD96.iso`, SDL3; the client is 960×720, `import
-window` used for every capture).

Walkthrough: intro RETURN → front-end BACKSPACE → RETURN (match). The live drive
is the phase-31 M2 prefix (by trace tick) + the FU320 tail (by wall clock, since
the trace's `tick` can arrive in bursts — §"Timing limits" 1):

| anchor | null (FU320 / the tape) | live (this run) |
|---|---|---|
| phase 2 | step 1802 (probe); null trace `f=538`, `tick 1797` | **`f=539`, `tick 1800`** (wall anchor) |
| RIGHT | tape 6…44 | tick 7 down |
| KICK (opening) | tape 12…13 | tick 14 down / 17 up |
| LEFT (prefix) | tape 45…139 | tick 47 down / 140 up |
| KICK (act-1) | tape 1794…1798 | tick 1790 down / 1797 up |
| **LEFT (drive)** | **tape 1820…1849** | **phase 2 + 20 ticks** (wall +0.215 s, ≈tick 1822), held 0.313 s (≈tick 1852) |
| **DOWN (drive)** | **tape 1850…end** | keydown at +0.528 s (≈tick 1853); keyup after the stop (`f=1557`, ≈tick 5200) |
| gate | `f=1528`, tick 5097 | **`f=1528`, tick 5097** |
| event | `f=1529`, tick 5100 | **`f=1529`, tick 5100** |
| motion | `f=1530…1556`, tick 5104…5190 | **`f=1530…1556`, tick 5104…5190** |
| stop | `f=1557`, tick 5194 | **`f=1557`, tick 5194** |

The live and the null traces agree **tick-for-tick** at the gate/event/motion/stop
(the only differences are the phase-2 landing — live tick 1800 vs the null
trace's 1797 / the probe's step 1802 — and the clock cell at the gate frame:
live 00:32, null 00:33). The `f=` cells are **run-relative** granted-frame
indices, not a shared clock, so their equality follows from the matched grant
pacing; the **tick** agreement is the strong claim. The null cells are from
FU320 §4.2 and its scratch trace `/tmp/opencode/fu320/l30.trace` (1 header + 3
598 body lines). The wall-derived drive ticks are ≈1822…1852 against the tape's
1820…1849 (DOWN ≈1853 against 1850) — ~2 ticks after the script's **+20-tick
anchor** (phase 2 tick 1800 + 20 = 1820, the tape head; the probe-relative +18
is the same absolute point).

**The pan (trace primary), cells from the `FIFA96_CAMERA_TRACE` sink:**

| frame | clock | pos | vel | trk | pan | tim | reading |
|---|---|---|---|---|---|---|---|
| f=1528 | 00:32 | 0,0,0 | 0,0 | −1 | 0 | 0/0 | the parked camera; the gate frame (the row-04 tuple is Layer-1 — G3) |
| **f=1529** | 00:33 | 0,0,0 | **−5,0** | **2** | 0 | **0/12** | the event set: `trk=2`, `spd=5`, `ramp=12`, `ev=0x0` (the height-0 arm) |
| **f=1530** | 00:33 | **−10,0,0** | −5,0 | 2 | 0 | 2/12 | first motion |
| f=1543 | 00:33 | −106,0,0 | −2,0 | 2 | 2 | 0/12 | the second decay |
| f=1556 | 00:33 | **−144,0,0** | −1,0 | 2 | 3 | 12/12 | last motion |
| **f=1557** | 00:33 | −144,0,0 | **0,0** | 2 | **4** | 0/12 | the `speed == 0` stop |

**27 changed frames (f=1530…1556), place `0 → −0x90`, `pan 0→4`, Y=Z=0.**
`trk=2` from the event through the run. `follow=0` and `pos_y=0` on all **15 043**
body lines (`f=0…15 042`; 1 921 636 B; `grep -c 'follow=1'` = 0). The only other
changed-place frame is the half-time roll at **f=4141** (`ph=12` at f=4139, then
`ph=1`; the camera is reset to `0,0,0` with `trk=-1`, `pan=0` — one frame, no
velocity): a period reset, not a second pan. It is outside the drive's window
and is included in the extract so the claim is not a cherry-pick.

- **G1 (trace):** exactly one delta run inside the pan window (`f=1530…1556`,
  27 frames), `trk=2` at the event and across it, `pan 0→4`, `pos_y=0` and
  `follow=0` on every line.
- **G2 (arithmetic):** the observed `(|Δ|, frames) = (0x90, 27)` read off the
  trace; against the recorded seed (slot dir `(−1,0)` ⇒ `s = −0x40`,
  `v0 = −5`) it is `T(−0x40)` (FU321 §3.4, FU320 §4.2).
- **G4 (seed):** `|v0| = 5` — the event line's `vel=(−5,0)`, `spd=5`. The
  `pan`-count trap is avoided: `pan=4` counts pan steps, not frames; the run is
  the 27 changed frames.

**The offset is a Layer-1 claim (G3), not a trace cell.** At the passing
dispatch of this same drive Task 1's witness `run_natural_offset_pass`
(`6fb8062`) pins the FU320/T1 tuple: record 2, `code 4`,
`active 2`, **`pos (−32,0,16)`**, **`lane 36`**, `bound 36`, `cam_dx +32`,
`cam_dz −16`, `face7d 0x100`, **bearing `0x14C`**, **Δ `0x4C`**, `timer81 0`,
`has_slot 1`, `anim 0`, `dir (−1,0)`, `tracked −1`, `controlled 1`,
`slot.entity 2`, camera `(0,0,0)`. The trace carries no record/gate/face cells
(its other cells are `ph`, `clock` and `tick`; `match_run.c:4591-4607`); it
reproduces the **drive and the arithmetic** and **cannot show the offset**
(FU321-L1). The live pan's window is the offset
drive's (gate `f=1528`, event `f=1529`, motion `f=1530…1556`, stop `f=1557`);
the T1 zero-delta drive's window is `f=1512…1541` — the live window identifies
the drive, the Layer-1 tuple identifies the offset.

## The captures and the AE

The pairs are `import -window` grabs (the FU317 §5.3 primary route), from the
same run as the trace, each grab logged with the trace's PRE/POST line; the
displayed clock is checked on the HUD crop. The committed files are
`pan-01a` → `p32-l2-pan-a.png`, `pan-02a` → `p32-l2-pan-b.png`,
`still-s2-a` → `p32-l2-still-a.png`, `still-s2-b` → `p32-l2-still-b.png`
(the `pan-01a`/`pan-02a`/`still-s2-*` names are the capture watcher's; the
`p32-l2-*` names are the committed ones).

| pair | files | clock | trace bounds (PRE → POST) | AE (`960x591+0+0`) | full-frame AE | HUD clock crop AE (`140x66+0+650`) | bbox |
|---|---|---|---|---|---|---|---|
| pan | `p32-l2-pan-a.png`, `p32-l2-pan-b.png` | **00:33** | f=1537→1538 and f=1543→1545 | **63 720** | 63 720 | **0** | **335×397+281+0** |
| still | `p32-l2-still-a.png`, `p32-l2-still-b.png` | **00:38** | f=1695→1697 and f=1700→1702 | **0** | 0 | **0** | — (no diff) |

- The pan pair: both grabs lie **inside the delta run** f=1530…1556, 0.215 s
  apart, both on the displayed second **00:33**. Both frames show the same
  player sprite **identity** (record 2); its **pose differs** between the two
  grabs (the sprite animation advances), so the AE alone is not an animation
  discriminator — the **trace attributes the change to the camera
  translation**: the camera place moves from `−72…−78` (grab a) to `−106…−114`
  (grab b), and the sprite's screen position and apparent size shift with it.
- The still pair: same run, camera constant at `−144`, **no key held** (the DOWN
  keyup followed the `f=1557` stop, ≈tick 5200), the `s2` pair **~4.6 s after
  the stop** (stop tick 5194 / 00:33; the grabs f=1695…1702 at tick
  5654…5677 / 00:38), 0.158 s apart, both **00:38**.
- **Gate 1 (relative, binding): AE_pan 63 720 > AE_still 0**, both measured in
  the same session/build. The still scene had settled — the port renders
  sprites on black and after the pan the view is a single idle player sprite —
  so the still pair is pixel-identical; the two committed PNGs are in fact
  **byte-identical** (sha256 `65099f0c…`), two distinct grabs of a static
  framebuffer. The clock crop AE 0 verifies the displayed second on both pairs.
  The AE is the relative corroboration, **not** the attribution.
- **Gate 2 (whole-scene bbox) FAILS and cannot pass in this build:** the diff is
  sprite-limited (`335×397+281+0` vs the proposed `864×443`), because the
  renderer draws sprites on black (no pitch/background). `p32-l2-pan-diff.png`
  is the diff artefact; this is the FU313-L4/FU305-L6 caveat, never an
  attribution.

## Verdict

**A pan reproduced live under the non-zero-delta drive — YES, trace-attributed.**
The camera place moves `0 → −0x90` over 27 granted frames, `pan 0→4`, `trk=2`,
and the same-run pair at displayed **00:33** lies inside that delta run. The
drive is FU320's nominated tape (`LEFT 1820..1849`, `DOWN 1850..end`) replayed
at phase 2 + 20 ticks; its **offset tuple is cited from Task 1's Layer-1 run**
(`run_natural_offset_pass`, `6fb8062`: record 2 at `(−32,0,16)`, `lane 36`,
Δ`0x4C`, `has_slot 1`, `dir (−1,0)`) — **not** read from the trace. The pan is
the slot-arm class, `−0x90`/27 frames (the same `T(s)` family as phase 31;
FU320-L5: the seed is the slot dir, not the bearing). **Follow/Y stays WAIVED**
(`follow=0`, `pos_y=0` on every line); the armer/OOB stays unreachable —
**`goal_armed = 0` on every frame and `oob_frames = 0`** in the T1 witness
(`armer 0/OOB 0` on its stderr) — `|x| = 0x90 < 0x6C0 < 0x730`; the trace has
no armer cell, so the bound and the T1 pins carry it, and the camera's `z` is 0
on the pan. Gate 2 remains the documented sprite-only caveat.

## Timing limits found (FU321-L4)

1. **The live timing must be scheduled on the wall clock, not on trace-line
   arrival.** A first attempt (same drive, same session) ran with a 60 fps
   lossless x11grab clip encoding during the walkthrough. Under that load the
   engine/trace ran in a catch-up burst and the trace-line driver compressed the
   drive: `keydown LEFT` and `keyup LEFT/keydown DOWN` are 31 ms apart in wall
   time (`/tmp/opencode/p32/driver-run1.log`: 1792699205.073 / 1792699205.104,
   trace ticks 1887 / 1917). The LEFT hold was ~3 ticks, the z nudge never
   landed, and that run reproduced the **T1 zero-delta** timeline instead
   (gate `f=1512`/tick 5044, event `f=1513`, motion `f=1514…1540`, stop
   `f=1541`, clock 00:31…00:32; `/tmp/opencode/p32/trace-run1-zero-delta.log`,
   scratch, not committed; its clip `/tmp/opencode/p32/p32-live.mkv` is unused).
   The acceptance run removed the encoder and scheduled the drive from the
   phase-2 anchor on the wall clock (phase 2 + 0.20 s, held 0.30 s).
2. **The phase-2 landing drifts between live runs:** this run tick **1800**
   (`f=539`); the phase-31 T3 run tick **1897**; the null probe step **1802**
   (null trace `f=538`/tick 1797). The drive is therefore **phase-2-relative**,
   and the gate/event/motion/stop then land on the null's ticks
   (5097/5100/5104…5190/5194) because the walk timeline is absolute in ticks
   (the T3 finding).
3. **The LEFT window sets the `z` target** (FU320-L2): 30 ticks ≈ 0.30 s at
   100 Hz; 31 ms is `z ≈ 0` (the failed attempt). The script's anchor is the
   **+20-tick convention** (phase 2 + 20 ticks); the wall clock measured the
   actual presses at **≈+22 ticks** (≈1822…1852 against the tape's 1820…1849 —
   the same 2-tick offset), and the gate landed at the FU320 tuple's frame
   (`f=1528`).
4. **The pan is 27 frames ≈ 0.9 s** at 30 grants/s (displayed **00:33**); the
   pair must fit inside one displayed second. The accepted pair is 0.215 s
   apart (f≈1537–1538 / f≈1543–1545).
5. **The still scene settles.** After the pan the rendered view is a single
   idle player sprite; the post-pan pairs are pixel-identical and the two
   committed still PNGs are **byte-identical** (sha256 `65099f0c…`) — two
   distinct grabs of a static framebuffer (AE 0, clock crop AE 0). The still
   control is a same-second, no-key, camera-constant pair, recorded as
   measured — not tuned.
6. **No absolute AE floor** (FU309-L6): Gate 1 is relative, and the AE's
   attribution is the trace's.

## Invariants

No code/test/golden change. M1 `09b726b7…` (22 004 B) and M2 `8434cfab…`
(295 087 B) unchanged; `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt`
and `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` are
identical; `make check` 111/111 (the plan's Task 2 Step 7 gate; the committed
close-out register `docs/ENGINE.md` §M2 phase-32 records the same count). The
trace sink stayed default-off outside this run. The census stays `60/80 = 75%`.

## Commands (exact)

```sh
# the run
DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p32/trace-run2.log \
  ./build/fifa96 game/FIFAPCCD96.iso          # Makefile's `make game` entry
# walkthrough: Return (intro) / BackSpace (front-end) / Return (match)
# the driver: the M2 prefix by trace tick, the FU320 tail by wall clock
python3 /tmp/opencode/p32/p32-run.py      "$WID" /tmp/opencode/p32/trace-run2.log \
  /tmp/opencode/p32/driver-run2.log
# the captures (same process, trace-bounded, 10 pairs + the still pairs)
python3 /tmp/opencode/p32/p32-capture2.py "$WID" /tmp/opencode/p32/trace-run2.log \
  /tmp/opencode/p32
# the crops, the AE, the diff, the bbox
for p in pan-a pan-b still-a still-b; do
  convert docs/screens/p32-l2-$p.png -crop 960x591+0+0 +repage /tmp/opencode/p32/c-$p.png
  convert docs/screens/p32-l2-$p.png -crop 140x66+0+650 +repage /tmp/opencode/p32/clock-$p.png
done
compare -metric AE /tmp/opencode/p32/c-pan-a.png /tmp/opencode/p32/c-pan-b.png null: 2>&1     # 63720
compare -metric AE /tmp/opencode/p32/c-still-a.png /tmp/opencode/p32/c-still-b.png null: 2>&1 # 0
compare -metric AE /tmp/opencode/p32/clock-pan-a.png /tmp/opencode/p32/clock-pan-b.png null: 2>&1   # 0
compare -compose src /tmp/opencode/p32/c-pan-a.png /tmp/opencode/p32/c-pan-b.png \
  docs/screens/p32-l2-pan-diff.png
convert docs/screens/p32-l2-pan-diff.png -format '%@' info:   # 335x397+281+0
```

The committed PNGs are the 8-bit conversion (`-depth 8`) of the 16-bit `import`
grabs; the window's source pixels are 8-bit, and the AE is unchanged by the
conversion (63 720 / 0 on both).

## The extract

`docs/screens/p32-l2-trace-pan.txt` (80 lines: 4 comment lines + 1 trace header
+ 75 selected body lines) carries the phase-2 line `f=539`, the gate/event/pan
window `f=1520…1562`, the still window `f=1685…1710`, and the half-time reset
`f=4138…4142`, from the full trace
`/tmp/opencode/p32/trace-run2.log` (1 header + 15 043 body lines,
sha256 `aa8875703c4b912c1f62413a360450476095309267709ebcb8e6763d9fa5812f`).
