# FU-329 — the no-slot pan acceptance and golden governance (phase-34 R2-L8)

**Phase/Provenance:** M2 phase-34 Stage-1 recon-ahead wave, **Task 4 of 4**
(`docs/superpowers/plans/2026-10-23-fifa96-m2-phase34-recon-ahead.md:86-101`),
serving `docs/superpowers/specs/2026-10-23-fifa96-m2-phase34-noslot-pan-design.md`
§5 (Structure), §6 (Acceptance), §7 (honest-stop) and §8 (Decisions).
**Repo-static design: no Ghidra read, no probe, no build, no measurement.** Every
native fact is **carried** with its owner doc (FU316 §3.2/§4.3/§5.1/§7/§8,
FU321 §3–§6, FU323 §2/§4, FU319-L8) and every quantity the parallel slices own is
forward-cited as *the measured value* — the algebra (FU326), the distribution
(FU327), the witness (FU328) — never a number. Every port/test claim is a
first-hand `file:line` read this session (§12). Fresh this session: the M1/M2
`sha256sum`/`wc` pins (§6.4). The only repo write is this draft; no git; the ISO
untouched; no probe was run (the only scratch file is a table-escaping helper
under `/tmp/opencode/fu329/`).

---

## 1. The question

> Given the spec §5–§8 and the phases-32/33 acceptance precedents, what exactly
> does Stage 2 implement for the no-slot pan: the Layer-1 witness tuple, the
> recorded-vs-asserted split, the exact pins and the drive-naming grammar; the
> Layer-2 live design with its waivers; the golden governance if the witness
> moves M2 — or, if the wave finds no witness, the NO-GO record — while
> asserting no number FU326/FU327/FU328 own?

## 2. The one-page answer

**The design is GO in both branches; the acceptance is conditional on FU328's
measured tuple.** The witness is a row-04 arm-B pass on the **undriven natural
tape** whose record holds **no slot** (`has_slot 0`), whose `+0x73/+0x75` words
are non-zero, and whose event frame carries a **moving** camera seed
(`|v0| ≥ 1`), followed by a real delta run of `T(v0)` over the decay. It is a
**test-side** pin: the natural pass gains the two-frame gate ring and the
per-segment records the phase-32/33 driven passes already carry
(`tests/test_engine_m2.c:2442-2499`, `:4219-4257`), plus the seed/arm cells the
harness lacks (`+0x73/+0x75`). **No production change, therefore no golden
change:** the golden transcript is produced by `run_scenario`
(`tests/test_engine_m2.c:3714-3718`) — a different run from the natural probe
(`:5146-5161`) — so the acceptance **cannot** move M2 by construction, and the
byte-identical `cmp` is a gate, not a hope (Case A). The only M2 exposure is a
*seed-staging fidelity fix* if FU326's audit finds a measured divergence (spec §5
decision 5); that is a different, separately pinned commit under the
faithful-move recipe (§6.2), never folded into the acceptance.

The Layer-2 vehicle is the **undriven live run** (`make game`, the `M2_KEYS`
prefix replayed by trace tick, **nothing added**); the wall-clock scheduling rule
of FU321-L4 is **waived by construction** (no post-phase-2 drive exists), the
trace is primary, the AE is the same-run relative corroboration, the bbox stays
the sprite-only caveat, and each committed pair anchors to **one named segment**
(phase 33's lesson, not a mixed span). If the measured pan is too short for a
same-second pair, a declared pair-width waiver is recorded — or Layer 2 is
recorded NOT-GO-as-measured with the Layer-1 witness standing; it is **never**
replaced by the slot-arm FU320 drive, which is a different branch of the seed
switch and would silently change the phenomenon.

If FU328 returns NO-GO, the phase lands the **measured-distribution register**
(FU327's table + FU326's fidelity verdict + the reproduced phase-33 silent bind)
and the legs, with **no test change, no fixture tuning, and no golden change**
(§7). Both branches close with the same invariants: M1 `09b726b7…` immutable,
M2 `d5446144…` unmoved, census `60/80 = 75%`, `make check` green.

### 2.1 The two branches at a glance

| | Branch A — a witness is measured (FU328 GO) | Branch B — no witness (FU328 NO-GO) |
|---|---|---|
| Layer 1 | the natural pass extension + the witness pins (§4) | no test change; the distribution register (§7) |
| Layer 2 | the undriven live run + pairs (§5) | not attempted (no tuple to reproduce) |
| Golden | Case A: byte-identical (§6.1) | the no-golden-change statement (§7.2) |
| Legs | FU329-L1…L11 (§9) | FU329-L1…L11, with L1 narrowed to the distribution |

---

## 3. The bindings

| # | Binding (owner cite) | What it fixes here |
|---|---|---|
| B1 | spec §6 (witness + NO-GO + golden clauses) | the acceptance's shape and the two branches |
| B2 | spec §7 (the honest-stop traps: `lane = 0`, `v0 = 0`, zero place delta, a staged slot/seed cell) | the fail signatures and the stop-and-report rule |
| B3 | spec §8 decisions 1–6 (the undriven vehicle, no staging, measured numbers bind, larger seeds measured, the seed-staging fix is separate, never a staged-slot producer) | the vehicle, the no-staging rule, the fix separation |
| B4 | FU316 §4.3 (the `T(s)` model; `T(0x40) = 0x90`, `s = 0x80 → 0x174`) + §7 L1 (the no-slot seed leg) + §8 (the armer NO-GO) | the arithmetic oracle and the armer class |
| B5 | FU321 §3.4 (`m2_pan_curve`'s closed form), §3.8/G3 (`lane_at_pass ≥ 1`), §4.3–§4.6 (the trace cells and G1–G8), §5 (the re-pin recipe), §6 (the hazards), the review erratum 1 (`\|v0\|`, not `v0`) | the arithmetic pins, the gates, the recipe, the criterion's absolute form |
| B6 | `docs/screens/p32-l2-notes.md` (the drive-only acceptance, the timing limits 1–6) and `docs/screens/p33-l2-notes.md` (segment-aware anchoring, the ball-residual still pair, the global-XTEST rule) | the Layer-2 procedure and the waivers |
| B7 | `docs/ENGINE.md` phase-32/33 registers (`:6294-6560`, `:6715-7183`, the final-review erratum `:7184-7233`) | the re-pin house form, the stratum-set gate precedent, the measured baselines |
| B8 | FU323 §2/§4 (the phase-33 silent bind: `trk=12`, `v0 = 0`, `camera_zero_frames` 1517 / `camera_left_zero` 2081; `segments 0 / changed 0 / off-zero 0`) | the in-run zero-seed control |
| B9 | FU319-L8 (the raw-point focus guard at the camera origin) | the face/target coupling hazard |
| B10 | `tests/test_engine_m2.c`: `m2_pan_curve` `:2467`, the gate struct `:2442-2457`, the segment struct `:2487-2499`, the natural structs `:2572-2627`, the sampler `:3754-3855`, the natural asserts `:3962-3979`, the snapshot `:4219-4257`, the golden compare `:7021-7096` | the exact pin surfaces |

---

## 4. The Layer-1 witness design (Branch A)

### 4.1 The tuple

On the **undriven natural tape** (`M2_KEYS` verbatim, 12 000 steps, ISO present,
no added hold — spec §6/§8.1), a row-04 gate pass at granted frame `g` with the
camera event at `g+1` and the first motion at `g+2` (the sampler's post-frame ring
names the same three frames gate `f−2`, event `f−1`, motion `f`, i.e. `f = g+2`;
the phase-32/33 convention, `tests/test_engine_m2.c:4287-4288`), such that:

| clause | cell (at the gate `g`) | why |
|---|---|---|
| **the row** | `code == 4` | the dispatched row is the record's own action code; a `code != 4` gate is a *different* camera event (the phase-33 seg[1] trap, §8/H3) |
| **the arm is B** | `anim_id != 0x13` | the row byte `0x13` takes arm A (`0x15`, the type-dir seed `<<5`), not the half-line arm (`src/fifa96_loader/fifa96_outfield.c:670-681`) |
| **no height** | `cam_pos_y == 0` | the arm's first condition is `ball_height == 0`, and the staging is `s.ball_height = camera.pos_y` (`fifa96_match_handlers.c:2707`) |
| **no slot** | `has_slot == 0` | the `0x7F056` arm's second condition (`fifa96_outfield.c:692`) |
| **the arm's camera sub-gate** | `\|cam_pos_x\| ≤ 0x630`, `\|cam_pos_z\| ≤ 0xA20` | outside it the arm returns `(0,0)` (`fifa96_outfield.c:693-700`) |
| **the gate passes** | `lane_x ≤ 0x40 − timer81`, `lane_x ≤ bound`, `cam_pos_y ≤ 0x50`, `\|fold(bearing − face7d)\| ≤ 0xAB`, `tracked != id` | the half-line gate set (FU316 §3.1); Δ recomputed only by the port's own `fifa96_entity_angle` + the exact fold, on the **live** face |
| **not degenerate** | `lane_x ≥ 1` | spec §7; `lane == 0` is the zero-offset fold (`fifa96_entity_angle(0,0) = 0x80`, FU321 E12/§3.8) |
| **the seed** | `vel73`/`vel75` non-zero (recorded); `s = (int16)((uint16)w << 2)` per axis | the arm's own seed (`fifa96_outfield.c:697-698`); the word threshold is FU326's algebra, not this doc's |
| **the event moves** | `\|v0\| ≥ 1` (at least one axis non-zero) ⇔ the event frame's `speed > 0`; `event.cam_vel_x/z` = `trunc(s/0xC)` per axis **while the pair's metric ≤ `0x19`** | the absolute form (FU321 review erratum 1 — `v0` is signed); the fast path `F8 = 0xC` (FU321 E6); at/above the `0x19` cap the pair is rebuilt from the bearing and capped (`fifa96_camera.c:771-791`), so the measured pair — not `s/0xC` — is the authority there (§8/H5) |
| **the bind** | `tracked == id`, `timer 0/12`, `ramp 12`, `event_param 0` | the height-0 idle arm + the `0x71D27` bind (FU316 §2.1, FU321 E1/E2) |
| **the run** | `changed_frames == max(fr_x, fr_z)`, `stop − prev_stop == (pl_x, 0, pl_z)`, `pan 0→max(n_x, n_z)`, `pos_y == 0`, per-frame `speed == fifa96_entity_distance(vel_x, vel_z)` | the closed form (`tests/test_engine_m2.c:2467`, FU321 §3.4) + the two-axis stop rule (`src/fifa96_loader/fifa96_camera.c:148`) |
| **the one-shot** | `row04_after_event_frames == 0` | the pan's own tail installs row 5/6 (FU316 §3.2 + the row-6 erratum) |

The **place delta is `T(s)`**, read off the recorded seed via `m2_pan_curve` —
the exact pins are the curve's outputs, never a literal `0x90` (spec §8.3: the
carried table is a check, not an authority). The two-axis form is required
because the no-slot arm sets **both** words: `changed_frames` is the **maximum**
of the two axes' frame counts (the shared `speed == 0` stop), and the stop place
is `(T_x, 0, T_z)`. A single-axis witness degenerates to the phase-31/32/33 pins.

**The exact pins** (literal values from FU328's re-run-once tuple, not from this
doc): the gate frame `g`; the arm cells (`code`, `anim_id`, `cam_pos_y`,
`has_slot`, the camera x/z); the seed words and the derived seed; the event frame
`g+1` with `cam_vel`/`speed`/`timer`/`ramp`/`tracked`; the per-segment
`first_motion`/`stop`/`changed`/stop-place/`pan`/the velocity chain; the gate's
`lane_x`/`bound`/`timer81`/`face7d`/`Δ`/`tracked`/`controlled`/`slot_entity`. The
spec's "`pan 0→4` per segment" is the measured `v0 = 5` instance; the pin is the
curve, so a smaller seed pins `pan 0→n`, `n ∈ 1..4`.

### 4.2 The recorded-vs-asserted split (the phases-32/33 rules)

| class | cells | rule |
|---|---|---|
| **pinned exact** (null tape is deterministic) | the gate's arm cells, seed words and pass terms; the event's cells; the per-segment run cells | asserted literally from the measured tuple (the phase-32 T1 / phase-33 re-pin form) |
| **asserted structural** | `\|v0\| ≥ 1`; `segments ≥ 1`; `lane_x ≥ 1`; `Δ ≤ 0xAB`; the frame order `gate+1 = event`, `event+1 = first motion`; the one-shot; `pos_y == 0`; the phase-2-relative window (the A3 form, `tests/test_engine_m2.c:3977-3979`); the control's precedence (N-W0, below) | predicates, never frame literals |
| **recorded only** (stderr + the extract) | the exact `f`/`step`/`tick`/clock of every anchor; the bearing and the exact Δ; `off_zero_frames` (never the run count — FU317 §3.4); every later segment and every other changed window; the keeper ladder seam; the sample hash; the achieved `(\|seed\|, \|v0\|, T)` per event (spec §8.4) | never asserted; the acceptance wording cites them as measured |

The split exists because the trace and the transcript carry no record cells
(§5.2) and because a pin that freezes today's *measurement* (rather than its
*shape*) turns the next faithful producer into a RED (the phase-33 inversion
lesson, FU321-L11).

### 4.3 Where the pins land (test-side only)

| pin | surface (fresh) | content |
|---|---|---|
| **N-W0 the control** | `struct m2_natural` / `m2_nat_pan` (`:2572-2627`), the A1–A3 block (`:3962-3979`), plus a band-specific bind record | the **band record's** (entity 12) silent bind **reproduces**: the first frame with `tracked == 12` carries `bind_speed == 0`, `bind_vel == (0,0)`, `bind_timer 0/12`, `bind_ramp 12`, `band_code == 4` (the phase-33 baseline, FU323 §2/§4), and the witness segment's event frame is **not** that bind frame. If the band's own bind moves (`\|v0\| ≥ 1`, `segments ≥ 1` at that frame), the frozen baseline is contradicted: **STOP**, not a re-pin |
| **N-W1 the arm** | the gate ring (the `m2_claim_snapshot` form, `:4219-4257`) + the new `vel73`/`vel75` cells from `fifa96_match_entities.h:176` | `code == 4`, `anim_id != 0x13`, `cam_pos_y == 0`, `has_slot == 0`, the camera sub-gate, `tracked != id`, `controlled != id` |
| **N-W2 the gate** | the same ring | the pass terms + `lane_x ≥ 1`, Δ by the port's own helper |
| **N-W3 the seed** | the same ring + the event frame | the words recorded; `s` by the port's `<<2` form; on the unclamped fast path (`speed ≤ 0x19`) `event.cam_vel == trunc(s/0xC)` per axis; `speed > 0` |
| **N-W4 the run** | a `m2_claim_segment`-shaped record (`:2487-2499`) per segment in the natural pass | the curve's per-axis frames/places, the two-axis max, the stop, `pan`, the per-frame metric identity, `pos_y == 0`, the monotone signs = the seed's signs |
| **N-W5 the bounds** | the existing `m2_nat_pan` counters | `oob_frames == 0`, `goal_armed == 0` **for the witness segment** (recorded; asserted only if the measured cumulative place stays inside `\|x\| ≤ 0x6C0`, `\|z\| ≤ 0xAB0` — else it is the armer class, §8/H5) |
| **N-W6 the honesty** | the extract + stderr | every changed window in the whole run recorded (the p32 half-time-reset rule); the number of segments recorded; later segments not asserted |

The sampler has no gate ring today (`m2_natural_sample`, `:3754-3855`) and the
gate struct has no `+0x73/+0x75` cells (`:2442-2457`) — both are the test-side
additions this design specifies. The natural pass stages **only** the tape copy
(`:3862`) and the keeper-ladder observation seam (`:3905-3906`, FU313 §5.2.4);
the acceptance adds **no** staging. That is the source-level form of spec §7's
"a staged slot/seed cell is not a natural witness".

**The stability re-run is already built in:** `run_natural_probe` replays the
natural pass twice and asserts the two `struct m2_natural` byte images are equal
(`tests/test_engine_m2.c:5146-5150`), so every witness cell and every segment
record is exercised twice per `test_engine_m2` run — spec §6's "re-run once for
stability" is satisfied by the harness, not by a manual second drive.

### 4.4 The drive-naming grammar

```
drive   := { tape: M2_KEYS verbatim, steps: 12 000, keys added: none }
anchor  := { f: the granted-frame index (null: deterministic; live: run-relative),
             step: the engine step, rel: step − phase2_step,
             tick: the trace tick (live only) }
witness := "the undriven tape; record id <FU328>; gate f = <FU328> (rel <FU328>);
            event f+1; first motion f+2"
```

1. **Two clocks minimum.** Every anchor is named in `f` **and** `step` (null) or
   `f` **and** `tick` (live); the portable name is `rel = step − phase2_step`,
   because the live phase-2 landing drifts (phase-31 tick 1897, phase-32/33 tick
   1800, the null probe step 1802 — `docs/screens/p32-l2-notes.md` timing limit 2
   and its close-out erratum 1).
2. **The prefix's keys are named by tape index** (`RIGHT 7…47`, `KICK 14/17`,
   `LEFT 47…140`, the act-1 `KICK 1790/1797` — the p31/p32 convention); no
   absolute tick is written into the acceptance.
3. **The record id is read from the bind** (`tracked`), never assumed from the
   band entity; the site names the measured id as a constant (the
   `M2_CLAIM_ENTITY` convention, `:1912`) and asserts the bind equals it.
4. **The arm is a cell, not an inference from `has_slot` alone** (§8/H3).
5. **No staged slot/seed/face cell** (spec §7/§8.2); a witness that needs one is
   a leg + NO-GO.

---

## 5. The Layer-2 design (Branch A)

### 5.1 The vehicle and the run

`DISPLAY=:1 FIFA96_CAMERA_TRACE=/tmp/opencode/fu329/trace.log make game`
(the `Makefile` `game` entry; SDL3; the 960×720 client). Walkthrough: intro
RETURN → front-end BACKSPACE → RETURN (match); then the `M2_KEYS` prefix replayed
by **trace tick** and **nothing added** — the post-phase-2 play is the tape's own
(spec §6: "no added holds"). All keys are **global XTEST** (`xdotool
keydown/keyup` without `--window`): the phase-33 lesson is that the `--window`
XSendEvent route leaves the act-1 KICK's release arm unobserved and the cutscene
stalls (`docs/screens/p33-l2-notes.md` timing limit 1). No lossless clip encoder
runs during the walkthrough (the p32 timing limit 1: the encode load compressed a
trace-tick-driven hold into a catch-up burst).

**The wall-clock waiver (W-a).** FU321 §4.2's rule — schedule anything timed on
the wall clock, never on trace-line arrival — is **waived by construction**:
there is no post-phase-2 drive to schedule. The only timed artefact is the
prefix, which is trace-tick-driven, and the anchors are read from this run's own
trace. The waiver is recorded in the Layer-2 note, not assumed.

### 5.2 The anchors, segment-aware (phase 33's lesson)

The trace cells are camera-only (`src/fifa96_engine/fifa96_match_run.c:4657-4671`:
`f clock ph pos vel spd fspd ev cur trk pan tim ramp rate follow tick`). The
trace **attributes** the motion; it cannot show the arm, the seed words, the
gate or the Δ (FU321-L1) — those stay Layer-1 citations (§4).

- **Segment-aware anchoring.** Every committed pair is anchored to **one named
  segment's** delta run (its PRE→POST trace lines), never a mixed span; the
  phase-33 defect this avoids is the pass-level cells holding the *last*
  segment's values (FU322's erratum 2; FU323's erratum 1; register
  `docs/ENGINE.md:6820-6823`). A pair spanning two segments or a segment and the
  gap is invalid.
- **The gate/event/motion/stop reading.** `ev` is `event_param` (the height), not
  the event code; the pan's signature is `trk` changing to the record id *and*
  `vel` jumping to `v0` while `pan` stays 0; `pan` counts pan **steps**, not
  frames; the run is the maximal changed-`pos` run, never the off-zero count.
- **The strong claim.** The `f=` cells are run-relative; the **tick** agreement
  with the Layer-1 null run's own timeline is the strong claim (the p32/p33
  rule). The pan timeline is absolute in ticks; the phase-2 landing is not.
  Record both and their delta.
- **The whole run.** The extract carries the phase-2 line, every changed-place
  window (including the half-time roll reset — the p32 `f=4141` precedent, and
  any later segment), the committed pair windows, and post-reset sentinels — so
  the claim is not a cherry-pick (the p33 note's own model).

### 5.3 The pairs, the AE, the bbox, the waivers

- **Trace primary, AE relative.** `compare -metric AE` on the `960x591+0+0` crop;
  Gate 1 is `AE_pan > AE_still`, both same session/build/same run; no absolute
  floor (FU309-L6). The HUD clock crop must be AE 0 within a pair unless the
  pair-width waiver below is invoked.
- **Same displayed second.** Both pan frames on one displayed second **and**
  inside the named segment's delta run (the p32/p33 rule).
- **The still control** is a same-run, no-key, camera-constant (trace cells)
  pair on one second. It need not be pixel-identical: the p33 ball residual
  (AE 29 120, bbox confined to the ball, `200×200+416+347`) is the precedent;
  the camera-constant property is carried by the trace cells and the residual
  attributed by the bbox, never tuned away (FU325-L8).
- **The pair-width waiver (W-b).** A `|v0| = 1` no-slot pan is 6 granted frames ≈
  0.2 s (the `m2_pan_curve` form), so a same-second pair may not exist. The
  **primary** route is the p31 clip fallback (60 fps, ~12 clip frames inside the
  segment) picking two frames 0.05–0.1 s apart inside one displayed second. If
  even that fails, the waiver is: both frames inside one named segment, the
  displayed second recorded (a one-second straddle allowed), the clock-crop AE
  recorded as waived, and the note says the AE is a *weaker* corroboration for
  this witness. The waiver never weakens the trace.
- **Gate 2 (whole-scene bbox) fails and cannot pass in this build**: the renderer
  draws sprites on black and the scene stage copies only pos/anim_id/frame/hidden
  (`fifa96_match_run.c:1743-1756` — no face, no pitch), so every diff is
  sprite-limited (FU313-L4/FU305-L6; the FU323 §7 drawn-facing note). The bbox is
  recorded, never an attribution.
- **The visible-shift band** scales with the seed (FU321-L7): record the expected
  shift from the **measured** `T`, not the phase-31 numbers.

### 5.4 The artifact set and the verdict wording

`docs/screens/p34-l2-notes.md` (the p31/p32/p33 format: the run, the trace table,
the segment-anchored AE table, the verdict, the timing limits found, the
invariants) + the pairs (`p34-l2-pan-a/b.png` per segment, `p34-l2-still-a/b.png`,
the diff PNGs) + the trace extract `p34-l2-trace-pan.txt`. The verdict is worded
**"a no-slot pan reproduced live under the undriven tape"** — never "the trace
shows the seed" and never "the natural route pans" without the measured tuple.

---

## 6. The golden governance

### 6.1 Case A — the expected case: the acceptance cannot move the golden

M2 is produced by `run_scenario`/`run_tape` (`tests/test_engine_m2.c:3714-3718`)
and compared in `main` (`:7090-7096`, `report_diff` at `:6509`). The natural pass
is a **separate** run inside `run_natural_probe` (`:5146-5161`) and writes no
transcript. A test-side witness pin therefore cannot change a byte of M2, and the
Acceptance records the byte-identity as a gate:

```sh
# CWD = repo root, ISO present
./build/test_engine_m1 | cmp - tests/golden/engine/m1-frames.txt   # silent
./build/test_engine_m2 | cmp - tests/golden/engine/m2-frames.txt   # silent
sha256sum tests/golden/engine/m1-frames.txt tests/golden/engine/m2-frames.txt
make check                                                          # 111/111
```

This is the phase-32 Case A precedent (FU321 §5.1), not a new policy, and it is
also the plan's global constraint: **this wave may not re-pin**; an expected move
is analysed, never applied.

### 6.2 Case B — only a separately pinned seed-staging fix may move M2

If FU326's audit finds a measured divergence between the port's seed staging and
the native bytes, that fix is its **own** commit (spec §5 decision 5; spec §8.5),
with its own RED/GREEN, and it — not the acceptance — is the only thing in this
phase that can move the golden. It lands under the faithful-move recipe:

| step | command / artefact | expected shape |
|---|---|---|
| 1. the fix's own pin | a unit pin of the seed-staging cell (the `fifa96_outfield_row04_state` seed words → the event pair), RED on the pre-fix tree | a value mismatch, not a compile error |
| 2. the early-warning fixtures **first** | the m-42 band/link scenario (`:2954-3080`): `pos_z == 0xB28`, `vel_z == 5`, `goal_armed == 1`, `zone 1` (`:3198-3219`); a seed/decay/arm change breaks them **loudly** | a genuine RED, never a silent tape move (FU321 §5.4 item 2) |
| 3. the candidate transcript | `./build/test_engine_m2 > /tmp/opencode/fu329/m2-new.txt` (ISO present) | — |
| 4. **the first-diff rule** | `cmp tests/golden/engine/m2-frames.txt …/m2-new.txt` → char/line; the line's `frame=` id must **equal** the first frame the *measured* patched probe moved (FU323's 852-on-the-same-patch precedent, FU325's erratum 1 wording) | any id earlier than the measured first frame = **STOP** |
| 5. the `state=`-suffix check | `diff <(cut -d' ' -f3- golden) <(cut -d' ' -f3- new)`; `grep -c ' state=' golden new` equal | empty / equal; a moved or new tag = **STOP** |
| 6. the stratum-set gate | in-test at `:7021-7089` (the phase-33 **Outcome B** precedent: the 10-tag registered set; a NEW tag fails the lookup, a LOST tag fails the all-seen assert) | no stratum added or removed |
| 7. the prefix analysis | a move inside the first 2 522 lines is **not** an additive extension and must be argued in the written reason (the additivity gate is gone — FU321-L6; it was tautological and was replaced in phase 33) | argued, or **STOP** |
| 8. the written reason | the house form in the `tests/test_engine_m2.c` provenance block (`:1716-1749`): old → new sha (both 64-hex) + bytes/lines; the first/last differing line and its `frame=`; the contiguous range; the moved columns (`hash=` only or more); the strata; the anchors crossed (m 41 / m 42 (`M2_M_BAND`, `:1808`) / m 58 (`M2_M_LINK`, `:1809`) / the v21 prefix); the intended consequence (the measured first gate/motion frame and the seed class); the collateral classified | complete |
| 9. M1 immutable | `./build/test_engine_m1 \| cmp - …/m1-frames.txt` | silent; a move is a regression report, never an upgrade (AGENTS.md) |
| 10. the re-pin + the register | `./build/test_engine_m2 > tests/golden/engine/m2-frames.txt`; the register pin in `docs/ENGINE.md` + `docs/PROGRESS.md` in the same commit | one commit |

**Tripwires (all stop-and-report, spec §7).** TW1 the first diff is earlier than
the measured first frame; TW2 a `state=` suffix moved; TW3 a `frame=` id changed
or a stratum appeared/disappeared; TW4 M1 moved; TW5 a moved frame the seed
staging cannot explain (the classification is against the measured gate list, not
against prose); TW6 the m-42/link fixtures went RED without a measured seed
change; TW7 the fix rode the acceptance's commit (the separation rule).

### 6.3 If FU328 returns NO-GO

**NO GOLDEN CHANGE, stated as a deliverable:** M1 `09b726b7…` (22 004 B, 691
lines) and M2 `d5446144…` (295 087 B, 6 522 lines) are byte-identical; there is
no re-pin, no frame diff, no fixture re-staging and no `state=` change; the
wave's only repo writes are the four draft docs. The census stays `60/80 = 75%`
(§6.4).

### 6.4 The pins and the invariants (fresh this session)

| golden | sha256 | bytes | lines |
|---|---|---|---|
| M1 `tests/golden/engine/m1-frames.txt` | `09b726b7a7510facffef7883926e55b44f69f8d61385eb3baa0c269c8006a2cd` | 22 004 | 691 |
| M2 `tests/golden/engine/m2-frames.txt` | `d5446144f08c22c5a4d986e1d07529068f715ff8b8e2cd86411ee493ea804ad8` | 295 087 | 6 522 |

Census **`60/80 = 75%`** (`37 action + 23 phase`; `docs/ENGINE.md:8`, `:268`;
README `:1`). The phase wires **no** action row — the seed switch and the
acceptance add no dispatch — so the README title is neither recomputed nor
hand-tuned (AGENTS.md). `make check` is recorded as 111/111 by the phase-33
register (`docs/ENGINE.md:7176-7178`); Stage 2 re-runs it and records the count
of its own tree.

---

## 7. The NO-GO record design (Branch B)

### 7.1 What lands

1. **The measured-distribution register** (docs): FU327's gate-pass census
   verbatim — every row-04 pass's slot state, arm, seed word, `v0`, event, place
   delta; the no-slot instance count; the maximum `|seed|`, `|v0|`, `T`; the slot
   controls — plus FU326's port-fidelity verdict and the **reproduced phase-33
   silent bind** as the control (`trk=12`, `v0 = 0`, `camera_zero_frames` 1517 /
   `camera_left_zero` 2081, `segments 0 / changed 0 / off-zero 0`).
2. **The remaining legs, owner-named:** FU316-L1 narrowed to the distribution
   evidence (its port owner `fifa96_outfield.c:685-701` + `row04_step`), FU316-L7
   the natural frequency, plus this doc's FU329-L2/L3/L6/L9/L11 as applicable.
3. **No test change.** The phase-33 A1–A3 set already pins the bind's existence
   (`:3962-3979`); the recorded bind cells stay recorded. A *new* assert of
   today's zero seed would pre-commit the acceptance to the no-witness world and
   invert the day a witness appears (the phase-33 inversion lesson, FU321-L11) —
   so it is deliberately not taken.

### 7.2 The no-golden-change statement (verbatim form)

> No golden change. M1 `09b726b7…` (22 004 B, 691 lines) and M2 `d5446144…`
> (295 087 B, 6 522 lines) byte-identical (`cmp`-verified); no re-pin, no frame
> diff, no fixture change. The census holds `60/80 = 75%`; no row is wired. The
> wave's only repo writes are `docs/ghidra/FU326…FU329`.

---

## 8. The hazards

| # | hazard | authority | tripwire / rule |
|---|---|---|---|
| **H1** | **The `lane = 0` knife edge.** A pass at the camera (`(cam_dx,cam_dz) = (0,0)`) is the zero-offset fold (`fifa96_entity_angle(0,0) = 0x80`) — the phase-31/32 false pass. spec §7 makes it NO-GO. For the no-slot arm the seed is **offset-independent**, so such a pass could move for real; that conflict is **not** silently accepted *or* silently discarded — it is a stop-and-report (FU329-L2). | FU321 §3.8/E12; spec §7 | G3: `lane_x ≥ 1` asserted; a `lane = 0`-only measurement is a leg + NO-GO |
| **H2** | **The zero-seed silent bind.** `\|w\| ≤ 2` ⇒ `\|s\| ≤ 8` ⇒ `v0 = 0`: the event binds `trk`, `tim 0/12`, `ramp 12`, and the place never moves; the AE still passes because the scene animates. This is *today's measured baseline* (the phase-33 bind), so the acceptance must be worded "moves **off** the baseline". | FU321 §6.8/L2; FU323 §2/§4 | N-W0 (the band bind reproduces as the control) + N-W3 (`speed > 0`) + N-W4 (`segments ≥ 1`, `changed_frames ≥ 6`); a `trk` set with `pan = 0` and no run = the fail signature |
| **H3** | **The staged-slot trap and the `has_slot`-only misread.** A fixture that gives the witness record a slot silently takes `0x7F035` (`±0x40`) instead of `0x7F056`; and `has_slot 0` alone is **not** the arm — the phase-33 driven seg[1] gate has `has_slot 0` but is a `code == 3` **walk** event (`:4980`, register `:6846-6848`). | FU316 §4.3; construct from `fifa96_outfield.c:689-700` | N-W1: `code == 4`, `anim_id != 0x13`, `cam_pos_y == 0`, `has_slot == 0`, **and** the seed words recorded; the record id comes from the bind, not the fixture |
| **H4** | **The moving-record face/target coupling.** A non-zero seed requires a **moving** record; a moving record with a far target (`move_attr >> 16 > 0x60`) turns its face to the **movement direction**, not the camera, so Δ may fail; a settled record faces the camera but its stride words decay to ±1. At the undriven camera origin the port's raw-point focus guard (`face_x != 0 \|\| face_z != 0`) disables the focus branch entirely (FU319-L8) — the native delta test would steer to `(0,0,0)`. | `fifa96_action_handlers.c:219-231`; FU319-L8; FU316 §3.3 | Δ is evaluated on the **live** face, never staged; a witness that requires staging the face is a leg + NO-GO; the admitted face and the movement direction are recorded, and the reason is the measured coupling, not a tuned drive |
| **H5** | **The armer reach (`0x730`), the OOB edge (`0x6C0`) and the `0x19` velocity cap.** The no-slot arm admits larger seeds than the measured `0x40`, so `T(s)` can exceed the phases-31/32/33 `\|x\| = 0x90` assumption, and a **chained** cumulative place can cross `\|x\| > 0x6C0` (the OOB handler) or `\|x\| > 0x730` / `\|z\| > 0xB20` (the armer). Separately, a seed whose fast-path pair exceeds a metric of `0x19` has its velocity **rebuilt from the bearing and capped** (`fifa96_camera.c:784-791`), so `v0 = trunc(s/0xC)` no longer describes the event pair. | FU316 §4.3/§5.1/§8; FU321 E24; `fifa96_camera.c:779-791` | the acceptance pins the **per-segment** `T(s)` from the **measured** `cam_vel` pair (never from `s/0xC` when `speed == 0x19`) and the `pan` count, records the cumulative place, and asserts `goal_armed`/`oob` **only** for the witness's own accumulation; a crossing is the armer class (a recorded consequence, a leg), never a failure and never an `goal_armed == 0` claim |
| **H6** | **The m-42 fixture class.** The staged band fixture is a **slot** record (`has_slot == 1`) with `vel73/vel75 = 0` and a staged `face7d = 0` (equal to the pre-fix constant, so insensitive to re-staging); the link assert pins `pos_z 0xB28`, `vel_z 5`, `goal_armed 1`, `zone 1`. It cannot stand in for the no-slot witness, and any seed/decay/arm change breaks it loudly. | `tests/test_engine_m2.c:3035-3048`, `:3198-3219`; FU321 §5.4 items 1-3/E21/E22 | the witness is a **separate** pin (or none); the staged cells stay declared producer inputs; a camera/face gate staged to force a tape is forbidden (spec §7/§8.2) |
| **H7** | **The census/README invariants.** `60/80 = 75%`; the README title is derived, never hand-tuned; the phase wires no row. | AGENTS.md; `docs/ENGINE.md:8`, `:268`; README `:1` | grep the census headline before/after; a wired-count change without a fresh census = NO-GO; a README recompute without one = forbidden |
| **H8** | **The reading traps.** `ev` is the **height**, not the event code; `pan` counts **steps**, not frames; the off-zero count is not the run count; `f=` is run-relative; the trace has no record/seed cells; `p`-values from a different run are not comparable. | FU321 §4.3/§4.6; FU317 §3.4; p32/p33 notes | the extract names its run and its clocks; the AE/bbox never attribute; the seed/arm/Δ stay Layer-1 |
| **H9** | **The live-run hazards.** The `--window` XSendEvent key route fails (the release arm); the clip encoder compresses trace-tick holds; the phase-2 landing drifts; a short pan has a narrow pair window. | p32 timing limits 1–6; p33 timing limit 1–6 | global XTEST; no encoder during the walkthrough; every anchor from this run's trace; W-a/W-b recorded, never assumed |
| **H10** | **The honest-stop rule itself.** Any measurement contradicting a frozen authority (the phase-33 baseline, the `T(s)` form, the arm's identity) becomes a numbered leg with its native owner — never explained away, never "fixed" by staging. | spec §7; FU316 §8; FU325 §7/H9 | every tripwire above lands as a leg + a report; the controller re-scopes |

---

## 9. Numbered legs

1. **FU329-L1 — the witness pin is unwritten until the tuple is measured.** This
   doc fixes the shape, the terms, the split and the fail signatures; the frame
   numbers, the record id and the seed are FU328's tuple (re-run once for
   stability). Owner: FU328 → the Stage-2 P1.
2. **FU329-L2 — the `lane = 0` conflict is a spec decision, not a fixture
   decision.** The no-slot seed is offset-independent, so a `lane = 0` pass with
   a real seed and a real delta is a genuine pan that spec §7 nevertheless
   excludes. If the only measured pass is at `lane = 0`, the phase stops and
   reports; it may be neither accepted as the witness nor worded as a
   zero-delta repeat. Owner: the wave review / the spec.
3. **FU329-L3 — the seed words are not yet a harness cell.** `struct
   m2_claim_gate` (`:2442-2457`) has no `+0x73/+0x75`; the seed is visible only
   indirectly (`has_slot`, `dir`, the event's `cam_vel`). Stage 2 adds `vel73`/
   `vel75` from the pool record (`fifa96_match_entities.h:176`) and cross-checks
   the derived seed against the event's `v0` on the unclamped fast path; a
   mismatch is a **staging divergence** (FU326's separately pinned fix), never a
   re-pin here. Above the `0x19` cap the equality is not required (H5). Owner: the
   Stage-2 P1 + FU326.
4. **FU329-L4 — the Layer-2 pair window for a small `v0`.** A `|v0| = 1` pan is
   6 frames ≈ 0.2 s; the same-second rule may be unsatisfiable. Waiver W-b is the
   option (recorded as weaker corroboration); the alternative is Layer-2
   NOT-GO-as-measured with the Layer-1 witness standing. Owner: the Layer-2 task.
5. **FU329-L5 — the no-slot Layer-2 has no slot-arm fallback.** The FU320
   LEFT/DOWN drive takes `0x7F035` (the slot arm); substituting it would silently
   change the phenomenon. If the live natural run fails, the natural clause is
   NOT-GO-as-measured (the phase-33 wording, `p33-l2-notes.md:206-210`) and the
   live half stays open. Owner: the Layer-2 task + the register wording.
6. **FU329-L6 — the armer class of a large no-slot seed is unmeasured.** `T(s)`
   and the cumulative place decide whether a chained witness reaches the OOB/armer
   edges; the acceptance records and does not assert them. Owner: FU326 (the seed
   table) + FU328 (the achieved `T`).
7. **FU329-L7 — the natural run's later windows.** The witness may not be the only
   changed-place event (the half-time re-placement, a later band entry); the
   extract covers the whole run and every window is classified. Owner: the
   Layer-2 note.
8. **FU329-L8 — the trace cannot show the arm or the seed (FU321-L1
   continued).** Layer 2 reproduces the drive and the motion only; an AE-only
   claim is not an acceptance. Owner: the acceptance wording.
9. **FU329-L9 — the m-42/link class is a slot-arm fixture.** It cannot stand in
   for the no-slot witness; a "reuse" of its record is the staged-slot trap
   (H3/H6). Owner: the Stage-2 P1.
10. **FU329-L10 — the census/README recompute is out of scope.** No row is wired;
    `60/80 = 75%`. A fresh census that disagrees is its own commit with its own
    derivation (AGENTS.md). Owner: the close-out.
11. **FU329-L11 — Case B is conditional on a fix this wave did not make.** The
    acceptance may not be measured on a tree with a known-divergent seed staging
    (it would pin a wrong `T`); the fix lands first with its own pins and diff.
    Owner: the Stage-2 + FU326.

---

## 10. GO / NO-GO

**GO — the acceptance design.** The witness tuple (§4.1), the split (§4.2), the
pin surfaces (§4.3), the naming grammar (§4.4), the Layer-2 design with its two
declared waivers (§5), the golden policy (§6) and the NO-GO record (§7) are all
implementable from the frozen authority with **no production change, no new native
port, no golden prediction, and not one number invented here**. The arithmetic
oracle is the existing `m2_pan_curve` (`:2467`), which already reproduces the
published instances (FU321 §3.4).

**The Layer-1 GO is conditional on FU328's measured tuple:**
**(C1)** `has_slot 0` with `code == 4`, `anim_id != 0x13`, `cam_pos_y == 0` and
the arm's camera sub-gate — the arm is the no-slot branch, not merely a record
without a slot (H3);
**(C2)** `|v0| ≥ 1` (the absolute form) with the seed words recorded and, on the
unclamped fast path (`speed ≤ 0x19`), the derived seed matching the event's `v0`
(above the cap the measured pair is the authority, H5);
**(C3)** a real delta run: `changed_frames == max(fr_x, fr_z)`, the stop place
`(T_x, 0, T_z)`, `pan 0→n`, `pos_y == 0`;
**(C4)** `lane_x ≥ 1` (spec §7) and the phase-33 silent bind reproduced as the
in-run control (N-W0);
**(C5)** no staged slot/seed/face/camera cell (the source-level check, §4.3).

**The Layer-2 GO is conditional on the Layer-1 witness** and on the live run
reproducing it inside one named segment; otherwise the natural clause is recorded
NOT-GO-as-measured and the live half stays open (L5).

**The golden GO is Case A** (byte-identical; provable by construction, §6.1).
**Case B** is permitted only for a separately pinned seed-staging fidelity fix,
under §6.2 in one commit, with every tripwire armed.

**NO-GO on:** (a) accepting a bind with `v0 = 0` or no delta run (H2); (b) a
`lane = 0` pass worded as the witness (spec §7/H1); (c) staging any slot, seed,
face or camera cell to force the tuple (spec §7/§8.2); (d) substituting the
slot-arm FU320 drive for the no-slot Layer-2 (L5); (e) a golden re-pin without the
frame diff, the `state=`-suffix check, the stratum-set gate and the written
reason in the same commit (TW1–TW7); (f) a move in this wave at all (the plan's
global constraint); (g) an attribution from the AE or the bbox alone; (h)
asserting `goal_armed == 0` where the measured cumulative place crosses the armer
(H5); (i) a README/census change without a fresh census; (j) a witness measured
on a tree with a known-divergent seed staging (L11).

---

## 11. Known limits

1. **No measurement in this slice** (deliberately): the witness, the distribution
   and the algebra are FU328's, FU327's and FU326's; every quantity here is a
   shape or a gate, never a value.
2. **The pins are designed, not written.** The gate ring, the segment records and
   the `vel73/vel75` cells are test-side work; their exact line placements move
   when they land (the phase-33 final-review erratum is the precedent for fresh
   re-cites).
3. **The two-axis arithmetic is the general form**; the phase-31/32/33 pins are
   its single-axis instance (z = 0). A two-axis no-slot witness exercises the
   `speed == fifa96_entity_distance(vel_x, vel_z)` identity (`fifa96_camera.c:148`)
   for the first time — if it fails, that is a measured finding, not a pin to
   loosen.
4. **The Layer-2 numbers are per-run** (the phase-2 drift, the run-relative `f`,
   the short-pan pair window); only same-run pairs and same-drive anchors are
   valid.
5. **The `T(s)` table is a check, not an authority** (spec §8.3): the acceptance
   reads the observed `(|v0|, frames, Δ)` off the recorded seed and compares with
   the curve; a mismatch is a measurement, not a re-derivation.
6. **A staged-slot route does not exist by design.** If the only way to produce
   the tuple is a fixture, the phase is a NO-GO with a leg (spec §8.6) — this doc
   deliberately provides no such fixture.

---

## 12. Provenance and self-audit

**Fresh this session (repo reads only; no writes, no git, no Ghidra, no probe,
no build):** `AGENTS.md`; the spec §1–§9; the plan `:1-117`;
`docs/ghidra/FU316_r2l6_pan_walk.md` (whole incl. the review erratum),
`FU321_r2l6_nonzero_acceptance.md` (whole incl. the review erratum),
`FU323_r2l7_m2_probe.md` (`:1-140`), `FU325_r2l7_acceptance.md` (whole incl. the
errata); `docs/screens/p31-l2-notes.md`, `p32-l2-notes.md`, `p33-l2-notes.md`
(whole); `docs/ENGINE.md` (`:6294-6593`, `:6715-7233`, `:8`, `:268`);
`src/fifa96_loader/fifa96_outfield.c` (`:660-729`);
`src/fifa96_loader/fifa96_camera.c` (`:60-160`, `:732-801`, `:900-957`);
`src/fifa96_loader/fifa96_action_handlers.c` (`:195-274`);
`src/fifa96_engine/fifa96_match_handlers.c` (`:2620-2679`);
`include/fifa96_loader/fifa96_outfield.h` (`:180-259`);
`include/fifa96_engine/fifa96_match_entities.h` (`:140-189`);
`src/fifa96_engine/fifa96_match_run.c` (`:1735-1800`, `:1895-1935`,
`:4575-4673`);
`tests/test_engine_m2.c` (`:1690-1760`, `:2423-2642`, `:2795-2885`,
`:2995-3080`, `:3700-4002`, `:4219-4338`, `:4563-4630`, `:4750-5045`,
`:5140-5175`, `:6509`, `:6940-7100`); README `:1`; the goldens (`sha256sum` +
`wc`, §6.4).

**Carried (owner docs named, never re-derived):** the pan chain and `T(s)`
(FU316 §4.3); the seed switch's three arms and the `0x7F056` condition (FU316
§4.3; FU321 E3); the gate set and the tracked lifecycle (FU316 §2.1/§3.1); the
armer/OOB bounds (FU316 §5.1; FU321 E24); the closed form and its five published
instances (FU321 §3.4); G1–G8 and the attribution rules (FU321 §4.3–§4.6); the
faithful-move recipe and the fixture hazards (FU321 §5); the multi-consumer face,
the zero-seed class and the controlled fallback (FU321 §6); the phase-33 measured
baseline and the silent bind (FU323 §2/§4; register `:6798-6819`); the Layer-2
timing lessons and the segment-aware anchoring (p32/p33 notes; FU322's erratum
2); the stratum-set gate and the phase-33 provenance form (`docs/ENGINE.md`
`:6786-6797`, `:7184-7233`; `tests/test_engine_m2.c:1716-1749`, `:7021-7089`);
FU319-L8 (the focus-guard divergence).

**Self-audit.** Every port/test/native statement above is either (a) a fresh
`file:line` read this session, (b) a *carried* owner-cite with its section named,
or (c) a forward-cite to FU326/FU327/FU328 for a quantity they own — no measured
number is asserted here, no native byte is re-derived, and no prior doc section is
contradicted. The doc's only arithmetic statements are the port's own forms quoted
with their lines (`s = w << 2` at `fifa96_outfield.c:697-698`; `v0 = s/0xC` and
the 6/7 cadence via `m2_pan_curve`; `speed = distance(vel)` at
`fifa96_camera.c:148`), each labelled as a form, never as an achieved value. The
unproven items are numbered: the witness tuple and the record id (FU329-L1), the
`lane = 0` conflict (L2), the seed-word cell (L3), the small-`v0` pair window
(L4), the no-slot Layer-2's missing fallback (L5), the armer class (L6), the later
windows (L7), the trace's blindness (L8), the m-42 class (L9), the census (L10)
and the fix separation (L11). This doc creates one repo file (this draft), runs
no git command, writes nothing to `build/`, `tests/golden/` or the ISO, and runs
no probe.

## Review erratum (wave review, 2026-10-23)

**1. The `s = 0x80 → 0x174` row (§3 B4).** Quote: "FU316 §4.3 (the `T(s)` model; `T(0x40) = 0x90`, `s = 0x80 → 0x174`)". **Corrected:** the `0x174` row is arithmetic-only and is **not reachable from the no-slot arm**; the no-slot seed is `4w` with `|w| ≤ 15` in the locomotion domain → `|s| ≤ 0x3C`, `T ≤ 0x90` (FU326 §4.2/§8; FU321 §3.5 already carried the same bound). Keep `T(0x40) = 0x90` as the arithmetic oracle; the `0x174` row is a slot/arithmetic check only.

**2. H5's larger-seed premise.** Quote: "The no-slot arm admits larger seeds than the measured `0x40`, so `T(s)` can exceed the phases-31/32/33 `|x| = 0x90` assumption". **Corrected:** inverted. The no-slot maximum is `|s| = 0x3C < 0x40`, so `T(s) ≤ 0x90`; FU327 measured `|w| ≤ 1` on every natural record-frame. The **cumulative** place can still cross the OOB/armer bounds over chained pans (13 x / 20 z at `T = 0x90`), so H5's rule (record the cumulative place; assert only the witness's own accumulation) stands; its premise does not. L6's "large no-slot seed" should read "chained no-slot pans".

**3. H3's example.** Quote: "the phase-33 driven seg[1] gate has `has_slot 0` but is a `code == 3` **walk** event (`:4980`, register `:6846-6848`)". **Corrected:** the firing record is **entity 1** (`code 4`, `has_slot 1`, seed `−0x40`, `trk 1`); the `code 3`/`has_slot 0` cell is the harness's **entity-2** pool snapshot (`m2_claim_snapshot`, `M2_CLAIM_ENTITY = 2`), not the firing gate (FU327-L3; the FU323 erratum). The rule (the arm is a cell on the firing record, never inferred from `has_slot` alone) stands; the example must not be reused.

**4. The M2 exposure and the leg hand-off (§2, §6.2, §7.1).** Quote: "The only M2 exposure is a *seed-staging fidelity fix* if FU326's audit finds a measured divergence" and "Case B — only a separately pinned seed-staging fix may move M2". **Corrected:** this phase's exposure is empty (FU326 found parity), but a future `+0x79` producer (FU328-L1) is a separate, pinned, **golden-moving** fidelity change (FU328-L8) under the same faithful-move recipe; it is not this phase's Case B and must not be folded into the acceptance. The NO-GO register (§7.1) must also carry **FU326-L1** (the non-locomotion writers), **FU327-L1…L7** (the distribution, the FU323 corrections, the replica predicate) and **FU328-L1…L8** (the `+0x79` producer, the row-03 counter, the native value, the stall, the finite families, the filter/gate offset, the arm classifier, the M2 exposure) — the current list names only FU316-L1/L7 + FU329-L2/L3/L6/L9/L11.

**5. The m-42/link cite (§5.4 step 2).** Quote: "`pos_z == 0xB28`, `vel_z == 5`, `goal_armed == 1`, `zone 1` (`:3198-3219`)". **Corrected:** `vel_z == 5` is at `:3072` and `camera.pos_z == 0xB28` at `:3071` (the m-42/link block `:2954-3080`); `:3198-3219` pins the goal/`0xB32` frame. The cells exist; the line range is split.
