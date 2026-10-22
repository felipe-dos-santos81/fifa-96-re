# FU-317 — the natural claim-pan acceptance harness (the drive, the trace capture, the acceptance matrix)

**Phase/Provenance:** M2 phase-31 recon-ahead wave (Stage-1, **task 4/4 — `FU317: the acceptance harness`**), 2026-10-22, read-only. Deliverable of `docs/superpowers/plans/2026-10-22-fifa96-m2-phase31-recon-ahead.md:67-75` under `docs/superpowers/specs/2026-10-22-fifa96-m2-phase31-natural-claim-pans-design.md` §2–§5/§8. Authoritative program **`/FIFA96.EXE`** (Ghidra MCP, project `fifa96`; fresh `get_current_program_info` this session = `watcom:LE:32:default`, image base 0, `.image` overlay, 2708 functions; scripts disabled). Native evidence is either a **fresh `disassemble_bytes` this session** (§3.3) or marked *carried* with the frozen doc named. Engine/test evidence is a fresh read of the C sources (file:line) plus fresh runs of the already-built `./build/test_engine_m2` and seven throwaway `/tmp/opencode` probes (§10). No repo file was created or changed except this draft; no git; the ISO untouched; M1 `09b726b7…` and M2 `8434cfab…` re-verified byte-identical (§10).

---

## 1. Result and verdict

**The natural claim pan exists today, and the cheapest honest harness is one extra natural-play pass in the existing `test_engine_m2` probe plus the already-landed trace.**

Measured this session on the current tree, no production change, no `act.id` staging, no camera staging, the verbatim `M2_KEYS` tape extended only **after** the measured natural phase 2 (step 1802) with a plain **DOWN hold from tape index 1820**:

| measurement | value (run-relative granted frame `f`, 0-based — the trace key) |
|---|---|
| match start / phase 2 | step 5 / **step 1802** (unchanged) |
| the claim (`ball.carrier`) | **f = 1514**, carrier = **2**, `mr.slot.entity` = **2**, `pair.code/ack` = **0/0** |
| the row-04 gate frame | **f = 1512**: record 2 `code = 4`, `lane_x = 0`, `bound = 2`, `timer81 = 0`, `has_slot = 1`, `dir = (−1,0)`, `pos_y = 0` |
| the angle pass | **f = 1512** (the gate tail): the camera deltas `lane_z = cam_dz6f = 0` → `bearing = 0x80`, `face7d = 0x100`, `Δ = 0x80 ≤ 0xAB` — the **zero-delta degenerate pass** (the record is exactly at the parked camera; the approach frames carry `bearing 0xFF00`, `Δ = 0x200`, and fail — FU314's finding) |
| the event frame | **f = 1513**: `code = 5` (the row-05 install), `vel = (−5,0)`, `speed = 5`, `trk = 2`, `tim = 0/12`, `ramp = 12`, `ev = 0` |
| the pan | **f = 1514 … 1540** (27 changed frames, monotone), place `(0,0,0) → (−0x90,0,0)`; `speed 5→3→2→1→0`; `pan 0→4` |
| Y / Z | `max|y| = 0`, `max|z| = 0` (the pan is X-only) |
| the armer / OOB | `|x| = 0x90`, `goal_armed = 0`, `|x| ≤ 0x6C0` and `≤ 0x730` on **every** frame |
| the follow | `follow = 0` on **all 3 598** frames (`grep -c 'follow=1'` = 0) |
| the trace | 1 header + **3 598** body lines, `trk=2`, `pan=0..4`, `ev=0x0`, `clock=00:32` at the pan |

So the phase-31 Layer-1 witness is **GREEN on the current tree**, not RED-first: the phase-30 "the natural probe never fires the pan" datum is *tape-scoped* (that tape ends at index 1799, i.e. it supplies no open-play input), not an engine limit. The phase-30 **P6 partial** (the scripted pan only) is superseded: P6 becomes fully GREEN through a natural-route pad input; the scripted witness stays as the two-pan **fixture**.

**Honesty boundary (binding on the interpretation):** the angle tail passes at the **zero-delta** point (`lane_z = cam_dz6f = 0` → bearing `0x80`, Δ `0x80`), i.e. the claimant runs exactly over the parked camera; the approach frames carry bearing `0xFF00`, Δ `0x200`, and fail (FU314's measurement). The witness is a real native-gate pass over live cells with no staging, but it is **knife-edged**, and the face-aligned natural route stays FU314-L1's open question (§7.9, leg FU317-L10). The harness pins the pass terms (P13) so the mechanism is never invisible.

**Verdict: GO on the harness** — one extra probe pass (~200 LOC in `tests/test_engine_m2.c`, one run, +~9 s measured), zero production code, zero new target, zero golden change, plus the frozen `make game` capture procedure. **NO-GO only** on (a) re-deriving the row-04 gate algebra inside the test (measured to over-approximate by 2 082 vs 33 frames, §3.4), (b) treating the AE as attribution, and (c) claiming the second pan / the follow from this witness (§7).

---

## 2. The question, and the links the harness must carry

> **What is the cheapest honest harness that proves the natural claim pan headlessly and attributes the visible pan?**

The spec §5 acceptance decomposes into seven links, and the harness carries exactly these:

- **H1 — the natural witness** (spec §5 Layer-1, §8.1): a headless run from the **real kickoff** with a **scripted input sequence**, **no staged camera cells**, in which a claim fires the row-04 pan and the camera place leaves `(0,0,0)`. The claim inputs may be scripted; the witness must be the natural route (no `act.id = 0`).
- **H2 — the claim-state witness**: *which* cell is the claim (`ball.carrier` through the row-05 possession arm), with the pair cells recorded (`pair.code/ack = 0/0` here — the claim is not the pass/receive chain, leg FU317-L4).
- **H3 — the pan witness**: the pan count (delta runs, not "frames off zero"), the place delta, the monotone decay, the pan counter, the stop.
- **H4 — the tracked / gate / armer state**: `tracked` = the claimant at the event frame; the claimant's gate inputs at the gate frame; `goal_armed = 0` and the OOB predicate 0 (the honest limit).
- **H5 — the frame-path fixture + riders** (spec §5 Layer-1 second clause): the existing `test_live_claim_pan_row04_half_line` (producer inputs only, `begin_isolated`) with one mutation-RED rider per producer; the second-pan rider lands with FU316's verdict.
- **H6 — the visible capture** (spec §5 Layer-2): `make game` shows the pan; same-clock pan/still pairs; the **trace is primary**, the AE is relative corroboration; the whole-scene bbox gate.
- **H7 — the invariants**: M1 immutable, M2 byte-identical, the census `60/80`, the live-chain rule (no staged gates), the trace default-off.

---

## 3. Evidence

### 3.1 The harness as it stands (fresh lines; the phase-30 T1 harness)

| piece | fresh site | what it gives today |
|---|---|---|
| the trace sink | `fifa96_match_run.c:1839-1855` (open), `:2638` (called at the end of `begin`), `:4674` (close at `_end`), `:1860` (close at `_teardown`) | `FIFA96_CAMERA_TRACE=<path>`, `"w"`, 1 header + one line per granted frame, `fflush` per line; default off |
| the trace line | `match_run.c:4586-4608` (loop tail), `follow_ran` saved at `:4426` | the 16 fields `frame clock phase pos vel spd fspd ev cur trk pan tim ramp rate follow tick` (the frame field prints `f`) |
| the frame body | `match_run.c:4362` (entry), `:4388` (grant loop), `:4403` (slot), `:4413-4416` (camera walk), `:4426` (follow), `:4443-4445` (OOB→armer), `:4471` (entity chain) | the order the harness relies on: an event set in the entity chain of frame *f* moves the place in *f+1* |
| the natural pass (control) | `tests/test_engine_m2.c:3521-3621`; asserts `:3593-3618` | 3 598 grants / 101 s / zero 3598/3598 / ladder 6 118 calls, `ran=0`, `lane_x=2541` |
| the P10 self-check | `test_engine_m2.c:3741-3789`; called `:3814-3815` | one traced natural pass, 1 header + 3 598 body lines, contiguous `f=`, `pos` zero |
| the scripted pan witness | `test_engine_m2.c:3632-3730` (asserts `:3713-3727`) | `0→0x90→0x120`, tracked 9, 358 grants, Y=0 (the `begin_isolated` fixture path) |
| the probe entry | `test_engine_m2.c:3809-3817`, report `:5246-5277` | P1–P5 in both ISO modes; P6–P10 ISO-gated (the pan witness is ISO-only) |

Measured fresh this session (`./build/test_engine_m2`, rc=0): the natural probe line reads *match start 5, kickoff 12, act-1 1679, phase 2 1802, 3 598 granted frames (101 s), camera zero 3598/3598, ladder 6 118 calls / ran=0, cam=(0,0,0), lane_x=2541*; the pan witness line reads *place 0 → 0x90 → 0x120 at steps 425/605 (first motion 339, tracked 9), Y=0, 358 granted frames*. Stdout `cmp`-identical to `tests/golden/engine/m2-frames.txt` (`8434cfab…`, 295 087 B); M1 `09b726b7…`.

### 3.2 The natural-route drive — the measurement

The probe-local tape is `M2_KEYS` **verbatim** (`test_engine_m2.c:1948-2118`) extended to 12 000 entries; the only added inputs are the direction hold. Runs from `/tmp/opencode/fu317_probe2.c` (the direction sweep) and `/tmp/opencode/fu317_probe3.c` (the timing / no-`LONG_BALL` variants), same null backend/ISO/`step_ns = 10 ms` as the test:

| drive (after phase 2) | claim frames | row-04 frames | pan | place | tracked | result |
|---|---|---|---|---|---|---|
| none (the control) | 0 | 3 060 | 0 | `(0,0,0)` | −1 | the phase-30 zero baseline |
| **DOWN 1820…end** | **2 084** | 976 | **1 segment, 27 frames** | **`(0,0,0)→(−0x90,0,0)`** | **2** | **the natural claim pan** |
| DOWN 2100…end | 2 054 | — | 1, 27 frames | `→(−0x90,0,0)` | 2 | the claim follows the drive start (`f = 1544`) |
| UP / LEFT / RIGHT 1820…end | 0 | 3 060 | 0 | `(0,0,0)` | −1 | inert (identical to the control) |
| DOWN 1810…end | 2 084 | — | 1, 27 frames | `→(−0x90,0,0)` | 2 | index 1810 also safe |
| DOWN 1820, **no** `LONG_BALL` | 2 084 | — | 1, 27 frames | `→(−0x90,0,0)` | 2 | the `LONG_BALL` press at 1810..1813 is **inert** |

The row-04 census was sampled only on the probe2 sweep (`—` = not sampled); the claim/pan columns are from probe3's variants.

Two more controls: the keeper-ladder seam removed reproduces the identical claim/pan (`claim f=1514`, motion `1514..1540`, `max|x|=144`, `panctr=4`, `trk=2`), and the drive's ladder counters are **6 118 calls / `ran=0`**, identical to the control — the pan does not wake the stage-select (the AX prepass still needs `|cam.z| > 0x7B0`).

### 3.3 The claim chain, frame by frame (native fresh, port fresh)

Native (fresh `disassemble_bytes` this session, `/FIFA96.EXE`):

```
0x7ED89  MOV EAX,0x40
0x7ED8E  MOV BX,word[EBP+0x81]      ; timer81
0x7ED95  MOV CX,word[EBP+0x6B]      ; lane
0x7ED99  SUB EAX,EBX
0x7ED9B  CMP AX,CX
0x7ED9E  JGE 0x7EDCE                ; (0x40 - timer81) >= lane -> bound tail
0x7EDA0  CMP EBP,[0x1577CA]         ; the tracked record
0x7EDA6  JZ  0x7F138                ; tracked -> return (the exclusion)
0x7EDB2  CMP EAX,0x90
0x7EDB7  JGE 0x7F138                ; lane >= 0x90 -> return
0x7EDCE  CMP CX,word[EBP+0x77]      ; bound
0x7EDD2  JG  0x7F138                ; lane > bound -> return
0x7EDD8  CMP dword[0x157750],0x50    ; camY
0x7EDDF  JLE 0x7EE4B                ; camY <= 0x50 -> half line
0x7EFEE  CMP EBP,[0x1577CA]          ; the 0x1D arm's own exclusion
0x7EFF4  JZ  0x7F0D6
0x7EFFA  CMP dword[0x157750],0       ; ball height == 0
0x7F001  JZ  0x7F035                 ; -> the has_slot seed
0x7F035  MOV ESI,dword[EBP+0x20]     ; the slot sub-object
0x7F03C  MOVSX AX,byte[ESI+0x20]     ; slot dir_x
0x7F041  SHL EAX,0x6
0x7F04C  MOVSX AX,byte[EAX+0x21]     ; slot dir_z
0x7F051  SHL EAX,0x6
0x7F0C8  CALL 0x92820                ; the event staging
0x7F0D1  CALL 0x71C94                ; the camera event setter (the 0x1D arm)
```

Port mirror: the gate `fifa96_outfield.c:610-618` (`0x7ED89..0x7EDDF`), the angle tail `:626-642`, the 0x13 arm `:670-681`, the 0x1D arm `:682-707`, the row-05 claim install `:719`; the row-04 state build `fifa96_match_handlers.c:2743, 2864-2892`; the event set + tracked bind `handlers.c:2777-2788`; the row-05 possession arm `fifa96_action_handlers.c:439-460` and the claim cells written at `fifa96_match_handlers.c:2908-2918`; the setter/ramp `fifa96_camera.c:920-956`; the integrator `camera.c:97-111`; the pan step/counter `camera.c:970-990`.

The measured chain (probe samples, the drive's `f`):

| f | record 2 `code` | `lane_x` | `bound` | `timer81` | `has_slot` | `dir` | camera | reading |
|---|---|---|---|---|---|---|---|---|
| 1507 | 4 | 10 | 12 | 0 | 1 | (−1,0) | `(0,0,0)`, `trk=−1` | row 04, walking the near field (record pos `(10,0,0)`) |
| 1511 | 4 | 2 | 4 | 0 | 1 | (−1,0) | `(0,0,0)` | approaching (record pos `(4,0,0)`; bearing `0xFF00`, face `0x100`, Δ `0x200` — the angle fails) |
| **1512** | **4** | **0** | **2** | **0** | **1** | **(−1,0)** | `(0,0,0)` | **the admission: `0x40−0 ≥ 0`, `0 < 0x90`, `0 ≤ 2`, camY `0 ≤ 0x50`; the angle passes at the zero-delta point (`lane_z = cam_dz6f = 0` → bearing `0x80`, Δ `0x80`)** |
| **1513** | **5** | 2 | 0 | 0 | 1 | (−1,0) | `vel=(−5,0)`, `trk=2`, `tim=0/12` | **the 0x1D event (`slot_dir_x<<6 = −0x40`) and the tracked bind; the row-05 install drained (`code 5`)** |
| **1514** | 5 | 6 | 2 | 0 | 1 | (−1,0) | `pos=(−10,0,0)`, `trk=2` | **the claim (`carrier=2`) and the first motion (the event is integrated in `f+1`)** |
| 1540 | 5 | — | — | — | — | — | `pos=(−0x90,0,0)`, `vel=(−1,0)`, `pan=3` | the last motion frame |
| 1541 | 5 | — | — | — | — | — | `pos=(−0x90,0,0)`, `vel=0`, `pan=4` | the stop |

The pan's decay is exactly the FU157 §3.4 signature: `vel = seed/F8 = (slot_dir_x<<6)/0xC = −64/12 = −5`, then the `0x70544` slow path with `k = pan_rate_lo = 24` (`camera.c:770-777`) gives `−5 → −3 → −2 → −1 → 0`; the counter increments once per pan step (`camera.c:970`), reaching 4.

*Face operand note:* the table's `face7d = 0x100` is the entity pool cell; the row-04 handler stages `s.face_word7d = 0` (`handlers.c:2646`, OL-72 / FU314-L2). At the zero-delta point both operands give Δ `0x80` (`0x80 − 0` and `0x80 − 0x100` fold to `0x80`), so the pass does not depend on the missing staging; at the approach frames both give Δ `0x200`. The staging fix is still required for a face-aligned route (FU314-L2).

### 3.4 The gate must not be re-derived in the test

A test-side algebra of `0x7ED9B/0x7EDCE/0x7EDD8` (`(0x40−timer81) ≥ lane`, `lane < 0x90`, `lane ≤ bound`, `camY ≤ 0x50`) is **true on 2 082 granted frames of the undriven control** — which never pans — and on only 33 frames of the drive. The missing terms are the body's earlier arms (`is_ball_track`/`ecx`/`track_577f0`, the face-angle tail `:629-635`, the 0x13-vs-0x1D branch) and the one-frame lane/bound ordering inside the entity walk (FU314's measured note; the body's operands are the previous tail's cells). Consequence, binding on the harness: **count the pan from the camera cells (delta runs) and snapshot the tracked record's cells at the gate frame; never assert a re-derived algebra.** The 2 082-vs-33 gap is the evidence.

### 3.5 The input model and the drive's tape shape

- `fifa96_match_run_input` (`match_run.c:436-455`) consumes **press events only** (`state != 1` skipped) and maps raw→FU-61: arrows = directions, KICK `0x10`, PASS `0x20`, LONG_BALL `0x40` (`:423-434`). A held direction is therefore one `state = 1` entry per engine step (the null tape: `platform_null.c:25-34`, one entry per poll; the SDL3 backend maps the same codes at `platform_sdl3.c:144-154`: arrows, `Z`=KICK, `C`=PASS, `V`=LONG_BALL, RETURN/BACKSPACE).
- The match branch polls once per engine step and feeds the run (`fifa96_engine.c:211-240`), so `fifa96_engine_step × N` with a tape is `make game` minus the SDL3 present.
- **The safe append rule (measured):** entries from index **1810** or **1820** preserve phase 2 at 1802; the FU313-L2 datum shows a held tail from index **1800** suppresses phase 2 entirely. The drive must start after the `M2_KEYS` end (1799) *and* past the phase-2 poll; 1810 is the measured-safe boundary (1820 is the pinned witness value). The claim frame follows the start (+30 frames when the hold starts 280 entries later).
- The `LONG_BALL` press at 1810..1813 is **inert** (identical run without it): the FU157 §3.4 long-ball claim is *not* the natural route's claim. The natural claim is the walk-onto-ball row-05 possession claim; do not carry a dead input.
- Direction mapping: the DOWN key yields `slot_dir_x = −1` (a 90° pad→world rotation), so the pan is X-**negative**. The harness records `dir_x/dir_z`; it must not assume UP→+x.

### 3.6 The trace, and why it is the primary Layer-2 artefact

Fresh traced drive (`FIFA96_CAMERA_TRACE=/tmp/opencode/fu317_drive.trace`): 3 599 lines (1 header + 3 598 body), `grep -c 'follow=1'` = **0**, and the pan lines are e.g.

```
f=1512 clock=00:32 ph=2 pos=0,0,0 vel=0,0 spd=0 fspd=0 ev=0x0 cur=0x0 trk=-1 pan=0 tim=0/0 ramp=0 rate=0,0 follow=0 tick=5044
f=1513 clock=00:32 ph=2 pos=0,0,0 vel=-5,0 spd=5 fspd=0 ev=0x0 cur=0x0 trk=2 pan=0 tim=0/12 ramp=12 rate=0,0 follow=0 tick=5047
f=1514 clock=00:32 ph=2 pos=-10,0,0 vel=-5,0 spd=5 fspd=5 ev=0x0 cur=0x0 trk=2 pan=0 tim=2/12 ramp=12 rate=0,0 follow=0 tick=5050
…
f=1540 clock=00:33 ph=2 pos=-144,0,0 vel=-1,0 spd=1 fspd=1 ev=0x0 cur=0x0 trk=2 pan=3 tim=12/12 ramp=12 rate=0,0 follow=0 tick=5137
f=1541 clock=00:33 ph=2 pos=-144,0,0 vel=0,0 spd=0 fspd=0 ev=0x0 cur=0x0 trk=2 pan=4 tim=0/12 ramp=12 rate=0,0 follow=0 tick=5140
```

The trace already carries every attribution cell the pan needs (`pos`, `vel`, `spd/fspd`, `ev`, `trk`, `pan`, `tim`, `ramp`, `follow`, `clock`); the probe must only add the **claim** and **gate-frame** cells, which the trace does not carry.

### 3.7 Visible feasibility (the Layer-2 datum)

The pan moves the camera 0x90 = 144 world units in X. Projecting two fixed world points through the engine's own chain (`fifa96_projection_matrix`/`render_camera_stage`/`fifa96_projection_project`; probe `/tmp/opencode/fu317_probe5.c`, yaw 49052, pitch 200, ratio 5376, 320×240):

| world point | screen_x at `pos=(0,0,0)` | screen_x at `pos=(−0x90,0,0)` | shift |
|---|---|---|---|
| `(0x300,0,0x300)` | −166.74 px | −115.04 px | **+51.7 px** (320 canvas) |
| `(0x300,0,−0x300)` | 473.81 px | 424.28 px | **+49.5 px** (320 canvas) |

≈ **50 px in the 320-wide canvas ≈ 150 px at the 960×720 window** — a visible whole-scene translation, so the FU313 bbox gate (spans the scene band) is the right discriminator. This is a projection computation, not a capture; it is guidance for the first pair (leg FU317-L7).

---

### 3.8 Reconciled with the concurrent wave drafts

The three sibling drafts landed while this slice was measuring; their verdicts and this slice's measurements agree once the *drive* and the *pass geometry* are separated:

- **FU314** (`docs/ghidra/FU314_r2l6_pan_triggers.md` §6/§8) is **NO-GO on the undriven tape**: the only band record the 12 000-step tape produces approaches from the east with `face 0x100`, bearing `0x300`, Δ `0x200 > 0xAB`, in 2 081/2 081 dispatches — and it assigns the missing natural drive to **FU314-L8 (owner FU317)**. This slice **narrows FU314-L8, in the negative for the face side**: a DOWN hold from 1820 drives the controlled record **2** into the band and the pan fires, but only through the **zero-delta** point (`lane_z = cam_dz6f = 0` → bearing `0x80`, Δ `0x80`), i.e. the record runs exactly over the parked camera. The **face-aligned** band entry (the east→west pass with a live `+0x7D` near the bearing) is **not** produced; FU314-L1's "real" natural pan stays open (§7.9, leg FU317-L10).
- **FU315** (`docs/ghidra/FU315_r2l6_claim_chain.md` §4.3/§7) is **GO on the natural claim** and names the exact chain this slice measures: row 04 → code-5 install → row-05 `0x7F1FF` claim (`ball.carrier = 2`), with the natural probe's `tracked == -1` being a *drive* limitation. This slice confirms both (the claim at f 1514, `tracked = 2` at f 1513) and adds the camera consequence (the pan). FU315's unported `0x7F4E7` claim-tail bind is **not needed** for the pan (the pan binds the tracked cell at `handlers.c:2788`); it would be needed only for a claim-time tracked bind *on the claim frame*.
- **FU316** (`docs/ghidra/FU316_r2l6_pan_walk.md` §1) is **GO on the walk model, NO-GO on a natural armer reach**, with the per-pan lattice step `T = 0x90`. This slice measures exactly one `T = 0x90` pan (`|x| = 0x90`, armer 0) — the natural drive's one-shot, and the second pan remains FU316's re-arm question.

---

## 4. The probe extension — exact shape

### 4.1 Where it lives, and what it must not do

**Extend `tests/test_engine_m2.c`** with one new pass, `run_natural_claim_pass`, called from `run_natural_probe` (`:3809-3817`) **after** the P10 self-check and **before** the ISO-gated scripted witness; one new stderr line reports the drive's claim/pan/tracked summary (the existing report block, `:5246-5277`). No new binary, no CMake entry, no golden change (stderr/one temp file only); one extra 12 000-step run (~9 s measured; the test is 57 s today). It must **not**: set `mr->act.id`, call `fifa96_camera_*` setters, stage any record/camera cell, edit `M2_KEYS`, or touch `run_scenario`. It may install the keeper-ladder observation seam (measured neutral, §3.2) and set `FIFA96_CAMERA_TRACE` around its own run only (the P10 pattern, env restored).

### 4.2 The tape

```c
#define M2_CLAIM_STEPS      M2_NATURAL_WINDOW_STEPS       /* 12 000 */
#define M2_CLAIM_TAPE_LEN   (M2_CLAIM_STEPS + 1)
#define M2_CLAIM_DRIVE_FROM 1820                          /* measured-safe; 1810 also green */

static fifa96_platform_key tape[M2_CLAIM_TAPE_LEN];
memcpy(tape, M2_KEYS, sizeof M2_KEYS);                    /* the verbatim prefix */
for (int i = M2_CLAIM_DRIVE_FROM; i < M2_CLAIM_TAPE_LEN; i++) {
  tape[i].raw_code = FIFA96_ENGINE_KEY_DOWN;              /* the natural claim drive */
  tape[i].state = 1;                                      /* a held direction */
}
pcfg.tape = tape; pcfg.tape_len = M2_CLAIM_TAPE_LEN;
```

No `LONG_BALL`, no button press (measured inert, §3.2). The prefix equality is itself an assert (`memcmp(tape, M2_KEYS, sizeof M2_KEYS) == 0`) so a future tape edit cannot drift the witness.

### 4.3 The sampler (per granted frame, trace-relative `f`)

One struct `m2_claim` (the `m2_pan` pattern, `:2327-2341`) plus a sampler that records:

1. the **16 trace cells + `follow`** (the `m2_natural_sample` shape, `:3476-3515`) — so P16 compares in-memory vs the file;
2. the **claim**: `mr->entities.ball.carrier`, `pair.code/ack`, `mr->slot.entity`; the first `carrier >= 0` frame and its entity;
3. the **pan segmentation by delta**, not by off-zero: `changed = pos != prev_pos`; a segment is a maximal changed run; record `first_motion_f`, `stop_f`, the stop place, `pan_counter` max, monotone/non-monotone counts;
4. the **gate snapshot** (a rolling two-frame ring) at `first_motion_f − 2` and `first_motion_f − 1` (the gate frame and the event frame, §3.3): the tracked record's `code/lane_x/bound/timer81/has_slot/dir_x/dir_z/pos_y`;
5. the **armer**: `mr->goal_armed`, `goal_zone`, and `fifa96_camera_out_of_bounds(pos_x,pos_z)` per frame; the `|pos|` maxima;
6. the **AI/background census**: the nearest `code == 4` record over both teams per frame (`id`, `lane_x`, `timer81`, `bound`) — recorded, not asserted; the undriven control keeps its nearest at record 12, `lane 4`, and never pans (leg FU317-L6);
7. the ladder counters (the existing seam) — recorded, not asserted beyond the control's equality.

### 4.4 The assertions (P11–P18); the measured pins in **bold**

| # | assertion | basis |
|---|---|---|
| **P11** | the drive reaches phase 2 at **1802** (recorded, not hard-coded) and a **claim**: `ball.carrier == mr.slot.entity == 2` at **f = 1514**; `pair.code == 0 && pair.ack == 0` (the walk claim, not the pass pair) | spec §5 L1; measured |
| **P12** | the pan: **exactly one delta segment**, **f = 1514 … 1540** (**27** changed frames), monotone, stop place **`(−0x90,0,0)`**, no return to zero; `pan_counter` **0 → 4**; `speed` **5→3→2→1→0**; `|y| = |z| = 0` | the FU157/FU310 decay carried; measured |
| **P13** | the tracked/gate state: `tracked == 2` at the event frame (**f = 1513**) and to the end; the gate tail (**f = 1512**) record 2 `code == 4`, `lane_x == 0`, `bound == 2`, `timer81 == 0`, `has_slot == 1`, `dir = (−1,0)`, `pos_y == 0`, and the angle terms `lane_z == 0`, `cam_dz6f == 0`, `face7d == 0x100` (bearing `0x80`, Δ `0x80` — the zero-delta pass); the event frame `code == 5` (the row-05 install) | §3.3; the port lines `outfield.c:610-719`, `handlers.c:2777-2788` |
| **P14** | the armer/OOB: `fifa96_camera_out_of_bounds == 0` on every frame (so `fifa96_match_goal_arm` is never called, `match_run.c:4443-4445`) and `goal_armed == 0`; `|x| = 0x90`; the scanner therefore never runs | the phase-30 armer chain (carried); the honest limit |
| **P15** | the **control** (the unchanged `run_natural_pass`) still has `camera_left_zero == 0` / 3 598 of 3 598 zero and ladder 6 118/`ran=0` — so the pan is the drive's, not the tape's | `:3603-3618` |
| **P16** | the trace cross-check: the drive pass with `FIFA96_CAMERA_TRACE` yields **1 header + 3 598 body lines**, contiguous run-relative `f=`, `ph=` on every line, the delta run **f = 1514 … 1540** == the in-memory segment, `trk=2` on the pan lines, `pan=0..4`, `ev=0x0`, and **`follow=0` on every line** | §3.6; the FU313 §4.3 contract |
| **P17** | determinism/pin: the measured pins above are exact values over the deterministic null backend (the drive is one run; the control's existing 3-replay gate `:5218-5237` and P5's two-pass `memcmp` `:3810-3813` pin the engine). A second drive replay is optional (+9 s) and only needed if Stage-2 touches the input path | the phase-30 measured-pin style |
| **P18** | the fixture + riders (§4.5) stay GREEN, each rider RED | FU157 §9 / FU309 §5.4 carried |

**ISO gating:** P11–P18 are ISO-dependent (the formation seed places record 2 and the ball) and go behind the existing `with_iso` gate, like the scripted witness; P15 holds in both modes.

**RED-first staging:** unlike phase-30, the pan assertions are **GREEN today** and land in the same commit as the harness; the RED comes from the riders (§4.5). If the wave's Stage-2 changes the claim producer, P11–P13 are the pins that move first.

### 4.5 The fixture and the mutation-RED riders

The fixture is the existing `test_engine_match_frame.c::test_live_claim_pan_row04_half_line` (`:2610-2679`, called `:10102`): the real frame path, `begin_isolated` (`:157-160`, i.e. `begin` + `mr->act.id = 0` — a **fixture** allowance, never the natural witness), KICK → phase 2 → LONG_BALL → UP; it already pins `ball.carrier == id` (`:2647`), `tracked == id` (`:2652`), `vel_x == 5`, `event_param == 0`, the decay and the `0x90` stop (`:2656-2675`). Phase-31 adds the **second-pan/re-arm** rider from FU316's verdict (the scripted witness's `0x120` second stop is the current two-pan datum, `test_engine_m2.c:3717`).

One-line riders, each must turn P11–P13 (or the fixture) RED:

| producer | rider site (fresh) |
|---|---|
| the row-04 half-line gate | `fifa96_outfield.c:610` (`if (0 && …)`), `:616-618` |
| the 0x1D arm's tracked exclusion | `fifa96_outfield.c:682` |
| the `has_slot` seed | `fifa96_outfield.c:689-691` |
| the row-05 claim install | `fifa96_outfield.c:719` |
| the event set / the tracked bind | `fifa96_match_handlers.c:2777`, `:2788` |
| the setter's fast-path seed | `fifa96_camera.c:770` |
| the pan step / counter | `fifa96_camera.c:970`, `:990` |
| the integrator | `fifa96_camera.c:110-111` |
| the follow height gate (P16's `follow=0`) | `fifa96_match_run.c:3976` (the `> 0x10` gate that refuses the pan's `ev=0`); `:3974` is the pan-lockout, which refuses from f 1520 on |

Each is a manual reversion in a scratch worktree (the FU309-L9 manual precedent; automation stays out of scope).

### 4.6 Cost

| piece | cost | default |
|---|---|---|
| `run_natural_claim_pass` + `m2_claim` + the sampler + P11–P17 | ~200 LOC, one file, one 12 000-step run (+~9 s) | runs with the existing test, ISO-gated |
| the trace cross-check | 0 new code beyond the sampler (the env/P10 pattern) | off unless the pass sets it |
| the fixture riders | 9 one-line reversions, manual | out of CI |
| Layer-2 | the existing trace + `import`/`convert`/`compare`/`ffmpeg` | off unless asked |

---

## 5. The Layer-2 capture (same-clock pairs, the trace primary, the AE relative)

### 5.1 Run and walkthrough

```
export DISPLAY=:1
FIFA96_CAMERA_TRACE=/tmp/opencode/p31-cam-trace.log make game     # Makefile:39-41
```

Walkthrough (the phase-8/29/30 precedent): intro RETURN skip → front-end BACKSPACE (panel) → RETURN (panel confirm → match) → the kickoff KICK press/release (clock starts) → wait for `ph=2` in the trace → **hold DOWN** until the trace shows `trk=2` and `pos` moving (the null run: `clock=00:32`; the live timing differs because the M2 tape's early RIGHT/LEFT holds are not reproducible by hand — leg FU317-L9).

### 5.2 The pan window

The pan is **27 frames ≈ 0.9 s** (the null run `f=1514..1540`, `clock=00:32..00:33`). Two routes, in order of preference:

1. **The clip fallback (robust):** `ffmpeg -f x11grab -framerate 60 -video_size 960x720 -i :1+0,0 -t 4 /tmp/opencode/p31-pan.mkv` started just before the trace's first motion, then extract two frames **inside one HUD second** and inside the pan.
2. **Two `import` calls** back-to-back (`< 0.3 s` apart) as soon as `tail -f` shows `trk=2`; verify both PNGs carry the same `MM:SS`.

The trace is the **only** artefact that attributes the motion; the capture pair must lie inside the trace's delta run (`pos` changing, `trk=2`), never outside it.

### 5.3 The same-clock pairs and the AE gate

```
WID=$(xdotool search --name '^FIFA 96$' | head -1)
import -window "$WID" /tmp/opencode/p31-pan-a.png        # both inside the pan, one HUD second
import -window "$WID" /tmp/opencode/p31-pan-b.png
# the still control: no key held, a second with pos constant, same window/crop
import -window "$WID" /tmp/opencode/p31-still-a.png
import -window "$WID" /tmp/opencode/p31-still-b.png
for p in pan-a pan-b still-a still-b; do
  convert /tmp/opencode/p31-$p.png -crop 960x591+0+0 +repage /tmp/opencode/p31-$p-c.png
done
compare -metric AE /tmp/opencode/p31-pan-a-c.png /tmp/opencode/p31-pan-b-c.png null: 2>&1
compare -metric AE /tmp/opencode/p31-still-a-c.png /tmp/opencode/p31-still-b-c.png null: 2>&1
compare -compose src /tmp/opencode/p31-pan-a-c.png /tmp/opencode/p31-pan-b-c.png /tmp/opencode/p31-diff.png
convert /tmp/opencode/p31-diff.png -format '%@' info:      # the bbox
```

- **Gate 1 (relative, binding):** `AE_pan > AE_still`; both numbers recorded with the pair's clock second and the trace interval. The still control is **re-measured in the same session/build** (FU309-L6; there is no absolute floor).
- **Gate 2 (the whole-scene bbox, FU313 §6.5, still proposed):** the diff bbox must span the scene band (`w ≥ 864`, `h ≥ 443`); a band-limited diff is entity motion, never a pan. The projection datum (§3.7, ≈50 px / 320) says the pan should span it; the first pair re-pins the threshold (FU313-L4).
- **Guidance, not a floor:** the phase-9 whole-scene landings sat at ~2.9e4–4.6e4; a pan AE in the low thousands is a fail signal to investigate.

### 5.4 Attribution rules (binding)

1. The **trace** attributes; the AE corroborates (spec §5; FU309-L5 closed).
2. The trace and the captures are the **same run**.
3. Same-clock pairs only; the pan pair must lie inside the trace's delta run with `trk=2`.
4. The still control is re-measured in-session; no absolute floor.
5. A band-limited bbox is entity motion, not a pan.
6. No golden re-pin for a capture; a live path that moves M1/M2 stops the task for the written diff.
7. If the trace shows no delta run, Layer-2 is a numbered waiver with the trace as the record — never a widened fixture.

---

## 6. The acceptance matrix

| link (spec §5) | artefact | today |
|---|---|---|
| **H1** natural witness, no staging | the new `run_natural_claim_pass` (P11/P12/P15), `test_engine_m2` (ISO) | **GREEN measured** (claim f=1514, one pan to −0x90) |
| **H2** the claim state | P11 (`ball.carrier`, `pair.code/ack`, `slot.entity`) | GREEN (`2/0/0`), the pair cells 0 (FU317-L4) |
| **H3** the pan | P12 (delta segments, place, decay, pan counter) | GREEN (1 segment, 27 frames, −0x90, pan 4) |
| **H4** tracked / gate / armer | P13/P14 (+ the trace `trk`/`pan`/`ev`/`follow`) | GREEN (tracked 2; armer 0; follow 0) |
| **H5** the frame-path fixture + riders | `test_live_claim_pan_row04_half_line` (`:2610`) + §4.5 | GREEN (one pan); the second-pan rider FU316 |
| **H6** the visible pan | `make game` + trace primary (§5.1-5.2) + the same-clock pairs + the relative AE + the bbox | procedure frozen; the first capture at the acceptance |
| **H7** invariants | M1 `cmp` `09b726b7…`; M2 `cmp` `8434cfab…`; the trace default-off (M2 stdout identical with the env set); census `60/80`; the live-chain rule (P3/P15); the M2 3-replay gate `:5218-5237` | M1/M2 verified this session; census untouched (no row wired) |

The phase-30 labels map as: **P6** (the camera leaves zero) is carried by P11/P12 and is no longer partial; **P7** (a sustained follow window) and **P8** (the lockout) stay **RED by design** (no `>0xF0` event; `follow=0` on every frame); **P9** is recorded (stage-select `ran=0`, 6 118 calls, unchanged); **P10** is asserted and extended to the drive by P16.

---

## 7. The honest limits

1. **The follow / Y stays waived.** The pan's event height is 0 (`ev=0x0`), so the setter's idle arm seeds a place-only pan; `follow = 0` on all 3 598 frames, the call gate (`event_param > 0x10` → 0, `match_run.c:3976`) and the arm gate (`> 0xF0`) never open, and `pan_counter != 0` short-circuits the follow (`:3974`). The Y writers are the phase-30 circular route (carried, no change). P7/P8 stay RED by design; the spec §8.2 waiver stands.
2. **The reach is one pan, `0x90`.** The natural drive claims once (record 2), the same record becomes `tracked`, and the 0x1D arm's own exclusion (`0x7EFEE`, `outfield.c:682`) refuses it afterwards — measured as `pan_segments == 1` to the window end. The `0x120` second pan exists only on the scripted witness (a *different* claimant, record 10). Re-arm/repeatability is FU316's.
3. **The armer is unreachable from this witness.** `|x| = 0x90` is below the OOB call site (`0x6C0`) and the armer (`0x730`); `goal_armed = 0` on every frame. The scanner/set-piece consequence is measured as 0, not promised (spec §1, §7).
4. **The claim is the row-05 possession block, not the pass/receive chain.** `pair.code/ack = 0/0`; the claim cell is `ball.carrier` set by `fifa96_action_carrier_arm` (`action_handlers.c:439-460`), gated by `lane <= 0x40` (`:473-477`). The P3 possession/pair producer remains (FU315).
5. **The AI does not produce the pan.** The undriven control's nearest row-04 record is record 12 at `lane 4` and it never pans; the pan needs the controlled record's row-04 → row-05 install. An AI-only claim/pan is unproven (FU317-L6).
6. **The drive is timing-coupled.** The claim frame follows the drive start (1820 → f 1514; 2100 → f 1544); a hold from 1800 suppresses phase 2 (FU313-L2). The witness pins the start index; it is not a free-running "play".
7. **The AE cannot attribute.** The visible shift is computed (~50 px/320) but the first pair must confirm it; the trace remains primary, the AE relative (FU309-L6 / FU313-L4/L5).
8. **The interactive timing differs from the null run** (the tape's early holds are not reproducible by hand); the live capture keys on the trace's first motion, and the 0.9 s window is tight (the clip fallback).
9. **The angle pass is the zero-delta degenerate point, not a face alignment.** The admission's angle tail (`outfield.c:629-635`) passes here because the claimant crosses the parked camera exactly (`lane_z = cam_dz6f = 0` → bearing `0x80`, Δ `0x80`); the approach frames are `bearing 0xFF00`, Δ `0x200 > 0xAB`. The witness is a **knife-edge**: any Stage-2 change to the walk step/axis that makes the crossing inexact drops the bearing to ±0x100 and the pan with it. The face-aligned natural route (FU314 R2/R1) stays FU314-L1's; the harness pins the pass terms (P13) so the mechanism is never invisible.

---

## 8. Numbered legs

1. **FU317-L1 — the drive-start boundary.** Measured safe: 1810 and 1820 preserve phase 2 at 1802 and produce the pan; 1800 suppresses phase 2 (the FU313-L2 datum). The 1801..1809 window is unmeasured; index 1800's cause is still FU313-L2's (candidate: the row-01 kickoff release gate reading the slot edge). Owner: the probe's tape builder + `fifa96_match_run_input` (`match_run.c:436-455`) / row 01.
2. **FU317-L2 — the gate census must not be re-derived.** The algebraic half-line predicate is true on 2 082 undriven frames (no pan) vs 33 driven frames (one pan); the body's earlier arms and the lane/bound ordering are not reproducible test-side. The harness counts delta runs and snapshots the gate frame. Owner: the probe (test-side) + FU314's trigger census.
3. **FU317-L3 — the natural pan is one-shot at `0x90`.** The tracked exclusion (`outfield.c:682`) blocks the claimant; the second pan (`0x120`) is only the scripted witness's second claimant. The re-arm/repeatability is FU316's. Owner: FU316.
4. **FU317-L4 — the claim is the possession block, not the pair.** `pair.code/ack = 0/0`; the pass/receive possession producer (the P3 cell) is not exercised. Owner: FU315.
5. **FU317-L5 — the pad→world axis mapping.** DOWN yields `slot_dir_x = −1` (a 90° rotation); the pan is X-negative. The harness records the slot dirs; the input row/slot map owns the mapping. Owner: `fifa96_match_run_input` / the slot map (`match_run.c:423-455`).
6. **FU317-L6 — the AI-only claim/pan.** The undriven control's nearest row-04 record (record 12, `lane 4`) never pans; the AI's claim path is unproven in the window. Owner: FU314/FU315.
7. **FU317-L7 — the visible shift is computed, not captured.** ~51.7 px / 49.5 px (320 canvas) is a projection of two fixed points, not a pixel measurement; the first live pan pair must confirm it and re-pin the FU313-L4 bbox threshold. Owner: the Layer-2 acceptance task.
8. **FU317-L8 — the probe cost.** +1 run ≈ +9 s (57 s → ~66 s); a direction sweep would multiply it. The harness pins one drive (DOWN) and keeps the other directions as scratch measurements. Owner: the acceptance task (`test_engine_m2`).
9. **FU317-L9 — the interactive claim timing.** The M2 tape's early RIGHT/LEFT holds are not reproducible by hand, so the live claim second differs from the null run's `00:32`; the capture must key on the trace's first motion, and the pan window is 0.9 s (the clip fallback). Owner: the Layer-2 acceptance task.
10. **FU317-L10 — the angle pass is degenerate (a zero-delta crossing, not a face alignment).** Measured: the passing tail has `lane_z = cam_dz6f = 0` → bearing `0x80`, face `0x100`, Δ `0x80`; the approach frames carry `bearing 0xFF00`, Δ `0x200`. A Step-2 walk change that moves the crossing off exact `(0,0)` (a different step size, a z-offset, a side-flip camera) drops the pan. The face-aligned route (a live `+0x7D` near the bearing, FU314 R1/R2) is still unproven; FU314-L2's staging (`s.face_word7d = e->face7d`) is the prerequisite for it to be provable. Owner: FU314-L1/L2 + the Stage-2 drive's walk arithmetic.

**Settled legs (this slice):** **FU309-L5 — settled/closed** (the trace landed, P10 asserted, the fresh traced drive proves 1 + 3 598 lines and the pan; register `docs/ENGINE.md:5647-5650`, `:5777-5791`). **FU309-L6 — closed as a procedure** (the relative gate + the same-session re-measure + the bbox; no change). **FU313-L1 — settled** (the producer is the scripted direction hold; the natural pan is measured, so the camera assertions land GREEN, not RED-first; the riders provide the RED). **FU313-L2 — bounded** (the ≥1810 rule; the cause stays open, now FU317-L1). **FU313-L3 — closed for the harness** (the probe carries both counters; the drive keeps 6 118/`ran=0`). **FU313-L4 / FU313-L5 — still open** (the bbox threshold and the first live pan capture; the §3.7 datum is guidance only). **FU314-L8 — narrowed in the negative for the face side** (the DOWN drive reaches the band and pans, but only through the zero-delta point, FU317-L10; FU314-L1's face-aligned route stays open). **FU315's GO and FU316's `T = 0x90` model — confirmed** by the measurements above.

---

## 9. GO/NO-GO

**GO — the harness.** The exact natural chain, measured end to end on the current tree:

> the verbatim M2 tape (front-end → match start **5** → situation-0 row `0x24` at 12 → act 1 at 1 679 → **phase 2 at 1 802**) → the probe-local **DOWN hold from tape index 1820** → the controlled record **2** (slot 2) runs row 04 → its lane reaches `0x40 − timer81` (`timer81 = 0`; measured 10 → 0 at f 1507..1512) → the half-line gate (`0x7ED89..0x7EDDF` = `outfield.c:610-618`: `0x40−0 ≥ 0`, `0 < 0x90`, `0 ≤ bound 2`, camY `0 ≤ 0x50`) → the angle tail (`:626-642`) passes at the **zero-delta point** (`lane_z = cam_dz6f = 0` → bearing `0x80`, Δ `0x80 ≤ 0xAB`; §7.9) → the **0x1D arm** (`0x7EFEE..0x7F0D1` = `:682-707`; tracked-exclusion off, ball height 0, `has_slot`, `slot_dir_x<<6 = −0x40`) → `fifa96_camera_event_set(…, height 0)` + `fifa96_camera_set_tracked(2)` (`handlers.c:2777-2788`) → the walk integrates (`camera.c:110-111`) with `F8 = 0xC` → `vel = −5`, the pan-step decay (`camera.c:970-990`) → place **`0 → −0x90`** over **27 frames** (f 1514..1540) → the row-05 install (`outfield.c:719`) claims (`ball.carrier = 2`, `handlers.c:2908-2918`) → the next pan is refused by the tracked exclusion. Y = 0, Z = 0, armer 0, `follow = 0`.

- **file/API:** only `tests/test_engine_m2.c` (the pass, the struct, the sampler, P11–P18). No production edit — the trace already exists (`match_run.c:1839-1855/2638/4586-4608/4674`). No new target, no CMake change, no golden change.
- **cost:** ~200 LOC + one 12 000-step run (~9 s); the fixture riders are 9 one-line manual reversions.
- **the seven links** are carried by the matrix (§6); H1–H5 are GREEN today, H6 is the frozen procedure, H7 holds.

**NO-GO only on** (a) re-deriving the row-04 gate algebra in the test (§3.4), (b) treating the AE as attribution (§5.4), (c) claiming the second pan or the follow from this witness (§7.1-7.2). None is part of the harness as specified.

---

## 10. Provenance

**Ghidra (fresh this session, `/FIFA96.EXE`, read-only, no scripts/saves):** `get_current_program_info` (`watcom:LE:32:default`, base 0, `.image`, 2708 functions); `disassemble_bytes` `0x7ED80` (0x70 B: the `0x7ED89..0x7EDDF` gate), `0x7EFE0` (0x40 B: the `0x7EFEE` tracked exclusion + the `0x7F035` has_slot branch), `0x7F030` (0x50 B: the `slot_dir << 6` seed), `0x7F0C0` (0x30 B: `0x92820`/`0x71C94`, the 0x1D sink). Native claims not quoted above are *carried*: FU157 §3.2/§3.4 (the row-04 gate and the claim pan), FU309 §2-§4 (the follow gates), FU310 §5/§6 + errata (the one-shot/ceiling), FU313 §4/§5/§6 (the trace/probe/capture), the phase-30 register (`docs/ENGINE.md:2455-2523`, `:5615-5650`, `:5777-5791`).

**Engine/test sources read (fresh):** `src/fifa96_engine/fifa96_match_run.c` (423-455, 1826-1875, 2638, 3198-3234, 3967-3992, 4362-4475, 4586-4608, 4674); `src/fifa96_engine/fifa96_match_handlers.c` (2650-2816, 2860-2918); `src/fifa96_engine/fifa96_engine.c` (195-247); `src/fifa96_engine/fifa96_match_entities.c` (735-870); `src/fifa96_engine/platform_null.c` (25-34); `src/fifa96_engine/platform_sdl3.c` (140-170); `src/fifa96_loader/fifa96_outfield.c` (392-721); `src/fifa96_loader/fifa96_camera.c` (60-180, 700-830, 920-999); `src/fifa96_loader/fifa96_action_handlers.c` (380-510); `src/fifa96_loader/fifa96_keeper_cpu.c` (27-41, 90-96, 236-265); `include/fifa96_engine/fifa96_match_entities.h` (60-180, 245-300); `include/fifa96_loader/fifa96_camera.h`; `include/fifa96_loader/fifa96_projection.h`; `tests/test_engine_m2.c` (1716-1745, 1850-1859, 2268-2360, 3440-3621, 3632-3817, 5239-5277); `tests/test_engine_match_frame.c` (130-160, 2590-2679, 10102); `Makefile` (32-41); `docs/ENGINE.md` (2455-2523, 5615-5650, 5777-5791).

**Measurements (this session):** `./build/test_engine_m2` (rc=0; the natural-probe and pan-witness stderr lines quoted in §3.1; stdout `cmp`-identical to `M2`; M1/M2 sha256 `09b726b7…a2cd` / `8434cfab…cee1`); `/tmp/opencode/fu317_probe.c` (the first drive sweep), `/tmp/opencode/fu317_probe2.c` (the direction sweep + the row-04 census), `/tmp/opencode/fu317_probe3.c` (the delta-run/claim detail), `/tmp/opencode/fu317_probe4.c` (the claim-frame cells), `/tmp/opencode/fu317_probe5.c` (the projection shift), `/tmp/opencode/fu317_probe6.c` (seam-neutrality), `/tmp/opencode/fu317_probe7.c` (the traced drive + `/tmp/opencode/fu317_drive.trace`), `/tmp/opencode/fu317_probe8.c` (the angle-gate terms); all linked against the existing `build/*.a` with the `test_engine_m2` link order, `-fsanitize=address,undefined`, run from the repo root with `ASAN_OPTIONS=detect_leaks=0`. The M2 test wall clock: 57.3 s; the 4-run probe: 36.5 s (≈9 s per 12 000-step run).

**Concurrent drafts read (this wave):** `docs/ghidra/FU314_r2l6_pan_triggers.md` §6/§8/§7 (the gate tuple, the 2 081 angle failures, FU314-L1/L8), `docs/ghidra/FU315_r2l6_claim_chain.md` §4.3/§7 (the claim chain, the `0x7F4E7` tail, FU315-L1), `docs/ghidra/FU316_r2l6_pan_walk.md` §1/§2 (the `T = 0x90` walk model, the armer NO-GO). They are Stage-1 drafts, not yet promoted; the reconciliation is §3.8.

**No writes:** no repo file created or modified except this draft; no Ghidra rename/comment/label/function/script/save; no git; `/FIFA96.EXE` and the ISO untouched; every scratch probe and trace lives under `/tmp/opencode/`.

## Review erratum (wave review, 2026-10-22)

**Quote (§1):** "the phase-31 Layer-1 witness is **GREEN on the current tree** ... P6 becomes fully GREEN through a natural-route pad input." **Fresh (this review):** the drive reproduces exactly: claim f=1514 (`ball.carrier=2`, `slot.entity=2`, `pair=0/0`), event f=1513 (`code=5`, `vel=−5`, `trk=2`), pan f=1514..1540 (27 frames), place `0→−0x90`, `pan 0→4`, Y/Z=0, armer/OOB=0, `follow=0` all 3,598; trace 1 header + 3,598 body lines; ladder 6,118/`ran=0`; one run 8.97 s. **But P6's original assertion on `run_natural_pass` stays 0/3,598** (`tests/test_engine_m2.c:3609-3611`): the driven pass carries the acceptance link; the undriven control must stay the zero baseline (P15). Do not relax `run_natural_pass`.

**Quote (§7.2):** "the same record becomes `tracked`, and the 0x1D arm's own exclusion (`0x7EFEE`) refuses it afterwards — measured as `pan_segments == 1`." **Fresh:** the proximate block is the **row-05 install**, not the tracked exclusion. `review_rec2` shows record 2 `code 4` at f=1512, `code 5` at f=1513, and **code 5 through f=12,000** (no return to row 04); the tracked cell is 2 throughout. The `0x7EFEE` exclusion is a second, latent guard for a record that re-enters row 04. Stage-2: keep the one-shot P12; attribute the stop to `outfield.c:719` / `handlers.c:2797-2810` (row 5) and use `0x7EFEE` only for a re-entry fixture.

**Quote (§3.3/P13):** "**the gate tail (f = 1512)** ... `face7d = 0x100` (bearing `0x80`, Δ `0x80` — the zero-delta pass)." **Fresh:** the pool `face7d` is 0x100, but the handler's operand is `s.face_word7d = 0` (`handlers.c:2646`); both give Δ`0x80` at zero delta. P13 must label the **staged operand** vs the pool cell; the face-staging rider does not turn P13 red at zero delta. Also **the panning record is not the controlled record**: at f=1512 `controlled=1`, `slot.entity=2`, `tracked=−1`; the port's `is_ball_track` fallback (`handlers.c:2672`) would have blocked the event if record 2 were `controlled`. The witness passes because `id=2 != controlled=1`. P13 must snapshot `controlled`/`slot.entity`; a different drive whose panning record is `controlled` would be refused (FU316-L3).

**Quote (§7.1):** "The claim ... fires the row-04 pan." **Fresh:** the order is inverted. The row-04 event is f=1513; the first motion and `ball.carrier=2` are f=1514. The pan's seed is the existing control-slot dir (`slot.entity=2`, `dir=(−1,0)`), not a new claim. The acceptance wording should be "a natural row-04 pan that also produces the claim", not "a claim that fires the pan". Stage-2: the claim chain is not a pan producer; FU315's possession slice is follow/claim-frame work, not a prerequisite.

**Quote (§3.4):** "the algebraic half-line predicate is true on 2,082 granted frames ... vs 33 driven frames." **Fresh:** `probe2` reproduces 2,082 control frames / 33 driven frames; FU314's 2,081 is the dispatch record-frame count, one counter-boundary apart. The warning (do not re-derive the gate in the test) stands.

**Stage-2 consequence:** the harness is GO; keep the control zero, add P11–P18 with the `controlled`/`slot.entity`/staged-face snapshots, and use the trace as primary. The zero-delta caveat is real: the pass is exact `(lane_x,lane_z)=(0,0)`; a nonzero z or a missed x=0 drops it. A west-side band entry (FU314 R2) or a live-face route is the robust follow-up.
