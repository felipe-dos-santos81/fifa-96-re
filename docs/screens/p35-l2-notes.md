# P35-L2 — the natural no-slot pan live under the placement-tier producer (the undriven tape)

**Task:** the phase-35 producers plan task 3
(`docs/superpowers/plans/2026-10-23-fifa96-m2-phase35-producers.md`, the task-3
brief). **Vehicle rule:** FU333 §7 — the Layer-2 vehicle is the **undriven live
run** (the `M2_KEYS` prefix by trace tick, nothing added after phase 2), the
trace is primary, the tuple is **producer-conditioned** (the value is the
port-reachable staging, not a native per-record measurement), and the AE/bbox
are the relative corroboration only (W-a/W-b). **Procedure:** FU321 §4 (the
trace cells, the same-clock pairs, G1–G8, the attribution rules) + the frozen
p32/p33 procedure (`docs/screens/p32-l2-notes.md`,
`docs/screens/p33-l2-notes.md`). **Scope:** the captures + the trace extract
(`p35-l2-trace-pan.txt`) + this note only; no production code, no test change,
no golden change.

## The claim and the producer condition

The mechanism under test is the phase-35 placement-tier producer: `+0x79` is
produced at the record-creation seam from the native `0x10F394` table (FU330
§2/§3, FU331), the placement copies it into `+0x7B`
(`fifa96_match_entities.c:65/148`, native `0x79A4D`/`0x7DA9B`), the mover's
stride nibble rises (`fifa96_match_run.c:549`), and the no-slot band record's
row-04 camera event fires with a live seed instead of the FU323-L1 zero-seed
bind. The tree at this run is HEAD `673a10d` (the producer `749cc2f` + the
natural witness `673a10d`); the tree's own natural pass prints the tuple this
note reproduces (same session, `./build/test_engine_m2` stderr):
**gate f 703 (step 2352), event f 704 (step 2355), motion f 705…717, stop f 718
(step 2402), 13 changed frames, place `0 → (−38,0,0)`, `pan 0→2`, `trk 12`,
seed `(−0x18,0)`, `v0 (−2,0)`, `T (−0x26,0)`**.

The **value** is the producer-conditioned part: the port's produced limit is
**6** on all 22 records — `table[0]` with the staged `player_d = 0`, and the
native `+0x9D` OR term is **0 by construction** in the port (the record has no
`+0x9D` cell; FU330-L8/FU332 erratum 2), not a measured native per-record
value (FU330-L1 = FU328-L3). The native roster index is unmeasured, and the
P2/P3 `+0x79` re-writes (the lineup rebind, the row-12 no-replacement arm) are
unported (FU332 erratum 3); the measured tuple therefore describes the
**producer-conditioned** witness, not a native-roster measurement. Every row
value in this note is this run's own trace cell, never a native claim.

## The run

`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p35/p35-trace.log make game`
(SDL3; the client is 960×720; `import -window` for every committed capture;
global XTEST for every key).

Walkthrough: `windowactivate --sync` on the 960×720 client, then 0.2 s holds —
RETURN (the intro FMV) → BACKSPACE (the front-end; the red selection marker) →
RETURN (the match). Then the **`M2_KEYS` prefix replayed by trace tick** —
RIGHT 7, KICK 14/17, LEFT 47…140, act-1 KICK 1790/1797 — and
**nothing added after phase 2**: no LEFT/DOWN hold, no second press, no key
held at any grab. This is the natural tape (`M2_KEYS` verbatim: its last entry
is the act-1 KICK release at tape 1799); the p32/p33 **drive** is deliberately
absent.

| anchor | tree (null tape / the Task-2 witness) | live (this run) |
|---|---|---|
| phase 2 | `f=538`, step 1802 (probe) | **`f=539`, tick 1800** |
| gate | `f=703`, step 2352 (`code 4`, `lane 56/−56`, `bound 68`, `cam_dz6f 0`, `has_slot 0`, `vel73 −6`, nibble 6) | **`f=703`, tick 2347** (camera cells parked; the tuple is Layer-1) |
| event | `f=704`, step 2355 (`code 5`, `trk 12`, `vel (−2,0)`, `spd 2`, `tim 0/12`, `ramp 12`, `ev 0`) | **`f=704`, tick 2350** (`pos=0,0,0 vel=−2,0 spd=2 fspd=0 ev=0x0 trk=12 pan=0 tim=0/12 ramp=12`) |
| motion | `f=705…717` (13 frames, `0 → −38`) | **`f=705…717`** (13 changed-place frames) |
| stop | `f=718`, step 2402, `(−38,0,0)`, `pan 2` | **`f=718`, tick 2397** (`pos=−38,0,0 vel=0,0 spd=0 trk=12 pan=2`) |
| place / pan | `0 → (−38,0,0)`; `pan 0→2` | **`0 → (−38,0,0)`; `pan 0→2`** |
| `pos_y` / `follow` | 0 / 0 | **0 / 0** on all **4 167** body lines (`grep -c 'follow=1'` = 0; `pos_y`/`pos_z` non-zero = 0) |

The live and the tree's tuple agree **frame-for-frame** at every anchor; the
**tick** cells are a **constant −5** against the null tape's step column
(2347/2350/2354/2397 vs 2352/2355/2359/2402) — a pacing offset of the live run,
not a per-anchor drift (the p33 finding: the `f=` cells are run-relative
granted frames, the tick is the 100 Hz clock; the **frame** agreement is the
strong claim here, and the offset is constant). The phase-2 landing is the
same drift class as p32/p33 (live 1800 vs the null trace's 1797 / the probe's
1802).

**The pan (trace primary), cells from the `FIFA96_CAMERA_TRACE` sink:**

| frame | clock | pos | vel | spd | trk | pan | tim | reading |
|---|---|---|---|---|---|---|---|---|
| f=703 | 00:05 | 0,0,0 | 0,0 | 0 | −1 | 0 | 0/0 | the gate frame; the row-04 tuple is Layer-1 (G3) |
| **f=704** | 00:05 | 0,0,0 | **−2,0** | **2** | **12** | 0 | **0/12** | the event: the no-slot bind, `ramp 12`, `ev 0x0` (the height-0 arm) |
| **f=705** | 00:05 | **−4,0,0** | −2,0 | 2 | 12 | 0 | 2/12 | first motion |
| f=710 | 00:05 | **−24,0,0** | −2,0 | 2 | 12 | 0 | 12/12 | the last `spd 2` frame |
| f=711 | 00:05 | **−26,0,0** | −1,0 | 1 | 12 | 1 | 0/12 | the decay `v ← trunc(3v/4)` |
| f=717 | 00:05 | **−38,0,0** | −1,0 | 1 | 12 | 1 | 12/12 | last motion |
| **f=718** | 00:05 | −38,0,0 | **0,0** | **0** | 12 | **2** | 0/12 | the `speed == 0` stop |

**13 changed frames (f=705…717), place `0 → −0x26` (−38), `pan 0→2`, `trk=12`
from the event through the run and after it (3 437 `trk=12` lines, f=704…4140),
Y=Z=0 on every line.** The only other changed-place frame in the whole trace is
the half-time roll at **f=4141** (`ph=12` at f=4139, then `ph=1`; the camera is
reset to `0,0,0` with `trk=−1`, `pan=0` — one frame, no velocity): a period
reset, not a second pan. The mid-run sentinel f=2000 (tick 6670) and the last
line f=4166 (tick 13890, after the reset) are in the extract so the claim is
not a cherry-pick: after the pan the camera stays parked at −38 through f=4140,
and after the reset at 0,0,0 — no later no-slot event panned.

- **G1 (trace):** exactly one delta run in the run (`f=705…717`, 13 frames),
  `trk=12` at the event and across it, `pan 0→2`, `pos_y=0` and `follow=0` on
  every line.
- **G2 (arithmetic):** the observed `(|Δ|, frames) = (0x26, 13)` read off the
  trace; against the recorded seed (`vel=(−2,0)` ⇒ `s = −0x18`, `v0 = −2`) it is `T(−0x18)` over 13 frames (`T = 2·Σ n·v`, `v ← trunc(3v/4)`;
  FU321 §3.4), the same closed form the tree's `m2_pan_curve` pins
  (`tests/test_engine_m2.c`; the Task-2 witness prints `seed (−24,0) v0 (−2,0)
  T (−38,0)`).
- **G4 (seed):** `|v0| = 2` on the event line (`vel=−2,0`, `spd=2`); the
  `pan`-count trap is avoided: `pan=2` counts pan steps, not frames; the run is
  the 13 changed frames.

**The chain and the gate are Layer-1 claims (G3), not trace cells.** The trace
carries no record/gate/face cells (its cells are `ph`, `clock`, `tick` and the
camera cells; `match_run.c:4657-4671`); it reproduces the **event and the
arithmetic** and **cannot show the seed or the gate** (FU321-L1). Those are
cited from the tree's own natural witness in the same session (Task 2's pins,
`tests/test_engine_m2.c`; the stderr line quoted above): `code 4`, `has_slot 0`,
`lane 56/−56`, `cam_dz6f 0`, `bound 68`, `timer81 0`, `face7d 0xFF00`,
`dir (0,0)`, the live face `Δ 0x0`, and the producer chain `timer79 6`,
`timer7b 6`, `nib 6`, `vel (−6,0)`, `speed71 6`, `pos (56,0,0)` — the
producer-conditioned tuple of §"The claim".

## The captures and the AE

The pairs are `import -window` grabs (the FU317 §5.3 primary route) from the
**same run** as the trace; every grab is logged with its PRE/POST trace lines
(`capture.log`), so each committed image is anchored to the delta run and the
displayed second. The watcher bursts back-to-back pairs from **before** the
expected event window (phase 2 + 460 ticks) because the natural pan is only
**13 granted frames ≈ 0.43 s** — the p33/p32 lesson that an `import` grab costs
~45–70 ms (3–4 granted frames). The committed files are `win-09` →
`p35-l2-pan-a.png`, `win-10` → `p35-l2-pan-b.png` (**the strict in-run pair**),
`win-11` → `p35-l2-pan-c.png`, `win-12` → `p35-l2-pan-d.png` (**the run-end
pair**; the raw names are the watcher's), `still-s2-a/b` →
`p35-l2-still-a.png`/`p35-l2-still-b.png`.

| pair | files | clock | trace bounds (PRE → POST) | AE (`960x591+0+0`) | full-frame AE | HUD clock crop AE (`140x66+0+650`) | bbox |
|---|---|---|---|---|---|---|---|
| pan (strict in-run) | `p35-l2-pan-a.png`, `p35-l2-pan-b.png` | **00:05** | f=708→710 and f=710→714 | **0** | 0 | **0** | — (no diff) |
| pan (run-end) | `p35-l2-pan-c.png`, `p35-l2-pan-d.png` | **00:05** | f=714→716 and f=716→719 | **502** | 502 | **0** | **23x41+458+296** (`p35-l2-pan-diff.png`) |
| still (s2) | `p35-l2-still-a.png`, `p35-l2-still-b.png` | **00:09** | f=818→822 and f=825→827 | **0** | 0 | **0** | — (no diff) |

- **The strict in-run pair is pixel-identical (AE 0).** Both grabs lie wholly
  inside the delta run's motion frames (f 705…717; the camera place moves
  −16 → −32 across the pair) and both show 00:05 (clock crop AE 0), yet the
  play area is unchanged. The camera translation has **no scene-translation
  signature in this view**: the presenter keeps the **tracked** record at a
  fixed screen point (the five identical grabs `win-07…win-11` cover the event
  frame and the run's motion frames f 701…716, while the camera moves 0 → −36,
  and the run-end diff is confined to a 23×41 box at x=458), and the port draws
  sprites on black with no pitch/background and no other visible slot. This is
  measured, not assumed: five grabs across the event and twelve of the run's
  thirteen motion frames.
- **The run-end pair changes by 502 px, all inside a 23×41 box at the sprite**
  (`23x41+458+296`) — the tracked sprite's animation frame at the run's end,
  not a scene translation (the two crops show the same player in a different
  pose; the still scene's idle sprite does not animate). This is the only pixel
  change anywhere in the window's vicinity *except* the event's view switch.
- **The event's view switch is not a pan pair** (recorded for completeness, not
  committed): `win-06` (f 697→701) → `win-07` (f 701→704) AE **53 850**, bbox
  `293x394+317+0` — the pre-event close-up disappears when `tracked` goes
  `−1 → 12` at the event. It is the *event*'s effect (a dropped view), and
  using it as the pan pair would be a false attribution.
- **The still pair (s2) is pixel-identical (AE 0, clock 00:09)**, camera
  constant at −38, `trk=12`, `pan=2`, **no key held** (the run is undriven).
  The siblings agree: `s1` (f 749…758, 00:07) AE 0 and `s3` (f 917…921, 00:12)
  AE 0 — the idle scene is stable, and the pair is not tuned.
- **Gate 1 (relative, binding) is NOT MET in this view: AE_pan 0 = AE_still 0.**
  The relative-AE test cannot discriminate a camera translation that the
  presenter does not draw as a scene translation. This is the FU333 §7 **W-b
  waiver**, applied to the AE only: *the waiver weakens only the AE, never the
  trace*. Using the run-end pair (502 > 0) would pass the arithmetic but its
  502 px is the sprite's own animation frame, not the camera — the p33
  precedent's warning ("the AE alone is not an animation discriminator") is
  exactly why it is recorded as a sprite-only residual and **not** as Gate 1.
- **Gate 2 (whole-scene bbox) fails as documented and cannot pass in this
  build:** every diff is sprite-limited (the largest, the event view switch at
  293×394, is the pre-event close-up; the pan's own diff is 23×41), because the
  renderer draws sprites on black (no pitch/background) — the FU313-L4 /
  FU305-L6 caveat, never an attribution.
- **The clip-route check (both alternatives attempted):** a 60 fps lossless
  `x11grab` clip of the window was recorded after phase 2 (no walkthrough load;
  the p32 encoder finding cannot move a wall-clock key here — the run is
  undriven). Its frames inside the run are **byte-identical** except the same
  single animation step (AE 502 in a 23×41 box at the sprite), confirming the
  `import` result by an independent capture path. The clip could not be
  cropped to the client (`960x591+0+0`): `xdotool getwindowgeometry` reports
  the WM frame, not the client origin (the client is inset by ≈14 px left /
  ≈49 px top), so the clip was used as the static-display check only and no
  clip frame is committed.

The AE is the relative corroboration, **not** the attribution; here it cannot
corroborate the camera translation at all, and the note says so. The pan's
attribution is the trace's (the camera cells only).

## Verdict

**A natural no-slot pan reproduced live under the undriven tape and the
producer-conditioned witness — YES, trace-attributed.** The camera event fires
at `f=704` (`trk=12`, `vel (−2,0)`, `spd 2`, `tim 0/12`, `ramp 12`, `ev 0x0`),
the place travels `0 → (−38,0,0)` over **13 changed frames** (`f=705…717`),
the stop is `f=718` with `pan 0→2`, and `follow=0`/`pos_y=0` hold on every one
of the 4 167 body lines. The live **frame** cells match the tree's Task-2
witness and FU332's tuple exactly (gate 703, event 704, motion 705…717, stop
718, `T=−38`, `pan 2`); the tick cells are a constant −5. The gate/chain cells
are the tree's producer-conditioned witness (`timer79 6`, nibble 6,
`vel73 −6`, `lane 56`, `has_slot 0`, `Δ 0x0`), never read from the trace.

**The producer condition is stated, not hidden:** the produced limit is 6 —
`table[0]` with the staged `player_d = 0` and the port's `+0x9D` OR term 0 by
construction — the port-reachable staging, **not** a native per-record roster
measurement (FU330-L1/FU328-L3; FU332-L2). The native roster index, the
`+0x9D` producer (OL-72) and the P2/P3 `+0x79` re-writes are unported, so the
*mechanism* is native-faithful and the *value* is the staging's. The verdict is
never worded "the trace shows the seed" (Layer 1 owns the chain) and never "the
natural route pans" without the measured tuple.

**The pixel corroboration is waived, not faked:** the strict in-run pair is AE 0
(the presenter pins the tracked sprite and draws no pitch), the run-end pair's
502 px is the sprite's animation frame (bbox 23×41), and Gate 2 stays the
documented sprite-only caveat. The FU333 §7 W-b waiver covers the AE only.

## Timing limits found (FU321-L4 / FU333 §7)

1. **The prefix start is a race, and the natural pan survives it.** The driver
   reads the trace's **last** line (it cannot rewind to f=0), so the first seen
   tick is 7/10/14 across attempts. Attempt 4 started at **tick 14** (the RIGHT
   window 8 ticks late, KICK 14/20 instead of 14/17) and reproduced the
   **identical tuple** (phase 2 `f=539`/tick 1800; stop `f=718`/tick 2397) —
   the natural pan is robust to the prefix's opening jitter. Attempt 3 was
   void: the old `first tick > 10` guard aborted before any key (no phase 2),
   so the guard is now 25 and a late start is logged, not fatal.
2. **The phase-2 landing drifts between live runs** (tick 1800 in all three
   keyed attempts here; the p32/p33 runs 1800/1897; the null trace 1797; the
   probe 1802). The pan anchors are **absolute in ticks** (2347/2350/2397),
   constant −5 against the null tape's step column, while the `f=` cells match
   the tree exactly. A future replay anchors on the **frames**, not a tick
   conversion.
3. **The pan is 13 granted frames ≈ 0.43 s** at ~30 grants/s (displayed
   00:05; the clock crop AE 0 verifies the second). The `import` route is
   ~45–70 ms per grab (3–4 frames), so the watcher must burst from **before**
   the event (ARM = phase 2 + 460 ticks); the accepted strict pair is two
   back-to-back grabs whose PRE/POST windows both lie inside f 705…717.
4. **The renderer's coverage, not the capture route, is the AE limit.** The
   tracked sprite stays at a fixed screen point through the pan (the run-end
   diff is a 23-px box at x=458; the five identical grabs cover the event and
   the motion frames f 701…716), and there is no pitch/background; a camera
   translation therefore has no scene-translation pixel signature. Verified by
   both routes (the `import` burst and a 60 fps clip). Gate 1 cannot be met in
   this view — the W-b waiver.
5. **The clip route's geometry trap:** `xdotool getwindowgeometry` reports the
   WM frame; the client is inset (≈14 px left / ≈49 px top here), so a clip at
   the reported origin misses the client's left/top edge and cannot be cropped
   `960x591+0+0`. Use `xwininfo -id $WID`'s absolute upper-left for a future
   clip. (The clip was still valid for the sprite-region static check.)
6. **The `--window` (XSendEvent) key route remains unusable** (the p33 timing
   limit 1: the act-1 KICK's release arm never fires). Global XTEST everywhere,
   walkthrough and prefix alike.
7. **No absolute AE floor** (FU309-L6): the AE is relative; its attribution is
   the trace's — and here the relative test is waived, not relaxed.

## Invariants

No code/test/golden change. M1 `09b726b7…` (22 004 B) and M2 `6aa2703b…`
(295 087 B) unchanged; `./build/test_engine_m1 | cmp -
tests/golden/engine/m1-frames.txt` and `./build/test_engine_m2 | cmp -
tests/golden/engine/m2-frames.txt` are identical (re-run at the close of this
task). The trace sink stayed default-off outside this run; the ISO was only
read. The census stays `60/80 = 75%` (the producer wires no dispatch row), so
the README title is untouched. `make check` is **111/111** green on this tree
(0 failures; 222.5 s), and `./build/test_engine_m2` prints the natural witness
line quoted in §"The claim" unchanged (P6 GREEN).

## Commands (exact)

```sh
# the run (the brief's command; SDL3, :1)
DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p35/p35-trace.log make game
# walkthrough: windowactivate --sync $WID, then 0.2 s holds
#   Return (intro) / BackSpace (front-end) / Return (match)
# the driver: the M2_KEYS prefix by trace tick; NOTHING after phase 2
python3 /tmp/opencode/p35/p35-run.py "$WID" /tmp/opencode/p35/p35-trace.log \
  /tmp/opencode/p35/driver.log
# the capture watcher: the burst from phase 2 + 460 ticks + the s1/s2/s3 stills
python3 /tmp/opencode/p35/p35-capture.py "$WID" /tmp/opencode/p35/p35-trace.log \
  /tmp/opencode/p35
# the 8-bit conversions of the committed grabs (16-bit import -> 8-bit)
convert win-09.png -depth 8 docs/screens/p35-l2-pan-a.png
convert win-10.png -depth 8 docs/screens/p35-l2-pan-b.png
convert win-11.png -depth 8 docs/screens/p35-l2-pan-c.png
convert win-12.png -depth 8 docs/screens/p35-l2-pan-d.png
convert still-s2-a.png -depth 8 docs/screens/p35-l2-still-a.png
convert still-s2-b.png -depth 8 docs/screens/p35-l2-still-b.png
# the crops, the AE, the diff, the bbox
for p in pan-a pan-b pan-c pan-d still-a still-b; do
  convert docs/screens/p35-l2-$p.png -crop 960x591+0+0 +repage /tmp/opencode/p35/v-$p.png
  convert docs/screens/p35-l2-$p.png -crop 140x66+0+650 +repage /tmp/opencode/p35/vc-$p.png
done
compare -metric AE /tmp/opencode/p35/v-pan-a.png  /tmp/opencode/p35/v-pan-b.png  null: 2>&1  # 0
compare -metric AE /tmp/opencode/p35/v-pan-c.png  /tmp/opencode/p35/v-pan-d.png  null: 2>&1  # 502
compare -metric AE /tmp/opencode/p35/v-still-a.png /tmp/opencode/p35/v-still-b.png null: 2>&1 # 0
compare -metric AE /tmp/opencode/p35/vc-pan-a.png /tmp/opencode/p35/vc-pan-b.png null: 2>&1  # 0
compare -metric AE /tmp/opencode/p35/vc-pan-c.png /tmp/opencode/p35/vc-pan-d.png null: 2>&1  # 0
compare -metric AE /tmp/opencode/p35/vc-still-a.png /tmp/opencode/p35/vc-still-b.png null: 2>&1 # 0
compare -compose src /tmp/opencode/p35/v-pan-c.png /tmp/opencode/p35/v-pan-d.png \
  docs/screens/p35-l2-pan-diff.png
convert docs/screens/p35-l2-pan-diff.png -format '%@' info:   # 23x41+458+296
# the clip-route check (attempt 2/4; not committed: the client crop was stale)
ffmpeg -f x11grab -framerate 60 -video_size 960x720 -i :1+217,255 -t 7 \
  -c:v ffv1 -level 3 /tmp/opencode/p35/pan-clip.mkv
```

The committed PNGs are the 8-bit conversions (`-depth 8`) of the 16-bit
`import` grabs; the window's source pixels are 8-bit, and the AE is unchanged
by the conversion (0 / 502 / 0 measured on the committed files; the same
figures on the raw grabs).

## The extract

`docs/screens/p35-l2-trace-pan.txt` (60 lines: 1 trace header + 4 comment lines
+ 55 selected body lines) carries the phase-2 line `f=539` (tick 1800), the
gate/event/pan window `f=701…721` (gate f=703, event f=704, motion f=705…717,
stop f=718), the still windows `f=749…758` (s1), `f=818…827` (s2, the committed
still pair) and `f=917…921` (s3), the mid-run sentinel `f=2000`, the half-time
reset `f=4137…4143`, and the last line `f=4166`, from the full trace
`/tmp/opencode/p35/p35-trace.log` (1 header + 4 167 body lines, sha256
`953a0cc5b89ae7ba29b15956467022d2fe2e134f64155d88ad249713f4211f37`); every
extract body line is verbatim from the full trace (checked as a verbatim,
ordered subsequence).
