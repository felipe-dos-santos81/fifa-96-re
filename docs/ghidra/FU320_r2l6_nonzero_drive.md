# FU320 — the non-zero-delta natural drive (phase-32 R2-L6)

**Phase/Provenance:** M2 phase-32 recon-ahead wave, Stage-1 **task 3 of 4** (`docs/superpowers/plans/2026-10-22-fifa96-m2-phase32-recon-ahead.md:70-85`), scoped by `docs/superpowers/specs/2026-10-22-fifa96-m2-phase32-face-aligned-pan-design.md` §2/§3.3/§5–§9. Read-only slice: fresh Ghidra MCP reads against **`/FIFA96.EXE`** (`get_current_program_info` this session = `watcom:LE:32:default`, image base 0, `.image` overlay, 2708 functions), a fresh read of the named C sources, and an **out-of-tree** probe (`/tmp/opencode/fu320/{probe.c,angle_bounds.c,body_replay.c}`) linked against the built `build/libfifa96_*.a`. No repo file was created or changed except this draft; no git; the ISO was only read; M1 `cmp` and M2 `cmp` re-verified byte-identical (§9). Raw probe dumps and the run log are named in §9.

## 1. The question

Can a **natural input drive** bring a code-4 record into the `0x40` band at a **non-zero offset** from the parked camera, with its live `+0x7D` face within `±0xAB` of the real bearing, so the row-04 pan fires **away from the zero-delta knife edge**? If not, what minimal producer change creates one?

The phase-31 T1 witness (`tests/test_engine_m2.c:4007-4263`, `run_natural_claim_pass`: the verbatim `M2_KEYS` + a DOWN hold from tape index 1820; the undriven control is `:3617-3717`) drives the gate only at the exact point `pos == cam == (0,0)` (bearing `0x80`, face `0x100`, Δ`0x80`). This slice measures the **offset** variant.

## 2. The gate algebra for an offset record

### 2.1 The native fold (fresh this session)

| Claim | Fresh evidence |
|---|---|
| The band tail builds the bearing from the **camera-minus-position** words: `+0x6B>>16` = word `+0x6D`, `+0x6D>>16` = word `+0x6F`, then `CALL 0x8DD70` | `disassemble_bytes 0x7EE40/0x50`: `0x7EE4B MOV EDX,[EBP+0x6D]`, `0x7EE4E MOV EAX,[EBP+0x6B]`, `0x7EE51/54 SAR …,0x10`, `0x7EE57 CALL 0x8DD70` |
| `0x8DD70` is a two-argument wrapper into the angle routine | `disassemble_bytes 0x8DD70/0x90`: `MOVSX EDX,DX; PUSH EDX; CWDE; PUSH EAX; CALL 0xCD474` |
| The angle routine's **diagonal tie** yields `0x80` before the sign fold | `disassemble_bytes 0xCD474/0x100`: `0xCD491 CMP EDX,ECX; 0xCD495 JZ 0xCD4B4; 0xCD4B4 MOV EAX,0x80; 0xCD4B9 JMP [EBX+0xCD4C0]` (the fold table), the atan byte table at `0x14072C` (`0xCD4A6`) |
| The gate: `Δ = (angle − face) & 0x3FF`, folded at `0x200`, pass iff `Δ <= 0xAB` | `0x7EE5C SUB AX,[EBP+0x7D]`, `0x7EE60 AND AH,3`, `0x7EE63 MOVSX`, `0x7EE66 CMP EDX,0x200`, `0x7EE6E/73 EDX=0x400−EAX`, `0x7EE78 CMP EAX,0xAB`, `0x7EE7D JG 0x7F138` |
| The port is a faithful transcription of both stages | `fifa96_entity_update.c:75-119` (`fifa96_entity_angle`), `fifa96_entity_update.c:211-229` (the distance metric), `fifa96_outfield.c:626-635` (the tail) |
| The dispatch stages the live cells: `lane = e->lane_x` (+0x6B), `word6d = e->lane_z` (+0x6D = cam_dx), `word6f = e->cam_dz6f` (+0x6F = cam_dz), `face_word7d = r->face7d` (+0x7D) | `fifa96_match_handlers.c:2640-2654`; the producers `fifa96_match_run.c:1495-1516` (the BF20 block: `bound := old lane` `:1504`, `lane_z := cam_dx` `:1505`, `cam_dz6f := cam_dz` `:1506`, `lane_x := lane` `:1507`), `fifa96_action_handlers.c:335-350` |

Convention (measured this session, `angle_bounds`): **`0` = +z, `0x100` = +x, `0x200` = −z, `0x300` = −x**, quadrant `0x100` = 90°, octant `0x80` = 45°, `angle(0,0) = 0x80` (the tie). Let `dx = cam_x − pos_x`, `dz = cam_z − pos_z` (16-bit words, `SAR …,0x10`), `B = angle(dx,dz)`.

**The exact fold** (`0x7EE5C..0x7EE7D`, `outfield.c:632-635`):

```
d = ((uint16)B − (uint16)F) & 0x3FF       ;  SUB AX,[+0x7D] ; AND AH,3
if (d > 0x200) d = 0x400 − d              ;  CMP EDX,0x200 / EDX=0x400−EAX
pass  <=>  d <= 0xAB                      ;  CMP EAX,0xAB ; JG fail
```

### 2.2 The pass set

**Face `0x100` (the measured live value, the step-12 row-`0x24` constant).** `Δ = |B − 0x100|` folded, so the pass set is `B ∈ [0x55,0x1AB]`. Because `B` is an eight-octant atan fold, that set is exactly:

```
pass  <=>  dx > 0  and  4·|dz| < 7·dx          (the west-side cone)
           OR (dx,dz) == (0,0)                  (the tie: B = 0x80, Δ = 0x80)
```

`dx > 0` means **the camera is east of the record**; with the camera parked at the origin, `pos_x < 0`. The cone half-angle is `0xAB` (60.1°) and the integer boundary is `7/4` (measured: `dx=8/16/32/64` pass up to `|dz| = 13/27/55/111`, first fail `14/28/56/112` = exactly `7/4·dx`). `dx <= 0` fails everywhere except the origin (min `Δ = 0x100`), including the whole east approach of the T1 tape (`dx = −4`, `B = 0xFF00 ≡ 0x300`, `Δ = 0x200`).

**A live face `F`.** The same fold gives `B ∈ [F−0xAB, F+0xAB]`: the record passes when the **camera lies within ±60.1° of its face bearing**, i.e. when it is roughly facing the camera. If a live producer keeps `F` equal to the walk bearing, then on a straight approach the camera ahead gives `B = F` (`Δ = 0`) and the pass happens at the **first** band frame with `lane` strictly decreasing — no zero-delta, no plateau. That is the structural difference between R1 (live face) and R2 (static face).

**The zero-delta special case.** At `dx = dz = 0` (and on every exact `|dx| == |dz|` diagonal) the routine takes the tie `0x80` *before* the sign fold. The pass then needs `|0x80 − F| <= 0xAB` → `F ∈ [0x3D5,0x3FF] ∪ [0,0x12B]`: **343 of 1024 faces** (measured, `angle_bounds`), a circular 120° sector centred on `0x80`, not on `0x100`. The T1 face `0x100` sits inside it at Δ`0x80`; the exact-zero point is a single geometric point, which is the knife edge.

### 2.3 The second bind: `lane <= bound`

The admission is two gates, and the second one is what the offset drive really has to beat (`outfield.c:610-618` = `0x7ED89..0x7EDDF`):

```
(0x40 − timer81) >= lane     ; 0x7ED99/9B
lane <= bound                ; 0x7EDCE (bound = the previous frame's lane, +0x77)
camY (0x157750) <= 0x50      ; 0x7EDD8
```

`bound` is the *previous* lane (`match_run.c:1504`), so `lane <= bound` admits only a **non-increasing** lane. The BF20 block runs **after** the row-04 dispatch, so the tuple sampled at frame `F` is the tuple the dispatch at `F+1` evaluates (FU314 §4's measured one-frame boundary). The lane is the native octagonal metric (`entity_update.c:211-229`): `max + 3·min/8` while `min > max/2`, else `max + min/4`, `11a/8` on the tie. Its **integer plateaus** (a frame where `lane` repeats) are the only frames where a *receding* record can satisfy `lane <= bound`.

## 3. The probe (out of tree, repo untouched)

`/tmp/opencode/fu320/probe.c`: the FU314 §4 / FU317 method — the null backend, `step_ns = 10 ms`, `game/FIFAPCCD96.iso`, the verbatim `M2_KEYS` prefix (front-end → match start 5 → row `0x24` 12 → act-1 1679 → **phase 2 at step 1802**) plus a probe-local post-phase-2 drive, 6 500–12 000 steps, one run per drive. Per **granted frame** it dumps the camera cells and, for every active record with `code == 4` (or `lane <= 0x40`, or the slot entity), the full gate tuple `id, code, active, pos, lane(+0x6B), bound(+0x77), cam_dx(+0x6D), cam_dz(+0x6F), face7d, bearing, Δ, PASS, timer81, has_slot, anim_id, dir`, with `PASS` computed by the port's own `fifa96_entity_angle` + the exact fold above. `ctl` (the undriven tape) is the negative baseline.

## 4. Evidence: the measured sweep

### 4.1 The R2 family — a `z` nudge, then the west run

Every passing drive has the same shape: a short **LEFT** hold on tape indices `1820..1819+n` (the pad emits `dir = (0,+1)`, so the record strafes to a `z` target ≈ `0.6·n`: measured `12/16/16/20/24/36/48` for `n = 20/28/30/32/40/60/80`), then **DOWN** from `1820+n` to the end (`dir = (−1,0)`, a straight 2-unit/frame west run at constant `z`). The record then crosses `x = 0` **off-axis** (bearing `0x200`, Δ`0x100` → refused), keeps west, and the first frame where the cone opens *and* the lane ladder plateaus fires.

| drive (`LEFT 1820..1819+n`, `DOWN` after) | gate `F` (trace key) | gate tuple `pos (x,z)` | lane/bound | `dx,dz` | bearing | Δ | pan first motion `F` / claim |
|---|---|---|---|---|---|---|---|
| `L20` | 1517 | `(−10,12)` | 15/15 | `10,−12` | `0x18F` | `0x8F` | 1519 / 1519 r2 |
| `L28` | 1528 | `(−32,16)` | 36/36 | `32,−16` | `0x14C` | `0x4C` | 1530 / 1530 r2 |
| **`L30` (nominated)** | **1528** | **`(−32,16)`** | **36/36** | **`32,−16`** | **`0x14C`** | **`0x4C`** | **1530 / 1530 r2** |
| `L32` | 1521 | `(−18,20)` | 26/26 | `18,−20` | `0x189` | `0x89` | 1523 / 1523 r2 |
| `L40` | 1521 | `(−18,24)` | 30/30 | `18,−24` | `0x197` | `0x97` | 1523 / 1523 r2 |
| `L60` | 1525 | `(−26,36)` | 45/45 | `26,−36` | `0x19A` | `0x9A` | 1527 / 1527 r2 |
| `L80` | 1531 | `(−34,48)` | 60/60 | `34,−48` | `0x19C` | `0x9C` | 1533 / — (4 passes, no claim) |
| `L30` start 1810 | 1521 | `(−18,20)` | 26/26 | `18,−20` | `0x189` | `0x89` | 1523 / 1523 r2 |
| `L30` start 1830 | 1521 | `(−18,20)` | 26/26 | `18,−20` | `0x189` | `0x89` | 1523 / 1523 r2 |
| `R20` (mirror) | 1517 | `(−10,−12)` | 15/15 | `10,12` | `0x71` | `0x8F` | 1519 / 1519 r2 |
| `R40` (mirror) | 1521 | `(−18,−24)` | 30/30 | `18,24` | `0x69` | `0x97` | 1523 / 1523 r2 |
| `R80` (mirror) | 1539 | `(−34,−48)` | 60/60 | `34,48` | `0x64` | `0x9C` | 1541 / — (4 passes, no claim) |

All rows: `code 4`, `active 2`, `timer81 0`, `has_slot 1`, `anim_id 0` (arm A never), `dir (−1,0)`, `controlled 1`, `slot.entity 2`, `tracked −1`, camera `(0,0,0)`, `camY 0`, `pan 0`, `ev 0`. Each run has one gate pass except the `L80/R80` rows (4 admission passes, one pan): their later passes (`f=1603`, `1607`) are the same record at `lane 60/63` with the angle good, refused by arm B's own tracked exclusion (`outfield.c:682`: `is_ball_track` is 1 after the pan binds `[0x1577CA]`). The `L32`/`L30@1810` shift shows the **z is one step larger** (`z = 20`) when the LEFT window moves 10 entries earlier or later: the family is shape-stable, the tuple is not position-free (leg **FU320-L2**).

**Why the plateau is the real bind** (nominated run, frames around the gate):

| `F` | `pos` | lane/bound | evaluated? | Δ | verdict |
|---|---|---|---|---|---|
| 1511 | `(2,16)` | 16/17 | yes | `0x114` | angle fail (east approach) |
| 1512 | `(0,16)` | 16/16 | yes | `0x100` | angle fail (the crossing, bearing `0x200`) |
| 1513 | `(−2,16)` | 16/16 | yes | `0xEC` | angle fail |
| 1514 | `(−4,16)` | 17/16 | no | — | `lane > bound` |
| 1515 | `(−6,16)` | 17/17 | yes | `0xC6` | angle fail |
| 1516…1527 | `(−8…−30,16)` | 18…36 / 17…34 | no (12 frames) | — | `lane > bound` (the metric grows 2/frame) |
| **1528** | **`(−32,16)`** | **36/36** | **yes** | **`0x4C`** | **PASS** — the metric's branch switch at `abs(x) = 2z` holds the lane for two frames (`(2z−2) + 3z/8 = 2z + z/4 = 36` at `z = 16`) |

So the record is **west of the camera inside the band for 16 frames** (F=1513…1528) before the pass, but the cone opens only after `|x| >= |z|/1.75` and the ladder only admits on the metric's integer plateau. The pass is a **non-zero-delta** admission (real bearing `0x14C`, offset `(32,16)`, lane `0x24`) but it is **plateau-riding**, not an approach-side crossing (leg **FU320-L1**).

### 4.2 The nominated witness

**Drive:** `M2_KEYS` verbatim entries `0..1799`; `LEFT` held on tape indices **`1820..1849`** (30 entries); `DOWN` held **`1850..13999`**; 12 000 engine steps (**3 598 granted frames**, `f = 0…3597`, 101 s).

| measurement | value (trace key `f`, the same key the M2 P11–P13 pins use) |
|---|---|
| phase 2 | step **1802** (all runs) |
| the row-04 gate tuple | **`f = 1528`**: record **2** `code 4`, `active 2`, `pos (−32,0,16)`, `lane 36`, `bound 36`, `cam_dx +32`, `cam_dz −16`, `face7d 0x100`, **bearing `0x14C`**, **Δ `0x4C`**, `timer81 0`, `has_slot 1`, `anim_id 0`, `dir (−1,0)`, `controlled 1`, `slot.entity 2`, `tracked −1`, camera `(0,0,0)` `camY 0` `pan 0` `ev 0` |
| the offset | **32 units west (`0x20`), 16 off-axis (`0x10`); lane/distance `0x24`** — inside the cone (`16 < 1.75·32`) and the band (`36 <= 0x40`) |
| the event | **`f = 1529`**: `code 5` (the row-05 install), `vel (−5,0)`, `spd 5`, `trk 2`, `tim 0/12`, `ramp 12`, seed `slot_dir<<6 = (−0x40,0)` |
| the motion + the claim | **`f = 1530`**: `pos −10`, `tim 2/12`, `ball.carrier = 2`, `controlled 2` |
| the pan | **`f = 1530 … 1556`** (27 changed frames), stop **`f = 1557`** at **`(−0x90,0,0)`**, `pan 0→4`, `speed 5→3→2→1→0`, **one delta segment**, Y=Z=0 |
| the trace | `FIFA96_CAMERA_TRACE` 1 header + **3 598** body lines, `trk=2` on the pan lines, `ev=0x0`, **`follow=0` on all 3 598**, clock `00:32` (gate) → `00:33` (pan) |
| armer / OOB / stage | `goal_armed 0`, `abs(x) = 0x90 <= 0x6C0`; the camera `z` stays 0 on every frame, so the keeper stage-select's `abs(cam.z) > 0x7B0` prepass cannot arm (the undriven baseline is 6 118 calls / `ran=0`, FU317 §3.2; not re-instrumented here); one admission pass in 3 598 granted frames |
| determinism | the run was repeated; the per-frame dump is **byte-identical** (`sha256 c54503db…`) |

The pan is X-only because the arm-B seed is the **slot direction** (`outfield.c:689-691`, `slot_dir_x<<6 = −0x40`), not the gate bearing; the offset changes *where* the admission happens, not the pan axis (leg **FU320-L5**).

### 4.3 The negatives (each measured, same probe)

| drive / route | result |
|---|---|
| `ctl` (no drive) | 0 gate passes, camera 0; the AI record **12** approaches to `pos (4,0)`, lane `4`, and sits **east** (`bear 0xFF00`, Δ`0x200`); **0** west-band frames in the whole window |
| `DOWN 1820..end` (the T1 witness) | 1 pass at `f=1512`, `pos (0,0)`, `B 0x80`, Δ`0x80` — the **zero-delta** knife edge; pan `−0x90`, claim 1514 |
| `DOWN` start 1810 / 1830 | identical to `DOWN 1820` (the T1 hold start is insensitive) |
| `DOWN` released at 1840 / 1860 / 1890 / 1920 / 1960 | **0 passes**: the record stops at `x = 1828` (lane 1828); the released hold leaves the record 900+ units short of the near field |
| `LEFT` / `RIGHT` / `UP` alone | 0 passes (the record walks away in `z`/`x`) |
| `L10` (z=4) | 31 west-band frames, **0 passes**: the only angle-ready frame is `(−2,4)` at Δ`0xB4` (9 over the bound), and `(−4,4)` is already `lane > bound` |
| `L120` (z=72) | 0 passes: `z = 72` keeps the metric `>= 72 > 0x40`, so the record is never inside the band (measured `pos (−840,72)` at the window end) |
| `L40` + `DOWN` to 5200 + `UP` to the end (the turn-back) | the pan fires **outbound** at `f=1523`; the eastbound return never reaches a gate (the record is `code 5` from `f=1522`) — the "enter the band from the west moving east" crossing is **not** reachable with the static `+x` face |
| `L40` + `DOWN` to 5300 (stop) | identical to `L40` (the pan precedes the stop) |
| second half / restart / a defender over-run | not reachable in the window: `ph=2` on every frame from `f=538` (step 1802) to `f=3597`, no period end, no restart (a period/restart stage would be needed — spec §9.3; the R2 witness does not need it) |

**The algebra explains the negatives.** For a static `+x` face the pass set (`dx>0`, `4|dz| < 7dx`) is *symmetric in `|x|`*: any path that enters it must traverse it. Travelling west it exits the band only after the cone opens, so the westbound leg is always offered the pass first; an eastbound entry would require every westbound plateau to miss — measured false for every `z` that gets inside the band at all. Hence the only no-code witness is the off-axis west run, and its admission is the metric plateau.

## 5. The port shape

**No production change.** The witness runs on the current tree with no staged camera cell, no staged face, and no `act.id` staging: M1 `cmp` and M2 `cmp` are byte-identical (§9).

- **The prerequisite already landed:** FU314-L2's live-face staging, `fifa96_match_handlers.c:2654` (`s.face_word7d = r->face7d`). The body replay (`body_replay`, the FU314 §6 method) shows the nominated tuple emits arm B only with the live face: `face 0x100` → `events=1, code=0x1D, reload=0, seed=(−64,0)`; `face 0` (the pre-T2 staging) → `events=0` (Δ`0x14C`); `face 0x300` → `events=0` (Δ`0x1B4`).
- **The chain:** code-4 dispatch (`match_run.c:1457`) → `fifa96_outfield_row04_step` (`handlers.c:2751`) → admission `outfield.c:610-618` → the angle tail `:626-635` → the corner/slot returns (`team_7e7 0`, `has_slot 1` → `:662-668`) → arm B (`row_byte != 0x13`, `is_ball_track == 0`) `:682-707` → `event_code 0x1D`, seed `slot_dir<<6` → `fifa96_camera_event_set(...,0,...)` + `fifa96_camera_event_dir` + `fifa96_camera_set_tracked(2)` (`handlers.c:2785-2796`) → the walk (`camera.c:101-111`) with `vel = seed/0xC = −5`, the pan step (`camera.c:960-990`) `speed 5→3→2→1→0`, `pan_counter 0→4` → the row-05 install (`outfield.c:719`) claims at `f=1530`.
- **Why the record is eligible:** at the gate `tracked = −1`, so the handler's `is_ball_track` fallback is `id == controlled` = `2 == 1` = **0**; the panning record 2 is the *slot-bound* record, not the controlled actor (FU317's erratum). A drive whose band record is the controlled one would be refused (leg **FU320-L7**).
- **The robustness route (not needed for L1a).** If a live producer keeps `+0x7D` equal to the walk bearing (FU314-L5 `direct_face`, or FU314-L3's `0x8E1CB` frame-wrap arm), the pass becomes an approach-side admission: camera ahead ⇒ `B = F` ⇒ Δ`0`, at the **first** band frame (`lane = 0x40`), no plateau, offset 64. The replay confirms the geometry: the T1 band-entry tuple `pos (64,0)`, `lane 64`, `bound 66`, `dx = −64` emits `0x1D` with `face 0x300` (Δ`0`) and **nothing** with `face 0x100` (Δ`0x200`). This is a prediction of the producer slice, not a measurement of the tree (leg **FU320-L3**).

## 6. GO/NO-GO

**GO — a no-code R2 witness exists on the current tree.** The nominated drive (`LEFT 1820..1849`, `DOWN 1850..end`) admits record 2 at **`f = 1528`** with `pos (−32,0,16)`, `lane 36`, **bearing `0x14C`, face `0x100`, Δ `0x4C`**, `tracked −1`, `controlled 1`; the row-04 pan fires (`f=1529` event, `f=1530..1556` motion, stop `(−0x90,0,0)`, `pan 0→4`, `trk=2`, Y=Z=0, `follow=0`) and the claim follows at `f=1530` (`ball.carrier=2`). No production code, no staged camera/face, one deterministic run.

**NO-GO on** (a) calling it a robust **west-side crossing** — the admission rides a lane-ladder plateau (`lane == bound`) created by the octagonal metric, not an approach-side pass; the eastbound entry is unreachable with the static `+x` face; (b) any staged gate/face/camera or a re-pinned golden; (c) reading the pan as Z-steering — the seed is the slot dir, so the place delta is the same X-only `−0x90` as the zero-delta witness (FU321 may reuse the phase-31 Layer-2 procedure, leg **FU320-L5**); (d) treating the producer prediction (§5, `face 0x300`) as measured.

## 7. Numbered legs

1. **FU320-L1 — the R2 admission is plateau-riding.** Every measured pass (`L20…L80`, `R20…R80`) has `lane == bound` on an integer plateau of the octagonal metric (`entity_update.c:211-229`); the `L10` near-miss (Δ`0xB4` at the only angle-ready frame) is the counter-example. A robust non-zero admission needs the live-face producer. Owner: the row-04 gate + the metric; the producer route is FU318-L3/FU319-L5.
2. **FU320-L2 — the `z`-offset is tape-timing coupled.** The LEFT hold's length *and* start set the strafe target (`n=20/28/30/32/40/60/80 → z=12/16/16/20/24/36/48`; start `1810` or `1830` shifts `z` one step). The witness pins the exact indices; it is not a free-running play (FU317-L1/L9's class).
3. **FU320-L3 — the producer route prediction.** With `+0x7D` = the walk bearing the T1 east approach passes at `lane 0x40` with Δ`0` (body replay). Unmeasured on the tree (the face is the static step-12 `0x100`). Owner: FU318 (L3) / FU319 (L5) + the Stage-2 producer slice; this is the robustness/fidelity route, not the L1a witness.
4. **FU320-L4 — the second half / restart / defender over-run.** Unreachable in the 3 598-frame window (`ph=2` from `f=538`/step 1802 to the end; no period/restart; the AI record 12 reaches `(4,0)` east and never west). A period-staged probe is the only route (spec §9.3 allows it only if needed — it is not).
5. **FU320-L5 — the pan stays X-only.** The arm-B seed is `slot_dir<<6 = (−0x40,0)` (`outfield.c:689-691`); the gate bearing does not steer the pan. FU321 should expect the phase-31 place delta `0 → −0x90` and the 27-frame run; the *offset* is only in the admission tuple.
6. **FU320-L6 — the sampling boundary.** The probe samples post-frame; the tuple at `f` is the dispatch at `f+1` (event at `f+1`, motion at `f+2`). The same convention as FU314 §4/FU317 §7.9; the doc's `f` is the trace key.
7. **FU320-L7 — the controlled-record refusal.** `is_ball_track` falls back to `id == controlled` when `tracked == NONE` (`handlers.c:2675-2681`); the witness passes because record 2 ≠ controlled (1). A drive whose panning record is the controlled one is refused (FU314-L4). Owner: the row-04 staging.
8. **FU320-L8 — the pad→target model.** A released direction leaves the walk short (`DOWN` released at 1840…1960 all stop record 2 at `x = 1828`, 900+ units short); the LEFT `z` target (`z = 16`) is reached within ~60 frames and holds until the pan rearranges it. The exact native target-writer is not re-derived here. Owner: the input/locomotion slice.

## 8. Known limits

- **Probe sampling:** post-frame only; the tuple at `f` is the gate the dispatch at `f+1` evaluates. The `follow`/`rate` cells are read from the trace, not the in-memory sampler.
- **ISO-only:** the formation seed (record 2 at 1948, measured; the ball at ~218, FU314's review erratum) is ISO-dependent; the witness is a full-ISO run from the real kickoff.
- **Timing coupling:** the drive is a tape; the LEFT window sets the `z` offset and the DOWN hold must run to the crossing (a released hold freezes the target). A human `make game` drive reproduces the T1 timing only approximately (FU317-L9).
- **Window:** 12 000 steps = 101 s of the first half; no period, restart or second half.
- **One witness per `z`:** the family is deterministic but each `z` has a single pass frame; a Stage-2 change to the walk step, the metric or the lane order can move or drop it (the robustness route is leg FU320-L3).
- **The pan is the same place delta as phase 31** (`−0x90`, X-only): the acceptance's visible evidence is not new, only the admission is.

## 9. Provenance

**Fresh Ghidra (`/FIFA96.EXE`, standard tools only, scripts disabled, no writes/saves):** `get_current_program_info`; `disassemble_bytes 0x7EE40/0x50` (the gate `0x7EE4B..0x7EE7D`), `0x8DD70/0x90` (the wrapper), `0xCD474/0x100` (the tie `0xCD495 JZ 0xCD4B4`, `0xCD4B4 MOV EAX,0x80`, the table `0x14072C`, the fold `JMP [EBX+0xCD4C0]`).

**Sources read fresh (file:line):** `src/fifa96_loader/fifa96_entity_update.c` (`:63-73` sine, `:75-119` angle, `:205-229` distance/abs); `src/fifa96_loader/fifa96_outfield.c` (`:575-624`, `:626-720`); `src/fifa96_loader/fifa96_action_handlers.c` (`:335-350`); `src/fifa96_engine/fifa96_match_handlers.c` (`:2605-2827`, esp. `:2640-2654`, `:2744-2751`, `:2785-2796`); `src/fifa96_engine/fifa96_match_run.c` (`:1455-1534`); `include/fifa96_engine/fifa96_match_entities.h` (`:105-189`); `src/fifa96_loader/fifa96_camera.c` (`:95-118`, `:958-991`); `tests/test_engine_m2.c` (`:3617-3717` the undriven control, `:4007-4263` the T1 claim pass).

**Measurement artifacts (out of tree, `/tmp/opencode/fu320/`):**
- `probe.c` + `probe` — the sweep harness (the dump only; no engine/gate change); `sweep.txt` — the run summary log (31 runs: `ctl`, `down*`, `left/right/up`, `L10…L120`, `R20…R80`, `L*_down_up`, `L40_down_stop`, the timing shifts).
- `dump_ctl.txt`, `dump_down.txt`, `dump_down1810.txt`, `dump_down1830.txt`, `dump_down1840.txt`, `dump_down1860.txt`, `dump_down1890.txt`, `dump_down1920.txt`, `dump_down1960.txt`, `dump_left.txt`, `dump_right.txt`, `dump_up.txt`, `dump_l{10,20,28,30,32,40,60,80,120}_down.txt`, `dump_r{20,40,80}_down.txt`, `dump_l40_down_up.txt`, `dump_l80_down_up.txt`, `dump_l40_down_stop.txt`, `dump_l30_down1810.txt`, `dump_l30_down1830.txt`, `dump_left_down.txt`, `dump_right_down.txt`.
- **`dump_l30_down_12000.txt`** — the nominated run, 12 000 steps (`sha256 c54503db334784e74160a3de8d4dbc7e7026de386785dc70611e6917d968acb5`), byte-identical on the repeat (`dump_l30_run1.txt` + `rerun_l30.txt`); `dump_l30_down.txt` is the same drive at 6 500 steps.
- **`l30.trace`** — the same run with `FIFA96_CAMERA_TRACE` (1 header + 3 598 body lines; the pan lines `f=1529…1557`, `follow=0` on all).
- `angle_bounds.c` — the measured convention (`angle(0,1)=0`, `(1,0)=0x100`, `(0,−1)=0x200`, `(−1,0)=0xFF00`, `(0,0)=0x80`), the `7/4` cone boundary and the `343/1024` zero-delta face sector.
- `body_replay.c` — the measured tuples replayed through `fifa96_outfield_row04_step` (the nominated tuple, the T1 band entry with faces `0x100`/`0x300`, the zero-delta, the `L20/L40/L60/L80` tuples, the bound-34 near-miss).

**Invariants:** `./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt` → identical; `./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt` → identical (the test also prints the phase-31 witness line: gate `f=1512`, `code 4`, `lane_x 0`, `face7d 0x100`). No git operation; the repo tree was clean apart from the three sibling drafts.
