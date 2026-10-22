# P31-L2 — the visible-pan acceptance (a natural row-04 pan that also produces the claim, live)

**Task:** M2 phase-31 Stage-2 task 3 (`docs/superpowers/plans/2026-10-22-fifa96-m2-phase31-producers.md:50-57`).
**Procedure:** `docs/ghidra/FU317_r2l6_claim_acceptance.md` §5–§6 (the trace primary, the same-clock pairs,
the relative AE, the bbox caveat) + the phase-30 Layer-2 procedure (`FU313` §6).
**Scope:** captures + this note only; no production code, no test change, no golden change.

## The run

`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/p31-cam-trace2.log make game`
(`./build/fifa96 game/FIFAPCCD96.iso`, SDL3, 960×720 client at `+66,+69`).

Walkthrough: intro RETURN → front-end BACKSPACE → RETURN (match). The live drive replays the
`M2_KEYS` prefix by the trace's `tick` field (one 100 Hz engine step): RIGHT at tick 7…47 (the
KICK press/release at 14/17), LEFT 47…140, the act-1 KICK press/release at 1790/1797. Phase 2
lands at **tick 1897, f=568** (the null tape's 1802); DOWN starts at **tick 1917 (phase 2 + 20)**
and is held until the pan is complete.

**The pan (trace primary), all cells from the `FIFA96_CAMERA_TRACE` sink:**

| frame | clock | pos | vel | trk | pan | reading |
|---|---|---|---|---|---|---|
| f=1512 | 00:31 | 0,0,0 | 0,0 | −1 | 0 | the parked camera; the row-04 gate frame |
| **f=1513** | 00:31 | 0,0,0 | −5,0 | **2** | 0 | the event set (`tick=5047`), `tim=0/12` |
| **f=1514** | 00:31 | **−10,0,0** | −5,0 | 2 | 0 | first motion (`tick=5050`) |
| f=1540 | 00:32 | **−144,0,0** | −1,0 | 2 | 3 | last motion (`tick=5137`) |
| f=1541 | 00:32 | −144,0,0 | 0,0 | 2 | **4** | stop |

**27 changed frames (f=1514…1540), place `0 → −0x90`, `pan 0→4`, Y=Z=0.** `follow=0` on all
**14 984** body lines (f=0…14983; `grep -c 'follow=1'` = 0); `|x| = 0x90` is below the OOB call
site (`0x6C0`) and the armer (`0x730`), so the scanner/armer stays at 0. The trace is the
same-run attribution; the extract is `p31-l2-trace-pan.txt` (the phase-2 line, the pan window
f=1495…1560, and the still window f=1578…1590, from `/tmp/opencode/p31-cam-trace2.log`).

## The captures and the AE

The clip: `ffmpeg -f x11grab -framerate 60 -video_size 960x720 -i :1+66,69 -t 70` →
`/tmp/opencode/p31-live2.mkv`. The pan runs at match clock 00:31…00:32; the display-second
boundary 00:31→00:32 is at clip t≈44.15 s, so 00:31 covers the motion's first 14 frames
(f=1514…1527) and 00:32 the last 13 (f=1528…1540). The pan pair (t=43.883/44.133 s) maps to
match **≈f 1520 / ≈f 1527.6**, i.e. both inside the 00:31 half of the delta run.

| pair | files | clock | clip time | AE (`960x591+0+0`) | bbox |
|---|---|---|---|---|---|
| pan | `p31-l2-pan-a.png`, `p31-l2-pan-b.png` | **00:31** (clock-area AE 0) | 43.883 s, 44.133 s (0.25 s) | **73 402** | **317×382+269+0** |
| still | `p31-l2-still-a.png`, `p31-l2-still-b.png` | **00:33** (clock-area AE 0) | 46.050 s, 46.133 s (0.083 s) | **27 849** | 293×394+131+0 |

- **Gate 1 (relative, binding): AE_pan 73 402 > AE_still 27 849** (2.6×). Full-frame AE equals
  the crop on both pairs (the HUD band contributes 0, the displayed clocks are pixel-identical).
  The AE is the `960x591+0+0` crop (`convert -crop … +repage`, then `compare -metric AE`; the
  `compare -crop 960x591+0+0` form returns the same counts).
- The still pair is the same run, 2 s after the pan: camera static at `−144`, **no key held**
  (DOWN released at tick 5270 = f=1580 ≈ clip 45.93 s; the keyup is processed on the next poll,
  and the pair maps approximately to **≈f 1585 / ≈f 1588** — the second sits at the f=1588
  00:34 boundary, while the pixel clock is 00:33 on both), the scene still animating. The
  phase-28 precedent also used different seconds for the pan and still pairs.
- **Gate 2 (whole-scene bbox) FAILS and cannot pass in this build:** the port renders sprites on
  black (no pitch/background), so the diff is confined to the sprite band (`317×382` vs the
  proposed `864×443`). A whole-scene translation is not measurable through the AE here
  (FU313-L4 / FU305-L6); `p31-l2-pan-diff.png` is the diff artefact. The AE corroborates a scene
  change, it does **not** attribute it to the camera.

## Verdict

**Visible pan — YES, trace-attributed.** The camera place moves `0 → −0x90` over 27 granted
frames, `pan 0→4`, `trk=2`, and the same-run pair at the same displayed second (00:31) lies
inside that delta run. This is the natural row-04 pan measured headlessly by T1, reproduced live
through the tape-equivalent drive. **Follow/Y stays WAIVED** (`follow=0` on every line, Y=Z=0);
the armer/OOB stays unreachable (`|x|=0x90`). The whole-scene bbox gate is the documented
sprite-only caveat, never an attribution.

## Timing limits found

1. **The bare DOWN hold is not enough.** A first attempt that pressed the kickoff late and held
   DOWN only after phase 2 (no M2 prefix) kept `pos=(0,0,0)`, `trk=−1` for ~40 match seconds
   (`/tmp/opencode/p31-cam-trace.log`; not committed): a read-only `gdb` dump of that process
   showed `slot.entity=1` (record 1 at x=52) while the panning record 2 sat far from the band.
   The successful run needs the tape-equivalent prefix (RIGHT/KICK/LEFT at ticks 7/14/47/140)
   and the act-1 KICK at 1790.
2. The phase-2 landing shifted to tick 1897 (null: 1802), but the pan still landed at tick 5050
   (null: 5044) — the walk/claim timing is absolute, not phase-2-relative.
3. The same-second pan window is **14 match frames ≈ 0.47 s** at 00:31 (13 ≈ 0.43 s at 00:32);
   at 30 match frames per 100 ticks and a 60 fps clip that is ~28 clip frames (≈26 for the 13).
   The pair must be picked inside it; the HUD second boundary is the hard edge.
4. The scene (entities) stops changing at clock 00:34 while the trace continues; the HUD clock
   keeps ticking (00:35 / 00:47 / 00:57 at clip 48 / 55 / 65 s), so later frames are not static
   — their residual AE (688 at 48 s vs 55 s, 716 at 55 s vs 65 s) is the clock digits. The still
   pair is the last animated 00:33 second before the scene settles.

## Invariants

No code/test/golden change. M1 `09b726b7…` and M2 `8434cfab…` (295 087 B) unchanged; `make check`
green (see the task report). The trace sink stayed default-off outside this run.
